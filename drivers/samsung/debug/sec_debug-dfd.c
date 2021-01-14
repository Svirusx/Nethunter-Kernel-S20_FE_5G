/*
 * Copyright (c) 2012-2013 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/shrinker.h>
#include <linux/circ_buf.h>

static int dfd_enable(const char *val, struct kernel_param *kp);
module_param_call(enable, dfd_enable, NULL, NULL,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

/* Double free detector(dfd) can use lots of memory because
 * it needs to hold on the freed slabs, otherwise
 * the the freeing node will be put into the kmem cache
 * of it's size and the slab allocator will soon re-allocate
 * this slab when the slab of that size is requested.
 * So to alleviate the pressure of the other shrinkers when
 * there is a memory pressure, enable DFD_HAS_SHRINKER below.
 */
#define DFD_HAS_SHRINKER

#ifdef DFD_HAS_SHRINKER
/* Using DFD shrinker will keep the DFD buffer entries low
 * but at a cost. The page allocator will often go into
 * the slowpath and try to reclaim pages and eventually call
 * the shrinker.
 * If you want to avoid this overhead, enable below feature
 * to completely flush out the circular buffer and disable the
 * DFD when there is a memory pressure */
//#define DFD_SHRINKER_DISABLE_DFD_ON_MEM_PRESSURE
#endif

#define KFREE_HOOK_BYPASS_MASK 0x1
/* The average size of a slab object is about 256 bytes.
 * 1<<15 number of slab objects take about 8MB to 10MB
 * (This average was mesaured with a min slab size of 64) */
#define KFREE_CIRC_BUF_SIZE (1<<15)
#define KFREE_FREE_MAGIC 0x65655266

static int dfd_panic = 1;
static int dfd_disabled;

static DEFINE_SPINLOCK(dfd_list_lock);

struct dfd_node {
	void *addr;
	void *caller;
};

struct dfd_node_list {
	int head;
	int tail;
	struct dfd_node entry[KFREE_CIRC_BUF_SIZE];
};

struct dfd_node_list dfd_node_list;

static int __init setup_dfd_panic_disable(char *str)
{
	dfd_panic = 0;
	return 1;
}
__setup("dfd_panic_disable", setup_dfd_panic_disable);

/* the caller must hold the dfd_list_lock */
static void *circ_buf_lookup(struct dfd_node_list *circ_buf, void *addr)
{
	int i;
	for (i = circ_buf->tail; i != circ_buf->head ;
		i = (i + 1) & (KFREE_CIRC_BUF_SIZE - 1)) {
		if (circ_buf->entry[i].addr == addr)
			return &circ_buf->entry[i];
	}

	return NULL;
}

/* the caller must hold the dfd_list_lock and must check
 * for the buffer status before calling */
static void *circ_buf_get(struct dfd_node_list *circ_buf)
{
	void *entry;
	entry = &circ_buf->entry[circ_buf->tail];
	smp_rmb();
	circ_buf->tail = (circ_buf->tail + 1) &
		(KFREE_CIRC_BUF_SIZE - 1);
	return entry;
}

/* the caller must hold the dfd_list_lock and must check
 * for the buffer status before calling */
static void *circ_buf_put(struct dfd_node_list *circ_buf,
				struct dfd_node *entry)
{
	memcpy(&circ_buf->entry[circ_buf->head], entry, sizeof(*entry));
	smp_wmb();
	circ_buf->head = (circ_buf->head + 1) &
		(KFREE_CIRC_BUF_SIZE - 1);
	return entry;
}

static int dfd_flush(void)
{
	struct dfd_node *pentry;
	unsigned long cnt;
	unsigned long flags;

	spin_lock_irqsave(&dfd_list_lock, flags);
	cnt = CIRC_CNT(dfd_node_list.head, dfd_node_list.tail,
		KFREE_CIRC_BUF_SIZE);
	spin_unlock_irqrestore(&dfd_list_lock, flags);
	pr_debug("cnt=%lu\n", cnt);

do_flush:
	while (cnt) {
		void *tofree = NULL;
		/* we want to keep the lock region as short as possible
		 * so we will re-read the buf count every loop */
		spin_lock_irqsave(&dfd_list_lock, flags);
		cnt = CIRC_CNT(dfd_node_list.head, dfd_node_list.tail,
			KFREE_CIRC_BUF_SIZE);
		if (cnt == 0) {
			spin_unlock_irqrestore(&dfd_list_lock, flags);
			break;
		}
		if ((pentry = circ_buf_get(&dfd_node_list)) != NULL)
			tofree = pentry->addr;
		spin_unlock_irqrestore(&dfd_list_lock, flags);
		if (tofree)
			kfree((void *)((unsigned long)tofree |
				KFREE_HOOK_BYPASS_MASK));
		cnt--;
	}

	spin_lock_irqsave(&dfd_list_lock, flags);
	cnt = CIRC_CNT(dfd_node_list.head, dfd_node_list.tail,
		KFREE_CIRC_BUF_SIZE);
	spin_unlock_irqrestore(&dfd_list_lock, flags);

	if (!dfd_disabled)
		goto out;

	if (cnt)
		goto do_flush;

out:
	return cnt;
}

