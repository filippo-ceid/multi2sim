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

#ifndef MEM_SYSTEM_MODULE_H
#define MEM_SYSTEM_MODULE_H

#include <stdio.h>


/* Port */
struct mod_port_t
{
	/* Port lock status */
	int locked;
	long long lock_when;  /* Cycle when it was locked */
	struct mod_stack_t *stack;  /* Access locking port */

	/* Waiting list */
	struct mod_stack_t *waiting_list_head;
	struct mod_stack_t *waiting_list_tail;
	int waiting_list_count;
	int waiting_list_max;
};

/* String map for access type */
extern struct str_map_t mod_access_kind_map;

/* Access type */
enum mod_access_kind_t
{
	mod_access_invalid = 0,
	mod_access_load,
	mod_access_store,
	mod_access_nc_store,
	mod_access_prefetch
};

/* Module types */
enum mod_kind_t
{
	mod_kind_invalid = 0,
	mod_kind_cache,
	mod_kind_main_memory,
	mod_kind_local_memory,
        mod_kind_dram_main_memory // MY CODE
};

/* Any info that clients (cpu/gpu) can pass
 * to the memory system when mod_access() 
 * is called. */
struct mod_client_info_t
{
	/* This field is for use by the prefetcher. It is set
	 * to the PC of the instruction accessing the module */
	unsigned int prefetcher_eip;
};

/* Type of address range */
enum mod_range_kind_t
{
	mod_range_invalid = 0,
	mod_range_bounds,
	mod_range_interleaved
};

#define MOD_ACCESS_HASH_TABLE_SIZE  17
//================== MY CODE ==================//
enum dramcache_type_t
{
      none = 0,
      tag_access_readhit,
      tag_access_writehit,
      tag_access_readmiss,
      tag_access_writemiss,
      read_data_access,
      write_data_access,
      writeback_tag_access,
      writeback_data_access,
      new_block_allocation
};

struct dram_req_list_t
{
   struct mod_stack_t * stack;
   unsigned int address;
   unsigned char iswrite; 
   enum dramcache_type_t access_type;
   long long timeAdded;

   struct dram_req_list_t * prev;
   struct dram_req_list_t * next;
};
struct dramcache_info_list_t 
{
   struct dramcache_info_list_t * next;

   unsigned int addr;
   unsigned long long id;
   unsigned int hit;
};
enum miss_handle_policy_t
{
      normal = 0,
      no_miss_latency,		// assume accessing the off-chip dram at the same time to hide latency
      no_miss_transaction	// assume perfect prediction. don't access dramcache at all.
};
enum dramcache_priority_t
{
      AllEqual = 0,
      ReadFirst
};
struct virtualset_t
{
      int num_ways;
      int num_virtualsets;
      long long * access_cnt;

      long long interval_buckets[1000];
};
//================ END OF MY CODE ===============//

/* Memory module */
struct mod_t
{
	/* Parameters */
	enum mod_kind_t kind;
	char *name;
	int block_size;
	int log_block_size;
	int latency;
	int dir_latency;
	int mshr_size;

	/* Module level starting from entry points */
	int level;

	/* Address range served by module */
	enum mod_range_kind_t range_kind;
	union
	{
		/* For range_kind = mod_range_bounds */
		struct
		{
			unsigned int low;
			unsigned int high;
		} bounds;

		/* For range_kind = mod_range_interleaved */
		struct
		{
			unsigned int mod;
			unsigned int div;
			unsigned int eq;
		} interleaved;
	} range;

	/* Ports */
	struct mod_port_t *ports;
	int num_ports;
	int num_locked_ports;

	/* Accesses waiting to get a port */
	struct mod_stack_t *port_waiting_list_head;
	struct mod_stack_t *port_waiting_list_tail;
	int port_waiting_list_count;
	int port_waiting_list_max;

	/* Directory */
	struct dir_t *dir;
	int dir_size;
	int dir_assoc;
	int dir_num_sets;

	/* Waiting list of events */
	struct mod_stack_t *waiting_list_head;
	struct mod_stack_t *waiting_list_tail;
	int waiting_list_count;
	int waiting_list_max;

	/* Cache structure */
	struct cache_t *cache;

	//========= MY CODE =========//
	/* DRAM */
	void * DRAM;

	int epoch_interval_small;
	int epoch_interval_large;

	int enable_dirtyblock_trace;
	FILE * trace_ptr;
	int enable_dramcache_trace;
	FILE * dramcache_trace_ptr;

	// type of miss_handle_policy
	enum miss_handle_policy_t miss_dramcache_policy;
	enum dramcache_priority_t dramcache_priority;

