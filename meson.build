project ('demo-gl-antialiasing', 'c', version : '1', license: 'GPLv2')
sources = ['main.c', 'timer.c', 'glfont.c']
compiler = meson.get_compiler('c')

gl_dep = dependency('gl')

glut_dep = dependency('glut', required: false)
if not glut_dep.found()
  glut_dep = compiler.find_library('glut')
endif

glu_dep = dependency('glu')

math_dep = dependency('m', required: false)
if not math_dep.found()
  math_dep = compiler.find_library('m')
endif 

executable ('demo-gl-antialiasing', sources, dependencies: [gl_dep, glut_dep, glu_dep, math_dep])
