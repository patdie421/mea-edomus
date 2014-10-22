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
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // close
#include <netdb.h> // gethostbyname
#include <sys/stat.h>
#include <signal.h>

#include "globals.h"
#include "debug.h"
#include "error.h"
#include "guiServer.h"
#include "mongoose.h"
#include "string_utils.h"
#include "queue.h"
#include "xPLServer.h"
#include "tokens.h"
#include "init.h"
#include "sockets_utils.h"

#include "processManager.h"

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

int _httpServer_monitoring_id=-1;

pid_t pid_nodejs=0;

char php_sessions_path[256];

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


int gethttp(char *server, int port, char *url, char *response, int l_response)
{
   int sockfd;                   // descripteur de socket

   // creation de la requete
   char requete[1000]="GET ";
   strcat(requete, url);
   strcat(requete, " HTTP/1.1\r\n");
   strcat(requete, "Host: ");
   strcat(requete, server);
   strcat(requete, "\r\n");
   strcat(requete, "Connection: close\r\n");
   strcat(requete, "\r\n");

   if(mea_socket_connect(&sockfd, server, port)<0)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : mea_socket_connect - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
 
   if(mea_socket_send(&sockfd, requete)<0)
   {
      close(sockfd);
      return -1;
   }

   // reception de la reponse HTTP
   int n=recv(sockfd, response, l_response, 0);
   if(n == -1)
   {
      VERBOSE(2) {
         fprintf (stderr, "%s (%s) : recv - ", ERROR_STR,__func__);
         perror("");
      }
      close(sockfd);
      return -1;
   }
   
   //affichage de la reponse HTTP
   if(n)
   {
      VERBOSE(9) {
         fprintf (stderr, "%s  (%s) : recv - ", INFO_STR,__func__);
         fprintf(stderr, "%s\n",response);
      }
   }
   // fermeture de la socket
   close(sockfd);
 
   return 0;
}


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


void httpResponse(struct mg_connection *conn, char *response)
{
   mg_printf(conn,
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: %d\r\n" // Always set Content-Length
      "Cache-Control: no-cache, no-store, must-revalidate\r\n"
      "Pragma: no-cache\r\n"
      "Expires: -1\r\n"
      "Vary: *\r\n"
      "\r\n"
      "%s\r\n",
      (int)strlen(response)+2, response);
}


void httpErrno(struct mg_connection *conn, int n)
{
   char errno_str[20];
   sprintf(errno_str, "{ errno : %d }",n);
   httpResponse(conn, errno_str);
}


int _findPHPVar(char *phpserial, char *var, int *type, char *value, int l_value)
{
   char *_toFind;
   char _type[2];
   char _v1[17];
   char _v2[256];

   _toFind=(char *)malloc(strlen(var)+2);
   if(!_toFind)
      return -1;
   sprintf(_toFind,"%s|",var);
   char *str_ptr=strstr(phpserial,_toFind)+strlen(_toFind);
   free(_toFind);
   
   if(!str_ptr)
      return -1;

   int n=sscanf(str_ptr,"%1[^:;]:%16[^:;]:%255[^:;]",_type,_v1,_v2);
   if(n<2)
      return -1;
   
   *type=0;
   if(strcmp(_type,"i")==0)
   {
      *type=1;
      strncpy(value,_v1,l_value);
   }
   else if(strcmp(_type,"s")==0)
   {
      *type=2;
      int l=atoi(_v1)+1;
      if(l > l_value)
         l=l_value;
   
      strncpy(value,&_v2[1],l);
   }
   else
      return -1;

   return 0;
}


int _phpsessid_check_admin(char *phpsessid,char *sessions_files_path)
{
   char session_file[80];
   snprintf(session_file, sizeof(session_file), "%s/sess_%s",sessions_files_path, phpsessid);
   
   char *buff;
   struct stat st;

   if (stat(session_file, &st) == 0)
   {
      buff=(char *)malloc(st.st_size+1);
      if(!buff)
         return -1;
   }
   else
      return -1;

   FILE *fd;
   fd=fopen(session_file, "r");
   if(fd)
   {
      int n = fread(buff, 1, st.st_size, fd);
      fclose(fd);
      if(!n)
         return -1;
      buff[n]='\0'; // pour eviter les problèmes ...
      
      int logged_in;
      int admin;

      int type;
      char value[81];
      if(_findPHPVar(buff, "logged_in", &type, value, 80)==0)
         logged_in=atoi(value);
      else
         return -1;
      if(_findPHPVar(buff, "profil", &type, value, 80)==0)
         admin=atoi(value);
      else
         return -1;
      
      if(logged_in==1 && admin==1)
         return 0;
      else
         return -1;
   }
   else
   {
      VERBOSE(5) {
         fprintf(stderr, "%s (%s) : cannot read %s file - ",ERROR_STR,__func__,session_file);
         perror("");
      }
      return -1;
   }
}


#define MAX_BUFFER_SIZE 80
#define MAX_TOKEN 20
static int begin_request_handler(struct mg_connection *conn)
{
   uint32_t id = 0;
   const struct mg_request_info *request_info = mg_get_request_info(conn);
   char phpsessid[80];

   char *tokens[MAX_TOKEN]; // 10 tokens maximum
   char buffer[MAX_BUFFER_SIZE];

   if(mea_strncmplower("/CMD/ping.php",(char *)request_info->uri,13)==0)
   {
       process_heartbeat(_httpServer_monitoring_id);
       httpErrno(conn, 0);
       return 1;
   }
   else if(mea_strncmplower("/CMD/startstop.php",(char *)request_info->uri,18)==0)
   {
      // on récupère les cookies
      const char *cookie = mg_get_header(conn, "Cookie");

      // on récupère le session_id PHP
      if(mg_get_cookie(cookie, "PHPSESSID", phpsessid, sizeof(phpsessid))>0)
      {
         if(_phpsessid_check_admin(phpsessid, php_sessions_path)<0)
         {
            httpErrno(conn, 2); // pas habilité
            return 1;
         }
      }
         // lire les paramètres
      if(strcmp(request_info->request_method, "GET") != 0)
      {
         httpErrno(conn, 3); // pas la bonne méthode
         return 1;
      }

      if(request_info->query_string)
      {
         char process_id_str[10]; 
         int process_id=-1;
         char command[10]; // "start" ou "stop"
         int ret=0;

         ret=mg_get_var(request_info->query_string, strlen(request_info->query_string), "process", process_id_str, sizeof(process_id_str));
         if(ret)
         {
            process_id=atoi(process_id_str); // à remplacer par atol et test si number
         }
         else
         {
            httpErrno(conn, 4);
         }
         ret=mg_get_var(request_info->query_string, strlen(request_info->query_string), "cmnd", command, sizeof(command));
         if(ret)
         {
            if(mea_strncmplower("start", command, strlen(command))==0)
            {
               ret=process_start(process_id);
               if(!ret)
                  httpErrno(conn, 0);
               else
                  httpErrno(conn, 7);
            }
            else if (mea_strncmplower("stop", command, strlen(command))==0)
            {
               ret=process_stop(process_id);
               if(!ret)
                  httpErrno(conn, 0);
               else
                  httpErrno(conn, 7);
            }
            else
            {
               httpErrno(conn, 6);
            }
         }
      }
      else
         httpErrno(conn, 5);

      return 1;
   }
   else if(mea_strncmplower("/API/",(char *)request_info->uri,5)==0)
   {
      // traiter ici l'URL
      char reponse[255];
      reponse[0]=0;
      
      if(strlen(&(request_info->uri[5]))>(sizeof(buffer)-1))
         return 0;
      
      strcpy(buffer, &(request_info->uri[5]));
      int ret=mea_strsplit(buffer, '/', tokens, MAX_TOKEN);
      if(ret>2)
      {
         if(mea_strcmplower("XPL-INTERNAL",tokens[0])==0)
         {
            xPL_MessagePtr msg = mea_createReceivedMessage(xPL_MESSAGE_COMMAND);
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

               id=mea_getXplRequestId();
               sprintf(requestId,"%08d",id);
               xPL_setSource(msg, "mea", "internal", requestId); // 00000000 = message sans réponse attendue

               waitResp=TRUE;
            }
            else
            {
               xPL_releaseMessage(msg);
               return 0;
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

//                  DEBUG_SECTION fprintf(stderr, "key=%s value=%s\n",keyval[0],keyval[1]);
               }
               else
               {
                  xPL_releaseMessage(msg);
                  return 0;
               }
            }

            mea_sendXPLMessage(msg);
            xPL_releaseMessage(msg);
            
            if(waitResp==FALSE)
            {
               strcpy(reponse,"{ errno : 0 }");
            }
            else
            {
//               DEBUG_SECTION fprintf(stderr,"En attente de reponse\n");
               xPL_MessagePtr respMsg=mea_readXPLResponse(id);
               if(respMsg)
               {
                  xPL_NameValueListPtr body = xPL_getMessageBody(respMsg);
                  int n = xPL_getNamedValueCount(body);
                  strcpy(reponse,"{ ");
                  for (int i=0; i<n; i++)
                  {
                     xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
                     strcat(reponse, keyValuePtr->itemName);
                     strcat(reponse, " : ");
                     strcat(reponse, keyValuePtr->itemValue);
                     strcat(reponse,", "); // on rajoute une virgule
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
            httpResponse(conn, reponse);
            return 1;
         }
         else
            return 0;
      }
      else
         return 0;
   }
   // pas une api
   return 0;
}


void _stop_nodejs()
{
   int status;

   if(pid_nodejs>1)
   {
      kill(pid_nodejs, SIGKILL);
      wait(&status);
      pid_nodejs=-1;
   }
}


pid_t _start_nodejs(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *phpsession_path)
{
   pid_t nodejs_pid = -1;
   nodejs_pid = fork();

   if (nodejs_pid == 0) // child
   {
      // Code only executed by child process
      char str_port_socketio[80];
      char str_port_socketdata[80];
      char str_phpsession_path[80];
      
      char *params[6];

      sprintf(str_port_socketio,"--iosocketport=%d",port_socketio);
      sprintf(str_port_socketdata,"--dataport=%d",port_socketdata);
      sprintf(str_phpsession_path,"--phpsession_path=%s",phpsession_path);

      params[0]="mea-edomus[nodejs]";
      params[1]=eventServer_path;
      params[2]=str_port_socketio;
      params[3]=str_port_socketdata;
      params[4]=str_phpsession_path;
      params[5]=0;

      execvp(nodejs_path, params);

      perror("");

      exit(1);
   }
   else if (nodejs_pid < 0)
   { // failed to fork
      perror("");
      return -1;
   }
   // Code only executed by parent process
   return nodejs_pid;
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
   callbacks.begin_request = begin_request_handler;
//   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options);
   if (g_mongooseContext == NULL)
      return ERROR;
   else
      return NOERROR;
}


int stop_guiServer(int my_id, void *data)
{
   mg_stop(g_mongooseContext);
      
   _stop_nodejs();

   return 0;
}


//int start_httpServer(char **params_list, queue_t *interfaces)
int start_guiServer(int my_id, void *data)
{
   struct httpServerData_s *httpServerData = (struct httpServerData_s *)data;
   int nodejs_started=0;
   pid_t pid;
   
   if(httpServerData->params_list[NODEJSIOSOCKET_PORT] == NULL ||
      httpServerData->params_list[NODEJSDATA_PORT] == NULL     ||
      httpServerData->params_list[NODEJS_PATH] == NULL         ||
      httpServerData->params_list[LOG_PATH] == NULL)
   {
      VERBOSE(3) {
         fprintf (stderr, "%s (%s) : nodejs - parameters error ...",ERROR_STR,__func__);
      }
      goto httpServer_only;
   }

   int socketio_port = atoi(httpServerData->params_list[NODEJSIOSOCKET_PORT]);
   int socketdata_port = atoi(httpServerData->params_list[NODEJSDATA_PORT]);
   char *nodejs_path = httpServerData->params_list[NODEJS_PATH];
   char *phpsession_path = httpServerData->params_list[PHPSESSIONS_PATH];
   char serverjs_path[256];

   int n=snprintf(serverjs_path, sizeof(serverjs_path), "%s/nodeJS/server/server", httpServerData->params_list[GUI_PATH]);
   if(n<0 || n==sizeof(serverjs_path))
   {
      VERBOSE(3) {
         fprintf (stderr, "%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      goto httpServer_only;
   }

   pid=_start_nodejs(nodejs_path, serverjs_path, socketio_port, socketdata_port, phpsession_path);
   if(!pid)
   {
      VERBOSE(3) {
         fprintf (stderr, "%s (%s) : can't start nodejs",ERROR_STR,__func__);
      }
      goto httpServer_only;
   }
   pid_nodejs=pid;
   nodejs_started=1;
   
httpServer_only:
   if(!nodejs_started)
   {
      VERBOSE(1) {
         fprintf (stderr, "%s (%s) : nodejs not started, try httpServer only",ERROR_STR,__func__);
      }
   }
   
   char *phpcgibin=NULL;
   if(httpServerData->params_list[PHPCGI_PATH] &&
      httpServerData->params_list[PHPINI_PATH] &&
      httpServerData->params_list[GUI_PATH]    &&
      httpServerData->params_list[SQLITE3_DB_PARAM_PATH])
   {
      phpcgibin=(char *)malloc(strlen(httpServerData->params_list[PHPCGI_PATH])+10); // 9 = strlen("/cgi-bin") + 1
      sprintf(phpcgibin, "%s/php-cgi",httpServerData->params_list[PHPCGI_PATH]);

      long guiport;
      if(httpServerData->params_list[GUIPORT][0])
      {
         char *end;
         guiport=strtol(httpServerData->params_list[GUIPORT],&end,10);
         if(*end!=0 || errno==ERANGE)
         {
            VERBOSE(9) fprintf(stderr,"%s (%s) : GUI port (%s), not a number, 8083 will be used.\n",INFO_STR,__func__,httpServerData->params_list[GUIPORT]);
            guiport=8083;
         }
      }
      else
      {
         VERBOSE(9) fprintf(stderr,"%s (%s) : can't get GUI port, 8083 will be used.\n",INFO_STR,__func__);
         guiport=8083;
      }
      
      strncpy(php_sessions_path, httpServerData->params_list[PHPSESSIONS_PATH], sizeof(php_sessions_path)-1);

      if(create_configs_php(httpServerData->params_list[GUI_PATH],
                            httpServerData->params_list[SQLITE3_DB_PARAM_PATH],
                            httpServerData->params_list[LOG_PATH],
                            httpServerData->params_list[PHPSESSIONS_PATH],
                            atoi(httpServerData->params_list[NODEJSIOSOCKET_PORT]))==0)
      {
         httpServer(guiport, httpServerData->params_list[GUI_PATH], phpcgibin, httpServerData->params_list[PHPINI_PATH]);
         _httpServer_monitoring_id=my_id;

         return 0;
      }
      else
      {
         VERBOSE(3) fprintf(stderr,"%s (%s) : can't start GUI Server (can't create configs.php).\n",ERROR_STR,__func__);
         // on continu sans ihm
      }
      free(phpcgibin);
   }
   else
   {
      VERBOSE(3) fprintf(stderr,"%s (%s) : can't start GUI Server (parameters errors).\n",ERROR_STR,__func__);
   }

   return -1;
}
  


