#include <iostream>
#include <fstream>
#include <climits>
#include <math.h>
#include <GL/freeglut.h>
#include "loadTGA.h"

#define GL_CLAMP_TO_EDGE 0x812F // clamp to edge isn't defined by default

#define PLANE_X 1000
#define PLANE_Z 1000
#define FLOOR_SCALE 8.0
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
#define CRADLE_MAX_ANGLE 45
#define CRADLE_LENGTH 40
#define BALL_MASS 4
#define GRAVITY 9.80665

#define deg2rad(deg) (deg * 4.0 * atan(1)) / 180
#define rad2deg(rad) (180 * rad) / (4.0 * atan(1.0))
#define clamp(val, min, max) val < min ? min : (val > max ? max : val)
#define min(val1, val2) (val1 < val2 ? val1 : val2)
#define max(val1, val2) (val1 > val2 ? val1 : val2)

using namespace std;

typedef struct {
	float x;
	float y;
	float z;
} Vector;

//-- Globals --------------------------------------------------------------
float *x, *y, *z;		//vertex coordinate arrays
int *t1, *t2, *t3;		//triangles
int nvrt, ntri;			//total number of vertices and triangles
int angle = 90;	    //Rotation angle for viewing
float cam_hgt = 100;

float cam_x = 0;
float cam_y = 50;
float cam_z = -PLANE_Z / 2;

int sceneTime = 90;
int metatravellerAngles[METATRAVELLER_COUNT];
int mobiusStripBallAngle = 0;
Vector mobiusStripVertices[74];

float cradleAngle = CRADLE_MAX_ANGLE;

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
	cradleAngle = (-(BALL_MASS * GRAVITY) * (CRADLE_MAX_ANGLE * sin(deg2rad(sceneTime * 2)))) / CRADLE_LENGTH;
	sceneTime = (sceneTime + 1) % 360;

	glutPostRedisplay();
	glutTimerFunc(10, timer, 0);
}

void rotateVectorX(Vector* vec, float rot)
{
	Vector newVec =
	{
		vec->x,
		(cosf(deg2rad(rot)) * vec->y) - (sinf(deg2rad(rot)) * vec->z),
		(sinf(deg2rad(rot)) * vec->y) + (cosf(deg2rad(rot)) * vec->z),
	};
	vec->x = newVec.x;
	vec->y = newVec.y;
	vec->z = newVec.z;
}

void rotateVectorY(Vector* vec, float rot)
{
	Vector newVec =
	{
		(cosf(deg2rad(rot)) * vec->x) + (sinf(deg2rad(rot)) * vec->z),
		vec->y,
		-(sinf(deg2rad(rot)) * vec->x) + (cosf(deg2rad(rot)) * vec->z),
	};
	vec->x = newVec.x;
	vec->y = newVec.y;
	vec->z = newVec.z;
}

void rotateVectorZ(Vector* vec, float rot)
{
	Vector newVec =
	{
		(cosf(deg2rad(rot)) * vec->x) - (sinf(deg2rad(rot)) * vec->y),
		(sinf(deg2rad(rot)) * vec->x) + (cosf(deg2rad(rot)) * vec->y),
		vec->z,
	};
	vec->x = newVec.x;
	vec->y = newVec.y;
	vec->z = newVec.z;
}

