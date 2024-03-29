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

#include <assert.h>
#include <limits.h>

#include <arch/southern-islands/emu/ndrange.h>
#include <arch/southern-islands/emu/wavefront.h>
#include <arch/southern-islands/emu/work-group.h>
#include <lib/esim/esim.h>
#include <lib/esim/trace.h>
#include <lib/util/list.h>

#include "compute-unit.h"
#include "gpu.h"
#include "scalar-unit.h"
#include "uop.h"
#include "wavefront-pool.h"


/* Configurable by user at runtime */

int si_gpu_scalar_unit_width = 1;
int si_gpu_scalar_unit_issue_buffer_size = 4;

/*
 * Register accesses are not pipelined, so buffer size is not
 * multiplied by the latency.
 */
int si_gpu_scalar_unit_read_latency = 1;
int si_gpu_scalar_unit_read_buffer_size = 1;

int si_gpu_scalar_unit_exec_latency = 4;
int si_gpu_scalar_unit_inflight_mem_accesses = 32;

void si_scalar_unit_process_mem_accesses(struct si_scalar_unit_t *scalar_unit)
{
	struct si_uop_t *uop = NULL;
	int i;
	int list_entries;
	int list_index = 0;

	/* Process completed memory instructions */
	list_entries = list_count(scalar_unit->inflight_buffer);

	/* Sanity check the exec buffer */
	assert(list_entries <= si_gpu_scalar_unit_inflight_mem_accesses);

	for (i = 0; i < list_entries; i++)
	{
		uop = list_get(scalar_unit->inflight_buffer, list_index);
		assert(uop);

		if (!uop->global_mem_witness)
		{

			assert(uop->wavefront_pool_entry->lgkm_cnt > 0);
			uop->wavefront_pool_entry->lgkm_cnt--;

			/* Access complete, remove the uop from the queue */
			list_remove(scalar_unit->inflight_buffer, uop);

			/* Free uop */
			si_uop_free(uop);

			si_gpu->last_complete_cycle = esim_cycle;
		}
		else
		{
			list_index++;
		}
	}
}

void si_scalar_unit_writeback(struct si_scalar_unit_t *scalar_unit)
{
	struct si_uop_t *uop = NULL;
	int i;
	int list_entries;
	int list_index = 0;
	unsigned int complete;
	struct si_ndrange_t *ndrange = si_gpu->ndrange;
	struct si_work_group_t *work_group;
	struct si_wavefront_t *wavefront;
	int wavefront_id;

	/* Process completed memory instructions */
	list_entries = list_count(scalar_unit->exec_buffer);

	/* Sanity check the exec buffer */
	assert(list_entries <= si_gpu_scalar_unit_exec_latency * si_gpu_scalar_unit_width);
	
	for (i = 0; i < list_entries; i++)
	{
		uop = list_get(scalar_unit->exec_buffer, list_index);
		assert(uop);

		if (uop->execute_ready <= si_gpu->cycle)
		{
			/* If this is the last instruction and there are outstanding
			 * memory operations, wait for them to complete */
			if (uop->wavefront_last_inst && 
				(uop->wavefront_pool_entry->lgkm_cnt || 
				 uop->wavefront_pool_entry->vm_cnt || 
				 uop->wavefront_pool_entry->exp_cnt)) 
			{
				si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"s\"\n", 
					uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
					uop->wavefront->id, uop->id_in_wavefront);

				list_index++;
				continue;
			}

			/* Access complete, remove the uop from the queue */
			list_remove(scalar_unit->exec_buffer, uop);

			/* Instruction is complete */
			uop->wavefront_pool_entry->ready = 1;
			uop->wavefront_pool_entry->uop = NULL;
			uop->wavefront_pool_entry->cycle_fetched = INST_NOT_FETCHED;

			/* Check for "wait" instruction */
			if (uop->mem_wait_inst)
			{
				/* If a wait instruction was executed and there are 
				 * outstanding memory accesses, set the wavefront to waiting */
				uop->wavefront_pool_entry->wait_for_mem = 1;
			}

			/* Check for "barrier" instruction */
			if (uop->barrier_wait_inst)
			{
				/* Set a flag to wait until all wavefronts have reached the
				 * barrier */
				uop->wavefront_pool_entry->wait_for_barrier = 1;

				/* Check if all wavefronts have reached the barrier */
				complete = 1;
				work_group = uop->wavefront->work_group;
				SI_FOREACH_WAVEFRONT_IN_WORK_GROUP(work_group, wavefront_id)
				{
					wavefront = ndrange->wavefronts[wavefront_id];
					
					if (!wavefront->wavefront_pool_entry->wait_for_barrier)
						complete = 0;
				}

				/* If all wavefronts have reached the barrier, clear their flags */
				if (complete)
				{
					SI_FOREACH_WAVEFRONT_IN_WORK_GROUP(work_group, wavefront_id)
					{
						wavefront = ndrange->wavefronts[wavefront_id];
						wavefront->wavefront_pool_entry->wait_for_barrier = 0;
					}
				}
			}

			/* Check for "end" instruction */
			if (uop->wavefront_last_inst)
			{
				/* If the uop completes the wavefront, set a bit so that the 
				 * hardware wont try to fetch any more instructions for it */
				uop->work_group->compute_unit_finished_count++;
				uop->wavefront_pool_entry->wavefront_finished = 1;

				/* Check if wavefront finishes a work-group */
				assert(uop->work_group);
				assert(uop->work_group->compute_unit_finished_count <=
					uop->work_group->wavefront_count);
				if (uop->work_group->compute_unit_finished_count == 
						uop->work_group->wavefront_count)
				{
					si_compute_unit_unmap_work_group(scalar_unit->compute_unit,
						uop->work_group);
				}
			}


			si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"su-w\"\n", 
				uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
				uop->wavefront->id, uop->id_in_wavefront);

			/* Free uop */
			if (si_tracing())
				si_gpu_uop_trash_add(uop);
			else
				si_uop_free(uop);

			/* Statistics */
			scalar_unit->inst_count++;
			si_gpu->last_complete_cycle = esim_cycle;
		}
		else
		{
            /* Uop is not ready yet */
			list_index++;
            continue;
		}
	}
}

