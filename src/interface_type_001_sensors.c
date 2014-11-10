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

#include "globals.h"
#include "debug.h"
#include "xPLServer.h"
#include "arduino_pins.h"
#include "parameters_utils.h"
#include "string_utils.h"
#include "tokens.h"
#include "interface_type_001_sensors.h"

#include "processManager.h"

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 1.
char *valid_sensor_params[]={"S:PIN","S:TYPE","S:COMPUTE","S:ALGO","I:POLLING_PERIODE",NULL};
#define SENSOR_PARAMS_PIN       0
#define SENSOR_PARAMS_TYPE      1
#define SENSOR_PARAMS_COMPUTE   2
#define SENSOR_PARAMS_ALGO      3
#define SENSOR_PARAMS_POLLING_PERIODE 4


struct assoc_s type_pin_assocs_i001_sensors[] = {
   {DIGITAL_ID, ARDUINO_D4},
   {DIGITAL_ID, ARDUINO_D10},
   {DIGITAL_ID, ARDUINO_D12},
   {ANALOG_ID,  ARDUINO_AI3},
   {-1,-1}
};


struct assoc_s type_compute_assocs_i001_sensors[] = {
   {ANALOG_ID, XPL_TEMP_ID},
   {ANALOG_ID, XPL_VOLTAGE_ID},
   {ANALOG_ID, RAW_ID},
   {-1,-1}
};


struct assoc_s compute_algo_assocs_i001_sensors[] = {
   {XPL_TEMP_ID,XPL_TMP36_ID},
   {XPL_VOLTAGE_ID,XPL_AREF5_ID},
   {XPL_VOLTAGE_ID,XPL_AREF11_ID},
   {-1,-1}
};


float _compute_tmp36(unsigned int value)
{
   return (value * 1.1/1024.0-0.5)*100.0;
}


float _compute_aref5(unsigned int value)
{
   return value * 5/1024.0;
}


float _compute_aref11(unsigned int value)
{
   return value * 1.1/1024.0;
}


float _compute_default(unsigned int value)
{
   return (float)value;
}


int _sensor_pin_type_i001(int token_type_id, int pin_id)
{
   return is_in_assocs_list(type_pin_assocs_i001_sensors, token_type_id, pin_id);
}


int _sensor_type_compute_i001(int token_type_id, int token_compute_id)
{
   if(token_compute_id==-1)
      return 1;
   
   return is_in_assocs_list(type_compute_assocs_i001_sensors, token_type_id, token_compute_id);
}


int _sensor_compute_algo_i001(int token_compute_id, int token_algo_id)
{
   return is_in_assocs_list(compute_algo_assocs_i001_sensors, token_compute_id, token_algo_id);
}


int _valide_sensor_i001(int token_type_id, int pin_id, int token_compute_id, int token_algo_id, int *err)
{
   if(token_type_id==-1)
   {
      *err=1;
      VERBOSE(5) fprintf(stderr,"%s (%s) : bad i/o type (%d)\n",ERROR_STR,__func__,token_type_id);
   }
   
   if(pin_id==-1)
   {
      *err=2;
      VERBOSE(5) fprintf(stderr,"%s (%s) : bad pin (%d)\n",ERROR_STR,__func__,pin_id);
   }
   
   int ret;
   
   ret=_sensor_pin_type_i001(token_type_id,pin_id);
   if(!ret)
   {
      *err=3;
      VERBOSE(5) fprintf(stderr,"%s (%s) : bad pin (%d) for pin type (%d)\n",ERROR_STR,__func__,pin_id,token_type_id);
      return 0;
   }
   
   if(token_compute_id!=-1)
   {
      ret=_sensor_type_compute_i001(token_type_id,token_compute_id);
      if(!ret)
      {
         *err=3;
         VERBOSE(5) fprintf(stderr,"%s (%s) : bad compute (%d) for pin type (%d)\n",ERROR_STR,__func__,token_compute_id,token_type_id);
         return 0;
      }
      
      if(token_algo_id!=-1)
      {
         ret=_sensor_compute_algo_i001(token_compute_id,token_algo_id);
         if(!ret)
         {
            *err=4;
            VERBOSE(5) fprintf(stderr,"%s (%s) : bad algo (%d) for compute (%d)\n",ERROR_STR,__func__,token_algo_id,token_compute_id);
            return 0;
         }
      }
   }
   else
   {
      if(token_algo_id!=-1)
      {
         *err=4;
         VERBOSE(5) fprintf(stderr,"%s (%s) : aglo set (%d) but non compute set\n",ERROR_STR,__func__,token_algo_id);
         return 0;
      }
   }
   
   return 1;
}

