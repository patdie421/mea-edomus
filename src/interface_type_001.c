/*
 *  interface_type_001.c
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include "interfaces.h"
#include "interface_type_001.h"

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

#include "globals.h"
#include "token_strings.h"
#include "debug.h"
#include "memory.h"
#include "macros.h"

#include "comio.h"
#include "tomysqldb.h"

#include "arduino_pins.h"
#include "parameters_mgr.h"
#include "interface_type_001_sensors.h"
#include "interface_type_001_actuators.h"
#include "interface_type_001_counters.h"

#define TEMPO    300 // 5 minutes between 2 read


// structure contenant les parametres du thread de gestion des capteurs
struct thread_interface_type_001_params_s
{
   interface_type_001_t *it001;
   tomysqldb_md_t *md;
};


//
// xPLSend -c control.basic -m cmnd device=RELAY1 type=output current=pulse data1=2000
// xPLSend -c sensor.request -m cmnd request=current device=CONSO type=POWER => dernière puissance instantannée
// xPLSend -c sensor.request -m cmnd request=current device=CONSO type=ENERGY => valeur du compteur ERDF
//
int interface_type_001_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   xPL_NameValueListPtr ListNomsValeursPtr ;
   char *schema_type, *schema_class, *device, *type;
   interface_type_001_t *i001=(interface_type_001_t *)userValue;

   schema_class       = xPL_getSchemaClass(theMessage);
   schema_type        = xPL_getSchemaType(theMessage);
   ListNomsValeursPtr = xPL_getMessageBody(theMessage);
   device             = xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_DEVICE_ID));
   type               = xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_TYPE_ID));
   
   VERBOSE(9) fprintf(stderr,"INFO  (interface_type_001_xPL_callback) : xPL Message to process : %s.%s\n",schema_class,schema_type);

   if(strcmplower(schema_class, get_token_by_id(XPL_CONTROL_ID)) == 0 &&
      strcmplower(schema_type, get_token_by_id(XPL_BASIC_ID)) == 0)
   {
      if(!type)
         return 0;
      else
         return xpl_actuator(i001, ListNomsValeursPtr, device, type);
   }
   else if(strcmplower(schema_class, get_token_by_id(XPL_SENSOR_ID)) == 0 &&
           strcmplower(schema_type, get_token_by_id(XPL_REQUEST_ID)) == 0)
   {
      char *request = xPL_getNamedValue(ListNomsValeursPtr, get_token_by_id(XPL_REQUEST_ID));
      if(!request)
      {
         return 0;
      }
      
      if(strcmplower(request,get_token_by_id(XPL_CURRENT_ID))!=0)
      {
         return 0;
      }

      queue_t *counters_list=i001->counters_list;
      struct electricity_counter_s *counter;
      
      first_queue(counters_list);
      for(int i=0; i<counters_list->nb_elem; i++)
      {
         current_queue(counters_list, (void **)&counter);

         if(strcmplower(device,counter->name)==0)
         {
            xPL_MessagePtr cntrMessageStat ;
            char value[20];
            
            if(strcmplower(type, get_token_by_id(XPL_ENERGY_ID))==0)
            {
               sprintf(value,"%d", counter->kwh_counter);
            }
            else if(strcmplower(type, get_token_by_id(XPL_POWER_ID))==0)
            {
               sprintf(value,"%f", counter->power);
            }
            else
            {
               return 0;
            }
            
            cntrMessageStat = xPL_createBroadcastMessage(theService, xPL_MESSAGE_STATUS) ;
            
            xPL_setSchema(cntrMessageStat,  get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),counter->name);
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID),type);
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID),value);
            
            // Broadcast the message
            xPL_sendMessage(cntrMessageStat);
            
            xPL_releaseMessage(cntrMessageStat);
            
            return 1;
         }
         next_queue(counters_list);
      }
      
      xpl_sensor(i001, theService, ListNomsValeursPtr, device, type);
   }
   
   return 0;
}


/*
int cntr_trap(int numTrap, void *args, char *buff)
{
   return 0;
}
*/

int stop_interface_type_001(interface_type_001_t *i001, int signal_number)
{
   queue_t *counters_list=i001->counters_list;
   struct electricity_counter_s *counter;

   queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;

   queue_t *actuators_list=i001->actuators_list;
   struct actuator_s *actuator;

   VERBOSE(9) fprintf(stderr,"INFO  (stop_interface_type_001) : start stoping counter thread (signal = %d).\n",signal_number);
   
   first_queue(counters_list);
   while(counters_list->nb_elem)
   {
      out_queue_elem(counters_list, (void **)&counter);
      comio_remove_trap(i001->ad, counter->trap);
      free(counter);
      counter=NULL;
   }

   first_queue(sensors_list);
   while(sensors_list->nb_elem)
   {
      out_queue_elem(sensors_list, (void **)&sensor);
      comio_remove_trap(i001->ad, sensor->arduino_pin+10);
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
      i001->thread=NULL;
   }
   
   comio_close(i001->ad);

   FREE(i001->ad);
   FREE(i001->thread);
   FREE(i001->counters_list);
   
   VERBOSE(9) fprintf(stderr,"INFO  (stop_interface_type_001) : stoping counter thread done.\n");
   
   return 0;
}


