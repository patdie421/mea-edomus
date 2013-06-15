/*
 *  queue.c
 *
 *  Created by Patrice Dietsch on 28/07/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>


unsigned long nb_queue_elem(queue_t *queue)
{
   if(!queue)
      return -1;

   return queue->nb_elem;
}


int in_queue_elem(queue_t *queue, void *data)
{
   struct queue_elem *new;
   
   if(!queue)
      return -1;

   new=(struct queue_elem *)malloc(sizeof(struct queue_elem));
   if(!new)
      return -1;
   
   new->d=data;
   
   if(queue->first==NULL)
   {
      queue->first=new;
      queue->last=new;
      new->next=NULL;
      new->prev=NULL;
   }
   else
   {
      new->next=queue->first;
      new->prev=NULL;
      queue->first->prev=new;
      queue->first=new;
   }
   
   queue->nb_elem++;
   
   return 0;
}


int out_queue_elem(queue_t *queue, void **data)
{
   struct queue_elem *ptr;
   
   if(!queue)
      return -1;
   
   if(queue->last)
   {
      ptr=queue->last;
      
      if(queue->last == queue->first)
      {
         queue->last=NULL;
         queue->first=NULL;
      }
      else
      {
         queue->last=queue->last->prev;
         queue->last->next=NULL;
      }
      *data=ptr->d;
      free(ptr);
      ptr=NULL;
   }
   else 
      return -1;
   
   queue->nb_elem--;
   
   return 0;
}


int init_queue(queue_t *queue)
{
   if(!queue)
      return -1;

   queue->first=NULL;
   queue->last=NULL;
   queue->current=NULL;
   queue->nb_elem=0;
   
   return 0;
}


int first_queue(queue_t *queue)
{
    if(!queue || !queue->first)
      return -1;
   
   queue->current=queue->first;
   return 0;
}


int last_queue(queue_t *queue)
{
   if(!queue->last)
      return -1;
   
   queue->current=queue->last;
   return 0;
}


int next_queue(queue_t *queue)
{
   if(!queue->current)
      return -1;
   if(!queue->current->next)
   {
      queue->current=NULL;
      return -1;
   }
   
   queue->current=queue->current->next;
   return 0;
}


int prev_queue(queue_t *queue)
{
   if(!queue->current)
      return -1;
   if(!queue->current->prev)
   {
      queue->current=NULL;
      return -1;
   }
   
   queue->current=queue->current->prev;
   
   return 0;
}


int clear_queue(queue_t *queue,free_data_f f)
{
   struct queue_elem *ptr;
   
   if(!queue)
      return -1;

   while(queue->nb_elem>0)
   {
      if(queue->last)
      {
         ptr=queue->last;
         
         if(queue->last == queue->first)
         {
            queue->last=NULL;
            queue->first=NULL;
         }
         else
         {
            queue->last=queue->last->prev;
            queue->last->next=NULL;
         }
         if(ptr->d)
         {
            if(f)
               f(ptr->d);
         }
         free(ptr);
         ptr=NULL;
      }
      else 
         return 0;
      
      queue->nb_elem--;		
   }
   return 0;
}


int current_queue(queue_t *queue, void **data)
{
   if(!queue)
      return -1;

   if(!queue->current)
      return -1;
   
   *data=queue->current->d;
   
   return 0;
}


int remove_current_queue(queue_t *queue)
{
   if(!queue)
      return -1;

    if(queue->nb_elem==0)
       return -1;
   
    if(queue->nb_elem==1)
    {
       free(queue->current);
       queue->current=NULL;
       queue->first=NULL;
       queue->last=NULL;
       queue->nb_elem=0;
       return 0;
    }
   
   struct queue_elem *prev;
   struct queue_elem *next;
   struct queue_elem *old;

   prev=queue->current->prev;
   next=queue->current->next;
   old=queue->current;
   
   if(prev)
   {
      prev->next=queue->current->next;
   }
   
   if(next)
   {
      next->prev=queue->current->prev;
   }
   
   if(queue->current==queue->first)
      queue->first=queue->current->next;
   
   if(queue->current==queue->last)
      queue->last=queue->current->prev;
   
   queue->nb_elem--;
   
   queue->current=next;
   
   free(old);
   
   return 0;
}


int process_all_queue_elem(queue_t *queue, void (*f)(void *))
{
   struct queue_elem *ptr;
   
   if(!queue)
      return -1;

   ptr=queue->first;
   if(!ptr)
      return -1;
   do
   {
      f(ptr->d);
      ptr=ptr->next;
      
   } while (ptr);
   
   return 0;
}

