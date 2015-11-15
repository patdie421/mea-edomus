#include "interface_type_005.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sqlite3.h>
#include <errno.h>
#include <pthread.h>

#include "mea_verbose.h"
#include "mea_timer.h"
#include "mea_string_utils.h"
#include "mea_queue.h"

#include "cJSON.h"
#include "uthash.h"

#include "globals.h"
#include "consts.h"

#include "tokens.h"

#include "macros.h"
#include "dbServer.h"
#include "parameters_utils.h"
#include "processManager.h"

#include "notify.h"

#include "netatmo.h"

#include "interfacesServer.h"


struct type005_device_queue_elem_s
{
   char device_id[81];

   mea_queue_t modules_list;
};

struct type005_module_queue_elem_s
{
   char module_id[81];
   
   struct netatmo_thermostat_data_s d1, d2;
   struct netatmo_thermostat_data_s *current, *last;

   mea_queue_t sensors_actuators_list;
   struct type005_device_queue_elem_s *device;
};

struct type005_sensor_actuator_queue_elem_s
{
   int id;
   int type;
   char name[41];
   int todbflag;
   int sensor;
   int actuator;
   
   struct type005_module_queue_elem_s *module;
};


char *valid_netatmo_sa_params[]={"S:DEVICE_ID","S:MODULE_ID", "S:SENSOR", "S:ACTUATOR", NULL};
#define PARAMS_DEVICE_ID 0
#define PARAMS_MODULE_ID 1
#define PARAMS_SENSOR    2
#define PARAMS_ACTUATOR  3


enum netatmo_sensor_e { TEMPERATURE, RELAY_CMD, BATTERY, SETPOINT, MODE };

char *interface_type_005_xplin_str="XPLIN";
char *interface_type_005_xplout_str="XPLOUT";


static int16_t _interface_type_005_send_xPL_sensor_basic(interface_type_005_t *i005, int xplmsgtype, char *name, char *type, char *str_value, char *str_last)
{
   xPL_ServicePtr servicePtr = mea_getXPLServicePtr();
   if(servicePtr)
   {
      xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xplmsgtype);

      xPL_setSchema(cntrMessageStat, get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
      xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_DEVICE_ID),name);
      xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_TYPE_ID), type);
      xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_CURRENT_ID),str_value);
      if(str_last)
         xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_LAST_ID),str_last);

      mea_sendXPLMessage(cntrMessageStat);

      (i005->indicators.xplout)++;

      xPL_releaseMessage(cntrMessageStat);
   }
   
   return 0;
}


static struct type005_sensor_actuator_queue_elem_s *_find_sensor_actuator(mea_queue_t *q, char *device)
{
   struct type005_device_queue_elem_s *d;
   struct type005_module_queue_elem_s *m;
   struct type005_sensor_actuator_queue_elem_s *sa;

   mea_queue_first(q);
   while(mea_queue_current(q,(void **)&d)==0)
   {
      mea_queue_first(&(d->modules_list));
      while(mea_queue_current(&(d->modules_list),(void **)&m)==0)
      {
         mea_queue_first(&(m->sensors_actuators_list));
         while(mea_queue_current(&(m->sensors_actuators_list),(void **)&sa)==0)
         {
            if(strcmp(sa->name, device)==0)
               return sa;
 
            mea_queue_next(&(m->sensors_actuators_list));
         }
         mea_queue_next(&(d->modules_list));
      }
      mea_queue_next(q);
   }

   return NULL;
}


