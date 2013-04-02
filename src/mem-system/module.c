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

#include <assert.h>

#include <lib/esim/esim.h>
#include <lib/mhandle/mhandle.h>
#include <lib/util/debug.h>
#include <lib/util/linked-list.h>
#include <lib/util/misc.h>
#include <lib/util/string.h>
#include <lib/util/repos.h>

#include "cache.h"
#include "directory.h"
#include "local-mem-protocol.h"
#include "mem-system.h"
#include "mod-stack.h"
#include "nmoesi-protocol.h"


/* String map for access type */
struct str_map_t mod_access_kind_map =
{
	3, {
		{ "Load", mod_access_load },
		{ "Store", mod_access_store },
		{ "NCStore", mod_access_nc_store },
		{ "Prefetch", mod_access_prefetch }
	}
};




/*
 * Public Functions
 */

struct mod_t *mod_create(char *name, enum mod_kind_t kind, int num_ports,
	int block_size, int latency)
{
	struct mod_t *mod;

	/* Initialize */
	mod = xcalloc(1, sizeof(struct mod_t));
	mod->name = xstrdup(name);
	mod->kind = kind;
	mod->latency = latency;

	/* Ports */
	mod->num_ports = num_ports;
	mod->ports = xcalloc(num_ports, sizeof(struct mod_port_t));

	/* Lists */
	mod->low_mod_list = linked_list_create();
	mod->high_mod_list = linked_list_create();

	/* Block size */
	mod->block_size = block_size;
	assert(!(block_size & (block_size - 1)) && block_size >= 4);
	mod->log_block_size = log_base2(block_size);

	mod->client_info_repos = repos_create(sizeof(struct mod_client_info_t), mod->name);

	return mod;
}


void mod_free(struct mod_t *mod)
{
	linked_list_free(mod->low_mod_list);
	linked_list_free(mod->high_mod_list);
        //======= MY CODE =======//
        mod_dram_free(mod);
        //==== END OF MY CODE ===//
	if (mod->cache)
		cache_free(mod->cache);
	if (mod->dir)
		dir_free(mod->dir);
	free(mod->ports);
	repos_free(mod->client_info_repos);
	free(mod->name);
	free(mod);
}


void mod_dump(struct mod_t *mod, FILE *f)
{
}


/* Access a memory module.
 * Variable 'witness', if specified, will be increased when the access completes.
 * The function returns a unique access ID.
 */
long long mod_access(struct mod_t *mod, enum mod_access_kind_t access_kind, 
	unsigned int addr, int *witness_ptr, struct linked_list_t *event_queue,
	void *event_queue_item, struct mod_client_info_t *client_info)
{
	struct mod_stack_t *stack;
	int event;

	/* Create module stack with new ID */
	mod_stack_id++;
	stack = mod_stack_create(mod_stack_id,
		mod, addr, ESIM_EV_NONE, NULL);

	/* Initialize */
	stack->witness_ptr = witness_ptr;
	stack->event_queue = event_queue;
	stack->event_queue_item = event_queue_item;
	stack->client_info = client_info;

	/* Select initial CPU/GPU event */
	if (mod->kind == mod_kind_cache || mod->kind == mod_kind_main_memory
            || mod->kind==mod_kind_dram_main_memory) // MY CODE
	{
		if (access_kind == mod_access_load)
		{
			event = EV_MOD_NMOESI_LOAD;
		}
		else if (access_kind == mod_access_store)
		{
			event = EV_MOD_NMOESI_STORE;
		}
		else if (access_kind == mod_access_nc_store)
		{
			event = EV_MOD_NMOESI_NC_STORE;
		}
		else if (access_kind == mod_access_prefetch)
		{
			event = EV_MOD_NMOESI_PREFETCH;
		} else {
			panic("%s: invalid access kind", __FUNCTION__);
		}
	}
	else if (mod->kind == mod_kind_local_memory)
	{
		if (access_kind == mod_access_load)
		{
			event = EV_MOD_LOCAL_MEM_LOAD;
		}
		else if (access_kind == mod_access_store)
		{
			event = EV_MOD_LOCAL_MEM_STORE;
		}
		else
		{
			panic("%s: invalid access kind", __FUNCTION__);
		}
	}
	else
	{
		panic("%s: invalid mod kind", __FUNCTION__);
	}

	/* Schedule */
	esim_execute_event(event, stack);

	/* Return access ID */
	return stack->id;
}


