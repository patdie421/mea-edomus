//
//  pythonPlugin.h
//
//  Created by Patrice Dietsch on 04/05/13.
//
//

#ifndef pythonPluginServer_h
#define pythonPluginServer_h

#include <pthread.h>

#include "queue.h"

// extern pthread_mutex_t gil_lock;

// #define GIL_LOCK pthread_mutex_lock(&gil_lock);
// #define GIL_UNLOCK pthread_mutex_unlock(&gil_lock);

typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3} pythonPlugin_type;

typedef struct pythonPlugin_cmd_s
{
//   pythonPlugin_type type;
   char *python_module;
//   unsigned long id_sensor;
//   char *parameters;
   char *data;
   int  l_data;
} pythonPlugin_cmd_t;

int pythonPluginServer(queue_t *plugin_queue);
// int pythonPluginServer_add_cmd(pythonPlugin_type type, unsigned long id_sensor, char *module, char *parameters, void *data, int l_data);
int pythonPluginServer_add_cmd(char *module, char *module_parameters, void *data, int l_data);

void setPythonPluginPath(char *path);

#endif
