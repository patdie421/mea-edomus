//
//  parameters_mgr.c
//
//  Created by Patrice DIETSCH on 24/10/12.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#include "uthash.h"

#include "macros.h"
#include "mea_verbose.h"

#include "parameters_utils.h"
#include "mea_string_utils.h"


#ifdef PARSED_PARAMETERS_CACHING_ENABLED
struct _parsed_parameters_cache_elems_s
{
   char *parameters_string;
   time_t last_access;
   parsed_parameters_t *parsed_parameters;

   UT_hash_handle hh;
};


struct _parsed_parameters_cache_list_s
{
   char **parameters_to_find;
   struct _parsed_parameters_cache_elems_s *parsed_parameters_cache_elems;

   UT_hash_handle hh;
};


static struct _parsed_parameters_cache_list_s *_parsed_parameters_cache_list=NULL;
static uint32_t _parsed_parameters_cache_counter=0;

static pthread_rwlock_t *_parsed_parameters_cache_rwlock = NULL;


static void _parsed_parameters_clean_cache(time_t t)
{
   if(_parsed_parameters_cache_rwlock==NULL)
      goto _parsed_parameters_clean_cache_clean_exit;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)_parsed_parameters_cache_rwlock );
   pthread_rwlock_wrlock(_parsed_parameters_cache_rwlock);

   // à faire ...
   
_parsed_parameters_clean_cache_clean_exit:
   pthread_rwlock_unlock(_parsed_parameters_cache_rwlock);
   pthread_cleanup_pop(0);  
}


static int16_t _parsed_parameters_add_to_cache(char **parameters_to_find, int nb_params, char *parameters_string, parsed_parameters_t *parsed_parameters)
{
   struct _parsed_parameters_cache_list_s *s = NULL;
   struct _parsed_parameters_cache_elems_s *e = NULL;

   if(_parsed_parameters_cache_counter >= PARSED_PARAMETERS_CACHING_MAX)
      return -1;

   if(_parsed_parameters_cache_rwlock==NULL)
   {
#ifdef PARSED_PARAMETERS_CACHING_AUTOINIT   
      _parsed_parameters_cache_rwlock=(pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t));
      if(!_parsed_parameters_cache_rwlock)
      {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return -1;
      }
      pthread_rwlock_init(_parsed_parameters_cache_rwlock, NULL);
#else
      return -1; // pas de verrou on ne peut pas utiliser le cache
#endif
   }

   pthread_cleanup_push((void *)pthread_rwlock_unlock, _parsed_parameters_cache_rwlock);
   pthread_rwlock_wrlock(_parsed_parameters_cache_rwlock);


   HASH_FIND(hh, _parsed_parameters_cache_list, parameters_to_find, nb_params * sizeof(char *), s);
   if(!s)
   {
      s = malloc(sizeof(struct _parsed_parameters_cache_list_s));
      if(!s)
      {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         goto _parsed_parameters_add_to_cache_clean_exit;
      }
      s->parameters_to_find = parameters_to_find;
      s->parsed_parameters_cache_elems=NULL;
      
      HASH_ADD_KEYPTR(hh, _parsed_parameters_cache_list, s->parameters_to_find, nb_params * sizeof(char *), s );
   }

   HASH_FIND(hh, s->parsed_parameters_cache_elems, parameters_string, strlen(parameters_string), e);
   if(e)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : id key duplicated (%s)\n",DEBUG_STR, __func__, parameters_string);
   }
   else
   {
      e = malloc(sizeof(struct _parsed_parameters_cache_elems_s));
      if(!e)
      {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         goto _parsed_parameters_add_to_cache_clean_exit;
      }
      e->parameters_string = mea_string_alloc_and_copy(parameters_string);
      if(!e->parameters_string)
      {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         free(e);
         e=NULL;
         goto _parsed_parameters_add_to_cache_clean_exit;
      }
      e->parsed_parameters = parsed_parameters;
      e->last_access=time(NULL);
      HASH_ADD_KEYPTR(hh, s->parsed_parameters_cache_elems, e->parameters_string, strlen(e->parameters_string), e );
      parsed_parameters->from_cache=1;
      _parsed_parameters_cache_counter++;
   }

