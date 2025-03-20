/*
  pgfindlib.c --   To get a list of paths and .so files along
  ld_audit + ld_preload + -rpath (dt_rpath) + ld_library_path + -rpath (dt_run_path) + ld_run_path + /etc/ld.so.cache + default + extra-paths

   Version: 0.9.6
   Last modified: March 20 2025

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

  For explanation see the README.md file which comes with the pgfindlib package.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

#include "pgfindlib.h" /* definition of pgfindlib(), and some #define PGFINDLIB_... */

#if (PGFINDLIB_TOKEN_SOURCE_DT_RPATH_OR_DT_RUNPATH != 0)
#include <elf.h>
#include <link.h>
#endif

/* todo: don't ask for this if we do not stat */
#include <sys/stat.h>

#ifdef PGFINDLIB_FREEBSD
#include <sys/auxv.h>
#endif

#include <errno.h>

struct tokener
{
  const char* tokener_name;
  char tokener_length;
  char tokener_comment_id;
};

static int pgfindlib_strcat(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length);
/* todo: make this obsolete */
static int pgfindlib_comment(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length,
                             unsigned int comment_number);
static int pgfindlib_comment_in_row(char *comment, unsigned int comment_number, int additional_number);

static int pgfindlib_keycmp(const unsigned char *a, unsigned int a_len, const unsigned char *b);
static int pgfindlib_tokenize(const char* statement, struct tokener tokener_list[],
                              unsigned int* row_number,
                              char *buffer, unsigned int *buffer_length, unsigned int buffer_max_length);


static int pgfindlib_file(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length,
                          unsigned int* row_number,
                          ino_t inode_list[], unsigned int* inode_count, unsigned int* inode_warning_count,
                          struct tokener tokener_list_item);

static int pgfindlib_find_line_in_statement(const struct tokener tokener_list[], const char *line);
static int pgfindlib_row_version(char *buffer, unsigned int* buffer_length, unsigned buffer_max_length, unsigned int* row_number,
                                 ino_t inode_list[], unsigned int* inode_count);
static int pgfindlib_row_lib(char* buffer, unsigned int* buffer_length, unsigned buffer_max_length, unsigned int* row_number,
                             ino_t inode_list[], unsigned int* inode_count,
                             const char* lib, const char* platform, const char* origin);
int pgfindlib_row_source_name(char* buffer, unsigned int* buffer_length, unsigned buffer_max_length, unsigned int* row_number,
                             ino_t inode_list[], unsigned int* inode_count,
                             const char* source_name);

static int pgfindlib_replace_lib_or_platform_or_origin(char* one_library_or_file, unsigned int *replacements_count, const char *lib, const char *platform, const char *origin);
static int pgfindlib_get_origin_and_lib_and_platform(char* origin, char* lib, char *platform,
                                              char *buffer, unsigned int *buffer_length, unsigned buffer_max_length);
static int pgfindlib_so_cache(const struct tokener tokener_list[], int tokener_number,
                       char* malloc_buffer_1, unsigned int* malloc_buffer_1_length, unsigned malloc_buffer_1_max_length,
                       char** malloc_buffer_2, unsigned int* malloc_buffer_2_length, unsigned malloc_buffer_2_max_length);
static int pgfindlib_add_to_malloc_buffers(const char* new_item, int source_number,
                                    char* malloc_buffer_1, unsigned int* malloc_buffer_1_length, unsigned malloc_buffer_1_max_length,
                                    char** malloc_buffer_2, unsigned int* malloc_buffer_2_length, unsigned malloc_buffer_2_max_length);
                                          

#if (PGFINDLIB_TOKEN_SOURCE_DT_RPATH_OR_DT_RUNPATH != 0)
/* Ordinarily link.h has extern ElfW(Dyn) _DYNAMIC but it's missing with FreeBSD */
extern __attribute__((weak)) ElfW(Dyn) _DYNAMIC[];
#endif

/* pgfindlib_standard_source_array and pgfindlib_standard_source_array_n must match. */
const char* pgfindlib_standard_source_array[] = {"LD_AUDIT", "LD_PRELOAD", "DT_RPATH", "LD_LIBRARY_PATH", "DT_RUNPATH",
                                                 "LD_RUN_PATH", "ld.so.cache", "default_paths", "extra_paths", ""};
const char pgfindlib_standard_source_array_n[]= {PGFINDLIB_TOKEN_SOURCE_LD_AUDIT,PGFINDLIB_TOKEN_SOURCE_LD_PRELOAD,
                                                 PGFINDLIB_TOKEN_SOURCE_DT_RPATH, PGFINDLIB_TOKEN_SOURCE_LD_LIBRARY_PATH,
                                                 PGFINDLIB_TOKEN_SOURCE_DT_RUNPATH, PGFINDLIB_TOKEN_SOURCE_LD_RUN_PATH,
                                                 PGFINDLIB_TOKEN_SOURCE_LD_SO_CACHE, PGFINDLIB_TOKEN_SOURCE_DEFAULT_PATHS,
                                                 PGFINDLIB_TOKEN_SOURCE_EXTRA_PATHS, 0};
static int pgfindlib_qsort_compare(const void *p1, const void *p2);
static int pgfindlib_source_scan(const char *librarylist, char *buffer, unsigned int *buffer_length,
                                unsigned int tokener_number, unsigned int buffer_max_length,
                                const char *lib, const char *platform, const char *origin,
                                unsigned int* row_number,
                                ino_t inode_list[], unsigned int* inode_count, unsigned int* inode_warning_count,
                                struct tokener tokener_list[],
                                char* malloc_buffer_1, unsigned int* malloc_buffer_1_length, unsigned malloc_buffer_1_max_length,
                                char** malloc_buffer_2, unsigned int* malloc_buffer_2_length, unsigned malloc_buffer_2_max_length);
static int pgfindlib_call(const char *statement, char *buffer, unsigned int buffer_max_length);

int pgfindlib(const char *statement, char *buffer, unsigned int buffer_max_length)
{
  int rval= pgfindlib_call(statement, buffer, buffer_max_length);
  return rval;
}

