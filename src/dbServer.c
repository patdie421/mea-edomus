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
#include "macros.h"
#include "mea_queue.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "processManager.h"
#include "notify.h"

static int first_start=1;

typedef struct dbServer_md_s
{
   pthread_t thread;
   pthread_mutex_t lock;
   int16_t started;
//   int16_t opened;
   
   mea_queue_t *queue;

   char *db_server;
   char *db_server_port;
   char *base;
   char *user;
   char *passwd;
   
   char *sqlite3_db_path;

   unsigned long collector_id;

   sqlite3 *db;
   MYSQL *conn;

} dbServer_md_t;


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
   DEBUG_SECTION mea_log_printf("%s (%s) : Before, _dbServer_thread_is_running = %d\n", DEBUG_STR,__func__,_dbServer_thread_is_running);
   if(_md && _md->conn)
   {
      mysql_close(_md->conn);
      _md->conn=NULL;
   }
//   mysql_thread_end();
   _dbServer_thread_is_running=0;

   DEBUG_SECTION mea_log_printf("%s (%s) : after, _dbServer_thread_is_running = %d now\n", DEBUG_STR,__func__,_dbServer_thread_is_running);
}


void free_sensors_value(void *data)
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


void free_batch_data(void *data)
/**
 * \brief     libère les données allouées (malloc) pour un variable de structure sensors_values_s.
 * \param     data   pointeur anonyme sur une structure sensors_value_s
 */
{
}


int16_t start_batch(int batch_id)
{
   dbServer_queue_elem_t *elem;

   if(!_md || !_md->conn || !_md->started || !_dbServer_thread_is_running)
      return -1;

   elem=(dbServer_queue_elem_t *)malloc(sizeof(dbServer_queue_elem_t));

   if(!elem)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return -1;
   }

   elem->type=batch_id;
   elem->data=NULL;
   elem->freedata=free_batch_data;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
   pthread_mutex_lock(&(_md->lock));

   VERBOSE(9) mea_log_printf("%s (%s) : start batch %d", INFO_STR, __func__, batch_id);
   if(mea_queue_in_elem(_md->queue,(void *)elem)==ERROR)
   {
      free(elem);
      elem=NULL;
   }

   pthread_mutex_unlock(&(_md->lock));
   pthread_cleanup_pop(0);

   return 0;
}


int16_t start_resync_batch()
{
   return start_batch(START_BATCH_RESYNC);
}


int16_t start_consolidation_batch()
{
   return start_batch(START_BATCH_CONSO);
}


int16_t start_purge_batch()
{
   return start_batch(START_BATCH_PURGE);
}


