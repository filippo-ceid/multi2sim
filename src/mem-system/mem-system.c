/*
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


#include <lib/esim/esim.h>
#include <lib/esim/trace.h>
#include <lib/mhandle/mhandle.h>
#include <lib/util/debug.h>
#include <lib/util/file.h>
#include <lib/util/list.h>
#include <lib/util/string.h>
#include <network/network.h>
#include <assert.h>
#include "cache.h"
#include "command.h"
#include "config.h"
#include "local-mem-protocol.h"
#include "mem-system.h"
#include "module.h"
#include "nmoesi-protocol.h"

// MY CODE
#include "dramsim2_wrapper.h"

/*
 * Global Variables
 */

int mem_debug_category;
int mem_trace_category;
int mem_system_peer_transfers;

struct mem_system_t *mem_system;

char *mem_report_file_name = "";




/*
 * Public Functions
 */

void mem_system_init(void)
{
	/* Trace */
	mem_trace_category = trace_new_category();

	/* Try to open report file */
	if (*mem_report_file_name && !file_can_open_for_write(mem_report_file_name))
		fatal("%s: cannot open GPU cache report file",
			mem_report_file_name);

	/* Initialize */
	mem_system = xcalloc(1, sizeof(struct mem_system_t));
	mem_system->net_list = list_create();
	mem_system->mod_list = list_create();

	/* Event handler for memory hierarchy commands */
	EV_MEM_SYSTEM_COMMAND = esim_register_event_with_name(mem_system_command_handler, "mem_system_command");
	EV_MEM_SYSTEM_END_COMMAND = esim_register_event_with_name(mem_system_end_command_handler, "mem_system_end_command");

	/* NMOESI memory event-driven simulation */

	EV_MOD_NMOESI_LOAD = esim_register_event_with_name(mod_handler_nmoesi_load, "mod_nmoesi_load");
	EV_MOD_NMOESI_LOAD_LOCK = esim_register_event_with_name(mod_handler_nmoesi_load, "mod_nmoesi_load_lock");
	EV_MOD_NMOESI_LOAD_ACTION = esim_register_event_with_name(mod_handler_nmoesi_load, "mod_nmoesi_load_action");
	EV_MOD_NMOESI_LOAD_MISS = esim_register_event_with_name(mod_handler_nmoesi_load, "mod_nmoesi_load_miss");
	EV_MOD_NMOESI_LOAD_UNLOCK = esim_register_event_with_name(mod_handler_nmoesi_load, "mod_nmoesi_load_unlock");
	EV_MOD_NMOESI_LOAD_FINISH = esim_register_event_with_name(mod_handler_nmoesi_load, "mod_nmoesi_load_finish");

	EV_MOD_NMOESI_STORE = esim_register_event_with_name(mod_handler_nmoesi_store, "mod_nmoesi_store");
	EV_MOD_NMOESI_STORE_LOCK = esim_register_event_with_name(mod_handler_nmoesi_store, "mod_nmoesi_store_lock");
	EV_MOD_NMOESI_STORE_ACTION = esim_register_event_with_name(mod_handler_nmoesi_store, "mod_nmoesi_store_action");
	EV_MOD_NMOESI_STORE_UNLOCK = esim_register_event_with_name(mod_handler_nmoesi_store, "mod_nmoesi_store_unlock");
	EV_MOD_NMOESI_STORE_FINISH = esim_register_event_with_name(mod_handler_nmoesi_store, "mod_nmoesi_store_finish");
	
	EV_MOD_NMOESI_NC_STORE = esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store");
	EV_MOD_NMOESI_NC_STORE_LOCK = esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store_lock");
	EV_MOD_NMOESI_NC_STORE_WRITEBACK = esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store_writeback");
	EV_MOD_NMOESI_NC_STORE_ACTION = esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store_action");
	EV_MOD_NMOESI_NC_STORE_MISS= esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store_miss");
	EV_MOD_NMOESI_NC_STORE_UNLOCK = esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store_unlock");
	EV_MOD_NMOESI_NC_STORE_FINISH = esim_register_event_with_name(mod_handler_nmoesi_nc_store, "mod_nmoesi_nc_store_finish");

	EV_MOD_NMOESI_PREFETCH = esim_register_event_with_name(mod_handler_nmoesi_prefetch, "mod_nmoesi_prefetch");
	EV_MOD_NMOESI_PREFETCH_LOCK = esim_register_event_with_name(mod_handler_nmoesi_prefetch, "mod_nmoesi_prefetch_lock");
	EV_MOD_NMOESI_PREFETCH_ACTION = esim_register_event_with_name(mod_handler_nmoesi_prefetch, "mod_nmoesi_prefetch_action");
	EV_MOD_NMOESI_PREFETCH_MISS = esim_register_event_with_name(mod_handler_nmoesi_prefetch, "mod_nmoesi_prefetch_miss");
	EV_MOD_NMOESI_PREFETCH_UNLOCK = esim_register_event_with_name(mod_handler_nmoesi_prefetch, "mod_nmoesi_prefetch_unlock");
	EV_MOD_NMOESI_PREFETCH_FINISH = esim_register_event_with_name(mod_handler_nmoesi_prefetch, "mod_nmoesi_prefetch_finish");

	EV_MOD_NMOESI_FIND_AND_LOCK = esim_register_event_with_name(mod_handler_nmoesi_find_and_lock, "mod_nmoesi_find_and_lock");
	EV_MOD_NMOESI_FIND_AND_LOCK_PORT = esim_register_event_with_name(mod_handler_nmoesi_find_and_lock, "mod_nmoesi_find_and_lock_port");
	EV_MOD_NMOESI_FIND_AND_LOCK_ACTION = esim_register_event_with_name(mod_handler_nmoesi_find_and_lock, "mod_nmoesi_find_and_lock_action");
	EV_MOD_NMOESI_FIND_AND_LOCK_FINISH = esim_register_event_with_name(mod_handler_nmoesi_find_and_lock, "mod_nmoesi_find_and_lock_finish");

	EV_MOD_NMOESI_EVICT = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict");
	EV_MOD_NMOESI_EVICT_INVALID = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_invalid");
	EV_MOD_NMOESI_EVICT_ACTION = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_action");
	EV_MOD_NMOESI_EVICT_RECEIVE = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_receive");
	EV_MOD_NMOESI_EVICT_PROCESS = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_process");
	EV_MOD_NMOESI_EVICT_PROCESS_NONCOHERENT = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_process_noncoherent");
	EV_MOD_NMOESI_EVICT_REPLY = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_reply");
	EV_MOD_NMOESI_EVICT_REPLY = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_reply");
	EV_MOD_NMOESI_EVICT_REPLY_RECEIVE = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_reply_receive");
	EV_MOD_NMOESI_EVICT_FINISH = esim_register_event_with_name(mod_handler_nmoesi_evict, "mod_nmoesi_evict_finish");

	EV_MOD_NMOESI_WRITE_REQUEST = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request");
	EV_MOD_NMOESI_WRITE_REQUEST_RECEIVE = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_receive");
	EV_MOD_NMOESI_WRITE_REQUEST_ACTION = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_action");
	EV_MOD_NMOESI_WRITE_REQUEST_EXCLUSIVE = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_exclusive");
	EV_MOD_NMOESI_WRITE_REQUEST_UPDOWN = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_updown");
	EV_MOD_NMOESI_WRITE_REQUEST_UPDOWN_FINISH = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_updown_finish");
	EV_MOD_NMOESI_WRITE_REQUEST_DOWNUP = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_downup");
	EV_MOD_NMOESI_WRITE_REQUEST_DOWNUP_FINISH = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_downup_finish");
	EV_MOD_NMOESI_WRITE_REQUEST_REPLY = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_reply");
	EV_MOD_NMOESI_WRITE_REQUEST_FINISH = esim_register_event_with_name(mod_handler_nmoesi_write_request, "mod_nmoesi_write_request_finish");

	EV_MOD_NMOESI_READ_REQUEST = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request");
	EV_MOD_NMOESI_READ_REQUEST_RECEIVE = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_receive");
	EV_MOD_NMOESI_READ_REQUEST_ACTION = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_action");
	EV_MOD_NMOESI_READ_REQUEST_UPDOWN = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_updown");
	EV_MOD_NMOESI_READ_REQUEST_UPDOWN_MISS = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_updown_miss");
	EV_MOD_NMOESI_READ_REQUEST_UPDOWN_FINISH = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_updown_finish");
	EV_MOD_NMOESI_READ_REQUEST_DOWNUP = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_downup");
	EV_MOD_NMOESI_READ_REQUEST_DOWNUP_WAIT_FOR_REQS = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_downup_wait_for_reqs");
	EV_MOD_NMOESI_READ_REQUEST_DOWNUP_FINISH = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_downup_finish");
	EV_MOD_NMOESI_READ_REQUEST_REPLY = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_reply");
	EV_MOD_NMOESI_READ_REQUEST_FINISH = esim_register_event_with_name(mod_handler_nmoesi_read_request, "mod_nmoesi_read_request_finish");

	EV_MOD_NMOESI_INVALIDATE = esim_register_event_with_name(mod_handler_nmoesi_invalidate, "mod_nmoesi_invalidate");
	EV_MOD_NMOESI_INVALIDATE_FINISH = esim_register_event_with_name(mod_handler_nmoesi_invalidate, "mod_nmoesi_invalidate_finish");

	EV_MOD_NMOESI_PEER_SEND = esim_register_event_with_name(mod_handler_nmoesi_peer, "mod_nmoesi_peer_send");
	EV_MOD_NMOESI_PEER_RECEIVE = esim_register_event_with_name(mod_handler_nmoesi_peer, "mod_nmoesi_peer_receive");
	EV_MOD_NMOESI_PEER_REPLY = esim_register_event_with_name(mod_handler_nmoesi_peer, "mod_nmoesi_peer_reply");
	EV_MOD_NMOESI_PEER_FINISH = esim_register_event_with_name(mod_handler_nmoesi_peer, "mod_nmoesi_peer_finish");

	EV_MOD_NMOESI_MESSAGE = esim_register_event_with_name(mod_handler_nmoesi_message, "mod_nmoesi_message");
	EV_MOD_NMOESI_MESSAGE_RECEIVE = esim_register_event_with_name(mod_handler_nmoesi_message, "mod_nmoesi_message_receive");
	EV_MOD_NMOESI_MESSAGE_ACTION = esim_register_event_with_name(mod_handler_nmoesi_message, "mod_nmoesi_message_action");
	EV_MOD_NMOESI_MESSAGE_REPLY = esim_register_event_with_name(mod_handler_nmoesi_message, "mod_nmoesi_message_reply");
	EV_MOD_NMOESI_MESSAGE_FINISH = esim_register_event_with_name(mod_handler_nmoesi_message, "mod_nmoesi_message_finish");

	/* Local memory event driven simulation */

	EV_MOD_LOCAL_MEM_LOAD = esim_register_event_with_name(mod_handler_local_mem_load, "mod_local_mem_load");
	EV_MOD_LOCAL_MEM_LOAD_LOCK = esim_register_event_with_name(mod_handler_local_mem_load, "mod_local_mem_load_lock");
	EV_MOD_LOCAL_MEM_LOAD_FINISH = esim_register_event_with_name(mod_handler_local_mem_load, "mod_local_mem_load_finish");

	EV_MOD_LOCAL_MEM_STORE = esim_register_event_with_name(mod_handler_local_mem_store, "mod_local_mem_store");
	EV_MOD_LOCAL_MEM_STORE_LOCK = esim_register_event_with_name(mod_handler_local_mem_store, "mod_local_mem_store_lock");
	EV_MOD_LOCAL_MEM_STORE_FINISH = esim_register_event_with_name(mod_handler_local_mem_store, "mod_local_mem_store_finish");

	EV_MOD_LOCAL_MEM_FIND_AND_LOCK = esim_register_event_with_name(mod_handler_local_mem_find_and_lock, "mod_local_mem_find_and_lock");
	EV_MOD_LOCAL_MEM_FIND_AND_LOCK_PORT = esim_register_event_with_name(mod_handler_local_mem_find_and_lock, "mod_local_mem_find_and_lock_port");
	EV_MOD_LOCAL_MEM_FIND_AND_LOCK_ACTION = esim_register_event_with_name(mod_handler_local_mem_find_and_lock, "mod_local_mem_find_and_lock_action");
	EV_MOD_LOCAL_MEM_FIND_AND_LOCK_FINISH = esim_register_event_with_name(mod_handler_local_mem_find_and_lock, "mod_local_mem_find_and_lock_finish");

	/* Read configuration */
	mem_config_read();
}