int pgfindlib_call(const char *statement, char *buffer, unsigned int buffer_max_length)
{
  if ((buffer == NULL) || (statement == NULL)) return PGFINDLIB_ERROR_NULL;

  unsigned int buffer_length= 0;
  int rval;
  unsigned int row_number= 0;

  ino_t inode_list[PGFINDLIB_MAX_INODE_COUNT]; /* todo: disable if duplicate checking is off */
  unsigned int inode_count= 0;
  unsigned int inode_warning_count= 0;

#if (PGFINDLIB_ROW_VERSION != 0)
  pgfindlib_row_version(buffer, &buffer_length, buffer_max_length, &row_number, inode_list, &inode_count); /* first row including version number */
#endif

  char lib[PGFINDLIB_MAX_PATH_LENGTH]= ""; /* Usually this will be changed to whatever $LIB is. */
  char platform[PGFINDLIB_MAX_PATH_LENGTH]= ""; /* Usually this will be changed to whatever $LIB is. */
  char origin[PGFINDLIB_MAX_PATH_LENGTH]= ""; /* Usually this will be changed to whatever $LIB is. */
  rval= pgfindlib_get_origin_and_lib_and_platform(origin, lib, platform, buffer, &buffer_length, buffer_max_length);
  if (rval != PGFINDLIB_OK) return rval;

#if (PGFINDLIB_COMMENT_LIB != 0)
  {
    rval= pgfindlib_row_lib(buffer, &buffer_length, buffer_max_length, &row_number, inode_list, &inode_count, lib, platform, origin);
    if (rval != PGFINDLIB_OK) return rval;
  }
#endif

  /* Put together the list of sources and sonames from the FROM and WHERE of the input. */
  /* MAX_TOKENS_COUNT is fixed but more than twice the number of official tokeners */
  struct tokener tokener_list[PGFINDLIB_MAX_TOKENS_COUNT];
printf("before tokenize\n");
  {

    rval= pgfindlib_tokenize(statement, tokener_list, &row_number, buffer, &buffer_length, buffer_max_length);
    if (rval != PGFINDLIB_OK) return rval;
  }
printf("after tokenize\n");
  unsigned int rpath_or_runpath_count= 0;
  for (int i= 0; tokener_list[i].tokener_comment_id != PGFINDLIB_TOKEN_END; ++i)
  {
    if ((tokener_list[i].tokener_comment_id == PGFINDLIB_TOKEN_SOURCE_DT_RPATH)
     || (tokener_list[i].tokener_comment_id == PGFINDLIB_TOKEN_SOURCE_DT_RUNPATH))
      ++rpath_or_runpath_count;
  }

/* Preparation if DT_RPATH or DT_RUNPATH */
#if (PGFINDLIB_TOKEN_SOURCE_DT_RPATH_OR_DT_RUNPATH != 0)
  const ElfW(Dyn) *dynamic= _DYNAMIC;
  const ElfW(Dyn) *dt_rpath= NULL;
  const ElfW(Dyn) *dt_runpath= NULL;
  const char *dt_strtab= NULL;
  if (rpath_or_runpath_count > 0)
  {
    /* in theory "extern __attribute__((weak)) ... _DYNAMIC[];" could result in _DYNAMIC == NULL */
    if (_DYNAMIC == NULL)
    {
#if (PGFINDLIB_COMMENT_CANNOT_READ_RPATH != 0)
      rval= pgfindlib_comment(buffer, &buffer_length, "Cannot read DT_RPATH because _DYNAMIC is NULL", buffer_max_length, PGFINDLIB_COMMENT_CANNOT_READ_RPATH);
      if (rval != PGFINDLIB_OK) return rval;
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
    }
  }
  /* So now we have dt_rpath and dt_runpath */
#endif

  /* Go through the list of sources and add to the lists: source# length pointer-to-path */

  char* malloc_buffer_1; unsigned int malloc_buffer_1_length; unsigned int malloc_buffer_1_max_length;
  char** malloc_buffer_2; unsigned int malloc_buffer_2_length; unsigned int malloc_buffer_2_max_length;

  malloc_buffer_1_max_length= 1000;
  malloc_buffer_2_max_length= 4;

repeat_malloc:
  malloc_buffer_1_length= 0;
  malloc_buffer_1= malloc(malloc_buffer_1_max_length);
  if (malloc_buffer_1 == NULL)
  {
    /* TODO: Hmm, not much I can do, eh? */
    printf("**** MALLOC FAILURE\n");
    return PGFINDLIB_MALLOC_BUFFER_1_OVERFLOW;
  }
  malloc_buffer_2_length= 0;
  malloc_buffer_2= malloc(malloc_buffer_2_max_length * sizeof(char*));
  if (malloc_buffer_2 == NULL)
  {
    /* TODO: Hmm, not much I can do, eh? */
    printf("**** MALLOC FAILURE\n");
    return PGFINDLIB_MALLOC_BUFFER_2_OVERFLOW;
  }

  for (unsigned int tokener_number= 0; ; ++tokener_number) /* for each source in source name list */
  {
    int comment_number;
    comment_number= tokener_list[tokener_number].tokener_comment_id;

    if (comment_number == PGFINDLIB_TOKEN_END) break;
    if ((comment_number < PGFINDLIB_TOKEN_SOURCE_LD_AUDIT) || (comment_number > PGFINDLIB_TOKEN_SOURCE_NONSTANDARD)) continue;
    
    const char* ld;
    if (comment_number == PGFINDLIB_TOKEN_SOURCE_DT_RPATH)
    {
      if ((dt_strtab == NULL) || (dt_rpath == NULL)) continue;
      ld= dt_strtab + dt_rpath->d_un.d_val;
    }
    else if (comment_number == PGFINDLIB_TOKEN_SOURCE_DT_RUNPATH)
    {
      if ((dt_strtab == NULL) || (dt_runpath == NULL)) continue;
      ld= dt_strtab + dt_runpath->d_un.d_val;
    }
    else if (comment_number == PGFINDLIB_TOKEN_SOURCE_DEFAULT_PATHS)
    {
      ld= "/lib:/lib64:/usr/lib:/usr/lib64"; /* "lib64" is platform-dependent but dunno which platform */
    }
    else if (comment_number == PGFINDLIB_TOKEN_SOURCE_EXTRA_PATHS)
    {
      ; /* ld= extra_paths; */ /* huh? why not getenv? */ /* todo: use what was in CMakeLists.txt */
    }
    else if (comment_number != PGFINDLIB_TOKEN_SOURCE_LD_SO_CACHE)
    {
      char source_name[PGFINDLIB_MAX_TOKEN_LENGTH + 1];
      int source_name_length= tokener_list[tokener_number].tokener_length;
      memcpy(source_name, tokener_list[tokener_number].tokener_name, source_name_length);
      source_name[source_name_length]= '\0';
      ld= getenv(source_name); /* todo: they're not all environment variables! */
/* todo: what if ld is null or blank? */
    }
    if (comment_number == PGFINDLIB_TOKEN_SOURCE_LD_SO_CACHE)
    {
      rval= pgfindlib_so_cache(tokener_list, tokener_number,
                               malloc_buffer_1, &malloc_buffer_1_length, malloc_buffer_1_max_length,
                               malloc_buffer_2, &malloc_buffer_2_length, malloc_buffer_2_max_length);
    }
    else
    {
      rval= pgfindlib_source_scan(ld, buffer, &buffer_length, tokener_number,
                                 buffer_max_length, lib, platform, origin,
                                 &row_number,
                                 inode_list, &inode_count, &inode_warning_count,
                                 tokener_list,
                                 malloc_buffer_1, &malloc_buffer_1_length, malloc_buffer_1_max_length,
                                 malloc_buffer_2, &malloc_buffer_2_length, malloc_buffer_2_max_length);
    }

    if (rval == PGFINDLIB_MALLOC_BUFFER_1_OVERFLOW)
    {
      free(malloc_buffer_1); free(malloc_buffer_2);
      malloc_buffer_1_max_length+= 1000;
      goto repeat_malloc;
    }
    if (rval == PGFINDLIB_MALLOC_BUFFER_2_OVERFLOW)
    {
      free(malloc_buffer_1); free(malloc_buffer_2);
      malloc_buffer_2_max_length+= 1000;
      goto repeat_malloc;
    }

    if (rval != PGFINDLIB_OK) return rval;

  }
  /* TODO: free(malloc_buffer_1); */

  qsort(malloc_buffer_2, malloc_buffer_2_length, sizeof(char *), pgfindlib_qsort_compare);

  /* Phase 1 complete. At this point, we seem to have a sorted list of all the paths. */
  /* In Phase 2, we must dump the paths into the output buffer along with the warnings. */

  /* todo: malloc an inode_list about equal to malloc_buffer_2 i.e. # of rows (but more needed for comments I guess) */

  for (unsigned int i= 0; i < malloc_buffer_2_length; ++i)
  {
    const char* item= malloc_buffer_2[i];

    /* First char is source number + 32 */
    int token_number_of_source= *item - 32;
    rval= pgfindlib_file(buffer, &buffer_length, item + 1, buffer_max_length, &row_number,
                       inode_list, &inode_count, &inode_warning_count, tokener_list[token_number_of_source]);
  }

  return PGFINDLIB_OK;
}

