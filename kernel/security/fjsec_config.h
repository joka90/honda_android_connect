/*
 * Copyright(C) 2012 - 2013 FUJITSU TEN LIMITED
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <linux/ptrace.h>
#include <crypto/sha.h>
#include "fjsec_modinfo.h"
//#include "../../fjfeat/FjFeatMac.h"

//#define DEBUG_COPY_GUARD
//#define DEBUG_ACCESS_CONTROL
//#define DEBUG_KITTING_ACCESS_CONTROL

#define AID_ROOT             0  /* traditional unix root user */
#define AID_SYSTEM        1000  /* system server */
#define AID_MEDIA         1013
#define AID_PREINST_FELC  5003
#ifdef DEBUG_COPY_GUARD
#define AID_AUDIO         1005  /* audio devices */
#define AID_LOG           1007  /* log devices */
#endif
#define PRNAME_NO_CHECK 0
#define UID_NO_CHECK -1

#define JAVA_APP_PROCESS_PATH "/system/bin/app_process"

#define S2B(x) ((loff_t)x * SECTOR_SIZE)
#define ADDR2PFN(x) (x >> PAGE_SHIFT)

#define INIT_PROCESS_NAME "init"
#define INIT_PROCESS_PATH "/init"

#define BOOT_ARGS_MODE_FOTA "fotamode"
#define BOOT_ARGS_MODE_SDDOWNLOADER "sddownloadermode"
#define BOOT_ARGS_MODE_RECOVERY "recovery"
#define BOOT_ARGS_MODE_MAKERCMD "makermode"
/* MOD start FUJITSU TEN :2013-05-10 LSM */
#define BOOT_ARGS_MODE_OSUPDATE "osupdatemode"
//#define BOOT_ARGS_MODE_OSUPDATE "recoverymenu"
/* MOD start FUJITSU TEN :2013-05-10 LSM */
#define BOOT_ARGS_MODE_RECOVERYMENU "recoverymenu"
#define BOOT_ARGS_MODE_KERNEL "kernelmode"

#define OFFSET_BYTE_LIMIT LLONG_MAX

//#if (FJFEAT_PRODUCT == FJFEAT_PRODUCT_BOBSLEIGH || FJFEAT_PRODUCT == FJFEAT_PRODUCT_IRUKAD)
//#define RDEV_SYSTEM MKDEV(259, 27) /* APP */
//#define RDEV_SST MKDEV(259, 12) /* SST */
//
//#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
////#define RDEV_FELICA MKDEV(250, 3) /* Feilca */
//#define RDEV_FELICA MKDEV(245, 3) /* Feilca */
//#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
//
#define FOTA_PTN_PATH "/dev/block/mmcblk0p38"
#define FOTA_MOUNT_POINT "/fota2/"
#define FOTA_DIRS_PATH TEST_MOUNT_POINT "*"
//
//#elif (FJFEAT_PRODUCT == FJFEAT_PRODUCT_NABOOD)
//#define RDEV_SYSTEM MKDEV(259, 30) /* APP */
//#define RDEV_SST MKDEV(259, 15) /* SST */
//
//#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
//#define RDEV_FELICA MKDEV(250, 3) /* Feilca */
//#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
//
//#define FOTA_PTN_PATH "/dev/block/mmcblk0p41"
//#define FOTA_MOUNT_POINT "/fota2/"
//#define FOTA_DIRS_PATH TEST_MOUNT_POINT "*"
//
//#endif

#ifdef DEBUG_ACCESS_CONTROL
#define TEST_PTN_PATH "/dev/block/mmcblk0p31"
#define TEST_MOUNT_POINT "/test/"
#define TEST_DIRS_PATH TEST_MOUNT_POINT "*"
#endif

enum boot_mode_type {
    BOOT_MODE_NONE = 0,
    BOOT_MODE_FOTA = 1,
    BOOT_MODE_SDDOWNLOADER = 2,
    BOOT_MODE_RECOVERY = 3,
    BOOT_MODE_MAKERCMD = 4,
    BOOT_MODE_OSUPDATE = 5
};

enum result_mount_point {
    MATCH_SYSTEM_MOUNT_POINT,
#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
    MATCH_SECURE_STORAGE_MOUNT_POINT,
#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */
#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING
    MATCH_KITTING_MOUNT_POINT,
#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */
    MATCH_MOUNT_POINT,
    MISMATCH_MOUNT_POINT,
    UNRELATED_MOUNT_POINT
};

/**
 * Limitation of device file.
 */
struct fs_path_config {
    mode_t        mode;
    uid_t        uid;
    gid_t        gid;
    dev_t        rdev;
    const char    *prefix;
};

