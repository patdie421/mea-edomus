//
//  interface_type_010.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#include <Python.h>

#include "interface_type_010.h"

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "macros.h"
#include "globals.h"
#include "consts.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "parameters_utils.h"
#include "cJSON.h"

#include "dbServer.h"
#include "processManager.h"
#include "notify.h"
#include "pythonPluginServer.h"
#include "interfacesServer.h"

#include "python_utils.h"


char *interface_type_010_xplin_str="XPLIN";
char *interface_type_010_senttoplugin_str="SENTTOPLUGIN";

char *valid_interface_010_params[]={"S:FSTARTSTR","S:FENDSTR","I:FSIZE","I:FDURATION", "S:PLUGIN", "S:PLUGIN_PARAMETERS", "S:DIRECTION", "S:IN-EXT", "S:OUT-EXT", NULL};
#define INTERFACE_PARAMS_FSTARTSTR         0
#define INTERFACE_PARAMS_FENDSTR           1
#define INTERFACE_PARAMS_FSIZE             2
#define INTERFACE_PARAMS_FDURATION         3
#define INTERFACE_PARAMS_PLUGIN            4
#define INTERFACE_PARAMS_PLUGIN_PARAMETERS 5
#define INTERFACE_PARAMS_DIRECTION         6
#define INTERFACE_PARAMS_IN_EXT            7
#define INTERFACE_PARAMS_OUT_EXT           8

char *valid_plugin_010_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define PLUGIN_PARAMS_PLUGIN       0
#define PLUGIN_PARAMS_PARAMETERS   1


struct thread_params_s
{
   sqlite3 *param_db;
   interface_type_010_t *i010;
};

struct callback_xpl_data_s 
{
   PyThreadState  *mainThreadState; 
   PyThreadState  *myThreadState; 
};

typedef void (*thread_f)(void *);


int start_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg);


void set_interface_type_010_isnt_running(void *data)
{
   interface_type_010_t *i010 = (interface_type_010_t *)data;

   i010->thread_is_running=0;
}


int16_t _interface_type_010_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
//   char *device = NULL;
//   int ret = -1;
   int err = 0;

   interface_type_010_t *i010=(interface_type_010_t *)userValue;
   struct callback_xpl_data_s *callback_data=(struct callback_xpl_data_s *)i010->xPL_callback_data;

   i010->indicators.xplin++;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_plugin_010_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return -1;
   }

   plugin_queue_elem_t *plugin_qelem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_qelem)
   {
      plugin_qelem->type_elem=XPLMSG;
      {
         pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
         PyEval_AcquireLock();
         if(!callback_data->mainThreadState)
            callback_data->mainThreadState=PyThreadState_Get();
         if(!callback_data->myThreadState)
            callback_data->myThreadState = PyThreadState_New(callback_data->mainThreadState->interp);

         PyThreadState *tempState = PyThreadState_Swap(callback_data->myThreadState);

         plugin_qelem->aDict=mea_device_info_to_pydict_device(device_info);
         mea_addLong_to_pydict(plugin_qelem->aDict, "api_key", (long)i010->id_interface);
         PyObject *xplmsg=mea_xplMsgToPyDict2(xplMsgJson);
         PyDict_SetItemString(plugin_qelem->aDict, XPLMSG_STR_C, xplmsg);
         Py_DECREF(xplmsg);

         if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_qelem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
         
         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
         pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      }
      
      pythonPluginServer_add_cmd(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_qelem, sizeof(plugin_queue_elem_t));

      i010->indicators.senttoplugin++;
      free(plugin_qelem);
      plugin_qelem=NULL;
   }

   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;

   return 0;
}


