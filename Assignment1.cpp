#include <iostream>
#include <fstream>
#include <climits>
#include <math.h>
#include <GL/freeglut.h>
#include "loadTGA.h"

#define GL_CLAMP_TO_EDGE 0x812F // clamp to edge isn't defined by default

#define PLANE_X 1000
#define PLANE_Z 1000
#define PLANE_BOUNDARY 10
#define PLANE_TILE_SIZE 10
#define MOVE_SPEED 3
#define TURN_SPEED 3

#define MUSEUM_RADIUS 180
#define MUSEUM_SIDES 6
#define METATRAVELLER_COUNT 36
#define METATRAVELLER_SPEED 1

#define deg2rad(deg) (deg * 4.0 * atan(1)) / 180
#define rad2deg(rad) (180 * rad) / (4.0 * atan(1.0))
#define clamp(val, min, max) val < min ? min : (val > max ? max : val)

using namespace std;

//-- Globals --------------------------------------------------------------
float *x, *y, *z;		//vertex coordinate arrays
int *t1, *t2, *t3;		//triangles
int nvrt, ntri;			//total number of vertices and triangles
int angle = 90;	    //Rotation angle for viewing
float cam_hgt = 100;

float cam_x = 0;
float cam_y = 50;
float cam_z = -PLANE_Z / 2;

//-- Textures -------------------------------------------------------------
GLuint texIds[9];

