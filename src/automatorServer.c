//
//  automatorServer.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 24/11/2013.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>

#include <sqlite3.h>

#include "xPL.h"

#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
//#include "string_utils.h"

#define XPL_VERSION "0.1a2"

sqlite3 *db;

char *get_rules_sql="\
SELECT \
conditions.id_rule, \
rules.name, \
input_type, \
input_index, \
input_value \
FROM conditions \
JOIN rules ON conditions.id_rule=rules.id_rule \
WHERE (%s) \
AND rules.source='%s' \
AND rules.schema='%s' \
GROUP BY conditions.id_rule \
HAVING COUNT(conditions.id_rule)=rules.nb_conditions;";


int16_t isnumeric(char *s)
{
   float f;
   char *end;
   
   f=strtof(s,&end);
   if(*end!=0 || errno==ERANGE || end==s)
      return 0;
   else
      return 1;
}


static void _automator_stop(int signal_number)
{
   exit(0);
}


uint16_t find_rules(xPL_MessagePtr theMessage)
{
   char xpl_source[48];
   char xpl_schema[48];
   
   char conditions_keys[1024];
   char condition_key[256];
   char sql_query[2048];
   
   snprintf(xpl_source,sizeof(xpl_source),"%s-%s.%s", xPL_getSourceVendor(theMessage), xPL_getSourceDeviceID(theMessage), xPL_getSourceInstanceID(theMessage));
   snprintf(xpl_schema,sizeof(xpl_schema),"%s.%s", xPL_getSchemaClass(theMessage), xPL_getSchemaType(theMessage));

   conditions_keys[0]=0;
   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);

      if(!isnumeric(keyValuePtr->itemValue)) // pour les valeurs non numérique seul l'égalité est possible, on peut donc améliorer la recherche
         snprintf(condition_key,sizeof(condition_key),"(key = '%s' AND value = '%s')", keyValuePtr->itemName, keyValuePtr->itemValue);
      else
         snprintf(condition_key,sizeof(condition_key),"(key = '%s')", keyValuePtr->itemName);

      if(i)
         strcat(conditions_keys," OR ");

      strcat(conditions_keys, condition_key);
   }
   snprintf(sql_query, sizeof(sql_query), get_rules_sql, conditions_keys, xpl_source, xpl_schema);
   
   sqlite3_stmt * stmt;
   int ret = sqlite3_prepare_v2(db, sql_query, strlen(sql_query)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      return -1;
   }
   else
   {
      fprintf(stderr,"CONDITION : %s\n",conditions_keys);
      while (1)
      {
         int s = sqlite3_step (stmt); // sqlite function need int
         if (s == SQLITE_ROW)
         {
            int conditions_id_rule;
            char *rules_name;
            int input_type;
            int input_index;
            char *input_value;

            conditions_id_rule = sqlite3_column_int(stmt, 0);
            rules_name = (char *)sqlite3_column_text(stmt, 1);
            input_type = sqlite3_column_int(stmt, 2);
            input_index = sqlite3_column_int(stmt, 3);
            input_value = (char *)sqlite3_column_text(stmt, 1);
            
            fprintf(stderr,"%d %s %d %d %s\n",conditions_id_rule, rules_name, input_type, input_index, input_value);
         }
         else if (s == SQLITE_DONE)
         {
            sqlite3_finalize(stmt);
            break;
         }
         else
         {
            VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
            sqlite3_finalize(stmt);
            return -1;
         }
      }
   }
   return 0;
}


void printXPLMessage(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   find_rules(theMessage);
}


// 1 thread d'acquisition : trapper en continue les messages xpl
   // Les règles d'acquisition : quelles données utilisées ?
// 1 thread de mise à jour de la table
   // 1 tableau "mémoire de travail" mis à jour par le thread
// une file entre les deux thread

// 1 thread de gestion des timers

// 1 tableau des étapes


void automator()
{
   signal(SIGINT,  _automator_stop);
   signal(SIGQUIT, _automator_stop);
   signal(SIGTERM, _automator_stop);

/*
   while(1)
   {
      // attendre déclenchement : traitement uniquement si changement de données
      
      // acquisition des entrées
         // copie de données collectées dans la mémoire de traitement (les entrées sont figées)
      
      // traitements
      
      // affectation des sorties
      
      sleep(5);
   }
*/
   if ( !xPL_initialize(xPL_getParsedConnectionType()) )
   {
      exit(1);
   }
   
   xPL_ServicePtr xPLService = NULL;
   
   xPLService = xPL_createService("mea", "edomus", "cheznousdeva");
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_addMessageListener(printXPLMessage, NULL);

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            fprintf(stderr,"%s  (%s) : xPLServer thread actif\n",INFO_STR,__func__);
         }
         else
            compteur++;
      }
      
      xPL_processMessages(500);
      pthread_testcancel();
   }
   while (1);

}


pid_t start_automatorServer(char *db_path)
{
   pid_t automator_pid = -1;

   automator_pid = fork();
   
   if (automator_pid == 0) // child
   {
      // Code only executed by child process
      int ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
      if(ret)
      {
         VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__, sqlite3_errmsg (db));
         exit(1);
      }

      automator();
     sqlite3_close(db);

      exit(1);
   }
   else if (automator_pid < 0) // failed to fork
   {
      return -1;
   }

   // Code only executed by parent process
   return automator_pid;
}


