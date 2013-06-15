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

   compute_f compute_fn;
   
   float computed_val;
};


struct sensor_s *valid_and_malloc_sensor(int id_sensor_actuator, char *name, char *parameters);
void _interface_type_001_free_sensors_queue_elem(void *d);
error_t digital_in_trap(int numTrap, void *args, char *buff);
error_t xpl_sensor(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type);

#endif