static int init_interface_type_010_data_source_pipe(interface_type_010_t *i010)
{
//   int retour = 0;
   int ret = 0;

   i010->file_desc_in  = -1;
   i010->file_desc_out = -1;
   
   if(i010->direction == DIR_IN || i010->direction == DIR_BOTH)
   {
      char *fname = alloca(strlen(i010->file_name)+strlen(i010->in_ext)+1);
      sprintf(fname,"%s%s", i010->file_name, i010->in_ext);

      unlink(fname);

      ret = mkfifo(fname, 0666);
      if(ret != 0)
      {
         VERBOSE(5) {
            mea_log_printf("%s (%s) : mkfifo in - ", ERROR_STR, __func__);
            perror("");
         }
      }
      else
      {
         int p=open(fname, O_RDWR | O_NONBLOCK);
         if(p==-1)
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : open in - ", ERROR_STR, __func__);
               perror("");
            }

            free(i010->line_buffer);
            i010->line_buffer = NULL;
            i010->line_buffer_l=0;

            return -1;
         }
         else
         {
            i010->file_desc_in=p; 
         }
      }
   }


   if(i010->direction == DIR_OUT || i010->direction == DIR_BOTH)
   {
      char *fname = alloca(strlen(i010->file_name)+strlen(i010->out_ext)+1);
      sprintf(fname,"%s%s", i010->file_name, i010->out_ext);

      unlink(fname);

      ret = mkfifo(fname, 0666);
      if(ret != 0)
      {
         VERBOSE(5) {
            mea_log_printf("%s (%s) : mkfifo out - ", ERROR_STR, __func__);
            perror("");
         }
      }
      else
      {
         int p=open(fname, O_RDWR | O_NONBLOCK);
         if(p==-1)
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : open out - ", ERROR_STR, __func__);
               perror("");
            }

            if(i010->file_desc_in != -1)
            {
               close(i010->file_desc_in);
               i010->file_desc_in = -1;
            }

            free(i010->line_buffer);
            i010->line_buffer = NULL;
            i010->line_buffer_l=0;

            return -1;
         }
         else
         {
            i010->file_desc_out=p;
         }
      }
   }

   if(i010->file_desc_out == -1 && i010->file_desc_in == -1)
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : no fifo opend", ERROR_STR, __func__);
      }
      return -1;
   }

   return 0;
}


static int init_interface_type_010_data_source(interface_type_010_t *i010)
{
//   int ret = 0;

   if(!i010->file_name)
      return -1;

   if(i010->line_buffer)
      free(i010->line_buffer);
   i010->line_buffer_l = INIT_LINE_BUFFER_SIZE;
   i010->line_buffer_ptr = 0;
   i010->line_buffer = (char *)malloc(i010->line_buffer_l);
   if(!i010->line_buffer)
      return -1;

   i010->file_desc_in = -1;
   i010->file_desc_out = -1;

   switch(i010->file_type)
   {
      case FT_PIPE:
      {
         int p=init_interface_type_010_data_source_pipe(i010);
         if(p>=0)
            return 0;
         else
            return -1;
         break;
      }
      default:
         return -1;
   }
}


int init_interface_type_010_data_preprocessor(interface_type_010_t *i010, char *plugin_name, char *plugin_parameters)
{
   int ret = -1;

   mea_python_lock();
      
   PyObject *pName = PyString_FromString(plugin_name);
   if(!pName)
   {
      ret=-1;
      goto init_interface_type_010_data_preprocessor_clean_exit;
   }
   else
   {
      if(!i010->pModule)
      {
         i010->pModule =  PyImport_Import(pName);
         if(!i010->pModule)
         {
            ret=-1;
            goto init_interface_type_010_data_preprocessor_clean_exit;
         }
      }
      else
      {
         PyObject *m = NULL;
         m=i010->pModule;
         i010->pModule=PyImport_ReloadModule(m); // on force le rechargement (c'est pour simplifier)
         Py_DECREF(m);
         if(!i010->pModule)
         {
            ret=-1;
            goto init_interface_type_010_data_preprocessor_clean_exit;
         }
      }

      if(i010->pFunc)
      {
         Py_DECREF(i010->pFunc);
         i010->pFunc=NULL;
      }

      i010->pFunc = PyObject_GetAttrString(i010->pModule, "mea_dataPreprocessor");
      if(!i010->pFunc)
      {
         ret=-1;
         goto init_interface_type_010_data_preprocessor_clean_exit;
      }
      
      if(PyCallable_Check(i010->pFunc))
      {
         if(plugin_parameters)
            i010->pParams=PyString_FromString(plugin_parameters);
         else
            i010->pParams=NULL;
         ret = 0;
      }
      else
      {
         VERBOSE(5) mea_log_printf("%s (%s) : no mea_dataPreprocessor entry point\n", ERROR_STR, __func__);

         Py_XDECREF(i010->pFunc);
         i010->pFunc=NULL;

         Py_XDECREF(i010->pModule);
         i010->pModule=NULL;

         ret = -1;
      }
   }

init_interface_type_010_data_preprocessor_clean_exit:
   if(pName)
   {
      Py_XDECREF(pName);
      pName=NULL;      
   }
   mea_python_unlock();

   return ret;
}


