/*
 * $Id$
 *
 *  Copyright (c) 2003 Jason McMullan <ezrec@hotmail.com>
 *  Windows patch (c) 2004 Ross Bencina <rossb@audiomulch.com>
 *  MacOSX patch (c) 2004 Tim Kreger <tkreger@bigpond.net.au>
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
 
#if !defined(__MACOSX__) && defined(__APPLE__) && defined(__MACH__)
#define __MACOSX__	1
#endif 

#include <stdio.h>
#include <stdlib.h> /* for calloc */
#include <stdint.h>
#include <string.h> /* for memcpy */
#include <math.h>

#if defined(__WIN32__)
#include "win32_usb_hid.h"
#elif defined(__MACOSX__)
#include "macosx_usb_hid.h"
#else
#include <usb.h>
#endif

#include <errno.h>
#include "p5glove.h"

static uint8_t report_12[] = {
	0x0c, 0x00, 
	0xff, 0x01, 0x00, 0x0c, 0x00, 0xad, 
	0xff, 0x0d, 0xff, 0xe5, 0xff, 0xe4, 
	0xff, 0x50, 0x00, 0x1e, 0xff, 0xcf, 
	0xff, 0xdc, 0x00, 0x66, 0xff, 0xf6, 
	0x00, 0xa9, 0xff, 0xef, 0xff, 0xa5, 
	0x00, 0xcc, 0x00, 0x03, 0x00, 0xd6, 
	0xfe, 0xea, 0xff, 0xc1, 0x00, 0x9b, 
	0x00, 0xc6, 0xff, 0xb8, 0x00, 0xd9,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

struct p5glove {
	unsigned char data[24],later[24];
	char name[128];

	/* Calibration data from Report 12 & 6 */
	struct p5glove_cal {
		int	id;	/* Device id */
		struct {
			double	v;	/* in Radians */
			double	h;	/* in Radians */
		} head[2];
		double	head_dist;	/* in Meters */
		double  led[10][3];
	} cal;

#if defined(__WIN32__) || defined(__MACOSX__)
	USBHIDHandle *usb;
#else
	struct usb_dev_handle *usb;
	long long nextsamp;
#endif
};

static uint32_t get_bits(uint8_t *data,int pos,int len)
{
	int index = (pos>>3);
	uint32_t value = 0;

	pos &= 0x7;
	if (pos != 0) {
		value |= (data[index] & (0xff >> pos)) << (len-(8-pos));
		len -= (8-pos);
		index++;
	}

	for (; len >= 8; len-=8) {
		value |= data[index] << (len-8);
		index++;
	}

	if (len > 0)
		value |= (data[index] & (0xff << (8-len))) >> (8-len);

	return value;
}

static int32_t get_bits_signed(uint8_t *data,int pos,int len)
{
	uint32_t value,mask=0;

	value = get_bits(data,pos,len);
	if (value & (1 << (len-1)))
		mask = 0xffffffff << (len - 32);
	return value | mask;
}

static void unpack_sample(struct p5glove *p5, struct p5glove_data *info)
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

#if !defined(__WIN32__) && !defined(__MACOSX__)
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

		info->finger[i]=get_bits(data,8+6*i,6);
	}

	info->buttons=get_bits(data,40,4);

	/* Clear visibility */
	for (i=0 ; i <8; i++)
		info->ir[i].visible=0;

	for (i=0; i < 4; i++) {
		int axis;
		int value;

		axis = get_bits(data,44+4*i,4);
		if (axis & 0x8)
			continue;

		info->ir[axis].v2=get_bits_signed(data,60+(i*30),10);
		info->ir[axis].v1=get_bits_signed(data,60+(i*30)+10,10);
		info->ir[axis].h=get_bits_signed(data,60+(i*30)+20,10);
		info->ir[axis].visible=i+1;
	}
}

