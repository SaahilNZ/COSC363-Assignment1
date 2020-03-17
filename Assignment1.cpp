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
#define LOOK_HEIGHT 10

#define MUSEUM_RADIUS 180
#define MUSEUM_SIDES 6
#define METATRAVELLER_COUNT 72
#define METATRAVELLER_SPEED 2
#define METATRAVELLER_SPIRALS 6
#define MOBIUS_STRUP_RADIUS 20
#define MOBIUS_STRIP_SPEED 1
#define MOBIUS_STRIP_BALLS 3

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

int metatravellerAngles[METATRAVELLER_COUNT];
int mobiusStripBallAngle = 0;
float mobiusStripVertices[74][3];

GLuint texIds[9];

void calcMetatravellerAngles()
{
	for (int i = 0; i < METATRAVELLER_COUNT; i++)
	{
		metatravellerAngles[i] = (metatravellerAngles[i] + METATRAVELLER_SPEED) % 360;
	}
}

void timer(int value)
{
	calcMetatravellerAngles();
	mobiusStripBallAngle = (mobiusStripBallAngle + 1) % 720;

	glutPostRedisplay();
	glutTimerFunc(10, timer, 0);
}

float* rotateVectorX(float vec[3], float rot)
{
	float newVec[3] =
	{
		vec[0],
		(cosf(deg2rad(rot)) * vec[1]) - (sinf(deg2rad(rot)) * vec[2]),
		(sinf(deg2rad(rot)) * vec[1]) + (cosf(deg2rad(rot)) * vec[2]),
	};
	vec[0] = newVec[0];
	vec[1] = newVec[1];
	vec[2] = newVec[2];
	return vec;
}

float* rotateVectorY(float vec[], float rot)
{
	float newVec[3] =
	{
		(cosf(deg2rad(rot)) * vec[0]) + (sinf(deg2rad(rot)) * vec[2]),
		vec[1],
		-(sinf(deg2rad(rot)) * vec[0]) + (cosf(deg2rad(rot)) * vec[2]),
	};
	vec[0] = newVec[0];
	vec[1] = newVec[1];
	vec[2] = newVec[2];
	return vec;
}

float* rotateVectorZ(float vec[], float rot)
{
	float newVec[3] =
	{
		(cosf(deg2rad(rot)) * vec[0]) - (sinf(deg2rad(rot)) * vec[1]),
		(sinf(deg2rad(rot)) * vec[0]) + (cosf(deg2rad(rot)) * vec[1]),
		vec[2],
	};
	vec[0] = newVec[0];
	vec[1] = newVec[1];
	vec[2] = newVec[2];
	return vec;
}

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

void normal(float v1[3], float v2[3], float v3[3])
{
	float x1 = v1[0], x2 = v2[0], x3 = v3[0];
	float y1 = v1[1], y2 = v2[1], y3 = v3[1];
	float z1 = v1[2], z2 = v2[2], z3 = v3[2];
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
	// TODO: change glutSolidCylinder to gluCylinder (for texcoords)
	float pillarDistance = MUSEUM_RADIUS / sin(deg2rad(angle));
	for (int i = 0; i < MUSEUM_SIDES; i++)
	{
		glPushMatrix();
			glRotatef((angle * i), 0, 1, 0);
			glTranslatef(pillarDistance, 0, 0);
			glRotatef(-90, 1, 0, 0);
			glutSolidCylinder(10, 100, 12, 24);
		glPopMatrix();
	}

	// roof
	// TODO: change glutSolidCone to gluCylinder (for texcoords)
	glPushMatrix();
		glTranslatef(0, 100, 0);
		glRotatef(-90, 1, 0, 0);
		glutSolidCone(pillarDistance + 20, 100, MUSEUM_SIDES, MUSEUM_SIDES);
	glPopMatrix();

	// floor
	// TODO: generate floor using points and texcoords 
	glEnable(GL_TEXTURE_2D);
	// glEnable(GL_TEXTURE_GEN_S);
	// glEnable(GL_TEXTURE_GEN_T);
	glBindTexture(GL_TEXTURE_2D, texIds[7]);
	glPushMatrix();
		glTranslatef(0, -0.99, 0);
		glRotatef(-90, 1, 0, 0);
		glutSolidCylinder(pillarDistance, 1, MUSEUM_SIDES, MUSEUM_SIDES);
	glPopMatrix();
	// glDisable(GL_TEXTURE_GEN_S);
	// glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_2D);
}

