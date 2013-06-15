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

#include "tomysqldb.h"

#include "error.h"
#include "debug.h"
#include "globals.h"
#include "xPLServer.h"
#include "arduino_pins.h"
#include "parameters_mgr.h"
#include "token_strings.h"
#include "interface_type_001_counters.h"

char *valid_counter_params[]={"I:M1","I:M2","I:M3","I:M4","I:TRAP",NULL};
#define COUNTER_PARAMS_M1       0
#define COUNTER_PARAMS_M2       1
#define COUNTER_PARAMS_M3       2
#define COUNTER_PARAMS_M4       3
#define COUNTER_PARAMS_TRAP     4


void interface_type_001_free_counters_queue_elem(void *d)
{
   struct electricity_counter_s *e=(struct electricity_counter_s *)d;
   
   free(e);
   e=NULL;
}


// pour la reception d'un trap à chaque changement du compteur
error_t counter_trap(int numTrap, void *args, char *buff)
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
               fprintf (stderr, "ERROR (counter_trap) : malloc error (%s/%d) - ",__FILE__,__LINE__);
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
               fprintf (stderr, "ERROR (counter_trap) : malloc error (%s/%d) - ",__FILE__,__LINE__);
               perror("");
            }
            return ERROR;
         }
         qelem->type=TOMYSQLDB_TYPE_PINST;
         qelem->data=(void *)query_pinst;
         

         pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(md.lock));
         if(!pthread_mutex_lock(&(md.lock)))
         {
            if(qelem)
               in_queue_elem(md.queue,(void *)qelem);
            pthread_mutex_unlock(&(md.lock));
         }
         pthread_cleanup_pop(0);
         
         {
            char value[20];
            xPL_MessagePtr cntrMessageStat = xPL_createBroadcastMessage(get_xPL_ServicePtr(), xPL_MESSAGE_TRIGGER);
            
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
            fprintf(stderr,"INFO  (cptr_trap) : %s;%s;%f;%f;%f\n",counter->name,now,counter->t,counter->t-t_old,counter->power);
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
         fprintf (stderr, "ERROR (start_interface_type_001) : malloc (%s/%d) - ",__FILE__,__LINE__);
         perror(""); }
      goto valid_and_malloc_counter_clean_exit;
   }
   
   // récupération des paramètres et contrôle de validité
   int nb_counter_params;
   int err;
   
   counter_params=malloc_parsed_parameters(parameters, valid_counter_params, &nb_counter_params, &err,1);
   if(counter_params)
   {
      int all_memory_params_set = 1;
      // VERBOSE(9) display_parsed_parameters(counter_params, nb_counter_params);
      for(int i=0;i<4;i++)
      {
         if(counter_params[i].label)
            counter->sensor_mem_addr[i]=counter_params[i].value.i;
         else
         {
            all_memory_params_set=0; // manque au moins un paramètre
            break;
         }
      }
      if(all_memory_params_set==0)
         goto valid_and_malloc_counter_clean_exit;
      
      if(counter_params[4].label)
         counter->trap=counter_params[4].value.i;
      else
         goto valid_and_malloc_counter_clean_exit;
      
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
      fprintf(stderr,"ERROR (start_interface_type_001 : %s/%s invalid. Check parameters\n",name,parameters);
   }
   
   return NULL;
}

