#/*********************************************************\
# * Copyright (c) 2012-2021 The Unrimp Team
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
## Source codes
##################################################
set(SOURCE_CODES
	Private/Main.cpp
)

# Add a natvis file for better debug support in Visual Studio ( https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2019 )
if(MSVC)
	set(SOURCE_CODES ${SOURCE_CODES} ${CMAKE_SOURCE_DIR}/Source/Rhi/Rhi.natvis ${CMAKE_SOURCE_DIR}/Source/Renderer/Renderer.natvis)
endif(MSVC)


##################################################
## Executables
##################################################
if(WIN32)
	add_executable(ExampleSDL2 WIN32 ${SOURCE_CODES})

	# Set Visual Studio debugger working directory (see https://stackoverflow.com/a/42973332 )
	set_target_properties(ExampleSDL2 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Binary/${OS_ARCHITECTURE}/")
else()
	add_executable(ExampleSDL2 ${SOURCE_CODES})
endif()
if(SHARED_LIBRARY)
	set_target_properties(ExampleSDL2 PROPERTIES COMPILE_FLAGS -DSHARED_LIBRARIES)
else()
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_NULL NullRhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_VULKAN VulkanRhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_OPENGL OpenGLRhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_OPENGLES3 OpenGLES3Rhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_DIRECT3D9 Direct3D9Rhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_DIRECT3D10 Direct3D10Rhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_DIRECT3D11 Direct3D11Rhi)
	unrimp_add_conditional_library_dependency(ExampleSDL2 RHI_DIRECT3D12 Direct3D12Rhi)
	target_link_libraries(ExampleSDL2 ${LIBRARIES})
endif()
target_link_libraries(ExampleSDL2 ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY})


##################################################
## Preprocessor definitions
##################################################
unrimp_add_conditional_definition(ExampleSDL2 ARCHITECTURE_X64)
if(RHI_DEBUG)
	target_compile_definitions(ExampleSDL2 PRIVATE RHI_DEBUG)
	target_compile_definitions(ExampleSDL2 PRIVATE RHI_STATISTICS)
endif()
unrimp_add_conditional_rhi_definitions(ExampleSDL2)


##################################################
## Includes
##################################################
target_include_directories(ExampleSDL2 PRIVATE ${CMAKE_SOURCE_DIR}/Example/Source
											   ${CMAKE_SOURCE_DIR}/Source
											   ${SDL2_INCLUDE_DIR})
target_link_directories(ExampleSDL2 PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
if(RHI_DIRECT3D11)
	target_link_directories(ExampleSDL2 PRIVATE ${CMAKE_SOURCE_DIR}/External/Rhi/AGS_SDK/Windows_${ARCHITECTURE}/Library)
endif()


##################################################
## Post build
##################################################
# Copy the SDL2 shared library, if necessary
if(WIN32 AND SDL2_FOUND)
	message("SDL copy " "${CMAKE_SOURCE_DIR}/External/Example/SDL2/lib/${ARCHITECTURE}/SDL2.dll")
	add_custom_command(TARGET ExampleSDL2 COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/External/Example/SDL2/lib/${ARCHITECTURE}/SDL2.dll" ${OUTPUT_BINARY_DIRECTORY})
endif()


##################################################
## Install
##################################################
install(TARGETS ExampleSDL2 RUNTIME DESTINATION "${OUTPUT_BINARY_DIRECTORY}")
