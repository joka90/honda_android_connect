/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_TSC_H
#define __LITES_TSC_H

#ifdef CONFIG_PPC
# define __lites_mftbl(var)					\
	do {							\
		__asm__ __volatile__("mftb %0\n\t"		\
				     "sync\n\t":		\
				     "=r"(var)::"memory");	\
	} while (0)
# define __lites_mftbu(var)					\
	do {							\
		__asm__ __volatile__("mftbu %0\n\t"		\
				     "sync\n\t" :		\
				     "=r"(var)::"memory");	\
	} while (0)
# define lites_get_tsc(tsc)					\
	do {							\
		u32 check;					\
		struct { u32 high, low; } *p = (void *)&(tsc);	\
		do {						\
			__lites_mftbu(p->high);			\
			__lites_mftbl(p->low);			\
			__lites_mftbu(check);			\
		} while (p->high != check);			\
	} while (0)
#elif defined CONFIG_X86
#ifdef LITES_FTEN
# include <linux/jiffies.h>
# define lites_get_tsc(tsc) do { (tsc) = get_jiffies_64(); } while (0)
#else /* LITES_FTEN */
# include <asm/msr.h>
# define lites_get_tsc(tsc) do { rdtscll(tsc); } while (0)
#endif /* LITES_FTEN */
#elif defined CONFIG_ARM
# include <linux/jiffies.h>
# define lites_get_tsc(tsc) do { (tsc) = get_jiffies_64(); } while (0)
#else
# define lites_get_tsc(tsc) (0xdeadbeaf)
#endif

#endif /* __LITES_TSC_H */
