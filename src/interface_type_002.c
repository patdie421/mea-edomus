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
#include "tomysqldb.h"
#include "parameters_mgr.h"
#include "mea_api.h"
#include "pythonPluginServer.h"

typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_xbee_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define XBEE_PLUGIN_PARAMS_PLUGIN      0
#define XBEE_PLUGIN_PARAMS_PARAMETERS  1

char *sql_select_device_info="SELECT sensors_actuators.id_sensor_acutator, sensors_actuators.id_location, sensors_actuators.state, sensors_actuators.parameters, types.parameters, sensors_actuators.id_type, sensors_actuators.name, interfaces.name, interfaces.id_type, (SELECT types.name FROM types WHERE types.id_type = interfaces.id_type), interfaces.dev FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type";


struct callback_data_s // donnee "userdata" pour les callbacks
{
   pthread_mutex_t callback_lock;
   pthread_cond_t  callback_cond;
   xbee_xd_t      *xd;
   queue_t        *queue;
};


struct thread_params_s
{
   xbee_xd_t            *xd;
   tomysqldb_md_t       *md;
   sqlite3              *param_db;
   void *               *data;
   struct callback_data_s
                        *callback_xbeedata;
};


int display_frame(int ret, unsigned char *resp, uint16_t l_resp)
{
   DEBUG_SECTION {
      if(!ret)
      {
         for(int i=0;i<l_resp;i++)
            fprintf(stderr,"%s (display_frame) : %02x-[%c](%03d)\n",DEBUG_STR, resp[i],resp[i],resp[i]);
         printf("\n");
      }
   }
   return 0;
}


void display_addr(char *a)
{
   for(int i=0;i<4;i++)
      fprintf(stderr,"%s (display_addr) : %02x",DEBUG_STR, a[i]);
}


void _commissionning_free_queue_elem(void *d)
{
   commissionning_queue_elem_t *e=(commissionning_queue_elem_t *)d;
   
   free(e);
   e=NULL;
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


int add_64bits_to_at_uchar_cmd_from_char_array(unsigned char *at_cmd, char *add_64bits_addr_array)
{
   int i=0;
   
   while(add_64bits_addr_array[i]==0 && i<4)
      i++;
   if(i>3)
   {
      at_cmd[2]=0;
      at_cmd[3]=-1;
      return 3;
   }
   
   int j=2;
   for(;i<4;i++)
   {
      at_cmd[j]=add_64bits_addr_array[i];
      j++;
   }
   at_cmd[j]=-1;
   
   return j;
}


int at_set_reg_upto_64bits_from_char_array(xbee_xd_t *xd, xbee_host_t *host, char at_cmd[2], char reg_val[4], int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   uint16_t	l_at;
   int16_t ret;

   struct xbee_remote_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];
   
   l_at=add_64bits_to_at_uchar_cmd_from_char_array(at, reg_val);
   ret=xbee_atCmdToXbee(xd, host, at, l_at, resp, l_resp, nerr);

   if(ret)
      return -1;
   map_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   return (int)map_resp->cmd_status;
}


int at_set_16bits_reg_from_int(xbee_xd_t *xd, xbee_host_t *host, char at_cmd[2], int reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   uint16_t l_at;
   int16_t ret;
   
   struct xbee_remote_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];
   
   l_at=add_16bits_to_at_uchar_cmd_from_int(at, reg_val);
   ret=xbee_atCmdToXbee(xd, host, at, l_at, resp, &l_resp, nerr);

   if(ret)
      return -1;
   map_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   return (int)map_resp->cmd_status;
}


int at_get_char_array_reg(xbee_xd_t *xd, xbee_host_t *host, char at_cmd[2], char *reg_val, uint16_t *l_reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   int l_at;
   int ret;

   struct xbee_remote_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];l_at=2;
   ret=xbee_atCmdToXbee(xd, host, at, l_at, resp, &l_resp, nerr);

   if(ret<0)
      return -1;
   map_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   if(l_reg_val)
      *l_reg_val=l_resp-15;
   
   if(reg_val)
   {
      for(int i=0;i<(l_resp-15);i++)
         reg_val[i]=map_resp->at_cmd_data[i];
   }
   
   return (int)map_resp->cmd_status;
}


