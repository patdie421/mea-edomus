//
//  init.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 18/08/13.
//
//

#ifndef mea_edomus_init_h
#define mea_edomus_init_h

#include <inttypes.h>

int checkInstallationPaths(char *base_path);
int checkParamsDb(char *sqlite3_db_param_path, int16_t *cause);

int initMeaEdomus(char *sqlite3_db_param_path);

#endif
