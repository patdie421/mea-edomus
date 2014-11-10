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
#include <pthread.h>
#include <sqlite3.h>

#include "xPL.h"

#include "debug.h"
#include "error.h"
#include "macros.h"
#include "memory.h"
#include "string_utils.h"

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

char *get_conditions_sql="\
SELECT \
name, \
key, \
op, \
value \
FROM conditions \
WHERE id_rule=%d;";


/*
#include <string.h>
#include <stdio.h>
#define bool int
#define false 0
#define true 1

int op_preced(const char c)
{
    switch(c)    {
        case '|':
            return 6;
        case '&':
            return 5;
        case '!':
            return 4;
        case '*':
        case '/':
        case '%':
            return 3;
        case '+':
        case '-':
            return 2;
        case '=':
            return 1;
    }
    return 0;
}

bool op_left_assoc(const char c)
{
    switch(c)    {
        // left to right
        case '*':
        case '/':
        case '%':
        case '+':
        case '-':
        case '|':
        case '&':
            return true;
        // right to left
        case '=':
        case '!':
            return false;
    }
    return false;
}

unsigned int op_arg_count(const char c)
{
    switch(c)  {
        case '*':
        case '/':
        case '%':
        case '+':
        case '-':
        case '=':
        case '&':
        case '|':
            return 2;
        case '!':
            return 1;
        default:
            return c - 'A';
    }
    return 0;
}

#define is_operator(c)  (c == '+' || c == '-' || c == '/' || c == '*' || c == '!' || c == '%' || c == '=' || c == '&' || c == '|')
#define is_function(c)  (c >= 'A' && c <= 'Z')
#define is_ident(c)     ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))

bool shunting_yard(const char *input, char *output)
{
    const char *strpos = input, *strend = input + strlen(input);
    char c, *outpos = output;

    char stack[32];       // operator stack
    unsigned int sl = 0;  // stack length
    char     sc;          // used for record stack element

    while(strpos < strend)   {
        // read one token from the input stream
        c = *strpos;
        if(c != ' ')    {
            // If the token is a number (identifier), then add it to the output queue.
            if(is_ident(c))  {
                *outpos = c; ++outpos;
            }
            // If the token is a function token, then push it onto the stack.
            else if(is_function(c))   {
                stack[sl] = c;
                ++sl;
            }
            // If the token is a function argument separator (e.g., a comma):
            else if(c == ',')   {
                bool pe = false;
                while(sl > 0)   {
                    sc = stack[sl - 1];
                    if(sc == '(')  {
                        pe = true;
                        break;
                    }
                    else  {
                        // Until the token at the top of the stack is a left parenthesis,
                        // pop operators off the stack onto the output queue.
                        *outpos = sc;
                        ++outpos;
                        sl--;
                    }
                }
                // If no left parentheses are encountered, either the separator was misplaced
                // or parentheses were mismatched.
                if(!pe)   {
                    printf("Error: separator or parentheses mismatched\n");
                    return false;
                }
            }
            // If the token is an operator, op1, then:
            else if(is_operator(c))  {
                while(sl > 0)    {
                    sc = stack[sl - 1];
                    if(is_operator(sc) &&
                        ((op_left_assoc(c) && (op_preced(c) >= op_preced(sc))) ||
                           (op_preced(c) > op_preced(sc))))   {
                        // Pop op2 off the stack, onto the output queue;
                        *outpos = sc;
                        ++outpos;
                        sl--;
                    }
                    else   {
                        break;
                    }
                }
                // push op1 onto the stack.
                stack[sl] = c;
                ++sl;
            }
            // If the token is a left parenthesis, then push it onto the stack.
            else if(c == '(')   {
                stack[sl] = c;
                ++sl;
            }
            // If the token is a right parenthesis:
            else if(c == ')')    {
                bool pe = false;
                // Until the token at the top of the stack is a left parenthesis,
                // pop operators off the stack onto the output queue
                while(sl > 0)     {
                    sc = stack[sl - 1];
                    if(sc == '(')    {
                        pe = true;
                        break;
                    }
                    else  {
                        *outpos = sc;
                        ++outpos;
                        sl--;
                    }
                }
                // If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
                if(!pe)  {
                    printf("Error: parentheses mismatched\n");
                    return false;
                }
                // Pop the left parenthesis from the stack, but not onto the output queue.
                sl--;
                // If the token at the top of the stack is a function token, pop it onto the output queue.
                if(sl > 0)   {
                    sc = stack[sl - 1];
                    if(is_function(sc))   {
                        *outpos = sc;
                        ++outpos;
                        sl--;
                    }
                }
            }
            else  {
                printf("Unknown token %c\n", c);
                return false; // Unknown token
            }
        }
        ++strpos;
    }
    // When there are no more tokens to read:
    // While there are still operator tokens in the stack:
    while(sl > 0)  {
        sc = stack[sl - 1];
        if(sc == '(' || sc == ')')   {
            printf("Error: parentheses mismatched\n");
            return false;
        }
        *outpos = sc;
        ++outpos;
        --sl;
    }
    *outpos = 0; // Null terminator
    return true;
}

bool evalBoolExpr(char * expr)  {
    char output[500] = {0};
    char * op;
    bool tmp;
    char part1[250], part2[250];

    if(!shunting_yard(expr, output))
      return false;  // oops can't convert to postfix form

    while (strlen(output) > 1) {
        op = &output[0];
        while (!is_operator(*op) && *op != '\0')
          op++;
        if (*op == '\0') {
          return false;  // oops - zero operators found
        }
        else if (*op == '!') {
            tmp = !(*(op-1) - 48);
            *(op-1) = '\0';
        }
        else if(*op == '&') {
            tmp = (*(op-1) - 48) && (*(op-2) - 48);
            *(op-2) = '\0';
        }
        else if (*op == '|') {
            tmp = (*(op-1) - 48) || (*(op-2) - 48);
            *(op-2) = '\0';
        }

        memset(part1, 0, sizeof(part1));
        memset(part2, 0, sizeof(part2));
        strcpy(part1, output);
        strcpy(part2, op+1);
        memset(output, 0, sizeof(output));
        strcat(output, part1);
        strcat(output, ((tmp==false) ? "0" : "1"));
        strcat(output, part2);
    }
    return *output - 48;
}

int main() {
    char * boolString[2] = {"FALSE", "TRUE"};
    char * expr = "!((1 | 0) & (1 & ((1 & !0) | 0)))";
    bool result = evalBoolExpr(expr);
    printf("Boolean expr. %s is %s", expr, boolString[result]);
    return 0;
}
*/