void mem_system_done(void)
{
	/* Dump report */
	mem_system_dump_report();

        dram_free(); // MY CODE

	/* Free memory modules */
	while (list_count(mem_system->mod_list))
		mod_free(list_pop(mem_system->mod_list));
	list_free(mem_system->mod_list);

	/* Free networks */
	while (list_count(mem_system->net_list))
		net_free(list_pop(mem_system->net_list));
	list_free(mem_system->net_list);

	/* Free memory system */
	free(mem_system);
}


void mem_system_dump_report(void)
{
	struct net_t *net;
	struct mod_t *mod;
	struct cache_t *cache;

	FILE *f;

	int i;

	/* Open file */
	f = file_open_for_write(mem_report_file_name);
	if (!f)
		return;
	
	/* Intro */
	fprintf(f, "; Report for caches, TLBs, and main memory\n");
	fprintf(f, ";    Accesses - Total number of accesses\n");
	fprintf(f, ";    Hits, Misses - Accesses resulting in hits/misses\n");
	fprintf(f, ";    HitRatio - Hits divided by accesses\n");
	fprintf(f, ";    Evictions - Invalidated or replaced cache blocks\n");
	fprintf(f, ";    Retries - For L1 caches, accesses that were retried\n");
	fprintf(f, ";    ReadRetries, WriteRetries, NCWriteRetries - Read/Write retried accesses\n");
	fprintf(f, ";    NoRetryAccesses - Number of accesses that were not retried\n");
	fprintf(f, ";    NoRetryHits, NoRetryMisses - Hits and misses for not retried accesses\n");
	fprintf(f, ";    NoRetryHitRatio - NoRetryHits divided by NoRetryAccesses\n");
	fprintf(f, ";    NoRetryReads, NoRetryWrites - Not retried reads and writes\n");
	fprintf(f, ";    Reads, Writes, NCWrites - Total read/write accesses\n");
	fprintf(f, ";    BlockingReads, BlockingWrites, BlockingNCWrites - Reads/writes coming from lower-level cache\n");
	fprintf(f, ";    NonBlockingReads, NonBlockingWrites, NonBlockingNCWrites - Coming from upper-level cache\n");
	fprintf(f, "\n\n");
	
	/* Report for each cache */
	for (i = 0; i < list_count(mem_system->mod_list); i++)
	{
		mod = list_get(mem_system->mod_list, i);
		cache = mod->cache;
		fprintf(f, "[ %s ]\n", mod->name);
		fprintf(f, "\n");

		/* Configuration */
		if (cache) 
                {
			fprintf(f, "Sets = %d\n", cache->num_sets);
			fprintf(f, "Assoc = %d\n", cache->assoc);
			fprintf(f, "Policy = %s\n\n", str_map_value(&cache_policy_map, cache->policy));

                        //====== MY CODE =======//
                        cache_printstats(cache, f);

                        if ((mod->DRAM)&&(mod->kind == mod_kind_cache)) 
                        {
                           dramcache_report_dump(f);
                           dramcache_victim_printstats(f);
                        }
                        //=== END OF MY CODE ===//
                }
		fprintf(f, "BlockSize = %d\n", mod->block_size);
		fprintf(f, "Latency = %d\n", mod->latency);
		fprintf(f, "Ports = %d\n", mod->num_ports);
		fprintf(f, "\n");

                if (mod->kind == mod_kind_dram_main_memory) 
                {
                   fprintf(f, "DRAM Read Requests = %lld\n", mod->dram_request_read);
                   fprintf(f, "DRAM Write Requests = %lld\n\n", mod->dram_request_write);
                }

		/* Statistics */
		fprintf(f, "Accesses = %lld\n", mod->accesses);
		fprintf(f, "Hits = %lld\n", mod->hits);
		fprintf(f, "Misses = %lld\n", mod->accesses - mod->hits);
		fprintf(f, "HitRatio = %.4g\n\n", mod->accesses ?
			(double) mod->hits / mod->accesses : 0.0);

		fprintf(f, "Evictions = %lld\n", mod->evictions);
                fprintf(f, "DoA Evictions = %lld\n", mod->doa_evictions);  //===== MY CODE =====//
                fprintf(f, "DoA Ratio = %.4g\n\n", mod->evictions ? 
                        (double) mod->doa_evictions/mod->evictions : 0.0); //===== MY CODE =====//

                fprintf(f, "Writebacks (from upper level) = %lld\n", mod->writebacks_from_up);//===== MY CODE =====//
                fprintf(f, "Writebacks (to lower level) = %lld\n", mod->writebacks_to_down);//===== MY CODE =====//
                fprintf(f, "Dir evicts = %lld\n\n", mod->evict_accesses);  //===== MY CODE =====//

                fprintf(f, "Retries = %lld\n", mod->read_retries + mod->write_retries + 
			mod->nc_write_retries);
		fprintf(f, "\n");
		fprintf(f, "Reads = %lld\n", mod->reads);
		fprintf(f, "ReadRetries = %lld\n", mod->read_retries);
		fprintf(f, "BlockingReads = %lld\n", mod->blocking_reads);
		fprintf(f, "NonBlockingReads = %lld\n", mod->non_blocking_reads);
		fprintf(f, "ReadHits = %lld\n", mod->read_hits);
		fprintf(f, "ReadMisses = %lld\n", mod->reads - mod->read_hits);
		fprintf(f, "\n");
		fprintf(f, "Writes = %lld\n", mod->writes);
		fprintf(f, "WriteRetries = %lld\n", mod->write_retries);
		fprintf(f, "BlockingWrites = %lld\n", mod->blocking_writes);
		fprintf(f, "NonBlockingWrites = %lld\n", mod->non_blocking_writes);
		fprintf(f, "WriteHits = %lld\n", mod->write_hits);
		fprintf(f, "WriteMisses = %lld\n", mod->writes - mod->write_hits);
		fprintf(f, "\n");
		fprintf(f, "NCWrites = %lld\n", mod->nc_writes);
		fprintf(f, "NCWriteRetries = %lld\n", mod->nc_write_retries);
		fprintf(f, "NCBlockingWrites = %lld\n", mod->blocking_nc_writes);
		fprintf(f, "NCNonBlockingWrites = %lld\n", mod->non_blocking_nc_writes);
		fprintf(f, "NCWriteHits = %lld\n", mod->nc_write_hits);
		fprintf(f, "NCWriteMisses = %lld\n", mod->nc_writes - mod->nc_write_hits);
		fprintf(f, "Prefetches = %lld\n", mod->prefetches);
		fprintf(f, "PrefetchAborts = %lld\n", mod->prefetch_aborts);
		fprintf(f, "UselessPrefetches = %lld\n", mod->useless_prefetches);
		fprintf(f, "\n");
		fprintf(f, "NoRetryAccesses = %lld\n", mod->no_retry_accesses);
		fprintf(f, "NoRetryHits = %lld\n", mod->no_retry_hits);
		fprintf(f, "NoRetryMisses = %lld\n", mod->no_retry_accesses - mod->no_retry_hits);
		fprintf(f, "NoRetryHitRatio = %.4g\n", mod->no_retry_accesses ?
			(double) mod->no_retry_hits / mod->no_retry_accesses : 0.0);
		fprintf(f, "NoRetryReads = %lld\n", mod->no_retry_reads);
		fprintf(f, "NoRetryReadHits = %lld\n", mod->no_retry_read_hits);
		fprintf(f, "NoRetryReadMisses = %lld\n", (mod->no_retry_reads -
			mod->no_retry_read_hits));
		fprintf(f, "NoRetryWrites = %lld\n", mod->no_retry_writes);
		fprintf(f, "NoRetryWriteHits = %lld\n", mod->no_retry_write_hits);
		fprintf(f, "NoRetryWriteMisses = %lld\n", mod->no_retry_writes
			- mod->no_retry_write_hits);
		fprintf(f, "NoRetryNCWrites = %lld\n", mod->no_retry_nc_writes);
		fprintf(f, "NoRetryNCWriteHits = %lld\n", mod->no_retry_nc_write_hits);
		fprintf(f, "NoRetryNCWriteMisses = %lld\n", mod->no_retry_nc_writes
			- mod->no_retry_nc_write_hits);
		fprintf(f, "\n\n");
	}

	/* Dump report for networks */
	for (i = 0; i < list_count(mem_system->net_list); i++)
	{
		net = list_get(mem_system->net_list, i);
		net_dump_report(net, f);
	}
	
	/* Done */
	fclose(f);
}


