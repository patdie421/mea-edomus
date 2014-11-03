/*
 *  interface_type_001.c
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sqlite3.h>
#include <termios.h>

#include "globals.h"
#include "consts.h"

#include "string_utils.h"
#include "tokens.h"
#include "debug.h"
#include "memory.h"
#include "macros.h"
#include "error.h"

//#include "comio.h"
#include "comio2.h"

#include "arduino_pins.h"
#include "parameters_utils.h"

#include "dbServer.h"
#include "xPLServer.h"

#include "processManager.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_001_sensors.h"
#include "interface_type_001_actuators.h"
#include "interface_type_001_counters.h"

#include "notify.h"

#define TEMPO    5 // 5 secondes 2 read


// structure contenant les parametres du thread de gestion des capteurs
struct interface_type_001_thread_params_s
{
   interface_type_001_t *it001;
//   tomysqldb_md_t *md;
};


void set_interface_type_001_isnt_running(void *data)
{
   interface_type_001_t *i001 = (interface_type_001_t *)data;
   i001->thread_is_running=0;
}


// xPLSend -c control.basic -m cmnd device=RELAY1 type=output current=pulse data1=125
// xPLSend -c sensor.request -m cmnd request=current device=CONSO type=POWER => dernière puissance instantannée
// xPLSend -c sensor.request -m cmnd request=current device=CONSO type=ENERGY => valeur du compteur ERDF
//
int16_t interface_type_001_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   xPL_NameValueListPtr ListNomsValeursPtr ;
   char *schema_type, *schema_class, *device, *type;
   interface_type_001_t *i001=(interface_type_001_t *)userValue;
   schema_class       = xPL_getSchemaClass(theMessage);
   schema_type        = xPL_getSchemaType(theMessage);
   ListNomsValeursPtr = xPL_getMessageBody(theMessage);
   device             = xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_DEVICE_ID));
   type               = xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_TYPE_ID));
   
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : xPL Message to process : %s.%s\n",INFO_STR,__func__,schema_class,schema_type);

   if(mea_strcmplower(schema_class, get_token_by_id(XPL_CONTROL_ID)) == 0 &&
      mea_strcmplower(schema_type, get_token_by_id(XPL_BASIC_ID)) == 0)
   {
      if(!device)
      {
         VERBOSE(5) fprintf(stderr,"%s  (%s) : xPL message no device\n",INFO_STR,__func__);
         return 0;
      }
      if(!type)
      {
         VERBOSE(5) fprintf(stderr,"%s  (%s) : xPL message no type\n",INFO_STR,__func__);
         return 0;
      }
      return xpl_actuator(i001, ListNomsValeursPtr, device, type);
   }
   else if(mea_strcmplower(schema_class, get_token_by_id(XPL_SENSOR_ID)) == 0 &&
           mea_strcmplower(schema_type, get_token_by_id(XPL_REQUEST_ID)) == 0)
   {
      char *request = xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_REQUEST_ID));
      if(!request)
      {
         VERBOSE(5) fprintf(stderr,"%s  (%s) : xPL message no request\n",INFO_STR,__func__);
         return 0;
      }
      if(mea_strcmplower(request,get_token_by_id(XPL_CURRENT_ID))!=0)
      {
         VERBOSE(5) fprintf(stderr,"%s  (%s) : xPL message request!=current\n",INFO_STR,__func__);
         return 0;
      }
      
      interface_type_001_counters_process_xpl_msg(i001, theService, theMessage, device, type);
      interface_type_001_sensors_process_xpl_msg(i001, theService, theMessage, device, type);
   }
   
   return 0;
}


int16_t check_status_interface_type_001(interface_type_001_t *i001)
{
   if(i001->ad->signal_flag!=0)
      return -1;
   
   return 0;
}


int load_interface_type_001(interface_type_001_t *i001, int interface_id, sqlite3 *db)
{
   sqlite3_stmt * stmt;
   int16_t nb_sensors_actuators=0;
   char sql_request[255];
   int ret;
   
   // préparation des éléments de contexte de l'interface
   i001->interface_id=interface_id;
   i001->xPL_callback=NULL;
   i001->counters_list=NULL;
   i001->sensors_list=NULL;
   i001->actuators_list=NULL;
   i001->loaded=0;

   // initialisation de la liste des capteurs de type compteur
   i001->counters_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->counters_list)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto load_interface_type_001_clean_exit;
   }
   init_queue(i001->counters_list);
   
   
   // initialisation de la liste des actionneurs (sorties logiques de l'arduino)
   i001->actuators_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->actuators_list)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto load_interface_type_001_clean_exit;
   }
   init_queue(i001->actuators_list);


   // initialisation de la liste des autres capteurs (entrees logiques et analogiques)
   i001->sensors_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->sensors_list)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto load_interface_type_001_clean_exit;
   }
   init_queue(i001->sensors_list);

   
   // préparation de la requete permettant d'obtenir les capteurs associés à l'interface
   sprintf(sql_request,"SELECT * FROM sensors_actuators WHERE id_interface=%d", interface_id);
   ret = sqlite3_prepare_v2(db,sql_request,strlen(sql_request)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      goto load_interface_type_001_clean_exit;
   }
   
   // récupération des parametrages des capteurs dans la base
   while (1) // boucle de traitement du résultat de la requete
   {
      int s=sqlite3_step(stmt);
      if (s==SQLITE_ROW)
      {
         // les valeurs dont ont a besoin
         int id_sensor_actuator=sqlite3_column_int(stmt, 1);
         int id_type=sqlite3_column_int(stmt, 2);
         const unsigned char *name=sqlite3_column_text(stmt, 4);
         const unsigned char *parameters=sqlite3_column_text(stmt, 7);
         
         switch (id_type)
         {
            case 1000: // capteur de type compteur
            {
               struct electricity_counter_s *counter;
               counter=interface_type_001_sensors_valid_and_malloc_counter(id_sensor_actuator, (char *)name, (char *)parameters);
               if(counter)
               {
                  counter->power=0.0;
                  counter->counter=0;
                  counter->last_power=0.0;
                  counter->last_counter=0;
                  in_queue_elem(i001->counters_list, counter);
               }
               nb_sensors_actuators++;
               break;
            }
               
            case 1002: // relais
            {
               struct actuator_s *actuator;
               actuator=valid_and_malloc_actuator(id_sensor_actuator, (char *)name, (char *)parameters);
               if(actuator)
                  in_queue_elem(i001->actuators_list, actuator);
               nb_sensors_actuators++;
               break;
            }
            
            case 1001: // entrées
            {
               struct sensor_s *sensor;
               sensor=interface_type_001_sensors_valid_and_malloc_sensor(id_sensor_actuator, (char *)name, (char *)parameters);
               if(sensor)
                  in_queue_elem(i001->sensors_list, sensor);
               nb_sensors_actuators++;
               break;
            }

            default:
               break;
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
         goto load_interface_type_001_clean_exit;
      }
   }
   i001->loaded=1;
   return nb_sensors_actuators;

load_interface_type_001_clean_exit:
   clean_interface_type_001(i001);
   return -1;
}


int clean_interface_type_001(interface_type_001_t *i001)
{
   if(i001==NULL)
      return 0;

   if(i001->counters_list)
   {
      struct counter_s *counter;
      first_queue(i001->counters_list);
      while(i001->counters_list->nb_elem)
      {
         out_queue_elem(i001->counters_list, (void **)&counter);
         free(counter);
         counter=NULL;
      }
      free(i001->counters_list);
      i001->counters_list=NULL;
   }

   if(i001->sensors_list)
   {
      struct sensor_s *sensor;
      first_queue(i001->sensors_list);
      while(i001->sensors_list->nb_elem)
      {
         out_queue_elem(i001->sensors_list, (void **)&sensor);
         free(sensor);
         sensor=NULL;
      } 
      free(i001->sensors_list);
      i001->sensors_list=NULL;
   }

   if(i001->actuators_list)
   {
      struct actuator_s *actuator;
      first_queue(i001->actuators_list);
      while(i001->actuators_list->nb_elem)
      {
         out_queue_elem(i001->actuators_list, (void **)&actuator);
         free(actuator);
         actuator=NULL;
      }
      free(i001->actuators_list);
      i001->actuators_list=NULL;
   }
   i001->loaded=0;
   
   return 0;
}


void *_thread_interface_type_001(void *args)
{
   struct interface_type_001_thread_params_s *interface_type_001_thread_params=(struct interface_type_001_thread_params_s *)args;
   
   interface_type_001_t *i001=interface_type_001_thread_params->it001;
   
   pthread_cleanup_push( (void *)set_interface_type_001_isnt_running, (void *)i001 );
   i001->thread_is_running=1;
   process_heartbeat(i001->monitoring_id);
   
//   tomysqldb_md_t *md=interface_type_001_thread_params->md;
   free(interface_type_001_thread_params);
   interface_type_001_thread_params=NULL;
   
   interface_type_001_counters_init(i001);
   interface_type_001_sensors_init(i001);

   uint32_t cntr=0;
   while(1)
   {
      process_heartbeat(i001->monitoring_id);

      if(interface_type_001_counters_poll_inputs(i001)<0)
      {
         pthread_exit(NULL);
      }
      
      if(interface_type_001_sensors_poll_inputs(i001)<0)
      {
         pthread_exit(NULL);
      }
      
      cntr++;
      pthread_testcancel();
      sleep(5);
   }
   
   pthread_cleanup_pop(1);
}


int restart_interface_type_001(int id)
{
   process_stop(id, NULL, 0);
   sleep(5);
   return process_start(id, NULL, 0);
}


int stop_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_001_start_stop_params_s *start_stop_params=(struct interface_type_001_start_stop_params_s *)data;

   if(start_stop_params->i001->thread_id)
   {
      pthread_cancel(*start_stop_params->i001->thread_id);
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(start_stop_params->i001->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }
      DEBUG_SECTION fprintf(stderr,"%s (%s) : %s, fin après %d itération\n",DEBUG_STR, __func__,start_stop_params->i001->name,100-counter);

      free(start_stop_params->i001->thread_id);
      start_stop_params->i001->thread_id=NULL;
   }
   
   if(start_stop_params->i001->ad)
   {
      comio2_close(start_stop_params->i001->ad);
      comio2_free_ad(start_stop_params->i001->ad);
      start_stop_params->i001->ad=0;
   }
   VERBOSE(2) fprintf(stderr,"%s  (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i001->name, stopped_successfully_str);
   mea_notify_printf('S', "%s %s.", start_stop_params->i001->name, stopped_successfully_str);

   return 0;
}


int start_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int16_t ret;
   
   char dev[81];
   char buff[81];
   speed_t speed;
   int fd = 0;
   comio2_ad_t *ad=NULL;
   char err_str[81];

   pthread_t *interface_type_001_thread_id=NULL; // descripteur du thread
   
   struct interface_type_001_thread_params_s *interface_type_001_thread_params=NULL; // parametre à transmettre au thread
   struct interface_type_001_start_stop_params_s *start_stop_params=(struct interface_type_001_start_stop_params_s *)data;

   if(start_stop_params->i001->loaded!=1)
   {
      ret=load_interface_type_001(start_stop_params->i001, start_stop_params->i001->interface_id, start_stop_params->sqlite3_param_db);
      if(ret<0)
      {
         VERBOSE(2) fprintf (stderr, "%s (%s) : can not load sensors/actuators.\n", ERROR_STR,__func__);
         mea_notify_printf('E', "%s can't be launched - can't load sensors/actuators.", start_stop_params->i001->name);
         return -1;
      }
   }

   // si on a trouvé une config
   if(start_stop_params->i001->loaded==1)
   {
      ret=get_dev_and_speed((char *)start_stop_params->dev, buff, sizeof(buff), &speed);
      if(!ret)
      {
         int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
         if( n<0 || n==(sizeof(buff)-1) )
         {
            strerror_r(errno, err_str, sizeof(err_str));
            VERBOSE(2) {
               fprintf (stderr, "%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
               perror("");
            }
            mea_notify_printf('E', "%s can't be launched - %s.", start_stop_params->i001->name, err_str);
            goto start_interface_type_001_clean_exit;
         }
      }
      else
      {
         VERBOSE(2) fprintf (stderr, "%s (%s) : unknow interface device - %s\n", ERROR_STR,__func__, start_stop_params->dev);
         mea_notify_printf('E', "%s can't be launched - unknow interface device (%s).", start_stop_params->i001->name, start_stop_params->dev);
         goto start_interface_type_001_clean_exit;
      }

      ad=(comio2_ad_t *)malloc(sizeof(comio2_ad_t));
      if(!ad)
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            fprintf (stderr, "%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
            perror("");
         }
         mea_notify_printf('E', "%s can't be launched - %s.", start_stop_params->i001->name, err_str);
         goto start_interface_type_001_clean_exit;
      }
      
      fd = comio2_init(ad, dev, speed);
      if (fd == -1)
      {
         VERBOSE(2) {
            fprintf(stderr,"%s (%s) : init_arduino - Unable to open serial port (%s) - ",ERROR_STR,__func__,start_stop_params->dev);
            perror("");
         }
         mea_notify_printf('E', "%s can't be launched - unable to open serial port (%s).", start_stop_params->i001->name, start_stop_params->dev);
         fd=0;
         goto start_interface_type_001_clean_exit;
      }
   }
   else
   {
      VERBOSE(5) fprintf(stderr,"%s (%s) : no sensor/actuator active for this interface (%d) - ",ERROR_STR,__func__,start_stop_params->i001->interface_id);
      mea_notify_printf('E', "%s can't be launched - no sensor/actuator active for this interface.", start_stop_params->i001->name);

      goto start_interface_type_001_clean_exit;
   }
   
   start_stop_params->i001->ad=ad;
   
   interface_type_001_thread_params=malloc(sizeof(struct interface_type_001_thread_params_s));
   if(!interface_type_001_thread_params)
      goto start_interface_type_001_clean_exit;
   
   interface_type_001_thread_params->it001=start_stop_params->i001;
//   interface_type_001_thread_params->md=start_stop_params->myd;
   
   interface_type_001_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!interface_type_001_thread_id)
      goto start_interface_type_001_clean_exit;
   
   start_stop_params->i001->xPL_callback=interface_type_001_xPL_callback;

   if(pthread_create (interface_type_001_thread_id, NULL, _thread_interface_type_001, (void *)interface_type_001_thread_params))
   {
      VERBOSE(2) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      if(errmsg)
         snprintf(errmsg, l_errmsg, "internal error (pthread_create)");
      goto start_interface_type_001_clean_exit;
   }
  
   start_stop_params->i001->thread_id=interface_type_001_thread_id;
   
   pthread_detach(*interface_type_001_thread_id);
   
   VERBOSE(2) fprintf(stderr,"%s  (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i001->name, launched_successfully_str);
   mea_notify_printf('S', "%s %s", start_stop_params->i001->name, launched_successfully_str);
   
   return 0;
   
start_interface_type_001_clean_exit:
   if(interface_type_001_thread_params)
   {
      free(interface_type_001_thread_params);
      interface_type_001_thread_params=NULL;
   }
   if(interface_type_001_thread_id)
   {
      free(interface_type_001_thread_id);
      interface_type_001_thread_id=NULL;
   }
   if(fd)
      comio2_close(ad);
   if(ad)
   {
      free(ad);
      ad=NULL;
   }
   
   return -1;
}

