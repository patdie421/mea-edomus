//
//  interface_type_003.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#include <Python.h>

#include "interface_type_003.h"

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
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "macros.h"
#include "mea_string_utils.h"
#include "cJSON.h"
#include "enocean.h"
#include "serial.h"
#include "dbServer.h"
#include "parameters_utils.h"
#include "pythonPluginServer.h"
#include "python_utils.h"

#include "processManager.h"
#include "notify.h"

#include "interfacesServer.h"


char *interface_type_003_senttoplugin_str="SENT2PLUGIN";
char *interface_type_003_xplin_str="XPLIN";
char *interface_type_003_enoceandatain_str="ENOCEANIN";


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 3.
char *valid_enocean_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define ENOCEAN_PLUGIN_PARAMS_PLUGIN      0
#define ENOCEAN_PLUGIN_PARAMS_PARAMETERS  1

struct enocean_callback_data_s // donnee "userdata" pour les callbacks
{
//   PyThreadState   *mainThreadState;
//   sqlite3         *param_db;
   enocean_ed_t    *ed;
   mea_queue_t     *queue;
   pthread_mutex_t *callback_lock;
   pthread_cond_t  *callback_cond;
};


struct xpl_callback_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
};


struct enocean_thread_params_s
{
   enocean_ed_t         *ed;
   sqlite3              *param_db;
   mea_queue_t          *queue;
   pthread_mutex_t       callback_lock;
   pthread_cond_t        callback_cond;
//   PyThreadState        *mainThreadState;
//   PyThreadState        *myThreadState;
   parsed_parameters_t  *plugin_params;
   int                   nb_plugin_params;
   sqlite3_stmt         *stmt;
   interface_type_003_t *i003;
};


int start_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);


void set_interface_type_003_isnt_running(void *data)
{
   interface_type_003_t *i003 = (interface_type_003_t *)data;
   i003->thread_is_running=0;
}


void _enocean_data_free_queue_elem(void *d)
{
   enocean_data_queue_elem_t *e=(enocean_data_queue_elem_t *)d;
   
   if(e->data)
   {
      free(e->data);
      e->data=NULL;
   }

   free(e);
   e=NULL;
}


int16_t _interface_type_003_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
//   char *device = NULL;
//   int ret = -1;
   int err =0;

   interface_type_003_t *i003=(interface_type_003_t *)userValue;
   struct xpl_callback_data_s *callback_data=(struct xpl_callback_data_s *)i003->xPL_callback_data;

   char *dev = (char *)device_info->interface_dev;
   int a,b,c,d;
   if(sscanf(dev, "%*[^:]://%x-%x-%x-%x", &a,&b,&c,&d)!=4)
   {
      VERBOSE(9) mea_log_printf("%s (%s) : scanf - %s\n", ERROR_STR, __func__, dev);
      return -1;
   }

   i003->indicators.xplin++;

   uint32_t enocean_addr = a;
   enocean_addr = (enocean_addr << 8) + b;
   enocean_addr = (enocean_addr << 8) + c;
   enocean_addr = (enocean_addr << 8) + d;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;
        
   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_enocean_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return -1;
   }

   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));

   if(plugin_elem)
   {
      plugin_elem->type_elem=XPLMSG;

      PyEval_AcquireLock(); {

         if(!callback_data->mainThreadState)
            callback_data->mainThreadState=PyThreadState_Get();
         if(!callback_data->myThreadState)
            callback_data->myThreadState = PyThreadState_New(callback_data->mainThreadState->interp);

         PyThreadState *tempState = PyThreadState_Swap(callback_data->myThreadState);
                   
         plugin_elem->aDict=mea_device_info_to_pydict_device(device_info);

         mea_addLong_to_pydict(plugin_elem->aDict, XPL_ENOCEAN_ADDR_STR_C, (long)enocean_addr);
         mea_addLong_to_pydict(plugin_elem->aDict, "api_key", (long)i003->id_interface);

         PyObject *dd=mea_xplMsgToPyDict2(xplMsgJson);
         PyDict_SetItemString(plugin_elem->aDict, XPLMSG_STR_C, dd);
         Py_DECREF(dd);
               
         if(plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PARAMETERS].value.s);
         PyThreadState_Swap(tempState);

      } PyEval_ReleaseLock();
            
      pythonPluginServer_add_cmd(plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
      i003->indicators.senttoplugin++;
      free(plugin_elem);
      plugin_elem=NULL;
   }
         
   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;

   return 0;
}