struct mod_t *mem_system_get_mod(char *mod_name)
{
	struct mod_t *mod;

	int mod_id;

	/* Look for module */
	LIST_FOR_EACH(mem_system->mod_list, mod_id)
	{
		mod = list_get(mem_system->mod_list, mod_id);
		if (!strcasecmp(mod->name, mod_name))
			return mod;
	}

	/* Not found */
	return NULL;
}


struct net_t *mem_system_get_net(char *net_name)
{
	struct net_t *net;

	int net_id;

	/* Look for network */
	LIST_FOR_EACH(mem_system->net_list, net_id)
	{
		net = list_get(mem_system->net_list, net_id);
		if (!strcasecmp(net->name, net_name))
			return net;
	}

	/* Not found */
	return NULL;
}

//================================ MY CODE ==================================//
//------FUNCTIONS FOR DRAM MAINMEMORY------//

// read callback function
// will be call by DRAMSim2 whenever a read request is finished
void dram_read_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle)
{
   struct mod_stack_t * stack;
   struct mod_t * dram_mod;

   dram_mod = mod_get_dram_mod();
   stack = mod_dram_req_remove(dram_mod,address,0);

   // Add reply event
   esim_schedule_event(EV_MOD_NMOESI_READ_REQUEST_REPLY, stack, 0);

   return;
}
// write callback function
// will be call by DRAMSim2 whenever a write request is finished
void dram_write_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle)
{
   struct mod_stack_t * stack;
   struct mod_t * dram_mod;

   dram_mod = mod_get_dram_mod();
   stack = mod_dram_req_remove(dram_mod,address,1);

   // Add reply event
   esim_schedule_event(EV_MOD_NMOESI_WRITE_REQUEST_REPLY, stack, 0);

   return;
}

