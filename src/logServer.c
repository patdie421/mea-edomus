//
//  logServer.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "logServer.h"

#include "globals.h"
#include "mea_verbose.h"
#include "processManager.h"
#include "mea_timer.h"
#include "mea_sockets_utils.h"
#include "mea_string_utils.h"
#include "consts.h"
#include "notify.h"

// pour la detection de changement d'un fichier voir :
// linux : http://www.ibm.com/developerworks/linux/library/l-inotify/
// mac os x : https://github.com/dmatveev/libinotify-kqueue (lib pour simuler inotify)

char *log_server_name_str="LOGSERVER";

char *log_server_logsenterr_str="LOGSENTERR";
char *log_server_readerror_str="READERR";
char *log_server_logsent_str="LOGSENT";

int _livelog_enable=0;


struct logServer_thread_data_s
{
   char *log_path;
   char *hostname;
   int port_socketdata;
};


// Variable globale privée
volatile sig_atomic_t
           _logServer_thread_is_running=0;
pthread_t *_logServer_thread=NULL;
int        _logServer_monitoring_id=-1;
struct logServer_thread_data_s
           _logServer_thread_data;

long logsent_indicator = 0;
long logsenterr_indicator = 0;
long readerror_indicator = 0;

void mea_livelog_enable()
{
   _livelog_enable=1;
}


void mea_livelog_disable()
{
   _livelog_enable=0;
}


void _set_logServer_isnt_running(void *data)
{
   FILE *fp=(FILE *)data;
   if(fp)
   {
      fclose(fp);
      fp=NULL;
   }
   _logServer_thread_is_running=0;
}


int send_line(char *hostname, int port_socketdata, char *line)
/**
 * \brief     Envoie une ligne à la console web.
 * \details   Formate un message "console" à destination du serveur nodejs. Si la socket est fermée, la fonction
 *            va essayer d'établir la connexion. La socket est mémorisée par la fonction (variable static).
 *            Attention, en cas d'erreur le message est perdu (sauf si traité par l'éméteur).
 * \param     hostname  nom ou adresse ip du serveur nodejs
 * \param     port_socketdata pour de reception du serveur nodejs
 * \param     line ligne à afficher dans la console
 * \return    -1 le message n'a pas pu être envoyé, 0 sinon
 */
{
   static int nodejs_socket = -1;
   
   if(nodejs_socket == -1)
   {
      if(mea_socket_connect(&nodejs_socket, hostname, port_socketdata)<0)
      {
         process_update_indicator(_logServer_monitoring_id, log_server_logsenterr_str, ++logsenterr_indicator);
         return -1;
      }
   }

   int l_data=(int)(strlen(line)+4); // 4 pour strlen de "LOG:"
   char *message=(char *)alloca(l_data+12);
   sprintf(message,"$$$xxLOG:%s###", line); // on remplacera les xx par la longueur
   message[3]=(char)(l_data%128);
   message[4]=(char)(l_data/128);
   int ret = mea_socket_send(&nodejs_socket, message, l_data+12);
   if(ret<0)
   {
      process_update_indicator(_logServer_monitoring_id, log_server_logsenterr_str, ++logsenterr_indicator);
      close(nodejs_socket);
      nodejs_socket=-1;
      return -1;
   }
   else
   {
      process_update_indicator(_logServer_monitoring_id, log_server_logsent_str, ++logsent_indicator);
   }
   
   return 0;
}


