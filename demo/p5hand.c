#include <stdlib.h>
#include <time.h>
#include <math.h>

#ifndef M_PI
#define M_PI (3.1415926535898)
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "p5glove.h"

#define WORLD_SIZE 5.0
#define WORLD_SCALE 10.0

const GLdouble zero[4]={0.0,0.0,0.0,1.0};
const GLdouble y_up[4]={0.0,1.0,0.0,1.0};
const GLdouble y_down[4]={0.0,-1.0,0.0,1.0};
const GLdouble x_left[4]={-1.0,0.0,0.0,1.0};
const GLdouble x_right[4]={1.0,0.0,0.0,1.0};
const GLdouble z_front[4]={0.0,0.0,-1.0,1.0};
const GLdouble z_back[4]={0.0,0.0,1.0,1.0};

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
	gluPerspective(60, (GLfloat) w/ (GLfloat) h, WORLD_SIZE*0.1 , WORLD_SIZE*2);
//	glOrtho(-WORLD_SIZE,WORLD_SIZE,WORLD_SIZE,-WORLD_SIZE,0.0,WORLD_SIZE);

	/* Restore old matrix mode */
	glMatrixMode(oldmat);
}

static GLfloat ambient[4]={0.2, 0.2, 0.2, 1.0};
static GLfloat diffuse[4]={0.8, 0.8, 0.8, 1.0};
static GLfloat specular[4]={0.0, 0.0, 0.0, 1.0};
static GLfloat emission[4]={0.0, 0.0, 0.0, 1.0};

void render_label(GLdouble size, const char *string)
{
	glPushMatrix();
	glTranslated(+size/2,size,0);
	glRotated(180,0.0,1.0,0.0);
	glScaled(0.001,0.001,0.001);
	while (*string)
		glutStrokeCharacter(GLUT_STROKE_ROMAN,*(string++));
	glPopMatrix();
}

void render_cylinder(GLdouble diameter,const GLdouble p1[3],const GLdouble p2[3],const char *label)
{
	GLdouble v[3],dist,ax,r[3];
	GLUquadricObj *obj;

	v[0]=p2[0]-p1[0];
	v[1]=p2[1]-p1[1];
	v[2]=p2[2]-p1[2];

	dist=sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
	if (dist == 0)
		return;

	v[0] /= dist;
	v[1] /= dist;
	v[2] /= dist;

	ax=180.0/M_PI * acos(p5glove_dot(z_front,v));

	if (ax != 0.0) {
		p5glove_normal(z_front,zero,v,r);

	}

	obj=gluNewQuadric();
	gluQuadricDrawStyle(obj,GLU_SMOOTH);
	gluQuadricOrientation(obj,GLU_OUTSIDE);

	/* Fun fun. Draw a cylinder between two points.
	 */
	glPushMatrix();
	glPushMatrix();
	glTranslated(p1[0],p1[1],p1[2]);
	gluSphere(obj,diameter,10,10);
	glRotated(ax, r[0], r[1], r[2]);
	gluCylinder(obj,diameter,diameter,dist,10,1);
	glPopMatrix();
	glTranslated(p2[0],p2[1],p2[2]);
	gluSphere(obj,diameter,10,10);
	render_label(diameter,label);
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
	 glPopMatrix();

	 gluDeleteQuadric(obj);
}

static P5Glove glove=NULL;
static struct p5glove_data info;

static GLfloat ambient_ir[4]={0.0, 0.2, 0.2, 1.0};
static GLfloat diffuse_ir[4]={0.0, 0.8, 0.8, 1.0};

void render_obj_leds(void)
{
	int i;
	int mode=GL_FRONT;
	GLUquadricObj *obj;

	obj=gluNewQuadric();
	gluQuadricDrawStyle(obj,GLU_SMOOTH);
	gluQuadricOrientation(obj,GLU_OUTSIDE);

	for (i=0;i < 8;i++) {
		double pos[3];
		char label[2]={0,0};
		double size = 0.05;
		p5glove_reference_led(glove,i,pos);
		glMaterialfv(mode,GL_AMBIENT,ambient_ir);

		diffuse_ir[0]=0.8;
		diffuse_ir[1]=0.8;
		diffuse_ir[2]=0.8;

		if (info.ir[i].visible > 0) {
			size *= 1.0+(5.0-info.ir[i].visible)/5.0;
			diffuse_ir[0]=0.0;
		}

		glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);

		glPushMatrix();
		glTranslated(pos[0]*WORLD_SCALE,pos[1]*WORLD_SCALE,pos[2]*WORLD_SCALE);
		label[0]='0'+i;
		render_label(size,label);
		gluSphere(obj,size,10,10);

		if (info.ir[i].visible==1) {
			/* White: Normal */
			diffuse_ir[0]=1.0;
			diffuse_ir[1]=1.0;
			diffuse_ir[2]=1.0;
			glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
			render_cylinder(0.05,zero,info.normal,"Normal");

			/* Blue: Reference normal */
			diffuse_ir[0]=0.0;
			diffuse_ir[1]=0.0;
			diffuse_ir[2]=1.0;
			glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
			render_cylinder(0.05,zero,info.ref_normal,"Ref");
		}

		glPopMatrix();
	}

	gluDeleteQuadric(obj);
}

