//
//  interfaces.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 04/11/2013.
//
//

#include <stdio.h>
#include <termios.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <signal.h>

#include "string_utils.h"
#include "queue.h"
#include "debug.h"
#include "xPL.h"
#include "sockets_utils.h"

#include "processManager.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

queue_t *_interfaces=NULL;

pthread_rwlock_t interfaces_queue_rwlock;


uint32_t speeds[][3]={
   {   300,    B300},
   {  1200,   B1200},
   {  4800,   B4800},
   {  9600,   B9600},
   { 19200,  B19200},
   { 38400,  B38400},
   { 57600,  B57600},
   {115200, B115200},
   {0,0}
};


int32_t get_speed_from_speed_t(speed_t speed)
{
   for(int16_t i=0;speeds[i][0];i++)
   {
      if(speeds[i][1]==speed)
         return speeds[i][0];
   }
   return -1;
}


int16_t get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed)
{
   *speed=0;
   char _dev[41];
   char reste[41];
   char vitesse[41];

   char *_dev_ptr;
   char *reste_ptr;
   char *vitesse_ptr;
   char *end=NULL;

   int16_t n=sscanf(device,"SERIAL://%40[^:]%40[^/r/n]",_dev,reste);
   if(n<=0)
      return -1;

   _dev_ptr=mea_strtrim(_dev);

   if(n==1)
   {
      *speed=B9600;
   }
   else
   {
      uint32_t v;

      reste_ptr=mea_strtrim(reste);
      n=sscanf(reste_ptr,":%40[^/n/r]",vitesse);
      if(n!=1)
         return -1;

      vitesse_ptr=mea_strtrim(vitesse);
      v=strtol(vitesse_ptr,&end,10);
      if(end==vitesse || *end!=0 || errno==ERANGE)
         return -1;

      *speed=0;
      int i;
      for(i=0;speeds[i][0];i++)
      {
         if(speeds[i][0]==v)
         {
            *speed=speeds[i][1];
            break;
         }
      }
      if(*speed==0)
         return -1;
   }

   strncpy(dev,_dev_ptr,dev_l-1);

   return 0;
}


void dispatchXPLMessageToInterfaces(xPL_ServicePtr theService, xPL_MessagePtr theMessage)
{
   int ret;

   interfaces_queue_elem_t *iq;

   VERBOSE(9) fprintf(stderr,"%s  (%s) : Reception message xPL\n",INFO_STR,__func__);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
   
   if(_interfaces && _interfaces->nb_elem)
   {
      first_queue(_interfaces);
      while(1)
      {
         current_queue(_interfaces, (void **)&iq);
         switch (iq->type)
         {
            case INTERFACE_TYPE_001:
            {
               interface_type_001_t *i001 = (interface_type_001_t *)(iq->context);
               if(i001->monitoring_id>-1 && process_is_running(i001->monitoring_id) && i001->xPL_callback)
                  i001->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i001);
               break;
            }

            case INTERFACE_TYPE_002:
            {
               interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
               if(i002->xPL_callback)
                  i002->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i002);
               break;
            }
            default:
               break;
         }
         ret=next_queue(_interfaces);
         if(ret<0)
            break;
      }
   }
   
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
}


void stop_interfaces()
{
   interfaces_queue_elem_t *iq;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);

   if(_interfaces && _interfaces->nb_elem)
   {
      first_queue(_interfaces);
      while(_interfaces->nb_elem)
      {
         out_queue_elem(_interfaces, (void **)&iq);
         switch (iq->type)
         {
            case INTERFACE_TYPE_001:
            {
               interface_type_001_t *i001=(interface_type_001_t *)(iq->context);
               
               if(i001->xPL_callback)
                  i001->xPL_callback=NULL;

               if(i001->monitoring_id!=-1)
               {
                  struct interface_type_001_Data_s *interface_type_001Data = process_get_data_ptr(i001->monitoring_id);

                  process_stop(i001->monitoring_id, NULL, 0);
                  process_unregister(i001->monitoring_id);
                  i001->monitoring_id=-1;
                  if(interface_type_001Data)
                  {
                     free(interface_type_001Data);
                     interface_type_001Data=NULL;
                  }
               } 

               free(i001);
               i001=NULL;
               break;
            }
            case INTERFACE_TYPE_002:
            {
               interface_type_002_t *i002=(interface_type_002_t *)(iq->context);
               
               if(i002->xPL_callback)
                  i002->xPL_callback=NULL;
               
               if(i002->monitoring_id!=-1)
               {
                  struct interface_type_002_Data_s *interface_type_002Data = process_get_data_ptr(i002->monitoring_id);

                  process_stop(i002->monitoring_id, NULL, 0);
                  process_unregister(i002->monitoring_id);
                  i002->monitoring_id=-1;
                  if(interface_type_002Data)
                  {
                     free(interface_type_002Data);
                     interface_type_002Data=NULL;
                  }
               } 

               free(i002);
               i002=NULL;
               break;
            }
         
            default:
               break;
         }
         free(iq);
         iq=NULL;
      }
      free(_interfaces);
      _interfaces=NULL;
   }
   
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0); 
   
   return;
}


queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db)
{
   char sql[255];
   sqlite3_stmt * stmt;
   int16_t ret;
   int sortie=0;
   interfaces_queue_elem_t *iq;

   pthread_rwlock_init(&interfaces_queue_rwlock, NULL);
   
   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      sqlite3_close(sqlite3_param_db);
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      return NULL;
   }

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);

   _interfaces=(queue_t *)malloc(sizeof(queue_t));
   if(!_interfaces)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      sqlite3_close(sqlite3_param_db);
      goto start_interfaces_clean_exit;
   }

   init_queue(_interfaces);
   while (1)
   {
      int s = sqlite3_step (stmt); // sqlite function need int
      if (s == SQLITE_ROW)
      {
         int16_t id_interface;
         int16_t id_type;
         const unsigned char *dev;
         const unsigned char *parameters;
         const unsigned char *name;
         const unsigned char *description;
         int16_t state;
         
         id_interface = sqlite3_column_int(stmt, 1);
         id_type = sqlite3_column_int(stmt, 2);
         name = sqlite3_column_text(stmt, 3);
         description = sqlite3_column_text(stmt, 4);
         dev = sqlite3_column_text(stmt, 5);
         parameters = sqlite3_column_text(stmt, 6);
         state = sqlite3_column_int(stmt, 7);
         
         if(state==1)
         {
            switch(id_type)
            {
               case INTERFACE_TYPE_001:
               {
                  interface_type_001_t *i001;
                  
                  // allocation du contexte de l'inteface
                  i001=(interface_type_001_t *)malloc(sizeof(interface_type_001_t));
                  if(!i001)
                  {
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror("");
                     }
                     break;
                  }

                  // initialisation contexte de l'interface
                  i001->thread_is_running=0;
                  struct interface_type_001_start_stop_params_s *i001_start_stop_params=(struct interface_type_001_start_stop_params_s *)malloc(sizeof(struct interface_type_001_start_stop_params_s));
                  if(!i001_start_stop_params)
                  {
                     free(i001);
                     i001=NULL;
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror("");
                     }
                     break;
                  } 
                  i001_start_stop_params->i001=i001;
                  i001_start_stop_params->sqlite3_param_db = sqlite3_param_db;
                  strncpy(i001_start_stop_params->dev, (char *)dev, sizeof(i001_start_stop_params->dev)-1);
                  strncpy(i001->name, (char *)name, sizeof(i001->name)-1);
                  i001->monitoring_id=0;
                  i001->loaded=0;
                  i001->indicators.nbactuatorsout = 0;
                  i001->indicators.nbactuatorsxplrecv = 0;
                  i001->indicators.nbactuatorsouterr = 0;
                  i001->indicators.nbsensorstraps = 0;
                  i001->indicators.nbsensorsread = 0;
                  i001->indicators.nbsensorsreaderr = 0;
                  i001->indicators.nbsensorsxplsent = 0;
                  i001->indicators.nbsensorsxplrecv = 0;
                  i001->indicators.nbcounterstraps = 0;
                  i001->indicators.nbcountersread = 0;
                  i001->indicators.nbcountersreaderr = 0;
                  i001->indicators.nbcountersxplsent = 0;
                  i001->indicators.nbcountersxplrecv = 0;
                  i001->indicators.nbxplin = 0;
                  i001->interface_id=id_interface;
                  
                  // initialisation du process
                  i001->monitoring_id=process_register((char *)name);
                  process_set_group(i001->monitoring_id, 1);
                  process_set_start_stop(i001->monitoring_id, start_interface_type_001, stop_interface_type_001, (void *)i001_start_stop_params, 1);
                  process_set_watchdog_recovery(i001->monitoring_id, start_interface_type_001, (void *)i001_start_stop_params);
                  process_set_description(i001->monitoring_id, (char *)description);
                  
                  process_add_indicator(i001->monitoring_id, I001_XPLINNB, 0);
                  process_add_indicator(i001->monitoring_id, I001_STNBRAP, 0);
                  process_add_indicator(i001->monitoring_id, I001_SNBREAD, 0);
                  process_add_indicator(i001->monitoring_id, I001_SNBREADERR, 0);
                  process_add_indicator(i001->monitoring_id, I001_SNBXPLOUT, 0);
                  process_add_indicator(i001->monitoring_id, I001_SNBXPLIN, 0);
                  process_add_indicator(i001->monitoring_id, I001_ANBOUTERR, 0);
                  process_add_indicator(i001->monitoring_id, I001_ANBOUT, 0);
                  process_add_indicator(i001->monitoring_id, I001_ANBXPLIN, 0);
                  process_add_indicator(i001->monitoring_id, I001_CTNBRAP, 0);
                  process_add_indicator(i001->monitoring_id, I001_CNBREAD, 0);
                  process_add_indicator(i001->monitoring_id, I001_CNBREADERR, 0);
                  process_add_indicator(i001->monitoring_id, I001_CNBXPLOUT, 0);
                  process_add_indicator(i001->monitoring_id, I001_CNBXPLIN, 0);
 
                  // lancement du process
                  ret=process_start(i001->monitoring_id, NULL, 0);

                  iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
                  iq->type=id_type;
                  iq->context=i001;
                  in_queue_elem(_interfaces, iq);
                  break;
               }
                  
               case INTERFACE_TYPE_002:
               {
                  interface_type_002_t *i002;
                  
                  i002=(interface_type_002_t *)malloc(sizeof(interface_type_002_t));
                  if(!i002)
                  {
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror(""); }
                     break;
                  }
                  i002->thread_is_running=0;
                  
                  struct interface_type_002_data_s *i002_start_stop_params=(struct interface_type_002_data_s *)malloc(sizeof(struct interface_type_002_data_s));
                  if(!i002_start_stop_params)
                  {
                     free(i002);
                     i002=NULL;
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror("");
                     }  
                     break;
                  }
                  strncpy(i002->dev, (char *)dev, sizeof(i002->dev)-1);
                  strncpy(i002->name, (char *)name, sizeof(i002->name)-1);
                  i002->id_interface=id_interface;
                  i002->monitoring_id=process_register((char *)name);

                  process_set_group(i002->monitoring_id, 1);
                  i002_start_stop_params->sqlite3_param_db = sqlite3_param_db;
                  i002_start_stop_params->parameters = (char *)parameters;
                  i002_start_stop_params->i002=i002;

                  process_set_start_stop(i002->monitoring_id, start_interface_type_002, stop_interface_type_002, (void *)i002_start_stop_params, 1);
                  process_set_watchdog_recovery(i002->monitoring_id, restart_interface_type_002, (void *)i002_start_stop_params);
                  process_set_description(i002->monitoring_id, (char *)description);
                  process_set_heartbeat_interval(i002->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat
                  ret=process_start(i002->monitoring_id, NULL, 0);

                  interfaces_queue_elem_t *iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
                  iq->type=id_type;
                  iq->context=i002;
                  in_queue_elem(_interfaces, iq);
                  break;
               }
                  
               default:
                  break;
            }
         }
         else
         {
            VERBOSE(9) fprintf(stderr,"%s  (%s) : %s not activated (state = %d)\n",INFO_STR,__func__,dev,state);
         }
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_finalize(stmt);
         sqlite3_close(sqlite3_param_db);
         goto start_interfaces_clean_exit;
      }
   }
   sortie=1;

start_interfaces_clean_exit:
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0); 

   if(sortie==0)
   {
      stop_interfaces(); // stop fait le free de interfaces.
   }

   return _interfaces;
}


int send_reload( char *hostname, int port)
{
   int ret;
   int s;
   char reload_str[80];
   
   if(mea_socket_connect(&s, hostname, port)<0)
      return -1;
   fprintf(stderr,"send_reload %d\n",s);
   
   int reload_str_l=strlen(reload_str)+4;
   char message[2048];
   sprintf(message,"$$$%c%cREL:%s###", (char)(reload_str_l%128), (char)(reload_str_l/128), reload_str);

   ret = mea_socket_send(&s, message, reload_str_l+12);

   close(s);

   return ret;
}


int restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct interfacesServerData_s *interfacesServerData = (struct interfacesServerData_s *)data;

   stop_interfaces();
   sleep(1);
   start_interfaces(interfacesServerData->params_list, interfacesServerData->sqlite3_param_db);
   send_reload("localhost",5600);
   return 0;
}

