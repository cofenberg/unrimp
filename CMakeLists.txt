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
## Project
##################################################
project(Unrimp)


##################################################
## User options
##################################################
set(SHARED_LIBRARY "1" CACHE BOOL "'true' to build as shared library (dynamic library aka dll on Microsoft Windows), else 'false' for static library")

# Advanced options
	if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
		set(RENDERER_DEBUG "1" CACHE BOOL "If false, the e.g. Direct3D 9 PIX functions (D3DPERF_* functions) debug features are disabled (disabling support just reduces the binary size slightly but makes debugging more difficult)")
	else()
		set(RENDERER_DEBUG "0" CACHE BOOL "If false, the e.g. Direct3D 9 PIX functions (D3DPERF_* functions) debug features are disabled (disabling support just reduces the binary size slightly but makes debugging more difficult)")
	endif()
	mark_as_advanced(RENDERER_DEBUG)

# Renderer backends
	# Null renderer backend
		# Null renderer doesn't compile under Android for now disable it
		if(NOT ANDROID)
			set(RENDERER_NULL "1" CACHE BOOL "Build null renderer?")
		endif()

	# OpenGL renderer backend
		# OpenGL renderer is not for Android
		if(NOT ANDROID)
			set(RENDERER_OPENGL "1" CACHE BOOL "Build OpenGL renderer?")

			# Advanced OpenGL renderer options
			set(RENDERER_OPENGL_NO_STATE_CLEANUP "1" CACHE BOOL "If true, the previous OpenGL state will not be restored after performing an operation (better performance, reduces the binary size slightly, but might result in unexpected behaviour when using OpenGL directly beside this renderer)")
			mark_as_advanced(RENDERER_OPENGL_NO_STATE_CLEANUP)

			set(RENDERER_OPENGL_GLSLTOSPIRV "1" CACHE BOOL "If true, GLSL to SPIR-V compiler is included for online shader compilation, increases the binary size around one MiB")
			mark_as_advanced(RENDERER_OPENGL_GLSLTOSPIRV)
		endif()

	# OpenGL ES 3 renderer backend
		set(RENDERER_OPENGLES3 "1" CACHE BOOL "Build OpenGL ES 3 renderer?")

		# Advanced OpenGL ES 3 renderer options
		set(RENDERER_OPENGLES3_NO_STATE_CLEANUP "0" CACHE BOOL "If true, the previous OpenGL ES 3 state will not be restored after performing an operation (better performance, reduces the binary size slightly, but might result in unexpected behaviour when using OpenGL ES 3 directly beside this renderer)")
		mark_as_advanced(RENDERER_OPENGLES3_NO_STATE_CLEANUP)

	# Vulkan renderer backend
		set(RENDERER_VULKAN "1" CACHE BOOL "Build Vulkan renderer?")

		# Advanced Vulkan renderer options
		set(RENDERER_VULKAN_GLSLTOSPIRV "1" CACHE BOOL "If true, GLSL to SPIR-V compiler is included for online shader compilation, increases the binary size around one MiB")
		mark_as_advanced(RENDERER_VULKAN_GLSLTOSPIRV)

	# Microsoft Windows only Direct3D renderer backends
	if(WIN32)
		set(RENDERER_DIRECT3D9  "1" CACHE BOOL "Build Direct3D 9 renderer?")
		set(RENDERER_DIRECT3D10 "1" CACHE BOOL "Build Direct3D 10 renderer?")
		set(RENDERER_DIRECT3D11 "1" CACHE BOOL "Build Direct3D 11 renderer?")
		set(RENDERER_DIRECT3D12 "1" CACHE BOOL "Build Direct3D 12 renderer?")
	endif()

# Renderer runtime
set(RENDERER_RUNTIME "1" CACHE BOOL "Build renderer runtime?")

# Renderer toolkit is not for Android
if(NOT ANDROID)
	set(RENDERER_TOOLKIT "1" CACHE BOOL "Build renderer toolkit?")
endif()

# No examples for Android for now
if(NOT ANDROID)
	# Examples
	set(EXAMPLES				 "1" CACHE BOOL "Build examples?")
	set(EXAMPLE_PROJECT_COMPILER "1" CACHE BOOL "Build example project compiler?")
	set(EXAMPLE_RUNNER_SDL2 	 "0" CACHE BOOL "Build sdl2 version of the examples?")
endif()


##################################################
## Macros
##################################################

##################################################
## MACRO: "unrimp_replace_cmake_cxx_flags()" for replacing content inside "CMAKE_C_FLAGS" as well as "CMAKE_CXX_FLAGS"
#
# Usage example: "unrimp_replace_cmake_cxx_flags(/EHsc /EHs-c-)"
##################################################
macro(unrimp_replace_cmake_cxx_flags from to)
	# Basing on https://stackoverflow.com/a/14172871
	set(CompilerFlags
			CMAKE_CXX_FLAGS
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_RELEASE
			CMAKE_C_FLAGS
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
			)
	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "${from}" "${to}" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endmacro()


