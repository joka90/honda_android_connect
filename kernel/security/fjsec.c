/*
 * FJSEC LSM module
 *
 * based on deckard.c
 * based on root_plug.c
 * Copyright (C) 2002 Greg Kroah-Hartman <greg@kroah.com>
 *
 * _xx_is_valid(), _xx_encode(), _xx_realpath_from_path()
 * is ported from security/tomoyo/realpath.c in linux-2.6.32
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012 - 2013
/*----------------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/security.h>
#include <linux/moduleparam.h>
#include <linux/mount.h>
#include <linux/mnt_namespace.h>
#include <linux/fs_struct.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/magic.h>
#include <asm/mman.h>
#include <linux/msdos_fs.h>
#include "fjsec_config.h"
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>

/* DEL start FUJITSU TEN :2013-02-18 LSM */
//#include "detect_rh.h"
/* DEL end FUJITSU TEN :2013-02-18 LSM */

#define PRCONFIG
#define DUMP_PRINFO
//#define DUMP_PTRACE_REQUEST

//#define dprintk printk
#define dprintk(...)
//#define ada_dprintk(...) printk(__VA_ARGS__)
#define ada_dprintk(...)
/* FUJITSU TEN:2013-09-04 LSM mod start */
//#define ada_dprintk2(...) printk(__VA_ARGS__)
#define ada_dprintk2(...)
/* FUJITSU TEN:2013-09-04 LSM mod end */

static enum boot_mode_type boot_mode = BOOT_MODE_NONE;

//static bool mount_flag = true;
static bool mount_flag = false;
#ifdef DUMP_PTRACE_REQUEST
static long prev_request = -1;
#endif

static inline bool _xx_is_valid(const unsigned char c)
{
    return c > ' ' && c < 127;
}

static int _xx_encode(char *buffer, int buflen, const char *str)
{
    while (1) {
        const unsigned char c = *(unsigned char *) str++;

        if (_xx_is_valid(c)) {
            if (--buflen <= 0)
                break;
            *buffer++ = (char) c;
            if (c != '\\')
                continue;
            if (--buflen <= 0)
                break;
            *buffer++ = (char) c;
            continue;
        }
        if (!c) {
            if (--buflen <= 0)
                break;
            *buffer = '\0';
            return 0;
        }
        buflen -= 4;
        if (buflen <= 0)
            break;
        *buffer++ = '\\';
        *buffer++ = (c >> 6) + '0';
        *buffer++ = ((c >> 3) & 7) + '0';
        *buffer++ = (c & 7) + '0';
    }
    return -ENOMEM;
}

static int _xx_realpath_from_path(struct path *path, char *newname,
                  int newname_len)
{
    struct dentry *dentry = path->dentry;
    int error = -ENOMEM;
    char *sp;

    if (!dentry || !path->mnt || !newname || newname_len <= 2048)
        return -EINVAL;
    if (dentry->d_op && dentry->d_op->d_dname) {
        /* For "socket:[\$]" and "pipe:[\$]". */
        static const int offset = 1536;
        sp = dentry->d_op->d_dname(dentry, newname + offset,
                       newname_len - offset);
    }
    else {
        /* go to whatever namespace root we are under */
        sp = d_absolute_path(path, newname, newname_len);

        /* Prepend "/proc" prefix if using internal proc vfs mount. */
        if (!IS_ERR(sp) && (path->mnt->mnt_flags & MNT_INTERNAL) &&
            (path->mnt->mnt_sb->s_magic == PROC_SUPER_MAGIC)) {
            sp -= 5;
            if (sp >= newname)
                memcpy(sp, "/proc", 5);
            else
                sp = ERR_PTR(-ENOMEM);
        }
    }
    if (IS_ERR(sp)) {
        error = PTR_ERR(sp);
    }
    else {
        error = _xx_encode(newname, sp - newname, sp);
    }
    /* Append trailing '/' if dentry is a directory. */
    if (!error && dentry->d_inode && S_ISDIR(dentry->d_inode->i_mode)
        && *newname) {
        sp = newname + strlen(newname);
        if (*(sp - 1) != '/') {
            if (sp < newname + newname_len - 4) {
                *sp++ = '/';
                *sp = '\0';
            } else {
                error = -ENOMEM;
            }
        }
    }
    return error;
}

static int __init setup_mode(char *str)
{
/* MOD start FUJITSU TEN :2013-04-12 LSM */
//    if (strcmp(str, BOOT_ARGS_MODE_FOTA) == 0) {
//        boot_mode = BOOT_MODE_FOTA;
//    } else if (strcmp(str, BOOT_ARGS_MODE_SDDOWNLOADER) == 0) {
//        boot_mode = BOOT_MODE_SDDOWNLOADER;
//    } else if (strcmp(str, BOOT_ARGS_MODE_RECOVERY) == 0) {
//        boot_mode = BOOT_MODE_RECOVERY;
//    } else if (strcmp(str, BOOT_ARGS_MODE_MAKERCMD) == 0) {
//        boot_mode = BOOT_MODE_MAKERCMD;
//    } else if (strcmp(str, BOOT_ARGS_MODE_OSUPDATE) == 0) {
//        boot_mode = BOOT_MODE_OSUPDATE;
//    } else if (strcmp(str, BOOT_ARGS_MODE_RECOVERYMENU) == 0) {
//        boot_mode = BOOT_MODE_SDDOWNLOADER;
//    } else if (strcmp(str, BOOT_ARGS_MODE_KERNEL) == 0) {
//        boot_mode = BOOT_MODE_MAKERCMD;
//    }
    if (strcmp(str, BOOT_ARGS_MODE_OSUPDATE) == 0) {
        boot_mode = BOOT_MODE_OSUPDATE;
    }
/* MOD end FUJITSU TEN :2013-04-12 LSM */
    dprintk(KERN_INFO "boot mode=<%d>\n", boot_mode);
    return 0;
}
early_param("mode", setup_mode);

