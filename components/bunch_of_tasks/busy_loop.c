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

#include "busy_loop.h"
#include <math.h>
#include <assert.h>
#include <stdlib.h>


struct busy_loop_data
{
	double a;
	double b;
	long long nbloop;
};


struct busy_loop_data * busy_loop_create(long long nbloop)
{
	struct busy_loop_data * data = (struct busy_loop_data *)
			malloc(sizeof(struct busy_loop_data));
	if (data == NULL)
		return NULL;
	data->a = -0.2;
	data->b = 0.01;
	data->nbloop = nbloop;
	return data;
}

bool busy_loop_run(struct busy_loop_data * data)
{
	/* -0.2+0.01i is a complex number in mandelbrot set
	 * so, we will never exit the while before nbloop
	 * but the compiler doesn't know that and so, it can't
	 * remove the loop :-) */

	int i = 0;
	while (i < data->nbloop)
	{
		double a1 = data->a * data->a - data->b * data->b - 0.2;
		double b1 = 2 * data->a * data->b + 0.01;
		data->a = a1;
		data->b = b1;
		if (data->a * data->a + data->b * data->b > 4)
			return false;
		i++;
	}
	return true;
}

void busy_loop_destroy(struct busy_loop_data * data)
{
	if (data == NULL)
		return;
	free(data);
}
