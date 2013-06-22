//
//  interface_type_001_actuators.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//
#ifndef _interface_type_001_actuators_h
#define _interface_type_001_actuators_h

#include "error.h"

#include "interface_type_001.h"

// modélisation d'un relai
struct actuator_s
{
   char name[20];
   int actuator_id;
   
   char arduino_pin;
   char arduino_pin_type;
   char action;
   int arduion_pin_option; // ( pulse width )
   char arduino_function;
   
   int old_val;
};

void _interface_type_001_free_actuators_queue_elem(void *d);
struct actuator_s *valid_and_malloc_actuator(int id_sensor_actuator, char *name, char *parameters);
mea_error_t xpl_actuator(interface_type_001_t *i001, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type);

#endif