int interface_type_010_data_preprocessor(interface_type_010_t *i010)
{
   int retour=-1;
   if(i010->pFunc)
   {
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *tempState = PyThreadState_Swap(i010->myThreadState);

      PyObject *aDict=PyDict_New();
      if(aDict)
      {
         PyObject *value = PyByteArray_FromStringAndSize((char *)i010->line_buffer, (long)i010->line_buffer_ptr);
         PyDict_SetItemString(aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(aDict, "l_data", (long)i010->line_buffer_ptr);
         mea_addLong_to_pydict(aDict, INTERFACE_ID_STR_C, i010->id_interface);
         mea_addLong_to_pydict(aDict, "api_key", (long)i010->id_interface);

         if(i010->pParams)
            PyDict_SetItemString(aDict, "plugin_paramters", i010->pParams);

         PyObject *res=NULL;
         
         mea_call_python_function3(i010->pFunc, aDict, &res);

         if(!res)
         {
            retour=0;
         }
         else
         {
            if(PyObject_CheckBuffer(res))
            {
               Py_buffer py_packet;
               int ret=PyObject_GetBuffer(res, &py_packet, PyBUF_SIMPLE);
               if(ret<0)
               {
                  VERBOSE(5) mea_log_printf("%s (%s) : python buffer error\n", ERROR_STR, __func__);
                  retour = -1;
               }
               else
               {
                  if(py_packet.len >= i010->line_buffer_l)
                  {
                     int n=(int)(((py_packet.len / INC_LINE_BUFFER_SIZE) + 1) * INC_LINE_BUFFER_SIZE);
                     char *tmp = realloc(i010->line_buffer, n);
                     if(!tmp)
                     {
                        VERBOSE(5) {
                           mea_log_printf("%s (%s) : realloc error - ", ERROR_STR, __func__);
                           perror("");
                        }
                     }
                     else
                     {
                        i010->line_buffer = tmp;
                        i010->line_buffer_l=n;
                     }
                  }

                  memcpy(i010->line_buffer,py_packet.buf,py_packet.len);
                  i010->line_buffer_ptr=(int)(py_packet.len);

                  retour = 1;

                  PyBuffer_Release(&py_packet);
               }
            }
            else if(PyNumber_Check(res))
            {
               retour = (int)PyLong_AsLong(res);
            }

            Py_DECREF(res);
         }

         Py_DECREF(aDict);
      }

      PyThreadState_Swap(tempState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   }

   return retour;
}


static int _interface_type_010_data_to_plugin(interface_type_010_t *i010, sqlite3_stmt *stmt)
{
   int err = 0;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters((char *)sqlite3_column_text(stmt, 3), valid_plugin_010_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return -1;
   }

   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem)
   {
      plugin_elem->type_elem=DATAFROMSENSOR;
      {
         pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
         PyEval_AcquireLock();
         PyThreadState *tempState = PyThreadState_Swap(i010->myThreadState);

         plugin_elem->aDict=mea_stmt_to_pydict_device(stmt);
         mea_addLong_to_pydict(plugin_elem->aDict, "api_key", (long)i010->id_interface);
         mea_addLong_to_pydict(plugin_elem->aDict, DEVICE_TYPE_ID_STR_C, sqlite3_column_int(stmt, 5));

         // les datas
         PyObject *value = PyByteArray_FromStringAndSize((char *)i010->line_buffer, (long)i010->line_buffer_ptr);
         PyDict_SetItemString(plugin_elem->aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(plugin_elem->aDict, "l_data", (long)i010->line_buffer_ptr);

         if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s)
         {
            mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
         }
         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
         pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      }
   }
   pythonPluginServer_add_cmd(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
   i010->indicators.senttoplugin++;

   free(plugin_elem);
   plugin_elem=NULL;
   
   return 0;
}


static int interface_type_010_data_to_plugin(interface_type_010_t *i010, sqlite3 *params_db)
{
//   char *device = NULL;
   int ret = -1;
//   int err = 0;

//   interface_type_010_t *interface=i010;
   char sql[2048];
   sqlite3_stmt * stmt;

   ret=interface_type_010_data_preprocessor(i010);
   if(ret==-1)
      return -1;
   if(ret==1)
   {
   }

   sprintf(sql,"%s WHERE sensors_actuators.deleted_flag <> 1 "
                    "AND sensors_actuators.state='1' "
                    "AND types.typeoftype='0' " // entrée
                    "AND sensors_actuators.id_interface='%d';",
           sql_select_device_info, i010->id_interface);

   ret = sqlite3_prepare_v2(params_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
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
         // traiter ici le message en fonction des données des capteurs/actionneurs
         
/* contenue stmt
0: sensors_actuators.id_sensor_actuator, \
1: sensors_actuators.id_location, \
2: sensors_actuators.state, \
3: sensors_actuators.parameters, \
4: types.parameters, \
5: sensors_actuators.id_type, \
6: lower(sensors_actuators.name), \
7: lower(interfaces.name), \
8: interfaces.id_type, \
9: (SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), \
10: interfaces.dev, \
11: sensors_actuators.todbflag, \
12: types.typeoftype, \
13: sensors_actuators.id_interface \
*/
         _interface_type_010_data_to_plugin(i010, stmt);
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         sqlite3_finalize(stmt);
         break;
      }
   }
   return 0;
}


int haveFrameStartStr(interface_type_010_t *i010)
{
   if(!i010->fstartstr || !i010->fstartstr[0])
      return -1;
   int l=(int)strlen(i010->fstartstr);
   char *s=&(i010->line_buffer[i010->line_buffer_ptr-l]);
   if(strncmp(s, i010->fstartstr, l) == 0)
      return 0;
   else
      return -1;
}


int haveFrameEndStr(interface_type_010_t *i010)
{
   if(!i010->fendstr || !i010->fendstr[0])
      return -1;

   int l=(int)strlen(i010->fendstr);
   char *s=&(i010->line_buffer[i010->line_buffer_ptr-l]);
   if(strncmp(s, i010->fendstr, l) == 0)
      return l;
   else
      return -1;
}


static int process_interface_type_010_data(interface_type_010_t *i010, sqlite3 *params_db)
{
   if(i010->file_desc_in == -1)
      return -1;

   fd_set set;
   struct timeval timeout;
//   char buffer[255];

   mea_timer_t timer;
   mea_init_timer(&timer, 10, 1);
   mea_start_timer(&timer);

   /* Initialize the file descriptor set. */
   FD_ZERO(&set);
   FD_SET(i010->file_desc_in, &set);
   int ret = 0;
   do
   {
      if(mea_test_timer(&timer)==0)
      {
         break;
      }

      /* Initialize the timeout data structure. */
      if(i010->fduration <= 0)
      {
         timeout.tv_sec = 5;
         timeout.tv_usec = 0;
      }
      else
      {
         timeout.tv_sec = i010->fduration / 1000;
         timeout.tv_usec = (i010->fduration % 1000)*1000;
      }

      ret = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
      if(ret == 0)
      {
         if((i010->fduration > 0) && (i010->line_buffer_ptr))
         {
            i010->line_buffer[i010->line_buffer_ptr]=0;
            interface_type_010_data_to_plugin(i010, params_db);
            i010->line_buffer_ptr=0;
         }
         
         break;
      }
      else if(ret < 0)
      {
         // erreur de select
         VERBOSE(5) {
            mea_log_printf("%s (%s) : select error - ", ERROR_STR, __func__);
            perror("");
         }
         break;
      }
      else
      {
         char c;

         int r=(int)read(i010->file_desc_in, &c, 1);
         if(r<1)
         {
            if(r<0)
            {
               VERBOSE(5) {
                  mea_log_printf("%s (%s) : select error - ", ERROR_STR, __func__);
                  perror("");
               }
            }
            break;
         }

         i010->line_buffer[i010->line_buffer_ptr++]=c;
         if(i010->line_buffer_ptr>=i010->line_buffer_l)
         {
            char *tmp = realloc(i010->line_buffer, i010->line_buffer_l + INC_LINE_BUFFER_SIZE);
            if(!tmp)
            {
               i010->line_buffer_ptr = 0;
               VERBOSE(5) {
                  mea_log_printf("%s (%s) : realloc error - ", ERROR_STR, __func__);
                  perror("");
               }
               break;
            }
            else
            {
               i010->line_buffer = tmp;
               i010->line_buffer_l+=INC_LINE_BUFFER_SIZE;
            }
         }

         if(i010->fsize>0 && i010->line_buffer_ptr==i010->fsize)
         {
            i010->line_buffer[i010->line_buffer_ptr]=0;
            
            int b = haveFrameEndStr(i010);
            if(b>0)
            {
               i010->line_buffer_ptr-=b;
               i010->line_buffer[i010->line_buffer_ptr]=0;
            }

            interface_type_010_data_to_plugin(i010, params_db);
            i010->line_buffer_ptr=0;
            break;
         }

         if(i010->fendstr && i010->fendstr[0])
         {
            int b = 0;
            b = haveFrameEndStr(i010);
            if(b>0)
            {
               i010->line_buffer_ptr-=b;
               i010->line_buffer[i010->line_buffer_ptr]=0;
               interface_type_010_data_to_plugin(i010, params_db);
               i010->line_buffer_ptr=0;
               break;
            }
         }

         if(i010->fstartstr && i010->fstartstr[0] && haveFrameStartStr(i010)==0)
         {
            i010->line_buffer_ptr=0;
            break;
         }
      }
   }
   while(ret);
   
   return 0;
}


static int clean_interface_type_010_data_source(interface_type_010_t *i010)
{
   if(i010->file_desc_in >= 0)
      close(i010->file_desc_in);
   char *fname = alloca(strlen(i010->file_name)+strlen(i010->in_ext)+1);
   sprintf(fname,"%s%s", i010->file_name, i010->in_ext);
   unlink(fname);

   if(i010->file_desc_out >= 0)
      close(i010->file_desc_out);
   fname = alloca(strlen(i010->file_name)+strlen(i010->out_ext)+1);
   sprintf(fname,"%s%s", i010->file_name, i010->out_ext);
   unlink(fname);

   i010->line_buffer_l = 0;
   i010->line_buffer_ptr = 0;
   free(i010->line_buffer);
   i010->line_buffer = NULL;

   return 0;
}


int clean_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010->parameters)
   {
      free(i010->parameters);
      i010->parameters=NULL;
   }

   if(i010->xPL_callback2)
      i010->xPL_callback2=NULL;

   if(i010->thread)
   {
      free(i010->thread);
      i010->thread=NULL;
   }

   if(i010->file_name)
   {
      free(i010->file_name);
      i010->file_name=NULL;
   }

   if(i010->in_ext)
   {
      free(i010->in_ext);
      i010->in_ext=NULL;
   }

   if(i010->out_ext)
   {
      free(i010->out_ext);
      i010->out_ext=NULL;
   }

   if(i010->fstartstr)
   {
      free(i010->fstartstr);
      i010->fstartstr=NULL;
   }

   if(i010->fendstr)
   {
      free(i010->fendstr);
      i010->fendstr=NULL;
   }

   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();

   if(i010->pFunc)
   {
      Py_XDECREF(i010->pFunc);
      i010->pFunc=NULL;
   }

   if(i010->pModule)
   {
      Py_XDECREF(i010->pModule);
      i010->pModule=NULL;
   }

   if(i010->pParams)
   {
      Py_XDECREF(i010->pParams);
      i010->pParams=NULL;
   }
 
   if(i010->myThreadState)
   {
      PyThreadState_Clear(i010->myThreadState);
      PyThreadState_Delete(i010->myThreadState);
      i010->myThreadState=NULL;
   }

   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

   return 0;
}


