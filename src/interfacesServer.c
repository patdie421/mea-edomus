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

#include "mea_string_utils.h"
#include "mea_queue.h"
//#include "debug.h"
#include "mea_verbose.h"
#include "mea_sockets_utils.h"
#include "consts.h"

#include "cJSON.h"
#include "xPL.h"

#include "processManager.h"
#include "nodejsServer.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"
#include "interface_type_003.h"
#include "interface_type_004.h"
#include "interface_type_005.h"
#include "interface_type_006.h"

mea_queue_t *_interfaces=NULL;

pthread_rwlock_t interfaces_queue_rwlock;

//char *sql_select_device_info="SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, types.parameters, sensors_actuators.id_type, lower(sensors_actuators.name), lower(interfaces.name), interfaces.id_type, (SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), interfaces.dev FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";
//char *sql_select_device_info="SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, types.parameters, sensors_actuators.id_type, lower(sensors_actuators.name), lower(interfaces.name), interfaces.id_type, (SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), interfaces.dev, sensors_actuators.todbflag FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";
//char *sql_select_device_info="SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, types.parameters, sensors_actuators.id_type, lower(sensors_actuators.name), lower(interfaces.name), interfaces.id_type, (SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), interfaces.dev, sensors_actuators.todbflag, types.typeoftype, sensors_actuators.id_interface FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";
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
FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";

char *sql_select_interface_info="SELECT * FROM interfaces";

/*
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
*/


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
         switch (iq->type)
         {
            case INTERFACE_TYPE_001:
            {
               interface_type_001_t *i001 = (interface_type_001_t *)(iq->context);
               if(i001->monitoring_id>-1 && process_is_running(i001->monitoring_id) && i001->xPL_callback)
                  i001->xPL_callback2(xplMsgJson, (xPL_ObjectPtr)i001);
               break;
            }

            case INTERFACE_TYPE_002:
            {
               interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
               if(i002->monitoring_id>-1 && process_is_running(i002->monitoring_id) && i002->xPL_callback)
                  i002->xPL_callback2(xplMsgJson, (xPL_ObjectPtr)i002);
               break;
            }

            case INTERFACE_TYPE_003:
            {
               interface_type_003_t *i003 = (interface_type_003_t *)(iq->context);
               if(i003->monitoring_id>-1 && process_is_running(i003->monitoring_id) && i003->xPL_callback)
                  i003->xPL_callback2(xplMsgJson, (xPL_ObjectPtr)i003);
               break;
            }

            case INTERFACE_TYPE_004:
            {
               interface_type_004_t *i004 = (interface_type_004_t *)(iq->context);
               if(i004->monitoring_id>-1 && process_is_running(i004->monitoring_id) && i004->xPL_callback)
                  i004->xPL_callback2(xplMsgJson, (xPL_ObjectPtr)i004);
               break;
            }

            case INTERFACE_TYPE_005:
            {
               interface_type_005_t *i005 = (interface_type_005_t *)(iq->context);
               if(i005->monitoring_id>-1 && process_is_running(i005->monitoring_id) && i005->xPL_callback)
                  i005->xPL_callback2(xplMsgJson, (xPL_ObjectPtr)i005);
               break;
            }

            case INTERFACE_TYPE_006:
            {
               interface_type_006_t *i006 = (interface_type_006_t *)(iq->context);
               if(i006->monitoring_id>-1 && process_is_running(i006->monitoring_id) && i006->xPL_callback)
                  i006->xPL_callback2(xplMsgJson, (xPL_ObjectPtr)i006);
               break;
            }

            default:
               break;
         }
         ret=mea_queue_next(_interfaces);
         if(ret<0)
            break;
      }
   }
   
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
}


