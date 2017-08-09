/**
 * @file nonvolatile.c
 *
 * nonvolatile data READ/WRITE driver for Android
 *
 * Copyright(C) 2010,2011 FUJITSU LIMITED
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/proc_fs.h>
#include <linux/memblock.h>

#include <linux/stddef.h>
#include <linux/earlydevice.h>

#include <linux/fs.h>
#include <linux/err.h>
#include <linux/cdev.h>

#include <linux/mutex.h>
#include <linux/semaphore.h>

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include "nonvolatile_priv.h"
#include <linux/lites_trace_drv.h>

#include <ada/memorymap.h>

#ifdef	UNITTEST_CUTTER
#define	LOCAL
#else
#define	LOCAL	static
#endif

struct work_struct nonvolatile_sys_work;

static const unsigned int nonvolatile_devs = 1; /* device count */
static unsigned int nonvolatile_major = 0;
static struct cdev nonvolatile_cdev;
static struct class	 *nonvolatile_class;
static dev_t dev_id;

#define SETUP_BUF_SIZE (NONVOLATILE_SIZE_ALL+NONVOLATILE_SIZE_ACC)


uint8_t			*iSetupBuffer;
DEFINE_SEMAPHORE(nonvolatile_init_sem);
atomic_t writing_counter = ATOMIC_INIT(0);


struct nonvolatile_device {
	uint8_t *NONVOLATILE_RAM_ADRS;
	struct mutex lock_sys;
	int suspend_flag;
	int modify_flag;
	int modify_flag_refresh;
	int initialized;
	int mode;
	struct early_device edev;
};

LOCAL struct nonvolatile_device *nonvolatile_device = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief       Search for block device from device name.
 * @param[in]   *name Device name.
 * @return      block_device Block device structure.
 */
LOCAL struct block_device * block_finddev(const char *name)
{
	struct class_dev_iter	diter;
	struct device	*dev;
	struct gendisk	*disk;
	struct disk_part_iter	piter;
	struct hd_struct	*part;
	struct block_device	*bdev=NULL;
	char	buf[BDEVNAME_SIZE];

	NONVOL_DEBUG_FUNCTION_ENTER
	/* block class is searched */
	class_dev_iter_init(&diter, &block_class, NULL, NULL);

	while((bdev == NULL) && ((dev = class_dev_iter_next(&diter)) != NULL)) {

		if(strcmp(dev->type->name, "disk") != 0) {
			continue;
		}

		NONVOL_INFO_LOG( "disk find\n");
		disk = dev_to_disk(dev);
		if (get_capacity(disk) == 0 ) {
			continue;
		}

		NONVOL_INFO_LOG( "get_capacity find\n");
		if (disk->flags & GENHD_FL_SUPPRESS_PARTITION_INFO) {
			continue;
		}

		NONVOL_INFO_LOG( "flags find\n");
		/* partition is searched */
		disk_part_iter_init(&piter, disk, DISK_PITER_INCL_PART0);
		while((part = disk_part_iter_next(&piter)) != NULL) {
			disk_name(disk, part->partno, buf);
			/* name is Agreement */
			if(strcmp(buf, name) == 0) {
				bdev = bdget(part_devt(part));
				NONVOL_INFO_LOG( "part_devt find\n");
				break;
			}
		}
		disk_part_iter_exit(&piter);
	}
	class_dev_iter_exit(&diter);

	NONVOL_DEBUG_FUNCTION_EXIT
	return bdev;
}

#if 0
/**
 * @brief       Read the data from block device(eMMC)
 * @param[in]   *devname Block device name.
 * @param[in]   offset Offset data.
 * @param[out]  *buf Data read pointer.
 * @param[in]   len Data length.
 * @return      0 Normal return.
 */
LOCAL int block_read(const char *devname, loff_t offset, uint8_t *buf, size_t len)
{
	struct block_device *bdev;
	pgoff_t index;
	size_t start;
	size_t size;
	struct page *page;
	int ret;
	uint8_t *page_addr;
	fmode_t mode=FMODE_READ;

	NONVOL_DEBUG_FUNCTION_ENTER
	bdev = block_finddev(devname);
	if(bdev == NULL) {
		NONVOL_ERR_LOG( "ERROR! block_finddev() ret=NULL\n");
		return ENODEV;
	}

	ret = blkdev_get(bdev, mode, NULL);
	if(ret) {
		NONVOL_ERR_LOG( "ERROR! blkdev_get() ret=%d\n",ret);
		return ret;
	}

	index = offset >> PAGE_SHIFT;
	start = offset % PAGE_SIZE;

	do {
		/* reading length Calculation */
		if((start + len) > PAGE_SIZE) {
			size = PAGE_SIZE - start;
		}
		else {
			size = len;
		}
		/* Page reading */
		page = read_mapping_page(bdev->bd_inode->i_mapping, index, NULL);
		if(page == NULL) {
			ret = ENOMEM;
			NONVOL_ERR_LOG( "ERROR! read_mapping_page() page=NULL\n");
			break;
		}

		if(IS_ERR(page)) {
			ret = PTR_ERR(page);
			NONVOL_ERR_LOG( "ERROR! IS_ERR(page:%p) ret=%d\n",page, ret);
			break;
		}

		/* Read content is copied */
		page_addr = (uint8_t *)page_address(page);
		page_addr += start;
		memcpy(buf, page_addr, size);
		page_cache_release(page);

		index++;
		start = 0;
		buf += size;
		len -= size;
	} while(len > 0);
	blkdev_put(bdev, mode);

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}
#endif

