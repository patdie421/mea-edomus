/*
 *  interface_type_004.h
 *
 *  Created by Patrice Dietsch on 27/02/15.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_004_h
#define __interface_type_004_h

#include <inttypes.h>
#include <pthread.h>
#include <sqlite3.h>

#include "uthash.h"
#include "cJSON.h"
#include "xPLServer.h"

#include "interface_type_004.h"


extern char *interface_type_004_xplin_str;
extern char *interface_type_004_xplout_str;
extern char *interface_type_004_lightschanges_str;

#define I004_XPLIN  interface_type_004_xplin_str
#define I004_XPLOUT interface_type_004_xplout_str
#define I004_LIGHSCHANGES interface_type_004_lightschanges_str

struct interface_type_004_indicators_s
{
   uint32_t xplin;
   uint32_t xplout;
   uint32_t lightschanges;
};


struct lightsListElem_s {
    const char *sensorname; /* key */
    const char *actuatorname; /* key */
    const char *huename;
    int16_t id_sensor;
    int16_t id_actuator;
    int16_t todbflag_sensor;
    int16_t todbflag_actuator;
    int16_t reachable_use;
    UT_hash_handle hh_sensorname;/* makes this structure hashable */
    UT_hash_handle hh_actuatorname;/* makes this structure hashable */
    UT_hash_handle hh_huename;/* makes this structure hashable */
};


struct groupsListElem_s {
    const char *groupname; /* key */
    const char *huegroupname;
    UT_hash_handle hh_groupname;/* makes this structure hashable */
};


struct scenesListElem_s {
    const char *scenename; /* key */
    const char *huescenename;
    UT_hash_handle hh_scenename;/* makes this structure hashable */
};


typedef struct interface_type_004_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];
   char             server[41];
   char             user[41];
   int              port;
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t
                    thread_is_running;
                    
   pthread_mutex_t  lock;
   
   struct lightsListElem_s
                   *lightsListByHueName;
   struct lightsListElem_s
                   *lightsListByActuatorName;
   struct lightsListElem_s
                   *lightsListBySensorName;
                   
   struct groupsListElem_s
                   *groupsListByGroupName;
                   
   struct scenesListElem_s
                   *scenesListBySceneName;
                   
   cJSON *lastHueLightsState;
   cJSON *currentHueLightsState;
   cJSON *allGroups;
   cJSON *allScenes;
   
   struct interface_type_004_indicators_s
                    indicators;
   int              loaded;
   xpl_f            xPL_callback;
   char            *parameters;

} interface_type_004_t;


struct interface_type_004_start_stop_params_s
{
   interface_type_004_t *i004;
   sqlite3 *sqlite3_param_db;
};


struct thread_interface_type_004_args_s {
   interface_type_004_t *i004;
};

int start_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_004(interface_type_004_t *i004);
interface_type_004_t *malloc_and_init_interface_type_004(sqlite3 *sqlite3_param_db, int id_interface, char *name, char *dev, char *parameters, char *description);
int clean_interface_type_004(interface_type_004_t *i004);


#endif