void interface_type_001_sensors_free_queue_elem(void *d)
{
   struct sensor_s *e=(struct sensor_s *)d;
   
   free(e);
   e=NULL;
}

//typedef int16_t (*trap_f)(int16_t, char *, int16_t, void *);
int16_t interface_type_001_sensors_process_traps(int16_t numTrap, char *data, int16_t l_data, void *userdata)
{
   struct sensor_s *sensor;
   
//   sensor=(struct sensor_s *)args;
   sensor=(struct sensor_s *)userdata;
   
//   sensor->val=(unsigned char)buff[0];
   sensor->val=(unsigned char)data[0];
   
   {
      *(sensor->nbtrap)=*(sensor->nbtrap)+1;
      
      char value[20];
      xPL_ServicePtr servicePtr;
      
      servicePtr=mea_getXPLServicePtr();
      if(!servicePtr)
         return 1;
      
      xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_TRIGGER);
      
      if(data[0]==0)
      {
         sensor->val=0;
         sprintf(value,"low");
      }
      else
      {
         sensor->val=1;
         sprintf(value,"high");
      }
      
      VERBOSE(9) fprintf(stderr,"%s  (%s) : sensor %s = %s\n",INFO_STR,__func__,sensor->name,value);
      
      xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),sensor->name);
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_INPUT_ID));
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID),value);
      
      // Broadcast the message
      mea_sendXPLMessage(cntrMessageStat);
      
      *(sensor->nbxplout)=*(sensor->nbxplout)+1;
      
      xPL_releaseMessage(cntrMessageStat);
   }
   
   return NOERROR;
}




struct sensor_s *interface_type_001_sensors_valid_and_malloc_sensor(int16_t id_sensor_actuator, char *name, char *parameters)
{
   int pin_id;
   int type_id;
   int compute_id;
   int algo_id;
   
   parsed_parameters_t *sensor_params=NULL;
   int nb_sensor_params;
   int err;
   
