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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/wait.h>

#include <sqlite3.h>

#include "globals.h"
#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
#include "queue.h"
#include "string_utils.h"

#include "init.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"
#include "httpServer.h"
#include "automatorServer.h"

#include "monitoringServer.h"

tomysqldb_md_t *myd;                       /*!< descripteur mysql. Variable globale car doit être accessible par les gestionnaires de signaux. */
queue_t *interfaces;                       /*!< liste (file) des interfaces. Variable globale car doit être accessible par les gestionnaires de signaux. */
sqlite3 *sqlite3_param_db;                 /*!< descripteur pour la base sqlite de paramétrage. Variable globale car doit être accessible par les gestionnaires de signaux. */
pthread_t *xPLServer_thread=NULL;          /*!< Adresse du thread du serveur xPL. Variable globale car doit être accessible par les gestionnaires de signaux.*/
pthread_t *pythonPluginServer_thread=NULL; /*!< Adresse du thread Python. Variable globale car doit être accessible par les gestionnaires de signaux.*/
pthread_t *monitoringServer_thread=NULL;   /*!< Adresse du thread de surveillance interne. Variable globale car doit être accessible par les gestionnaires de signaux.*/

char *params_names[MAX_LIST_SIZE];          /*!< liste des noms (chaines) de paramètres dans la base sqlite3 de paramétrage.*/
char *params_list[MAX_LIST_SIZE];          /*!< liste des valeurs de paramètres.*/

pid_t automator_pid = 0;


tomysqldb_md_t *get_myd()
{
   return myd;
}


queue_t * get_interfaces()
{
   return interfaces;
}


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
      "  --nodatabase, -b    : le gestionnaire de base de données n'est pas lancé (pas d'historisation des données",
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
      "  --phpsessionspath, -s",
      "                      (défaut : basepath/log/sessions ou /tmp si basepath=/usr)"
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
      "  --nodejspath, -j    (défaut : /usr/bin/nodejs)",
      "  --nodejssocketioport, -J",
      "                      (défaut : 8000)",
      "  --nodejdataport, -k",
      "                      (défaut : 5600)",
      "",
      NULL
   };
   
   if(!cmd)
      cmd="mea-edomus";
   printf("\nusage : %s [options de lancememnt] [options d'intialisation]\n", cmd);
   for(int16_t i=0;usage_text[i];i++)
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
   param_names[PHPSESSIONS_PATH]     = "PHPSESSIONSPATH";
   param_names[NODEJS_PATH]          = "NODEJSPATH";
   param_names[NODEJSIOSOCKET_PORT]  = "NODEJSSOCKETIOPORT";
   param_names[NODEJSDATA_PORT]      = "NODEJSDATAPORT";
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
   int ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL); // sqlite function need int
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      return -1;
   }
   
   while (1)
   {
      int s = sqlite3_step (stmt); // sqlite function need int
      if (s == SQLITE_ROW)
      {
         // uint32_t id = sqlite3_column_int(stmt, 0);
         char *key = (char *)sqlite3_column_text(stmt, 1);
         char *value = (char *)sqlite3_column_text(stmt, 2);
         // char *complement = (char *)sqlite3_column_text(stmt, 3);
         
         mea_strtoupper(key);
         for(int16_t i=0;i<MAX_LIST_SIZE;i++)
         {
            if(params_names[i] && strcmp(params_names[i],key)==0)
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


void stop_all_services_and_exit()
{
   if(xPLServer_thread)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping xPLServer... ",INFO_STR,__func__);
      stop_xPLServer();
      VERBOSE(9) fprintf(stderr,"done\n");
   }

   if(interfaces)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping interfaces...\n",INFO_STR,__func__);
      stop_interfaces(interfaces);
      VERBOSE(9) fprintf(stderr,"%s  (%s) : done\n",INFO_STR,__func__);
   }
   
   if(pythonPluginServer_thread)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping pythonPluginServer... ",INFO_STR,__func__);
      stop_pythonPluginServer();
      VERBOSE(9) fprintf(stderr,"done\n");
   }

   if(monitoringServer_thread)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping monitoringServer... ",INFO_STR,__func__);
      stop_monitoringServer();
      VERBOSE(9) fprintf(stderr,"done\n");
   }

   if(myd)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping dbServer... ",INFO_STR,__func__);
      stop_dbServer(myd);
      VERBOSE(9) fprintf(stderr,"done\n");
   }
   
   if(automator_pid>0)
   {
      int status;
      
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping automatorServer... ",INFO_STR,__func__);
      kill(automator_pid, SIGTERM);
      waitpid(automator_pid, &status, 0);
      VERBOSE(9) fprintf(stderr,"done\n");
   }
   
   for(int16_t i=0;i<MAX_LIST_SIZE;i++)
   {
      if(params_list[i])
      {
        free(params_list[i]);
        params_list[i]=NULL;
      }
   }
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : mea-edomus done ...\n",INFO_STR,__func__);

   exit(0);
}


