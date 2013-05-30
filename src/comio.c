//
//  comio.c
//
//  Created by Patrice DIETSCH on 16/07/12.
//  Copyright (c) 2012 -. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "debug.h"
#include "comio.h"

#define TIMEOUT_DELAY 1

struct termios comio_options_old;


void _comio_reprise_err_sys(comio_ad_t *ad)
{
   comio_close(ad);
   comio_open(ad, ad->serial_dev_name);
}


void _comio_free_queue_elem(void *d)
{
   comio_queue_elem_t *e=(comio_queue_elem_t *)d;
   
   free(e);
   e=NULL;
}


int comio_set_trap(comio_ad_t *ad, int numTrap, trap_f trap)
{
   if(numTrap>0 && numTrap<=MAX_TRAP)
   {
      ad->tabTrap[numTrap-1].trap=trap;
      ad->tabTrap[numTrap-1].args=NULL;
      return 0;
   }
   return -1;
}


int comio_set_trap2(comio_ad_t *ad, int numTrap, trap_f trap, void *args)
{
   if(numTrap>0 && numTrap<=MAX_TRAP)
   {
      ad->tabTrap[numTrap-1].trap=trap;
      ad->tabTrap[numTrap-1].args=args;
      return 0;
   }
   return -1;
}


int comio_remove_trap(comio_ad_t *ad, int numTrap)
{
   if(numTrap>0 && numTrap<=MAX_TRAP)
   {
      ad->tabTrap[numTrap-1].trap=NULL;
      ad->tabTrap[numTrap-1].args=NULL;
      return 0;
   }
   return -1;
}


static void _comio_timeout(int signal_number)
{
   if (signal_number == SIGALRM)
   {
      DEBUG_SECTION fprintf(stderr,"DEBUG (_comio_timeout) : Timeout (read semaphore)\n");
   }
}


int _comio_read(comio_ad_t *ad, unsigned char *op, unsigned char *var, unsigned char *type, unsigned int *val, char **option, int *nerr)
{
   unsigned char c;
   fd_set input_set;
   struct timeval timeout;
   
   int error_num=0;
   int trap_num=-1;
   
   char *data=NULL;
   int data_len=0;
   int data_index;
   
#ifdef COMIO_DEBUG
   char buff[256];
   int nb_cars=0;
#endif
   
   timeout.tv_sec  = 5; // timeout after 1 second
   timeout.tv_usec = 0;
   
   FD_ZERO(&input_set);
   FD_SET(ad->fd, &input_set);
   
   int ret=0;
   int step=0;
   int nb_retry=0;

   buff[0]=0;
   
   while(1)
   {
      ret = select(ad->fd+1, &input_set, NULL, NULL, &timeout);
      if (ret <= 0)
      {
         if(ret == 0)
            *nerr=COMIO_ERR_TIMEOUT;
         else
            *nerr=COMIO_ERR_SELECT;
         goto on_error_exit_arduino_read;
      }
      
      ret=read(ad->fd,&c,1);
      if(ret!=1)
      {
         if(nb_retry>4)
         {
            *nerr=COMIO_ERR_READ;
            goto on_error_exit_arduino_read;
         }
         nb_retry++;
         continue;
      }
      nb_retry=0;
      
#ifdef COMIO_DEBUG
      buff[nb_cars++]=c;
#endif
      switch(step)
      {
         case 0:
            if(c=='&')
            {
               step++;
               break;
            }
            if(c=='!')
            {
               step=10;
               break;
            }
            if(c=='*')
            {
               step=20;
               break;
            }
            if(c=='#')
            {
               step=30;
               break;
            }
            
            *nerr=COMIO_ERR_STARTTRAME;
            goto on_error_exit_arduino_read;
         case 1:
            *op=(((unsigned char)c) & 0xF0) >> 4;
            *type=(((unsigned char)c) & 0x0F);
            step++;
            break;
         case 2:
            *var=(unsigned char)c;
            step++;
            break;
         case 3:
            *val=((int)c)*256;
            step++;
            break;
         case 4:
            *val=*val+(int)c;
            step++;
            break;
         case 5:
            if(c!='$')
            {
               *nerr=COMIO_ERR_ENDTRAME;
               goto on_error_exit_arduino_read;
            }
            else
            {
               if(trap_num>0)
               {
                  if(trap_num<=MAX_TRAP)
                  {
                     *nerr=COMIO_ERR_NOERR;
                     return trap_num;
                  }
                  else if(trap_num>=256) // && (trap_num - 256)<=MAX_TRAP)
                  {
                     *nerr=COMIO_ERR_NOERR;
                     *option=data;
                     return trap_num;
                  }
                  else
                  {
                     *nerr=COMIO_ERR_BADTRAPNUM;
                     return -1;
                  }
               }
               
               if(error_num)
                  *nerr=error_num+100;
               else
                  *nerr=COMIO_ERR_NOERR;
               return 0;
            }
            break;
            
         case 10:
            error_num=(int)c;
            step=5;
            break;
            
         case 20: // trap court
            trap_num=(int)c;
            step=5;
            break;
            
         case 30: // trap long
            trap_num=(int)c+256;
            step=31;
            break;
         case 31:
            data_len=(int)c;
            data_index=0;
            data=(char *)malloc(data_len);
            step=32;
            break;
         case 32:
            if(data)
               data[data_index++]=c;
            if(data_index==data_len)
            {
               step=5;
               break;
            }
            break;

      }
   }
   *nerr=COMIO_ERR_UNKNOWN;
   
on_error_exit_arduino_read:
   if(data)
      free(data);
#ifdef COMIO_DEBUG
   buff[nb_cars++]=0;
   if(strlen(buff)>0)
   {
      for(int i=0;i<strlen(buff);i++)
         fprintf(stderr,"%x(%c) ",buff[i],buff[i]);
      fprintf(stderr,"\n");
      fprintf(stderr,"nerr=%d\n",*nerr);
   }
#endif
   return -1;
}


