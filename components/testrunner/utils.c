/*
 * Copyright (C) 2015 ARM Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include "utils.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>


static int verbose_flag;

/* a system() clone*/
int call_script(char *script_path, char *args[])
{
   if(0==fork())
   {//child
      char *shell_interpreter, *t_script_path;
      size_t shell_interpreter_sz, t_script_path_sz;
      int fd_android = 0, system_ret;

      fd_android = open("/system/build.prop", O_RDONLY);
      if (fd_android != -1) {
          shell_interpreter_sz = strlen("/system/bin/sh ") + 1;
          shell_interpreter = (char *)calloc(1, shell_interpreter_sz);
          strcpy(shell_interpreter, "/system/bin/sh ");
      }
      else
      {
          shell_interpreter_sz = strlen("/bin/bash ") + 1;
          shell_interpreter = (char *)calloc(1, shell_interpreter_sz);
          strcpy(shell_interpreter, "/bin/bash ");
      }

      close(fd_android);
      t_script_path_sz  = shell_interpreter_sz + strlen(script_path) + 1;
      t_script_path = (char *)calloc(1, t_script_path_sz);

      strcpy(t_script_path, shell_interpreter);
      strcat(t_script_path, script_path);
      system_ret = system(t_script_path);
      free(t_script_path);
      free(shell_interpreter);

      if (system_ret != 0) {
        message_report("an unexpected error has occurred!", FATAL_ERROR);
        perror("");
        exit(1);
      }
      exit(0);
   }
   else
   {
      int stat;
      wait(&stat);
      stat = WEXITSTATUS(stat);
      return stat;
   }
}

/* in verrbose mode all warnings errors and results are reported */
void set_verbose_mode()
{
   verbose_flag = 1;
}

/* brief is the default state in which only fatal errors are reported */
void set_brief_mode()
{
   verbose_flag = 0;
}

/*This should be used for irecoverable errors that cause termination of tdriver*/
void fatal_report(const char *err)
{
   printf("Fatal error: %s \n", err);
   fflush(stdout);
}

/*This should be used for all types of messages (including fatal errors)
 *as long as the correct type is specified (defined in utils.h)
 */
void message_report(const char *msg, int message_type)
{
   switch(message_type)
   {
      case FATAL_ERROR:
         fatal_report(msg);
         exit(-1);
         break;
      case RESULT:
         fprintf(stdout, "%s", msg);
         break;
      case WARNING:
         if(0!=verbose_flag)
            fprintf(stderr, "warning: %s\n", msg);
         break;
      case CHATTER:
         if(0!=verbose_flag)
            fprintf(stderr, "%s\n", msg);
         break;
      case ERROR:
         fprintf(stderr, "error: %s\n", msg);
         exit(-1);
         break;
      default:
         assert(0);
   }
   fflush(stderr);
}

/*returns true if pathname points to an existing directory
 */
int is_dir(char *pathname)
{
   struct stat sbuf;

   if(-1==stat(pathname, &sbuf))
      return 0;
   else
      return S_ISDIR(sbuf.st_mode);
}

/*returns true if pathname points to an existing regular file
 */
int is_file(char *pathname)
{
   struct stat sbuf;

   if (-1==stat(pathname, &sbuf))
      return 0;
   else
      return S_ISREG(sbuf.st_mode);
}

void start_trace()
{
   call_script("trace_start.sh", NULL);
}

void stop_trace()
{
   call_script("trace_stop.sh", NULL);
}

int dot_filter(const struct dirent *d)
{
   if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
      return 0;
   return 1;
}

void map_to_all_alphasorted(char *pathname, void (*f)(char *s))
{
    int items=0, i=0;
    struct dirent **d;

    if( !is_dir(pathname) )
       return;
    items=scandir(pathname, &d, dot_filter, alphasort);
    for(i=0; i<items; ++i)
      f(d[i]->d_name);
}

void lsprint(char *s)
{
   printf("%s\n", s);
}

void custom_ls(char *pathname)
{
   map_to_all_alphasorted(pathname, lsprint);
}

int redirect_stdout_to(char *path)
{
   unsigned int out_fd;
   unsigned int back_stdout;

   fflush(stdout);
   if(-1 == (back_stdout = dup(1)) )
   {
      message_report("failed to redirect stdout", FATAL_ERROR);
      exit(-1);
   }
   if(-1 == (out_fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 0644)) )
   {
      message_report("failed to redirect stdout", FATAL_ERROR);
      exit(-1);
   }
   if(-1 == (dup2(out_fd, 1)) )
   {
      message_report("failed to redirect stdout", FATAL_ERROR);
      exit(-1);
   }
   close(out_fd);
   return back_stdout;
}

void revert_stdout_from(int back_stdout)
{
   fflush(stdout);
   if(-1 == (dup2(back_stdout, 1)) )
   {
      message_report("failed to restore stdout", FATAL_ERROR);
      exit(-1);
   }
   close(back_stdout);
}