static struct fs_path_config devs[] = {
/* MOD start FUJITSU TEN :2013-05-10 LSM */
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 1), "/dev/block/mmcblk0p1" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 2), "/dev/block/mmcblk0p2" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 3), "/dev/block/mmcblk0p3" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 4), "/dev/block/mmcblk0p4" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 5), "/dev/block/mmcblk0p5" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 6), "/dev/block/mmcblk0p6" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 7), "/dev/block/mmcblk0p7" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 0), "/dev/block/mmcblk0p8" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 1), "/dev/block/mmcblk0p9" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 2), "/dev/block/mmcblk0p10" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 3), "/dev/block/mmcblk0p11" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 4), "/dev/block/mmcblk0p12" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 5), "/dev/block/mmcblk0p13" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 6), "/dev/block/mmcblk0p14" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 7), "/dev/block/mmcblk0p15" }, 
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 8), "/dev/block/mmcblk0p16" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 9), "/dev/block/mmcblk0p17" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 10), "/dev/block/mmcblk0p18" }, 
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 11), "/dev/block/mmcblk0p19" },
    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 12), "/dev/block/mmcblk0p20" },
/* MOD start FUJITSU TEN :2013-08-30 LSM */
/* MOD start FUJITSU TEN :2013-11-25 LSM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 13), "/dev/block/mmcblk0p21" },
/* MOD end FUJITSU TEN :2013-11-25 LSM */
/* MOD end FUJITSU TEN :2013-08-30 LSM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 0), CONFIG_SECURITY_FJSEC_DISK_DEV_PATH },
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, RDEV_SYSTEM, CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH },
//
//#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, RDEV_SST, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DEV_PATH },
//#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */
//
//#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
//    { (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFCHR), AID_ROOT, AID_ROOT, RDEV_FELICA, CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH },
//#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
//
//#if (FJFEAT_PRODUCT == FJFEAT_PRODUCT_BOBSLEIGH || FJFEAT_PRODUCT == FJFEAT_PRODUCT_IRUKAD)
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 1), "/dev/block/mmcblk0p1" }, /* CID */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 2), "/dev/block/mmcblk0p2" }, /* CVE */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 3), "/dev/block/mmcblk0p3" }, /* CGD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 4), "/dev/block/mmcblk0p4" }, /* CL2 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 5), "/dev/block/mmcblk0p5" }, /* CDM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 6), "/dev/block/mmcblk0p6" }, /* CL1 */
//    { (S_IRUSR | S_IWUSR | S_IRGRP | S_IFBLK), AID_ROOT, AID_SYSTEM, MKDEV(179, 7), "/dev/block/mmcblk0p7" }, /* CNV */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 8), "/dev/block/mmcblk0p8" }, /* CD1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 9), "/dev/block/mmcblk0p9" }, /* CD2 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 10), "/dev/block/mmcblk0p10" }, /* CLM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 11), "/dev/block/mmcblk0p11" }, /* CBB */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 12), "/dev/block/mmcblk0p12" }, /* CRF */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 13), "/dev/block/mmcblk0p13" }, /* CRV */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 14), "/dev/block/mmcblk0p14" }, /* CDS */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 15), "/dev/block/mmcblk0p15" }, /* DM1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 0), "/dev/block/mmcblk0p16" }, /* LM4 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 1), "/dev/block/mmcblk0p17" }, /* NV1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 2), "/dev/block/mmcblk0p18" }, /* VER */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 3), "/dev/block/mmcblk0p19" }, /* PID */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 4), "/dev/block/mmcblk0p20" }, /* SIM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 5), "/dev/block/mmcblk0p21" }, /* SC1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 6), "/dev/block/mmcblk0p22" }, /* SC2 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 7), "/dev/block/mmcblk0p23" }, /* SC3 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 8), "/dev/block/mmcblk0p24" }, /* SC4 */
//    { (S_IRUSR | S_IWUSR | S_IRGRP | S_IFBLK), AID_ROOT, AID_SYSTEM, MKDEV(259, 9), "/dev/block/mmcblk0p25" }, /* LLG */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 10), "/dev/block/mmcblk0p26" }, /* BI1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 11), "/dev/block/mmcblk0p27" }, /* BI2 */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 12), "/dev/block/mmcblk0p28" }, /* SST */
//    //{ (S_IRUSR | S_IWUSR | S_IRGRP | S_IFBLK), AID_ROOT, AID_SYSTEM, MKDEV(259, 13), "/dev/block/mmcblk0p29" }, /* KLG */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 14), "/dev/block/mmcblk0p30" }, /* ALG */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 15), "/dev/block/mmcblk0p31" }, /* LOG */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 16), "/dev/block/mmcblk0p32" }, /* SOS */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 17), "/dev/block/mmcblk0p33" }, /* LNX */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 18), "/dev/block/mmcblk0p34" }, /* CAC */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 19), "/dev/block/mmcblk0p35" }, /* MSC */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 20), "/dev/block/mmcblk0p36" }, /* USP */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 21), "/dev/block/mmcblk0p37" }, /* FOT */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 22), "/dev/block/mmcblk0p38" }, /* PRI */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 23), "/dev/block/mmcblk0p39" }, /* NUT */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 24), "/dev/block/mmcblk0p40" }, /* MMD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 25), "/dev/block/mmcblk0p41" }, /* KIT */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 26), "/dev/block/mmcblk0p42" }, /* PIM */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 27), "/dev/block/mmcblk0p43" }, /* APP */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 28), "/dev/block/mmcblk0p44" }, /* JSY */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 29), "/dev/block/mmcblk0p45" }, /* UDA */
//#elif (FJFEAT_PRODUCT == FJFEAT_PRODUCT_NABOOD)
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 1), "/dev/block/mmcblk0p1" }, /* CID */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 2), "/dev/block/mmcblk0p2" }, /* CVE */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 3), "/dev/block/mmcblk0p3" }, /* CMD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 4), "/dev/block/mmcblk0p4" }, /* CCD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 5), "/dev/block/mmcblk0p5" }, /* CVD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 6), "/dev/block/mmcblk0p6" }, /* CPS */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 7), "/dev/block/mmcblk0p7" }, /* CHD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 8), "/dev/block/mmcblk0p8" }, /* CGD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 9), "/dev/block/mmcblk0p9" }, /* CED */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 10), "/dev/block/mmcblk0p10" }, /* CFS */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 11), "/dev/block/mmcblk0p11" }, /* CL2 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 12), "/dev/block/mmcblk0p12" }, /* CDM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 13), "/dev/block/mmcblk0p13" }, /* CL1 */
//    { (S_IRUSR | S_IWUSR | S_IRGRP | S_IFBLK), AID_ROOT, AID_SYSTEM, MKDEV(179, 14), "/dev/block/mmcblk0p14" }, /* CNV */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(179, 15), "/dev/block/mmcblk0p15" }, /* CD1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 0), "/dev/block/mmcblk0p16" }, /* CD2 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 1), "/dev/block/mmcblk0p17" }, /* CLM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 2), "/dev/block/mmcblk0p18" }, /* CBB */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 3), "/dev/block/mmcblk0p19" }, /* LM4 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 4), "/dev/block/mmcblk0p20" }, /* NV1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 5), "/dev/block/mmcblk0p21" }, /* VER */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 6), "/dev/block/mmcblk0p22" }, /* PID */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 7), "/dev/block/mmcblk0p23" }, /* SIM */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 8), "/dev/block/mmcblk0p24" }, /* SC1 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 9), "/dev/block/mmcblk0p25" }, /* SC2 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 10), "/dev/block/mmcblk0p26" }, /* SC3 */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 11), "/dev/block/mmcblk0p27" }, /* SC4 */ 
//    { (S_IRUSR | S_IWUSR | S_IRGRP | S_IFBLK), AID_ROOT, AID_SYSTEM, MKDEV(259, 12), "/dev/block/mmcblk0p28" }, /* LLG */ 
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 13), "/dev/block/mmcblk0p29" }, /* BI1 */ 
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 14), "/dev/block/mmcblk0p30" }, /* BI2 */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 15), "/dev/block/mmcblk0p31" }, /* SST */
//    //{ (S_IRUSR | S_IWUSR | S_IRGRP | S_IFBLK), AID_ROOT, AID_SYSTEM, MKDEV(259, 16), "/dev/block/mmcblk0p32" }, /* KLG */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 17), "/dev/block/mmcblk0p33" }, /* ALG */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 18), "/dev/block/mmcblk0p34" }, /* LOG */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 19), "/dev/block/mmcblk0p35" }, /* SOS */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 20), "/dev/block/mmcblk0p36" }, /* LNX */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 21), "/dev/block/mmcblk0p37" }, /* CAC */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 22), "/dev/block/mmcblk0p38" }, /* MSC */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 23), "/dev/block/mmcblk0p39" }, /* USP */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 24), "/dev/block/mmcblk0p40" }, /* FOT */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 25), "/dev/block/mmcblk0p41" }, /* PRI */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 26), "/dev/block/mmcblk0p42" }, /* NUT */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 27), "/dev/block/mmcblk0p43" }, /* MMD */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 28), "/dev/block/mmcblk0p44" }, /* KIT */
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 29), "/dev/block/mmcblk0p45" }, /* PIM */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 29), "/dev/block/mmcblk0p46" }, /* APP */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 29), "/dev/block/mmcblk0p47" }, /* JSY */
//    //{ (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 29), "/dev/block/mmcblk0p48" }, /* UDA */
//#endif
//
//#ifdef DEBUG_ACCESS_CONTROL
//    { (S_IRUSR | S_IWUSR | S_IFBLK), AID_ROOT, AID_ROOT, MKDEV(259, 7), "/dev/block/mmcblk0p23" }, /* SC3 */
//#endif
/* MOD end FUJITSU TEN :2013-05-10 LSM */
    { 0, 0, 0, 0, 0 },
};

/**
 * add module list.
 */
struct module_list {
    const char *name;
    char checksum[SHA256_DIGEST_SIZE];
};

static struct module_list modlist[] = {
    {"baseband_usb_chr", CHECKSUM_BASEBAND_USB_CHR},
    {"bluetooth", CHECKSUM_BLUETOOTH},
    {"bnep" , CHECKSUM_BNEP},
    {"btwilink", CHECKSUM_BTWILINK},
    {"cdc_ncm", CHECKSUM_CDC_NCM},
    {"cfg80211", CHECKSUM_CFG80211},
    {"compat", CHECKSUM_COMPAT},
    {"gps_drv", CHECKSUM_GPS_DRV},
    {"gspca_main", CHECKSUM_GSPCA_MAIN},
    {"hidp", CHECKSUM_HIDP},
    {"ipw2100", CHECKSUM_IPW2100},
    {"lib80211", CHECKSUM_LIB80211},
    {"lib80211_crypt_ccmp", CHECKSUM_LIB80211_CRYPT_CCMP},
    {"lib80211_crypt_tkip", CHECKSUM_LIB80211_CRYPT_TKIP},
    {"lib80211_crypt_wep", CHECKSUM_LIB80211_CRYPT_WEP},
    {"libipw", CHECKSUM_LIBIPW},
    {"mac80211", CHECKSUM_MAC80211},
    {"max2165", CHECKSUM_MAX2165},
    {"mc44s803", CHECKSUM_MC44S803},
    {"mccildkcdcncm", CHECKSUM_MCCILDKCDCNCM},
    {"michael_mic", CHECKSUM_MICHAEL_MIC},
    {"mt2060", CHECKSUM_MT2060},
    {"mt20xx", CHECKSUM_MT20XX},
    {"mt2131", CHECKSUM_MT2131},
    {"mt2266", CHECKSUM_MT2266},
    {"mxl5005s", CHECKSUM_MXL5005S},
    {"mxl5007t", CHECKSUM_MXL5007T},
    {"qt1010", CHECKSUM_QT1010},
    {"rfcomm", CHECKSUM_RFCOMM},
    {"scsi_wait_scan", CHECKSUM_SCSI_WAIT_SCAN},
    {"tda18212", CHECKSUM_TDA18212},
    {"tda18218", CHECKSUM_TDA18218},
    {"tda18271", CHECKSUM_TDA18271},
    {"tda827x", CHECKSUM_TDA827X},
    {"tda8290", CHECKSUM_TDA8290},
    {"tda9887", CHECKSUM_TDA9887},
    {"tea5761", CHECKSUM_TEA5761},
    {"tea5767", CHECKSUM_TEA5767},
    {"tuner_simple", CHECKSUM_TUNER_SIMPLE},
    {"tuner_types", CHECKSUM_TUNER_TYPES},
    {"tuner_xc2028", CHECKSUM_TUNER_XC2028},
    {"wl12xx", CHECKSUM_WL12XX},
    {"wl12xx_sdio", CHECKSUM_WL12XX_SDIO},
    {"wl12xx_sdio_test", CHECKSUM_WL12XX_SDIO_TEST},
    {"wl12xx_spi", CHECKSUM_WL12XX_SPI},
    {"xc4000", CHECKSUM_XC4000},
    {"xc5000", CHECKSUM_XC5000},
    { 0, { 0 } },
};

