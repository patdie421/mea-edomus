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

#include "mea_queue.h"
#include "dbServer.h"
#include "xPL.h"

#define INTERFACE_TYPE_001 100
#define INTERFACE_TYPE_002 200
#define INTERFACE_TYPE_003 300
#define INTERFACE_TYPE_004 400
#define INTERFACE_TYPE_005 455

extern char *sql_select_device_info;
extern char *sql_select_interface_info;

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


int16_t      get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed);
int32_t      get_speed_from_speed_t(speed_t speed);
mea_queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db);
void         stop_interfaces();
void         dispatchXPLMessageToInterfaces(xPL_ServicePtr theService, xPL_MessagePtr theMessage);
int          restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