int16_t isnumeric3(char *s)
{
//   float f;
   char *end;
   
//   f=strtof(s,&end);
   strtof(s,&end);
   if(*end!=0 || errno==ERANGE || end==s)
      return 0;
   else
      return 1;
}

int16_t isnumeric2(char *s, float *v)
{
   char *end;
   float f;
   
   f=strtof(s,&end);
   if(*end!=0 || errno==ERANGE || end==s)
      return 0;
   else
   {
      if(v)
         *v=f;
      return 1;
   }
}

static void _automator_stop(int signal_number)
{
   exit(0);
}

uint8_t digital_inputs[255];
float analog_inputs[255];
uint8_t steps[255]; // les étape
uint8_t evaluation_digital_inputs[255];
float evaluation_analog_inputs[255];

int n;


int16_t InputsToEvaluationBuffer()
{
   memcpy(evaluation_digital_inputs, digital_inputs, 255*sizeof(uint8_t));
   memcpy(evaluation_analog_inputs, analog_inputs, 255*sizeof(float));
}


int16_t set_input(int input_type, int input_index, char *input_value, xPL_NameValueListPtr xpl_body)
{
   char value[128];
   char key[128];
   char *value_ptr;
   
   if(input_value[0]=='#') // traitement d'une fonction
   {
      if(sscanf(input_value,"#valueof(%s)%n", key, &n) == 1 && n == strlen(input_value))
      {
         value_ptr = xPL_getNamedValue(xpl_body, key);
         if(value_ptr)
            strcpy(value,value_ptr);
         else
            return -1;
      }
      else
         return -1; // erreur
   }
   else
      strcpy(value, input_value);
      mea_strtoupper(value);
   
   if(input_type==1) // digital
   {
      int v;
      float fvalue;

      if(strcmp(value,"high")==0)
         v=255;
      else if(strcmp(value,"low")==0)
         v=0;
      else if(isnumeric2(value,&fvalue))
      {
         if(fvalue==0)
            v=0;
         else
            v=255;
      }
      else
         return -1;
      
      // lock la table
      // update_digital_inputs(input_index,v);
      digital_inputs[input_index]=v;
      // delock la table
      return 0;
   }
   else
   {
      float fvalue;

      if(isnumeric2(value,&fvalue))
      {
         // lock la table
         // update_analog_inputs(input_index,v);
         analog_inputs[input_index]=fvalue;
         // delock la table
         return 0;
      }
      else
         return -1;
   }
   
   return 0;
}