static int dfd_enable(const char *val, struct kernel_param *kp)
{
	if (!strncmp(val, "1", 1)) {
		dfd_disabled = 0;
		pr_info("double free detection is enabled\n");
	} else if (!strncmp(val, "0", 1)) {
		dfd_disabled = 1;
		dfd_flush();
		pr_info("double free detection is disabled\n");
	}

	return 0;
}

#ifdef DFD_HAS_SHRINKER
int dfd_shrink(struct shrinker *shrinker, struct shrink_control *sc)
{
#ifndef DFD_SHRINKER_DISABLE_DFD_ON_MEM_PRESSURE
	struct dfd_node *pentry;
	unsigned long nr = sc->nr_to_scan;
#endif
	unsigned long flags;
	unsigned long nr_objs;

	spin_lock_irqsave(&dfd_list_lock, flags);
	nr_objs = CIRC_CNT(dfd_node_list.head, dfd_node_list.tail,
		KFREE_CIRC_BUF_SIZE);
	spin_unlock_irqrestore(&dfd_list_lock, flags);

	/* nothing to reclaim from here */
	if (nr_objs == 0) {
		nr_objs = -1;
		goto out;
	}

#ifdef DFD_SHRINKER_DISABLE_DFD_ON_MEM_PRESSURE
	/* disable double free detection. This will flush
	 * the entire circular buffer out. */
	dfd_disable();
#else
	/* return max slab objects freeable */
	if (nr == 0)
		return  nr_objs;

	if (nr > nr_objs)
		nr = nr_objs;

	pr_debug("nr_objs=%lu\n", nr_objs);
	while (nr) {
		unsigned long cnt;
		void *tofree = NULL;
		spin_lock_irqsave(&dfd_list_lock, flags);
		cnt = CIRC_CNT(dfd_node_list.head, dfd_node_list.tail,
			KFREE_CIRC_BUF_SIZE);
		if (cnt > 0) {
			if ((pentry = circ_buf_get(&dfd_node_list)) != NULL)
				tofree = pentry->addr;
		}
		spin_unlock_irqrestore(&dfd_list_lock, flags);
		if (tofree)
			kfree((void *)((unsigned long)tofree |
				KFREE_HOOK_BYPASS_MASK));
		nr--;
	}
#endif
	spin_lock_irqsave(&dfd_list_lock, flags);
	nr_objs = CIRC_CNT(dfd_node_list.head, dfd_node_list.tail,
		KFREE_CIRC_BUF_SIZE);
	spin_unlock_irqrestore(&dfd_list_lock, flags);
	if (nr_objs == 0) {
		pr_info("nothing more to reclaim from here!\n");
		nr_objs = -1;
	}

out:
	return nr_objs;
}

static struct shrinker dfd_shrinker = {
	.shrink = dfd_shrink,
	.seeks = DEFAULT_SEEKS
};

static int __init dfd_shrinker_init(void)
{
	register_shrinker(&dfd_shrinker);
	return 0;
}

static void __exit dfd_shrinker_exit(void)
{
	unregister_shrinker(&dfd_shrinker);
}

module_init(dfd_shrinker_init);
module_exit(dfd_shrinker_exit);
#endif

static inline int dfd_check_magic_any(void *addr)
{
	return (((unsigned int *)addr)[0] == KFREE_FREE_MAGIC ||
		((unsigned int *)addr)[1] == KFREE_FREE_MAGIC ||
		((unsigned int *)addr)[2] == KFREE_FREE_MAGIC ||
		((unsigned int *)addr)[3] == KFREE_FREE_MAGIC);
}

static inline int dfd_check_magic_all(void *addr)
{
	return (((unsigned int *)addr)[0] == KFREE_FREE_MAGIC &&
		((unsigned int *)addr)[1] == KFREE_FREE_MAGIC &&
		((unsigned int *)addr)[2] == KFREE_FREE_MAGIC &&
		((unsigned int *)addr)[3] == KFREE_FREE_MAGIC);
}

