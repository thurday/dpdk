/*-
 *   BSD LICENSE
 * 
 *   Copyright(c) 2010-2012 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without 
 *   modification, are permitted provided that the following conditions 
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the 
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 *       contributors may be used to endorse or promote products derived 
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *  version: DPDK.L.1.2.3-3
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_errno.h>
#include <rte_string_fns.h>
#include <rte_common.h>

#include "eal_private.h"

/* internal copy of free memory segments */
static struct rte_memseg free_memseg[RTE_MAX_MEMSEG];

/* pointer to last reserved memzone */
static unsigned memzone_idx;

/*
 * Return a pointer to a correctly filled memzone descriptor. If the
 * allocation cannot be done, return NULL.
 */
const struct rte_memzone *
rte_memzone_reserve(const char *name, uint64_t len, int socket_id,
		      unsigned flags)
{
	return rte_memzone_reserve_aligned(name,
			len, socket_id, flags, CACHE_LINE_SIZE);
}

/*
 * Return a pointer to a correctly filled memzone descriptor (with a
 * specified alignment). If the allocation cannot be done, return NULL.
 */
const struct rte_memzone *
rte_memzone_reserve_aligned(const char *name, uint64_t len,
		int socket_id, unsigned flags, unsigned align)
{
	struct rte_config *config;
	unsigned i = 0;
	int memseg_idx = -1;
	uint64_t requested_len;
	uint64_t memseg_len = 0;
	phys_addr_t memseg_physaddr;
	void *memseg_addr;
	uintptr_t addr_offset;

	/* if secondary processes return error */
	if (rte_eal_process_type() == RTE_PROC_SECONDARY){
		RTE_LOG(ERR, EAL, "%s(): Not allowed in secondary process\n", __func__);
		rte_errno = E_RTE_SECONDARY;
		return NULL;
	}

	/* if alignment is not a power of two */
	if (!rte_is_power_of_2(align)) {
		RTE_LOG(ERR, EAL, "%s(): Invalid alignment: %u\n", __func__,
				align);
		rte_errno = EINVAL;
		return NULL;
	}

	/* alignment less than cache size is not allowed */
	if (align < CACHE_LINE_SIZE)
		align = CACHE_LINE_SIZE;

	/* get pointer to global configuration */
	config = rte_eal_get_configuration();

	/* no more room in config */
	if (memzone_idx >= RTE_MAX_MEMZONE) {
		RTE_LOG(ERR, EAL, "%s(): No more room in config\n", __func__);
		rte_errno = ENOSPC;
		return NULL;
	}

	/* both sizes cannot be explicitly called for */
	if ((flags & RTE_MEMZONE_1GB) && (flags & RTE_MEMZONE_2MB)) {
		rte_errno = EINVAL;
		return NULL;
	}

	/* zone already exist */
	if (rte_memzone_lookup(name) != NULL) {
		RTE_LOG(DEBUG, EAL, "%s(): memzone <%s> already exists\n",
			__func__, name);
		rte_errno = EEXIST;
		return NULL;
	}

	/* align length on cache boundary */
	len += CACHE_LINE_MASK;
	len &= ~((uint64_t)CACHE_LINE_MASK);



	/* save requested length */
	requested_len = len;

	/* reserve extra space for future alignment */
	if (len)
		len += align;

	/* find the smallest segment matching requirements */
	for (i = 0; i < RTE_MAX_MEMSEG; i++) {

		/* last segment */
		if (free_memseg[i].addr == NULL)
			break;

		/* empty segment, skip it */
		if (free_memseg[i].len == 0)
			continue;

		/* bad socket ID */
		if (socket_id != SOCKET_ID_ANY &&
		    socket_id != free_memseg[i].socket_id)
			continue;

		/* check len */
		if (len != 0 && len > free_memseg[i].len)
			continue;

		/* check flags for hugepage sizes */
		if ((flags & RTE_MEMZONE_2MB) &&
				free_memseg[i].hugepage_sz == RTE_PGSIZE_1G )
			continue;
		if ((flags & RTE_MEMZONE_1GB) &&
				free_memseg[i].hugepage_sz == RTE_PGSIZE_2M )
			continue;

		/* this segment is the best until now */
		if (memseg_idx == -1) {
			memseg_idx = i;
			memseg_len = free_memseg[i].len;
		}
		/* find the biggest contiguous zone */
		else if (len == 0) {
			if (free_memseg[i].len > memseg_len) {
				memseg_idx = i;
				memseg_len = free_memseg[i].len;
			}
		}
		/*
		 * find the smallest (we already checked that current
		 * zone length is > len
		 */
		else if (free_memseg[i].len < memseg_len) {
			memseg_idx = i;
			memseg_len = free_memseg[i].len;
		}
	}

	/* no segment found */
	if (memseg_idx == -1) {
		/*
		 * If RTE_MEMZONE_SIZE_HINT_ONLY flag is specified,
		 * try allocating again without the size parameter otherwise -fail.
		 */
		if ((flags & RTE_MEMZONE_SIZE_HINT_ONLY)  &&
                ((flags & RTE_MEMZONE_1GB) || (flags & RTE_MEMZONE_2MB)))
			return rte_memzone_reserve_aligned(name, len - align,
					socket_id, 0, align);

		RTE_LOG(ERR, EAL, "%s(): No appropriate segment found\n", __func__);
		rte_errno = ENOMEM;
		return NULL;
	}

	/* get offset needed to adjust alignment */
	addr_offset = (uintptr_t) RTE_PTR_SUB(
			RTE_ALIGN_CEIL(free_memseg[memseg_idx].addr, (uintptr_t) align),
			(uintptr_t) free_memseg[memseg_idx].addr);

	/* save aligned physical and virtual addresses */
	memseg_physaddr = free_memseg[memseg_idx].phys_addr + addr_offset;
	memseg_addr = RTE_PTR_ADD(free_memseg[memseg_idx].addr, addr_offset);

	/* if we are looking for a biggest memzone */
	if (requested_len == 0)
		requested_len = memseg_len - addr_offset;

	/* set length to correct value */
	len = addr_offset + requested_len;

	/* update our internal state */
	free_memseg[memseg_idx].len -= len;
	free_memseg[memseg_idx].phys_addr += len;
	free_memseg[memseg_idx].addr =
		(char *)free_memseg[memseg_idx].addr + len;

	/* fill the zone in config */
	struct rte_memzone *mz = &config->mem_config->memzone[memzone_idx++];
	rte_snprintf(mz->name, sizeof(mz->name), "%s", name);
	mz->phys_addr = memseg_physaddr;
	mz->addr = memseg_addr;
	mz->len = requested_len;
	mz->hugepage_sz = free_memseg[memseg_idx].hugepage_sz;
	mz->socket_id = free_memseg[memseg_idx].socket_id;
	mz->flags = 0;

	return mz;
}

