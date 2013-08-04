//
//  httpServer.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "error.h"
#include "httpServer.h"
#include "mongoose.h"

//
// pour compilation php-cgi :
//
// ./configure --prefix=/usr/local/php --disable-all --enable-session --enable-pdo --with-sqlite3 --with-pdo-sqlite --enable-json
//
// supprimer aussi "-g" dans Makefile resultant pour taille minimum de l'executable
//

const char *str_document_root="document_root";
const char *str_listening_ports="listening_ports";
const char *str_index_files="index_files";
const char *str_cgi_interpreter="cgi_interpreter";
const char *str_cgi_environment="cgi_environment";
const char *str_cgi_pattern="cgi_pattern";
const char *str_num_threads="num_threads";

char *val_document_root;
char *val_listening_ports;
char *val_index_files;
char *val_cgi_interpreter;
char *val_cgi_environment;
char *val_cgi_pattern;
char *val_num_threads;

/*
const char* options[] = {
   "document_root",   "/Data/www",
   "listening_ports", "8083",
   "index_files",     "index.php",
   "cgi_interpreter", "/Data/bin/php-cgi",
   "cgi_environment", "PHPRC=/Data/php",
   "cgi_pattern",     "**.php$",
   "num_threads",     "5",
   NULL
};
*/

// "/Data/mea-edomus/var/db/params.db";

const char *options_[15];

char *content = "<html><head><title>Test PHP</title></head><body><p>Bonjour le monde</p></body></html>";

struct mg_context* g_mongooseContext = 0;


void make_config_default_php(char *params_db_fullname)
{
   printf("<?php");
   printf("$config['atk']['base_path']='./atk4/';");
   printf("$config['dsn'] = array('sqlite:%s,');",params_db_fullname);
   printf("$config['url_postfix']='';");
   printf("$config['url_prefix']='?page=';");
}


// fichier "mémoire"
static const char *open_file_handler(const struct mg_connection *conn, const char *path, size_t *size)
// ne peut pas marcher avec les cgi car le cgi-php va chercher sur disque le fichier ... utile néanmoins pour fournir des pages statiques ...
{
   if (!strcmp(path, "/Data/www/test.html"))
   {
      *size = strlen(content);
      return content;
   }
   return NULL;
}


static int begin_request_handler(struct mg_connection *conn)
{
// pour diffuser des pages dynamique
   
//   const struct mg_request_info *request_info = mg_get_request_info(conn);

   return 0;
}


mea_error_t httpServer(uint16_t port, char *home, char *php_cgi, char *php_ini_path)
{
   if(port==0)
      port=8083;
   val_listening_ports=(char *)malloc(6);
   sprintf(val_listening_ports,"%d",port);
   
   if(home==NULL)
      home="/usr/local/mea-edomus/gui";
   val_document_root=malloc(strlen(home)+1);
   strcpy(val_document_root,home);
   
   if(php_cgi==NULL)
      php_cgi="/usr/local/bin";
   val_cgi_interpreter=malloc(strlen(php_cgi)+1);
   strcpy(val_cgi_interpreter,php_cgi);
   
   if(php_ini_path==NULL)
      php_ini_path="/usr/local/lib";
   val_cgi_environment=malloc(6+strlen(php_ini_path)+1);
   sprintf(val_cgi_environment,"PHPRC=%s", php_ini_path);
   
   options_[ 0]=str_document_root;
   options_[ 1]=(const char *)val_document_root;
   options_[ 2]=str_listening_ports;
   options_[ 3]=(const char *)val_listening_ports;
   options_[ 4]=str_index_files;
   options_[ 5]="index.php";
   options_[ 6]=str_cgi_interpreter;
   options_[ 7]=(const char *)val_cgi_interpreter;
   options_[ 8]=str_cgi_environment;
   options_[ 9]=(const char *)val_cgi_environment;
   options_[10]=str_cgi_pattern;
   options_[11]="**.php$";
   options_[12]=str_num_threads;
   options_[13]="5";
   options_[14]=NULL;
   
   struct mg_callbacks callbacks;
   memset(&callbacks,0,sizeof(struct mg_callbacks));
//   callbacks.begin_request = begin_request_handler;
   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options_);
   if (g_mongooseContext == NULL)
      return ERROR;
   else
      return NOERROR;
}