static char *get_process_path(struct task_struct *task, char *buf, size_t size)
{
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    char *cp = NULL;

    mm = task->mm;
    if (!mm) {
        dprintk(KERN_INFO "%s:%d mm is null.\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    down_read(&mm->mmap_sem);
    for (vma = mm->mmap; vma; vma = vma->vm_next) {
        if ((vma->vm_flags & VM_EXECUTABLE) && vma->vm_file) {
            cp = d_path(&vma->vm_file->f_path, buf, size);
            break;
        }
    }
    up_read(&mm->mmap_sem);

    return cp;
}

#ifdef DUMP_PRINFO
static void dump_prinfo(const char *function_name, int line_number)
{
    char *binname;
    char *buf = kzalloc(PATH_MAX, GFP_NOFS);

    if (!buf) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return;
    }
    binname = get_process_path(current, buf, PATH_MAX-1);
    printk(KERN_INFO "%s:%d: current process name=<%s>, path=<%s>, uid=<%d>\n"
            , function_name, line_number, current->comm, binname, current->cred->uid);
    kfree(buf);
}
#else
#define dump_prinfo(a, b)
#endif


static int fjsec_check_access_process_path(char *process_path)
{
    char *buf = kzalloc(PATH_MAX, GFP_NOFS);
    char *binname;

    if (!buf) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    binname = get_process_path(current, buf, PATH_MAX-1);
    if (binname == NULL || IS_ERR(binname)) {
        printk(KERN_INFO "%s:%d: Failed getting process path.\n", __FUNCTION__, __LINE__);
        kfree(buf);
        return -EPERM;
    }

    dprintk(KERN_INFO "%s:%d: process path=%s\n", __FUNCTION__, __LINE__, binname);

    if (strcmp(binname, process_path) == 0) {
        kfree(buf);
        return 0;
    }

    dprintk(KERN_INFO "%s:%d: mismatched process path. config=<%s>, current=<%s>\n"
            , __FUNCTION__, __LINE__, process_path, binname);
    kfree(buf);
    return -EPERM;
}

static int fjsec_check_access_process(char *process_name, char *process_path)
{
    dprintk(KERN_INFO "%s:%d: process name=%s\n", __FUNCTION__, __LINE__, current->comm);

    if (strcmp(current->comm, process_name) == 0) {
        if (fjsec_check_access_process_path(process_path) == 0) {
            return 0;
        }
    } else {
        dprintk(KERN_INFO "%s:%d: mismatched process name. config=<%s>, current=<%s>\n"
                , __FUNCTION__, __LINE__, process_name, current->comm);
    }

    return -EPERM;
}

int fjsec_check_mmcdl_access_process(void)
{
    int index;

    for (index = 0; mmcdl_device_list[index].process_name; index++) {
        if (boot_mode == mmcdl_device_list[index].boot_mode) {
            if (fjsec_check_access_process(mmcdl_device_list[index].process_name,
                                            mmcdl_device_list[index].process_path) == 0) {
                return 0;
            }
        }
    }

    printk(KERN_INFO "%s:%d: REJECT Failed accessing mmcdl device. process name=%s, uid=%d, pid=%d\n",
         __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
    return -1;
}

int fjsec_check_devmem_access(unsigned long pfn)
{
    int index;

    for (index = 0; index < ARRAY_SIZE(devmem_acl); index++) {
        if ((pfn >= devmem_acl[index].head) && (pfn <= devmem_acl[index].tail)) {
            if (fjsec_check_access_process(devmem_acl[index].process_name, devmem_acl[index].process_path) == 0) {
                dprintk(KERN_INFO "%s:%d: GRANTED accessing devmem. pfn=<0x%lx> process name=<%s> process path=<%s>\n"
                            , __FUNCTION__, __LINE__, pfn, devmem_acl[index].process_name, devmem_acl[index].process_path);
                return 0;
            }
        }
    }

    dump_prinfo(__FUNCTION__, __LINE__);
    printk(KERN_INFO "%s:%d: REJECT Failed accessing devmem. pfn=<0x%lx> process name=%s, uid=%d, pid=%d\n",
         __FUNCTION__, __LINE__, pfn, current->comm, current->cred->uid, current->pid);
    return -1;
}

static int fjsec_check_disk_device_access_offset(struct accessible_area_disk_dev *accessible_areas, loff_t head, loff_t length)
{
    struct accessible_area_disk_dev *accessible_area;
    loff_t tail;

    tail = head + (length - 1);

    if (tail < head) {
        tail = head;
    }

    dprintk(KERN_INFO "%s:%d: head=<0x%llx>, length=<0x%llx>: tail=<0x%llx>\n"
        , __FUNCTION__, __LINE__, head, length, tail);

    for (accessible_area = accessible_areas; accessible_area->tail; accessible_area++) {
        if ((accessible_area->head <= head) && (accessible_area->tail >= head)) {
            if ((accessible_area->head <= tail) && (accessible_area->tail >= tail)) {
                dprintk(KERN_INFO "%s:%d: SUCCESS accessing disk device.\n", __FUNCTION__, __LINE__);
                return 0;
            }
        }
    }

    printk(KERN_INFO "%s:%d: REJECT Failed accessing disk device. process name=%s, uid=%d, pid=%d\n",
         __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
    return -EPERM;
}

static int fjsec_check_disk_device_access(char *realpath, loff_t offset, loff_t length)
{
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_DISK_DEV_PATH) == 0) {
        dprintk(KERN_INFO "%s:%d:boot mode=<%d>\n", __FUNCTION__, __LINE__, boot_mode);
/* MOD start FUJITSU TEN :2013-04-12 LSM */
//        if (boot_mode == BOOT_MODE_FOTA) {
//            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME,
//                                            CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH) == 0) {
//                if(fjsec_check_disk_device_access_offset(accessible_areas_fota, offset, length) == 0) {
//                    return 0;
//                }
//            }
//        }
//        else if (boot_mode == BOOT_MODE_SDDOWNLOADER) {
//            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,
//                                            CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH) == 0) {
//                if(fjsec_check_disk_device_access_offset(accessible_areas_sddownloader, offset, length) == 0) {
//                    return 0;
//                }
//            }
//        }
//        else if (boot_mode == BOOT_MODE_OSUPDATE) {
        if (boot_mode == BOOT_MODE_OSUPDATE) {
            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
                                            CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH1) == 0) {
                if(fjsec_check_disk_device_access_offset(accessible_areas_osupdate, offset, length) == 0) {
                    return 0;
                }
            }
            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
                                            CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH2) == 0) {
                if(fjsec_check_disk_device_access_offset(accessible_areas_osupdate, offset, length) == 0) {
                    return 0;
                }
            }
        }
//        else if (boot_mode == BOOT_MODE_RECOVERY) {
//            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,
//                                            CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH) == 0) {
//                if(fjsec_check_disk_device_access_offset(accessible_areas_recovery, offset, length) == 0) {
//                    return 0;
//                }
//            }
//        }
//        else if (boot_mode == BOOT_MODE_MAKERCMD) {
//            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_NAME,
//                                            CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_PATH) == 0) {
//                if(fjsec_check_disk_device_access_offset(accessible_areas_maker, offset, length) == 0) {
//                    return 0;
//                }
//            }
//        }
/* MOD end FUJITSU TEN :2013-04-12 LSM */
        return -EPERM;
    }

    return 0;
}

static int fjsec_check_system_access_process(void)
{
/* MOD start FUJITSU TEN :2013-04-12 LSM */
    if (boot_mode == BOOT_MODE_OSUPDATE) {
        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
                                        CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH1) == 0) {
            return 0;
        }
        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
                                        CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH2) == 0) {
            return 0;
        }
    }
//    if (boot_mode == BOOT_MODE_FOTA) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }
//
//#if 0
//    if (boot_mode == BOOT_MODE_SDDOWNLOADER) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }
//    if (boot_mode == BOOT_MODE_OSUPDATE) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }
//#endif
/* MOD end FUJITSU TEN :2013-04-12 LSM */

    if (!current->mm) {
        dprintk(KERN_INFO "%s:%d (!current->mm)\n", __FUNCTION__, __LINE__);
        return 0;
    }
/* DEL start FUJITSU TEN :2013-02-18 LSM */
//    set_rhflag();
/* DEL start FUJITSU TEN :2013-02-18 LSM */
    return -1;
}

static int fjsec_check_system_directory_access_process(char *realpath)
{
    if (strncmp(realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH,
                    strlen(CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH)) == 0) {
        if (fjsec_check_system_access_process() == 0) {
            return 0;
        }

        return -EPERM;
    }

    return 0;
}

static int fjsec_check_system_device_access_process(char *realpath)
{
/* FUJITSU TEN:2013-09-04 LSM mod start */
    if ((strcmp(realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH) == 0) | 
        (strcmp(realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH2) == 0)) {
        ada_dprintk2(KERN_INFO "%s:%d match system dev path: process name=%s, realpath=%s\n"
                        , __FUNCTION__, __LINE__, current->comm, realpath);
/* FUJITSU TEN:2013-09-04 LSM mod end */

        if (fjsec_check_system_access_process() == 0) {
            return 0;
        }

        return -EPERM;
    }
/* FUJITSU TEN:2013-09-04 LSM mod start */
    ada_dprintk2(KERN_INFO "%s:%d mismatch system dev path: process name=%s, realpath=%s\n"
                    , __FUNCTION__, __LINE__, current->comm, realpath);
/* FUJITSU TEN:2013-09-04 LSM mod end */

    return 0;
}

