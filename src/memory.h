//
//  memory.h
//
//  Created by Patrice DIETSCH on 17/09/12.
//
//

#ifndef __memory_h
#define __memory_h

#define MEA_FREE(x) if((x)) { free(x); x=NULL; }

char *mea_string_free_malloc_and_copy(char **org_str, char *str,int v);
char *mea_string_malloc_and_copy(char *str,int v);
#endif
