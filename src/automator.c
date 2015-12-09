//
//  automator.c
//
//  Created by Patrice DIETSCH on 05/12/15.
//
//

#define DEBUGFLAGON 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "automator.h"
#include "automatorServer.h"

#include "globals.h"
#include "consts.h"
#include "xPL.h"
#include "mea_verbose.h"
#include "mea_queue.h"
#include "mea_timer.h"
#include "tokens.h"
#include "tokens_da.h"
#include "xPLServer.h"
#include "mea_string_utils.h"
#include "processManager.h"
#include "cJSON.h"
#include "uthash.h"

#define DEBUG
#define USEALLOCA

// RULES :
// xpl-trig: source = mea-edomus.home, destination = *, schema = sensor.basic, body = [device = conso, type = power, current = 274.591704]
// xpl-trig: source = mea-edomus.home, destination = *, schema = sensor.basic, body = [device = epgstat01, current = 1]
/*
char *outputs_rules="[ \
   { \
      \"name\": \"O1\", \
      \"action\" : \"xPLSend\", \
      \"parameters\" : [ \
         { \"msgtype\"  : \"'trigger'\" }, \
         { \"schema\"  : \"'sensor.basic'\" }, \
         { \"device\"  : \"'conso01'\" }, \
         { \"current\" : \"&high\" } \
      ], \
      \"condition\" : { \"'V1'\" : \"rise\"} \
   }, \
   { \
      \"name\": \"O2\", \
      \"action\" : \"xPLSend\", \
      \"parameters\" : [ \
         { \"schema\"  : \"'control.basic'\" }, \
         { \"device\"  : \"'toto'\" }, \
         { \"current\" : \"{E3}\" } \
      ], \
      \"condition\" : { \"'E3'\" : \"fall\"} \
   }, \
   { \
      \"name\": \"O3\", \
      \"action\" : \"xPLSend\", \
      \"parameters\" : [ \
         { \"schema\"  : \"'control.basic'\" }, \
         { \"target\"  : \"'mea-edomus.test'\" }, \
         { \"device\"  : \"'tata'\" }, \
         { \"current\" : \"{E3}\" } \
      ], \
      \"condition\" : { \"'E3'\" : \"change\"} \
   } \
]";


char *inputs_rules="[ \
   { \
      \"name\": \"V1\", \
      \"value\" : \"$now()\", \
      \"onmatch\": \"moveforward 'V2'\" \
   }, \
   { \
      \"name\": \"<NOP>\", \
      \"value\" : \"#0\", \
      \"conditions\" : [ \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'hbeat.app'\" } \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"<NOP>\", \
      \"value\" : \"#0\", \
      \"conditions\" : [ \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'watchdog.basic'\" } \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"STOPEVAL\", \
      \"value\" : \"current\", \
      \"conditions\" : [ \
         { \"value1\" : \"source\", \"op\" : \"==\", \"value2\" : \"'mea-edomus.home'\" }, \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'sensor.basic'\"}, \
         { \"value1\" : \"device\", \"op\" : \"==\", \"value2\" : \"'stopeval01'\"} \
      ], \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"BREAK\", \
      \"value\" : \"#0\", \
      \"conditions\" : [ \
         { \"value1\" : \"{STOPEVAL}\", \"op\" : \"==\", \"value2\" : \"&true\" } \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"T1\", \
      \"value\" : \"#0\", \
      \"conditions\" : [ \
         { \"value1\" : \"$exist('T1')\", \"op\" : \"==\", \"value\" : \"&false\" } \
      ], \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"V2\", \
      \"value\" : \"{E1}\", \
      \"onmatch\": \"continue\" \
   }, \
   { \
      \"name\": \"E1\", \
      \"value\" : \"current\", \
      \"conditions\" : [ \
         { \"value1\" : \"device\", \"op\" : \"==\", \"value2\" : \"'BUTTON1'\"}, \
         { \"value1\" : \"source\", \"op\" : \"==\", \"value2\" : \"'mea-edomus.home'\" }, \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'sensor.basic'\"}, \
         { \"value1\" : \"type\",   \"op\" : \"==\", \"value2\" : \"'input'\"} \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"E2\", \
      \"value\" : \"current\", \
      \"conditions\" : [ \
         { \"value1\" : \"device\", \"op\" : \"==\", \"value2\" : \"'BUTTON2'\"}, \
         { \"value1\" : \"source\", \"op\" : \"==\", \"value2\" : \"'mea-edomus.home'\" }, \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'sensor.basic'\"}, \
         { \"value1\" : \"type\",   \"op\" : \"==\", \"value2\" : \"'input'\"} \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"E3\", \
      \"value\" : \"current\", \
      \"conditions\" : [ \
         { \"value1\" : \"device\", \"op\" : \"==\", \"value2\" : \"'conso'\"}, \
         { \"value1\" : \"source\", \"op\" : \"==\", \"value2\" : \"'mea-edomus.home'\" }, \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'sensor.basic'\"}, \
         { \"value1\" : \"type\",   \"op\" : \"==\", \"value2\" : \"'power'\"} \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"TEMP1\", \
      \"value\" : \"current\", \
      \"conditions\" : [ \
         { \"value1\" : \"device\", \"op\" : \"==\", \"value2\" : \"'temp01'\"}, \
         { \"value1\" : \"source\", \"op\" : \"==\", \"value2\" : \"'mea-edomus.home'\" }, \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'sensor.basic'\"} \
      ], \
      \"onmatch\": \"break\" \
   }, \
   { \
      \"name\": \"E4\", \
      \"value\" : \"current\", \
      \"conditions\" : [ \
         { \"value1\" : \"device\", \"op\" : \"==\", \"value2\" : \"'epgstat01'\"}, \
         { \"value1\" : \"source\", \"op\" : \"==\", \"value2\" : \"'mea-edomus.home'\" }, \
         { \"value1\" : \"schema\", \"op\" : \"==\", \"value2\" : \"'sensor.basic'\"} \
      ], \
      \"onmatch\": \"break\" \
   } \
]";
*/
/*
# les règles sont évaluées dans l'ordre. Par defaut toutes les règles sont évaluées l'une après l'autre
# ce comportement par défaut est modifiée par "onmatch:" (break, continue, moveforward, ...)
#
# syntaxe d'une règle d'entréee :
# <inputname> is: <value> | <"<NOP>">
# <inputname> is: <value> | <"<NOP>"> if: ( <condition1> [, condition2] ... [, conditionN])
# <inputname> is: <value> | <"<NOP>"> onmatch: <matchvalue>
# <inputname> is: <value> | <"<NOP>"> if: ( <condition1> [, condition2] ... [, conditionN]) onmatch: <matchvalue>
#
# avec :
# <value> :
#   key (valeur associée à une clé dans le message xPL)
#   'chaine'
#   #numerique
#   &boolean (&true, &false, &high, &low)
#   $function()
#
# D1 is: <NOP>     if: (schema == 'hbeat.app') onmatch: break
# V1 is: #1
# V2 is: #2.1      if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON3', current == #2) onmatch: break
# V2 is: #10       if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON3', current > #3, current < #5) onmatch: continue
# E1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON1', type == 'input') onmatch: break
# E2 is: last      if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON2') onmatch: continue
# T2 is: $now()    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON2') onmatch: continue
# P1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'CONSO', type == 'power') onmatch: continue
# P2 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'CONSO', type == 'power', current != #0) onmatch: continue
# P3 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'PROD',  type == 'power', current != {V1}) onmatch: continue
# C1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'PROD',  type == 'power', {T2}>0 ) onmatch: moveforward 'S2'
# B1 is: &false    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'TEMP',  type == 'temp',  current > 0)   onmatch: continue
# S1 is: 'toto'    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'TEMP',  type == 'temp',  current <= #0) onmatch: continue
# S2 is: &true     if: ($timer('timer1')==&false) onmatch: continue (à faire pour les timer)
# S2 is: &false    if: ($timer('timer1')==&true) onmatch: continue (à faire pour les timer)

# des exemples de règles "compliqués"
# T1_last is: {T1} if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high)
# T1 is: $now() if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high)
# DIFF is: $calc({T2} - {T2_last}) if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high)
# P1 is: &high if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high, {DIFF} > #1000)
# P1 is: &low  if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high, {DIFF} <= #1000)
# exemple de compteur
# C1 is: #0 if: $exist('C1') == &false // initialisation du compteur
# C1 is: $calc({C1}+#1) (à faire)
#

# Pour les actions :
#
# A1 do: xPLSend with: (schema='control.basic', device='toto', current={E1}) when: 'C1' rise
# A2 do: xPLSend with: (schema='control.basic', device='toto', current={E1}) when: 'C1' fall
# A3 do: xPLSend with: (schema='control.basic', device='tata', current={E1}) when: 'C1' change
# A4 do: xPLSend with: (schema='control.basic', device='tata', current=$eval(!{E1})) when: 'C1' change
# A5 do: timerstart with: (name='timer1', value=10, unit='s', autorestart=&false) when: 'E1' rise (à faire)
*/