void dispatchXPLMessageToInterfaces(xPL_ServicePtr theService, xPL_MessagePtr theMessage)
{
   int ret;

   interfaces_queue_elem_t *iq;

   if(theService == NULL)
      return;

   DEBUG_SECTION mea_log_printf("%s (%s) : Reception message xPL\n", INFO_STR, __func__);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
   
   if(_interfaces && _interfaces->nb_elem)
   {
      mea_queue_first(_interfaces);
      while(1)
      {
         mea_queue_current(_interfaces, (void **)&iq);
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
               if(i002->monitoring_id>-1 && process_is_running(i002->monitoring_id) && i002->xPL_callback)
                  i002->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i002);
               break;
            }

            case INTERFACE_TYPE_003:
            {
               interface_type_003_t *i003 = (interface_type_003_t *)(iq->context);
               if(i003->monitoring_id>-1 && process_is_running(i003->monitoring_id) && i003->xPL_callback)
                  i003->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i003);
               break;
            }

            case INTERFACE_TYPE_004:
            {
               interface_type_004_t *i004 = (interface_type_004_t *)(iq->context);
               if(i004->monitoring_id>-1 && process_is_running(i004->monitoring_id) && i004->xPL_callback)
                  i004->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i004);
               break;
            }

            case INTERFACE_TYPE_005:
            {
               interface_type_005_t *i005 = (interface_type_005_t *)(iq->context);
               if(i005->monitoring_id>-1 && process_is_running(i005->monitoring_id) && i005->xPL_callback)
                  i005->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i005);
               break;
            }

            case INTERFACE_TYPE_006:
            {
               interface_type_006_t *i006 = (interface_type_006_t *)(iq->context);
               if(i006->monitoring_id>-1 && process_is_running(i006->monitoring_id) && i006->xPL_callback)
                  i006->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i006);
               break;
            }

            default:
               break;
         }
         ret=mea_queue_next(_interfaces);
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
      mea_queue_first(_interfaces);
      while(_interfaces->nb_elem)
      {
         mea_queue_out_elem(_interfaces, (void **)&iq);
         switch (iq->type)
         {
            case INTERFACE_TYPE_001:
            {
               interface_type_001_t *i001=(interface_type_001_t *)(iq->context);
               
               if(i001->xPL_callback)
                  i001->xPL_callback=NULL;

               if(i001->monitoring_id!=-1)
               {
                  struct interface_type_001_start_stop_params_s *interface_type_001_start_stop_params = (struct interface_type_001_start_stop_params_s *)process_get_data_ptr(i001->monitoring_id);
                  process_stop(i001->monitoring_id, NULL, 0);
                  process_unregister(i001->monitoring_id);
                  i001->monitoring_id=-1;
                  if(interface_type_001_start_stop_params)
                  {
                     free(interface_type_001_start_stop_params);
                     interface_type_001_start_stop_params=NULL;
                  }
               }
               clean_interface_type_001(i001);
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
                  struct interface_type_002_start_stop_params_s *interface_type_002_start_stop_params = (struct interface_type_002_start_stop_params_s *)process_get_data_ptr(i002->monitoring_id);
                  process_stop(i002->monitoring_id, NULL, 0);
                  process_unregister(i002->monitoring_id);
                  i002->monitoring_id=-1;
                  if(interface_type_002_start_stop_params)
                  {
                     free(interface_type_002_start_stop_params);
                     interface_type_002_start_stop_params=NULL;
                  }
                  if(i002->parameters)
                  {
                     free(i002->parameters);
                     i002->parameters=NULL;
                  }
               }
               clean_interface_type_002(i002);
               free(i002);
               i002=NULL;
               break;
            }
         
            case INTERFACE_TYPE_003:
            {
               interface_type_003_t *i003=(interface_type_003_t *)(iq->context);
               
               if(i003->xPL_callback)
                  i003->xPL_callback=NULL;
               
               if(i003->monitoring_id!=-1)
               {
                  struct interface_type_003_start_stop_params_s *interface_type_003_start_stop_params = (struct interface_type_003_start_stop_params_s *)process_get_data_ptr(i003->monitoring_id);
                  process_stop(i003->monitoring_id, NULL, 0);
                  process_unregister(i003->monitoring_id);
                  i003->monitoring_id=-1;
                  if(interface_type_003_start_stop_params)
                  {
                     free(interface_type_003_start_stop_params);
                     interface_type_003_start_stop_params=NULL;
                  }
                  if(i003->parameters)
                  {
                     free(i003->parameters);
                     i003->parameters=NULL;
                  }
               }
               clean_interface_type_003(i003);
               free(i003);
               i003=NULL;
               break;
            }

            case INTERFACE_TYPE_004:
            {
               interface_type_004_t *i004=(interface_type_004_t *)(iq->context);
               
               if(i004->xPL_callback)
                  i004->xPL_callback=NULL;
               
               if(i004->monitoring_id!=-1)
               {
                  struct interface_type_004_start_stop_params_s *interface_type_004_start_stop_params = (struct interface_type_004_start_stop_params_s *)process_get_data_ptr(i004->monitoring_id);
                  process_stop(i004->monitoring_id, NULL, 0);
                  process_unregister(i004->monitoring_id);
                  i004->monitoring_id=-1;
                  if(interface_type_004_start_stop_params)
                  {
                     free(interface_type_004_start_stop_params);
                     interface_type_004_start_stop_params=NULL;
                  }
                  if(i004->parameters)
                  {
                     free(i004->parameters);
                     i004->parameters=NULL;
                  }
               }
               clean_interface_type_004(i004);
               free(i004);
               i004=NULL;
               break;
            }

            case INTERFACE_TYPE_005:
            {
               interface_type_005_t *i005=(interface_type_005_t *)(iq->context);
               
               if(i005->xPL_callback)
                  i005->xPL_callback=NULL;
               
               if(i005->monitoring_id!=-1)
               {
                  struct interface_type_005_start_stop_params_s *interface_type_005_start_stop_params = (struct interface_type_005_start_stop_params_s *)process_get_data_ptr(i005->monitoring_id);
                  process_stop(i005->monitoring_id, NULL, 0);
                  process_unregister(i005->monitoring_id);
                  i005->monitoring_id=-1;
                  if(interface_type_005_start_stop_params)
                  {
                     free(interface_type_005_start_stop_params);
                     interface_type_005_start_stop_params=NULL;
                  }
                  if(i005->parameters)
                  {
                     free(i005->parameters);
                     i005->parameters=NULL;
                  }
               }
               clean_interface_type_005(i005);
               free(i005);
               i005=NULL;
               break;
            }

            case INTERFACE_TYPE_006:
            {
               interface_type_006_t *i006=(interface_type_006_t *)(iq->context);
               
               if(i006->xPL_callback)
                  i006->xPL_callback=NULL;
               
               if(i006->monitoring_id!=-1)
               {
                  struct interface_type_006_start_stop_params_s *interface_type_006_start_stop_params = (struct interface_type_006_start_stop_params_s *)process_get_data_ptr(i006->monitoring_id);
                  process_stop(i006->monitoring_id, NULL, 0);
                  process_unregister(i006->monitoring_id);
                  i006->monitoring_id=-1;
                  if(interface_type_006_start_stop_params)
                  {
                     free(interface_type_006_start_stop_params);
                     interface_type_006_start_stop_params=NULL;
                  }
                  if(i006->parameters)
                  {
                     free(i006->parameters);
                     i006->parameters=NULL;
                  }
               }
               clean_interface_type_006(i006);
               free(i006);
               i006=NULL;
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


mea_queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db)
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

            int monitoring_id=-1;

            switch(id_type)
            {
               case INTERFACE_TYPE_001:
               {
                  interface_type_001_t *i001;
                  
                  i001=malloc_and_init_interface_type_001(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)description);
                  if(i001 == NULL)
                     break;
                  iq->context=i001;
                  monitoring_id=i001->monitoring_id;
                  break;
               }
                 
               case INTERFACE_TYPE_002:
               {
                  interface_type_002_t *i002;

                  i002=malloc_and_init_interface_type_002(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)parameters ,(char *)description);
                  if(i002 == NULL)
                     break;
                  iq->context=i002;
                  monitoring_id=i002->monitoring_id;
                  break;
               }
                  
               case INTERFACE_TYPE_003:
               {
                  interface_type_003_t *i003;

                  i003=malloc_and_init_interface_type_003(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)parameters, (char *)description);
                  if(i003 == NULL)
                     break;
                  iq->context=i003;
                  monitoring_id=i003->monitoring_id;
                  break;
               }

               case INTERFACE_TYPE_004:
               {
                  interface_type_004_t *i004;

                  i004=malloc_and_init_interface_type_004(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)parameters, (char *)description);
                  if(i004 == NULL)
                     break;
                  iq->context=i004;
                  monitoring_id=i004->monitoring_id;
                  break;
               }

               case INTERFACE_TYPE_005:
               {
                  interface_type_005_t *i005;

                  i005=malloc_and_init_interface_type_005(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)parameters, (char *)description);
                  if(i005 == NULL)
                     break;
                  iq->context=i005;
                  monitoring_id=i005->monitoring_id;
                  break;
               }

               case INTERFACE_TYPE_006:
               {
                  interface_type_006_t *i006;

                  i006=malloc_and_init_interface_type_006(sqlite3_param_db, id_interface, (char *)name, (char *)dev, (char *)parameters, (char *)description);
                  if(i006 == NULL)
                     break;
                  iq->context=i006;
                  monitoring_id=i006->monitoring_id;
                  break;
               }

               default:
                  break;
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


int restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct interfacesServerData_s *interfacesServerData = (struct interfacesServerData_s *)data;

   stop_interfaces();
   sleep(1);
   start_interfaces(interfacesServerData->params_list, interfacesServerData->sqlite3_param_db);

   send_reload(localhost_const, get_nodejsServer_socketdata_port());

   return 0;
}

