//
//  init.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 18/08/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>

#include <sqlite3.h>

#include "init.h"

#include "globals.h"
#include "debug.h"
#include "error.h"
#include "string_utils.h"
#include "types.h"

#include "sqlite3db.h"

char const *usr_str="/usr";

char *get_and_malloc_path(char *base_path,char *dir_name,char *question_str)
 /**
  * \brief     Demande un chemin de fichier ou de répertoire depuis l'entrée standard et retourne la valeur dans une chaine allouée (malloc).
  * \details   Une question est affichée et une propostion de répertoire est faite (validée par ENTER sans saisir de caractères).
  * \param     base_path    chemin de base (basename) pour la proposition de fichier/répertoire
  * \param     dir_name     nom dur fichier/répertoire "final"
  * \param     question_str la question à poser
  * \return    le chemain complet dans une chaine qui a été allouée ou NULL en cas d'erreur
  */
{
   char proposed_path_str[1024];
   char read_path_str[1024];
   char *path_to_return_str=NULL;

   if(base_path)
   {
      int16_t n=snprintf(proposed_path_str,sizeof(proposed_path_str), "%s/%s", base_path, dir_name);
      if(n<0 || n==sizeof(proposed_path_str))
         return NULL;
   }
   else
      proposed_path_str[0]=0;
   do
   {
      printf("%s [%s] : ", question_str, proposed_path_str);
      if(fgets(read_path_str,sizeof(read_path_str), stdin))
      {
         read_path_str[strlen(read_path_str)-1]=0; // retire le \n à la fin de la ligne
         if(strlen(read_path_str)) // appuie sur Enter sans saisir de caractères
            strcpy(proposed_path_str,read_path_str); // on prend la valeur qu'on a prosee
      }
      else
         return NULL;
   } while (strlen(proposed_path_str)==0);
    
   path_to_return_str=(char *)malloc(strlen(proposed_path_str)+1);
   strcpy(path_to_return_str, proposed_path_str);
    
   return path_to_return_str;
}


char *get_and_malloc_string(char *default_value, char *question_str)
 /**
  * \brief     Demande une chaine de caractères depuis l'entrée standard et retourne la valeur dans une chaine allouée (malloc).
  * \details   Une question est affichée et une propostion de valeur est faite (validée par ENTER sans saisir de caractères).
  * \param     default_value  valeur proposée
  * \param     question_str   la question à poser
  * \return    une chaine qui a été allouée ou NULL en cas d'erreur
  */
{
   char read_str[1024];
   char *to_return_str=NULL;

   if(!default_value)
   {
      default_value="";
   }

   do
   {
      printf("%s [%s] : ", question_str, default_value);
      if(fgets(read_str,sizeof(read_str), stdin))
      {
         read_str[strlen(read_str)-1]=0; // retire le \n à la fin de la ligne
         if(!strlen(read_str)) // appuie sur Enter sans saisir de caractères
            strcpy(read_str,default_value); // on prend la valeur qu'on a prosee
      }
      else
         return NULL;
   } while (strlen(read_str)==0);
    
   to_return_str=(char *)malloc(strlen(read_str)+1);
   strcpy(to_return_str, read_str);
    
   return to_return_str;
}


int16_t lineToFile(char *file, char *lines[])
 /**
  * \brief     Création d'un fichier (texte) avec les lignes passées en parametres (tableau de chaine).
  * \details   Le dernier élément du tableau de lignes doit être NULL.
  * \param     file    nom et chemin (complet) du fichier à créer.
  * \param     lines   tableau de pointeur sur les chaines à intégrer au fichier.
  * \return    0 si OK, -1 si KO. Voir errno en cas d'erreur
  */
{
   FILE *fd;
   
   fd=fopen(file,"w");
   if(fd)
   {
      int error_flag=0;
      
      for(int16_t i=0;lines[i];i++)
      {
         if(fprintf(fd,"%s\n",lines[i])<0)
         {
            error_flag=-1;
            break;
         }
      }
      fclose(fd);
      
      return error_flag;
   }
   else
      return -1;
}


