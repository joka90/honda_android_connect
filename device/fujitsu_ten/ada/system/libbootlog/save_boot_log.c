/*
 * Copyright(C) 2013 FUJITSU TEN LIMITED
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <libbootlog/save_boot_log.h>
/* FUJITSU TEN: 2013-08-28 : BootloaderLog add start */
#include <android/log.h>
/* FUJITSU TEN: 2013-08-28 : BootloaderLog add end */

/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
#define LOG_TAG "libbootlog"
/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
#define LOG_MAXSIZE (2*1024*1024)
#define LOG_SYSTEMDATA_SIZE_POS (LOG_MAXSIZE-4)
#define LOG_SYSTEMDATA_CHECK_POS (LOG_MAXSIZE-8)
#define LOG_SYSTEMDATA_WRITELOGSIZE_POS (LOG_MAXSIZE-12)
#define LOG_SYSTEMDATA_SIZE (12)
#define LOG_READCHECK_CODE (0x66666667)

int save_boot_log(const char *filepath)
{
	FILE	*log_fp;
	FILE	*save_fp;
	unsigned char	buf[256];
	unsigned int	totalsize;
	unsigned int	logsize;
	unsigned int	writesize;
	unsigned int	startpoint;
	char	sizebuf[4];
	char	cmd[256];

	log_fp = fopen("/dev/block/platform/sdhci-tegra.3/by-name/LLG", "r+b");
	if(log_fp == NULL) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Log DeviceFile fopen() Error!\n");
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		return GETBOOTLOG_RTN_FAIL;
	}

	save_fp = fopen(filepath, "wb");
	if(save_fp == NULL) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "filepath=%s fopen() Error!\n", filepath);
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		fclose(log_fp);
		return GETBOOTLOG_RTN_FAIL;
	}

	//Get ReadCheckCode
	fseek(log_fp, LOG_SYSTEMDATA_CHECK_POS, SEEK_SET);
	if(fread(sizebuf, sizeof(sizebuf), 1, log_fp) < 1) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Read CheckCode Get Error!\n");
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		fclose(log_fp);
		fclose(save_fp);
		return GETBOOTLOG_RTN_FAIL;
	}
	if (*(unsigned int*)&sizebuf != LOG_READCHECK_CODE) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "CheckCode don't much.\n");
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		fclose(log_fp);
		fclose(save_fp);
		return GETBOOTLOG_RTN_FAIL;
	}

	//Get Total Log Size
	fseek(log_fp, LOG_SYSTEMDATA_SIZE_POS, SEEK_SET);
	if(fread(sizebuf, sizeof(sizebuf), 1, log_fp) < 1) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "TotalLogsize Get Error!\n");
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		fclose(log_fp);
		fclose(save_fp);
		return GETBOOTLOG_RTN_FAIL;
	}
	totalsize = *(unsigned int*)&sizebuf;
	/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "TotalLogsize:%d\n", totalsize);
	/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */

	//Get log write size	
	fseek(log_fp, LOG_SYSTEMDATA_WRITELOGSIZE_POS, SEEK_SET);
	if(fread(sizebuf, sizeof(sizebuf), 1, log_fp) < 1) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "WriteLogSize Get Error!\n");
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		fclose(log_fp);
		fclose(save_fp);
		return GETBOOTLOG_RTN_FAIL;
	}
	logsize = *(unsigned int*)&sizebuf;
	/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "WriteLogSize:%d\n", logsize);
	/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */

	//LogStartPoint
	startpoint = totalsize - logsize;
	fseek(log_fp, startpoint, SEEK_SET);

	while(1) {
		if(logsize >= 256) {
			writesize = 256;
			logsize -= 256;
		} else {
			writesize = logsize;
			logsize = 0;
		}
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog del start */
		//printf("writesize:%d, logsize:%d\n", writesize, logsize);
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog del end */

		if(fread(buf, writesize, 1, log_fp) < 1) {
			/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "BootloaderLog Read Error!\n");
			/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
			fclose(log_fp);
			fclose(save_fp);
			return GETBOOTLOG_RTN_FAIL;
		}

		if (fwrite(buf, writesize, 1, save_fp) < 1) {
			/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "File Write Error!\n");
			/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
			fclose(log_fp);
			fclose(save_fp);
			return GETBOOTLOG_RTN_FAIL;
		}

		if(logsize == 0) {
			break;
		}
	};

	//clear writelogsize
	fseek(log_fp, LOG_SYSTEMDATA_WRITELOGSIZE_POS, SEEK_SET);
	*(unsigned int*)&sizebuf = 0;
	if(fwrite(sizebuf, sizeof(sizebuf), 1, log_fp) < 1) {
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod start */
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Writelogsize clear Error!\n");
		/* FUJITSU TEN: 2013-08-28 : BootloaderLog mod end */
		fclose(log_fp);
		fclose(save_fp);
		return GETBOOTLOG_RTN_FAIL;
	}

	fclose(log_fp);
	fclose(save_fp);

	return GETBOOTLOG_RTN_SUCCESS;
}

