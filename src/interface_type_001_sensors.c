//
//  interface_type_002_sensors.c
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "debug.h"
#include "xPLServer.h"
#include "arduino_pins.h"
#include "parameters_mgr.h"
#include "token_strings.h"
#include "interface_type_001_sensors.h"


// parametres valide pour les capteurs ou actionneurs pris en compte par le type 1.
char *valid_sensor_params[]={"S:PIN","S:TYPE","S:COMPUTE","S:ALGO",NULL};
#define SENSOR_PARAMS_PIN       0
#define SENSOR_PARAMS_TYPE      1
#define SENSOR_PARAMS_COMPUTE   2
#define SENSOR_PARAMS_ALGO      3


struct assoc_s type_pin_assocs_i001_sensors[] = {
   {DIGITAL_IN_ID, ARDUINO_D4},
   {DIGITAL_IN_ID, ARDUINO_D10},
   {DIGITAL_IN_ID, ARDUINO_D12},
   {ANALOG_IN_ID,  ARDUINO_AI3},
   {-1,-1}
};


struct assoc_s type_compute_assocs_i001_sensors[] = {
   {ANALOG_IN_ID, XPL_TEMP_ID},
   {ANALOG_IN_ID, XPL_VOLTAGE_ID},
   {-1,-1}
};


struct assoc_s compute_algo_assocs_i001_sensors[] = {
   {XPL_TEMP_ID,XPL_TMP36_ID},
   {XPL_VOLTAGE_ID,XPL_AREF5_ID},
   {XPL_VOLTAGE_ID,XPL_AREF11_ID},
   {-1,-1}
};


float compute_tmp36(unsigned int value)
{
   return (value * 1.1/1024.0-0.5)*100.0;
}


float compute_aref5(unsigned int value)
{
   return value * 1.1/1024.0;
}


float compute_aref11(unsigned int value)
{
   return value * 5/1024.0;
}


float compute_default(unsigned int value)
{
   return (float)value;
}


void _interface_type_001_free_sensors_queue_elem(void *d)
{
   struct sensor_s *e=(struct sensor_s *)d;
   
   free(e);
   e=NULL;
}


int digital_in_trap(int numTrap, void *args, char *buff)
{
   struct sensor_s *sensor;
   
   sensor=(struct sensor_s *)args;
   
   sensor->val=(unsigned char)buff[0];
   
   //   pthread_mutex_lock(&(counter->lock));
   {
      char value[20];
      xPL_ServicePtr servicePtr;
      
      servicePtr=get_xPL_ServicePtr();
      if(!servicePtr)
         return 1;
      xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_TRIGGER);
      
      if(buff[0]==0)
      {
         sensor->val=0;
         sprintf(value,"low");
      }
      else
      {
         sensor->val=1;
         sprintf(value,"high");
      }
      
      xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),sensor->name);
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_INPUT_ID));
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID),value);
      
      // Broadcast the message
      xPL_sendMessage(cntrMessageStat);
      
      xPL_releaseMessage(cntrMessageStat);
   }
   
   //   pthread_mutex_unlock(&(counter->lock));
   
   printf("Changement d'état de %d\n",numTrap-10);
   
   printf("%d\n",buff[0]);
   
   return 0;
}


int sensor_pin_type_i001(int token_type_id, int pin_id)
{
   return is_in_assocs_list(type_pin_assocs_i001_sensors, token_type_id, pin_id);
}


int sensor_type_compute_i001(int token_type_id, int token_compute_id)
{
   if(token_compute_id==-1)
      return 1;
   
   return is_in_assocs_list(type_compute_assocs_i001_sensors, token_type_id, token_compute_id);
}


int sensor_compute_algo_i001(int token_compute_id, int token_algo_id)
{
   return is_in_assocs_list(compute_algo_assocs_i001_sensors, token_compute_id, token_algo_id);
}


int valide_sensor_i001(int token_type_id, int pin_id, int token_compute_id, int token_algo_id, int *err)
{
   if(token_type_id==-1)
   {
      *err=1;
      VERBOSE(5) fprintf(stderr,"ERROR (valide_sensor_i001) : bad i/o type (%d)\n",token_type_id);
   }
   
   if(pin_id==-1)
   {
      *err=2;
      VERBOSE(5) fprintf(stderr,"ERROR (valide_sensor_i001) : bad pin (%d)\n",pin_id);
   }
   
   int ret;
   
   ret=sensor_pin_type_i001(token_type_id,pin_id);
   if(!ret)
   {
      *err=3;
      VERBOSE(5) fprintf(stderr,"ERROR (valide_sensor_i001) : bad pin (%d) for pin type (%d)\n",pin_id,token_type_id);
      return 0;
   }
   
   if(token_compute_id!=-1)
   {
      ret=sensor_type_compute_i001(token_type_id,token_compute_id);
      if(!ret)
      {
         *err=3;
         VERBOSE(5) fprintf(stderr,"ERROR (valide_sensor_i001) : bad compute (%d) for pin type (%d)\n",token_compute_id,token_type_id);
         return 0;
      }
      
      if(token_algo_id!=-1)
      {
         ret=sensor_compute_algo_i001(token_compute_id,token_algo_id);
         if(!ret)
         {
            *err=4;
            VERBOSE(5) fprintf(stderr,"ERROR (valide_sensor_i001) : bad algo (%d) for compute (%d)\n",token_algo_id,token_compute_id);
            return 0;
         }
      }
   }
   else
   {
      if(token_algo_id!=-1)
      {
         *err=4;
         VERBOSE(5) fprintf(stderr,"ERROR (valide_sensor_i001) : aglo set (%d) but non compute set\n",token_algo_id);
         return 0;
      }
   }
   
   return 1;
}


