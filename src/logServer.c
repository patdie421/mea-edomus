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

// pour la detection de changement d'un fichier voir :
// linux : http://www.ibm.com/developerworks/linux/library/l-inotify/
// mac os x : https://github.com/dmatveev/libinotify-kqueue (lib pour simuler inotify)

char *log_server_name_str="LOGSERVER"; // voir si utilisé

int _livelog_enable=0;


struct logServer_thread_data_s
{
   char *log_path;
   char *hostname;
   int port_socketdata;
};


// Variable globale privée
pthread_t *_logServer_thread=NULL;
volatile sig_atomic_t
           _logServer_thread_is_running=0;
int        _logServer_monitoring_id=-1;
struct logServer_thread_data_s
           _logServer_thread_data;

long logsent_indicator = 0;
long logsenterr_indicator = 0;

// pour envoyer les x dernieres lignes à la connexion d'un client
// prévoir une fonction send last x lines avec comme position de lecture la derniere valeur de pos (à mettre en globale donc)
// cette fonction est appeler par le consommateur avant l'initialisation du gestionnaire d'événement. Lette fonction, par l'intermédiaire d'un verrou,
// bloque toutes les lectures/emissions de lignes (encadrer fpget/mea_send_socket avec le verrou).
// elle lit ensuite les x dernieres lignes sans repositionner pos
// envoyer les x dernieres lignes au demandeur
// débloquer les envoies

void mea_livelog_enable()
{
   _livelog_enable=1;
}


void mea_livelog_disable()
{
   _livelog_enable=0;
}


void _set_logServer_isnt_running(void *data)
{
   FILE *fp=(FILE *)data;
   if(fp)
      fclose(fp);
   _logServer_thread_is_running=0;
}


void send_line(char *hostname, int port_socketdata, char *line)
{
   static int nodejs_socket = -1;
   
   if(nodejs_socket == -1)
      mea_socket_connect(&nodejs_socket, hostname, port_socketdata);
   
   char message[1024];
   int l_data=strlen(line)+4;

   sprintf(message,"$$$xxLOG:%s###", line);
   message[3]=(char)(l_data%128);
   message[4]=(char)(l_data/128);
   int ret = mea_socket_send(&nodejs_socket, message, l_data+12);
   if(ret<0)
   {
      process_update_indicator(_logServer_monitoring_id, "LOGSENTERR", ++logsenterr_indicator);
      close(nodejs_socket);
      if(mea_socket_connect(&nodejs_socket, hostname, port_socketdata)==-1)
         nodejs_socket=-1;
   }
   else
   {
      process_update_indicator(_logServer_monitoring_id, "LOGSENT", ++logsent_indicator);
   }
}


int read_string(char *line, int line_l, FILE *fp)
{
   int i=0;
   int timeout_counter=0;
   do
   {
      int c=fgetc(fp);
      if(c==EOF)
      {
         clearerr(fp);
         if(timeout_counter>10)
            return -3;
         usleep(100000);
         timeout_counter++;
      }
      else
      {
         timeout_counter=0;
         if(c<0)
         {
            perror("");
            return -2;
         }
         if(c==10)
         {
            line[i]=0;
            return i;
         }
         else
         {
            if(i<(line_l-1))
            {
               line[i]=(char)c;
               i++;
            }
            else
            {
               line[line_l-1]='\0';
               return -1;
            }
         }
      }
   }
   while(1);
}


long seek_to_previous_line(FILE *fp, long *pos)
{
   int found=0;
   int i=1;
   int nb_cars=0;
   long p=0;
   long end;

   fseek(fp,0,SEEK_END); // On va a la fin du fichier
   end=ftell(fp); // récupération de la position
   *pos=end;
   
   do
   {
      p = end-256*i; // on remonte d'un bloc
      if(p<0) // bloque trop court, on ajuste
      {
         nb_cars=256+p;
         p=0;
         fseek(fp,0,SEEK_SET); // on point au débug
      }
      else
      {
         nb_cars=256;
         fseek(fp, p, SEEK_SET);
      }
      
      for(int j=0;j<nb_cars;j++) // on lit jusqu'à trouver une fin de ligne
      {
         int c=fgetc(fp);
         if(c==EOF && ferror(fp))
         {
            return -1;
         }
         else if(c==EOF)
         {
            clearerr(fp);
            i++; // on remonte d'un bloc
            break;
         }
         else if(c==10)
         {
            if(i==1 && j==255 && found==0) // fin du premier bloc et rien trouvé jusqu'à présent, on a surement pas le début de la ligne
            {
               // on lira un deuxieme bloc
            }
            else
               found=1;
            *pos=ftell(fp); // on marque la possition
         }
      }
      i++;
      if(found)
         break;
   }
   while(p!=0);
   
   fseek(fp,*pos,SEEK_SET);
   return 0;
}



