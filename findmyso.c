/*
  findmyso.c --   To get a list of paths and .so files along
  ld_audit + ld_preload + -rpath (dt_rpath) + ld_library_path + -rpath (dt_run_path) + ld_run_path + /etc/ld.so.cache

   Version: 0.9.0
   Last modified: February 27 2025

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

  For explanation see the README.md file which comes with the findmyso package.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <link.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

#include "findmyso.h" /* definition of findmyso(), and some #define FINDMYSO_... */

static int findmyso_libraries_or_files(const char *sonames, const char *librarylist, char *buffer,
                                       unsigned int *buffer_length, const char *type, unsigned int buffer_max_length);
static int findmyso_strcat(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length);
static int findmyso_find_line_in_sonames(const char *sonames, const char *line);

int findmyso(const char *sonames, char *buffer, unsigned int buffer_max_length, const char *extra_paths)
{
  if ((buffer == NULL) || (sonames == NULL)) return FINDMYSO_ERROR_NULL;
  
  /* Check that no soname length is greater than or equal to FINDMYSO_MAX_PATH_LENGTH */
  const char *prev_p_sonames= sonames;
  for (const char *p_sonames= sonames; *p_sonames != '\0'; ++p_sonames)
  {
    if (*p_sonames == ':')
    {
      prev_p_sonames= p_sonames;
      continue;
    }
    if ((p_sonames - prev_p_sonames) >= FINDMYSO_MAX_PATH_LENGTH) return FINDMYSO_ERROR_MAX_PATH_LENGTH_TOO_SMALL;
  }

  unsigned int buffer_length= 0;
  int rval;

#if (FINDMYSO_INCLUDE_COMMENTS == 1)
  {
    char version[128];
    sprintf(version, "/* findmyso version %d.%d.%d */\n", FINDMYSO_VERSION_MAJOR, FINDMYSO_VERSION_MINOR, FINDMYSO_VERSION_PATCH);
   rval= findmyso_strcat(buffer, &buffer_length, version, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }
#endif

  char *ld_audit= getenv("LD_AUDIT");
  if (ld_audit != NULL)
  {
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
   rval= findmyso_strcat(buffer, &buffer_length, "/* LD_AUDIT */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    if (ld_audit != NULL)
    {
      rval= findmyso_libraries_or_files(sonames, ld_audit, buffer, &buffer_length, "LD_AUDIT", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
    }
  }

  char *ld_preload= getenv("LD_PRELOAD");
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
  rval= findmyso_strcat(buffer, &buffer_length, "/* LD_PRELOAD */\n", buffer_max_length);
  if (rval != FINDMYSO_OK) return rval;
#endif
  if (ld_preload != NULL)
  {
    rval= findmyso_libraries_or_files(sonames, ld_preload, buffer, &buffer_length, "LD_PRELOAD", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }

  const ElfW(Dyn) *dynamic= _DYNAMIC;
  const ElfW(Dyn) *dt_rpath= NULL;
  const ElfW(Dyn) *dt_runpath= NULL;
  const char *strtab= NULL;
  while (dynamic->d_tag != DT_NULL)
  {
    if (dynamic->d_tag == DT_RPATH) dt_rpath= dynamic;
    if (dynamic->d_tag == DT_RUNPATH) dt_runpath= dynamic;
    if (dynamic->d_tag == DT_STRTAB) strtab= (const char *)dynamic->d_un.d_val;
    ++dynamic;
  }
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
  rval= findmyso_strcat(buffer, &buffer_length, "/* DT_RPATH */\n", buffer_max_length);
  if (rval != FINDMYSO_OK) return rval;
#endif
  if ((strtab != NULL) && (dt_rpath != NULL))
  {
    rval= findmyso_libraries_or_files(sonames, strtab + dt_rpath->d_un.d_val, buffer, &buffer_length, "DT_RPATH", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }

  char *ld_library_path= getenv("LD_LIBRARY_PATH");
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
  rval= findmyso_strcat(buffer, &buffer_length, "/* LD_LIBRARY_PATH */\n", buffer_max_length);
  if (rval != FINDMYSO_OK) return rval;
#endif
  if (ld_library_path != NULL)
  {
    rval= findmyso_libraries_or_files(sonames, ld_library_path, buffer, &buffer_length, "LD_LIBRARY_PATH", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }

#if (FINDMYSO_INCLUDE_COMMENTS == 1)
  rval= findmyso_strcat(buffer, &buffer_length, "/* DT_RUNPATH */\n", buffer_max_length);
  if (rval != FINDMYSO_OK) return rval;
#endif
  if ((strtab != NULL) && (dt_runpath != NULL))
  {
    rval= findmyso_libraries_or_files(sonames, strtab + dt_runpath->d_un.d_val, buffer, &buffer_length, "DT_RUNPATH", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }

  char *ld_run_path= getenv("LD_RUN_PATH");
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
  rval= findmyso_strcat(buffer, &buffer_length, "/* LD_RUN_PATH */\n", buffer_max_length);
  if (rval != FINDMYSO_OK) return rval;
#endif
  if (ld_run_path != NULL)
  {
    rval= findmyso_libraries_or_files(sonames, ld_run_path, buffer, &buffer_length, "LD_RUN_PATH", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }

  {
    /* Assume library is whatever's between first and last / on a line */
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
    rval= findmyso_strcat(buffer, &buffer_length, "/* ld.so.cache */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    char ld_so_cache_line[FINDMYSO_MAX_PATH_LENGTH * 5];
    char popen_arg[FINDMYSO_MAX_PATH_LENGTH];
    strcpy(popen_arg, "/sbin/ldconfig -p  2>/dev/null ");
    FILE *fp;
    fp= popen(popen_arg, "r");
    /* e.g. FILE *fp= popen("ldconfig -p", "r"); */
    if (fp != NULL) /* popen failure unlikely even if ldconfig not found */
    {
      int counter= 0;
      while (fgets(ld_so_cache_line, sizeof(ld_so_cache_line), fp) != NULL)
      {
        ++counter;
        if (findmyso_find_line_in_sonames(sonames, ld_so_cache_line) == 0) continue; /* doesn't match requirement */
        char *address= strchr(ld_so_cache_line,'/');
        if (address != NULL)
        {
          rval= findmyso_strcat(buffer, &buffer_length, address, buffer_max_length);
          if (rval != FINDMYSO_OK) return rval;
        }
      }
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
    if (counter == 0)
    {
      rval= findmyso_strcat(buffer, &buffer_length, "/* /sbin/ldconfig -r failed */\n", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
    }
#endif
      pclose(fp);
    }
  }

  if (extra_paths != NULL)
  {
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
    rval= findmyso_strcat(buffer, &buffer_length, "/* extra_paths */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    {
      rval= findmyso_libraries_or_files(sonames, extra_paths, buffer, &buffer_length, "extra_paths", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
    }
  }

  return FINDMYSO_OK;
}

/* Pass: list of libraries separated by colons (or spaces or semicolons depending on char *type)
   I don't check wheher the colon is enclosed within ""s and don't expect a path name to contain a colon.
   If this is called for LD_PRELOAD then librarylist is actually a filelist, which should still be okay.
*/
int findmyso_libraries_or_files (const char *sonames, const char *librarylist, char *buffer, unsigned int *buffer_length,
                                 const char *type, unsigned int buffer_max_length)
{
  int rval;
  char delimiter1, delimiter2;
  if (strcmp(type, "LD_AUDIT") == 0)        { delimiter1= ':'; delimiter2= ':'; } /* colon or colon */
  else if (strcmp(type, "LD_PRELOAD") == 0)      { delimiter1= ':'; delimiter2= ' '; } /* colon or space */
  else if (strcmp(type, "DT_RPATH") == 0)        { delimiter1= ':'; delimiter2= ':'; } /* colon or colon */
  else if (strcmp(type, "LD_LIBRARY_PATH") == 0) { delimiter1= ':'; delimiter2= ';'; } /* colon or semicolon */
  else if (strcmp(type, "DT_RUNPATH") == 0)      { delimiter1= ':'; delimiter2= ':'; } /* colon or colon */
  else if (strcmp(type, "LD_RUN_PATH") == 0)     { delimiter1= ':'; delimiter2= ':'; } /* colon or colon */
  else /* extra_paths */                         { delimiter1= ':'; delimiter2= ';'; } /* colon or semicolon */
  char one_library_or_file[FINDMYSO_MAX_PATH_LENGTH + 1];
  char *p_out= &one_library_or_file[0];
  const char *p_in= librarylist;
  for (;;)
  {
    if ((*p_in == delimiter1) || (*p_in == delimiter2) || (*p_in == '\0'))
    {
      *p_out= '\0';
      char *pointer_to_one_library_or_file= &one_library_or_file[0];
      while (*pointer_to_one_library_or_file == ' ') ++pointer_to_one_library_or_file; /* skip lead spaces */
      while (p_out > pointer_to_one_library_or_file) /* skip trail spaces */
      {
        --p_out;
        if (*p_out == ' ') *p_out= '\0';
        else break;
      }

      if (strlen(pointer_to_one_library_or_file) > 0) /* If it's a blank we skip it. Is that right? */
      {
/*        char popen_arg[FINDMYSO_MAX_PATH_LENGTH]; */
        char ls_line[FINDMYSO_MAX_PATH_LENGTH + 1];
        {
          /* strcpy(ls_line, pointer_to_one_library_or_file); */
          char *p_line_out= &ls_line[0];
          for (char *p_line_in= pointer_to_one_library_or_file; *p_line_in != '\0'; ++p_line_in)
          {
            const char *strcatter;
            char buffer_for_replacement[FINDMYSO_MAX_PATH_LENGTH + 1];
            int replacee_length;
            if (strncmp(p_line_in, "$ORIGIN", 7) == 0)
            {
#ifdef OCELOT_OS_FREEBSD
              strcatter= "/* Guessing $ORIGIN from /proc/curproc/file */\n";
              readlink("/proc/curproc/file", buffer_for_replacement, FINDMYSO_MAX_PATH_LENGTH);
#else
              strcatter= "/* Guessing $ORIGIN from /proc/self/exe */\n";
              readlink("/proc/self/exe", buffer_for_replacement, FINDMYSO_MAX_PATH_LENGTH);
#endif
              /* warning: dirname() changes buffer_for_origin so don't use it twice */
              strcpy(buffer_for_replacement, dirname(buffer_for_replacement));
              replacee_length= 7;
            }
            else if (strncmp(p_line_in, "$LIB", 4) == 0)
            {
              strcatter= "/* Guessing $LIB from (sizeof(void*)) */\n";
              if ((sizeof(void*)) == 8) strcpy(buffer_for_replacement, "lib64");
              else strcpy(buffer_for_replacement, "lib");
              replacee_length= 4;
            }
            else if (strncmp(p_line_in, "$PLATFORM", 9) == 0)
            {
              strcatter= "/* Guessing $PLATFORM from uname -a */\n";
              FILE *fp= popen("uname -m 2>/dev/null", "r");
              if (fgets(buffer_for_replacement, sizeof(buffer_for_replacement), fp) == NULL)
                strcpy(buffer_for_replacement, "?");
              pclose(fp);
              replacee_length= 9;
            }
            else
            {
              *p_line_out= *p_line_in;
              ++p_line_out;
              continue;
            }
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
            rval= findmyso_strcat(buffer, buffer_length, strcatter, buffer_max_length);
            if (rval != FINDMYSO_OK) return rval;
#endif
            strcat(p_line_out, buffer_for_replacement);
            p_line_out+= strlen(buffer_for_replacement);
            if (*(p_line_out - 1) == '\n') --p_line_out;
            p_line_in+= replacee_length - 1;
          }
          *p_line_out= '\0';
        }
        strcpy(one_library_or_file, ls_line);

        if (strcmp(type, "LD_PRELOAD") == 0) /* if LD_PRELOAD we want a directory name */
        {
          if (findmyso_find_line_in_sonames(sonames, one_library_or_file) == 0) continue; /* doesn't match requirement */
#if (FINDMYSO_INCLUDE_COMMENTS == 1)
          if (access(one_library_or_file, F_OK) != 0)
          {
            rval= findmyso_strcat(buffer, buffer_length, "/* file access failed */\n", buffer_max_length);
            if (rval != FINDMYSO_OK) return rval;
          }
#endif
          rval= findmyso_strcat(buffer, buffer_length, one_library_or_file, buffer_max_length);
          if (rval != FINDMYSO_OK) return rval;
          rval= findmyso_strcat(buffer, buffer_length, "\n", buffer_max_length);
          if (rval != FINDMYSO_OK) return rval;
          continue;
        }

        /* not LD_PRELOAD so it should be a directory name */
        {
          /* char ls_line[FINDMYSO_MAX_PATH_LENGTH * 5]; */
          DIR* dir= opendir(one_library_or_file);
          if (dir != NULL) /* perhaps would be null if directory not found */
          {
            struct dirent* dirent;
            while ((dirent= readdir(dir)) != NULL)
            {
              if ((dirent->d_type !=  DT_REG) &&  (dirent->d_type !=  DT_LNK)) continue; /* not regular file or symbolic link */
              if (findmyso_find_line_in_sonames(sonames, dirent->d_name) == 0) continue; /* doesn't match requirement */
              {
                rval= findmyso_strcat(buffer, buffer_length, one_library_or_file, buffer_max_length);
                if (rval != FINDMYSO_OK) return rval;
                unsigned int len= strlen(one_library_or_file);
                if (len == 0) {;}
                else
                {
                  if (one_library_or_file[len - 1] != '/')
                  {
                    rval= findmyso_strcat(buffer, buffer_length, "/", buffer_max_length);
                    if (rval != FINDMYSO_OK) return rval;
                  }
                }
              }
              rval= findmyso_strcat(buffer, buffer_length, dirent->d_name, buffer_max_length);
              if (rval != FINDMYSO_OK) return rval;
              rval= findmyso_strcat(buffer, buffer_length, "\n", buffer_max_length);
              if (rval != FINDMYSO_OK) return rval;
            }
            closedir(dir);
          }
        }
      }
      if (*p_in == '\0') break;
      ++p_in;
      p_out= &one_library_or_file[0];
    }
    *p_out= *p_in;
    ++p_out;
    ++p_in;
  }
  return FINDMYSO_OK;
}

int findmyso_strcat(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length)
{
  unsigned int line_length= strlen(line);
  const char *pointer_to_line= line;
  while (*pointer_to_line == ' ') {++pointer_to_line; --line_length; } /* skip lead spaces */
  while ((line_length > 0) && (*(pointer_to_line + line_length - 1) == ' ')) --line_length; /* skip trail spaces */
  if (*buffer_length + line_length > buffer_max_length) return FINDMYSO_ERROR_BUFFER_MAX_LENGTH_TOO_SMALL;
  memcpy(buffer + *buffer_length, pointer_to_line, line_length);
  *buffer_length+= line_length;
  *(buffer + *buffer_length)= '\0';
  return FINDMYSO_OK;
}

/*
  Compare line to each of the items in sonames. If match for number-of-characters-in-soname: 1 true. Else: 0 false.
  Assume that line cannot contain ':' and ':' within sonames is a delimiter.
  Ignore lead or trail spaces.
  Treat \n as end of line and ignore it.
  We have already checked that strlen(each soname) is <= FINDMYSO_MAX_PATH_LENGTH.
  Unfortunately ldconfig -p lines start with a control character (tab?) which complicates the skipping.
*/
int findmyso_find_line_in_sonames(const char *sonames, const char *line)
{
  const char *pointer_to_line= line;
  while (*pointer_to_line <= ' ') ++pointer_to_line; /* skip lead spaces (or control characters!) in line */
  unsigned int line_length= strlen(pointer_to_line);
  if ((line_length > 1) && (pointer_to_line[line_length - 1] == '\n')) --line_length; /* skip trail \n */
  while ((line_length > 0) && (pointer_to_line[line_length - 1] == ' ')) --line_length; /* skip trail spaces */
  if (line_length == 0) return 0; /* false */
  const char *pointer_to_sonames= sonames;
  for (;;)
  {
    while (*pointer_to_sonames == ' ') ++pointer_to_sonames; /* skip lead spaces in soname */
    unsigned int soname_length= 0;
    while ((*(pointer_to_sonames + soname_length) != ' ')
     && (*(pointer_to_sonames + soname_length) != ':')
     && (*(pointer_to_sonames + soname_length)  != '\0'))
      ++soname_length;
    if ((soname_length > 0) && (soname_length <= line_length))
    {
      if (memcmp(pointer_to_line, pointer_to_sonames, soname_length) == 0) return 1; /* true */
    }
    pointer_to_sonames= strstr(pointer_to_sonames, ":"); /* point to next ":" within sonames */
    if (pointer_to_sonames == NULL) return 0;/* false, no more sonames */
    ++pointer_to_sonames; /* point past the "." */
  }
  return 0; /* false (though actually we shouldn't get this far) */
}
