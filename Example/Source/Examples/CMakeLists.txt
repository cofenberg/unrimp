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

# Optional "Simple DirectMedia Layer" (SDL, https://www.libsdl.org/ ) support inside the example framework
if(EXAMPLE_SDL2_ENABLED)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/Build/")
	set(SDL2_BUILDING_LIBRARY "1")	# Ignore "SDL2main" since the example framework has an own main program entry point
	find_package(SDL2)
endif()
if(SDL2_FOUND)
	message("Found SDL2, SDL2 example enabled")
endif()


##################################################
## Preprocessor definitions
##################################################
add_definitions(-DGLM_FORCE_CXX17 -DGLM_FORCE_INLINE -DGLM_FORCE_SIMD_AVX2 -DGLM_FORCE_XYZW_ONLY -DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DGLM_ENABLE_EXPERIMENTAL)
if(WIN32)
	add_definitions(-DUNICODE)
endif()
unrimp_add_conditional_renderer_definitions()
unrimp_add_conditional_definition(RENDERER_RUNTIME)
unrimp_add_conditional_definition(RENDERER_RUNTIME_GRAPHICS_DEBUGGER)
unrimp_add_conditional_definition(RENDERER_RUNTIME_PROFILER)
if(RENDERER_RUNTIME_PROFILER)
	if(RENDERER_DIRECT3D11)
		add_definitions(-DRMT_USE_D3D11)
	endif()
	if(RENDERER_OPENGL)
		add_definitions(-DRMT_USE_OPENGL)
	endif()
endif()
unrimp_add_conditional_definition(RENDERER_RUNTIME_IMGUI)
unrimp_add_conditional_definition(RENDERER_RUNTIME_OPENVR)
unrimp_add_conditional_definition(SDL2_FOUND)

# Static builds are usually used for shipping builds, so disable the renderer toolkit for static builds
if(SHARED_LIBRARY)
	unrimp_add_conditional_definition(RENDERER_TOOLKIT)
endif()


##################################################
## Includes
##################################################
include_directories(${CMAKE_SOURCE_DIR}/Example/Source)
include_directories(${CMAKE_SOURCE_DIR}/Source)
include_directories(${CMAKE_SOURCE_DIR}/External/Example)	# For "DeviceInput" and "ini.h"
if(ANDROID)
	include_directories(${CMAKE_SOURCE_DIR}/External/Example/Android)
endif()
include_directories(${CMAKE_SOURCE_DIR}/External/RendererRuntime)	# "glm", "PhysicsFS", "ImGui", "OpenVR", "RenderDoc" and "Remotery"
if(RENDERER_TOOLKIT)
	include_directories(${CMAKE_SOURCE_DIR}/Renderer/RendererToolkit/include)
endif()
link_directories(${CMAKE_SOURCE_DIR}/Library/${OS_ARCHITECTURE})
if(RENDERER_DIRECT3D11)
	link_directories(${CMAKE_SOURCE_DIR}/External/Renderer/AGS_SDK/Windows_${ARCHITECTURE}/Library)
endif()
if(SDL2_FOUND)
	include_directories(${SDL2_INCLUDE_DIR})
endif()


##################################################
## Source codes
##################################################
set(EXTERNAL_SOURCE_CODES
	# DeviceInput external library
	${CMAKE_SOURCE_DIR}/External/Example/DeviceInput/DeviceInput.cpp
)
if(NOT ANDROID)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# PhysicsFS external library
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs.c
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_archiver_zip.c
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_archiver_dir.c
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_archiver_unpacked.c
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_byteorder.c
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_unicode.c
	)
endif()
if(WIN32)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# PhysicsFS external library
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_platform_windows.c
	)
elseif(UNIX AND NOT ANDROID)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# PhysicsFS external library
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_platform_unix.c
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/PhysicsFS/physfs_platform_posix.c
	)
endif()
if(RENDERER_RUNTIME_PROFILER)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# Remotery external library
		${CMAKE_SOURCE_DIR}/External/RendererRuntime/Remotery/Remotery.c
	)
endif()
set(SOURCE_CODES
	${EXTERNAL_SOURCE_CODES}
	ExamplesUnityBuild.cpp
	Private/ExampleRunner.cpp
	Private/Framework/CommandLineArguments.cpp
	Private/Framework/Color4.cpp
	Private/Framework/ExampleBase.cpp
	Private/Framework/IApplication.cpp
	Private/Framework/IApplicationRenderer.cpp
	Private/Framework/ExampleBase.cpp
	Private/Main.cpp
)
if(WIN32)
	if(SDL2_FOUND)
		set(SOURCE_CODES
			${SOURCE_CODES}
			Private/Framework/ApplicationImplSdl2.cpp
		)
	else()
		set(SOURCE_CODES
			${SOURCE_CODES}
			Private/Framework/ApplicationImplWindows.cpp
		)
	endif()
elseif(ANDROID)
	message("TODO(co) Examples: Add Android support")
elseif(UNIX)
	set(SOURCE_CODES
		${SOURCE_CODES}
		Private/Framework/Linux/ApplicationImplLinux.cpp
		Private/Framework/Linux/X11Window.cpp
		Private/Framework/Linux/X11Application.cpp
	)
