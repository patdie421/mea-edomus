//
//  parametersparser.c
//
//  Created by Patrice DIETSCH on 24/10/12.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "macros.h"
#include "debug.h"
#include "memory.h"
#include "parameters_mgr.h"


int16_t is_in_assocs_list(struct assoc_s *assocs_list, int val1, int val2)
{
   for(int i=0;assocs_list[i].val1!=-1;i++)
      if(assocs_list[i].val1==val1 && assocs_list[i].val2==val2)
         return 1;
   return 0;
}


char *getToken(char *str)
{
   char *end;
   
   // suppression des blancs avant
   while(isspace(*str))
      str++;
   
   if(*str!=0) // si la chaine n'est pas vide
   {
      end=str+strlen(str) - 1;
      
      // suppression des blancs après
      while(end > str && isspace(*end))
         end--;
      *(end+1)=0;
   }
   
   // vérification des caractères
   for(int i=0;str[i];i++)
   {
      if(!(isalpha(str[i]) || isdigit(str[i]) || str[i]=='_' || str[i]=='.' || str[i]==':'))
         return 0;
      
      str[i]=toupper(str[i]);
   }
   
   return str;
}


void free_parsed_parameters(parsed_parameter_t *params, int nb_params)
{
   for(int i=0;i<nb_params;i++)
      if(params[i].type==STRING)
         if(params[i].value.s)
            free(params[i].value.s);
}
       
       
parsed_parameter_t *malloc_parsed_parameters(char *parameters_string, char *parameters_to_find[], int *nb_params, int *err, int value_to_upper)
{
   char *ptr = parameters_string;
   char label[21];
   char *label_token;
   char value[256];
   char *value_token;
   int n;
   int ret;
   parsed_parameter_t *parsed_parameters;
   
   if(err) *err=0;
   // nombre de label dans la liste
   for(*nb_params=0;parameters_to_find[*nb_params];(*nb_params)++);
   
   // préparation du tableau à retourner
   if(!*nb_params)
   {
      // afficher ici un message d'erreur
      return NULL;
   }
   
   parsed_parameters=malloc(sizeof(parsed_parameter_t) * *nb_params);
   if(!parsed_parameters)
   {
      VERBOSE(1) fprintf(stderr,"%s (%s) : malloc error\n",ERROR_STR,__func__);
      if(err) *err=1; // erreur système, voir errno;
      return NULL;
   }
   
   memset(parsed_parameters,0,sizeof(parsed_parameter_t) * *nb_params);
   
   while(1)
   {
      ret=sscanf(ptr,"%20[^=;]=%256[^;]%n",label,value,&n);
      if(ret==EOF) // plus rien a lire
         break;
      
      if(ret==2)
      {
         // ici on traite les données lues
         label_token=getToken(label);
         if(value_to_upper)
            value_token=getToken(value);
         else
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
                        VERBOSE(1) fprintf(stderr,"%s (%s) : malloc error\n", ERROR_STR,__func__);
                        if(parsed_parameters)
                        {
                           free_parsed_parameters(parsed_parameters, *nb_params);
                           free(parsed_parameters);
                        }
                        if(err) *err=1; // erreur système, voir errno
                        return NULL;
                     }
                     strcpy(parsed_parameters[i].value.s,value_token);
                     break;
                     
                  default:
                     break;
               }
               if(r!=strlen(value_token))
               {
                  if(parsed_parameters)
                  {
                     free_parsed_parameters(parsed_parameters, *nb_params);
                     free(parsed_parameters);
                     parsed_parameters=NULL;
                  }
                  if(err) *err=2; // erreur de syntaxe;
                  return NULL;
               }
               break;
            }
         }
         if(!trouvee)
         {
            if(parsed_parameters)
            {
               free_parsed_parameters(parsed_parameters, *nb_params);
               free(parsed_parameters);
            }
            if(err) *err=3; // label inconnu
            return NULL;
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
      {
         break; // sortie de la boucle
      }
      
      if(ret<2) // si on a pas un label et une valeur
      {
         if(parsed_parameters)
         {
            free_parsed_parameters(parsed_parameters, *nb_params);
            free(parsed_parameters);
            parsed_parameters=NULL;
         }
         if(err) *err=4; // label sans valeur
         return NULL;
      }
   }
   return parsed_parameters;
}


int nb;

/*
parsed_parameter_t *mpp=malloc_parsed_parameters("aA=1; b = 00021;C=10;;X=1;A_a =20.0;", tofind, &nb);
*/
void display_parsed_parameters(parsed_parameter_t *mpp, int nb)
{
   DEBUG_SECTION {
   if(mpp)
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
