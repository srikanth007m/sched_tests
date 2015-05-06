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

#ifndef _CMD_PARSER_H_
#define _CMD_PARSER_H_

typedef struct
{
            /* flags */
   int verboseF;
   int listF;
   int runF;
   int allF; /*indicates that the --all option was selected */
   int descriptionF;
   int addsuiteF;
   int suiteF; /*indicates that a suite has been specified*/
   int testcaseF; /*indicates that a test case has been specified*/
   int nrepetitionsF; /*indicates that a number of repetitions were specified*/
         /* arguments */
   unsigned int repetitions;
   char *testcase;
   char *suite;
   char *newsuitepath;
   char *tag;
}run_setup;

run_setup *parse_command(int argc, char *argv[]);
void del_run_setup(run_setup *rs);
void print_usage();
run_setup *new_run_setup();

#endif
