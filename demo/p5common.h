#ifndef P5COMMON_H
#define P5COMMON_H

#include <p5glove.h>

double p5distance(struct p5glove_data *info,int a,int b);
/* Angle is in degrees (for OpenGL) */
double p5angle(struct p5glove_data *info,int a,int b,int c);
void p5surface_normal(struct p5glove_data *info,int a,int b,int c,double *xyz);
int p5normal(struct p5glove_data *info,double *plane);

#endif /* P5COMMON_H */