void print_digital_inputs()
{
   for(i=0;i<16;i++)
   {
      fprintf(stderr, "%03d : ",i);
      for(j=0;j<16;j++)
      {
         fprintf(stderr, "%03d", digital_inputs[i*j];
      }
   }
}

               
int16_t check_conditions(int conditions_id_rule, xPL_NameValueListPtr xpl_body)
{
   char sql_query[1024];

   int n=snprintf(sql_query,sizeof(sql_query),get_conditions_sql,conditions_id_rule);
   if(n<0 || n==sizeof(sql_query))
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   sqlite3_stmt * stmt;
   int ret = sqlite3_prepare_v2(db, sql_query, strlen(sql_query)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      return -1;
   }
   else
   {
      int16_t ret=0;
      while (1)
      {
         int s = sqlite3_step (stmt); // sqlite function need int
         if (s == SQLITE_ROW)
         {
//            char *name;
            char *key;
            int op;
            char value[128];
            char xpl_value[128];
            
            float fvalue, fxpl_value;
            
//            name = (char *)sqlite3_column_text(stmt, 0);
            key = (char *)sqlite3_column_text(stmt, 1);
            op = sqlite3_column_int(stmt, 2);
            strncpy(value, (char *)sqlite3_column_text(stmt, 3), sizeof(value)-1);
            mea_strtoupper(value);
            
            char *xpl_value_ptr = xPL_getNamedValue(xpl_body, key);
            if(!xpl_value_ptr)
            {
               sqlite3_finalize(stmt);
               return -1;
            }
            strncpy(xpl_value,xpl_value_ptr,sizeof(xpl_value)-1);
            mea_strtoupper(xpl_value);
            
            int16_t isnum=isnumeric2(value,&fvalue) && isnumeric2(xpl_value,&fxpl_value);
            
            switch(op)
            {
               case 1: // '=' égal
                  if(isnum)
                     if(fxpl_value == fvalue)
                        break;
                     else
                        ret=1;
                  else
                     if(strcmp(value,xpl_value)==0)
                        break;
                     else
                        ret=1;
                  break;
               case 2: // '<>' différent
                  if(isnum)
                     if(fxpl_value != fvalue)
                        break;
                     else
                        ret=1;
                  else
                     if(strcmp(value,xpl_value)!=0)
                        break;
                     else
                        ret=1;
                  break;
               case 3: // '<'
                  if(isnum && (fxpl_value < fvalue))
                     break;
                  else
                     ret=1;
                  break;
               case 4: // '<='
                  if(isnum && (fxpl_value <= fvalue))
                     break;
                  else
                     ret=1;
                  break;
               case 5: // '>'
                  if(isnum && (fxpl_value > fvalue))
                     break;
                  else
                     ret=1;
                  break;
               case 6: // '>='
                  if(isnum && (fxpl_value >= fvalue))
                     break;
                  else
                     ret=1;
                  break;
               default:
                  ret=1;
                  break;
            }
            
            if(ret==1)
            {
               sqlite3_finalize(stmt);
               return -1;
            }
         }
         else if (s == SQLITE_DONE)
         {
            sqlite3_finalize(stmt);
            return 0;
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
}


int16_t find_and_process_rules(sqlite3 *db, xPL_MessagePtr theMessage)
{
   char xpl_source[48];
   char xpl_schema[48];
   
   char conditions_keys[1024];
   char condition_key[256];
   char sql_query[2048];
   
   int n;
   
   n=snprintf(xpl_source, sizeof(xpl_source), "%s-%s.%s", xPL_getSourceVendor(theMessage), xPL_getSourceDeviceID(theMessage), xPL_getSourceInstanceID(theMessage));
   if(n<0 || n==sizeof(xpl_source))
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   n=snprintf(xpl_schema, sizeof(xpl_schema), "%s.%s", xPL_getSchemaClass(theMessage), xPL_getSchemaType(theMessage));
   if(n<0 || n==sizeof(xpl_schema))
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }

   conditions_keys[0]=0;
   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);

      if(!isnumeric2(keyValuePtr->itemValue,NULL)) // pour les valeurs non numérique seul l'égalité ou l'inégalité est possible, on peut donc améliorer la 1ere recherche
      {
         n=snprintf(condition_key,sizeof(condition_key),"(key = '%s' AND value = '%s')", keyValuePtr->itemName, keyValuePtr->itemValue);
      }
      else
      {
         n=snprintf(condition_key,sizeof(condition_key),"(key = '%s')", keyValuePtr->itemName);
      }
      if(n<0 || n==sizeof(condition_key))
      {
         VERBOSE(2) {
            fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
         }
         return -1;
      }

      if(i)
         strcat(conditions_keys," OR ");

      strcat(conditions_keys, condition_key);
   }
   n=snprintf(sql_query, sizeof(sql_query), get_rules_sql, conditions_keys, xpl_source, xpl_schema);
   if(n<0 || n==sizeof(sql_query))
   {
      VERBOSE(2) {
            fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
      }
      return -1;
   }
   
   sqlite3_stmt * stmt;
   int ret = sqlite3_prepare_v2(db, sql_query, strlen(sql_query)+1, &stmt, NULL);
   if(ret)
   {
      VERBOSE(2) fprintf (stderr, "%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg (db));
      return -1;
   }
   else
   {
      VERBOSE(9) fprintf(stderr,"%s (%s) : RULE CONDITION - %s\n",INFO_STR,__func__,conditions_keys);
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
            
            if(check_conditions(conditions_id_rule, xpl_body)==0)
            {
               VERBOSE(9) fprintf (stderr, "%s (%s) : RULE %s MATCH :-)\n", INFO_STR,__func__,rules_name);
               ret=set_input(input_type, input_index, input_value, xpl_body);
               print_digital_inputs();
               // si ret==0 déclenchement de l'automate (synchro a définir : signal, condition, ...)
            }
            else
            {
               VERBOSE(9) fprintf (stderr, "%s (%s) : RULE %s NOT MATCH :-(\n", INFO_STR,__func__,rules_name);
            }
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


void acquireInput(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   find_and_process_rules(db, theMessage);
}


void automator(char *db_path)
{
   signal(SIGINT,  _automator_stop);
   signal(SIGQUIT, _automator_stop);
   signal(SIGTERM, _automator_stop);
/*
Chargement de tables en mémoires avec SQLITE3 depuis une autre db (pour les perfs de l’automate par exemple).

1.Création d’une base en mémoire ( utiliser à la place du nom du fichier « :memory: » )
2.Attacher la base à copier
  ATTACH « fic.db » AS sourcedb ;
3.Copier les tables
  CREATE TABLE main._table_à_copier_ AS SELECT * FROM sourcedb._table_à_copier_ ;
  (creation des index et contrainte éventuels, pas possible de les copier)
4.Libération de la base source
  DETACH sourcedb ;

Voir aussi comment prendre en compte les changements sur la table source. Par exemple sur reception d'un signal, l'automate se bloque et on recharge les tables.
*/

   int ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
   if(ret)
   {
      VERBOSE(1) fprintf (stderr, "%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__, sqlite3_errmsg (db));
      exit(1);
   }

   /*
   // cinématique des flux et traitements
   messges xPL --> (Thread d'acquisition des entrées) --[entrée]--> | file tampon asynchrone | --[entrée]--> (thread gestion table entrées) -X-> { table entrées }
      --> (thread automate) --> { table des sorties } -X-> ( thread emission des sorties ) --> messages xPL

   THREAD automate :
      - copie de la table des entrées
      - copie de la table des sorties
      - évaluation des instructions
      - mise à jour de la table des sorties
   */
   if ( !xPL_initialize(xPL_getParsedConnectionType()) )
      exit(1);
   
   xPL_ServicePtr xPLService = NULL;
   
   xPLService = xPL_createService("mea", "edomus", "cheznousdeva");
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_addMessageListener(acquireInput, NULL);

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      VERBOSE(9) {
         static char compteur=0;
         if(compteur>59)
         {
            compteur=0;
            fprintf(stderr,"%s  (%s) : automatorServer thread actif\n",INFO_STR,__func__);
         }
         else
            compteur++;
      }
      
      xPL_processMessages(500);
      pthread_testcancel();
   }
   while (1);
   
   sqlite3_close(db);
}


pid_t start_automatorServer(char *db_path)
{
   pid_t automator_pid = -1;

// ajouter ici la création de deux pipes pour une communication bi-directionnelle avec le process
// utile pour mise en place d'un heartbeat et l'échange d'information entre le moteur et l'automate

   automator_pid = fork();
   
   if (automator_pid == 0) // child
   {
      // Code only executed by child process
      automator(db_path);

      exit(1);
   }
   else if (automator_pid < 0) // failed to fork
      return -1;

   // Code only executed by parent process
   return automator_pid;
}