int read_string(char *line, int line_l, FILE *fp)
/**
 * \brief     lit la prochaine chaine de cararctères d'un fichier ouvert à partir de la position courante
 *            dans le fichier.
 * \details   cette fonction va lire caractère par caractère le flot de données d'un fichier ouvert jusqu'à
 *            trouver un caractère "LF" (asc=10). La chaine trouvée est copiée dans "line" si la taille le
 *            permet. Si la taille de "line" ne le permet pas, line est retournée avec les données qui ont pu
 *            être lu et la fonction retourne -1. La fin de chaine est toujours 0 même si la limite
 *            de taille de "line" est atteinte. Si aucune donnée n'est disponible pendant 1s la fonction retourne
 *            -3 mais "line" contient les données qui ont pu être lues.
 * \param     line   chaine allouée par l'appelant qui contiendra la chaine lue.
 * \param     line_l taille en octet de line (fourni par l'appelant).
 * \param     fp     descripteur du fichier à lire
 * \return    -3 timeout de lecture (pas de caractère dispo depuis environ 1 seconde, -2 erreur de lecture,
 *            -1 taille de "line" insuffisante, nombre de caractères lus sinon
 */
{
   int i=0;
   int timeout_counter=0;
   do
   {
      int c=fgetc(fp);
      if(c==EOF)
      {
         clearerr(fp);
         if(timeout_counter>10)
         {
            line[i]='\0';
            return -3;
         }
         usleep(100000);
         timeout_counter++;
      }
      else
      {
         timeout_counter=0;
         if(c<0)
         {
            perror("");
            return -2;
         }
         if(c==0x0A) // LF
         {
            line[i]=0;
            return i;
         }
         else
         {
            if(i<(line_l-1))
            {
               if(c != 0x0D) // CR : on oubli ce caratère s'il est trouvé dans une chaine
               {
                 line[i]=(char)c;
                 i++;
               }
            }
            else
            {
               line[line_l-1]='\0';
               return -1;
            }
         }
      }
   }
   while(1);
}


long seek_to_next_line(FILE *fp, long *pos)
/**
 * \brief     "avance" le pointeur (*pos) de lecture jusqu'au prochain LF (ou CR) dans le fichier.
 * \details   Si la fin de fichier est atteinte avant d'avoir trouvé un LF (CR), on se met en attente
 *            et on continu jusqu'à ce qu'un LF (ou CR) soit ajouté dans le fichier.
 * \param     fp   fichier ouvert à lire.
 * \param     pos  en entrée : pointeur de lecture, en sortie debut de la ligne suivante.
 * \return    -1 : erreur de lecture,
 *             0 : debut de ligne trouvé.
 */
{
/*
   if(fseek(fp,*pos,SEEK_END))
   {
      if(ferror(fp))
      {
         DEBUG_SECTION {
            mea_log_printf("%s  (%s) : fseek - ", INFO_STR, __func__);
            perror("");
         }
         *pos=-1;
         return -1;
      }
   }
*/
   while(1)
   {
      int c=fgetc(fp);

      if(c==EOF && ferror(fp)) // erreur de lecture
      {
         *pos=-1;
         return -1;
      }
      else if(c==EOF)
      {
         clearerr(fp);
         usleep(100000);
      }
      else
      {
         *pos=*pos+1;
         if(c==0x0A || c==0x0D)
         {
            return 0;
         }
      }
   }
}


long seek_to_previous_line(FILE *fp, long *pos)
/**
 * \brief     Positionne le pointeur de lecture au début de la dernière ligne d'un fichier.
 * \details   lecture d'un fichier à partir de la fin pour trouver le debut de la dernière ligne.
 *            Une fin de ligne est identifiée par un LF (0x0A) ou LFCR (0x0A suivi de 0x0D).
 * \param     fp   descripteur ouvert du fichier.
 * \param     pos  valeur du pointeur de fichier.
 * \return    -1 pas de ligne trouvée dans le fichier (pos=-1).
 *            0 fseek réalisé et pos OK.
 */
{
   int found=0;
   int i=0;
   int block_size=0;
   long p=0;
   long end;
   char previous_caracter=0;

   if(fseek(fp,0,SEEK_END)) // On va à la fin du fichier
   {
      if(ferror(fp))
      {
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : fseek - ", INFO_STR, __func__);
            perror("");
         }
         *pos=-1;
         return -1;
      }
   }
   
   end=ftell(fp); // récupération de la position
   *pos=end;
   
   i=1; // un premier bloc à lire
   do
   {
      p = end-256*i; // on prévoit de remonter d'un nouveau bloc de 256 octets par rapport à la fin du fichier
      if(p<0) // bloc trop court (on a atteint le début du fichier), on ajuste
      {
         block_size=(int)(256+p); // rappel : ici p est négatif (c'est donc une soustraction ...)
         p=0;
         fseek(fp,0,SEEK_SET); // on pointe au début du fichier
      }
      else
      {
         block_size=256;
         fseek(fp, p, SEEK_SET); // on point au début du nouveau bloc
      }
      
      // lecture séquentielle des caractères
      for(int j=0;j<block_size;j++) // on lit jusqu'à trouver une fin de ligne
      {
         int c=fgetc(fp);
         if(c==EOF && ferror(fp)) // erreur de lecture
         {
            *pos=-1;
            return -1;
         }
         else if(c==EOF && feof(fp)) // plus rien à lire (fin de fichier)
         {
            clearerr(fp); // c'est pas une erreur ... mais il faut clearer quand même
            break; // et on quitte la boucle for
         }
         else if(c==0x0A || (c==0x0D && previous_caracter==0x0A)) // une fin de ligne
         {
            if(i==1 && j==255) // cas particulier : c'est le premier bloc et le dernier caractère.
            {
               if(c==0x0D && (end-*pos)==1) // la dernière prise de position correspond au LF associé au CR ?
               {
                  found=0; // on dit que finalement on n'a pas trouvé pour pouvoir remonter d'un bloc
               }
            }
            else
               found=1; // dans tous les autres cas on dit qu'on a trouvé une fin de ligne
            *pos=ftell(fp); // on marque la position
         }
         previous_caracter=c; // on garde la trace du dernier caractère lu pour pouvoir traiter les CR en fin de ligne
      }
      if(found)
         break;
      i++;
   }
   while(p!=0);
   
   fseek(fp, *pos, SEEK_SET);
   return 0;
}