// add a new DRAM request
void dram_add_request(struct mod_t * dram_mod, struct mod_stack_t *stack, unsigned char iswrite)
{
   // add the request to the global queue
   mod_dram_req_insert(dram_mod,stack,stack->addr,iswrite, none);
   // send the request to DRAMSim2
   memory_addtransaction(dram_mod->DRAM, iswrite, stack->addr);

   // statistics

   if (iswrite) 
   {
      dram_mod->dram_request_write++;
   }
   else
   {
      dram_mod->dram_request_read++;
   }

   return;
}

// DRAM update
// will be call once in every CPU cycle.
void dram_update(void)
{
   struct mod_t * dram_mod;

   dram_mod = mod_get_dram_mod();

   if (dram_mod == NULL || dram_mod->DRAM == NULL) 
   {
      return;
   }

   memory_update(dram_mod->DRAM);

   return;
}

void dram_printstats(void)
{
   struct mod_t * dram_mod;

   dram_mod = mod_get_dram_mod();

   if (dram_mod == NULL || dram_mod->DRAM == NULL) 
   {
      return;
   }

   memory_printstats(dram_mod->DRAM);

   return;
}

//------FUNCTIONS FOR DRAM CACHE------//
// read callback function
// will be call by DRAMSim2 whenever a read request is finished
void dramcache_read_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle)
{
   struct mod_stack_t * stack;
   struct mod_t * dramcache_mod;
   enum dramcache_type_t access_type;

   unsigned int cache_assoc;

   dramcache_mod = mod_get_dramcache_mod();
   // get the access type
   access_type = mod_dram_req_type(dramcache_mod,address,0);
   // get the stack pointer and remove the request from queue
   stack = mod_dram_req_remove(dramcache_mod,address,0);

   cache_assoc = dramcache_mod->cache->assoc;

   // Add reply event
   if (access_type == tag_access_writehit)
   {
      // do nothing
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) tag read-dram cache callback-write hit\n", 
                esim_cycle, 
                stack->id,
                stack->tag, 
                dramcache_mod->name,
                address);
      if (cache_assoc == 1)  // alloy cache
      {
         dramcache_add_request(dramcache_mod, stack, 1, write_data_access);
         //esim_schedule_event(EV_MOD_NMOESI_WRITE_REQUEST_REPLY, stack, 0);
      }
      else if (cache_assoc == 29) // gabriel's method
      {
         dramcache_add_request(dramcache_mod, stack, 1, write_data_access);
      }
   }
   else if (access_type == tag_access_readhit)
   {
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) tag read-dram cache callback-read hit\n", 
                esim_cycle, 
                stack->id,
                stack->tag, 
                dramcache_mod->name,
                address);
      if (cache_assoc == 1) // alloy cache
      {
         esim_schedule_event(EV_MOD_NMOESI_READ_REQUEST_REPLY, stack, 0);
      }
      else if (cache_assoc == 29) // gabriel's method
      {
         dramcache_add_request(dramcache_mod, stack, 0, read_data_access);
      }
   }
   else if (access_type == tag_access_readmiss)
   {
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) tag read-dram cache callback-read miss\n", 
                esim_cycle, 
                stack->id,
                stack->tag, 
                dramcache_mod->name,
                address);
      if (dramcache_mod->miss_dramcache_policy == normal) {
         esim_schedule_event(EV_MOD_NMOESI_READ_REQUEST_REPLY, stack, 0);
      }
   }
   else if (access_type == tag_access_writemiss)
   {
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) tag read-dram cache callback-write miss\n", 
                esim_cycle, 
                stack->id,
                stack->tag, 
                dramcache_mod->name,
                address);
      dramcache_add_request(dramcache_mod, stack, 1, write_data_access);
      //esim_schedule_event(EV_MOD_NMOESI_WRITE_REQUEST_REPLY, stack, 0);

      
   }
   else if (access_type == read_data_access) 
   {
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) data read-dram cache callback\n", 
                esim_cycle, 
                stack->id,
                stack->tag, 
                dramcache_mod->name,
                address);
      esim_schedule_event(EV_MOD_NMOESI_READ_REQUEST_REPLY, stack, 0);
   }
   else if (access_type == writeback_tag_access) 
   {
      dramcache_add_request(dramcache_mod, stack, 1, writeback_data_access);
   }
   else 
   {
      // something is wrong
      // for DRAM cache, there should not be other access types for reads.
      return;
   }
   

   return;
}

