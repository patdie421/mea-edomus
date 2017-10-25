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

#include "mea_queue.h"

#define TOMYSQLDB_TYPE_SENSORS_VALUES 1
#define START_BATCH_CONSO             2
#define START_BATCH_PURGE             3
#define START_BATCH_RESYNC            4

struct sensor_value_s
{
   uint16_t sensor_id;
   struct timeval date_tv;
   double value1; // valeur principale
   uint16_t unit; // code unité de mesure (s'applique à la valeur principale)
   double value2; // valeur secondaire
   char *complement; // spécifique à un capteur données stocké sous forme de chaine de caractères
};


typedef struct dbServer_queue_elem_s
{
   unsigned char type;
   void *data;
   void (*freedata)(void *);
} dbServer_queue_elem_t;


struct dbServerData_s
{
   char **params_list;
};


int16_t dbServer_add_data_to_sensors_values(uint16_t sensor_id, double value1, uint16_t unit, double value2, char *complement);

int start_dbServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_dbServer(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_dbServer(int my_id, void *data, char *errmsg, int l_errmsg);

int16_t start_consolidation_batch(void);
int16_t start_purge_batch(void);
int16_t start_resync_batch(void);

#endif