struct accessible_area_disk_dev {
    loff_t head;
    loff_t tail;
};

static struct accessible_area_disk_dev accessible_areas_fota[] = {
    { S2B(0x00000000), OFFSET_BYTE_LIMIT },
    { 0,          0 },
};

static struct accessible_area_disk_dev accessible_areas_sddownloader[] = {
    { S2B(0x00000000), OFFSET_BYTE_LIMIT },
    { 0,          0 },
};

static struct accessible_area_disk_dev accessible_areas_osupdate[] = {
    { S2B(0x00000000), OFFSET_BYTE_LIMIT },
    { 0,          0 },
};

static struct accessible_area_disk_dev accessible_areas_recovery[] = {
    { 0,          0 },
};

static struct accessible_area_disk_dev accessible_areas_maker[] = {
    { 0,          0 },
};

struct access_control_mmcdl_device {
    enum boot_mode_type boot_mode;
    char *process_name;
    char *process_path;
};

static struct access_control_mmcdl_device mmcdl_device_list[] = {
    { 0, 0, 0 },
};

static long ptrace_read_request_policy[] = {
    PTRACE_TRACEME,
    PTRACE_PEEKTEXT,
    PTRACE_PEEKDATA,
    PTRACE_PEEKUSR,
    PTRACE_CONT,
    PTRACE_GETREGS,
    PTRACE_GETFPREGS,
    PTRACE_ATTACH,
    PTRACE_DETACH,
    PTRACE_GETWMMXREGS,
    PTRACE_GET_THREAD_AREA,
    PTRACE_GETCRUNCHREGS,
    PTRACE_GETVFPREGS,
    PTRACE_GETHBPREGS,
    PTRACE_GETEVENTMSG,
    PTRACE_GETSIGINFO,
    PTRACE_GETREGSET,
    PTRACE_SEIZE,
    PTRACE_INTERRUPT,
    PTRACE_LISTEN,
};

