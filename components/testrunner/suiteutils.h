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

#ifndef _SUITEUTILS_H_
#define _SUITEUTILS_H_

#define MAX_SUITE_NAME 32
#define MAX_SUITE_PATH 256

void add_suite(const char *suitename, const char *suitepath);
char *get_suite_path(const char *suitename);
char *get_suite_resultfile(const char *suitename, char *tag);
char *get_testcase_path(const char *suitename, const char *testcasename);
char *get_testcase_script(const char *suitename, const char *testcasename);
char *get_testcase_descfile(const char *suitename, const char *testcasename);
char *get_testcase_metafile(const char *suitename, const char *testcasename);
char *get_testcase_pplib(const char *suitename, const char *testcasename);
char *get_testcase_resultfile(const char *suitename, const char *testcasename, char *tag);
char *get_testcase_top(const char *suitename);
void create_tagfile(const char *suitename, char *tag);
void delete_tagfile(const char *suitename);
FILE *get_suitefile_fp();
#endif
