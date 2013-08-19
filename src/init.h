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

int16_t checkInstallationPaths(char *base_path);
int16_t checkParamsDb(char *sqlite3_db_param_path, int16_t *cause);

int16_t initMeaEdomus(int16_t mode, char *sqlite3_db_param_path, char *base_path);

#endif
