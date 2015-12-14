#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include "mea_verbose.h"

#include "mea_timer.h"

pthread_t *_timerServer_thread_id=NULL;

pthread_rwlock_t _timeServer_rwlock = PTHREAD_RWLOCK_INITIALIZER;


void *_timeServer_thread(void *data)
{
   while(1)
   {
      mea_microsleep(1000*100); // 100 ms
      
      pthread_testcancel();
   }
   
   pthread_exit(NULL);

   return NULL;
}


pthread_t *timeServer()
{
   pthread_t *timeServer_thread=NULL;

   timeServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!timeServer_thread)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto timeServer_clean_exit;
   }
   
   if(pthread_create (timeServer_thread, NULL, _timeServer_thread, NULL))
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
         perror("");
      }
      goto timeServer_clean_exit;
   }
   pthread_detach(*timeServer_thread);
   
   if(timeServer_thread)
      return timeServer_thread;

timeServer_clean_exit:
   if(timeServer_thread)
   {
      free(timeServer_thread);
      timeServer_thread=NULL;
   }

   return NULL;
}


int start_timeServer()
{
   _timerServer_thread_id=timeServer();
   
   if(_timerServer_thread_id==NULL)
      return -1;

   return 0;
}
