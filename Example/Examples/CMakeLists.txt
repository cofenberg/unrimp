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


find_package(Qt4 COMPONENTS QtCore QtGui)
if(QT4_FOUND AND QT_QTGUI_FOUND)
	set(EXAMPLE_RUNNER_QT4 "0" CACHE BOOL "Use Qt GUI runner")
else()
	unset(EXAMPLE_RUNNER_QT4 CACHE)
endif()
if(EXAMPLE_RUNNER_QT4)
	include(${QT_USE_FILE})
endif()


##################################################
## Preprocessor definitions
##################################################
add_definitions(-DGLM_FORCE_RADIANS -DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_ENABLE_EXPERIMENTAL)
if(NOT RENDERER_RUNTIME)
	add_definitions(-DRENDERER_NO_RUNTIME)
endif()
if(EXAMPLE_RUNNER_QT4)
	add_definitions(-DEXAMPLE_RUNNER_QT4)
	ADD_DEFINITIONS(${QT_DEFINITIONS})
endif()
if(WIN32)
	add_definitions(-DUNICODE)
elseif(UNIX)
	add_definitions(-DRENDERER_NO_DIRECT3D9 -DRENDERER_NO_DIRECT3D10 -DRENDERER_NO_DIRECT3D11 -DRENDERER_NO_DIRECT3D12)
endif()
if (Boost_FILESYSTEM_FOUND)
	add_definitions(-DUNRIMP_USE_BOOST_FILESYSTEM)
endif()
unrimp_add_definition(RENDERER_RUNTIME_PROFILER)
unrimp_add_definition(RENDERER_RUNTIME_IMGUI)
unrimp_add_definition(RENDERER_RUNTIME_OPENVR)


##################################################
## Includes
##################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Renderer/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Renderer/RendererRuntime/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Renderer/RendererToolkit/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/glm/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/utf8cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/)	# For "DeviceInput"
if(RENDERER_RUNTIME_PROFILER)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/Remotery/)
endif()
if(RENDERER_RUNTIME_IMGUI)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/ImGui/include/)
endif()
if(RENDERER_RUNTIME_OPENVR)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/OpenVR/include/)
endif()
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/${OS_ARCHITECTURE})


##################################################
## Source codes
##################################################
set(EXTERNAL_SOURCE_CODES
	# PhysicsFS external library
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_archiver_zip.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_archiver_dir.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_archiver_unpacked.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_byteorder.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_unicode.c
	# DeviceInput external library
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/DeviceInput/DeviceInput.cpp
	# Remotery external library
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/Remotery/Remotery/Remotery.c
)
if(WIN32)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# PhysicsFS external library
		${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_platform_windows.c
	)
elseif(UNIX)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# PhysicsFS external library
		${CMAKE_CURRENT_SOURCE_DIR}/../../External/PhysicsFS/src/physfs_platform_posix.c
	)
endif()
set(SOURCE_CODES
	${EXTERNAL_SOURCE_CODES}
	src/Advanced/FirstGpgpu/FirstGpgpu.cpp
	src/Advanced/IcosahedronTessellation/IcosahedronTessellation.cpp
	src/Basics/FirstGeometryShader/FirstGeometryShader.cpp
	src/Basics/FirstIndirectBuffer/FirstIndirectBuffer.cpp
	src/Basics/FirstInstancing/FirstInstancing.cpp
	src/Basics/FirstMultipleRenderTargets/FirstMultipleRenderTargets.cpp
	src/Basics/FirstMultipleSwapChains/FirstMultipleSwapChains.cpp
	src/Basics/FirstRenderToTexture/FirstRenderToTexture.cpp
	src/Basics/FirstTessellation/FirstTessellation.cpp
	src/Basics/FirstTexture/FirstTexture.cpp
	src/Basics/FirstTriangle/FirstTriangle.cpp
	src/Basics/VertexBuffer/VertexBuffer.cpp
	src/ConsoleExampleRunner.cpp
	src/ExampleRunner.cpp
	src/Framework/CommandLineArguments.cpp
	src/Framework/Color4.cpp
	src/Framework/ExampleBase.cpp
	src/Framework/IApplication.cpp
	src/Framework/IApplicationRenderer.cpp
	src/Framework/ExampleBase.cpp
	src/Main.cpp
)
if(WIN32)
	set(SOURCE_CODES
		${SOURCE_CODES}
		src/Framework/ApplicationImplWindows.cpp
		src/PrecompiledHeader.cpp
	)
