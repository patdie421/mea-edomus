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

#include "dbServer.h"

#include "error.h"
#include "debug.h"
#include "globals.h"
#include "xPLServer.h"
#include "arduino_pins.h"
#include "parameters_mgr.h"
#include "token_strings.h"
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

char *valid_counter_params[]={"I:COUNTER", NULL};
#define COUNTER_PARAMS_COUNTER  0


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
   struct pinst_query_s *query_pinst;
   struct electricity_counter_s *counter;
   
   counter=(struct electricity_counter_s *)args;
   
   // prise du chrono
   gettimeofday(&tv, NULL);
   
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(counter->lock));
   pthread_mutex_lock(&(counter->lock));
   { // début section critique
      if(counter->t<0)
      {
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
      }
      else
      {
         tomysqldb_queue_elem_t *qelem;
         
         t_old=counter->t;
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
         
         // calcul de la conso instantannée (enfin une estimation)
         counter->power=3600/(counter->t-t_old);
         
         // préparation des données de la requete
         query_pinst=malloc(sizeof(struct pinst_query_s));
         if(!query_pinst)
         {
            VERBOSE(1) {
               fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
               perror("");
            }
            return ERROR;
         }
         
         memcpy(&(query_pinst->date_tv),&tv,sizeof(struct timeval));
         query_pinst->sensor_id=counter->sensor_id;
         query_pinst->power=counter->power;
         query_pinst->delta_t=counter->t-t_old;
         query_pinst->flag=0;
         
         qelem=malloc(sizeof(tomysqldb_queue_elem_t));
         if(!qelem)
         {
            VERBOSE(1) {
               fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
               perror("");
            }
            return ERROR;
         }
         qelem->type=TOMYSQLDB_TYPE_PINST;
         qelem->data=(void *)query_pinst;
         

         pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(myd->lock));
         if(!pthread_mutex_lock(&(myd->lock)))
         {
            if(qelem)
               in_queue_elem(myd->queue,(void *)qelem);
            pthread_mutex_unlock(&(myd->lock));
         }
         pthread_cleanup_pop(0);
         
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
   } // fin section critique
   pthread_mutex_unlock(&(counter->lock));
   pthread_cleanup_pop(0);
   return NOERROR;
}


struct electricity_counter_s *valid_and_malloc_counter(int id_sensor_actuator, char *name, char *parameters)
{
   parsed_parameter_t *counter_params=NULL;
   
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
   
   counter_params=malloc_parsed_parameters(parameters, valid_counter_params, &nb_counter_params, &err,1);
   if(counter_params)
   {
//      VERBOSE(9) display_parsed_parameters(counter_params, nb_counter_params);
      int16_t num_counter=-1;
      
      num_counter=counter_params[COUNTER_PARAMS_COUNTER].value.i;
      if(num_counter<0 || num_counter>1)
         goto valid_and_malloc_counter_clean_exit;
      
      for(int i=0;i<4;i++)
         counter->sensor_mem_addr[i]=counters_mem[num_counter][i];
      counter->trap=counters_trap[num_counter];
      
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
   counter->counter=0;
   counter->t=-1.0;
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

