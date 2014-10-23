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
#include "consts.h"

#include "init.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"

#include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"
#include "guiServer.h"
#include "logServer.h"
#include "automatorServer.h"

#include "processManager.h"

int xplServer_monitoring_id=-1;
int httpServer_monitoring_id=-1;
int logServer_monitoring_id=-1;
int pythonPluginServer_monitoring_id=-1;
int dbServer_monitoring_id=-1;

queue_t *interfaces=NULL;                  /*!< liste (file) des interfaces. Variable globale car doit être accessible par les gestionnaires de signaux. */
sqlite3 *sqlite3_param_db=NULL;            /*!< descripteur pour la base sqlite de paramétrage. Variable globale car doit être accessible par les gestionnaires de signaux. */
pthread_t *monitoringServer_thread=NULL;   /*!< Adresse du thread de surveillance interne. Variable globale car doit être accessible par les gestionnaires de signaux.*/

char *params_names[MAX_LIST_SIZE];          /*!< liste des noms (chaines) de paramètres dans la base sqlite3 de paramétrage.*/
char *params_list[MAX_LIST_SIZE];          /*!< liste des valeurs de paramètres.*/

pid_t automator_pid = 0;
int main_monitoring_id = -1;


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


void clean_all_and_exit()
{
   if(xplServer_monitoring_id!=-1)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping xPLServer... ",INFO_STR,__func__);
      process_stop(xplServer_monitoring_id);
      process_unregister(xplServer_monitoring_id);
      xplServer_monitoring_id=-1;
      VERBOSE(9) fprintf(stderr,"done\n");
   }

   if(httpServer_monitoring_id!=-1)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping httpServer... ",INFO_STR,__func__);
      process_stop(httpServer_monitoring_id);
      process_unregister(httpServer_monitoring_id);
      httpServer_monitoring_id=-1;
      VERBOSE(9) fprintf(stderr,"done\n");
   }
   
   if(interfaces)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping interfaces...\n",INFO_STR,__func__);
      stop_interfaces();
      VERBOSE(9) fprintf(stderr,"%s  (%s) : done\n",INFO_STR,__func__);
   }
   
   if(pythonPluginServer_monitoring_id!=-1)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping pythonPluginServer... ",INFO_STR,__func__);
      process_stop(pythonPluginServer_monitoring_id);
      process_unregister(pythonPluginServer_monitoring_id);
      pythonPluginServer_monitoring_id=-1;
      VERBOSE(9) fprintf(stderr,"done\n");
   }

   if(dbServer_monitoring_id!=-1)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping dbServer... ",INFO_STR,__func__);
      process_stop(dbServer_monitoring_id);
      process_unregister(dbServer_monitoring_id);
      dbServer_monitoring_id=-1;
      VERBOSE(9) fprintf(stderr,"done\n");
   }
   
   if(logServer_monitoring_id!=-1)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping logServer... ",INFO_STR,__func__);
      process_stop(logServer_monitoring_id);
      process_unregister(logServer_monitoring_id);
      logServer_monitoring_id=-1;
      VERBOSE(9) fprintf(stderr,"done\n");
   }
   
   if(httpServer_monitoring_id!=-1)
   {
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping guiServer... ",INFO_STR,__func__);
      process_stop(httpServer_monitoring_id);
      process_unregister(httpServer_monitoring_id);
      httpServer_monitoring_id=-1;
      VERBOSE(9) fprintf(stderr,"done\n");
   }
   
   if(automator_pid>0)
   {
      int status;
      
      VERBOSE(9) fprintf(stderr,"%s  (%s) : Stopping automatorServer... (%d)",INFO_STR,__func__,automator_pid);

      kill(automator_pid, SIGTERM);
      //waitpid(automator_pid, &status, 0);
      wait(&status);
      VERBOSE(9) fprintf(stderr,"done\n");
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
   
   VERBOSE(9) fprintf(stderr,"%s  (%s) : mea-edomus down ...\n",INFO_STR,__func__);

   exit(0);
}


