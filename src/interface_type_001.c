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
#include "string_utils.h"
#include "tokens_strings.h"
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

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_001_sensors.h"
#include "interface_type_001_actuators.h"
#include "interface_type_001_counters.h"

#define TEMPO    5 // 5 secondes 2 read


// structure contenant les parametres du thread de gestion des capteurs
struct thread_interface_type_001_params_s
{
   interface_type_001_t *it001;
   tomysqldb_md_t *md;
};


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
      
        // interface_type_001_counters_process_xpl_msg(i001, theService, ListNomsValeursPtr, device, type);
        interface_type_001_counters_process_xpl_msg(i001, theService, theMessage, device, type);
        // interface_type_001_sensors_process_xpl_msg(i001, theService, ListNomsValeursPtr, device, type);
        interface_type_001_sensors_process_xpl_msg(i001, theService, theMessage, device, type);
   }
   
   return 0;
}


mea_error_t stop_interface_type_001(interface_type_001_t *i001)
{

//   comio2_remove_all_traps(i001->ad);

   queue_t *counters_list=i001->counters_list;
   struct electricity_counter_s *counter;

   queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;

   queue_t *actuators_list=i001->actuators_list;
   struct actuator_s *actuator;

   VERBOSE(9) fprintf(stderr,"%s  (%s) : shutdown thread ... ",INFO_STR,__func__);
   
   first_queue(counters_list);
   while(counters_list->nb_elem)
   {
      out_queue_elem(counters_list, (void **)&counter);
      free(counter);
      counter=NULL;
   }

   first_queue(sensors_list);
   while(sensors_list->nb_elem)
   {
      out_queue_elem(sensors_list, (void **)&sensor);
      free(sensor);
      sensor=NULL;
   }
   
   first_queue(actuators_list);
   while(actuators_list->nb_elem)
   {
      out_queue_elem(actuators_list, (void **)&actuator);
      free(actuator);
      actuator=NULL;
   }
   
   if(i001->thread)
   {
      pthread_cancel(*(i001->thread));
      pthread_join(*(i001->thread), NULL);
      FREE(i001->thread);
      i001->thread=NULL;
   }
   
   comio2_close(i001->ad);

   FREE(i001->ad);
   FREE(i001->counters_list);
   FREE(i001->sensors_list);
   FREE(i001->actuators_list);
   
   VERBOSE(9) fprintf(stderr,"done.\n");
   
   return NOERROR;
}


mea_error_t restart_interface_type_001(interface_type_001_t *i001,sqlite3 *db, tomysqldb_md_t *md)
{
   char full_dev[80];
   char dev[80];
   uint32_t id_interface;
   int ret;
   
   sscanf(i001->ad->serial_dev_name,"/dev/%s",full_dev);
   sprintf(dev,"SERIAL://%s:%ld",full_dev,(long)get_speed_from_speed_t(i001->ad->speed));
   
   id_interface=i001->id_interface;
   
   stop_interface_type_001(i001);
   sleep(5);
   ret=start_interface_type_001(i001, db, id_interface, (const unsigned char *)dev, md);

   return ret;
}


void *_thread_interface_type_001(void *args)
{
   struct thread_interface_type_001_params_s *params=(struct thread_interface_type_001_params_s *)args;
   
   interface_type_001_t *i001=params->it001;
   tomysqldb_md_t *md=params->md;
   free(params);
   params=NULL;

   interface_type_001_counters_init(i001);
   interface_type_001_sensors_init(i001);

   uint32_t cntr=0;
   while(1)
   {
      interface_type_001_counters_poll_inputs(i001, md);
      interface_type_001_sensors_poll_inputs(i001, md);
      cntr++;
      sleep(5);
   }
   pthread_testcancel();
}


int16_t check_status_interface_type_001(interface_type_001_t *i001)
{
   if(i001->ad->signal_flag!=0)
      return -1;
   
   return 0;
}


