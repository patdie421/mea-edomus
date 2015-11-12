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

#include "globals.h"
#include "consts.h"

#include "tokens.h"
#include "mea_verbose.h"
#include "mea_timer.h"
#include "mea_string_utils.h"
#include "mea_queue.h"

#include "macros.h"
#include "dbServer.h"
#include "parameters_utils.h"
#include "processManager.h"
#include "notify.h"
#include "netatmo.h"
#include "cJSON.h"
#include "uthash.h"

#include "interfacesServer.h"


struct type005_device_queue_elem_s
{
   char device_id[81];
   mea_queue_t modules_list;
};


struct type005_module_queue_elem_s
{
   char module_id[81];
   mea_queue_t sensors_actuators_list;
   
   struct netatmo_thermostat_data_s d1, d2;
   struct netatmo_thermostat_data_s *current, *last;
};


struct type005_sensor_actuator_queue_elem_s
{
   int id;
   int type;
   char name[41];
   int todbflag;
   int sensor;
   int actuator;
};


char *interface_type_005_xplin_str="XPLIN";
char *interface_type_005_xplout_str="XPLOUT";


int16_t interface_type_005_xPL_actuator(interface_type_005_t *i005, xPL_ServicePtr theService, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type)
{
   int16_t ret=-1;
   
   return ret;
}


int16_t interface_type_005_xPL_sensor(interface_type_005_t *i005, xPL_ServicePtr theService, xPL_MessagePtr msg, char *device, char *type)
{
   int16_t ret=-1;
   
   return ret;
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
   
   (i005->indicators.xplin)++;
   
   VERBOSE(9) mea_log_printf("%s  (%s) : xPL Message to process : %s.%s\n",INFO_STR,__func__,schema_class,schema_type);
   
   if(mea_strcmplower(schema_class, get_token_string_by_id(XPL_CONTROL_ID)) == 0 &&
      mea_strcmplower(schema_type, get_token_string_by_id(XPL_BASIC_ID)) == 0)
   {
      if(!device)
      {
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message no device\n",INFO_STR,__func__);
         return -1;
      }
      if(!type)
      {
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message no type\n",INFO_STR,__func__);
         return -1;
      }
      return interface_type_005_xPL_actuator(i005, theService, ListNomsValeursPtr, device, type);
   }
   else if(mea_strcmplower(schema_class, get_token_string_by_id(XPL_SENSOR_ID)) == 0 &&
           mea_strcmplower(schema_type, get_token_string_by_id(XPL_REQUEST_ID)) == 0)
   {
      char *request = xPL_getNamedValue(ListNomsValeursPtr, get_token_string_by_id(XPL_REQUEST_ID));
      if(!request)
      {
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message no request\n",INFO_STR,__func__);
         return -1;
      }
      if(mea_strcmplower(request,get_token_string_by_id(XPL_CURRENT_ID))!=0)
      {
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message request!=current\n",INFO_STR,__func__);
         return -1;
      }
      return interface_type_005_xPL_sensor(i005, theService, theMessage, device, type);
   }
   
   return 0;
}