   struct sensor_s *sensor=(struct sensor_s *)malloc(sizeof(struct sensor_s));
   if(!sensor)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto interface_type_001_sensors_valid_and_malloc_sensor_clean_exit;
   }
   
   sensor_params=malloc_parsed_parameters((char *)parameters, valid_sensor_params, &nb_sensor_params, &err,1);
   
   if(sensor_params)
   {
      type_id=get_id_by_string(sensor_params[SENSOR_PARAMS_TYPE].value.s);
      pin_id=mea_getArduinoPin(sensor_params[SENSOR_PARAMS_PIN].value.s);
      compute_id=get_id_by_string(sensor_params[SENSOR_PARAMS_COMPUTE].value.s);
      algo_id=get_id_by_string(sensor_params[SENSOR_PARAMS_ALGO].value.s);
      
      switch(type_id)
      {
         case ANALOG_ID:
            sensor->arduino_function=5;
            break;
         case DIGITAL_ID:
            sensor->arduino_function=6;
            break;
         default:
            VERBOSE(1) fprintf (stderr, "%s (%s) : bad sensor type (%s)\n",ERROR_STR,__func__,sensor_params[SENSOR_PARAMS_TYPE].value.s);
            goto interface_type_001_sensors_valid_and_malloc_sensor_clean_exit;
            break;
      }
      
      if(_valide_sensor_i001(type_id,pin_id,compute_id,algo_id,&err))
      {
         strcpy(sensor->name,(char *)name);
         sensor->sensor_id=id_sensor_actuator;
         
         sensor->arduino_pin=pin_id;
         sensor->arduino_pin_type=type_id;
         sensor->compute=compute_id;
         sensor->algo=algo_id;
         
         strcpy(sensor->name,(char *)name);
         mea_strtolower(sensor->name);
         sensor->sensor_id=id_sensor_actuator;
         
         switch(algo_id)
         {
            case XPL_TMP36_ID:
               sensor->compute_fn=_compute_tmp36;
               break;
            case XPL_AREF5_ID:
               sensor->compute_fn=_compute_aref5;
               break;
            case XPL_AREF11_ID:
               sensor->compute_fn=_compute_aref11;
               break;
            default:
               sensor->compute_fn=NULL;
               break;
         }
         
         if(sensor_params[SENSOR_PARAMS_POLLING_PERIODE].value.i>0)
         {
            init_timer(&(sensor->timer),sensor_params[SENSOR_PARAMS_POLLING_PERIODE].value.i,1);
         }
         else
         {
            init_timer(&(sensor->timer),60,1); // lecture toutes les 5 minutes par défaut
         }
         // start_timer(&(sensor->timer));
         sensor->nbtrap=NULL;
         free_parsed_parameters(sensor_params, nb_sensor_params);
         free(sensor_params);
         sensor_params=NULL;
         return sensor;
      }
      else
      {
         VERBOSE(1) fprintf (stderr, "%s (%s) : parametres (%s) non valides\n",ERROR_STR,__func__,parameters);
         goto interface_type_001_sensors_valid_and_malloc_sensor_clean_exit;
      }
   }
   else
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s/%s invalid. Check parameters.\n",ERROR_STR,__func__,name,parameters);
      }
   }
   
interface_type_001_sensors_valid_and_malloc_sensor_clean_exit:
   if(sensor)
   {
      free(sensor);
      sensor=NULL;
   }
   if(sensor_params)
   {
      free_parsed_parameters(sensor_params, nb_sensor_params);
      free(sensor_params);
      sensor_params=NULL;
   }
   return NULL;
}