// write callback function
// will be call by DRAMSim2 whenever a write request is finished
void dramcache_write_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle)
{
   struct mod_stack_t * stack;
   struct mod_t * dramcache_mod;
   enum dramcache_type_t access_type;

   dramcache_mod = mod_get_dramcache_mod();
   // get the access type
   access_type = mod_dram_req_type(dramcache_mod, address, 1);
   // get the stack pointer and remove the request from queue
   stack = mod_dram_req_remove(dramcache_mod,address,1);

   // Add reply event
   if (access_type == write_data_access) 
   {
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) data write-dram cache callback\n", 
                esim_cycle, 
                stack->id,
                stack->tag, 
                dramcache_mod->name,
                address);
      esim_schedule_event(EV_MOD_NMOESI_WRITE_REQUEST_REPLY, stack, 0);
   }
   else if (access_type == new_block_allocation) 
   {
      mem_debug("  %lld %lld 0x%x %s (addr after mapping: 0x%x) new line allocation-dram cache callback\n", 
                esim_cycle, 
                stack->set,
                stack->tag, 
                dramcache_mod->name,
                address);
      //cache_set_block(stack->target_mod->cache, stack->set, stack->way, stack->tag,
      //          stack->shared ? cache_block_shared : cache_block_exclusive);

      //free(stack);
   }
   else if (access_type == writeback_data_access)  
   {
      // do nothing
   }

   return;
}

