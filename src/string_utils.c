//
//  string_utils.c
//
//  Created by Patrice DIETSCH on 08/07/2013.
//
//
#include <ctype.h>
#include <string.h>

#include "string_utils.h"


char *ltrim(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche d'une chaine.
  * \details   la chaine n'est pas modifiée. un pointeur est retourné sur le premier caractère "non blanc" de la chaine. Attention à ne pas perdre le pointeur d'origine pour les chaines allouées. Eviter 'absolument s=ltrim(s)', préférez 's_trimed=ltrim(s)'.
  * \param     s  chaine à trimer
  * \return    pointeur sur le premier caractère "non blanc" de la chaine
  */
{
    while(isspace(*s)) s++;
    return s;
}


char *rtrim(char *s)
 /**
  * \brief     trim (suppression de blanc) à droite d'une chaine.
  * \details   la chaine est modifiée. un '\0' est positionné à la place de chaque caractère "blanc" en fin de chaine. s=rtrim() est équivalant à rtrim(s).
  * \param     s  chaine à trimer
  * \return    pointeur sur le début de la chaine.
  */
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}


char *trim(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche et à droite d'une chaine.
  * \details   la chaine est modifiée, un '\0' est positionné à la place de chaque caractère "blanc" en fin de chaine. s=rtrim() est équivalant à rtrim(s) et un pointeur est retourné sur le premier caractère "non blanc" de la chaine. Voir aussi ltrim et rtrim.
  * \param     s  chaine à trimer
  * \return    pointeur sur le début de la chaine.
  */
{
    return rtrim(ltrim(s)); 
}

void strToUpper(char *str)
/**
 * \brief     convertit tous les caractères d'une chaine en majuscule
 * \details   La chaine en paramètre est modifiée : les minuscules sont remplacées par des majuscules.
 * \param     str  chaine à modifier.
 * \return    pas de retour.
 */
{
   for(uint16_t i=0;i<strlen(str);i++)
	   str[i]=toupper(str[i]);
}

int16_t strcmplower(char *str1, char *str2)
/**
 * \brief     comparaison de deux chaines sur la base de "caractères en mimuscules"
 * \details   chaque caractère des deux chaines est converti en minuscule avant de les comparer
 * \param     str1   premiere chaine à comparer.
 * \param     str2   deuxième chaine à comparer.
 * \return    0 chaines égales, 1 chaines différentes

 */
{
   int i;
   for(i=0;str1[i];i++)
      if(tolower(str1[i])!=tolower(str2[i]))
         return 1;
   if(str1[i]!=str2[i])
      return 1;
   return 0;
}