mea_error_t interface_type_001_sensors_process_xpl_msg(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_MessagePtr msg, char *device, char *type)
/**
  * \brief     Traite les demandes xpl de retransmission de la valeur courrante ("sensor.request/request=current") pour interface_type_001
  * \details   La demande sensor.request peut être de la forme est de la forme :
  *            sensor.request
  *            {
  *               request=current
  *               [device=<device>]
  *               [type=<type>]
  *            }
  *            device et type sont optionnels. S'il ne sont pas précisé, le statut tous les capteurs sera émis. Si le type est précisé sans device,
  *            tous les statuts des capteurs du type "type" seront transmis.
  *
  * \param     i001                contexte de l'interface.
  * \param     theService          xxx
  * \param     ListNomsValeursPtr  xxx
  * \param     device              le périphérique à interroger ou NULL
  * \param     device              le type à interroger ou NULL
  * \return    ERROR en cas d'erreur, NOERROR sinon  */  
{
   queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;
   int type_id=0;
   uint16_t send_xpl_flag=0;
   int16_t no_type=0;

//   xPL_NameValueListPtr ListNomsValeursPtr = xPL_getMessageBody(msg);
   i001->indicators.nbsensorsxplrecv++;
   if(type)
   {
      type_id=get_id_by_string(type);
      if(type_id==-1)
         return ERROR; // type inconnu, on ne peut pas traiter
   }
   else
      no_type=1; // type par defaut, le type est celui du capteur
   
   first_queue(sensors_list);
   for(int i=0; i<sensors_list->nb_elem; i++)
   {
      current_queue(sensors_list, (void **)&sensor);
      if(!device || mea_strcmplower(device,sensor->name)==0) // pas de device, on transmettra le statut de tous les capteurs du type demandé (si précisé)
      {
         xPL_MessagePtr cntrMessageStat ;
         char value[20];
          char *unit;
         if(no_type==1) // pas de type demandé, on "calcule" le type du capteur
         {
            if(sensor->arduino_pin_type==ANALOG_ID)
            {
               if(sensor->algo==XPL_AREF5_ID || sensor->algo==XPL_AREF5_ID)
                  type_id=XPL_VOLTAGE_ID;
               else if(sensor->algo==XPL_TMP36_ID)
                  type_id=XPL_TEMP_ID;
               else
                  type_id=GENERIC_ID; // juste une valeur ...
            }
            else if(sensor->arduino_pin_type==DIGITAL_ID)
            {
               type_id=XPL_INPUT_ID;
            }
            type=get_token_by_id(type_id);
         }
         
         // préparation de la réponse
         send_xpl_flag=0;
         switch(type_id)
         {
            case XPL_TEMP_ID:
            {
               if(sensor->arduino_pin_type==ANALOG_ID)
               {
                  if(sensor->algo==XPL_TMP36_ID)
                  {
                     sprintf(value,"%0.2f", sensor->computed_val);
                     unit="c";
                  }
                  else
                  {
                     sprintf(value,"%d", sensor->val);
                     unit=NULL;
                  }
                  send_xpl_flag=1;
               }
               break;
            }
            
            case XPL_VOLTAGE_ID:
            {
               if(sensor->arduino_pin_type==ANALOG_ID)
               {
                  if(sensor->algo==XPL_AREF5_ID || sensor->algo==XPL_AREF11_ID)
                  {
                     sprintf(value,"%0.2f", sensor->computed_val);
                     unit="v";
                  }
                  else
                  {
                     sprintf(value,"%d", sensor->val);
                     unit=NULL;
                  }
                  send_xpl_flag=1;
               }
               break;
            }
            
            case XPL_INPUT_ID:
            {
               if(sensor->arduino_pin_type==DIGITAL_ID)
               {
                  if(sensor->val==0)
                     sprintf(value,"low");
                  else
                     sprintf(value,"high");
                  unit=NULL;
                  send_xpl_flag=1;
               }
               break;
            }
            
            case GENERIC_ID:
            {
               if(sensor->arduino_pin_type==ANALOG_ID)
               {
                  sprintf(value,"%d", sensor->val);
                  unit=NULL;
                  send_xpl_flag=1;
                  break;
               }
            }
            default:
               break;
         }
         
         if(send_xpl_flag)
         {
            VERBOSE(9) fprintf(stderr,"%s  (%s) : sensor %s = %s\n",INFO_STR,__func__,sensor->name,value);
            cntrMessageStat = xPL_createBroadcastMessage(theService, xPL_MESSAGE_STATUS) ;
            xPL_setBroadcastMessage(cntrMessageStat, FALSE);

            xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),sensor->name);
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID),type);
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID),value);
            if(unit)
               xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(UNIT_ID),unit);

            xPL_setTarget(cntrMessageStat, xPL_getSourceVendor(msg), xPL_getSourceDeviceID(msg), xPL_getSourceInstanceID(msg));

            mea_sendXPLMessage(cntrMessageStat);
            
            i001->indicators.nbsensorsxplsent++;
            
            xPL_releaseMessage(cntrMessageStat);
         }
         
         if(device) // demande ciblée ?
            return NOERROR; // oui => fin de traitement, sinon on continu
      }
      next_queue(sensors_list);
   }
   return NOERROR;
}


