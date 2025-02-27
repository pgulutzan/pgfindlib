#findmyso_tests.h -- Test effects of flags + environment with Linux .so libraries.
#
#
#Copyright (c) 2025 by Peter Gulutzan.
#
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the  nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL  BE LIABLE FOR ANY
#DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#!/bin/bash
#Comments:
#  Required utilities: linux, bash, sed, gcc
#  Required permissions: to create/modify/destroy files and directories in /tmp/findmyso_tests
#  This creates small files in /tmp/findmyso_tests and destroys everything in /tmp/findmyso_tests. 
#  To run:
#    chmod +x findmyso_tests.sh
#    ./findmyso_tests.sh

#The 27 tests here involve combinations of gcc flags, LD_LIBRARY_PATH, LD_RUN_PATH, and LD_PRELOAD.
#They should all work, i.e. printf "Good".
#The method is to create multiple .so files with the same name (but different displays) in different directories,
#then change some of the settings to point to the different directories, then execute and see from the display
#which .so in which directory was actually loaded. That will depend on the priority that the loader uses.
#For example, in the test where gcc flags + LD_LIBRARY_PATH + LD_RUN_PATH + LD_PRELOAD all have settings,
#this will display the directory that LD_PRELOAD mentions, showing it has priority.
#There is no testing of ld.so.cache, of $LIB, of $PLATFORM, or of flags other than dtags flags.
#There is no testing with LD_AUDIT, it is unset at the start.
#The LD_PRELOAD testing is always done in combination with gcc ...  -Wl,-rpath,/tmp/findmyso_tests for unknown reasons.
#The results do not contradict the Linux documentation, but hopefully:
#1. Demonstrate that the Linux documentation is correct even though it may seem hard to follow.
#2. Correct misimpressions caused by incomplete statements made on stackoverflow, vendor manuals, etc.
#3. Show that the delimiters really are not consistent.
#4. Relieve people from the rather tedious job of making directories, building .so libraries, and interpreting.


#See Peter Gulutzan's github repository https://github.com/pgulutzan/ for updates or for related information.

unset LD_PRELOAD; unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_AUDIT

printf "findmyso_tests.h -- Test effects of flags + environment with Linux .so libraries.\n"
printf "For explanation read the comments inside findmyso.c findmyso_tests.sh\n"
printf "This creates small files in /tmp/findmyso_tests and destroys everything in /tmp/findmyso_tests.\n"
read -p "Type Y to continue, or anything else to exit. " yn
if [[ "$yn" != "Y" ]]; then exit; fi

rm -r -f /tmp/findmyso_tests || { echo "rm -r -f /tmp/findmyso_tests failed so execution cannot continue" && return 1; }
mkdir /tmp/findmyso_tests || { echo "mkdir /tmp/findmyso_tests failed so execution cannot continue" && return 1; }
cd /tmp/findmyso_tests


#Create shared_library.h
echo '
#ifndef SHARED_LIBRARY_H
#define SHARED_LIBRARY_H
extern void shared_library(void);
#endif  // SHARED_LIBRARY_H
' > shared_library.h

#Create shared_library.c. It must print '** !'
echo '
#include <stdio.h>
#include <unistd.h>
void shared_library(void)
{
  printf("** !");
}
' > shared_library.c

#Create main.c. It should not print anything.
echo '
#include <stdio.h>
#include "shared_library.h"
int main(void)
{
  shared_library();
  return 0;
}' > main.c

#Compile shared_library and make shared_library a shared library
gcc -c -Wall -Werror -fpic shared_library.c
gcc -shared -o libshared_library.so shared_library.o

#Compile main with executable name = test
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library

#Create subdirectories A B C D (not E) and put libshared_library.so in them

mkdir /tmp/findmyso_tests/A
 sed -i 's/** !/** A/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/A/libshared_library.so

mkdir /tmp/findmyso_tests/B
 sed -i 's/** A/** B/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/B/libshared_library.so

mkdir /tmp/findmyso_tests/C
 sed -i 's/** B/** C/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/C/libshared_library.so

mkdir /tmp/findmyso_tests/D
 sed -i 's/** C/** D/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/D/libshared_library.so

#Create subdirectory E A
mkdir "/tmp/findmyso_tests/E A"
 sed -i 's/** D/** E A/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so "/tmp/findmyso_tests/E A/libshared_library.so"

#Create subdirectory F and put libshared_library.so + variants in it
mkdir /tmp/findmyso_tests/F
 sed -i 's/** E A/** F .so/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/F/libshared_library.so
 sed -i 's/** F .so/** F .so.21/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/F/libshared_library.so.21
 sed -i 's/** F .so.21/** F .so./g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/F/libshared_library.so.
 sed -i 's/** F .so./** F .so.100/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/F/libshared_library.so.100

