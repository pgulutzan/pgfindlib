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

#ifndef FINDMYSO_INCLUDE_LD_AUDIT
#define FINDMYSO_INCLUDE_LD_AUDIT 1
#endif

#ifndef FINDMYSO_INCLUDE_LD_PRELOAD
#define FINDMYSO_INCLUDE_LD_PRELOAD 1
#endif

#ifndef FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH
#define FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH 1
#endif

#ifndef FINDMYSO_INCLUDE_LD_LIBRARY_PATH
#define FINDMYSO_INCLUDE_LD_LIBRARY_PATH 1
#endif

#ifndef FINDMYSO_INCLUDE_LD_RUN_PATH
#define FINDMYSO_INCLUDE_LD_RUN_PATH 1
#endif

#ifndef FINDMYSO_INCLUDE_LD_SO_CACHE
#define FINDMYSO_INCLUDE_LD_SO_CACHE 1
#endif

#ifndef FINDMYSO_INCLUDE_EXTRA_PATHS
#define FINDMYSO_INCLUDE_EXTRA_PATHS 1
#endif

#ifndef FINDMYSO_INCLUDE_GET_LIB_OR_PLATFORM
#define FINDMYSO_INCLUDE_GET_LIB_OR_PLATFORM 1
#endif

#ifndef FINDMYSO_MAX_PATH_LENGTH
#define FINDMYSO_MAX_PATH_LENGTH 4096
#endif

#ifndef FINDMYSO_FREEBSD
#if defined(__FreeBSD__)
#define FINDMYSO_FREEBSD
#endif
#endif

#define FINDMYSO_VERSION_MAJOR 0
#define FINDMYSO_VERSION_MINOR 9
#define FINDMYSO_VERSION_PATCH 2

#endif
