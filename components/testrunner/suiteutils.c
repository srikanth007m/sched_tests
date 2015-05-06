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
#include <string.h>
#include <assert.h>
#include "suiteutils.h"
#include "utils.h"
#include <unistd.h>

#define SUITEFILE "suites.txt"
#define TESTCASEDIR "/testcases/"
#define SCRIPTNAME "/run.sh"
#define DESC_METAFILENAME "/desc.meta"
#define VAL_METAFILENAME "/val.meta"
#define PP_LIBFILENAME "/analysis.so"
#define TAG_FILE_NAME "/tag.txt"

void add_suite(const char *suitename, const char *suitepath)
{
   FILE *sfile;

   if(!is_dir(suitepath) )
   {
      message_report("specified suite path does not exist", ERROR);
      return;
   }
   if(!(sfile=fopen(SUITEFILE, "a+") ))
   {
      message_report("unable to open suite file", FATAL_ERROR);
      exit(-1);
   }
   if('/'==suitepath[strlen(suitepath)-1])/*eat up last '/' from pathname*/
   {
      char *new_suitepath=(char *)calloc(1, strlen(suitepath)+ 1);
      if(!new_suitepath)
      {
         message_report("failed to allocate memory", FATAL_ERROR);
         exit(-1);
      }
      strcpy(new_suitepath, suitepath);
      new_suitepath[strlen(new_suitepath)-1]='\0';
      fprintf(sfile, "%s %s\n", suitename, new_suitepath);
      fclose(sfile);
      free(new_suitepath);
      return;
   }
   fprintf(sfile, "%s %s\n", suitename, suitepath);
   fclose(sfile);
}

char * get_suite_path(const char *suitename)
{
   char sname[MAX_SUITE_NAME];
   char spath[MAX_SUITE_PATH];
   FILE *sfile = fopen(SUITEFILE, "r");
   if(NULL==sfile)
   {
      /*TODO error handling*/
      exit(-1);
   }
   while(!feof(sfile)){
      fscanf(sfile, "%s %s", sname, spath);
      if(!strcmp(sname, suitename) ) /*suite name found*/
      {
         char *retpath = (char *)calloc(1, strlen(spath)+1 );
         if(!retpath)
         {
            fatal_report("failed to allocate memory");
            exit(-1);
         }
         fclose(sfile);
         strcpy(retpath, spath);
         return retpath;
      }
   }
   message_report("specified suite not found", ERROR);
   exit (-1);
}

char *get_testcase_top(const char *suitename)
{
   char *tc_top;

   tc_top = get_suite_path(suitename);
   tc_top = realloc(tc_top, strlen(tc_top)+strlen(TESTCASEDIR)+1);
   if(!tc_top)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   if(!strcat(tc_top, TESTCASEDIR))
   {
      message_report("failed to concatenate string", FATAL_ERROR);
      exit(-1);
   }
   return tc_top;
}