elseif(UNIX)
	set(SOURCE_CODES
		${SOURCE_CODES}
		src/Framework/ApplicationImplLinux.cpp
		src/Framework/X11Window.cpp
		src/Framework/X11Application.cpp
	)
endif()
if(RENDERER_RUNTIME)
	set(SOURCE_CODES
		${SOURCE_CODES}
		src/Framework/IApplicationRendererRuntime.cpp
		src/Runtime/FirstMesh/FirstMesh.cpp
		src/Runtime/FirstScene/FirstScene.cpp
		src/Runtime/FirstScene/FreeCameraController.cpp
		src/Runtime/FirstCompositor/FirstCompositor.cpp
		src/Runtime/FirstCompositor/CompositorInstancePassFirst.cpp
		src/Runtime/FirstCompositor/CompositorPassFactoryFirst.cpp
		src/Advanced/InstancedCubes/CubeRendererDrawInstanced/BatchDrawInstanced.cpp
		src/Advanced/InstancedCubes/CubeRendererDrawInstanced/CubeRendererDrawInstanced.cpp
		src/Advanced/InstancedCubes/CubeRendererInstancedArrays/BatchInstancedArrays.cpp
		src/Advanced/InstancedCubes/CubeRendererInstancedArrays/CubeRendererInstancedArrays.cpp
		src/Advanced/InstancedCubes/InstancedCubes.cpp
	)
	if(RENDERER_RUNTIME_OPENVR)
		set(SOURCE_CODES
			${SOURCE_CODES}
			src/Runtime/FirstScene/VrController.cpp
		)
	endif()
endif()
if(EXAMPLE_RUNNER_QT4)
	# Qt runner source codes
	set(SOURCE_CODES
		${SOURCE_CODES}
		src/Framework/QtRunner/ExampleRunnerQt4.cpp
	)
endif()

# Disable warnings in external libraries, we can't fix them
if(WIN32)
	# Used options:
	# - Disable "warning C4100: 'len': unreferenced formal parameter"
	# - Disable "warning C4127: conditional expression is constant"
	# - Disable "warning C4242: '=': conversion from 'PHYSFS_uint32' to 'PHYSFS_uint16', possible loss of data"
	# - Disable "warning C4244: 'initializing': conversion from 'const PHYSFS_uint32' to 'const PHYSFS_uint16', possible loss of data"
	# - Disable "warning C4255: '__slwpcb': no function prototype given: converting '()' to '(void)'"
	# - Disable "warning C4389: '==': signed/unsigned mismatch"
	# - Disable "warning C4456: declaration of 'len' hides previous local declaration"
	# - Disable "warning C4548: expression before comma has no effect; expected expression with side-effect"
	# - Disable "warning C4668: '_USING_V110_SDK71_' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'"
	set_source_files_properties(${EXTERNAL_SOURCE_CODES} PROPERTIES COMPILE_FLAGS "-wd 4100 -wd 4127 -wd 4242 -wd 4244 -wd 4255 -wd 4389 -wd 4456 -wd 4548 -wd 4668")
endif()