#Create subdirectory G and put something invalid in G/libshared_library.so
mkdir /tmp/findmyso_tests/G
echo '
Junk
' > /tmp/findmyso_tests/G/libshared_library.so

#Create subdirectory H/H2 and 
mkdir /tmp/findmyso_tests/H
mkdir /tmp/findmyso_tests/H/H2
 sed -i 's/** F .so.100/** H2/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o
 cp libshared_library.so /tmp/findmyso_tests/H/H2/libshared_library.so

#Still on /tmp/findmyso_tests the printf should be !
 sed -i 's/** H2/** !/g' shared_library.c
 gcc -c -Wall -Werror -fpic shared_library.c
 gcc -shared -o libshared_library.so shared_library.o

#End of setup. Now run the tests.
#If, for the specified situation, loader finds the right library, "Good."
#Else "Bad."

let good_count=0
let bad_count=0

printf "Test #1 -- with no non-default flags or environment variables\n"
printf "  (Default means gcc with no rpath, LD_LIBRARY_PATH + LD_RUN_PATH + LD_PRELOAD unset.)\n"
printf "  Result should be: Found no library.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

printf "Test #2 -- with LD_LIBRARY_PATH to A\n"
printf "  Result should be: Found library A, showing LD_LIBRARY_PATH has effect.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_LIBRARY_PATH=/tmp/findmyso_tests/A
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #3 -- gcc with -rpath to E and B but new-dtag not specified, LD_LIBRARY_PATH to A.\n"
printf "  Result should be: Found library A, because:\n"
printf "  Nowadays gcc thinks --enable-new-dtag is default so -rpath affects DT_RUNPATH,\n"
printf "  and DT_RUNPATH is lower priority than LD_LIBRARY_PATH.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests/E,-rpath,/tmp/findmyso_tests/B
export LD_LIBRARY_PATH=/tmp/findmyso_tests/A
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #4 -- gcc with -rpath to E and B and new-dtag disabled explicitly, LD_LIBRARY_PATH to A.\n"
printf "  Result should be: B, because:\n"
printf "  The blank library E is skipped\n"
printf "  --enable-new-dtag is disabled so -rpath affects DT_RPATH,\n"
printf "  and DT_RPATH is higher priority than LD_LIBRARY_PATH.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests/E,-rpath,/tmp/findmyso_tests/B,-disable-new-dtag
export LD_LIBRARY_PATH=/tmp/findmyso_tests/A
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** B" ]]; then
   echo "  Found library B -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #5 -- gcc with -rpath to H and new-dtag disabled explicitly, LD_LIBRARY_PATH to B.\n"
printf "  Result should be: B, because:\n"
printf "  DT_RPATH has priority but there is nothing in H, and H/H2 is not checked,\n"
printf "  so LD_LIBRARY_PATH has effect.\n"
printf "  There are statements that DT_RPATH searching is recursive, but in this context\n"
printf "  recursive doesn't mean search subdirectories as one would do for ls -R,\n"
printf "  rather it apparently means: if .so #1 depends on .so #2 there will be a search for .so #2.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests/E,-rpath,/tmp/findmyso_tests/H,-disable-new-dtag
export LD_LIBRARY_PATH=/tmp/findmyso_tests/B
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** B" ]]; then
   echo "  Found library B -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #6 -- with -rpath to E and B. new-dtag disabled explicitly, LD_LIBRARY_PATH to A after gcc.\n"
printf "  Result should be: B, because: DT_RPATH has both E and B, but E is blank so it is skipped.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests/E,-rpath,/tmp/findmyso_tests/B,-disable-new-dtag
export LD_LIBRARY_PATH=/tmp/findmyso_tests/A
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** B" ]]; then
   echo "  Found library B -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #7 -- -rpath to A, LD_LIBRARY_PATH to B. LD_RUN_PATH to C. LD_PRELOAD to D (after gcc).\n"
printf "  Result should be: D, because LD_PRELOAD has higher priority than all the others.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests/A,-disable-new-dtag
export LD_LIBRARY_PATH=/tmp/findmyso_tests/B
export LD_RUN_PATH=/tmp/findmyso_tests/C
export LD_PRELOAD=/tmp/findmyso_tests/D/libshared_library.so
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** D" ]]; then
   echo "  Found library D -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #8 -- LD_LIBRARY_PATH to E. LD_RUN_PATH to D and C (after gcc).\n"
