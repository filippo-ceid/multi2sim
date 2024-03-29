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

#ifndef ARCH_SOUTHERN_ISLANDS_EMU_OPENCL_COMMAND_QUEUE_H
#define ARCH_SOUTHERN_ISLANDS_EMU_OPENCL_COMMAND_QUEUE_H


/* Forward declaration of x86 context used below in callback function. This
 * dependence would be removed if OpenCL API implementation was in 'arch/x86/emu'
 * instead. Is this a better option? */
struct x86_ctx_t;

enum si_opencl_command_type_t
{
	si_opencl_command_queue_task_invalid,
	si_opencl_command_queue_task_read_buffer,
	si_opencl_command_queue_task_write_buffer,
	si_opencl_command_queue_task_ndrange_kernel
};

struct si_opencl_command_t
{
	enum si_opencl_command_type_t type;
	union
	{
		struct
		{
			struct si_ndrange_t *ndrange;
		} ndrange_kernel;
	} u;
};

struct si_opencl_command_queue_t
{
	unsigned int id;
	int ref_count;

	unsigned int device_id;
	unsigned int context_id;
	unsigned int properties;

	struct linked_list_t *command_list;
};

struct si_opencl_command_queue_t *si_opencl_command_queue_create(void);
void si_opencl_command_queue_free(struct si_opencl_command_queue_t *command_queue);

struct si_opencl_command_t *si_opencl_command_create(enum
	si_opencl_command_type_t type);
void si_opencl_command_free(struct si_opencl_command_t *command);

void si_opencl_command_queue_submit(struct si_opencl_command_queue_t *command_queue,
	struct si_opencl_command_t *command);
void si_opencl_command_queue_complete(struct si_opencl_command_queue_t *command_queue,
	struct si_opencl_command_t *command);

/* Callback function of type 'x86_ctx_wakeup_callback_func_t'.
 * Argument 'data' is type-casted to 'struct si_opencl_command_queue_t' */
int si_opencl_command_queue_can_wakeup(struct x86_ctx_t *ctx, void *data);

#endif
