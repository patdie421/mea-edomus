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
//sqlite3 *sqlite3_param_db; // descritpteur SQLITE
queue_t *interfaces; // globale car doit être accessible par les gestionnaires de signaux
sqlite3 *sqlite3_param_db; // globale car doit être accessible par les gestionnaires de signaux

pthread_t *xPLServer_thread=NULL;
pthread_t *pythonPluginServer_thread=NULL;
// pthread_t *counters_thread=NULL;

char *param_names[MAX_LIST_SIZE]; // liste des noms de paramètres dans la base
char *params_list[MAX_LIST_SIZE]; // liste des valeurs de paramètres


void usage(char *cmd)
/**
 * \brief     Affiche les "usages" de mea-edomus
 * \param     cmd    nom d'appel de mea-edomus (argv[0])
 */
{
   char *usage_text[]={
    //12345678901234567890123456789012345678901234567890123456789012345678901234567890
      "",
      "gestion de capteurs/actionneurs (xbee, arduino, ...) commandés par des messages",
      "xPL",
      "",
      "Options de lancement de l'application : ",
      "  --basepath, -p      : chemin de l'installation",
      "  --paramsdb, -d      : chemin et nom la base de paramétrage",
//      "  --config, -c        : fichier de configuration (mea-edomus.ini peut contenir contient basepath, paramsdb et guiport). Les options de la ligne de commandes sont prioritaires sur le fichier de config.",
      "  --guiport, -g       : port tcp de l'ihm",
      "  --verboselevel, -v  : niveau de détail des messages d'erreur (1 à 9)",
      "  --help, -h          : ce message",
      "",
      "Options d'initialisation et de modification : ",
      "  --init     (-i)     : initialisation interactive.",
      "  --autoinit (-a)     : intialisation automatique.",
      "  --update   (-u)     : modification d'un ou plusieurs parametre de la base.",
      "Remarque : --init, --autoinit et --update sont incompatibles.",
      "",
      "Parametres pour --init, --autoinit ou --update uniquement : ",
      "  --basepath, -p      (défaut : chemin de l'exécutable (moins 'bin'.",
      "                       Ex : si /usr/bin/mea-edomus => basepath=/usr)",
      "  --paramsdb, -d      (défaut : basepath/var/db/params.db ou ",
      "                                /var/db/mea-params.db si basepath=/usr)",
      "  --phpcgipath, -C    (défaut : basepath/bin)",
      "  --phpinipath, -H    (défaut : basepath/etc ou /etc si basepath=/usr)",
      "  --guipath, -G       (défaut : basepath/lib/mea-gui)",
      "  --logpath, -L       (défaut : basepath/var/log ou /var/log si basepath=/usr)",
      "  --pluginspath, -A   (défaut : basepath/lib/mea-plugins)",
      "  --bufferdb, -B      (défaut : basepath/var/db/queries.db ou",
      "                                /var/db/mea-queries.db si basepath=/usr)",
      "  --dbserver, -D      (défaut : 127.0.0.1)",
      "  --dbport, -P        (défaut : 3306)",
      "  --dbname, -N        (défaut : meaedomusdb)",
      "  --dbuser, -U        (défaut : meaedomus)",
      "  --dbpassword, -W    (défaut : meaedomus)",
      "  --vendorid, -V      (défaut : mea)",
      "  --deviceid, -E      (défaut : edomus)",
      "  --instanceid, -S    (défaut : home)",
      "",
      NULL
   };
   
   if(!cmd)
      cmd="mea-edomus";
   printf("\nusage : %s [options de lancememnt] [options d'intialisation]\n", cmd);
   for(int16_t i;usage_text[i];i++)
      printf("%s\n",usage_text[i]);
}


