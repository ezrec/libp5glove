#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <p5glove.h>

#include "p5common.h"

double p5distance(struct p5glove_data *info,int a,int b)
{
	double tmp,dist;

	tmp=(info->ir[a].x-info->ir[b].x);
	dist = tmp*tmp;

	tmp=(info->ir[a].y-info->ir[b].y);
	dist += tmp*tmp;

	tmp=(info->ir[a].z-info->ir[b].z);
	dist += tmp*tmp;

	return sqrt(dist);
}

double p5angle(struct p5glove_data *info,int a,int b,int c)
{
	double v1[4],v2[4],mult;
	int i;

	v1[0]=(info->ir[a].x-info->ir[b].x);
	v1[1]=(info->ir[a].y-info->ir[b].y);
	v1[2]=(info->ir[a].z-info->ir[b].z);
	v1[3]=sqrt(v1[0]*v1[0]+v1[1]*v1[1]+v1[2]*v1[2]);

	v2[0]=(info->ir[c].x-info->ir[b].x);
	v2[1]=(info->ir[c].y-info->ir[b].y);
	v2[2]=(info->ir[c].z-info->ir[b].z);
	v2[3]=sqrt(v2[0]*v2[0]+v2[1]*v2[1]+v2[2]*v2[2]);

	mult=0.0;
	for (i=0; i < 3; i++)
		mult += v1[i]*v2[i];

	return acos(mult/(v1[3]*v2[3]))*180.0/M_PI;
}


void p5surface_normal(struct p5glove_data *info,int a,int b,int c,double *xyz)
{
	double v1[3],v2[3],tmp;
	int i;

	v1[0]=info->ir[a].x-info->ir[b].x;
	v1[1]=info->ir[a].y-info->ir[b].y;
	v1[2]=info->ir[a].z-info->ir[b].z;

	v2[0]=info->ir[c].x-info->ir[b].x;
	v2[1]=info->ir[c].y-info->ir[b].y;
	v2[2]=info->ir[c].z-info->ir[b].z;

	xyz[0]=v1[1]*v2[2]-v1[2]*v2[1];
	xyz[1]=v1[2]*v2[0]-v1[0]*v2[2];
	xyz[2]=v1[0]*v2[1]-v1[1]*v2[0];

	tmp=0;
	for (i=0; i < 3; i++) {
		tmp += xyz[i]*xyz[i];
	}

	tmp=sqrt(tmp);

	for (i=0; i < 3; i++)
		xyz[i] /= tmp;
}

/* Snarked:
 * FROM: Doug Rogers
 * DATE: 06/06/2002 15:52:33
 * SUBJECT: RE:  [Algorithms] plane fitting to points
 * LIST: gdalgorithms-list@sourceforge.net
 */
int p5normal(struct p5glove_data *info,double *plane)
{
    double  D = 0;
    double  E = 0;
    double  F = 0;
    double  G = 0;
    double  H = 0;
    double  I = 0;
    double  J = 0;
    double  K = 0;
    double  L = 0;
    double  error = 0;
    double  denom = 0;
    double  u;
    int i;

    for (i=0; i < 8; i++ ) {
	struct p5glove_ir *point=&info->ir[i];
	if (!point->visible)
		continue;

        D += point->x * point->x;
        E += point->x * point->y;
        F += point->x;
        G += point->y * point->y;
        H += point->y;
        I += 1;
        J += point->x * point->z;
        K += point->y * point->z;
        L += point->z;
    }

    denom = F*F*G - 2*E*F*H + D*H*H + E*E*I - D*G*I;

    /* X axis slope */
    plane[0] = (H*H*J - G*I*J + E*I*K + F*G*L - H*(F*K + E*L)) / denom;
    /* Y axis slope */
    plane[1] = (E*I*J + F*F*K - D*I*K + D*H*L - F*(H*J + E*L)) / denom;
    /* Z axis intercept */
    plane[3] = (F*G*J - E*H*J - E*F*K + D*H*K + E*E*L - D*G*L) / denom;

    plane[2] = -1;

    /* normalize a b c  */
    u = sqrt (plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    if (u == 0.0)
        return -1;

    if (u == 1.0)
        return 0;

    plane[0] /= u;
    plane[1] /= u;
    plane[2] /= u;

    return 0;
}

