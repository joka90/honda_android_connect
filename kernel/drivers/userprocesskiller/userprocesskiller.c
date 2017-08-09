/**
 * @file userprocesskiller.c
 *
 * userprocesskiller driver for Android
 *
 * Copyright(C) 2012,2013 FUJITSU LIMITED
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
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*----------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/cred.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/lites_trace.h>

static int user_process_killer_pf_remove(struct platform_device* pfdev);
static int user_process_killer_pf_probe(struct platform_device* pfdev);
static int user_process_killer_pf_suspend(struct platform_device* pfdev, pm_message_t state );
static int user_process_killer_pf_resume(struct platform_device* pfdev);
static void force_sig_descendants(int signal, struct task_struct* task);

#define USERPROCESSKILLER_MODULE_NAME "userprocesskiller"
#define USERPROCESSKILLER_DEVICE_NAME "userprocesskiller"

#ifdef UPK_DEBUG
#define DEBUG_PRINT(...) printk(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#define UPK_DRIVER_MODULE_ID (LITES_KNL_LAYER+100)
#define FORMAT_ID 2

#define DRVREC_TRACE( level_, trace_id_, errcode_, ... ) {\
	struct lites_trace_format data = { 0 }; \
	struct lites_trace_header header = { 0 }; \
	enum LOG_LEVEL level = level_; \
	char buf[36] = { 0 };\
	int ierr;\
    char *tmp;\
\
	header.level = level; \
	header.trace_no = UPK_DRIVER_MODULE_ID; \
	header.trace_id = trace_id_; \
	header.format_id = FORMAT_ID; \
	data.trc_header = &header; \
\
	ierr = errcode_; \
    tmp = kmalloc(512, GFP_TEMPORARY);\
    if(tmp) { \
        snprintf(tmp, 512, __VA_ARGS__);\
        if(strlen(tmp) > (36-sizeof(ierr))) {\
            memcpy(buf, tmp+(strlen(tmp)-(36-sizeof(ierr))), 36-sizeof(ierr)); \
        }\
        else {\
            strcpy(buf, tmp); \
        }\
        kfree(tmp);\
    }\
    else {\
        snprintf(buf, 36-sizeof(ierr), __VA_ARGS__);\
    }\
    memcpy(buf+(36-sizeof(ierr)), &ierr, sizeof(ierr)); \
	data.buf = buf; \
	data.count = 36; \
\
	lites_trace_write( REGION_TRACE_DRIVER, &data ); \
}

#define DRVREC_ERROR(trace_id_, errcode_, ... ) DRVREC_TRACE( ERROR, trace_id_, errcode_, __VA_ARGS__)
#define DRVREC_INFO(trace_id_, errcode_, ... )  DRVREC_TRACE(  INFO, trace_id_, errcode_, __VA_ARGS__)

enum {
	TRACE_ADD_WHITELIST,
	TRACE_DEL_WHITELIST,
	TRACE_CLR_WHITELIST,
	TRACE_ZYGOTE_NOT_FOUND,
	TRACE_MEMORY_ALLOCATE_FAILED,
	TRACE_FORCE_SIG_RECURSIVE,
	TRACE_FORCE_SIG_RECURSIVE_TOO_DEEP,
	TRACE_FORCE_SIG_DESCENDANTS,
	TRACE_DRIVER_CALL_KILL_TASKS,
	TRACE_DRIVER_INITIALIZE_ERROR,
	TRACE_DRIVER_INITIALIZED,
	TRACE_DRIVER_PROVE_ERROR,
	TRACE_DRIVER_PROVED,
};

static const char ZYGOTE_COMM_NAME[] = "zygote";

/* Process list structure */
struct process_list {
	char *processname;
	struct list_head list;
};

/* Mutex related */
static DEFINE_MUTEX( user_process_killerk_whitelist_mutex );
static LIST_HEAD( user_process_killerk_whitelist_head );