int read_line(FILE *fp, char *line, int line_l, long *pos)
{
   long ret=0;
   long p=0;
   
   if(*pos==-1)
   {
      seek_to_previous_line(fp, pos);
   }
   else
   {
      fseek(fp,0,SEEK_END);
      p=ftell(fp);
      
      if(p>*pos) // ancien position avant la fin du fichier
      {
         fseek(fp,*pos,SEEK_SET);
         ret=read_string(line, line_l, fp);
         if(ret<0)
            return -1; // erreur
         else
         {
            *pos+=ret+1;
            return 0; // ok pour prochaine lecture
         }
      }
      else if(p<*pos) // ancien position après la fin de la linge
      {
         *pos=-1; // on reinitialise tout
         return 2;
      }
      else
      {
         return 1; // fichier pas bougé
      }
   }
   return 0; // fin normal, lire prochaine ligne
}


void *logServer_thread(void *data)
{
   int exit=0;
   char log_file[256];
   mea_timer_t log_timer;
   FILE *fp=NULL;
   long pos=-1;
   
   struct logServer_thread_data_s *logServer_thread_data=(struct logServer_thread_data_s *)data;
   
   pthread_cleanup_push( (void *)_set_logServer_isnt_running, (void *)fp);
   _logServer_thread_is_running=1;

   _livelog_enable=1; // les log en live sont disponibles
   pthread_testcancel();
   process_heartbeat(_logServer_monitoring_id);

   snprintf(log_file, sizeof(log_file)-1, "%s/mea-edomus.log", logServer_thread_data->log_path);
   _livelog_enable=1;
   init_timer(&log_timer, 5, 1); // heartbeat toutes les 5 secondes
   start_timer(&log_timer);

   do
   {
      if(_livelog_enable==1)
      {
         char line[512];
            
         if(!fp)
         {
            fp = fopen(log_file, "r");
            if(!fp)
            {
               // traiter ici l'erreur
               perror("");
               sleep(1); // on attends un peu
            }
            pos=-1;
         }
            
         if(fp)
         {
            int ret=read_line(fp, line, sizeof(line), &pos);
            if(ret==0)
            {
               send_line(logServer_thread_data->hostname, logServer_thread_data->port_socketdata, line);
            }
            else if(ret==1)
            {
               usleep(500000);
            }
            else if(ret==2)
            {
               mea_log_printf("%s (%s) : log file changed\n", INFO_STR, __func__);
            }
            else
            {
               fclose(fp);
               fp=NULL;
               sleep(1);
            }
         }
      }
      pthread_testcancel();
      if(test_timer(&log_timer)==0)
         process_heartbeat(_logServer_monitoring_id);
   }
   while(exit==0);

   pthread_cleanup_pop(1);

   return NULL;
}


int stop_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=-1;
   if(_logServer_thread)
   {
      pthread_cancel(*_logServer_thread);
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
        if(_logServer_thread_is_running)
        {
           usleep(100); // will sleep for 1 ms
        }
        else
        {
           stopped=0;
           break;
        }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération\n", DEBUG_STR, __func__, log_server_name_str, 100-counter);
      
      free(_logServer_thread);
      _logServer_thread=NULL;
   }

   _livelog_enable=0;
   VERBOSE(1) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, log_server_name_str, stopped_successfully_str);
   mea_notify_printf('S', "%s %s (%d).",stopped_successfully_str, log_server_name_str, ret);

   return 0;
}


int start_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct logServerData_s *logServerData = (struct logServerData_s *)data;

   char err_str[80];

   _logServer_thread_data.log_path=logServerData->params_list[LOG_PATH];
   _logServer_thread_data.hostname=localhost_const;
   _logServer_thread_data.port_socketdata=atoi(logServerData->params_list[NODEJSDATA_PORT]);

   _logServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_logServer_thread)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         mea_log_printf("%s (%s) : malloc - %s",ERROR_STR,__func__,err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s", log_server_name_str, err_str);
      return -1;
   }
   
   if(pthread_create (_logServer_thread, NULL, logServer_thread, (void *)&_logServer_thread_data))
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - %s",ERROR_STR,__func__,err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s", log_server_name_str, err_str);

      return -1;
   }
   pthread_detach(*_logServer_thread);
   _logServer_monitoring_id=my_id;

   VERBOSE(1) mea_log_printf("%s  (%s) :  %s %s.\n", INFO_STR, __func__, log_server_name_str, launched_successfully_str);
   mea_notify_printf('S', "%s %s.", log_server_name_str, launched_successfully_str);
   return 0;
}


int restart_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_logServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_logServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}