int16_t interface_type_005_current_last(struct type005_sensor_actuator_queue_elem_s *e,
                                        char *type,
                                        char *str_current,
                                        char *str_last,
                                        double *d1,
                                        double *d2,
                                        char *comp)
{
   if(!type || !str_current || !str_last)
      return -1;

   if(d1) *d1=0.0;
   if(d2) *d2=0.0;
   if(comp) comp[0]=0;

   switch(e->sensor)
   {
      case TEMPERATURE:
         sprintf(str_current, "%4.1f", e->module->current->temperature);
         sprintf(str_last,    "%4.1f", e->module->last->temperature);
         strcpy(type, "temp");
         if(d1) *d1=(double)e->module->current->temperature;
         break;

      case RELAY_CMD:
         sprintf(str_current, "%d", e->module->current->therm_relay_cmd);
         sprintf(str_last,    "%d", e->module->last->therm_relay_cmd);
         strcpy(type, "output");
         if(d1) *d1=(double)e->module->current->therm_relay_cmd;
         break;

      case BATTERY:
      {
         float v1=(e->module->current->battery_vp-3000.0)/1500.0*100.0;
         if(v1>100.0) v1=100.0;
         if(v1<0.0) v1=0.0;
         float v2=(e->module->last->battery_vp-3000.0)/1500.0*100.0;
         if(v2>100.0) v2=100.0;
         if(v2<0.0) v2=0.0;
         sprintf(str_current, "%3.0f", v1);
         sprintf(str_last,    "%3.0f", v2);
         strcpy(type, "battery");
         if(d1) *d1=(double)e->module->current->battery_vp;
         if(d2) *d2=(double)v1;
         break;
      }

      case MODE:
         sprintf(str_current, "%d", e->module->current->setpoint);
         sprintf(str_last,    "%d", e->module->last->setpoint);
         strcpy(type,"generic");
         if(d1) *d1=(double)e->module->current->setpoint;
         if(comp) strcpy(comp, netatmo_therm_mode[e->module->current->setpoint]);
         break;

     case SETPOINT:
         sprintf(str_current, "%4.1f", e->module->current->setpoint_temp);
         sprintf(str_last,    "%4.1f", e->module->last->setpoint_temp);
         strcpy(type,"setpoint");
         if(d1) *d1=(double)e->module->current->setpoint_temp;
         break;

     default:
         return -1;
   }

   if(strcmp(str_current, str_last)==0)
      return 0;
   else
      return 1;
}



int16_t interface_type_005_xPL_actuator(interface_type_005_t *i005,
                                        xPL_ServicePtr theService,
                                        xPL_NameValueListPtr ListNomsValeursPtr,
                                        char *device,
                                        char *type,
                                        struct type005_sensor_actuator_queue_elem_s *e)
{
// xPLSend -m cmnd -c control.basic device=therm01 mode=manual setpoint=25.5 expire=3600
// xPLSend -m cmnd -c control.basic device=therm01 mode=away expire=3600
// xPLSend -m cmnd -c control.basic device=therm01 mode=away
// xPLSend -m cmnd -c control.basic device=therm01 mode=program
   int16_t ret=-1;
   char err[81];
   if(e->type==501)
   {
      char *mode     = xPL_getNamedValue(ListNomsValeursPtr, "mode");
      char *setpoint = xPL_getNamedValue(ListNomsValeursPtr, "setpoint");
      char *expire   = xPL_getNamedValue(ListNomsValeursPtr, "expire");
      
      char * pEnd;
  
      long int _mode=-1L;
      long int _expire=0L;
      double _setpoint=0.0;
      
      if(mode)
      {
         if(expire)
         {
            _expire=strtol(expire, &pEnd, 10);
            if(*pEnd>0)
               return -1;
         }
      
         if(setpoint)
         {
            _setpoint=strtod(setpoint, &pEnd);
            if(*pEnd>0)
               return -1;
         }

         for(int i=0;netatmo_therm_mode[i];i++)
         {
            if(strcmp(netatmo_therm_mode[i],mode)==0)
            {
               _mode=i;
               break;
            }
         }
         
         switch(_mode)
         {
            case OFF:
            case HG:
            case MAX:
            case PROGRAM:
               _setpoint=0.0;
            case MANUAL:
            case AWAY:
               netatmo_set_thermostat_setpoint(i005->token.access, e->module->device->device_id, e->module->module_id, _mode, _expire, _setpoint, err, sizeof(err)-1);
               break;
               
            default:
               return -1;
         }
      }
      else
         ret=-1;
   }
   return ret;
}


int16_t interface_type_005_xPL_sensor(interface_type_005_t *i005,
                                      xPL_ServicePtr theService,
                                      xPL_MessagePtr msg,
                                      char *device,
                                      char *type,
                                      struct type005_sensor_actuator_queue_elem_s *e)
{
// xPLSend -m cmnd -c sensor.request device=temp02 request=current
   char err[81];
   static time_t chrono=0;
   
   time_t now;
   