_parsed_parameters_add_to_cache_clean_exit:
   pthread_rwlock_unlock(_parsed_parameters_cache_rwlock);
   pthread_cleanup_pop(0);
}


static parsed_parameters_t *_parsed_parameters_get_from_cache(char *parameters_to_find[], int nb_params, char *parameters_string)
{
   struct _parsed_parameters_cache_list_s *s = NULL;
   struct _parsed_parameters_cache_elems_s *e = NULL;
   parsed_parameters_t *ret=NULL;

   if(_parsed_parameters_cache_rwlock==NULL)
      return NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)_parsed_parameters_cache_rwlock );
   pthread_rwlock_rdlock(_parsed_parameters_cache_rwlock);
      
   HASH_FIND(hh, _parsed_parameters_cache_list, parameters_to_find, nb_params * sizeof(char *), s);
   if(!s)
      goto _parsed_parameters_get_clean_exit;
  
   HASH_FIND(hh, s->parsed_parameters_cache_elems, parameters_string, strlen(parameters_string), e);
   if(!e)
     goto _parsed_parameters_get_clean_exit;
   
   e->last_access=time(NULL);
   ret=e->parsed_parameters;
   
_parsed_parameters_get_clean_exit:
   pthread_rwlock_unlock(_parsed_parameters_cache_rwlock);
   pthread_cleanup_pop(0);  
   
   return ret;
}
#endif


int16_t is_in_assocs_list(struct assoc_s *assocs_list, int val1, int val2)
{
   for(int i=0;assocs_list[i].val1!=-1;i++)
      if(assocs_list[i].val1==val1 && assocs_list[i].val2==val2)
         return 1;
   return 0;
}


int16_t parsed_parameters_clean_all()
{
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   _parsed_parameters_clean_cache(0);
#endif
}


int16_t parsed_parameters_clean_older_than(time_t t)
{
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   _parsed_parameters_clean_cache(t);
#endif
}


int16_t parsed_parameters_init()
{
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   if(_parsed_parameters_cache_rwlock==NULL)
   {
      _parsed_parameters_cache_rwlock=(pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t));
      if(!_parsed_parameters_cache_rwlock)
      {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return -1;
      }
      pthread_rwlock_init(_parsed_parameters_cache_rwlock, NULL);
   }
#endif
   return 0;
}


char *getToken(char *str)
{
   char *end;
   
   // suppression des blancs avant
   while(isspace(*str) && str)
      str++;
   
   if(*str!=0) // si la chaine n'est pas vide
   {
      end=str+strlen(str) - 1;
      
      // suppression des blancs après
      while(end > str && isspace(*end))
         end--;
      *(end+1)=0;
   }
   else
      return NULL;
   
   // vérification des caractères
   for(int i=0;str[i];i++)
   {
      if(!(isalpha(str[i]) || isdigit(str[i]) || str[i]=='_' || str[i]=='.' || str[i]==':'))
         return NULL;
      
      str[i]=toupper(str[i]);
   }
   
   return str;
}


void clean_parsed_parameters(parsed_parameters_t *params, int nb_params)
{
   for(int i=0;i<nb_params;i++)
   {
      if(params[i].type==STRING)
      {
         if(params[i].value.s)
         {
            free(params[i].value.s);
            params[i].value.s=NULL;
         }
      }
   }
}


void free_parsed_parameters(parsed_parameters_t **params)
{
   if(*params)
   {
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
      if((*params)->from_cache==1)
         return;
#endif
      free(*params);
      *params=NULL;
   }
}


