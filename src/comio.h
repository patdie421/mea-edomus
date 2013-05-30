//
//  comio.h
//
//  Created by Patrice DIETSCH on 16/07/12.
//  Copyright (c) 2012 -. All rights reserved.
//
#ifndef __comio_h
#define __comio_h

#include <pthread.h>
#include <semaphore.h>

#include "debug.h"
#include "queue.h"

#define MAX_TRAP              40

#define OP_ECRITURE            0
#define OP_LECTURE             1
#define OP_FONCTION            2

#define TYPE_NUMERIQUE         0
#define TYPE_ANALOGIQUE        1
#define TYPE_NUMERIQUE_PWM     2
#define TYPE_MEMOIRE           3
#define TYPE_FONCTION          4

#define COMIO_ERR_NOERR        0
#define COMIO_ERR_ENDTRAME     1
#define COMIO_ERR_STARTTRAME   2
#define COMIO_ERR_TIMEOUT      3
#define COMIO_ERR_SELECT       4
#define COMIO_ERR_READ         5
#define COMIO_ERR_DISCORDANCE  6
#define COMIO_ERR_SYS          7
#define COMIO_ERR_BADTRAPNUM   8
#define COMIO_ERR_UNKNOWN1    98
#define COMIO_ERR_UNKNOWN     99


#define COMIO_DEBUG            1
#define COMIO_DEBIT_PORT   B9600
// #define COMIO_DEBIT_PORT   B57600
typedef struct comio_elem_s
{
   unsigned char ret_op;
   unsigned char ret_var;
   unsigned char ret_type;
   unsigned int  ret_val;
   int           comio_err;
} comio_queue_elem_t;


typedef int (*trap_f)(int,void *, char *);


struct comio_trap_def_s
{
   trap_f trap;
   void  *args;
};

typedef struct comio_ad_s
{
   int             fd;
   pthread_t       read_thread;
   pthread_cond_t  sync_cond;
   pthread_mutex_t sync_lock;
   char		       serial_dev_name[255];
   queue_t        *queue;
   struct comio_trap_def_s
                   tabTrap[MAX_TRAP];
   int             signal_flag;
   
} comio_ad_t;


int  comio_open(comio_ad_t *ad,char *dev);
int  comio_init(comio_ad_t *ad, char *dev);
int  comio_operation(comio_ad_t *ad, unsigned char op, unsigned char var, unsigned char type, unsigned int val, int *comio_err);
void comio_close(comio_ad_t *ad);

int  comio_set_trap(comio_ad_t *ad, int numTrap, trap_f trap);
int  comio_set_trap2(comio_ad_t *ad, int numTrap, trap_f trap, void *args);
int  comio_remove_trap(comio_ad_t *ad, int numTrap);

int  comio_call(comio_ad_t *ad, unsigned char num_function, unsigned int val, int *comio_err);


#endif