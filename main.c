/**
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright 2008-2020 J Rick Ramstetter, rick.ramstetter@gmail.com
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

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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

struct UserSettings
{
  GLuint enableAA;
  GLuint enableDOF;
  GLuint blur;
  GLuint debug;
};

struct Sphere
{
  GLuint hit;
  GLuint blurTicks;
  GLfloat zDistance;
  GLfloat rotation;
  GLint colorIdx;
  GLint glName;
};

struct State
{
  GLuint fps;
  GLuint baseTime;
  GLuint framesElapsed;
};

struct CheckerboardFloor
{
	GLuint texName;
	GLuint width;
	GLuint height;
	GLubyte *image;
};

struct UserSettings g_userSettings;
struct Sphere g_spheres[2];
struct State g_state;
struct CheckerboardFloor g_floor;

static void InitSpheres()
{
	memset(&g_spheres, 0, sizeof(g_spheres));

	for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
	{
		struct Sphere* sphere = &g_spheres[i];
		sphere->glName = i + 1;
		sphere->colorIdx = i % ( sizeof(g_colors) / sizeof(g_colors[0]) );

		printf("Set sphere %d to color %s (idx: %d)\n",
				sphere->glName, g_colorNames[sphere->colorIdx],
				sphere->colorIdx);
	}
}


static void InitData()
{
	memset(&g_userSettings, 0, sizeof(g_userSettings));
	memset(&g_state, 0, sizeof(g_state));
	InitSpheres();

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

	GLfloat light_position[] = { 0.0, 0.0, 10.0, 1.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	GLfloat lm_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
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
	glTexImage2D(GL_TEXTURE_2D, 0, 4, g_floor.width, g_floor.height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, g_floor.image);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearAccum(0.0, 0.0, 0.0, 0.0);
}


static void cleanup()
{
	if (g_floor.image)
	{
		free(g_floor.image);
		g_floor.image = 0;
	}
}


void currentInfo()
{
	if (g_userSettings.debug)
	{
		printf("\n=====================\n");
		printf("Time: %d\n", glutGet(GLUT_ELAPSED_TIME));
		printf("FPS: %u\n", g_state.fps);
		printf("AA jitter: %u\n", g_userSettings.enableAA);
		printf("Depth of field: %u\n", g_userSettings.enableDOF);
		printf("Motion blur: %u%s\n", g_userSettings.blur,
				(g_userSettings.blur && !g_userSettings.enableAA &&
						!g_userSettings.enableDOF) ? " (AA&DoF off)" : ""
				);
	}
	glutTimerFunc(1000, currentInfo, 0);
}


void updateFps()
{
	if (!g_userSettings.debug)
		return;

	GLuint fpsTimeCurrent = glutGet(GLUT_ELAPSED_TIME);
	if (fpsTimeCurrent - g_state.baseTime > 1000)
	{
		g_state.fps = ceil( g_state.framesElapsed*1000.0 /
						 (fpsTimeCurrent-g_state.baseTime) );
		g_state.baseTime = fpsTimeCurrent;
		g_state.framesElapsed = 0;
	}
}


void SphereRotationAndZ(struct Sphere* sphere)
{
	GLfloat step = 0.1;
	if (sphere->hit &&
	    (!g_userSettings.blur ||
		 (!g_userSettings.enableAA && !g_userSettings.enableDOF)))
	{
		step = 1.5;
	}

	sphere->zDistance -= step;
	sphere->rotation = (g_spheres[0].zDistance / (2 * PI_) ) * 360;

	if(sphere->zDistance < -48)
	{
		sphere->zDistance = 0.0;
		sphere->rotation = 0;
	}
}


/*
NOTE:
Z distances for selected balls are handled in display()
such distances are dependant on the frame rate rather than
the elapsed time
*/
void timer25ms(int x) /* 1000 / 25ms = ~40fps */
{
	for (int i = 0; i < sizeof(g_spheres) / sizeof(g_spheres[0]); ++i)
	{
		SphereRotationAndZ(&g_spheres[i]);
	}

	glutPostRedisplay();

	glutTimerFunc(25, timer25ms, 0);
}