int at_get_local_char_array_reg(xbee_xd_t *xd, unsigned char at_cmd[2], char *reg_val, uint16_t *l_reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   uint16_t	l_at;
   int16_t ret;
   
   struct xbee_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];l_at=2;
   
   int16_t err;

   ret=xbee_atCmdToXbee(xd,NULL,at,l_at,resp,&l_resp, &err);
   if(ret<0)
      return -1;

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


int at_ac(xbee_xd_t *xd, xbee_host_t *host, int16_t *nerr)
{
   return at_get_char_array_reg(xd, host, "AC", NULL, NULL, nerr);
}


int set_io_sample_delay(xbee_xd_t *xd, xbee_host_t *host, xbee_host_t *dest, int io_sample_delay_ms)
{
   int16_t nerr;
   
   if(at_set_reg_upto_64bits_from_char_array(xd, host, "DH", dest->addr_64_h, &nerr))
      return -1;
   if(at_set_reg_upto_64bits_from_char_array(xd, host, "DL", dest->addr_64_l, &nerr))
      return -1;
   if(at_set_16bits_reg_from_int(xd, host, "IR", io_sample_delay_ms, &nerr))
      return -1;
   if(at_ac(xd, host, &nerr))
      return -1;
   
   return 0;
}


int set_sleep_mode(xbee_xd_t *xd, xbee_host_t *host, int sleep_delay_ms, int awake_delay_ms)
{
   int16_t nerr;
   
   awake_delay_ms=awake_delay_ms*10;
   
   if(at_set_16bits_reg_from_int(xd, host, "SM", 0x05, &nerr))
      return -1;
   if(at_set_16bits_reg_from_int(xd, host, "SP", sleep_delay_ms, &nerr))
      return -1;
   if(at_set_16bits_reg_from_int(xd, host, "SN", 0x01, &nerr))
      return -1;
   if(at_set_16bits_reg_from_int(xd, host, "SO", 0x00, &nerr))
      return -1;
   if(at_set_16bits_reg_from_int(xd, host, "ST", awake_delay_ms, &nerr))
      return -1;
   if(at_ac(xd, host, &nerr))
      return -1;
   
   return 0;
}


int set_ios(xbee_xd_t *xd, xbee_host_t *host,
            int disabled_mask,
            int default_function_mask,
            int digital_out_on_mask,
            int digital_out_off_mask,
            int digital_in_mask,
            int analog_in_mask)
{
   int op=0;
   int16_t ret, nerr;
   
   for(int i=0;i<XBEE_NB_ID_PIN;i++)
   {
      op=-1;
      
      if(disabled_mask & 0x01)
         op=0x00;
      disabled_mask=disabled_mask>>1;
      
      if(default_function_mask & 0x01)
         op=0x01;
      default_function_mask=default_function_mask >>1;
      
      if(digital_out_on_mask & 0x01)
         op=0x05;
      digital_out_on_mask=digital_out_on_mask>>1;
      
      if(digital_out_off_mask & 0x01)
         op=0x04;
      digital_out_off_mask=digital_out_off_mask>>1;
      
      if(analog_in_mask & 0x01)
         op=0x02;
      analog_in_mask=analog_in_mask>>1;
      
      if(op>-1)
      {
         ret=at_set_16bits_reg_from_int(xd, host, xbee_at_io_id_list[i], op, &nerr);
         if(ret<0)
            return -1;
      }
   }
   ret=at_ac(xd, host, &nerr);
   if(ret<0)
      return -1;
   
   return 0;
}


