//
//  automatorServer2.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 13/03/2015.
//
//
#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include <getopt.h>

#include "automatorServer2.h"

#include "xPL.h"

#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
#include "consts.h"

#include "python_utils.h"
#include "string_utils.h"
#include "processManager.h"
#include "sockets_utils.h"
#include "notify.h"

#define XPL_VERSION "0.1a2"

char *acquire_name_str="ACQUIRE";
char *automator1_name_str="AUTOMATOR";

xPL_ServicePtr xPLService = NULL;
char xpl_vendorID[9];
char xpl_deviceID[9];
char xpl_instanceID[17];

pthread_mutex_t automator_lock;

// gestion du thread et des indicateurs
pthread_t *_acquire_thread_id = NULL;
int _acquire_monitoring_id = -1;
volatile sig_atomic_t _acquire_thread_is_running=0;

pthread_t *_automator1_thread_id = NULL;
int _automator1_monitoring_id = -1;
volatile sig_atomic_t _automator1_thread_is_running=0;

PyObject *automator_known_modules=NULL;

//char *python_automator_path="/Data/test5/lib/mea-plugins";
char python_automator_path[256]="";

PyObject *automator_memory=NULL;

static PyObject *mea_automator_getMemory(PyObject *self, PyObject *args);

static PyMethodDef MeaMethods[] = {
   {"getMemory", mea_automator_getMemory, METH_VARARGS, "Return a dictionary"},
   {NULL, NULL, 0, NULL}
};


static PyObject *mea_automator_getMemory(PyObject *self, PyObject *args)
{
   return mea_getMemory(self, args, automator_memory);
}


void automator_modules_init()
{
   automator_memory=PyDict_New(); // initialisation de la mémoire
   
   Py_InitModule("mea_automator", MeaMethods);
}


void automator_modules_release()
{
// /!\ a ecrire pour librérer tous le contenu de la memoire partagé ...
   if(automator_memory)
      Py_DECREF(automator_memory);
}

int callPythonFunction(char *module_path, char *module, char *function, PyObject *data_dict)
{
   PyObject *pName, *pModule, *pFunc;
   PyObject *pArgs, *pValue;

   PyErr_Clear();
   
   pName = PyString_FromString(module);
   
   pModule = PyDict_GetItem(automator_known_modules, pName);
   if(!pModule)
   {
      pModule = PyImport_Import(pName);
      if(pModule)
      {
         PyDict_SetItem(automator_known_modules, pName, pModule);
      }
      else
      {
         VERBOSE(5) mea_log_printf("%s (%s) : module %s not found.\n", ERROR_STR, __func__, module);
         return NOERROR;
      }
   }
   else
   {
      Py_INCREF(pModule);
      
      char str_module_py[255];
      
      strcpy(str_module_py,module_path);
      strcat(str_module_py,"/");
      strcat(str_module_py,module);
      strcat(str_module_py,".");
      strcat(str_module_py,"reload");
      
      int ret=unlink(str_module_py);
      if(!ret)
      {
         pModule = PyImport_ReloadModule(pModule);
         if(pModule)
         {
            PyDict_SetItem(automator_known_modules, pName, pModule);
         }
         else
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(stderr, "\n");
            }
         }
      }
   }
   
   Py_DECREF(pName);

   int return_code = 0;
   if (pModule != NULL)
   {
      pFunc = PyObject_GetAttrString(pModule, function);

      if (pFunc && PyCallable_Check(pFunc))
      {
         pArgs = PyTuple_New(1);

         Py_INCREF(data_dict); // incrément car PyTuple_SetItem vole la référence
         PyTuple_SetItem(pArgs, 0, data_dict);
         pValue = PyObject_CallObject(pFunc, pArgs);
         Py_DECREF(pArgs);

         if (pValue != NULL)
         {
//            DEBUG_SECTION mea_log_printf("%s (%s) : result of call of %s : %ld\n", DEBUG_STR, __func__, function, PyInt_AsLong(pValue));
         }
         else
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(stderr, "\n");
            }
            return_code=-1;
            goto callPythonFunction_clean_exit;
         }
         
         Py_DECREF(pValue); // verifier si nécessaire
      }
      else
      {
         if (PyErr_Occurred())
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - (%s/%s) - ", ERROR_STR, __func__, module, function);
               PyErr_Print();
               fprintf(stderr, "\n");
            }
            return_code=-1;
            goto callPythonFunction_clean_exit;
         }
         else
         {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python unknown error\n", ERROR_STR, __func__);
            }
            return_code=-1;
            goto callPythonFunction_clean_exit;
         }
      }
   }
   else
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : -python error - ", ERROR_STR, __func__);
         PyErr_Print();
         fprintf(stderr, "\n");
      }
      return -1;
   }
   
