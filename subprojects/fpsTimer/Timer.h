#ifndef Timer_h
#define Timer_h

/*
 Timer functions, for use in the subject 433 380, taught in Semester 1
 at the University of Melbourne.

 The timer functions handle two important tasks.

 Frame Rate Counting - the number of times you can redraw the screen
 in one second is called the frame rate, or frames per second, or FPS.
 ProcessTimer should be called every time you step through your
 update loop. If it returns a non-zero value, then one second of time
 has elapsed and the value stored in "framespersecond" will be the
 FPS for the previous second.

 Framerate independent motion - some computers run faster than others.
 If you change the size of the OpenGL window on the same computer, the
 FPS will probably change. We need to allow for this in our animation,
 so that our application looks the same no matter how fast the computer
 is. We do this by measuring how long it takes us to draw and process
 each frame, and by adjusting our animation by that amount. For instance,
 if it took 0.01 seconds to draw the previous frame, we will progress
 our animation by 1/100 of a second.
  - Note that the GetPreviousFrameDeltaInSeconds function is slightly
    dodgy on non-Windows machines, due to the low resolution of the
	"clock" function. If anyone can improve this, please go for it!
  - An alternative for non-Windows machines is to use the function
    glutTimer, which will invoke your callback function at set
	intervals. You will then be (roughly) guaranteed that your
	program will not run **faster** than the specified rate, however,
	it may run slower if your processing and rendering takes too long.

 Also note: If you are compiling on a windows machine without using
 Visual Studio, you will need to #define WINDOWS in Timer.c
 */

void InitialiseTimer();
unsigned char ProcessTimer(unsigned int *framespersecond);
float GetPreviousFrameDeltaInSeconds();

#endif
