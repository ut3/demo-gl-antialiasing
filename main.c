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

#include "glfont.h"


#define PI_ 3.14159265358979323846
#define    checkImageWidth 512
#define    checkImageHeight 1024
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];
static GLuint texName;

GLFONT font;



static const GLfloat yellow[4] = { 0.7, 0.7, 0.0, 1.0 };
static const GLfloat greenish[4] = { 0.0, 0.7, 0.7, 1.0 };
static const GLfloat red[4] = { 0.7, 0.0, 0.0, 1.0 };


struct UserSettings {
  GLuint enableAA;
  GLuint enableDOF;
  GLuint blur;
  GLuint debug;
};

struct Sphere {
  GLuint hit;
  GLuint blurTicks;
  GLfloat zDistance;
  GLfloat rotation;
  GLfloat color[4];
};

struct State {
  GLuint fps;
  GLuint baseTime;
  GLuint framesElapsed;
};

struct UserSettings g_userSettings;
struct Sphere g_spheres[2];
struct State g_state;


void cleanup( int sig )
{
	// insert cleanup code here (i.e. deleting structures or so)

	glFontDestroy(&font);
	exit( 0 );
}




/*  Initialize lighting and other values.
*/


void init(void)
{
	GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_position[] = { 0.0, 0.0, 10.0, 1.0 };
	GLfloat lm_ambient[] = { 0.2, 0.2, 0.2, 1.0 };

	memset(&g_userSettings, 0, sizeof(g_userSettings));
	memset(&g_spheres, 0, sizeof(g_spheres));
	memcpy(g_spheres[0].color, yellow, sizeof(g_spheres[0].color));
	memcpy(g_spheres[1].color, greenish, sizeof(g_spheres[1].color));

	memset(&g_state, 0, sizeof(g_state));

	//Create font
	glFontCreate(&font, "jramstet-glfont.glf", 0) || printf("\n\nERROR CREATING GLFONT\n");

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, 50.0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel (GL_FLAT);

	// create checkerboard texture
	makeCheckImage(&checkImage[0][0][0], checkImageWidth, checkImageHeight);

	// set up the texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, checkImageHeight,
			0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, checkImageWidth, checkImageHeight,
			0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);



	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearAccum(0.0, 0.0, 0.0, 0.0);
}

// print debug info if debug is enabled
void currentInfo()
{
	if (g_userSettings.debug)
	{
		printf("\n\ncurrent AA: %i", g_userSettings.enableAA);
		printf("\ncurrent DOF: %i", g_userSettings.enableDOF);
		printf("\ncurrent FPS: %i", g_state.fps);

	}
}


// gets called every 50ms
void timer50ms(int x)
{

	/* calculate sphere 1 z distance and rotation when it is not currently selected */
	if (!g_spheres[0].hit)
	{
		g_spheres[0].zDistance -= 0.1;

		if(g_spheres[0].zDistance < -48)
		{
			g_spheres[0].zDistance = 0.0;
			g_spheres[0].rotation = 0;
		}
		else {
			g_spheres[0].rotation = (g_spheres[0].zDistance / (2 * PI_) ) * 360;
		}

	}
	// if sphere1 is selected, but either 
	//  - blurring disabled
	//  - blurring enabled, but AA off
	else if ( (!g_userSettings.blur) || (g_userSettings.blur && !g_userSettings.enableAA) )
	{
		g_spheres[0].zDistance -= 3;

		if(g_spheres[0].zDistance < -48)
		{
			g_spheres[0].zDistance = 0.0;
			g_spheres[0].rotation = 0;
		}
		else
		{
			g_spheres[0].rotation = (g_spheres[0].zDistance / (2 * PI_) ) * 360;
		}
	}


	/* calculate sphere 2 z distance and rotation when it is not currently selected */
	if (!g_spheres[1].hit)
	{
		g_spheres[1].zDistance = g_spheres[1].zDistance - 0.4;
		if(g_spheres[1].zDistance < -50)
		{
			g_spheres[1].zDistance = 0.0;
			g_spheres[1].rotation = 0;
		}
		else
			g_spheres[1].rotation = (g_spheres[1].zDistance / (2 * PI_) ) * 360;
	}
	// if sphere2 is selected, but either 
	//  - blurring disabled
	//  - blurring enabled, but AA off
	else if ( (!g_userSettings.blur) || (g_userSettings.blur && !g_userSettings.enableAA) )
	{
		g_spheres[1].zDistance = g_spheres[1].zDistance - 3;

		if(g_spheres[1].zDistance < -48)
		{
			g_spheres[1].zDistance = 0.0;
			g_spheres[1].rotation = 0;
		}
		else
			g_spheres[1].rotation = (g_spheres[1].zDistance / (2 * PI_) ) * 360;
	}


	/*
NOTE: 
Z distances for selected balls are handled in display() 
such distances are dependant on the frame rate rather than
the elapsed time
*/

	/* redisplay what's changed */
	glutPostRedisplay();

	glutTimerFunc(50, timer50ms, 0);
}

void RenderSphere(struct Sphere* sphere, GLfloat x_translate)
{
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
			sphere->rotation = (sphere->zDistance / (2 * PI_) ) * 360;
		}

		// set material properties for color red
		glMaterialfv(GL_FRONT, GL_DIFFUSE, red );

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
		glMaterialfv(GL_FRONT, GL_DIFFUSE, sphere->color );

	}
	glTranslatef (x_translate, -1.0, sphere->zDistance);
	glRotatef (sphere->rotation, 1.0, 0.0, 0.0);
	glRotatef (90.0, 0.0, 1.0, 0.0);
	glutSolidSphere (1.0, 24, 24);
}

void displayObjects()

{
	glMatrixMode(GL_MODELVIEW);


	// BEGIN LEFT SPHERE
	glPushMatrix();
	glPushName(1); 
	RenderSphere(&g_spheres[0], -2.0);
	glPopName();
	glPopMatrix();
	// END LEFT SPHERE


	// BEGIN RIGHT SPHERE
	glPushMatrix();
	glPushName(2);
	RenderSphere(&g_spheres[1], 2.0);
	glPopName();
	glPopMatrix();
	// END RIGHT SPHERE



	// BEGIN CHECKERBOARD FLOOR
	glPushMatrix ();
	glPushName(3);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, texName);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-5.0, -2.0, -50.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-5.0, -2.0, 1.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(5.0, -2.0, 1.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(5.0, -2.0, -50.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopName();
	glPopMatrix();
	// END CHECKERBOARD FLOOR


}



// display 2d HUD (heads up display) text
void displayText()
{
	char line1[32];
	char line2[32];
	char line3[32];
	char line4[32];

	snprintf(sizeof(line1), &line1, "FPS: %lu", g_state.fps);
	snprintf(sizeof(line2), &line2, "AA jitter: %lu", g_userSettings.enableAA);
	snprintf(sizeof(line3), &line3, "Depth of field: %lu", g_userSettings.enableDOF);
	snprintf(sizeof(line4), &line4, "Blur: %lu%s", g_userSettings.blur,
			(g_userSettings.blur && !g_userSettings.enableAA &&
					!g_userSettings.enableDOF) ? " (AA&DoF off)" : ""
			);

	// necessary for using glFont package
	//	glfont is being used for 2d text in the hud
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// Set a temporary  & simple 2d projection matrix
	//   normal projection will be replaced at the end of all this 
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 640, 0, 480, -1, 1); // simple projection


	// back to model mode to render the text
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glFontBegin(&font);
	glScalef(8.0, 8.0, 8.0);
	glTranslatef(15,15, 0);
	glFontTextOut(line1, 1, 60 , 0);
	glFontTextOut(line2, 1, 57 , 0);
	glFontTextOut(line3, 1, 54 , 0);
	glFontTextOut(line4, 1, 51 , 0);
	glFontEnd();
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

}




// render the scene
//   but not the HUD
// used by display() and called for each GL_ACCUM jitter
void displayHelper()
{
	//glPushMatrix ();
	glTranslatef (0.0, 0.0, -5.0);
	displayObjects();
	//displayText(); // should be in display() near the end
	glFlush();
	//glPopMatrix ();
}


void updateFps()
{
  GLuint fpsTimeCurrent = glutGet(GLUT_ELAPSED_TIME);
  if (fpsTimeCurrent - g_state.baseTime > 1000)
  {
	g_state.fps = ceil( g_state.framesElapsed*1000.0 /
					 (fpsTimeCurrent-g_state.baseTime) );
	g_state.baseTime = fpsTimeCurrent;
	g_state.framesElapsed = 0;
  }
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
		currentInfo();
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
				currentInfo();

				accPerspective (50.0,
						(GLdouble) viewport[2]/(GLdouble) viewport[3],
						1.0, 100.0, jitAry[jitter].x, jitAry[jitter].y, 0.33*jitAry[jitter].x, 0.33*jitAry[jitter].y, g_userSettings.enableDOF+5);

			}
			else
				// only AA is enabled
			{
				currentInfo();

				accPerspective (50.0,
						(GLdouble) viewport[2]/(GLdouble) viewport[3],
						1.0, 100.0, j8[jitter].x, j8[jitter].y, 0.0, 0.0, 1.0);

			}


			// render scene (but not HUD)
			displayHelper (GL_RENDER);

			// set the ACCUM buffer
			glAccum(GL_ACCUM, 1.0/g_userSettings.enableAA);

		}

		glAccum (GL_RETURN, 1.0);

	}
	else if (g_userSettings.enableDOF)
		// only DOF is enabled
		//   assume a jitter of 8 for DoF only
	{
		currentInfo();

		glClear(GL_ACCUM_BUFFER_BIT);
		for (jitter = 0; jitter < 8; jitter++)

		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			accPerspective (50.0,
					(GLdouble) viewport[2]/(GLdouble) viewport[3],
					1.0, 100.0, 0.0, 0.0,
					0.33*j8[jitter].x, 0.33*j8[jitter].y, g_userSettings.enableDOF+5);
			displayHelper (GL_RENDER);
			glAccum(GL_ACCUM, 1.0/8);

		}
		glAccum (GL_RETURN, 1.0);

	}



	else
		// neither AA nor DOF is currently enabled
	{
		currentInfo();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		accPerspective (50.0,
				(GLdouble) viewport[2]/(GLdouble) viewport[3],
				1.0, 100.0, 0.0, 0.0, 0.0, 0.0, 1.0);
		displayHelper (GL_RENDER);

	}


	// display the HUD
	displayText();

	// force openGL flush
	glFlush();

	updateFps();

	glutSwapBuffers();
} // end HUGE display() function



// HANDLE WINDOW RESIZINGS
void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}





// HANDLE KEYBOARD PRESSES
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 27:
			exit(0);
			break;

		case '_':
		case '-':	
			// for DOF > 50, speed up the adds
			if (g_userSettings.enableDOF > 50)
				g_userSettings.enableDOF=g_userSettings.enableDOF-25;
			else if (g_userSettings.enableDOF > 0)
				g_userSettings.enableDOF--;
			break;

		case '+':
		case '=':
			if (g_userSettings.enableDOF < 50)
				g_userSettings.enableDOF++;
			// for DOF > 50, speed up the subtracts
			else if (g_userSettings.enableDOF < 1000)
				g_userSettings.enableDOF=g_userSettings.enableDOF+25;
			break;

		case '0':
			g_userSettings.enableAA=0;
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
			g_userSettings.enableAA=24;
			break;
		case '6':
			g_userSettings.enableAA=66;
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

void processHits(GLint hits, GLuint buffer[], GLuint x, GLuint y)
{

	unsigned int i, j;
	GLuint names, *ptr;

	if (hits < 1)
		return;

	// If the user has clicked the left sphere
	if ( (buffer[3]==1) || (hits > 1 &&(buffer[7]==1) ) )
	{
		if (g_userSettings.debug)
		  printf("clicked sphere left\n");

		g_spheres[0].hit = glutGet(GLUT_ELAPSED_TIME);
	}

	// If the user has clicked the right sphere
	if ( (buffer[3]==2) || (hits > 1 && (buffer[7]==2) ) )
	{
		if (g_userSettings.debug)
		  printf("clicked sphere right\n");

		g_spheres[1].hit = glutGet(GLUT_ELAPSED_TIME);
	}



}

void mouse(int button, int state, int x, int y)
{
	GLuint selectBuf[512];
	GLint hits;
	GLint viewport[4];

	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
		return;

	glGetIntegerv(GL_VIEWPORT, viewport);

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

	// determine what was clicked on
	hits = glRenderMode(GL_RENDER);
	processHits(hits, selectBuf, x, y);

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
	init();
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutTimerFunc(50, timer50ms, 0);
	glutMainLoop();
	return 0;
}