cJSON *_rules;
cJSON *_inputs_rules;
cJSON *_outputs_rules;

enum input_state_e { NEW, RISE, FALL, STAY, CHANGE, TYPECHANGE, UNKNOWN };

struct value_s {
   char type;
   union {
      char strval[41];
      double floatval;
      char booleanval;
   } val;
};

struct inputs_table_s
{
   char name[21];
   enum input_state_e state;
   struct value_s v;

   UT_hash_handle hh;
};

struct inputs_table_s *inputs_table = NULL;


int reset_inputs_table_change_flag();
static int automator_add_to_inputs_table(char *name, struct value_s *v);
static int automator_print_inputs_table();

static int evalStr(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr);

static int getBoolean(char *s, char *b)
{
   *b=-1;
   if(mea_strcmplower(s, "true")==0 ||
      mea_strcmplower(s, "high")==0)
   {
      *b=1;
      return 0;
   }
   else if(mea_strcmplower(s, "false")==0 ||
           mea_strcmplower(s, "low")==0)
   {
      *b=0;
      return 0;
   }
   return -1;
}


static int getNumber(char *s, double *v)
{
   char *n;
 
   *v=strtod(s, &n);

   if(*n!=0)
      return -1;
   else
      return 0;
}


static int _setValueFromStr(struct value_s *v, char *str, char trimFlag)
{
   double f;
   char b;
   char _str[sizeof(v->val.strval)];
   char *p;

   if(trimFlag!=0)
      p=mea_strtrim(strcpy(_str, v->val.strval));
   else
      p=str;

   if(getNumber(p, &f)==0)
   {
      v->type=0;
      v->val.floatval=f; 
   }
   else if(getBoolean(p, &b)==0)
   {
      v->type=2;
      v->val.booleanval=b;
   }
   else
   {
      v->type=1;
      strncpy(v->val.strval, str, sizeof(v->val.strval)); 
      v->val.strval[sizeof(v->val.strval)-1]=0;
   }
   return 0;
}


