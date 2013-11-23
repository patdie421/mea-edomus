//
//  pythonPlugin.h
//
//  Created by Patrice Dietsch on 04/05/13.
//
//

#ifndef pythonPluginServer_h
#define pythonPluginServer_h

#include <pthread.h>

#include "error.h"
#include "queue.h"


typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3} pythonPlugin_type;

typedef struct pythonPlugin_cmd_s
{
   char *python_module;
   char *data;
   int  l_data;
} pythonPlugin_cmd_t;

pthread_t *pythonPluginServer(queue_t *plugin_queue);
mea_error_t pythonPluginServer_add_cmd(char *module, void *data, int l_data);
void setPythonPluginPath(char *path);

#endif