int16_t create_php_ini(char *phpini_path)
 /**
  * \brief     Création d'un fichier fichier php.ini "standard" (minimum nécessaire au fonctionnement d'un php-cgi).
  * \details   Un seul paramètre pour l'instant : timezone.
  * \param     phpini_path  chemin (complet) du fichier php.ini à créer.
  * \return    0 si OK, -1 si KO. Voir errno en cas d'erreur
  */
{
   char *lines[] = {
      "[Date]",
      "date.timezone = \"Europe/Berlin\"",
      NULL
   };
   
   char phpini[1024];
   
   sprintf(phpini,"%s/php.ini",phpini_path);

   int16_t rc=lineToFile(phpini,lines);
   
   if(rc)
   {
      VERBOSE(5) {
         fprintf(stderr, "%s (%s) : cannot write %s file - ", ERROR_STR, __func__, phpini);
         perror("");
      }
   }
   return rc;
}


int create_configs_php(char *gui_home, char *params_db_fullname, char *php_log_fullname)
 /**
  * \brief     Création d'un fichier configs.php cohérent avec l'installation de l'interface graphique et les chemins de fichiers à utiliser.
  * \details   Actuellement positionne fichier de log de php et chemin de la base sqlite3 de parametrage.
  * \param     gui_home            chemin du répertoire contenant l'arboressance des fichiers de l'interface graphique. Le fichier sera créé dans "gui_home/lib"
  * \param     params_db_fullname  nom complet et chemin de la base de paramétrage.
  * \param     php_log_fullname    nom complet et chemin du fichier de log.
  * \return    0 si OK, -1 si KO. Voir errno en cas d'erreur.
  */
{
   FILE *fd;
   const char *file="lib/configs.php";
   char *f;
   
   f=malloc(strlen(gui_home) + strlen(file) + 2); // 2 : fin de fichier + '/'
   sprintf(f,"%s/%s",gui_home,file);
   
   fd=fopen(f,"w");
   if(fd)
   {
      fprintf(fd,"<?php\n");
      fprintf(fd,"ini_set('error_reporting', E_ALL);\n");
      fprintf(fd,"ini_set('log_errors', 'On');\n");
      fprintf(fd,"ini_set('display_errors', 'Off');\n");
      fprintf(fd,"ini_set(\"error_log\", \"%s\");\n",php_log_fullname);
      fprintf(fd,"$TITRE_APPLICATION='Mea eDomus Admin';\n");
      fprintf(fd,"$PARAMS_DB_PATH='sqlite:%s';\n",params_db_fullname);
      fprintf(fd,"$QUERYDB_SQL='sql/querydb.sql';\n");
   
      fclose(fd);
      free(f);
   }
   else
   {
      VERBOSE(5) {
         fprintf(stderr, "%s (%s) : cannot write %s file - ",ERROR_STR,__func__,f);
         perror("");
      }
      free(f);
      return -1;
   }
   return 0;
}


int16_t createMeaTables(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Création des tables de l'application dans la base sqlite3 de paramétrage.
  * \details   Les tables application_parameters, interfaces, locations, sensors_actuators, types, sessions et users sont créées à vide.
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   char *sql_createTable[] = {
      "CREATE TABLE application_parameters(id INTEGER PRIMARY KEY,key TEXT,value TEXT,complement TEXT)",
      "CREATE TABLE interfaces(id INTEGER PRIMARY KEY,id_interface INTEGER,id_type INTEGER,name TEXT,description TEXT,dev TEXT,parameters TEXT,state INTEGER)",
      "CREATE TABLE locations(id INTEGER PRIMARY KEY,id_location INTEGER,name TEXT,description TEXT)",
      "CREATE TABLE sensors_actuators(id INTEGER PRIMARY KEY,id_sensor_actuator INTEGER,id_type INTEGER,id_interface INTERGER,name TEXT,description TEXT,id_location INTEGER,parameters TEXT,state INTEGER)",
      "CREATE TABLE types(id INTEGER PRIMARY KEY,id_type INTEGER,name TEXT,description TEXT,parameters TEXT,flag INTEGER)",
      "CREATE TABLE sessions (id INTEGER PRIMARY KEY, userid TEXT, sessionid INTEGER, lastaccess DATETIME)",
      "CREATE TABLE users (id INTEGER PRIMARY KEY, id_user INTEGER, name TEXT, password TEXT, description TEXT, profil INTEGER, flag INTEGER)",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_createTable);
}


