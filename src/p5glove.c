/*
 * $Id$
 *
 *  Copyright (c) 2003 Jason McMullan <ezrec@hotmail.com>
 *  Windows patch (c) 2004 Ross Bencina <rossb@audiomulch.com>
 *
 *  USB P5 Data Glove support
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h> /* for calloc */
#include <string.h> /* for memcpy */
#ifdef __WIN32__
#include "win32_usb_hid.h"
#else
#include <usb.h>
#endif
#include <errno.h>
#include "p5glove.h"

struct p5glove {
	unsigned char data[24],later[24];
	char name[128];
#ifdef __WIN32__
    USBHIDHandle *usb;
#else
	struct usb_dev_handle *usb;
	long long nextsamp;
#endif
};

static void process_sample(struct p5glove *p5, struct p5glove_data *info)
{
	unsigned char *data = p5->data, *later=p5->later;
	unsigned char tmp[24];
	int i;

	/* Decode data.
	 * Format (nibbles from LSB to MSB)
	 *                 11111111111111112222222222222222
	 * 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
	 * 01ddddddddBCCCCVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVxxx
	 *  0 1 2 3 4 5 6 7 8 9 A B C D E F 0 1 2 3 4 5 6 7
	 *                                  1 1 1 1 1 1 1 1
	 *	  (bytes from LSB to MSB)
	 *
	 * d - Packed 6-bit bend sensor data (Index, Middle, Ring, Pinky, Thumb)
	 * B - Button data. A=1, B=2, C=4
	 * C - IR sensor report index
	 * V - Packed 30 bit signed IR info (10 bits X, 10 bits Y, 10 bits Z), x4
	 */

#ifndef __WIN32__
	if (data[16] == 1) {	/* Hmm. Offset by 16. Fix it. */
		memcpy(tmp,later,24-16);
		memcpy(later,data+16,24-16);
		memcpy(tmp+24-16,data,16);
		data=tmp;
	} else if (data[8] == 1) { /* Hmm. Offset by 8. Fix it. */
		memcpy(tmp,later,24-8);
		memcpy(later,data+8,24-8);
		memcpy(tmp+24-8,data,8);
		data=tmp;
	}
#endif 

	if (data[0] != 1) {
		return;
	}

	for (i=0; i < 5; i++) {
		int value;

		switch (i) {
			case 0: value = data[1] >> 2; break;
			case 1: value = ((data[1] & 0x3) << 4) | (data[2] >> 4); break;
			case 2: value = ((data[2] & 0xF) << 2) | (data[3] >> 6); break;
			case 3: value = (data[3] & 0x3F); break;
			case 4: value = data[4] >> 2; break;
			default: value = 0; break;
		}

		info->finger[i]=value;
	}

	info->buttons=data[5]>>4;

	/* Clear visibility */
	for (i=0 ; i <8; i++)
		info->ir[i].visible=0;

	for (i=0; i < 4; i++) {
		int axis;
		int value;

		axis = (data[5+((i+1)>>1)] >> ((i & 1) * 4)) & 0xf;
		if (axis > 7 ){
            /* axis == 15 probably means that the slot is unused */
            if( axis != 15){
                /* at this stage we're not sure why the glove returns values > 7 here, but it does */
                printf( "-------------------------------------------------------------------------------\n" );
                printf( "warning: sensor slot %d > 7 (%d)\n", i, axis );
                printf( "-------------------------------------------------------------------------------\n" );
			}
            continue;
        }

		switch (i) {
			case 0: value = ((data[0x7] & 0x0F)<<26) | (data[0x8]<<18) | (data[0x9]<<10) | (data[0xA]<<2) | (data[0x0B]>>6);
				break;
			case 1: value = ((data[0xB] & 0x3F)<<24) | (data[0xC]<<16) | (data[0xD] << 8) | data[0x0E];
				break;
			case 2: value = (data[0xF] << 22) | (data[0x10]<<14) | (data[0x11] << 6) | (data[0x12] >> 2);
				break;
			case 3: value = ((data[0x12] & 0x03)<<28) | (data[0x13] << 20) | (data[0x14] << 12) | (data[0x15] << 4) | (data[0x16] >> 4);
				break;
			default:
				/* Impossible! */
				return;
		}

#define SEX(value) ((((value) & 0x3FF) << (32-10)) >> (32-10))
		info->ir[axis].x=SEX(value);
		info->ir[axis].y=SEX(value>>10);
		info->ir[axis].z=SEX(value>>20);
		info->ir[axis].visible=1;
	}

	/* Remove any spurious data */
	for (i=0; i < 8; i++) {
		int j;

		if (!info->ir[i].visible)
			continue;
	
		for (j=0; j < 8; j++) {
			double dist,tmp;

			if (i == j)
				continue;
			if (!info->ir[j].visible)
				continue;

			tmp=info->ir[i].x-info->ir[j].x;
			dist = tmp*tmp;
			tmp=info->ir[i].y-info->ir[j].y;
			dist += tmp*tmp;
			tmp=info->ir[i].z-info->ir[j].z;
			dist += tmp*tmp;

			if (dist < (200.0*200.0))
				break;
		}

		if (j == 8)
			info->ir[i].visible=0;
	}
}

