# This is based on the logic generated by CMake for EXPORT, but customized for
# use with ExternalProject:
#
# https://cmake.org/cmake/help/latest/module/ExternalProject.html
#
# The goal is to create an imported target based on the ExternalProject
# information, and then append the necessary install logic to manage RPath
# settings in the external projects as if the external files were built by the
# main CMake project.


# The catch to this is that the external project outputs MUST be built in a way
# that is compatible with CMake's RPath handling assumptions.  See
# https://stackoverflow.com/questions/41175354/can-i-install-shared-imported-library
# for one of the issues surrounding this - file(RPATH_CHANGE) must be able to
# succeed, and it is up to the 3rd party build setups to prepare their outputs
# to be ready.  The key variable CMAKE_BUILD_RPATH comes from running the
# function cmake_set_rpath, which must be available.

# Sophisticated argument parsing is needed (TODO - remove this line after
# we require >3.4 CMake - after that it is built in
include(CMakeParseArguments)

# Custom patch utility to replace the build directory path with the install
# directory path in text files - make sure CMAKE_BINARY_DIR and
# CMAKE_INSTALL_PREFIX are finalized before generating this file!
configure_file(${CMAKE_SOURCE_DIR}/CMake/buildpath_replace.cxx.in ${CMAKE_BINARY_DIR}/buildpath_replace.cxx)
add_executable(buildpath_replace ${CMAKE_BINARY_DIR}/buildpath_replace.cxx)

function(ExternalProject_ByProducts extproj E_IMPORT_PREFIX)
  cmake_parse_arguments(E "FIXPATH" "" "" ${ARGN})
  if (EXTPROJ_VERBOSE)
    list(LENGTH E_UNPARSED_ARGUMENTS FCNT)
    if (E_FIXPATH)
      if (FCNT GREATER 1)
	message("${extproj}: Adding path adjustment and installation rules for ${FCNT} byproducts")
      else (FCNT GREATER 1)
	message("${extproj}: Adding path adjustment and installation rules for ${FCNT} byproduct")
      endif (FCNT GREATER 1)
    else (E_FIXPATH)
      if (FCNT GREATER 1)
	message("${extproj}: Adding install rules for ${FCNT} byproducts")
      else (FCNT GREATER 1)
	message("${extproj}: Adding install rules for ${FCNT} byproduct")
      endif (FCNT GREATER 1)
    endif (E_FIXPATH)
  endif (EXTPROJ_VERBOSE)
  foreach (bpf ${E_UNPARSED_ARGUMENTS})
    set(I_IMPORT_PREFIX ${CMAKE_BINARY_DIR}/${E_IMPORT_PREFIX})
    get_filename_component(BPF_DIR "${bpf}" DIRECTORY)
    set(D_IMPORT_PREFIX "${E_IMPORT_PREFIX}")
    if (BPF_DIR)
      set(D_IMPORT_PREFIX "${D_IMPORT_PREFIX}/${BPF_DIR}")
    endif (BPF_DIR)
    install(FILES "${I_IMPORT_PREFIX}/${bpf}" DESTINATION "${D_IMPORT_PREFIX}/")
    if (E_FIXPATH)
      # Note - proper quoting for install(CODE) is extremely important for CPack, see
      # https://stackoverflow.com/a/48487133
      install(CODE "execute_process(COMMAND ${CMAKE_BINARY_DIR}/${BIN_DIR}/buildpath_replace${CMAKE_EXECUTABLE_SUFFIX} \"\${CMAKE_INSTALL_PREFIX}/${E_IMPORT_PREFIX}/${bpf}\")")
    endif (E_FIXPATH)
  endforeach (bpf ${E_UNPARSED_ARGUMENTS})
endfunction(ExternalProject_ByProducts)


