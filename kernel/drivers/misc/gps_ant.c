/**
 * @file gps_ant.c
 *
 * GPS Antenna detect driver
 *
 * Copyright(C) 2012-2013 FUJITSU TEN LIMITED
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

#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/gps_ant.h>

static struct kobject *gps_ant_kobj;

extern u8 gps_ant_read_gps_ant_open( void );
extern u8 gps_ant_read_gps_ant_short( void );

/**
 * @brief         Sysfs interface for GPIO data "/GPS_ANT_OPEN" show.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr Device attribute structure.
 * @param[out]    *buf Show gpio data.
 * @return        Size of buf.
 */
static ssize_t gps_ant_open_show( struct device *dev,
	struct device_attribute *attr, char *buf )
{
	int count;
	unsigned char data;

	GPS_ANT_INFO_LOG( "start.\n" );

	data = gps_ant_read_gps_ant_open();
	count = sprintf(buf, "%d\n", data);

	GPS_ANT_DEBUG_LOG( "end. data = %d, size = %d\n", data, count );

	return count;
};

/**
 * @brief         Sysfs interface for GPIO data "/GPS_ANT_SHORT" show.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr Device attribute structure.
 * @param[out]    *buf Show gpio data.
 * @return        Size of buf.
 */
static ssize_t gps_ant_short_show( struct device *dev,
	struct device_attribute *attr, char *buf )
{
	int count;
	unsigned char data;

	GPS_ANT_INFO_LOG( "start.\n" );

	data = gps_ant_read_gps_ant_short();
	count = sprintf(buf, "%d\n", data);

	GPS_ANT_DEBUG_LOG( "end. data = %d, size = %d\n", data, count );

	return count;
};

static DEVICE_ATTR( open_det, 0664, gps_ant_open_show, NULL );
static DEVICE_ATTR( short_det, 0664, gps_ant_short_show, NULL );

static struct attribute *gps_ant_attrs[] = {
	&dev_attr_open_det.attr,
	&dev_attr_short_det.attr,
	NULL
};

static const struct attribute_group gps_ant_attr_group = {
	.attrs = gps_ant_attrs,
};

/**
 * @brief         GPS Antenna detect driver initialize function.
 * @param         None.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -ENOMEM No memory.
 */
static int __init gps_ant_init(void)
{
	int ret = 0;

	GPS_ANT_INFO_LOG( "start.\n" );

	gps_ant_kobj = kobject_create_and_add( "gps_ant", kernel_kobj );
	if( !( gps_ant_kobj) ) {
		GPS_ANT_ERR_LOG( "Failure gps_ant creating kobject\n" );
		goto err_kobject_create_and_add;
		ret = -ENOMEM;
	}

	ret = sysfs_create_group( gps_ant_kobj, &gps_ant_attr_group);
	if( ret ) {
		GPS_ANT_ERR_LOG( "Failure %d creating sysfs group\n", ret );
		goto err_sysfs_create_group;
	}

	/* FUJITSU TEN:2012-12-17 log edit for notice. start */
	GPS_ANT_NOTICE_LOG( "GPS Antenna chip initialize finished.\n" );
	/* FUJITSU TEN:2012-12-17 log edit for notice. end */

	GPS_ANT_DEBUG_LOG( "end.\n" );

	return 0;

err_sysfs_create_group:
	kobject_put( gps_ant_kobj );

err_kobject_create_and_add:
	GPS_ANT_DEBUG_LOG( "end. ret = %d\n", ret );

	return ret;
}
module_init( gps_ant_init );

/**
 * @brief         GPS Antenna detect driver exit function.
 * @param         None.
 * @return        None.
 */
static void __exit gps_ant_exit(void)
{
	GPS_ANT_INFO_LOG( "start.\n" );

	kobject_put( gps_ant_kobj );

	GPS_ANT_DEBUG_LOG( "end.\n" );
}
module_exit( gps_ant_exit );

MODULE_DESCRIPTION("GPS Antenna detect driver");
MODULE_AUTHOR("");
MODULE_LICENSE("GPL");