void render_axes(void)
{
	int mode=GL_FRONT;
	diffuse_ir[0]=1.0;
	diffuse_ir[1]=1.0;
	diffuse_ir[2]=0.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	render_cylinder(0.05,zero,y_up,"Up");
	render_cylinder(0.05,zero,y_down,"Down");
	render_cylinder(0.05,zero,x_left,"Left");
	render_cylinder(0.05,zero,x_right,"Right");
	render_cylinder(0.05,zero,z_front,"Front");
	render_cylinder(0.05,zero,z_back,"Back");
}

void render_obj_hand(void)
{
	int i,j;
	int mode=GL_FRONT;
	double pos[3];
	char label[2]={0,0};
	GLUquadricObj *obj;

	obj=gluNewQuadric();
	gluQuadricDrawStyle(obj,GLU_SMOOTH);
	gluQuadricOrientation(obj,GLU_OUTSIDE);

	glMaterialfv(mode,GL_AMBIENT,ambient_ir);

	diffuse_ir[0]=1.0;
	diffuse_ir[1]=0.0;
	diffuse_ir[2]=1.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	render_cylinder(0.05,zero,y_up,"Ref Up");

#if 1
	glPushMatrix();
	glRotated(info.rotation.angle,
			info.rotation.axis[0],
			info.rotation.axis[1],
			info.rotation.axis[2]);
	render_axes();
	glPopMatrix();
#endif

#if 1
	glPushMatrix();
	memcpy(pos,info.position,sizeof(double)*3);
	glTranslated(pos[0]*WORLD_SCALE,pos[1]*WORLD_SCALE,pos[2]*WORLD_SCALE);

	/* White: Normal */
	diffuse_ir[0]=1.0;
	diffuse_ir[1]=1.0;
	diffuse_ir[2]=1.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	render_cylinder(0.05,zero,info.normal,"Normal");

	/* Green: Rotation axis */
	diffuse_ir[0]=0.0;
	diffuse_ir[1]=1.0;
	diffuse_ir[2]=0.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	render_cylinder(0.05,zero,info.rotation.axis,"+Axis");
	{ 
		double nrot[3];
		nrot[0]= -info.rotation.axis[0];
		nrot[1]= -info.rotation.axis[1];
		nrot[2]= -info.rotation.axis[2];
		render_cylinder(0.05,zero,nrot,"-Axis");
	}

	/* Blue: Reference normal */
	diffuse_ir[0]=0.0;
	diffuse_ir[1]=0.0;
	diffuse_ir[2]=1.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	render_cylinder(0.05,zero,info.ref_normal,"Ref");
	glPopMatrix();
#endif

	/* Render the positions */
	glPushMatrix();
	diffuse_ir[0]=1.0;
	diffuse_ir[1]=0.0;
	diffuse_ir[2]=0.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	glTranslated(info.position[0]*WORLD_SCALE,info.position[1]*WORLD_SCALE,info.position[2]*WORLD_SCALE);
	gluSphere(obj,0.05,10,10);
	label[0]='0'+info.position_led[0];
	render_label(0.05,label);
	glPopMatrix();

	glPushMatrix();
	diffuse_ir[0]=0.0;
	diffuse_ir[1]=1.0;
	diffuse_ir[2]=0.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	glTranslated(info.position_next[0]*WORLD_SCALE,info.position_next[1]*WORLD_SCALE,info.position_next[2]*WORLD_SCALE);
	gluSphere(obj,0.05,10,10);
	label[0]='0'+info.position_led[1];
	render_label(0.05,label);
	glPopMatrix();

	glPushMatrix();
	diffuse_ir[0]=0.0;
	diffuse_ir[1]=0.0;
	diffuse_ir[2]=1.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	glTranslated(info.position_last[0]*WORLD_SCALE,info.position_last[1]*WORLD_SCALE,info.position_last[2]*WORLD_SCALE);
	gluSphere(obj,0.05,10,10);
	label[0]='0'+info.position_led[2];
	render_label(0.05,label);
	glPopMatrix();

#if 0
	glPushMatrix();
	diffuse_ir[0]=1.0;
	diffuse_ir[1]=0.0;
	diffuse_ir[2]=0.0;
	glMaterialfv(mode,GL_DIFFUSE,diffuse_ir);
	glScaled(7.0,1.0,10.0);
	glutSolidCube(0.1);
	glPopMatrix();
#endif

	gluDeleteQuadric(obj);
}

static double yaw=0.0,tilt=0.0,pitch=0.0;

void render_display(void)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(      0.0, 0.0, -WORLD_SIZE*.95, /* Eye */
			0.0, 0.0, 1.0, /* Horizon */
			0.0, 1.0, 0.0);    /* Up */

	glRotated(tilt,0.0, 0.1, 0.0);
	glRotated(yaw,0.0, 0.0, 1.0);
	glRotated(pitch,1.0, 0.0, 0.0);

	/* Render world */
	render_obj_world();

	render_obj_leds();

	/* Draw samples here */
	render_obj_hand();

	glutSwapBuffers();
}


void render_next(void)
{
	int err;

	/* Get samples here */
	err=p5glove_sample(glove, &info);

	if (err == 0) {
		p5glove_process_sample(glove, &info); 

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
		case 'w':
			pitch += 1.0;
			break;
		case 's':
			pitch -= 1.0;
			break;
		case 'a':
			tilt += 1.0;
			break;
		case 'd':
			tilt -= 1.0;
			break;
		case 'r':
			tilt = 0.0;
			pitch = 0.0;
			yaw = 0.0;
			break;
		default:
			break;
	}
}

int main(int argc,char **argv)
{
	glutInit(&argc,argv);

	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
	glutInitWindowSize(600,600);
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

    return 0;
}