   now=time(NULL);
   if(e->type==500)
   {
      char ntype[41], str_value[41], str_last[41];

      if(difftime(now, chrono)>10.0)
      {
         struct netatmo_thermostat_data_s data;
         int ret=netatmo_get_thermostat_data(i005->token.access, e->module->device->device_id, e->module->module_id, &data, err, sizeof(err)-1);
         if(ret==0)
         {
            // switch des espaces de données last et current
            struct netatmo_thermostat_data_s *tmp;
            tmp=e->module->last;
            e->module->last=e->module->current;
            e->module->current=tmp;
            memcpy(e->module->current, &data, sizeof(data));
         }
         chrono=now;
      }
      if(interface_type_005_current_last(e, ntype, str_value, str_last, NULL, NULL, NULL)<0)
         return -1;
      else
      {
         _interface_type_005_send_xPL_sensor_basic(i005, xPL_MESSAGE_STATUS, e->name, ntype, str_value, str_last);
         return 0;
      }
   }
   else
      return -1;
   
   return 0;
}


int16_t interface_type_005_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   xPL_NameValueListPtr ListNomsValeursPtr ;
   char *schema_type = NULL, *schema_class = NULL, *device = NULL, *type = NULL;
   interface_type_005_t *i005=(interface_type_005_t *)userValue;
   schema_class       = xPL_getSchemaClass(theMessage);
   schema_type        = xPL_getSchemaType(theMessage);
   ListNomsValeursPtr = xPL_getMessageBody(theMessage);
   device             = xPL_getNamedValue(ListNomsValeursPtr, get_token_string_by_id(XPL_DEVICE_ID));
   type               = xPL_getNamedValue(ListNomsValeursPtr, get_token_string_by_id(XPL_TYPE_ID));
   
   mea_strtolower(device);
   if(!device)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : xPL message no device\n",INFO_STR,__func__);
      return -1;
   }

   struct type005_sensor_actuator_queue_elem_s *elem=_find_sensor_actuator(&(i005->devices_list), device);
   if(elem==NULL)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : %s not for me\n",INFO_STR,__func__, device);
      return -1;
   } 
   (i005->indicators.xplin)++;
   
   VERBOSE(9) mea_log_printf("%s (%s) : xPL Message to process : %s.%s\n",INFO_STR,__func__,schema_class,schema_type);
   
   if(mea_strcmplower(schema_class, get_token_string_by_id(XPL_CONTROL_ID)) == 0 &&
      mea_strcmplower(schema_type, get_token_string_by_id(XPL_BASIC_ID)) == 0)
   {
      return interface_type_005_xPL_actuator(i005, theService, ListNomsValeursPtr, device, type, elem);
   }
   else if(mea_strcmplower(schema_class, get_token_string_by_id(XPL_SENSOR_ID)) == 0 &&
           mea_strcmplower(schema_type, get_token_string_by_id(XPL_REQUEST_ID)) == 0)
   {
      char *request = xPL_getNamedValue(ListNomsValeursPtr, get_token_string_by_id(XPL_REQUEST_ID));
      if(!request)
      {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message no request\n",INFO_STR,__func__);
         return -1;
      }
      if(mea_strcmplower(request,get_token_string_by_id(XPL_CURRENT_ID))!=0)
      {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message request!=current\n",INFO_STR,__func__);
         return -1;
      }

      return interface_type_005_xPL_sensor(i005, theService, theMessage, device, type, elem);
   }
   
   return 0;
}


int clean_queues(mea_queue_t *q)
{
   struct type005_device_queue_elem_s *d;
   struct type005_module_queue_elem_s *m;
   struct type005_sensor_actuator_queue_elem_s *sa;
  
   mea_queue_first(q);
   while(mea_queue_current(q,(void **)&d)==0)
   {
      mea_queue_first(&(d->modules_list));
      while(mea_queue_current(&(d->modules_list),(void **)&m)==0)
      {
         mea_queue_first(&(m->sensors_actuators_list));
         while(mea_queue_current(&(m->sensors_actuators_list),(void **)&sa)==0)
         {
            free(sa);
            sa=NULL;
            mea_queue_remove_current(&(m->sensors_actuators_list));
         }
         free(m);
         m=NULL;
         mea_queue_remove_current(&(d->modules_list));
      }
      free(d);
      d=NULL;
      mea_queue_remove_current(q);
   }
   return 0;
}


