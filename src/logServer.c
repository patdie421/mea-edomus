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
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) :  fopen - can't open %s : ",ERROR_STR,__func__,file);
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

         sprintf(message,"LOG:%s",line);
         int ret = mea_socket_send(&nodejs_socket, message);
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
      if(mea_socket_connect(&nodejs_socket, (char *)(d->hostname), (d->port_socketdata))==0)
      {
         do
         {
            if(test_timer(&log_timer)==0)
               process_heartbeat( _logServer_monitoring_id);
            
            int ret=_readAndSendLine(nodejs_socket, log_file, &pos);

            if(ret==-1)
               break; // erreur de com. on essaye de se reconnecter au prochain tour.

            if(ret!=1)
               sleep(1);
         }
         while(1);

         close(nodejs_socket);
      }
      else
      {
         VERBOSE(9) {
            fprintf(stderr, "%s (%s) : connexion - retry next time ...\n",INFO_STR,__func__);
         }
         sleep(5); // on essayera de se reconnecter dans 1 secondes
      }
   }
   while(exit==0);
   
   return NULL;
}


int stop_logServer(int my_id, void *data)
{
   if(_logServer_thread)
   {
      pthread_cancel(*_logServer_thread);
      pthread_join(*_logServer_thread, NULL);
      free(_logServer_thread);
      _logServer_thread=NULL;
   }
   return 0;
}


int start_logServer(int my_id, void *data)
{
   struct logServerData_s *logServerData = (struct logServerData_s *)data;

   logServer_thread_data.log_path=logServerData->params_list[LOG_PATH];
   logServer_thread_data.hostname="localhost";
   logServer_thread_data.port_socketdata=atoi(logServerData->params_list[NODEJSDATA_PORT]);

   _logServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_logServer_thread)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   if(pthread_create (_logServer_thread, NULL, logServer_thread, (void *)&logServer_thread_data))
   {
      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return -1;
   }
   _logServer_monitoring_id=my_id;

   return 0;
}