void dramcache_add_request(struct mod_t * dramcache_mod, 
                           struct mod_stack_t *stack, 
                           unsigned char iswrite,
                           enum dramcache_type_t access_type)
{
   unsigned int block_num=0;
   unsigned int row_cnt=0;
   unsigned int col_cnt=0;
   unsigned int new_addr=0;
   unsigned int lower_bits1=0, lower_bits2=0, lower_bits3=0;
   unsigned int middle_bits=0;

   // Get cache info
   unsigned int cache_sets = dramcache_mod->cache->num_sets;
   unsigned int cache_assoc = dramcache_mod->cache->assoc;
   unsigned int row_bits = 0;
   
   if (cache_assoc == 1) // Moin's Method
   {
      if (cache_sets == 2097152) //128MB dram cache 
      {
         row_bits = 7;
      }
      else if (cache_sets == 4194304) // 256MB dram cache
      {
         row_bits = 8;
      }
      else if (cache_sets == 8388608) // 512MB dram cache
      {
         row_bits = 9;
      }
      else if (cache_sets == 16777216) // 1GB dram cache
      {
         row_bits = 10;
      }
      else
      {
         fatal("Wrong Cache Set Number: %d\n",cache_sets);
      }
   }
   else if (cache_assoc == 29) // Gabriel's Method
   {
      if (cache_sets == 65536) //128MB dram cache 
      {
         row_bits = 7;
      }
      else if (cache_sets == 131072) // 256MB dram cache
      {
         row_bits = 8;
      }
      else if (cache_sets == 262144) // 512MB dram cache
      {
         row_bits = 9;
      }
      else if (cache_sets == 524288) // 1GB dram cache
      {
         row_bits = 10;
      }
      else
      {
         fatal("Wrong Cache Set Number: %d\n",cache_sets);
      }
   }
   else
   {
      fatal("Wrong Cache Assoc Number: %d\n",cache_assoc);
   }

   if (cache_assoc == 1) // Moin's Method
   {
      // direct-mapped cache, 64bytes cache line size
      block_num = (stack->set) % (4*128*(1<<row_bits)*28); 

      // address remapping for alloy-cache
      // TAD-72B: tag-8B data-64B 
      // 2KB row buffer: 28 TADs per row (32 bytes unused)

      row_cnt = block_num/28;
      col_cnt = block_num%28;
   }
   else if (cache_assoc == 29) // Gabriel's Method
   {
      // set-associative cache
      // 2KB row buffer 
      // 29 tags stored in first 3 data blocks 
      // 29 cache data blocks

      row_cnt = stack->set;
      col_cnt = stack->way;
   }
   else
      fatal("Wrong Cache Assoc Number: %d\n",cache_assoc);

   // DRAMSim2 setting: 
   // ADDRESS_MAPPING_SCHEME=schemex
   // mapping: chan:rank:bank:row:col
   // remapping row_cnt, so that sequencial rows are mapped 
   // to different channels.
   // -------------------------
   // 4 channels: 2 bits
   // 8 banks:    3 bits
   // 1024/512/256/128 rows:  10/9/8/7 bits
   // 1 rank:     0 bit
   // 2048 cols:  11 bits
   // BUS 128 bits/16 bytes: 4 bits
   // -------------------------
   //  chan  :  rank :  bank  :    row        :   col   : bus offset
   // 2 bits : 0 bit : 3 bits : 10/9/8/7 bits : 11 bits : 4 bits

   lower_bits1 = row_cnt & ( (1<<4) - 1); // 4 bits [3:0]
   lower_bits2 = (row_cnt>>4) & ( (1<<2) - 1); // 2 bits [5:4]
   lower_bits3 = (row_cnt>>(4+2)) & ( (1<<3) - 1); // 3 bits [8:6]
   middle_bits = row_cnt>>(4+2+3);

   // lower_bits2: chan 
   // lower_bits3: bank
   row_cnt = (lower_bits2<<(3+row_bits+4)) | (lower_bits3<<(row_bits+4)) | (middle_bits<<4) | lower_bits1;


   if (cache_assoc == 1) 
   {

      if (access_type <= tag_access_writemiss) 
      {
         new_addr = (row_cnt<<11)+(col_cnt*72);
      }
      else if ( (access_type == write_data_access)||(access_type == read_data_access) ) 
      {
         // for alloy cache, data is already fetched by tag access.
         // so this only happens for stores.
         new_addr = (row_cnt<<11)+(col_cnt*72);
      }
      else if (access_type == new_block_allocation)
      {
         new_addr = (row_cnt<<11)+(col_cnt*72);
         
      }  
   }
   else if (cache_assoc == 29) 
   {
      if (access_type <= tag_access_writemiss) 
      {
         new_addr = (row_cnt<<11);
      }
      else if ( (access_type == write_data_access)||(access_type == read_data_access) )
      {
         // for alloy cache, data is already fetched by tag access.
         // so this only happens for stores.
         new_addr = (row_cnt<<11) + ((col_cnt+3)*64);
      }
      else if (access_type == new_block_allocation)
      {
         new_addr = (row_cnt<<11)+((col_cnt+3)*64);
      }  
   }

