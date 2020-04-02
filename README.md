# demo-gl-antialiasing

This is an OpenGL 2.X demo based on circa 2004 Redbook code. It implements motion blur, antialiasing, depth of field, and movable field of view angle.

This code requires:
- meson & ninja build system (suggestion: package 'meson')
- OpenGL 2.X (suggestion: packages 'mesa mesa-common-dev')
	- This code uses the fixed function pipeline of OpenGL and will likely not work with OpenGL 3.1+ 
- GLU (suggestion: package 'libglu1-mesa-dev')
- glut3 (suggestion: package 'freeglut3-dev')

Code taken directly from the Redbook has been Meson-ified as subprojects under ./subprojects/


### Quick start
	$ git clone https://github.com/ut3/demo-gl-antialiasing.git
	$ cd demo-gl-antialiasing
	$ sudo apt-get install meson mesa-common-dev mesa libglu1-mesa-dev freeglut3-dev
	$ meson build
	$ cd build
	$ ninja
	$ ./demo-gl-antialiasing


