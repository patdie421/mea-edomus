/*
 *  globals.h
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __globals_h
#define __globals_h

#include <sqlite3.h>

#include "dbServer.h"
#include "queue.h"

#define __MEA_EDOMUS_VERSION__ "0.1aplha2-ondev"

#define MEA_PATH               1
#define SQLITE3_DB_PARAM_PATH  2
#define CONFIG_FILE            3
#define PHPCGI_PATH            4
#define PHPINI_PATH            5
#define GUI_PATH               6
#define LOG_PATH               7
#define PLUGINS_PATH           8
#define SQLITE3_DB_BUFF_PATH   9
#define MYSQL_DB_SERVER       10
#define MYSQL_DB_PORT         11
#define MYSQL_DATABASE        12
#define MYSQL_USER            13
#define MYSQL_PASSWD          14
#define VENDOR_ID             15
#define DEVICE_ID             16
#define INSTANCE_ID           17
#define VERBOSELEVEL          18
#define GUIPORT               19
#define PHPSESSIONSPATH       20
#define MAX_LIST_SIZE         21

extern tomysqldb_md_t *myd;
tomysqldb_md_t *get_myd();
queue_t * get_interfaces();


// voir ou mettre
#define UNIT_WH 1 // Watt/Heure
#define UNIT_W  2 // Watt
#define UNIT_C  3 // degré C
#define UNIT_V  4 // Volt
#define UNIT_H  5 // pourcentage humidité

#endif