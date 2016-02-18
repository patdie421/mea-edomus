//
//  automator.c
//
//  Created by Patrice DIETSCH on 05/12/15.
//
//

#define DEBUGFLAG 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>
#undef __USE_XOPEN
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
#include "datetimeServer.h"

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
#include "mea_eval.h"

#define DEBUG
#define USEALLOCA

/*
# les règles sont évaluées dans l'ordre. Par defaut toutes les règles sont évaluées l'une après l'autre
# ce comportement par défaut est modifiée par "onmatch:" (break, continue, moveforward, ...)
#
# syntaxe d'une règle d'entréee :
# <inputname> is: <value> | <"<NOP>"> | <"<LABEL>">
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
#   $function[]
#
# D1 is: <NOP>     if: (schema == 'hbeat.app') onmatch: break
# V1 is: #1
# V2 is: #2.1      if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON3', current == #2) onmatch: break
# V2 is: #10       if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON3', current > #3, current < #5) onmatch: continue
# E1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON1', type == 'input') onmatch: break
# E2 is: last      if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON2') onmatch: continue
# T2 is: $now[]    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'BUTTON2') onmatch: continue
# P1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'CONSO', type == 'power') onmatch: continue
# P2 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'CONSO', type == 'power', current != #0) onmatch: continue
# P3 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'PROD',  type == 'power', current != {V1}) onmatch: continue
# C1 is: current   if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'PROD',  type == 'power', {T2}>0 ) onmatch: moveforward 'S2'
# B1 is: &false    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'TEMP',  type == 'temp',  current > 0)   onmatch: continue
# S1 is: 'toto'    if: (source == 'mea-edomus.home', schema == 'sensor.basic', device == 'TEMP',  type == 'temp',  current <= #0) onmatch: continue
# S2 is: &true     if: ($timer['timer1']==&false) onmatch: continue
# S2 is: &false    if: ($timer['timer1']==&true) onmatch: continue
# S3 is: $timer['timer2']

# des exemples de règles "compliqués"
# T1_last is: {T1}   if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high)
# T1      is: $now[] if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high)
# DIFF    is: $calcn[{T2} - {T2_last}] if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high)
# P1      is: &high  if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high, {DIFF} > #1000)
# P1      is: &low   if: (source == mea-edomus.home, schema == sensor.basic, device == "BUTTON2", current == &high, {DIFF} <= #1000)
# exemple de compteur
# C1      is: #0     if: ($exist['C1'] == &false) // initialisation du compteur
# C1      is: $calcn[{C1}+#1]
#

# Pour les actions :
#
# A1 do: xPLSend    with: (schema='control.basic', device='toto', current={E1}) when: C1 rise
# A2 do: xPLSend    with: (schema='control.basic', device='toto', current={E1}) when: C1 fall
# A3 do: xPLSend    with: (schema='control.basic', device='tata', current={E1}) when: C1 change
# A4 do: xPLSend    with: (schema='control.basic', device='tata', current=$calcb[!{E1}]) when: C1 change
# A5 do: timerCtrl  with: (command='start', name='timer1', value=#10, unit='s') when: E1 rise
# A6 do: timerCtrl  with: (command='stop', name='timer1') when: E1 fall
*/

cJSON *_rules;
cJSON *_inputs_rules;
cJSON *_outputs_rules;

enum input_state_e { NEW, RISE, FALL, STAY, CHANGE, TYPECHANGE, UNKNOWN };

uint32_t next_input_id = 0;

struct value_s {
   char type;
   union {
      char strval[VALUE_MAX_STR_SIZE];
      double floatval;
      char booleanval;
   } val;
};

struct inputs_table_s
{
   char name[VALUE_MAX_STR_SIZE];
   enum input_state_e state;
   struct value_s v;
   uint32_t id;
   UT_hash_handle hh;
};

struct inputs_id_name_assoc_s
{
   uint32_t id;
   struct inputs_table_s *elem;

   UT_hash_handle hh;
};

struct inputs_table_s *inputs_table;
struct inputs_id_name_assoc_s *inputs_id_name_assoc = NULL;

static int startupStatus = 1;

static int automator_print_inputs_table();
static struct inputs_table_s *automator_add_to_inputs_table(char *name, struct value_s *v);
static struct inputs_table_s *_automator_add_to_inputs_table(char *name, struct value_s *v, int16_t updatestate);
static int reset_inputs_table_change_flag();
static int evalStr(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr);
static void automator_rule_debug_info_print(cJSON *rule, char *msg);
static cJSON *cJSON_DetachItemFromItem(cJSON *item, cJSON *e);

time_t automator_now = 0;

static int getBoolean(char *s, char *b)
{
   *b=-1;
   if((s[0]=='1' && s[1]==0) ||
//      strcmp(s, "true")==0   ||
//      strcmp(s, "high")==0)
      strcmp(s, c_true_str)==0   ||
      strcmp(s, c_high_str)==0)
   {
      *b=1;
      return 0;
   }
   else if((s[0]=='0' && s[1]==0) ||
//           strcmp(s, "false")==0  ||
//           strcmp(s, "low")==0)
           strcmp(s, c_false_str)==0  ||
           strcmp(s, c_true_str)==0)
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


static int setValueFromStrTrim(struct value_s *v, char *str)
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
//            strcpy(str, "low");
            strcpy(str, c_low_str);
         else
//            strcpy(str, "high");
            strcpy(str, c_high_str);
      else if(flag & TRUEFALSE)
         if(v->val.booleanval==0)
//            strcpy(str, "false");
            strcpy(str, c_false_str);
         else
            strcpy(str, c_true_str);
      else
         snprintf(str,l_str,"%d",v->val.booleanval);
   }

   return 0;
}