parsed_parameters_t *malloc_parsed_parameters(char *parameters_string, char *parameters_to_find[], int *nb_params, int *err, int value_to_upper)
{
/*
 parsed_parameter_t *mpp=malloc_parsed_parameters("aA=1; b = 00021;C=10;;X=1;A_a =20.0;", tofind, &nb);
 */
   char *ptr = parameters_string;
   char label[21];
   char *label_token;
   char *value;
   char *value_token;
   int n;
   int ret;
   parsed_parameters_t *parsed_parameters;
   
   if(err)
      *err=0;
      
   if(parameters_string == NULL)
   {
      *err=11;
      return NULL;
   }
   
   if(strlen(parameters_string)==0)
   {
      *err=10;
      return NULL;
   }
   
   // nombre max de label dans la liste
   for(*nb_params=0;parameters_to_find[*nb_params];(*nb_params)++); // nombre max de parametres
   
   // préparation du tableau à retourner
   if(!*nb_params)
   {
      // afficher ici un message d'erreur
      return NULL;
   }

#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   parsed_parameters=_parsed_parameters_get_from_cache(parameters_to_find, *nb_params, parameters_string);
   if(parsed_parameters)
   {
/*
      DEBUG_SECTION {
         fprintf (stderr, "%s (%s) : good ! parameters from cache\n", INFO_STR, __func__);
      }
*/
      return parsed_parameters;
   }
#endif

   value=malloc(strlen(parameters_string+1)); // taille de valeur au max (et même un peu plus).
   if(!value)
   {
      if(err) *err=1;
      return NULL;
   }
   
   parsed_parameters=malloc(sizeof(parsed_parameters_t) * *nb_params);
   if(!parsed_parameters)
   {
      DEBUG_SECTION {
         fprintf (stderr, "%s (%s) : %s - ",DEBUG_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      if(err)
         *err=1; // erreur système, voir errno
      return NULL;
   }
   
   memset(parsed_parameters,0,sizeof(parsed_parameters_t) * *nb_params);
   
   while(1)
   {
      ret=sscanf(ptr, "%20[^=;]=%[^;]%n", label, value, &n); // /!\ pas plus de 20 caractères pour un TOKEN
      if(ret==EOF) // plus rien à lire
         break;
      
      if(ret==2)
      {
         // ici on traite les données lues
         label_token=getToken(label);
         if(value_to_upper)
            mea_strtoupper(value);
         value_token=value;
         
         char trouvee=0;
         for(int i=0;parameters_to_find[i];i++)
         {
            char *str=&(parameters_to_find[i][2]);
            if(label_token && strcmp(label_token,str)==0)
            {
               char type=parameters_to_find[i][0];
               trouvee=1;
               parsed_parameters[i].label=str;
               int r=-1;
               switch(type)
               {
                  case 'I':
                     parsed_parameters[i].type=INT;
                     sscanf(value_token,"%d%n",&(parsed_parameters[i].value.i),&r);
                     break;
                     
                  case 'L':
                     parsed_parameters[i].type=LONG;
                     sscanf(value_token,"%ld%n",&(parsed_parameters[i].value.l),&r);
                     break;
                     
                  case 'F':
                     parsed_parameters[i].type=FLOAT;
                     sscanf(value_token,"%f%n",&(parsed_parameters[i].value.f),&r);
                     break;
                     
                  case 'S':
                     parsed_parameters[i].type=STRING;
                     r=strlen(value_token);
                     parsed_parameters[i].value.s=malloc(r+1);
                     if(!parsed_parameters[i].value.s)
                     {
                        DEBUG_SECTION {
                           fprintf (stderr, "%s (%s) : %s - ", DEBUG_STR, __func__, MALLOC_ERROR_STR);
                           perror("");
                        }
                        if(parsed_parameters)
                        {
                           clean_parsed_parameters(parsed_parameters, *nb_params);
                           free(parsed_parameters);
                           parsed_parameters=NULL;
                        }
                        if(err) *err=1; // erreur système, voir errno
                        goto malloc_parsed_parameters_exit;
                     }
                     strcpy(parsed_parameters[i].value.s, value_token);
                     break;
                     
                  default:
                     break;
               }
               if(r!=strlen(value_token))
               {
                  if(parsed_parameters)
                  {
                     clean_parsed_parameters(parsed_parameters, *nb_params);
                     free(parsed_parameters);
                     parsed_parameters=NULL;
                  }
                  if(err) *err=2; // erreur de syntaxe;
                  goto malloc_parsed_parameters_exit;
               }
               break;
            }
         }
         if(!trouvee)
         {
            if(parsed_parameters)
            {
               clean_parsed_parameters(parsed_parameters, *nb_params);
               free(parsed_parameters);
               parsed_parameters=NULL;
            }
            if(err) *err=3; // label inconnu
            goto malloc_parsed_parameters_exit;
         }
         // déplacement du pointeur sur les données suivantes
         ptr+=n;
      }
      
      if(*ptr == ';') // séparateur, on passe au caractère suivant
      {
         ptr++;
         if(!ret) // si on avait rien lu
            ret=2; // on fait comme si on avait lu ...
      }
      
      if(*ptr == 0) // fin de ligne, OK.
         break; // sortie de la boucle

      if(ret<2) // si on a pas un label et une valeur
      {
         if(parsed_parameters)
         {
            clean_parsed_parameters(parsed_parameters, *nb_params);
            free(parsed_parameters);
            parsed_parameters=NULL;
         }
         if(err) *err=4; // label sans valeur
         goto malloc_parsed_parameters_exit;
      }
   }
   free(value);
   value=NULL;

#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   _parsed_parameters_add_to_cache(parameters_to_find, *nb_params, parameters_string, parsed_parameters);
#endif
   return parsed_parameters;

malloc_parsed_parameters_exit:
   if(value)
   {
      free(value);
      value=NULL;
   }
   return NULL;
}


void display_parsed_parameters(parsed_parameters_t *mpp, int nb)
{
   DEBUG_SECTION {
      if(mpp)
      {
         for(int i=0;i<nb;i++)
         {
            printf("%s = ",mpp[i].label);
            switch((int)(mpp[i].type))
            {
               case 1:
                  printf("%d (I)\n",mpp[i].value.i);
                  break;
               case 2:
                  printf("%ld (L)\n",mpp[i].value.l);
                  break;
               case 3:
                  printf("%f (F)\n",mpp[i].value.f);
                  break;
               case 4:
                  printf("%s (S)\n",mpp[i].value.s);
                  break;
               default:
                  printf("\n");
                  break;
            }
         }
      }
   }
}

#ifdef PARAMETERS_UTILS_MODULE_R7
// gcc -std=c99 -std=gnu99 parameters_utils.c mea_string_utils.c -lpthread -DPARAMETERS_UTILS_MODULE_R7
#include <sys/time.h>

double millis()
{
   struct timeval te;
   gettimeofday(&te, NULL);

   double milliseconds = (double)te.tv_sec*1000.0 + (double)te.tv_usec/1000.0;

   return milliseconds;
}

char *params_str[]={"S:P1","S:P2",NULL};
#define PARAMS_1 0
#define PARAMS_2 1

int main(int argc, char *argv[])
{
   int nb_params=0;
   char *test_str="P2=10;P1=11";
   int err;

   parsed_parameters_t *params1,*params2;

   parsed_parameters_init();

   params1=malloc_parsed_parameters(test_str, params_str, &nb_params, &err, 0);   
   for(int i=0;i<nb_params;i++)
      fprintf(stderr,"%s = %s\n",params1[i].label, params1[i].value.s);
   free_parsed_parameters(&params1);

   double t0=millis();
   for(int i=0;i<1000000;i++)
   {
      params2=malloc_parsed_parameters(test_str, params_str, &nb_params, &err, 0);   
      free_parsed_parameters(&params2);
   }
   fprintf(stderr, "%5.2f ms\n",millis()-t0);

   parsed_parameters_clean_all();
}

#endif
