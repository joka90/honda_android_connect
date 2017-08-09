/*
 * Copyright(C) 2012 FUJITSU TEN LIMITED
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

/*
 *	Display color driver APIdefine
 */

/*
 *	IOCTL Request code
 */
enum DISPLAY_COLOR_DRV_REQ_CODE {
	DISP_COLOR_REQ_SET_CSC,
	DISP_COLOR_REQ_SET_PAL
};

/*
 *	Parameter structure
 */
struct DISP_COLOR_PALETTE_PARAM {
	unsigned char	red  [256];
	unsigned char	green[256];
	unsigned char	blue [256];
};

struct DISP_COLOR_CSC_PARAM {
	unsigned short	yof;		// 7.0
	unsigned short	kyrgb;		// 2.8
	unsigned short	kur;		// s2.8
	unsigned short	kvr;		// s2.8
	unsigned short	kug;		// s1.8
	unsigned short	kvg;		// s1.8
	unsigned short	kub;		// s2.8
	unsigned short	kvb;		// s2.8
};