void si_scalar_unit_execute(struct si_scalar_unit_t *scalar_unit)
{
	struct si_uop_t *uop;
	int list_entries;
	int list_index = 0;
	int instructions_processed = 0;
	int i;

	list_entries = list_count(scalar_unit->read_buffer);

	/* Sanity check the read buffer.  Register accesses are not pipelined, so
	 * buffer size is not multiplied by the latency. */
	assert(list_entries <= si_gpu_scalar_unit_width);

	for (i = 0; i < list_entries; i++)
	{
		uop = list_get(scalar_unit->read_buffer, list_index);
		assert(uop);

		/* Uop is not ready yet */
		if (si_gpu->cycle < uop->read_ready)
		{
			list_index++;
			continue;
		}

		/* Stall if the issue width has been reached */
		if (instructions_processed == si_gpu_scalar_unit_width)
		{
			si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"s\"\n", 
				uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
				uop->wavefront->id, uop->id_in_wavefront);
			list_index++;
			continue;
		}

		/* Memory instructions require another step */
		if (uop->scalar_mem_read)
		{
			/* Sanity check in-flight buffer */
			assert(list_count(scalar_unit->inflight_buffer) <= 
				si_gpu_scalar_unit_inflight_mem_accesses);

			/* Sanity check exec buffer */
			assert(list_count(scalar_unit->exec_buffer) <= si_gpu_scalar_unit_width * 
				si_gpu_scalar_unit_exec_latency);

			/* If there is room in the outstanding memory buffer, issue the access */
			if ((list_count(scalar_unit->inflight_buffer) < 
				si_gpu_scalar_unit_inflight_mem_accesses) &&
				list_count(scalar_unit->exec_buffer) < si_gpu_scalar_unit_width * 
				si_gpu_scalar_unit_exec_latency)
			{
				/* TODO replace this with a lightweight uop */
				struct si_uop_t *mem_uop;
				mem_uop = si_uop_create();
				mem_uop->wavefront = uop->wavefront;
				mem_uop->compute_unit = uop->compute_unit;
				mem_uop->wavefront_pool_entry = uop->wavefront_pool_entry;
				mem_uop ->scalar_mem_read = uop->scalar_mem_read;
				mem_uop->id_in_compute_unit = 
					uop->compute_unit->mem_uop_id_counter++;

				/* Access global memory */
				mem_uop->global_mem_witness--;
				/* FIXME Get rid of dependence on wavefront here */
				mem_uop->global_mem_access_addr =
					uop->wavefront->scalar_work_item->global_mem_access_addr;
				mod_access(scalar_unit->compute_unit->global_memory,
					mod_access_load, uop->global_mem_access_addr,
					&mem_uop->global_mem_witness, NULL, NULL, NULL);

				/* Increment outstanding memory access count */
				mem_uop->wavefront_pool_entry->lgkm_cnt++;

				/* Transfer the uop to the exec buffer */
				uop->execute_ready = si_gpu->cycle + 1;
				list_remove(scalar_unit->read_buffer, uop);
				list_enqueue(scalar_unit->exec_buffer, uop);

				/* Add a mem_uop to the in-flight buffer */
				list_enqueue(scalar_unit->inflight_buffer, mem_uop);

				instructions_processed++;

				si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"su-e\"\n", 
					uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
					uop->wavefront->id, uop->id_in_wavefront);
			}
			else 
			{
				si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"s\"\n", 
					uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
					uop->wavefront->id, uop->id_in_wavefront);
				list_index++;
				continue;
			}

		}
		/* ALU Instruction */
		else
		{
			/* Sanity check exec buffer */
			assert(list_count(scalar_unit->exec_buffer) <= si_gpu_scalar_unit_width * 
				si_gpu_scalar_unit_exec_latency);

			if (list_count(scalar_unit->exec_buffer) == si_gpu_scalar_unit_width * 
				si_gpu_scalar_unit_exec_latency)
			{
				si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"s\"\n", 
					uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
					uop->wavefront->id, uop->id_in_wavefront);
				list_index++;
				continue;
			}

			uop->execute_ready = si_gpu->cycle + si_gpu_scalar_unit_exec_latency;

			/* Transfer the uop to the outstanding execution buffer */
			list_remove(scalar_unit->read_buffer, uop);
			list_enqueue(scalar_unit->exec_buffer, uop);

			instructions_processed++;
			si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"su-e\"\n", 
				uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
				uop->wavefront->id, uop->id_in_wavefront);
		}
	}
}

