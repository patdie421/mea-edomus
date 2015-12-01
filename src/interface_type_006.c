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
//#include "mea_api.h"
#include "pythonPluginServer.h"
#include "python_utils.h"

#include "processManager.h"
#include "notify.h"

#include "interfacesServer.h"
#include "interface_type_006.h"


char *interface_type_006_senttoplugin_str="SENT2PLUGIN";
char *interface_type_006_xplin_str="XPLIN";
char *interface_type_006_xplout_str="XPLOUT";
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
   sqlite3       *param_db;
   PyThreadState *mainThreadState;
   PyThreadState *myThreadState;
   sqlite3_stmt  *stmt;
   PyObject      *pModule, *pFunc, *pParams;
   interface_type_006_t *i006;
};


int interface_type_006_call_data_pre(struct genericserial_thread_params_s *params, void *data, int l_data)
{
   int retour=-1;
   if(params->pFunc)
   {
      PyEval_AcquireLock();
      PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);

      PyObject *aDict=PyDict_New();
      if(aDict)
      {
         PyObject *value = PyByteArray_FromStringAndSize((char *)data, l_data);
         PyDict_SetItemString(aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(aDict, "l_data", (long)l_data);

         mea_addLong_to_pydict(aDict, INTERFACE_ID_STR_C, params->i006->id_interface);
         mea_addLong_to_pydict(aDict, "fd", params->i006->fd);

         if(params->pParams)
            PyDict_SetItemString(aDict, "plugin_paramters", params->pParams);

         retour=mea_call_python_function2(params->pFunc, aDict);
         Py_DECREF(aDict);
      }

      PyThreadState_Swap(tempState);
      PyEval_ReleaseLock();
   }

   return retour;
}


int interface_type_006_data_to_plugin(PyThreadState *myThreadState, int fd, sqlite3_stmt * stmt, int data_type, void *data, int l_data)
{
   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;
   int err;
   int retour=-1;
   plugin_queue_elem_t *plugin_elem = NULL;

   plugin_params=alloc_parsed_parameters((char *)sqlite3_column_text(stmt, 3), valid_genericserial_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
      goto interface_type_006_data_to_plugin_clean_exit;

   plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem)
   {
      plugin_elem->type_elem=data_type;
            
      { // appel des fonctions Python => on lock
         PyEval_AcquireLock();
         pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // trop compliquer de traiter avec pthread_cleanup => on interdit les arrêts lors des commandes python

         PyThreadState *tempState = PyThreadState_Swap(myThreadState);
               
         plugin_elem->aDict=PyDict_New();
         if(!plugin_elem->aDict)
            goto interface_type_006_data_to_plugin_clean_exit;

         // le contexte
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_ID_ID), sqlite3_column_int(stmt, 0));
         mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_NAME_ID), (char *)sqlite3_column_text(stmt, 5));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_TYPE_ID_ID), sqlite3_column_int(stmt, 4));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_LOCATION_ID_ID), sqlite3_column_int(stmt, 1));
         mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_STATE_ID), (char *)sqlite3_column_text(stmt, 2));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(TODBFLAG_ID), sqlite3_column_int(stmt, 6));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(INTERFACE_ID_ID), sqlite3_column_int(stmt, 7));
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(TYPEOFTYPE_ID), sqlite3_column_int(stmt, 8));

         // les datas
         PyObject *value = PyByteArray_FromStringAndSize((char *)data, (long)l_data);
         PyDict_SetItemString(plugin_elem->aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(plugin_elem->aDict, "l_data", (long)l_data);

         mea_addLong_to_pydict(plugin_elem->aDict, "fd", fd);

         // parametres spécifiques
         if(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);

         PyThreadState_Swap(tempState);
         pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // on réauthorise les arrêts         
         PyEval_ReleaseLock();
      } // fin appel des fonctions Python
     
      // commande dans la file de traitement 
      pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));

      retour = 0;
   }

