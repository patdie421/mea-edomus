//
//  interfaces.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 04/11/2013.
//
//

#include <stdio.h>
#include <termios.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "string_utils.h"
#include "queue.h"

uint32_t speeds[][3]={
   {   300,    B300},
   {  1200,   B1200},
   {  4800,   B4800},
   {  9600,   B9600},
   { 19200,  B19200},
   { 38400,  B38400},
   { 57600,  B57600},
   {115200, B115200},
   {0,0}
};


int32_t get_speed_from_speed_t(speed_t speed)
{
   for(int16_t i;speeds[i][0];i++)
   {
      if(speeds[i][1]==speed)
         return speeds[i][0];
   }
   return -1;
}


int16_t get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed)
{
   *speed=0;
   char _dev[41];
   char reste[41];
   char vitesse[41];

   char *_dev_ptr;
   char *reste_ptr;
   char *vitesse_ptr;
   char *end=NULL;

   int16_t n=sscanf(device,"SERIAL://%40[^:]%40[^/r/n]",_dev,reste);
   if(n<=0)
      return -1;

   _dev_ptr=mea_strtrim(_dev);

   if(n==1)
   {
      *speed=B9600;
   }
   else
   {
      uint32_t v;

      reste_ptr=mea_strtrim(reste);
      n=sscanf(reste,":%40[^/n/r]",vitesse);
      if(n!=1)
         return -1;

      vitesse_ptr=mea_strtrim(vitesse);
      v=strtol(vitesse_ptr,&end,10);
      if(end==vitesse || *end!=0 || errno==ERANGE)
         return -1;

      *speed=0;
      int i;
      for(i=0;speeds[i][0];i++)
      {
         if(speeds[i][0]==v)
         {
            *speed=speeds[i][1];
            break;
         }
      }
      if(*speed==0)
         return -1;
   }

   strncpy(dev,_dev_ptr,dev_l);

   return 0;
}


queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db)
{
   char sql[255];
   sqlite3_stmt * stmt;
   int16_t ret;
   queue_t *interfaces;

   interfaces=(queue_t *)malloc(sizeof(queue_t));
   if(!interfaces)
   {
      sqlite3_close(sqlite3_param_db);
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      stop_all_services_and_exit();
   }
   init_queue(interfaces);
   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      sqlite3_close(sqlite3_param_db);
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      stop_all_services_and_exit();
   }
   while (1)
   {
      int s = sqlite3_step (stmt); // sqlite function need int
      if (s == SQLITE_ROW)
      {
         int16_t id_interface;
         int16_t id_type;
         const unsigned char *dev;
         const unsigned char *parameters;
         int16_t state;
         
         id_interface = sqlite3_column_int(stmt, 1);
         id_type = sqlite3_column_int(stmt, 2);
         dev = sqlite3_column_text(stmt, 5);
         parameters = sqlite3_column_text(stmt, 6);
         state = sqlite3_column_int(stmt, 7);
         
         if(state==1)
         {
            switch(id_type)
            {
               case INTERFACE_TYPE_001:
               {
                  interface_type_001_t *i001;
                  
                  i001=(interface_type_001_t *)malloc(sizeof(interface_type_001_t));
                  if(!i001)
                  {
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror(""); }
                     break;
                  }
                  i001->id_interface=id_interface;
                  ret=start_interface_type_001(i001, sqlite3_param_db, id_interface, dev, myd);
                  if(!ret)
                  {
                     interfaces_queue_elem_t *iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
                     iq->type=id_type;
                     iq->context=i001;
                     in_queue_elem(interfaces, iq);
                  }
                  else
                  {
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : start_interface_type_001 - can't start interface (%d).\n",ERROR_STR,__func__,id_interface);
                     }
                     free(i001);
                     i001=NULL;
                  }
                  break;
               }
                  
               case INTERFACE_TYPE_002:
               {
                  interface_type_002_t *i002;
                  
                  i002=(interface_type_002_t *)malloc(sizeof(interface_type_002_t));
                  if(!i002)
                  {
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror(""); }
                     break;
                  }
                  i002->id_interface=id_interface;
                  ret=start_interface_type_002(i002, sqlite3_param_db, id_interface, dev, myd, (char *)parameters);
                  if(!ret)
                  {
                     interfaces_queue_elem_t *iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
                     iq->type=id_type;
                     iq->context=i002;
                     in_queue_elem(interfaces, iq);
                  }
                  else
                  {
                     VERBOSE(2) {
                        fprintf (stderr, "%s (%s) : start_interface_type_002 - can't start interface (%d).\n",ERROR_STR,__func__,id_interface);
                     }
                     free(i002);
                     i002=NULL;
                  }
                  break;
               }
                  
               default:
                  break;
            }
         }
         else
         {
            VERBOSE(9) fprintf(stderr,"%s  (%s) : %s not activated (state = %d)\n",INFO_STR,__func__,dev,state);
         }
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_finalize(stmt);
         sqlite3_close(sqlite3_param_db);
         stop_all_services_and_exit();
      }
   }
   return interfaces;
}


void stop_interfaces(queut_t *interfaces)
{
   VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping interfaces...\n",INFO_STR,__func__);
   first_queue(interfaces);
   while(interfaces->nb_elem)
   {
      out_queue_elem(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
         {
            interface_type_001_t *i001=(interface_type_001_t *)(iq->context);
            VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping #%d\n",INFO_STR,__func__,i001->id_interface);
            if(i001->xPL_callback)
               i001->xPL_callback=NULL;
            stop_interface_type_001(i001);
            break;
         }
         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002=(interface_type_002_t *)(iq->context);
            VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping #%d\n",INFO_STR,__func__,i002->id_interface);
            if(i002->xPL_callback)
               i002->xPL_callback=NULL;
            stop_interface_type_002(i002);
            break;
         }
         
         default:
            break;
      }
      free(iq);
      iq=NULL;
   }
   VERBOSE(9) fprintf(stderr,"%s  (%s) : done\n",INFO_STR,__func__);
}



