//
//  automatorServer.h
//
//  Created by Patrice Dietsch on 07/12/15.
//
//

#ifndef __automatorServer_h
#define __automatorServer_h

#include <pthread.h>

#include "xPL.h"

extern char* automator_server_name_str;

struct automatorServer_start_stop_params_s
{
   char **params_list;
//   sqlite3 *sqlite3_param_db;
};


typedef struct automator_msg_s
{
   xPL_MessagePtr msg;
} automator_msg_t;


typedef struct automator_queue_elem_s
{
} automator_queue_elem_t;


mea_error_t automatorServer_add_msg(xPL_MessagePtr msg);
void        setAutomatorRulesFile(char *file);
int         start_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg);
int         stop_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg);
int         restart_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
