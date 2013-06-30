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

#include "error.h"
#include "xbee.h"
#include "dbServer.h"
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
   pthread_t       *thread;
   xpl_f            xPL_callback;
   void            *xPL_callback_data;
} interface_type_002_t;

#define PLUGIN_DATA_MAX_SIZE 80


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


typedef struct plugin_commissionning_queue_elem_s
{
   PyObject      *parameters;
} plugin_commissionning_queue_elem_t;


typedef struct plugin_queue_elem_s
{
   pythonPlugin_type type_elem;
   PyObject *aDict;
   char buff[128];
   uint16_t l_buff;
} plugin_queue_elem_t;


mea_error_t start_interface_type_002(interface_type_002_t *it002, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md);
mea_error_t stop_interface_type_002(interface_type_002_t *it002, int signal_number);
mea_error_t restart_interface_type_002(interface_type_002_t *i002,sqlite3 *db, tomysqldb_md_t *md);
mea_error_t check_status_interface_type_002(interface_type_002_t *it002);

#endif
