/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_PARSE_H
#define __LITES_PARSE_H

#define is_hex(c)       (strlen(&c) >= 2 && (c) == '0' && (&(c))[1] == 'x')
#define is_comma(c)     ((c) == ',')
#define is_delimiter(c) ((c) == ',' || (c) == '\0')

int parse_addr(char **arg, u64 *addr);
int parse_size(char **arg, u32 *size);
int parse_level(char **arg, u32 *level);

#endif /* __LITES_PARSE_H */
