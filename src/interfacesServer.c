//
//  interfaces.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 04/11/2013.
//
//
#include <Python.h>

#include <stdio.h>
#include <termios.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <signal.h>
#include <sqlite3.h>

#include <dlfcn.h>

#include "globals.h"

#include "mea_string_utils.h"
#include "mea_queue.h"
#include "mea_verbose.h"
#include "mea_sockets_utils.h"
#include "consts.h"

#include "cJSON.h"

#include "processManager.h"
#include "nodejsServer.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"
#include "interface_type_003.h"
#include "interface_type_004.h"
#include "interfaces/type_005/interface_type_005.h"
#include "interface_type_006.h"

struct interfacesServer_interfaceFns_s interfacesFns[7];

mea_queue_t *_interfaces=NULL;

pthread_rwlock_t interfaces_queue_rwlock;

char *sql_select_device_info="SELECT \
sensors_actuators.id_sensor_actuator, \
sensors_actuators.id_location, \
sensors_actuators.state, \
sensors_actuators.parameters, \
types.parameters, \
sensors_actuators.id_type, \
lower(sensors_actuators.name), \
lower(interfaces.name), \
interfaces.id_type, \
(SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), \
interfaces.dev, \
sensors_actuators.todbflag, \
types.typeoftype, \
sensors_actuators.id_interface \
FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type" ;

char *sql_select_interface_info="SELECT * FROM interfaces";

void *interface_type_005_lib = NULL;


int send_reload( char *hostname, int port)
{
   int ret;
   int s;
   char reload_str[80];
   
   if(mea_socket_connect(&s, hostname, port)<0)
      return -1;
   
   int reload_str_l=strlen(reload_str)+4;
   char message[2048];
   sprintf(message,"$$$%c%cREL:%s###", (char)(reload_str_l%128), (char)(reload_str_l/128), reload_str);

   ret = mea_socket_send(&s, message, reload_str_l+12);

   close(s);

   return ret;
}


void dispatchXPLMessageToInterfaces2(cJSON *xplMsgJson)
{
   int ret;

   interfaces_queue_elem_t *iq;

   DEBUG_SECTION mea_log_printf("%s (%s) : Reception message xPL\n", INFO_STR, __func__);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);

   if(_interfaces && _interfaces->nb_elem)
   {
      mea_queue_first(_interfaces);
      while(1)
      {
         mea_queue_current(_interfaces, (void **)&iq);

         int i=0;

         for(;interfacesFns[i].get_type;i++)
         {
            int type=interfacesFns[i].get_type();
            if(type==iq->type)
            {
               int monitoring_id = interfacesFns[i].get_monitoring_id(iq->context);
               if(monitoring_id>-1 && process_is_running(monitoring_id))
               { 
                  xpl2_f xPL_callback2 = interfacesFns[i].get_xPLCallback(iq->context);
                  if(xPL_callback2)
                     xPL_callback2(xplMsgJson, iq->context);
               }
               break;
            }
         }

         ret=mea_queue_next(_interfaces);
         if(ret<0)
            break;
      }
   }
   
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);

   cJSON_Delete(xplMsgJson);
}