void RenderSphere(struct Sphere* sphere, GLfloat x_translate)
{
	glPushMatrix();
	glPushName(sphere->glName); 

	// if selected, handle the distance (e.g. motion blurring)
	if (sphere->hit)
	{
		if (g_userSettings.blur)
		{
			if (g_userSettings.enableAA)
			{
				sphere->blurTicks++;

				if (g_userSettings.enableAA == 2)
					sphere->zDistance -= 2;
				else if (g_userSettings.enableAA == 4)
					--sphere->zDistance;
				else
				{
					if (sphere->blurTicks >= ceil(g_userSettings.enableAA/4))
					{
						--sphere->zDistance;
						sphere->blurTicks = 0;
					}
				}

			}
			else if (g_userSettings.enableDOF)
				//assumed jitter is 8
			{
				sphere->blurTicks++;
				if (sphere->blurTicks >= 3)
				{
					--sphere->zDistance;
					sphere->blurTicks=0;
				}
			}
			// else
			// BLURRING W/O DOF OR AA... DOES NOTHING
			// handled by 50ms timer

		} //end if blurring

		// if at end of plane
		if(sphere->zDistance < -48)
		{
			sphere->zDistance = 0.0;
			sphere->rotation = 0.0;
		}
		else {
			sphere->rotation = (sphere->zDistance / (2 * M_PI) ) * 360;
		}

		// set material properties for color red
		glMaterialfv(GL_FRONT, GL_DIFFUSE, g_red );

		// disable the sphere click after 500 ms
		if (  (glutGet(GLUT_ELAPSED_TIME) - sphere->hit ) > 500)
		{
			sphere->hit = 0;

			// reset bluring ticks for this sphere
			sphere->blurTicks = 0;
		}
	}
	else
	{
		// normal sphere properties
		glMaterialfv(GL_FRONT, GL_DIFFUSE, g_colors[sphere->colorIdx]);

	}
	glTranslatef (x_translate, -1.0, sphere->zDistance);
	glRotatef (sphere->rotation, 1.0, 0.0, 0.0);
	glRotatef (90.0, 0.0, 1.0, 0.0);
	glutSolidSphere (1.0, 24, 24);

	glPopName();
	glPopMatrix();
}

void displayObjects()
{
	glTranslatef (0.0, 0.0, -5.0);
	glMatrixMode(GL_MODELVIEW);

	RenderSphere(&g_spheres[0], -2.0);
	RenderSphere(&g_spheres[1], 2.0);

	glPushMatrix ();
	glPushName(3);
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
	glPopName();
	glPopMatrix();
}



