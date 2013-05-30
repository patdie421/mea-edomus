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

extern tomysqldb_md_t md;
extern char *plugin_path;

sqlite3 *get_sqlite3_param_db(); // temporaire en attendant une reflexion plus globale

#endif