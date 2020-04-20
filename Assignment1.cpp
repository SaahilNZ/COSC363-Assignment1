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
#define MOVE_SPEED 1
#define TURN_SPEED 1
#define LOOK_HEIGHT 10

#define MUSEUM_RADIUS 180
#define MUSEUM_SIDES 6
#define MUSEUM_PILLAR_SIDES 24
#define METATRAVELLER_COUNT 72
#define METATRAVELLER_SPEED 2
#define METATRAVELLER_SPIRALS 6
#define MOBIUS_STRUP_RADIUS 20
#define MOBIUS_STRIP_SPEED 1
#define MOBIUS_STRIP_BALLS 3
#define CRADLE_MAX_ANGLE 45
#define CRADLE_LENGTH 40
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
int moveForward = 0;
int moveBack = 0;
int turnLeft = 0;
int turnRight = 0;

Vector museumPillarVertices[MUSEUM_PILLAR_SIDES * 2];
Vector museumPillarNormals[MUSEUM_PILLAR_SIDES * 2];

int sceneTime = 90;
int metatravellerAngles[METATRAVELLER_COUNT];
bool metatravellerRingsEnabled = false;
int mobiusStripBallAngle = 0;
Vector mobiusStripVertices[74];
Vector mobiusStripNormals[74];

float cradleAngle = CRADLE_MAX_ANGLE;

float shadowColor[4] = {0.2, 0.2, 0.2, 1};

GLuint texIds[9];

void calcMetatravellerAngles()
{
	for (int i = 0; i < METATRAVELLER_COUNT; i++)
	{
		metatravellerAngles[i] = (metatravellerAngles[i] + METATRAVELLER_SPEED) % 360;
	}
}

void calculateCamPos()
{
	angle = (angle + (360 + (TURN_SPEED * (turnLeft + turnRight)))) % 360;
	cam_x += (cos(deg2rad(angle)) * MOVE_SPEED) * (moveForward + moveBack);
	cam_z += (sin(deg2rad(angle)) * MOVE_SPEED) * (moveForward + moveBack);

	cam_x = clamp(cam_x, -PLANE_X + PLANE_BOUNDARY, PLANE_X - PLANE_BOUNDARY);
	cam_z = clamp(cam_z, -PLANE_Z + PLANE_BOUNDARY, PLANE_Z - PLANE_BOUNDARY);
}