void si_scalar_unit_read(struct si_scalar_unit_t *scalar_unit)
{
	struct si_uop_t *uop;
	int instructions_processed = 0;
	int list_entries;
	int list_index = 0;
	int i;

	list_entries = list_count(scalar_unit->issue_buffer);

	/* Sanity check the decode buffer */
	assert(list_entries <= si_gpu_scalar_unit_issue_buffer_size);

	for (i = 0; i < list_entries; i++)
	{
		uop = list_get(scalar_unit->issue_buffer, list_index);
		assert(uop);

        /* Uop is not ready yet */
		if (si_gpu->cycle < uop->issue_ready)
		{
			list_index++;
			continue;
		}

		/* Stall if the issue width has been reached */
		if (instructions_processed == si_gpu_scalar_unit_width)
		{
			si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"s\"\n", 
				uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
				uop->wavefront->id, uop->id_in_wavefront);
			list_index++;
			continue;
		}

		assert(list_count(scalar_unit->read_buffer) <= 
			si_gpu_scalar_unit_read_buffer_size);

		/* Stall if the read buffer is full */
		if (list_count(scalar_unit->read_buffer) == si_gpu_scalar_unit_read_buffer_size)
		{
			si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"s\"\n", 
				uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
				uop->wavefront->id, uop->id_in_wavefront);
			list_index++;
			continue;
		}

		uop->read_ready = si_gpu->cycle + si_gpu_scalar_unit_read_latency;
		list_remove(scalar_unit->issue_buffer, uop);
		list_enqueue(scalar_unit->read_buffer, uop);

		instructions_processed++;

		si_trace("si.inst id=%lld cu=%d wf=%d uop_id=%lld stg=\"su-r\"\n", 
			uop->id_in_compute_unit, scalar_unit->compute_unit->id, 
			uop->wavefront->id, uop->id_in_wavefront);
	}
}

void si_scalar_unit_run(struct si_scalar_unit_t *scalar_unit)
{
	si_scalar_unit_process_mem_accesses(scalar_unit);
	si_scalar_unit_writeback(scalar_unit);
	si_scalar_unit_execute(scalar_unit);
	si_scalar_unit_read(scalar_unit);
}

