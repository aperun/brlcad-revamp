# - Find lex executable and provides a macro to generate custom build rules
#
# The module defines the following variables:
#  LEX_FOUND - true is lex executable is found
#  LEX_EXECUTABLE - the path to the lex executable
#  LEX_LIBRARIES - The lex libraries
#
# If lex is found on the system, the module provides the macro:
#  LEX_TARGET(Name FlexInput FlexOutput [COMPILE_FLAGS <string>])
# which creates a custom command  to generate the <FlexOutput> file from
# the <FlexInput> file.  If  COMPILE_FLAGS option is specified, the next
# parameter is added to the lex  command line. Name is an alias used to
# get  details of  this custom  command.  Indeed the  macro defines  the
# following variables:
#  LEX_${Name}_DEFINED - true is the macro ran successfully
#  LEX_${Name}_OUTPUTS - the source file generated by the custom rule, an
#  alias for FlexOutput
#  LEX_${Name}_INPUT - the lex source file, an alias for ${FlexInput}
#
#  ====================================================================
#  Example:
#
#   find_package(LEX)
#
#   LEX_TARGET(MyScanner lexer.l  lexer.cpp)
#
#   include_directories("${CMAKE_CURRENT_BINARY_DIR}")
#   add_executable(Foo
#      Foo.cc
#      ${LEX_MyScanner_OUTPUTS}
#   )
#  ====================================================================
#
#=============================================================================
# Copyright (c) 2010-2018 United States Government as represented by
#                the U.S. Army Research Laboratory.
# Copyright 2009 Kitware, Inc.
# Copyright 2006 Tristan Carel
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * The names of the authors may not be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

#Need to run a test lex file to determine if YYTEXT_POINTER needs
#to be defined
function(yytext_pointer_test)
  set(LEX_TEST_SRCS "
%option noyywrap
%%
a { ECHO; }
b { REJECT; }
c { yymore (); }
d { yyless (1); }
e { yyless (input () != 0); }
f { unput (yytext[0]); }
. { BEGIN INITIAL; }
%%
#ifdef YYTEXT_POINTER
extern char *yytext;
#endif
extern int yyparse();
int main (void)
{
  char test_str[] = \"BRL-CAD\";
  YY_BUFFER_STATE buffer = yy_scan_string(test_str);
  yylex();
  yy_delete_buffer(buffer);
  return 0;
}

")
  if(NOT DEFINED YYTEXT_POINTER)
    file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/lex_test.l" "${LEX_TEST_SRCS}")
    execute_process(COMMAND ${LEX_EXECUTABLE} -o "${CMAKE_BINARY_DIR}/CMakeTmp/lex_test.c" "${CMAKE_BINARY_DIR}/CMakeTmp/lex_test.l" RESULT_VARIABLE _retval OUTPUT_VARIABLE _lexOut)

    try_run(YYTEXT_POINTER_RAN YYTEXT_POINTER_COMPILED
      "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/CMakeTmp/lex_test.c"
      COMPILE_DEFINITIONS "-DYYTEXT_POINTER=1"
      COMPILE_OUTPUT_VARIABLE COUTPUT
      RUN_OUTPUT_VARIABLE ROUTPUT)
    #message("COUTPUT: ${COUTPUT}")
    #message("ROUTPUT: ${ROUTPUT}")
    if(YYTEXT_POINTER_COMPILED AND NOT YYTEXT_POINTER_RAN)
      set(YYTEXT_POINTER 1 PARENT_SCOPE)
      if(CONFIG_H_FILE)
	CONFIG_H_APPEND(${CMAKE_CURRENT_PROJECT} "#define YYTEXT_POINTER 1\n")
      endif(CONFIG_H_FILE)
    else(YYTEXT_POINTER_COMPILED AND NOT YYTEXT_POINTER_RAN)
      set(YYTEXT_POINTER 0 PARENT_SCOPE)
    endif(YYTEXT_POINTER_COMPILED AND NOT YYTEXT_POINTER_RAN)

    file(REMOVE "${CMAKE_BINARY_DIR}/CMakeTmp/lex_test.c")
    file(REMOVE "${CMAKE_BINARY_DIR}/CMakeTmp/lex_test.l")
  endif(NOT DEFINED YYTEXT_POINTER)
endfunction(yytext_pointer_test)

find_program(LEX_EXECUTABLE flex DOC "path to the lex executable")
if(NOT LEX_EXECUTABLE)
  find_program(LEX_EXECUTABLE lex DOC "path to the lex executable")
endif(NOT LEX_EXECUTABLE)
mark_as_advanced(LEX_EXECUTABLE)

find_library(FL_LIBRARY NAMES fl DOC "path to the fl library")
mark_as_advanced(FL_LIBRARY)
set(LEX_LIBRARIES ${FL_LIBRARY})

if(LEX_EXECUTABLE)

  #============================================================
  # LEX_TARGET (public macro)
  #============================================================
  #
  macro(LEX_TARGET Name Input Output)
    set(LEX_TARGET_usage "LEX_TARGET(<Name> <Input> <Output> [COMPILE_FLAGS <string>]")
    if(${ARGC} GREATER 3)
      if(${ARGC} EQUAL 5)
        if("${ARGV3}" STREQUAL "COMPILE_FLAGS")
          set(LEX_EXECUTABLE_opts  "${ARGV4}")
          SEPARATE_ARGUMENTS(LEX_EXECUTABLE_opts)
        else()
          message(SEND_ERROR ${LEX_TARGET_usage})
        endif()
      else()
        message(SEND_ERROR ${LEX_TARGET_usage})
      endif()
    endif()

    add_custom_command(OUTPUT ${Output}
      COMMAND ${LEX_EXECUTABLE}
      ARGS ${LEX_EXECUTABLE_opts} -o${Output} ${Input}
      DEPENDS ${Input}
      COMMENT "[LEX][${Name}] Building scanner with ${LEX_EXECUTABLE}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    set(LEX_${Name}_DEFINED TRUE)
    set(LEX_${Name}_OUTPUTS ${Output})
    set(LEX_${Name}_INPUT ${Input})
    set(LEX_${Name}_COMPILE_FLAGS ${LEX_EXECUTABLE_opts})
  endmacro(LEX_TARGET)
  #============================================================

  # Execute the YYTEXT pointer test defined above
  yytext_pointer_test()

endif(LEX_EXECUTABLE)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LEX DEFAULT_MSG LEX_EXECUTABLE)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