void timer(int value)
{
	calcMetatravellerAngles();
	calculateCamPos();
	mobiusStripBallAngle = (mobiusStripBallAngle + 1) % 720;
	cradleAngle = (-GRAVITY * ((CRADLE_MAX_ANGLE * 4) * sin(deg2rad(sceneTime * 2)))) / CRADLE_LENGTH;
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

void normalise(Vector* vec)
{
	float n = 0;
	if (vec->x >= vec->y && vec->x >= vec->z)
	{
		n = vec->x;
	}
	else if (vec->y >= vec->x && vec->y >= vec->z)
	{
		n = vec->y;
	}
	else
	{
		n = vec->z;
	}
	if (n == 0) n = 1;

	vec->x = vec->x / n;
	vec->y = vec->y / n;
	vec->z = vec->z / n;
}

Vector normal(Vector v1, Vector v2, Vector v3)
{
	float x1 = v1.x, x2 = v2.x, x3 = v3.x;
	float y1 = v1.y, y2 = v2.y, y3 = v3.y;
	float z1 = v1.z, z2 = v2.z, z3 = v3.z;
	float nx, ny, nz;
	nx = y1*(z2-z3) + y2*(z3-z1) + y3*(z1-z2);
	ny = z1*(x2-x3) + z2*(x3-x1) + z3*(x1-x2);
	nz = x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2);
	glNormal3f(nx, ny, nz);
	return { nx, ny, nz };
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
	ImageData brick1024 = loadTGAImageData("textures/brick/brick1024.tga");
	ImageData brick512 = loadTGAImageData("textures/brick/brick512.tga");
	ImageData brick256 = loadTGAImageData("textures/brick/brick256.tga");
	ImageData brick128 = loadTGAImageData("textures/brick/brick128.tga");
	ImageData brick64 = loadTGAImageData("textures/brick/brick64.tga");
	ImageData brick32 = loadTGAImageData("textures/brick/brick32.tga");
	ImageData brick16 = loadTGAImageData("textures/brick/brick16.tga");
	ImageData brick8 = loadTGAImageData("textures/brick/brick8.tga");
	ImageData brick4 = loadTGAImageData("textures/brick/brick4.tga");
	ImageData brick2 = loadTGAImageData("textures/brick/brick2.tga");
	ImageData brick1 = loadTGAImageData("textures/brick/brick1.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, brick1024.nbytes, brick1024.width, brick1024.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick1024.data);
	glTexImage2D(GL_TEXTURE_2D, 1, brick512.nbytes, brick512.width, brick512.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick512.data);
	glTexImage2D(GL_TEXTURE_2D, 2, brick256.nbytes, brick256.width, brick256.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick256.data);
	glTexImage2D(GL_TEXTURE_2D, 3, brick128.nbytes, brick128.width, brick128.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick128.data);
	glTexImage2D(GL_TEXTURE_2D, 4, brick64.nbytes, brick64.width, brick64.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick64.data);
	glTexImage2D(GL_TEXTURE_2D, 5, brick32.nbytes, brick32.width, brick32.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick32.data);
	glTexImage2D(GL_TEXTURE_2D, 6, brick16.nbytes, brick16.width, brick16.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick16.data);
	glTexImage2D(GL_TEXTURE_2D, 7, brick8.nbytes, brick8.width, brick8.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick8.data);
	glTexImage2D(GL_TEXTURE_2D, 8, brick4.nbytes, brick4.width, brick4.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick4.data);
	glTexImage2D(GL_TEXTURE_2D, 9, brick2.nbytes, brick2.width, brick2.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick2.data);
	glTexImage2D(GL_TEXTURE_2D, 10, brick1.nbytes, brick1.width, brick1.height, 0, GL_RGB, GL_UNSIGNED_BYTE, brick1.data);
	delete brick1024.data;
	delete brick512.data;
	delete brick256.data;
	delete brick128.data;
	delete brick64.data;
	delete brick32.data;
	delete brick16.data;
	delete brick8.data;
	delete brick4.data;
	delete brick2.data;
	delete brick1.data;

	// Concrete
	glBindTexture(GL_TEXTURE_2D, texIds[7]);
	ImageData concrete1024 = loadTGAImageData("textures/concrete/concrete1024.tga");
	ImageData concrete512 = loadTGAImageData("textures/concrete/concrete512.tga");
	ImageData concrete256 = loadTGAImageData("textures/concrete/concrete256.tga");
	ImageData concrete128 = loadTGAImageData("textures/concrete/concrete128.tga");
	ImageData concrete64 = loadTGAImageData("textures/concrete/concrete64.tga");
	ImageData concrete32 = loadTGAImageData("textures/concrete/concrete32.tga");
	ImageData concrete16 = loadTGAImageData("textures/concrete/concrete16.tga");
	ImageData concrete8 = loadTGAImageData("textures/concrete/concrete8.tga");
	ImageData concrete4 = loadTGAImageData("textures/concrete/concrete4.tga");
	ImageData concrete2 = loadTGAImageData("textures/concrete/concrete2.tga");
	ImageData concrete1 = loadTGAImageData("textures/concrete/concrete1.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, concrete1024.nbytes, concrete1024.width, concrete1024.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete1024.data);
	glTexImage2D(GL_TEXTURE_2D, 1, concrete512.nbytes, concrete512.width, concrete512.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete512.data);
	glTexImage2D(GL_TEXTURE_2D, 2, concrete256.nbytes, concrete256.width, concrete256.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete256.data);
	glTexImage2D(GL_TEXTURE_2D, 3, concrete128.nbytes, concrete128.width, concrete128.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete128.data);
	glTexImage2D(GL_TEXTURE_2D, 4, concrete64.nbytes, concrete64.width, concrete64.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete64.data);
	glTexImage2D(GL_TEXTURE_2D, 5, concrete32.nbytes, concrete32.width, concrete32.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete32.data);
	glTexImage2D(GL_TEXTURE_2D, 6, concrete16.nbytes, concrete16.width, concrete16.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete16.data);
	glTexImage2D(GL_TEXTURE_2D, 7, concrete8.nbytes, concrete8.width, concrete8.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete8.data);
	glTexImage2D(GL_TEXTURE_2D, 8, concrete4.nbytes, concrete4.width, concrete4.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete4.data);
	glTexImage2D(GL_TEXTURE_2D, 9, concrete2.nbytes, concrete2.width, concrete2.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete2.data);
	glTexImage2D(GL_TEXTURE_2D, 10, concrete1.nbytes, concrete1.width, concrete1.height, 0, GL_RGB, GL_UNSIGNED_BYTE, concrete1.data);
	delete concrete1024.data;
	delete concrete512.data;
	delete concrete256.data;
	delete concrete128.data;
	delete concrete64.data;
	delete concrete32.data;
	delete concrete16.data;
	delete concrete8.data;
	delete concrete4.data;
	delete concrete2.data;
	delete concrete1.data;

	// Sediment
	glBindTexture(GL_TEXTURE_2D, texIds[8]);
	ImageData sediment1024 = loadTGAImageData("textures/sediment/sediment1024.tga");
	ImageData sediment512 = loadTGAImageData("textures/sediment/sediment512.tga");
	ImageData sediment256 = loadTGAImageData("textures/sediment/sediment256.tga");
	ImageData sediment128 = loadTGAImageData("textures/sediment/sediment128.tga");
	ImageData sediment64 = loadTGAImageData("textures/sediment/sediment64.tga");
	ImageData sediment32 = loadTGAImageData("textures/sediment/sediment32.tga");
	ImageData sediment16 = loadTGAImageData("textures/sediment/sediment16.tga");
	ImageData sediment8 = loadTGAImageData("textures/sediment/sediment8.tga");
	ImageData sediment4 = loadTGAImageData("textures/sediment/sediment4.tga");
	ImageData sediment2 = loadTGAImageData("textures/sediment/sediment2.tga");
	ImageData sediment1 = loadTGAImageData("textures/sediment/sediment1.tga");
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, sediment1024.nbytes, sediment1024.width, sediment1024.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment1024.data);
	glTexImage2D(GL_TEXTURE_2D, 1, sediment512.nbytes, sediment512.width, sediment512.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment512.data);
	glTexImage2D(GL_TEXTURE_2D, 2, sediment256.nbytes, sediment256.width, sediment256.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment256.data);
	glTexImage2D(GL_TEXTURE_2D, 3, sediment128.nbytes, sediment128.width, sediment128.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment128.data);
	glTexImage2D(GL_TEXTURE_2D, 4, sediment64.nbytes, sediment64.width, sediment64.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment64.data);
	glTexImage2D(GL_TEXTURE_2D, 5, sediment32.nbytes, sediment32.width, sediment32.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment32.data);
	glTexImage2D(GL_TEXTURE_2D, 6, sediment16.nbytes, sediment16.width, sediment16.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment16.data);
	glTexImage2D(GL_TEXTURE_2D, 7, sediment8.nbytes, sediment8.width, sediment8.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment8.data);
	glTexImage2D(GL_TEXTURE_2D, 8, sediment4.nbytes, sediment4.width, sediment4.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment4.data);
	glTexImage2D(GL_TEXTURE_2D, 9, sediment2.nbytes, sediment2.width, sediment2.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment2.data);
	glTexImage2D(GL_TEXTURE_2D, 10, sediment1.nbytes, sediment1.width, sediment1.height, 0, GL_RGB, GL_UNSIGNED_BYTE, sediment1.data);
	delete sediment1024.data;
	delete sediment512.data;
	delete sediment256.data;
	delete sediment128.data;
	delete sediment64.data;
	delete sediment32.data;
	delete sediment16.data;
	delete sediment8.data;
	delete sediment4.data;
	delete sediment2.data;
	delete sediment1.data;
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
			glTexCoord2f(((x / PLANE_TILE_SIZE) + 0) / FLOOR_SCALE, ((z / PLANE_TILE_SIZE) + 0) / FLOOR_SCALE);
			glVertex3f(x, 0, z);
			
			glTexCoord2f(((x / PLANE_TILE_SIZE) + 0) / FLOOR_SCALE, ((z / PLANE_TILE_SIZE) + 1) / FLOOR_SCALE);
			glVertex3f(x, 0, z + PLANE_TILE_SIZE);
			
			glTexCoord2f(((x / PLANE_TILE_SIZE) + 1) / FLOOR_SCALE, ((z / PLANE_TILE_SIZE) + 1) / FLOOR_SCALE);
			glVertex3f(x + PLANE_TILE_SIZE, 0, z + PLANE_TILE_SIZE);
			
			glTexCoord2f(((x / PLANE_TILE_SIZE) + 1) / FLOOR_SCALE, ((z / PLANE_TILE_SIZE) + 0) / FLOOR_SCALE);
			glVertex3f(x + PLANE_TILE_SIZE, 0, z);
		}
	}
	glEnd();
	glEnable(GL_LIGHTING);
}

