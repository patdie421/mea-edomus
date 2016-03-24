//
//  main.c
//
//  Created by Patrice DIETSCH on 08/07/12.
//  Copyright (c) 2012 -. All rights reserved.
//
#include <Python.h>

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
#include <execinfo.h>
#include <sys/resource.h>

#include "globals.h"
#include "macros.h"
#include "consts.h"

#include "tokens.h"
#include "tokens_da.h"

#include "mea_verbose.h"
#include "mea_queue.h"
#include "mea_string_utils.h"
#include "mea_sockets_utils.h"
#include "mea_timer.h"

#include "parameters_utils.h"

#include "init.h"

#include "processManager.h"

#include "datetimeServer.h"
#include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"
#include "guiServer.h"
#include "logServer.h"
#include "automatorServer.h"
#include "automator.h"
#include "nodejsServer.h"
#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#include "notify.h"


int xplServer_monitoring_id=-1;
int httpServer_monitoring_id=-1;
int logServer_monitoring_id=-1;
int pythonPluginServer_monitoring_id=-1;
int dbServer_monitoring_id=-1;
int nodejsServer_monitoring_id=-1;
int automatorServer_monitoring_id=-1;
int log_rotation_id=-1;

mea_queue_t *interfaces=NULL;                  /*!< liste (file) des interfaces. Variable globale car doit être accessible par les gestionnaires de signaux. */
sqlite3 *sqlite3_param_db=NULL;            /*!< descripteur pour la base sqlite de paramétrage. */
pthread_t *monitoringServer_thread=NULL;   /*!< Adresse du thread de surveillance interne. Variable globale car doit être accessible par les gestionnaires de signaux.*/

long sigsegv_indicator = 0;

uint16_t params_db_version=0;
char *params_names[MAX_LIST_SIZE];          /*!< liste des noms (chaines) de paramètres dans la base sqlite3 de paramétrage.*/
char *params_list[MAX_LIST_SIZE];          /*!< liste des valeurs de paramètres.*/

pid_t automator_pid = 0;
int main_monitoring_id = -1;