int clean_queues(mea_queue_t *q)
{
   struct type005_device_queue_elem_s *d;
   struct type005_module_queue_elem_s *m;
   struct type005_sensor_actuator_queue_elem_s *sa;
   
   mea_queue_first(q);
   while(mea_queue_current(q,(void **)&d))
   {
      mea_queue_first(&(d->modules_list));
      while(mea_queue_current(&(d->modules_list),(void **)&m))
      {
         mea_queue_first(&(m->sensors_actuators_list));
         while(mea_queue_current(&(m->sensors_actuators_list),(void **)&sa))
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


char *valid_netatmo_sa_params[]={"S:DEVICE_ID","S:MODULE_ID", "S:SENSOR", "S:ACTUATOR", NULL};
#define PARAMS_DEVICE_ID 0
#define PARAMS_MODULE_ID 1
#define PARAMS_SENSOR    2
#define PARAMS_ACTUATOR  3

enum netatmo_sensor_e { TEMPERATURE, RELAY_CMD, BATTERY, SETPOINT, MODE };

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
   
   int new_d_elem_flag=0;
   int new_m_elem_flag=0;

   // récupération des parametrages des capteurs dans la base
   while (1) // boucle de traitement du résultat de la requete
   {
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

            //mea_queue_t sensors_actuators_list;
            
            mea_queue_first(&(i005->devices_list));
            for(int i=0; i<i005->devices_list.nb_elem; i++)
            {
               mea_queue_current(&(i005->devices_list), (void **)&d_elem);
               if(strcmp(d_elem->device_id, device_id)==0)
                  break;
               else
                  d_elem=NULL;
            }
            if(d_elem==NULL)
            {
               d_elem=(struct type005_device_queue_elem_s *)malloc(sizeof(struct type005_device_queue_elem_s));
               if(d_elem==NULL)
               {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                     perror("");
                  }
                  release_parsed_parameters(&netatmo_sa_params);
                  continue; // on passe au suivant
               }
               strncpy(d_elem->device_id,device_id, sizeof(d_elem->device_id)-1);
               mea_queue_init(&(d_elem->modules_list));
               new_d_elem_flag=1;
            }
 
            mea_queue_first(&(d_elem->modules_list));
            for(int i=0; i<d_elem->modules_list.nb_elem; i++)
            {
               mea_queue_current(&(d_elem->modules_list), (void **)&m_elem);
               if(strcmp(m_elem->module_id, module_id)==0)
                  break;
               else
                  m_elem=NULL;
            }
            if(m_elem==NULL)
            {
               m_elem=(struct type005_module_queue_elem_s *)malloc(sizeof(struct type005_module_queue_elem_s));
               if(m_elem==NULL)
               {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                     perror("");
                  }
                  release_parsed_parameters(&netatmo_sa_params);
                  continue; // on passe au suivant
               }
               strncpy(m_elem->module_id, module_id, sizeof(m_elem->module_id)-1);
               mea_queue_init(&(m_elem->sensors_actuators_list));
               
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

               new_m_elem_flag=1;
            }

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

            sa_elem->sensor=-1;
            sa_elem->actuator=-1;
            if(id_type==500)
            {
               if(sensor && actuator==NULL)
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
                     VERBOSE(2) mea_log_printf("%s (%s) : unknown sensor error - %s\n", ERROR_STR, __func__, sensor);
                     goto load_interface_type_005_clean_queues;
                  }
               }
               else
               {
                  VERBOSE(2) mea_log_printf("%s (%s) : sensor parameter error - SENSOR and ACUTATOR are incompatible\n", ERROR_STR,__func__,sensor);
                  goto load_interface_type_005_clean_queues;
               }
            }
            else if(id_type==501)
            {
               if(sensor==NULL && actuator)
               {
                  sa_elem->actuator=1;
               }
               else
               {
                  VERBOSE(2) mea_log_printf("%s (%s) : configuration error - SENSOR and ACUTATOR are incompatible\n", ERROR_STR,__func__);
                  goto load_interface_type_005_clean_queues;
               }
            }
            else
            {
               VERBOSE(2) mea_log_printf("%s (%s) : configuration error - unknown type\n", ERROR_STR,__func__);
               goto load_interface_type_005_clean_queues;
            }

            sa_elem->id=id_sensor_actuator;
            sa_elem->type=id_type;
            strncpy(sa_elem->name, (const char *)name, sizeof(sa_elem->name)-1);
            mea_strtolower(sa_elem->name);
            sa_elem->todbflag=todbflag;

            mea_queue_in_elem(&(m_elem->sensors_actuators_list), (void *)sa_elem);
            if(new_m_elem_flag==1)
               mea_queue_in_elem(&(d_elem->modules_list), (void *)m_elem);
            if(new_d_elem_flag==1)
               mea_queue_in_elem(&(i005->devices_list), (void *)d_elem);

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
            // erreur de parametre 
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
   // faire le ménage ici
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

   // faire ici les initialisation
   char *client_id="563e5ce3cce37c07407522f2";
   char *client_secret="lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4";
