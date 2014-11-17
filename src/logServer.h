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

extern char *log_server_name_str;
extern char *log_server_logsenterr_str;
extern char *log_server_readerror_str;
extern char *log_server_logsenterr_str;

int start_logServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_logServer(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_logServer(int my_id, void *data, char *errmsg, int l_errmsg);

void mea_livelog_enable();
void mea_livelog_disable();

#endif