int16_t populateMeaUsers(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Ajoute les utilisateurs par défaut dans la table des utilisateurs.
  * \details   1 seul utilisateur est créé (ou recréé après suppression) pour le moment : admin.
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   char *sql_usersTable[] = {
      "DELETE FROM 'users' WHERE name='admin'",
      "INSERT INTO 'users' (name, password, description, profil, flag) VALUES ('admin','admin','Default administrator','1','1')",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_usersTable);
}


int16_t populateMeaLocations(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Ajoute les lieux par défaut dans la table des lieux.
  * \details   1 seul lieu est créé (ou recréé après suppression) pour le moment : unknown (ie pas de localisation précisée).
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   char *sql_usersTable[] = {
      "DELETE FROM 'locations' WHERE name='unknown'",
      "INSERT INTO 'locations' (id_location, name, description) VALUES ('1','unknown','')",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_usersTable);
}


int populateMeaTypes(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Ajoute les types par défaut dans la table des types.
  * \details   Il s'agit des types utilisés directement par l'application (interface_001 ou interface_002 pour le moment).
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   struct types_value_s
   {
      int16_t  id_type;
      char *name;
      char *description;
      char *parameters;
      char *flag;
   };
   
   struct types_value_s types_values[] = {
      {INTERFACE_TYPE_001,"INTYP01","Interface de type 01","","1"},
      {INTERFACE_TYPE_002,"INTYP02","Interface de type 02","","1"},
      {201,"XBEECA","Capteurs et actionneurs a interface XBee","","1"},
      {1000,"PWRCTR","Capteur de compteur ERDF","","1"},
      {1001,"ARDINA","Entree interface type 01","","1"},
      {1002,"ARDOUTD","Sortie interface type 02","","1"},
      {2000,"XDHT22H","Humidité de DTH22","","2"},
      {2001,"XDHT22T","Température de DTH22","","2"},
      {2002,"XDHT22P","Pile de DTH22","","2"},
      {0,NULL,NULL,NULL,NULL}
   };
   
   char sql[1024];
   int16_t rc = 0;
   char *err = NULL;
   
   for(int16_t i=0;types_values[i].name;i++)
   {
      int16_t ret;
      
      sprintf(sql,"DELETE FROM 'types' WHERE name='%s'",types_values[i].name);
      ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_free(err);
         break;
      }
      
      sprintf(sql, "INSERT INTO 'types' (id_type,name,description,parameters,flag) VALUES (%d,'%s','%s','%s', '%s')",types_values[i].id_type,types_values[i].name,types_values[i].description,types_values[i].parameters,types_values[i].flag);
      
      ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_free(err);
         rc=1;
         break;
      }
   }
   return rc;
}


