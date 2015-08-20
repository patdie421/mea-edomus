//
//  xPL_strings.c
//
//  Created by Patrice DIETSCH on 29/10/12.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "string_utils.h"
#include "debug.h"
#include "error.h"

#include "tokens.h"
#include "consts.h"

struct token_s
{
   char *str;
   int16_t id;
};

struct token_s tokens_list[]={
   {"control",                    XPL_CONTROL_ID},
   {"basic",                      XPL_BASIC_ID},
   {"device",                     XPL_DEVICE_ID},
   {"type",                       XPL_TYPE_ID},
   {"output",                     XPL_OUTPUT_ID},
   {"current",                    XPL_CURRENT_ID},
   {"sensor",                     XPL_SENSOR_ID},
   {"pulse",                      XPL_PULSE_ID},
   {"command",                    XPL_COMMAND_ID},
   {"data1",                      XPL_DATA1_ID},
   {"request",                    XPL_REQUEST_ID},
   {"energy",                     XPL_ENERGY_ID},
   {"power",                      XPL_POWER_ID},
   {"temp",                       XPL_TEMP_ID},
   {"input",                      XPL_INPUT_ID},
   {"digital_in",                 DIGITAL_IN_ID},
   {"digital_out",                DIGITAL_OUT_ID},
   {"analog_in",                  ANALOG_IN_ID},
   {"analog_out",                 ANALOG_OUT_ID},
   {"voltage",                    XPL_VOLTAGE_ID},
   {"tmp36",                      XPL_TMP36_ID},
   {"aref5",                      XPL_AREF5_ID},
   {"aref11",                     XPL_AREF11_ID},
   {"algo",                       XPL_ALGO_ID},
   {"action",                     ACTION_ID},
   {"pwm",                        PWM_ID},
   {"onoff",                      ONOFF_ID},
   {"variable",                   VARIABLE_ID},
   {"inc",                        INC_ID},
   {"dec",                        DEC_ID},
   {"device_id",                  DEVICE_ID_ID},
   {"device_name",                DEVICE_NAME_ID},
   {"device_type_id",             DEVICE_TYPE_ID_ID},
   {"device_location_id",         DEVICE_LOCATION_ID_ID},
   {"device_interface_name",      DEVICE_INTERFACE_NAME_ID},
   {"device_interface_type_name", DEVICE_INTERFACE_TYPE_NAME_ID},
   {"device_state",               DEVICE_STATE_ID},
   {"device_type_parameters",     DEVICE_TYPE_PARAMETERS_ID},
   {"interface_id",               INTERFACE_ID_ID},
   {"interface_name",             INTERFACE_NAME_ID},
   {"interface_type_id",          INTERFACE_TYPE_ID_ID},
   {"interface_location_id",      INTERFACE_LOCATION_ID_ID},
   {"interface_state",            INTERFACE_STATE_ID},
   {"interface_parameters",       INTERFACE_PARAMETERS_ID},
   {"device_parameters",          DEVICE_PARAMETERS_ID},
   {"ID_XBEE",                    ID_XBEE_ID},
   {"ADDR_H",                     ADDR_H_ID},
   {"ADDR_L",                     ADDR_L_ID},
   {"digital",                    DIGITAL_ID},
   {"analog",                     ANALOG_ID},
   {"last",                       XPL_LAST_ID},
   {"raw",                        RAW_ID},
   {"high",                       HIGH_ID},
   {"low",                        LOW_ID},
   {"generic",                    GENERIC_ID},
   {"unit",                       UNIT_ID},
   {"todbflag",                   TODBFLAG_ID},
   {"ENOCEAN_ADDR",               ENOCEAN_ADDR_ID},
   {"ID_ENOCEAN",                 ID_ENOCEAN_ID},
   {"watchdog",                   XPL_WATCHDOG_ID},
   {"reachable",                  XPL_REACHABLE_ID},
/*
   {"high",                       XPL_HIGH_ID},
   {"low",                        XPL_LOW_ID},
*/
   {"color",                      XPL_COLOR_ID},
   {"true",                       TRUE_ID},
   {"false",                      FALSE_ID},
   {NULL,0}
};

#define TOKEN_BY_HASH_TABLE 1
// #define TOKEN_BY_INDEX 1
// #define TOKEN_BY_SEQUENTIAL_READ 1

#ifdef TOKEN_BY_SEQUENTIAL_READ
void init_tokens()
{
}


