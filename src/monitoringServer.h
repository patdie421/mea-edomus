//
//  monitoringServer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 25/09/2014.
//
//

#ifndef __monitoringServer_h
#define __monitoringServer_h

#include "timer.h"

struct process_indicator_s
{
   char name[41];
   long value;
};

typedef int (*process_start_stop_f)(int, void *);

struct monitored_process_s
{
   char name[41];
   time_t last_heartbeat;
   int heartbeat_interval; // second
   queue_t *indicators_list;
   int enable_autorestart;
   int status; // started=1, stopped=0, not_managed=2
   process_start_stop_f stop;
   process_start_stop_f start;
   void *start_stop_data;
};


struct monitored_processes_s
{
   int max_processes;
   struct monitored_process_s **processes_table;
   mea_timer_t timer;
   pthread_mutex_t lock;
} monitored_processes;


//pthread_t *start_monitoringServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *log_path);
pthread_t *start_monitoringServer(char **parms_list);

void stop_monitoringServer();

struct monitored_processes_s *get_monitored_processes_descriptor();

int init_monitored_processes_list(int max_nb_processes);
int clear_monitored_processes_list();

int process_register(char *name);
int process_unregister(int id);
int process_add_indicator(int id, char *name, long initial_value);
int process_update_indicator(int id, char *name, long value);
int process_heartbeat(int id);
int process_set_start_stop(int id,  process_start_stop_f start, process_start_stop_f stop, void *start_stop_data, int auto_restart);
int process_start(int id);
int process_stop(int id);
int process_set_not_managed(int id);
int monitoringServer_loop(char *hostname, int port);
void* process_getDataPtr(int id);

#endif
