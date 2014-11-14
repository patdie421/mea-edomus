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
#include "queue.h"
#include "debug.h"
#include "processManager.h"
#include "timer.h"
#include "sockets_utils.h"
#include "string_utils.h"
#include "consts.h"
#include "notify.h"

// pour la detection de changement d'un fichier voir :
// linux : http://www.ibm.com/developerworks/linux/library/l-inotify/
// mac os x : https://github.com/dmatveev/libinotify-kqueue (lib pour simuler inotify)

char *log_server_name_str="LOGSERVER"; // voir si utilisé

int _livelog_enable=0;


struct logServer_thread_data_s
{
   char *log_path;
   char *hostname;
   int port_socketdata;
};


// Variable globale privée
pthread_t *_logServer_thread=NULL;
volatile sig_atomic_t
           _logServer_thread_is_running=0;
int        _logServer_monitoring_id=-1;
struct logServer_thread_data_s
           _logServer_thread_data;

long logsent_indicator = 0;
long logsenterr_indicator = 0;

// pour envoyer les x dernieres lignes à la connexion d'un client
// prévoir une fonction send last x lines avec comme position de lecture la derniere valeur de pos (à mettre en globale donc)
// cette fonction est appeler par le consommateur avant l'initialisation du gestionnaire d'événement. Lette fonction, par l'intermédiaire d'un verrou,
// bloque toutes les lectures/emissions de lignes (encadrer fpget/mea_send_socket avec le verrou).
// elle lit ensuite les x dernieres lignes sans repositionner pos
// envoyer les x dernieres lignes au demandeur
// débloquer les envoies

/*
static void
recheck (struct File_spec *f, bool blocking)
{
  // open/fstat the file and announce if dev/ino have changed
  struct stat new_stats;
  bool ok = true;
  bool is_stdin = (STREQ (f->name, "-"));
  bool was_tailable = f->tailable;
  int prev_errnum = f->errnum;
  bool new_file;
  int fd = (is_stdin
            ? STDIN_FILENO
            : open (f->name, O_RDONLY | (blocking ? 0 : O_NONBLOCK)));

  assert (valid_file_spec (f));

  // If the open fails because the file doesn't exist,
  // then mark the file as not tailable.
  f->tailable = !(reopen_inaccessible_files && fd == -1);

  if (! disable_inotify && ! lstat (f->name, &new_stats)
      && S_ISLNK (new_stats.st_mode))
    {
      // Diagnose the edge case where a regular file is changed
      // to a symlink.  We avoid inotify with symlinks since
      // it's awkward to match between symlink name and target.
      ok = false;
      f->errnum = -1;
      f->ignore = true;

      error (0, 0, _("%s has been replaced with a symbolic link. "
                     "giving up on this name"), quote (pretty_name (f)));
    }
  else if (fd == -1 || fstat (fd, &new_stats) < 0)
    {
      ok = false;
      f->errnum = errno;
      if (!f->tailable)
        {
          if (was_tailable)
            {
              // FIXME-maybe: detect the case in which the file first becomes
              //   unreadable (perms), and later becomes readable again and can
              //   be seen to be the same file (dev/ino).  Otherwise, tail prints
              //   the entire contents of the file when it becomes readable.
              error (0, f->errnum, _("%s has become inaccessible"),
                     quote (pretty_name (f)));
            }
          else
            {
              // say nothing... it's still not tailable
            }
        }
      else if (prev_errnum != errno)
        {
          error (0, errno, "%s", pretty_name (f));
        }
    }
  else if (!IS_TAILABLE_FILE_TYPE (new_stats.st_mode))
    {
      ok = false;
      f->errnum = -1;
      error (0, 0, _("%s has been replaced with an untailable file;\
 giving up on this name"),
             quote (pretty_name (f)));
      f->ignore = true;
    }
  else if (!disable_inotify && fremote (fd, pretty_name (f)))
    {
      ok = false;
      f->errnum = -1;
      error (0, 0, _("%s has been replaced with a remote file. "
                     "giving up on this name"), quote (pretty_name (f)));
      f->ignore = true;
      f->remote = true;
    }
  else
    {
      f->errnum = 0;
    }

  new_file = false;
  if (!ok)
    {
      close_fd (fd, pretty_name (f));
      close_fd (f->fd, pretty_name (f));
      f->fd = -1;
    }
  else if (prev_errnum && prev_errnum != ENOENT)
    {
      new_file = true;
      assert (f->fd == -1);
      error (0, 0, _("%s has become accessible"), quote (pretty_name (f)));
    }
  else if (f->ino != new_stats.st_ino || f->dev != new_stats.st_dev)
    {
      new_file = true;
      if (f->fd == -1)
        {
          error (0, 0,
                 _("%s has appeared;  following end of new file"),
                 quote (pretty_name (f)));
        }
      else
        {
          // Close the old one
          close_fd (f->fd, pretty_name (f));

          // File has been replaced (e.g., via log rotation) --
          // tail the new one.
          error (0, 0,
                 _("%s has been replaced;  following end of new file"),
                 quote (pretty_name (f)));
        }
    }
  else
    {
      if (f->fd == -1)
        {
          // This happens when one iteration finds the file missing,
          // then the preceding <dev,inode> pair is reused as the
          // file is recreated.
          new_file = true;
        }
      else
        {
          close_fd (fd, pretty_name (f));
        }
    }

  if (new_file)
    {
      // Start at the beginning of the file.
      record_open_fd (f, fd, 0, &new_stats, (is_stdin ? -1 : blocking));
      xlseek (fd, 0, SEEK_SET, pretty_name (f));
    }
}


tail_forever (struct File_spec *f, size_t n_files, double sleep_interval)
{
  // Use blocking I/O as an optimization, when it's easy.
  bool blocking = (pid == 0 && follow_mode == Follow_descriptor
                   && n_files == 1 && ! S_ISREG (f[0].mode));
  size_t last;
  bool writer_is_dead = false;

  last = n_files - 1;

  while (1)
    {
      size_t i;
      bool any_input = false;

      for (i = 0; i < n_files; i++)
        {
          int fd;
          char const *name;
          mode_t mode;
          struct stat stats;
          uintmax_t bytes_read;

          if (f[i].ignore)
            continue;

          if (f[i].fd < 0)
            {
              recheck (&f[i], blocking);
              continue;
            }

          fd = f[i].fd;
          name = pretty_name (&f[i]);
          mode = f[i].mode;

          if (f[i].blocking != blocking)
            {
              int old_flags = fcntl (fd, F_GETFL);
              int new_flags = old_flags | (blocking ? 0 : O_NONBLOCK);
              if (old_flags < 0
                  || (new_flags != old_flags
                      && fcntl (fd, F_SETFL, new_flags) == -1))
                {
                  // Don't update f[i].blocking if fcntl fails.
                  if (S_ISREG (f[i].mode) && errno == EPERM)
                    {
                      // This happens when using tail -f on a file with
                      // the append-only attribute.
                    }
                  else
                    error (EXIT_FAILURE, errno,
                           _("%s: cannot change nonblocking mode"), name);
                }
              else
                f[i].blocking = blocking;
            }

          if (!f[i].blocking)
            {
              if (fstat (fd, &stats) != 0)
                {
                  f[i].fd = -1;
                  f[i].errnum = errno;
                  error (0, errno, "%s", name);
                  close (fd); // ignore failure
                  continue;
                }

              if (f[i].mode == stats.st_mode
                  && (! S_ISREG (stats.st_mode) || f[i].size == stats.st_size)
                  && timespec_cmp (f[i].mtime, get_stat_mtime (&stats)) == 0)
                {
                  if ((max_n_unchanged_stats_between_opens
                       <= f[i].n_unchanged_stats++)
                      && follow_mode == Follow_name)
                    {
                      recheck (&f[i], f[i].blocking);
                      f[i].n_unchanged_stats = 0;
                    }
                  continue;
                }

              // This file has changed.  Print out what we can, and
              // then keep looping.

              f[i].mtime = c (&stats);
              f[i].mode = stats.st_mode;

              // reset counter
              f[i].n_unchanged_stats = 0;

              if (S_ISREG (mode) && stats.st_size < f[i].size)
                {
                  error (0, 0, _("%s: file truncated"), name);
                  last = i;
                  xlseek (fd, stats.st_size, SEEK_SET, name);
                  f[i].size = stats.st_size;
                  continue;
                }

              if (i != last)
                {
                  if (print_headers)
                    write_header (name);
                  last = i;
                }
            }

          bytes_read = dump_remainder (name, fd,
                                       (f[i].blocking
                                        ? COPY_A_BUFFER : COPY_TO_EOF));
          any_input |= (bytes_read != 0);
          f[i].size += bytes_read;
        }

      if (! any_live_files (f, n_files))
        {
          error (0, 0, _("no files remaining"));
          break;
        }

      if ((!any_input || blocking) && fflush (stdout) != 0)
        error (EXIT_FAILURE, errno, _("write error"));

      // If nothing was read, sleep and/or check for dead writers.
      if (!any_input)
        {
          if (writer_is_dead)
            break;

          // Once the writer is dead, read the files once more to
          // avoid a race condition.
          writer_is_dead = (pid != 0
                            && kill (pid, 0) != 0
                            // Handle the case in which you cannot send a
                            // signal to the writer, so kill fails and sets
                            // errno to EPERM.
                            && errno != EPERM);

          if (!writer_is_dead && xnanosleep (sleep_interval))
            error (EXIT_FAILURE, errno, _("cannot read realtime clock"));

        }
    }
}
*/
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
      fclose(fp);
   _logServer_thread_is_running=0;
}