int16_t init_db(char *sqlite3_db_param_path,
                char *base_path,
                char *phpcgi_path,
                char *phpini_path,
                char *gui_path,
                char *plugins_path,
                char *log_path)
 /**
  * \brief     initialisation de la table "application_parameters".
  * \details   Les différents paramètres nécessaires au lancement de l'application sont insérés dans la base
  * \param     sqlite3_db_param_path  chemin vers la base
  * \param     base_path              chemin de base de l'installation
  * \param     phpcgi_path            chemin vers le répertoire contenant le "php-cgi"
  * \param     phpini_path            chemin vers le répertoire contenant le "php.ini"
  * \param     gui_path               chemin vers le répertoire racine de l'arboressance de l'interface graphique
  * \param     plugins_path           chemin vers le répertoire contenant les plugins python
  * \param     log_path               chemin vers le répertoire des logs
  * \return    0 si OK, un code de 1 à 10 en cas d'erreur.
  */
{
   struct key_value_complement_s {
      char *key;
      char *value;
      char *complement;
   };
   struct key_value_complement_s keys_values_complement[] = {
      /* 00 */ {"BUFFERDB",NULL,NULL},
      /* 01 */ {"DBSERVER",NULL,NULL},
      /* 02 */ {"DATABASE",NULL,NULL},
      /* 03 */ {"USER",NULL,NULL},
      /* 04 */ {"PASSWORD",NULL,NULL},
      /* 05 */ {"VENDORID","mea",NULL},
      /* 06 */ {"DEVICEID","edomus",NULL},
      /* 07 */ {"INSTANCEID","demo",NULL},
      /* 08 */ {"PLUGINPATH",NULL,NULL},
      /* 09 */ {"PHPINIPATH",NULL,NULL},
      /* 10 */ {"PHPCGIPATH",NULL,NULL},
      /* 11 */ {"GUIPATH",NULL,NULL},
      /* 12 */ {"DBPORT",NULL,NULL},
      /* 13 */ {"LOGPATH",NULL,NULL},
      /* 14 */ {NULL,NULL,NULL}
   };
    
   char *queries_db_end_path="var/db/mea-queries.db";
    
   sqlite3 *sqlite3_param_db=NULL;
   int16_t retcode=0;
   char *queries_db_path=NULL;

   if(phpini_path)
      keys_values_complement[9].value=phpini_path;
   if(phpcgi_path)
      keys_values_complement[10].value=phpcgi_path;
   if(gui_path)
      keys_values_complement[11].value=gui_path;
   if(plugins_path)
      keys_values_complement[8].value=plugins_path;
   if(log_path)
      keys_values_complement[13].value=log_path;

   if(base_path)
   {
      int16_t l=strlen(base_path);
        
      // queries db
      queries_db_path=malloc(l+strlen(queries_db_end_path)+1);
      if(strcmp(usr_str,base_path)==0)
         sprintf(queries_db_path,"/%s",queries_db_end_path);
      else
         sprintf(queries_db_path,"%s/%s",base_path,queries_db_end_path);
      keys_values_complement[0].value=queries_db_path;
   }
   else
   {
      retcode=1;
      goto exit_init;
   }
    
   // suppression de la base si elle existe déjà
   int16_t func_ret;
   func_ret = sqlite3_dropDatabase(sqlite3_db_param_path);
   if(func_ret!=0 && errno!=ENOENT)
   {
      retcode=2;
      goto exit_init;
   }
    
   // création de la base
   int16_t nerr = sqlite3_open(sqlite3_db_param_path, &sqlite3_param_db);
   if(nerr)
   {
      VERBOSE(5) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      retcode=3;
      goto exit_init;
   }
    
   // création des tables
   if(createMeaTables(sqlite3_param_db))
   {
      retcode=4;
      goto exit_init;
   }
    
   // Chargement des types standards
   if(populateMeaTypes(sqlite3_param_db))
   {
      retcode=5;
      goto exit_init;
   }
   
   if(populateMeaLocations(sqlite3_param_db))
   {
      retcode=6;
      goto exit_init;
   }

   if(populateMeaUsers(sqlite3_param_db))
   {
      retcode=7;
      goto exit_init;
   }

    
    // initialisation des paramètres de l'application
    char sql_query[1024];
    char *errmsg = NULL;
    
    for(int16_t i=0;keys_values_complement[i].key;i++)
    {
        int16_t func_ret;
        int16_t n=snprintf(sql_query,sizeof(sql_query),"DELETE FROM 'application_parameters' WHERE key='%s'",keys_values_complement[i].key);
        if(n<0 || n==sizeof(sql_query))
        {
            retcode=8;
            goto exit_init;
        }
        func_ret = sqlite3_exec(sqlite3_param_db, sql_query, NULL, NULL, &errmsg);
        if( func_ret != SQLITE_OK )
        {
            VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__, errmsg);
            sqlite3_free(errmsg);
        }
        
        n=sprintf(sql_query, "INSERT INTO 'application_parameters' (key,value,complement) VALUES ('%s','%s','%s')",keys_values_complement[i].key, keys_values_complement[i].value, keys_values_complement[i].complement);
        if(n<0 || n==sizeof(sql_query))
        {
            retcode=9;
            goto exit_init;
        }
        func_ret = sqlite3_exec(sqlite3_param_db, sql_query, NULL, NULL, &errmsg);
        if( func_ret != SQLITE_OK )
        {
            VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__, errmsg);
            sqlite3_free(errmsg);
            retcode=10;
            break;
        }
    }
exit_init:
    if(sqlite3_param_db)
        sqlite3_close(sqlite3_param_db);
    if(queries_db_path)
        free(queries_db_path);
    return retcode;
}


int _construct_path(char **params_list, int16_t index, char *path, char *end_path)
{
   char tmp_str[1024];
   if(!params_list[index])
   {
      int16_t n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", path, end_path);
      if(n<0 || n==sizeof(tmp_str)) {
         return -1;
      }
      params_list[index]=malloc(n+1);
      if(!params_list[index]) {
         return -2;
      }
      strcpy(params_list[index],tmp_str);
   }
   return 0;
}


int _construct_string(char **params_list, int16_t index, char *value)
{
   if(!params_list[index])
   {
      params_list[index]=malloc(strlen(value)+1);
      strcpy(params_list[index],value);
   }
   return 0;
}


int _read_path(char **params_list, uint16_t index, char *base_path, char *dir_name, char *question)
{
   char tmp_str[1024];
   if(params_list[index])
   {
      strncpy(tmp_str, params_list[index], sizeof(tmp_str));
      free(params_list[index]);
      params_list[index]=NULL;
      
      params_list[index]=get_and_malloc_string(tmp_str, question);
   }
   else
   {
      params_list[index]=get_and_malloc_path(base_path, dir_name, question);
   }
   return 0;
}


int _read_string(char **params_list, uint16_t index, char *default_value, char *question)
{
   char tmp_str[1024];
   if(params_list[index])
   {
      strcpy(tmp_str, params_list[index]);
      free(params_list[index]);
      params_list[index]=NULL;
   }
   else
   {
      strncpy(tmp_str,default_value,sizeof(tmp_str));
   }
   params_list[index]=get_and_malloc_string(tmp_str, question);
   
   return 0;
}


int16_t autoInit(char **params_list)
 /**
  * \brief     réalise une configuration initiale automatique de l'application.
  * \details   contrôle installation, création des répertoires (si absent), création des bases et tables, alimentation des tables 
  * \param     params_list  chemin complet vers le fichier de la base
  * \return    0 si OK, code > 0 en cas d'erreur.
  */
{
   char to_check[1024];

   char *p_str; // pointeur sur une chaine
   int16_t retcode=0;
   
   if(strcmp(usr_str,params_list[MEA_PATH])==0)
      p_str="";
   else
      p_str=params_list[MEA_PATH];


   _construct_string(params_list, VENDOR_ID,   "mea");
   _construct_string(params_list, DEVICE_ID,   "domus");
   _construct_string(params_list, INSTANCE_ID, "home");
   
   _construct_path(params_list, PHPCGI_PATH,  params_list[MEA_PATH], "bin");
   _construct_path(params_list, PHPINI_PATH,  p_str,                 "etc");
   _construct_path(params_list, GUI_PATH,     params_list[MEA_PATH], "lib/mea-gui");
   _construct_path(params_list, PLUGINS_PATH, params_list[MEA_PATH], "lib/mea-plugins");
   _construct_path(params_list, LOG_PATH,     p_str,                 "var/log");

   _construct_string(params_list, MYSQL_DB_SERVER, "127.0.0.1");
   _construct_string(params_list, MYSQL_DB_PORT,   "3306");
   _construct_string(params_list, MYSQL_DATABASE,  "meaedomus");
   _construct_string(params_list, MYSQL_USER,      "meaedomus");
   _construct_string(params_list, MYSQL_PASSWD,    "meaedomus");
   
   _construct_path(params_list,   SQLITE3_DB_BUFF_PATH, p_str, "var/db");
   _construct_string(params_list, SQLITE3_DB_BUFF_NAME, "queries.db");
   
   _construct_path(params_list, LOG_PATH, p_str, "var/log");


   snprintf(to_check,sizeof(to_check),"%s/php.ini",params_list[PHPINI_PATH]);
   if( access(to_check, R_OK) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/php.ini - ",WARNING_STR,__func__,params_list[PHPINI_PATH]);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'php.ini' exist, create one.\n",WARNING_STR);
      if(create_php_ini(params_list[PHPINI_PATH])) {
         retcode=11; goto autoInit_exit;
      }
   }

   snprintf(to_check,sizeof(to_check),"%s/cgi-bin",params_list[PHPCGI_PATH]);
   if( access(to_check, R_OK | X_OK) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/cgi-bin - ", WARNING_STR, __func__, params_list[PHPCGI_PATH]);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'cgi-bin', gui will not start.\n",WARNING_STR);
   }

   retcode=init_db(params_list[SQLITE3_DB_PARAM_PATH], params_list[MEA_PATH], params_list[PHPCGI_PATH], params_list[PHPINI_PATH], params_list[GUI_PATH], params_list[PLUGINS_PATH], params_list[LOG_PATH])+11;

