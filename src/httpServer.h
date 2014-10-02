//
//  httpServer.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifndef __httpServer_h
#define __httpServer_h

#include "error.h"
#include "queue.h"
#include <sqlite3.h>

int start_httpServer(char **params_list, queue_t *interfaces);
void stop_httpServer();

#endif