int read_line(FILE *fp, char *line, int line_l, long *pos)
/**
 * \brief     Lit à partir de "*pos" ou en fin de fichier si "*pos" == -1.
 * \details   lecture d'un fichier à partir de la fin pour trouver le début de la dernière ligne.
 *            Une fin de ligne est identifiée par un LF (0x0A) ou LFCR (0x0A suivi de 0x0D).
 * \param     fp      descripteur ouvert du fichier.
 * \param     line    chaine allouée par l'appelant qui contiendra la chaine lu.
 * \param     line_l  taille en octet de line (fourni par l'appelant).
 * \param     pos     en entrée : pointeur de lecture (ou -1), en sortie début de la ligne suivante.
 * \return    -1 : erreur de lecture,
 *             0 : ligne lue et potentiellement une autre ligne à lire,
 *             1 : pointeur déjà sur la fin de fichier (rien à lire de suite),
 *             2 : le fichier est plus petit que la position "*pos" => réinitialisation (*pos=-1).
 */
{
   long ret=0;
   long p=0;
   
   if(*pos==-1)
   {
      seek_to_previous_line(fp, pos);
      line[0]='\0';
   }
   else
   {
      fseek(fp,0,SEEK_END);
      p=ftell(fp);
      
      if(p>*pos) // ancien position avant la fin du fichier
      {
         fseek(fp,*pos,SEEK_SET);
         ret=read_string(line, line_l, fp);
         if(ret==-1)
         {
            snprintf(line, line_l-1, "=== line to long for internal buffer, skipped ===");
            *pos=*pos+line_l;
            seek_to_next_line(fp,pos);
            return 0;
         }
         else if(ret < -1)
            return -1; // erreur
         else
         {
            *pos+=(ret+1);
            return 0; // ok pour prochaine lecture
         }
      }
      else if(p<*pos) // ancien position après la fin de la ligne
      {
         *pos=-1; // on reinitialise tout
         return 2;
      }
      else
      {
         return 1; // fichier pas bougé ? (à verifier ailleurs ...)
      }
   }
   return 0; // fin normal, lire prochaine ligne
}


void logServer_thread_cleanup(void *args)
{
//   struct logServer_thread_data_s *params=(struct logServer_thread_data_s *)args;
   
//   free(params);
}