static int add_whitelist(const char *filename )
{
	struct process_list *plptr;
	int err = 0 ;

	DEBUG_PRINT("add_whitelist: %s\n", filename);
	list_for_each_entry(plptr, &user_process_killerk_whitelist_head, list) {
		if(strcmp(filename, plptr->processname) == 0) {
			return 0;
		}
	}

	mutex_lock( &user_process_killerk_whitelist_mutex );
	plptr = (struct process_list *)kmalloc(sizeof(struct process_list), GFP_KERNEL);
	if( plptr != NULL ){
		plptr->processname = (char*)kmalloc(strlen(filename)+1, GFP_KERNEL);
		if( plptr->processname != NULL ){
			strcpy(plptr->processname, filename);
			list_add_tail( &plptr->list , &user_process_killerk_whitelist_head );
			DEBUG_PRINT("userprocesskiller: Whitelisting %s.\n", plptr->processname);
			DRVREC_INFO(TRACE_ADD_WHITELIST, 0, "%s", plptr->processname);
		}else{
			kfree(plptr);
			err = -ENOMEM;
		}
	}else{
		err = -ENOMEM;
	}

	mutex_unlock( &user_process_killerk_whitelist_mutex );

	DEBUG_PRINT("exit add_whitelist: %d\n", err);
	return err;
}

static void clear_whitelist( void )
{
	DEBUG_PRINT("clear_whitelist:\n" );
	
	mutex_lock( &user_process_killerk_whitelist_mutex );

	while(!list_empty(&user_process_killerk_whitelist_head)) {
		struct process_list* p = list_entry(user_process_killerk_whitelist_head.next, struct process_list, list);
		DEBUG_PRINT("userprocesskiller: Clear %s from whitelist.\n", p->processname);
		DRVREC_INFO(TRACE_CLR_WHITELIST, 0, "%s", p->processname);
		list_del(&p->list);
		kfree(p->processname);
		kfree(p);
	}

	mutex_unlock( &user_process_killerk_whitelist_mutex );

	DEBUG_PRINT("exit clear_whitelist:\n" );
}

static bool is_whitelisted_process(const char* procname)
{
	struct process_list *plptr;
	list_for_each_entry(plptr, &user_process_killerk_whitelist_head, list) {
		if(strcmp(procname, plptr->processname) == 0) {
			DEBUG_PRINT("is_whitelisted_process: already registered %s\n", procname);
			return true;
		}
	}

	DEBUG_PRINT("is_whitelisted_process: missing %s\n", procname);
	return false;
}

static int get_zygote_pid(void)
{
	struct task_struct *p;
	int pid = -EINVAL;
	int cmp = 0;

	/* 全てのプロセスに対して */
	for_each_process(p) {
		task_lock(p);
		cmp = strcmp(ZYGOTE_COMM_NAME, p->comm);
		pid = p->pid;
		task_unlock(p);

		if(cmp == 0) {
			DEBUG_PRINT("zygote pid=%d\n", pid);
			return pid;
		}
	}

	printk(KERN_NOTICE "get_zygote_pid: zygote process not found.\n");
	DRVREC_ERROR(TRACE_ZYGOTE_NOT_FOUND, 0, "TRACE_ZYGOTE_NOT_FOUND");
	return -EINVAL;
}


/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      find_lock_task_mm                                                    */
/*  [DESCRIPTION]                                                            */
/*      与えられたプロセス内のスレッドのうち、mmが有効なもの情報を得る       */
/*  [TYPE]                                                                   */
/*      struct task_struct *                                                 */
/*  [PARAMETER]                                                              */
/*      struct task_struct *p : プロセスのtask_struct構造体                  */
/*  [RETURN]                                                                 */
/*      取得したスレッドのtask_struct                                        */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
/*
 * The process p may have detached its own ->mm while exiting or through
 * use_mm(), but one or more of its subthreads may still have a valid
 * pointer.  Return p, or any of its subthreads with a valid ->mm, with
 * task_lock() held.
 */
