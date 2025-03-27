pgfindlib

<P>Version 0.9.7</P>

<P>The pgfindlib function finds dynamic libraries (.so files)
in the order the loader would find them.</P>

<P>Copyright (c) 2025 by Peter Gulutzan.
All rights reserved.</P>

<H3 id="executive-summary">What pgfindlib is good for</H3><HR>

<P>Knowing what .so files the loader would load,
but loading them yourself with dlopen(),
means you can customize at runtime.</P>

<P>Or, as part of --help you can tell users what the loader
picked up, and from where, and what choices it ignored.</P>

<P>An example: Using files supplied with the package:
<PRE>
mkdir /tmp/pgfindlib_example
echo "Dummy .so" >> /tmp/pgfindlib_example/libutil.so
gcc -o main main.c pgfindlib.c  -Wl,-rpath,/tmp/pgfindlib_example
LD_LIBRARY_PATH='/$LIB' ./main 'where libutil.so, libcurl.so, libgcc_s.so'
</PRE>
The result might look like this:
<PRE>
1,,,002 pgfindlib,001 version 0.9.7,003 https://github.com/pgulutzan/pgfindlib,,
2,,,005 $LIB=lib/x86_64-linux-gnu,006 $PLATFORM=x86_64,007 $ORIGIN=/home/pgulutzan/pgfindlib,,
3,,,012 in source LD_LIBRARY_PATH replaced /$LIB with /lib/x86_64-linux-gnu,,,,
4,/lib/x86_64-linux-gnu/libcurl.so,LD_LIBRARY_PATH,013 symlink,,,,
5,/lib/x86_64-linux-gnu/libcurl.so.4,LD_LIBRARY_PATH,013 symlink,,,,
6,/lib/x86_64-linux-gnu/libcurl.so.4.6.0,LD_LIBRARY_PATH,,,,,
7,/lib/x86_64-linux-gnu/libgcc_s.so.1,LD_LIBRARY_PATH,,,,,
8,/lib/x86_64-linux-gnu/libutil.so,LD_LIBRARY_PATH,013 symlink,,,,
9,/lib/x86_64-linux-gnu/libutil.so.1,LD_LIBRARY_PATH,013 symlink,,,,
10,/tmp/pgfindlib_example/libutil.so,DT_RUNPATH,071 elf read failed,,,,
11,/lib/libgcc_s.so.1,ld.so.cache,,,,,
12,/lib/x86_64-linux-gnu/libcurl.so,ld.so.cache,013 symlink,014 duplicate of 4,,,
13,/lib/x86_64-linux-gnu/libcurl.so.4,ld.so.cache,013 symlink,014 duplicate of 5,,,
14,/lib/x86_64-linux-gnu/libgcc_s.so.1,ld.so.cache,014 duplicate of 7,,,,
15,/lib/x86_64-linux-gnu/libutil.so,ld.so.cache,013 symlink,014 duplicate of 8,,,
16,/lib/x86_64-linux-gnu/libutil.so.1,ld.so.cache,013 symlink,014 duplicate of 9,,,
17,/lib32/libgcc_s.so.1,ld.so.cache,075 elf machine does not match,,,,
18,/lib32/libutil.so.1,ld.so.cache,013 symlink,075 elf machine does not match,,,
19,/lib/libgcc_s.so.1,default_paths,014 duplicate of 11,,,,
20,/usr/lib/libgcc_s.so.1,default_paths,014 duplicate of 11,,,,

rval=0
</PRE>
</P>

<P>This means: the loader would look first in /lib/x86_64-linux-gnu
because of LD_LIBRARY_PATH.
This takes precedence over DT_RUNPATH, which is where the first
occurrence of libutil.so appears (this appears because of the
-rpath option in the gcc command). Finally there are some .so libraries
in ld.so.cache and the system libraries, which is where the loader would go if there was no
prior. But some of the lines contain warnings, for example
"071 elf read failed" because /tmp/pgfindlib_example/libutil.so
is not a loadable file, or for example
"075 elf machine does not match" because main is 64-bit and 
/lib32/libgcc_s.so.1 is 32-bit.
</P>

<P>That's all you need to know in order to decide if you're interested.
If you are, read on, there are many options and a few warnings.</P>

<H3 id="The Files">The Files</H3><HR>

<P>
The supplied files are:<BR>
pgfindlib.c, a C program with a pgfindlib() routine that's ordered to "find my .so" files.<BR>
pgfindlib.h, a small file to include in any program that calls pgfindlib<BR>
main.c, an example program that includes pgfindlib.h and calls the pgfindlib() routine<BR>
pgfindlib_tests.sh, a script that checks the assumptions and claims made about .so searching<BR>
README.md, this file.</P>

