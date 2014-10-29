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
#include "consts.h"

#include "tokens.h"
#include "error.h"
#include "debug.h"
#include "macros.h"
#include "memory.h"

#include "xbee.h"
#include "dbServer.h"
#include "parameters_utils.h"
#include "mea_api.h"
#include "pythonPluginServer.h"
#include "python_api_utils.h"

#include "processManager.h"
#include "notify.h"

#include "interfacesServer.h"
#include "interface_type_002.h"


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_xbee_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define XBEE_PLUGIN_PARAMS_PLUGIN      0
#define XBEE_PLUGIN_PARAMS_PARAMETERS  1

char *sql_select_device_info="SELECT sensors_actuators.id_sensor_actuator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, types.parameters, sensors_actuators.id_type, lower(sensors_actuators.name), lower(interfaces.name), interfaces.id_type, (SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), interfaces.dev FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";
char *sql_select_interface_info="SELECT * FROM interfaces";

char *printf_mask_addr="%02x%02x%02x%02x-%02x%02x%02x%02x";


struct callback_data_s // donnee "userdata" pour les callbacks
{
   PyThreadState   *mainThreadState;
   sqlite3         *param_db;
   xbee_xd_t       *xd;
   queue_t         *queue;
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
   pthread_mutex_t       callback_lock;
   pthread_cond_t        callback_cond;
   PyThreadState        *mainThreadState;
   PyThreadState        *myThreadState;
   parsed_parameters_t  *plugin_params;
   int                   nb_plugin_params;
   sqlite3_stmt         *stmt;
   data_queue_elem_t    *e;
//   int                   monitoring_id;
   interface_type_002_t *i002;
};


void set_interface_type_002_isnt_running(void *data)
{
   interface_type_002_t *i002 = (interface_type_002_t *)data;
   i002->thread_is_running=0;
}


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

