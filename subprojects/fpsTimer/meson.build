# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright 2020 J Rick Ramstetter, rick@anteaterllc.com
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

project('fpsTimer', 'c')

fpsTimer = shared_library('fpsTimer', 'Timer.c')

inc = include_directories('.')
fpsTimer_dep = declare_dependency(link_with : fpsTimer, include_directories: inc)
