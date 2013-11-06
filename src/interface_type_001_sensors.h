//
//  interface_type_001_sensors.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//

#ifndef _interface_type_001_sensors_h
#define _interface_type_001_sensors_h

#include "error.h"
#include "timer.h"
#include "interface_type_001.h"

struct sensor_s
{
   int sensor_id;
   char name[20];
   
   char arduino_pin_type;
   char arduino_pin;
   char arduino_function;
   
   char compute;
   char algo;
   
   unsigned int val;
   unsigned int last;

   compute_f compute_fn;
   
   float computed_val;
   
   mea_timer_t timer;
};

void interface_type_001_sensors_free_queue_elem(void *d);
mea_error_t interface_type_001_sensors_process_traps(int numTrap, void *args, char *buff);

struct sensor_s *interface_type_001_sensors_valid_and_malloc_sensor(int id_sensor_actuator, char *name, char *parameters);
mea_error_t interface_type_001_sensors_process_xpl_msg(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type);
void interface_type_001_sensors_poll_inputs(interface_type_001_t *i001, tomysqldb_md_t *md);
void interface_type_001_sensors_init(interface_type_001_t *i001);

#endif