callPythonFunction_clean_exit:
   Py_XDECREF(pFunc);
   Py_XDECREF(pModule);

   return return_code;
}


PyObject *mea_xplMsgToPyDict(xPL_MessagePtr xplMsg)
{
   PyObject *pyXplMsg = NULL;
   PyObject *s = NULL;
//   PyObject *l = NULL;
   char tmpStr[35]=""; // chaine temporaire. Taille max pour vendorID(8) + "-"(1) + deviceID(8) + "."(1) + instanceID(16)
   
   pyXplMsg = PyDict_New();
   if(!pyXplMsg)
   {
      return NULL;
   }

/*
   // xplmsg
   l = PyLong_FromLong(1L);
   PyDict_SetItemString(pyXplMsg, "xplmsg", l);
   Py_DECREF(l);
*/

   // message-type
   switch(xPL_getMessageType(xplMsg))
   {
      case xPL_MESSAGE_COMMAND:
         s=PyString_FromString("xpl-cmnd");
         break;
      case xPL_MESSAGE_STATUS:
         s= PyString_FromString("xpl-stat");
         break;
      case xPL_MESSAGE_TRIGGER:
         s= PyString_FromString("xpl-trig");
         break;
      default:
         return NULL;
   }
   PyDict_SetItemString(pyXplMsg, "message_xpl_type", s);
   Py_DECREF(s);
   
   // hop
   sprintf(tmpStr,"%d",xPL_getHopCount(xplMsg));
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "hop", s);
   Py_DECREF(s);

   // source
   sprintf(tmpStr,"%s-%s.%s", xPL_getSourceVendor(xplMsg), xPL_getSourceDeviceID(xplMsg), xPL_getSourceInstanceID(xplMsg));
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "source", s);
   Py_DECREF(s);

   if (xPL_isBroadcastMessage(xplMsg))
   {
      strcpy(tmpStr,"*");
   }
   else
   {
      sprintf(tmpStr,"%s-%s.%s", xPL_getTargetVendor(xplMsg), xPL_getTargetDeviceID(xplMsg), xPL_getTargetInstanceID(xplMsg));
   }
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "target", s);
   Py_DECREF(s);
   
   // schema
   sprintf(tmpStr,"%s.%s", xPL_getSchemaClass(xplMsg), xPL_getSchemaType(xplMsg));
   s=PyString_FromString(tmpStr);
   PyDict_SetItemString(pyXplMsg, "schema", s);
   Py_DECREF(s);
   
   // body
   PyObject *pyBody=PyDict_New();
   xPL_NameValueListPtr body = xPL_getMessageBody(xplMsg);
   int n = xPL_getNamedValueCount(body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
      if (keyValuePtr->itemValue != NULL)
      {
         s=PyString_FromString(keyValuePtr->itemValue);
      }
      else
      {
         s=Py_None;
         Py_INCREF(s);
      }
      PyDict_SetItemString(pyBody, keyValuePtr->itemName, s);
      Py_DECREF(s);
   }
   
   PyDict_SetItemString(pyXplMsg, "xpl-body", pyBody);
   Py_DECREF(pyBody);
   
   return pyXplMsg;
}


int16_t _displayXPLMsg(xPL_MessagePtr theMessage)
{
   char xpl_source[48]="";
   char xpl_destination[48]="";
   char xpl_schema[48]="";

   char *xpl_msg_any = "xpl-any";
   char *xpl_msg_command = "xpl-cmnd";
   char *xpl_msg_status = "xpl-stat";
   char *xpl_msg_trigger = "xpl-trig";

   snprintf(xpl_source, sizeof(xpl_source), "%s-%s.%s", xPL_getSourceVendor(theMessage), xPL_getSourceDeviceID(theMessage), xPL_getSourceInstanceID(theMessage));
   if(xPL_isBroadcastMessage(theMessage))
   {
      strcpy(xpl_destination, "*");
   }
   else
   {
      snprintf(xpl_destination,sizeof(xpl_destination),"%s-%s.%s", xPL_getTargetVendor(theMessage), xPL_getTargetDeviceID(theMessage), xPL_getTargetInstanceID(theMessage));
   }

   xPL_MessageType msgType = xPL_getMessageType(theMessage);
   char *msgTypeStrPtr = "";
   switch(msgType)
   {
      case xPL_MESSAGE_ANY:
         msgTypeStrPtr = xpl_msg_any;
         break;
      case xPL_MESSAGE_COMMAND:
         msgTypeStrPtr = xpl_msg_command;
         break;
      case xPL_MESSAGE_STATUS:
         msgTypeStrPtr = xpl_msg_status;
         break;
      case xPL_MESSAGE_TRIGGER:
         msgTypeStrPtr = xpl_msg_trigger;
         break;
      default:
         msgTypeStrPtr = "???";
         break;
   }
   
   snprintf(xpl_schema,sizeof(xpl_schema),"%s.%s", xPL_getSchemaClass(theMessage), xPL_getSchemaType(theMessage));

   fprintf(stderr, "%s: source = %s, destination = %s, schema = %s, body = [", msgTypeStrPtr, xpl_source, xpl_destination
   , xpl_schema);

   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);

      if(i)
         fprintf(stderr, ", ");
      fprintf(stderr, "%s = %s", keyValuePtr->itemName, keyValuePtr->itemValue);
   }
   fprintf(stderr, "]\n");
   return 0;
}