xpl2_f get_xPLCallback_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL)
      return NULL;
   else
   {
      return i010->xPL_callback2;
   }
}


int get_monitoring_id_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL)
      return -1;
   else
      return i010->monitoring_id;
}


int set_xPLCallback_interface_type_010(void *ixxx, xpl2_f cb)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL)
      return -1;
   else
   {
      i010->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_010(void *ixxx, int id)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL)
      return -1;
   else
   {
      i010->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_010()
{
   return INTERFACE_TYPE_010;
}


int get_interface_id_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL)
      return -1;
   else
      return i010->id_interface;
}


static int api_write_data(interface_type_010_t *ixxx, PyObject *args, PyObject **res, int16_t *nerr, char *err, int l_err)
{
   if(ixxx->file_desc_out == -1)
   {
      *nerr=253;
      return -253;
   }
   PyObject *arg;
   int16_t ret;
   *nerr=255;
   *res = NULL;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=3)
      return -255;

   Py_buffer py_packet;
   arg=PyTuple_GetItem(args, 2);
   if(PyObject_CheckBuffer(arg))
   {
      ret=PyObject_GetBuffer(arg, &py_packet, PyBUF_SIMPLE);
      if(ret<0)
      return -255;
   }
   else
      return -255;

   ret=write(ixxx->file_desc_out, py_packet.buf, py_packet.len);
   if(ret<0)
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : write - ", ERROR_STR, __func__);
         perror("");
      }
   }
 
   PyBuffer_Release(&py_packet);
 
   nerr=0;

   return 0;
}


