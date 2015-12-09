//
//  mea-eDomus
//
//  Created by Patrice Dietsch on 07/12/15.
//
//
#define DEBUGFLAG 0

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include "xPL.h"
#include "xPLServer.h"
#include "mea_verbose.h"
#include "globals.h"
#include "consts.h"
#include "mea_queue.h"
#include "tokens.h"

#include "processManager.h"
#include "notify.h"

#include "automator.h"
#include "automatorServer.h"

char *automator_server_name_str="AUTOMATORSERVER";
char *automator_input_exec_time_str="INEXECTIME";
char *automator_output_exec_time_str="OUTEXECTIME";
char *automator_xplin_str="XPLIN";
char *automator_xplout_str="XPLOUT";

long automator_xplin_indicator=0;
long automator_xplout_indicator=0;

char *rules_file=NULL;

// globales pour le fonctionnement du thread
pthread_t *_automatorServer_thread_id=NULL;
int _automatorServer_thread_is_running=0;
int _automatorServer_monitoring_id=-1;

mea_queue_t *automator_msg_queue;
pthread_cond_t automator_msg_queue_cond;
pthread_mutex_t automator_msg_queue_lock;


void set_automatorServer_isnt_running(void *data)
{
   _automatorServer_thread_is_running=0;
}


void setAutomatorRulesFile(char *file)
{
   if(rules_file)
   {
      free(rules_file);
      rules_file=NULL;
   }
   rules_file=malloc(strlen(file)+1);
   
   strcpy(rules_file, file);
}


mea_error_t automatorServer_add_msg(xPL_MessagePtr msg)
{
   automator_msg_t *e=NULL;
   int ret=NOERROR;
   e=(automator_msg_t *)malloc(sizeof(automator_msg_t));
   if(!e)
      return ERROR;

   e->msg=msg;
 
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
   pthread_mutex_lock(&automator_msg_queue_lock);
   if(automator_msg_queue)
   {
      mea_queue_in_elem(automator_msg_queue, e);
      if(automator_msg_queue->nb_elem>=1)
         pthread_cond_broadcast(&automator_msg_queue_cond);
   }
   else
      ret=ERROR;
   pthread_mutex_unlock(&automator_msg_queue_lock);
   pthread_cleanup_pop(0);

   if(ret!=ERROR)
      return NOERROR;
   
automatorServer_add_msg_clean_exit:
   if(e)
   {
      if(e->msg)
      {
         free(e->msg);
         e->msg=NULL;
      }
      free(e);
      e=NULL;
   }
   return ERROR;
}


void *_automator_thread(void *data)
{
   int ret;
   
   automator_msg_t *e;
   
   pthread_cleanup_push( (void *)set_automatorServer_isnt_running, (void *)NULL );
   _automatorServer_thread_is_running=1;
   process_heartbeat(_automatorServer_monitoring_id);

   automator_xplin_indicator=0;
   automator_xplout_indicator=0;

   mea_timer_t indicator_timer;
   mea_init_timer(&indicator_timer, 5, 1);
   mea_start_timer(&indicator_timer);

   automator_init(rules_file);

   int16_t timeout;
   
   while(1)
   {
      ret=0;
      process_heartbeat(_automatorServer_monitoring_id);
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
      pthread_mutex_lock(&automator_msg_queue_lock);

      if(mea_test_timer(&indicator_timer)==0)
      {
         process_update_indicator(_automatorServer_monitoring_id, automator_xplin_str, automator_xplin_indicator);
         process_update_indicator(_automatorServer_monitoring_id, automator_xplout_str, automator_xplout_indicator);
      }
   
      timeout=0; // pour faire en sorte de n'avoir qu'un seul pthread_mutex_unlock en face du pthread_mutex_lock ci-dessus
      if(automator_msg_queue && automator_msg_queue->nb_elem==0)
      {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
/*
         ts.tv_sec = tv.tv_sec + 10; // timeout de 10 secondes
         ts.tv_nsec = tv.tv_nsec;
*/
         long ns_timeout=1000 * 1000 * 50; // 50 ms en nanoseconde
         ts.tv_sec = tv.tv_sec; // timeout de 10 secondes
         ts.tv_nsec = (tv.tv_usec * 1000) + ns_timeout;
         if(ts.tv_nsec>1000000000L) // 1.000.000.000 ns = 1 seconde
         {
            ts.tv_sec++;
            ts.tv_nsec=ts.tv_nsec - 1000000000L;
         }
         ret=pthread_cond_timedwait(&automator_msg_queue_cond, &automator_msg_queue_lock, &ts);
         if(ret!=0)
         {
            if(ret==ETIMEDOUT)
            {
               timeout=1;
               ret=0;
            }
            else if(ret==EINVAL)
            {
               DEBUG_SECTION2(DEBUGFLAG) {
                  mea_log_printf("%s (%s) : pthread_cond_timedwait EINVAL error\n", DEBUG_STR, __func__);
               }
            }
            else
            {
               // autres erreurs à traiter
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : pthread_cond_timedwait error (%d) - ", DEBUG_STR, __func__, ret);
                  perror("");
               }
               
               process_async_stop(_automatorServer_monitoring_id);
               for(;;) sleep(1);
            }
         }
      }
      
      if (ret==0 && automator_msg_queue && !timeout) // pas d'erreur, on récupère un élément dans la queue
         ret=mea_queue_out_elem(automator_msg_queue, (void **)&e);
      else
         ret=-1;

      pthread_mutex_unlock(&automator_msg_queue_lock);
      pthread_cleanup_pop(0);

      process_heartbeat(_automatorServer_monitoring_id); 

      if (timeout==1) // timeout => un tour d'automate
      {
         pthread_testcancel();
         
         automator_match_inputs_rules(_inputs_rules, NULL);
         automator_play_output_rules(_outputs_rules);
         automator_reset_inputs_change_flags();
         continue;
      }
     
      if(!ret) // on a sortie un élément de la queue
      {
//         DEBUG_SECTION2(DEBUGFLAG) displayXPLMsg(e->msg);
         automator_match_inputs_rules(_inputs_rules, e->msg);
         automator_play_output_rules(_outputs_rules);
         automator_reset_inputs_change_flags();
         automator_xplin_indicator++;

         // faire ici ce qu'il y a à faire
         if(e)
         {
            if(e->msg)
            {
               // liberer le message
               
               xPL_releaseMessage(e->msg);
               e->msg=NULL;
            }
            free(e);
            e=NULL;
         }
      }
      else
      {
         // pb d'accés aux données de la file
         VERBOSE(9) mea_log_printf("%s (%s) : mea_queue_out_elem - can't access queue element\n", ERROR_STR, __func__);
      }
      
      pthread_testcancel();
   }
   
   pthread_cleanup_pop(1);

   pthread_exit(NULL);
   
   return NULL;
}


