/*
 *  tomysqldb.h
 *
 *  Created by Patrice Dietsch on 13/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __tomysqldb_h
#define __tomysqldb_h

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

#include "queue.h"

#define TOMYSQLDB_TYPE_PROD        1
#define TOMYSQLDB_TYPE_CONSO       2
#define TOMYSQLDB_TYPE_TEMPERATURE 5
#define TOMYSQLDB_TYPE_PINST       6
#define TOMYSQLDB_TYPE_EC          7

//#define TOMYSQLDB_TYPE_P_INST_P    3
//#define TOMYSQLDB_TYPE_P_INST_C    4

struct electricity_counters_query_s
{
   int sensor_id;
   
   struct timeval date_tv;
   int wh_counter;
   int kwh_counter;
   char unsigned flag;
};


struct pinst_query_s
{
   int sensor_id;

   struct timeval date_tv;
   float power;
   float delta_t;
   char unsigned flag;
};


struct requete_temperature
{
   struct timeval date_tv;
   float temperature;
   char unsigned thermometre;
   char unsigned flag;
   char addr_source[18];
};


typedef struct tomysqldb_queue_elem_s
{
   unsigned char type;
   void *data;
   
} tomysqldb_queue_elem_t;


typedef struct tomysqldb_md_s
{
   queue_t *queue;
   pthread_t thread;
   pthread_mutex_t lock;
   char *db_server;
   char *base;
   char *user;
   char *passwd;
   
   char *sqlite3_db_path;
   
} tomysqldb_md_t;


int  tomysqldb_init(tomysqldb_md_t *md, char *db_server, char *base, char *user, char *passwd, char *sqlite3_db_path);
void tomysqldb_release(tomysqldb_md_t *md);


#endif