#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
static int fjsec_check_secure_storage_access_process(void)
{
/* MOD start FUJITSU TEN :2013-04-12 LSM */
//#if 0
//    if (boot_mode == BOOT_MODE_FOTA) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }
//    else 
//#endif
//    if (boot_mode == BOOT_MODE_SDDOWNLOADER) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }
//    else if (boot_mode == BOOT_MODE_OSUPDATE) {
    if (boot_mode == BOOT_MODE_OSUPDATE) {
        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
                                        CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH1) == 0) {
            return 0;
        }
        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME,
                                        CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH2) == 0) {
            return 0;
        }
    }
//    else if (boot_mode == BOOT_MODE_RECOVERY) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }
//    else if (boot_mode == BOOT_MODE_MAKERCMD) {
//        if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_NAME,
//                                        CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_PATH) == 0) {
//            return 0;
//        }
//    }

    if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_SECURE_STORAGE_ACCESS_PROCESS_NAME,
                                        CONFIG_SECURITY_FJSEC_SECURE_STORAGE_ACCESS_PROCESS_PATH) == 0) {
            return 0;
    }

    if (fjsec_check_access_process(INIT_PROCESS_NAME, INIT_PROCESS_PATH) == 0) {
        return 0;
    }
/* MOD end FUJITSU TEN :2013-04-12 LSM */
    return -1;
}

static int fjsec_check_secure_storage_directory_access_process(char *realpath)
{
    if (strncmp(realpath, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH,
                    strlen(CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH)) == 0) {
        if (fjsec_check_secure_storage_access_process() == 0) {
            return 0;
        }

        return -EPERM;
    }

    return 0;
}

static int fjsec_check_secure_storage_device_access_process(char *realpath)
{
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DEV_PATH) == 0) {
        if (fjsec_check_secure_storage_access_process () == 0) {
            return 0;
        }

        return -EPERM;
    }

    return 0;
}

#else

#define fjsec_check_secure_storage_directory_access_process(a) (0)
#define fjsec_check_secure_storage_device_access_process(a) (0)

#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */

#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING

static int fjsec_check_kitting_access_process(char *process_name, char *process_path)
{
    dprintk(KERN_INFO "%s:%d: process name=%s\n", __FUNCTION__, __LINE__, current->comm);

    if (strcmp(current->group_leader->comm, process_name) == 0) {
        if (fjsec_check_access_process_path(process_path) == 0) {
            return 0;
        }
    } else {
        dprintk(KERN_INFO "%s:%d: mismatched process name. config=<%s>, current=<%s>\n"
                , __FUNCTION__, __LINE__, process_name, current->group_leader->comm);
    }

    return -EPERM;
}

static int fjsec_check_kitting_directory_access_process(char *realpath)
{
    if (strncmp(realpath, CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH,
                    strlen(CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH)) == 0) {
        if (boot_mode == BOOT_MODE_NONE) {
            struct kitting_directory_access_process *kd_access_control_process;
            
            for (kd_access_control_process = kitting_directory_access_process_list; kd_access_control_process->process_path; kd_access_control_process++) {
                char *process_name;
                int process_name_len;

                if (kd_access_control_process->uid != UID_NO_CHECK && kd_access_control_process->uid != current->cred->uid) {
                    dprintk(KERN_INFO "%s:%d: mismatched process UID\n", __FUNCTION__, __LINE__);
                    continue;
                }

                process_name_len = strlen(kd_access_control_process->process_name);
                process_name = kd_access_control_process->process_name;

                if (process_name_len > (TASK_COMM_LEN - 1)) {
                    process_name += (process_name_len - (TASK_COMM_LEN - 1));
                }

                if (fjsec_check_kitting_access_process(process_name,
                                                kd_access_control_process->process_path) != 0) {
                    dprintk(KERN_INFO "%s:%d: mismatched process name, process path\n", __FUNCTION__, __LINE__);
                    continue;
                }

                dprintk(KERN_INFO "%s:%d: SUCCESS realpath=%s\n", __FUNCTION__, __LINE__, realpath);
                return 0;
            }
        }

        if (fjsec_check_access_process(INIT_PROCESS_NAME, INIT_PROCESS_PATH) == 0) {
            return 0;
        }

        return -EPERM;
    }

    return 0;
}

static int fjsec_check_kitting_device_access_process(char *realpath)
{
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_KITTING_DEV_PATH) == 0) {
        if (boot_mode == BOOT_MODE_SDDOWNLOADER) {
            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME,
                                            CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH) == 0) {
                return 0;
            }
        } 
        else if (boot_mode == BOOT_MODE_RECOVERY) {
            if (fjsec_check_access_process(CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME,
                                            CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH) == 0) {
                return 0;
            }
        }

        return -EPERM;
    }

    return 0;
}

#else

#define fjsec_check_kitting_directory_access_process(a) (0)
#define fjsec_check_kitting_device_access_process(a) (0)

#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */

static int fjsec_check_device_access_process(char *realpath, struct ac_config *acl)
{
    struct ac_config *config;
    int result = 0;

    for (config = acl; config->prefix; config++) {
        int length = strlen(config->prefix);

        if (config->prefix[length - 1] == '*') {
            if (strncmp(realpath, config->prefix, length - 1) != 0) {
                continue;
            }
        } else {
            if (strcmp(realpath, config->prefix) != 0) {
                continue;
            }
        }

        if (!config->boot_mode && !config->process_name && !config->process_path) {
            printk(KERN_INFO "%s:%d: <%s> is banned access.\n", __FUNCTION__, __LINE__, config->prefix);
            return -EPERM;
        }

        if (boot_mode == config->boot_mode) {
            if (fjsec_check_access_process(config->process_name, config->process_path) == 0) {
                return 0;
            }
        }

        result = -EPERM;
    }

    return result;
}

static int fjsec_read_write_access_control(char *realpath)
{
    struct read_write_access_control *rw_access_control;
    struct read_write_access_control_process *rw_access_control_process;
    int result = 1;

    if (!(boot_mode == BOOT_MODE_NONE || boot_mode == BOOT_MODE_RECOVERY)) {
        return 1;
    }

    for (rw_access_control = rw_access_control_list; rw_access_control->prefix; rw_access_control++) {
        int length = strlen(rw_access_control->prefix);

        if (rw_access_control->prefix[length - 1] == '*') {
            if (strncmp(rw_access_control->prefix, realpath, length - 1) != 0) {
                continue;
            }
        } else {
            if (strcmp(rw_access_control->prefix, realpath) != 0) {
                continue;
            }
        }
printk(KERN_INFO "%s:%d: prefix hit uid:%d \n", __FUNCTION__, __LINE__, current->cred->uid);
        if (current->cred->uid == AID_ROOT || current->cred->uid == AID_SYSTEM) {
            return 0;
        }

        result = -EPERM;

        for (rw_access_control_process = rw_access_control->process_list; rw_access_control_process->process_path; rw_access_control_process++) {
            char *process_name;
            int process_name_len;
///////
//            printk (KERN_INFO "  config=<%s><%s><%s><%d>\n"
//                    , rw_access_control->prefix, rw_access_control_process->process_name, rw_access_control_process->process_path, rw_access_control_process->uid);
            if (rw_access_control_process->uid != UID_NO_CHECK && rw_access_control_process->uid != current->cred->uid) {
                dprintk(KERN_INFO "%s:%d: mismatched process UID\n", __FUNCTION__, __LINE__);
                continue;
            }

            if (rw_access_control_process->process_name != PRNAME_NO_CHECK) {
                process_name_len = strlen(rw_access_control_process->process_name);
                process_name = rw_access_control_process->process_name;

                if (process_name_len > (TASK_COMM_LEN - 1)) {
                    process_name += (process_name_len - (TASK_COMM_LEN - 1));
                }

                if (fjsec_check_access_process(process_name,
                                                rw_access_control_process->process_path) != 0) {
                    dprintk(KERN_INFO "%s:%d: mismatched process name, process path\n", __FUNCTION__, __LINE__);
                    continue;
                }
            } else {
                if (fjsec_check_access_process_path(rw_access_control_process->process_path) != 0) {
                    dprintk(KERN_INFO "%s:%d: mismatched process path\n", __FUNCTION__, __LINE__);
                    continue;
                }
            }

            dprintk(KERN_INFO "%s:%d: SUCCESS realpath=%s\n", __FUNCTION__, __LINE__, realpath);
            return 0;
        }
    }
    return result;
}