void drawMuseum(bool isShadow)
{
	if (isShadow)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
	}
	else 
	{
		glEnable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);
	}
	float angle = 360.0 / MUSEUM_SIDES;
	float wallLength = tan(deg2rad(angle / 2)) * MUSEUM_RADIUS * 2;
	for (int i = 1; i < MUSEUM_SIDES; i++)
	{
		glPushMatrix();
			glRotatef((angle * i) + 90, 0, 1, 0);
			glTranslatef(MUSEUM_RADIUS, 50, 0);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glBindTexture(GL_TEXTURE_2D, texIds[6]);
			int numColumns = (int)ceil(wallLength / 20.0);

			glPushMatrix();
				glDisable(GL_LIGHT0);
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
				glEnable(GL_LIGHT0);
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
		float xScale = 1;
		float yScale = 5;

		// Draw cylinder
		glPushMatrix();
			glRotatef((angle * i), 0, 1, 0);
			glTranslatef(pillarDistance, 0, 0);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glBindTexture(GL_TEXTURE_2D, texIds[8]);
			glBegin(GL_QUADS);
				for (int j = 0; j < MUSEUM_PILLAR_SIDES; j++)
				{
					Vector v1 = museumPillarVertices[2 * j];
					Vector v1n = museumPillarNormals[2 * j];
					Vector v2 = museumPillarVertices[(2 * j + 1) % (MUSEUM_PILLAR_SIDES * 2)];
					Vector v2n = museumPillarNormals[(2 * j + 1) % (MUSEUM_PILLAR_SIDES * 2)];
					Vector v3 = museumPillarVertices[(2 * j + 3) % (MUSEUM_PILLAR_SIDES * 2)];
					Vector v3n = museumPillarNormals[(2 * j + 3) % (MUSEUM_PILLAR_SIDES * 2)];
					Vector v4 = museumPillarVertices[(2 * j + 2) % (MUSEUM_PILLAR_SIDES * 2)];
					Vector v4n = museumPillarNormals[(2 * j + 2) % (MUSEUM_PILLAR_SIDES * 2)];

					glNormal3f(v1n.x, v1n.y, v1n.z);
					glTexCoord2f((xScale / MUSEUM_PILLAR_SIDES) * j, 0);
					glVertex3f(v1.x, v1.y, v1.z);
					
					glNormal3f(v2n.x, v2n.y, v2n.z);
					glTexCoord2f((xScale / MUSEUM_PILLAR_SIDES) * j, 1);
					glVertex3f(v2.x, v2.y, v2.z);
					
					glNormal3f(v3n.x, v3n.y, v3n.z);
					glTexCoord2f((xScale / MUSEUM_PILLAR_SIDES) * (j + 1), 1);
					glVertex3f(v3.x, v3.y, v3.z);
					
					glNormal3f(v4n.x, v4n.y, v4n.z);
					glTexCoord2f((xScale / MUSEUM_PILLAR_SIDES) * (j + 1), 0);
					glVertex3f(v4.x, v4.y, v4.z);
				}
			glEnd();
		glPopMatrix();
	}
	// glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	// roof
	// TODO: change glutSolidCone to gluCylinder (for texcoords)
	if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
		else glColor3f(0.3, 0.3, 0.3);
	glPushMatrix();
		glTranslatef(0, 100, 0);
		glRotatef(-90, 1, 0, 0);
		glutSolidCone(pillarDistance + 20, 100, MUSEUM_SIDES, MUSEUM_SIDES);
	glPopMatrix();

	// floor
	// TODO: generate floor using points and texcoords
	if (!isShadow)
	{
		// glColor3f(0.5, 0, 0.8);
		glColor3f(0.5, 0, 0);
		glPushMatrix();
			glTranslatef(0, -0.98, 0);
			glRotatef(-90, 1, 0, 0);
			glutSolidCylinder(pillarDistance, 1, MUSEUM_SIDES, MUSEUM_SIDES);
		glPopMatrix();
	}
}