int restart_interface_type_001(interface_type_001_t *i001,sqlite3 *db, tomysqldb_md_t *md)
{
   char full_dev[80];
   char dev[80];
   int id_interface;
   int ret;
   
   sscanf(i001->ad->serial_dev_name,"/dev/%s",full_dev);
   sprintf(dev,"SERIAL://%s",full_dev);

   id_interface=i001->id_interface;
   
   stop_interface_type_001(i001, 0);
   ret=start_interface_type_001(i001, db, id_interface, (const unsigned char *)dev, md);
   

   return ret;
}


void *_thread_interface_type_001(void *args)
{
   struct thread_interface_type_001_params_s *params=(struct thread_interface_type_001_params_s *)args;
   
   interface_type_001_t *i001=params->it001;
   comio_ad_t *ad=i001->ad;
   queue_t *counters_list=i001->counters_list;
   queue_t *sensors_list=i001->sensors_list;
   tomysqldb_md_t *md=params->md;
   free(params);
   params=NULL;
   
   int err=0;
   int ret;
   
   tomysqldb_queue_elem_t *qelem;
   struct electricity_counters_query_s *ec_query;
   
   struct timeval tv;
   
   struct electricity_counter_s *counter;
   struct sensor_s *sensor;
   
   // initialisation des trap compteurs
   first_queue(counters_list);
   for(int i=0; i<counters_list->nb_elem; i++)
   {
      current_queue(counters_list, (void **)&counter);
      comio_set_trap2(ad, counter->trap, counter_trap, (void *)counter);
      
      next_queue(counters_list);
   }

   // initialisation des trap changement etat entrées logiques
   first_queue(sensors_list);
   for(int i=0; i<sensors_list->nb_elem; i++)
   {
      current_queue(sensors_list, (void **)&sensor);
      comio_set_trap2(ad,  sensor->arduino_pin+10, digital_in_trap, (void *)sensor);
      
      next_queue(sensors_list);
   }
   

   // a partir d'ici besoin de mutuex pour l'acces à compteur_prod et compteur_conso, car le trap est généré par un
   // thread s'exécutant en parallele
   unsigned int cntr=0;
   while(1)
   {
      cntr++;
      
      {
         int comio_err;

         first_queue(sensors_list);
         for(int i=0; i<sensors_list->nb_elem; i++)
         {
            current_queue(sensors_list, (void **)&sensor);

            if(sensor->arduino_pin_type==ANALOG_IN_ID)
            {
               pthread_mutex_lock(&i001->operation_lock);
               int v=(int16_t)comio_call(i001->ad, sensor->arduino_function, sensor->arduino_pin, &comio_err);
               pthread_mutex_unlock(&i001->operation_lock);

               if(v>=0 && sensor->val!=v)
               {
                  sensor->val=v;
                  sensor->computed_val=sensor->compute_fn(v);
                  
                  if(sensor->compute==XPL_TEMP_ID)
                  {
                     VERBOSE(9) fprintf(stderr,"INFO  (_thread_interface_type_001) : Temp sensor %s =  %.1f °C (%d) \n",sensor->name,sensor->computed_val,sensor->val);
                  }
                  else if(sensor->compute==XPL_VOLTAGE_ID)
                  {
                     VERBOSE(9) fprintf(stderr,"INFO  (_thread_interface_type_001) : Voltage sensor %s =  %.1f V (%d) \n",sensor->name,sensor->computed_val,sensor->val);
                  }
                  else
                  {
                     VERBOSE(9) fprintf(stderr,"INFO  (_thread_interface_type_001) : sensor %s = %d\n",sensor->name,sensor->val);
                  }
                  
                  {
                     char str_value[20];
                     
                     xPL_ServicePtr servicePtr = get_xPL_ServicePtr();
                     if(servicePtr)
                     {
                        xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_TRIGGER);
                     
                        sprintf(str_value,"%0.1f",sensor->computed_val);
                     
                        xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
                        xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),sensor->name);
                        xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_ENERGY_ID));
                        xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TEMP_ID),str_value);
                     
                        // Broadcast the message
                        xPL_sendMessage(cntrMessageStat);
                     
                        xPL_releaseMessage(cntrMessageStat);
                     }
                  }
               }
            }
            
            next_queue(sensors_list);
         }

      }
      
      if(cntr>60)
      {
         int l1,l2,l3,l4;
         unsigned long c;
         cntr=0;
         
         first_queue(counters_list);
         for(int i=0; i<counters_list->nb_elem; i++)
         {
            current_queue(counters_list, (void **)&counter);
            do
            {
               l2=0;l3=0;l4=0;
               // lecture des compteurs stockés dans les variables partagées
               pthread_mutex_lock(&i001->operation_lock);
               l1=comio_operation(ad, OP_LECTURE, counter->sensor_mem_addr[0], TYPE_MEMOIRE, 0, &err);
               if(l1<0)
                  goto _thread_interface_type_001_operation_abord;
               l2=comio_operation(ad, OP_LECTURE, counter->sensor_mem_addr[1], TYPE_MEMOIRE, 0, &err);
               if(l2<0)
                  goto _thread_interface_type_001_operation_abord;
               l3=comio_operation(ad, OP_LECTURE, counter->sensor_mem_addr[2], TYPE_MEMOIRE, 0, &err);
               if(l3<0)
                  goto _thread_interface_type_001_operation_abord;
               l4=comio_operation(ad, OP_LECTURE, counter->sensor_mem_addr[3], TYPE_MEMOIRE, 0, &err);
               
_thread_interface_type_001_operation_abord:
               pthread_mutex_unlock(&i001->operation_lock);
            }
            while(l1<0 || l2<0 || l3<0 || l4<0);
            c=     l4;
            c=c <<  8;
            c=c |  l3;
            c=c <<  8;
            c=c |  l2;
            c=c <<  8;
            c=c |  l1;
            
            // prise du chrono
            gettimeofday(&tv, NULL);
            
            ret=pthread_mutex_lock(&(counter->lock));
            if(!ret)
            {
               // debut section critique
               counter->wh_counter=c;
               counter->kwh_counter=c / 1000;
               counter->counter=c;
               
               ec_query=(struct electricity_counters_query_s *)malloc(sizeof(struct electricity_counters_query_s));
               if(!ec_query)
               {
                  VERBOSE(1) {
                     fprintf (stderr, "ERROR (_thread_interface_type_001) : malloc error (%s/%d) - ",__FILE__,__LINE__);
                     perror("");
                  }
                  pthread_exit(NULL);
               }
               ec_query->sensor_id=counter->sensor_id; // à remplacer
               ec_query->wh_counter=counter->wh_counter;
               ec_query->kwh_counter=counter->kwh_counter;
               ec_query->flag=0;
               memcpy(&(ec_query->date_tv),&tv,sizeof(struct timeval));
               
               pthread_mutex_unlock(&(counter->lock));
               
               qelem=malloc(sizeof(tomysqldb_queue_elem_t));
               if(!qelem)
               {
                  VERBOSE(1) {
                     fprintf (stderr, "ERROR (_thread_interface_type_001) : malloc error (%s/%d) - ",__FILE__,__LINE__);
                     perror("");
                  }
                  if(ec_query)
                  {
                     free(ec_query);
                     ec_query=NULL;
                  }
                  pthread_exit(NULL);
               }
               qelem->type=TOMYSQLDB_TYPE_EC;
               qelem->data=(void *)ec_query;
               
               {
                  char value[20];
                  xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(get_xPL_ServicePtr(), xPL_MESSAGE_TRIGGER);
                  
                  sprintf(value,"%d",counter->kwh_counter);
                  
                  xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
                  xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),counter->name);
                  xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_ENERGY_ID));
                  xPL_setMessageNamedValue(cntrMessageStat,  get_token_by_id(XPL_CURRENT_ID),value);
                  
                  // Broadcast the message
                  xPL_sendMessage(cntrMessageStat);
                  
                  xPL_releaseMessage(cntrMessageStat);
               }
               
               VERBOSE(9) fprintf(stderr,"INFO  (_thread_interface_type_001) : counter %s %ld (WH=%d KWH=%d)\n",counter->name, counter->counter, counter->wh_counter,counter->kwh_counter);
            }
            else
            {
               qelem=NULL;
               VERBOSE(1) {
                  fprintf(stderr,"ERROR (_thread_interface_type_001) : can't take semaphore - ");
                  perror("");
               }
            }
            
            if(qelem)
            {
               pthread_mutex_lock(&(md->lock));
               in_queue_elem(md->queue,(void *)qelem);
               pthread_mutex_unlock(&(md->lock));
            }
            
            next_queue(counters_list);
         }
      }
      pthread_testcancel();
      
      // sleep(TEMPO);
      sleep(5);
   }
}


