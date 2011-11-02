# Macro for three-way options that optionally check whether a system
# resource is available.
MACRO(TCL_BUNDLE_OPTION optname default_raw)
	STRING(TOUPPER "${default_raw}" default)
	IF(default)
		IF(${default} STREQUAL "ON")
			SET(DEFAULT "BUNDLED")
		ENDIF(${default} STREQUAL "ON")
		IF(${default} STREQUAL "OFF")
			SET(DEFAULT "SYSTEM")
		ENDIF(${default} STREQUAL "OFF")
	ENDIF(default)
	IF(NOT ${optname})
		IF(default)
			SET(${optname} "${default}" CACHE STRING "Build bundled ${optname} libraries.")
		ELSE(default)
			IF(${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "BUNDLED" OR ${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "AUTO (B)")
				SET(${optname} "AUTO (B)" CACHE STRING "Build bundled ${optname} libraries.")
			ENDIF(${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "BUNDLED" OR ${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "AUTO (B)")
			IF(${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "SYSTEM" OR ${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "AUTO (S)")
				SET(${optname} "AUTO (S)" CACHE STRING "Build bundled ${optname} libraries.")
			ENDIF(${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "SYSTEM" OR ${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "AUTO (S)")
			IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "AUTO")
				SET(${optname} "AUTO" CACHE STRING "Build bundled ${optname} libraries.")
			ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "AUTO")
		ENDIF(default)
	ENDIF(NOT ${optname})
	STRING(TOUPPER "${${optname}}" optname_upper)
	SET(${optname} ${optname_upper})
	IF(${optname} STREQUAL "AUTO (B)" OR ${optname} STREQUAL "AUTO (S)" OR ${optname} STREQUAL "AUTO")
		IF(${CMAKE_PROJECT_NAME}_TCL_BUILD)
			SET(${optname} "AUTO (B)" CACHE STRING "Build bundled ${optname} libraries." FORCE)
		ENDIF(${CMAKE_PROJECT_NAME}_TCL_BUILD)
		IF(${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "SYSTEM" OR ${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "AUTO (S)")
			SET(${optname} "AUTO (S)" CACHE STRING "Build bundled ${optname} libraries." FORCE)
		ENDIF(${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "SYSTEM" OR ${${CMAKE_PROJECT_NAME}_TCL} STREQUAL "AUTO (S)")
	ENDIF(${optname} STREQUAL "AUTO (B)" OR ${optname} STREQUAL "AUTO (S)" OR ${optname} STREQUAL "AUTO")
	set_property(CACHE ${optname} PROPERTY STRINGS AUTO BUNDLED SYSTEM)
	IF(NOT ${${optname}} STREQUAL "AUTO" AND NOT ${${optname}} STREQUAL "BUNDLED" AND NOT ${${optname}} STREQUAL "SYSTEM" AND NOT ${${optname}} STREQUAL "AUTO (B)" AND NOT ${${optname}} STREQUAL "AUTO (S)")
		MESSAGE(WARNING "Unknown value ${${optname}} supplied for ${optname} - defaulting to AUTO")
		MESSAGE(WARNING "Valid options are AUTO, BUNDLED and SYSTEM")
		SET(${optname} "AUTO" CACHE STRING "Build bundled libraries." FORCE)
	ENDIF(NOT ${${optname}} STREQUAL "AUTO" AND NOT ${${optname}} STREQUAL "BUNDLED" AND NOT ${${optname}} STREQUAL "SYSTEM" AND NOT ${${optname}} STREQUAL "AUTO (B)" AND NOT ${${optname}} STREQUAL "AUTO (S)")
ENDMACRO()


MACRO(THIRD_PARTY_TCL_PACKAGE packagename dir wishcmd depends)
	STRING(TOUPPER ${packagename} PKGNAME_UPPER)
	# If we are doing a local Tcl build, default to bundled.
   TCL_BUNDLE_OPTION(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} "") 
	IF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "BUNDLED" OR ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "AUTO (B)")
		SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD ON)
	ELSE(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "BUNDLED" OR ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "AUTO (B)")
		SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD OFF)
		# Stash the previous results (if any) so we don't repeatedly call out the tests - only report
		# if something actually changes in subsequent runs.
		SET(${PKGNAME_UPPER}_FOUND_STATUS ${${PKGNAME_UPPER}_FOUND})
		IF(${wishcmd} STREQUAL "")
			SET(${PKGNAME_UPPER}_FOUND "${PKGNAME_UPPER}-NOTFOUND" CACHE STRING "${PKGNAME_UPPER}_FOUND" FORCE)
			IF(NOT ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "SYSTEM" AND NOT ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "AUTO (S)")
				# Can't test and we're not forced to system by either local settings or the toplevel - turn it on 
				SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD ON)
			ELSE(NOT ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "SYSTEM" AND NOT ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "AUTO (S)")
				IF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "${PKGNAME_UPPER}-NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
					MESSAGE(WARNING "No tclsh/wish command available for testing, but system version of ${packagename} is requested - assuming availability of package.")
				ENDIF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "${PKGNAME_UPPER}-NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
			ENDIF(NOT ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "SYSTEM" AND NOT ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "AUTO (S)")
		ELSE(${wishcmd} STREQUAL "")
			SET(packagefind_script "
catch {package require ${packagename}}
set packageversion NOTFOUND
set packageversion [lindex [lsort -decreasing [package versions ${packagename}]] 0]
set filename \"${CMAKE_BINARY_DIR}/CMakeTmp/${PKGNAME_UPPER}_PKG_VERSION\"
set fileId [open $filename \"w\"]
puts $fileId $packageversion
close $fileId
exit
")
			SET(packagefind_scriptfile "${CMAKE_BINARY_DIR}/CMakeTmp/${packagename}_packageversion.tcl")
			FILE(WRITE ${packagefind_scriptfile} ${packagefind_script})
			EXEC_PROGRAM(${wishcmd} ARGS ${packagefind_scriptfile} OUTPUT_VARIABLE EXECOUTPUT)
			FILE(READ ${CMAKE_BINARY_DIR}/CMakeTmp/${PKGNAME_UPPER}_PKG_VERSION pkgversion)
			STRING(REGEX REPLACE "\n" "" ${PKGNAME_UPPER}_PACKAGE_VERSION ${pkgversion})
			IF(${PKGNAME_UPPER}_PACKAGE_VERSION)
				SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD OFF)
				SET(${PKGNAME_UPPER}_FOUND "TRUE" CACHE STRING "${PKGNAME_UPPER}_FOUND" FORCE)
			ELSE(${PKGNAME_UPPER}_PACKAGE_VERSION)
				SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD ON)
				SET(${PKGNAME_UPPER}_FOUND "${PKGNAME_UPPER}-NOTFOUND" CACHE STRING "${PKGNAME_UPPER}_FOUND" FORCE)
			ENDIF(${PKGNAME_UPPER}_PACKAGE_VERSION)
			# Be quiet if we're doing this over
			IF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "${PKGNAME_UPPER}-NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
				SET(${PKGNAME_UPPER}_FIND_QUIETLY TRUE)
			ENDIF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "${PKGNAME_UPPER}-NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
			FIND_PACKAGE_HANDLE_STANDARD_ARGS(${PKGNAME_UPPER} DEFAULT_MSG ${PKGNAME_UPPER}_PACKAGE_VERSION)
		ENDIF(${wishcmd} STREQUAL "")
	ENDIF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "BUNDLED" OR ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} STREQUAL "AUTO (B)")
	IF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD)
		STRING(TOLOWER ${packagename} PKGNAME_LOWER)
		add_subdirectory(${dir})
		FOREACH(dep ${depends})
			string(TOUPPER ${dep} DEP_UPPER)
			if(BRLCAD_BUILD_${DEP_UPPER})
				add_dependencies(${packagename} ${dep})
			endif(BRLCAD_BUILD_${DEP_UPPER})
		ENDFOREACH(dep ${depends})
		INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${PKGNAME_LOWER}.dist)
		DISTCHECK_IGNORE(${dir} ${PKGNAME_LOWER}_ignore_files)
	ELSE(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD)
		DISTCHECK_IGNORE_ITEM(${dir})
	ENDIF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD)
	MARK_AS_ADVANCED(${PKGNAME_UPPER}_FOUND)
ENDMACRO(THIRD_PARTY_TCL_PACKAGE)