int16_t api_interface_type_010(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
//   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   PyObject *pyArgs = (PyObject *)args;
   PyObject **pyRes = (PyObject **)res;
   
   if(strcmp(cmnd, "mea_writeData") == 0)
   {
      int ret=api_write_data(ixxx, pyArgs, pyRes, nerr, err, l_err);
      if(ret<0)
      {
         strncpy(err, "error", l_err);
         return -1;
      }
      else
      {
         strncpy(err, "no error", l_err);
         *nerr=0;
         return 0;
      }
   }
/*
   else if(strcmp(cmnd, "test") == 0)
   {
      *res = PyString_FromString("New style Api call OK !!!");
      *nerr=0;
      strncpy(err, "no error", l_err);

      return 0;
   }
*/
   else
   {
      strncpy(err, "unknown function", l_err);

      return -254;
   }

   return -1;
}


void *_thread_interface_type_010_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   if(!params)
      return NULL;

   return NULL;
}


interface_type_010_t *malloc_and_init_interface_type_010(sqlite3 *sqlite3_param_db, int id_driver, int id_interface, char *name, char *dev, char *parameters, char *description)
{
   interface_type_010_t *i010;
                  
   i010=(interface_type_010_t *)malloc(sizeof(interface_type_010_t));
   if(!i010)
   {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   i010->thread_is_running=0;
   i010->file_name = NULL;

                  
   struct interface_type_010_data_s *i010_start_stop_params=(struct interface_type_010_data_s *)malloc(sizeof(struct interface_type_010_data_s));
   if(!i010_start_stop_params)
   {
      free(i010);
      i010=NULL;

      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }  
      return NULL;
   }

   strncpy(i010->dev, (char *)dev, sizeof(i010->dev)-1);
   strncpy(i010->name, (char *)name, sizeof(i010->name)-1);
   i010->id_interface=id_interface;
   i010->id_driver=id_driver;
   i010->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(i010->parameters)
      strcpy(i010->parameters,(char *)parameters);

   i010->indicators.xplin=0;

   i010->line_buffer = NULL;
   i010->line_buffer_l=0;
   i010->line_buffer_ptr=0;
   i010->fduration = 2000;
   i010->fendstr = NULL;
   i010->fstartstr = NULL;
   i010->fsize = 0;
   i010->direction = DIR_IN;
   i010->in_ext = malloc(4);
   if(i010->in_ext)
      strcpy(i010->in_ext,"-in");
   i010->out_ext = malloc(5);
   if(i010->out_ext)
      strcpy(i010->out_ext,"-out");
   i010->pModule = NULL;
   i010->pFunc = NULL;
   i010->pParams = NULL;
   i010->mainThreadState=NULL;
   i010->myThreadState=NULL;


   parsed_parameters_t *interface_params=NULL;
   int nb_interface_params;
   int err;
   interface_params=alloc_parsed_parameters(parameters, valid_interface_010_params, &nb_interface_params, &err, 0);
   if(!interface_params)
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : alloc_parsed_parameters error\n", ERROR_STR, __func__);
      }
   }
   else
   {
      if(interface_params->parameters[INTERFACE_PARAMS_IN_EXT].label)
      {
         char *tmp = realloc(i010->in_ext, strlen(interface_params->parameters[INTERFACE_PARAMS_IN_EXT].value.s)+1);
         if(tmp)
         {
            i010->in_ext=tmp;
            strcpy(i010->in_ext, interface_params->parameters[INTERFACE_PARAMS_IN_EXT].value.s);
         } 
      }

      if(interface_params->parameters[INTERFACE_PARAMS_OUT_EXT].label)
      {
         char *tmp = realloc(i010->out_ext, strlen(interface_params->parameters[INTERFACE_PARAMS_OUT_EXT].value.s)+1);
         if(tmp)
         {
            i010->out_ext=tmp;
            strcpy(i010->out_ext, interface_params->parameters[INTERFACE_PARAMS_OUT_EXT].value.s);
         } 
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FSIZE].label)
      {
         if(interface_params->parameters[INTERFACE_PARAMS_FSIZE].value.i>0)
            i010->fsize=interface_params->parameters[INTERFACE_PARAMS_FSIZE].value.i;
      }

      if(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].label)
      {
         if(mea_strcmplower(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s, "IN")==0)
         {
            i010->direction=DIR_IN;
         }
         else if(mea_strcmplower(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s, "OUT")==0)
         {
            i010->direction=DIR_OUT;
         }
         else if(mea_strcmplower(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s, "BOTH")==0)
         {
            i010->direction=DIR_BOTH;
         }
         else
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : direction parameter error - \"%s\" unknown and not used\n", ERROR_STR, __func__, interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s);
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FDURATION].label)
      {
         if(interface_params->parameters[INTERFACE_PARAMS_FDURATION].value.i>0)
         {
            i010->fduration=interface_params->parameters[INTERFACE_PARAMS_FDURATION].value.i;
            if(i010->fduration<0)
               i010->fduration=0;
            if(i010->fduration>5000)
               i010->fduration=5000; // 5 secondes max
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].label)
      {
         int l=(int)strlen(interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].value.s);
         if(l>0)
         {
            i010->fstartstr=malloc(l+1);
            if(i010->fstartstr)
            {
               mea_unescape(i010->fstartstr, interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].value.s);
               i010->fstartstr = realloc(i010->fstartstr, strlen(i010->fstartstr)+1);
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FENDSTR].label)
      {
         int l=(int)strlen(interface_params->parameters[INTERFACE_PARAMS_FENDSTR].value.s);
         if(l>0)
         {
            i010->fendstr=malloc(l+1);
            if(i010->fendstr)
            {
               mea_unescape(i010->fendstr, interface_params->parameters[INTERFACE_PARAMS_FENDSTR].value.s);
               i010->fendstr = realloc(i010->fendstr, strlen(i010->fendstr)+1);
            }
         }
      }


      if(interface_params->parameters[INTERFACE_PARAMS_PLUGIN].label)
      {
         init_interface_type_010_data_preprocessor(i010, interface_params->parameters[INTERFACE_PARAMS_PLUGIN].value.s, interface_params->parameters[INTERFACE_PARAMS_PLUGIN_PARAMETERS].value.s);
      }

      release_parsed_parameters(&interface_params);
   }


   char *tmpstr1=alloca(strlen(dev)+1);
   char *tmpstr2=alloca(strlen(dev)+1);
   i010->file_type=0;
   int n=sscanf(i010->dev, "%[^:]://%[^\n]", tmpstr1, tmpstr2);
   if(n==2)
   {
      i010->file_name=(char *)malloc(strlen(tmpstr2)+1+1);
      if(i010->file_name)
      {
         if(strcmp(tmpstr1,"PIPE")==0)
         {
            sprintf(i010->file_name, "/%s", tmpstr2);
            i010->file_type=FT_PIPE;
         }
      }
   }

   DEBUG_SECTION {
      fprintf(stderr,"DEV       =%s\n", i010->dev);
      fprintf(stderr,"FSIZE     =%d\n", i010->fsize);
      fprintf(stderr,"FDURATION =%d\n", i010->fduration);
      fprintf(stderr,"FSTARTSTR =%s\n", i010->fstartstr);
      fprintf(stderr,"FENDSTR   =%s\n", i010->fendstr);
      fprintf(stderr,"FILE_NAME =%s (%d)\n", i010->file_name, n);
      fprintf(stderr,"FILE_TYPE =%d\n", i010->file_type);
   }

   i010->thread=NULL;
   i010->xPL_callback2=NULL;
   i010->xPL_callback_data=NULL;
   i010->monitoring_id=process_register((char *)name);

   i010_start_stop_params->sqlite3_param_db = sqlite3_param_db;
   i010_start_stop_params->i010=i010;
                  
   process_set_group(i010->monitoring_id, 1);
   process_set_start_stop(i010->monitoring_id, start_interface_type_010, stop_interface_type_010, (void *)i010_start_stop_params, 1);
   process_set_watchdog_recovery(i010->monitoring_id, restart_interface_type_010, (void *)i010_start_stop_params);
   process_set_description(i010->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i010->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i010->monitoring_id, interface_type_010_xplin_str, 0);
   process_add_indicator(i010->monitoring_id, interface_type_010_senttoplugin_str, 0);

   return i010;
}