void drawPlatform()
{
	glPushMatrix();
		glColor3f(0.8, 0.8, 0.8);

		glPushMatrix();
			glTranslatef(-60, 5.05, -40);
			glBegin(GL_QUADS);
				for (int x = 0; x < 120; x++)
				{
					for (int z = 0; z < 80; z++)
					{
						glNormal3f(0, 1, 0);
						glVertex3f(x, 0, z);
						glVertex3f(x, 0, z + 1);
						glVertex3f(x + 1, 0, z + 1);
						glVertex3f(x + 1, 0, z);
					}
				}
			glEnd();
		glPopMatrix();

		glScalef(120, 10, 80);
		glutSolidCube(1);
	glPopMatrix();
}

void drawMetatravellers(bool isShadow)
{
	glPushMatrix();
		glTranslatef(0, 0, 120);
		if (!isShadow)
		{
			drawPlatform();

			glPushMatrix();
				glDisable(GL_LIGHTING);
				float textScale = 0.05;
				float textWidth = 0;
				unsigned char message[] = "Press 'E' to toggle rings";
				for (int i = 0; i < sizeof(message) / sizeof(message[0]); i++)
				{
					textWidth += (glutStrokeWidth(GLUT_STROKE_ROMAN, message[i]) * textScale);
				}
				glTranslatef(textWidth / 2, 10, -40);
				glColor3f(1, 1, 1);
				glScalef(-textScale, textScale, 1);
				glutStrokeString(GLUT_STROKE_ROMAN, message);
				glEnable(GL_LIGHTING);
			glPopMatrix();
		}

		glPushMatrix();
			glTranslatef(0, 30, 0);
			for (int i = 0; i < METATRAVELLER_COUNT; i++)
			{
				if (metatravellerRingsEnabled)
				{
					if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
						else glColor3f(1, 0.9, 0.3);
					glPushMatrix();
						glRotatef(i * (360.0 / METATRAVELLER_COUNT), 0, 1, 0);
						glTranslatef(0, 0, 20);
						glRotatef(90, 0, 1, 0);
						glutSolidTorus(0.1, 5, 4, 36);
					glPopMatrix();
				}
				
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.8, 0, 0.8);
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

void drawMobiusStrip(bool isShadow)
{
	glPushMatrix();
		glTranslatef(120, 0, 0);
		glRotatef(90, 0, 1, 0);
		// Base
		if (!isShadow)
		{
			drawPlatform();
		}

		glPushMatrix();
			// Mobius Strip
			if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
				else glColor3f(0, 0.0, 0.8);
			glTranslatef(0, 20, 0);
			glBegin(GL_TRIANGLES);
			for (int i = 0; i < 37; i++)
			{
				Vector v1 = mobiusStripVertices[2 * i];
				Vector v1n = mobiusStripNormals[2 * i];
				Vector v2 = mobiusStripVertices[(2 * i + 1) % 74];
				Vector v2n = mobiusStripNormals[(2 * i + 1) % 74];
				Vector v3 = mobiusStripVertices[(2 * i + 2) % 74];
				Vector v3n = mobiusStripNormals[(2 * i + 2) % 74];
				Vector v4 = mobiusStripVertices[(2 * i + 3) % 74];
				Vector v4n = mobiusStripNormals[(2 * i + 3) % 74];

				glNormal3f(v1n.x, v1n.y, v1n.z); glVertex3f(v1.x, v1.y, v1.z);
				glNormal3f(v2n.x, v2n.y, v2n.z); glVertex3f(v2.x, v2.y, v2.z);
				glNormal3f(v3n.x, v3n.y, v3n.z); glVertex3f(v3.x, v3.y, v3.z);

				glNormal3f(v4n.x, v4n.y, v4n.z); glVertex3f(v4.x, v4.y, v4.z);
				glNormal3f(v3n.x, v3n.y, v3n.z); glVertex3f(v3.x, v3.y, v3.z);
				glNormal3f(v2n.x, v2n.y, v2n.z); glVertex3f(v2.x, v2.y, v2.z);
			}
			glEnd();
			
			// Balls
			float angleOffset = 720.0 / MOBIUS_STRIP_BALLS;
			for (int i = 0; i < MOBIUS_STRIP_BALLS; i++)
			{
				glPushMatrix();
					if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
						else glColor3f(0.6, 0.6, 0.6);
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

void drawNewtonsCradle(bool isShadow)
{
	glPushMatrix();
		glTranslatef(-120, 0, 0);
		glRotatef(-90, 0, 1, 0);
		// Base
		if (!isShadow)
		{
			drawPlatform();
		}

		// // Pendulums
		glPushMatrix();
			glTranslatef(0, 10, 0);

			float white[4] = { 1, 1, 1, 1 };
			float spotlightPos[4] = { 0, 0, 0, 1.0 }; 
			float spotDir[3] = { 0, -1, 0 };

			glPushMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.5, 0.5, 0.5);
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
				if (isShadow)
				{
					glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);	
				}
				else
				{
					glColor3f(1, 1, 0.8);
					glDisable(GL_LIGHTING);
				}
				glutSolidSphere(3, 12, 12);
				if (!isShadow)
				{
					glEnable(GL_LIGHTING);
					glPushMatrix();
						glLightfv(GL_LIGHT2, GL_DIFFUSE, white);
						glLightfv(GL_LIGHT2, GL_SPECULAR, white);
						glLightfv(GL_LIGHT2, GL_POSITION, spotlightPos);
						glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spotDir);
						glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 15);
						glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 100);
					glPopMatrix();
				}
			glPopMatrix();

			glPushMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.5, 0.5, 0.5);
				glTranslatef(-6, 0, 0);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.5, 0.5, 0.5);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.5, 0.5, 0.5);
				glTranslatef(6, 0, 0);
				glPushMatrix();
					glRotatef(-70, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				glPushMatrix();
					glRotatef(-110, 1, 0, 0);
					glutSolidCylinder(0.5, CRADLE_LENGTH, 12, 12);
				glPopMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.8, 0.8, 0.8);
				glutSolidSphere(3, 12, 12);
			glPopMatrix();

			glPushMatrix();
				if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
					else glColor3f(0.5, 0.5, 0.5);
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
				if (isShadow)
				{
					glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);	
				}
				else
				{
					glColor3f(1, 1, 0.8);
					glDisable(GL_LIGHTING);
				}
				glutSolidSphere(3, 12, 12);
				if (!isShadow)
				{
					glEnable(GL_LIGHTING);
					glPushMatrix();
						glLightfv(GL_LIGHT3, GL_DIFFUSE, white);
						glLightfv(GL_LIGHT3, GL_SPECULAR, white);
						glLightfv(GL_LIGHT3, GL_POSITION, spotlightPos);
						glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, spotDir);
						glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 15);
						glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 100);
					glPopMatrix();
				}
			glPopMatrix();
		glPopMatrix();

		// Frame
		glPushMatrix();
			if (isShadow) glColor4f(shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3]);
				else glColor3f(0.5, 0.5, 0.5);
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

