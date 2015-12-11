//
//  mea_eval.c
//
//  Created by Patrice DIETSCH on 10/12/15.
//
//
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
#include <inttypes.h>

#include "mea_string_utils.h"
#include "mea_eval.h"

static void *getVarUserData = NULL;

static getVarVal_f _getVarVal = NULL;
static getVarId_f _getVarId = NULL;


static inline int getEvalOperatorPriorityCmpN(int op1, int op2);
static inline int getOperatorN(char *str, char **newptr);

static inline void getSpace(char *p, char **newptr);
static inline int getFunctionId(char *str, int l);


//static int getSpace(char *str, char **newptr)
static void getSpace(char *p, char **newptr)
{
   while(*p && isspace(*p)) p++;
   *newptr=p;
}


static int getOperatorN(char *str, char **newptr)
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

   for(int i=0; functions[i]; ++i)
   {
      if(strcmp(functions[i], name)==0)
      {
         functionId=i+128;
         break;
      }
   }
   return functionId;
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


union eval_token_data_u {
   double floatval;
   int var_id;
   int fn_id;
};


/*
Un token pour une formule "numérique" est soit :
- un nombre réel : NUMERIC_T
- une variable ({nomVar}) : VARIABLE_T => le numero de la variable doit être retournée par _getVarId(nomVar, ...)
- une fonction (fn(x)) : FUNCTION_T
*/
enum eval_token_type_id_e { NUMERIC_T, VARIABLE_T, FUNCTION_T };

// enum eval_token_e getEvalToken(char *str, char **newptr, double *v)
static enum eval_token_type_id_e getEvalToken(char *str, char **newptr, union eval_token_data_u *v)
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
               v->floatval=d;
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
                  int16_t id = 0;

                  if(_getVarId != NULL && _getVarId(name, getVarUserData, &id)<0)
                     return -1;
                  v->var_id=id;
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
               v->fn_id=ret;
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


static int doEvalOperationN(double *d, double d1, int op, double d2)
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


static int getEvalOperatorPriorityN(int op)
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



