//
//  main.c
//
//  Created by Patrice DIETSCH on 08/07/12.
//  Copyright (c) 2012 -. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#include <sqlite3.h>

#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
#include "queue.h"
#include "comio.h"
#include "xbee.h"
#include "dbServer.h"

#include "interfaces.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#include "xPLServer.h"
#include "pythonPluginServer.h"
#include "httpServer.h"
#include "parameters_mgr.h"


//tomysqldb_md_t md;
tomysqldb_md_t *myd;

sqlite3 *sqlite3_param_db; // descritpteur SQLITE

queue_t *interfaces;


pthread_t *counters_thread=NULL;

char *sqlite3_db_file=NULL;
char *mysql_db_server=NULL;
char *mysql_database=NULL;
char *mysql_user=NULL;
char *mysql_passwd=NULL;

char *sqlite3_db_buff_path=NULL; // old name : sqlite3_db_file
char *sqlite3_db_param_path=NULL; // path to parameters db file

pthread_t *xPLServer_thread=NULL;
pthread_t *pythonPluginServer_thread=NULL;

void strToUpper(char *str)
{
   for(uint16_t i=0;i<strlen(str);i++)
	   str[i]=toupper(str[i]);
}


int16_t read_all_application_parameters(sqlite3 *sqlite3_param_db)
{
   char sql[41];
   sqlite3_stmt * stmt;
   
   sprintf(sql,"SELECT * FROM application_parameters");
   int ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      return -1;
   }
   
   while (1)
   {
      int s = sqlite3_step (stmt);
      
      if (s == SQLITE_ROW)
      {
         // uint32_t id = sqlite3_column_int(stmt, 0);
         char *key = (char *)sqlite3_column_text(stmt, 1);
         char *value = (char *)sqlite3_column_text(stmt, 2);
         // char *complement = (char *)sqlite3_column_text(stmt, 3);
         
         strToUpper(key);
         
         if(strcmp(key,"BUFFERDB")==0)
            string_free_malloc_and_copy(&sqlite3_db_buff_path, value, 1);
         else if (strcmp(key,"DBSERVER")==0)
            string_free_malloc_and_copy(&mysql_db_server, value, 1);
         else if (strcmp(key,"DATABASE")==0)
            string_free_malloc_and_copy(&mysql_database, value, 1);
         else if (strcmp(key,"USER")==0)
            string_free_malloc_and_copy(&mysql_user, value, 1);
         else if (strcmp(key,"PASSWORD")==0)
            string_free_malloc_and_copy(&mysql_passwd, value, 1);
         else if (strcmp(key,"VENDORID")==0)
            set_xPL_vendorID(value);
         else if (strcmp(key,"DEVICEID")==0)
            set_xPL_deviceID(value);
         else if (strcmp(key,"INSTANCEID")==0)
            set_xPL_instanceID(value);
         else if (strcmp(key,"PLUGINPATH")==0)
            setPythonPluginPath(value);
         else if (strcmp(key,"VERBOSELEVEL")==0)
            set_verbose_level(atoi(value));
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
         return -1;
   }
   return 0;
}


static void _signal_STOP(int signal_number)
{
   interfaces_queue_elem_t *iq;
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : arrêt programme demandé (signal = %d).\n",INFO_STR,__func__,signal_number);
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : stoping xPLServer ... ",INFO_STR,__func__);
   pthread_cancel(*xPLServer_thread);
   pthread_join(*xPLServer_thread, NULL);
   VERBOSE(5) fprintf(stderr,"done\n");
   
   first_queue(interfaces);
   while(interfaces->nb_elem)
   {
      out_queue_elem(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
         {
            interface_type_001_t *i001=(interface_type_001_t *)(iq->context);
            if(i001->xPL_callback)
               i001->xPL_callback=NULL;
            stop_interface_type_001(i001, signal_number);
            // rajouter ici les free manquants
            break;
         }
         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002=(interface_type_002_t *)(iq->context);
            if(i002->xPL_callback)
               i002->xPL_callback=NULL;
            stop_interface_type_002(i002, signal_number);
            // rajouter ici les free manquants
            break;
         }
            
         default:
            break;
      }
      free(iq);
      iq=NULL;
   }
   
   tomysqldb_release(myd);
   
   exit(0);
}


static void _signal_HUP(int signal_number)
{
   interfaces_queue_elem_t *iq;
   int ret;
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : communication error signal (signal = %d).\n", INFO_STR, __func__, signal_number);

   if(!interfaces->nb_elem)
      return;
   
   // on cherche qui est à l'origine du signal
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
               VERBOSE(5) fprintf(stderr,"%s  (%s) : restart interface type_001 (interface_id=%d).\n", INFO_STR, __func__, i001->id_interface);
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
               VERBOSE(5) fprintf(stderr,"%s  (%s) : restart interface type_002 (interface_id=%d).\n", INFO_STR, __func__, i002->id_interface);
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
   
   return;
}