int xPLAcquire(xPL_MessagePtr xplMsg, PyThreadState *myThreadState)
{
 PyObject *aDict=NULL;
 PyThreadState *tempState=NULL;
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(automator_lock) );
   pthread_mutex_lock(&(automator_lock));

   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   tempState = PyThreadState_Swap(myThreadState);
         
   aDict=PyDict_New();

   PyObject *d=mea_xplMsgToPyDict(xplMsg);
   PyDict_SetItemString(aDict, "xplmsg", d);
   Py_DECREF(d);
   callPythonFunction(python_automator_path, "automator", "acquire", aDict);
   Py_DECREF(aDict);

   PyThreadState_Swap(tempState);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

   pthread_mutex_unlock(&(automator_lock));
   pthread_cleanup_pop(0);
   
   return 0;
}

int16_t set_xpl_address(char *xpladdr) // a compléter ... récupérer info sur la ligne de commande
/**
 * \brief     initialise les données pour l'adresse xPL
 * \details   positionne vendorID, deviceID et instanceID
 * \param     params_liste  liste des parametres.
 * \return   -1 en cas d'erreur, 0 sinon
 */
{
   int n=0,r=0;
   r=sscanf(xpladdr,"%8[a-zA-Z0-9]-%8[a-zA-Z0-9].%16[a-zA-Z0-9]%n", xpl_vendorID, xpl_deviceID, xpl_instanceID, &n);
   if(r!=3 || n!=strlen(xpladdr))
   {
      xpl_vendorID[0]=0;
      xpl_deviceID[0]=0;
      xpl_instanceID[0]=0;
      
      return -1;
   }
   return 0;
}


void process_msg(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   PyThreadState *myThreadState=(PyThreadState *)userValue;
   xPLAcquire(theMessage, myThreadState);
//   _displayXPLMsg(theMessage);
}


void set_acquire_isnt_running(void *data)
{
   _acquire_thread_is_running=0;
}


void clean_acquire(void *data)
{
   PyThreadState *myThreadState=NULL;
   
   xPL_removeMessageListener(process_msg);
   xPL_setServiceEnabled(xPLService, FALSE);
   xPL_releaseService(xPLService);
   
   myThreadState=(PyThreadState *)data;
   if(myThreadState)
   {
      PyEval_AcquireLock();
      PyThreadState_Clear(myThreadState);
      PyThreadState_Delete(myThreadState);
      PyEval_ReleaseLock();
      myThreadState=NULL;
   }
}


void *acquire_thread(void *data)
{
   PyThreadState *mainThreadState, *myThreadState=NULL;

   pthread_cleanup_push( (void *)set_acquire_isnt_running, (void *)NULL );
   
   _acquire_thread_is_running=1;
   process_heartbeat(_acquire_monitoring_id); // 1er heartbeat après démarrage.
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   mainThreadState = PyThreadState_Get();
   myThreadState = PyThreadState_New(mainThreadState->interp);
   pthread_cleanup_push( (void *)clean_acquire, myThreadState);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();
   
//   xPL_setDebugging(TRUE); // xPL en mode debug
   
   xPLService = xPL_createService(xpl_vendorID, xpl_deviceID, xpl_instanceID);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setHeartbeatInterval(xPLService, 5000); // en milliseconde
   xPL_addMessageListener(process_msg, myThreadState);
   
   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      pthread_testcancel();
      process_heartbeat(_acquire_monitoring_id); // heartbeat après chaque boucle
      
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            mea_log_printf("%s  (%s) : %s thread is running\n", INFO_STR, __func__, acquire_name_str);
         }
         else
            compteur++;
      }

      xPL_processMessages(500);
   }
   while (1);

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
}