int16_t dbServer_add_data_to_sensors_values(uint16_t sensor_id, double value1, uint16_t unit, double value2, char *complement)
/**
 * \brief     Récupère les données de type "sensors values" pour stockage dans la table sensors_values de la base mysql.
 * \details   En dehors des données en provenance des capteurs la date courrante est rajoutée par cette fonction.
 * \param     sensor_id  identifiant du capteur
 * \param     value1     valeur principale
 * \param     unit       identifiant de l'unité de mesure
 * \param     value2     une valeur complémentaire optionnelle
 * \param     complement un complément de données au format text optionnel
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   struct sensor_value_s *value;
   dbServer_queue_elem_t *elem;
   struct timeval tv;
   gettimeofday(&tv, NULL);

   if(!_md)
      return -1;

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
   
   (void)(value->sensor_id=sensor_id),
   memcpy(&(value->date_tv),&tv,sizeof(struct timeval));
   value->value1=value1; // valeur principale
   value->unit=unit; // code unité de mesure (s'applique à la valeur principale)
   value->value2=value2; // valeur secondaire
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
   elem->freedata=free_sensors_value;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
   pthread_mutex_lock(&(_md->lock));

   VERBOSE(9) mea_log_printf("%s (%s) : data to queue(%ld) (sensor_id=%d, value1=%f)\n", INFO_STR, __func__, _md->queue->nb_elem, sensor_id, value1);
   if(mea_queue_in_elem(_md->queue,(void *)elem)==ERROR)
   {
      free_sensors_value(elem->data);
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
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -1;
   }
   return 0;
}


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
   ret = sqlite3_prepare_v2(_md->db,sql,(int)(strlen(sql)+1),&stmt,NULL);
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
            VERBOSE(9) mea_log_printf("%s (%s) : mysql_query = %s\n", INFO_STR, __func__, query);
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


uint16_t get_location_name_description_from_param_db(int id, char *name, char *description)
{
   char sql[255]="";
   sqlite3_stmt *stmt = NULL;
   sqlite3 *sqlite3_param_db=get_sqlite3_param_db();
   int ret=-1;

   if(sqlite3_param_db)
   {
      sprintf(sql, "SELECT name, description FROM locations WHERE id_location = %d AND deleted_flag <> 1;", id);
      ret = sqlite3_prepare_v2(sqlite3_param_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (sqlite3_param_db));
         return -1;
      }

      ret=-1;
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         strcpy(name, (char *)sqlite3_column_text(stmt, 0));
         strcpy(description, (char *)sqlite3_column_text(stmt, 1));

         ret=0;
      }
      sqlite3_finalize(stmt);
   }

   return ret;
}


uint16_t get_sensor_name_description_from_param_db(int id, char *name, char *description)
{
   char sql[255]="";
   sqlite3_stmt *stmt = NULL;
   sqlite3 *sqlite3_param_db=get_sqlite3_param_db();
   int ret=-1;   

   if(sqlite3_param_db)
   {
      sprintf(sql, "SELECT name, description FROM sensors_actuators WHERE id_sensor_actuator = %d AND deleted_flag <> 1;", id);
      ret = sqlite3_prepare_v2(sqlite3_param_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (sqlite3_param_db));
         return -1;
      }

      ret=-1;
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         strcpy(name, (char *)sqlite3_column_text(stmt, 0));
         strcpy(description, (char *)sqlite3_column_text(stmt, 1));

         ret=0;
      }
      sqlite3_finalize(stmt);
   }

   return ret;
}


int16_t get_sensor_location_from_param_db(int id)
{
   char sql[255]="";
   sqlite3_stmt *stmt = NULL;
   sqlite3 *sqlite3_param_db=get_sqlite3_param_db();
   int ret=-1;

   if(sqlite3_param_db)
   {
      sprintf(sql, "SELECT id_location FROM sensors_actuators WHERE id_sensor_actuator = %d AND deleted_flag <> 1;", id);
      ret = sqlite3_prepare_v2(sqlite3_param_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (sqlite3_param_db));
         return ERROR;
      }

      ret=-1;
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         ret = sqlite3_column_int(stmt, 0);
      }
      sqlite3_finalize(stmt);
   }

   return ret;
}


uint16_t resync_sensors_mysql_sqlite3_batch()
{
   int ret;
   char sql_query[255];

   if(!_md || !_md->conn)
      return -1;

   // sensors_names
   sprintf(sql_query, "SELECT id, sensor_id FROM sensors_names WHERE collector_id = %u", (unsigned int)_md->collector_id);

   ret=mysql_query(_md->conn, sql_query);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -1;
   }

   MYSQL_RES *result = mysql_store_result(_md->conn);
   if(result == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_store_result - %u : %s\n", ERROR_STR, __func__, mysql_errno(_md->conn), mysql_error(_md->conn));
      return -1;
   }

   MYSQL_ROW row;
   int nb_rows = (int)mysql_num_rows(result);
   unsigned int *sensors_ids = alloca(nb_rows * sizeof(unsigned int));
   if(sensors_ids == NULL)
   {
      mysql_free_result(result);
      return -1;
   }
 
   unsigned int *ids = alloca(nb_rows * sizeof(unsigned int));
   if(ids == NULL)
   {
      mysql_free_result(result);
      return -1;
   }

   int i=0; 
   while ((row = mysql_fetch_row(result))) 
   { 
      ids[i]=atoi(row[0]);
      sensors_ids[i]=atoi(row[1]);
      ++i;
   }

   mysql_free_result(result);

   char name[40]="", description[256]="";
   for(i=0; i<nb_rows;i++)
   {
      if(get_sensor_name_description_from_param_db(sensors_ids[i], name, description)<0)
         continue;

      sprintf(sql_query, "UPDATE sensors_names SET name='%s', description='%s' WHERE id = %u", name, description, ids[i]);
      ret=mysql_query(_md->conn, sql_query);
      if(ret)
      {
         VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      }
   }

   return 0;
}


uint16_t resync_locations_mysql_sqlite3_batch()
{
   int ret;
   char sql_query[255];

   if(!_md || !_md->conn)
      return -1;

   // sensors_names
   sprintf(sql_query, "SELECT id, location_id FROM locations WHERE collector_id = %u", (unsigned int)_md->collector_id);

   ret=mysql_query(_md->conn, sql_query);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -1;
   }

   MYSQL_RES *result = mysql_store_result(_md->conn);
   if(result == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_store_result - %u : %s\n", ERROR_STR, __func__, mysql_errno(_md->conn), mysql_error(_md->conn));
      return -1;
   }

   MYSQL_ROW row;
   int nb_rows = (int)mysql_num_rows(result);
   unsigned int *locations_ids = alloca(nb_rows * sizeof(unsigned int));
   if(locations_ids == NULL)
   {
      mysql_free_result(result);
      return -1;
   }
 
   unsigned int *ids = alloca(nb_rows * sizeof(unsigned int));
   if(ids == NULL)
   {
      mysql_free_result(result);
      return -1;
   }

   int i=0; 
   while ((row = mysql_fetch_row(result))) 
   { 
      ids[i]=atoi(row[0]);
      locations_ids[i]=atoi(row[1]);
      ++i;
   }

   mysql_free_result(result);

   char name[40]="", description[256]="";
   for(i=0; i<nb_rows;i++)
   {
      if(get_location_name_description_from_param_db(locations_ids[i], name, description)<0)
         continue;

      sprintf(sql_query, "UPDATE locations SET name='%s', description='%s' WHERE id = %u", name, description, ids[i]);
      ret=mysql_query(_md->conn, sql_query);
      if(ret)
      {
         VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      }
   }

   return 0;
}


int get_keyid_of_collector_in_mysql_db(unsigned int collector_id)
{
   int ret;
   char sql_query[255];
   int id=-1;

   if(!_md || !_md->conn)
      return -1;

   sprintf(sql_query, "SELECT id FROM collectors_names WHERE collector_id = %u", collector_id);
   ret=mysql_query(_md->conn, sql_query);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -2;
   }
   MYSQL_RES *result = mysql_store_result(_md->conn);
   if(result == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_store_result - %u : %s\n", ERROR_STR, __func__, mysql_errno(_md->conn), mysql_error(_md->conn));
      return -2;
   }
   int nb_rows = (int)mysql_num_rows(result);
   if(nb_rows == 1)
   {
      MYSQL_ROW row = mysql_fetch_row(result);
      if(row)
         id=atoi(row[0]);
   }
   mysql_free_result(result);

   return id;
}


int get_keyid_of_location_in_mysql_db(unsigned int location_id)
{
   int ret;
   char sql_query[255];
   int id=-1;

   if(!_md || !_md->conn)
      return -1;

   sprintf(sql_query, "SELECT id FROM locations WHERE location_id = %d and collector_id = %u", location_id, (unsigned int)_md->collector_id);
   ret=mysql_query(_md->conn, sql_query);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -2;
   }
   MYSQL_RES *result = mysql_store_result(_md->conn);
   if(result == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_store_result - %u : %s\n", ERROR_STR, __func__, mysql_errno(_md->conn), mysql_error(_md->conn));
      return -2;
   }
   int nb_rows = (int)mysql_num_rows(result);
   if(nb_rows == 1)
   {
      MYSQL_ROW row = mysql_fetch_row(result);
      if(row)
         id=atoi(row[0]);
   }
   mysql_free_result(result);

   return id;
}



uint16_t add_or_update_collector_in_mysql_db(int collector_id, char *name, char *description)
{
   int ret;
   char sql_query[255];
   int id=-1;

   if(!_md || !_md->conn)
      return -1;

   id=get_keyid_of_collector_in_mysql_db(collector_id);
   if(id==-1)
   {
      sprintf(sql_query, "INSERT INTO collectors_names (collector_id, name, description) values (%d, \"%s\", \"%s\")", collector_id, name, description);
      ret=mysql_query(_md->conn, sql_query);
      if(ret)
      {
          VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
          return -1;
      }
   }
   else if(id >= 0)
   {
      sprintf(sql_query, "UPDATE collectors_names SET name = '%s', description = '%s' WHERE id = %d", name, description, id);
      ret=mysql_query(_md->conn, sql_query);
      if(ret)
      {
          VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
          return -1;
      }
   }

   return 0;
}


uint16_t sync_mysql_sqlite3_locations_tables(unsigned int location_id)
{
   int ret;
   char sql_query[255];
//   int id=-1;

   if(!_md || !_md->conn)
      return -1;

   if(get_keyid_of_location_in_mysql_db(location_id)==-1)
   {
      char name[40], description[256];

      if(get_location_name_description_from_param_db(location_id, name, description) < 0)
      {
          VERBOSE(2) mea_log_printf("%s (%s) : can't get location name and description for %d\n", ERROR_STR,__func__, location_id);
         return -1;
      }
      else
      {
         sprintf(sql_query, "INSERT INTO locations (location_id, collector_id, name, description) values (%d, %u, \"%s\", \"%s\")", location_id, (unsigned int)_md->collector_id, name, description);
         ret=mysql_query(_md->conn, sql_query);
         if(ret)
         {
             VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
             return -1;
         }
      }
   }

   return 0;
}


int exist_sensorid_in_mysql_db(unsigned int sensor_id)
{
   int ret;
   char sql_query[255];
   int id=-1;

   if(!_md || !_md->conn)
      return -1;

   sprintf(sql_query, "SELECT count(sensor_id) FROM sensors_names WHERE sensor_id = %d and collector_id = %u", sensor_id, (unsigned int)_md->collector_id);
   ret=mysql_query(_md->conn, sql_query);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
      return -2;
   }
   MYSQL_RES *result = mysql_store_result(_md->conn);
   if(result == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql_store_result - %u : %s\n", ERROR_STR, __func__, mysql_errno(_md->conn), mysql_error(_md->conn));
      return -2;
   }
   int nb_rows = (int)mysql_num_rows(result);
   if(nb_rows == 1)
   {
      MYSQL_ROW row = mysql_fetch_row(result);
      if(row)
         id=atoi(row[0]);
   }
   mysql_free_result(result);

   return id;
}


uint16_t sync_mysql_sqlite3_sensors_tables(unsigned int sensor_id)
{
   int ret;
   char sql_query[255];
//   int exist=-1;

   if(!_md || !_md->conn)
      return -1;

   if(exist_sensorid_in_mysql_db(sensor_id)==0) // pas trouvé dans la base mysql, on le rajoute
   {
      char name[40], description[256];

      if(get_sensor_name_description_from_param_db(sensor_id, name, description) < 0)
      {
          VERBOSE(2) mea_log_printf("%s (%s) : can't get name and description for %d\n", ERROR_STR,__func__,sensor_id);
         return -1;
      }
      else
      {
         sprintf(sql_query, "INSERT INTO sensors_names (sensor_id, collector_id, name, description) values (%d, %u, \"%s\", \"%s\")", sensor_id, (unsigned int)_md->collector_id, name, description);
         ret=mysql_query(_md->conn, sql_query);
         if(ret)
         {
             VERBOSE(1) mea_log_printf("%s (%s) : mysql_query - %u : %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
             return -1;
         }
      }
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
         mea_log_printf("%s (%s) : strftime - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   } 
  
   int location_id = get_sensor_location_from_param_db(sensor_value->sensor_id);
   if(location_id < 0)
      location_id = 1; // location = unknown

   sync_mysql_sqlite3_sensors_tables(sensor_value->sensor_id);
   sync_mysql_sqlite3_locations_tables(location_id);
 
   n=snprintf(sql_query,
              l_sql_query,
              "INSERT INTO sensors_values (sensor_id, date, value1, unit, value2, complement, collector_id, location_id) VALUES ( %d,\"%s\",%f,%d,%f,\"%s\",%u, %d )",
              sensor_value->sensor_id,
              time_str,
              sensor_value->value1, // valeur principale
              sensor_value->unit, // code unité de mesure (s'applique à la valeur principale)
              sensor_value->value2, // valeur secondaire
              sensor_value->complement,
              (unsigned int)_md->collector_id,
              location_id

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


int consolidation_batch()
{
   int ret=-1;
   char date[40];
   time_t t;
   struct tm timeinfo;

   if(!_md || !_md->conn)
      return -1;

   VERBOSE(2) mea_log_printf("%s (%s) : starting batch\n", INFO_STR,__func__);

   t = time(NULL);

   t = t - 30 * 24 * 3600; // - 30 jours en seconde

   localtime_r(&t, &timeinfo);
   strftime(date, sizeof(date)-1, "%Y-%m-%d %H:00:00", &timeinfo);

   if(_md->conn)
   {
      char *request=
         "INSERT IGNORE INTO sensors_values_c (sensor_id, collector_id, location_id, date, nb_values, avg1, min1, max1, unit, avg2, min2, max2) "
         "SELECT "
            "sensor_id AS sensor_id," 
            "collector_id AS collector_id,"
            "location_id AS location_id,"
            "max(date) as date,"
//            "FROM_UNIXTIME((UNIX_TIMESTAMP(date) DIV 3600) * 3600 +3600 DIV 2) as date,"
            "count(sensor_id) AS nb_values,"
            "avg(value1) AS avg1,"
            "min(value1) AS min1,"
            "max(value1) AS max1,"
            "unit AS unit,"
            "avg(value2) AS avg2,"
            "min(value2) AS min2,"
            "max(value2) AS max2 "
         "FROM sensors_values "
         "WHERE date < '%s' AND (archive_flag <> 1 OR archive_flag is NULL) "
         "GROUP BY location_id, collector_id, sensor_id, UNIX_TIMESTAMP(date) DIV 3600 "
         "ORDER BY date;";
      char *sql = alloca(strlen(request)+25);
      if(!sql)
      {
         return -1;
      }

      ret=mysql_query(_md->conn, "START TRANSACTION;");
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : mysql_query - %u: %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
         return -1;
      }
      
      sprintf(sql, request, date);
      ret=mysql_query(_md->conn, sql);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : mysql_query - INSERT INTO (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
         ret=mysql_query(_md->conn, "ROLLBACK;");
         if(ret)
         {
            VERBOSE(2) mea_log_printf("%s (%s) : mysql_query - ROLLBACK (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
            return -1; 
         }
         return -1;
      }

      request = "UPDATE sensors_values SET archive_flag = 1 WHERE date < '%s'" ;
      sprintf(sql, request, date);

      ret=mysql_query(_md->conn, sql);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sql_query - UPDATE (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
         ret=mysql_query(_md->conn, "ROLLBACK;");
         if(ret)
         {
            VERBOSE(2) mea_log_printf("%s (%s) : sql_query - ROLLBACK (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
            return -1;
         }
         return -1;
      }

      ret=mysql_query(_md->conn, "COMMIT;");
      if(ret)
      {
          VERBOSE(2) mea_log_printf("%s (%s) : sql_query - COMMIT (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
          return -1;
      }

      VERBOSE(2) mea_log_printf("%s (%s) : batch done\n", INFO_STR,__func__);
      return 0;
   }

   VERBOSE(2) mea_log_printf("%s (%s) : batch error\n", ERROR_STR,__func__);
   return -1;
}


int16_t purge_batch()
{
   int ret=-1;
   char date[40];
   time_t t;
   struct tm timeinfo;

   VERBOSE(2) mea_log_printf("%s (%s) : starting batch\n", INFO_STR,__func__);

   t = time(NULL);

   t = t - 35 * 24 * 3600; // - 35 jours en seconde

   localtime_r(&t, &timeinfo);
   strftime(date, sizeof(date)-1, "%Y-%m-%d %H:00:00", &timeinfo);

   if(_md->conn)
   {
      char *request=
         "DELETE FROM sensors_values "
         "WHERE date < '%s' AND archive_flag = 1 ";
      char *sql = alloca(strlen(request)+25);
      if(!sql)
         return -1;

      ret=mysql_query(_md->conn, "START TRANSACTION;");
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : mysql_query - %u: %s\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
         return -1;
      }

      sprintf(sql, request, date);
      ret=mysql_query(_md->conn, sql);
      if(ret)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : mysql_query - DELETE (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
         ret=mysql_query(_md->conn, "ROLLBACK;");
         if(ret)
         {
            VERBOSE(2) mea_log_printf("%s (%s) : mysql_query - ROLLBACK (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
            return -1;
         }
         return -1;
      }

      ret=mysql_query(_md->conn, "COMMIT;");
      if(ret)
      {
          VERBOSE(2) mea_log_printf("%s (%s) : sql_query - COMMIT (%u: %s)\n", ERROR_STR,__func__,mysql_errno(_md->conn), mysql_error(_md->conn));
          return -1;
      }

      VERBOSE(2) mea_log_printf("%s (%s) : batch done\n", INFO_STR,__func__);
      return 0;
   }

   VERBOSE(2) mea_log_printf("%s (%s) : batch error\n", ERROR_STR,__func__);
   return -1;
}


int do_request(dbServer_queue_elem_t *elem)
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
   
   VERBOSE(9) mea_log_printf("%s (%s) : insertion data type %d\n", INFO_STR, __func__, elem->type);

   switch(elem->type)
   {
      case TOMYSQLDB_TYPE_SENSORS_VALUES:
         build_query_for_sensors_values(query, sizeof(query), elem->data);
         break;

      case START_BATCH_CONSO:
         return consolidation_batch();
         
      case START_BATCH_PURGE:
         return purge_batch();

      case START_BATCH_RESYNC:
         resync_sensors_mysql_sqlite3_batch();
         resync_locations_mysql_sqlite3_batch();
         return 0;

      default:
         return 0;
         break;
   }
   
   if(_md->conn)
   {

      int ret = exec_mysql_query(query);
      if(ret==0)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : insert in mysql db\n", INFO_STR, __func__);
         mysqlwrite_indicator++;
      }
      else
      {
         return -1;
      }
   }
   else if(_md->db)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : insertion in sqlite db\n", INFO_STR, __func__);
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
   my_bool reconnect = 1;
   unsigned int timeout = 5;

// int ret;
   
   if(*conn)
   {
      mysql_close(*conn);
//      mysql_thread_end();
   }
   
   // initialisation
   *conn = mysql_init(NULL);
   if (*conn == NULL)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : %u - %s\n", ERROR_STR,__func__,mysql_errno(*conn), mysql_error(*conn));
      return -1;
   }
   
   // on authorise les reconnexions automatiques pour que le ping puisse gérer la connexion
   mysql_options(*conn, MYSQL_OPT_RECONNECT, &reconnect);
   mysql_options(*conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
   
   unsigned long my_version = mysql_get_client_version();
   if(my_version < 50112)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : mysql read/write timeout not available for client version (%l)\n", WARNING_STR,__func__,my_version);
   }
   else
   {
      mysql_options(*conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
      mysql_options(*conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
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
      mysql_close(*conn);
//      mysql_thread_end();
      *conn=NULL;
      return -1;
   }
   else
      mysql_options(*conn, MYSQL_OPT_RECONNECT, &reconnect);

   return 0;
}


int flush_data(int heartbeat_flag)
{
   int nb=-1;
   int ret=-1;
   int return_code=0;
   dbServer_queue_elem_t *elem = NULL;
   
   if(_md->conn == NULL && _md->db == NULL)
      return -1;
   
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
//         move_sqlite3_queries_to_mysql(_md->db, _md->conn);
         move_sqlite3_queries_to_mysql();
         
         sqlite3_close(_md->db); // et on ferme la base sqlite
         _md->db=NULL;
      }
   }
   
   do
   {
      if(heartbeat_flag)
      {
         process_heartbeat(_dbServer_monitoring_id);
//         int hrtbt=process_heartbeat(_dbServer_monitoring_id);
//         DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat. Previous : %d seconds ago.\n", DEBUG_STR,__func__, hrtbt);
      }
      pthread_testcancel();
      
      // lecture du dernier element de la file si dispo
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
      pthread_mutex_lock(&(_md->lock));

      nb=(int)_md->queue->nb_elem;
      if(nb>0)
      {
         mea_queue_out_elem(_md->queue,(void **)&elem);
      }
      
      pthread_mutex_unlock(&(_md->lock));
      pthread_cleanup_pop(0);
   
      // si dispo on essaye d'écrire la donnée dans la base
      if(nb>0)
      {
         ret=do_request(elem);
         if(!ret)
         {
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
         else // pas écriture impossible on remet la donnée dans la file
         {
            pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
            pthread_mutex_lock(&(_md->lock));

            mea_queue_in_elem(_md->queue,(void *)elem);

            pthread_mutex_unlock(&(_md->lock));
            pthread_cleanup_pop(0);
         
            return_code=-1;
            nb=-1; // on force la sortie de la boucle, on verra plus tard.
         }
      }
   }
   while(nb>0);
   
   return return_code;
}


int select_database(int selection) // section : 0 auto, 1 mysql, 2 sqlite3
{
   int ret;
   unsigned long _mysql_thread_id = -1, _mysql_thread_id_avant = -1;
   time_t now = time(NULL);
   
   DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, start db selection (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));

   if( selection == 2 && _md->conn)
   {
      mysql_close(_md->conn);
//      mysql_thread_end();
      _md->conn=NULL;
   }
   
   if( selection == 1 && _md->db)
   {
      sqlite3_close(_md->db);
      _md->db=NULL;
   }
   
   if(_md->conn && (selection != 2)) // on a déjà été connecté un jour ...
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, good, _md->conn not null (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      _mysql_thread_id_avant=mysql_thread_id(_md->conn); // on récupère d'abord l'id du thread, il sera utile pour savoir s'il y a eu une reconnexion.

      // on s'assure d'abord que la connexion avec le serveur Mysql est encore possible
      ret=mysql_ping(_md->conn); // le ping pour éventuellement forcer une reconnexion
      if(ret) // pas de réponse au ping et donc reconnexion impossible.
      {
         VERBOSE(5) mea_log_printf("%s (%s) : mysql_ping - %u: %s\n", INFO_STR, __func__, mysql_errno(_md->conn), mysql_error(_md->conn));
         mysql_close(_md->conn);
//         mysql_thread_end();
         _md->conn=NULL;
         DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, _md->conn closed (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
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
            VERBOSE(9) mea_log_printf("%s (%s) : a mysql_ping auto-reconnect occured.\n", INFO_STR, __func__);
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, good, mysql connection check and available (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
   }

   if(_md->conn == NULL && selection != 2) // jamais connecté ou plus de connexion, on essaye de se connecter
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, try to (re)connect to mysql database (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      ret=_connect(&_md->conn);
      if(ret)
      {
         VERBOSE(5) mea_log_printf("%s  (%s) : _connect - can't connect to mysql server\n",ERROR_STR,__func__);
      }
      else
      {
 //        _md->opened=1;
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, (re)connection status = %d (%d)\n",DEBUG_STR, __func__, ret, (int)(time(NULL)-now));
   }
         
   if((_md->conn==NULL) || selection == 2) // toujours pas de connexion Mysql. Repli sur sqlite3, ouverture si nécessaire
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, try to connect to sqlite3 if needed (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(!_md->db) // sqlite n'est pas ouverte
      {
         ret = sqlite3_open (_md->sqlite3_db_path, &_md->db);
         if(ret)
         {
            _md->db=NULL;
            VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (_md->db));
            DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, selection error, no database opened (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
            return -1;
         }
      }
      else
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, sqlite3 all ready opened (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      }
   }

   DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, selection done (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
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
   
//   time_t last_time = 0;
   
   _md->db=NULL; // descritpteur SQLITE
   _md->conn=NULL; // descripteur com. MYSQL
   _md->started=0;
   
//   int hrtbt=process_heartbeat(_dbServer_monitoring_id);
   process_heartbeat(_dbServer_monitoring_id);
//   DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat. Previous : %d seconds ago.\n", DEBUG_STR,__func__, hrtbt);
   pthread_testcancel();

   pthread_cleanup_push( (void *)set_dbServer_isnt_running, (void *)NULL );
   _dbServer_thread_is_running=1;

   mysql_library_init(0, NULL, NULL);
    
   // ce qui suit va permettre de vider les transactions en stock au démarrage
   ret=select_database(0); // on tente d'ouvrir la base mysql
   
   if(ret==0) // l'une des deux bases est ouverte, mais on ne sais pas encore la quelle
   {
      if(_md->conn) // c'est mysql
      {
         // on force l'ouverture de la base sqlite3
         ret = sqlite3_open (_md->sqlite3_db_path, &_md->db);
         if(ret)
         {
            _md->db=NULL;
            VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (_md->db));
         }
         else
         {
            // comme les deux bases sont ouvertes flush_data va d'abord vider la base sqlide3 dans mysql
            flush_data(1);
         }
      }
      else // arg !!!, c'est sqlite3
      {
         // pas de mysql au demarrage du thread ...
         if(first_start) // et c'est la première fois depuis le lancement du programme que le thread est lancé
         {
            //  dans ce cas on "arrête" le process (il pourra être relancé manuellement)
            managed_processes.processes_table[_dbServer_monitoring_id]->status=STOPPED;
            if(_md->db) // on fait le menage
            {
               sqlite3_close(_md->db);
               _md->db=NULL;
            }
            VERBOSE(2) mea_log_printf("%s (%s) : no mysql database available at first start. DBSERVER goes down.\n", ERROR_STR,__func__);

//            mysql_thread_end();
            pthread_exit(NULL); // et on s'arrête
         }
      }
   }
   else // aucune db disponible
   {
      managed_processes.processes_table[_dbServer_monitoring_id]->status=STOPPED;
//      mysql_thread_end();
      pthread_exit(NULL);
   }
   
   _md->started=1;
   first_start=0;

   if(1){
      char hostname[256];
      gethostname(hostname, sizeof(hostname)-1);
      add_or_update_collector_in_mysql_db((int)_md->collector_id, hostname, "");
   }

   while(1)
   {
      process_heartbeat(_dbServer_monitoring_id);
      pthread_testcancel();

      process_update_indicator(_dbServer_monitoring_id, "DBSERVERINSQLITE", insqlite_indicator);
      process_update_indicator(_dbServer_monitoring_id, "DBSERVERMYWRITE", mysqlwrite_indicator);

      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(_md->lock));
      pthread_mutex_lock(&(_md->lock));
      if(_md->queue)
      {
         process_update_indicator(_dbServer_monitoring_id, "DBSERVERINMEM", _md->queue->nb_elem);
         nb=(int)_md->queue->nb_elem;
      }
      else
         nb=-1;
      pthread_mutex_unlock(&(_md->lock));
      pthread_cleanup_pop(0);

      if(nb>0) // s'il y a quelque chose à traiter
      {
         ret=select_database(0);
         if(ret!=-1)
         {
            flush_data(1);
         }
      }
      
      process_heartbeat(_dbServer_monitoring_id);
//      DEBUG_SECTION mea_log_printf("%s (%s) : heatbeat. Previous : %d seconds ago.\n", DEBUG_STR,__func__, hrtbt);
      pthread_testcancel();
      sleep(10);
   }
   
   pthread_cleanup_pop(1);
   return NULL;
}


int dbServer(char *db_server, char *db_server_port, char *base, char *user, char *passwd, char *sqlite3_db_path, char *str_collector_id)
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
 * \param     str_collector_id identifiant unique du collecteur
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   char *ptr = NULL;

   if(!db_server || !base || !user || !passwd || !sqlite3_db_path || !db_server_port)
      return -1;

   _md->db_server=mea_string_alloc_and_copy(db_server);
   IF_NULL_RETURN(_md->db_server,-1);
   
   _md->db_server_port=mea_string_alloc_and_copy(db_server_port);
   IF_NULL_RETURN(_md->db_server,-1);
   
   _md->base=mea_string_alloc_and_copy(base);
   IF_NULL_RETURN(_md->db_server,-1);

   _md->user=mea_string_alloc_and_copy(user);
   IF_NULL_RETURN(_md->user,-1);
   
   _md->passwd=mea_string_alloc_and_copy(passwd);
   IF_NULL_RETURN(_md->passwd,-1);
   
   _md->sqlite3_db_path=mea_string_alloc_and_copy(sqlite3_db_path);
   IF_NULL_RETURN(_md->sqlite3_db_path,-1);
   
   _md->collector_id = strtoul(str_collector_id, &ptr, 10);
   if(ptr[0]!=0)
      return -1;

   _md->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!_md->queue)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't create queue.\n",ERROR_STR,__func__);
      return -1;
   }
   mea_queue_init(_md->queue); // initialisation de la file
   
   pthread_mutex_init(&_md->lock, NULL);
   
//   if(pthread_create (&(_md->thread), NULL, dbServer_thread, (void *)_md))
   if(pthread_create (&(_md->thread), NULL, dbServer_thread, NULL))   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't create thread.\n", ERROR_STR, __func__);
      return -1;
   }
   fprintf(stderr,"DBSERVER : %x\n", (unsigned int)_md->thread);
   pthread_detach(_md->thread);
   
   return 0;
}


int stop_dbServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   time_t now = time(NULL);

   if(_md)
   {
      if(_md->thread)
      {
         pthread_cancel(_md->thread);
         int counter=500; // 5 secondes environ
         while(counter--)
         {
            if(_dbServer_thread_is_running)
            {  // pour éviter une attente "trop" active
               usleep(10000); // will sleep for 10 ms
            }
            else
               break;
            DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, fin après %d itération(s) (%d)\n",DEBUG_STR, __func__,500-counter,(int)(time(NULL)-now));
         }
      }
      
      _md->started=0;

      if(select_database(2)!=0)
      {
         if(flush_data(0)<0) // flush en local uniquement
         {
            // signaler risque de perte de données
         }
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, sqlite3_close (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->db)
      {
         sqlite3_close(_md->db);
         _md->db=NULL;
      }

      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, clean_queue (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      mea_queue_cleanup(_md->queue,_dbServer_free_queue_elem);
      if(_md->queue)
      {
         free(_md->queue);
         _md->queue=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, pthread_mutex_destroy (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      pthread_mutex_destroy(&(_md->lock));
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->db_server) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->db_server)
      {
         free(_md->db_server);
         _md->db_server=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->db_server_port) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->db_server_port)
      {
         free(_md->db_server_port);
         _md->db_server_port=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->base) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->base)
      {
         free(_md->base);
         _md->base=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->user) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->user)
      {
         free(_md->user);
         _md->user=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->passwd) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->passwd)
      {
         free(_md->passwd);
         _md->passwd=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->sqlite3_db_path) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      if(_md->sqlite3_db_path)
      {
         free(_md->sqlite3_db_path);
         _md->sqlite3_db_path=NULL;
      }
      
      DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));
      free(_md);
      _md=NULL;
   }
   
   DEBUG_SECTION mea_log_printf("%s (%s) : DBSERVER, free(_md->user) (%d)\n",DEBUG_STR, __func__,(int)(time(NULL)-now));

   _dbServer_monitoring_id=-1;
   
   VERBOSE(1) mea_log_printf("%s (%s) : DBSERVER, last chrono = %d.\n", DEBUG_STR, __func__, (int)(time(NULL)-now));
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
                dbServerData->params_list[SQLITE3_DB_BUFF_PATH],
                dbServerData->params_list[COLLECTOR_ID]);
   if(ret==-1)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : can't init data base communication.\n", ERROR_STR, __func__);
      mea_notify_printf('E', "DBSERVER : can't init data base communication.");
      return -1;
   }
   _dbServer_monitoring_id=my_id;

   VERBOSE(1) mea_log_printf("%s (%s) : DBSERVER %s.\n", INFO_STR, __func__, launched_successfully_str);
   mea_notify_printf('S', "DBSERVER %s.", launched_successfully_str);

#else
   VERBOSE(9) mea_log_printf("%s (%s) : dbServer desactivated.\n", INFO_STR, __func__);
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

