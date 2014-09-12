//
//  string_utils.h
//
//  Created by Patrice DIETSCH on 08/07/2013.
//
//

#ifndef __string_utils_h
#define __string_utils_h

#include <inttypes.h>

void mea_strtoupper(char *str);
void mea_strtolower(char *str);
int16_t mea_strcmplower(char *str1, char *str2);
int16_t mea_strncmplower(char *str1, char *str2, int n);
char *mea_strltrim(char *s);
char *mea_strrtrim(char *s);
char *mea_strtrim(char *s);
int16_t mea_strsplit(char str[], char separator, char *tokens[], char l_tokens);

#endif

