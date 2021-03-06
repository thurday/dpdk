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

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <termios.h>
#ifndef __linux__
#include <net/socket.h>
#endif
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_timer.h>

#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_ipaddr.h>
#include <cmdline_parse_num.h>
#include <cmdline_parse_string.h>
#include <cmdline.h>

#include "test.h"

/****************/

struct cmd_autotest_result {
	cmdline_fixed_string_t autotest;
};

static void cmd_autotest_parsed(void *parsed_result,
				__attribute__((unused)) struct cmdline *cl,
				__attribute__((unused)) void *data)
{
	struct cmd_autotest_result *res = parsed_result;
	int ret = 0;
	int all = 0;

	if (!strcmp(res->autotest, "all_autotests"))
		all = 1;

	if (all || !strcmp(res->autotest, "version_autotest"))
		ret |= test_version();
	if (all || !strcmp(res->autotest, "debug_autotest"))
		ret |= test_debug();
	if (all || !strcmp(res->autotest, "pci_autotest"))
		ret |= test_pci();
	if (all || !strcmp(res->autotest, "prefetch_autotest"))
		ret |= test_prefetch();
	if (all || !strcmp(res->autotest, "byteorder_autotest"))
		ret |= test_byteorder();
	if (all || !strcmp(res->autotest, "per_lcore_autotest"))
		ret |= test_per_lcore();
	if (all || !strcmp(res->autotest, "atomic_autotest"))
		ret |= test_atomic();
	if (all || !strcmp(res->autotest, "malloc_autotest"))
		ret |= test_malloc();
	if (all || !strcmp(res->autotest, "spinlock_autotest"))
		ret |= test_spinlock();
	if (all || !strcmp(res->autotest, "memory_autotest"))
		ret |= test_memory();
	if (all || !strcmp(res->autotest, "memzone_autotest"))
		ret |= test_memzone();
	if (all || !strcmp(res->autotest, "rwlock_autotest"))
		ret |= test_rwlock();
	if (all || !strcmp(res->autotest, "mbuf_autotest"))
		ret |= test_mbuf();
	if (all || !strcmp(res->autotest, "logs_autotest"))
		ret |= test_logs();
	if (all || !strcmp(res->autotest, "errno_autotest"))
		ret |= test_errno();
	if (all || !strcmp(res->autotest, "hash_autotest"))
		ret |= test_hash();
	if (all || !strcmp(res->autotest, "lpm_autotest"))
		ret |= test_lpm();
	if (all || !strcmp(res->autotest, "cpuflags_autotest"))
		ret |= test_cpuflags();
	/* tailq autotest must go after all lpm and hashs tests or any other
	 * tests which need to create tailq objects (ring and mempool are implicitly
	 * created in earlier tests so can go later)
	 */
	if (all || !strcmp(res->autotest, "tailq_autotest"))
		ret |= test_tailq();
	if (all || !strcmp(res->autotest, "multiprocess_autotest"))
		ret |= test_mp_secondary();
	if (all || !strcmp(res->autotest, "memcpy_autotest"))
		ret |= test_memcpy();
	if (all || !strcmp(res->autotest, "string_autotest"))
		ret |= test_string_fns();
	if (all || !strcmp(res->autotest, "eal_flags_autotest"))
		ret |= test_eal_flags();
	if (all || !strcmp(res->autotest, "alarm_autotest"))
		ret |= test_alarm();
	if (all || !strcmp(res->autotest, "interrupt_autotest"))
		ret |= test_interrupt();
	if (all || !strcmp(res->autotest, "cycles_autotest"))
		ret |= test_cycles();
	if (all || !strcmp(res->autotest, "ring_autotest"))
		ret |= test_ring();
	if (all || !strcmp(res->autotest, "timer_autotest"))
		ret |= test_timer();
	if (all || !strcmp(res->autotest, "mempool_autotest"))
		ret |= test_mempool();

	if (ret == 0)
		printf("Test OK\n");
	else
		printf("Test Failed\n");
	fflush(stdout);
}

