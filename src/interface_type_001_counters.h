//
//  interface_type_001_counters.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//

#ifndef __interface_type_001_counters_h
#define __interface_type_001_counters_h

#include <pthread.h>
#include <inttypes.h>

#include "comio2.h"
#include "dbServer.h"
#include "error.h"
#include "timer.h"

// modélisation d'un compteur
struct electricity_counter_s
{
   char name[20];
   int sensor_id;
   
   uint32_t wh_counter;
   uint32_t kwh_counter;
   uint32_t counter; // wh total
   
   double power; // estimation de la puissance instantanée
   double t; // time reférence
   
   uint32_t last_counter;
   double last_power;
   
   pthread_mutex_t lock;
   
   int sensor_mem_addr[4]; // sensor data addr
   int trap; // comio trap number
   
   mea_timer_t timer;
   mea_timer_t trap_timer;
};

struct electricity_counter_s
            *interface_type_001_sensors_valid_and_malloc_counter(int id_sensor_actuator, char *name, char *parameters);
void         interface_type_001_free_counters_queue_elem(void *d);
//mea_error_t  interface_type_001_counters_process_traps(int numTrap, void *args, char *buff);
int16_t      interface_type_001_counters_process_traps(int16_t numTrap, char *buff, int16_t l_buff, void * args);
//mea_error_t  interface_type_001_counters_process_xpl_msg(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_NameValueListPtr ListNomsValeursPtr,
//                                                         char *device, char *type);
mea_error_t interface_type_001_counters_process_xpl_msg(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_MessagePtr msg, char *device, char *type);


void         interface_type_001_counters_poll_inputs(interface_type_001_t *i001, tomysqldb_md_t *md);
void         interface_type_001_counters_init(interface_type_001_t *i001);

#endif