void signal_callback_handler(int signum)
{
   fprintf(stderr, "%s  (%s) : Caught signal SIGPIPE %d\n", INFO_STR, __func__, signum);
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
   clean_all_and_exit();
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
   restart_down_interfaces(sqlite3_param_db, dbServer_get_md());
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
   if(!buff)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
         perror("");
      }
      clean_services_and_exit();
   }


   if(realpath(argv[0], buff))
   {
      char *path;

      path=malloc(strlen(dirname(buff))+1);
      if(!path)
      {
         VERBOSE(1) {
            fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
            perror("");
         }
         free(buff);
         clean_services_and_exit();
      }
      strcpy(path,dirname(buff));
      
      params_list[MEA_PATH]=(char *)malloc(strlen(dirname(path))+1);
      if(!patparams_list[MEA_PATH])
      {
         VERBOSE(1) {
            fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
            perror("");
         }
         free(buff);
         free(path);
         clean_services_and_exit();
      }

      strcpy(params_list[MEA_PATH],dirname(path));
      free(path);
   }
   free(buff);
   */
   
   string_free_malloc_and_copy(&params_list[MEA_PATH], "/usr/local/mea-edomus", 1);
   if(!params_list[MEA_PATH])
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
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
            if(!params_list[MEA_PATH])
            {
               VERBOSE(1) {
                  fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
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
      }

      if(c>'!') // parametre non attendu trouvé (! = premier caractère imprimable).
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : Paramètre \"%s\" inconnu.\n",ERROR_STR,__func__,optarg);
         usage((char *)argv[0]);
         clean_all_and_exit();
      }
      
      if(c!=-1 && c!=0)
      {
         if(c!=MEA_PATH)
            _o=1;
         string_free_malloc_and_copy(&params_list[c], optarg, 1);
         if(params_list[c]==NULL)
         {
            VERBOSE(1) {
               fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
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
      VERBOSE(1) fprintf(stderr,"%s (%s) : --init (-i), --autoinit (-a), et --update (-u) incompatible\n",ERROR_STR,__func__);
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
            fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
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
      // à faire
      updateMeaEdomus(params_list, params_names);
      clean_all_and_exit();
   }

   if(_o)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : options complémentaires uniquement utilisable avec --init (-i), --autoinit (-a), et --update (-u)\n", ERROR_STR, __func__);
      usage((char *)argv[0]);
      clean_all_and_exit();
   }

   
   //
   // Contrôle et ouverture de la base de paramétrage
   //
   int16_t cause;
   if(checkParamsDb(params_list[SQLITE3_DB_PARAM_PATH], &cause))
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : checkParamsDb - parameters database error\n", ERROR_STR, __func__);
      clean_all_and_exit();
   }

 
   // ouverture de la base de paramétrage
   ret = sqlite3_open_v2(params_list[SQLITE3_DB_PARAM_PATH], &sqlite3_param_db, SQLITE_OPEN_READWRITE, NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      clean_all_and_exit();
   }

   
   // lecture de tous les paramètres de l'application
   ret = read_all_application_parameters(sqlite3_param_db);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't load parameters\n",ERROR_STR,__func__);
      sqlite3_close(sqlite3_param_db);
      clean_all_and_exit();
   }


   //
   // strout et stderr vers fichier log
   //

   char log_file[255];
   int16_t n;

   if(!params_list[LOG_PATH] || !strlen(params_list[LOG_PATH]))
   {
      params_list[LOG_PATH]=(char *)malloc(strlen("/var/log"));
      if(params_list[LOG_PATH]==NULL)
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : malloc - ", ERROR_STR,__func__);
         perror("");
         clean_all_and_exit();
      }
      strcpy(params_list[LOG_PATH],"/var/log");
   }

   n=snprintf(log_file,sizeof(log_file),"%s/mea-edomus.log", params_list[LOG_PATH]);
   if(n<0 || n==sizeof(log_file))
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
         clean_all_and_exit();
      }
   }

   int fd=open(log_file, O_CREAT | O_APPEND | O_RDWR,  S_IWUSR | S_IRUSR);
   if(fd<0)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't open log file - ",ERROR_STR,__func__);
      perror("");
      clean_all_and_exit();
   }
   
   dup2(fd, 1);
   dup2(fd, 2);
   close(fd);

   DEBUG_SECTION fprintf(stderr,"Starting MEA-EDOMUS %s\n",__MEA_EDOMUS_VERSION__);

   //
   // initialisation du gestionnaire de process
   //
   init_processes_manager(40);

   //   
   // demarrage du processus de l'automate
   //