int pgfindlib_strcat(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length)
{
  unsigned int line_length= strlen(line);
  const char *pointer_to_line= line;
  while (*pointer_to_line == ' ') {++pointer_to_line; --line_length; } /* skip lead spaces */
  while ((line_length > 0) && (*(pointer_to_line + line_length - 1) == ' ')) --line_length; /* skip trail spaces */
  if (*buffer_length + line_length > buffer_max_length) return PGFINDLIB_ERROR_BUFFER_MAX_LENGTH_TOO_SMALL;
  memcpy(buffer + *buffer_length, pointer_to_line, line_length);
  *buffer_length+= line_length;
  *(buffer + *buffer_length)= '\0';
  return PGFINDLIB_OK;
}

/*
  Compare line to each of the items in statement. If match for number-of-characters-in-soname: 1 true. Else: 0 false.
  Assume that line cannot contain ':' and ':' within statement is a delimiter.
  Ignore lead or trail spaces.
  Treat \n as end of line and ignore it.
  We have already checked that strlen(each soname) is <= PGFINDLIB_MAX_PATH_LENGTH.
  Beware: ldconfig -p lines start with a control character (tab?).
*/
int pgfindlib_find_line_in_statement(const struct tokener tokener_list[], const char *line)
{
  const char *pointer_to_line= line;
  while (*pointer_to_line <= ' ') ++pointer_to_line; /* skip lead spaces (or control characters!) in line */
  unsigned int line_length= strlen(pointer_to_line);
  if ((line_length > 1) && (pointer_to_line[line_length - 1] == '\n')) --line_length; /* skip trail \n */
  while ((line_length > 0) && (pointer_to_line[line_length - 1] == ' ')) --line_length; /* skip trail spaces */
  if (line_length == 0) return 0; /* false */
  for (unsigned int tokener_number= 0; ; ++tokener_number) /* for each source in source name list */
  {
    int comment_number;
    comment_number= tokener_list[tokener_number].tokener_comment_id;
    if (comment_number == PGFINDLIB_TOKEN_END) break;
    if (comment_number != PGFINDLIB_TOKEN_FILE) continue;
    const char* pointer_to_statement= tokener_list[tokener_number].tokener_name;
    unsigned soname_length= tokener_list[tokener_number].tokener_length;
    if (memcmp(pointer_to_line, pointer_to_statement, soname_length) == 0)
    {
      return 1; /* true */
    }
  }
  return 0; /* false (though actually we shouldn't get this far) */
}

/*
  Pass: library or file name
  Do: replace with same library or file name, except that $ORIGIN or $LIB or $PLATFORM is replaced
  Return: rval
  Todo: It seems loader replaces $LIB or $LIB/ but not $LIB in $LIBxxx, this replaces $LIB in $LIBxxx too.
  Todo: https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=187114 suggests no need to check DF_ORIGIN flag nowadays
        but that hasn't been tested
*/
int pgfindlib_replace_lib_or_platform_or_origin(char* one_library_or_file, unsigned int *replacements_count, const char *lib, const char *platform, const char *origin)
{
  *replacements_count= 0;
  if (strchr(one_library_or_file, '$') == NULL) return PGFINDLIB_OK;
  char buffer_for_output[PGFINDLIB_MAX_PATH_LENGTH*2 + 1];
  char *p_line_out= &buffer_for_output[0];
  *p_line_out= '\0';
  for (const char *p_line_in= one_library_or_file; *p_line_in != '\0';)
  {
    if ((strncmp(p_line_in, "$ORIGIN", 7) == 0) || (strncmp(p_line_in, "${ORIGIN}", 9) == 0))
    {
      strcpy(p_line_out, origin);
      p_line_out+= strlen(origin);
      if (*(p_line_in + 1) == '{') p_line_in+= 9 - 7;
      p_line_in+= 7;
      ++*replacements_count;
    }
    else if ((strncmp(p_line_in, "$LIB", 4) == 0) || (strncmp(p_line_in, "${LIB}", 6) == 0))
    {
      strcpy(p_line_out, lib);
      p_line_out+= strlen(lib);
      if (*(p_line_in + 1) == '{') p_line_in+= 6 - 4;
      p_line_in+= 4;
      ++*replacements_count;
    }
    else if ((strncmp(p_line_in, "$PLATFORM", 9) == 0) || (strncmp(p_line_in, "${PLATFORM}", 11) == 0))
    {
      strcpy(p_line_out, platform);
      p_line_out+= strlen(platform);
      if (*(p_line_in + 1) == '{') p_line_in+= 11 - 9;
      p_line_in+= 9;
      ++*replacements_count;
    }
    else
    {
      *p_line_out= *p_line_in;
      ++p_line_out;
      ++p_line_in;
    }
    if ((p_line_out - &buffer_for_output[0]) > PGFINDLIB_MAX_PATH_LENGTH)
      return PGFINDLIB_ERROR_MAX_PATH_LENGTH_TOO_SMALL;
    *p_line_out= '\0';
  }
  strcpy(one_library_or_file, buffer_for_output);
  return PGFINDLIB_OK;
}