void *_thread_interface_type_010(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;
//   int ret;

   pthread_cleanup_push( (void *)_thread_interface_type_010_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_010_isnt_running, (void *)params->i010 );

   params->i010->thread_is_running=1;
   process_heartbeat(params->i010->monitoring_id);

   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();

   params->i010->mainThreadState = PyThreadState_Get();
   params->i010->myThreadState   = PyThreadState_New(params->i010->mainThreadState->interp);

   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

   if(params->i010->pModule)
   {
//      mea_python_lock(); // attention python_lock / python_unlock définissent un block ({ }) les variables déclérées restent locales au bloc
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *tempState = PyThreadState_Swap(params->i010->myThreadState);

      PyObject *plugin_params_dict=PyDict_New();
      mea_addLong_to_pydict(plugin_params_dict, INTERFACE_ID_STR_C, params->i010->id_interface);
      PyDict_SetItemString(plugin_params_dict, INTERFACE_PARAMETERS_STR_C, params->i010->pParams);

      mea_call_python_function_from_module(params->i010->pModule, "mea_init", plugin_params_dict);

      Py_DECREF(plugin_params_dict);

//      mea_python_unlock();
      PyThreadState_Swap(tempState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   }

   while(1)
   {
      process_heartbeat(params->i010->monitoring_id);
      process_update_indicator(params->i010->monitoring_id, interface_type_010_xplin_str, params->i010->indicators.xplin);
      process_update_indicator(params->i010->monitoring_id, interface_type_010_xplin_str, params->i010->indicators.senttoplugin);

      // traiter les données en provenance des périphériques
      if(params->i010->file_desc_in != -1)
         process_interface_type_010_data(params->i010, params->param_db);
      else
         sleep(1);

      pthread_testcancel();
   }

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
}


pthread_t *start_interface_type_010_thread(interface_type_010_t *i010, void *fd, sqlite3 *db, thread_f thread_function)
{
   pthread_t *thread=NULL;
   struct thread_params_s *thread_params=NULL;
//   struct callback_data_s *callback_data=NULL;

   thread_params=malloc(sizeof(struct thread_params_s));
   if(!thread_params)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }

   thread_params->param_db=db;
   thread_params->i010=(void *)i010;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }

   if(pthread_create (thread, NULL, (void *)thread_function, (void *)thread_params))
      goto clean_exit;

   pthread_detach(*thread);

   return thread;

