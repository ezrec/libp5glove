#include <stdlib.h>
#include <time.h>

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
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, (GLfloat) w/ (GLfloat) h, 0.01 , 1000.0);
}

static GLfloat ambient[4]={0.2, 0.2, 0.2, 1.0};
static GLfloat diffuse[4]={0.8, 0.8, 0.8, 1.0};
static GLfloat specular[4]={0.0, 0.0, 0.0, 1.0};
static GLfloat emission[4]={0.0, 0.0, 0.0, 1.0};

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
	 glPopMatrix();

	 gluDeleteQuadric(obj);
}

static P5Glove glove=NULL;
static struct p5glove_data info;

static GLfloat ambient_ir[4]={0.0, 0.2, 0.2, 1.0};
static GLfloat diffuse_ir[4]={0.0, 0.8, 0.8, 1.0};

void render_obj_ir(void)
{
	int i;
	GLUquadricObj *obj;
	 int mode=GL_FRONT;

	glMaterialfv(mode,GL_AMBIENT,ambient_ir);
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);

	for (i=0; i < 8; i++) {

		if (!info.ir[i].visible)
			continue;

		obj=gluNewQuadric();

		glPushMatrix();
		glTranslatef(-info.ir[i].x,-info.ir[i].y,-info.ir[i].z);
		gluQuadricDrawStyle(obj,GLU_SMOOTH);
		gluSphere(obj,4,10,10);
		glPopMatrix();

		gluDeleteQuadric(obj);
	}
}

void render_display(void)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(      0.0, 0.0, -WORLD_SIZE+0.1, /* Eye */
			0.0, 0.0, 100.0, /* Horizon */
			0, 1.0, 0);    /* Up */

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

	if (err == 0)
		glutPostRedisplay();
}


void render_keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 'q':
		case 27:
			exit(0);
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
