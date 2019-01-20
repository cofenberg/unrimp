/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


/**
*  @brief
*    OpenGL renderer amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    - OpenGL capable graphics driver
*    - OpenGL headers which can be found at "<unrimp>\External\OpenGL\include\"
*    - smol-v (directly compiled and linked in)
*    - glslang if "RENDERER_OPENGL_GLSLTOSPIRV" is set (directly compiled and linked in)
*
*    == Preprocessor Definitions ==
*    - Set "RENDERER_OPENGL_EXPORTS" as preprocessor definition when building this library as shared library
*    - If this renderer was compiled with "RENDERER_OPENGL_STATE_CLEANUP" set as preprocessor definition, the previous OpenGL state will be restored after performing an operation (worse performance, increases the binary size slightly, might avoid unexpected behaviour when using OpenGL directly beside this renderer)
*    - Set "RENDERER_OPENGL_GLSLTOSPIRV" as preprocessor definition when building this library to add support for compiling GLSL into SPIR-V, increases the binary size around one MiB
*    - Do also have a look into the renderer header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Public/Renderer.h>

#ifdef RENDERER_OPENGL_GLSLTOSPIRV
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator '<x>' in switch of enum '<y>' is not explicitly handled by a case label
		PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from '<x>' to '<y>', signed/unsigned mismatch
		PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
		PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
		PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_List_node<_Ty,std::_Default_allocator_traits<_Alloc>::void_pointer>': default constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: '<x>': copy constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
		PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
		#include <SPIRV/GlslangToSpv.h>
		#include <glslang/MachineIndependent/localintermediate.h>
	PRAGMA_WARNING_POP
#endif

#include <smol-v/smolv.h>

#ifdef _WIN32
	// Set Windows version to Windows Vista (0x0600), we don't support Windows XP (0x0501)
	#ifdef WINVER
		#undef WINVER
	#endif
	#define WINVER			0x0600
	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
	#endif
	#define _WIN32_WINNT	0x0600

	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '<x>' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
		PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
		#undef GL_GLEXT_PROTOTYPES
		#include <GL/gl.h>
		#include <GL/glext.h>	// Requires definitions from "gl.h"
		#include <GL/wglext.h>	// Requires definitions from "gl.h"
	PRAGMA_WARNING_POP

	// Exclude some stuff from "windows.h" to speed up compilation a bit
	#define NOGDICAPMASKS
	#define NOMENUS
	#define NOICONS
	#define NOKEYSTATES
	#define NOSYSCOMMANDS
	#define NORASTEROPS
	#define OEMRESOURCE
	#define NOATOM
	#define NOMEMMGR
	#define NOMETAFILE
	#define NOOPENFILE
	#define NOSCROLL
	#define NOSERVICE
	#define NOSOUND
	#define NOWH
	#define NOCOMM
	#define NOKANJI
	#define NOHELP
	#define NOPROFILER
	#define NODEFERWINDOWPOS
	#define NOMCX
	#define NOCRYPT
	#include <windows.h>

	// Get rid of some nasty OS macros
	#undef max
#elif LINUX
	// Get rid of some nasty OS macros
	#undef None	// Linux: Undefine "None", this name is used inside enums defined by Unrimp (which gets defined inside "Xlib.h" pulled in by "glx.h")

	#ifdef LINUX
		#include <GL/glx.h>
		#include <GL/glxext.h>
	#endif
	#include <GL/gl.h>
	#include <GL/glext.h>	// Requires definitions from "gl.h"

	// TODO(co) Review which of the following headers can be removed
	#include <X11/Xlib.h>

	#include <dlfcn.h>
	#include <link.h>
	#include <iostream>	// TODO(co) Can this include be removed?

	// Need to redefine "None"-macro (which got undefined in "Extensions.h" due name clashes used in enums)
	#ifndef None
		#define None 0L	///< Universal null resource or null atom
	#endif
#else
	#error "Unsupported platform"
#endif




//[-------------------------------------------------------]
//[ OpenGLRenderer/MakeID.h                               ]
//[-------------------------------------------------------]
/*

Author:
	Emil Persson, A.K.A. Humus.
	http://www.humus.name

Version history:
	1.0  - Initial release.
	1.01 - Code review fixes. Code reviewed by Denis A. Gladkiy.
	1.02 - Fixed an off-by-one error in DestroyRange() found by Markus Billeter

License:
	Public Domain

	This file is released in the hopes that it will be useful. Use in whatever way you like, but no guarantees that it
	actually works or fits any particular purpose. It has been unit-tested and benchmarked though, and seems to do
	what it was designed to do, and seems pretty quick at it too.

Notes: 
	There are many applications where it is desired to generate unique IDs at runtime for various resources, such that they can be
	distinguished, sorted or otherwise processed in an efficient manner. It can in some cases replace hashes, handles and pointers.
	In cases where resource pointers are used as IDs, it offers a unique ID that requires far fewer bits, especially for 64bit apps.
	The design goal of this implementation was to return the most compact IDs as possible, limiting to a specific range if necessary.

	The properties of this system are as follows:
		- Creating a new ID returns the smallest possible unused ID.
		- Creating a new range of IDs returns the smallest possible continuous range of the specified size.
		- Created IDs remain valid until destroyed.
		- Destroying an ID returns it to the pool and may be returned by subsequent allocations.
		- The system is NOT thread-safe.

	Performance properties:
		- Creating an ID is O(1) and generally super-cheap.
		- Destroying an ID is also cheap, but O(log(n)), where n is the current number of distinct available ranges.
		- The system merges available ranges when IDs are destroyed, keeping said n generally very small in practice.
		- After warmup, no further memory allocations should be necessary, or be very rare.
		- The system uses very little memory.
		- It is possible to construct a pathological case where fragmentation would cause n to become large. This can be done by
		  first allocating a very large range of IDs, then deleting every other ID, causing a new range to be allocated for every
		  free ID, or as many ranges as there are free IDs. I believe nothing close to this situation happens in practical applications.
		  In tests, millions of random scattered creations and deletions only resulted in a relatively short list in the worst case.
		  This is because freed IDs are quickly reused and ranges eagerly merged.

	Where would this system be useful? It was originally thought up as a replacement for resource pointers as part of sort-ids
	in rendering. Using for instance a 64-bit sort-id packing various flags and states, putting a pointer in there takes an
	awful lot of bits, especially considering the actual possible resources range in the thousands at most. This got far worse
	of course with the switch to 64bit as pointers are now twice as large and essentially eats all bits except bottom few for
	alignment.
	Another application would be for managing a shared pool of resources. IDs could be handed out as handles and used to access
	the actual resource from an array. By always returning the lowest possible ID or range of IDs we get very good cache behavior
	since all active resources will grouped together in the bottom part of the array. Using IDs instead of pointers for handles
	also allows easy resizing of the allocated memory since IDs can remain the same even if the underlying storage changed.

*/


#ifdef _DEBUG
	#include <cstdio>  // For printf(). Remove if you don't need the PrintRanges() function (mostly for debugging anyway).
#endif

#include <cstdint>	// uint32_t
#include <limits>	// std::numeric_limits<type>::max()
#include <cstdlib>
#include <cstring>

class MakeID final
{
private:
	// Change to uint16_t here for a more compact implementation if 16bit or less IDs work for you.
	typedef uint16_t uint;

	struct Range
	{
		uint m_First;
		uint m_Last;
	};

	Renderer::IAllocator& m_Allocator;
	Range *m_Ranges; // Sorted array of ranges of free IDs
	uint m_Count;    // Number of ranges in list
	uint m_Capacity; // Total capacity of range list

	MakeID & operator=(const MakeID &) = delete;
	MakeID(const MakeID &) = delete;

public:
	MakeID(Renderer::IAllocator& allocator, const uint max_id = std::numeric_limits<uint>::max()) :
		m_Allocator(allocator),
		m_Ranges(static_cast<Range*>(allocator.reallocate(nullptr, 0, sizeof(Range), 1))),
		m_Count(1),
		m_Capacity(1)
	{
		// Start with a single range, from 0 to max allowed ID (specified)
		m_Ranges[0].m_First = 0;
		m_Ranges[0].m_Last = max_id;
	}

	~MakeID()
	{
		m_Allocator.reallocate(m_Ranges, 0, 0, 1);
	}

	bool CreateID(uint &id)
	{
		if (m_Ranges[0].m_First <= m_Ranges[0].m_Last)
		{
			id = m_Ranges[0].m_First;

			// If current range is full and there is another one, that will become the new current range
			if (m_Ranges[0].m_First == m_Ranges[0].m_Last && m_Count > 1)
			{
				DestroyRange(0);
			}
			else
			{
				++m_Ranges[0].m_First;
			}
			return true;
		}

		// No available ID left
		return false;
	}

	bool CreateRangeID(uint &id, const uint count)
	{
		uint i = 0;
		do
		{
			const uint range_count = 1u + m_Ranges[i].m_Last - m_Ranges[i].m_First;
			if (count <= range_count)
			{
				id = m_Ranges[i].m_First;

				// If current range is full and there is another one, that will become the new current range
				if (count == range_count && i + 1 < m_Count)
				{
					DestroyRange(i);
				}
				else
				{
					m_Ranges[i].m_First += count;
				}
				return true;
			}
			++i;
		} while (i < m_Count);

		// No range of free IDs was large enough to create the requested continuous ID sequence
		return false;
	}

	bool DestroyID(const uint id)
	{
		return DestroyRangeID(id, 1);
	}

	bool DestroyRangeID(const uint id, const uint count)
	{
		const uint end_id = static_cast<uint>(id + count);

		// Binary search of the range list
		uint i0 = 0u;
		uint i1 = m_Count - 1u;

		for (;;)
		{
			const uint i = (i0 + i1) / 2u;

			if (id < m_Ranges[i].m_First)
			{
				// Before current range, check if neighboring
				if (end_id >= m_Ranges[i].m_First)
				{
					if (end_id != m_Ranges[i].m_First)
						return false; // Overlaps a range of free IDs, thus (at least partially) invalid IDs

					// Neighbor id, check if neighboring previous range too
					if (i > i0 && id - 1 == m_Ranges[i - 1].m_Last)
					{
						// Merge with previous range
						m_Ranges[i - 1].m_Last = m_Ranges[i].m_Last;
						DestroyRange(i);
					}
					else
					{
						// Just grow range
						m_Ranges[i].m_First = id;
					}
					return true;
				}
				else
				{
					// Non-neighbor id
					if (i != i0)
					{
						// Cull upper half of list
						i1 = i - 1u;
					}
					else
					{
						// Found our position in the list, insert the deleted range here
						InsertRange(i);
						m_Ranges[i].m_First = id;
						m_Ranges[i].m_Last = end_id - 1u;
						return true;
					}
				}
			}
			else if (id > m_Ranges[i].m_Last)
			{
				// After current range, check if neighboring
				if (id - 1 == m_Ranges[i].m_Last)
				{
					// Neighbor id, check if neighboring next range too
					if (i < i1 && end_id == m_Ranges[i + 1].m_First)
					{
						// Merge with next range
						m_Ranges[i].m_Last = m_Ranges[i + 1].m_Last;
						DestroyRange(i + 1u);
					}
					else
					{
						// Just grow range
						m_Ranges[i].m_Last += count;
					}
					return true;
				}
				else
				{
					// Non-neighbor id
					if (i != i1)
					{
						// Cull bottom half of list
						i0 = i + 1u;
					}
					else
					{
						// Found our position in the list, insert the deleted range here
						InsertRange(i + 1u);
						m_Ranges[i + 1].m_First = id;
						m_Ranges[i + 1].m_Last = end_id - 1u;
						return true;
					}
				}
			}
			else
			{
				// Inside a free block, not a valid ID
				return false;
			}

		}
	}

	bool IsID(const uint id) const
	{
		// Binary search of the range list
		uint i0 = 0u;
		uint i1 = m_Count - 1u;

		for (;;)
		{
			const uint i = (i0 + i1) / 2u;

			if (id < m_Ranges[i].m_First)
			{
				if (i == i0)
					return true;

				// Cull upper half of list
				i1 = i - 1u;
			}
			else if (id > m_Ranges[i].m_Last)
			{
				if (i == i1)
					return true;

				// Cull bottom half of list
				i0 = i + 1u;
			}
			else
			{
				// Inside a free block, not a valid ID
				return false;
			}

		}
	}

	uint GetAvailableIDs() const
	{
		uint count = m_Count;
		uint i = 0;

		do
		{
			count += m_Ranges[i].m_Last - m_Ranges[i].m_First;
			++i;
		} while (i < m_Count);

		return count;
	}

	uint GetLargestContinuousRange() const
	{
		uint max_count = 0;
		uint i = 0;

		do
		{
			uint count = m_Ranges[i].m_Last - m_Ranges[i].m_First + 1u;
			if (count > max_count)
				max_count = count;

			++i;
		} while (i < m_Count);

		return max_count;
	}

	#ifdef _DEBUG
		void PrintRanges() const
		{
			uint i = 0;
			for (;;)
			{
				if (m_Ranges[i].m_First < m_Ranges[i].m_Last)
					printf("%u-%u", m_Ranges[i].m_First, m_Ranges[i].m_Last);
				else if (m_Ranges[i].m_First == m_Ranges[i].m_Last)
					printf("%u", m_Ranges[i].m_First);
				else
					printf("-");

				++i;
				if (i >= m_Count)
				{
					printf("\n");
					return;
				}

				printf(", ");
			}
		}
	#endif


private:

	void InsertRange(const uint index)
	{
		if (m_Count >= m_Capacity)
		{
			m_Ranges = static_cast<Range *>(m_Allocator.reallocate(m_Ranges, sizeof(Range) * m_Capacity, (m_Capacity + m_Capacity) * sizeof(Range), 1));
			m_Capacity += m_Capacity;
		}
 
		::memmove(m_Ranges + index + 1, m_Ranges + index, (m_Count - index) * sizeof(Range));
		++m_Count;
	}

	void DestroyRange(const uint index)
	{
		--m_Count;
		::memmove(m_Ranges + index, m_Ranges + index + 1, (m_Count - index) * sizeof(Range));
	}
};




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace OpenGLRenderer
{
	class Extensions;
	class VertexArray;
	class RootSignature;
	class IOpenGLContext;
	class OpenGLRuntimeLinking;
	class ComputePipelineState;
	class GraphicsPipelineState;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
/*
*  @brief
*    Check whether or not the given resource is owned by the given renderer
*/
#ifdef RENDERER_DEBUG
	#define OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference) \
		RENDERER_ASSERT(mContext, &rendererReference == &(resourceReference).getRenderer(), "OpenGL error: The given resource is owned by another renderer instance")
#else
	#define OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference)
#endif


//[-------------------------------------------------------]
//[ OpenGL functions                                      ]
//[-------------------------------------------------------]
#define FNDEF_GL(retType, funcName, args) retType (GLAPIENTRY *funcPtr_##funcName) args
FNDEF_GL(GLubyte*,	glGetString,		(GLenum));
FNDEF_GL(void,		glGetIntegerv,		(GLenum, GLint*));
FNDEF_GL(void,		glBindTexture,		(GLenum, GLuint));
FNDEF_GL(void,		glClear,			(GLbitfield));
FNDEF_GL(void,		glClearStencil,		(GLint));
FNDEF_GL(void,		glClearDepth,		(GLclampd));
FNDEF_GL(void,		glClearColor,		(GLclampf, GLclampf, GLclampf, GLclampf));
FNDEF_GL(void,		glDrawArrays,		(GLenum, GLint, GLsizei));
FNDEF_GL(void,		glDrawElements,		(GLenum, GLsizei, GLenum, const GLvoid*));
FNDEF_GL(void,		glColor4f,			(GLfloat, GLfloat, GLfloat, GLfloat));
FNDEF_GL(void,		glEnable,			(GLenum));
FNDEF_GL(void,		glDisable,			(GLenum));
FNDEF_GL(void,		glBlendFunc,		(GLenum, GLenum));
FNDEF_GL(void,		glFrontFace,		(GLenum));
FNDEF_GL(void,		glCullFace,			(GLenum));
FNDEF_GL(void,		glPolygonMode,		(GLenum, GLenum));
FNDEF_GL(void,		glTexParameteri,	(GLenum, GLenum, GLint));
FNDEF_GL(void,		glGenTextures,		(GLsizei, GLuint*));
FNDEF_GL(void,		glDeleteTextures,	(GLsizei, const GLuint*));
FNDEF_GL(void,		glTexImage1D,		(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
FNDEF_GL(void,		glTexImage2D,		(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*));
FNDEF_GL(void,		glPixelStorei,		(GLenum, GLint));
FNDEF_GL(void,		glDepthFunc,		(GLenum func));
FNDEF_GL(void,		glDepthMask,		(GLboolean));
FNDEF_GL(void,		glViewport,			(GLint, GLint, GLsizei, GLsizei));
FNDEF_GL(void,		glDepthRange,		(GLclampd, GLclampd));
FNDEF_GL(void,		glScissor,			(GLint, GLint, GLsizei, GLsizei));
FNDEF_GL(void,		glFlush,			(void));
FNDEF_GL(void,		glFinish,			(void));

// >= OpenGL 3.0
FNDEF_GL(GLubyte *,	glGetStringi,	(GLenum, GLuint));

// >= OpenGL 4.5
FNDEF_GL(void,	glCreateQueries,	(GLenum, GLsizei, GLuint*));

// Platform specific
#ifdef _WIN32
	FNDEF_GL(HDC,	wglGetCurrentDC,	(VOID));
	FNDEF_GL(PROC,	wglGetProcAddress,	(LPCSTR));
	FNDEF_GL(HGLRC,	wglCreateContext,	(HDC));
	FNDEF_GL(BOOL,	wglDeleteContext,	(HGLRC));
	FNDEF_GL(BOOL,	wglMakeCurrent,		(HDC, HGLRC));
#elif LINUX
	FNDEF_GL(Bool,			  glXMakeCurrent,			(Display*, GLXDrawable, GLXContext));
	FNDEF_GL(XVisualInfo*,	  glXChooseVisual,			(Display*, int, int*));
	FNDEF_GL(GLXContext,	  glXCreateContext,			(Display*, XVisualInfo*, GLXContext, int));
	FNDEF_GL(void,			  glXDestroyContext,		(Display*, GLXContext));
	FNDEF_GL(GLXContext,	  glXGetCurrentContext,		(void));
	FNDEF_GL(const char*,	  glXQueryExtensionsString, (Display*, int));
	FNDEF_GL(__GLXextFuncPtr, glXGetProcAddress,		(const GLubyte*));
	FNDEF_GL(__GLXextFuncPtr, glXGetProcAddressARB,		(const GLubyte*));
	FNDEF_GL(GLXFBConfig*,	  glXChooseFBConfig,		(Display*, int, const int*, int*));
	FNDEF_GL(void,			  glXSwapBuffers,			(Display*, GLXDrawable));
	FNDEF_GL(const char*,	  glXGetClientString,		(Display*, int));
#else
	#error "Unsupported platform"
#endif

// Redirect gl* function calls to funcPtr_gl*
#ifndef FNPTR
	#define FNPTR(name) funcPtr_##name
#endif

// OpenGL
#define glGetString			FNPTR(glGetString)
#define glGetIntegerv		FNPTR(glGetIntegerv)
#define glBindTexture		FNPTR(glBindTexture)
#define glClear				FNPTR(glClear)
#define glClearStencil		FNPTR(glClearStencil)
#define glClearDepth		FNPTR(glClearDepth)
#define glClearColor		FNPTR(glClearColor)
#define glDrawArrays		FNPTR(glDrawArrays)
#define glDrawElements		FNPTR(glDrawElements)
#define glColor4f			FNPTR(glColor4f)
#define glEnable			FNPTR(glEnable)
#define glDisable			FNPTR(glDisable)
#define glBlendFunc			FNPTR(glBlendFunc)
#define glFrontFace			FNPTR(glFrontFace)
#define glCullFace			FNPTR(glCullFace)
#define glPolygonMode		FNPTR(glPolygonMode)
#define glTexParameteri		FNPTR(glTexParameteri)
#define glGenTextures		FNPTR(glGenTextures)
#define glDeleteTextures	FNPTR(glDeleteTextures)
#define glTexImage1D		FNPTR(glTexImage1D)
#define glTexImage2D		FNPTR(glTexImage2D)
#define glPixelStorei		FNPTR(glPixelStorei)
#define glDepthFunc			FNPTR(glDepthFunc)
#define glDepthMask			FNPTR(glDepthMask)
#define glViewport			FNPTR(glViewport)
#define glDepthRange		FNPTR(glDepthRange)
#define glScissor			FNPTR(glScissor)
#define glFlush				FNPTR(glFlush)
#define glFinish			FNPTR(glFinish)

// >= OpenGL 3.0
#define glGetStringi FNPTR(glGetStringi)

// >= OpenGL 4.5
#define glCreateQueries FNPTR(glCreateQueries)

// Platform specific
#ifdef _WIN32
	#define wglGetCurrentDC		FNPTR(wglGetCurrentDC)
	#define wglGetProcAddress	FNPTR(wglGetProcAddress)
	#define wglCreateContext	FNPTR(wglCreateContext)
	#define wglDeleteContext	FNPTR(wglDeleteContext)
	#define wglMakeCurrent		FNPTR(wglMakeCurrent)
#elif LINUX
	#define glXMakeCurrent				FNPTR(glXMakeCurrent)
	#define glXGetProcAddress			FNPTR(glXGetProcAddress)
	#define glXGetProcAddressARB		FNPTR(glXGetProcAddressARB)
	#define glXChooseVisual				FNPTR(glXChooseVisual)
	#define glXCreateContext			FNPTR(glXCreateContext)
	#define glXDestroyContext			FNPTR(glXDestroyContext)
	#define glXGetCurrentContext		FNPTR(glXGetCurrentContext)
	#define glXQueryExtensionsString	FNPTR(glXQueryExtensionsString)
	#define glXChooseFBConfig			FNPTR(glXChooseFBConfig)
	#define glXSwapBuffers				FNPTR(glXSwapBuffers)
	#define glXGetClientString			FNPTR(glXGetClientString)
#else
	#error "Unsupported platform"
#endif

// Define a helper macro
#ifdef _WIN32
	#define IMPORT_FUNC(funcName)																																					\
		if (result)																																									\
		{																																											\
			void* symbol = ::GetProcAddress(static_cast<HMODULE>(mOpenGLSharedLibrary), #funcName);																					\
			if (nullptr == symbol)																																					\
			{																																										\
				symbol = wglGetProcAddress(#funcName);																																\
			}																																										\
			if (nullptr != symbol)																																					\
			{																																										\
				*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
			}																																										\
			else																																									\
			{																																										\
				wchar_t moduleFilename[MAX_PATH];																																	\
				moduleFilename[0] = '\0';																																			\
				::GetModuleFileNameW(static_cast<HMODULE>(mOpenGLSharedLibrary), moduleFilename, MAX_PATH);																			\
				RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library \"%s\"", #funcName, moduleFilename)	\
				result = false;																																						\
			}																																										\
		}
#elif LINUX
	#define IMPORT_FUNC(funcName)																																				\
		if (result)																																								\
		{																																										\
			void* symbol = ::dlsym(mOpenGLSharedLibrary, #funcName);																											\
			if (nullptr != symbol)																																				\
			{																																									\
				*(reinterpret_cast<void**>(&(funcName))) = symbol;																												\
			}																																									\
			else																																								\
			{																																									\
				link_map *linkMap = nullptr;																																	\
				const char* libraryName = "unknown";																															\
				if (dlinfo(mOpenGLSharedLibrary, RTLD_DI_LINKMAP, &linkMap))																									\
				{																																								\
					libraryName = linkMap->l_name;																																\
				}																																								\
				RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library \"%s\"", #funcName, libraryName)	\
				result = false;																																					\
			}																																									\
		}
#else
	#error "Unsupported platform"
#endif


//[-------------------------------------------------------]
//[ OpenGL extension functions                            ]
//[-------------------------------------------------------]
#define FNDEF_EX(funcName, funcTypedef) funcTypedef funcName = nullptr

//[-------------------------------------------------------]
//[ WGL (Windows only) definitions                        ]
//[-------------------------------------------------------]
#ifdef _WIN32
	// WGL_ARB_extensions_string
	FNDEF_EX(wglGetExtensionsStringARB,	PFNWGLGETEXTENSIONSSTRINGARBPROC);

	// WGL_EXT_swap_control
	FNDEF_EX(wglSwapIntervalEXT,	PFNWGLSWAPINTERVALEXTPROC);
#endif

//[-------------------------------------------------------]
//[ EXT                                                   ]
//[-------------------------------------------------------]
// GL_EXT_texture3D
FNDEF_EX(glTexImage3DEXT,		PFNGLTEXIMAGE3DEXTPROC);
FNDEF_EX(glTexSubImage3DEXT,	PFNGLTEXSUBIMAGE3DEXTPROC);

// GL_EXT_direct_state_access
FNDEF_EX(glNamedBufferDataEXT,					PFNGLNAMEDBUFFERDATAEXTPROC);
FNDEF_EX(glNamedBufferSubDataEXT,				PFNGLNAMEDBUFFERSUBDATAEXTPROC);
FNDEF_EX(glMapNamedBufferEXT,					PFNGLMAPNAMEDBUFFEREXTPROC);
FNDEF_EX(glUnmapNamedBufferEXT,					PFNGLUNMAPNAMEDBUFFEREXTPROC);
FNDEF_EX(glProgramUniform1iEXT,					PFNGLPROGRAMUNIFORM1IEXTPROC);
FNDEF_EX(glProgramUniform1uiEXT,				PFNGLPROGRAMUNIFORM1UIEXTPROC);
FNDEF_EX(glProgramUniform1fEXT,					PFNGLPROGRAMUNIFORM1FEXTPROC);
FNDEF_EX(glProgramUniform2fvEXT,				PFNGLPROGRAMUNIFORM2FVEXTPROC);
FNDEF_EX(glProgramUniform3fvEXT,				PFNGLPROGRAMUNIFORM3FVEXTPROC);
FNDEF_EX(glProgramUniform4fvEXT,				PFNGLPROGRAMUNIFORM4FVEXTPROC);
FNDEF_EX(glProgramUniformMatrix3fvEXT,			PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC);
FNDEF_EX(glProgramUniformMatrix4fvEXT,			PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC);
FNDEF_EX(glTextureImage1DEXT,					PFNGLTEXTUREIMAGE1DEXTPROC);
FNDEF_EX(glTextureImage2DEXT,					PFNGLTEXTUREIMAGE2DEXTPROC);
FNDEF_EX(glTextureImage3DEXT,					PFNGLTEXTUREIMAGE3DEXTPROC);
FNDEF_EX(glTextureSubImage3DEXT,				PFNGLTEXTURESUBIMAGE3DEXTPROC);
FNDEF_EX(glTextureParameteriEXT,				PFNGLTEXTUREPARAMETERIEXTPROC);
FNDEF_EX(glGenerateTextureMipmapEXT,			PFNGLGENERATETEXTUREMIPMAPEXTPROC);
FNDEF_EX(glCompressedTextureImage1DEXT,			PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC);
FNDEF_EX(glCompressedTextureImage2DEXT,			PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC);
FNDEF_EX(glCompressedTextureImage3DEXT,			PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC);
FNDEF_EX(glVertexArrayVertexAttribOffsetEXT,	PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC);
FNDEF_EX(glEnableVertexArrayAttribEXT,			PFNGLENABLEVERTEXARRAYATTRIBEXTPROC);
FNDEF_EX(glBindMultiTextureEXT,					PFNGLBINDMULTITEXTUREEXTPROC);
FNDEF_EX(glNamedFramebufferTexture2DEXT,		PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC);
FNDEF_EX(glNamedFramebufferTextureLayerEXT,		PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC);
FNDEF_EX(glCheckNamedFramebufferStatusEXT,		PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC);
FNDEF_EX(glNamedRenderbufferStorageEXT,			PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC);	
FNDEF_EX(glNamedFramebufferRenderbufferEXT,		PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC);

// GL_EXT_shader_image_load_store
FNDEF_EX(glBindImageTextureEXT,	PFNGLBINDIMAGETEXTUREEXTPROC);
FNDEF_EX(glMemoryBarrierEXT,	PFNGLMEMORYBARRIEREXTPROC);

//[-------------------------------------------------------]
//[ KHR                                                   ]
//[-------------------------------------------------------]
// GL_KHR_debug
FNDEF_EX(glDebugMessageInsert,	PFNGLDEBUGMESSAGEINSERTPROC);
FNDEF_EX(glPushDebugGroup,		PFNGLPUSHDEBUGGROUPPROC);
FNDEF_EX(glPopDebugGroup,		PFNGLPOPDEBUGGROUPPROC);
FNDEF_EX(glObjectLabel,			PFNGLOBJECTLABELPROC);

//[-------------------------------------------------------]
//[ ARB                                                   ]
//[-------------------------------------------------------]
// GL_ARB_framebuffer_object
FNDEF_EX(glBindRenderbuffer,		PFNGLBINDRENDERBUFFERPROC);
FNDEF_EX(glDeleteRenderbuffers,		PFNGLDELETERENDERBUFFERSPROC);
FNDEF_EX(glGenRenderbuffers,		PFNGLGENRENDERBUFFERSPROC);
FNDEF_EX(glRenderbufferStorage,		PFNGLRENDERBUFFERSTORAGEPROC);
FNDEF_EX(glBindFramebuffer,			PFNGLBINDFRAMEBUFFERPROC);
FNDEF_EX(glDeleteFramebuffers,		PFNGLDELETEFRAMEBUFFERSPROC);
FNDEF_EX(glGenFramebuffers,			PFNGLGENFRAMEBUFFERSPROC);
FNDEF_EX(glCheckFramebufferStatus,	PFNGLCHECKFRAMEBUFFERSTATUSPROC);
FNDEF_EX(glFramebufferTexture2D,	PFNGLFRAMEBUFFERTEXTURE2DPROC);
FNDEF_EX(glFramebufferTextureLayer,	PFNGLFRAMEBUFFERTEXTURELAYERPROC);
FNDEF_EX(glFramebufferRenderbuffer,	PFNGLFRAMEBUFFERRENDERBUFFERPROC);
FNDEF_EX(glBlitFramebuffer,			PFNGLBLITFRAMEBUFFERPROC);
FNDEF_EX(glGenerateMipmap,			PFNGLGENERATEMIPMAPPROC);

// GL_ARB_multitexture
FNDEF_EX(glActiveTextureARB,	PFNGLACTIVETEXTUREARBPROC);

// GL_ARB_texture_multisample
FNDEF_EX(glTexImage2DMultisample,	PFNGLTEXIMAGE2DMULTISAMPLEPROC);

// GL_ARB_vertex_buffer_object
FNDEF_EX(glBindBufferARB,		PFNGLBINDBUFFERARBPROC);
FNDEF_EX(glDeleteBuffersARB,	PFNGLDELETEBUFFERSARBPROC);
FNDEF_EX(glGenBuffersARB,		PFNGLGENBUFFERSARBPROC);
FNDEF_EX(glBufferDataARB,		PFNGLBUFFERDATAARBPROC);
FNDEF_EX(glBufferSubDataARB,	PFNGLBUFFERSUBDATAARBPROC);
FNDEF_EX(glMapBufferARB,		PFNGLMAPBUFFERARBPROC);
FNDEF_EX(glUnmapBufferARB,		PFNGLUNMAPBUFFERARBPROC);

// GL_ARB_texture_compression
FNDEF_EX(glCompressedTexImage1DARB,	PFNGLCOMPRESSEDTEXIMAGE1DARBPROC);
FNDEF_EX(glCompressedTexImage2DARB,	PFNGLCOMPRESSEDTEXIMAGE2DARBPROC);
FNDEF_EX(glCompressedTexImage3DARB,	PFNGLCOMPRESSEDTEXIMAGE3DARBPROC);

// GL_ARB_vertex_program
FNDEF_EX(glVertexAttribPointerARB,		PFNGLVERTEXATTRIBPOINTERARBPROC);
FNDEF_EX(glVertexAttribIPointer,		PFNGLVERTEXATTRIBIPOINTERPROC);	// GL_NV_vertex_program4
FNDEF_EX(glEnableVertexAttribArrayARB,	PFNGLENABLEVERTEXATTRIBARRAYARBPROC);
FNDEF_EX(glDisableVertexAttribArrayARB,	PFNGLDISABLEVERTEXATTRIBARRAYARBPROC);

// GL_ARB_draw_buffers
FNDEF_EX(glDrawBuffersARB,	PFNGLDRAWBUFFERSARBPROC);

// GL_ARB_shader_objects
FNDEF_EX(glDeleteShader,		PFNGLDELETESHADERPROC);			// glDeleteObjectARB
FNDEF_EX(glGetHandleARB,		PFNGLGETHANDLEARBPROC);
FNDEF_EX(glDetachShader,		PFNGLDETACHSHADERPROC);			// glDetachObjectARB
FNDEF_EX(glCreateShader,		PFNGLCREATESHADERPROC);			// glCreateShaderObjectARB
FNDEF_EX(glShaderSource,		PFNGLSHADERSOURCEPROC);			// glShaderSourceARB
FNDEF_EX(glCompileShader,		PFNGLCOMPILESHADERPROC);		// glCompileShaderARB
FNDEF_EX(glCreateProgram,		PFNGLCREATEPROGRAMPROC);		// glCreateProgramObjectARB
FNDEF_EX(glAttachShader,		PFNGLATTACHSHADERPROC);			// glAttachObjectARB
FNDEF_EX(glLinkProgram,			PFNGLLINKPROGRAMPROC);			// glLinkProgramARB
FNDEF_EX(glUseProgram,			PFNGLUSEPROGRAMPROC);			// glUseProgramObjectARB
FNDEF_EX(glUniform1f,			PFNGLUNIFORM1FPROC);			// glUniform1fARB
FNDEF_EX(glUniform1i,			PFNGLUNIFORM1IPROC);			// glUniform1iARB
FNDEF_EX(glUniform2fv,			PFNGLUNIFORM2FVPROC);			// glUniform2fvARB
FNDEF_EX(glUniform3fv,			PFNGLUNIFORM3FVPROC);			// glUniform3fvARB
FNDEF_EX(glUniform4fv,			PFNGLUNIFORM4FVPROC);			// glUniform4fvARB
FNDEF_EX(glUniformMatrix3fv,	PFNGLUNIFORMMATRIX3FVPROC);		// glUniformMatrix3fvARB
FNDEF_EX(glUniformMatrix4fv,	PFNGLUNIFORMMATRIX4FVPROC);		// glUniformMatrix4fvARB
FNDEF_EX(glGetShaderiv,			PFNGLGETSHADERIVPROC);			// glGetObjectParameterivARB
FNDEF_EX(glGetProgramiv,		PFNGLGETPROGRAMIVPROC);			// glGetObjectParameterivARB
FNDEF_EX(glGetShaderInfoLog,	PFNGLGETSHADERINFOLOGPROC);		// glGetInfoLogARB
FNDEF_EX(glGetProgramInfoLog,	PFNGLGETPROGRAMINFOLOGPROC);	// glGetInfoLogARB
FNDEF_EX(glGetUniformLocation,	PFNGLGETUNIFORMLOCATIONPROC);	// glGetUniformLocationARB

// GL_ARB_separate_shader_objects
FNDEF_EX(glCreateShaderProgramv,		PFNGLCREATESHADERPROGRAMVPROC);
FNDEF_EX(glDeleteProgram,				PFNGLDELETEPROGRAMPROC);
FNDEF_EX(glGenProgramPipelines,			PFNGLGENPROGRAMPIPELINESPROC);
FNDEF_EX(glDeleteProgramPipelines,		PFNGLDELETEPROGRAMPIPELINESPROC);
FNDEF_EX(glBindProgramPipeline,			PFNGLBINDPROGRAMPIPELINEPROC);
FNDEF_EX(glUseProgramStages,			PFNGLUSEPROGRAMSTAGESPROC);
FNDEF_EX(glValidateProgramPipeline,		PFNGLVALIDATEPROGRAMPIPELINEPROC);
FNDEF_EX(glGetProgramPipelineiv,		PFNGLGETPROGRAMPIPELINEIVPROC);
FNDEF_EX(glGetProgramPipelineInfoLog,	PFNGLGETPROGRAMPIPELINEINFOLOGPROC);
FNDEF_EX(glActiveShaderProgram,			PFNGLACTIVESHADERPROGRAMPROC);

// GL_ARB_get_program_binary
FNDEF_EX(glProgramParameteri,	PFNGLPROGRAMPARAMETERIPROC);

// GL_ARB_uniform_buffer_object
FNDEF_EX(glGetUniformBlockIndex,	PFNGLGETUNIFORMBLOCKINDEXPROC);
FNDEF_EX(glUniformBlockBinding,		PFNGLUNIFORMBLOCKBINDINGPROC);
FNDEF_EX(glBindBufferBase,			PFNGLBINDBUFFERBASEPROC);

// GL_ARB_texture_buffer_object
FNDEF_EX(glTexBufferARB,	PFNGLTEXBUFFERARBPROC);

// GL_ARB_draw_indirect
FNDEF_EX(glDrawArraysIndirect,		PFNGLDRAWARRAYSINDIRECTPROC);
FNDEF_EX(glDrawElementsIndirect,	PFNGLDRAWELEMENTSINDIRECTPROC);

// GL_ARB_multi_draw_indirect
FNDEF_EX(glMultiDrawArraysIndirect,		PFNGLMULTIDRAWARRAYSINDIRECTPROC);
FNDEF_EX(glMultiDrawElementsIndirect,	PFNGLMULTIDRAWELEMENTSINDIRECTPROC);

// GL_ARB_vertex_shader
FNDEF_EX(glBindAttribLocation,	PFNGLBINDATTRIBLOCATIONPROC);	// glBindAttribLocationARB

// GL_ARB_tessellation_shader
FNDEF_EX(glPatchParameteri,	PFNGLPATCHPARAMETERIPROC);

// GL_ARB_geometry_shader4
FNDEF_EX(glProgramParameteriARB,	PFNGLPROGRAMPARAMETERIARBPROC);

// GL_ARB_compute_shader
FNDEF_EX(glDispatchCompute,	PFNGLDISPATCHCOMPUTEPROC);

// GL_ARB_draw_instanced
FNDEF_EX(glDrawArraysInstancedARB,		PFNGLDRAWARRAYSINSTANCEDARBPROC);
FNDEF_EX(glDrawElementsInstancedARB,	PFNGLDRAWELEMENTSINSTANCEDARBPROC);

// GL_ARB_base_instance
FNDEF_EX(glDrawArraysInstancedBaseInstance,				PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC);
FNDEF_EX(glDrawElementsInstancedBaseInstance,			PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC);
FNDEF_EX(glDrawElementsInstancedBaseVertexBaseInstance,	PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC);

// GL_ARB_instanced_arrays
FNDEF_EX(glVertexAttribDivisorARB,	PFNGLVERTEXATTRIBDIVISORARBPROC);

// GL_ARB_vertex_array_object
FNDEF_EX(glBindVertexArray,		PFNGLBINDVERTEXARRAYPROC);
FNDEF_EX(glDeleteVertexArrays,	PFNGLDELETEVERTEXARRAYSPROC);
FNDEF_EX(glGenVertexArrays,		PFNGLGENVERTEXARRAYSPROC);

// GL_ARB_sampler_objects
FNDEF_EX(glGenSamplers,			PFNGLGENSAMPLERSPROC);
FNDEF_EX(glDeleteSamplers,		PFNGLDELETESAMPLERSPROC);
FNDEF_EX(glBindSampler,			PFNGLBINDSAMPLERPROC);
FNDEF_EX(glSamplerParameteri,	PFNGLSAMPLERPARAMETERIPROC);
FNDEF_EX(glSamplerParameterf,	PFNGLSAMPLERPARAMETERFPROC);
FNDEF_EX(glSamplerParameterfv,	PFNGLSAMPLERPARAMETERFVPROC);

// GL_ARB_draw_elements_base_vertex
FNDEF_EX(glDrawElementsBaseVertex,			PFNGLDRAWELEMENTSBASEVERTEXPROC);
FNDEF_EX(glDrawElementsInstancedBaseVertex,	PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC);

// GL_ARB_debug_output
FNDEF_EX(glDebugMessageCallbackARB,	PFNGLDEBUGMESSAGECALLBACKARBPROC);
FNDEF_EX(glDebugMessageControlARB,	PFNGLDEBUGMESSAGECONTROLARBPROC);

// GL_ARB_direct_state_access
FNDEF_EX(glCreateBuffers,					PFNGLCREATEBUFFERSPROC);
FNDEF_EX(glCreateFramebuffers,				PFNGLCREATEFRAMEBUFFERSPROC);
FNDEF_EX(glCreateTextures,					PFNGLCREATETEXTURESPROC);
FNDEF_EX(glCreateVertexArrays,				PFNGLCREATEVERTEXARRAYSPROC);
FNDEF_EX(glNamedBufferData,					PFNGLNAMEDBUFFERDATAPROC);
FNDEF_EX(glNamedBufferSubData,				PFNGLNAMEDBUFFERSUBDATAPROC);
FNDEF_EX(glMapNamedBuffer,					PFNGLMAPNAMEDBUFFERPROC);
FNDEF_EX(glUnmapNamedBuffer,				PFNGLUNMAPNAMEDBUFFERPROC);
FNDEF_EX(glProgramUniform1i,				PFNGLPROGRAMUNIFORM1IPROC);
FNDEF_EX(glProgramUniform1ui,				PFNGLPROGRAMUNIFORM1UIPROC);
FNDEF_EX(glProgramUniform1f,				PFNGLPROGRAMUNIFORM1FPROC);
FNDEF_EX(glProgramUniform2fv,				PFNGLPROGRAMUNIFORM2FVPROC);
FNDEF_EX(glProgramUniform3fv,				PFNGLPROGRAMUNIFORM3FVPROC);
FNDEF_EX(glProgramUniform4fv,				PFNGLPROGRAMUNIFORM4FVPROC);
FNDEF_EX(glProgramUniformMatrix3fv,			PFNGLPROGRAMUNIFORMMATRIX3FVPROC);
FNDEF_EX(glProgramUniformMatrix4fv,			PFNGLPROGRAMUNIFORMMATRIX4FVPROC);
FNDEF_EX(glTextureParameteri,				PFNGLTEXTUREPARAMETERIPROC);
FNDEF_EX(glGenerateTextureMipmap,			PFNGLGENERATETEXTUREMIPMAPPROC);
FNDEF_EX(glEnableVertexArrayAttrib,			PFNGLENABLEVERTEXARRAYATTRIBPROC);
FNDEF_EX(glCheckNamedFramebufferStatus,		PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC);
FNDEF_EX(glNamedRenderbufferStorage,		PFNGLNAMEDRENDERBUFFERSTORAGEPROC);
FNDEF_EX(glNamedFramebufferRenderbuffer,	PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC);
FNDEF_EX(glNamedFramebufferTexture,			PFNGLNAMEDFRAMEBUFFERTEXTUREPROC);
FNDEF_EX(glNamedFramebufferTextureLayer,	PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC);
FNDEF_EX(glTextureBuffer,					PFNGLTEXTUREBUFFERPROC);
FNDEF_EX(glBindTextureUnit,					PFNGLBINDTEXTUREUNITPROC);
FNDEF_EX(glCompressedTextureSubImage1D,		PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC);
FNDEF_EX(glCompressedTextureSubImage2D,		PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC);
FNDEF_EX(glCompressedTextureSubImage3D,		PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC);
FNDEF_EX(glTextureSubImage1D,				PFNGLTEXTURESUBIMAGE1DPROC);
FNDEF_EX(glTextureSubImage2D,				PFNGLTEXTURESUBIMAGE2DPROC);
FNDEF_EX(glTextureSubImage3D,				PFNGLTEXTURESUBIMAGE3DPROC);
FNDEF_EX(glVertexArrayAttribFormat,			PFNGLVERTEXARRAYATTRIBFORMATPROC);
FNDEF_EX(glVertexArrayAttribIFormat,		PFNGLVERTEXARRAYATTRIBIFORMATPROC);
FNDEF_EX(glVertexArrayAttribBinding,		PFNGLVERTEXARRAYATTRIBBINDINGPROC);
FNDEF_EX(glVertexArrayVertexBuffer,			PFNGLVERTEXARRAYVERTEXBUFFERPROC);
FNDEF_EX(glVertexArrayBindingDivisor,		PFNGLVERTEXARRAYBINDINGDIVISORPROC);
FNDEF_EX(glVertexArrayElementBuffer,		PFNGLVERTEXARRAYELEMENTBUFFERPROC);

// GL_ARB_texture_storage
FNDEF_EX(glTextureStorage1D,			PFNGLTEXTURESTORAGE1DPROC);
FNDEF_EX(glTextureStorage2D,			PFNGLTEXTURESTORAGE2DPROC);
FNDEF_EX(glTextureStorage3D,			PFNGLTEXTURESTORAGE3DPROC);
FNDEF_EX(glTextureStorage2DMultisample,	PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC);

// GL_ARB_copy_image
FNDEF_EX(glCopyImageSubData,	PFNGLCOPYIMAGESUBDATAPROC);

// GL_ARB_gl_spirv
FNDEF_EX(glSpecializeShaderARB,	PFNGLSPECIALIZESHADERARBPROC);

// GL_ARB_clip_control
FNDEF_EX(glClipControl,	PFNGLCLIPCONTROLPROC);

// GL_ARB_occlusion_query
FNDEF_EX(glGenQueriesARB,			PFNGLGENQUERIESARBPROC);
FNDEF_EX(glDeleteQueriesARB,		PFNGLDELETEQUERIESARBPROC);
FNDEF_EX(glBeginQueryARB,			PFNGLBEGINQUERYARBPROC);
FNDEF_EX(glEndQueryARB,				PFNGLENDQUERYARBPROC);
FNDEF_EX(glGetQueryObjectuivARB,	PFNGLGETQUERYOBJECTUIVARBPROC);

// GL_ARB_timer_query
FNDEF_EX(glQueryCounter,	PFNGLQUERYCOUNTERPROC);


//[-------------------------------------------------------]
//[ Core (OpenGL version dependent)                       ]
//[-------------------------------------------------------]
FNDEF_EX(glShaderBinary,	PFNGLSHADERBINARYPROC);	// OpenGL 4.1

//[-------------------------------------------------------]
//[ Undefine helper macro                                 ]
//[-------------------------------------------------------]
#undef FNDEF_EX




//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr const char* GLSL_NAME = "GLSL";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)

		#ifdef RENDERER_OPENGL_GLSLTOSPIRV
			static bool GlslangInitialized = false;

			// Settings from "glslang/StandAlone/ResourceLimits.cpp"
			static constexpr TBuiltInResource DefaultTBuiltInResource =
			{
				32,		///< MaxLights
				6,		///< MaxClipPlanes
				32,		///< MaxTextureUnits
				32,		///< MaxTextureCoords
				64,		///< MaxVertexAttribs
				4096,	///< MaxVertexUniformComponents
				64,		///< MaxVaryingFloats
				32,		///< MaxVertexTextureImageUnits
				80,		///< MaxCombinedTextureImageUnits
				32,		///< MaxTextureImageUnits
				4096,	///< MaxFragmentUniformComponents
				32,		///< MaxDrawBuffers
				128,	///< MaxVertexUniformVectors
				8,		///< MaxVaryingVectors
				16,		///< MaxFragmentUniformVectors
				16,		///< MaxVertexOutputVectors
				15,		///< MaxFragmentInputVectors
				-8,		///< MinProgramTexelOffset
				7,		///< MaxProgramTexelOffset
				8,		///< MaxClipDistances
				65535,	///< MaxComputeWorkGroupCountX
				65535,	///< MaxComputeWorkGroupCountY
				65535,	///< MaxComputeWorkGroupCountZ
				1024,	///< MaxComputeWorkGroupSizeX
				1024,	///< MaxComputeWorkGroupSizeY
				64,		///< MaxComputeWorkGroupSizeZ
				1024,	///< MaxComputeUniformComponents
				16,		///< MaxComputeTextureImageUnits
				8,		///< MaxComputeImageUniforms
				8,		///< MaxComputeAtomicCounters
				1,		///< MaxComputeAtomicCounterBuffers
				60,		///< MaxVaryingComponents
				64,		///< MaxVertexOutputComponents
				64,		///< MaxGeometryInputComponents
				128,	///< MaxGeometryOutputComponents
				128,	///< MaxFragmentInputComponents
				8,		///< MaxImageUnits
				8,		///< MaxCombinedImageUnitsAndFragmentOutputs
				8,		///< MaxCombinedShaderOutputResources
				0,		///< MaxImageSamples
				0,		///< MaxVertexImageUniforms
				0,		///< MaxTessControlImageUniforms
				0,		///< MaxTessEvaluationImageUniforms
				0,		///< MaxGeometryImageUniforms
				8,		///< MaxFragmentImageUniforms
				8,		///< MaxCombinedImageUniforms
				16,		///< MaxGeometryTextureImageUnits
				256,	///< MaxGeometryOutputVertices
				1024,	///< MaxGeometryTotalOutputComponents
				1024,	///< MaxGeometryUniformComponents
				64,		///< MaxGeometryVaryingComponents
				128,	///< MaxTessControlInputComponents
				128,	///< MaxTessControlOutputComponents
				16,		///< MaxTessControlTextureImageUnits
				1024,	///< MaxTessControlUniformComponents
				4096,	///< MaxTessControlTotalOutputComponents
				128,	///< MaxTessEvaluationInputComponents
				128,	///< MaxTessEvaluationOutputComponents
				16,		///< MaxTessEvaluationTextureImageUnits
				1024,	///< MaxTessEvaluationUniformComponents
				120,	///< MaxTessPatchComponents
				32,		///< MaxPatchVertices
				64,		///< MaxTessGenLevel
				16,		///< MaxViewports
				0,		///< MaxVertexAtomicCounters
				0,		///< MaxTessControlAtomicCounters
				0,		///< MaxTessEvaluationAtomicCounters
				0,		///< MaxGeometryAtomicCounters
				8,		///< MaxFragmentAtomicCounters
				8,		///< MaxCombinedAtomicCounters
				1,		///< MaxAtomicCounterBindings
				0,		///< MaxVertexAtomicCounterBuffers
				0,		///< MaxTessControlAtomicCounterBuffers
				0,		///< MaxTessEvaluationAtomicCounterBuffers
				0,		///< MaxGeometryAtomicCounterBuffers
				1,		///< MaxFragmentAtomicCounterBuffers
				1,		///< MaxCombinedAtomicCounterBuffers
				16384,	///< MaxAtomicCounterBufferSize
				4,		///< MaxTransformFeedbackBuffers
				64,		///< MaxTransformFeedbackInterleavedComponents
				8,		///< MaxCullDistances
				8,		///< MaxCombinedClipAndCullDistances
				4,		///< MaxSamples
				{		///< limits
					1,	///< nonInductiveForLoops
					1,	///< whileLoops
					1,	///< doWhileLoops
					1,	///< generalUniformIndexing
					1,	///< generalAttributeMatrixVectorIndexing
					1,	///< generalVaryingIndexing
					1,	///< generalSamplerIndexing
					1,	///< generalVariableIndexing
					1,	///< generalConstantMatrixVectorIndexing
				}
			};
		#endif


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void updateWidthHeight(uint32_t mipmapIndex, uint32_t textureWidth, uint32_t textureHeight, uint32_t& width, uint32_t& height)
		{
			Renderer::ITexture::getMipmapSize(mipmapIndex, textureWidth, textureHeight);
			if (width > textureWidth)
			{
				width = textureWidth;
			}
			if (height > textureHeight)
			{
				height = textureHeight;
			}
		}

		void printOpenGLShaderInformationIntoLog(const Renderer::Context& context, GLuint openGLShader)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetShaderiv(openGLShader, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetShaderInfoLog(openGLShader, informationLength, nullptr, informationLog);

				// Output the debug string
				RENDERER_LOG(context, CRITICAL, informationLog)

				// Cleanup information memory
				RENDERER_FREE(context, informationLog);
			}
		}

		void printOpenGLShaderInformationIntoLog(const Renderer::Context& context, GLuint openGLShader, const char* sourceCode)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetShaderiv(openGLShader, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetShaderInfoLog(openGLShader, informationLength, nullptr, informationLog);

				// Output the debug string
				if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
				{
					DEBUG_BREAK;
				}

				// Cleanup information memory
				RENDERER_FREE(context, informationLog);
			}
		}

		void printOpenGLProgramInformationIntoLog(const Renderer::Context& context, GLuint openGLProgram)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetProgramiv(openGLProgram, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetProgramInfoLog(openGLProgram, informationLength, nullptr, informationLog);

				// Output the debug string
				RENDERER_LOG(context, CRITICAL, informationLog)

				// Cleanup information memory
				RENDERER_FREE(context, informationLog);
			}
		}

		void printOpenGLProgramInformationIntoLog(const Renderer::Context& context, GLuint openGLProgram, const char* sourceCode)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetProgramiv(openGLProgram, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetProgramInfoLog(openGLProgram, informationLength, nullptr, informationLog);

				// Output the debug string
				if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
				{
					DEBUG_BREAK;
				}

				// Cleanup information memory
				RENDERER_FREE(context, informationLog);
			}
		}

		/**
		*  @brief
		*    Create and load a shader from bytecode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] shaderBytecode
		*    Shader SPIR-V bytecode ("GL_ARB_gl_spirv"-extension) compressed via SMOL-V
		*
		*  @return
		*    The OpenGL shader, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderFromBytecode(const Renderer::Context& context, GLenum shaderType, const Renderer::ShaderBytecode& shaderBytecode)
		{
			// Create the shader object
			const GLuint openGLShader = glCreateShader(shaderType);

			// Load the SPIR-V module into the shader object
			// -> "glShaderBinary" is OpenGL 4.1
			{
				// Decode from SMOL-V: like Vulkan/Khronos SPIR-V, but smaller
				// -> https://github.com/aras-p/smol-v
				// -> http://aras-p.info/blog/2016/09/01/SPIR-V-Compression/
				const size_t spirvOutputBufferSize = smolv::GetDecodedBufferSize(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
				uint8_t* spirvOutputBuffer = RENDERER_MALLOC_TYPED(context, uint8_t, spirvOutputBufferSize);
				smolv::Decode(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), spirvOutputBuffer, spirvOutputBufferSize);
				glShaderBinary(1, &openGLShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, spirvOutputBuffer, static_cast<GLsizei>(spirvOutputBufferSize));
				RENDERER_FREE(context, spirvOutputBuffer);
			}

			// Done
			return openGLShader;
		}

		/**
		*  @brief
		*    Create and load a shader program from bytecode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] shaderBytecode
		*    Shader SPIR-V bytecode ("GL_ARB_gl_spirv"-extension) compressed via SMOL-V
		*
		*  @return
		*    The OpenGL shader program, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderProgramFromBytecode(const Renderer::Context& context, GLenum shaderType, const Renderer::ShaderBytecode& shaderBytecode)
		{
			// Create and load the shader object
			const GLuint openGLShader = loadShaderFromBytecode(context, shaderType, shaderBytecode);

			// Specialize the shader
			// -> Before this shader the isn't compiled, after this shader is supposed to be compiled
			glSpecializeShaderARB(openGLShader, "main", 0, nullptr, nullptr);

			// Check the compile status
			GLint compiled = GL_FALSE;
			glGetShaderiv(openGLShader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
			if (GL_TRUE == compiled)
			{
				// All went fine, create and return the program
				const GLuint openGLProgram = glCreateProgram();
				glProgramParameteri(openGLProgram, GL_PROGRAM_SEPARABLE, GL_TRUE);
				glAttachShader(openGLProgram, openGLShader);
				glLinkProgram(openGLProgram);
				glDetachShader(openGLProgram, openGLShader);
				glDeleteShader(openGLShader);

				// Check the link status
				GLint linked = GL_FALSE;
				glGetProgramiv(openGLProgram, GL_LINK_STATUS, &linked);
				if (GL_TRUE != linked)
				{
					// Error, program link failed!
					printOpenGLProgramInformationIntoLog(context, openGLProgram, nullptr);
				}

				// Done
				return openGLProgram;
			}
			else
			{
				// Error, failed to compile the shader!
				printOpenGLShaderInformationIntoLog(context, openGLShader, nullptr);

				// Destroy the OpenGL shader
				// -> A value of 0 for shader will be silently ignored
				glDeleteShader(openGLShader);

				// Error!
				return 0u;
			}
		}

		/**
		*  @brief
		*    Create, load and compile a shader program from source code
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*
		*  @return
		*    The OpenGL shader program, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderProgramFromSourceCode(const Renderer::Context& context, GLenum shaderType, const GLchar* sourceCode)
		{
			// Create the shader program
			const GLuint openGLProgram = glCreateShaderProgramv(shaderType, 1, &sourceCode);

			// Check the link status
			GLint linked = GL_FALSE;
			glGetProgramiv(openGLProgram, GL_LINK_STATUS, &linked);
			if (GL_TRUE == linked)
			{
				// All went fine, return the program
				return openGLProgram;
			}
			else
			{
				// Error, failed to compile the shader!
				printOpenGLProgramInformationIntoLog(context, openGLProgram, sourceCode);

				// Destroy the program
				// -> A value of 0 for shader will be silently ignored
				glDeleteProgram(openGLProgram);

				// Error!
				return 0u;
			}
		}

		// Basing on the implementation from https://www.opengl.org/registry/specs/ARB/separate_shader_objects.txt
		[[nodiscard]] GLuint createShaderProgramObject(const Renderer::Context& context, GLuint openGLShader, const Renderer::VertexAttributes& vertexAttributes)
		{
			if (openGLShader > 0)
			{
				// Create the OpenGL program
				const GLuint openGLProgram = glCreateProgram();
				if (openGLProgram > 0)
				{
					glProgramParameteri(openGLProgram, GL_PROGRAM_SEPARABLE, GL_TRUE);

					// Attach the shader to the program
					glAttachShader(openGLProgram, openGLShader);

					// Define the vertex array attribute binding locations ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 & 12 terminology)
					// -> Crucial code that glCreateShaderProgram doesn't do
					{
						const uint32_t numberOfVertexAttributes = vertexAttributes.numberOfAttributes;
						for (uint32_t vertexAttribute = 0; vertexAttribute < numberOfVertexAttributes; ++vertexAttribute)
						{
							glBindAttribLocation(openGLProgram, vertexAttribute, vertexAttributes.attributes[vertexAttribute].name);
						}
					}

					// Link the program
					glLinkProgram(openGLProgram);

					// Detach the shader from the program
					glDetachShader(openGLProgram, openGLShader);
				}

				// Destroy the OpenGL shader
				glDeleteShader(openGLShader);

				// Check the link status
				if (openGLProgram > 0)
				{
					GLint linked = GL_FALSE;
					glGetProgramiv(openGLProgram, GL_LINK_STATUS, &linked);
					if (GL_TRUE == linked)
					{
						// Done
						return openGLProgram;
					}
					else
					{
						// Error, program link failed!
						printOpenGLProgramInformationIntoLog(context, openGLProgram);
					}
				}
			}

			// Error!
			return 0;
		}

		[[nodiscard]] GLuint loadShaderProgramFromBytecode(const Renderer::Context& context, const Renderer::VertexAttributes& vertexAttributes, GLenum shaderType, const Renderer::ShaderBytecode& shaderBytecode)
		{
			// Create and load the shader object
			const GLuint openGLShader = loadShaderFromBytecode(context, shaderType, shaderBytecode);

			// Specialize the shader
			// -> Before this shader the isn't compiled, after this shader is supposed to be compiled
			glSpecializeShaderARB(openGLShader, "main", 0, nullptr, nullptr);

			// Check the compile status
			GLint compiled = GL_FALSE;
			glGetShaderiv(openGLShader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
			if (GL_TRUE == compiled)
			{
				// All went fine, create and return the program
				return createShaderProgramObject(context, openGLShader, vertexAttributes);
			}
			else
			{
				// Error, failed to compile the shader!
				printOpenGLShaderInformationIntoLog(context, openGLShader);

				// Destroy the OpenGL shader
				// -> A value of 0 for shader will be silently ignored
				glDeleteShader(openGLShader);

				// Error!
				return 0;
			}
		}

		/**
		*  @brief
		*    Creates, loads and compiles a shader from source code
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*
		*  @return
		*    The OpenGL shader, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderFromSourcecode(const Renderer::Context& context, GLenum shaderType, const GLchar* sourceCode)
		{
			// Create the shader object
			const GLuint openGLShader = glCreateShader(shaderType);

			// Load the shader source
			glShaderSource(openGLShader, 1, &sourceCode, nullptr);

			// Compile the shader
			glCompileShader(openGLShader);

			// Check the compile status
			GLint compiled = GL_FALSE;
			glGetShaderiv(openGLShader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
			if (GL_TRUE == compiled)
			{
				// All went fine, return the shader
				return openGLShader;
			}
			else
			{
				// Error, failed to compile the shader!

				{ // Get the length of the information
					GLint informationLength = 0;
					glGetShaderiv(openGLShader, GL_INFO_LOG_LENGTH, &informationLength);
					if (informationLength > 1)
					{
						// Allocate memory for the information
						GLchar* informationLog = RENDERER_MALLOC_TYPED(context, GLchar, informationLength);

						// Get the information
						glGetShaderInfoLog(openGLShader, informationLength, nullptr, informationLog);

						// Output the debug string
						if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
						{
							DEBUG_BREAK;
						}

						// Cleanup information memory
						RENDERER_FREE(context, informationLog);
					}
				}

				// Destroy the shader
				// -> A value of 0 for shader will be silently ignored
				glDeleteShader(openGLShader);

				// Error!
				return 0u;
			}
		}

		[[nodiscard]] GLuint loadShaderProgramFromSourcecode(const Renderer::Context& context, const Renderer::VertexAttributes& vertexAttributes, GLenum type, const char* sourceCode)
		{
			return createShaderProgramObject(context, loadShaderFromSourcecode(context, type, sourceCode), vertexAttributes);
		}

		/**
		*  @brief
		*    Compile shader source code to shader bytecode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*  @param[out] shaderBytecode
		*    Receives the shader SPIR-V bytecode ("GL_ARB_gl_spirv"-extension) compressed via SMOL-V
		*/
		// TODO(co) Visual Studio 2017 compile settings: For some reasons I need to disable optimization for the following method or else "glslang::TShader::parse()" will output the error "ERROR: 0:1: '€' : unexpected token" (glslang (latest commit c325f4364666eedb94c20a13670df058a38a14ab - April 20, 2018), Visual Studio 2017 15.7.1)
		#ifdef _MSC_VER
			#pragma optimize("", off)
		#endif
		void shaderSourceCodeToShaderBytecode(const Renderer::Context& context, GLenum shaderType, const GLchar* sourceCode, Renderer::ShaderBytecode& shaderBytecode)
		{
			#ifdef RENDERER_OPENGL_GLSLTOSPIRV
				// Initialize glslang, if necessary
				if (!GlslangInitialized)
				{
					glslang::InitializeProcess();
					GlslangInitialized = true;
				}

				// GLSL to intermediate
				// -> OpenGL 4.1 (the best OpenGL version Mac OS X 10.11 supports, so lowest version we have to support)
				// TODO(co) OpenGL GLSL 430 instead of 410 for e.g. "GL_ARB_shader_image_load_store" build in support. Apply dropped OpenGL support so we can probably drop Apple support. If at once point Unrimp should run on Apple hardware, we probably will use MoltenVK for Vulkan (yet another renderer API Metal just for Apple hardware is probably to much work for a spare time project).
				// const int glslVersion = 410;
				const int glslVersion = 430;
				EShLanguage shLanguage = EShLangCount;
				switch (shaderType)
				{
					case GL_VERTEX_SHADER_ARB:
						shLanguage = EShLangVertex;
						break;

					case GL_TESS_CONTROL_SHADER:
						shLanguage = EShLangTessControl;
						break;

					case GL_TESS_EVALUATION_SHADER:
						shLanguage = EShLangTessEvaluation;
						break;

					case GL_GEOMETRY_SHADER_ARB:
						shLanguage = EShLangGeometry;
						break;

					case GL_FRAGMENT_SHADER_ARB:
						shLanguage = EShLangFragment;
						break;

					case GL_COMPUTE_SHADER:
						shLanguage = EShLangCompute;
						break;
				}
				glslang::TShader shader(shLanguage);
				shader.setEnvInput(glslang::EShSourceGlsl, shLanguage, glslang::EShClientOpenGL, glslVersion);
				shader.setEntryPoint("main");
				{
					const char* sourcePointers[] = { sourceCode };
					shader.setStrings(sourcePointers, 1);
				}
				const EShMessages shMessages = static_cast<EShMessages>(EShMsgDefault);
				if (shader.parse(&DefaultTBuiltInResource, glslVersion, false, shMessages))
				{
					glslang::TProgram program;
					program.addShader(&shader);
					if (program.link(shMessages))
					{
						// Intermediate to SPIR-V
						const glslang::TIntermediate* intermediate = program.getIntermediate(shLanguage);
						if (nullptr != intermediate)
						{
							std::vector<unsigned int> spirv;
							glslang::GlslangToSpv(*intermediate, spirv);

							// Encode to SMOL-V: like Vulkan/Khronos SPIR-V, but smaller
							// -> https://github.com/aras-p/smol-v
							// -> http://aras-p.info/blog/2016/09/01/SPIR-V-Compression/
							// -> Don't apply "spv::spirvbin_t::remap()" or the SMOL-V result will be bigger
							smolv::ByteArray byteArray;
							smolv::Encode(spirv.data(), sizeof(unsigned int) * spirv.size(), byteArray, smolv::kEncodeFlagStripDebugInfo);

							// Done
							shaderBytecode.setBytecodeCopy(static_cast<uint32_t>(byteArray.size()), reinterpret_cast<uint8_t*>(byteArray.data()));
						}
					}
					else
					{
						// Failed to link the program
						if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to link the GLSL program: %s", program.getInfoLog()))
						{
							DEBUG_BREAK;
						}
					}
				}
				else
				{
					// Failed to parse the shader source code
					if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to parse the GLSL shader source code: %s", shader.getInfoLog()))
					{
						DEBUG_BREAK;
					}
				}
			#endif
		}
		// TODO(co) Visual Studio 2017 compile settings: For some reasons I need to disable optimization for the following method or else "glslang::TShader::parse()" will output the error "ERROR: 0:1: '€' : unexpected token" (glslang (latest commit c325f4364666eedb94c20a13670df058a38a14ab - April 20, 2018), Visual Studio 2017 15.7.1)
		#ifdef _MSC_VER
			#pragma optimize("", on)
		#endif

		void bindUniformBlock(const Renderer::DescriptorRange& descriptorRange, uint32_t openGLProgram, uint32_t uniformBlockBindingIndex)
		{
			// Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension,
			// for backward compatibility, ask for the uniform block index
			const GLuint uniformBlockIndex = glGetUniformBlockIndex(openGLProgram, descriptorRange.baseShaderRegisterName);
			if (GL_INVALID_INDEX != uniformBlockIndex)
			{
				// Associate the uniform block with the given binding point
				glUniformBlockBinding(openGLProgram, uniformBlockIndex, uniformBlockBindingIndex);
			}
		}

		void bindUniformLocation(const Renderer::DescriptorRange& descriptorRange, uint32_t openGLProgramPipeline, uint32_t openGLProgram)
		{
			const GLint uniformLocation = glGetUniformLocation(openGLProgram, descriptorRange.baseShaderRegisterName);
			if (uniformLocation >= 0)
			{
				// OpenGL/GLSL is not automatically assigning texture units to samplers, so, we have to take over this job
				// -> When using OpenGL or OpenGL ES 3 this is required
				// -> OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension supports explicit binding points ("layout(binding = 0)"
				//    in GLSL shader) , for backward compatibility we don't use it in here
				// -> When using Direct3D 9, 10, 11 or 12, the texture unit
				//    to use is usually defined directly within the shader by using the "register"-keyword
				// -> Use the "GL_ARB_direct_state_access" or "GL_EXT_direct_state_access" extension if possible to not change OpenGL states
				if (nullptr != glProgramUniform1i)
				{
					glProgramUniform1i(openGLProgram, uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
				}
				else if (nullptr != glProgramUniform1iEXT)
				{
					glProgramUniform1iEXT(openGLProgram, uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
				}
				else
				{
					// TODO(co) There's room for binding API call related optimization in here (will certainly be no huge overall efficiency gain)
					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Backup the currently used OpenGL program
						GLint openGLProgramBackup = 0;
						glGetProgramPipelineiv(openGLProgramPipeline, GL_ACTIVE_PROGRAM, &openGLProgramBackup);
						if (static_cast<uint32_t>(openGLProgramBackup) == openGLProgram)
						{
							// Set uniform, please note that for this our program must be the currently used one
							glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
						}
						else
						{
							// Set uniform, please note that for this our program must be the currently used one
							glActiveShaderProgram(openGLProgramPipeline, openGLProgram);
							glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));

							// Be polite and restore the previous used OpenGL program
							glActiveShaderProgram(openGLProgramPipeline, static_cast<GLuint>(openGLProgramBackup));
						}
					#else
						glActiveShaderProgram(openGLProgramPipeline, openGLProgram);
						glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
					#endif
				}
			}
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace OpenGLRenderer
{




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/OpenGLRenderer.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL renderer class
	*/
	class OpenGLRenderer final : public Renderer::IRenderer
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class GraphicsPipelineState;


	//[-------------------------------------------------------]
	//[ Public data                                           ]
	//[-------------------------------------------------------]
	public:
		MakeID VertexArrayMakeId;
		MakeID GraphicsPipelineStateMakeId;
		MakeID ComputePipelineStateMakeId;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*
		*  @note
		*    - Do never ever use a not properly initialized renderer! Use "Renderer::IRenderer::isInitialized()" to check the initialization state.
		*/
		explicit OpenGLRenderer(const Renderer::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~OpenGLRenderer() override;

		/**
		*  @brief
		*    Return the OpenGL context instance
		*
		*  @return
		*    The OpenGL context instance, do not free the memory the reference is pointing to
		*/
		[[nodiscard]] inline const IOpenGLContext& getOpenGLContext() const
		{
			return *mOpenGLContext;
		}

		/**
		*  @brief
		*    Return the available extensions
		*
		*  @return
		*    The available extensions, do not free the memory the reference is pointing to
		*/
		[[nodiscard]] inline const Extensions& getExtensions() const
		{
			return *mExtensions;
		}

		/**
		*  @brief
		*    Return the available extensions
		*
		*  @return
		*    The available extensions, do not free the memory the reference is pointing to
		*/
		[[nodiscard]] inline Extensions& getExtensions()
		{
			return *mExtensions;
		}

		//[-------------------------------------------------------]
		//[ Graphics                                              ]
		//[-------------------------------------------------------]
		void setGraphicsRootSignature(Renderer::IRootSignature* rootSignature);
		void setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState);
		void setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup);
		void setGraphicsVertexArray(Renderer::IVertexArray* vertexArray);															// Input-assembler (IA) stage
		void setGraphicsViewports(uint32_t numberOfViewports, const Renderer::Viewport* viewports);									// Rasterizer (RS) stage
		void setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles);	// Rasterizer (RS) stage
		void setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget);														// Output-merger (OM) stage
		void clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil);
		void drawGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		//[-------------------------------------------------------]
		//[ Compute                                               ]
		//[-------------------------------------------------------]
		void setComputeRootSignature(Renderer::IRootSignature* rootSignature);
		void setComputePipelineState(Renderer::IComputePipelineState* computePipelineState);
		void setComputeResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup);
		void dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		//[-------------------------------------------------------]
		//[ Resource                                              ]
		//[-------------------------------------------------------]
		void resolveMultisampleFramebuffer(Renderer::IRenderTarget& destinationRenderTarget, Renderer::IFramebuffer& sourceMultisampleFramebuffer);
		void copyResource(Renderer::IResource& destinationResource, Renderer::IResource& sourceResource);
		void generateMipmaps(Renderer::IResource& resource);
		//[-------------------------------------------------------]
		//[ Query                                                 ]
		//[-------------------------------------------------------]
		void resetQueryPool(Renderer::IQueryPool& queryPool, uint32_t firstQueryIndex, uint32_t numberOfQueries);
		void beginQuery(Renderer::IQueryPool& queryPool, uint32_t queryIndex, uint32_t queryControlFlags);
		void endQuery(Renderer::IQueryPool& queryPool, uint32_t queryIndex);
		void writeTimestampQuery(Renderer::IQueryPool& queryPool, uint32_t queryIndex);
		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RENDERER_DEBUG
			void setDebugMarker(const char* name);
			void beginDebugEvent(const char* name);
			void endDebugEvent();
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual const char* getName() const override;
		[[nodiscard]] virtual bool isInitialized() const override;
		[[nodiscard]] virtual bool isDebugEnabled() override;
		//[-------------------------------------------------------]
		//[ Shader language                                       ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual uint32_t getNumberOfShaderLanguages() const override;
		[[nodiscard]] virtual const char* getShaderLanguageName(uint32_t index) const override;
		[[nodiscard]] virtual Renderer::IShaderLanguage* getShaderLanguage(const char* shaderLanguageName = nullptr) override;
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual Renderer::IRenderPass* createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat = Renderer::TextureFormat::UNKNOWN, uint8_t numberOfMultisamples = 1) override;
		[[nodiscard]] virtual Renderer::IQueryPool* createQueryPool(Renderer::QueryType queryType, uint32_t numberOfQueries = 1) override;
		[[nodiscard]] virtual Renderer::ISwapChain* createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool useExternalContext = false) override;
		[[nodiscard]] virtual Renderer::IFramebuffer* createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment = nullptr) override;
		[[nodiscard]] virtual Renderer::IBufferManager* createBufferManager() override;
		[[nodiscard]] virtual Renderer::ITextureManager* createTextureManager() override;
		[[nodiscard]] virtual Renderer::IRootSignature* createRootSignature(const Renderer::RootSignature& rootSignature) override;
		[[nodiscard]] virtual Renderer::IGraphicsPipelineState* createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState) override;
		[[nodiscard]] virtual Renderer::IComputePipelineState* createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader) override;
		[[nodiscard]] virtual Renderer::ISamplerState* createSamplerState(const Renderer::SamplerState& samplerState) override;
		//[-------------------------------------------------------]
		//[ Resource handling                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual bool map(Renderer::IResource& resource, uint32_t subresource, Renderer::MapType mapType, uint32_t mapFlags, Renderer::MappedSubresource& mappedSubresource) override;
		virtual void unmap(Renderer::IResource& resource, uint32_t subresource) override;
		[[nodiscard]] virtual bool getQueryPoolResults(Renderer::IQueryPool& queryPool, uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex = 0, uint32_t numberOfQueries = 1, uint32_t strideInBytes = 0, uint32_t queryResultFlags = Renderer::QueryResultFlags::WAIT) override;
		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual bool beginScene() override;
		virtual void submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer) override;
		virtual void endScene() override;
		//[-------------------------------------------------------]
		//[ Synchronization                                       ]
		//[-------------------------------------------------------]
		virtual void flush() override;
		virtual void finish() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(mContext, OpenGLRenderer, this);
		}


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Debug message callback function called by the "GL_ARB_debug_output"-extension
		*
		*  @param[in] source
		*    Source of the debug message
		*  @param[in] type
		*    Type of the debug message
		*  @param[in] id
		*    ID of the debug message
		*  @param[in] severity
		*    Severity of the debug message
		*  @param[in] length
		*    Length of the debug message
		*  @param[in] message
		*    The debug message
		*  @param[in] userParam
		*    Additional user parameter of the debug message
		*/
		static void CALLBACK debugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int length, const char* message, const void* userParam);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit OpenGLRenderer(const OpenGLRenderer& source) = delete;
		OpenGLRenderer& operator =(const OpenGLRenderer& source) = delete;

		/**
		*  @brief
		*    Initialize the capabilities
		*/
		void initializeCapabilities();

		/**
		*  @brief
		*    Unset the currently used vertex array
		*/
		void unsetGraphicsVertexArray();

		/**
		*  @brief
		*    Set resource group, method used by graphics and compute
		*
		*  @param[in] rootSignature
		*    Used root signature
		*  @param[in] rootParameterIndex
		*    Root parameter index
		*  @param[in] resourceGroup
		*    Resource group to set, can be a null pointer
		*/
		void setResourceGroup(const RootSignature& rootSignature, uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup);

		/**
		*  @brief
		*    Set OpenGL graphics program
		*
		*  @param[in] graphicsProgram
		*    Graphics program to set
		*/
		void setOpenGLGraphicsProgram(Renderer::IGraphicsProgram* graphicsProgram);

		/**
		*  @brief
		*    Set OpenGL compute pipeline state
		*
		*  @param[in] computePipelineState
		*    Compute pipeline state to set
		*/
		void setOpenGLComputePipelineState(ComputePipelineState* computePipelineState);

		/**
		*  @brief
		*    Update "GL_ARB_base_instance" emulation
		*
		*  @param[in] startInstanceLocation
		*    Start instance location
		*/
		void updateGL_ARB_base_instanceEmulation(uint32_t startInstanceLocation);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLRuntimeLinking*	   mOpenGLRuntimeLinking;			///< OpenGL runtime linking instance, always valid
		IOpenGLContext*			   mOpenGLContext;					///< OpenGL context instance, always valid
		Extensions*				   mExtensions;						///< Extensions instance, always valid
		Renderer::IShaderLanguage* mShaderLanguage;					///< Shader language instance (we keep a reference to it), can be a null pointer
		RootSignature*			   mGraphicsRootSignature;			///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*			   mComputeRootSignature;			///< Currently set compute root signature (we keep a reference to it), can be a null pointer
		Renderer::ISamplerState*   mDefaultSamplerState;			///< Default rasterizer state (we keep a reference to it), can be a null pointer
		GLuint					   mOpenGLCopyResourceFramebuffer;	///< OpenGL framebuffer ("container" object, not shared between OpenGL contexts) used by "OpenGLRenderer::OpenGLRenderer::copyResource()" if the "GL_ARB_copy_image"-extension isn't available, can be zero if no resource is allocated
		GLuint					   mDefaultOpenGLVertexArray;		///< Default OpenGL vertex array ("container" object, not shared between OpenGL contexts) to enable attribute-less rendering, can be zero if no resource is allocated
		// States
		GraphicsPipelineState* mGraphicsPipelineState;	///< Currently set graphics pipeline state (we keep a reference to it), can be a null pointer
		ComputePipelineState*  mComputePipelineState;	///< Currently set compute pipeline state (we keep a reference to it), can be a null pointer
		// Input-assembler (IA) stage
		VertexArray* mVertexArray;				///< Currently set vertex array (we keep a reference to it), can be a null pointer
		GLenum		 mOpenGLPrimitiveTopology;	///< OpenGL primitive topology describing the type of primitive to render
		GLint		 mNumberOfVerticesPerPatch;	///< Number of vertices per patch
		// Output-merger (OM) stage
		Renderer::IRenderTarget* mRenderTarget;	///< Currently set render target (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant OpenGL calls
		GLenum mOpenGLClipControlOrigin;	///< Currently set OpenGL clip control origin
		GLuint mOpenGLProgramPipeline;		///< Currently set OpenGL program pipeline, can be zero if no resource is set
		GLuint mOpenGLProgram;				///< Currently set OpenGL program, can be zero if no resource is set
		GLuint mOpenGLIndirectBuffer;		///< Currently set OpenGL indirect buffer, can be zero if no resource is set
		// Draw ID uniform location for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
		GLuint	 mOpenGLVertexProgram;			///< Currently set OpenGL vertex program, can be zero if no resource is set
		GLint	 mDrawIdUniformLocation;		///< Draw ID uniform location
		uint32_t mCurrentStartInstanceLocation;	///< Currently set start instance location


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/OpenGLRuntimeLinking.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL runtime linking
	*/
	class OpenGLRuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class IOpenGLContext;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit OpenGLRuntimeLinking(OpenGLRenderer& openGLRenderer) :
			mOpenGLRenderer(openGLRenderer),
			mOpenGLSharedLibrary(mOpenGLRenderer.getContext().getRendererApiSharedLibrary()),
			mOwnsOpenGLSharedLibrary(nullptr == mOpenGLSharedLibrary),	// We can do this here because "mOpenGLSharedLibrary" lays before this variable
			mEntryPointsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~OpenGLRuntimeLinking()
		{
			if (mOwnsOpenGLSharedLibrary)
			{
				// Destroy the shared library instances
				#ifdef _WIN32
					if (nullptr != mOpenGLSharedLibrary)
					{
						::FreeLibrary(static_cast<HMODULE>(mOpenGLSharedLibrary));
					}
				#elif LINUX
					if (nullptr != mOpenGLSharedLibrary)
					{
						::dlclose(mOpenGLSharedLibrary);
					}
				#else
					#error "Unsupported platform"
				#endif
			}
		}

		/**
		*  @brief
		*    Return whether or not OpenGL is available
		*
		*  @return
		*    "true" if OpenGL is available, else "false"
		*/
		[[nodiscard]] bool isOpenGLAvaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the OpenGL entry points
					mEntryPointsRegistered = loadOpenGLEntryPoints();
				}
			}

			// Entry points successfully registered?
			return mEntryPointsRegistered;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit OpenGLRuntimeLinking(const OpenGLRuntimeLinking& source) = delete;
		OpenGLRuntimeLinking& operator =(const OpenGLRuntimeLinking& source) = delete;

		/**
		*  @brief
		*    Load the shared libraries
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadSharedLibraries()
		{
			if (mOwnsOpenGLSharedLibrary)
			{
				// Load the shared library
				#ifdef _WIN32
					mOpenGLSharedLibrary = ::LoadLibraryExA("opengl32.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr == mOpenGLSharedLibrary)
					{
						RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to load in the shared OpenGL library \"opengl32.dll\"")
					}
				#elif LINUX
					mOpenGLSharedLibrary = ::dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
					if (nullptr == mOpenGLSharedLibrary)
					{
						RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to load in the shared OpenGL library \"libGL.so\"")
					}
				#else
					#error "Unsupported platform"
				#endif
			}

			// Done
			return (nullptr != mOpenGLSharedLibrary);
		}

		/**
		*  @brief
		*    Load the OpenGL entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadOpenGLEntryPoints()
		{
			bool result = true;	// Success by default

			// Load the entry points
			IMPORT_FUNC(glGetString);
			IMPORT_FUNC(glGetIntegerv);
			IMPORT_FUNC(glBindTexture);
			IMPORT_FUNC(glClear);
			IMPORT_FUNC(glClearStencil);
			IMPORT_FUNC(glClearDepth);
			IMPORT_FUNC(glClearColor);
			IMPORT_FUNC(glDrawArrays);
			IMPORT_FUNC(glDrawElements);
			IMPORT_FUNC(glColor4f);
			IMPORT_FUNC(glEnable);
			IMPORT_FUNC(glDisable);
			IMPORT_FUNC(glBlendFunc);
			IMPORT_FUNC(glFrontFace);
			IMPORT_FUNC(glCullFace);
			IMPORT_FUNC(glPolygonMode);
			IMPORT_FUNC(glTexParameteri);
			IMPORT_FUNC(glGenTextures);
			IMPORT_FUNC(glDeleteTextures);
			IMPORT_FUNC(glTexImage1D);
			IMPORT_FUNC(glTexImage2D);
			IMPORT_FUNC(glPixelStorei);
			IMPORT_FUNC(glDepthFunc);
			IMPORT_FUNC(glDepthMask);
			IMPORT_FUNC(glViewport);
			IMPORT_FUNC(glDepthRange);
			IMPORT_FUNC(glScissor);
			IMPORT_FUNC(glFlush);
			IMPORT_FUNC(glFinish);
			#ifdef _WIN32
				IMPORT_FUNC(wglGetCurrentDC);
				IMPORT_FUNC(wglGetProcAddress);
				IMPORT_FUNC(wglCreateContext);
				IMPORT_FUNC(wglDeleteContext);
				IMPORT_FUNC(wglMakeCurrent);
			#elif LINUX
				IMPORT_FUNC(glXMakeCurrent);
				IMPORT_FUNC(glXGetProcAddress);
				IMPORT_FUNC(glXGetProcAddressARB);
				IMPORT_FUNC(glXChooseVisual);
				IMPORT_FUNC(glXCreateContext);
				IMPORT_FUNC(glXDestroyContext);
				IMPORT_FUNC(glXGetCurrentContext);
				IMPORT_FUNC(glXQueryExtensionsString);
				IMPORT_FUNC(glXChooseFBConfig);
				IMPORT_FUNC(glXSwapBuffers);
				IMPORT_FUNC(glXGetClientString);
			#else
				#error "Unsupported platform"
			#endif

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the >= OpenGL 3.0 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @note
		*    - This method is only allowed to be called after an >= OpenGL context has been created and set
		*/
		[[nodiscard]] bool loadOpenGL3EntryPoints()
		{
			bool result = true;	// Success by default

			// Optional >= OpenGL 4.5: Load the entry points
			IMPORT_FUNC(glCreateQueries);

			// Mandatory >= OpenGL 3.0: Load the entry points
			result = true;	// Success by default
			IMPORT_FUNC(glGetStringi);

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLRenderer&	mOpenGLRenderer;			///< Owner OpenGL renderer instance
		void*			mOpenGLSharedLibrary;		///< OpenGL shared library, can be a null pointer
		bool			mOwnsOpenGLSharedLibrary;	///< Indicates if the OpenGL shared library was loaded from ourself or provided from external
		bool			mEntryPointsRegistered;		///< Entry points successfully registered?
		bool			mInitialized;				///< Already initialized?


	};

	// Undefine the helper macro
	#undef IMPORT_FUNC




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Extensions.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Supported OpenGL graphic card extensions
	*
	*  @remarks
	*    You must check if the extension is supported by the current hardware before
	*    you use it. If the extension isn't available you should offer an alternative
	*    technique aka fallback.
	*
	*  @see
	*    - OpenGL extension registry at http://oss.sgi.com/projects/ogl-sample/registry/ for more information about
	*      the different extensions
	*/
	class Extensions final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] openGLContext
		*    Owner OpenGL context
		*/
		inline Extensions(OpenGLRenderer& openGLRenderer, IOpenGLContext& openGLContext) :
			mOpenGLRenderer(openGLRenderer),
			mOpenGLContext(&openGLContext),
			mInitialized(false)
		{
			// Reset extensions
			resetExtensions();
		}

		/**
		*  @brief
		*    Destructor
		*
		*  @note
		*    - Not virtual by intent
		*/
		inline ~Extensions()
		{}

		/**
		*  @brief
		*    Returns whether the extensions are initialized or not
		*
		*  @return
		*    "true" if the extension are initialized, else "false"
		*/
		[[nodiscard]] inline bool isInitialized() const
		{
			return mInitialized;
		}

		///////////////////////////////////////////////////////////
		// Returns whether an extension is supported or not
		///////////////////////////////////////////////////////////
		// WGL (Windows only)
		[[nodiscard]] inline bool isWGL_ARB_extensions_string() const
		{
			return mWGL_ARB_extensions_string;
		}

		[[nodiscard]] inline bool isWGL_EXT_swap_control() const
		{
			return mWGL_EXT_swap_control;
		}

		[[nodiscard]] inline bool isWGL_EXT_swap_control_tear() const
		{
			return mWGL_EXT_swap_control_tear;
		}

		// EXT
		[[nodiscard]] inline bool isGL_EXT_texture_lod_bias() const
		{
			return mGL_EXT_texture_lod_bias;
		}

		[[nodiscard]] inline bool isGL_EXT_texture_filter_anisotropic() const
		{
			return mGL_EXT_texture_filter_anisotropic;
		}

		[[nodiscard]] inline bool isGL_EXT_texture_array() const
		{
			return mGL_EXT_texture_array;
		}

		[[nodiscard]] inline bool isGL_EXT_texture3D() const
		{
			return mGL_EXT_texture3D;
		}

		[[nodiscard]] inline bool isGL_EXT_direct_state_access() const
		{
			return mGL_EXT_direct_state_access;
		}

		[[nodiscard]] inline bool isGL_EXT_shader_image_load_store() const
		{
			return mGL_EXT_shader_image_load_store;
		}

		// KHR
		[[nodiscard]] inline bool isGL_KHR_debug() const
		{
			return mGL_KHR_debug;
		}

		// ARB
		[[nodiscard]] inline bool isGL_ARB_framebuffer_object() const
		{
			return mGL_ARB_framebuffer_object;
		}

		[[nodiscard]] inline bool isGL_ARB_multitexture() const
		{
			return mGL_ARB_multitexture;
		}

		[[nodiscard]] inline bool isGL_ARB_texture_multisample() const
		{
			return mGL_ARB_texture_multisample;
		}

		[[nodiscard]] inline bool isGL_ARB_vertex_buffer_object() const
		{
			return mGL_ARB_vertex_buffer_object;
		}

		[[nodiscard]] inline bool isGL_ARB_texture_compression() const
		{
			return mGL_ARB_texture_compression;
		}

		[[nodiscard]] inline bool isGL_ARB_draw_buffers() const
		{
			return mGL_ARB_draw_buffers;
		}

		[[nodiscard]] inline bool isGL_ARB_shader_objects() const
		{
			return mGL_ARB_shader_objects;
		}

		[[nodiscard]] inline bool isGL_ARB_separate_shader_objects() const
		{
			return mGL_ARB_separate_shader_objects;
		}

		[[nodiscard]] inline bool isGL_ARB_get_program_binary() const
		{
			return mGL_ARB_get_program_binary;
		}

		[[nodiscard]] inline bool isGL_ARB_uniform_buffer_object() const
		{
			return mGL_ARB_uniform_buffer_object;
		}

		[[nodiscard]] inline bool isGL_ARB_texture_buffer_object() const
		{
			return mGL_ARB_texture_buffer_object;
		}

		[[nodiscard]] inline bool isGL_ARB_draw_indirect() const
		{
			return mGL_ARB_draw_indirect;
		}

		[[nodiscard]] inline bool isGL_ARB_multi_draw_indirect() const
		{
			return mGL_ARB_multi_draw_indirect;
		}

		[[nodiscard]] inline bool isGL_ARB_vertex_shader() const
		{
			return mGL_ARB_vertex_shader;
		}

		[[nodiscard]] inline bool isGL_ARB_vertex_program() const
		{
			return mGL_ARB_vertex_program;
		}

		[[nodiscard]] inline bool isGL_ARB_tessellation_shader() const
		{
			return mGL_ARB_tessellation_shader;
		}

		[[nodiscard]] inline bool isGL_ARB_geometry_shader4() const
		{
			return mGL_ARB_geometry_shader4;
		}

		[[nodiscard]] inline bool isGL_ARB_fragment_shader() const
		{
			return mGL_ARB_fragment_shader;
		}

		[[nodiscard]] inline bool isGL_ARB_fragment_program() const
		{
			return mGL_ARB_fragment_program;
		}

		[[nodiscard]] inline bool isGL_ARB_compute_shader() const
		{
			return mGL_ARB_compute_shader;
		}

		[[nodiscard]] inline bool isGL_ARB_draw_instanced() const
		{
			return mGL_ARB_draw_instanced;
		}

		[[nodiscard]] inline bool isGL_ARB_base_instance() const
		{
			return mGL_ARB_base_instance;
		}

		[[nodiscard]] inline bool isGL_ARB_instanced_arrays() const
		{
			return mGL_ARB_instanced_arrays;
		}

		[[nodiscard]] inline bool isGL_ARB_vertex_array_object() const
		{
			return mGL_ARB_vertex_array_object;
		}

		[[nodiscard]] inline bool isGL_ARB_sampler_objects() const
		{
			return mGL_ARB_sampler_objects;
		}

		[[nodiscard]] inline bool isGL_ARB_draw_elements_base_vertex() const
		{
			return mGL_ARB_draw_elements_base_vertex;
		}

		[[nodiscard]] inline bool isGL_ARB_debug_output() const
		{
			return mGL_ARB_debug_output;
		}

		[[nodiscard]] inline bool isGL_ARB_direct_state_access() const
		{
			return mGL_ARB_direct_state_access;
		}

		[[nodiscard]] inline bool isGL_ARB_texture_storage() const
		{
			return mGL_ARB_texture_storage;
		}

		[[nodiscard]] inline bool isGL_ARB_shader_storage_buffer_object() const
		{
			return mGL_ARB_shader_storage_buffer_object;
		}

		[[nodiscard]] inline bool isGL_ARB_copy_image() const
		{
			return mGL_ARB_copy_image;
		}

		[[nodiscard]] inline bool isGL_ARB_gl_spirv() const
		{
			return mGL_ARB_gl_spirv;
		}

		[[nodiscard]] inline bool isGL_ARB_clip_control() const
		{
			return mGL_ARB_clip_control;
		}

		[[nodiscard]] inline bool isGL_ARB_occlusion_query() const
		{
			return mGL_ARB_occlusion_query;
		}

		[[nodiscard]] inline bool isGL_ARB_pipeline_statistics_query() const
		{
			return mGL_ARB_pipeline_statistics_query;
		}

		[[nodiscard]] inline bool isGL_ARB_timer_query() const
		{
			return mGL_ARB_timer_query;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	public:
		explicit Extensions(const Extensions& source) = delete;
		Extensions& operator =(const Extensions& source) = delete;

		/**
		*  @brief
		*    Checks whether an extension is supported by the given hardware or not
		*
		*  @param[in] extension
		*    ASCII name of the extension, if a null pointer, nothing happens
		*
		*  @return
		*    "true" if the extensions is supported, else "false"
		*/
		[[nodiscard]] bool isSupported(const char* extension) const
		{
			// Check whether or not the given extension string pointer is valid
			if (nullptr != extension)
			{
				// Is the extension supported by the hardware?
				if (checkExtension(extension))
				{
					// Extension is supported!
					return true;
				}
			}

			// Extension isn't supported!
			return false;
		}

		/**
		*  @brief
		*    Checks whether an extension is supported by the given hardware or not
		*
		*  @param[in] extension
		*    ASCII name of the extension, if a null pointer, nothing happens
		*
		*  @return
		*    "true" if the extensions is supported, else "false"
		*/
		[[nodiscard]] bool checkExtension(const char* extension) const
		{
			// Check whether or not the given extension string pointer is valid
			if (nullptr != extension)
			{
				// Under Windows all available extensions can be received via one additional function
				// but under Linux there are two additional functions for this
				#ifdef _WIN32
					// "glGetString()" & "wglGetExtensionsStringARB()"
					const int numberOfLoops = 2;
				#elif APPLE
					// On Mac OS X, only "glGetString(GL_EXTENSIONS)" is required
					const int numberOfLoops = 1;
				#elif LINUX
					// "glGetString()" & "glXQueryExtensionsString()" & "glXGetClientString()"
					const int numberOfLoops = 3;
				#else
					#error "Unsupported platform"
				#endif
				const char* extensions = nullptr;
				for (int loopIndex = 0; loopIndex < numberOfLoops; ++loopIndex)
				{
					// Extension names should not have spaces
					const char* where = strchr(extension, ' ');
					if (nullptr != where || '\0' == *extension)
					{
						return false; // Extension not found
					}

					// Advanced extensions
					// TODO(sw) Move the query for advanced extensions (via platform specific methods) to the context?
					if (loopIndex > 0)
					{
						#ifdef _WIN32
							// WGL extensions
							if (!mWGL_ARB_extensions_string)
							{
								// Extension not found
								return false;
							}
							extensions = static_cast<const char*>(wglGetExtensionsStringARB(wglGetCurrentDC()));
						#elif APPLE
							// On Mac OS X, only "glGetString(GL_EXTENSIONS)" is required
						#elif LINUX
							// Get the X server display connection
							Display* display = static_cast<OpenGLContextLinux&>(*mOpenGLContext).getDisplay();
							if (nullptr != display)
							{
								if (2 == loopIndex)
								{
									extensions = static_cast<const char*>(glXQueryExtensionsString(display, XDefaultScreen(display)));
								}
								else
								{
									extensions = static_cast<const char*>(glXGetClientString(display, GLX_EXTENSIONS));
								}
							}
						#else
							#error "Unsupported platform"
						#endif

					// Normal extensions
					}
					else
					{
						extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
						if (nullptr == extensions)
						{
							// "glGetString(GL_EXTENSIONS)" is not available in core profiles, we have to use "glGetStringi()" 
							int numberOfExtensions = 0;
							glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExtensions);
							for (GLuint extensionIndex = 0; extensionIndex < static_cast<GLuint>(numberOfExtensions); ++extensionIndex)
							{
								if (0 == strcmp(extension, reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, extensionIndex))))
								{
									// Extension found
									return true;
								}
							}
						}
					}
					if (nullptr != extensions)
					{
						// It takes a bit of care to be fool-proof about parsing the
						// OpenGL extensions string. Don't be fooled by substrings,
						// etc:
						const char* start = extensions;
						where = strstr(start, extension);
						while (nullptr != where)
						{
							const char* terminator = where + strlen(extension);
							if ((where == start || ' ' == *(where - 1)) && (' ' == *terminator || '\0' == *terminator))
							{
								// Extension found
								return true;
							}
							start = terminator;
							where = strstr(start, extension);
						}
					}
				}
			}

			// Extension not found
			return false;
		}

		/**
		*  @brief
		*    Resets the extensions
		*/
		void resetExtensions()
		{
			mInitialized = false;

			// Extensions
			// WGL (Windows only)
			mWGL_ARB_extensions_string			 = false;
			mWGL_EXT_swap_control				 = false;
			mWGL_EXT_swap_control_tear			 = false;
			// EXT
			mGL_EXT_texture_lod_bias			 = false;
			mGL_EXT_texture_filter_anisotropic	 = false;
			mGL_EXT_texture_array				 = false;
			mGL_EXT_texture3D					 = false;
			mGL_EXT_direct_state_access			 = false;
			mGL_EXT_shader_image_load_store		 = false;
			// KHR
			mGL_KHR_debug						 = false;
			// ARB
			mGL_ARB_framebuffer_object			 = false;
			mGL_ARB_multitexture				 = false;
			mGL_ARB_texture_multisample			 = false;
			mGL_ARB_vertex_buffer_object		 = false;
			mGL_ARB_texture_compression			 = false;
			mGL_ARB_draw_buffers				 = false;
			mGL_ARB_shader_objects				 = false;
			mGL_ARB_separate_shader_objects		 = false;
			mGL_ARB_get_program_binary			 = false;
			mGL_ARB_uniform_buffer_object		 = false;
			mGL_ARB_texture_buffer_object		 = false;
			mGL_ARB_draw_indirect				 = false;
			mGL_ARB_multi_draw_indirect			 = false;
			mGL_ARB_vertex_shader				 = false;
			mGL_ARB_vertex_program				 = false;
			mGL_ARB_tessellation_shader			 = false;
			mGL_ARB_geometry_shader4			 = false;
			mGL_ARB_fragment_shader				 = false;
			mGL_ARB_fragment_program			 = false;
			mGL_ARB_compute_shader				 = false;
			mGL_ARB_draw_instanced				 = false;
			mGL_ARB_base_instance				 = false;
			mGL_ARB_instanced_arrays			 = false;
			mGL_ARB_vertex_array_object			 = false;
			mGL_ARB_sampler_objects				 = false;
			mGL_ARB_draw_elements_base_vertex	 = false;
			mGL_ARB_debug_output				 = false;
			mGL_ARB_direct_state_access			 = false;
			mGL_ARB_texture_storage				 = false;
			mGL_ARB_shader_storage_buffer_object = false;
			mGL_ARB_copy_image					 = false;
			mGL_ARB_gl_spirv					 = false;
			mGL_ARB_clip_control				 = false;
			mGL_ARB_occlusion_query				 = false;
			mGL_ARB_pipeline_statistics_query	 = false;
			mGL_ARB_timer_query					 = false;
		}

		/**
		*  @brief
		*    Initialize the supported extensions
		*
		*  @param[in] useExtensions
		*    Use extensions?
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @note
		*    - Platform dependent implementation
		*/
		#ifdef _WIN32
			[[nodiscard]] bool initialize(bool useExtensions = true)
			{
				// Disable the following warning, we can't do anything to resolve this warning
				PRAGMA_WARNING_PUSH
				PRAGMA_WARNING_DISABLE_MSVC(4191)	// warning C4191: 'reinterpret_cast' : unsafe conversion from 'PROC' to '<x>'

				// Should the extensions be used?
				if (useExtensions)
				{
					mInitialized = true;
				}
				else
				{
					resetExtensions();
					mInitialized = true;

					// Done
					return true;
				}


				//[-------------------------------------------------------]
				//[ WGL (Windows only) definitions                        ]
				//[-------------------------------------------------------]
				// WGL_ARB_extensions_string
				wglGetExtensionsStringARB = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(wglGetProcAddress("wglGetExtensionsStringARB"));
				mWGL_ARB_extensions_string = (nullptr != wglGetExtensionsStringARB);

				// WGL_EXT_swap_control
				mWGL_EXT_swap_control = isSupported("WGL_EXT_swap_control");
				if (mWGL_EXT_swap_control)
				{
					wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));
					mWGL_EXT_swap_control = (nullptr != wglGetExtensionsStringARB);
				}

				// WGL_EXT_swap_control_tear
				mWGL_EXT_swap_control_tear = isSupported("WGL_EXT_swap_control_tear");


				// Restore the previous warning configuration
				PRAGMA_WARNING_POP

				// Initialize the supported universal extensions
				return initializeUniversal();
			}
		#elif LINUX
			[[nodiscard]] bool initialize(bool useExtensions)
			{
				// Disable the following warning, we can't do anything to resolve this warning
				// Should the extensions be used?
				if (useExtensions)
				{
					mInitialized = true;
				}
				else
				{
					resetExtensions();
					mInitialized = true;

					// Done
					return true;
				}

				// Initialize the supported universal extensions
				return initializeUniversal();
			}
		#else
			#error "Unsupported platform"
		#endif

		/**
		*  @brief
		*    Initialize the supported universal extensions
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @note
		*    - Platform independent implementation
		*/
		[[nodiscard]] bool initializeUniversal()
		{
			// Define a platform dependent helper macro
			#ifdef _WIN32
				#define IMPORT_FUNC(funcName)																															\
					if (result)																																			\
					{																																					\
						void* symbol = wglGetProcAddress(#funcName);																									\
						if (nullptr != symbol)																															\
						{																																				\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																							\
						}																																				\
						else																																			\
						{																																				\
							RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library", #funcName)	\
							result = false;																																\
						}																																				\
					}
			#elif APPLE
				// For OpenGL extension handling, Apple provides several documents like
				// - "Technical Note TN2080 Understanding and Detecting OpenGL Functionality" (http://developer.apple.com/library/mac/#technotes/tn2080/_index.html)
				// - "Cross-Development Programming Guide" (http://www.filibeto.org/unix/macos/lib/dev/documentation/DeveloperTools/Conceptual/cross_development/cross_development.pdf)
				// -> All referencing to "QA1188: GetProcAdress and OpenGL Entry Points" (http://developer.apple.com/qa/qa2001/qa1188.html).
				//    Sadly, it appears that this site no longer exists.
				// -> It appears that for Mac OS X v10.6 >, the "dlopen"-way is recommended.
				#define IMPORT_FUNC(funcName)																															\
					if (result)																																			\
					{																																					\
						void* symbol = m_pOpenGLSharedLibrary ? dlsym(mOpenGLSharedLibrary, #funcName) : nullptr;														\
						if (nullptr != symbol)																															\
						{																																				\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																							\
						}																																				\
						else																																			\
						{																																				\
							RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library", #funcName)	\
							result = false;																																\
						}																																				\
					}
			#elif LINUX
				typedef void (*GLfunction)();
				#define IMPORT_FUNC(funcName)																															\
					if (result)																																			\
					{																																					\
						GLfunction symbol = glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(#funcName));															\
						if (nullptr != symbol)																															\
						{																																				\
							*(reinterpret_cast<GLfunction*>(&(funcName))) = symbol;																						\
						}																																				\
						else																																			\
						{																																				\
							RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library", #funcName)	\
							result = false;																																\
						}																																				\
					}
			#else
				#error "Unsupported platform"
			#endif

			// One thing about OpenGL versions and extensions: In case we're using a certain OpenGL core profile, graphics driver implementations might
			// decide to not list OpenGL extensions which are a part of this OpenGL core profile. Such a behavior was first noted using Linux Mesa 3D.
			// When not taking this into account, horrible things will happen.
			GLint profile = 0;
			glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
			const bool isCoreProfile = (profile & GL_CONTEXT_CORE_PROFILE_BIT);


			//[-------------------------------------------------------]
			//[ EXT                                                   ]
			//[-------------------------------------------------------]
			// GL_EXT_texture_lod_bias - Is core since OpenGL 1.4
			mGL_EXT_texture_lod_bias = isCoreProfile ? true : isSupported("GL_EXT_texture_lod_bias");

			// GL_EXT_texture_filter_anisotropic
			mGL_EXT_texture_filter_anisotropic = isSupported("GL_EXT_texture_filter_anisotropic");

			// GL_EXT_texture_array - Is core since OpenGL 3.0
			mGL_EXT_texture_array = isCoreProfile ? true : isSupported("GL_EXT_texture_array");

			// GL_EXT_texture3D - Is core since OpenGL 1.2
			mGL_EXT_texture3D = isCoreProfile ? true : isSupported("GL_EXT_texture3D");
			if (mGL_EXT_texture3D)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glTexImage3DEXT)
				IMPORT_FUNC(glTexSubImage3DEXT)
				mGL_EXT_texture3D = result;
			}

			// GL_EXT_direct_state_access - Is core since OpenGL 2.1
			mGL_EXT_direct_state_access = isCoreProfile ? true : isSupported("GL_EXT_direct_state_access");
			if (mGL_EXT_direct_state_access)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glNamedBufferDataEXT)
				IMPORT_FUNC(glNamedBufferSubDataEXT)
				IMPORT_FUNC(glMapNamedBufferEXT)
				IMPORT_FUNC(glUnmapNamedBufferEXT)
				IMPORT_FUNC(glProgramUniform1iEXT)
				IMPORT_FUNC(glProgramUniform1uiEXT)
				IMPORT_FUNC(glProgramUniform1fEXT)
				IMPORT_FUNC(glProgramUniform2fvEXT)
				IMPORT_FUNC(glProgramUniform3fvEXT)
				IMPORT_FUNC(glProgramUniform4fvEXT)
				IMPORT_FUNC(glProgramUniformMatrix3fvEXT)
				IMPORT_FUNC(glProgramUniformMatrix4fvEXT)
				IMPORT_FUNC(glTextureImage1DEXT)
				IMPORT_FUNC(glTextureImage2DEXT)
				IMPORT_FUNC(glTextureImage3DEXT)
				IMPORT_FUNC(glTextureSubImage3DEXT)
				IMPORT_FUNC(glTextureParameteriEXT)
				IMPORT_FUNC(glGenerateTextureMipmapEXT)
				IMPORT_FUNC(glCompressedTextureImage1DEXT)
				IMPORT_FUNC(glCompressedTextureImage2DEXT)
				IMPORT_FUNC(glCompressedTextureImage3DEXT)
				IMPORT_FUNC(glVertexArrayVertexAttribOffsetEXT)
				IMPORT_FUNC(glEnableVertexArrayAttribEXT)
				IMPORT_FUNC(glBindMultiTextureEXT)
				IMPORT_FUNC(glNamedFramebufferTexture2DEXT)
				IMPORT_FUNC(glNamedFramebufferTextureLayerEXT)
				IMPORT_FUNC(glCheckNamedFramebufferStatusEXT)
				IMPORT_FUNC(glNamedRenderbufferStorageEXT);
				IMPORT_FUNC(glNamedFramebufferRenderbufferEXT);
				mGL_EXT_direct_state_access = result;
			}

			// GL_EXT_shader_image_load_store
			mGL_EXT_shader_image_load_store = isSupported("GL_EXT_shader_image_load_store");
			if (mGL_EXT_shader_image_load_store)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glBindImageTextureEXT)
				IMPORT_FUNC(glMemoryBarrierEXT)
				mGL_EXT_shader_image_load_store = result;
			}


			//[-------------------------------------------------------]
			//[ KHR                                                   ]
			//[-------------------------------------------------------]
			// GL_KHR_debug
			mGL_KHR_debug = isSupported("GL_KHR_debug");
			if (mGL_KHR_debug)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDebugMessageInsert)
				IMPORT_FUNC(glPushDebugGroup)
				IMPORT_FUNC(glPopDebugGroup)
				IMPORT_FUNC(glObjectLabel)
				mGL_KHR_debug = result;
			}


			//[-------------------------------------------------------]
			//[ ARB                                                   ]
			//[-------------------------------------------------------]
			// GL_ARB_framebuffer_object
			mGL_ARB_framebuffer_object = isSupported("GL_ARB_framebuffer_object");
			if (mGL_ARB_framebuffer_object)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glBindRenderbuffer)
				IMPORT_FUNC(glDeleteRenderbuffers)
				IMPORT_FUNC(glGenRenderbuffers)
				IMPORT_FUNC(glRenderbufferStorage)
				IMPORT_FUNC(glBindFramebuffer)
				IMPORT_FUNC(glDeleteFramebuffers)
				IMPORT_FUNC(glGenFramebuffers)
				IMPORT_FUNC(glCheckFramebufferStatus)
				IMPORT_FUNC(glFramebufferTexture2D)
				IMPORT_FUNC(glFramebufferTextureLayer)
				IMPORT_FUNC(glFramebufferRenderbuffer)
				IMPORT_FUNC(glBlitFramebuffer)
				IMPORT_FUNC(glGenerateMipmap)
				mGL_ARB_framebuffer_object = result;
			}

			// GL_ARB_multitexture - Is core feature since OpenGL 1.3
			mGL_ARB_multitexture = isCoreProfile ? true : isSupported("GL_ARB_multitexture");
			if (mGL_ARB_multitexture)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glActiveTextureARB)
				mGL_ARB_multitexture = result;
			}

			// GL_ARB_texture_multisample - Is core feature since OpenGL 3.2
			mGL_ARB_texture_multisample = isCoreProfile ? true : isSupported("GL_ARB_texture_multisample");
			if (mGL_ARB_texture_multisample)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glTexImage2DMultisample)
				mGL_ARB_texture_multisample = result;
			}

			// GL_ARB_vertex_buffer_object - Is core feature since OpenGL 1.5
			mGL_ARB_vertex_buffer_object = isCoreProfile ? true : isSupported("GL_ARB_vertex_buffer_object");
			if (mGL_ARB_vertex_buffer_object)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glBindBufferARB)
				IMPORT_FUNC(glDeleteBuffersARB)
				IMPORT_FUNC(glGenBuffersARB)
				IMPORT_FUNC(glBufferDataARB)
				IMPORT_FUNC(glBufferSubDataARB)
				IMPORT_FUNC(glMapBufferARB)
				IMPORT_FUNC(glUnmapBufferARB)
				mGL_ARB_vertex_buffer_object = result;
			}

			// GL_ARB_texture_compression - Is core since OpenGL 1.3
			mGL_ARB_texture_compression = isCoreProfile ? true : isSupported("GL_ARB_texture_compression");
			if (mGL_ARB_texture_compression)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glCompressedTexImage1DARB)
				IMPORT_FUNC(glCompressedTexImage2DARB)
				IMPORT_FUNC(glCompressedTexImage3DARB)
				mGL_ARB_texture_compression = result;
			}

			// GL_ARB_draw_buffers - Is core since OpenGL 2.0
			mGL_ARB_draw_buffers = isCoreProfile ? true : isSupported("GL_ARB_draw_buffers");
			if (mGL_ARB_draw_buffers)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawBuffersARB)
				mGL_ARB_draw_buffers = result;
			}

			// GL_ARB_shader_objects - Is core since OpenGL 2.0
			mGL_ARB_shader_objects = isCoreProfile ? true : isSupported("GL_ARB_shader_objects");
			if (mGL_ARB_shader_objects)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDeleteShader)
				IMPORT_FUNC(glGetHandleARB)
				IMPORT_FUNC(glDetachShader)
				IMPORT_FUNC(glCreateShader)
				IMPORT_FUNC(glShaderSource)
				IMPORT_FUNC(glCompileShader)
				IMPORT_FUNC(glCreateProgram)
				IMPORT_FUNC(glAttachShader)
				IMPORT_FUNC(glLinkProgram)
				IMPORT_FUNC(glUseProgram)
				IMPORT_FUNC(glUniform1f)
				IMPORT_FUNC(glUniform1i)
				IMPORT_FUNC(glUniform2fv)
				IMPORT_FUNC(glUniform3fv)
				IMPORT_FUNC(glUniform4fv)
				IMPORT_FUNC(glUniformMatrix3fv)
				IMPORT_FUNC(glUniformMatrix4fv)
				IMPORT_FUNC(glGetShaderiv)
				IMPORT_FUNC(glGetProgramiv)
				IMPORT_FUNC(glGetShaderInfoLog)
				IMPORT_FUNC(glGetProgramInfoLog)
				IMPORT_FUNC(glGetUniformLocation)
				mGL_ARB_shader_objects = result;
			}

			// GL_ARB_separate_shader_objects - Is core since OpenGL 4.1
			mGL_ARB_separate_shader_objects = isCoreProfile ? true : isSupported("GL_ARB_separate_shader_objects");
			if (mGL_ARB_separate_shader_objects)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glCreateShaderProgramv)
				IMPORT_FUNC(glDeleteProgram)
				IMPORT_FUNC(glGenProgramPipelines)
				IMPORT_FUNC(glDeleteProgramPipelines)
				IMPORT_FUNC(glBindProgramPipeline)
				IMPORT_FUNC(glUseProgramStages)
				IMPORT_FUNC(glValidateProgramPipeline)
				IMPORT_FUNC(glGetProgramPipelineiv)
				IMPORT_FUNC(glGetProgramPipelineInfoLog)
				IMPORT_FUNC(glActiveShaderProgram)
				mGL_ARB_separate_shader_objects = result;
			}

			// GL_ARB_get_program_binary - Is core since OpenGL 4.1
			mGL_ARB_get_program_binary = isCoreProfile ? true : isSupported("GL_ARB_get_program_binary");
			if (mGL_ARB_get_program_binary)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glProgramParameteri)
				mGL_ARB_get_program_binary = result;
			}

			// GL_ARB_uniform_buffer_object - Is core since OpenGL 3.1
			mGL_ARB_uniform_buffer_object = isCoreProfile ? true : isSupported("GL_ARB_uniform_buffer_object");
			if (mGL_ARB_uniform_buffer_object)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glGetUniformBlockIndex)
				IMPORT_FUNC(glUniformBlockBinding)
				IMPORT_FUNC(glBindBufferBase)
				mGL_ARB_uniform_buffer_object = result;
			}

			// GL_ARB_texture_buffer_object - Is core since OpenGL 3.1
			mGL_ARB_texture_buffer_object = isCoreProfile ? true : isSupported("GL_ARB_texture_buffer_object");
			if (mGL_ARB_texture_buffer_object)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glTexBufferARB)
				mGL_ARB_texture_buffer_object = result;
			}

			// GL_ARB_draw_indirect - Is core since OpenGL 4.0
			mGL_ARB_draw_indirect = isCoreProfile ? true : isSupported("GL_ARB_draw_indirect");
			if (mGL_ARB_draw_indirect)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawArraysIndirect)
				IMPORT_FUNC(glDrawElementsIndirect)
				mGL_ARB_draw_indirect = result;
			}

			// GL_ARB_multi_draw_indirect - Is core since OpenGL 4.3
			mGL_ARB_multi_draw_indirect = isSupported("GL_ARB_multi_draw_indirect");
			if (mGL_ARB_multi_draw_indirect)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glMultiDrawArraysIndirect)
				IMPORT_FUNC(glMultiDrawElementsIndirect)
				mGL_ARB_multi_draw_indirect = result;
			}

			// GL_ARB_vertex_shader - Is core since OpenGL 2.0
			mGL_ARB_vertex_shader = isCoreProfile ? true : isSupported("GL_ARB_vertex_shader");
			if (mGL_ARB_vertex_shader)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glBindAttribLocation)
				mGL_ARB_vertex_shader = result;
			}

			// GL_ARB_vertex_program
			mGL_ARB_vertex_program = isCoreProfile ? true : isSupported("GL_ARB_vertex_program");
			if (mGL_ARB_vertex_program)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glVertexAttribPointerARB)
				IMPORT_FUNC(glVertexAttribIPointer)	// GL_NV_vertex_program4
				IMPORT_FUNC(glEnableVertexAttribArrayARB)
				IMPORT_FUNC(glDisableVertexAttribArrayARB)
				mGL_ARB_vertex_program = result;
			}

			// GL_ARB_tessellation_shader - Is core since OpenGL 4.0
			mGL_ARB_tessellation_shader = isCoreProfile ? true : isSupported("GL_ARB_tessellation_shader");
			if (mGL_ARB_tessellation_shader)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glPatchParameteri)
				mGL_ARB_tessellation_shader = result;
			}

			// GL_ARB_geometry_shader4
			// TODO(sw) This extension was promoted to core feature but heavily modified source: https://www.khronos.org/opengl/wiki/History_of_OpenGL#OpenGL_3.2_.282009.29
			// TODO(sw) But this extension doesn't show up with mesa 3D either with an old OpenGL context (max OpenGL 3.3) or with an profile context (with OpenGL 4.3)
			mGL_ARB_geometry_shader4 = isSupported("GL_ARB_geometry_shader4");
			if (mGL_ARB_geometry_shader4)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glProgramParameteriARB)
				mGL_ARB_geometry_shader4 = result;
			}

			// GL_ARB_fragment_shader - Is core since OpenGL 2.0
			mGL_ARB_fragment_shader = isCoreProfile ? true : isSupported("GL_ARB_fragment_shader");

			// GL_ARB_fragment_program (we do not need any of the functions this extension provides)
			mGL_ARB_fragment_program = isCoreProfile ? true : isSupported("GL_ARB_fragment_program");

			// GL_ARB_compute_shader - Is core since OpenGL 4.3
			mGL_ARB_compute_shader = isSupported("GL_ARB_compute_shader");
			if (mGL_ARB_compute_shader)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDispatchCompute)
				mGL_ARB_compute_shader = result;
			}

			// GL_ARB_draw_instanced - Is core since OpenGL 3.1
			mGL_ARB_draw_instanced = isCoreProfile ? true : isSupported("GL_ARB_draw_instanced");
			if (mGL_ARB_draw_instanced)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawArraysInstancedARB)
				IMPORT_FUNC(glDrawElementsInstancedARB)
				mGL_ARB_draw_instanced = result;
			}

			// GL_ARB_base_instance - Is core since OpenGL 4.3
			mGL_ARB_base_instance = isSupported("GL_ARB_base_instance");
			if (mGL_ARB_base_instance)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawArraysInstancedBaseInstance)
				IMPORT_FUNC(glDrawElementsInstancedBaseInstance)
				IMPORT_FUNC(glDrawElementsInstancedBaseVertexBaseInstance)
				mGL_ARB_base_instance = result;
			}

			// GL_ARB_instanced_arrays - Is core since OpenGL 3.3
			mGL_ARB_instanced_arrays = isCoreProfile ? true : isSupported("GL_ARB_instanced_arrays");
			if (mGL_ARB_instanced_arrays)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glVertexAttribDivisorARB)
				mGL_ARB_instanced_arrays = result;
			}

			// GL_ARB_vertex_array_object
			mGL_ARB_vertex_array_object = isSupported("GL_ARB_vertex_array_object");
			if (mGL_ARB_vertex_array_object)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glBindVertexArray)
				IMPORT_FUNC(glDeleteVertexArrays)
				IMPORT_FUNC(glGenVertexArrays)
				mGL_ARB_vertex_array_object = result;
			}

			// GL_ARB_sampler_objects - Is core since OpenGL 3.3
			mGL_ARB_sampler_objects = isCoreProfile ? true : isSupported("GL_ARB_sampler_objects");
			if (mGL_ARB_sampler_objects)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glGenSamplers)
				IMPORT_FUNC(glDeleteSamplers)
				IMPORT_FUNC(glBindSampler)
				IMPORT_FUNC(glSamplerParameteri)
				IMPORT_FUNC(glSamplerParameterf)
				IMPORT_FUNC(glSamplerParameterfv)
				mGL_ARB_sampler_objects = result;
			}

			// GL_ARB_draw_elements_base_vertex - Is core since OpenGL 3.2
			mGL_ARB_draw_elements_base_vertex = isCoreProfile ? true : isSupported("GL_ARB_draw_elements_base_vertex");
			if (mGL_ARB_draw_elements_base_vertex)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawElementsBaseVertex)
				IMPORT_FUNC(glDrawElementsInstancedBaseVertex)
				mGL_ARB_draw_elements_base_vertex = result;
			}

			// GL_ARB_debug_output - Is core since OpenGL 4.3
			mGL_ARB_debug_output = isSupported("GL_ARB_debug_output");
			if (mGL_ARB_debug_output)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDebugMessageCallbackARB)
				IMPORT_FUNC(glDebugMessageControlARB)
				mGL_ARB_debug_output = result;
			}

			// GL_ARB_direct_state_access - Is core since OpenGL 4.5
			mGL_ARB_direct_state_access = isSupported("GL_ARB_direct_state_access");
			if (mGL_ARB_direct_state_access)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glCreateBuffers)
				IMPORT_FUNC(glCreateFramebuffers)
				IMPORT_FUNC(glCreateTextures)
				IMPORT_FUNC(glCreateVertexArrays)
				IMPORT_FUNC(glNamedBufferData)
				IMPORT_FUNC(glNamedBufferSubData)
				IMPORT_FUNC(glMapNamedBuffer)
				IMPORT_FUNC(glUnmapNamedBuffer)
				IMPORT_FUNC(glProgramUniform1i)
				IMPORT_FUNC(glProgramUniform1ui)
				IMPORT_FUNC(glProgramUniform1f)
				IMPORT_FUNC(glProgramUniform2fv)
				IMPORT_FUNC(glProgramUniform3fv)
				IMPORT_FUNC(glProgramUniform4fv)
				IMPORT_FUNC(glProgramUniformMatrix3fv)
				IMPORT_FUNC(glProgramUniformMatrix4fv)
				IMPORT_FUNC(glTextureParameteri)
				IMPORT_FUNC(glGenerateTextureMipmap)
				IMPORT_FUNC(glEnableVertexArrayAttrib)
				IMPORT_FUNC(glCheckNamedFramebufferStatus)
				IMPORT_FUNC(glNamedRenderbufferStorage)
				IMPORT_FUNC(glNamedFramebufferRenderbuffer)
				IMPORT_FUNC(glNamedFramebufferTexture)
				IMPORT_FUNC(glNamedFramebufferTextureLayer)
				IMPORT_FUNC(glTextureBuffer)
				IMPORT_FUNC(glBindTextureUnit)
				IMPORT_FUNC(glCompressedTextureSubImage1D)
				IMPORT_FUNC(glCompressedTextureSubImage2D)
				IMPORT_FUNC(glCompressedTextureSubImage3D)
				IMPORT_FUNC(glTextureSubImage1D)
				IMPORT_FUNC(glTextureSubImage2D)
				IMPORT_FUNC(glTextureSubImage3D)
				IMPORT_FUNC(glVertexArrayAttribFormat)
				IMPORT_FUNC(glVertexArrayAttribIFormat)
				IMPORT_FUNC(glVertexArrayAttribBinding)
				IMPORT_FUNC(glVertexArrayVertexBuffer)
				IMPORT_FUNC(glVertexArrayBindingDivisor)
				IMPORT_FUNC(glVertexArrayElementBuffer)
				mGL_ARB_direct_state_access = result;
			}

			// GL_ARB_texture_storage - Is core since OpenGL 4.5
			mGL_ARB_texture_storage = isSupported("GL_ARB_texture_storage");
			if (mGL_ARB_texture_storage)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glTextureStorage1D)
				IMPORT_FUNC(glTextureStorage2D)
				IMPORT_FUNC(glTextureStorage3D)
				IMPORT_FUNC(glTextureStorage2DMultisample)
				mGL_ARB_texture_storage = result;
			}

			// GL_ARB_shader_storage_buffer_object - Is core since OpenGL 4.3
			mGL_ARB_shader_storage_buffer_object = isSupported("GL_ARB_shader_storage_buffer_object");

			// GL_ARB_copy_image - Is core since OpenGL 4.3
			mGL_ARB_copy_image = isSupported("GL_ARB_copy_image");
			if (mGL_ARB_copy_image)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glCopyImageSubData)
				mGL_ARB_copy_image = result;
			}

			// GL_ARB_gl_spirv
			mGL_ARB_gl_spirv = isSupported("GL_ARB_gl_spirv");
			if (mGL_ARB_gl_spirv)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glSpecializeShaderARB)
				mGL_ARB_gl_spirv = result;

				// TODO(co) "GL_ARB_gl_spirv"-support is under construction
				mGL_ARB_gl_spirv = false;
			}

			// GL_ARB_clip_control
			mGL_ARB_clip_control = isSupported("GL_ARB_clip_control");
			if (mGL_ARB_clip_control)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glClipControl)
				mGL_ARB_clip_control = result;
			}

			// GL_ARB_occlusion_query - Is core since OpenGL 1.5
			mGL_ARB_occlusion_query = isCoreProfile ? true : isSupported("GL_ARB_occlusion_query");
			if (mGL_ARB_occlusion_query)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glGenQueriesARB)
				IMPORT_FUNC(glDeleteQueriesARB)
				IMPORT_FUNC(glBeginQueryARB)
				IMPORT_FUNC(glEndQueryARB)
				IMPORT_FUNC(glGetQueryObjectuivARB)
				mGL_ARB_occlusion_query = result;
			}

			// GL_ARB_pipeline_statistics_query
			mGL_ARB_pipeline_statistics_query = isSupported("GL_ARB_pipeline_statistics_query");

			// GL_ARB_timer_query - Is core since OpenGL 3.3
			mGL_ARB_timer_query = isCoreProfile ? true : isSupported("GL_ARB_timer_query");
			if (mGL_ARB_timer_query)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glQueryCounter)
				mGL_ARB_timer_query = result;
			}


			//[-------------------------------------------------------]
			//[ Core (OpenGL version dependent)                       ]
			//[-------------------------------------------------------]
			{
				[[maybe_unused]] bool result = true;	// Success by default
				IMPORT_FUNC(glShaderBinary)	// OpenGL 4.1
			}


			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return true;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLRenderer& mOpenGLRenderer;	///< Owner OpenGL renderer instance
		IOpenGLContext* mOpenGLContext;		///< Owner OpenGL context, always valid!
		bool			mInitialized;		///< Are the extensions initialized?

		// Supported extensions
		// WGL (Windows only)
		bool mWGL_ARB_extensions_string;
		bool mWGL_EXT_swap_control;
		bool mWGL_EXT_swap_control_tear;
		// EXT
		bool mGL_EXT_texture_lod_bias;
		bool mGL_EXT_texture_filter_anisotropic;
		bool mGL_EXT_texture_array;
		bool mGL_EXT_texture3D;
		bool mGL_EXT_direct_state_access;
		bool mGL_EXT_shader_image_load_store;
		// KHR
		bool mGL_KHR_debug;
		// ARB
		bool mGL_ARB_framebuffer_object;
		bool mGL_ARB_multitexture;
		bool mGL_ARB_texture_multisample;
		bool mGL_ARB_vertex_buffer_object;
		bool mGL_ARB_texture_compression;
		bool mGL_ARB_draw_buffers;
		bool mGL_ARB_shader_objects;
		bool mGL_ARB_separate_shader_objects;
		bool mGL_ARB_get_program_binary;
		bool mGL_ARB_uniform_buffer_object;
		bool mGL_ARB_texture_buffer_object;
		bool mGL_ARB_draw_indirect;
		bool mGL_ARB_multi_draw_indirect;
		bool mGL_ARB_vertex_shader;
		bool mGL_ARB_vertex_program;
		bool mGL_ARB_tessellation_shader;
		bool mGL_ARB_geometry_shader4;
		bool mGL_ARB_fragment_shader;
		bool mGL_ARB_fragment_program;
		bool mGL_ARB_compute_shader;
		bool mGL_ARB_draw_instanced;
		bool mGL_ARB_base_instance;
		bool mGL_ARB_instanced_arrays;
		bool mGL_ARB_vertex_array_object;
		bool mGL_ARB_sampler_objects;
		bool mGL_ARB_draw_elements_base_vertex;
		bool mGL_ARB_debug_output;
		bool mGL_ARB_direct_state_access;
		bool mGL_ARB_texture_storage;
		bool mGL_ARB_shader_storage_buffer_object;
		bool mGL_ARB_copy_image;
		bool mGL_ARB_gl_spirv;
		bool mGL_ARB_clip_control;
		bool mGL_ARB_occlusion_query;
		bool mGL_ARB_pipeline_statistics_query;
		bool mGL_ARB_timer_query;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/IOpenGLContext.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL context interface
	*
	*  @remarks
	*    While the OpenGL specification is platform independent, creating an OpenGL context is not.
	*
	*  @note
	*    - Every native OS window needs its own context instance
	*/
	class IOpenGLContext
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IOpenGLContext()
		{}


	//[-------------------------------------------------------]
	//[ Public virtual IOpenGLContext methods                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return whether or not the content is initialized
		*
		*  @return
		*    "true" if the context is initialized, else "false"
		*/
		[[nodiscard]] virtual bool isInitialized() const = 0;

		/**
		*  @brief
		*    Make the context current
		*/
		virtual void makeCurrent() const = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRuntimeLinking
		*    OpenGL runtime linking instance, if null pointer this isn't a primary context
		*/
		inline explicit IOpenGLContext(OpenGLRuntimeLinking* openGLRuntimeLinking) :
			mOpenGLRuntimeLinking(openGLRuntimeLinking)
		{}

		explicit IOpenGLContext(const IOpenGLContext& source) = delete;
		IOpenGLContext& operator =(const IOpenGLContext& source) = delete;

		/**
		*  @brief
		*    Load the >= OpenGL 3.0 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @note
		*    - This method is only allowed to be called after an >= OpenGL context has been created and set
		*/
		[[nodiscard]] inline bool loadOpenGL3EntryPoints() const
		{
			return (nullptr != mOpenGLRuntimeLinking) ? mOpenGLRuntimeLinking->loadOpenGL3EntryPoints() : true;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLRuntimeLinking* mOpenGLRuntimeLinking;	///< OpenGL runtime linking instance, if null pointer this isn't a primary context


	};




	#ifdef _WIN32
		//[-------------------------------------------------------]
		//[ OpenGLRenderer/OpenGLContextWindows.h                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Windows OpenGL context class
		*/
		class OpenGLContextWindows : public IOpenGLContext
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
			friend class OpenGLRenderer;


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] depthStencilAttachmentTextureFormat
			*    Depth stencil attachment texture format
			*  @param[in] nativeWindowHandle
			*    Optional native main window handle, can be a null handle
			*  @param[in] shareContextWindows
			*    Optional share context, can be a null pointer
			*/
			inline OpenGLContextWindows(Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, Renderer::handle nativeWindowHandle, const OpenGLContextWindows* shareContextWindows = nullptr) :
				OpenGLContextWindows(nullptr, depthStencilAttachmentTextureFormat, nativeWindowHandle, shareContextWindows)
			{}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~OpenGLContextWindows() override
			{
				// Release the device context of the OpenGL window
				if (NULL_HANDLE != mWindowDeviceContext)
				{
					// Is the device context of the OpenGL window is the currently active OpenGL device context?
					if (wglGetCurrentDC() == mWindowDeviceContext)
					{
						wglMakeCurrent(nullptr, nullptr);
					}

					// Destroy the render context of the OpenGL window
					if (NULL_HANDLE != mWindowRenderContext && mOwnsRenderContext)
					{
						wglDeleteContext(mWindowRenderContext);
					}

					// Release the device context of the OpenGL window
					if (NULL_HANDLE != mNativeWindowHandle)
					{
						::ReleaseDC(reinterpret_cast<HWND>(mNativeWindowHandle), mWindowDeviceContext);
					}
				}

				// Destroy the OpenGL dummy window, in case there's one
				if (NULL_HANDLE != mDummyWindow)
				{
					// Destroy the OpenGL dummy window
					::DestroyWindow(reinterpret_cast<HWND>(mDummyWindow));

					// Unregister the window class for the OpenGL dummy window
					::UnregisterClass(TEXT("OpenGLDummyWindow"), ::GetModuleHandle(nullptr));
				}
			}

			/**
			*  @brief
			*    Return the primary device context
			*
			*  @return
			*    The primary device context, null pointer on error
			*/
			[[nodiscard]] inline HDC getDeviceContext() const
			{
				return mWindowDeviceContext;
			}

			/**
			*  @brief
			*    Return the primary render context
			*
			*  @return
			*    The primary render context, null pointer on error
			*/
			[[nodiscard]] inline HGLRC getRenderContext() const
			{
				return mWindowRenderContext;
			}


		//[-------------------------------------------------------]
		//[ Public virtual OpenGLRenderer::IOpenGLContext methods ]
		//[-------------------------------------------------------]
		public:
			[[nodiscard]] inline virtual bool isInitialized() const override
			{
				return (nullptr != mWindowRenderContext);
			}

			inline virtual void makeCurrent() const override
			{
				wglMakeCurrent(mWindowDeviceContext, mWindowRenderContext);
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit OpenGLContextWindows(const OpenGLContextWindows& source) = delete;
			OpenGLContextWindows& operator =(const OpenGLContextWindows& source) = delete;

			/**
			*  @brief
			*    Constructor for primary context
			*
			*  @param[in] openGLRuntimeLinking
			*    OpenGL runtime linking instance, if null pointer this isn't a primary context
			*  @param[in] depthStencilAttachmentTextureFormat
			*    Depth stencil attachment texture format
			*  @param[in] nativeWindowHandle
			*    Optional native main window handle, can be a null handle
			*  @param[in] shareContextWindows
			*    Optional share context, can be a null pointer
			*/
			OpenGLContextWindows(OpenGLRuntimeLinking* openGLRuntimeLinking, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, Renderer::handle nativeWindowHandle, const OpenGLContextWindows* shareContextWindows = nullptr) :
				IOpenGLContext(openGLRuntimeLinking),
				mNativeWindowHandle(nativeWindowHandle),
				mDummyWindow(NULL_HANDLE),
				mWindowDeviceContext(NULL_HANDLE),
				mWindowRenderContext(NULL_HANDLE),
				mOwnsRenderContext(true)
			{
				// Create a OpenGL dummy window?
				// -> Under Microsoft Windows, a OpenGL context is always coupled to a window... even if we're not going to render into a window at all...
				if (NULL_HANDLE == mNativeWindowHandle)
				{
					// Setup and register the window class for the OpenGL dummy window
					WNDCLASS windowDummyClass;
					windowDummyClass.hInstance		= ::GetModuleHandle(nullptr);
					windowDummyClass.lpszClassName	= TEXT("OpenGLDummyWindow");
					windowDummyClass.lpfnWndProc	= DefWindowProc;
					windowDummyClass.style			= 0;
					windowDummyClass.hIcon			= nullptr;
					windowDummyClass.hCursor		= nullptr;
					windowDummyClass.lpszMenuName	= nullptr;
					windowDummyClass.cbClsExtra		= 0;
					windowDummyClass.cbWndExtra		= 0;
					windowDummyClass.hbrBackground	= nullptr;
					::RegisterClass(&windowDummyClass);

					// Create the OpenGL dummy window
					mNativeWindowHandle = mDummyWindow = reinterpret_cast<Renderer::handle>(::CreateWindow(TEXT("OpenGLDummyWindow"), TEXT("PFormat"), WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, nullptr, ::GetModuleHandle(nullptr), nullptr));
				}

				// Is there a valid window handle?
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					// Get the device context of the OpenGL window
					mWindowDeviceContext = ::GetDC(reinterpret_cast<HWND>(mNativeWindowHandle));
					if (NULL_HANDLE != mWindowDeviceContext)
					{
						// Get the color depth of the desktop
						int bits = 32;
						{
							HDC deskTopDC = ::GetDC(nullptr);
							bits = ::GetDeviceCaps(deskTopDC, BITSPIXEL);
							::ReleaseDC(nullptr, deskTopDC);
						}

						// Get the first best pixel format
						// TODO(co) Use more detailed color and depth/stencil information from render pass
						const BYTE depthBufferBits = (Renderer::TextureFormat::Enum::UNKNOWN == depthStencilAttachmentTextureFormat) ? 0u : 24u;
						const PIXELFORMATDESCRIPTOR pixelFormatDescriptor =
						{
							sizeof(PIXELFORMATDESCRIPTOR),	// Size of this pixel format descriptor
							1,								// Version number
							PFD_DRAW_TO_WINDOW |			// Format must support window
							PFD_SUPPORT_OPENGL |			// Format must support OpenGL
							PFD_DOUBLEBUFFER,				// Must support double buffering
							PFD_TYPE_RGBA,					// Request an RGBA format
							static_cast<UCHAR>(bits),		// Select our color depth
							0, 0, 0, 0, 0, 0,				// Color bits ignored
							0,								// No alpha buffer
							0,								// Shift bit ignored
							0,								// No accumulation buffer
							0, 0, 0, 0,						// Accumulation bits ignored
							depthBufferBits,				// Z-buffer (depth buffer)
							0,								// No stencil buffer
							0,								// No auxiliary buffer
							PFD_MAIN_PLANE,					// Main drawing layer
							0,								// Reserved
							0, 0, 0							// Layer masks ignored
						};
						const int pixelFormat = ::ChoosePixelFormat(mWindowDeviceContext, &pixelFormatDescriptor);
						if (0 != pixelFormat)
						{
							// Set the pixel format
							::SetPixelFormat(mWindowDeviceContext, pixelFormat, &pixelFormatDescriptor);

							// Lookout! OpenGL context sharing chaos: https://www.opengl.org/wiki/OpenGL_Context
							// "State" objects are not shared between contexts, including but not limited to:
							// - Vertex Array Objects (VAOs)
							// - Framebuffer Objects (FBOs)
							// -> Keep away from "wglShareLists()" and the share context parameter of "wglCreateContextAttribsARB()" and just share the OpenGL render context instead
							if (nullptr != shareContextWindows)
							{
								mWindowRenderContext = shareContextWindows->getRenderContext();
								mOwnsRenderContext = false;
							}
							else
							{
								// Create a legacy OpenGL render context
								HGLRC legacyRenderContext = wglCreateContext(mWindowDeviceContext);
								if (NULL_HANDLE != legacyRenderContext)
								{
									// Make the legacy OpenGL render context to the current one
									wglMakeCurrent(mWindowDeviceContext, legacyRenderContext);

									// Load the >= OpenGL 3.0 entry points
									if (loadOpenGL3EntryPoints())
									{
										// Create the render context of the OpenGL window
										mWindowRenderContext = createOpenGLContext(nullptr);

										// Destroy the legacy OpenGL render context
										wglMakeCurrent(nullptr, nullptr);
										wglDeleteContext(legacyRenderContext);

										// If there's an OpenGL context, do some final initialization steps
										if (NULL_HANDLE != mWindowRenderContext)
										{
											// Make the OpenGL context to the current one
											// TODO(co) Review this, might cause issues when creating a context while a program is running
											wglMakeCurrent(mWindowDeviceContext, mWindowRenderContext);
										}
									}
									else
									{
										// Error, failed to load >= OpenGL 3 entry points!
									}
								}
								else
								{
									// Error, failed to create a legacy OpenGL render context!
								}
							}
						}
						else
						{
							// Error, failed to choose a pixel format!
						}
					}
					else
					{
						// Error, failed to obtain the device context of the OpenGL window!
					}
				}
				else
				{
					// Error, failed to create the OpenGL window!
				}
			}

			/**
			*  @brief
			*    Create a OpenGL context
			*
			*  @param[in] shareContextWindows
			*    Optional share context, can be a null pointer
			*
			*  @return
			*    The created OpenGL context, null pointer on error
			*/
			[[nodiscard]] HGLRC createOpenGLContext(const OpenGLContextWindows* shareContextWindows)
			{
				// Disable the following warning, we can't do anything to resolve this warning
				PRAGMA_WARNING_PUSH
				PRAGMA_WARNING_DISABLE_MSVC(4191)	// warning C4191: 'reinterpret_cast' : unsafe conversion from 'PROC' to '<x>'

				// Get the OpenGL extension wglGetExtensionsStringARB function pointer, we need it to check for further supported OpenGL extensions
				PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARBLocal = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(wglGetProcAddress("wglGetExtensionsStringARB"));
				if (nullptr != wglGetExtensionsStringARBLocal)
				{
					// Get the available WGL extensions as string
					const char* extensions = wglGetExtensionsStringARBLocal(mWindowDeviceContext);

					// Check whether or not "WGL_ARB_create_context" is a substring of the WGL extension string meaning that this OpenGL extension is supported
					if (nullptr != strstr(extensions, "WGL_ARB_create_context"))
					{
						// Get the OpenGL extension "wglCreateContextAttribsARB" function pointer
						PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
						if (nullptr != wglCreateContextAttribsARB)
						{
							// Create the OpenGL context
							// -> OpenGL 4.1 (the best OpenGL version Mac OS X 10.11 supports, so lowest version we have to support)
							static const int ATTRIBUTES[] =
							{
								// We want an OpenGL context
								WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
								WGL_CONTEXT_MINOR_VERSION_ARB, 1,
								WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
								#ifdef RENDERER_DEBUG
									// "WGL_CONTEXT_DEBUG_BIT_ARB" comes from the "GL_ARB_debug_output"-extension
									WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
								#else
									// "WGL_ARB_create_context_no_error" and "GL_KHR_no_error"-extension
									((nullptr != strstr(extensions, "WGL_ARB_create_context_no_error")) ? WGL_CONTEXT_OPENGL_NO_ERROR_ARB : 0), 1,
								#endif
								// Done
								0
							};

							// Lookout! OpenGL context sharing chaos: https://www.opengl.org/wiki/OpenGL_Context
							// "State" objects are not shared between contexts, including but not limited to:
							// - Vertex Array Objects (VAOs)
							// - Framebuffer Objects (FBOs)
							// -> Practically, this makes a second OpenGL context only useful for resource background loading
							const HGLRC hglrc = wglCreateContextAttribsARB(mWindowDeviceContext, (nullptr != shareContextWindows) ? shareContextWindows->getRenderContext() : nullptr, ATTRIBUTES);
							if (nullptr != hglrc)
							{
								// Done
								return hglrc;
							}
							else
							{
								// Error, context creation failed!
								return NULL_HANDLE;
							}
						}
						else
						{
							// Error, failed to obtain the "wglCreateContextAttribsARB" function pointer (wow, something went terrible wrong!)
							return NULL_HANDLE;
						}
					}
					else
					{
						// Error, the OpenGL extension "WGL_ARB_create_context" is not supported... as a result we can't create an OpenGL context!
						return NULL_HANDLE;
					}
				}
				else
				{
					// Error, failed to obtain the "wglGetExtensionsStringARB" function pointer (wow, something went terrible wrong!)
					return NULL_HANDLE;
				}

				// Restore the previous warning configuration
				PRAGMA_WARNING_POP
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Renderer::handle mNativeWindowHandle;	///< OpenGL window, can be a null pointer (HWND)
			Renderer::handle mDummyWindow;			///< OpenGL dummy window, can be a null pointer (HWND)
			HDC				 mWindowDeviceContext;	///< The device context of the OpenGL dummy window, can be a null pointer
			HGLRC			 mWindowRenderContext;	///< The render context of the OpenGL dummy window, can be a null pointer
			bool			 mOwnsRenderContext;	///< Does this context owns the OpenGL render context?


		};
	#elif LINUX
		//[-------------------------------------------------------]
		//[ OpenGLRenderer/OpenGLContextLinux.h                   ]
		//[-------------------------------------------------------]
		// TODO(co) Cleanup
		static bool ctxErrorOccurred = false;
		[[nodiscard]] static int ctxErrorHandler(Display*, XErrorEvent*)
		{
			ctxErrorOccurred = true;
			return 0;
		}

		/**
		*  @brief
		*    Linux OpenGL context class
		*/
		class OpenGLContextLinux final : public IOpenGLContext
		{


		//[-------------------------------------------------------]
		//[ Friends                                               ]
		//[-------------------------------------------------------]
			friend class OpenGLRenderer;


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] openGLRenderer
			*    Owner OpenGL renderer instance
			*  @param[in] depthStencilAttachmentTextureFormat
			*    Depth stencil attachment texture format
			*  @param[in] nativeWindowHandle
			*    Optional native main window handle, can be a null handle
			*  @param[in] useExternalContext
			*    When true an own OpenGL context won't be created and the context pointed by "shareContextLinux" is ignored
			*  @param[in] shareContextLinux
			*    Optional share context, can be a null pointer
			*/
			inline OpenGLContextLinux(OpenGLRenderer& openGLRenderer, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, Renderer::handle nativeWindowHandle, bool useExternalContext, const OpenGLContextLinux* shareContextLinux = nullptr) :
				OpenGLContextLinux(openGLRenderer, nullptr, depthStencilAttachmentTextureFormat, nativeWindowHandle, useExternalContext, shareContextLinux)
			{}

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~OpenGLContextLinux() override
			{
				// Release the device context of the OpenGL window
				if (nullptr != mDisplay)
				{
					// Is the device context of the OpenGL window is the currently active OpenGL device context?
					if (glXGetCurrentContext() == mWindowRenderContext)
					{
						glXMakeCurrent(mDisplay, None, nullptr);
					}

					// Destroy the render context of the OpenGL window
					if (NULL_HANDLE != mWindowRenderContext && mOwnsRenderContext)
					{
						glXDestroyContext(mDisplay, mWindowRenderContext);
					}

					if (mOwnsX11Display)
					{
						XCloseDisplay(mDisplay);
					}
				}
			}

			/**
			*  @brief
			*    Return the primary device context
			*
			*  @return
			*    The primary device context, null pointer on error
			*/
			[[nodiscard]] inline Display* getDisplay() const
			{
				return mDisplay;
			}

			/**
			*  @brief
			*    Return the primary render context
			*
			*  @return
			*    The primary render context, null pointer on error
			*/
			[[nodiscard]] inline GLXContext getRenderContext() const
			{
				return mWindowRenderContext;
			}


		//[-------------------------------------------------------]
		//[ Public virtual OpenGLRenderer::IOpenGLContext methods ]
		//[-------------------------------------------------------]
		public:
			[[nodiscard]] inline virtual bool isInitialized() const override
			{
				return (nullptr != mWindowRenderContext || mUseExternalContext);
			}

			virtual void makeCurrent() const override
			{
				// Only do something when have created our renderer context and don't use a external renderer context
				if (!mUseExternalContext)
				{
					glXMakeCurrent(getDisplay(), mNativeWindowHandle, getRenderContext());
				}
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit OpenGLContextLinux(const OpenGLContextLinux& source) = delete;
			OpenGLContextLinux& operator =(const OpenGLContextLinux& source) = delete;

			/**
			*  @brief
			*    Constructor for primary context
			*
			*  @param[in] openGLRenderer
			*    Owner OpenGL renderer instance
			*  @param[in] openGLRuntimeLinking
			*    OpenGL runtime linking instance, if null pointer this isn't a primary context
			*  @param[in] depthStencilAttachmentTextureFormat
			*    Depth stencil attachment texture format
			*  @param[in] nativeWindowHandle
			*    Optional native main window handle, can be a null handle
			*  @param[in] useExternalContext
			*    When true an own OpenGL context won't be created and the context pointed by "shareContextLinux" is ignored
			*  @param[in] shareContextLinux
			*    Optional share context, can be a null pointer
			*/
			OpenGLContextLinux(OpenGLRenderer& openGLRenderer, OpenGLRuntimeLinking* openGLRuntimeLinking, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, Renderer::handle nativeWindowHandle, bool useExternalContext, const OpenGLContextLinux* shareContextLinux = nullptr) :
				IOpenGLContext(openGLRuntimeLinking),
				mOpenGLRenderer(openGLRenderer),
				mNativeWindowHandle(nativeWindowHandle),
				mDisplay(nullptr),
				mOwnsX11Display(true),
				mWindowRenderContext(NULL_HANDLE),
				mUseExternalContext(useExternalContext),
				mOwnsRenderContext(true)
			{
				if (mUseExternalContext)
				{
					// We use an external context so just load the OpenGL 3 entry points
					[[maybe_unused]] const bool result = loadOpenGL3EntryPoints();
				}
				else
				{
					const Renderer::Context& context = openGLRenderer.getContext();
					RENDERER_ASSERT(context, context.getType() == Renderer::Context::ContextType::X11, "Invalid OpenGL context type")

					// If the given renderer context is an X11 context use the display connection object provided by the context
					if (context.getType() == Renderer::Context::ContextType::X11)
					{
						mDisplay = static_cast<const Renderer::X11Context&>(context).getDisplay();
						mOwnsX11Display = mDisplay == nullptr;
					}

					if (mOwnsX11Display)
					{
						mDisplay = XOpenDisplay(0);
					}
				}
				if (nullptr != mDisplay)
				{
					// Lookout! OpenGL context sharing chaos: https://www.opengl.org/wiki/OpenGL_Context
					// "State" objects are not shared between contexts, including but not limited to:
					// - Vertex Array Objects (VAOs)
					// - Framebuffer Objects (FBOs)
					// -> Keep away from the share context parameter of "glxCreateContextAttribsARB()" and just share the OpenGL render context instead
					if (nullptr != shareContextLinux)
					{
						mWindowRenderContext = shareContextLinux->getRenderContext();
						mOwnsRenderContext = false;
					}
					else
					{
						// TODO(sw) We don't need a dummy context to load gl/glx entry points see "Misconception #2" from https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/
						// Load the >= OpenGL 3.0 entry points
						if (loadOpenGL3EntryPoints())
						{
							// Create the render context of the OpenGL window
							mWindowRenderContext = createOpenGLContext(depthStencilAttachmentTextureFormat);

							// If there's an OpenGL context, do some final initialization steps
							if (NULL_HANDLE != mWindowRenderContext)
							{
								// Make the OpenGL context to the current one, native window handle can be zero -> thus only offscreen rendering is supported/wanted
								const int result = glXMakeCurrent(mDisplay, mNativeWindowHandle, mWindowRenderContext);
								RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "Make new OpenGL context current: %d", result)
								{
									int major = 0;
									glGetIntegerv(GL_MAJOR_VERSION, &major);

									int minor = 0;
									glGetIntegerv(GL_MINOR_VERSION, &minor);

									GLint profile = 0;
									glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

									RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "OpenGL context version: %d.%d %s", major, minor, ((profile & GL_CONTEXT_CORE_PROFILE_BIT) ? "core" : "noncore"))
									int numberOfExtensions = 0;
									glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExtensions);
									RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "Number of supported OpenGL extensions: %d", numberOfExtensions)
									for (GLuint extensionIndex = 0; extensionIndex < static_cast<GLuint>(numberOfExtensions); ++extensionIndex)
									{
										RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "%s", glGetStringi(GL_EXTENSIONS, extensionIndex))
									}
								}
							}
						}
						else
						{
							// Error, failed to load >= OpenGL 3 entry points!
						}
					}
				}
				else
				{
					// Error, failed to get display!
				}
			}

			/**
			*  @brief
			*    Create a OpenGL context
			*
			*  @param[in] depthStencilAttachmentTextureFormat
			*    Depth stencil attachment texture format
			*
			*  @return
			*    The created OpenGL context, null pointer on error
			*/
			[[nodiscard]] GLXContext createOpenGLContext(Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat)
			{
				#define GLX_CONTEXT_MAJOR_VERSION_ARB	0x2091
				#define GLX_CONTEXT_MINOR_VERSION_ARB	0x2092

				// Get the available GLX extensions as string
				const char* extensions = glXQueryExtensionsString(mDisplay, XDefaultScreen(mDisplay));

				// Check whether or not "GLX_ARB_create_context" is a substring of the GLX extension string meaning that this OpenGL extension is supported
				if (nullptr != strstr(extensions, "GLX_ARB_create_context"))
				{
					// Get the OpenGL extension "glXCreateContextAttribsARB" function pointer
					typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
					GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = reinterpret_cast<GLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB"));
					if (nullptr != glXCreateContextAttribsARB)
					{
						// TODO(co) Cleanup
						ctxErrorOccurred = false;
						int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

						// Create the OpenGL context
						// -> OpenGL 4.1 (the best OpenGL version Mac OS X 10.11 supports, so lowest version we have to support)
						// TODO(co) Add support for the "GL_KHR_no_error"-extension
						int ATTRIBUTES[] =
						{
							// We want an OpenGL context
							GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
							GLX_CONTEXT_MINOR_VERSION_ARB, 1,
							// -> "GLX_CONTEXT_DEBUG_BIT_ARB" comes from the "GL_ARB_debug_output"-extension
							GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
							// TODO(co) Make it possible to activate "GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB" from the outside
							#ifdef RENDERER_DEBUG
							//	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
							//	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,	// Error messages like "Implicit version number 110 not supported by GL3 forward compatible context" might occur
							#else
							//	GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,	// Error messages like "Implicit version number 110 not supported by GL3 forward compatible context" might occur
							#endif
							// Done
							0
						};

						// TODO(co) Use more detailed color and depth/stencil information from render pass
						const int depthBufferBits = 24;//(Renderer::TextureFormat::Enum::UNKNOWN == depthStencilAttachmentTextureFormat) ? 24 : 24;
						int numberOfElements = 0;
						int visualAttributes[] =
						{
							GLX_RENDER_TYPE,	GLX_RGBA_BIT,
							GLX_DOUBLEBUFFER,	true,
							GLX_RED_SIZE,		8,
							GLX_GREEN_SIZE,		8,
							GLX_BLUE_SIZE,		8,
							GLX_ALPHA_SIZE,		8,
							GLX_DEPTH_SIZE,		depthBufferBits,
							GLX_STENCIL_SIZE,	8,
							None
						};
						GLXFBConfig* fbc = glXChooseFBConfig(mDisplay, DefaultScreen(mDisplay), visualAttributes, &numberOfElements);
						RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "Got %d of OpenGL GLXFBConfig", numberOfElements)
						GLXContext glxContext = glXCreateContextAttribsARB(mDisplay, *fbc, 0, true, ATTRIBUTES);

						XSync(mDisplay, False);

						// TODO(sw) make this fallback optional (via an option)
						if (ctxErrorOccurred)
						{
							RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "Could not create OpenGL 3+ context try creating pre 3+ context")
							ctxErrorOccurred = false;

							// GLX_CONTEXT_MAJOR_VERSION_ARB = 1
							ATTRIBUTES[1] = 1;
							// GLX_CONTEXT_MINOR_VERSION_ARB = 0
							ATTRIBUTES[3] = 0;
							glxContext = glXCreateContextAttribsARB(mDisplay, *fbc, 0, true, ATTRIBUTES);

							// Synchronize to ensure any errors generated are processed
							XSync(mDisplay, False);

							// Restore the original error handler
							XSetErrorHandler(oldHandler);
						}

						if (nullptr != glxContext)
						{
							// Done
							RENDERER_LOG(mOpenGLRenderer.getContext(), DEBUG, "OpenGL context with glXCreateContextAttribsARB created")
							return glxContext;
						}
						else
						{
							// Error, context creation failed!
							RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Could not create OpenGL context with glXCreateContextAttribsARB")
							return NULL_HANDLE;
						}
					}
					else
					{
						// Error, failed to obtain the "GLX_ARB_create_context" function pointer (wow, something went terrible wrong!)
						RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "Could not find OpenGL glXCreateContextAttribsARB")
						return NULL_HANDLE;
					}
				}
				else
				{
					// Error, the OpenGL extension "GLX_ARB_create_context" is not supported... as a result we can't create an OpenGL context!
					RENDERER_LOG(mOpenGLRenderer.getContext(), CRITICAL, "OpenGL GLX_ARB_create_context not supported")
					return NULL_HANDLE;
				}

				#undef GLX_CONTEXT_MAJOR_VERSION_ARB
				#undef GLX_CONTEXT_MINOR_VERSION_ARB
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			OpenGLRenderer&	 mOpenGLRenderer;		///< Owner OpenGL renderer instance
			Renderer::handle mNativeWindowHandle;	///< OpenGL window, can be a null pointer (Window)
			Display*		 mDisplay;				///< The X11 display connection, can be a null pointer
			bool			 mOwnsX11Display;		///< Indicates if this instance owns the X11 display
			GLXContext		 mWindowRenderContext;	///< The render context of the OpenGL dummy window, can be a null pointer
			bool			 mUseExternalContext;
			bool			 mOwnsRenderContext;	///< Does this context own the OpenGL render context?


		};
	#else
		#error "Unsupported platform"
	#endif




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Mapping.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL mapping
	*/
	class Mapping final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Renderer::FilterMode                                  ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::FilterMode" to OpenGL magnification filter mode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    OpenGL magnification filter mode
		*/
		[[nodiscard]] static GLint getOpenGLMagFilterMode([[maybe_unused]] const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return GL_NEAREST;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return GL_NEAREST;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return GL_LINEAR;	// There's no special setting in OpenGL

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return GL_LINEAR;	// There's no special setting in OpenGL

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "OpenGL filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Renderer::FilterMode" to OpenGL minification filter mode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*  @param[in] hasMipmaps
		*    Are mipmaps available?
		*
		*  @return
		*    OpenGL minification filter mode
		*/
		[[nodiscard]] static GLint getOpenGLMinFilterMode([[maybe_unused]] const Renderer::Context& context, Renderer::FilterMode filterMode, bool hasMipmaps)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;	// There's no special setting in OpenGL

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;	// There's no special setting in OpenGL

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "OpenGL filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Renderer::FilterMode" to OpenGL compare mode
		*
		*  @param[in] context
		*    Renderer context to use
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    OpenGL compare mode
		*/
		[[nodiscard]] static GLint getOpenGLCompareMode([[maybe_unused]] const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
				case Renderer::FilterMode::ANISOTROPIC:
					return GL_NONE;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return GL_COMPARE_REF_TO_TEXTURE;

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "OpenGL filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureAddressMode                          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureAddressMode" to OpenGL texture address mode
		*
		*  @param[in] textureAddressMode
		*    "Renderer::TextureAddressMode" to map
		*
		*  @return
		*    OpenGL texture address mode
		*/
		[[nodiscard]] static GLint getOpenGLTextureAddressMode(Renderer::TextureAddressMode textureAddressMode)
		{
			static constexpr GLint MAPPING[] =
			{
				GL_REPEAT,			// Renderer::TextureAddressMode::WRAP
				GL_MIRRORED_REPEAT,	// Renderer::TextureAddressMode::MIRROR
				GL_CLAMP_TO_EDGE,	// Renderer::TextureAddressMode::CLAMP
				GL_CLAMP_TO_BORDER,	// Renderer::TextureAddressMode::BORDER
				GL_MIRRORED_REPEAT	// Renderer::TextureAddressMode::MIRROR_ONCE	// TODO(co) OpenGL equivalent? GL_ATI_texture_mirror_once ?
			};
			return MAPPING[static_cast<int>(textureAddressMode) - 1];	// Lookout! The "Renderer::TextureAddressMode"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::ComparisonFunc                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::ComparisonFunc" to OpenGL comparison function
		*
		*  @param[in] comparisonFunc
		*    "Renderer::ComparisonFunc" to map
		*
		*  @return
		*    OpenGL comparison function
		*/
		[[nodiscard]] static GLenum getOpenGLComparisonFunc(Renderer::ComparisonFunc comparisonFunc)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_NEVER,		// Renderer::ComparisonFunc::NEVER
				GL_LESS,		// Renderer::ComparisonFunc::LESS
				GL_EQUAL,		// Renderer::ComparisonFunc::EQUAL
				GL_LEQUAL,		// Renderer::ComparisonFunc::LESS_EQUAL
				GL_GREATER,		// Renderer::ComparisonFunc::GREATER
				GL_NOTEQUAL,	// Renderer::ComparisonFunc::NOT_EQUAL
				GL_GEQUAL,		// Renderer::ComparisonFunc::GREATER_EQUAL
				GL_ALWAYS		// Renderer::ComparisonFunc::ALWAYS
			};
			return MAPPING[static_cast<int>(comparisonFunc) - 1];	// Lookout! The "Renderer::ComparisonFunc"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::VertexAttributeFormat                       ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::VertexAttributeFormat" to OpenGL size (number of elements)
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to map
		*
		*  @return
		*    OpenGL size
		*/
		[[nodiscard]] static GLint getOpenGLSize(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLint MAPPING[] =
			{
				1,	// Renderer::VertexAttributeFormat::FLOAT_1
				2,	// Renderer::VertexAttributeFormat::FLOAT_2
				3,	// Renderer::VertexAttributeFormat::FLOAT_3
				4,	// Renderer::VertexAttributeFormat::FLOAT_4
				4,	// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				4,	// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				2,	// Renderer::VertexAttributeFormat::SHORT_2
				4,	// Renderer::VertexAttributeFormat::SHORT_4
				1	// Renderer::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    "Renderer::VertexAttributeFormat" to OpenGL type
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_FLOAT,			// Renderer::VertexAttributeFormat::FLOAT_1
				GL_FLOAT,			// Renderer::VertexAttributeFormat::FLOAT_2
				GL_FLOAT,			// Renderer::VertexAttributeFormat::FLOAT_3
				GL_FLOAT,			// Renderer::VertexAttributeFormat::FLOAT_4
				GL_UNSIGNED_BYTE,	// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				GL_UNSIGNED_BYTE,	// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				GL_SHORT,			// Renderer::VertexAttributeFormat::SHORT_2
				GL_SHORT,			// Renderer::VertexAttributeFormat::SHORT_4
				GL_UNSIGNED_INT		// Renderer::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    Return whether or not "Renderer::VertexAttributeFormat" is a normalized format
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to check
		*
		*  @return
		*    "GL_TRUE" if the format is normalized, else "GL_FALSE"
		*/
		[[nodiscard]] static GLboolean isOpenGLVertexAttributeFormatNormalized(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLboolean MAPPING[] =
			{
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_1
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_2
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_3
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_4
				GL_TRUE,	// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				GL_FALSE,	// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				GL_FALSE,	// Renderer::VertexAttributeFormat::SHORT_2
				GL_FALSE,	// Renderer::VertexAttributeFormat::SHORT_4
				GL_FALSE	// Renderer::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    Return whether or not "Renderer::VertexAttributeFormat" is an integer format
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to check
		*
		*  @return
		*    "GL_TRUE" if the format is integer, else "GL_FALSE"
		*/
		[[nodiscard]] static GLboolean isOpenGLVertexAttributeFormatInteger(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLboolean MAPPING[] =
			{
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_1
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_2
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_3
				GL_FALSE,	// Renderer::VertexAttributeFormat::FLOAT_4
				GL_FALSE,	// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				GL_TRUE,	// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				GL_TRUE,	// Renderer::VertexAttributeFormat::SHORT_2
				GL_TRUE,	// Renderer::VertexAttributeFormat::SHORT_4
				GL_TRUE		// Renderer::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		//[-------------------------------------------------------]
		//[ Renderer::IndexBufferFormat                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::IndexBufferFormat" to OpenGL type
		*
		*  @param[in] indexBufferFormat
		*    "Renderer::IndexBufferFormat" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Renderer::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_UNSIGNED_BYTE,	// Renderer::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API)
				GL_UNSIGNED_SHORT,	// Renderer::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				GL_UNSIGNED_INT		// Renderer::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureFormat                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureFormat" to OpenGL internal format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    OpenGL internal format
		*/
		[[nodiscard]] static GLuint getOpenGLInternalFormat(Renderer::TextureFormat::Enum textureFormat)
		{
			static constexpr GLuint MAPPING[] =
			{
				GL_R8,										// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_RGB8,									// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_RGBA8,									// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_SRGB8_ALPHA8,							// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_RGBA8,									// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_R11F_G11F_B10F_EXT,						// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "GL_EXT_packed_float" OpenGL extension
				GL_RGBA16F_ARB,								// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_RGBA32F_ARB,								// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha),
				GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,			// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,		// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,			// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,		// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,			// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,		// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_LUMINANCE_LATC1_EXT,			// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,	// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				0,											// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in OpenGL
				GL_R16,										// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_R32UI,									// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_R32F,									// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_DEPTH_COMPONENT32F,						// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_RG16_SNORM,								// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_RG16F,									// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0											// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Renderer::TextureFormat" to OpenGL format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    OpenGL format
		*/
		[[nodiscard]] static GLuint getOpenGLFormat(Renderer::TextureFormat::Enum textureFormat)
		{
			static constexpr GLuint MAPPING[] =
			{
				GL_RED,										// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_RGB,										// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_RGBA,									// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_RGBA,									// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_BGRA,									// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_RGB,										// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "GL_EXT_packed_float" OpenGL extension
				GL_RGBA,									// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_RGBA,									// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,			// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,		// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,			// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,		// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,			// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,		// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_LUMINANCE_LATC1_EXT,			// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,	// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				0,											// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in OpenGL
				GL_RED,										// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_RED_INTEGER,								// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_RED,										// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_DEPTH_COMPONENT,							// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_RG,										// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_RG,										// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0											// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Renderer::TextureFormat" to OpenGL type
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Renderer::TextureFormat::Enum textureFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_UNSIGNED_BYTE,						// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_UNSIGNED_BYTE,						// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_UNSIGNED_BYTE,						// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_UNSIGNED_BYTE,						// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_UNSIGNED_BYTE,						// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_UNSIGNED_INT_10F_11F_11F_REV_EXT,	// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "GL_EXT_packed_float" OpenGL extension
				GL_HALF_FLOAT_ARB,						// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_FLOAT,								// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				0,										// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				0,										// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				0,										// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				0,										// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				0,										// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				0,										// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				0,										// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				0,										// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				0,										// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in OpenGL
				GL_UNSIGNED_SHORT,						// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_UNSIGNED_INT,						// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_FLOAT,								// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_FLOAT,								// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_BYTE,								// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_FLOAT,								// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0										// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		//[-------------------------------------------------------]
		//[ Renderer::PrimitiveTopology                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::PrimitiveTopology" to OpenGL type
		*
		*  @param[in] primitiveTopology
		*    "Renderer::PrimitiveTopology" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Renderer::PrimitiveTopology primitiveTopology)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_POINTS,			// Renderer::PrimitiveTopology::POINT_LIST
				GL_LINES,			// Renderer::PrimitiveTopology::LINE_LIST
				GL_LINE_STRIP,		// Renderer::PrimitiveTopology::LINE_STRIP
				GL_TRIANGLES,		// Renderer::PrimitiveTopology::TRIANGLE_LIST
				GL_TRIANGLE_STRIP	// Renderer::PrimitiveTopology::TRIANGLE_STRIP
			};
			return MAPPING[static_cast<int>(primitiveTopology) - 1];	// Lookout! The "Renderer::PrimitiveTopology"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::MapType                                     ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::MapType" to OpenGL type
		*
		*  @param[in] mapType
		*    "Renderer::MapType" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLMapType(Renderer::MapType mapType)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_READ_ONLY,	// Renderer::MapType::READ
				GL_WRITE_ONLY,	// Renderer::MapType::WRITE
				GL_READ_WRITE,	// Renderer::MapType::READ_WRITE
				GL_WRITE_ONLY,	// Renderer::MapType::WRITE_DISCARD
				GL_WRITE_ONLY	// Renderer::MapType::WRITE_NO_OVERWRITE
			};
			return MAPPING[static_cast<int>(mapType) - 1];	// Lookout! The "Renderer::MapType"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::MapType                                     ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::Blend" to OpenGL type
		*
		*  @param[in] blend
		*    "Renderer::Blend" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLBlendType(Renderer::Blend blend)
		{
			if (blend <= Renderer::Blend::SRC_ALPHA_SAT)
			{
				static constexpr GLenum MAPPING[] =
				{
					GL_ZERO,				// Renderer::Blend::ZERO
					GL_ONE,					// Renderer::Blend::ONE
					GL_SRC_COLOR,			// Renderer::Blend::SRC_COLOR
					GL_ONE_MINUS_SRC_COLOR,	// Renderer::Blend::INV_SRC_COLOR
					GL_SRC_ALPHA,			// Renderer::Blend::SRC_ALPHA
					GL_ONE_MINUS_SRC_ALPHA,	// Renderer::Blend::INV_SRC_ALPHA
					GL_DST_ALPHA,			// Renderer::Blend::DEST_ALPHA
					GL_ONE_MINUS_DST_ALPHA,	// Renderer::Blend::INV_DEST_ALPHA
					GL_DST_COLOR,			// Renderer::Blend::DEST_COLOR
					GL_ONE_MINUS_DST_COLOR,	// Renderer::Blend::INV_DEST_COLOR
					GL_SRC_ALPHA_SATURATE	// Renderer::Blend::SRC_ALPHA_SAT
				};
				return MAPPING[static_cast<int>(blend) - static_cast<int>(Renderer::Blend::ZERO)];
			}
			else
			{
				static constexpr GLenum MAPPING[] =
				{
					GL_SRC_COLOR,				// Renderer::Blend::BLEND_FACTOR		TODO(co) Mapping "Renderer::Blend::BLEND_FACTOR" to OpenGL possible?
					GL_ONE_MINUS_SRC_COLOR,		// Renderer::Blend::INV_BLEND_FACTOR	TODO(co) Mapping "Renderer::Blend::INV_BLEND_FACTOR" to OpenGL possible?
					GL_SRC1_COLOR,				// Renderer::Blend::SRC_1_COLOR
					GL_ONE_MINUS_SRC1_COLOR,	// Renderer::Blend::INV_SRC_1_COLOR
					GL_SRC1_ALPHA,				// Renderer::Blend::SRC_1_ALPHA
					GL_ONE_MINUS_SRC1_ALPHA,	// Renderer::Blend::INV_SRC_1_ALPHA
				};
				return MAPPING[static_cast<int>(blend) - static_cast<int>(Renderer::Blend::BLEND_FACTOR)];
			}
		}


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/ResourceGroup.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL resource group class
	*/
	class ResourceGroup final : public Renderer::IResourceGroup
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] rootParameterIndex
		*    The root parameter index number for binding
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(Renderer::IRenderer& renderer, const Renderer::RootSignature& rootSignature, uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates) :
			IResourceGroup(renderer),
			mRootParameterIndex(rootParameterIndex),
			mNumberOfResources(numberOfResources),
			mResources(RENDERER_MALLOC_TYPED(renderer.getContext(), Renderer::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr),
			mResourceIndexToUniformBlockBindingIndex(nullptr)
		{
			// Get the uniform block binding start index
			const Renderer::Context& context = renderer.getContext();
			uint32_t uniformBlockBindingIndex = 0;
			for (uint32_t currentRootParameterIndex = 0; currentRootParameterIndex < rootParameterIndex; ++currentRootParameterIndex)
			{
				const Renderer::RootParameter& rootParameter = rootSignature.parameters[currentRootParameterIndex];
				if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
				{
					RENDERER_ASSERT(renderer.getContext(), nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
					const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
					for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
					{
						if (Renderer::DescriptorRangeType::UBV == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex].rangeType)
						{
							++uniformBlockBindingIndex;
						}
					}
				}
			}

			// Process all resources and add our reference to the renderer resource
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Renderer::IResource* resource = *resources;
				RENDERER_ASSERT(renderer.getContext(), nullptr != resource, "Invalid OpenGL resource")
				mResources[resourceIndex] = resource;
				resource->addReference();

				// Uniform block binding index handling
				const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];
				if (Renderer::DescriptorRangeType::UBV == descriptorRange.rangeType)
				{
					if (nullptr == mResourceIndexToUniformBlockBindingIndex)
					{
						mResourceIndexToUniformBlockBindingIndex = RENDERER_MALLOC_TYPED(context, uint32_t, mNumberOfResources);
						memset(mResourceIndexToUniformBlockBindingIndex, 0, sizeof(uint32_t) * mNumberOfResources);
					}
					mResourceIndexToUniformBlockBindingIndex[resourceIndex] = uniformBlockBindingIndex;
					++uniformBlockBindingIndex;
				}
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RENDERER_MALLOC_TYPED(context, Renderer::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Renderer::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->addReference();
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ResourceGroup() override
		{
			// Remove our reference from the renderer resources
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mSamplerStates)
			{
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Renderer::ISamplerState* samplerState = mSamplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->releaseReference();
					}
				}
				RENDERER_FREE(context, mSamplerStates);
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
			{
				mResources[resourceIndex]->releaseReference();
			}
			RENDERER_FREE(context, mResources);
			RENDERER_FREE(context, mResourceIndexToUniformBlockBindingIndex);
		}

		/**
		*  @brief
		*    Return the number of resources this resource group groups together
		*
		*  @return
		*    The number of resources this resource group groups together
		*/
		[[nodiscard]] inline uint32_t getNumberOfResources() const
		{
			return mNumberOfResources;
		}

		/**
		*  @brief
		*    Return the renderer resources
		*
		*  @return
		*    The renderer resources, don't release or destroy the returned pointer
		*/
		[[nodiscard]] inline Renderer::IResource** getResources() const
		{
			return mResources;
		}

		/**
		*  @brief
		*    Return the sampler states
		*
		*  @return
		*    The sampler states, don't release or destroy the returned pointer
		*/
		[[nodiscard]] inline Renderer::ISamplerState** getSamplerState() const
		{
			return mSamplerStates;
		}

		/**
		*  @brief
		*    Return the resource index to uniform block binding index mapping
		*
		*  @return
		*    The resource index to uniform block binding index mapping, only valid for uniform buffer resources
		*/
		[[nodiscard]] inline uint32_t* getResourceIndexToUniformBlockBindingIndex() const
		{
			return mResourceIndexToUniformBlockBindingIndex;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ResourceGroup, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ResourceGroup(const ResourceGroup& source) = delete;
		ResourceGroup& operator =(const ResourceGroup& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t				  mRootParameterIndex;						///< The root parameter index number for binding
		uint32_t				  mNumberOfResources;						///< Number of resources this resource group groups together
		Renderer::IResource**	  mResources;								///< Renderer resources, we keep a reference to it
		Renderer::ISamplerState** mSamplerStates;							///< Sampler states, we keep a reference to it
		uint32_t*				  mResourceIndexToUniformBlockBindingIndex;	///< Resource index to uniform block binding index mapping, only valid for uniform buffer resources


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/RootSignature.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL root signature ("pipeline layout" in Vulkan terminology) class
	*/
	class RootSignature final : public Renderer::IRootSignature
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(OpenGLRenderer& openGLRenderer, const Renderer::RootSignature& rootSignature) :
			IRootSignature(openGLRenderer),
			mRootSignature(rootSignature)
		{
			const Renderer::Context& context = openGLRenderer.getContext();

			{ // Copy the parameter data
				const uint32_t numberOfParameters = mRootSignature.numberOfParameters;
				if (numberOfParameters > 0)
				{
					mRootSignature.parameters = RENDERER_MALLOC_TYPED(context, Renderer::RootParameter, numberOfParameters);
					Renderer::RootParameter* destinationRootParameters = const_cast<Renderer::RootParameter*>(mRootSignature.parameters);
					memcpy(destinationRootParameters, rootSignature.parameters, sizeof(Renderer::RootParameter) * numberOfParameters);

					// Copy the descriptor table data
					for (uint32_t i = 0; i < numberOfParameters; ++i)
					{
						Renderer::RootParameter& destinationRootParameter = destinationRootParameters[i];
						const Renderer::RootParameter& sourceRootParameter = rootSignature.parameters[i];
						if (Renderer::RootParameterType::DESCRIPTOR_TABLE == destinationRootParameter.parameterType)
						{
							const uint32_t numberOfDescriptorRanges = destinationRootParameter.descriptorTable.numberOfDescriptorRanges;
							destinationRootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(RENDERER_MALLOC_TYPED(context, Renderer::DescriptorRange, numberOfDescriptorRanges));
							memcpy(reinterpret_cast<Renderer::DescriptorRange*>(destinationRootParameter.descriptorTable.descriptorRanges), reinterpret_cast<const Renderer::DescriptorRange*>(sourceRootParameter.descriptorTable.descriptorRanges), sizeof(Renderer::DescriptorRange) * numberOfDescriptorRanges);
						}
					}
				}
			}

			{ // Copy the static sampler data
				const uint32_t numberOfStaticSamplers = mRootSignature.numberOfStaticSamplers;
				if (numberOfStaticSamplers > 0)
				{
					mRootSignature.staticSamplers = RENDERER_MALLOC_TYPED(context, Renderer::StaticSampler, numberOfStaticSamplers);
					memcpy(const_cast<Renderer::StaticSampler*>(mRootSignature.staticSamplers), rootSignature.staticSamplers, sizeof(Renderer::StaticSampler) * numberOfStaticSamplers);
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RootSignature() override
		{
			// Destroy the root signature data
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mRootSignature.parameters)
			{
				for (uint32_t i = 0; i < mRootSignature.numberOfParameters; ++i)
				{
					const Renderer::RootParameter& rootParameter = mRootSignature.parameters[i];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_FREE(context, reinterpret_cast<Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges));
					}
				}
				RENDERER_FREE(context, const_cast<Renderer::RootParameter*>(mRootSignature.parameters));
			}
			RENDERER_FREE(context, const_cast<Renderer::StaticSampler*>(mRootSignature.staticSamplers));
		}

		/**
		*  @brief
		*    Return the root signature data
		*
		*  @return
		*    The root signature data
		*/
		[[nodiscard]] inline const Renderer::RootSignature& getRootSignature() const
		{
			return mRootSignature;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRootSignature methods       ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Renderer::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates = nullptr) override
		{
			// Sanity checks
			RENDERER_ASSERT(getRenderer().getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The OpenGL root parameter index is out-of-bounds")
			RENDERER_ASSERT(getRenderer().getContext(), numberOfResources > 0, "The number of OpenGL resources must not be zero")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr != resources, "The OpenGL resource pointers must be valid")

			// Create resource group
			return RENDERER_NEW(getRenderer().getContext(), ResourceGroup)(getRenderer(), mRootSignature, rootParameterIndex, numberOfResources, resources, samplerStates);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), RootSignature, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RootSignature(const RootSignature& source) = delete;
		RootSignature& operator =(const RootSignature& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::RootSignature mRootSignature;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/IndexBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL index buffer object (IBO, "element array buffer" in OpenGL terminology) interface
	*/
	class IndexBuffer : public Renderer::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBuffer() override
		{
			// Destroy the OpenGL element array buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffersARB(1, &mOpenGLElementArrayBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL element array buffer
		*
		*  @return
		*    The OpenGL element array buffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLElementArrayBuffer() const
		{
			return mOpenGLElementArrayBuffer;
		}

		/**
		*  @brief
		*    Return the OpenGL element array buffer data type
		*
		*  @return
		*    The OpenGL element array buffer data type
		*/
		[[nodiscard]] inline GLenum getOpenGLType() const
		{
			return mOpenGLType;
		}

		/**
		*  @brief
		*    Return the number of bytes of an index
		*
		*  @return
		*    The number of bytes of an index
		*/
		[[nodiscard]] inline uint32_t getIndexSizeInBytes() const
		{
			return mIndexSizeInBytes;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL element array buffer and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLElementArrayBuffer && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_BUFFER, mOpenGLElementArrayBuffer, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		inline IndexBuffer(OpenGLRenderer& openGLRenderer, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IIndexBuffer(openGLRenderer),
			mOpenGLElementArrayBuffer(0),
			mOpenGLType(Mapping::getOpenGLType(indexBufferFormat)),
			mIndexSizeInBytes(Renderer::IndexBufferFormat::getNumberOfBytesPerElement(indexBufferFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint   mOpenGLElementArrayBuffer;	///< OpenGL element array buffer, can be zero if no resource is allocated
		GLenum   mOpenGLType;				///< OpenGL element array buffer data type
		uint32_t mIndexSizeInBytes;			///< Number of bytes of an index


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBuffer(const IndexBuffer& source) = delete;
		IndexBuffer& operator =(const IndexBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/IndexBufferBind.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL index buffer object (IBO, "element array buffer" in OpenGL terminology) class, traditional bind version
	*/
	class IndexBufferBind final : public IndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBufferBind(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IndexBuffer(openGLRenderer, indexBufferFormat)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL element array buffer
				GLint openGLElementArrayBufferBackup = 0;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &openGLElementArrayBufferBackup);
			#endif

			// Create the OpenGL element array buffer
			glGenBuffersARB(1, &mOpenGLElementArrayBuffer);

			// Bind this OpenGL element array buffer and upload the data
			// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mOpenGLElementArrayBuffer);
			glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL element array buffer
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLElementArrayBufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBufferBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBufferBind(const IndexBufferBind& source) = delete;
		IndexBufferBind& operator =(const IndexBufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/IndexBufferDsa.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL index buffer object (IBO, "element array buffer" in OpenGL terminology) class, effective direct state access (DSA)
	*/
	class IndexBufferDsa final : public IndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBufferDsa(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IndexBuffer(openGLRenderer, indexBufferFormat)
		{
			if (openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Create the OpenGL element array buffer
				glCreateBuffers(1, &mOpenGLElementArrayBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferData(mOpenGLElementArrayBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
			else
			{
				// Create the OpenGL element array buffer
				glGenBuffersARB(1, &mOpenGLElementArrayBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferDataEXT(mOpenGLElementArrayBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBufferDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBufferDsa(const IndexBufferDsa& source) = delete;
		IndexBufferDsa& operator =(const IndexBufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL vertex buffer object (VBO, "array buffer" in OpenGL terminology) interface
	*/
	class VertexBuffer : public Renderer::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBuffer() override
		{
			// Destroy the OpenGL array buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffersARB(1, &mOpenGLArrayBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL array buffer
		*
		*  @return
		*    The OpenGL array buffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLArrayBuffer() const
		{
			return mOpenGLArrayBuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL array buffer and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLArrayBuffer && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_BUFFER, mOpenGLArrayBuffer, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit VertexBuffer(OpenGLRenderer& openGLRenderer) :
			IVertexBuffer(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLArrayBuffer(0)
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLArrayBuffer;	///< OpenGL array buffer, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBuffer(const VertexBuffer& source) = delete;
		VertexBuffer& operator =(const VertexBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexBufferBind.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL vertex buffer object (VBO, "array buffer" in OpenGL terminology) class, traditional bind version
	*/
	class VertexBufferBind final : public VertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBufferBind(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			VertexBuffer(openGLRenderer)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL array buffer
				GLint openGLArrayBufferBackup = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &openGLArrayBufferBackup);
			#endif

			// Create the OpenGL array buffer
			glGenBuffersARB(1, &mOpenGLArrayBuffer);

			// Bind this OpenGL array buffer and upload the data
			// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, mOpenGLArrayBuffer);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL array buffer
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLArrayBufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBufferBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBufferBind(const VertexBufferBind& source) = delete;
		VertexBufferBind& operator =(const VertexBufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexBufferDsa.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL vertex buffer object (VBO, "array buffer" in OpenGL terminology) class, effective direct state access (DSA)
	*/
	class VertexBufferDsa final : public VertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBufferDsa(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			VertexBuffer(openGLRenderer)
		{
			if (openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Create the OpenGL array buffer
				glCreateBuffers(1, &mOpenGLArrayBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferData(mOpenGLArrayBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
			else
			{
				// Create the OpenGL array buffer
				glGenBuffersARB(1, &mOpenGLArrayBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferDataEXT(mOpenGLArrayBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBufferDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBufferDsa(const VertexBufferDsa& source) = delete;
		VertexBufferDsa& operator =(const VertexBufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexArray.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL vertex array interface
	*/
	class VertexArray : public Renderer::IVertexArray
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Internal resource type
		*/
		class InternalResourceType
		{
			public:
			enum Enum
			{
				NO_VAO = 0,	///< No vertex array object
				VAO    = 1	///< Vertex array object
			};
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexArray() override
		{
			// Release the index buffer reference
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->releaseReference();
			}

			// Free the unique compact vertex array ID
			static_cast<OpenGLRenderer&>(getRenderer()).VertexArrayMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the used index buffer
		*
		*  @return
		*    The used index buffer, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IndexBuffer* getIndexBuffer() const
		{
			return mIndexBuffer;
		}

		/**
		*  @brief
		*    Return the internal resource type
		*
		*  @return
		*    The internal resource type
		*/
		[[nodiscard]] inline InternalResourceType::Enum getInternalResourceType() const
		{
			return mInternalResourceType;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexArray, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] internalResourceType
		*    Internal resource type
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		inline VertexArray(OpenGLRenderer& openGLRenderer, IndexBuffer* indexBuffer, InternalResourceType::Enum internalResourceType, uint16_t id) :
			IVertexArray(openGLRenderer, id),
			mIndexBuffer(indexBuffer),
			mInternalResourceType(internalResourceType)
		{
			// Add a reference to the given index buffer
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->addReference();
			}
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArray(const VertexArray& source) = delete;
		VertexArray& operator =(const VertexArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IndexBuffer*			   mIndexBuffer;			///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		InternalResourceType::Enum mInternalResourceType;	///< Internal resource type


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexArrayNoVao.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL vertex array class, traditional version
	*/
	class VertexArrayNoVao final : public VertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		VertexArrayNoVao(OpenGLRenderer& openGLRenderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			VertexArray(openGLRenderer, indexBuffer, InternalResourceType::NO_VAO, id),
			mNumberOfAttributes(vertexAttributes.numberOfAttributes),
			mAttributes(mNumberOfAttributes ? RENDERER_MALLOC_TYPED(openGLRenderer.getContext(), Renderer::VertexAttribute, mNumberOfAttributes) : nullptr),
			mNumberOfVertexBuffers(numberOfVertexBuffers),
			mVertexBuffers(numberOfVertexBuffers ? RENDERER_MALLOC_TYPED(openGLRenderer.getContext(), Renderer::VertexArrayVertexBuffer, numberOfVertexBuffers) : nullptr),
			mIsGL_ARB_instanced_arrays(openGLRenderer.getExtensions().isGL_ARB_instanced_arrays())
		{
			// Copy over the data
			if (nullptr != mAttributes)
			{
				memcpy(mAttributes, vertexAttributes.attributes, sizeof(Renderer::VertexAttribute) * mNumberOfAttributes);
			}
			if (nullptr != mVertexBuffers)
			{
				memcpy(mVertexBuffers, vertexBuffers, sizeof(Renderer::VertexArrayVertexBuffer) * mNumberOfVertexBuffers);
			}

			// Add a reference to the used vertex buffers
			const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = mVertexBuffers + mNumberOfVertexBuffers;
			for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = mVertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
			{
				vertexBuffer->vertexBuffer->addReference();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexArrayNoVao() override
		{
			// Destroy the vertex array attributes
			const Renderer::Context& context = getRenderer().getContext();
			RENDERER_FREE(context, mAttributes);

			// Destroy the vertex array vertex buffers
			if (nullptr != mVertexBuffers)
			{
				// Release the reference to the used vertex buffers
				const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = mVertexBuffers + mNumberOfVertexBuffers;
				for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = mVertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					vertexBuffer->vertexBuffer->releaseReference();
				}

				// Cleanup
				RENDERER_FREE(context, mVertexBuffers);
			}
		}

		/**
		*  @brief
		*    Enable OpenGL vertex attribute arrays
		*/
		void enableOpenGLVertexAttribArrays()
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL array buffer
				// -> Using "GL_EXT_direct_state_access" this would not help in here because "glVertexAttribPointerARB" is not specified there :/
				GLint openGLArrayBufferBackup = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &openGLArrayBufferBackup);
			#endif

			// Loop through all attributes
			// -> We're using "glBindAttribLocation()" when linking the program so we have known attribute locations (the vertex array can't know about the program)
			GLuint attributeLocation = 0;
			const Renderer::VertexAttribute* attributeEnd = mAttributes + mNumberOfAttributes;
			for (const Renderer::VertexAttribute* attribute = mAttributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Set the OpenGL vertex attribute pointer
				// TODO(co) Add security check: Is the given resource one of the currently used renderer?
				const Renderer::VertexArrayVertexBuffer& vertexArrayVertexBuffer = mVertexBuffers[attribute->inputSlot];
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<VertexBuffer*>(vertexArrayVertexBuffer.vertexBuffer)->getOpenGLArrayBuffer());
				if (Mapping::isOpenGLVertexAttributeFormatInteger(attribute->vertexAttributeFormat))
				{
					glVertexAttribIPointer(attributeLocation,
										   Mapping::getOpenGLSize(attribute->vertexAttributeFormat),
										   Mapping::getOpenGLType(attribute->vertexAttributeFormat),
										   static_cast<GLsizei>(attribute->strideInBytes),
										   reinterpret_cast<void*>(static_cast<uintptr_t>(attribute->alignedByteOffset)));
				}
				else
				{
					glVertexAttribPointerARB(attributeLocation,
											 Mapping::getOpenGLSize(attribute->vertexAttributeFormat),
											 Mapping::getOpenGLType(attribute->vertexAttributeFormat),
											 static_cast<GLboolean>(Mapping::isOpenGLVertexAttributeFormatNormalized(attribute->vertexAttributeFormat)),
											 static_cast<GLsizei>(attribute->strideInBytes),
											 reinterpret_cast<void*>(static_cast<uintptr_t>(attribute->alignedByteOffset)));
				}

				// Per-instance instead of per-vertex requires "GL_ARB_instanced_arrays"
				if (attribute->instancesPerElement > 0 && mIsGL_ARB_instanced_arrays)
				{
					glVertexAttribDivisorARB(attributeLocation, attribute->instancesPerElement);
				}

				// Enable OpenGL vertex attribute array
				glEnableVertexAttribArrayARB(attributeLocation);
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL array buffer
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLArrayBufferBackup));
			#endif

			// Set the used index buffer
			// -> In case of no index buffer we don't bind buffer 0, there's not really a point in it
			const IndexBuffer* indexBuffer = getIndexBuffer();
			if (nullptr != indexBuffer)
			{
				// Bind OpenGL element array buffer
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexBuffer->getOpenGLElementArrayBuffer());
			}
		}

		/**
		*  @brief
		*    Disable OpenGL vertex attribute arrays
		*/
		void disableOpenGLVertexAttribArrays()
		{
			// No previous bound OpenGL element array buffer restore, there's not really a point in it

			// Loop through all attributes
			// -> We're using "glBindAttribLocation()" when linking the program so we have known attribute locations (the vertex array can't know about the program)
			GLuint attributeLocation = 0;
			const Renderer::VertexAttribute* attributeEnd = mAttributes + mNumberOfAttributes;
			for (const Renderer::VertexAttribute* attribute = mAttributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Disable OpenGL vertex attribute array
				glDisableVertexAttribArrayARB(attributeLocation);

				// Per-instance instead of per-vertex requires "GL_ARB_instanced_arrays"
				if (attribute->instancesPerElement > 0 && mIsGL_ARB_instanced_arrays)
				{
					glVertexAttribDivisorARB(attributeLocation, 0);
				}
			}
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArrayNoVao(const VertexArrayNoVao& source) = delete;
		VertexArrayNoVao& operator =(const VertexArrayNoVao& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t						   mNumberOfAttributes;			///< Number of attributes (position, color, texture coordinate, normal...), having zero attributes is valid
		Renderer::VertexAttribute*		   mAttributes;					///< At least "numberOfAttributes" instances of vertex attributes, can be a null pointer in case there are zero attributes
		uint32_t						   mNumberOfVertexBuffers;		///< Number of vertex buffers, having zero vertex buffers is valid
		Renderer::VertexArrayVertexBuffer* mVertexBuffers;				///< At least mNumberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		bool							   mIsGL_ARB_instanced_arrays;	///< Is the "GL_ARB_instanced_arrays"-extension supported?


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexArrayVao.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL vertex array interface, effective vertex array object (VAO)
	*/
	class VertexArrayVao : public VertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexArrayVao() override
		{
			// Destroy the OpenGL vertex array
			// -> Silently ignores 0's and names that do not correspond to existing vertex array objects
			glDeleteVertexArrays(1, &mOpenGLVertexArray);

			// Release the reference to the used vertex buffers
			if (nullptr != mVertexBuffers)
			{
				// Release references
				VertexBuffer** vertexBuffersEnd = mVertexBuffers + mNumberOfVertexBuffers;
				for (VertexBuffer** vertexBuffer = mVertexBuffers; vertexBuffer < vertexBuffersEnd; ++vertexBuffer)
				{
					(*vertexBuffer)->releaseReference();
				}

				// Cleanup
				RENDERER_FREE(getRenderer().getContext(), mVertexBuffers);
			}
		}

		/**
		*  @brief
		*    Return the OpenGL vertex array
		*
		*  @return
		*    The OpenGL vertex array, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLVertexArray() const
		{
			return mOpenGLVertexArray;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL vertex array buffer and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLVertexArray && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_VERTEX_ARRAY, mOpenGLVertexArray, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		VertexArrayVao(OpenGLRenderer& openGLRenderer, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			VertexArray(openGLRenderer, indexBuffer, InternalResourceType::VAO, id),
			mOpenGLVertexArray(0),
			mNumberOfVertexBuffers(numberOfVertexBuffers),
			mVertexBuffers(nullptr)
		{
			// Add a reference to the used vertex buffers
			if (numberOfVertexBuffers > 0)
			{
				mVertexBuffers = RENDERER_MALLOC_TYPED(openGLRenderer.getContext(), VertexBuffer*, numberOfVertexBuffers);

				// Loop through all vertex buffers
				VertexBuffer** currentVertexBuffers = mVertexBuffers;
				const Renderer::VertexArrayVertexBuffer* vertexBuffersEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBuffersEnd; ++vertexBuffer, ++currentVertexBuffers)
				{
					// TODO(co) Add security check: Is the given resource one of the currently used renderer?
					*currentVertexBuffers = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
					(*currentVertexBuffers)->addReference();
				}
			}
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint		   mOpenGLVertexArray;		///< OpenGL vertex array ("container" object, not shared between OpenGL contexts), can be zero if no resource is allocated
		uint32_t	   mNumberOfVertexBuffers;	///< Number of vertex buffers
		VertexBuffer** mVertexBuffers;			///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArrayVao(const VertexArrayVao& source) = delete;
		VertexArrayVao& operator =(const VertexArrayVao& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexArrayVaoBind.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL vertex array class, effective vertex array object (VAO), traditional bind version
	*/
	class VertexArrayVaoBind final : public VertexArrayVao
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		VertexArrayVaoBind(OpenGLRenderer& openGLRenderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			VertexArrayVao(openGLRenderer, numberOfVertexBuffers, vertexBuffers, indexBuffer, id)
		{
			// Vertex buffer reference handling is done within the base class "VertexArrayVao"

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL array buffer
				GLint openGLArrayBufferBackup = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &openGLArrayBufferBackup);

				// Backup the currently bound OpenGL element array buffer
				GLint openGLElementArrayBufferBackup = 0;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &openGLElementArrayBufferBackup);

				// Backup the currently bound OpenGL vertex array
				GLint openGLVertexArrayBackup = 0;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &openGLVertexArrayBackup);
			#endif

			// Create the OpenGL vertex array
			glGenVertexArrays(1, &mOpenGLVertexArray);

			// Bind this OpenGL vertex array
			glBindVertexArray(mOpenGLVertexArray);

			// Loop through all attributes
			// -> We're using "glBindAttribLocation()" when linking the program so we have known attribute locations (the vertex array can't know about the program)
			GLuint attributeLocation = 0;
			const Renderer::VertexAttribute* attributeEnd = vertexAttributes.attributes + vertexAttributes.numberOfAttributes;
			for (const Renderer::VertexAttribute* attribute = vertexAttributes.attributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Set the OpenGL vertex attribute pointer
				// TODO(co) Add security check: Is the given resource one of the currently used renderer?
				const Renderer::VertexArrayVertexBuffer& vertexArrayVertexBuffer = vertexBuffers[attribute->inputSlot];
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<VertexBuffer*>(vertexArrayVertexBuffer.vertexBuffer)->getOpenGLArrayBuffer());
				if (Mapping::isOpenGLVertexAttributeFormatInteger(attribute->vertexAttributeFormat))
				{
					glVertexAttribIPointer(attributeLocation,
										   Mapping::getOpenGLSize(attribute->vertexAttributeFormat),
										   Mapping::getOpenGLType(attribute->vertexAttributeFormat),
										   static_cast<GLsizei>(attribute->strideInBytes),
										   reinterpret_cast<void*>(static_cast<uintptr_t>(attribute->alignedByteOffset)));
				}
				else
				{
					glVertexAttribPointerARB(attributeLocation,
											 Mapping::getOpenGLSize(attribute->vertexAttributeFormat),
											 Mapping::getOpenGLType(attribute->vertexAttributeFormat),
											 static_cast<GLboolean>(Mapping::isOpenGLVertexAttributeFormatNormalized(attribute->vertexAttributeFormat)),
											 static_cast<GLsizei>(attribute->strideInBytes),
											 reinterpret_cast<void*>(static_cast<uintptr_t>(attribute->alignedByteOffset)));
				}

				// Per-instance instead of per-vertex requires "GL_ARB_instanced_arrays"
				if (attribute->instancesPerElement > 0 && openGLRenderer.getExtensions().isGL_ARB_instanced_arrays())
				{
					glVertexAttribDivisorARB(attributeLocation, attribute->instancesPerElement);
				}

				// Enable OpenGL vertex attribute array
				glEnableVertexAttribArrayARB(attributeLocation);
			}

			// Check the used index buffer
			// -> In case of no index buffer we don't bind buffer 0, there's not really a point in it
			if (nullptr != indexBuffer)
			{
				// Bind OpenGL element array buffer
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexBuffer->getOpenGLElementArrayBuffer());
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL vertex array
				glBindVertexArray(static_cast<GLuint>(openGLVertexArrayBackup));

				// Be polite and restore the previous bound OpenGL element array buffer
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLElementArrayBufferBackup));

				// Be polite and restore the previous bound OpenGL array buffer
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLArrayBufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexArrayVaoBind() override
		{
			// Vertex buffer reference handling is done within the base class "VertexArrayVao"
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArrayVaoBind(const VertexArrayVaoBind& source) = delete;
		VertexArrayVaoBind& operator =(const VertexArrayVaoBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/VertexArrayVaoDsa.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL vertex array class, effective vertex array object (VAO), effective direct state access (DSA)
	*/
	class VertexArrayVaoDsa final : public VertexArrayVao
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		VertexArrayVaoDsa(OpenGLRenderer& openGLRenderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			VertexArrayVao(openGLRenderer, numberOfVertexBuffers, vertexBuffers, indexBuffer, id)
		{
			// Vertex buffer reference handling is done within the base class "VertexArrayVao"
			const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();
			if (isArbDsa)
			{
				// Create the OpenGL vertex array
				glCreateVertexArrays(1, &mOpenGLVertexArray);
			}
			else
			{
				// Create the OpenGL vertex array
				glGenVertexArrays(1, &mOpenGLVertexArray);
			}

			// Loop through all attributes
			// -> We're using "glBindAttribLocation()" when linking the program so we have known attribute locations (the vertex array can't know about the program)
			GLuint attributeLocation = 0;
			const Renderer::VertexAttribute* attributeEnd = vertexAttributes.attributes + vertexAttributes.numberOfAttributes;
			for (const Renderer::VertexAttribute* attribute = vertexAttributes.attributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Set the OpenGL vertex attribute pointer
				// TODO(co) Add security check: Is the given resource one of the currently used renderer?
				const Renderer::VertexArrayVertexBuffer& vertexArrayVertexBuffer = vertexBuffers[attribute->inputSlot];

				if (isArbDsa)
				{
					// Enable attribute
					glEnableVertexArrayAttrib(mOpenGLVertexArray, attributeLocation);

					// Set up the format for my attribute
					if (Mapping::isOpenGLVertexAttributeFormatInteger(attribute->vertexAttributeFormat))
					{
						glVertexArrayAttribIFormat(mOpenGLVertexArray, attributeLocation, Mapping::getOpenGLSize(attribute->vertexAttributeFormat), Mapping::getOpenGLType(attribute->vertexAttributeFormat), static_cast<GLuint>(attribute->alignedByteOffset));
					}
					else
					{
						glVertexArrayAttribFormat(mOpenGLVertexArray, attributeLocation, Mapping::getOpenGLSize(attribute->vertexAttributeFormat), Mapping::getOpenGLType(attribute->vertexAttributeFormat), static_cast<GLboolean>(Mapping::isOpenGLVertexAttributeFormatNormalized(attribute->vertexAttributeFormat)), static_cast<GLuint>(attribute->alignedByteOffset));
					}
					glVertexArrayAttribBinding(mOpenGLVertexArray, attributeLocation, attributeLocation);

					// Bind vertex buffer to buffer point
					glVertexArrayVertexBuffer(mOpenGLVertexArray,
											  attributeLocation,
											  static_cast<VertexBuffer*>(vertexArrayVertexBuffer.vertexBuffer)->getOpenGLArrayBuffer(),
											  0,	// No offset to the first element of the buffer
											  static_cast<GLsizei>(attribute->strideInBytes));

					// Per-instance instead of per-vertex requires "GL_ARB_instanced_arrays"
					if (attribute->instancesPerElement > 0 && openGLRenderer.getExtensions().isGL_ARB_instanced_arrays())
					{
						glVertexArrayBindingDivisor(mOpenGLVertexArray, attributeLocation, attribute->instancesPerElement);
					}
				}
				else
				{
					glVertexArrayVertexAttribOffsetEXT(mOpenGLVertexArray,
													  static_cast<VertexBuffer*>(vertexArrayVertexBuffer.vertexBuffer)->getOpenGLArrayBuffer(),
													  attributeLocation, Mapping::getOpenGLSize(attribute->vertexAttributeFormat),
													  Mapping::getOpenGLType(attribute->vertexAttributeFormat),
													  static_cast<GLboolean>(Mapping::isOpenGLVertexAttributeFormatNormalized(attribute->vertexAttributeFormat)),
													  static_cast<GLsizei>(attribute->strideInBytes),
													  static_cast<GLintptr>(attribute->alignedByteOffset));

					// Per-instance instead of per-vertex requires "GL_ARB_instanced_arrays"
					if (attribute->instancesPerElement > 0 && openGLRenderer.getExtensions().isGL_ARB_instanced_arrays())
					{
						// Sadly, DSA has no support for "GL_ARB_instanced_arrays", so, we have to use the bind way
						// -> Keep the bind-horror as local as possible

						#ifdef RENDERER_OPENGL_STATE_CLEANUP
							// Backup the currently bound OpenGL vertex array
							GLint openGLVertexArrayBackup = 0;
							glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &openGLVertexArrayBackup);
						#endif

						// Bind this OpenGL vertex array
						glBindVertexArray(mOpenGLVertexArray);

						// Set divisor
						if (attribute->instancesPerElement > 0)
						{
							glVertexAttribDivisorARB(attributeLocation, attribute->instancesPerElement);
						}

						#ifdef RENDERER_OPENGL_STATE_CLEANUP
							// Be polite and restore the previous bound OpenGL vertex array
							glBindVertexArray(static_cast<GLuint>(openGLVertexArrayBackup));
						#endif
					}

					// Enable OpenGL vertex attribute array
					glEnableVertexArrayAttribEXT(mOpenGLVertexArray, attributeLocation);
				}
			}

			// Check the used index buffer
			// -> In case of no index buffer we don't bind buffer 0, there's not really a point in it
			if (nullptr != indexBuffer)
			{
				if (isArbDsa)
				{
					// Bind the index buffer
					glVertexArrayElementBuffer(mOpenGLVertexArray, indexBuffer->getOpenGLElementArrayBuffer());
				}
				else
				{
					// Sadly, EXT DSA has no support for element array buffer, so, we have to use the bind way
					// -> Keep the bind-horror as local as possible

					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Backup the currently bound OpenGL vertex array
						GLint openGLVertexArrayBackup = 0;
						glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &openGLVertexArrayBackup);

						// Backup the currently bound OpenGL element array buffer
						GLint openGLElementArrayBufferBackup = 0;
						glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &openGLElementArrayBufferBackup);
					#endif

					// Bind this OpenGL vertex array
					glBindVertexArray(mOpenGLVertexArray);

					// Bind OpenGL element array buffer
					glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexBuffer->getOpenGLElementArrayBuffer());

					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL vertex array
						glBindVertexArray(static_cast<GLuint>(openGLVertexArrayBackup));

						// Be polite and restore the previous bound OpenGL element array buffer
						glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLElementArrayBufferBackup));
					#endif
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexArrayVaoDsa() override
		{
			// Vertex buffer reference handling is done within the base class "VertexArrayVao"
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArrayVaoDsa(const VertexArrayVaoDsa& source) = delete;
		VertexArrayVaoDsa& operator =(const VertexArrayVaoDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/TextureBuffer.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL texture buffer object (TBO) interface
	*/
	class TextureBuffer : public Renderer::ITextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureBuffer() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);

			// Destroy the OpenGL texture buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffersARB(1, &mOpenGLTextureBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL texture buffer instance
		*
		*  @return
		*    The OpenGL texture buffer instance, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTextureBuffer() const
		{
			return mOpenGLTextureBuffer;
		}

		/**
		*  @brief
		*    Return the OpenGL texture instance
		*
		*  @return
		*    The OpenGL texture instance, can be zero if no resource is allocated
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// "GL_KHR_debug"-extension available?
				if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					if (0 != mOpenGLTexture)
					{
						glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
					}
					if (0 != mOpenGLTextureBuffer)
					{
						glObjectLabel(GL_BUFFER, mOpenGLTextureBuffer, -1, name);
					}
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		inline TextureBuffer(OpenGLRenderer& openGLRenderer, Renderer::TextureFormat::Enum textureFormat) :
			ITextureBuffer(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLTextureBuffer(0),
			mOpenGLTexture(0),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLTextureBuffer;	///< OpenGL texture buffer, can be zero if no resource is allocated
		GLuint mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		GLuint mOpenGLInternalFormat;	///< OpenGL internal format


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/TextureBufferBind.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL texture buffer object (TBO) class, traditional bind version
	*/
	class TextureBufferBind final : public TextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBufferBind(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, Renderer::TextureFormat::Enum textureFormat) :
			TextureBuffer(openGLRenderer, textureFormat)
		{
			{ // Buffer part
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL texture buffer
					GLint openGLTextureBufferBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_ARB, &openGLTextureBufferBackup);
				#endif

				// Create the OpenGL texture buffer
				glGenBuffersARB(1, &mOpenGLTextureBuffer);

				// Bind this OpenGL texture buffer and upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glBindBufferARB(GL_TEXTURE_BUFFER_ARB, mOpenGLTextureBuffer);
				glBufferDataARB(GL_TEXTURE_BUFFER_ARB, static_cast<GLsizeiptrARB>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture buffer
					glBindBufferARB(GL_TEXTURE_BUFFER_ARB, static_cast<GLuint>(openGLTextureBufferBackup));
				#endif
			}

			{ // Texture part
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL texture
					GLint openGLTextureBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_ARB, &openGLTextureBackup);
				#endif

				// Create the OpenGL texture instance
				glGenTextures(1, &mOpenGLTexture);

				// Make this OpenGL texture instance to the currently used one
				glBindTexture(GL_TEXTURE_BUFFER_ARB, mOpenGLTexture);

				// Attaches the storage for the buffer object to the active buffer texture
				glTexBufferARB(GL_TEXTURE_BUFFER_ARB, mOpenGLInternalFormat, mOpenGLTextureBuffer);

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture
					glBindTexture(GL_TEXTURE_BUFFER_ARB, static_cast<GLuint>(openGLTextureBackup));
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureBufferBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBufferBind(const TextureBufferBind& source) = delete;
		TextureBufferBind& operator =(const TextureBufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/TextureBufferDsa.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL texture buffer object (TBO) class, effective direct state access (DSA)
	*/
	class TextureBufferDsa final : public TextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBufferDsa(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, Renderer::TextureFormat::Enum textureFormat) :
			TextureBuffer(openGLRenderer, textureFormat)
		{
			if (openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				{ // Buffer part
					// Create the OpenGL texture buffer
					glCreateBuffers(1, &mOpenGLTextureBuffer);

					// Upload the data
					// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
					glNamedBufferData(mOpenGLTextureBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
				}

				{ // Texture part
					// Create the OpenGL texture instance
					glCreateTextures(GL_TEXTURE_BUFFER_ARB, 1, &mOpenGLTexture);

					// Attach the storage for the buffer object to the buffer texture
					glTextureBuffer(mOpenGLTexture, mOpenGLInternalFormat, mOpenGLTextureBuffer);
				}
			}
			else
			{
				// Create the OpenGL texture buffer
				glGenBuffersARB(1, &mOpenGLTextureBuffer);

				// Create the OpenGL texture instance
				glGenTextures(1, &mOpenGLTexture);

				// Buffer part
				// -> Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferDataEXT(mOpenGLTextureBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

				{ // Texture part
					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Backup the currently bound OpenGL texture
						GLint openGLTextureBackup = 0;
						glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_ARB, &openGLTextureBackup);
					#endif

					// Make this OpenGL texture instance to the currently used one
					glBindTexture(GL_TEXTURE_BUFFER_ARB, mOpenGLTexture);

					// Attaches the storage for the buffer object to the active buffer texture
					// -> Sadly, there's no direct state access (DSA) function defined for this in "GL_EXT_direct_state_access"
					glTexBufferARB(GL_TEXTURE_BUFFER_ARB, mOpenGLInternalFormat, mOpenGLTextureBuffer);

					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL texture
						glBindTexture(GL_TEXTURE_BUFFER_ARB, static_cast<GLuint>(openGLTextureBackup));
					#endif
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureBufferDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBufferDsa(const TextureBufferDsa& source) = delete;
		TextureBufferDsa& operator =(const TextureBufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/StructuredBuffer.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL structured buffer object interface
	*/
	class StructuredBuffer : public Renderer::IStructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		virtual ~StructuredBuffer() override
		{
			// Destroy the OpenGL structured buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffersARB(1, &mOpenGLStructuredBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL structured buffer instance
		*
		*  @return
		*    The OpenGL structured buffer instance, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLStructuredBuffer() const
		{
			return mOpenGLStructuredBuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// "GL_KHR_debug"-extension available?
				if (0 != mOpenGLStructuredBuffer && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_BUFFER, mOpenGLStructuredBuffer, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), StructuredBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit StructuredBuffer(OpenGLRenderer& openGLRenderer) :
			IStructuredBuffer(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLStructuredBuffer(0)
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLStructuredBuffer;	///< OpenGL structured buffer, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit StructuredBuffer(const StructuredBuffer& source) = delete;
		StructuredBuffer& operator =(const StructuredBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/StructuredBufferBind.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL structured buffer object class, traditional bind version
	*/
	class StructuredBufferBind final : public StructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBufferBind(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes) :
			StructuredBuffer(openGLRenderer)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL structured buffer
				GLint openGLStructuredBufferBackup = 0;
				glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &openGLStructuredBufferBackup);
			#endif

			// Create the OpenGL structured buffer
			glGenBuffersARB(1, &mOpenGLStructuredBuffer);

			// Bind this OpenGL structured buffer and upload the data
			// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
			glBindBufferARB(GL_SHADER_STORAGE_BUFFER, mOpenGLStructuredBuffer);
			glBufferDataARB(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptrARB>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL structured buffer
				glBindBufferARB(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(openGLStructuredBufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~StructuredBufferBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit StructuredBufferBind(const StructuredBufferBind& source) = delete;
		StructuredBufferBind& operator =(const StructuredBufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/StructuredBufferDsa.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL structured buffer object class, effective direct state access (DSA)
	*/
	class StructuredBufferDsa final : public StructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBufferDsa(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes) :
			StructuredBuffer(openGLRenderer)
		{
			if (openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Create the OpenGL structured buffer
				glCreateBuffers(1, &mOpenGLStructuredBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferData(mOpenGLStructuredBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
			else
			{
				// Create the OpenGL structured buffer
				glGenBuffersARB(1, &mOpenGLStructuredBuffer);

				// Buffer part
				// -> Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferDataEXT(mOpenGLStructuredBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~StructuredBufferDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit StructuredBufferDsa(const StructuredBufferDsa& source) = delete;
		StructuredBufferDsa& operator =(const StructuredBufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/IndirectBuffer.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL indirect buffer object interface
	*/
	class IndirectBuffer : public Renderer::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{
			// Destroy the OpenGL indirect buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffersARB(1, &mOpenGLIndirectBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL indirect buffer
		*
		*  @return
		*    The OpenGL indirect buffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLIndirectBuffer() const
		{
			return mOpenGLIndirectBuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IIndirectBuffer methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const uint8_t* getEmulationData() const override
		{
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL indirect buffer and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLIndirectBuffer && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_BUFFER, mOpenGLIndirectBuffer, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndirectBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit IndirectBuffer(OpenGLRenderer& openGLRenderer) :
			IIndirectBuffer(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLIndirectBuffer(0)
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLIndirectBuffer;	///< OpenGL indirect buffer, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBuffer(const IndirectBuffer& source) = delete;
		IndirectBuffer& operator =(const IndirectBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/IndirectBufferBind.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL indirect buffer object class, traditional bind version
	*/
	class IndirectBufferBind final : public IndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBufferBind(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			IndirectBuffer(openGLRenderer)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL indirect buffer
				GLint openGLIndirectBufferBackup = 0;
				glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &openGLIndirectBufferBackup);
			#endif

			// Create the OpenGL indirect buffer
			glGenBuffersARB(1, &mOpenGLIndirectBuffer);

			// Bind this OpenGL indirect buffer and upload the data
			// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
			glBindBufferARB(GL_DRAW_INDIRECT_BUFFER, mOpenGLIndirectBuffer);
			glBufferDataARB(GL_DRAW_INDIRECT_BUFFER, static_cast<GLsizeiptrARB>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL indirect buffer
				glBindBufferARB(GL_DRAW_INDIRECT_BUFFER, static_cast<GLuint>(openGLIndirectBufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBufferBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBufferBind(const IndirectBufferBind& source) = delete;
		IndirectBufferBind& operator =(const IndirectBufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/IndirectBufferDsa.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL indirect buffer object class, effective direct state access (DSA)
	*/
	class IndirectBufferDsa final : public IndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBufferDsa(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			IndirectBuffer(openGLRenderer)
		{
			if (openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Create the OpenGL indirect buffer
				glCreateBuffers(1, &mOpenGLIndirectBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferData(mOpenGLIndirectBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
			else
			{
				// Create the OpenGL indirect buffer
				glGenBuffersARB(1, &mOpenGLIndirectBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferDataEXT(mOpenGLIndirectBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBufferDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBufferDsa(const IndirectBufferDsa& source) = delete;
		IndirectBufferDsa& operator =(const IndirectBufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/UniformBuffer.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
	*/
	class UniformBuffer : public Renderer::IUniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBuffer() override
		{
			// Destroy the OpenGL uniform buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffersARB(1, &mOpenGLUniformBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL uniform buffer instance
		*
		*  @return
		*    The OpenGL uniform buffer instance, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLUniformBuffer() const
		{
			return mOpenGLUniformBuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL uniform buffer and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLUniformBuffer && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_BUFFER, mOpenGLUniformBuffer, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), UniformBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit UniformBuffer(OpenGLRenderer& openGLRenderer) :
			IUniformBuffer(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLUniformBuffer(0)
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLUniformBuffer;	///< OpenGL uniform buffer, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBuffer(const UniformBuffer& source) = delete;
		UniformBuffer& operator =(const UniformBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/UniformBufferBind.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL uniform buffer object (UBO, "constant buffer" in Direct3D terminology) class, traditional bind version
	*/
	class UniformBufferBind final : public UniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBufferBind(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			UniformBuffer(openGLRenderer)
		{
			// TODO(co) Review OpenGL uniform buffer alignment topic

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL uniform buffer
				GLint openGLUniformBufferBackup = 0;
				glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &openGLUniformBufferBackup);
			#endif

			// Create the OpenGL uniform buffer
			glGenBuffersARB(1, &mOpenGLUniformBuffer);

			// Bind this OpenGL uniform buffer and upload the data
			// -> Usage: These constants directly map to GL_ARB_vertex_buffer_object and OpenGL ES 3 constants, do not change them
			glBindBufferARB(GL_UNIFORM_BUFFER, mOpenGLUniformBuffer);
			glBufferDataARB(GL_UNIFORM_BUFFER, static_cast<GLsizeiptrARB>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL uniform buffer
				glBindBufferARB(GL_UNIFORM_BUFFER, static_cast<GLuint>(openGLUniformBufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBufferBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBufferBind(const UniformBufferBind& source) = delete;
		UniformBufferBind& operator =(const UniformBufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/UniformBufferDsa.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL uniform buffer object (UBO, "constant buffer" in Direct3D terminology) class, effective direct state access (DSA)
	*/
	class UniformBufferDsa final : public UniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBufferDsa(OpenGLRenderer& openGLRenderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			UniformBuffer(openGLRenderer)
		{
			// TODO(co) Review OpenGL uniform buffer alignment topic

			if (openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Create the OpenGL uniform buffer
				glCreateBuffers(1, &mOpenGLUniformBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferData(mOpenGLUniformBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
			else
			{
				// Create the OpenGL uniform buffer
				glGenBuffersARB(1, &mOpenGLUniformBuffer);

				// Upload the data
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glNamedBufferDataEXT(mOpenGLUniformBuffer, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBufferDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBufferDsa(const UniformBufferDsa& source) = delete;
		UniformBufferDsa& operator =(const UniformBufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Buffer/BufferManager.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL buffer manager interface
	*/
	class BufferManager final : public Renderer::IBufferManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit BufferManager(OpenGLRenderer& openGLRenderer) :
			IBufferManager(openGLRenderer),
			mExtensions(&openGLRenderer.getExtensions())
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BufferManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IBufferManager methods       ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Renderer::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// "GL_ARB_vertex_buffer_object" required
			if (mExtensions->isGL_ARB_vertex_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), VertexBufferDsa)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), VertexBufferBind)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::IndexBufferFormat::Enum indexBufferFormat = Renderer::IndexBufferFormat::UNSIGNED_SHORT) override
		{
			// "GL_ARB_vertex_buffer_object" required
			if (mExtensions->isGL_ARB_vertex_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), IndexBufferDsa)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, indexBufferFormat);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), IndexBufferBind)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, indexBufferFormat);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IVertexArray* createVertexArray(const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, Renderer::IIndexBuffer* indexBuffer = nullptr) override
		{
			// Sanity checks
			#ifdef RENDERER_DEBUG
			{
				const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RENDERER_ASSERT(getRenderer().getContext(), &getRenderer() == &vertexBuffer->vertexBuffer->getRenderer(), "OpenGL error: The given vertex buffer resource is owned by another renderer instance")
				}
			}
			#endif
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == indexBuffer || &getRenderer() == &indexBuffer->getRenderer(), "OpenGL error: The given index buffer resource is owned by another renderer instance")

			// Create vertex array
			uint16_t id = 0;
			if (static_cast<OpenGLRenderer&>(getRenderer()).VertexArrayMakeId.CreateID(id))
			{
				// Is "GL_ARB_vertex_array_object" there?
				if (mExtensions->isGL_ARB_vertex_array_object())
				{
					// Effective vertex array object (VAO)

					// Is "GL_EXT_direct_state_access" there?
					if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
					{
						// Effective direct state access (DSA)
						return RENDERER_NEW(getRenderer().getContext(), VertexArrayVaoDsa)(static_cast<OpenGLRenderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id);
					}
					else
					{
						// Traditional bind version
						return RENDERER_NEW(getRenderer().getContext(), VertexArrayVaoBind)(static_cast<OpenGLRenderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id);
					}
				}
				else
				{
					// Traditional version
					return RENDERER_NEW(getRenderer().getContext(), VertexArrayNoVao)(static_cast<OpenGLRenderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id);
				}
			}

			// Error: Ensure a correct reference counter behaviour
			const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
			for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
			{
				vertexBuffer->vertexBuffer->addReference();
				vertexBuffer->vertexBuffer->releaseReference();
			}
			if (nullptr != indexBuffer)
			{
				indexBuffer->addReference();
				indexBuffer->releaseReference();
			}
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t = Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::R32G32B32A32F) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), (numberOfBytes % Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The OpenGL texture buffer size must be a multiple of the selected texture format bytes per texel")

			// "GL_ARB_texture_buffer_object" required
			if (mExtensions->isGL_ARB_texture_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), TextureBufferDsa)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, textureFormat);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), TextureBufferBind)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, textureFormat);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t bufferFlags, Renderer::BufferUsage bufferUsage, uint32_t numberOfStructureBytes) override
		{
			// Sanity checks
			RENDERER_ASSERT(getRenderer().getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The OpenGL structured buffer size must be a multiple of the given number of structure bytes")
			RENDERER_ASSERT(getRenderer().getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The OpenGL structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// "GL_ARB_shader_storage_buffer_object" required
			if (mExtensions->isGL_ARB_shader_storage_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), StructuredBufferDsa)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, numberOfStructureBytes);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), StructuredBufferBind)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage, numberOfStructureBytes);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// "GL_ARB_uniform_buffer_object" required
			if (mExtensions->isGL_ARB_uniform_buffer_object())
			{
				// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
				// -> Inside GLSL "layout(binding = 0, std140) writeonly uniform OutputUniformBuffer" will result in the GLSL compiler error "Failed to parse the GLSL shader source code: ERROR: 0:85: 'assign' :  l-value required "anon@6" (can't modify a uniform)"
				// -> Inside GLSL "layout(binding = 0, std430) writeonly buffer  OutputUniformBuffer" will work in OpenGL but will fail in Vulkan with "Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT" Object: "0" Location: "0" Message code: "13" Layer prefix: "Validation" Message: "Object: VK_NULL_HANDLE (Type = 0) | Type mismatch on descriptor slot 0.0 (used as type `ptr to uniform struct of (vec4 of float32)`) but descriptor of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER""
				// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid OpenGL buffer flags, uniform buffer can't be used for unordered access")
				// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0, "Invalid OpenGL buffer flags, uniform buffer must be used as shader resource")

				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), UniformBufferDsa)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), UniformBufferBind)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t indirectBufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// Sanity checks
			RENDERER_ASSERT(getRenderer().getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid OpenGL flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RENDERER_ASSERT(getRenderer().getContext(), !((indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid OpenGL flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RENDERER_ASSERT(getRenderer().getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawArguments)) == 0, "OpenGL indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RENDERER_ASSERT(getRenderer().getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawIndexedArguments)) == 0, "OpenGL indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// "GL_ARB_draw_indirect" required
			if (mExtensions->isGL_ARB_draw_indirect())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), IndirectBufferDsa)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), IndirectBufferBind)(static_cast<OpenGLRenderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), BufferManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit BufferManager(const BufferManager& source) = delete;
		BufferManager& operator =(const BufferManager& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const Extensions* mExtensions;	///< Extensions instance, always valid


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture1D.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 1D texture interface
	*/
	class Texture1D : public Renderer::ITexture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1D() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);
		}

		/**
		*  @brief
		*    Return the OpenGL texture
		*
		*  @return
		*    The OpenGL texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL texture and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLTexture && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture1D, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture1D(OpenGLRenderer& openGLRenderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat) :
			ITexture1D(static_cast<Renderer::IRenderer&>(openGLRenderer), width),
			mOpenGLTexture(0),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		GLuint mOpenGLInternalFormat;	///< OpenGL internal format


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1D(const Texture1D& source) = delete;
		Texture1D& operator =(const Texture1D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture1DBind.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 1D texture class, traditional bind version
	*/
	class Texture1DBind final : public Texture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture1DBind(OpenGLRenderer& openGLRenderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			Texture1D(openGLRenderer, width, textureFormat)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_1D, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_1D, mOpenGLTexture);

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						glCompressedTexImage1DARB(GL_TEXTURE_1D, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glCompressedTexImage1DARB(GL_TEXTURE_1D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						glTexImage1D(GL_TEXTURE_1D, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), 0, format, type, data);

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glTexImage1D(GL_TEXTURE_1D, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) && openGLRenderer.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_1D);
				glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_1D, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DBind(const Texture1DBind& source) = delete;
		Texture1DBind& operator =(const Texture1DBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture1DDsa.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 1D texture class, effective direct state access (DSA)
	*/
	class Texture1DDsa final : public Texture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture1DDsa(OpenGLRenderer& openGLRenderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			Texture1D(openGLRenderer, width, textureFormat)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Multisample texture?
			const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Create the OpenGL texture instance
			if (isArbDsa)
			{
				glCreateTextures(GL_TEXTURE_1D, 1, &mOpenGLTexture);
				glTextureStorage1D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width));
			}
			else
			{
				glGenTextures(1, &mOpenGLTexture);
			}

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glCompressedTextureSubImage1D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, static_cast<GLsizei>(width), format, numberOfBytesPerSlice, data);
						}
						else
						{
							glCompressedTextureImage1DEXT(mOpenGLTexture, GL_TEXTURE_1D, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glCompressedTextureSubImage1D(mOpenGLTexture, 0, 0, static_cast<GLsizei>(width), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
						}
					}
					else
					{
						glCompressedTextureImage1DEXT(mOpenGLTexture, GL_TEXTURE_1D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glTextureSubImage1D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, static_cast<GLsizei>(width), format, type, data);
						}
						else
						{
							glTextureImage1DEXT(mOpenGLTexture, GL_TEXTURE_1D, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), 0, format, type, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glTextureSubImage1D(mOpenGLTexture, 0, 0, static_cast<GLsizei>(width), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
					else
					{
						glTextureImage1DEXT(mOpenGLTexture, GL_TEXTURE_1D, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS)
			{
				if (isArbDsa)
				{
					glGenerateTextureMipmap(mOpenGLTexture);
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glGenerateTextureMipmapEXT(mOpenGLTexture, GL_TEXTURE_1D);
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
			}
			else
			{
				if (isArbDsa)
				{
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				else
				{
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			}

			if (isArbDsa)
			{
				glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DDsa(const Texture1DDsa& source) = delete;
		Texture1DDsa& operator =(const Texture1DDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture1DArray.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 1D array texture interface
	*/
	class Texture1DArray : public Renderer::ITexture1DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DArray() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);
		}

		/**
		*  @brief
		*    Return the OpenGL texture
		*
		*  @return
		*    The OpenGL texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL texture and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLTexture && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture1DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture1DArray(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat) :
			ITexture1DArray(static_cast<Renderer::IRenderer&>(openGLRenderer), width, numberOfSlices),
			mOpenGLTexture(0),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		GLuint mOpenGLInternalFormat;	///< OpenGL internal format


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DArray(const Texture1DArray& source) = delete;
		Texture1DArray& operator =(const Texture1DArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture1DArrayBind.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 1D array texture class, traditional bind version
	*/
	class Texture1DArrayBind final : public Texture1DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture1DArrayBind(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			Texture1DArray(openGLRenderer, width, numberOfSlices, textureFormat)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_1D_ARRAY_EXT, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, mOpenGLTexture);

			// TODO(co) Add support for user provided mipmaps
			// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
			//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   etc.

			// Upload the base map of the texture (mipmaps are automatically created as soon as the base map is changed)
			glTexImage2D(GL_TEXTURE_1D_ARRAY_EXT, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) && openGLRenderer.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_1D_ARRAY_EXT);
				glTexParameteri(GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DArrayBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DArrayBind(const Texture1DArrayBind& source) = delete;
		Texture1DArrayBind& operator =(const Texture1DArrayBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture1DArrayDsa.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 1D array texture class, effective direct state access (DSA)
	*/
	class Texture1DArrayDsa final : public Texture1DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture1DArrayDsa(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			Texture1DArray(openGLRenderer, width, numberOfSlices, textureFormat)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Create the OpenGL texture instance
			#ifdef _WIN32
				// TODO(co) It appears that DSA "glGenerateTextureMipmap()" is not working (one notices the noise) or we're using it wrong, tested with
				//			- "InstancedCubes"-example -> "CubeRendereDrawInstanced"
				//		    - AMD 290X Radeon software version 17.7.2 as well as with GeForce 980m 384.94
				//		    - Windows 10 x64
				const bool isArbDsa = (openGLRenderer.getExtensions().isGL_ARB_direct_state_access() && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) == 0);
			#else
				const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();
			#endif
			if (isArbDsa)
			{
				glCreateTextures(GL_TEXTURE_1D_ARRAY_EXT, 1, &mOpenGLTexture);
				glTextureStorage2D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices));
			}
			else
			{
				glGenTextures(1, &mOpenGLTexture);
			}

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glCompressedTextureSubImage2D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), format, numberOfBytesPerSlice, data);
						}
						else
						{
							glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, numberOfBytesPerSlice, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glCompressedTextureSubImage2D(mOpenGLTexture, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices), data);
						}
					}
					else
					{
						glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glTextureSubImage2D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), format, type, data);
						}
						else
						{
							glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, format, type, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glTextureSubImage2D(mOpenGLTexture, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
					else
					{
						glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS)
			{
				if (isArbDsa)
				{
					glGenerateTextureMipmap(mOpenGLTexture);
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glGenerateTextureMipmapEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT);
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
			}
			else
			{
				if (isArbDsa)
				{
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				else
				{
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			}

			if (isArbDsa)
			{
				glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DArrayDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DArrayDsa(const Texture1DArrayDsa& source) = delete;
		Texture1DArrayDsa& operator =(const Texture1DArrayDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture2D.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 2D texture interface
	*/
	class Texture2D : public Renderer::ITexture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2D() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);
		}

		/**
		*  @brief
		*    Return the number of multisamples
		*
		*  @return
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		[[nodiscard]] inline uint8_t getNumberOfMultisamples() const
		{
			return mNumberOfMultisamples;
		}

		/**
		*  @brief
		*    Return the OpenGL texture
		*
		*  @return
		*    The OpenGL texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Texture2D methods                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Set minimum maximum mipmap index
		*
		*  @param[in] minimumMipmapIndex
		*    Minimum mipmap index, the most detailed mipmap, also known as base mipmap, 0 by default
		*  @param[in] maximumMipmapIndex
		*    Maximum mipmap index, the least detailed mipmap, <number of mipmaps> by default
		*/
		virtual void setMinimumMaximumMipmapIndex(uint32_t minimumMipmapIndex, uint32_t maximumMipmapIndex) = 0;


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL texture and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLTexture && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2D, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		inline Texture2D(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, uint8_t numberOfMultisamples) :
			ITexture2D(static_cast<Renderer::IRenderer&>(openGLRenderer), width, height),
			mNumberOfMultisamples(numberOfMultisamples),
			mOpenGLTexture(0),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		uint8_t mNumberOfMultisamples;	///< The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		GLuint  mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		GLuint  mOpenGLInternalFormat;	///< OpenGL internal format


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2D(const Texture2D& source) = delete;
		Texture2D& operator =(const Texture2D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture2DBind.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 2D texture class, traditional bind version
	*/
	class Texture2DBind final : public Texture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		Texture2DBind(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples) :
			Texture2D(openGLRenderer, width, height, textureFormat, numberOfMultisamples)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS), "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Renderer::TextureFlag::RENDER_TARGET), "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			// Multisample texture?
			if (numberOfMultisamples > 1)
			{
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL texture
					GLint openGLTextureBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &openGLTextureBackup);
				#endif

				// Make this OpenGL texture instance to the currently used one
				glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mOpenGLTexture);

				// Define the texture
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numberOfMultisamples, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_TRUE);

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture
					glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(openGLTextureBackup));
				#endif
			}
			else
			{
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently set alignment
					GLint openGLAlignmentBackup = 0;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

					// Backup the currently bound OpenGL texture
					GLint openGLTextureBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLTextureBackup);
				#endif

				// Set correct unpack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

				// Calculate the number of mipmaps
				const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
				const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
				const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

				// Make this OpenGL texture instance to the currently used one
				glBindTexture(GL_TEXTURE_2D, mOpenGLTexture);

				// Upload the texture data
				if (Renderer::TextureFormat::isCompressed(textureFormat))
				{
					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
							glCompressedTexImage2DARB(GL_TEXTURE_2D, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, numberOfBytesPerSlice, data);

							// Move on to the next mipmap and ensure the size is always at least 1x1
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
							width = getHalfSize(width);
							height = getHalfSize(height);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
					}
				}
				else
				{
					// Texture format is not compressed

					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
						const uint32_t type = Mapping::getOpenGLType(textureFormat);
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
							glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, data);

							// Move on to the next mipmap and ensure the size is always at least 1x1
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
							width = getHalfSize(width);
							height = getHalfSize(height);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
					}
				}

				// Build mipmaps automatically on the GPU? (or GPU driver)
				if ((textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) && openGLRenderer.getExtensions().isGL_ARB_framebuffer_object())
				{
					glGenerateMipmap(GL_TEXTURE_2D);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture
					glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLTextureBackup));

					// Restore previous alignment
					glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DBind() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Texture2D methods                      ]
	//[-------------------------------------------------------]
	public:
		virtual void setMinimumMaximumMipmapIndex(uint32_t minimumMipmapIndex, uint32_t maximumMipmapIndex) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLTextureBackup);
			#endif

			// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_sampler_objects.txt
			// "
			//  2) What is the set of state associated with a sampler object?
			//     Specifically, should TEXTURE_BASE_LEVEL and TEXTURE_MAX_LEVEL be
			//     part of the sampler or the texture?
			//  DISCUSSION: TEXTURE_BASE_LEVEL and TEXTURE_MAX_LEVEL are presently
			//  part of the image state (texture) and are thus not included in the
			//  sampler object.
			// "
			glBindTexture(GL_TEXTURE_2D, mOpenGLTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, static_cast<GLint>(minimumMipmapIndex));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(maximumMipmapIndex));

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLTextureBackup));
			#endif
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DBind(const Texture2DBind& source) = delete;
		Texture2DBind& operator =(const Texture2DBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture2DDsa.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 2D texture class, effective direct state access (DSA)
	*/
	class Texture2DDsa final : public Texture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		Texture2DDsa(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples) :
			Texture2D(openGLRenderer, width, height, textureFormat, numberOfMultisamples)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS), "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Renderer::TextureFlag::RENDER_TARGET), "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Multisample texture?
			const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();
			if (numberOfMultisamples > 1)
			{
				if (isArbDsa)
				{
					// Create the OpenGL texture instance
					glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &mOpenGLTexture);
					glTextureStorage2DMultisample(mOpenGLTexture, numberOfMultisamples, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_TRUE);
				}
				else
				{
					// Create the OpenGL texture instance
					glGenTextures(1, &mOpenGLTexture);

					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Backup the currently bound OpenGL texture
						GLint openGLTextureBackup = 0;
						glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &openGLTextureBackup);
					#endif

					// Make this OpenGL texture instance to the currently used one
					glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mOpenGLTexture);

					// Define the texture
					// -> Sadly, there's no direct state access (DSA) function defined for this in "GL_EXT_direct_state_access"
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numberOfMultisamples, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_TRUE);

					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL texture
						glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(openGLTextureBackup));
					#endif
				}
			}
			else
			{
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently set alignment
					GLint openGLAlignmentBackup = 0;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
				#endif

				// Set correct unpack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

				// Calculate the number of mipmaps
				const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
				const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
				const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

				// Create the OpenGL texture instance
				if (isArbDsa)
				{
					glCreateTextures(GL_TEXTURE_2D, 1, &mOpenGLTexture);
					glTextureStorage2D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
				}
				else
				{
					glGenTextures(1, &mOpenGLTexture);
				}

				// Upload the texture data
				if (Renderer::TextureFormat::isCompressed(textureFormat))
				{
					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
							if (isArbDsa)
							{
								// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
								glCompressedTextureSubImage2D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), format, numberOfBytesPerSlice, data);
							}
							else
							{
								glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_2D, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, numberOfBytesPerSlice, data);
							}

							// Move on to the next mipmap and ensure the size is always at least 1x1
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
							width = getHalfSize(width);
							height = getHalfSize(height);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						if (isArbDsa)
						{
							if (nullptr != data)
							{
								glCompressedTextureSubImage2D(mOpenGLTexture, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
							}
						}
						else
						{
							glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_2D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
						}
					}
				}
				else
				{
					// Texture format is not compressed

					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
						const uint32_t type = Mapping::getOpenGLType(textureFormat);
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
							if (isArbDsa)
							{
								// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
								glTextureSubImage2D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), format, type, data);
							}
							else
							{
								glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_2D, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, data);
							}

							// Move on to the next mipmap and ensure the size is always at least 1x1
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
							width = getHalfSize(width);
							height = getHalfSize(height);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						if (isArbDsa)
						{
							if (nullptr != data)
							{
								glTextureSubImage2D(mOpenGLTexture, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
							}
						}
						else
						{
							glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_2D, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
				}

				// Build mipmaps automatically on the GPU? (or GPU driver)
				if (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS)
				{
					if (isArbDsa)
					{
						glGenerateTextureMipmap(mOpenGLTexture);
						glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
					}
					else
					{
						glGenerateTextureMipmapEXT(mOpenGLTexture, GL_TEXTURE_2D);
						glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
					}
				}
				else
				{
					if (isArbDsa)
					{
						glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					}
					else
					{
						glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					}
				}

				if (isArbDsa)
				{
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}
				else
				{
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Restore previous alignment
					glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Texture2D methods                      ]
	//[-------------------------------------------------------]
	public:
		virtual void setMinimumMaximumMipmapIndex(uint32_t minimumMipmapIndex, uint32_t maximumMipmapIndex) override
		{
			// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_sampler_objects.txt
			// "
			//  2) What is the set of state associated with a sampler object?
			//     Specifically, should TEXTURE_BASE_LEVEL and TEXTURE_MAX_LEVEL be
			//     part of the sampler or the texture?
			//  DISCUSSION: TEXTURE_BASE_LEVEL and TEXTURE_MAX_LEVEL are presently
			//  part of the image state (texture) and are thus not included in the
			//  sampler object.
			// "
			glTextureParameteri(mOpenGLTexture, GL_TEXTURE_BASE_LEVEL, static_cast<GLint>(minimumMipmapIndex));
			glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(maximumMipmapIndex));
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DDsa(const Texture2DDsa& source) = delete;
		Texture2DDsa& operator =(const Texture2DDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture2DArray.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 2D array texture interface
	*/
	class Texture2DArray : public Renderer::ITexture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArray() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);
		}

		/**
		*  @brief
		*    Return the number of multisamples
		*
		*  @return
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		[[nodiscard]] inline uint8_t getNumberOfMultisamples() const
		{
			return mNumberOfMultisamples;
		}

		/**
		*  @brief
		*    Return the OpenGL texture
		*
		*  @return
		*    The OpenGL texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL texture and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLTexture && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture2DArray(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat) :
			ITexture2DArray(static_cast<Renderer::IRenderer&>(openGLRenderer), width, height, numberOfSlices),
			mNumberOfMultisamples(1),	// TODO(co) Currently no MSAA support for 2D array textures
			mOpenGLTexture(0),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		uint8_t mNumberOfMultisamples;	///< The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		GLuint  mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		GLuint  mOpenGLInternalFormat;	///< OpenGL internal format


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture2DArrayBind.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 2D array texture class, traditional bind version
	*/
	class Texture2DArrayBind final : public Texture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture2DArrayBind(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			Texture2DArray(openGLRenderer, width, height, numberOfSlices, textureFormat)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY_EXT, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mOpenGLTexture);

			// TODO(co) Add support for user provided mipmaps
			// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
			//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   etc.

			// Upload the base map of the texture (mipmaps are automatically created as soon as the base map is changed)
			glTexImage3DEXT(GL_TEXTURE_2D_ARRAY_EXT, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) && openGLRenderer.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_2D_ARRAY_EXT);
				glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArrayBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArrayBind(const Texture2DArrayBind& source) = delete;
		Texture2DArrayBind& operator =(const Texture2DArrayBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture2DArrayDsa.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 2D array texture class, effective direct state access (DSA)
	*/
	class Texture2DArrayDsa final : public Texture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		Texture2DArrayDsa(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			Texture2DArray(openGLRenderer, width, height, numberOfSlices, textureFormat)
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Create the OpenGL texture instance
			#ifdef _WIN32
				// TODO(co) It appears that DSA "glGenerateTextureMipmap()" is not working (one notices the noise) or we're using it wrong, tested with
				//			- "InstancedCubes"-example -> "CubeRendereDrawInstanced"
				//		    - AMD 290X Radeon software version 17.7.2 as well as with GeForce 980m 384.94
				//		    - Windows 10 x64
				const bool isArbDsa = (openGLRenderer.getExtensions().isGL_ARB_direct_state_access() && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) == 0);
			#else
				const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();
			#endif
			if (isArbDsa)
			{
				glCreateTextures(GL_TEXTURE_2D_ARRAY_EXT, 1, &mOpenGLTexture);
				glTextureStorage3D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices));
			}
			else
			{
				glGenTextures(1, &mOpenGLTexture);
			}

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glCompressedTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), format, numberOfBytesPerSlice, data);
						}
						else
						{
							glCompressedTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, numberOfBytesPerSlice, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glCompressedTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices), data);
						}
					}
					else
					{
						glCompressedTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), format, type, data);
						}
						else
						{
							glTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, format, type, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
					else
					{
						glTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS)
			{
				if (isArbDsa)
				{
					glGenerateTextureMipmap(mOpenGLTexture);
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glGenerateTextureMipmapEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT);
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
			}
			else
			{
				if (isArbDsa)
				{
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				else
				{
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			}

			if (isArbDsa)
			{
				glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArrayDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArrayDsa(const Texture2DArrayDsa& source) = delete;
		Texture2DArrayDsa& operator =(const Texture2DArrayDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture3D.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 3D texture interface
	*/
	class Texture3D : public Renderer::ITexture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3D() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);
			glDeleteBuffersARB(1, &mOpenGLPixelUnpackBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL texture
		*
		*  @return
		*    The OpenGL texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the texture format
		*
		*  @return
		*    The texture format
		*/
		[[nodiscard]] inline Renderer::TextureFormat::Enum getTextureFormat() const
		{
			return mTextureFormat;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}

		/**
		*  @brief
		*    Return the OpenGL pixel unpack buffer for dynamic textures
		*
		*  @return
		*    The OpenGL pixel unpack buffer for dynamic textures, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLPixelUnpackBuffer() const
		{
			return mOpenGLPixelUnpackBuffer;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL texture and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLTexture && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture3D, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] depth
		*    The depth of the texture
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture3D(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat) :
			ITexture3D(static_cast<Renderer::IRenderer&>(openGLRenderer), width, height, depth),
			mOpenGLTexture(0),
			mTextureFormat(textureFormat),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat)),
			mOpenGLPixelUnpackBuffer(0)
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint						  mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		Renderer::TextureFormat::Enum mTextureFormat;
		GLuint						  mOpenGLInternalFormat;	///< OpenGL internal format
		GLuint						  mOpenGLPixelUnpackBuffer;	///< OpenGL pixel unpack buffer for dynamic textures, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture3DBind.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 3D texture class, traditional bind version
	*/
	class Texture3DBind final : public Texture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] depth
		*    Texture depth, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture3DBind(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage) :
			Texture3D(openGLRenderer, width, height, depth, textureFormat)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_3D, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create OpenGL pixel unpack buffer for dynamic textures, if necessary
			if (Renderer::TextureUsage::IMMUTABLE != textureUsage)
			{
				// Backup the currently bound OpenGL pixel unpack buffer
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					GLint openGLUnpackBufferBackup = 0;
					glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &openGLUnpackBufferBackup);
				#endif

				// Create the OpenGL pixel unpack buffer
				glGenBuffersARB(1, &mOpenGLPixelUnpackBuffer);

				// Bind this OpenGL pixel unpack buffer, the OpenGL pixel unpack buffer must be able to hold the top-level mipmap
				// TODO(co) Or must the OpenGL pixel unpack buffer able to hold the entire texture including all mipmaps? Depends on the later usage I assume.
				const uint32_t numberOfBytes = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth;
				glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, mOpenGLPixelUnpackBuffer);
				glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, static_cast<GLsizeiptrARB>(numberOfBytes), nullptr, static_cast<GLenum>(GL_STREAM_DRAW));

				// Be polite and restore the previous bound OpenGL pixel unpack buffer
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, static_cast<GLuint>(openGLUnpackBufferBackup));
				#else
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
				#endif
			}

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(openGLRenderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "OpenGL immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_3D, mOpenGLTexture);

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						glCompressedTexImage3DARB(GL_TEXTURE_3D, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, numberOfBytesPerMipmap, data);

						// Move on to the next mipmap and ensure the size is always at least 1x1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerMipmap;
						width = getHalfSize(width);
						height = getHalfSize(height);
						depth = getHalfSize(depth);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glCompressedTexImage3DARB(GL_TEXTURE_3D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						glTexImage3DEXT(GL_TEXTURE_3D, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, type, data);

						// Move on to the next mipmap and ensure the size is always at least 1x1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerMipmap;
						width = getHalfSize(width);
						height = getHalfSize(height);
						depth = getHalfSize(depth);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glTexImage3DEXT(GL_TEXTURE_3D, 0, static_cast<GLenum>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) && openGLRenderer.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_3D);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_3D, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3DBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3DBind(const Texture3DBind& source) = delete;
		Texture3DBind& operator =(const Texture3DBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/Texture3DDsa.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL 3D texture class, effective direct state access (DSA)
	*/
	class Texture3DDsa final : public Texture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] depth
		*    Texture depth, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture3DDsa(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage) :
			Texture3D(openGLRenderer, width, height, depth, textureFormat)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create OpenGL pixel unpack buffer for dynamic textures, if necessary
			if (Renderer::TextureUsage::IMMUTABLE != textureUsage)
			{
				// Create the OpenGL pixel unpack buffer
				glCreateBuffers(1, &mOpenGLPixelUnpackBuffer);

				// The OpenGL pixel unpack buffer must be able to hold the top-level mipmap
				// TODO(co) Or must the OpenGL pixel unpack buffer able to hold the entire texture including all mipmaps? Depends on the later usage I assume.
				const uint32_t numberOfBytes = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth;
				glNamedBufferData(mOpenGLPixelUnpackBuffer, static_cast<GLsizeiptr>(numberOfBytes), nullptr, GL_STREAM_DRAW);
			}

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(openGLRenderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "OpenGL immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;

			// Create the OpenGL texture instance
			const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();
			if (isArbDsa)
			{
				glCreateTextures(GL_TEXTURE_3D, 1, &mOpenGLTexture);
				glTextureStorage3D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth));
			}
			else
			{
				glGenTextures(1, &mOpenGLTexture);
			}

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glCompressedTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), format, numberOfBytesPerMipmap, data);
						}
						else
						{
							glCompressedTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_3D, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, numberOfBytesPerMipmap, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerMipmap;
						width = getHalfSize(width);
						height = getHalfSize(height);
						depth = getHalfSize(depth);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glCompressedTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
						}
					}
					else
					{
						glCompressedTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_3D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), format, type, data);
						}
						else
						{
							glTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_3D, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, type, data);
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerMipmap;
						width = getHalfSize(width);
						height = getHalfSize(height);
						depth = getHalfSize(depth);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
					else
					{
						glTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_3D, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS)
			{
				if (isArbDsa)
				{
					glGenerateTextureMipmap(mOpenGLTexture);
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glGenerateTextureMipmapEXT(mOpenGLTexture, GL_TEXTURE_3D);
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
			}
			else
			{
				if (isArbDsa)
				{
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				else
				{
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			}

			if (isArbDsa)
			{
				glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3DDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3DDsa(const Texture3DDsa& source) = delete;
		Texture3DDsa& operator =(const Texture3DDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/TextureCube.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL cube texture interface
	*/
	class TextureCube : public Renderer::ITextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCube() override
		{
			// Destroy the OpenGL texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLTexture);
		}

		/**
		*  @brief
		*    Return the OpenGL texture
		*
		*  @return
		*    The OpenGL texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLTexture() const
		{
			return mOpenGLTexture;
		}

		/**
		*  @brief
		*    Return the OpenGL internal format
		*
		*  @return
		*    The OpenGL internal format
		*/
		[[nodiscard]] inline GLuint getOpenGLInternalFormat() const
		{
			return mOpenGLInternalFormat;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL texture and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLTexture && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, name);
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureCube, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline TextureCube(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat) :
			ITextureCube(static_cast<Renderer::IRenderer&>(openGLRenderer), width, height),
			mOpenGLTexture(0),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat))
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		GLuint mOpenGLInternalFormat;	///< OpenGL internal format


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCube(const TextureCube& source) = delete;
		TextureCube& operator =(const TextureCube& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/TextureCubeBind.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL cube texture class, traditional bind version
	*/
	class TextureCubeBind final : public TextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		TextureCubeBind(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			TextureCube(openGLRenderer, width, height, textureFormat)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_CUBE_MAP, mOpenGLTexture);

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, numberOfBytesPerSlice, data);

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					const uint32_t numberOfBytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
					for (uint32_t face = 0; face < 6; ++face)
					{
						glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(numberOfBytesPerSlice), data);
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, data);

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					const uint32_t numberOfBytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
					const uint32_t openGLFormat = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t openGLType = Mapping::getOpenGLType(textureFormat);
					for (uint32_t face = 0; face < 6; ++face)
					{
						glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, openGLFormat, openGLType, data);
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) && openGLRenderer.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCubeBind() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCubeBind(const TextureCubeBind& source) = delete;
		TextureCubeBind& operator =(const TextureCubeBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/TextureCubeDsa.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL cube texture class, effective direct state access (DSA)
	*/
	class TextureCubeDsa final : public TextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*/
		TextureCubeDsa(OpenGLRenderer& openGLRenderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags) :
			TextureCube(openGLRenderer, width, height, textureFormat)
		{
			// Sanity checks
			RENDERER_ASSERT(openGLRenderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RENDERER_ASSERT(openGLRenderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Create the OpenGL texture instance
			// TODO(co) "GL_ARB_direct_state_access" AMD graphics card driver bug ahead
			// -> AMD graphics card: 13.02.2017 using Radeon software 17.1.1 on Microsoft Windows: Looks like "GL_ARB_direct_state_access" is broken when trying to use "glCompressedTextureSubImage3D()" for upload
			// -> Describes the same problem: https://community.amd.com/thread/194748 - "Upload data to GL_TEXTURE_CUBE_MAP with glTextureSubImage3D (DSA) broken ?"
			#ifdef _WIN32
				const bool isArbDsa = false;
			#else
				const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();
			#endif
			if (isArbDsa)
			{
				glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mOpenGLTexture);
				glTextureStorage2D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
			}
			else
			{
				glGenTextures(1, &mOpenGLTexture);
			}

			// Upload the texture data
			if (Renderer::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						if (isArbDsa)
						{
							// With ARB DSA cube maps are a special form of a cube map array so we can upload all 6 faces at once per mipmap
							// See https://www.khronos.org/opengl/wiki/Direct_State_Access (Last paragraph in "Changes from EXT")
							// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glCompressedTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 6, format, numberOfBytesPerSlice * 6, data);

							// Move on to the next mipmap
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice * 6;
						}
						else
						{
							for (uint32_t face = 0; face < 6; ++face)
							{
								// Upload the current face
								glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, numberOfBytesPerSlice, data);

								// Move on to the next face
								data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
							}
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glCompressedTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 6, Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)) * 6, data);
						}
					}
					else
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, numberOfBytesPerSlice, data);

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							if (isArbDsa)
							{
								// We know that "data" must be valid when we're in here due to the "Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
								glTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, static_cast<GLint>(face), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 1, format, type, data);
							}
							else
							{
								glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, data);
							}

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					if (isArbDsa)
					{
						if (nullptr != data)
						{
							glTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 6, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
					else
					{
						const uint32_t numberOfBytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						const uint32_t openGLFormat = Mapping::getOpenGLFormat(textureFormat);
						const uint32_t openGLType = Mapping::getOpenGLType(textureFormat);
						for (uint32_t face = 0; face < 6; ++face)
						{
							glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, openGLFormat, openGLType, data);
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS)
			{
				if (isArbDsa)
				{
					glGenerateTextureMipmap(mOpenGLTexture);
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glGenerateTextureMipmapEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP);
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
			}
			else
			{
				if (isArbDsa)
				{
					glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				else
				{
					glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			}

			if (isArbDsa)
			{
				glTextureParameteri(mOpenGLTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTextureParameteriEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCubeDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCubeDsa(const TextureCubeDsa& source) = delete;
		TextureCubeDsa& operator =(const TextureCubeDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Texture/TextureManager.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL texture manager interface
	*/
	class TextureManager final : public Renderer::ITextureManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit TextureManager(OpenGLRenderer& openGLRenderer) :
			ITextureManager(openGLRenderer),
			mExtensions(&openGLRenderer.getExtensions())
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ITextureManager methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Renderer::ITexture1D* createTexture1D(uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0, "OpenGL create texture 1D was called with invalid parameters")

			// Create 1D texture resource: Is "GL_EXT_direct_state_access" there?
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RENDERER_NEW(getRenderer().getContext(), Texture1DDsa)(static_cast<OpenGLRenderer&>(getRenderer()), width, textureFormat, data, textureFlags);
			}
			else
			{
				// Traditional bind version
				return RENDERER_NEW(getRenderer().getContext(), Texture1DBind)(static_cast<OpenGLRenderer&>(getRenderer()), width, textureFormat, data, textureFlags);
			}
		}

		[[nodiscard]] virtual Renderer::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && numberOfSlices > 0, "OpenGL create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource, "GL_EXT_texture_array"-extension required
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_texture_array())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), Texture1DArrayDsa)(static_cast<OpenGLRenderer&>(getRenderer()), width, numberOfSlices, textureFormat, data, textureFlags);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), Texture1DArrayBind)(static_cast<OpenGLRenderer&>(getRenderer()), width, numberOfSlices, textureFormat, data, textureFlags);
				}
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0, "OpenGL create texture 2D was called with invalid parameters")

			// Create 2D texture resource: Is "GL_EXT_direct_state_access" there?
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RENDERER_NEW(getRenderer().getContext(), Texture2DDsa)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, numberOfMultisamples);
			}
			else
			{
				// Traditional bind version
				return RENDERER_NEW(getRenderer().getContext(), Texture2DBind)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, numberOfMultisamples);
			}
		}

		[[nodiscard]] virtual Renderer::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0 && numberOfSlices > 0, "OpenGL create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource, "GL_EXT_texture_array"-extension required
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_texture_array())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RENDERER_NEW(getRenderer().getContext(), Texture2DArrayDsa)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, numberOfSlices, textureFormat, data, textureFlags);
				}
				else
				{
					// Traditional bind version
					return RENDERER_NEW(getRenderer().getContext(), Texture2DArrayBind)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, numberOfSlices, textureFormat, data, textureFlags);
				}
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0 && depth > 0, "OpenGL create texture 3D was called with invalid parameters")

			// Create 3D texture resource: Is "GL_EXT_direct_state_access" there?
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RENDERER_NEW(getRenderer().getContext(), Texture3DDsa)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, depth, textureFormat, data, textureFlags, textureUsage);
			}
			else
			{
				// Traditional bind version
				return RENDERER_NEW(getRenderer().getContext(), Texture3DBind)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, depth, textureFormat, data, textureFlags, textureUsage);
			}
		}

		[[nodiscard]] virtual Renderer::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0, "OpenGL create texture cube was called with invalid parameters")

			// Create cube texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			// Is "GL_EXT_direct_state_access" or "GL_ARB_direct_state_access" there?
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RENDERER_NEW(getRenderer().getContext(), TextureCubeDsa)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, textureFormat, data, textureFlags);
			}
			else
			{
				// Traditional bind version
				return RENDERER_NEW(getRenderer().getContext(), TextureCubeBind)(static_cast<OpenGLRenderer&>(getRenderer()), width, height, textureFormat, data, textureFlags);
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureManager(const TextureManager& source) = delete;
		TextureManager& operator =(const TextureManager& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const Extensions* mExtensions;	///< Extensions instance, always valid


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/SamplerState.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL sampler state interface
	*/
	class SamplerState : public Renderer::ISamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerState() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SamplerState, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit SamplerState(OpenGLRenderer& openGLRenderer) :
			ISamplerState(openGLRenderer)
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerState(const SamplerState& source) = delete;
		SamplerState& operator =(const SamplerState& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/SamplerStateBind.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL sampler state class, traditional bind version to emulate a sampler object
	*/
	class SamplerStateBind final : public SamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerStateBind(OpenGLRenderer& openGLRenderer, const Renderer::SamplerState& samplerState) :
			SamplerState(openGLRenderer),
			mOpenGLMagFilterMode(Mapping::getOpenGLMagFilterMode(openGLRenderer.getContext(), samplerState.filter)),
			mOpenGLMinFilterMode(Mapping::getOpenGLMinFilterMode(openGLRenderer.getContext(), samplerState.filter, samplerState.maxLOD > 0.0f)),
			mOpenGLTextureAddressModeS(Mapping::getOpenGLTextureAddressMode(samplerState.addressU)),
			mOpenGLTextureAddressModeT(Mapping::getOpenGLTextureAddressMode(samplerState.addressV)),
			mOpenGLTextureAddressModeR(Mapping::getOpenGLTextureAddressMode(samplerState.addressW)),
			mMipLODBias(samplerState.mipLODBias),
			mMaxAnisotropy(static_cast<float>(samplerState.maxAnisotropy)),	// Maximum anisotropy is "uint32_t" in Direct3D 10 & 11
			mOpenGLCompareMode(Mapping::getOpenGLCompareMode(openGLRenderer.getContext(), samplerState.filter)),
			mOpenGLComparisonFunc(Mapping::getOpenGLComparisonFunc(samplerState.comparisonFunc)),
			mMinLOD(samplerState.minLOD),
			mMaxLOD(samplerState.maxLOD)
		{
			// Sanity check
			RENDERER_ASSERT(openGLRenderer.getContext(), samplerState.maxAnisotropy <= openGLRenderer.getCapabilities().maximumAnisotropy, "Maximum OpenGL anisotropy value violated")

			// Renderer::SamplerState::borderColor[4]
			mBorderColor[0] = samplerState.borderColor[0];
			mBorderColor[1] = samplerState.borderColor[1];
			mBorderColor[2] = samplerState.borderColor[2];
			mBorderColor[3] = samplerState.borderColor[3];
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerStateBind() override
		{}

		/**
		*  @brief
		*    Set the OpenGL sampler states
		*/
		void setOpenGLSamplerStates() const
		{
			// TODO(co) Support other targets, not just "GL_TEXTURE_2D"

			// Renderer::SamplerState::filter
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mOpenGLMagFilterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mOpenGLMinFilterMode);

			// Renderer::SamplerState::addressU
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mOpenGLTextureAddressModeS);

			// Renderer::SamplerState::addressV
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mOpenGLTextureAddressModeT);
			/*
			// TODO(co) Support for 3D textures
			// Renderer::SamplerState::addressW
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, mOpenGLTextureAddressModeR);

			// TODO(co) Complete me
			// Renderer::SamplerState::mipLODBias
			// -> "GL_EXT_texture_lod_bias"-extension
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_LOD_BIAS, samplerState.mipLODBias);

			// Renderer::SamplerState::maxAnisotropy
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerState.maxAnisotropy);

			// Renderer::SamplerState::comparisonFunc
			// -> "GL_EXT_shadow_funcs"/"GL_EXT_shadow_samplers"-extension
			glTexParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_MODE, Mapping::getOpenGLCompareMode(samplerState.filter));
			glTexParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_FUNC, Mapping::getOpenGLComparisonFunc(samplerState.comparisonFunc));

			// Renderer::SamplerState::borderColor[4]
			glSamplerParameterfv(mOpenGLSampler, GL_TEXTURE_BORDER_COLOR, samplerState.borderColor);

			// Renderer::SamplerState::minLOD
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MIN_LOD, samplerState.minLOD);

			// Renderer::SamplerState::maxLOD
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_LOD, samplerState.maxLOD);
			*/
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerStateBind(const SamplerStateBind& source) = delete;
		SamplerStateBind& operator =(const SamplerStateBind& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// "Renderer::SamplerState" translated into OpenGL
		GLint  mOpenGLMagFilterMode;		///< Renderer::SamplerState::filter
		GLint  mOpenGLMinFilterMode;		///< Renderer::SamplerState::filter
		GLint  mOpenGLTextureAddressModeS;	///< Renderer::SamplerState::addressU
		GLint  mOpenGLTextureAddressModeT;	///< Renderer::SamplerState::addressV
		GLint  mOpenGLTextureAddressModeR;	///< Renderer::SamplerState::addressW
		float  mMipLODBias;					///< Renderer::SamplerState::mipLODBias
		float  mMaxAnisotropy;				///< Renderer::SamplerState::maxAnisotropy
		GLint  mOpenGLCompareMode;			///< Renderer::SamplerState::comparisonFunc
		GLenum mOpenGLComparisonFunc;		///< Renderer::SamplerState::comparisonFunc
		float  mBorderColor[4];				///< Renderer::SamplerState::borderColor[4]
		float  mMinLOD;						///< Renderer::SamplerState::minLOD
		float  mMaxLOD;						///< Renderer::SamplerState::maxLOD


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/SamplerStateDsa.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL sampler state class, direct state access (DSA) version to emulate a sampler object
	*/
	class SamplerStateDsa final : public SamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		inline SamplerStateDsa(OpenGLRenderer& openGLRenderer, const Renderer::SamplerState& samplerState) :
			SamplerState(openGLRenderer),
			mSamplerState(samplerState)
		{
			// Sanity check
			RENDERER_ASSERT(openGLRenderer.getContext(), samplerState.maxAnisotropy <= openGLRenderer.getCapabilities().maximumAnisotropy, "Maximum OpenGL anisotropy value violated")
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerStateDsa() override
		{}

		/**
		*  @brief
		*    Set the OpenGL sampler states
		*/
		inline void setOpenGLSamplerStates() const
		{
			// TODO(co) Implement me
			// http://www.opengl.org/registry/specs/ARB/sampler_objects.txt - GL_ARB_sampler_objects
			// http://www.ozone3d.net/blogs/lab/20110908/tutorial-opengl-3-3-sampler-states-configurer-unites-de-texture/#more-701 - sample
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerStateDsa(const SamplerStateDsa& source) = delete;
		SamplerStateDsa& operator =(const SamplerStateDsa& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::SamplerState mSamplerState;	///< Sampler state


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/SamplerStateSo.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL sampler state class, effective sampler object (SO)
	*/
	class SamplerStateSo final : public SamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerStateSo(OpenGLRenderer& openGLRenderer, const Renderer::SamplerState& samplerState) :
			SamplerState(openGLRenderer),
			mOpenGLSampler(0)
		{
			// Sanity check
			RENDERER_ASSERT(openGLRenderer.getContext(), samplerState.maxAnisotropy <= openGLRenderer.getCapabilities().maximumAnisotropy, "Maximum OpenGL anisotropy value violated")

			// Create the OpenGL sampler
			glGenSamplers(1, &mOpenGLSampler);

			// Renderer::SamplerState::filter
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_MAG_FILTER, Mapping::getOpenGLMagFilterMode(openGLRenderer.getContext(), samplerState.filter));
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_MIN_FILTER, Mapping::getOpenGLMinFilterMode(openGLRenderer.getContext(), samplerState.filter, samplerState.maxLOD > 0.0f));

			// Renderer::SamplerState::addressU
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_WRAP_S, Mapping::getOpenGLTextureAddressMode(samplerState.addressU));

			// Renderer::SamplerState::addressV
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_WRAP_T, Mapping::getOpenGLTextureAddressMode(samplerState.addressV));

			// Renderer::SamplerState::addressW
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_WRAP_R, Mapping::getOpenGLTextureAddressMode(samplerState.addressW));

			// Renderer::SamplerState::mipLODBias
			// -> "GL_EXT_texture_lod_bias"-extension
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_LOD_BIAS, samplerState.mipLODBias);

			// Renderer::SamplerState::maxAnisotropy
			// -> Maximum anisotropy is "uint32_t" in Direct3D 10 & 11
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(samplerState.maxAnisotropy));

			// Renderer::SamplerState::comparisonFunc
			// -> "GL_EXT_shadow_funcs"/"GL_EXT_shadow_samplers"-extension
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_MODE, Mapping::getOpenGLCompareMode(openGLRenderer.getContext(), samplerState.filter));
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_FUNC, static_cast<GLint>(Mapping::getOpenGLComparisonFunc(samplerState.comparisonFunc)));

			// Renderer::SamplerState::borderColor[4]
			glSamplerParameterfv(mOpenGLSampler, GL_TEXTURE_BORDER_COLOR, samplerState.borderColor);

			// Renderer::SamplerState::minLOD
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MIN_LOD, samplerState.minLOD);

			// Renderer::SamplerState::maxLOD
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_LOD, samplerState.maxLOD);
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerStateSo() override
		{
			// Destroy the OpenGL sampler
			// -> Silently ignores 0's and names that do not correspond to existing samplers
			glDeleteSamplers(1, &mOpenGLSampler);
		}

		/**
		*  @brief
		*    Return the OpenGL sampler
		*
		*  @return
		*    The OpenGL sampler, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLSampler() const
		{
			return mOpenGLSampler;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL sampler and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLSampler && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SAMPLER, mOpenGLSampler, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerStateSo(const SamplerStateSo& source) = delete;
		SamplerStateSo& operator =(const SamplerStateSo& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLSampler;	///< OpenGL sampler, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/IState.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract state base class
	*/
	class IState
	{


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Default constructor
		*/
		inline IState()
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~IState()
		{}


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/RasterizerState.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL rasterizer state class
	*/
	class RasterizerState final : public IState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rasterizerState
		*    Rasterizer state to use
		*/
		inline explicit RasterizerState(const Renderer::RasterizerState& rasterizerState) :
			mRasterizerState(rasterizerState),
			mOpenGLFrontFaceMode(static_cast<GLenum>(mRasterizerState.frontCounterClockwise ? GL_CCW : GL_CW))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~RasterizerState()
		{}

		/**
		*  @brief
		*    Return the rasterizer state
		*
		*  @return
		*    The rasterizer state
		*/
		[[nodiscard]] inline const Renderer::RasterizerState& getRasterizerState() const
		{
			return mRasterizerState;
		}

		/**
		*  @brief
		*    Set the OpenGL rasterizer states
		*/
		void setOpenGLRasterizerStates() const
		{
			// Renderer::RasterizerState::fillMode
			switch (mRasterizerState.fillMode)
			{
				// Wireframe
				case Renderer::FillMode::WIREFRAME:
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					break;

				// Solid
				default:
				case Renderer::FillMode::SOLID:
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					break;
			}

			// Renderer::RasterizerState::cullMode
			switch (mRasterizerState.cullMode)
			{
				// No culling
				default:
				case Renderer::CullMode::NONE:
					glDisable(GL_CULL_FACE);
					break;

				// Selects clockwise polygons as front-facing
				case Renderer::CullMode::FRONT:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);
					break;

				// Selects counterclockwise polygons as front-facing
				case Renderer::CullMode::BACK:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);
					break;
			}

			// Renderer::RasterizerState::frontCounterClockwise
			glFrontFace(mOpenGLFrontFaceMode);

			// TODO(co) Map the rest of the rasterizer states

			// RasterizerState::depthBias

			// RasterizerState::depthBiasClamp

			// RasterizerState::slopeScaledDepthBias

			// RasterizerState::depthClipEnable
			if (mRasterizerState.depthClipEnable)
			{
				glDisable(GL_DEPTH_CLAMP);
			}
			else
			{
				glEnable(GL_DEPTH_CLAMP);
			}

			// RasterizerState::scissorEnable
			if (mRasterizerState.scissorEnable)
			{
				glEnable(GL_SCISSOR_TEST);
			}
			else
			{
				glDisable(GL_SCISSOR_TEST);
			}

			// RasterizerState::multisampleEnable

			// RasterizerState::antialiasedLineEnable
			if (mRasterizerState.antialiasedLineEnable)
			{
				glEnable(GL_LINE_SMOOTH);
			}
			else
			{
				glDisable(GL_LINE_SMOOTH);
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::RasterizerState mRasterizerState;		///< Rasterizer state
		GLenum					  mOpenGLFrontFaceMode;	///< OpenGL front face mode


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/DepthStencilState.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL depth stencil state class
	*/
	class DepthStencilState final : public IState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] depthStencilState
		*    Depth stencil state to use
		*/
		inline explicit DepthStencilState(const Renderer::DepthStencilState& depthStencilState) :
			mDepthStencilState(depthStencilState),
			mOpenGLDepthMaskEnabled(static_cast<GLboolean>((Renderer::DepthWriteMask::ALL == mDepthStencilState.depthWriteMask) ? GL_TRUE : GL_FALSE)),
			mOpenGLDepthFunc(Mapping::getOpenGLComparisonFunc(depthStencilState.depthFunc))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~DepthStencilState()
		{}

		/**
		*  @brief
		*    Return the depth stencil state
		*
		*  @return
		*    The depth stencil state
		*/
		[[nodiscard]] inline const Renderer::DepthStencilState& getDepthStencilState() const
		{
			return mDepthStencilState;
		}

		/**
		*  @brief
		*    Set the OpenGL depth stencil states
		*/
		void setOpenGLDepthStencilStates() const
		{
			// Renderer::DepthStencilState::depthEnable
			if (mDepthStencilState.depthEnable)
			{
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				glDisable(GL_DEPTH_TEST);
			}

			// Renderer::DepthStencilState::depthWriteMask
			glDepthMask(mOpenGLDepthMaskEnabled);

			// Renderer::DepthStencilState::depthFunc
			glDepthFunc(static_cast<GLenum>(mOpenGLDepthFunc));

			// TODO(co) Map the rest of the depth stencil states
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::DepthStencilState mDepthStencilState;			///< Depth stencil state
		GLboolean					mOpenGLDepthMaskEnabled;	///< OpenGL depth mask enabled state
		GLenum						mOpenGLDepthFunc;			///< OpenGL depth function


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/BlendState.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL blend state class
	*/
	class BlendState final : public IState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] blendState
		*    Blend state to use
		*/
		inline explicit BlendState(const Renderer::BlendState& blendState) :
			mBlendState(blendState),
			mOpenGLSrcBlend(Mapping::getOpenGLBlendType(mBlendState.renderTarget[0].srcBlend)),
			mOpenGLDstBlend(Mapping::getOpenGLBlendType(mBlendState.renderTarget[0].destBlend))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~BlendState()
		{}

		/**
		*  @brief
		*    Return the blend state
		*
		*  @return
		*    The blend state
		*/
		[[nodiscard]] inline const Renderer::BlendState& getBlendState() const
		{
			return mBlendState;
		}

		/**
		*  @brief
		*    Set the OpenGL blend states
		*/
		void setOpenGLBlendStates() const
		{
			// "GL_ARB_multisample"-extension
			if (mBlendState.alphaToCoverageEnable)
			{
				glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
			}
			else
			{
				glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
			}

			// TODO(co) Add support for blend state per render target
			if (mBlendState.renderTarget[0].blendEnable)
			{
				glEnable(GL_BLEND);
				glBlendFunc(mOpenGLSrcBlend, mOpenGLDstBlend);
			}
			else
			{
				glDisable(GL_BLEND);
			}

			// TODO(co) Map the rest of the blend states
			// GL_EXT_blend_func_separate
			// (GL_EXT_blend_equation_separate)
			// GL_EXT_blend_color
			// GL_EXT_blend_minmax
			// GL_EXT_blend_subtract
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::BlendState mBlendState;		///< Blend state
		GLenum				 mOpenGLSrcBlend;	///< OpenGL source blend function
		GLenum				 mOpenGLDstBlend;	///< OpenGL destination blend function


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/RenderTarget/RenderPass.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL render pass interface
	*/
	class RenderPass final : public Renderer::IRenderPass
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] numberOfColorAttachments
		*    Number of color render target textures, must be <="Renderer::Capabilities::maximumNumberOfSimultaneousRenderTargets"
		*  @param[in] colorAttachmentTextureFormats
		*    The color render target texture formats, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "numberOfColorAttachments" textures in the provided C-array of pointers
		*  @param[in] depthStencilAttachmentTextureFormat
		*    The optional depth stencil render target texture format, can be a "Renderer::TextureFormat::UNKNOWN" if there should be no depth buffer
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		RenderPass(Renderer::IRenderer& renderer, uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples) :
			IRenderPass(renderer),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat),
			mNumberOfMultisamples(numberOfMultisamples)
		{
			RENDERER_ASSERT(renderer.getContext(), mNumberOfColorAttachments < 8, "Invalid number of OpenGL color attachments")
			memcpy(mColorAttachmentTextureFormats, colorAttachmentTextureFormats, sizeof(Renderer::TextureFormat::Enum) * mNumberOfColorAttachments);
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~RenderPass() override
		{}

		/**
		*  @brief
		*    Return the number of color render target textures
		*
		*  @return
		*    The number of color render target textures
		*/
		[[nodiscard]] inline uint32_t getNumberOfColorAttachments() const
		{
			return mNumberOfColorAttachments;
		}

		/**
		*  @brief
		*    Return the number of render target textures (color and depth stencil)
		*
		*  @return
		*    The number of render target textures (color and depth stencil)
		*/
		[[nodiscard]] inline uint32_t getNumberOfAttachments() const
		{
			return (mDepthStencilAttachmentTextureFormat != Renderer::TextureFormat::Enum::UNKNOWN) ? (mNumberOfColorAttachments + 1) : mNumberOfColorAttachments;
		}

		/**
		*  @brief
		*    Return the color attachment texture format
		*
		*  @param[in] colorAttachmentIndex
		*    Color attachment index
		*
		*  @return
		*    The color attachment texture format
		*/
		[[nodiscard]] inline Renderer::TextureFormat::Enum getColorAttachmentTextureFormat(uint32_t colorAttachmentIndex) const
		{
			RENDERER_ASSERT(getRenderer().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid OpenGL color attachment index")
			return mColorAttachmentTextureFormats[colorAttachmentIndex];
		}

		/**
		*  @brief
		*    Return the depth stencil attachment texture format
		*
		*  @return
		*    The depth stencil attachment texture format
		*/
		[[nodiscard]] inline Renderer::TextureFormat::Enum getDepthStencilAttachmentTextureFormat() const
		{
			return mDepthStencilAttachmentTextureFormat;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), RenderPass, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RenderPass(const RenderPass& source) = delete;
		RenderPass& operator =(const RenderPass& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t					  mNumberOfColorAttachments;
		Renderer::TextureFormat::Enum mColorAttachmentTextureFormats[8];
		Renderer::TextureFormat::Enum mDepthStencilAttachmentTextureFormat;
		uint8_t						  mNumberOfMultisamples;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/QueryPool.h                            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL asynchronous query pool base class
	*/
	class QueryPool : public Renderer::IQueryPool
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		inline QueryPool(OpenGLRenderer& openGLRenderer, Renderer::QueryType queryType, uint32_t numberOfQueries) :
			IQueryPool(openGLRenderer),
			mQueryType(queryType),
			mNumberOfQueries(numberOfQueries)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~QueryPool() override
		{}

		/**
		*  @brief
		*    Return the query type
		*
		*  @return
		*    The query type
		*/
		[[nodiscard]] inline Renderer::QueryType getQueryType() const
		{
			return mQueryType;
		}

		/**
		*  @brief
		*    Return the number of queries
		*
		*  @return
		*    The number of queries
		*/
		[[nodiscard]] inline uint32_t getNumberOfQueries() const
		{
			return mNumberOfQueries;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit QueryPool(const QueryPool& source) = delete;
		QueryPool& operator =(const QueryPool& source) = delete;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		Renderer::QueryType mQueryType;
		uint32_t			mNumberOfQueries;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/OcclusionTimestampQueryPool.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL asynchronous occlusion and timestamp query pool interface
	*/
	class OcclusionTimestampQueryPool final : public QueryPool
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		OcclusionTimestampQueryPool(OpenGLRenderer& openGLRenderer, Renderer::QueryType queryType, uint32_t numberOfQueries) :
			QueryPool(openGLRenderer, queryType, numberOfQueries),
			mOpenGLQueries(RENDERER_MALLOC_TYPED(openGLRenderer.getContext(), GLuint, numberOfQueries))
		{
			// If possible, use "glCreateQueries()" (OpenGL 4.5) in order to create the query instance at once
			if (nullptr != glCreateQueries)
			{
				switch (queryType)
				{
					case Renderer::QueryType::OCCLUSION:
						glCreateQueries(GL_SAMPLES_PASSED_ARB, static_cast<GLsizei>(numberOfQueries), mOpenGLQueries);
						break;

					case Renderer::QueryType::PIPELINE_STATISTICS:
						RENDERER_ASSERT(openGLRenderer.getContext(), false, "Use \"OpenGLRenderer::PipelineStatisticsQueryPool\"")
						break;

					case Renderer::QueryType::TIMESTAMP:
						glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(numberOfQueries), mOpenGLQueries);
						break;
				}
			}
			else
			{
				glGenQueriesARB(static_cast<GLsizei>(numberOfQueries), mOpenGLQueries);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				switch (queryType)
				{
					case Renderer::QueryType::OCCLUSION:
						setDebugName("Occlusion query");
						break;

					case Renderer::QueryType::PIPELINE_STATISTICS:
						RENDERER_ASSERT(openGLRenderer.getContext(), false, "Use \"OpenGLRenderer::PipelineStatisticsQueryPool\"")
						break;

					case Renderer::QueryType::TIMESTAMP:
						setDebugName("Timestamp query");
						break;
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~OcclusionTimestampQueryPool() override
		{
			// Destroy the OpenGL queries
			// -> Silently ignores 0's and names that do not correspond to existing queries
			glDeleteQueriesARB(static_cast<GLsizei>(mNumberOfQueries), mOpenGLQueries);
			RENDERER_FREE(getRenderer().getContext(), mOpenGLQueries);
		}

		/**
		*  @brief
		*    Return the OpenGL queries
		*
		*  @return
		*    The OpenGL queries
		*/
		[[nodiscard]] inline GLuint* getOpenGLQueries() const
		{
			return mOpenGLQueries;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// "glCreateQueries()" (OpenGL 4.5) as well as "GL_KHR_debug"-extension available?
				if (nullptr != glCreateQueries && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					for (uint32_t i = 0; i < mNumberOfQueries; ++i)
					{
						glObjectLabel(GL_QUERY, mOpenGLQueries[i], -1, name);
					}
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), OcclusionTimestampQueryPool, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit OcclusionTimestampQueryPool(const OcclusionTimestampQueryPool& source) = delete;
		OcclusionTimestampQueryPool& operator =(const OcclusionTimestampQueryPool& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint* mOpenGLQueries;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/PipelineStatisticsQueryPool.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL asynchronous pipeline statistics query pool interface
	*/
	class PipelineStatisticsQueryPool final : public QueryPool
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		PipelineStatisticsQueryPool(OpenGLRenderer& openGLRenderer, Renderer::QueryType queryType, uint32_t numberOfQueries) :
			QueryPool(openGLRenderer, queryType, numberOfQueries),
			mVerticesSubmittedOpenGLQueries(RENDERER_MALLOC_TYPED(openGLRenderer.getContext(), GLuint, numberOfQueries * 11)),
			mPrimitivesSubmittedOpenGLQueries(mVerticesSubmittedOpenGLQueries + numberOfQueries),
			mVertexShaderInvocationsOpenGLQueries(mPrimitivesSubmittedOpenGLQueries + numberOfQueries),
			mGeometryShaderInvocationsOpenGLQueries(mVertexShaderInvocationsOpenGLQueries + numberOfQueries),
			mGeometryShaderPrimitivesEmittedOpenGLQueries(mGeometryShaderInvocationsOpenGLQueries + numberOfQueries),
			mClippingInputPrimitivesOpenGLQueries(mGeometryShaderPrimitivesEmittedOpenGLQueries + numberOfQueries),
			mClippingOutputPrimitivesOpenGLQueries(mClippingInputPrimitivesOpenGLQueries + numberOfQueries),
			mFragmentShaderInvocationsOpenGLQueries(mClippingOutputPrimitivesOpenGLQueries + numberOfQueries),
			mTessControlShaderPatchesOpenGLQueries(mFragmentShaderInvocationsOpenGLQueries + numberOfQueries),
			mTesEvaluationShaderInvocationsOpenGLQueries(mTessControlShaderPatchesOpenGLQueries + numberOfQueries),
			mComputeShaderInvocationsOpenGLQueries(mTesEvaluationShaderInvocationsOpenGLQueries + numberOfQueries)
		{
			// "glCreateQueries()" (OpenGL 4.5) doesn't support "GL_ARB_pipeline_statistics_query"
			glGenQueriesARB(static_cast<GLsizei>(numberOfQueries) * 11, mVerticesSubmittedOpenGLQueries);

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				switch (queryType)
				{
					case Renderer::QueryType::OCCLUSION:
					case Renderer::QueryType::TIMESTAMP:
						RENDERER_ASSERT(openGLRenderer.getContext(), false, "Use \"OpenGLRenderer::OcclusionTimestampQueryPool\"")
						break;

					case Renderer::QueryType::PIPELINE_STATISTICS:
						// Enforce instant query creation so we can set a debug name
						for (uint32_t i = 0; i  < numberOfQueries; ++i)
						{
							beginQuery(i);
							endQuery();
						}

						// Set debug name
						setDebugName("Pipeline statistics query");
						break;
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~PipelineStatisticsQueryPool() override
		{
			// Destroy the OpenGL queries
			// -> Silently ignores 0's and names that do not correspond to existing queries
			glDeleteQueriesARB(static_cast<GLsizei>(mNumberOfQueries) * 11, mVerticesSubmittedOpenGLQueries);
			RENDERER_FREE(getRenderer().getContext(), mVerticesSubmittedOpenGLQueries);
		}

		void beginQuery(uint32_t queryIndex) const
		{
			glBeginQueryARB(GL_VERTICES_SUBMITTED_ARB, mVerticesSubmittedOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_PRIMITIVES_SUBMITTED_ARB, mPrimitivesSubmittedOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_VERTEX_SHADER_INVOCATIONS_ARB, mVertexShaderInvocationsOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_GEOMETRY_SHADER_INVOCATIONS, mGeometryShaderInvocationsOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB, mGeometryShaderPrimitivesEmittedOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_CLIPPING_INPUT_PRIMITIVES_ARB, mClippingInputPrimitivesOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB, mClippingOutputPrimitivesOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, mFragmentShaderInvocationsOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_TESS_CONTROL_SHADER_PATCHES_ARB, mTessControlShaderPatchesOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB, mTesEvaluationShaderInvocationsOpenGLQueries[queryIndex]);
			glBeginQueryARB(GL_COMPUTE_SHADER_INVOCATIONS_ARB, mComputeShaderInvocationsOpenGLQueries[queryIndex]);
		}

		void endQuery() const
		{
			glEndQueryARB(GL_VERTICES_SUBMITTED_ARB);
			glEndQueryARB(GL_PRIMITIVES_SUBMITTED_ARB);
			glEndQueryARB(GL_VERTEX_SHADER_INVOCATIONS_ARB);
			glEndQueryARB(GL_GEOMETRY_SHADER_INVOCATIONS);
			glEndQueryARB(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB);
			glEndQueryARB(GL_CLIPPING_INPUT_PRIMITIVES_ARB);
			glEndQueryARB(GL_CLIPPING_OUTPUT_PRIMITIVES_ARB);
			glEndQueryARB(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
			glEndQueryARB(GL_TESS_CONTROL_SHADER_PATCHES_ARB);
			glEndQueryARB(GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB);
			glEndQueryARB(GL_COMPUTE_SHADER_INVOCATIONS_ARB);
		}

		bool getQueryPoolResults(uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, uint32_t strideInBytes, bool waitForResult) const
		{
			bool resultAvailable = true;

			// Define a helper macro
			#define GET_QUERY_RESULT(openGLQueries, queryResult) \
				resultAvailable = getQueryPoolResult(openGLQueries[firstQueryIndex + i], waitForResult, currentPipelineStatisticsQueryResult->queryResult); \
				if (!resultAvailable) \
				{ \
					break; \
				}

			// Get query pool results
			Renderer::PipelineStatisticsQueryResult* currentPipelineStatisticsQueryResult = reinterpret_cast<Renderer::PipelineStatisticsQueryResult*>(data);
			for (uint32_t i = 0; i  < numberOfQueries; ++i)
			{
				GET_QUERY_RESULT(mVerticesSubmittedOpenGLQueries, numberOfInputAssemblerVertices)
				GET_QUERY_RESULT(mPrimitivesSubmittedOpenGLQueries, numberOfInputAssemblerPrimitives)
				GET_QUERY_RESULT(mVertexShaderInvocationsOpenGLQueries, numberOfVertexShaderInvocations)
				GET_QUERY_RESULT(mGeometryShaderInvocationsOpenGLQueries, numberOfGeometryShaderInvocations)
				GET_QUERY_RESULT(mGeometryShaderPrimitivesEmittedOpenGLQueries, numberOfGeometryShaderOutputPrimitives)
				GET_QUERY_RESULT(mClippingInputPrimitivesOpenGLQueries, numberOfClippingInputPrimitives)
				GET_QUERY_RESULT(mClippingOutputPrimitivesOpenGLQueries, numberOfClippingOutputPrimitives)
				GET_QUERY_RESULT(mFragmentShaderInvocationsOpenGLQueries, numberOfFragmentShaderInvocations)
				GET_QUERY_RESULT(mTessControlShaderPatchesOpenGLQueries, numberOfTessellationControlShaderInvocations)
				GET_QUERY_RESULT(mTesEvaluationShaderInvocationsOpenGLQueries, numberOfTessellationEvaluationShaderInvocations)
				GET_QUERY_RESULT(mComputeShaderInvocationsOpenGLQueries, numberOfComputeShaderInvocations)
				currentPipelineStatisticsQueryResult += strideInBytes;
			}

			// Undefine helper macro
			#undef GET_QUERY_RESULT

			// Done
			return resultAvailable;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName([[maybe_unused]] const char* name) override
			{
				// "GL_KHR_debug"-extension available?
				if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					for (uint32_t i = 0; i < mNumberOfQueries * 11; ++i)
					{
						glObjectLabel(GL_QUERY, mVerticesSubmittedOpenGLQueries[i], -1, name);
					}
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), PipelineStatisticsQueryPool, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit PipelineStatisticsQueryPool(const PipelineStatisticsQueryPool& source) = delete;
		PipelineStatisticsQueryPool& operator =(const PipelineStatisticsQueryPool& source) = delete;

		bool getQueryPoolResult(GLuint openGLQuery, bool waitForResult, uint64_t& queryResult) const
		{
			bool resultAvailable = true;
			GLuint openGLQueryResult = GL_FALSE;
			do
			{
				glGetQueryObjectuivARB(openGLQuery, GL_QUERY_RESULT_AVAILABLE_ARB, &openGLQueryResult);
			}
			while (waitForResult && GL_TRUE != openGLQueryResult);
			if (GL_TRUE == openGLQueryResult)
			{
				glGetQueryObjectuivARB(openGLQuery, GL_QUERY_RESULT_ARB, &openGLQueryResult);
				queryResult = static_cast<uint64_t>(openGLQueryResult);
			}
			else
			{
				// Result not ready
				resultAvailable = false;
			}

			// Done
			return resultAvailable;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint* mVerticesSubmittedOpenGLQueries;				///< "GL_VERTICES_SUBMITTED_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfInputAssemblerVertices"
		GLuint* mPrimitivesSubmittedOpenGLQueries;				///< "GL_PRIMITIVES_SUBMITTED_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfInputAssemblerPrimitives"
		GLuint* mVertexShaderInvocationsOpenGLQueries;			///< "GL_VERTEX_SHADER_INVOCATIONS_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfVertexShaderInvocations"
		GLuint* mGeometryShaderInvocationsOpenGLQueries;		///< "GL_GEOMETRY_SHADER_INVOCATIONS", "Renderer::PipelineStatisticsQueryResult::numberOfGeometryShaderInvocations"
		GLuint* mGeometryShaderPrimitivesEmittedOpenGLQueries;	///< "GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfGeometryShaderOutputPrimitives"
		GLuint* mClippingInputPrimitivesOpenGLQueries;			///< "GL_CLIPPING_INPUT_PRIMITIVES_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfClippingInputPrimitives"
		GLuint* mClippingOutputPrimitivesOpenGLQueries;			///< "GL_CLIPPING_OUTPUT_PRIMITIVES_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfClippingOutputPrimitives"
		GLuint* mFragmentShaderInvocationsOpenGLQueries;		///< "GL_FRAGMENT_SHADER_INVOCATIONS_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfFragmentShaderInvocations"
		GLuint* mTessControlShaderPatchesOpenGLQueries;			///< "GL_TESS_CONTROL_SHADER_PATCHES_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfTessellationControlShaderInvocations"
		GLuint* mTesEvaluationShaderInvocationsOpenGLQueries;	///< "GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfTessellationEvaluationShaderInvocations"
		GLuint* mComputeShaderInvocationsOpenGLQueries;			///< "GL_COMPUTE_SHADER_INVOCATIONS_ARB", "Renderer::PipelineStatisticsQueryResult::numberOfComputeShaderInvocations"


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/RenderTarget/SwapChain.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL swap chain class
	*/
	class SwapChain final : public Renderer::ISwapChain
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] windowHandle
		*    Information about the window to render into
		*  @param[in] useExternalContext
		*    Indicates if an external renderer context is used; in this case the renderer itself has nothing to do with the creation/managing of an renderer context
		*/
		SwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, [[maybe_unused]] bool useExternalContext) :
			ISwapChain(renderPass),
			mNativeWindowHandle(windowHandle.nativeWindowHandle),
			#ifdef _WIN32
				mOpenGLContext(RENDERER_NEW(renderPass.getRenderer().getContext(), OpenGLContextWindows)(static_cast<const RenderPass&>(renderPass).getDepthStencilAttachmentTextureFormat(), windowHandle.nativeWindowHandle, static_cast<const OpenGLContextWindows*>(&static_cast<OpenGLRenderer&>(renderPass.getRenderer()).getOpenGLContext()))),
			#elif defined LINUX
				mOpenGLContext(RENDERER_NEW(renderPass.getRenderer().getContext(), OpenGLContextLinux)(static_cast<OpenGLRenderer&>(renderPass.getRenderer()), static_cast<const RenderPass&>(renderPass).getDepthStencilAttachmentTextureFormat(), windowHandle.nativeWindowHandle, useExternalContext, static_cast<const OpenGLContextLinux*>(&static_cast<OpenGLRenderer&>(renderPass.getRenderer()).getOpenGLContext()))),
			#else
				#error "Unsupported platform"
			#endif
			mOwnsOpenGLContext(true),
			mRenderWindow(windowHandle.renderWindow),
			mVerticalSynchronizationInterval(0),
			mNewVerticalSynchronizationInterval(0)	// 0 instead of ~0u to ensure that we always set the swap interval at least once to have a known initial setting
		{}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SwapChain() override
		{
			if (mOwnsOpenGLContext)
			{
				RENDERER_DELETE(getRenderer().getContext(), IOpenGLContext, mOpenGLContext);
			}
		}

		/**
		*  @brief
		*    Return the OpenGL context
		*
		*  @return
		*    The OpenGL context
		*/
		[[nodiscard]] inline IOpenGLContext& getOpenGLContext() const
		{
			return *mOpenGLContext;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// Return stored width and height when both valid
			if (nullptr != mRenderWindow)
			{
				mRenderWindow->getWidthAndHeight(width, height);
				return;
			}
			#ifdef _WIN32
				// Is there a valid native OS window?
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					// Get the width and height
					long swapChainWidth  = 1;
					long swapChainHeight = 1;
					{
						// Get the client rectangle of the native output window
						// -> Don't use the width and height stored in "DXGI_SWAP_CHAIN_DESC" -> "DXGI_MODE_DESC"
						//    because it might have been modified in order to avoid zero values
						RECT rect;
						::GetClientRect(reinterpret_cast<HWND>(mNativeWindowHandle), &rect);

						// Get the width and height...
						swapChainWidth  = rect.right  - rect.left;
						swapChainHeight = rect.bottom - rect.top;

						// ... and ensure that none of them is ever zero
						if (swapChainWidth < 1)
						{
							swapChainWidth = 1;
						}
						if (swapChainHeight < 1)
						{
							swapChainHeight = 1;
						}
					}

					// Write out the width and height
					width  = static_cast<UINT>(swapChainWidth);
					height = static_cast<UINT>(swapChainHeight);
				}
				else
			#elif defined LINUX
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
					Display* display = static_cast<const OpenGLContextLinux&>(openGLRenderer.getOpenGLContext()).getDisplay();

					// Get the width and height...
					::Window rootWindow = 0;
					int positionX = 0, positionY = 0;
					unsigned int unsignedWidth = 0, unsignedHeight = 0, border = 0, depth = 0;
					if (nullptr != display)
					{
						XGetGeometry(display, mNativeWindowHandle, &rootWindow, &positionX, &positionY, &unsignedWidth, &unsignedHeight, &border, &depth);
					}

					// ... and ensure that none of them is ever zero
					if (unsignedWidth < 1)
					{
						unsignedWidth = 1;
					}
					if (unsignedHeight < 1)
					{
						unsignedHeight = 1;
					}

					// Done
					width = unsignedWidth;
					height = unsignedHeight;
				}
				else
			#else
				#error "Unsupported platform"
			#endif
			{
				// Set known default return values
				width  = 1;
				height = 1;
			}
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ISwapChain methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Renderer::handle getNativeWindowHandle() const override
		{
			return mNativeWindowHandle;
		}

		inline virtual void setVerticalSynchronizationInterval(uint32_t synchronizationInterval) override
		{
			mNewVerticalSynchronizationInterval = synchronizationInterval;
		}

		virtual void present() override
		{
			if (nullptr != mRenderWindow)
			{
				mRenderWindow->present();
				return;
			}
			#ifdef _WIN32
			{
				// Set new vertical synchronization interval?
				// -> We do this in here to avoid having to use "wglMakeCurrent()"/"glXMakeCurrent()" to often at multiple places
				if (~0u != mNewVerticalSynchronizationInterval)
				{
					const Extensions& extensions = static_cast<OpenGLRenderer&>(getRenderer()).getExtensions();
					if (extensions.isWGL_EXT_swap_control())
					{
						// Use adaptive vertical synchronization if possible
						wglSwapIntervalEXT((extensions.isWGL_EXT_swap_control_tear() && mNewVerticalSynchronizationInterval > 0) ? -static_cast<int>(mNewVerticalSynchronizationInterval) : static_cast<int>(mNewVerticalSynchronizationInterval));
					}
					mVerticalSynchronizationInterval = mNewVerticalSynchronizationInterval;
					mNewVerticalSynchronizationInterval = ~0u;
				}

				// Swap buffers
				::SwapBuffers(static_cast<OpenGLContextWindows*>(mOpenGLContext)->getDeviceContext());
				if (mVerticalSynchronizationInterval > 0)
				{
					glFinish();
				}
			}
			#elif defined LINUX
				// TODO(co) Add support for vertical synchronization and adaptive vertical synchronization: "GLX_EXT_swap_control" and "GLX_EXT_swap_control_tear"
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
					glXSwapBuffers(static_cast<const OpenGLContextLinux&>(openGLRenderer.getOpenGLContext()).getDisplay(), mNativeWindowHandle);
				}
			#else
				#error "Unsupported platform"
			#endif
		}

		inline virtual void resizeBuffers() override
		{
			// Nothing here
		}

		[[nodiscard]] inline virtual bool getFullscreenState() const override
		{
			// TODO(co) Implement me
			return false;
		}

		inline virtual void setFullscreenState(bool) override
		{
			// TODO(co) Implement me
		}

		inline virtual void setRenderWindow(Renderer::IRenderWindow* renderWindow) override
		{
			mRenderWindow = renderWindow;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SwapChain, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SwapChain(const SwapChain& source) = delete;
		SwapChain& operator =(const SwapChain& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::handle		 mNativeWindowHandle;	///< Native window handle window, can be a null handle
		IOpenGLContext*			 mOpenGLContext;		///< OpenGL context, must be valid
		bool					 mOwnsOpenGLContext;	///< Does this swap chain own the OpenGL context?
		Renderer::IRenderWindow* mRenderWindow;			///< Render window instance, can be a null pointer, don't destroy the instance since we don't own it
		uint32_t				 mVerticalSynchronizationInterval;
		uint32_t				 mNewVerticalSynchronizationInterval;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/RenderTarget/Framebuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL framebuffer interface
	*/
	class Framebuffer : public Renderer::IFramebuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			// Destroy the OpenGL framebuffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteFramebuffers(1, &mOpenGLFramebuffer);

			// Release the reference to the used color textures
			if (nullptr != mColorTextures)
			{
				// Release references
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture)
				{
					(*colorTexture)->releaseReference();
				}

				// Cleanup
				RENDERER_FREE(getRenderer().getContext(), mColorTextures);
			}

			// Release the reference to the used depth stencil texture
			if (nullptr != mDepthStencilTexture)
			{
				// Release reference
				mDepthStencilTexture->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the OpenGL framebuffer
		*
		*  @return
		*    The OpenGL framebuffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLFramebuffer() const
		{
			return mOpenGLFramebuffer;
		}

		/**
		*  @brief
		*    Return the number of color render target textures
		*
		*  @return
		*    The number of color render target textures
		*/
		[[nodiscard]] inline uint32_t getNumberOfColorTextures() const
		{
			return mNumberOfColorTextures;
		}

		/**
		*  @brief
		*    Return whether or not the framebuffer is a multisample render target
		*
		*  @return
		*    "true" if the framebuffer is a multisample render target, else "false"
		*/
		[[nodiscard]] inline bool isMultisampleRenderTarget() const
		{
			return mMultisampleRenderTarget;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL framebuffer and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLFramebuffer && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_FRAMEBUFFER, mOpenGLFramebuffer, -1, name);
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLFramebuffer));
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		inline virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// No fancy implementation in here, just copy over the internal information
			width  = mWidth;
			height = mHeight;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Framebuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] colorFramebufferAttachments
		*    The color render target textures, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "Renderer::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		Framebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment) :
			IFramebuffer(renderPass),
			mOpenGLFramebuffer(0),
			mNumberOfColorTextures(static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments()),
			mColorTextures(nullptr),	// Set below
			mDepthStencilTexture(nullptr),
			mWidth(UINT_MAX),
			mHeight(UINT_MAX),
			mMultisampleRenderTarget(false)
		{
			// The "GL_ARB_framebuffer_object"-extension documentation says the following about the framebuffer width and height
			//   "If the attachment sizes are not all identical, rendering will be limited to the largest area that can fit in
			//    all of the attachments (i.e. an intersection of rectangles having a lower left of (0,0) and an upper right of
			//    (width,height) for each attachment)"

			// Add a reference to the used color textures
			if (mNumberOfColorTextures > 0)
			{
				mColorTextures = RENDERER_MALLOC_TYPED(renderPass.getRenderer().getContext(), Renderer::ITexture*, mNumberOfColorTextures);

				// Loop through all color textures
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments)
				{
					// Sanity check
					RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid OpenGL color framebuffer attachment texture")

					// TODO(co) Add security check: Is the given resource one of the currently used renderer?
					*colorTexture = colorFramebufferAttachments->texture;
					(*colorTexture)->addReference();

					// Evaluate the color texture type
					switch ((*colorTexture)->getResourceType())
					{
						case Renderer::ResourceType::TEXTURE_2D:
						{
							const Texture2D* texture2D = static_cast<Texture2D*>(*colorTexture);

							// Sanity checks
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL color framebuffer attachment mipmap index")
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid OpenGL color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
							break;
						}

						case Renderer::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);
							break;
						}

						case Renderer::ResourceType::ROOT_SIGNATURE:
						case Renderer::ResourceType::RESOURCE_GROUP:
						case Renderer::ResourceType::GRAPHICS_PROGRAM:
						case Renderer::ResourceType::VERTEX_ARRAY:
						case Renderer::ResourceType::RENDER_PASS:
						case Renderer::ResourceType::QUERY_POOL:
						case Renderer::ResourceType::SWAP_CHAIN:
						case Renderer::ResourceType::FRAMEBUFFER:
						case Renderer::ResourceType::INDEX_BUFFER:
						case Renderer::ResourceType::VERTEX_BUFFER:
						case Renderer::ResourceType::TEXTURE_BUFFER:
						case Renderer::ResourceType::STRUCTURED_BUFFER:
						case Renderer::ResourceType::INDIRECT_BUFFER:
						case Renderer::ResourceType::UNIFORM_BUFFER:
						case Renderer::ResourceType::TEXTURE_1D:
						case Renderer::ResourceType::TEXTURE_1D_ARRAY:
						case Renderer::ResourceType::TEXTURE_3D:
						case Renderer::ResourceType::TEXTURE_CUBE:
						case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
						case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
						case Renderer::ResourceType::SAMPLER_STATE:
						case Renderer::ResourceType::VERTEX_SHADER:
						case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
						case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
						case Renderer::ResourceType::GEOMETRY_SHADER:
						case Renderer::ResourceType::FRAGMENT_SHADER:
						case Renderer::ResourceType::COMPUTE_SHADER:
						default:
							// Nothing here
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != mDepthStencilTexture, "Invalid OpenGL depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						// Nothing here
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid OpenGL framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid OpenGL framebuffer height")
				mHeight = 1;
			}
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint				 mOpenGLFramebuffer;		///< OpenGL framebuffer ("container" object, not shared between OpenGL contexts), can be zero if no resource is allocated
		uint32_t			 mNumberOfColorTextures;	///< Number of color render target textures
		Renderer::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Renderer::ITexture*  mDepthStencilTexture;		///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t			 mWidth;					///< The framebuffer width
		uint32_t			 mHeight;					///< The framebuffer height
		bool				 mMultisampleRenderTarget;	///< Multisample render target?


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Framebuffer(const Framebuffer& source) = delete;
		Framebuffer& operator =(const Framebuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/RenderTarget/FramebufferBind.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL framebuffer class, traditional bind version
	*/
	class FramebufferBind final : public Framebuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] colorFramebufferAttachments
		*    The color render target textures, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "Renderer::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		FramebufferBind(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment) :
			Framebuffer(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment)
		{
			// Texture reference handling is done within the base class "Framebuffer"
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(renderPass.getRenderer());

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL framebuffer
				GLint openGLFramebufferBackup = 0;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &openGLFramebufferBackup);
			#endif

			// Create the OpenGL framebuffer
			glGenFramebuffers(1, &mOpenGLFramebuffer);

			// Bind this OpenGL framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, mOpenGLFramebuffer);

			// Loop through all framebuffer color attachments
			const Renderer::FramebufferAttachment* colorFramebufferAttachment    = colorFramebufferAttachments;
			const Renderer::FramebufferAttachment* colorFramebufferAttachmentEnd = colorFramebufferAttachments + mNumberOfColorTextures;
			for (GLenum openGLAttachment = GL_COLOR_ATTACHMENT0; colorFramebufferAttachment < colorFramebufferAttachmentEnd; ++colorFramebufferAttachment, ++openGLAttachment)
			{
				Renderer::ITexture* texture = colorFramebufferAttachment->texture;

				// Security check: Is the given resource owned by this renderer?
				#ifdef RENDERER_DEBUG
					if (&openGLRenderer != &texture->getRenderer())
					{
						// Output an error message and keep on going in order to keep a reasonable behaviour even in case on an error
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The given color texture at index %u is owned by another renderer instance", colorFramebufferAttachment - colorFramebufferAttachments)

						// Continue, there's no point in trying to do any error handling in here
						continue;
					}
				#endif

				// Evaluate the color texture type
				switch (texture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						// Set the OpenGL framebuffer color attachment
						const Texture2D* texture2D = static_cast<const Texture2D*>(texture);
						glFramebufferTexture2D(GL_FRAMEBUFFER, openGLAttachment, static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex));
						if (!mMultisampleRenderTarget && texture2D->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Set the OpenGL framebuffer color attachment
						const Texture2DArray* texture2DArray = static_cast<const Texture2DArray*>(texture);
						glFramebufferTextureLayer(GL_FRAMEBUFFER, openGLAttachment, texture2DArray->getOpenGLTexture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex), static_cast<GLint>(colorFramebufferAttachment->layerIndex));
						if (!mMultisampleRenderTarget && texture2DArray->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "The type of the given color texture at index %ld is not supported by the OpenGL renderer backend", colorFramebufferAttachment - colorFramebufferAttachments)
						break;
				}
			}

			// Depth stencil texture
			if (nullptr != mDepthStencilTexture)
			{
				// Security check: Is the given resource owned by this renderer?
				#ifdef RENDERER_DEBUG
					if (&openGLRenderer != &mDepthStencilTexture->getRenderer())
					{
						// Output an error message and keep on going in order to keep a reasonable behaviour even in case on an error
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The given depth stencil texture is owned by another renderer instance")
					}
				#endif

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<const Texture2D*>(mDepthStencilTexture);

						// Sanity check
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL depth stencil framebuffer attachment layer index")

						// Bind the depth stencil texture to framebuffer
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex));
						if (!mMultisampleRenderTarget && texture2D->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Bind the depth stencil texture to framebuffer
						const Texture2DArray* texture2DArray = static_cast<const Texture2DArray*>(mDepthStencilTexture);
						glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture2DArray->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex), static_cast<GLint>(depthStencilFramebufferAttachment->layerIndex));
						if (!mMultisampleRenderTarget && texture2DArray->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The type of the given depth stencil texture is not supported by the OpenGL renderer backend")
						break;
				}
			}

			// Check the status of the OpenGL framebuffer
			const GLenum openGLStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			switch (openGLStatus)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Not all framebuffer attachment points are framebuffer attachment complete (\"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: No images are attached to the framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete draw buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete read buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete multisample framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\")")
					break;

				case GL_FRAMEBUFFER_UNDEFINED:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Undefined framebuffer (\"GL_FRAMEBUFFER_UNDEFINED\")")
					break;

				case GL_FRAMEBUFFER_UNSUPPORTED:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The combination of internal formats of the attached images violates an implementation-dependent set of restrictions (\"GL_FRAMEBUFFER_UNSUPPORTED\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Not all attached images have the same width and height (\"GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete formats framebuffer object (\"GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\")")
					break;

				default:
				case GL_FRAMEBUFFER_COMPLETE:
					// Nothing here
					break;
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(openGLFramebufferBackup));
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FramebufferBind() override
		{
			// Texture reference handling is done within the base class "Framebuffer"
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FramebufferBind(const FramebufferBind& source) = delete;
		FramebufferBind& operator =(const FramebufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/RenderTarget/FramebufferDsa.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL framebuffer class, effective direct state access (DSA)
	*/
	class FramebufferDsa final : public Framebuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] colorFramebufferAttachments
		*    The color render target textures, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "Renderer::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		FramebufferDsa(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment) :
			Framebuffer(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment)
		{
			// Texture reference handling is done within the base class "Framebuffer"
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(renderPass.getRenderer());
			const bool isArbDsa = openGLRenderer.getExtensions().isGL_ARB_direct_state_access();

			// Create the OpenGL framebuffer
			if (isArbDsa)
			{
				glCreateFramebuffers(1, &mOpenGLFramebuffer);
			}
			else
			{
				glGenFramebuffers(1, &mOpenGLFramebuffer);
			}

			// Loop through all framebuffer color attachments
			const Renderer::FramebufferAttachment* colorFramebufferAttachment    = colorFramebufferAttachments;
			const Renderer::FramebufferAttachment* colorFramebufferAttachmentEnd = colorFramebufferAttachments + mNumberOfColorTextures;
			for (GLenum openGLAttachment = GL_COLOR_ATTACHMENT0; colorFramebufferAttachment < colorFramebufferAttachmentEnd; ++colorFramebufferAttachment, ++openGLAttachment)
			{
				Renderer::ITexture* texture = colorFramebufferAttachment->texture;

				// Security check: Is the given resource owned by this renderer?
				#ifdef RENDERER_DEBUG
					if (&openGLRenderer != &texture->getRenderer())
					{
						// Output an error message and keep on going in order to keep a reasonable behaviour even in case on an error
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The given color texture at index %u is owned by another renderer instance", colorFramebufferAttachment - colorFramebufferAttachments)

						// Continue, there's no point in trying to do any error handling in here
						continue;
					}
				#endif

				// Evaluate the color texture type
				switch (texture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						// Set the OpenGL framebuffer color attachment
						const Texture2D* texture2D = static_cast<const Texture2D*>(texture);
						if (isArbDsa)
						{
							glNamedFramebufferTexture(mOpenGLFramebuffer, openGLAttachment, texture2D->getOpenGLTexture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex));
						}
						else
						{
							glNamedFramebufferTexture2DEXT(mOpenGLFramebuffer, openGLAttachment, static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex));
						}
						if (!mMultisampleRenderTarget && texture2D->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Set the OpenGL framebuffer color attachment
						const Texture2DArray* texture2DArray = static_cast<const Texture2DArray*>(texture);
						if (isArbDsa)
						{
							glNamedFramebufferTextureLayer(mOpenGLFramebuffer, openGLAttachment, texture2DArray->getOpenGLTexture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex),  static_cast<GLint>(colorFramebufferAttachment->layerIndex));
						}
						else
						{
							glNamedFramebufferTextureLayerEXT(mOpenGLFramebuffer, openGLAttachment, texture2DArray->getOpenGLTexture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex), static_cast<GLint>(colorFramebufferAttachment->layerIndex));
						}
						if (!mMultisampleRenderTarget && texture2DArray->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "The type of the given color texture at index %ld is not supported by the OpenGL renderer backend", colorFramebufferAttachment - colorFramebufferAttachments)
						break;
				}
			}

			// Depth stencil texture
			if (nullptr != mDepthStencilTexture)
			{
				// Security check: Is the given resource owned by this renderer?
				#ifdef RENDERER_DEBUG
					if (&openGLRenderer != &mDepthStencilTexture->getRenderer())
					{
						// Output an error message and keep on going in order to keep a reasonable behaviour even in case on an error
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The given depth stencil texture is owned by another renderer instance")
					}
				#endif

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<const Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL depth stencil framebuffer attachment layer index")

						// Bind the depth stencil texture to framebuffer
						if (isArbDsa)
						{
							glNamedFramebufferTexture(mOpenGLFramebuffer, GL_DEPTH_ATTACHMENT, texture2D->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex));
						}
						else
						{
							glNamedFramebufferTexture2DEXT(mOpenGLFramebuffer, GL_DEPTH_ATTACHMENT, static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex));
						}
						if (!mMultisampleRenderTarget && texture2D->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Bind the depth stencil texture to framebuffer
						const Texture2DArray* texture2DArray = static_cast<const Texture2DArray*>(mDepthStencilTexture);
						if (isArbDsa)
						{
							glNamedFramebufferTextureLayer(mOpenGLFramebuffer, GL_DEPTH_ATTACHMENT, texture2DArray->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex), static_cast<GLint>(depthStencilFramebufferAttachment->layerIndex));
						}
						else
						{
							glNamedFramebufferTextureLayerEXT(mOpenGLFramebuffer, GL_DEPTH_ATTACHMENT, texture2DArray->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex), static_cast<GLint>(depthStencilFramebufferAttachment->layerIndex));
						}
						if (!mMultisampleRenderTarget && texture2DArray->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "The type of the given depth stencil texture is not supported by the OpenGL renderer backend")
						break;
				}
			}

			// Check the status of the OpenGL framebuffer
			const GLenum openGLStatus = isArbDsa ? glCheckNamedFramebufferStatus(mOpenGLFramebuffer, GL_FRAMEBUFFER) : glCheckNamedFramebufferStatusEXT(mOpenGLFramebuffer, GL_FRAMEBUFFER);
			switch (openGLStatus)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Not all framebuffer attachment points are framebuffer attachment complete (\"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: No images are attached to the framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete draw buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete read buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete multisample framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\")")
					break;

				case GL_FRAMEBUFFER_UNDEFINED:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Undefined framebuffer (\"GL_FRAMEBUFFER_UNDEFINED\")")
					break;

				case GL_FRAMEBUFFER_UNSUPPORTED:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: The combination of internal formats of the attached images violates an implementation-dependent set of restrictions (\"GL_FRAMEBUFFER_UNSUPPORTED\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Not all attached images have the same width and height (\"GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
					RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "OpenGL error: Incomplete formats framebuffer object (\"GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\")")
					break;

				default:
				case GL_FRAMEBUFFER_COMPLETE:
					// Nothing here
					break;
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FramebufferDsa() override
		{
			// Texture reference handling is done within the base class "Framebuffer"
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FramebufferDsa(const FramebufferDsa& source) = delete;
		FramebufferDsa& operator =(const FramebufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/VertexShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic vertex shader class
	*/
	class VertexShaderMonolithic final : public Renderer::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline VertexShaderMonolithic(OpenGLRenderer& openGLRenderer, const char* sourceCode) :
			IVertexShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRenderer.getContext(), GL_VERTEX_SHADER_ARB, sourceCode))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexShaderMonolithic() override
		{
			// Destroy the OpenGL shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLShader);
		}

		/**
		*  @brief
		*    Return the OpenGL shader
		*
		*  @return
		*    The OpenGL shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShader() const
		{
			return mOpenGLShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShader && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShaderMonolithic(const VertexShaderMonolithic& source) = delete;
		VertexShaderMonolithic& operator =(const VertexShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/TessellationControlShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderMonolithic final : public Renderer::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationControlShaderMonolithic(OpenGLRenderer& openGLRenderer, const char* sourceCode) :
			ITessellationControlShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRenderer.getContext(), GL_TESS_CONTROL_SHADER, sourceCode))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationControlShaderMonolithic() override
		{
			// Destroy the OpenGL shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLShader);
		}

		/**
		*  @brief
		*    Return the OpenGL shader
		*
		*  @return
		*    The OpenGL shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShader() const
		{
			return mOpenGLShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShader && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationControlShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShaderMonolithic(const TessellationControlShaderMonolithic& source) = delete;
		TessellationControlShaderMonolithic& operator =(const TessellationControlShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/TessellationEvaluationShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderMonolithic final : public Renderer::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationEvaluationShaderMonolithic(OpenGLRenderer& openGLRenderer, const char* sourceCode) :
			ITessellationEvaluationShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRenderer.getContext(), GL_TESS_EVALUATION_SHADER, sourceCode))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationEvaluationShaderMonolithic() override
		{
			// Destroy the OpenGL shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLShader);
		}

		/**
		*  @brief
		*    Return the OpenGL shader
		*
		*  @return
		*    The OpenGL shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShader() const
		{
			return mOpenGLShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShader && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationEvaluationShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShaderMonolithic(const TessellationEvaluationShaderMonolithic& source) = delete;
		TessellationEvaluationShaderMonolithic& operator =(const TessellationEvaluationShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/GeometryShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic geometry shader class
	*/
	class GeometryShaderMonolithic final : public Renderer::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		inline GeometryShaderMonolithic(OpenGLRenderer& openGLRenderer, const char* sourceCode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices) :
			IGeometryShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRenderer.getContext(), GL_GEOMETRY_SHADER_ARB, sourceCode)),
			mOpenGLGsInputPrimitiveTopology(static_cast<int>(gsInputPrimitiveTopology)),	// The "Renderer::GsInputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			mOpenGLGsOutputPrimitiveTopology(static_cast<int>(gsOutputPrimitiveTopology)),	// The "Renderer::GsOutputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			mNumberOfOutputVertices(numberOfOutputVertices)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GeometryShaderMonolithic() override
		{
			// Destroy the OpenGL shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLShader);
		}

		/**
		*  @brief
		*    Return the OpenGL shader
		*
		*  @return
		*    The OpenGL shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShader() const
		{
			return mOpenGLShader;
		}

		/**
		*  @brief
		*    Return the OpenGL geometry shader input primitive topology
		*
		*  @return
		*    The OpenGL geometry shader input primitive topology
		*/
		[[nodiscard]] inline GLint getOpenGLGsInputPrimitiveTopology() const
		{
			return mOpenGLGsInputPrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the OpenGL geometry shader output primitive topology
		*
		*  @return
		*    The OpenGL geometry shader output primitive topology
		*/
		[[nodiscard]] inline GLint getOpenGLGsOutputPrimitiveTopology() const
		{
			return mOpenGLGsOutputPrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the number of output vertices
		*
		*  @return
		*    The number of output vertices
		*/
		[[nodiscard]] inline uint32_t getNumberOfOutputVertices() const
		{
			return mNumberOfOutputVertices;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShader && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GeometryShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShaderMonolithic(const GeometryShaderMonolithic& source) = delete;
		GeometryShaderMonolithic& operator =(const GeometryShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint	 mOpenGLShader;						///< OpenGL shader, can be zero if no resource is allocated
		GLint	 mOpenGLGsInputPrimitiveTopology;	///< OpenGL geometry shader input primitive topology
		GLint	 mOpenGLGsOutputPrimitiveTopology;	///< OpenGL geometry shader output primitive topology
		uint32_t mNumberOfOutputVertices;			///< Number of output vertices


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/FragmentShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic fragment shader (FS, "pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderMonolithic final : public Renderer::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline FragmentShaderMonolithic(OpenGLRenderer& openGLRenderer, const char* sourceCode) :
			IFragmentShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRenderer.getContext(), GL_FRAGMENT_SHADER_ARB, sourceCode))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FragmentShaderMonolithic() override
		{
			// Destroy the OpenGL shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLShader);
		}

		/**
		*  @brief
		*    Return the OpenGL shader
		*
		*  @return
		*    The OpenGL shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShader() const
		{
			return mOpenGLShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShader && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), FragmentShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShaderMonolithic(const FragmentShaderMonolithic& source) = delete;
		FragmentShaderMonolithic& operator =(const FragmentShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/ComputeShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic compute shader (CS) class
	*/
	class ComputeShaderMonolithic final : public Renderer::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline ComputeShaderMonolithic(OpenGLRenderer& openGLRenderer, const char* sourceCode) :
			IComputeShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRenderer.getContext(), GL_COMPUTE_SHADER, sourceCode))
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ComputeShaderMonolithic() override
		{
			// Destroy the OpenGL shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLShader);
		}

		/**
		*  @brief
		*    Return the OpenGL shader
		*
		*  @return
		*    The OpenGL shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShader() const
		{
			return mOpenGLShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShader && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputeShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputeShaderMonolithic(const ComputeShaderMonolithic& source) = delete;
		ComputeShaderMonolithic& operator =(const ComputeShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/GraphicsProgramMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic graphics program class
	*/
	class GraphicsProgramMonolithic : public Renderer::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShaderMonolithic
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderMonolithic
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderMonolithic
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderMonolithic
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderMonolithic
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramMonolithic(OpenGLRenderer& openGLRenderer, const Renderer::IRootSignature& rootSignature, const Renderer::VertexAttributes& vertexAttributes, VertexShaderMonolithic* vertexShaderMonolithic, TessellationControlShaderMonolithic* tessellationControlShaderMonolithic, TessellationEvaluationShaderMonolithic* tessellationEvaluationShaderMonolithic, GeometryShaderMonolithic* geometryShaderMonolithic, FragmentShaderMonolithic* fragmentShaderMonolithic) :
			IGraphicsProgram(openGLRenderer),
			mOpenGLProgram(glCreateProgram()),
			mDrawIdUniformLocation(-1)
		{
			// Attach the shaders to the program
			// -> We don't need to keep a reference to the shader, to add and release at once to ensure a nice behaviour
			if (nullptr != vertexShaderMonolithic)
			{
				vertexShaderMonolithic->addReference();
				glAttachShader(mOpenGLProgram, vertexShaderMonolithic->getOpenGLShader());
				vertexShaderMonolithic->releaseReference();
			}
			if (nullptr != tessellationControlShaderMonolithic)
			{
				tessellationControlShaderMonolithic->addReference();
				glAttachShader(mOpenGLProgram, tessellationControlShaderMonolithic->getOpenGLShader());
				tessellationControlShaderMonolithic->releaseReference();
			}
			if (nullptr != tessellationEvaluationShaderMonolithic)
			{
				tessellationEvaluationShaderMonolithic->addReference();
				glAttachShader(mOpenGLProgram, tessellationEvaluationShaderMonolithic->getOpenGLShader());
				tessellationEvaluationShaderMonolithic->releaseReference();
			}
			if (nullptr != geometryShaderMonolithic)
			{
				// Add a reference to the shader
				geometryShaderMonolithic->addReference();

				// Attach the monolithic shader to the monolithic program
				glAttachShader(mOpenGLProgram, geometryShaderMonolithic->getOpenGLShader());

				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions

				// Set the OpenGL geometry shader input primitive topology
				glProgramParameteriARB(mOpenGLProgram, GL_GEOMETRY_INPUT_TYPE_ARB, geometryShaderMonolithic->getOpenGLGsInputPrimitiveTopology());

				// Set the OpenGL geometry shader output primitive topology
				glProgramParameteriARB(mOpenGLProgram, GL_GEOMETRY_OUTPUT_TYPE_ARB, geometryShaderMonolithic->getOpenGLGsOutputPrimitiveTopology());

				// Set the number of output vertices
				glProgramParameteriARB(mOpenGLProgram, GL_GEOMETRY_VERTICES_OUT_ARB, static_cast<GLint>(geometryShaderMonolithic->getNumberOfOutputVertices()));

				// Release the shader
				geometryShaderMonolithic->releaseReference();
			}
			if (nullptr != fragmentShaderMonolithic)
			{
				fragmentShaderMonolithic->addReference();
				glAttachShader(mOpenGLProgram, fragmentShaderMonolithic->getOpenGLShader());
				fragmentShaderMonolithic->releaseReference();
			}

			{ // Define the vertex array attribute binding locations ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 & 12 terminology)
				const uint32_t numberOfVertexAttributes = vertexAttributes.numberOfAttributes;
				for (uint32_t vertexAttribute = 0; vertexAttribute < numberOfVertexAttributes; ++vertexAttribute)
				{
					glBindAttribLocation(mOpenGLProgram, vertexAttribute, vertexAttributes.attributes[vertexAttribute].name);
				}
			}

			// Link the program
			glLinkProgram(mOpenGLProgram);

			// Check the link status
			GLint linked = GL_FALSE;
			glGetProgramiv(mOpenGLProgram, GL_LINK_STATUS, &linked);
			if (GL_TRUE == linked)
			{
				// We're not using "glBindFragDataLocation()", else the user would have to provide us with additional OpenGL-only specific information
				// -> Use modern GLSL:
				//    "layout(location = 0) out vec4 ColorOutput0;"
				//    "layout(location = 1) out vec4 ColorOutput1;"
				// -> Use legacy GLSL if necessary:
				//    "gl_FragData[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);"
				//    "gl_FragData[1] = vec4(0.0f, 0.0f, 1.0f, 0.0f);"

				// Get draw ID uniform location
				if (!openGLRenderer.getExtensions().isGL_ARB_base_instance())
				{
					mDrawIdUniformLocation = glGetUniformLocation(mOpenGLProgram, "drawIdUniform");
				}

				// The actual locations assigned to uniform variables are not known until the program object is linked successfully
				// -> So we have to build a root signature parameter index -> uniform location mapping here
				const Renderer::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Renderer::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_ASSERT(openGLRenderer.getContext(), nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Renderer::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								// Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension,
								// for backward compatibility, ask for the uniform block index
								const GLuint uniformBlockIndex = glGetUniformBlockIndex(mOpenGLProgram, descriptorRange.baseShaderRegisterName);
								if (GL_INVALID_INDEX != uniformBlockIndex)
								{
									// Associate the uniform block with the given binding point
									glUniformBlockBinding(mOpenGLProgram, uniformBlockIndex, uniformBlockBindingIndex);
									++uniformBlockBindingIndex;
								}
							}
							else if (Renderer::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								const GLint uniformLocation = glGetUniformLocation(mOpenGLProgram, descriptorRange.baseShaderRegisterName);
								if (uniformLocation >= 0)
								{
									// OpenGL/GLSL is not automatically assigning texture units to samplers, so, we have to take over this job
									// -> When using OpenGL or OpenGL ES 3 this is required
									// -> OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension supports explicit binding points ("layout(binding = 0)"
									//    in GLSL shader) , for backward compatibility we don't use it in here
									// -> When using Direct3D 9, 10, 11 or 12, the texture unit
									//    to use is usually defined directly within the shader by using the "register"-keyword
									// -> Use the "GL_ARB_direct_state_access" or "GL_EXT_direct_state_access" extension if possible to not change OpenGL states
									if (nullptr != glProgramUniform1i)
									{
										glProgramUniform1i(mOpenGLProgram, uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
									}
									else if (nullptr != glProgramUniform1iEXT)
									{
										glProgramUniform1iEXT(mOpenGLProgram, uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
									}
									else
									{
										// TODO(co) There's room for binding API call related optimization in here (will certainly be no huge overall efficiency gain)
										#ifdef RENDERER_OPENGL_STATE_CLEANUP
											// Backup the currently used OpenGL program
											GLint openGLProgramBackup = 0;
											glGetIntegerv(GL_CURRENT_PROGRAM, &openGLProgramBackup);
											if (static_cast<GLuint>(openGLProgramBackup) == mOpenGLProgram)
											{
												// Set uniform, please note that for this our program must be the currently used one
												glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
											}
											else
											{
												// Set uniform, please note that for this our program must be the currently used one
												glUseProgram(mOpenGLProgram);
												glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));

												// Be polite and restore the previous used OpenGL program
												glUseProgram(static_cast<GLhandleARB>(openGLProgramBackup));
											}
										#else
											glUseProgram(mOpenGLProgram);
											glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
										#endif
									}
								}
							}
						}
					}
				}
			}
			else
			{
				// Error, program link failed!

				// Get the length of the information (including a null termination)
				GLint informationLength = 0;
				glGetProgramiv(mOpenGLProgram, GL_INFO_LOG_LENGTH, &informationLength);
				if (informationLength > 1)
				{
					// Allocate memory for the information
					const Renderer::Context& context = openGLRenderer.getContext();
					char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramInfoLog(mOpenGLProgram, informationLength, nullptr, informationLog);

					// Output the debug string
					RENDERER_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RENDERER_FREE(context, informationLog);
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GraphicsProgramMonolithic() override
		{
			// Destroy the OpenGL program
			// -> A value of 0 for program will be silently ignored
			glDeleteShader(mOpenGLProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL program
		*
		*  @return
		*    The OpenGL program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLProgram() const
		{
			return mOpenGLProgram;
		}

		/**
		*  @brief
		*    Return the draw ID uniform location
		*
		*  @return
		*    Draw ID uniform location, -1 if there's no such uniform
		*/
		[[nodiscard]] inline GLint getDrawIdUniformLocation() const
		{
			return mDrawIdUniformLocation;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IGraphicsProgram methods     ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Renderer::handle getUniformHandle(const char* uniformName) override
		{
			return static_cast<Renderer::handle>(glGetUniformLocation(mOpenGLProgram, uniformName));
		}

		virtual void setUniform1i(Renderer::handle uniformHandle, int value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform1i(static_cast<GLint>(uniformHandle), value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniform1i(static_cast<GLint>(uniformHandle), value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniform1i(static_cast<GLint>(uniformHandle), value);
			#endif
		}

		virtual void setUniform1f(Renderer::handle uniformHandle, float value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform1f(static_cast<GLint>(uniformHandle), value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniform1f(static_cast<GLint>(uniformHandle), value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniform1f(static_cast<GLint>(uniformHandle), value);
			#endif
		}

		virtual void setUniform2fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		virtual void setUniform3fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		virtual void setUniform4fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		virtual void setUniformMatrix3fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			#endif
		}

		virtual void setUniformMatrix4fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program
				const GLhandleARB openGLProgramBackup = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
				if (openGLProgramBackup == mOpenGLProgram)
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLProgram);
					glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);

					// Be polite and restore the previous used OpenGL program
					glUseProgram(openGLProgramBackup);
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLProgram);
				glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			#endif
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GraphicsProgramMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLProgram;	///< OpenGL program, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramMonolithic(const GraphicsProgramMonolithic& source) = delete;
		GraphicsProgramMonolithic& operator =(const GraphicsProgramMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLint mDrawIdUniformLocation;	///< Draw ID uniform location, used for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/GraphicsProgramMonolithicDsa.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic graphics program class, effective direct state access (DSA)
	*/
	class GraphicsProgramMonolithicDsa final : public GraphicsProgramMonolithic
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShaderMonolithic
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderMonolithic
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderMonolithic
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderMonolithic
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderMonolithic
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		inline GraphicsProgramMonolithicDsa(OpenGLRenderer& openGLRenderer, const Renderer::IRootSignature& rootSignature, const Renderer::VertexAttributes& vertexAttributes, VertexShaderMonolithic* vertexShaderMonolithic, TessellationControlShaderMonolithic* tessellationControlShaderMonolithic, TessellationEvaluationShaderMonolithic* tessellationEvaluationShaderMonolithic, GeometryShaderMonolithic* geometryShaderMonolithic, FragmentShaderMonolithic* fragmentShaderMonolithic) :
			GraphicsProgramMonolithic(openGLRenderer, rootSignature, vertexAttributes, vertexShaderMonolithic, tessellationControlShaderMonolithic, tessellationEvaluationShaderMonolithic, geometryShaderMonolithic, fragmentShaderMonolithic)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GraphicsProgramMonolithicDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IGraphicsProgram methods     ]
	//[-------------------------------------------------------]
	public:
		virtual void setUniform1f(Renderer::handle uniformHandle, float value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform1f(mOpenGLProgram, static_cast<GLint>(uniformHandle), value);
			}
			else
			{
				glProgramUniform1fEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), value);
			}
		}

		virtual void setUniform2fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform2fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform2fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform3fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform3fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform3fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform4fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform4fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform4fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniformMatrix3fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniformMatrix3fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
			else
			{
				glProgramUniformMatrix3fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
		}

		virtual void setUniformMatrix4fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniformMatrix4fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
			else
			{
				glProgramUniformMatrix4fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramMonolithicDsa(const GraphicsProgramMonolithicDsa& source) = delete;
		GraphicsProgramMonolithicDsa& operator =(const GraphicsProgramMonolithicDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/ComputePipelineState.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract compute pipeline state base class
	*/
	class ComputePipelineState : public Renderer::IComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		inline explicit ComputePipelineState(OpenGLRenderer& openGLRenderer, uint16_t id) :
			IComputePipelineState(openGLRenderer, id)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ComputePipelineState() override
		{
			// Free the unique compact compute pipeline state ID
			static_cast<OpenGLRenderer&>(getRenderer()).ComputePipelineStateMakeId.DestroyID(getId());
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineState(const ComputePipelineState& source) = delete;
		ComputePipelineState& operator =(const ComputePipelineState& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/ComputePipelineStateMonolithic.h  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic compute pipeline state class
	*/
	class ComputePipelineStateMonolithic : public ComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] computeShaderMonolithic
		*    Compute shader the compute pipeline state is using
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*
		*  @note
		*    - The compute pipeline state keeps a reference to the provided compute shader and releases it when no longer required
		*/
		ComputePipelineStateMonolithic(OpenGLRenderer& openGLRenderer, const Renderer::IRootSignature& rootSignature, ComputeShaderMonolithic& computeShaderMonolithic, uint16_t id) :
			ComputePipelineState(openGLRenderer, id),
			mOpenGLProgram(glCreateProgram())
		{
			// Attach the compute shader to the program
			// -> We don't need to keep a reference to the shader, to add and release at once to ensure a nice behaviour
			computeShaderMonolithic.addReference();
			glAttachShader(mOpenGLProgram, computeShaderMonolithic.getOpenGLShader());
			computeShaderMonolithic.releaseReference();

			// Link the program
			glLinkProgram(mOpenGLProgram);

			// Check the link status
			GLint linked = GL_FALSE;
			glGetShaderiv(mOpenGLProgram, GL_LINK_STATUS, &linked);
			if (GL_TRUE == linked)
			{
				// We're not using "glBindFragDataLocation()", else the user would have to provide us with additional OpenGL-only specific information
				// -> Use modern GLSL:
				//    "layout(location = 0) out vec4 ColorOutput0;"
				//    "layout(location = 1) out vec4 ColorOutput1;"
				// -> Use legacy GLSL if necessary:
				//    "gl_FragData[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);"
				//    "gl_FragData[1] = vec4(0.0f, 0.0f, 1.0f, 0.0f);"

				// The actual locations assigned to uniform variables are not known until the program object is linked successfully
				// -> So we have to build a root signature parameter index -> uniform location mapping here
				const Renderer::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Renderer::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_ASSERT(openGLRenderer.getContext(), nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Renderer::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								// Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension,
								// for backward compatibility, ask for the uniform block index
								const GLuint uniformBlockIndex = glGetUniformBlockIndex(mOpenGLProgram, descriptorRange.baseShaderRegisterName);
								if (GL_INVALID_INDEX != uniformBlockIndex)
								{
									// Associate the uniform block with the given binding point
									glUniformBlockBinding(mOpenGLProgram, uniformBlockIndex, uniformBlockBindingIndex);
									++uniformBlockBindingIndex;
								}
							}
							else if (Renderer::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								const GLint uniformLocation = glGetUniformLocation(mOpenGLProgram, descriptorRange.baseShaderRegisterName);
								if (uniformLocation >= 0)
								{
									// OpenGL/GLSL is not automatically assigning texture units to samplers, so, we have to take over this job
									// -> When using OpenGL or OpenGL ES 3 this is required
									// -> OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension supports explicit binding points ("layout(binding = 0)"
									//    in GLSL shader) , for backward compatibility we don't use it in here
									// -> When using Direct3D 9, 10, 11 or 12, the texture unit
									//    to use is usually defined directly within the shader by using the "register"-keyword
									// -> Use the "GL_ARB_direct_state_access" or "GL_EXT_direct_state_access" extension if possible to not change OpenGL states
									if (nullptr != glProgramUniform1i)
									{
										glProgramUniform1i(mOpenGLProgram, uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
									}
									else if (nullptr != glProgramUniform1iEXT)
									{
										glProgramUniform1iEXT(mOpenGLProgram, uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
									}
									else
									{
										// TODO(co) There's room for binding API call related optimization in here (will certainly be no huge overall efficiency gain)
										#ifdef RENDERER_OPENGL_STATE_CLEANUP
											// Backup the currently used OpenGL program
											GLint openGLProgramBackup = 0;
											glGetIntegerv(GL_CURRENT_PROGRAM, &openGLProgramBackup);
											if (static_cast<GLuint>(openGLProgramBackup) == mOpenGLProgram)
											{
												// Set uniform, please note that for this our program must be the currently used one
												glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
											}
											else
											{
												// Set uniform, please note that for this our program must be the currently used one
												glUseProgram(mOpenGLProgram);
												glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));

												// Be polite and restore the previous used OpenGL program
												glUseProgram(static_cast<GLhandleARB>(openGLProgramBackup));
											}
										#else
											glUseProgram(mOpenGLProgram);
											glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
										#endif
									}
								}
							}
						}
					}
				}
			}
			else
			{
				// Error, program link failed!

				// Get the length of the information (including a null termination)
				GLint informationLength = 0;
				glGetProgramiv(mOpenGLProgram, GL_INFO_LOG_LENGTH, &informationLength);
				if (informationLength > 1)
				{
					// Allocate memory for the information
					const Renderer::Context& context = openGLRenderer.getContext();
					char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramInfoLog(mOpenGLProgram, informationLength, nullptr, informationLog);

					// Output the debug string
					RENDERER_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RENDERER_FREE(context, informationLog);
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ComputePipelineStateMonolithic() override
		{
			// Destroy the OpenGL program
			// -> A value of 0 for program will be silently ignored
			glDeleteShader(mOpenGLProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL program
		*
		*  @return
		*    The OpenGL program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLProgram() const
		{
			return mOpenGLProgram;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputePipelineStateMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLProgram;	///< OpenGL program, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineStateMonolithic(const ComputePipelineStateMonolithic& source) = delete;
		ComputePipelineStateMonolithic& operator =(const ComputePipelineStateMonolithic& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Monolithic/ShaderLanguageMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic shader language class
	*/
	class ShaderLanguageMonolithic final : public Renderer::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit ShaderLanguageMonolithic(OpenGLRenderer& openGLRenderer) :
			IShaderLanguage(openGLRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageMonolithic() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShaderLanguage methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}

		[[nodiscard]] inline virtual Renderer::IVertexShader* createVertexShaderFromBytecode(const Renderer::VertexAttributes&, const Renderer::ShaderBytecode&) override
		{
			// Error!
			RENDERER_ASSERT(getRenderer().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's vertex shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_ARB_vertex_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), VertexShaderMonolithic)(openGLRenderer, shaderSourceCode.sourceCode);
			}
			else
			{
				// Error! There's no vertex shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Renderer::ShaderBytecode&) override
		{
			// Error!
			RENDERER_ASSERT(getRenderer().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's tessellation support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_ARB_tessellation_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderMonolithic)(openGLRenderer, shaderSourceCode.sourceCode);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Renderer::ShaderBytecode&) override
		{
			// Error!
			RENDERER_ASSERT(getRenderer().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's tessellation support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_ARB_tessellation_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderMonolithic)(openGLRenderer, shaderSourceCode.sourceCode);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode(const Renderer::ShaderBytecode&, Renderer::GsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology, uint32_t) override
		{
			// Error!
			RENDERER_ASSERT(getRenderer().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's geometry shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_ARB_geometry_shader4())
			{
				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
				return RENDERER_NEW(getRenderer().getContext(), GeometryShaderMonolithic)(openGLRenderer, shaderSourceCode.sourceCode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices);
			}
			else
			{
				// Error! There's no geometry shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(const Renderer::ShaderBytecode&) override
		{
			// Error!
			RENDERER_ASSERT(getRenderer().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's fragment shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_ARB_fragment_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), FragmentShaderMonolithic)(openGLRenderer, shaderSourceCode.sourceCode);
			}
			else
			{
				// Error! There's no fragment shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromBytecode(const Renderer::ShaderBytecode&) override
		{
			// Error!
			RENDERER_ASSERT(getRenderer().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::IComputeShader* createComputeShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's compute shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_ARB_compute_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), ComputeShaderMonolithic)(openGLRenderer, shaderSourceCode.sourceCode);
			}
			else
			{
				// Error! There's no compute shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IGraphicsProgram* createGraphicsProgram(const Renderer::IRootSignature& rootSignature, const Renderer::VertexAttributes& vertexAttributes, Renderer::IVertexShader* vertexShader, Renderer::ITessellationControlShader* tessellationControlShader, Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL vertex shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL tessellation control shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL tessellation evaluation shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL geometry shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL fragment shader language mismatch")

			// Create the graphics program: Is "GL_EXT_direct_state_access" there?
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			if (openGLRenderer.getExtensions().isGL_EXT_direct_state_access() || openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramMonolithicDsa)(openGLRenderer, rootSignature, vertexAttributes, static_cast<VertexShaderMonolithic*>(vertexShader), static_cast<TessellationControlShaderMonolithic*>(tessellationControlShader), static_cast<TessellationEvaluationShaderMonolithic*>(tessellationEvaluationShader), static_cast<GeometryShaderMonolithic*>(geometryShader), static_cast<FragmentShaderMonolithic*>(fragmentShader));
			}
			else
			{
				// Traditional bind version
				return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramMonolithic)(openGLRenderer, rootSignature, vertexAttributes, static_cast<VertexShaderMonolithic*>(vertexShader), static_cast<TessellationControlShaderMonolithic*>(tessellationControlShader), static_cast<TessellationEvaluationShaderMonolithic*>(tessellationEvaluationShader), static_cast<GeometryShaderMonolithic*>(geometryShader), static_cast<FragmentShaderMonolithic*>(fragmentShader));
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ShaderLanguageMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageMonolithic(const ShaderLanguageMonolithic& source) = delete;
		ShaderLanguageMonolithic& operator =(const ShaderLanguageMonolithic& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/VertexShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate vertex shader class
	*/
	class VertexShaderSeparate final : public Renderer::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader bytecode
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline VertexShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderBytecode& shaderBytecode) :
			IVertexShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRenderer.getContext(), vertexAttributes, GL_VERTEX_SHADER_ARB, shaderBytecode)),
			mDrawIdUniformLocation(openGLRenderer.getExtensions().isGL_ARB_base_instance() ? -1 : glGetUniformLocation(mOpenGLShaderProgram, "drawIdUniform"))
		{}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline VertexShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::VertexAttributes& vertexAttributes, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IVertexShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourcecode(openGLRenderer.getContext(), vertexAttributes, GL_VERTEX_SHADER_ARB, sourceCode)),
			mDrawIdUniformLocation(openGLRenderer.getExtensions().isGL_ARB_base_instance() ? -1 : glGetUniformLocation(mOpenGLShaderProgram, "drawIdUniform"))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRenderer.getContext(), GL_VERTEX_SHADER_ARB, sourceCode, *shaderBytecode);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexShaderSeparate() override
		{
			// Destroy the OpenGL shader program
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteProgram(mOpenGLShaderProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL shader program
		*
		*  @return
		*    The OpenGL shader program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShaderProgram() const
		{
			return mOpenGLShaderProgram;
		}

		/**
		*  @brief
		*    Return the draw ID uniform location
		*
		*  @return
		*    Draw ID uniform location, -1 if there's no such uniform
		*/
		[[nodiscard]] inline GLint getDrawIdUniformLocation() const
		{
			return mDrawIdUniformLocation;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShaderProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShaderSeparate(const VertexShaderSeparate& source) = delete;
		VertexShaderSeparate& operator =(const VertexShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated
		GLint  mDrawIdUniformLocation;	///< Draw ID uniform location, used for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/TessellationControlShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderSeparate final : public Renderer::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline TessellationControlShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			ITessellationControlShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRenderer.getContext(), GL_TESS_CONTROL_SHADER, shaderBytecode))
		{}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationControlShaderSeparate(OpenGLRenderer& openGLRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			ITessellationControlShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRenderer.getContext(), GL_TESS_CONTROL_SHADER, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRenderer.getContext(), GL_TESS_CONTROL_SHADER, sourceCode, *shaderBytecode);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationControlShaderSeparate() override
		{
			// Destroy the OpenGL shader program
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteProgram(mOpenGLShaderProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL shader program
		*
		*  @return
		*    The OpenGL shader program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShaderProgram() const
		{
			return mOpenGLShaderProgram;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShaderProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationControlShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShaderSeparate(const TessellationControlShaderSeparate& source) = delete;
		TessellationControlShaderSeparate& operator =(const TessellationControlShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/TessellationEvaluationShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderSeparate final : public Renderer::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline TessellationEvaluationShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			ITessellationEvaluationShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRenderer.getContext(), GL_TESS_EVALUATION_SHADER, shaderBytecode))
		{}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationEvaluationShaderSeparate(OpenGLRenderer& openGLRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			ITessellationEvaluationShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRenderer.getContext(), GL_TESS_EVALUATION_SHADER, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRenderer.getContext(), GL_TESS_EVALUATION_SHADER, sourceCode, *shaderBytecode);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationEvaluationShaderSeparate() override
		{
			// Destroy the OpenGL shader program
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteProgram(mOpenGLShaderProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL shader program
		*
		*  @return
		*    The OpenGL shader program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShaderProgram() const
		{
			return mOpenGLShaderProgram;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShaderProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationEvaluationShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShaderSeparate(const TessellationEvaluationShaderSeparate& source) = delete;
		TessellationEvaluationShaderSeparate& operator =(const TessellationEvaluationShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/GeometryShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate geometry shader class
	*/
	class GeometryShaderSeparate final : public Renderer::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader bytecode
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		inline GeometryShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::ShaderBytecode& shaderBytecode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices) :
			IGeometryShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRenderer.getContext(), GL_GEOMETRY_SHADER_ARB, shaderBytecode))
		{}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		inline GeometryShaderSeparate(OpenGLRenderer& openGLRenderer, const char* sourceCode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IGeometryShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRenderer.getContext(), GL_GEOMETRY_SHADER_ARB, sourceCode))
		{
			// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
			//   "layout(triangles) in;"
			//   "layout(triangle_strip, max_vertices = 3) out;"
			// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
			glProgramParameteriARB(mOpenGLShaderProgram, GL_GEOMETRY_INPUT_TYPE_ARB, static_cast<int>(gsInputPrimitiveTopology));	// The "Renderer::GsInputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			glProgramParameteriARB(mOpenGLShaderProgram, GL_GEOMETRY_OUTPUT_TYPE_ARB, static_cast<int>(gsOutputPrimitiveTopology));	// The "Renderer::GsOutputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			glProgramParameteriARB(mOpenGLShaderProgram, GL_GEOMETRY_VERTICES_OUT_ARB, static_cast<GLint>(numberOfOutputVertices));

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRenderer.getContext(), GL_GEOMETRY_SHADER_ARB, sourceCode, *shaderBytecode);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GeometryShaderSeparate() override
		{
			// Destroy the OpenGL shader program
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteProgram(mOpenGLShaderProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL shader program
		*
		*  @return
		*    The OpenGL shader program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShaderProgram() const
		{
			return mOpenGLShaderProgram;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShaderProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GeometryShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShaderSeparate(const GeometryShaderSeparate& source) = delete;
		GeometryShaderSeparate& operator =(const GeometryShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/FragmentShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate fragment shader (FS, "pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderSeparate final : public Renderer::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader bytecode
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline FragmentShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IFragmentShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRenderer.getContext(), GL_FRAGMENT_SHADER_ARB, shaderBytecode))
		{}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline FragmentShaderSeparate(OpenGLRenderer& openGLRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IFragmentShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRenderer.getContext(), GL_FRAGMENT_SHADER_ARB, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRenderer.getContext(), GL_FRAGMENT_SHADER_ARB, sourceCode, *shaderBytecode);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FragmentShaderSeparate() override
		{
			// Destroy the OpenGL shader program
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteProgram(mOpenGLShaderProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL shader program
		*
		*  @return
		*    The OpenGL shader program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShaderProgram() const
		{
			return mOpenGLShaderProgram;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShaderProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), FragmentShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShaderSeparate(const FragmentShaderSeparate& source) = delete;
		FragmentShaderSeparate& operator =(const FragmentShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/ComputeShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate compute shader (CS) class
	*/
	class ComputeShaderSeparate final : public Renderer::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader bytecode
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline ComputeShaderSeparate(OpenGLRenderer& openGLRenderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IComputeShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRenderer.getContext(), GL_COMPUTE_SHADER, shaderBytecode))
		{}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline ComputeShaderSeparate(OpenGLRenderer& openGLRenderer, const char* sourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IComputeShader(static_cast<Renderer::IRenderer&>(openGLRenderer)),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRenderer.getContext(), GL_COMPUTE_SHADER, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRenderer.getContext(), GL_COMPUTE_SHADER, sourceCode, *shaderBytecode);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ComputeShaderSeparate() override
		{
			// Destroy the OpenGL shader program
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteProgram(mOpenGLShaderProgram);
		}

		/**
		*  @brief
		*    Return the OpenGL shader program
		*
		*  @return
		*    The OpenGL shader program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLShaderProgram() const
		{
			return mOpenGLShaderProgram;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL shader program and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLShaderProgram && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputeShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputeShaderSeparate(const ComputeShaderSeparate& source) = delete;
		ComputeShaderSeparate& operator =(const ComputeShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/GraphicsProgramSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate graphics program class
	*/
	class GraphicsProgramSeparate : public Renderer::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexShaderSeparate
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderSeparate
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderSeparate
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderSeparate
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderSeparate
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramSeparate(OpenGLRenderer& openGLRenderer, const Renderer::IRootSignature& rootSignature, VertexShaderSeparate* vertexShaderSeparate, TessellationControlShaderSeparate* tessellationControlShaderSeparate, TessellationEvaluationShaderSeparate* tessellationEvaluationShaderSeparate, GeometryShaderSeparate* geometryShaderSeparate, FragmentShaderSeparate* fragmentShaderSeparate) :
			IGraphicsProgram(openGLRenderer),
			mOpenGLProgramPipeline(0),
			mVertexShaderSeparate(vertexShaderSeparate),
			mTessellationControlShaderSeparate(tessellationControlShaderSeparate),
			mTessellationEvaluationShaderSeparate(tessellationEvaluationShaderSeparate),
			mGeometryShaderSeparate(geometryShaderSeparate),
			mFragmentShaderSeparate(fragmentShaderSeparate)
		{
			// Create the OpenGL program pipeline
			glGenProgramPipelines(1, &mOpenGLProgramPipeline);

			// If the "GL_ARB_direct_state_access" nor "GL_EXT_direct_state_access" extension is available, we need to change OpenGL states during resource creation (nasty thing)
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
			#endif
			if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
			{
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);
				#endif
				glBindProgramPipeline(mOpenGLProgramPipeline);
			}

			// Add references to the provided shaders
			#define USE_PROGRAM_STAGES(ShaderBit, ShaderSeparate) if (nullptr != ShaderSeparate) { ShaderSeparate->addReference(); glUseProgramStages(mOpenGLProgramPipeline, ShaderBit, ShaderSeparate->getOpenGLShaderProgram()); }
			USE_PROGRAM_STAGES(GL_VERTEX_SHADER_BIT,		  mVertexShaderSeparate)
			USE_PROGRAM_STAGES(GL_TESS_CONTROL_SHADER_BIT,	  mTessellationControlShaderSeparate)
			USE_PROGRAM_STAGES(GL_TESS_EVALUATION_SHADER_BIT, mTessellationEvaluationShaderSeparate)
			USE_PROGRAM_STAGES(GL_GEOMETRY_SHADER_BIT,		  mGeometryShaderSeparate)
			USE_PROGRAM_STAGES(GL_FRAGMENT_SHADER_BIT,		  mFragmentShaderSeparate)
			#undef USE_PROGRAM_STAGES

			// Validate program pipeline
			glValidateProgramPipeline(mOpenGLProgramPipeline);
			GLint validateStatus = 0;
			glGetProgramPipelineiv(mOpenGLProgramPipeline, GL_VALIDATE_STATUS, &validateStatus);
			if (GL_TRUE == validateStatus)
			{
				// We're not using "glBindFragDataLocation()", else the user would have to provide us with additional OpenGL-only specific information
				// -> Use modern GLSL:
				//    "layout(location = 0) out vec4 ColorOutput0;"
				//    "layout(location = 1) out vec4 ColorOutput1;"
				// -> Use legacy GLSL if necessary:
				//    "gl_FragData[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);"
				//    "gl_FragData[1] = vec4(0.0f, 0.0f, 1.0f, 0.0f);"

				// The actual locations assigned to uniform variables are not known until the program object is linked successfully
				// -> So we have to build a root signature parameter index -> uniform location mapping here
				const Renderer::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Renderer::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_ASSERT(openGLRenderer.getContext(), nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Renderer::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								#define BIND_UNIFORM_BLOCK(ShaderSeparate) if (nullptr != ShaderSeparate) ::detail::bindUniformBlock(descriptorRange, ShaderSeparate->getOpenGLShaderProgram(), uniformBlockBindingIndex);
								switch (descriptorRange.shaderVisibility)
								{
									case Renderer::ShaderVisibility::ALL:
									case Renderer::ShaderVisibility::ALL_GRAPHICS:
										BIND_UNIFORM_BLOCK(mVertexShaderSeparate)
										BIND_UNIFORM_BLOCK(mTessellationControlShaderSeparate)
										BIND_UNIFORM_BLOCK(mTessellationEvaluationShaderSeparate)
										BIND_UNIFORM_BLOCK(mGeometryShaderSeparate)
										BIND_UNIFORM_BLOCK(mFragmentShaderSeparate)
										break;

									case Renderer::ShaderVisibility::VERTEX:
										BIND_UNIFORM_BLOCK(mVertexShaderSeparate)
										break;

									case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
										BIND_UNIFORM_BLOCK(mTessellationControlShaderSeparate)
										break;

									case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
										BIND_UNIFORM_BLOCK(mTessellationEvaluationShaderSeparate)
										break;

									case Renderer::ShaderVisibility::GEOMETRY:
										BIND_UNIFORM_BLOCK(mGeometryShaderSeparate)
										break;

									case Renderer::ShaderVisibility::FRAGMENT:
										BIND_UNIFORM_BLOCK(mFragmentShaderSeparate)
										break;

									case Renderer::ShaderVisibility::COMPUTE:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL compute shader visibility")
										break;
								}
								#undef BIND_UNIFORM_BLOCK
								++uniformBlockBindingIndex;
							}
							else if (Renderer::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								#define BIND_UNIFORM_LOCATION(ShaderSeparate) if (nullptr != ShaderSeparate) ::detail::bindUniformLocation(descriptorRange, mOpenGLProgramPipeline, ShaderSeparate->getOpenGLShaderProgram());
								switch (descriptorRange.shaderVisibility)
								{
									case Renderer::ShaderVisibility::ALL:
									case Renderer::ShaderVisibility::ALL_GRAPHICS:
										BIND_UNIFORM_LOCATION(mVertexShaderSeparate)
										BIND_UNIFORM_LOCATION(mTessellationControlShaderSeparate)
										BIND_UNIFORM_LOCATION(mTessellationEvaluationShaderSeparate)
										BIND_UNIFORM_LOCATION(mGeometryShaderSeparate)
										BIND_UNIFORM_LOCATION(mFragmentShaderSeparate)
										break;

									case Renderer::ShaderVisibility::VERTEX:
										BIND_UNIFORM_LOCATION(mVertexShaderSeparate)
										break;

									case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
										BIND_UNIFORM_LOCATION(mTessellationControlShaderSeparate)
										break;

									case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
										BIND_UNIFORM_LOCATION(mTessellationEvaluationShaderSeparate)
										break;

									case Renderer::ShaderVisibility::GEOMETRY:
										BIND_UNIFORM_LOCATION(mGeometryShaderSeparate)
										break;

									case Renderer::ShaderVisibility::FRAGMENT:
										BIND_UNIFORM_LOCATION(mFragmentShaderSeparate)
										break;

									case Renderer::ShaderVisibility::COMPUTE:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL compute shader visibility")
										break;
								}
								#undef BIND_UNIFORM_LOCATION
							}
						}
					}
				}
			}
			else
			{
				// Error, program pipeline validation failed!

				// Get the length of the information (including a null termination)
				GLint informationLength = 0;
				glGetProgramPipelineiv(mOpenGLProgramPipeline, GL_INFO_LOG_LENGTH, &informationLength);
				if (informationLength > 1)
				{
					// Allocate memory for the information
					const Renderer::Context& context = openGLRenderer.getContext();
					char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramPipelineInfoLog(mOpenGLProgramPipeline, informationLength, nullptr, informationLog);

					// Output the debug string
					RENDERER_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RENDERER_FREE(context, informationLog);
				}
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous used OpenGL program pipeline
				if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
				{
					glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsProgramSeparate() override
		{
			// Destroy the OpenGL program pipeline
			glDeleteProgramPipelines(1, &mOpenGLProgramPipeline);

			// Release the shader references
			if (nullptr != mVertexShaderSeparate)
			{
				mVertexShaderSeparate->releaseReference();
			}
			if (nullptr != mTessellationControlShaderSeparate)
			{
				mTessellationControlShaderSeparate->releaseReference();
			}
			if (nullptr != mTessellationEvaluationShaderSeparate)
			{
				mTessellationEvaluationShaderSeparate->releaseReference();
			}
			if (nullptr != mGeometryShaderSeparate)
			{
				mGeometryShaderSeparate->releaseReference();
			}
			if (nullptr != mFragmentShaderSeparate)
			{
				mFragmentShaderSeparate->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the OpenGL program pipeline
		*
		*  @return
		*    The OpenGL program pipeline, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLProgramPipeline() const
		{
			return mOpenGLProgramPipeline;
		}

		/**
		*  @brief
		*    Return the vertex shader the program is using
		*
		*  @return
		*    Vertex shader the program is using, can be a null pointer, don't destroy the instance
		*/
		[[nodiscard]] inline VertexShaderSeparate* getVertexShaderSeparate() const
		{
			return mVertexShaderSeparate;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL program pipeline and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLProgramPipeline && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM_PIPELINE, mOpenGLProgramPipeline, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IGraphicsProgram methods     ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Renderer::handle getUniformHandle(const char* uniformName) override
		{
			GLint uniformLocation = -1;
			#define GET_UNIFORM_LOCATION(ShaderSeparate) if (uniformLocation < 0 && nullptr != ShaderSeparate) uniformLocation = glGetUniformLocation(ShaderSeparate->getOpenGLShaderProgram(), uniformName);
			GET_UNIFORM_LOCATION(mVertexShaderSeparate)
			GET_UNIFORM_LOCATION(mTessellationControlShaderSeparate)
			GET_UNIFORM_LOCATION(mTessellationEvaluationShaderSeparate)
			GET_UNIFORM_LOCATION(mGeometryShaderSeparate)
			GET_UNIFORM_LOCATION(mFragmentShaderSeparate)
			#undef GET_UNIFORM_LOCATION
			return static_cast<Renderer::handle>(uniformLocation);
		}

		virtual void setUniform1i(Renderer::handle uniformHandle, int value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform1i(static_cast<GLint>(uniformHandle), value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform1i(static_cast<GLint>(uniformHandle), value);
			#endif
		}

		virtual void setUniform1f(Renderer::handle uniformHandle, float value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform1f(static_cast<GLint>(uniformHandle), value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform1f(static_cast<GLint>(uniformHandle), value);
			#endif
		}

		virtual void setUniform2fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		virtual void setUniform3fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		virtual void setUniform4fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		virtual void setUniformMatrix3fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			#endif
		}

		virtual void setUniformMatrix4fv(Renderer::handle uniformHandle, const float* value) override
		{
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
				glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);

				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);

				// Be polite and restore the previous used OpenGL program pipeline
				glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glBindProgramPipeline(mOpenGLProgramPipeline);
				glActiveShaderProgram(mOpenGLProgramPipeline, mVertexShaderSeparate->getOpenGLShaderProgram());
				glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			#endif
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GraphicsProgramSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint								  mOpenGLProgramPipeline;					///< OpenGL program pipeline ("container" object, not shared between OpenGL contexts), can be zero if no resource is allocated
		VertexShaderSeparate*				  mVertexShaderSeparate;					///< Vertex shader the program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderSeparate*	  mTessellationControlShaderSeparate;		///< Tessellation control shader the program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderSeparate* mTessellationEvaluationShaderSeparate;	///< Tessellation evaluation shader the program is using (we keep a reference to it), can be a null pointer
		GeometryShaderSeparate*				  mGeometryShaderSeparate;					///< Geometry shader the program is using (we keep a reference to it), can be a null pointer
		FragmentShaderSeparate*				  mFragmentShaderSeparate;					///< Fragment shader the program is using (we keep a reference to it), can be a null pointer


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramSeparate(const GraphicsProgramSeparate& source) = delete;
		GraphicsProgramSeparate& operator =(const GraphicsProgramSeparate& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/GraphicsProgramSeparateDsa.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate graphics program class, effective direct state access (DSA)
	*/
	class GraphicsProgramSeparateDsa final : public GraphicsProgramSeparate
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexShaderSeparate
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderSeparate
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderSeparate
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderSeparate
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderSeparate
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		inline GraphicsProgramSeparateDsa(OpenGLRenderer& openGLRenderer, const Renderer::IRootSignature& rootSignature, VertexShaderSeparate* vertexShaderSeparate, TessellationControlShaderSeparate* tessellationControlShaderSeparate, TessellationEvaluationShaderSeparate* tessellationEvaluationShaderSeparate, GeometryShaderSeparate* geometryShaderSeparate, FragmentShaderSeparate* fragmentShaderSeparate) :
			GraphicsProgramSeparate(openGLRenderer, rootSignature, vertexShaderSeparate, tessellationControlShaderSeparate, tessellationEvaluationShaderSeparate, geometryShaderSeparate, fragmentShaderSeparate)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GraphicsProgramSeparateDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IGraphicsProgram methods     ]
	//[-------------------------------------------------------]
	public:
		virtual void setUniform1f(Renderer::handle uniformHandle, float value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform1f(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), value);
			}
			else
			{
				glProgramUniform1fEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), value);
			}
		}

		virtual void setUniform2fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform2fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform2fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform3fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform3fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform3fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform4fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform4fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform4fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniformMatrix3fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniformMatrix3fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
			else
			{
				glProgramUniformMatrix3fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
		}

		virtual void setUniformMatrix4fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniformMatrix4fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
			else
			{
				glProgramUniformMatrix4fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramSeparateDsa(const GraphicsProgramSeparateDsa& source) = delete;
		GraphicsProgramSeparateDsa& operator =(const GraphicsProgramSeparateDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/ComputePipelineStateSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate compute pipeline state class
	*/
	class ComputePipelineStateSeparate : public ComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] computeShaderSeparate
		*    Compute shader the compute pipeline state is using
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*
		*  @note
		*    - The compute pipeline state keeps a reference to the provided compute shader and releases it when no longer required
		*/
		ComputePipelineStateSeparate(OpenGLRenderer& openGLRenderer, const Renderer::IRootSignature& rootSignature, ComputeShaderSeparate& computeShaderSeparate, uint16_t id) :
			ComputePipelineState(openGLRenderer, id),
			mOpenGLProgramPipeline(0),
			mComputeShaderSeparate(computeShaderSeparate)
		{
			// Create the OpenGL program pipeline
			glGenProgramPipelines(1, &mOpenGLProgramPipeline);

			// If the "GL_ARB_direct_state_access" nor "GL_EXT_direct_state_access" extension is available, we need to change OpenGL states during resource creation (nasty thing)
			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
			#endif
			if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
			{
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);
				#endif
				glBindProgramPipeline(mOpenGLProgramPipeline);
			}

			// Add reference to the provided compute shader
			computeShaderSeparate.addReference();
			glUseProgramStages(mOpenGLProgramPipeline, GL_COMPUTE_SHADER_BIT, computeShaderSeparate.getOpenGLShaderProgram());

			// Validate program pipeline
			glValidateProgramPipeline(mOpenGLProgramPipeline);
			GLint validateStatus = 0;
			glGetProgramPipelineiv(mOpenGLProgramPipeline, GL_VALIDATE_STATUS, &validateStatus);
			if (true)	// TODO(co) Compute shader: Validate status always returns failure without log when using a compute shader? AMD 290X Radeon software version 18.7.1.
			//if (GL_TRUE == validateStatus)
			{
				// We're not using "glBindFragDataLocation()", else the user would have to provide us with additional OpenGL-only specific information
				// -> Use modern GLSL:
				//    "layout(location = 0) out vec4 ColorOutput0;"
				//    "layout(location = 1) out vec4 ColorOutput1;"
				// -> Use legacy GLSL if necessary:
				//    "gl_FragData[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);"
				//    "gl_FragData[1] = vec4(0.0f, 0.0f, 1.0f, 0.0f);"

				// The actual locations assigned to uniform variables are not known until the program object is linked successfully
				// -> So we have to build a root signature parameter index -> uniform location mapping here
				const Renderer::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Renderer::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_ASSERT(openGLRenderer.getContext(), nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Renderer::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								switch (descriptorRange.shaderVisibility)
								{
									case Renderer::ShaderVisibility::ALL_GRAPHICS:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL all graphics shader visibility")
										break;

									case Renderer::ShaderVisibility::VERTEX:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL vertex shader visibility")
										break;

									case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL tessellation control shader visibility")
										break;

									case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL tessellation evaluation shader visibility")
										break;

									case Renderer::ShaderVisibility::GEOMETRY:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL geometry shader visibility")
										break;

									case Renderer::ShaderVisibility::FRAGMENT:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL fragment shader visibility")
										break;

									case Renderer::ShaderVisibility::ALL:
									case Renderer::ShaderVisibility::COMPUTE:
										::detail::bindUniformBlock(descriptorRange, mComputeShaderSeparate.getOpenGLShaderProgram(), uniformBlockBindingIndex);
										break;
								}
								++uniformBlockBindingIndex;
							}
							else if (Renderer::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								switch (descriptorRange.shaderVisibility)
								{
									case Renderer::ShaderVisibility::ALL_GRAPHICS:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL all graphics shader visibility")
										break;

									case Renderer::ShaderVisibility::VERTEX:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL vertex shader visibility")
										break;

									case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL tessellation control shader visibility")
										break;

									case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL tessellation evaluation shader visibility")
										break;

									case Renderer::ShaderVisibility::GEOMETRY:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL geometry shader visibility")
										break;

									case Renderer::ShaderVisibility::FRAGMENT:
										RENDERER_LOG(openGLRenderer.getContext(), CRITICAL, "Invalid OpenGL fragment shader visibility")
										break;

									case Renderer::ShaderVisibility::ALL:
									case Renderer::ShaderVisibility::COMPUTE:
										::detail::bindUniformLocation(descriptorRange, mOpenGLProgramPipeline, mComputeShaderSeparate.getOpenGLShaderProgram());
										break;
								}
							}
						}
					}
				}
			}
			else
			{
				// Error, program pipeline validation failed!

				// Get the length of the information (including a null termination)
				GLint informationLength = 0;
				glGetProgramPipelineiv(mOpenGLProgramPipeline, GL_INFO_LOG_LENGTH, &informationLength);
				if (informationLength > 1)
				{
					// Allocate memory for the information
					const Renderer::Context& context = openGLRenderer.getContext();
					char* informationLog = RENDERER_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramPipelineInfoLog(mOpenGLProgramPipeline, informationLength, nullptr, informationLog);

					// Output the debug string
					RENDERER_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RENDERER_FREE(context, informationLog);
				}
			}

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous used OpenGL program pipeline
				if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
				{
					glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputePipelineStateSeparate() override
		{
			// Destroy the OpenGL program pipeline
			glDeleteProgramPipelines(1, &mOpenGLProgramPipeline);

			// Release the compute shader reference
			mComputeShaderSeparate.releaseReference();
		}

		/**
		*  @brief
		*    Return the OpenGL program pipeline
		*
		*  @return
		*    The OpenGL program pipeline, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLProgramPipeline() const
		{
			return mOpenGLProgramPipeline;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid OpenGL program pipeline and "GL_KHR_debug"-extension available?
				if (0 != mOpenGLProgramPipeline && static_cast<OpenGLRenderer&>(getRenderer()).getExtensions().isGL_KHR_debug())
				{
					glObjectLabel(GL_PROGRAM_PIPELINE, mOpenGLProgramPipeline, -1, name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputePipelineStateSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint				   mOpenGLProgramPipeline;	///< OpenGL program pipeline ("container" object, not shared between OpenGL contexts), can be zero if no resource is allocated
		ComputeShaderSeparate& mComputeShaderSeparate;	///< Compute shader the compute pipeline state is using (we keep a reference to it)


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineStateSeparate(const ComputePipelineStateSeparate& source) = delete;
		ComputePipelineStateSeparate& operator =(const ComputePipelineStateSeparate& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/Shader/Separate/ShaderLanguageSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate shader language class
	*/
	class ShaderLanguageSeparate final : public Renderer::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*/
		inline explicit ShaderLanguageSeparate(OpenGLRenderer& openGLRenderer) :
			IShaderLanguage(openGLRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ShaderLanguageSeparate() override
		{
			// De-initialize glslang, if necessary
			#ifdef RENDERER_OPENGL_GLSLTOSPIRV
				if (::detail::GlslangInitialized)
				{
					// TODO(co) Fix glslang related memory leaks. See also
					//		    - "Fix a few memory leaks #916" - https://github.com/KhronosGroup/glslang/pull/916
					//		    - "FreeGlobalPools is never called in glslang::FinalizeProcess()'s path. #928" - https://github.com/KhronosGroup/glslang/issues/928
					glslang::FinalizeProcess();
					::detail::GlslangInitialized = false;
				}
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShaderLanguage methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}

		[[nodiscard]] virtual Renderer::IVertexShader* createVertexShaderFromBytecode(const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// Check whether or not there's vertex shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_vertex_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RENDERER_NEW(getRenderer().getContext(), VertexShaderSeparate)(openGLRenderer, vertexAttributes, shaderBytecode);
			}
			else
			{
				// Error! There's no vertex shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IVertexShader* createVertexShaderFromSourceCode(const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's vertex shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_vertex_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), VertexShaderSeparate)(openGLRenderer, vertexAttributes, shaderSourceCode.sourceCode, extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr);
			}
			else
			{
				// Error! There's no vertex shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// Check whether or not there's tessellation support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderSeparate)(openGLRenderer, shaderBytecode);
			}
			else
			{
				// Error! There's no tessellation support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's tessellation support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderSeparate)(openGLRenderer, shaderSourceCode.sourceCode, extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// Check whether or not there's tessellation support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderSeparate)(openGLRenderer, shaderBytecode);
			}
			else
			{
				// Error! There's no tessellation support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's tessellation support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderSeparate)(openGLRenderer, shaderSourceCode.sourceCode, extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices) override
		{
			// Check whether or not there's geometry shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_geometry_shader4() && extensions.isGL_ARB_gl_spirv())
			{
				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
				return RENDERER_NEW(getRenderer().getContext(), GeometryShaderSeparate)(openGLRenderer, shaderBytecode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices);
			}
			else
			{
				// Error! There's no geometry shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's geometry shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_geometry_shader4())
			{
				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
				return RENDERER_NEW(getRenderer().getContext(), GeometryShaderSeparate)(openGLRenderer, shaderSourceCode.sourceCode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices, extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr);
			}
			else
			{
				// Error! There's no geometry shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// Check whether or not there's fragment shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_fragment_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RENDERER_NEW(getRenderer().getContext(), FragmentShaderSeparate)(openGLRenderer, shaderBytecode);
			}
			else
			{
				// Error! There's no fragment shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's fragment shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_fragment_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), FragmentShaderSeparate)(openGLRenderer, shaderSourceCode.sourceCode, extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr);
			}
			else
			{
				// Error! There's no fragment shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IComputeShader* createComputeShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// Check whether or not there's compute shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_compute_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RENDERER_NEW(getRenderer().getContext(), ComputeShaderSeparate)(openGLRenderer, shaderBytecode);
			}
			else
			{
				// Error! There's no compute shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IComputeShader* createComputeShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// Check whether or not there's compute shader support
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());
			const Extensions& extensions = openGLRenderer.getExtensions();
			if (extensions.isGL_ARB_compute_shader())
			{
				return RENDERER_NEW(getRenderer().getContext(), ComputeShaderSeparate)(openGLRenderer, shaderSourceCode.sourceCode, extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr);
			}
			else
			{
				// Error! There's no compute shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::IGraphicsProgram* createGraphicsProgram(const Renderer::IRootSignature& rootSignature, [[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, Renderer::IVertexShader* vertexShader, Renderer::ITessellationControlShader* tessellationControlShader, Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			OpenGLRenderer& openGLRenderer = static_cast<OpenGLRenderer&>(getRenderer());

			// A shader can be a null pointer, but if it's not the shader and graphics program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			if (nullptr != vertexShader && vertexShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Vertex shader language mismatch!
			}
			else if (nullptr != tessellationControlShader && tessellationControlShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Tessellation control shader language mismatch!
			}
			else if (nullptr != tessellationEvaluationShader && tessellationEvaluationShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Tessellation evaluation shader language mismatch!
			}
			else if (nullptr != geometryShader && geometryShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Geometry shader language mismatch!
			}
			else if (nullptr != fragmentShader && fragmentShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Fragment shader language mismatch!
			}

			// Is "GL_EXT_direct_state_access" there?
			else if (openGLRenderer.getExtensions().isGL_EXT_direct_state_access() || openGLRenderer.getExtensions().isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramSeparateDsa)(openGLRenderer, rootSignature, static_cast<VertexShaderSeparate*>(vertexShader), static_cast<TessellationControlShaderSeparate*>(tessellationControlShader), static_cast<TessellationEvaluationShaderSeparate*>(tessellationEvaluationShader), static_cast<GeometryShaderSeparate*>(geometryShader), static_cast<FragmentShaderSeparate*>(fragmentShader));
			}
			else
			{
				// Traditional bind version
				return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramSeparate)(openGLRenderer, rootSignature, static_cast<VertexShaderSeparate*>(vertexShader), static_cast<TessellationControlShaderSeparate*>(tessellationControlShader), static_cast<TessellationEvaluationShaderSeparate*>(tessellationEvaluationShader), static_cast<GeometryShaderSeparate*>(geometryShader), static_cast<FragmentShaderSeparate*>(fragmentShader));
			}

			// Error! Shader language mismatch!
			// -> Ensure a correct reference counter behaviour, even in the situation of an error
			if (nullptr != vertexShader)
			{
				vertexShader->addReference();
				vertexShader->releaseReference();
			}
			if (nullptr != tessellationControlShader)
			{
				tessellationControlShader->addReference();
				tessellationControlShader->releaseReference();
			}
			if (nullptr != tessellationEvaluationShader)
			{
				tessellationEvaluationShader->addReference();
				tessellationEvaluationShader->releaseReference();
			}
			if (nullptr != geometryShader)
			{
				geometryShader->addReference();
				geometryShader->releaseReference();
			}
			if (nullptr != fragmentShader)
			{
				fragmentShader->addReference();
				fragmentShader->releaseReference();
			}

			// Error!
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ShaderLanguageSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageSeparate(const ShaderLanguageSeparate& source) = delete;
		ShaderLanguageSeparate& operator =(const ShaderLanguageSeparate& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRenderer/State/GraphicsPipelineState.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL graphics pipeline state class
	*/
	class GraphicsPipelineState final : public Renderer::IGraphicsPipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRenderer
		*    Owner OpenGL renderer instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(OpenGLRenderer& openGLRenderer, const Renderer::GraphicsPipelineState& graphicsPipelineState, uint16_t id) :
			IGraphicsPipelineState(openGLRenderer, id),
			mOpenGLPrimitiveTopology(0xFFFF),	// Unknown default setting
			mNumberOfVerticesPerPatch(0),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mRasterizerState(graphicsPipelineState.rasterizerState),
			mDepthStencilState(graphicsPipelineState.depthStencilState),
			mBlendState(graphicsPipelineState.blendState)
		{
			// Tessellation support: Up to 32 vertices per patch are supported "Renderer::PrimitiveTopology::PATCH_LIST_1" ... "Renderer::PrimitiveTopology::PATCH_LIST_32"
			if (graphicsPipelineState.primitiveTopology >= Renderer::PrimitiveTopology::PATCH_LIST_1)
			{
				// Use tessellation

				// Get number of vertices that will be used to make up a single patch primitive
				// -> There's no need to check for the "GL_ARB_tessellation_shader" extension, it's there if "Renderer::Capabilities::maximumNumberOfPatchVertices" is not 0
				const int numberOfVerticesPerPatch = static_cast<int>(graphicsPipelineState.primitiveTopology) - static_cast<int>(Renderer::PrimitiveTopology::PATCH_LIST_1) + 1;
				if (numberOfVerticesPerPatch <= static_cast<int>(openGLRenderer.getCapabilities().maximumNumberOfPatchVertices))
				{
					// Set number of vertices that will be used to make up a single patch primitive
					mNumberOfVerticesPerPatch = numberOfVerticesPerPatch;

					// Set OpenGL primitive topology
					mOpenGLPrimitiveTopology = GL_PATCHES;
				}
				else
				{
					// Error!
					RENDERER_ASSERT(openGLRenderer.getContext(), false, "Invalid number of OpenGL vertices per patch")
				}
			}
			else
			{
				// Do not use tessellation

				// Map and backup the set OpenGL primitive topology
				mOpenGLPrimitiveTopology = Mapping::getOpenGLType(graphicsPipelineState.primitiveTopology);
			}

			// Ensure a correct reference counter behaviour
			graphicsPipelineState.rootSignature->addReference();
			graphicsPipelineState.rootSignature->releaseReference();

			// Add a reference to the referenced renderer resources
			mGraphicsProgram->addReference();
			mRenderPass->addReference();
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsPipelineState() override
		{
			// Release referenced renderer resources
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Free the unique compact graphics pipeline state ID
			static_cast<OpenGLRenderer&>(getRenderer()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the graphics program
		*
		*  @return
		*    Graphics program, always valid
		*/
		[[nodiscard]] inline Renderer::IGraphicsProgram* getGraphicsProgram() const
		{
			return mGraphicsProgram;
		}

		/**
		*  @brief
		*    Return the OpenGL primitive topology describing the type of primitive to render
		*
		*  @return
		*    OpenGL primitive topology describing the type of primitive to render
		*/
		[[nodiscard]] inline GLenum getOpenGLPrimitiveTopology() const
		{
			return mOpenGLPrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the number of vertices per patch
		*
		*  @return
		*    Return the number of vertices per patch
		*/
		[[nodiscard]] inline GLint getNumberOfVerticesPerPatch() const
		{
			return mNumberOfVerticesPerPatch;
		}

		/**
		*  @brief
		*    Bind the graphics pipeline state
		*/
		void bindGraphicsPipelineState() const
		{
			static_cast<OpenGLRenderer&>(getRenderer()).setOpenGLGraphicsProgram(mGraphicsProgram);

			// Set the OpenGL rasterizer state
			mRasterizerState.setOpenGLRasterizerStates();

			// Set OpenGL depth stencil state
			mDepthStencilState.setOpenGLDepthStencilStates();

			// Set OpenGL blend state
			mBlendState.setOpenGLBlendStates();
		}

		//[-------------------------------------------------------]
		//[ Detail state access                                   ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const Renderer::RasterizerState& getRasterizerState() const
		{
			return mRasterizerState.getRasterizerState();
		}

		[[nodiscard]] inline const Renderer::DepthStencilState& getDepthStencilState() const
		{
			return mDepthStencilState.getDepthStencilState();
		}

		[[nodiscard]] inline const Renderer::BlendState& getBlendState() const
		{
			return mBlendState.getBlendState();
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GraphicsPipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsPipelineState(const GraphicsPipelineState& source) = delete;
		GraphicsPipelineState& operator =(const GraphicsPipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLenum						mOpenGLPrimitiveTopology;	///< OpenGL primitive topology describing the type of primitive to render
		GLint						mNumberOfVerticesPerPatch;	///< Number of vertices per patch
		Renderer::IGraphicsProgram*	mGraphicsProgram;
		Renderer::IRenderPass*		mRenderPass;
		RasterizerState				mRasterizerState;
		DepthStencilState			mDepthStencilState;
		BlendState					mBlendState;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // OpenGLRenderer




//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		// Implementation from "08/02/2015 Better array 'countof' implementation with C++ 11 (updated)" - https://www.g-truc.net/post-0708.html
		template<typename T, std::size_t N>
		[[nodiscard]] constexpr std::size_t countof(T const (&)[N])
		{
			return N;
		}

		[[nodiscard]] bool mapBuffer([[maybe_unused]] const Renderer::Context& context, const OpenGLRenderer::Extensions& extensions, GLenum target, [[maybe_unused]] GLenum bindingTarget, GLuint openGLBuffer, Renderer::MapType mapType, Renderer::MappedSubresource& mappedSubresource)
		{
			// TODO(co) This buffer update isn't efficient, use e.g. persistent buffer mapping

			// Is "GL_ARB_direct_state_access" there?
			if (extensions.isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				mappedSubresource.data		 = glMapNamedBuffer(openGLBuffer, OpenGLRenderer::Mapping::getOpenGLMapType(mapType));
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
			}
			// Is "GL_EXT_direct_state_access" there?
			else if (extensions.isGL_EXT_direct_state_access())
			{
				// Effective direct state access (DSA)
				mappedSubresource.data		 = glMapNamedBufferEXT(openGLBuffer, OpenGLRenderer::Mapping::getOpenGLMapType(mapType));
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
			}
			else
			{
				// Traditional bind version

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL buffer
					GLint openGLBufferBackup = 0;
					glGetIntegerv(bindingTarget, &openGLBufferBackup);
				#endif

				// Bind this OpenGL buffer
				glBindBufferARB(target, openGLBuffer);

				// Map
				mappedSubresource.data		 = glMapBufferARB(target, OpenGLRenderer::Mapping::getOpenGLMapType(mapType));
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL buffer
					glBindBufferARB(target, static_cast<GLuint>(openGLBufferBackup));
				#endif
			}

			// Done
			RENDERER_ASSERT(context, nullptr != mappedSubresource.data, "Mapping of OpenGL buffer failed")
			return (nullptr != mappedSubresource.data);
		}

		void unmapBuffer(const OpenGLRenderer::Extensions& extensions, GLenum target, [[maybe_unused]] GLenum bindingTarget, GLuint openGLBuffer)
		{
			// Is "GL_ARB_direct_state_access" there?
			if (extensions.isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				glUnmapNamedBuffer(openGLBuffer);
			}
			// Is "GL_EXT_direct_state_access" there?
			else if (extensions.isGL_EXT_direct_state_access())
			{
				// Effective direct state access (DSA)
				glUnmapNamedBufferEXT(openGLBuffer);
			}
			else
			{
				// Traditional bind version

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL buffer
					GLint openGLBufferBackup = 0;
					glGetIntegerv(bindingTarget, &openGLBufferBackup);
				#endif

				// Bind this OpenGL buffer
				glBindBufferARB(target, openGLBuffer);

				// Unmap
				glUnmapBufferARB(target);

				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL buffer
					glBindBufferARB(target, static_cast<GLuint>(openGLBufferBackup));
				#endif
			}
		}

		namespace BackendDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ExecuteCommandBuffer* realData = static_cast<const Renderer::Command::ExecuteCommandBuffer*>(data);
				RENDERER_ASSERT(renderer.getContext(), nullptr != realData->commandBufferToExecute, "The OpenGL command buffer to execute must be valid")
				renderer.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsRootSignature* realData = static_cast<const Renderer::Command::SetGraphicsRootSignature*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsPipelineState* realData = static_cast<const Renderer::Command::SetGraphicsPipelineState*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsResourceGroup* realData = static_cast<const Renderer::Command::SetGraphicsResourceGroup*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Renderer::IRenderer& renderer)
			{
				// Input-assembler (IA) stage
				const Renderer::Command::SetGraphicsVertexArray* realData = static_cast<const Renderer::Command::SetGraphicsVertexArray*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsViewports* realData = static_cast<const Renderer::Command::SetGraphicsViewports*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Renderer::Viewport*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsScissorRectangles* realData = static_cast<const Renderer::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Renderer::ScissorRectangle*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Renderer::IRenderer& renderer)
			{
				// Output-merger (OM) stage
				const Renderer::Command::SetGraphicsRenderTarget* realData = static_cast<const Renderer::Command::SetGraphicsRenderTarget*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ClearGraphics* realData = static_cast<const Renderer::Command::ClearGraphics*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawGraphics* realData = static_cast<const Renderer::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).drawGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).drawGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawIndexedGraphics* realData = static_cast<const Renderer::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).drawIndexedGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).drawIndexedGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputeRootSignature* realData = static_cast<const Renderer::Command::SetComputeRootSignature*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputePipelineState* realData = static_cast<const Renderer::Command::SetComputePipelineState*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputeResourceGroup* realData = static_cast<const Renderer::Command::SetComputeResourceGroup*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DispatchCompute* realData = static_cast<const Renderer::Command::DispatchCompute*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).dispatchCompute(realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Renderer::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					static_cast<OpenGLRenderer::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RENDERER_LOG(static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).getContext(), CRITICAL, "Unsupported OpenGL texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Renderer::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::CopyResource* realData = static_cast<const Renderer::Command::CopyResource*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::GenerateMipmaps* realData = static_cast<const Renderer::Command::GenerateMipmaps*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResetQueryPool* realData = static_cast<const Renderer::Command::ResetQueryPool*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::BeginQuery* realData = static_cast<const Renderer::Command::BeginQuery*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::EndQuery* realData = static_cast<const Renderer::Command::EndQuery*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::WriteTimestampQuery* realData = static_cast<const Renderer::Command::WriteTimestampQuery*>(data);
				static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RENDERER_DEBUG
				void SetDebugMarker(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::SetDebugMarker* realData = static_cast<const Renderer::Command::SetDebugMarker*>(data);
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::BeginDebugEvent* realData = static_cast<const Renderer::Command::BeginDebugEvent*>(data);
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Renderer::IRenderer& renderer)
				{
					static_cast<OpenGLRenderer::OpenGLRenderer&>(renderer).endDebugEvent();
				}
			#else
				void SetDebugMarker(const void*, Renderer::IRenderer&)
				{
					// Nothing here
				}

				void BeginDebugEvent(const void*, Renderer::IRenderer&)
				{
					// Nothing here
				}

				void EndDebugEvent(const void*, Renderer::IRenderer&)
				{
					// Nothing here
				}
			#endif


		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Renderer::BackendDispatchFunction DISPATCH_FUNCTIONS[Renderer::CommandDispatchFunctionIndex::NumberOfFunctions] =
		{
			// Command buffer
			&BackendDispatch::ExecuteCommandBuffer,
			// Graphics
			&BackendDispatch::SetGraphicsRootSignature,
			&BackendDispatch::SetGraphicsPipelineState,
			&BackendDispatch::SetGraphicsResourceGroup,
			&BackendDispatch::SetGraphicsVertexArray,		// Input-assembler (IA) stage
			&BackendDispatch::SetGraphicsViewports,			// Rasterizer (RS) stage
			&BackendDispatch::SetGraphicsScissorRectangles,	// Rasterizer (RS) stage
			&BackendDispatch::SetGraphicsRenderTarget,		// Output-merger (OM) stage
			&BackendDispatch::ClearGraphics,
			&BackendDispatch::DrawGraphics,
			&BackendDispatch::DrawIndexedGraphics,
			// Compute
			&BackendDispatch::SetComputeRootSignature,
			&BackendDispatch::SetComputePipelineState,
			&BackendDispatch::SetComputeResourceGroup,
			&BackendDispatch::DispatchCompute,
			// Resource
			&BackendDispatch::SetTextureMinimumMaximumMipmapIndex,
			&BackendDispatch::ResolveMultisampleFramebuffer,
			&BackendDispatch::CopyResource,
			&BackendDispatch::GenerateMipmaps,
			// Query
			&BackendDispatch::ResetQueryPool,
			&BackendDispatch::BeginQuery,
			&BackendDispatch::EndQuery,
			&BackendDispatch::WriteTimestampQuery,
			// Debug
			&BackendDispatch::SetDebugMarker,
			&BackendDispatch::BeginDebugEvent,
			&BackendDispatch::EndDebugEvent
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace OpenGLRenderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	OpenGLRenderer::OpenGLRenderer(const Renderer::Context& context) :
		IRenderer(Renderer::NameId::OPENGL, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		ComputePipelineStateMakeId(context.getAllocator()),
		mOpenGLRuntimeLinking(nullptr),
		mOpenGLContext(nullptr),
		mExtensions(nullptr),
		mShaderLanguage(nullptr),
		mGraphicsRootSignature(nullptr),
		mComputeRootSignature(nullptr),
		mDefaultSamplerState(nullptr),
		mOpenGLCopyResourceFramebuffer(0),
		mDefaultOpenGLVertexArray(0),
		// States
		mGraphicsPipelineState(nullptr),
		mComputePipelineState(nullptr),
		// Input-assembler (IA) stage
		mVertexArray(nullptr),
		mOpenGLPrimitiveTopology(0xFFFF),	// Unknown default setting
		mNumberOfVerticesPerPatch(0),
		// Output-merger (OM) stage
		mRenderTarget(nullptr),
		// State cache to avoid making redundant OpenGL calls
		mOpenGLClipControlOrigin(GL_INVALID_ENUM),
		mOpenGLProgramPipeline(0),
		mOpenGLProgram(0),
		mOpenGLIndirectBuffer(0),
		// Draw ID uniform location for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
		mOpenGLVertexProgram(0),
		mDrawIdUniformLocation(-1),
		mCurrentStartInstanceLocation(~0u)
	{
		// Is OpenGL available?
		mOpenGLRuntimeLinking = RENDERER_NEW(mContext, OpenGLRuntimeLinking)(*this);
		if (mOpenGLRuntimeLinking->isOpenGLAvaiable())
		{
			const Renderer::handle nativeWindowHandle = mContext.getNativeWindowHandle();
			const Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::Enum::R8G8B8A8;
			const RenderPass renderPass(*this, 1, &textureFormat, Renderer::TextureFormat::Enum::UNKNOWN, 1);
			#ifdef _WIN32
			{
				// TODO(co) Add external OpenGL context support
				mOpenGLContext = RENDERER_NEW(mContext, OpenGLContextWindows)(mOpenGLRuntimeLinking, renderPass.getDepthStencilAttachmentTextureFormat(), nativeWindowHandle);
			}
			#elif defined LINUX
				mOpenGLContext = RENDERER_NEW(mContext, OpenGLContextLinux)(*this, mOpenGLRuntimeLinking, renderPass.getDepthStencilAttachmentTextureFormat(), nativeWindowHandle, mContext.isUsingExternalContext());
			#else
				#error "Unsupported platform"
			#endif

			// We're using "this" in here, so we are not allowed to write the following within the initializer list
			mExtensions = RENDERER_NEW(mContext, Extensions)(*this, *mOpenGLContext);

			// Is the OpenGL context and extensions initialized?
			if (mOpenGLContext->isInitialized() && mExtensions->initialize())
			{
				#ifdef RENDERER_DEBUG
					// "GL_ARB_debug_output"-extension available?
					if (mExtensions->isGL_ARB_debug_output())
					{
						// Synchronous debug output, please
						// -> Makes it easier to find the place causing the issue
						glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

						// Disable severity notifications, most drivers print many things with this severity
						glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, false);

						// Set the debug message callback function
						glDebugMessageCallbackARB(&OpenGLRenderer::debugMessageCallback, this);
					}
				#endif

				// Initialize the capabilities
				initializeCapabilities();

				// Create the default sampler state
				mDefaultSamplerState = createSamplerState(Renderer::ISamplerState::getDefaultSamplerState());

				// Create default OpenGL vertex array
				if (mExtensions->isGL_ARB_vertex_array_object())
				{
					glGenVertexArrays(1, &mDefaultOpenGLVertexArray);
					glBindVertexArray(mDefaultOpenGLVertexArray);
				}

				// Add references to the default sampler state and set it
				if (nullptr != mDefaultSamplerState)
				{
					mDefaultSamplerState->addReference();
					// TODO(co) Set default sampler states
				}
			}
		}
	}

	OpenGLRenderer::~OpenGLRenderer()
	{
		// Set no graphics and compute pipeline state reference, in case we have one
		if (nullptr != mGraphicsPipelineState)
		{
			setGraphicsPipelineState(nullptr);
		}
		if (nullptr != mComputePipelineState)
		{
			setComputePipelineState(nullptr);
		}

		// Set no vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			setGraphicsVertexArray(nullptr);
		}

		// Release instances
		if (nullptr != mRenderTarget)
		{
			mRenderTarget->releaseReference();
			mRenderTarget = nullptr;
		}
		if (nullptr != mDefaultSamplerState)
		{
			mDefaultSamplerState->releaseReference();
			mDefaultSamplerState = nullptr;
		}

		// Destroy the OpenGL framebuffer used by "OpenGLRenderer::OpenGLRenderer::copyResource()" if the "GL_ARB_copy_image"-extension isn't available
		// -> Silently ignores 0's and names that do not correspond to existing buffer objects
		glDeleteFramebuffers(1, &mOpenGLCopyResourceFramebuffer);

		// Destroy the OpenGL default vertex array
		// -> Silently ignores 0's and names that do not correspond to existing vertex array objects
		glDeleteVertexArrays(1, &mDefaultOpenGLVertexArray);

		// Release the graphics and compute root signature instance, in case we have one
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
		}

		#ifdef RENDERER_STATISTICS
		{ // For debugging: At this point there should be no resource instances left, validate this!
			// -> Are the currently any resource instances?
			const uint32_t numberOfCurrentResources = getStatistics().getNumberOfCurrentResources();
			if (numberOfCurrentResources > 0)
			{
				// Error!
				if (numberOfCurrentResources > 1)
				{
					RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend is going to be destroyed, but there are still %lu resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the shader language instance, in case we have one
		if (nullptr != mShaderLanguage)
		{
			mShaderLanguage->releaseReference();
		}

		// Destroy the extensions instance
		RENDERER_DELETE(mContext, Extensions, mExtensions);

		// Destroy the OpenGL context instance
		RENDERER_DELETE(mContext, IOpenGLContext, mOpenGLContext);

		// Destroy the OpenGL runtime linking instance
		RENDERER_DELETE(mContext, OpenGLRuntimeLinking, mOpenGLRuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void OpenGLRenderer::setGraphicsRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		mGraphicsRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->addReference();

			// Sanity check
			OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void OpenGLRenderer::setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (mGraphicsPipelineState != graphicsPipelineState)
		{
			if (nullptr != graphicsPipelineState)
			{
				// Sanity check
				OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsPipelineState)

				// Set new graphics pipeline state and add a reference to it
				if (nullptr != mGraphicsPipelineState)
				{
					mGraphicsPipelineState->releaseReference();
				}
				mGraphicsPipelineState = static_cast<GraphicsPipelineState*>(graphicsPipelineState);
				mGraphicsPipelineState->addReference();

				// Set OpenGL primitive topology
				mOpenGLPrimitiveTopology = mGraphicsPipelineState->getOpenGLPrimitiveTopology();
				const int newNumberOfVerticesPerPatch = mGraphicsPipelineState->getNumberOfVerticesPerPatch();
				if (0 != newNumberOfVerticesPerPatch && mNumberOfVerticesPerPatch != newNumberOfVerticesPerPatch)
				{
					mNumberOfVerticesPerPatch = newNumberOfVerticesPerPatch;
					glPatchParameteri(GL_PATCH_VERTICES, mNumberOfVerticesPerPatch);
				}

				// Set graphics pipeline state
				mGraphicsPipelineState->bindGraphicsPipelineState();
			}
			else if (nullptr != mGraphicsPipelineState)
			{
				// TODO(co) Handle this situation by resetting OpenGL states?
				mGraphicsPipelineState->releaseReference();
				mGraphicsPipelineState = nullptr;
			}
		}

		// Set graphics pipeline state
		else if (nullptr != mGraphicsPipelineState)
		{
			// Set OpenGL graphics pipeline state
			// -> This is necessary since OpenGL is using just a single current program, for graphics as well as compute
			setOpenGLGraphicsProgram(mGraphicsPipelineState->getGraphicsProgram());
		}
	}

	void OpenGLRenderer::setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No OpenGL renderer backend graphics root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		// Set graphics resource group
		setResourceGroup(*mGraphicsRootSignature, rootParameterIndex, resourceGroup);
	}

	void OpenGLRenderer::setGraphicsVertexArray(Renderer::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage

		// New vertex array?
		if (mVertexArray != vertexArray)
		{
			// Set a vertex array?
			if (nullptr != vertexArray)
			{
				// Sanity check
				OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *vertexArray)

				// Unset the currently used vertex array
				unsetGraphicsVertexArray();

				// Set new vertex array and add a reference to it
				mVertexArray = static_cast<VertexArray*>(vertexArray);
				mVertexArray->addReference();

				// Evaluate the internal array type of the new vertex array to set
				switch (static_cast<VertexArray*>(mVertexArray)->getInternalResourceType())
				{
					case VertexArray::InternalResourceType::NO_VAO:
						// Enable OpenGL vertex attribute arrays
						static_cast<VertexArrayNoVao*>(mVertexArray)->enableOpenGLVertexAttribArrays();
						break;

					case VertexArray::InternalResourceType::VAO:
						// Bind OpenGL vertex array
						glBindVertexArray(static_cast<VertexArrayVao*>(mVertexArray)->getOpenGLVertexArray());
						break;
				}
			}
			else
			{
				// Unset the currently used vertex array
				unsetGraphicsVertexArray();
			}
		}
	}

	void OpenGLRenderer::setGraphicsViewports([[maybe_unused]] uint32_t numberOfViewports, const Renderer::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid OpenGL rasterizer state viewports")

		// In OpenGL, the origin of the viewport is left bottom while Direct3D is using a left top origin. To make the
		// Direct3D 11 implementation as efficient as possible the Direct3D convention is used and we have to convert in here.
		// -> This isn't influenced by the "GL_ARB_clip_control"-extension

		// Get the width and height of the current render target
		uint32_t renderTargetHeight = 1;
		if (nullptr != mRenderTarget)
		{
			uint32_t renderTargetWidth = 1;
			mRenderTarget->getWidthAndHeight(renderTargetWidth, renderTargetHeight);
		}

		// Set the OpenGL viewport
		// TODO(co) "GL_ARB_viewport_array" support ("OpenGLRenderer::setGraphicsViewports()")
		// TODO(co) Check for "numberOfViewports" out of range or are the debug events good enough?
		RENDERER_ASSERT(mContext, numberOfViewports <= 1, "OpenGL supports only one viewport")
		glViewport(static_cast<GLint>(viewports->topLeftX), static_cast<GLint>(renderTargetHeight - viewports->topLeftY - viewports->height), static_cast<GLsizei>(viewports->width), static_cast<GLsizei>(viewports->height));
		glDepthRange(static_cast<GLclampf>(viewports->minDepth), static_cast<GLclampf>(viewports->maxDepth));
	}

	void OpenGLRenderer::setGraphicsScissorRectangles([[maybe_unused]] uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid OpenGL rasterizer state scissor rectangles")

		// In OpenGL, the origin of the scissor rectangle is left bottom while Direct3D is using a left top origin. To make the
		// Direct3D 9 & 10 & 11 implementation as efficient as possible the Direct3D convention is used and we have to convert in here.
		// -> This isn't influenced by the "GL_ARB_clip_control"-extension

		// Get the width and height of the current render target
		uint32_t renderTargetHeight = 1;
		if (nullptr != mRenderTarget)
		{
			uint32_t renderTargetWidth = 1;
			mRenderTarget->getWidthAndHeight(renderTargetWidth, renderTargetHeight);
		}

		// Set the OpenGL scissor rectangle
		// TODO(co) "GL_ARB_viewport_array" support ("OpenGLRenderer::setGraphicsViewports()")
		// TODO(co) Check for "numberOfViewports" out of range or are the debug events good enough?
		RENDERER_ASSERT(mContext, numberOfScissorRectangles <= 1, "OpenGL supports only one scissor rectangle")
		const GLsizei width  = scissorRectangles->bottomRightX - scissorRectangles->topLeftX;
		const GLsizei height = scissorRectangles->bottomRightY - scissorRectangles->topLeftY;
		glScissor(static_cast<GLint>(scissorRectangles->topLeftX), static_cast<GLint>(renderTargetHeight - scissorRectangles->topLeftY - height), width, height);
	}

	void OpenGLRenderer::setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Sanity check
				OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *renderTarget)

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					// Unbind OpenGL framebuffer?
					if (Renderer::ResourceType::FRAMEBUFFER == mRenderTarget->getResourceType() && Renderer::ResourceType::FRAMEBUFFER != renderTarget->getResourceType())
					{
						// Do we need to disable multisample?
						if (static_cast<Framebuffer*>(mRenderTarget)->isMultisampleRenderTarget())
						{
							glDisable(GL_MULTISAMPLE);
						}

						// We do not render into a OpenGL framebuffer
						glBindFramebuffer(GL_FRAMEBUFFER, 0);
					}

					// Release
					mRenderTarget->releaseReference();
				}

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Evaluate the render target type
				GLenum clipControlOrigin = GL_UPPER_LEFT;
				switch (mRenderTarget->getResourceType())
				{
					case Renderer::ResourceType::SWAP_CHAIN:
					{
						static_cast<SwapChain*>(mRenderTarget)->getOpenGLContext().makeCurrent();
						clipControlOrigin = GL_LOWER_LEFT;	// Compensate OS window coordinate system y-flip
						break;
					}

					case Renderer::ResourceType::FRAMEBUFFER:
					{
						// Get the OpenGL framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Bind the OpenGL framebuffer
						glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->getOpenGLFramebuffer());

						// Define the OpenGL buffers to draw into, "GL_ARB_draw_buffers"-extension required
						if (mExtensions->isGL_ARB_draw_buffers())
						{
							// https://www.opengl.org/registry/specs/ARB/draw_buffers.txt - "The draw buffer for output colors beyond <n> is set to NONE."
							// -> Meaning depth only rendering which has no color textures at all will work as well, no need for "glDrawBuffer(GL_NONE)"
							static constexpr GLenum OPENGL_DRAW_BUFFER[16] =
							{
								GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1,  GL_COLOR_ATTACHMENT2,  GL_COLOR_ATTACHMENT3,
								GL_COLOR_ATTACHMENT4,  GL_COLOR_ATTACHMENT5,  GL_COLOR_ATTACHMENT6,  GL_COLOR_ATTACHMENT7,
								GL_COLOR_ATTACHMENT8,  GL_COLOR_ATTACHMENT9,  GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
								GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15
							};
							glDrawBuffersARB(static_cast<GLsizei>(framebuffer->getNumberOfColorTextures()), OPENGL_DRAW_BUFFER);
						}

						// Do we need to enable multisample?
						if (framebuffer->isMultisampleRenderTarget())
						{
							glEnable(GL_MULTISAMPLE);
						}
						else
						{
							glDisable(GL_MULTISAMPLE);
						}
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::INDEX_BUFFER:
					case Renderer::ResourceType::VERTEX_BUFFER:
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::UNIFORM_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_2D:
					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::SAMPLER_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
					default:
						// Not handled in here
						break;
				}

				// Setup clip control
				if (mOpenGLClipControlOrigin != clipControlOrigin && mExtensions->isGL_ARB_clip_control())
				{
					// OpenGL default is "GL_LOWER_LEFT" and "GL_NEGATIVE_ONE_TO_ONE", change it to match Vulkan and Direct3D
					mOpenGLClipControlOrigin = clipControlOrigin;
					glClipControl(mOpenGLClipControlOrigin, GL_ZERO_TO_ONE);
				}
			}
			else if (nullptr != mRenderTarget)
			{
				// Evaluate the render target type
				if (Renderer::ResourceType::FRAMEBUFFER == mRenderTarget->getResourceType())
				{
					// We do not render into a OpenGL framebuffer
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}

				// TODO(co) Set no active render target

				// Release the render target reference, in case we have one
				mRenderTarget->releaseReference();
				mRenderTarget = nullptr;
			}
		}
	}

	void OpenGLRenderer::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Get API flags
		uint32_t flagsApi = 0;
		if (clearFlags & Renderer::ClearFlag::COLOR)
		{
			flagsApi |= GL_COLOR_BUFFER_BIT;
		}
		if (clearFlags & Renderer::ClearFlag::DEPTH)
		{
			flagsApi |= GL_DEPTH_BUFFER_BIT;
		}
		if (clearFlags & Renderer::ClearFlag::STENCIL)
		{
			flagsApi |= GL_STENCIL_BUFFER_BIT;
		}

		// Are API flags set?
		if (0 != flagsApi)
		{
			// Set clear settings
			if (clearFlags & Renderer::ClearFlag::COLOR)
			{
				glClearColor(color[0], color[1], color[2], color[3]);
			}
			if (clearFlags & Renderer::ClearFlag::DEPTH)
			{
				glClearDepth(z);
				if (nullptr != mGraphicsPipelineState && Renderer::DepthWriteMask::ALL != mGraphicsPipelineState->getDepthStencilState().depthWriteMask)
				{
					glDepthMask(GL_TRUE);
				}
			}
			if (clearFlags & Renderer::ClearFlag::STENCIL)
			{
				glClearStencil(static_cast<GLint>(stencil));
			}

			// Unlike OpenGL, when using Direct3D 10 & 11 the scissor rectangle(s) do not affect the clear operation
			// -> We have to compensate the OpenGL behaviour in here

			// Disable OpenGL scissor test, in case it's not disabled, yet
			if (nullptr != mGraphicsPipelineState && mGraphicsPipelineState->getRasterizerState().scissorEnable)
			{
				glDisable(GL_SCISSOR_TEST);
			}

			// Clear
			glClear(flagsApi);

			// Restore the previously set OpenGL states
			if (nullptr != mGraphicsPipelineState && mGraphicsPipelineState->getRasterizerState().scissorEnable)
			{
				glEnable(GL_SCISSOR_TEST);
			}
			if ((clearFlags & Renderer::ClearFlag::DEPTH) && nullptr != mGraphicsPipelineState && Renderer::DepthWriteMask::ALL != mGraphicsPipelineState->getDepthStencilState().depthWriteMask)
			{
				glDepthMask(GL_FALSE);
			}
		}
	}

	void OpenGLRenderer::drawGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, indirectBuffer)
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "Number of OpenGL draws must not be zero")
		RENDERER_ASSERT(mContext, mExtensions->isGL_ARB_draw_indirect(), "The GL_ARB_draw_indirect OpenGL extension isn't supported")
		// It's possible to draw without "mVertexArray"

		// Tessellation support: "glPatchParameteri()" is called within "OpenGLRenderer::iaSetPrimitiveTopology()"

		{ // Bind indirect buffer
			const GLuint openGLIndirectBuffer = static_cast<const IndirectBuffer&>(indirectBuffer).getOpenGLIndirectBuffer();
			if (openGLIndirectBuffer != mOpenGLIndirectBuffer)
			{
				mOpenGLIndirectBuffer = openGLIndirectBuffer;
				glBindBufferARB(GL_DRAW_INDIRECT_BUFFER, mOpenGLIndirectBuffer);
			}
		}

		// Draw indirect
		if (1 == numberOfDraws)
		{
			glDrawArraysIndirect(mOpenGLPrimitiveTopology, reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)));
		}
		else if (numberOfDraws > 1)
		{
			if (mExtensions->isGL_ARB_multi_draw_indirect())
			{
				glMultiDrawArraysIndirect(mOpenGLPrimitiveTopology, reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)), static_cast<GLsizei>(numberOfDraws), 0);	// 0 = tightly packed
			}
			else
			{
				// Emulate multi-draw-indirect
				#ifdef RENDERER_DEBUG
					beginDebugEvent("Multi-draw-indirect emulation");
				#endif
				for (uint32_t i = 0; i < numberOfDraws; ++i)
				{
					glDrawArraysIndirect(mOpenGLPrimitiveTopology, reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)));
					indirectBufferOffset += sizeof(Renderer::DrawArguments);
				}
				#ifdef RENDERER_DEBUG
					endDebugEvent();
				#endif
			}
		}
	}

	void OpenGLRenderer::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The OpenGL emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-draw-indirect emulation");
			}
		#endif
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Renderer::DrawArguments& drawArguments = *reinterpret_cast<const Renderer::DrawArguments*>(emulationData);
			updateGL_ARB_base_instanceEmulation(drawArguments.startInstanceLocation);

			// Draw and advance
			if ((drawArguments.instanceCount > 1 && mExtensions->isGL_ARB_draw_instanced()) || (drawArguments.startInstanceLocation > 0 && mExtensions->isGL_ARB_base_instance()))
			{
				// With instancing
				if (drawArguments.startInstanceLocation > 0 && mExtensions->isGL_ARB_base_instance())
				{
					glDrawArraysInstancedBaseInstance(mOpenGLPrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance), static_cast<GLsizei>(drawArguments.instanceCount), drawArguments.startInstanceLocation);
				}
				else
				{
					glDrawArraysInstancedARB(mOpenGLPrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance), static_cast<GLsizei>(drawArguments.instanceCount));
				}
			}
			else
			{
				// Without instancing
				RENDERER_ASSERT(mContext, drawArguments.instanceCount <= 1, "Invalid OpenGL instance count")
				glDrawArrays(mOpenGLPrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance));
			}
			emulationData += sizeof(Renderer::DrawArguments);
		}
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void OpenGLRenderer::drawIndexedGraphics(const Renderer::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, indirectBuffer)
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "Number of OpenGL draws must not be zero")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray, "OpenGL draw indexed needs a set vertex array")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "OpenGL draw indexed needs a set vertex array which contains an index buffer")
		RENDERER_ASSERT(mContext, mExtensions->isGL_ARB_draw_indirect(), "The GL_ARB_draw_indirect OpenGL extension isn't supported")

		// Tessellation support: "glPatchParameteri()" is called within "OpenGLRenderer::iaSetPrimitiveTopology()"

		{ // Bind indirect buffer
			const GLuint openGLIndirectBuffer = static_cast<const IndirectBuffer&>(indirectBuffer).getOpenGLIndirectBuffer();
			if (openGLIndirectBuffer != mOpenGLIndirectBuffer)
			{
				mOpenGLIndirectBuffer = openGLIndirectBuffer;
				glBindBufferARB(GL_DRAW_INDIRECT_BUFFER, mOpenGLIndirectBuffer);
			}
		}

		// Draw indirect
		if (1 == numberOfDraws)
		{
			glDrawElementsIndirect(mOpenGLPrimitiveTopology, mVertexArray->getIndexBuffer()->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)));
		}
		else if (numberOfDraws > 1)
		{
			if (mExtensions->isGL_ARB_multi_draw_indirect())
			{
				glMultiDrawElementsIndirect(mOpenGLPrimitiveTopology, mVertexArray->getIndexBuffer()->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)), static_cast<GLsizei>(numberOfDraws), 0);	// 0 = tightly packed
			}
			else
			{
				// Emulate multi-indexed-draw-indirect
				#ifdef RENDERER_DEBUG
					beginDebugEvent("Multi-indexed-draw-indirect emulation");
				#endif
				const uint32_t openGLType = mVertexArray->getIndexBuffer()->getOpenGLType();
				for (uint32_t i = 0; i < numberOfDraws; ++i)
				{
					glDrawElementsIndirect(mOpenGLPrimitiveTopology, openGLType, reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)));
					indirectBufferOffset += sizeof(Renderer::DrawIndexedArguments);
				}
				#ifdef RENDERER_DEBUG
					endDebugEvent();
				#endif
			}
		}
	}

	void OpenGLRenderer::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The OpenGL emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL draws must not be zero")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray, "OpenGL draw indexed needs a set vertex array")
		RENDERER_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "OpenGL draw indexed needs a set vertex array which contains an index buffer")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			}
		#endif
		IndexBuffer* indexBuffer = mVertexArray->getIndexBuffer();
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Renderer::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Renderer::DrawIndexedArguments*>(emulationData);
			updateGL_ARB_base_instanceEmulation(drawIndexedArguments.startInstanceLocation);

			// Draw and advance
			if ((drawIndexedArguments.instanceCount > 1 && mExtensions->isGL_ARB_draw_instanced()) || (drawIndexedArguments.startInstanceLocation > 0 && mExtensions->isGL_ARB_base_instance()))
			{
				// With instancing
				if (drawIndexedArguments.baseVertexLocation > 0)
				{
					// Use start instance location?
					if (drawIndexedArguments.startInstanceLocation > 0 && mExtensions->isGL_ARB_base_instance())
					{
						// Draw with base vertex location and start instance location
						glDrawElementsInstancedBaseVertexBaseInstance(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount), static_cast<GLint>(drawIndexedArguments.baseVertexLocation), drawIndexedArguments.startInstanceLocation);
					}

					// Is the "GL_ARB_draw_elements_base_vertex" extension there?
					else if (mExtensions->isGL_ARB_draw_elements_base_vertex())
					{
						// Draw with base vertex location
						glDrawElementsInstancedBaseVertex(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount), static_cast<GLint>(drawIndexedArguments.baseVertexLocation));
					}
					else
					{
						// Error!
						RENDERER_ASSERT(mContext, false, "Failed to OpenGL draw indexed emulated")
					}
				}
				else if (drawIndexedArguments.startInstanceLocation > 0 && mExtensions->isGL_ARB_base_instance())
				{
					// Draw without base vertex location and with start instance location
					glDrawElementsInstancedBaseInstance(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount), drawIndexedArguments.startInstanceLocation);
				}
				else
				{
					// Draw without base vertex location
					glDrawElementsInstancedARB(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount));
				}
			}
			else
			{
				// Without instancing
				RENDERER_ASSERT(mContext, drawIndexedArguments.instanceCount <= 1, "Invalid OpenGL instance count")

				// Use base vertex location?
				if (drawIndexedArguments.baseVertexLocation > 0)
				{
					// Is the "GL_ARB_draw_elements_base_vertex" extension there?
					if (mExtensions->isGL_ARB_draw_elements_base_vertex())
					{
						// Draw with base vertex location
						glDrawElementsBaseVertex(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLint>(drawIndexedArguments.baseVertexLocation));
					}
					else
					{
						// Error!
						RENDERER_ASSERT(mContext, false, "Failed to OpenGL draw indexed emulated")
					}
				}
				else
				{
					// Draw without base vertex location
					glDrawElements(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())));
				}
			}
			emulationData += sizeof(Renderer::DrawIndexedArguments);
		}
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}


	//[-------------------------------------------------------]
	//[ Compute                                               ]
	//[-------------------------------------------------------]
	void OpenGLRenderer::setComputeRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
		}
		mComputeRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->addReference();

			// Sanity check
			OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void OpenGLRenderer::setComputePipelineState(Renderer::IComputePipelineState* computePipelineState)
	{
		if (mComputePipelineState != computePipelineState)
		{
			if (nullptr != computePipelineState)
			{
				// Sanity check
				OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *computePipelineState)

				// Set new compute pipeline state and add a reference to it
				if (nullptr != mComputePipelineState)
				{
					mComputePipelineState->releaseReference();
				}
				mComputePipelineState = static_cast<ComputePipelineState*>(computePipelineState);
				mComputePipelineState->addReference();

				// Set OpenGL compute pipeline state
				setOpenGLComputePipelineState(mComputePipelineState);
			}
			else if (nullptr != mComputePipelineState)
			{
				// TODO(co) Handle this situation by resetting OpenGL states?
				mComputePipelineState->releaseReference();
				mComputePipelineState = nullptr;
			}
		}

		// Set compute pipeline state
		else if (nullptr != mComputePipelineState)
		{
			// Set OpenGL compute pipeline state
			// -> This is necessary since OpenGL is using just a single current program, for graphics as well as compute
			setOpenGLComputePipelineState(mComputePipelineState);
		}
	}

	void OpenGLRenderer::setComputeResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mComputeRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No OpenGL renderer backend compute root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The OpenGL renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		// Set compute resource group
		setResourceGroup(*mComputeRootSignature, rootParameterIndex, resourceGroup);
	}

	void OpenGLRenderer::dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		// "GL_ARB_compute_shader"-extension required
		if (mExtensions->isGL_ARB_compute_shader())
		{
			glDispatchCompute(groupCountX, groupCountY, groupCountZ);

			// TODO(co) Compute shader: Memory barrier currently fixed build in: Make sure writing to image has finished before read
			glMemoryBarrierEXT(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glMemoryBarrierEXT(GL_SHADER_STORAGE_BARRIER_BIT);
		}
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void OpenGLRenderer::resolveMultisampleFramebuffer(Renderer::IRenderTarget& destinationRenderTarget, Renderer::IFramebuffer& sourceMultisampleFramebuffer)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, destinationRenderTarget)
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, sourceMultisampleFramebuffer)

		// Evaluate the render target type
		switch (destinationRenderTarget.getResourceType())
		{
			case Renderer::ResourceType::SWAP_CHAIN:
			{
				// Get the OpenGL swap chain instance
				// TODO(co) Implement me, not that important in practice so not directly implemented
				// SwapChain& swapChain = static_cast<SwapChain&>(destinationRenderTarget);
				break;
			}

			case Renderer::ResourceType::FRAMEBUFFER:
			{
				// Get the OpenGL framebuffer instances
				const Framebuffer& openGLDestinationFramebuffer = static_cast<const Framebuffer&>(destinationRenderTarget);
				const Framebuffer& openGLSourceMultisampleFramebuffer = static_cast<const Framebuffer&>(sourceMultisampleFramebuffer);

				// Get the width and height of the destination and source framebuffer
				uint32_t destinationWidth = 1;
				uint32_t destinationHeight = 1;
				openGLDestinationFramebuffer.getWidthAndHeight(destinationWidth, destinationHeight);
				uint32_t sourceWidth = 1;
				uint32_t sourceHeight = 1;
				openGLSourceMultisampleFramebuffer.getWidthAndHeight(sourceWidth, sourceHeight);

				// Resolve multisample
				glBindFramebuffer(GL_READ_FRAMEBUFFER, openGLSourceMultisampleFramebuffer.getOpenGLFramebuffer());
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, openGLDestinationFramebuffer.getOpenGLFramebuffer());
				glBlitFramebuffer(
					0, 0, static_cast<GLint>(sourceWidth), static_cast<GLint>(sourceHeight),			// Source
					0, 0, static_cast<GLint>(destinationWidth), static_cast<GLint>(destinationHeight),	// Destination
					GL_COLOR_BUFFER_BIT, GL_NEAREST);
				break;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::INDEX_BUFFER:
			case Renderer::ResourceType::VERTEX_BUFFER:
			case Renderer::ResourceType::TEXTURE_BUFFER:
			case Renderer::ResourceType::STRUCTURED_BUFFER:
			case Renderer::ResourceType::INDIRECT_BUFFER:
			case Renderer::ResourceType::UNIFORM_BUFFER:
			case Renderer::ResourceType::TEXTURE_1D:
			case Renderer::ResourceType::TEXTURE_1D_ARRAY:
			case Renderer::ResourceType::TEXTURE_2D:
			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			case Renderer::ResourceType::TEXTURE_3D:
			case Renderer::ResourceType::TEXTURE_CUBE:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Not handled in here
				break;
		}
	}

	void OpenGLRenderer::copyResource(Renderer::IResource& destinationResource, Renderer::IResource& sourceResource)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, destinationResource)
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, sourceResource)

		// Evaluate the render target type
		switch (destinationResource.getResourceType())
		{
			case Renderer::ResourceType::TEXTURE_2D:
				if (sourceResource.getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					// Get the OpenGL texture 2D instances
					const Texture2D& openGlDestinationTexture2D = static_cast<const Texture2D&>(destinationResource);
					const Texture2D& openGlSourceTexture2D = static_cast<const Texture2D&>(sourceResource);
					RENDERER_ASSERT(mContext, openGlDestinationTexture2D.getWidth() == openGlSourceTexture2D.getWidth(), "OpenGL source and destination width must be identical for resource copy")
					RENDERER_ASSERT(mContext, openGlDestinationTexture2D.getHeight() == openGlSourceTexture2D.getHeight(), "OpenGL source and destination height must be identical for resource copy")

					// Copy resource, but only the top-level mipmap
					const GLsizei width = static_cast<GLsizei>(openGlDestinationTexture2D.getWidth());
					const GLsizei height = static_cast<GLsizei>(openGlDestinationTexture2D.getHeight());
					if (mExtensions->isGL_ARB_copy_image())
					{
						glCopyImageSubData(openGlSourceTexture2D.getOpenGLTexture(), GL_TEXTURE_2D, 0, 0, 0, 0,
										   openGlDestinationTexture2D.getOpenGLTexture(), GL_TEXTURE_2D, 0, 0, 0, 0,
										   width, height, 1);
					}
					else
					{
						#ifdef RENDERER_OPENGL_STATE_CLEANUP
							// Backup the currently bound OpenGL framebuffer
							GLint openGLFramebufferBackup = 0;
							glGetIntegerv(GL_FRAMEBUFFER_BINDING, &openGLFramebufferBackup);
						#endif

						// Copy resource by using a framebuffer, but only the top-level mipmap
						if (0 == mOpenGLCopyResourceFramebuffer)
						{
							glGenFramebuffers(1, &mOpenGLCopyResourceFramebuffer);
						}
						glBindFramebuffer(GL_FRAMEBUFFER, mOpenGLCopyResourceFramebuffer);
						glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, openGlSourceTexture2D.getOpenGLTexture(), 0);
						glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, openGlDestinationTexture2D.getOpenGLTexture(), 0);
						static constexpr GLenum OPENGL_DRAW_BUFFER[1] =
						{
							GL_COLOR_ATTACHMENT1
						};
						glDrawBuffersARB(1, OPENGL_DRAW_BUFFER);	// We could use "glDrawBuffer(GL_COLOR_ATTACHMENT1);", but then we would also have to get the "glDrawBuffer()" function pointer, avoid OpenGL function overkill
						glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

						#ifdef RENDERER_OPENGL_STATE_CLEANUP
							// Be polite and restore the previous bound OpenGL framebuffer
							glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(openGLFramebufferBackup));
						#endif
					}
				}
				else
				{
					// Error!
					RENDERER_ASSERT(mContext, false, "Failed to copy OpenGL resource")
				}
				break;

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::INDEX_BUFFER:
			case Renderer::ResourceType::VERTEX_BUFFER:
			case Renderer::ResourceType::TEXTURE_BUFFER:
			case Renderer::ResourceType::STRUCTURED_BUFFER:
			case Renderer::ResourceType::INDIRECT_BUFFER:
			case Renderer::ResourceType::UNIFORM_BUFFER:
			case Renderer::ResourceType::TEXTURE_1D:
			case Renderer::ResourceType::TEXTURE_1D_ARRAY:
			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			case Renderer::ResourceType::TEXTURE_3D:
			case Renderer::ResourceType::TEXTURE_CUBE:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Not handled in here
				break;
		}
	}

	void OpenGLRenderer::generateMipmaps(Renderer::IResource& resource)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, resource)
		RENDERER_ASSERT(mContext, resource.getResourceType() == Renderer::ResourceType::TEXTURE_2D, "TODO(co) Mipmaps can only be generated for OpenGL 2D texture resources")

		Texture2D& texture2D = static_cast<Texture2D&>(resource);

		// Is "GL_EXT_direct_state_access" there?
		if (mExtensions->isGL_ARB_direct_state_access())
		{
			// Effective direct state access (DSA)
			glGenerateTextureMipmap(texture2D.getOpenGLTexture());
		}
		else if (mExtensions->isGL_EXT_direct_state_access())
		{
			// Effective direct state access (DSA)
			glGenerateTextureMipmapEXT(texture2D.getOpenGLTexture(), GL_TEXTURE_2D);
		}
		else
		{
			// Traditional bind version

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL texture
				// TODO(co) It's possible to avoid calling this multiple times
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLTextureBackup);
			#endif

			// Generate mipmaps
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, texture2D.getOpenGLTexture());
			glGenerateMipmap(GL_TEXTURE_2D);

			#ifdef RENDERER_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLTextureBackup));
			#endif
		}
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void OpenGLRenderer::resetQueryPool([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)
		RENDERER_ASSERT(mContext, firstQueryIndex < static_cast<QueryPool&>(queryPool).getNumberOfQueries(), "OpenGL out-of-bounds query index")
		RENDERER_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= static_cast<QueryPool&>(queryPool).getNumberOfQueries(), "OpenGL out-of-bounds query index")

		// Nothing to do in here for OpenGL
	}

	void OpenGLRenderer::beginQuery(Renderer::IQueryPool& queryPool, uint32_t queryIndex, uint32_t)
	{
		// Sanity check
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// Query pool type dependent processing
		QueryPool& openGLQueryPool = static_cast<QueryPool&>(queryPool);
		RENDERER_ASSERT(mContext, queryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		switch (openGLQueryPool.getQueryType())
		{
			case Renderer::QueryType::OCCLUSION:
				// At this point in time we know that the "GL_ARB_occlusion_query"-extension is supported
				glBeginQueryARB(GL_SAMPLES_PASSED_ARB, static_cast<OcclusionTimestampQueryPool&>(openGLQueryPool).getOpenGLQueries()[queryIndex]);
				break;

			case Renderer::QueryType::PIPELINE_STATISTICS:
				// At this point in time we know that the "GL_ARB_pipeline_statistics_query"-extension is supported
				static_cast<PipelineStatisticsQueryPool&>(openGLQueryPool).beginQuery(queryIndex);
				break;

			case Renderer::QueryType::TIMESTAMP:
				RENDERER_ASSERT(mContext, false, "OpenGL begin query isn't allowed for timestamp queries, use \"Renderer::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void OpenGLRenderer::endQuery(Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// Query pool type dependent processing
		QueryPool& openGLQueryPool = static_cast<QueryPool&>(queryPool);
		RENDERER_ASSERT(mContext, queryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		switch (openGLQueryPool.getQueryType())
		{
			case Renderer::QueryType::OCCLUSION:
				// At this point in time we know that the "GL_ARB_occlusion_query"-extension is supported
				glEndQueryARB(GL_SAMPLES_PASSED_ARB);
				break;

			case Renderer::QueryType::PIPELINE_STATISTICS:
				// At this point in time we know that the "GL_ARB_pipeline_statistics_query"-extension is supported
				static_cast<PipelineStatisticsQueryPool&>(openGLQueryPool).endQuery();
				break;

			case Renderer::QueryType::TIMESTAMP:
				RENDERER_ASSERT(mContext, false, "OpenGL end query isn't allowed for timestamp queries, use \"Renderer::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void OpenGLRenderer::writeTimestampQuery(Renderer::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// Query pool type dependent processing
		QueryPool& openGLQueryPool = static_cast<QueryPool&>(queryPool);
		RENDERER_ASSERT(mContext, queryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		switch (openGLQueryPool.getQueryType())
		{
			case Renderer::QueryType::OCCLUSION:
				RENDERER_ASSERT(mContext, false, "OpenGL write timestamp query isn't allowed for occlusion queries, use \"Renderer::Command::BeginQuery\" and \"Renderer::Command::EndQuery\" instead")
				break;

			case Renderer::QueryType::PIPELINE_STATISTICS:
				RENDERER_ASSERT(mContext, false, "OpenGL write timestamp query isn't allowed for pipeline statistics queries, use \"Renderer::Command::BeginQuery\" and \"Renderer::Command::EndQuery\" instead")
				break;

			case Renderer::QueryType::TIMESTAMP:
				// At this point in time we know that the "GL_ARB_timer_query"-extension is supported
				glQueryCounter(static_cast<OcclusionTimestampQueryPool&>(openGLQueryPool).getOpenGLQueries()[queryIndex], GL_TIMESTAMP);
				break;
		}
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void OpenGLRenderer::setDebugMarker(const char* name)
		{
			// "GL_KHR_debug"-extension required
			if (mExtensions->isGL_KHR_debug())
			{
				RENDERER_ASSERT(mContext, nullptr != name, "OpenGL debug marker names must not be a null pointer")
				glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, name);
			}
		}

		void OpenGLRenderer::beginDebugEvent(const char* name)
		{
			// "GL_KHR_debug"-extension required
			if (mExtensions->isGL_KHR_debug())
			{
				RENDERER_ASSERT(mContext, nullptr != name, "OpenGL debug event names must not be a null pointer")
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, name);
			}
		}

		void OpenGLRenderer::endDebugEvent()
		{
			// "GL_KHR_debug"-extension required
			if (mExtensions->isGL_KHR_debug())
			{
				glPopDebugGroup();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	const char* OpenGLRenderer::getName() const
	{
		return "OpenGL";
	}

	bool OpenGLRenderer::isInitialized() const
	{
		// Is the OpenGL context initialized?
		return (nullptr != mOpenGLContext && mOpenGLContext->isInitialized());
	}

	bool OpenGLRenderer::isDebugEnabled()
	{
		// OpenGL has nothing that is similar to the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)

		// Debug disabled
		return false;
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t OpenGLRenderer::getNumberOfShaderLanguages() const
	{
		uint32_t numberOfShaderLanguages = 0;

		// "GL_ARB_shader_objects" or "GL_ARB_separate_shader_objects" required
		if (mExtensions->isGL_ARB_shader_objects() || mExtensions->isGL_ARB_separate_shader_objects())
		{
			// GLSL supported
			++numberOfShaderLanguages;
		}

		// Done, return the number of supported shader languages
		return numberOfShaderLanguages;
	}

	const char* OpenGLRenderer::getShaderLanguageName(uint32_t index) const
	{
		// "GL_ARB_shader_objects" or "GL_ARB_separate_shader_objects" required
		if (mExtensions->isGL_ARB_shader_objects() || mExtensions->isGL_ARB_separate_shader_objects())
		{
			// GLSL supported
			if (0 == index)
			{
				return ::detail::GLSL_NAME;
			}
		}

		// Error!
		return nullptr;
	}

	Renderer::IShaderLanguage* OpenGLRenderer::getShaderLanguage(const char* shaderLanguageName)
	{
		// "GL_ARB_shader_objects" or "GL_ARB_separate_shader_objects" required
		if (mExtensions->isGL_ARB_shader_objects() || mExtensions->isGL_ARB_separate_shader_objects())
		{
			// In case "shaderLanguage" is a null pointer, use the default shader language
			if (nullptr != shaderLanguageName)
			{
				// Optimization: Check for shader language name pointer match, first
				if (::detail::GLSL_NAME == shaderLanguageName || !stricmp(shaderLanguageName, ::detail::GLSL_NAME))
				{
					// Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
					if (mExtensions->isGL_ARB_separate_shader_objects())
					{
						// If required, create the separate shader language instance right now
						if (nullptr == mShaderLanguage)
						{
							mShaderLanguage = RENDERER_NEW(mContext, ShaderLanguageSeparate)(*this);
							mShaderLanguage->addReference();	// Internal renderer reference
						}

						// Return the shader language instance
						return mShaderLanguage;
					}
					else if (mExtensions->isGL_ARB_shader_objects())
					{
						// If required, create the monolithic shader language instance right now
						if (nullptr == mShaderLanguage)
						{
							mShaderLanguage = RENDERER_NEW(mContext, ShaderLanguageMonolithic)(*this);
							mShaderLanguage->addReference();	// Internal renderer reference
						}

						// Return the shader language instance
						return mShaderLanguage;
					}
				}
			}
			else
			{
				// Return the shader language instance as default
				return getShaderLanguage(::detail::GLSL_NAME);
			}
		}

		// Error!
		return nullptr;
	}


	//[-------------------------------------------------------]
	//[ Resource creation                                     ]
	//[-------------------------------------------------------]
	Renderer::IRenderPass* OpenGLRenderer::createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples)
	{
		return RENDERER_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples);
	}

	Renderer::IQueryPool* OpenGLRenderer::createQueryPool(Renderer::QueryType queryType, uint32_t numberOfQueries)
	{
		RENDERER_ASSERT(mContext, numberOfQueries > 0, "OpenGL: Number of queries mustn't be zero")
		switch (queryType)
		{
			case Renderer::QueryType::OCCLUSION:
				if (!mExtensions->isGL_ARB_occlusion_query())
				{
					RENDERER_LOG(mContext, CRITICAL, "OpenGL extension \"GL_ARB_occlusion_query\" isn't supported")
					return nullptr;
				}
				return RENDERER_NEW(mContext, OcclusionTimestampQueryPool)(*this, queryType, numberOfQueries);

			case Renderer::QueryType::PIPELINE_STATISTICS:
				if (!mExtensions->isGL_ARB_pipeline_statistics_query())
				{
					RENDERER_LOG(mContext, CRITICAL, "OpenGL extension \"GL_ARB_pipeline_statistics_query\" isn't supported")
					return nullptr;
				}
				return RENDERER_NEW(mContext, PipelineStatisticsQueryPool)(*this, queryType, numberOfQueries);

			case Renderer::QueryType::TIMESTAMP:
				if (!mExtensions->isGL_ARB_timer_query())
				{
					RENDERER_LOG(mContext, CRITICAL, "OpenGL extension \"GL_ARB_timer_query\" isn't supported")
					return nullptr;
				}
				return RENDERER_NEW(mContext, OcclusionTimestampQueryPool)(*this, queryType, numberOfQueries);
		}
		return nullptr;
	}

	Renderer::ISwapChain* OpenGLRenderer::createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool useExternalContext)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)
		RENDERER_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle || nullptr != windowHandle.renderWindow, "OpenGL: The provided native window handle or render window must not be a null handle / null pointer")

		// Create the swap chain
		return RENDERER_NEW(mContext, SwapChain)(renderPass, windowHandle, useExternalContext);
	}

	Renderer::IFramebuffer* OpenGLRenderer::createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment)
	{
		// Sanity check
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)

		// "GL_ARB_framebuffer_object" required
		if (mExtensions->isGL_ARB_framebuffer_object())
		{
			// Is "GL_EXT_direct_state_access" there?
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				// -> Validation is done inside the framebuffer implementation
				return RENDERER_NEW(mContext, FramebufferDsa)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment);
			}
			else
			{
				// Traditional bind version
				// -> Validation is done inside the framebuffer implementation
				return RENDERER_NEW(mContext, FramebufferBind)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment);
			}
		}
		else
		{
			// Error!
			return nullptr;
		}
	}

	Renderer::IBufferManager* OpenGLRenderer::createBufferManager()
	{
		return RENDERER_NEW(mContext, BufferManager)(*this);
	}

	Renderer::ITextureManager* OpenGLRenderer::createTextureManager()
	{
		return RENDERER_NEW(mContext, TextureManager)(*this);
	}

	Renderer::IRootSignature* OpenGLRenderer::createRootSignature(const Renderer::RootSignature& rootSignature)
	{
		return RENDERER_NEW(mContext, RootSignature)(*this, rootSignature);
	}

	Renderer::IGraphicsPipelineState* OpenGLRenderer::createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "OpenGL: Invalid graphics pipeline state root signature")
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "OpenGL: Invalid graphics pipeline state graphics program")
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "OpenGL: Invalid graphics pipeline state render pass")

		// Create graphics pipeline state
		uint16_t id = 0;
		if (GraphicsPipelineStateMakeId.CreateID(id))
		{
			return RENDERER_NEW(mContext, GraphicsPipelineState)(*this, graphicsPipelineState, id);
		}

		// Error: Ensure a correct reference counter behaviour
		graphicsPipelineState.rootSignature->addReference();
		graphicsPipelineState.rootSignature->releaseReference();
		graphicsPipelineState.graphicsProgram->addReference();
		graphicsPipelineState.graphicsProgram->releaseReference();
		graphicsPipelineState.renderPass->addReference();
		graphicsPipelineState.renderPass->releaseReference();
		return nullptr;
	}

	Renderer::IComputePipelineState* OpenGLRenderer::createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, rootSignature)
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, computeShader)

		// Create the compute pipeline state
		uint16_t id = 0;
		if ((mExtensions->isGL_ARB_separate_shader_objects() || mExtensions->isGL_ARB_shader_objects()) && ComputePipelineStateMakeId.CreateID(id))
		{
			// Create the compute pipeline state
			// -> Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
			if (mExtensions->isGL_ARB_separate_shader_objects())
			{
				return RENDERER_NEW(mContext, ComputePipelineStateSeparate)(*this, rootSignature, static_cast<ComputeShaderSeparate&>(computeShader), id);
			}
			else if (mExtensions->isGL_ARB_shader_objects())
			{
				return RENDERER_NEW(mContext, ComputePipelineStateMonolithic)(*this, rootSignature, static_cast<ComputeShaderMonolithic&>(computeShader), id);
			}
		}

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();
		return nullptr;
	}

	Renderer::ISamplerState* OpenGLRenderer::createSamplerState(const Renderer::SamplerState& samplerState)
	{
		// Is "GL_ARB_sampler_objects" there?
		if (mExtensions->isGL_ARB_sampler_objects())
		{
			// Effective sampler object (SO)
			return RENDERER_NEW(mContext, SamplerStateSo)(*this, samplerState);
		}

		// Is "GL_EXT_direct_state_access" there?
		else if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
		{
			// Direct state access (DSA) version to emulate a sampler object
			return RENDERER_NEW(mContext, SamplerStateDsa)(*this, samplerState);
		}
		else
		{
			// Traditional bind version to emulate a sampler object
			return RENDERER_NEW(mContext, SamplerStateBind)(*this, samplerState);
		}
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool OpenGLRenderer::map(Renderer::IResource& resource, uint32_t, Renderer::MapType mapType, uint32_t, Renderer::MappedSubresource& mappedSubresource)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_ELEMENT_ARRAY_BUFFER_ARB, GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, static_cast<IndexBuffer&>(resource).getOpenGLElementArrayBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::VERTEX_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_ARRAY_BUFFER_ARB, GL_ARRAY_BUFFER_BINDING_ARB, static_cast<VertexBuffer&>(resource).getOpenGLArrayBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::TEXTURE_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<TextureBuffer&>(resource).getOpenGLTextureBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::STRUCTURED_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<StructuredBuffer&>(resource).getOpenGLStructuredBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::INDIRECT_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_DRAW_INDIRECT_BUFFER, GL_DRAW_INDIRECT_BUFFER_BINDING, static_cast<IndirectBuffer&>(resource).getOpenGLIndirectBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::UNIFORM_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, static_cast<UniformBuffer&>(resource).getOpenGLUniformBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::TEXTURE_1D:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::TEXTURE_1D_ARRAY:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::TEXTURE_2D:
			{
				bool result = false;

				// TODO(co) Implement me
				/*
				// Begin debug event
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

				// Get the Direct3D 11 resource instance
				ID3D11Resource* d3d11Resource = nullptr;
				static_cast<Texture2D&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource);
				if (nullptr != d3d11Resource)
				{
					// Map the Direct3D 11 resource
					result = (S_OK == mD3D11DeviceContext->Map(d3d11Resource, subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

					// Release the Direct3D 11 resource instance
					d3d11Resource->Release();
				}

				// End debug event
				RENDERER_END_DEBUG_EVENT(this)
				*/

				// Done
				return result;
			}

			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			{
				bool result = false;

				// TODO(co) Implement me
				/*
				// Begin debug event
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

				// Get the Direct3D 11 resource instance
				ID3D11Resource* d3d11Resource = nullptr;
				static_cast<Texture2DArray&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource);
				if (nullptr != d3d11Resource)
				{
					// Map the Direct3D 11 resource
					result = (S_OK == mD3D11DeviceContext->Map(d3d11Resource, subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

					// Release the Direct3D 11 resource instance
					d3d11Resource->Release();
				}

				// End debug event
				RENDERER_END_DEBUG_EVENT(this)
				*/

				// Done
				return result;
			}

			case Renderer::ResourceType::TEXTURE_3D:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_PIXEL_UNPACK_BUFFER_ARB, GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, static_cast<Texture3D&>(resource).getOpenGLPixelUnpackBuffer(), mapType, mappedSubresource);

			case Renderer::ResourceType::TEXTURE_CUBE:
			{
				// TODO(co) Implement me
				return false;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can map, set known return values
				mappedSubresource.data		 = nullptr;
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;

				// Error!
				return false;
		}
	}

	void OpenGLRenderer::unmap(Renderer::IResource& resource, uint32_t)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_ELEMENT_ARRAY_BUFFER_ARB, GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, static_cast<IndexBuffer&>(resource).getOpenGLElementArrayBuffer());
				break;

			case Renderer::ResourceType::VERTEX_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_ARRAY_BUFFER_ARB, GL_ARRAY_BUFFER_BINDING_ARB, static_cast<VertexBuffer&>(resource).getOpenGLArrayBuffer());
				break;

			case Renderer::ResourceType::TEXTURE_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<TextureBuffer&>(resource).getOpenGLTextureBuffer());
				break;

			case Renderer::ResourceType::STRUCTURED_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<StructuredBuffer&>(resource).getOpenGLStructuredBuffer());
				break;

			case Renderer::ResourceType::INDIRECT_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_DRAW_INDIRECT_BUFFER, GL_DRAW_INDIRECT_BUFFER_BINDING, static_cast<IndirectBuffer&>(resource).getOpenGLIndirectBuffer());
				break;

			case Renderer::ResourceType::UNIFORM_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, static_cast<UniformBuffer&>(resource).getOpenGLUniformBuffer());
				break;

			case Renderer::ResourceType::TEXTURE_1D:
			{
				// TODO(co) Implement me
				break;
			}

			case Renderer::ResourceType::TEXTURE_1D_ARRAY:
			{
				// TODO(co) Implement me
				break;
			}

			case Renderer::ResourceType::TEXTURE_2D:
			{
				// TODO(co) Implement me
				/*
				// Get the Direct3D 11 resource instance
				ID3D11Resource* d3d11Resource = nullptr;
				static_cast<Texture2D&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource);
				if (nullptr != d3d11Resource)
				{
					// Unmap the Direct3D 11 resource
					mD3D11DeviceContext->Unmap(d3d11Resource, subresource);

					// Release the Direct3D 11 resource instance
					d3d11Resource->Release();
				}
				*/
				break;
			}

			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			{
				// TODO(co) Implement me
				/*
				// Get the Direct3D 11 resource instance
				ID3D11Resource* d3d11Resource = nullptr;
				static_cast<Texture2DArray&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource);
				if (nullptr != d3d11Resource)
				{
					// Unmap the Direct3D 11 resource
					mD3D11DeviceContext->Unmap(d3d11Resource, subresource);

					// Release the Direct3D 11 resource instance
					d3d11Resource->Release();
				}
				*/
				break;
			}

			case Renderer::ResourceType::TEXTURE_3D:
			{
				// Unmap pixel unpack buffer
				const Texture3D& texture3D = static_cast<Texture3D&>(resource);
				const Renderer::TextureFormat::Enum textureFormat = texture3D.getTextureFormat();
				const uint32_t openGLPixelUnpackBuffer = texture3D.getOpenGLPixelUnpackBuffer();
				::detail::unmapBuffer(*mExtensions, GL_PIXEL_UNPACK_BUFFER_ARB, GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, openGLPixelUnpackBuffer);

				// Backup the currently set alignment and currently bound OpenGL pixel unpack buffer
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					GLint openGLAlignmentBackup = 0;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
					GLint openGLUnpackBufferBackup = 0;
					glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &openGLUnpackBufferBackup);
				#endif
				glPixelStorei(GL_UNPACK_ALIGNMENT, (Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

				// Copy pixel unpack buffer to texture
				glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, openGLPixelUnpackBuffer);
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					if (mExtensions->isGL_ARB_direct_state_access())
					{
						glTextureSubImage3D(texture3D.getOpenGLTexture(), 0, 0, 0, 0, static_cast<GLsizei>(texture3D.getWidth()), static_cast<GLsizei>(texture3D.getHeight()), static_cast<GLsizei>(texture3D.getDepth()), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), 0);
					}
					else
					{
						glTextureSubImage3DEXT(texture3D.getOpenGLTexture(), GL_TEXTURE_3D, 0, 0, 0, 0, static_cast<GLsizei>(texture3D.getWidth()), static_cast<GLsizei>(texture3D.getHeight()), static_cast<GLsizei>(texture3D.getDepth()), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), 0);
					}
				}
				else
				{
					// Traditional bind version

					// Backup the currently bound OpenGL texture
					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						GLint openGLTextureBackup = 0;
						glGetIntegerv(GL_TEXTURE_BINDING_3D, &openGLTextureBackup);
					#endif

					// Copy pixel unpack buffer to texture
					glBindTexture(GL_TEXTURE_3D, texture3D.getOpenGLTexture());
					glTexSubImage3DEXT(GL_TEXTURE_3D, 0, 0, 0, 0, static_cast<GLsizei>(texture3D.getWidth()), static_cast<GLsizei>(texture3D.getHeight()), static_cast<GLsizei>(texture3D.getDepth()), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), 0);

					// Be polite and restore the previous bound OpenGL texture
					#ifdef RENDERER_OPENGL_STATE_CLEANUP
						glBindTexture(GL_TEXTURE_3D, static_cast<GLuint>(openGLTextureBackup));
					#endif
				}

				// Restore previous alignment and pixel unpack buffer
				#ifdef RENDERER_OPENGL_STATE_CLEANUP
					glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, static_cast<GLuint>(openGLUnpackBufferBackup));
				#else
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
				#endif
				break;
			}

			case Renderer::ResourceType::TEXTURE_CUBE:
			{
				// TODO(co) Implement me
				break;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
			case Renderer::ResourceType::SAMPLER_STATE:
			case Renderer::ResourceType::VERTEX_SHADER:
			case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Renderer::ResourceType::GEOMETRY_SHADER:
			case Renderer::ResourceType::FRAGMENT_SHADER:
			case Renderer::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can unmap
				break;
		}
	}

	bool OpenGLRenderer::getQueryPoolResults(Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, uint32_t strideInBytes, uint32_t queryResultFlags)
	{
		// Sanity checks
		OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)
		RENDERER_ASSERT(mContext, numberOfDataBytes >= sizeof(UINT64), "OpenGL out-of-memory query access")
		RENDERER_ASSERT(mContext, 1 == numberOfQueries || strideInBytes > 0, "OpenGL invalid stride in bytes")
		RENDERER_ASSERT(mContext, numberOfDataBytes >= strideInBytes * numberOfQueries, "OpenGL out-of-memory query access")
		RENDERER_ASSERT(mContext, nullptr != data, "OpenGL out-of-memory query access")
		RENDERER_ASSERT(mContext, numberOfQueries > 0, "OpenGL number of queries mustn't be zero")

		// Query pool type dependent processing
		bool resultAvailable = true;
		const QueryPool& openGLQueryPool = static_cast<QueryPool&>(queryPool);
		RENDERER_ASSERT(mContext, firstQueryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		RENDERER_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		const bool waitForResult = ((queryResultFlags & Renderer::QueryResultFlags::WAIT) != 0);
		switch (openGLQueryPool.getQueryType())
		{
			case Renderer::QueryType::OCCLUSION:
			case Renderer::QueryType::TIMESTAMP:	// OpenGL return the time in nanoseconds
			{
				uint8_t* currentData = data;
				const GLuint* openGLQueries = static_cast<const OcclusionTimestampQueryPool&>(openGLQueryPool).getOpenGLQueries();
				for (uint32_t i = 0; i  < numberOfQueries; ++i)
				{
					const GLuint openGLQuery = openGLQueries[firstQueryIndex + i];
					GLuint openGLQueryResult = GL_FALSE;
					do
					{
						glGetQueryObjectuivARB(openGLQuery, GL_QUERY_RESULT_AVAILABLE_ARB, &openGLQueryResult);
					}
					while (waitForResult && GL_TRUE != openGLQueryResult);
					if (GL_TRUE == openGLQueryResult)
					{
						glGetQueryObjectuivARB(openGLQuery, GL_QUERY_RESULT_ARB, &openGLQueryResult);
						*reinterpret_cast<uint64_t*>(currentData) = openGLQueryResult;
					}
					else
					{
						// Result not ready
						resultAvailable = false;
						break;
					}
					currentData += strideInBytes;
				}
				break;
			}

			case Renderer::QueryType::PIPELINE_STATISTICS:
				RENDERER_ASSERT(mContext, numberOfDataBytes >= sizeof(Renderer::PipelineStatisticsQueryResult), "OpenGL out-of-memory query access")
				RENDERER_ASSERT(mContext, 1 == numberOfQueries || strideInBytes >= sizeof(Renderer::PipelineStatisticsQueryResult), "OpenGL out-of-memory query access")
				resultAvailable = static_cast<const PipelineStatisticsQueryPool&>(openGLQueryPool).getQueryPoolResults(data, firstQueryIndex, numberOfQueries, strideInBytes, waitForResult);
				break;
		}

		// Done
		return resultAvailable;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool OpenGLRenderer::beginScene()
	{
		// Not required when using OpenGL

		// Done
		return true;
	}

	void OpenGLRenderer::submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer)
	{
		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Renderer::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Submit command packet
				const Renderer::CommandDispatchFunctionIndex commandDispatchFunctionIndex = Renderer::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket);
				const void* command = Renderer::CommandPacketHelper::loadCommand(constCommandPacket);
				detail::DISPATCH_FUNCTIONS[commandDispatchFunctionIndex](command, *this);
			}

			{ // Next command
				const uint32_t nextCommandPacketByteIndex = Renderer::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
				constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
			}
		}
	}

	void OpenGLRenderer::endScene()
	{
		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);

		// We need to forget about the currently set vertex array
		unsetGraphicsVertexArray();
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void OpenGLRenderer::flush()
	{
		glFlush();
	}

	void OpenGLRenderer::finish()
	{
		glFinish();
	}


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void OpenGLRenderer::debugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int, const char* message, const void* userParam)
		{
			// Source to string
			char debugSource[20 + 1]{0};	// +1 for terminating zero
			switch (source)
			{
				case GL_DEBUG_SOURCE_API_ARB:
					strncpy(debugSource, "OpenGL", 20);
					break;

				case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
					strncpy(debugSource, "Windows", 20);
					break;

				case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
					strncpy(debugSource, "Shader compiler", 20);
					break;

				case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
					strncpy(debugSource, "Third party", 20);
					break;

				case GL_DEBUG_SOURCE_APPLICATION_ARB:
					strncpy(debugSource, "Application", 20);
					break;

				case GL_DEBUG_SOURCE_OTHER_ARB:
					strncpy(debugSource, "Other", 20);
					break;

				default:
					strncpy(debugSource, "?", 20);
					break;
			}

			// Debug type to string
			Renderer::ILog::Type logType = Renderer::ILog::Type::CRITICAL;
			char debugType[25 + 1]{0};	// +1 for terminating zero
			switch (type)
			{
				case GL_DEBUG_TYPE_ERROR_ARB:
					strncpy(debugType, "Error", 25);
					break;

				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
					logType = Renderer::ILog::Type::COMPATIBILITY_WARNING;
					strncpy(debugType, "Deprecated behavior", 25);
					break;

				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
					strncpy(debugType, "Undefined behavior", 25);
					break;

				case GL_DEBUG_TYPE_PORTABILITY_ARB:
					logType = Renderer::ILog::Type::COMPATIBILITY_WARNING;
					strncpy(debugType, "Portability", 25);
					break;

				case GL_DEBUG_TYPE_PERFORMANCE_ARB:
					logType = Renderer::ILog::Type::PERFORMANCE_WARNING;
					strncpy(debugType, "Performance", 25);
					break;

				case GL_DEBUG_TYPE_OTHER_ARB:
					strncpy(debugType, "Other", 25);
					break;

				case GL_DEBUG_TYPE_MARKER:
					strncpy(debugType, "Marker", 25);
					break;

				case GL_DEBUG_TYPE_PUSH_GROUP:
					strncpy(debugType, "Push group", 25);
					break;

				case GL_DEBUG_TYPE_POP_GROUP:
					strncpy(debugType, "Pop group", 25);
					break;

				default:
					strncpy(debugType, "?", 25);
					break;
			}

			// Debug severity to string
			char debugSeverity[20 + 1]{0};	// +1 for terminating zero
			switch (severity)
			{
				case GL_DEBUG_SEVERITY_HIGH_ARB:
					strncpy(debugSeverity, "High", 20);
					break;

				case GL_DEBUG_SEVERITY_MEDIUM_ARB:
					strncpy(debugSeverity, "Medium", 20);
					break;

				case GL_DEBUG_SEVERITY_LOW_ARB:
					strncpy(debugSeverity, "Low", 20);
					break;

				case GL_DEBUG_SEVERITY_NOTIFICATION:
					strncpy(debugSeverity, "Notification", 20);
					break;

				default:
					strncpy(debugSeverity, "?", 20);
					break;
			}

			// Print into log
			if (static_cast<const OpenGLRenderer*>(userParam)->getContext().getLog().print(logType, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), "OpenGL debug message\tSource:\"%s\"\tType:\"%s\"\tID:\"%u\"\tSeverity:\"%s\"\tMessage:\"%s\"", debugSource, debugType, id, debugSeverity, message))
			{
				DEBUG_BREAK;
			}
		}
	#else
		void OpenGLRenderer::debugMessageCallback(uint32_t, uint32_t, uint32_t, uint32_t, int, const char*, const void*)
		{
			// Nothing here
		}
	#endif


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void OpenGLRenderer::initializeCapabilities()
	{
		GLint openGLValue = 0;

		{ // Get device name
		  // -> OpenGL 4.3 Compatibility Profile Specification, section 22.2, page 627: "String queries return pointers to UTF-8 encoded, null-terminated static strings describing properties of the current GL context."
		  // -> For example "AMD Radeon R9 200 Series"
			const size_t numberOfCharacters = ::detail::countof(mCapabilities.deviceName) - 1;
			strncpy(mCapabilities.deviceName, reinterpret_cast<const char*>(glGetString(GL_RENDERER)), numberOfCharacters);
			mCapabilities.deviceName[numberOfCharacters] = '\0';
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Renderer::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		// TODO(co) "GL_ARB_viewport_array" support ("OpenGLRenderer::setGraphicsViewports()")
		mCapabilities.maximumNumberOfViewports = 1;	// TODO(co) GL_ARB_viewport_array

		// Maximum number of simultaneous render targets (if <1 render to texture is not supported, "GL_ARB_draw_buffers" required)
		if (mExtensions->isGL_ARB_draw_buffers())
		{
			glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &openGLValue);
			mCapabilities.maximumNumberOfSimultaneousRenderTargets = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			// "GL_ARB_framebuffer_object"-extension for render to texture required
			mCapabilities.maximumNumberOfSimultaneousRenderTargets = static_cast<uint32_t>(mExtensions->isGL_ARB_framebuffer_object());
		}

		// Maximum texture dimension
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &openGLValue);
		mCapabilities.maximumTextureDimension = static_cast<uint32_t>(openGLValue);

		// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
		// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
		if (mExtensions->isGL_EXT_texture_array())
		{
			glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &openGLValue);
			mCapabilities.maximumNumberOf1DTextureArraySlices = static_cast<uint32_t>(openGLValue);
			mCapabilities.maximumNumberOf2DTextureArraySlices = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumNumberOf1DTextureArraySlices = 0;
			mCapabilities.maximumNumberOf2DTextureArraySlices = 0;
		}

		// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		if (mExtensions->isGL_ARB_texture_buffer_object())
		{
			glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE_EXT, &openGLValue);
			mCapabilities.maximumTextureBufferSize = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumTextureBufferSize = 0;
		}

		// Maximum structured buffer size in bytes
		if (mExtensions->isGL_ARB_shader_storage_buffer_object())
		{
			glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &openGLValue);
			mCapabilities.maximumStructuredBufferSize = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumStructuredBufferSize = 0;
		}

		// Maximum indirect buffer size in bytes
		if (mExtensions->isGL_ARB_draw_indirect())
		{
			// TODO(co) How to get the indirect buffer maximum size? Didn't find any information about this.
			mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB
		}
		else
		{
			mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB
		}

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		if (mExtensions->isGL_ARB_uniform_buffer_object())
		{
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &openGLValue);
			mCapabilities.maximumUniformBufferSize = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumUniformBufferSize = 0;
		}

		// Maximum number of multisamples (always at least 1, usually 8)
		if (mExtensions->isGL_ARB_texture_multisample())
		{
			glGetIntegerv(GL_MAX_SAMPLES, &openGLValue);
			if (openGLValue > 8)
			{
				// Limit to known maximum we can test, even if e.g. GeForce 980m reports 32 here
				openGLValue = 8;
			}
			mCapabilities.maximumNumberOfMultisamples = static_cast<uint8_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumNumberOfMultisamples = 1;
		}

		// Maximum anisotropy (always at least 1, usually 16)
		// -> "GL_EXT_texture_filter_anisotropic"-extension
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &openGLValue);
		mCapabilities.maximumAnisotropy = static_cast<uint8_t>(openGLValue);

		// Coordinate system
		// -> If the "GL_ARB_clip_control"-extension is available: Left-handed coordinate system with clip space depth value range 0..1
		// -> If the "GL_ARB_clip_control"-extension isn't available: Right-handed coordinate system with clip space depth value range -1..1
		// -> For background theory see "Depth Precision Visualized" by Nathan Reed - https://developer.nvidia.com/content/depth-precision-visualized
		// -> For practical information see "Reversed-Z in OpenGL" by Nicolas Guillemot - https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
		// -> Shaders might want to take the following into account: "Mac computers that use OpenCL and OpenGL graphics" - https://support.apple.com/en-us/HT202823 - "iMac (Retina 5K, 27-inch, 2017)" - OpenGL 4.1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = mExtensions->isGL_ARB_clip_control();

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = true;

		// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex, "GL_ARB_instanced_arrays" required)
		mCapabilities.instancedArrays = mExtensions->isGL_ARB_instanced_arrays();

		// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID, "GL_ARB_draw_instanced" required)
		mCapabilities.drawInstanced = mExtensions->isGL_ARB_draw_instanced();

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = mExtensions->isGL_ARB_draw_elements_base_vertex();

		// OpenGL has no native multithreading
		mCapabilities.nativeMultithreading = false;

		// We don't support the OpenGL program binaries since those are operation system and graphics driver version dependent, which renders them useless for pre-compiled shaders shipping
		mCapabilities.shaderBytecode = mExtensions->isGL_ARB_gl_spirv();

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = mExtensions->isGL_ARB_vertex_shader();

		// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		if (mExtensions->isGL_ARB_tessellation_shader())
		{
			glGetIntegerv(GL_MAX_PATCH_VERTICES, &openGLValue);
			mCapabilities.maximumNumberOfPatchVertices = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumNumberOfPatchVertices = 0;
		}

		// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
		if (mExtensions->isGL_ARB_geometry_shader4())
		{
			glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB, &openGLValue);
			mCapabilities.maximumNumberOfGsOutputVertices = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumNumberOfGsOutputVertices = 0;
		}

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = mExtensions->isGL_ARB_fragment_shader();

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = mExtensions->isGL_ARB_compute_shader();
	}

	void OpenGLRenderer::unsetGraphicsVertexArray()
	{
		// Release the currently used vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			// Evaluate the internal array type of the currently set vertex array
			switch (static_cast<VertexArray*>(mVertexArray)->getInternalResourceType())
			{
				case VertexArray::InternalResourceType::NO_VAO:
					// Disable OpenGL vertex attribute arrays
					static_cast<VertexArrayNoVao*>(mVertexArray)->disableOpenGLVertexAttribArrays();
					break;

				case VertexArray::InternalResourceType::VAO:
					// Unbind OpenGL vertex array
					// -> No need to check for "GL_ARB_vertex_array_object", in case were in here we know it must exist
					glBindVertexArray(mDefaultOpenGLVertexArray);
					break;
			}

			// Release reference
			mVertexArray->releaseReference();
			mVertexArray = nullptr;
		}
	}

	void OpenGLRenderer::setResourceGroup(const RootSignature& rootSignature, uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		if (nullptr != resourceGroup)
		{
			// Sanity check
			OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resourceGroup)

			// Set resource group
			const ResourceGroup* openGLResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = openGLResourceGroup->getNumberOfResources();
			Renderer::IResource** resources = openGLResourceGroup->getResources();
			const Renderer::RootParameter& rootParameter = rootSignature.getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				Renderer::IResource* resource = *resources;
				RENDERER_ASSERT(mContext, nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
				const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Renderer::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_1D_ARRAY:
					case Renderer::ResourceType::TEXTURE_2D:
					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					{
						// In OpenGL, all shaders share the same texture units (= "Renderer::RootParameter::shaderVisibility" stays unused)
						switch (descriptorRange.rangeType)
						{
							case Renderer::DescriptorRangeType::SRV:
							{
								// Is "GL_ARB_direct_state_access" or "GL_EXT_direct_state_access" there?
								if (mExtensions->isGL_ARB_direct_state_access() || mExtensions->isGL_EXT_direct_state_access())
								{
									// Effective direct state access (DSA)
									const bool isArbDsa = mExtensions->isGL_ARB_direct_state_access();

									// "glBindTextureUnit()" unit parameter is zero based so we can simply use the value we received
									const GLuint unit = descriptorRange.baseShaderRegister;

									// TODO(co) Some security checks might be wise *maximum number of texture units*
									// Evaluate the texture type
									switch (resourceType)
									{
										case Renderer::ResourceType::TEXTURE_BUFFER:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<TextureBuffer*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_BUFFER_ARB, static_cast<TextureBuffer*>(resource)->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::TEXTURE_1D:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture1D*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												const Texture1D* texture1D = static_cast<Texture1D*>(resource);
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_1D, texture1D->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::TEXTURE_1D_ARRAY:
											// No texture 1D array extension check required, if we in here we already know it must exist
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture1DArray*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_1D_ARRAY_EXT, static_cast<Texture1DArray*>(resource)->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::TEXTURE_2D:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture2D*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												const Texture2D* texture2D = static_cast<Texture2D*>(resource);
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::TEXTURE_2D_ARRAY:
											// No texture 2D array extension check required, if we in here we already know it must exist
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture2DArray*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_2D_ARRAY_EXT, static_cast<Texture2DArray*>(resource)->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::TEXTURE_3D:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture3D*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												const Texture3D* texture3D = static_cast<Texture3D*>(resource);
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_3D, texture3D->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::TEXTURE_CUBE:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<TextureCube*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												const TextureCube* textureCube = static_cast<TextureCube*>(resource);
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_CUBE_MAP, textureCube->getOpenGLTexture());
											}
											break;

										case Renderer::ResourceType::ROOT_SIGNATURE:
										case Renderer::ResourceType::RESOURCE_GROUP:
										case Renderer::ResourceType::GRAPHICS_PROGRAM:
										case Renderer::ResourceType::VERTEX_ARRAY:
										case Renderer::ResourceType::RENDER_PASS:
										case Renderer::ResourceType::QUERY_POOL:
										case Renderer::ResourceType::SWAP_CHAIN:
										case Renderer::ResourceType::FRAMEBUFFER:
										case Renderer::ResourceType::INDEX_BUFFER:
										case Renderer::ResourceType::VERTEX_BUFFER:
										case Renderer::ResourceType::STRUCTURED_BUFFER:
										case Renderer::ResourceType::INDIRECT_BUFFER:
										case Renderer::ResourceType::UNIFORM_BUFFER:
										case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
										case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
										case Renderer::ResourceType::SAMPLER_STATE:
										case Renderer::ResourceType::VERTEX_SHADER:
										case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
										case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
										case Renderer::ResourceType::GEOMETRY_SHADER:
										case Renderer::ResourceType::FRAGMENT_SHADER:
										case Renderer::ResourceType::COMPUTE_SHADER:
											RENDERER_LOG(mContext, CRITICAL, "Invalid OpenGL renderer backend resource type")
											break;
									}

									// Set the OpenGL sampler states, if required (texture buffer has no sampler state), it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
									if (Renderer::ResourceType::TEXTURE_BUFFER != resourceType && nullptr != openGLResourceGroup->getSamplerState())
									{
										const SamplerState* samplerState = static_cast<const SamplerState*>(openGLResourceGroup->getSamplerState()[resourceIndex]);
										if (nullptr != samplerState)
										{
											// Is "GL_ARB_sampler_objects" there?
											if (mExtensions->isGL_ARB_sampler_objects())
											{
												// Effective sampler object (SO)
												glBindSampler(descriptorRange.baseShaderRegister, static_cast<const SamplerStateSo*>(samplerState)->getOpenGLSampler());
											}
											else
											{
												#ifdef RENDERER_OPENGL_STATE_CLEANUP
													// Backup the currently active OpenGL texture
													GLint openGLActiveTextureBackup = 0;
													glGetIntegerv(GL_ACTIVE_TEXTURE, &openGLActiveTextureBackup);
												#endif

												// TODO(co) Some security checks might be wise *maximum number of texture units*
												// Activate the texture unit we want to manipulate
												// -> "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glActiveTextureARB(GL_TEXTURE0_ARB + unit);

												// Is "GL_EXT_direct_state_access" there?
												if (mExtensions->isGL_EXT_direct_state_access())
												{
													// Direct state access (DSA) version to emulate a sampler object
													static_cast<const SamplerStateDsa*>(samplerState)->setOpenGLSamplerStates();
												}
												else
												{
													// Traditional bind version to emulate a sampler object
													static_cast<const SamplerStateBind*>(samplerState)->setOpenGLSamplerStates();
												}

												#ifdef RENDERER_OPENGL_STATE_CLEANUP
													// Be polite and restore the previous active OpenGL texture
													glActiveTextureARB(static_cast<GLenum>(openGLActiveTextureBackup));
												#endif
											}
										}
									}
								}
								else
								{
									// Traditional bind version

									// "GL_ARB_multitexture" required
									if (mExtensions->isGL_ARB_multitexture())
									{
										#ifdef RENDERER_OPENGL_STATE_CLEANUP
											// Backup the currently active OpenGL texture
											GLint openGLActiveTextureBackup = 0;
											glGetIntegerv(GL_ACTIVE_TEXTURE, &openGLActiveTextureBackup);
										#endif

										// TODO(co) Some security checks might be wise *maximum number of texture units*
										// Activate the texture unit we want to manipulate
										// -> "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
										glActiveTextureARB(GL_TEXTURE0_ARB + descriptorRange.baseShaderRegister);

										// Evaluate the resource type
										switch (resourceType)
										{
											case Renderer::ResourceType::TEXTURE_BUFFER:
												glBindTexture(GL_TEXTURE_BUFFER_ARB, static_cast<TextureBuffer*>(resource)->getOpenGLTexture());
												break;

											case Renderer::ResourceType::TEXTURE_1D:
												glBindTexture(GL_TEXTURE_1D, static_cast<Texture1D*>(resource)->getOpenGLTexture());
												break;

											case Renderer::ResourceType::TEXTURE_1D_ARRAY:
												// No extension check required, if we in here we already know it must exist
												glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, static_cast<Texture1DArray*>(resource)->getOpenGLTexture());
												break;

											case Renderer::ResourceType::TEXTURE_2D:
											{
												const Texture2D* texture2D = static_cast<Texture2D*>(resource);
												glBindTexture(static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture());
												break;
											}

											case Renderer::ResourceType::TEXTURE_2D_ARRAY:
												// No extension check required, if we in here we already know it must exist
												glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, static_cast<Texture2DArray*>(resource)->getOpenGLTexture());
												break;

											case Renderer::ResourceType::TEXTURE_3D:
												glBindTexture(GL_TEXTURE_3D, static_cast<Texture3D*>(resource)->getOpenGLTexture());
												break;

											case Renderer::ResourceType::TEXTURE_CUBE:
												glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<TextureCube*>(resource)->getOpenGLTexture());
												break;

											case Renderer::ResourceType::ROOT_SIGNATURE:
											case Renderer::ResourceType::RESOURCE_GROUP:
											case Renderer::ResourceType::GRAPHICS_PROGRAM:
											case Renderer::ResourceType::VERTEX_ARRAY:
											case Renderer::ResourceType::RENDER_PASS:
											case Renderer::ResourceType::QUERY_POOL:
											case Renderer::ResourceType::SWAP_CHAIN:
											case Renderer::ResourceType::FRAMEBUFFER:
											case Renderer::ResourceType::INDEX_BUFFER:
											case Renderer::ResourceType::VERTEX_BUFFER:
											case Renderer::ResourceType::STRUCTURED_BUFFER:
											case Renderer::ResourceType::INDIRECT_BUFFER:
											case Renderer::ResourceType::UNIFORM_BUFFER:
											case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
											case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
											case Renderer::ResourceType::SAMPLER_STATE:
											case Renderer::ResourceType::VERTEX_SHADER:
											case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
											case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
											case Renderer::ResourceType::GEOMETRY_SHADER:
											case Renderer::ResourceType::FRAGMENT_SHADER:
											case Renderer::ResourceType::COMPUTE_SHADER:
												RENDERER_LOG(mContext, CRITICAL, "Invalid OpenGL renderer backend resource type")
												break;
										}

										// Set the OpenGL sampler states, if required (texture buffer has no sampler state), it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
										if (Renderer::ResourceType::TEXTURE_BUFFER != resourceType)
										{
											RENDERER_ASSERT(mContext, nullptr != openGLResourceGroup->getSamplerState(), "Invalid OpenGL sampler state")
											const SamplerState* samplerState = static_cast<const SamplerState*>(openGLResourceGroup->getSamplerState()[resourceIndex]);
											if (nullptr != samplerState)
											{
												// Is "GL_ARB_sampler_objects" there?
												if (mExtensions->isGL_ARB_sampler_objects())
												{
													// Effective sampler object (SO)
													glBindSampler(descriptorRange.baseShaderRegister, static_cast<const SamplerStateSo*>(samplerState)->getOpenGLSampler());
												}
												// Is "GL_EXT_direct_state_access" there?
												else if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
												{
													// Direct state access (DSA) version to emulate a sampler object
													static_cast<const SamplerStateDsa*>(samplerState)->setOpenGLSamplerStates();
												}
												else
												{
													// Traditional bind version to emulate a sampler object
													static_cast<const SamplerStateBind*>(samplerState)->setOpenGLSamplerStates();
												}
											}
										}

										#ifdef RENDERER_OPENGL_STATE_CLEANUP
											// Be polite and restore the previous active OpenGL texture
											glActiveTextureARB(static_cast<GLenum>(openGLActiveTextureBackup));
										#endif
									}
								}
								break;
							}

							case Renderer::DescriptorRangeType::UAV:
							{
								// Is "GL_EXT_shader_image_load_store" there?
								if (mExtensions->isGL_EXT_shader_image_load_store())
								{
									// "glBindImageTextureEXT()" unit parameter is zero based so we can simply use the value we received
									const GLuint unit = descriptorRange.baseShaderRegister;

									// TODO(co) Some security checks might be wise *maximum number of texture units*
									// Evaluate the texture type
									switch (resourceType)
									{
										case Renderer::ResourceType::TEXTURE_BUFFER:
										{
											const TextureBuffer* textureBuffer = static_cast<TextureBuffer*>(resource);
											glBindImageTextureEXT(unit, textureBuffer->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(textureBuffer->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::TEXTURE_1D:
										{
											const Texture1D* texture1D = static_cast<Texture1D*>(resource);
											glBindImageTextureEXT(unit, texture1D->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture1D->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::TEXTURE_1D_ARRAY:
										{
											const Texture1DArray* texture1DArray = static_cast<Texture1DArray*>(resource);
											glBindImageTextureEXT(unit, texture1DArray->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture1DArray->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::TEXTURE_2D:
										{
											const Texture2D* texture2D = static_cast<Texture2D*>(resource);
											glBindImageTextureEXT(unit, texture2D->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture2D->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::TEXTURE_2D_ARRAY:
										{
											const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(resource);
											glBindImageTextureEXT(unit, texture2DArray->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture2DArray->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::TEXTURE_3D:
										{
											const Texture3D* texture3D = static_cast<Texture3D*>(resource);
											glBindImageTextureEXT(unit, texture3D->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture3D->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::TEXTURE_CUBE:
										{
											const TextureCube* textureCube = static_cast<TextureCube*>(resource);
											glBindImageTextureEXT(unit, textureCube->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(textureCube->getOpenGLInternalFormat()));
											break;
										}

										case Renderer::ResourceType::ROOT_SIGNATURE:
										case Renderer::ResourceType::RESOURCE_GROUP:
										case Renderer::ResourceType::GRAPHICS_PROGRAM:
										case Renderer::ResourceType::VERTEX_ARRAY:
										case Renderer::ResourceType::RENDER_PASS:
										case Renderer::ResourceType::QUERY_POOL:
										case Renderer::ResourceType::SWAP_CHAIN:
										case Renderer::ResourceType::FRAMEBUFFER:
										case Renderer::ResourceType::INDEX_BUFFER:
										case Renderer::ResourceType::VERTEX_BUFFER:
										case Renderer::ResourceType::STRUCTURED_BUFFER:
										case Renderer::ResourceType::INDIRECT_BUFFER:
										case Renderer::ResourceType::UNIFORM_BUFFER:
										case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
										case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
										case Renderer::ResourceType::SAMPLER_STATE:
										case Renderer::ResourceType::VERTEX_SHADER:
										case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
										case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
										case Renderer::ResourceType::GEOMETRY_SHADER:
										case Renderer::ResourceType::FRAGMENT_SHADER:
										case Renderer::ResourceType::COMPUTE_SHADER:
											RENDERER_LOG(mContext, CRITICAL, "Invalid OpenGL renderer backend resource type")
											break;
									}
								}
								break;
							}

							case Renderer::DescriptorRangeType::UBV:
							case Renderer::DescriptorRangeType::SAMPLER:
							case Renderer::DescriptorRangeType::NUMBER_OF_RANGE_TYPES:
								RENDERER_LOG(mContext, CRITICAL, "Invalid OpenGL descriptor range type")
								break;
						}
						break;
					}

					case Renderer::ResourceType::INDEX_BUFFER:
					{
						RENDERER_ASSERT(mContext, Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL index buffer must bound at SRV or UAV descriptor range type")
						RENDERER_ASSERT(mContext, Renderer::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Renderer::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "OpenGL descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")

						// "GL_ARB_uniform_buffer_object" required
						if (mExtensions->isGL_ARB_uniform_buffer_object())
						{
							// "glBindBufferBase()" unit parameter is zero based so we can simply use the value we received
							const GLuint index = descriptorRange.baseShaderRegister;

							// Attach the buffer to the given SSBO binding point
							glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, static_cast<IndexBuffer*>(resource)->getOpenGLElementArrayBuffer());
						}
						break;
					}

					case Renderer::ResourceType::VERTEX_BUFFER:
					{
						RENDERER_ASSERT(mContext, Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL vertex buffer must bound at SRV or UAV descriptor range type")
						RENDERER_ASSERT(mContext, Renderer::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Renderer::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "OpenGL descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")

						// "GL_ARB_uniform_buffer_object" required
						if (mExtensions->isGL_ARB_uniform_buffer_object())
						{
							// "glBindBufferBase()" unit parameter is zero based so we can simply use the value we received
							const GLuint index = descriptorRange.baseShaderRegister;

							// Attach the buffer to the given SSBO binding point
							glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, static_cast<VertexBuffer*>(resource)->getOpenGLArrayBuffer());
						}
						break;
					}

					case Renderer::ResourceType::STRUCTURED_BUFFER:
					{
						RENDERER_ASSERT(mContext, Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL structured buffer must bound at SRV or UAV descriptor range type")

						// "GL_ARB_uniform_buffer_object" required
						if (mExtensions->isGL_ARB_uniform_buffer_object())
						{
							// "glBindBufferBase()" unit parameter is zero based so we can simply use the value we received
							const GLuint index = descriptorRange.baseShaderRegister;

							// Attach the buffer to the given SSBO binding point
							glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, static_cast<StructuredBuffer*>(resource)->getOpenGLStructuredBuffer());
						}
						break;
					}

					case Renderer::ResourceType::INDIRECT_BUFFER:
					{
						RENDERER_ASSERT(mContext, Renderer::DescriptorRangeType::SRV == descriptorRange.rangeType || Renderer::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL indirect buffer must bound at SRV or UAV descriptor range type")
						RENDERER_ASSERT(mContext, Renderer::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Renderer::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "OpenGL descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")

						// "GL_ARB_uniform_buffer_object" required
						if (mExtensions->isGL_ARB_uniform_buffer_object())
						{
							// "glBindBufferBase()" unit parameter is zero based so we can simply use the value we received
							const GLuint index = descriptorRange.baseShaderRegister;

							// Attach the buffer to the given SSBO binding point
							glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, static_cast<IndirectBuffer*>(resource)->getOpenGLIndirectBuffer());
						}
						break;
					}

					case Renderer::ResourceType::UNIFORM_BUFFER:
						// "GL_ARB_uniform_buffer_object" required
						if (mExtensions->isGL_ARB_uniform_buffer_object())
						{
							// Attach the buffer to the given UBO binding point
							// -> Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension
							// -> Direct3D 10 and Direct3D 11 have explicit binding points
							RENDERER_ASSERT(mContext, Renderer::DescriptorRangeType::UBV == descriptorRange.rangeType, "OpenGL uniform buffer must bound at UBV descriptor range type")
							RENDERER_ASSERT(mContext, nullptr != openGLResourceGroup->getResourceIndexToUniformBlockBindingIndex(), "Invalid OpenGL resource index to uniform block binding index")
							glBindBufferBase(GL_UNIFORM_BUFFER, openGLResourceGroup->getResourceIndexToUniformBlockBindingIndex()[resourceIndex], static_cast<UniformBuffer*>(resource)->getOpenGLUniformBuffer());
						}
						break;

					case Renderer::ResourceType::SAMPLER_STATE:
						// Unlike Direct3D >=10, OpenGL directly attaches the sampler settings to the texture (unless the sampler object extension is used)
						break;

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
					case Renderer::ResourceType::QUERY_POOL:
					case Renderer::ResourceType::SWAP_CHAIN:
					case Renderer::ResourceType::FRAMEBUFFER:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
						RENDERER_LOG(mContext, CRITICAL, "Invalid OpenGL renderer backend resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void OpenGLRenderer::setOpenGLGraphicsProgram(Renderer::IGraphicsProgram* graphicsProgram)
	{
		if (nullptr != graphicsProgram)
		{
			// Sanity check
			OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsProgram)

			// Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
			if (mExtensions->isGL_ARB_separate_shader_objects())
			{
				// Bind the graphics program pipeline, if required
				GraphicsProgramSeparate* graphicsProgramSeparate = static_cast<GraphicsProgramSeparate*>(graphicsProgram);
				const uint32_t openGLProgramPipeline = graphicsProgramSeparate->getOpenGLProgramPipeline();
				if (openGLProgramPipeline != mOpenGLProgramPipeline)
				{
					mOpenGLProgramPipeline = openGLProgramPipeline;
					{ // Draw ID uniform location for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
						const VertexShaderSeparate* vertexShaderSeparate = graphicsProgramSeparate->getVertexShaderSeparate();
						if (nullptr != vertexShaderSeparate)
						{
							mOpenGLVertexProgram = vertexShaderSeparate->getOpenGLShaderProgram();
							mDrawIdUniformLocation = vertexShaderSeparate->getDrawIdUniformLocation();
						}
						else
						{
							mOpenGLVertexProgram = 0;
							mDrawIdUniformLocation = -1;
						}
						mCurrentStartInstanceLocation = ~0u;
					}
					glBindProgramPipeline(mOpenGLProgramPipeline);
				}
			}
			else if (mExtensions->isGL_ARB_shader_objects())
			{
				// Bind the graphics program, if required
				const GraphicsProgramMonolithic* graphicsProgramMonolithic = static_cast<GraphicsProgramMonolithic*>(graphicsProgram);
				const uint32_t openGLProgram = graphicsProgramMonolithic->getOpenGLProgram();
				if (openGLProgram != mOpenGLProgram)
				{
					mOpenGLProgram = mOpenGLVertexProgram = openGLProgram;
					mDrawIdUniformLocation = graphicsProgramMonolithic->getDrawIdUniformLocation();
					mCurrentStartInstanceLocation = ~0u;
					glUseProgram(mOpenGLProgram);
				}
			}
		}
		else
		{
			// Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
			if (mExtensions->isGL_ARB_separate_shader_objects())
			{
				// Unbind the program pipeline, if required
				if (0 != mOpenGLProgramPipeline)
				{
					glBindProgramPipeline(0);
					mOpenGLProgramPipeline = 0;
				}
			}
			else if (mExtensions->isGL_ARB_shader_objects())
			{
				// Unbind the program, if required
				if (0 != mOpenGLProgram)
				{
					glUseProgram(0);
					mOpenGLProgram = 0;
				}
			}
			mOpenGLVertexProgram = 0;
			mDrawIdUniformLocation = -1;
			mCurrentStartInstanceLocation = ~0u;
		}
	}

	void OpenGLRenderer::setOpenGLComputePipelineState(ComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			OPENGLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *computePipelineState)

			// Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
			if (mExtensions->isGL_ARB_separate_shader_objects())
			{
				// Bind the program pipeline, if required
				const uint32_t openGLProgramPipeline = static_cast<ComputePipelineStateSeparate*>(computePipelineState)->getOpenGLProgramPipeline();
				if (openGLProgramPipeline != mOpenGLProgramPipeline)
				{
					mOpenGLProgramPipeline = openGLProgramPipeline;
					glBindProgramPipeline(mOpenGLProgramPipeline);
					mOpenGLVertexProgram = 0;
					mDrawIdUniformLocation = -1;
					mCurrentStartInstanceLocation = ~0u;
				}
			}
			else if (mExtensions->isGL_ARB_shader_objects())
			{
				// Bind the program, if required
				const uint32_t openGLProgram = static_cast<ComputePipelineStateMonolithic*>(computePipelineState)->getOpenGLProgram();
				if (openGLProgram != mOpenGLProgram)
				{
					mOpenGLProgram = openGLProgram;
					glUseProgram(mOpenGLProgram);
					mOpenGLVertexProgram = 0;
					mDrawIdUniformLocation = -1;
					mCurrentStartInstanceLocation = ~0u;
				}
			}
		}
		else
		{
			// Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
			if (mExtensions->isGL_ARB_separate_shader_objects())
			{
				// Unbind the program pipeline, if required
				if (0 != mOpenGLProgramPipeline)
				{
					glBindProgramPipeline(0);
					mOpenGLProgramPipeline = 0;
				}
			}
			else if (mExtensions->isGL_ARB_shader_objects())
			{
				// Unbind the program, if required
				if (0 != mOpenGLProgram)
				{
					glUseProgram(0);
					mOpenGLProgram = 0;
				}
			}
			mOpenGLVertexProgram = 0;
			mDrawIdUniformLocation = -1;
			mCurrentStartInstanceLocation = ~0u;
		}
	}

	void OpenGLRenderer::updateGL_ARB_base_instanceEmulation(uint32_t startInstanceLocation)
	{
		if (mDrawIdUniformLocation != -1 && 0 != mOpenGLVertexProgram && mCurrentStartInstanceLocation != startInstanceLocation)
		{
			glProgramUniform1ui(mOpenGLVertexProgram, mDrawIdUniformLocation, startInstanceLocation);
			mCurrentStartInstanceLocation = startInstanceLocation;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // OpenGLRenderer




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RENDERER_OPENGL_EXPORTS
	#define OPENGLRENDERER_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define OPENGLRENDERER_FUNCTION_EXPORT
#endif
OPENGLRENDERER_FUNCTION_EXPORT Renderer::IRenderer* createOpenGLRendererInstance(const Renderer::Context& context)
{
	return RENDERER_NEW(context, OpenGLRenderer::OpenGLRenderer)(context);
}
#undef OPENGLRENDERER_FUNCTION_EXPORT