	struct dram_req_list_t * dram_pending_request_head;
	struct dram_req_list_t * dram_pending_request_tail;
	struct dramcache_info_list_t * dramcache_hit_info;

	struct linked_list_t * dramcache_victim_list;
	struct cache_t * dramcache_victim;

	struct virtualset_t * dramcache_virtualsets;
	
	// interval profiling
	struct cache_t * last_interval_table;
	unsigned int last_interval_table_size;
	long long last_interval_table_conflicts;
	long long total_delta_value;
	long long total_delta_cnt;
		
	/* MY Statistics */
	long long doa_evictions;

	// Statistics for dramcache
	long long dramcache_request_tag_access_readhit;
	long long dramcache_request_tag_access_writehit;
	long long dramcache_request_tag_access_readmiss;
	long long dramcache_request_tag_access_writemiss;
	long long dramcache_request_write_data_access;
	long long dramcache_request_read_data_access;
	long long dramcache_request_writeback_tag_access;
	long long dramcache_request_writeback_data_access;
	long long dramcache_request_writeback_hit;
	long long dramcache_request_writeback_miss;
	long long dramcache_request_new_block_allocation;

	long long dramcache_hit_victim;
	long long dramcache_victim_reference_interval;


	long long dramcache_request_write;
	long long dramcache_request_read;
	long long dramcache_request_write_latency;
	long long dramcache_request_read_latency;


	long long prior_dramcache_request_tag_access_readhit;
	long long prior_dramcache_request_tag_access_writehit;
	long long prior_dramcache_request_tag_access_readmiss;
	long long prior_dramcache_request_tag_access_writemiss;
	long long prior_dramcache_request_write_data_access;
	long long prior_dramcache_request_read_data_access;
	long long prior_dramcache_request_writeback_tag_access;
	long long prior_dramcache_request_writeback_data_access;
	long long prior_dramcache_request_writeback_hit;
	long long prior_dramcache_request_writeback_miss;
	long long prior_dramcache_request_new_block_allocation;

	long long prior_dramcache_request_write;
	long long prior_dramcache_request_read;
	long long prior_dramcache_request_write_latency;
	long long prior_dramcache_request_read_latency;

	// Statistics for off-chip dram
	long long dram_request_write;
	long long dram_request_read;

	// Statistics for dirty blocks
	unsigned int dirty_num_peak;
	unsigned int dirty_num_peak_after_compression_14;
	unsigned int dirty_num_peak_after_compression_28;
	unsigned int dirty_peak_compressed_14;
	unsigned int dirty_peak_compressed_28;
	//====== END OF MY CODE =====//

	/* Low and high memory modules */
	struct linked_list_t *high_mod_list;
	struct linked_list_t *low_mod_list;

	/* Smallest block size of high nodes. When there is no high node, the
	 * sub-block size is equal to the block size. */
	int sub_block_size;
	int num_sub_blocks;  /* block_size / sub_block_size */

	/* Interconnects */
	struct net_t *high_net;
	struct net_t *low_net;
	struct net_node_t *high_net_node;
	struct net_node_t *low_net_node;

	/* Access list */
	struct mod_stack_t *access_list_head;
	struct mod_stack_t *access_list_tail;
	int access_list_count;
	int access_list_max;

	/* Write access list */
	struct mod_stack_t *write_access_list_head;
	struct mod_stack_t *write_access_list_tail;
	int write_access_list_count;
	int write_access_list_max;

	/* Number of in-flight coalesced accesses. This is a number
	 * between 0 and 'access_list_count' at all times. */
	int access_list_coalesced_count;

	/* Clients (CPU/GPU) that use this module can fill in some
	 * optional information in the mod_client_info_t structure.
	 * Using a repos_t memory allocator for these structures. */
	struct repos_t *client_info_repos;

	/* Hash table of accesses */
	struct
	{
		struct mod_stack_t *bucket_list_head;
		struct mod_stack_t *bucket_list_tail;
		int bucket_list_count;
		int bucket_list_max;
	} access_hash_table[MOD_ACCESS_HASH_TABLE_SIZE];

	/* Architecture accessing this module. For versions of Multi2Sim where it is
	 * allowed to have multiple architectures sharing the same subset of the
	 * memory hierarchy, the field is used to check this restriction. */
	struct arch_t *arch;

	/* Statistics */
	long long accesses;
	long long hits;
	long long evict_accesses; 	// MY CODE
	long long writebacks_from_up;	// MY CODE
	long long writebacks_to_down;	// MY CODE