static int setValueFromStr(struct value_s *v, char *str)
{
   return _setValueFromStr(v, str, 0);
}


static int setValueFromTrimedStr(struct value_s *v, char *str)
{
   return _setValueFromStr(v, str, 1);
}


enum conversion_e { INT = 0x01, FLOAT=0x02, HIGHLOW=0x04, TRUEFALSE=0x08, DEFAULT=0x00 };

static int valueToStr(struct value_s *v, char *str, int l_str, enum conversion_e flag)
{
   if(v->type==0)
      if(flag & INT)
         snprintf(str,l_str,"%d", (int)v->val.floatval);
      else
         snprintf(str,l_str,"%f", v->val.floatval);
   else if(v->type==1)
   {
      strncpy(str,v->val.strval,l_str-1);
      str[l_str-1]=0;
   }
   else
   {
      if(flag & HIGHLOW)
         if(v->val.booleanval==0)
            strcpy(str, "low");
         else
            strcpy(str, "high");
      else if(flag & TRUEFALSE)
         if(v->val.booleanval==0)
            strcpy(str, "false");
         else
            strcpy(str, "true");
      else
         snprintf(str,l_str,"%d",v->val.booleanval);
   }

   return 0;
}


char *cmpOperators[] = {"==","!=",">","<",">=","<=",NULL};
enum operator_e { O_EQ=0, O_NE, O_GR, O_LO, O_GE, O_LE };

static int getOperator(char *str)
{
   int operatorNum=-1;
   for(int i=0;cmpOperators[i];i++)
   {
      if(strcmp(cmpOperators[i], str)==0)
      {
         operatorNum=i;
         break;
      }
   }
   return operatorNum;
}

 
static int valueCmp(struct value_s *v1, int operator, struct value_s *v2)
{
   // cas type différent : seule la différence peut retourner 1
   if(v1->type != v2->type) {
      if(operator == O_NE) {
         return 1;
      }
      else {
         return 0;
      }
   }
   // autres cas
   int cmp=0; 
   if(v1->type == 0) // numérique
   {
      double diff=v1->val.floatval - v2->val.floatval;
      if(diff==0.0)
         cmp=0;
      else if(diff<0)
         cmp=-1;
      else
         cmp=1;
   } 
   else if(v1->type == 1) // chaine
      cmp=mea_strcmplower(v1->val.strval, v2->val.strval);
   else // boolean
      cmp=v1->val.booleanval - v2->val.booleanval;

   switch(operator)
   {
      case O_EQ:
         if(cmp==0) return 1; 
         break;
      case O_NE:
         if(cmp!=0) return 1;
         break;
      case O_GR:
         if(cmp>0)  return 1;
         break;
      case O_LO:
         if(cmp<0)  return 1;
         break;
      case O_GE:
         if(cmp>=0) return 1;
         break;
      case O_LE:
         if(cmp<=0) return 1;
         break;
      default:
         return -1;
   }
   return 0;
}