int all_known_routers_setup(xbee_xd_t *xd)
{
   char *fn_name="all_known_routers_setup";
   uint16_t nb=0;
   int16_t ret;
   int16_t nerr;
   char rep[80];
   uint16_t l_rep;
   
   xbee_hosts_table_t *table;
   
   table=xd->hosts;
   rep[0]=0;
   
   if(!table)
      return -1;
   
   for(uint16_t i=0;i<table->max_hosts;i++)
   {
      if(nb<table->nb_hosts && table->hosts_table[i])
      {
         // chercher dans la base sql l'interface
         // si l'interface est declaree, on la configure
         // sinon alerte
         
         DEBUG_SECTION fprintf(stderr, "DEBUG (%s) : Try to configure XBEE (%s)(%x-%x)\n", fn_name, table->hosts_table[i]->name, table->hosts_table[i]->l_addr_64_h, table->hosts_table[i]->l_addr_64_l);
         int retry=0;
         do
         {
            ret=at_get_char_array_reg(xd, table->hosts_table[i], "SM", rep, &l_rep, &nerr);
            DEBUG_SECTION
            {
               fprintf(stderr, "at_get_char_array_reg return %d %d\n",ret,rep[0]);
               for(int i=0;i<l_rep;i++)
                  printf("%x(%c) ",rep[i],rep[i]);
               printf("\n");
            }
         }
         while(ret==-1 && 5>retry++);
         VERBOSE(1) if(retry>5) xbee_perror(nerr);
         
         if(ret>0)
         {
            // reponse avec code erreur à traiter
         }
         
         if(ret==-1 && rep[0]==0)
         {
            retry=0;
            do
            {
               // SP à 2000 (20s valeur max "choisie" (au lieu de 2900)
               ret=at_set_16bits_reg_from_int(xd, table->hosts_table[i], "SP", 2000, &nerr);
//               DEBUG_SECTION fprintf(stderr, "at_set_16bits_reg_from_int return %d\n",ret);
            }
            while(ret==-1 && 5>retry++);
            VERBOSE(1) if(retry>5) xbee_perror(nerr);
         }
         nb++;
      }
      else
         break;
   }
   return 0;
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


int interface_type_002_activate_plugin(plugin_queue_elem_t *e)
{
   parsed_parameter_t *plugin_params=NULL;
   int nb_plugin_params;
   int err;
   
   plugin_params=malloc_parsed_parameters((char *)e->device_context.device_parameters, valid_xbee_plugin_params, &nb_plugin_params, &err, 0);
   if(plugin_params)
   {
      if(plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s && plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
      {
         pythonPluginServer_add_cmd(plugin_params[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, plugin_params[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s, (void *)e, sizeof(plugin_queue_elem_t));
         
         free_parsed_parameters(plugin_params, nb_plugin_params);
         free(plugin_params);
         
         return 0;
      }
   }
   
   if(plugin_params)
   {
      free_parsed_parameters(plugin_params, nb_plugin_params);
      free(plugin_params);
      return -1;
   }
   return 0;
}


PyObject *device_context_to_pydict(plugin_queue_elem_t *e, int l_data)
{
   PyObject *data_dict;
   PyObject *value;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
   
   value = PyLong_FromLong((long)e->device_context.device_id);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_ID_ID), value);
   Py_DECREF(value);
   
   value = PyString_FromString(e->device_context.device_name);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_NAME_ID), value);
   Py_DECREF(value);
   
   value = PyLong_FromLong((long)e->device_context.device_type_id);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_TYPE_ID_ID), value);
   Py_DECREF(value);
   
   value = PyLong_FromLong((long)e->device_context.device_location_id);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_LOCATION_ID_ID), value);
   Py_DECREF(value);
   
   value = PyString_FromString(e->device_context.device_interface_name);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_INTERFACE_NAME_ID), value);
   Py_DECREF(value);
   
   value = PyString_FromString(e->device_context.device_interface_type_name);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_INTERFACE_TYPE_NAME_ID), value);
   Py_DECREF(value);
   
   value = PyLong_FromLong((long)e->device_context.device_state);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_STATE_ID), value);
   Py_DECREF(value);
   
   value = PyString_FromString(e->device_context.device_type_parameters);
   PyDict_SetItemString(data_dict, get_token_by_id(DEVICE_TYPE_PARAMETERS_ID), value);
   Py_DECREF(value);

   value = PyLong_FromLong((long)e->xd);
   PyDict_SetItemString(data_dict, "ID_XBEE", value);
   Py_DECREF(value);
   
   value = PyLong_FromLong((long)e->device_context.addr_h);
   PyDict_SetItemString(data_dict, "ADDR_H", value);
   Py_DECREF(value);

   value = PyLong_FromLong((long)e->device_context.addr_l);
   PyDict_SetItemString(data_dict, "ADDR_L", value);
   Py_DECREF(value);

   switch(e->type_elem)
   {
      case XBEEDATA:
      {
         value = PyBuffer_FromMemory(e->complement.xbeedata.cmd, e->complement.xbeedata.l_cmd);
         PyDict_SetItemString(data_dict, "cmd", value);
         Py_DECREF(value);
         
         value = PyLong_FromLong(e->complement.xbeedata.l_cmd);
         PyDict_SetItemString(data_dict, "l_cmd", value);
         Py_DECREF(value);
         break;
      }
         
      case XPLMSG:
      {
         PyDict_SetItemString(data_dict, "xplmsg", e->complement.xpl.pyXplMsg);
         Py_DECREF(e->complement.xpl.pyXplMsg);
         break;
      }
   }
   return data_dict;
}


