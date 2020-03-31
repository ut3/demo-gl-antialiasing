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

project('redbook_checker', 'c')

gl_dep = dependency('gl')
glu_dep = dependency('glu')

compiler = meson.get_compiler('c')
glut_dep = dependency('glut', required: false)
if not glut_dep.found()
  glut_dep = compiler.find_library('glut')
endif

redbook_checker = shared_library('redbook_checker', 'checker.c', 
		dependencies: [gl_dep, glu_dep, glut_dep],
		c_args: [ '-fPIC' ],
		link_args: ['-pie', '-Wl,-E'])

inc = include_directories('.')
redbook_checker_dep = declare_dependency(link_with : redbook_checker, 
                                         include_directories: inc)