void usage(char *cmd)
{
   fprintf(stderr,"usage : %s -a <sqlite3_db_path>\n",cmd);
}

// char *tofind[]={"I:B","L:AA","F:A_A","S:X","I:C", NULL};
//-s
//192.168.0.22
//-b
//domotique
//-u
//domotique
//-p
//maison
//-l
///Data/mea-edomus/queries.db

int main(int argc, const char * argv[])
{
 int c;
 int _a=0;
 int ret;

#ifdef __DEBUG_ON__
   debug_on();
#else
   debug_off();
#endif

   
   set_verbose_level(9);

   DEBUG_SECTION fprintf(stderr,"Starting MEA-EDOMUS 0.1aplha1-A\n");
   
   while ((c = getopt (argc, (char **)argv, "a:")) != -1)
   {
      switch (c)
      {
         case 'a':
            string_free_malloc_and_copy(&sqlite3_db_param_path, optarg,1);
            IF_NULL_EXIT(sqlite3_db_param_path,1);
            _a=1;
            break;
            
         default:
            VERBOSE(1) fprintf(stderr,"%s (%s) : Paramètre \"%s\" inconnu.\n",ERROR_STR,__func__,optarg);
            exit(1);
      }
   }
   
   if(!_a)
   {
      usage((char *)argv[0]);
      exit(1);
   }
   
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);
   
   
   sqlite3_config(SQLITE_CONFIG_SERIALIZED); // pour le multithreading
   ret = sqlite3_open (sqlite3_db_param_path, &sqlite3_param_db);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      exit(1);
   }
   
   ret = read_all_application_parameters(sqlite3_param_db);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : can't load parameters\n",ERROR_STR,__func__);
      exit(1);
   }

   myd=NULL;
#ifndef __NO_TOMYSQL__
   myd=(struct tomysqldb_md_s *)malloc(sizeof(struct tomysqldb_md_s));
   if(!myd)
   {
      VERBOSE(1) {
         fprintf(stderr,"%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      exit(1);
   }
   memset(myd,0,sizeof(struct tomysqldb_md_s));
   
   ret=tomysqldb_init(myd,mysql_db_server,mysql_database,mysql_user,mysql_passwd,sqlite3_db_buff_path);
   if(ret==-1)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : impossible d'initialiser la gestion de la base de données.\n",ERROR_STR,__func__);
      exit(1);
   }
#else
   VERBOSE(9) fprintf(stderr,"%s  (%s) : tomysql not started.\n",INFO_STR,__func__);
#endif
   
   pythonPluginServer_thread=pythonPluginServer(NULL);
   if(pythonPluginServer_thread==NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start Python Plugin Server.\n",ERROR_STR,__func__);
      exit(1);
   }

   httpServer();
   

   /*
    * recherche de toutes les interfaces
    */
   interfaces=(queue_t *)malloc(sizeof(queue_t));
   if(!interfaces)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      exit(1);
   }
   init_queue(interfaces);

   
   char sql[255];
   sqlite3_stmt * stmt;

   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      exit(1);
   }
   while (1)
   {
      int s = sqlite3_step (stmt);
      if (s == SQLITE_ROW)
      {
         int id_interface;
         int id_type;
         const unsigned char *dev;
         int state;
         
         id_interface = sqlite3_column_int(stmt, 1);
         id_type = sqlite3_column_int(stmt, 2);
         dev = sqlite3_column_text(stmt, 5);
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
                     VERBOSE(1) {
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
                     VERBOSE(1) {
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
                     VERBOSE(1) {
                        fprintf (stderr, "%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                        perror(""); }
                     break;
                  }
                  i002->id_interface=id_interface;
                  ret=start_interface_type_002(i002, sqlite3_param_db, id_interface, dev, myd);
                  if(!ret)
                  {
                     interfaces_queue_elem_t *iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
                     iq->type=id_type;
                     iq->context=i002;
                     in_queue_elem(interfaces, iq);
                  }
                  else
                  {
                     VERBOSE(1) {
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
         exit(1);
   }
   
   
   xPLServer_thread=xPLServer(interfaces);
   if(xPLServer_thread==NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start xpl server.\n",ERROR_STR,__func__);
      exit(1);
   }

   
   free(mysql_db_server);
   mysql_db_server=NULL;
   free(mysql_database);
   mysql_database=NULL;
   free(mysql_user);
   mysql_user=NULL;
   free(mysql_passwd);
   mysql_passwd=NULL;

   while(1)
   {
      sleep(5);
   }

   sqlite3_finalize(stmt);
}