static int fjsec_check_path_from_path(struct path *path)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    int r;

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    r = fjsec_read_write_access_control(realpath);
    if (r == -EPERM) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s, process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
    else if (r == 0) {
        kfree(realpath);
        return 0;
    }

    if (fjsec_check_system_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_device_access_process(realpath, fs_acl) != 0) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_secure_storage_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
        __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_kitting_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
        __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    kfree(realpath);
    return 0;
}

static int fjsec_check_chmod_from_path(struct path *path, mode_t mode)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    int r;
    struct fs_path_config *pc;

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    dprintk(KERN_INFO "(%s:%d) path=<%s> mode=<%lu>\n", __FUNCTION__, __LINE__, realpath, mode);

    for (pc = devs; pc->prefix; pc++) {
        if (strcmp(realpath, pc->prefix) == 0) {
            if (mode == (pc->mode & S_IALLUGO)) {
                break;
            }

            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }
    }

    kfree(realpath);
    return 0;
}

static int fjsec_check_chown_from_path(struct path *path, uid_t uid, gid_t gid)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    int r;
    struct fs_path_config *pc;

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    dprintk(KERN_INFO "(%s:%d) <%s>\n", __FUNCTION__, __LINE__, realpath);

    for (pc = devs; pc->prefix; pc++) {
        if (strcmp(realpath, pc->prefix) == 0) {
            if (((uid == pc->uid) || (uid == -1)) && ((gid == pc->gid) || (gid == -1))) {
                break;
            }

            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }
    }

    kfree(realpath);
    return 0;
}

static int fjsec_check_mknod(struct path *path, struct dentry *dentry,
                 int mode, unsigned int dev)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    int r;
    struct fs_path_config *pc;
    u32 rdev;

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    if (PATH_MAX > strlen(realpath) + strlen(dentry->d_name.name)) {
        strcat(realpath, dentry->d_name.name);
    } else {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -ENOMEM;
    }

    dprintk(KERN_INFO "(%s:%d) <%s> <%s>\n", __FUNCTION__, __LINE__, realpath, dentry->d_name.name);

#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */

    rdev = new_encode_dev(dev);

    for (pc = devs; pc->prefix; pc++) {
        if (rdev != pc->rdev) {
            continue;
        }

        if (mode != pc->mode) {
            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }

        if (strcmp(realpath, pc->prefix) != 0) {
            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }

        // rdev, mode, realpath all matched
        kfree(realpath);
        return 0;
    }

    for (pc = devs; pc->prefix; pc++) {
        if (strcmp(realpath, pc->prefix) != 0) {
            continue;
        }

        if (mode != pc->mode) {
            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }

        if (rdev != pc->rdev) {
            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }

        // realpath, mode, rdev all matched
        kfree(realpath);
        return 0;
    }

    kfree(realpath);
    return 0;
}

static int fjsec_check_mount_point(char *realpath, char *dev_realpath)
{
/* FUJITSU TEN:2013-09-04 LSM mod start */
    if ((strcmp(dev_realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH) == 0) | 
        (strcmp(dev_realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH2) == 0)) {
        ada_dprintk2(KERN_INFO "%s:%d match system dev path: process name=%s, dev_realpath=%s\n"
                        , __FUNCTION__, __LINE__, current->comm, dev_realpath);
/* FUJITSU TEN:2013-09-04 LSM mod end */
        if (strcmp(realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH) == 0) {
            return MATCH_SYSTEM_MOUNT_POINT;
        }
        else {
            return MISMATCH_MOUNT_POINT;
        }
    }
    else if (strncmp(realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH,
                        strlen(CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH)) == 0) {
/* FUJITSU TEN:2013-09-04 LSM mod start */
        ada_dprintk2(KERN_INFO "%s:%d mismatch system dev path: process name=%s, dev_realpath=%s\n"
                        , __FUNCTION__, __LINE__, current->comm, dev_realpath);
/* FUJITSU TEN:2013-09-04 LSM mod end */
        return MISMATCH_MOUNT_POINT;
    }
/* FUJITSU TEN:2013-09-04 LSM mod start */
    ada_dprintk2(KERN_INFO "%s:%d mismatch system dev path: process name=%s, dev_realpath=%s\n"
                    , __FUNCTION__, __LINE__, current->comm, dev_realpath);
/* FUJITSU TEN:2013-09-04 LSM mod end */
#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
    if (strcmp(dev_realpath, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DEV_PATH) == 0) {
        if (strcmp(realpath, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH) == 0) {
            return MATCH_SECURE_STORAGE_MOUNT_POINT;
        }
        else {
            return MISMATCH_MOUNT_POINT;
        }
    }
    else if (strncmp(realpath, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH,
                        strlen(CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH)) == 0) {
        return MISMATCH_MOUNT_POINT;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */

#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING
    if (strcmp(dev_realpath, CONFIG_SECURITY_FJSEC_KITTING_DEV_PATH) == 0) {
        if (strcmp(realpath, CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH) == 0) {
            return MATCH_KITTING_MOUNT_POINT;
        }
        else {
            return MISMATCH_MOUNT_POINT;
        }
    }
    else if (strncmp(realpath, CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH,
                        strlen(CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH)) == 0) {
        return MISMATCH_MOUNT_POINT;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */

    if (strcmp(dev_realpath, FOTA_PTN_PATH) == 0) {
        if (strcmp(realpath, FOTA_MOUNT_POINT) == 0) {
            return MATCH_MOUNT_POINT;
        }
        else {
            return MISMATCH_MOUNT_POINT;
        }
    }
    else if (strncmp(realpath, FOTA_MOUNT_POINT, strlen(FOTA_MOUNT_POINT)) == 0) {
        return MISMATCH_MOUNT_POINT;
    }

#ifdef DEBUG_ACCESS_CONTROL
        if (strcmp(dev_realpath, TEST_PTN_PATH) == 0) {
                if (strcmp(realpath, TEST_MOUNT_POINT) == 0) {
                        return MATCH_MOUNT_POINT;
                }
                else {
                        return MISMATCH_MOUNT_POINT;
                }
        }
        else if (strncmp(realpath, TEST_MOUNT_POINT, strlen(TEST_MOUNT_POINT)) == 0) {
                return MISMATCH_MOUNT_POINT;
        }
#endif

    return UNRELATED_MOUNT_POINT;
}

static int fjsec_ptrace_request_check(long request)
{
    int index;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%ld):%d:", __FUNCTION__, request, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    for (index = 0; index < ARRAY_SIZE(ptrace_read_request_policy); index++) {
        if (request == ptrace_read_request_policy[index]) {
#ifdef DUMP_PTRACE_REQUEST
            if (prev_request != request) {
                printk(KERN_INFO "%s:%d: GRANTED read request. request=<%ld>, process name=<%s>, pid=<%d>.\n", __FUNCTION__, __LINE__, request, current->comm, current->pid);
                 prev_request = request;
            }
#endif
            return 0;
        }
    }

    printk(KERN_INFO "%s:%d: REJECT write request. request=<%ld>, process name=%s, uid=%d, pid=%d\n",
         __FUNCTION__, __LINE__, request, current->comm, current->cred->uid, current->pid);
    return -EPERM;
}