/* Return true if module can be accessed. */
int mod_can_access(struct mod_t *mod, unsigned int addr)
{
	int non_coalesced_accesses;

	/* There must be a free port */
	assert(mod->num_locked_ports <= mod->num_ports);
	if (mod->num_locked_ports == mod->num_ports)
		return 0;

	/* If no MSHR is given, module can be accessed */
	if (!mod->mshr_size)
		return 1;

	/* Module can be accessed if number of non-coalesced in-flight
	 * accesses is smaller than the MSHR size. */
	non_coalesced_accesses = mod->access_list_count -
		mod->access_list_coalesced_count;
	return non_coalesced_accesses < mod->mshr_size;
}


/* Return {set, way, tag, state} for an address.
 * The function returns TRUE on hit, FALSE on miss. */
int mod_find_block(struct mod_t *mod, unsigned int addr, int *set_ptr,
	int *way_ptr, int *tag_ptr, int *state_ptr)
{
	struct cache_t *cache = mod->cache;
	struct cache_block_t *blk;
	struct dir_lock_t *dir_lock;

	int set;
	int way;
	int tag;

	/* A transient tag is considered a hit if the block is
	 * locked in the corresponding directory. */
	tag = addr & ~cache->block_mask;
	if (mod->range_kind == mod_range_interleaved)
	{
		unsigned int num_mods = mod->range.interleaved.mod;
		set = ((tag >> cache->log_block_size) / num_mods) % cache->num_sets;
	}
	else if (mod->range_kind == mod_range_bounds)
	{
		set = (tag >> cache->log_block_size) % cache->num_sets;
	}
	else 
	{
		panic("%s: invalid range kind (%d)", __FUNCTION__, mod->range_kind);
	}

	for (way = 0; way < cache->assoc; way++)
	{
		blk = &cache->sets[set].blocks[way];
		if (blk->tag == tag && blk->state)
			break;
		if (blk->transient_tag == tag)
		{
			dir_lock = dir_lock_get(mod->dir, set, way);
			if (dir_lock->lock)
				break;
		}
	}

	PTR_ASSIGN(set_ptr, set);
	PTR_ASSIGN(tag_ptr, tag);

	/* Miss */
	if (way == cache->assoc)
	{
	/*
		PTR_ASSIGN(way_ptr, 0);
		PTR_ASSIGN(state_ptr, 0);
	*/
		return 0;
	}

	/* Hit */
	PTR_ASSIGN(way_ptr, way);
	PTR_ASSIGN(state_ptr, cache->sets[set].blocks[way].state);
	return 1;
}

void mod_block_set_prefetched(struct mod_t *mod, unsigned int addr, int val)
{
	int set, way;

	assert(mod->kind == mod_kind_cache && mod->cache != NULL);
	if (mod->cache->prefetcher && mod_find_block(mod, addr, &set, &way, NULL, NULL))
	{
		mod->cache->sets[set].blocks[way].prefetched = val;
	}
}

int mod_block_get_prefetched(struct mod_t *mod, unsigned int addr)
{
	int set, way;

	assert(mod->kind == mod_kind_cache && mod->cache != NULL);
	if (mod->cache->prefetcher && mod_find_block(mod, addr, &set, &way, NULL, NULL))
	{
		return mod->cache->sets[set].blocks[way].prefetched;
	}

	return 0;
}

/* Lock a port, and schedule event when done.
 * If there is no free port, the access is enqueued in the port
 * waiting list, and it will retry once a port becomes available with a
 * call to 'mod_unlock_port'. */
