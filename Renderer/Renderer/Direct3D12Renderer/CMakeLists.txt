#/*********************************************************\
# * Copyright (c) 2012-2018 The Unrimp Team
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
# Used options:
# - "Enable C++ Exceptions"				= "No" = "/EHs-c-"
# - "Enable Run-Time Type Information"	= "No (/GR-)"
unrimp_replace_cmake_cxx_flags(/EHsc /EHs-c-)
unrimp_replace_cmake_cxx_flags(/GR /GR-)


##################################################
## Includes
##################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)	# The directory "Renderer/Renderer.h" resides in


##################################################
## Source codes
##################################################
set(SOURCE_CODES Direct3D12Renderer.cpp)


##################################################
## Libraries
##################################################
add_library(Direct3D12Renderer ${LIBRARY_TYPE} ${SOURCE_CODES})
if(SHARED_LIBRARY)
	set_target_properties(Direct3D12Renderer PROPERTIES COMPILE_FLAGS "-DSHARED_LIBRARIES -DDIRECT3D12RENDERER_EXPORTS")
endif()


##################################################
## Post build
##################################################
add_custom_command(TARGET Direct3D12Renderer COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${LIBRARY_PREFIX}Direct3D12Renderer${LIBRARY_SUFFIX}" ${OUTPUT_LIB_BIN_DIRECTORY})
if(SHARED_LIBRARY)
	add_custom_command(TARGET Direct3D12Renderer COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Direct3D12Renderer.lib" ${OUTPUT_LIB_DIRECTORY})
endif()