static int fjsec_ptrace_traceme(struct task_struct *parent)
{
    printk(KERN_DEBUG "%s:%d: REJECT pid=%d process name=%s, uid=%d, pid=%d\n",
        __FUNCTION__, __LINE__, parent->pid, current->comm, current->cred->uid, current->pid);
    return -EPERM;
}

static int fjsec_sb_mount(char *dev_name, struct path *path,
                char *type, unsigned long flags, void *data)
{
    char *realpath;
    char *dev_realpath;
    int r;
    enum result_mount_point result;
    struct path dev_path;

    realpath = kzalloc(PATH_MAX, GFP_NOFS);
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%s, %p, %s, %lu, %p):%d:", __FUNCTION__, dev_name, path, type,
        flags, data, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    dev_realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!dev_realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -ENOMEM;
    }

    r = kern_path(dev_name, LOOKUP_FOLLOW, &dev_path);
    if (r == 0) {
        r = _xx_realpath_from_path(&dev_path, dev_realpath, PATH_MAX-1);
        path_put(&dev_path);
        if (r != 0) {
            printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            kfree(dev_realpath);
            return r;
        }
    }

    dprintk(KERN_INFO "(%s:%d) <%s> <%s>\n", __FUNCTION__, __LINE__, dev_name, dev_realpath);

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        kfree(dev_realpath);
        return r;
    }

    dprintk(KERN_INFO "(%s:%d) <%s>\n", __FUNCTION__, __LINE__, realpath);

    result = fjsec_check_mount_point(realpath, dev_realpath);

    dprintk(KERN_INFO "(%s:%d) mount point check result=<%d>\n", __FUNCTION__, __LINE__, result);

    if (result == MATCH_SYSTEM_MOUNT_POINT) {
/* MOD start FUJITSU TEN :2013-04-12 LSM */
        if (boot_mode == BOOT_MODE_OSUPDATE) {
//        if (boot_mode == BOOT_MODE_FOTA || boot_mode == BOOT_MODE_SDDOWNLOADER || boot_mode == BOOT_MODE_OSUPDATE) {
/* MOD end FUJITSU TEN :2013-04-12 LSM */
            kfree(realpath);
            kfree(dev_realpath);
            return 0;
        }

        if ((flags & MS_RDONLY) == 0) {
            if ((flags & MS_REMOUNT) == 0) {
                if (mount_flag == false) {
                    mount_flag = true;
                }
                else {
                    printk(KERN_INFO "%s:%d: REJECT R/W MOUNT dev_realpath=%s realpath=%s process name=%s, uid=%d, pid=%d\n",
                        __FUNCTION__, __LINE__, dev_realpath, realpath, current->comm, current->cred->uid, current->pid);
                    kfree(realpath);
                    kfree(dev_realpath);
                    return -EPERM;
                }
            }
            else {
                printk(KERN_INFO "%s:%d: REJECT R/W REMOUNT dev_realpath=%s realpath=%s process name=%s, uid=%d, pid=%d\n",
                    __FUNCTION__, __LINE__, dev_realpath, realpath, current->comm, current->cred->uid, current->pid);
                kfree(realpath);
                kfree(dev_realpath);
                return -EPERM;
            }
        }

        kfree(realpath);
        kfree(dev_realpath);
        return 0;
    }
#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
    else if (result == MATCH_SECURE_STORAGE_MOUNT_POINT) {
        kfree(realpath);
        kfree(dev_realpath);
        return 0;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */
#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING
    else if (result == MATCH_KITTING_MOUNT_POINT) {
        kfree(realpath);
        kfree(dev_realpath);
        return 0;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */
    else if (result == MATCH_MOUNT_POINT || result == UNRELATED_MOUNT_POINT) {
        kfree(realpath);
        kfree(dev_realpath);
        return 0;
    }

    printk(KERN_INFO "%s:%d: REJECT MOUNT dev_realpath=%s realpath=%s process name=%s, uid=%d, pid=%d\n",
        __FUNCTION__, __LINE__, dev_realpath, realpath, current->comm, current->cred->uid, current->pid);
    kfree(realpath);
    kfree(dev_realpath);
    return -EPERM;
}

static int fjsec_sb_umount(struct vfsmount *mnt, int flags)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    struct path path = { mnt, mnt->mnt_root };
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %d):%d:", __FUNCTION__, mnt, flags, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(&path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    dprintk(KERN_INFO "%s:%d:(%s).\n", __FUNCTION__, __LINE__, realpath);

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */
#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */

    kfree(realpath);
    return 0;
}

static int fjsec_sb_pivotroot(struct path *old_path, struct path *new_path)
{
    char *old_realpath;
    char *new_realpath;
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p):%d:", __FUNCTION__, old_path, new_path, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    old_realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!old_realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    new_realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!new_realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(old_realpath);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(old_path, old_realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath from old_path. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(old_realpath);
        kfree(new_realpath);
        return r;
    }

    r = _xx_realpath_from_path(new_path, new_realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath from new_path. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(old_realpath);
        kfree(new_realpath);
        return r;
    }

    printk(KERN_INFO "%s:%d: REJECT old_path=%s new_path=%s process name=%s, uid=%d, pid=%d\n",
        __FUNCTION__, __LINE__, old_realpath, new_realpath, current->comm, current->cred->uid, current->pid);
    kfree(old_realpath);
    kfree(new_realpath);
    return -EPERM;
}

#ifdef CONFIG_SECURITY_FJSEC_PROTECT_CHROOT
static int fjsec_path_chroot(struct path *path)
{
    char *realpath;
    char *tmp;
    char *p, *p2;
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p):%d:", __FUNCTION__, path, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    tmp = kzalloc(PATH_MAX, GFP_NOFS);
    if (!tmp) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        kfree(tmp);
        return r;
    }

    p = CONFIG_SECURITY_FJSEC_CHROOT_PATH;
    while (*p) {
        p2 = strchr(p, ':');
        if (p2) {
            strncpy(tmp, p, (p2 - p));
            tmp[p2 - p] = 0;
        }
        else {
            strcpy(tmp, p);
        }

        if (strcmp(tmp, realpath) == 0) {
            kfree(realpath);
            kfree(tmp);
            return 0;
        }

        if (p2) {
            p = p2 + 1;
        }
        else {
            p += strlen(p);
        }
    }

    printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
    kfree(realpath);
    kfree(tmp);
    return -EPERM;
}
#endif    /* CONFIG_SECURITY_FJSEC_PROTECT_CHROOT */

