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

#include "debug.h"
#include "error.h"
#include "string_utils.h"
#include "types.h"

#include "sqlite3db.h"

char const *usr_str="/usr";

char *get_and_malloc_path(char *base_path,char *dir_name,char *question_str)
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


char *get_and_malloc_path_old(char *base_path,char *dir,char *question)
{
   char tmpStr[1024];
   char aPath[1024];
   
   char *path=NULL;
   
   if(base_path)
      sprintf(tmpStr,"%s/%s",base_path, dir);
   else
      tmpStr[0]=0;
   do
   {
      printf("%s [%s]", question, tmpStr);
      
      fgets(aPath,1023, stdin);
      aPath[strlen(aPath)-1]=0; // retire le \n a la fin de la ligne
      
      if(strlen(aPath))
         strcpy(tmpStr,aPath);
   } while (strlen(tmpStr)==0);
   path=(char *)malloc(strlen(tmpStr)+1);
   strcpy(path,tmpStr);
   
   return path;
}


int16_t lineToFile(char *file, char *lines[])
{
   FILE *fd;

   fd=fopen(file,"w");
   if(fd)
   {
      for(int16_t i=0;lines[i];i++)
      {
         fprintf(fd,"%s\n",lines[i]);
      }
      fclose(fd);
      return 0;
   }
   else
      return -1;
}


int16_t create_php_ini(char *phpini_path)
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
         fprintf(stderr, "%s (%s) : cannot write %s file - ",ERROR_STR,__func__,phpini);
         perror("");
      }
   }
   return rc;
}


int create_configs_php(char *gui_home, char *params_db_fullname)
{
   FILE *fd;
   const char *file="lib/configs.php";
   char *f;
   
   f=malloc(strlen(gui_home) + strlen(file) + 2);
   sprintf(f,"%s/%s",gui_home,file);
   
   fd=fopen(f,"w");
   if(fd)
   {
      fprintf(fd,"<?php\n");
      fprintf(fd,"ini_set('error_reporting', E_ALL);\n");
      fprintf(fd,"ini_set('log_errors', 'On');\n");
      fprintf(fd,"ini_set('display_errors', 'Off');\n");
      fprintf(fd,"ini_set(\"error_log\", \"/Data/mea_log.txt\");\n");
      fprintf(fd,"$TITRE_APPLICATION='Mea eDomus Admin';\n");
      fprintf(fd,"$PARAMS_DB_PATH='sqlite:%s';\n",params_db_fullname);
      fprintf(fd,"$QUERYDB_SQL='sql/querydb.sql';\n");
   
      fclose(fd);
   }
   else
   {
      VERBOSE(5) {
         fprintf(stderr, "%s (%s) : cannot write %s file - ",ERROR_STR,__func__,f);
         perror("");
      }
      return -1;
   }
   return 0;
}


int16_t createMeaTables(sqlite3 *sqlite3_param_db)
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
{
   char *sql_usersTable[] = {
      "DELETE FROM 'users' WHERE name='admin'",
      "INSERT INTO 'users' (name, password, description, profil, flag) VALUES ('admin','admin','Default administrator','1','1')",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_usersTable);
}


int16_t populateMeaLocations(sqlite3 *sqlite3_param_db)
{
   char *sql_usersTable[] = {
      "DELETE FROM 'locations' WHERE name='default'",
      "INSERT INTO 'locations' (id_location, name, description) VALUES ('1','default','')",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_usersTable);
}


int populateMeaTypes(sqlite3 *sqlite3_param_db)
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
         sprintf(queries_db_path,"%s",queries_db_end_path);
      else
         sprintf(queries_db_path,"%s/%s",base_path,queries_db_end_path);
      keys_values_complement[0].value=queries_db_path;
    }
   else
   {
      retcode=8;
      goto exit_init;
   }
    
   // suppression de la base si elle existe déjà
   int16_t func_ret;
   func_ret = sqlite3_dropDatabase(sqlite3_db_param_path);
   if(func_ret!=0 && errno!=ENOENT)
   {
      retcode=1;
      goto exit_init;
   }
    
   // création de la base
   int16_t nerr = sqlite3_open(sqlite3_db_param_path, &sqlite3_param_db);
   if(nerr)
   {
      VERBOSE(5) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      retcode=2;
      goto exit_init;
   }
    
   // création des tables
   if(createMeaTables(sqlite3_param_db))
   {
      retcode=3;
      goto exit_init;
   }
    
   // Chargement des types standards
   if(populateMeaTypes(sqlite3_param_db))
   {
      retcode=4;
      goto exit_init;
   }
   
   if(populateMeaLocations(sqlite3_param_db))
   {
      retcode=5;
      goto exit_init;
   }

   if(populateMeaUsers(sqlite3_param_db))
   {
      retcode=6;
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
            retcode=5;
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
            retcode=6;
            goto exit_init;
        }
        func_ret = sqlite3_exec(sqlite3_param_db, sql_query, NULL, NULL, &errmsg);
        if( func_ret != SQLITE_OK )
        {
            VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__, errmsg);
            sqlite3_free(errmsg);
            retcode=7;
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


