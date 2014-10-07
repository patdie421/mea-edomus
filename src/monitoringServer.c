#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // close
#include <netdb.h> // gethostbyname
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define PORT 5600

#include "globals.h"
#include "queue.h"
#include "debug.h"
#include "monitoringServer.h"


struct process_indicator_s
{
   char name[41];
   long value;
}


struct monitored_process_s
{
   char name[41];
   time_t last_heartbeat;
   int heartbeat_interval; // second
   queue *indicators_list;
};


struct monitored_processes_s
{
   int max_processes;
   struct monitored_process_s *processes_table;
} monitored_processes;


struct monitoring_thread_data_s
{
   char *log_path;
   char *hostname;
   int port_socketdata;
} monitoring_thread_data;


pthread_t *_monitoring_thread=NULL;

pid_t pid_nodejs=0;

const char *hostname = "localhost";


int clear_monitored_processes_list(struct monitored_processes_s *monitored_processes)
{
   for(int i=0;i<max_nb_processes;i++)
   {
      unregister_process(i);
   }
   return 0;
}


int init_monitored_processes_list(struct monitored_processes_s *monitored_processes, int max_nb_processes)
{
   monitored_processes->processes_table=(struct monitored_processes_s *)malloc(max_nb_processes * sizeof(struct monitored_processes_s);
   if(!monitored_processes->processes_table)
   {
      return -1;
   }
   for(int i=0;i<max_nb_processes;i++)
      monitored_processes->processes_table[i]=NULL;
   max_processes = max_nb_processes;
}


int register_process(char *name)
{
   for(i=0;monitored_processes.max_processes;i++)
   {
      if(!monitored_processes.processes_table[i])
      {
         monitored_processes.processes_table[i]=(struct monitored_process_s *)malloc(sizeof(struct monitored_process_s));
         if(!monitored_processes.processes_table[i])
            return -1;
         else
         {
            strncpy(monitored_processes.processes_table[i]->name, name, 40);
            last_heartbeat=time();
            queue_init(indicator_list);
            return i;
         }
      }
   }
   return -1;
}


int unregister_process(int id)
{
   if(monitored_processes.processes_table[id])
   {
      free(monitored_processes.processes_table[id);
      monitored_processes.processes_table[i]=NULL;
      return 0;
   }
   return -1;
}


int send_Heartbeat(int id)
{
   if(monitored_processes.processes_table[id])
   {
      monitored_processes.processes_table[id]->heartbeat = time();
   }
}


int process_add_indicator(int id, char *name, long initial_value)
{
   send_Heartbeat(int id);

}


int process_del_indicator(int id, char *name)
{
   send_Heartbeat(int id);

}


int process_update_indicator(int id, char *name, long value)
{
   send_Heartbeat(int id);

   monitored_processes_send_indicator(&monitored_processes_list, id);
}


int monitored_processes_send_indicator(struct monitored_processes_s *monitored_processes, int id)
{
}


int monitored_processes_send_all_indicators(struct monitored_processes_s *monitored_processes)
{
}


int monitored_processes_run(struct monitored_processes_s *monitored_processes)
{
}


int connexion(int *s, char *hostname, int port)
{
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *serv_info = NULL;

   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock < 0)
   {
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) :  socket - can't create : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   serv_info = gethostbyname(hostname); // on récupère les informations de l'hôte auquel on veut se connecter
   if(serv_info == NULL)
   {
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) :  gethostbyname - can't get information : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(port);
   bcopy((char *)serv_info->h_addr, (char *)&serv_addr.sin_addr.s_addr, serv_info->h_length);
   
   if(connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) :  connect - can't connect : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   *s=sock;
   return 0;
}


int envoie(int *s, char *message)
{
   if(send(*s, message, strlen(message), 0) < 0)
   {
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) :  send - can't send : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   return 0;
}


int seek_to_prevs_lines(FILE *fp, int nb_lines)
{
   char buff[512];
   long pos=0;
   int n=0;

   if(nb_lines < 1)
      return -1;


   fseek(fp, 0, SEEK_END);

   long max=ftell(fp);
   pos=max;

   do
   {
      int nb_read=0;

      pos=pos-sizeof(buff);
      if(pos<0)
      {
         nb_read=sizeof(buff)+pos;
         pos=0;
      }
      else
      {
         nb_read=sizeof(buff);
      }

      fread(buff, nb_read, 1, fp);

      for(int i=nb_read;i;i--)
      {
         if( (buff[i-1]=='\r') && ((pos+i) != 0) )
         {
            n++;
            if(n==nb_lines)
            {
               fseek(fp,pos+i,SEEK_SET);
               return 0;
            }
        }
     }
  }
  while(pos>0);

  fseek(fp, pos, SEEK_END);
  return 0;
}


int readAndSendLine(int nodejs_socket, char *file, long *pos)
{
   FILE *fp=NULL;
   char line[512];
   int nb_loop=0;

/*
   struct stat fileStat;
   time_t mtime;

   stat(file, &fileStat);
   if(fileStat.st_mtime == mtime)
   { // aucun changement
      return 0;
   }
   mtime=fileStat.st_mtime;
*/
   fp = fopen(file, "r");
   if(fp == NULL)
   {
      VERBOSE(1) {
         fprintf(stderr, "%s (%s) :  fopen - can't open %s : ",ERROR_STR,__func__,file);
         perror("");
      }
      *pos=0; // le fichier n'existe pas. lorsqu'il sera créé on le lira depuis le debut
//      mtime=0;
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
         char message[512];

         sprintf(message,"LOG:%s",line);
         int ret = envoie(&nodejs_socket, message);
         if(ret<0)
         {
            ret=-1;
            break;
         }
         nb_lopp++;
      }
   }
   if(fp)
      fclose(fp);

   return ret;
}


