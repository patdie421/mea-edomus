//
//  nodejsServer.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/11/14.
//
//
#ifndef __nodejsServer_h
#define __nodejsServer_h

struct nodejsServerData_s
{
   char **params_list;
};

int get_nodejsServer_socketio_port();
nodejsServer_send_cmnd(char *hostname, int port, char cmnd, char *str);
int stop_nodejsServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif

