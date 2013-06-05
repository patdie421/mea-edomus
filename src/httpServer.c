//
//  ihm.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "httpServer.h"
#include "mongoose.h"

// Mongoose web server.
const char* options[] = {
   "document_root", "/Data/www",
   "listening_ports", "8083",
   "index_files", "index.html,index.php",
   "cgi_interpreter", "/Data/bin/php-cgi",
   "cgi_pattern", "**.php$",
   "num_threads", "5",
   NULL
};

char *content = "<html><head><title>Test PHP</title></head><body><p>Bonjour le monde</p></body></html>";

struct mg_context* g_mongooseContext = 0;

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


int16_t start_ihm()
{
   struct mg_callbacks callbacks;

   memset(&callbacks,0,sizeof(struct mg_callbacks));

//   callbacks.begin_request = begin_request_handler;
   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options);
   if (g_mongooseContext == NULL)
      return 0;
   else
      return 1;
}