void mod_lock_port(struct mod_t *mod, struct mod_stack_t *stack, int event)
{
	struct mod_port_t *port = NULL;
	int i;

	/* No free port */
	if (mod->num_locked_ports >= mod->num_ports)
	{
		assert(!DOUBLE_LINKED_LIST_MEMBER(mod, port_waiting, stack));

		/* If the request to lock the port is down-up, give it priority since 
		 * it is possibly holding up a large portion of the memory hierarchy */
		if (stack->request_dir == mod_request_down_up)
		{
			DOUBLE_LINKED_LIST_INSERT_HEAD(mod, port_waiting, stack);
		}
		else 
		{
			DOUBLE_LINKED_LIST_INSERT_TAIL(mod, port_waiting, stack);
		}
		stack->port_waiting_list_event = event;
		return;
	}

	/* Get free port */
	for (i = 0; i < mod->num_ports; i++)
	{
		port = &mod->ports[i];
		if (!port->stack)
			break;
	}

	/* Lock port */
	assert(port && i < mod->num_ports);
	port->stack = stack;
	stack->port = port;
	mod->num_locked_ports++;

	/* Debug */
	mem_debug("  %lld stack %lld %s port %d locked\n", esim_cycle, stack->id, mod->name, i);

	/* Schedule event */
	esim_schedule_event(event, stack, 0);
}


void mod_unlock_port(struct mod_t *mod, struct mod_port_t *port,
	struct mod_stack_t *stack)
{
	int event;

	/* Checks */
	assert(mod->num_locked_ports > 0);
	assert(stack->port == port && port->stack == stack);
	assert(stack->mod == mod);

	/* Unlock port */
	stack->port = NULL;
	port->stack = NULL;
	mod->num_locked_ports--;

	/* Debug */
	mem_debug("  %lld %lld %s port unlocked\n", esim_cycle,
		stack->id, mod->name);

	/* Check if there was any access waiting for free port */
	if (!mod->port_waiting_list_count)
		return;

	/* Wake up one access waiting for a free port */
	stack = mod->port_waiting_list_head;
	event = stack->port_waiting_list_event;
	assert(DOUBLE_LINKED_LIST_MEMBER(mod, port_waiting, stack));
	DOUBLE_LINKED_LIST_REMOVE(mod, port_waiting, stack);
	mod_lock_port(mod, stack, event);

}


void mod_access_start(struct mod_t *mod, struct mod_stack_t *stack,
	enum mod_access_kind_t access_kind)
{
	int index;

	/* Record access kind */
	stack->access_kind = access_kind;

	/* Insert in access list */
	DOUBLE_LINKED_LIST_INSERT_TAIL(mod, access, stack);

	/* Insert in write access list */
	if (access_kind == mod_access_store)
		DOUBLE_LINKED_LIST_INSERT_TAIL(mod, write_access, stack);

	/* Insert in access hash table */
	index = (stack->addr >> mod->log_block_size) % MOD_ACCESS_HASH_TABLE_SIZE;
	DOUBLE_LINKED_LIST_INSERT_TAIL(&mod->access_hash_table[index], bucket, stack);
}


void mod_access_finish(struct mod_t *mod, struct mod_stack_t *stack)
{
	int index;

	/* Remove from access list */
	DOUBLE_LINKED_LIST_REMOVE(mod, access, stack);

	/* Remove from write access list */
	assert(stack->access_kind);
	if (stack->access_kind == mod_access_store)
		DOUBLE_LINKED_LIST_REMOVE(mod, write_access, stack);

	/* Remove from hash table */
	index = (stack->addr >> mod->log_block_size) % MOD_ACCESS_HASH_TABLE_SIZE;
	DOUBLE_LINKED_LIST_REMOVE(&mod->access_hash_table[index], bucket, stack);

	/* If this was a coalesced access, update counter */
	if (stack->coalesced)
	{
		assert(mod->access_list_coalesced_count > 0);
		mod->access_list_coalesced_count--;
	}
}


/* Return true if the access with identifier 'id' is in flight.
 * The address of the access is passed as well because this lookup is done on the
 * access truth table, indexed by the access address.
 */
int mod_in_flight_access(struct mod_t *mod, long long id, unsigned int addr)
{
	struct mod_stack_t *stack;
	int index;

	/* Look for access */
	index = (addr >> mod->log_block_size) % MOD_ACCESS_HASH_TABLE_SIZE;
	for (stack = mod->access_hash_table[index].bucket_list_head; stack; stack = stack->bucket_list_next)
		if (stack->id == id)
			return 1;

	/* Not found */
	return 0;
}