pthread_t *acquire()
{
   if(!xpl_deviceID || !xpl_instanceID || !xpl_vendorID)
   {
      return NULL;
   }
   
   _acquire_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_acquire_thread_id)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   if(pthread_create (_acquire_thread_id, NULL, acquire_thread, NULL))
   {
      VERBOSE(1) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }
      
   return _acquire_thread_id;
}


int stop_acquire(int my_id, void *data,  char *errmsg, int l_errmsg)
{
   int ret=0;
   
   if(_acquire_thread_id)
   {
      pthread_cancel(*_acquire_thread_id);
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(_acquire_thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 1 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }

      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop\n",DEBUG_STR, __func__,acquire_name_str,100-counter);
      ret=stopped;
   }

   _acquire_monitoring_id=-1;

   if(_acquire_thread_id)
   {
      free(_acquire_thread_id);
      _acquire_thread_id=NULL;
   }   

   xPL_shutdown();

   if(ret==0)
   {
      VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, acquire_name_str, stopped_successfully_str);
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s  (%s) : %s can't cancel thread.\n", INFO_STR, __func__, acquire_name_str);
   }
   return ret;
}


int start_acquire(int my_id, void *data, char *errmsg, int l_errmsg)
{
//   struct acquire_start_stop_params_s *acquire_start_stop_params = (struct acquire_start_stop_params_s *)data;
   char err_str[256]="";

   _acquire_monitoring_id=my_id;

#ifdef __APPLE__
//   xPL_setBroadcastInterface("en0"); // comprendre pourquoi ca ne marche plus sans ...
#endif
   if ( !xPL_initialize(xPL_getParsedConnectionType()) )
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : xPL_initialize - error\n",ERROR_STR,__func__);
      }
      return -1;
   }

   _acquire_thread_id=acquire();

   if(_acquire_thread_id==NULL)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't start acquire process - %s\n",ERROR_STR,__func__,err_str);
      }

      return -1;
   }
   else
   {
      pthread_detach(*_acquire_thread_id);
      VERBOSE(2) {
         mea_log_printf("%s  (%s) : %s launched successfully.\n", INFO_STR, __func__, acquire_name_str);
      }
      
      return 0;
   }
}


int restart_acquire(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_acquire(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_acquire(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}


void set_automator_isnt_running(void *data)
{
   _automator1_thread_is_running=0;
}


void clean_automator(void *data)
{
   PyThreadState *myThreadState=(PyThreadState *)data;
   if(myThreadState)
   {
      PyEval_AcquireLock();
      PyThreadState_Clear(myThreadState);
      PyThreadState_Delete(myThreadState);
      PyEval_ReleaseLock();
      myThreadState=NULL;
   }
}


void *automator_thread(void *data)
{
   PyThreadState *mainThreadState=NULL, *myThreadState=NULL;
   PyObject *aDict=NULL;

   pthread_cleanup_push( (void *)set_automator_isnt_running, (void *)NULL );
   
   _automator1_thread_is_running=1;
   process_heartbeat(_automator1_monitoring_id); // 1er heartbeat après démarrage.
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   mainThreadState = PyThreadState_Get();
   myThreadState = PyThreadState_New(mainThreadState->interp);
   pthread_cleanup_push( (void *)clean_automator, myThreadState);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();
   
   do
   {
      pthread_testcancel();
      process_heartbeat(_automator1_monitoring_id); // heartbeat après chaque boucle


      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(automator_lock) );
      pthread_mutex_lock(&(automator_lock));

      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *tempState = PyThreadState_Swap(myThreadState);
         
      aDict=PyDict_New();

      // ajouter ici les données pour l'automate ...
      
      // PyObject *d=mea_xplMsgToPyDict(xplMsg);
      // PyDict_SetItemString(aDict, "xplmsg", d);
      // Py_DECREF(d);
      
      callPythonFunction(python_automator_path, "automator", "automator", aDict);
      
      Py_DECREF(aDict);

      PyThreadState_Swap(tempState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

      pthread_mutex_unlock(&(automator_lock));
      pthread_cleanup_pop(0);
      
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            mea_log_printf("%s  (%s) : %s thread is running\n", INFO_STR, __func__, acquire_name_str);
         }
         else
            compteur++;
      }
      
//      nanosleep((struct timespec[]){{0, 50000000L}}, NULL); // dodo 50ms
      sleep(1);
   }
   while (1);

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
}


pthread_t *automator()
{
   _automator1_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_automator1_thread_id)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   if(pthread_create (_automator1_thread_id, NULL, automator_thread, NULL))
   {
      VERBOSE(1) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }
      
   return _automator1_thread_id;
}