endif()
if(RENDERER_RUNTIME)
	set(SOURCE_CODES
		${SOURCE_CODES}
		Private/Framework/IApplicationRendererRuntime.cpp
	)
endif()

# Disable warnings in external libraries, we can't fix them
if(WIN32)
	# Used options:
	# - Disable "warning C4018: '<': signed/unsigned mismatch"
	# - Disable "warning C4100: 'len': unreferenced formal parameter"
	# - Disable "warning C4101: 'retval': unreferenced local variable"
	# - Disable "warning C4127: conditional expression is constant"
	# - Disable "warning C4201: nonstandard extension used: nameless struct/union"
	# - Disable "warning C4242: '=': conversion from 'PHYSFS_uint32' to 'PHYSFS_uint16', possible loss of data"
	# - Disable "warning C4244: 'initializing': conversion from 'const PHYSFS_uint32' to 'const PHYSFS_uint16', possible loss of data"
	# - Disable "warning C4255: '__slwpcb': no function prototype given: converting '()' to '(void)'"
	# - Disable "warning C4267: '=': conversion from 'size_t' to 'int', possible loss of data"
	# - Disable "warning C4388: '<': signed/unsigned mismatch"
	# - Disable "warning C4389: '==': signed/unsigned mismatch"
	# - Disable "warning C4456: declaration of 'len' hides previous local declaration"
	# - Disable "warning C4548: expression before comma has no effect; expected expression with side-effect"
	# - Disable "warning C4668: '_USING_V110_SDK71_' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'"
	# - Disable "warning C4701: potentially uninitialized local variable 'timestamp' used"
	set_source_files_properties(${EXTERNAL_SOURCE_CODES} PROPERTIES COMPILE_DEFINITIONS "PHYSFS_SUPPORTS_7Z=0;PHYSFS_SUPPORTS_GRP=0;PHYSFS_SUPPORTS_HOG=0;;PHYSFS_SUPPORTS_MVL=0;PHYSFS_SUPPORTS_WAD=0;PHYSFS_SUPPORTS_QPAK=0;PHYSFS_SUPPORTS_SLB=0;PHYSFS_SUPPORTS_ISO9660=0;PHYSFS_SUPPORTS_VDF=0")
	set_source_files_properties(${EXTERNAL_SOURCE_CODES} PROPERTIES COMPILE_FLAGS "/wd4018 /wd4100 /wd4101 /wd4127 /wd4201 /wd4242 /wd4244 /wd4255 /wd4267 /wd4388 /wd4389 /wd4456 /wd4548 /wd4668 /wd4701")
endif()


##################################################
## Executables
##################################################
if(WIN32)
	add_executable(Examples WIN32 ${SOURCE_CODES})

	# Set Visual Studio debugger working directory (see https://stackoverflow.com/a/42973332 )
	set_target_properties(Examples PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Binary/${OS_ARCHITECTURE}/")
elseif(ANDROID)
	add_library(Examples SHARED ${SOURCE_CODES})
else()
	add_executable(Examples ${SOURCE_CODES})
	set(LIBRARIES ${LIBRARIES} X11 dl)
endif()
if(SHARED_LIBRARY)
	if(RENDERER_RUNTIME)
		if(WIN32)
			target_link_libraries(Examples RendererRuntime.lib)
		else()
			target_link_libraries(Examples RendererRuntime)
		endif()
		add_dependencies(Examples RendererRuntime)
	endif()
	set_target_properties(Examples PROPERTIES COMPILE_FLAGS -DSHARED_LIBRARIES)
else()
	unrimp_add_conditional_library_dependency(Examples RENDERER_NULL NullRenderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_VULKAN VulkanRenderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_OPENGL OpenGLRenderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_OPENGLES3 OpenGLES3Renderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_DIRECT3D9 Direct3D9Renderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_DIRECT3D10 Direct3D10Renderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_DIRECT3D11 Direct3D11Renderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_DIRECT3D12 Direct3D12Renderer)
	unrimp_add_conditional_library_dependency(Examples RENDERER_RUNTIME RendererRuntime)
	if(UNIX AND NOT ANDROID)
		set(LIBRARIES ${LIBRARIES} pthread)
	endif()
	target_link_libraries(Examples ${LIBRARIES})
endif()
if(SDL2_FOUND)
	target_link_libraries(Examples ${SDL2_LIBRARY})
endif()


##################################################
## Post build
##################################################
# Copy the SDL2 shared library, if necessary
if(WIN32 AND SDL2_FOUND)
	message("SDL copy " "${CMAKE_SOURCE_DIR}/External/Example/SDL2/lib/${ARCHITECTURE}/SDL2.dll")
	add_custom_command(TARGET Examples COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/External/Example/SDL2/lib/${ARCHITECTURE}/SDL2.dll" ${OUTPUT_BINARY_DIRECTORY})
endif()


##################################################
## Install
##################################################
install(TARGETS Examples RUNTIME DESTINATION "${OUTPUT_BINARY_DIRECTORY}")
