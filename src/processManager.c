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
#include <unistd.h>

#include "globals.h"
#include "queue.h"
#include "debug.h"
#include "processManager.h"
#include "timer.h"
#include "sockets_utils.h"
#include "string_utils.h"


struct managed_processes_s menaged_processes;


int _indicator_exist(int id, char *name)
{
   int ret=-1;
   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_rdlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id])
   {
      struct process_indicator_s *e;

      if(first_queue(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               if(strcmp(name, e->name) == 0)
               {
                  ret=0;
                  break;
               }
               else
                  next_queue(managed_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int _managed_processes_process_to_json(int id, char *s, int s_l, int flag)
{
   struct process_indicator_s *e;
   char buff[256];
   
   s[0]=0;
   if(id>=0 && managed_processes.processes_table[id])
   {
      time_t now = time(NULL);

      if(mea_strncat(s, s_l, "{")<0)
         return -1;

      if(flag && managed_processes.processes_table[id]->status==RUNNING)
      {
         if(mea_strncat(s, s_l, "\"heartbeat\":")<0)
            return -1;
   
         if((now - managed_processes.processes_table[id]->last_heartbeat)<30)
         {
            if(mea_strncat(s, s_l, "\"OK\"")<0)
               return -1;
         }
         else
         {
            if(mea_strncat(s, s_l, "\"KO\"")<0)
               return -1;
         }
         
         if(mea_strncat(s, s_l, ",")<0)
            return -1;
      }
      
      int n=snprintf(buff,sizeof(buff),"\"pid\":%d",id);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;
      
      n=snprintf(buff,sizeof(buff),",\"status\":%d",managed_processes.processes_table[id]->status);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;
      
      n=snprintf(buff,sizeof(buff),",\"type\":%d",managed_processes.processes_table[id]->type);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;

      n=snprintf(buff,sizeof(buff),",\"group\":%d",managed_processes.processes_table[id]->group_id);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;

      if(flag && first_queue(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               int n=snprintf(buff,sizeof(buff),",\"%s\":%ld",e->name,e->value);
               if(n<0 || n==sizeof(buff))
                  return -1;
               if(mea_strncat(s,s_l,buff)<0)
                  return -1;
               next_queue(managed_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }

      if(mea_strncat(s,s_l,"}")<0)
         return -1;
   }
   return 0;
}


//int _managed_processes_send_all_indicators(char *hostname, int port)
int _managed_processes_processes_to_json(char *message, int l_message)
{
   char buff[512];
   char json[2048];
   
   json[0]=0;
   
   if(mea_strncat(json, sizeof(json), "{ ")<0)
      return -1;
   int flag=0;
   for(int i=0;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i])
      {
         if(flag==1)
         {
            if(mea_strncat(json, sizeof(json), ", ")<0)
               return -1;
         }
         int n=snprintf(buff,sizeof(buff),"\"%s\":",managed_processes.processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;

         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;

         flag=1;
         if(_managed_processes_process_to_json(i, buff, sizeof(buff), 1)<0)
            return -1;
         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;
      }
   }
   if(mea_strncat(json,sizeof(json)," }\n")<0)
      return -1;

   strcpy(message,"");
   if(mea_strncat(message,l_message,json)<0)
      return -1;

   return 0;
}


void _indicators_free_queue_elem(void *e)
{
}


int managed_processes_processes_to_json_mini(char *json, int l_json)
{
   char buff[512];
   json[0]=0;
   
   if(mea_strncat(json, l_json-1, "{ ")<0)
      return -1;
   int flag=0;
   for(int i=0;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i])
      {
         if(flag==1)
         {
            if(mea_strncat(json, l_json, ", ")<0)
               return -1;
         }
         int n=snprintf(buff,sizeof(buff),"\"%s\":",managed_processes.processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;

         if(mea_strncat(json,l_json-1,buff)<0)
            return -1;

         flag=1;
         if(_managed_processes_process_to_json(i, buff, sizeof(buff), 0)<0)
            return -1;
         if(mea_strncat(json,l_json-1,buff)<0)
            return -1;
      }
   }
   if(mea_strncat(json,l_json-1," }\n")<0)
      return -1;
   
   return 0;
}


int process_set_status(int id, process_status_t status)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   if(id<0 || !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->status=status;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_type(int id, process_type_t type)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 || !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->type=type;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_group(int id, int group_id)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 || !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->group_id=group_id;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_start_stop(int id, process_start_stop_f start, process_start_stop_f stop, void *start_stop_data, int auto_restart)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 || !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->type=AUTOSTART;
      managed_processes.processes_table[id]->status=STOPPED;
      managed_processes.processes_table[id]->start=start;
      managed_processes.processes_table[id]->stop=stop;
      managed_processes.processes_table[id]->start_stop_data=start_stop_data;
   }
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_register(char *name)
{
   int ret=0;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   for(int i=0;i<managed_processes.max_processes;i++)
   {
      if(!managed_processes.processes_table[i])
      {
         managed_processes.processes_table[i]=(struct process_s *)malloc(sizeof(struct process_s));
         if(!managed_processes.processes_table[i])
         {
            ret=-1;
            goto register_process_clean_exit;
         }
         else
         {
            strncpy(managed_processes.processes_table[i]->name, name, sizeof(managed_processes.processes_table[i]->name)-1);
            managed_processes.processes_table[i]->last_heartbeat=time(NULL);
            managed_processes.processes_table[i]->indicators_list=(queue_t *)malloc(sizeof(queue_t));
            init_queue(managed_processes.processes_table[i]->indicators_list);
            managed_processes.processes_table[i]->heartbeat_interval=20;
            managed_processes.processes_table[i]->type=AUTOSTART;
            managed_processes.processes_table[i]->status=STOPPED; // arrêté par défaut
            managed_processes.processes_table[i]->start=NULL;
            managed_processes.processes_table[i]->stop=NULL;
            managed_processes.processes_table[i]->start_stop_data=NULL;
            managed_processes.processes_table[i]->group_id=DEFAULTGROUP;
            ret=i;
            goto register_process_clean_exit;
         }
      }
   }
   ret=-1;
register_process_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_unregister(int id)
{
   int ret=-1;
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id])
   {
      clear_queue(managed_processes.processes_table[id]->indicators_list, _indicators_free_queue_elem);
      free(managed_processes.processes_table[id]);
      managed_processes.processes_table[id]=NULL;
      ret=0;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int clear_managed_processes()
{
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   for(int i=0;i<managed_processes.max_processes;i++)
      process_unregister(i);

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return 0;
}


int init_processes_manager(int max_nb_processes)
{
   managed_processes.processes_table=(struct process_s **)malloc(max_nb_processes * sizeof(struct process_s *));
   if(!managed_processes.processes_table)
   {
      return -1;
   }

   for(int i=0;i<max_nb_processes;i++)
   {
      managed_processes.processes_table[i]=NULL;
   }

   managed_processes.max_processes = max_nb_processes;
   init_timer(&managed_processes.timer, 5, 1);
   start_timer(&managed_processes.timer);

   pthread_rwlock_init(&(managed_processes.rwlock), NULL);

   return 0;
}


int process_heartbeat(int id)
{
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id >= 0 && managed_processes.processes_table[id])
   {
      managed_processes.processes_table[id]->last_heartbeat = time(NULL);
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 
   
   return 0;
}


int process_add_indicator(int id, char *name, long initial_value)
{
   struct process_indicator_s *e = NULL;

   if(id<0)
      return 0;
   else
      process_heartbeat(id);

   if(_indicator_exist(id, name)==0)
      return 0;

   e=(struct process_indicator_s *)malloc(sizeof(struct process_indicator_s));
   if(e)
   {
      strncpy(e->name, name, 40);
      e->value=initial_value;
   }
   else
     return -1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   in_queue_elem(managed_processes.processes_table[id]->indicators_list,e);

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return 0;
}


int process_del_indicator(int id, char *name)
{
   if(id<0)
      return -1;
   
   process_heartbeat(id);// à compléter

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   // à faire
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 
   
   return 0;
}


int process_update_indicator(int id, char *name, long value)
{
   int ret=-1;
   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id])
   {
      struct process_indicator_s *e;

      if(first_queue(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(current_queue(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               if(strcmp(name, e->name) == 0)
               {
                  e->value=value;
                  ret=0;
                  break;
               }
               else
                  next_queue(managed_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int managed_processes_refresh_now(char *hostname, int port)
{
   int ret;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_rdlock(&managed_processes.rwlock);

   char json[2048];
   int sock;
   
   _managed_processes_processes_to_json(json, sizeof(json)-1);
   
   if(mea_socket_connect(&sock, hostname, port)<0)
   {
      ret=-1;
      goto managed_processes_refresh_now_clean_exit;
   }
   
   char message[2048];
   int l_data=strlen(json)+4;
   sprintf(message,"$$$%c%cMON:%s###", (char)(l_data%128), (char)(l_data/128), json);
   
   ret = mea_socket_send(&sock, message, l_data+12);
   close(sock);

managed_processes_refresh_now_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 
   
   return ret;
}


int _managed_processes_run(char *hostname, int port)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_rdlock(&managed_processes.rwlock);

   if(!test_timer(&managed_processes.timer))
   {
      char json[2048];
      int sock;
      
      _managed_processes_processes_to_json(json, sizeof(json)-1);
      
      if(mea_socket_connect(&sock, hostname, port)<0)
      {
         ret=-1;
         goto _managed_processes_run_clean_exit;
      }

      char message[2048];
      int l_data=strlen(json)+4;
      sprintf(message,"$$$%c%cMON:%s###", (char)(l_data%128), (char)(l_data/128), json);
   
      ret = mea_socket_send(&sock, message, l_data+12);
      
      close(sock);
   }

_managed_processes_run_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 
   
   return ret;
}


int managed_processes_loop(char *hostname, int port)
{
   return _managed_processes_run(hostname, port);
}


int process_is_running(int id)
{
   int ret=-1;
   
   if(id<0)
      return -1;
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_rdlock(&managed_processes.rwlock);

   if(managed_processes.processes_table[id])
   {
      ret=managed_processes.processes_table[id]->status;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}

int process_start(int id, char *errmsg, int l_errmsg)
{
   int ret=1;
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->status==STOPPED &&
      managed_processes.processes_table[id]->type!=NOTMANAGED)
   {
      if(managed_processes.processes_table[id]->start)
      {
         ret=managed_processes.processes_table[id]->start(id, managed_processes.processes_table[id]->start_stop_data, errmsg, l_errmsg);
         if(ret<0)
            managed_processes.processes_table[id]->status=STOPPED;
         else
         {
            managed_processes.processes_table[id]->status=RUNNING;
         }
      }
      else
         return -1;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   process_heartbeat(id);

   return ret;
}


struct task_thread_data_s
{
   int id;
   process_start_stop_f task;
   void *task_data;
};


void *_task_thread(void *data)
{
   struct task_thread_data_s *task_thread_data=(struct task_thread_data_s *)data;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   managed_processes.processes_table[id]->status=RUNNING;
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   task_threak_data->task(task_threak_data->id, task_threak_data->task_data, NULL, 0);
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   managed_processes.processes_table[id]->status=STOPPED;
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   if(data)
   {
      free(data);
      data=NULL;
   }   
}


int process_task(int id, char *errmsg, int l_errmsg)
{
   int ret=1;
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->type==TASK)
   {
      if(managed_processes.processes_table[id]->start)
      {
         struct task_tread_data_s *task_threak_data=(struct task_tread_data_s *)malloc(sizeof(struct struct task_tread_data_s *));
         task_threak_data->task=managed_processes.processes_table[id]->start;
         task_threak_data->task_data=managed_processes.processes_table[id]->start_stop_data;
         task_threak_data->id=id;
         pthread_t process_task_thread;
         if(pthread_create (process_task_thread, NULL, _task_thread, (void *)task_threak_data))
         {
            VERBOSE(2) fprintf(stderr, "%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
            if(errmsg)
            {
               snprintf(errmsg, l_errmsg, "internal error (pthread_create)");
               ret=-1;
               goto process_task_clean_exit;
            }
         }
         pthread_detach(process_task_thread);
      }
   }

process_task_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_stop(int id, char *errmsg, int l_errmsg)
{
   int ret=1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->status==RUNNING &&
      managed_processes.processes_table[id]->type!=NOTMANAGED)
   {
      managed_processes.processes_table[id]->status=0;
      ret=managed_processes.processes_table[id]->stop(id, managed_processes.processes_table[id]->start_stop_data, errmsg, l_errmsg);
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 
   
   return ret;
}


void* process_get_data_ptr(int id)
{
   void *ptr=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 && managed_processes.processes_table[id])
   {
      ptr=managed_processes.processes_table[id]->start_stop_data;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ptr;
}
