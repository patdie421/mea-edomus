//
//  interface_type_006.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/11/2015.
//
//
#include <Python.h>

#include "interface_type_006.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sqlite3.h>
#include <errno.h>
#include <pthread.h>

#include "globals.h"
#include "consts.h"

#include "serial.h"

#include "tokens.h"
#include "tokens_da.h"

#include "mea_verbose.h"
#include "macros.h"

#include "dbServer.h"
#include "parameters_utils.h"
#include "mea_api.h"
#include "pythonPluginServer.h"
#include "python_utils.h"

#include "processManager.h"
#include "notify.h"

#include "interfacesServer.h"
#include "interface_type_006.h"


char *interface_type_006_senttoplugin_str="SENT2PLUGIN";
char *interface_type_006_xplin_str="XPLIN";
char *interface_type_006_serialin_str="SERIALIN";
char *interface_type_006_serialout_str="SERIALOUT";


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_genericserial_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define GENERICSERIAL_PLUGIN_PARAMS_PLUGIN      0
#define GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS  1


struct genericserial_callback_xpl_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
   sqlite3        *param_db;
};


struct genericserial_thread_params_s
{
//   int                   fd;
   sqlite3              *param_db;
//   pthread_mutex_t       callback_lock;
//   pthread_cond_t        callback_cond;
   PyThreadState        *mainThreadState;
   PyThreadState        *myThreadState;
   parsed_parameters_t  *plugin_params;
   int                   nb_plugin_params;
   sqlite3_stmt         *stmt;
   interface_type_006_t *i006;
};



#define python_lock() \
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); \
   PyEval_AcquireLock(); \
   PyThreadState *__mainThreadState=PyThreadState_Get(); \
   PyThreadState *__myThreadState = PyThreadState_New(__mainThreadState->interp); \
   PyThreadState *__tempState = PyThreadState_Swap(__myThreadState)
   
#define python_unlock() \
   PyThreadState_Swap(__tempState); \
   PyThreadState_Clear(__myThreadState); \
   PyThreadState_Delete(__myThreadState); \
   PyEval_ReleaseLock(); \
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)

   
int python_call_function(char *plugin_name, char *plugin_func, PyObject *plugin_params_dict)
{
   PyObject *pName, *pModule, *pFunc;
   PyObject *pArgs, *pValue=NULL;
   int retour=-1;

   PyErr_Clear();
   pName = PyString_FromString(plugin_name);
   pModule = PyImport_Import(pName);
   if(!pModule)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : %s not found\n", ERROR_STR, __func__, plugin_name);
   }
   else
   {
      pFunc = PyObject_GetAttrString(pModule, plugin_func);
      if (pFunc && PyCallable_Check(pFunc))
      {
         pArgs = PyTuple_New(1);
         Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
         PyTuple_SetItem(pArgs, 0, plugin_params_dict);
         
         pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
         if (pValue != NULL)
         {
            retour=(int)PyInt_AsLong(pValue);
            Py_DECREF(pValue);
            DEBUG_SECTION mea_log_printf("%s (%s) : Result of call of %s : %ld\n", DEBUG_STR, __func__, plugin_func, plugin_name);
         }
         Py_DECREF(pArgs);
      }
      else
      {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not fount in %s module\n", ERROR_STR, __func__, plugin_func, plugin_name);
      }
      Py_XDECREF(pFunc);
   }
   Py_XDECREF(pModule);
   Py_DECREF(pName);
   PyErr_Clear();
      
   return retour;
}


