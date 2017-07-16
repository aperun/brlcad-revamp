#               B R L C A D _ U T I L . C M A K E
# BRL-CAD
#
# Copyright (c) 2011-2016 United States Government as represented by
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
#-----------------------------------------------------------------------------
# Pretty-printing macro that generates a box around a string and prints the
# resulting message.
function(BOX_PRINT input_string border_string)
  string(LENGTH ${input_string} MESSAGE_LENGTH)
  string(LENGTH ${border_string} SEPARATOR_STRING_LENGTH)
  while(${MESSAGE_LENGTH} GREATER ${SEPARATOR_STRING_LENGTH})
    set(SEPARATOR_STRING "${SEPARATOR_STRING}${border_string}")
    string(LENGTH ${SEPARATOR_STRING} SEPARATOR_STRING_LENGTH)
  endwhile(${MESSAGE_LENGTH} GREATER ${SEPARATOR_STRING_LENGTH})
  message("${SEPARATOR_STRING}")
  message("${input_string}")
  message("${SEPARATOR_STRING}")
endfunction()


#-----------------------------------------------------------------------------
# It is sometimes convenient to be able to supply both a filename and a
# variable name containing a list of files to a single macro.
# This routine handles both forms of input - separate variables are
# used to indicate which variable names are supposed to contain the
# initial list contents and the full path version of that list.  Thus,
# macros using the normalize macro get the list in a known variable and
# can use it reliably, regardless of whether inlist contained the actual
# list contents or a variable.
macro(NORMALIZE_FILE_LIST inlist targetvar fullpath_targetvar)

  # First, figure out whether we have list contents or a list name
  set(havevarname 0)
  foreach(maybefilename ${inlist})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${maybefilename}")
      set(havevarname 1)
    endif(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${maybefilename}")
  endforeach(maybefilename ${${targetvar}})

  # Put the list contents in the targetvar variable and
  # generate a target name.
  if(NOT havevarname)

    set(${targetvar} "${inlist}")

    # Initial clean-up
    string(REGEX REPLACE " " "_" targetstr "${inlist}")
    string(REGEX REPLACE "/" "_" targetstr "${targetstr}")
    string(REGEX REPLACE "\\." "_" targetstr "${targetstr}")

    # For situations like file copying, where we sometimes need to autogenerate
    # target names, it is important to make sure we can avoid generating absurdly
    # long names.  To do this, we run candidate names through a length filter
    # and use their MD5 hash if they are longer than 30 characters.
    # It's cryptic but the odds are very good the result will be a unique
    # target name and the string will be short enough, which is what we need.
    string(LENGTH "${targetstr}" STRLEN)
    if ("${STRLEN}" GREATER 30)
      string(MD5 targetname "${targetstr}")
    else ("${STRLEN}" GREATER 30)
      set(targetname "${targetstr}")
    endif ("${STRLEN}" GREATER 30)

  else(NOT havevarname)

    set(${targetvar} "${${inlist}}")
    set(targetname "${inlist}")

  endif(NOT havevarname)

  # Mark the inputs as files to ignore in distcheck
  CMAKEFILES(${${targetvar}})

  # For some uses, we need the contents of the input list
  # with full paths.  Generate a list that we're sure has
  # full paths, and return that to the second variable.
  set(${fullpath_targetvar} "")
  foreach(filename ${${targetvar}})
    get_filename_component(file_fullpath "${filename}" ABSOLUTE)
    set(${fullpath_targetvar} ${${fullpath_targetvar}} "${file_fullpath}")
  endforeach(filename ${${targetvar}})

  # Some macros will also want a valid build target name
  # based on the input - if a third input parameter has
  # been supplied, return the target name using it.
  if(NOT "${ARGV3}" STREQUAL "")
    set(${ARGV3} "${targetname}")
  endif(NOT "${ARGV3}" STREQUAL "")

endmacro(NORMALIZE_FILE_LIST)

#-----------------------------------------------------------------------------
# It is sometimes necessary for build logic to be aware of all instances
# of a certain category of target that have been defined for a particular
# build directory - for example, the pkgIndex.tcl generation targets need
# to ensure that all data copying targets have done their work before they
# generate their indexes.  To support this, macros are define that allow
# globally available lists to be defined, maintained and accessed.  We use
# this approach instead of directory properties because CMake's documentation
# seems to indicate that directory properties also apply to subdirectories,
# and we want these lists to be associated with one and only one directory.
macro(BRLCAD_ADD_DIR_LIST_ENTRY list_name dir_in list_entry)
  string(REGEX REPLACE "/" "_" currdir_str ${dir_in})
  string(TOUPPER "${currdir_str}" currdir_str)
  get_property(${list_name}_${currdir_str} GLOBAL PROPERTY DATA_TARGETS_${currdir_str})
  if(NOT ${list_name}_${currdir_str})
    define_property(GLOBAL PROPERTY CMAKE_LIBRARY_TARGET_LIST BRIEF_DOCS "${list_name}" FULL_DOCS "${list_name} for directory ${dir_in}")
  endif(NOT ${list_name}_${currdir_str})
  set_property(GLOBAL APPEND PROPERTY ${list_name}_${currdir_str} ${list_entry})
endmacro(BRLCAD_ADD_DIR_LIST_ENTRY)

macro(BRLCAD_GET_DIR_LIST_CONTENTS list_name dir_in outvar)
  string(REGEX REPLACE "/" "_" currdir_str ${dir_in})
  string(TOUPPER "${currdir_str}" currdir_str)
  get_property(${list_name}_${currdir_str} GLOBAL PROPERTY ${list_name}_${currdir_str})
  set(${outvar} "${DATA_TARGETS_${currdir_str}}")
endmacro(BRLCAD_GET_DIR_LIST_CONTENTS)


#-----------------------------------------------------------------------------
# We need a way to tell whether one path is a subpath of another path without
# relying on regular expressions, since file paths may have characters in them
# that will trigger regex matching behavior when we don't want it.  (To test,
# for example, use a build directory name of build++)
#
# Sets ${result_var} to 1 if the candidate subpath is actually a subpath of
# the supplied "full" path, otherwise sets it to 0.
#
# The routine below does the check without using regex matching, in order to
# handle path names that contain characters that would be interpreted as active
# in a regex string.
function(IS_SUBPATH in_candidate_subpath in_full_path result_var)
  # Convert paths to lists of directories - regex based
  # matching won't work reliably, so instead look at each
  # element compared to its corresponding element in the
  # other path using string comparison.

  # get the CMake form of the path so we have something consistent
  # to work on
  file(TO_CMAKE_PATH "${in_full_path}" full_path)
  file(TO_CMAKE_PATH "${in_candidate_subpath}" candidate_subpath)

  # check the string lengths - if the "subpath" is longer
  # than the full path, there's not point in going further
  string(LENGTH "${full_path}" FULL_LENGTH)
  string(LENGTH "${candidate_subpath}" SUB_LENGTH)
  if("${SUB_LENGTH}" GREATER "${FULL_LENGTH}")
    set(${result_var} 0 PARENT_SCOPE)
  else("${SUB_LENGTH}" GREATER "${FULL_LENGTH}")
    # OK, maybe it's a subpath - time to actually check
    string(REPLACE "/" ";" full_path_list "${full_path}")
    string(REPLACE "/" ";" candidate_subpath_list "${candidate_subpath}")
    set(found_difference 0)
    while(NOT found_difference AND candidate_subpath_list)
      list(GET full_path_list 0 full_path_element)
      list(GET candidate_subpath_list 0 subpath_element)
      if("${full_path_element}" STREQUAL "${subpath_element}")
	list(REMOVE_AT full_path_list 0)
	list(REMOVE_AT candidate_subpath_list 0)
      else("${full_path_element}" STREQUAL "${subpath_element}")
	set(found_difference 1)
      endif("${full_path_element}" STREQUAL "${subpath_element}")
    endwhile(NOT found_difference AND candidate_subpath_list)
    # Now we know - report the result
    if(NOT found_difference)
      set(${result_var} 1 PARENT_SCOPE)
    else(NOT found_difference)
      set(${result_var} 0 PARENT_SCOPE)
    endif(NOT found_difference)
  endif("${SUB_LENGTH}" GREATER "${FULL_LENGTH}")
endfunction(IS_SUBPATH)

#-----------------------------------------------------------------------------
# Determine whether a list of source files contains all C, all C++, or
# mixed source types.
function(SRCS_LANG sourceslist resultvar targetname)
  # Check whether we have a mixed C/C++ library or just a single language.
  # If the former, different compilation flag management is needed.
  set(has_C 0)
  set(has_CXX 0)
  foreach(srcfile ${sourceslist})
    get_property(file_language SOURCE ${srcfile} PROPERTY LANGUAGE)
    if(NOT file_language)
      get_filename_component(srcfile_ext ${srcfile} EXT)
      if(${srcfile_ext} MATCHES ".cxx$" OR ${srcfile_ext} MATCHES ".cpp$" OR ${srcfile_ext} MATCHES ".cc$")
        set(has_CXX 1)
        set(file_language CXX)
      elseif(${srcfile_ext} STREQUAL ".c")
        set(has_C 1)
        set(file_language C)
      endif(${srcfile_ext} MATCHES ".cxx$" OR ${srcfile_ext} MATCHES ".cpp$" OR ${srcfile_ext} MATCHES ".cc$")
    endif(NOT file_language)
    if(NOT file_language)
      message("WARNING - file ${srcfile} listed in the ${targetname} sources list does not appear to be a C or C++ file.")
    endif(NOT file_language)
  endforeach(srcfile ${sourceslist})
  set(${resultvar} "UNKNOWN" PARENT_SCOPE)
  if(has_C AND has_CXX)
    set(${resultvar} "MIXED" PARENT_SCOPE)
  elseif(has_C AND NOT has_CXX)
    set(${resultvar} "C" PARENT_SCOPE)
  elseif(NOT has_C AND has_CXX)
    set(${resultvar} "CXX" PARENT_SCOPE)
  endif(has_C AND has_CXX)
endfunction(SRCS_LANG)

#---------------------------------------------------------------------------
# Add dependencies to a target, but only if they are defined as targets in
# CMake
function(ADD_TARGET_DEPS tname)
  if(TARGET ${tname})
    foreach(target ${ARGN})
      if(TARGET ${target})
	add_dependencies(${tname} ${target})
      endif(TARGET ${target})
    endforeach(target ${ARGN})
  endif(TARGET ${tname})
endfunction(ADD_TARGET_DEPS tname)

#---------------------------------------------------------------------------
# Write out an execution script to run commands with the necessary
# variables set to allow execution in the build directory, even if
# there are installed libraries present in the final installation
# directory.
function(generate_cmd_script cmd_exe script_file)

  cmake_parse_arguments(GCS "" "OLOG;ELOG" "CARGS" ${ARGN})

  # Initialize file 
  file(WRITE "${script_file}" "# Script to run ${cmd_exe}\n")

  # Handle multiconfig (must be run-time determination for Visual Studio and XCode)
  # TODO - logic writing this trick needs to become some sort of standard routine...
  file(APPEND "${script_file}" "if(EXISTS \"${CMAKE_BINARY_DIR}/CMakeTmp/CURRENT_PATH/Release\")\n")
  file(APPEND "${script_file}" "  set(CBDIR \"${CMAKE_BINARY_DIR}/Release\")\n")
  file(APPEND "${script_file}" "elseif(EXISTS \"${CMAKE_BINARY_DIR}/CMakeTmp/CURRENT_PATH/Debug\")\n")
  file(APPEND "${script_file}" "  set(CBDIR \"${CMAKE_BINARY_DIR}/Debug\")\n")
  file(APPEND "${script_file}" "else(EXISTS \"${CMAKE_BINARY_DIR}/CMakeTmp/CURRENT_PATH/Release\")\n")
  file(APPEND "${script_file}" "  set(CBDIR \"${CMAKE_BINARY_DIR}\")\n")
  file(APPEND "${script_file}" "endif(EXISTS \"${CMAKE_BINARY_DIR}/CMakeTmp/CURRENT_PATH/Release\")\n")

  # BRLCAD_ROOT is the hammer that makes certain we are running
  # things found in the build directory
  file(APPEND "${script_file}" "set(ENV{BRLCAD_ROOT} \"\${CBDIR}\")\n")

  # Substitute in the correct binary path anywhere it is needed in the args
  file(APPEND "${script_file}" "string(REPLACE \"CURRENT_BUILD_DIR\" \"\${CBDIR}\" FIXED_CMD_ARGS \"${GCS_CARGS}\")\n")

  # Use the CMake executable to figure out if we need an extension
  get_filename_component(EXE_EXT "${CMAKE_COMMAND}" EXT)

  # Write the actual cmake command to run the process
  file(APPEND "${script_file}" "execute_process(COMMAND \"\${CBDIR}/${BIN_DIR}/${cmd_exe}${EXE_EXT}\" \${FIXED_CMD_ARGS} RESULT_VARIABLE CR OUTPUT_VARIABLE CO ERROR_VARIABLE CE)\n")

  # Log the outputs, if we are supposed to do that
  if(GCS_OLOG)
    file(APPEND "${script_file}" "file(APPEND \"${GCS_OLOG}\" \"\${CO}\")\n")
  endif(GCS_OLOG)
  if(GCS_ELOG)
    file(APPEND "${script_file}" "file(APPEND \"${GCS_ELOG}\" \"\${CE}\")\n")
  endif(GCS_ELOG)

  # Fail the command if the result was non-zero
  file(APPEND "${script_file}" "if(CR)\n")
  file(APPEND "${script_file}" "  message(FATAL_ERROR \"\${CBDIR}/${BIN_DIR}/${cmd_exe}${EXE_EXT} failure: \${CR}\\n\${CO}\\n\${CE}\")\n")
  file(APPEND "${script_file}" "endif(CR)\n")

  # If we are doing distclean, let CMake know to remove the script and log files
  if(COMMAND distclean)
    set(distclean_files "${script_file}" "${GCS_OLOG}" "${GCS_ELOG}")
    list(REMOVE_DUPLICATES distclean_files)
    distclean(${distclean_files})
  endif(COMMAND distclean)

endfunction(generate_cmd_script)

#---------------------------------------------------------------------------
# Set variables reporting time of configuration.  Sets CONFIG_DATE and
# CONFIG_DATESTAMP in parent scope.
#
# Unfortunately, CMake doesn't give you variables with current day,
# month, etc.  There are several possible approaches to this, but most
# (e.g. the date command) are not cross platform. We build a small C
# program which supplies the needed info.
function(set_config_time)

  # We don't want any parent C flags here - hopefully, the below code will
  # "just work" in any environment we are about.  The gnu89 standard doesn't
  # like the %z print specifier, but unless/until we hit a compiler that really
  # can't deal with it don't worry about it.
  set(CMAKE_C_FLAGS "")

  set(rfc2822_src "
/* RFC2822 timestamp and individual day, month and year files */

#include <time.h>
#include <stdio.h>
#include <string.h>

int main(int argc, const char **argv) {
   time_t t = time(NULL);
   struct tm *currenttime = localtime(&t);
   if (argc <= 1) {
      char rfc2822[200];
      strftime(rfc2822, sizeof(rfc2822), \"%a, %d %b %Y %H:%M:%S %z\", currenttime);
      printf(\"%s\", rfc2822);
      return 0;
   }
   if (strncmp(argv[1], \"day\", 4) == 0) {
      printf(\"%02d\", currenttime->tm_mday);
   }
   if (strncmp(argv[1], \"month\", 5) == 0) {
      printf(\"%02d\", currenttime->tm_mon + 1);
   }
   if (strncmp(argv[1], \"year\", 4) == 0) {
      printf(\"%d\", currenttime->tm_year + 1900);
   }
   return 0;
}")

  # Build the code so we can run it
  file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822.c" "${rfc2822_src}")
  try_compile(rfc2822_build "${CMAKE_BINARY_DIR}/CMakeTmp"
    SOURCES "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822.c"
    OUTPUT_VARIABLE RFC2822_BUILD_INFO
    COPY_FILE "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822")
  if(NOT rfc2822_build)
    message(FATAL_ERROR "Could not build rfc2822 timestamp pretty-printing utility: ${RFC2822_BUILD_INFO}")
  endif(NOT rfc2822_build)
  file(REMOVE "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822.c")

  # Build up and set CONFIG_DATE
  execute_process(COMMAND "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822" day OUTPUT_VARIABLE CONFIG_DAY)
  string(STRIP ${CONFIG_DAY} CONFIG_DAY)
  execute_process(COMMAND "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822" month OUTPUT_VARIABLE CONFIG_MONTH)
  string(STRIP ${CONFIG_MONTH} CONFIG_MONTH)
  execute_process(COMMAND "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822" year OUTPUT_VARIABLE CONFIG_YEAR)
  string(STRIP ${CONFIG_YEAR} CONFIG_YEAR)
  set(CONFIG_DATE "${CONFIG_YEAR}${CONFIG_MONTH}${CONFIG_DAY}" PARENT_SCOPE)

  # Set DATESTAMP
  execute_process(COMMAND "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822" OUTPUT_VARIABLE RFC2822_STRING)
  string(STRIP ${RFC2822_STRING} RFC2822_STRING)
  set(CONFIG_DATESTAMP "${RFC2822_STRING}" PARENT_SCOPE)

  # Cleanup
  file(REMOVE "${CMAKE_BINARY_DIR}/CMakeTmp/rfc2822")

endfunction(set_config_time)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