enum comparator_e { O_EQ=0, O_NE, O_GR, O_LO, O_GE, O_LE };
/*
char *comparators[] = {"==","!=",">","<",">=","<=",NULL};
static int getComparator(char *str)
{
   int comparatorNum=-1;
   for(int i=0;comparators[i];i++)
   {
      if(strcmp(comparators[i], str)==0)
      {
         comparatorNum=i;
         break;
      }
   }
   return comparatorNum;
}
*/
static int getComparator(char *str)
{
   if(str[1]=='=' && str[2]==0)
   {
      switch(str[0])
      {
         case '=' : return O_EQ;
         case '!' : return O_NE;
         case '>' : return O_GE;
         case '<' : return O_LE;
         default: return -1;
      }
   }
   else if(str[1]==0)
   {
      switch(str[0])
      {
         case '>' : return O_GR;
         case '<' : return O_LO;
         default: return -1;
      }
   }
   else
      return -1;
}

 
static int valueCmp(struct value_s *v1, int comparator, struct value_s *v2)
{
   // cas type différent : seule la différence peut retourner 1
   if(v1->type != v2->type) {
      if(comparator == O_NE) {
         return 1;
      }
      else {
         return 0;
      }
   }
   // autres cas on a forcement v1->type == v2->type
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

   switch(comparator)
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


#ifdef DEBUG
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
#endif

enum function_e {
   F_NOW=0,
   F_CALCN,
   F_EXIST,
   F_RISE,
   F_FALL,
   F_STARTUP,
   F_STAY,
   F_CHANGE,
   F_DATE,
   F_TIME,
   F_TIMER,
   F_TIMERSTATUS,
   F_SUNRISE,
   F_SUNSET, 
   F_TWILIGHTSTART,
   F_TWILIGHTEND,
   F_XPLMSGDATA,
   F_TOHLSTR,
   F_TOTFSTR,
   _FN_LIST_END };

struct function_def_s
{
   char *name;
   enum function_e num;
   uint16_t l;
};

int16_t functions_index[_FN_LIST_END];

struct function_def_s functionsList2[]={
   { "calcn", F_CALCN, 5 },
   { "change", F_CHANGE, 5 },
   { "date", F_DATE, 4 },
   { "exist", F_EXIST, 5 },
   { "fall", F_FALL, 4 },
   { "now", F_NOW, 3 },
   { "rise", F_RISE, 4 },
   { "startup", F_STARTUP, 7 },
   { "stay", F_STAY, 4 },
   { "sunrise", F_SUNRISE, 7 },
   { "sunset", F_SUNSET, 6 },
   { "timerstatus", F_TIMERSTATUS, 11 },
   { "timer", F_TIMER, 5 },
   { "time", F_TIME, 4 },
   { "tohlstr", F_TOHLSTR, 7 },
   { "totfstr", F_TOTFSTR, 7 },
   { "twilightstart", F_TWILIGHTSTART, 13 },
   { "twilightend", F_TWILIGHTEND, 11 },
   { "xplmsgdata", F_XPLMSGDATA, 10 },
   { NULL, 0, 0 }
};


int qsort_compare_functions_names(const void * a, const void * b)
{
   return strcmp(functionsList2[functions_index[*(int16_t *)a]].name, functionsList2[functions_index[*(int16_t *)b]].name);
}


void init_functions_index()
{
   for(int i=0;i<_FN_LIST_END;i++)
      functions_index[i]=i;
   qsort (functions_index, _FN_LIST_END, sizeof (int16_t), qsort_compare_functions_names); 
}


enum function_e getFunctionNum(char *str, char *params, int l_params)
{
   char fn[VALUE_MAX_STR_SIZE];
   int l=VALUE_MAX_STR_SIZE;
   char *fnPtr=fn;

   while(l && isalnum(*str))
   {
      *fnPtr=*str;
      ++fnPtr;
      ++str;
      --l;
   }
   *fnPtr=0;

   if(*str!='[')
      return -1;
   ++str;

   int16_t start = 0;
   int16_t end = _FN_LIST_END - 1;
   int16_t _cmpres;
   do
   {
      int16_t middle=(end + start) / 2;
      if(middle<0)
         return -1;

      _cmpres=(int)strcmp(functionsList2[functions_index[middle]].name, fn);
      if(_cmpres==0)
      {
         while(*str && *str!=']' && l_params)
         {
            *params=*str;
            ++str;
            ++params;
            --l_params;
         }
         *params=0;

         if(*str==']' && *(str+1)==0)
            return functionsList2[functions_index[middle]].num;
         else
            return -1;
      }
      if(_cmpres<0)
         start=middle+1;
      if(_cmpres>0)
         end=middle-1;
   }
   while(start<=end);

   return -1;
}
/*
static int getFunctionNum(char *str, char *params, int l_params)
{
   int ls=strlen(str); 
   for(int i=0; functionsList2[i].name; i++)
   {
      int lf=functionsList2[i].l;
      if(strncmp(str,functionsList2[i].name, functionsList2[i].l)==0)
      {
         if(str[lf]=='[' && str[ls-1]==']')
         {
            int lp = ls-lf-2;
            if(lp>l_params)
               lp=l_params;
            strncpy(params, &(str[lf+1]), lp);
            params[lp]=0;
            return functionsList2[i].num;
         }
         else
            return -1;
      }
   }
   return -1;
}
*/


static int getInputEdge(char *expr, int direction,  struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   if((direction==CHANGE ||
       direction==RISE   ||
       direction==STAY   ||
       direction==NEW    ||
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
               v->val.booleanval=(e->state==CHANGE || e->state==RISE  || e->state==FALL || e->state==TYPECHANGE || e->state==NEW);
            return 0;
         }
         else
            return 1;
      }
   }
   return -1;
}


