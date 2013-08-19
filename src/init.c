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

#include <sqlite3.h>

#include "init.h"

#include "debug.h"
#include "error.h"
#include "string_utils.h"
#include "types.h"


char *get_and_malloc_path(char *base_path,char *dir,char *question)
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


int16_t dropTable(sqlite3 *sqlite3_param_db, char *table)
{
   char sql[1024];
   char *err = 0;
   
   sprintf(sql,"DROP TABLE IF EXISTS '%s'",table);
   
   int16_t ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
   if( ret != SQLITE_OK )
   {
      VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      sqlite3_free(err);
      return 1;
   }
   else
   {
      return 0;
   }
}
   

int16_t tableExist(sqlite3 *sqlite3_param_db, char *table)
{
   sqlite3_stmt * stmt;
   char sql[1024];
   int16_t nb=0;
   
   sprintf(sql,"SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'", table);
   
   int16_t ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(!ret)
   {
      int16_t s = sqlite3_step (stmt);
      
      if (s == SQLITE_ROW)
      {
         nb = sqlite3_column_int(stmt, 0);
      }
      else if (s==SQLITE_ERROR)
      {
         VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_step - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      }
      
      sqlite3_finalize(stmt);
   }
   else
   {
      VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      return 0;
   }
   
   return nb;
}


int16_t dropDatabase(char *db_path)
{
   unlink(db_path);
   
   return 0;
}