void drawMetatravellers()
{
	glPushMatrix();
		glTranslatef(0, 0, 120);
		glPushMatrix();
			glColor3f(0.2, 0.2, 0.2);
			glScalef(50, 10, 60);
			glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 30, 0);
			for (int i = 0; i < METATRAVELLER_COUNT; i++)
			{
				// TODO: Implement toggle button

				// glColor3f(1, 0.9, 0.3);
				// glPushMatrix();
				// 	glRotatef(i * (360.0 / METATRAVELLER_COUNT), 0, 1, 0);
				// 	glTranslatef(0, 0, 20);
				// 	glRotatef(90, 0, 1, 0);
				// 	glutSolidTorus(0.05, 5, 4, 36);
				// glPopMatrix();
				
				glColor3f(0.8, 0, 0.8);
				glPushMatrix();
					glRotatef(i * (360.0 / METATRAVELLER_COUNT), 0, 1, 0);
					glTranslatef(0, 0, 20);
					glRotatef(metatravellerAngles[i], 1, 0, 0);
					glTranslatef(0, 0, 5);
					glutSolidSphere(1, 12, 12);
				glPopMatrix();
			}
		glPopMatrix();
	glPopMatrix();
}

void drawMobiusStrip()
{
	glPushMatrix();
		// glTranslatef(120, 0, 0);
		// glRotatef(90, 0, 1, 0);
		// glPushMatrix();
		// 	glColor3f(0.2, 0.2, 0.2);
		// 	glScalef(50, 10, 60);
		// 	glutSolidCube(1);
		// glPopMatrix();

		// float offset = (sin(deg2rad(5)) * MOBIUS_STRUP_RADIUS);
		// glPushMatrix();
		// 	glTranslatef(0, 20, 0);

		// 	// Planks
		// 	for (int i = 0; i < 360; i += 10)
		// 	{
		// 		glPushMatrix();
		// 			glColor3f(0.5, 0.3, 0.0);
		// 			glRotatef(i, 0, 1, 0);
		// 			glTranslatef(0, 0, MOBIUS_STRUP_RADIUS);
		// 			glRotatef(i / 2.0, 1, 0, 0);

		// 			glBegin(GL_TRIANGLES);
		// 			glNormal3f(0, 0, 1);
		// 			glTexCoord2f(0, 0); glVertex3f(-offset, 5, 0);
		// 			glTexCoord2f(0, 1); glVertex3f(-offset, -5, 0);
		// 			glTexCoord2f(1, 1); glVertex3f(offset, cos(deg2rad(5)) * 5, sin(deg2rad(5)) * 5);

		// 			glTexCoord2f(1, 0); glVertex3f(offset, cos(deg2rad(5)) * -5, sin(deg2rad(5)) * -5);
		// 			glTexCoord2f(1, 1); glVertex3f(offset, cos(deg2rad(5)) * 5, sin(deg2rad(5)) * 5);
		// 			glTexCoord2f(1, 0); glVertex3f(-offset, -5, 0);
		// 			glEnd();
		// 		glPopMatrix();
		// 	}

		// 	// Rail
		// 	glPushMatrix();
		// 		glColor3f(0.3, 0.3, 0.3);
		// 		glRotatef(90, 1, 0, 0);
		// 		glutSolidTorus(0.2, MOBIUS_STRUP_RADIUS, 4, 36);
		// 	glPopMatrix();

		// 	// Balls
		// 	float angleOffset = 360.0 / MOBIUS_STRIP_BALLS;
		// 	for (int i = 0; i < MOBIUS_STRIP_BALLS; i++)
		// 	{
		// 		glPushMatrix();
		// 			glColor3f(0.6, 0.6, 0.6);
		// 			glRotatef((mobiusStripBallAngle + i * angleOffset), 0, 1, 0);
		// 			glTranslatef(0, 0, -MOBIUS_STRUP_RADIUS);
		// 			glRotatef((-(mobiusStripBallAngle + i * angleOffset) / 2.0), 1, 0, 0);
		// 			glTranslatef(0, 2, 0);
		// 			glutSolidSphere(2, 12, 12);
		// 		glPopMatrix();
		// 	}
		// glPopMatrix();

		glPushMatrix();
			glColor3f(0.6, 0.0, 1.0);
			glTranslatef(0, 20, 0);
			glBegin(GL_TRIANGLES);
			for (int i = 0; i < 37; i++)
			{
				float* v1 = mobiusStripVertices[2 * i];
				float* v2 = mobiusStripVertices[(2 * i + 1) % 74];
				float* v3 = mobiusStripVertices[(2 * i + 2) % 74];
				float* v4 = mobiusStripVertices[(2 * i + 3) % 74];

				normal(v1, v2, v3);
				glVertex3f(v1[0], v1[1], v1[2]);
				glVertex3f(v2[0], v2[1], v2[2]);
				glVertex3f(v3[0], v3[1], v3[2]);

				normal(v4, v3, v2);
				glVertex3f(v4[0], v4[1], v4[2]);
				glVertex3f(v3[0], v3[1], v3[2]);
				glVertex3f(v2[0], v2[1], v2[2]);
			}
			glEnd();
			
			// Balls
			float angleOffset = 720.0 / MOBIUS_STRIP_BALLS;
			for (int i = 0; i < MOBIUS_STRIP_BALLS; i++)
			{
				glPushMatrix();
					glColor3f(0.6, 0.6, 0.6);
					glRotatef((mobiusStripBallAngle + i * angleOffset), 0, 1, 0);
					glTranslatef(0, 0, -MOBIUS_STRUP_RADIUS);
					glRotatef((-(mobiusStripBallAngle + i * angleOffset) / 2.0), 1, 0, 0);
					glTranslatef(0, 2.5, 0);
					glutSolidSphere(2, 12, 12);
				glPopMatrix();
			}
		glPopMatrix();
	glPopMatrix();
}

