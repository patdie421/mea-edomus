//
//  main.c
//
//  Created by Patrice DIETSCH on 08/07/12.
//  Copyright (c) 2012 -. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

#include <sqlite3.h>

#include "globals.h"
#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
#include "queue.h"
#include "string_utils.h"

#include "init.h"

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

char *params_list[MAX_LIST_SIZE];

char *phpcgibin=NULL;

void usage(char *cmd)
/**
 * \brief     Affiche les "usages" de mea_edomus
 * \param     cmd    nom d'appel de mea_edomus (argv[0])
 */
{
   fprintf(stderr,"usage : %s -i -a <sqlite3_db_path>\n",cmd);
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
            string_free_malloc_and_copy(&params_list[SQLITE3_DB_BUFF_PATH], value, 1);
         else if (strcmp(key,"DBSERVER")==0)
            string_free_malloc_and_copy(&params_list[MYSQL_DB_SERVER], value, 1);
         else if (strcmp(key,"DATABASE")==0)
            string_free_malloc_and_copy(&params_list[MYSQL_DATABASE], value, 1);
         else if (strcmp(key,"DBPORT")==0)
            string_free_malloc_and_copy(&params_list[MYSQL_DB_PORT], value, 1);
         else if (strcmp(key,"USER")==0)
            string_free_malloc_and_copy(&params_list[MYSQL_USER], value, 1);
         else if (strcmp(key,"PASSWORD")==0)
            string_free_malloc_and_copy(&params_list[MYSQL_PASSWD], value, 1);
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
         else if (strcmp(key,"PHPCGIPATH")==0)
            string_free_malloc_and_copy(&params_list[PHPCGI_PATH], value, 1);
         else if (strcmp(key,"PHPINIPATH")==0)
            string_free_malloc_and_copy(&params_list[PHPINI_PATH], value, 1);
         else if (strcmp(key,"GUIPATH")==0)
            string_free_malloc_and_copy(&params_list[GUI_PATH], value, 1);
         else if (strcmp(key,"LOGPATH")==0)
            string_free_malloc_and_copy(&params_list[LOG_PATH], value, 1);
         else if (strcmp(key,"BUFFDBPATH")==0)
            string_free_malloc_and_copy(&params_list[SQLITE3_DB_BUFF_PATH], value, 1);
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
   
   for(int i=0;i<MAX_LIST_SIZE;i++)
   {
      if(params_list[i])
      {
        free(params_list[i]);
        params_list[i]=NULL;
      }
   }

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
   char *buff;
   
   // toutes les options possibles
   static struct option long_options[] = {
      {"init",        no_argument,       0,  'i'                  },
      {"autoinit",    no_argument,       0,  'a'                  },
      {"update",      no_argument,       0,  'u'                  },
      {"basepath",    required_argument, 0,  MEA_PATH             }, // 'p'
      {"paramsdb",    required_argument, 0,  SQLITE3_DB_PARAM_PATH}, // 'd'
      {"configfile",  required_argument, 0,  CONFIG_FILE          }, // 'c'
      {"phpcgipath",  required_argument, 0,  PHPCGI_PATH          }, // 'C'
      {"phpinipath",  required_argument, 0,  PHPINI_PATH          }, // 'H'
      {"guipath",     required_argument, 0,  GUI_PATH             }, // 'G'
      {"logpath",     required_argument, 0,  LOG_PATH             }, // 'L'
      {"pluginspath", required_argument, 0,  PLUGINS_PATH         }, // 'A'
      {"bufferdbpath",required_argument, 0,  SQLITE3_DB_BUFF_PATH }, // 'B'
      {"bufferdbname",required_argument, 0,  SQLITE3_DB_BUFF_NAME }, // 'F'
      {"dbserver",    required_argument, 0,  MYSQL_DB_SERVER      }, // 'D'
      {"dbport",      required_argument, 0,  MYSQL_DB_PORT        }, // 'P'
      {"dbname",      required_argument, 0,  MYSQL_DATABASE       }, // 'N'
      {"dbuser",      required_argument, 0,  MYSQL_USER           }, // 'U'
      {"dbpassword",  required_argument, 0,  MYSQL_PASSWD         }, // 'W'
      {"vendorid",    required_argument, 0,  VENDOR_ID            }, // 'V'
      {"deviceid",    required_argument, 0,  DEVICE_ID            }, // 'E'
      {"instanceid",  required_argument, 0,  INSTANCE_ID          }, // 'S'
      {0,             0,                 0,  0                    }
   };

#ifdef __DEBUG_ON__
   debug_on();
   set_verbose_level(9);
#else
   debug_off();
#endif
   
   DEBUG_SECTION fprintf(stderr,"Starting MEA-EDOMUS %s\n",__MEA_EDOMUS_VERSION__);

   sqlite3_config(SQLITE_CONFIG_SERIALIZED); // pour le multithreading

   // initialisation de la liste des parametres à NULL
   for(int i=0;i<MAX_LIST_SIZE;i++)
      params_list[i]=NULL;

   // chemin "théorique" de l'installation mea-domus (si les recommendations ont été respectées)
   buff=(char *)malloc(strlen(argv[0])+1);
   if(realpath(argv[0], buff))
   {
      char *path;

      path=malloc(strlen(dirname(buff))+1);
      strcpy(path,dirname(buff));
      
      params_list[MEA_PATH]=(char *)malloc(strlen(dirname(path))+1);
      strcpy(params_list[MEA_PATH],dirname(path));
      free(path);
   }
   free(buff);

   // récupération des paramètres de la ligne de commande
   int _d=0, _i=0, _a=0, _c, _u=0, _o=0;
   int option_index = 0;
   
   while ((c = getopt_long(argc, (char * const *)argv, "iaup:d:c:C:H:G:L:A:B:F:D:P:N:U:W:V:E:S:", long_options, &option_index)) != -1)
   {
      switch (c)
      {
         case 'p':
            c=MEA_PATH;
            break;
            
         case 'd':
         case SQLITE3_DB_PARAM_PATH:
            string_free_malloc_and_copy(&params_list[SQLITE3_DB_PARAM_PATH], optarg, 1);
            IF_NULL_EXIT(&params_list[SQLITE3_DB_PARAM_PATH], 1);
            c=-1;
            _d=1;
            break;
         
         case 'c': // fichier de configuration (mea-edomus.ini qui contient basepath et paramsdb)
         case CONFIG_FILE:
            string_free_malloc_and_copy(&params_list[CONFIG_FILE], optarg, 1);
            IF_NULL_EXIT(&params_list[CONFIG_FILE], 1);
            c=-1;
            _c=1;
            break;

         case 'i':
            c=-1;
            _i=1;
            break;
            
         case 'a':
            c=-1;
            _a=1;
            break;
         
         case 'u':
            c=-1;
            _u=1;
            break;

         case 'C':
            c=PHPCGI_PATH;
            break;
            
         case 'H':
            c=PHPINI_PATH;
            break;
            
         case 'G':
            c=GUI_PATH;
            break;
            
         case 'L':
            c=LOG_PATH;
            break;
            
         case 'A':
            c=PLUGINS_PATH;
            break;
            
         case 'B':
            c=SQLITE3_DB_BUFF_PATH;
            break;
            
         case 'F':
            c=SQLITE3_DB_BUFF_NAME;
            break;

         case 'D':
            c=MYSQL_DB_SERVER;
            break;

         case 'P':
            c=MYSQL_DB_PORT;
            break;

         case 'N':
            c=MYSQL_DATABASE;
            break;

         case 'U':
            c=MYSQL_USER;
            break;

         case 'W':
            c=MYSQL_PASSWD;
            break;

         case 'V':
            c=VENDOR_ID;
            break;
            
         case 'E':
            c=DEVICE_ID;
            break;

         case 'S':
            c=INSTANCE_ID;
            break;
      }

      if(c>'!') // parametre non attendu trouvé (! = premier caractère imprimable).
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : Paramètre \"%s\" inconnu.\n",ERROR_STR,__func__,optarg);
         exit(1);
      }
      
      if(c!=-1 && c!=0)
      {
         if(c!=MEA_PATH && c!=LOG_PATH && c!=SQLITE3_DB_PARAM_PATH)
            _o=1;
         string_free_malloc_and_copy(&params_list[c], optarg, 1);
      }
   }

   
   if((_i+_a+_u)>1)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : -i, -a et -u incompatible\n",ERROR_STR,__func__);
      exit(1);
   }
   
   if(!_d) // si pas de db en parametre on construit un chemin vers le nom "théorique" de la db
   {
      params_list[SQLITE3_DB_PARAM_PATH]=(char *)malloc(strlen(params_list[MEA_PATH])+1 + 17); // lenght("/var/db/params.db") = 17
      sprintf(params_list[SQLITE3_DB_PARAM_PATH],"%s/var/db/params.db",params_list[MEA_PATH]);
   }
   
   if(_i)
   {
      initMeaEdomus(0, params_list);
      exit(0);
   }
   
   if(_a)
   {
      initMeaEdomus(1, params_list);
      exit(0);
   }
   
   if(_u)
   {
      // à faire
      updateMeaEdomus(params_list);
      exit(0);
   }

   if(_o)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : options complémentaires uniquement utilisable avec -i, -a et -u\n",ERROR_STR,__func__);
      exit(1);
   }
   
   int16_t cause;
   if(checkParamsDb(params_list[SQLITE3_DB_PARAM_PATH], &cause))
   {
      exit(1);
   }

      
   // ouverture de la base de paramétrage
   ret = sqlite3_open_v2(params_list[SQLITE3_DB_PARAM_PATH], &sqlite3_param_db, SQLITE_OPEN_READWRITE, NULL);
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
      sqlite3_close(sqlite3_param_db);
      
      exit(1);
   }

   
   // initialisation gestions des signaux (arrêt de l'appli et réinitialisation
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);
   

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
   
   ret=tomysqldb_init(myd,params_list[MYSQL_DB_SERVER], params_list[MYSQL_DATABASE], params_list[MYSQL_USER], params_list[MYSQL_PASSWD], params_list[SQLITE3_DB_BUFF_PATH]);
   if(ret==-1)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : impossible d'initialiser la gestion de la base de données.\n",ERROR_STR,__func__);
