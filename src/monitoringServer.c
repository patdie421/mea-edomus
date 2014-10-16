#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "globals.h"
#include "queue.h"
#include "debug.h"
#include "monitoringServer.h"
#include "timer.h"
#include "sockets_utils.h"
#include "string_utils.h"

struct monitored_processes_s monitored_processes;

const char *hostname = "localhost";

void _indicators_free_queue_elem(void *e)
{
}

//struct monitored_processes_s *get_monitored_processes_descriptor()
//{
//   return &monitored_processes;
//}


int process_set_start_stop(int id,  process_start_stop_f start, process_start_stop_f stop, void *start_stop_data, int auto_restart)
{
   if(id<0 || !monitored_processes.processes_table[id])
      return -1;

   monitored_processes.processes_table[id]->enable_autorestart=auto_restart;
   monitored_processes.processes_table[id]->start=start;
   monitored_processes.processes_table[id]->stop=stop;
   monitored_processes.processes_table[id]->start_stop_data=start_stop_data;

   return 0;
}


int process_register(char *name)
{
   int ret=0;
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&monitored_processes.lock );
   pthread_mutex_lock(&monitored_processes.lock);

   for(int i=0;i<monitored_processes.max_processes;i++)
   {
      if(!monitored_processes.processes_table[i])
      {
         monitored_processes.processes_table[i]=(struct monitored_process_s *)malloc(sizeof(struct monitored_process_s));
         if(!monitored_processes.processes_table[i])
         {
            ret=-1;
            goto register_process_clean_exit;
         }
         else
         {
            strncpy(monitored_processes.processes_table[i]->name, name, 40);
            monitored_processes.processes_table[i]->last_heartbeat=time(NULL);
            monitored_processes.processes_table[i]->indicators_list=(queue_t *)malloc(sizeof(queue_t));
            init_queue(monitored_processes.processes_table[i]->indicators_list);
            monitored_processes.processes_table[i]->heartbeat_interval=20;
            monitored_processes.processes_table[i]->enable_autorestart=0;
            monitored_processes.processes_table[i]->status=0; // arrêté par défaut
            monitored_processes.processes_table[i]->start=NULL;
            monitored_processes.processes_table[i]->stop=NULL;
            monitored_processes.processes_table[i]->start_stop_data=NULL;
            ret=i;
            goto register_process_clean_exit;
         }
      }
   }
   ret=-1;
register_process_clean_exit:
   pthread_mutex_unlock(&monitored_processes.lock);
   pthread_cleanup_pop(0); 
   return ret;
}


int process_unregister(int id)
{
   int ret=-1;
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));

   if(monitored_processes.processes_table[id])
   {
      clear_queue(monitored_processes.processes_table[id]->indicators_list, _indicators_free_queue_elem);
      free(monitored_processes.processes_table[id]);
      monitored_processes.processes_table[id]=NULL;
      ret=0;
   }

   pthread_mutex_unlock(&monitored_processes.lock);
   pthread_cleanup_pop(0); 

   return ret;
}


int _monitored_processes_send_indicators(int id, char *s, int s_l)
{
   struct process_indicator_s *e;
   char buff[256];
   
   s[0]=0;
   if(monitored_processes.processes_table[id])
   {
      time_t now = time(NULL);

      DEBUG_SECTION fprintf(stderr, "{\"heartbeat\":");
      if(mea_strncat(s, s_l, "{\"heartbeat\":")<0)
         return -1;

      if((now - monitored_processes.processes_table[id]->last_heartbeat)<30)
      {
         DEBUG_SECTION fprintf(stderr, "\"OK\"");
         if(mea_strncat(s, s_l, "\"OK\"")<0)
            return -1;
      }
      else
      {
         DEBUG_SECTION fprintf(stderr, "\"KO\"");
         if(mea_strncat(s, s_l, "\"KO\"")<0)
            return -1;
      }

      DEBUG_SECTION fprintf(stderr,",\"pid\":%d",id);
      int n=snprintf(buff,sizeof(buff),",\"pid\":%d",id);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;
      
      DEBUG_SECTION fprintf(stderr,",\"status\":%d",monitored_processes.processes_table[id]->status);
      n=snprintf(buff,sizeof(buff),",\"status\":%d",monitored_processes.processes_table[id]->status);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;
      
      if(first_queue(monitored_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(monitored_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               DEBUG_SECTION fprintf(stderr,",\"%s\":%ld",e->name,e->value);
               int n=snprintf(buff,sizeof(buff),",\"%s\":%ld",e->name,e->value);
               if(n<0 || n==sizeof(buff))
                  return -1;
               if(mea_strncat(s,s_l,buff)<0)
                  return -1;
               next_queue(monitored_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }

      DEBUG_SECTION fprintf(stderr, "}");
      if(mea_strncat(s,s_l,"}")<0)
         return -1;
   }
   return 0;
}


int _monitored_processes_send_all_indicators(char *hostname, int port)
{
   char buff[256];
   char json[2048];
   char message[2048];
   
   json[0]=0;
   
   DEBUG_SECTION {
      fprintf(stderr, "%s (%s) :  json message - ",DEBUG_STR,__func__);
   }
   
   DEBUG_SECTION fprintf(stderr, "{");
   if(mea_strncat(json, sizeof(json), "{")<0)
      return -1;
   int flag=0;
   for(int i=0;i<monitored_processes.max_processes;i++)
   {
      if(monitored_processes.processes_table[i])
      {
         if(flag==1)
         {
            DEBUG_SECTION fprintf(stderr,",");
            if(mea_strncat(json, sizeof(json), ",")<0)
               return -1;
         }
         DEBUG_SECTION fprintf(stderr,"\"%s\":",monitored_processes.processes_table[i]->name);
         int n=snprintf(buff,sizeof(buff),"\"%s\":",monitored_processes.processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;

         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;

         flag=1;
         if(_monitored_processes_send_indicators(i, buff, sizeof(buff))<0)
            return -1;
         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;
      }
   }
   DEBUG_SECTION fprintf(stderr, "}\n");
   if(mea_strncat(json,sizeof(json),"}\n")<0)
      return -1;

   strcpy(message,"MON:");
   if(mea_strncat(message,sizeof(message),json)<0)
      return -1;

   int sock;
   int ret;
   if(mea_socket_connect(&sock, hostname, port)<0)
      return -1;
   ret = mea_socket_send(&sock, message);
   close(sock);
   
   return ret;
}


int clear_monitored_processes_list()
{
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock));
   pthread_mutex_lock(&monitored_processes.lock);

   for(int i=0;i<monitored_processes.max_processes;i++)
      process_unregister(i);

   pthread_mutex_unlock(&(monitored_processes.lock));
   pthread_cleanup_pop(0); 

   return 0;
}


int init_monitored_processes_list(int max_nb_processes)
{
   monitored_processes.processes_table=(struct monitored_process_s **)malloc(max_nb_processes * sizeof(struct monitored_process_s *));
   if(!monitored_processes.processes_table)
   {
      return -1;
   }
   for(int i=0;i<max_nb_processes;i++)
      monitored_processes.processes_table[i]=NULL;

   monitored_processes.max_processes = max_nb_processes;
   init_timer(&monitored_processes.timer, 5, 1);
   start_timer(&monitored_processes.timer);

   pthread_mutex_init(&(monitored_processes.lock), NULL);

   return 0;
}


int process_heartbeat(int id)
{
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));

   if(id >= 0 && monitored_processes.processes_table[id])
   {
      monitored_processes.processes_table[id]->last_heartbeat = time(NULL);
   }

   pthread_mutex_unlock(&monitored_processes.lock);
   pthread_cleanup_pop(0);
   
   return 0;
}


