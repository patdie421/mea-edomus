//
//  httpServer.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifndef mea_eDomus_httpServer_h
#define mea_eDomus_httpServer_h

#include "error.h"
#include "queue.h"
#include <sqlite3.h>

int start_httpServer(char **params_list, sqlite3 *sqlite3_param_db, queue_t *interfaces);
void stop_httpServer();

#endif