interface_type_006_data_to_plugin_clean_exit:
   if(plugin_elem)
   {
      free(plugin_elem);
      plugin_elem=NULL;
   }

   if(plugin_params)
   {
      release_parsed_parameters(&plugin_params);
      nb_plugin_params=0;
      plugin_params=NULL; 
   }
 
   return retour;
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
   
   char sql[2048];
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
            {
               release_parsed_parameters(&plugin_params);
               nb_plugin_params=0;
               plugin_params=NULL;
            }

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

               mea_addLong_to_pydict(plugin_elem->aDict, "fd", (long)interface->fd);

               PyObject *_xplmsg=mea_xplMsgToPyDict(xplMsg); 
               PyDict_SetItemString(plugin_elem->aDict, "xplmsg", _xplmsg); 
               
               PyThreadState_Swap(tempState);
               PyEval_ReleaseLock();
            } // fin appel des fonctions Python
            
            pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
            interface->indicators.senttoplugin++;
            free(plugin_elem);
            plugin_elem=NULL;
         }
         
         release_parsed_parameters(&plugin_params);
         nb_plugin_params=0;
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
      PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
      if(params->pFunc)
      {
         Py_XDECREF(params->pFunc);
         params->pFunc=NULL;
      } 
      if(params->pModule)
      {
         Py_XDECREF(params->pModule);
         params->pModule=NULL;
      }
      if(params->pParams)
      {
         Py_XDECREF(params->pParams);
         params->pParams=NULL;
      }
      PyThreadState_Swap(tempState);
      PyThreadState_Clear(params->myThreadState);
      PyThreadState_Delete(params->myThreadState);
      PyEval_ReleaseLock();

      params->myThreadState=NULL;
   }
   else
   {
      python_lock();
      if(params->pFunc)
      {
         Py_XDECREF(params->pFunc);
         params->pFunc=NULL;
      }
      if(params->pModule)
      {
         Py_XDECREF(params->pModule);
         params->pModule=NULL;
      }
      if(params->pParams)
      {
         Py_XDECREF(params->pParams);
         params->pParams=NULL;
      }
      python_unlock();
   }
      
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
      if(params->i006->fd<0) // pas ou plus decommunication avec le périphérique serie
      {
         params->i006->fd=serial_open(params->i006->real_dev, params->i006->real_speed);
         if(params->i006->fd<0)
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
               process_update_indicator(params->i006->monitoring_id, interface_type_006_xplout_str, params->i006->indicators.xplout);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_serialin_str, params->i006->indicators.serialin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_serialout_str, params->i006->indicators.serialout);
            }

            fd_set input_set;
            FD_ZERO(&input_set);
            FD_SET(params->i006->fd, &input_set);

            timeout.tv_sec  = 0; // timeout après
            timeout.tv_usec = 200000; // 100ms

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

            int ret=interface_type_006_call_data_pre(params, (void *)buffer, buffer_ptr+1);
            if(ret!=0)
            {
               // transmettre buffer aux plugins
               char sql_request[1024];
               sqlite3_stmt * stmt;

               sprintf(sql_request, "SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, sensors_actuators.id_type, lower(sensors_actuators.name), sensors_actuators.todbflag, sensors_actuators.id_interface, types.typeoftype FROM sensors_actuators INNER JOIN types ON sensors_actuators.id_type = types.id_type WHERE sensors_actuators.id_interface=%d and sensors_actuators.state='1'", params->i006->id_interface);
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
                        int ret=interface_type_006_data_to_plugin(params->myThreadState, params->i006->fd, stmt, GENERICSERIALDATA, (void *)buffer, buffer_ptr+1);
                        if(ret<0)
                        {
                           VERBOSE(5) mea_log_printf("%s (%s) : can't send to plugin\n", ERROR_STR, __func__);
                        }
                     }
                     else if (s==SQLITE_DONE) 
                     {
                        sqlite3_finalize(stmt);
                        break; 
                     }
                     else 
                     {
                        VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_step - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
                        sqlite3_finalize(stmt);
                     }
                  }
               }
            }
         }
         pthread_testcancel();
      }
      else
      {
         err_counter++;
         if(err_counter<5)
            sleep(5);
         else goto _thread_interface_type_006_genericserial_data_clean_exit;
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


pthread_t *start_interface_type_006_genericserial_data_thread(interface_type_006_t *i006, sqlite3 *db,  parsed_parameters_t *interface_parameters, thread_f function)
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
      goto start_interface_type_006_genericserial_data_thread_clean_exit;
   }

   params->param_db=db;
   params->i006=(void *)i006;
   params->mainThreadState = NULL;
   params->myThreadState = NULL;
   params->pModule = NULL;
   params->pFunc = NULL;
   params->pParams = NULL;

   // recherche pré-traitement du plugin
   PyObject *pName=NULL, *pModule=NULL, *pFunc=NULL;

   python_lock(); // attention python_lock / python_unlock définissent un block ({ }) les variables déclérées restent locales au bloc
   if(interface_parameters && interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      pName = PyString_FromString(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s);
      pModule =  PyImport_Import(pName);
      Py_XDECREF(pName);
      pName=NULL;

      if(pModule)
      {
         // rechargement à revoir ...
         PyObject *m;
         m=pModule;
         pModule=PyImport_ReloadModule(m); // on force le rechargement (c'est pour simplifier)
         Py_DECREF(m); 
         pFunc = PyObject_GetAttrString(pModule, "mea_serialDataPre");

         if(pFunc && PyCallable_Check(pFunc))
         {
            params->pModule=pModule;
            params->pFunc=pFunc;
            if(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
               params->pParams=PyString_FromString(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);
            else
               params->pParams=NULL;
         }
         else
         {
            VERBOSE(5) mea_log_printf("%s (%s) : no mea_serialDataPre entry point\n", ERROR_STR, __func__);
            if(pFunc)
              Py_XDECREF(pFunc);
            Py_XDECREF(pModule);
            pFunc=NULL;
            pModule=NULL;
         }
      }
   }
   python_unlock();

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto start_interface_type_006_genericserial_data_thread_clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto start_interface_type_006_genericserial_data_thread_clean_exit;
   
   return thread;
   
start_interface_type_006_genericserial_data_thread_clean_exit:
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
//   process_add_indicator(i006->monitoring_id, interface_type_006_xplout_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialout_str, 0);

   return i006;
}


