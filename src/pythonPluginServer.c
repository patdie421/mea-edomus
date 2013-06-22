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
#include "token_strings.h"

#include "pythonPluginServer.h"

#include "interface_type_002.h"

#include "mea_api.h"


char *plugin_path=NULL;


queue_t *pythonPluginCmd_queue;
pthread_cond_t pythonPluginCmd_queue_cond;
pthread_mutex_t pythonPluginCmd_queue_lock;

PyObject *known_modules;


void _pythonPlugin_thread_cleanup_PyEval_AcquireLock(void *arg)
{
   PyThreadState *tempState=(PyThreadState *)arg;
   
   if(tempState)
      PyThreadState_Swap(tempState);
   PyEval_ReleaseLock();
}


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


mea_error_t pythonPluginServer_init()
{
   pythonPluginCmd_queue=(queue_t *)malloc(sizeof(queue_t));
   if(!pythonPluginCmd_queue)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return ERROR;
   }
   init_queue(pythonPluginCmd_queue);
   
   pthread_mutex_init(&pythonPluginCmd_queue_lock, NULL);
   pthread_cond_init(&pythonPluginCmd_queue_cond, NULL);

   return NOERROR;
}


mea_error_t pythonPluginServer_add_cmd(char *module, char *module_parameters, void *data, int l_data)
{
   pythonPlugin_cmd_t *e=NULL;
   
   e=(pythonPlugin_cmd_t *)malloc(sizeof(pythonPlugin_cmd_t));
   if(!e)
      return ERROR;
   e->python_module=NULL;
   e->data=NULL;
   e->l_data=0;
   
   e->python_module=(char *)malloc(strlen(module)+1);
   if(!e->python_module)
      goto exit_pythonPluginServer_add_cmd;
   strcpy(e->python_module, module);

   e->data=(char *)malloc(l_data);
   if(!e->data)
      goto exit_pythonPluginServer_add_cmd;
   memcpy(e->data,data,l_data);
   
   e->l_data=l_data;
   
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&pythonPluginCmd_queue_lock);
   pthread_mutex_lock(&pythonPluginCmd_queue_lock);
   
   in_queue_elem(pythonPluginCmd_queue, e);
   
   if(pythonPluginCmd_queue->nb_elem>=1)
      pthread_cond_broadcast(&pythonPluginCmd_queue_cond);
   pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
   pthread_cleanup_pop(0);

   return NOERROR;
   
exit_pythonPluginServer_add_cmd:
   if(e)
   {
      if(e->python_module)
         free(e->python_module);
      if(e->data)
         free(e->data);
      e->l_data=0;
      free(e);
   }
   return ERROR;
}


mea_error_t call_pythonPlugin(char *module, int type, PyObject *data_dict)
{
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
         VERBOSE(5) fprintf(stderr, "%s (%s) : %s not found\n", ERROR_STR, __func__, module);
         
         return NOERROR;
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
         case COMMISSIONNING: pFunc = PyObject_GetAttrString(pModule, "mea_commissionningRequest");
            break;
         default:
            return NOERROR;
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
            DEBUG_SECTION fprintf(stderr, "%s (%s) : Result of call: %ld\n", DEBUG_STR, __func__, PyInt_AsLong(pValue));
         }
         else
         {
            VERBOSE(5) fprintf(stderr, "%s (%s) : python error - ", __func__, ERROR_STR);
            PyErr_Print();
            return_code=ERROR;
            goto call_pythonPlugin_clean_exit;
         }
         
         Py_DECREF(pValue); // verifier si nécessaire
      }
      else
      {
         if (PyErr_Occurred())
         {
            VERBOSE(5) fprintf(stderr, "%s (%s) : python error - ", ERROR_STR, __func__);
            PyErr_Print();
         }
         VERBOSE(5) fprintf(stderr, "%s (%s) : mea_edomus_plugin not found\n", ERROR_STR, __func__);
         return_code=ERROR;
         goto call_pythonPlugin_clean_exit;
      }
   }
   else
   {
      VERBOSE(5) fprintf(stderr, "%s (%s) : python error - ", ERROR_STR, __func__);
      PyErr_Print();
      return ERROR;
   }
   
call_pythonPlugin_clean_exit:
   Py_XDECREF(pFunc);
   Py_XDECREF(pModule);

   return return_code;
}


