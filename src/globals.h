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

#include "tomysqldb.h"

#define __MEA_EDOMUS_VERSION__ "0.1aplha2"

extern char *mysql_db_server;
extern char *mysql_database;
extern char *mysql_user;
extern char *mysql_passwd;

//extern tomysqldb_md_t md;
extern tomysqldb_md_t *myd;
extern char *plugin_path;

#endif