int *stmt_to_device_context(device_context_t *device_context, sqlite3_stmt * stmt)
{
   uint32_t addr_h, addr_l;

   // info sur le capteur
   device_context->device_id=sqlite3_column_int(stmt, 0);
   device_context->device_location_id=sqlite3_column_int(stmt, 1);
   device_context->device_type_id=sqlite3_column_int(stmt, 5);
   strncpy(device_context->device_name, (char *)sqlite3_column_text(stmt, 6), sizeof(device_context->device_name));
   strncpy(device_context->device_parameters, (char *)sqlite3_column_text(stmt, 3), sizeof(device_context->device_parameters));
   device_context->device_state=sqlite3_column_int(stmt, 2);
   
   strncpy(device_context->device_type_parameters, (char *)sqlite3_column_text(stmt, 4), sizeof(device_context->device_type_parameters));
   strncpy(device_context->device_interface_name, (char *)sqlite3_column_text(stmt, 7), sizeof(device_context->device_interface_name));
   strncpy(device_context->device_interface_type_name, (char *)sqlite3_column_text(stmt, 9), sizeof(device_context->device_interface_type_name));
   
   if(sscanf((char *)sqlite3_column_text(stmt, 10), "MESH://%x-%x", &addr_h, &addr_l)==2)
   {
      device_context->addr_h=addr_h;
      device_context->addr_l=addr_l;
   }
   else
   {
      device_context->addr_h=0;
      device_context->addr_l=0;
   }
   
   return 1;
}


int interface_type_002_xPL_callback(xPL_ServicePtr theService, xPL_MessagePtr xplMsg, xPL_ObjectPtr userValue)
{
   static const char *fn_name = "interface_type_002_xPL_callback";
   xPL_NameValueListPtr xplBody;
   char *device;
   int ret;
   
   interface_type_002_t *interface=(interface_type_002_t *)userValue;
   
   sqlite3 *params_db = get_sqlite3_param_db(); // temporaire en attendant une reflexion plus globale. Prototype dans globals.h

   xplBody = xPL_getMessageBody(xplMsg);
   device  = xPL_getNamedValue(xplBody, get_token_by_id(XPL_DEVICE_ID));
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : xPL Message to process : %s.%s (%s)\n", INFO_STR, fn_name, xPL_getSchemaClass(xplMsg), xPL_getSchemaType(xplMsg), device);

   char sql[1024];
   sqlite3_stmt * stmt;
   
   sprintf(sql,"%s WHERE sensors_actuators.name='%s';", sql_select_device_info, device);
   ret = sqlite3_prepare_v2(params_db, sql, strlen(sql)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, fn_name, sqlite3_errmsg (params_db));
      exit(1);
   }
   
   while(1)
   {
      int s = sqlite3_step(stmt);
      if (s == SQLITE_ROW)
      {
         plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
         if(plugin_elem)
         {
            plugin_elem->type_elem=XPLMSG;
            plugin_elem->xd=interface->xd;
            stmt_to_device_context(&(plugin_elem->device_context), stmt);
            
            PyObject *pO=xplMsgToPyDict(xplMsg);
            plugin_elem->complement.xpl.pyXplMsg=pO;
            
            interface_type_002_activate_plugin(plugin_elem);
            
            free(plugin_elem);
         }
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


int inteface_type_002_xbeedata_callback(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *h, char *l)
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
   
   pthread_mutex_lock(&callback_data->callback_lock);
   in_queue_elem(callback_data->queue, e);
   if(callback_data->queue->nb_elem==1)
      pthread_cond_broadcast(&callback_data->callback_cond);
   pthread_mutex_unlock(&callback_data->callback_lock);
   
   return 0;
}


int commissionning_callback(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *addr_h, char *addr_l)
{
   static const char *fn_name = "commissionning_callback";
   
   struct xbee_node_identification_response_s *nd_resp;
   struct xbee_node_identification_nd_data_s *nd_data;
   commissionning_queue_elem_t *e;
   int rval=0;
   
   VERBOSE(9) printf("%s (%s) : commissionning request received.\n", INFO_STR, fn_name);
   
   struct callback_data_s *callback_data=(struct callback_data_s *)data;
   
   int ret=pthread_mutex_lock(&(callback_data->callback_lock));
   if(ret)
      return -1;
   
   nd_resp=(struct xbee_node_identification_response_s *)cmd;
   nd_data=(struct xbee_node_identification_nd_data_s *)(nd_resp->nd_data+strlen((char *)nd_resp->nd_data)+1);
   
   e=malloc(sizeof(commissionning_queue_elem_t));
   if(e)
   {
      memcpy(e->addr_64_h, nd_resp->addr_64_h, 4);
      memcpy(e->addr_64_l, nd_resp->addr_64_l, 4);
      strcpy(e->host_name, (char *)nd_resp->nd_data);
      e->node_type=nd_data->device_type;
      
      if(in_queue_elem(callback_data->queue, e)<0)
         rval=-1;
   }
   else
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %s\n", MALLOC_ERROR_STR, ERROR_STR, fn_name);
      rval=-1;
   }
   
   pthread_mutex_unlock(&(callback_data->callback_lock));
   
   VERBOSE(9) {
      if(!rval)
         printf("%s (%s) : commissionning request transmitted.\n", INFO_STR, fn_name);
      else
         printf("%s (%s) : can't transmit commissionning request.\n", INFO_STR, fn_name);
   }
   return rval;
}


