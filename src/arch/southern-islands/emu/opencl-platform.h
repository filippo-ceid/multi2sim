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

#ifndef ARCH_SOUTHERN_ISLANDS_EMU_OPENCL_PLATFORM_H
#define ARCH_SOUTHERN_ISLANDS_EMU_OPENCL_PLATFORM_H


struct si_opencl_platform_t
{
	unsigned int id;
};

struct mem_t;  /* Forward declaration */

extern struct si_opencl_platform_t *si_opencl_platform;

struct si_opencl_platform_t *si_opencl_platform_create(void);
void si_opencl_platform_free(struct si_opencl_platform_t *platform);

unsigned int si_opencl_platform_get_info(struct si_opencl_platform_t *platform,
	unsigned int name, struct mem_t *mem, unsigned int addr, unsigned int size);

#endif