//char *get_token_by_id(int id)
//char *get_string_by_id(int id)
char *get_token_string_by_id(int16_t id)
/**
 * \brief     recherche un token par son id
 * \param     id  identifiant du token.
 * \return    pointeur sur le token ou NULL s'il n'existe pas.
 */
{
   for(int i=0;tokens_list[i].str;i++)
   {
      if(tokens_list[i].id == id)
         return tokens_list[i].str;
   }
   return NULL;
}


//int16_t get_id_by_string(char *str)
int16_t get_token_id_by_string(char *str)
/**
 * \brief     recherche l'id un token
 * \param     token (chaine) à trouver.
 * \return    id du token
 */
{
   if(!str)
      return -1;
   
   for(int i=0;tokens_list[i].str;i++)
   {
      if(mea_strcmplower(tokens_list[i].str, str) == 0)
         return tokens_list[i].id;
   }
   return -1;
}


//int16_t int_isin(int val, int list[])
/**
 * \brief     recherche une valeur dans un liste (un table) d'entiers
 * \details   les valeurs de la liste doivent être différentes de 0. 0 indique une fin de liste
 * \param     val  valeur à rechercher.
 * \param     list  liste des valeurs.
 * \return    1 si la valeur est trouvée, 0 sinon.
 */
/*
{
   for(int i=0; list[i]!=-1; i++)
      if(val==list[i])
         return 1;
   return 0;
}
*/
#endif

#ifdef TOKEN_BY_HASH_TABLE
#include "uthash.h"

struct tokens_hash_s
{
   struct token_s *token;
   UT_hash_handle hh_token_by_string;
   UT_hash_handle hh_token_by_id;
};


struct tokens_hash_s *tokens_hash_by_string = NULL;
struct tokens_hash_s *tokens_hash_by_id = NULL;


void init_tokens()
/**
 * \brief     initialisation des listes de hashage
 * \return    pointeur sur le token ou NULL s'il n'existe pas.
 */
{
   struct tokens_hash_s *s = NULL;
   
   tokens_hash_by_string = NULL;
   tokens_hash_by_id = NULL;
   
   for(int i=0;tokens_list[i].str;i++)
   {
      HASH_FIND(hh_token_by_string, tokens_hash_by_string, tokens_list[i].str, strlen(tokens_list[i].str), s);
      if(s)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : string key duplicated (%s/%d)\n",DEBUG_STR, __func__, tokens_list[i].str, tokens_list[i].id);
         continue;
      }
      HASH_FIND(hh_token_by_id, tokens_hash_by_id, &(tokens_list[i].id), sizeof(tokens_list[i].id), s);
      if(s)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : id key duplicated (%s/%d)\n",DEBUG_STR, __func__, tokens_list[i].str, tokens_list[i].id);
         continue;
      }

      s = malloc(sizeof(struct tokens_hash_s));
      s->token=&tokens_list[i];
      HASH_ADD_KEYPTR( hh_token_by_string, tokens_hash_by_string, tokens_list[i].str, strlen(tokens_list[i].str), s );
      HASH_ADD_KEYPTR( hh_token_by_id, tokens_hash_by_id, &(tokens_list[i].id), sizeof(tokens_list[i].id), s );
   }
}


char *get_token_string_by_id(int16_t id)
/**
 * \brief     recherche un token par son id
 * \param     id  identifiant du token.
 * \return    pointeur sur le token ou NULL s'il n'existe pas.
 */
{
   struct tokens_hash_s *s = NULL;
   
   if(tokens_hash_by_string == NULL)
      init_tokens();
      
   HASH_FIND(hh_token_by_id, tokens_hash_by_id, &id, sizeof(id), s);
   
   if(s)
      return s->token->str;

   return NULL;
}


int16_t get_token_id_by_string(char *str)
/**
 * \brief     recherche l'id un token
 * \param     token (chaine) à trouver.
 * \return    id du token
 */
{
   struct tokens_hash_s *s = NULL;

   if(!str)
      return -1;
      
   if(tokens_hash_by_id == NULL)
      init_tokens();
 
   HASH_FIND(hh_token_by_string, tokens_hash_by_string, str, strlen(str), s);
   
   if(s)
   {
//      DEBUG_SECTION mea_log_printf("%s (%s) : (%s => %d)\n", DEBUG_STR, __func__, str, s->token->id);
      return s->token->id;
   }

//   DEBUG_SECTION mea_log_printf("%s (%s) : (%s => -1)\n", DEBUG_STR, __func__, str);

   return -1;
}
#endif

#ifdef TOKEN_BY_INDEX
struct tokens_index_s
{
   int16_t nb_tokens;
   int16_t *index_by_string;
   int16_t *index_by_id;
};

struct tokens_index_s *tokens_index=NULL;