int getInputId(char *name, uint32_t *id)
{
   struct inputs_table_s *e = NULL;
   HASH_FIND_STR(inputs_table, name, e);
   if(!e)
   {
      struct value_s v;
      v.type=0;
      v.val.floatval=0.0;
      e=automator_add_to_inputs_table(name, &v);
   }

   if(e)
   {
     if(e->id==-1)
     {
        e->id=++next_input_id;

        struct inputs_id_name_assoc_s *a;
        a=(struct inputs_id_name_assoc_s *)malloc(sizeof(struct inputs_id_name_assoc_s));
        if(a==NULL)
           return -1;
        a->id=e->id;
        a->elem = e;
        HASH_ADD_INT(inputs_id_name_assoc, id, a);
     }
     *id=e->id;
     return 0;
   }

   return -1;
}


int getInputFloatValueById(uint32_t id, double *d)
{
   struct inputs_id_name_assoc_s *a;

   HASH_FIND_INT(inputs_id_name_assoc, &id, a);
   if(a == NULL)
      return -1;
   else
   {
      if(a->elem->v.type == 0)
         *d = a->elem->v.val.floatval;
      else
         return -1;
   }
   return 0;
}


void idNameAssocsClean()
{
   struct inputs_id_name_assoc_s *current, *tmp;

   HASH_ITER(hh, inputs_id_name_assoc, current, tmp)
   {
      HASH_DEL(inputs_id_name_assoc, current);
      free(current);
      current=NULL;
   }
   inputs_id_name_assoc=NULL;
}


int16_t myGetVarId(char *str, void *userdata, int16_t *id)
{
   uint32_t _id = 0;
   int16_t retour = 0;

   retour=(int16_t)getInputId(str, &_id);

   *id=(int16_t)_id; 

   return retour;
}


int16_t myGetVarVal(int16_t id, void *userdata, double *d)
{
   return getInputFloatValueById((uint32_t)id, d);
}


#define MAX_STR_FUNCTION_SIZE 1024

static int callFunction(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   
   char *f = NULL;
   int retour=-1;
   int str_l = strlen(str)+1;
   if(str_l > MAX_STR_FUNCTION_SIZE)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : string length overflow - %s\n", ERROR_STR, __func__, str);
      return -1;
   }
