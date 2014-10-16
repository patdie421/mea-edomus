//
//  pythonPlugin.h
//
//  Created by Patrice Dietsch on 04/05/13.
//
//

#ifndef __pythonPluginServer_h
#define __pythonPluginServer_h
#include <Python.h>

#include <pthread.h>
#include <sqlite3.h>

#include "error.h"
#include "queue.h"

typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3} pythonPlugin_type;


struct pythonPluginServerData_s
{
   char **params_list;
   sqlite3 *sqlite3_param_db;
};  


typedef struct pythonPlugin_cmd_s
{
   char *python_module;
   char *data;
   int  l_data;
} pythonPlugin_cmd_t;


typedef struct plugin_queue_elem_s
{
   pythonPlugin_type type_elem;
   PyObject *aDict;
   char buff[128];
   uint16_t l_buff;
} plugin_queue_elem_t;


mea_error_t pythonPluginServer_add_cmd(char *module, void *data, int l_data);
void setPythonPluginPath(char *path);
int start_pythonPluginServer(int my_id, void *data);
int stop_pythonPluginServer(int my_id, void *data);

#endif