/* Return the youngest in-flight access older than 'older_than_stack' to block containing 'addr'.
 * If 'older_than_stack' is NULL, return the youngest in-flight access containing 'addr'.
 * The function returns NULL if there is no in-flight access to block containing 'addr'.
 */
struct mod_stack_t *mod_in_flight_address(struct mod_t *mod, unsigned int addr,
	struct mod_stack_t *older_than_stack)
{
	struct mod_stack_t *stack;
	int index;

	/* Look for address */
	index = (addr >> mod->log_block_size) % MOD_ACCESS_HASH_TABLE_SIZE;
	for (stack = mod->access_hash_table[index].bucket_list_head; stack;
		stack = stack->bucket_list_next)
	{
		/* This stack is not older than 'older_than_stack' */
		if (older_than_stack && stack->id >= older_than_stack->id)
			continue;

		/* Address matches */
		if (stack->addr >> mod->log_block_size == addr >> mod->log_block_size)
			return stack;
	}

	/* Not found */
	return NULL;
}


/* Return the youngest in-flight write older than 'older_than_stack'. If 'older_than_stack'
 * is NULL, return the youngest in-flight write. Return NULL if there is no in-flight write.
 */
struct mod_stack_t *mod_in_flight_write(struct mod_t *mod,
	struct mod_stack_t *older_than_stack)
{
	struct mod_stack_t *stack;

	/* No 'older_than_stack' given, return youngest write */
	if (!older_than_stack)
		return mod->write_access_list_tail;

	/* Search */
	for (stack = older_than_stack->access_list_prev; stack;
		stack = stack->access_list_prev)
		if (stack->access_kind == mod_access_store)
			return stack;

	/* Not found */
	return NULL;
}


int mod_serves_address(struct mod_t *mod, unsigned int addr)
{
	/* Address bounds */
	if (mod->range_kind == mod_range_bounds)
		return addr >= mod->range.bounds.low &&
			addr <= mod->range.bounds.high;

	/* Interleaved addresses */
	if (mod->range_kind == mod_range_interleaved)
		return (addr / mod->range.interleaved.div) %
			mod->range.interleaved.mod ==
			mod->range.interleaved.eq;

	/* Invalid */
	panic("%s: invalid range kind", __FUNCTION__);
	return 0;
}


/* Return the low module serving a given address. */
struct mod_t *mod_get_low_mod(struct mod_t *mod, unsigned int addr)
{
	struct mod_t *low_mod;
	struct mod_t *server_mod;

	/* Main memory does not have a low module */
	assert(mod_serves_address(mod, addr));
	if (mod->kind == mod_kind_main_memory 
            || mod->kind == mod_kind_dram_main_memory) // MY CODE
	{
		assert(!linked_list_count(mod->low_mod_list));
		return NULL;
	}

	/* Check which low module serves address */
	server_mod = NULL;
	LINKED_LIST_FOR_EACH(mod->low_mod_list)
	{
		/* Get new low module */
		low_mod = linked_list_get(mod->low_mod_list);
		if (!mod_serves_address(low_mod, addr))
			continue;

		/* Address served by more than one module */
		if (server_mod)
			fatal("%s: low modules %s and %s both serve address 0x%x",
				mod->name, server_mod->name, low_mod->name, addr);

		/* Assign server */
		server_mod = low_mod;
	}

	/* Error if no low module serves address */
	if (!server_mod)
		fatal("module %s: no lower module serves address 0x%x",
			mod->name, addr);

	/* Return server module */
	return server_mod;
}


int mod_get_retry_latency(struct mod_t *mod)
{
	return random() % mod->latency + mod->latency;
}


/* Check if an access to a module can be coalesced with another access older
 * than 'older_than_stack'. If 'older_than_stack' is NULL, check if it can
 * be coalesced with any in-flight access.
 * If it can, return the access that it would be coalesced with. Otherwise,
 * return NULL. */
struct mod_stack_t *mod_can_coalesce(struct mod_t *mod,
	enum mod_access_kind_t access_kind, unsigned int addr,
	struct mod_stack_t *older_than_stack)
{
	struct mod_stack_t *stack;
	struct mod_stack_t *tail;

