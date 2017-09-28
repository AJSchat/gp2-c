/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors
Copyright (C) 2017 - Ane-Jouke Schat

This file is part of the gp2-c source code.

gp2-c is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/
// main.cpp - Main C++ file.

extern "C" {
    #include "qcommon/q_shared.h"
    #include "qcommon/qcommon.h"
    #include "qcommon/q_platform.h"
}
#include "gp2-cpp/genericparser2.h"

int main(int argc, char** argv) {
    Com_Init(argv[0]);

    return 0;
}