/*
   f=(char *)alloca(str_l);
   if(f==NULL)
      return -1;
   mea_strcpytrim(f, str);
*/
   char *params=(char *)alloca(str_l);
   f = str;
   int fn=getFunctionNum(f, params, str_l);
   switch(fn)
   {
      case F_NOW:
         if(params[0]==0)
         {
            time_t t=automator_now;
            v->type=0;
            v->val.floatval=(double)t;
            retour=0;
         }
         break;
      case F_STARTUP:
         if(params[0]==0)
         {
            v->type=2;
            v->val.booleanval=startupStatus;
            retour=0;
         }
         break;
      case F_XPLMSGDATA:
         if(params[0]==0)
         {
            v->type=2;
            if(ListNomsValeursPtr == NULL)
               v->val.booleanval=0;
            else
               v->val.booleanval=1;
            retour=0;
         }
         break;
      case F_CALCN:
         {
            int l_params=strlen(params)-1;
            double d;

            if(l_params > 1 && params[0]=='\'' && params[l_params]=='\'')
            {
               char *p=&(params[1]);
               params[l_params]=0; 

               if(mea_eval_calc_numeric_by_cache(p, &d)==0)
               {
                  v->type=0;
                  v->val.floatval=(double)d;
                  retour = 0;
               }
               else
                  retour = 1;
            }
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
      case F_TIMER:
         {
            int ret;
            struct value_s r;
            ret=evalStr(params, &r, ListNomsValeursPtr);
            if(ret==0 && r.type==1)
            {
               int state = 0;
                  state = mea_datetime_getTimerState(r.val.strval);
               v->type=2;
               if(state == TIMER_FALLED)
                  v->val.booleanval=1;
               else
                  v->val.booleanval=0;
               retour=0;
            }
         }
         break;
      case F_TIME: // $time['18:31:01']
         {
            // struct tm tm;
            int ret;
            struct value_s r;
            time_t t;
            ret=evalStr(params, &r, ListNomsValeursPtr);
            if(ret==0 && r.type==1 && r.val.strval[8]==0)
            {
               if(mea_timeFromStr(r.val.strval, &t)==0)
               {
                  v->type=0;
                  v->val.floatval=(double)t;
                  retour=0;
               }
            }
         }
         break;
      case F_DATE: // date pour comparaison : $date['2001-11-12 18:31:01'] 
         {
            int ret;
            struct tm tm;
            struct value_s r;

            ret=evalStr(params, &r, ListNomsValeursPtr);
            if(ret==0 && r.type==1 && r.val.strval[19])
            {
               memset(&tm, 0, sizeof(struct tm));
               // format date reconnu : 2001-11-12 18:31:01
               char *p=strptime(r.val.strval, "%Y-%m-%d %H:%M:%S", &tm);
               if(p != NULL && *p==0)
               {
                  v->type=0;
                  v->val.floatval=(double)mktime(&tm);
                  retour=0;
               }
            }
         }
         break;
      case F_TOHLSTR:
      case F_TOTFSTR:
         {
            int ret;
            struct value_s r;
            ret=evalStr(params, &r, ListNomsValeursPtr);
            if(ret==0 && r.type==2)
            {
               v->type=1;
               switch(fn)
               {
                  case F_TOHLSTR:
                     if(r.val.booleanval==0)
                        // strcpy(v->val.strval, "low");
                        strcpy(v->val.strval, c_low_str);
                     else 
                        // strcpy(v->val.strval, "high");
                        strcpy(v->val.strval, c_high_str);
                     break;
                  case F_TOTFSTR:
                     if(r.val.booleanval==0)
                        // strcpy(v->val.strval, "false");
                        strcpy(v->val.strval, c_false_str);
                     else 
                        // strcpy(v->val.strval, "true");
                        strcpy(v->val.strval, c_true_str);
                     break;
               }
               retour = 0;
            }
         }
         break;
         
      case F_SUNSET:
      case F_SUNRISE:
      case F_TWILIGHTSTART:
      case F_TWILIGHTEND:
         {
            int ret;
            struct value_s r;
            r.type=0;
            r.val.floatval=0.0;
            ret=evalStr(params, &r, ListNomsValeursPtr);
            if( (ret==0 && r.type==0) || ret==1) // ret == 1 : chaine vide
            {
               time_t t=0;
               switch(fn)
               {
                  case F_SUNSET:
                     t=mea_datetime_sunset();
                     break;
                  case F_SUNRISE:
                     t=mea_datetime_sunrise();
                     break;
                  case F_TWILIGHTSTART:
                     t=mea_datetime_twilightstart();
                     break;
                  case F_TWILIGHTEND:
                     t=mea_datetime_twilightend();
                     break;
               }
               v->type=0;
               v->val.floatval=(double)t;
               if(ret!=1)
                  v->val.floatval+=r.val.floatval;
               retour=0;
            }
         }     
         break;
   }

   return retour;
}


static int evalStr(char *str, struct value_s *v, xPL_NameValueListPtr ListNomsValeursPtr)
{
   char p[sizeof(v->val.strval)];

   if( (str[1]=='0' || str[1]=='1') && str[0]=='&' && str[2]==0)
   {
      v->type=2;
      v->val.booleanval=str[1]-'0';
      return 0;
   }
   
   mea_strncpytrim(p, str, sizeof(v->val.strval)-1);
   p[sizeof(v->val.strval)-1]=0;

   switch(*p)
   {
      case 0:
         return 1;
      case '#':
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
         break;
      case '\'':
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
         }
         break;
      case '&':
         {
            char b=0;
            if(getBoolean(&p[1], &b)==0)
            {
               v->type=2;
               v->val.booleanval=b;
            }
            else
               return -1;
        }
        break;
     case '$':
        {
           int ret=callFunction(&(p[1]), v, ListNomsValeursPtr);
           return ret;
        }
        break;
     case '{':
        {
           int l=strlen(p);
           if(p[l-1]!='}')
              return -1;
           char name[VALUE_MAX_STR_SIZE];
           struct inputs_table_s *e = NULL;

           if(l>(sizeof(name)+2))
              l=sizeof(name)+2;

           char *_p=NULL;
           mea_strncpytrim(name, &(p[1]), l-2);
           name[l-2]=0;
           _p=name; 

           HASH_FIND_STR(inputs_table, _p, e);
           if(e)
           {
              v->type=e->v.type;
              switch(e->v.type)
              {
                 case 0: v->val.floatval = e->v.val.floatval; break;
                 case 1: strcpy(v->val.strval, e->v.val.strval); break;
                 case 2: v->val.booleanval = e->v.val.booleanval; break;
              }
          }
          else
             return 1;
        }
        break;
     case '<':
        {
           int l=strlen(p);
           if(p[l-1]!='>')
              return -1; 

           if(l==5 && strncmp(&(p[1]),"NOP",3)==0)
           {
              strcpy(v->val.strval, p);
              v->type=1;
           }
           else if(l==7 && strncmp(&(p[1]),"LABEL",5)==0)
           {
              strcpy(v->val.strval, p);
              v->type=1;
           }
           else
              return -1;
        }
        break;
    default:
       {
          if(ListNomsValeursPtr==NULL)
             return 1;
          char *_value=xPL_getNamedValue(ListNomsValeursPtr, p);
          if(_value!=NULL)
             setValueFromStr(v, _value);
          else
            return 1;
      }
   }
   return 0;
}


