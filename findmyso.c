/*
  findmyso.c --   To get a list of paths and .so files along
  ld_audit + ld_preload + -rpath (dt_rpath) + ld_library_path + -rpath (dt_run_path) + ld_run_path + /etc/ld.so.cache + extra-paths

   Version: 0.9.3
   Last modified: March 9 2025

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
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

#include "findmyso.h" /* definition of findmyso(), and some #define FINDMYSO_... */

#if (FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH == 1)
#include <elf.h>
#include <link.h>
#endif

static int findmyso_libraries_or_files(const char *sonames, const char *librarylist, char *buffer,
                                       unsigned int *buffer_length, const char *type, unsigned int buffer_max_length,
                                       const char *lib, const char *platform, const char *origin);
static int findmyso_strcat(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length);
static int findmyso_find_line_in_sonames(const char *sonames, const char *line);
int findmyso_replace_lib_or_platform_or_origin(char* one_library_or_file, unsigned int *replacements_count, const char *lib, const char *platform, const char *origin);
int findmyso_get_origin_and_lib_and_platform(char* origin, char* lib, char *platform,
                                              char *buffer, unsigned int *buffer_length, unsigned buffer_max_length);

#if (FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH == 1)
/* Ordinarily link.h has extern ElfW(Dyn) _DYNAMIC but it's missing with FreeBSD */
extern __attribute__((weak)) ElfW(Dyn) _DYNAMIC[];
#endif