##################################################
## Executables
##################################################
if(NOT SHARED_LIBRARY)
	if(WIN32)
		add_executable(Examples WIN32 ${SOURCE_CODES})

		# Set Visual Studio debugger working directory (see https://stackoverflow.com/a/42973332 )
		set_target_properties(Examples PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${OS_ARCHITECTURE}/")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_DIRECT3D9 -DRENDERER_NO_DIRECT3D10 -DRENDERER_NO_DIRECT3D11 -DRENDERER_NO_DIRECT3D12")
		add_executable(Examples ${SOURCE_CODES})
		if(EXAMPLE_RUNNER_QT4)
			TARGET_LINK_LIBRARIES(Examples ${QT_LIBRARIES})
		endif()
		TARGET_LINK_LIBRARIES(Examples dl)
		if(HAVE_CXX_FILESYSTEM_TS_SUPPORT)
			TARGET_LINK_LIBRARIES(Examples stdc++fs)
		elseif(Boost_FILESYSTEM_FOUND)
			TARGET_LINK_LIBRARIES(Examples Boost::filesystem)
		endif()
	endif()
	if(RENDERER_NULL)
		set(LIBRARIES ${LIBRARIES} NullRenderer)
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_NULL")
	endif()
	if(RENDERER_OPENGL)
		set(LIBRARIES ${LIBRARIES} OpenGLRenderer)
		if(UNIX)
			set(LIBRARIES ${LIBRARIES} X11)
		endif()
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_OPENGL")
	endif()
	if(RENDERER_OPENGLES3)
		set(LIBRARIES ${LIBRARIES} OpenGLES3Renderer)
		if(UNIX)
			set(LIBRARIES ${LIBRARIES} X11)
		endif()
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_OPENGLES3")
	endif()
	if(RENDERER_VULKAN)
		set(LIBRARIES ${LIBRARIES} VulkanRenderer)
		if(UNIX)
			set(LIBRARIES ${LIBRARIES} X11)
		endif()
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_VULKAN")
	endif()
	if(WIN32)
		if(RENDERER_DIRECT3D9)
			set(LIBRARIES ${LIBRARIES} Direct3D9Renderer)
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_DIRECT3D9")
		endif()
		if(RENDERER_DIRECT3D10)
			set(LIBRARIES ${LIBRARIES} Direct3D10Renderer)
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_DIRECT3D10")
		endif()
		if(RENDERER_DIRECT3D11)
			set(LIBRARIES ${LIBRARIES} Direct3D11Renderer)
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_DIRECT3D11")
		endif()
		if(RENDERER_DIRECT3D12)
			set(LIBRARIES ${LIBRARIES} Direct3D12Renderer)
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRENDERER_NO_DIRECT3D12")
		endif()
	endif()
	if(RENDERER_RUNTIME)
		set(LIBRARIES ${LIBRARIES} RendererRuntime)
	endif()
	if(UNIX)
		set(LIBRARIES ${LIBRARIES} pthread)
	endif()
	target_link_libraries(Examples ${LIBRARIES})
	if(WIN32)
		add_dependencies(Examples NullRenderer Direct3D9Renderer Direct3D10Renderer Direct3D11Renderer Direct3D12Renderer)
	else()
		add_dependencies(Examples NullRenderer)
	endif()
	if(RENDERER_VULKAN)
		add_dependencies(Examples VulkanRenderer)
	endif()
	if(RENDERER_OPENGL)
		add_dependencies(Examples OpenGLRenderer)
	endif()
	if(RENDERER_OPENGLES3)
		add_dependencies(Examples OpenGLES3Renderer)
	endif()
endif()
if(SHARED_LIBRARY)
	# Shared
	if(WIN32)
		add_executable(Examples WIN32 ${SOURCE_CODES})

		# Set Visual Studio debugger working directory (see https://stackoverflow.com/a/42973332 )
		set_target_properties(Examples PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${OS_ARCHITECTURE}/")
	else()
		add_executable(Examples ${SOURCE_CODES})
		target_link_libraries(Examples X11 dl)
		if(HAVE_CXX_FILESYSTEM_TS_SUPPORT)
			TARGET_LINK_LIBRARIES(Examples stdc++fs)
		elseif(Boost_FILESYSTEM_FOUND)
			TARGET_LINK_LIBRARIES(Examples Boost::filesystem)
		endif()
	endif()
	if(RENDERER_RUNTIME)
		if(WIN32)
			target_link_libraries(Examples RendererRuntime.lib)
		else()
			target_link_libraries(Examples RendererRuntime)
		endif()
		add_dependencies(Examples RendererRuntime)
	endif()
	if(EXAMPLE_RUNNER_QT4)
		TARGET_LINK_LIBRARIES(Examples ${QT_LIBRARIES})
	endif()
	set_target_properties(Examples PROPERTIES COMPILE_FLAGS -DSHARED_LIBRARIES)
endif()


##################################################
## Post build
##################################################
add_custom_command(TARGET Examples COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Examples${CMAKE_EXECUTABLE_SUFFIX}" ${OUTPUT_BIN_DIRECTORY})
if(EXAMPLE_RUNNER_SDL2)
	add_subdirectory(sdl2)
endif()
