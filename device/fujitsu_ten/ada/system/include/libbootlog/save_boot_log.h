/*
 * Copyright(C) 2013 FUJITSU TEN LIMITED
 */

/*
 *	GetBootLog APIdefine
 */

#ifdef __cplusplus
extern "C" {
#endif

enum GETBOOTLOG_RTN {
	GETBOOTLOG_RTN_SUCCESS,
	GETBOOTLOG_RTN_FAIL
};

extern int save_boot_log(const char *filepath);

#ifdef __cplusplus
}
#endif
