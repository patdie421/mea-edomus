//
//  value.c
//
//  Created by Patrice DIETSCH on 05/12/15.
//
//
#define DEBUGFLAGON 1

//#define EVAL_MODULE_TEST

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
#include "mea_eval.h"

static void *getVarUserData = NULL;

static getVarVal_f _getVarVal = NULL;
static getVarId_f _getVarId = NULL;


static int getSpace(char *str, char **newptr)
{
   char *p = str;
   while(*p && isspace(*p)) p++;
   *newptr=p;
   
   return 0;
}


static int getOperator(char *str, char **newptr)
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


static char *functions[] = {"int", NULL};
enum function_e { INT_F=0 };

static int getFunctionId(char *str, int l)
{
   char name[41];
   int functionId = -1;
   
   if(l>(sizeof(name)-1))
      l=sizeof(name)-1;
  
   strncpy(name, str, l);
   name[l]=0;

   for(int i=0; functions[i]; i++)
   {
      if(strcmp(functions[i], name)==0)
      {
         functionId=i+128;
         break;
      }
   }
   return functionId;
}


union eval_token_u {
   double d;
   int v;
   int f;
};

enum eval_token_e { NUMERIC_T, VARIABLE_T, FUNCTION_T };

// enum eval_token_e getEvalToken(char *str, char **newptr, double *v)
enum eval_token_e getEvalToken(char *str, char **newptr, union eval_token_u *v)
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
//               *v=d;
               v->d=d;
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

               if(_getVarVal != NULL)
               {
                  double d = 0.0;

                  if(_getVarId(name, getVarUserData, &d)<0)
                     return -1;
//                  *v=d;
                  v->v=d;
               }
               p++;
               *newptr=p;
               return VARIABLE_T;
            }
            else
               return -1;
         }
         break;

      default:
         {
            ++p;

            if(!isalpha(*p))
               return -1;
            ++p;
            while(*p && isalpha(*p))
               ++p;
            int ret=getFunctionId(str, (int)(p-str));
            if(ret>=0)
            {
//               *v=(double)ret;
               v->f=ret;
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


static int doEvalFunction(double *d, int fn, double d1)
{
   switch(fn-128)
   {
      case INT_F: // int()
        *d=(double)((int)d1);
        break;
      default:
        return -1;
   }
   return 0;
}


static int doEvalOperation(double *d, double d1, int op, double d2)
{
   switch(op)
   {
      case '+':
         *d=d1+d2;
         break;
      case '-':
         *d=d1-d2;
         break;
      case '/':
          if(d2==0.0)
             return -1;
         *d=d1/d2;
         break;
      default:
      case '*':
         *d=d1*d2;
         break;
         return -1;
   }
   return 0;
}


static int getEvalOperatorPriority(int op)
{
   if(op>127) // c'est un id fonction
      return 3;

   switch(op)
   {
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


static int getEvalOperatorPriorityCmp(int op1, int op2)
{
   return getEvalOperatorPriority(op1) - getEvalOperatorPriority(op2);
}


static int pushToEvalStack(int type, void *value, struct eval_stack_s **stack, int *stack_size, int *stack_index)
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
      case 2:
         s[*stack_index].val.op=*((int *)value);
         break;
      case 3:
         s[*stack_index].val.id=*((int *)value);
         break;
      default:
         return -1;
   }

   return 0;
}


static int execOperator(int op, struct eval_stack_s *stack, int *stack_size, int *stack_index)
{
   double d, d1, d2;

   if(op<128)
   {
      d2=stack[(*stack_index)--].val.value;
      d1=stack[(*stack_index)--].val.value;
      if(doEvalOperation(&d, d1, op, d2)<0)
         return -1;
   }
   else
   {
      if(op==255) // c'est une variable
      {
         d1=stack[(*stack_index)--].val.value;
         if(_getVarVal((int)d1, getVarUserData, &d)<0)
            return -1; 
      }
      else
      {
         d1=stack[(*stack_index)--].val.value;
         if(doEvalFunction(&d, op, d1)<0)
            return -1;
      }
   }
   pushToEvalStack(1, (void *)&d, &stack, stack_size, stack_index);
   
   return 0;
}


static int _evalCalc(char *str, char **newptr, int *lvl, struct eval_stack_s *stack, int *stack_size, int *stack_index, int *err)
{
   int operators[10];
   int operators_index=-1;

   if(!*str)
   {
      *err=1; // pas de ligne à évaluer
      return -1;
   }

   if(*lvl>10)
   {
      *err=2; // trop de niveau de parenthèse
      return -1;
   }

//   double v;
   char *p=str;
   char *s;

   do
   {
//      int ret=0;

      s=p;
      getSpace(s, &p);
      
      s=p;
      if(*s=='(')
      {
         s++;
         *lvl=*lvl+1;
         if(_evalCalc(s, &p, lvl, stack, stack_size, stack_index, err)<0)
            return -1;
         *lvl=*lvl-1;
         
         s=p;
         getSpace(s, &p);
         
         if(*p==')')
            p++;
         else
         {
            *newptr=p;
            *err=3; // parenthèse fermante attendu (expression)
            return -1;
         }
      }
      else
      {
         union eval_token_u v;
//         int ret=getEvalToken(s, &p, &v);
         int ret=getEvalToken(s, &p, &v);
         if(ret==NUMERIC_T)
         {
            pushToEvalStack(1, (void *)&v, &stack, stack_size, stack_index);
         }
         else if(ret==VARIABLE_T)
         {
#if DIRECTINTERP==0
            int f=255;
            pushToEvalStack(1, (void *)&v, &stack, stack_size, stack_index);
            pushToEvalStack(2, (void *)&f, &stack, stack_size, stack_index);
#else
            double d=0.0;
//            if(_getVarVal((int)v, getVarUserData, &d) < 0)
            if(_getVarVal(v.v, getVarUserData, &d) < 0)
            {
               *newptr=p;
               *err=10;
               return -1;
            }
            else
               pushToEvalStack(1, (void *)&d, &stack, stack_size, stack_index);
#endif
         }
         else if(ret==FUNCTION_T)
         {
            s=p;
            getSpace(s, &p);
            
            s=p;
            if(*s!='(')
            {
               *newptr=p;
               *err=6; // parenthèse ouvrante attendu
               return -1;
            }
            else
            {
               s++;
               *lvl=*lvl+1;
               if(_evalCalc(s, &p, lvl, stack, stack_size, stack_index, err)<0)
                  return -1;
               *lvl=*lvl-1;
               s=p;
               getSpace(s,&p);
               
               s=p;
               if(*s!=')')
               {
                  *newptr=p;
                  *err=7; // parenthèse fermante attendu (fonction)
                  return -1;
               }
               p++;
               s=p;

//               int op=(int)v;
               int op=v.f;

               if(getEvalOperatorPriorityCmp(op, operators[operators_index])<=0)
               {
#if DIRECTINTERP==0
                  pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
                  operators[operators_index]=op;
#else
                  if(execOperator(operators[operators_index], stack, stack_size, stack_index) < 0)
                  {
                     *newptr=p;
                     *err=11;
                     return -1;
                  }
                  operators[operators_index]=op;
#endif
               }
               else
               {
                  operators[++operators_index]=op;
               }
            }
         }
         else
         {
            *newptr=p;
            *err=4; // pas d'opérande
            return -1;
         }
      }
      
      s=p;
      getSpace(s, &p);
      
      s=p;
//      ret=getOperator(s, &p);
      int op=getOperator(s, &p);
//      if(ret!=-1)
      if(op!=-1)
      {
         if(operators_index==-1)
//            operators[++operators_index]=ret;
            operators[++operators_index]=op;
         else
         {
//            if(getEvalOperatorPriorityCmp(ret, operators[operators_index])<=0)
            if(getEvalOperatorPriorityCmp(op, operators[operators_index])<=0)
            {
#if DIRECTINTERP==0
               pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
               operators[operators_index]=ret;
#else
               if(execOperator(operators[operators_index], stack, stack_size, stack_index) < 0)
               {
                  *newptr=p;
                  *err=11;
                  return -1;
               }
//               operators[operators_index]=ret;
               operators[operators_index]=op;
#endif
            }
            else
//               operators[++operators_index]=ret;
               operators[++operators_index]=op;
         }
      }
      else if((*s == 0) || (*s == ')' && *lvl > 0))
         break;
      else
      {
         *newptr=p;
         if(*s==')')
            *err=8;
         else
            *err=9;
         return -1;
      }
   }
   while(1);
   
   *newptr=p;
#if DIRECTINTERP==0
   for(;operators_index>=0;--operators_index)
      pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
#else
   for(;operators_index>=0;--operators_index)
      execOperator(operators[operators_index], stack, stack_size, stack_index);
#endif
   return 0;
}


int setGetVarCallBacks(getVarId_f fid, getVarVal_f fval, void *userdata)
{
   _getVarVal=fval;
   _getVarId=fid;
   getVarUserData=userdata;
   
   return 0;
} 


#if DIRECTINTERP==0
struct eval_stack_s *getEvalStack(char *str, char **p, int *err, int *stack_ptr)
{
   int lvl=0;
   struct eval_stack_s *stack, *tmp;
   int stack_index=-1;
   int stack_size = 80;

   stack = (struct eval_stack_s *)malloc(stack_size * (sizeof(struct eval_stack_s)));
   if(stack==NULL)
      return NULL;

   int ret=_evalCalc(str, p, &lvl, stack, &stack_size, &stack_index, err);
   if(ret < 0)
      return NULL;

   tmp=stack;
   stack=realloc(stack, (stack_index+1)*(sizeof(struct eval_stack_s)));
   if(stack==NULL)
   {
      free(stack);
      return NULL;
   }

   *stack_ptr=stack_index;

   return stack;
}


int evalCalc(struct eval_stack_s *stack, int stack_ptr, double *r)
{
   struct eval_stack_s exec_stack[20];
   int exec_stack_index=-1;
   double d, d1, d2;

   for(int i=0;i<=stack_ptr;++i)
   {
      if(stack[i].type==1)
         exec_stack[++exec_stack_index].val.value = stack[i].val.value;
      else
      {
         int op = stack[i].val.op;
         if(op<128)
         {
            d2=exec_stack[exec_stack_index--].val.value;
            d1=exec_stack[exec_stack_index--].val.value;
            if(doEvalOperation(&d, d1, op, d2)<0)
               return -1;
         }
         else
         {
            if(op==255)
            {
               d1=exec_stack[exec_stack_index--].val.value;
               if(_getVarVal((int)d1, getVarUserData, &d)<0)
                  return -1;
            }
            else
            {
               d1=exec_stack[exec_stack_index--].val.value;
               if(doEvalFunction(&d, op, d1)<0)
                  return -1;
            }
         }
         exec_stack[++exec_stack_index].val.value=d;
      }
   }

   if(exec_stack_index!=0)
   {
      *r=0.0;
      return -1;
   }
   else
   {
      *r=exec_stack[0].val.value;
      return 0;
   }

   return 0;
}


#else
int evalCalc(char *str, char **p, double *r, int *err)
{
   int lvl=0;
   struct eval_stack_s *stack;
   int stack_index=-1;
   int stack_size = 80;
 
   stack = (struct eval_stack_s *)malloc(stack_size * (sizeof(struct eval_stack_s)));
   if(stack==NULL)
      return -1;

   int ret=_evalCalc(str, p, &lvl, stack, &stack_size, &stack_index, err);

   if(ret < 0)
   {
      free(stack);
      return -1;
   }
   if(stack_index!=0 && stack[0].type!=1 && *p != 0)
   {
      *r=0.0;
      free(stack);
      return -1;
   }
   else
   {
      *r=stack[0].val.value;
      free(stack);
      return 0;
   }
}
#endif


#if DIRECTINTERP==0
int calcn(char *str, double *r)
{
   char *p;
   int err;
   double d;

   int stack_ptr=0;

   struct eval_stack_s *stack=getEvalStack(str, &p, &err, &stack_ptr);
   if(stack == NULL)
      return -1;

   int ret=evalCalc(stack, stack_ptr, &d);

   *r=d;

   free(stack);

   return ret;
}
#else
int calcn(char *str, double *r)
{
   char *p;
   int err;
   double d;

   int ret=evalCalc(str, &p, &d, &err);

   *r=d;

   return ret;
}
#endif


#ifdef EVAL_MODULE_TEST
int myGetVarId(char *str, void *userdata, double *d)
{
   *d=1234.0;

   return 0;
}

int myGetVarVal(int id, void *userdata, double *d)
{
   *d=5678;

   return 0;
}

int main(int argc, char *argv[])
{
//   char *expr = "int(#99) + #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";
//   char *expr = "sin(#2 + #1 + {tata} * #12) - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";
//   char *expr = "(#1 + #2 * #3)";
   char *expr1 = "int((#2 + #1 + {tata} * #12) - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3)))";
//   char *expr1 = "int(#2.2)";
   
//   char *expr2 = "(#2 + #1 + (#-9999) * #12) - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";
   char *expr2 = "int(#2 + #1 + (#-9999) * #12) - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";
   char *expr3 = "int(#123.12)";
   char *expr4 ="{tata}+{toto}";

   char *p;
   int err;
   double d;
   
   setGetVarCallBacks(&myGetVarId, &myGetVarVal, NULL);
  
#if DIRECTINTERP==0
   int stack_ptr=0;
   struct eval_stack_s *stack=getEvalStack(expr1, &p, &err, &stack_ptr);
   if(stack == NULL)
   {
      fprintf(stderr,"> get stack error (%d) at \"%s\"\n", err, p);
      return -1;
   }
   
   for(int i=0; i<=stack_ptr; i++)
   {
      if(stack[i].type==1)
      {
         fprintf(stderr,"v = %f\n", stack[i].val.value);
      }
      else
      {
         if(stack[i].val.op<128)
            fprintf(stderr,"op = %c\n", stack[i].val.op);
         else
            fprintf(stderr,"f = %d\n", stack[i].val.op);
      }
   }

   int ret=evalCalc(stack, stack_ptr, &d);
   if(ret<0)
   {
      fprintf(stderr,"> Evalution error");
   }
   else 
      fprintf(stderr,"#%f\n", d);
#else
   fprintf(stderr,"\n\nEXPR1\n"); 
   int ret=evalCalc(expr1, &p, &d, &err);
   if(ret<0)
   {
      fprintf(stderr,"> Evalution error (%d) at \"%s\"\n", err, p);
   }
   else 
      fprintf(stderr," #%f\n", d);

   fprintf(stderr,"\n\nEXPR2\n"); 
   ret=evalCalc(expr2, &p, &d, &err);
   if(ret<0)
   {
      fprintf(stderr,"> Evalution error (%d) at \"%s\"\n", err, p);
   }
   else 
      fprintf(stderr," #%f\n", d);

   fprintf(stderr,"\n\nEXPR3\n"); 
   ret=evalCalc(expr3, &p, &d, &err);
   if(ret<0)
   {
      fprintf(stderr,"> Evalution error (%d) at \"%s\"\n", err, p);
   }
   else 
      fprintf(stderr," #%f\n", d);

   fprintf(stderr,"\n\nEXPR4\n"); 
   ret=evalCalc(expr4, &p, &d, &err);
   if(ret<0)
   {
      fprintf(stderr,"> Evalution error (%d) at \"%s\"\n", err, p);
   }
   else 
      fprintf(stderr," #%f\n", d);
   
   double r;
   calcn("#1+#2*#3", &r);
   fprintf(stderr,"\n%f\n", r);
#endif
}
#endif