static int automator_timerCtrl(cJSON *parameters)
{
//   double now;
   if(parameters==NULL || parameters->child==NULL)
      return -1;

   cJSON *command = cJSON_GetObjectItem(parameters, "command");
   cJSON *name = cJSON_GetObjectItem(parameters, "name");

   if(command == NULL || name == NULL)
      return -1;

   struct value_s c, n;

    
   if(command->type != cJSON_String || evalStr(command->valuestring, &c, NULL)<0 || c.type != 1 )
      return -1;
   if(name->type != cJSON_String || evalStr(name->valuestring, &n, NULL)<0 || n.type != 1)
      return -1;


   char timername[sizeof(n.val.strval)];
   char timercommand[sizeof(c.val.strval)];

   mea_strcpytrimlower(timercommand, c.val.strval);

   if(command->type == cJSON_String && strcmp(timercommand,"start")==0)
   {
      int _mode=0;

      cJSON *mode = cJSON_GetObjectItem(parameters, "mode");
      if(mode)
      {
         struct value_s m;
         if(mode->type != cJSON_String || evalStr(mode->valuestring, &m, NULL)<0 || m.type != 1 )
            return -1;

         if(mea_strcmplower(m.val.strval, "alarm")==0)
            _mode=1;
         else if(mea_strcmplower(m.val.strval, "timer")==0)
            _mode=0;
         else
         {
            return -1;
         }
      }

      cJSON *value = cJSON_GetObjectItem(parameters, "value");
      cJSON *unit = cJSON_GetObjectItem(parameters, "unit");

      if(value == NULL)
         return -1;
      if(_mode==0 && unit == NULL)
         return -1;
      if(_mode==1 && unit != NULL)
         return -1;

      struct value_s v, u;
      if(evalStr(value->valuestring, &v, NULL)<0 || v.val.floatval < 1.0 || v.type != 0 )
         return -1;

      mea_strcpytrimlower(timername, n.val.strval);
      if(_mode==0)
      {
         if(evalStr(unit->valuestring, &u, NULL)<0 || u.type != 1)
            return -1;

         char timerunit[sizeof(u.val.strval)];

         mea_strcpytrimlower(timerunit, u.val.strval);

         enum datetime_timer_unit_e timerunitval = -1;
         if(strcmp(timerunit, "sec")==0)
            timerunitval = TIMER_SEC;
         else if(strcmp(timerunit, "min")==0)
            timerunitval = TIMER_MIN;
         else if(strcmp(timerunit, "msec")==0)
            timerunitval = TIMER_MSEC;
         else if(strcmp(timerunit, "csec")==0)
            timerunitval = TIMER_CSEC;
         else
            return -1;
//         VERBOSE(9) mea_log_printf("%s (%s) : start timer %s\n", INFO_STR, __func__, timername);
         int ret=mea_datetime_startTimer2(timername, (int)v.val.floatval, timerunitval, automatorServer_timer_wakeup, NULL); 
         return ret;
      }
      else
      {
//         VERBOSE(9) mea_log_printf("%s (%s) : start alarm\n", INFO_STR, __func__, timername);
         int ret=mea_datetime_startAlarm2(timername, (time_t)v.val.floatval, automatorServer_timer_wakeup, NULL); 
         return ret;
      }
   }
   else if(command->type == cJSON_String && strcmp(timercommand,"stop")==0)
   {
//      VERBOSE(9) mea_log_printf("%s (%s) : stop timer/alarm\n", INFO_STR, __func__, timername);
      int ret=mea_datetime_stopTimer(timername);
      return ret;
   }
   VERBOSE(5)  mea_log_printf("%s (%s) : unknown command - %s\n", ERROR_STR, __func__, command->valuestring);

   return -1;
}


static int automator_setinputvalue(cJSON *parameters)
{
   int16_t _updatestate = 1;

   if(parameters==NULL || parameters->child==NULL)
      return -1;

   cJSON *name = cJSON_GetObjectItem(parameters, "name");
   if(!name)
      return -1;
   cJSON *value = cJSON_GetObjectItem(parameters, "value");
   if(!value)
      return -1;
   cJSON *updatestate = cJSON_GetObjectItem(parameters, "updatestate");

   struct value_s n,v,u;
   if(name->type != cJSON_String || evalStr(name->valuestring, &n, NULL)<0 || n.type != 1 )
      return -1;
   if(value->type != cJSON_String || evalStr(value->valuestring, &v, NULL)<0)
      return -1;
   if(updatestate)
   {
      if(updatestate->type != cJSON_String || evalStr(updatestate->valuestring, &u, NULL)<0 || u.type != 2 )
         return -1;
      _updatestate = u.val.booleanval;
   }
  
   _automator_add_to_inputs_table(n.val.strval, &v, _updatestate);
    
   return 0;  
}


