/*  ----------------------------------------------------------------------

    Copyright (C) 2012 Joe Wei, HTC

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */

#ifndef HSML_CLIENT_H_
#define HSML_CLIENT_H_

enum {
	HSML_REQ_DIR_IN = 0,
	HSML_REQ_DIR_OUT
};

/*-------------------------------------------
    TYPEDEFS
----------------------------------------------*/
typedef struct server_info {
	unsigned short width;
	unsigned short height;
	unsigned int pixel_format;
} __attribute__ ((__packed__)) server_info_type;


struct hsml_request {
	int code;
	unsigned short value;
	unsigned short index;
	int len;
	unsigned char dir;			/* Direction of the request */
#define MAX_DATA_SIZE 512
	unsigned char data[MAX_DATA_SIZE];
} __attribute__ ((__packed__));


struct hsml_header {
#define SIGNATURE_SIZE 8
	unsigned char signature[SIGNATURE_SIZE];
	unsigned short seq;
	unsigned int timestamp;
	unsigned short num_context_info;
	unsigned char reserved[496];
} __attribute__ ((__packed__));

/* A-DA Customize */
#define ADA_CUSTOMIZE
#define ADA_CUSTOMIZE_DEBUG

/*----------------------------------------------------------------
  IOCTL command defines
  -----------------------------------------------------------------*/
/* magic number 1 of the sep IOCTL command */
#define HSMLCLIENT_IOC_MAGIC_NUMBER                           's'

/* Get front frame buffer index */
#define HSMLCLIENT_IOC_READ_FRONT_FB_INDEX             	_IOR(HSMLCLIENT_IOC_MAGIC_NUMBER , 1, int)

/* Tell the driver the server framebuffer info */
#define HSMLCLIENT_IOC_WRITE_SERVER_FB_INFO     		_IOW(HSMLCLIENT_IOC_MAGIC_NUMBER , 2, server_info_type)

/* Transfer HSML specific requests */
#define HSMLCLIENT_IOC_PROTOCOL_TRANSFER		  		_IOWR(HSMLCLIENT_IOC_MAGIC_NUMBER, 3, void *)


#ifdef ADA_CUSTOMIZE
#define HSML_DEVICE_NAME			"hsml_client"
#define HSML_DRIVER_NAME			"hsml_client"
#define HSML_CLASS_NAME				"hsml_client"

#define HSML_PIXEL_FORMAT_RGB565	(1 << 0)
#define HSML_PIXEL_FORMAT_RGB888	(2 << 0)
#endif /* ADA_CUSTOMIZE */

enum {
	HSML_REQ_GET_SERVER_VERSION = 0x40,
	HSML_REQ_GET_NUM_COMPRESSION_SETTINGS,
	HSML_REQ_GET_SERVER_CONF,
	HSML_REQ_SET_SERVER_CONF,
	HSML_REQ_GET_FB,
	HSML_REQ_STOP,
	HSML_REQ_GET_SERVER_DISP,
	HSML_REQ_SET_SERVER_DISP,
	HSML_REQ_SET_MAX_FRAME_RATE,
#ifdef ADA_CUSTOMIZE
	HSML_REQ_TOUCH_EVENT,
#endif /* ADA_CUSTOMIZE */
	HSML_REQ_COUNT
};


#endif /*HSML_CLIENT_H_*/
