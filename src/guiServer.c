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
#include <sys/wait.h>

#include "globals.h"
#include "consts.h"
#include "mea_verbose.h"
#include "guiServer.h"
#include "mongoose.h"
#include "mea_string_utils.h"
#include "mea_queue.h"
#include "xPLServer.h"
#include "tokens.h"
#include "init.h"
#include "mea_sockets_utils.h"
#include "cJSON.h"

#include "processManager.h"
#include "automatorServer.h"

#include "notify.h"
#include "nodejsServer.h"
#include "automator.h"

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


// Variable globale privée
int  _httpServer_monitoring_id=-1;
char _php_sessions_path[256];

/* old nodejs
pid_t _pid_nodejs=0;
int  _socketio_port=0;
int  _socketdata_port=0;
*/

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

int httpin_indicator=0; // nombre de demandes recues
int httpout_indicator=0; // nombre de réponses émises

const char *options[15];
struct mg_context* g_mongooseContext = 0;

/* old nodejs
int get_socketio_port()
{
   return _socketio_port;
}
*/


int gethttp(char *server, int port, char *url, char *response, int l_response)
{
   int sockfd; // descripteur de socket
   
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
         mea_log_printf("%s (%s) : mea_socket_connect - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   if(mea_socket_send(&sockfd, requete, strlen(requete))<0)
   {
      close(sockfd);
      return -1;
   }
   
   // reception de la reponse HTTP
   int n=recv(sockfd, response, l_response, 0);
   if(n == -1)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : recv - ", ERROR_STR,__func__);
         perror("");
      }
      close(sockfd);
      return -1;
   }
   
   // fermeture de la socket
   close(sockfd);
   
   return 0;
}


// fichier "mémoire"
static const char *_open_file_handler(const struct mg_connection *conn, const char *path, size_t *size)
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


void _httpResponse(struct mg_connection *conn, char *response)
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

   process_update_indicator(_httpServer_monitoring_id, "HTTPOUT", ++httpout_indicator);
}


void _httpErrno(struct mg_connection *conn, int n, char *msg)
{
   char errno_str[100];
   char *iserror;

   if(n!=0)
      iserror="true";
   else
      iserror="false";
      
   if(msg)
      snprintf(errno_str, sizeof(errno_str)-1, "{ \"iserror\" : %s, \"errno\" : %d, \"errMsg\" : \"%s\" }",iserror, n, msg);
   else
      snprintf(errno_str, sizeof(errno_str)-1, "{ \"iserror\" : %s, \"errno\" : %d }", iserror, n);
   
   _httpResponse(conn, errno_str);
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
   _toFind=NULL;
   
   if(!str_ptr)
      return -1;
   
   int n=sscanf(str_ptr,"%1[^:;]:%16[^:;]:%255[^:;]",_type,_v1,_v2);
   if(n<2)
      return -1;
   
   *type=0;
   if(strcmp(_type,"i")==0)
   {
      *type=1;
      strncpy(value,_v1,l_value-1);
   }
   else if(strcmp(_type,"s")==0)
   {
      *type=2;
      int l=atoi(_v1)+1;
      if(l > l_value)
         l=l_value;
      
      strncpy(value,&_v2[1],l-1);
   }
   else
      return -1;
   
   return 0;
}


int _phpsessid_check_loggedin_and_admin(char *phpsessid,char *sessions_files_path)
{
   int ret=0;
   char session_file[256];
   
   int n=snprintf(session_file, sizeof(session_file), "%s/sess_%s", sessions_files_path, phpsessid);
   if(n<0 || n==sizeof(session_file))
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   char *buff=NULL;
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
      {
         ret=-1;
         goto _phpsessid_check_loggedin_and_admin_clean_exit;
      }
      buff[n]='\0'; // pour eviter les problèmes ...
      
      int logged_in;
      int admin;
      
      int type;
      char value[81];
      if(_findPHPVar(buff, "logged_in", &type, value, 80)==0)
         logged_in=atoi(value);
      else
      {
         ret=-1;
         goto _phpsessid_check_loggedin_and_admin_clean_exit;
      }
      if(_findPHPVar(buff, "profil", &type, value, 80)==0)
         admin=atoi(value);
      else
      {
         ret=-1;
         goto _phpsessid_check_loggedin_and_admin_clean_exit;
      }
      
      if(logged_in==1 && admin==1)
         ret=1;
      else
      {
         if(logged_in==1)
            ret=0;
         else
            ret=-1;
      }
   }
   else
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : cannot read %s file - ",ERROR_STR,__func__,session_file);
         perror("");
      }
      ret=-1;
   }
_phpsessid_check_loggedin_and_admin_clean_exit:
   if(buff)
   {
      free(buff);
      buff=NULL;
   }

   return ret;
}


#define MAX_BUFFER_SIZE 80
#define MAX_TOKEN 20


int gui_automator(struct mg_connection *conn, char *phpsessid, char *_php_sessions_path)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   if(!phpsessid[0] || _phpsessid_check_loggedin_and_admin(phpsessid, _php_sessions_path)<0)
   {
      _httpErrno(conn, 2, NULL); // pas habilité
      return 1;
   }

   if(strcmp(request_info->request_method, "POST") != 0)
   {
      _httpErrno(conn, 3, NULL); // pas la bonne méthode
      return 1;
   }

   const char *_l = mg_get_header(conn, "Content-Length");
   if(_l)
   {
      int l = atoi(_l);
      char *post = (char *)alloca(l+1);
      if(post)
      { 
         char *cmnd=(char *)alloca(l+1);

         mg_read(conn, post, l);
         post[l]=0;
         fprintf(stderr,"%s\n",post);
         if(mg_get_var((const char *)post, l, "cmnd", cmnd, l)<0)
         {
            _httpErrno(conn, 40, NULL); // erreur interne
            return 1;
         }
         else
         {
            if(strcmp(cmnd,"sendallinputs")==0)
            {
               automatorServer_send_all_inputs();
            }
            else
            {
               _httpErrno(conn, 20, NULL); // bad command
               return 1;
            }
         }
      }

      _httpErrno(conn, 0, NULL);
      return 1;
   }
}


int gui_xplsend(struct mg_connection *conn, char *phpsessid, char *_php_sessions_path)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   if(!phpsessid[0] || _phpsessid_check_loggedin_and_admin(phpsessid, _php_sessions_path)<0)
   {
      _httpErrno(conn, 2, NULL); // pas habilité
      return 1;
   }

   if(strcmp(request_info->request_method, "POST") != 0)
   {
      _httpErrno(conn, 3, NULL); // pas la bonne méthode
      return 1;
   }

   const char *_l = mg_get_header(conn, "Content-Length");
   if(_l)
   {
      int l = atoi(_l);
      char *post = (char *)alloca(l+1);
      if(post)
      {
         char *xplmsg=(char *)alloca(l+1);

         mg_read(conn, post, l);
         post[l]=0;

         if(mg_get_var((const char *)post, l, "msg", xplmsg, l)<0)
         {
            _httpErrno(conn, 40, NULL); // erreur interne
            return 1;
         }

         cJSON *json_xplmsg = cJSON_Parse(xplmsg);
         automator_sendxpl(json_xplmsg);
         cJSON_Delete(json_xplmsg);

         _httpErrno(conn, 0, NULL);
         return 1;
      }
      else
      {
         _httpErrno(conn, 40, NULL);
         return 1;
      }
   }
   else
   {
      _httpErrno(conn, 40, NULL);
      return 1;
   }
}


static int _begin_request_handler(struct mg_connection *conn)
{
   uint32_t id = 0;
   const struct mg_request_info *request_info = mg_get_request_info(conn);
   char phpsessid[80];
   
   char *tokens[MAX_TOKEN]; // 10 tokens maximum
   char buffer[MAX_BUFFER_SIZE];

   httpin_indicator++;
   process_update_indicator(_httpServer_monitoring_id, "HTTPIN", httpin_indicator);

   const char *cookie = mg_get_header(conn, "Cookie");
   if(mg_get_cookie(cookie, "PHPSESSID", phpsessid, sizeof(phpsessid))>0)
   {
   }
   else
      phpsessid[0]=0;
   
   if(strlen((char *)request_info->uri)==11 && mea_strncmplower("/CMD/ps.php",(char *)request_info->uri,11)==0)
   {
      if(!phpsessid[0] || _phpsessid_check_loggedin_and_admin(phpsessid, _php_sessions_path)<0)
      {
         _httpErrno(conn, 2, NULL); // pas habilité
         return 1;
      }
      
      char json[2048];
      managed_processes_processes_to_json_mini(json, sizeof(json)-1);
      _httpResponse(conn, json);
      return 1;
   }
   else if(strlen((char *)request_info->uri)==19 && mea_strncmplower("/CMD/indicators.php",(char *)request_info->uri,11)==0)
   {
      if(!phpsessid[0])
      {
         _httpErrno(conn, 2, NULL); // pas habilité
         return 1;
      }
      
      char json[2048];
      managed_processes_indicators_list(json, sizeof(json)-1);
      _httpResponse(conn, json);
      return 1;
   }
   else if(strlen((char *)request_info->uri)==13 &&  mea_strncmplower("/CMD/ping.php",(char *)request_info->uri,13)==0)
   {
      process_heartbeat(_httpServer_monitoring_id); // le heartbeat est fait de l'extérieur ...
      
      _httpErrno(conn, 0, NULL);
      return 1;
   }
   else if(mea_strncmplower("/CMD/automator.php",(char *)request_info->uri,18)==0)
   {
      return gui_automator(conn, phpsessid, _php_sessions_path);
   }
   else if(mea_strncmplower("/CMD/xplsend.php",(char *)request_info->uri,16)==0)
   {
      return gui_xplsend(conn, phpsessid, _php_sessions_path);
   }
   else if(mea_strncmplower("/CMD/startstop.php",(char *)request_info->uri,18)==0)
   {
      // on récupère le session_id PHP
      if(!phpsessid[0] || _phpsessid_check_loggedin_and_admin(phpsessid, _php_sessions_path)!=1)
      {
         _httpErrno(conn, 2, NULL); // pas habilité
         return 1;
      }
      
      // lire les paramètres
      if(strcmp(request_info->request_method, "GET") != 0)
      {
         _httpErrno(conn, 3, NULL); // pas la bonne méthode
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
            _httpErrno(conn, 4, NULL);
         }
         ret=mg_get_var(request_info->query_string, strlen(request_info->query_string), "cmnd", command, sizeof(command));
         if(ret)
         {
            char errmsg[80];
            if(mea_strncmplower("start", command, strlen(command))==0)
            {
               ret=process_start(process_id, errmsg, sizeof(errmsg));
               if(!ret)
                  _httpErrno(conn, 0, "started");
               else if(ret!=1)
                  _httpErrno(conn, 7, "can't start");
               else
                  _httpErrno(conn, -1, "all ready running !");
            }
            else if (mea_strncmplower("stop", command, strlen(command))==0)
            {
               ret=process_stop(process_id, errmsg, sizeof(errmsg));
               if(!ret)
                  _httpErrno(conn, 0, "stopped");
               else if(ret!=1)
                  _httpErrno(conn, 7, "can't stop");
               else
                  _httpErrno(conn, -1, "all ready not running !");
            }
            else if (mea_strncmplower("task", command, strlen(command))==0)
            {
               ret=process_run_task(process_id, errmsg, sizeof(errmsg));
               if(!ret)
                  _httpErrno(conn, 0, "done");
               else if(ret!=1)
                  _httpErrno(conn, 7, errmsg);
            }
            else
            {
               _httpErrno(conn, 6, NULL);
            }
         }
      }
      else
         _httpErrno(conn, 5, NULL);

      managed_processes_send_stats_now(localhost_const, get_nodejsServer_socketdata_port());
      
      return 1;
   }
/*
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
               xPL_setSchema(msg, get_token_string_by_id(XPL_CONTROL_ID), get_token_string_by_id(XPL_BASIC_ID));
               xPL_setSource(msg, "mea", "internal", "00000000"); // 00000000 = message sans réponse attendue
               waitResp=FALSE;
            }
            else if(mea_strcmplower("SENSOR",tokens[1])==0)
            {
               xPL_setSchema(msg, get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_REQUEST_ID));
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
               strcpy(reponse,"{ \"errno\" : 0 }");
            }
            else
            {
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
                  strcat(reponse,"\"errno\" : 0 }");
                  
                  // réponse traitée, on libère
                  xPL_releaseMessage(respMsg);
               }
               else
               {
                  strcpy(reponse,"{ \"errno\" : 1 }");
               }
            }
            _httpResponse(conn, reponse);
            return 1;
         }
         else
            return 0;
      }
      else
         return 0;
   }
   // pas une api
*/
   return 0;
}


int stop_httpServer()
{
   if(g_mongooseContext)
      mg_stop(g_mongooseContext);
   
   return 0;
}


mea_error_t start_httpServer(uint16_t port, char *home, char *php_cgi, char *php_ini_path)
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
   callbacks.begin_request = _begin_request_handler;
   //   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options);
   if (g_mongooseContext == NULL)
      return ERROR;
   else
      return NOERROR;
}


int stop_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   stop_httpServer();
   VERBOSE(1) mea_log_printf("%s (%s) : HTTPSERVER %s.\n", INFO_STR, __func__, stopped_successfully_str);
   mea_notify_printf('S', "HTTPSERVER %s.", stopped_successfully_str);
   return 0;
}


int start_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct httpServerData_s *httpServerData = (struct httpServerData_s *)data;
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
            VERBOSE(9) mea_log_printf("%s (%s) : GUI port (%s), not a number, 8083 will be used.\n", INFO_STR, __func__, httpServerData->params_list[GUIPORT]);
            guiport=8083;
         }
      }
      else
      {
         VERBOSE(9) mea_log_printf("%s (%s) : can't get GUI port, 8083 will be used.\n", INFO_STR, __func__);
         guiport=8083;
      }
      
      strncpy(_php_sessions_path, httpServerData->params_list[PHPSESSIONS_PATH], sizeof(_php_sessions_path)-1);
      
      if(create_configs_php(httpServerData->params_list[MEA_PATH],
                            httpServerData->params_list[GUI_PATH],
                            httpServerData->params_list[SQLITE3_DB_PARAM_PATH],
                            httpServerData->params_list[LOG_PATH],
                            httpServerData->params_list[PHPSESSIONS_PATH],
                            atoi(httpServerData->params_list[NODEJSIOSOCKET_PORT]))==0)
      {
         _httpServer_monitoring_id=my_id;
         start_httpServer(guiport, httpServerData->params_list[GUI_PATH], phpcgibin, httpServerData->params_list[PHPINI_PATH]);
         
         if(phpcgibin)
         {
            free(phpcgibin);
            phpcgibin=NULL;
         }
         
         VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER %s.\n", INFO_STR, __func__, launched_successfully_str);
         mea_notify_printf('S', "GUISERVER %s.", launched_successfully_str);
         process_heartbeat(_httpServer_monitoring_id); // un premier heartbeat pour le faire au plus vite ...
         return 0;
      }
      else
      {
         VERBOSE(3) mea_log_printf("%s (%s) : can't start GUI Server (can't create configs.php).\n",ERROR_STR,__func__);
         // on continu sans ihm
      }
      free(phpcgibin);
      phpcgibin=NULL;
   }
   else
   {
      VERBOSE(3) mea_log_printf("%s (%s) : can't start GUI Server (parameters errors).\n",ERROR_STR,__func__);
   }
   
   VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER can't be launched.\n", ERROR_STR, __func__);
   mea_notify_printf('E', "GUISERVER can't be launched.");
   
   return -1;
}


int restart_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_guiServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_guiServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}


