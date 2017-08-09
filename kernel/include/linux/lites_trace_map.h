/**
 * @file lites_trace_map.h
 *
 * @brief Definition of lites memory map
 *
 * Copyright (c) 2012-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef LITES_TRACE_MAP_H_
#define LITES_TRACE_MAP_H_

/** FUJITSU TEN:2012-11-07 14A-DA add start*/
#include <ada/memorymap.h>
#define	FTENBB_RAS_REGION_ADDR			MEMMAP_DIAGLOG_ADDR			/* default */
/** FUJITSU TEN:2012-11-07 14A-DA add end */
/** FUJITSU TEN:2013-11-20 14A-DA add start*/
#define	FTENBB_RAS_REGION_SIZE			MEMMAP_DIAGLOG_SIZE			/* default */
/** FUJITSU TEN:2013-11-20 14A-DA add end */

#ifdef CONFIG_MACH_FTENBB
#include <linux/ftenbb_config.h>
#endif	/* CONFIG_MACH_FTENBB */


#ifndef FTENBB_RAS_REGION_ADDR
	#error "RAS region address not specified."
#endif


#define	LITES_REGION_ADDR(offset)		(FTENBB_RAS_REGION_ADDR + offset)

/*======================================================================*/
/*							"Region Definition"							*/
/*======================================================================*/

/*	Region of management information	*/
#define	LITES_MGMT_REGION_ADDR				LITES_REGION_ADDR(0x00000000)
/** FUJITSU TEN:2012-11-07 ADA LOG start */
// #define	LITES_MGMT_REGION_SIZE				0x400		/* Size of management information	*/
#define	LITES_MGMT_REGION_SIZE				0x1000		/* Size of management information	*/
/** FUJITSU TEN:2012-11-07 ADA LOG end */

/*	Region of ubinux kernel panic information	*/
/** FUJITSU TEN:2012-11-07 ADA LOG start */
// #define	LITES_UKPI_REGION_ADDR				LITES_REGION_ADDR(0x00000400)
#define	LITES_UKPI_REGION_ADDR				LITES_REGION_ADDR(0x000001000)
/** FUJITSU TEN:2012-11-07 ADA LOG end */
#define	LITES_UKPI_REGION_SIZE				0x4000		/* Size of ubinux kernel panic information	*/

/*	Region of printk	*/
/** FUJITSU TEN:2012-11-07 ADA LOG start */
// #define	LITES_PRINTK_REGION_ADDR			LITES_REGION_ADDR(0x00004400)
#define	LITES_PRINTK_REGION_ADDR			LITES_REGION_ADDR(0x00005000)
/** FUJITSU TEN:2012-11-07 ADA LOG end */
#define	LITES_PRINTK_REGION_SIZE			0x10000		/* Region size of printk			*/
#define	LITES_PRINTK_REGION_LEVEL			-1	/* filtering invalid	*/
#define	LITES_PRINTK_REGION_WRAP			0	/* overwrite			*/
#define	LITES_PRINTK_REGION_ACCESS_BYTE		-1	/* variable log			*/

/*	Region of syslog	*/
/** FUJITSU TEN:2012-11-07 ADA LOG start */
// #define	LITES_SYSLOG_REGION_ADDR			LITES_REGION_ADDR(0x00014400)
#define	LITES_SYSLOG_REGION_ADDR			LITES_REGION_ADDR(0x00015000)
/** FUJITSU TEN:2012-11-07 ADA LOG end */
#define	LITES_SYSLOG_REGION_SIZE			0x2000		/* Region size of syslog			*/
#define	LITES_SYSLOG_REGION_LEVEL			-1	/* filtering invalid	*/
#define	LITES_SYSLOG_REGION_WRAP			0	/* overwrite			*/
#define	LITES_SYSLOG_REGION_ACCESS_BYTE		-1	/* variable log			*/




/*======================================================================*/
/*				"Table Definition of trace group"						*/
/*======================================================================*/

/**
 * @struct lites_group_def
 * @brief Structure of group definition
 */
struct lites_group_def {
	u32		group_id;			/**< group_id */
	u32		region_num;			/**< number of regions */
	u64		region_address;		/**< first region address */
	u32		region_size;		/**< region size */
	u32		region_level;		/**< region level */
	u32		region_wrap;		/**< region wrap */
	u32		region_access_byte;	/**< region access_byte */
};

/**
 * @brief Table Definition of trace group
 *
 *	Contents of lites_trace_group_info structure.
 *	Index of the array becomes group_id.
 */

/*      { group	region	region							region		region	region	region		}	*/
/*      { id,	num,	address							size		level	wrap	access_byte	}	*/
#if 0 /** FUJITSU TEN: 2012.10.31 remove start */
#define LITES_TRACE_GROUP_LIST	{ 																												\
		{ 0,	0,		LITES_REGION_ADDR(0x00000000),	0x00000,	0,		0,		0			},	/* [0] no use index 0					*/	\
		{ 1,	3,		LITES_REGION_ADDR(0x00016400),	0x02000,	-1,		1,		-1			},	/* [1] Power supply						*/	\
		{ 2,	3,		LITES_REGION_ADDR(0x0001c400),	0x10000,	-1,		1,		-1			},	/* [2] Mode management/vehicle signal	*/	\
		{ 3,	3,		LITES_REGION_ADDR(0x0004c400),	0x30000,	-1,		1,		-1			},	/* [3] AVC-LAN communication			*/	\
		{ 4,	3,		LITES_REGION_ADDR(0x000dc400),	0x04000,	-1,		1,		-1			},	/* [4] Key								*/	\
		{ 5,	3,		LITES_REGION_ADDR(0x000e8400),	0x02000,	-1,		1,		-1			},	/* [5] Error							*/	\
		{ 6,	3,		LITES_REGION_ADDR(0x000ee400),	0x20000,	-1,		1,		-1			},	/* [6] CAN								*/	\
		{ 7,	3,		LITES_REGION_ADDR(0x0014e400),	0x02000,	-1,		1,		-1			},	/* [7] Car location						*/	\
		{ 8,	3,		LITES_REGION_ADDR(0x00154400),	0x40000,	-1,		1,		-1			},	/* [8] Maker peculiarity				*/	\
	}
#endif /** FUJITSU TEN: 2012.10.31 remove end */
/** FUJITSU TEN: 2012-12-04 ADA LOG add start */
/** FUJITSU TEN: 2013-02-04 ADA LOG add start */
#if 0
#define LITES_TRACE_GROUP_LIST	{												\
	{REGION_NOT_USE            ,	0,	LITES_REGION_ADDR(0x00000000),	0x00000,	0,	0,	0	},/* [ 0] no use index 0		*/	\
	{REGION_PRINTK             ,	3,	LITES_REGION_ADDR(0x00017000),	0x10000,	-1,	1,	-1	},/* [ 1] printk			*/	\
	{REGION_ADA_PRINTK         ,	3,	LITES_REGION_ADDR(0x00047000),	0x10000,	-1,	1,	-1	},/* [ 2] kernel(Automotive)	*/	\
	{REGION_SYSLOG             ,	3,	LITES_REGION_ADDR(0x00077000),	0x02000,	-1,	1,	-1	},/* [ 3] syslog			*/	\
	{REGION_EVENT_LOG          ,	3,	LITES_REGION_ADDR(0x0007D000),	0x22000,	-1,	1,	-1	},/* [ 4] Event LOG			*/	\
	{REGION_TRACE_BATTELY      ,	3,	LITES_REGION_ADDR(0x000E3000),	0x02000,	-1,	1,	-1	},/* [ 5] Power supply			*/	\
	{REGION_TRACE_MODE_SIG     ,	3,	LITES_REGION_ADDR(0x000E9000),	0x10000,	-1,	1,	-1	},/* [ 6] Mode management/vehicle signal	*/	\
	{REGION_TRACE_AVC_LAN      ,	3,	LITES_REGION_ADDR(0x00119000),	0x30000,	-1,	1,	-1	},/* [ 7] AVC-LAN communication		*/	\
	{REGION_TRACE_KEY          ,	3,	LITES_REGION_ADDR(0x001A9000),	0x04000,	-1,	1,	-1	},/* [ 8] Key				*/	\
	{REGION_TRACE_ERROR        ,	3,	LITES_REGION_ADDR(0x001B5000),	0x02000,	-1,	1,	-1	},/* [ 9] Error			*/	\
	{REGION_TRACE_HARD_ERROR   ,	3,	LITES_REGION_ADDR(0x001BB000),	0x02000,	-1,	1,	-1	},/* [10] Hard Error			*/	\
	{REGION_TRACE_CAN          ,	3,	LITES_REGION_ADDR(0x001C1000),	0x20000,	-1,	1,	-1	},/* [11] CAN				*/	\
	{REGION_TRACE_MAKER        ,	3,	LITES_REGION_ADDR(0x00221000),	0x40000,	-1,	1,	-1	},/* [12] Maker peculiarity		*/	\
	{REGION_TRACE_DEVICE       ,	3,	LITES_REGION_ADDR(0x002E1000),	0x20000,	-1,	1,	-1	},/* [13] Device State		*/	\
	{REGION_TRACE_OPE_HIS      ,	3,	LITES_REGION_ADDR(0x00341000),	0x10000,	-1,	1,	-1	},/* [14] Operation History		*/	\
	{REGION_TRACE_OS_UPDATE    ,	3,	LITES_REGION_ADDR(0x00371000),	0x10000,	-1,	1,	-1	},/* [15] OS Ver.UP History		*/	\
	{REGION_TRACE_APL_INSTALL  ,	3,	LITES_REGION_ADDR(0x003A1000),	0x02000,	-1,	1,	-1	},/* [16] App Install History	*/	\
	{REGION_ANDROID_ADA_MAIN   ,	3,	LITES_REGION_ADDR(0x003A7000),	0x40000,	-1,	1,	-1	},/* [17] main(Automotive)		*/	\
	{REGION_ANDROID_MAIN       ,	3,	LITES_REGION_ADDR(0x00467000),	0x40000,	-1,	1,	-1	},/* [18] Android			*/	\
	{REGION_ANDROID_SYSTEM     ,	3,	LITES_REGION_ADDR(0x00527000),	0x20000,	-1,	1,	-1	},/* [19] system			*/	\
	{REGION_ANDROID_EVENT      ,	3,	LITES_REGION_ADDR(0x00587000),	0x10000,	-1,	1,	-1	},/* [20] event			*/	\
	{REGION_ANDROID_RADIO      ,	3,	LITES_REGION_ADDR(0x005B7000),	0x10000,	-1,	1,	-1	},/* [21] radio			*/	\
	{REGION_PROC_SELF_NET_DEV  ,	3,	LITES_REGION_ADDR(0x005E7000),	0x10000,	-1,	1,	-1	},/* [22] /proc/self/net/dev	*/	\
	{REGION_PROC_SLABINFO      ,	3,	LITES_REGION_ADDR(0x00617000),	0x10000,	-1,	1,	-1	},/* [23] /proc/slabinfo		*/	\
	{REGION_PROC_VMSTAT        ,	3,	LITES_REGION_ADDR(0x00647000),	0x10000,	-1,	1,	-1	},/* [24] /proc/vmstat		*/	\
	{REGION_PROC_INTERRUPTS    ,	3,	LITES_REGION_ADDR(0x00677000),	0x10000,	-1,	1,	-1	},/* [25] /proc/interrupts		*/	\
}
#endif
/** FUJITSU TEN: 2013-02-04 ADA LOG add end */