mea_error_t print_frame(int ret, unsigned char *resp, uint16_t l_resp)
{
   DEBUG_SECTION {
      for(int i=0;i<l_resp;i++)
         fprintf(stderr,"[%03x - %c]",resp[i],resp[i]);
      printf("\n");
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
   
   h=(val/0xFF) & 0xFF;
   l=val%0xFF;
   
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
   unsigned char at[5];
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
   {
      params->mainThreadState=PyThreadState_Get();
   }
   if(!params->myThreadState)
      params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   
   xplBody = xPL_getMessageBody(xplMsg);
   device  = xPL_getNamedValue(xplBody, get_token_by_id(XPL_DEVICE_ID));
   
   if(!device)
      return ERROR;
   
   char sql[1024];
   sqlite3_stmt * stmt;
   
   sprintf(sql,"%s WHERE lower(sensors_actuators.name)='%s' and sensors_actuators.state='1';", sql_select_device_info, device);
   ret = sqlite3_prepare_v2(params_db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
      return ERROR;
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
               PyObject *d=mea_xplMsgToPyDict(xplMsg);
               PyDict_SetItemString(plugin_elem->aDict, "xplmsg", d);
               addLong_to_pydict(plugin_elem->aDict, get_token_by_id(DEVICE_TYPE_ID_ID), sqlite3_column_int(stmt, 5));
               Py_DECREF(d);
               
               if(plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
                  addString_to_pydict(plugin_elem->aDict, get_token_by_id(DEVICE_PARAMETERS_ID), plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
               
               PyThreadState_Swap(tempState);
               PyEval_ReleaseLock();
            } // fin appel des fonctions Python
            
            pythonPluginServer_add_cmd(plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
            
            free_parsed_parameters(plugin_params, nb_plugin_params);
            free(plugin_elem);
            plugin_elem=NULL;
         }
         
         free(plugin_params);
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
   int rval=0;
   int err;
   
   struct callback_commissionning_data_s *callback_commissionning=(struct callback_commissionning_data_s *)data;
   sqlite3 *params_db=callback_commissionning->param_db;
   
   nd_resp=(struct xbee_node_identification_response_s *)cmd;
   
   char addr[18];
   // pourquoi ne pas utiliser addr_h et addr_l (snprintf(_addr,sizeof(_addr),"%08x-%08x",addr_h,addr_l))
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
   
   VERBOSE(9) printf("%s (%s)  : commissionning request received from %s.\n", INFO_STR, __func__, addr);
    
   char sql[1024];
   sqlite3_stmt * stmt;
   sprintf(sql,"%s WHERE interfaces.dev ='MESH://%s' AND interfaces.state='2';", sql_select_interface_info, addr);
   
   int ret = sqlite3_prepare_v2(params_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
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
         {
            callback_commissionning->mainThreadState=PyThreadState_Get();
         }
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
         
         pythonPluginServer_add_cmd(plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
         
         free(plugin_elem);
         plugin_elem=NULL;
      }
      
      free_parsed_parameters(plugin_params, nb_plugin_params);
      free(plugin_params);
      plugin_params=NULL;
   }
   sqlite3_finalize(stmt);
   
   VERBOSE(9) {
      if(!rval)
         printf("%s (%s)  : commissionning request transmitted.\n", INFO_STR, __func__);
      else
         printf("%s (%s)  : can't transmit commissionning request.\n", INFO_STR, __func__);
   }
   return rval;
}


void *_thread_interface_type_002_xbeedata_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   if(!params)
      return NULL;
   
   if(params->e)
   {
      free(params->e->cmd);
      params->e->cmd=NULL;
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
   
   if(params->queue)
   {
      free(params->queue);
      params->queue=NULL;
   }

   free(params);
   params=NULL;

   return NULL;
}


void *_thread_interface_type_002_xbeedata(void *args)
/**
 * \brief     Gestion des données asynchrones en provenances d'xbee
 * \details   Les iodata peuvent arriver n'importe quand, il sagit de pouvoir les traiter dés réceptions.
 *            Le callback associé est en charge de poster les données à traiter dans une file qui seront consommées par ce thread.
 * \param     args   ensemble des parametres nécessaires au thread regroupé dans une structure de données (voir struct thread_params_s)
 */
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   pthread_cleanup_push( (void *)set_interface_type_002_isnt_running, (void *)params->i002 );
   params->i002->thread_is_running=1;

   sqlite3 *params_db=params->param_db;
   data_queue_elem_t *e;
   int ret;
   
   params->plugin_params=NULL;
   params->nb_plugin_params=0;
   // params->e=NULL;
   params->stmt=NULL;
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->mainThreadState = PyThreadState_Get();
   params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   
   while(1)
   {
      process_heartbeat(params->i002->monitoring_id);
      
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
         
         sprintf(sql,"%s WHERE interfaces.dev ='MESH://%s' and sensors_actuators.state='1';", sql_select_device_info, addr);
         ret = sqlite3_prepare_v2(params_db,sql,strlen(sql)+1,&(params->stmt),NULL);
         if(ret)
         {
            VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, __func__, sqlite3_errmsg (params_db));
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
                  continue; // si pas de paramètre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin
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
                     pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // trop compliquer de traiter avec pthread_cleanup => on interdit les arrets lors des commandes python
                     PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
                     
                     plugin_elem->aDict=stmt_to_pydict_device(params->stmt);
                     
                     PyObject *value;

                     value = PyByteArray_FromStringAndSize(plugin_elem->buff, (long)plugin_elem->l_buff);
                     PyDict_SetItemString(plugin_elem->aDict, "cmd", value);
                     Py_DECREF(value);
                     addLong_to_pydict(plugin_elem->aDict, "l_cmd", (long)plugin_elem->l_buff);

                     value = PyByteArray_FromStringAndSize(&(plugin_elem->buff[12]), (long)plugin_elem->l_buff-12);
                     PyDict_SetItemString(plugin_elem->aDict, "cmd_data", value);
                     Py_DECREF(value);
                     addLong_to_pydict(plugin_elem->aDict, "l_cmd_data", (long)plugin_elem->l_buff-12);

/*
                     value = PyBuffer_FromMemory(&(plugin_elem->buff[12]), plugin_elem->l_buff-12);
                     PyDict_SetItemString(plugin_elem->aDict, "cmd_data", value);
                     Py_DECREF(value);
                     addLong_to_pydict(plugin_elem->aDict, "l_cmd_data", (long)plugin_elem->l_buff-12);

                     value = PyBuffer_FromMemory(plugin_elem->buff, plugin_elem->l_buff);
                     PyDict_SetItemString(plugin_elem->aDict, "cmd", value);
                     Py_DECREF(value);
                     addLong_to_pydict(plugin_elem->aDict, "l_cmd", (long)plugin_elem->l_buff);
*/

                     addLong_to_pydict(plugin_elem->aDict, "data_type", (long)data_type);
                     addLong_to_pydict(plugin_elem->aDict, get_token_by_id(ID_XBEE_ID), (long)params->xd);
                     
                     if(params->plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
                        addString_to_pydict(plugin_elem->aDict, get_token_by_id(DEVICE_PARAMETERS_ID), params->plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
                     
                     PyThreadState_Swap(tempState);
                     PyEval_ReleaseLock();
                     pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // on réauthorise les arrêts 
                     pthread_testcancel(); // on test tout de suite pour être sûr qu'on a pas ratté une demande d'arrêt
                  } // fin appel des fonctions Python
                  pythonPluginServer_add_cmd(params->plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
                  
                  free(plugin_elem);
                  plugin_elem=NULL;
                  
                  pthread_cleanup_pop(0);
               }
               else
               {
                  VERBOSE(2) {
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
               break; // traitement des erreurs Ã  affiner avant break ...
            }
         }
         
         free(e->cmd);
         e->cmd=NULL;
         free(e);
         e=NULL;
         
         pthread_testcancel();
      }
      else
      {
         // pb d'accès aux données de la file
         DEBUG_SECTION fprintf(stderr,"%s (%s) : out_queue_elem - no data in queue\n", DEBUG_STR, __func__);
      }
      pthread_testcancel();
   }
   
   pthread_cleanup_pop(1);

   return NULL;
}


// xd est déjà dans i002 pourquoi le passer en parametre ...
pthread_t *start_interface_type_002_xbeedata_thread(interface_type_002_t *i002, xbee_xd_t *xd, sqlite3 *db, tomysqldb_md_t *md,thread_f function)
/**  
 * \brief     Demarrage du thread de gestion des données (non solicitées) en provenance des xbee
 * \param     i002           descripteur de l'interface
 * \param     xd             descripteur de com. avec l'xbee
 * \param     db             descripteur ouvert de la base de paramétrage  
 * \param     md             descripteur ouvert de la base d'historique
 * \param     function       function principale du thread à démarrer 
 * \return    ERROR ou NOERROR
 **/ 
{
   pthread_t *thread=NULL;
   struct thread_params_s *params=NULL;
   struct callback_data_s *callback_xbeedata=NULL;
   
   params=malloc(sizeof(struct thread_params_s));
   if(!params)
   {
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   params->queue=(queue_t *)malloc(sizeof(queue_t));
   if(!params->queue)
   {
      VERBOSE(2) {
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
   params->i002=(void *)i002;
   params->mainThreadState = NULL;
   params->myThreadState = NULL;
//   params->monitoring_id = i002->monitoring_id;
   
   // préparation des données pour les callback io_data et data_flow dont les données sont traitées par le même thread
   callback_xbeedata=(struct callback_data_s *)malloc(sizeof(struct callback_data_s));
   if(!callback_xbeedata)
   {
      VERBOSE(2) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
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
      VERBOSE(2) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
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
   
   if(callback_xbeedata)
   {
      free(callback_xbeedata);
      callback_xbeedata=NULL;
   }

   if(params && params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      clear_queue(params->queue, _iodata_free_queue_elem);

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


//mea_error_t stop_interface_type_002(interface_type_002_t *i002)
int stop_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     arrêt d'une interface de type 2
 * \details   RAZ de tous les structures de données et libération des mémoires allouées
 * \param     i002           descripteur de l'interface  
 * \return    ERROR ou NOERROR
 **/ 
{
   if(!data)
      return -1;

   struct interface_type_002_data_s *start_stop_params=(struct interface_type_002_data_s *)data;

   VERBOSE(1) fprintf(stderr,"%s  (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i002->name);

   if(start_stop_params->i002->xPL_callback_data)
   {
      free(start_stop_params->i002->xPL_callback_data);
      start_stop_params->i002->xPL_callback_data=NULL;
   }
   
   if(start_stop_params->i002->xPL_callback)
   {
      start_stop_params->i002->xPL_callback=NULL;
   }
   
   if(start_stop_params->i002->xd->dataflow_callback_data)
   {
      free(start_stop_params->i002->xd->dataflow_callback_data);
      start_stop_params->i002->xd->dataflow_callback_data=NULL;
   }

   if(start_stop_params->i002->xd->io_callback_data)
   {
      free(start_stop_params->i002->xd->io_callback_data);
      start_stop_params->i002->xd->io_callback_data=NULL;
   }
   
   if(start_stop_params->i002->thread)
   {
      pthread_cancel(*(start_stop_params->i002->thread));
      
      int counter=100;
      int stopped=-1;
      while(counter--)
      {
         if(start_stop_params->i002->thread_is_running)
         {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
         {
            stopped=0;
            break;
         }
      }
      DEBUG_SECTION fprintf(stderr,"%s (%s) : %s, fin après %d itération\n",DEBUG_STR, __func__,start_stop_params->i002->name,100-counter);

      free(start_stop_params->i002->thread);
      start_stop_params->i002->thread=NULL;
   }
   
   xbee_remove_commissionning_callback(start_stop_params->i002->xd);
   
   if(start_stop_params->i002->xd->commissionning_callback_data)
   {
      free(start_stop_params->i002->xd->commissionning_callback_data);
      start_stop_params->i002->xd->commissionning_callback_data=NULL;
   }

   xbee_close(start_stop_params->i002->xd);

   if(start_stop_params->i002->xd)
   {
      free(start_stop_params->i002->xd);
      start_stop_params->i002->xd=NULL;
   }
   
   if(start_stop_params->i002->local_xbee)
   {
      free(start_stop_params->i002->local_xbee);
      start_stop_params->i002->local_xbee=NULL;
   }
   
   VERBOSE(1) fprintf(stderr, "done.\n");
   mea_notify_printf('S', "%s %s", start_stop_params->i002->name, stopped_successfully_str);

   return 0;
}


int restart_interface_type_002(int id)
{
   process_stop(id, NULL, 0);
   sleep(5);
   return process_start(id, NULL, 0);
}


//mea_error_t restart_interface_type_002(interface_type_002_t *i002,sqlite3 *db, tomysqldb_md_t *md)
/**  
 * \brief     Re-demarrage d'une interface de type 2
 * \details   appel stop_interface_type_002 puis start_interface_type_002 en ayant pris soit de récupérer les différentes données nécessaires.
 * \param     db             descripteur ouvert de la base de paramétrage  
 * \param     md             descripteur ouvert de la base d'historique
 * \return    ERROR ou NOERROR
 **/
 /*
{
   char full_dev[80];
   char dev[80];
   int id_interface;
   int ret;
   char *parameters;
   
   sscanf(i002->xd->serial_dev_name,"/dev/%s",full_dev);
   sprintf(dev,"SERIAL://%s:%ld",full_dev,(long)get_speed_from_speed_t(i002->xd->speed));

   id_interface=i002->id_interface;
   
   stop_interface_type_002(i002);

   // récupération des (nouveaux) paramètres dans la base
   char sql_request[255];
   sqlite3_stmt * stmt;
   snprintf(sql_request,sizeof(sql_request),"SELECT parameters FROM interfaces WHERE id_interface=%d", id_interface);
   // traiter l'erreur éventuelle snprintf ici
   ret = sqlite3_prepare_v2(db,sql_request,strlen(sql_request)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg(db));
      return ERROR;
   }
   parameters=NULL;
   while(1)
   {
      int s=sqlite3_step(stmt);
      if(s==SQLITE_ROW)
      {
         parameters=(char *)sqlite3_column_text(stmt, 0);
      }
      else if(s==SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      } 
      else 
      {
         VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg(db));
         sqlite3_finalize(stmt);
         return ERROR;
      }
   }   
   
   for(int16_t i=0;i<5;i++)
   {
      ret=start_interface_type_002(i002, db, id_interface, (const unsigned char *)dev, md, parameters);
      if(ret!=-1)
         break;
      sleep(5);
   }
   
   return ret;
}
*/

int16_t check_status_interface_type_002(interface_type_002_t *it002)
/**  
 * \brief     indique si une anomalie a généré l'emission d'un signal SIGHUP
 * \param     i002           descripteur de l'interface  
 * \return    ERROR signal émis ou NOERROR sinon
 **/ 
{
   if(it002->xd->signal_flag!=0)
      return -1;
   return 0;
}


//mea_error_t start_interface_type_002(interface_type_002_t *i002, sqlite3 *db, int id_interface, const unsigned char *dev_and_speed, tomysqldb_md_t *md, char *parameters)
int start_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     Demarrage d'une interface de type 2
 * \details   ouverture de la communication avec l'xbee point d'entrée MESH, démarrage du thread de gestion des données iodata et xbeedata, mise en place des callback xpl et commissionnement
 * \param     i002           descripteur de l'interface  
 * \param     db             descripteur ouvert de la base de paramétrage  
 * \param     id_interface   identifiant de l'interface
 * \param     dev_and_speed  chemin et vitesse (débit) de l'interface série (sous forme SERIAL://dev:speed
 * \param     parameters     paramètres associés à l'interface
 * \param     md             descripteur ouvert de la base d'historique
 * \return    ERROR ou NOERROR
 **/ 
{
   char dev[81];
   char buff[80];
   speed_t speed;

   int fd=0;
   int16_t nerr;
   int err;
   int ret;
   
   xbee_xd_t *xd=NULL;
   xbee_host_t *local_xbee=NULL;
   
   struct callback_commissionning_data_s *commissionning_callback_params=NULL;
   struct callback_xpl_data_s *xpl_callback_params=NULL;
   
   struct interface_type_002_data_s *start_stop_params=(struct interface_type_002_data_s *)data;

   start_stop_params->i002->thread=NULL;

   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;
   
   char err_str[128];
   
   ret=get_dev_and_speed((char *)start_stop_params->i002->dev, buff, sizeof(buff), &speed);
   if(!ret)
   {
      int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(buff)-1))
      {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            fprintf (stderr, "%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i002->name, err_str);
         goto clean_exit;
      }
   }
   else
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR,__func__,start_stop_params->i002->dev);
      mea_notify_printf('E', "%s can't be launched - incorrect device/speed interface - %s.\n", start_stop_params->i002->name, start_stop_params->i002->dev);
      goto clean_exit;
   }

   xd=(xbee_xd_t *)malloc(sizeof(xbee_xd_t));
   if(!xd)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i002->name, err_str);
      goto clean_exit;
   }
   
   fd=xbee_init(xd, dev, speed);
   if (fd == -1)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : init_xbee - Unable to open serial port (%s).\n", ERROR_STR, __func__, dev);
      }
      mea_notify_printf('E', "%s : unable to open serial port (%s) - %s.\n", start_stop_params->i002->name, dev, err_str);
      goto clean_exit;
   }
   start_stop_params->i002->xd=xd;
   
   /*
    * Préparation du réseau XBEE
    */
   local_xbee=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   if(!local_xbee)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : %s - %s.\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i002->name, err_str);
      goto clean_exit;
   }
   
   // récupération de l'adresse de l'xbee connecter au PC (pas forcement le coordinateur).
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
   VERBOSE(9) fprintf(stderr, "%s  (%s) : local address is : %02x-%02x\n", INFO_STR, __func__, addr_64_h, addr_64_l);
 
   xbee_get_host_by_addr_64(xd, local_xbee, addr_64_h, addr_64_l, &nerr);

   start_stop_params->i002->local_xbee=local_xbee;
 
   /*
    * exécution du plugin de paramétrage
    */   
   interface_parameters=malloc_parsed_parameters(start_stop_params->parameters, valid_xbee_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s)
   {
      if(interface_parameters)
      {
         // pas de plugin spécifié
         free(interface_parameters);
         interface_parameters=NULL;
         VERBOSE(9) fprintf(stderr, "%s  (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else
      {
         VERBOSE(2) fprintf(stderr, "%s (%s) : invalid python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->parameters);
         mea_notify_printf('E', "%s - invalid python plugin parameters (%s)", start_stop_params->i002->name, start_stop_params->parameters);
         goto clean_exit;
      }
   }
   else
   {
      PyObject *plugin_params_dict=NULL;
      PyObject *pName, *pModule, *pFunc;
      PyObject *pArgs, *pValue=NULL;
      
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *mainThreadState=PyThreadState_Get();
      PyThreadState *myThreadState = PyThreadState_New(mainThreadState->interp);
      
      PyThreadState *tempState = PyThreadState_Swap(myThreadState);

      PyErr_Clear();
      pName = PyString_FromString(interface_parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s);
      pModule = PyImport_Import(pName);
      if(!pModule)
      {
         VERBOSE(5) fprintf(stderr, "%s (%s) : %s not found\n", ERROR_STR, __func__, interface_parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s);
      }
      else
      {
         pFunc = PyObject_GetAttrString(pModule, "mea_init");
         if (pFunc && PyCallable_Check(pFunc))
         {
            // préparation du parametre du module
            plugin_params_dict=PyDict_New();
            addLong_to_pydict(plugin_params_dict, get_token_by_id(ID_XBEE_ID), (long)xd);
//            addLong_to_pydict(plugin_params_dict, get_token_by_id(INTERFACE_ID_ID), id_interface);
            addLong_to_pydict(plugin_params_dict, get_token_by_id(INTERFACE_ID_ID), start_stop_params->i002->id_interface);
            if(interface_parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
               addString_to_pydict(plugin_params_dict, get_token_by_id(INTERFACE_PARAMETERS_ID), interface_parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);

            pArgs = PyTuple_New(1);
            Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
            PyTuple_SetItem(pArgs, 0, plugin_params_dict);
         
            pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
            if (pValue != NULL)
            {
               DEBUG_SECTION fprintf(stderr, "%s (%s) : Result of call of mea_init : %ld\n", DEBUG_STR, __func__, PyInt_AsLong(pValue));
            }
            Py_DECREF(pValue);
            Py_DECREF(pArgs);
            Py_DECREF(plugin_params_dict);
         }
         else
         {
            VERBOSE(5) fprintf(stderr, "%s (%s) : mea_init not fount in %s module\n", ERROR_STR, __func__, interface_parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s);
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

      free_parsed_parameters(interface_parameters, interface_nb_parameters);
      if(interface_parameters)
      {
         free(interface_parameters);
         interface_parameters=NULL;
      }
      interface_nb_parameters=0;
   }

   /*
    * parametrage du réseau
    */
   // déouverte des xbee du réseau
   xbee_start_network_discovery(xd, &nerr); // lancement de la découverte "asynchrone"
      
   /*
    * Gestion des sous-interfaces
    */
   start_stop_params->i002->thread=start_interface_type_002_xbeedata_thread(start_stop_params->i002, xd, start_stop_params->sqlite3_param_db, start_stop_params->myd, (thread_f)_thread_interface_type_002_xbeedata);

   //
   // gestion du commissionnement : mettre une zone donnees specifique au commissionnement
   //
   commissionning_callback_params=(struct callback_commissionning_data_s *)malloc(sizeof(struct callback_commissionning_data_s));
   if(!commissionning_callback_params)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : %s - %s", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i002->name, err_str);
      goto clean_exit;
   }
   commissionning_callback_params->xd=xd;
   commissionning_callback_params->param_db=start_stop_params->sqlite3_param_db;
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
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         fprintf(stderr,"%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->i002->name, err_str);
      goto clean_exit;
   }
//   xpl_callback_params->param_db=db;
   xpl_callback_params->param_db=start_stop_params->sqlite3_param_db;
   xpl_callback_params->mainThreadState=NULL;
   xpl_callback_params->myThreadState=NULL;
   
   start_stop_params->i002->xPL_callback_data=xpl_callback_params;
   start_stop_params->i002->xPL_callback=_interface_type_002_xPL_callback;
   
   VERBOSE(2) fprintf(stderr,"%s (%s) : %s %s.\n", ERROR_STR, __func__,start_stop_params->i002->name, launched_successfully_str);
   mea_notify_printf('S', "%s %s", start_stop_params->i002->name, launched_successfully_str);
   
   return NOERROR;
   
clean_exit:
   if(xd)
   {
      xbee_remove_commissionning_callback(xd);
      xbee_remove_iodata_callback(xd);
      xbee_remove_dataflow_callback(xd);
   }
   
   if(start_stop_params->i002->thread)
   {
      stop_interface_type_002(start_stop_params->i002->monitoring_id, start_stop_params, NULL, 0);
   }
   
   if(interface_parameters)
   {
      free_parsed_parameters(interface_parameters, interface_nb_parameters);
      free(interface_parameters);
      interface_parameters=NULL;
      interface_nb_parameters=0;

   }
   
   if(commissionning_callback_params)
   {
      free(commissionning_callback_params);
      commissionning_callback_params=NULL;
   }
   
   if(xpl_callback_params)
   {
      free(xpl_callback_params);
      xpl_callback_params=NULL;
   }
   
   if(local_xbee)
   {
      free(local_xbee);
      local_xbee=NULL;
   }
   
   if(xd)
   {
      if(fd)
         xbee_close(xd);
      xbee_free_xd(xd);
      xd=NULL;
   }
   
   return -1;
}