sqlite3 *get_sqlite3_param_db()
{
   return sqlite3_param_db;
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
      "  --nodatabase, -b    : le gestionnaire de base de données n'est pas lancé (pas",
      "                        d'historisation des données",
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
      "  --basepath, -p      (défaut : /usr/local/mea-edomus)",
      "  --paramsdb, -d      (défaut : basepath/var/db/params.db ou ",
      "                                /var/db/mea-params.db si basepath=/usr)",
#ifdef __linux__
      "  --interface, -I     (défaut : eth0)",
#else
      "  --interface, -I     (défaut : en0)",
#endif
      "  --phpcgipath, -C    (défaut : basepath/bin)",
      "  --phpsessionspath, -s",
      "                      (défaut : basepath/log/sessions ou /tmp si basepath=/usr)"
      "  --phpinipath, -H    (défaut : basepath/etc ou /etc si basepath=/usr)",
      "  --guipath, -G       (défaut : basepath/lib/mea-gui)",
      "  --logpath, -L       (défaut : basepath/var/log ou /var/log si basepath=/usr)",
      "  --pluginspath, -A   (défaut : basepath/lib/mea-plugins)",
      "  --rulesfilespath, -R(défaut : basepath/lib/rules)",
      "  --rulesfile, -r     (défaut : basepath/lib/rules/automator.rules)",
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
      "  --nodejsdataport, -k",
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


int logfile_rotation_job(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char *log_file_name=(char *)data;

   if(log_file_name)
   {
      mea_log_printf("%s (%s) : log file rotation job start\n", INFO_STR, __func__);

      if(mea_rotate_open_log_file(log_file_name, 6)<0)
      {
         VERBOSE(2) fprintf (MEA_STDERR, "%s (%s) : can't rotate %s - ", ERROR_STR, __func__, log_file_name);
         perror("");
         return -1;
      }

      mea_log_printf("%s (%s) : log file rotation job done\n", INFO_STR, __func__);
   }
   return 0;
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
   param_names[PARAMSDBVERSION]      = "PARAMSDBVERSION";
   param_names[INTERFACE]            = "INTERFACE";
   param_names[RULES_FILE]           = "RULESFILE";
   param_names[RULES_FILES_PATH]     = "RULESFILESPATH";
   param_names[COLLECTOR_ID]         = "COLLECTORID";
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
      VERBOSE(2) fprintf (MEA_STDERR, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
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
               mea_string_free_alloc_and_copy(&params_list[i], value);
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


void clean_all_and_exit()
{
   if(xplServer_monitoring_id!=-1)
   {
      process_stop(xplServer_monitoring_id, NULL, 0);
      process_unregister(xplServer_monitoring_id);
      xplServer_monitoring_id=-1;
   }

   if(httpServer_monitoring_id!=-1)
   {
      process_stop(httpServer_monitoring_id, NULL, 0);
      process_unregister(httpServer_monitoring_id);
      httpServer_monitoring_id=-1;
   }
  
   if(interfaces)
   {
      stop_interfaces();
   }
 
   if(pythonPluginServer_monitoring_id!=-1)
   {
      process_stop(pythonPluginServer_monitoring_id, NULL, 0);
      process_unregister(pythonPluginServer_monitoring_id);
      pythonPluginServer_monitoring_id=-1;
   }

   if(dbServer_monitoring_id!=-1)
   {
      process_stop(dbServer_monitoring_id, NULL, 0);
      process_unregister(dbServer_monitoring_id);
      dbServer_monitoring_id=-1;
   }
   
   if(logServer_monitoring_id!=-1)
   {
      process_stop(logServer_monitoring_id, NULL, 0);
      process_unregister(logServer_monitoring_id);
      logServer_monitoring_id=-1;
   }
   
   if(httpServer_monitoring_id!=-1)
   {
      process_stop(httpServer_monitoring_id, NULL, 0);
      process_unregister(httpServer_monitoring_id);
      httpServer_monitoring_id=-1;
   }

   if(nodejsServer_monitoring_id!=-1)
   {
      process_stop(nodejsServer_monitoring_id, NULL, 0);
      process_unregister(nodejsServer_monitoring_id);
      nodejsServer_monitoring_id=-1;
   }
   
   if(automatorServer_monitoring_id!=-1)
   {
      process_stop(automatorServer_monitoring_id, NULL, 0);
      process_unregister(automatorServer_monitoring_id);
      automatorServer_monitoring_id=-1;
   }
   
   process_unregister(main_monitoring_id);
   
   for(int16_t i=0;i<MAX_LIST_SIZE;i++)
   {
      if(params_list[i])
      {
        free(params_list[i]);
        params_list[i]=NULL;
      }
   }
   
   parsed_parameters_clean_all(1);
   release_strings_da();
   release_tokens();

//   VERBOSE(9) mea_log_printf("%s (%s) : mea-edomus down ...\n", INFO_STR, __func__);

   exit(0);
}


static void error_handler(int signal_number)
{
/*
   void *array[10];
   char **messages;

   size_t size;

   size = backtrace(array, 50);

   // print out all the frames to stderr
   fprintf(stderr, "Error: signal %d:\n", signal_number);

   backtrace_symbols_fd(array, size, fileno(stderr));

   messages = backtrace_symbols(array, size);

   for (int i=1; i < size && messages != NULL; ++i)
      fprintf(stderr, "[backtrace]: (%d) %s\n", i, messages[i]);
   free(messages);
*/
   ++sigsegv_indicator;

   fprintf(stderr, "Error: signal %d:\n", signal_number);
   if((_xPLServer_thread_id!=NULL) && pthread_equal(*_xPLServer_thread_id, pthread_self())!=0)
   {
      fprintf(stderr, "Error: in xPLServer, try to recover\n");
      longjmp(xPLServer_JumpBuffer, 1);
   }
   else if((_automatorServer_thread_id!=NULL) && pthread_equal(*_automatorServer_thread_id, pthread_self())!=0)
   {
      fprintf(stderr, "Error: in automator.c/automatorServer.c\n");
#if DEBUGFLAG_AUTOMATOR > 0
      fprintf(stderr, "Error: in function : %s\n", _automatorServer_fn);
      if(strcmp(_automatorServer_fn, "_evalStr")==0)
      {
         fprintf(stderr, "Error: function caller : %s\n", _automatorEvalStrCaller);
         fprintf(stderr, "Error: evalStr operation : %c\n", _automatorEvalStrOperation);
         fprintf(stderr, "Error: evalStr str : %s\n", _automatorEvalStrArg);
      }
#endif
   }

   fprintf(stderr, "Error: aborting\n");
   fprintf(stderr, "Thread id : %x", (unsigned int)pthread_self());

   abort();
   exit(1);
}


static void _signal_STOP(int signal_number)
/**
 * \brief     Traitement des signaux d'arrêt (SIGINT, SIGQUIT, SIGTERM)
 * \details   L'arrêt de l'application se déclanche par l'émission d'un signal d'arrêt. A la réception de l'un de ces signaux
 *            Les différents process de mea_edomus doivent être arrêtés.
 * \param     signal_number  numéro du signal (pas utilisé mais nécessaire pour la déclaration du handler).
 */
{
   VERBOSE(9) mea_log_printf("%s (%s) : Stopping mea-edomus requested (signal = %d).\n", INFO_STR, __func__, signal_number);

   clean_all_and_exit();
}


static void _signal_SIGCHLD(int signal_number)
{
   /* Wait for all dead processes.
    * We use a non-blocking call to be sure this signal handler will not
    * block if a child was cleaned up in another part of the program. */
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
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
  
   // activation du core dump
   struct rlimit core_limit = { RLIM_INFINITY, RLIM_INFINITY };
   assert( setrlimit( RLIMIT_CORE, &core_limit ) == 0 );
   // pour changer l'endroit ou sont les cores dump et donner un nom plus explicite
   // echo '/tmp/core_%e.%p' | sudo tee /proc/sys/kernel/core_pattern

   signal(SIGSEGV, error_handler);

   // toutes les options possibles
   static struct option long_options[] = {
      {"init",              no_argument,       0,  'i'                  },
      {"autoinit",          no_argument,       0,  'a'                  },
      {"update",            no_argument,       0,  'u'                  },
      {"basepath",          required_argument, 0,  MEA_PATH             }, // 'p'
      {"paramsdb",          required_argument, 0,  SQLITE3_DB_PARAM_PATH}, // 'd'
//      {"configfile",      required_argument, 0,  CONFIG_FILE          }, // 'c'
      {"interface",         required_argument, 0,  INTERFACE            }, // 'I'
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
      {"rulesfile",         required_argument, 0,  RULES_FILE           }, // 'r'
      {"rulesfilespath",    required_argument, 0,  RULES_FILES_PATH     }, // 'R'
      {"nodejspath",        required_argument, 0,  NODEJS_PATH          }, // 'j'
      {"nodejssocketioport",required_argument, 0,  NODEJSIOSOCKET_PORT  }, // 'J'
      {"nodejsdataport",    required_argument, 0,  NODEJSDATA_PORT      }, // 'k'
      {"guiport",           required_argument, 0,  GUIPORT              }, // 'g'
      {"nodatabase",        no_argument,       0,  'b'                  }, // 'b'
      {"help",              no_argument,       0,  'h'                  }, // 'h'
      {0,                   0,                 0,  0                    }
   };

#define __MEA_DEBUG_ON__ 1
#ifdef __MEA_DEBUG_ON__
   debug_on();
   set_verbose_level(10);
#else
   debug_off();
   set_verbose_level(2);
#endif

   init_tokens();
   init_strings_da();

   parsed_parameters_init();

   //
   // initialisation
   //

   // XCODE
   // pour debugger sans être interrompu par les SIGPIPE
   // rajouter ici un breakpoint, l'éditer et rajouter une action "Command debugger" avec la ligne suivante :
   // process handle SIGPIPE -n true -p true -s false
   // cocher ensuite "Automatically continue after evaluating"
   
   sqlite3_config(SQLITE_CONFIG_SERIALIZED); // pour le multithreading
   
   // initialisation des noms des parametres
   init_param_names(params_names);
   
   // initialisation de la liste des parametres à NULL
   for(int16_t i=0;i<MAX_LIST_SIZE;i++)
      params_list[i]=NULL;
   
   mea_string_free_alloc_and_copy(&params_list[MEA_PATH], "/usr/local/mea-edomus");
   if(!params_list[MEA_PATH])
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : malloc - ", ERROR_STR,__func__);
         perror("");
      }
      clean_all_and_exit();
   }

   //
   // récupération des paramètres de la ligne de commande
   //
   int16_t _d=0, _i=0, _a=0, /* _c ,*/ _u=0, _o=0, _v=-1, _b=0;

   int option_index = 0; // getopt_long function need int
   int c; // getopt_long function need int
//   while ((c = getopt_long(argc, (char * const *)argv, "bhiaup:d:c:C:H:s:G:L:A:B:D:P:N:U:W:V:E:S:v:g:j:J:k:I:", long_options, &option_index)) != -1)
   while ((c = getopt_long(argc, (char * const *)argv, "bhiaup:d:C:H:s:G:L:A:B:D:P:N:U:W:V:E:S:v:g:j:J:k:I:", long_options, &option_index)) != -1)
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
            mea_string_free_alloc_and_copy(&params_list[SQLITE3_DB_PARAM_PATH], optarg);
            if(!params_list[SQLITE3_DB_PARAM_PATH])
            {
               VERBOSE(1) {
                  mea_log_printf("%s (%s) : malloc - ", ERROR_STR,__func__);
                  perror("");
               }
               clean_all_and_exit();
            }
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

         case 'I':
            c=INTERFACE;
            break;

         case 'r':
            c=RULES_FILE;
            break;

         case 'R':
            c=RULES_FILES_PATH;
            break;
      }

      if(c>'!') // parametre non attendu trouvé (! = premier caractère imprimable).
      {
         VERBOSE(1) mea_log_printf("%s (%s) : Paramètre \"%s\" inconnu.\n",ERROR_STR,__func__,optarg);
         usage((char *)argv[0]);
         clean_all_and_exit();
      }
      
      if(c!=-1 && c!=0)
      {
         if(c!=MEA_PATH)
            _o=1;
         mea_string_free_alloc_and_copy(&params_list[c], optarg);
         if(params_list[c]==NULL)
         {
            VERBOSE(1) {
               mea_log_printf("%s (%s) : malloc - ", ERROR_STR,__func__);
               perror("");
            }
            clean_all_and_exit();
         }
      }
   }

   //
   // Contrôle des parametres
   //
   if((_i+_a+_u)>1)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : --init (-i), --autoinit (-a), et --update (-u) incompatible\n",ERROR_STR,__func__);
      usage((char *)argv[0]);
      clean_all_and_exit();
   }
   
   if(_v > 0 && _v < 10)
         set_verbose_level(_v);

   if(!_d) // si pas de db en parametre on construit un chemin vers le nom "théorique" de la db
   {
      params_list[SQLITE3_DB_PARAM_PATH]=(char *)malloc(strlen(params_list[MEA_PATH])+1 + 17); // lenght("/var/db/params.db") = 17
      if(!params_list[SQLITE3_DB_PARAM_PATH])
      {
         VERBOSE(1) {
            mea_log_printf("%s (%s) : malloc - ", ERROR_STR,__func__);
            perror("");
         }
         clean_all_and_exit();
      }
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
      clean_all_and_exit();
   }
   
   if(_u)
   {
      updateMeaEdomus(params_list, params_names);
      clean_all_and_exit();
   }

   if(_o)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : options complémentaires uniquement utilisable avec --init (-i), --autoinit (-a), et --update (-u)\n", ERROR_STR, __func__);
      usage((char *)argv[0]);
      clean_all_and_exit();
   }

   
   //
   // Contrôle et ouverture de la base de paramétrage
   //
   int16_t cause;
   if(checkParamsDb(params_list[SQLITE3_DB_PARAM_PATH], &cause))
   {
      VERBOSE(1) mea_log_printf("%s (%s) : checkParamsDb - parameters database error (%d)\n", ERROR_STR, __func__, cause);
      clean_all_and_exit();
   }

 
   // ouverture de la base de paramétrage
   ret = sqlite3_open_v2(params_list[SQLITE3_DB_PARAM_PATH], &sqlite3_param_db, SQLITE_OPEN_READWRITE, NULL);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      clean_all_and_exit();
   }

   
   // lecture de tous les paramètres de l'application
