#      B R L C A D _ C O M P I L E R F L A G S . C M A K E
# BRL-CAD
#
# Copyright (c) 2011-2012 United States Government as represented by
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
# -fast provokes a stack corruption in the shadow computations because
# of strict aliasing getting enabled.  we _require_
# -fno-strict-aliasing until someone changes how lists are managed.
# -fast-math results in non-IEEE floating point math among a handful
# of other optimizations that cause substantial error in ray tracing
# and tessellation (and probably more).

IF(${BRLCAD_OPTIMIZED_BUILD} MATCHES "ON")
  CHECK_C_FLAG_GATHER(O3 OPTIMIZE_FLAGS)
  CHECK_C_FLAG_GATHER(fstrength-reduce OPTIMIZE_FLAGS)
  CHECK_C_FLAG_GATHER(fexpensive-optimizations OPTIMIZE_FLAGS)
  CHECK_C_FLAG_GATHER(finline-functions OPTIMIZE_FLAGS)
  CHECK_C_FLAG_GATHER("finline-limit=10000" OPTIMIZE_FLAGS)
  IF(NOT ${CMAKE_BUILD_TYPE} MATCHES "^Debug$" AND NOT BRLCAD_ENABLE_DEBUG AND NOT BRLCAD_ENABLE_PROFILING)
    CHECK_C_FLAG_GATHER(fomit-frame-pointer OPTIMIZE_FLAGS)
  ELSE(NOT ${CMAKE_BUILD_TYPE} MATCHES "^Debug$" AND NOT BRLCAD_ENABLE_DEBUG AND NOT BRLCAD_ENABLE_PROFILING)
    CHECK_C_FLAG_GATHER(fno-omit-frame-pointer OPTIMIZE_FLAGS)
  ENDIF(NOT ${CMAKE_BUILD_TYPE} MATCHES "^Debug$" AND NOT BRLCAD_ENABLE_DEBUG AND NOT BRLCAD_ENABLE_PROFILING)
  ADD_NEW_FLAG(C OPTIMIZE_FLAGS)
  ADD_NEW_FLAG(CXX OPTIMIZE_FLAGS)
ENDIF(${BRLCAD_OPTIMIZED_BUILD} MATCHES "ON")
MARK_AS_ADVANCED(OPTIMIZE_FLAGS)
#need to strip out non-debug-compat flags after the fact based on build type, or do something else
#that will restore them if build type changes

# verbose warning flags
IF(BRLCAD_ENABLE_COMPILER_WARNINGS OR BRLCAD_ENABLE_STRICT)
  # also of interest:
  # -Wunreachable-code -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -ansi
  # -Wformat=2 (after bu_fopen_uniq() is obsolete)
  CHECK_C_FLAG_GATHER(pedantic WARNING_FLAGS)
  # The Wall warnings are too verbose with Visual C++
  IF(NOT MSVC)
    CHECK_C_FLAG_GATHER(Wall WARNING_FLAGS)
  ELSE(NOT MSVC)
    CHECK_C_FLAG_GATHER(W4 WARNING_FLAGS)
  ENDIF(NOT MSVC)
  CHECK_C_FLAG_GATHER(Wextra WARNING_FLAGS)
  CHECK_C_FLAG_GATHER(Wundef WARNING_FLAGS)
  CHECK_C_FLAG_GATHER(Wfloat-equal WARNING_FLAGS)
  CHECK_C_FLAG_GATHER(Wshadow WARNING_FLAGS)
  CHECK_C_FLAG_GATHER(Winline WARNING_FLAGS)
  # Need this for tcl.h
  CHECK_C_FLAG_GATHER(Wno-long-long WARNING_FLAGS) 
  ADD_NEW_FLAG(C WARNING_FLAGS)
  ADD_NEW_FLAG(CXX WARNING_FLAGS)
ENDIF(BRLCAD_ENABLE_COMPILER_WARNINGS OR BRLCAD_ENABLE_STRICT)
MARK_AS_ADVANCED(WARNING_FLAGS)

IF(BRLCAD_ENABLE_STRICT)
  CHECK_C_FLAG_GATHER(Werror STRICT_FLAGS)
  ADD_NEW_FLAG(C STRICT_FLAGS)
  ADD_NEW_FLAG(CXX STRICT_FLAGS)
ENDIF(BRLCAD_ENABLE_STRICT)
MARK_AS_ADVANCED(STRICT_FLAGS)

SET(CMAKE_C_FLAGS_${BUILD_TYPE} "${CMAKE_C_FLAGS_${BUILD_TYPE}}" CACHE STRING "Make sure c flags make it into the cache" FORCE)
SET(CMAKE_CXX_FLAGS_${BUILD_TYPE} "${CMAKE_CXX_FLAGS_${BUILD_TYPE}}" CACHE STRING "Make sure c++ flags make it into the cache" FORCE)
SET(CMAKE_SHARED_LINKER_FLAGS_${BUILD_TYPE} ${CMAKE_SHARED_LINKER_FLAGS_${BUILD_TYPE}} CACHE STRING "Make sure shared linker flags make it into the cache" FORCE)
SET(CMAKE_EXE_LINKER_FLAGS_${BUILD_TYPE} ${CMAKE_EXE_LINKER_FLAGS_${BUILD_TYPE}} CACHE STRING "Make sure exe linker flags make it into the cache" FORCE)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
