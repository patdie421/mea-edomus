//
//  sqldb.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 22/10/13.
//
//
#include "sqlite3db.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

#include <sqlite3.h>

#include "debug.h"


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


int16_t doSqlQueries(sqlite3 *sqlite3_db, char *queries[])
{
   int16_t rc=0;

   for(int16_t i=0;queries[i];i++)
   {
      char *err = NULL;
      
      int16_t ret = sqlite3_exec(sqlite3_db, queries[i], NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) fprintf (stderr, "%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_db));
         sqlite3_free(err);
         rc=1;
         break;
      }
   }
   return rc;
}