	/* For efficiency, first check in the hash table of accesses
	 * whether there is an access in flight to the same block. */
	assert(access_kind);
	if (!mod_in_flight_address(mod, addr, older_than_stack))
		return NULL;

	/* Get youngest access older than 'older_than_stack' */
	tail = older_than_stack ? older_than_stack->access_list_prev :
		mod->access_list_tail;

	/* Coalesce depending on access kind */
	switch (access_kind)
	{

	case mod_access_load:
	{
		for (stack = tail; stack; stack = stack->access_list_prev)
		{
			/* Only coalesce with groups of reads or prefetches at the tail */
			if (stack->access_kind != mod_access_load &&
			    stack->access_kind != mod_access_prefetch)
				return NULL;

			if (stack->addr >> mod->log_block_size ==
				addr >> mod->log_block_size)
				return stack->master_stack ? stack->master_stack : stack;
		}
		break;
	}

	case mod_access_store:
	{
		/* Only coalesce with last access */
		stack = tail;
		if (!stack)
			return NULL;

		/* Only if it is a write */
		if (stack->access_kind != mod_access_store)
			return NULL;

		/* Only if it is an access to the same block */
		if (stack->addr >> mod->log_block_size != addr >> mod->log_block_size)
			return NULL;

		/* Only if previous write has not started yet */
		if (stack->port_locked)
			return NULL;

		/* Coalesce */
		return stack->master_stack ? stack->master_stack : stack;
	}

	case mod_access_nc_store:
	{
		/* Only coalesce with last access */
		stack = tail;
		if (!stack)
			return NULL;

		/* Only if it is a non-coherent write */
		if (stack->access_kind != mod_access_nc_store)
			return NULL;

		/* Only if it is an access to the same block */
		if (stack->addr >> mod->log_block_size != addr >> mod->log_block_size)
			return NULL;

		/* Only if previous write has not started yet */
		if (stack->port_locked)
			return NULL;

		/* Coalesce */
		return stack->master_stack ? stack->master_stack : stack;
	}
	case mod_access_prefetch:
		/* At this point, we know that there is another access (load/store)
		 * to the same block already in flight. Just find and return it.
		 * The caller may abort the prefetch since the block is already
		 * being fetched. */
		for (stack = tail; stack; stack = stack->access_list_prev)
		{
			if (stack->addr >> mod->log_block_size ==
				addr >> mod->log_block_size)
				return stack;
		}
		assert(!"Hash table wrongly reported another access to same block.\n");
		break;


	default:
		panic("%s: invalid access type", __FUNCTION__);
		break;
	}

	/* No access found */
	return NULL;
}


void mod_coalesce(struct mod_t *mod, struct mod_stack_t *master_stack,
	struct mod_stack_t *stack)
{
	/* Debug */
	mem_debug("  %lld %lld 0x%x %s coalesce with %lld\n", esim_cycle,
		stack->id, stack->addr, mod->name, master_stack->id);

	/* Master stack must not have a parent. We only want one level of
	 * coalesced accesses. */
	assert(!master_stack->master_stack);

	/* Access must have been recorded already, which sets the access
	 * kind to a valid value. */
	assert(stack->access_kind);

	/* Set slave stack as a coalesced access */
	stack->coalesced = 1;
	stack->master_stack = master_stack;
	assert(mod->access_list_coalesced_count <= mod->access_list_count);

	/* Record in-flight coalesced access in module */
	mod->access_list_coalesced_count++;
}

struct mod_client_info_t *mod_client_info_create(struct mod_t *mod)
{
	struct mod_client_info_t *client_info;

	/* Create object */
	client_info = repos_create_object(mod->client_info_repos);

	/* Return */
	return client_info;
}

void mod_client_info_free(struct mod_t *mod, struct mod_client_info_t *client_info)
{
	repos_free_object(mod->client_info_repos, client_info);
}


//================== MY CODE ==================//