mea_error_t start_interface_type_001(interface_type_001_t *i001, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md)
{
   int16_t ret;
   
   char sql_request[255];
   char real_dev[80];
   char buff[80];
   speed_t speed;
   int fd = 0;
   sqlite3_stmt * stmt;
   comio2_ad_t *ad=NULL;
   
   pthread_t *counters_thread=NULL; // descripteur du thread
   struct thread_interface_type_001_params_s *params=NULL; // parametre à transmettre au thread
   
   // préparation des éléments de contexte de l'interface
   i001->id_interface=id_interface;
   i001->xPL_callback=NULL;
   i001->counters_list=NULL;
   i001->sensors_list=NULL;
   i001->actuators_list=NULL;


   ret=get_dev_and_speed((char *)dev, buff, sizeof(buff), &speed);
   if(!ret)
      sprintf(real_dev,"/dev/%s",buff);
   else
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : unknow interface device - %s\n", ERROR_STR,__func__,dev);
      return ERROR;
   }
   
   
   // initialisation de la liste des capteurs de type compteur
   i001->counters_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->counters_list)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto start_interface_type_001_clean_exit;
   }
   init_queue(i001->counters_list);
   
   
   // initialisation de la liste des actionneurs (sorties logiques de l'arduino)
   i001->actuators_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->actuators_list)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto start_interface_type_001_clean_exit;
   }
   init_queue(i001->actuators_list);


   // initialisation de la liste des autres capteurs (entrees logiques et analogiques)
   i001->sensors_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->sensors_list)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto start_interface_type_001_clean_exit;
   }
   init_queue(i001->sensors_list);

   
   // préparation de la requete permettant d'obtenir les capteurs associés à l'interface
   sprintf(sql_request,"SELECT * FROM sensors_actuators WHERE id_interface=%d", id_interface);
   ret = sqlite3_prepare_v2(db,sql_request,strlen(sql_request)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      goto start_interface_type_001_clean_exit;
   }
   
   // récupération des parametrages des capteurs dans la base
   int16_t nb_sensors_actuators=0;
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
         goto start_interface_type_001_clean_exit;
      }
   }
   
   // si on a trouvé une config
   if(nb_sensors_actuators)
   {
      ad=(comio2_ad_t *)malloc(sizeof(comio2_ad_t));
      if(!ad)
      {
         VERBOSE(2) {
            fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
            perror("");
         }
         goto start_interface_type_001_clean_exit;
      }
      
      fd = comio2_init(ad, real_dev, speed);
      if (fd == -1)
      {
         VERBOSE(2) {
            fprintf(stderr,"%s (%s) : init_arduino - Unable to open serial port (%s) : ",ERROR_STR,__func__,dev);
            perror("");
         }
         fd=0;
         goto start_interface_type_001_clean_exit;
      }
   }
   else
   {
      VERBOSE(5) fprintf(stderr,"%s (%s) : no sensor/actuator active for this interface (%d) : ",ERROR_STR,__func__,id_interface);
      goto start_interface_type_001_clean_exit;
   }
   
   i001->ad=ad;
   
   params=malloc(sizeof(struct thread_interface_type_001_params_s));
   if(!params)
      goto start_interface_type_001_clean_exit;
   
   params->it001=i001;
   params->md=md;
   
   counters_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!counters_thread)
      goto start_interface_type_001_clean_exit;
   
   i001->xPL_callback=interface_type_001_xPL_callback;

   if(pthread_create (counters_thread, NULL, _thread_interface_type_001, (void *)params))
   {
      VERBOSE(2) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      goto start_interface_type_001_clean_exit;
   }
   
   i001->thread=counters_thread;
   
   return NOERROR;
   
start_interface_type_001_clean_exit:
   FREE(params);
   FREE(counters_thread);
   if(fd)
      comio2_close(ad);
   if(ad)
      free(ad);
   if(i001)
   {
      if(i001->counters_list)
      {
         clear_queue(i001->counters_list,interface_type_001_free_counters_queue_elem);
         FREE(i001->counters_list);
      }
      if(i001->actuators_list)
      {
         clear_queue(i001->actuators_list,_interface_type_001_free_actuators_queue_elem);
         FREE(i001->actuators_list);
      }
      if(i001->sensors_list)
      {
         clear_queue(i001->sensors_list,interface_type_001_sensors_free_queue_elem);
         FREE(i001->sensors_list);
      }
   }
   
   return ERROR;
}

