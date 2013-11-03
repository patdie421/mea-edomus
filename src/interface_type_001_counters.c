//
//  interface_type_001_counters.c
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
#include <pthread.h>

#include "error.h"
#include "debug.h"
#include "globals.h"
#include "comio.h"
#include "arduino_pins.h"
#include "parameters_mgr.h"
#include "token_strings.h"
#include "timer.h"
#include "dbServer.h"
#include "xPLServer.h"

#include "interface_type_001_counters.h"

uint16_t counters_mem[2][4]={
   {0,1,2,3},
   {10,11,12,13}
};

uint16_t counters_trap[2]={1,2};

/*
char *valid_counter_params[]={"I:M1","I:M2","I:M3","I:M4","I:TRAP",NULL};
#define COUNTER_PARAMS_M1       0
#define COUNTER_PARAMS_M2       1
#define COUNTER_PARAMS_M3       2
#define COUNTER_PARAMS_M4       3
#define COUNTER_PARAMS_TRAP     4
*/

char *valid_counter_params[]={"I:COUNTER", "I:FREQUENCY", NULL};
#define COUNTER_PARAMS_COUNTER   0
#define COUNTER_PARAMS_FREQUENCY 1


void interface_type_001_free_counters_queue_elem(void *d)
{
   struct electricity_counter_s *e=(struct electricity_counter_s *)d;
   
   free(e);
   e=NULL;
}


// pour la reception d'un trap à chaque changement du compteur
mea_error_t counter_trap(int numTrap, void *args, char *buff)
 {
   double t_old;
   struct timeval tv;
   struct electricity_counter_s *counter;
   counter=(struct electricity_counter_s *)args;

   // prise du chrono
   gettimeofday(&tv, NULL); 
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(counter->lock));
   pthread_mutex_lock(&(counter->lock));
   {
      if(counter->t<0)
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
      else
      {
         t_old=counter->t;
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
         // calcul de la conso instantannée (enfin une estimation)
         counter->last_power=counter->power;
         counter->power=3600/(counter->t-t_old);
         
         /* à revoir */
         // tomysqldb_add_data_to_sensors_values(myd, counter->sensor_id, counter->power, UNIT_W, (float)counter->t-t_old, "");

         char value[20];
         xPL_ServicePtr servicePtr = get_xPL_ServicePtr();
         if(servicePtr)
         {
            xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_TRIGGER);
            sprintf(value,"%f",counter->power);
            xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID), counter->name);
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_POWER_ID));
            xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_CURRENT_ID), value);
            // Broadcast the message
            xPL_sendMessage(cntrMessageStat);
            xPL_releaseMessage(cntrMessageStat);
         }
         VERBOSE(9) {
            char now[30];
            strftime(now,30,"%d/%m/%y;%H:%M:%S",localtime(&tv.tv_sec));
            fprintf(stderr,"%s  (%s) : %s;%s;%f;%f;%f\n",INFO_STR,__func__,counter->name,now,counter->t,counter->t-t_old,counter->power);
         }
      }
   }
   // fin section critique
   pthread_mutex_unlock(&(counter->lock));
   pthread_cleanup_pop(0);
   return NOERROR;
}  


struct electricity_counter_s *valid_and_malloc_counter(int id_sensor_actuator, char *name, char *parameters)
{
   parsed_parameters_t *counter_params=NULL;
   
   struct electricity_counter_s *counter=(struct electricity_counter_s *)malloc(sizeof(struct electricity_counter_s));
   if(!counter)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto valid_and_malloc_counter_clean_exit;
   }
   
   // récupération des paramètres et contrôle de validité
   int nb_counter_params;
   int err;
   
   int16_t num_counter=-1;
   counter_params=malloc_parsed_parameters(parameters, valid_counter_params, &nb_counter_params, &err,1);
   if(counter_params)
   {
      //      VERBOSE(9) display_parsed_parameters(counter_params, nb_counter_params);
      num_counter=counter_params[COUNTER_PARAMS_COUNTER].value.i;
      if(num_counter<0 || num_counter>1)
         goto valid_and_malloc_counter_clean_exit;
      
      for(int i=0;i<4;i++)
         counter->sensor_mem_addr[i]=counters_mem[num_counter][i];
      counter->trap=counters_trap[num_counter];
      
      if(counter_params[COUNTER_PARAMS_FREQUENCY].value.i>0)
      {
         init_timer(&(counter->timer),counter_params[COUNTER_PARAMS_FREQUENCY].value.i,1);
      }
      else
      {
         init_timer(&(counter->timer),300,1); // lecture toutes les 5 minutes par défaut
      }
      
      free_parsed_parameters(counter_params, nb_counter_params);
      free(counter_params);
   }
   else
   {
      // traité ici err;
      goto valid_and_malloc_counter_clean_exit;
   }
   
   strcpy(counter->name,(char *)name);
   counter->sensor_id=id_sensor_actuator;
   counter->wh_counter=0;
   counter->kwh_counter=0;
   counter->counter=num_counter;
   counter->t=-1.0;
   start_timer(&(counter->timer));
   counter->lock=(pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
   
   return counter;
   
valid_and_malloc_counter_clean_exit:
   if(counter)
      free(counter);
   if(counter_params)
   {
      free_parsed_parameters(counter_params, nb_counter_params);
      free(counter_params);
   }
   VERBOSE(1) {
      fprintf(stderr,"%s (%s) : %s/%s invalid. Check parameters.\n",ERROR_STR,__func__,name,parameters);
   }
   
   return NULL;
}


void counter_to_xpl(struct electricity_counter_s *counter)
{
   char value[20];
   
   xPL_ServicePtr servicePtr = get_xPL_ServicePtr();
   if(servicePtr)
   {
      xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_TRIGGER);
      
      sprintf(value,"%d",counter->kwh_counter);
      
      xPL_setSchema(cntrMessageStat, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_BASIC_ID));
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_DEVICE_ID),counter->name);
      xPL_setMessageNamedValue(cntrMessageStat, get_token_by_id(XPL_TYPE_ID), get_token_by_id(XPL_ENERGY_ID));
      xPL_setMessageNamedValue(cntrMessageStat,  get_token_by_id(XPL_CURRENT_ID),value);
      
      // Broadcast the message
      xPL_sendMessage(cntrMessageStat);
      
      xPL_releaseMessage(cntrMessageStat);
   }
}


int16_t counter_to_db(tomysqldb_md_t *md, struct electricity_counter_s *counter)
{
   return tomysqldb_add_data_to_sensors_values(md, counter->sensor_id, counter->wh_counter, UNIT_WH, counter->kwh_counter, "WH");
}


void counter_read(comio_ad_t *ad, struct electricity_counter_s *counter)
{
   int l1,l2,l3,l4;
   unsigned long c;
   int err=0;
   
   do
   {
      /* l1=0; */l2=0;l3=0;l4=0;
      // lecture des compteurs stockés dans les variables partagées
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
      continue;
   }
   while(l1<0 || l2<0 || l3<0 || l4<0);
   c=     l4;
   c=c <<  8;
   c=c |  l3;
   c=c <<  8;
   c=c |  l2;
   c=c <<  8;
   c=c |  l1;
   
   // debut section critique
   counter->last_counter=counter->counter;
   counter->wh_counter=c;
   counter->kwh_counter=c / 1000;
   counter->counter=c;
}