void *monitoring_thread(void *data)
{
   int exit=0;
   int nodejs_socket=-1;
   long pos = -1;
   char log_file[256];

   struct monitoring_thread_data_s *d=(struct monitoring_thread_data_s *)data;
   sprintf(log_file,"%s/mea-edomus.log", d->log_path);

   do
   {
     if(connexion(&nodejs_socket, (char *)(d->hostname), 5600)==0)
     {
       int nb_loop=0;
       do
       {
         // monitored_processes_run(&monitored_processes); // mettre un timer à 10 secondes

         int ret=readAndSendLine(nodejs_socket, log_file, &pos);

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
       // monitored_processes_run(&monitored_processes); mettre un timer à 10 secondes
       sleep(1); // on essayera de se reconnecter dans 1 secondes
     }
   }
   while(exit==0);
   
   return NULL;
}


pid_t start_nodejs(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *phpsession_path)
{
   pid_t nodejs_pid = -1;
   nodejs_pid = fork();

   if (nodejs_pid == 0) // child
   {
      // Code only executed by child process
      char str_port_socketio[80];
      char str_port_socketdata[80];
      char str_phpsession_path[80];
      
      char *params[6];

      sprintf(str_port_socketio,"--iosocketport=%d",port_socketio);
      sprintf(str_port_socketdata,"--dataport=%d",port_socketdata);
      sprintf(str_phpsession_path,"--phpsession_path=%s",phpsession_path);

      params[0]="nodejs";
      params[1]=eventServer_path;
      params[2]=str_port_socketio;
      params[3]=str_port_socketdata;
      params[4]=str_phpsession_path;
      params[5]=0;

      execvp(nodejs_path, params);

      perror("");

      exit(1);
   }

   else if (nodejs_pid < 0)
   { // failed to fork
      perror("");
      return -1;
   }
   // Code only executed by parent process
   return nodejs_pid;
}



void stop_nodejs()
{
   int status;

   if(pid_nodejs>1)
   {
      kill(pid_nodejs, SIGKILL);
      wait(&status);
      pid_nodejs=-1;
   }
}


void stop_monitoringServer()
{
   if(_monitoring_thread)
   {
      pthread_cancel(*_monitoring_thread);
      pthread_join(*_monitoring_thread, NULL);
      free(_monitoring_thread);
      _monitoring_thread=NULL;
   }
   stop_nodejs();
}


//pthread_t *start_monitoringServer(char *nodejs_path, char *eventServer_path, int port_socketdata, char *log_path)
pthread_t *start_monitoringServer(char **params_list)
{
   pid_t pid;
   if(params_list[NODEJSIOSOCKET_PORT] == NULL ||
      params_list[NODEJSDATA_PORT] == NULL     ||
      params_list[NODEJS_PATH] == NULL         ||
      params_list[LOG_PATH] == NULL)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : parameters error ...",ERROR_STR,__func__);
      }
      return NULL;
   }

   int socketio_port = atoi(params_list[NODEJSIOSOCKET_PORT]);
   int socketdata_port = atoi(params_list[NODEJSDATA_PORT]);
   char *nodejs_path = params_list[NODEJS_PATH];
   char *phpsession_path = params_list[PHPSESSIONS_PATH];
   char serverjs_path[256];

   int n=snprintf(serverjs_path, sizeof(serverjs_path), "%s/nodeJS/server/server", params_list[GUI_PATH]);
   if(n<0 || n==sizeof(serverjs_path))
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return NULL; 
   }

   pid=start_nodejs(nodejs_path, serverjs_path, socketio_port, socketdata_port, phpsession_path);
   if(!pid)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : can't start nodejs",ERROR_STR,__func__);
      }
      return NULL;
   }
   
   monitoring_thread_data.log_path=params_list[LOG_PATH];
   monitoring_thread_data.hostname="localhost";
   monitoring_thread_data.port_socketdata=socketdata_port;

   _monitoring_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_monitoring_thread)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - ",ERROR_STR,__func__);
         perror("");
      }
      return NULL;
   }
   if(pthread_create (_monitoring_thread, NULL, monitoring_thread, (void *)&monitoring_thread_data))
   {
      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }

   pid_nodejs=pid;
   return _monitoring_thread;
}

