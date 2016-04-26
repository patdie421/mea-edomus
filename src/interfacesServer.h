//
//  interfaces.h
//
//  Created by Patrice DIETSCH on 13/09/12.
//
//

#ifndef __interfacesServer_h
#define __interfacesServer_h

#include <termios.h>
#include <inttypes.h>
#include <sqlite3.h>

#include "xPLServer.h"

#include "mea_queue.h"
#include "dbServer.h"
#include "cJSON.h"

#define INTERFACE_TYPE_001 100
#define INTERFACE_TYPE_002 200
#define INTERFACE_TYPE_003 300
#define INTERFACE_TYPE_004 400
#define INTERFACE_TYPE_005 455
#define INTERFACE_TYPE_006 465

extern char *sql_select_device_info;
extern char *sql_select_interface_info;

typedef void * (*malloc_and_init_interface_f)(sqlite3 *, int, char *, char *, char *, char *);
typedef int    (*get_monitoring_id_f)(void *);
typedef int    (*set_monitoring_id_f)(void *, int);
typedef xpl2_f (*get_xPLCallback_f)(void *);
typedef int    (*set_xPLCallback_f)(void *, xpl2_f);
typedef int    (*clean_f)(void *);
typedef int    (*get_type_f)();

struct interfacesServer_interfaceFns_s {
   void *lib;
   malloc_and_init_interface_f malloc_and_init_interface;
   get_monitoring_id_f get_monitoring_id;
   set_monitoring_id_f set_monitoring_id;
   get_xPLCallback_f get_xPLCallback;
   set_xPLCallback_f set_xPLCallback;
   get_type_f get_type;
   clean_f clean;
};

#ifdef ASPLUGIN
typedef int (*get_fns_interface_f)(void *, struct interfacesServer_interfaceFns_s *);
#else
typedef int (*get_fns_interface_f)(struct interfacesServer_interfaceFns_s *);
#endif

typedef struct interfaces_queue_elem_s
{
   int type;
   void *context;
} interfaces_queue_elem_t;


struct interfacesServerData_s
{
   char **params_list;
   sqlite3 *sqlite3_param_db;
//   dbServer_md_t *myd;
};  

//typedef int (*get_interface_fns_f)(struct interfacesServer_interfaceFns_s *);

//int16_t      get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed);
//int32_t      get_speed_from_speed_t(speed_t speed);
mea_queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db);
void         stop_interfaces();
//void         dispatchXPLMessageToInterfaces(xPL_ServicePtr theService, xPL_MessagePtr theMessage);
void         dispatchXPLMessageToInterfaces2(cJSON *xplMsgJson);
int          restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