int interface_type_006_data_to_plugin(PyThreadState *myThreadState, sqlite3_stmt * stmt, int data_type, void *data, int l_data)
{
   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;
   int err;

/*
0:sensors_actuators.id_sensor_actuator
1:sensors_actuators.id_location
2:sensors_actuators.state
3:sensors_actuators.parameters
4:sensors_actuators.id_type
5:lower(sensors_actuators.name)
6:sensors_actuators.todbflag
*/
   
   plugin_params=alloc_parsed_parameters((char *)sqlite3_column_text(stmt, 3), valid_genericserial_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return -1;
   }

   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem)
   {
      plugin_elem->type_elem=data_type;
            
      { // appel des fonctions Python
         PyEval_AcquireLock();
         PyThreadState *tempState = PyThreadState_Swap(myThreadState);
               
         plugin_elem->aDict=PyDict_New();
         if(!plugin_elem->aDict)
         {
            free(plugin_elem);
            plugin_elem=NULL;
            release_parsed_parameters(&plugin_params);
            return -1;
         }

         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_ID_ID), sqlite3_column_int(stmt, 0));
         mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_NAME_ID), (char *)sqlite3_column_text(stmt, 5));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_TYPE_ID_ID), sqlite3_column_int(stmt, 4));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_LOCATION_ID_ID), sqlite3_column_int(stmt, 1));
         mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_STATE_ID), (char *)sqlite3_column_text(stmt, 2));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(TODBFLAG_ID), sqlite3_column_int(stmt, 6));

         // les datas         
         PyObject *value = PyByteArray_FromStringAndSize((char *)data, l_data);
         PyDict_SetItemString(plugin_elem->aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(plugin_elem->aDict, "l_data", (long)l_data);

         // parametres spécifiques
         if(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);

         
         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
      } // fin appel des fonctions Python
      
      pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
      
      free(plugin_elem);
      plugin_elem=NULL;
      release_parsed_parameters(&plugin_params);
      
      return 0;
   }
   
   return -1;
}


void set_interface_type_006_isnt_running(void *data)
{
   interface_type_006_t *i006 = (interface_type_006_t *)data;

   i006->thread_is_running=0;
}


int16_t _interface_type_006_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr xplMsg, xPL_ObjectPtr userValue)
{
   xPL_NameValueListPtr xplBody;
   char *device;
   int ret;
   int err;
   
   interface_type_006_t *interface=(interface_type_006_t *)userValue;
   struct genericserial_callback_xpl_data_s *params=(struct genericserial_callback_xpl_data_s *)interface->xPL_callback_data;
   
   interface->indicators.xplin++;
   
   sqlite3 *params_db = params->param_db;
   
   if(!params->mainThreadState)
   {
      params->mainThreadState=PyThreadState_Get();
   }
   if(!params->myThreadState)
      params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   
   xplBody = xPL_getMessageBody(xplMsg);
   device  = xPL_getNamedValue(xplBody, XPL_DEVICE_STR_C);
   
   if(!device)
      return -1;
   
   char sql[1024];
   sqlite3_stmt * stmt;
   
   sprintf(sql,"%s WHERE lower(sensors_actuators.name)='%s' and sensors_actuators.state='1';", sql_select_device_info, device);
   ret = sqlite3_prepare_v2(params_db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
      return -1;
   }
   
   while(1)
   {
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         parsed_parameters_t *plugin_params=NULL;
         int nb_plugin_params;
        
         plugin_params=alloc_parsed_parameters((char *)sqlite3_column_text(stmt, 3), valid_genericserial_plugin_params, &nb_plugin_params, &err, 0);
         if(!plugin_params || !plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
         {
            if(plugin_params)
               release_parsed_parameters(&plugin_params);

            continue;
         }
         plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
         if(plugin_elem)
         {
            plugin_elem->type_elem=XPLMSG;
            
            { // appel des fonctions Python
               PyEval_AcquireLock();
               PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
               
               plugin_elem->aDict=mea_stmt_to_pydict_device(stmt);
               if(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
                  mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);
                  
               // ajouter ici les données xpl ...
               
               PyThreadState_Swap(tempState);
               PyEval_ReleaseLock();
            } // fin appel des fonctions Python
            
            pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
            interface->indicators.senttoplugin++;
            free(plugin_elem);
            plugin_elem=NULL;
         }
         
         release_parsed_parameters(&plugin_params);
         plugin_params=NULL;
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         sqlite3_finalize(stmt);
         break; // voir autres erreurs possibles
      }
   }
   return 0;
}