int _comio_write(comio_ad_t *ad, unsigned char op, unsigned char var, unsigned char type, unsigned int val)
{
   unsigned char trame[6];
   unsigned char op_type;
   int i=0;
   
   trame[i++]='?';
   
   op_type=(op & 0x0F) << 4;
   op_type=op_type | (type & 0x0F);
   trame[i++]=op_type;
   
   trame[i++]=var;
   
   unsigned char pfort=(unsigned char)(val/256);
   trame[i++]=pfort;
   
   unsigned char pfaible=(unsigned char)(val-(unsigned int)(pfort*256));
   trame[i++]=pfaible;
   
   trame[i++]='$';
   
   write(ad->fd,trame,i);
   
   return 0;
}


int comio_operation(comio_ad_t *ad, unsigned char op, unsigned char var, unsigned char type, unsigned int val, int *comio_err)
{
   comio_queue_elem_t *c;
   
   if(_comio_write(ad,op,var,type,val)==0) // send operation
   {
      int ret;
      
      struct timeval tv;
      struct timespec ts;
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec + TIMEOUT_DELAY;
      ts.tv_nsec = 0;
      
      pthread_mutex_lock(&ad->sync_lock);
      if(ad->queue->nb_elem==0)
      {
         ret=pthread_cond_timedwait(&ad->sync_cond, &ad->sync_lock, &ts);
         
         if(ret)
         {
            if(ret==ETIMEDOUT)
            {
               *comio_err=COMIO_ERR_TIMEOUT;
               
               DEBUG_SECTION fprintf(stderr,"DEBUG (comio_operation) : Nb elements in queue after TIMEOUT : %ld)\n",ad->queue->nb_elem);
               if(ad->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
                  clear_queue(ad->queue,_comio_free_queue_elem);
            }
            else
            {
               *comio_err=COMIO_ERR_SYS;
            }
            pthread_mutex_unlock(&ad->sync_lock);
            return -1;
         }
      }
      
      ret=out_queue_elem(ad->queue, (void **)&c);
      pthread_mutex_unlock(&ad->sync_lock);
      
      if(!ret)
      {
         if(c->comio_err!=COMIO_ERR_NOERR)
         {
            *comio_err=c->comio_err;
            free(c);
            c=NULL;
            return -1;
         }
         
         if((c->ret_op==op)&&(c->ret_var==var)&&(c->ret_type==type))
         {
            int ret_val=c->ret_val;
            *comio_err=c->comio_err;
            free(c);
            c=NULL;
            return ret_val;
         }
         else
         {
            *comio_err=COMIO_ERR_DISCORDANCE;
            return -1;
         }
      }
      else
      {
         *comio_err=COMIO_ERR_UNKNOWN1;
         return -1;
      }
   }
   *comio_err=COMIO_ERR_UNKNOWN;
   return -1;
}


void *_comio_read_thread_func(void *args)
{
   unsigned char op=0;
   unsigned char var=0;
   unsigned char type=0;
   unsigned int val=0;
   int nerr=0;
   char *buff=NULL;
   
   comio_ad_t *ad=(comio_ad_t *)args;
   while(1)
   {
      int ret=_comio_read(ad, &op, &var, &type, &val, &buff, &nerr);
      if(ret==0)
      {
         comio_queue_elem_t *c=malloc(sizeof(comio_queue_elem_t));
         if(c)
         {
            c->ret_op=op;
            c->ret_var=var;
            c->ret_type=type;
            c->ret_val=val;
            c->comio_err=nerr;
            
            pthread_mutex_lock(&ad->sync_lock);
            
            in_queue_elem(ad->queue, c);
            
            if(ad->queue->nb_elem==1)
               pthread_cond_broadcast(&ad->sync_cond);
            pthread_mutex_unlock(&ad->sync_lock);
         }
         else
         {
            VERBOSE(1) {
               fprintf (stderr, "ERROR (arduino_read_thread_func) : malloc error (%s/%d) - ",__FILE__,__LINE__);
               perror("");
            }
            pthread_exit(NULL);
         }
      }
      if(ret>0)
      {
         if(ret<MAX_TRAP)
         {
            // on vient de recevoir un trap "valide"
            if(ad->tabTrap[ret-1].trap)
               ad->tabTrap[ret-1].trap(ret,ad->tabTrap[ret-1].args,NULL);
            else
            {
               DEBUG_SECTION fprintf(stderr,"DEBUG (_comio_read_thread_func) : no handler for trap %d.\n",ret);
            }
         }
         else
         {
            // c'est un trap long (avec des datas)
            int trap=ret-256-1;
            if(ad->tabTrap[trap].trap)
               ad->tabTrap[trap].trap(ret-256,ad->tabTrap[trap].args, buff);
            else
            {
               DEBUG_SECTION fprintf(stderr,"DEBUG (_comio_read_thread_func) : no handler for long trap %d.\n",trap);
            }
            free(buff);
         }
      }
      
      if(ret<0)
      {
         if(nerr!=COMIO_ERR_TIMEOUT && nerr!=COMIO_ERR_STARTTRAME && nerr!=COMIO_ERR_ENDTRAME)
         {
            // connection lost with arduino
            VERBOSE(1) {
               fprintf(stderr,"ERROR (_comio_read_thread_func) : irremediable error (%d) - ",nerr);
               perror("");
            }
            ad->signal_flag=1;
            raise(SIGHUP);
            pthread_exit(NULL);
         }
      }
      pthread_testcancel();
   }
   
   pthread_exit(NULL);
}


int comio_call(comio_ad_t *ad, unsigned char num_function, unsigned int val, int *comio_err)
{
   return comio_operation(ad, OP_FONCTION, num_function, TYPE_FONCTION, val, comio_err);
}


int comio_open(comio_ad_t *ad, char *dev)
{
   struct termios options;
   int fd;
   
   int flags;
   memset (ad,0,sizeof(comio_ad_t));
   
   flags=O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL;
#ifdef O_CLOEXEC
   flags |= O_CLOEXEC;
#endif
   
   fd = open(dev, flags);
   if (fd == -1)
   {
      // can't open serial port
      return -1;
   }
   strcpy(ad->serial_dev_name,dev);
   
   // backup serial port caracteristics
   tcgetattr(fd, &comio_options_old);
   
   // RAZ serial options structure (termios)
   memset(&options, 0, sizeof(struct termios));
   
   // set speed
   if(cfsetispeed(&options, COMIO_DEBIT_PORT)<0)
      return -1;
   if(cfsetospeed(&options, COMIO_DEBIT_PORT)<0)
      return -1;
   
   // ???
   options.c_cflag |= (CLOCAL | CREAD); // mise à 1 du CLOCAL et CREAD
   
   // data 8 bits, no parity, 1 stop bit (8N1):
   options.c_cflag &= ~PARENB; // (N)
   options.c_cflag &= ~CSTOPB; // 1 bit de stop seulement (1)
   options.c_cflag &= ~CSIZE;
   options.c_cflag |= CS8; // 8 bits (8)
   
   // bit ICANON = 0 => port en RAW (au lieu de canonical)
   // bit ECHO =   0 => pas d'écho des caractères
   // bit ECHOE =  0 => pas de substitution du caractère d'"erase"
   // bit ISIG =   0 => interruption non autorisées
   options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
   
   // no parity check
   options.c_iflag &= ~INPCK;
   
   // pas de contrôle de flux
   options.c_iflag &= ~(IXON | IXOFF | IXANY);
   
   // parce qu'on est en raw
   options.c_oflag &=~ OPOST;
   
   // VMIN : Nombre minimum de caractère à lire
   // VTIME : temps d'attentes de données (en 10eme de secondes)
   // à 0 car O_NDELAY utilisé
   options.c_cc[VMIN] = 0;
   options.c_cc[VTIME] = 0;
   
   // réécriture des options
   tcsetattr(fd, TCSANOW, &options);
   
   ad->fd=fd;
   
   // préparation synchro consommateur / producteur
   pthread_cond_init(&ad->sync_cond, NULL);
   pthread_mutex_init(&ad->sync_lock, NULL);
   
   ad->queue=(queue_t *)malloc(sizeof(queue_t));
   if(!ad->queue)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (comio_open) : malloc error (%s/%d) - ",__FILE__,__LINE__);
         perror("");
      }
      return -1;
   }
   
   init_queue(ad->queue);
   
   for(int i=0;i<MAX_TRAP;i++)
      ad->tabTrap[i].trap=(trap_f)NULL;
   
   ad->signal_flag=0;
   
   sleep(2); // delay : wait arduino reset for USB
   
   if(pthread_create (&(ad->read_thread), NULL, _comio_read_thread_func, (void *)ad))
      return -1;
   
   return fd;
}