void init_param_names(char *param_names[])
/**
 * \brief     initialisation de la table de correspondance nom (chaine, dans sqlite) avec un ID de paramètre numérique (index du tableau)
 * \param     table à initialiser.
 */
{
   param_names[0]                    = NULL;
   param_names[MEA_PATH]             = NULL;
   param_names[SQLITE3_DB_PARAM_PATH]= NULL;
   param_names[CONFIG_FILE]          = NULL;
   param_names[PHPCGI_PATH]          = "PHPCGIPATH";
   param_names[PHPINI_PATH]          = "PHPINIPATH";
   param_names[GUI_PATH]             = "GUIPATH";
   param_names[LOG_PATH]             = "LOGPATH";
   param_names[PLUGINS_PATH]         = "PLUGINPATH";
   param_names[SQLITE3_DB_BUFF_PATH] = "BUFFERDB";
   param_names[MYSQL_DB_SERVER]      = "DBSERVER";
   param_names[MYSQL_DB_PORT]        = "DBPORT";
   param_names[MYSQL_DATABASE]       = "DATABASE";
   param_names[MYSQL_USER]           = "USER";
   param_names[MYSQL_PASSWD]         = "PASSWORD";
   param_names[VENDOR_ID]            = "VENDORID";
   param_names[DEVICE_ID]            = "DEVICEID";
   param_names[INSTANCE_ID]          = "INSTANCEID";
   param_names[VERBOSELEVEL]         = "VERBOSELEVEL";
   param_names[GUIPORT]              = "GUIPORT";
}


int16_t set_xpl_address(char **params_list)
/**
 * \brief     initialise les données pour l'adresse xPL
 * \details   positionne vendorID, deviceID et instanceID pour xPLServer
 * \param     params_liste  liste des parametres.
 * \return   -1 en cas d'erreur, 0 sinon
 */
{
   set_xPL_vendorID(params_list[VENDOR_ID]);
   set_xPL_deviceID(params_list[DEVICE_ID]);
   set_xPL_instanceID(params_list[INSTANCE_ID]);
   
   return 0;
}


