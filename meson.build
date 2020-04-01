# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright 2020 J Rick Ramstetter, rick.ramstetter@gmail.com
#
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2
# as published by the Free Software Foundation.
#
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

project ('demo-gl-antialiasing', 'c', version : '1', license: 'GPLv2')
sources = ['main.c']
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

redbook_accpersp = subproject('redbook_accpersp')
redbook_accpersp_dep = redbook_accpersp.get_variable('redbook_accpersp_dep')

redbook_checker = subproject('redbook_checker')
redbook_checker_dep = redbook_checker.get_variable('redbook_checker_dep')

executable ('demo-gl-antialiasing', sources, 
	dependencies: 
		[gl_dep, glut_dep, glu_dep, math_dep, redbook_accpersp_dep, redbook_checker_dep]
)