#ifdef __linux__
   mea_string_free_alloc_and_copy(&params_list[INTERFACE], "eth0");
#else
   mea_string_free_alloc_and_copy(&params_list[INTERFACE], "en0");
#endif


   ret = read_all_application_parameters(sqlite3_param_db);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't load parameters\n",ERROR_STR,__func__);
      sqlite3_close(sqlite3_param_db);
      clean_all_and_exit();
   }

   if(params_list[PARAMSDBVERSION]!=NULL)
   {
      params_db_version=atoi(params_list[PARAMSDBVERSION]);
   }

   if(params_db_version != CURRENT_PARAMS_DB_VERSION)
   {
      struct upgrade_params_s upgrade_params;
      
      upgrade_params.params_list = params_list;
      
      // mise à jour de la base
      if(upgrade_params_db(sqlite3_param_db, params_db_version, CURRENT_PARAMS_DB_VERSION, &upgrade_params)<0)
      {
         VERBOSE(1) mea_log_printf("%s (%s) : database upgrade failed.\n",ERROR_STR,__func__);
         sqlite3_close(sqlite3_param_db);
         clean_all_and_exit();      
      }
      
      // rechargement des parametres
      ret=read_all_application_parameters(sqlite3_param_db);
      if(ret)
      {
         VERBOSE(1) mea_log_printf("%s (%s) : can't reload parameters\n",ERROR_STR,__func__);
         sqlite3_close(sqlite3_param_db);
         clean_all_and_exit();
      }
   }
   
   
   //
   // stdout et stderr vers fichier log
   //
   char log_file[255];
   int16_t n;