void *_thread_interface_type_002_xbeedata(void *args)
{
   static const char *fn_name="_thread_interface_type_002_xbeedata";
   
   struct thread_params_s *params=(struct thread_params_s *)args;
   struct callback_data_s *callback_data=params->callback_xbeedata;
   sqlite3 *params_db=params->param_db;
   data_queue_elem_t *e;
   int ret;

   free(params);
   params=NULL;
   
   while(1)
   {
      pthread_mutex_lock(&callback_data->callback_lock);
      if(callback_data->queue->nb_elem==0)
      {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 30; // timeout de 30 secondes
         ts.tv_nsec = 0;
         
         ret=pthread_cond_timedwait(&callback_data->callback_cond, &callback_data->callback_lock, &ts);
         if(ret)
         {
            if(ret==ETIMEDOUT)
            {
               DEBUG_SECTION fprintf(stderr,"%s (%s) : Nb elements in queue after TIMEOUT : %ld)\n", DEBUG_STR, fn_name, callback_data->queue->nb_elem);
//               pthread_mutex_unlock(&callback_data->callback_lock);
//               pthread_testcancel();
//               break;
            }
            else
            {
               // autres erreurs à traiter
            }
         }
      }
      if(!out_queue_elem(callback_data->queue, (void **)&e))
      {
         sqlite3_stmt * stmt;
         char sql[1024];
         char addr[18];
               
         pthread_mutex_unlock(&callback_data->callback_lock);
         sprintf(addr,
               "%02x%02x%02x%02x-%02x%02x%02x%02x",
               e->addr_64_h[0],
               e->addr_64_h[1],
               e->addr_64_h[2],
               e->addr_64_h[3],
               e->addr_64_l[0],
               e->addr_64_l[1],
               e->addr_64_l[2],
               e->addr_64_l[3]);
         VERBOSE(9) fprintf(stderr, "%s (%s) : data from = %s received\n",DEBUG_STR, fn_name, addr);

         sprintf(sql,"%s WHERE interfaces.dev ='MESH://%s';", sql_select_device_info, addr);
         ret = sqlite3_prepare_v2(params_db,sql,strlen(sql)+1,&stmt,NULL);
         if(ret)
         {
            VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR, fn_name, sqlite3_errmsg (params_db));
            exit(1);
         }

         while(1)
         {
            int s = sqlite3_step(stmt);
            if (s == SQLITE_ROW)
            {
               plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
               if(plugin_elem)
               {
                  plugin_elem->type_elem=XBEEDATA;
                  plugin_elem->xd=callback_data->xd;
                  stmt_to_device_context(&(plugin_elem->device_context), stmt);
                  
                  // source xbee
                  memcpy(plugin_elem->complement.xbeedata.addr_64_h,e->addr_64_h,4);
                  memcpy(plugin_elem->complement.xbeedata.addr_64_l,e->addr_64_l,4);
                  
                  // données reçu
                  memcpy(plugin_elem->complement.xbeedata.cmd,e->cmd,e->l_cmd);
                  plugin_elem->complement.xbeedata.l_cmd=e->l_cmd;
                  
                  // date de reception
                  plugin_elem->complement.xbeedata.tv=e->tv;
                  
                  interface_type_002_activate_plugin(plugin_elem);
               }
               else
               {
                  // traitement de l'erreur malloc à faire
               }
               free(plugin_elem);
            }
            else if (s == SQLITE_DONE)
            {
               sqlite3_finalize(stmt);
               break;
            }
            else
            {
               sqlite3_finalize(stmt);
               break; // traitement des erreurs à affiner avant break ...
            }
         }
         
         if(e)
         {
            if(e->cmd)
               free(e->cmd);
            free(e);
            e=NULL;
         }
      }
      else
      {
         // pb d'accès aux données de la file
         VERBOSE(5) fprintf(stderr,"%s (%s) : out_queue_elem - can't access\n", ERROR_STR, fn_name);
         pthread_mutex_unlock(&callback_data->callback_lock);
      }
      pthread_testcancel();
   }
   return NULL;
}


