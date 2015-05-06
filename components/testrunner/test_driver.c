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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "cmd_parser.h"
#include "utils.h"
#include "suiteutils.h"
#include "metafile_parser.h"

#define DEFAULT_PASS_THRESHOLD 0.9999
#define MAX_THRESHOLD 1.0
#define MIN_THRESHOLD 0.0

int run(run_setup *);
int runall(run_setup *);
int list(run_setup *);
int addsuite(run_setup *);
int description(run_setup *);

int main(int argc, char *argv[])
{
   run_setup *setup;
   setup=parse_command(argc, argv);
   int ret_val = 0;

   if(setup->runF)
   {
      if(setup->suiteF)
          create_tagfile(setup->suite, setup->tag);
      if(setup->allF)
         ret_val = runall(setup);
      else
         ret_val = run(setup);
      if(setup->suiteF)
          delete_tagfile(setup->suite);
   }
   if(setup->descriptionF)
      description(setup);
   if(setup->addsuiteF)
      addsuite(setup);
   if(setup->listF)
      ret_val=list(setup);

   del_run_setup(setup);
   return ret_val;
}

/*
 * runs a testcase according to the parameters of the metafile and the options chosen
 * by the user kept in the setup argument of this function
 * returns
 * 0 if testcase passes
 * 1 if it does not pass
 * -1 if testcase is deprecated
 */
int run(run_setup *setup)
{
   char *scriptpath, *descfile_path;
   char *trunner_path, *tcase_path;
   char *result_path, *suit_result_path;
   int exit_status, i, passes=0, postprocessing_enabled=1, postprocessing_outcome;
   int deprecated = 0;
   int saved_stdout_fd;
   float pass_threashold=DEFAULT_PASS_THRESHOLD, pass_ratio=0.0;
   FILE *suite_log;

   trunner_path = get_current_dir_name();
   tcase_path = get_testcase_path(setup->suite, setup->testcase);
   scriptpath = get_testcase_script(setup->suite, setup->testcase);
   result_path = get_testcase_resultfile(setup->suite, setup->testcase, setup->tag);
   suit_result_path = get_suite_resultfile(setup->suite, setup->tag);
   if(!is_file(scriptpath))
   {
      message_report(
         "entry point script is missing for specified suite and testcase", FATAL_ERROR);
      exit(-1);
   }
   descfile_path = get_testcase_descfile(setup->suite, setup->testcase);
   if(is_file(descfile_path) )
   {
      get_isdeprecated(&deprecated, descfile_path);
      if(deprecated)
      {
         char *deprecated_message = (char *)calloc( strlen(setup->testcase)+strlen(": deprecated") + 1, sizeof(char) );
         if(!deprecated_message)
         {
            message_report("failed to allocate memory", FATAL_ERROR);
            exit(-1);
         }
         sprintf(deprecated_message, "%s: deprecated", setup->testcase);
         message_report(deprecated_message, CHATTER);
         suite_log = fopen(suit_result_path, "a");
         fprintf(suite_log, "Testcase: %s\t\t\tStatus: deprecated\n", setup->testcase);
         free(scriptpath);
         free(trunner_path);
         free(tcase_path);
         fclose(suite_log);
         return -1;
      }
      if(!get_passthreshold(&pass_threashold, descfile_path) )
         pass_threashold=DEFAULT_PASS_THRESHOLD;
      else if(pass_threashold > MAX_THRESHOLD || pass_threashold < MIN_THRESHOLD )
      {
         message_report("defined pass threashold is out of range, it must be between 0.0 and 1.0", WARNING);
         pass_threashold=DEFAULT_PASS_THRESHOLD;
      }
   }
   if( chdir(tcase_path) ) /*changing into testcase directory in order to enable run script to use relative paths */
   {
      message_report("unable to open testcase directory", FATAL_ERROR);
      exit(-1);
   }
   saved_stdout_fd = redirect_stdout_to(result_path);
   for(i=0; i<setup->repetitions; ++i){
      if(i>0)
         printf("\n-----------------------------------------------------\n");
      exit_status = call_script(scriptpath, NULL);
      if(!exit_status)
         ++passes;
   }
   revert_stdout_from(saved_stdout_fd);
   if( chdir(trunner_path) ) /* back to testrunner directory */
   {
      message_report("an unexpected error has occurred", FATAL_ERROR);
      exit(-1);
   }
   free(scriptpath);
   free(trunner_path);
   free(tcase_path);
   pass_ratio=(float)passes / (float)setup->repetitions;
   suite_log = fopen(suit_result_path, "a");
   if(!suite_log)
   {
      message_report("failed to open suite log file", FATAL_ERROR);
      exit(-1);
   }
   if(pass_ratio>pass_threashold)
   {
      char *success_message = (char *)calloc( strlen(setup->testcase)+strlen(": succeeded") + 1, sizeof(char) );
      if(!success_message)
      {
         message_report("failed to allocate memory", FATAL_ERROR);
         exit(-1);
      }
      sprintf(success_message, "%s: succeeded", setup->testcase);
      fprintf(suite_log, "Testcase: %s\t\t\tStatus: pass\tSuccess Ratio: %f\n", setup->testcase, pass_ratio);
      fflush(suite_log);
      message_report(success_message, CHATTER);
      free(success_message);
      fclose(suite_log);
      return 0;
   }
   else
   {
      char *fail_message = (char *)calloc( strlen(setup->testcase)+strlen(": failed") + 1, sizeof(char) );
      if(!fail_message)
      {
         message_report("failed to allocate memory", FATAL_ERROR);
         exit(-1);
      }
      sprintf(fail_message, "%s: failed", setup->testcase);
      fprintf(suite_log, "Testcase: %s\t\t\tStatus: fail\tSuccess Ratio: %f\n", setup->testcase, pass_ratio);
      fflush(suite_log);
      message_report( fail_message, CHATTER);
      free(fail_message);
      fclose(suite_log);
      return 1;
   }
}

