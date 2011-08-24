# -fast provokes a stack corruption in the shadow computations because
# of strict aliasing getting enabled.  we _require_
# -fno-strict-aliasing until someone changes how lists are managed.
IF(${BRLCAD_OPTIMIZED_BUILD} STREQUAL "ON")
	CHECK_C_FLAG_GATHER(O3 OPTIMIZE_FLAGS)
	#CHECK_C_FLAG_GATHER(ffast-math OPTIMIZE_FLAGS)
	CHECK_C_FLAG_GATHER(fstrength-reduce OPTIMIZE_FLAGS)
	CHECK_C_FLAG_GATHER(fexpensive-optimizations OPTIMIZE_FLAGS)
	CHECK_C_FLAG_GATHER(finline-functions OPTIMIZE_FLAGS)
	CHECK_C_FLAG_GATHER("finline-limit=10000" OPTIMIZE_FLAGS)
	IF(NOT ${CMAKE_BUILD_TYPE} MATCHES "^Debug$" AND NOT BRLCAD-ENABLE_DEBUG AND NOT BRLCAD-ENABLE_PROFILING)
		CHECK_C_FLAG_GATHER(fomit-frame-pointer OPTIMIZE_FLAGS)
	ELSE(NOT ${CMAKE_BUILD_TYPE} MATCHES "^Debug$" AND NOT BRLCAD-ENABLE_DEBUG AND NOT BRLCAD-ENABLE_PROFILING)
		CHECK_C_FLAG_GATHER(fno-omit-frame-pointer OPTIMIZE_FLAGS)
	ENDIF(NOT ${CMAKE_BUILD_TYPE} MATCHES "^Debug$" AND NOT BRLCAD-ENABLE_DEBUG AND NOT BRLCAD-ENABLE_PROFILING)
	ADD_NEW_FLAG(C OPTIMIZE_FLAGS)
	ADD_NEW_FLAG(CXX OPTIMIZE_FLAGS)
ENDIF(${BRLCAD_OPTIMIZED_BUILD} STREQUAL "ON")
MARK_AS_ADVANCED(OPTIMIZE_FLAGS)
#need to strip out non-debug-compat flags after the fact based on build type, or do something else
#that will restore them if build type changes

# verbose warning flags
IF(BRLCAD-ENABLE_COMPILER_WARNINGS OR BRLCAD-ENABLE_STRICT)
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
ENDIF(BRLCAD-ENABLE_COMPILER_WARNINGS OR BRLCAD-ENABLE_STRICT)
MARK_AS_ADVANCED(WARNING_FLAGS)

# There are some specific situations, such as auto-generated sources,
# where we have no real control over the code generating the warnings
# and need to disable them individually.  Test for those flags.
CHECK_C_FLAG(Wno-unused)

IF(BRLCAD-ENABLE_STRICT)
	CHECK_C_FLAG_GATHER(Werror STRICT_FLAGS)
	ADD_NEW_FLAG(C STRICT_FLAGS)
	ADD_NEW_FLAG(CXX STRICT_FLAGS)
ENDIF(BRLCAD-ENABLE_STRICT)
MARK_AS_ADVANCED(STRICT_FLAGS)