int16_t _inteface_type_003_enocean_data_callback(uint8_t *data, uint16_t l_data, uint32_t enocean_addr, void *callbackdata)
{
   struct timeval tv;
   struct enocean_callback_data_s *callback_data;
   enocean_data_queue_elem_t *e;
   
   callback_data=(struct enocean_callback_data_s *)callbackdata;
   
   gettimeofday(&tv, NULL);
   
   e=(enocean_data_queue_elem_t *)malloc(sizeof(enocean_data_queue_elem_t));
   
   e->enocean_addr=enocean_addr;
   
   e->data=(uint8_t *)malloc(l_data+1);
   memcpy(e->data,data,l_data);
   e->l_data=l_data;
   memcpy(&e->tv,&tv,sizeof(struct timeval));
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&callback_data->callback_lock) );
   pthread_mutex_lock(callback_data->callback_lock);
   mea_queue_in_elem(callback_data->queue, e);
   if(callback_data->queue->nb_elem>=1)
      pthread_cond_broadcast(callback_data->callback_cond);
   pthread_mutex_unlock(callback_data->callback_lock);
   pthread_cleanup_pop(0);
   
   return 0;
}


void *_thread_interface_type_003_enocean_data_cleanup(void *args)
{
   struct enocean_thread_params_s *params=(struct enocean_thread_params_s *)args;

   if(!params)
      return NULL;
   if(params->i003->myThreadState)
   {
      PyEval_AcquireLock();
      PyThreadState_Clear(params->i003->myThreadState);
      PyThreadState_Delete(params->i003->myThreadState);
      PyEval_ReleaseLock();
      params->i003->myThreadState=NULL;
   }
   
   if(params->plugin_params)
      release_parsed_parameters(&(params->plugin_params));
   
   if(params->stmt)
   {
      sqlite3_finalize(params->stmt);
      params->stmt=NULL;
   }
   
   if(params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      mea_queue_cleanup(params->queue, _enocean_data_free_queue_elem);
   
   if(params->queue)
   {
      free(params->queue);
      params->queue=NULL;
   }

   free(params);
   params=NULL;

   return NULL;
}


void *_thread_interface_type_003_enocean_data(void *args)
/**
 * \brief     Gestion des données asynchrones en provenances de equipement enocean
 * \details   Les data peuvent arriver n'importe quand, il sagit de pouvoir les traiter dès réceptions.
 *            Le callback associé est en charge de poster les données à traiter dans une file qui seront consommées par ce thread.
 * \param     args   ensemble des parametres nécessaires au thread regroupé dans une structure de données (voir struct enocean_thread_params_s)
 */
{
   struct enocean_thread_params_s *params=(struct enocean_thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_003_enocean_data_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_003_isnt_running, (void *)params->i003 );
   
   params->i003->thread_is_running=1;
   process_heartbeat(params->i003->monitoring_id);
   
   sqlite3 *params_db=params->param_db;
   enocean_ed_t *ed=params->ed;
   enocean_data_queue_elem_t *e;
   int ret;
   
   params->plugin_params=NULL;
   params->nb_plugin_params=0;
   params->stmt=NULL;
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->i003->mainThreadState = PyThreadState_Get();
   params->i003->myThreadState = PyThreadState_New(params->i003->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();
   
   while(1)
   {
      if(ed->signal_flag==1)
         goto _thread_interface_type_003_enocean_data_clean_exit;

      process_heartbeat(params->i003->monitoring_id);
      process_update_indicator(params->i003->monitoring_id, interface_type_003_senttoplugin_str, params->i003->indicators.senttoplugin);
      process_update_indicator(params->i003->monitoring_id, interface_type_003_xplin_str, params->i003->indicators.xplin);
      process_update_indicator(params->i003->monitoring_id, interface_type_003_enoceandatain_str, params->i003->indicators.enoceandatain);

      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&params->callback_lock) );
      pthread_mutex_lock(&params->callback_lock);
      
      if(params->queue->nb_elem==0)
      {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 10; // timeout de 10 secondes
         ts.tv_nsec = 0;
         
         ret=pthread_cond_timedwait(&params->callback_cond, &params->callback_lock, &ts);
         if(ret)
         {
            if(ret==ETIMEDOUT)
            {
//               DEBUG_SECTION mea_log_printf("%s (%s) : Nb elements in queue after TIMEOUT : %ld)\n", DEBUG_STR, __func__, params->queue->nb_elem);
            }
            else
            {
               // autres erreurs à traiter
            }
         }
      }
      
      ret=mea_queue_out_elem(params->queue, (void **)&e);
      
      pthread_mutex_unlock(&params->callback_lock);
      pthread_cleanup_pop(0);
      
      if(!ret)
      {
         char sql[2048];
         uint32_t addr;
         
         params->i003->indicators.enoceandatain++;
         addr = e->enocean_addr;
         uint8_t a,b,c,d;
         d=addr & 0xFF;
         addr = addr >> 8;
         c=addr & 0xFF;
         addr = addr >> 8;
         b=addr & 0xFF;
         addr = addr >> 8;
         a=addr & 0xFF;
         
         mea_log_printf("%s (%s) : enocean data from - %02x-%02x-%02x-%02x\n", INFO_STR, __func__, a, b, c, d);
         
//         sprintf(sql,"%s WHERE interfaces.dev ='ENOCEAN://%02x-%02x-%02x-%02x' AND sensors_actuators.deleted_flag <> 1 AND sensors_actuators.state='1';", sql_select_device_info,a,b,c,d);
         sprintf(sql,"%s WHERE lower(interfaces.dev) = lower('%s://%02x-%02x-%02x-%02x') AND sensors_actuators.deleted_flag <> 1 AND sensors_actuators.state='1';", sql_select_device_info, params->i003->name, a, b, c, d);
         
         ret = sqlite3_prepare_v2(params_db,sql,(int)(strlen(sql)+1),&(params->stmt),NULL);
         if(ret)
         {
            VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
            pthread_exit(NULL);
         }
         
         while(1)
         {
            int s = sqlite3_step(params->stmt);
            if (s == SQLITE_ROW)
            {
               int err;

               params->plugin_params=alloc_parsed_parameters((char *)sqlite3_column_text(params->stmt, 3), valid_enocean_plugin_params, &(params->nb_plugin_params), &err, 0);
               if(!params->plugin_params || !params->plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s)
               {
                  if(params->plugin_params)
                     release_parsed_parameters(&(params->plugin_params));

                  continue; // si pas de paramètre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin
               }
               plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
               if(plugin_elem)
               {
                  pthread_cleanup_push( (void *)free, (void *)plugin_elem );

                  plugin_elem->type_elem=DATAFROMSENSOR;
                 
                  memcpy(plugin_elem->buff, e->data, e->l_data);
                  plugin_elem->l_buff=e->l_data;
                  
                  { // appel des fonctions Python
                     PyEval_AcquireLock();
                     pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // trop compliquer de traiter avec pthread_cleanup => on interdit les arrêts lors des commandes python
                     PyThreadState *tempState = PyThreadState_Swap(params->i003->myThreadState);
                     
                     plugin_elem->aDict=mea_stmt_to_pydict_device(params->stmt);
                     mea_addLong_to_pydict(plugin_elem->aDict, XPL_ENOCEAN_ADDR_STR_C, (long)e->enocean_addr);
                     
                     PyObject *value;

                     value = PyByteArray_FromStringAndSize(plugin_elem->buff, (long)plugin_elem->l_buff);
                     PyDict_SetItemString(plugin_elem->aDict, "data", value);
                     Py_DECREF(value);
                     mea_addLong_to_pydict(plugin_elem->aDict, "l_data", (long)plugin_elem->l_buff);
                     mea_addLong_to_pydict(plugin_elem->aDict, "api_key", (long)params->i003->id_interface);

                     if(params->plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PARAMETERS].value.s)
                        mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, params->plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PARAMETERS].value.s);
                     
                     PyThreadState_Swap(tempState);
                     PyEval_ReleaseLock();
                     pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // on réauthorise les arrêts
                     pthread_testcancel(); // on test tout de suite pour être sûr qu'on a pas ratté une demande d'arrêt
                  } // fin appel des fonctions Python
                  
                  pythonPluginServer_add_cmd(params->plugin_params->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
                  params->i003->indicators.senttoplugin++;
                  free(plugin_elem);
                  plugin_elem=NULL;
                  
                  pthread_cleanup_pop(0);
               }
               else
               {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                     perror("");
                  }
                  pthread_exit(PTHREAD_CANCELED);
               }
               release_parsed_parameters(&(params->plugin_params));
            }
            else if (s == SQLITE_DONE)
            {
               sqlite3_finalize(params->stmt);
               params->stmt=NULL;
               break;
            }
            else
            {
               sqlite3_finalize(params->stmt);
               params->stmt=NULL;
               break; // traitement des erreurs à affiner avant break ...
            }
         }
         
         free(e->data);
         e->data=NULL;
         free(e);
         e=NULL;
         
         pthread_testcancel();
      }
      else
      {
         // pb d'accès aux données de la file
         DEBUG_SECTION mea_log_printf("%s (%s) : mea_queue_out_elem - no data in queue\n", DEBUG_STR, __func__);
      }
      pthread_testcancel();
   }