void display(void)
{
	unsigned char jitter;
	GLint viewport[4];
	jitter_point* jitAry;
	glGetIntegerv (GL_VIEWPORT, viewport);

	++g_state.framesElapsed;

	/*  rendering */

	if (g_userSettings.enableAA)
		// if AA is on (regardless of DOF)
	{
		switch(g_userSettings.enableAA)
		{
			case 2:
				jitAry = j2;
				break;
			case 4:
				jitAry = j4;
				break;
			case 8:
				jitAry = j8;
				break;
			case 15:
				jitAry = j15;
				break;
			case 24:
				jitAry = j24;
				break;
			case 66:
				jitAry = j66;
				break;

			default:
				printf("Undefined value of EnableAA! %i\n\n", g_userSettings.enableAA);
				break;
		}


		glClear(GL_ACCUM_BUFFER_BIT);
		for (jitter = 0; jitter < g_userSettings.enableAA; jitter++)

		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (g_userSettings.enableDOF)
				//both AA and DOF are enabled
				//   DoF jitter count and AA jitter count are the same
			{
				accPerspective (50.0,
						(GLdouble) viewport[2]/(GLdouble) viewport[3],
						1.0, 100.0, jitAry[jitter].x, jitAry[jitter].y, 0.33*jitAry[jitter].x, 0.33*jitAry[jitter].y, g_userSettings.enableDOF+5);

			}
			else
				// only AA is enabled
			{
				accPerspective (50.0,
						(GLdouble) viewport[2]/(GLdouble) viewport[3],
						1.0, 100.0, j8[jitter].x, j8[jitter].y, 0.0, 0.0, 1.0);

			}


			// render scene (but not HUD)
			displayObjects ();

			// set the ACCUM buffer
			glAccum(GL_ACCUM, 1.0/g_userSettings.enableAA);

		}

		glAccum (GL_RETURN, 1.0);

	}
	else if (g_userSettings.enableDOF)
		// only DOF is enabled
		//   assume a jitter of 8 for DoF only
	{
		glClear(GL_ACCUM_BUFFER_BIT);
		for (jitter = 0; jitter < 8; jitter++)

		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			accPerspective (50.0,
					(GLdouble) viewport[2]/(GLdouble) viewport[3],
					1.0, 100.0, 0.0, 0.0,
					0.33*j8[jitter].x, 0.33*j8[jitter].y, g_userSettings.enableDOF+5);
			displayObjects();
			glAccum(GL_ACCUM, 1.0/8);

		}
		glAccum (GL_RETURN, 1.0);

	}



	else
		// neither AA nor DOF is currently enabled
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		accPerspective (50.0,
				(GLdouble) viewport[2]/(GLdouble) viewport[3],
				1.0, 100.0, 0.0, 0.0, 0.0, 0.0, 1.0);
		displayObjects();
	}

	updateFps();
	glFlush();

	glutSwapBuffers();
}


void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}


void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 27:
			cleanup();
			exit(0);
			break;

		case '_':
		case '-':
			if (g_userSettings.enableDOF > 0)
				--g_userSettings.enableDOF;
			break;

		case '+':
		case '=':
			g_userSettings.enableDOF++;
			break;

		case '0':
			g_userSettings.enableAA = 0;
			break;

		case '1':
		case '2':
		case '3':
			g_userSettings.enableAA = pow(2, key - '0');
			break;

		case '4':
			g_userSettings.enableAA = 15;
			break;
		case '5':
			g_userSettings.enableAA = 24;
			break;
		case '6':
			g_userSettings.enableAA = 66;
			break;

		case 'r':
		case 'R':
			memset(&g_userSettings, 0, sizeof(g_userSettings));
			memset(&g_spheres, 0, sizeof(g_spheres));
			memset(&g_state, 0, sizeof(g_state));
			break;

		case 'd':
		case 'D':
			g_userSettings.debug = g_userSettings.debug ? 0 : 1;
			printf("%s debug output\n", g_userSettings.debug ? "Enabled" : "Disabled");
			break;

		case 'b':
		case 'B':
			g_userSettings.blur = g_userSettings.blur ? 0 : 1;
			printf("%s motion blur\n", g_userSettings.blur ? "Enabled" : "Disabled");
			break;

		default:
			break;
	}

	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
		return;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	GLuint selectBuf[512];
	glSelectBuffer(512, selectBuf);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glRenderMode(GL_SELECT);
	glLoadIdentity();
	gluPickMatrix((GLdouble) x, (GLdouble) (viewport[3] - y),
			5.0, 5.0, viewport);
	gluPerspective(50.0,
			(GLdouble) viewport[2]/(GLdouble) viewport[3],
			1.0, 100.0);
	glInitNames();

	displayObjects();

	GLint hits = glRenderMode(GL_RENDER);
	if (hits > 0)
	{
		for (int i = 0; i < (sizeof(g_spheres) / sizeof(g_spheres[0])); ++i)
		{
			if (g_spheres[i].glName == selectBuf[3] ||
				(hits > 1 && g_spheres[i].glName == selectBuf[7]))
			{
				if (g_userSettings.debug)
				  printf("clicked sphere %d\n", i);

				g_spheres[i].hit = glutGet(GLUT_ELAPSED_TIME);
			}
		}
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
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
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutTimerFunc(25, timer25ms, 0);
	glutTimerFunc(1000, currentInfo, 0);
	glutMainLoop();
	cleanup();
	return 0;
}