pthread_t *start_interface_type_002_thread(xbee_xd_t *xd, sqlite3 *db, tomysqldb_md_t *md,thread_f function, void *data, struct callback_data_s *callback_xbeedata)
{
   static const char *fn_name="start_interface_type_002_thread";
   
   pthread_t *thread=NULL;
   struct thread_params_s *params=NULL;
   
   // préparation de la file de traitement "iodata", son verrou et sa condition
   pthread_mutex_init(&callback_xbeedata->callback_lock, NULL);
   pthread_cond_init(&callback_xbeedata->callback_cond, NULL);
   
   callback_xbeedata->queue=(queue_t *)malloc(sizeof(queue_t));
   if(!callback_xbeedata->queue)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, MALLOC_ERROR_STR, fn_name);
      goto clean_exit;
   }
   init_queue(callback_xbeedata->queue);
   
   // parametrage des callback
   params=malloc(sizeof(struct thread_params_s));
   if(!params)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, MALLOC_ERROR_STR, fn_name);
      goto clean_exit;
   }
   
   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : %s\n", ERROR_STR, MALLOC_ERROR_STR, fn_name);
      goto clean_exit;
   }
   
   params->md=md;
   params->param_db=db;
   params->callback_xbeedata=callback_xbeedata;
   params->xd=xd;
   params->data=data;
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto clean_exit;
   
   return thread;
   
clean_exit:
   FREE(thread);
   FREE(params);
   FREE(callback_xbeedata->queue);
   return NULL;
}


int stop_interface_type_002(interface_type_002_t *it002, int signal_number)
{
   static const char *fn_name="stop_interface_type_002";

   VERBOSE(5) fprintf(stderr,"%s  (%s) : shutdown counter thread (signal = %d).\n",INFO_STR, fn_name,signal_number);
   
   if(it002->thread_commissionning)
   {
      pthread_cancel(*(it002->thread_commissionning));
      pthread_join(*(it002->thread_commissionning), NULL);
      it002->thread_commissionning=NULL;
   }

   if(it002->thread_data)
   {
      pthread_cancel(*(it002->thread_data));
      pthread_join(*(it002->thread_data), NULL);
      it002->thread_data=NULL;
   }
   
   xbee_close(it002->xd);
   
   xbee_free_xd(it002->xd);
   FREE(it002->local_xbee);
   FREE(it002->thread_commissionning);
   FREE(it002->thread_data);

   VERBOSE(5) fprintf(stderr,"%s  (%s) : counter thread is down.\n",INFO_STR, fn_name);
   
   return 0;
}


int restart_interface_type_002(interface_type_002_t *i002,sqlite3 *db, tomysqldb_md_t *md)
{
   char full_dev[80];
   char dev[80];
   int id_interface;
   int ret;
   
   sscanf(i002->xd->serial_dev_name,"/dev/%s",full_dev);
   sprintf(dev,"SERIAL://%s",full_dev);
   
   id_interface=i002->id_interface;
   
   stop_interface_type_002(i002, 0);
   ret=start_interface_type_002(i002, db, id_interface, (const unsigned char *)dev, md);
   
   
   return ret;
}


int check_status_interface_type_002(interface_type_002_t *it002)
{
   if(it002->xd->signal_flag!=0)
      return -1;
   
   return 0;
}