int load_interface_type_005(interface_type_005_t *i005, sqlite3 *db)
{
   sqlite3_stmt * stmt = NULL;
   char sql_request[255];
   parsed_parameters_t *netatmo_sa_params=NULL;
   int nb_netatmo_sa_params=0;
   int nerr=0;
   int ret;

   if(mea_queue_nb_elem(&(i005->devices_list))!=0)
   {
      fprintf(stderr,"ICI\n");
      clean_queues(&(i005->devices_list));
   }

   sprintf(sql_request,"SELECT * FROM sensors_actuators WHERE id_interface=%d and sensors_actuators.state='1'", i005->id_interface);

   ret = sqlite3_prepare_v2(db,sql_request,strlen(sql_request)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      goto load_interface_type_005_clean_exit;
   }

   struct type005_device_queue_elem_s *d_elem=NULL;
   struct type005_module_queue_elem_s *m_elem=NULL;
   struct type005_sensor_actuator_queue_elem_s *sa_elem=NULL;
   
   // récupération des parametrages des capteurs dans la base
   while (1) // boucle de traitement du résultat de la requete
   {
      int new_d_elem_flag=0;
      int new_m_elem_flag=0;
      int s=sqlite3_step(stmt);
      if (s==SQLITE_ROW)
      {
         // les valeurs dont on a besoin
         int id_sensor_actuator=sqlite3_column_int(stmt, 1);
         int id_type=sqlite3_column_int(stmt, 2);
         const unsigned char *name=sqlite3_column_text(stmt, 4);
         const unsigned char *parameters=sqlite3_column_text(stmt, 7);
         int todbflag=sqlite3_column_int(stmt, 9);

         netatmo_sa_params=alloc_parsed_parameters((char *)parameters, valid_netatmo_sa_params, &nb_netatmo_sa_params, &nerr, 0); // les deux parametres sont obligatoires
         if(netatmo_sa_params &&
            netatmo_sa_params->parameters[PARAMS_DEVICE_ID].value.s &&
            netatmo_sa_params->parameters[PARAMS_MODULE_ID].value.s)
         {
            char *device_id=netatmo_sa_params->parameters[PARAMS_DEVICE_ID].value.s;
            char *module_id=netatmo_sa_params->parameters[PARAMS_MODULE_ID].value.s;

            mea_queue_first(&(i005->devices_list));
            for(int i=0; i<i005->devices_list.nb_elem; i++) // on commence par chercher si le device existe déjà
            {
               mea_queue_current(&(i005->devices_list), (void **)&d_elem);
               if(strcmp(d_elem->device_id, device_id)==0)
                  break;
               else
                  d_elem=NULL;
               mea_queue_next(&(i005->devices_list));
            }
            if(d_elem==NULL) // il n'existe pas, on va le créer
            {
               //fprintf(stderr,"creation device\n");
               d_elem=(struct type005_device_queue_elem_s *)malloc(sizeof(struct type005_device_queue_elem_s));
               if(d_elem==NULL)
               {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                     perror("");
                  }
                  release_parsed_parameters(&netatmo_sa_params);
                  continue; // on à pas encore fait d'autre malloc on boucle direct
               }
               strncpy(d_elem->device_id,device_id, sizeof(d_elem->device_id)-1);
               mea_queue_init(&(d_elem->modules_list));
               new_d_elem_flag=1; // on signale qu'on a créé un device
            }
 
            mea_queue_first(&(d_elem->modules_list));
            for(int i=0; i<d_elem->modules_list.nb_elem; i++) // on cherche le module est déjà associé au device
            {
               mea_queue_current(&(d_elem->modules_list), (void **)&m_elem);
               if(strcmp(m_elem->module_id, module_id)==0)
                  break;
               else
                  m_elem=NULL;
               mea_queue_next(&(d_elem->modules_list));
            }
            if(m_elem==NULL) // pas encore associé on le créé
            {
               //fprintf(stderr,"creation module\n");
               m_elem=(struct type005_module_queue_elem_s *)malloc(sizeof(struct type005_module_queue_elem_s));
               if(m_elem==NULL)
               {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                     perror("");
                  }
                  goto load_interface_type_005_clean_queues; // cette fois on a peut-être déjà fait un malloc, on branche sur la section de clean
               }
               strncpy(m_elem->module_id, module_id, sizeof(m_elem->module_id)-1);
               mea_queue_init(&(m_elem->sensors_actuators_list));
              
               // initialisation des données associées au module 
               m_elem->d1.battery_vp=-1;
               m_elem->d1.setpoint=-1;
               m_elem->d1.setpoint_temp=0.0;
               m_elem->d1.temperature=0.0;
               m_elem->d1.therm_relay_cmd=-1;

               m_elem->d2.battery_vp=-1;
               m_elem->d2.setpoint=-1;
               m_elem->d2.setpoint_temp=0.0;
               m_elem->d2.temperature=0.0;
               m_elem->d2.therm_relay_cmd=-1;
            
               m_elem->last=&(m_elem->d1);
               m_elem->current=&(m_elem->d2);

               m_elem->device=d_elem;

               new_m_elem_flag=1; // on signale qu'on a créé un module
            }

            //fprintf(stderr,"creation sensor/actuator\n");
            sa_elem=(struct type005_sensor_actuator_queue_elem_s *)malloc(sizeof(struct type005_sensor_actuator_queue_elem_s));
            if(sa_elem==NULL)
            {
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                  perror("");
               }
               goto load_interface_type_005_clean_queues;
            }
   
            char *sensor=netatmo_sa_params->parameters[PARAMS_SENSOR].value.s;
            char *actuator=netatmo_sa_params->parameters[PARAMS_ACTUATOR].value.s;

            if(sensor && actuator)
            {
               VERBOSE(2) mea_log_printf("%s (%s) : parameter errors - SENSOR and ACUTATOR are incompatible\n", ERROR_STR,__func__,sensor);
               goto load_interface_type_005_clean_queues;
            }

            sa_elem->sensor=-1;
            sa_elem->actuator=-1;
            if(id_type==500)
            {
               if(sensor)
               {
                  mea_strtolower(sensor);
                  if(strcmp(sensor, "temperature")==0)
                     sa_elem->sensor=TEMPERATURE;
                  else if(strcmp(sensor, "relay_cmd")==0)
                     sa_elem->sensor=RELAY_CMD;
                  else if(strcmp(sensor, "battery")==0)
                     sa_elem->sensor=BATTERY;
                  else if(strcmp(sensor, "setpoint")==0)
                     sa_elem->sensor=SETPOINT;
                  else if(strcmp(sensor, "mode")==0)
                     sa_elem->sensor=MODE;
                  else
                  {
                     VERBOSE(2) mea_log_printf("%s (%s) : %s, incorrect sensor type error - %s\n", ERROR_STR, __func__, name, sensor);
                     goto load_interface_type_005_clean_queues;
                  }
               }
               else
               {
                  VERBOSE(2) mea_log_printf("%s (%s) : %s, INPUT type need SENSOR parameter - %s\n", ERROR_STR, __func__, name, sensor);
                  goto load_interface_type_005_clean_queues;
               }
            }
            else if(id_type==501)
            {
               if(actuator)
               {
                  mea_strtolower(actuator);
                  if(strcmp(actuator, "setpoint")==0)
                     sa_elem->actuator=1;
                  else
                  {
                     VERBOSE(2) mea_log_printf("%s (%s) : %s, incorrect actuator type error - %s\n", ERROR_STR, __func__, name, actuator);
                     goto load_interface_type_005_clean_queues;
                  }
               }
               else
               {
                  VERBOSE(2) mea_log_printf("%s (%s) : %s, OUTPUT type need ACTUATOR parameter - %s\n", ERROR_STR, __func__, name, sensor);
                  goto load_interface_type_005_clean_queues;
               }
            }
            else
            {
               VERBOSE(2) mea_log_printf("%s (%s) : %s, configuration error - incorrect (INPUT ou OUPUT only allowed) type\n", ERROR_STR,__func__, name);
               goto load_interface_type_005_clean_queues;
            }

            sa_elem->id=id_sensor_actuator;
            sa_elem->type=id_type;
            strncpy(sa_elem->name, (const char *)name, sizeof(sa_elem->name)-1);
            mea_strtolower(sa_elem->name);
            sa_elem->todbflag=todbflag;
            sa_elem->module=m_elem;

            mea_queue_in_elem(&(m_elem->sensors_actuators_list), (void *)sa_elem);
            if(new_m_elem_flag==1)
            {
               mea_queue_in_elem(&(d_elem->modules_list), (void *)m_elem);
            }
            if(new_d_elem_flag==1)
            {
               mea_queue_in_elem(&(i005->devices_list), (void *)d_elem);
            }

            release_parsed_parameters(&netatmo_sa_params);
            continue;

load_interface_type_005_clean_queues:
            if(netatmo_sa_params)
               release_parsed_parameters(&netatmo_sa_params);
            if(sa_elem)
            {
               free(sa_elem);
               sa_elem=NULL;
            }
            if(new_d_elem_flag==1)
            {
               if(d_elem)
               {
                  free(d_elem);
                  d_elem=NULL;
               }
            }
            if(new_m_elem_flag==1)
            {
               if(m_elem)
               {
                  free(m_elem);
                  m_elem=NULL;
               }
            }
         }
         else
         {
            VERBOSE(2) mea_log_printf("%s (%s) : %s, configuration error - DEVICE_ID and MODULE_ID are mandatory\n", ERROR_STR, __func__, name);
         }
      }
      else if (s==SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         // traitement d'erreur à faire ici
         sqlite3_finalize(stmt);
         goto load_interface_type_005_clean_exit;
      }
   }
   return 0;