   // add request to the global queue
   mod_dram_req_insert(dramcache_mod,stack,new_addr,iswrite, access_type);
   // send the request to DRAMSim2
   memory_addtransaction(dramcache_mod->DRAM, iswrite, new_addr);

   // statistics
   if (access_type == tag_access_readhit) 
   {
      (dramcache_mod->dramcache_request_tag_access_readhit)++;
   }
   else if (access_type == tag_access_writehit)
   {
      (dramcache_mod->dramcache_request_tag_access_writehit)++;
   }
   else if (access_type == tag_access_readmiss)
   {
      (dramcache_mod->dramcache_request_tag_access_readmiss)++;
   }
   else if (access_type == tag_access_writemiss)
   {
      (dramcache_mod->dramcache_request_tag_access_writemiss)++;
   }
   else if (access_type == write_data_access)
   {
      (dramcache_mod->dramcache_request_write_data_access)++;
   }
   else if (access_type == read_data_access)
   {
      (dramcache_mod->dramcache_request_read_data_access)++;
   }
   else if (access_type == new_block_allocation)
   {
      (dramcache_mod->dramcache_request_new_block_allocation)++;
   }
   else if (access_type == writeback_data_access)
   {
      (dramcache_mod->dramcache_request_writeback_data_access)++;
   }
   else if (access_type == writeback_tag_access)
   {
      (dramcache_mod->dramcache_request_writeback_tag_access)++;
   }

   
   return;
}

// DRAM update
// will be call once in every CPU cycle.
void dramcache_update(void)
{
   struct mod_t * dramcache_mod;

   dramcache_mod = mod_get_dramcache_mod();

   if (dramcache_mod == NULL || dramcache_mod->DRAM == NULL) 
   {
      return;
   }

   memory_update(dramcache_mod->DRAM);

   return;
}

void dramcache_printstats(void)
{
   struct mod_t * dramcache_mod;

   dramcache_mod = mod_get_dramcache_mod();

   if (dramcache_mod == NULL || dramcache_mod->DRAM == NULL) 
   {
      return;
   }

   memory_printstats(dramcache_mod->DRAM);

   return;
}

// Victim cache for DRAM Cache
unsigned inline dramcache_log2(unsigned value)
{
	unsigned logbase2 = 0;
	unsigned orig = value;
	value>>=1;
	while (value>0)
	{
		value >>= 1;
		logbase2++;
	}
	if ((unsigned)1<<logbase2<orig)logbase2++;
	return logbase2;
}

void dramcache_victimCreate(struct mod_t * dramcache_mod, unsigned int victimsize, unsigned int victim_assoc)
{
   unsigned int num_sets, log_assoc;

   if (dramcache_mod == NULL) 
   {
      return;
   }
   if (victimsize == 0) // Inifinite victim cache
   {
      dramcache_mod->dramcache_victim_list = linked_list_create();
      dramcache_mod->dramcache_victim = NULL;
      return;
   }
   else
   {
      dramcache_mod->dramcache_victim_list = NULL;
      // Finite victim cache
      // Size in MB: victimsize
      // Assoc: fixed-32
      log_assoc = dramcache_log2(victim_assoc);
      num_sets = victimsize<<(20 - 6 - log_assoc);
      dramcache_mod->dramcache_victim = cache_create("dramcache_victim", 
                                       num_sets, 64, victim_assoc, cache_policy_lru);
   }
   
   return;
}

void dramcache_addVictim(unsigned int addr)
{
   struct mod_t * dramcache_mod = mod_get_dramcache_mod();

   if (dramcache_mod == NULL) 
   {
      return;
   }
   if (dramcache_mod->dramcache_victim_list == NULL) 
   {
      unsigned int offset;
      int set,tag,way;

      if (dramcache_mod->dramcache_victim == NULL) 
      {
         return;
      }
      // decode address to get set/tag
      cache_decode_address(dramcache_mod->dramcache_victim, addr, &set, &tag, &offset);
      // find out the way to insert
      way = cache_replace_block(dramcache_mod->dramcache_victim, set);
      // insert the block
      cache_set_block(dramcache_mod->dramcache_victim, set, way, tag, cache_block_modified);
      // update the LRU info
      cache_access_block(dramcache_mod->dramcache_victim, set, way);

      return;
   }
   else
   {
      linked_list_add_new(dramcache_mod->dramcache_victim_list, (void*)addr, esim_cycle);
      return;
   }
}

unsigned char dramcache_hitVictim(unsigned int addr)
{
   struct mod_t * dramcache_mod = mod_get_dramcache_mod();

   if (dramcache_mod == NULL) 
   {
      return 0;
   }
   if (dramcache_mod->dramcache_victim_list == NULL) 
   {
      int set,tag,way,state,ret;

      if (dramcache_mod->dramcache_victim == NULL) 
      {
         return 0;
      }
      ret = cache_find_block(dramcache_mod->dramcache_victim, addr, &set, &way, &state);
      if (ret) // hit victim cache. Need to remove it from victim cache. 
      {
         // hit victim list
         (dramcache_mod->dramcache_hit_victim)++;
         
         cache_set_block(dramcache_mod->dramcache_victim, set, way, tag, cache_block_invalid);
      }

      return ret;
   }
   else
   {
      linked_list_find(dramcache_mod->dramcache_victim_list, (void *)addr);

      if ((dramcache_mod->dramcache_victim_list->error_code) != LINKED_LIST_ERR_OK) // miss victim list
      {
         return 0;
      }

      // hit victim list
      (dramcache_mod->dramcache_hit_victim)++;
      (dramcache_mod->dramcache_victim_reference_interval) += esim_cycle 
                     - dramcache_mod->dramcache_victim_list->current->cycle;

      linked_list_remove(dramcache_mod->dramcache_victim_list);
      return 1;
   }

   return 0;
}

