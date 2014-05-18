# Inspired by http://www.cmake.org/Wiki/RecipeAddUninstallTarget
SET(INSTALL_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")
IF (NOT EXISTS ${INSTALL_MANIFEST})
  MESSAGE(FATAL_ERROR "Could not uninstall with no manifest: ${INSTALL_MANIFEST}")
ENDIF()
 
FILE(STRINGS ${INSTALL_MANIFEST} files)
FOREACH (file ${files})
  IF (NOT EXISTS ${file})
    MESSAGE(STATUS "File does not exist: ${file}")
  ELSE()
    MESSAGE(STATUS "Removing file: ${file}")
    EXEC_PROGRAM(
      ${CMAKE_COMMAND} ARGS "-E remove ${file}"
      OUTPUT_VARIABLE stdout
      RETURN_VALUE result
      )
    IF (NOT "${result}" STREQUAL 0)
      MESSAGE(FATAL_ERROR "Failed to remove file: ${file}")
    ENDIF()
  ENDIF()
ENDFOREACH()
FILE(REMOVE ${INSTALL_MANIFEST})
