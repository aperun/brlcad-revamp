#     B R L C A D _ C H E C K F U N C T I O N S . C M A K E
# BRL-CAD
#
# Copyright (c) 2011-2018 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###
# Automate putting variables from tests into a config.h.in file,
# and otherwise wrap check macros in extra logic as needed

include(CMakeParseArguments)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckCInline)

set(std_hdr_includes
"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if HAVE_GETOPT_H
# include <getopt.h>
#endif
#if HAVE_SIGNAL_H
# include <signal.h>
#endif
#if HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#if HAVE_SYS_SHM_H
# include <sys/shm.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
"
)

# Use this function to construct compile defines that uses the CMake
# header tests results to properly support the above header includes
function(std_hdr_defs def_str)
  set(def_full_str)
  string(REPLACE "\n" ";" deflist "${std_hdr_includes}")
  foreach(dstr ${deflist})
    if("${dstr}" MATCHES ".*HAVE_.*" )
      string(REGEX REPLACE "^.if[ ]+" "" defcore "${dstr}")
      set(def_full_str "${def_full_str} -D${defcore}=${${defcore}}")
    endif("${dstr}" MATCHES ".*HAVE_.*" )
  endforeach(dstr ${deflist})
  set(${def_str} "${def_full_str}" PARENT_SCOPE)
endfunction(std_hdr_defs)

###
# Check if a function exists (i.e., compiles to a valid symbol).  Adds
# HAVE_* define to config header.
###
macro(BRLCAD_FUNCTION_EXISTS function)

  # Use the upper case form of the function for variable names
  string(TOUPPER "${function}" var)

  # Only do the testing once per configure run
  if(NOT DEFINED HAVE_${var})

    # For this first test, be permissive.  For a number of cases, if
    # the function exists AT ALL we want to know it, so don't let the
    # flags get in the way
    set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS "")

    # Clear our testing variables, unless something specifically requested
    # is supplied as a command line argument
    cmake_push_check_state(RESET)
    if(${ARGC} GREATER 2)
      # Parse extra arguments
      CMAKE_PARSE_ARGUMENTS(${var} "" "DECL_TEST_SRCS" "WORKING_TEST_SRCS;REQUIRED_LIBS;REQUIRED_DEFS;REQUIRED_FLAGS;REQUIRED_DIRS" ${ARGN})
      set(CMAKE_REQUIRED_LIBRARIES ${${var}_REQUIRED_LIBS})
      set(CMAKE_REQUIRED_FLAGS ${${var}_REQUIRED_FLAGS})
      set(CMAKE_REQUIRED_INCLUDES ${${var}_REQUIRED_DIRS})
      set(CMAKE_REQUIRED_DEFINITIONS ${${var}_REQUIRED_DEFS})
    endif(${ARGC} GREATER 2)

    # Set the compiler definitions for the standard headers
    std_hdr_defs(std_defs)
    set(CMAKE_REQUIRED_DEFINITIONS "${std_defs} ${CMAKE_REQUIRED_DEFINITIONS}")

    # First (permissive) test
    CHECK_FUNCTION_EXISTS(${function} HAVE_${var})

    # Now, restore the C flags - any subsequent tests will be done using the
    # parent C_FLAGS environment.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")

    if(HAVE_${var})

      # Test if the function is declared.This will set the HAVE_DECL_${var}
      # flag.  Unlike the HAVE_${var} results, this will not be automatically
      # written to the CONFIG_H_FILE - the caller must do so if they want to
      # capture the results.
      if(NOT DEFINED HAVE_DECL_${var})
	if(NOT "${${var}_DECL_TEST_SRCS}" STREQUAL "")
	  check_c_source_compiles("${${var}_DECL_TEST_SRCS}" HAVE_DECL_${var})
	else(NOT "${${var}_DECL_TEST_SRCS}" STREQUAL "")
	  check_c_source_compiles("${std_hdr_includes}\nint main() {(void)${function}; return 0;}" HAVE_DECL_${var})
	endif(NOT "${${var}_DECL_TEST_SRCS}" STREQUAL "")
	set(HAVE_DECL_${var} ${HAVE_DECL_${var}} CACHE BOOL "Cache decl test result")
      endif(NOT DEFINED HAVE_DECL_${var})

      # If we have sources supplied for the purpose, test if the function is working.
      if(NOT DEFINED HAVE_WORKING_${var})
	if(NOT "${${var}_COMPILE_TEST_SRCS}" STREQUAL "")
	  set(HAVE_WORKING_${var} 1)
	  foreach(test_src ${${var}_DECL_TEST_SRCS})
	    check_c_source_compiles("${${test_src}}" ${var}_${test_src}_COMPILE)
	    if(NOT ${var}_${test_src}_COMPILE)
	      set(HAVE_WORKING_${var} 0)
	    endif(NOT ${var}_${test_src}_COMPILE)
	  endforeach(test_src ${${var}_COMPILE_TEST_SRCS})
	  set(HAVE_WORKING_${var} ${HAVE_DECL_${var}} CACHE BOOL "Cache working test result")
	endif(NOT "${${var}_COMPILE_TEST_SRCS}" STREQUAL "")
      endif(NOT DEFINED HAVE_WORKING_${var})

    endif(HAVE_${var})

    cmake_pop_check_state()

  endif(NOT DEFINED HAVE_${var})

  # The config file is regenerated every time CMake is run, so we
  # always need this bit even if the testing is already complete.
  if(CONFIG_H_FILE AND HAVE_${var})
    CONFIG_H_APPEND(BRLCAD "#cmakedefine HAVE_${var} 1\n")
  endif(CONFIG_H_FILE AND HAVE_${var})

