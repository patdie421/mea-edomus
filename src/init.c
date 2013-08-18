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

#include "debug.h"
#include "error.h"
#include "string_utils.h"


int tableExit(sqlite3 *sqlite3_param_db, char *table)
{
   sqlite3_stmt * stmt;
   char sql[1024];
   int nb=0;
   
   sprintf(sql,"SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'", table);
   
   int ret = sqlite3_prepare_v2(sqlite3_param_db,sql,strlen(sql)+1,&stmt,NULL);
   if(!ret)
   {
      int s = sqlite3_step (stmt);
   
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


int initMeaEdomus(char *db_path)
{
   return 0;
}


int checkParamsDb(char *sqlite3_db_param_path, int16_t *cause)
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

   int ret = sqlite3_open_v2(sqlite3_db_param_path, &sqlite3_param_db, SQLITE_OPEN_READONLY, NULL);
   if(ret)
   {
      VERBOSE(5) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      *cause=3;
      return 0;
   }

   // contrôler le contenu de la base ...
   printf("Exite : %d\n",tableExit(sqlite3_param_db, "toto"));
   printf("Exite : %d\n",tableExit(sqlite3_param_db, "interfaces"));
   
   sqlite3_close(sqlite3_param_db);
   
   return 1;
}


int checkInstallationPaths(char *base_path)
/**
 * \brief     Verifie si l'installation est conforme aux recommendations.
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
   int16_t error=1;
   
   for(int i=0;pathsList[i];i++)
   {
      sprintf(pathToCheck,"%s/%s",base_path,pathsList[i]);
      if( access(pathToCheck, R_OK | W_OK | X_OK) == -1 )
      {
         VERBOSE(1) fprintf(stderr,"%s (%s) : %s - ",ERROR_STR,__func__,pathsList[i]);
         VERBOSE(1) perror("");
         
         error=0;
      }
   }
   
   return error;
}


