/*
 * $Id$
 *
 *  Copyright (c) 2003 Jason McMullan <ezrec@hotmail.com>
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

#ifndef P5GLOVE_H
#define P5GLOVE_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct p5glove *P5Glove;

/* Button bitmasks */
#define P5GLOVE_BUTTON_A	1
#define P5GLOVE_BUTTON_B	2
#define P5GLOVE_BUTTON_C	4

/* IR Sensor Index */
#define P5GLOVE_IR_WRIST_TOP	0
#define P5GLOVE_IR_PINKY_R	1
#define P5GLOVE_IR_PINKY_L	2
#define P5GLOVE_IR_PALM		3
#define P5GLOVE_IR_INDEX	4
#define P5GLOVE_IR_THUMB_TOP	5
#define P5GLOVE_IR_WRIST_BOT	6
#define P5GLOVE_IR_THUMB_BOT	7

/* Finger Sensor Index */
#define P5GLOVE_INDEX	0
#define P5GLOVE_MIDDLE	1
#define P5GLOVE_RING	2
#define P5GLOVE_PINKY	3
#define P5GLOVE_THUMB	4

struct p5glove_data {
	int buttons;	/* Button bitmask */
	int finger[5];	/* Finger clench values (0-63) */
	/* Raw glove data */
	struct p5glove_ir {
		int visible;	/* Was the sensor visible? */
		int v1,v2,h;
	} ir[8];	/* IR Sensors values.  (-511 - 511) */

	/* Computed from p5glove_process_sample 
	 */
	double position[3];	/* Position */
	double ref_normal[3];	/* Position */
	double normal[3];	/* Position */
	double position_next[3];	/* Position */
	double position_last[3];	/* Position */
	int position_led[3];

	/* Rotation information */
	struct {
		double axis[3];	/* Rotation axis (normalized) */
		double angle;	/* In degrees */
	} rotation;
};


/* p5glove_open:
 * Open a handle to a P5 Glove. Returns NULL on error,
 * and sets errno appropriately.
 */
P5Glove p5glove_open(void);


/* p5glove_close:
 * Close an open handle to a P5 Glove.
 */
void p5glove_close(P5Glove glove);


/* p5glove_sample
 * Retrieve a sample from the P5
 * Returns 0 on success, -1 on error, and sets 
 * errno to EAGAIN is called faster then the refresh frequency.
 */
int p5glove_sample(P5Glove glove, struct p5glove_data *data);


/* p5glove_process_sample
 * Clean up raw glove data returned by p5glove_sample
 * Eliminates erroneous values, linearizes coordinates, etc.
 *
 * Returns:
 * 	 -ENOENT if no LED found
 * 	 0	position (at least 1 led)
 * 	 1	position & rotation (at least 3 leds)
 */
int p5glove_process_sample(P5Glove glove, struct p5glove_data *data);

int p5glove_reference_led(P5Glove glove,int led,double pos[3]);

void p5glove_begin_calibration(P5Glove glove);
void p5glove_end_calibration(P5Glove glove);

int p5glove_get_mouse_mode(P5Glove glove);
void p5glove_mouse_mode_on(P5Glove glove);
void p5glove_mouse_mode_off(P5Glove glove);

/* Inline helpers */

/* Distance between two points */
static inline double p5glove_dist(const double a[3],const double b[3])
{
	double w = 0.0;
	int i;

	for (i = 0; i < 3; i++)
		w += (a[i]-b[i])*(a[i]-b[i]);

	return sqrt(w);
}

/* Cross product of two vectors */
static inline void p5glove_cross(const double v1[3],const double v2[3],double res[3])
{
	res[0]=v1[1]*v2[2]-v1[2]*v2[1];
	res[1]=v1[2]*v2[0]-v1[0]*v2[2];
	res[2]=v1[0]*v2[1]-v1[1]*v2[0];
}

/* Optimized for our use. We know the last row and last column are all zeros,
 * except for [3][3]=1.
 */
static inline void p5glove_vec_mat(const double vec[3],double mat[4][4],double res[3])
{
	int r,c;

	for (r=0; r < 3; r++) for (c=0; c < 3; c++)
		res[r] += vec[c]*mat[c][r];
}

static inline double p5glove_dot(const double a[3],const double b[3])
{
	double res = 0.0;
	int i;

	for (i=0;i < 3; i++)
		res += a[i]*b[i];

	return res;
}

/* This is safe to call with v==res
 */
static inline void p5glove_normalize(const double v[3],double res[3])
{
	double tmp=0.0;
	int i;

	for (i=0; i < 3; i++) {
		tmp += v[i]*v[i];
	}

	tmp=sqrt(tmp);

	for (i=0; i < 3; i++)
		res[i] = v[i] / tmp;
}

/* Normal of three points */
static inline void p5glove_normal(const double p1[3],const double p2[3],const double p3[3],double xyz[3])
{
	double v1[3],v2[3];

	v1[0]=p1[0]-p2[0];
	v1[1]=p1[1]-p2[1];
	v1[2]=p1[2]-p2[2];

	v2[0]=p3[0]-p2[0];
	v2[1]=p3[1]-p2[1];
	v2[2]=p3[2]-p2[2];

	p5glove_cross(v1,v2,xyz);

	p5glove_normalize(xyz,xyz);
}

/* Angle of three points */
static inline double p5glove_angle(const double p1[3],const double p2[3],const double p3[3])
{
	double v1[3],v2[3];

	v1[0]=p1[0]-p2[0];
	v1[1]=p1[1]-p2[1];
	v1[2]=p1[2]-p2[2];

	v2[0]=p3[0]-p2[0];
	v2[1]=p3[1]-p2[1];
	v2[2]=p3[2]-p2[2];

	p5glove_normalize(v1,v1);
	p5glove_normalize(v2,v2);

	return acos(p5glove_dot(v1,v2));
}


#ifdef __cplusplus
}
#endif

#endif /* P5GLOVE_H */
