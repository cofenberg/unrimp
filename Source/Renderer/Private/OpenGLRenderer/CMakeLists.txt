#/*********************************************************\
# * Copyright (c) 2012-2019 The Unrimp Team
# *
# * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# * and associated documentation files (the "Software"), to deal in the Software without
# * restriction, including without limitation the rights to use, copy, modify, merge, publish,
# * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
# * Software is furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice shall be included in all copies or
# * substantial portions of the Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#\*********************************************************/


##################################################
## CMake
##################################################
cmake_minimum_required(VERSION 3.11.0)


##################################################
## Compiler configuration
##################################################
if(WIN32)
	# Used options:
	# - "Enable C++ Exceptions"				= "No" = "/EHs-c-"
	# - "Enable Run-Time Type Information"	= "No (/GR-)"
	unrimp_replace_cmake_cxx_flags(/EHsc /EHs-c-)
	unrimp_replace_cmake_cxx_flags(/GR /GR-)
endif()


##################################################
## Preprocessor definitions
##################################################
unrimp_add_conditional_definition(RENDERER_OPENGL_STATE_CLEANUP)
unrimp_add_conditional_definition(RENDERER_OPENGL_GLSLTOSPIRV)


##################################################
## Includes
##################################################
include_directories(${CMAKE_SOURCE_DIR}/Source)
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer)	# For "smol-v"
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer/OpenGL)
if(RENDERER_OPENGL_GLSLTOSPIRV)
	set(GLSLANG_DIR ${CMAKE_SOURCE_DIR}/External/Renderer/glslang/src)
	include_directories(${GLSLANG_DIR})
endif()


##################################################
## Source codes
##################################################
set(SOURCE_CODES
	${CMAKE_SOURCE_DIR}/External/Renderer/smol-v/smolv.cpp
	OpenGLRenderer.cpp
)
if(RENDERER_OPENGL_GLSLTOSPIRV)
	set(GLSLANG_SOURCE_CODES
		${GLSLANG_DIR}/../glslangUnityBuild1.cpp
		${GLSLANG_DIR}/../glslangUnityBuild2.cpp
		${GLSLANG_DIR}/../glslangUnityBuild3.cpp
	)
	set(SOURCE_CODES
		${SOURCE_CODES}
		${GLSLANG_SOURCE_CODES}
	)
endif()

# Disable warnings in external libraries, we can't fix them
if(WIN32)
	# Used options:
	# - Disable "warning C4018: '<': signed/unsigned mismatch"
	# - Disable "warning C4242: '=': conversion from 'int' to 'uint8_t', possible loss of data"
	# - Disable "warning C4244: '=': conversion from 'int' to 'uint8_t', possible loss of data"
	# - Disable "warning C4389: '!=': signed/unsigned mismatch"
	# - Disable "warning C4365: 'initializing': conversion from 'int' to 'size_t', signed/unsigned mismatch"
	# - Disable "warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc"
	# - Disable "warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?""
	# - Disable "warning C4626: 'std::_Basic_container_proxy_ptr12': assignment operator was implicitly defined as deleted"
	set_source_files_properties(${CMAKE_SOURCE_DIR}/External/Renderer/smol-v/smolv.cpp PROPERTIES COMPILE_FLAGS "/wd4018 /wd4242 /wd4244 /wd4389 /wd4365 /wd4530 /wd4574 /wd4626")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set_source_files_properties(${CMAKE_SOURCE_DIR}/External/Renderer/smol-v/smolv.cpp PROPERTIES COMPILE_FLAGS "-Wno-sign-compare")
	if(RENDERER_OPENGL_GLSLTOSPIRV)
		set_source_files_properties(${GLSLANG_SOURCE_CODES} PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers -Wno-reorder")
	endif()
endif()


##################################################
## Libraries
##################################################
add_library(OpenGLRenderer ${LIBRARY_TYPE} ${SOURCE_CODES})
if(SHARED_LIBRARY)
	set_target_properties(OpenGLRenderer PROPERTIES COMPILE_FLAGS "-DSHARED_LIBRARIES -DRENDERER_OPENGL_EXPORTS")
endif()


##################################################
## Post build
##################################################
if(WIN32 AND SHARED_LIBRARY)
	add_custom_command(TARGET OpenGLRenderer COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/OpenGLRenderer.lib" ${OUTPUT_LIBRARY_DIRECTORY})
endif()


##################################################
## Install
##################################################
install(TARGETS OpenGLRenderer
		RUNTIME DESTINATION "${OUTPUT_LIBRARY_BINARY_DIRECTORY}"
		ARCHIVE DESTINATION "${OUTPUT_LIBRARY_DIRECTORY}")