load_interface_type_005_clean_exit:
   clean_queues(&(i005->devices_list));
   return -1;
}


void set_interface_type_005_isnt_running(void *data)
{
   interface_type_005_t *i005 = (interface_type_005_t *)data;
   i005->thread_is_running=0;
}


void *_thread_interface_type_005(void *thread_data)
{
   struct thread_interface_type_005_args_s *args = (struct thread_interface_type_005_args_s *)thread_data;
   
   interface_type_005_t *i005=args->i005;
  
   // récupérer ici les données nécessaires
 
   free(thread_data);
   
   pthread_cleanup_push( (void *)set_interface_type_005_isnt_running, (void *)i005 );
   i005->thread_is_running=1;

   char *client_id="563e5ce3cce37c07407522f2";
   char *client_secret="lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4";

   int auth_flag=0;
   mea_timer_t refresh_timer;
   mea_timer_t getdata_timer;
   char err[80];
   int ret;

   mea_init_timer(&refresh_timer,0,0);
   mea_init_timer(&getdata_timer,60,1);
   mea_start_timer(&getdata_timer); 

   while(1)
   {
      pthread_testcancel();
     
      process_heartbeat(i005->monitoring_id);
      process_update_indicator(i005->monitoring_id, I005_XPLIN,  i005->indicators.xplin);
      process_update_indicator(i005->monitoring_id, I005_XPLOUT, i005->indicators.xplout);
      if(auth_flag==0)
      {
         ret=netatmo_get_token(client_id, client_secret, i005->user, i005->password, "read_thermostat write_thermostat", &(i005->token), err, sizeof(err)-1);
         if(ret!=0)
         {
            fprintf(MEA_STDERR, "Authentification error : ");
            if(ret<0)
               fprintf(MEA_STDERR, "%d\n", ret);
            else
               fprintf(MEA_STDERR, "%s (%d)\n", err, ret);
         }
         else
         {
            fprintf(MEA_STDERR, "Authentification done : %s\n", i005->token.access);
          
            auth_flag=1;
            mea_init_timer(&refresh_timer,(int)((float)i005->token.expire_in*0.95), 0);
            mea_start_timer(&refresh_timer);
         }
      }
      if(mea_test_timer(&refresh_timer)==0)
      {
         ret=netatmo_refresh_token(client_id, client_secret, &(i005->token), err, sizeof(err)-1);
         if(ret!=0)
         {
            fprintf(MEA_STDERR, "Authentification error : ");
            auth_flag=0;
            continue;
         }
         else
         {
            mea_init_timer(&refresh_timer,(int)((float)i005->token.expire_in*0.95), 0);
            mea_start_timer(&refresh_timer);
         }
      }

      if(auth_flag!=0 && mea_test_timer(&getdata_timer)==0)
      {
         struct type005_device_queue_elem_s *d_elem;
         mea_queue_first(&(i005->devices_list));
         for(int i=0;i<i005->devices_list.nb_elem;i++)
         {
            mea_queue_current(&(i005->devices_list), (void **)&d_elem);
            
            struct type005_module_queue_elem_s *m_elem;
            mea_queue_first(&(d_elem->modules_list));
            for(int j=0;j<d_elem->modules_list.nb_elem;j++)
            {
               mea_queue_current(&(d_elem->modules_list), (void **)&m_elem);
              

               struct netatmo_thermostat_data_s data;
               ret=netatmo_get_thermostat_data(i005->token.access, d_elem->device_id, m_elem->module_id, &data, err, sizeof(err)-1);
               if(ret==0 && m_elem->last->therm_relay_cmd!=-1)
               {
                  struct netatmo_thermostat_data_s *tmp;
               // switch des espaces de données last et current 
                  tmp=m_elem->last;
                  m_elem->last=m_elem->current;
                  m_elem->current=tmp;
                  memcpy(m_elem->current, &data, sizeof(data));
                  
                  struct type005_sensor_actuator_queue_elem_s *sa_elem;

                  mea_queue_first(&(m_elem->sensors_actuators_list));
                  for(int k=0;k<m_elem->sensors_actuators_list.nb_elem;k++)
                  {
                     mea_queue_current(&(m_elem->sensors_actuators_list), (void **)&sa_elem);
                     if(sa_elem->type==500)
                     {
                        char str_value[20], str_last[20], type[20];
                        double data4db=0.0;
                        double data4db2=0.0;
                        char data4dbComp[20]="";
                        
                        ret=interface_type_005_current_last(sa_elem, type, str_value, str_last, &data4db, &data4db2, data4dbComp);
                        if(ret==1)
                        {
                           _interface_type_005_send_xPL_sensor_basic(i005, xPL_MESSAGE_TRIGGER, sa_elem->name, type, str_value, str_last);
                           if(sa_elem->todbflag==1)
                              dbServer_add_data_to_sensors_values(sa_elem->id, data4db, 0, data4db2, data4dbComp);
                        } 
                        mea_queue_next(&(m_elem->sensors_actuators_list));
                     }
                  }
               }
               else
               {
                  if(ret!=0)
                     DEBUG_SECTION mea_log_printf("%s (%s) : can't get data (%d: %s)\n", ERROR_STR, __func__, ret, err);
               }
            }
            mea_queue_next(&(d_elem->modules_list));
         }
         mea_queue_next(&(i005->devices_list));
      }

      sleep(5);
   }
   
_thread_interface_type_005_clean_exit:
   pthread_cleanup_pop(1);

   pthread_exit(NULL);
}