printf "  Result should be: Found no library, because LD_RUN_PATH has no effect at runtime.\n"
printf "  (Apparently it does in some places, but not with Linux and gcc.)\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_LIBRARY_PATH=/tmp/findmyso_tests/E
export LD_RUN_PATH=/tmp/findmyso_tests/D:/tmp/findmyso_tests/C
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** D" ]]; then
   echo "  Found library D -- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Good."; let "good_count=good_count+1"
fi

printf "Test #9 -- LD_LIBRARY_PATH to E. LD_RUN_PATH (before gcc) to E and A, (after gcc) to D and C.\n"
printf "  Result should be: A, because:\n"
printf "  although LD_RUN_PATH has no effect if it's set after gcc,\n"
printf "  it does have effect if it's set before gcc, i.e. it's part of the building.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
export LD_RUN_PATH='/tmp/findmyso_tests/E:/tmp/findmyso_tests/A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_RUN_PATH=/tmp/findmyso_tests/D:/tmp/findmyso_tests/C
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #10 -- LD_LIBRARY_PATH to F.\n"
printf "  Result should be: F .so. This was just to check the loader only looks at .so and not .so*.\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH='/tmp/findmyso_tests/F'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_RUN_PATH=/tmp/findmyso_tests/D:/tmp/findmyso_tests/C
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** F .so" ]]; then
   echo "  Found library F .so -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi

#Result should be .so in J even though the -L said to use path C
#because that is "the directory containing the shared object"
#(though there are stories that the 
printf "Test #11 -- gcc with origin flag\n"
printf "  Result should be: ! because ORIGIN is recognized, so original-directory .so is loaded.\n"
printf "  Warning: Tests with dollar signs are tricky, using dollar sign plus ORIGIN might not always work.\n"
printf "  The same applies for dollar sign plus LIB or dollar sign plus PLATFORM,\n"
printf "  but they are not tested because the testing would require root privileges.\n"
printf "  See https://man7.org/linux/man-pages/man8/ld.so.8.html\n"
unset LD_LIBRARY_PATH; unset LD_RUN_PATH; unset LD_PRELOAD; unset ORIGIN; unset LDFLAGS
#Create subdirectory J and ensure library directory != executable directory
cd /tmp/findmyso_tests
cp shared_library.c shared_library.c.bak
cp shared_library.o shared_library.o.bak
cp libshared_library.so libshared_library.so.bak
mkdir /tmp/findmyso_tests/J
cp /tmp/findmyso_tests/shared_library.c /tmp/findmyso_tests/J/shared_library.c
cp /tmp/findmyso_tests/shared_library.h /tmp/findmyso_tests/J/shared_library.h
cp /tmp/findmyso_tests/main.c /tmp/findmyso_tests/J/main.c
cd /tmp/findmyso_tests/J
gcc -L/tmp/findmyso_tests -Wall  main.c -lshared_library -o test -Wl,-z,origin -Wl,-rpath,\$ORIGIN
sed -i 's/** !/** J/g' shared_library.c
gcc -c -Wall -Werror -fpic shared_library.c
gcc -shared -o libshared_library.so shared_library.o
cd /tmp/findmyso_tests
cp libshared_library.so.bak libshared_library.so
cp shared_library.o.bak shared_library.o
cp shared_library.c.bak shared_library.c
cd /tmp/findmyso_tests/B
result=$(/tmp/findmyso_tests/J/test 2>/dev/null)
if [[ "$result" == "** J" ]]; then
   echo "  Found library J -- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library-- Bad."; let "bad_count=bad_count+1"
fi
cd /tmp/findmyso_tests

printf "Test #12 -- with LD_LIBRARY_PATH=G;A.\n"
printf "  Result should be: Found no library, because the library in G is invalid,\n"
printf "  and when there is an error the loader does not continue to look for the library in A.\n"
printf "  LD_LIBRARY_PATH=G;A. LD_RUN_PATH unset. LD_PRELOAD unset. "
unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH='G;A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

printf "Test #13 -- with LD_LIBRARY_PATH=.. (two full stops).\n"
printf "  Result should be: ! because .. means the directory above the current one.\n"
printf "  (The test includes a cd to A after the gcc but before the execution.)\n"
unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH=..
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
cd /tmp/findmyso_tests/A
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** !" ]]; then
   echo "  Found library !-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi
cd /tmp/findmyso_tests

printf "Test #14 -- with LD_LIBRARY_PATH=E;A\n"
printf "  Result should be: A because semicolon is a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH='E;A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #15 -- with LD_LIBRARY_PATH=E:A\n"
printf "  Result should be: A because colon is a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH='E:A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

