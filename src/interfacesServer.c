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
#include "debug.h"
#include "xPL.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"


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
   for(int16_t i=0;speeds[i][0];i++)
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
      n=sscanf(reste_ptr,":%40[^/n/r]",vitesse);
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


void dispatchXPLMessageToInterfaces(xPL_ServicePtr theService, xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   int ret;

   queue_t *interfaces;
   interfaces_queue_elem_t *iq;

   interfaces=(queue_t *)userValue;
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : Reception message xPL\n",INFO_STR,__func__);
   
   if(first_queue(interfaces)==-1)
      return;

   while(1)
   {
      current_queue(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
         {
            interface_type_001_t *i001 = (interface_type_001_t *)(iq->context);
            if(i001->xPL_callback)
               i001->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i001);
            break;
         }

         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
            if(i002->xPL_callback)
               i002->xPL_callback(theService, theMessage, (xPL_ObjectPtr)i002);
            break;
         }
         default:
            break;
      }
      ret=next_queue(interfaces);
      if(ret<0)
         break;
   }
}


queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db, tomysqldb_md_t *myd)
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
      return NULL;
   }
   init_queue(interfaces);
   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      sqlite3_close(sqlite3_param_db);
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      if(interfaces)
      {
         free(interfaces);
         interfaces=NULL;
      }
      return NULL;
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
         if(interfaces)
         {
            free(interfaces);
            interfaces=NULL;
         }
         return NULL;
      }
   }
   return interfaces;
}


void stop_interfaces(queue_t *interfaces)
{
   interfaces_queue_elem_t *iq;

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
}


void restart_down_interfaces(queue_t *interfaces, sqlite3 *sqlite3_param_db, tomysqldb_md_t *myd)
{
   interfaces_queue_elem_t *iq;
   int16_t ret;

   if(!interfaces->nb_elem)
     return;

   first_queue(interfaces);
   while(1)
   {
      current_queue(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
         {
            interface_type_001_t *i001 = (interface_type_001_t *)(iq->context);
            ret=check_status_interface_type_001(i001);
            if( ret != 0)
            {
               if(i001->xPL_callback)
                  i001->xPL_callback=NULL;
               sleep(1);
               VERBOSE(9) fprintf(stderr,"%s  (%s) : restart interface type_001 (interface_id=%d).\n", INFO_STR, __func__, i001->id_interface);
               restart_interface_type_001(i001, sqlite3_param_db, myd);
            }
            break;
         }
         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
            ret=check_status_interface_type_002(i002);
            if( ret != 0)
            {
               if(i002->xPL_callback)
                  i002->xPL_callback=NULL;
               sleep(1);
               VERBOSE(9) fprintf(stderr,"%s  (%s) : restart interface type_002 (interface_id=%d).\n", INFO_STR, __func__, i002->id_interface);
               restart_interface_type_002(i002, sqlite3_param_db, myd);
            }
            break;
         }
            
         default:
            break;
      }
      ret=next_queue(interfaces);
      if(ret<0)
         break;
   }
}



