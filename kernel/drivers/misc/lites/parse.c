/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include "region.h"
#include "parse.h"

/**
 * @brief アドレス解析用関数
 * 　文字列をアドレスとして解析する。アドレスは0xが付いていた場合に16進数として
 * 解釈され、0xが付いていない場合に10進数として解釈される。解析が完了した場合、
 * 文字列のポインタを解析で使用した分だけインクリメントする。
 *
 * @param[in/out] arg  解析対象の文字列
 * @param[in]     addr アドレス格納用領域
 *
 * @retval 0      解析が成功した
 * @retval 負の値 解析が失敗した
 */
int parse_addr(char **arg, u64 *addr)
{
	u64 value;
	char *startp, *endp;

	if (arg == NULL || *arg == NULL || addr == NULL)
		return -1;

	startp = *arg;
	if (!is_hex(*startp))
		return -2;

	value = simple_strtoull(startp, &endp, 16);
	if (startp == endp || !is_delimiter(*endp))
		return -3;

	*addr = value;
	*arg = is_comma(*endp) ? endp + 1 : endp;

	return 0;
}

/**
 * @brief サイズ解析用関数
 * 　文字列をサイズとして解析する。文字列は10進数表記されているものとする。末尾
 * にkやmが付いている場合は1024倍あるいは1024*1024倍される。
 *
 * @param[in/out] arg  解析対象の文字列
 * @param[in]     size サイズ格納用領域
 *
 * @retval 0      解析が成功した
 * @retval 負の値 解析が失敗した
 */
int parse_size(char **arg, u32 *size)
{
	u32 value;
	char *startp, *endp;

	if (arg == NULL || *arg == NULL || size == NULL)
		return -1;

	startp = *arg;
	if (!is_hex(*startp)) {
		value = simple_strtoll(startp, &endp, 10);
		switch (*endp) {
		case 'm':
		case 'M':
			value *= 1024;	/* 意図的にbreakなし */
		case 'k':
		case 'K':
			value *= 1024;
			endp++;
			break;
		}
	} else {
		value = simple_strtoll(startp, &endp, 16);
	}

	if (startp == endp || !is_delimiter(*endp))
		return -2;

	*arg = is_comma(*endp) ? endp + 1 : endp;
	*size = value;

	return 0;
}

/**
 * @brief 登録レベル解析用関数
 * 　文字列を登録レベルとして解析する。文字列は正負の10進数表記されているものと
 * する。
 *
 * @param[in/out] arg   解析対象の文字列
 * @param[in]     level サイズ格納用領域
 *
 * @retval 0      解析が成功した
 * @retval 負の値 解析が失敗した
 */
int parse_level(char **arg, u32 *level)
{
	u32 value;
	char *startp, *endp;

	if (arg == NULL || *arg == NULL || level == NULL)
		return -1;

	startp = *arg;
	value = simple_strtoll(startp, &endp, 10);
	if (startp == endp || !is_delimiter(*endp) || !check_level_value(value))
		return -2;

	*arg = is_comma(*endp) ? endp + 1 : endp;
	*level = (unsigned int)value;

	return 0;
}
