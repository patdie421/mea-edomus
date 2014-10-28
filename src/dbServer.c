/*
 *  tomysqldb.c
 *
 *  Created by Patrice Dietsch on 13/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include <mysql.h>
#include <sqlite3.h>

#include <errmsg.h>

#include "dbServer.h"

#include "globals.h"
#include "debug.h"
#include "macros.h"
#include "memory.h"

#include "processManager.h"
#include "notify.h"

tomysqldb_md_t *_md=NULL;
int _dbServer_monitoring_id=-1;


int tomysqldb_connect(tomysqldb_md_t *md, MYSQL **conn);


void free_value(void *data)
/**
 * \brief     libère les données allouées (malloc) pour un variable de structure sensors_values_s.
 * \param     data   pointeur anonyme sur une structure sensors_value_s
 */
{
   struct sensor_value_s *value;
   
   value=(struct sensor_value_s *)data;
   if(value->complement)
   {
      free(value->complement);
      value->complement=NULL;
   }
   free(data);
   data=NULL;
}


int16_t tomysqldb_add_data_to_sensors_values(tomysqldb_md_t *md, uint16_t sensor_id, double value1, uint16_t unit, double value2, char *complement)
/**
 * \brief     Récupère les données de type "sensors values" pour stockage dans la table sensors_values de la base mysql.
 * \details   En dehors des données en provenance des capteurs la date courrante est rajoutée par cette fonction.
 * \param     sensor_id  identifiant du capteur
 * \param     value1     valeur principale
 * \param     unit       identifiant de l'unité de mesure
 * \param     value2     une valeur complémentaire optionnelle
 * \param     specific   un complément de données au format text optionnel
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   struct sensor_value_s *value;
   tomysqldb_queue_elem_t *elem;
   struct timeval tv;
   gettimeofday(&tv, NULL);

   if(!md)
      return -1;

    if(!md->opened) // md initialisé mais connexion avec Mysql jamais établie (peut-être un pb de paramétrage ???)
      return -1;    // on s'arrête pour ne pas remplir la mémoire
   
   value=(struct sensor_value_s *)malloc(sizeof(struct sensor_value_s));
   if(!value)
   { 
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return -1;
   }
   
   value->sensor_id=sensor_id,
   memcpy(&(value->date_tv),&tv,sizeof(struct timeval));
   value->value1=value1, // valeur principale
   value->unit=unit, // code unité de mesure (s'applique à la valeur principale)
   value->value2=value2, // valeur secondaire
   value->complement=NULL;
   if(complement)
   {
      value->complement=malloc(strlen(complement)+1);
      strcpy(value->complement,complement);
   }
   
   elem=malloc(sizeof(tomysqldb_queue_elem_t));
   if(!elem)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      if(value)
      {
         if(value->complement)
         {
            free(value->complement);
            value->complement=NULL;
         }
         free(value);
         value=NULL;
      }
      return -1;
   }
   elem->type=TOMYSQLDB_TYPE_SENSORS_VALUES;
   elem->data=(void *)value;
   elem->freedata=free_value;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(md->lock));
   if(!pthread_mutex_lock(&(md->lock)))
   {
      VERBOSE(9) fprintf (stderr, "%s  (%s) : data to queue(%ld) (sensor_id=%d, value1=%f)\n",INFO_STR,__func__,md->queue->nb_elem,sensor_id,value1);
      in_queue_elem(md->queue,(void *)elem);
      pthread_mutex_unlock(&(md->lock));
   }
   else
   {
      free_value(elem->data);
      free(elem);
      elem=NULL;
   }
   pthread_cleanup_pop(0);

   return 0;
}  


void _tomysqldb_free_queue_elem(void *d) // pour vider_file2()
/**
 * \brief     libère proprement le contenu d'un élément de file.
 * \details   fonction appelée par le gestionnaire de file.
 */
{
   tomysqldb_queue_elem_t *e=(tomysqldb_queue_elem_t *)d;
   
   if(e)
   {
      if(e->data && e->freedata)
         e->freedata(e->data);
      e->data=NULL;
      free(e);
      e=NULL;
   }
}


