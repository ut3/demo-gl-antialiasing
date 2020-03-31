/*
    J Rick Ramstetter
    Guy Kisel

    CS 112 Spring 2008 Final Project

    Portions of this project taken from the OpenGL tutorial files
        accpersp.c and jitter.h
 */
 
 

 The goal of our project was to use the Accumulation buffer for anti-aliasing. 
 This goal was achieved, in addition to many features being added.
 
 The accumulation buffer antialiases by rendering multiple copies of a frame, each
 perturbed a bit from the normal frame. The number of perturbed renders used for each
 frame is called the "jitter count"
 
 *****************************************************************
 INPUT KEYS:
 0: set the anti aliasing jitter count to 0 (disable AA)
 1: jitter count to 2
 2: jitter count to 4
 3: jitter count to 8
 4: jitter count to 15
 5: jitter count to 24
 6: jitter count to 66
 R/r: reset all values (DoF, AA, sphere Z distances, etc).
 X: enable console output
 x: disable console output
 B/b: enable motion blurring (ONLY WORKS IF AAJITTER!=0 or DoF!=0)
 V/b: disable motion blurring
 esc: exit
 
 + or =: 
	increase focal point (depth of field)
	set to 0 to disable DoF
	if the current value of DoF is greater than 50, 
		this adds 25 (instead of 1) per press
		
 - or _:
	decrease focal point  (depth of field)
 	set to 0 to disable DoF
	if the current value of DoF is greater than 50, 
		this subtracts 25 (instead of 1) per press
	
Mouse Press on Ball:
	* Speed the ball up significantly
	* If (blurring && (aa jitter || dof) )
		then ball's speedup is motion blurred
	
	
*****************************************************************
Things we have implemented (for extra credit):
  
* Anti Aliasing
	Push keys 0 to 6 to try it

* Depth of field 
	push + or - to try it
	NOTE: DoF values increase/decrease rapidly when current DoF value is above 50

* glRenderMode(GL_SELECT)
	We are using the GL_SELECT render mode to tell when the user has clicked on a ball
	Clicking on one of the two balls will:
		- change that ball's color to red for 500ms
		- speed up that ball's velocity for 500ms
	
* Motion Blurring
	To enable motion blurring: 
	- Press b/B to enable blurring flag
	- Enable either AA jitter or DoF (or both)
	- Click on one of the balls so that it turns red and speeds up. The ball's 
		speed up, in combination with motion blurring, causes it to "warp ahead"
		
	To disable motion blurring:
	- Press v/V to disable blurring flag
	- Verify the balls no longer "warp" by clicking then. The balls will speed up,
		but not blur.  
	
* 2D HuD text display
	In the upper left of the window, 4 lines of 2d text are rendered. 
	This is a rather complicated process, requiring the use of a seperate and temporary
	projection matrix (see displayText()) and an external 3rd party library (glFont).

* Frames Per Second display:
	Using our 2D HuD text display and various bits of math, we have implemented a frame 
	counter using glutGet(ELAPSED_TIME). This allows you to see in real time the frame
	rate difference for various settings of AA jitter and DoF. 

	
To the maximum extent realistically possible, we have tried to ensure these various 
features work together in a manner which is coherent and sane. 

POINTS TO REMEMBER:
 - Motion blurring only works when BOTH the blurring flag AND
	 either AA jitter or DoF are enabled
 - Depth of field values increase/decrease rapidly beyond the value of 50
	