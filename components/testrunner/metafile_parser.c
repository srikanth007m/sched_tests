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
#include <regex.h>
#include <string.h>
#include "metafile_parser.h"
#include "utils.h"

#define ANY_TAG ".*=[[:blank:]]*|.*=[[:blank:]]*"
#define DESC_TAG "[[:blank:]]*DESCRIPTION[[:blank:]]*:[[:blank:]]*|[[:blank:]]*DESCRIPTION[[:blank:]]*=[[:blank:]]*"
#define SUCCESSRATE_TAG "[[:blank:]]*ExpectedSuccessRate[[:blank:]]*:[[:blank:]]*|[[:blank:]]*ExpectedSuccessRate[[:blank:]]*=[[:blank:]]*"
#define DEPRECATED_TAG "[[:blank:]]*DEPRECATED[[:blank:]]*:[[:blank:]]*|[[:blank:]]*DEPRECATED[[:blank:]]*=[[:blank:]]*"


FILE *open_metafile(char *metafilepath)
{
   FILE *fp = fopen(metafilepath, "r");
   if(!fp)
   {
      message_report("failed to open meta file", FATAL_ERROR);
      exit(-1);
   }
   return fp;
}

size_t file_size(FILE *fp)
{
   size_t sz;

   fseek(fp, 0L, SEEK_END);
   sz = ftell(fp);
   rewind(fp);
   return sz;
}

char *get_tagval(FILE *fp, char *tag_pattern)
{
   int fsz, match_f, dstart, dsz;
   regex_t re;
   regmatch_t re_match[2];
   char *buff, *ret;

   fsz = file_size(fp);
   if( !( buff=(char *)malloc(sizeof(char)*(fsz+1)) ))
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   buff[fsz]='\0';/*making sure buff is nul termninated*/
   fread(buff, sizeof(char), fsz, fp);
   regcomp(&re, tag_pattern, REG_EXTENDED | REG_ICASE);
   match_f=regexec(&re, buff,  1, re_match, 0);
   regfree(&re);
   if(match_f)
   {
      free(buff);
      return NULL;
   }
   dstart =re_match[0].rm_eo;
   regcomp(&re, ANY_TAG, REG_EXTENDED | REG_ICASE | REG_NEWLINE);
   match_f=regexec(&re, buff+dstart,  1, re_match, 0);
   if(!match_f)
      dsz=re_match[0].rm_so;
   else
      dsz=fsz-dstart;
   if( !( ret=(char *)calloc(dsz+1, sizeof(char)) ))
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   memcpy(ret, buff+dstart, dsz);
   regfree(&re);
   free(buff);
   return ret;
}

/*
 * returns 0 if description was not found in metafile.
 * the result is stored
 */
int get_description(char **description, char *metafilepath)
{
   FILE *fp;

   fp=open_metafile(metafilepath);
   *description = get_tagval(fp, DESC_TAG);
   if(*description)
      return 1;
   return 0;
}


int get_passthreshold(float *threashold, char *metafilepath)
{
   FILE *fp;
   char *th, *pos;

   fp=open_metafile(metafilepath);
   th = get_tagval(fp, SUCCESSRATE_TAG);
   if(th)
   {
      *threashold=strtof(th, &pos);
      if(th==pos)
         return 0;
      return 1;
   }
   return 0;
}

int get_isdeprecated(int *deprecated, char *metafilepath)
{
   FILE *fp;
   char *dep, *pos;

   fp=open_metafile(metafilepath);
   dep = get_tagval(fp, DEPRECATED_TAG);
   fclose(fp);
   *deprecated = 0;
   if(dep)
   {
      if( !strncasecmp("true", dep, strlen("true") ) )
         *deprecated = 1;
   }
   return 1;
}

int get_uniquename(char** name, char *metafilepath);

