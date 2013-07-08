//
//  interface_type_001_actuators.c
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
#include "arduino_pins.h"
#include "parameters_mgr.h"
#include "token_strings.h"
#include "string_utils.h"

#include "interface_type_001_actuators.h"


// PIN=D5;TYPE=DIGITAL_OUT
char *valid_relay_params[]={"S:PIN","S:MODE","S:ACTION",NULL};
#define ACTUATOR_PARAMS_PIN        0
#define ACTUATOR_PARAMS_TYPE       1
#define ACTUATOR_PARAMS_ACTION     2


struct assoc_s type_pin_assocs_i001_actuators[] = {
   {DIGITAL_ID,ARDUINO_D5},
   {DIGITAL_ID,ARDUINO_D11},
   {DIGITAL_ID,ARDUINO_D13},
   {DIGITAL_ID,ARDUINO_AI0},
   {DIGITAL_ID,ARDUINO_AI1},
   {DIGITAL_ID,ARDUINO_AI2},
   {ANALOG_ID, ARDUINO_D5},
   {ANALOG_ID, ARDUINO_D11},
   {-1,-1}
};


struct assoc_s type_action_assocs_i001_actuator[] = {
   {DIGITAL_ID, XPL_PULSE_ID},
   {DIGITAL_ID, ONOFF_ID},
   {ANALOG_ID, PWM_ID},
   {-1,-1}
};


void _interface_type_001_free_actuators_queue_elem(void *d)
{
   struct actuator_s *e=(struct actuator_s *)d;
   
   free(e);
   e=NULL;
}


int actuator_pin_type_i001(int token_type_id, int pin_id)
{
   return is_in_assocs_list(type_pin_assocs_i001_actuators, token_type_id, pin_id);
}


int actuator_type_action_i001(int token_type_id, int token_action_id)
{
   if(token_action_id==0)
      return 1;
   
   return is_in_assocs_list(type_action_assocs_i001_actuator, token_type_id, token_action_id);
}


int valide_actuator_i001(int token_type_id, int pin_id, int token_action_id, int *err)
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
   
   int ret=actuator_pin_type_i001(token_type_id, pin_id);
   
   if(!ret)
   {
      *err=3;
      VERBOSE(5) fprintf(stderr,"%s (%s) : bad pin (%d) for pin type (%d)\n",ERROR_STR,__func__,pin_id,token_type_id);
      return 0;
   }
   
   ret=actuator_type_action_i001(token_type_id, token_action_id);
   return ret;
}


struct actuator_s *valid_and_malloc_actuator(int id_sensor_actuator, char *name, char *parameters)
{
   unsigned char pin_id;
   int type_id;
   int action_id;
   
   parsed_parameter_t *relay_params=NULL;
   int nb_relay_params;
   int err;
   
   struct actuator_s *actuator=(struct actuator_s *)malloc(sizeof(struct actuator_s));
   if(!actuator)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto valid_and_malloc_relay_clean_exit;
   }

   relay_params=malloc_parsed_parameters((char *)parameters, valid_relay_params, &nb_relay_params, &err,1);
   if(relay_params)
   {
      type_id=get_id_by_string(relay_params[ACTUATOR_PARAMS_TYPE].value.s);
      pin_id=get_arduino_pin(relay_params[ACTUATOR_PARAMS_PIN].value.s);
      if(relay_params[ACTUATOR_PARAMS_ACTION].label) // action set ?
         action_id=get_id_by_string(relay_params[ACTUATOR_PARAMS_ACTION].value.s);
      else
         action_id=0;
      
      if(valide_actuator_i001(type_id,pin_id,action_id,&err))
      {
         strcpy(actuator->name,(char *)name);
         actuator->actuator_id=id_sensor_actuator;
         actuator->arduino_pin=pin_id;
         actuator->arduino_pin_type=type_id;
         actuator->arduion_pin_option=0;
         actuator->action=action_id;
         actuator->old_val=0;
         
         switch(action_id)
         {
            case XPL_PULSE_ID:
               actuator->arduino_function=0;
               actuator->arduion_pin_option=25;
               break;
            case ONOFF_ID:
               actuator->arduino_function=1;
               break;
            case PWM_ID:
               actuator->arduino_function=2;
               break;
            case 0:
               switch(type_id)
               {
                  case DIGITAL_ID:
                     actuator->arduino_function=1;
                     break;
                  case ANALOG_ID:
                     actuator->arduino_function=2;
                     break;
               }
               break;
         }
         
         free_parsed_parameters(relay_params, nb_relay_params);
         free(relay_params);

         return actuator;
      }
      else
      {
         VERBOSE(2) fprintf (stderr, "%s (%s) : parametres (%s) non valides\n",ERROR_STR,__func__,parameters);
         goto valid_and_malloc_relay_clean_exit;
      }
   }
   else
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s/%s invalid. Check parameters.\n",ERROR_STR,__func__,name,parameters);
      }
   }
   
valid_and_malloc_relay_clean_exit:
   if(actuator)
      free(actuator);
   if(relay_params)
   {
      free_parsed_parameters(relay_params, nb_relay_params);
      free(relay_params);
   }
   return NULL;
}