static void _signal_STOP(int signal_number)
/**
 * \brief     Traitement des signaux d'arrêt (SIGINT, SIGQUIT, SIGTERM)
 * \details   L'arrêt de l'application se déclanche par l'émission d'un signal d'arrêt. A la réception de l'un de ces signaux
 *            Les différents process de mea_edomus doivent être arrêtés.
 * \param     signal_number  numéro du signal (pas utilisé mais nécessaire pour la déclaration du handler).
 */
{
   VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping mea-edomus requested (signal = %d).\n",INFO_STR,__func__,signal_number);
   stop_all_services_and_exit();
}


static void _signal_HUP(int signal_number)
/**
 * \brief     Traitement des signaux HUP
 * \details   Un signal HUP peut être est émis par les threads de gestion des interfaces lorsqu'ils détectent une anomalie bloquante.
 *            Le gestionnaire de signal, lorsqu'il est appelé, doit déterminer quelle interface à émis le signal et forcer un arrêt/relance de cette l'interface.
 * \param     signal_number  numéro du signal (pas utilisé mais nécessaire pour la déclaration du handler).
 */
{
   VERBOSE(9) fprintf(stderr,"%s  (%s) : communication error signal (signal = %d).\n", INFO_STR, __func__, signal_number);
  
   // on cherche qui est à l'origine du signal et on le relance
   restart_down_interfaces(interfaces, sqlite3_param_db, myd);
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
   int ret; // sqlite function need int
   sqlite3 *sqlite3_param_db; // descritpteur SQLITE
   
   // toutes les options possibles
   static struct option long_options[] = {
      {"init",              no_argument,       0,  'i'                  },
      {"autoinit",          no_argument,       0,  'a'                  },
      {"update",            no_argument,       0,  'u'                  },
      {"basepath",          required_argument, 0,  MEA_PATH             }, // 'p'
      {"paramsdb",          required_argument, 0,  SQLITE3_DB_PARAM_PATH}, // 'd'
//      {"configfile",      required_argument, 0,  CONFIG_FILE          }, // 'c'
      {"phpcgipath",        required_argument, 0,  PHPCGI_PATH          }, // 'C'
      {"phpinipath",        required_argument, 0,  PHPINI_PATH          }, // 'H'
      {"phpsessionspath",   required_argument, 0,  PHPSESSIONS_PATH     }, // 's'
      {"guipath",           required_argument, 0,  GUI_PATH             }, // 'G'
      {"logpath",           required_argument, 0,  LOG_PATH             }, // 'L'
      {"pluginspath",       required_argument, 0,  PLUGINS_PATH         }, // 'A'
      {"bufferdbpath",      required_argument, 0,  SQLITE3_DB_BUFF_PATH }, // 'B'
      {"dbserver",          required_argument, 0,  MYSQL_DB_SERVER      }, // 'D'
      {"dbport",            required_argument, 0,  MYSQL_DB_PORT        }, // 'P'
      {"dbname",            required_argument, 0,  MYSQL_DATABASE       }, // 'N'
      {"dbuser",            required_argument, 0,  MYSQL_USER           }, // 'U'
      {"dbpassword",        required_argument, 0,  MYSQL_PASSWD         }, // 'W'
      {"vendorid",          required_argument, 0,  VENDOR_ID            }, // 'V'
      {"deviceid",          required_argument, 0,  DEVICE_ID            }, // 'E'
      {"instanceid",        required_argument, 0,  INSTANCE_ID          }, // 'S'
      {"verboselevel",      required_argument, 0,  VERBOSELEVEL         }, // 'v'
      {"nodejspath",        required_argument, 0,  NODEJS_PATH          }, // 'j'
      {"nodejssocketioport",required_argument, 0,  NODEJSIOSOCKET_PORT  }, // 'J'
      {"nodejsdataport",    required_argument, 0,  NODEJSDATA_PORT      }, // 'k'
      {"guiport",           required_argument, 0,  GUIPORT              }, // 'g'
      {"nodatabase",        no_argument,       0,  'b'                  }, // 'b'
      {"help",              no_argument,       0,  'h'                  }, // 'h'
      {0,                   0,                 0,  0                    }
   };

#define __DEBUG_ON__ 1
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
   init_param_names(params_names);
   // initialisation de la liste des parametres à NULL
   for(int16_t i=0;i<MAX_LIST_SIZE;i++)
      params_list[i]=NULL;

   /*
   // chemin "théorique" de l'installation mea-domus (si les recommendations ont été respectées) => ne marche pas sur linux
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
   */
   string_free_malloc_and_copy(&params_list[MEA_PATH], "/usr/local/mea-edomus", 1);
   IF_NULL_EXIT(&params_list[MEA_PATH], 1);

   //
   // récupération des paramètres de la ligne de commande
   //
   int16_t _d=0, _i=0, _a=0, /* _c ,*/ _u=0, _o=0, _v=-1, _b=0;

   int option_index = 0; // getopt_long function need int
   int c; // getopt_long function need int
//   while ((c = getopt_long(argc, (char * const *)argv, "bhiaup:d:c:C:H:s:G:L:A:B:D:P:N:U:W:V:E:S:v:g:j:J:k", long_options, &option_index)) != -1)
   while ((c = getopt_long(argc, (char * const *)argv, "bhiaup:d:C:H:s:G:L:A:B:D:P:N:U:W:V:E:S:v:g:j:J:k", long_options, &option_index)) != -1)
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
            c=-1;
            break;
            
         case 'b':
            c=-1;
            _b=1;
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
            
         case 's':
            c=PHPSESSIONS_PATH;
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

         case 'j':
            c=NODEJS_PATH;
            break;

         case 'J':
            c=NODEJSIOSOCKET_PORT;
            break;

         case 'k':
            c=NODEJSDATA_PORT;
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
      VERBOSE(1) fprintf(stderr,"%s (%s) : --init (-i), --autoinit (-a), et --update (-u) incompatible\n",ERROR_STR,__func__);
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
   
/*
   if(_c)
   {
      // chargement des options si non présente dans params_list
   }
*/   
   if(_i || _a)
   {
      initMeaEdomus(_a, params_list, params_names);
      exit(0);
   }
   
   if(_u)
   {
      // à faire
      updateMeaEdomus(params_list, params_names);
      exit(0);
   }

   if(_o)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : options complémentaires uniquement utilisable avec --init (-i), --autoinit (-a), et --update (-u)\n", ERROR_STR, __func__);
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
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      exit(1);
   }

   
   // lecture de tous les paramètres de l'application
   ret = read_all_application_parameters(sqlite3_param_db);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't load parameters\n",ERROR_STR,__func__);
      sqlite3_close(sqlite3_param_db);
      
      exit(1);
   }


   //
   // strout et stderr vers fichier log
   //
   char log_file[255];
   int16_t n;


   if(!params_list[LOG_PATH] || !strlen(params_list[LOG_PATH]))
      params_list[LOG_PATH]="/var/log";
   n=snprintf(log_file,sizeof(log_file),"%s/mea-edomus.log", params_list[LOG_PATH]);
   if(n<0 || n==sizeof(log_file))
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      exit(1);
   }
   int fd=open(log_file, O_CREAT | O_APPEND | O_RDWR,  S_IWUSR | S_IRUSR);
   if(fd<0)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't open log file - \n",ERROR_STR,__func__);
      perror("");
      exit(1);
   }
   
   dup2(fd, 1);
   dup2(fd, 2);
   close(fd);

   //   
   // demarrage du processus de l'automate
   //
   //automator_pid = start_automatorServer(params_list[SQLITE3_DB_PARAM_PATH]);

   //
   // initialisation gestions des signaux (arrêt de l'appli et réinitialisation
   //
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);


   // démarrage des "services" (les services "majeurs" arrêtent tout (exit) si non démarrage
   if(!_b)
   {
      myd=start_dbServer(params_list, sqlite3_param_db); // initialisation de la communication avec la base MySQL
      if(!myd)
      stop_all_services_and_exit();
   }

   pythonPluginServer_thread=start_pythonPluginServer(params_list, sqlite3_param_db); // initialisation du serveur de plugin python
   if(!pythonPluginServer_thread)
      stop_all_services_and_exit();

   interfaces=start_interfaces(params_list, sqlite3_param_db, myd); // démarrage des interfaces

   xPLServer_thread=start_xPLServer(params_list, interfaces, sqlite3_param_db); // initialisation du serveur xPL
   if(!xPLServer_thread)
      stop_all_services_and_exit();

   start_httpServer(params_list, interfaces); // initialisation du serveur HTTP

   // monitoringServer_thread=start_monitoringServer("/data/rec/mea-edomus/0.1alpha3/bin/node", "/data/rec/mea-edomus/0.1alpha3/lib/mea-gui/nodeJS/server/server.js", 8000, 5600, "/data/rec/mea-edomus/0.1alpha3/var/log/mea-edomus.log");
   monitoringServer_thread=start_monitoringServer(params_list);

   if(!monitoringServer_thread)
      stop_all_services_and_exit();

   DEBUG_SECTION fprintf(stderr,"MEA-EDOMUS %s starded\n",__MEA_EDOMUS_VERSION__);

   // boucle sans fin.
   while(1)
   {
      sleep(5);
   }
}