int list(run_setup *setup)
{
   if(setup->suiteF) /*a suite was specified*/
   {
      char *tcase_top = get_testcase_top(setup->suite);

      if(!is_dir(tcase_top))
      {
         message_report("specified suite does not exist!", ERROR);
         return 1;
      }
      else
      {
         custom_ls(tcase_top);
      }
   }
   else /*suite was not specified*/
   {
      char suite_name[MAX_SUITE_NAME], suite_path[MAX_SUITE_PATH];
      FILE *sfp = get_suitefile_fp();
      while(1)
      {
         fscanf(sfp, "%s %s", suite_name, suite_path);
         if(feof(sfp))
            break;
         message_report(suite_name, RESULT);
         message_report(" ", RESULT);
      }
      message_report("\n", RESULT);
   }
   return 0;
}

int runall(run_setup *setup)
{
   char *spath;
   int items=0, i=0, overal_return=0;
   int pass=0, fail=0, result=0;
   struct dirent **d;
   FILE *suite_log;
   char *suite_result_path = get_suite_resultfile(setup->suite, setup->tag);

   spath = get_testcase_top(setup->suite);
   items=scandir(spath, &d, dot_filter, alphasort);
   for(i=0; i<items; ++i)
   {
      setup->testcase = d[i]->d_name;
      result = run(setup);
      if(-1==result)
      {
         continue;
      }
      overal_return |= result;
      if(result)
         ++fail;
      else
         ++pass;
   }
   suite_log = fopen(suite_result_path, "a");
   if(!suite_log)
   {
      message_report("unable to open suite log file", FATAL_ERROR);
      exit(-1);
   }
   fprintf(suite_log, "\nOveral results:\nSuite Name: %s\nNumber of testcases: %d\nPassed: %d\nFailed: %d\n",
           setup->suite, pass+fail, pass, fail);
   fclose(suite_log);
   setup->testcase = NULL;
   return overal_return;
}

int addsuite(run_setup *setup)
{
   if(!is_dir(setup->newsuitepath))
   {
      message_report("New suite path is not a valid directory", ERROR);
      return 1;
   }
   add_suite(setup->suite, setup->newsuitepath);
   return 0;
}

int description(run_setup *setup)
{
   char *desc;
   char *desc_path;

   desc_path=get_testcase_descfile(setup->suite, setup->testcase);
   if(!is_file(desc_path))
   {
      message_report("desc.meta file not found for given suite and testcase", ERROR);
      exit(-1);
   }
   if(get_description(&desc, desc_path))
      message_report(desc, RESULT);
   else
      message_report("testcase does not provide description information", CHATTER);
   return 0;
}