clean_exit:
   if(thread)
   {
      free(thread);
      thread=NULL;
   }

   if(thread_params)
   {
      free(thread_params);
      thread_params=NULL;
   }
   return NULL;
}


int stop_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_010_data_s *start_stop_params=(struct interface_type_010_data_s *)data;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i010->name);

   if(start_stop_params->i010->xPL_callback2)
      start_stop_params->i010->xPL_callback2=NULL;
      
   if(start_stop_params->i010->xPL_callback_data)
   {
      struct callback_xpl_data_s *data = (struct callback_xpl_data_s *)start_stop_params->i010->xPL_callback_data;
      PyEval_AcquireLock(); 
     
      if(data->myThreadState)
      { 
         PyThreadState_Clear(data->myThreadState); 
         PyThreadState_Delete(data->myThreadState); 
         data->myThreadState=NULL; 
      }
      PyEval_ReleaseLock(); 
      
      free(start_stop_params->i010->xPL_callback_data);
      start_stop_params->i010->xPL_callback_data=NULL;
   }

   clean_interface_type_010_data_source(start_stop_params->i010);

   if(start_stop_params->i010->thread)
   {
      pthread_cancel(*(start_stop_params->i010->thread));

      int counter=100;
      while(counter--)
      {
         if(start_stop_params->i010->thread_is_running)
         {
            usleep(100);
         }
         else
            break;
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop(s)\n", DEBUG_STR, __func__, start_stop_params->i010->name, 100-counter);

      free(start_stop_params->i010->thread);
      start_stop_params->i010->thread=NULL;
   }

   mea_notify_printf('S', "%s %s", start_stop_params->i010->name, stopped_successfully_str);

   return 0;
}


