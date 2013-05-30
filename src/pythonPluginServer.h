//
//  pythonPlugin.h
//
//  Created by Patrice Dietsch on 04/05/13.
//
//

#ifndef pythonPluginServer_h
#define pythonPluginServer_h

#include "queue.h"

typedef enum {XBEEDATA=1, XPLMSG=2} pythonPlugin_type;

typedef struct pythonPlugin_cmd_s
{
//   pythonPlugin_type type;
   char *python_module;
//   unsigned long id_sensor;
   char *parameters;
   char *data;
   int  l_data;
} pythonPlugin_cmd_t;

int pythonPluginServer(queue_t *plugin_queue);
// int pythonPluginServer_add_cmd(pythonPlugin_type type, unsigned long id_sensor, char *module, char *parameters, void *data, int l_data);
int pythonPluginServer_add_cmd(char *module, char *module_parameters, void *data, int l_data);

void setPythonPluginPath(char *path);

#endif