void *_thread_interface_type_006_genericserial_data_cleanup(void *args)
{
   struct genericserial_thread_params_s *params=(struct genericserial_thread_params_s *)args;

   if(!params)
      return NULL;
   
   if(params->i006->fd!=-1)
   {
      close(params->i006->fd);
   }
   
   if(params->myThreadState)
   {
      PyEval_AcquireLock();
      PyThreadState_Clear(params->myThreadState);
      PyThreadState_Delete(params->myThreadState);
      PyEval_ReleaseLock();
      params->myThreadState=NULL;
   }
   
   if(params->plugin_params)
      release_parsed_parameters(&(params->plugin_params));
   
   if(params->stmt)
   {
      sqlite3_finalize(params->stmt);
      params->stmt=NULL;
   }
   
   free(params);
   params=NULL;

   return NULL;
}


void *_thread_interface_type_006_genericserial_data(void *args)
{
   struct genericserial_thread_params_s *params=(struct genericserial_thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_006_genericserial_data_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_006_isnt_running, (void *)params->i006 );
   
   params->i006->thread_is_running=1;
   process_heartbeat(params->i006->monitoring_id);
   
   int err_counter=0;
   
   sqlite3 *params_db=params->param_db;
//   int ret;
   
   params->plugin_params=NULL;
   params->nb_plugin_params=0;
   params->stmt=NULL;
   
   // on se met un context python sous le coude pour ce thread
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->mainThreadState = PyThreadState_Get();
   params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();

   mea_timer_t process_timer;

   mea_init_timer(&process_timer, 5, 1);
   mea_start_timer(&process_timer); 

   params->i006->fd=-1; 

   while(1)
   {
      if(params->i006->fd<0)
      {
         params->i006->fd=serial_open(params->i006->real_dev, params->i006->real_speed);
         if(params->i006->fd < 0)
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : can't open %s - ", ERROR_STR, __func__, params->i006->real_dev);
               perror("");
            }
         }
      }

      if(params->i006->fd>=0)
      {
         char c;

         char buffer[4096];
         int buffer_ptr=0;
         struct timeval timeout;
         
         err_counter=0;
         
         while(1)
         {
            if(mea_test_timer(&process_timer)==0)
            {
               process_heartbeat(params->i006->monitoring_id);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_senttoplugin_str, params->i006->indicators.senttoplugin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_xplin_str, params->i006->indicators.xplin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_serialin_str, params->i006->indicators.serialin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_serialout_str, params->i006->indicators.serialout);
            }

            fd_set input_set;
            FD_ZERO(&input_set);
            FD_SET(params->i006->fd, &input_set);

            timeout.tv_sec  = 0; // timeout après
            timeout.tv_usec = 100000; // 100ms

            int ret = select(params->i006->fd+1, &input_set, NULL, NULL, &timeout);
            if (ret <= 0)
            {
               if(ret == 0)
               {
                  if(buffer_ptr>0)
                     break; // après un "blanc" de 100 ms, si on a des données, on les envoie au plugin
               }
               else
               {
                  // erreur à traiter ...
                  VERBOSE(5) {
                     mea_log_printf("%s (%s) : select error - ", ERROR_STR, __func__);
                     perror("");
                  }
                  close(params->i006->fd);
                  params->i006->fd=-1;
                  break;
               }
            }
            else
            {
               ret=read(params->i006->fd, &c, 1);
               if(ret<0)
               {
                  close(params->i006->fd);
                  params->i006->fd=-1;
                  break;
               }
            }
            if(ret>0)
            {
               buffer[buffer_ptr++]=c;
               if(buffer_ptr >= sizeof(buffer)-1)
                  break;
            }
         }

         if(buffer_ptr>0)
         {
            params->i006->indicators.serialin+=buffer_ptr;
         
            buffer[buffer_ptr]=0;
//            DEBUG_SECTION mea_log_printf("%s (%s) : data from Serial : %s\n", INFO_STR, __func__, buffer);

            // transmettre buffer aux plugins            
            char sql_request[1024];
            sqlite3_stmt * stmt;

            sprintf(sql_request, "SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, sensors_actuators.id_type, lower(sensors_actuators.name), sensors_actuators.todbflag FROM sensors_actuators WHERE id_interface=%d and sensors_actuators.state='1'", params->i006->id_interface);
            
            int ret = sqlite3_prepare_v2(params_db, sql_request, strlen(sql_request)+1, &stmt, NULL);
            if(ret)
            {
               VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
            }
            else
            {
               while(1)
               {
                  int s=sqlite3_step(stmt);
                  
                  if(s==SQLITE_ROW)
                  {
/*
                     for(int i=0;i<buffer_ptr;i++) fprintf(stderr,"%d:[%03d-%02x-%c] ", i, (unsigned char)buffer[i], (unsigned char)buffer[i], buffer[i]); fprintf(stderr,"\n");
*/

                     int ret=interface_type_006_data_to_plugin(params->myThreadState, stmt, GENERICSERIALDATA, (void *)buffer, buffer_ptr);
                     if(ret<0)
                     {
                        fprintf(stderr,"ERROR\n");
                     }
                  }
                  else if (s==SQLITE_DONE) 
                  {
                     sqlite3_finalize(stmt);
                     break; 
                  }
                  else 
                  {
                     fprintf(stderr,"ERREUR\n");
                     // traitement d'erreur à faire ici 
                     sqlite3_finalize(stmt);
                  }
               }
            }

            buffer_ptr=0;
         }
         else
         {
         }

         pthread_testcancel();
      }
      else
      {
         err_counter++;
         if(err_counter<5)
            sleep(5);
         else
         {
            goto _thread_interface_type_006_genericserial_data_clean_exit;
         }
      }

      pthread_testcancel();
   }

