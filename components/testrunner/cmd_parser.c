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
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include "cmd_parser.h"
#include "utils.h"


void del_run_setup(run_setup *rs)
{
   free(rs->testcase);
   free(rs->suite);
   free(rs);
}

void print_usage()
{
   printf("usage :\n");
   printf("testrunner [--[OPTION] [ARGUMENT]]...\n\n");
   printf("OPTION LIST :\n");
   printf("--run TAG\tindicates that the testcase specified must be executed using TAG to mark output files\n");
   printf("\t\t\tsee --suite and --testcase\n");
   printf("--list \tif a suite is specified it will list all its testcases\n");
   printf("\t\t\totherwise it will print all known suites\n");
   printf("--add PATH\t requires that the --suite option is given\n");
   printf("\t\t\t it will permanently asociate the given PATH with the specified suite\n");
   printf("--suite SUITENAME\t indicates on which suite testrunner should operate\n");
   printf("--testcase TESTCASE\t indicates the testcase on which testrunner should operate\n");
   printf("\t\t\t whenever the --testcase option is enabled the --suite should also be enabled\n");
   printf("--description\t will print the description of the specified testcase\n");
   printf("--verbose\t indicates that all messages warnings and results should be printed\n");
   printf("--n N\t specifies the number of repetitions of the given testcase\n");
}


/*
 * This function is responsible for checking the semantic correctness
 * of the given command (but does not do path checking)
 * WARNING: when adding functionality to tdriver this function needs to be updated
 */
void semantic_check(run_setup *rs)
{
   if(rs->verboseF)
      set_verbose_mode();
   if(rs->listF+rs->runF+rs->descriptionF+rs->addsuiteF > 1)
   {
      message_report("too manny arguments", WARNING);
      if(rs->runF)
      {
         rs->listF=0;
         rs->descriptionF=0;
         rs->addsuiteF=0;
      }
      else
      {
         rs->runF=0;
         rs->descriptionF=0;
         rs->addsuiteF=0;
      }
   }
   if(rs->runF && ( !(rs->suiteF && rs->testcaseF) && !(rs->suiteF && rs->allF) ) )
   {
      rs->runF=0;
      message_report("suite and test case or --all need to be specified", ERROR);
      return;
   }
   if(rs->addsuiteF & !(rs->suiteF))
   {
      rs->addsuiteF=0;
      message_report("suite name needs to be specified: --suite <name>", ERROR);
   }
   if(rs->descriptionF & !(rs->suiteF && rs->testcaseF))
   {
      rs->descriptionF=0;
      message_report("suite and test case need to be specified", ERROR);
   }
   if(rs->listF & rs->testcaseF)
   {
      message_report("test case argument ignored", WARNING);
      rs->testcaseF = 0; /* maybe unneeded */
   }
}

run_setup *new_run_setup()
{
   run_setup *nsetup = (run_setup *)calloc(1, sizeof(run_setup));
   if(!nsetup)
   {
      message_report("memory allocation failed", FATAL_ERROR);
      exit(-1);
   }
//   nsetup->suite = nsetup->testcase = NULL;
   nsetup->repetitions=1;
   return nsetup;
}

/*returns 1 if arg is a valid argument else it returns 0*/
int is_valid_argument(char *arg)
{
   if(arg)
   {
      if(arg[0]=='-' && arg[1]=='-')
         return 0;
   }
   return 1;
}

/*
 * allocates and initializes the run_setup struct based on the parsed arguments
 * from command line. run_setup contains all information needed to implement
 * the tdriver logic.
 */
run_setup *parse_command(int argc, char *argv[])
{
   int c;
   int option_index = 0;
   run_setup *rs = new_run_setup();

   struct option opts[] =
   {
      /*options with no arguments*/
      {"verbose", no_argument, &(rs->verboseF), 1},
      {"list", no_argument, &(rs->listF), 1},
      {"description", no_argument, &(rs->descriptionF), 1},
      {"all", no_argument, &(rs->allF), 1},

      /*options requiring arguments*/
      {"run", required_argument, &(rs->runF), 1},
      {"suite", required_argument, &(rs->suiteF), 1},
      {"testcase", required_argument, &(rs->testcaseF), 1},
      {"n", required_argument, &(rs->nrepetitionsF), 1},
      {"add", required_argument, &(rs->addsuiteF), 1},
   /*    {"delete",  required_argument, 0, 'd'}, */
      {0, 0, 0, 0} // not sure if this is needed
   };

   while (1){
      option_index = 0;
      c = getopt_long (argc, argv, "", opts, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
         break;
      if(!strcmp(opts[option_index].name, "suite")) /*suite option detected*/
      {
         assert(optarg);
         rs->suite = (char *)malloc(strlen(optarg)+1);
         if(!rs->suite)
         {
            message_report("memory allocation failed", FATAL_ERROR);
            exit(-1);
         }
         if(!is_valid_argument(optarg) )
         {
            printf("option '--suite' requires an argument\n");
            print_usage();
            exit(0);
         }
         strcpy(rs->suite, optarg);
      }
      else if(!strcmp(opts[option_index].name, "run")) /*run option detected*/
      {
         assert(optarg);
         rs->tag = (char *)malloc(strlen(optarg)+1);
         if(!rs->tag)
         {
            message_report("memory allocation failed", FATAL_ERROR);
            exit(-1);
         }
         if(!is_valid_argument(optarg) )
         {
            printf("option '--run' requires an argument\n");
            print_usage();
            exit(0);
         }
         strcpy(rs->tag, optarg);
      }
      else if(!strcmp(opts[option_index].name, "testcase")) /*testcase option detected*/
      {
         assert(optarg);
         rs->testcase = (char *)malloc(strlen(optarg)+1);
         if(!rs->testcase)
         {
            message_report("memory allocation failed", FATAL_ERROR);
            exit(-1);
         }
         if(!is_valid_argument(optarg) )
         {
            printf("option '--testcase' requires an argument\n");
            print_usage();
            exit(0);
         }
         strcpy(rs->testcase, optarg);
      }
      else if(!strcmp(opts[option_index].name, "add")) /**/
      {
         assert(optarg);
         rs->newsuitepath = (char *)malloc(strlen(optarg)+1);
         if(!rs->newsuitepath)
         {
            message_report("memory allocation failed", FATAL_ERROR);
            exit(-1);
         }
         if(!is_valid_argument(optarg) )
         {
            printf("option '--add' requires an argument\n");
            print_usage();
            exit(0);
         }
         strcpy(rs->newsuitepath, optarg);
      }
      else if(!strcmp(opts[option_index].name, "n")) /**/
      {
         char *check;
         assert(optarg);
         rs->repetitions = strtol(optarg, &check, 10);
         if(check==optarg)
         {
            message_report("invalid argument given as number of repetitions, argument will be ignored", WARNING);
            rs->repetitions = 1;
         }
         if(!is_valid_argument(optarg) )
         {
            printf("option '--n' requires an argument\n");
            print_usage();
            exit(0);
         }
      }
      else if('?'==c) /*illegal input (error must have already been reported)*/
      {
         print_usage();
         exit(0);
      }
   }
   semantic_check(rs);
   return rs;
}