void *logServer_thread(void *data)
{
   int exit=0;
   char log_file[256];
   mea_timer_t log_timer;
   FILE *fp=NULL;
   long pos=-1;
   
   struct logServer_thread_data_s *logServer_thread_data=(struct logServer_thread_data_s *)data;

   pthread_cleanup_push( (void *)logServer_thread_cleanup, (void *)logServer_thread_data);
   pthread_cleanup_push( (void *)_set_logServer_isnt_running, (void *)fp);
   _logServer_thread_is_running=1;

   _livelog_enable=1; // les log en live sont disponibles
   pthread_testcancel();
   process_heartbeat(_logServer_monitoring_id);

   snprintf(log_file, sizeof(log_file)-1, "%s/mea-edomus.log", logServer_thread_data->log_path);
   mea_init_timer(&log_timer, 5, 1); // heartbeat toutes les 5 secondes
   mea_start_timer(&log_timer);

    char line[4096];
   do
   {
      if(_livelog_enable==1)
      {
         line[0]=0; 
         if(!fp)
         {
            fp = fopen(log_file, "r");
            if(!fp)
            {
               DEBUG_SECTION {
                  mea_log_printf("%s (%s) : fopen - ", INFO_STR, __func__);
                  perror("");
               }
               sleep(5); // on attends un peu
               break;
            }
            pos=-1;
         }
            
         if(fp)
         {
            int ret=read_line(fp, line, sizeof(line)-1, &pos);
            if(ret==0)
            {
               if(line[0]) // ligne non vide
               {
//                  fprintf(dbgfd, "[%s]\n",line);
//                  fflush(dbgfd);
                  if(send_line(logServer_thread_data->hostname, logServer_thread_data->port_socketdata, line) < 0)
                  {
//                     VERBOSE(9) mea_log_printf("%s (%s) : can't send line.\n", ERROR_STR, __func__);
                  }
               }
            }
            else if(ret==1)
            {
               usleep(500000); // 500 ms d'attente
            }
            else if(ret==2) // fichier est plus petit
            {
               VERBOSE(9) mea_log_printf("%s (%s) : log file changed, reset reading to last line.\n", WARNING_STR, __func__);
               // par précaution on ferme le fichier pour tout réinitialiser, il sera réouvert au prochain tour.
               fclose(fp);
               fp=NULL;
            }
            else
            {
               process_update_indicator(_logServer_monitoring_id, log_server_readerror_str, ++readerror_indicator);
               fclose(fp);
               fp=NULL;
               sleep(1);
            }
         }
      }
      pthread_testcancel();
      if(mea_test_timer(&log_timer)==0)
         process_heartbeat(_logServer_monitoring_id);
   }
   while(exit==0);

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
   
   return NULL;
}


int stop_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=-1;
   if(_logServer_thread)
   {
      pthread_cancel(*_logServer_thread);
      int counter=100;
//      int stopped=-1;
      while(counter--)
      {
        if(_logServer_thread_is_running)
        {
           usleep(100); // will sleep for 1 ms
        }
        else
        {
//           stopped=0;
           break;
        }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d iterations\n", DEBUG_STR, __func__, log_server_name_str, 100-counter);
      
      free(_logServer_thread);
      _logServer_thread=NULL;
   }

   _livelog_enable=0;
   VERBOSE(1) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, log_server_name_str, stopped_successfully_str);
   mea_notify_printf('S', "%s %s (%d).",stopped_successfully_str, log_server_name_str, ret);

   return 0;
}


int start_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct logServerData_s *logServerData = (struct logServerData_s *)data;

   char err_str[80];

   _logServer_thread_data.log_path=logServerData->params_list[LOG_PATH];
   _logServer_thread_data.hostname=localhost_const;
   _logServer_thread_data.port_socketdata=atoi(logServerData->params_list[NODEJSDATA_PORT]);

   _logServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_logServer_thread)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         mea_log_printf("%s (%s) : malloc - %s",ERROR_STR,__func__,err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s", log_server_name_str, err_str);
      return -1;
   }
   
   if(pthread_create (_logServer_thread, NULL, logServer_thread, (void *)&_logServer_thread_data))
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(1) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - %s",ERROR_STR,__func__,err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s", log_server_name_str, err_str);

      return -1;
   }
   fprintf(stderr,"LOGSERVER : %x\n", (unsigned int)*_logServer_thread);
   pthread_detach(*_logServer_thread);

   _logServer_monitoring_id=my_id;

   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, log_server_name_str, launched_successfully_str);
   mea_notify_printf('S', "%s %s.", log_server_name_str, launched_successfully_str);
   return 0;
}


int restart_logServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_logServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_logServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}