int16_t read_all_application_parameters(sqlite3 *sqlite3_param_db)
/**
 * \brief     Chargement de tous les paramètres nécessaires au démarrage de l'application depuis la base de paramètres
 * \param     sqlite3_param_db descripteur initialisé (ouvert) d'accès à la base.
 * \return   -1 en cas d'erreur, 0 sinon
 */
{
   sqlite3_stmt * stmt;
   
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
         for(int i=0;i<MAX_LIST_SIZE;i++)
         {
            if(param_names[i] && strcmp(param_names[i],key)==0)
            {
               string_free_malloc_and_copy(&params_list[i], value, 1);
               break;
            }
         }
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
 * \details   L'arrêt de l'application se déclanche par l'émission d'un signal d'arrêt. A la réception de l'un de ces signaux
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
 * \details   Un signal HUP peut être est émis par les threads de gestion des interfaces lorsqu'ils détectent une anomalie bloquante.
 *            Le gestionnaire de signal, lorsqu'il est appelé, doit déterminer quelle interface à émis le signal et forcer un arrêt/relance de cette l'interface.
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
 * \brief     Point d'entrée du mea-edomus
 * \details   Intitialisation des structures de données et lancement des différents "process" (threads) de l'application
 * \param     argc   paramètres de lancement de l'application.
 * \param     argv   nombre de paramètres.
 * \return    1 en cas d'erreur, 0 sinon
 */
{
   int c;
   int ret;
   char *buff;
   sqlite3 *sqlite3_param_db; // descritpteur SQLITE
   
   // toutes les options possibles
   static struct option long_options[] = {
      {"init",        no_argument,       0,  'i'                  },
      {"autoinit",    no_argument,       0,  'a'                  },
      {"update",      no_argument,       0,  'u'                  },
      {"basepath",    required_argument, 0,  MEA_PATH             }, // 'p'
      {"paramsdb",    required_argument, 0,  SQLITE3_DB_PARAM_PATH}, // 'd'
//      {"configfile",  required_argument, 0,  CONFIG_FILE          }, // 'c'
      {"phpcgipath",  required_argument, 0,  PHPCGI_PATH          }, // 'C'
      {"phpinipath",  required_argument, 0,  PHPINI_PATH          }, // 'H'
      {"guipath",     required_argument, 0,  GUI_PATH             }, // 'G'
      {"logpath",     required_argument, 0,  LOG_PATH             }, // 'L'
      {"pluginspath", required_argument, 0,  PLUGINS_PATH         }, // 'A'
      {"bufferdbpath",required_argument, 0,  SQLITE3_DB_BUFF_PATH }, // 'B'
      {"dbserver",    required_argument, 0,  MYSQL_DB_SERVER      }, // 'D'
      {"dbport",      required_argument, 0,  MYSQL_DB_PORT        }, // 'P'
      {"dbname",      required_argument, 0,  MYSQL_DATABASE       }, // 'N'
      {"dbuser",      required_argument, 0,  MYSQL_USER           }, // 'U'
      {"dbpassword",  required_argument, 0,  MYSQL_PASSWD         }, // 'W'
      {"vendorid",    required_argument, 0,  VENDOR_ID            }, // 'V'
      {"deviceid",    required_argument, 0,  DEVICE_ID            }, // 'E'
      {"instanceid",  required_argument, 0,  INSTANCE_ID          }, // 'S'
      {"verboselevel",required_argument, 0,  VERBOSELEVEL         }, // 'v'
      {"guiport",     required_argument, 0,  GUIPORT              }, // 'g'
      {"help",        no_argument,       0,  'h'                  }, // 'h'
      {0,             0,                 0,  0                    }
   };

#ifdef __DEBUG_ON__
   debug_on();
   set_verbose_level(9);
#else
   debug_off();
#endif
   
   DEBUG_SECTION fprintf(stderr,"Starting MEA-EDOMUS %s\n",__MEA_EDOMUS_VERSION__);

   //
   // initialisation
   //
   sqlite3_config(SQLITE_CONFIG_SERIALIZED); // pour le multithreading
   // initialisation des noms des parametres dans la base
   init_param_names(param_names);
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

   //
   // récupération des paramètres de la ligne de commande
   //
   int _d=0, _i=0, _a=0, /* _c ,*/ _u=0, _o=0, _v=-1;
   int option_index = 0;
   
//   while ((c = getopt_long(argc, (char * const *)argv, "hiaup:d:c:C:H:G:L:A:B:D:P:N:U:W:V:E:S:v:g:", long_options, &option_index)) != -1)
   while ((c = getopt_long(argc, (char * const *)argv, "hiaup:d:C:H:G:L:A:B:D:P:N:U:W:V:E:S:v:g:", long_options, &option_index)) != -1)
   {
      switch (c)
      {
         case 'h':
            usage((char *)argv[0]);
            exit(0);
            break;
            
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
/*
         case 'c': // fichier de configuration (mea-edomus.ini qui contient basepath et paramsdb)
         case CONFIG_FILE:
            c=CONFIG_FILE;
            _c=1;
            break;
*/
         case 'v':
         case VERBOSELEVEL:
            _v=atoi(optarg);
            c=VERBOSELEVEL;
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
            
         case 'g':
            c=GUIPORT;
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
         usage((char *)argv[0]);
         exit(1);
      }
      
      if(c!=-1 && c!=0)
      {
         if(c!=MEA_PATH)
            _o=1;
         string_free_malloc_and_copy(&params_list[c], optarg, 1);
         IF_NULL_EXIT(&params_list[c], 1);
      }
   }

   //
   // Contrôle des parametres
   //
   if((_i+_a+_u)>1)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : -i, -a et -u incompatible\n",ERROR_STR,__func__);
      usage((char *)argv[0]);
      exit(1);
   }
   
   if(_v > 0 && _v < 10)
         set_verbose_level(_v);

   if(!_d) // si pas de db en parametre on construit un chemin vers le nom "théorique" de la db
   {
      params_list[SQLITE3_DB_PARAM_PATH]=(char *)malloc(strlen(params_list[MEA_PATH])+1 + 17); // lenght("/var/db/params.db") = 17
      sprintf(params_list[SQLITE3_DB_PARAM_PATH],"%s/var/db/params.db",params_list[MEA_PATH]);
   }
   
   //
   // traitement des paramateres
   //
/*
   if(_c)
   {
      // chargement des options si non présente dans params_list
   }
*/   
   if(_i)
   {
      initMeaEdomus(0, params_list, param_names);
      exit(0);
   }
   
   if(_a)
   {
      initMeaEdomus(1, params_list, param_names);
      exit(0);
   }
   
   if(_u)
   {
      // à faire
      updateMeaEdomus(params_list, param_names);
      exit(0);
   }

   if(_o)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : options complémentaires uniquement utilisable avec --init (-i), --autoinit (-a), et --update (-u)\n",ERROR_STR,__func__);
      usage((char *)argv[0]);
      exit(1);
   }
   
   //
   // Contrôle et ouverture de la base de paramétrage
   //
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

   //   
   // initialisation gestions des signaux (arrêt de l'appli et réinitialisation
   //
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);
   

   //
   // initialisation de la communication avec la base MySQL
   //
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
      VERBOSE(1) fprintf(stderr,"%s (%s) : Can not init data base communication.\n",ERROR_STR,__func__);