int start_interface_type_002(interface_type_002_t *i002, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md)
{
   static const char *fn_name="start_interface_type_002";

   char unix_dev[80];
   char buff[80];
   int fd;
   int16_t nerr;
   int ret;

   xbee_xd_t *xd=NULL;
   xbee_host_t *local_xbee=NULL;
   
//   struct callback_data_s *callback_data_commissionning=NULL;
   struct callback_data_s *callback_xbeedata=NULL;
   
   pthread_mutex_init(&i002->operation_lock, NULL);
   
   if(sscanf((char *)dev,"SERIAL://%s",buff)==1)
      sprintf(unix_dev,"/dev/%s",buff);
   else
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : unknow interface device - %s\n", ERROR_STR, fn_name, dev);
      goto clean_exit;
   }   

   xd=(xbee_xd_t *)malloc(sizeof(xbee_xd_t));
   if(!xd)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : malloc error\n", ERROR_STR, fn_name);
      goto clean_exit;
   }
   
   fd=xbee_init(xd, unix_dev, B9600);
   if (fd == -1)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : init_xbee - Unable to open serial port (%s) : ", ERROR_STR, fn_name, unix_dev);
         perror("");
      }
      goto clean_exit;
   }
   i002->xd=xd;
   
   /*
    * Préparation du réseau XBEE
    */
   
   local_xbee=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   if(!local_xbee)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : malloc error\n", ERROR_STR, fn_name);
      goto clean_exit;
   }
   i002->local_xbee=local_xbee;


   // récupération de l'adresse de l'xbee connecter au PC (pas forcement le coordinateur).
   uint32_t addr_64_h;
   uint32_t addr_64_l;
   {
      unsigned char at_cmd[2];
      uint16_t l_reg_val;
      char addr_l[4];
      char addr_h[4];
      
      // lecture de l'adresse de l'xbee local
      at_cmd[0]='S';at_cmd[1]='H';
      do { ret=at_get_local_char_array_reg(xd, at_cmd, (char *)addr_h, &l_reg_val, &nerr); } while (ret==-1);
      at_cmd[0]='S';at_cmd[1]='L';
      do { ret=at_get_local_char_array_reg(xd, at_cmd, (char *)addr_l, &l_reg_val, &nerr); } while (ret==-1);
      addr_64_char_array_to_int(addr_h, addr_l, &addr_64_h, &addr_64_l);
   }
   VERBOSE(9) fprintf(stderr, "INFO  (%s) : local address is : %x-%x\n", fn_name, addr_64_h, addr_64_l);
   xbee_get_host_by_addr_64(xd, local_xbee, addr_64_h, addr_64_l, &nerr);
//   xbee_get_host_by_addr_64(xd, local_xbee, 0x00000000, 0x00000000, &nerr);
   
   
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
   
   /*
    * parametrage du réseau
    */
   // découverte des xbee du réseau
   xbee_start_network_discovery(xd, &nerr); // lancement de la découverte "asynchrone"
   
   // on attend que la découverte soit terminée (à remplacer par un "evenement")
   sleep(10);
   
   
   // on positionne certain parametre sur tous les routeurs/coordinateur
//   all_known_routers_setup(xd); // à revoir, voir même si c'est nécessaire
   
   /*
    * Gestion des sous-interfaces
    */
   // initialisation du thread de gestion des "entrées" xbee
   callback_xbeedata=(struct callback_data_s *)malloc(sizeof(struct callback_data_s));
   if(!callback_xbeedata)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : malloc error\n", ERROR_STR, fn_name);
      goto clean_exit;
   }
   callback_xbeedata->queue=NULL;
   callback_xbeedata->xd=xd;
   i002->thread_data=start_interface_type_002_thread(xd,db,md,(thread_f)_thread_interface_type_002_xbeedata,(void *)i002,callback_xbeedata);

   xbee_set_iodata_callback2(xd, inteface_type_002_xbeedata_callback, (void *)callback_xbeedata);
   xbee_set_dataflow_callback2(xd, inteface_type_002_xbeedata_callback, (void *)callback_xbeedata);

//   xbee_set_commissionning_callback2(xd, commissionning_callback, (void *)callback_data_commissionning);

   i002->xPL_callback=interface_type_002_xPL_callback;

   return 0;

clean_exit:
   // vider la queue de callback_data_iodata
   if(callback_xbeedata)
   {
      if(callback_xbeedata->queue && callback_xbeedata->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
         clear_queue(callback_xbeedata->queue,_iodata_free_queue_elem);
      free(callback_xbeedata);
   }

   FREE(local_xbee);
   xbee_free_xd(xd);
   
   return -1;
}

