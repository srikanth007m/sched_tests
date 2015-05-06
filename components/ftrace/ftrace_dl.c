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

/* ftrace_dl - framework to call library that will analyze ftrace */

#define _FTRACE_DL_C
#include "./ftrace_dl.h"
#include "./ftrace_dl_api.h"
#include "./ftrace_decode.h"
#include <stdlib.h> /* for NULL */
#include <stdio.h>
#include <string.h>


struct ftrace_dl_libs_chained_struct *
	ftrace_dl_libs_chained = NULL;

#define FTRACE_DL_MAX 16
static int ftrace_dl_nb = 0;
static struct 
{
	void * lib[FTRACE_DL_MAX];
	void * data[FTRACE_DL_MAX];
} ftrace_dl_data;


#ifndef FTRACE_DL_SO
#define RTLD_NOW 0

static const char * dlerror_last = NULL;

const char * dlerror(void)
{
	return dlerror_last;
}

static void * dlopen(const char * name, int unused)
{
	struct ftrace_dl_libs_chained_struct *
		temp = ftrace_dl_libs_chained;
	while (strchr(name, '/') != NULL)
		name = strchr(name, '/') + 1;
	dlerror_last = "";
	while (temp != NULL)
	{
		struct ftrace_dl_libs_chained_struct *
			temp1 = temp;
		const char * c_file = temp->filename;
		temp = temp->next;
		/* remove the path */
		if (strcmp(name, c_file) != 0)
			continue;
		return (void *)temp1;
	}
	dlerror_last = "Not found";
	return NULL;
}

static void * dlsym(void * handle, const char * name)
{
	struct ftrace_dl_libs_chained_struct *
		temp = (struct ftrace_dl_libs_chained_struct *)handle;
	dlerror_last = "NULL handle";
	if (handle == NULL)
		return NULL;
	if (strcmp(name, "extern_ftrace_dl_init") == 0)
		return (void*)(temp->extern_ftrace_dl_init);
	if (strcmp(name, "extern_ftrace_dl_info") == 0)
		return (void*)(temp->extern_ftrace_dl_info);
	if (strcmp(name, "extern_ftrace_dl_cleanup") == 0)
		return (void*)(temp->extern_ftrace_dl_cleanup);
	dlerror_last = "Unknown function";
	return NULL;
}

static void dlclose(void * handle)
{
}

/*
 * Just look for internal libs
 */
struct ftrace_dl_enum
{
	struct ftrace_dl_libs_chained_struct * lib;
};

struct ftrace_dl_enum * ftrace_dl_enum_open(void)
{
	struct ftrace_dl_enum * temp =
		(struct ftrace_dl_enum *)malloc(sizeof(struct ftrace_dl_enum));
	if (temp == NULL)
		return NULL;
	temp->lib = ftrace_dl_libs_chained;
	return temp;
}

bool ftrace_dl_enum_next(struct ftrace_dl_enum * handle, const char ** libname, const char ** info)
{
	if (handle->lib == NULL)
		return false;
	*libname = handle->lib->filename;
	*info = "internal library";
	handle->lib = handle->lib->next;
	return true;
}
extern void ftrace_dl_enum_close(struct ftrace_dl_enum * handle)
{
	free(handle);
}

#else
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * Just look for internal libs
 */
struct ftrace_dl_enum
{
	DIR * dir;
        struct dirent entry[_POSIX_NAME_MAX + 1];
	char name[256];
        struct dirent * result;
};

struct ftrace_dl_enum * ftrace_dl_enum_open(void)
{
	struct ftrace_dl_enum * temp =
		(struct ftrace_dl_enum *)malloc(sizeof(struct ftrace_dl_enum));
	if (temp == NULL)
		return NULL;
	temp->dir = opendir(".");
	if (temp->dir == NULL)
	{
		free(temp);
		return NULL;
	}
	return temp;
}