_thread_interface_type_006_genericserial_data_clean_exit:
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);

   process_async_stop(params->i006->monitoring_id);
   for(;;) sleep(1);
   
   return NULL;
}


pthread_t *start_interface_type_006_genericserial_data_thread(interface_type_006_t *i006, sqlite3 *db, thread_f function)
{
   pthread_t *thread=NULL;
   struct genericserial_thread_params_s *params=NULL;
   struct genericserial_callback_data_s *genericserial_callback_data=NULL;
   
   params=malloc(sizeof(struct genericserial_thread_params_s));
   if(!params)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }

   params->param_db=db;
   params->i006=(void *)i006;
   params->mainThreadState = NULL;
   params->myThreadState = NULL;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto clean_exit;
   
   return thread;
   
clean_exit:
   if(thread)
   {
      free(thread);
      thread=NULL;
   }
   
   if(genericserial_callback_data)
   {
      free(genericserial_callback_data);
      genericserial_callback_data=NULL;
   }

   if(params)
   {
      free(params);
      params=NULL;
   }
   return NULL;
}


int clean_interface_type_006(interface_type_006_t *i006)
{
   if(i006->parameters)
   {
      free(i006->parameters);
      i006->parameters=NULL;
   }

   if(i006->xPL_callback_data)
   {
      free(i006->xPL_callback_data);
      i006->xPL_callback_data=NULL;
   }
   
   if(i006->xPL_callback)
      i006->xPL_callback=NULL;

   if(i006->thread)
   {
      free(i006->thread);
      i006->thread=NULL;
   }
      
   return 0;
}


interface_type_006_t *malloc_and_init_interface_type_006(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description)
{
   interface_type_006_t *i006;
                  
   i006=(interface_type_006_t *)malloc(sizeof(interface_type_006_t));
   if(!i006)
   {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   i006->thread_is_running=0;
                  
   struct interface_type_006_data_s *i006_start_stop_params=(struct interface_type_006_data_s *)malloc(sizeof(struct interface_type_006_data_s));
   if(!i006_start_stop_params)
   {
      free(i006);
      i006=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }  
      return NULL;
   }
   strncpy(i006->dev, (char *)dev, sizeof(i006->dev)-1);
   strncpy(i006->name, (char *)name, sizeof(i006->name)-1);
   i006->id_interface=id_interface;
   i006->parameters=(char *)malloc(strlen((char *)parameters)+1);
   strcpy(i006->parameters,(char *)parameters);
   i006->indicators.senttoplugin=0;
   i006->indicators.xplin=0;
   i006->indicators.serialin=0;
   i006->indicators.serialout=0;
   
   i006->thread=NULL;
   i006->xPL_callback=NULL;
   i006->xPL_callback_data=NULL;

   i006->monitoring_id=process_register((char *)name);
   i006_start_stop_params->sqlite3_param_db = sqlite3_param_db;
   i006_start_stop_params->i006=i006;
                  
   process_set_group(i006->monitoring_id, 1);
   process_set_start_stop(i006->monitoring_id, start_interface_type_006, stop_interface_type_006, (void *)i006_start_stop_params, 1);
   process_set_watchdog_recovery(i006->monitoring_id, restart_interface_type_006, (void *)i006_start_stop_params);
   process_set_description(i006->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i006->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i006->monitoring_id, interface_type_006_senttoplugin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_xplin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialout_str, 0);

   return i006;
}


