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
#include <signal.h>
#include <mysql.h>
#include <sqlite3.h>

#include <errmsg.h>

#include "dbServer.h"

#include "globals.h"
#include "consts.h"
#include "debug.h"
#include "macros.h"
#include "memory.h"

#include "processManager.h"
#include "notify.h"


typedef struct dbServer_md_s
{
   pthread_t thread;
   pthread_mutex_t lock;
   int16_t started;
   
   queue_t *queue;

   char *db_server;
   char *db_server_port;
   char *base;
   char *user;
   char *passwd;
   
   char *sqlite3_db_path;

   int16_t opened;
   
   sqlite3 *db;
   MYSQL *conn;

} dbServer_md_t;


//int tomysqldb_connect(tomysqldb_md_t *md, MYSQL **conn);
int tomysqldb_connect(MYSQL **conn);

const char *db_server_name_str="DBSERVER";

// Variable globale privée
dbServer_md_t *_md=NULL;
int            _dbServer_monitoring_id=-1;
volatile sig_atomic_t
               _dbServer_thread_is_running=0;

long insqlite_indicator = 0;
long mysqlwrite_indicator = 0;

void set_dbServer_isnt_running(void *data)
{
   _dbServer_thread_is_running=0;
   mysql_thread_end();
}


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


int16_t dbServer_add_data_to_sensors_values(uint16_t sensor_id, double value1, uint16_t unit, double value2, char *complement)
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
   dbServer_queue_elem_t *elem;
   struct timeval tv;
   gettimeofday(&tv, NULL);

   if(!_md)
      return -1;

   if(!_md->opened) // md initialisé mais connexion avec Mysql jamais établie (peut-être un pb de paramétrage ???)
      return -1;    // on s'arrête pour ne pas remplir la mémoire
   
   if(!_md->started)
      return -1; // pas ou plus de thread consommateur.
      
   value=(struct sensor_value_s *)malloc(sizeof(struct sensor_value_s));
   if(!value)
   { 
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
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
   
   elem=malloc(sizeof(dbServer_queue_elem_t));
   if(!elem)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
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

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
   pthread_mutex_lock(&(_md->lock));

   VERBOSE(9) mea_log_printf("%s  (%s) : data to queue(%ld) (sensor_id=%d, value1=%f)\n",INFO_STR,__func__,_md->queue->nb_elem,sensor_id,value1);
   if(in_queue_elem(_md->queue,(void *)elem)==ERROR)
   {
      free_value(elem->data);
      free(elem);
      elem=NULL;
   }
   
   pthread_mutex_unlock(&(_md->lock));
   pthread_cleanup_pop(0);

   return 0;
}  


void _dbServer_free_queue_elem(void *d) // pour vider_file2()
/**
 * \brief     libère proprement le contenu d'un élément de file.
 * \details   fonction appelée par le gestionnaire de file.
 */
{
   dbServer_queue_elem_t *e=(dbServer_queue_elem_t *)d;
   
   if(e)
   {
      if(e->data && e->freedata)
         e->freedata(e->data);
      e->data=NULL;
      free(e);
      e=NULL;
   }
}


