/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Some GLUT spheres rolling along the Z plane with misc. effects.
 *
 *
 * Copyright 2006 - 2020 J Rick Ramstetter, rick.ramstetter@gmail.com
 * Copyright 2006 Guy Kisel, beefofages@gmail.com
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glut.h>

/* Redbook includes (see ./subprojects/) */
#include "accpersp.h"
#include "checker.h"
#include "jitter.h"

static const GLfloat g_colors[][4] = {
		{ 0.7, 0.7, 0.0, 1.0 },
		{ 0.0, 0.7, 0.7, 1.0 },
		{ 0.0, 0.7, 0.0, 1.0 },
		{ 0.0, 0.0, 0.7, 1.0 }
};
static const char* g_colorNames[] = {
		"Yellow", "GreenBlue", "Green", "Blue"
};

/* the color of warp 9 */
static const GLfloat g_red[4] = { 0.7, 0.0, 0.0, 1.0 };

static const GLint g_fpsTarget = 40;

struct UserSettings
{
  GLuint enableAA; /* 0 if disabled, a jitter index from jitter.h otherwise */
  GLuint enableDOF; /* 0 if disabled, 1-250 otherwise */
  GLuint enableBlur; /* count of frames to motion blur on hit */
  GLuint debug; /* 1 if enabled */
  GLfloat fovAngle;
  GLuint hitDuration; /* time in milliseconds that hits are reported */
  GLuint focus;
};

struct Sphere
{
  GLuint hit; /* if 1 glut reports a GL_PROJECTION hit */
  GLfloat zDistance;
  GLfloat rotation; /* X rotation (roll effect) */
  GLint colorIdx; /* see g_colors[] */
  GLint glName; /* unique id for glPushName */
  GLfloat zSpeed; /* How fast to move on Z */
  GLfloat zSpeedDefault;
  GLfloat radius;
  GLfloat xOffset;
};

struct State
{
  GLuint fps;
  GLuint baseTime;
  GLuint framesRendered; /* does not include motion blur or jitter frames */
};

struct CheckerboardFloor
{
	GLuint texName;
	GLuint width;
	GLuint height;
	GLubyte *image;
};

static struct UserSettings g_userSettings;
static struct Sphere g_spheres[2];
static struct State g_state;
static struct CheckerboardFloor g_floor;

static unsigned int RandomInt1to20()
{
	static int didSrand = 0;
	if (didSrand)
		goto srand;

	FILE* handle = fopen("/dev/urandom", "r");
	if(!handle)
		goto srand;

	unsigned int random = 0;
	size_t bytes = 0;
	for (int i = 0; 0 == bytes && i < 50; ++i)
	{
		bytes = fread(&random, 1, sizeof(random), handle);
		usleep(2);
	}
	if (0 == bytes)
		goto srand;

	goto out;

srand:
	if (!didSrand)
	{
		printf("Warning: using rand() instead of /dev/urandom\n");
		srand(time(NULL));
		didSrand = 1;
	}

	random = rand();
out:

	return (random % 20) + 1;
}

static void ResetData()
{
	/* User settings */
	memset(&g_userSettings, 0, sizeof(g_userSettings));
	g_userSettings.fovAngle = 50.0;
	g_userSettings.hitDuration = 500; /* millisec */

	/* Program state */
	memset(&g_state, 0, sizeof(g_state));

	/* Spheres */
	memset(&g_spheres, 0, sizeof(g_spheres));

	/* TODO make dynamic to support n>2 spheres */
	g_spheres[0].xOffset = -2.0;
	g_spheres[1].xOffset = 2.0;

	for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
	{
		struct Sphere* sphere = &g_spheres[i];
		sphere->glName = i + 1;
		sphere->colorIdx = i % ( sizeof(g_colors) / sizeof(g_colors[0]) );
		sphere->zSpeed = RandomInt1to20() / 40.0f;
		sphere->zSpeedDefault = sphere->zSpeed;
		sphere->radius = 1.0;

		printf("Set sphere %d to color %s (idx: %d), speed %f, radius %f, offset %f\n",
				sphere->glName,
				g_colorNames[sphere->colorIdx], sphere->colorIdx,
				sphere->zSpeed,
				sphere->radius,
				sphere->xOffset);
	}
}

static void InitData()
{
	ResetData();

	memset(&g_floor, 0, sizeof(g_floor));
	g_floor.width = 512;
	g_floor.height = 1024;
	int size = sizeof(GLubyte) * 4 * g_floor.width * g_floor.height;
	g_floor.image = malloc(size);
	memset(g_floor.image, 0, size);
	makeCheckImage(g_floor.image, g_floor.width, g_floor.height);
}

static void InitGL()
{
	GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);

	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, 50.0);

	GLfloat light_position[] = { 0.0, 20.0, 0.0, 1.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	GLfloat lm_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel (GL_FLAT);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &g_floor.texName);
	glBindTexture(GL_TEXTURE_2D, g_floor.texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_floor.width, g_floor.height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, g_floor.image);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearAccum(0.0, 0.0, 0.0, 0.0);
}


static void Cleanup()
{
	if (g_floor.image)
	{
		free(g_floor.image);
		g_floor.image = 0;
	}
}


static void PrintData()
{
	if (g_userSettings.debug)
	{
		printf("\n=====================\n");
		printf("Time: %d\n", glutGet(GLUT_ELAPSED_TIME));
		printf("FPS: %u\n", g_state.fps);
		printf("AA jitter: %u\n", g_userSettings.enableAA);
		printf("Depth of field: %u\n", g_userSettings.enableDOF);
		printf("FoV angle: %f\n", g_userSettings.fovAngle);
		printf("Motion blur: %u\n", g_userSettings.enableBlur);
	}
	glutTimerFunc(1000, PrintData, 0);
}


static void UpdateFps()
{
	++g_state.framesRendered;
	GLuint fpsTimeCurrent = glutGet(GLUT_ELAPSED_TIME);
	if (fpsTimeCurrent - g_state.baseTime > 1000)
	{
		g_state.fps = ceil( g_state.framesRendered*1000.0 /
						 (fpsTimeCurrent-g_state.baseTime) );
		g_state.baseTime = fpsTimeCurrent;
		g_state.framesRendered = 0;
	}
}


static void SphereTimeStep(struct Sphere* sphere, GLfloat factor)
{
	if (sphere->hit &&
		((glutGet(GLUT_ELAPSED_TIME) - sphere->hit) > g_userSettings.hitDuration))
	{
		sphere->hit = 0;
		sphere->zSpeed = sphere->zSpeedDefault;
	}

	GLfloat step = sphere->zSpeed * factor;
	sphere->zDistance -= step;
	assert(sphere->zDistance <= 0.0);

	sphere->rotation = (sphere->zDistance / sphere->radius) * (180.0 / M_PI);
	if (sphere->zDistance < -48.0)
	{
		sphere->zDistance = 0.0;
		sphere->rotation = 0.0;
	}
}


void TimerTimeStep(int x)
{
	for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
	{
		struct Sphere *sphere = &g_spheres[i];
		if (g_userSettings.enableBlur && sphere->hit)
			continue;
		SphereTimeStep(sphere, 1.0f);
	}

	glutPostRedisplay();
	glutTimerFunc(1000 / g_fpsTarget, TimerTimeStep, 0);
}


void RenderSphere(struct Sphere* sphere)
{
	glPushMatrix();
	glPushName(sphere->glName); 

	glMaterialfv(GL_FRONT, GL_DIFFUSE, sphere->hit ?
			g_red : g_colors[sphere->colorIdx]);

	glTranslatef (sphere->xOffset, -1.0, sphere->zDistance);

	glRotatef (sphere->rotation, 1.0, 0.0, 0.0);
	glRotatef (90.0, 0.0, 1.0, 0.0);
	glutSolidSphere (sphere->radius, 24, 24);

	glPopName();
	glPopMatrix();
}


static void RenderObjects()
{
	glInitNames();
	for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
	{
		struct Sphere *sphere = &g_spheres[i];
		RenderSphere(sphere);
	}
}


static void RenderFloor()
{
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, g_floor.texName);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-5.0, -2.0, -50.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-5.0, -2.0, 1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(5.0, -2.0, 1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(5.0, -2.0, -50.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}


static jitter_point* JitterArray(GLint match)
{
	jitter_point* rv = 0;
	for (int test = match; rv == 0 && test <= 66; ++test)
	{
		switch(test)
		{
			case 2:
				rv = j2;
				break;
			case 4:
				rv = j4;
				break;
			case 8:
				rv = j8;
				break;
			case 15:
				rv = j15;
				break;
			case 24:
				rv = j24;
				break;
			case 66:
				rv = j66;
				break;
			default:
				rv = 0;
				continue;
				break;
		}
	}
	assert(rv);
	return rv;
}


static void SimplePerspective(GLint* viewport)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(g_userSettings.fovAngle,
			(GLdouble) viewport[2] / (GLdouble) viewport[3], 1.0, 100.0);
}

static void SimpleDisplay(GLint *viewport)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	RenderFloor();
	RenderObjects();
	glutSwapBuffers();
}


static void GlutDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLint viewport[4];
	glGetIntegerv (GL_VIEWPORT, viewport);

	if (0 == g_userSettings.enableAA &&
		0 == g_userSettings.enableDOF &&
		0 == g_userSettings.enableBlur)
	{
		SimplePerspective(viewport);
		SimpleDisplay(viewport);
		goto finish;
	}

	/*
	 * This and the prior case are implemented individually only as a proof-of-
	 * concept, the SGI accPerspective based code below could handle these
	 * two cases with little change
	 */
	if (0 == g_userSettings.enableAA &&
		0 == g_userSettings.enableDOF &&
		0 != g_userSettings.enableBlur)
	{
		int hasBlur = 0;
		for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
		{
			struct Sphere *sphere = &g_spheres[i];
			if (sphere->hit)
				++hasBlur;
		}
		if (0 == hasBlur)
		{
			SimplePerspective(viewport);
			SimpleDisplay(viewport);
			goto finish;
		}

		glClear(GL_ACCUM_BUFFER_BIT);
		SimplePerspective(viewport);
		glMatrixMode(GL_MODELVIEW);

		for (int j = 0; j < 10; ++j)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			RenderFloor();

			for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
			{
				struct Sphere *sphere = &g_spheres[i];
				if (sphere->hit)
				{
					GLfloat factor = (-sphere->zDistance + 1.0) / 48.0;
					SphereTimeStep(sphere, factor);
				}
				RenderSphere(sphere);
			}

			glAccum(GL_ACCUM, 1.0 / 10.0f);
		}

		glAccum(GL_RETURN, 1.0);
		glutSwapBuffers();
		goto finish;
	}

	/* The rest is taken from redbook exercises */

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_ACCUM_BUFFER_BIT);

	GLuint jitterMax = g_userSettings.enableAA ? g_userSettings.enableAA : 8;
	for (GLuint jitter = 0; jitter < jitterMax; ++jitter)
	{
		GLdouble pixdx = 0.0;
		GLdouble pixdy = 0.0;
		GLdouble eyex = 0.0;
		GLdouble eyey = 0.0;
		jitter_point* jitAry = JitterArray(jitterMax);

		if (g_userSettings.enableAA)
		{
			pixdx = jitAry[jitter].x;
			pixdy = jitAry[jitter].y;
		}

		if (g_userSettings.enableDOF)
		{
			eyex = 0.33 * jitAry[jitter].x;
			eyey = 0.33 * jitAry[jitter].y;
		}

		accPerspective (g_userSettings.fovAngle,
				(GLdouble) viewport[2]/(GLdouble) viewport[3],
				1.0, 100.0,
				pixdx, pixdy,
				eyex, eyey,
				g_userSettings.focus + 1);

		glMatrixMode(GL_MODELVIEW);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		RenderFloor();

		for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
		{
			struct Sphere *sphere = &g_spheres[i];
			if (g_userSettings.enableBlur && sphere->hit)
			{
				GLfloat factor = (-sphere->zDistance + 1.0) / 4.0;
				SphereTimeStep(sphere, factor);
			}

			RenderSphere(sphere);
		}

		glAccum(GL_ACCUM, 1.0 / jitterMax);
	}
	glAccum (GL_RETURN, 1.0);
	glutSwapBuffers();

finish:
	UpdateFps();
}


static void Reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}


