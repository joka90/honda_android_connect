/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_BUFFER_H
#define __LITES_BUFFER_H

#include <linux/lites_trace.h>
#include "region.h"

/**
 * @struct lites_buffer
 * @brief copy_to_user/copy_from_user用のバッファを定義する構造体
 */
struct lites_buffer {
	char *buffer;
	size_t size;
	struct mutex mutex;
};

/** バッファ用マクロ */
static inline unsigned int __lites_smp_processor_id(void)
{
	unsigned int this_cpu;

	preempt_disable();
	this_cpu = smp_processor_id();
	preempt_enable();

	return this_cpu;
}

#define EXTERN_LITES_BUFFER(name) extern struct lites_buffer *name##_buffer_list[]
#define lites_buffer(name)       name##_buffer_list[__lites_smp_processor_id()]
#define lites_buffer_lock(buf)   do { mutex_lock(&(buf)->mutex); } while (0)
#define lites_buffer_unlock(buf) do { mutex_unlock(&(buf)->mutex); } while (0)

/** ドライバの初期化/終了処理で一度だけ呼ぶAPI */
extern int lites_init_buffer(void);
extern void lites_exit_buffer(void);

#endif /** __LITES_BUFFER_H */