<P>There is one callable function in pgfindlib.c, named pgfindlib.
Its arguments are:<BR>
 statement, including 'where' + a comma-delimited list of .so names e.g. where libcrypto.so,libmariadb.so<BR>
 buffer, a space to receive the result<BR>
 buffer_max_length, maximum number of bytes in result including \0 terminator<BR>
 The character set is assumed to be 8-bit including ASCII.
</P>

<P>The return is:<BR>
buffer contents with paths including .so names in the order the loader looks for them,
which is determined by gcc flags and environment variables.
For example:<BR>
20,/usr/lib/libgcc_s.so.1,default_paths,014 duplicate of 11,,,,<BR>
has 7 columns (all rows have 7 columns).
The first column is a row number.
The second column is a path.
The third column is the "source" -- this was in the dynamic loader's default_paths.
The remaining columns are comments including errors or warnings, and are often blank.
</P>

<P>The license is: GPLv2.</P>

<P>The assumed environment is: Linux or FreeBSD containing gcc and the almost-always-present utilities ldconfig, uname.
</P>

<H3 id="Re Errors">Re Errors</H3><HR>
  As well as filling the buffer, pgfindlib returns an error code as defined in pgfindlib.h:
  0 PGFINDLIB_OK no error,
  -1 PGFINDLIB_ERROR_MAX_BUFFER_LENGTH_TOO_SMALL execution stopped after output of nearly buffer_max_length bytes,
  -2 PGFINDLIB_ERROR_NULL e.g. pgfindlib() was called with a null pointer so nothing was done,
  -3 PGFINDLIB_ERROR_MAX_PATH_LENGTH_TOO_SMALL e.g. a path name is 5000 bytes (the default maximum is 4096),
  -4 and -5 PGFINDLIB_MALLOC_BUFFER_x_OVERFLOW because malloc() failed for a few bytes,
  -6 some problem with the statement syntax
  ... In fact anything other than 0 should be extremely rare if what's passed is okay.

