//
//  value.c
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
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "mea_string_utils.h"

#define USEALLOCA

struct value_s {
   char type;
   union {
      char strval[41];
      char *longstrval;
      double floatval;
      char booleanval;
   } val;
};


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


void mea_valueRelease(struct value_s *v)
{
   if(v->type == 3 && v->val.longstrval != NULL)
   {
      free(v->val.longstrval);
      v->val.longstrval = NULL;
   }
}


static int _setValueFromStr(struct value_s *v, char *str, char trimFlag)
{
   double f;
   char b;
   char _str[sizeof(v->val.strval)];
   char *p;

   mea_valueRelease(v);
   
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
      int l=strlen(str);
      if(l<sizeof(v->val.strval)-1)
      {
         v->type=1;
         strcpy(v->val.strval, str); 
      }
      else
      {
         v->type=3;
         v->val.longstrval = (char *)malloc(l+1);
         if(v->val.longstrval == NULL)
            return -1;
         strcpy(v->val.longstrval,str);
      }
   }
   return 0;
}


static int mea_setValueFromStr(struct value_s *v, char *str)
{
   return _setValueFromStr(v, str, 0);
}


static int mea_setValueFromTrimedStr(struct value_s *v, char *str)
{
   return _setValueFromStr(v, str, 1);
}


enum conversion_format_e { INT = 0x01, FLOAT=0x02, HIGHLOW=0x04, TRUEFALSE=0x08, DEFAULT=0x00 };