void mod_dram_free(struct mod_t* mod)
{
   if (mod==NULL) 
   {
      return;
   }

   if (mod->dramcache_hit_info != NULL) 
   {
      struct dramcache_info_list_t * ptr = mod->dramcache_hit_info;
      struct dramcache_info_list_t * prev_ptr = NULL;
      while (ptr) 
      {
         prev_ptr = ptr;
         ptr = ptr->next;
         free(prev_ptr);
      }
      mod->dramcache_hit_info = NULL;
   }

   if (mod->dram_pending_request_head != NULL) 
   {
      struct dram_req_list_t * ptr, * pre_ptr;
      ptr = mod->dram_pending_request_head;
      pre_ptr = NULL;
      while (ptr) 
      {
         pre_ptr = ptr;
         ptr = ptr->next;
         free(pre_ptr);
      }
      mod->dram_pending_request_head = NULL;
      mod->dram_pending_request_tail = NULL;
   }

}

// insert a new node to given mod's DRAM request queue
void mod_dram_req_insert(struct mod_t *mod, struct mod_stack_t *stack, 
                         unsigned int addr, unsigned char iswrite,
                         enum dramcache_type_t access_type)
{
   struct dram_req_list_t * new_node;

   if (mod->DRAM == NULL) 
   {
      return;
   }

   new_node = (struct dram_req_list_t *) xcalloc(1, sizeof(struct dram_req_list_t));
   new_node->stack = stack;
   new_node->address = addr;
   new_node->iswrite = iswrite;
   new_node->access_type = access_type;
   new_node->next = NULL;
   
   // for new_block_allocation, the original event stack will be 
   // replied and freed before the actual block allocation time. 
   // Need to create a new one to keep set/way/tag data.
   //if (access_type == new_block_allocation) 
   //{
      /* Initialize */
	//new_node->stack = xcalloc(1, sizeof(struct mod_stack_t));
	//new_node->stack->id = stack->id;
	//new_node->stack->mod = stack->mod;
	//new_node->stack->addr = stack->addr;
	//new_node->stack->ret_event = stack->ret_event;
	//new_node->stack->ret_stack = stack->ret_stack;
	//if (stack->ret_stack != NULL)
	//	new_node->stack->client_info = stack->client_info;
	//new_node->stack->way = stack->way;
	//new_node->stack->set = stack->set;
	//new_node->stack->tag = stack->tag;
        //new_node->stack->target_mod = stack->target_mod;
        //new_node->stack->shared = stack->shared;
   //}


   if (mod->dram_pending_request_head == NULL) 
   {
      mod->dram_pending_request_head = new_node;
      mod->dram_pending_request_tail = new_node;
      new_node->prev = NULL;
      return;
   }

   new_node->prev = mod->dram_pending_request_tail;
   mod->dram_pending_request_tail->next = new_node;
   mod->dram_pending_request_tail = new_node;

   return;
}

// search with the given address in DRAM request queue and 
// return its access_type.
enum dramcache_type_t mod_dram_req_type(struct mod_t *mod, 
                                          unsigned int address, 
                                          unsigned char iswrite)
{
   struct dram_req_list_t * ptr;

   if (mod->dram_pending_request_head == NULL) 
   {
      return none;
   }

   ptr = mod->dram_pending_request_head;

   while (ptr) 
   {
      if ((ptr->address == address)&&(ptr->iswrite == iswrite))
      {

         return ptr->access_type;
      }
      ptr = ptr->next;
   }
}

// search with the given address in DRAM request queue. 
// return the found node's stack and remove the node from queue.
struct mod_stack_t * mod_dram_req_remove(struct mod_t *mod, unsigned int address, unsigned char iswrite)
{
   struct dram_req_list_t * ptr;
   struct mod_stack_t * ret;

   if (mod->dram_pending_request_head == NULL) 
   {
      return NULL;
   }

   ptr = mod->dram_pending_request_head;


   while (ptr) 
   {
      if ((ptr->address == address)&&(ptr->iswrite == iswrite))
      {
         ret = ptr->stack;

         if (ptr == mod->dram_pending_request_head) 
         {
            mod->dram_pending_request_head = ptr->next;
         }

         if (ptr == mod->dram_pending_request_tail) 
         {
            mod->dram_pending_request_tail = ptr->prev;
         }

         if (ptr->prev) 
         {
            ptr->prev->next = ptr->next;
         }
         if (ptr->next) 
         {
            ptr->next->prev = ptr->prev;
         }

         return ret;
      }
      ptr = ptr->next;
   }


   return NULL;
}

