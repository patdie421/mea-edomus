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

#define PORT 5600 // à remplacer par le port déclaré

#include "globals.h"
#include "queue.h"
#include "debug.h"
#include "monitoringServer.h"
#include "timer.h"


struct monitoring_thread_data_s
{
   char *log_path;
   char *hostname;
   int port_socketdata;
} monitoring_thread_data;

struct monitored_processes_s monitored_processes;

pthread_t *_monitoring_thread=NULL;

pid_t pid_nodejs=0;

const char *hostname = "localhost";


void _indicators_free_queue_elem(void *e)
{
}


struct monitored_processes_s *get_monitored_processes_descriptor()
{
   return &monitored_processes;
}


int process_register(struct monitored_processes_s *monitored_processes, char *name)
{
   int ret=0;
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&monitored_processes->lock );
   pthread_mutex_lock(&monitored_processes->lock);  

   for(int i=0;i<monitored_processes->max_processes;i++)
   {
      if(!monitored_processes->processes_table[i])
      {
         monitored_processes->processes_table[i]=(struct monitored_process_s *)malloc(sizeof(struct monitored_process_s));
         if(!monitored_processes->processes_table[i])
         {
            ret=-1;
            goto register_process_clean_exit;
         }
         else
         {
            strncpy(monitored_processes->processes_table[i]->name, name, 40);
            monitored_processes->processes_table[i]->last_heartbeat=time(NULL);
            monitored_processes->processes_table[i]->indicators_list=(queue_t *)malloc(sizeof(queue_t));
            init_queue(monitored_processes->processes_table[i]->indicators_list);
            monitored_processes->processes_table[i]->heartbeat_interval=20;
            ret=i;
            goto register_process_clean_exit;
         }
      }
   }
   ret=-1;
register_process_clean_exit:
   pthread_mutex_unlock(&monitored_processes->lock); 
   pthread_cleanup_pop(0); 
   return ret;
}


int process_unregister(struct monitored_processes_s *monitored_processes, int id)
{
   int ret=-1;
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock) );
   pthread_mutex_lock(&(monitored_processes->lock));  

   if(monitored_processes->processes_table[id])
   {
      clear_queue(monitored_processes->processes_table[id]->indicators_list, _indicators_free_queue_elem);
      free(monitored_processes->processes_table[id]);
      monitored_processes->processes_table[id]=NULL;
      ret=0;
   }

   pthread_mutex_unlock(&monitored_processes->lock); 
   pthread_cleanup_pop(0); 

   return ret;
}


int _strncat2(char *dest, int max_dest, char *source)
{
   int l_dest = strlen(dest);
   int l_source = strlen(source);
   if((l_dest+lsource)>max_test)
      return -1;
   else
   {
      strcat(dest,source);
   }
}