//   char *user_name="patrice.dietsch@gmail.com";
//   char *password="WEBcdpii10";

   int auth_flag=0;
   mea_timer_t refresh_timer;
   mea_timer_t getdata_timer;
   char err[80];
   struct netatmo_token_s token;
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
         ret=netatmo_get_token(client_id, client_secret, i005->user, i005->password, "read_thermostat write_thermostat", &token, err, sizeof(err)-1);
         if(ret!=0)
         {
            fprintf(MEA_STDERR, "Authentification error : ");
            if(ret<0)
               fprintf(MEA_STDERR, "%d\n", ret);
            else
               fprintf(MEA_STDERR, "%s (%d)\n", err, ret);
            goto _thread_interface_type_005_clean_exit;
         }
         fprintf(MEA_STDERR, "Authentification done : %s\n", token.access);
          
         auth_flag=1;
         mea_init_timer(&refresh_timer,(int)((float)token.expire_in*0.95), 0);
         mea_start_timer(&refresh_timer);
      }
      if(mea_test_timer(&refresh_timer)==0)
      {
         ret=netatmo_refresh_token(client_id, client_secret, &token, err, sizeof(err)-1);
         if(ret!=0)
         {
            fprintf(MEA_STDERR, "Authentification error : ");
            auth_flag=0;
            continue;
         }
         else
         {
            mea_init_timer(&refresh_timer,(int)((float)token.expire_in*0.95), 0);
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
               
               struct netatmo_thermostat_data_s *tmp;
               
               tmp=m_elem->last;
               m_elem->last=m_elem->current;
               m_elem->current=tmp;
               ret=netatmo_get_thermostat_data(token.access, d_elem->device_id, m_elem->module_id, m_elem->current, err, sizeof(err)-1);
               if(ret==0)
               {
                  
                  struct type005_sensor_actuator_queue_elem_s *sa_elem;
                  mea_queue_first(&(m_elem->sensors_actuators_list));
                  for(int k=0;k<d_elem->modules_list.nb_elem;k++)
                  {
                     mea_queue_current(&(m_elem->sensors_actuators_list), (void **)&sa_elem);
                     if(sa_elem->type==500)
                     {
                        switch(sa_elem->sensor)
                        {
                           case TEMPERATURE:
                              if(m_elem->last->temperature != m_elem->current->temperature)
                              {
                                 fprintf(MEA_STDERR, "[%s] temperature = %f, mode=%d (current)\n", m_elem->module_id, m_elem->current->temperature, m_elem->current->setpoint);
                                 fprintf(MEA_STDERR, "[%s] temperature = %f, mode=%d (last)\n", m_elem->module_id, m_elem->last->temperature, m_elem->last->setpoint);
                              }
                              else
                              {
                                 fprintf(MEA_STDERR, "No change\n");
                              }
                              break;
                           default:
                              break;
                        }
                     }
                  }
               }
               else
               {
                  fprintf(MEA_STDERR, "error : %s (%d)\n", err, ret);
               }
            }
         }
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
   
   VERBOSE(1) mea_log_printf("%s  (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i005->name);
   
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

//      free(start_stop_params->i005->thread);
//      start_stop_params->i005->thread=NULL;
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
//   int16_t ret;
   
   pthread_t *interface_type_005_thread_id=NULL; // descripteur du thread
   
   struct interface_type_005_start_stop_params_s *start_stop_params=(struct interface_type_005_start_stop_params_s *)data; // données pour la gestion des arrêts/relances
   struct thread_interface_type_005_args_s *interface_type_005_thread_args=NULL; // parametre à transmettre au thread

   load_interface_type_005(start_stop_params->i005, start_stop_params->sqlite3_param_db);
 
   interface_type_005_thread_args=malloc(sizeof(struct thread_interface_type_005_args_s));
   if(!interface_type_005_thread_args)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__);
      perror("");
      goto start_interface_type_005_clean_exit;
   }
   interface_type_005_thread_args->i005=start_stop_params->i005;
   start_stop_params->i005->user[0]=0;
   start_stop_params->i005->password[0]=0;
   sscanf(start_stop_params->i005->dev, "NETATMO://%80[^/]/%80[^\n]", start_stop_params->i005->user, start_stop_params->i005->password);
   
   // tester ici la validité de l'adresse (checkmacaddrformat)
   if(start_stop_params->i005->user[0]==0 || start_stop_params->i005->password[0]==0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : bad netatmo dev - %s\n", ERROR_STR, __func__, start_stop_params->i005->dev);
      goto start_interface_type_005_clean_exit;
   }
   start_stop_params->i005->xPL_callback=interface_type_005_xPL_callback;
   
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
   
   interface_type_005_thread_args->i005->thread=interface_type_005_thread_id;
   
   pthread_detach(*interface_type_005_thread_id);
   
   VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i005->name, launched_successfully_str);
   mea_notify_printf('S', "%s %s", start_stop_params->i005->name, launched_successfully_str);
   
   return 0;
   
start_interface_type_005_clean_exit:
   if(interface_type_005_thread_args)
   {
      free(interface_type_005_thread_args);
      interface_type_005_thread_args=NULL;
   }
   if(interface_type_005_thread_id)
   {
      free(interface_type_005_thread_id);
      interface_type_005_thread_id=NULL;
   }
   
   return -1;
}