//   automator_pid = start_automatorServer(params_list[SQLITE3_DB_PARAM_PATH]);

   //
   // initialisation gestions des signaux (arrêt de l'appli et réinitialisation
   //
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGHUP,  _signal_HUP);
   
   signal(SIGPIPE, signal_callback_handler);


   // démarrage des "services" (les services "majeurs" arrêtent tout (exit) si non démarrage
   struct dbServerData_s dbServerData;
   dbServerData.params_list=params_list;
   dbServer_monitoring_id=process_register("DBSERVER");
   VERBOSE(9) fprintf (stderr, "%s  (%s) : starting DBSERVER ... ",INFO_STR,__func__);
   process_set_start_stop(dbServer_monitoring_id, start_dbServer, stop_dbServer, (void *)(&dbServerData), 1);
   if(!_b)
   {
      if(process_start(dbServer_monitoring_id)<0)
      {
         VERBOSE(9) fprintf (stderr, "error !!!\n");
         VERBOSE(1) fprintf (stderr, "%s (%s) : can't start database server\n",ERROR_STR,__func__);
         clean_all_and_exit();
      }
   }
   VERBOSE(9) fprintf (stderr, "done\n");


   struct pythonPluginServerData_s pythonPluginServerData;
   pythonPluginServerData.params_list=params_list;
   pythonPluginServerData.sqlite3_param_db=sqlite3_param_db;
   pythonPluginServer_monitoring_id=process_register("PYTHONPLUGINSERVER");
   VERBOSE(9) fprintf (stderr, "%s  (%s) : starting PYTHONPLUGINSERVER ... ",INFO_STR,__func__);
   process_set_start_stop(pythonPluginServer_monitoring_id , start_pythonPluginServer, stop_pythonPluginServer, (void *)(&pythonPluginServerData), 1);
   if(process_start(pythonPluginServer_monitoring_id)<0)
   {
      VERBOSE(9) fprintf (stderr, "error !!!\n");
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't start python plugin server\n",ERROR_STR,__func__);
      clean_all_and_exit();
   }
   VERBOSE(9) fprintf (stderr, "done\n");

   
   interfaces=start_interfaces(params_list, sqlite3_param_db, dbServer_get_md()); // démarrage des interfaces


   struct xplServerData_s xplServerData;
   xplServerData.params_list=params_list;
   xplServerData.sqlite3_param_db=sqlite3_param_db;
   xplServer_monitoring_id=process_register("XPLSERVER");
   process_set_start_stop(xplServer_monitoring_id , start_xPLServer, stop_xPLServer, (void *)(&xplServerData), 1);
   VERBOSE(9) fprintf (stderr, "%s  (%s) : starting XPLSERVER ... ",INFO_STR,__func__);
   if(process_start(xplServer_monitoring_id)<0)
   {
      VERBOSE(9) fprintf (stderr, "error !!!\n");
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't start xpl server\n",ERROR_STR,__func__);
      clean_all_and_exit();
   }
   VERBOSE(9) fprintf (stderr, "done\n");


   struct httpServerData_s httpServerData;
   httpServerData.params_list=params_list;
   httpServer_monitoring_id=process_register("GUISERVER");
   process_set_start_stop(httpServer_monitoring_id , start_guiServer, stop_guiServer, (void *)(&httpServerData), 1);
   
   VERBOSE(9) fprintf (stderr, "%s  (%s) : starting GUISERVER ... ",INFO_STR,__func__);
   if(process_start(httpServer_monitoring_id)<0)
   {
      VERBOSE(9) fprintf (stderr, "error !!!\n");
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't start gui server\n",ERROR_STR,__func__);
   }
   VERBOSE(9) fprintf (stderr, "done\n");
   
   
   struct logServerData_s logServerData;
   logServerData.params_list=params_list;
   logServer_monitoring_id=process_register("LOGSERVER");
   VERBOSE(9) fprintf (stderr, "%s (%s) : starting LOGSERVER ... ",INFO_STR,__func__);
   process_set_start_stop(logServer_monitoring_id , start_logServer, stop_logServer, (void *)(&logServerData), 1);
   if(process_start(logServer_monitoring_id)<0)
   {
      VERBOSE(9) fprintf (stderr, "error !!!\n");
      VERBOSE(1) fprintf (stderr, "%s (%s) : can't start log server\n",ERROR_STR,__func__);
   }
   VERBOSE(9) fprintf (stderr, "done\n");


   time_t start_time;
   long uptime = 0;

   start_time = time(NULL);

   DEBUG_SECTION fprintf(stderr,"MEA-EDOMUS %s starded\n",__MEA_EDOMUS_VERSION__);

   main_monitoring_id=process_register("MAIN");
   process_set_type(main_monitoring_id, NOTMANAGED);
   process_set_status(main_monitoring_id, RUNNING);
   process_add_indicator(main_monitoring_id, "UPTIME", 0);

   // boucle sans fin.
   char response[512];
   int nodejsdata_port = atoi(params_list[NODEJSDATA_PORT]);
   int guiport = atoi(params_list[GUIPORT]);
   while(1)
   {
      // interrogation du serveur HTTP Interne pour heartbeat ... (voir le passage d'un parametre pour sécuriser ...)
      gethttp(localhost_const, guiport, "/CMD/ping.php", response, sizeof(response));
 
      uptime = (long)(time(NULL)-start_time);
      process_update_indicator(main_monitoring_id, "UPTIME", uptime);

      managed_processes_loop(localhost_const, nodejsdata_port);

      sleep(10);
   }
}