cmdline_parse_token_string_t cmd_autotest_autotest =
	TOKEN_STRING_INITIALIZER(struct cmd_autotest_result, autotest,
			"pci_autotest#memory_autotest#"
			"per_lcore_autotest#spinlock_autotest#"
			"rwlock_autotest#atomic_autotest#"
			"byteorder_autotest#prefetch_autotest#"
			"cycles_autotest#logs_autotest#"
			"memzone_autotest#ring_autotest#"
			"mempool_autotest#mbuf_autotest#"
			"timer_autotest#malloc_autotest#"
			"memcpy_autotest#hash_autotest#"
			"lpm_autotest#debug_autotest#"
			"errno_autotest#tailq_autotest#"
			"string_autotest#multiprocess_autotest#"
			"cpuflags_autotest#eal_flags_autotest#"
			"alarm_autotest#interrupt_autotest#"
			"version_autotest#"
			"all_autotests");

cmdline_parse_inst_t cmd_autotest = {
	.f = cmd_autotest_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = "launch autotest",
	.tokens = {        /* token list, NULL terminated */
		(void *)&cmd_autotest_autotest,
		NULL,
	},
};

/****************/

struct cmd_dump_result {
	cmdline_fixed_string_t dump;
};

static void
dump_struct_sizes(void)
{
#define DUMP_SIZE(t) printf("sizeof(" #t ") = %u\n", (unsigned)sizeof(t));
	DUMP_SIZE(struct rte_mbuf);
	DUMP_SIZE(struct rte_pktmbuf);
	DUMP_SIZE(struct rte_ctrlmbuf);
	DUMP_SIZE(struct rte_mempool);
	DUMP_SIZE(struct rte_ring);
#undef DUMP_SIZE
}

static void cmd_dump_parsed(void *parsed_result,
			    __attribute__((unused)) struct cmdline *cl,
			    __attribute__((unused)) void *data)
{
	struct cmd_dump_result *res = parsed_result;

	if (!strcmp(res->dump, "dump_physmem"))
		rte_dump_physmem_layout();
	else if (!strcmp(res->dump, "dump_memzone"))
		rte_memzone_dump();
	else if (!strcmp(res->dump, "dump_log_history"))
		rte_log_dump_history();
	else if (!strcmp(res->dump, "dump_struct_sizes"))
		dump_struct_sizes();
	else if (!strcmp(res->dump, "dump_ring"))
		rte_ring_list_dump();
	else if (!strcmp(res->dump, "dump_mempool"))
		rte_mempool_list_dump();
}

cmdline_parse_token_string_t cmd_dump_dump =
	TOKEN_STRING_INITIALIZER(struct cmd_dump_result, dump,
				 "dump_physmem#dump_memzone#dump_log_history#"
				 "dump_struct_sizes#dump_ring#dump_mempool");

cmdline_parse_inst_t cmd_dump = {
	.f = cmd_dump_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = "dump status",
	.tokens = {        /* token list, NULL terminated */
		(void *)&cmd_dump_dump,
		NULL,
	},
};

/****************/

struct cmd_dump_one_result {
	cmdline_fixed_string_t dump;
	cmdline_fixed_string_t name;
};

static void cmd_dump_one_parsed(void *parsed_result, struct cmdline *cl,
				__attribute__((unused)) void *data)
{
	struct cmd_dump_one_result *res = parsed_result;

	if (!strcmp(res->dump, "dump_ring")) {
		struct rte_ring *r;
		r = rte_ring_lookup(res->name);
		if (r == NULL) {
			cmdline_printf(cl, "Cannot find ring\n");
			return;
		}
		rte_ring_dump(r);
	}
	else if (!strcmp(res->dump, "dump_mempool")) {
		struct rte_mempool *mp;
		mp = rte_mempool_lookup(res->name);
		if (mp == NULL) {
			cmdline_printf(cl, "Cannot find mempool\n");
			return;
		}
		rte_mempool_dump(mp);
	}
}