int stop_automator(int my_id, void *data,  char *errmsg, int l_errmsg)
{
   int ret=0;
   
   if(_automator1_thread_id)
   {
      pthread_cancel(*_automator1_thread_id);
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(_automator1_thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 1 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }

      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop\n",DEBUG_STR, __func__,automator1_name_str,100-counter);
      ret=stopped;
   }

   _automator1_monitoring_id=-1;

   if(_automator1_thread_id)
   {
      free(_automator1_thread_id);
      _automator1_thread_id=NULL;
   }   

   if(ret==0)
   {
      VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, automator1_name_str, stopped_successfully_str);
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s  (%s) : %s can't cancel thread.\n", INFO_STR, __func__, automator1_name_str);
   }
   return ret;
}


int start_automator(int my_id, void *data, char *errmsg, int l_errmsg)
{
//   struct automator_start_stop_params_s *automator_start_stop_params = (struct automator_start_stop_params_s *)data;
   char err_str[256]="";

   _automator1_monitoring_id=my_id;

   _automator1_thread_id=automator();

   if(_automator1_thread_id==NULL)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't start automator process - %s\n",ERROR_STR,__func__,err_str);
      }

      return -1;
   }
   else
   {
      pthread_detach(*_automator1_thread_id);
      VERBOSE(2) mea_log_printf("%s  (%s) : %s launched successfully.\n", INFO_STR, __func__, automator1_name_str);

      return 0;
   }
}


int restart_automator(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_automator(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_automator(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}


int main(int argc, char *argv[])
{
   static struct option long_options[] = {
      {"scriptspath",       required_argument, 0,  'y'                  }, // 'y'
      {"xpladdr",           required_argument, 0,  'x'                  }, // 'd'
      {"help",              no_argument,       0,  'h'                  }, // 'h'
      {0,                   0,                 0,  0                    }
   };

   int option_index = 0; // getopt_long function need int
   int c=0; // getopt_long function need int
   while ((c = getopt_long(argc, (char * const *)argv, "hx:y:", long_options, &option_index)) != -1)
   {
      switch (c)
      {
         case 'h':
            // usage((char *)argv[0]);
            exit(0);
            break;
         case 'x':
            fprintf(stderr, "%c optarg = %s\n", c, optarg);
            set_xpl_address(optarg);
            break;
         case 'y':
            fprintf(stderr, "%c optarg = %s\n", c, optarg);
            strncpy(python_automator_path, optarg, sizeof(python_automator_path)-1);
            break;
         default:
            fprintf(stderr, "option inconnue : \"%c\"\n", c);
            exit(1);
      }
   }

   if(xpl_vendorID[0]==0 || strlen(python_automator_path)==0)
   {
      // erreur, pas d'adresse xpl et pas de path pour les scripts de l'automate
      exit(1);
   }
   
   init_processes_manager(10);

   pthread_mutex_init(&automator_lock, NULL);
   
   Py_Initialize();
   PyEval_InitThreads(); // voir ici http://www.codeproject.com/Articles/11805/Embedding-Python-in-C-C-Part-I

   // chemin vers les plugins rajoutés dans le path de l'interpréteur Python
   PyObject* sysPath = PySys_GetObject((char*)"path");
   PyObject* pluginsDir = PyString_FromString(python_automator_path);
   PyList_Append(sysPath, pluginsDir);
   Py_DECREF(pluginsDir);

   automator_known_modules=PyDict_New(); // initialisation du cache de module

   automator_modules_init();

   PyEval_ReleaseLock();
   
   struct acquire_start_stop_param_s acquire_start_stop_param;
   _acquire_monitoring_id=process_register(acquire_name_str);
   process_set_start_stop(_acquire_monitoring_id, start_acquire, stop_acquire, (void *)(&acquire_start_stop_param), 1);
//   process_wd_disable(_acquire_monitoring_id, 1);
   process_set_watchdog_recovery(_acquire_monitoring_id, restart_acquire, (void *)(&acquire_start_stop_param));
   if(process_start(_acquire_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start acquire process\n",ERROR_STR,__func__);
      exit(1);
   }

   struct automator_start_stop_param_s automator_start_stop_param;
   _automator1_monitoring_id=process_register(automator1_name_str);
   process_set_start_stop(_automator1_monitoring_id, start_automator, stop_automator, (void *)(&automator_start_stop_param), 1);
   process_set_watchdog_recovery(_automator1_monitoring_id, restart_automator, (void *)(&automator_start_stop_param));
   if(process_start(_automator1_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start automator process\n",ERROR_STR,__func__);
      exit(1);
   }
   
   
   while(1)
   {
      managed_processes_loop(); // watchdog et indicateurs
      sleep(1);
   }
}

