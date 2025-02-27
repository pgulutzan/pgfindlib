/*
  findmyso.h - the file that's #included in findmyso.c and anything that calls it.

  Copyright (c) 2025 by Peter Gulutzan. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

  For explanation see the README.md file which comes with the readmyso package
*/

#ifndef FINDMYSO_H
#define FINDMYSO_H
extern int findmyso(const char *sonames, char *buffer, unsigned int buffer_max_length, const char *extra_paths);

#define FINDMYSO_OK 0
#define FINDMYSO_ERROR_BUFFER_MAX_LENGTH_TOO_SMALL -1
#define FINDMYSO_ERROR_NULL -2
#define FINDMYSO_ERROR_MAX_PATH_LENGTH_TOO_SMALL -3

#ifndef FINDMYSO_INCLUDE_COMMENTS
#define FINDMYSO_INCLUDE_COMMENTS 1
#endif

#ifndef FINDMYSO_MAX_PATH_LENGTH
#define FINDMYSO_MAX_PATH_LENGTH 4096
#endif

#ifndef OCELOT_OS_LINUX
#if defined(__linux__) || defined(__linux) || defined(linux)
#define OCELOT_OS_LINUX
#else
#define OCELOT_OS_NONLINUX
#endif
#if defined(__FreeBSD__)
#define OCELOT_OS_FREEBSD
#endif
#endif

#define FINDMYSO_VERSION_MAJOR 0
#define FINDMYSO_VERSION_MINOR 9
#define FINDMYSO_VERSION_PATCH 0

#endif