//      exit(1);
   }
#else
   VERBOSE(9) fprintf(stderr,"%s  (%s) : dbServer not started.\n",INFO_STR,__func__);
#endif
   
   
   // initialisation du serveur de plugin python
   pythonPluginServer_thread=pythonPluginServer(NULL);
   if(pythonPluginServer_thread==NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start Python Plugin Server.\n",ERROR_STR,__func__);
      sqlite3_close(sqlite3_param_db);

      exit(1);
   }
   
   
   //
   // initialisation du serveur HTTP
   //GUI_PATH
   if(params_list[PHPCGI_PATH] && params_list[PHPINI_PATH] && params_list[GUI_PATH] && params_list[SQLITE3_DB_PARAM_PATH])
   {
      phpcgibin=(char *)malloc(strlen(params_list[PHPCGI_PATH])+10); // 9 = strlen("/cgi-bin") + 1
      sprintf(phpcgibin, "%s/php-cgi",params_list[PHPCGI_PATH]);

      if(create_configs_php(params_list[GUI_PATH], params_list[SQLITE3_DB_PARAM_PATH], params_list[LOG_PATH])==0)
         httpServer(8083, params_list[GUI_PATH], phpcgibin, params_list[PHPINI_PATH]);
      else
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : can't start gui Server.\n",ERROR_STR,__func__);
         // on continu sans ihm
      }
   }
   else
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start gui Server.\n",ERROR_STR,__func__);
      // on continu sans ihm
   }
   
   
   //
   // initialisation des interfaces
   //
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
      sqlite3_close(sqlite3_param_db);
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
      {
         VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_finalize(stmt);
         sqlite3_close(sqlite3_param_db);
         exit(1);
      }
   }
   
   sqlite3_close(sqlite3_param_db);
   
   
   // initialisation du serveur xPL
   xPLServer_thread=xPLServer(interfaces);
   if(xPLServer_thread==NULL)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start xpl server.\n",ERROR_STR,__func__);
      exit(1);
   }
   
   
   DEBUG_SECTION fprintf(stderr,"MEA-EDOMUS %s starded\n",__MEA_EDOMUS_VERSION__);


   // boucle sans fin.
   while(1)
   {
      sleep(5);
   }
}