//
//  interface_type_003.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#ifndef __interface_type_003_h
#define __interface_type_003_h

#include <Python.h>
#include <sqlite3.h>

#include "enocean.h"
#include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"


typedef struct enocean_data_queue_elem_s
{
   uint8_t  *data;
   uint16_t l_data;
   uint32_t enocean_addr;
   struct timeval tv;
} enocean_data_queue_elem_t;


struct interface_type_003_indicators_s
{
   uint32_t senttoplugin;
   uint32_t xplin;
   uint32_t enoceandatain;
};

extern char *interface_type_003_senttoplugin_str;
extern char *interface_type_003_xplin_str;
extern char *interface_type_003_enoceandatain_str;

typedef struct interface_type_003_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];
   int              monitoring_id;
   enocean_ed_t    *ed;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
//   xpl_f            xPL_callback;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;
   char            *parameters;
   
   struct interface_type_003_indicators_s indicators;
} interface_type_003_t;


struct interface_type_003_data_s
{
   interface_type_003_t *i003;
   sqlite3 *sqlite3_param_db;
};


#define PLUGIN_DATA_MAX_SIZE 80


int start_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_003(interface_type_003_t *i003);

xpl2_f get_xPLCallback_interface_type_003(void *ixxx);
int get_monitoring_id_interface_type_003(void *ixxx);
int set_xPLCallback_interface_type_003(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_003(void *ixxx, int id);

interface_type_003_t *malloc_and_init_interface_type_003(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description);
int clean_interface_type_003(interface_type_003_t *i003);

#endif