void loadTextures()
{
	glGenTextures(9, texIds);

	// Skybox Front
	glBindTexture(GL_TEXTURE_2D, texIds[0]);
	loadTGA("textures/skybox/Front.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Back
	glBindTexture(GL_TEXTURE_2D, texIds[1]);
	loadTGA("textures/skybox/Back.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Right
	glBindTexture(GL_TEXTURE_2D, texIds[2]);
	loadTGA("textures/skybox/Right.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Left
	glBindTexture(GL_TEXTURE_2D, texIds[3]);
	loadTGA("textures/skybox/Left.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Bottom
	glBindTexture(GL_TEXTURE_2D, texIds[4]);
	loadTGA("textures/skybox/Bottom.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Skybox Top
	glBindTexture(GL_TEXTURE_2D, texIds[5]);
	loadTGA("textures/skybox/Top.tga");
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
}

void normal(Vector v1, Vector v2, Vector v3)
{
	float x1 = v1.x, x2 = v2.x, x3 = v3.x;
	float y1 = v1.y, y2 = v2.y, y3 = v3.y;
	float z1 = v1.z, z2 = v2.z, z3 = v3.z;
	float nx, ny, nz;
	nx = y1*(z2-z3) + y2*(z3-z1) + y3*(z1-z2);
	ny = z1*(x2-x3) + z2*(x3-x1) + z3*(x1-x2);
	nz = x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2);
	glNormal3f(nx, ny, nz);
}

void drawSkybox()
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	float skyBoxScale = 2 * (PLANE_X >= PLANE_Z ? PLANE_X : PLANE_Z);
	glPushMatrix();
		// glTranslatef(0, skyBoxScale - 0.1, 0);
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
		glTexCoord2f(0, 1);
		glVertex3f(-skyBoxScale, -skyBoxScale, -skyBoxScale); // bottom right
		glTexCoord2f(1, 1);
		glVertex3f(skyBoxScale, -skyBoxScale, -skyBoxScale); // top right
		glTexCoord2f(1, 0);
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
	glPopMatrix();
}

//----------draw a floor plane-------------------
void drawFloor()
{
	glDisable(GL_LIGHTING);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, texIds[7]);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	for(int x = -PLANE_X; x <= PLANE_X; x += PLANE_TILE_SIZE)
	{
		for(int z = -PLANE_Z; z <= PLANE_Z; z += PLANE_TILE_SIZE)
		{
			glTexCoord2f((x + 0) / FLOOR_SCALE, (z + 0) / FLOOR_SCALE); glVertex3f(x, 0, z);
			glTexCoord2f((x + 1) / FLOOR_SCALE, (z + 0) / FLOOR_SCALE); glVertex3f(x, 0, z + PLANE_TILE_SIZE);
			glTexCoord2f((x + 1) / FLOOR_SCALE, (z + 1) / FLOOR_SCALE); glVertex3f(x + PLANE_TILE_SIZE, 0, z + PLANE_TILE_SIZE);
			glTexCoord2f((x + 0) / FLOOR_SCALE, (z + 1) / FLOOR_SCALE); glVertex3f(x + PLANE_TILE_SIZE, 0, z);
		}
	}
	glEnd();
	glEnable(GL_LIGHTING);
}

void drawMuseum()
{
	glColor3f(1, 1, 1);
	float angle = 360.0 / MUSEUM_SIDES;
	float wallLength = tan(deg2rad(angle / 2)) * MUSEUM_RADIUS * 2;
	// glDisable(GL_LIGHTING);
	for (int i = 1; i < MUSEUM_SIDES; i++)
	{
		glPushMatrix();
			glRotatef((angle * i) + 90, 0, 1, 0);
			glTranslatef(MUSEUM_RADIUS, 50, 0);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glBindTexture(GL_TEXTURE_2D, texIds[6]);
			int numColumns = (int)ceil(wallLength / 20.0);

			glPushMatrix();
				glTranslatef(-5, -50, -100);
				glBegin(GL_QUADS);
					glNormal3f(-1, 0, 0);
					for (int x = 0; x < numColumns; x++)
					{
						for (int y = 0; y < 5; y++)
						{
							float left = min(20 * (x + 1), wallLength);
							float texCoordLeft;
							if (((int)left) % 20 == 0)
							{
								texCoordLeft = 1;
							}
							else
							{
								texCoordLeft = (wallLength - (20 * x)) / 20.0;
							}

							glTexCoord2f(0, 0); glVertex3f(0, 20 * y, 20 * x);
							glTexCoord2f(texCoordLeft, 0); glVertex3f(0, 20 * y, left);
							glTexCoord2f(texCoordLeft, 1); glVertex3f(0, 20 * (y + 1), left);
							glTexCoord2f(0, 1); glVertex3f(0, 20 * (y + 1), 20 * x);
						}
					}
				glEnd();
			glPopMatrix();

			glPushMatrix();
				glTranslatef(5, -50, -100);
				glBegin(GL_QUADS);
					glNormal3f(-1, 0, 0);
					for (int x = 0; x < numColumns; x++)
					{
						for (int y = 0; y < 5; y++)
						{
							float left = min(20 * (x + 1), wallLength);
							float texCoordLeft;
							if (((int)left) % 20 == 0)
							{
								texCoordLeft = 1;
							}
							else
							{
								texCoordLeft = (wallLength - (20 * x)) / 20.0;
							}

							glTexCoord2f(0, 0); glVertex3f(0, 20 * y, 20 * x);
							glTexCoord2f(texCoordLeft, 0); glVertex3f(0, 20 * y, left);
							glTexCoord2f(texCoordLeft, 1); glVertex3f(0, 20 * (y + 1), left);
							glTexCoord2f(0, 1); glVertex3f(0, 20 * (y + 1), 20 * x);
						}
					}
				glEnd();
			glPopMatrix();
		glPopMatrix();
	}

	// pillars
	float pillarDistance = MUSEUM_RADIUS / sin(deg2rad(angle));
	for (int i = 0; i < MUSEUM_SIDES; i++)
	{
		int cylinderSides = 24;
		float xScale = 1;
		float yScale = 5;

		glPushMatrix();
			glRotatef((angle * i), 0, 1, 0);
			glTranslatef(pillarDistance, 0, 0);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glBindTexture(GL_TEXTURE_2D, texIds[8]);
			glBegin(GL_QUADS);
				for (int i = 0; i <= cylinderSides; i++)
				{
					Vector v1 = {10, 0, 0};
					Vector v2 = {10, 100, 0};
					Vector v3 = {10, 100, 0};
					Vector v4 = {10, 0, 0};
					rotateVectorY(&v1, (360.0 / cylinderSides) * i);
					rotateVectorY(&v2, (360.0 / cylinderSides) * i);
					rotateVectorY(&v3, (360.0 / cylinderSides) * (i + 1));
					rotateVectorY(&v4, (360.0 / cylinderSides) * (i + 1));

					normal(v1, v2, v3);
					glTexCoord2f((xScale / cylinderSides) * i, 0); glVertex3f(v1.x, v1.y, v1.z);
					glTexCoord2f((xScale / cylinderSides) * i, 1); glVertex3f(v2.x, v2.y, v2.z);
					glTexCoord2f((xScale / cylinderSides) * (i + 1), 1); glVertex3f(v3.x, v3.y, v3.z);
					glTexCoord2f((xScale / cylinderSides) * (i + 1), 0); glVertex3f(v4.x, v4.y, v4.z);
				}
			glEnd();
		glPopMatrix();
	}
	// glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	// roof
	// TODO: change glutSolidCone to gluCylinder (for texcoords)
	glColor3f(0.3, 0.3, 0.3);
	glPushMatrix();
		glTranslatef(0, 100, 0);
		glRotatef(-90, 1, 0, 0);
		glutSolidCone(pillarDistance + 20, 100, MUSEUM_SIDES, MUSEUM_SIDES);
	glPopMatrix();

	// floor
	// TODO: generate floor using points and texcoords
	glColor3f(0.5, 0, 0.8);
	glPushMatrix();
		glTranslatef(0, -0.99, 0);
		glRotatef(-90, 1, 0, 0);
		glutSolidCylinder(pillarDistance, 1, MUSEUM_SIDES, MUSEUM_SIDES);
	glPopMatrix();
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
		glTranslatef(120, 0, 0);
		glRotatef(90, 0, 1, 0);
		// Base
		glPushMatrix();
			glColor3f(0.2, 0.2, 0.2);
			glScalef(50, 10, 60);
			glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();
			// Mobius Strip
			glColor3f(0.6, 0.0, 1.0);
			glTranslatef(0, 20, 0);
			glBegin(GL_TRIANGLES);
			for (int i = 0; i < 37; i++)
			{
				Vector v1 = mobiusStripVertices[2 * i];
				Vector v2 = mobiusStripVertices[(2 * i + 1) % 74];
				Vector v3 = mobiusStripVertices[(2 * i + 2) % 74];
				Vector v4 = mobiusStripVertices[(2 * i + 3) % 74];

				normal(v1, v2, v3);
				glVertex3f(v1.x, v1.y, v1.z);
				glVertex3f(v2.x, v2.y, v2.z);
				glVertex3f(v3.x, v3.y, v3.z);

				normal(v4, v3, v2);
				glVertex3f(v4.x, v4.y, v4.z);
				glVertex3f(v3.x, v3.y, v3.z);
				glVertex3f(v2.x, v2.y, v2.z);
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


// For Newton's Cradle:
// https://en.wikipedia.org/wiki/Pendulum_(mathematics)
// Use a min call and a max call on the simple gravity pendulum differential equation
// min should be used on one end of the Newton's Cradle, while max should be used on the other

void drawNewtonsCradle()
{
	glPushMatrix();
		glTranslatef(-120, 0, 0);
		glRotatef(-90, 0, 1, 0);
		// Base
		glPushMatrix();
			glColor3f(0.2, 0.2, 0.2);
			glScalef(50, 10, 60);
			glutSolidCube(1);
		glPopMatrix();

		// Pendulums
		glPushMatrix();
			glTranslatef(0, 10, 0);

			glPushMatrix();
				glColor3f(0.5, 0.5, 0.5);
				glTranslatef(-12, CRADLE_LENGTH, 0);
				glRotatef(min(0, cradleAngle), 0, 0, 1);
				glTranslatef(-0, -CRADLE_LENGTH, 0);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				glColor3f(0.5, 0.5, 0.5);
				glTranslatef(-6, 0, 0);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				glColor3f(0.5, 0.5, 0.5);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				glColor3f(0.5, 0.5, 0.5);
				glTranslatef(6, 0, 0);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				glColor3f(0.5, 0.5, 0.5);
				glTranslatef(12, CRADLE_LENGTH, 0);
				glRotatef(max(0, cradleAngle), 0, 0, 1);
				glTranslatef(0, -CRADLE_LENGTH, 0);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();
		glPopMatrix();

		// Frame
		glPushMatrix();
			glColor3f(0.5, 0.5, 0.5);
			glPushMatrix();
				glTranslatef(-21.5, 10 + (CRADLE_LENGTH * cos(deg2rad(20))), (CRADLE_LENGTH * sin(deg2rad(20))));
				glRotatef(90, 0, 1, 0);
				glutSolidCylinder(1.5, 43, 12, 12);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(-21.5, 10 + (CRADLE_LENGTH * cos(deg2rad(20))), -(CRADLE_LENGTH * sin(deg2rad(20))));
				glRotatef(90, 0, 1, 0);
				glutSolidCylinder(1.5, 43, 12, 12);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(20, 0, (CRADLE_LENGTH * sin(deg2rad(20))));
				glRotatef(-90, 1, 0, 0);
				glutSolidCylinder(1.5, 10 + (CRADLE_LENGTH * cos(deg2rad(20))), 12, 12);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(-20, 0, (CRADLE_LENGTH * sin(deg2rad(20))));
				glRotatef(-90, 1, 0, 0);
				glutSolidCylinder(1.5, 10 + (CRADLE_LENGTH * cos(deg2rad(20))), 12, 12);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(20, 0, -(CRADLE_LENGTH * sin(deg2rad(20))));
				glRotatef(-90, 1, 0, 0);
				glutSolidCylinder(1.5, 10 + (CRADLE_LENGTH * cos(deg2rad(20))), 12, 12);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(-20, 0, -(CRADLE_LENGTH * sin(deg2rad(20))));
				glRotatef(-90, 1, 0, 0);
				glutSolidCylinder(1.5, 10 + (CRADLE_LENGTH * cos(deg2rad(20))), 12, 12);
			glPopMatrix();
		glPopMatrix();
	glPopMatrix();
}

void display()
{
	float outerLightPos[4] = {0., 1000., 0., 10.0};  //light's position
	float innerLightPos[4] = {0., 90., 0., 1.0};  //light's position

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);    //GL_LINE = Wireframe;   GL_FILL = Solid
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float look_x = cos(deg2rad(angle)) * 200;
	float look_z = sin(deg2rad(angle)) * 200;

	gluLookAt(cam_x, cam_y, cam_z, cam_x + look_x, LOOK_HEIGHT, cam_z + look_z, 0, 1, 0);
	// glLightfv(GL_LIGHT0, GL_POSITION, innerLightPos);   //set light position
	glLightfv(GL_LIGHT1, GL_POSITION, outerLightPos);   //set light position

	drawSkybox();

	drawFloor();
	drawMuseum();
	drawMetatravellers();
	drawMobiusStrip();
	drawNewtonsCradle();

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
		mobiusStripVertices[2 * i].x = 0;
		mobiusStripVertices[2 * i].y = 5;
		mobiusStripVertices[2 * i].z = 0;
		
		mobiusStripVertices[2 * i + 1].x = 0;
		mobiusStripVertices[2 * i + 1].y = -5;
		mobiusStripVertices[2 * i + 1].z = 0;

		rotateVectorX(&mobiusStripVertices[2 * i], 5 * i);
		rotateVectorX(&mobiusStripVertices[2 * i + 1], 5 * i);

		mobiusStripVertices[2 * i].z += MOBIUS_STRUP_RADIUS;
		mobiusStripVertices[2 * i + 1].z += MOBIUS_STRUP_RADIUS;

		rotateVectorY(&mobiusStripVertices[2 * i], 10 * i);
		rotateVectorY(&mobiusStripVertices[2 * i + 1], 10 * i);
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
	glEnable(GL_LIGHT1);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
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