int exec_mysql_query(MYSQL *conn, char *sql_query)
/**
 * \brief     Execute une commande sql dans Mysql.
 * \param     conn       descripteur de la base mysql
 * \param     sql_query  requête (texte SQL) à stocké dans la base sqlite3
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   int ret;
   
   ret=mysql_query(conn, sql_query);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(conn), mysql_error(conn));
      return -1;
   }
   return 0;
}


int move_sqlite3_queries_to_mysql(sqlite3 *db, MYSQL *conn)
/**
 * \brief     récupère les requêtes en attente dans la base sqlite3 et les envoie au serveur mysql pour traitement.
 * \details   Les requêtes sont transmise sans transformation. Elles sont sorties de la base sqlite3 que si le serveur mysql les à traiter sans erreur.
 * \param     db    descripteur de la base sqlite3
 * \param     conn  descripteur de la base mysql
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   char sql[255];
   sqlite3_stmt * stmt;
   int ret;
   
   sprintf(sql,"SELECT * FROM queries ORDER BY id");
   ret = sqlite3_prepare_v2(db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      return -1;
   }

   while (1)
   {
      int s;
      
      s = sqlite3_step (stmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *query, *id;
         
         id     = sqlite3_column_text (stmt, 0);
         query  = sqlite3_column_text (stmt, 1);
         
         ret=mysql_query(conn, (const char *)query);
         if(ret)
         {
            VERBOSE(1) fprintf (stderr, "%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(conn), mysql_error(conn));
            return -1;
         }
         else
         {
            VERBOSE(9) fprintf(stderr,"%s  (%s) : mysql_query = %s\n",INFO_STR,__func__,query);
         }

         sprintf(sql,"DELETE FROM queries WHERE id=%s", id);
         ret = sqlite3_exec(db,sql,0,0,0);
         if(ret)
         {
            VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
            return -1;
         }
      }
      else
      {
         if (s == SQLITE_DONE)
         {
            sqlite3_finalize(stmt);
            break;
         }
         else
         {
            VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
            sqlite3_finalize(stmt);
            return -1;
         }
      }
   }

   return 0;
}


int move_mysql_query_to_sqlite3(sqlite3 *db, char *sql_query)
/**
 * \brief     transfert la requête sql vers la base sqlite3 pour être lancée ultérieurement
 * \details   La requête est "bêtement" stockée dans la base sqlite3 au format "SQL". Les données ne sont pas exploitable dans la base sqlite3.
 * \param     db              descripteur de la base sqlite3
 * \param     sql_query       requête (texte SQL) à stocké dans la base sqlite3
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   char sql[255];
   int ret;
      
   int16_t n=snprintf(sql,sizeof(sql),"INSERT INTO queries (request) VALUES ( '%s' )", sql_query);
   if(n<0 || n==sizeof(sql))
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   ret = sqlite3_exec(db,sql,0,0,0);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", ERROR_STR,__func__,sqlite3_errmsg(db));
      return -1;
   }
   return 0;
}


uint16_t build_query_for_sensors_values(char *sql_query, uint16_t l_sql_query, void *data)
/**
 * \brief     créer une requête SQL pour alimenter la table sensors_values.
 * \param     sql_query    chaine qui contiendra la requête (alloué par l'appelant)
 * \param     l_sql_query  taille max de la chaine
 * \param     data         structure contenant les données à insérer dans la base
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   char time_str[80];
   struct tm t;
   struct sensor_value_s *sensor_value;
   int16_t n;
   
   sensor_value=(struct sensor_value_s *)data;
   
   localtime_r(&(sensor_value->date_tv.tv_sec), &t);
   n=strftime(time_str,sizeof(time_str)-1,"%y-%m-%d %H:%M:%S",&t); // format compatible mysql
   if(n==0)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : strftime - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }   
   n=snprintf(sql_query,
              l_sql_query,
              "INSERT INTO sensors_values (sensor_id, date, value1, unit, value2, complement) VALUES ( %d,\"%s\",%f,%d,%f,\"%s\" )",
              sensor_value->sensor_id,
              time_str,
              sensor_value->value1, // valeur principale
              sensor_value->unit, // code unité de mesure (s'applique à la valeur principale)
              sensor_value->value2, // valeur secondaire
              sensor_value->complement
   );
   if(n<0 || n==l_sql_query)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   else
      return 0;
}


int write_data_to_db(tomysqldb_md_t *md, int mysql_connected, MYSQL *conn, sqlite3 *db, tomysqldb_queue_elem_t *elem)
/**
 * \brief     écrit les données vers une table en fonction du type d'élément récupéré dans la file de traitement
 * \param     md              descripteur du gestionnaire. Il est alloué par l'appelant.
 * \param     mysql_connected flag msql connecté ou nom (pour choisir la destination)
 * \param     conn            le descripteur allouée par la librairie Mysql
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   char query[255];

   if(!md) // pas de descripteur == pas de stockage dans la base ...
      return 0;
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : Insertion data type %d\n",INFO_STR,__func__,elem->type);
   
   switch(elem->type)
   {
      case TOMYSQLDB_TYPE_SENSORS_VALUES:
         build_query_for_sensors_values(query, sizeof(query), elem->data);
         break;

      default:
         return 0;
         break;
   }
   
   if(mysql_connected==1)
      exec_mysql_query(conn, query);
   else
      if(db)
         move_mysql_query_to_sqlite3(db, query);
      else
         return -1;

   return 0;
}


void *tomysqldb_thread(void *args)
/**
 * \brief     Thread de tomysql
 * \details   Les insersions de données dans la base Mysql sont traitées de façon asynchrone. Les "clients" postent les demandes de stockage de données dans une file qui est parcourues régulièrement et vidée dans la base.
 *            Le thread gère la disponibilité des bases Mysql et sqlite3. Si aucune des bases n'est disponible les données sont gardées en mémoire jusqu'à épuisement de la mémoire.
 * \param     args  pointeur (anonyme) sur une structure md (le descripteur)  
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   int ret;
   
   tomysqldb_queue_elem_t *elem;
   tomysqldb_md_t *md=(tomysqldb_md_t *)args;

   int mysql_connected=0; // indicateur connexion active (1) ou inactive (0)
   unsigned long _mysql_thread_id,_mysql_thread_id_avant;
   
   md->db=NULL; // descritpteur SQLITE
   md->conn=NULL; // descripteur com. MYSQL
   md->opened=0;
   
   ret=tomysqldb_connect(md, &md->conn);
   if(ret)
      mysql_connected=0;
   else
   {
      md->opened=1;
      mysql_connected=1;
   }

   
   while(1)
   {
      int nb=0;
      
      process_heartbeat(_dbServer_monitoring_id);

      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(md->lock));
      if(!pthread_mutex_lock(&(md->lock)))
      {
         nb=md->queue->nb_elem;
         pthread_mutex_unlock(&(md->lock));
      }
      pthread_cleanup_pop(0);
         
      if(nb) // s'il y a quelque chose à traiter
      {
         if(mysql_connected == 1) // on a déjà été connecté un jour ...
         {
            _mysql_thread_id_avant=mysql_thread_id(md->conn); // on récupère d'abord l'id du thread, il sera utile pour savoir s'il y a eu une reconnexion.

            // on s'assure d'abord que la connexion avec le serveur Mysql est encore possible
            ret=mysql_ping(md->conn); // le ping pour éventuellement forcer une reconnexion
            if(ret) // pas de réponse au ping et reconnexion impossible.
            {
               VERBOSE(5) fprintf (stderr, "%s  (%s) : mysql_ping - %u: %s\n",INFO_STR,__func__,mysql_errno(md->conn), mysql_error(md->conn));
               mysql_connected = 0; // plus de connexion au serveur mysql
            }
            else
            {
               _mysql_thread_id=mysql_thread_id(md->conn);      // id du thread après mysql ping
               if(_mysql_thread_id_avant!=_mysql_thread_id) // que l'on compare avec l'ancien id
               {
                  // si différent, une reconnexion à eu lieu :
                  // faire ce qu'il y a a faire en cas de reconnexion
                  // voir ici pour les info : http://dev.mysql.com/doc/refman/5.0/en/auto-reconnect.html
                  // pour l'instant on ne fait rien
                  VERBOSE(9) fprintf(stderr,"%s  (%s) : Une reconnexion à la base Mysql à eu lieu\n", INFO_STR,__func__);
               }
            }
         }      
         
         if(mysql_connected == 0) // jamais connecté ou plus de connexion, on essaye de se connecter
         {
            ret=tomysqldb_connect(md, &md->conn);
            if(ret)
               mysql_connected=0; // toujours pas connecté
            else
            {
               md->opened=1;
               mysql_connected=1; // ouf, reconnecté
            }
         }
         
         if(mysql_connected == 0) // toujours pas de connexion Mysql. Repli sur sqlite3, ouverture si nécessaire
         {
            // DEBUG_SECTION fprintf(stderr,"SQLITE3_DB_PATH = %s\n",md->sqlite3_db_path);
            if(!md->db) // sqlite n'est pas ouverte
            {
               ret = sqlite3_open (md->sqlite3_db_path, &md->db);
               if(ret)
               {
                  md->db=NULL;
                  VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (md->db));
               }
            }
         }
         else // une connexion au serveur mysql est active
         {
            char sql_query[255];
            
            sprintf(sql_query,"USE %s", md->base); // et on utilise la base "base"
            ret=exec_mysql_query(md->conn, sql_query);
            if(ret)
            {
               VERBOSE(2) fprintf (stderr, "%s (%s) : sql_query - %u: %s\n", ERROR_STR,__func__,mysql_errno(md->conn), mysql_error(md->conn));
            }

            if(md->db) // sqlite est encore ouvert, il faut vider la table des requetes si elle n'est pas vide
            {
               // transferer le contenu de la base sqlite vers la base mysql ici :
               move_sqlite3_queries_to_mysql(md->db, md->conn);
               
               sqlite3_close(md->db); // et on la referme
               md->db=NULL;
            }
         }

         while(nb>0)
         {
            pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(md->lock));
            if(!pthread_mutex_lock(&(md->lock)))
            {
               last_queue(md->queue); // on se positionne en fin de queue
               current_queue(md->queue,(void **)&elem); // on lit le dernier element de la file sans le sortir (on le sortira s'il est inséré dans une base
                  
               ret=write_data_to_db(md, mysql_connected, md->conn, md->db, elem);
               if(!ret)
               {
                  out_queue_elem(md->queue,(void **)&elem);
                  nb=md->queue->nb_elem;
                  if(elem->data && elem->freedata)
                  {
                     elem->freedata(elem->data);
                     elem->data=NULL;
                  }
                  if(elem)
                  {
                     free(elem);
                     elem=NULL;
                  }
               }
               else // on peut écrire dans aucune base ...
                  nb=-1; // on force la sortie de la boucle, on verra plus tard.
               pthread_mutex_unlock(&(md->lock));
            }
            else
               nb=-1;
            pthread_cleanup_pop(0);
         }
      }
      
      pthread_testcancel();
      sleep(10);
//      DEBUG_SECTION {
//         printf("Boucle tomysqldb\n");
//      }
   }
   md->started=0;
   mysql_close(md->conn);
   sqlite3_close(md->db);
   md->db=NULL;
   pthread_exit(NULL);
}


int tomysqldb_connect(tomysqldb_md_t *md, MYSQL **conn)
/**
 * \brief     Initialise la communication avec un serveur Mysql
 * \details   Cette fonction permet de réinitialiser une communication avec un serveur déjà atteint (si conn != NULL).
 * \param     md              descripteur du gestionnaire. Il est alloué par l'appelant.
 * \param     conn            le descripteur allouée par la librairie Mysql
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
 my_bool reconnect = 0;
 int ret;
   
   if(*conn)
      mysql_close(*conn);

   // initialisation
   *conn = mysql_init(NULL);
   if (*conn == NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // récupération du port si disponible
   char *end;
   uint16_t port=3306;
   if(md->db_server_port[0])
   {
      port=strtol(md->db_server_port,&end,10);
      if(*end!=0 || errno==ERANGE)
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : port (%s) incorrect\n", ERROR_STR,__func__,md->db_server_port);
         port=0;
      }
   }
      
   // connexion à mysql
   if (mysql_real_connect(*conn, md->db_server, md->user, md->passwd, md->base, port, NULL, 0) == NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // on authorise les reconnexions automatiques pour que le ping puisse gérer la connexion
   ret=mysql_options(*conn, MYSQL_OPT_RECONNECT, &reconnect);
   if(ret)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }

   return 0;
}


int tomysqldb_init(tomysqldb_md_t *md, char *db_server, char *db_server_port, char *base, char *user, char *passwd, char *sqlite3_db_path)
/**
 * \brief     Initialise et démarre un gestionnaire de base de données mysql
 * \details   Tous les paramètres sont obligatoires. En cas de problème de communication avec le serveur mysql, les requêtes sont stockées dans une base sqlite3 locale. Dès que la base mysql est disponible, les requêtes sont envoyées au serveur.
 * \param     md              descripteur du gestionnaire. Il est alloué par l'appelant.
 * \param     db_server       nom ou adresse IP du serveur qui héberge la base
 * \param     db_server_port  port (valeur numérique) du service mysql sur le serveur
 * \param     base            nom de la base de données
 * \param     user            utilisateur de la base (doit avoir les droits suffisant pour lire et modifier le contenu des tables
 * \param     passwd          mot de passe de l'utilisateur
 * \param     sqlite3_db_path chemin vers la base sqlite de secours
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   if(!db_server || !base || !user || !passwd || !sqlite3_db_path || !db_server_port)
      return -1;

   md->db_server=string_malloc_and_copy(db_server,1);
   IF_NULL_RETURN(md->db_server,-1);
   
   md->db_server_port=string_malloc_and_copy(db_server_port,1);
   IF_NULL_RETURN(md->db_server,-1);
   
   md->base=string_malloc_and_copy(base,1);
   IF_NULL_RETURN(md->db_server,-1);

   md->user=string_malloc_and_copy(user,1);
   IF_NULL_RETURN(md->user,-1);
   
   md->passwd=string_malloc_and_copy(passwd,1);
   IF_NULL_RETURN(md->passwd,-1);
   
   md->sqlite3_db_path=string_malloc_and_copy(sqlite3_db_path,1);
   IF_NULL_RETURN(md->sqlite3_db_path,-1);
   
   
   md->queue=(queue_t *)malloc(sizeof(queue_t));
   if(!md->queue)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't create queue.\n",ERROR_STR,__func__);
      return -1;
   }
   init_queue(md->queue); // initialisation de la file
   
   pthread_mutex_init(&md->lock, NULL);
   
   if(pthread_create (&(md->thread), NULL, tomysqldb_thread, (void *)md))
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't create thread.\n", ERROR_STR, __func__);
      return -1;
   }
   
   md->started=1;
   
   return 0;
}


void tomysqldb_release(tomysqldb_md_t *md)
/**
 * \brief     arrête et libère le descripteur du module de gestion de base de données mysql
 * \details   fonction à appeler pour arrêter le thread et libérer la mémoire
 * \param     md   descripteur tomysqldb.
 */
{
   if(md)
   {
      pthread_cancel(md->thread);
      pthread_join(md->thread,NULL);

      md->started=0;

      mysql_close(md->conn);
      md->conn=NULL;
      sqlite3_close(md->db);
      md->db=NULL;

      clear_queue(md->queue,_tomysqldb_free_queue_elem);
      
      if(md->queue)
      {
         free(md->queue);
         md->queue=NULL;
      }
      
      if(md->db_server)
      {
         free(md->db_server);
         md->db_server=NULL;
      }
      
      if(md->db_server_port)
      {
         free(md->db_server_port);
         md->db_server_port=NULL;
      }
      
      if(md->base)
      {
         free(md->base);
         md->base=NULL;
      }
      
      if(md->user)
      {
         free(md->user);
         md->user=NULL;
      }
      
      if(md->passwd)
      {
         free(md->passwd);
         md->passwd=NULL;
      }
      
      if(md->sqlite3_db_path)
      {
         free(md->sqlite3_db_path);
         md->sqlite3_db_path=NULL;
      }
   }
}