int _monitored_processes_send_indicators(struct monitored_processes_s *monitored_processes, int id, char *s, int s_l)
{
   struct process_indicator_s *e;
   char buff[256];
   
   s[0]=0;
   if(monitored_processes->processes_table[id])
   {
   time_t now = time(NULL);

      DEBUG_SECTION fprintf(stderr, "{heartbeat:");
      if(_strncat2(s, s_l, "{heartbeat:")<0)
         return -1;
      
      if((now - monitored_processes->processes_table[id]->last_heartbeat)<30)
      {
         DEBUG_SECTION fprintf(stderr, "\"OK"\");
         if(_strncat2(s, s_l, "\"OK\"")<0)
            return -1;
      }
      else
      {
         DEBUG_SECTION fprintf(stderr, "\"KO"\");
         if(_strncat2(s, s_l, "\"KO\"")<0)
            return -1;
      }

      if(first_queue(monitored_processes->processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(monitored_processes->processes_table[id]->indicators_list, (void **)&e)==0)
            {
               DEBUG_SECTION fprintf(stderr,",\"%s\":%ld",e->name,e->value);
               int n=snprintf(buff,sizeof(buff),",\"%s\":%ld",e->name,e->value);
               if(n<0 || n==sizeof(buff))
                  return -1;
               if(_strncat2(s,s_l,buff)<0)
                  return -1;
               next_queue(monitored_processes->processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }

      DEBUG_SECTION fprintf(stderr, "}");
      if(_strncat2(s,s_l,"}")<0)
         return -1;
   }
   return 0;
}


int _monitored_processes_send_all_indicators(struct monitored_processes_s *monitored_processes, char *hostname, int port)
{
   char buff[256];
   char json[2048];
   char message[2048];
   
   json[0]=0;
   
   DEBUG_SECTION {
      fprintf(stderr, "%s (%s) :  json message - ",DEBUG_STR,__func__);
   }
   
   DEBUG_SECTION fprintf(stderr, "{");
   if(_strncat2(json, sizeof(json), "{")<0)
      return -1;
   int flag=0;
   for(int i=0;i<monitored_processes->max_processes;i++)
   {
      if(monitored_processes->processes_table[i])
      {
         if(flag==1)
         {
            DEBUG_SECTION fprintf(stderr,",");
            if(_strncat2(json, sizeof(json), ",")<0)
               return -1;
         }
         DEBUG_SECTION fprintf(stderr,"\"%s\":",monitored_processes->processes_table[i]->name);
         int n=snprintf(buff,sizeof(buff),"\"%s\":",monitored_processes->processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;

         if(_strncat2(json,sizeof(json),buff)<0)
            return -1;

         flag=1;
         if(_monitored_processes_send_indicators(monitored_processes, i, buff, sizeof(buff))<0)
            return -1;
         if(strncat2(json,sizeof(json),buff)<0)
            return -1;
      }
   }
   DEBUG_SECTION fprintf(stderr, "}\n");
   if(_strncat2(json,sizeof(json),"}\n")<0)
      return -1;

   strcpy(message,"MON:");
   if(_strncat2(message,sizeof(message),json)<0)
      return -1;

   int sock;
   int ret;
   if(connexion(&sock, hostname, port)<0)
      return -1;
   int ret = envoie(&sock, message);
   close(sock);
   
   return return ret;
}


int _clear_monitored_processes_list(struct monitored_processes_s *monitored_processes)
{
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock));
   pthread_mutex_lock(&monitored_processes->lock);  

   for(int i=0;i<monitored_processes->max_processes;i++)
      process_unregister(monitored_processes,i);

   pthread_mutex_unlock(&(monitored_processes->lock)); 
   pthread_cleanup_pop(0); 

   return 0;
}


int _init_monitored_processes_list(struct monitored_processes_s *monitored_processes, int max_nb_processes)
{
   monitored_processes->processes_table=(struct monitored_process_s **)malloc(max_nb_processes * sizeof(struct monitored_process_s));
   if(!monitored_processes->processes_table)
   {
      return -1;
   }
   for(int i=0;i<max_nb_processes;i++)
      monitored_processes->processes_table[i]=NULL;

   monitored_processes->max_processes = max_nb_processes;
   init_timer(&monitored_processes->timer, 5, 1);
   start_timer(&monitored_processes->timer);

   pthread_mutex_init(&(monitored_processes->lock), NULL);

   return 0;
}


int process_heartbeat(struct monitored_processes_s *monitored_processes, int id)
{
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock) );
   pthread_mutex_lock(&(monitored_processes->lock));  

   if(monitored_processes->processes_table[id])
   {
      monitored_processes->processes_table[id]->last_heartbeat = time(NULL);
   }

   pthread_mutex_unlock(&monitored_processes->lock); 
   pthread_cleanup_pop(0);
   
   return 0;
}


int process_add_indicator(struct monitored_processes_s *monitored_processes, int id, char *name, long initial_value)
{
   int ret=-1;

   process_heartbeat(monitored_processes, id);

   struct process_indicator_s *e;

   e=(struct process_indicator_s *)malloc(sizeof(struct process_indicator_s));
   if(e)
   {
      strncpy(e->name,name,40);
      e->value=initial_value;
   }
   else
     return -1;

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock) );
   pthread_mutex_lock(&(monitored_processes->lock));

   if(monitored_processes->processes_table[id])
   {
      in_queue_elem(monitored_processes->processes_table[id]->indicators_list,e);
      ret=0;
   }
   else
      ret=-1;

   pthread_mutex_unlock(&(monitored_processes->lock)); 
   pthread_cleanup_pop(0); 

   return ret;
}


int process_del_indicator(struct monitored_processes_s *monitored_processes, int id, char *name)
{
   process_heartbeat(monitored_processes, id);// à compléter

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock) );
   pthread_mutex_lock(&(monitored_processes->lock));
   
   // à faire
   
   pthread_mutex_unlock(&monitored_processes->lock);
   pthread_cleanup_pop(0);
   
   return 0;
}


int process_update_indicator(struct monitored_processes_s *monitored_processes, int id, char *name, long value)
{
   int ret=-1;
   process_heartbeat(monitored_processes, id);

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock) );
   pthread_mutex_lock(&(monitored_processes->lock));

   if(monitored_processes->processes_table[id])
   {
      struct process_indicator_s *e;

      if(first_queue(monitored_processes->processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(monitored_processes->processes_table[id]->indicators_list, (void **)&e)==0)
            {
               if(strcmp(name, e->name) == 0)
               {
                  e->value=value;
                  ret=0;
                  break;
               }
               else
                  next_queue(monitored_processes->processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }
   }

   pthread_mutex_unlock(&(monitored_processes->lock));
   pthread_cleanup_pop(0); 

   return ret;
}


int _monitored_processes_run(struct monitored_processes_s *monitored_processes, char *hostname, int port)
{
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes->lock) );
   pthread_mutex_lock(&(monitored_processes->lock));  

   if(!test_timer(&monitored_processes->timer))
   {
      _monitored_processes_send_all_indicators(monitored_processes, hostname, port);
   }

   pthread_mutex_unlock(&(monitored_processes->lock)); 
   pthread_cleanup_pop(0);
   
   return 0;
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


int readAndSendLine(int nodejs_socket, char *file, long *pos)
{
   FILE *fp=NULL;
   char line[512];
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
         char message[512];

         sprintf(message,"LOG:%s",line);
         int ret = envoie(&nodejs_socket, message);
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
     if(connexion(&nodejs_socket, (char *)(d->hostname), (d->socketdata)==0)
     {
       do
       {
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

      params[0]="mea-edomus[nodejs]";
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


int monitoringServer_indicators_loop(char *hostname, int port)
{
   if(_monitoring_thread)
   {
      return _monitored_processes_run(&monitored_processes, hostname, port);
   }
   return -1;
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

   _init_monitored_processes_list(&monitored_processes, 40);
   
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