function(ET_target_props etarg IMPORT_PREFIX)

  cmake_parse_arguments(ET "" "LINK_TARGET;OUTPUT_FILE" "" ${ARGN})

  set_property(TARGET ${etarg} APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
  if (ET_LINK_TARGET)
    set_target_properties(${etarg} PROPERTIES
      IMPORTED_LOCATION_NOCONFIG "${IMPORT_PREFIX}/${ET_LINK_TARGET}"
      IMPORTED_SONAME_NOCONFIG "${ET_LINK_TARGET}"
      )
  else (ET_LINK_TARGET)
    set_target_properties(${etarg} PROPERTIES
      IMPORTED_LOCATION_NOCONFIG "${IMPORT_PREFIX}/${ET_OUTPUT_FILE}"
      IMPORTED_SONAME_NOCONFIG "${ET_OUTPUT_FILE}"
      )
  endif (ET_LINK_TARGET)

endfunction(ET_target_props)


# Mimic the magic of the CMake install(TARGETS) form of the install command.
# This is the key to treating external project build outputs as fully managed
# CMake outputs, and requires that the external project build in such a way
# that the rpath settings in the build outputs are compatible with this
# mechanism.
function(ET_RPath REL_DIR LIB_DIR E_OUTPUT_FILE)
  if (NOT APPLE)
    set(NEW_RPATH "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${LIB_DIR}:$ORIGIN/../${LIB_DIR}")
  else (NOT APPLE)
    set(NEW_RPATH "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${LIB_DIR}:@loader_path/../${LIB_DIR}")
  endif (NOT APPLE)
  if (NOT DEFINED CMAKE_BUILD_RPATH)
    message(FATAL_ERROR "ET_RPath run without CMAKE_BUILD_RPATH defined - run cmake_set_rpath before defining external projects.")
  endif (NOT DEFINED CMAKE_BUILD_RPATH)
  # Note - proper quoting for install(CODE) is extremely important for CPack, see
  # https://stackoverflow.com/a/48487133
  install(CODE "
  file(RPATH_CHANGE
    FILE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${REL_DIR}/${E_OUTPUT_FILE}\"
    OLD_RPATH \"${CMAKE_BUILD_RPATH}\"
    NEW_RPATH \"${NEW_RPATH}\")
  ")
endfunction(ET_RPath)

function(ExternalProject_Target etarg extproj)

  cmake_parse_arguments(E "RPATH;EXEC" "IMPORT_PREFIX;OUTPUT_FILE;LINK_TARGET;STATIC_OUTPUT_FILE;STATIC_LINK_TARGET" "SYMLINKS;DEPS" ${ARGN})

  if(NOT TARGET ${extproj})
    message(FATAL_ERROR "${extprog} is not a target")
  endif(NOT TARGET ${extproj})

  # Protect against redefinition of already defined targets.
  if(TARGET ${etarg})
    message(FATAL_ERROR "Target ${etarg} is already defined\n")
  endif(TARGET ${etarg})
  if(E_STATIC AND TARGET ${etarg}-static)
    message(FATAL_ERROR "Target ${etarg}-static is already defined\n")
  endif(E_STATIC AND TARGET ${etarg}-static)

  if (EXTPROJ_VERBOSE)
    message("${extproj}: Adding target \"${etarg}\"")
  endif (EXTPROJ_VERBOSE)

  if (E_STATIC_OUTPUT_FILE)
    set(E_STATIC 1)
  endif (E_STATIC_OUTPUT_FILE)

  if (E_OUTPUT_FILE AND NOT E_EXEC)
    set(E_SHARED 1)
  endif (E_OUTPUT_FILE AND NOT E_EXEC)

  # Create imported target - need to both make the target itself
  # and set the necessary properties.  See also
  # https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/Exporting-and-Importing-Targets

  if (E_IMPORT_PREFIX)
    set(LIB_DIR "${LIB_DIR}/${E_IMPORT_PREFIX}")
    set(BIN_DIR "${BIN_DIR}/${E_IMPORT_PREFIX}")
  endif (E_IMPORT_PREFIX)

  # Because the outputs are not properly build target outputs of the primary
  # CMake project, we need to install as either FILES or PROGRAMS
  if (NOT E_EXEC)

    # Handle shared library
    if (E_SHARED)
      add_library(${etarg} SHARED IMPORTED GLOBAL)
      set(IMPORT_PREFIX "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
      if(E_IMPORT_PREFIX)
	set(IMPORT_PREFIX "${IMPORT_PREFIX}/${E_IMPORT_PREFIX}")
      endif(E_IMPORT_PREFIX)
      if (E_LINK_TARGET)
	ET_target_props(${etarg} "${IMPORT_PREFIX}" LINK_TARGET ${E_LINK_TARGET})
      else (E_LINK_TARGET)
	ET_target_props(${etarg} "${IMPORT_PREFIX}" OUTPUT_FILE_TARGET ${E_OUTPUT_FILE})
      endif (E_LINK_TARGET)
      install(FILES "${IMPORT_PREFIX}/${E_OUTPUT_FILE}" DESTINATION ${LIB_DIR})
      if (E_RPATH AND NOT MSVC)
	ET_RPath(${LIB_DIR} ${LIB_DIR} ${E_OUTPUT_FILE})
      endif (E_RPATH AND NOT MSVC)
    endif (E_SHARED)

    # If we do have a static lib as well, handle that
    if (E_STATIC)
      add_library(${etarg}-static STATIC IMPORTED GLOBAL)
      set(IMPORT_PREFIX "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
      if(E_IMPORT_PREFIX)
	set(IMPORT_PREFIX "${IMPORT_PREFIX}/${E_IMPORT_PREFIX}")
      endif(E_IMPORT_PREFIX)
      if (E_STATIC_LINK_TARGET)
	ET_target_props(${etarg}-static "${INPORT_PREFIX}" LINK_TARGET ${E_STATIC_LINK_TARGET})
      else (E_STATIC_LINK_TARGET)
	ET_target_props(${etarg}-static "${IMPORT_PREFIX}" OUTPUT_FILE ${E_STATIC_OUTPUT_FILE})
      endif (E_STATIC_LINK_TARGET)
      install(FILES "${IMPORT_PREFIX}/${E_STATIC_OUTPUT_FILE}" DESTINATION ${LIB_DIR})
    endif (E_STATIC)

  else (NOT E_EXEC)

    add_executable(${etarg} IMPORTED GLOBAL)
    set(IMPORT_PREFIX "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    if(E_IMPORT_PREFIX)
      set(IMPORT_PREFIX "${IMPORT_PREFIX}/${E_IMPORT_PREFIX}")
    endif(E_IMPORT_PREFIX)
    ET_target_props(${etarg} "${IMPORT_PREFIX}" OUTPUT_FILE ${E_OUTPUT_FILE})
    install(PROGRAMS "${IMPORT_PREFIX}/${E_OUTPUT_FILE}" DESTINATION ${BIN_DIR})
    if (E_RPATH AND NOT MSVC)
      ET_RPath(${BIN_DIR} ${LIB_DIR} ${E_OUTPUT_FILE})
    endif (E_RPATH AND NOT MSVC)

  endif (NOT E_EXEC)

  # Let CMake know there is a target dependency here, despite this being an import target
  add_dependencies(${etarg} ${extproj})

  # Add install rules for any symlinks the caller has listed
  if(E_SYMLINKS)
    foreach(slink ${E_SYMLINKS})
      install(FILES "${IMPORT_PREFIX}/${slink}" DESTINATION ${LIB_DIR})
    endforeach(slink ${E_SYMLINKS})
  endif (E_SYMLINKS)

endfunction(ExternalProject_Target)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