int file_changed(char *file, time_t last_mtime, time_t *new_mtime)
{
  struct stat sb;
  
  if (stat(file, &sb) == -1)
    return -1;
  time_t t = last_mtime;
  *new_mtime = sb.st_mtime;
  if(sb.st_mtime > t)
  {
     return 1;
  }
  else
  {
     return 0;
  }
}


int _read_and_send_lines(int nodejs_socket, char *file, long *pos)
{
   FILE *fp=NULL;
   char line[4096];
   int nb_loop=0;

   fp = fopen(file, "r");
   if(fp == NULL)
   {
      VERBOSE(5) {
//         fprintf(stderr, "%s (%s) :  fopen - can't open %s : ", ERROR_STR, __func__, file);
         mea_log_printf("%s (%s) :  fopen - can't open %s : ", ERROR_STR, __func__, file);
         perror("");
      }
      *pos=0; // le fichier n'existe pas. lorsqu'il sera créé on le lira depuis le debut
      return 0;
   }

   fseek(fp, 0, SEEK_END); // on se met à la fin du fichier
   if(*pos>=0)
   {
      long current=ftell(fp); // position dans le fichier = taille du fichier

      if(*pos >= current) // le fichier a diminué ou est identique en taille, on le relira depuis le debut (on sait qu'il a changé)
      {
         fseek(fp, 0, SEEK_SET);
         *pos=0;
      }
      else // le fichier a grossie (en taille).
      {
         fseek(fp, *pos, SEEK_SET);
      }
   }

   int ret=0;
   while (1)
   {
      pthread_testcancel();
      if(_livelog_enable==0)
      {
          ret=-2; // log desactivée
          break; // plus de communication live ...
      }
      if (fgets(line, sizeof(line), fp) == NULL || nb_loop >= 20)
      {
         // plus de ligne à lire ou déjà 20 ligne transmise on rend la mains, on mémorise la dernière position connue
         *pos=ftell(fp);
         // et on s'arrete pour l'instant
         if(nb_loop >= 20)
            ret=1; // pour dire qu'il ne faudra pas attendre ...
         break; 
      }
      else
      {
         char message[1024];
         int l_data=strlen(line)+4;

         sprintf(message,"$$$xxLOG:%s###", line);
         message[3]=(char)(l_data%128);
         message[4]=(char)(l_data/128);
         int ret = mea_socket_send(&nodejs_socket, message, l_data+12);
         if(ret<0)
         {
            process_update_indicator(_logServer_monitoring_id, "LOGSENTERR", ++logsenterr_indicator);
            ret=-1;
            break;
         }
         process_update_indicator(_logServer_monitoring_id, "LOGSENT", ++logsent_indicator);
         nb_loop++;
      }
   }
   if(fp)
      fclose(fp);

   return ret;
}