struct read_write_access_control_process {
    char *process_name;
    char *process_path;
    uid_t uid;
};

#ifdef DEBUG_COPY_GUARD
static struct read_write_access_control_process test_process_list[] = {
//    { "com.fujitsu.FjsecTest", JAVA_APP_PROCESS_PATH, AID_LOG },
//    { "ok_native", "/data/lsm-test/tmp/regza1", AID_AUDIO },
//    { PRNAME_NO_CHECK, "/data/lsm-test/tmp/regza2", AID_AUDIO },
//    { "ok_native", "/data/lsm-test/tmp/regza3", UID_NO_CHECK },
    { 0, 0, 0 },
};
#endif

static struct read_write_access_control_process casdrm_process_list_lib[] = {
//    { "jp.co.mmbi.app", JAVA_APP_PROCESS_PATH, UID_NO_CHECK },
//    { "com.nttdocomo.mmb.android.mmbsv.process", JAVA_APP_PROCESS_PATH, UID_NO_CHECK },
//    { "jp.pixela.StationMobile", JAVA_APP_PROCESS_PATH, AID_MEDIA },
//    { "MmbCaCasDrmMw", "/system/bin/MmbCaCasDrmMw", AID_ROOT },
//    { "MmbFcCtlMw", "/system/bin/MmbFcCtlMw", AID_ROOT },
//    { "MmbFcLiceMwServ", "/system/bin/MmbFcLiceMwServer", AID_ROOT }, /* "MmbFcLiceMwServ" is first 15 characters of "MmbFcLiceMwServer" */
//    { "MmbStCtlMwServi", "/system/bin/MmbStCtlMwService", AID_ROOT }, /* "MmbStCtlMwServi" is first 15 characters of "MmbStCtlMwService" */
//    { "MmbFcMp4MwServe", "/system/bin/MmbFcMp4MwServer", AID_ROOT }, /* "MmbFcMp4MwServe" is first 15 characters of "MmbFcMp4MwServer" */
//    { "ficsd", "/system/bin/ficsd", AID_SYSTEM },
//    { "debuggerd", "/system/bin/debuggerd", AID_ROOT },
//    { "installd", "/system/bin/installd", AID_ROOT },
//#if (FJFEAT_PRODUCT == FJFEAT_PRODUCT_BOBSLEIGH || FJFEAT_PRODUCT == FJFEAT_PRODUCT_NABOOD)
//    { "fmt_dtv_conflic", "/system/bin/fmt_dtv_conflict", AID_ROOT }, /* "fmt_dtv_conflic" is first 15 characters of "fmt_dtv_conflict" */
//#endif
    { 0, 0, 0 },
};

static struct read_write_access_control_process casdrm_process_list_bin[] = {
//    { INIT_PROCESS_NAME, INIT_PROCESS_PATH, AID_ROOT },
//    { "ficsd", "/system/bin/ficsd", AID_SYSTEM },
    { 0, 0, 0 },
};

/* ADD start FUJITSU TEN :2013-08-21 LSM */
static struct read_write_access_control_process fjsec_inspection_process_list[] = {
    { "com.fujitsu_ten.displayaudio.inspectionservice", JAVA_APP_PROCESS_PATH, UID_NO_CHECK},
    {0, 0, 0},
};
/* ADD end FUJITSU TEN :2013-08-21 LSM */

struct read_write_access_control {
    char *prefix;
    struct read_write_access_control_process *process_list;
};


static struct read_write_access_control rw_access_control_list[] = {
/* ADD start FUJITSU TEN :2013-08-21 LSM */
    { "/system/bin/inspection_emmc_read_check", fjsec_inspection_process_list},
    { "/system/bin/inspection_get_mac_address", fjsec_inspection_process_list},
    { "/system/bin/inspection_set_mac_address", fjsec_inspection_process_list},
    { "/system/bin/inspection_wifi_calibration", fjsec_inspection_process_list},
/* ADD end FUJITSU TEN :2013-08-21 LSM */
/* ADD start FUJITSU TEN :2014-02-14 LSM */
    { "/system/bin/inspection_touch_update", fjsec_inspection_process_list},
/* ADD end FUJITSU TEN :2014-02-14 LSM */
//    { "/system/lib/libMmbCaCasDrmMw.so", casdrm_process_list_lib },
//    { "/system/lib/libMmbCaKyMngMw.so", casdrm_process_list_lib },
//    { "/system/bin/MmbCaCasDrmMw", casdrm_process_list_bin },
//#ifdef DEBUG_COPY_GUARD
//    { "/system/lib/libptct.so", #test_process_list },
//    { "/data/lsm-test/tmp/ptct/*", test_process_list },
//#endif
    { 0, 0 },
};

struct ac_config {
    char *prefix;
    enum boot_mode_type boot_mode;
    char *process_name;
    char *process_path;
};

