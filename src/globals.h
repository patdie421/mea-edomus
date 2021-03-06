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
#include "mea_queue.h"

#define __MEA_EDOMUS_VERSION__ "0.1aplha6-ondev"

// base de paramétrage

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
#define PHPSESSIONS_PATH      20
#define NODEJS_PATH           21
#define NODEJSIOSOCKET_PORT   22
#define NODEJSDATA_PORT       23
#define PARAMSDBVERSION       24
#define INTERFACE             25
#define RULES_FILE            26
#define RULES_FILES_PATH      27
#define COLLECTOR_ID          28
#define DRIVERS_PATH          29
#define BACKUPDIR_PATH        30
#define MAX_LIST_SIZE         31

#define CURRENT_PARAMS_DB_VERSION 12
sqlite3 *get_sqlite3_param_db(void);

// voir ou mettre
#define UNIT_WH 1 // Watt/Heure
#define UNIT_W  2 // Watt
#define UNIT_C  3 // degré C
#define UNIT_V  4 // Volt
#define UNIT_H  5 // pourcentage humidité

#define NOPTHREADJOIN 1

// extern FILE *dbgfd;

#endif