int read_line(FILE *fp, char *line, int line_l, long *pos)
{
   char buff[256];
   long current;
   
   fseek(fp, 0, SEEK_END); // on se met à la fin du fichier
   current=ftell(fp); // position dans le fichier = taille du fichier

   if(*pos > current || *pos == -1) // le fichier a diminué ou c'est la première lecture (démarrage du thread)
   {
      // on dit ce qu'on va faire
      if(*pos==-1)
      {
         snprintf(line, line_l-1, "=== start reading from last line ===");
      }
      else
      {
         snprintf(line, line_l-1, "=== file truncated or replaced ===");
      }
      int i=1;
      
      // on va se positionner "un peu" en arrière pour chercher la dernière ligne
      int found=0;
      do
      {
         long p = current-256*i;
         if(p<0)
         {
            *pos=0;
            fseek(fp,0,SEEK_SET);
            found=1;
         }
         else
         {
            fseek(fp, p, SEEK_SET); // on remonte le curseur de lecture de quelques octets ou de quelques octets de plus

            for(int j=0;j<256;j++) // on lit jusqu'à trouver une fin de ligne
            {
               int c=fgetc(fp);
               if(c==EOF && ferror(fp))
               {
                  // erreur de lecture
                  return -1;
               }
               else if(c==EOF)
               {
                  i++;
                  found=0; // pas de fin de ligne trouvé
                  break;
               }
               else if((c==10) || ftell(fp)==current)
               {
                  //) && (ftell(fp) != current)
                  found=1;
                  *pos=ftell(fp); // on marque la possition
                  break;
               }
            }
         }
      }
      while(found!=1);
         
      // cherche la dernière ligne
      fgets(buff, sizeof(buff), fp);
      *pos=ftell(fp);
      
      return 0;
   }
   else if(*pos<current) // le fichier a grossie (en taille).
   {
      fseek(fp, *pos, SEEK_SET);
      fgets(line, line_l, fp);
      *pos=ftell(fp);
      return 0;
   }
   else
   {
      // meme taille
      return 1;
   }
}


void send_line(char *hostname, int port_socketdata, char *line)
{
   static int nodejs_socket = -1;
   
   if(nodejs_socket == -1)
      mea_socket_connect(&nodejs_socket, hostname, port_socketdata);
   
   char message[1024];
   int l_data=strlen(line)+4;

   sprintf(message,"$$$xxLOG:%s###", line);
   message[3]=(char)(l_data%128);
   message[4]=(char)(l_data/128);
   int ret = mea_socket_send(&nodejs_socket, message, l_data+12);
   if(ret<0)
   {
      process_update_indicator(_logServer_monitoring_id, "LOGSENTERR", ++logsenterr_indicator);
      close(nodejs_socket);
      if(mea_socket_connect(&nodejs_socket, hostname, port_socketdata)==-1)
         nodejs_socket=-1;
   }
   else
   {
      process_update_indicator(_logServer_monitoring_id, "LOGSENT", ++logsent_indicator);
   }
}


void *logServer_thread2(void *data)
{
   int exit=0;
//   int nodejs_socket=-1;
   char log_file[256];
   mea_timer_t log_timer;
   FILE *fp=NULL;
   long pos=-1;
   
   struct logServer_thread_data_s *logServer_thread_data=(struct logServer_thread_data_s *)data;
   
   pthread_cleanup_push( (void *)_set_logServer_isnt_running, (void *)fp);
   _logServer_thread_is_running=1;

   _livelog_enable=1; // les log en live sont disponibles
   pthread_testcancel();
   process_heartbeat(_logServer_monitoring_id);

   snprintf(log_file, sizeof(log_file)-1, "%s/mea-edomus.log", logServer_thread_data->log_path);
   _livelog_enable=1;
   init_timer(&log_timer, 5, 1); // heartbeat toutes les 5 secondes
   start_timer(&log_timer);

   do
   {
      if(_livelog_enable==1)
      {
         char line[512];
            
         if(!fp)
         {
            fp = fopen(log_file, "r");
            if(!fp)
            {
               // traiter ici l'erreur
               perror("");
               sleep(1); // on attends un peu
            }
            pos=-1;
         }
            
         if(fp)
         {
            int ret=read_line(fp, line, sizeof(line), &pos);
            if(ret==0)
            {
               send_line(logServer_thread_data->hostname, logServer_thread_data->port_socketdata, line);
            }
            else if(ret==1)
            {
               usleep(500000);
            }
            else
            {
               fclose(fp);
               fp=NULL;
               sleep(1);
            }
         }
      }
      pthread_testcancel();
      if(test_timer(&log_timer)==0)
         process_heartbeat(_logServer_monitoring_id);
   }
   while(exit==0);

   pthread_cleanup_pop(1);

   return NULL;
}


