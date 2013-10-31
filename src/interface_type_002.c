/*
 *
 *  Created by Patrice Dietsch on 24/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <Python.h>

#include "interface_type_002.h"

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
#include "token_strings.h"

#include "error.h"
#include "debug.h"
#include "macros.h"
#include "memory.h"

#include "xbee.h"
#include "dbServer.h"
#include "parameters_mgr.h"
#include "mea_api.h"
#include "pythonPluginServer.h"
#include "python_api_utils.h"


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_xbee_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define XBEE_PLUGIN_PARAMS_PLUGIN      0
#define XBEE_PLUGIN_PARAMS_PARAMETERS  1

char *sql_select_device_info="SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, types.parameters, sensors_actuators.id_type, sensors_actuators.name, interfaces.name, interfaces.id_type, (SELECT types.name FROM types WHERE types.id_type = interfaces.id_type), interfaces.dev FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";
char *sql_select_interface_info="SELECT * FROM interfaces";

char *printf_mask_addr="%02x%02x%02x%02x-%02x%02x%02x%02x";


struct callback_data_s // donnee "userdata" pour les callbacks
{
   PyThreadState  *mainThreadState;
   sqlite3        *param_db;
   xbee_xd_t      *xd;
   queue_t        *queue;
   pthread_mutex_t *callback_lock;
   pthread_cond_t  *callback_cond;
};


struct callback_commissionning_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
   sqlite3        *param_db;
   xbee_xd_t      *xd;
   xbee_host_t    *local_xbee;
};


struct callback_xpl_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
   sqlite3        *param_db;
};


struct thread_params_s
{
   xbee_xd_t            *xd;
   tomysqldb_md_t       *md;
   sqlite3              *param_db;
   queue_t              *queue;
   pthread_mutex_t      callback_lock;
   pthread_cond_t       callback_cond;
   PyThreadState       *mainThreadState;
   PyThreadState       *myThreadState;
   parsed_parameters_t  *plugin_params;
   int                  nb_plugin_params;
   sqlite3_stmt        *stmt;
   data_queue_elem_t   *e;
   void *               *data;
};


mea_error_t display_frame(int ret, unsigned char *resp, uint16_t l_resp)
{
   DEBUG_SECTION {
      if(!ret)
      {
         for(int i=0;i<l_resp;i++)
            fprintf(stderr,"%s (%s) : %02x-[%c](%03d)\n",DEBUG_STR,__func__,resp[i],resp[i],resp[i]);
         printf("\n");
      }
   }
   return NOERROR;
}


void display_addr(char *a)
{
   DEBUG_SECTION {
      for(int i=0;i<4;i++)
         fprintf(stderr,"%s (%s) : %02x",DEBUG_STR, __func__,a[i]);
   }
}


void _iodata_free_queue_elem(void *d)
{
   data_queue_elem_t *e=(data_queue_elem_t *)d;
   
   free(e);
   e=NULL;
}


int add_16bits_to_at_uchar_cmd_from_int(unsigned char *at_cmd, uint16_t val)
{
   uint16_t i=2;
   int8_t h,l;
   
   h=(val/256) & 0xFF;
   l=val%256;
   
   if(h>0)
   {
      at_cmd[i]=h;
      i++;
   }
   at_cmd[i]=l;
   i++;
   at_cmd[i]=-1;
   
   return i;
}


int at_set_16bits_reg_from_int(xbee_xd_t *xd, xbee_host_t *host, char at_cmd[2], int reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   uint16_t l_at;
   int16_t ret;
   
   if(!xd)
      return ERROR;
   
   struct xbee_remote_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];
   
   l_at=add_16bits_to_at_uchar_cmd_from_int(at, reg_val);
   ret=xbee_atCmdSendAndWaitResp(xd, host, at, l_at, resp, &l_resp, nerr);
   
   if(ret)
      return ERROR;
   map_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   return (int)map_resp->cmd_status;
}


int at_get_local_char_array_reg(xbee_xd_t *xd, unsigned char at_cmd[2], char *reg_val, uint16_t *l_reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   uint16_t     l_at;
   int16_t ret;
   
   if(!xd)
      return ERROR;

   struct xbee_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];l_at=2;
   
   int16_t err;
   
   ret=xbee_atCmdSendAndWaitResp(xd,NULL,at,l_at,resp,&l_resp, &err);
   if(ret<0)
      return ERROR;
   
   map_resp=(struct xbee_cmd_response_s *)resp;
   if(l_reg_val)
      *l_reg_val=l_resp-5;
   
   if(reg_val)
   {
      for(int i=0;i<(l_resp-5);i++)
         reg_val[i]=map_resp->at_cmd_data[i];
   }
   
   return (int)map_resp->cmd_status;
}


void addr_64_char_array_to_int(char *h, char *l, uint32_t *addr_64_h, uint32_t *addr_64_l)
{
   *addr_64_h=0;
   for(int i=0;i<4;i++)
      *addr_64_h=(*addr_64_h) | (int32_t)((unsigned char)h[i])<<((3-i)*8);
   
   *addr_64_l=0;
   for(int i=0;i<4;i++)
      *addr_64_l=*addr_64_l | (int32_t)((unsigned char)l[i])<<((3-i)*8);
}


PyObject *stmt_to_pydict_device(sqlite3_stmt * stmt)
{
   PyObject *data_dict=NULL;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
   
   addLong_to_pydict(data_dict, get_token_by_id(DEVICE_ID_ID), sqlite3_column_int(stmt, 0));
   addString_to_pydict(data_dict, get_token_by_id(DEVICE_NAME_ID), (char *)sqlite3_column_text(stmt, 6));
   addLong_to_pydict(data_dict, get_token_by_id(DEVICE_TYPE_ID_ID), sqlite3_column_int(stmt, 5));
   addLong_to_pydict(data_dict, get_token_by_id(DEVICE_LOCATION_ID_ID), sqlite3_column_int(stmt, 1));
   addString_to_pydict(data_dict, get_token_by_id(DEVICE_INTERFACE_NAME_ID), (char *)sqlite3_column_text(stmt, 7));
   addString_to_pydict(data_dict, get_token_by_id(DEVICE_INTERFACE_TYPE_NAME_ID), (char *)sqlite3_column_text(stmt, 9));
   addString_to_pydict(data_dict, get_token_by_id(DEVICE_STATE_ID), (char *)sqlite3_column_text(stmt, 2));
   addString_to_pydict(data_dict, get_token_by_id(DEVICE_TYPE_PARAMETERS_ID), (char *)sqlite3_column_text(stmt, 4));
   
   uint32_t addr_h, addr_l;
   
   if(sscanf((char *)sqlite3_column_text(stmt, 10), "MESH://%x-%x", &addr_h, &addr_l)==2)
   {
      addLong_to_pydict(data_dict, get_token_by_id(ADDR_H_ID), (long)addr_h);
      addLong_to_pydict(data_dict, get_token_by_id(ADDR_L_ID), (long)addr_l);
   }
   
   return data_dict;
}


PyObject *stmt_to_pydict_interface(sqlite3_stmt * stmt)
{
   PyObject *data_dict;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
   
   addLong_to_pydict(data_dict, get_token_by_id(INTERFACE_ID_ID), (long)sqlite3_column_int(stmt, 1));
   addLong_to_pydict(data_dict, get_token_by_id(INTERFACE_TYPE_ID_ID), (long)sqlite3_column_int(stmt, 2));
   addString_to_pydict(data_dict, get_token_by_id(INTERFACE_NAME_ID), (char *)sqlite3_column_text(stmt, 3));
   addLong_to_pydict(data_dict, get_token_by_id(INTERFACE_STATE_ID), (long)sqlite3_column_int(stmt, 7));
   
   uint32_t addr_h, addr_l;
   
   if(sscanf((char *)sqlite3_column_text(stmt, 5), "MESH://%x-%x", &addr_h, &addr_l)==2)
   {
      addLong_to_pydict(data_dict, get_token_by_id(ADDR_H_ID), (long)addr_h);
      addLong_to_pydict(data_dict, get_token_by_id(ADDR_L_ID), (long)addr_l);
   }
   
   return data_dict;
}


int16_t _interface_type_002_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr xplMsg, xPL_ObjectPtr userValue)
{
   xPL_NameValueListPtr xplBody;
   char *device;
   int ret;
   int err;
   
   
   interface_type_002_t *interface=(interface_type_002_t *)userValue;
   struct callback_xpl_data_s *params=(struct callback_xpl_data_s *)interface->xPL_callback_data;
   
   sqlite3 *params_db = params->param_db;
   
   if(!params->mainThreadState)
      params->mainThreadState=PyThreadState_Get();
   if(!params->myThreadState)
      params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   
   xplBody = xPL_getMessageBody(xplMsg);
   device  = xPL_getNamedValue(xplBody, get_token_by_id(XPL_DEVICE_ID));
   
   if(!device)
      return NOERROR;
   
   char sql[1024];
   sqlite3_stmt * stmt;
   
   sprintf(sql,"%s WHERE sensors_actuators.name='%s';", sql_select_device_info, device);
   ret = sqlite3_prepare_v2(params_db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
      return NOERROR;
   }
   
   while(1)
   {
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         parsed_parameters_t *plugin_params=NULL;
         int nb_plugin_params;
         
         plugin_params=malloc_parsed_parameters((char *)sqlite3_column_text(stmt, 3), valid_xbee_plugin_params, &nb_plugin_params, &err, 0);
         if(!plugin_params || !plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s)
         {
            if(plugin_params)
            {
               free(plugin_params);
               plugin_params=NULL;
            }
            continue; // si pas de parametre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin pour ce capteur
         }
         plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
         if(plugin_elem)
         {
            plugin_elem->type_elem=XPLMSG;
            
            { // appel des fonctions Python
               PyEval_AcquireLock();
               PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
               
               plugin_elem->aDict=stmt_to_pydict_device(stmt);
               addLong_to_pydict(plugin_elem->aDict, get_token_by_id(ID_XBEE_ID), (long)interface->xd);
               PyObject *d=xplMsgToPyDict(xplMsg);
               PyDict_SetItemString(plugin_elem->aDict, "xplmsg", d);
               addLong_to_pydict(plugin_elem->aDict, get_token_by_id(DEVICE_TYPE_ID_ID), sqlite3_column_int(stmt, 5));
               Py_DECREF(d);
               
               if(plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
                  addString_to_pydict(plugin_elem->aDict, get_token_by_id(DEVICE_PARAMETERS_ID), plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
               
               PyThreadState_Swap(tempState);
               PyEval_ReleaseLock();
            } // fin appel des fonctions Python
            
            pythonPluginServer_add_cmd(plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
            
            free_parsed_parameters(plugin_params, nb_plugin_params);
            free(plugin_elem);
         }
         
         free(plugin_params);
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
   return NOERROR;
}


mea_error_t _inteface_type_002_xbeedata_callback(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *h, char *l)
{
   struct timeval tv;
   struct callback_data_s *callback_data;
   data_queue_elem_t *e;
   
   
   callback_data=(struct callback_data_s *)data;
   
   gettimeofday(&tv, NULL);
   
   e=(data_queue_elem_t *)malloc(sizeof(data_queue_elem_t));
   
   memcpy(e->addr_64_h, h, 4);
   memcpy(e->addr_64_l, l, 4);
   e->cmd=(unsigned char *)malloc(l_cmd+1);
   memcpy(e->cmd,cmd,l_cmd);
   e->l_cmd=l_cmd;
   memcpy(&e->tv,&tv,sizeof(struct timeval));
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&callback_data->callback_lock) );
   pthread_mutex_lock(callback_data->callback_lock);
   in_queue_elem(callback_data->queue, e);
   if(callback_data->queue->nb_elem>=1)
      pthread_cond_broadcast(callback_data->callback_cond);
   pthread_mutex_unlock(callback_data->callback_lock);
   pthread_cleanup_pop(0);
   
   return NOERROR;
}


mea_error_t _interface_type_002_commissionning_callback(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *addr_h, char *addr_l)
{
   struct xbee_node_identification_response_s *nd_resp;
//   struct xbee_node_identification_nd_data_s *nd_data;
   int rval=0;
   int err;
   
   struct callback_commissionning_data_s *callback_commissionning=(struct callback_commissionning_data_s *)data;
   sqlite3 *params_db=callback_commissionning->param_db;
   
   nd_resp=(struct xbee_node_identification_response_s *)cmd;
//   nd_data=(struct xbee_node_identification_nd_data_s *)(nd_resp->nd_data+strlen((char *)nd_resp->nd_data)+1);
   
   char addr[18];
   sprintf(addr,
           //           "%02x%02x%02x%02x-%02x%02x%02x%02x",
           printf_mask_addr,
           nd_resp->addr_64_h[0],
           nd_resp->addr_64_h[1],
           nd_resp->addr_64_h[2],
           nd_resp->addr_64_h[3],
           nd_resp->addr_64_l[0],
           nd_resp->addr_64_l[1],
           nd_resp->addr_64_l[2],
           nd_resp->addr_64_l[3]);
   
   VERBOSE(9) printf("%s (%s) : commissionning request received from %s.\n", INFO_STR, __func__, addr);
   
   char sql[1024];
   sqlite3_stmt * stmt;
   sprintf(sql,"%s WHERE interfaces.dev ='MESH://%s';", sql_select_interface_info, addr);
   
   int ret = sqlite3_prepare_v2(params_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
      return ERROR;
   }
   
   int s = sqlite3_step(stmt);
   if (s == SQLITE_ROW) // on ne traite que la premiere ligne si elle existe car on ne devrait pas pouvoir avoir n ligne sur cette requete
   {
      parsed_parameters_t *plugin_params=NULL;
      int nb_plugin_params;
      
      plugin_params=malloc_parsed_parameters((char *)sqlite3_column_text(stmt, 6), valid_xbee_plugin_params, &nb_plugin_params, &err, 0);
      if(!plugin_params || !plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s)
      {
         if(plugin_params)
         {
            free(plugin_params);
            plugin_params=NULL;
         }
         sqlite3_finalize(stmt);
         return ERROR;
      }
      
      plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
      if(plugin_elem)
      {
         plugin_elem->type_elem=COMMISSIONNING;
         
         if(!callback_commissionning->mainThreadState)
            callback_commissionning->mainThreadState=PyThreadState_Get();
         if(!callback_commissionning->myThreadState)
            callback_commissionning->myThreadState = PyThreadState_New(callback_commissionning->mainThreadState->interp);
         
         { // appel des fonctions Python
            PyEval_AcquireLock();
            PyThreadState *tempState = PyThreadState_Swap(callback_commissionning->myThreadState);
            
            plugin_elem->aDict=stmt_to_pydict_interface(stmt);
            addLong_to_pydict(plugin_elem->aDict, get_token_by_id(ID_XBEE_ID), (long)callback_commissionning->xd);
            addLong_to_pydict(plugin_elem->aDict, "LOCAL_XBEE_ADDR_H", (long)callback_commissionning->local_xbee->l_addr_64_h);
            addLong_to_pydict(plugin_elem->aDict, "LOCAL_XBEE_ADDR_L", (long)callback_commissionning->local_xbee->l_addr_64_l);
            
            if(plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
               addString_to_pydict(plugin_elem->aDict, get_token_by_id(INTERFACE_PARAMETERS_ID), plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
            
            PyThreadState_Swap(tempState);
            PyEval_ReleaseLock();
         } // fin appel des fonctions Python
         
         pythonPluginServer_add_cmd(plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
         
         FREE(plugin_elem);
         free_parsed_parameters(plugin_params, nb_plugin_params);
         FREE(plugin_params);
      }
   }
   sqlite3_finalize(stmt);
   
   VERBOSE(9) {
      if(!rval)
         printf("%s (%s) : commissionning request transmitted.\n", INFO_STR, __func__);
      else
         printf("%s (%s) : can't transmit commissionning request.\n", INFO_STR, __func__);
   }
   return rval;
}


void *_thread_interface_type_002_xbeedata_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   if(params->e)
   {
      free(params->e->cmd);
      free(params->e);
      params->e=NULL;
   }

   if(params->myThreadState)
   {
      PyThreadState_Clear(params->myThreadState);
      PyThreadState_Delete(params->myThreadState);
      params->myThreadState=NULL;
   }
   
   if(params->plugin_params)
   {
      free_parsed_parameters(params->plugin_params, params->nb_plugin_params);
      free(params->plugin_params);
      params->plugin_params=NULL;
   }
   
   if(params->stmt)
   {
      sqlite3_finalize(params->stmt);
      params->stmt=NULL;
   }
   
   if(params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      clear_queue(params->queue, _iodata_free_queue_elem);
   FREE(params->queue);
   FREE(params);

   return NULL;
}


void *_thread_interface_type_002_xbeedata(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_002_xbeedata_cleanup, (void *)params );
   
   sqlite3 *params_db=params->param_db;
   data_queue_elem_t *e;
   int ret;
   
   params->plugin_params=NULL;
   params->nb_plugin_params=0;
   params->e=NULL;
   params->stmt=NULL;
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->mainThreadState = PyThreadState_Get();
   params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   
   while(1)
   {
      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&params->callback_lock) );
      pthread_mutex_lock(&params->callback_lock);
      
      if(params->queue->nb_elem==0)
      {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 30; // timeout de 30 secondes
         ts.tv_nsec = 0;
         
         ret=pthread_cond_timedwait(&params->callback_cond, &params->callback_lock, &ts);
         if(ret)
         {
            if(ret==ETIMEDOUT)
            {
               DEBUG_SECTION fprintf(stderr,"%s (%s) : Nb elements in queue after TIMEOUT : %ld)\n", DEBUG_STR, __func__, params->queue->nb_elem);
            }
            else
            {
               // autres erreurs à traiter
            }
         }
      }
      
      ret=out_queue_elem(params->queue, (void **)&e);
      params->e=e;
      pthread_mutex_unlock(&params->callback_lock);
      pthread_cleanup_pop(0);
      
      if(!ret)
      {
         char sql[1024];
         char addr[18];
         
         sprintf(addr,
                 printf_mask_addr, // "%02x%02x%02x%02x-%02x%02x%02x%02x"
                 e->addr_64_h[0],
                 e->addr_64_h[1],
                 e->addr_64_h[2],
                 e->addr_64_h[3],
                 e->addr_64_l[0],
                 e->addr_64_l[1],
                 e->addr_64_l[2],
                 e->addr_64_l[3]);
         VERBOSE(9) fprintf(stderr, "%s  (%s) : data from = %s received\n",INFO_STR, __func__, addr);
         
         sprintf(sql,"%s WHERE interfaces.dev ='MESH://%s';", sql_select_device_info, addr);
         ret = sqlite3_prepare_v2(params_db,sql,strlen(sql)+1,&(params->stmt),NULL);
         if(ret)
         {
            VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
            raise(SIGQUIT); // arreur du process
            sleep(5); // on attend 5 secondes avant de s'arrêter seul.
            pthread_exit(NULL);
         }
         
         while(1)
         {
            int s = sqlite3_step(params->stmt);
            if (s == SQLITE_ROW)
            {
               int err;
               
               params->plugin_params=malloc_parsed_parameters((char *)sqlite3_column_text(params->stmt, 3), valid_xbee_plugin_params, &(params->nb_plugin_params), &err, 0);
               if(!params->plugin_params || !params->plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s)
               {
                  if(params->plugin_params)
                  {
                     free(params->plugin_params);
                     params->plugin_params=NULL;
                  }
                  continue; // si pas de parametre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin
               }
               
               plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
               if(plugin_elem)
               {
                  pthread_cleanup_push( (void *)free, (void *)plugin_elem );

                  plugin_elem->type_elem=XBEEDATA;
                  
                  memcpy(plugin_elem->buff, e->cmd, e->l_cmd);
                  plugin_elem->l_buff=e->l_cmd;
                  
                  uint16_t data_type = e->cmd[0]; // 0x90 serie, 0x92 iodata
                  
                  { // appel des fonctions Python
                     PyEval_AcquireLock();
                     pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // trop compliquer de traiter avec pthread_cleanup => on interdit les arrêts lors des commandes python
                     PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
                     
                     plugin_elem->aDict=stmt_to_pydict_device(params->stmt);
                     
                     PyObject *value;
                     value = PyBuffer_FromMemory(plugin_elem->buff, plugin_elem->l_buff);
                     PyDict_SetItemString(plugin_elem->aDict, "cmd", value);
                     Py_DECREF(value);
                     addLong_to_pydict(plugin_elem->aDict, "l_cmd_data", (long)plugin_elem->l_buff-12);
                     
                     value = PyBuffer_FromMemory(&(plugin_elem->buff[12]), plugin_elem->l_buff-12);
                     PyDict_SetItemString(plugin_elem->aDict, "cmd_data", value);
                     Py_DECREF(value);
                     addLong_to_pydict(plugin_elem->aDict, "l_cmd_data", (long)plugin_elem->l_buff-12);
                     addLong_to_pydict(plugin_elem->aDict, "data_type", (long)data_type);
                     addLong_to_pydict(plugin_elem->aDict, get_token_by_id(ID_XBEE_ID), (long)params->xd);
                     
                     if(params->plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
                        addString_to_pydict(plugin_elem->aDict, get_token_by_id(DEVICE_PARAMETERS_ID), params->plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
                     
                     PyThreadState_Swap(tempState);
                     PyEval_ReleaseLock();
                     pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                     pthread_testcancel();
                  } // fin appel des fonctions Python
                  
                  pythonPluginServer_add_cmd(params->plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, params->plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
                  
                  pthread_cleanup_pop(0);
                  
                  free(plugin_elem);
                  plugin_elem=NULL;
               }
               else
               {
                  VERBOSE(1) {
                     fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
                     perror("");
                  }
                  pthread_exit(PTHREAD_CANCELED);
               }
               free_parsed_parameters(params->plugin_params, params->nb_plugin_params);
               free(params->plugin_params);
               params->plugin_params=NULL;
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
               break; // traitement des erreurs √  affiner avant break ...
            }
         }
         
         free(e->cmd);
         free(e);
         params->e=NULL;
         e=NULL;
         
         pthread_testcancel();
      }
      else
      {
         // pb d'accès aux données de la file
         VERBOSE(9) fprintf(stderr,"%s (%s) : out_queue_elem - no data in queue\n", DEBUG_STR, __func__);
      }
      pthread_testcancel();
   }
   
   pthread_cleanup_pop(0);
   
   return NULL;
}


pthread_t *start_interface_type_002_xbeedata_thread(interface_type_002_t *i002, xbee_xd_t *xd, sqlite3 *db, tomysqldb_md_t *md,thread_f function)
{
   pthread_t *thread=NULL;
   struct thread_params_s *params=NULL;
   struct callback_data_s *callback_xbeedata=NULL;
   
   params=malloc(sizeof(struct thread_params_s));
   if(!params)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   params->queue=(queue_t *)malloc(sizeof(queue_t));
   if(!params->queue)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   init_queue(params->queue);

   params->xd=xd;
   params->md=md;
   params->param_db=db;
   pthread_mutex_init(&params->callback_lock, NULL);
   pthread_cond_init(&params->callback_cond, NULL);
   params->data=(void *)i002;
   params->mainThreadState = NULL;
   params->myThreadState = NULL;
   
   // préparation des données pour les callback io_data et data_flow dont les données sont traitées par le même thread
   callback_xbeedata=(struct callback_data_s *)malloc(sizeof(struct callback_data_s));
   if(!callback_xbeedata)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   callback_xbeedata->xd=xd;
   callback_xbeedata->param_db=db;
   callback_xbeedata->callback_lock=&params->callback_lock;
   callback_xbeedata->callback_cond=&params->callback_cond;
   callback_xbeedata->queue=params->queue;
   
   xbee_set_iodata_callback2(xd, _inteface_type_002_xbeedata_callback, (void *)callback_xbeedata);
   xbee_set_dataflow_callback2(xd, _inteface_type_002_xbeedata_callback, (void *)callback_xbeedata);

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto clean_exit;
   
   return thread;
   
clean_exit:
   FREE(thread);
   FREE(callback_xbeedata);
   if(params && params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      clear_queue(params->queue, _iodata_free_queue_elem);
   if(params)
      FREE(params->queue);
   FREE(params);
   return NULL;
}


mea_error_t stop_interface_type_002(interface_type_002_t *i002)
{
   VERBOSE(5) fprintf(stderr,"%s  (%s) : shutdown interface_type_002 thread.\n",INFO_STR, __func__);

   FREE(i002->xPL_callback_data);
   if(i002->xPL_callback)
      i002->xPL_callback=NULL;
   
   if(i002->xd->dataflow_callback_data)
   {
      free(i002->xd->dataflow_callback_data);
      i002->xd->dataflow_callback_data=NULL;
      i002->xd->io_callback_data=NULL;
   }
   
   if(i002->thread)
   {
      pthread_cancel(*(i002->thread));
      pthread_join(*(i002->thread), NULL);
   }
   FREE(i002->thread);
   
   xbee_remove_commissionning_callback(i002->xd);
   FREE(i002->xd->commissionning_callback_data);

   xbee_close(i002->xd);

   FREE(i002->xd);
   FREE(i002->local_xbee);

   VERBOSE(5) fprintf(stderr,"%s  (%s) : counter thread is down.\n",INFO_STR, __func__);
   
   return NOERROR;
}


mea_error_t restart_interface_type_002(interface_type_002_t *i002,sqlite3 *db, tomysqldb_md_t *md)
{
   char full_dev[80];
   char dev[80];
   int id_interface;
   int ret;
   
   sscanf(i002->xd->serial_dev_name,"/dev/%s",full_dev);
   sprintf(dev,"SERIAL://%s",full_dev);
   
   id_interface=i002->id_interface;
   
   stop_interface_type_002(i002);

   for(int16_t i=0;i<5;i++)
   {
      ret=start_interface_type_002(i002, db, id_interface, (const unsigned char *)dev, md);
      if(ret!=-1)
         break;
      sleep(5);
   }
   
   return ret;
}


mea_error_t check_status_interface_type_002(interface_type_002_t *it002)
{
   if(it002->xd->signal_flag!=0)
      return ERROR;
   
   return NOERROR;
}


mea_error_t start_interface_type_002(interface_type_002_t *i002, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md)
{
//   static const char *fn_name="start_interface_type_002";
   const char *fn_name=__func__;
   
   char unix_dev[80];
   char buff[80];
   int fd=0;
   int16_t nerr;
   int ret;
   
   xbee_xd_t *xd=NULL;
   xbee_host_t *local_xbee=NULL;
   
   struct callback_commissionning_data_s *commissionning_callback_params=NULL;
   struct callback_xpl_data_s *xpl_callback_params=NULL;
//   struct callback_data_s *xbeedata_callback_params=NULL;
   
   i002->thread=NULL;
   
   if(sscanf((char *)dev,"SERIAL://%s",buff)==1)
      sprintf(unix_dev,"/dev/%s",buff);
   else
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : unknow device name - %s\n", ERROR_STR, __func__, dev);
      goto clean_exit;
   }
   
   xd=(xbee_xd_t *)malloc(sizeof(xbee_xd_t));
   if(!xd)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   
   fd=xbee_init(xd, unix_dev, B9600); // mettre la vitesse dans les parametres de l'interface
   if (fd == -1)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : init_xbee - Unable to open serial port (%s) : ", ERROR_STR, __func__, unix_dev);
         perror("");
      }
      goto clean_exit;
   }
   i002->xd=xd;
   
   /*
    * Préparation du réseau XBEE
    */
   local_xbee=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connect√©
   if(!local_xbee)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   i002->local_xbee=local_xbee;
   
   
   // r√©cupération de l'adresse de l'xbee connecter au PC (pas forcement le coordinateur).
   uint32_t addr_64_h;
   uint32_t addr_64_l;
   {
      unsigned char at_cmd[2];
      uint16_t l_reg_val;
      char addr_l[4];
      char addr_h[4];
      
      memset(addr_l,0,4);
      memset(addr_h,0,4);
      
      // lecture de l'adresse de l'xbee local
      at_cmd[0]='S';at_cmd[1]='H';
      do { ret=at_get_local_char_array_reg(xd, at_cmd, (char *)addr_h, &l_reg_val, &nerr); } while (ret==-1); // a revoir boucle infinie possible ...
      at_cmd[0]='S';at_cmd[1]='L';
      do { ret=at_get_local_char_array_reg(xd, at_cmd, (char *)addr_l, &l_reg_val, &nerr); } while (ret==-1); // a revoir boucle infinie possible ...
      addr_64_char_array_to_int(addr_h, addr_l, &addr_64_h, &addr_64_l);
   }
   VERBOSE(9) fprintf(stderr, "INFO  (%s) : local address is : %02x-%02x\n", fn_name, addr_64_h, addr_64_l);
   xbee_get_host_by_addr_64(xd, local_xbee, addr_64_h, addr_64_l, &nerr);
   
   /* a remplacer par l'appel du plugin python associé a l'interface s'il existe
   int xbee_flag=0;
   for(int i=0;i<5;i++) // 5 essais
   {
      if(at_set_16bits_reg_from_int(xd, local_xbee, "SP", 2000, &nerr))
      {
         xbee_perror(nerr);
      }
      else
      {
         xbee_flag=1;
         break;
      }
      sleep(1);
   }
   if(xbee_flag==0)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't ...\n", ERROR_STR, fn_name);
      goto clean_exit;
   }
   */
   
   /*
    * parametrage du réseau
    */
   // découverte des xbee du réseau
   xbee_start_network_discovery(xd, &nerr); // lancement de la d√©couverte "asynchrone"
   
   // on attend que la découverte soit terminée (à remplacer par un "evenement")
   sleep(10);
   
   
   // on positionne certain parametre sur tous les routeurs/coordinateur
   //   all_known_routers_setup(xd);
   
   /*
    * Gestion des sous-interfaces
    */
   i002->thread=start_interface_type_002_xbeedata_thread(i002, xd, db, md, (thread_f)_thread_interface_type_002_xbeedata);

   //
   // gestion du commissionnement : mettre une zone donnees specifique au commissionnement
   //
   commissionning_callback_params=(struct callback_commissionning_data_s *)malloc(sizeof(struct callback_commissionning_data_s));
   if(!commissionning_callback_params)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   commissionning_callback_params->xd=xd;
   commissionning_callback_params->param_db=db;
   commissionning_callback_params->mainThreadState=NULL;
   commissionning_callback_params->myThreadState=NULL;
   commissionning_callback_params->local_xbee=local_xbee;
   xbee_set_commissionning_callback2(xd, _interface_type_002_commissionning_callback, (void *)commissionning_callback_params);
   
   //
   // gestion des demandes xpl : ajouter une zone de donnees specifique au callback xpl (pas simplement passe i002).
   //
   xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s));
   if(!xpl_callback_params)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   xpl_callback_params->param_db=db;
   xpl_callback_params->mainThreadState=NULL;
   xpl_callback_params->myThreadState=NULL;
   
   i002->xPL_callback_data=xpl_callback_params;
   i002->xPL_callback=_interface_type_002_xPL_callback;
   
   return NOERROR;
   
clean_exit:
   if(xd)
   {
      xbee_remove_commissionning_callback(xd);
      xbee_remove_iodata_callback(xd);
      xbee_remove_dataflow_callback(xd);
   }
   
   // vider la queue de callback_data_iodata
   if(i002->thread)
      stop_interface_type_002(i002);
   
//   if(xbeedata_callback_params)
//      free(xbeedata_callback_params);
   
   if(commissionning_callback_params)
      free(commissionning_callback_params);
   
   if(xpl_callback_params)
      free(xpl_callback_params);
   
   FREE(local_xbee);
   if(xd)
   {
      if(fd)
         xbee_close(xd);
      xbee_free_xd(xd);
   }
   
   return ERROR;
}
