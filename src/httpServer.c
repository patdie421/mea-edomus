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

#include "debug.h"
#include "error.h"
#include "httpServer.h"
#include "mongoose.h"

//
// pour compilation php-cgi :
//
// ./configure --prefix=/usr/local/php --disable-all --enable-session --enable-pdo --with-sqlite3 --with-pdo-sqlite --enable-json
//
// supprimer aussi "-g" dans Makefile pour une taille minimum de l'executable
//

const char *str_document_root="document_root";
const char *str_listening_ports="listening_ports";
const char *str_index_files="index_files";
const char *str_cgi_interpreter="cgi_interpreter";
const char *str_cgi_environment="cgi_environment";
const char *str_cgi_pattern="cgi_pattern";
const char *str_num_threads="num_threads";

char *val_document_root=NULL;
char *val_listening_ports=NULL;
char *val_index_files=NULL;
char *val_cgi_interpreter=NULL;
char *val_cgi_environment=NULL;
char *val_cgi_pattern=NULL;
char *val_num_threads=NULL;

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

const char *options[15];

struct mg_context* g_mongooseContext = 0;


// fichier "mémoire"
static const char *open_file_handler(const struct mg_connection *conn, const char *path, size_t *size)
{
   // ne peut pas marcher avec les cgi car le cgi-php va chercher sur disque le fichier ... utile néanmoins pour fournir des pages statiques ...
   char *content = "<html><head><title>Test PHP</title></head><body><p>Bonjour le monde</p></body></html>";

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
   
   if(!home || !php_cgi || !php_ini_path)
      return ERROR;
   
   val_listening_ports=(char *)malloc(6);
   sprintf(val_listening_ports,"%d",port);
   
   val_document_root=malloc(strlen(home)+1);
   strcpy(val_document_root,home);
   
   val_cgi_interpreter=malloc(strlen(php_cgi)+1);
   strcpy(val_cgi_interpreter,php_cgi);
   
   val_cgi_environment=malloc(6+strlen(php_ini_path)+1);
   sprintf(val_cgi_environment,"PHPRC=%s", php_ini_path);
   
   options[ 0]=str_document_root;
   options[ 1]=(const char *)val_document_root;
   options[ 2]=str_listening_ports;
   options[ 3]=(const char *)val_listening_ports;
   options[ 4]=str_index_files;
   options[ 5]="index.php";
   options[ 6]=str_cgi_interpreter;
   options[ 7]=(const char *)val_cgi_interpreter;
   options[ 8]=str_cgi_environment;
   options[ 9]=(const char *)val_cgi_environment;
   options[10]=str_cgi_pattern;
   options[11]="**.php$";
   options[12]=str_num_threads;
   options[13]="5";
   options[14]=NULL;
   
   struct mg_callbacks callbacks;
   memset(&callbacks,0,sizeof(struct mg_callbacks));
//   callbacks.begin_request = begin_request_handler;
   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options);
   if (g_mongooseContext == NULL)
      return ERROR;
   else
      return NOERROR;
}