<H3 id="Re Filter">Re Filter</H3><HR>
  From the passed sonames, pgfindlib filters the results of readdir() + popen("...ldconfig") from the sources.
  For example, when calling from
  <a href="https://github.com/ocelot-inc/ocelotgui">ocelotgui</a>, we only care about .so libraries that might be needed for
  MariaDB or MySQL or Tarantool, which can include libcrypto.so (we might care for fewer .so libraries if ocelotgui
  is started with appropriate command-line or configuration-file options or a CMakeLists.txt switch.
  So we'd call pgfindlib("where libmysqlclient.so,libmariadb.so,libmariadbclient.so,tarantool.so", ...);

<H3 id="standard">Re standard sources</H3><HR>
<P>
The standard sources are, in order:
LD_AUDIT LD_PRELOAD DT_RPATH LD_LIBRARY_PATH DT_RUNPATH LD_RUN_PATH ld.so.cache default_paths LD_PGFINDLIB_PATH.
These are environment variables and caches that the dynamic loader would look at,
along with some that are just informational.
</p>

<H3 id="Re LD_AUDIT">Re LD_AUDIT</H3><HR>
<P>Actually .so files in the
<a href="https://man7.org/linux/man-pages/man7/rtld-audit.7.html">LD_AUDIT environment variable</a> 
would come first but would be specialized .so files that nobody cares about, thus this category should be empty.
</P>

<H3 id="Re LD_PRELOAD">Re LD_PRELOAD</H3><HR>
<P>Ordinarily this appears first. See https://www.secureideas.com/blog/2020/ldpreload-introduction.html
Path name shouldn't contain spaces, see
https://stackoverflow.com/questions/10072609/how-to-escape-spaces-in-library-path-appended-to-ld-preload
There is no check for ld.so.preload.</P>

<H3 id="Re rpath">Re rpath</H3><HR>
<P>Get DT_RPATH and/or DT_RUNPATH in .dynamic section of the executable file's header.
If header is stripped (unlikely), then the buffer will have nothing in DT_RPATH or DT_RUNPATH.
On test machine it's DT_RPATH iff --disable-new-dtags, this may be default on an old platform.
On test machine it's DT_RUNPATH if --enable-new-dtags which is typically default nowadays..
Some say that loader searches DT_RPATH recursively, but in tests it doesn't happen.</P>

<H3 id="Re LD_LIBRARY_PATH environment variable">Re LD_LIBRARY_PATH environment variable</H3><HR>
<P>This comes after DT_RPATH and before others.</P>

<H3 id="Re LD_RUN_PATH environment variable">Re LD_RUN_PATH environment variable</H3><HR>
<P>At build time, it causes DT_RUNPATH (or DT_RPATH if not enable-new-dtags)
but at run time it does not.
An IBM document says
"LD_RUN_PATH. Specifies the directories that are to be searched for libraries at both link and run time.
The result would be that the /home/me/lib directory is the directory that gets searched at link time and run time."
But on test machine that's not the case.
So assume that: The loader knows nothing about the items in this section, it's informative.</P>

<H3 id="Re ld.so.cache">Re ld.so.cache</H3><HR>
<P>Delivers the result of /sbin/ldconfig -p which by default looks at /etc/ld.so.cache.
If /sbin/ldconfig is not found (very unlikely), it tries again with
/usr/sbin/ldconfig or /bin/ldconfig or /usr/bin/ldconfig or ldconfig. If execution with the -p option
fails, it tries again with the -r option (-p is correct for Linux, -r is possible with FreeBSD).
If everything fails, there will be a comment.</P>

<H3 id="Re default_paths">Re default_paths</H3><HR>
<P>Delivers the matches in /lib or /usr/lib or /lib64 or /usr/lib64,
which are sometimes called "trusted" or "system" files.
As you can see from the example at the start, the function detected that the second system file
had the same inode as the first one which is characteristic of hardlinks.
Showing symlinks and hardlinks is part of the job.
</P>

<H3 id="Re LD_PGFINDLIB_PATH">Re LD_PGFINDLIB_PATH</H3><HR>
<P>This is another colon-or-semicolon-delimited string that can contain more paths that will be added at the end.
For example<BR>
LD_PGFINDLIB_PATH='/lib/x86_64-linux-gnu' ./main 'where libcrypto.so'
<BR>
will include the contents of the LD_PGFINDLIB_PATH environment variable as a standard source,
and buffer will get a list of any qualifying .so files in /lib/x86_64-linux-gnu.
The loader knows nothing about the items in this section, it's informative.</P>

<H3 id="Re order of execution">Re order of execution</H3><HR>
<P>Officially it's LD_AUDIT then LD_PRELOAD then DT_RPATH then LD_LIBRARY_PATH then DT_RUNPATH then ld.so.cache
then default_paths then LD_PGFINDLIB_PATH, so that's the display order.
However, one of the function's benefits is that it provides enough information for programmers to
pick .so files from the buffer and dlopen() them ignoring the loader's order.
Also the calling program can change the desired order, or add and remove sources, via
the FROM clause which will be described later.
</P>

<H3 id="Re $ORIGIN">Re $ORIGIN</H3><HR>
<P>It sees $ORIGIN, and replaces it with the path of the executable.
Example: if the executable is /home/pgulutzan/pgfindlib/main,
         and there is a file libcrypto.so on /home/pgulutzan/pgfindlib/lib,
         and the gcc has -Wl,-rpath,\$ORIGIN/lib (but good luck trying to specify it in a script or cmake!),
         then the result will be
         /home/pgulutzan/pgfindlib/lib/libcrypto.so
         and not $ORIGIN/lib/libcrypto.so, but a comment may also appear.
This is done by reading the symbolic link proc/self/exe with readlink.
Replacement is supposed to happen for DT_RPATH and DT_RUN_PATH but in fact it happens regardless of source.
There has to be a different source for
<a href="https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe">FreeBSD</a>.
In tests the usage was -Wl,-z,origin,-rpath,./lib,-rpath,\$ORIGIN</P>

<H3 id="Re $LIB and $PLATFORM">Re Re $LIB and $PLATFORM</H3><HR> 
<P>
It sees and replaces these "dynamic string tokens" according to what the loader would do, which may differ
from what the Linux documentation says.
For $LIB we only fall back to /lib64 or lib if there's an unexpected severe problem.
The results are what's expected for the platform but we do not search /tls or platform subdirectories.</P>

<H3 id="Re library lists">Re library lists</H3><HR>
<P>DT_RPATH and DT_RUNPATH and LD_LIBRARY_PATH and LD_RUNPATH and LD_PGFINDLIB_PATH can all contain lists of paths.
If the path is blank or there's a trailing : we do not treat it as "." which seems to be how loader handles it.</P>

<H3 id="Re ldconfig -p and uname -m">Re ldconfig -p and uname -m</H3><HR>
<P>These are very common, so one assumes that they're in the usual places and invokes them via popen().
If the call fails then there's a warning.
There's an assumption that ldconfig -p output looks the same on every Linux distro, which is undocumented,
and in fact the format on FreeBSD with ldconfig -r is quite different, but still okay.
Really minimal Linux distros e.g. Alpine won't have them, on such distros you'd need to install in advance.
The strings utility e.g. "strings -n5 /etc/ld.so.cache" might work but not with very short names.</P>

<H3 id="Re security">Re security</H3><HR>
<P>The list does not prove that the first-displayed library will be loaded first.
For example, the user might not have permission -- there will be a comment if access(..., R_OK)
fails, but no comment for other cases.</P>

<H3 id="Re trailing solidus i.e. forward slash i.e. /">Re trailing solidus i.e. forward slash i.e. /</H3><HR>
<P>See https://stackoverflow.com/questions/980255/should-a-directory-path-variable-end-with-a-trailing-slash
So if it already ends with / then we don't add a solidus, but if people want to pass ///, well, it's up to them.</P>

<H3 id="Re PGFINDLIB_MAX_PATH_LENGTH">Re PGFINDLIB_MAX_PATH_LENGTH</H3><HR>
<P>See https://stackoverflow.com/questions/833291/is-there-an-equivalent-to-winapis-max-path-under-linux-unix
Conclusion: depending on #include linux/limits.h won't do. So there's a #define PGFINDLIB_MAX_PATH_LENGTH 4096.
There will be an error if any soname is longer than that, or if the name of any found path is longer than that.</P>

<H3 id="Re Comments">Re Comments</H3><HR>
<P>As well as file names, by default buffer rows will contain comments.
For a list of possible comments see lines beginning with #define PGFINDLIB_COMMENT in pgfindlib.h.
If comments are due to errors, they will be in a warning column and most other columns will be empty.
</P>

<H3 id="Re comparisons">Re comparisons</H3><HR>
<P>Usually pgfindlib considers that a file is matching if it starts with one of the passed soname values.
This is so that soname = libx.so will also match libx.so.99 etc.
Emphasis: the loader prefers to grab the exact soname e.g. libcrypto.so.1.1. The fact that you get hits for libcrypto.so is just a hint. There is a message if a name refers to a symlink or hardlink.
Comparisons are case sensitive and results are undefined if the soname does not contain ".so*".</P>

<H3 id="Re pgfindlib_tests.sh">Re pgfindlib_tests.sh</H3><HR>
<P>This Bash script has a set of tests that the loader really goes in this order.
To make sure that your situation is the same as what we found on various test machines,
say chmod +x then ./pgfindlib_tests.sh --
You should see that all test results are marked "Good".</P>

<H3 id="FROM">Re FROM</H3><HR>
Initially you'l only care about "WHERE so-name-list". But there is an optional leading clause:<BR>
FROM source-list<BR>
The source can be: any of the standard source names (LD_LIBRARY_PATH etc.),
or non-standard names, in any order. A non-standard name should be a directory.
Comparisons or names are case sensitive.
For example:<BR>
FROM default_paths, LD_LIBRARY_PATH, /tmp WHERE libcrypto.so<BR>
will look in default_paths and LD_LIBRARY_PATH ignoring the other standard sources,
and will additionally check a non-standard source, the /tmp directory.

<H3 id="Some more possible tests">Some more possible tests</H3><HR>
<P>(using main.c which is supplied with the package)
<PRE>
gcc -Wall -o main main.c pgfindlib.c  -Wl,-rpath,./lib,-rpath,"/tmp/A B"
./main 'where libcrypto.so'
export LD_LIBRARY_PATH=/usr/lib64/'unusual path'
./main 'where libcrypto.so'
gcc -o main main.c pgfindlib.c -Wl,-rpath,./lib,-rpath,./ocelotgui,-disable-new-dtag
./main 'FROM D_RPATH, D_RUNPATH WHERE libcrypto.so'
gcc -o main main.c pgfindlib.c -Wl,-z,origin,-rpath,./lib,-rpath,\$ORIGIN
./main 'where libcrypto.so'
gcc -o main main.c pgfindlib.c -DPGFINDLIB_INCLUDE_ROW_SOURCE_NAME=1 -Wl,-z,origin,-rpath,./lib,-rpath,\$ORIGIN
./main 'where libcrypto.so'</PRE>
</PRE></P>

<H3 id="Contacts">Contacts</H3><HR>
<P>GitHub. https://github.com/pgulutzan/pgfindlib
This is where the sources of pgfindlib are, so use git clone to get them initially.
The pgfindlib repository is also the place to put error reports, or just click "Like".
</P>