bool ftrace_dl_enum_next(struct ftrace_dl_enum * handle, const char ** libname, const char ** info)
{
	if (handle->dir == NULL)
		return false;
	while (1)
	{
		void * lib;
		if ((readdir_r(handle->dir, &handle->entry[0], &handle->result) != 0) || (handle->result == NULL))
		{
			closedir(handle->dir);
			handle->dir = NULL;
			return false;
		}

		snprintf(handle->name, 255, "./%s", handle->result->d_name);
		handle->name[255] = 0;
		/* check if the lib is loadable and has the right function before saying it's right */
		lib = dlopen(handle->name, RTLD_NOW);
		if (lib == NULL)
			continue;
		do
		{
			if (dlsym(lib, "extern_ftrace_dl_init") == NULL)
				break;
			if (dlsym(lib, "extern_ftrace_dl_cleanup") == NULL)
				break;
			if (dlsym(lib, "extern_ftrace_dl_info") == NULL)
				break;
			dlclose(lib);
			*libname = handle->name;
			*info = "external shared lib";
			return true;
		} while (0);
		dlclose(lib);
	}
	return false;;
}
extern void ftrace_dl_enum_close(struct ftrace_dl_enum * handle)
{
	if (handle->dir != NULL)
		closedir(handle->dir);
	free(handle);
}

#endif

void * ftrace_dl_load(const char * name)
{
	int i;
	void * lib = dlopen(name, RTLD_NOW);
	void * (*ftrace_dl_init)(void);
	if (lib == NULL)
	{
		char name2[256];
		/* second try, add the prefix "./" */
		snprintf(name2, 255, "./%s", name2);
		name2[255] = 0;
		lib = dlopen(name, RTLD_NOW);
	}
        if (lib == NULL)
	{
		printf("# Can't load <%s> error %s\n", name, dlerror());
                return NULL;
	}
	for (i = 0; i < ftrace_dl_nb; i++)
		if (ftrace_dl_data.lib[i] == NULL)
			break;
	if (i == FTRACE_DL_MAX)
	{
		dlclose(lib);
		return NULL;
	}
	if (i == ftrace_dl_nb)
		ftrace_dl_nb++;
	
	ftrace_dl_data.lib[i] = lib;


	ftrace_dl_init = (void * (*)(void))dlsym(lib, "extern_ftrace_dl_init");
	if (ftrace_dl_init == NULL)
	{
		dlclose(lib);
		ftrace_dl_data.lib[i] = NULL;
		return NULL;
	}
	ftrace_dl_data.data[i] = ftrace_dl_init();
	return lib;
}

char * ftrace_dl_getinfo(const char * name)
{
	char * info;
	char * (*ftrace_dl_info)(void);
	void * lib = dlopen(name, RTLD_NOW);
	if (lib == NULL)
	{
		char name2[256];
		/* second try, add the prefix "./" */
		snprintf(name2, 255, "./%s", name2);
		name2[255] = 0;
		lib = dlopen(name, RTLD_NOW);
	}		
        if (lib == NULL)
                return strdup("Unknown lib");
	ftrace_dl_info = (char * (*)(void))dlsym(lib, "extern_ftrace_dl_info");
	if (ftrace_dl_info == NULL)
	{
		dlclose(lib);
		return strdup("No info function");
	}

	info = ftrace_dl_info();
	if (info == NULL)
		info = strdup("No info");
	else
		info = strdup(info);
	dlclose(lib);
	return info;
}

bool ftrace_dl_unload(void * handle)
{
	int i;
	bool ret = false;
	bool (*ftrace_dl_cleanup)(void *);
	if (handle == NULL)
		return false;
	for (i = 0;  i < ftrace_dl_nb; i++)
		if (ftrace_dl_data.lib[i] == handle)
			break;
	if (i == ftrace_dl_nb)
		return false;
	ftrace_dl_cleanup = (bool (*)(void*))dlsym(handle, "extern_ftrace_dl_cleanup");
	if (ftrace_dl_cleanup != NULL)
		ret = ftrace_dl_cleanup(ftrace_dl_data.data[i]);
	dlclose(handle);
	ftrace_dl_data.lib[i] = NULL;
	return ret;
}