//   dbgfd = fopen("/tmp/dbg.log", "r+");

   if(!params_list[LOG_PATH] || !strlen(params_list[LOG_PATH]))
   {
      params_list[LOG_PATH]=(char *)malloc(strlen("/var/log"));
      if(params_list[LOG_PATH]==NULL)
      VERBOSE(1) {
         mea_log_printf("%s (%s) : malloc - ", ERROR_STR,__func__);
         perror("");
         clean_all_and_exit();
      }
      strcpy(params_list[LOG_PATH],"/var/log");
   }

   n=snprintf(log_file,sizeof(log_file),"%s/mea-edomus.log", params_list[LOG_PATH]);
   if(n<0 || n==sizeof(log_file))
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
         clean_all_and_exit();
      }
   }

   int fd=open(log_file, O_CREAT | O_APPEND | O_RDWR,  S_IWUSR | S_IRUSR);
   if(fd<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't open log file - ",ERROR_STR,__func__);
      perror("");
      clean_all_and_exit();
   }
   
   dup2(fd, 1);
   dup2(fd, 2);
   close(fd);

   logfile_rotation_job(-1, (void *)log_file, NULL, 0);

   DEBUG_SECTION mea_log_printf("INFO Starting MEA-EDOMUS %s\n",__MEA_EDOMUS_VERSION__);

   //
   // initialisation gestions des signaux (arrêt de l'appli et réinitialisation
   //
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGCHLD, _signal_SIGCHLD); // pour eviter les zombis
   signal(SIGPIPE, SIG_IGN);

   mea_notify_disable(); // système de notification desactivé

   //
   // démarrage du serveur de temps
   //
   start_timeServer();


   //
   // initialisation du gestionnaire de process
   //
   init_processes_manager(40);


   //
   // déclaration du process principal
   //
   main_monitoring_id=process_register("MAIN");
   process_set_type(main_monitoring_id, NOTMANAGED);
   process_set_status(main_monitoring_id, RUNNING);
   process_add_indicator(main_monitoring_id, "UPTIME", 0);
   process_add_indicator(main_monitoring_id, "SIGSEGV", 0);
   fprintf(stderr,"MAIN : %x\n",  (unsigned int)pthread_self());

   //
   // nodejsServer (1er lancé, il est utilisé par les autres serveurs pour les notifications ou les log)
   //
   struct nodejsServerData_s nodejsServer_start_stop_params;
   nodejsServer_start_stop_params.params_list=params_list;
   nodejsServer_monitoring_id=process_register("NODEJSSERVER");
   process_set_group(nodejsServer_monitoring_id, 5); // voir si on permet de desactiver (supprimer le groupe). Si desactivation il faut aussi desactiver les notifications ... (utilisation de wrap des fonctions start/stop pour intégrer l'arrêt/relance des notifications.
   process_set_start_stop(nodejsServer_monitoring_id , start_nodejsServer, stop_nodejsServer, (void *)(&nodejsServer_start_stop_params), 1);
   process_set_watchdog_recovery(nodejsServer_monitoring_id, restart_nodejsServer, (void *)(&nodejsServer_start_stop_params));
   if(process_start(nodejsServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : can't start nodejs server\n",ERROR_STR,__func__);
   }
   else
   {
      mea_notify_set_port(get_nodejsServer_socketdata_port());
      mea_notify_enable(); // les notifications sont activées
      managed_processes_set_notification_hostname(localhost_const);
      managed_processes_set_notification_port(get_nodejsServer_socketdata_port());
   }

   //
   // guiServer
   //
   struct httpServerData_s httpServer_start_stop_params;
   httpServer_start_stop_params.params_list=params_list;
   httpServer_monitoring_id=process_register("GUISERVER");
   process_set_group(httpServer_monitoring_id, 5);
   process_set_start_stop(httpServer_monitoring_id , start_guiServer, stop_guiServer, (void *)(&httpServer_start_stop_params), 1);
   process_set_watchdog_recovery(httpServer_monitoring_id, restart_guiServer, (void *)(&httpServer_start_stop_params));
   process_add_indicator(httpServer_monitoring_id, "HTTPIN", 0);
   process_add_indicator(httpServer_monitoring_id, "HTTPOUT", 0);

   if(process_start(httpServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : can't start gui server\n",ERROR_STR,__func__);
   }

   //
   // LogServer
   //
   struct logServerData_s logServer_start_stop_params;
   logServer_start_stop_params.params_list=params_list;
   logServer_monitoring_id=process_register(log_server_name_str);
   process_set_start_stop(logServer_monitoring_id , start_logServer, stop_logServer, (void *)(&logServer_start_stop_params), 1);
   process_set_watchdog_recovery(logServer_monitoring_id, restart_logServer, (void *)(&logServer_start_stop_params));
   process_add_indicator(logServer_monitoring_id, log_server_logsent_str, 0);
   process_add_indicator(logServer_monitoring_id, log_server_logsenterr_str, 0);
   process_add_indicator(logServer_monitoring_id, log_server_readerror_str, 0);

   if(process_start(logServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : can't start log server\n",ERROR_STR,__func__);
   }

   //
   // dbServer
   //
   struct dbServerData_s dbServer_start_stop_params;
   dbServer_start_stop_params.params_list=params_list;
   dbServer_monitoring_id=process_register("DBSERVER");
   process_set_start_stop(dbServer_monitoring_id, start_dbServer, stop_dbServer, (void *)(&dbServer_start_stop_params), 1);
   process_set_watchdog_recovery(dbServer_monitoring_id, restart_dbServer, (void *)(&dbServer_start_stop_params));
   process_set_heartbeat_interval(dbServer_monitoring_id, 60);
   process_add_indicator(dbServer_monitoring_id, "DBSERVERINMEM", 0);
   process_add_indicator(dbServer_monitoring_id, "DBSERVERINSQLITE", 0);
   process_add_indicator(dbServer_monitoring_id, "DBSERVERMYWRITE", 0);

   if(!_b)
   {
      if(process_start(dbServer_monitoring_id, NULL, 0)<0)
      {
         VERBOSE(2) mea_log_printf("%s (%s) : can't start database server\n",ERROR_STR,__func__);
      }
   }

   //
   // pythonPluginServer
   //
   struct pythonPluginServer_start_stop_params_s pythonPluginServer_start_stop_params;
   pythonPluginServer_start_stop_params.params_list=params_list;
   pythonPluginServer_monitoring_id=process_register("PYTHONPLUGINSERVER");
   process_set_start_stop(pythonPluginServer_monitoring_id , start_pythonPluginServer, stop_pythonPluginServer, (void *)(&pythonPluginServer_start_stop_params), 1);
   process_set_watchdog_recovery(xplServer_monitoring_id, restart_pythonPluginServer, (void *)(&pythonPluginServer_start_stop_params));
   process_add_indicator(pythonPluginServer_monitoring_id, "PYCALL", 0);
   process_add_indicator(pythonPluginServer_monitoring_id, "PYCALLERR", 0);
   if(process_start(pythonPluginServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start python plugin server\n",ERROR_STR,__func__);
      clean_all_and_exit();
   }
   sleep(2);

   //
   // xPLServer
   //
   struct xplServer_start_stop_params_s xplServer_start_stop_params;
   xplServer_start_stop_params.params_list=params_list;
   xplServer_start_stop_params.sqlite3_param_db=sqlite3_param_db;
   xplServer_monitoring_id=process_register(xpl_server_name_str);
   process_set_start_stop(xplServer_monitoring_id, start_xPLServer, stop_xPLServer, (void *)(&xplServer_start_stop_params), 1);
   process_set_watchdog_recovery(xplServer_monitoring_id, restart_xPLServer, (void *)(&xplServer_start_stop_params));
   process_add_indicator(xplServer_monitoring_id, xpl_server_xplin_str, 0);
   process_add_indicator(xplServer_monitoring_id, xpl_server_xplout_str, 0);
   if(process_start(xplServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start xpl server\n",ERROR_STR,__func__);
      clean_all_and_exit();
   }

   //
   // automatorServer
   //
   struct automatorServer_start_stop_params_s automatorServer_start_stop_params;
   automatorServer_start_stop_params.params_list=params_list;
   automatorServer_monitoring_id=process_register(automator_server_name_str);
   process_set_start_stop(automatorServer_monitoring_id, start_automatorServer, stop_automatorServer, (void *)(&automatorServer_start_stop_params), 1);
   process_set_watchdog_recovery(automatorServer_monitoring_id, restart_automatorServer, (void *)(&automatorServer_start_stop_params));
   process_add_indicator(automatorServer_monitoring_id, automator_input_exec_time_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_output_exec_time_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_xplin_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_xplout_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_err_str, 0);
   if(process_start(automatorServer_monitoring_id, NULL, 0)<0)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start automator server\n",ERROR_STR,__func__);
      clean_all_and_exit();
   }

   //
   // interfacesServer
   //
   struct interfacesServerData_s interfacesServerData;
   interfacesServerData.params_list=params_list;
   interfacesServerData.sqlite3_param_db=sqlite3_param_db;
   interfaces=start_interfaces(params_list, sqlite3_param_db); // démarrage des interfaces
   int interfaces_reload_task_id=process_register("RELOAD"); // mise en place de la tâche de rechargement des paramètrages des interfaces
   process_set_group(interfaces_reload_task_id, 2);
   process_set_start_stop(interfaces_reload_task_id , restart_interfaces, NULL, (void *)(&interfacesServerData), 1);
   process_set_type(interfaces_reload_task_id, TASK);

   //
   // rotation des log 1x par jour
   //
   int log_rotation_id=process_register("LOGROTATION");
   process_set_start_stop(log_rotation_id, logfile_rotation_job, NULL, (void *)log_file, 1);
   process_set_type(log_rotation_id, JOB);
//   process_job_set_scheduling_data(log_rotation_id, "0,5,10,15,20,25,30,35,40,45,50,55|*|*|*|*", 0);
   process_job_set_scheduling_data(log_rotation_id, "0|0|*|*|*", 0); // rotation des log tous les jours à minuit
   process_set_group(log_rotation_id, 7);

   // au démarrage : rotation des log.
//   process_run_task(log_rotation_id, NULL, 0);

   VERBOSE(1) mea_log_printf("%s (%s) : MEA-EDOMUS %s starded\n", INFO_STR, __func__, __MEA_EDOMUS_VERSION__);

   time_t start_time;
   long uptime = 0;
   start_time = time(NULL);
   int guiport = atoi(params_list[GUIPORT]);

   while(1) // boucle principale
   {
      // supervision "externe" des process
      if(process_is_running(httpServer_monitoring_id)==RUNNING)
      {
         char response[512];
         // interrogation du serveur HTTP Interne pour heartbeat ... (voir le passage d'un parametre pour sécuriser ...)
         gethttp(localhost_const, guiport, "/CMD/ping.php", response, sizeof(response)); // a remplacer par un guiServer_ping();
      }
      
      if(process_is_running(nodejsServer_monitoring_id)==RUNNING)
      {
         nodejsServer_ping(); // pour mettre à jour le heartbeat de nodejs
      }
      
      // indicateur de fonctionnement de mea-edomus
      uptime = (long)(time(NULL)-start_time);
      process_update_indicator(main_monitoring_id, "UPTIME", uptime);
      process_update_indicator(main_monitoring_id, "SIGSEGV", sigsegv_indicator);

      managed_processes_loop(); // watchdog et indicateurs
      
      sleep(1);
   }
}

// http://www.tux-planet.fr/utilisation-des-commandes-diff-et-patch-sous-linux/
