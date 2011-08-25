# - Find lemon executable and provides macros to generate custom build rules
# The module defines the following variables
#
#  LEMON_EXECUTABLE - path to the lemon program
#  LEMON_FOUND - true if the program was found
#
# If lemon is found, the module defines the macros:
#  LEMON_TARGET(<Name> <LemonInput> <CodeOutput> [VERBOSE <file>]
#              [COMPILE_FLAGS <string>])
# which will create  a custom rule to generate  a parser. <LemonInput> is
# the path to  a lemon file. <CodeOutput> is the name  of the source file
# generated by lemon.  A header file is also  be generated, and contains
# the  token  list.  If  COMPILE_FLAGS  option is  specified,  the  next
# parameter is  added in the lemon command line.  if  VERBOSE option is
# specified, <file> is created  and contains verbose descriptions of the
# grammar and parser. The macro defines a set of variables:
#  LEMON_${Name}_DEFINED - true is the macro ran successfully
#  LEMON_${Name}_INPUT - The input source file, an alias for <LemonInput>
#  LEMON_${Name}_OUTPUT_SOURCE - The source file generated by lemon 
#  LEMON_${Name}_OUTPUT_HEADER - The header file generated by lemon
#  LEMON_${Name}_OUTPUTS - The sources files generated by lemon
#  LEMON_${Name}_COMPILE_FLAGS - Options used in the lemon command line
#
#  ====================================================================
#  Example:
#
#   find_package(LEMON)
#   LEMON_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
#   add_executable(Foo main.cpp ${LEMON_MyParser_OUTPUTS})
#  ====================================================================
#
#=============================================================================
# Copyright 2010 United States Government as represented by
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

FIND_PROGRAM(LEMON_EXECUTABLE lemon DOC "path to the lemon executable")
MARK_AS_ADVANCED(LEMON_EXECUTABLE)
IF(LEMON_EXECUTABLE)
	get_filename_component(lemon_path ${LEMON_EXECUTABLE} PATH)
	IF(lemon_path)
		SET(LEMON_TEMPLATE "")
		IF(EXISTS ${lemon_path}/lempar.c)
			SET(LEMON_TEMPLATE "${lemon_path}/lempar.c")
		ENDIF(EXISTS ${lemon_path}/lempar.c)
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(LEMON DEFAULT_MSG LEMON_EXECUTABLE LEMON_TEMPLATE)
	ENDIF(lemon_path)
ENDIF(LEMON_EXECUTABLE)
MARK_AS_ADVANCED(LEMON_TEMPLATE)

#============================================================
# LEMON_TARGET (public macro)
#============================================================
#
MACRO(LEMON_TARGET Name LemonInput LemonOutput)
	SET(LEMON_TARGET_outputs "${LemonOutput}")
	IF(NOT ${ARGC} EQUAL 3 AND NOT ${ARGC} EQUAL 4)
		MESSAGE(SEND_ERROR "Usage")
	ELSE()
		# lemon generates files in source dir, so we need to move them
		IF("${ARGV1}" MATCHES "yy$")
			STRING(REGEX REPLACE "yy$" "c" SRC_FILE "${ARGV1}")
			STRING(REGEX REPLACE "yy$" "h" HEADER_FILE "${ARGV1}")
			STRING(REGEX REPLACE "yy$" "out" OUT_FILE "${ARGV1}")
			STRING(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\2" _fileext "${ARGV2}")
			STRING(REPLACE "cpp" "hpp" _hfileext ${_fileext})
		ELSE("${ARGV1}" MATCHES "yy$")
			STRING(REGEX REPLACE "y$" "c" SRC_FILE "${ARGV1}")
			STRING(REGEX REPLACE "y$" "h" HEADER_FILE "${ARGV1}")
			STRING(REGEX REPLACE "y$" "out" OUT_FILE "${ARGV1}")
			STRING(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\2" _fileext "${ARGV2}")
			STRING(REPLACE "c" "h" _hfileext ${_fileext})
		ENDIF("${ARGV1}" MATCHES "yy$")
		STRING(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\1${_hfileext}" LEMON_${Name}_OUTPUT_HEADER "${ARGV2}")
		STRING(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\1.out" LEMON_${Name}_LOG "${ARGV2}")
		LIST(APPEND LEMON_TARGET_outputs "${LEMON_${Name}_OUTPUT_HEADER}")
		LIST(APPEND LEMON_TARGET_outputs "${LEMON_${Name}_LOG}")

		ADD_CUSTOM_COMMAND(OUTPUT ${LEMON_TARGET_outputs}
			COMMAND ${LEMON_EXECUTABLE} ${ARGV1} ${ARGV3}
			COMMAND ${CMAKE_COMMAND} -E rename ${HEADER_FILE} ${LEMON_${Name}_OUTPUT_HEADER}
			COMMAND ${CMAKE_COMMAND} -E rename ${SRC_FILE} ${ARGV2}
			COMMAND ${CMAKE_COMMAND} -E rename ${OUT_FILE} ${LEMON_${Name}_LOG}
			DEPENDS ${ARGV1}
			COMMENT "[LEMON][${Name}] Building parser with ${LEMON_EXECUTABLE}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

		# define target variables
		SET(LEMON_${Name}_DEFINED TRUE)
		SET(LEMON_${Name}_INPUT ${ARGV1})
		SET(LEMON_${Name}_OUTPUTS ${LEMON_TARGET_outputs})
		SET(LEMON_${Name}_COMPILE_FLAGS ${LEMON_TARGET_cmdopt})
		SET(LEMON_${Name}_OUTPUT_SOURCE "${LemonOutput}")
	ENDIF(NOT ${ARGC} EQUAL 3 AND NOT ${ARGC} EQUAL 4)
ENDMACRO(LEMON_TARGET)
#
#============================================================
# FindLEMON.cmake ends here