int16_t autoInit(char *sqlite3_db_param_path, char *base_path)
{
   char *phpcgi_path="bin";
   char *phpini_path="etc";
   char *gui_path="lib/mea-gui";
   char *plugins_path="lib/mea-plugins";
   char *log_path="var/log";
   
   char to_check[1024];

   char *p_str;
   char tmp_str[1024];
   int16_t retcode=0;
    
   if(strcmp(usr_str,base_path)==0)
      p_str="";
   else
      p_str=base_path;
        
   // cgi-bin
   int16_t n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", base_path, phpcgi_path);
   if(n<0 || n==sizeof(tmp_str)) {
      retcode=1; goto autoInit_exit;
   }
   phpcgi_path=malloc(n+1);
   if(!phpcgi_path) {
      retcode=1; goto autoInit_exit;
   }
   strcpy(phpcgi_path,tmp_str);
    
    // php.ini
   n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", p_str, phpini_path);
   if(n<0 || n==sizeof(tmp_str)) {
      retcode=1; goto autoInit_exit;
   }
   phpini_path=malloc(n+1);
   if(!phpini_path){
      retcode=1; goto autoInit_exit;
   }
   strcpy(phpini_path,tmp_str);
    
   // gui_path
   n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", base_path, gui_path);
   if(n<0 || n==sizeof(tmp_str)) {
      retcode=1; goto autoInit_exit;
   }
   gui_path=malloc(n+1);
   if(!gui_path) {
      retcode=1; goto autoInit_exit;
   }
   strcpy(gui_path,tmp_str);
    
   // plugins_path
   n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", base_path, plugins_path);
   if(n<0 || n==sizeof(tmp_str)) {
      retcode=1; goto autoInit_exit;
   }
   plugins_path=malloc(n+1);
   if(!plugins_path) {
      retcode=1; goto autoInit_exit;
   }
   strcpy(plugins_path,tmp_str);
    
   // log_path
   n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", p_str, log_path);
   if(n<0 || n==sizeof(tmp_str)) {
      retcode=1; goto autoInit_exit;
   }
   log_path=malloc(n+1);
   if(!log_path) {
      retcode=1; goto autoInit_exit;
   }
   strcpy(log_path,tmp_str);

   
   snprintf(to_check,sizeof(to_check),"%s/php.ini",phpini_path);
   if( access(to_check, R_OK) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/php.ini - ",WARNING_STR,__func__,phpcgi_path);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'php.ini' exist, create one.\n",WARNING_STR);
      create_php_ini(phpini_path);
   }

   snprintf(to_check,sizeof(to_check),"%s/cgi-bin",phpcgi_path);
   if( access(to_check, R_OK | X_OK) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/cgi-bin - ",WARNING_STR,__func__,phpcgi_path);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'cgi-bin', gui will not start.\n",WARNING_STR);
   }

   retcode=init_db(sqlite3_db_param_path, base_path, phpcgi_path, phpini_path, gui_path, plugins_path, log_path);

autoInit_exit:    
   if(phpcgi_path)
      free(phpcgi_path);
   if(phpini_path)
      free(phpini_path);
   if(gui_path)
      free(gui_path);
   if(plugins_path)
      free(plugins_path);
   if(log_path)
      free(log_path);

   return retcode;
}


