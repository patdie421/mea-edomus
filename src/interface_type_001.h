/*
 *  interface_type_001.h
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_001_h
#define __interface_type_001_h

#include <sqlite3.h>
#include <inttypes.h>

#include "error.h"
#include "comio.h"
#include "dbServer.h"
#include "xPLServer.h"

#include "interface_type_001.h"

typedef struct interface_type_001_s
{
   int id_interface;
   pthread_t  *thread; // thread id
   comio_ad_t *ad; // comio descriptor
   pthread_mutex_t operation_lock;
   queue_t *counters_list; // counter sensors attach to interface
   queue_t *actuators_list;
   queue_t *sensors_list;
   xpl_f xPL_callback;
} interface_type_001_t;

typedef float (*compute_f)(unsigned int value);

void counters_stop(pthread_t *counters_thread, comio_ad_t *ad, int signal_number);

mea_error_t stop_interface_type_001(interface_type_001_t *i001);
mea_error_t start_interface_type_001(interface_type_001_t *itd, sqlite3 *db, int id_interface, const unsigned char *dev, tomysqldb_md_t *md);
mea_error_t restart_interface_type_001(interface_type_001_t *i001,sqlite3 *db, tomysqldb_md_t *md);

int16_t check_status_interface_type_001(interface_type_001_t *i001);

#endif