/**
 * @brief       Write the data from block device(eMMC)
 * @param[in]   *devname Block device name.
 * @param[in]   offset Offset data.
 * @param[in]   *buf Data write pointer.
 * @param[in]   len Data length.
 * @retval      0 Normal return.
 * @retval      EFAULT Data write pointer is NULL.
 * @retval      ENODEV Block device not found.
 * @retval      ENOMEM Memory mapping error.
 */
LOCAL int block_write(const char *devname, loff_t offset, const uint8_t *buf, size_t len)
{
	struct block_device *bdev;
	pgoff_t index;
	size_t start;
	size_t size;
	struct page *page;
	int ret;
	int ret2;
	uint8_t *page_addr;
	fmode_t mode=FMODE_READ|FMODE_WRITE;

	NONVOL_DEBUG_FUNCTION_ENTER
	if(buf == NULL) {
		NONVOL_ERR_LOG( "ERROR! buffer is NULL\n" );
		return EFAULT;
	}
	bdev = block_finddev(devname);
	if(bdev == NULL) {
		NONVOL_ERR_LOG( "ERROR! block_finddev() ret=NULL\n");
		return ENODEV;
	}

	ret = blkdev_get(bdev, mode, NULL);
	if(ret) {
		NONVOL_ERR_LOG( "ERROR! blkdev_get() ret=%d\n",ret);
		return ret;
	}

	index = offset >> PAGE_SHIFT;
	start = offset % PAGE_SIZE;

	do {
		/* Writing length calculation */
		if((start + len) > PAGE_SIZE) {
			size = PAGE_SIZE - start;
		}
		else {
			size = len;
		}

		/* Page reading */
		page = read_mapping_page(bdev->bd_inode->i_mapping, index, NULL);
		if(page == NULL) {
			ret = ENOMEM;
			NONVOL_ERR_LOG( "ERROR! read_mapping_page() page=NULL\n");
			break;
		}

		if(IS_ERR(page)) {
			ret = PTR_ERR(page);
			NONVOL_ERR_LOG( "ERROR! IS_ERR(page:%p) ret=%d\n",page, ret);
			break;
		}

		/* Read content is rewritten */
		lock_page(page);
		page_addr = (uint8_t *)page_address(page);
		page_addr += start;
		if(memcmp(page_addr, buf, size)) {
			memcpy(page_addr, buf, size);
			set_page_dirty(page);
		}
		unlock_page(page);

		page_cache_release(page);

		index++;
		start = 0;
		buf += size;
		len -= size;
	} while(len > 0);

	ret2 = sync_blockdev(bdev);
	if(ret2){
		NONVOL_ERR_LOG( "ERROR! sync_blockdev() ret=%d\n",ret2);
		if(!ret)
			ret = ret2;
	}
	blkdev_put(bdev, mode);

	NONVOL_NOTICE_LOG( "eMMC data write complete.\n" );
	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Read the data from memory(SDRAM)
 * @param[out]  *iBuff Data read pointer.
 * @param[in]   iOffset Offset data.
 * @param[in]   iSize Data length.
 * @param[in]   *addr SDRAM memory mapping pointer.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int nonvolatile_readcpy(uint8_t* iBuff, unsigned int iOffset, unsigned int iSize, uint8_t *addr)
{
	uint32_t header_work[8];
	unsigned long nonvolatile_mode;

	NONVOL_DEBUG_FUNCTION_ENTER
	if( iBuff == NULL ) {
		NONVOL_ERR_LOG( "ERROR! iBuff=NULL\n" );
		return -1;
	}
	if (iOffset < (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE)) {
		nonvolatile_mode = NONVOLATILE_MODE_NORMAL;
		NONVOL_DEBUG_LOG( "nonvolatile_readcpy : NONVOLATILE_MODE_NORMAL mode\n" );
	} else if((iOffset >= (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE+NONVOLATILE_SIZE_NONACCESS))
			   && (iOffset < (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE_ALL)) ) {
		nonvolatile_mode = NONVOLATILE_MODE_REFRESH;
		NONVOL_DEBUG_LOG( "nonvolatile_readcpy : NONVOLATILE_MODE_REFRESH mode\n" );
	} else {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. size=%d, offset=0x%08x\n", iSize, iOffset );
		return -1;
	}

	// Nonvolatile header read
	memcpy((uint8_t*)header_work, addr, NONVOLATILE_HEADER_SIZE);
	NONVOL_INFO_LOG( "header_work[0] = %x, header_work[1] = %x, header_work[2] = %x,\n",
	                 header_work[0],
	                 header_work[1],
	                 header_work[2]);

	if ((header_work[0] != NONVOLATILE_CTL_HEADER0) || (header_work[1] != NONVOLATILE_CTL_HEADER1)){
		NONVOL_ERR_LOG( "ERROR! header_work error! 0x%x, 0x%x\n", header_work[0], header_work[1] );
		return -1;
	}

	if ((header_work[2] == 0xFFFFFFFF) || (header_work[2] == 0x0)) {
		NONVOL_ERR_LOG( "ERROR! CHECK1 error\n" );
		return -1;
	}

	if ((nonvolatile_mode == NONVOLATILE_MODE_NORMAL)
		 && (iSize + iOffset) > (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE)) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. size=%d, offset=0x%08x\n", iSize, iOffset );
		return -1;
	} else if ((nonvolatile_mode == NONVOLATILE_MODE_REFRESH)
				&& (iSize + iOffset) > (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE_ALL)) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. size=%d, offset=0x%08x\n", iSize, iOffset );
		return -1;
	}
	memcpy(iBuff, addr + iOffset, iSize);

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

