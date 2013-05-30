//
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/05/13.
//
//
#include <Python.h>

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "error.h"
#include "debug.h"
#include "queue.h"

#include "pythonPluginServer.h"

#include "interface_type_002.h"

#include "mea_api.h"


char *plugin_path=NULL;


queue_t *pythonPluginCmd_queue;
pthread_cond_t pythonPluginCmd_queue_cond;
pthread_mutex_t pythonPluginCmd_queue_lock;

PyObject *known_modules;


void setPythonPluginPath(char *path)
{
   if(plugin_path)
   {
      free(plugin_path);
      plugin_path=NULL;
   }
   plugin_path=malloc(strlen(path)+1);
   
   strcpy(plugin_path, path);
}


int pythonPluginServer_init()
{
   static const char *fn_name="pythonPluginServer_init";
   
   pythonPluginCmd_queue=(queue_t *)malloc(sizeof(queue_t));
   if(!pythonPluginCmd_queue)
   {
      VERBOSE(5) {
         fprintf (stderr, "%s (%s) : %s - ", ERROR_STR, fn_name, MALLOC_ERROR_STR);
         perror("");
      }
      return -1;
   }
   init_queue(pythonPluginCmd_queue);
   
   pthread_mutex_init(&pythonPluginCmd_queue_lock, NULL);
   pthread_cond_init(&pythonPluginCmd_queue_cond, NULL);
   
   return 0;
}


int pythonPluginServer_add_cmd(char *module, char *module_parameters, void *data, int l_data)
{
   pythonPlugin_cmd_t *e=NULL;
   int ret=-1;
   
   e=(pythonPlugin_cmd_t *)malloc(sizeof(pythonPlugin_cmd_t));
   if(!e)
      return -1;
   e->python_module=NULL;
   e->parameters=NULL;
   e->data=NULL;
   e->l_data=0;
   
   //   e->type=type;
   e->python_module=(char *)malloc(strlen(module)+1);
   if(!e->python_module)
      goto exit_pythonPluginServer_add_cmd;
   strcpy(e->python_module, module);
   
   e->parameters=(char *)malloc(strlen(module_parameters)+1);
   if(!e->parameters)
      goto exit_pythonPluginServer_add_cmd;
   strcpy(e->parameters, module_parameters);
   
   e->data=(char *)malloc(l_data);
   if(!e->data)
      goto exit_pythonPluginServer_add_cmd;
   memcpy(e->data,data,l_data);
   
   e->l_data=l_data;
   pthread_mutex_lock(&pythonPluginCmd_queue_lock);
   
   in_queue_elem(pythonPluginCmd_queue, e);
   
   if(pythonPluginCmd_queue->nb_elem>=1)
      pthread_cond_broadcast(&pythonPluginCmd_queue_cond);
   pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
   
   return 0;
   
exit_pythonPluginServer_add_cmd:
   if(e)
   {
      if(e->python_module)
         free(e->python_module);
      if(e->parameters)
         free(e->parameters);
      if(e->data)
         free(e->data);
      e->l_data=0;
      free(e);
   }
   return ret;
}


int call_pythonPlugin2(char *module, int type, PyObject *data_dict)
{
   static const char *fn_name = "call_pythonPlugin2";
   
   PyObject *pName, *pModule, *pFunc;
   PyObject *pArgs, *pValue;
   
   pName = PyString_FromString(module);
   
   pModule = PyDict_GetItem(known_modules, pName);
   if(!pModule)
   {
      pModule = PyImport_Import(pName);
      if(pModule)
         PyDict_SetItem(known_modules, pName, pModule);
      else
      {
         VERBOSE(5) fprintf(stderr, "%s (%s) : %s not found\n", ERROR_STR, fn_name, module);
         
         return -1;
      }
   }
   else
   {
      Py_INCREF(pModule);
      
      char str_module_py[255];
      
      strcpy(str_module_py,plugin_path);
      strcat(str_module_py,"/");
      strcat(str_module_py,module);
      strcat(str_module_py,".");
      strcat(str_module_py,"reload");
      
      int ret=unlink(str_module_py);
      if(!ret)
      {
         pModule = PyImport_ReloadModule(pModule);
         if(pModule)
            PyDict_SetItem(known_modules, pName, pModule);
         else
            PyErr_Print();
      }
   }
   
   Py_DECREF(pName);

   int return_code = 0;
   
   if (pModule != NULL)
   {
      switch(type)
      {
         case XBEEDATA: pFunc = PyObject_GetAttrString(pModule, "mea_dataFromSensor");
            break;
         case XPLMSG: pFunc = PyObject_GetAttrString(pModule, "mea_xplCmndMsg");
            break;
         default:
            return 0;
      }
      
      if (pFunc && PyCallable_Check(pFunc))
      {
         pArgs = PyTuple_New(1); // on passe 4 argument
         
         // data_dict
         Py_INCREF(data_dict); // incrément car PyTuple_SetItem vole la référence
         PyTuple_SetItem(pArgs, 0, data_dict);
         
         pValue = PyObject_CallObject(pFunc, pArgs);
         Py_DECREF(pArgs);
         
         if (pValue != NULL)
         {
            DEBUG_SECTION fprintf(stderr, "%s (%s) : Result of call: %ld\n", DEBUG_STR, fn_name, PyInt_AsLong(pValue));
         }
         else
         {
            VERBOSE(5) fprintf(stderr, "%s (%s) : python error - ", fn_name, DEBUG_STR);
            PyErr_Print();
            return_code=-1;
            goto call_pythonPlugin_clean_exit;
         }
         
         Py_DECREF(pValue); // verifier si nécessaire
      }
      else
      {
         if (PyErr_Occurred())
         {
            VERBOSE(5) fprintf(stderr, "%s (%s) : python error - ", ERROR_STR, fn_name);
            PyErr_Print();
         }
         VERBOSE(5) fprintf(stderr, "%s (%s) : mea_edomus_plugin not found", ERROR_STR, fn_name);
         return_code=-1;
         goto call_pythonPlugin_clean_exit;
      }
   }
   else
   {
      VERBOSE(5) fprintf(stderr, "%s (%s) : python error - ", ERROR_STR, fn_name);
      PyErr_Print();
      return -1;
   }
   
call_pythonPlugin_clean_exit:
   Py_XDECREF(pFunc);
   Py_XDECREF(pModule);

   return return_code;
}


