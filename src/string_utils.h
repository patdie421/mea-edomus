//
//  string_utils.h
//
//  Created by Patrice DIETSCH on 08/07/2013.
//
//

#ifndef __string_utils_h
#define __string_utils_h

#include <inttypes.h>

void strToUpper(char *str);
void strToLower(char *str);
int16_t strcmplower(char *str1, char *str2);
int16_t strncmplower(char *str1, char *str2, int n);
char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);
int splitStr(char str[], char separator, char *tokens[], char l_tokens);

#endif