/**
 * @brief       Write the data from memory(SDRAM)
 * @param[in]   *iBuff Data pointer.
 * @param[in]   iOffset Offset data.
 * @param[in]   iSize Data length.
 * @param[in]   *addr SDRAM memory mapping pointer.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int nonvolatile_writecpy(const char* iBuff, unsigned int iOffset, unsigned int iSize)
{
	uint32_t header_work[8];
	unsigned long nonvolatile_mode;

	NONVOL_DEBUG_FUNCTION_ENTER

	if( iBuff == NULL ) {
		NONVOL_ERR_LOG( "ERROR! iBuff=NULL\n" );
		return -1;
	}
	if (iOffset < (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE)) {
		nonvolatile_mode = NONVOLATILE_MODE_NORMAL;
		NONVOL_DEBUG_LOG( "nonvolatile_writecpy : NONVOLATILE_MODE_NORMAL mode\n" );
	} else if((iOffset >= (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE+NONVOLATILE_SIZE_NONACCESS))
			   && (iOffset < (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE_ALL)) ) {
		nonvolatile_mode = NONVOLATILE_MODE_REFRESH;
		NONVOL_DEBUG_LOG( "nonvolatile_writecpy : NONVOLATILE_MODE_REFRESH mode\n" );
	} else {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. size=%d, offset=0x%08x\n", iSize, iOffset );
		return -1;
	}

	if (nonvolatile_mode == NONVOLATILE_MODE_NORMAL) {
		memcpy((uint8_t*)header_work, nonvolatile_device->NONVOLATILE_RAM_ADRS, NONVOLATILE_HEADER_SIZE );
		// Flag confirmed
		if ((header_work[0] != NONVOLATILE_CTL_HEADER0) || (header_work[1] != NONVOLATILE_CTL_HEADER1)) {
			NONVOL_ERR_LOG( "ERROR! header_work error!\n" );
			return -1;
		}
	}

	if ((nonvolatile_mode == NONVOLATILE_MODE_NORMAL)
		 && (iSize + iOffset) > (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE)) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. size=%d, offset=0x%08x\n", iSize, iOffset );
		return -1;
	} else if ((nonvolatile_mode == NONVOLATILE_MODE_REFRESH)
				&& (iSize + iOffset) > (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE_ALL)) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. size=%d, offset=0x%08x\n", iSize, iOffset );
		return -1;
	}

	memcpy(nonvolatile_device->NONVOLATILE_RAM_ADRS + iOffset, iBuff, iSize);

	if (nonvolatile_mode == NONVOLATILE_MODE_NORMAL) {
		header_work[2] = (header_work[2]==0xFFFFFFFE)?1:(header_work[2]+1);
		memcpy(nonvolatile_device->NONVOLATILE_RAM_ADRS, (uint8_t*)header_work, NONVOLATILE_HEADER_SIZE);
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

/**
 * @brief       SDRAM memory mapping.
 * @param       None.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int Init_Setup(void)
{
	uint8_t *addr;
	NONVOL_DEBUG_FUNCTION_ENTER
	// It is alloc& 0 clearness of RAM for the not volatile storage
	// RAM0
	addr = (uint8_t*)ioremap(MEMMAP_NONVOLATILE_ADDR, SETUP_BUF_SIZE);
	if (addr == NULL) {
		NONVOL_ERR_LOG( "ERROR! ioremap arg = %x\n", MEMMAP_NONVOLATILE_ADDR );
		return -1;
	}
	NONVOL_INFO_LOG( "ioremap addr=0x%x\n", (unsigned int)addr );

	// The address of RAM is booked
	nonvolatile_device->NONVOLATILE_RAM_ADRS = addr;

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

#ifdef DEBUG_CREATE_HEADER
/**
 * @brief       Create nonvolatile data header.
 * @param       None.
 * @return      None.
 */
LOCAL void Init_Header(void)
{
	uint32_t iBuff0[8];

	NONVOL_DEBUG_FUNCTION_ENTER
	iBuff0[0] = NONVOLATILE_CTL_HEADER0;
	iBuff0[1] = NONVOLATILE_CTL_HEADER1;
	iBuff0[2] = 0x00000002;

	memcpy(nonvolatile_device->NONVOLATILE_RAM_ADRS, (uint8_t*)iBuff0, NONVOLATILE_HEADER_SIZE);
	NONVOL_DEBUG_LOG( "Header create. [0]=0x%08x, [1]=0x%08x, [2]=0x%08x\n",
	                  iBuff0[0], iBuff0[1], iBuff0[2] );

	NONVOL_DEBUG_FUNCTION_EXIT
	return;
}
#endif	// DEBUG_CREATE_HEADER