//void stop_dbServer(tomysqldb_md_t *md)
int stop_dbServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   tomysqldb_release(_md);
   free(_md);
   _md=NULL;

   _dbServer_monitoring_id=-1;
   
   mea_notify_printf('S',"XPLSERVER stopped successfully");

   return 0;
}


tomysqldb_md_t *dbServer_get_md()
{
   return _md;
}


int start_dbServer(int my_id, void *data, char *errmsg, int l_errmsg)
// tomysqldb_md_t *start_dbServer(char **params_list, sqlite3 *sqlite3_param_db)
{
#ifndef __NO_TOMYSQL__
   struct dbServerData_s *dbServerData = (struct dbServerData_s *)data;
   int16_t ret;
   char err_str[80], notify_str[80];

   _md=(struct tomysqldb_md_s *)malloc(sizeof(struct tomysqldb_md_s));
   if(!_md)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      mea_notify_printf('E',"XPLSERVER can't be launched - %s.", err_str);
      return -1;
   }
   memset(_md,0,sizeof(struct tomysqldb_md_s));
   
   ret=tomysqldb_init(_md, dbServerData->params_list[MYSQL_DB_SERVER],
                           dbServerData->params_list[MYSQL_DB_PORT],
                           dbServerData->params_list[MYSQL_DATABASE],
                           dbServerData->params_list[MYSQL_USER],
                           dbServerData->params_list[MYSQL_PASSWD],
                           dbServerData->params_list[SQLITE3_DB_BUFF_PATH]);
   if(ret==-1)
   {
      VERBOSE(2) fprintf(stderr,"%s (%s) : Can not init data base communication.\n", ERROR_STR, __func__);
      snprintf(notify_str, strlen(notify_str), "Can't start DBSERVER - Can not init data base communication.\n");
      mea_notify2(notify_str, 'E');
      return -1;
   }
   _dbServer_monitoring_id=my_id;

   mea_notify_printf('S',"XPLSERVER Started");

#else
   VERBOSE(9) fprintf(stderr,"%s  (%s) : dbServer desactivated.\n", INFO_STR,__func__);
#endif
   return 0;
}