static int fjsec_file_permission(struct file *file, int mask, loff_t offset, loff_t length)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
//    dprintk(KERN_INFO "%s(%p, %d, %lld, %lld):%d:", __FUNCTION__, file, mask, offset, length, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(&file->f_path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    r = fjsec_read_write_access_control(realpath);
    if (r == -EPERM) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
    else if (r == 0) {
        kfree(realpath);
        return 0;
    }

    if (fjsec_check_secure_storage_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_kitting_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    /* read mode -> OK! */
    if ((mask & MAY_WRITE) == 0) {
        kfree(realpath);
        return 0;
    }

    /* write mode -> check... */
    if (fjsec_check_disk_device_access(realpath, offset, length) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s offset=0x%llx length=0x%llx process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, offset, length, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_system_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_device_access_process(realpath, fs_acl) != 0) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    kfree(realpath);
    return 0;
}

int fjsec_file_mmap(struct file *file, unsigned long reqprot,
            unsigned long prot, unsigned long flags,
            unsigned long addr, unsigned long addr_only)
{
    char *realpath;
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
//    dprintk(KERN_INFO "%s(%p, %lu, %lu, %lu, %lu, %lu):%d:", __FUNCTION__,
//            file, reqprot, prot, flags, addr, addr_only, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    r = cap_file_mmap(file, reqprot, prot, flags, addr, addr_only);
    if (r) {
        printk(KERN_INFO "%s:%d: REJECT mmap. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -EPERM;
    }

    realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    if (!file) {
        dprintk(KERN_INFO "%s:%d: file is null. process name =<%s>\n", __FUNCTION__, __LINE__, current->comm);
        kfree(realpath);
        return 0;
    }

    /* read only mode -> OK! */
    if ((prot & PROT_WRITE) == 0) {
        kfree(realpath);
        return 0;
    }

    dprintk(KERN_INFO "%s:%d: prot=<%ld>\n", __FUNCTION__, __LINE__, prot);

    r = _xx_realpath_from_path(&file->f_path, realpath, PATH_MAX-1);
    if (r != 0) {
        if (file->f_path.mnt->mnt_flags == 0) {
            kfree(realpath);
            return 0;
        } else {
            printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return r;
        }
    }

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_DISK_DEV_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    kfree(realpath);
    return 0;
}

static int fjsec_dentry_open(struct file *file, const struct cred *cred)
{
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
//    dprintk(KERN_INFO "%s(%p, %p):%d:", __FUNCTION__, file, cred, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(&file->f_path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    r = fjsec_read_write_access_control(realpath);
    if (r == -EPERM) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s, process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
    else if (r == 0) {
        kfree(realpath);
        return 0;
    }

    if (fjsec_check_secure_storage_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_secure_storage_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_kitting_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_kitting_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    /* read only mode -> OK! */
    if ((file->f_mode & FMODE_WRITE) == 0) {
        kfree(realpath);
        return 0;
    }

    if (fjsec_check_system_directory_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_device_access_process(realpath, fs_acl) != 0) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_system_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_device_access_process(realpath, ptn_acl) != 0) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    kfree(realpath);
    return 0;
}

#ifdef CONFIG_SECURITY_PATH
static int fjsec_path_mknod(struct path *path, struct dentry *dentry, int mode,
                unsigned int dev)
{
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p, %d, %ud):%d:", __FUNCTION__, path, dentry, mode, dev, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    r = fjsec_check_mknod(path, dentry, mode, dev);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    return 0;
}

static int fjsec_path_mkdir(struct path *path, struct dentry *dentry, int mode)
{
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p, %d):%d:", __FUNCTION__, path, dentry, mode, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    return 0;
}

static int fjsec_path_rmdir(struct path *path, struct dentry *dentry)
{
    int r;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p):%d:", __FUNCTION__, path, dentry, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    return 0;
}

static int fjsec_path_unlink(struct path *path, struct dentry *dentry)
{
#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    char *realpath;
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
    int r;
    
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p):%d:", __FUNCTION__, path, dentry, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }
    
    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    if (PATH_MAX > strlen(realpath) + strlen(dentry->d_name.name)) {
        strcat(realpath, dentry->d_name.name);
    } else {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
               __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -ENOMEM;
    }

    dprintk(KERN_INFO "(%s:%d) <%s> <%s>\n", __FUNCTION__, __LINE__, realpath, dentry->d_name.name);

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH) == 0 || 
        strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    kfree(realpath);
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
    return 0;
}

static int fjsec_path_symlink(struct path *path, struct dentry *dentry,
                  const char *old_name)
{
#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    char *realpath;
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
    int r;

/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p, %s):%d:", __FUNCTION__, path, dentry, old_name, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    realpath = kzalloc(PATH_MAX, GFP_NOFS);
    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = _xx_realpath_from_path(path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    if (PATH_MAX > strlen(realpath) + strlen(dentry->d_name.name)) {
        strcat(realpath, dentry->d_name.name);
    } else {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
               __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -ENOMEM;
    }

    dprintk(KERN_INFO "(%s:%d) <%s> <%s> <%s>\n", __FUNCTION__, __LINE__, realpath, dentry->d_name.name, old_name);

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH) == 0) {
        if (strcmp(old_name, CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH) != 0) {
            printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
                 __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
            kfree(realpath);
            return -EPERM;
        }
    }

    kfree(realpath);
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
    return 0;
}

static int fjsec_path_link(struct dentry *old_dentry, struct path *new_dir,
               struct dentry *new_dentry)
{
    int r;
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    struct path old_path = {new_dir->mnt, old_dentry};
#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    struct path new_path = {new_dir->mnt, new_dentry};
#endif 

/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p, %p):%d:", __FUNCTION__, old_dentry, new_dir, new_dentry, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = fjsec_check_path_from_path(new_dir);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        kfree(realpath);
        return r;
    }

    r = _xx_realpath_from_path(&old_path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    dprintk(KERN_INFO "(%s:%d) <%s>\n", __FUNCTION__, __LINE__, realpath);

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_DISK_DEV_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_system_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_secure_storage_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_kitting_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_device_access_process(realpath, ptn_acl) != 0) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    r = _xx_realpath_from_path(&new_path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    dprintk(KERN_INFO "(%s:%d) <%s>\n", __FUNCTION__, __LINE__, realpath);

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH) == 0 ||
        strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */

    kfree(realpath);
    return 0;
}

static int fjsec_path_rename(struct path *old_dir, struct dentry *old_dentry,
                 struct path *new_dir, struct dentry *new_dentry)
{
    int r;
    char *realpath = kzalloc(PATH_MAX, GFP_NOFS);
    struct path old_path = {old_dir->mnt, old_dentry};
#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    struct path new_path = {new_dir->mnt, new_dentry};
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */

/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p, %p, %p):%d:", __FUNCTION__, old_dir, old_dentry, new_dir, new_dentry, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    if (!realpath) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    r = fjsec_check_path_from_path(new_dir);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        kfree(realpath);
        return r;
    }

    r = _xx_realpath_from_path(&old_path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    dprintk(KERN_INFO "(%s:%d) <%s> <%s>\n", __FUNCTION__, __LINE__, realpath, old_dentry->d_name.name);

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_DISK_DEV_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_system_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_secure_storage_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_kitting_device_access_process(realpath) != 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    if (fjsec_check_device_access_process(realpath, ptn_acl) != 0) {
        dump_prinfo(__FUNCTION__, __LINE__);
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
            __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH) == 0 || 
        strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }

    r = _xx_realpath_from_path(&new_path, realpath, PATH_MAX-1);
    if (r != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating realpath. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return r;
    }

    if (strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH) == 0 || 
        strcmp(realpath, CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH) == 0) {
        printk(KERN_INFO "%s:%d: REJECT realpath=%s process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, realpath, current->comm, current->cred->uid, current->pid);
        kfree(realpath);
        return -EPERM;
    }
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */

    kfree(realpath);
    return 0;
}