endmacro(BRLCAD_FUNCTION_EXISTS)


###
# Check if a header exists.  Headers requiring other headers being
# included first can be prepended in the filelist separated by
# semicolons.  Add HAVE_*_H define to config header.
###
macro(BRLCAD_INCLUDE_FILE filelist var)
  # check with no flags set
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_STD_FLAG}")

  # !!! why doesn't this work?
  # if("${var}" STREQUAL "HAVE_X11_XLIB_H")
  #  message("CMAKE_REQUIRED_INCLUDES for ${var} is ${CMAKE_REQUIRED_INCLUDES}")
  # endif("${var}" STREQUAL "HAVE_X11_XLIB_H")

  if(NOT "${ARGV2}" STREQUAL "")
    set(CMAKE_REQUIRED_INCLUDES_BKUP ${CMAKE_REQUIRED_INCLUDES})
    set(CMAKE_REQUIRED_INCLUDES ${ARGV2} ${CMAKE_REQUIRED_INCLUDES})
  endif(NOT "${ARGV2}" STREQUAL "")

  # search for the header
  CHECK_INCLUDE_FILES("${filelist}" ${var})

  # !!! curiously strequal matches true above for all the NOT ${var} cases
  # if (NOT ${var})
  #   message("--- ${var}")
  # endif (NOT ${var})

  if(CONFIG_H_FILE AND ${var})
    CONFIG_H_APPEND(BRLCAD "#cmakedefine ${var} 1\n")
  endif(CONFIG_H_FILE AND ${var})

  if(NOT "${ARGV2}" STREQUAL "")
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_BKUP})
  endif(NOT "${ARGV2}" STREQUAL "")

  # restore flags
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endmacro(BRLCAD_INCLUDE_FILE)


###
# Check if a C++ header exists.  Adds HAVE_* define to config header.
###
macro(BRLCAD_INCLUDE_FILE_CXX filename var)
  set(CMAKE_CXX_FLAGS_TMP "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_STD_FLAG}")
  CHECK_INCLUDE_FILE_CXX(${filename} ${var})
  if(CONFIG_H_FILE AND ${var})
    CONFIG_H_APPEND(BRLCAD "#cmakedefine ${var} 1\n")
  endif(CONFIG_H_FILE AND ${var})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_TMP}")
endmacro(BRLCAD_INCLUDE_FILE_CXX)


###
# Check the size of a given typename, setting the size in the provided
# variable.  If type has header prerequisites, a semicolon-separated
# header list may be specified.  Adds HAVE_ and SIZEOF_ defines to
# config header.
###
macro(BRLCAD_TYPE_SIZE typename headers)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_STD_FLAG}")
  set(CMAKE_EXTRA_INCLUDE_FILES_BAK "${CMAKE_EXTRA_INCLUDE_FILES}")
  set(CMAKE_EXTRA_INCLUDE_FILES "${headers}")
  # Generate a variable name from the type - need to make sure
  # we end up with a valid variable string.
  string(REGEX REPLACE "[^a-zA-Z0-9]" "_" var ${typename})
  string(TOUPPER "${var}" var)
  # Proceed with type check.  To make sure checks are re-run when
  # re-testing the same type with different headers, create a test
  # variable incorporating both the typename and the headers string
  string(REGEX REPLACE "[^a-zA-Z0-9]" "_" testvar "HAVE_${typename}${headers}")
  string(TOUPPER "${testvar}" testvar)
  CHECK_TYPE_SIZE(${typename} ${testvar})
  set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_EXTRA_INCLUDE_FILES_BAK}")
  # Produce config.h lines as appropriate
  if(CONFIG_H_FILE AND ${testvar})
    CONFIG_H_APPEND(BRLCAD "#define HAVE_${var} 1\n")
    CONFIG_H_APPEND(BRLCAD "#define SIZEOF_${var} ${${testvar}}\n")
  endif(CONFIG_H_FILE AND ${testvar})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endmacro(BRLCAD_TYPE_SIZE)


