# Checking compiler flags benefits from some macro logic

INCLUDE(CheckCCompilerFlag)

MACRO(CHECK_C_FLAG flag)
	STRING(TOUPPER ${flag} UPPER_FLAG)
	STRING(REGEX REPLACE " " "_" UPPER_FLAG ${UPPER_FLAG})
	STRING(REGEX REPLACE "=" "_" UPPER_FLAG ${UPPER_FLAG})
	IF(${ARGC} LESS 2)
		CHECK_C_COMPILER_FLAG(-${flag} ${UPPER_FLAG}_COMPILER_FLAG)
	ELSE(${ARGC} LESS 2)
		IF(NOT ${ARGV1})
			CHECK_C_COMPILER_FLAG(-${flag} ${UPPER_FLAG}_COMPILER_FLAG)
			IF(${UPPER_FLAG}_COMPILER_FLAG)
				MESSAGE("Found ${ARGV1} - setting to -${flag}")
				SET(${ARGV1} "-${flag}" CACHE STRING "${ARGV1}" FORCE)
			ENDIF(${UPPER_FLAG}_COMPILER_FLAG)
		ENDIF(NOT ${ARGV1})
	ENDIF(${ARGC} LESS 2)
	IF(${UPPER_FLAG}_COMPILER_FLAG)
		SET(${UPPER_FLAG}_COMPILER_FLAG "-${flag}")
	ENDIF(${UPPER_FLAG}_COMPILER_FLAG)
ENDMACRO()

MACRO(CHECK_C_FLAG_GATHER flag FLAGS)
	STRING(TOUPPER ${flag} UPPER_FLAG)
	STRING(REGEX REPLACE " " "_" UPPER_FLAG ${UPPER_FLAG})
	CHECK_C_COMPILER_FLAG(-${flag} ${UPPER_FLAG}_COMPILER_FLAG)
	IF(${UPPER_FLAG}_COMPILER_FLAG)
		SET(${FLAGS} "${${FLAGS}} -${flag}")
	ENDIF(${UPPER_FLAG}_COMPILER_FLAG)
ENDMACRO()


# Automate putting variables from tests into CFLAGS,
# and otherwise wrap check macros in extra logic as needed

INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckTypeSize)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckStructHasMember)
INCLUDE(CheckCSourceCompiles)
INCLUDE(ResolveCompilerPaths)

MACRO(CHECK_FUNCTION_EXISTS_D function var)
  CHECK_FUNCTION_EXISTS(${function} ${var})
  if(${var})
			 SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -D${var}=1" CACHE STRING "TCL CFLAGS" FORCE)
  endif(${var})
ENDMACRO(CHECK_FUNCTION_EXISTS_D)

MACRO(CHECK_INCLUDE_FILE_D filename var)
  CHECK_INCLUDE_FILE(${filename} ${var})
  if(${var})
			 SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -D${var}=1" CACHE STRING "TCL CFLAGS" FORCE)
  endif(${var})
ENDMACRO(CHECK_INCLUDE_FILE_D)

MACRO(CHECK_INCLUDE_FILE_USABILITY_D filename var)
	CHECK_INCLUDE_FILE_D(${filename} HAVE_${var})
	IF(HAVE_${var})
		SET(HEADER_SRC "
		#include <${filename}>
		main(){};
		")
		CHECK_C_SOURCE_COMPILES("${HEADER_SRC}" ${var}_USABLE)
	ENDIF(HAVE_${var})
	IF(NOT HAVE_${var} OR NOT ${var}_USABLE)
		SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -DNO_${var}=1" CACHE STRING "TCL CFLAGS" FORCE)
	ENDIF(NOT HAVE_${var} OR NOT ${var}_USABLE)
ENDMACRO(CHECK_INCLUDE_FILE_USABILITY_D filename var)

MACRO(CHECK_INCLUDE_FILE_CXX_D filename var)
  CHECK_INCLUDE_FILE_CXX(${filename} ${var})
  if(${var})
	 SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -D${var}=1" CACHE STRING "TCL CFLAGS" FORCE)
  endif(${var})
ENDMACRO(CHECK_INCLUDE_FILE_CXX_D)

MACRO(CHECK_TYPE_SIZE_D typename var)
	FOREACH(arg ${ARGN})
		SET(headers ${headers} ${arg})
	ENDFOREACH(arg ${ARGN})
	SET(CHECK_EXTRA_INCLUDE_FILES ${headers})
	CHECK_TYPE_SIZE(${typename} HAVE_${var}_T)
	SET(CHECK_EXTRA_INCLUDE_FILES)
	if(HAVE_${var}_T)
	   SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -DHAVE_${var}_T=1" CACHE STRING "TCL CFLAGS" FORCE)
		SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -DSIZEOF_${var}=${HAVE_${var}_T}" CACHE STRING "TCL CFLAGS" FORCE)
	endif(HAVE_${var}_T)
ENDMACRO(CHECK_TYPE_SIZE_D)

MACRO(CHECK_STRUCT_HAS_MEMBER_D structname member header var)
	CHECK_STRUCT_HAS_MEMBER(${structname} ${member} ${header} HAVE_${var})
	if(HAVE_${var})
		SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -DHAVE_${var}=1" CACHE STRING "TCL CFLAGS" FORCE)
	endif(HAVE_${var})
ENDMACRO(CHECK_STRUCT_HAS_MEMBER_D)

MACRO(CHECK_LIBRARY targetname lname func)
	IF(NOT ${targetname}_LIBRARY)
		CHECK_LIBRARY_EXISTS(${lname} ${func} "" HAVE_${targetname}_${lname})
		IF(HAVE_${targetname}_${lname})
			RESOLVE_LIBRARIES (${targetname}_LIBRARY "-l${lname}")
			SET(${targetname}_LINKOPT "-l${lname}")
		ENDIF(HAVE_${targetname}_${lname})
	ENDIF(NOT ${targetname}_LIBRARY)
ENDMACRO(CHECK_LIBRARY lname func)