static int mea_valueToStr(struct value_s *v, char *str, int l_str, enum conversion_format_e flag)
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
   else if(v->type==3)
   {
      strncpy(str,v->val.longstrval,l_str-1);
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


char *cmpComparators[] = {"==","!=",">","<",">=","<=",NULL};
enum comparator_e { O_EQ=0, O_NE, O_GR, O_LO, O_GE, O_LE };

static int getComparator(char *str)
{
   int comparatorNum=-1;
   for(int i=0;cmpComparators[i];i++)
   {
      if(strcmp(cmpComparators[i], str)==0)
      {
         comparatorNum=i;
         break;
      }
   }
   return comparatorNum;
}


int mea_valueCmp(struct value_s *v1, int comparator, struct value_s *v2)
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
   else if(v1->type == 2) // boolean
      cmp=v1->val.booleanval - v2->val.booleanval;
   else if(v1->type == 3) // chaine
      cmp=mea_strcmplower(v1->val.longstrval, v2->val.longstrval);
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


int mea_valueCmp2(struct value_s *v1, struct value_s *v2)
{
   int cmp=0;
   
   if(v1->type == 0 && v2->type == 0) // numérique
   {
      double diff=v1->val.floatval - v2->val.floatval;
      if(diff==0.0)
         cmp=0;
      else if(diff<0)
         cmp=-1;
      else
         cmp=1;
   } 
   else if(v1->type == 2 && v1->type == 2) // boolean
   {
      cmp=v1->val.booleanval - v2->val.booleanval;
   }
   else if( (v1->type == 1 || v1->type == 3) &&
            (v2->type == 1 || v2->type == 3) ) // chaine
   {
      char *p1, *p2;
      if(v1->type == 1)
         p1 = v1->val.strval;
      else
         p1 = v1->val.longstrval;

      if(v2->type == 1)
         p2 = v2->val.strval;
      else
         p2 = v2->val.longstrval;

      cmp=mea_strcmplower(p1, p2);
   }
   else
   {
      cmp=v1->type - v2->type;
   }

   return 0;
}


int mea_valuePrint(struct value_s *v)
{
   if(v->type==0)
      fprintf(stderr,"(float)%f",v->val.floatval);
   else if(v->type==1)
      fprintf(stderr,"(string)%s",v->val.strval);
   else if(v->type==2)
      fprintf(stderr,"(boolean)%d",v->val.booleanval);
   else
      fprintf(stderr,"(long string)%s", v->val.longstrval);

   return 0;
}


int getSpace(char *str, char **newptr)
{
   char *p = str;
   while(*p && isspace(*p)) p++;
   *newptr=p;
}


int getOperator(char *str, char **newptr)
{
   char *p = str;
   *newptr = p;

   switch(*p)
   {
      case '+':
      case '-':
      case '*':
      case '/':
         *newptr=p+1;
         return(*p);
   }
   return -1;
}


int isfunction(char *str, int l)
{
   fprintf(stderr,"%s l=%d\n", str, l);
   return 0;
}


enum eval_token_e { STRING_T = 1, NUMERIC_T, FUNCTION_T };
enum eval_token_e getToken(char *str, char **newptr, struct value_s *v)
{
   char *p = str;
   *newptr=p;
 
   switch(*p)
   {
      case '#':
         {
            char *n = NULL;
            ++p;
            double d=strtod(p, &n);
            if(p!=n)
            {
               v->type=0;
               v->val.floatval=d;
               *newptr=n;
               return NUMERIC_T;
            }
            else
               return -1;
         }
         break;
      case '{':
         {
            ++p;
            while(*p && *p!='}')
               ++p;
            if(*p=='}')
            {
               char name[41];
               strncpy(name, str+1, (int)(p-str)-1);
               name[(int)(p-str-1)]=0; 

               // récupérer ici la valeur

               v->type=0;
               v->val.floatval=-9999.0;
               *p++;
               *newptr=p;
               return NUMERIC_T;
            }
            else
               return -1;
         }
         break;

      default:
         {
            int i=0;
            ++p;

            if(!isalpha(*p))
               return -1;
            ++p;
            while(*p && isalpha(*p))
               ++p;
            if(isfunction(str, (int)(p-str))==0)
            {
               *newptr=p;
               return FUNCTION_T;
            }
            else
               return -1;
         }
         break;
   }
   return -1;
}


int operatorPriority(int op)
{
   switch(op)
   {
      case 'F':
         return 3;
      case '+':
      case '-':
         return 1;
      case '*':
      case '/':
         return 2;
      default:
         return -1;
   }

   return -1;
}


int operatorPriorityCmp(int op1, int op2)
{
   return operatorPriority(op1) - operatorPriority(op2);
}


struct eval_stack_s {
   char type;
   union {
      char op;
      double value;
   } val;
};


static int push_eval_stack(int type, void *value, struct eval_stack_s **stack, int *stack_size, int *stack_index)
{
   ++(*stack_index);
   if(*stack_index >= *stack_size)
   {
      *stack_size = *stack_size + 10;
      struct eval_stack_s *tmp;
      tmp = *stack;
      *stack = (struct eval_stack_s *)realloc(*stack, *stack_size * sizeof(struct eval_stack_s));
      if(*stack == NULL)
      {
         *stack = tmp;
         return -1;
      }
   }

   struct eval_stack_s *s=*stack;

   s[*stack_index].type=type;
   switch(type)
   {
      case 1:
         s[*stack_index].val.value=*((double *)value);
         break;
      default:
         s[*stack_index].val.op=*((char *)value);
         break;
   }

   return 0;
}


static int _eval_str(char *str, char **newptr, int *lvl, struct eval_stack_s *stack, int *stack_size, int *stack_index, int *err)
{
   int operators[10];
   int operators_index=-1;

   if(!*str)
   {
      *err=1;
      return -1;
   }

   if(*lvl>5)
   {
      *err=2;
      return -1;
   }

   struct value_s v;
   char *p=str;
   char *s;

   s=p;
   getSpace(s, &p);
   int ret=0;

   s=p;
   if(*s=='(')
   {
      s++;
      *lvl=*lvl+1;

      if(_eval_str(s, &p, lvl, stack, stack_size, stack_index, err)<0)
         return -1;

      *lvl=*lvl-1;

      s=p;
      getSpace(s, &p);

      if(*p==')')
         p++;
      else
      {
         *newptr=p;
         *err=3;
         return -1;
      }
   }
   else
   {
      int ret=getToken(s, &p, &v);
      if(ret==NUMERIC_T)
      {
         push_eval_stack(1, (void *)&(v.val.floatval), &stack, stack_size, stack_index);
      }
      else if(ret==FUNCTION_T)
      {
         s=p;
         getSpace(s, &p);

         s=p;
         if(*s!='(')
         {
            *err=6;
            return -1;
         }
         else
         {
            s++;
            if(_eval_str(s, &p, lvl, stack, stack_size, stack_index, err)<0)
               return -1;
            s=p;
            getSpace(s,&p);

            s=p;
            if(*s!=')')
            {
               *err=7;
               return -1;
            }
            p++;
            s=p;
            fprintf(stderr,">%s\n",s);
            int op='F';
            push_eval_stack(2, (void *)&(op), &stack, stack_size, stack_index);
         }
      }
      else
      {
         *err=4;
         return -1;
      }
   }

   s=p;
   getSpace(s, &p);

   s=p;
   ret=getOperator(s, &p);
   if(ret!=-1)
   {
      if(operators_index==-1)
         operators[++operators_index]=ret;
      else
      {
         if(operatorPriorityCmp(ret, operators[operators_index])<=0)
         {
            push_eval_stack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
            operators[operators_index]=ret;
         }
      }

      s=p;
      getSpace(s, &p);

      s=p;
      if(_eval_str(s, &p, lvl, stack, stack_size, stack_index, err)<0)
      {
         return -1;
      }
   }
   else
   {
      if(*s && *s!=')')
      {
         *err=5;
         return -1;
      }
   }

   *newptr=p;

   for(;operators_index>=0;--operators_index)
      push_eval_stack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
   return 0;
}


int eval_str(char *str, char **p)
{
   int lvl=0;
   int err=0;
   struct eval_stack_s *stack;
   int stack_index=-1;
   int stack_size = 80;

   stack = (struct eval_stack_s *)malloc(stack_size * (sizeof(struct eval_stack_s)));
   if(stack==NULL)
      return -1;

   int ret=_eval_str(str, p, &lvl, stack, &stack_size, &stack_index, &err);

   for(;stack_index>=0;--stack_index)
      if(stack[stack_index].type==1)
         fprintf(stderr,"%d value=%f\n", stack_index, stack[stack_index].val.value);
      else
         fprintf(stderr,"%d op=%c\n", stack_index, stack[stack_index].val.op);

   fprintf(stderr,"err=%d\n", err);

   return ret;
}


int main(int argc, char *argv[])
{
//   char *expr = "int(#99) + #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";
   char *expr = "{tata} - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";
   char *p;

   fprintf(stderr,"%s\n", expr);

   int ret=eval_str(expr, &p);

   fprintf(stderr,"fin [%s] %d\n", p, ret);
}