##################################################
## Compiler configuration
##################################################
# We need C++17 support (see https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html for details)
set(CMAKE_CXX_STANDARD 17)				# "C++ Language Standard" = "ISO C++17 Standard (/std:c++17)"
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)			# "Disable Language Extensions" = "Yes (/Za)"

if(WIN32)
	# Used options:
	# - "Warning Level"						= "EnableAllWarnings (/Wall)"
	# - "Treat Warnings As Error"			= "Yes (/WX)""
	# - "Multi-processor Compilation"		= "Yes (/MP)""
	# - "Enable Enhanced Instruction Set"	= "Advanced Vector Extensions 2 (/arch:AVX2)"
	# - Enable/disable warnings
	add_definitions(-D "_HAS_ITERATOR_DEBUGGING=0" -D "_SECURE_SCL=0")
	set(MSVC_DISABLED_WARNINGS_LIST
		"C4514" # unreferenced inline/local function has been removed
		"C4577" # 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
		"C4710" # function '<x>' not inlined
		"C4711" # function <x> selected for inline expansion
		"C4738" # warning C4738: storing 32-bit float result in memory, possible loss of performance
		"C4820" # '<x>' : '<y>' bytes padding added after data member '<z>'
		"C4996" # '<x>': This function or variable may be unsafe
		"C5045" # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified - https://blogs.msdn.microsoft.com/vcblog/2018/04/20/spectre-diagnostic-in-visual-studio-2017-version-15-7-preview-4/
		)
	set(MSVC_LEVEL4_WARNINGS_LIST
		# See https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/23k5d385(v=vs.100)
		"C4265" # 'class': class has virtual functions, but destructor is not virtual
		)
	string(REPLACE "C" " -wd" MSVC_DISABLED_WARNINGS_STR ${MSVC_DISABLED_WARNINGS_LIST})
	string(REPLACE "C" " -w4" MSVC_LEVEL4_WARNINGS_STR ${MSVC_LEVEL4_WARNINGS_LIST})
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -WX /MP ${MSVC_DISABLED_WARNINGS_STR} ${MSVC_LEVEL4_WARNINGS_STR}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -WX /MP ${MSVC_DISABLED_WARNINGS_STR} ${MSVC_LEVEL4_WARNINGS_STR}")

	# Used release build options:
	# - "Inline Function Expansion"			= "Any Suitable (/Ob2)"
	# - "Enable Intrinsic Functions"		= "Yes (/Oi)"
	# - "Favor Size Or Speed"				= "Favor fast code (/Ot)"
	# - "Omit Fame Pointers"				= "Yes (/Oy)"
	# - "Enable String Pooling"				= "Yes (/GF)"
	# - "Security Check"					= "Disable Security Check (/GS-)"
	# - "Enable Function-Level Linking"		= "Yes (/Gy)"
	# - "Floating Point Model"				= "Fast (/fp:fast)"
	# - "Enable Floating Point Exceptions"	= "No (/fp:except-)"
	if(NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug")
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Ob2 -Oi -Ot -Oy -GF -GS- -Gy -fp:fast -fp:except-")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ob2 -Oi -Ot -Oy -GF -GS- -Gy -fp:fast -fp:except-")
	endif()
	if(NOT SHARED_LIBRARY)
		# "Runtime Library" = "Multi-threaded (/MT)" or "Multi-threaded Debug (/MTd)" in debug builds (the "d" at the end won't be touched)
		unrimp_replace_cmake_cxx_flags(/MD /MT)
	endif()
endif()

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("filesystem" HAVE_CXX17_FILESYSTEM)
CHECK_INCLUDE_FILE_CXX("experimental/filesystem" HAVE_CXX1z_FILESYSTEM)

if(NOT WIN32 AND (RENDERER_TOOLKIT OR EXAMPLES))
	if(NOT HAVE_CXX17_FILESYSTEM AND NOT HAVE_CXX1z_FILESYSTEM)
		FIND_PACKAGE(Boost COMPONENTS "filesystem")
		if(Boost_FILESYSTEM_FOUND)
			message(STATUS "Using boost filesystem as C++ filesystem TS replacement")
		else()
			message(FATAL_ERROR "Renderer toolkit and/or Examples needs C++ filesystem TS support but the compiler ${CMAKE_CXX_COMPILER} doesn't support C++ filesystem TS. Please use a different C++ compiler.")
		endif()
	else()
		set(HAVE_CXX_FILESYSTEM_TS_SUPPORT "ON")
	endif()
endif()

# Enable visibility hidden support (with gcc/clang this would be -fvisibility=hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Enable inline visibility hidden support (with gcc/clang this would be -fvisibility-inlines-hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# At first we treat UNIX = Linux
if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX")
endif()

# We treat Android also as Linux
if(ANDROID)
	# We use exceptions so enable exception feature, is disabled by default by the Android NDK
	set(ANDROID_CPP_FEATURES "exceptions" CACHE STRING "")

	# We only support Android 5.1+
	set(ANDROID_PLATFORM "android-22" CACHE STRING "")

	# We want also the llvm libc++ which is fully C++11 compatible
	# TODO(sw) For qtunrimp we cannot use llvm libc++ because Qt is not yet compilable with llvm libc++ on Android
	#          For now we need to use gnustl_shared
	#set(ANDROID_STL "c++_shared" CACHE STRING "")
	set(ANDROID_STL "gnustl_shared" CACHE STRING "")
endif()


##################################################
## Platform
##################################################
# Check which architecture has the host system
if(${CMAKE_GENERATOR} MATCHES "Ninja")	# Ninja is used for "CMake support in Visual" - https://blogs.msdn.microsoft.com/vcblog/2016/10/05/cmake-support-in-visual-studio/
	if(ARCHITECTURE_X64)
		set(ARCHITECTURE "x64")
	elseif(ARCHITECTURE_X86)
		set(ARCHITECTURE "x86")
	else()
		message(FATAL_ERROR "Unsupported architecture")
	endif()
elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 15 Win64")
	set(ARCHITECTURE_X64 1)
	set(ARCHITECTURE "x64")
elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 15")
	set(ARCHITECTURE_X86 1)
	set(ARCHITECTURE "x86")
elseif(${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
	if(ANDROID)
		set(ARCHITECTURE ${ANDROID_ABI})
	else()
		if(CMAKE_SIZEOF_VOID_P MATCHES 8)
			set(ARCHITECTURE_X64 1)
			set(ARCHITECTURE "x64")
		else()
			set(ARCHITECTURE_X86 1)
			set(ARCHITECTURE "x86")
		endif()
	endif()
else()
	message(FATAL_ERROR "Unsupported architecture")
endif()

# Construct a string which identifies the current configuration: <operation system>_<architecture><build type>_<library type>
# - Operation system examples: "Windows", "Linux", "Android"
# - Architecture examples: "x86", "x64" (aka "x86_64")
# - Build type examples: "" for release build, "d" for debug build
# - Library type examples: "Shared", "Static"
# - Example results: "Windows_x86d_Shared", "Windows_x86_Static"
if(SHARED_LIBRARY)
	set(LIBRARY_TYPE "SHARED")
	set(LIBRARY_TYPE_STRING "Shared")
	set(LIBRARY_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
	set(LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
	set(LIBRARY_TYPE "STATIC")
	set(LIBRARY_TYPE_STRING "Static")
	set(LIBRARY_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
	set(LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	set(BUILD_TYPE_ACRONYM "d")
endif()
set(OS_ARCHITECTURE "${CMAKE_SYSTEM_NAME}_${ARCHITECTURE}${BUILD_TYPE_ACRONYM}_${LIBRARY_TYPE_STRING}")
message(STATUS "Operation system + architecture: ${OS_ARCHITECTURE}")

# Output paths and create the directories
set(OUTPUT_BIN_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${OS_ARCHITECTURE}")
message(STATUS "Output binary directory: ${OUTPUT_BIN_DIRECTORY}")
file(MAKE_DIRECTORY ${OUTPUT_BIN_DIRECTORY})

set(OUTPUT_LIB_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${OS_ARCHITECTURE}")
message(STATUS "Output library directory: ${OUTPUT_LIB_DIRECTORY}")
file(MAKE_DIRECTORY ${OUTPUT_LIB_DIRECTORY})

if(SHARED_LIBRARY)
	set(OUTPUT_LIB_BIN_DIRECTORY "${OUTPUT_BIN_DIRECTORY}")
else()
	set(OUTPUT_LIB_BIN_DIRECTORY "${OUTPUT_LIB_DIRECTORY}")
endif()
message(STATUS "Output library binary directory: ${OUTPUT_LIB_BIN_DIRECTORY}")


##################################################
## Preprocessor definitions
##################################################
if(${ARCHITECTURE_X64})
	add_definitions(-DX64_ARCHITECTURE)
endif()
if(RENDERER_DEBUG)
	add_definitions(-DRENDERER_DEBUG)
else()
	add_definitions(-DRENDERER_NO_STATISTICS)
endif()


##################################################
## Subdirectories
##################################################
add_subdirectory(Renderer)
if(EXAMPLES)
	add_subdirectory(Example/Examples)
endif()
if(EXAMPLE_PROJECT_COMPILER AND RENDERER_RUNTIME AND RENDERER_TOOLKIT)
	add_subdirectory(Example/ExampleProjectCompiler)
endif()
