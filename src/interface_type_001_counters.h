//
//  interface_type_001_counters.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//

#ifndef _interface_type_001_counters_h
#define _interface_type_001_counters_h

#include "error.h"

#include <pthread.h>

// modélisation d'un compteur
struct electricity_counter_s
{
   char name[20];
   int sensor_id;
   
   int wh_counter;
   int kwh_counter;
   unsigned long counter; // wh total
   
   double power; // estimation de la puissance instantanée
   double t; // time reférence
   
   pthread_mutex_t lock;
   
   int sensor_mem_addr[4]; // sensor data addr
   int trap; // comio trap number
};

struct electricity_counter_s *valid_and_malloc_counter(int id_sensor_actuator, char *name, char *parameters);
void interface_type_001_free_counters_queue_elem(void *d);
error_t counter_trap(int numTrap, void *args, char *buff);


#endif