static int fjsec_path_truncate(struct path *path)
{
    int r;

/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p):%d:", __FUNCTION__, path, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    return 0;
}

static int fjsec_path_chmod(struct dentry *dentry, struct vfsmount *mnt,
                mode_t mode)
{
    struct path path = { mnt, dentry };
    int r;

/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %p, %d):%d:", __FUNCTION__, dentry, mnt, mode, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    r = fjsec_check_path_from_path(&path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    r = fjsec_check_chmod_from_path(&path, mode);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    return 0;
}

static int fjsec_path_chown(struct path *path, uid_t uid, gid_t gid)
{
    int r;

/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%p, %d, %d):%d:", __FUNCTION__, path, uid, gid,  __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */

    r = fjsec_check_path_from_path(path);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    r = fjsec_check_chown_from_path(path, uid, gid);
    if (r) {
        printk(KERN_INFO "%s:%d: r=%d\n", __FUNCTION__, __LINE__, r);
        return r;
    }

    return 0;
}
#endif    /* CONFIG_SECURITY_PATH */

/* fjsec_vmalloc_to_sg() is ported from drivers/media/video/videobuf-dma-sg.c
 *
 * Return a scatterlist for some vmalloc()'ed memory
 * block (NULL on errors).  Memory for the scatterlist is allocated
 * using kmalloc.  The caller must free the memory.
 */
static struct scatterlist *fjsec_vmalloc_to_sg(void *buf, unsigned long buflen)
{
    struct scatterlist *sglist;
    struct page *pg;
    int index;
    int pages;
    int size;

    pages = PAGE_ALIGN(buflen) >> PAGE_SHIFT;