autoInit_exit:
   return retcode;
}


int16_t interactiveInit(char **params_list)
 /**
  * \brief     réalise une configuration initiale interactive de l'application.
  * \details   création des répertoires, création des bases et tables, alimentation des tables à partir de données saisies par l'utilisateur
  * \param     params_list  liste de tous les parametres
  * \return    0 si OK, code > 0 en cas d'erreur.
  */
{
   char *p_str;
   char to_check[1024];
   int16_t retcode=0;

   if(strcmp(usr_str,params_list[MEA_PATH])==0)
      p_str="";
   else
      p_str=params_list[MEA_PATH];

   // Récupération des données
   _read_string(params_list, VENDOR_ID,       "mea",       "xPL Vendor ID");
   _read_string(params_list, DEVICE_ID,       "domus",     "xPL Device ID");
   _read_string(params_list, INSTANCE_ID,     "home",      "xPL Instance ID");
   
   _read_path(params_list,   GUI_PATH,        params_list[MEA_PATH], "lib/mea-gui",     "PATH to gui directory");
   _read_path(params_list,   PLUGINS_PATH,    params_list[MEA_PATH], "lib/mea-plugins", "PATH to plugins directory");
   _read_path(params_list,   PHPCGI_PATH,     params_list[MEA_PATH], "bin",             "PATH to 'php-cgi' directory");
   _read_path(params_list,   PHPINI_PATH,     p_str,                 "etc",             "PATH to 'php.ini' directory");

   _read_string(params_list, MYSQL_DB_SERVER, "127.0.0.1", "mysql dbserver name or address");
   _read_string(params_list, MYSQL_DB_PORT,   "3306",      "mysql dbserver port");
   _read_string(params_list, MYSQL_DATABASE,  "meaedomus", "mysql database name");
   _read_string(params_list, MYSQL_USER,      "meaedomus", "mysql user name");
   _read_string(params_list, MYSQL_PASSWD,    "meaedomus", "mysql user password");

   _read_path(params_list,   SQLITE3_DB_BUFF_PATH, p_str,        "var/db", "PATH to sqlite3 buffer db");
   _read_string(params_list, SQLITE3_DB_BUFF_NAME, "queries.db", "sqlite3 buffer name");

   _read_path(params_list,   LOG_PATH,        p_str, "var/log", "PATH to logs directory");

   // contrôle des données
   snprintf(to_check,sizeof(to_check), "%s/php-cgi", params_list[PHPCGI_PATH]);
   if( access(to_check, R_OK | W_OK | X_OK) == -1 )
   {
      VERBOSE(9) fprintf(stderr, "%s (%s) : %s/php-cgi - ", INFO_STR, __func__, params_list[PHPCGI_PATH]);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'php-cgi', gui will not start.\n",WARNING_STR);
   }

   snprintf(to_check,sizeof(to_check),"%s/php.ini",params_list[PHPINI_PATH]);
   if( access(to_check, R_OK ) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/php.ini - ",INFO_STR,__func__,params_list[PHPINI_PATH]);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'php.ini' exist, create one.\n",WARNING_STR);
      create_php_ini(params_list[PHPINI_PATH]);
   }

   // insertion des données dans la base
   retcode=init_db(params_list[SQLITE3_DB_PARAM_PATH], params_list[MEA_PATH], params_list[PHPCGI_PATH], params_list[PHPINI_PATH], params_list[GUI_PATH], params_list[PLUGINS_PATH], params_list[LOG_PATH])+11;

   return retcode;
}