int qsort_compare_ids(const void * a, const void * b)
{
/*
   int16_t _a,_b;
   
   _a = *(int16_t *)a;
   _b = *(int16_t *)b;

   int16_t id_a, id_b;
   
   id_a = tokens_list[_a].id;
   id_b = tokens_list[_b].id;

   if(id_a==id_b)
      return 0;
   else if(id_a<id_b)
      return -1;
   else
      return 1;
*/
   return tokens_list[*(int16_t *)a].id - tokens_list[*(int16_t *)b].id;
}


int qsort_compare_strings(const void *a, const void *b)
{
/*
   int16_t _a,_b;
   char *str_a, *str_b;
   
   _a = *(int16_t *)a;
   _b = *(int16_t *)b;

   str_a = tokens_list[_a].str;
   str_b = tokens_list[_b].str;

   return strcmp(str_a,str_b);
*/
   return strcmp(tokens_list[*(int16_t *)a].str, tokens_list[*(int16_t *)b].str);
}


void init_tokens()
{
   if(tokens_index!=NULL)
   {
      free(tokens_index);
   }
   tokens_index=(struct tokens_index_s *)malloc(sizeof(struct tokens_index_s));
   if(tokens_index==NULL)
   {
      perror("init_tokens_index");
      return;
   }
   
   int i=0;
   for(;tokens_list[i].str;i++);
   
   if(i>0)
   {
      tokens_index->nb_tokens=i;
   
      tokens_index->index_by_string = (int16_t *)malloc(tokens_index->nb_tokens*sizeof(int16_t));
      if(tokens_index->index_by_string == NULL)
      {
         perror("init_tokens_index");
         return;
      }
      tokens_index->index_by_id = (int16_t *)malloc(tokens_index->nb_tokens*sizeof(int16_t));
      if(tokens_index->index_by_id  == NULL)
      {
         perror("init_tokens_index");
         free(tokens_index->index_by_string);
         tokens_index->index_by_string=NULL;
         return;
      }

      for(int i=0;i<tokens_index->nb_tokens;i++)
      {
         tokens_index->index_by_string[i]=i;
         tokens_index->index_by_id[i]=i;
      }

      qsort (tokens_index->index_by_id, tokens_index->nb_tokens, sizeof (int16_t), qsort_compare_ids);
      qsort (tokens_index->index_by_string, tokens_index->nb_tokens, sizeof (int16_t), qsort_compare_strings);
   }
}


char *get_token_string_by_id(int16_t id)
{
   if(tokens_index==NULL)
   {
      init_tokens();
      if(tokens_index==NULL)
         return NULL;
   }
   
   int16_t start = 0;
   int16_t end = tokens_index->nb_tokens - 1;
   int16_t _cmpres;
   for(;;)
   {
      int16_t middle=(end - start) / 2;
      if(middle<0)
      {
//         DEBUG_SECTION mea_log_printf("%s (%s) : (%d => NULL)\n", DEBUG_STR, __func__, id);
         return NULL;
      }
      middle+=start;
      
      _cmpres=tokens_list[tokens_index->index_by_id[middle]].id - id;
      if(_cmpres==0)
      {
//         DEBUG_SECTION mea_log_printf("%s (%s) : (%d => %s)\n", DEBUG_STR, __func__, id, tokens_list[tokens_index->index_by_id[middle]].str);
         return tokens_list[tokens_index->index_by_id[middle]].str;
      }
      if(_cmpres<0)
         start=middle;
      if(_cmpres>0)
         end=middle;
   }
}


int16_t get_token_id_by_string(char *str)
{
   if(tokens_index==NULL)
   {
      init_tokens();
      if(tokens_index==NULL)
         return -1;
   }

   if(!str)
      return -1;
      
   int16_t start = 0;
   int16_t end = tokens_index->nb_tokens - 1;
   int _cmpres;
   
   for(;;)
   {
      int16_t middle = (end - start) / 2;
      
      if(middle<0)
      {
//         DEBUG_SECTION mea_log_printf("%s (%s) : (%s => -1)\n", DEBUG_STR, __func__, str);
         return -1;
      }
      middle+=start;
      
      _cmpres=strcmp(tokens_list[tokens_index->index_by_string[middle]].str,str);
      if(_cmpres==0)
      {
         return tokens_list[tokens_index->index_by_string[middle]].id;
//         DEBUG_SECTION mea_log_printf("%s (%s) : (%s => %d)\n", DEBUG_STR, __func__, str, tokens_list[tokens_index->index_by_string[middle]].id);

      }
      if(_cmpres<0)
         start=middle;
      if(_cmpres>0)
         end=middle;
   }
}
#endif
