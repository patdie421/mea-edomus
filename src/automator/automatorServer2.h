//
//  automatorServer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 24/11/2013.
//
//
#ifndef __automatorServer_h
#define __automatorServer_h

#include <inttypes.h>
#include <sys/time.h>

struct acquireData_s
{
};

struct acquire_start_stop_param_s
{
};

struct automator_start_stop_param_s
{
};

int start_acquire(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_acquire(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_acquire(int my_id, void *data, char *errmsg, int l_errmsg);

int start_automator(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_automator(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_automator(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