static int valuePrint(struct value_s *v)
{
   if(v->type==0)
      fprintf(stderr,"(float)%f",v->val.floatval);
   else if(v->type==1)
      fprintf(stderr,"(string)%s",v->val.strval);
   else
      fprintf(stderr,"(boolean)%d",v->val.booleanval);

   return 0;
}


char *functionsList[]={"now", "exist", "rise", "fall", "stay", "change", NULL };
enum function_e { F_NOW=0, F_EXIST, F_RISE, F_FALL, F_STAY, F_CHANGE };

static int getFunctionNum(char *str, char *params, int l_params)
{
   int ls=strlen(str); 
   for(int i=0; functionsList[i]; i++)
   {
      int lf=strlen(functionsList[i]);
      if(strstr(str,functionsList[i])==str)
      {
         if(str[lf]=='(' && str[ls-1]==')')
         {
            int lp = ls-lf-2;
            if(lp>l_params)
               lp=l_params;
            strncpy(params, &(str[lf+1]), lp);
            params[lp]=0;
            return 0;
         }
         else
            return -1;
      }
   }
   return -1;
}


static int getInputEdge(char *expr, int direction,  struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   if((direction==CHANGE ||
       direction==RISE   ||
       direction==STAY   ||
       direction==FALL)  &&
      expr[0]!=0)
   {
      struct value_s r;
      struct inputs_table_s *e = NULL;

      int ret=evalStr(expr, &r, ListNomsValeursPtr);
      if(ret==0 && r.type==1)
      {
         v->type=2;
         HASH_FIND_STR(inputs_table, r.val.strval, e);
         if(e)
         {
            if(direction != CHANGE)
               v->val.booleanval=(e->state==direction);
            else
               v->val.booleanval=(e->state==CHANGE || e->state==RISE  || e->state==FALL || e->state==TYPECHANGE);
            return 0;
         }
      }
   }
   return -1;
}


static int callFunction(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   char *_f, *f;
   int retour=-1;

#ifndef USEALLOCA
   _f=(char *)malloc(strlen(str)+1);
#else
   _f=(char *)alloca(strlen(str)+1);
#endif

   if(_f==NULL)
      return -1;
   strcpy(_f, str);
   f=mea_strtrim(_f);
    
   char params[256];
 
   int fn=getFunctionNum(f, params, sizeof(params));
   switch(fn)
   {
      case F_NOW:
         if(params[0]==0)
         {
            time_t t=time(NULL);
            v->type=0;
            v->val.floatval=(double)t;
            retour=0;
         }
         break;
      case F_EXIST:
         if(params[0]!=0)
         {
            struct value_s r;
            struct inputs_table_s *e = NULL;

            int ret=evalStr(params, &r, ListNomsValeursPtr);
            if(ret==0 && r.type==1)
            {
               v->type=2;
               HASH_FIND_STR(inputs_table, r.val.strval, e);
               if(e)
                  v->val.booleanval=1;
               else
                  v->val.booleanval=0;
               retour=0;
            }
         }
         break;
      case F_RISE:
         retour=getInputEdge(params, RISE, v, ListNomsValeursPtr);
         break;
      case F_FALL:
         retour=getInputEdge(params, FALL, v, ListNomsValeursPtr);
         break;
      case F_STAY:
         retour=getInputEdge(params, STAY, v, ListNomsValeursPtr);
         break;
      case F_CHANGE:
         retour=getInputEdge(params, CHANGE, v, ListNomsValeursPtr);
         break;
   }

//callFunction_clean_exit:
#ifndef USEALLOCA
   if(_f)
     free(_f);
#endif
   return retour;
}