/*
  Return: what would loader would change "$LIB" or "$PLATFORM" or "$ORIGIN" to
  Re method: Use a dummy utility, preferably bin/true, ls should also work but isn't as good
            (any program anywhere will provided it requires any .so, and libc.so is such).
            First attempt: read ELF to get "ELF interpreter" i.e. loader name,
            which probably is /lib64/ld-linux-x86-64.so.2 or /lib/ld-linux.so.2,
            then use it with LD_DEBUG to see search_path when executing the dummy utility.
            If that fails: same idea but just running the dummy.
            If that fails: (lib) lib64 for 64-bit, lib for 32-bit. (platform) uname -m.
            FreeBSD is different.
  Todo: get PT_DYNAMIC the way we get PT_INTERN, instead of depending on extern _DYNAMIC
  Todo: If there is a /tls (thread local storage) subdirectory or an x86_64 subdirectory,
        search all. But show library search path=main, main-tls, main/x86_64
  Todo: If dynamic loader is not the usual e.g. due to "-Wl,-I/tmp/my_ld.so" then add a comment.
*/
int pgfindlib_get_origin_and_lib_and_platform(char* origin, char* lib, char* platform,
                                              char* buffer, unsigned int* buffer_length, unsigned int buffer_max_length)
{
  int platform_change_count= 0;
  int rval= PGFINDLIB_OK;

#ifdef PGFINDLIB_FREEBSD
  int aux_info_return= elf_aux_info(AT_EXECPATH, origin, PGFINDLIB_MAX_PATH_LENGTH);
  if (aux_info_return != 0)
  {
#if (PGFINDLIB_COMMENT_ELF_AUX_INFO_FAILED != 0)
    rval= pgfindlib_comment(buffer, buffer_length, "elf_aux_info failed so $ORIGIN is unknown", buffer_max_length, PGFINDLIB_COMMENT_ELF_AUX_INFO_FAILED);
    if (rval != PGFINDLIB_OK) return rval;
#endif
    strcpy(origin, "");
  }
  strcpy(lib, "lib");
  /* platform should be set after the if/else/endif */
#else /* the endif for this else is just before "if (platform_change_count == 0)" */
  int lib_change_count= 0;
  {
    int readlink_return;
    readlink_return= readlink("/proc/self/exe", origin, PGFINDLIB_MAX_PATH_LENGTH);
    if ((readlink_return < 0) || (readlink_return >= PGFINDLIB_MAX_PATH_LENGTH))
    {
#if (PGFINDLIB_COMMENT_READLINK_FAILED != 0)
      rval= pgfindlib_comment(buffer, buffer_length, "readlink failed so $ORIGIN is unknown", buffer_max_length, PGFINDLIB_COMMENT_READLINK_FAILED);
      if (rval != PGFINDLIB_OK) return rval;
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

#if (PGFINDLIB_IF_GET_LIB_OR_PLATFORM != 0)
  /* utility name */
  char utility_name[256]= "";
  if (access("/bin/true", X_OK) == 0) strcpy(utility_name, "/bin/true");
  else if (access("/bin/cp", X_OK) == 0) strcpy(utility_name, "/bin/cp");
  else if (access("/usr/bin/true", X_OK) == 0) strcpy(utility_name, "/usr/bin/true");
  else if (access("/usr/bin/cp", X_OK) == 0) strcpy(utility_name, "/usr/bin/cp");
#if (PGFINDLIB_COMMENT_NO_TRUE_OR_CP != 0)
  if (strcmp(utility_name, "") == 0)
  {
    rval= pgfindlib_comment(buffer, buffer_length, "no access to [/usr]/bin/true or [/usr]/cp", buffer_max_length, PGFINDLIB_COMMENT_NO_TRUE_OR_CP);
    if (rval != PGFINDLIB_OK) return rval;
  }
#endif

/* todo: move this up, and if it's zilch then we cannot red elf here */
extern void* __executable_start;
  ElfW(Ehdr)*ehdr= NULL;
  if (__executable_start != NULL)
  {
    ehdr= (ElfW(Ehdr)*) &__executable_start; /* linker e.g. ld.bfd or ld.lld is supposed to add this */
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
#if (PGFINDLIB_COMMENT_EHDR_IDENT != 0)
      rval= pgfindlib_comment(buffer, buffer_length, "ehdr->ident not valid", buffer_max_length, PGFINDLIB_COMMENT_EHDR_IDENT);
      if (rval != PGFINDLIB_OK) return rval;
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
#if (PGFINDLIB_COMMENT_CANT_FIND_DYNAMIC_LOADER != 0)
    char comment[512];
    sprintf(comment, "can't get ehdr dynamic loader so assume %s", dynamic_loader_name);
    rval= pgfindlib_comment(buffer, buffer_length, comment, buffer_max_length, PGFINDLIB_COMMENT_CANT_FIND_DYNAMIC_LOADER);
    if (rval != PGFINDLIB_OK) return rval;
#endif
  }

  if (access(dynamic_loader_name, X_OK) != 0)
  {
#if (PGFINDLIB_COMMENT_CANT_ACCESS_DYNAMIC_LOADER != 0)
    char comment[512];
    sprintf(comment, "can't access %s", dynamic_loader_name);
    rval= pgfindlib_comment(buffer, buffer_length, comment, buffer_max_length, PGFINDLIB_COMMENT_CANT_ACCESS_DYNAMIC_LOADER);
    if (rval != PGFINDLIB_OK) return rval;
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
      char popen_arg[PGFINDLIB_MAX_PATH_LENGTH + 1];
      sprintf(popen_arg,
      "env -u LD_DEBUG_OUTPUT LD_LIBRARY_PATH='/PRE_OOKPIK/%s/POST_OOKPIK' LD_DEBUG=libs %s --inhibit-cache %s 2>/dev/stdout",
      replacee, dynamic_loader_name, utility_name);
      fp= popen(popen_arg, "r");
      if (fp != NULL)
      {
        char buffer_for_ookpik[PGFINDLIB_MAX_PATH_LENGTH + 1];
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
        pclose(fp);
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
          char buffer_for_ookpik[PGFINDLIB_MAX_PATH_LENGTH + 1];
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
          pclose(fp);
        }
      }
    }
  }
#endif
  if (lib_change_count == 0)
  {
    if ((sizeof(void*)) == 8) strcpy(lib, "lib64"); /* default $LIB if pgfindlib__get_lib_or_platform doesn't work */
    else strcpy(lib, "lib");
#if (PGFINDLIB_COMMENT_ASSUMING_LIB != 0)
    char comment[512];
    sprintf(comment, "assuming $LIB is %s", lib);
    rval= pgfindlib_comment(buffer, buffer_length, comment, buffer_max_length, PGFINDLIB_COMMENT_ASSUMING_LIB);
    if (rval != PGFINDLIB_OK) return rval;
#endif
  }
#endif /* ifdef PGFINDLIB_FREEBSD ... else ... */

  if (platform_change_count == 0)
  {
    char buffer_for_replacement[PGFINDLIB_MAX_PATH_LENGTH + 1]= "?";
    FILE *fp= popen("LD_LIBRARY_PATH= LD_DEBUG= LD_PRELOAD= uname -m 2>/dev/null", "r");
    if (fp != NULL)
    {
      if (fgets(buffer_for_replacement, sizeof(buffer_for_replacement), fp) == NULL)
        ;
      pclose(fp);
    }
#if (PGFINDLIB_COMMENT_UNAME_FAILED != 0)
    if (strcmp(buffer_for_replacement, "?") == 0)
    {
      rval= pgfindlib_comment(buffer, buffer_length, "uname -m failed", buffer_max_length, PGFINDLIB_COMMENT_UNAME_FAILED);
      if (rval != PGFINDLIB_OK) return rval;
    }
#endif
    char* pointer_to_n= strchr(buffer_for_replacement, '\n');
    if (pointer_to_n != NULL) *pointer_to_n= '\0';
    strcpy(platform, buffer_for_replacement);
#if (PGFINDLIB_COMMENT_ASSUMING_PLATFORM != 0)
    char comment[512];
    sprintf(comment, "assuming $PLATFORM is %s", platform);
    rval= pgfindlib_comment(buffer, buffer_length, comment, buffer_max_length, PGFINDLIB_COMMENT_ASSUMING_PLATFORM);
    if (rval != PGFINDLIB_OK) return rval;
#endif
  }
  return rval; /* which is doubtless PGFINDLIB_OK */
}

/*
  Put comment in buffer.
  All warnings and comments must have form: slash+asterisk+space 3-hex-digit-number text \n space+asterisk-slash \n
  The number should never change, the text should rarely change.
  Todo: make this obsolete
*/
static int pgfindlib_comment(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length,
                             const unsigned int comment_number)
{
  char comment[PGFINDLIB_MAX_PATH_LENGTH];
  int rval;
  sprintf(comment, "/* %03x %s */\n", comment_number, line);
  rval= pgfindlib_strcat(buffer, buffer_length, comment, buffer_max_length);
  return rval;
}

/*
 *For a comment column
  Put column of row in buffer.
  All warnings and comments must have form: 3-decimal-digit-number text delimiter
  The number should never change, the text should rarely change.
  Todo: flag to ignore column because column_number
        flag to exclude text and only show number
        limit text length
        enclose in ''s or ""s if contains delimiter
*/
int pgfindlib_comment_in_row(char* comment, unsigned int comment_number, int additional_number)
{
  const char* text;
  if (comment_number == PGFINDLIB_COMMENT_NEXT_DUPLICATE) sprintf(comment, "%03d %s %d", comment_number, "duplicate of", additional_number);
  else
  {
    if (comment_number == PGFINDLIB_COMMENT_ACCESS_NEXT_FAILED) text= "access(filename, R_OK) failed";
    if (comment_number == PGFINDLIB_COMMENT_STAT_NEXT_FAILED) text= "stat(filename) failed";
    if (comment_number == PGFINDLIB_COMMENT_SYMLINK) text= "symlink";
    if (comment_number == PGFINDLIB_COMMENT_MAX_INODE_COUNT_TOO_SMALL) text= "MAX_INODE_COUNT is too small";    
    sprintf(comment, "%03d %s", comment_number, text);
  }
  return PGFINDLIB_OK;
}