// get DRAM mod pointer from mod_list
// assume that there's only one DRAM in system. 
struct mod_t * mod_get_dram_mod(void)
{
   struct mod_t *mod;
   int i;

   for (i = 0; i < list_count(mem_system->mod_list); i++)
   {
      mod = list_get(mem_system->mod_list, i);
      if (mod->kind == mod_kind_dram_main_memory) 
      {
         return mod;
      }
   }

   return NULL;
}

// get DRAM cache mod pointer from mod_list
// assume that there's only one DRAM cache in system.
struct mod_t * mod_get_dramcache_mod(void)
{
   struct mod_t *mod;
   int i;

   for (i = 0; i < list_count(mem_system->mod_list); i++)
   {
      mod = list_get(mem_system->mod_list, i);
      if ( (mod->kind == mod_kind_cache) && (mod->DRAM!=NULL) ) 
      {
         return mod;
      }
   }

   return NULL;
}

void mod_dramcache_info_free(unsigned long long id)
{
   struct mod_t *mod = mod_get_dramcache_mod();
   struct dramcache_info_list_t * ptr;
   struct dramcache_info_list_t * ptr_prev;

   if (mod==NULL) 
      return;
   else if (mod->DRAM==NULL) 
      return;

   ptr = mod->dramcache_hit_info;
   ptr_prev = NULL;

   while (ptr) 
   {
      if ( ptr->id == id )
      {

         if (ptr == mod->dramcache_hit_info) 
         {
            mod->dramcache_hit_info = ptr->next;
         }
         else
         {
            ptr_prev->next = ptr->next; 
         }

         free (ptr);

         if (ptr_prev) 
            ptr = ptr_prev->next;
         else
            ptr = mod->dramcache_hit_info;
      }
      else 
      {
         ptr_prev = ptr;
         ptr = ptr->next;
      }
   }

   return;
}

//
void mod_insert_dramcache_info(struct mod_t *mod, unsigned long long id, unsigned int hit, unsigned int addr)
{
   struct dramcache_info_list_t * new_node;
   struct dramcache_info_list_t * ptr;

   if (mod->DRAM == NULL) 
   {
      return;
   }

   new_node = (struct dramcache_info_list_t *) xcalloc(1, sizeof(struct dramcache_info_list_t));
   new_node->hit = hit;
   new_node->id = id;
   new_node->addr = addr;
   new_node->next = NULL;
   
   //printf("mod_insert_dramcache_info: hit:%d id:%lld addr:0x%x\n",hit, id, addr);fflush(stdout);

   if (mod->dramcache_hit_info == NULL) 
   {
      mod->dramcache_hit_info = new_node;
      //printf("mod_insert_dramcache_info: finish\n");fflush(stdout);

      return;
   }

   ptr = mod->dramcache_hit_info;

   while (ptr->next) 
   {
      ptr = ptr->next;
   }

   ptr->next = new_node;
   //printf("mod_insert_dramcache_info: finish\n");fflush(stdout);
   return;
}

// 
unsigned int mod_get_dramcache_info(struct mod_t *mod, unsigned long long id, unsigned int addr)
{
   struct dramcache_info_list_t * ptr;
   struct dramcache_info_list_t * ptr_prev;
   unsigned int ret;

   if (mod->DRAM == NULL) 
   {
      return 0;
   }

   ptr = mod->dramcache_hit_info;
   ptr_prev = NULL;

   while (ptr) 
   {
      if ( (ptr->id == id) && (ptr->addr == addr) )
      {
         ret = ptr->hit;

         if (ptr == mod->dramcache_hit_info) 
         {
            mod->dramcache_hit_info = ptr->next;
         }
         else
         {
            ptr_prev->next = ptr->next; 
         }
         //printf("mod_get_dramcache_info: hit:%d id:%lld addr:0x%x ptr:0x%x\n",ret, id,addr, ptr);fflush(stdout);
         free (ptr);
         //printf("mod_get_dramcache_info: finish\n");fflush(stdout);
         return ret;
      }

      ptr_prev = ptr;
      ptr = ptr->next;
   }
   //printf("mod_get_dramcache_info: finish not found\n");fflush(stdout);
   return 0;
}

//================== END OF MY CODE ==================//