char *get_testcase_path(const char *suitename, const char *testcasename)
{
   char *suitepath, *fullpath;
   size_t fpath_sz;

   suitepath=get_suite_path(suitename);
   fpath_sz = strlen(suitepath) + strlen(testcasename) + strlen(TESTCASEDIR) + 1;
   fullpath = (char *)calloc(1, fpath_sz);
   if(!fullpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(fullpath, suitepath);
   strcat(fullpath, TESTCASEDIR);
   strcat(fullpath, testcasename);
   free(suitepath);
   return fullpath;
}

/*
 * returns the pathname of the entry point script for the given
 * suite and testcase
 */
char *get_testcase_script(const char *suitename, const char *testcasename)
{
   char *tcasepath, *scriptpath;
   size_t fpath_sz;

   tcasepath=get_testcase_path(suitename, testcasename);
   fpath_sz = strlen(tcasepath) + strlen(SCRIPTNAME) + 1;
   scriptpath = (char *)calloc(1, fpath_sz);
   if(!scriptpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(scriptpath, tcasepath);
   strcat(scriptpath, SCRIPTNAME);
   free(tcasepath);
   return scriptpath;
}

char *get_testcase_descfile(const char *suitename, const char *testcasename)
{
   char *tcasepath, *metafileptpath;
   size_t fpath_sz;

   tcasepath=get_testcase_path(suitename, testcasename);
   fpath_sz = strlen(tcasepath) + strlen(DESC_METAFILENAME) + 1;
   metafileptpath = (char *)calloc(1, fpath_sz);
   if(!metafileptpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(metafileptpath, tcasepath);
   strcat(metafileptpath, DESC_METAFILENAME);
   free(tcasepath);
   return metafileptpath;
}

//WARNING valfile is deprecated
char *get_testcase_valfile(const char *suitename, const char *testcasename)
{
   char *tcasepath, *metafileptpath;
   size_t fpath_sz;

   message_report("val file is deprecated", WARNING);
   tcasepath=get_testcase_path(suitename, testcasename);
   fpath_sz = strlen(tcasepath) + strlen(VAL_METAFILENAME) + 1;
   metafileptpath = (char *)calloc(1, fpath_sz);
   if(!metafileptpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(metafileptpath, tcasepath);
   strcat(metafileptpath, VAL_METAFILENAME);
   free(tcasepath);
   return metafileptpath;
}

/*
 * returns the path of the post processing library for the specified testcase and suite
 * does not check whether the shared lib file actually exists (this is
 * the responsibility of the caller)
 */
char *get_testcase_pplib(const char *suitename, const char *testcasename)
{
   char *tcasepath, *pplibpath;
   size_t fpath_sz;

   tcasepath=get_testcase_path(suitename, testcasename);
   fpath_sz = strlen(tcasepath) + strlen(PP_LIBFILENAME) + 1;
   pplibpath = (char *)calloc(1, fpath_sz);
   if(!pplibpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(pplibpath, tcasepath);
   strcat(pplibpath, PP_LIBFILENAME);
   free(tcasepath);
   return pplibpath;
}


char *get_testcase_resultfile(const char *suitename, const char *testcasename, char *tag)
{
   char *tcasepath, *result_fileptpath;
   size_t fpath_sz;

   tcasepath=get_testcase_path(suitename, testcasename);
   fpath_sz = strlen(tcasepath) + strlen(testcasename) + strlen(tag) + strlen("/_.res") + 1;
   result_fileptpath = (char *)calloc(1, fpath_sz);
   if(!result_fileptpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(result_fileptpath, tcasepath);
   strcat(result_fileptpath, "/");
   strcat(result_fileptpath, testcasename);
   strcat(result_fileptpath, "_");
   strcat(result_fileptpath, tag);
   strcat(result_fileptpath, ".res");
   free(tcasepath);
   return result_fileptpath;
}

char *get_suite_resultfile(const char *suitename, char *tag)
{
   char *suitepath, *result_fileptpath;
   size_t fpath_sz;

   suitepath=get_suite_path(suitename);
   fpath_sz = strlen(suitepath) + strlen(suitename) + strlen(tag) + strlen("/_.res") + 1;
   result_fileptpath = (char *)calloc(1, fpath_sz);
   if(!result_fileptpath)
   {
      message_report("failed to allocate memory", FATAL_ERROR);
      exit(-1);
   }
   strcpy(result_fileptpath, suitepath);
   strcat(result_fileptpath, "/");
   strcat(result_fileptpath, suitename);
   strcat(result_fileptpath, "_");
   strcat(result_fileptpath, tag);
   strcat(result_fileptpath, ".res");
   free(suitepath);
   return result_fileptpath;
}



char *tagfile_name(const char *suitename)
{
    char *suitepath, *result_fileptpath;
    size_t fpath_sz;

    suitepath=get_suite_path(suitename);
    fpath_sz = strlen(suitepath) + strlen(TAG_FILE_NAME) + 1;
    result_fileptpath = (char *)calloc(1, fpath_sz);
    if(!result_fileptpath)
    {
        message_report("failed to allocate memory", FATAL_ERROR);
        exit(-1);
    }
    strcpy(result_fileptpath, suitepath);
    strcat(result_fileptpath, TAG_FILE_NAME);
    return result_fileptpath;
}

/*
 * creates a tagfile at the route of a runnigng suite.
 * tagfile's path is /path/to/suite/TAG_FILE.txt
 */
void create_tagfile(const char *suitename, char *tag)
{
    FILE *tagfp;
    char *tagfname = tagfile_name(suitename);

    if( NULL == (tagfp = fopen(tagfname, "w") ) )
    {
        message_report("failed to create tag file", FATAL_ERROR);
        exit(-1);
    }
    fprintf(tagfp, "%s", tag);
    fclose(tagfp);
    free(tagfname);
}

/*
 * deletes tag file for the given suite
 */
void delete_tagfile(const char *suitename)
{
    char *tagfname = tagfile_name(suitename);

    if(-1 == unlink(tagfname) )
    {
        message_report("failed to delete tag file", FATAL_ERROR);
        exit(-1);
    }
    free(tagfname);
}

/*
 * returns a read only file pointer to the file that contains all suitnames and paths
 * If file does not exist, returns NULL.
 * caller is responsible to close the file pointer after he is done reading the file
 */
FILE *get_suitefile_fp()
{
   FILE *sfp = NULL;

   if(!is_file(SUITEFILE) )
      return NULL;
   sfp = fopen(SUITEFILE, "r");
   if(!sfp)
   {
      message_report("problem with opening internal suite file", FATAL_ERROR);
      exit(-1);
   }
   return sfp;
}