void *logServer_thread(void *data)
{
   int exit=0;
   int nodejs_socket=-1;
   long pos = -1;
   char log_file[256];
   time_t last_mtime = 0;
   
   mea_timer_t log_timer;
   
   struct logServer_thread_data_s *d=(struct logServer_thread_data_s *)data;
   
   pthread_cleanup_push( (void *)_set_logServer_isnt_running, (void *)NULL );
   _logServer_thread_is_running=1;

   process_heartbeat(_logServer_monitoring_id);

   snprintf(log_file, sizeof(log_file)-1, "%s/mea-edomus.log", d->log_path);
   _livelog_enable=1;
   init_timer(&log_timer, 5, 1); // heartbeat toutes les 5 secondes
   start_timer(&log_timer);
   
   do
   {
      pthread_testcancel();
      process_heartbeat(_logServer_monitoring_id);

      if(_livelog_enable == 0) // pas de livelog pas de connexion nécessaire
      {
         pos=-1; // réinitialisation de la lecture (pas de rattrapage).
      }
      else
      {
         if(mea_socket_connect(&nodejs_socket, (char *)(d->hostname), (d->port_socketdata))==0)
         {
            do
            {
               pthread_testcancel();
               if(test_timer(&log_timer)==0)
                  process_heartbeat(_logServer_monitoring_id);

               if(_livelog_enable == 0) // plus de livelog sortie et deconnexion
                  break;

               int ret=file_changed(log_file, last_mtime, &last_mtime);
               if(ret==1)
               {
                  ret=_read_and_send_lines(nodejs_socket, log_file, &pos);
               }
               else if(ret == 0)
               {
               }
               else
               {
                  VERBOSE(9) {
                     mea_log_printf("%s (%s) : stat error - retry next time\n", INFO_STR, __func__);
                  }
                  break;
               }
               
               if(ret==-1)
               {
                  VERBOSE(9) {
                     mea_log_printf("%s (%s) : connection error - retry next time (1)\n", INFO_STR, __func__);
                  }
                  break; // erreur de com. on essayera de se reconnecter au prochain tour.
               }
               else if(ret==-2)
               {
                  VERBOSE(9) {
                     mea_log_printf("%s (%s) : livelog disabled\n", INFO_STR, __func__);
                  }
                  break; // on revoie la situation au prochain tour.
               }
               else if(ret==0)
                  usleep(100000); // 100 ms
            }
            while(1);

            close(nodejs_socket);
         }
         else
         {
            VERBOSE(9) {
               mea_log_printf("%s (%s) : connection error - retry next time (2)\n", INFO_STR, __func__);
            }
         }
      }
      sleep(1); // on essayera de se reconnecter dans 1 secondes
   }
   while(exit==0);

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
      int stopped=-1;
      while(counter--)
      {
        if(_logServer_thread_is_running)
        {
           usleep(100); // will sleep for 1 ms
        }
        else
        {
           stopped=0;
           break;
        }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération\n", DEBUG_STR, __func__, log_server_name_str, 100-counter);
      
      free(_logServer_thread);
      _logServer_thread=NULL;
   }

   _livelog_enable=0;
   VERBOSE(1) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, log_server_name_str, stopped_successfully_str);
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
   pthread_detach(*_logServer_thread);
   _logServer_monitoring_id=my_id;

   VERBOSE(1) mea_log_printf("%s  (%s) :  %s %s.\n", INFO_STR, __func__, log_server_name_str, launched_successfully_str);
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

