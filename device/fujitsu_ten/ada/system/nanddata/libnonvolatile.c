/*
 * Copyright(C) 2012 FUJITSU TEN LIMITED
 */
#include <stdio.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/nonvolatile.h>

// Not volatile reading interface
int GetNONVOLATILE(uint8_t* aBuff, unsigned int aOffset, unsigned int aSize)
{
	int	ret;
	int	fd;

	// parameter check
	if( (aBuff == NULL) || ((aOffset + aSize) > NONVOLATILE_SIZE_ALL) ){
		return -EINVAL;
	}

	// nonvolatile device open
	fd = open( NONVOLATILE_DRV_NAME, O_RDWR );
	if( fd < 0 ){
		return -1;
	}
	// file pointer seek
	lseek( fd, aOffset, SEEK_SET );
	// data read
	ret = read( fd, aBuff, aSize );
	if( ret < 0 ){
		close( fd );
		return -1;
	}
	close( fd );
	return ret;
}

// Not volatile writing interface
int SetNONVOLATILE(uint8_t* aBuff, unsigned int aOffset, unsigned int aSize)
{
	int	ret;
	int	fd;

	// parameter check
	if( (aBuff == NULL) || ((aOffset + aSize) > NONVOLATILE_SIZE_ALL) ){
		return -EINVAL;
	}

	// nonvolatile device open
	fd = open( NONVOLATILE_DRV_NAME, O_RDWR );
	if( fd < 0 ){
		return -1;
	}
	// file pointer seek
	lseek( fd, aOffset, SEEK_SET );
	// data write
	ret = write( fd, aBuff, aSize );
	if( ret < 0 ){
		close( fd );
		return -1;
	}
	close( fd );
	return ret;
}

