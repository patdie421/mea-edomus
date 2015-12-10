//
//  mea_eval.h
//
//  Created by Patrice DIETSCH on 10/12/15.
//
//
#define DIRECTINTERP 1

typedef int (*getVarVal_f)(int id, void *userdata, double *d);
typedef int (*getVarId_f)(char *str, void *userdata, double *d);

struct eval_stack_s {
   char type;
   union {
      int op;
      double value;
      int id;
   } val;
};

int setGetVarCallBacks(getVarId_f fid, getVarVal_f fval, void *userdata);

#if DIRECTINTERP==0
struct eval_stack_s *getEvalStack(char *str, char **p, int *err, int *stack_ptr);
int evalCalc(struct eval_stack_s *stack, int stack_ptr, double *r);
#else
int evalCalc(char *str, char **p, double *r, int *err);
#endif
int calcn(char *str, double *r);