static inline void dfd_set_magic(void *addr)
{
	BUILD_BUG_ON(KMALLOC_MIN_SIZE < 16);
	((unsigned long *)addr)[0] = KFREE_FREE_MAGIC;
	((unsigned long *)addr)[1] = KFREE_FREE_MAGIC;
	((unsigned long *)addr)[2] = KFREE_FREE_MAGIC;
	((unsigned long *)addr)[3] = KFREE_FREE_MAGIC;
}

static inline void dfd_clear_magic(void *addr)
{
	BUILD_BUG_ON(KMALLOC_MIN_SIZE < 16);
	((unsigned long *)addr)[0] = 0;
	((unsigned long *)addr)[1] = 0;
	((unsigned long *)addr)[2] = 0;
	((unsigned long *)addr)[3] = 0;
}

static void __hexdump(void *mem, unsigned long size)
{
	#define WORDS_PER_LINE 4
	#define WORD_SIZE 4
	#define LINE_SIZE (WORDS_PER_LINE * WORD_SIZE)
	#define LINE_BUF_SIZE (WORDS_PER_LINE * WORD_SIZE * 3 \
		+ WORDS_PER_LINE + 4)
	unsigned long addr;
	char linebuf[LINE_BUF_SIZE];
	int numline = size / LINE_SIZE;
	int i;
	for (i = 0; i < numline; i++) {
		addr = (unsigned long)mem + i * LINE_SIZE;
		hex_dump_to_buffer((const void *)addr,
			LINE_SIZE, LINE_SIZE,
			WORD_SIZE, linebuf, sizeof(linebuf), 1);
		pr_info(" %lx : %s\n", addr, linebuf);
	}

}

void *kfree_hook(void *p, void *caller)
{
	unsigned long flags;
	struct dfd_node *match = NULL;
	void *tofree = NULL;
	unsigned long addr = (unsigned long)p;
	struct dfd_node entry;
	struct dfd_node *pentry;

	if (!virt_addr_valid(addr)) {
		/* there are too many NULL pointers so don't print for NULL */
		if (addr)
			pr_debug("trying to free an invalid addr %lx" \
				"from %pS\n", addr, caller);
		return NULL;
	}

	if (addr & KFREE_HOOK_BYPASS_MASK || dfd_disabled) {
		/* return original address to free */
		return (void *)(addr&~(KFREE_HOOK_BYPASS_MASK));
	}

	spin_lock_irqsave(&dfd_list_lock, flags);

	if (dfd_node_list.head == 0)
		pr_debug("circular buffer head rounded to zero.");

	/* We can detect all the double free in the circular buffer time frame
	 * if we scan the whole circular buffer all the time, but to minimize
	 * the performance degradation we will just check for the magic values
	 * (the number of magic values can be up to KMALLOC_MIN_SIZE/4) */
	if (dfd_check_magic_any(p)) {
		/* memory that is to be freed may originally have had magic
		 * value, so search the whole circ buf for an actual match */
		match = circ_buf_lookup(&dfd_node_list, p);
		if (!match) {
			pr_debug("magic set but not in circ buf\n");
		}
	}

	if (match) {
		pr_err("0x%08lx was already freed by %pS()\n",
			(unsigned long)p, match->caller);
		spin_unlock_irqrestore(&dfd_list_lock, flags);
		if (dfd_panic)
			panic("double free detected!");
		/* if we don't panic we just return without adding this entry
		 * to the circular buffer. This means that this kfree is ommited
		 * and we are just forgiving the double free */
		dump_stack();
		return NULL;
	}

	/* mark free magic on the freeing node */
	dfd_set_magic(p);

	/* do an actual kfree for the oldest entry
	 * if the circular buffer is full */
	if (CIRC_SPACE(dfd_node_list.head, dfd_node_list.tail,
		KFREE_CIRC_BUF_SIZE) == 0) {
		pentry = circ_buf_get(&dfd_node_list);
		if (pentry)
			tofree = pentry->addr;
	}

	/* add the new entry to the circular buffer */
	entry.addr = p;
	entry.caller = caller;
	circ_buf_put(&dfd_node_list, &entry);
	if (tofree) {
		if (unlikely(!dfd_check_magic_all(tofree))) {
			pr_emerg("Use after free detected on the node " \
				"0x%lx which was freed by %pS.\n", \
				(unsigned long)tofree,
				pentry->caller);
			__hexdump((void *)tofree, KMALLOC_MIN_SIZE);
			pr_err("\n");
		}
		dfd_clear_magic(tofree);
		spin_unlock_irqrestore(&dfd_list_lock, flags);
		/* do the real kfree */
		kfree((void *)((unsigned long)tofree | KFREE_HOOK_BYPASS_MASK));
		return NULL;
	}

	spin_unlock_irqrestore(&dfd_list_lock, flags);
	return NULL;
}