void drawCeilingLight()
{
	glPushMatrix();
		glTranslatef(0, 92, 0);
		glPushMatrix();
			glRotatef(90, -1, 0, 0);
			glColor3f(0.4, 0.4, 0.4);
			glutSolidCone(10, 15, 12, 12);
		glPopMatrix();
		glPushMatrix();
			glDisable(GL_LIGHTING);
			glRotatef(90, -1, 0, 0);
			glColor3f(1, 1, 0.8);
			glutSolidSphere(4, 12, 12);
			glEnable(GL_LIGHTING);
		glPopMatrix();
	glPopMatrix();
}

void initialisePillars()
{
	// Calculate vertex positions
	for (int i = 0; i < MUSEUM_PILLAR_SIDES; i++)
	{
		museumPillarVertices[2 * i] = { 10, 100, 0 };
		museumPillarVertices[2 * i + 1] = { 10, 0, 0 };

		rotateVectorY(&museumPillarVertices[2 * i], (360.0 / MUSEUM_PILLAR_SIDES) * i);
		rotateVectorY(&museumPillarVertices[2 * i + 1], (360.0 / MUSEUM_PILLAR_SIDES) * i);
	}
	
	// Calculate vertex normals
	for (int i = 0; i < MUSEUM_PILLAR_SIDES; i++)
	{
		Vector v1 = museumPillarVertices[2 * i];
		Vector v2 = museumPillarVertices[(2 * i + 1) % (MUSEUM_PILLAR_SIDES * 2)];
		Vector v3 = museumPillarVertices[(2 * i + 3) % (MUSEUM_PILLAR_SIDES * 2)];

		Vector n = normal(v1, v2, v3);
		museumPillarNormals[2 * i].x += n.x;
		museumPillarNormals[2 * i].y += n.y;
		museumPillarNormals[2 * i].z += n.z;

		museumPillarNormals[(2 * i + 1) % (MUSEUM_PILLAR_SIDES * 2)].x += n.x;
		museumPillarNormals[(2 * i + 1) % (MUSEUM_PILLAR_SIDES * 2)].y += n.y;
		museumPillarNormals[(2 * i + 1) % (MUSEUM_PILLAR_SIDES * 2)].z += n.z;

		museumPillarNormals[(2 * i + 2) % (MUSEUM_PILLAR_SIDES * 2)].x += n.x;
		museumPillarNormals[(2 * i + 2) % (MUSEUM_PILLAR_SIDES * 2)].y += n.y;
		museumPillarNormals[(2 * i + 2) % (MUSEUM_PILLAR_SIDES * 2)].z += n.z;

		museumPillarNormals[(2 * i + 3) % (MUSEUM_PILLAR_SIDES * 2)].x += n.x;
		museumPillarNormals[(2 * i + 3) % (MUSEUM_PILLAR_SIDES * 2)].y += n.y;
		museumPillarNormals[(2 * i + 3) % (MUSEUM_PILLAR_SIDES * 2)].z += n.z;
	}

	// Normalise vertex normals
	for (int i = 0; i < MUSEUM_PILLAR_SIDES * 2; i++)
	{
		normalise(&museumPillarNormals[i]);
	}
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
	// Calculate vertex positions
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

	// Calculate vertex normals
	for (int i = 0; i < 37; i++)
	{
		Vector v1 = mobiusStripVertices[2 * i];
		Vector v2 = mobiusStripVertices[(2 * i + 1) % 74];
		Vector v3 = mobiusStripVertices[(2 * i + 2) % 74];
		Vector v4 = mobiusStripVertices[(2 * i + 3) % 74];

		Vector n1 = normal(v1, v2, v3);
		mobiusStripNormals[2 * i].x += n1.x;
		mobiusStripNormals[2 * i].y += n1.y;
		mobiusStripNormals[2 * i].z += n1.z;

		mobiusStripNormals[(2 * i + 1) % 74].x += n1.x;
		mobiusStripNormals[(2 * i + 1) % 74].y += n1.y;
		mobiusStripNormals[(2 * i + 1) % 74].z += n1.z;

		mobiusStripNormals[(2 * i + 2) % 74].x += n1.x;
		mobiusStripNormals[(2 * i + 2) % 74].y += n1.y;
		mobiusStripNormals[(2 * i + 2) % 74].z += n1.z;
		
		Vector n2 = normal(v4, v3, v2);
		mobiusStripNormals[(2 * i + 3) % 74].x += n2.x;
		mobiusStripNormals[(2 * i + 3) % 74].y += n2.y;
		mobiusStripNormals[(2 * i + 3) % 74].z += n2.z;

		mobiusStripNormals[(2 * i + 2) % 74].x += n2.x;
		mobiusStripNormals[(2 * i + 2) % 74].y += n2.y;
		mobiusStripNormals[(2 * i + 2) % 74].z += n2.z;

		mobiusStripNormals[(2 * i + 1) % 74].x += n2.x;
		mobiusStripNormals[(2 * i + 1) % 74].y += n2.y;
		mobiusStripNormals[(2 * i + 1) % 74].z += n2.z;
	}

	// Normalise vertex normals
	for (int i = 0; i < 74; i++)
	{
		normalise(&mobiusStripNormals[i]);
	}
}