int automator_sendxpl(cJSON *parameters)
{
   if(parameters==NULL || parameters->child==NULL)
      return -1;

   xPL_ServicePtr servicePtr = mea_getXPLServicePtr();
   if(!servicePtr)
   {
      fprintf(stderr,"pas de service xpl ...\n");
      return -1;
   }

   xPL_MessagePtr xplMessage = xPL_createBroadcastMessage(servicePtr, xPL_MESSAGE_COMMAND);
   if(!xplMessage)
      return -1;

   cJSON *e=parameters->child;

   int retour=0;
   char schema[21]="control.basic";
   char target[VALUE_MAX_STR_SIZE]="";
   int msgtype=xPL_MESSAGE_COMMAND;
   while(e)
   {
      struct value_s v;
    
      if(evalStr(e->valuestring, &v, NULL)==0)
      {
         // type de message
         if(strcmp(e->string, "msgtype")==0)
         {
            if(v.type==1)
            {
               if(strcmp(v.val.strval, "trigger")==0)
                  msgtype=xPL_MESSAGE_TRIGGER;
               else if(strcmp(v.val.strval, "status")==0)
                  msgtype=xPL_MESSAGE_STATUS;
               else if(strcmp(v.val.strval, "command")==0)
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
         else if(strcmp(e->string, "schema")==0) {
         
            strncpy(schema, v.val.strval, sizeof(schema)-1);
            schema[sizeof(schema)-1]=0;
         }
         else if(strcmp(e->string,"source")==0) {
            retour=-1;
            break;
         }
         else if(strcmp(e->string,"target")==0) {
            strncpy(target, v.val.strval, sizeof(target)-1);
            target[sizeof(target)-1]=0;
         }
         else
         {
            char s[sizeof(v.val.strval)];
            valueToStr(&v, s, sizeof(v.val.strval), INT | HIGHLOW);
            xPL_setMessageNamedValue(xplMessage, e->string, s);
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

   DEBUG_SECTION2(DEBUGFLAG) displayXPLMsg(xplMessage);

   xPL_releaseMessage(xplMessage);
 
   return 0;
}


int automator_play_output_rules(cJSON *rules)
{
   if(rules==NULL)
   {
      DEBUG_SECTION2(DEBUGFLAG)  mea_log_printf("%s (%s) : NO OUTPUT RULE\n", DEBUG_STR, __func__);
      return -1;
   }

   double start=mea_now();
   cJSON *e=rules->child;
   while(e) // balayage des règles
   {
//      cJSON *name       = cJSON_GetObjectItem(e,"name");
      cJSON *cond  = cJSON_GetObjectItem(e,"condition");

//      struct inputs_table_s  *current, *tmp; HASH_ITER(hh, inputs_table, current, tmp) fprintf(stderr,"> [%s]\n", current->name); 

      struct inputs_table_s *i = NULL;
      HASH_FIND_STR(inputs_table, cond->child->string, i);
      int actionFlag=0; 
      if(i==NULL)
      {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) : Input rule (%s) no data found\n", DEBUG_STR, __func__, cond->child->string);
         goto next_rule;
      }

      switch(cond->child->valueint)
      {
         case 1:
            if(i->state==RISE)
               actionFlag=1; 
            break;
         case 2:
            if(i->state==FALL)
               actionFlag=1; 
            break;
         case 3:
            if(i->state==CHANGE || i->state==RISE || i->state==FALL || i->state == NEW)
               actionFlag=1; 
            break;
         case 4:
            if(i->state==NEW)
               actionFlag=1; 
            break;
         default:
            automator_rule_debug_info_print(e, "Incorrect output rule condition - not rise, fall, change or new (rule removed)");
            cJSON *c;
            c=e;
            e=e->next;
            c=cJSON_DetachItemFromItem(rules, c);
            cJSON_Delete(c);
            goto next_iteration;
      }

      if(actionFlag==1)
      {
         cJSON *action     = cJSON_GetObjectItem(e,"action");
         cJSON *parameters = cJSON_GetObjectItem(e,"parameters");

         if(mea_strcmplower(action->valuestring, "xPLSend")==0)
            automator_sendxpl(parameters);
         else if(mea_strcmplower(action->valuestring, "timerCtrl")==0)
            automator_timerCtrl(parameters);
         else if(mea_strcmplower(action->valuestring, "setInput")==0)
            automator_setinputvalue(parameters);
      } 

next_rule:
      e=e->next;
next_iteration: {}
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

      DEBUG_SECTION2(DEBUGFLAG) {
         mea_log_printf("%s (%s) : outputs rules processing time=%ld us\n", DEBUG_STR, __func__, exectime);
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


static void automator_rule_debug_info_print(cJSON *rule, char *msg)
{
   cJSON *files  = cJSON_GetObjectItem(_rules, "files");
   if(files==NULL)
      return;

   cJSON *fileno = cJSON_GetObjectItem(rule,"file");
   if(fileno == NULL)
      return;

   cJSON *file = cJSON_GetArrayItem(files, fileno->valueint);
   if(file == NULL)
      return;

   cJSON *lineno = cJSON_GetObjectItem(rule, "line");
   if(lineno != NULL)
   {
      mea_log_printf("%s (%s) : file \"%s\", line %d : %s\n", DEBUG_STR, __func__, file->valuestring, lineno->valueint, msg);
   }
}


static cJSON *cJSON_DetachItemFromItem(cJSON *item, cJSON *e)
{
   cJSON *c=item->child;

   while (c) // on cherche e dans les fils de item
   {
      if(c != e)
         c=c->next;
      else
         break;
   }
   if (c==NULL) // on l'a trouvé ?
      return NULL;

   if (c->prev)
      c->prev->next=c->next;
   if (c->next)
      c->next->prev=c->prev;

   if (c==item->child) // c'est le premier de la liste
      item->child=c->next;

   c->prev=NULL;
   c->next=NULL;

   return c;
}


struct moveforward_dest_s {
   char rule[VALUE_MAX_STR_SIZE];
   cJSON *e;
   UT_hash_handle hh;
};

struct moveforward_dest_s *moveforward_dests = NULL;

int automator_match_inputs_rules(cJSON *rules, xPL_MessagePtr message)
{
   if(rules==NULL)
   {
      DEBUG_SECTION2(DEBUGFLAG)  mea_log_printf("%s (%s) : NO INPUT RULE\n", DEBUG_STR, __func__);
      return -1;
   }

   double start=mea_now();

   xPL_NameValueListPtr ListNomsValeursPtr = NULL;
   char *schema_type = NULL, *schema_class = NULL, *vendor = NULL, *deviceID = NULL, *instanceID = NULL;
   char source[80]="";
   char schema[80]="";

   automator_now=mea_datetime_time(NULL);
   
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

//   int cntr=0;
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
         automator_rule_debug_info_print(e, "incomplete rule, no name or value (rule removed)");

         // pas de nom ou pas de valeur, pas la peine de relire à chaque fois, on supprime la règle.
         cJSON *c = e;
         e=e->next;
         c=cJSON_DetachItemFromItem(rules, c);
         cJSON_Delete(c); 

         continue;
      }
      DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) : RULE - %s\n", DEBUG_STR, __func__, name->valuestring);
      int ret=evalStr(value->valuestring, &res, ListNomsValeursPtr);
      if(ret<0)
      {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) :    [%s] incorrect value\n",  DEBUG_STR, __func__, value->valuestring);
         automator_rule_debug_info_print(e, "incorrect rule value (rule removed)");

         cJSON *c;
         c=e;
         e=e->next;
         c=cJSON_DetachItemFromItem(rules, c);
         cJSON_Delete(c);

         continue;
      }

//      if(res.type==1 && mea_strcmplower(res.val.strval,"<LABEL>")==0)
      if(res.type==1 && strcmp(res.val.strval,"<LABEL>")==0)
      {
         match=0;
         goto next_rule;
      }

      // évaluation des conditions
      cJSON *conditions=cJSON_GetObjectItem(e,"conditions");
      if(conditions!=NULL)
      {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) :    CONDITIONS : \n",  DEBUG_STR, __func__);
         cJSON *c=conditions->child;
         while(c)
         {
            int operator=-1;
            cJSON *value1 = cJSON_GetObjectItem(c, "value1");
            cJSON *op     = cJSON_GetObjectItem(c, "op");
            cJSON *value2 = cJSON_GetObjectItem(c, "value2");
            if(!value1 || !op || !value2)
            {
               automator_rule_debug_info_print(e, "incorrect condition, check JSON (rule removed)");

               cJSON *c = e;
               e=e->next;
               c=cJSON_DetachItemFromItem(rules, c);
               cJSON_Delete(c);

               goto next_loop;
            }

            // récuperation de l'opération
            operator=getComparator(op->valuestring);
            if(operator<0)
            {
               automator_rule_debug_info_print(e, "incorrect condition, unknown operator (rule removed)");

               cJSON *c = e;
               e=e->next;
               c=cJSON_DetachItemFromItem(rules, c);
               cJSON_Delete(c);

               goto next_loop;
            }

            if(automator_getValue(value1, &val1, source, schema, ListNomsValeursPtr)<0)
            {
               automator_rule_debug_info_print(e, "incorrect value1, in condition (rule removed)");

               cJSON *c = e;
               e=e->next;
               c=cJSON_DetachItemFromItem(rules, c);
               cJSON_Delete(c);

               goto next_loop;
            }
            
            if(automator_getValue(value2, &val2, source, schema, ListNomsValeursPtr)<0)
            {
               automator_rule_debug_info_print(e, "incorrect value2, in condition (rule removed)");

               cJSON *c = e;
               e=e->next;
               c=cJSON_DetachItemFromItem(rules, c);
               cJSON_Delete(c);

               goto next_loop;
            }

            // comparaison
            int cmpr=valueCmp(&val1, operator, &val2);
            if(cmpr < 1)
            {
               match=0;
               goto next_rule;
            }

            c=c->next;
         }
      }
      else
      {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) :    NO CONDITION\n",  DEBUG_STR, __func__);
      }

   next_rule:
      if(match==1)
      {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) :    MATCH !\n", DEBUG_STR, __func__);

         if(strcmp(res.val.strval, "<NOP>")!=0)
         {
            automator_add_to_inputs_table(name->valuestring, &res);
         }
         else
         {
            DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) :    Result discarded\n", DEBUG_STR, __func__);
         }

         if(onmatch) // post action
         {
/*
            char action[VALUE_MAX_STR_SIZE]="";
            mea_strncpylower(action, onmatch->valuestring, sizeof(action)-1);
            action[sizeof(action)-1]=0;
            // découpage de la chaine si nécessaire
            char *p=NULL;
            for(int i=0;action[i];i++)
            {
               if(action[i]==' ')
               {
                  p=&(action[i+1]);
                  action[i]=0;
                  break;
               }
            }
*/
            char action[VALUE_MAX_STR_SIZE]="";
            char action_l=sizeof(action)-1;
            char *p_action  = action;
            char *p_onmatch = onmatch->valuestring;
            // découpage de la chaine
            while(*p_onmatch && *p_onmatch!=' ' && action_l)
            {
               *p_action=tolower(*p_onmatch);
               ++p_action;
               ++p_onmatch;
               --action_l;
            }
            *p_action=0;
 
            if(strcmp(action, "break")==0 && !(*p_onmatch))
               break;
            else if(strcmp(action, "continue")==0 && !(*p_onmatch))
            {
            }
            else if(strcmp(action, "moveforward")==0 && *p_onmatch)
            {
               struct value_s r;
               cJSON *_e = NULL;
               int flag=0;
               struct moveforward_dest_s *md = NULL;

               // supprime les blancs en début de p_onmatch
               while(*p_onmatch && *p_onmatch==' ') ++p_onmatch;

//               if(evalStr(p_onmatch, &r, NULL)==0 && r.type==1)
               if(*p_onmatch)
               {
                  int16_t notCacheMoveForwardFlag = 0;
                  // HASH_FIND_STR(moveforward_dests, r.val.strval, md);
                  HASH_FIND_STR(moveforward_dests, p_onmatch, md);
                  if(md) // on va vérifier qu'on fait bien un saut en avant
                  {
                     cJSON *numc = cJSON_GetObjectItem(e, "num");
                     cJSON *numn = cJSON_GetObjectItem(md->e, "num");
                     if( numc->type != cJSON_Number ||
                         numn->type != cJSON_Number ||
                         numn->valueint <= numc->valueint)
                        notCacheMoveForwardFlag = 1;
                  }
                  
                  if(!md || notCacheMoveForwardFlag == 1)
                  { 
                     _e = e->next;
                     while(_e)
                     {
//                        if(cJSON_GetObjectItem(_e,"name") && mea_strcmplower(cJSON_GetObjectItem(_e,"name")->valuestring,r.val.strval)==0)
                        // if(cJSON_GetObjectItem(_e,"name") && strcmp(cJSON_GetObjectItem(_e,"name")->valuestring,r.val.strval)==0)
                        if(cJSON_GetObjectItem(_e,"name") && strcmp(cJSON_GetObjectItem(_e,"name")->valuestring, p_onmatch)==0)
                        {
                           if(notCacheMoveForwardFlag == 0)
                           {
                              md = (struct moveforward_dest_s *)malloc(sizeof(struct moveforward_dest_s));
                              if(md)
                              {
                                 // strcpy(md->rule, r.val.strval);
                                 strcpy(md->rule, p_onmatch);
                                 md->e=_e;
                                 HASH_ADD_STR(moveforward_dests, rule, md);
                              } 
                           }
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
                  {
                     e=md->e;
                     continue;
                  }
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
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) :    NOT MATCH !\n", DEBUG_STR, __func__);
      }
      e=e->next;
next_loop:{}
//      cntr++;
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

      DEBUG_SECTION2(DEBUGFLAG) {
         mea_log_printf("%s (%s) : inputs rules processing time=%ld us\n", DEBUG_STR, __func__, exectime);
         automator_print_inputs_table();
      } 
//      automator_print_inputs_table();
   }

   startupStatus = 0;
 
   return 0;
}


struct inputs_table_s *_automator_add_to_inputs_table(char *_name, struct value_s *v, int16_t update_state)
{
   struct inputs_table_s *e = NULL;

   HASH_FIND_STR(inputs_table, _name, e);
   if(e)
   {
      enum input_state_e state = UNKNOWN;
      if(update_state == 1)
      { 
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
      }
      e->v.type=v->type;
      if(v->type == 0)
         e->v.val.floatval = v->val.floatval;
      if(v->type == 1)
         strcpy(e->v.val.strval, v->val.strval);
      if(v->type == 2)
         e->v.val.booleanval = v->val.booleanval;
      e->state = state;

      return e;
   }
   else
   {
      struct inputs_table_s *s=(struct inputs_table_s *)malloc(sizeof(struct inputs_table_s));
      strncpy(s->name, _name, sizeof(s->name));
      s->name[sizeof(s->name)-1]=0;
      s->id=-1;
      s->v.type=v->type;
      if(v->type == 0)
         s->v.val.floatval = v->val.floatval;
      if(v->type == 1)
         strcpy(s->v.val.strval, v->val.strval);
      if(v->type == 2)
         s->v.val.booleanval = v->val.booleanval;
      if(update_state == 1)
         s->state=NEW;
      else
         s->state=UNKNOWN;
      HASH_ADD_STR(inputs_table, name, s);

      return s;
   }

   return NULL;
}


static inline struct inputs_table_s *automator_add_to_inputs_table(char *_name, struct value_s *v)
{
   return _automator_add_to_inputs_table(_name, v, 1);
}


static inline struct inputs_table_s *automator_add_to_inputs_table_noupdate(char *_name, struct value_s *v)
{
   return _automator_add_to_inputs_table(_name, v, 1);
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

   char *rules = (char *)alloca(size);
   if(rules !=NULL)
   {
      int nbread = fread(rules, 1, size, fd);
      
      if (nbread != size)
      {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : can't load rules - \n", ERROR_STR, __func__);
            perror("");
         }
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
   idNameAssocsClean();
   mea_eval_clean_stack_cache();
   mea_eval_setGetVarCallBacks(&myGetVarId, &myGetVarVal, NULL);
   init_functions_index();

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

   if(moveforward_dests)
   {
      struct moveforward_dest_s  *current, *tmp;

      HASH_ITER(hh, moveforward_dests, current, tmp)
      {
         HASH_DEL(moveforward_dests, current);
         free(current);
      }
      moveforward_dests = NULL;
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
