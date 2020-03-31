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

project('redbook_accpersp', 'c')

gl_dep = dependency('gl')

compiler = meson.get_compiler('c')
glut_dep = dependency('glut', required: false)
if not glut_dep.found()
  glut_dep = compiler.find_library('glut')
endif

math_dep = dependency('m', required: false)
if not math_dep.found()
  math_dep = compiler.find_library('m')
endif

lib = shared_library('redbook_accpersp', 'accpersp.c', 
          dependencies: [gl_dep, glut_dep, math_dep],
          c_args: [ '-fPIC' ],
          link_args: ['-pie', '-Wl,-E'])
		              
inc = include_directories('.')
redbook_accpersp_dep = declare_dependency(link_with : lib, 
                                          include_directories: inc)