int16_t interactiveInit(char *sqlite3_db_param_path, char *base_path)
{
   char *phpcgi_path=NULL;
   char *phpini_path=NULL;
   char *gui_path=NULL;
   char *plugins_path=NULL;
   char *log_path=NULL;
   char *p_str;
   
   char to_check[1024];

   if(strcmp(usr_str,base_path)==0)
      p_str="";
   else
      p_str=base_path;
        
   // cgi-bin
   phpcgi_path=get_and_malloc_path(base_path,"bin","PATH to 'php-cgi' directory");
   // php.ini
   phpini_path=get_and_malloc_path(p_str,"etc","PATH to 'php.ini' directory");
   // gui_path
   gui_path=get_and_malloc_path(base_path,"lib/mea-gui","PATH to gui directory");
   // plugins_path
   plugins_path=get_and_malloc_path(base_path,"lib/mea-plugins","PATH to plugins directory");
   // log_path
   log_path=get_and_malloc_path(p_str,"var/log","PATH to logs directory");


   snprintf(to_check,sizeof(to_check),"%s/php-cgi",phpcgi_path);
   if( access(to_check, R_OK | W_OK | X_OK) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/php-cgi - ",INFO_STR,__func__,phpcgi_path);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'php-cgi', gui will not start.\n",WARNING_STR);
   }

   snprintf(to_check,sizeof(to_check),"%s/php.ini",phpini_path);
   if( access(to_check, R_OK ) == -1 )
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : %s/php.ini - ",INFO_STR,__func__,phpcgi_path);
      VERBOSE(9) perror("");
      VERBOSE(1) fprintf(stderr,"%s : no 'php.ini' exist, create one.\n",WARNING_STR);
      create_php_ini(phpini_path);
   }

   init_db(sqlite3_db_param_path, base_path, phpcgi_path, phpini_path, gui_path, plugins_path, log_path);
    
   if(phpcgi_path)
      free(phpcgi_path);
   if(phpini_path)
      free(phpini_path);
   if(gui_path)
      free(gui_path);
   if(plugins_path)
      free(plugins_path);
   if(log_path)
      free(log_path);

   return 0;
}


int16_t initMeaEdomus(int16_t mode, char *sqlite3_db_param_path, char *base_path)
 {
    int16_t installPathFlag=0;
    int16_t func_ret;

    if(!base_path)
        base_path="/usr/local/mea-edomus";
    
    
    if(mode==1) // automatique
    {
        installPathFlag=checkInstallationPaths(base_path,1); // en auto, on essaye de créer les repertoires manquants
        if(installPathFlag==1)
        {
            return 1; // les répertoires n'existent pas et n'ont pas pu être créés, installation automatique impossible
        }
        func_ret=autoInit(sqlite3_db_param_path, base_path);
    }
    else // interactif
    {
        installPathFlag=checkInstallationPaths(base_path,0);
        if(installPathFlag==2) // au moins 1 répertoire n'existe pas (mais tous les répertoires existants sont accessibles en lecture/écriture).
        {
            installPathFlag=1;
            // doit-on essayer de les creer ?
            printf("Certains repertoires préconisés n'existent pas. Voulez-vous tenter de les créer ? (O/N) : ");
            char c=fgetc(stdin); // a tester
            if(c=='Y' || c=='y' || c=='O' || c=='o')
            {
                installPathFlag=checkInstallationPaths(base_path, 1); // on rebalaye mais cette fois on essaye de créer les repertoires
                if(!installPathFlag) // pas d'erreur tous les repertoires existent maintenant en lecture écriture.
                    printf("Les répertoires ont été créés\n");
            }
        }
        if(installPathFlag==1)
            printf("Certains répertoire recommandés ne sont pas accessible en lecture/écriture ou sont absents. Vous devrez choisir d'autres chemins pour l'installation\n");
        func_ret=interactiveInit(sqlite3_db_param_path, base_path);
    }
    if(func_ret)
        return 1;
    else
        return 0;
}


int16_t checkParamsDb(char *sqlite3_db_param_path, int16_t *cause)
{
   sqlite3 *sqlite3_param_db;

   if( access( sqlite3_db_param_path, F_OK) == -1 ) // file exist ?
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : \"%s\" n'existe pas ou n'est pas accessible.\n",ERROR_STR,__func__,sqlite3_db_param_path);
      *cause=1;
      return 0;
   }
   
   if( access( sqlite3_db_param_path, R_OK | W_OK) == -1 )
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : \"%s\" doit etre accessible en lecture/ecriture.\n",ERROR_STR,__func__,sqlite3_db_param_path);
      *cause=2;
      return 0;
   }

   int16_t ret = sqlite3_open_v2(sqlite3_db_param_path, &sqlite3_param_db, SQLITE_OPEN_READONLY, NULL);
   if(ret)
   {
      VERBOSE(5) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      *cause=3;
      return 0;
   }

   // contrôler le contenu de la base ...
   //printf("Existe : %d\n",tableExist(sqlite3_param_db, "toto"));
   //printf("Existe : %d\n",tableExist(sqlite3_param_db, "interfaces"));
   
   sqlite3_close(sqlite3_param_db);
   
   return 1;
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
  * \param     basepath
  * \return    1 if installation is ok, 0 else
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
        return 1;
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
            return 1;
        
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
                    flag=1; // pas droit en écriture
                }
                else
                    if(flag!=1)
                        flag=2; // repertoire n'existe pas. 1 (erreur de droit irattapable a priorité sur 2 ...)
            }
            else
            {
                VERBOSE(2) {
                    fprintf(stderr,"%s (%s) : %s - ",INFO_STR,__func__,path_to_check);
                    perror("");
                }
                flag=1; // pas droit en écriture
            }
        }
    }
    return flag;
}