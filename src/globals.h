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
#define SQLITE3_DB_BUFF_NAME  10
#define MYSQL_DB_SERVER       11
#define MYSQL_DB_PORT         12
#define MYSQL_DATABASE        13
#define MYSQL_USER            14
#define MYSQL_PASSWD          15
#define VENDOR_ID             16
#define DEVICE_ID             17
#define INSTANCE_ID           18
#define MAX_LIST_SIZE         19

//extern tomysqldb_md_t md;
extern tomysqldb_md_t *myd;
extern char *plugin_path;

#endif