static int getEvalOperatorPriorityCmpN(int op1, int op2)
{
   return getEvalOperatorPriorityN(op1) - getEvalOperatorPriorityN(op2);
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


static int execOperatorN(int op, struct eval_stack_s *stack, int *stack_size, int *stack_index)
{
   double d, d1, d2;

   if(op<128)
   {
      d2=stack[(*stack_index)--].val.value;
      d1=stack[(*stack_index)--].val.value;
      if(doEvalOperationN(&d, d1, op, d2)<0)
         return -1;
   }
   else
   {
      if(op==255) // c'est une variable
      {
         d1=stack[(*stack_index)--].val.value;
         if(_getVarVal != NULL && _getVarVal((int)d1, getVarUserData, &d)<0)
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


static int _evalCalcN(char *str, char **newptr, int16_t *lvl, struct eval_stack_s *stack, int32_t *stack_size, int32_t *stack_index, int16_t *err)
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

   char *p=str;
   char *s;

   do
   {
      s=p;
      getSpace(s, &p);
      
      s=p;
      if(*s=='(')
      {
         ++s;
         *lvl=*lvl+1;
         if(_evalCalcN(s, &p, lvl, stack, stack_size, stack_index, err)<0)
            return -1;
         *lvl=*lvl-1;
         
         s=p;
         getSpace(s, &p);
         
         if(*p==')')
            ++p;
         else
         {
            *newptr=p;
            *err=3; // parenthèse fermante attendu (expression)
            return -1;
         }
      }
      else
      {
         union eval_token_data_u v;
         int ret=getEvalToken(s, &p, &v);

         if(ret==NUMERIC_T)
         {
            pushToEvalStack(1, (void *)&(v.floatval), &stack, stack_size, stack_index);
         }
         else if(ret==VARIABLE_T)
         {
#if ONFLYEVAL==0
            int f=255;
            pushToEvalStack(3, (void *)&(v.var_id), &stack, stack_size, stack_index);
            pushToEvalStack(2, (void *)&f, &stack, stack_size, stack_index);
#else
            double d=0.0;
            if(_getVarVal(v.var_id, getVarUserData, &d) < 0)
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
               ++s;
               ++(*lvl);
               if(_evalCalcN(s, &p, lvl, stack, stack_size, stack_index, err)<0)
                  return -1;
               --(*lvl);

               s=p;
               getSpace(s,&p);

               s=p;
               if(*s!=')')
               {
                  *newptr=p;
                  *err=7; // parenthèse fermante attendu (fonction)
                  return -1;
               }
               ++p;

               s=p;
               //int op=v.fn_id;
               if(operators_index==-1)
                  operators[++operators_index]=v.fn_id;
               else
               {
                  if(getEvalOperatorPriorityCmpN(v.fn_id, operators[operators_index])<=0)
                  {
#if ONFLYEVAL==0
                     pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
#else
                     if(execOperatorN(operators[operators_index], stack, stack_size, stack_index) < 0)
                     {
                        *newptr=p;
                        *err=11;
                        return -1;
                     }
#endif
                     operators[operators_index]=v.fn_id;
                  }
                  else
                     operators[++operators_index]=v.fn_id;
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
      int op=getOperatorN(s, &p);
      if(op!=-1)
      {
         if(operators_index==-1)
            operators[++operators_index]=op;
         else
         {
            if(getEvalOperatorPriorityCmpN(op, operators[operators_index])<=0)
            {
#if ONFLYEVAL==0
               pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
//               operators[operators_index]=op;
#else
               if(execOperatorN(operators[operators_index], stack, stack_size, stack_index) < 0)
               {
                  *newptr=p;
                  *err=11;
                  return -1;
               }
//               operators[operators_index]=op;
#endif
               operators[operators_index]=op;
            }
            else
               operators[++operators_index]=op;
         }
      }
      else if((*s == 0) || (*s == ')' && *lvl > 0))
      {
         break;
      }
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
#if ONFLYEVAL==0
   // flush tous les opérateurs sur la pile principale
   for(;operators_index>=0;--operators_index)
      pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
#else
   // évaluation des opérations restantes
   for(;operators_index>=0;--operators_index)
      execOperatorN(operators[operators_index], stack, stack_size, stack_index);
#endif
   return 0;
}


int16_t setGetVarCallBacks(getVarId_f fid, getVarVal_f fval, void *userdata)
{
   _getVarVal=fval;
   _getVarId=fid;
   getVarUserData=userdata;
   
   return 0;
} 


#define DEFAULT_STACK_SIZE 80

#if ONFLYEVAL==0

#define EXEC_STACK_SIZE 20

struct eval_stack_s *getEvalStack(char *str, char **p, int16_t *err, int32_t *stack_ptr)
{
   int16_t lvl=0;
   struct eval_stack_s *stack, *tmp;
   int32_t stack_index=-1;
   int stack_size = DEFAULT_STACK_SIZE;

   stack = (struct eval_stack_s *)malloc(stack_size * (sizeof(struct eval_stack_s)));
   if(stack==NULL)
      return NULL;

   int ret=_evalCalcN(str, p, &lvl, stack, &stack_size, &stack_index, err);
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


void displayStack(struct eval_stack_s *stack, int32_t stack_ptr)
{
   for(int i=0; i<=stack_ptr; ++i)
   {
      if(stack[i].type==1)
      {
         fprintf(stderr,"v = %f\n", stack[i].val.value);
      }
      else if(stack[i].type==2)
      {
         if(stack[i].val.op<128)
            fprintf(stderr,"op = %c\n", stack[i].val.op);
         else
            fprintf(stderr,"f = %d\n", stack[i].val.op);
      }
      else
         fprintf(stderr,"id = %d\n", stack[i].val.id);
   }
}


int16_t evalCalc(struct eval_stack_s *stack, int32_t stack_ptr, double *r)
{
   struct eval_stack_s exec_stack[EXEC_STACK_SIZE];
   int32_t exec_stack_index=-1;
   double d, d1, d2;

   for(int i=0;i<=stack_ptr;++i)
   {
      if(stack[i].type==1)
         exec_stack[++exec_stack_index].val.value = stack[i].val.value;
      else if(stack[i].type==2)
      {
         int op = stack[i].val.op;
         if(op<128)
         {
            d2=exec_stack[exec_stack_index--].val.value;
            d1=exec_stack[exec_stack_index--].val.value;
            if(doEvalOperationN(&d, d1, op, d2)<0)
               return -1;
         }
         else
         {
            if(op==255)
            {
               int var_id=exec_stack[exec_stack_index--].val.id;
               if(_getVarVal(var_id, getVarUserData, &d)<0)
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
      else if(stack[i].type==3)
         exec_stack[++exec_stack_index].val.id = stack[i].val.id;
      else
         return -1;

        if(exec_stack_index >= EXEC_STACK_SIZE-1)
        {
           fprintf(stderr,"execution stack overflow\n");
           return -1;
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
int16_t evalCalc(char *str, char **p, double *r, int16_t *err)
{
   int16_t lvl=0;
   struct eval_stack_s *stack;
   int32_t stack_index=-1;
   int32_t stack_size = DEFAULT_STACK_SIZE;
 
   stack = (struct eval_stack_s *)malloc(stack_size * (sizeof(struct eval_stack_s)));
   if(stack==NULL)
      return -1;

   int16_t ret=_evalCalcN(str, p, &lvl, stack, &stack_size, &stack_index, err);

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


#if ONFLYEVAL==0
int16_t calcn(char *str, double *r)
{
   char *p;
   int16_t err;
   double d;

   int32_t stack_ptr=0;

   struct eval_stack_s *stack=getEvalStack(str, &p, &err, &stack_ptr);
   if(stack == NULL)
      return -1;

   int ret=evalCalc(stack, stack_ptr, &d);

   *r=d;

   free(stack);

   return ret;
}
#else
int16_t calcn(char *str, double *r)
{
   char *p;
   int16_t err;
   double d;

   int ret=evalCalc(str, &p, &d, &err);

   *r=d;

   return ret;
}
#endif


#ifdef EVAL_MODULE_TEST
double millis()
{
   struct timeval te;
   gettimeofday(&te, NULL);

   double milliseconds = (double)te.tv_sec*1000.0 + (double)te.tv_usec/1000.0;

   return milliseconds;
}


int16_t myGetVarId(char *str, void *userdata, int16_t *id)
{
   *id=1234;

   return 0;
}

int16_t myGetVarVal(int16_t id, void *userdata, double *d)
{
   *d=5678;

   return 0;
}

int main(int argc, char *argv[])
{
   char *expr = "int(#2 + #1 + {tata} * #12) - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";

   char *p;
   int16_t err;
   double d;
   
   setGetVarCallBacks(&myGetVarId, &myGetVarVal, NULL);

   double t0=millis();

#if ONFLYEVAL==0
   int stack_ptr=0;
   struct eval_stack_s *stack=getEvalStack(expr, &p, &err, &stack_ptr);
   if(stack == NULL)
   {
      fprintf(stderr,"> get stack error (%d) at \"%s\"\n", err, p);
      return -1;
   }

   // displayStack(stack, stack_ptr);

   for(int i=0;i<400000;++i)
   {
      int ret=evalCalc(stack, stack_ptr, &d);
      if(ret<0)
      {
         fprintf(stderr,"> evalution error\n");
         return -1;
      }
   }
   free(stack);
#else
   for(int i=0;i<100000;++i)
   {
      calcn(expr, &d);
   }
#endif
   fprintf(stderr, "execution time : %5.2f ms\n",millis()-t0);
   fprintf(stderr,"#%f\n", d);

//   double r;
//   calcn("int(#1.1 * (#2 + #3) * #4 + {tata})", &r);
//   fprintf(stderr,"%f\n",r);
}
#endif