    sglist = kmalloc((pages * sizeof(*sglist)), GFP_NOFS);
    if (sglist == NULL) {
        printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    sg_init_table(sglist, pages);
    for (index = 0; index < pages; index++, buf += PAGE_SIZE) {
        pg = vmalloc_to_page(buf);
        if (pg == NULL) {
            printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
            kfree(sglist);
            return NULL;
        }

        size = buflen - (index * PAGE_SIZE);
        if (size > PAGE_SIZE) {
            size = PAGE_SIZE;
        }

        sg_set_page(&sglist[index], pg, size, 0);
    }
    return sglist;
}

static char *get_checksum(void *buf, unsigned long buflen)
{
    int ret;
    char *output;
    struct crypto_hash *tfm;
    struct scatterlist *sg;
    struct hash_desc desc;

    tfm = crypto_alloc_hash("sha256", 0, CRYPTO_ALG_ASYNC);
    if (IS_ERR(tfm)) {
         printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
         return NULL;
    }

    output = kmalloc(crypto_hash_digestsize(tfm), GFP_NOFS);
    if (output == NULL) {
        crypto_free_hash(tfm);
        printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    desc.tfm = tfm;
    ret = crypto_hash_init(&desc);
    if (ret != 0) {
        kfree(output);
        crypto_free_hash(tfm);
        printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    sg = fjsec_vmalloc_to_sg(buf, buflen);
    if (sg == NULL) {
        kfree(output);
        crypto_free_hash(tfm);
        printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    ret = crypto_hash_digest(&desc, sg, buflen, output);
    if (ret != 0) {
        kfree(sg);
        kfree(output);
        crypto_free_hash(tfm);
        printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    kfree(sg);
    crypto_free_hash(tfm);
    return output;
}

static int fjsec_kernel_load_module(char *name, void __user *umod, unsigned long len)
{
    struct module_list *p;
    void *kmod;
    char *checksum = NULL;
    bool match_modname = false;
/* ADD start FUJITSU TEN :2013-04-08 LSM */
    ada_dprintk(KERN_INFO "%s(%s, %p, %lu):%d:", __FUNCTION__, name, umod, len, __LINE__);
/* ADD end FUJITSU TEN :2013-04-08 LSM */
    if (!name) {
        printk(KERN_INFO "%s:%d: REJECT name=NULL process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -EINVAL;
    }

    dprintk(KERN_INFO "%s:%d: name=%s\n", __FUNCTION__, __LINE__, name);

    for (p = modlist; p->name; p++) {
        if (strcmp(name, p->name) == 0) {
            match_modname = true;
            break;
        }
    }

    if (match_modname == false) {
        printk(KERN_INFO "%s:%d: REJECT Mismatched name=<%s> process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, name, current->comm, current->cred->uid, current->pid);
        return -EPERM;
    }

    kmod = vmalloc(len);
    if (!kmod) {
        printk(KERN_INFO "%s:%d: REJECT Failed allocating buffer. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        return -ENOMEM;
    }

    if (copy_from_user(kmod, umod, len) != 0) {
        printk(KERN_INFO "%s:%d: REJECT Failed copying from user. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        vfree(kmod);
        return -EFAULT;
    }

    checksum = get_checksum(kmod, len);
    if (checksum == NULL) {
        printk(KERN_INFO "%s:%d: REJECT Failed creating checksum. process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, current->comm, current->cred->uid, current->pid);
        vfree(kmod);
        return -EPERM;
    }

    if (memcmp((const void *)checksum, (const void *)p->checksum, SHA256_DIGEST_SIZE) != 0) {
        printk(KERN_INFO "%s:%d: REJECT Mismatched checksum. name=<%s> process name=%s, uid=%d, pid=%d\n",
             __FUNCTION__, __LINE__, name, current->comm, current->cred->uid, current->pid);
        print_hex_dump(KERN_INFO, "checksum: ", DUMP_PREFIX_OFFSET, 16, 1, checksum, SHA256_DIGEST_SIZE, false);
        vfree(kmod);
        kfree(checksum);
        return -EPERM;
    }

    vfree(kmod);
    kfree(checksum);
    return 0;
}

struct security_operations fjsec_security_ops = {
    .name =                    "fjsec",
    .ptrace_traceme =        fjsec_ptrace_traceme,
    .ptrace_request_check =    fjsec_ptrace_request_check,
    .sb_mount =            fjsec_sb_mount,
    .sb_pivotroot =            fjsec_sb_pivotroot,
#ifdef CONFIG_SECURITY_PATH
#ifdef CONFIG_SECURITY_FJSEC_PROTECT_CHROOT
    .path_chroot =            fjsec_path_chroot,
#endif    /* CONFIG_SECURITY_FJSEC_PROTECT_CHROOT */
#endif    /*  CONFIG_SECURITY_PATH */
    .sb_umount =            fjsec_sb_umount,
    .file_permission =        fjsec_file_permission,
    .file_mmap =            fjsec_file_mmap,
    .dentry_open =            fjsec_dentry_open,
#ifdef CONFIG_SECURITY_PATH
    .path_mknod =            fjsec_path_mknod,
    .path_mkdir =            fjsec_path_mkdir,
    .path_rmdir =            fjsec_path_rmdir,
    .path_unlink =            fjsec_path_unlink,
    .path_symlink =            fjsec_path_symlink,
    .path_link =            fjsec_path_link,
    .path_rename =            fjsec_path_rename,
    .path_truncate =        fjsec_path_truncate,
    .path_chmod =            fjsec_path_chmod,
    .path_chown =            fjsec_path_chown,
#endif    /* CONFIG_SECURITY_PATH */
    .kernel_load_module =        fjsec_kernel_load_module,
};

#ifdef PRCONFIG

static void fjsec_prconfig(void)
{
    struct module_list *p;
    struct fs_path_config *pc;
    struct accessible_area_disk_dev *accessible_areas;
    struct access_control_mmcdl_device *mmcdl_device;
    int index;
    struct read_write_access_control *rw_access_control;
    struct read_write_access_control_process *rw_access_control_process;
    struct ac_config *ac_cfg;

    printk (KERN_INFO " --------------------\n");
    printk (KERN_INFO "boot mode=<%d>\n", boot_mode);

    printk (KERN_INFO "CONFIG_SECURITY_PATH=<%d>\n", CONFIG_SECURITY_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_DISK_DEV_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_DISK_DEV_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_SYSTEM_DIR_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH);
/* FUJITSU TEN:2013-09-04 LSM mod start */
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH2=<%s>\n",
                CONFIG_SECURITY_FJSEC_SYSTEM_DEV_PATH2);
/* FUJITSU TEN:2013-09-04 LSM mod end */
#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE=<%d>\n", CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DIR_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DEV_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_SECURE_STORAGE_DEV_PATH);
#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */
#ifdef CONFIG_SECURITY_FJSEC_AC_KITTING
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_AC_KITTING=<%d>\n", CONFIG_SECURITY_FJSEC_AC_KITTING);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_KITTING_DIR_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_KITTING_DEV_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_KITTING_DEV_PATH);
#endif /* CONFIG_SECURITY_FJSEC_AC_KITTING */
#ifdef CONFIG_SECURITY_FJSEC_AC_FELICA
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_AC_FELICA=<%d>\n", CONFIG_SECURITY_FJSEC_AC_FELICA);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_FELICA_DEV_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_FELICA_SYMLINK_PATH);
#endif /* CONFIG_SECURITY_FJSEC_AC_FELICA */
#ifdef CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SECURE_STORAGE_ACCESS_PROCESS_NAME=<%s>\n", CONFIG_SECURITY_FJSEC_SECURE_STORAGE_ACCESS_PROCESS_NAME);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SECURE_STORAGE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_SECURE_STORAGE_ACCESS_PROCESS_PATH);
#endif /* CONFIG_SECURITY_FJSEC_AC_SECURE_STORAGE */
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME=<%s>\n", CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_NAME);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_FOTA_MODE_ACCESS_PROCESS_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME=<%s>\n", CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_NAME);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_SDDOWNLOADER_MODE_ACCESS_PROCESS_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME=<%s>\n", CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_NAME);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_RECOVERY_MODE_ACCESS_PROCESS_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_NAME=<%s>\n", CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_NAME);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_MAKERCMD_MODE_ACCESS_PROCESS_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME=<%s>\n", CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_NAME);
/* MOD start FUJITSU TEN :2013-04-12 LSM */
//    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH1);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_OSUPDATE_MODE_ACCESS_PROCESS_PATH2);
/* MOD end FUJITSU TEN :2013-04-12 LSM */
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_PROTECT_CHROOT=<%d>\n", CONFIG_SECURITY_FJSEC_PROTECT_CHROOT);
    printk (KERN_INFO "CONFIG_SECURITY_FJSEC_CHROOT_PATH=<%s>\n", CONFIG_SECURITY_FJSEC_CHROOT_PATH);

    printk (KERN_INFO " devs\n");
    for (pc = devs; pc->prefix; pc++) {
        printk (KERN_INFO "  <0x%08x> <%d> <%d> <0x%08x> <%s>\n", pc->mode, pc->uid, pc->gid, pc->rdev, pc->prefix);
    }

    printk (KERN_INFO " modlist\n");
    for (p = modlist; p->name; p++) {
        printk (KERN_INFO "  <%s>\n", p->name);
    }

    printk (KERN_INFO " accessible area\n");
    printk (KERN_INFO "  fota mode\n");
    for (accessible_areas = accessible_areas_fota; accessible_areas->tail; accessible_areas++) {
        printk (KERN_INFO "   <0x%llx><0x%llx>\n", accessible_areas->head, accessible_areas->tail);
    }

    printk (KERN_INFO "  sddownloader mode\n");
    for (accessible_areas = accessible_areas_sddownloader; accessible_areas->tail; accessible_areas++) {
        printk (KERN_INFO "   <0x%llx><0x%llx>\n", accessible_areas->head, accessible_areas->tail);
    }

    printk (KERN_INFO "  osupdate mode\n");
    for (accessible_areas = accessible_areas_osupdate; accessible_areas->tail; accessible_areas++) {
        printk (KERN_INFO "   <0x%llx><0x%llx>\n", accessible_areas->head, accessible_areas->tail);
    }

    printk (KERN_INFO "  recovery mode\n");
    for (accessible_areas = accessible_areas_recovery; accessible_areas->tail; accessible_areas++) {
        printk (KERN_INFO "   <0x%llx><0x%llx>\n", accessible_areas->head, accessible_areas->tail);
    }

    printk (KERN_INFO "  maker mode\n");
    for (accessible_areas = accessible_areas_maker; accessible_areas->tail; accessible_areas++) {
        printk (KERN_INFO "   <0x%llx><0x%llx>\n", accessible_areas->head, accessible_areas->tail);
    }

    printk (KERN_INFO " mmcdl_device_list\n");
    for (mmcdl_device = mmcdl_device_list; mmcdl_device->process_name; mmcdl_device++) {
        printk (KERN_INFO "  <%d><%s><%s>\n", mmcdl_device->boot_mode, mmcdl_device->process_name, mmcdl_device->process_path);
    }

    printk(KERN_INFO " ptrace_read_request_policy\n");
    for (index = 0; index < ARRAY_SIZE(ptrace_read_request_policy); index++) {
        printk(KERN_INFO "  <%ld>\n", ptrace_read_request_policy[index]);
    }

    printk (KERN_INFO " rw_access_control_list\n");
    for (rw_access_control = rw_access_control_list; rw_access_control->prefix; rw_access_control++) {
        for (rw_access_control_process = rw_access_control->process_list; rw_access_control_process->process_path; rw_access_control_process++) {
            printk (KERN_INFO "  <%s><%s><%s><%d>\n", rw_access_control->prefix, rw_access_control_process->process_name, rw_access_control_process->process_path, rw_access_control_process->uid);
        }
    }

    printk (KERN_INFO " ptn_acl\n");
    for (ac_cfg = ptn_acl; ac_cfg->prefix; ac_cfg++) {
        printk (KERN_INFO "  <%s><%d><%s><%s>\n", ac_cfg->prefix, ac_cfg->boot_mode, ac_cfg->process_name, ac_cfg->process_path);
    }

    printk (KERN_INFO " fs_acl\n");
    for (ac_cfg = fs_acl; ac_cfg->prefix; ac_cfg++) {
        printk (KERN_INFO "  <%s><%d><%s><%s>\n", ac_cfg->prefix, ac_cfg->boot_mode, ac_cfg->process_name, ac_cfg->process_path);
    }

    printk (KERN_INFO " devmem_acl\n");
    for (index = 0; index < ARRAY_SIZE(devmem_acl); index++) {
        printk (KERN_INFO "  <0x%lx><0x%lx><%s><%s>\n", devmem_acl[index].head, devmem_acl[index].tail, devmem_acl[index].process_name, devmem_acl[index].process_path);
    }

    printk (KERN_INFO " --------------------\n");
}
#else
#define fjsec_prconfig()
#endif

static int __init fjsec_init (void)
{
    printk (KERN_INFO "FJSEC LSM module initialized\n");
    fjsec_prconfig();

    return 0;
}

security_initcall (fjsec_init);