static void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 27:
			Cleanup();
			exit(0);
			break;

		case '=':
		case '+':
			g_userSettings.focus += 5;
			printf("%c: focus is %u\n", key, g_userSettings.focus);
			break;

		case '-':
		case '_':
			if (g_userSettings.focus <= 5)
				g_userSettings.focus = 0;
			else
				g_userSettings.focus -= 5;
			printf("%c: focus is %u\n", key, g_userSettings.focus);
			break;

		case '}':
		case ']':
			g_userSettings.fovAngle += 2.0;
			if (g_userSettings.fovAngle > 100.0)
				g_userSettings.fovAngle = 100.0;
			printf("%c: FOV angle is %f\n", key, g_userSettings.fovAngle);
			break;

		case '{':
		case '[':
			g_userSettings.fovAngle -= 2.0;
			if (g_userSettings.fovAngle < 10.0)
				g_userSettings.fovAngle = 10.0;
			printf("%c: FOV angle is %f\n", key, g_userSettings.fovAngle);
			break;

		case '0':
			g_userSettings.enableAA = 0;
			printf("%c: Disabled AA\n", key);
			break;

		case '1':
		case '2':
		case '3':
			g_userSettings.enableAA = pow(2, key - '0');
			printf("%c: Enabled %dX AA\n", key, g_userSettings.enableAA);
			break;

		case '4':
			g_userSettings.enableAA = 15;
			printf("%c: Enabled %dX AA\n", key, g_userSettings.enableAA);
			break;
		case '5':
			g_userSettings.enableAA = 24;
			printf("%c: Enabled %dX AA\n", key, g_userSettings.enableAA);
			break;
		case '6':
			g_userSettings.enableAA = 66;
			printf("%c: Enabled %dX AA\n", key, g_userSettings.enableAA);
			break;

		case 'r':
		case 'R':
			ResetData();
			printf("%c: Reset state\n", key);
			break;

		case 'd':
		case 'D':
			g_userSettings.debug = g_userSettings.debug ? 0 : 1;
			printf("%c: %s debug output\n", key, g_userSettings.debug ? "Enabled" : "Disabled");
			break;

		case 'b':
		case 'B':
			g_userSettings.enableBlur = g_userSettings.enableBlur ? 0 : 1;
			printf("%c: %s motion blur\n", key, g_userSettings.enableBlur ? "Enabled" : "Disabled");
			break;

		case 'f':
		case 'F':
			g_userSettings.enableDOF = g_userSettings.enableDOF ? 0 : 1;
			printf("%c: %s depth of field \n", key, g_userSettings.enableDOF ? "Enabled" : "Disabled");
			break;

		default:
			break;
	}
}


static void Mouse(int button, int state, int x, int y)
{
	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
		return;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	GLuint selectBuf[512];
	glSelectBuffer(512, selectBuf);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPickMatrix((GLdouble) x, (GLdouble) (viewport[3] - y),
			10.0, 10.0, viewport);

	gluPerspective(g_userSettings.fovAngle,
			(GLdouble) viewport[2]/(GLdouble) viewport[3],
			1.0, 100.0);

	glRenderMode(GL_SELECT);
	RenderObjects();
	GLint hits = glRenderMode(GL_RENDER);
	if (hits > 0)
	{
		for (int i = 0; i < (sizeof(g_spheres) / sizeof(g_spheres[0])); ++i)
		{
			struct Sphere *sphere = &g_spheres[i];
			if (sphere->glName == selectBuf[3] ||
				(hits > 1 && sphere->glName == selectBuf[7]))
			{
				if (g_userSettings.debug)
				  printf("clicked sphere %d\n", i);

				sphere->hit = glutGet(GLUT_ELAPSED_TIME);
				sphere->zSpeed *= 2;
			}
		}
	}
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_ACCUM | GLUT_DEPTH);
	glutInitWindowSize (1024, 1024);
	glutInitWindowPosition (100, 100);
	glutCreateWindow (argv[0]);
	InitData();
	InitGL();
	glutReshapeFunc(Reshape);
	glutDisplayFunc(GlutDisplay);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutTimerFunc(1000, PrintData, 0);
	glutTimerFunc(1000 / g_fpsTarget, TimerTimeStep, 0);
	glutMainLoop();
	Cleanup();
	return 0;
}