void display()
{
	float innerLightPos[4] = {0., 90., 0., 1.0};  //light's position
	float lightDir[4] = {0, -1, -1, 0};

	float white[4] = {1, 1, 1, 1};
	float grey[4] {0.5, 0.5, 0.5, 1};
	float black[4] = {0, 0, 0, 1};

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);    //GL_LINE = Wireframe;   GL_FILL = Solid
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float look_x = cos(deg2rad(angle)) * 200;
	float look_z = sin(deg2rad(angle)) * 200;

	gluLookAt(cam_x, cam_y, cam_z, cam_x + look_x, LOOK_HEIGHT, cam_z + look_z, 0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightDir);

	glLightfv(GL_LIGHT1, GL_DIFFUSE, grey);
	glLightfv(GL_LIGHT1, GL_SPECULAR, grey);
	glLightfv(GL_LIGHT1, GL_POSITION, innerLightPos);

	drawSkybox();

	drawFloor();

	// Shadows
	float lightPos[3] = {120, 90, 0};
	float shadowMatrix[16] = {
		lightPos[1], 0, 0, 0,
		-lightPos[0], 0, -lightPos[2], -1,
		0, 0, lightPos[1], 0,
		0, 0, 0, lightPos[1]
	};

	glDisable(GL_LIGHTING);
	glPushMatrix();
		glTranslatef(0, 5.1, 0);
		glMultMatrixf(shadowMatrix);
		drawMobiusStrip(true);
	glPopMatrix();
	
	glPushMatrix();
		glTranslatef(0, 0.01, 0);
		lightPos[0] = 0;
		lightPos[1] = 500;
		lightPos[2] = -500;
		shadowMatrix[0] = lightPos[1];
		shadowMatrix[4] = -lightPos[0];
		shadowMatrix[6] = -lightPos[2];
		shadowMatrix[10] = lightPos[1];
		shadowMatrix[15] = lightPos[1];

		glMultMatrixf(shadowMatrix);
		drawMuseum(true);
	glPopMatrix();
	

	// glPushMatrix();
	// 	glTranslatef(0, 5.1, 0);
	// 	lightPos[0] = -120;
	// 	lightPos[1] = 90;
	// 	lightPos[2] = 0;
	// 	shadowMatrix[0] = lightPos[1];
	// 	shadowMatrix[4] = -lightPos[0];
	// 	shadowMatrix[6] = -lightPos[2];
	// 	shadowMatrix[10] = lightPos[1];
	// 	shadowMatrix[15] = lightPos[1];

	// 	glMultMatrixf(shadowMatrix);
	// 	drawNewtonsCradle(true);
	// glPopMatrix();
	
	glPushMatrix();
		glTranslatef(0, 5.1, 0);
		lightPos[0] = 0;
		lightPos[1] = 90;
		lightPos[2] = 120;
		shadowMatrix[0] = lightPos[1];
		shadowMatrix[4] = -lightPos[0];
		shadowMatrix[6] = -lightPos[2];
		shadowMatrix[10] = lightPos[1];
		shadowMatrix[15] = lightPos[1];

		glMultMatrixf(shadowMatrix);
		drawMetatravellers(true);
	glPopMatrix();
	glEnable(GL_LIGHTING);

	// Draw
	glPushMatrix();
		glEnable(GL_LIGHT0);
		drawMuseum(false);
		glDisable(GL_LIGHT0);

		drawMetatravellers(false);
		drawMobiusStrip(false);
		drawNewtonsCradle(false);
		drawCeilingLight();
	glPopMatrix();

	glutSwapBuffers();
}

void initialize()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	loadTextures();
	initialisePillars();
	initialiseMetatravellers();
	initialiseMobiusStrip();

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_LIGHT3);
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
			turnLeft = -1;
			break;
		case GLUT_KEY_RIGHT:
			turnRight = 1;
			break;
		case GLUT_KEY_UP:
			moveForward = 1;
			break;
		case GLUT_KEY_DOWN:
			moveBack = -1;
			break;
	}
}

void specialUp(int key, int x, int y)
{
	switch (key)
	{
		case GLUT_KEY_LEFT:
			turnLeft = 0;
			break;
		case GLUT_KEY_RIGHT:
			turnRight = 0;
			break;
		case GLUT_KEY_UP:
			moveForward = 0;
			break;
		case GLUT_KEY_DOWN:
			moveBack = 0;
			break;
	}
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'e':
			metatravellerRingsEnabled = !metatravellerRingsEnabled;
			break;
	}
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
   glutSpecialUpFunc(specialUp);
   glutKeyboardFunc(keyboard);
   glutTimerFunc(10, timer, 0);
   glutMainLoop();
   return 0;
}