/** FUJITSU TEN: 2013-12-06 set print log level add begin */
/** defconfigファイルに定義がある場合はdefconfigファイルの定義を設定 */
/** 定義が内場合はフィルタなし(-1)を設定 */
#ifndef CONFIG_LITES_OUTLOG_LEVEL
#define CONFIG_LITES_OUTLOG_LEVEL -1
#endif

/** 出力レベルを設定 */
#define LITES_PRINT_LOG_LEVEL	CONFIG_LITES_OUTLOG_LEVEL
/** FUJITSU TEN: 2013-12-06 set print log level add end   */

/** FUJITSU TEN: 2013-02-04 ADA LOG add start */
/** FUJITSU TEN: 2013-09-27 ADA LOG mod start */
/** FUJITSU TEN: 2013-10-29 ADA LOG mod start */
#define LITES_TRACE_GROUP_LIST {		\
	{REGION_NOT_USE             , 0 ,LITES_REGION_ADDR(0x00000000),0x00000 , 0 , 0 , 0 },/* [ 0] no use index 0         */	\
	{REGION_PRINTK              , 3 ,LITES_REGION_ADDR(0x00017000),0x10000 ,-1 , 1 ,-1 },/* [ 1] printk                 */	\
	{REGION_ADA_PRINTK          , 3 ,LITES_REGION_ADDR(0x00047000),0x10000 ,-1 , 1 ,-1 },/* [ 2] kernel(Automotive)	    */	\
	{REGION_SYSLOG              , 3 ,LITES_REGION_ADDR(0x00077000),0x02000 ,-1 , 1 ,-1 },/* [ 3] syslog                 */	\
	{REGION_EVENT_LOG           , 3 ,LITES_REGION_ADDR(0x0007D000),0x22000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [ 4] Event LOG              */	\
	{REGION_TRACE_BATTELY       , 3 ,LITES_REGION_ADDR(0x000E3000),0x02000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [ 5] Power supply           */	\
	{REGION_TRACE_MODE_SIG      , 3 ,LITES_REGION_ADDR(0x000E9000),0x10000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [ 6] vehicle signal         */	\
	{REGION_TRACE_AVC_LAN       , 3 ,LITES_REGION_ADDR(0x00119000),0x30000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [ 7] AVC-LAN communication  */	\
	{REGION_TRACE_KEY           , 3 ,LITES_REGION_ADDR(0x001A9000),0x04000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [ 8] Key                    */	\
	{REGION_TRACE_ERROR         , 3 ,LITES_REGION_ADDR(0x001B5000),0x02000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [ 9] Soft Error             */	\
	{REGION_TRACE_DIAG_ERROR    , 3 ,LITES_REGION_ADDR(0x001BB000),0x02000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [10] Soft Error(diag)       */	\
	{REGION_TRACE_HARD_ERROR    , 3 ,LITES_REGION_ADDR(0x001C1000),0x02000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [11] Hard Error             */	\
	{REGION_TRACE_DIAG_HARD_ERROR, 3 ,LITES_REGION_ADDR(0x001C7000),0x02000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [12] Hard Error(diag)      */	\
	{REGION_TRACE_APL_INSTALL   , 3 ,LITES_REGION_ADDR(0x001CD000),0x10000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [13] App Install History    */	\
	{REGION_TRACE_OS_UPDATE     , 3 ,LITES_REGION_ADDR(0x001FD000),0x10000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [14] OS Ver.UP History      */	\
	{REGION_TRACE_DRIVER        , 3 ,LITES_REGION_ADDR(0x0022D000),0x10000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [15] Drivers Trace          */	\
	{REGION_TRACE_MAKER         , 3 ,LITES_REGION_ADDR(0x0025D000),0x40000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [16] Maker peculiarity      */	\
	{REGION_TRACE_WHITELIST     , 3 ,LITES_REGION_ADDR(0x0031D000),0x02000 ,LITES_PRINT_LOG_LEVEL , 1 ,-1 },/* [17] Whitelist              */	\
	{REGION_ANDROID_ADA_MAIN    , 3 ,LITES_REGION_ADDR(0x00323000),0x40000 ,-1 , 1 ,-1 },/* [18] main(Automotive)       */	\
	{REGION_ANDROID_MAIN        , 3 ,LITES_REGION_ADDR(0x003E3000),0x80000 ,-1 , 1 ,-1 },/* [19] Android main           */	\
	{REGION_ANDROID_SYSTEM      , 3 ,LITES_REGION_ADDR(0x00563000),0x40000 ,-1 , 1 ,-1 },/* [20] Android system         */	\
	{REGION_ANDROID_EVENT       , 3 ,LITES_REGION_ADDR(0x00623000),0x10000 ,-1 , 1 ,-1 },/* [21] Android event          */	\
	{REGION_ANDROID_RADIO       , 3 ,LITES_REGION_ADDR(0x00653000),0x10000 ,-1 , 1 ,-1 },/* [22] Android radio          */	\
	{REGION_PROC_SELF_NET_DEV   , 3 ,LITES_REGION_ADDR(0x00683000),0x10000 ,-1 , 1 ,-1 },/* [23] /proc/self/net/dev     */	\
	{REGION_PROC_SLABINFO       , 3 ,LITES_REGION_ADDR(0x006B3000),0x10000 ,-1 , 1 ,-1 },/* [24] /proc/slabinfo         */	\
	{REGION_PROC_VMSTAT         , 3 ,LITES_REGION_ADDR(0x006E3000),0x10000 ,-1 , 1 ,-1 },/* [25] /proc/vmstat           */	\
	{REGION_PROC_INTERRUPTS     , 3 ,LITES_REGION_ADDR(0x00713000),0x10000 ,-1 , 1 ,-1 },/* [26] /proc/interrupts       */	\
	{REGION_PROC_DISK           , 3 ,LITES_REGION_ADDR(0x00743000),0x10000 ,-1 , 1 ,-1 },/* [27] emmc info              */	\
}
/** FUJITSU TEN: 2013-10-29 ADA LOG mod end */
/** FUJITSU TEN: 2013-09-27 ADA LOG mod end */
/** FUJITSU TEN: 2013-02-04 ADA LOG add end */
/** FUJITSU TEN: 2012-12-04 ADA LOG add end */

#endif  /** LITES_TRACE_MAP_H_ */
