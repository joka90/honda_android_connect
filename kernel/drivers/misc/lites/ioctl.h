/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_IOCTL_H
#define __LITES_IOCTL_H

/**
 * @struct lites_ioctl_t
 * @brief 各ioctlコマンドを呼び出すための構造体
 */
struct lites_ioctl {
	unsigned int cmd; /**< コマンド番号 */
	char *name;       /**< コマンド名 */
	int (*ioctl)(struct file *filep, unsigned long user_addr);
	/**< コマンドに該当する関数 */
};

#define ENTRY_LITES_IOCTL(name) { name, #name, &ioctl_##name }
#define DEFINE_LITES_IOCTL(name) \
	static int ioctl_##name(struct file *file, unsigned long user_addr)
#define ENTRY_LITES_COMPAT_IOCTL(name) { name, #name, &compat_ioctl_##name }
#define DEFINE_LITES_COMPAT_IOCTL(name) \
	static int compat_ioctl_##name(struct file *file, unsigned long user_addr)
#define LITES_LITES_COMPAT_IOCTL(name, file, user_addr) compat_ioctl_##name(file, user_addr)

#define get_arg1() (file)
#define get_arg2() (user_addr)
#define copy_from_user_addr(args, size) copy_from_user(((void *)args), (void *)get_arg2(), (size))
#define copy_to_user_addr(args, size)   copy_to_user((void *)get_arg2(), ((void *)args), (size))

/** ioctlコマンドの配列 */
extern struct lites_ioctl lites_ioctls[];
extern size_t lites_ioctls_nr;

#endif /** __LITES_IOCTL_H */