int stop_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;

   VERBOSE(1) mea_log_printf("%s  (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i006->name);

   if(start_stop_params->i006->xPL_callback_data)
   {
      free(start_stop_params->i006->xPL_callback_data);
      start_stop_params->i006->xPL_callback_data=NULL;
   }
   
   if(start_stop_params->i006->xPL_callback)
   {
      start_stop_params->i006->xPL_callback=NULL;
   }
   

   if(start_stop_params->i006->thread)
   {
      pthread_cancel(*(start_stop_params->i006->thread));
      
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(start_stop_params->i006->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i006->name,100-counter);

      free(start_stop_params->i006->thread);
      start_stop_params->i006->thread=NULL;
   }
   
   mea_notify_printf('S', "%s %s", start_stop_params->i006->name, stopped_successfully_str);

   return 0;
}


int restart_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int16_t check_status_interface_type_006(interface_type_006_t *i006)
{
   return 0;
}

int start_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char dev[81];
   char buff[80];
   speed_t speed;
   int err=0;
   char err_str[128];
   int ret=0;
   struct genericserial_callback_xpl_data_s *xpl_callback_params=NULL;
   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;
    
   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;
   start_stop_params->i006->thread=NULL;

   ret=get_dev_and_speed((char *)start_stop_params->i006->dev, buff, sizeof(buff), &speed);
   if(!ret)
   {
      int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(buff)-1))
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i006->name, err_str);
         goto clean_exit;
      }
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR, __func__, start_stop_params->i006->dev);
      mea_notify_printf('E', "%s can't be launched - incorrect device/speed interface - %s.\n", start_stop_params->i006->name, start_stop_params->i006->dev);
      goto clean_exit;
   }
   strncpy(start_stop_params->i006->real_dev, dev, sizeof( start_stop_params->i006->real_dev)-1);
   start_stop_params->i006->real_speed=speed;


   interface_parameters=alloc_parsed_parameters(start_stop_params->i006->parameters, valid_genericserial_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(interface_parameters)
      {
         // pas de plugin spécifié
         free(interface_parameters);
         interface_parameters=NULL;
         VERBOSE(9) mea_log_printf("%s  (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->i006->parameters);
   }
   else
   {
      python_lock();

      PyObject *plugin_params_dict=PyDict_New();
      mea_addLong_to_pydict(plugin_params_dict, INTERFACE_ID_STR_C, start_stop_params->i006->id_interface);
      
      if(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
         mea_addString_to_pydict(plugin_params_dict, INTERFACE_PARAMETERS_STR_C, interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);
      
      python_call_function(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, "mea_init", plugin_params_dict);
      
      Py_DECREF(plugin_params_dict);
      
      python_unlock();

      if(interface_parameters)
         release_parsed_parameters(&interface_parameters);
   }


   // données pour les callbacks xpl
   xpl_callback_params=(struct genericserial_callback_xpl_data_s *)malloc(sizeof(struct genericserial_callback_xpl_data_s));
   if(!xpl_callback_params)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s", start_stop_params->i006->name, err_str);
      goto clean_exit;
   }
   xpl_callback_params->param_db=start_stop_params->sqlite3_param_db;
   xpl_callback_params->mainThreadState=NULL;
   xpl_callback_params->myThreadState=NULL;
   start_stop_params->i006->xPL_callback_data=xpl_callback_params;
   start_stop_params->i006->xPL_callback=_interface_type_006_xPL_callback;

   start_stop_params->i006->thread=start_interface_type_006_genericserial_data_thread(start_stop_params->i006, start_stop_params->sqlite3_param_db, (thread_f)_thread_interface_type_006_genericserial_data);
   
   if(start_stop_params->i006->thread!=0)
   {
      VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__,start_stop_params->i006->name, launched_successfully_str);
      mea_notify_printf('S', "%s %s", start_stop_params->i006->name, launched_successfully_str);
      return 0;
   }

   strerror_r(errno, err_str, sizeof(err_str));
   VERBOSE(2) mea_log_printf("%s  (%s) : %s can't start - %s.\n", ERROR_STR, __func__, start_stop_params->i006->name, err_str);
   mea_notify_printf('E', "%s can't be launched - %s", start_stop_params->i006->name, err_str);
   
clean_exit:
   if(start_stop_params->i006->thread)
   {
      stop_interface_type_006(start_stop_params->i006->monitoring_id, start_stop_params, NULL, 0);
   }
   
   if(interface_parameters)
   {
      release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;

   }
   
   if(xpl_callback_params)
   {
      free(xpl_callback_params);
      xpl_callback_params=NULL;
   }
   
   return -1;
}
/*
AT

OK
ATE0

OK
AT+CPIN=1234

ERROR
AT+CMGF=1

OK
AT+CNMI=2,2,0,0,0

OK
AT+CSDH=0

OK

+CMT: "+33661665082","","15/11/17,23:51:28+04"
Info télésurveillance 15/11/2015 15:40:20 : mise en marche via code NATHALIE du clavier ENTREE (zone 1) au 16 RUE JULES GUESDE - ROSNY SOUS

+CMT: "+33661665082","","15/11/17,23:51:52+04"
##4,A,G;7,L,L##
AT

OK
AT+CMGS="+33661665082"

> A/PIN4=688;DONE

> 
+CMGS: 94

OK

+CMT: "+33661665082","","15/11/17,23:54:28+04"
 Info télésurveillance 13/11/2015 08:40:27 : mise à l'arret via code NATHALIE du clavier ENTREE (zone 1) au 16 RUE JULES GUESDE - ROSNY SOUS
*/

