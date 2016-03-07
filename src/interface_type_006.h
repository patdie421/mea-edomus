//
//  interface_type_006.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/11/2015.
//
//
#ifndef __interface_type_006_h
#define __interface_type_006_h

#include <Python.h>
#include <sqlite3.h>

#include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"


struct interface_type_006_indicators_s
{
   uint32_t senttoplugin;
   uint32_t xplin;
   uint32_t xplout;
   uint32_t serialin;
   uint32_t serialout;
};

extern char *interface_type_006_senttoplugin_str;
extern char *interface_type_006_xplin_str;
extern char *interface_type_006_serialin_str;

typedef struct interface_type_006_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];

   char             real_dev[81];
   int              real_speed;
   int              fd;

   char            *parameters;
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
//   xpl_f            xPL_callback;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;

//   PyObject /* *pName, */ *pModule, *pFunc, *pParams;
   
   struct interface_type_006_indicators_s indicators;
} interface_type_006_t;


struct interface_type_006_data_s
{
   interface_type_006_t *i006;
   sqlite3 *sqlite3_param_db;
};


int start_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_006(interface_type_006_t *i006);
interface_type_006_t *malloc_and_init_interface_type_006(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description);
int clean_interface_type_006(interface_type_006_t *i006);

#endif