static int evalStr(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   char *p=NULL;
   char _str[sizeof(v->val.strval)];

   strncpy(_str, str, sizeof(v->val.strval)-1);
   _str[sizeof(v->val.strval)-1]=0;
   p=mea_strtrim(_str);

   if(p[0]=='#') // une constante numérique
   {
      double f=0;
      if(getNumber(&p[1], &f)==0)
      {
         v->type=0;
         v->val.floatval=f;
      }
      else
         return -1;
   }
   else if(p[0]=='\'') // une constante chaine de caractères
   {
      int l=strlen(p);
      if(p[l-1]!='\'')
         return -1;

      l=l-2;
      if(l>sizeof(v->val.strval)-1)
         l=sizeof(v->val.strval)-1;
      strncpy(v->val.strval, &(p[1]), l);
      v->val.strval[l]=0; // pour supprimer le "'" en fin de chaine
      v->type=1;
      return 0;
   }
   else if(p[0]=='&') // une constante booleen
   {
      char b=0;
      if(getBoolean(&p[1], &b)==0)
      {
         v->type=2;
         v->val.booleanval=b;
      }
      else
         return -1;
      return 0;
   }
   else if(p[0]=='$')
   {
      int ret=callFunction(&(p[1]), v, ListNomsValeursPtr);
      return ret;
   }
   else if(p[0]=='{')
   {
      int l=strlen(p);
      if(p[l-1]!='}')
         return -1;

      char name[21];
      struct inputs_table_s *e = NULL;

      if(l>(sizeof(name)+2))
         l=sizeof(name)+2;
      strncpy(name, &(p[1]), l-2);
      name[l-2]=0;

      char *_p=mea_strtrim(name);
      HASH_FIND_STR(inputs_table, _p, e);
     if(e)
        memcpy(v, &(e->v), sizeof(struct value_s));
     else
        return -1;
     return 0;
   }
   else if(p[0]=='<')
   {
      int l=strlen(p);
      if(p[l-1]!='>')
          return -1; 

      if(strstr(p,"NOP")==&(p[1]) && l==5)
      {
         strncpy(v->val.strval, &(p[1]), 5);
         v->val.strval[5]=0;
      }
      else
         return -1;
   }
   else
   {
      char *_value=xPL_getNamedValue(ListNomsValeursPtr, p);
      if(_value!=NULL)
         setValueFromStr(v, _value);
      else
         return -1;
      return 0;
   }
   return 0;
}


static int automator_sendxpl(cJSON *parameters)
{
   if(parameters==NULL || parameters->child==NULL)
      return -1;

   xPL_ServicePtr servicePtr = mea_getXPLServicePtr();
   if(!servicePtr)
      return -1;

   xPL_MessagePtr xplMessage = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_COMMAND);
   if(!xplMessage)
      return -1;

   cJSON *e=parameters->child;

   int retour=0;
   char schema[21]="control.basic";
   char target[41]="";
   int msgtype=xPL_MESSAGE_COMMAND;
   while(e)
   {
      struct value_s v;
     
      if(evalStr(e->child->valuestring, &v, NULL)==0)
      {
         // type de message
         if(strcmp(e->child->string, "msgtype")==0)
         {
            if(v.type==1)
            {
               if(strcmp(v.val.strval,"trigger")==0)
                  msgtype=xPL_MESSAGE_TRIGGER;
               else if(strcmp(v.val.strval,"status")==0)
                  msgtype=xPL_MESSAGE_STATUS;
               else if(strcmp(v.val.strval,"command")==0)
                  msgtype=xPL_MESSAGE_COMMAND;
               else {
                  retour=-1;
                  break;
               }
            }
            else {
               retour=-1;
               break;
            }
         }
         else if(strcmp(e->child->string, "schema")==0) {
         
            strncpy(schema, v.val.strval, sizeof(schema)-1);
            schema[sizeof(schema)-1]=0;
         }
         else if(strcmp(e->child->string,"source")==0) {
            retour=-1;
            break;
         }
         else if(strcmp(e->child->string,"target")==0) {
            strncpy(target, v.val.strval, sizeof(target)-1);
            target[sizeof(target)-1]=0;
         }
         else
         {
            char s[sizeof(v.val.strval)];
            valueToStr(&v, s, sizeof(v.val.strval), INT | HIGHLOW);
            xPL_setMessageNamedValue(xplMessage, e->child->string, s);
         }
      }
      else {
         retour=-1;
         break;
      }
      e=e->next;
   }
   if(retour==-1) { 
      xPL_releaseMessage(xplMessage);
      return -1;
   }

   // changement du type de message
   xplMessage->messageType = msgtype;

   // mise en place du schema
   char class[21]; char type[21];
   sscanf(schema,"%[^.].%s\n", class, type);
   xPL_setSchema(xplMessage, class, type);

   // changement du target si nécessaire
   if(target[0]!=0) {
   }

   // emission du message
   automator_xplout_indicator++;
   mea_sendXPLMessage(xplMessage);