_thread_interface_type_003_enocean_data_clean_exit:   
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
  
   process_async_stop(params->i003->monitoring_id);
   for(;;) sleep(1);
 
   return NULL;
}


pthread_t *start_interface_type_003_enocean_data_thread(interface_type_003_t *i003, enocean_ed_t *ed, sqlite3 *db, thread_f function)
/**  
 * \brief     Demarrage du thread de gestion des données (non solicitées) en provenance des enocean
 * \param     i003           descripteur de l'interface
 * \param     ed             descripteur de com. avec l'enocean
 * \param     db             descripteur ouvert de la base de paramétrage
 * \param     function       function principale du thread à démarrer
 * \return    adresse du thread ou NULL en cas d'erreur
 **/ 
{
   pthread_t *thread=NULL;
   struct enocean_thread_params_s *params=NULL;
   struct enocean_callback_data_s *enocean_callback_data=NULL;
   
   params=malloc(sizeof(struct enocean_thread_params_s));
   if(!params)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   params->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!params->queue)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   mea_queue_init(params->queue);

   params->ed=ed;
   params->param_db=db;
   pthread_mutex_init(&params->callback_lock, NULL);
   pthread_cond_init(&params->callback_cond, NULL);
   params->i003=(void *)i003;
   params->i003->mainThreadState = NULL;
   params->i003->myThreadState = NULL;

   // préparation des données pour les callback io_data et data_flow dont les données sont traitées par le même thread
   enocean_callback_data=(struct enocean_callback_data_s *)malloc(sizeof(struct enocean_callback_data_s));
   if(!enocean_callback_data)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   enocean_callback_data->ed=ed;
