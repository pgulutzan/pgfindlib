/*
  pgfindlib.h - the file that's #included in pgfindlib.c and anything that calls it.

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

#ifndef PGFINDLIB_H
#define PGFINDLIB_H
extern int pgfindlib(const char *sonames, char *buffer, unsigned int buffer_max_length, const char *extra_paths);

#define PGFINDLIB_OK 0
#define PGFINDLIB_ERROR_BUFFER_MAX_LENGTH_TOO_SMALL -1
#define PGFINDLIB_ERROR_NULL -2
#define PGFINDLIB_ERROR_MAX_PATH_LENGTH_TOO_SMALL -3

#ifndef PGFINDLIB_WARNING_LEVEL
#define PGFINDLIB_WARNING_LEVEL 4
#endif

#ifndef PGFINDLIB_INCLUDE_LD_AUDIT
#define PGFINDLIB_INCLUDE_LD_AUDIT 1
#endif

#ifndef PGFINDLIB_INCLUDE_LD_PRELOAD
#define PGFINDLIB_INCLUDE_LD_PRELOAD 1
#endif

#ifndef PGFINDLIB_INCLUDE_DT_RPATH_OR_DT_RUNPATH
#define PGFINDLIB_INCLUDE_DT_RPATH_OR_DT_RUNPATH 1
#endif

#ifndef PGFINDLIB_INCLUDE_LD_LIBRARY_PATH
#define PGFINDLIB_INCLUDE_LD_LIBRARY_PATH 1
#endif

#ifndef PGFINDLIB_INCLUDE_LD_RUN_PATH
#define PGFINDLIB_INCLUDE_LD_RUN_PATH 1
#endif

#ifndef PGFINDLIB_INCLUDE_DEFAULT_PATHS
#define PGFINDLIB_INCLUDE_DEFAULT_PATHS 1
#endif

#ifndef PGFINDLIB_INCLUDE_LD_SO_CACHE
#define PGFINDLIB_INCLUDE_LD_SO_CACHE 1
#endif

#ifndef PGFINDLIB_INCLUDE_EXTRA_PATHS
#define PGFINDLIB_INCLUDE_EXTRA_PATHS 1
#endif

#ifndef PGFINDLIB_INCLUDE_GET_LIB_OR_PLATFORM
#define PGFINDLIB_INCLUDE_GET_LIB_OR_PLATFORM 1
#endif

#ifndef PGFINDLIB_INCLUDE_SYMLINKS
#define PGFINDLIB_INCLUDE_SYMLINKS 1
#endif

#ifndef PGFINDLIB_INCLUDE_HARDLINKS
#define PGFINDLIB_INCLUDE_HARDLINKS 1
#endif

#ifndef PGFINDLIB_MAX_INODE_COUNT
#define PGFINDLIB_MAX_INODE_COUNT 100
#endif

#ifndef PGFINDLIB_MAX_PATH_LENGTH
#define PGFINDLIB_MAX_PATH_LENGTH 4096
#endif

#ifndef PGFINDLIB_FREEBSD
#if defined(__FreeBSD__)
#define PGFINDLIB_FREEBSD
#endif
#endif

#define PGFINDLIB_VERSION_MAJOR 0
#define PGFINDLIB_VERSION_MINOR 9
#define PGFINDLIB_VERSION_PATCH 5

#endif
