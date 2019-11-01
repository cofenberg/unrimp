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
add_definitions(-DGLM_FORCE_CXX17 -DGLM_FORCE_INLINE -DGLM_FORCE_SIMD_AVX2 -DGLM_FORCE_XYZW_ONLY -DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DGLM_ENABLE_EXPERIMENTAL)
unrimp_add_conditional_rhi_definitions()
unrimp_add_conditional_definition(RENDERER_GRAPHICS_DEBUGGER)
unrimp_add_conditional_definition(RENDERER_PROFILER)
unrimp_add_conditional_definition(RENDERER_IMGUI)
unrimp_add_conditional_definition(RENDERER_OPENVR)
if(UNIX AND RENDERER_OPENVR)
	# Needed for OpenVR
	add_definitions(-DCOMPILER_GCC)
endif()


##################################################
## Includes
##################################################
include_directories(${CMAKE_SOURCE_DIR}/Source)
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer)	# For "glm", "lz4", "xsimd", "MojoShader", "mikktspace", "acl", "ImGui", "ImGuizmo" and "OpenVR"
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer/crunch/inc)


##################################################
## Source codes
##################################################
set(EXTERNAL_SOURCE_CODES
	${CMAKE_SOURCE_DIR}/External/Renderer/lz4/lz4.c
	${CMAKE_SOURCE_DIR}/External/Renderer/lz4/lz4frame.c
	${CMAKE_SOURCE_DIR}/External/Renderer/lz4/lz4hc.c
	${CMAKE_SOURCE_DIR}/External/Renderer/lz4/xxhash.c
	${CMAKE_SOURCE_DIR}/External/Renderer/MojoShader/mojoshader_common.cpp
	${CMAKE_SOURCE_DIR}/External/Renderer/MojoShader/mojoshader_lexer.cpp
	${CMAKE_SOURCE_DIR}/External/Renderer/MojoShader/mojoshader_preprocessor.cpp
	${CMAKE_SOURCE_DIR}/External/Renderer/mikktspace/mikktspace.c
)
if(RENDERER_IMGUI)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		${CMAKE_SOURCE_DIR}/External/Renderer/ImGui/imgui.cpp
		${CMAKE_SOURCE_DIR}/External/Renderer/ImGui/imgui_draw.cpp
		${CMAKE_SOURCE_DIR}/External/Renderer/ImGui/imgui_widgets.cpp
		${CMAKE_SOURCE_DIR}/External/Renderer/ImGuizmo/ImGuizmo.cpp
	)
endif()
set(SOURCE_CODES
	${EXTERNAL_SOURCE_CODES}
	RendererUnityBuild.cpp
	Public/Resource/Scene/Culling/SceneCullingManager.cpp	# Not compatible with "Advanced Vector Extensions 2" (/arch:AVX2)
)

# Disable warnings in external libraries, we can't fix them
if(WIN32)
	# Used options:
	# - Disable "warning C4201: nonstandard extension used: nameless struct/union""
	# - Disable "warning C4204: nonstandard extension used: non-constant aggregate initializer"
	# - Disable "warning C4255: '__slwpcb': no function prototype given: converting '()' to '(void)'"
	# - Disable "warning C4365: '=': conversion from 'ImGuiTabItemFlags' to 'ImGuiID', signed/unsigned mismatch""
	# - Disable "warning C4456: declaration of 'i' hides previous local declaration"
	# - Disable "warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc"
	# - Disable "warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'"
	# - Disable "warning C4774: 'sscanf' : format string expected in argument 2 is not a string literal"
	set_source_files_properties(${EXTERNAL_SOURCE_CODES} PROPERTIES COMPILE_FLAGS "/wd4201 /wd4204 /wd4255 /wd4365 /wd4456 /wd4530 /wd4668 /wd4774")
endif()


##################################################
## Libraries
##################################################
add_library(Renderer ${LIBRARY_TYPE} ${SOURCE_CODES})
if(SHARED_LIBRARY)
	set_target_properties(Renderer PROPERTIES COMPILE_FLAGS "-DSHARED_LIBRARIES -DRENDERER_EXPORTS")
	if(ANDROID)
		target_link_libraries(Renderer dl ${ANDROID_CXX_STANDARD_LIBRARIES})
	elseif(UNIX)
		target_link_libraries(Renderer dl pthread)
	endif()
endif()


##################################################
## Post build
##################################################
if(WIN32)
	if(SHARED_LIBRARY)
		add_custom_command(TARGET Renderer COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Renderer.lib" ${OUTPUT_LIBRARY_DIRECTORY})
	endif()
	
	# Copy OpenVR dll
	if(RENDERER_OPENVR)
		add_custom_command(TARGET Renderer COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/External/Renderer/OpenVR/Windows_${ARCHITECTURE}/openvr_api.dll" ${OUTPUT_BINARY_DIRECTORY})
	endif()
endif()


##################################################
## Install
##################################################
install(TARGETS Renderer
		RUNTIME DESTINATION "${OUTPUT_LIBRARY_BINARY_DIRECTORY}"
		ARCHIVE DESTINATION "${OUTPUT_LIBRARY_DIRECTORY}")