struct sensor_s *valid_and_malloc_sensor(int id_sensor_actuator, char *name, char *parameters)
{
   int pin_id;
   int type_id;
   int compute_id;
   int algo_id;
   
   parsed_parameter_t *sensor_params=NULL;
   int nb_sensor_params;
   int err;
   
   struct sensor_s *sensor=(struct sensor_s *)malloc(sizeof(struct sensor_s));
   if(!sensor)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (valid_and_malloc_sensor) : malloc (%s/%d) - ",__FILE__,__LINE__-4);
         perror("");
      }
      goto valid_and_malloc_sensor_clean_exit;
   }
   
   sensor_params=malloc_parsed_parameters((char *)parameters, valid_sensor_params, &nb_sensor_params, &err,1);
   
   if(sensor_params)
   {
      type_id=get_id_by_string(sensor_params[SENSOR_PARAMS_TYPE].value.s);
      pin_id=get_arduino_pin(sensor_params[SENSOR_PARAMS_PIN].value.s);
      compute_id=get_id_by_string(sensor_params[SENSOR_PARAMS_COMPUTE].value.s);
      algo_id=get_id_by_string(sensor_params[SENSOR_PARAMS_ALGO].value.s);
      
      switch(type_id)
      {
         case ANALOG_IN_ID:
            sensor->arduino_function=5;
            break;
         case DIGITAL_IN_ID:
            sensor->arduino_function=6;
            break;
         default:
            VERBOSE(1) fprintf (stderr, "ERROR (valid_and_malloc_sensor) : bad sensor type (%s)\n",sensor_params[SENSOR_PARAMS_TYPE].value.s);
            goto valid_and_malloc_sensor_clean_exit;
            break;
      }
      
      if(valide_sensor_i001(type_id,pin_id,compute_id,algo_id,&err))
      {
         strcpy(sensor->name,(char *)name);
         sensor->sensor_id=id_sensor_actuator;
         
         sensor->arduino_pin=pin_id;
         sensor->arduino_pin_type=type_id;
         sensor->compute=compute_id;
         sensor->algo=algo_id;
         
         strcpy(sensor->name,(char *)name);
         sensor->sensor_id=id_sensor_actuator;
         
         switch(algo_id)
         {
            case XPL_TMP36_ID:
               sensor->compute_fn=compute_tmp36;
               break;
            case XPL_AREF5_ID:
               sensor->compute_fn=compute_aref5;
               break;
            case XPL_AREF11_ID:
               sensor->compute_fn=compute_aref11;
               break;
            default:
               sensor->compute_fn=NULL;
               break;
         }
         
         free_parsed_parameters(sensor_params, nb_sensor_params);
         free(sensor_params);
         
         return sensor;
      }
      else
      {
         VERBOSE(1) fprintf (stderr, "ERROR (valid_and_malloc_sensor) : parametres (%s) non valides\n",parameters);
         goto valid_and_malloc_sensor_clean_exit;
      }
   }
   else
   {
      fprintf(stderr,"%s ERROR\n",parameters);
   }
   
valid_and_malloc_sensor_clean_exit:
   if(sensor)
      free(sensor);
   if(sensor_params)
   {
      free_parsed_parameters(sensor_params, nb_sensor_params);
      free(sensor_params);
   }
   return NULL;
}


int xpl_sensor(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type)
{
   queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;
   
   int type_id=get_id_by_string(type);
//   if(type_id != XPL_OUTPUT_ID && type_id !=VARIABLE_ID)
//      return 0;

   first_queue(sensors_list);
   for(int i=0; i<sensors_list->nb_elem; i++)
   {
      current_queue(sensors_list, (void **)&sensor);
      if(strcmplower(device,sensor->name)==0)
      {
         xPL_MessagePtr cntrMessageStat ;
         char value[20];
         
         if(type_id==XPL_TEMP_ID)
         {
            if(sensor->algo==XPL_TMP36_ID)
               sprintf(value,"%0.1f", sensor->computed_val);
            else
               sprintf(value,"%d", sensor->val);
         }
         else if(type_id==XPL_VOLTAGE_ID)
         {
            if(sensor->algo==XPL_AREF5_ID)
               sprintf(value,"%0.1f", sensor->computed_val);
            else
               sprintf(value,"%d", sensor->val);            
         }
         else if(type_id==XPL_INPUT_ID)
         {
            if(sensor->val==0)
               sprintf(value,"low");
            else
               sprintf(value,"high");
         }
         else
            return 0;
         
         VERBOSE(9) fprintf(stderr,"INFO  (xPL_callback) : sensor %s = %s\n",sensor->name,value);
         cntrMessageStat = xPL_createBroadcastMessage(theService, xPL_MESSAGE_STATUS) ;
         
         xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
         xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),sensor->name);
         xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID),type);
         xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID),value);
         
         // Broadcast the message
         xPL_sendMessage(cntrMessageStat);
         
         xPL_releaseMessage(cntrMessageStat);
         
         return 1;
      }
      
      next_queue(sensors_list);
   }
   return 0;
}