void loadTextures()
{
	glGenTextures(9, texIds);

	// Skybox Front
	glBindTexture(GL_TEXTURE_2D, texIds[0]);
	loadTGA("textures/skybox/negz.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Back
	glBindTexture(GL_TEXTURE_2D, texIds[1]);
	loadTGA("textures/skybox/posz.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Right
	glBindTexture(GL_TEXTURE_2D, texIds[2]);
	loadTGA("textures/skybox/posx.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Left
	glBindTexture(GL_TEXTURE_2D, texIds[3]);
	loadTGA("textures/skybox/negx.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Bottom
	glBindTexture(GL_TEXTURE_2D, texIds[4]);
	loadTGA("textures/skybox/negy.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Top
	glBindTexture(GL_TEXTURE_2D, texIds[5]);
	loadTGA("textures/skybox/posy.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Brick
	glBindTexture(GL_TEXTURE_2D, texIds[6]);
	loadTGA("textures/brick.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 

	// Concrete
	glBindTexture(GL_TEXTURE_2D, texIds[7]);
	loadTGA("textures/concrete.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 

	// Sediment
	glBindTexture(GL_TEXTURE_2D, texIds[8]);
	loadTGA("textures/sediment.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

//-- Loads mesh data in OFF format    -------------------------------------
void loadMeshFile(const char* fname)  
{
	ifstream fp_in;
	int num, ne;

	fp_in.open(fname, ios::in);
	if(!fp_in.is_open())
	{
		cout << "Error opening mesh file" << endl;
		exit(1);
	}

	fp_in.ignore(INT_MAX, '\n');				//ignore first line
	fp_in >> nvrt >> ntri >> ne;			    // read number of vertices, polygons, edges

    x = new float[nvrt];                        //create arrays
    y = new float[nvrt];
    z = new float[nvrt];

    t1 = new int[ntri];
    t2 = new int[ntri];
    t3 = new int[ntri];

	for(int i=0; i < nvrt; i++)                         //read vertex list 
		fp_in >> x[i] >> y[i] >> z[i];

	for(int i=0; i < ntri; i++)                         //read polygon list 
	{
		fp_in >> num >> t1[i] >> t2[i] >> t3[i];
		if(num != 3)
		{
			cout << "ERROR: Polygon with index " << i  << " is not a triangle." << endl;  //not a triangle!!
			exit(1);
		}
	}

	fp_in.close();
	cout << " File successfully read." << endl;
}

//--Function to compute the normal vector of a triangle with index tindx ----------
void normal(int tindx)
{
	float x1 = x[t1[tindx]], x2 = x[t2[tindx]], x3 = x[t3[tindx]];
	float y1 = y[t1[tindx]], y2 = y[t2[tindx]], y3 = y[t3[tindx]];
	float z1 = z[t1[tindx]], z2 = z[t2[tindx]], z3 = z[t3[tindx]];
	float nx, ny, nz;
	nx = y1*(z2-z3) + y2*(z3-z1) + y3*(z1-z2);
	ny = z1*(x2-x3) + z2*(x3-x1) + z3*(x1-x2);
	nz = x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2);
	glNormal3f(nx, ny, nz);
}

void drawSkybox()
{
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	// glColor3f(0.2, 0.6, 0.8);
	float skyBoxScale = 2 * (PLANE_X >= PLANE_Z ? PLANE_X : PLANE_Z);
	
	// FRONT
	glBindTexture(GL_TEXTURE_2D, texIds[0]);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, -1);
	glTexCoord2f(0, 0);
	glVertex3f(skyBoxScale, -skyBoxScale, skyBoxScale); // bottom left
	glTexCoord2f(1, 0);
	glVertex3f(-skyBoxScale, -skyBoxScale, skyBoxScale); // bottom right
	glTexCoord2f(1, 1);
	glVertex3f(-skyBoxScale, skyBoxScale, skyBoxScale); // top right
	glTexCoord2f(0, 1);
	glVertex3f(skyBoxScale, skyBoxScale, skyBoxScale); // top left
	glEnd();
	
	// BACK
	glBindTexture(GL_TEXTURE_2D, texIds[1]);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3f(-skyBoxScale, -skyBoxScale, -skyBoxScale); // bottom left
	glTexCoord2f(1, 0);
	glVertex3f(skyBoxScale, -skyBoxScale, -skyBoxScale); // bottom right
	glTexCoord2f(1, 1);
	glVertex3f(skyBoxScale, skyBoxScale, -skyBoxScale); // top right
	glTexCoord2f(0, 1);
	glVertex3f(-skyBoxScale, skyBoxScale, -skyBoxScale); // top left
	glEnd();
	
	// LEFT
	glBindTexture(GL_TEXTURE_2D, texIds[2]);
	glBegin(GL_QUADS);
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 0);
	glVertex3f(skyBoxScale, -skyBoxScale, -skyBoxScale); // bottom left
	glTexCoord2f(1, 0);
	glVertex3f(skyBoxScale, -skyBoxScale, skyBoxScale); // bottom right
	glTexCoord2f(1, 1);
	glVertex3f(skyBoxScale, skyBoxScale, skyBoxScale); // top right
	glTexCoord2f(0, 1);
	glVertex3f(skyBoxScale, skyBoxScale, -skyBoxScale); // top left
	glEnd();
	
	// RIGHT
	glBindTexture(GL_TEXTURE_2D, texIds[3]);
	glBegin(GL_QUADS);
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-skyBoxScale, -skyBoxScale, skyBoxScale); // bottom left
	glTexCoord2f(1, 0);
	glVertex3f(-skyBoxScale, -skyBoxScale, -skyBoxScale); // bottom right
	glTexCoord2f(1, 1);
	glVertex3f(-skyBoxScale, skyBoxScale, -skyBoxScale); // top right
	glTexCoord2f(0, 1);
	glVertex3f(-skyBoxScale, skyBoxScale, skyBoxScale); // top left
	glEnd();
	
	// BOTTOM
	glBindTexture(GL_TEXTURE_2D, texIds[4]);
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-skyBoxScale, -skyBoxScale, skyBoxScale); // bottom left
	glTexCoord2f(1, 0);
	glVertex3f(-skyBoxScale, -skyBoxScale, -skyBoxScale); // bottom right
	glTexCoord2f(1, 1);
	glVertex3f(skyBoxScale, -skyBoxScale, -skyBoxScale); // top right
	glTexCoord2f(0, 1);
	glVertex3f(skyBoxScale, -skyBoxScale, skyBoxScale); // top left
	glEnd();
	
	// TOP
	glBindTexture(GL_TEXTURE_2D, texIds[5]);
	glBegin(GL_QUADS);
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-skyBoxScale, skyBoxScale, -skyBoxScale); // bottom left
	glTexCoord2f(1, 0);
	glVertex3f(-skyBoxScale, skyBoxScale, skyBoxScale); // bottom right
	glTexCoord2f(1, 1);
	glVertex3f(skyBoxScale, skyBoxScale, skyBoxScale); // top right
	glTexCoord2f(0, 1);
	glVertex3f(skyBoxScale, skyBoxScale, -skyBoxScale); // top left
	glEnd();

	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
}

//----------draw a floor plane-------------------
void drawFloor()
{
	bool flag = false;

	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glColor3f(0.6, 1.0, 0.8);
	for(int x = -PLANE_X; x <= PLANE_X; x += PLANE_TILE_SIZE)
	{
		for(int z = -PLANE_Z; z <= PLANE_Z; z += PLANE_TILE_SIZE)
		{
			if(flag) glColor3f(0.6, 1.0, 0.8);
			else glColor3f(0.8, 1.0, 0.6);
			glVertex3f(x, 0, z);
			glVertex3f(x, 0, z + PLANE_TILE_SIZE);
			glVertex3f(x + PLANE_TILE_SIZE, 0, z + PLANE_TILE_SIZE);
			glVertex3f(x + PLANE_TILE_SIZE, 0, z);
			flag = !flag;
		}
	}
	glEnd();
}

void drawMuseum()
{
	glColor3f(0.8, 0.5, 0.0);
	// remove this after
	glPushMatrix();
		glutSolidSphere(5, 12, 12);
	glPopMatrix();

	float angle = 360.0 / MUSEUM_SIDES;
	for (int i = 1; i < MUSEUM_SIDES; i++)
	{
		glPushMatrix();
			glRotatef((angle * i) + 90, 0, 1, 0);
			glTranslatef(MUSEUM_RADIUS, 50, 0);
			glScalef(10, 100, tan(deg2rad(angle / 2)) * MUSEUM_RADIUS * 2);
			glutSolidCube(1);
		glPopMatrix();
	}

	// pillars
	float pillarDistance = MUSEUM_RADIUS / sin(deg2rad(angle));
	cout << pillarDistance << endl;
	for (int i = 0; i < MUSEUM_SIDES; i++)
	{
		glPushMatrix();
			glRotatef((angle * i), 0, 1, 0);
			glTranslatef(pillarDistance, 0, 0);
			glRotatef(-90, 1, 0, 0);
			glutSolidCylinder(10, 100, 12, 24);
		glPopMatrix();
	}
}

void display()
{
	float lpos[4] = {0., 200., 0., 1.0};  //light's position

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);    //GL_LINE = Wireframe;   GL_FILL = Solid
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float look_x = cos(deg2rad(angle)) * 200;
	float look_z = sin(deg2rad(angle)) * 200;

	gluLookAt(cam_x, cam_y, cam_z, cam_x + look_x, 0, cam_z + look_z, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);   //set light position

	drawSkybox();

	drawFloor();
	drawMuseum();

    // glFlush();
	glutSwapBuffers();
}

void initialize()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);	//Background colour
	
	loadTextures();

	glEnable(GL_LIGHTING);					//Enable OpenGL states
	glEnable(GL_LIGHT0);
 	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1, 10, 5000);  //The camera view volume
}

