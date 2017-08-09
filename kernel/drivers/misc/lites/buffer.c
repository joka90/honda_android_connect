/**
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/lites_trace.h>
#include "buffer.h"

#define DEFINE_LITES_BUFFER(name) \
	struct lites_buffer *name##_buffer_list[NR_CPUS]

DEFINE_LITES_BUFFER(lites_read);
DEFINE_LITES_BUFFER(lites_write);
DEFINE_LITES_BUFFER(__lites_write);
DEFINE_LITES_BUFFER(LITES_READ);
DEFINE_LITES_BUFFER(LITES_READ_SQ);
/* FUJITSU-TEN 2013-11-20: LITES AREA DUMP add start */
DEFINE_LITES_BUFFER(lites_dump);
/* FUJITSU-TEN 2013-11-20: LITES AREA DUMP add end   */

void __lites_free_buffer(struct lites_buffer **buffer_list, size_t nmemb)
{
	int i;

	for (i = 0; i < nmemb; i++) {
		vfree(buffer_list[i]->buffer);
		vfree(buffer_list[i]);
	}
}

int __lites_alloc_buffer(struct lites_buffer **buffer_list, size_t size, size_t nmemb)
{
	int i;

	for (i = 0; i < nmemb; i++) {
		buffer_list[i] = vmalloc(sizeof(**buffer_list));
		if (buffer_list[i] == NULL)
			goto err1;
		memset(buffer_list[i], 0, sizeof(*buffer_list));

		buffer_list[i]->buffer = vmalloc(size);
		if (buffer_list[i]->buffer == NULL)
			goto err2;
		memset(buffer_list[i]->buffer, 0, size);
		mutex_init(&buffer_list[i]->mutex);
		buffer_list[i]->size = size;
	}

	return 0;
err2:
	vfree(buffer_list[i]->buffer);
err1:
	__lites_free_buffer(buffer_list, i);
	return -ENOMEM;
}

#define lites_alloc_buffer(name, size) \
	__lites_alloc_buffer(name##_buffer_list, size, ARRAY_SIZE(name##_buffer_list))
#define lites_free_buffer(name) \
	__lites_free_buffer(name##_buffer_list, ARRAY_SIZE(name##_buffer_list))

int lites_init_buffer(void)
{
	int rc;

	rc = lites_alloc_buffer(lites_read, LITES_READ_BUF_SIZE);
	if (rc < 0)
		goto err1;

	rc = lites_alloc_buffer(lites_write, LITES_WRITE_BUF_SIZE);
	if (rc < 0)
		goto err2;

	rc = lites_alloc_buffer(__lites_write, LITES_TRACE_WRITE_BUF_SIZE);
	if (rc < 0)
		goto err3;

	rc = lites_alloc_buffer(LITES_READ, LITES_READ_BUF_SIZE);
	if (rc < 0)
		goto err4;

	rc = lites_alloc_buffer(LITES_READ_SQ, LITES_READ_BUF_SIZE);
	if (rc < 0)
		goto err5;

	/* FUJITSU-TEN 2013-11-20:LITES DUMP AREA add start */
	rc = lites_alloc_buffer(lites_dump, PAGE_SIZE);
	if (rc < 0)
		goto err6;
	/* FUJITSU-TEN 2013-11-20:LITES DUMP AREA add end   */

	return 0;

/* FUJITSU-TEN 2013-11-20: LITES AREA DUMP add start */
err6:
	lites_free_buffer(LITES_READ_SQ);
/* FUJITSU-TEN 2013-11-20: LITES AREA DUMP add end   */
err5:
	lites_free_buffer(LITES_READ);
err4:
	lites_free_buffer(__lites_write);
err3:
	lites_free_buffer(lites_write);
err2:
	lites_free_buffer(lites_read);
err1:
	return rc;
}

void lites_exit_buffer(void)
{
/* FUJITSU-TEN 2013-11-20: LITES AREA DUMP add start */
	lites_free_buffer(lites_dump);
/* FUJITSU-TEN 2013-11-20: LITES AREA DUMP add end   */
	lites_free_buffer(LITES_READ_SQ);
	lites_free_buffer(LITES_READ);
	lites_free_buffer(__lites_write);
	lites_free_buffer(lites_write);
	lites_free_buffer(lites_read);
}
