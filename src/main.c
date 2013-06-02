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
#include "macros.h"
#include "memory.h"
#include "queue.h"
#include "comio.h"
#include "xbee.h"
#include "tomysqldb.h"

#include "interfaces.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#include "xPLServer.h"
#include "pythonPluginServer.h"

#include "parameters_mgr.h"

tomysqldb_md_t md;
sqlite3 *sqlite3_param_db; // descritpteur SQLITE

sqlite3 *get_sqlite3_param_db()  // temporaire en attendant une reflexion plus globale. Prototype dans globals.h
{
   return sqlite3_param_db;
}

// xbee_xd_t xd;


queue_t *interfaces;


pthread_t *counters_thread=NULL;

char *sqlite3_db_file=NULL;
char *sqlite3_db_param_path=NULL;
char *mysql_db_server=NULL;
char *database=NULL;
char *user=NULL;
char *passwd=NULL;


static void _signal_STOP(int signal_number)
{
   interfaces_queue_elem_t *iq;
   
   VERBOSE(5) fprintf(stderr,"INFO  (_signal_STOP) : arrêt programme demandé (signal = %d).\n",signal_number);
   
   first_queue(interfaces);
   while(interfaces->nb_elem)
   {
      out_queue_elem(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
            stop_interface_type_001((interface_type_001_t *)(iq->context), 0);
            // rajouter ici les free manquants
            break;
         case INTERFACE_TYPE_002:
            stop_interface_type_002((interface_type_002_t *)(iq->context), 0);
            // rajouter ici les free manquants
            break;
            
         default:
            break;
      }
      free(iq);
      iq=NULL;
   }
   
   tomysqldb_release(&md);	
   
   exit(0);
}


static void _signal_HUP(int signal_number)
{
   interfaces_queue_elem_t *iq;
   int ret;
   
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
               restart_interface_type_001(i001, sqlite3_param_db, &md);
            }
            break;
         }
         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002 = (interface_type_002_t *)(iq->context);
            ret=check_status_interface_type_002(i002);
            if( ret != 0)
            {
               restart_interface_type_002(i002, sqlite3_param_db, &md);
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
   fprintf(stderr,"usage : %s -d <dev> -s <serveur> -b <base> -u <user> -p <passwd> -l <sqlite3_db_path>\n\n",cmd);
   fprintf(stderr,"TOUS LES PARAMETRES SONT OBLIGATOIRES\n\n");
}


char *tofind[]={"I:B","L:AA","F:A_A","S:X","I:C", NULL};

int main(int argc, const char * argv[])
{
 int c;
 int _a=0,_s=0,_b=0,_u=0,_p=0,_l=0;
 int ret;

   
#ifdef __DEBUG_ON__
   debug_on();
#else
   debug_off();
#endif
   
   set_verbose_level(9);
   
   setPythonPluginPath("/Data/mea-edomus/plugins");

   while ((c = getopt (argc, (char **)argv, "a:s:b:u:p:l:")) != -1)
   {
      switch (c)
      {
         case 'a':
            string_free_malloc_and_copy(&sqlite3_db_param_path, optarg,1);
            IF_NULL_EXIT(sqlite3_db_param_path,1);
            _a=1;
            break;
            
         case 's':
            string_free_malloc_and_copy(&mysql_db_server, optarg,1);
            IF_NULL_EXIT(mysql_db_server,1);
            _s=1;
            break;
            
         case 'l':
            string_free_malloc_and_copy(&sqlite3_db_file, optarg,1);
            IF_NULL_EXIT(sqlite3_db_file,1);
            _l=1;
            break;
            
         case 'b':
            string_free_malloc_and_copy(&database, optarg,1);
            IF_NULL_EXIT(database,1);
            _b=1;
            break;
            
         case 'u':
            string_free_malloc_and_copy(&user, optarg,1);
            IF_NULL_EXIT(user,1);
            _u=1;
            break;
            
         case 'p':
            if(optarg==NULL)
               string_free_malloc_and_copy(&passwd, "",1);
            else
               string_free_malloc_and_copy(&passwd, optarg,1);
            IF_NULL_EXIT(passwd,1);
            _p=1;
            break;
            
         default:
            VERBOSE(1) fprintf(stderr,"ERROR (main) : Paramètre \"%s\" inconnu.\n",optarg);
            exit(1);
      }
   }
   
   if((_a+_s+_b+_u+_p+_l)!=6)
   {
      usage((char *)argv[0]);
      exit(1);
   }
   
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);
  
   pythonPluginServer(NULL);

   ret=tomysqldb_init(&md,mysql_db_server,database,user,passwd,sqlite3_db_file);
   if(ret==-1)
   {
      VERBOSE(1) fprintf(stderr,"ERROR (main) : impossible d'initialiser la gestion de la base de données.\n");
      exit(1);
   }
   
   
   char sql[255];
   sqlite3_stmt * stmt;
   
   sqlite3_config(SQLITE_CONFIG_SERIALIZED); // pour le multithreading
   ret = sqlite3_open (sqlite3_db_param_path, &sqlite3_param_db);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "ERROR (main) : sqlite3_open - %s\n", sqlite3_errmsg (sqlite3_param_db));
      exit(1);
   }
   
   
   /*
    * recherche de toutes les interfaces
    */
   interfaces=(queue_t *)malloc(sizeof(queue_t));
   if(!interfaces)
   {
      VERBOSE(1) {
         fprintf (stderr, "ERROR (main) : malloc - ");
         perror("");
      }
      exit(1);
   }
   init_queue(interfaces);

   
   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "ERROR (main) : sqlite3_prepare_v2 - %s\n", sqlite3_errmsg (sqlite3_param_db));
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
                        fprintf (stderr, "ERROR (main) : malloc - can't start interface (%d).\n",id_interface);
                        perror(""); }
                     break;
                  }
                  i001->id_interface=id_interface;
                  ret=start_interface_type_001(i001, sqlite3_param_db, id_interface, dev, &md);
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
                        fprintf (stderr, "ERROR  (main) : start_interface_type_001 - can't start interface (%d).\n",id_interface);
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
                        fprintf (stderr, "ERROR  (main) : malloc - can't start interface (%d).\n",id_interface);
                        perror(""); }
                     break;
                  }
                  i002->id_interface=id_interface;
                  ret=start_interface_type_002(i002, sqlite3_param_db, id_interface, dev, &md);
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
                        fprintf (stderr, "ERROR  (main) : start_interface_type_002 - can't start interface (%d).\n",id_interface);
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
            VERBOSE(9) fprintf(stderr,"INFO  (main) : %s not activated (state = %d)\n",dev,state);
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
   
   xPLServer(interfaces);
   
   while(1)
   {
      sleep(5);
   }

   sqlite3_finalize(stmt);
}
