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
set(EXTERNAL_SOURCE_CODES)
if(EXAMPLES_MIMALLOC)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# mimalloc external library
		${CMAKE_SOURCE_DIR}/External/Rhi/mimalloc/MimallocUnityBuild.cpp
	)
endif()
set(SOURCE_CODES
	${EXTERNAL_SOURCE_CODES}
	Private/Main.cpp
	Private/CommandLineArguments.cpp
)

# Add a natvis file for better debug support in Visual Studio ( https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2019 )
if(MSVC)
	set(SOURCE_CODES ${SOURCE_CODES} ${CMAKE_SOURCE_DIR}/Source/Rhi/Rhi.natvis ${CMAKE_SOURCE_DIR}/Source/Renderer/Renderer.natvis)
endif(MSVC)


##################################################
## Executables
##################################################
if(NOT SHARED_LIBRARY)
	if(WIN32)
		add_executable(ExampleProjectCompiler WIN32 ${SOURCE_CODES})

		# We want to have a console application (see https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/recipe/VSConfigSpecificSettings )
		set_target_properties(ExampleProjectCompiler PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
		set_target_properties(ExampleProjectCompiler PROPERTIES COMPILE_DEFINITIONS "_CONSOLE")

		# Set Visual Studio debugger working directory (see https://stackoverflow.com/a/42973332 )
		set_target_properties(ExampleProjectCompiler PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Binary/${OS_ARCHITECTURE}/")
	else()
		add_executable(ExampleProjectCompiler ${SOURCE_CODES})
	endif()
	set(LIBRARIES ${LIBRARIES} RendererToolkit Renderer)
	if(UNIX)
		set(LIBRARIES ${LIBRARIES} pthread)
	endif()
	target_link_libraries(ExampleProjectCompiler ${LIBRARIES})
	add_dependencies(ExampleProjectCompiler Renderer RendererToolkit)
endif()
if(SHARED_LIBRARY)
	if(WIN32)
		add_executable(ExampleProjectCompiler WIN32 ${SOURCE_CODES})
		target_link_libraries(ExampleProjectCompiler Renderer.lib)

		# We want to have a console application (see https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/recipe/VSConfigSpecificSettings )
		set_target_properties(ExampleProjectCompiler PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
		set_target_properties(ExampleProjectCompiler PROPERTIES COMPILE_DEFINITIONS "_CONSOLE")

		# Set Visual Studio debugger working directory (see https://stackoverflow.com/a/42973332 )
		set_target_properties(ExampleProjectCompiler PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Binary/${OS_ARCHITECTURE}/")
	else()
		add_executable(ExampleProjectCompiler ${SOURCE_CODES})
		target_link_libraries(ExampleProjectCompiler Renderer stdc++fs)
	endif()
	add_dependencies(ExampleProjectCompiler Renderer)
	set_target_properties(ExampleProjectCompiler PROPERTIES COMPILE_FLAGS -DSHARED_LIBRARIES)
endif()


##################################################
## Preprocessor definitions
##################################################
unrimp_add_conditional_definition(ExampleProjectCompiler ARCHITECTURE_X64)
if(RHI_DEBUG)
	target_compile_definitions(ExampleProjectCompiler PRIVATE RHI_DEBUG)
	target_compile_definitions(ExampleProjectCompiler PRIVATE RHI_STATISTICS)
endif()
if(WIN32)
	target_compile_definitions(ExampleProjectCompiler PRIVATE UNICODE)
endif()
unrimp_add_conditional_rhi_definitions(ExampleProjectCompiler)
if(EXAMPLES_MIMALLOC)
	target_compile_definitions(ExampleProjectCompiler PRIVATE EXAMPLES_MIMALLOC)
endif()


##################################################
## Includes
##################################################
target_include_directories(ExampleProjectCompiler PRIVATE ${CMAKE_SOURCE_DIR}/Example/Source
														  ${CMAKE_SOURCE_DIR}/Source)
if(EXAMPLES_MIMALLOC)
	target_include_directories(ExampleProjectCompiler PRIVATE ${CMAKE_SOURCE_DIR}/External/Rhi/mimalloc/include)
endif()
target_link_directories(ExampleProjectCompiler PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


##################################################
## Install
##################################################
install(TARGETS ExampleProjectCompiler RUNTIME DESTINATION "${OUTPUT_BINARY_DIRECTORY}")