int check_status_interface_type_001(interface_type_001_t *i001)
{
   if(i001->ad->signal_flag!=0)
      return -1;
   
   return 0;
}


int start_interface_type_001(interface_type_001_t *i001, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md)
{
   int ret;
   
   char sql_request[255];
   char real_dev[80];
   char buff[80];
   
   sqlite3_stmt * stmt;
   comio_ad_t *ad=NULL;
   
   pthread_t *counters_thread=NULL; // descripteur du thread
   struct thread_interface_type_001_params_s *params=NULL; // parametre à transmettre au thread

   
   // préparation des éléments de contexte de l'interface
   i001->id_interface=id_interface;
   i001->xPL_callback=NULL;
   pthread_mutex_init(&i001->operation_lock, NULL);
   if(sscanf((char *)dev,"SERIAL://%s",buff)==1) // construction du nom de device unix
      sprintf(real_dev,"/dev/%s",buff);
   else
   {
      VERBOSE(1) fprintf (stderr, "ERROR (start_interface_type_001) : unknow interface device - %s\n", dev);
      return -1;
   }
   
   
   // initialisation de la liste des capteurs de type compteur
   i001->counters_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->counters_list)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (start_interface_type_001) : malloc (%s/%d) - ",__FILE__,__LINE__-4);
         perror(""); }
      return -1;
   }
   init_queue(i001->counters_list);
   
   
   // initialisation de la liste des actionneurs (sorties logiques de l'arduino)
   i001->actuators_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->actuators_list)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (start_interface_type_001) : malloc (%s/%d) - ",__FILE__,__LINE__-4);
         perror(""); }
      return -1;
   }
   init_queue(i001->actuators_list);


   // initialisation de la liste des autres capteurs (entrees logiques et analogiques)
   i001->sensors_list=(queue_t *)malloc(sizeof(queue_t));
   if(!i001->sensors_list)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (start_interface_type_001) : malloc (%s/%d) - ",__FILE__,__LINE__-4);
         perror(""); }
      return -1;
   }
   init_queue(i001->sensors_list);

   
   // préparation de la requete permettant d'obtenir les capteurs associés à l'interface
   sprintf(sql_request,"SELECT * FROM sensors_actuators WHERE id_interface=%d", id_interface);
   ret = sqlite3_prepare_v2(db,sql_request,strlen(sql_request)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "ERROR (start_interface_type_001) : sqlite3_prepare_v2 - %s\n", sqlite3_errmsg (db));
      return -1;
   }
   
   // récupération des parametrages des capteurs dans la base
   int nb_sensors_actuators=0;
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
               counter=valid_and_malloc_counter(id_sensor_actuator, (char *)name, (char *)parameters);
               if(counter)
                  in_queue_elem(i001->counters_list, counter);
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
               sensor=valid_and_malloc_sensor(id_sensor_actuator, (char *)name, (char *)parameters);
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
         return -1;
      }
   }
   
   // si on a trouvé une config
   if(nb_sensors_actuators)
   {
      ad=(comio_ad_t *)malloc(sizeof(comio_ad_t));
      if(!ad)
      {
         VERBOSE(1) {
            fprintf (stderr, "ERROR (start_interface_type_001) : malloc (%s/%d) - ",__FILE__,__LINE__-4);
            perror("");
         }
         goto start_interface_type_001_clean_exit;
      }
      
      int fd = comio_init(ad, real_dev);
      if (fd == -1)
      {
         VERBOSE(1) {
            fprintf(stderr,"ERROR (start_interface_type_001) : init_arduino - Unable to open serial port (%s) : ",dev);
            perror("");
         }
         goto start_interface_type_001_clean_exit;
      }
   }
   else
   {
      fprintf(stderr,"ERROR (start_interface_type_001) : no sensor/actuator active for this interface (%d) : ",id_interface);
      return -1; // pas de capteur on s'arrête
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
      VERBOSE(1) fprintf(stderr, "ERROR (start_interface_type_001) : pthread_create - can't start thread\n");
      goto start_interface_type_001_clean_exit;
   }
   
   i001->thread=counters_thread;
   
   return 0;
   
start_interface_type_001_clean_exit:
   FREE(params);
   FREE(counters_thread);
   if(ad)
   {
      FREE(ad->queue); // il n'y a forcement pas d'élément dans la file, un free suffit. Sinon il aurait fallu vider la file d'abord
      free(ad);
   }
   if(i001)
   {
      if(i001->counters_list)
      {
         clear_queue(i001->counters_list,_interface_type_001_free_counters_queue_elem);
         FREE(i001->counters_list);
      }
      if(i001->actuators_list)
      {
         clear_queue(i001->actuators_list,_interface_type_001_free_actuators_queue_elem);
         FREE(i001->actuators_list);
      }
      if(i001->sensors_list)
      {
         clear_queue(i001->sensors_list,_interface_type_001_free_sensors_queue_elem);
         FREE(i001->sensors_list);
      }
   }
   
   return -1;
}

