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
## Subdirectories
##################################################
if(RENDERER_NULL)
	add_subdirectory(NullRenderer)
endif()
if(RENDERER_OPENGL)
	add_subdirectory(OpenGLRenderer)
endif()
if(RENDERER_OPENGLES3)
	add_subdirectory(OpenGLES3Renderer)
endif()
if(RENDERER_VULKAN)
	add_subdirectory(VulkanRenderer)
endif()
if(RENDERER_DIRECT3D9)
	add_subdirectory(Direct3D9Renderer)
endif()
if(RENDERER_DIRECT3D10)
	add_subdirectory(Direct3D10Renderer)
endif()
if(RENDERER_DIRECT3D11)
	add_subdirectory(Direct3D11Renderer)
endif()
if(RENDERER_DIRECT3D12)
	add_subdirectory(Direct3D12Renderer)
endif()