/**
 * @brief       Nonvolatile driver header initialization.
 * @param       None.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int nonvolatile_device_init(void)
{
	int ret = 0;
	NONVOL_DEBUG_FUNCTION_ENTER

	down(&nonvolatile_init_sem);
	if( nonvolatile_device != NULL ) {
		NONVOL_INFO_LOG( "nonvolatile_device skipped\n" );
		goto init_end;
	}

	nonvolatile_device = kzalloc(sizeof(struct nonvolatile_device), GFP_KERNEL);
	if (nonvolatile_device == NULL) {
		NONVOL_ERR_LOG( "ERROR! nonvolatile_device=NULL\n");
		ret = -1;
		goto init_end;
	}

	// mode setting
#ifdef CONFIG_NONVOLATILE_MODE_SDRAM
	nonvolatile_device->mode = NONVOL_MODE_ONLY_SDRAM;
#elif defined(CONFIG_NONVOLATILE_MODE_MEMORY)
	nonvolatile_device->mode = NONVOL_MODE_MEMORY;
#else
	nonvolatile_device->mode = NONVOL_MODE_NORMAL;
#endif
	NONVOL_NOTICE_LOG( "Nonvolatile access mode is %d\n",
	                   nonvolatile_device->mode );

	nonvolatile_device->suspend_flag= 1;
	nonvolatile_device->modify_flag = 0;
	nonvolatile_device->modify_flag_refresh = 0;
	NONVOL_DEBUG_LOG( "initialize suspend flag and modify flag.\n" );

	mutex_init(&nonvolatile_device->lock_sys);

	// RAM Initialization
	ret = Init_Setup();
	NONVOL_DEBUG_LOG( "SDRAM initialize complete.\n" );

#ifdef	DEBUG_CREATE_HEADER
	Init_Header();
	NONVOL_DEBUG_LOG( "Nonvolatile header data create.\n" );
#endif	// DEBUG_CREATE_HEADER
	NONVOL_NOTICE_LOG( "## INITIALIZE MODE ##\n" );
	NONVOL_DRC_TRC( "## INITIALIZE MODE ##\n" );

init_end:
	up(&nonvolatile_init_sem);

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Reading nonvolatile data (internal)
 * @param[out]  *aBuff Data read pointer.
 * @param[in]   aOffset Offset data.
 * @param[in]   aSize Data length.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int GetNONVOLATILESys(uint8_t* aBuff, unsigned int aOffset, unsigned int aSize)
{
	int ret = 0;
	unsigned long nonvolatile_mode;

	NONVOL_DEBUG_FUNCTION_ENTER
	// Nonvolatile mode check
	if (aOffset < NONVOLATILE_SIZE) {
		nonvolatile_mode = NONVOLATILE_MODE_NORMAL;
		NONVOL_INFO_LOG( "GetNONVOLATILESys : NONVOLATILE_MODE_NORMAL mode\n" );
	} else if((aOffset >= (NONVOLATILE_SIZE+NONVOLATILE_SIZE_NONACCESS))
	           && (aOffset < NONVOLATILE_SIZE_ALL) ) {
		nonvolatile_mode = NONVOLATILE_MODE_REFRESH;
		NONVOL_INFO_LOG( "GetNONVOLATILESys : NONVOLATILE_MODE_REFRESH mode\n" );
	} else {
		NONVOL_ERR_LOG( "ERROR! over aOffset=0x%04x\n", aOffset );
		return -1;
	}

	if ((nonvolatile_mode == NONVOLATILE_MODE_NORMAL)
		 && (aSize + aOffset) > NONVOLATILE_SIZE) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. aOffset=0x%04x aSize=%d\n", aOffset, aSize );
		return -1;
	} else if ((nonvolatile_mode == NONVOLATILE_MODE_REFRESH)
	            && (aSize + aOffset) > NONVOLATILE_SIZE_ALL) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. aOffset=0x%04x aSize=%d\n", aOffset, aSize );
		return -1;
	}

	// Because it becomes an object since the flag area, 1KB is added to the address
	aOffset += NONVOLATILE_OFFSET_ACC;

	if (nonvolatile_device == NULL) {
		NONVOL_NOTICE_LOG( "nonvolatile_device initialize\n" );
		if(nonvolatile_device_init() != 0){
			NONVOL_ERR_LOG( "ERROR! Nonvolatile initalized failed\n" );
			return -1;
		}
	}
	mutex_lock(&nonvolatile_device->lock_sys);
	ret = nonvolatile_readcpy(aBuff, aOffset, aSize, nonvolatile_device->NONVOLATILE_RAM_ADRS);
	mutex_unlock(&nonvolatile_device->lock_sys);

	if( ret != 0 ) {
		NONVOL_ERR_LOG( "ERROR! data get failed. ret=%d\n", ret );
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Reading nonvolatile data (in kernel)
 * @param[out]  *aBuff Data read pointer.
 * @param[in]   aOffset Offset data.
 * @param[in]   aSize Data length.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
int GetNONVOLATILE(uint8_t* aBuff, unsigned int aOffset, unsigned int aSize)
{
	int ret = 0;

	NONVOL_DEBUG_FUNCTION_ENTER
	ret = GetNONVOLATILESys( aBuff, aOffset, aSize);
	if (ret != 0) {
		NONVOL_ERR_LOG( "ERROR! ADR=0x%04x,SIZE=%d, ret=%d\n", aOffset, aSize,ret );
		if( aBuff != NULL )	memset( aBuff, 0xFF, aSize );
	}
	if (!nonvolatile_device->suspend_flag) {
		NONVOL_NOTICE_LOG( "Data read ADR=0x%04x,SIZE=%d\n", aOffset, aSize );
	} else {
		NONVOL_INFO_LOG( "Data read ADR=0x%04x,SIZE=%d\n", aOffset, aSize );
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}
EXPORT_SYMBOL(GetNONVOLATILE);


/**
 * @brief       Writing nonvolatile data (internal)
 * @param[in]   *aBuff Data readn pointer.
 * @param[in]   aOffset Offset data.
 * @param[in]   aSize Data length.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int SetNONVOLATILESys(const char* aBuff, unsigned int aOffset, unsigned int aSize)
{
	int ret = 0;
	loff_t part_offset = 0;
	unsigned long nonvolatile_mode = 0;
	char msg[32];

	NONVOL_DEBUG_FUNCTION_ENTER
	if (nonvolatile_device == NULL) {
		NONVOL_NOTICE_LOG( "nonvolatile_device initialize\n" );
		if(nonvolatile_device_init() != 0){
			NONVOL_ERR_LOG( "ERROR! Nonvolatile initalized failed\n" );
			return -1;
		}
	}

	// Nonvolatile mode check
	if (aOffset < NONVOLATILE_SIZE) {
		nonvolatile_mode = NONVOLATILE_MODE_NORMAL;
		NONVOL_INFO_LOG( "SetNONVOLATILESys : NONVOLATILE_MODE_NORMAL mode\n" );
	} else if((aOffset >= (NONVOLATILE_SIZE+NONVOLATILE_SIZE_NONACCESS))
			   && (aOffset < NONVOLATILE_SIZE_ALL) ) {
		nonvolatile_mode = NONVOLATILE_MODE_REFRESH;
		NONVOL_INFO_LOG( "SetNONVOLATILESys : NONVOLATILE_MODE_REFRESH mode\n" );
	} else {
		NONVOL_ERR_LOG( "ERROR! over aOffset=0x%04x\n", aOffset );
		return -1;
	}

	if ((nonvolatile_mode == NONVOLATILE_MODE_NORMAL)
		 && (aSize + aOffset) > NONVOLATILE_SIZE) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. aOffset=0x%04x aSize=%d\n", aOffset, aSize );
		return -1;
	} else if ((nonvolatile_mode == NONVOLATILE_MODE_REFRESH)
				&& (aSize + aOffset) > NONVOLATILE_SIZE_ALL) {
		NONVOL_ERR_LOG( "ERROR! Nonvolatile size over. aOffset=0x%04x aSize=%d\n", aOffset, aSize );
		return -1;
	}

	// Because it becomes an object since the flag area, 1KB is added to the address
	aOffset += NONVOLATILE_OFFSET_ACC;

	// Writes it in RAM and eMMC
	mutex_lock(&nonvolatile_device->lock_sys);
	ret = nonvolatile_writecpy(aBuff, aOffset, aSize);

	if (ret == 0) {
		if (nonvolatile_device->suspend_flag == 0 &&
			nonvolatile_device->mode == NONVOL_MODE_NORMAL ) {
			atomic_inc( &writing_counter );
            part_offset = find_partition_start_address_by_name(NONVOLATILE_BLOCK_DEV, NONVOLATILE_PART_NAME);
			if(part_offset > 0) {
				NONVOL_NOTICE_LOG( "NONVOLATILE block_write APL0_OFFSET %llx\n",
				                   (long long)part_offset );
				if (nonvolatile_mode == NONVOLATILE_MODE_NORMAL) {
					ret = block_write( NONVOLATILE_BLOCK_DEV,
				                       part_offset,
				                       nonvolatile_device->NONVOLATILE_RAM_ADRS,
				    	               (NONVOLATILE_OFFSET_ACC+NONVOLATILE_SIZE));
				} else {
					ret = block_write( NONVOLATILE_BLOCK_DEV,
				                       (part_offset+nonvolatile_mode),
				                       (nonvolatile_device->NONVOLATILE_RAM_ADRS+nonvolatile_mode),
				                       NONVOLATILE_SIZE_REFRESH);
				}
				atomic_dec( &writing_counter );
				if (ret != 0) {
					NONVOL_ERR_LOG( "ERROR! block_write() ret=%d\n", ret);
					if(ret == ENODEV){
						ret = -2;
					}
					ret = -1;
				}
			} else {
				NONVOL_ERR_LOG( "ERROR! find_partition_start_address_by_name() ret=%lld\n",
				                (long long)part_offset);
				ret = -1;
			}
		} else {
			if (nonvolatile_device->modify_flag == 0 && nonvolatile_device->modify_flag_refresh == 0) {
				NONVOL_NOTICE_LOG( "### eMMC != SDRAM mode ###\n" );
				NONVOL_DRC_TRC( "### eMMC != SDRAM mode ###\n" );
			}
			if (nonvolatile_mode == NONVOLATILE_MODE_NORMAL) {
				nonvolatile_device->modify_flag = 1;
			} else {
				nonvolatile_device->modify_flag_refresh = 1;
			}
			snprintf( msg, sizeof(msg), "DATA off=0x%04x, sz=0x%04x\n",
			          (aOffset-NONVOLATILE_OFFSET_ACC), aSize );
			NONVOL_DRC_TRC( msg );
		}
	}
	else {
		NONVOL_ERR_LOG( "ERROR! nonvolatile_writecpy() ret=-1\n");
	}
	mutex_unlock(&nonvolatile_device->lock_sys);

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Writing nonvolatile data (in kernel)
 * @param[out]  *aBuff Data read pointer.
 * @param[in]   aOffset Offset data.
 * @param[in]   aSize Data length.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
int SetNONVOLATILE(uint8_t* aBuff, unsigned int aOffset, unsigned int aSize)
{
	int ret = 0;

	NONVOL_DEBUG_FUNCTION_ENTER

	ret = SetNONVOLATILESys(aBuff, aOffset, aSize);
	if (ret != 0) {
		NONVOL_ERR_LOG( "ERROR! ADR=0x%04x,SIZE=%d, ret=%d\n", aOffset, aSize,ret );
	}
	if (!nonvolatile_device->suspend_flag) {
		NONVOL_NOTICE_LOG( "Data write ADR=0x%04x,SIZE=%d\n", aOffset, aSize );
	} else {
		NONVOL_INFO_LOG( "Data write ADR=0x%04x,SIZE=%d\n", aOffset, aSize );
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}
EXPORT_SYMBOL(SetNONVOLATILE);

/**
 * @brief       Check the resume flag of resume.
 * @param[in]   flag Suspend flag.
 * @param[in]   *param Nonvolatile device information.
 * @return      0 Normal return.
 */