/*
# coding: utf8

import re
import string
import sys
import unicodedata

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose

from sets import Set
import string

debug=0


def mea_xplCmndMsg(data):
   fn_name=sys._getframe().f_code.co_name

   return 0


def mea_serialData(data):
   fn_name=sys._getframe().f_code.co_name

   try:
       id_sensor=data["device_id"]
       serial_data=data["data"]
       l_serial_data=data["l_data"]
       parameters=data["device_parameters"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem=mea.getMemory(id_sensor)
#   verbose(2,"mem = ",mem)
   paramsDict=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")

# format du SMS que nous devons traiter :
# +CMT: "+33661665082","","15/11/19,22:57:29+04",145,32,0,0,"+33660003151",145,140
# Info télésurveillance 13/11/2015 08:40:27 : mise à l'arret via code NATHALIE du clavier ENTREE (zone 1) au 16 RUE JULES GUESDE - ROSNY SOUS

   s=serial_data.decode('latin-1')

   alarm=-1
   numtel=u""
   nbchars=0

   s1=s.split("\r\n");
   for i in range(len(s1)):
      if len(s1[i]) <> 0:
         if s1[i].find(u"+CMT:") <> -1:
            s2=s1[i].split(",");
            numtel  = s2[0].split('"')[1::2][0] # plus efficace qu'une expression régulière
            nbchars = s2[10]
            # on sait que les infos que nous attendons sont dans la ligne suivante (nbchars ne va pas nous servir dans ce cas ...
            if s1[i+1].find(u"mise à l'arret") <> -1:
               print "A l'arrêt"
               alarm=1
            elif s1[i+1].find(u"mise en marche") <> -1:
               print "En marche"
               alarm=2
            break

   return alarm
*/
