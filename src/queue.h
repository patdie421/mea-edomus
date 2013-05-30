/*
 *  queue.h
 *
 *  Created by Patrice Dietsch on 28/07/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __queue_h
#define __queue_h

typedef void (*free_data_f)(void *);

struct queue_elem
{
   void *d;
   struct queue_elem *next;
   struct queue_elem *prev;
};

typedef struct
{
   struct queue_elem *first;
   struct queue_elem *last;
   struct queue_elem *current;
   unsigned long nb_elem;
} queue_t;


int init_queue(queue_t *queue);
int out_queue_elem(queue_t *queue, void **data);
int in_queue_elem(queue_t *queue, void *data);
int process_all_elem_queue(queue_t *queue, void (*f)(void *));
int first_queue(queue_t *queue);
int last_queue(queue_t *queue);
int next_queue(queue_t *queue);
int prev_queue(queue_t *queue);
int current_queue(queue_t *queue, void **data);
int clear_queue(queue_t *queue,free_data_f f);
int remove_current_queue(queue_t *queue);


#endif