//   enocean_callback_data->param_db=db;
   enocean_callback_data->callback_lock=&params->callback_lock;
   enocean_callback_data->callback_cond=&params->callback_cond;
   enocean_callback_data->queue=params->queue;

   enocean_set_data_callback2(ed, _inteface_type_003_enocean_data_callback, (void *)enocean_callback_data);

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto clean_exit;
   pthread_detach(*thread);

   return thread;
   
clean_exit:
   if(thread)
   {
      free(thread);
      thread=NULL;
   }
   
   if(enocean_callback_data)
   {
      free(enocean_callback_data);
      enocean_callback_data=NULL;
   }

   if(params && params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      mea_queue_cleanup(params->queue, _enocean_data_free_queue_elem);

   if(params)
   {
      if(params->queue)
      {
         free(params->queue);
         params->queue=NULL;
      }
      free(params);
      params=NULL;
   }
   return NULL;
}


int clean_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003->parameters)
   {
      free(i003->parameters);
      i003->parameters=NULL;
   }
/*
   if(i003->xPL_callback_data)
   {
      struct callback_xpl_data_s *data = (struct callback_xpl_data_s *)i003->xPL_callback_data;
     
      if(data->myThreadState)
      {
         PyEval_AcquireLock();
         PyThreadState_Clear(data->myThreadState);
         PyThreadState_Delete(data->myThreadState);
         PyEval_ReleaseLock();
         data->myThreadState=NULL;
      }
      free(i003->xPL_callback_data);
      i003->xPL_callback_data=NULL;
   }
*/
   
   if(i003->xPL_callback2)
      i003->xPL_callback2=NULL;

   if(i003->ed && i003->ed->enocean_callback_data)
   {
         free(i003->ed->enocean_callback_data);
         i003->ed->enocean_callback_data=NULL;
   }

   if(i003->ed)
   {
      free(i003->ed);
      i003->ed=NULL;
   }

   if(i003->thread)
   {
      free(i003->thread);
      i003->thread=NULL;
   }
      
   return 0;
}