void p5glove_process_sample(P5Glove p5, struct p5glove_data *info)
{
	int i;
	int best_led = -1;
	double v1,v2,h,d,zprime;

	/* Get the position from the 'best led' */

	for (i = 0; i < 8; i++)
		if (info->ir[i].visible == 1)
			best_led = i;

	/* Hmm. No 'best' led. 
	 */
	if (best_led < 0)
		return;

	v1 = atan(info->ir[best_led].v1/512.0)+p5->cal.head[0].v;
	v2 = atan(info->ir[best_led].v2/512.0)+p5->cal.head[1].v;
	h = atan(info->ir[best_led].h/512.0);
	d = p5->cal.head_dist;

	zprime = d/(tan(v1)-tan(v2));

	info->position[0]=sin(h)*zprime;
	info->position[1]=tan(v1)*zprime-d/2.0;	/* Places 0 in the center */
	info->position[2]=-(cos(h)*zprime-d*2.0); /* A reasonable Z center */
}

#ifdef __linux__
#define USB_TIMEOUT 1000

int usb_get_report(struct usb_dev_handle *udev,int ReportId, unsigned char *raw_buf, int ReportSize )
{
	return usb_control_msg(udev, 
                         USB_ENDPOINT_IN + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                         0x01, /* HID_REPORT_GET */
                         ReportId+(0x03<<8), /* HID_REPORT_TYPE_FEATURE */
                         0, raw_buf, ReportSize, USB_TIMEOUT);
}


int usb_set_report(struct usb_dev_handle *udev,int ReportId, unsigned char *raw_buf, int ReportSize )
{
	return usb_control_msg(udev, 
                         USB_ENDPOINT_OUT + USB_TYPE_CLASS + USB_RECIP_INTERFACE,
                         0x09, /* HID_REPORT_SET = 0x09*/
                         ReportId+(0x03<<8), /* HID_REPORT_TYPE_FEATURE */
                         0, raw_buf, ReportSize, USB_TIMEOUT);
}
#endif

static int p5glove_parse_report6(struct p5glove *p5,int8_t *buff)
{
	/* We are given 32nds of a degree, so: */
#define REPORT6_TO_RAD(x)	((x)/32.0*M_PI/180.0)
	p5->cal.head[0].v += REPORT6_TO_RAD(buff[1]);
	p5->cal.head[0].h += REPORT6_TO_RAD(buff[2]);
	p5->cal.head[1].v += REPORT6_TO_RAD(buff[3]);
	p5->cal.head[1].h += REPORT6_TO_RAD(buff[4]);

	/* The following is in 10ths of an inch. Convert to meters */
#define REPORT6_TO_METERS(x)	((x)/393.70039)
	p5->cal.head_dist = REPORT6_TO_METERS(buff[5]);

	return 0;
}

static int p5glove_parse_report12(struct p5glove *p5,uint8_t *buff)
{
	int led,axis;
	int16_t val[3];

	p5->cal.id = get_bits(buff,8,8);
	switch (p5->cal.id) {
		case 0:
			p5->cal.head[0].v +=  17.0 * (M_PI/180.0);
			p5->cal.head[1].v += -10.0 * (M_PI/180.0);
			break;
		default: fprintf(stderr,"libp5glove: unknown glove type %d\n",p5->cal.id);
			 return -EINVAL;
	}
	for (led = 0; led < 10; led++) {
		for (axis = 0; axis < 3; axis++) {
			val[axis] = get_bits(buff,16+(48*led)+(16*axis),16);
			p5->cal.led[led][axis]=val[axis]/100.0;
		}
	}

	return 0;
}

static int p5glove_calibrate(struct p5glove *p5)
{
	int8_t report6_buff[6];
	int8_t report12_buff[255];
	int err;

	memset(&p5->cal,0,sizeof(p5->cal));
	err=usb_get_report(p5->usb,12,report12_buff,sizeof(report12_buff));
	if (err < 0) goto end;

	err=p5glove_parse_report12(p5,report12_buff);
	if (err < 0) goto end;

	err=usb_get_report(p5->usb,6,report6_buff,sizeof(report6_buff));
	if (err < 0) goto end;

	err=p5glove_parse_report6(p5,report6_buff);

#ifdef DEBUG_CALIB
printf("Cal results:\n");
{ int i; for (i=0;i<2;i++) {
printf("\tHead %d.v = %.2f deg\n",i,p5->cal.head[i].v * 180.0/M_PI);
printf("\tHead %d.h = %.2f deg\n",i,p5->cal.head[i].h * 180.0/M_PI);} }
printf("\tHead Distance = %.2f meters\n",p5->cal.head_dist);
#endif

end:
	return err;
}

