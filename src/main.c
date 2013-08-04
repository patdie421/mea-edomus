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

#include "globals.h"
#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
#include "queue.h"
#include "string_utils.h"

//#include "parameters_mgr.h"

//#include "comio.h"
//#include "xbee.h"

#include "interfaces.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"
#include "httpServer.h"


tomysqldb_md_t *myd;
sqlite3 *sqlite3_param_db; // descritpteur SQLITE
queue_t *interfaces;

pthread_t *xPLServer_thread=NULL;
pthread_t *pythonPluginServer_thread=NULL;
// pthread_t *counters_thread=NULL;

char *sqlite3_db_file=NULL;
char *mysql_db_server=NULL;
char *mysql_database=NULL;
char *mysql_user=NULL;
char *mysql_passwd=NULL;
char *sqlite3_db_buff_path=NULL; // old name : sqlite3_db_file
char *sqlite3_db_param_path=NULL; // path to parameters db file


void usage(char *cmd)
/**
 * \brief     Affiche les "usages" de mea_edomus
 * \param     cmd    nom d'appel de mea_edomus (argv[0])
 */
{
   fprintf(stderr,"usage : %s -a <sqlite3_db_path>\n",cmd);
}


int16_t read_all_application_parameters(sqlite3 *sqlite3_param_db)
/**
 * \brief     Chargement de tous les parametres nécessaires au démarrage de l'application depuis la base de parametres
 * \param     sqlite3_param_db descripteur initialisé (ouvert) d'accès à la base.
 * \return   -1 en cas d'erreur, 0 sinon
 */
{
   sqlite3_stmt * stmt;
   
   //char sql[41];
   //sprintf(sql,"SELECT * FROM application_parameters");
   char *sql="SELECT * FROM application_parameters";
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
/**
 * \brief     Traitement des signaux d'arrêt (SIGINT, SIGQUIT, SIGTERM)
 * \details   L'arrêt de l'application se déclanche par l'émission d'un signal d'arrêt. A l'a reception de l'un de ces signaux
 *            Les différents process de mea_edomus doivent être arrêtés.
 * \param     signal_number  numéro du signal (pas utilisé mais nécessaire pour la déclaration du handler).
 */
{
   interfaces_queue_elem_t *iq;
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : Stopping mea-edomus requested (signal = %d).\n",INFO_STR,__func__,signal_number);
   VERBOSE(5) fprintf(stderr,"%s  (%s) : Stopping xPLServer... ",INFO_STR,__func__);
   pthread_cancel(*xPLServer_thread);
   pthread_join(*xPLServer_thread, NULL);
   VERBOSE(5) fprintf(stderr,"done\n");
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : Stopping interfaces... (",INFO_STR,__func__);
   first_queue(interfaces);
   while(interfaces->nb_elem)
   {
      out_queue_elem(interfaces, (void **)&iq);
      switch (iq->type)
      {
         case INTERFACE_TYPE_001:
         {
            interface_type_001_t *i001=(interface_type_001_t *)(iq->context);
            VERBOSE(5) fprintf(stderr,"(%d:",i001->id_interface);
            if(i001->xPL_callback)
               i001->xPL_callback=NULL;
            stop_interface_type_001(i001, signal_number);
            VERBOSE(5) fprintf(stderr,"done)");
            break;
         }
         case INTERFACE_TYPE_002:
         {
            interface_type_002_t *i002=(interface_type_002_t *)(iq->context);
            VERBOSE(5) fprintf(stderr,"(%d:",i002->id_interface);
            if(i002->xPL_callback)
               i002->xPL_callback=NULL;
            stop_interface_type_002(i002, signal_number);
            VERBOSE(5) fprintf(stderr,"done)");
            break;
         }
            
         default:
            break;
      }
      free(iq);
      iq=NULL;
   }
   VERBOSE(5) fprintf(stderr,") done\n");
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : Stopping dbServer... ",INFO_STR,__func__);
   tomysqldb_release(myd);
   VERBOSE(5) fprintf(stderr,"done\n");
   
   exit(0);
}


static void _signal_HUP(int signal_number)
/**
 * \brief     Traitement des signaux HUP
 * \details   Un signal HUP peut être est levé par les threads de gestion des interfaces lorsqu'elles détectent une anomalie bloquante.
 *            Le gestionnaire de signal lorsqu'il est déclanché doit determiner quelle interface a émis le signal et forcer un arrêt/relance de cette l'interface.
 * \param     signal_number  numéro du signal (pas utilisé mais nécessaire pour la déclaration du handler).
 */
{
   interfaces_queue_elem_t *iq;
   int ret;
   
   VERBOSE(5) fprintf(stderr,"%s  (%s) : communication error signal (signal = %d).\n", INFO_STR, __func__, signal_number);
   if(!interfaces->nb_elem)
      return;
   
   // on cherche qui est à l'origine du signal et on le relance
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


int main(int argc, const char * argv[])
/**
 * \brief     Point d'entrée du mea_edomus
 * \details   Intitialisation des structures de données et lancement des différents "process" (threads) de l'application
 * \param     argc   parametres de lancement de l'application. Un seul paramètre attendu (-a parameters.db).
 * \param     argv   nombre de paramètres.
 * \return    1 en cas d'erreur, 0 sinon
 */
{
   int c;
   int ret;
   
#ifdef __DEBUG_ON__
   debug_on();
   set_verbose_level(9);
#else
   debug_off();
#endif
   
   DEBUG_SECTION fprintf(stderr,"Starting MEA-EDOMUS %s\n",__MEA_EDOMUS_VERSION__);
   
   // initialisation gestions des signaux
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);
   
   // récupération des paramètres de la ligne de commande
   int _a=0;
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
   
   
   // ouverture de la base de paramétrage
   sqlite3_config(SQLITE_CONFIG_SERIALIZED); // pour le multithreading
   ret = sqlite3_open (sqlite3_db_param_path, &sqlite3_param_db);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      exit(1);
   }
   
   
   // lecture de tous les paramètres de l'application
   ret = read_all_application_parameters(sqlite3_param_db);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : can't load parameters\n",ERROR_STR,__func__);
      exit(1);
   }
   
   
   // initialisation de la communication avec la base MySQL
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
   VERBOSE(9) fprintf(stderr,"%s  (%s) : dbServer not started.\n",INFO_STR,__func__);
#endif
   
   
   // initialisation du serveur de plugin python
   pythonPluginServer_thread=pythonPluginServer(NULL);
   if(pythonPluginServer_thread==NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start Python Plugin Server.\n",ERROR_STR,__func__);
      exit(1);
   }
   
   
   // initialisation du serveur HTTP
   httpServer(8083,"/Data/mea-edomus/gui","/Data/mea-edomus/bin/php-cgi","/Data/mea-edomus/etc");
   
   
   // initialisation des interfaces
   char sql[255];
   sqlite3_stmt * stmt;
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
   sqlite3_finalize(stmt);
   
   
   // initialisation du serveur xPL
   xPLServer_thread=xPLServer(interfaces);
   if(xPLServer_thread==NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start xpl server.\n",ERROR_STR,__func__);
      exit(1);
   }
   
   
   // libération des espaces mémoires globaux inutiles
   free(mysql_db_server);
   mysql_db_server=NULL;
   free(mysql_database);
   mysql_database=NULL;
   free(mysql_user);
   mysql_user=NULL;
   free(mysql_passwd);
   mysql_passwd=NULL;
   
   DEBUG_SECTION fprintf(stderr,"MEA-EDOMUS %s starded\n",__MEA_EDOMUS_VERSION__);
   
   // boucle sans fin.
   while(1)
   {
      sleep(5);
   }
}