/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <lib/mhandle/mhandle.h>

#include "cuda-function-arg.h"


struct frm_cuda_function_arg_t *frm_cuda_function_arg_create(char *name)
{
	struct frm_cuda_function_arg_t *arg;

	/* Initialize */
	arg = xcalloc(1, sizeof(struct frm_cuda_function_arg_t) + strlen(name) + 1);
	strncpy(arg->name, name, MAX_STRING_SIZE);

	/* Return */
	return arg;
}


void frm_cuda_function_arg_free(struct frm_cuda_function_arg_t *arg)
{
	free(arg);
}