/*
  We have this leftover, I'm not sure what it solved.
              {
                rval= pgfindlib_file(buffer, buffer_length, one_library_or_file, buffer_max_length, row_number,
                               inode_list, inode_count, inode_warning_count, comment_number);
                if (rval != PGFINDLIB_OK) return rval;
                unsigned int len= strlen(one_library_or_file);
                if (len == 0) {;}
                else
                {
                  if (one_library_or_file[len - 1] != '/')
                  {
                    rval= pgfindlib_strcat(buffer, buffer_length, "/", buffer_max_length);
                    if (rval != PGFINDLIB_OK) return rval;
                  }
                }
              }
*/

/*
  called "bottom level" because ultimately all pgfindlib_row_* functions should call here
  todo: move overflow check to here
*/
int pgfindlib_row_bottom_level(char *buffer, unsigned int *buffer_length, unsigned int buffer_max_length, unsigned int* row_number, 
                               const char* columns_list[])
{
  int rval;
  char row_number_string[8];
  unsigned int buffer_length_save= *buffer_length;
  for (int i= 0; i < MAX_COLUMNS_PER_ROW; ++i)
  {
    if (i == COLUMN_FOR_ROW_NUMBER)
    {
      sprintf(row_number_string, "%d", *row_number);
      rval= pgfindlib_strcat(buffer, buffer_length, row_number_string, buffer_max_length);
    }
    else rval= pgfindlib_strcat(buffer, buffer_length, columns_list[i], buffer_max_length);
    if (rval != PGFINDLIB_OK) goto overflow;
    rval= pgfindlib_strcat(buffer, buffer_length, PGFINDLIB_COLUMN_DELIMITER, buffer_max_length); /* ":" */
    if (rval != PGFINDLIB_OK) goto overflow;
  }
  rval= pgfindlib_strcat(buffer, buffer_length, PGFINDLIB_ROW_DELIMITER, buffer_max_length); /* "\n" */
  if (rval != PGFINDLIB_OK) goto overflow;
  ++*row_number;
  return rval;
overflow:
/* todo: this should be a row and there should be a guarantee that it will fit i.e. the regular strcat check is buffer_length - what's needed for overflow message */
  *buffer_length= buffer_length_save;
  pgfindlib_strcat(buffer, buffer_length, "OVFLW", buffer_max_length);
  return rval; /* i.e. return the rval that caused overflow */
}

static int pgfindlib_row_version(char *buffer, unsigned int* buffer_length, unsigned buffer_max_length, unsigned int* row_number, ino_t inode_list[], unsigned int* inode_count)
{
  if (*inode_count != PGFINDLIB_MAX_INODE_COUNT) { inode_list[*inode_count]= -1; ++*inode_count; }
  char row_version[32];
  sprintf(row_version, "version %d.%d.%d", PGFINDLIB_VERSION_MAJOR, PGFINDLIB_VERSION_MINOR, PGFINDLIB_VERSION_PATCH);
  const char* columns_list[MAX_COLUMNS_PER_ROW];
  for (int i= 0; i < MAX_COLUMNS_PER_ROW; ++i) columns_list[i]= "";
  columns_list[COLUMN_FOR_COMMENT_1]= "pgfindlib";
  columns_list[COLUMN_FOR_COMMENT_2]= row_version;
  return pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
}


int pgfindlib_row_lib(char* buffer, unsigned int* buffer_length, unsigned buffer_max_length, unsigned int* row_number,
                      ino_t inode_list[], unsigned int* inode_count,
                      const char* lib, const char* platform, const char* origin)
{
  if (*inode_count != PGFINDLIB_MAX_INODE_COUNT) { inode_list[*inode_count]= -1; ++*inode_count; }
  char column_lib[PGFINDLIB_MAX_PATH_LENGTH + 1];
  char column_platform[PGFINDLIB_MAX_PATH_LENGTH + 1];
  char column_origin[PGFINDLIB_MAX_PATH_LENGTH + 1];
  sprintf(column_lib, "%03d $LIB=%s", PGFINDLIB_COMMENT_LIB_STRING, lib);
  sprintf(column_platform, "%03d $PLATFORM=%s", PGFINDLIB_COMMENT_PLATFORM_STRING, platform);
  sprintf(column_origin, "%03d $ORIGIN=%s", PGFINDLIB_COMMENT_ORIGIN_STRING, origin);
  const char* columns_list[MAX_COLUMNS_PER_ROW];
  for (int i= 0; i < MAX_COLUMNS_PER_ROW; ++i) columns_list[i]= "";
  columns_list[COLUMN_FOR_COMMENT_1]= column_lib;
  columns_list[COLUMN_FOR_COMMENT_2]= column_platform;
  columns_list[COLUMN_FOR_COMMENT_3]= column_origin;
  return pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
}

/* Todo: usually this should be disabled */
int pgfindlib_row_source_name(char* buffer, unsigned int* buffer_length, unsigned buffer_max_length, unsigned int* row_number,
                             ino_t inode_list[], unsigned int* inode_count,
                             const char* source_name)
{
  if (*inode_count != PGFINDLIB_MAX_INODE_COUNT) { inode_list[*inode_count]= -1; ++*inode_count; }
  const char* columns_list[MAX_COLUMNS_PER_ROW];
  for (int i= 0; i < MAX_COLUMNS_PER_ROW; ++i) columns_list[i]= "";
  columns_list[COLUMN_FOR_SOURCE]= source_name;
  return pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
}


/*
  Put file name in buffer. Precede with comments if there are any.
  todo: pgfindlib_comment = source name if first in source and not done before
  todo: some parameters unused e.g. inode_warning_count
*/
int pgfindlib_file(char *buffer, unsigned int *buffer_length, const char *line, unsigned int buffer_max_length,
                          unsigned int* row_number,
                          ino_t inode_list[], unsigned int* inode_count, unsigned int* inode_warning_count,
                          struct tokener tokener_list_item)
{
  char line_copy[PGFINDLIB_MAX_PATH_LENGTH + 1]; /* todo: change following to "get rid of trailing \n" */
  {
    int i= 0; int j= 0;
    for (;;)
    {
      if (*(line + i) != '\n') { *(line_copy + j)= *(line + i); ++j; }
      if (*(line + i) == '\0') break;
      ++i;
    }
  }
/* https://android.googlesource.com/kernel/lk/+/dima/for-travis/include/errno.h */

  const char* columns_list[MAX_COLUMNS_PER_ROW];
  for (int i= 0; i < MAX_COLUMNS_PER_ROW; ++i) columns_list[i]= "";
  unsigned int columns_list_number= COLUMN_FOR_COMMENT_1;
  
  char warning_max_inode_count_is_too_small[64]= "";
  char warning_stat_next_failed[64]= "";
  char warning_access_next_failed[64]= "";
  char warning_symlink[64]= "";
  char warning_duplicate[64]= "";
  if (access(line_copy, R_OK) != 0) /* It's poorly documented but tests indicate X_OK doesn't matter and R_OK matters */
  {
    pgfindlib_comment_in_row(warning_access_next_failed, PGFINDLIB_COMMENT_ACCESS_NEXT_FAILED, 0);
    columns_list[columns_list_number++]= warning_access_next_failed;  
  }
  ino_t inode;
  struct stat sb;
  if (stat(line_copy, &sb) == -1)
  {
    pgfindlib_comment_in_row(warning_stat_next_failed, PGFINDLIB_COMMENT_STAT_NEXT_FAILED, 9);
    columns_list[columns_list_number++]= warning_stat_next_failed;
    inode= -1; /* a dummy so count of inodes is right but this won't be found later */
  }
  else
  {
    mode_t st_mode= sb.st_mode & S_IFMT;
    if ((st_mode !=S_IFREG) && (st_mode != S_IFLNK)) return PGFINDLIB_OK; /* not file or symlink so not candidate */
    /* Here, if (st_mode == S_IFLNK) and include_symlinks is off, return */
    /* Here, if (duplicate) and include duplicates is off, return */
    if (st_mode == S_IFLNK)
    {
      /* todo: find out: symlink of what? */
      pgfindlib_comment_in_row(warning_symlink, PGFINDLIB_COMMENT_SYMLINK, 0);
      columns_list[columns_list_number++]= warning_symlink;
    }
    inode= sb.st_ino;
    for (int i= 0; i < *inode_count; ++i)
    {
      if (inode_list[i] == inode)
      {
       pgfindlib_comment_in_row(warning_duplicate, PGFINDLIB_COMMENT_NEXT_DUPLICATE, i);
        columns_list[columns_list_number++]= warning_duplicate;
        break;
      }
    }
  }
  if (*inode_count == PGFINDLIB_MAX_INODE_COUNT)
  {
    pgfindlib_comment_in_row(warning_max_inode_count_is_too_small, PGFINDLIB_COMMENT_MAX_INODE_COUNT_TOO_SMALL, 0);
    columns_list[columns_list_number++]= warning_symlink;
  }
  else
  {
    inode_list[*inode_count]= inode;
    ++*inode_count;  
  }
  char comment_string[256]; /* todo: check: too small */ /* "LD_AUDIT" "LD_PRELOAD" etc. */
  memcpy(comment_string, tokener_list_item.tokener_name, tokener_list_item.tokener_length);
  comment_string[tokener_list_item.tokener_length]= '\0';

  columns_list[COLUMN_FOR_PATH]= line_copy;
  columns_list[COLUMN_FOR_SOURCE]= comment_string;

    /* more columns! */

  return pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);

}

/*
  Tokenize comments
  [SELECT *]                   Possible future addition
  [FROM source [, source ...]  There's a default if it's missing
  [WHERE file [, file ...]     There's a default if it's missing
  [ORDER BY id]                Possible future addition
  [LIMIT n]                    Possible future addition
  Keywords can be any case, source and file are case sensitive.
  Source and file can be enclosed in either ""s or ''s or ::s, which are stripped.
    (Although ' and " look more natural, running main 'a b' will fail because argv strips them already,
    so only : is reliable, say main ':a b:')
  Source can be: LD_LIBRARY_PATH etc. but also: {file|dir|env}.source
  Spaces are ignored, except that a keyword must be followed by at least one space
  Only possible end-of-token marks are space and comma and \0
  Possible errors:
    ' or " without ending ' or "
    , without following source name or file name
    source name or file name is too long (max = 255)
  End with _END
    const char* tokener_name;
    char tokener_length;
    char tokener_comment_id;
};
*/
/* Compare for equality with an ascii keyword already lower case,  */
int pgfindlib_keycmp(const unsigned char *a, unsigned int a_len, const unsigned char *b)
{
  if (a_len != strlen(b)) return -1;
  for (int i= 0; i < a_len; ++i)
  {
    if ((*a != *b) && (*a != *b - 32)) return -1;
    ++a; ++b;
  }
  return 0;
}
int pgfindlib_tokenize(const char* statement, struct tokener tokener_list[],
                       unsigned int* row_number,
                       char* buffer, unsigned int* buffer_length, unsigned buffer_max_length)
{
  const char* p= statement;
  const char* p_next;
  unsigned int token_number= 0;

  const char* columns_list[MAX_COLUMNS_PER_ROW];  
  for (int i= 0; i < MAX_COLUMNS_PER_ROW; ++i) columns_list[i]= "";
  if (p == NULL)     /* hmm, but we already check for this don't we? */
  {
    columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
    pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
    return PGFINDLIB_STATEMENT_SYNTAX;
  }
  const char* statement_end= p + strlen(p);
  for (;;)
  {
    if (p > statement_end)
    {
      columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
      pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
      return PGFINDLIB_STATEMENT_SYNTAX;
    }
    if (*p == ' ') { ++p; continue; }
    else if (*p == '\0') break;
    else if ((*p == '"') || (*p == 0x27) || (*p == ':')) /* if " or ' then skip to next " or ' */
    {
      p_next= strchr(p + 1, *p);
      if (p_next == NULL)
      {
        columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
        pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
        return PGFINDLIB_STATEMENT_SYNTAX;
      }
    }
    else if (*p == ',')
      p_next= p + 1;
    else
    {
      p_next= p + 1;
      for (;;)
      {
        if ((*p_next == ' ') || (*p_next == ',') || (*p_next == '\0')) break;
        ++p_next;
      }
    }
    tokener_list[token_number].tokener_name= p;
    tokener_list[token_number].tokener_length= p_next - p;
    if (tokener_list[token_number].tokener_length >= PGFINDLIB_MAX_TOKEN_LENGTH)
    {
      columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
      pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
      return PGFINDLIB_STATEMENT_SYNTAX;
    }
    ++token_number;
    if (token_number >= PGFINDLIB_MAX_TOKENS_COUNT)
    {
      columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
      pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
      return PGFINDLIB_STATEMENT_SYNTAX;
    }
    if (*p_next == '\0') break;
    if (*p_next == ',')
    {
      tokener_list[token_number].tokener_name= p_next;
      tokener_list[token_number].tokener_length= 1;
      ++token_number;
      if (token_number >= PGFINDLIB_MAX_TOKENS_COUNT)
      {
        columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
        pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
        return PGFINDLIB_STATEMENT_SYNTAX;
      }
    }
    p= p_next + 1;
  }
  /* tokener_list[token_number].tokener_name= "END!"; */
  /* tokener_list[token_number].tokener_length= 4; */
  tokener_list[token_number].tokener_comment_id= PGFINDLIB_TOKEN_END;
  unsigned int current_clause= 0;
  unsigned int from_count= 0;
  unsigned int where_count= 0; 
  for (int i= 0; tokener_list[i].tokener_comment_id != PGFINDLIB_TOKEN_END; ++i)
  {
    if (pgfindlib_keycmp(tokener_list[i].tokener_name, tokener_list[i].tokener_length, "from") == 0)
    {
      if (current_clause != 0)
      {
        columns_list[COLUMN_FOR_COMMENT_1]= "Syntax";
        pgfindlib_row_bottom_level(buffer, buffer_length, buffer_max_length, row_number, columns_list);
        return PGFINDLIB_STATEMENT_SYNTAX;
      }
      tokener_list[i].tokener_comment_id= PGFINDLIB_TOKEN_FROM;
      current_clause= PGFINDLIB_TOKEN_FROM;
      ++from_count;
    }
    else if (pgfindlib_keycmp(tokener_list[i].tokener_name, tokener_list[i].tokener_length, "where") == 0)
    {
      if ((current_clause != 0) && (current_clause != PGFINDLIB_TOKEN_FROM))
      {
        pgfindlib_comment(buffer, buffer_length, "Syntax", buffer_max_length, PGFINDLIB_STATEMENT_SYNTAX);
        return PGFINDLIB_STATEMENT_SYNTAX;
      }
      tokener_list[i].tokener_comment_id= PGFINDLIB_TOKEN_WHERE;
      current_clause= PGFINDLIB_TOKEN_WHERE;
      ++where_count;
    }
    else if (tokener_list[i].tokener_name[0] == ',')
      tokener_list[i].tokener_comment_id= PGFINDLIB_TOKEN_COMMA;
    else
    {
      if (current_clause == PGFINDLIB_TOKEN_FROM)
      {
        for (int k= 0; ; ++k)
        {
          const char* c= pgfindlib_standard_source_array[k];
          if (strcmp(c, "") == 0) break;
          tokener_list[i].tokener_comment_id= PGFINDLIB_TOKEN_SOURCE_NONSTANDARD;
          if ((strlen(c) == tokener_list[i].tokener_length) && (memcmp(c, tokener_list[i].tokener_name, tokener_list[i].tokener_length) == 0))
          {
            tokener_list[i].tokener_comment_id= pgfindlib_standard_source_array_n[k];
            break;
          }
        }
      }
      else if (current_clause == PGFINDLIB_TOKEN_WHERE)
      {
        tokener_list[i].tokener_comment_id= PGFINDLIB_TOKEN_FILE;
      }
      else /* presumably an error */
        tokener_list[i].tokener_comment_id= PGFINDLIB_TOKEN_UNKNOWN;
    }
  }
  /* If there was no FROM, make a list from standard sources and put it at end. (We can search for FROM after WHERE.) */
  if (from_count == 0)
  {
    /* token_number should still be at end, which will be overwritten */
    for (int i= 0; pgfindlib_standard_source_array_n[i] != 0; ++i)
    {
      if (token_number >= PGFINDLIB_MAX_TOKENS_COUNT - 1)
      {
        pgfindlib_comment(buffer, buffer_length, "Syntax", buffer_max_length, PGFINDLIB_STATEMENT_SYNTAX);
        return PGFINDLIB_STATEMENT_SYNTAX;
      }
      tokener_list[token_number].tokener_name= pgfindlib_standard_source_array[i];
      tokener_list[token_number].tokener_length= strlen(pgfindlib_standard_source_array[i]);
      tokener_list[token_number].tokener_comment_id= pgfindlib_standard_source_array_n[i];
      ++token_number;
    }
    tokener_list[token_number].tokener_comment_id= PGFINDLIB_TOKEN_END;
  }
  for (int i= 0;; ++i)
  {
    /* char token_x[256]; */
    if (tokener_list[i].tokener_comment_id == PGFINDLIB_TOKEN_END) break;
  }
  return PGFINDLIB_OK;
}

