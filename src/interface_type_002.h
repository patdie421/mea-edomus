/*
 *
 *  Created by Patrice Dietsch on 24/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_002_h
#define __interface_type_002_h

#include <Python.h>
#include <sqlite3.h>

#include "xbee.h"
#include "tomysqldb.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"


typedef struct commissionning_queue_elem_s
{
   unsigned char addr_64_h[4];
   unsigned char addr_64_l[4];
   char          host_name[21];
   unsigned char node_type;
} commissionning_queue_elem_t;


typedef struct data_queue_elem_s
{
   unsigned char  addr_64_h[4];
   unsigned char  addr_64_l[4];
   unsigned char *cmd;
   int            l_cmd;
   struct timeval tv;
} data_queue_elem_t;

typedef struct interface_type_002_s
{
   int              id_interface;
   xbee_xd_t       *xd;
   xbee_host_t     *local_xbee;
   pthread_t       *thread_commissionning; // thread id
   pthread_t       *thread_data;
   pthread_mutex_t  operation_lock;
   xpl_f            xPL_callback;
} interface_type_002_t;

#define PLUGIN_DATA_MAX_SIZE 80

typedef struct device_context_s
{
   /*
    -          « device_id » : OK
    -          « device_name » : OK
    -          « device_parameters » : OK
    -          « device_type_id » : OK
    -          « device_type_name » :
    -          « device_type_parameters » : OK
    -          « device_interface_id » : OK
    -          « device_interface_name » : OK
    -          « device_interface_type_id » :
    -          « device_interface_type_name » : OK
    -          « device_location_id » :
    -          « device_location_name » :
    -          « xpl-vendor-id » :
    -          « xpl-device-id » :
    -          « xpl-instance-id » :
    */
   int            device_id;
   char           device_name[PLUGIN_DATA_MAX_SIZE];
   char           device_parameters[PLUGIN_DATA_MAX_SIZE];
   int            device_type_id;
   char           device_type_name[PLUGIN_DATA_MAX_SIZE];
   char           device_type_parameters[PLUGIN_DATA_MAX_SIZE];
   int            device_interface_id;
   char           device_interface_name[PLUGIN_DATA_MAX_SIZE];
   char           device_interface_type_name[PLUGIN_DATA_MAX_SIZE];
   int            device_location_id;
   int            device_state;
   uint32_t       addr_h,addr_l;
} device_context_t;


typedef struct plugin_xbeedata_queue_elem_s
{
   unsigned char    addr_64_h[4];
   unsigned char    addr_64_l[4];
   unsigned char    cmd[80];
   int              l_cmd;
   struct timeval   tv;
   
} plugin_xbeedata_queue_elem_t;


typedef struct plugin_xpl_queue_elem_s
{
   PyObject      *pyXplMsg;
} plugin_xpl_queue_elem_t;


typedef struct plugin_queue_elem_s
{
   pythonPlugin_type type_elem;
   device_context_t  device_context;
   xbee_xd_t        *xd;

   union
   {
      plugin_xpl_queue_elem_t      xpl;
      plugin_xbeedata_queue_elem_t xbeedata;
   } complement;
   
} plugin_queue_elem_t;


int interface_type_002_activate_plugin(plugin_queue_elem_t *e);

void temperatures_init(xbee_xd_t *xd);
void temperatures_reset(xbee_xd_t *xd);
pthread_t *temperatures(xbee_xd_t *xd, char *dev, tomysqldb_md_t *md);

int start_interface_type_002(interface_type_002_t *it002, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md);
int stop_interface_type_002(interface_type_002_t *it002, int signal_number);
int restart_interface_type_002(interface_type_002_t *i002,sqlite3 *db, tomysqldb_md_t *md);
int check_status_interface_type_002(interface_type_002_t *it002);
PyObject *device_context_to_pydict(plugin_queue_elem_t *e, int l_data);


#endif