static struct task_struct *find_lock_task_mm(struct task_struct *p)
{
	struct task_struct *t = p;

	do {
		task_lock(t);
		if(likely(t->mm))
			return t;
		task_unlock(t);
	} while_each_thread(p, t);

	return NULL;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_unkillable_task                                  */
/*  [DESCRIPTION]                                                            */
/*      kill対象のタスクか判定する                                           */
/*  [TYPE]                                                                   */
/*      bool                                                                 */
/*  [PARAMETER]                                                              */
/*      struct task_struct *p:  判定対象のtask_struct構造体                  */
/*  [RETURN]                                                                 */
/*      true:  kill対象                                                      */
/*      false: 非kill対象                                                    */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static bool user_process_killer_unkillable_task( struct task_struct *p )
{
	const struct cred *tcred;
	unsigned int uid,gid;
	
	int zygote_pid = 0;
	char* cmdline = NULL;

	/* is init process(pid=1), this process is unkillable */
	if( is_global_init(p) ){
		DEBUG_PRINT("user_process_killer_unkillable_task: init process %d\n", p->pid);
		return true;
	}

	/* kernel thread process is unkillable */
	if( p->flags & PF_KTHREAD ){
		DEBUG_PRINT("user_process_killer_unkillable_task: kernel thread %d\n", p->pid);
		return true;
	}

	/* kill only zygote children */
	zygote_pid = get_zygote_pid();
	if (!p->real_parent || p->real_parent->pid != zygote_pid ) {
		DEBUG_PRINT("user_process_killer_unkillable_task: not zygote child %d\n", p->pid);
		return true;
	}

	/* allocate page */
	cmdline = (char*)get_zeroed_page(GFP_TEMPORARY);
	if(!cmdline) {
		printk(KERN_ERR "user_process_killer_unkillable_task: Can't allocate page.\n");
		DRVREC_ERROR(TRACE_MEMORY_ALLOCATE_FAILED, 0, "TRACE_MEMORY_ALLOCATE_FAILED");
		return true;
	}

	/* white-listed process is unkillable */
	proc_pid_cmdline(p, cmdline);
	DEBUG_PRINT("cmdline %d %s\n", p->pid, cmdline);
	if ( is_whitelisted_process(cmdline) ) {
		free_page((unsigned long)cmdline);
		return true;
	}

	/* read task's uid and gid */
	rcu_read_lock();
	tcred = __task_cred(p);
	uid = tcred->uid;
	gid = tcred->gid;
	rcu_read_unlock();

	DEBUG_PRINT("user_process_killer_unkillable_task: process %d (%s), uid:%d, gid:%d \n",
		task_pid_nr(p), cmdline,uid,gid);

	free_page((unsigned long)cmdline);

	return false;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_kill_task                                        */
/*  [DESCRIPTION]                                                            */
/*      タスクにkill signalを送る                                            */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct task_struct *p:  判定対象のtask_struct構造体                  */
/*  [RETURN]                                                                 */
/*      1: kill失敗                                                          */
/*      0: kill成功                                                          */
/*  [NOTE]                                                                   */
/*      対象タスクと同じメモリを共有しているプロセスも同時にkillする         */
/*      (OOM_KILLERと同じ動作)                                               */
/*****************************************************************************/
#define K(x) ((x) << (PAGE_SHIFT-10))
static int user_process_killer_kill_task( struct task_struct *p )
{
	struct task_struct *q;
	struct mm_struct *mm;

	p = find_lock_task_mm(p);
	if (!p)
		return 1;

	/* mm cannot be safely dereferenced after task_unlock(p) */
	mm = p->mm;

// Comment out from OOM original.
//	pr_err("Killed process %d (%s) total-vm:%lukB, anon-rss:%lukB, file-rss:%lukB\n",
//		task_pid_nr(p), p->comm, K(p->mm->total_vm),
//		K(get_mm_counter(p->mm, MM_ANONPAGES)),
//		K(get_mm_counter(p->mm, MM_FILEPAGES)));
	task_unlock(p);

	/*
	 * Kill all processes sharing p->mm in other thread groups, if any.
	 * They don't get access to memory reserves or a higher scheduler
	 * priority, though, to avoid depletion of all memory or task
	 * starvation.  This prevents mm->mmap_sem livelock when an oom killed
	 * task cannot exit because it requires the semaphore and its contended
	 * by another thread trying to allocate memory itself.  That thread will
	 * now get access to memory reserves since it has a pending fatal
	 * signal.
	 */
	for_each_process(q)
		if (q->mm == mm && !same_thread_group(q, p)) {
			task_lock(q);	/* Protect ->comm from prctl() */
			pr_err("Kill process %d (%s) sharing same memory\n",
				task_pid_nr(q), q->comm);
			task_unlock(q);
			force_sig(SIGKILL, q);
		}

	set_tsk_thread_flag(p, TIF_MEMDIE);
	force_sig_descendants(SIGKILL, p); // Change from OOM original.

	return 0;
}
#undef K

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_kill_process                                     */
/*  [DESCRIPTION]                                                            */
/*      プロセスにkill signalを送る                                          */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct task_struct *p:  判定対象のtask_struct構造体                  */
/*  [RETURN]                                                                 */
/*      1: kill失敗                                                          */
/*      0: kill成功                                                          */
/*  [NOTE]                                                                   */
/*      対象プロセスと同じメモリを共有しているプロセスも同時にkillする       */
/*      (OOM_KILLERと同じ動作)                                               */
/*****************************************************************************/
static int user_process_killer_kill_process(struct task_struct *p)
{
	struct task_struct *victim = p;

	/*
	 * If the task is already exiting, don't alarm the sysadmin or kill
	 * its children or threads, just set TIF_MEMDIE so it can die quickly
	 */
	if (p->flags & PF_EXITING) {
		/* 既に終了中の場合はTIF_MEMDIEを設定するのみ */
		set_tsk_thread_flag(p, TIF_MEMDIE);
		return 0;
	}

	return user_process_killer_kill_task(victim);
}

struct ps_entry {
	struct task_struct* task;
};

struct ps_list {
	struct ps_entry* list;
	int count;
};

static int recursive_depth = 0;

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      force_sig_recursive                                                  */
/*  [DESCRIPTION]                                                            */
/*      再帰的にプロセスとその子プロセスに対してシグナルを発行する           */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      struct ps_list* プロセスのリスト                                     */
/*      int signal シグナル種別                                              */
/*      struct task_struct* task シグナル発行対象のプロセス構造体            */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*****************************************************************************/
static void force_sig_recursive(struct ps_list* pslist, int signal, struct task_struct* killtask) {
	int i=0;
	DEBUG_PRINT("force_sig_recursive: %d\n", recursive_depth);
	if(recursive_depth > 10) {
		//paranoire check for infinity loop.
		printk(KERN_ERR "Too deep recursive detect. %s", __func__);
		DRVREC_INFO(TRACE_FORCE_SIG_RECURSIVE_TOO_DEEP, killtask->pid, "TRACE_FORCE_SIG_RECURSIVE_TOO_DEEP");
		return;
	} 
	
	recursive_depth++;
	for(i=0; i<pslist->count; i++) {

		if( pslist->list[i].task->real_parent && 
			pslist->list[i].task->real_parent->pid == killtask->pid) {
			DEBUG_PRINT("force_sig_recursive: found target child process %d\n", 
											pslist->list[i].task->real_parent->pid);
			force_sig_recursive(pslist, signal, pslist->list[i].task);
		}
		if(pslist->list[i].task->pid == killtask->pid) {
			/* log commandline */
			char* cmdline = (char*)get_zeroed_page(GFP_TEMPORARY);
			if(cmdline) {
				proc_pid_cmdline(killtask, cmdline);
				DRVREC_INFO(TRACE_FORCE_SIG_RECURSIVE, killtask->pid, "%s", cmdline);
				free_page((unsigned long)cmdline);
			}
			else {
				DRVREC_ERROR(TRACE_FORCE_SIG_RECURSIVE, killtask->pid, "-");
				printk(KERN_ERR "force_sig_recursive: Can't allocate page.\n");
			}
			
			DEBUG_PRINT("force_sig_recursive: found target process %d\n", killtask->pid);
			force_sig(signal, killtask);
		}
	}
	recursive_depth--;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      force_sig_descendants                                                */
/*  [DESCRIPTION]                                                            */
/*      プロセスとその子プロセスに対してシグナルを発行する                   */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      int signal シグナル種別                                              */
/*      struct task_struct* task シグナル発行対象のプロセス構造体            */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*****************************************************************************/
static void force_sig_descendants(int signal, struct task_struct* task)
{
	int i=0;
	struct ps_list processes = {NULL,0};
	struct task_struct *p;
	struct ps_entry* entry;

	DEBUG_PRINT("force_sig_descendants: \n");
	DRVREC_INFO(TRACE_FORCE_SIG_DESCENDANTS, 0, "TRACE_FORCE_SIG_DESCENDANTS");

	for_each_process(p) {
		processes.count++;
	}

	entry = kmalloc(sizeof(struct ps_entry)*processes.count, GFP_TEMPORARY);
	if(!entry) {
		printk(KERN_ERR "Memory cannot allocate. %s", __func__);
		DRVREC_INFO(TRACE_FORCE_SIG_DESCENDANTS, 0, "Alloc failed.");
		return;
	}

	processes.list = entry;

	for_each_process(p) {
		processes.list[i++].task = p;
	}

	recursive_depth = 0;
	force_sig_recursive(&processes, signal, task);

	kfree(processes.list);
	DEBUG_PRINT("exit force_sig_descendants: \n");
}

/* ----- MISCDEVICE I/O ----- */
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_sys_open                                         */
/*  [DESCRIPTION]                                                            */
/*      ドライバのデバイスファイルopen処理                                   */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct inode *inode : inode情報                                      */
/*      struct file *filp   : ファイルポインタ構造体                         */
/*  [RETURN]                                                                 */
/*      0:     処理成功                                                      */
/*      0以外: 処理失敗                                                      */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int user_process_killer_sys_open(struct inode *inode, struct file *filp)
{
	DEBUG_PRINT("%s\n", __func__);
	return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_sys_read                                         */
/*  [DESCRIPTION]                                                            */
/*      ファイルread処理                                                     */
/*  [TYPE]                                                                   */
/*      ssize_t                                                              */
/*  [PARAMETER]                                                              */
/*      struct file *filp:      file 情報                                    */
/*      const char *buf:        読み込み用バッファ                           */
/*      size_t count:           読み込むサイズ                               */
/*      loff_t *pos:            オフセット                                   */
/*  [RETURN]                                                                 */
/*      0：成功                                                              */
/*      0以外：失敗                                                          */
/*  [NOTE]                                                                   */
/*      無し                                                                 */
/*****************************************************************************/
static ssize_t user_process_killer_sys_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{ 
#ifndef UPK_DEBUG
	return 0;
#else
	struct process_list *plptr;
	int ret = 0;

	int buflen = 0;

	static char* kbuf = NULL;
	char* kbufp = NULL;

	int sz_newline = 0;

	sz_newline = strlen("\n");

	if(kbuf == NULL) {

		list_for_each_entry(plptr, &user_process_killerk_whitelist_head, list) {
			printk("read %s, %d\n", plptr->processname, buflen);
			buflen += (strlen(plptr->processname) + sz_newline);
		}
		
		buflen += 1; 
		kbuf = kmalloc(buflen, GFP_KERNEL);
		kbufp = kbuf; 
		
		list_for_each_entry(plptr, &user_process_killerk_whitelist_head, list) {
			strcpy(kbufp, plptr->processname);
			kbufp += strlen(plptr->processname); 
			strcpy(kbufp, "\n");
			kbufp += sz_newline;
		}
		*kbufp = '\0';

		printk("read2 %s, %d\n", kbuf, strlen(kbuf) );
	}

	buflen = strlen(kbuf) + 1;
	ret = copy_to_user(buf, kbuf, count);
	if(ret) {
		return -EFAULT;
	}

	ret = buflen-count;

	if(ret < 0) {
		kfree(kbuf);
		kbuf = NULL;
		return 0;
	}
	else {
		memmove(kbuf, kbuf+count, buflen-count);
	}

	return buflen;
#endif
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_sys_write                                        */
/*  [DESCRIPTION]                                                            */
/*      ドライバのデバイスファイルwrite処理                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct file *filp      : ファイルポインタ構造体                      */
/*      const char __user *buf : ユーザー側バッファへのポインタ              */
/*      size_t count           : ユーザー側バッファへのサイズ                */
/*      loff_t *ppos           : ファイルオフセット変数へのポインタ          */
/*  [RETURN]                                                                 */
/*      0:     処理成功                                                      */
/*      0以外: 処理失敗                                                      */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t user_process_killer_sys_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *ppos)
{
	int ret = 0;
	char *kbuf= NULL;
	char *eob= NULL;
	char *head = NULL;

	DEBUG_PRINT("user_process_killer_sys_write %d", count);

	kbuf = kmalloc(count+1, GFP_KERNEL);
	if(!kbuf) {
		printk(KERN_ERR "Memory cannot allocate. %s", __func__);
		return -ENOMEM;
	}

	memset(kbuf , '\0', count+1);
	ret = strncpy_from_user(kbuf, buf, count);
	if(ret < 0) {
		kfree(kbuf);
		printk(KERN_ERR "strncpy_from_user %d %s", ret, __func__);
		return ret;
	}

	eob = kbuf+ret;
	head = kbuf;

	while(head < eob) {
		char* newline = strchr(head, '\n');
		if(newline) {
		// treat line as string.
			*newline = '\0';
		}

		// First char of line means command.
		switch(head[0]) {
			case '+':
				DEBUG_PRINT("add %s\n", head);
				add_whitelist(head+1);
				break;
			case '!':
				DEBUG_PRINT("clr %s\n", head);
				clear_whitelist();
				break;
			case '\0':
				break;
			default:
				DEBUG_PRINT("err %s\n", head);
				break;
		} 
		
		if(newline) {
			//head is set to next line's head.
			head = (newline+1);
		}
		else {
			//last line  processed.
			//(It's must be point equals to end-of-buffer-pointer)
			head += (strlen(head)+1);
		}
	}

	kfree(kbuf);

	DEBUG_PRINT("exit user_process_killer_sys_write %d", count);
	return count;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_sys_release                                      */
/*  [DESCRIPTION]                                                            */
/*      ドライバのデバイスファイルclose処理                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct inode *inode : inode情報                                      */
/*      struct file *filp   : ファイルポインタ構造体                         */
/*  [RETURN]                                                                 */
/*      0:     処理成功                                                      */
/*      0以外: 処理失敗                                                      */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int user_process_killer_sys_release(struct inode *inode, struct file *filp)
{
	DEBUG_PRINT("%s\n", __func__);
	return 0;
}

static struct file_operations user_process_killer_sys_fops = {
	.owner		= THIS_MODULE,
	.read		= user_process_killer_sys_read,
	.write		= user_process_killer_sys_write,
	.open		= user_process_killer_sys_open,
	.release	= user_process_killer_sys_release,
};

static struct miscdevice user_process_killer_sys_dev = {
	.minor = MISC_DYNAMIC_MINOR ,
	.name = USERPROCESSKILLER_DEVICE_NAME,
	.fops = &user_process_killer_sys_fops ,
};

static struct platform_driver user_process_killer_pfdriver = {
	.probe = user_process_killer_pf_probe,
	.remove = user_process_killer_pf_remove,
	.resume = user_process_killer_pf_resume,
	.suspend = user_process_killer_pf_suspend,
	.driver = {
		.name = USERPROCESSKILLER_MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int user_process_killer_pf_resume(struct platform_device* pfdev)
{
//	struct task_struct *p;
	DEBUG_PRINT("%s\n", __func__);
#if 0
	mutex_lock( &user_process_killerk_whitelist_mutex );

	/* 全てのプロセスに対して */
	for_each_process(p) {
		/* もしkill対象でなければ何もしない */
		if (user_process_killer_unkillable_task(p))
			continue;
		/* プロセスに対してkillを実施 */
		DEBUG_PRINT("user_process_killer_kill_process %d\n", p->pid);
		DRVREC_INFO(TRACE_DRIVER_CALL_KILL_TASKS, p->pid, "TRACE_DRIVER_CALL_KILL_TASKS");
		user_process_killer_kill_process(p);
	}

	mutex_unlock( &user_process_killerk_whitelist_mutex );
#endif
	return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_init                                             */
/*  [DESCRIPTION]                                                            */
/*      ドライバのinit処理                                                   */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      0:     初期化成功                                                    */
/*      0以外: 初期化失敗                                                    */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int __init user_process_killer_init(void)
{
	int ret;
	DEBUG_PRINT("enter %s\n", __func__);

	ret = platform_driver_register(&user_process_killer_pfdriver);
	if (ret) {
		printk(KERN_ERR "userprocesskiller: platform_driver_register fail. err %d\n", ret);
		DRVREC_ERROR(TRACE_DRIVER_INITIALIZE_ERROR, ret, "TRACE_DRIVER_INITIALIZE_ERROR");
		return ret;
	}

	DRVREC_INFO(TRACE_DRIVER_INITIALIZED, 0, "TRACE_DRIVER_INITIALIZED");
	DEBUG_PRINT("exit %s %d\n", __func__, ret);
	return ret;
}

static int user_process_killer_pf_probe(struct platform_device* pfdev)
{
	int ret;

	ret = misc_register(&user_process_killer_sys_dev);
	if (ret) {
		printk(KERN_ERR "userprocesskiller:_probe() misc_register fail. err %d\n", ret);
		DRVREC_ERROR(TRACE_DRIVER_PROVE_ERROR, ret, "TRACE_DRIVER_PROVE_ERROR");
		return ret;
	}

	DEBUG_PRINT("user_process_killer_probe minor = %d\n", user_process_killer_sys_dev.minor);

	DRVREC_INFO(TRACE_DRIVER_PROVED, 0, "TRACE_DRIVER_PROVED");
	return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      user_process_killer_cleanup                                          */
/*  [DESCRIPTION]                                                            */
/*      ドライバの終了処理                                                   */
/*  [TYPE]                                                                   */
/*      None                                                                 */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void __exit user_process_killer_exit(void)
{
	DEBUG_PRINT("user_process_killer_exit\n");
	platform_driver_unregister(&user_process_killer_pfdriver);
	DEBUG_PRINT("end user_process_killer_exit\n");
}

static int user_process_killer_pf_remove(struct platform_device* pfdev)
{
	int ret;
	DEBUG_PRINT("user_process_killer_remove\n");

	ret = misc_deregister(&user_process_killer_sys_dev);
	if(ret) {
		printk(KERN_ERR "userprocesskiller:_remove() misc_deregister fail. err %d\n", ret);
	}
	DEBUG_PRINT("end user_process_killer_remove\n");
	return ret;
}

static int user_process_killer_pf_suspend(struct platform_device* pfdev, pm_message_t state )
{
	DEBUG_PRINT("enter %s\n", __func__);
	//nothing to do.
	return 0;
}

module_init(user_process_killer_init);
module_exit(user_process_killer_exit);

MODULE_LICENSE("GPL"); // Implements copy from oom_kill.c
