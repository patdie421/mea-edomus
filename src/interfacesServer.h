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

#include "queue.h"
#include "dbServer.h"
#include "xPL.h"

#define INTERFACE_TYPE_001 100
#define INTERFACE_TYPE_002 200

typedef struct interfaces_queue_elem_s
{
   int type;
   void *context;
} interfaces_queue_elem_t;

int16_t get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed);
int32_t get_speed_from_speed_t(speed_t speed);
queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db, tomysqldb_md_t *myd);
void restart_down_interfaces(sqlite3 *sqlite3_param_db, tomysqldb_md_t *myd);
void stop_interfaces();
void dispatchXPLMessageToInterfaces(xPL_ServicePtr theService, xPL_MessagePtr theMessage);

#endif
