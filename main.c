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
static GLuint enableAA;
static GLuint enableDOF;
static GLfloat xRotSphere;
static GLfloat xRotSphere2;
static GLfloat sphereZdistance;
static GLfloat sphereZdistance2;
static char debug;
static GLuint blurring;

static GLuint sphereHits[2];
static GLuint blurTicks[2];

static GLuint framesElapsed;
static GLuint fpsTimeCurrent, fpsTimeBase;
static GLuint fps;


GLFONT font;



GLfloat sphereColor[] = { 0.7, 0.7, 0.0, 1.0 };
GLfloat sphereColor2[] = { 0.0, 0.7, 0.7, 1.0 };
GLfloat redColor[] = { 0.7, 0.0, 0.0, 1.0 };


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


	enableAA = 0;
	debug = 0;
	enableDOF = 0;
	sphereZdistance = 0;
	sphereZdistance2 = 0;
	xRotSphere = 0;
	xRotSphere2 = 0;

	sphereHits[0] = 0;
	sphereHits[1] = 0;

	blurTicks[0] = 0;
	blurTicks[1] = 0;

	fpsTimeCurrent = 0;
	fpsTimeBase = 0;
	framesElapsed=0;
	fps = 999;
	blurring = 0;


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
	if (debug)
	{
		printf("\n\ncurrent AA: %i", enableAA);
		printf("\ncurrent DOF: %i", enableDOF);
		printf("\ncurrent FPS: %i", fps);

	}
}


// gets called every 50ms
void timer50ms(int x)
{

	/* calculate sphere 1 z distance and rotation when it is not currently selected */
	if (!sphereHits[0])
	{
		sphereZdistance = sphereZdistance - 0.1;

		if(sphereZdistance < -48)
		{
			sphereZdistance = 0.0;
			xRotSphere = 0;
		}
		else
			xRotSphere = (sphereZdistance / (2 * PI_) ) * 360;

	}
	// if sphere1 is selected, but either 
	//  - blurring disabled
	//  - blurring enabled, but AA off
	else if ( (!blurring) || (blurring && !enableAA) )
	{
		sphereZdistance = sphereZdistance - 3;

		if(sphereZdistance < -48)
		{
			sphereZdistance = 0.0;
			xRotSphere = 0;
		}
		else
			xRotSphere = (sphereZdistance / (2 * PI_) ) * 360;
	}


	/* calculate sphere 2 z distance and rotation when it is not currently selected */
	if (!sphereHits[1])
	{
		sphereZdistance2 = sphereZdistance2 - 0.4;
		if(sphereZdistance2 < -50)
		{
			sphereZdistance2 = 0.0;
			xRotSphere2 = 0;
		}
		else
			xRotSphere2 = (sphereZdistance2 / (2 * PI_) ) * 360;
	}
	// if sphere2 is selected, but either 
	//  - blurring disabled
	//  - blurring enabled, but AA off
	else if ( (!blurring) || (blurring && !enableAA) )
	{
		sphereZdistance2 = sphereZdistance2 - 3;

		if(sphereZdistance2 < -48)
		{
			sphereZdistance2 = 0.0;
			xRotSphere2 = 0;
		}
		else
			xRotSphere2 = (sphereZdistance2 / (2 * PI_) ) * 360;
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

// Draw left sphere
void leftSphereRender()
{
	// if selected, handle the distance (e.g. motion blurring)
	if (sphereHits[0])
	{
		if (blurring)
		{
			if (enableAA)
			{
				blurTicks[0]++;


				if (enableAA == 2) 
					sphereZdistance = sphereZdistance - 2;

				else if (enableAA == 4)
					sphereZdistance--;

				else 
				{
					if (blurTicks[0] >= ceil(enableAA/4))
					{
						sphereZdistance--;
						blurTicks[0] = 0;
					}	
				}

			}
			else if (enableDOF)
				//assumed jitter is 8
			{
				blurTicks[0]++;
				if (blurTicks[0] >= 3)
				{
					sphereZdistance--;
					blurTicks[0]=0;
				}
			}
			// else
			// BLURRING W/O DOF OR AA... DOES NOTHING
			// handled by 50ms timer

		} //end if blurring

		// if at end of plane
		if(sphereZdistance < -48)
		{
			sphereZdistance = 0.0;
			xRotSphere = 0;
		}
		else
			xRotSphere = (sphereZdistance / (2 * PI_) ) * 360;

		// set material properties for color red
		glMaterialfv(GL_FRONT, GL_DIFFUSE, redColor );

		// disable the sphere click after 500 ms
		if (  (glutGet(GLUT_ELAPSED_TIME) - sphereHits[0] ) > 500)
		{
			sphereHits[0] = 0;

			// reset bluring ticks for this sphere
			blurTicks[0] = 0;	 
		}
	}
	else
	{
		// normal sphere properties
		glMaterialfv(GL_FRONT, GL_DIFFUSE, sphereColor );

	}
	glTranslatef (-2.0, -1.0, sphereZdistance);
	glRotatef (xRotSphere, 1.0, 0.0, 0.0);
	glRotatef (90.0, 0.0, 1.0, 0.0);
	glutSolidSphere (1.0, 24, 24);
}


// draw right sphere
void rightSphereRender()
{
	// if selected, handle the distance (e.g. motion blurring)
	if (sphereHits[1])
	{

		if (blurring)
		{
			if (enableAA)

				// BLURRING AND AA ENABLED
			{
				blurTicks[1]++;


				if (enableAA == 2) 
					sphereZdistance2 = sphereZdistance2 - 2;

				else if (enableAA == 4)
					sphereZdistance2--;

				else 
				{
					if (blurTicks[1] >= ceil(enableAA/4))
					{
						sphereZdistance2--;
						blurTicks[1] = 0;
					}	
				}

			}
			else if (enableDOF)

				// BLURRING AND DOF ENABLED

			{
				blurTicks[1]++;
				if (blurTicks[1] >= 3)
				{
					sphereZdistance2--;
					blurTicks[1]=0;
				}
			}
			// else
			// BLURRING W/O DOF OR AA... DOES NOTHING
			// handled by 50ms timer
		} // end if blurring


		// if at end of plane
		if(sphereZdistance2 < -48)
		{
			sphereZdistance2 = 0.0;
			xRotSphere2 = 0;
		}
		else
			xRotSphere2 = (sphereZdistance2 / (2 * PI_) ) * 360;

		// set material properties for color red
		glMaterialfv(GL_FRONT, GL_DIFFUSE, redColor );

		// disable the sphere click after 500 ms
		if (  (glutGet(GLUT_ELAPSED_TIME) - sphereHits[1] ) > 500)
		{
			sphereHits[1] = 0;

			// reset bluring ticks for this sphere
			blurTicks[1] = 0;	 
		}
	}
	else
	{
		// normal sphere properties
		glMaterialfv(GL_FRONT, GL_DIFFUSE, sphereColor2 );

	}
	glTranslatef (2.0, -1.0, sphereZdistance2);
	glRotatef (xRotSphere2, 1.0, 0.0, 0.0);
	glRotatef (90.0, 0.0, 1.0, 0.0);
	glutSolidSphere (1.0, 24, 24);
}


void displayObjects()

{
	glMatrixMode(GL_MODELVIEW);


	// BEGIN LEFT SPHERE
	glPushMatrix();
	glPushName(1); 
	leftSphereRender();
	glPopName();
	glPopMatrix();
	// END LEFT SPHERE


	// BEGIN RIGHT SPHERE
	glPushMatrix();
	glPushName(2);
	rightSphereRender();
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


char *itoa(int num, char *str, int unused)
{
	if(str == NULL)
	{
		return NULL;
	}
	sprintf(str, "%d", num);
	return str;
}



// display 2d HUD (heads up display) text
void displayText()
{

	GLint viewport[4];

	char tempc[15];

	char line1[25];
	char line2[25];
	char line3[25];
	char line4[30];

	// line1 of hud
	itoa(fps, tempc, 10);
	strcpy(line1, "FPS: ");
	strcat(line1, tempc );

	// line2 of hud
	itoa(enableAA, tempc, 10);
	strcpy(line2, "AA jitter: ");
	strcat(line2, tempc);

	// line3 of hud
	itoa(enableDOF, tempc, 10);
	strcpy(line3, "Depth of Field: ");
	strcat(line3, tempc);


	// line4 of hud
	itoa(blurring, tempc, 10);
	strcpy(line4, "Blurring: ");
	strcat(line4, tempc);
	if (blurring && !enableAA && !enableDOF)
		strcat(line4, "(but AA/DoF is off)");


	// necessary for using glFont package
	//	glfont is being used for 2d text in the hud
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// get viewport values
	glGetIntegerv (GL_VIEWPORT, viewport);

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


void display(void)
{
	unsigned char jitter;
	GLint viewport[4];
	jitter_point* jitAry;
	glGetIntegerv (GL_VIEWPORT, viewport);

	//for fps counter
	framesElapsed++;

	/*  rendering */

	if (enableAA)
		// if AA is on (regardless of DOF)
	{
		currentInfo();
		switch(enableAA)
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
				printf("Undefined value of EnableAA! %i\n\n", enableAA);
				break;
		}


		glClear(GL_ACCUM_BUFFER_BIT);
		for (jitter = 0; jitter < enableAA; jitter++)

		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (enableDOF)
				//both AA and DOF are enabled
				//   DoF jitter count and AA jitter count are the same
			{
				currentInfo();

				accPerspective (50.0,
						(GLdouble) viewport[2]/(GLdouble) viewport[3],
						1.0, 100.0, jitAry[jitter].x, jitAry[jitter].y, 0.33*jitAry[jitter].x, 0.33*jitAry[jitter].y, enableDOF+5);

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
			glAccum(GL_ACCUM, 1.0/enableAA);

		}

		glAccum (GL_RETURN, 1.0);

	}
	else if (enableDOF)
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
					0.33*j8[jitter].x, 0.33*j8[jitter].y, enableDOF+5);
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


	/* fps  calcs */
	fpsTimeCurrent=glutGet(GLUT_ELAPSED_TIME);
	if (fpsTimeCurrent - fpsTimeBase > 1000) 
	{
		fps = ceil(framesElapsed*1000.0/(fpsTimeCurrent-fpsTimeBase));
		fpsTimeBase = fpsTimeCurrent;		
		framesElapsed = 0;
	}

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
			if (enableDOF > 50)
				enableDOF=enableDOF-25;
			else if (enableDOF > 0)
				enableDOF--;
			break;

		case '+':
		case '=':
			if (enableDOF < 50)
				enableDOF++;
			// for DOF > 50, speed up the subtracts
			else if (enableDOF < 1000)
				enableDOF=enableDOF+25;
			break;

		case '0':
			enableAA=0;
			break;

		case '1':
			enableAA=2;
			break;

		case '2':
			enableAA=4;
			break;

		case '3':
			enableAA=8;
			break;

		case '4':
			enableAA=15;
			break;

		case '5':
			enableAA=24;
			break;
		case '6':
			enableAA=66;
			break;

		case 'r':
		case 'R':
			enableAA=0;
			enableDOF=0;
			sphereZdistance = 0;
			sphereZdistance2 =0;
			sphereHits[0] = 0;
			sphereHits[1] = 0;
			blurTicks[0] = 0;
			blurTicks[1] = 0;
			blurring = 0;
			break;
		case 'x':
			debug=0;
			break;
		case 'X':
			debug=1;
			break;

		case 'b':
		case 'B':
			blurring=1;
			break;

		case 'v':
		case 'V':
			blurring = 0;
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
		debug && printf("clicked sphere left\n");

		sphereHits[0] = glutGet(GLUT_ELAPSED_TIME);
	}

	// If the user has clicked the right sphere
	if ( (buffer[3]==2) || (hits > 1 && (buffer[7]==2) ) )
	{
		debug && printf("clicked sphere right\n");

		sphereHits[1] = glutGet(GLUT_ELAPSED_TIME);
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

/*  Main Loop
 *  Be certain you request an accumulation buffer.
 */
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