int stop_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ...\n", INFO_STR, __func__, start_stop_params->i006->name);

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
//      int stopped=-1;
      while(counter--)
      {
         if(start_stop_params->i006->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
         {
//            stopped=0;
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

/*
int16_t check_status_interface_type_006(interface_type_006_t *i006)
{
   return 0;
}
*/

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
         release_parsed_parameters(&interface_parameters);
         interface_nb_parameters=0;
         interface_parameters=NULL;
         VERBOSE(9) mea_log_printf("%s (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->i006->parameters);
   }
   else
   {
      python_lock(); // attention python_lock / python_unlock définissent un block ({ }) les variables déclérées restent locales au bloc

      PyObject *plugin_params_dict=PyDict_New();
      mea_addLong_to_pydict(plugin_params_dict, INTERFACE_ID_STR_C, start_stop_params->i006->id_interface);

      if(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
         mea_addString_to_pydict(plugin_params_dict, INTERFACE_PARAMETERS_STR_C, interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);
      
      mea_call_python_function(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, "mea_init", plugin_params_dict);
      Py_DECREF(plugin_params_dict);

      python_unlock();
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

   start_stop_params->i006->thread=start_interface_type_006_genericserial_data_thread(start_stop_params->i006, start_stop_params->sqlite3_param_db, interface_parameters, (thread_f)_thread_interface_type_006_genericserial_data);
   
   if(start_stop_params->i006->thread!=0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__,start_stop_params->i006->name, launched_successfully_str);
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
      interface_parameters=NULL;
      interface_nb_parameters=0;

   }
   
   if(xpl_callback_params)
   {
      free(xpl_callback_params);
      xpl_callback_params=NULL;
   }
   
   return -1;
}
