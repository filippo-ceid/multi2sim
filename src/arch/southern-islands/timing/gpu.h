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

#ifndef ARCH_SOUTHERN_ISLANDS_TIMING_GPU_H
#define ARCH_SOUTHERN_ISLANDS_TIMING_GPU_H


/* Trace */
#define si_tracing() trace_status(si_trace_category)
#define si_trace(...) trace(si_trace_category, __VA_ARGS__)
#define si_trace_header(...) trace_header(si_trace_category, __VA_ARGS__)
extern int si_trace_category;

extern char *si_gpu_config_help;
extern char *si_gpu_config_file_name;
extern char *si_gpu_report_file_name;

/* OpenCL Device Info */
extern unsigned long long si_gpu_device_type;
extern unsigned int si_gpu_device_vendor_id;

extern char *si_gpu_device_profile;
extern char *si_gpu_device_name;
extern char *si_gpu_device_vendor;
extern char *si_gpu_device_extensions;
extern char *si_gpu_device_version;
extern char *si_gpu_driver_version;
extern char *si_gpu_opencl_version;

extern unsigned int si_gpu_work_group_size;
extern unsigned int si_gpu_work_item_dimensions;
extern unsigned int si_gpu_work_item_sizes[3];

extern unsigned int si_gpu_image_support;
extern unsigned int si_gpu_max_read_image_args;
extern unsigned int si_gpu_max_write_image_args;

extern unsigned int si_gpu_image2d_max_width;
extern unsigned int si_gpu_image2d_max_height;
extern unsigned int si_gpu_image3d_max_width;
extern unsigned int si_gpu_image3d_max_height;
extern unsigned int si_gpu_image3d_max_depth;
extern unsigned int si_gpu_max_samplers;

extern unsigned int si_gpu_max_parameter_size;
extern unsigned int si_gpu_mem_base_addr_align;
extern unsigned int si_gpu_min_data_type_align_size;

extern unsigned int si_gpu_single_fp_config;
extern unsigned int si_gpu_double_fp_config;

extern unsigned int si_gpu_max_clock_frequency;
extern unsigned int si_gpu_address_bits;

extern unsigned int si_gpu_global_mem_cache_type;
extern unsigned int si_gpu_global_mem_cacheline_size;
extern unsigned long long si_gpu_global_mem_cache_size;
extern unsigned long long si_gpu_global_mem_size;
extern unsigned long long si_gpu_max_mem_alloc_size;

extern unsigned int si_gpu_local_mem_type;

extern unsigned long long si_gpu_max_constant_buffer_size;
extern unsigned int si_gpu_max_constant_args;

extern unsigned int si_gpu_vector_width_half;

extern unsigned int si_gpu_error_correction_support;
extern unsigned int si_gpu_host_unified_memory;
extern unsigned int si_gpu_profiling_timer_resolution;
extern unsigned int si_gpu_endian_little;

extern unsigned int si_gpu_device_available;
extern unsigned int si_gpu_compiler_available;
extern unsigned int si_gpu_linker_available;
extern unsigned int si_gpu_execution_capabilities;

extern unsigned int si_gpu_queue_properties;

extern unsigned int si_gpu_platform;

extern unsigned int si_gpu_num_compute_units;
extern unsigned int si_gpu_num_registers;
extern unsigned int si_gpu_num_wavefront_pools;
extern unsigned int si_gpu_num_stream_cores;
extern unsigned int si_gpu_register_alloc_size;

extern struct str_map_t si_gpu_register_alloc_granularity_map;
extern enum si_gpu_register_alloc_granularity_t
{
	si_gpu_register_alloc_invalid = 0,  /* For invalid user input */
	si_gpu_register_alloc_wavefront,
	si_gpu_register_alloc_work_group
} si_gpu_register_alloc_granularity;

extern int si_gpu_max_work_groups_per_wavefront_pool;
extern int si_gpu_max_wavefronts_per_wavefront_pool;

extern char *si_gpu_calc_file_name;

extern int si_gpu_fetch_latency;
extern int si_gpu_fetch_width;
extern int si_gpu_fetch_buffer_size;

extern int si_gpu_decode_latency;
extern int si_gpu_decode_width;
extern int si_gpu_decode_buffer_size;

extern int si_gpu_issue_latency;
extern int si_gpu_issue_width;

extern int si_gpu_local_mem_size;
extern int si_gpu_local_mem_alloc_size;
extern int si_gpu_local_mem_latency;
extern int si_gpu_local_mem_block_size;
extern int si_gpu_local_mem_num_ports;

