/*
 *  interface_type_005.h
 *
 *  Created by Patrice Dietsch on 27/02/15.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_005_h
#define __interface_type_005_h

#include <inttypes.h>
#include <pthread.h>
#include <sqlite3.h>
#include <signal.h>
#include "uthash.h"
#include "cJSON.h"
#include "xPLServer.h"

#include "mea_queue.h"
#include "netatmo.h"

extern char *interface_type_005_xplin_str;
extern char *interface_type_005_xplout_str;

#define I005_XPLIN  interface_type_005_xplin_str
#define I005_XPLOUT interface_type_005_xplout_str

struct interface_type_005_indicators_s
{
   uint32_t xplin;
   uint32_t xplout;
};


typedef struct interface_type_005_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];
   char             user[81];
   char             password[81];
   char            *parameters;
   xpl_f            xPL_callback;
   mea_queue_t      devices_list;
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t
                    thread_is_running;
   pthread_mutex_t  lock;
   struct interface_type_005_indicators_s
                    indicators;
   struct netatmo_token_s token;
} interface_type_005_t;


struct interface_type_005_start_stop_params_s
{
   interface_type_005_t *i005;
   sqlite3 *sqlite3_param_db;
};


struct thread_interface_type_005_args_s {
   interface_type_005_t *i005;
};

int start_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_005(interface_type_005_t *i005);
interface_type_005_t *malloc_and_init_interface_type_005(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description);
int clean_interface_type_005(interface_type_005_t *i005);


#endif
