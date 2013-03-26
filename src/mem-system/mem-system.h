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

#ifndef MEM_SYSTEM_H
#define MEM_SYSTEM_H

#include "mod-stack.h"

extern char *mem_report_file_name;

#define mem_debugging() debug_status(mem_debug_category)
#define mem_debug(...) debug(mem_debug_category, __VA_ARGS__)
extern int mem_debug_category;

#define mem_tracing() trace_status(mem_trace_category)
#define mem_trace(...) trace(mem_trace_category, __VA_ARGS__)
#define mem_trace_header(...) trace_header(mem_trace_category, __VA_ARGS__)
extern int mem_trace_category;

extern int mem_system_peer_transfers;

struct mem_system_t
{
	/* List of modules and networks */
	struct list_t *mod_list;
	struct list_t *net_list;
};

extern struct mem_system_t *mem_system;


void mem_system_init(void);
void mem_system_done(void);

void mem_system_dump_report(void);

struct mod_t *mem_system_get_mod(char *mod_name);
struct net_t *mem_system_get_net(char *net_name);

// MY CODE
void dram_read_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle);
void dram_write_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle);

void dram_add_request(struct mod_t * mod, struct mod_stack_t *stack, unsigned char iswrite);
void dram_update(void);
void dram_printstats(void);

void dramcache_read_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle);
void dramcache_write_callback(unsigned id, unsigned long long address, unsigned long long clock_cycle);

void dramcache_add_request(struct mod_t * mod, struct mod_stack_t *stack, 
                           unsigned char iswrite,enum dramcache_type_t access_type);
void dramcache_update(void);
void dramcache_printstats(void);

#endif