//int exec_mysql_query(MYSQL *conn, char *sql_query)
int exec_mysql_query(char *sql_query)
/**
 * \brief     Execute une commande sql dans Mysql.
 * \param     conn       descripteur de la base mysql
 * \param     sql_query  requête (texte SQL) à stocké dans la base sqlite3
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   int ret;
   
   ret=mysql_query(_md->conn, sql_query);
   if(ret)
   {
//      VERBOSE(1) fprintf (stderr, "%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -1;
   }
   return 0;
}


//int move_sqlite3_queries_to_mysql(sqlite3 *db, MYSQL *conn)
int move_sqlite3_queries_to_mysql()
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
   ret = sqlite3_prepare_v2(_md->db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (_md->db));
      return -1;
   }

   while (1)
   {
      int s;

      int hrtbt=process_heartbeat(_dbServer_monitoring_id);
      DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat (%d)\n", DEBUG_STR,__func__, hrtbt);
      pthread_testcancel();
      
      s = sqlite3_step (stmt);
      if (s == SQLITE_ROW)
      {
         const unsigned char *query, *id;
         
         id     = sqlite3_column_text (stmt, 0);
         query  = sqlite3_column_text (stmt, 1);
         
         ret=mysql_query(_md->conn, (const char *)query);
         if(ret)
         {
            VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
            return -1;
         }
         else
         {
            VERBOSE(9) mea_log_printf("%s  (%s) : mysql_query = %s\n",INFO_STR,__func__,query);
            mysqlwrite_indicator++;
         }

         sprintf(sql,"DELETE FROM queries WHERE id=%s", id);
         ret = sqlite3_exec(_md->db,sql,0,0,0);
         if(ret)
         {
            VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_exec - %s\n", ERROR_STR,__func__,sqlite3_errmsg (_md->db));
            return -1;
         }
         else
         {
            insqlite_indicator--;
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
            VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg(_md->db));
            sqlite3_finalize(stmt);
            return -1;
         }
      }
   }

   return 0;
}


int move_mysql_query_to_sqlite3(char *sql_query)
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
   
   if(!_md->db)
      return -1;
  
   int16_t n=snprintf(sql,sizeof(sql),"INSERT INTO queries (request) VALUES ( '%s' )", sql_query);
   if(n<0 || n==sizeof(sql))
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   ret = sqlite3_exec(_md->db,sql,0,0,0);
   if(ret)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_exec - %s\n", ERROR_STR,__func__,sqlite3_errmsg(_md->db));
      return -1;
   }
   insqlite_indicator++;

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
         mea_log_printf("%s (%s) : strftime - ", ERROR_STR,__func__);
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
         mea_log_printf("%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   else
      return 0;
}


//int write_data_to_db(int mysql_connected, MYSQL *conn, sqlite3 *db, dbServer_queue_elem_t *elem)
int write_data_to_db(dbServer_queue_elem_t *elem)
/**
 * \brief     écrit les données vers une table en fonction du type d'élément récupéré dans la file de traitement
 * \param     md              descripteur du gestionnaire. Il est alloué par l'appelant.
 * \param     mysql_connected flag msql connecté ou nom (pour choisir la destination)
 * \param     conn            le descripteur allouée par la librairie Mysql
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   char query[255];

   if(!_md) // pas de descripteur == pas de stockage dans la base ...
      return 0;
   
   VERBOSE(9) mea_log_printf("%s  (%s) : Insertion data type %d\n",INFO_STR,__func__,elem->type);

   switch(elem->type)
   {
      case TOMYSQLDB_TYPE_SENSORS_VALUES:
         build_query_for_sensors_values(query, sizeof(query), elem->data);
         break;

      default:
         return 0;
         break;
   }
   
   if(_md->conn)
   {
      if(exec_mysql_query(query)==0)
         mysqlwrite_indicator++;
      else
         return -1;
   }
   else if(_md->db)
   {
      move_mysql_query_to_sqlite3(query);
   }
   else
      return -1;

   return 0;
}


int _connect(MYSQL **conn)
/**
 * \brief     Initialise la communication avec un serveur Mysql
 * \details   Cette fonction permet de réinitialiser une communication avec un serveur déjà atteint (si conn != NULL).
 * \param     md    descripteur du gestionnaire. Il est alloué par l'appelant.
 * \param     conn  le descripteur allouée par la librairie Mysql
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
      VERBOSE(1) mea_log_printf("%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // récupération du port si disponible
   char *end;
   uint16_t port=3306;
   if(_md->db_server_port[0])
   {
      port=strtol(_md->db_server_port,&end,10);
      if(*end!=0 || errno==ERANGE)
      {
         VERBOSE(1) mea_log_printf("%s (%s) : port (%s) incorrect\n", ERROR_STR,__func__,_md->db_server_port);
         port=0;
      }
   }
      
   // connexion à mysql
   if (mysql_real_connect(*conn, _md->db_server, _md->user, _md->passwd, _md->base, port, NULL, 0) == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // on authorise les reconnexions automatiques pour que le ping puisse gérer la connexion
   ret=mysql_options(*conn, MYSQL_OPT_RECONNECT, &reconnect);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      mysql_close(*conn);
      *conn=NULL;
      return -1;
   }

   return 0;
}


int flush_data()
{
   int nb=-1;
   int ret=-1;
   int return_code=0;
   dbServer_queue_elem_t *elem = NULL;
   
   if(_md->conn)
   {
      char sql_query[80];
            
      snprintf(sql_query, sizeof(sql_query), "USE %s", _md->base); // et on utilise la bonne base
      ret=exec_mysql_query(sql_query);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sql_query - %u: %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      }

      if(_md->db) // sqlite est encore ouvert, il faut vider la table des requetes en attente si elle n'est pas vide
      {
         // transferer le contenu de la base sqlite vers la base mysql ici :
         move_sqlite3_queries_to_mysql(_md->db, _md->conn);
               
         sqlite3_close(_md->db); // et on ferme la base sqlite
         _md->db=NULL;
      }
   }
   
   do
   {
      int hrtbt=process_heartbeat(_dbServer_monitoring_id);
      DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat (%d)\n", DEBUG_STR,__func__, hrtbt);
      pthread_testcancel();
      
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
      pthread_mutex_lock(&(_md->lock));

      nb=_md->queue->nb_elem;
      if(nb>0)
      {
         last_queue(_md->queue); // on se positionne en fin de queue
         current_queue(_md->queue,(void **)&elem); // on lit le dernier element de la file sans le sortir (on le sortira s'il est inséré dans une base

         ret=write_data_to_db(elem);
         if(!ret)
         {
            out_queue_elem(_md->queue,(void **)&elem);
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
         else // on ne peut écrire dans aucune base ...
         {
            return_code=-1;
            nb=-1; // on force la sortie de la boucle, on verra plus tard.
         }
      }
   
      pthread_mutex_unlock(&(_md->lock));
      pthread_cleanup_pop(0);
   }
   while(nb>0);
   
   return return_code;
}


int select_database()
{
   int ret;
   
   unsigned long _mysql_thread_id = -1,_mysql_thread_id_avant = -1;
   
   if(_md->conn) // on a déjà été connecté un jour ...
   {
      _mysql_thread_id_avant=mysql_thread_id(_md->conn); // on récupère d'abord l'id du thread, il sera utile pour savoir s'il y a eu une reconnexion.

      // on s'assure d'abord que la connexion avec le serveur Mysql est encore possible
      ret=mysql_ping(_md->conn); // le ping pour éventuellement forcer une reconnexion
      if(ret) // pas de réponse au ping et donc reconnexion impossible.
      {
         VERBOSE(5) mea_log_printf("%s  (%s) : mysql_ping - %u: %s\n",INFO_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
         mysql_close(_md->conn);
         _md->conn=NULL;
      }
      else
      {
         _mysql_thread_id=mysql_thread_id(_md->conn);      // id du thread après mysql ping
         if(_mysql_thread_id_avant!=_mysql_thread_id) // que l'on compare avec l'ancien id
         {
            // si différent, une reconnexion à eu lieu :
            // faire ce qu'il y a a faire en cas de reconnexion
            // voir ici pour les info : http://dev.mysql.com/doc/refman/5.0/en/auto-reconnect.html
            // pour l'instant on ne fait rien
            VERBOSE(9) mea_log_printf("%s  (%s) : Une reconnexion à la base Mysql à eu lieu\n", INFO_STR,__func__);
         }
      }
   }      
         
   if(_md->conn == NULL) // jamais connecté ou plus de connexion, on essaye de se connecter
   {
      ret=_connect(&_md->conn);
      if(ret)
      {
      }
      else
      {
         _md->opened=1;
      }
   }
         
   if(_md->conn==NULL) // toujours pas de connexion Mysql. Repli sur sqlite3, ouverture si nécessaire
   {
      if(!_md->db) // sqlite n'est pas ouverte
      {
         ret = sqlite3_open (_md->sqlite3_db_path, &_md->db);
         if(ret)
         {
            _md->db=NULL;
            VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (_md->db));
            return -1; // aucune connexion disponible
         }
      }
   }
//   return *mysql_connected;
   return 0;
}


void *dbServer_thread(void *args)
/**
 * \brief     Thread de tomysql
 * \details   Les insersions de données dans la base Mysql sont traitées de façon asynchrone. Les "clients" postent les demandes de stockage de données dans une file qui est parcourues régulièrement et vidée dans la base.
 *            Le thread gère la disponibilité des bases Mysql et sqlite3. Si aucune des bases n'est disponible les données sont gardées en mémoire jusqu'à épuisement de la mémoire.
 * \param     args  pointeur (anonyme) sur une structure md (le descripteur)  
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   int ret;
   int nb=-1;
   
//   dbServer_queue_elem_t *elem = NULL;

//   int mysql_connected=0; // indicateur connexion active (1) ou inactive (0)
//   unsigned long _mysql_thread_id = -1,_mysql_thread_id_avant = -1;
   
   time_t last_time = time(NULL);
   
   _md->db=NULL; // descritpteur SQLITE
   _md->conn=NULL; // descripteur com. MYSQL
   _md->opened=0;
   
   int hrtbt=process_heartbeat(_dbServer_monitoring_id);
   DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat (%d)\n", DEBUG_STR,__func__, hrtbt);
   pthread_testcancel();

   pthread_cleanup_push( (void *)set_dbServer_isnt_running, (void *)NULL );
   _dbServer_thread_is_running=1;

   // ouverture de la base
   ret=select_database();
   // ouverture de la base sqlite3
   // cela va permettre de vider les transactions en stock au démarrage
   ret = sqlite3_open (_md->sqlite3_db_path, &_md->db);
   if(ret)
   {
      _md->db=NULL;
      VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (_md->db));
   }
   if(_md->conn) // on est normalement connecté à la base (on flush pour vider sqlite3 dans mysql si nécessaire
      flush_data();
   
   while(1)
   {
      time_t now = time(NULL);
 
      DEBUG_SECTION {
        mea_log_printf("%s (%s) : last loop %d (watchdog debug)\n", DEBUG_STR,__func__,now-last_time);
        last_time=now;
      }

      int hrtbt=process_heartbeat(_dbServer_monitoring_id);
      DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat (%d)\n", DEBUG_STR,__func__, hrtbt);
      pthread_testcancel();

      process_update_indicator(_dbServer_monitoring_id, "DBSERVERINSQLITE", insqlite_indicator);
      process_update_indicator(_dbServer_monitoring_id, "DBSERVERMYWRITE", mysqlwrite_indicator);

      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
      pthread_mutex_lock(&(_md->lock));
      if(_md->queue)
      {
         process_update_indicator(_dbServer_monitoring_id, "DBSERVERINMEM", _md->queue->nb);
         nb=_md->queue->nb_elem;
      }
      else
         nb=-1;
      pthread_mutex_unlock(&(_md->lock));
      pthread_cleanup_pop(0);

      if(nb>0) // s'il y a quelque chose à traiter
      {
         ret=select_database();
         if(ret!=-1)
         {
            flush_data();
         }
      }
      
      int hrtbt=process_heartbeat(_dbServer_monitoring_id);
      DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat (%d)\n", DEBUG_STR,__func__, hrtbt);
      pthread_testcancel();
      sleep(10);
   }
   
   pthread_cleanup_pop(1);
   return NULL;
}


int dbServer(char *db_server, char *db_server_port, char *base, char *user, char *passwd, char *sqlite3_db_path)
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

   _md->db_server=string_malloc_and_copy(db_server,1);
   IF_NULL_RETURN(_md->db_server,-1);
   
   _md->db_server_port=string_malloc_and_copy(db_server_port,1);
   IF_NULL_RETURN(_md->db_server,-1);
   
   _md->base=string_malloc_and_copy(base,1);
   IF_NULL_RETURN(_md->db_server,-1);

   _md->user=string_malloc_and_copy(user,1);
   IF_NULL_RETURN(_md->user,-1);
   
   _md->passwd=string_malloc_and_copy(passwd,1);
   IF_NULL_RETURN(_md->passwd,-1);
   
   _md->sqlite3_db_path=string_malloc_and_copy(sqlite3_db_path,1);
   IF_NULL_RETURN(_md->sqlite3_db_path,-1);
   
   
   _md->queue=(queue_t *)malloc(sizeof(queue_t));
   if(!_md->queue)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't create queue.\n",ERROR_STR,__func__);
      return -1;
   }
   init_queue(_md->queue); // initialisation de la file
   
   pthread_mutex_init(&_md->lock, NULL);
   
   if(pthread_create (&(_md->thread), NULL, dbServer_thread, (void *)_md))
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't create thread.\n", ERROR_STR, __func__);
      return -1;
   }
   pthread_detach(_md->thread);
   _md->started=1;
   
   return 0;
}


//void stop_dbServer(tomysqldb_md_t *md)
int stop_dbServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret;
   
   if(_md)
   {
      pthread_cancel(_md->thread);
      int counter=500; // 5 secondes environ
      int stopped=-1;
      while(counter--)
      {
         if(_dbServer_thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(10000); // will sleep for 10 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, fin après %d itération\n",DEBUG_STR, __func__,500-counter);

      _md->started=0;

      ret=select_database();
      if(ret!=-1)
         if(flush_data()<0)
         {
            // signaler risque de perte de données
         }

      if(_md->conn)
      {
         mysql_close(_md->conn);
         _md->conn=NULL;
      }
      
      if(_md->db)
      {
         sqlite3_close(_md->db);
         _md->db=NULL;
      }

      clear_queue(_md->queue,_dbServer_free_queue_elem);
      if(_md->queue)
      {
         free(_md->queue);
         _md->queue=NULL;
      }
      
      pthread_mutex_destroy(&(_md->lock));
      
      if(_md->db_server)
      {
         free(_md->db_server);
         _md->db_server=NULL;
      }
      
      if(_md->db_server_port)
      {
         free(_md->db_server_port);
         _md->db_server_port=NULL;
      }
      
      if(_md->base)
      {
         free(_md->base);
         _md->base=NULL;
      }
      
      if(_md->user)
      {
         free(_md->user);
         _md->user=NULL;
      }
      
      if(_md->passwd)
      {
         free(_md->passwd);
         _md->passwd=NULL;
      }
      
      if(_md->sqlite3_db_path)
      {
         free(_md->sqlite3_db_path);
         _md->sqlite3_db_path=NULL;
      }
      
      free(_md);
      _md=NULL;
   }

   _dbServer_monitoring_id=-1;
   
   VERBOSE(1) mea_log_printf("%s  (%s) : DBSERVER %s.\n", INFO_STR, __func__, stopped_successfully_str);
   mea_notify_printf('S',"DBSERVER %s.", stopped_successfully_str);

   return 0;
}


int start_dbServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
#ifndef __NO_TOMYSQL__
   struct dbServerData_s *dbServerData = (struct dbServerData_s *)data;
   int16_t ret;
   char err_str[256];

   _md=(struct dbServer_md_s *)malloc(sizeof(struct dbServer_md_s));
   if(!_md)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      mea_notify_printf('E',"DBSERVER can't be launched - %s.", err_str);
      return -1;
   }
   memset(_md,0,sizeof(struct dbServer_md_s));

   ret=dbServer(dbServerData->params_list[MYSQL_DB_SERVER],
                dbServerData->params_list[MYSQL_DB_PORT],
                dbServerData->params_list[MYSQL_DATABASE],
                dbServerData->params_list[MYSQL_USER],
                dbServerData->params_list[MYSQL_PASSWD],
                dbServerData->params_list[SQLITE3_DB_BUFF_PATH]);
   if(ret==-1)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : Can not init data base communication.\n", ERROR_STR, __func__);
      mea_notify_printf('E', "DBSERVER : can't init data base communication.");
      return -1;
   }
   _dbServer_monitoring_id=my_id;

   VERBOSE(1) mea_log_printf("%s  (%s) : DBSERVER %s.\n", INFO_STR, __func__, launched_successfully_str);
   mea_notify_printf('S', "DBSERVER %s.", launched_successfully_str);

#else
   VERBOSE(9) mea_log_printf("%s  (%s) : dbServer desactivated.\n", INFO_STR,__func__);
#endif
   return 0;
}

int restart_dbServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_dbServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_dbServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}