interface_type_005_t *malloc_and_init_interface_type_005(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description)
{
   interface_type_005_t *i005=NULL;
   
   i005=(interface_type_005_t *)malloc(sizeof(interface_type_005_t));
   if(!i005)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   i005->thread_is_running=0;
   
   struct interface_type_005_start_stop_params_s *i005_start_stop_params=(struct interface_type_005_start_stop_params_s *)malloc(sizeof(struct interface_type_005_start_stop_params_s));
   if(!i005_start_stop_params)
   {
      free(i005);
      i005=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   i005->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(i005->parameters==NULL)
   {
      free(i005);
      i005=NULL;
      free(i005_start_stop_params);
      i005_start_stop_params=NULL;

      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   strcpy(i005->parameters,(char *)parameters);
   strncpy(i005->name,(char *)name,sizeof(i005->name)-1);
   strncpy(i005->dev,(char *)dev,sizeof(i005->dev)-1);
   i005->id_interface=id_interface;
   mea_queue_init(&(i005->devices_list));

   i005->indicators.xplin=0;
   i005->indicators.xplout=0;
   i005->xPL_callback=NULL;
   i005->thread=NULL;
   
   i005->monitoring_id=process_register((char *)name);
   i005_start_stop_params->sqlite3_param_db = sqlite3_param_db;
   i005_start_stop_params->i005=i005;
   
   process_set_group(i005->monitoring_id, 1);
   process_set_start_stop(i005->monitoring_id, start_interface_type_005, stop_interface_type_005, (void *)i005_start_stop_params, 1);
   process_set_watchdog_recovery(i005->monitoring_id, restart_interface_type_005, (void *)i005_start_stop_params);
   process_set_description(i005->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i005->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat
   
   process_add_indicator(i005->monitoring_id, interface_type_005_xplout_str, 0);
   process_add_indicator(i005->monitoring_id, interface_type_005_xplin_str, 0);
   
   return i005;
}


int clean_interface_type_005(interface_type_005_t *i005)
{
   clean_queues(&(i005->devices_list));

   if(i005->parameters)
   {
      free(i005->parameters);
      i005->parameters=NULL;
   }
   
   i005->xPL_callback=NULL;
   
   if(i005->thread)
   {
      free(i005->thread);
      i005->thread=NULL;
   }

   return 0;
}


int stop_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;
   
   struct interface_type_005_start_stop_params_s *start_stop_params=(struct interface_type_005_start_stop_params_s *)data;
   
   VERBOSE(1) mea_log_printf("%s  (%s) : %s shutdown thread ...\n", INFO_STR, __func__, start_stop_params->i005->name);
   
   if(start_stop_params->i005->xPL_callback)
   {
      start_stop_params->i005->xPL_callback=NULL;
   }
   
   if(start_stop_params->i005->thread)
   {
      pthread_cancel(*(start_stop_params->i005->thread));
      
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(start_stop_params->i005->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération\n", DEBUG_STR, __func__, start_stop_params->i005->name, 100-counter);

      clean_interface_type_005(start_stop_params->i005);
   }
   
   mea_notify_printf('S', "%s %s", start_stop_params->i005->name, stopped_successfully_str);
   
   return 0;
}


int restart_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int16_t check_status_interface_type_005(interface_type_005_t *it005)
/**
 * \brief     indique si une anomalie a généré l'emission d'un signal SIGHUP
 * \param     i005           descripteur de l'interface
 * \return    toujours 0, pas d'emission de signal
 **/
{
   return 0;
}


int start_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg)
{
   pthread_t *interface_type_005_thread_id=NULL; // descripteur du thread
   
   struct interface_type_005_start_stop_params_s *start_stop_params=(struct interface_type_005_start_stop_params_s *)data; // données pour la gestion des arrêts/relances
   struct thread_interface_type_005_args_s *interface_type_005_thread_args=NULL; // parametre à transmettre au thread

   start_stop_params->i005->xPL_callback=NULL;

   if(load_interface_type_005(start_stop_params->i005, start_stop_params->sqlite3_param_db)<0)
      return -1;
 
   interface_type_005_thread_args=malloc(sizeof(struct thread_interface_type_005_args_s));
   if(!interface_type_005_thread_args)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__);
      perror("");
      goto start_interface_type_005_clean_exit;
   }
   interface_type_005_thread_args->i005=start_stop_params->i005;

   // initialisation et préparation des données
   start_stop_params->i005->user[0]=0;
   start_stop_params->i005->password[0]=0;
   sscanf(start_stop_params->i005->dev, "NETATMO://%80[^/]/%80[^\n]", start_stop_params->i005->user, start_stop_params->i005->password);
   if(start_stop_params->i005->user[0]==0 || start_stop_params->i005->password[0]==0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : bad netatmo dev - incorrect user/password%s\n", ERROR_STR, __func__, start_stop_params->i005->dev);
      goto start_interface_type_005_clean_exit;
   }
   
   interface_type_005_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!interface_type_005_thread_id)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__);
      goto start_interface_type_005_clean_exit;
   }
   
   if(pthread_create (interface_type_005_thread_id, NULL, _thread_interface_type_005, (void *)interface_type_005_thread_args))
   {
      VERBOSE(2) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      goto start_interface_type_005_clean_exit;
   }

   start_stop_params->i005->xPL_callback=interface_type_005_xPL_callback;
   start_stop_params->i005->thread=interface_type_005_thread_id;
   
   pthread_detach(*interface_type_005_thread_id);
   
   VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i005->name, launched_successfully_str);
   mea_notify_printf('S', "%s %s", start_stop_params->i005->name, launched_successfully_str);
   
   return 0;
   
start_interface_type_005_clean_exit:
   if(interface_type_005_thread_id)
   {
      free(interface_type_005_thread_id);
      interface_type_005_thread_id=NULL;
   }
   if(interface_type_005_thread_args)
   {
      free(interface_type_005_thread_args);
      interface_type_005_thread_args=NULL;
   }
   
   return -1;
}