xpl2_f get_xPLCallback_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return NULL;
   else
      return i003->xPL_callback2;
}


int get_monitoring_id_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else
      return i003->monitoring_id;
}


int set_xPLCallback_interface_type_003(void *ixxx, xpl2_f cb)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else
   {
      i003->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_003(void *ixxx, int id)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else
   {
      i003->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_003()
{
   return INTERFACE_TYPE_003;
}


int get_interface_id_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else
      return i003->id_interface;
}


int api_sendEnoceanRadioErp1Packet(interface_type_003_t *i003, PyObject *args, PyObject **res, int16_t *nerr, char *err, int l_err)
{
   PyObject *arg;
//   enocean_ed_t *ed;
   int16_t ret;

   *nerr=255;
   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=6)
      return -255;

   // rorg
   uint32_t rorg;
   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      rorg=(uint32_t)PyLong_AsLong(arg);
   else
      return -255;

   // sub_id
   uint32_t sub_id;
   arg=PyTuple_GetItem(args, 3);
   if(PyNumber_Check(arg))
      sub_id=(uint32_t)PyLong_AsLong(arg);
   else
      return -255;

   // dest addr
   uint32_t dest_addr;
   arg=PyTuple_GetItem(args, 4);
   if(PyNumber_Check(arg))
      dest_addr=(uint32_t)PyLong_AsLong(arg);
   else
      return -255;

   Py_buffer py_packet;
   arg=PyTuple_GetItem(args, 5);
   if(PyObject_CheckBuffer(arg))
   {
      ret=PyObject_GetBuffer(arg, &py_packet, PyBUF_SIMPLE);
      if(ret<0)
      return -255;
   }
   else
      return -255;

   *nerr = 0;
   ret = enocean_send_radio_erp1_packet(i003->ed, rorg, i003->ed->id, sub_id, dest_addr, py_packet.buf, py_packet.len, 0, nerr);
   if(ret<0)
      strncpy(err, "error", l_err);
   else
      strncpy(err, "no error", l_err);

   PyBuffer_Release(&py_packet);

   *res = NULL;

   return ret;
}


