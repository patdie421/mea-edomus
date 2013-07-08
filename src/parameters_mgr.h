//
//  parameters_mgr.h
//
//  Created by Patrice DIETSCH on 24/10/12.
//
//

#ifndef __parameters_mgr_h
#define __parameters_mgr_h

#define PARAM_SYSTEM_ERR 1
#define PARAM_SYSTAX_ERR 2
#define PARAM_UNKNOW_ERR 3
#define PARAM_NOVAL_ERR  4

typedef union
{
   int i;
   long l;
   float f;
   char *s;
} value_token_t;

typedef struct parsed_parameters_s
{
   char *label;
   enum { INT=1, LONG=2, FLOAT=3, STRING=4 } type;
   value_token_t value;
} parsed_parameters_t;


struct assoc_s
{
   int val1;
   int val2;
};

parsed_parameters_t *malloc_parsed_parameters(char *parameters_string, char *parameters_to_find[], int *nb_params, int *err, int value_to_upper)
;
void free_parsed_parameters(parsed_parameters_t *params, int nb_params);
void display_parsed_parameters(parsed_parameters_t *params, int nb_params);

int16_t is_in_assocs_list(struct assoc_s *assocs_list, int val1, int val2);

#endif