//   displayXPLMsg(xplMessage);

   xPL_releaseMessage(xplMessage);
 
   return 0;
}


int automator_play_output_rules(cJSON *rules)
{
   if(rules==NULL)
   {
      fprintf(stderr,"NO OUTPUT RULE\n");
      return -1;
   }

   double start=mea_now();

   cJSON *e=rules->child;
   while(e) // balayage des règles
   {
//      cJSON *name       = cJSON_GetObjectItem(e,"name");
      cJSON *action     = cJSON_GetObjectItem(e,"action");
      cJSON *parameters = cJSON_GetObjectItem(e,"parameters");
      cJSON *condition  = cJSON_GetObjectItem(e,"condition");
      
      struct value_s v;
      if(evalStr(condition->child->string, &v, NULL)!=0)
      {
         fprintf(stderr,"%s : can't eval\n", condition->child->string);
         goto next_rule;
      }
      if(v.type!=1)
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"%s : evaluation result not a string\n", condition->child->string);
         goto next_rule;
      }
      
      struct inputs_table_s *i = NULL;
      HASH_FIND_STR(inputs_table, v.val.strval, i);
      if(i==NULL)
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"Input rule (%s) no data found\n", v.val.strval);
         goto next_rule;
      }
     
      int actionFlag=0; 
      if(strcmp(condition->child->valuestring, "rise")==0)
      {
         if(i->state==RISE)
            actionFlag=1; 
      }
      else if(strcmp(condition->child->valuestring, "fall")==0)
      {
         if(i->state==FALL)
            actionFlag=1; 
      }
      else if(strcmp(condition->child->valuestring, "change")==0)
      {
         if(i->state==CHANGE || i->state==RISE || i->state==FALL)
            actionFlag=1; 
      }
      else
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"condition error\n");
         goto next_rule;
      }

      if(actionFlag==1)
      {
         if(strcmp(action->valuestring, "xPLSend")==0)
            automator_sendxpl(parameters);
      } 

next_rule:
      e=e->next;
   }
   
   double now=mea_now();

   {
      static mea_timer_t calc_timer;
      static char istimerinit=0;
      static int output_cntr=0;
      static long exectime=0;

      if(istimerinit==0)
      {
         mea_init_timer(&calc_timer, 5, 1);
         mea_start_timer(&calc_timer);
         istimerinit=1;
      }

      exectime=exectime+(long)((now-start)*1000);
      output_cntr++;

      if(mea_test_timer(&calc_timer)==0)
      {
         process_update_indicator(_automatorServer_monitoring_id, automator_output_exec_time_str, exectime/(output_cntr+1));
         output_cntr=0;
         exectime=0L;
      }

      DEBUG_SECTION2(DEBUGFLAGON) {
         fprintf(stderr,"\noutputs rules processing time=%ld us\n", exectime);
      } 
   }

   return 0;
}


int automator_getValue(cJSON *value, struct value_s *v, char *source, char *schema, xPL_NameValueListPtr ListNomsValeursPtr)
{
   int ret=0;
   
   if(strcmp(value->valuestring, "source")==0)
      ret=setValueFromStr(v, source);
   else if(strcmp(value->valuestring, "schema")==0)
      ret=setValueFromStr(v, schema);
   else
      ret=evalStr(value->valuestring, v, ListNomsValeursPtr);
   
   return ret;
}