#This also shows that LD_LIBRARY_PATH can be relative
printf "Test #16 -- with LD_LIBRARY_PATH=E:E A\n"
printf "  Result should be: Found library E A  because space is not a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH='E:E A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** E A" ]]; then
   echo "  Found library E A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #17 -- with LD_LIBRARY_PATH=E,A\n"
printf "  Result should be: Found no library because comma is not a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_PRELOAD
export LD_LIBRARY_PATH='E,A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

#LD_PRELOAD cannot be relative, specify in full.
#Also  -Wl,-rpath,/tmp/findmyso_tests is necessary.
#Also it has to happen after gcc, gcc can fail if LD_PRELOAD happens first.
#Errors might be ignored.
printf "Test #18 -- with LD_PRELOAD=A;A\n"
printf "  Result should be: Found no library because semicolon is not a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_LIBRARY_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_PRELOAD='/tmp/findmyso_tests/A/libshared_library.so;/tmp/findmyso_tests/A/libshared_library.so'
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

printf "Test #19 -- with LD_PRELOAD=A:A"
printf "  Result should be: A because colon is a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_LIBRARY_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests
export LD_PRELOAD='/tmp/findmyso_tests/A/libshared_library.so:/tmp/findmyso_tests/A/libshared_library.so'
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #20 -- with LD_PRELOAD=E A\n"
printf "  Result should be: Found Library A because space is a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_LIBRARY_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests
export LD_PRELOAD='/tmp/findmyso_tests/E/libshared_library.so /tmp/findmyso_tests/E A/libshared_library.so'
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #21 -- with LD_PRELOAD=E:E A\n"
printf "  Result should be: Found no library because space is a valid delimiter.\n"
unset LD_RUN_PATH; unset LD_LIBRARY_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_PRELOAD='/tmp/findmyso_tests/E: /tmp/findmyso_tests/E A'
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** E A" ]]; then
   echo "  Found library E A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

printf "Test #22 -- with LD_PRELOAD=A,A\n"
printf "  Result should be: Found ! because comma is not a valid delimiter.\n"
printf "  (In this case the error is ignored so the .so in the -rpath is loaded.)\n"
unset LD_RUN_PATH; unset LD_LIBRARY_PATH; unset LD_PRELOAD
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library -Wl,-rpath,/tmp/findmyso_tests
export LD_PRELOAD='/tmp/findmyso_tests/A/libshared_library.so,/tmp/findmyso_tests/A/libshared_library.so'
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** !" ]]; then
   echo "  Found library !-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #23 -- with LD_RUN_PATH=E;A\n"
printf "  Result should be: Found no library because semicolon is not a valid delimiter.\n"
unset LD_LIBRARY_PATH; unset LD_PRELOAD
export LD_RUN_PATH='E;A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

printf "Test #24 -- with LD_RUN_PATH=E:A\n"
printf "  Result should be: A because colon is a valid delimiter.\n"
unset LD_LIBRARY_PATH; unset LD_PRELOAD
export LD_RUN_PATH='E:A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi

printf "Test #25 -- with LD_RUN_PATH=E:E A\n"
printf "  Result should be: Found library E A because space is not a valid delimiter.\n"
unset LD_LIBRARY_PATH; unset LD_PRELOAD
export LD_RUN_PATH="E:E A"
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** E A" ]]; then
   echo "  Found library E A-- Good."; let "good_count=good_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Bad."; let "bad_count=bad_count+1"
fi


printf "Test #26 -- with LD_RUN_PATH=E,A\n"
printf "  Result should be: Found no library because comma is not a valid delimiter.\n"
unset unset LD_PRELOAD: unset LD_LIBRARY_PATH
export LD_RUN_PATH='E,A'
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

printf "Test #27 -- with LD_LIBRARY_PATH=' '\n"
printf "  Result should be: Found no library though blank doesn't mean unset.\n"
unset LD_RUN_PATH; unset LD_PRELOAD; unset LD_LIBRARY_PATH
gcc -L/tmp/findmyso_tests -Wall -o test main.c -lshared_library
export LD_LIBRARY_PATH=''
result=$(/tmp/findmyso_tests/test 2>/dev/null)
if [[ "$result" == "** A" ]]; then
   echo "  Found library A-- Bad."; let "bad_count=bad_count+1"
elif [[ "$result" > "** " ]]; then
   echo "  Found library $result-- Bad."; let "bad_count=bad_count+1"
else
   echo "  Found no library -- Good."; let "good_count=good_count+1"
fi

#Cleanup by cd back to /tmp and destroying all files in findmyso_tests.
#cd ..
#rm -r -f /tmp/findmyso_tests

echo "Good: $good_count"
echo "Bad: $bad_count"
