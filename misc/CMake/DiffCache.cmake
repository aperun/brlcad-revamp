MACRO(DIFF_CACHE_FILE)
	SET(CACHE_FILE "")
	SET(INCREMENT_COUNT_FILE 0)
	IF(EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt.prev)
		FILE(READ ${CMAKE_BINARY_DIR}/CMakeCache.txt.prev CACHE_FILE)
		STRING(REGEX REPLACE ";" "semicolon" ENT1 "${CACHE_FILE}")
		STRING(REGEX REPLACE "\r?\n" ";" ENT "${ENT1}")
		FOREACH(line ${ENT})
			IF(NOT ${line} MATCHES "^#")
				IF(NOT ${line} MATCHES "^//")
					STRING(REGEX REPLACE "std=" "stdequals" line1a ${line})
					STRING(REGEX REPLACE "limit=" "limitequals" line1 "${line1a}")
					STRING(REGEX REPLACE ":.*" "" var ${line1})
					STRING(REGEX REPLACE ".*:" "" valstr ${line1})
					STRING(REGEX REPLACE ".*=" "" val ${valstr})
					IF(NOT ${var} MATCHES "ADVANCED$" AND NOT ${var} MATCHES "STRINGS$" AND NOT ${var} MATCHES "INTERNAL$")
						STRING(REGEX REPLACE ";" "semicolon" currvar1 "${${var}}")
						STRING(REGEX REPLACE "std=" "stdequals" currvar1a "${currvar1}")
						STRING(REGEX REPLACE "limit=" "limitequals" currvar "${currvar1a}")
						IF(NOT "${currvar}" STREQUAL "${val}")
							#MESSAGE("Found difference: ${var}")
							#MESSAGE("Cache: ${val}")
							#MESSAGE("Current: ${currval}")
							SET(INCREMENT_COUNT_FILE 1)
						ENDIF(NOT "${currvar}" STREQUAL "${val}")
					ENDIF(NOT ${var} MATCHES "ADVANCED$" AND NOT ${var} MATCHES "STRINGS$" AND NOT ${var} MATCHES "INTERNAL$")
				ENDIF(NOT ${line} MATCHES "^//")
			ENDIF(NOT ${line} MATCHES "^#")
		ENDFOREACH(line ${ENT})
	ENDIF(EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt.prev)
ENDMACRO()