int16_t interface_type_001_sensors_poll_inputs(interface_type_001_t *i001)
{
   queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;

   int16_t comio2_err;

   first_queue(sensors_list);
   for(int16_t i=0; i<sensors_list->nb_elem; i++)
   {
      current_queue(sensors_list, (void **)&sensor);
      if(!test_timer(&(sensor->timer)))
      {
         if(sensor->arduino_pin_type==ANALOG_ID)
         {
            int v;

            //pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&i001->operation_lock) );
            //pthread_mutex_lock(&i001->operation_lock);

            unsigned char buffer[8], resp[8];
            uint16_t l_resp;
            buffer[0]=sensor->arduino_pin;

            int ret=comio2_call_fn(i001->ad, (uint16_t)sensor->arduino_function, (char *)buffer, 1, &v, resp, &l_resp, &comio2_err);
            if(ret<0)
            {
               VERBOSE(5) {
                  fprintf(stderr,"%s (%s) : comio2 error = %d.\n", ERROR_STR, __func__, comio2_err);
               }
               i001->indicators.nbsensorsreaderr++;
               if(comio2_err == COMIO2_ERR_DOWN)
               {
                  return -1;
               }
               continue;
            }
            else if(ret>0)
            {
               VERBOSE(5) {
                  fprintf(stderr,"%s (%s) : function %d return error = %d.\n", ERROR_STR, __func__, sensor->arduino_function, comio2_err);
               }
               i001->indicators.nbsensorsreaderr++;
               continue;
            }
            
            if(sensor->val!=v)
            {
               int16_t last=sensor->val;
               float computed_last;
               i001->indicators.nbsensorsread++;
               
               sensor->val=v;
               sensor->computed_val=sensor->compute_fn(v);
               computed_last=sensor->compute_fn(last);
                  
               if(sensor->compute==XPL_TEMP_ID)
               {
                  VERBOSE(9) fprintf(stderr,"%s  (%s) : temperature sensor %s =  %.1f °C (%d) \n",INFO_STR,__func__,sensor->name,sensor->computed_val,sensor->val);
                  dbServer_add_data_to_sensors_values(sensor->sensor_id, sensor->computed_val, UNIT_C, sensor->val, "");
               }
               else if(sensor->compute==XPL_VOLTAGE_ID)
               {
                  VERBOSE(9) fprintf(stderr,"%s  (%s) : voltage sensor %s =  %.1f V (%d) \n",INFO_STR,__func__,sensor->name,sensor->computed_val,sensor->val);
               }
               else
               {
                  VERBOSE(9) fprintf(stderr,"%s  (%s) : raw sensor %s = %d\n",INFO_STR,__func__,sensor->name,sensor->val);
               }
                  
               char str_value[20];
               char str_last[20];
                  
               xPL_ServicePtr servicePtr = mea_getXPLServicePtr();
               if(servicePtr)
               {
                  xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_TRIGGER);
               
                  sprintf(str_value,"%0.1f",sensor->computed_val);
                  sprintf(str_last,"%0.1f",computed_last);
                     
                  xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
                  xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),sensor->name);
                  xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_TEMP_ID));
                  xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID),str_value);
                  xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_LAST_ID),str_last);
                     
                  // Broadcast the message
                  mea_sendXPLMessage(cntrMessageStat);
                  
                  (i001->indicators.nbsensorsxplsent)++;
                  
                  xPL_releaseMessage(cntrMessageStat);
               }
            }
         }
         else
         {
            // traiter ici les capteurs logiques
         }
      }
      next_queue(sensors_list);
   }
   return 0;
}


void interface_type_001_sensors_init(interface_type_001_t *i001)
{
   queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;

   first_queue(sensors_list);
   for(int16_t i=0; i<sensors_list->nb_elem; i++)
   {
      current_queue(sensors_list, (void **)&sensor);

      sensor->nbtrap=&(i001->indicators.nbsensorstraps);
      sensor->nbxplout=&(i001->indicators.nbsensorsxplsent);
      
      comio2_setTrap(i001->ad, sensor->arduino_pin+10, interface_type_001_sensors_process_traps, (void *)sensor);

      start_timer(&(sensor->timer));

      next_queue(sensors_list);
   }
}
