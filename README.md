pgfindlib

<P>Version 0.9.4</P>

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
./main libutil.so:libcurl.so
</PRE>
The result might look like this:
<PRE>
/* pgfindlib version 0.9.4 */
/* $LIB=lib/x86_64-linux-gnu $PLATFORM=x86_64 $ORIGIN=/home/pgulutzan/pgfindlib */
/* LD_PRELOAD */
/* DT_RPATH */
/* LD_LIBRARY_PATH */
/* replaced /$LIB with /lib/x86_64-linux-gnu */
/lib/x86_64-linux-gnu/libutil.so
/lib/x86_64-linux-gnu/libutil.so.1
/lib/x86_64-linux-gnu/libcurl.so.4.6.0
/lib/x86_64-linux-gnu/libcurl.so
/lib/x86_64-linux-gnu/libcurl.so.4
/* DT_RUNPATH */
/tmp/pgfindlib_example/libutil.so
/* LD_RUN_PATH */
/* ld.so.cache */
/lib/x86_64-linux-gnu/libutil.so.1
/lib/x86_64-linux-gnu/libutil.so
/lib/x86_64-linux-gnu/libcurl.so.4
/lib/x86_64-linux-gnu/libcurl.so

rval=0
</PRE>
</P>

<P>This means: the loader would look first in /lib/x86_64-linux-gnu
because there was an earlier "export LD_LIBRARY_PATH='/$LIB'"
(not shown because values of environment variables can be surprises).
This takes precedence over DT_RUNPATH, which is where the first
occurrence of libutil.so appears (this appears because of the
-rpath option in the gcc command). Finally there are some .so libraries
in ld.so.cache, which is where the loader would go if there was no
prior.</P>

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
It's arguments are:<BR>
 sonames, a colon-separated list of .so names e.g. libcrypto.so:libmariadb.so<BR>
 buffer, a space to receive the result<BR>
 buffer_max_length, maximum number of bytes in result including \0 terminator<BR>
 extra_paths, usually null, described later
</P>

<P>The return is:<BR>
buffer contents with paths including .so names in the order the loader looks for them,
which is determined by gcc flags and environment variables.</P>

<P>The license is: GPLv2.</P>

<P>The assumed environment is: linux or FreeBSD containing gcc and the almost-always-present utilities ldconfig, uname.
</P>

<H3 id="Re Errors">Re Errors</H3><HR>
  As well as filling the buffer, pgfindlib returns an error code as defined in pgfindlib.h:
  0 PGFINDLIB_OK no error
  -1 PGFINDLIB_ERROR_MAX_BUFFER_LENGTH_TOO_SMALL execution stopped after output of nearly buffer_max_length bytes
  -2 PGFINDLIB_ERROR_NULL e.g. pgfindlib() was called with a null pointer so nothing was done
  -3 PGFINDLIB_ERROR_MAX_PATH_LENGTH_TOO_SMALL e.g. a path is 5000 bytes (the default maximum is 4096)
  ... In fact anything other than 0 should be extremely rare.