//      exit(1);
   }
#else
   VERBOSE(9) fprintf(stderr,"%s  (%s) : dbServer not started.\n",INFO_STR,__func__);
#endif
   
   //
   // initialisation du serveur de plugin python
   //
   if(params_list[PLUGINS_PATH])
   {
      setPythonPluginPath(params_list[PLUGINS_PATH]);
      pythonPluginServer_thread=pythonPluginServer(NULL);
      if(pythonPluginServer_thread==NULL)
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : can't start Python Plugin Server (thread error).\n",ERROR_STR,__func__);
         sqlite3_close(sqlite3_param_db);

         exit(1);
      }
   }
   else
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start Python Plugin Server (incorrect plugin path).\n",ERROR_STR,__func__);
      sqlite3_close(sqlite3_param_db);

      exit(1);
   }
   
   
   //
   // initialisation du serveur HTTP
   //
   char *phpcgibin=NULL;
   if(params_list[PHPCGI_PATH] && params_list[PHPINI_PATH] && params_list[GUI_PATH] && params_list[SQLITE3_DB_PARAM_PATH])
   {
      phpcgibin=(char *)malloc(strlen(params_list[PHPCGI_PATH])+10); // 9 = strlen("/cgi-bin") + 1
      sprintf(phpcgibin, "%s/php-cgi",params_list[PHPCGI_PATH]);

      long guiport;
      if(params_list[GUIPORT][0])
      {
         char *end;
         guiport=strtol(params_list[GUIPORT],&end,10);
         if(*end!=0 || errno==ERANGE)
         {
            VERBOSE(5) fprintf(stderr,"%s (%s) : GUI port (%s), not a number, use 8083.\n",INFO_STR,__func__,params_list[GUIPORT]);
            guiport=8083;
         }
      }
      else
      {
         VERBOSE(5) fprintf(stderr,"%s (%s) : can't get GUI port, use 8083.\n",INFO_STR,__func__);
         guiport=8083;
      }
      
      if(create_configs_php(params_list[GUI_PATH], params_list[SQLITE3_DB_PARAM_PATH], params_list[LOG_PATH])==0)
         httpServer(guiport, params_list[GUI_PATH], phpcgibin, params_list[PHPINI_PATH]);
      else
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : can't start GUI Server (can't create configs.php).\n",ERROR_STR,__func__);
         // on continu sans ihm
      }
   }
   else
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start GUI Server (parameters errors).\n",ERROR_STR,__func__);
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
   
   //
   // initialisation du serveur xPL
   //
   if(!set_xpl_address(params_list))
   {
      xPLServer_thread=xPLServer(interfaces);
      if(xPLServer_thread==NULL)
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : can't start xpl server.\n",ERROR_STR,__func__);
         exit(1);
      }
   }
   else
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : can't start xpl server (incorrect xPL address).\n",ERROR_STR,__func__);
      exit(1);
   }
   
   DEBUG_SECTION fprintf(stderr,"MEA-EDOMUS %s starded\n",__MEA_EDOMUS_VERSION__);

   // boucle sans fin.
   while(1)
   {
      sleep(5);
   }
}