cmdline_parse_token_string_t cmd_dump_one_dump =
	TOKEN_STRING_INITIALIZER(struct cmd_dump_one_result, dump,
				 "dump_ring#dump_mempool");

cmdline_parse_token_string_t cmd_dump_one_name =
	TOKEN_STRING_INITIALIZER(struct cmd_dump_one_result, name, NULL);

cmdline_parse_inst_t cmd_dump_one = {
	.f = cmd_dump_one_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = "dump one ring/mempool: dump_ring|dump_mempool <name>",
	.tokens = {        /* token list, NULL terminated */
		(void *)&cmd_dump_one_dump,
		(void *)&cmd_dump_one_name,
		NULL,
	},
};

/****************/

struct cmd_set_ring_result {
	cmdline_fixed_string_t set;
	cmdline_fixed_string_t name;
	uint32_t value;
};

static void cmd_set_ring_parsed(void *parsed_result, struct cmdline *cl,
				__attribute__((unused)) void *data)
{
	struct cmd_set_ring_result *res = parsed_result;
	struct rte_ring *r;
	int ret;

	r = rte_ring_lookup(res->name);
	if (r == NULL) {
		cmdline_printf(cl, "Cannot find ring\n");
		return;
	}

	if (!strcmp(res->set, "set_quota")) {
		ret = rte_ring_set_bulk_count(r, res->value);
		if (ret != 0)
			cmdline_printf(cl, "Cannot set quota\n");
	}
	else if (!strcmp(res->set, "set_watermark")) {
		ret = rte_ring_set_water_mark(r, res->value);
		if (ret != 0)
			cmdline_printf(cl, "Cannot set water mark\n");
	}
}

cmdline_parse_token_string_t cmd_set_ring_set =
	TOKEN_STRING_INITIALIZER(struct cmd_set_ring_result, set,
				 "set_quota#set_watermark");

cmdline_parse_token_string_t cmd_set_ring_name =
	TOKEN_STRING_INITIALIZER(struct cmd_set_ring_result, name, NULL);

cmdline_parse_token_num_t cmd_set_ring_value =
	TOKEN_NUM_INITIALIZER(struct cmd_set_ring_result, value, UINT32);

cmdline_parse_inst_t cmd_set_ring = {
	.f = cmd_set_ring_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = "set quota/watermark: "
			"set_quota|set_watermark <ring_name> <value>",
	.tokens = {        /* token list, NULL terminated */
		(void *)&cmd_set_ring_set,
		(void *)&cmd_set_ring_name,
		(void *)&cmd_set_ring_value,
		NULL,
	},
};

/****************/

struct cmd_quit_result {
	cmdline_fixed_string_t quit;
};

static void
cmd_quit_parsed(__attribute__((unused)) void *parsed_result,
		struct cmdline *cl,
		__attribute__((unused)) void *data)
{
	cmdline_quit(cl);
}

cmdline_parse_token_string_t cmd_quit_quit =
	TOKEN_STRING_INITIALIZER(struct cmd_quit_result, quit,
				 "quit");

cmdline_parse_inst_t cmd_quit = {
	.f = cmd_quit_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = "exit application",
	.tokens = {        /* token list, NULL terminated */
		(void *)&cmd_quit_quit,
		NULL,
	},
};

/****************/

cmdline_parse_ctx_t main_ctx[] = {
	(cmdline_parse_inst_t *)&cmd_autotest,
	(cmdline_parse_inst_t *)&cmd_dump,
	(cmdline_parse_inst_t *)&cmd_dump_one,
	(cmdline_parse_inst_t *)&cmd_set_ring,
	(cmdline_parse_inst_t *)&cmd_quit,
	NULL,
};

