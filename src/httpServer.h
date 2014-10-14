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

struct httpServerData_s
{
   char **params_list;
};

int start_httpServer(int my_id, void *data);
int stop_httpServer(int my_id, void *data);
int gethttp(char *server, int port, char *url, char *response, int l_response);
#endif