int findmyso(const char *sonames, char *buffer, unsigned int buffer_max_length, const char *extra_paths)
{
  if ((buffer == NULL) || (sonames == NULL)) return FINDMYSO_ERROR_NULL;

  {
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
  }

  unsigned int buffer_length= 0;
  int rval;

#if (FINDMYSO_WARNING_LEVEL > 3)
  {
    char version[128];
    sprintf(version, "/* findmyso version %d.%d.%d */\n", FINDMYSO_VERSION_MAJOR, FINDMYSO_VERSION_MINOR, FINDMYSO_VERSION_PATCH);
   rval= findmyso_strcat(buffer, &buffer_length, version, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }
#endif

  char lib[FINDMYSO_MAX_PATH_LENGTH]= ""; /* Usually this will be changed to whatever $LIB is. */
  char platform[FINDMYSO_MAX_PATH_LENGTH]= ""; /* Usually this will be changed to whatever $LIB is. */
  char origin[FINDMYSO_MAX_PATH_LENGTH]= ""; /* Usually this will be changed to whatever $LIB is. */
  rval= findmyso_get_origin_and_lib_and_platform(origin, lib, platform, buffer, &buffer_length, buffer_max_length);
  if (rval != FINDMYSO_OK) return rval;

#if (FINDMYSO_WARNING_LEVEL > 3)
  {
    char lib_platform_origin_comment[FINDMYSO_MAX_PATH_LENGTH * 4];
    sprintf(lib_platform_origin_comment, "/* $LIB=%s $PLATFORM=%s $ORIGIN=%s */\n", lib, platform, origin);
    rval= findmyso_strcat(buffer, &buffer_length, lib_platform_origin_comment, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }
#endif

#if (FINDMYSO_INCLUDE_AUDIT == 1)
  {
    const char *ld_audit= getenv("LD_AUDIT");
#if (FINDMYSO_WARNING_LEVEL > 2)
      rval= findmyso_strcat(buffer, &buffer_length, "/* LD_AUDIT */\n", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
#endif
    if (ld_audit != NULL)
    {
      if (ld_audit != NULL)
      {
        rval= findmyso_libraries_or_files(sonames, ld_audit, buffer, &buffer_length, "LD_AUDIT", buffer_max_length, lib, platform, origin);
        if (rval != FINDMYSO_OK) return rval;
      }
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_AUDIT == 1) */

#if (FINDMYSO_INCLUDE_LD_PRELOAD == 1)
  {
    const char *ld_preload= getenv("LD_PRELOAD");
#if (FINDMYSO_WARNING_LEVEL > 2)
    rval= findmyso_strcat(buffer, &buffer_length, "/* LD_PRELOAD */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    if (ld_preload != NULL)
    {
      rval= findmyso_libraries_or_files(sonames, ld_preload, buffer, &buffer_length, "LD_PRELOAD", buffer_max_length, lib, platform, origin);
      if (rval != FINDMYSO_OK) return rval;
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_LD_PRELOAD == 1) */

#if (FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH == 1)
  const ElfW(Dyn) *dynamic= _DYNAMIC;
  const ElfW(Dyn) *dt_rpath= NULL;
  const ElfW(Dyn) *dt_runpath= NULL;
  const char *dt_strtab= NULL;
  /* in theory "extern __attribute__((weak)) ... _DYNAMIC[];" could result in _DYNAMIC == NULL */
  if (_DYNAMIC == NULL)
  {
#if (FINDMYSO_WARNING_LEVEL > 0)
    rval= findmyso_strcat(buffer, &buffer_length, "/* Cannot read DT_RPATH because _DYNAMIC is NULL */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
  }
  else /* _DYNAMIC != NULL */
  {
    while (dynamic->d_tag != DT_NULL)
    {
      if (dynamic->d_tag == DT_RPATH) dt_rpath= dynamic;
      if (dynamic->d_tag == DT_RUNPATH) dt_runpath= dynamic;
      if (dynamic->d_tag == DT_STRTAB) dt_strtab= (const char *)dynamic->d_un.d_val;
      ++dynamic;
    }
#if (FINDMYSO_WARNING_LEVEL > 2)
    rval= findmyso_strcat(buffer, &buffer_length, "/* DT_RPATH */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    if ((dt_strtab != NULL) && (dt_rpath != NULL))
    {
      rval= findmyso_libraries_or_files(sonames, dt_strtab + dt_rpath->d_un.d_val, buffer, &buffer_length, "DT_RPATH", buffer_max_length, lib, platform, origin);
      if (rval != FINDMYSO_OK) return rval;
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH == 1) */

#if (FINDMYSO_INCLUDE_LD_LIBRARY_PATH == 1)
  {
    const char *ld_library_path= getenv("LD_LIBRARY_PATH");
#if (FINDMYSO_WARNING_LEVEL > 2)
    rval= findmyso_strcat(buffer, &buffer_length, "/* LD_LIBRARY_PATH */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    if (ld_library_path != NULL)
    {
      rval= findmyso_libraries_or_files(sonames, ld_library_path, buffer, &buffer_length, "LD_LIBRARY_PATH", buffer_max_length, lib, platform, origin);
      if (rval != FINDMYSO_OK) return rval;
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_LD_LIBRARY_PATH == 1) */

#if (FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH == 1)
  if (_DYNAMIC == NULL)
  {
#if (FINDMYSO_WARNING_LEVEL > 0)
    rval= findmyso_strcat(buffer, &buffer_length, "/* Cannot read DT_RUNPATH because _DYNAMIC is NULL */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    ;
  }
  else /* if (_DYNAMIC != NULL) */
  {
#if (FINDMYSO_WARNING_LEVEL > 2)
    rval= findmyso_strcat(buffer, &buffer_length, "/* DT_RUNPATH */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    if ((dt_strtab != NULL) && (dt_runpath != NULL))
    {
      rval= findmyso_libraries_or_files(sonames, dt_strtab + dt_runpath->d_un.d_val, buffer, &buffer_length, "DT_RUNPATH", buffer_max_length, lib, platform, origin);
      if (rval != FINDMYSO_OK) return rval;
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_DT_RPATH_OR_DT_RUNPATH == 1) */

#if (FINDMYSO_INCLUDE_LD_RUN_PATH == 1)
  {
    const char *ld_run_path= getenv("LD_RUN_PATH");
#if (FINDMYSO_WARNING_LEVEL > 3)
    rval= findmyso_strcat(buffer, &buffer_length, "/* LD_RUN_PATH */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    if (ld_run_path != NULL)
    {
      rval= findmyso_libraries_or_files(sonames, ld_run_path, buffer, &buffer_length, "LD_RUN_PATH", buffer_max_length, lib, platform, origin);
      if (rval != FINDMYSO_OK) return rval;
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_LD_RUN_PATH == 1) */

#if (FINDMYSO_INCLUDE_LD_SO_CACHE == 1)
  {
#if (FINDMYSO_WARNING_LEVEL > 2)
    rval= findmyso_strcat(buffer, &buffer_length, "/* ld.so.cache */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    const char *ldconfig; /* must be able to access ldconfig in some standard directory or user's path */
    for (int i= 0; i <= 5; ++i)
    {
      if (i == 0) ldconfig= "/sbin/ldconfig";
      else if (i == 1) ldconfig= "/usr/sbin/ldconfig";
      else if (i == 2) ldconfig= "/bin/ldconfig";
      else if (i == 3) ldconfig= "/usr/bin/ldconfig";
      else if (i == 4) ldconfig= "ldconfig";
      else if (i == 5) { ldconfig= NULL; break; }
      if (access(ldconfig, X_OK) == 0) break;
    }
    if (ldconfig == NULL)
    {
#if (FINDMYSO_WARNING_LEVEL > 0)
      rval= findmyso_strcat(buffer, &buffer_length, "/* access('...ldconfig', X_OK) failed */\n", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
#endif
      ;
    }
    else
    {
      /* Assume library is whatever's between first and last / on a line */
      /* Line 1 is probably "[n] libs found in cache `/etc/ld.so.cache'" */
      /* First ldconfig attempt will be Linux-style ldconfig -p, second will be FreeBSD-style ldconfig -r */
      int counter= 0;
      for (int ldconfig_attempts= 0; ldconfig_attempts < 2; ++ldconfig_attempts)
      {
        char ld_so_cache_line[FINDMYSO_MAX_PATH_LENGTH * 5];
        char popen_arg[FINDMYSO_MAX_PATH_LENGTH];
        if (ldconfig_attempts == 0) sprintf(popen_arg, "LD_LIBRARY_PATH= LD_DEBUG= LD_PRELOAD= %s -p 2>/dev/null ", ldconfig);
        else sprintf(popen_arg, "%s -r 2>/dev/null ", ldconfig);
        counter= 0;
        FILE *fp;
        fp= popen(popen_arg, "r");
        if (fp != NULL) /* popen failure unlikely even if ldconfig not found */
        {
          while (fgets(ld_so_cache_line, sizeof(ld_so_cache_line), fp) != NULL)
          {
            ++counter;
            char* pointer_to_ld_so_cache_line= ld_so_cache_line + strlen(ld_so_cache_line);
            for (;;)
            {
              if (*pointer_to_ld_so_cache_line == '/') break;
              if (pointer_to_ld_so_cache_line <= ld_so_cache_line) break;
              --pointer_to_ld_so_cache_line;
            }
            if (pointer_to_ld_so_cache_line == ld_so_cache_line) continue; /* blank line */
            ++pointer_to_ld_so_cache_line; /* So pointer is just after the final / which should be at the file name */
            if (findmyso_find_line_in_sonames(sonames, pointer_to_ld_so_cache_line) == 0) continue; /* doesn't match requirement */
            char *address= strchr(ld_so_cache_line,'/');
            if (address != NULL)
            {
              rval= findmyso_strcat(buffer, &buffer_length, address, buffer_max_length);
              if (rval != FINDMYSO_OK) return rval;
            }
          }
          pclose(fp);
        }
        if (counter > 0) break; /* So if ldconfig -p succeeds, don't try ldconfig -r */
      }
#if (FINDMYSO_WARNING_LEVEL > 0)
      if (counter == 0)
      {
        char comment[256];
        sprintf(comment, "/* %s -p or %s -r failed */\n", ldconfig, ldconfig); 
        rval= findmyso_strcat(buffer, &buffer_length, comment, buffer_max_length);
        if (rval != FINDMYSO_OK) return rval;
      }
#endif
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_LD_SO_CACHE == 1) */

#if (FINDMYSO_INCLUDE_EXTRA_PATHS == 1)
  {
    if (extra_paths != NULL)
    {
#if (FINDMYSO_WARNING_LEVEL > 2)
      rval= findmyso_strcat(buffer, &buffer_length, "/* extra_paths */\n", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
#endif
      {
        rval= findmyso_libraries_or_files(sonames, extra_paths, buffer, &buffer_length, "extra_paths", buffer_max_length, lib, platform, origin);
        if (rval != FINDMYSO_OK) return rval;
      }
    }
  }
#endif /* #if (FINDMYSO_INCLUDE_EXTRA_PATHS == 1) */

  return FINDMYSO_OK;
}

/* Pass: list of libraries separated by colons (or spaces or semicolons depending on char *type)
   I don't check wheher the colon is enclosed within ""s and don't expect a path name to contain a colon.
   If this is called for LD_AUDIT or LD_PRELOAD then librarylist is actually a filelist, which should still be okay.
*/
int findmyso_libraries_or_files(const char *sonames, const char *librarylist, char *buffer, unsigned int *buffer_length,
                                const char *type, unsigned int buffer_max_length,
                                const char *lib, const char *platform, const char *origin)
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
      {
        char *pointer_to_one_library_or_file= &one_library_or_file[0];
        while (*pointer_to_one_library_or_file == ' ') ++pointer_to_one_library_or_file; /* skip lead spaces */
        while (p_out > pointer_to_one_library_or_file) /* skip trail spaces */
        {
          --p_out;
          if (*p_out == ' ') *p_out= '\0';
          else break;
        }
        strcpy(one_library_or_file, pointer_to_one_library_or_file);
      }
      if (strlen(one_library_or_file) > 0) /* If it's a blank we skip it. Is that right? */
      {
        unsigned int replacements_count;
        char orig_one_library_or_file[FINDMYSO_MAX_PATH_LENGTH + 1];
        strcpy(orig_one_library_or_file, one_library_or_file);
        rval= findmyso_replace_lib_or_platform_or_origin(one_library_or_file, &replacements_count, lib, platform, origin);
        if (rval !=FINDMYSO_OK) return rval;
#if (FINDMYSO_WARNING_LEVEL > 3)
        if (replacements_count > 0)
        {
          char comment[FINDMYSO_MAX_PATH_LENGTH*2 + 128];
          sprintf(comment, "/* replaced %s with %s */\n", orig_one_library_or_file, one_library_or_file);
          rval= findmyso_strcat(buffer, buffer_length, comment, buffer_max_length);
          if (rval != FINDMYSO_OK) return rval;
        }
#endif
        if ((strcmp(type, "LD_AUDIT") == 0) || (strcmp(type, "LD_PRELOAD") == 0)) /* if LD_AUDIT or LD_PRELOAD we want a file name */
        {
          if (findmyso_find_line_in_sonames(sonames, one_library_or_file) == 0) continue; /* doesn't match requirement */
#if (FINDMYSO_WARNING_LEVEL > 1)
          if (access(one_library_or_file, X_OK) != 0)
          {
            rval= findmyso_strcat(buffer, buffer_length, "/* access(filename, X_OK) failed for following file */\n", buffer_max_length);
            if (rval != FINDMYSO_OK) return rval;
          }
#endif
          rval= findmyso_strcat(buffer, buffer_length, one_library_or_file, buffer_max_length);
          if (rval != FINDMYSO_OK) return rval;
          rval= findmyso_strcat(buffer, buffer_length, "\n", buffer_max_length);
          if (rval != FINDMYSO_OK) return rval;
          continue; /* todo: find out why */
        }

        /* not LD_AUDIT or LD_PRELOAD so it should be a directory name */
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
  Beware: ldconfig -p lines start with a control character (tab?).
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

/*
  Pass: library or file name
  Do: replace with same library or file name, except that $ORIGIN or $LIB or $PLATFORM are replaced
  Return: rval
  Todo: It seems loader replaces $LIB or $LIB/ but not $LIB in $LIBxxx, this replaces $LIB in $LIBxxx too.
*/
int findmyso_replace_lib_or_platform_or_origin(char* one_library_or_file, unsigned int *replacements_count, const char *lib, const char *platform, const char *origin)
{
  *replacements_count= 0;
  if (strchr(one_library_or_file, '$') == NULL) return FINDMYSO_OK;
  char buffer_for_output[FINDMYSO_MAX_PATH_LENGTH*2 + 1];
  char *p_line_out= &buffer_for_output[0];
  *p_line_out= '\0';
  for (const char *p_line_in= one_library_or_file; *p_line_in != '\0';)
  {
    if (strncmp(p_line_in, "$ORIGIN", 7) == 0)
    {
      strcpy(p_line_out, origin);
      p_line_out+= strlen(origin);
      p_line_in+= strlen("$ORIGIN");
      ++*replacements_count;
    }
    else if (strncmp(p_line_in, "$LIB", 4) == 0)
    {
      strcpy(p_line_out, lib);
      p_line_out+= strlen(lib);
      p_line_in+= strlen("$LIB");
      ++*replacements_count;
    }
    else if (strncmp(p_line_in, "$PLATFORM", 9) == 0)
    {
      strcpy(p_line_out, platform);
      p_line_out+= strlen(platform);
      p_line_in+= strlen("$PLATFORM");
      ++*replacements_count;
    }
    else
    {
      *p_line_out= *p_line_in;
      ++p_line_out;
      ++p_line_in;
    }
    if ((p_line_out - &buffer_for_output[0]) > FINDMYSO_MAX_PATH_LENGTH)
      return FINDMYSO_ERROR_MAX_PATH_LENGTH_TOO_SMALL;
    *p_line_out= '\0';
  }
  strcpy(one_library_or_file, buffer_for_output);
  return FINDMYSO_OK;
}

/*
  Pass: "$LIB" or "$PLATFORM"
  Return: replacer has what would loader would change "$LIB" or "$PLATFORM" or "$ORIGIN" to
  Re method: Use a dummy utility, preferably bin/true, ls should also work but isn't as good
            (any program anywhere will provided it requires any .so, and libc.so is such).
            First attempt: read ELF to get "ELF interpreter" i.e. loader name,
            which probably is /lib64/ld-linux-x86-64.so.2 or /lib/ld-linux.so.2,
            then use it with LD_DEBUG to see search_path when executing the dummy utility.
            If that fails: same idea but just running the dummy.
            If that fails: (lib) lib64 for 64-bit, lib for 32-bit. (platform) uname -m.
  Todo: get PT_DYNAMIC the way we get PT_INTERN, instead of depending on extern _DYNAMIC
  Todo: If there is a /tls (thread local storage) subdirectory or an x86_64 subdirectory,
        search all. But show library search path=main, main-tls, main/x86_64
  Todo: If dynamic loader is not the usual e.g. due to "-Wl,-I/tmp/my_ld.so" then add a comment.
*/
int findmyso_get_origin_and_lib_and_platform(char* origin, char* lib, char* platform,
                                              char* buffer, unsigned int* buffer_length, unsigned int buffer_max_length)
{
  int lib_change_count= 0;
  int platform_change_count= 0;
  int rval= FINDMYSO_OK;

  {
    int readlink_return;
#ifdef FINDMYSO_FREEBSD
    readlink_return= readlink("/proc/curproc/file", origin, FINDMYSO_MAX_PATH_LENGTH);
#else
    readlink_return= readlink("/proc/self/exe", origin, FINDMYSO_MAX_PATH_LENGTH);
#endif
    if ((readlink_return < 0) || (readlink_return >= FINDMYSO_MAX_PATH_LENGTH))
    {
#if (FINDMYSO_WARNING_LEVEL > 1)
      rval= findmyso_strcat(buffer, buffer_length, "/* readlink failed so $ORIGIN is unknown */\n", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
#endif
      strcpy(origin, "");
    }
    else *(origin + readlink_return)= '\0';
    for (char *p= origin + strlen(origin); p != origin; --p)
    {
      if (*p == '/')
      {
        *p= '\0';
        break;
      }
    }
  }

#if (FINDMYSO_INCLUDE_GET_LIB_OR_PLATFORM == 1)
  /* utility name */
  char utility_name[256]= "";
  if (access("/bin/true", X_OK) == 0) strcpy(utility_name, "/bin/true");
  else if (access("/bin/cp", X_OK) == 0) strcpy(utility_name, "/bin/cp");
  else if (access("/usr/bin/true", X_OK) == 0) strcpy(utility_name, "/usr/bin/true");
  else if (access("/usr/bin/cp", X_OK) == 0) strcpy(utility_name, "/usr/bin/cp");
#if (FINDMYSO_WARNING_LEVEL > 1)
  if (strcmp(utility_name, "") == 0)
  {
    rval= findmyso_strcat(buffer, buffer_length, "/* no access to [/usr]/bin/true or [/usr]/cp */\n", buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
  }
#endif

extern void* __executable_start;
  ElfW(Ehdr)*ehdr= NULL;
  if (__executable_start != NULL)
  {
    ehdr= (ElfW(Ehdr)*) &__executable_start; /* linker e.g. ld.bfd or ld.lld is supposed to add this */
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
#if (FINDMYSO_WARNING_LEVEL > 1)
      rval= findmyso_strcat(buffer, buffer_length, "/* ehdr->ident not valid */\n", buffer_max_length);
      if (rval != FINDMYSO_OK) return rval;
#endif
      ehdr= NULL;
    }
  }

  /* first attempt */

  const char* dynamic_loader_name= NULL;
  if (ehdr != NULL)
  {
    const char*cc= (char*)ehdr; /* offsets are in bytes so I prefer to use a byte pointer */
    cc+= ehdr->e_phoff; /* -> start of program headers */
    ElfW(Phdr)*phdr;
    for (unsigned int i= 0; i < ehdr->e_phnum; ++i) /* loop through program headers */
    {
      phdr= (ElfW(Phdr)*)cc;
      if (phdr->p_type == PT_INTERP) /* i.e. ELF interpreter */
      {
        char*cc2= (char*)ehdr;
        cc2+= phdr->p_offset;
        dynamic_loader_name= cc2;
        break;
      }
      cc+= ehdr->e_phentsize;
    }
  }

  if (dynamic_loader_name == NULL)
  {
    if ((sizeof(void*)) == 8) dynamic_loader_name= "/lib64/ld-linux-x86-64.so.2"; /* make some gcc/glibc assumptions */
    else dynamic_loader_name= "/lib/ld-linux.so.2";
#if (FINDMYSO_WARNING_LEVEL > 1)
    char comment[512];
    sprintf(comment, "/* can't get ehdr dynamic loader so assume %s */\n", dynamic_loader_name);
    rval= findmyso_strcat(buffer, buffer_length, comment, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
  }

  if (access(dynamic_loader_name, X_OK) != 0)
  {
#if (FINDMYSO_WARNING_LEVEL > 1)
    char comment[512];
    sprintf(comment, "/* can't access %s */\n", dynamic_loader_name);
    rval= findmyso_strcat(buffer, buffer_length, comment, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
    dynamic_loader_name= NULL;
  }

  if (dynamic_loader_name != NULL)
  {
#define REPLACEE_IS_LIB 0
#define REPLACEE_IS_PLATFORM 1
    for (int i= REPLACEE_IS_LIB; i <= REPLACEE_IS_PLATFORM; ++i) /* 0 means "$LIB", 1 means "$PLATFORM" */
    {
      char replacee[16];
      if (i == REPLACEE_IS_LIB) strcpy(replacee, "$LIB");
      else strcpy(replacee, "$PLATFORM");
      int change_count= 0;
      FILE *fp;
      char popen_arg[FINDMYSO_MAX_PATH_LENGTH + 1];
      sprintf(popen_arg,
      "env -u LD_DEBUG_OUTPUT LD_LIBRARY_PATH='/PRE_OOKPIK/%s/POST_OOKPIK' LD_DEBUG=libs %s --inhibit-cache %s 2>/dev/stdout",
      replacee, dynamic_loader_name, utility_name);
      fp= popen(popen_arg, "r");
      if (fp != NULL)
      {
        char buffer_for_ookpik[FINDMYSO_MAX_PATH_LENGTH + 1];
        while (fgets(buffer_for_ookpik, sizeof(buffer_for_ookpik), fp) != NULL)
        {
          const char *pre_ookpik= strstr(buffer_for_ookpik, "PRE_OOKPIK/");
          if (pre_ookpik == NULL) continue;
          const char *post_ookpik= strstr(pre_ookpik, "POST_OOKPIK");
          if (post_ookpik == NULL) continue;
          pre_ookpik+= strlen("PRE_OOKPIK/");
          unsigned int len= post_ookpik - (pre_ookpik + 1);
          if (i == REPLACEE_IS_LIB) { memcpy(lib, pre_ookpik, len); *(lib + len)= '\0'; ++lib_change_count; }
          else { memcpy(platform, pre_ookpik, len); *(platform + len)= '\0'; ++platform_change_count; }
          ++change_count;
          break;
        }
        fclose(fp);
      }
      /* second attempt */
      if (change_count == 0)
      {
        sprintf(popen_arg,
        "env -u LD_DEBUG_OUTPUT LD_LIBRARY_PATH='/PRE_OOKPIK/%s/POST_OOKPIK' LD_DEBUG=libs %s 2>/dev/stdout",
        replacee, utility_name);
        fp= popen(popen_arg, "r");
        if (fp != NULL)
        {
          char buffer_for_ookpik[FINDMYSO_MAX_PATH_LENGTH + 1];
          while (fgets(buffer_for_ookpik, sizeof(buffer_for_ookpik), fp) != NULL)
          {
            const char *pre_ookpik= strstr(buffer_for_ookpik, "PRE_OOKPIK/");
            if (pre_ookpik == NULL) continue;
            const char *post_ookpik= strstr(pre_ookpik, "POST_OOKPIK");
            if (post_ookpik == NULL) continue;
            pre_ookpik+= strlen("PRE_OOKPIK/");
            unsigned int len= post_ookpik - (pre_ookpik + 1);
            if (i == REPLACEE_IS_LIB) { memcpy(lib, pre_ookpik, len); *(lib + len)= '\0'; ++lib_change_count; }
            else { memcpy(platform, pre_ookpik, len); *(platform + len)= '\0'; ++platform_change_count; }
            break;
          }
          fclose(fp);
        }
      }
    }
  }
#endif
  if (lib_change_count == 0)
  {
    if ((sizeof(void*)) == 8) strcpy(lib, "lib64"); /* default $LIB if findmyso__get_lib_or_platform doesn't work */
    else strcpy(lib, "lib");
#if (FINDMYSO_WARNING_LEVEL > 1)
    char comment[512];
    sprintf(comment, "/* assuming $LIB is %s */\n", lib);
    rval= findmyso_strcat(buffer, buffer_length, comment, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
  }

  if (platform_change_count == 0)
  {
    char buffer_for_replacement[FINDMYSO_MAX_PATH_LENGTH + 1]= "?";
    FILE *fp= popen("LD_LIBRARY_PATH= LD_DEBUG= LD_PRELOAD= uname -m 2>/dev/null", "r");
    if (fp != NULL)
    {
      if (fgets(buffer_for_replacement, sizeof(buffer_for_replacement), fp) == NULL)
        strcpy(buffer_for_replacement, "?");
      pclose(fp);
    }
    char* pointer_to_n= strchr(buffer_for_replacement, '\n');
    if (pointer_to_n != NULL) *pointer_to_n= '\0';
    strcpy(platform, buffer_for_replacement);
#if (FINDMYSO_WARNING_LEVEL > 1)
    char comment[512];
    sprintf(comment, "/* assuming $PLATFORM is %s */\n", platform);
    rval= findmyso_strcat(buffer, buffer_length, comment, buffer_max_length);
    if (rval != FINDMYSO_OK) return rval;
#endif
  }
  return rval;
}
