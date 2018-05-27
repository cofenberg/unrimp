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
add_definitions(-DGLM_FORCE_RADIANS -DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_ENABLE_EXPERIMENTAL)
unrimp_add_definition(RENDERER_RUNTIME_PROFILER)
unrimp_add_definition(RENDERER_RUNTIME_IMGUI)
if(UNIX)
	# Needed for OpenVR
	add_definitions(-DCOMPILER_GCC)
endif()


##################################################
## Includes
##################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)	# The directory "Renderer/Renderer.h" resides in
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/ImGui/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/ImGuizmo/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/OpenVR/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/crunch/inc/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/glm/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/xsimd/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/lz4/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/mikktspace/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../External/utf8cpp/)


##################################################
## Source codes
##################################################
set(EXTERNAL_SOURCE_CODES
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/lz4/src/lz4.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/lz4/src/lz4frame.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/lz4/src/lz4hc.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/lz4/src/xxhash.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../External/mikktspace/src/mikktspace.c
)
if(RENDERER_RUNTIME_IMGUI)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		${CMAKE_CURRENT_SOURCE_DIR}/../../External/ImGui/src/imgui.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/../../External/ImGui/src/imgui_draw.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/../../External/ImGuizmo/src/ImGuizmo.cpp
	)
endif()
set(SOURCE_CODES
	${EXTERNAL_SOURCE_CODES}
	RendererRuntimeUnityBuild.cpp
	src/Resource/Scene/Culling/SceneCullingManager.cpp	# Not compatible with "Advanced Vector Extensions 2" (/arch:AVX2)
)

# Disable warnings in external libraries, we can't fix them
if(WIN32)
	# Used options:
	# - Disable "warning C4201: nonstandard extension used: nameless struct/union""
	# - Disable "warning C4204: nonstandard extension used: non-constant aggregate initializer"
	# - Disable "warning C4255: '__slwpcb': no function prototype given: converting '()' to '(void)'"
	# - Disable "warning C4456: declaration of 'i' hides previous local declaration"
	# - Disable "warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc"
	# - Disable "warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'"
	set_source_files_properties(${EXTERNAL_SOURCE_CODES} PROPERTIES COMPILE_FLAGS "-wd 4201 -wd 4204 -wd 4255 -wd 4456 -wd 4530 -wd 4668")
endif()


##################################################
## Libraries
##################################################
add_library(RendererRuntime ${LIBRARY_TYPE} ${SOURCE_CODES})
if(SHARED_LIBRARY)
	set_target_properties(RendererRuntime PROPERTIES COMPILE_FLAGS "-DSHARED_LIBRARIES -DRENDERER_RUNTIME_EXPORTS")
	if(ANDROID)
		target_link_libraries(RendererRuntime dl ${ANDROID_CXX_STANDARD_LIBRARIES})
	elseif(UNIX)
		target_link_libraries(RendererRuntime dl pthread)
	endif()
endif()


##################################################
## Post build
##################################################
add_custom_command(TARGET RendererRuntime COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${LIBRARY_PREFIX}RendererRuntime${LIBRARY_SUFFIX}" ${OUTPUT_LIB_BIN_DIRECTORY})
if(WIN32)
	if(SHARED_LIBRARY)
		add_custom_command(TARGET RendererRuntime COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/RendererRuntime.lib" ${OUTPUT_LIB_DIRECTORY})
	endif()
	
	# Copy OpenVR dll
	add_custom_command(TARGET RendererRuntime COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../../External/Windows_${ARCHITECTURE}/OpenVR/bin/openvr_api.dll" ${OUTPUT_BIN_DIRECTORY})
endif()
