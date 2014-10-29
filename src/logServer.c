//
//  logServer.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "logServer.h"

#include "globals.h"
#include "queue.h"
#include "debug.h"
#include "processManager.h"
#include "timer.h"
#include "sockets_utils.h"
#include "string_utils.h"
#include "consts.h"
#include "notify.h"

pthread_t *_logServer_thread=NULL;
int _logServer_monitoring_id=-1;

struct logServer_thread_data_s
{
   char *log_path;
   char *hostname;
   int port_socketdata;
} logServer_thread_data;

int _readAndSendLine(int nodejs_socket, char *file, long *pos)
{
   FILE *fp=NULL;
   char line[4096];
   int nb_loop=0;

   fp = fopen(file, "r");
   if(fp == NULL)
   {
      VERBOSE(5) {
         fprintf(stderr, "%s (%s) :  fopen - can't open %s : ", ERROR_STR, __func__, file);
         perror("");
      }
      *pos=0; // le fichier n'existe pas. lorsqu'il sera créé on le lira depuis le debut
      return 0;
   }

   fseek(fp,0,SEEK_END);
   if(*pos>=0)
   {
      long current=ftell(fp);

      if(*pos > current) // le fichier a diminué, on le relie depuis le debut
      {
         fseek(fp, 0, SEEK_SET);
         *pos=0;
      }
      else // le fichier a grossie ou est resté identique (en taille).
      {
         if(current != *pos) // il a grossie
            fseek(fp, *pos, SEEK_SET);
      }
   }

   int ret=0;
   while (1)
   {
      pthread_testcancel();
      if (fgets(line, sizeof(line), fp) == NULL || nb_loop >= 20)
      {
         // plus de ligne à lire ou déjà 20 ligne transmise on rend la mains, on mémorise la dernière position connue
         *pos=ftell(fp);
         // et on s'arrete pour l'instant
         if(nb_loop >= 20)
            ret=1; // pour dire qu'il ne faudra pas attendre ...
         break; 
      }
      else
      {
         char message[1024];
         int l_data=strlen(line)+4;

         sprintf(message,"$$$xxLOG:%s###", line);
         message[3]=(char)(l_data%128);
         message[4]=(char)(l_data/128);
         int ret = mea_socket_send(&nodejs_socket, message, l_data+12);
         if(ret<0)
         {
            ret=-1;
            break;
         }
         nb_loop++;
      }
   }
   if(fp)
      fclose(fp);

   return ret;
}


void *logServer_thread(void *data)
{
   int exit=0;
   int nodejs_socket=-1;
   long pos = -1;
   char log_file[256];
   mea_timer_t log_timer;
   
   init_timer(&log_timer, 10, 1);
   start_timer(&log_timer);
   
   struct logServer_thread_data_s *d=(struct logServer_thread_data_s *)data;
   sprintf(log_file,"%s/mea-edomus.log", d->log_path);

   do
   {
      process_heartbeat(_logServer_monitoring_id);
      pthread_testcancel();

      if( mea_socket_connect(&nodejs_socket, (char *)(d->hostname), (d->port_socketdata))==0)
      {
         do
         {
            pthread_testcancel();
            if(test_timer(&log_timer)==0)
               process_heartbeat( _logServer_monitoring_id);
            
            int ret=_readAndSendLine(nodejs_socket, log_file, &pos);

            if(ret==-1)
            {
               VERBOSE(9) {
                  fprintf(stderr, "%s (%s) : connection error - retry next time\n", INFO_STR, __func__);
                  break; // erreur de com. on essayera de se reconnecter au prochain tour.
               }
            }
            if(ret!=1)
               sleep(1);
         }
         while(1);

         close(nodejs_socket);
      }
      else
      {
         VERBOSE(9) {
            fprintf(stderr, "%s (%s) : connection error - retry next time\n", INFO_STR, __func__);
         }
         sleep(5); // on essayera de se reconnecter dans 1 secondes
      }
   }
   while(exit==0);
   
   return NULL;
}


int stop_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=-1;
   if(_logServer_thread)
   {
      int count=5; // 5 secondes pour s'arrêter
      pthread_cancel(*_logServer_thread);
      while(count)
      {
         if(pthread_kill(*_logServer_thread, 0) == 0)
         {
            sleep(1);
            count--;
         }
         else
         {
            ret=0;
            break;
         }
      }
//      pthread_join(*_logServer_thread, NULL);

      free(_logServer_thread);
      _logServer_thread=NULL;
   }

   VERBOSE(1) fprintf(stderr,"%s (%s) : LOGSERVER  stopped successfully.\n", INFO_STR, __func__);
   mea_notify_printf('S', "LOGSERVER  stopped successfully (%d).",ret);

   return 0;
}


int start_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct logServerData_s *logServerData = (struct logServerData_s *)data;

   char err_str[80];

   logServer_thread_data.log_path=logServerData->params_list[LOG_PATH];
   logServer_thread_data.hostname=localhost_const;
   logServer_thread_data.port_socketdata=atoi(logServerData->params_list[NODEJSDATA_PORT]);

   _logServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_logServer_thread)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - %s",ERROR_STR,__func__,err_str);
      }
      mea_notify_printf('E', "LOGSERVER can't be launched - %s", err_str);
      return -1;
   }
   if(pthread_create (_logServer_thread, NULL, logServer_thread, (void *)&logServer_thread_data))
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) : pthread_create - can't start thread - %s",ERROR_STR,__func__,err_str);
      }
      mea_notify_printf('E', "LOGSERVER can't be launched - %s", err_str);

      return -1;
   }
   _logServer_monitoring_id=my_id;

   VERBOSE(1) fprintf(stderr,"%s (%s) : LOGSERVER launched successfully.\n", INFO_STR, __func__);
   mea_notify_printf('S', "LOGSERVER launched successfully.");
   return 0;
}