void special(int key, int x, int y)
{
	switch (key)
	{
		case GLUT_KEY_LEFT:
			angle = (angle + (360 - TURN_SPEED)) % 360;
			break;
		case GLUT_KEY_RIGHT:
			angle = (angle + TURN_SPEED) % 360;
			break;
		case GLUT_KEY_UP:
			cam_x += cos(deg2rad(angle)) * MOVE_SPEED;
			cam_z += sin(deg2rad(angle)) * MOVE_SPEED;
			break;
		case GLUT_KEY_DOWN:
			cam_x -= cos(deg2rad(angle)) * MOVE_SPEED;
			cam_z -= sin(deg2rad(angle)) * MOVE_SPEED;
			break;
	}
    
	cam_x = clamp(cam_x, -PLANE_X + PLANE_BOUNDARY, PLANE_X - PLANE_BOUNDARY);
	cam_z = clamp(cam_z, -PLANE_Z + PLANE_BOUNDARY, PLANE_Z - PLANE_BOUNDARY);

	glutPostRedisplay();
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_SINGLE | GLUT_DEPTH);
   glutInitWindowSize (800, 800); 
   glutInitWindowPosition (10, 10);
   glutCreateWindow ("Museum");
   initialize();

   glutDisplayFunc(display);
   glutSpecialFunc(special); 
   glutMainLoop();
   return 0;
}