static struct ac_config ptn_acl[] = {
/* MOD start FUJITSU TEN :2013-05-10 LSM */
/* MOD start FUJITSU TEN :2013-08-30 LSM */
//    { "/dev/block/mmcblk0p1", 0, 0, 0 },  //APP
    { "/dev/block/mmcblk0p2", 0, 0, 0 }, //AP2
    { "/dev/block/mmcblk0p3", 0, 0, 0 }, //SOS
    { "/dev/block/mmcblk0p4", 0, 0, 0 }, //LM4
    { "/dev/block/mmcblk0p5", 0, 0, 0 }, //NUT
    { "/dev/block/mmcblk0p6", 0, 0, 0 }, //PAD
    { "/dev/block/mmcblk0p7", 0, 0, 0 }, //NV1
    { "/dev/block/mmcblk0p8", 0, 0, 0 }, //EKS
    { "/dev/block/mmcblk0p9", 0, 0, 0 }, //SC1
    { "/dev/block/mmcblk0p10", 0, 0, 0 }, //SC2
//    { "/dev/block/mmcblk0p11", 0, 0, 0 }, //SST
    { "/dev/block/mmcblk0p12", BOOT_MODE_NONE, "tsk_sys_System", "/system/bin/DrcInterMiddle" }, //LLG
    { "/dev/block/mmcblk0p13", 0, 0, 0 }, //KLG
    { "/dev/block/mmcblk0p14", BOOT_MODE_NONE, "tsk_sys_System", "/system/bin/DrcInterMiddle" }, //ALG
    { "/dev/block/mmcblk0p15", 0, 0, 0 }, //LOG
    { "/dev/block/mmcblk0p16", 0, 0, 0 }, //PP1
    { "/dev/block/mmcblk0p17", 0, 0, 0 }, //PP2
    { "/dev/block/mmcblk0p18", 0, 0, 0 }, //MSC
    { "/dev/block/mmcblk0p19", 0, 0, 0 }, //CAC
    { "/dev/block/mmcblk0p20", 0, 0, 0 }, //UDA
//    { "/dev/block/mmcblk0p21", 0, 0, 0 }, //
/* MOD end FUJITSU TEN :2013-08-30 LSM */
//    { "/dev/block/mmcblk0p1", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p2", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p3", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p4", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p5", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p6", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p7", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p8", 0, 0, 0 }, 
////    { "/dev/block/mmcblk0p9", 0, 0, 0 },  //SST
//    { "/dev/block/mmcblk0p10", 0, 0, 0 },
//    { "/dev/block/mmcblk0p11", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p12", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p13", 0, 0, 0 }, 
////    { "/dev/block/mmcblk0p14", 0, 0, 0 }, //APP
//    { "/dev/block/mmcblk0p15", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p16", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p17", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p18", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p19", 0, 0, 0 }, 
//    { "/dev/block/mmcblk0p20", 0, 0, 0 }, 
//#if (FJFEAT_PRODUCT == FJFEAT_PRODUCT_BOBSLEIGH || FJFEAT_PRODUCT == FJFEAT_PRODUCT_IRUKAD)
//    { "/dev/block/mmcblk0p1", 0, 0, 0 }, /* CID */
//    { "/dev/block/mmcblk0p2", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CVE */
//    { "/dev/block/mmcblk0p3", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CGD */
//    { "/dev/block/mmcblk0p4", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CL2 */
//    { "/dev/block/mmcblk0p5", 0, 0, 0 }, /* CDM */
//    { "/dev/block/mmcblk0p6", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CL1 */
//    { "/dev/block/mmcblk0p7", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CNV */
//    { "/dev/block/mmcblk0p8", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CD1 */
//    { "/dev/block/mmcblk0p9", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CD2 */
//    { "/dev/block/mmcblk0p10", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CLM */
//    { "/dev/block/mmcblk0p11", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CBB */
//    { "/dev/block/mmcblk0p12", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CRF */
//    { "/dev/block/mmcblk0p13", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CRV */
//    { "/dev/block/mmcblk0p14", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CDS */
//    { "/dev/block/mmcblk0p15", 0, 0, 0 }, /* DM1 */
//    { "/dev/block/mmcblk0p16", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* LM4 */
//    { "/dev/block/mmcblk0p17", 0, 0, 0 }, /* NV1 */
//    { "/dev/block/mmcblk0p18", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* VER */
//    { "/dev/block/mmcblk0p19", 0, 0, 0 }, /* PID */
//    { "/dev/block/mmcblk0p20", 0, 0, 0 }, /* SIM */
//    { "/dev/block/mmcblk0p21", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC1 */
//    { "/dev/block/mmcblk0p22", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC2 */
//    { "/dev/block/mmcblk0p23", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC3 */
//    { "/dev/block/mmcblk0p24", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC4 */
//    { "/dev/block/mmcblk0p25", 0, 0, 0 }, /* LLG */
//    { "/dev/block/mmcblk0p26", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* BI1 */
//    { "/dev/block/mmcblk0p27", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* BI2 */
//    //{ "/dev/block/mmcblk0p28", 0, 0, 0 }, /* SST */
//    //{ "/dev/block/mmcblk0p29", 0, 0, 0 }, /* KLG */
//    //{ "/dev/block/mmcblk0p30", 0, 0, 0 }, /* ALG */
//    //{ "/dev/block/mmcblk0p31", 0, 0, 0 }, /* LOG */
//    { "/dev/block/mmcblk0p32", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SOS */
//    { "/dev/block/mmcblk0p33", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* LNX */
//    //{ "/dev/block/mmcblk0p34", 0, 0, 0 }, /* CAC */
//    { "/dev/block/mmcblk0p35", 0, 0, 0 }, /* MSC */
//    { "/dev/block/mmcblk0p36", 0, 0, 0 }, /* USP */
//    { "/dev/block/mmcblk0p37", 0, 0, 0 }, /* FOT */
//    //{ "/dev/block/mmcblk0p38", 0, 0, 0 }, /* PRI */
//    { "/dev/block/mmcblk0p39", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* NUT */
//    { "/dev/block/mmcblk0p40", BOOT_MODE_RECOVERY,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH }, /* MMD */
//    { "/dev/block/mmcblk0p40", BOOT_MODE_SDDOWNLOADER,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH }, /* MMD */
//    //{ "/dev/block/mmcblk0p41", BOOT_MODE_RECOVERY,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH },  /* KIT */
//    //{ "/dev/block/mmcblk0p41", BOOT_MODE_SDDOWNLOADER,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH },/* KIT */
//    { "/dev/block/mmcblk0p42", 0, 0, 0 }, /* PIM */
//    { "/dev/block/mmcblk0p43", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* APP */
//    //{ "/dev/block/mmcblk0p44", 0, 0, 0 }, /* JSY */
//    //{ "/dev/block/mmcblk0p45", 0, 0, 0 }, /* UDA */
//#elif (FJFEAT_PRODUCT == FJFEAT_PRODUCT_NABOOD)
//    { "/dev/block/mmcblk0p1", 0, 0, 0 }, /* CID */
//    { "/dev/block/mmcblk0p2", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CVE */
//    { "/dev/block/mmcblk0p3", 0, 0, 0 }, /* CMD */
//    { "/dev/block/mmcblk0p4", 0, 0, 0 }, /* CCD */
//    { "/dev/block/mmcblk0p5", 0, 0, 0 }, /* CVD */
//    { "/dev/block/mmcblk0p6", 0, 0, 0 }, /* CPS */
//    { "/dev/block/mmcblk0p7", 0, 0, 0 }, /* CHD */
//    { "/dev/block/mmcblk0p8", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CGD */
//    { "/dev/block/mmcblk0p9", 0, 0, 0 }, /* CED */
//    { "/dev/block/mmcblk0p10", 0, 0, 0 }, /* CFS */
//    { "/dev/block/mmcblk0p11", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CL2 */
//    { "/dev/block/mmcblk0p12", 0, 0, 0 }, /* CDM */
//    { "/dev/block/mmcblk0p13", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CL1 */
//    { "/dev/block/mmcblk0p14", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CNV */
//    { "/dev/block/mmcblk0p15", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CD1 */
//    { "/dev/block/mmcblk0p16", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CD2 */
//    { "/dev/block/mmcblk0p17", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CLM */
//    { "/dev/block/mmcblk0p18", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* CBB */
//    { "/dev/block/mmcblk0p19", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* LM4 */
//    { "/dev/block/mmcblk0p20", 0, 0, 0 }, /* NV1 */
//    { "/dev/block/mmcblk0p21", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* VER */
//    { "/dev/block/mmcblk0p22", 0, 0, 0 }, /* PID */
//    { "/dev/block/mmcblk0p23", 0, 0, 0 }, /* SIM */
//    { "/dev/block/mmcblk0p24", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC1 */
//    { "/dev/block/mmcblk0p25", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC2 */
//    { "/dev/block/mmcblk0p26", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC3 */
//    { "/dev/block/mmcblk0p27", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SC4 */ 
//    { "/dev/block/mmcblk0p28", 0, 0, 0 }, /* LLG */ 
//    { "/dev/block/mmcblk0p29", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* BI1 */ 
//    { "/dev/block/mmcblk0p30", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* BI2 */
//    //{ "/dev/block/mmcblk0p31", 0, 0, 0 }, /* SST */
//    //{ "/dev/block/mmcblk0p32", 0, 0, 0 }, /* KLG */
//    //{ "/dev/block/mmcblk0p33", 0, 0, 0 }, /* ALG */
//    //{ "/dev/block/mmcblk0p34", 0, 0, 0 }, /* LOG */
//    { "/dev/block/mmcblk0p35", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* SOS */
//    { "/dev/block/mmcblk0p36", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* LNX */
//    //{ "/dev/block/mmcblk0p37", 0, 0, 0 }, /* CAC */
//    { "/dev/block/mmcblk0p38", 0, 0, 0 }, /* MSC */
//    { "/dev/block/mmcblk0p39", 0, 0, 0 }, /* USP */
//    { "/dev/block/mmcblk0p40", 0, 0, 0 }, /* FOT */
//    //{ "/dev/block/mmcblk0p41", 0, 0, 0 }, /* PRI */
//    { "/dev/block/mmcblk0p42", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* NUT */
//    { "/dev/block/mmcblk0p43", BOOT_MODE_RECOVERY,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH }, /* MMD */
//    { "/dev/block/mmcblk0p43", BOOT_MODE_SDDOWNLOADER,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH }, /* MMD */
//    //{ "/dev/block/mmcblk0p44", BOOT_MODE_RECOVERY,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH },  /* KIT */
//    //{ "/dev/block/mmcblk0p44",BOOT_MODE_SDDOWNLOADER,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH },/* KIT */
//    { "/dev/block/mmcblk0p45", 0, 0, 0 }, /* PIM */
//    { "/dev/block/mmcblk0p46", BOOT_MODE_FOTA, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME, CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH }, /* APP */
//    //{ "/dev/block/mmcblk0p47", 0, 0, 0 }, /* JSY */
//    //{ "/dev/block/mmcblk0p48", 0, 0, 0 }, /* UDA */
//#endif

//#ifdef DEBUG_ACCESS_CONTROL
//    { "/dev/block/mmcblk0p23", BOOT_MODE_NONE, "test", "/data/lsm-test/tmp/test_tool" }, /* fota */
//    { TEST_PTN_PATH, 0, 0, 0 }, /* fota2 */
//#endif
/* MOD end FUJITSU TEN :2013-05-10 LSM */
    { 0, 0, 0, 0 },
};

