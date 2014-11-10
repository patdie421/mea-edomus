/*
 *  interface_type_001.h
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_001_h
#define __interface_type_001_h

#include <sqlite3.h>
#include <inttypes.h>

#include "error.h"
#include "comio2.h"
#include "dbServer.h"
#include "xPLServer.h"

#include "interface_type_001.h"

struct interface_type_001_indicators_s
{
   uint32_t nbxplin;
   uint32_t nbactuatorsout;
   uint32_t nbactuatorsxplrecv;
   uint32_t nbactuatorsouterr;
   
   uint32_t nbsensorstraps;
   uint32_t nbsensorsread;
   uint32_t nbsensorsreaderr;
   uint32_t nbsensorsxplsent;
   uint32_t nbsensorsxplrecv;
  
   uint32_t nbcounterstraps;
   uint32_t nbcountersread;
   uint32_t nbcountersreaderr;
   uint32_t nbcountersxplsent;
   uint32_t nbcountersxplrecv;
};

typedef struct interface_type_001_s
{
   uint16_t interface_id;
   char name[41];
   pthread_t  *thread_id; // thread id
   volatile sig_atomic_t thread_is_running;
   int monitoring_id;
   comio2_ad_t *ad; // comio descriptor
   int loaded;
   queue_t *counters_list; // counter sensors attach to interface
   queue_t *actuators_list;
   queue_t *sensors_list;
   xpl_f xPL_callback;
   
   struct interface_type_001_indicators_s indicators;

} interface_type_001_t;


struct interface_type_001_start_stop_params_s
{
   interface_type_001_t *i001;
   sqlite3 *sqlite3_param_db;
   char dev[81];
};


typedef float (*compute_f)(unsigned int value);

int stop_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg);
int start_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg);

int clean_interface_type_001(interface_type_001_t *i001);

#endif