void *_pythonPlugin_thread(void *data)
{
   static const char *fn_name="_pythonPlugin_thread";
   
   pythonPlugin_cmd_t *e;
   
   Py_Initialize();
//   PyEval_InitThreads(); // voir ici http://www.codeproject.com/Articles/11805/Embedding-Python-in-C-C-Part-I
//   PyEval_ReleaseLock();
   
   // chemin vers les plugins rajouté dans le path de l'interpréteur Python
   PyObject* sysPath = PySys_GetObject((char*)"path");
   PyObject* pluginsDir = PyString_FromString(plugin_path);
   PyList_Append(sysPath, pluginsDir);
   Py_DECREF(pluginsDir);

   known_modules=PyDict_New(); // initialisation du cache de module
   mea_api_init(); // initialisation du module mea mis à disposition du plugin
   
   while(1)
   {
      pthread_mutex_lock(&pythonPluginCmd_queue_lock);
      if(pythonPluginCmd_queue->nb_elem==0)
      {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 30; // timeout de 30 secondes
         ts.tv_nsec = 0;
         
         int ret=pthread_cond_timedwait(&pythonPluginCmd_queue_cond, &pythonPluginCmd_queue_lock, &ts);
         if(ret)
         {
            if(ret==ETIMEDOUT)
            {
               DEBUG_SECTION fprintf(stderr,"%s (%s) : Nb elements in queue after TIMEOUT : %ld)\n",DEBUG_STR, fn_name, pythonPluginCmd_queue->nb_elem);
               // pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
               // pthread_testcancel();
               // break;
            }
            else
            {
               // autres erreurs à traiter
            }
            pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
         }
      }
      if(!out_queue_elem(pythonPluginCmd_queue, (void **)&e))
      {
         pthread_mutex_unlock(&pythonPluginCmd_queue_lock);

         plugin_queue_elem_t *data = (plugin_queue_elem_t *)e->data;
         
         PyObject *pydict_data=device_context_to_pydict(data,(int)e->l_data);
         call_pythonPlugin2(e->python_module, data->type_elem, pydict_data);
         Py_DECREF(pydict_data);

         if(e)
         {
            if(e->python_module)
               free(e->python_module);
           if(e->parameters)
               free(e->parameters);
            if(e->data)
               free(e->data);
            e->l_data=0;
            free(e);
         }
      }
      else
      {
         // pb d'accès aux données de la file
         VERBOSE(5) fprintf(stderr,"%s (%s) : out_queue_elem - can't access\n", ERROR_STR, fn_name);
         pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
      }
      pthread_testcancel();
   }

   Py_Finalize();

   pthread_exit(NULL);

   return NULL;
}


pthread_t *pythonPlugin_thread=NULL;

int pythonPluginServer(queue_t *plugin_queue)
{
   static const char *fn_name="pythonPluginServer";
   
   if(pythonPluginServer_init())
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't initialize pluginServer\n", FATAL_ERROR_STR, fn_name);
      exit(1);
   }
   
   pythonPlugin_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!pythonPlugin_thread)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",FATAL_ERROR_STR, fn_name, MALLOC_ERROR_STR);
         perror("");
      }
      exit(1);
   }
   
   if(pthread_create (pythonPlugin_thread, NULL, _pythonPlugin_thread, (void *)pythonPluginCmd_queue))
   {
      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, fn_name);
      perror("");
      exit(1);
   }
   
   return 0;
}