void stop_interfaces()
{
   interfaces_queue_elem_t *iq;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);

   if(_interfaces && _interfaces->nb_elem)
   {
      mea_queue_first(_interfaces);
      while(_interfaces->nb_elem)
      {
         mea_queue_out_elem(_interfaces, (void **)&iq);

         int i=0;
         for(;interfacesFns[i].get_type;i++)
         {
            int type=interfacesFns[i].get_type();
            if(type==iq->type)
            {
               int monitoring_id = interfacesFns[i].get_monitoring_id(iq->context);
               if(monitoring_id>-1 && process_is_running(monitoring_id))
               {
                  interfacesFns[i].set_xPLCallback(iq->context, NULL);

                  int monitoring_id = interfacesFns[i].get_monitoring_id(iq->context);

                  if(monitoring_id!=-1)
                  {
                     void *start_stop_params = process_get_data_ptr(monitoring_id);

                     process_stop(monitoring_id, NULL, 0);
                     process_unregister(monitoring_id);

                     interfacesFns[i].set_monitoring_id(iq->context, -1);

                     if(start_stop_params)
                     {
                        free(start_stop_params);
                        start_stop_params=NULL;
                     }
                  }
                  interfacesFns[i].clean(iq->context);
                  free(iq->context);
                  iq->context=NULL;
               }
               break;
            }
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


int16_t load_interfaces_fns(char **params_list)
{
/*
   interfacesFns[0].malloc_and_init_interface = (malloc_and_init_interface_f)&malloc_and_init_interface_type_001;
   interfacesFns[0].get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_001;
   interfacesFns[0].get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_001;
   interfacesFns[0].clean = (clean_f)&clean_interface_type_001;
   interfacesFns[0].set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_001;
   interfacesFns[0].set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_001;
   interfacesFns[0].get_type = (get_type_f)&get_type_interface_type_001;

   interfacesFns[1].malloc_and_init_interface = (malloc_and_init_interface_f)&malloc_and_init_interface_type_002;
   interfacesFns[1].get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_002;
   interfacesFns[1].get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_002;
   interfacesFns[1].clean = (clean_f)&clean_interface_type_002;
   interfacesFns[1].set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_002;
   interfacesFns[1].set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_002;
   interfacesFns[1].get_type = (get_type_f)&get_type_interface_type_002;

   interfacesFns[2].malloc_and_init_interface = (malloc_and_init_interface_f)&malloc_and_init_interface_type_003;
   interfacesFns[2].get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_003;
   interfacesFns[2].get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_003;
   interfacesFns[2].clean = (clean_f)&clean_interface_type_003;
   interfacesFns[2].set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_003;
   interfacesFns[2].set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_003;
   interfacesFns[2].get_type = (get_type_f)&get_type_interface_type_003;

   interfacesFns[3].malloc_and_init_interface = (malloc_and_init_interface_f)&malloc_and_init_interface_type_004;
   interfacesFns[3].get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_004;
   interfacesFns[3].get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_004;
   interfacesFns[3].clean = (clean_f)&clean_interface_type_004;
   interfacesFns[3].set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_004;
   interfacesFns[3].set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_004;
   interfacesFns[3].get_type = (get_type_f)&get_type_interface_type_004;

   interfacesFns[4].malloc_and_init_interface = (malloc_and_init_interface_f)&malloc_and_init_interface_type_005;
   interfacesFns[4].get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_005;
   interfacesFns[4].get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_005;
   interfacesFns[4].clean = (clean_f)&clean_interface_type_005;
   interfacesFns[4].set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_005;
   interfacesFns[4].set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_005;
   interfacesFns[4].get_type = (get_type_f)&get_type_interface_type_005;

   interfacesFns[5].malloc_and_init_interface = (malloc_and_init_interface_f)&malloc_and_init_interface_type_006;
   interfacesFns[5].get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_006;
   interfacesFns[5].get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_006;
   interfacesFns[5].clean = (clean_f)&clean_interface_type_006;
   interfacesFns[5].set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_006;
   interfacesFns[5].set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_006;
   interfacesFns[5].get_type = (get_type_f)&get_type_interface_type_006;
*/
#ifdef ASPLUGIN
   struct interfacesServer_interfaceFns_s fns;
   get_fns_interface_f fn = NULL;
   char interface_so[256];

   snprintf(interface_so, sizeof(interface_so)-1,"%s/%s", params_list[PLUGINS_PATH], "interface_type_005.so");

   interface_type_005_lib = dlopen(interface_so, RTLD_NOW);
   if(interface_type_005_lib)
   {
      char *err;

      fn=dlsym(interface_type_005_lib, "get_fns_interface");
      err = dlerror();
      if(err!=NULL)
      {
         fprintf(stderr,"ERROR: %s\n", err);
      }
   }
   else
   {
      fprintf(stderr,"lib not loaded: %s\n", dlerror());
   }
#endif

   get_fns_interface_type_001(&(interfacesFns[0]));
   get_fns_interface_type_002(&(interfacesFns[1]));
   get_fns_interface_type_003(&(interfacesFns[2]));
   get_fns_interface_type_004(&(interfacesFns[3]));
#ifdef ASPLUGIN
   fn(interface_type_005_lib, &(interfacesFns[4]));
   interfacesFns[4].lib = interface_type_005_lib; 
#else
   get_fns_interface_type_005(&(interfacesFns[4]));
#endif
   get_fns_interface_type_006(&(interfacesFns[5]));

   
   interfacesFns[6].get_type = NULL;
}


mea_queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db)
{
   char sql[255];
   sqlite3_stmt * stmt;
   int16_t ret;
   int sortie=0;
   interfaces_queue_elem_t *iq;

   pthread_rwlock_init(&interfaces_queue_rwlock, NULL);
  
   load_interfaces_fns(params_list);
 
   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      sqlite3_close(sqlite3_param_db);
      VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      return NULL;
   }

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);

   _interfaces=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!_interfaces)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      sqlite3_close(sqlite3_param_db);
      goto start_interfaces_clean_exit;
   }

   mea_queue_init(_interfaces);
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
            iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
            if(iq==NULL)
            {
               goto start_interfaces_clean_exit;
            }
            iq->context = NULL;

            int monitoring_id=-1;
            int i=0;
            for(;interfacesFns[i].get_type;i++)
            {
               int type=interfacesFns[i].get_type();
               if(type==id_type)
               {
                  void *ptr = interfacesFns[i].malloc_and_init_interface(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)parameters, (char *)description);
                  if(ptr)
                  {
                     iq->context=ptr;
                     monitoring_id = interfacesFns[i].get_monitoring_id(ptr);                      
                  }
                  break;
               }
            }

            if(monitoring_id!=-1)
            {
               iq->type=id_type;
               mea_queue_in_elem(_interfaces, iq);
               process_start(monitoring_id, NULL, 0);
            }
            else
            {
               free(iq);
               iq=NULL;
            }
         }
         else
         {
            VERBOSE(9) mea_log_printf("%s (%s) : %s not activated (state = %d)\n", INFO_STR, __func__, dev, state);
         }
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
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


int restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct interfacesServerData_s *interfacesServerData = (struct interfacesServerData_s *)data;

   stop_interfaces();
   sleep(1);
   start_interfaces(interfacesServerData->params_list, interfacesServerData->sqlite3_param_db);

   send_reload(localhost_const, get_nodejsServer_socketdata_port());

   return 0;
}

