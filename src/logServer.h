//
//  logServer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//

#ifndef mea_edomus_logServer_h
#define mea_edomus_logServer_h

struct logServerData_s
{
   char **params_list;
};

int start_logServer(int my_id, void *data);
int stop_logServer(int my_id, void *data);

#endif