static struct ac_config fs_acl[] = {
//#ifdef DEBUG_ACCESS_CONTROL
//    { "/data/lsm-test/tmp/fspt-ptct/*", BOOT_MODE_NONE, "test", "/data/lsm-test/tmp/test_tool" },
//    { "/data/lsm-test/tmp/fspt-ptct.txt", BOOT_MODE_NONE, "test", "/data/lsm-test/tmp/test_tool" },
//    { "/data/lsm-test/tmp/fspt-ptct2.txt", 0, 0, 0 },
//    { TEST_DIRS_PATH, BOOT_MODE_NONE, "test", "/data/lsm-test/tmp/test_tool" },
//#endif
    { 0, 0, 0, 0 },
};

struct ac_config_devmem {
    unsigned long head;
    unsigned long tail;
    char *process_name;
    char *process_path;
};

static struct ac_config_devmem devmem_acl[] = {

    { 0, 0, 0, 0 },
};

#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING
struct kitting_directory_access_process {
    char *process_name;
    char *process_path;
    uid_t uid;
};

static struct kitting_directory_access_process kitting_directory_access_process_list[] = {
    { "com.android.settings", JAVA_APP_PROCESS_PATH, AID_SYSTEM },
    { "com.android.defcontainer", JAVA_APP_PROCESS_PATH, UID_NO_CHECK },
#ifdef DEBUG_KITTING_ACCESS_CONTROL
    { "com.android.settings", "/system/bin/busybox", AID_SYSTEM },
#endif
    { 0, 0, 0 },
};
#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */

