#/*********************************************************\
# * Copyright (c) 2012-2022 The Unrimp Team
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
cmake_minimum_required(VERSION 3.14.0)


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
## Source codes
##################################################
set(SOURCE_CODES NullRhi.cpp)

# Add a natvis file for better debug support in Visual Studio ( https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2019 )
if(MSVC)
	set(SOURCE_CODES ${SOURCE_CODES} ../../Rhi.natvis)
endif(MSVC)


##################################################
## Libraries
##################################################
add_library(NullRhi ${LIBRARY_TYPE} ${SOURCE_CODES})
if(SHARED_LIBRARY)
	set_target_properties(NullRhi PROPERTIES COMPILE_FLAGS "-DSHARED_LIBRARIES -DRHI_NULL_EXPORTS")
endif()


##################################################
## Preprocessor definitions
##################################################
unrimp_add_conditional_definition(NullRhi ARCHITECTURE_X64)
if(RHI_DEBUG)
	target_compile_definitions(NullRhi PRIVATE RHI_DEBUG)
	target_compile_definitions(NullRhi PRIVATE RHI_STATISTICS)
endif()


##################################################
## Includes
##################################################
target_include_directories(NullRhi PRIVATE ${CMAKE_SOURCE_DIR}/Source)
