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
#include "string_utils.h"
#include "queue.h"
#include "xPLServer.h"
#include "token_strings.h"

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
/*
   // ne peut pas marcher avec les cgi car le cgi-php va chercher sur disque le fichier ... utile néanmoins pour fournir des pages statiques ...
   char *content = "<html><head><title>Test PHP</title></head><body><p>Bonjour le monde</p></body></html>";

   int i;
   for(i=0; path[i]==val_document_root[i]; i++);

   if(strcmp(&path[i],"/test.html")==0)
   {
      *size = strlen(content);
      return content;
   }
*/
   return NULL;
}


#define MAX_BUFFER_SIZE 80
#define MAX_TOKEN 20
static int begin_request_handler(struct mg_connection *conn)
{
// API REST ...
   uint32_t id = 0;
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   char *tokens[MAX_TOKEN]; // 10 tokens maximum
   char buffer[MAX_BUFFER_SIZE];

   if(mea_strncmplower("/API/",(char *)request_info->uri,5)==0)
   {
      // traiter ici l'URL
      char reponse[255];
      reponse[0]=0;
      
      if(strlen(&(request_info->uri[5]))>(sizeof(buffer)-1))
         return NULL;
      
      strcpy(buffer, &(request_info->uri[5]));
      int ret=splitStr(buffer, '/', tokens, MAX_TOKEN);
      if(ret>2)
      {
         if(mea_strcmplower("XPL-INTERNAL",tokens[0])==0)
         {
            xPL_MessagePtr msg = xPL_createReceivedMessage(xPL_MESSAGE_COMMAND);
            xPL_setBroadcastMessage(msg, TRUE); // pas de destinataire spécifié (pas nécessaire, on sait que c'est pour nous
            int waitResp = FALSE;

            if(mea_strcmplower("ACTUATOR",tokens[1])==0)
            {
               xPL_setSchema(msg, get_token_by_id(XPL_CONTROL_ID), get_token_by_id(XPL_BASIC_ID));
               xPL_setSource(msg, "mea", "internal", "00000000"); // 00000000 = message sans réponse attendue
               waitResp=FALSE;
            }
            else if(mea_strcmplower("SENSOR",tokens[1])==0)
            {
               xPL_setSchema(msg, get_token_by_id(XPL_SENSOR_ID), get_token_by_id(XPL_REQUEST_ID));
               char requestId[9];
               id=getRequestId();
               sprintf(requestId,"%08d",id);
               xPL_setSourceInstanceID(msg, requestId);
               waitResp=TRUE;
            }
            else
            {
               xPL_releaseMessage(msg);
               return NULL;
            }
            
            // fabrication du body
            for(int i=2; i<ret; i++)
            {
               char keyval_buff[MAX_BUFFER_SIZE];
               char *keyval[2];
               strcpy(keyval_buff,tokens[i]);
               
               int ret2=mea_strsplit(keyval_buff, '_', keyval, 2);
               if(ret2==2)
               {
                  mea_strtolower(keyval[0]);
                  mea_strtolower(keyval[1]);
                  xPL_setMessageNamedValue(msg, keyval[0], keyval[1]);

                  DEBUG_SECTION fprintf(stderr, "key=%s value=%s\n",keyval[0],keyval[1]);
               }
               else
               {
                  xPL_releaseMessage(msg);
                  return NULL;
               }
            }

            sendXplMessage(msg);
            xPL_releaseMessage(msg);
            
            if(waitResp==FALSE)
            {
               strcpy(reponse,"{ errno : 0 }");
            }
            else
            {
               DEBUG_SECTION fprintf(stderr,"En attente de reponse\n");
               xPL_MessagePtr respMsg=readResponseFromQueue(id);
               if(respMsg)
               {
               xPL_NameValueListPtr body = xPL_getMessageBody(xPLMsg);
               int n = xPL_getNamedValueCount(body);
               strcpy(reponse,"{");
               for (int i=0; i<n; i++)
               {
                  xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
                  strcat(reponse, keyValuePtr->itemName);
                  strcat(reponse, " : ");
                  strcat(reponse, keyValuePtr->itemValue);
                  strcat(reponse,","); // on rajoute une virgule
               }
               strcat(reponse,"errno : 0 }");

               // réponse traitée, on libère
               xPL_releaseMessage(respMsg);
               }
               else
               {
                  strcpy(reponse,"{ errno : 1 }");
               }
            }
            mg_printf(conn,
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json\r\n" 
                      "Content-Length: %d\r\n" // Always set Content-Length
                      "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                      "Pragma: no-cache\r\n"
                      "Expires: -1\r\n"
                      "Vary: *\r\n"
                      "\r\n"
                      "%s",
                      (int)strlen(reponse), reponse);
            return "done";
         }
         else
            return NULL;
      }
      else
         return NULL;
   }
   // pas une api
   return 0;
}


mea_error_t httpServer(uint16_t port, char *home, char *php_cgi, char *php_ini_path, queue_t *interfaces)
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
   callbacks.begin_request = begin_request_handler;
//   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options);
   if (g_mongooseContext == NULL)
      return ERROR;
   else
      return NOERROR;
}