void dramcache_report_dump(FILE * f)
{
   long long total_request;
   long long tag_request, data_request;
   struct mod_t * dramcache_mod = mod_get_dramcache_mod();
   if (dramcache_mod == NULL) 
   {
      return;
   }

   tag_request = dramcache_mod->dramcache_request_tag_access_readhit 
         + dramcache_mod->dramcache_request_tag_access_writehit
         + dramcache_mod->dramcache_request_tag_access_readmiss 
         + dramcache_mod->dramcache_request_tag_access_writemiss
         + dramcache_mod->dramcache_request_writeback_tag_access;

   data_request = dramcache_mod->dramcache_request_read_data_access
         + dramcache_mod->dramcache_request_write_data_access
         + dramcache_mod->dramcache_request_writeback_data_access;

   total_request = tag_request 
         + data_request 
         + dramcache_mod->dramcache_request_new_block_allocation;

   fprintf(f, "DRAMCache Requests = %lld\n", total_request);
   fprintf(f, "DRAMCache Tag Requests = %lld\n", tag_request);
   fprintf(f, "DRAMCache Data Requests = %lld\n", data_request);
   fprintf(f, "DRAMCache NewBlock Allocation = %lld\n\n", dramcache_mod->dramcache_request_new_block_allocation);

   fprintf(f, "DRAMCache Tag Requests (ReadHit) = %lld\n", dramcache_mod->dramcache_request_tag_access_readhit);
   fprintf(f, "DRAMCache Tag Requests (ReadMiss) = %lld\n", dramcache_mod->dramcache_request_tag_access_readmiss);
   fprintf(f, "DRAMCache Tag Requests (WriteHit) = %lld\n", dramcache_mod->dramcache_request_tag_access_writehit);
   fprintf(f, "DRAMCache Tag Requests (WriteMiss) = %lld\n", dramcache_mod->dramcache_request_tag_access_writemiss);
   fprintf(f, "DRAMCache Tag Requests (Writeback) = %lld\n\n", dramcache_mod->dramcache_request_writeback_tag_access);

   fprintf(f, "DRAMCache Data Requests (Read) = %lld\n", dramcache_mod->dramcache_request_read_data_access);
   fprintf(f, "DRAMCache Data Requests (Write) = %lld\n", dramcache_mod->dramcache_request_write_data_access);
   fprintf(f, "DRAMCache Data Requests (Writeback) = %lld\n\n", dramcache_mod->dramcache_request_writeback_data_access);
   return;
}

void dramcache_victim_printstats(FILE * f)
{
   struct mod_t * dramcache_mod = mod_get_dramcache_mod();
   if (dramcache_mod == NULL) 
   {
      return;
   }
   fprintf(f, "DRAMCache Victim Hits = %lld\n", dramcache_mod->dramcache_hit_victim);
   if (dramcache_mod->dramcache_hit_victim) 
   {
      fprintf(f, "DRAMCache Victim Average Reference Invertal = %.4g\n\n", 
             (double)dramcache_mod->dramcache_victim_reference_interval /dramcache_mod->dramcache_hit_victim);
   }
   else
   {
      fprintf(f, "DRAMCache Victim Average Reference Invertal = 0.0\n\n");
   }
}

void dram_free(void)
{
   struct mod_t * dram_mod = mod_get_dram_mod();
   struct mod_t * dramcache_mod = mod_get_dramcache_mod();

   if (dram_mod) 
   {
      struct dram_req_list_t * req_ptr = dram_mod->dram_pending_request_head;
      struct dram_req_list_t * req_next = NULL;

      struct dramcache_info_list_t * info_ptr = dram_mod->dramcache_hit_info;
      struct dramcache_info_list_t * info_next = NULL;

      while (req_ptr) 
      {
         req_next = req_ptr->next;
         free(req_ptr);
         req_ptr = req_next;
      }
      while (info_ptr) 
      {
         info_next = info_ptr->next;
         free(info_ptr);
         info_ptr = info_next;
      }
   }

   if (dramcache_mod) 
   {
      struct dram_req_list_t * req_ptr = dramcache_mod->dram_pending_request_head;
      struct dram_req_list_t * req_next = NULL;

      struct dramcache_info_list_t * info_ptr = dramcache_mod->dramcache_hit_info;
      struct dramcache_info_list_t * info_next = NULL;


      while (req_ptr) 
      {
         req_next = req_ptr->next;
         free(req_ptr);
         req_ptr = req_next;
      }
      while (info_ptr) 
      {
         info_next = info_ptr->next;
         free(info_ptr);
         info_ptr = info_next;
      }

      if (dramcache_mod->dramcache_victim_list != NULL) 
      {
         
         linked_list_clear(dramcache_mod->dramcache_victim_list);
      }
     
      if (dramcache_mod->dramcache_victim != NULL) 
      {
         cache_free(dramcache_mod->dramcache_victim);
      }
         
   }

}

//====================END OF MY CODE========================//
