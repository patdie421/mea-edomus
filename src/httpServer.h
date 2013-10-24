//
//  httpServer.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifndef mea_eDomus_httpServer_h
#define mea_eDomus_httpServer_h

#include "error.h"

mea_error_t httpServer(uint16_t port, char *home, char *php_cgi, char *php_ini_path);
int create_config_default_php(char *home, char *params_db_fullname);
int create_configs_php(char *home, char *params_db_fullname);

#endif