LOCAL int nonvolatile_resumeflag_atomic_callback(bool flag, void* param)
{
	NONVOL_DEBUG_FUNCTION_ENTER
	struct nonvolatile_device *data = (struct nonvolatile_device *)param;
	if( flag == false ){
		data->suspend_flag = 0;
	}
	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

/**
 * @brief       Nonvolatile data synchronization process (work queue)
 * @param[in]   *work Work queue.
 * @return      None.
 */
LOCAL void wq_block_write(struct work_struct *work)
{
	int ret = 0;
	int count = 0;
	loff_t part_offset = 0;
	unsigned long nonvolatile_mode = 0;

	NONVOL_DEBUG_FUNCTION_ENTER
	NONVOL_NOTICE_LOG( "## BLOCKWRITE MODE ##\n" );
	if( nonvolatile_device->mode != NONVOL_MODE_NORMAL ){
		NONVOL_INFO_LOG( "Nonvolatile mode is %d. eMMC not support mode.\n",
		                  nonvolatile_device->mode );
		NONVOL_NOTICE_LOG( "## ACCESS MODE ##\n" );
		return;
	}
	while( (block_finddev( NONVOLATILE_BLOCK_DEV ) == NULL) && (count < NONVOLATILE_MAX_COUNT) ){
		NONVOL_NOTICE_LOG( "##### eMMC device init wait 5sec. #####\n");
		msleep(NONVOLATILE_WAIT_TIME*1000);
		count++;
	}
	if( count == NONVOLATILE_MAX_COUNT ){
		NONVOL_ERR_LOG( "ERROR! eMMC device none. device=%s\n", NONVOLATILE_BLOCK_DEV );
		return;
	}
	early_device_invoke_with_flag_irqsave(
		EARLY_DEVICE_BUDET_INTERRUPTION_MASKED,
		nonvolatile_resumeflag_atomic_callback, nonvolatile_device );
	if( nonvolatile_device->suspend_flag ){
		NONVOL_NOTICE_LOG( "## re_init called in bu-det ##\n" );
		return;
	}
	mutex_lock(&nonvolatile_device->lock_sys);
	if (nonvolatile_device->modify_flag)
	{
		part_offset = find_partition_start_address_by_name(NONVOLATILE_BLOCK_DEV, NONVOLATILE_PART_NAME);
		if(part_offset > 0) {
			NONVOL_NOTICE_LOG( "NONVOLATILE block_write APL0_OFFSET %llx\n",
			                   (long long)part_offset);
			ret = block_write( NONVOLATILE_BLOCK_DEV,
			                   part_offset,
		    	               nonvolatile_device->NONVOLATILE_RAM_ADRS, 
		        	           (NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE));

			if( ret == 0 ) {
				nonvolatile_device->modify_flag = 0;
				NONVOL_NOTICE_LOG( "## SDRAM to eMMC data modify (NORMAL) ##\n" );
				NONVOL_DRC_TRC( "## SDRAM to eMMC data modify ##\n" );
			}
			else {
				NONVOL_ERR_LOG( "ERROR! block_write ret  = %d\n", ret );
			}
		} else {
			NONVOL_ERR_LOG( "ERROR! find_partition_start_address_by_name() ret=%lld\n",
			                (long long)part_offset );
		}
	}
	if (nonvolatile_device->modify_flag_refresh)
	{
		nonvolatile_mode = NONVOLATILE_SIZE_ACC+NONVOLATILE_SIZE+NONVOLATILE_SIZE_NONACCESS;
		part_offset = find_partition_start_address_by_name(NONVOLATILE_BLOCK_DEV, NONVOLATILE_PART_NAME);
		if(part_offset > 0) {
			NONVOL_NOTICE_LOG( "NONVOLATILE block_write APL0_OFFSET %llx\n",
			                   (long long)part_offset);
			ret = block_write( NONVOLATILE_BLOCK_DEV,
			                   (part_offset+nonvolatile_mode),
		                       (nonvolatile_device->NONVOLATILE_RAM_ADRS+nonvolatile_mode),
		        	           NONVOLATILE_SIZE_REFRESH);

			if( ret == 0 ) {
				nonvolatile_device->modify_flag_refresh = 0;
				NONVOL_NOTICE_LOG( "## SDRAM to eMMC data modify (REFRESH) ##\n" );
			}
			else {
				NONVOL_ERR_LOG( "ERROR! block_write ret  = %d\n", ret );
			}
		} else {
			NONVOL_ERR_LOG( "ERROR! find_partition_start_address_by_name() ret=%lld\n",
			                (long long)part_offset );
		}
	}
	mutex_unlock(&nonvolatile_device->lock_sys);
	NONVOL_NOTICE_LOG( "## ACCESS MODE ##\n" );

	NONVOL_DEBUG_FUNCTION_EXIT
}

/**
 * @brief       Nonvolatile driver initialize function.
 * @param       None.
 * @return      None.
 */
LOCAL int Initialize(void)
{
	int ret = 0;

	NONVOL_DEBUG_FUNCTION_ENTER

	INIT_WORK(&nonvolatile_sys_work, wq_block_write);
	NONVOL_DEBUG_LOG( "work_queue(wq_block_write) initialize complete.\n" );
	// modify mode check
	schedule_work( &nonvolatile_sys_work );

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Nonvolatile device open function.
 * @param[in]   *inode inode.
 * @param[in]   *flip File information.
 * @return      0 Normal return.
 */
LOCAL int nonvolatile_open(struct inode *inode, struct file *filp)
{
	NONVOL_DEBUG_FUNCTION_ENTER_ONLY
	return 0;
}

/**
 * @brief       Nonvolatile device close function.
 * @param[in]   *inode inode.
 * @param[in]   *flip File information.
 * @return      0 Normal return.
 */
LOCAL int nonvolatile_close(struct inode *inode, struct file *filp)
{
	NONVOL_DEBUG_FUNCTION_ENTER_ONLY
	return 0;
}

/**
 * @brief       Reading nonvolatile data (user layer)
 * @param[in]   *filp File information.
 * @param[out]  *buff Data read pointer.
 * @param[in]   len Data length.
 * @param[in]   off_t Offset data.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL ssize_t nonvolatile_read(struct file *filp, char *buf, size_t len, loff_t *loffset)
{
	ssize_t ret = 0;
	uint8_t *work_buf;

	NONVOL_DEBUG_FUNCTION_ENTER

	if( filp == NULL ) {
		NONVOL_ERR_LOG( "ERROR! filp = NULL\n" );
		return -1;
	}

	if( buf == NULL ) {
		NONVOL_ERR_LOG( "ERROR! buf = NULL\n" );
		return -1;
	}

	NONVOL_INFO_LOG( "Start data read ADR=0x%04x,SIZE=%d\n", (unsigned int)filp->f_pos, len );
	work_buf = kzalloc(len, GFP_KERNEL);
	if(work_buf == NULL) {
		NONVOL_ERR_LOG( "ERROR! kzalloc failed. work_buf = NULL\n" );
		return -1;
	}

	if( GetNONVOLATILESys( work_buf, filp->f_pos, len) != 0 ) {
		NONVOL_ERR_LOG( "ERROR! GetNONVOLATILESys. read data set is ALL 0xFF\n" );
		memset( work_buf, 0xFF, len );
		ret = -1;
	}
	if( copy_to_user( buf, work_buf, len ) ) {
		NONVOL_ERR_LOG( "ERROR! copy_to_user()\n" );
		ret = -EFAULT;
	}

	kfree(work_buf);
	if (!nonvolatile_device->suspend_flag) {
		NONVOL_NOTICE_LOG( "Data read ADR=0x%04x,SIZE=%d\n", (unsigned int)filp->f_pos, len );
	} else {
		NONVOL_INFO_LOG( "Data read ADR=0x%04x,SIZE=%d\n", (unsigned int)filp->f_pos, len );
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Writing nonvolatile data (user layer)
 * @param[in]   *filp File information.
 * @param[in]   *buff Data write pointer.
 * @param[in]   len Data length.
 * @param[in]   off_t Offset data.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL ssize_t nonvolatile_write(struct file *filp, const char *buf, size_t len, loff_t *loffset)
{
	ssize_t ret = 0;
	uint8_t *work_buf;

	NONVOL_DEBUG_FUNCTION_ENTER

	if( filp == NULL ) {
		NONVOL_ERR_LOG( "ERROR! filp = NULL\n" );
		return -1;
	}

	if( buf == NULL ) {
		NONVOL_ERR_LOG( "ERROR! buf = NULL\n" );
		return -1;
	}

	NONVOL_INFO_LOG( "Start data write ADR=0x%04x,SIZE=%d\n", (unsigned int)filp->f_pos, len );
	work_buf = kzalloc(len, GFP_KERNEL);
	if(work_buf == NULL) {
		NONVOL_ERR_LOG( "ERROR! kzalloc failed. work_buf = NULL\n" );
		return -1;
	}

	if( copy_from_user( work_buf, buf, len ) == 0 ) {
		if( SetNONVOLATILESys(work_buf, filp->f_pos, len) != 0 ) {
			NONVOL_ERR_LOG( "ERROR! SetNONVOLATILESys. ret=%d\n", ret );
			ret = -1;
		}
	}
	else {
		NONVOL_ERR_LOG( "ERROR! copy_to_user()\n" );
		ret = -EFAULT;
	}
	kfree(work_buf);
	if (!nonvolatile_device->suspend_flag) {
		NONVOL_NOTICE_LOG( "Data write ADR=0x%04x,SIZE=%d\n", (unsigned int)filp->f_pos, len );
	} else {
		NONVOL_INFO_LOG( "Data write ADR=0x%04x,SIZE=%d\n", (unsigned int)filp->f_pos, len );
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Set the nonvolatile data offset.
 * @param[in]   *fp File information.
 * @param[in]   pos Data pointer offset.
 * @param[in]   mod Not used.
 * @return      pos Data pointer offset.
 */
loff_t nonvolatile_llseek( struct file *fp, loff_t pos, int mod )
{
	NONVOL_DEBUG_FUNCTION_ENTER
	NONVOL_DEBUG_LOG( "OrgPos = %lld, NewPos = %lld\n", fp->f_pos, pos );
	fp->f_pos = pos;

	NONVOL_DEBUG_FUNCTION_EXIT
	return pos;
}


struct file_operations nonvolatile_fops = {
	.owner   = THIS_MODULE,
	.open    = nonvolatile_open,
	.release = nonvolatile_close,
	.read    = nonvolatile_read,
	.write   = nonvolatile_write,
	.llseek  = nonvolatile_llseek,

};

static struct platform_device nonvolatile_devices = {
	.name = "nonvolatile",
	.id   = -1,
	.dev = {
		.dma_mask          = NULL,
		.coherent_dma_mask = 0xffffffff,
	},
};

static struct platform_device *devices[] __initdata = {
	&nonvolatile_devices,
};

/**
 * @brief       Nonvolatile driver suspend.
 * @param[in]   *pdev Nonvolatile device
 * @param[in]   state pm_message_t
 * @retval      0 Normal return.
 * @retval      -EBUSY Nonvolatile driver working.
 */
LOCAL int nonvolatile_suspend(struct platform_device *pdev, pm_message_t state)
{
	NONVOL_DEBUG_FUNCTION_ENTER

	if (atomic_read( &writing_counter )) {
		NONVOL_ERR_LOG( "WARNING! eMMC writting.\n" );
		return -EBUSY;
	}
	nonvolatile_device->suspend_flag= 1;
	NONVOL_INFO_LOG( "## SUSPEND MODE ##\n" );

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

/**
 * @brief       Nonvolatile driver reinitialize.
 * @param[in]   *dev Nonvolatile device.
 * @return      0 Normal return.
 */
LOCAL int nonvolatile_resume(struct device *dev)
{
	NONVOL_DEBUG_FUNCTION_ENTER

	if (nonvolatile_device->suspend_flag == 0) {
		NONVOL_ERR_LOG( "ERROR! not suspend mode. resume cancel\n" );
		return 0;
	}
	schedule_work( &nonvolatile_sys_work );

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief       Nonvolatile driver device initialize.
 * @param[in]   *pdev Nonvolatile device
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
LOCAL int nonvolatile_drv_probe(struct platform_device *pdev)
{
	int ret;
	struct device *class_dev = NULL;

	NONVOL_DEBUG_FUNCTION_ENTER
	NONVOL_INFO_LOG( "probe call\n" );

	dev_id = MKDEV(0, 0);	// For initialization of dynamic allocation?

    // Character device number that has become empty is acquired
    ret = alloc_chrdev_region(&dev_id, 0, nonvolatile_devs, DRIVER_NAME);
	if (ret) {
		NONVOL_ERR_LOG( "ERROR! alloc_chrdev_region\n");
		goto error;
	}
    nonvolatile_major = MAJOR(dev_id);

	// Relation of method
	cdev_init(&nonvolatile_cdev, &nonvolatile_fops);
	nonvolatile_cdev.owner = THIS_MODULE;
	nonvolatile_cdev.ops   = &nonvolatile_fops;

	ret = cdev_add(&nonvolatile_cdev, MKDEV(nonvolatile_major, 0), 1);
	if (ret) {
		NONVOL_ERR_LOG( "ERROR! cdev_add\n");
		goto error;
	}

	// register class
	nonvolatile_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(nonvolatile_class)) {
		NONVOL_ERR_LOG( "ERROR! class_create\n");
		goto error;
	}

	// register class device
	class_dev = device_create( nonvolatile_class, NULL, MKDEV(nonvolatile_major, 0), NULL, "%s", DEVICE_NAME);

	// nonvolatile device init
	if(nonvolatile_device == NULL) {
		if(nonvolatile_device_init() != 0) {
			goto error;
		}
		NONVOL_INFO_LOG( "nonvolatile_device initialize complete\n" );
	} else {
		NONVOL_INFO_LOG( "nonvolatile_device initialize skipped\n" );
	}

	// nonvolatile set resume action
	nonvolatile_device->edev.dev                    = &nonvolatile_devices.dev;
	nonvolatile_device->edev.reinit                 = nonvolatile_resume;
	nonvolatile_device->edev.sudden_device_poweroff = NULL;
	ret = early_device_register(&nonvolatile_device->edev);
	if ( ret < 0 ){
		NONVOL_ERR_LOG( "ERROR! Early device register failed.\n" );
		goto error_sysfs_add;
	}

	ret = Initialize();
	if (ret) {
		NONVOL_ERR_LOG( "ERROR! Initialize failed.\n" );
		goto error_sysfs_add;
	}

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;

error_sysfs_add:
	device_destroy(nonvolatile_class, MKDEV(nonvolatile_major, 0));
	cdev_del( &nonvolatile_cdev );
	unregister_chrdev_region( dev_id, nonvolatile_devs );
error:
	NONVOL_DEBUG_FUNCTION_EXIT
	return -1;
}

/**
 * @brief       Nonvolatile driver device remove processing.
 * @param[in]   *pdev Nonvolatile device
 * @return      0 Normal return.
 */
LOCAL int nonvolatile_drv_remove(struct platform_device *pdev)
{
	NONVOL_DEBUG_FUNCTION_ENTER
	NONVOL_NOTICE_LOG( "## EXIT MODE ##\n" );

	early_device_unregister(&nonvolatile_device->edev);
	device_destroy(nonvolatile_class, MKDEV(nonvolatile_major, 0));
	cdev_del( &nonvolatile_cdev );
	unregister_chrdev_region( dev_id, nonvolatile_devs );

	// Device exclusion processing
	kfree(nonvolatile_device);

	NONVOL_DEBUG_FUNCTION_EXIT
	return 0;
}

static struct platform_driver nonvolatile_driver = {
	.probe    = nonvolatile_drv_probe,
	.remove   = nonvolatile_drv_remove,
	.suspend  = nonvolatile_suspend,
	.resume   = NULL,
	.driver   = {
		.name = DRIVER_NAME,
	},
};

/**
 * @brief       Nonvolatile driver initialize.
 * @param       None
 * @return      0 normal return.
 */
LOCAL int __devinit nonvolatile_init(void)
{
	int ret;

	NONVOL_DEBUG_FUNCTION_ENTER

	platform_add_devices(devices, ARRAY_SIZE(devices));
	ret = platform_driver_register(&nonvolatile_driver);

	NONVOL_DEBUG_FUNCTION_EXIT
	return ret;
}

/**
 * @brief       Nonvolatile driver finish
 * @param       None
 * @return      0 normal return.
 */
LOCAL void __exit nonvolatile_exit(void)
{
	NONVOL_DEBUG_FUNCTION_ENTER_ONLY
	platform_driver_unregister(&nonvolatile_driver);
}

module_init(nonvolatile_init);
//subsys_initcall(nonvolatile_init);

module_exit(nonvolatile_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("nonvolatile device");
MODULE_LICENSE("GPL");