int16_t createMeaTables(sqlite3 *sqlite3_param_db)
{
   char *sql_createTable[] = {
      "CREATE TABLE application_parameters(id INTEGER PRIMARY KEY,key TEXT,value TEXT,complement TEXT)",
      "CREATE TABLE interfaces(id INTEGER PRIMARY KEY,id_interface INTEGER,id_type INTEGER,name TEXT,description TEXT,dev TEXT,parameters TEXT,state INTEGER)",
      "CREATE TABLE locations(id INTEGER PRIMARY KEY,id_location INTEGER,name TEXT,description TEXT)",
      "CREATE TABLE sensors_actuators(id INTEGER PRIMARY KEY,id_sensor_actuator INTEGER,id_type INTEGER,id_interface INTERGER,name TEXT,description TEXT,id_location INTEGER,parameters TEXT,state INTEGER)",
      "CREATE TABLE types(id INTEGER PRIMARY KEY,id_type INTEGER,name TEXT,description TEXT,parameters TEXT)",
      NULL
   };

   int16_t rc=0;

   
   for(int16_t i=0;sql_createTable[i];i++)
   {
      char *err = NULL;
      
      int16_t ret = sqlite3_exec(sqlite3_param_db, sql_createTable[i], NULL, NULL, &err);
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


int populateMeaTypes(sqlite3 *sqlite3_param_db)
{
   struct types_value_s
   {
      int16_t  id_type;
      char *name;
      char *description;
      char *parameters;
   };
   
   struct types_value_s types_values[] = {
      {INTERFACE_TYPE_001,"INTYP01","Interface de type 01",""},
      {INTERFACE_TYPE_002,"INTYP02","Interface de type 02",""},
      {201,"XBEECA","Capteurs et actionneurs a interface XBee",""},
      {1000,"PWRCTR","Capteur de compteur ERDF",""},
      {1001,"ARDINA","Entree interface type 01",""},
      {1002,"ARDOUTD","Sortie interface type 02",""},
      {2000,"XDHT22H","Humidité de DTH22",""},
      {2001,"XDHT22T","Température de DTH22",""},
      {2002,"XDHT22P","Pile de DTH22",""},
      {0,NULL,NULL,NULL}
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
      
      sprintf(sql, "INSERT INTO 'types' (id_type,name,description,parameters) VALUES (%d,'%s','%s','%s')",types_values[i].id_type,types_values[i].name,types_values[i].description,types_values[i].parameters);
      
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


int16_t init(char *sqlite3_db_param_path, char *base_path, char *phpcgi_path, char *phpini_path, char *gui_path)
{
   struct key_value_complement_s
   {
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
      /* 12 */ {NULL,NULL,NULL}
   };
   
   sqlite3 *sqlite3_param_db=NULL;
   int16_t retcode=0;
   char *bufferdb_path=NULL;
   char *plugins_path=NULL;
   
   if(phpini_path)
      keys_values_complement[9].value=phpini_path;
   if(phpcgi_path)
      keys_values_complement[10].value=phpcgi_path;
   if(phpcgi_path)
      keys_values_complement[11].value=gui_path;
   if(base_path)
   {
      int16_t l=strlen(base_path);
      
      bufferdb_path=malloc(l+17); // 17 = len("/var/db/query.db") + 1
      sprintf(bufferdb_path,"%s/%s",base_path,"var/db/query.db");
      keys_values_complement[0].value=bufferdb_path;
      
      plugins_path=malloc(l+13); // 13 = len("/lib/plugins") + 1
      sprintf(plugins_path,"%s/%s",base_path,"lib/plugins");
      keys_values_complement[8].value=plugins_path;
   }
   
   if(dropDatabase(sqlite3_db_param_path))
   {
      retcode=1;
      goto exit_init;
   }
   
   int16_t nerr = sqlite3_open(sqlite3_db_param_path, &sqlite3_param_db);
   if(nerr)
   {
      VERBOSE(5) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      retcode=2;
      goto exit_init;
   }
   
   if(createMeaTables(sqlite3_param_db))
   {
      retcode=3;
      goto exit_init;
   }
   
   if(populateMeaTypes(sqlite3_param_db))
   {
      retcode=4;
      goto exit_init;
   }
   
   char sql[1024];
   char *err = NULL;
   
   for(int16_t i=0;keys_values_complement[i].key;i++)
   {
      int16_t ret;
      
      sprintf(sql,"DELETE FROM 'application_parameters' WHERE key='%s'",keys_values_complement[i].key);
      ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_free(err);
      }
      
      sprintf(sql, "INSERT INTO 'application_parameters' (key,value,complement) VALUES ('%s','%s','%s')",keys_values_complement[i].key, keys_values_complement[i].value, keys_values_complement[i].complement);
      
      ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_free(err);
         retcode=5;
         break;
      }
   }
   
exit_init:
   if(sqlite3_param_db)
      sqlite3_close(sqlite3_param_db);
   if(bufferdb_path)
      free(bufferdb_path);
   if(plugins_path)
      free(plugins_path);
   
   return retcode;
}


int16_t autoInit()
{
   return 0;
}


int16_t interactiveInit(char *sqlite3_db_param_path, char *base_path)
{
   char *phpcgi_path=NULL;
   char *phpini_path=NULL;
   char *gui_path=NULL;
   
   phpcgi_path=get_and_malloc_path(base_path,"bin","PATH to 'cgi-bin' directory");
   phpini_path=get_and_malloc_path(base_path,"etc","PATH to 'php.ini' directory");
   gui_path=get_and_malloc_path(base_path,"gui","PATH to gui directory");
   
   // contrôler cgi-bin, php.ini et gui (presence index.cgi)
   // si php.ini absent, proposition d'en creer 1
   // si cgi-bin absent => informer que l'interface graphique ne peut pas fonctionner
   // si gui/index.cgi absent => informer que l'interface ne peut pas fonctionner
   
   init(sqlite3_db_param_path, base_path, phpcgi_path, phpini_path, gui_path);
   
   if(phpcgi_path)
      free(phpcgi_path);
   if(phpini_path)
      free(phpini_path);
   if(gui_path)
      free(gui_path);
   
   return 0;
}


int16_t initMeaEdomus(int16_t mode, char *sqlite3_db_param_path, char *base_path)
{
   int16_t installPathFlag=0;
   int16_t ret;
   
   
   if(base_path)
      installPathFlag=checkInstallationPaths(base_path);
   
   if(mode==1)
   {
      if(installPathFlag==0)
      {
         return 1;
      }
      ret=autoInit();
   }
   else
   {
      if(installPathFlag==0)
         ret=interactiveInit(sqlite3_db_param_path, NULL);
      else
         ret=interactiveInit(sqlite3_db_param_path, base_path);
   }
   
   if(ret)
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


int16_t checkInstallationPaths(char *base_path)
/**
 * \brief     Verifie que l'installation est conforme aux recommendations.
 * \details   Les répertoires suivants doivent exister :
 *            BASEPATH/bin
 *            BASEPATH/etc
 *            BASEPATH/lib
 *            BASEPATH/lib/plugins
 *            BASEPATH/var
 *            BASEPATH/var/db
 *            BASEPATH/var/log
 *            BASEPATH/gui
 * \param     basepath
 * \return    1 if installation is ok, 0 else
 */
{
   const char *pathsList[]={"bin","etc","lib","lib/plugins","var","var/db","var/log","gui",NULL};
   char pathToCheck[256];
   int16_t flag=1;
   
   for(int16_t i=0;pathsList[i];i++)
   {
      sprintf(pathToCheck,"%s/%s",base_path,pathsList[i]);
      if( access(pathToCheck, R_OK | W_OK | X_OK) == -1 )
      {
         VERBOSE(9) fprintf(stderr,"%s (%s) : %s/%s - ",INFO_STR,__func__,base_path,pathsList[i]);
         VERBOSE(9) perror("");
         
         flag=0;
      }
   }
   
   return flag;
}