int16_t initMeaEdomus(int16_t mode, char **params_list)
 /**
  * \brief     prépare et choisi le type d'initialisation à réaliser.
  * \details   c'est dans cette fonction que la création des répertoires peut être réalisées
  * \param     mode         mode d'initialisation : 0 = interactif, 1 = automatique
  * \param     params_list  liste de tous les parametres
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
    int16_t installPathFlag=0;
    int16_t func_ret;

    if(!params_list[MEA_PATH])
        params_list[MEA_PATH]="/usr/local/mea-edomus";
    
    
    if(mode==1) // automatique
    {
        installPathFlag=checkInstallationPaths(params_list[MEA_PATH], 1); // en auto, on essaye de créer les repertoires manquants
        if(installPathFlag==-1)
        {
            return 1; // les répertoires n'existent pas et n'ont pas pu être créés, installation automatique impossible
        }
        func_ret=autoInit(params_list);
    }
    else // interactif
    {
        installPathFlag=checkInstallationPaths(params_list[MEA_PATH], 0);
        if(installPathFlag==-2) // au moins 1 répertoire n'existe pas (mais tous les répertoires existants sont accessibles en lecture/écriture).
        {
            installPathFlag=-1;
            // doit-on essayer de les creer ?
            printf("Certains repertoires préconisés n'existent pas. Voulez-vous tenter de les créer ? (O/N) : ");
            char c=fgetc(stdin); // a tester
            if(c=='Y' || c=='y' || c=='O' || c=='o')
            {
                installPathFlag=checkInstallationPaths(params_list[MEA_PATH], 1); // on rebalaye mais cette fois on essaye de créer les repertoires
                if(!installPathFlag) // pas d'erreur tous les repertoires existent maintenant en lecture écriture.
                    printf("Les répertoires ont été créés\n");
            }
        }
        if(installPathFlag==-1)
            printf("Certains répertoire recommandés ne sont pas accessible en lecture/écriture ou sont absents. Vous devrez choisir d'autres chemins pour l'installation\n");
        func_ret=interactiveInit(params_list);
    }
    if(func_ret)
        return -1;
    else
        return 0;
}


int16_t updateMeaEdomus(char **params_list)
{
   return 0;
}


int16_t checkParamsDb(char *sqlite3_db_param_path, int16_t *cause)
 /**
  * \brief     Contrôle l'état de la base de parametrage.
  * \details   pour l'instant contrôle de la présence et de l'accessibilité en lecture/écriture uniquement
  * \param     sqlite3_db_param_path  chemin complet vers le fichier de la base
  * \param     cause                  en cas d'erreur contient le code de l'erreur
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
   sqlite3 *sqlite3_param_db;

   if( access( sqlite3_db_param_path, F_OK) == -1 ) // file exist ?
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : \"%s\" n'existe pas ou n'est pas accessible.\n",ERROR_STR,__func__,sqlite3_db_param_path);
      *cause=1;
      return -1;
   }
   
   if( access( sqlite3_db_param_path, R_OK | W_OK) == -1 )
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : \"%s\" doit etre accessible en lecture/ecriture.\n",ERROR_STR,__func__,sqlite3_db_param_path);
      *cause=2;
      return -1;
   }

   int16_t ret = sqlite3_open_v2(sqlite3_db_param_path, &sqlite3_param_db, SQLITE_OPEN_READONLY, NULL);
   if(ret)
   {
      VERBOSE(5) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      *cause=3;
      return -1;
   }

   // contrôler le contenu de la base ... à faire ???
   //printf("Existe : %d\n",tableExist(sqlite3_param_db, "toto"));
   //printf("Existe : %d\n",tableExist(sqlite3_param_db, "interfaces"));
   
   sqlite3_close(sqlite3_param_db);
   
   return 0;
}


int16_t checkInstallationPaths(char *base_path, int16_t try_to_create_flag)
 /**
  * \brief     Verifie que l'installation est conforme aux recommendations.
  * \details   Les répertoires suivants doivent exister et etre accessibles en lecture/écriture :
  *    Si BASEPATH != /usr
  *            BASEPATH/bin
  *            BASEPATH/etc
  *            BASEPATH/lib
  *            BASEPATH/lib/plugins
  *            BASEPATH/var
  *            BASEPATH/var/db
  *            BASEPATH/var/log
  *            BASEPATH/gui
  *   Si BASEPATH == /usr (installation traditionnelle des distributions linux)
  *            /usr/bin
  *            /etc
  *            /usr/lib
  *            /usr/lib/mea-plugins
  *            /var
  *            /var/db
  *            /var/log
  *            /usr/lib/mea-gui
  *   Mes recommendations :
  *      BASEPATH=/usr/local/mea-edomus ou /opt/mea-edomus ou /apps/mea-edomus
  * \param     basepath            chemin vers le répertoire de base de l'installation
  * \param     try_to_create_flag  création ou non des répertoires s'il n'existe pas (0 = pas de création, 1 = création).
  * \return    0 si l'installation est ok, -1 = erreur bloquante, -2 = au moins un répertoire n'existe pas
  */
{
    const char *default_paths_list[]={NULL,"bin","etc","lib","lib/mea-plugins","var","var/db","var/log","lib/mea-gui",NULL};
    const char *usr_paths_list[]={"/etc","/usr/lib/mea-plugins","/var/db","/var/log","/usr/lib/mea-gui",NULL};
    
    char **paths_list;
    
    char path_to_check[1024];
    int16_t flag=0;
    int16_t is_usr_flag;
    int16_t n;
    
    if(strcmp("/",base_path)==0 || base_path[0]==0) // root et pas de base_path interdit !!!
    {
        VERBOSE(2) fprintf(stderr,"%s (%s) : root (\"/\") and empty string (\"\") not allowed for base path\n",ERROR_STR,__func__);
        return -1;
    }

    if(strcmp(usr_str,base_path)==0) // l'installation dans /usr n'a pas la même configuration de répertoire
    {
        paths_list=(char **)usr_paths_list;
        is_usr_flag=1;
    }
    else
    {
        paths_list=(char **)default_paths_list;
        paths_list[0]=base_path; // on ajoute basepath pas dans la liste pour éventuellement pouvoir le créer.
        is_usr_flag=0;
    }
        
    for(int16_t i=0;paths_list[i];i++)
    {
        if(is_usr_flag || i==0) // si i==0 c'est basepath
            n=snprintf(path_to_check,sizeof(path_to_check),"%s",paths_list[i]);
       else
            n=snprintf(path_to_check, sizeof(path_to_check), "%s/%s", base_path, paths_list[i]);
        if(n<0 || n==sizeof(path_to_check))
            return -1;
        
        int16_t func_ret=access(path_to_check, R_OK | W_OK | X_OK);
        if( func_ret == -1 ) // pas d'acces ou pas lecture/ecriture
        {
            if(errno == ENOENT) // le repertoire n'existe pas
            {
                // si option de creation on essaye
                if(try_to_create_flag && mkdir(path_to_check, S_IRWXU | // xwr pour user
                                                              S_IRGRP | S_IXGRP | // x_r pour group
                                                              S_IROTH | S_IXOTH)) // x_r pour other
                {
                    VERBOSE(2) {
                        fprintf(stderr,"%s (%s) : %s - ",INFO_STR,__func__,path_to_check);
                        perror("");
                    }
                    flag=-1; // pas les droits en écriture nécessaire
                }
                else
                    if(flag!=-1)
                        flag=-2; // repertoire n'existe pas. 1 (erreur de droit irattapable a priorité sur 2 ...)
            }
            else
            {
                VERBOSE(2) {
                    fprintf(stderr,"%s (%s) : %s - ",INFO_STR,__func__,path_to_check);
                    perror("");
                }
                flag=-1; // pas droit en écriture
            }
        }
    }
    return flag;
}