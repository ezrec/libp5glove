#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "p5glove.h"

#define WORLD_SIZE 500

void render_init(void)
{
	GLfloat fogColor[4]= {0.7, 0.7, 0.7, 1.0};
	GLfloat light_pos[4]={0.0, WORLD_SIZE, 0.0, 1.0};

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	/* Lighting
	 */
	glLightfv(GL_LIGHT0,GL_POSITION,light_pos);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);
}

void  render_reshape(int w,int h)
{
	int oldmat;

	/* Get old matrix mode */
	glGetIntegerv(GL_MATRIX_MODE,&oldmat);

	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, (GLfloat) w/ (GLfloat) h, 0.01 , 1000.0);
//	glOrtho(-WORLD_SIZE,WORLD_SIZE,WORLD_SIZE,-WORLD_SIZE,0.01,1000.0);

	/* Restore old matrix mode */
	glMatrixMode(oldmat);
}

static GLfloat ambient[4]={0.2, 0.2, 0.2, 1.0};
static GLfloat diffuse[4]={0.8, 0.8, 0.8, 1.0};
static GLfloat specular[4]={0.0, 0.0, 0.0, 1.0};
static GLfloat emission[4]={0.0, 0.0, 0.0, 1.0};

void render_cylinder(GLdouble diameter,GLdouble *p1,GLdouble *p2)
{
	GLdouble vx,vy,vz,dist,ax,rx,ry;
	GLUquadricObj *obj;

	vx=p2[0]-p1[0];
	vy=p2[1]-p1[1];
	vz=p2[2]-p1[2];

	dist=sqrt(vx*vx+vy*vy+vz*vz);

	ax=180.0/M_PI * acos(vz/dist);
	if (vz < 0.0) ax = -ax;

	rx=-vy*vz;
	ry=vx*vz;

	obj=gluNewQuadric();

	/* Fun fun. Draw a cylinder between two points.
	 */
	glPushMatrix();
	glTranslated(p1[0],p1[1],p1[2]);
	glRotated(ax, rx, ry, 0.0);
	gluQuadricDrawStyle(obj,GLU_SMOOTH);
	gluQuadricOrientation(obj,GLU_OUTSIDE);
	gluCylinder(obj,4,4,dist,10,1);
	glPopMatrix();

	gluDeleteQuadric(obj);
}

void render_obj_world(void)
{
	 GLUquadricObj *obj;
	 int mode=GL_FRONT;
	 

	 obj=gluNewQuadric();
	 glMaterialfv(mode,GL_AMBIENT,ambient);
	 glMaterialfv(mode,GL_DIFFUSE,diffuse);
	 glMaterialfv(mode,GL_SPECULAR,specular);
	 glMaterialf(mode,GL_SHININESS,0.0);
	 glMaterialfv(mode,GL_EMISSION,emission);

	 glPushMatrix();
	 glTranslatef(0.0,0.0,0.0);
	 gluQuadricDrawStyle(obj,GLU_SMOOTH);
	 gluQuadricOrientation(obj,GLU_INSIDE);
	 gluSphere(obj,WORLD_SIZE,10,10);

//	 glutSolidCube(WORLD_SIZE/5.0);
	 glPopMatrix();

	 gluDeleteQuadric(obj);
}

static P5Glove glove=NULL;
static struct p5glove_data info;

static GLfloat ambient_ir[4]={0.0, 0.2, 0.2, 1.0};
static GLfloat diffuse_ir[4]={0.0, 0.8, 0.8, 1.0};

void render_obj_ir(void)
{
	int i,j;
	GLUquadricObj *obj;
	int mode=GL_FRONT;
	double plane[4];

	glMaterialfv(mode,GL_AMBIENT,ambient_ir);

	for (i=0; i < 8; i++) {

		if (!info.ir[i].visible)
			continue;

		diffuse_ir[0]=1.0*(i&1);
		diffuse_ir[1]=1.0*((i>>1)&1);
		diffuse_ir[2]=1.0*((i>>2)&1);

		glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);

		obj=gluNewQuadric();

		glPushMatrix();
		glTranslatef(info.ir[i].x,info.ir[i].y,info.ir[i].z);
		gluQuadricDrawStyle(obj,GLU_SMOOTH);
	 	gluQuadricOrientation(obj,GLU_OUTSIDE);
		gluSphere(obj,20,10,10);
		glPopMatrix();

		gluDeleteQuadric(obj);
	}

	/* Compute the normal to the pointset
	 */
	p5normal(&info,plane);

	for (i=0; i < 8; i++) {
		GLdouble p1[3],p2[3];

		if (!info.ir[i].visible)
			continue;

		p1[0]=info.ir[i].x;
		p1[1]=info.ir[i].y;
		p1[2]=info.ir[i].z;

		for (j=i+1; j < 8; j++) {
			if (!info.ir[j].visible)
				continue;

			p2[0]=info.ir[j].x;
			p2[1]=info.ir[j].y;
			p2[2]=info.ir[j].z;

			render_cylinder(4,p1,p2);
		}

		for (j=0; j < 3; j++)
			p2[j]=p1[j]+plane[j]*50;

		render_cylinder(4,p1,p2);
	}

}

static double yaw=0.0,tilt=0.0;

void render_display(void)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(      0.0, 0.0, -WORLD_SIZE*0.95, /* Eye */
			0.0, 0.0, 100000.0, /* Horizon */
			0.0, 1.0, 0.0);    /* Up */

	glRotated(tilt,0.0, 0.1, 0.0);
	glRotated(yaw,0.0, 0.0, 1.0);

	/* Render world */
	render_obj_world();

	/* Draw samples here */
	render_obj_ir();

	glutSwapBuffers();
}

void render_next(void)
{
	int err;

	/* Get samples here */
	err=p5glove_sample(glove, &info);

	if (err == 0) {
		/* Point Z the other way */
		int i;
		for (i=0; i < 8; i++)
			if (info.ir[i].visible)
				info.ir[i].z *= -1.0;
		glutPostRedisplay();
	}
}


void render_keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 'q':
		case 27:
			exit(0);
			break;
		case 'a':
			yaw += 1.0;
			break;
		case 'd':
			yaw -= 1.0;
			break;
		case 'w':
			tilt += 1.0;
			break;
		case 's':
			tilt -= 1.0;
			break;
		default:
			break;
	}
}

int main(int argc,char **argv)
{
	glutInit(&argc,argv);

	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
	glutInitWindowSize(200,200);
	glutCreateWindow(argv[0]);

	render_init();

	memset(&info,0,sizeof(info));

	glove=p5glove_open();
	if (glove==NULL) {
		perror(argv[0]);
		exit(1);
	}

	glutDisplayFunc(render_display);
	glutIdleFunc(render_next);
	glutKeyboardFunc(render_keyboard);
	glutReshapeFunc(render_reshape);
	glutMainLoop();
}