P5Glove p5glove_open(void)
{
#ifdef __WIN32__
    struct p5glove *p5;
    USBHIDHandle *usb = OpenUSBHID (
            0,                                                  /* 0th matching device */
            0x0d7f,                                             /* vendor id */
            0x0100,                                             /* product id */
            0,                                                  /* version number (not used) */
            SELECT_VENDOR_ID_FLAG | SELECT_PRODUCT_ID_FLAG );   /* selection flags */
    if( usb != INVALID_USBHIDHANDLE_VALUE ){

        p5 = calloc(1,sizeof(*p5));
        p5->usb = usb;

		return p5;
    }

#else
	struct usb_bus *bus;
	struct usb_device *dev;
	struct p5glove *p5;
	int err;

	usb_init();

	usb_find_busses();
	usb_find_devices();

	for (bus = usb_get_busses(); bus != NULL; bus = bus->next) {
		for (dev = bus->devices; dev != NULL; dev = dev->next) {
			usb_dev_handle *udev;

			if (dev->descriptor.idVendor != 0x0d7f ||
			    dev->descriptor.idProduct != 0x0100)
				continue;

			printf("Found P5 device at %s/%s\n",bus->dirname,dev->filename);
			udev = usb_open(dev);
			if (udev == NULL)
				continue;

			err=usb_claim_interface(udev,1);
			if (err < 0) {
				fprintf(stderr,"Can't claim P5 glove interface: %s\n",strerror(errno));
				usb_close(udev);
				return NULL;
			}

			p5=calloc(1,sizeof(*p5));
			p5->usb=udev;

			return p5;
		}
	}
#endif
	return NULL;
}

void p5glove_close(P5Glove p5)
{
	if (p5->usb != NULL)
#if __WIN32__
        CloseUSBHID(p5->usb);
#else
		usb_close(p5->usb);
#endif
	p5->usb=NULL;
	free(p5);
}

int p5glove_sample(P5Glove p5, struct p5glove_data *info)
{
	int err;
	
#ifdef __WIN32__
    if( ReadUSBHID( p5->usb, p5->data, 24 ) == 24 ){
        process_sample(p5, info);
        err = 0;
    }else{
        err = EACCES;
    }

#else
	long long now;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	now=tv.tv_sec*1000000 + tv.tv_usec;
	if (p5->nextsamp != 0 && now < p5->nextsamp) {
		errno = EAGAIN;
		return -1;
	}

	p5->nextsamp = now + (1000000/30);	/* 30Hz refresh rate */

	err=usb_bulk_read(p5->usb, 0x81, p5->data, 24, 2000);

	if (err == 24) { 
		process_sample(p5, info);
		err=0;
	} else if (err < 0 && errno == 84)
		err=0;
	else if (err < 0 && errno == 19) {
		fprintf(stderr,"Device reset. Crud.\n");
		exit(1);
	}
#endif   
    
	return err;
}
