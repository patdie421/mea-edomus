/*
 *  tomysqldb.h
 *
 *  Created by Patrice Dietsch on 13/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __dbServer_h
#define __dbServer_h

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

#include <sqlite3.h>
#include <mysql.h>

#include "queue.h"

#define TOMYSQLDB_TYPE_SENSORS_VALUES 1

struct sensor_value_s
{
   uint16_t sensor_id;
   struct timeval date_tv;
   double value1; // valeur principale
   uint16_t unit; // code unité de mesure (s'applique à la valeur principale)
   double value2; // valeur secondaire
   char *complement; // spécifique à un capteur données stocké sous forme de chaine de caractères
};


typedef struct tomysqldb_queue_elem_s
{
   unsigned char type;
   void *data;
   void (*freedata)();
} tomysqldb_queue_elem_t;


typedef struct tomysqldb_md_s
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

} tomysqldb_md_t;

int  tomysqldb_init(tomysqldb_md_t *md, char *db_server, char *db_server_port, char *base, char *user, char *passwd, char *sqlite3_db_path);
void tomysqldb_release(tomysqldb_md_t *md);

int16_t tomysqldb_add_data_to_sensors_values(tomysqldb_md_t *md, uint16_t sensor_id, double value1, uint16_t unit, double value2, char *complement);

tomysqldb_md_t *start_dbServer(char **params_list, sqlite3 *sqlite3_param_db);
void stop_dbServer(tomysqldb_md_t *md);

#endif