mea_error_t xpl_actuator(interface_type_001_t *i001, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type)
{
   int ret;
   int comio_err;
   uint16_t val;
   int type_id;
   
   /*
    if(strcmplower(type, get_token_by_id(XPL_OUTPUT_ID))!=0)
    return 0; // on ne sais traiter que output
    */
   type_id=get_id_by_string(type);
   if(type_id != XPL_OUTPUT_ID && type_id !=VARIABLE_ID)
      return ERROR;
   
   if(first_queue(i001->actuators_list)==-1)
      return ERROR;
   
   struct actuator_s *iq;
   while(1)
   {
      current_queue(i001->actuators_list, (void **)&iq);
      
      if(strcmplower(iq->name,device)==0) // OK, c'est bien pour moi ...
      {
         if(type_id==XPL_OUTPUT_ID)
         {
            switch(iq->action)
            {
               case XPL_PULSE_ID:
               {
                  if(strcmplower(xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_CURRENT_ID)), get_token_by_id(XPL_PULSE_ID))==0)
                  {
                     int pulse_width;
                     
                     char *data1=xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_DATA1_ID));
                     if(data1)
                     {
                        pulse_width=atoi(data1);
                        if(pulse_width<=0)
                           pulse_width=250;
                     }
                     else
                        pulse_width=250;
                     
                     VERBOSE(9) fprintf(stderr,"%s  (%s) : %s PLUSE %d ms on %d\n",INFO_STR,__func__,device,pulse_width,iq->arduino_pin);
                     
                     val=(iq->arduino_pin) << 8;
                     val=val | ((pulse_width / 100) & 0xFF);
                     
                     pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&i001->operation_lock) );
                     pthread_mutex_lock(&i001->operation_lock);
                     comio_call(i001->ad, iq->arduino_function, val, &comio_err);
                     pthread_mutex_unlock(&i001->operation_lock);
                     pthread_cleanup_pop(0);
                  }
                  else
                     return ERROR;
                  return NOERROR;
                  break;
               }
                  
               case 0: // pas d'action, donc valeur par defaut = ONOFF
               case XPL_OUTPUT_ID:
               {
                  int o=0;
                  
                  char *current_value=xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_CURRENT_ID));
                  
                  if(strcmplower(current_value, "high")==0)
                     o=255;
                  else if(strcmplower(current_value, "low")==0)
                     o=0;
                  else
                     return ERROR;
                  
                  VERBOSE(9) fprintf(stderr,"%s  (%s) : %s set %d on pin %d\n",INFO_STR,__func__,device,o,iq->arduino_pin);
                  
                  val=(iq->arduino_pin) << 8;
                  val=val | o;
                  
                  pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&i001->operation_lock) );
                  pthread_mutex_lock(&i001->operation_lock);
                  comio_call(i001->ad, iq->arduino_function, val, &comio_err);
                  pthread_mutex_unlock(&i001->operation_lock);
                  pthread_cleanup_pop(0);
                  
                  return NOERROR;
                  break;
               }
            }
         }
         else if(type_id==VARIABLE_ID)
         {
            switch(iq->action)
            {
               case PWM_ID:
               {
                  int o=0;
                  
                  char *current_value=xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_CURRENT_ID));
                  
                  int current_id=get_id_by_string(current_value);
                  
                  switch(current_id)
                  {
                     case DEC_ID:
                        o=iq->old_val-1;
                        break;
                     case INC_ID:
                        o=iq->old_val+1;
                        break;
                     default:
                     {
                        char inc_dec=0; // 1 = inc ; 2 = dec
                        char *str;
                        if(current_value[0]=='-')
                        {
                           inc_dec=2;
                           str=&current_value[1];
                        }
                        else if (current_value[0]=='+')
                        {
                           inc_dec=1;
                           str=&current_value[1];
                        }
                        else str=current_value;
                        
                        int n;
                        ret=sscanf(str,"%d%n",&o,&n);
                        
                        if(ret==1 && !(strlen(str)-n))
                        {
                           switch(inc_dec)
                           {
                              case 0:
                                 break;
                              case 1:
                                 o=iq->old_val+o;
                                 break;
                              case 2:
                                 o=iq->old_val-o;
                                 break;
                           }
                        }
                        else
                        {
                           VERBOSE(9) fprintf(stderr,"%s  (%s) : %s ???\n",INFO_STR,__func__,current_value);
                           return ERROR; // erreur de syntaxe ...
                        }
                     }
                     break;
                  }
                  
                  if(o>255)
                     o=255;
                  else if(o<0)
                     o=0;
                  iq->old_val=o;
                  
                  // fonction à ajouter dans l'arduino
                  VERBOSE(9) fprintf(stderr,"%s  (%s) : %s set %d on pin %d\n",INFO_STR, __func__,device,o,iq->arduino_pin);
                  
                  val=(iq->arduino_pin) << 8;
                  val=val | o;
                  
                  pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&i001->operation_lock) );
                  pthread_mutex_lock(&i001->operation_lock);
                  comio_call(i001->ad, iq->arduino_function, val, &comio_err);
                  pthread_mutex_unlock(&i001->operation_lock);
                  pthread_cleanup_pop(0);
                  
                  return NOERROR;
                  break;
               }
            }
         }
         return ERROR;
      }
      ret=next_queue(i001->actuators_list);
      if(ret<0)
         break;
   }
   return ERROR;
}