#if (PGFINDLIB_TOKEN_SOURCE_LD_SO_CACHE != 0)
int pgfindlib_so_cache(const struct tokener tokener_list[], int tokener_number,
                       char* malloc_buffer_1, unsigned int* malloc_buffer_1_length, unsigned malloc_buffer_1_max_length,
                       char** malloc_buffer_2, unsigned int* malloc_buffer_2_length, unsigned malloc_buffer_2_max_length)
{
  int rval= PGFINDLIB_OK;
#ifdef OBSOLETE
  rval= pgfindlib_row_source_name(buffer, &buffer_length, buffer_max_length, &row_number, inode_list, &inode_count, "ld.so.cache");
  if (rval != PGFINDLIB_OK) return rval;
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
  {
    /* Assume library is whatever's between first and last / on a line */
    /* Line 1 is probably "[n] libs found in cache `/etc/ld.so.cache'" */
    /* First ldconfig attempt will be Linux-style ldconfig -p, second will be FreeBSD-style ldconfig -r */
    int counter= 0;
    for (int ldconfig_attempts= 0; ldconfig_attempts < 2; ++ldconfig_attempts)
    {
      char ld_so_cache_line[PGFINDLIB_MAX_PATH_LENGTH * 5];
      char popen_arg[PGFINDLIB_MAX_PATH_LENGTH];
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
          if (pgfindlib_find_line_in_statement(tokener_list, pointer_to_ld_so_cache_line) == 0) continue;/* doesn't match requirement */
          char *address= strchr(ld_so_cache_line,'/');
          if (address != NULL)
          {
            rval= pgfindlib_add_to_malloc_buffers(address, tokener_number,
                                                  malloc_buffer_1, malloc_buffer_1_length, malloc_buffer_1_max_length,
                                                  malloc_buffer_2, malloc_buffer_2_length, malloc_buffer_2_max_length);
            if (rval != PGFINDLIB_OK) return rval;
            if (rval != PGFINDLIB_OK) return rval;
          }
        }
        pclose(fp);
      }
      if (counter > 0) break; /* So if ldconfig -p succeeds, don't try ldconfig -r */
    }
#ifdef OBSOLETE
#if (PGFINDLIB_COMMENT_LDCONFIG_FAILED != 0)
    if (counter == 0)
    {
      char comment[256];
      sprintf(comment, "%s -p or %s -r failed", ldconfig, ldconfig); 
      rval= pgfindlib_comment(buffer, &buffer_length, comment, buffer_max_length, PGFINDLIB_COMMENT_LDCONFIG_FAILED);
      if (rval != PGFINDLIB_OK) return rval;
    }
#endif
#endif
  }
  return PGFINDLIB_OK;
}
#endif /* #if (PGFINDLIB_TOKEN_SOURCE_LD_SO_CACHE != 0) */

/*
  Todo: return rval = PGFINDLIB_MALLOC_BUFFER_1_OVERFLOW if it won't fit, and this should percolate upward to force a new malloc.
  NB: malloc_buffer_2 is char** so length is #-of-items rather than #-of-chars
  At the front we add a char = source_number + 32 (because assumption is we won't have 255 - 32 sources)
*/
int pgfindlib_add_to_malloc_buffers(const char* new_item, int source_number,
                                    char* malloc_buffer_1, unsigned int* malloc_buffer_1_length, unsigned malloc_buffer_1_max_length,
                                    char** malloc_buffer_2, unsigned int* malloc_buffer_2_length, unsigned malloc_buffer_2_max_length)
{
  unsigned int strlen_new_item= strlen(new_item) + 2; /* because we'll allocate char-of-source at start and \0 at end */
  if (*malloc_buffer_1_length + strlen_new_item >= malloc_buffer_1_max_length) return PGFINDLIB_MALLOC_BUFFER_1_OVERFLOW;
  char c= source_number + 32;
  *(malloc_buffer_1 + *malloc_buffer_1_length)= c;
  strcpy(malloc_buffer_1 + *malloc_buffer_1_length + 1, new_item);
  if (*malloc_buffer_2_length + 1 >= malloc_buffer_2_max_length) return PGFINDLIB_MALLOC_BUFFER_2_OVERFLOW;
  malloc_buffer_2[*malloc_buffer_2_length]= malloc_buffer_1 + *malloc_buffer_1_length; 
  *malloc_buffer_1_length+= strlen_new_item;
  *malloc_buffer_2_length+= 1;
  return PGFINDLIB_OK;
}

int pgfindlib_qsort_compare(const void *p1, const void *p2)
{
  return strcmp(*(char* const *) p1, *(char* const *) p2);
}