void *_pythonPlugin_thread(void *data)
{
   int ret;
   pythonPlugin_cmd_t *e;
   PyThreadState *mainThreadState, *myThreadState;
   
   Py_Initialize();
   PyEval_InitThreads(); // voir ici http://www.codeproject.com/Articles/11805/Embedding-Python-in-C-C-Part-I
   mainThreadState = PyThreadState_Get();
   myThreadState = PyThreadState_New(mainThreadState->interp);
   PyEval_ReleaseLock();
   
   // chemin vers les plugins rajoutÈ dans le path de l'interprÈteur Python
   PyObject* sysPath = PySys_GetObject((char*)"path");
   PyObject* pluginsDir = PyString_FromString(plugin_path);
   PyList_Append(sysPath, pluginsDir);
   Py_DECREF(pluginsDir);
   
   known_modules=PyDict_New(); // initialisation du cache de module
   
   mea_api_init(); // initialisation du module mea mis à disposition du plugin

   int16_t pass;
   
   while(1)
   {
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&pythonPluginCmd_queue_lock);
      pthread_mutex_lock(&pythonPluginCmd_queue_lock);
   
      pass=0; // pour faire en sorte de n'avoir qu'un seul pthread_mutex_unlock en face du pthread_mutex_lock ci-dessus
      ret=0;
      if(pythonPluginCmd_queue->nb_elem==0)
      {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 30; // timeout de 30 secondes
         ts.tv_nsec = 0;
         
         ret=pthread_cond_timedwait(&pythonPluginCmd_queue_cond, &pythonPluginCmd_queue_lock, &ts);
         if(ret)
         {
            if(ret==ETIMEDOUT)
            {
               DEBUG_SECTION fprintf(stderr,"%s (%s) : Nb elements in queue after TIMEOUT : %ld\n",DEBUG_STR, __func__, pythonPluginCmd_queue->nb_elem);
               pass=1;
            }
            else
            {
               // autres erreurs à traiter
               DEBUG_SECTION fprintf(stderr,"%s (%s) : pthread_cond_timedwait error - ",DEBUG_STR, __func__);
               perror("");
               pass=1;
            }
         }
      }
      
      if (!pass) // pas d'erreur, on récupère un élément dans la queue
         ret=out_queue_elem(pythonPluginCmd_queue, (void **)&e);
      
      pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
      pthread_cleanup_pop(0);

      if (pass) // une erreur, retour au début de la boucle
      {
         pthread_testcancel();
         continue;
      }
      
      if(!ret)
      {
         plugin_queue_elem_t *data = (plugin_queue_elem_t *)e->data;

         PyThreadState *tempState=NULL;
         pthread_cleanup_push(_pythonPlugin_thread_cleanup_PyEval_AcquireLock,tempState);
         PyEval_AcquireLock(); // DEBUG_PyEval_AcquireLock(fn_name, &local_last_time);
         tempState = PyThreadState_Swap(myThreadState);
         
         PyObject *pydict_data=data->aDict;
         
         call_pythonPlugin(e->python_module, data->type_elem, pydict_data);
         Py_DECREF(pydict_data);
         
         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock(); // DEBUG_PyEval_ReleaseLock(fn_name, &local_last_time);
         pthread_cleanup_pop(0);
         
         if(e)
         {
            if(e->python_module)
               free(e->python_module);
            if(e->data)
               free(e->data);
            e->l_data=0;
            free(e);
         }
      }
      else
      {
         // pb d'accés aux données de la file
         VERBOSE(5) fprintf(stderr,"%s (%s) : out_queue_elem - can't access\n", ERROR_STR, __func__);
      }
      pthread_testcancel();
   }
   
   Py_Finalize();
   
   PyThreadState_Clear(myThreadState);
   PyThreadState_Delete(myThreadState);
   
   pthread_exit(NULL);
   
   return NULL;
}


pthread_t *pythonPluginServer(queue_t *plugin_queue)
{
   pthread_t *pythonPlugin_thread=NULL;
   
   if(pythonPluginServer_init())
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't initialize pluginServer\n", FATAL_ERROR_STR, __func__);
      return NULL;
   }
   
   pythonPlugin_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!pythonPlugin_thread)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   
   if(pthread_create (pythonPlugin_thread, NULL, _pythonPlugin_thread, (void *)pythonPluginCmd_queue))
   {
      VERBOSE(1) fprintf(stderr, "%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
      perror("");
      return NULL;
   }
   
   return pythonPlugin_thread;
}