P5Glove p5glove_open(void)
{
	struct p5glove *p5 = NULL;
#if defined(__WIN32__) || defined(__MACOSX__)
	USBHIDHandle *usb = OpenUSBHID (
            0,                                                  /* 0th matching device */
            0x0d7f,                                             /* vendor id */
            0x0100,                                             /* product id */
            0,                                                  /* version number (not used) */
            SELECT_VENDOR_ID_FLAG | SELECT_PRODUCT_ID_FLAG );   /* selection flags */
	if ( usb == INVALID_USBHIDHANDLE_VALUE )
		usb = NULL;
#else
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_dev_handle *usb = NULL;
	int err;

	usb_init();

	usb_find_busses();
	usb_find_devices();

	for (bus = usb_get_busses(); bus != NULL; bus = bus->next) {
		for (dev = bus->devices; dev != NULL; dev = dev->next) {
			if (dev->descriptor.idVendor != 0x0d7f ||
			    dev->descriptor.idProduct != 0x0100)
				continue;

			printf("Found P5 device at %s/%s\n",bus->dirname,dev->filename);
			usb = usb_open(dev);
			if (usb == NULL)
				continue;

			err=usb_claim_interface(usb,1);
			if (err < 0) {
				fprintf(stderr,"Can't claim P5 glove interface: %s\n",strerror(errno));
				usb_close(usb);
				return NULL;
			}

			break;
		}
		if (usb != NULL)
			break;
	}
#endif
	if (usb != NULL) {
		int err;

		p5=calloc(1,sizeof(*p5));
		p5->usb=usb;
		err = p5glove_calibrate(p5);
		if (err < 0) {
			free(p5);
			return NULL;
		}
	}

	return p5;
}

void p5glove_close(P5Glove p5)
{
	if (p5->usb != NULL)
#if defined(__WIN32__) || defined(__MACOSX__)
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
	
#if defined(__WIN32__) || defined(__MACOSX__)
    if( ReadUSBHID( p5->usb, p5->data, 24 ) == 24 ){
        unpack_sample(p5, info);
        err = 0;
    }else{
        errno = EACCES;
        err = -1;
    }

#else
	err=usb_interrupt_read(p5->usb, 0x81, p5->data, 24, 2000);
	if (err == 24) { 
		unpack_sample(p5, info);
		err=0;
	} else if (err < 0 && errno == 84)
		err=0;
	else if (err < 0 && errno == 19) {
		fprintf(stderr,"Device reset. Crud.\n");
		usb_resetep(p5->usb, 0x81);
	}
#endif   
    
	return err;
}

void p5glove_begin_calibration(P5Glove p5)
{
#ifdef __WIN32__
    char report[2] = { 0x01, 0x01 };
    SetUSBHIDFeature( p5->usb, report, 2 );
#endif
}

void p5glove_end_calibration(P5Glove p5)
{
#ifdef __WIN32__
    char report[2] = { 0x01, 0x00 };
    SetUSBHIDFeature( p5->usb, report, 2 );
#endif
}

int p5glove_get_mouse_mode(P5Glove p5)
{
#ifdef __WIN32__
    char report[2] = { 0x05, 0x00 };
    GetUSBHIDFeature( p5->usb, report, 2 );

    return (report[1] == 0x01)? 1 : 0;
#endif
}

void p5glove_mouse_mode_on(P5Glove p5)
{
#ifdef __WIN32__
    char report[2] = { 0x05, 0x01 };
    SetUSBHIDFeature( p5->usb, report, 2 );
#endif
}

void p5glove_mouse_mode_off(P5Glove p5)
{
#ifdef __WIN32__
    char report[2] = { 0x05, 0xFF };
    SetUSBHIDFeature( p5->usb, report, 2 );
#endif
}

