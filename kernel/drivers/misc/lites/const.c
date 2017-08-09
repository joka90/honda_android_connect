/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
#include "region.h"
#include "tsc.h"

static u32 const_max_log_size(struct lites_region *region)
{
	return region->rb->access_byte - region->header_size;
}

static u32 const_log_size(struct lites_region *region, u32 index)
{
	lites_assert(index <= region->total);

	if (index >= region->total)
		index = 0;

	return *(u16 *)(&region->rb->log[index]);
}

static u32 const_writable_size(struct lites_region *region)
{
	u32 read, write;

	read = region->rb->read;
	write = region->rb->write;

	if (read == write && !(region->rb->flags & LITES_FLAG_READ))
		return region->total; /** There is no data */

	return (read - write + region->total) % region->total;
}

static u32 const_next_index(struct lites_region *region, u32 index)
{
	u32 next;

	lites_assert(index <= region->total);

	if (index >= region->total)
		index = 0;

	next = index + region->rb->access_byte;
	if (next >= region->total)
		next = 0;

	return next;
}

static s64 const_calc_index(struct lites_region *region, u32 index, u32 offset)
{
	s64 __offset = (s64)offset;

	lites_assert(index <= region->total);

	if (index >= region->total)
		index = 0;

	if (offset >= region->total)
		return -EINVAL;

	while (__offset > 0) {
		__offset -= const_log_size(region, index);
		index  = const_next_index(region, index);
		if (index == region->rb->write)
			return -EINVAL;
	}

	return index;
}

static s64 const_calc_index_with_header(struct lites_region *region, u32 index, u32 offset)
{
	s64 __offset = (s64)offset;

	lites_assert(index <= region->total);

	if (index >= region->total)
		index = 0;

	if (offset >= region->total)
		return -EINVAL;

	while (__offset > 0) {
		__offset -= const_log_size(region, index) + region->header_size;
		index  = const_next_index(region, index);
		if (index == region->rb->write)
			return -EINVAL;
	}

	return index;
}

static s64 const_update_index(struct lites_region *region, u32 start, u32 index, u16 size, u32 flags)
{
	lites_assert(start <= region->total && index <= region->total);

	if (!flags)
		return index;

	if (index == region->total)
		index = 0;

	if ((index - start + region->total) % region->total < size)
		index = (index + region->rb->access_byte) % region->total;

	return index;
}

static u32 const_write_header(struct lites_region *region, u32 write, u16 logsize)
{
	u64 tsc;

	lites_get_tsc(tsc);

	lites_assert(write + region->header_size <= region->total);

	memcpy(&region->rb->log[write], &logsize, sizeof(logsize));
	// jiffies64の値を格納領域サイズ分だけコピーする
	memcpy(&region->rb->log[write + sizeof(logsize)], &tsc, LITES_TIMESTAMP_SIZE);
	/** Return current log position (or current extra data position) */
	return write + region->header_size;
}

static u32 const_write_log(struct lites_region *region, char *buffer, u32 write, u16 logsize)
{
	lites_pr_dbg("write = %08x, logsize = %08x", write, logsize);
	lites_assert(write + logsize <= region->total);

	memcpy(&region->rb->log[write], buffer, logsize);

	/** Return next header position */
	return (((write / region->rb->access_byte) + 1) * region->rb->access_byte) % region->total;
}

static int const_read_header(struct lites_region *region, char *buffer, u32 read)
{
	lites_assert(read + region->header_size <= region->total);

	memcpy(buffer, &region->rb->log[read], region->header_size);
	return 0;
}

static int const_read_log(struct lites_region *region, char *buffer, u32 read, u16 logsize)
{
	lites_pr_dbg("read = %08x, logsize = %08x", read, logsize);
	lites_assert(read + logsize <= region->total);

	memcpy(buffer, &region->rb->log[read], logsize);
	return 0;
}

struct lites_operations lites_const_lop = {
	.max_log_size           = const_max_log_size,
	.log_size               = const_log_size,
	.writable_size          = const_writable_size,
	.next_index             = const_next_index,
	.calc_index             = const_calc_index,
	.calc_index_with_header = const_calc_index_with_header,
	.update_index           = const_update_index,
	.write_header           = const_write_header,
	.write_log              = const_write_log,
	.read_header            = const_read_header,
	.read_log               = const_read_log,
};