pthread_t *automatorServer()
{
   pthread_t *automator_thread=NULL;

   automator_msg_queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!automator_msg_queue)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   mea_queue_init(automator_msg_queue);
   pthread_mutex_init(&automator_msg_queue_lock, NULL);
   pthread_cond_init(&automator_msg_queue_cond, NULL);
 
   automator_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!automator_thread)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto automatorServer_clean_exit;
   }
   
   if(pthread_create (automator_thread, NULL, _automator_thread, NULL))
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
         perror("");
      }
      goto automatorServer_clean_exit;

   }
   pthread_detach(*automator_thread);
   
   if(automator_thread)   
      return automator_thread;

automatorServer_clean_exit:
   if(automator_thread)
   {
      free(automator_thread);
      automator_thread=NULL;
   }

   if(automator_msg_queue)
   {
      free(automator_msg_queue);
      automator_msg_queue=NULL;
   }

   return NULL;
}


void automator_msg_queue_free_queue_elem(void *d)
{
   automator_msg_t *e=(automator_msg_t *)d;
   xPL_releaseMessage(e->msg);
   free(e->msg);
} 


int stop_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(_automatorServer_thread_id)
   {
      pthread_cancel(*_automatorServer_thread_id);
      int counter=100;
      while(counter--)
      {
         if(_automatorServer_thread_is_running)
            usleep(100);
         else
            break;
      }
      DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__, automator_server_name_str, 100-counter);

      
      free(_automatorServer_thread_id);
      _automatorServer_thread_id=NULL;
   }

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
   pthread_mutex_lock(&automator_msg_queue_lock);
   if(automator_msg_queue)
   {
      mea_queue_cleanup(automator_msg_queue, automator_msg_queue_free_queue_elem);
      free(automator_msg_queue);
      automator_msg_queue=NULL;
   }
   pthread_mutex_unlock(&automator_msg_queue_lock);
   pthread_cleanup_pop(0);

   _automatorServer_monitoring_id=-1;

   VERBOSE(1) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, automator_server_name_str, stopped_successfully_str);
   mea_notify_printf('S', "%s %s.", automator_server_name_str, stopped_successfully_str);

   return 0;
}


int start_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct automatorServer_start_stop_params_s *automatorServer_start_stop_params = (struct automatorServer_start_stop_params_s *)data;

   char err_str[80], notify_str[256];

//   fprintf(stderr,"%s\n",automatorServer_start_stop_params->params_list[RULES_FILE]);
   if(automatorServer_start_stop_params->params_list[RULES_FILE])
   {
      setAutomatorRulesFile(automatorServer_start_stop_params->params_list[RULES_FILE]);
      
      _automatorServer_thread_id=automatorServer();
      if(_automatorServer_thread_id==NULL)
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't start %s (thread error) - %s\n", ERROR_STR, __func__, automator_server_name_str, notify_str);
         }
         mea_notify_printf('E', "Can't start %s - %s", automator_server_name_str, err_str);

         return -1;
      }
      _automatorServer_monitoring_id=my_id;
   }
   else
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't start %s (incorrect rules path).\n", ERROR_STR, __func__, automator_server_name_str);
      }
      mea_notify_printf('E', "Can't start %s - incorrect rules path", automator_server_name_str);
      return -1;
   }

   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, automator_server_name_str, launched_successfully_str);
   mea_notify_printf('S', "%s %s.", automator_server_name_str, launched_successfully_str);

   return 0;
}


int restart_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_automatorServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_automatorServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}