extern int si_gpu_simd_width;
extern int si_gpu_simd_issue_buffer_size;
extern int si_gpu_simd_read_latency;
extern int si_gpu_simd_read_buffer_size;
extern int si_gpu_simd_alu_latency;
extern int si_gpu_simd_num_subwavefronts;

extern int si_gpu_vector_mem_width;
extern int si_gpu_vector_mem_issue_buffer_size;
extern int si_gpu_vector_mem_read_latency;
extern int si_gpu_vector_mem_read_buffer_size;
extern int si_gpu_vector_mem_inflight_mem_accesses;

extern int si_gpu_lds_width;
extern int si_gpu_lds_issue_buffer_size;
extern int si_gpu_lds_read_latency;
extern int si_gpu_lds_read_buffer_size;
extern int si_gpu_lds_inflight_mem_accesses;

extern int si_gpu_scalar_unit_width;
extern int si_gpu_scalar_unit_issue_buffer_size;
extern int si_gpu_scalar_unit_read_latency;
extern int si_gpu_scalar_unit_read_buffer_size;
extern int si_gpu_scalar_unit_exec_latency;
extern int si_gpu_scalar_unit_inflight_mem_accesses;

extern int si_gpu_branch_unit_width;
extern int si_gpu_branch_unit_issue_buffer_size;
extern int si_gpu_branch_unit_read_latency;
extern int si_gpu_branch_unit_read_buffer_size;
extern int si_gpu_branch_unit_exec_latency;

struct si_gpu_t
{
	/* Current cycle */
	long long cycle;

	/* ND-Range running on it */
	struct si_ndrange_t *ndrange;
	int work_groups_per_wavefront_pool;
	int wavefronts_per_wavefront_pool;
	int work_items_per_wavefront_pool;
	int work_groups_per_compute_unit;
	int wavefronts_per_compute_unit;
	int work_items_per_compute_unit;

	/* Compute units */
	struct si_compute_unit_t **compute_units;

	/* List of ready compute units accepting work-groups */
	struct si_compute_unit_t *compute_unit_ready_list_head;
	struct si_compute_unit_t *compute_unit_ready_list_tail;
	int compute_unit_ready_list_count;
	int compute_unit_ready_list_max;

	/* List of busy compute units */
	struct si_compute_unit_t *compute_unit_busy_list_head;
	struct si_compute_unit_t *compute_unit_busy_list_tail;
	int compute_unit_busy_list_count;
	int compute_unit_busy_list_max;

	/* List of deleted instructions */
	struct linked_list_t *trash_uop_list;

	long long int last_complete_cycle;
};

extern struct si_gpu_t *si_gpu;

#define SI_GPU_FOREACH_COMPUTE_UNIT(COMPUTE_UNIT_ID) \
	for ((COMPUTE_UNIT_ID) = 0; (COMPUTE_UNIT_ID) < si_gpu_num_compute_units; (COMPUTE_UNIT_ID)++)

#define SI_GPU_FOREACH_WORK_ITEM_IN_SUBWAVEFRONT(WAVEFRONT, SUBWAVEFRONT_ID, WORK_ITEM_ID) \
	for ((WORK_ITEM_ID) = (WAVEFRONT)->work_item_id_first + (SUBWAVEFRONT_ID) * si_gpu_num_stream_cores; \
		(WORK_ITEM_ID) <= MIN((WAVEFRONT)->work_item_id_first + ((SUBWAVEFRONT_ID) + 1) \
			* si_gpu_num_stream_cores - 1, (WAVEFRONT)->work_item_id_last); \
		(WORK_ITEM_ID)++)

/* Forward declaration */
struct si_uop_t;
struct si_compute_unit_t;
struct si_scalar_unit_t;
struct si_simd_t;
struct si_branch_unit_t;
struct si_vector_mem_unit_t;
struct si_lds_t;

void si_gpu_init(void);
void si_gpu_done(void);

void si_gpu_dump_report(void);
void si_gpu_dump_summary(FILE *f);

void si_gpu_uop_trash_add(struct si_uop_t *uop);
void si_gpu_uop_trash_empty(void);

void si_compute_unit_run_simd(struct si_compute_unit_t *compute_unit);
void si_compute_unit_run_scalar_unit(struct si_compute_unit_t *compute_unit);
void si_compute_unit_run_branch_unit(struct si_compute_unit_t *compute_unit);
int si_gpu_run(void);

void si_simd_run(struct si_simd_t *simd);
void si_scalar_unit_run(struct si_scalar_unit_t *scalar_unit);
void si_branch_unit_run(struct si_branch_unit_t *branch_unit);
void si_vector_mem_run(struct si_vector_mem_unit_t *vector_mem);
void si_lds_run(struct si_lds_t *lds);

#endif