int _indicator_exist(int id, char *name)
{
   int ret=-1;
   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));

   if(monitored_processes.processes_table[id])
   {
      struct process_indicator_s *e;

      if(first_queue(monitored_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(monitored_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               if(strcmp(name, e->name) == 0)
               {
                  ret=0;
                  break;
               }
               else
                  next_queue(monitored_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }
   }

   pthread_mutex_unlock(&(monitored_processes.lock));
   pthread_cleanup_pop(0); 

   return ret;
}


int process_add_indicator(int id, char *name, long initial_value)
{
   struct process_indicator_s *e = NULL;

   if(!monitored_processes.processes_table[id])
      return 0;
   else
      process_heartbeat(id);

   if(_indicator_exist(id, name)==0)
      return 0;

   e=(struct process_indicator_s *)malloc(sizeof(struct process_indicator_s));
   if(e)
   {
      strncpy(e->name,name,40);
      e->value=initial_value;
   }
   else
     return -1;

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));

   in_queue_elem(monitored_processes.processes_table[id]->indicators_list,e);

   pthread_mutex_unlock(&(monitored_processes.lock));
   pthread_cleanup_pop(0); 

   return 0;
}


int process_del_indicator(int id, char *name)
{
   process_heartbeat(id);// à compléter

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));
   
   // à faire
   
   pthread_mutex_unlock(&monitored_processes.lock);
   pthread_cleanup_pop(0);
   
   return 0;
}


int process_update_indicator(int id, char *name, long value)
{
   int ret=-1;
   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));

   if(monitored_processes.processes_table[id])
   {
      struct process_indicator_s *e;

      if(first_queue(monitored_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(monitored_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               if(strcmp(name, e->name) == 0)
               {
                  e->value=value;
                  ret=0;
                  break;
               }
               else
                  next_queue(monitored_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }
   }

   pthread_mutex_unlock(&(monitored_processes.lock));
   pthread_cleanup_pop(0); 

   return ret;
}


int _monitored_processes_run(char *hostname, int port)
{
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(monitored_processes.lock) );
   pthread_mutex_lock(&(monitored_processes.lock));

   if(!test_timer(&monitored_processes.timer))
   {
      _monitored_processes_send_all_indicators(hostname, port);
   }

   pthread_mutex_unlock(&(monitored_processes.lock));
   pthread_cleanup_pop(0);
   
   return 0;
}


int monitoringServer_loop(char *hostname, int port)
{
   return _monitored_processes_run(hostname, port);
}


int process_start(int id)
{
   if(id>=0 && monitored_processes.processes_table[id] && monitored_processes.processes_table[id]->status==0)
   {
      monitored_processes.processes_table[id]->status=1;
      return monitored_processes.processes_table[id]->start(id, monitored_processes.processes_table[id]->start_stop_data);
   }
   else
      return -1;

   return 0;
}


int process_stop(int id)
{
   if(id>=0 && monitored_processes.processes_table[id] && monitored_processes.processes_table[id]->status==1)
   {
      monitored_processes.processes_table[id]->status=0;
      return monitored_processes.processes_table[id]->stop(id, monitored_processes.processes_table[id]->start_stop_data);
   }
   else
      return -1;
   
   return 0;
}


int process_set_not_managed(int id)
{
   if(monitored_processes.processes_table[id])
   {
      monitored_processes.processes_table[id]->status=2;
   }
   else
      return -1;
   
   return 0;
}