int comio_init(comio_ad_t *ad, char *dev)
{
   int fd;
   int flag=0;
   
   VERBOSE(5) fprintf(stderr,"INFO  (comio_init) : try to initialize commmunication (%s).\n",dev);
   
   //comio_close(ad);
   for(int i=0;i<5;i++) // try 5 times to reopen
   {
      fd = comio_open(ad, dev);
      if (fd == -1)
      {
         VERBOSE(5) {
            fprintf(stderr,"INFO  (comio_init) : unable to open serial port (%s) - ",dev);
            perror("");
         }
      }
      else
      {
         flag=1;
         break;
      }
      sleep(5);
   }
   
   if(!flag)
   {
      VERBOSE(1) fprintf(stderr,"ERROR (comio_init) : can't initialize communication.\n");
      return -1;
   }
   
   VERBOSE(5) fprintf(stderr,"INFO  (comio_init) : initialisazion done.\n");
   return fd;
}


void comio_close(comio_ad_t *ad)
{
   if(ad->read_thread)
   {
      pthread_cancel(ad->read_thread);
      pthread_join(ad->read_thread,NULL);
      // ad->read_thread=NULL;
   }
   
   if(ad->queue)
   {
      clear_queue(ad->queue,_comio_free_queue_elem);
      free(ad->queue);
      ad->queue=NULL;
   }
   
   tcsetattr(ad->fd, TCSANOW, &comio_options_old);
   
   close(ad->fd);
}