###
# Check if a given structure has a specific member element.  Structure
# should be defined within the semicolon-separated header file list.
# Adds HAVE_* to config header and sets VAR.
###
macro(BRLCAD_STRUCT_MEMBER structname member headers var)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_STD_FLAG}")
  CHECK_STRUCT_HAS_MEMBER(${structname} ${member} "${headers}" HAVE_${var})
  if(CONFIG_H_FILE AND HAVE_${var})
    CONFIG_H_APPEND(BRLCAD "#define HAVE_${var} 1\n")
  endif(CONFIG_H_FILE AND HAVE_${var})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endmacro(BRLCAD_STRUCT_MEMBER)


###
# Check if a given function exists in the specified library.  Sets
# targetname_LINKOPT variable as advanced if found.
###
macro(BRLCAD_CHECK_LIBRARY targetname lname func)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_STD_FLAG}")

  if(NOT ${targetname}_LIBRARY)
    CHECK_LIBRARY_EXISTS(${lname} ${func} "" HAVE_${targetname}_${lname})
  else(NOT ${targetname}_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES_TMP ${CMAKE_REQUIRED_LIBRARIES})
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${${targetname}_LIBRARY})
    CHECK_FUNCTION_EXISTS(${func} HAVE_${targetname}_${lname})
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_TMP})
  endif(NOT ${targetname}_LIBRARY)

  if(HAVE_${targetname}_${lname})
    set(${targetname}_LIBRARY "${lname}")
  endif(HAVE_${targetname}_${lname})

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endmacro(BRLCAD_CHECK_LIBRARY lname func)


# Special purpose functions