	long long reads;
	long long effective_reads;
	long long effective_read_hits;
	long long writes;
	long long effective_writes;
	long long effective_write_hits;
	long long nc_writes;
	long long effective_nc_writes;
	long long effective_nc_write_hits;
	long long prefetches;
	long long prefetch_aborts;
	long long useless_prefetches;
	long long evictions;

	long long blocking_reads;
	long long non_blocking_reads;
	long long read_hits;
	long long blocking_writes;
	long long non_blocking_writes;
	long long write_hits;
	long long blocking_nc_writes;
	long long non_blocking_nc_writes;
	long long nc_write_hits;

	long long read_retries;
	long long write_retries;
	long long nc_write_retries;

	long long no_retry_accesses;
	long long no_retry_hits;
	long long no_retry_reads;
	long long no_retry_read_hits;
	long long no_retry_writes;
	long long no_retry_write_hits;
	long long no_retry_nc_writes;
	long long no_retry_nc_write_hits;
};

//================== MY CODE ==================//
void mod_dram_free(struct mod_t *mod);
void mod_dram_req_insert(struct mod_t *mod, struct mod_stack_t *stack, 
			 unsigned int addr, unsigned char iswrite,
			 enum dramcache_type_t access_type);
enum dramcache_type_t mod_dram_req_type(struct mod_t *mod, 
                                          unsigned int address, 
                                          unsigned char iswrite);
struct dram_req_list_t * mod_dram_req_find(struct mod_t *mod, 
                                          unsigned int address, 
                                          unsigned char iswrite);
struct mod_stack_t * mod_dram_req_remove(struct mod_t *mod, 
					 unsigned int address, 
					 unsigned char iswrite);
struct mod_t * mod_get_dram_mod(void);
struct mod_t * mod_get_dramcache_mod(void);
void mod_insert_dramcache_info(struct mod_t *mod, unsigned long long id, unsigned int hit, unsigned int addr);
unsigned int mod_get_dramcache_info(struct mod_t *mod, unsigned long long id, unsigned int addr);
//=============== END OF MY CODE ===============//


struct mod_t *mod_create(char *name, enum mod_kind_t kind, int num_ports,
	int block_size, int latency);
void mod_free(struct mod_t *mod);
void mod_dump(struct mod_t *mod, FILE *f);
void mod_stack_set_reply(struct mod_stack_t *stack, int reply);
struct mod_t *mod_stack_set_peer(struct mod_t *peer, int state);

long long mod_access(struct mod_t *mod, enum mod_access_kind_t access_kind, 
	unsigned int addr, int *witness_ptr, struct linked_list_t *event_queue,
	void *event_queue_item, struct mod_client_info_t *client_info);
int mod_can_access(struct mod_t *mod, unsigned int addr);

int mod_find_block(struct mod_t *mod, unsigned int addr, int *set_ptr, int *way_ptr, 
	int *tag_ptr, int *state_ptr);

void mod_block_set_prefetched(struct mod_t *mod, unsigned int addr, int val);
int mod_block_get_prefetched(struct mod_t *mod, unsigned int addr);

void mod_lock_port(struct mod_t *mod, struct mod_stack_t *stack, int event);
void mod_unlock_port(struct mod_t *mod, struct mod_port_t *port,
	struct mod_stack_t *stack);

void mod_access_start(struct mod_t *mod, struct mod_stack_t *stack,
	enum mod_access_kind_t access_kind);
void mod_access_finish(struct mod_t *mod, struct mod_stack_t *stack);

int mod_in_flight_access(struct mod_t *mod, long long id, unsigned int addr);
struct mod_stack_t *mod_in_flight_address(struct mod_t *mod, unsigned int addr,
	struct mod_stack_t *older_than_stack);
struct mod_stack_t *mod_in_flight_write(struct mod_t *mod,
	struct mod_stack_t *older_than_stack);

int mod_serves_address(struct mod_t *mod, unsigned int addr);
struct mod_t *mod_get_low_mod(struct mod_t *mod, unsigned int addr);

int mod_get_retry_latency(struct mod_t *mod);

struct mod_stack_t *mod_can_coalesce(struct mod_t *mod,
	enum mod_access_kind_t access_kind, unsigned int addr,
	struct mod_stack_t *older_than_stack);
void mod_coalesce(struct mod_t *mod, struct mod_stack_t *master_stack,
	struct mod_stack_t *stack);

struct mod_client_info_t *mod_client_info_create(struct mod_t *mod);
void mod_client_info_free(struct mod_t *mod, struct mod_client_info_t *client_info);

#endif