<H3 id="Re Filter">Re Filter</H3><HR>
  From the passed sonames, we use pgfindlib_find_line_in_sonames() to filter the results of readdir() + popen("...ldconfig").
  For example, when calling from ocelotgui, we only care about .so libraries that might be needed for
  MariaDB or MySQL or Tarantool, which can include libcrypto (we might care for fewer .so libraries if ocelotgui
  is started with appropriate command-line or configuration-file options or a CMakeLists.txt switch.
  So we'd call pgfindlib("libmysqlclient.so:libmariadb.so:libmariadbclient.so:tarantool.so", ...);

<H3 id="Re LD_AUDIT">Re LD_AUDIT</H3><HR>
<P>Actually .so files in the LD_AUDIT environment variable https://man7.org/linux/man-pages/man7/rtld-audit.7.html
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
elf.h seems to be standard but it might be useless to try to look for it with mingw.
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

<H3 id="Re /etc/ld.so.cache">Re /etc/ld.so.cache</H3><HR>
<P>Delivers the result of /sbin/ldconfig -p. If /sbin/ldconfig is not found (very unlikely), it tries again with
/usr/sbin/ldconfig or /bin/ldconfig or /usr/bin/ldconfig or ldconfig. If execution with the -p option
fails, it tries again with the -r option (-p is correct for Linux, -r is possible with FreeBSD).
If everything fails, there will be a comment.</P>

<H3 id="Re extra_paths">Re extra_paths</H3><HR>
<P>This is another colon-or-semicolon-delimited string that can contain more paths that will be added at the end.
For example ./main libcrypto.so /lib/x86_64-linux-gnu/ will pass "/lib/x86_64-linux-gnu/" as the final
argument to pgfindlib(), and buffer will get a list of any qualifying .so files in /lib/x86_64-linux-gnu.
The loader knows nothing about the items in this section, it's informative.</P>

<H3 id="Re order of execution">Re order of execution</H3><HR>
<P>Officially it's LD_PRELOAD then DT_RPATH then LD_LIBRARY_PATH then DT_RUNPATH then /etc/ls.so.cache,
so that's the display order.
However, ocelotgui in fact goes LD_LIBRARY_PATH then LD_RUN_PATH then /etc/ls.so.cache
then DT_RPATH or DT_RUNPATH.</P>

<H3 id="Re $ORIGIN">Re $ORIGIN</H3><HR>
<P>It sees $ORIGIN, and replaces it with the path of the executable.
Example: if the executable is /home/pgulutzan/pgfindlib/main libcrypto,
         and there is a file libcrypto.so on /home/pgulutzan/pgfindlib/lib,
         and the gcc has -Wl,-rpath,\$ORIGIN/lib (but good luck trying to specify it in a script or cmake!),
         then the result will be
         /home/pgulutzan/pgfindlib/lib/libcrypto.so
         and not $ORIGIN/lib/libcrypto.so, but a comment may also appear.
This is done by reading the symbolic link proc/self/exe with readlink.
It's supposed to happen for DT_RPATH and DT_RUN_PATH but in fact it happens regardless of source.
There has to be a different source for FreeBSD:
  https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
In tests the usage was -Wl,-z,origin,-rpath,./lib,-rpath,\$ORIGIN</P>

<H3 id="Re $LIB and $PLATFORM">Re Re $LIB and $PLATFORM</H3><HR> 
<P>
It sees and replaces these according to what the loader would do, which may differ
from what the Linux documentation says.
For $LIB we only fall back to /lib64 or lib if there's an unexpected severe problem.
The results are what's expected for the platform but we do not search /tls or platform subdirectoriesd.</P>

<H3 id="Re library lists">Re library lists</H3><HR>
<P>DT_RPATH and DT_RUNPATH and LD_LIBRARY_PATH and LD_RUNPATH and extra_paths can all contain lists of paths.
If the path is blank or there's a trailing : we do not treat it as "." which seems to be how loader handles it.</P>

<H3 id="Re ldconfig -p and uname -m">Re ldconfig -p and uname -m</H3><HR>
<P>These are very common, so one assumes that they're in the usual places and invokes them via popen().
If the call fails then there's no output and no error.
There's an assumption that ldconfig -p output looks the same on every Linux distro, which is undocumented,
and in fact the format on FreeBSD with ldconfig -r is quite different, but still okay.
Really minimal Linux distros e.g. Alpine won't have them, on such distros you'd need to install in advance.
The strings utility e.g. "strings -n5 /etc/ld.so.cache" might work but not with very short names.</P>

<H3 id="Re security">Re security</H3><HR>
<P>The list does not prove that the first-displayed library will be loaded first.
For example, the user might not have permission -- there will be a comment if access(..., X_OK)
fails, but no comment for other cases.</P>

<H3 id="Re trailing solidus i.e. forward slash i.e. /">Re trailing solidus i.e. forward slash i.e. /e</H3><HR>
<P>See https://stackoverflow.com/questions/980255/should-a-directory-path-variable-end-with-a-trailing-slash
So if it already ends with / then we don't add a solidus, but if people want to pass ///, well, it's up to them.</P>

<H3 id="Re PGFINDLIB_MAX_PATH_LENGTH">Re PGFINDLIB_MAX_PATH_LENGTH</H3><HR>
<P>See https://stackoverflow.com/questions/833291/is-there-an-equivalent-to-winapis-max-path-under-linux-unix
Conclusion: depending on #include linux/limits.h won't do. So there's a #define PGFINDLIB_MAX_PATH_LENGTH 4096.
There will be an error if any soname is longer than that, or if the name of any found path is longer than that.</P>

<H3 id="Re Comments">Re Comments</H3><HR>
<P>As well as file names, by default buffer will contain comments.
For example /* LD_LIBRARY_PATH */" tells you that what follows, if anything,
is due to the LD_LIBRARY_PATH environment variable.
Comments are always separate lines and always formatted like C-style comments.
To strip comments, use gcc ... -DPGFINDLIB_WARNING_LEVEL=0 (the default is 4).</P>

<H3 id="Re comparisons">Re comparisons</H3><HR>
<P>Usually pgfindlib considers that a file is matching if it starts with one of the passed soname values.
This is so that soname = libx.so will also match libx.so.99 etc.
Emphasis: the loader prefers to grab the exact soname e.g. libcrypto.so.1.1. The fact that you get hits for libcrypto.so is just a hint. There is no check whether a name refers to a symlink.</P>

<H3 id="Re pgfindlib_tests.sh">Re pgfindlib_tests.sh</H3><HR>
<P>This Bash script has a set of tests that the loader really goes in this order.
To make sure that your situation is the same as what we found on various test machines,
say chmod +x then ./pgfindlib_tests.sh
You should see that all test results are marked "Good".</P>

<H3 id="Some more possible tests">Some more possible tests</H3><HR>
<P>(using main.c which is supplied with the package)
<PRE>
gcc -Wall -o main main.c pgfindlib.c  -Wl,-rpath,./lib,-rpath,"/tmp/A B"
./main libcrypto.so
export LD_LIBRARY_PATH=/usr/lib64/'unusual path'
./main libcrypto.so
gcc -o main main.c pgfindlib.c -Wl,-rpath,./lib,-rpath,./ocelotgui,-disable-new-dtag
./main libcrypto.so
gcc -o main main.c pgfindlib.c -Wl,-z,origin,-rpath,./lib,-rpath,\$ORIGIN
./main libcrypto.so
gcc -o main main.c pgfindlib.c -DPGFINDLIB_WARNING_LEVEL=1 -Wl,-z,origin,-rpath,./lib,-rpath,\$ORIGIN
./main libcrypto.so</PRE>
</PRE></P>

<H3 id="Contacts">Contacts</H3><HR>
<P>GitHub. https://github.com/pgulutzan/pgfindlib
This is where the sources of pgfindlib are, so use get clone to get them initially.
The pgfindlib repository is also the place to put error reports, or just click "Like".
</P>