int automator_match_inputs_rules(cJSON *rules, xPL_MessagePtr message)
{
   if(rules==NULL)
   {
      fprintf(stderr,"NO INPUT RULE\n");
      return -1;
   }

   double start=mea_now();

   xPL_NameValueListPtr ListNomsValeursPtr = NULL;
   char *schema_type = NULL, *schema_class = NULL, *vendor = NULL, *deviceID = NULL, *instanceID = NULL;
   char source[80]="";
   char schema[80]="";
   
   if(message)
   {
      schema_class = xPL_getSchemaClass(message);
      schema_type  = xPL_getSchemaType(message);
      vendor       = xPL_getSourceVendor(message);
      deviceID     = xPL_getSourceDeviceID(message);
      instanceID   = xPL_getSourceInstanceID(message);

      ListNomsValeursPtr = xPL_getMessageBody(message);
      
      sprintf(source,"%s-%s.%s", vendor, deviceID, instanceID);
      sprintf(schema,"%s.%s", schema_class, schema_type);
   }

   cJSON *e=rules->child;
   while(e) // balayage des règles
   {
      int match=1;
      
      struct value_s res, val1, val2;
      
      cJSON *name    = cJSON_GetObjectItem(e,"name");
      cJSON *value   = cJSON_GetObjectItem(e,"value");
      cJSON *onmatch = cJSON_GetObjectItem(e,"onmatch");
      
      if(!name || !value)
      {
         e=e->next;
         continue;
      }

      DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"\nRULE : %s\n", name->valuestring);
      int ret=evalStr(value->valuestring, &res, ListNomsValeursPtr);
      if(ret<0)
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"   [%s] not found\n", value->valuestring);
         match=0;
         goto next_rule;
      }
      DEBUG_SECTION2(DEBUGFLAGON) { fprintf(stderr,"   RES = "); valuePrint(&res); fprintf(stderr," (%s)\n",  value->valuestring); }
      // évaluation des conditions
      cJSON *conditions=cJSON_GetObjectItem(e,"conditions");
      if(conditions!=NULL)
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"   CONDITIONS : \n");
         cJSON *c=conditions->child;
         while(c)
         {
            int operator=-1;
            cJSON *value1 = cJSON_GetObjectItem(c, "value1");
            cJSON *op     = cJSON_GetObjectItem(c, "op");
            cJSON *value2 = cJSON_GetObjectItem(c, "value2");
            if(!value1 || !op || !value2)
            {
               match=0;
               goto next_rule;
            }

            // récuperation de l'opération
            operator=getOperator(op->valuestring);
            if(operator<0)
            {
               match=0;
               goto next_rule;
            }

            if(automator_getValue(value1, &val1, source, schema, ListNomsValeursPtr)<0)
            {
               match=0;
               goto next_rule;
            }
            
            if(automator_getValue(value2, &val2, source, schema, ListNomsValeursPtr)<0)
            {
               match=0;
               goto next_rule;
            }

            // comparaison
            if(valueCmp(&val1, operator, &val2) < 1)
            {
               match=0;
               goto next_rule;
            }

            c=c->next;
         }
      }
      else
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"   NO CONDITION\n");
      }

   next_rule:
      if(match==1)
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"   MATCH !\n");
         if(strcmp(name->valuestring, "<NOP>")!=0)
            automator_add_to_inputs_table(name->valuestring, &res);
         else
         {
            DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr, "   Result discarded\n");
         }

         if(onmatch) // post action
         {
            char action[41]="";
            strncpy(action, onmatch->valuestring, sizeof(action)-1);
            action[sizeof(action)-1]=0;
            // découpage de la chaine si nécessaire
            char *p=NULL;
            for(int i=0;action[i];i++)
            {
               if(isspace(action[i]))
               {
                  p=&(action[i+1]);
                  action[i]=0;
                  break;
               }
            }
            if(mea_strcmplower(action, "break")==0 && !p)
               break;
            else if(mea_strcmplower(action, "continue")==0 && !p)
            {
            }
            else if(mea_strcmplower(action, "moveforward")==0 && p)
            {
               struct value_s r;
               cJSON *_e = e->next;
               int flag=0;
               if(evalStr(p, &r, NULL)==0 && r.type==1)
               {
                  while(_e)
                  {
                     if(cJSON_GetObjectItem(_e,"name") && strcmp(cJSON_GetObjectItem(_e,"name")->valuestring,r.val.strval)==0)
                     {
                        e=_e; // règle en avant trouvée, on y va
                        flag=1;
                        break;
                     }
                     _e=_e->next;
                  }
                  if(flag==1)
                     continue; 
               }
               else
                  break; // erreur
            }
            else
            {
            // erreur
            } 
         }
      }
      else
      {
         DEBUG_SECTION2(DEBUGFLAGON) fprintf(stderr,"   NOT MATCH !\n");
      }
      e=e->next;
   }

   double now=mea_now();

   {
      static mea_timer_t calc_timer;
      static char istimerinit=0;
      static int input_cntr=0;
      static long exectime=0;

      if(istimerinit==0)
      {
         mea_init_timer(&calc_timer, 5, 1);
         mea_start_timer(&calc_timer);
         istimerinit=1;
      }

      exectime=exectime+(long)((now-start)*1000);
      input_cntr++;

      if(mea_test_timer(&calc_timer)==0)
      {
         process_update_indicator(_automatorServer_monitoring_id, automator_input_exec_time_str, exectime/(input_cntr+1));
         input_cntr=0;
         exectime=0L;
      }

      DEBUG_SECTION2(DEBUGFLAGON) {
         fprintf(stderr,"\ninputs rules processing time=%ld us\n", exectime);
         automator_print_inputs_table();
      } 
   }

   return 0;
}


