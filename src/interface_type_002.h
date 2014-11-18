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

struct interface_type_002_indicators_s
{
   uint32_t senttoplugin;
   uint32_t xplin;
   uint32_t xbeedatain;
   uint32_t commissionning_request;
};

extern char *interface_type_002_senttoplugin_str;
extern char *interface_type_002_xplin_str;
extern char *interface_type_002_xbeedatain_str;
extern char *interface_type_002_commissionning_request_str;

typedef struct interface_type_002_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];
   int              monitoring_id;
   xbee_xd_t       *xd;
   xbee_host_t     *local_xbee;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
   xpl_f            xPL_callback;
   void            *xPL_callback_data;
   char            *parameters;
   
   struct interface_type_002_indicators_s indicators;
} interface_type_002_t;


struct interface_type_002_data_s
{
   interface_type_002_t *i002;
   sqlite3 *sqlite3_param_db;
//   int16_t id_interface;
//   tomysqldb_md_t *myd;
//   char *parameters;
};  


#define PLUGIN_DATA_MAX_SIZE 80


typedef struct plugin_commissionning_queue_elem_s
{
   PyObject      *parameters;
} plugin_commissionning_queue_elem_t;


int start_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg);
//int restart_interface_type_002(int id);
int restart_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_002(interface_type_002_t *it002);

#endif