void display()
{
	float lpos[4] = {0., 90., 0., 1.0};  //light's position

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);    //GL_LINE = Wireframe;   GL_FILL = Solid
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float look_x = cos(deg2rad(angle)) * 200;
	float look_z = sin(deg2rad(angle)) * 200;

	gluLookAt(cam_x, cam_y, cam_z, cam_x + look_x, LOOK_HEIGHT, cam_z + look_z, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);   //set light position

	drawSkybox();

	drawFloor();
	drawMuseum();
	drawMetatravellers();
	drawMobiusStrip();

    // glFlush();
	glutSwapBuffers();
}

void initialiseMetatravellers()
{
	for (int i = 0; i < METATRAVELLER_COUNT; i++)
	{
		metatravellerAngles[i] = ((360 * METATRAVELLER_SPIRALS) / METATRAVELLER_COUNT) * i;
	}
}

void initialiseMobiusStrip()
{
	for (int i = 0; i < 37; i++)
	{
		mobiusStripVertices[2 * i][0] = 0;
		mobiusStripVertices[2 * i][1] = 5;
		mobiusStripVertices[2 * i][2] = 0;
		
		mobiusStripVertices[2 * i + 1][0] = 0;
		mobiusStripVertices[2 * i + 1][1] = -5;
		mobiusStripVertices[2 * i + 1][2] = 0;

		float* v1rx = rotateVectorX(mobiusStripVertices[2 * i], 5 * i);
		float* v2rx = rotateVectorX(mobiusStripVertices[2 * i + 1], 5 * i);

		mobiusStripVertices[2 * i][0] = v1rx[0];
		mobiusStripVertices[2 * i][1] = v1rx[1];
		mobiusStripVertices[2 * i][2] = v1rx[2] + MOBIUS_STRUP_RADIUS;
		
		mobiusStripVertices[2 * i + 1][0] = v2rx[0];
		mobiusStripVertices[2 * i + 1][1] = v2rx[1];
		mobiusStripVertices[2 * i + 1][2] = v2rx[2] + MOBIUS_STRUP_RADIUS;

		float* v1ry = rotateVectorY(mobiusStripVertices[2 * i], 10 * i);
		float* v2ry = rotateVectorY(mobiusStripVertices[2 * i + 1], 10 * i);

		mobiusStripVertices[2 * i][0] = v1ry[0];
		mobiusStripVertices[2 * i][1] = v1ry[1];
		mobiusStripVertices[2 * i][2] = v1ry[2];
		
		mobiusStripVertices[2 * i + 1][0] = v2ry[0];
		mobiusStripVertices[2 * i + 1][1] = v2ry[1];
		mobiusStripVertices[2 * i + 1][2] = v2ry[2];
	}
}

void initialize()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	loadTextures();
	initialiseMetatravellers();
	initialiseMobiusStrip();

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
 	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1, 10, 5000);
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
   glutSetOption(GLUT_MULTISAMPLE, 4);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
   glutInitWindowSize (800, 800); 
   glutInitWindowPosition (10, 10);
   glutCreateWindow ("Museum");
   initialize();

   glutDisplayFunc(display);
   glutSpecialFunc(special); 
   glutTimerFunc(10, timer, 0);
   glutMainLoop();
   return 0;
}