static int automator_add_to_inputs_table(char *_name, struct value_s *v)
{
   struct inputs_table_s *e = NULL;

   HASH_FIND_STR(inputs_table, _name, e);
   if(e)
   {
      enum input_state_e state = UNKNOWN;
      
      if((e->v.type == v->type) && v->type!=1)
      {
         if(e->v.type == 0) // double
         {
            if(e->v.val.floatval == v->val.floatval)
               state=STAY;
            else if(e->v.val.floatval < v->val.floatval)
               state=RISE;
            else
               state=FALL;
         }
         else // boolean
         {
            if(e->v.val.booleanval == v->val.booleanval)
               state=STAY;
            else if(e->v.val.booleanval < v->val.booleanval)
               state=RISE;
            else
               state=FALL;
         }
      }
      else if((e->v.type == v->type) && v->type==1)
      {
         if(strcmp(e->v.val.strval, v->val.strval)==0)
            state=STAY;
         else
            state=CHANGE;
      }
      else
         state=TYPECHANGE;

      memcpy(&(e->v), v,sizeof(struct value_s));
      e->state = state;
   }
   else
   {
      struct inputs_table_s *s=(struct inputs_table_s *)malloc(sizeof(struct inputs_table_s));
      strncpy(s->name, _name, sizeof(s->name));
      s->name[sizeof(s->name)-1]=0;
      s->state=NEW;
      memcpy(&(s->v), v, sizeof(struct value_s));

      HASH_ADD_STR(inputs_table, name, s);
   }
   return 0;
}


int automator_reset_inputs_change_flags()
{
   struct inputs_table_s *s;

   for(s=inputs_table; s != NULL; s=s->hh.next)
      s->state=STAY;
   
   return 0;
}


#ifdef DEBUG
static int automator_print_inputs_table()
{
   struct inputs_table_s *s;

   fprintf(stderr,"INPUTS TABLE :\n");
   fprintf(stderr,"--------------\n");
   for(s=inputs_table; s != NULL; s=s->hh.next)
   {
      fprintf(stderr,"rule %s: ", s->name);
      valuePrint(&(s->v));
      fprintf(stderr," (%d)\n", s->state);
   }
   fprintf(stderr,"--------------\n");
   
   return 0;
}
#endif


cJSON *automator_load_rules_from_string(char *rules)
{
   cJSON *rules_json = cJSON_Parse(rules);

   return rules_json;
}


cJSON *automator_load_rules_from_file(char *file)
{
   cJSON *rules_json = NULL;
   FILE *fd = NULL;
   
   fd = fopen(file, "r");
   if(fd == NULL)
      return NULL;
   
   int d = fileno(fd); //if you have a stream (e.g. from fopen), not a file descriptor.
   struct stat buf;
   fstat(d, &buf);
   int size = buf.st_size;

#ifndef USEALLOCA   
   char *rules = (char *)malloc(size);
#else
   char *rules = (char *)alloca(size);
#endif
   if(rules !=NULL)
   {
      int nbread = fread(rules, 1, size, fd);
      
      if (nbread != size)
      {
         perror("");
#ifndef USEALLOCA
         free(rules);
#endif
         fclose(fd);

         return NULL;
      }
      else
         rules_json = automator_load_rules_from_string(rules);
   }

   fclose(fd);
   
   return rules_json;
}


int automator_init(char *rulesfile)
{
   if(inputs_table)
   {
      struct inputs_table_s  *current, *tmp;

      HASH_ITER(hh, inputs_table, current, tmp)
      {
         HASH_DEL(inputs_table, current);
         free(current);
      }
      inputs_table = NULL;
   }
   
   if(_rules)
   {
      cJSON_Delete(_rules);
      _rules = NULL;
      _inputs_rules = NULL;
      _outputs_rules = NULL;
   }
   
   _rules = automator_load_rules_from_file(rulesfile);
   if(_rules)
   {
      _inputs_rules = cJSON_GetObjectItem(_rules, "inputs");
      _outputs_rules = cJSON_GetObjectItem(_rules, "outputs");
   }
   
   return 0;
}


int automator_clean()
{
   if(_inputs_rules)
      cJSON_Delete(_inputs_rules);

   if(_outputs_rules)
      cJSON_Delete(_outputs_rules);

   return 0;
}