/* Pass: list of libraries separated by colons (or spaces or semicolons depending on char *type)
   I don't check wheher the colon is enclosed within ""s and don't expect a path name to contain a colon.
   If this is called for LD_AUDIT or LD_PRELOAD then librarylist is actually a filelist, which should still be okay.
   We won't get here for ld.so.cache.
*/
int pgfindlib_source_scan(const char *librarylist, char *buffer, unsigned int *buffer_length,
                                unsigned int tokener_number, unsigned int buffer_max_length,
                                const char *lib, const char *platform, const char *origin,
                                unsigned int* row_number,
                                ino_t inode_list[], unsigned int* inode_count, unsigned int* inode_warning_count,
                                struct tokener tokener_list[],
                                char* malloc_buffer_1, unsigned int* malloc_buffer_1_length, unsigned malloc_buffer_1_max_length,
                                char** malloc_buffer_2, unsigned int* malloc_buffer_2_length, unsigned malloc_buffer_2_max_length)
{
  int rval;
  char delimiter1, delimiter2;
  int comment_number= tokener_list[tokener_number].tokener_comment_id;

  if (comment_number == PGFINDLIB_TOKEN_SOURCE_LD_AUDIT)
  { delimiter1= ':'; delimiter2= ':'; } /* colon or colon */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_LD_PRELOAD)
  { delimiter1= ':'; delimiter2= ' '; } /* colon or space */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_DT_RPATH)
  { delimiter1= ':'; delimiter2= ':';  } /* colon or colon */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_LD_LIBRARY_PATH)
  { delimiter1= ':'; delimiter2= ';';  } /* colon or semicolon */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_DT_RUNPATH)
  { delimiter1= ':'; delimiter2= ':';  } /* colon or colon */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_LD_RUN_PATH)
  { delimiter1= ':'; delimiter2= ':';  } /* colon or colon */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_DEFAULT_PATHS)
  { delimiter1= ':'; delimiter2= ';';  } /* colon or semicolon */
  else if (comment_number == PGFINDLIB_TOKEN_SOURCE_EXTRA_PATHS)
  { delimiter1= ':'; delimiter2= ';';  } /* colon or semicolon */
  else /* presumably a user-defined environment variable */
  { printf("??? user-defined?\n");
   delimiter1= ':'; delimiter2= ';';  } /* colon or semicolon */

  {
    char source[PGFINDLIB_MAX_TOKEN_LENGTH + 1];
    int source_length= tokener_list[tokener_number].tokener_length;
    memcpy(source, tokener_list[tokener_number].tokener_name, source_length);
    source[source_length]= '\0';
/* Following should be: if PGFINDLIB_COMMENT_ANY != 0 */
/* #if (PGFINDLIB_TOKEN_SOURCE_LD_LIBRARY_PATH != 0) */
  /* todo: ordinarily we don't want source name */
    rval= pgfindlib_row_source_name(buffer, buffer_length, buffer_max_length, row_number, inode_list, inode_count, source);
    if (rval != PGFINDLIB_OK) return rval;
/* #endif */
  }
  char one_library_or_file[PGFINDLIB_MAX_PATH_LENGTH + 1];

  if (librarylist == NULL) return PGFINDLIB_OK;
  const char *p_in= librarylist;
  for (;;)
  {
    if (*p_in == '\0') break;

    char *pointer_to_one_library_or_file= &one_library_or_file[0];

    /* todo: maybe skip lead/trail spaces is unnecessary? pgrindlib_line_in_statement skips them too */
    for (;;)
    {
      if ((*p_in == delimiter1) || (*p_in == delimiter2) || (*p_in == '\0')) break;
      if ((*p_in != ' ') || (pointer_to_one_library_or_file != &one_library_or_file[0])) /* skip lead space */
      {
        *pointer_to_one_library_or_file= *p_in;
        ++pointer_to_one_library_or_file;
        *pointer_to_one_library_or_file= '\0';
      }
      ++p_in;
    }

    if ((*p_in == delimiter1) || (*p_in == delimiter2)) ++p_in; /* skip delimiter but don't skip \0 */
    for (;;) /* skip trail spaces */
    {
      --pointer_to_one_library_or_file;
      if (pointer_to_one_library_or_file < &one_library_or_file[0]) break;
      if (*pointer_to_one_library_or_file != ' ') break;
      *pointer_to_one_library_or_file= '\0';
    }
    if (strlen(one_library_or_file) > 0) /* If it's a blank we skip it. Is that right? */
    {
      unsigned int replacements_count;
      char orig_one_library_or_file[PGFINDLIB_MAX_PATH_LENGTH + 1];
      strcpy(orig_one_library_or_file, one_library_or_file);
      rval= pgfindlib_replace_lib_or_platform_or_origin(one_library_or_file, &replacements_count, lib, platform, origin);
      if (rval !=PGFINDLIB_OK) return rval;
#if (PGFINDLIB_COMMENT_REPLACE_STRING != 0)
      if (replacements_count > 0)
      {
        char comment[PGFINDLIB_MAX_PATH_LENGTH*2 + 128];
        sprintf(comment, "replaced %s with %s", orig_one_library_or_file, one_library_or_file);
        rval= pgfindlib_comment(buffer, buffer_length, comment, buffer_max_length, PGFINDLIB_COMMENT_REPLACE_STRING);
        if (rval != PGFINDLIB_OK) return rval;
      }
#endif
      /* if LD_AUDIT or LD_PRELOAD we want a file name */
      if ((comment_number == PGFINDLIB_TOKEN_SOURCE_LD_AUDIT) || (comment_number == PGFINDLIB_TOKEN_SOURCE_LD_PRELOAD)) 
      {
        if (pgfindlib_find_line_in_statement(tokener_list, one_library_or_file) == 0) continue; /* doesn't match requirement */
        rval= pgfindlib_file(buffer, buffer_length, one_library_or_file, buffer_max_length, row_number,
                             inode_list, inode_count, inode_warning_count, tokener_list[tokener_number]);
        if (rval != PGFINDLIB_OK) return rval;
        rval= pgfindlib_strcat(buffer, buffer_length, "\n", buffer_max_length);
        if (rval != PGFINDLIB_OK) return rval;
      }
      else
      /* not LD_AUDIT or LD_PRELOAD so it should be a directory name */
      {
        /* char ls_line[PGFINDLIB_MAX_PATH_LENGTH * 5]; */
        DIR* dir= opendir(one_library_or_file);
        if (dir != NULL) /* perhaps would be null if directory not found */
        {
          struct dirent* dirent;
          while ((dirent= readdir(dir)) != NULL)
          {
            if ((dirent->d_type !=  DT_REG) &&  (dirent->d_type !=  DT_LNK)) continue; /* not regular file or symbolic link */
             if (pgfindlib_find_line_in_statement(tokener_list, dirent->d_name) == 0) continue; /* doesn't match requirement */
            {
              char combo[PGFINDLIB_MAX_PATH_LENGTH * 2 + 1];
              strcpy(combo, one_library_or_file);
              strcat(combo, "/");
              strcat(combo, dirent->d_name);
              rval= pgfindlib_add_to_malloc_buffers(combo, tokener_number,
                                                    malloc_buffer_1, malloc_buffer_1_length, malloc_buffer_1_max_length,
                                                    malloc_buffer_2, malloc_buffer_2_length, malloc_buffer_2_max_length);
              if (rval != PGFINDLIB_OK) return rval;
            }
          }
          closedir(dir);
        }
      }
    }
  }
  return PGFINDLIB_OK;
}