int restart_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char err_str[128];
   struct callback_xpl_data_s *xpl_callback_params=NULL;
   struct interface_type_010_data_s *start_stop_params=(struct interface_type_010_data_s *)data;

   if(init_interface_type_010_data_source(start_stop_params->i010)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : cant'open start interface %s, initialisation error", INFO_STR, __func__, start_stop_params->i010->name);
      return -1;
   }

   xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s));
   if(!xpl_callback_params)
   {
       strerror_r(errno, err_str, sizeof(err_str));
       VERBOSE(2) {
          mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
       }
       mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i010->name, err_str);
       goto clean_exit;
   }
   xpl_callback_params->mainThreadState=NULL;
   xpl_callback_params->myThreadState=NULL;

   start_stop_params->i010->thread=start_interface_type_010_thread(start_stop_params->i010, NULL, start_stop_params->sqlite3_param_db, (thread_f)_thread_interface_type_010);

   start_stop_params->i010->xPL_callback_data=NULL;
   start_stop_params->i010->xPL_callback2=_interface_type_010_xPL_callback2;

   return 0;

clean_exit:
   if(start_stop_params->i010->xPL_callback_data)
   {
      free(start_stop_params->i010->xPL_callback_data);
      start_stop_params->i010->xPL_callback_data=NULL;
   }

   clean_interface_type_010_data_source(start_stop_params->i010);

   return -1; 
}


#ifndef ASPLUGIN
int get_fns_interface_type_010(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init = (malloc_and_init_f)&malloc_and_init_interface_type_010;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_010;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_010;
   interfacesFns->clean = (clean_f)&clean_interface_type_010;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_010;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_010;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_010;
   interfacesFns->api = (api_f)&api_interface_type_010;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
