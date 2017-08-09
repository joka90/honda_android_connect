/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
#include <asm/uaccess.h>
#include "tsc.h"
#include "region.h"

static u32 var_max_log_size(struct lites_region *region)
{
	return LITES_ACCESS_BYTE_MAX - region->header_size;
}

static u32 var_log_size(struct lites_region *region, u32 index)
{
	lites_assert(index <= region->total);

	if (index >= region->total)
		index = 0;

	return *(u16 *)(&region->rb->log[index]);
}

static u32 var_writable_size(struct lites_region *region)
{
	u32 read, write;

	read  = region->rb->read;
	write = region->rb->write;

	if (write + region->header_size >= region->total)
		write = 0;

	lites_pr_dbg("read = %08x, write = %08x, region->total = %08x, region->rb->flags = %08x",
		     read, write, region->total, region->rb->flags);

	/** データが書き込むスペースはwriteとreadで算出される。 */
	if (read == write && !(region->rb->flags & LITES_FLAG_READ))
		return region->total; /** There is no data */

	return (read - write + region->total) % region->total;
}

static u32 var_next_index(struct lites_region *region, u32 index)
{
	u32 next_index;

	lites_pr_dbg("index = %08x, region->total = %08x", index, region->total);

	if (index + region->header_size > region->total)
		index = 0;

	next_index = (var_log_size(region, index) + region->header_size + index) % region->total;

	if (next_index + region->header_size > region->total)
		next_index = 0;

	return next_index;
}

static s64 var_calc_index(struct lites_region *region, u32 index, u32 offset)
{
	s64 __offset = (s64)offset;

	lites_assert(index <= region->total);

	if (index + region->header_size > region->total)
		index = 0;

	while (__offset > 0) {
		__offset -= var_log_size(region, index);
		index = var_next_index(region, index);
		if (index == region->rb->write)
			return -EINVAL;
	}

	return index;
}

static s64 var_calc_index_with_header(struct lites_region *region, u32 index, u32 offset)
{
	s64 __offset = (s64)offset;

	lites_assert(index <= region->total);

	if (index + region->header_size > region->total)
		index = 0;

	while (__offset > 0) {
		__offset -= var_log_size(region, index) + region->header_size;
		index = var_next_index(region, index);
		if (index == region->rb->write)
			return -EINVAL;
	}

	return index;
}

static s64 var_update_index(struct lites_region *region, u32 start, u32 index, u16 size, u32 flags)
{
	u32 avail_size, next_index, total = 0;

	lites_pr_dbg("start = %08x, index = %08x, size = %08x, flags = %08x",
		     start, index, size, flags);
	lites_assert(start <= region->total && index <= region->total);

	if (start >= region->total)
		start = 0;
	if (index >= region->total)
		index = 0;

	if (index + region->header_size > region->total)
		index = 0;
	if (start + region->header_size > region->total)
		start = 0;

	/** There is no data */
	if (index == start && !flags)
		return index;

	while (1) {
		avail_size = (index - start + region->total) % region->total;
		if (avail_size >= size + region->header_size)
			break;

		next_index = var_next_index(region, index);
		total += (next_index - index + region->total) % region->total;

		if (total > region->total || next_index == index)
			return -EINVAL; /** Sentinel */

		index = next_index;
	}

	return index;
}

static u32 var_write_header(struct lites_region *region, u32 write, u16 logsize)
{
	u64 tsc;

	lites_get_tsc(tsc);

	lites_pr_dbg("write = %08x, logsize = %08x, tsc = %016llx", write, logsize, tsc);
	lites_assert(write <= region->total);

	if (write + region->header_size > region->total)
		write = 0;

	lites_assert(logsize <= LITES_ACCESS_BYTE_MAX - region->header_size);

	memcpy(&region->rb->log[write], &logsize, sizeof(logsize));
	// jiffies64の値を格納領域サイズ分だけコピーする
	memcpy(&region->rb->log[write + sizeof(logsize)], &tsc, LITES_TIMESTAMP_SIZE);

	if (write + region->header_size == region->total)
		return 0;
	return write + region->header_size; /** Next log position */
}

static u32 var_write_log(struct lites_region *region, char *buf, u32 write, u16 logsize)
{
	lites_pr_dbg("write = %08x, logsize = %08x", write, logsize);
	lites_assert(write <= region->total);

	if (write >= region->total)
		write = 0;

	if (write + logsize >= region->total) {
		u32 latter_logsize = logsize - (region->total - write);
		memcpy(&region->rb->log[write], &buf[0], region->total - write);
		memcpy(&region->rb->log[0], &buf[region->total - write], latter_logsize);
		return latter_logsize; /** Next header position */
	} else {
		memcpy(&region->rb->log[write], buf, logsize);
		if (write + logsize + region->header_size > region->total)
			return 0;
		return write + logsize; /** Next header position */
	}
}

static int var_read_header(struct lites_region *region, char *buf, u32 read)
{
	lites_pr_dbg("read = %08x", read);
	lites_assert(read <= region->total);

	if (read >= region->total)
		read = 0;

	memcpy(buf, &region->rb->log[read], region->header_size);
	return 0;
}

static int var_read_log(struct lites_region *region, char *buf, u32 read, u16 logsize)
{
	u32 latter_logsize = 0;

	lites_pr_dbg("read = %08x, logsize = %08x", read, logsize);
	lites_assert(read <= region->total);

	if (read >= region->total)
		read = 0;

	if (read + logsize >= region->total)
		latter_logsize = read + logsize - region->total;

	memcpy(&buf[0], &region->rb->log[read], logsize - latter_logsize);
	memcpy(&buf[logsize - latter_logsize], &region->rb->log[0], latter_logsize);

	return 0;
}

struct lites_operations lites_var_lop = {
	.max_log_size           = var_max_log_size,
	.log_size               = var_log_size,
	.writable_size          = var_writable_size,
	.calc_index             = var_calc_index,
	.calc_index_with_header = var_calc_index_with_header,
	.next_index             = var_next_index,
	.write_header           = var_write_header,
	.write_log              = var_write_log,
	.read_header            = var_read_header,
	.read_log               = var_read_log,
	.update_index           = var_update_index,
};