###
# Undocumented.
###
function(BRLCAD_CHECK_BASENAME)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "")
  set(BASENAME_SRC "
#include <libgen.h>
int main(int argc, char *argv[]) {
(void)basename(argv[0]);
return 0;
}")
  if(NOT DEFINED HAVE_BASENAME)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${BASENAME_SRC}" HAVE_BASENAME)
    cmake_pop_check_state()
  endif(NOT DEFINED HAVE_BASENAME)
  if(HAVE_BASENAME)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_BASENAME 1\n")
  endif(HAVE_BASENAME)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endfunction(BRLCAD_CHECK_BASENAME var)

###
# Undocumented.
###
function(BRLCAD_CHECK_DIRNAME)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "")
  set(DIRNAME_SRC "
#include <libgen.h>
int main(int argc, char *argv[]) {
(void)dirname(argv[0]);
return 0;
}")
  if(NOT DEFINED HAVE_DIRNAME)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${DIRNAME_SRC}" HAVE_DIRNAME)
    cmake_pop_check_state()
  endif(NOT DEFINED HAVE_DIRNAME)
  if(HAVE_DIRNAME)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_DIRNAME 1\n")
  endif(HAVE_DIRNAME)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endfunction(BRLCAD_CHECK_DIRNAME var)

###
# Undocumented.
# Based on AC_HEADER_SYS_WAIT
###
function(BRLCAD_HEADER_SYS_WAIT)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "")
  set(SYS_WAIT_SRC "
#include <sys/types.h>
#include <sys/wait.h>
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned int) (stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
int main() {
  int s;
  wait(&s);
  s = WIFEXITED(s) ? WEXITSTATUS(s) : 1;
  return 0;
}")
  if(NOT DEFINED WORKING_SYS_WAIT)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${SYS_WAIT_SRC}" WORKING_SYS_WAIT)
    cmake_pop_check_state()
  endif(NOT DEFINED WORKING_SYS_WAIT)
  if(WORKING_SYS_WAIT)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_SYS_WAIT_H 1\n")
  endif(WORKING_SYS_WAIT)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endfunction(BRLCAD_HEADER_SYS_WAIT)

###
# Undocumented.
# Based on AC_FUNC_ALLOCA
###
function(BRLCAD_ALLOCA)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "")
  set(ALLOCA_HEADER_SRC "
#include <alloca.h>
int main() {
   char *p = (char *) alloca (2 * sizeof (int));
   if (p) return 0;
   ;
   return 0;
}")
  set(ALLOCA_TEST_SRC "
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# ifdef _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# else
#  ifdef HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   ifdef _AIX
 #pragma alloca
#   else
#    ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#    endif
#   endif
#  endif
# endif
#endif

int main() {
   char *p = (char *) alloca (1);
   if (p) return 0;
   ;
   return 0;
}")
  if(WORKING_ALLOC_H STREQUAL "")
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${ALLOCA_HEADER_SRC}" WORKING_ALLOCA_H)
    cmake_pop_check_state()
    set(WORKING_ALLOCA_H ${WORKING_ALLOCA_H} CACHE INTERNAL "alloca_h test")
  endif(WORKING_ALLOC_H STREQUAL "")
  if(WORKING_ALLOCA_H)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_ALLOCA_H 1\n")
    set(FILE_RUN_DEFINITIONS "-DHAVE_ALLOCA_H")
  endif(WORKING_ALLOCA_H)
  if(NOT DEFINED WORKING_ALLOCA)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${ALLOCA_TEST_SRC}" WORKING_ALLOCA)
    cmake_pop_check_state()
  endif(NOT DEFINED WORKING_ALLOCA)
  if(WORKING_ALLOCA)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_ALLOCA 1\n")
  endif(WORKING_ALLOCA)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endfunction(BRLCAD_ALLOCA)

###
# See if the compiler supports the C99 %z print specifier for size_t.
# Sets -DHAVE_STDINT_H=1 as global preprocessor flag if found.
###
function(BRLCAD_CHECK_C99_FORMAT_SPECIFIERS)
  set(CMAKE_C_FLAGS_TMP "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "")
  set(CMAKE_REQUIRED_DEFINITIONS_BAK ${CMAKE_REQUIRED_DEFINITIONS})
  CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
  if(HAVE_STDINT_H)
    set(CMAKE_REQUIRED_DEFINITIONS "-DHAVE_STDINT_H=1")
  endif(HAVE_STDINT_H)
  set(CHECK_C99_FORMAT_SPECIFIERS_SRC "
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
int main(int ac, char *av[])
{
  char buf[64] = {0};
  if (sprintf(buf, \"%zu\", (size_t)1234) != 4)
    return 1;
  else if (strcmp(buf, \"1234\"))
    return 2;
  return 0;
}
")
  if(NOT DEFINED HAVE_C99_FORMAT_SPECIFIERS)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${CHECK_C99_FORMAT_SPECIFIERS_SRC}" HAVE_C99_FORMAT_SPECIFIERS)
    cmake_pop_check_state()
  endif(NOT DEFINED HAVE_C99_FORMAT_SPECIFIERS)
  if(HAVE_C99_FORMAT_SPECIFIERS)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_C99_FORMAT_SPECIFIERS 1\n")
  endif(HAVE_C99_FORMAT_SPECIFIERS)
  set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_BAK})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_TMP}")
endfunction(BRLCAD_CHECK_C99_FORMAT_SPECIFIERS)


function(BRLCAD_CHECK_STATIC_ARRAYS)
  set(CHECK_STATIC_ARRAYS_SRC "
#include <stdio.h>
#include <string.h>

int foobar(char arg[static 100])
{
  return (int)arg[0];
}

int main(int ac, char *av[])
{
  char hello[100];

  if (ac > 0 && av)
    foobar(hello);
  return 0;
}
")
  if(NOT DEFINED HAVE_STATIC_ARRAYS)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_STD_FLAG}")
    CHECK_C_SOURCE_RUNS("${CHECK_STATIC_ARRAYS_SRC}" HAVE_STATIC_ARRAYS)
    cmake_pop_check_state()
  endif(NOT DEFINED HAVE_STATIC_ARRAYS)
  if(HAVE_STATIC_ARRAYS)
    CONFIG_H_APPEND(BRLCAD "#define HAVE_STATIC_ARRAYS 1\n")
  endif(HAVE_STATIC_ARRAYS)
endfunction(BRLCAD_CHECK_STATIC_ARRAYS)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