int16_t api_interface_type_003(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   PyObject *pyArgs = (PyObject *)args;
   PyObject **pyRes = (PyObject **)res;

   if(strcmp(cmnd, "sendEnoceanRadioErp1Packet") == 0)
   {
//      int ret=api_sendEnoceanRadioErp1Packet(i003, pyArgs, pyRes, nerr, err, l_err);
      api_sendEnoceanRadioErp1Packet(i003, pyArgs, pyRes, nerr, err, l_err);
   }
#ifdef DEBUG
   else if(strcmp(cmnd, "test") == 0)
   {
      *res = PyString_FromString("New style Api call OK !!!");
      *nerr=0;
      strncpy(err, "no error", l_err);

      return 0;
   }
#endif
   else
   {
      strncpy(err, "unknown function", l_err);
      return -254;
   }

   return -1;
}


interface_type_003_t *malloc_and_init_interface_type_003(sqlite3 *sqlite3_param_db, int id_driver, int id_interface, char *name, char *dev, char *parameters, char *description)
{
   interface_type_003_t *i003;
                  
   i003=(interface_type_003_t *)malloc(sizeof(interface_type_003_t));
   if(!i003)
   {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   i003->thread_is_running=0;
                  
   struct interface_type_003_data_s *i003_start_stop_params=(struct interface_type_003_data_s *)malloc(sizeof(struct interface_type_003_data_s));
   if(!i003_start_stop_params)
   {
      free(i003);
      i003=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }  
      return NULL;
   }
   strncpy(i003->dev, (char *)dev, sizeof(i003->dev)-1);
   strncpy(i003->name, (char *)name, sizeof(i003->name)-1);
   i003->id_interface=id_interface;
   i003->id_driver=id_driver;
   i003->parameters=(char *)malloc(strlen((char *)parameters)+1);
   strcpy(i003->parameters,(char *)parameters);
   i003->indicators.senttoplugin=0;
   i003->indicators.xplin=0;
   i003->indicators.enoceandatain=0;
   
   i003->ed=NULL;
   i003->thread=NULL;
   i003->xPL_callback2=NULL;
   i003->xPL_callback_data=NULL;

   i003->mainThreadState=NULL;
   i003->myThreadState=NULL;

   i003->monitoring_id=process_register((char *)name);
   i003_start_stop_params->sqlite3_param_db = sqlite3_param_db;
   i003_start_stop_params->i003=i003;
                  
   process_set_group(i003->monitoring_id, 1);
   process_set_start_stop(i003->monitoring_id, start_interface_type_003, stop_interface_type_003, (void *)i003_start_stop_params, 1);
   process_set_watchdog_recovery(i003->monitoring_id, restart_interface_type_003, (void *)i003_start_stop_params);
   process_set_description(i003->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i003->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i003->monitoring_id, interface_type_003_senttoplugin_str, 0);
   process_add_indicator(i003->monitoring_id, interface_type_003_xplin_str, 0);
   process_add_indicator(i003->monitoring_id, interface_type_003_enoceandatain_str, 0);

   return i003;
}


int stop_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     arrêt d'une interface de type 3
 * \details   RAZ de tous les structures de données et libération des zones de mémoires allouées
 * \param     i003  descripteur de l'interface
 * \return    0 pas d'erreur ou -1 sinon
 **/ 
{
   if(!data)
      return -1;

   struct interface_type_003_data_s *start_stop_params=(struct interface_type_003_data_s *)data;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i003->name);

   if(start_stop_params->i003->xPL_callback_data)
   {
      struct xpl_callback_data_s *data = (struct xpl_callback_data_s *)start_stop_params->i003->xPL_callback_data;
     
      if(data->myThreadState)
      {
         PyEval_AcquireLock();
         PyThreadState_Clear(data->myThreadState);
         PyThreadState_Delete(data->myThreadState);
         PyEval_ReleaseLock();
         data->myThreadState=NULL;
      }

      free(start_stop_params->i003->xPL_callback_data);
      start_stop_params->i003->xPL_callback_data=NULL;
   }

   if(start_stop_params->i003->xPL_callback2)
      start_stop_params->i003->xPL_callback2=NULL;

   if(start_stop_params->i003->ed->enocean_callback_data)
   {
      free(start_stop_params->i003->ed->enocean_callback_data);
      start_stop_params->i003->ed->enocean_callback_data=NULL;
   }

   if(start_stop_params->i003->thread)
   {
      pthread_cancel(*(start_stop_params->i003->thread));
      
      int counter=100;
      while(counter--)
      {
         if(start_stop_params->i003->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
            break;
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i003->name,100-counter);

      free(start_stop_params->i003->thread);
      start_stop_params->i003->thread=NULL;
   }
   
   enocean_close(start_stop_params->i003->ed);
   enocean_clean_ed(start_stop_params->i003->ed);

   if(start_stop_params->i003->ed)
   {
      free(start_stop_params->i003->ed);
      start_stop_params->i003->ed=NULL;
   }
  
   mea_notify_printf('S', "%s %s", start_stop_params->i003->name, stopped_successfully_str);

   return 0;
}