/*
 * Lookup for the memzone identified by the given name
 */
const struct rte_memzone *
rte_memzone_lookup(const char *name)
{
	const struct rte_mem_config *mcfg;
	unsigned i = 0;

	/* get pointer to global configuration */
	mcfg = rte_eal_get_configuration()->mem_config;

	/*
	 * the algorithm is not optimal (linear), but there are few
	 * zones and this function should be called at init only
	 */
	for (i = 0; i < RTE_MAX_MEMZONE && mcfg->memzone[i].addr != NULL; i++) {
		if (!strncmp(name, mcfg->memzone[i].name, RTE_MEMZONE_NAMESIZE))
			return &mcfg->memzone[i];
	}
	return NULL;
}

/* Dump all reserved memory zones on console */
void
rte_memzone_dump(void)
{
	const struct rte_mem_config *mcfg;
	unsigned i = 0;

	/* get pointer to global configuration */
	mcfg = rte_eal_get_configuration()->mem_config;

	/* dump all zones */
	for (i=0; i<RTE_MAX_MEMZONE; i++) {
		if (mcfg->memzone[i].addr == NULL)
			break;
		printf("name:<%s>, phys:0x%"PRIx64", len:0x%"PRIx64""
		       ", virt:%p, socket_id:%"PRId32"\n",
		       mcfg->memzone[i].name,
		       mcfg->memzone[i].phys_addr,
		       mcfg->memzone[i].len,
		       mcfg->memzone[i].addr,
		       mcfg->memzone[i].socket_id);
	}
}

/*
 * called by init: modify the free memseg list to have cache-aligned
 * addresses and cache-aligned lengths
 */
static int
memseg_sanitize(struct rte_memseg *memseg)
{
	unsigned phys_align;
	unsigned virt_align;
	unsigned off;

	phys_align = memseg->phys_addr & CACHE_LINE_MASK;
	virt_align = (unsigned long)memseg->addr & CACHE_LINE_MASK;

	/*
	 * sanity check: phys_addr and addr must have the same
	 * alignment
	 */
	if (phys_align != virt_align)
		return -1;

	/* memseg is really too small, don't bother with it */
	if (memseg->len < (2 * CACHE_LINE_SIZE)) {
		memseg->len = 0;
		return 0;
	}

	/* align start address */
	off = (CACHE_LINE_SIZE - phys_align) & CACHE_LINE_MASK;
	memseg->phys_addr += off;
	memseg->addr = (char *)memseg->addr + off;
	memseg->len -= off;

	/* align end address */
	memseg->len &= ~((uint64_t)CACHE_LINE_MASK);

	return 0;
}

/*
 * Init the memzone subsystem
 */
int
rte_eal_memzone_init(void)
{
	struct rte_config *config;
	const struct rte_memseg *memseg;
	unsigned i = 0;

	/* secondary processes don't need to initialise anything */
	if (rte_eal_process_type() == RTE_PROC_SECONDARY)
		return 0;

	/* get pointer to global configuration */
	config = rte_eal_get_configuration();

	memseg = rte_eal_get_physmem_layout();
	if (memseg == NULL) {
		RTE_LOG(ERR, EAL, "%s(): Cannot get physical layout\n", __func__);
		return -1;
	}

	/* duplicate the memsegs from config */
	memcpy(free_memseg, memseg, sizeof(free_memseg));

	/* make all zones cache-aligned */
	for (i=0; i<RTE_MAX_MEMSEG; i++) {
		if (free_memseg[i].addr == NULL)
			break;
		if (memseg_sanitize(&free_memseg[i]) < 0) {
			RTE_LOG(ERR, EAL, "%s(): Sanity check failed\n", __func__);
			return -1;
		}
	}

	/* delete all zones */
	memzone_idx = 0;
	memset(config->mem_config->memzone, 0, sizeof(config->mem_config->memzone));

	return 0;
}
