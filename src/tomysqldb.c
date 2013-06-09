/*
 *  tomysqldb.c
 *
 *  Created by Patrice Dietsch on 13/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#include "tomysqldb.h"

#include "debug.h"
#include "macros.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <mysql.h>
#include <sqlite3.h>

#include <errmsg.h>

int tomysqldb_connect(tomysqldb_md_t *md, MYSQL **conn);


// #define SQLITEDB "/Users/patrice/db/domotique_dev.db"



void _tomysqldb_free_queue_elem(void *d) // pour vider_file2
{
   tomysqldb_queue_elem_t *e=(tomysqldb_queue_elem_t *)d;
   
   if(e)
   {
      FREE(e->data);      
      free(e);
      e=NULL;
   }
}


int sql_query_insert_tbl_ec(char *query, char *table, struct electricity_counters_query_s *ec_query)
{
   char s[255];
   
   strftime(s,sizeof(s)-1,"%y-%m-%d %H:%M:%S",localtime(&(ec_query->date_tv.tv_sec)));
   
   sprintf(query,"INSERT INTO %s (sensor_id,date,wh,kwh,flag) VALUES ( %d,\"%s\",%d,%d,%d )",
           table,
           ec_query->sensor_id,
           s,
           ec_query->wh_counter,
           ec_query->kwh_counter,
           ec_query->flag);
   
   return 0;
}


int sql_query_insert_tbl_pinst(char *query, char *table, struct pinst_query_s *p_inst)
{
   char s[255];
   
   // préparation de la requete
   strftime(s,sizeof(s)-1,"%y-%m-%d %H:%M:%S",localtime(&(p_inst->date_tv.tv_sec)));
   
   sprintf(query,"INSERT INTO %s (sensor_id,date,power,delta_t,flag) VALUES ( %d,\"%s\",%f,%f,%d )",
           table,
           p_inst->sensor_id,
           s,
           p_inst->power,
           p_inst->delta_t,
           p_inst->flag);
   
   return 0;
}


int sql_query_insert_tbl_temperature(char *query, char *table, struct requete_temperature *temperature)
{
   char s[255];
   struct tm t;
   
// trouver dans la table les info pour temperature->addr
   localtime_r(&(temperature->date_tv.tv_sec), &t);
   strftime(s,sizeof(s)-1,"%y-%m-%d %H:%M:%S",&t);
   
   sprintf(query,"INSERT INTO %s (date,temp,thermometre,flag) VALUES ( \"%s\",%f,%d,%d )",
           table,
           s,
           temperature->temperature,
           temperature->thermometre,
           temperature->flag);
   
   return 0;
}


int sqlite_to_mysql(sqlite3 *db, MYSQL *conn)
{
   char sql[255];
   sqlite3_stmt * stmt;
   int ret;
   
   sprintf(sql,"SELECT * FROM queries ORDER BY id");
   ret = sqlite3_prepare_v2(db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "ERROR (sqlite_to_mysql) : sqlite3_prepare_v2 - %s\n", sqlite3_errmsg (db));
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
            VERBOSE(1) fprintf (stderr, "ERROR (sqlite_to_mysql) : mysql_query - %u : %s\n", mysql_errno(conn), mysql_error(conn));
            return -1;
         }
         else
         {
            VERBOSE(9) fprintf(stderr,"INFO  (sqlite_to_mysql) : mysql_query = %s\n",query);
         }

         sprintf(sql,"DELETE FROM queries WHERE id=%s", id);
         ret = sqlite3_exec(db,sql,0,0,0);
         if(ret)
         {
            VERBOSE(1) fprintf (stderr, "ERROR (sqlite_to_mysql) : sqlite3_exec - %s\n", sqlite3_errmsg (db));
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
            VERBOSE(1) fprintf (stderr, "ERROR (sqlite_to_mysql) : sqlite3_step - %s\n", sqlite3_errmsg (db));
            sqlite3_finalize(stmt);
            return -1;
         }
      }
   }

   return 0;
}


int do_sql_query(MYSQL *conn, char *sql_query)
{
   int ret;
   
   ret=mysql_query(conn, sql_query);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "ERROR (do_sql_query) : mysql_query - %u : %s\n", mysql_errno(conn), mysql_error(conn));
      return -1;
   }
   return 0;
}


int save_sql_query(sqlite3 *db, char *sql_query)
{
   char sql[255];
   int ret;
      
   sprintf(sql,"INSERT INTO queries (request) VALUES ( '%s' )", sql_query);
   
   ret = sqlite3_exec(db,sql,0,0,0);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "ERROR (save_sql_query) : sqlite3_exec - %s\n", sqlite3_errmsg (db));
      return -1;
   }
   return 0;
}


int to_db(tomysqldb_md_t *md, int mysql_connected, MYSQL *conn, sqlite3 *db, tomysqldb_queue_elem_t *elem)
{
   char sql[255];
   
   VERBOSE(9) fprintf(stderr,"INFO  (to_db) : Insertion data type %d\n",elem->type);
   
   switch(elem->type)
   {
      case TOMYSQLDB_TYPE_PINST:
      {
         struct pinst_query_s *pinst_query;
         
         pinst_query=(struct pinst_query_s *)(elem->data);
         sql_query_insert_tbl_pinst(sql, "pinst", pinst_query);
         
         break;
      }
         
      case TOMYSQLDB_TYPE_EC:
      {
         struct electricity_counters_query_s *ec_query;
         
         ec_query=(struct electricity_counters_query_s *)(elem->data);
         sql_query_insert_tbl_ec(sql, "electricity_counters", ec_query);
         
         break;
      }

      case TOMYSQLDB_TYPE_TEMPERATURE:
      {
         struct requete_temperature *temperature;

         temperature=(struct requete_temperature *)(elem->data);
         sql_query_insert_tbl_temperature(sql, "temperatures", temperature);
         break;
      }
         
      default:
         return 0;
         break;
   }
   
   if(mysql_connected==1)
      do_sql_query(conn, sql);
   else
      save_sql_query(db, sql);

   return 0;
}


void *tomysqldb_thread(void *args)
{
   int ret;
   
   tomysqldb_queue_elem_t *elem;
   tomysqldb_md_t *md=(tomysqldb_md_t *)args;

   MYSQL *conn=NULL; // descripteur com. MYSQL
   int mysql_connected=0; // indicateur connexion active (1) ou inactive (0)
   unsigned long _mysql_thread_id,_mysql_thread_id_avant;
   
   sqlite3 *db=NULL; // descritpteur SQLITE
   int sqlite_opened=0;
   
   ret=tomysqldb_connect(md, &conn);
   if(ret)
      mysql_connected=0;
   else
      mysql_connected=1;

   
   while(1)
   {
      int nb=0;
      
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(md->lock));
      if(!pthread_mutex_lock(&(md->lock)))
      {
         nb=md->queue->nb_elem;
         pthread_mutex_unlock(&(md->lock));
      }
      pthread_cleanup_pop(0);
         
      if(nb) // s'il y a quelque chose à traiter
      {
         if(mysql_connected == 1)
         {
            // on s'assure d'abord que la connexion avec le serveur Mysql est encore possible
            _mysql_thread_id_avant=mysql_thread_id(conn); // on récupère d'abord l'id du thread, il sera utile pour savoir s'il y a eu une reconnexion.

            ret=mysql_ping(conn); // le ping pour éventuellement forcer une reconnexion
            if(ret) // pas de réponse au ping et reconnexion impossible.
            {
               VERBOSE(2) fprintf (stderr, "ERROR (tomysqldb_thread) : mysql_ping - %u: %s\n", mysql_errno(conn), mysql_error(conn));
               mysql_connected = 0; // plus de connexion au serveur mysql
            }
            else
            {
               _mysql_thread_id=mysql_thread_id(conn);      // id du thread après mysql ping
               if(_mysql_thread_id_avant!=_mysql_thread_id) // que l'on compare avec l'ancien id
               {
                  // si différent, une reconnexion à eu lieu :
                  // faire ce qu'il y a a faire en cas de reconnexion
                  // voir ici pour les info : http://dev.mysql.com/doc/refman/5.0/en/auto-reconnect.html
                  // pour l'instant on ne fait rien
                  VERBOSE(9) fprintf(stderr,"INFO  (tomysqldb_thread) : Une reconnexion à la base Mysql à eu lieu\n");
               }
            }
         }
         
         
         if(mysql_connected == 0) // pas de connexion, on essaye de se reconnecter
         {
            ret=tomysqldb_connect(md, &conn);
            if(ret)
               mysql_connected=0;
            else
               mysql_connected=1;
         }
         
         
         if(mysql_connected == 0) // toujours pas de connexion Mysql. On prépare la connexion à sqlite3
         {
            // DEBUG_SECTION fprintf(stderr,"SQLITE3_DB_PATH = %s\n",md->sqlite3_db_path);
            if(!sqlite_opened)
            {
               ret = sqlite3_open (md->sqlite3_db_path, &db);
               if(ret)
               {
                  sqlite_opened=0;
                  VERBOSE(2) fprintf (stderr, "ERROR (tomysqldb_thread) : sqlite3_open - %s\n", sqlite3_errmsg (db));
               }
               else
               {
                  sqlite_opened=1;
               }
            }
         }
         else // une connexion au serveur mysql est active
         {
            char sql_query[255];
            
            sprintf(sql_query,"USE %s",md->base); // et on utilise la base "base"
            ret=mysql_query(conn, sql_query);
            if(ret)
            {
               VERBOSE(2) fprintf (stderr, "ERROR (tomysqldb_thread) : sql_query - %u: %s\n", mysql_errno(conn), mysql_error(conn));
            }

            if(sqlite_opened==1) // sqlite est encore ouvert, il faut vider la table des requetes
            {
               // transferer le contenu de la base sqlite vers la base mysql ici :
               sqlite_to_mysql(db,conn);
               
               sqlite3_close(db);
               sqlite_opened=0;
            }
         }


         while(nb)
         {
            pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(md->lock));
            if(!pthread_mutex_lock(&(md->lock)))
            {
               last_queue(md->queue); // on se positionne en fin de queue
               current_queue(md->queue,(void **)&elem); // on lit le dernier element de la file sans le sortir
               
               ret=to_db(md, mysql_connected, conn, db, elem);
               if(!ret)
               {
                  out_queue_elem(md->queue,(void **)&elem);
                  nb=md->queue->nb_elem;
                  if(elem->data)
                  {
                     free(elem->data);
                     elem->data=NULL;
                  }
                  if(elem)
                  {
                     free(elem); // /!\ libération a revoir ... il faut aussi liberer à l'intérieur ...
                     elem=NULL;
                  }
               }
               else
               {
                  // pas inséré dans la base ...
               }
               pthread_mutex_unlock(&(md->lock));
            }
            pthread_cleanup_pop(0);
         }
      }
      
      pthread_testcancel();
      sleep(10);
   }
   
   mysql_close(conn);
   sqlite3_close(db);

   pthread_exit(NULL);
}


int tomysqldb_connect(tomysqldb_md_t *md, MYSQL **conn)
{
 my_bool reconnect = 0;
 int ret;
   
   if(*conn)
      mysql_close(*conn);

   // initialisation
   *conn = mysql_init(NULL);
   if (*conn == NULL)
   {
      VERBOSE(1) fprintf(stderr,"ERROR (tomysqldb_connect) : %u - %s\n", mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // connexion à mysql
   if (mysql_real_connect(*conn, md->db_server, md->user, md->passwd, md->base, 0, NULL, 0) == NULL)
   {
      VERBOSE(1) fprintf(stderr,"ERROR (tomysqldb_connect) : %u - %s\n", mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // on authorise les reconnexions automatiques pour que le ping puisse gérer la connexion
   ret=mysql_options(*conn, MYSQL_OPT_RECONNECT, &reconnect);
   if(ret)
   {
      VERBOSE(1) fprintf(stderr,"ERROR (tomysqldb_connect) : %u - %s\n", mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }

   return 0;
}


int tomysqldb_init(tomysqldb_md_t *md, char *db_server, char *base, char *user, char *passwd, char *sqlite3_db_path)
{
   if(!db_server || !base || !user || !passwd || !sqlite3_db_path)
      return -1;

   md->db_server=string_malloc_and_copy(db_server,1);
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
      VERBOSE(1) fprintf(stderr,"ERROR (tomysqldb_init) : can't create queue.\n");
      return -1;
   }
   init_queue(md->queue); // initialisation de la file
   
   pthread_mutex_init(&md->lock, NULL);
   
   if(pthread_create (&(md->thread), NULL, tomysqldb_thread, (void *)md))
   {
      VERBOSE(1) fprintf(stderr,"ERROR (tomysqldb_init) : can't create thread.\n");
      return -1;
   }
   
   return 0;
}


void tomysqldb_release(tomysqldb_md_t *md)
{
   pthread_cancel(md->thread);
   pthread_join(md->thread,NULL);
   
   clear_queue(md->queue,_tomysqldb_free_queue_elem);

   FREE(md->queue);
   FREE(md->db_server);
   FREE(md->base);
   FREE(md->user);
   FREE(md->passwd);
}