int restart_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     Demarrage d'une interface de type 3
 * \details   ouverture de la communication avec un dongle USB300 ou équivalant, démarrage du thread de gestion des données
 *            "non sollicitées" et mise en place des callback xpl et données
 * \param     my_id          id attribué par processManager
 * \param     data           données nécessaire au arret/relance (à caster sur struct interface_type_003_data_s)
 * \param     errmsg
 * \param     l_errmsg
 * \return    0 = Ok ou -1 = KO
 **/ 
{
   char dev[81];
   char buff[80];
   speed_t speed;

   int fd=-1;
   int err;
   int ret;
   
   enocean_ed_t *ed=NULL;
   
   struct xpl_callback_data_s *xpl_callback_data=NULL;
   
   struct interface_type_003_data_s *start_stop_params=(struct interface_type_003_data_s *)data;

   start_stop_params->i003->thread=NULL;

   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;
   
   char err_str[128];
   
   ret=get_dev_and_speed((char *)start_stop_params->i003->dev, buff, sizeof(buff), &speed);
   if(!ret)
   {
      int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(buff)-1))
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i003->name, err_str);
         goto clean_exit;
      }
   }
   else
   {
      VERBOSE(2) mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR,__func__,start_stop_params->i003->dev);
      mea_notify_printf('E', "%s can't be launched - incorrect device/speed interface - %s.\n", start_stop_params->i003->name, start_stop_params->i003->dev);
      goto clean_exit;
   }

   ed=(enocean_ed_t *)malloc(sizeof(enocean_ed_t));
   if(!ed)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i003->name, err_str);
      goto clean_exit;
   }
   
   fd=enocean_init(ed, dev);
   if (fd == -1)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : enocean_init - Unable to open serial port (%s).\n", ERROR_STR, __func__, dev);
      }
      mea_notify_printf('E', "%s : unable to open serial port (%s) - %s.\n", start_stop_params->i003->name, dev, err_str);
      goto clean_exit;
   }
   start_stop_params->i003->ed=ed;
   
   /*
    * exécution du plugin de paramétrage
    */
   interface_parameters=alloc_parsed_parameters(start_stop_params->i003->parameters, valid_enocean_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(interface_parameters)
      {
         // pas de plugin spécifié
         release_parsed_parameters(&interface_parameters);
         interface_parameters=NULL;
         VERBOSE(9) mea_log_printf("%s (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else
      {
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->i003->parameters);
      }
   }
   else
   {
      //
      // a remplacer par appel a python_call ... (voir python_utils.c)
      //
      PyObject *plugin_params_dict=NULL;
      PyObject *pName, *pModule, *pFunc;
      PyObject *pArgs, *pValue=NULL;
      
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *mainThreadState=PyThreadState_Get();
      PyThreadState *myThreadState = PyThreadState_New(mainThreadState->interp);
      
      PyThreadState *tempState = PyThreadState_Swap(myThreadState);

      // à remplacer par mea_callfunction() ...
      PyErr_Clear();
      pName = PyString_FromString(interface_parameters->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s);
      pModule = PyImport_Import(pName);
      if(!pModule)
      {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not found\n", ERROR_STR, __func__, interface_parameters->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s);
      }
      else
      {
         pFunc = PyObject_GetAttrString(pModule, "mea_init");
         if (pFunc && PyCallable_Check(pFunc))
         {
            // préparation du parametre du module
            plugin_params_dict=PyDict_New();

            mea_addLong_to_pydict(plugin_params_dict, INTERFACE_ID_STR_C, start_stop_params->i003->id_interface);
            if(interface_parameters->parameters[ENOCEAN_PLUGIN_PARAMS_PARAMETERS].value.s)
               mea_addString_to_pydict(plugin_params_dict, INTERFACE_PARAMETERS_STR_C, interface_parameters->parameters[ENOCEAN_PLUGIN_PARAMS_PARAMETERS].value.s);

            pArgs = PyTuple_New(1);
            Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
            PyTuple_SetItem(pArgs, 0, plugin_params_dict);
         
            pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
            if (pValue != NULL)
            {
               DEBUG_SECTION mea_log_printf("%s (%s) : Result of call of mea_init : %ld\n", DEBUG_STR, __func__, PyInt_AsLong(pValue));
               Py_DECREF(pValue);
            }
            PyErr_Clear(); 
            Py_DECREF(pArgs);
            Py_DECREF(plugin_params_dict);
         }
         else
         {
            VERBOSE(5) mea_log_printf("%s (%s) : mea_init not fount in %s module\n", ERROR_STR, __func__, interface_parameters->parameters[ENOCEAN_PLUGIN_PARAMS_PLUGIN].value.s);
         }
         Py_XDECREF(pFunc);
      }
      Py_XDECREF(pModule);
      Py_DECREF(pName);
      PyErr_Clear();
      
      PyThreadState_Swap(tempState);
      PyThreadState_Clear(myThreadState);
      PyThreadState_Delete(myThreadState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

      if(interface_parameters)
         release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;
   }

   /*
    * Gestion des sous-interfaces
    */
   start_stop_params->i003->thread=start_interface_type_003_enocean_data_thread(start_stop_params->i003, ed, start_stop_params->sqlite3_param_db, (thread_f)_thread_interface_type_003_enocean_data);

   //
   // gestion des demandes xpl : ajouter une zone de donnees specifique au callback xpl (pas simplement passe i003).
   //
   xpl_callback_data=(struct xpl_callback_data_s *)malloc(sizeof(struct xpl_callback_data_s));
   if(!xpl_callback_data)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i003->name, err_str);
      goto clean_exit;
   }
   xpl_callback_data->mainThreadState=NULL;
   xpl_callback_data->myThreadState=NULL;
   
   start_stop_params->i003->xPL_callback_data=xpl_callback_data;
   start_stop_params->i003->xPL_callback2=_interface_type_003_xPL_callback2;
   
   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i003->name, launched_successfully_str);
   mea_notify_printf('S', "%s %s", start_stop_params->i003->name, launched_successfully_str);
   
   return 0;
   
clean_exit:
   if(ed)
      enocean_remove_data_callback(ed);
   
   if(start_stop_params->i003->thread)
      stop_interface_type_003(start_stop_params->i003->monitoring_id, start_stop_params, NULL, 0);

   
   if(interface_parameters)
   {
      release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;
   }
   
   if(xpl_callback_data)
   {
      free(xpl_callback_data);
      xpl_callback_data=NULL;
   }
   
   if(ed)
   {
      if(fd>=0)
         enocean_close(ed);
      enocean_free_ed(ed);
      ed=NULL;
   }
   
   return -1;
}


#ifndef ASPLUGIN
int get_fns_interface_type_003(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init = (malloc_and_init_f)&malloc_and_init_interface_type_003;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_003;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_003;
   interfacesFns->clean = (clean_f)&clean_interface_type_003;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_003;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_003;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_003;
   interfacesFns->api = (api_f)&api_interface_type_003;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
