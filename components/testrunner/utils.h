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

#ifndef _UTILS_H_
#define _UTILS_H_

#define WARNING 1
#define ERROR 2
#define RESULT 3
#define FATAL_ERROR 4
#define CHATTER 5
#define TRACE_PATH "/sys/kernel/debug/tracing/trace"
#define LS_LOCATION "/system/bin/ls"


typedef struct
{
   char *description;
   float pass_threshold;
   
}metafile_info;

int call_script(char *script_path, char *args[]);
void set_verbose_mode();
void set_brief_mode();
void fatal_report(const char *err);
/*message types: WARNING, ERROR, RESULT, FATAL_ERROR*/
void message_report(const char *msg, int message_type);
int is_dir(char *pathname);
int is_file(char *pathname);
void start_trace();
void stop_trace();
void custom_ls(char *pathname);
int dot_filter(const struct dirent *d);
void map_to_all_alphasorted(char *pathname, void (*f)(char *s));
int redirect_stdout_to(char *path);
void revert_stdout_from(int back_stdout);


#endif
