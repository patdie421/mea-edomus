//
//  monitoringServer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 25/09/2014.
//
//

#ifndef __processManager_h
#define __processManager_h

#include "timer.h"

struct process_indicator_s
{
   char name[41];
   long value;
};

typedef int (*process_f)(int, void *, char *, int);
typedef enum process_status_e { STOPPED = 0,   RUNNING = 1, RECOVERY_ERROR } process_status_t;
typedef enum process_type_e   { AUTOSTART = 0, NOTMANAGED = 1, TASK = 2 } process_type_t;

#define DEFAULTGROUP 0

struct managed_processes_process_s
{
   char name[41];

   time_t last_heartbeat;
   int heartbeat_interval; // en second
   int heartheat_status; // (0 = KO, 1 = OK)
   int heartbeat_counter;

   queue_t *indicators_list;
   
   int group_id;
   int type; // 0 = , 1 autorestart, 2 oneshot (start mais pas de stop, ...)
   int status; // running=1, stopped=0, not_managed=2
   
   managed_processes_process_f stop;
   managed_processes_process_f start;
   void *start_stop_data;
   
   managed_processes_process_f heartbeat_recovery;
   void *recovery_data;
};


struct managed_processes_s
{
   int max_processes;
   struct process_s **processes_table;
   
   mea_timer_t timer;
//   pthread_mutex_t lock;
   pthread_rwlock_t rwlock;
} managed_processes;


//pthread_t *start_monitoringServer(char **parms_list);


struct managed_processes_s *get_managed_processes_descriptor();

int   process_register(char *name);
int   process_unregister(int id);
int   process_add_indicator(int id, char *name, long initial_value);
int   process_update_indicator(int id, char *name, long value);
int   process_heartbeat(int id);
int   process_start(int id, char *errmsg, int l_errmsg);
int   process_stop(int id, char *errmsg, int l_errmsg);
int   process_is_running(int id);
int   process_run_task(int id, char *errmsg, int l_errmsg);

int   process_set_start_stop(int id,  managed_processes_process_f start, managed_processes_process_f stop, void *start_stop_data, int type);
int   process_set_heartbeat_recovery(id, managed_processes_process_f recovery_task, void *recovery_data);
int   process_set_status(int id, process_status_t status);
int   process_set_type(int id, process_type_t type);
int   process_set_group(int id, int group_id);
void* process_get_data_ptr(int id);

int   init_processes_manager(int max_nb_processes);
int   clean_managed_processes();

int   managed_processes_check_heartbeat(int doRecovery);
int   managed_processes_loop(char *hostname, int port);

int   managed_processes_send_stats_now(char *hostname, int port);

#endif
