/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
*    OpenGL RHI amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    - OpenGL capable graphics driver
*    - OpenGL headers which can be found at "<unrimp>\External\Rhi\OpenGL\include\"
*    - smol-v (directly compiled and linked in)
*    - glslang if "RHI_OPENGL_GLSLTOSPIRV" is set (directly compiled and linked in)
*
*    == Preprocessor Definitions ==
*    - Set "RHI_OPENGL_EXPORTS" as preprocessor definition when building this library as shared library
*    - If this RHI was compiled with "RHI_OPENGL_STATE_CLEANUP" set as preprocessor definition, the previous OpenGL state will be restored after performing an operation (worse performance, increases the binary size slightly, might avoid unexpected behaviour when using OpenGL directly beside this RHI)
*    - Set "RHI_OPENGL_GLSLTOSPIRV" as preprocessor definition when building this library to add support for compiling GLSL into SPIR-V, increases the binary size around one MiB
*    - Do also have a look into the RHI header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

#ifdef RHI_OPENGL_GLSLTOSPIRV
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator '<x>' in switch of enum '<y>' is not explicitly handled by a case label
		PRAGMA_WARNING_DISABLE_MSVC(4100)	// warning C4100: 's': unreferenced formal parameter
		PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from '<x>' to '<y>', signed/unsigned mismatch
		PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
		PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
		PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_List_node<_Ty,std::_Default_allocator_traits<_Alloc>::void_pointer>': default constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: '<x>': copy constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
		PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
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
	#include <Windows.h>

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
//[ OpenGLRhi/MakeID.h                                    ]
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


#ifdef RHI_DEBUG
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

	Rhi::IAllocator& m_Allocator;
	Range *m_Ranges; // Sorted array of ranges of free IDs
	uint m_Count;    // Number of ranges in list
	uint m_Capacity; // Total capacity of range list

	MakeID & operator=(const MakeID &) = delete;
	MakeID(const MakeID &) = delete;

public:
	MakeID(Rhi::IAllocator& allocator, const uint max_id = std::numeric_limits<uint>::max()) :
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

	#ifdef RHI_DEBUG
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
namespace OpenGLRhi
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
#ifdef RHI_DEBUG
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given RHI
	*/
	#define RHI_MATCH_CHECK(rhiReference, resourceReference) \
		RHI_ASSERT(mContext, &rhiReference == &(resourceReference).getRhi(), "OpenGL error: The given resource is owned by another RHI instance")

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER , [[maybe_unused]] const char debugName[] = ""
#else
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given RHI
	*/
	#define RHI_MATCH_CHECK(rhiReference, resourceReference)

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER
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
	#define IMPORT_FUNC(funcName)																																			\
		if (result)																																							\
		{																																									\
			void* symbol = ::GetProcAddress(static_cast<HMODULE>(mOpenGLSharedLibrary), #funcName);																			\
			if (nullptr == symbol)																																			\
			{																																								\
				symbol = wglGetProcAddress(#funcName);																														\
			}																																								\
			if (nullptr != symbol)																																			\
			{																																								\
				*(reinterpret_cast<void**>(&(funcName))) = symbol;																											\
			}																																								\
			else																																							\
			{																																								\
				wchar_t moduleFilename[MAX_PATH];																															\
				moduleFilename[0] = '\0';																																	\
				::GetModuleFileNameW(static_cast<HMODULE>(mOpenGLSharedLibrary), moduleFilename, MAX_PATH);																	\
				RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library \"%s\"", #funcName, moduleFilename)	\
				result = false;																																				\
			}																																								\
		}
#elif LINUX
	#define IMPORT_FUNC(funcName)																																		\
		if (result)																																						\
		{																																								\
			void* symbol = ::dlsym(mOpenGLSharedLibrary, #funcName);																									\
			if (nullptr != symbol)																																		\
			{																																							\
				*(reinterpret_cast<void**>(&(funcName))) = symbol;																										\
			}																																							\
			else																																						\
			{																																							\
				link_map *linkMap = nullptr;																															\
				const char* libraryName = "unknown";																													\
				if (dlinfo(mOpenGLSharedLibrary, RTLD_DI_LINKMAP, &linkMap))																							\
				{																																						\
					libraryName = linkMap->l_name;																														\
				}																																						\
				RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library \"%s\"", #funcName, libraryName)	\
				result = false;																																			\
			}																																							\
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
//[ NV                                                    ]
//[-------------------------------------------------------]
// GL_NV_mesh_shader
FNDEF_EX(glDrawMeshTasksNV,	PFNGLDRAWMESHTASKSNVPROC);

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

		#ifdef RHI_OPENGL_GLSLTOSPIRV
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
				256,	///< MaxMeshOutputVerticesNV
				512,	///< MaxMeshOutputPrimitivesNV
				32,		///< MaxMeshWorkGroupSizeX_NV
				1,		///< MaxMeshWorkGroupSizeY_NV
				1,		///< MaxMeshWorkGroupSizeZ_NV
				32,		///< MaxTaskWorkGroupSizeX_NV
				1,		///< MaxTaskWorkGroupSizeY_NV
				1,		///< MaxTaskWorkGroupSizeZ_NV
				4,		///< MaxMeshViewCountNV
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
			Rhi::ITexture::getMipmapSize(mipmapIndex, textureWidth, textureHeight);
			if (width > textureWidth)
			{
				width = textureWidth;
			}
			if (height > textureHeight)
			{
				height = textureHeight;
			}
		}

		void printOpenGLShaderInformationIntoLog(const Rhi::Context& context, GLuint openGLShader)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetShaderiv(openGLShader, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetShaderInfoLog(openGLShader, informationLength, nullptr, informationLog);

				// Output the debug string
				RHI_LOG(context, CRITICAL, informationLog)

				// Cleanup information memory
				RHI_FREE(context, informationLog);
			}
		}

		void printOpenGLShaderInformationIntoLog(const Rhi::Context& context, GLuint openGLShader, const char* sourceCode)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetShaderiv(openGLShader, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetShaderInfoLog(openGLShader, informationLength, nullptr, informationLog);

				// Output the debug string
				if (context.getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
				{
					DEBUG_BREAK;
				}

				// Cleanup information memory
				RHI_FREE(context, informationLog);
			}
		}

		void printOpenGLProgramInformationIntoLog(const Rhi::Context& context, GLuint openGLProgram)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetProgramiv(openGLProgram, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetProgramInfoLog(openGLProgram, informationLength, nullptr, informationLog);

				// Output the debug string
				RHI_LOG(context, CRITICAL, informationLog)

				// Cleanup information memory
				RHI_FREE(context, informationLog);
			}
		}

		void printOpenGLProgramInformationIntoLog(const Rhi::Context& context, GLuint openGLProgram, const char* sourceCode)
		{
			// Get the length of the information (including a null termination)
			GLint informationLength = 0;
			glGetProgramiv(openGLProgram, GL_INFO_LOG_LENGTH, &informationLength);
			if (informationLength > 1)
			{
				// Allocate memory for the information
				char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

				// Get the information
				glGetProgramInfoLog(openGLProgram, informationLength, nullptr, informationLog);

				// Output the debug string
				if (context.getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
				{
					DEBUG_BREAK;
				}

				// Cleanup information memory
				RHI_FREE(context, informationLog);
			}
		}

		/**
		*  @brief
		*    Create and load a shader from bytecode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] shaderBytecode
		*    Shader SPIR-V bytecode ("GL_ARB_gl_spirv"-extension) compressed via SMOL-V
		*
		*  @return
		*    The OpenGL shader, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderFromBytecode(const Rhi::Context& context, GLenum shaderType, const Rhi::ShaderBytecode& shaderBytecode)
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
				uint8_t* spirvOutputBuffer = RHI_MALLOC_TYPED(context, uint8_t, spirvOutputBufferSize);
				smolv::Decode(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), spirvOutputBuffer, spirvOutputBufferSize);
				glShaderBinary(1, &openGLShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, spirvOutputBuffer, static_cast<GLsizei>(spirvOutputBufferSize));
				RHI_FREE(context, spirvOutputBuffer);
			}

			// Done
			return openGLShader;
		}

		/**
		*  @brief
		*    Create and load a shader program from bytecode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] shaderBytecode
		*    Shader SPIR-V bytecode ("GL_ARB_gl_spirv"-extension) compressed via SMOL-V
		*
		*  @return
		*    The OpenGL shader program, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderProgramFromBytecode(const Rhi::Context& context, GLenum shaderType, const Rhi::ShaderBytecode& shaderBytecode)
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
		*    RHI context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*
		*  @return
		*    The OpenGL shader program, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderProgramFromSourceCode(const Rhi::Context& context, GLenum shaderType, const GLchar* sourceCode)
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
		[[nodiscard]] GLuint createShaderProgramObject(const Rhi::Context& context, GLuint openGLShader, const Rhi::VertexAttributes& vertexAttributes)
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

		[[nodiscard]] GLuint loadShaderProgramFromBytecode(const Rhi::Context& context, const Rhi::VertexAttributes& vertexAttributes, GLenum shaderType, const Rhi::ShaderBytecode& shaderBytecode)
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
		*    RHI context to use
		*  @param[in] shaderType
		*    Shader type (for example "GL_VERTEX_SHADER_ARB")
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*
		*  @return
		*    The OpenGL shader, 0 on error, destroy the resource if you no longer need it
		*/
		[[nodiscard]] GLuint loadShaderFromSourcecode(const Rhi::Context& context, GLenum shaderType, const GLchar* sourceCode)
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
						GLchar* informationLog = RHI_MALLOC_TYPED(context, GLchar, informationLength);

						// Get the information
						glGetShaderInfoLog(openGLShader, informationLength, nullptr, informationLog);

						// Output the debug string
						if (context.getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
						{
							DEBUG_BREAK;
						}

						// Cleanup information memory
						RHI_FREE(context, informationLog);
					}
				}

				// Destroy the shader
				// -> A value of 0 for shader will be silently ignored
				glDeleteShader(openGLShader);

				// Error!
				return 0u;
			}
		}

		[[nodiscard]] GLuint loadShaderProgramFromSourcecode(const Rhi::Context& context, const Rhi::VertexAttributes& vertexAttributes, GLenum type, const char* sourceCode)
		{
			return createShaderProgramObject(context, loadShaderFromSourcecode(context, type, sourceCode), vertexAttributes);
		}

		/**
		*  @brief
		*    Compile shader source code to shader bytecode
		*
		*  @param[in] context
		*    RHI context to use
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
		void shaderSourceCodeToShaderBytecode(const Rhi::Context& context, GLenum shaderType, const GLchar* sourceCode, Rhi::ShaderBytecode& shaderBytecode)
		{
			#ifdef RHI_OPENGL_GLSLTOSPIRV
				// Initialize glslang, if necessary
				if (!GlslangInitialized)
				{
					glslang::InitializeProcess();
					GlslangInitialized = true;
				}

				// GLSL to intermediate
				// -> OpenGL 4.1 (the best OpenGL version Mac OS X 10.11 supports, so lowest version we have to support)
				// TODO(co) OpenGL GLSL 430 instead of 410 for e.g. "GL_ARB_shader_image_load_store" build in support. Apply dropped OpenGL support so we can probably drop Apple support. If at once point Unrimp should run on Apple hardware, we probably will use MoltenVK for Vulkan (yet another RHI API Metal just for Apple hardware is probably to much work for a spare time project).
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
						if (context.getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to link the GLSL program: %s", program.getInfoLog()))
						{
							DEBUG_BREAK;
						}
					}
				}
				else
				{
					// Failed to parse the shader source code
					if (context.getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to parse the GLSL shader source code: %s", shader.getInfoLog()))
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

		void bindUniformBlock(const Rhi::DescriptorRange& descriptorRange, uint32_t openGLProgram, uint32_t uniformBlockBindingIndex)
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

		void bindUniformLocation(const Rhi::DescriptorRange& descriptorRange, uint32_t openGLProgramPipeline, uint32_t openGLProgram)
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
					#ifdef RHI_OPENGL_STATE_CLEANUP
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
namespace OpenGLRhi
{




	//[-------------------------------------------------------]
	//[ OpenGLRhi/OpenGLRhi.h                                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL RHI class
	*/
	class OpenGLRhi final : public Rhi::IRhi
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
		*    RHI context, the RHI context instance must stay valid as long as the RHI instance exists
		*
		*  @note
		*    - Do never ever use a not properly initialized RHI. Use "Rhi::IRhi::isInitialized()" to check the initialization state.
		*/
		explicit OpenGLRhi(const Rhi::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~OpenGLRhi() override;

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
		void setGraphicsRootSignature(Rhi::IRootSignature* rootSignature);
		void setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState);
		void setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup);
		void setGraphicsVertexArray(Rhi::IVertexArray* vertexArray);															// Input-assembler (IA) stage
		void setGraphicsViewports(uint32_t numberOfViewports, const Rhi::Viewport* viewports);									// Rasterizer (RS) stage
		void setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles);	// Rasterizer (RS) stage
		void setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget);															// Output-merger (OM) stage
		void clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil);
		void drawGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawMeshTasks(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawMeshTasksEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		//[-------------------------------------------------------]
		//[ Compute                                               ]
		//[-------------------------------------------------------]
		void setComputeRootSignature(Rhi::IRootSignature* rootSignature);
		void setComputePipelineState(Rhi::IComputePipelineState* computePipelineState);
		void setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup);
		void dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		//[-------------------------------------------------------]
		//[ Resource                                              ]
		//[-------------------------------------------------------]
		void resolveMultisampleFramebuffer(Rhi::IRenderTarget& destinationRenderTarget, Rhi::IFramebuffer& sourceMultisampleFramebuffer);
		void copyResource(Rhi::IResource& destinationResource, Rhi::IResource& sourceResource);
		void generateMipmaps(Rhi::IResource& resource);
		//[-------------------------------------------------------]
		//[ Query                                                 ]
		//[-------------------------------------------------------]
		void resetQueryPool(Rhi::IQueryPool& queryPool, uint32_t firstQueryIndex, uint32_t numberOfQueries);
		void beginQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex, uint32_t queryControlFlags);
		void endQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex);
		void writeTimestampQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex);
		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RHI_DEBUG
			void setDebugMarker(const char* name);
			void beginDebugEvent(const char* name);
			void endDebugEvent();
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
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
		[[nodiscard]] virtual Rhi::IShaderLanguage* getShaderLanguage(const char* shaderLanguageName = nullptr) override;
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual Rhi::IRenderPass* createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat = Rhi::TextureFormat::UNKNOWN, uint8_t numberOfMultisamples = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IQueryPool* createQueryPool(Rhi::QueryType queryType, uint32_t numberOfQueries = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::ISwapChain* createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool useExternalContext = false RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IFramebuffer* createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IBufferManager* createBufferManager() override;
		[[nodiscard]] virtual Rhi::ITextureManager* createTextureManager() override;
		[[nodiscard]] virtual Rhi::IRootSignature* createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IGraphicsPipelineState* createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IComputePipelineState* createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::ISamplerState* createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		//[-------------------------------------------------------]
		//[ Resource handling                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual bool map(Rhi::IResource& resource, uint32_t subresource, Rhi::MapType mapType, uint32_t mapFlags, Rhi::MappedSubresource& mappedSubresource) override;
		virtual void unmap(Rhi::IResource& resource, uint32_t subresource) override;
		[[nodiscard]] virtual bool getQueryPoolResults(Rhi::IQueryPool& queryPool, uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex = 0, uint32_t numberOfQueries = 1, uint32_t strideInBytes = 0, uint32_t queryResultFlags = 0) override;
		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual bool beginScene() override;
		virtual void submitCommandBuffer(const Rhi::CommandBuffer& commandBuffer) override;
		virtual void endScene() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(mContext, OpenGLRhi, this);
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
		explicit OpenGLRhi(const OpenGLRhi& source) = delete;
		OpenGLRhi& operator =(const OpenGLRhi& source) = delete;

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
		void setResourceGroup(const RootSignature& rootSignature, uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup);

		/**
		*  @brief
		*    Set OpenGL graphics program
		*
		*  @param[in] graphicsProgram
		*    Graphics program to set
		*/
		void setOpenGLGraphicsProgram(Rhi::IGraphicsProgram* graphicsProgram);

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
		OpenGLRuntimeLinking* mOpenGLRuntimeLinking;			///< OpenGL runtime linking instance, always valid
		IOpenGLContext*		  mOpenGLContext;					///< OpenGL context instance, always valid
		Extensions*			  mExtensions;						///< Extensions instance, always valid
		Rhi::IShaderLanguage* mShaderLanguage;					///< Shader language instance (we keep a reference to it), can be a null pointer
		RootSignature*		  mGraphicsRootSignature;			///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*		  mComputeRootSignature;			///< Currently set compute root signature (we keep a reference to it), can be a null pointer
		Rhi::ISamplerState*   mDefaultSamplerState;				///< Default rasterizer state (we keep a reference to it), can be a null pointer
		GLuint				  mOpenGLCopyResourceFramebuffer;	///< OpenGL framebuffer ("container" object, not shared between OpenGL contexts) used by "OpenGLRhi::OpenGLRhi::copyResource()" if the "GL_ARB_copy_image"-extension isn't available, can be zero if no resource is allocated
		GLuint				  mDefaultOpenGLVertexArray;		///< Default OpenGL vertex array ("container" object, not shared between OpenGL contexts) to enable attribute-less rendering, can be zero if no resource is allocated
		// States
		GraphicsPipelineState* mGraphicsPipelineState;	///< Currently set graphics pipeline state (we keep a reference to it), can be a null pointer
		ComputePipelineState*  mComputePipelineState;	///< Currently set compute pipeline state (we keep a reference to it), can be a null pointer
		// Input-assembler (IA) stage
		VertexArray* mVertexArray;				///< Currently set vertex array (we keep a reference to it), can be a null pointer
		GLenum		 mOpenGLPrimitiveTopology;	///< OpenGL primitive topology describing the type of primitive to render
		GLint		 mNumberOfVerticesPerPatch;	///< Number of vertices per patch
		// Output-merger (OM) stage
		Rhi::IRenderTarget* mRenderTarget;	///< Currently set render target (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant OpenGL calls
		GLenum mOpenGLClipControlOrigin;	///< Currently set OpenGL clip control origin
		GLuint mOpenGLProgramPipeline;		///< Currently set OpenGL program pipeline, can be zero if no resource is set
		GLuint mOpenGLProgram;				///< Currently set OpenGL program, can be zero if no resource is set
		GLuint mOpenGLIndirectBuffer;		///< Currently set OpenGL indirect buffer, can be zero if no resource is set
		// Draw ID uniform location for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
		GLuint	 mOpenGLVertexProgram;			///< Currently set OpenGL vertex program, can be zero if no resource is set
		GLint	 mDrawIdUniformLocation;		///< Draw ID uniform location
		uint32_t mCurrentStartInstanceLocation;	///< Currently set start instance location
		#ifdef RHI_DEBUG
			bool mDebugBetweenBeginEndScene;	///< Just here for state tracking in debug builds
		#endif


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/OpenGLRuntimeLinking.h                      ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit OpenGLRuntimeLinking(OpenGLRhi& openGLRhi) :
			mOpenGLRhi(openGLRhi),
			mOpenGLSharedLibrary(mOpenGLRhi.getContext().getRhiApiSharedLibrary()),
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
						RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to load in the shared OpenGL library \"opengl32.dll\"")
					}
				#elif LINUX
					mOpenGLSharedLibrary = ::dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
					if (nullptr == mOpenGLSharedLibrary)
					{
						RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to load in the shared OpenGL library \"libGL.so\"")
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
			IMPORT_FUNC(glGetString)
			IMPORT_FUNC(glGetIntegerv)
			IMPORT_FUNC(glBindTexture)
			IMPORT_FUNC(glClear)
			IMPORT_FUNC(glClearStencil)
			IMPORT_FUNC(glClearDepth)
			IMPORT_FUNC(glClearColor)
			IMPORT_FUNC(glDrawArrays)
			IMPORT_FUNC(glDrawElements)
			IMPORT_FUNC(glColor4f)
			IMPORT_FUNC(glEnable)
			IMPORT_FUNC(glDisable)
			IMPORT_FUNC(glBlendFunc)
			IMPORT_FUNC(glFrontFace)
			IMPORT_FUNC(glCullFace)
			IMPORT_FUNC(glPolygonMode)
			IMPORT_FUNC(glTexParameteri)
			IMPORT_FUNC(glGenTextures)
			IMPORT_FUNC(glDeleteTextures)
			IMPORT_FUNC(glTexImage1D)
			IMPORT_FUNC(glTexImage2D)
			IMPORT_FUNC(glPixelStorei)
			IMPORT_FUNC(glDepthFunc)
			IMPORT_FUNC(glDepthMask)
			IMPORT_FUNC(glViewport)
			IMPORT_FUNC(glDepthRange)
			IMPORT_FUNC(glScissor)
			IMPORT_FUNC(glFlush)
			IMPORT_FUNC(glFinish)
			#ifdef _WIN32
				IMPORT_FUNC(wglGetCurrentDC)
				IMPORT_FUNC(wglGetProcAddress)
				IMPORT_FUNC(wglCreateContext)
				IMPORT_FUNC(wglDeleteContext)
				IMPORT_FUNC(wglMakeCurrent)
			#elif LINUX
				IMPORT_FUNC(glXMakeCurrent)
				IMPORT_FUNC(glXGetProcAddress)
				IMPORT_FUNC(glXGetProcAddressARB)
				IMPORT_FUNC(glXChooseVisual)
				IMPORT_FUNC(glXCreateContext)
				IMPORT_FUNC(glXDestroyContext)
				IMPORT_FUNC(glXGetCurrentContext)
				IMPORT_FUNC(glXQueryExtensionsString)
				IMPORT_FUNC(glXChooseFBConfig)
				IMPORT_FUNC(glXSwapBuffers)
				IMPORT_FUNC(glXGetClientString)
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
			IMPORT_FUNC(glCreateQueries)

			// Mandatory >= OpenGL 3.0: Load the entry points
			result = true;	// Success by default
			IMPORT_FUNC(glGetStringi)

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLRhi&	mOpenGLRhi;					///< Owner OpenGL RHI instance
		void*		mOpenGLSharedLibrary;		///< OpenGL shared library, can be a null pointer
		bool		mOwnsOpenGLSharedLibrary;	///< Indicates if the OpenGL shared library was loaded from ourself or provided from external
		bool		mEntryPointsRegistered;		///< Entry points successfully registered?
		bool		mInitialized;				///< Already initialized?


	};

	// Undefine the helper macro
	#undef IMPORT_FUNC




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Extensions.h                                ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] openGLContext
		*    Owner OpenGL context
		*/
		inline Extensions(OpenGLRhi& openGLRhi, IOpenGLContext& openGLContext) :
			mOpenGLRhi(openGLRhi),
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

		// NV
		[[nodiscard]] inline bool isGL_NV_mesh_shader() const
		{
			return mGL_NV_mesh_shader;
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
			mGL_NV_mesh_shader					 = false;
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
				#define IMPORT_FUNC(funcName)																													\
					if (result)																																	\
					{																																			\
						void* symbol = wglGetProcAddress(#funcName);																							\
						if (nullptr != symbol)																													\
						{																																		\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																					\
						}																																		\
						else																																	\
						{																																		\
							RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library", #funcName)	\
							result = false;																														\
						}																																		\
					}
			#elif APPLE
				// For OpenGL extension handling, Apple provides several documents like
				// - "Technical Note TN2080 Understanding and Detecting OpenGL Functionality" (http://developer.apple.com/library/mac/#technotes/tn2080/_index.html)
				// - "Cross-Development Programming Guide" (http://www.filibeto.org/unix/macos/lib/dev/documentation/DeveloperTools/Conceptual/cross_development/cross_development.pdf)
				// -> All referencing to "QA1188: GetProcAdress and OpenGL Entry Points" (http://developer.apple.com/qa/qa2001/qa1188.html).
				//    Sadly, it appears that this site no longer exists.
				// -> It appears that for Mac OS X v10.6 >, the "dlopen"-way is recommended.
				#define IMPORT_FUNC(funcName)																													\
					if (result)																																	\
					{																																			\
						void* symbol = m_pOpenGLSharedLibrary ? dlsym(mOpenGLSharedLibrary, #funcName) : nullptr;												\
						if (nullptr != symbol)																													\
						{																																		\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																					\
						}																																		\
						else																																	\
						{																																		\
							RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library", #funcName)	\
							result = false;																														\
						}																																		\
					}
			#elif LINUX
				typedef void (*GLfunction)();
				#define IMPORT_FUNC(funcName)																													\
					if (result)																																	\
					{																																			\
						GLfunction symbol = glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(#funcName));													\
						if (nullptr != symbol)																													\
						{																																		\
							*(reinterpret_cast<GLfunction*>(&(funcName))) = symbol;																				\
						}																																		\
						else																																	\
						{																																		\
							RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL shared library", #funcName)	\
							result = false;																														\
						}																																		\
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
			//[ NV                                                    ]
			//[-------------------------------------------------------]
			// GL_NV_mesh_shader
			mGL_NV_mesh_shader = isSupported("GL_NV_mesh_shader");
			if (mGL_NV_mesh_shader)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawMeshTasksNV)
				mGL_NV_mesh_shader = result;
			}


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
				IMPORT_FUNC(glNamedRenderbufferStorageEXT)
				IMPORT_FUNC(glNamedFramebufferRenderbufferEXT)
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
		OpenGLRhi&		mOpenGLRhi;		///< Owner OpenGL RHI instance
		IOpenGLContext* mOpenGLContext;	///< Owner OpenGL context, always valid!
		bool			mInitialized;	///< Are the extensions initialized?

		// Supported extensions
		// WGL (Windows only)
		bool mWGL_ARB_extensions_string;
		bool mWGL_EXT_swap_control;
		bool mWGL_EXT_swap_control_tear;
		// EXT
		bool mGL_NV_mesh_shader;
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
	//[ OpenGLRhi/IOpenGLContext.h                            ]
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
		//[ OpenGLRhi/OpenGLContextWindows.h                      ]
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
			friend class OpenGLRhi;


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
			inline OpenGLContextWindows(Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, Rhi::handle nativeWindowHandle, const OpenGLContextWindows* shareContextWindows = nullptr) :
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
		//[ Public virtual OpenGLRhi::IOpenGLContext methods      ]
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
			OpenGLContextWindows(OpenGLRuntimeLinking* openGLRuntimeLinking, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, Rhi::handle nativeWindowHandle, const OpenGLContextWindows* shareContextWindows = nullptr) :
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
					mNativeWindowHandle = mDummyWindow = reinterpret_cast<Rhi::handle>(::CreateWindow(TEXT("OpenGLDummyWindow"), TEXT("PFormat"), WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, nullptr, ::GetModuleHandle(nullptr), nullptr));
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
						const BYTE depthBufferBits = (Rhi::TextureFormat::Enum::UNKNOWN == depthStencilAttachmentTextureFormat) ? 0u : 24u;
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
								#ifdef RHI_DEBUG
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
			Rhi::handle mNativeWindowHandle;	///< OpenGL window, can be a null pointer (HWND)
			Rhi::handle mDummyWindow;			///< OpenGL dummy window, can be a null pointer (HWND)
			HDC			mWindowDeviceContext;	///< The device context of the OpenGL dummy window, can be a null pointer
			HGLRC		mWindowRenderContext;	///< The render context of the OpenGL dummy window, can be a null pointer
			bool		mOwnsRenderContext;		///< Does this context owns the OpenGL render context?


		};
	#elif LINUX
		//[-------------------------------------------------------]
		//[ OpenGLRhi/OpenGLContextLinux.h                        ]
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
			friend class OpenGLRhi;


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] openGLRhi
			*    Owner OpenGL RHI instance
			*  @param[in] depthStencilAttachmentTextureFormat
			*    Depth stencil attachment texture format
			*  @param[in] nativeWindowHandle
			*    Optional native main window handle, can be a null handle
			*  @param[in] useExternalContext
			*    When true an own OpenGL context won't be created and the context pointed by "shareContextLinux" is ignored
			*  @param[in] shareContextLinux
			*    Optional share context, can be a null pointer
			*/
			inline OpenGLContextLinux(OpenGLRhi& openGLRhi, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, Rhi::handle nativeWindowHandle, bool useExternalContext, const OpenGLContextLinux* shareContextLinux = nullptr) :
				OpenGLContextLinux(openGLRhi, nullptr, depthStencilAttachmentTextureFormat, nativeWindowHandle, useExternalContext, shareContextLinux)
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
		//[ Public virtual OpenGLRhi::IOpenGLContext methods      ]
		//[-------------------------------------------------------]
		public:
			[[nodiscard]] inline virtual bool isInitialized() const override
			{
				return (nullptr != mWindowRenderContext || mUseExternalContext);
			}

			virtual void makeCurrent() const override
			{
				// Only do something when have created our RHI context and don't use a external RHI context
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
			*  @param[in] openGLRhi
			*    Owner OpenGL RHI instance
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
			OpenGLContextLinux(OpenGLRhi& openGLRhi, OpenGLRuntimeLinking* openGLRuntimeLinking, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, Rhi::handle nativeWindowHandle, bool useExternalContext, const OpenGLContextLinux* shareContextLinux = nullptr) :
				IOpenGLContext(openGLRuntimeLinking),
				mOpenGLRhi(openGLRhi),
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
					const Rhi::Context& context = openGLRhi.getContext();
					RHI_ASSERT(context, context.getType() == Rhi::Context::ContextType::X11, "Invalid OpenGL context type")

					// If the given RHI context is an X11 context use the display connection object provided by the context
					if (context.getType() == Rhi::Context::ContextType::X11)
					{
						mDisplay = static_cast<const Rhi::X11Context&>(context).getDisplay();
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
								RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "Make new OpenGL context current: %d", result)
								{
									int major = 0;
									glGetIntegerv(GL_MAJOR_VERSION, &major);

									int minor = 0;
									glGetIntegerv(GL_MINOR_VERSION, &minor);

									GLint profile = 0;
									glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

									RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "OpenGL context version: %d.%d %s", major, minor, ((profile & GL_CONTEXT_CORE_PROFILE_BIT) ? "core" : "noncore"))
									int numberOfExtensions = 0;
									glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExtensions);
									RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "Number of supported OpenGL extensions: %d", numberOfExtensions)
									for (GLuint extensionIndex = 0; extensionIndex < static_cast<GLuint>(numberOfExtensions); ++extensionIndex)
									{
										RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "%s", glGetStringi(GL_EXTENSIONS, extensionIndex))
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
			[[nodiscard]] GLXContext createOpenGLContext(Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat)
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
							#ifdef RHI_DEBUG
							//	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
							//	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,	// Error messages like "Implicit version number 110 not supported by GL3 forward compatible context" might occur
							#else
							//	GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,	// Error messages like "Implicit version number 110 not supported by GL3 forward compatible context" might occur
							#endif
							// Done
							0
						};

						// TODO(co) Use more detailed color and depth/stencil information from render pass
						const int depthBufferBits = 24;//(Rhi::TextureFormat::Enum::UNKNOWN == depthStencilAttachmentTextureFormat) ? 24 : 24;
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
						RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "Got %d of OpenGL GLXFBConfig", numberOfElements)
						GLXContext glxContext = glXCreateContextAttribsARB(mDisplay, *fbc, 0, true, ATTRIBUTES);

						XSync(mDisplay, False);

						// TODO(sw) make this fallback optional (via an option)
						if (ctxErrorOccurred)
						{
							RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "Could not create OpenGL 3+ context try creating pre 3+ context")
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
							RHI_LOG(mOpenGLRhi.getContext(), DEBUG, "OpenGL context with glXCreateContextAttribsARB created")
							return glxContext;
						}
						else
						{
							// Error, context creation failed!
							RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Could not create OpenGL context with glXCreateContextAttribsARB")
							return NULL_HANDLE;
						}
					}
					else
					{
						// Error, failed to obtain the "GLX_ARB_create_context" function pointer (wow, something went terrible wrong!)
						RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "Could not find OpenGL glXCreateContextAttribsARB")
						return NULL_HANDLE;
					}
				}
				else
				{
					// Error, the OpenGL extension "GLX_ARB_create_context" is not supported... as a result we can't create an OpenGL context!
					RHI_LOG(mOpenGLRhi.getContext(), CRITICAL, "OpenGL GLX_ARB_create_context not supported")
					return NULL_HANDLE;
				}

				#undef GLX_CONTEXT_MAJOR_VERSION_ARB
				#undef GLX_CONTEXT_MINOR_VERSION_ARB
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			OpenGLRhi&	mOpenGLRhi;				///< Owner OpenGL RHI instance
			Rhi::handle mNativeWindowHandle;	///< OpenGL window, can be a null pointer (Window)
			Display*	mDisplay;				///< The X11 display connection, can be a null pointer
			bool		mOwnsX11Display;		///< Indicates if this instance owns the X11 display
			GLXContext	mWindowRenderContext;	///< The render context of the OpenGL dummy window, can be a null pointer
			bool		mUseExternalContext;
			bool		mOwnsRenderContext;		///< Does this context own the OpenGL render context?


		};
	#else
		#error "Unsupported platform"
	#endif




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Mapping.h                                   ]
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
		//[ Rhi::FilterMode                                       ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::FilterMode" to OpenGL magnification filter mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    OpenGL magnification filter mode
		*/
		[[nodiscard]] static GLint getOpenGLMagFilterMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Rhi::FilterMode::MIN_MAG_MIP_POINT:
					return GL_NEAREST;

				case Rhi::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Rhi::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Rhi::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return GL_NEAREST;

				case Rhi::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Rhi::FilterMode::MIN_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Rhi::FilterMode::ANISOTROPIC:
					return GL_LINEAR;	// There's no special setting in OpenGL

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_ANISOTROPIC:
					return GL_LINEAR;	// There's no special setting in OpenGL

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "OpenGL filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Rhi::FilterMode" to OpenGL minification filter mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*  @param[in] hasMipmaps
		*    Are mipmaps available?
		*
		*  @return
		*    OpenGL minification filter mode
		*/
		[[nodiscard]] static GLint getOpenGLMinFilterMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode, bool hasMipmaps)
		{
			switch (filterMode)
			{
				case Rhi::FilterMode::MIN_MAG_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Rhi::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Rhi::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Rhi::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Rhi::FilterMode::MIN_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Rhi::FilterMode::ANISOTROPIC:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;	// There's no special setting in OpenGL

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return hasMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

				case Rhi::FilterMode::COMPARISON_ANISOTROPIC:
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;	// There's no special setting in OpenGL

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "OpenGL filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Rhi::FilterMode" to OpenGL compare mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    OpenGL compare mode
		*/
		[[nodiscard]] static GLint getOpenGLCompareMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Rhi::FilterMode::MIN_MAG_MIP_POINT:
				case Rhi::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
				case Rhi::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
				case Rhi::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
				case Rhi::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
				case Rhi::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
				case Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
				case Rhi::FilterMode::MIN_MAG_MIP_LINEAR:
				case Rhi::FilterMode::ANISOTROPIC:
					return GL_NONE;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
				case Rhi::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
				case Rhi::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
				case Rhi::FilterMode::COMPARISON_ANISOTROPIC:
					return GL_COMPARE_REF_TO_TEXTURE;

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "OpenGL filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		//[-------------------------------------------------------]
		//[ Rhi::TextureAddressMode                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::TextureAddressMode" to OpenGL texture address mode
		*
		*  @param[in] textureAddressMode
		*    "Rhi::TextureAddressMode" to map
		*
		*  @return
		*    OpenGL texture address mode
		*/
		[[nodiscard]] static GLint getOpenGLTextureAddressMode(Rhi::TextureAddressMode textureAddressMode)
		{
			static constexpr GLint MAPPING[] =
			{
				GL_REPEAT,			// Rhi::TextureAddressMode::WRAP
				GL_MIRRORED_REPEAT,	// Rhi::TextureAddressMode::MIRROR
				GL_CLAMP_TO_EDGE,	// Rhi::TextureAddressMode::CLAMP
				GL_CLAMP_TO_BORDER,	// Rhi::TextureAddressMode::BORDER
				GL_MIRRORED_REPEAT	// Rhi::TextureAddressMode::MIRROR_ONCE	// TODO(co) OpenGL equivalent? GL_ATI_texture_mirror_once ?
			};
			return MAPPING[static_cast<int>(textureAddressMode) - 1];	// Lookout! The "Rhi::TextureAddressMode"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::ComparisonFunc                                   ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::ComparisonFunc" to OpenGL comparison function
		*
		*  @param[in] comparisonFunc
		*    "Rhi::ComparisonFunc" to map
		*
		*  @return
		*    OpenGL comparison function
		*/
		[[nodiscard]] static GLenum getOpenGLComparisonFunc(Rhi::ComparisonFunc comparisonFunc)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_NEVER,		// Rhi::ComparisonFunc::NEVER
				GL_LESS,		// Rhi::ComparisonFunc::LESS
				GL_EQUAL,		// Rhi::ComparisonFunc::EQUAL
				GL_LEQUAL,		// Rhi::ComparisonFunc::LESS_EQUAL
				GL_GREATER,		// Rhi::ComparisonFunc::GREATER
				GL_NOTEQUAL,	// Rhi::ComparisonFunc::NOT_EQUAL
				GL_GEQUAL,		// Rhi::ComparisonFunc::GREATER_EQUAL
				GL_ALWAYS		// Rhi::ComparisonFunc::ALWAYS
			};
			return MAPPING[static_cast<int>(comparisonFunc) - 1];	// Lookout! The "Rhi::ComparisonFunc"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::VertexAttributeFormat                            ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::VertexAttributeFormat" to OpenGL size (number of elements)
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    OpenGL size
		*/
		[[nodiscard]] static GLint getOpenGLSize(Rhi::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLint MAPPING[] =
			{
				1,	// Rhi::VertexAttributeFormat::FLOAT_1
				2,	// Rhi::VertexAttributeFormat::FLOAT_2
				3,	// Rhi::VertexAttributeFormat::FLOAT_3
				4,	// Rhi::VertexAttributeFormat::FLOAT_4
				4,	// Rhi::VertexAttributeFormat::R8G8B8A8_UNORM
				4,	// Rhi::VertexAttributeFormat::R8G8B8A8_UINT
				2,	// Rhi::VertexAttributeFormat::SHORT_2
				4,	// Rhi::VertexAttributeFormat::SHORT_4
				1	// Rhi::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    "Rhi::VertexAttributeFormat" to OpenGL type
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Rhi::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_FLOAT,			// Rhi::VertexAttributeFormat::FLOAT_1
				GL_FLOAT,			// Rhi::VertexAttributeFormat::FLOAT_2
				GL_FLOAT,			// Rhi::VertexAttributeFormat::FLOAT_3
				GL_FLOAT,			// Rhi::VertexAttributeFormat::FLOAT_4
				GL_UNSIGNED_BYTE,	// Rhi::VertexAttributeFormat::R8G8B8A8_UNORM
				GL_UNSIGNED_BYTE,	// Rhi::VertexAttributeFormat::R8G8B8A8_UINT
				GL_SHORT,			// Rhi::VertexAttributeFormat::SHORT_2
				GL_SHORT,			// Rhi::VertexAttributeFormat::SHORT_4
				GL_UNSIGNED_INT		// Rhi::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    Return whether or not "Rhi::VertexAttributeFormat" is a normalized format
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to check
		*
		*  @return
		*    "GL_TRUE" if the format is normalized, else "GL_FALSE"
		*/
		[[nodiscard]] static GLboolean isOpenGLVertexAttributeFormatNormalized(Rhi::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLboolean MAPPING[] =
			{
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_1
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_2
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_3
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_4
				GL_TRUE,	// Rhi::VertexAttributeFormat::R8G8B8A8_UNORM
				GL_FALSE,	// Rhi::VertexAttributeFormat::R8G8B8A8_UINT
				GL_FALSE,	// Rhi::VertexAttributeFormat::SHORT_2
				GL_FALSE,	// Rhi::VertexAttributeFormat::SHORT_4
				GL_FALSE	// Rhi::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    Return whether or not "Rhi::VertexAttributeFormat" is an integer format
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to check
		*
		*  @return
		*    "GL_TRUE" if the format is integer, else "GL_FALSE"
		*/
		[[nodiscard]] static GLboolean isOpenGLVertexAttributeFormatInteger(Rhi::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr GLboolean MAPPING[] =
			{
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_1
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_2
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_3
				GL_FALSE,	// Rhi::VertexAttributeFormat::FLOAT_4
				GL_FALSE,	// Rhi::VertexAttributeFormat::R8G8B8A8_UNORM
				GL_TRUE,	// Rhi::VertexAttributeFormat::R8G8B8A8_UINT
				GL_TRUE,	// Rhi::VertexAttributeFormat::SHORT_2
				GL_TRUE,	// Rhi::VertexAttributeFormat::SHORT_4
				GL_TRUE		// Rhi::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		//[-------------------------------------------------------]
		//[ Rhi::IndexBufferFormat                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::IndexBufferFormat" to OpenGL type
		*
		*  @param[in] indexBufferFormat
		*    "Rhi::IndexBufferFormat" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Rhi::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_UNSIGNED_BYTE,	// Rhi::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API)
				GL_UNSIGNED_SHORT,	// Rhi::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				GL_UNSIGNED_INT		// Rhi::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Rhi::TextureFormat                                    ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::TextureFormat" to OpenGL internal format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    OpenGL internal format
		*/
		[[nodiscard]] static GLuint getOpenGLInternalFormat(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr GLuint MAPPING[] =
			{
				GL_R8,										// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_RGB8,									// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_RGBA8,									// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_SRGB8_ALPHA8,							// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_RGBA8,									// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_R11F_G11F_B10F_EXT,						// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "GL_EXT_packed_float" OpenGL extension
				GL_RGBA16F_ARB,								// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_RGBA32F_ARB,								// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha),
				GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,			// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,		// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,			// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,		// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,			// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,		// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_LUMINANCE_LATC1_EXT,			// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,	// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				0,											// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in OpenGL
				GL_R16,										// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_R32UI,									// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_R32F,									// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_DEPTH_COMPONENT32F,						// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_RG16_SNORM,								// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_RG16F,									// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0											// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Rhi::TextureFormat" to OpenGL format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    OpenGL format
		*/
		[[nodiscard]] static GLuint getOpenGLFormat(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr GLuint MAPPING[] =
			{
				GL_RED,										// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_RGB,										// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_RGBA,									// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_RGBA,									// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_BGRA,									// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_RGB,										// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "GL_EXT_packed_float" OpenGL extension
				GL_RGBA,									// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_RGBA,									// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,			// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,		// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,			// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,		// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,			// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,		// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_COMPRESSED_LUMINANCE_LATC1_EXT,			// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT,	// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				0,											// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in OpenGL
				GL_RED,										// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_RED_INTEGER,								// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_RED,										// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_DEPTH_COMPONENT,							// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_RG,										// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_RG,										// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0											// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Rhi::TextureFormat" to OpenGL type
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_UNSIGNED_BYTE,						// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_UNSIGNED_BYTE,						// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_UNSIGNED_BYTE,						// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_UNSIGNED_BYTE,						// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_UNSIGNED_BYTE,						// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_UNSIGNED_INT_10F_11F_11F_REV_EXT,	// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "GL_EXT_packed_float" OpenGL extension
				GL_HALF_FLOAT_ARB,						// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_FLOAT,								// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				0,										// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				0,										// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				0,										// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				0,										// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				0,										// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				0,										// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				0,										// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				0,										// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				0,										// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in OpenGL
				GL_UNSIGNED_SHORT,						// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_UNSIGNED_INT,						// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_FLOAT,								// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_FLOAT,								// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_BYTE,								// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_FLOAT,								// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0										// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		//[-------------------------------------------------------]
		//[ Rhi::PrimitiveTopology                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::PrimitiveTopology" to OpenGL type
		*
		*  @param[in] primitiveTopology
		*    "Rhi::PrimitiveTopology" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLType(Rhi::PrimitiveTopology primitiveTopology)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_POINTS,			// Rhi::PrimitiveTopology::POINT_LIST
				GL_LINES,			// Rhi::PrimitiveTopology::LINE_LIST
				GL_LINE_STRIP,		// Rhi::PrimitiveTopology::LINE_STRIP
				GL_TRIANGLES,		// Rhi::PrimitiveTopology::TRIANGLE_LIST
				GL_TRIANGLE_STRIP	// Rhi::PrimitiveTopology::TRIANGLE_STRIP
			};
			return MAPPING[static_cast<int>(primitiveTopology) - 1];	// Lookout! The "Rhi::PrimitiveTopology"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::MapType                                          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::MapType" to OpenGL type
		*
		*  @param[in] mapType
		*    "Rhi::MapType" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLMapType(Rhi::MapType mapType)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_READ_ONLY,	// Rhi::MapType::READ
				GL_WRITE_ONLY,	// Rhi::MapType::WRITE
				GL_READ_WRITE,	// Rhi::MapType::READ_WRITE
				GL_WRITE_ONLY,	// Rhi::MapType::WRITE_DISCARD
				GL_WRITE_ONLY	// Rhi::MapType::WRITE_NO_OVERWRITE
			};
			return MAPPING[static_cast<int>(mapType) - 1];	// Lookout! The "Rhi::MapType"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::MapType                                          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::Blend" to OpenGL type
		*
		*  @param[in] blend
		*    "Rhi::Blend" to map
		*
		*  @return
		*    OpenGL type
		*/
		[[nodiscard]] static GLenum getOpenGLBlendType(Rhi::Blend blend)
		{
			if (blend <= Rhi::Blend::SRC_ALPHA_SAT)
			{
				static constexpr GLenum MAPPING[] =
				{
					GL_ZERO,				// Rhi::Blend::ZERO
					GL_ONE,					// Rhi::Blend::ONE
					GL_SRC_COLOR,			// Rhi::Blend::SRC_COLOR
					GL_ONE_MINUS_SRC_COLOR,	// Rhi::Blend::INV_SRC_COLOR
					GL_SRC_ALPHA,			// Rhi::Blend::SRC_ALPHA
					GL_ONE_MINUS_SRC_ALPHA,	// Rhi::Blend::INV_SRC_ALPHA
					GL_DST_ALPHA,			// Rhi::Blend::DEST_ALPHA
					GL_ONE_MINUS_DST_ALPHA,	// Rhi::Blend::INV_DEST_ALPHA
					GL_DST_COLOR,			// Rhi::Blend::DEST_COLOR
					GL_ONE_MINUS_DST_COLOR,	// Rhi::Blend::INV_DEST_COLOR
					GL_SRC_ALPHA_SATURATE	// Rhi::Blend::SRC_ALPHA_SAT
				};
				return MAPPING[static_cast<int>(blend) - static_cast<int>(Rhi::Blend::ZERO)];
			}
			else
			{
				static constexpr GLenum MAPPING[] =
				{
					GL_SRC_COLOR,				// Rhi::Blend::BLEND_FACTOR		TODO(co) Mapping "Rhi::Blend::BLEND_FACTOR" to OpenGL possible?
					GL_ONE_MINUS_SRC_COLOR,		// Rhi::Blend::INV_BLEND_FACTOR	TODO(co) Mapping "Rhi::Blend::INV_BLEND_FACTOR" to OpenGL possible?
					GL_SRC1_COLOR,				// Rhi::Blend::SRC_1_COLOR
					GL_ONE_MINUS_SRC1_COLOR,	// Rhi::Blend::INV_SRC_1_COLOR
					GL_SRC1_ALPHA,				// Rhi::Blend::SRC_1_ALPHA
					GL_ONE_MINUS_SRC1_ALPHA,	// Rhi::Blend::INV_SRC_1_ALPHA
				};
				return MAPPING[static_cast<int>(blend) - static_cast<int>(Rhi::Blend::BLEND_FACTOR)];
			}
		}


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/ResourceGroup.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL resource group class
	*/
	class ResourceGroup final : public Rhi::IResourceGroup
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rhi
		*    Owner RHI instance
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
		ResourceGroup(Rhi::IRhi& rhi, const Rhi::RootSignature& rootSignature, uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IResourceGroup(rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootParameterIndex(rootParameterIndex),
			mNumberOfResources(numberOfResources),
			mResources(RHI_MALLOC_TYPED(rhi.getContext(), Rhi::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr),
			mResourceIndexToUniformBlockBindingIndex(nullptr)
		{
			// Get the uniform block binding start index
			const Rhi::Context& context = rhi.getContext();
			uint32_t uniformBlockBindingIndex = 0;
			for (uint32_t currentRootParameterIndex = 0; currentRootParameterIndex < rootParameterIndex; ++currentRootParameterIndex)
			{
				const Rhi::RootParameter& rootParameter = rootSignature.parameters[currentRootParameterIndex];
				if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
				{
					RHI_ASSERT(rhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
					const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
					for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
					{
						if (Rhi::DescriptorRangeType::UBV == reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex].rangeType)
						{
							++uniformBlockBindingIndex;
						}
					}
				}
			}

			// Process all resources and add our reference to the RHI resource
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Rhi::IResource* resource = *resources;
				RHI_ASSERT(rhi.getContext(), nullptr != resource, "Invalid OpenGL resource")
				mResources[resourceIndex] = resource;
				resource->addReference();

				// Uniform block binding index handling
				const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];
				if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
				{
					if (nullptr == mResourceIndexToUniformBlockBindingIndex)
					{
						mResourceIndexToUniformBlockBindingIndex = RHI_MALLOC_TYPED(context, uint32_t, mNumberOfResources);
						memset(mResourceIndexToUniformBlockBindingIndex, 0, sizeof(uint32_t) * mNumberOfResources);
					}
					mResourceIndexToUniformBlockBindingIndex[resourceIndex] = uniformBlockBindingIndex;
					++uniformBlockBindingIndex;
				}
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RHI_MALLOC_TYPED(context, Rhi::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Rhi::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
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
			// Remove our reference from the RHI resources
			const Rhi::Context& context = getRhi().getContext();
			if (nullptr != mSamplerStates)
			{
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Rhi::ISamplerState* samplerState = mSamplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->releaseReference();
					}
				}
				RHI_FREE(context, mSamplerStates);
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
			{
				mResources[resourceIndex]->releaseReference();
			}
			RHI_FREE(context, mResources);
			RHI_FREE(context, mResourceIndexToUniformBlockBindingIndex);
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
		*    Return the RHI resources
		*
		*  @return
		*    The RHI resources, don't release or destroy the returned pointer
		*/
		[[nodiscard]] inline Rhi::IResource** getResources() const
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
		[[nodiscard]] inline Rhi::ISamplerState** getSamplerState() const
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ResourceGroup, this);
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
		uint32_t			 mRootParameterIndex;						///< The root parameter index number for binding
		uint32_t			 mNumberOfResources;						///< Number of resources this resource group groups together
		Rhi::IResource**	 mResources;								///< RHI resources, we keep a reference to it
		Rhi::ISamplerState** mSamplerStates;							///< Sampler states, we keep a reference to it
		uint32_t*			 mResourceIndexToUniformBlockBindingIndex;	///< Resource index to uniform block binding index mapping, only valid for uniform buffer resources


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/RootSignature.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL root signature ("pipeline layout" in Vulkan terminology) class
	*/
	class RootSignature final : public Rhi::IRootSignature
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(OpenGLRhi& openGLRhi, const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRootSignature(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature)
		{
			const Rhi::Context& context = openGLRhi.getContext();

			{ // Copy the parameter data
				const uint32_t numberOfParameters = mRootSignature.numberOfParameters;
				if (numberOfParameters > 0)
				{
					mRootSignature.parameters = RHI_MALLOC_TYPED(context, Rhi::RootParameter, numberOfParameters);
					Rhi::RootParameter* destinationRootParameters = const_cast<Rhi::RootParameter*>(mRootSignature.parameters);
					memcpy(destinationRootParameters, rootSignature.parameters, sizeof(Rhi::RootParameter) * numberOfParameters);

					// Copy the descriptor table data
					for (uint32_t i = 0; i < numberOfParameters; ++i)
					{
						Rhi::RootParameter& destinationRootParameter = destinationRootParameters[i];
						const Rhi::RootParameter& sourceRootParameter = rootSignature.parameters[i];
						if (Rhi::RootParameterType::DESCRIPTOR_TABLE == destinationRootParameter.parameterType)
						{
							const uint32_t numberOfDescriptorRanges = destinationRootParameter.descriptorTable.numberOfDescriptorRanges;
							destinationRootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(RHI_MALLOC_TYPED(context, Rhi::DescriptorRange, numberOfDescriptorRanges));
							memcpy(reinterpret_cast<Rhi::DescriptorRange*>(destinationRootParameter.descriptorTable.descriptorRanges), reinterpret_cast<const Rhi::DescriptorRange*>(sourceRootParameter.descriptorTable.descriptorRanges), sizeof(Rhi::DescriptorRange) * numberOfDescriptorRanges);
						}
					}
				}
			}

			{ // Copy the static sampler data
				const uint32_t numberOfStaticSamplers = mRootSignature.numberOfStaticSamplers;
				if (numberOfStaticSamplers > 0)
				{
					mRootSignature.staticSamplers = RHI_MALLOC_TYPED(context, Rhi::StaticSampler, numberOfStaticSamplers);
					memcpy(const_cast<Rhi::StaticSampler*>(mRootSignature.staticSamplers), rootSignature.staticSamplers, sizeof(Rhi::StaticSampler) * numberOfStaticSamplers);
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
			const Rhi::Context& context = getRhi().getContext();
			if (nullptr != mRootSignature.parameters)
			{
				for (uint32_t i = 0; i < mRootSignature.numberOfParameters; ++i)
				{
					const Rhi::RootParameter& rootParameter = mRootSignature.parameters[i];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_FREE(context, reinterpret_cast<Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges));
					}
				}
				RHI_FREE(context, const_cast<Rhi::RootParameter*>(mRootSignature.parameters));
			}
			RHI_FREE(context, const_cast<Rhi::StaticSampler*>(mRootSignature.staticSamplers));
		}

		/**
		*  @brief
		*    Return the root signature data
		*
		*  @return
		*    The root signature data
		*/
		[[nodiscard]] inline const Rhi::RootSignature& getRootSignature() const
		{
			return mRootSignature;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRootSignature methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Rhi::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Rhi::IRhi& rhi = getRhi();

			// Sanity checks
			RHI_ASSERT(rhi.getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The OpenGL root parameter index is out-of-bounds")
			RHI_ASSERT(rhi.getContext(), numberOfResources > 0, "The number of OpenGL resources must not be zero")
			RHI_ASSERT(rhi.getContext(), nullptr != resources, "The OpenGL resource pointers must be valid")

			// Create resource group
			return RHI_NEW(rhi.getContext(), ResourceGroup)(rhi, mRootSignature, rootParameterIndex, numberOfResources, resources, samplerStates RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), RootSignature, this);
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
		Rhi::RootSignature mRootSignature;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Buffer/VertexBuffer.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL vertex buffer object (VBO, "array buffer" in OpenGL terminology) interface
	*/
	class VertexBuffer : public Rhi::IVertexBuffer
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit VertexBuffer(OpenGLRhi& openGLRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Buffer/VertexBufferBind.h                   ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBufferBind(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			VertexBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL array buffer
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLArrayBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VBO", 6)	// 6 = "VBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLArrayBuffer, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Buffer/VertexBufferDsa.h                    ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBufferDsa(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			VertexBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			if (openGLRhi.getExtensions().isGL_ARB_direct_state_access())
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

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VBO", 6)	// 6 = "VBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLArrayBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/IndexBuffer.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL index buffer object (IBO, "element array buffer" in OpenGL terminology) interface
	*/
	class IndexBuffer : public Rhi::IIndexBuffer
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), IndexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		inline IndexBuffer(OpenGLRhi& openGLRhi, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IIndexBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLElementArrayBuffer(0),
			mOpenGLType(Mapping::getOpenGLType(indexBufferFormat)),
			mIndexSizeInBytes(Rhi::IndexBufferFormat::getNumberOfBytesPerElement(indexBufferFormat))
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
	//[ OpenGLRhi/Buffer/IndexBufferBind.h                    ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBufferBind(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IndexBuffer(openGLRhi, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL element array buffer
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLElementArrayBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IBO", 6)	// 6 = "IBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLElementArrayBuffer, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Buffer/IndexBufferDsa.h                     ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBufferDsa(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IndexBuffer(openGLRhi, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			if (openGLRhi.getExtensions().isGL_ARB_direct_state_access())
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

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IBO", 6)	// 6 = "IBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLElementArrayBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/VertexArray.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL vertex array interface
	*/
	class VertexArray : public Rhi::IVertexArray
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
			static_cast<OpenGLRhi&>(getRhi()).VertexArrayMakeId.DestroyID(getId());
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexArray, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] internalResourceType
		*    Internal resource type
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		inline VertexArray(OpenGLRhi& openGLRhi, IndexBuffer* indexBuffer, InternalResourceType::Enum internalResourceType, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexArray(openGLRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Buffer/VertexArrayNoVao.h                   ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		VertexArrayNoVao(OpenGLRhi& openGLRhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			VertexArray(openGLRhi, indexBuffer, InternalResourceType::NO_VAO, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfAttributes(vertexAttributes.numberOfAttributes),
			mAttributes(mNumberOfAttributes ? RHI_MALLOC_TYPED(openGLRhi.getContext(), Rhi::VertexAttribute, mNumberOfAttributes) : nullptr),
			mNumberOfVertexBuffers(numberOfVertexBuffers),
			mVertexBuffers(numberOfVertexBuffers ? RHI_MALLOC_TYPED(openGLRhi.getContext(), Rhi::VertexArrayVertexBuffer, numberOfVertexBuffers) : nullptr),
			mIsGL_ARB_instanced_arrays(openGLRhi.getExtensions().isGL_ARB_instanced_arrays())
		{
			// Copy over the data
			if (nullptr != mAttributes)
			{
				memcpy(mAttributes, vertexAttributes.attributes, sizeof(Rhi::VertexAttribute) * mNumberOfAttributes);
			}
			if (nullptr != mVertexBuffers)
			{
				memcpy(mVertexBuffers, vertexBuffers, sizeof(Rhi::VertexArrayVertexBuffer) * mNumberOfVertexBuffers);
			}

			// Add a reference to the used vertex buffers
			const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = mVertexBuffers + mNumberOfVertexBuffers;
			for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = mVertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
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
			const Rhi::Context& context = getRhi().getContext();
			RHI_FREE(context, mAttributes);

			// Destroy the vertex array vertex buffers
			if (nullptr != mVertexBuffers)
			{
				// Release the reference to the used vertex buffers
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = mVertexBuffers + mNumberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = mVertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					vertexBuffer->vertexBuffer->releaseReference();
				}

				// Cleanup
				RHI_FREE(context, mVertexBuffers);
			}
		}

		/**
		*  @brief
		*    Enable OpenGL vertex attribute arrays
		*/
		void enableOpenGLVertexAttribArrays()
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL array buffer
				// -> Using "GL_EXT_direct_state_access" this would not help in here because "glVertexAttribPointerARB" is not specified there :/
				GLint openGLArrayBufferBackup = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &openGLArrayBufferBackup);
			#endif

			// Loop through all attributes
			// -> We're using "glBindAttribLocation()" when linking the program so we have known attribute locations (the vertex array can't know about the program)
			GLuint attributeLocation = 0;
			const Rhi::VertexAttribute* attributeEnd = mAttributes + mNumberOfAttributes;
			for (const Rhi::VertexAttribute* attribute = mAttributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Set the OpenGL vertex attribute pointer
				// TODO(co) Add security check: Is the given resource one of the currently used RHI?
				const Rhi::VertexArrayVertexBuffer& vertexArrayVertexBuffer = mVertexBuffers[attribute->inputSlot];
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
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
			const Rhi::VertexAttribute* attributeEnd = mAttributes + mNumberOfAttributes;
			for (const Rhi::VertexAttribute* attribute = mAttributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
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
		uint32_t					  mNumberOfAttributes;			///< Number of attributes (position, color, texture coordinate, normal...), having zero attributes is valid
		Rhi::VertexAttribute*		  mAttributes;					///< At least "numberOfAttributes" instances of vertex attributes, can be a null pointer in case there are zero attributes
		uint32_t					  mNumberOfVertexBuffers;		///< Number of vertex buffers, having zero vertex buffers is valid
		Rhi::VertexArrayVertexBuffer* mVertexBuffers;				///< At least mNumberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		bool						  mIsGL_ARB_instanced_arrays;	///< Is the "GL_ARB_instanced_arrays"-extension supported?


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Buffer/VertexArrayVao.h                     ]
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
				RHI_FREE(getRhi().getContext(), mVertexBuffers);
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
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		VertexArrayVao(OpenGLRhi& openGLRhi, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			VertexArray(openGLRhi, indexBuffer, InternalResourceType::VAO, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLVertexArray(0),
			mNumberOfVertexBuffers(numberOfVertexBuffers),
			mVertexBuffers(nullptr)
		{
			// Add a reference to the used vertex buffers
			if (numberOfVertexBuffers > 0)
			{
				mVertexBuffers = RHI_MALLOC_TYPED(openGLRhi.getContext(), VertexBuffer*, numberOfVertexBuffers);

				// Loop through all vertex buffers
				VertexBuffer** currentVertexBuffers = mVertexBuffers;
				const Rhi::VertexArrayVertexBuffer* vertexBuffersEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBuffersEnd; ++vertexBuffer, ++currentVertexBuffers)
				{
					// TODO(co) Add security check: Is the given resource one of the currently used RHI?
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
	//[ OpenGLRhi/Buffer/VertexArrayVaoBind.h                 ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		VertexArrayVaoBind(OpenGLRhi& openGLRhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			VertexArrayVao(openGLRhi, numberOfVertexBuffers, vertexBuffers, indexBuffer, id RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Vertex buffer reference handling is done within the base class "VertexArrayVao"

			#ifdef RHI_OPENGL_STATE_CLEANUP
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
			const Rhi::VertexAttribute* attributeEnd = vertexAttributes.attributes + vertexAttributes.numberOfAttributes;
			for (const Rhi::VertexAttribute* attribute = vertexAttributes.attributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Set the OpenGL vertex attribute pointer
				// TODO(co) Add security check: Is the given resource one of the currently used RHI?
				const Rhi::VertexArrayVertexBuffer& vertexArrayVertexBuffer = vertexBuffers[attribute->inputSlot];
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
				if (attribute->instancesPerElement > 0 && openGLRhi.getExtensions().isGL_ARB_instanced_arrays())
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL vertex array
				glBindVertexArray(static_cast<GLuint>(openGLVertexArrayBackup));

				// Be polite and restore the previous bound OpenGL element array buffer
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLElementArrayBufferBackup));

				// Be polite and restore the previous bound OpenGL array buffer
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLArrayBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VAO", 6)	// 6 = "VAO: " including terminating zero
					glObjectLabel(GL_VERTEX_ARRAY, mOpenGLVertexArray, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Buffer/VertexArrayVaoDsa.h                  ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		VertexArrayVaoDsa(OpenGLRhi& openGLRhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			VertexArrayVao(openGLRhi, numberOfVertexBuffers, vertexBuffers, indexBuffer, id RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Vertex buffer reference handling is done within the base class "VertexArrayVao"
			const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();
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
			const Rhi::VertexAttribute* attributeEnd = vertexAttributes.attributes + vertexAttributes.numberOfAttributes;
			for (const Rhi::VertexAttribute* attribute = vertexAttributes.attributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
			{
				// Set the OpenGL vertex attribute pointer
				// TODO(co) Add security check: Is the given resource one of the currently used RHI?
				const Rhi::VertexArrayVertexBuffer& vertexArrayVertexBuffer = vertexBuffers[attribute->inputSlot];

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
					if (attribute->instancesPerElement > 0 && openGLRhi.getExtensions().isGL_ARB_instanced_arrays())
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
					if (attribute->instancesPerElement > 0 && openGLRhi.getExtensions().isGL_ARB_instanced_arrays())
					{
						// Sadly, DSA has no support for "GL_ARB_instanced_arrays", so, we have to use the bind way
						// -> Keep the bind-horror as local as possible

						#ifdef RHI_OPENGL_STATE_CLEANUP
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

						#ifdef RHI_OPENGL_STATE_CLEANUP
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

					#ifdef RHI_OPENGL_STATE_CLEANUP
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

					#ifdef RHI_OPENGL_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL vertex array
						glBindVertexArray(static_cast<GLuint>(openGLVertexArrayBackup));

						// Be polite and restore the previous bound OpenGL element array buffer
						glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, static_cast<GLuint>(openGLElementArrayBufferBackup));
					#endif
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VAO", 6)	// 6 = "VAO: " including terminating zero
					glObjectLabel(GL_VERTEX_ARRAY, mOpenGLVertexArray, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/TextureBuffer.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL texture buffer object (TBO) interface
	*/
	class TextureBuffer : public Rhi::ITextureBuffer
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		inline TextureBuffer(OpenGLRhi& openGLRhi, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITextureBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Buffer/TextureBufferBind.h                  ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBufferBind(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			TextureBuffer(openGLRhi, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			{ // Buffer part
				#ifdef RHI_OPENGL_STATE_CLEANUP
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

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture buffer
					glBindBufferARB(GL_TEXTURE_BUFFER_ARB, static_cast<GLuint>(openGLTextureBufferBackup));
				#endif
			}

			{ // Texture part
				#ifdef RHI_OPENGL_STATE_CLEANUP
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

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture
					glBindTexture(GL_TEXTURE_BUFFER_ARB, static_cast<GLuint>(openGLTextureBackup));
				#endif
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6)	// 6 = "TBO: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
					glObjectLabel(GL_BUFFER, mOpenGLTextureBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/TextureBufferDsa.h                   ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBufferDsa(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			TextureBuffer(openGLRhi, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			if (openGLRhi.getExtensions().isGL_ARB_direct_state_access())
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
					#ifdef RHI_OPENGL_STATE_CLEANUP
						// Backup the currently bound OpenGL texture
						GLint openGLTextureBackup = 0;
						glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_ARB, &openGLTextureBackup);
					#endif

					// Make this OpenGL texture instance to the currently used one
					glBindTexture(GL_TEXTURE_BUFFER_ARB, mOpenGLTexture);

					// Attaches the storage for the buffer object to the active buffer texture
					// -> Sadly, there's no direct state access (DSA) function defined for this in "GL_EXT_direct_state_access"
					glTexBufferARB(GL_TEXTURE_BUFFER_ARB, mOpenGLInternalFormat, mOpenGLTextureBuffer);

					#ifdef RHI_OPENGL_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL texture
						glBindTexture(GL_TEXTURE_BUFFER_ARB, static_cast<GLuint>(openGLTextureBackup));
					#endif
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6)	// 6 = "TBO: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
					glObjectLabel(GL_BUFFER, mOpenGLTextureBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/StructuredBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL structured buffer object (SBO) interface
	*/
	class StructuredBuffer : public Rhi::IStructuredBuffer
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), StructuredBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit StructuredBuffer(OpenGLRhi& openGLRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IStructuredBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Buffer/StructuredBufferBind.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL structured buffer object (SBO) class, traditional bind version
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		StructuredBufferBind(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			StructuredBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL structured buffer
				glBindBufferARB(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(openGLStructuredBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "SBO", 6)	// 6 = "SBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLStructuredBuffer, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Buffer/StructuredBufferDsa.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL structured buffer object (SBO) class, effective direct state access (DSA)
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		StructuredBufferDsa(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			StructuredBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			if (openGLRhi.getExtensions().isGL_ARB_direct_state_access())
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

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "SBO", 6)	// 6 = "SBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLStructuredBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/IndirectBuffer.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL indirect buffer object interface
	*/
	class IndirectBuffer : public Rhi::IIndirectBuffer
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
	//[ Public virtual Rhi::IIndirectBuffer methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const uint8_t* getEmulationData() const override
		{
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), IndirectBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit IndirectBuffer(OpenGLRhi& openGLRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IIndirectBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Buffer/IndirectBufferBind.h                 ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBufferBind(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IndirectBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL indirect buffer
				glBindBufferARB(GL_DRAW_INDIRECT_BUFFER, static_cast<GLuint>(openGLIndirectBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IndirectBufferObject", 23)	// 23 = "IndirectBufferObject: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLIndirectBuffer, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Buffer/IndirectBufferDsa.h                  ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer), the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBufferDsa(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IndirectBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			if (openGLRhi.getExtensions().isGL_ARB_direct_state_access())
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

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IndirectBufferObject", 23)	// 23 = "IndirectBufferObject: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLIndirectBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/UniformBuffer.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
	*/
	class UniformBuffer : public Rhi::IUniformBuffer
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), UniformBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit UniformBuffer(OpenGLRhi& openGLRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IUniformBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Buffer/UniformBufferBind.h                  ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBufferBind(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			UniformBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// TODO(co) Review OpenGL uniform buffer alignment topic

			#ifdef RHI_OPENGL_STATE_CLEANUP
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL uniform buffer
				glBindBufferARB(GL_UNIFORM_BUFFER, static_cast<GLuint>(openGLUniformBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "UBO", 6)	// 6 = "UBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLUniformBuffer, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Buffer/UniformBufferDsa.h                   ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBufferDsa(OpenGLRhi& openGLRhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			UniformBuffer(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// TODO(co) Review OpenGL uniform buffer alignment topic

			if (openGLRhi.getExtensions().isGL_ARB_direct_state_access())
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

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "UBO", 6)	// 6 = "UBO: " including terminating zero
					glObjectLabel(GL_BUFFER, mOpenGLUniformBuffer, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Buffer/BufferManager.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL buffer manager interface
	*/
	class BufferManager final : public Rhi::IBufferManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit BufferManager(OpenGLRhi& openGLRhi) :
			IBufferManager(openGLRhi),
			mExtensions(&openGLRhi.getExtensions())
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BufferManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IBufferManager methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Rhi::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "GL_ARB_vertex_buffer_object" required
			if (mExtensions->isGL_ARB_vertex_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), VertexBufferDsa)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), VertexBufferBind)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::IndexBufferFormat::Enum indexBufferFormat = Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "GL_ARB_vertex_buffer_object" required
			if (mExtensions->isGL_ARB_vertex_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), IndexBufferDsa)(openGLRhi, numberOfBytes, data, bufferUsage, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), IndexBufferBind)(openGLRhi, numberOfBytes, data, bufferUsage, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IVertexArray* createVertexArray(const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, Rhi::IIndexBuffer* indexBuffer = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity checks
			#ifdef RHI_DEBUG
			{
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RHI_ASSERT(openGLRhi.getContext(), &openGLRhi == &vertexBuffer->vertexBuffer->getRhi(), "OpenGL error: The given vertex buffer resource is owned by another RHI instance")
				}
			}
			#endif
			RHI_ASSERT(openGLRhi.getContext(), nullptr == indexBuffer || &openGLRhi == &indexBuffer->getRhi(), "OpenGL error: The given index buffer resource is owned by another RHI instance")

			// Create vertex array
			uint16_t id = 0;
			if (openGLRhi.VertexArrayMakeId.CreateID(id))
			{
				// Is "GL_ARB_vertex_array_object" there?
				if (mExtensions->isGL_ARB_vertex_array_object())
				{
					// Effective vertex array object (VAO)

					// Is "GL_EXT_direct_state_access" there?
					if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
					{
						// Effective direct state access (DSA)
						return RHI_NEW(openGLRhi.getContext(), VertexArrayVaoDsa)(openGLRhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
					}
					else
					{
						// Traditional bind version
						return RHI_NEW(openGLRhi.getContext(), VertexArrayVaoBind)(openGLRhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
					}
				}
				else
				{
					// Traditional version
					return RHI_NEW(openGLRhi.getContext(), VertexArrayNoVao)(openGLRhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}

			// Error: Ensure a correct reference counter behaviour
			const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
			for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
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

		[[nodiscard]] virtual Rhi::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t = Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::R32G32B32A32F RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), (numberOfBytes % Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The OpenGL texture buffer size must be a multiple of the selected texture format bytes per texel")

			// "GL_ARB_texture_buffer_object" required
			if (mExtensions->isGL_ARB_texture_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), TextureBufferDsa)(openGLRhi, numberOfBytes, data, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), TextureBufferBind)(openGLRhi, numberOfBytes, data, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The OpenGL structured buffer size must be a multiple of the given number of structure bytes")
			RHI_ASSERT(openGLRhi.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The OpenGL structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// "GL_ARB_shader_storage_buffer_object" required
			if (mExtensions->isGL_ARB_shader_storage_buffer_object())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), StructuredBufferDsa)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), StructuredBufferBind)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t indirectBufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid OpenGL flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RHI_ASSERT(openGLRhi.getContext(), !((indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid OpenGL flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RHI_ASSERT(openGLRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawArguments)) == 0, "OpenGL indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RHI_ASSERT(openGLRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawIndexedArguments)) == 0, "OpenGL indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// "GL_ARB_draw_indirect" required
			if (mExtensions->isGL_ARB_draw_indirect())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), IndirectBufferDsa)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), IndirectBufferBind)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "GL_ARB_uniform_buffer_object" required
			if (mExtensions->isGL_ARB_uniform_buffer_object())
			{
				OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

				// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
				// -> Inside GLSL "layout(binding = 0, std140) writeonly uniform OutputUniformBuffer" will result in the GLSL compiler error "Failed to parse the GLSL shader source code: ERROR: 0:85: 'assign' :  l-value required "anon@6" (can't modify a uniform)"
				// -> Inside GLSL "layout(binding = 0, std430) writeonly buffer  OutputUniformBuffer" will work in OpenGL but will fail in Vulkan with "Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT" Object: "0" Location: "0" Message code: "13" Layer prefix: "Validation" Message: "Object: VK_NULL_HANDLE (Type = 0) | Type mismatch on descriptor slot 0.0 (used as type `ptr to uniform struct of (vec4 of float32)`) but descriptor of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER""
				// RHI_ASSERT(openGLRhi.getContext(), (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid OpenGL buffer flags, uniform buffer can't be used for unordered access")
				// RHI_ASSERT(openGLRhi.getContext(), (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0, "Invalid OpenGL buffer flags, uniform buffer must be used as shader resource")

				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), UniformBufferDsa)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), UniformBufferBind)(openGLRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				// Error!
				return nullptr;
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), BufferManager, this);
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
	//[ OpenGLRhi/Texture/Texture1D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 1D texture interface
	*/
	class Texture1D : public Rhi::ITexture1D
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
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture1D, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture1D(OpenGLRhi& openGLRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture1D(openGLRhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Texture/Texture1DBind.h                     ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1DBind(OpenGLRhi& openGLRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture1D(openGLRhi, width, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_1D, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_1D, mOpenGLTexture);

			// Upload the texture data
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						glCompressedTexImage1DARB(GL_TEXTURE_1D, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glCompressedTexImage1DARB(GL_TEXTURE_1D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
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
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
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
			if ((textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) && openGLRhi.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_1D);
				glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_1D, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture", 13)	// 13 = "1D texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture1DDsa.h                      ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1DDsa(OpenGLRhi& openGLRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture1D(openGLRhi, width, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Multisample texture?
			const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
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
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
							glCompressedTextureSubImage1D(mOpenGLTexture, 0, 0, static_cast<GLsizei>(width), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
						}
					}
					else
					{
						glCompressedTextureImage1DEXT(mOpenGLTexture, GL_TEXTURE_1D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
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
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture", 13)	// 13 = "1D texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture1DArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 1D array texture interface
	*/
	class Texture1DArray : public Rhi::ITexture1DArray
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture1DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture1DArray(OpenGLRhi& openGLRhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture1DArray(openGLRhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Texture/Texture1DArrayBind.h                ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1DArrayBind(OpenGLRhi& openGLRhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture1DArray(openGLRhi, width, numberOfSlices, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_1D_ARRAY_EXT, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, mOpenGLTexture);

			// TODO(co) Add support for user provided mipmaps
			// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
			//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   etc.

			// Upload the base map of the texture (mipmaps are automatically created as soon as the base map is changed)
			glTexImage2D(GL_TEXTURE_1D_ARRAY_EXT, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) && openGLRhi.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_1D_ARRAY_EXT);
				glTexParameteri(GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_1D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture array", 19)	// 19 = "1D texture array: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture1DArrayDsa.h                 ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] numberOfSlices
		*    Number of slices, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1DArrayDsa(OpenGLRhi& openGLRhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture1DArray(openGLRhi, width, numberOfSlices, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Create the OpenGL texture instance
			#ifdef _WIN32
				// TODO(co) It appears that DSA "glGenerateTextureMipmap()" is not working (one notices the noise) or we're using it wrong, tested with
				//			- "InstancedCubes"-example -> "CubeRendereDrawInstanced"
				//		    - AMD 290X Radeon software version 17.7.2 as well as with GeForce 980m 384.94
				//		    - Windows 10 x64
				const bool isArbDsa = (openGLRhi.getExtensions().isGL_ARB_direct_state_access() && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) == 0);
			#else
				const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();
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
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
							glCompressedTextureSubImage2D(mOpenGLTexture, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices), data);
						}
					}
					else
					{
						glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_1D_ARRAY_EXT, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(numberOfSlices), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture array", 19)	// 19 = "1D texture array: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture2D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 2D texture interface
	*/
	class Texture2D : public Rhi::ITexture2D
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
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture2D, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		inline Texture2D(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture2D(openGLRhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Texture/Texture2DBind.h                     ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		Texture2DBind(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture2D(openGLRhi, width, height, textureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS), "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Rhi::TextureFlag::RENDER_TARGET), "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			// Multisample texture?
			if (numberOfMultisamples > 1)
			{
				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL texture
					GLint openGLTextureBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &openGLTextureBackup);
				#endif

				// Make this OpenGL texture instance to the currently used one
				glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mOpenGLTexture);

				// Define the texture
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numberOfMultisamples, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_TRUE);

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture
					glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(openGLTextureBackup));
				#endif
			}
			else
			{
				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Backup the currently set alignment
					GLint openGLAlignmentBackup = 0;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

					// Backup the currently bound OpenGL texture
					GLint openGLTextureBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLTextureBackup);
				#endif

				// Set correct unpack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

				// Calculate the number of mipmaps
				const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
				const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
				const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

				// Make this OpenGL texture instance to the currently used one
				glBindTexture(GL_TEXTURE_2D, mOpenGLTexture);

				// Upload the texture data
				if (Rhi::TextureFormat::isCompressed(textureFormat))
				{
					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
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
						glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
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
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
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
				if ((textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) && openGLRhi.getExtensions().isGL_ARB_framebuffer_object())
				{
					glGenerateMipmap(GL_TEXTURE_2D);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL texture
					glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLTextureBackup));

					// Restore previous alignment
					glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
				#endif
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture", 13)	// 13 = "2D texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
			#endif
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
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
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
	//[ OpenGLRhi/Texture/Texture2DDsa.h                      ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] height
		*    Texture height, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		Texture2DDsa(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture2D(openGLRhi, width, height, textureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS), "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Rhi::TextureFlag::RENDER_TARGET), "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Multisample texture?
			const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();
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

					#ifdef RHI_OPENGL_STATE_CLEANUP
						// Backup the currently bound OpenGL texture
						GLint openGLTextureBackup = 0;
						glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &openGLTextureBackup);
					#endif

					// Make this OpenGL texture instance to the currently used one
					glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mOpenGLTexture);

					// Define the texture
					// -> Sadly, there's no direct state access (DSA) function defined for this in "GL_EXT_direct_state_access"
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numberOfMultisamples, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_TRUE);

					#ifdef RHI_OPENGL_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL texture
						glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(openGLTextureBackup));
					#endif
				}
			}
			else
			{
				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Backup the currently set alignment
					GLint openGLAlignmentBackup = 0;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
				#endif

				// Set correct unpack alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

				// Calculate the number of mipmaps
				const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
				const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
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
				if (Rhi::TextureFormat::isCompressed(textureFormat))
				{
					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
							if (isArbDsa)
							{
								// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
								glCompressedTextureSubImage2D(mOpenGLTexture, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
							}
						}
						else
						{
							glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_2D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
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
							const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
							if (isArbDsa)
							{
								// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
				if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
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

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Restore previous alignment
					glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
				#endif
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture", 13)	// 13 = "2D texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/Texture/Texture2DArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 2D array texture interface
	*/
	class Texture2DArray : public Rhi::ITexture2DArray
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture2DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture2DArray(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture2DArray(openGLRhi, width, height, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Texture/Texture2DArrayBind.h                ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture2DArrayBind(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture2DArray(openGLRhi, width, height, numberOfSlices, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY_EXT, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mOpenGLTexture);

			// TODO(co) Add support for user provided mipmaps
			// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
			//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   etc.

			// Upload the base map of the texture (mipmaps are automatically created as soon as the base map is changed)
			glTexImage3DEXT(GL_TEXTURE_2D_ARRAY_EXT, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) && openGLRhi.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_2D_ARRAY_EXT);
				glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture array", 19)	// 19 = "2D texture array: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture2DArrayDsa.h                 ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture2DArrayDsa(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture2DArray(openGLRhi, width, height, numberOfSlices, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Create the OpenGL texture instance
			#ifdef _WIN32
				// TODO(co) It appears that DSA "glGenerateTextureMipmap()" is not working (one notices the noise) or we're using it wrong, tested with
				//			- "InstancedCubes"-example -> "CubeRendereDrawInstanced"
				//		    - AMD 290X Radeon software version 17.7.2 as well as with GeForce 980m 384.94
				//		    - Windows 10 x64
				const bool isArbDsa = (openGLRhi.getExtensions().isGL_ARB_direct_state_access() && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) == 0);
			#else
				const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();
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
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
							glCompressedTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices), data);
						}
					}
					else
					{
						glCompressedTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_2D_ARRAY_EXT, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * numberOfSlices);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture array", 19)	// 19 = "2D texture array: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture3D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL 3D texture interface
	*/
	class Texture3D : public Rhi::ITexture3D
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
		[[nodiscard]] inline Rhi::TextureFormat::Enum getTextureFormat() const
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
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture3D, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] depth
		*    The depth of the texture
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline Texture3D(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture3D(openGLRhi, width, height, depth RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLTexture(0),
			mTextureFormat(textureFormat),
			mOpenGLInternalFormat(Mapping::getOpenGLInternalFormat(textureFormat)),
			mOpenGLPixelUnpackBuffer(0)
		{}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint					 mOpenGLTexture;			///< OpenGL texture, can be zero if no resource is allocated
		Rhi::TextureFormat::Enum mTextureFormat;
		GLuint					 mOpenGLInternalFormat;		///< OpenGL internal format
		GLuint					 mOpenGLPixelUnpackBuffer;	///< OpenGL pixel unpack buffer for dynamic textures, can be zero if no resource is allocated


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Texture/Texture3DBind.h                     ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture3DBind(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture3D(openGLRhi, width, height, depth, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_3D, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create OpenGL pixel unpack buffer for dynamic textures, if necessary
			if (Rhi::TextureUsage::IMMUTABLE != textureUsage)
			{
				// Backup the currently bound OpenGL pixel unpack buffer
				#ifdef RHI_OPENGL_STATE_CLEANUP
					GLint openGLUnpackBufferBackup = 0;
					glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &openGLUnpackBufferBackup);
				#endif

				// Create the OpenGL pixel unpack buffer
				glGenBuffersARB(1, &mOpenGLPixelUnpackBuffer);

				// Bind this OpenGL pixel unpack buffer, the OpenGL pixel unpack buffer must be able to hold the top-level mipmap
				// TODO(co) Or must the OpenGL pixel unpack buffer able to hold the entire texture including all mipmaps? Depends on the later usage I assume.
				const uint32_t numberOfBytes = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth;
				glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, mOpenGLPixelUnpackBuffer);
				glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, static_cast<GLsizeiptrARB>(numberOfBytes), nullptr, static_cast<GLenum>(GL_STREAM_DRAW));

				// Be polite and restore the previous bound OpenGL pixel unpack buffer
				#ifdef RHI_OPENGL_STATE_CLEANUP
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, static_cast<GLuint>(openGLUnpackBufferBackup));
				#else
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
				#endif
			}

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(openGLRhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "OpenGL immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_3D, mOpenGLTexture);

			// Upload the texture data
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
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
					glCompressedTexImage3DARB(GL_TEXTURE_3D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
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
			if ((textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) && openGLRhi.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_3D);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_3D, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "3D texture", 13)	// 13 = "3D texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/Texture3DDsa.h                      ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture3DDsa(OpenGLRhi& openGLRhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Texture3D(openGLRhi, width, height, depth, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Create OpenGL pixel unpack buffer for dynamic textures, if necessary
			if (Rhi::TextureUsage::IMMUTABLE != textureUsage)
			{
				// Create the OpenGL pixel unpack buffer
				glCreateBuffers(1, &mOpenGLPixelUnpackBuffer);

				// The OpenGL pixel unpack buffer must be able to hold the top-level mipmap
				// TODO(co) Or must the OpenGL pixel unpack buffer able to hold the entire texture including all mipmaps? Depends on the later usage I assume.
				const uint32_t numberOfBytes = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth;
				glNamedBufferData(mOpenGLPixelUnpackBuffer, static_cast<GLsizeiptr>(numberOfBytes), nullptr, GL_STREAM_DRAW);
			}

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(openGLRhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "OpenGL immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;

			// Create the OpenGL texture instance
			const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();
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
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
							glCompressedTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
						}
					}
					else
					{
						glCompressedTextureImage3DEXT(mOpenGLTexture, GL_TEXTURE_3D, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
					}
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.

					// Upload all mipmaps
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						if (isArbDsa)
						{
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
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
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "3D texture", 13)	// 13 = "3D texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/TextureCube.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL cube texture interface
	*/
	class TextureCube : public Rhi::ITextureCube
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
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLTexture));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureCube, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] textureFormat
		*    Texture format
		*/
		inline TextureCube(OpenGLRhi& openGLRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITextureCube(openGLRhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
	//[ OpenGLRhi/Texture/TextureCubeBind.h                   ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		TextureCubeBind(OpenGLRhi& openGLRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			TextureCube(openGLRhi, width, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			// Create the OpenGL texture instance
			glGenTextures(1, &mOpenGLTexture);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);

				// Backup the currently bound OpenGL texture
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &openGLTextureBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Make this OpenGL texture instance to the currently used one
			glBindTexture(GL_TEXTURE_CUBE_MAP, mOpenGLTexture);

			// Upload the texture data
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width);
					for (uint32_t face = 0; face < 6; ++face)
					{
						glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, static_cast<GLsizei>(numberOfBytesPerSlice), data);
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
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, format, type, data);

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width);
					const uint32_t openGLFormat = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t openGLType = Mapping::getOpenGLType(textureFormat);
					for (uint32_t face = 0; face < 6; ++face)
					{
						glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, openGLFormat, openGLType, data);
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if ((textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS) && openGLRhi.getExtensions().isGL_ARB_framebuffer_object())
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(openGLTextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Cube texture", 15)	// 15 = "Cube texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/TextureCubeDsa.h                    ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		TextureCubeDsa(OpenGLRhi& openGLRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			TextureCube(openGLRhi, width, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Sanity checks
			RHI_ASSERT(openGLRhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL texture parameters")
			RHI_ASSERT(openGLRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL render target textures can't be filled using provided data")

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLAlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
			#endif

			// Set correct unpack alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Create the OpenGL texture instance
			// TODO(co) "GL_ARB_direct_state_access" AMD graphics card driver bug ahead
			// -> AMD graphics card: 13.02.2017 using Radeon software 17.1.1 on Microsoft Windows: Looks like "GL_ARB_direct_state_access" is broken when trying to use "glCompressedTextureSubImage3D()" for upload
			// -> Describes the same problem: https://community.amd.com/thread/194748 - "Upload data to GL_TEXTURE_CUBE_MAP with glTextureSubImage3D (DSA) broken ?"
			#ifdef _WIN32
				const bool isArbDsa = false;
			#else
				const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();
			#endif
			if (isArbDsa)
			{
				glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mOpenGLTexture);
				glTextureStorage2D(mOpenGLTexture, static_cast<GLsizei>(numberOfMipmaps), mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width));
			}
			else
			{
				glGenTextures(1, &mOpenGLTexture);
			}

			// Upload the texture data
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						if (isArbDsa)
						{
							// With ARB DSA cube maps are a special form of a cube map array so we can upload all 6 faces at once per mipmap
							// See https://www.khronos.org/opengl/wiki/Direct_State_Access (Last paragraph in "Changes from EXT")
							// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
							glCompressedTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 6, format, numberOfBytesPerSlice * 6, data);

							// Move on to the next mipmap
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice * 6;
						}
						else
						{
							for (uint32_t face = 0; face < 6; ++face)
							{
								// Upload the current face
								glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), format, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);

								// Move on to the next face
								data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
							}
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
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
							glCompressedTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 6, Mapping::getOpenGLFormat(textureFormat), static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width)) * 6, data);
						}
					}
					else
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glCompressedTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, mOpenGLInternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);

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
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//   etc.

					// Upload all mipmaps of all faces
					const uint32_t format = Mapping::getOpenGLFormat(textureFormat);
					const uint32_t type = Mapping::getOpenGLType(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							if (isArbDsa)
							{
								// We know that "data" must be valid when we're in here due to the "Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS"-flag
								glTextureSubImage3D(mOpenGLTexture, static_cast<GLint>(mipmap), 0, 0, static_cast<GLint>(face), static_cast<GLsizei>(width), static_cast<GLsizei>(width), 1, format, type, data);
							}
							else
							{
								glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, format, type, data);
							}

							// Move on to the next face
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
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
							glTextureSubImage3D(mOpenGLTexture, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 6, Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), data);
						}
					}
					else
					{
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width);
						const uint32_t openGLFormat = Mapping::getOpenGLFormat(textureFormat);
						const uint32_t openGLType = Mapping::getOpenGLType(textureFormat);
						for (uint32_t face = 0; face < 6; ++face)
						{
							glTextureImage2DEXT(mOpenGLTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, static_cast<GLint>(mOpenGLInternalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, openGLFormat, openGLType, data);
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Cube texture", 15)	// 15 = "Cube texture: " including terminating zero
					glObjectLabel(GL_TEXTURE, mOpenGLTexture, -1, detailedDebugName);
				}
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
	//[ OpenGLRhi/Texture/TextureManager.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL texture manager interface
	*/
	class TextureManager final : public Rhi::ITextureManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit TextureManager(OpenGLRhi& openGLRhi) :
			ITextureManager(openGLRhi),
			mExtensions(&openGLRhi.getExtensions())
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::ITextureManager methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Rhi::ITexture1D* createTexture1D(uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), width > 0, "OpenGL create texture 1D was called with invalid parameters")

			// Create 1D texture resource: Is "GL_EXT_direct_state_access" there?
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), Texture1DDsa)(openGLRhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), Texture1DBind)(openGLRhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}

		[[nodiscard]] virtual Rhi::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), width > 0 && numberOfSlices > 0, "OpenGL create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource, "GL_EXT_texture_array"-extension required
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_texture_array())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), Texture1DArrayDsa)(openGLRhi, width, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), Texture1DArrayBind)(openGLRhi, width, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), width > 0 && height > 0, "OpenGL create texture 2D was called with invalid parameters")

			// Create 2D texture resource: Is "GL_EXT_direct_state_access" there?
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), Texture2DDsa)(openGLRhi, width, height, textureFormat, data, textureFlags, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), Texture2DBind)(openGLRhi, width, height, textureFormat, data, textureFlags, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}

		[[nodiscard]] virtual Rhi::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), width > 0 && height > 0 && numberOfSlices > 0, "OpenGL create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource, "GL_EXT_texture_array"-extension required
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_texture_array())
			{
				// Is "GL_EXT_direct_state_access" there?
				if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
				{
					// Effective direct state access (DSA)
					return RHI_NEW(openGLRhi.getContext(), Texture2DArrayDsa)(openGLRhi, width, height, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
				else
				{
					// Traditional bind version
					return RHI_NEW(openGLRhi.getContext(), Texture2DArrayBind)(openGLRhi, width, height, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
				}
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), width > 0 && height > 0 && depth > 0, "OpenGL create texture 3D was called with invalid parameters")

			// Create 3D texture resource: Is "GL_EXT_direct_state_access" there?
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), Texture3DDsa)(openGLRhi, width, height, depth, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), Texture3DBind)(openGLRhi, width, height, depth, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}

		[[nodiscard]] virtual Rhi::ITextureCube* createTextureCube(uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), width > 0, "OpenGL create texture cube was called with invalid parameters")

			// Create cube texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL has no texture usage indication
			// Is "GL_EXT_direct_state_access" or "GL_ARB_direct_state_access" there?
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), TextureCubeDsa)(openGLRhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), TextureCubeBind)(openGLRhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}

		[[nodiscard]] virtual Rhi::ITextureCubeArray* createTextureCubeArray([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t numberOfSlices, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// TODO(co) Implement me
			#ifdef RHI_DEBUG
				debugName = debugName;
			#endif
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureManager, this);
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
	//[ OpenGLRhi/State/SamplerState.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL sampler state interface
	*/
	class SamplerState : public Rhi::ISamplerState
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), SamplerState, this);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit SamplerState(OpenGLRhi& openGLRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISamplerState(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerState(const SamplerState& source) = delete;
		SamplerState& operator =(const SamplerState& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/State/SamplerStateBind.h                    ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerStateBind(OpenGLRhi& openGLRhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			SamplerState(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLMagFilterMode(Mapping::getOpenGLMagFilterMode(openGLRhi.getContext(), samplerState.filter)),
			mOpenGLMinFilterMode(Mapping::getOpenGLMinFilterMode(openGLRhi.getContext(), samplerState.filter, samplerState.maxLod > 0.0f)),
			mOpenGLTextureAddressModeS(Mapping::getOpenGLTextureAddressMode(samplerState.addressU)),
			mOpenGLTextureAddressModeT(Mapping::getOpenGLTextureAddressMode(samplerState.addressV)),
			mOpenGLTextureAddressModeR(Mapping::getOpenGLTextureAddressMode(samplerState.addressW)),
			mMipLodBias(samplerState.mipLodBias),
			mMaxAnisotropy(static_cast<float>(samplerState.maxAnisotropy)),	// Maximum anisotropy is "uint32_t" in Direct3D 10 & 11
			mOpenGLCompareMode(Mapping::getOpenGLCompareMode(openGLRhi.getContext(), samplerState.filter)),
			mOpenGLComparisonFunc(Mapping::getOpenGLComparisonFunc(samplerState.comparisonFunc)),
			mMinLod(samplerState.minLod),
			mMaxLod(samplerState.maxLod)
		{
			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), samplerState.maxAnisotropy <= openGLRhi.getCapabilities().maximumAnisotropy, "Maximum OpenGL anisotropy value violated")

			// Rhi::SamplerState::borderColor[4]
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

			// Rhi::SamplerState::filter
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mOpenGLMagFilterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mOpenGLMinFilterMode);

			// Rhi::SamplerState::addressU
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mOpenGLTextureAddressModeS);

			// Rhi::SamplerState::addressV
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mOpenGLTextureAddressModeT);
			/*
			// TODO(co) Support for 3D textures
			// Rhi::SamplerState::addressW
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, mOpenGLTextureAddressModeR);

			// TODO(co) Complete me
			// Rhi::SamplerState::mipLodBias
			// -> "GL_EXT_texture_lod_bias"-extension
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_LOD_BIAS, samplerState.mipLodBias);

			// Rhi::SamplerState::maxAnisotropy
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerState.maxAnisotropy);

			// Rhi::SamplerState::comparisonFunc
			// -> "GL_EXT_shadow_funcs"/"GL_EXT_shadow_samplers"-extension
			glTexParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_MODE, Mapping::getOpenGLCompareMode(samplerState.filter));
			glTexParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_FUNC, Mapping::getOpenGLComparisonFunc(samplerState.comparisonFunc));

			// Rhi::SamplerState::borderColor[4]
			glSamplerParameterfv(mOpenGLSampler, GL_TEXTURE_BORDER_COLOR, samplerState.borderColor);

			// Rhi::SamplerState::minLod
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MIN_LOD, samplerState.minLod);

			// Rhi::SamplerState::maxLod
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_LOD, samplerState.maxLod);
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
		// "Rhi::SamplerState" translated into OpenGL
		GLint  mOpenGLMagFilterMode;		///< Rhi::SamplerState::filter
		GLint  mOpenGLMinFilterMode;		///< Rhi::SamplerState::filter
		GLint  mOpenGLTextureAddressModeS;	///< Rhi::SamplerState::addressU
		GLint  mOpenGLTextureAddressModeT;	///< Rhi::SamplerState::addressV
		GLint  mOpenGLTextureAddressModeR;	///< Rhi::SamplerState::addressW
		float  mMipLodBias;					///< Rhi::SamplerState::mipLodBias
		float  mMaxAnisotropy;				///< Rhi::SamplerState::maxAnisotropy
		GLint  mOpenGLCompareMode;			///< Rhi::SamplerState::comparisonFunc
		GLenum mOpenGLComparisonFunc;		///< Rhi::SamplerState::comparisonFunc
		float  mBorderColor[4];				///< Rhi::SamplerState::borderColor[4]
		float  mMinLod;						///< Rhi::SamplerState::minLod
		float  mMaxLod;						///< Rhi::SamplerState::maxLod


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/State/SamplerStateDsa.h                     ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		inline SamplerStateDsa(OpenGLRhi& openGLRhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			SamplerState(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mSamplerState(samplerState)
		{
			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), samplerState.maxAnisotropy <= openGLRhi.getCapabilities().maximumAnisotropy, "Maximum OpenGL anisotropy value violated")
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
		Rhi::SamplerState mSamplerState;	///< Sampler state


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/State/SamplerStateSo.h                      ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerStateSo(OpenGLRhi& openGLRhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			SamplerState(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLSampler(0)
		{
			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), samplerState.maxAnisotropy <= openGLRhi.getCapabilities().maximumAnisotropy, "Maximum OpenGL anisotropy value violated")

			// Create the OpenGL sampler
			glGenSamplers(1, &mOpenGLSampler);

			// Rhi::SamplerState::filter
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_MAG_FILTER, Mapping::getOpenGLMagFilterMode(openGLRhi.getContext(), samplerState.filter));
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_MIN_FILTER, Mapping::getOpenGLMinFilterMode(openGLRhi.getContext(), samplerState.filter, samplerState.maxLod > 0.0f));

			// Rhi::SamplerState::addressU
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_WRAP_S, Mapping::getOpenGLTextureAddressMode(samplerState.addressU));

			// Rhi::SamplerState::addressV
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_WRAP_T, Mapping::getOpenGLTextureAddressMode(samplerState.addressV));

			// Rhi::SamplerState::addressW
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_WRAP_R, Mapping::getOpenGLTextureAddressMode(samplerState.addressW));

			// Rhi::SamplerState::mipLodBias
			// -> "GL_EXT_texture_lod_bias"-extension
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_LOD_BIAS, samplerState.mipLodBias);

			// Rhi::SamplerState::maxAnisotropy
			// -> Maximum anisotropy is "uint32_t" in Direct3D 10 & 11
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(samplerState.maxAnisotropy));

			// Rhi::SamplerState::comparisonFunc
			// -> "GL_EXT_shadow_funcs"/"GL_EXT_shadow_samplers"-extension
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_MODE, Mapping::getOpenGLCompareMode(openGLRhi.getContext(), samplerState.filter));
			glSamplerParameteri(mOpenGLSampler, GL_TEXTURE_COMPARE_FUNC, static_cast<GLint>(Mapping::getOpenGLComparisonFunc(samplerState.comparisonFunc)));

			// Rhi::SamplerState::borderColor[4]
			glSamplerParameterfv(mOpenGLSampler, GL_TEXTURE_BORDER_COLOR, samplerState.borderColor);

			// Rhi::SamplerState::minLod
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MIN_LOD, samplerState.minLod);

			// Rhi::SamplerState::maxLod
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_LOD, samplerState.maxLod);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Sampler state", 16)	// 16 = "Sampler state: " including terminating zero
					glObjectLabel(GL_SAMPLER, mOpenGLSampler, -1, detailedDebugName);
				}
			#endif
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
	//[ OpenGLRhi/State/IState.h                              ]
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
	//[ OpenGLRhi/State/RasterizerState.h                     ]
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
		inline explicit RasterizerState(const Rhi::RasterizerState& rasterizerState) :
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
		[[nodiscard]] inline const Rhi::RasterizerState& getRasterizerState() const
		{
			return mRasterizerState;
		}

		/**
		*  @brief
		*    Set the OpenGL rasterizer states
		*/
		void setOpenGLRasterizerStates() const
		{
			// Rhi::RasterizerState::fillMode
			switch (mRasterizerState.fillMode)
			{
				// Wireframe
				case Rhi::FillMode::WIREFRAME:
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					break;

				// Solid
				default:
				case Rhi::FillMode::SOLID:
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					break;
			}

			// Rhi::RasterizerState::cullMode
			switch (mRasterizerState.cullMode)
			{
				// No culling
				default:
				case Rhi::CullMode::NONE:
					glDisable(GL_CULL_FACE);
					break;

				// Selects clockwise polygons as front-facing
				case Rhi::CullMode::FRONT:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);
					break;

				// Selects counterclockwise polygons as front-facing
				case Rhi::CullMode::BACK:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);
					break;
			}

			// Rhi::RasterizerState::frontCounterClockwise
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
		Rhi::RasterizerState mRasterizerState;		///< Rasterizer state
		GLenum				 mOpenGLFrontFaceMode;	///< OpenGL front face mode


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/State/DepthStencilState.h                   ]
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
		inline explicit DepthStencilState(const Rhi::DepthStencilState& depthStencilState) :
			mDepthStencilState(depthStencilState),
			mOpenGLDepthMaskEnabled(static_cast<GLboolean>((Rhi::DepthWriteMask::ALL == mDepthStencilState.depthWriteMask) ? GL_TRUE : GL_FALSE)),
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
		[[nodiscard]] inline const Rhi::DepthStencilState& getDepthStencilState() const
		{
			return mDepthStencilState;
		}

		/**
		*  @brief
		*    Set the OpenGL depth stencil states
		*/
		void setOpenGLDepthStencilStates() const
		{
			// Rhi::DepthStencilState::depthEnable
			if (mDepthStencilState.depthEnable)
			{
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				glDisable(GL_DEPTH_TEST);
			}

			// Rhi::DepthStencilState::depthWriteMask
			glDepthMask(mOpenGLDepthMaskEnabled);

			// Rhi::DepthStencilState::depthFunc
			glDepthFunc(static_cast<GLenum>(mOpenGLDepthFunc));

			// TODO(co) Map the rest of the depth stencil states
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::DepthStencilState mDepthStencilState;		///< Depth stencil state
		GLboolean			   mOpenGLDepthMaskEnabled;	///< OpenGL depth mask enabled state
		GLenum				   mOpenGLDepthFunc;		///< OpenGL depth function


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/State/BlendState.h                          ]
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
		inline explicit BlendState(const Rhi::BlendState& blendState) :
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
		[[nodiscard]] inline const Rhi::BlendState& getBlendState() const
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
		Rhi::BlendState mBlendState;		///< Blend state
		GLenum			mOpenGLSrcBlend;	///< OpenGL source blend function
		GLenum			mOpenGLDstBlend;	///< OpenGL destination blend function


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/RenderTarget/RenderPass.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL render pass interface
	*/
	class RenderPass final : public Rhi::IRenderPass
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rhi
		*    Owner RHI instance
		*  @param[in] numberOfColorAttachments
		*    Number of color render target textures, must be <="Rhi::Capabilities::maximumNumberOfSimultaneousRenderTargets"
		*  @param[in] colorAttachmentTextureFormats
		*    The color render target texture formats, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "numberOfColorAttachments" textures in the provided C-array of pointers
		*  @param[in] depthStencilAttachmentTextureFormat
		*    The optional depth stencil render target texture format, can be a "Rhi::TextureFormat::UNKNOWN" if there should be no depth buffer
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		RenderPass(Rhi::IRhi& rhi, uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRenderPass(rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat),
			mNumberOfMultisamples(numberOfMultisamples)
		{
			RHI_ASSERT(rhi.getContext(), mNumberOfColorAttachments < 8, "Invalid number of OpenGL color attachments")
			memcpy(mColorAttachmentTextureFormats, colorAttachmentTextureFormats, sizeof(Rhi::TextureFormat::Enum) * mNumberOfColorAttachments);
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
			return (mDepthStencilAttachmentTextureFormat != Rhi::TextureFormat::Enum::UNKNOWN) ? (mNumberOfColorAttachments + 1) : mNumberOfColorAttachments;
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
		[[nodiscard]] inline Rhi::TextureFormat::Enum getColorAttachmentTextureFormat(uint32_t colorAttachmentIndex) const
		{
			RHI_ASSERT(getRhi().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid OpenGL color attachment index")
			return mColorAttachmentTextureFormats[colorAttachmentIndex];
		}

		/**
		*  @brief
		*    Return the depth stencil attachment texture format
		*
		*  @return
		*    The depth stencil attachment texture format
		*/
		[[nodiscard]] inline Rhi::TextureFormat::Enum getDepthStencilAttachmentTextureFormat() const
		{
			return mDepthStencilAttachmentTextureFormat;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), RenderPass, this);
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
		uint32_t				 mNumberOfColorAttachments;
		Rhi::TextureFormat::Enum mColorAttachmentTextureFormats[8];
		Rhi::TextureFormat::Enum mDepthStencilAttachmentTextureFormat;
		uint8_t					 mNumberOfMultisamples;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/QueryPool.h                                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL asynchronous query pool base class
	*/
	class QueryPool : public Rhi::IQueryPool
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		inline QueryPool(OpenGLRhi& openGLRhi, Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IQueryPool(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
		[[nodiscard]] inline Rhi::QueryType getQueryType() const
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
		Rhi::QueryType mQueryType;
		uint32_t	   mNumberOfQueries;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/OcclusionTimestampQueryPool.h               ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		OcclusionTimestampQueryPool(OpenGLRhi& openGLRhi, Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			QueryPool(openGLRhi, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLQueries(RHI_MALLOC_TYPED(openGLRhi.getContext(), GLuint, numberOfQueries))
		{
			// If possible, use "glCreateQueries()" (OpenGL 4.5) in order to create the query instance at once
			if (nullptr != glCreateQueries)
			{
				switch (queryType)
				{
					case Rhi::QueryType::OCCLUSION:
						glCreateQueries(GL_SAMPLES_PASSED_ARB, static_cast<GLsizei>(numberOfQueries), mOpenGLQueries);
						break;

					case Rhi::QueryType::PIPELINE_STATISTICS:
						RHI_ASSERT(openGLRhi.getContext(), false, "Use \"OpenGLRhi::PipelineStatisticsQueryPool\"")
						break;

					case Rhi::QueryType::TIMESTAMP:
						glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(numberOfQueries), mOpenGLQueries);
						break;
				}
			}
			else
			{
				glGenQueriesARB(static_cast<GLsizei>(numberOfQueries), mOpenGLQueries);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					switch (queryType)
					{
						case Rhi::QueryType::OCCLUSION:
						{
							RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Occlusion query", 18)	// 18 = "Occlusion query: " including terminating zero
							for (uint32_t i = 0; i < mNumberOfQueries; ++i)
							{
								glObjectLabel(GL_QUERY, mOpenGLQueries[i], -1, detailedDebugName);
							}
							break;
						}

						case Rhi::QueryType::PIPELINE_STATISTICS:
							RHI_ASSERT(openGLRhi.getContext(), false, "Use \"OpenGLRhi::PipelineStatisticsQueryPool\"")
							break;

						case Rhi::QueryType::TIMESTAMP:
						{
							RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Timestamp query", 18)	// 18 = "Timestamp query: " including terminating zero
							for (uint32_t i = 0; i < mNumberOfQueries; ++i)
							{
								glObjectLabel(GL_QUERY, mOpenGLQueries[i], -1, detailedDebugName);
							}
							break;
						}
					}
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
			RHI_FREE(getRhi().getContext(), mOpenGLQueries);
		}

		/**
		*  @brief
		*    Return the OpenGL queries
		*
		*  @return
		*    The OpenGL queries
		*/
		[[nodiscard]] inline const GLuint* getOpenGLQueries() const
		{
			return mOpenGLQueries;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), OcclusionTimestampQueryPool, this);
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
	//[ OpenGLRhi/PipelineStatisticsQueryPool.h               ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		PipelineStatisticsQueryPool(OpenGLRhi& openGLRhi, Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			QueryPool(openGLRhi, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mVerticesSubmittedOpenGLQueries(RHI_MALLOC_TYPED(openGLRhi.getContext(), GLuint, numberOfQueries * 11)),
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
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					switch (queryType)
					{
						case Rhi::QueryType::OCCLUSION:
						case Rhi::QueryType::TIMESTAMP:
							RHI_ASSERT(openGLRhi.getContext(), false, "Use \"OpenGLRhi::OcclusionTimestampQueryPool\"")
							break;

						case Rhi::QueryType::PIPELINE_STATISTICS:
						{
							// Enforce instant query creation so we can set a debug name
							for (uint32_t i = 0; i < numberOfQueries; ++i)
							{
								beginQuery(i);
								endQuery();
							}

							// Set debug name
							RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Pipeline statistics query", 28)	// 28 = "Pipeline statistics query: " including terminating zero
							for (uint32_t i = 0; i < mNumberOfQueries * 11; ++i)
							{
								glObjectLabel(GL_QUERY, mVerticesSubmittedOpenGLQueries[i], -1, detailedDebugName);
							}
							break;
						}
					}
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
			RHI_FREE(getRhi().getContext(), mVerticesSubmittedOpenGLQueries);
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
			Rhi::PipelineStatisticsQueryResult* currentPipelineStatisticsQueryResult = reinterpret_cast<Rhi::PipelineStatisticsQueryResult*>(data);
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), PipelineStatisticsQueryPool, this);
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
		GLuint* mVerticesSubmittedOpenGLQueries;				///< "GL_VERTICES_SUBMITTED_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfInputAssemblerVertices"
		GLuint* mPrimitivesSubmittedOpenGLQueries;				///< "GL_PRIMITIVES_SUBMITTED_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfInputAssemblerPrimitives"
		GLuint* mVertexShaderInvocationsOpenGLQueries;			///< "GL_VERTEX_SHADER_INVOCATIONS_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfVertexShaderInvocations"
		GLuint* mGeometryShaderInvocationsOpenGLQueries;		///< "GL_GEOMETRY_SHADER_INVOCATIONS", "Rhi::PipelineStatisticsQueryResult::numberOfGeometryShaderInvocations"
		GLuint* mGeometryShaderPrimitivesEmittedOpenGLQueries;	///< "GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfGeometryShaderOutputPrimitives"
		GLuint* mClippingInputPrimitivesOpenGLQueries;			///< "GL_CLIPPING_INPUT_PRIMITIVES_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfClippingInputPrimitives"
		GLuint* mClippingOutputPrimitivesOpenGLQueries;			///< "GL_CLIPPING_OUTPUT_PRIMITIVES_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfClippingOutputPrimitives"
		GLuint* mFragmentShaderInvocationsOpenGLQueries;		///< "GL_FRAGMENT_SHADER_INVOCATIONS_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfFragmentShaderInvocations"
		GLuint* mTessControlShaderPatchesOpenGLQueries;			///< "GL_TESS_CONTROL_SHADER_PATCHES_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfTessellationControlShaderInvocations"
		GLuint* mTesEvaluationShaderInvocationsOpenGLQueries;	///< "GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfTessellationEvaluationShaderInvocations"
		GLuint* mComputeShaderInvocationsOpenGLQueries;			///< "GL_COMPUTE_SHADER_INVOCATIONS_ARB", "Rhi::PipelineStatisticsQueryResult::numberOfComputeShaderInvocations"


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/RenderTarget/SwapChain.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL swap chain class
	*/
	class SwapChain final : public Rhi::ISwapChain
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
		*    Indicates if an external RHI context is used; in this case the RHI itself has nothing to do with the creation/managing of an RHI context
		*/
		SwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, [[maybe_unused]] bool useExternalContext RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISwapChain(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNativeWindowHandle(windowHandle.nativeWindowHandle),
			#ifdef _WIN32
				mOpenGLContext(RHI_NEW(renderPass.getRhi().getContext(), OpenGLContextWindows)(static_cast<const RenderPass&>(renderPass).getDepthStencilAttachmentTextureFormat(), windowHandle.nativeWindowHandle, static_cast<const OpenGLContextWindows*>(&static_cast<OpenGLRhi&>(renderPass.getRhi()).getOpenGLContext()))),
			#elif defined LINUX
				mOpenGLContext(RHI_NEW(renderPass.getRhi().getContext(), OpenGLContextLinux)(static_cast<OpenGLRhi&>(renderPass.getRhi()), static_cast<const RenderPass&>(renderPass).getDepthStencilAttachmentTextureFormat(), windowHandle.nativeWindowHandle, useExternalContext, static_cast<const OpenGLContextLinux*>(&static_cast<OpenGLRhi&>(renderPass.getRhi()).getOpenGLContext()))),
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
				RHI_DELETE(getRhi().getContext(), IOpenGLContext, mOpenGLContext);
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
	//[ Public virtual Rhi::IRenderTarget methods             ]
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
					OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
					Display* display = static_cast<const OpenGLContextLinux&>(openGLRhi.getOpenGLContext()).getDisplay();

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
	//[ Public virtual Rhi::ISwapChain methods                ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Rhi::handle getNativeWindowHandle() const override
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
					const Extensions& extensions = static_cast<OpenGLRhi&>(getRhi()).getExtensions();
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
					glXSwapBuffers(static_cast<const OpenGLContextLinux&>(static_cast<OpenGLRhi&>(getRhi()).getOpenGLContext()).getDisplay(), mNativeWindowHandle);
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

		inline virtual void setRenderWindow(Rhi::IRenderWindow* renderWindow) override
		{
			mRenderWindow = renderWindow;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), SwapChain, this);
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
		Rhi::handle			mNativeWindowHandle;	///< Native window handle window, can be a null handle
		IOpenGLContext*		mOpenGLContext;			///< OpenGL context, must be valid
		bool				mOwnsOpenGLContext;		///< Does this swap chain own the OpenGL context?
		Rhi::IRenderWindow* mRenderWindow;			///< Render window instance, can be a null pointer, don't destroy the instance since we don't own it
		uint32_t			mVerticalSynchronizationInterval;
		uint32_t			mNewVerticalSynchronizationInterval;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/RenderTarget/Framebuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL framebuffer interface
	*/
	class Framebuffer : public Rhi::IFramebuffer
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
				Rhi::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Rhi::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture)
				{
					(*colorTexture)->releaseReference();
				}

				// Cleanup
				RHI_FREE(getRhi().getContext(), mColorTextures);
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
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLFramebuffer));
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRenderTarget methods             ]
	//[-------------------------------------------------------]
	public:
		inline virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// No fancy implementation in here, just copy over the internal information
			width  = mWidth;
			height = mHeight;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Framebuffer, this);
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
		*    least "Rhi::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		Framebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IFramebuffer(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
				mColorTextures = RHI_MALLOC_TYPED(renderPass.getRhi().getContext(), Rhi::ITexture*, mNumberOfColorTextures);

				// Loop through all color textures
				Rhi::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Rhi::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments)
				{
					// Sanity check
					RHI_ASSERT(renderPass.getRhi().getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid OpenGL color framebuffer attachment texture")

					// TODO(co) Add security check: Is the given resource one of the currently used RHI?
					*colorTexture = colorFramebufferAttachments->texture;
					(*colorTexture)->addReference();

					// Evaluate the color texture type
					switch ((*colorTexture)->getResourceType())
					{
						case Rhi::ResourceType::TEXTURE_2D:
						{
							const Texture2D* texture2D = static_cast<Texture2D*>(*colorTexture);

							// Sanity checks
							RHI_ASSERT(renderPass.getRhi().getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL color framebuffer attachment mipmap index")
							RHI_ASSERT(renderPass.getRhi().getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid OpenGL color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
							break;
						}

						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);
							break;
						}

						case Rhi::ResourceType::ROOT_SIGNATURE:
						case Rhi::ResourceType::RESOURCE_GROUP:
						case Rhi::ResourceType::GRAPHICS_PROGRAM:
						case Rhi::ResourceType::VERTEX_ARRAY:
						case Rhi::ResourceType::RENDER_PASS:
						case Rhi::ResourceType::QUERY_POOL:
						case Rhi::ResourceType::SWAP_CHAIN:
						case Rhi::ResourceType::FRAMEBUFFER:
						case Rhi::ResourceType::VERTEX_BUFFER:
						case Rhi::ResourceType::INDEX_BUFFER:
						case Rhi::ResourceType::TEXTURE_BUFFER:
						case Rhi::ResourceType::STRUCTURED_BUFFER:
						case Rhi::ResourceType::INDIRECT_BUFFER:
						case Rhi::ResourceType::UNIFORM_BUFFER:
						case Rhi::ResourceType::TEXTURE_1D:
						case Rhi::ResourceType::TEXTURE_1D_ARRAY:
						case Rhi::ResourceType::TEXTURE_3D:
						case Rhi::ResourceType::TEXTURE_CUBE:
						case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
						case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
						case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
						case Rhi::ResourceType::SAMPLER_STATE:
						case Rhi::ResourceType::VERTEX_SHADER:
						case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
						case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
						case Rhi::ResourceType::GEOMETRY_SHADER:
						case Rhi::ResourceType::FRAGMENT_SHADER:
						case Rhi::ResourceType::TASK_SHADER:
						case Rhi::ResourceType::MESH_SHADER:
						case Rhi::ResourceType::COMPUTE_SHADER:
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
				RHI_ASSERT(renderPass.getRhi().getContext(), nullptr != mDepthStencilTexture, "Invalid OpenGL depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(renderPass.getRhi().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(renderPass.getRhi().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
						break;
					}

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);
						break;
					}

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::SWAP_CHAIN:
					case Rhi::ResourceType::FRAMEBUFFER:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
					default:
						// Nothing here
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RHI_ASSERT(renderPass.getRhi().getContext(), false, "Invalid OpenGL framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RHI_ASSERT(renderPass.getRhi().getContext(), false, "Invalid OpenGL framebuffer height")
				mHeight = 1;
			}
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint			mOpenGLFramebuffer;			///< OpenGL framebuffer ("container" object, not shared between OpenGL contexts), can be zero if no resource is allocated
		uint32_t		mNumberOfColorTextures;		///< Number of color render target textures
		Rhi::ITexture** mColorTextures;				///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Rhi::ITexture*  mDepthStencilTexture;		///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t		mWidth;						///< The framebuffer width
		uint32_t		mHeight;					///< The framebuffer height
		bool			mMultisampleRenderTarget;	///< Multisample render target?


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Framebuffer(const Framebuffer& source) = delete;
		Framebuffer& operator =(const Framebuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/RenderTarget/FramebufferBind.h              ]
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
		*    least "Rhi::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		FramebufferBind(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Framebuffer(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Texture reference handling is done within the base class "Framebuffer"

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL framebuffer
				GLint openGLFramebufferBackup = 0;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &openGLFramebufferBackup);
			#endif

			// Create the OpenGL framebuffer
			glGenFramebuffers(1, &mOpenGLFramebuffer);

			// Bind this OpenGL framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, mOpenGLFramebuffer);

			// Loop through all framebuffer color attachments
			#ifdef RHI_DEBUG
				OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(renderPass.getRhi());
			#endif
			const Rhi::FramebufferAttachment* colorFramebufferAttachment    = colorFramebufferAttachments;
			const Rhi::FramebufferAttachment* colorFramebufferAttachmentEnd = colorFramebufferAttachments + mNumberOfColorTextures;
			for (GLenum openGLAttachment = GL_COLOR_ATTACHMENT0; colorFramebufferAttachment < colorFramebufferAttachmentEnd; ++colorFramebufferAttachment, ++openGLAttachment)
			{
				Rhi::ITexture* texture = colorFramebufferAttachment->texture;

				// Sanity check: Is the given resource owned by this RHI?
				RHI_ASSERT(openGLRhi.getContext(), &openGLRhi == &texture->getRhi(), "OpenGL error: The given color texture at index %u is owned by another RHI instance", colorFramebufferAttachment - colorFramebufferAttachments)

				// Evaluate the color texture type
				switch (texture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
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

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
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

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::SWAP_CHAIN:
					case Rhi::ResourceType::FRAMEBUFFER:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
					default:
						RHI_ASSERT(openGLRhi.getContext(), false, "The type of the given color texture at index %ld is not supported by the OpenGL RHI implementation", colorFramebufferAttachment - colorFramebufferAttachments)
						break;
				}
			}

			// Depth stencil texture
			if (nullptr != mDepthStencilTexture)
			{
				// Sanity check: Is the given resource owned by this RHI?
				RHI_ASSERT(openGLRhi.getContext(), &openGLRhi == &mDepthStencilTexture->getRhi(), "OpenGL error: The given depth stencil texture is owned by another RHI instance")

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<const Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(openGLRhi.getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(openGLRhi.getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL depth stencil framebuffer attachment layer index")

						// Bind the depth stencil texture to framebuffer
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex));
						if (!mMultisampleRenderTarget && texture2D->getNumberOfMultisamples() > 1)
						{
							mMultisampleRenderTarget = true;
						}
						break;
					}

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
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

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::SWAP_CHAIN:
					case Rhi::ResourceType::FRAMEBUFFER:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
					default:
						RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: The type of the given depth stencil texture is not supported by the OpenGL RHI implementation")
						break;
				}
			}

			// Check the status of the OpenGL framebuffer
			const GLenum openGLStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			switch (openGLStatus)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Not all framebuffer attachment points are framebuffer attachment complete (\"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: No images are attached to the framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete draw buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete read buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete multisample framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\")")
					break;

				case GL_FRAMEBUFFER_UNDEFINED:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Undefined framebuffer (\"GL_FRAMEBUFFER_UNDEFINED\")")
					break;

				case GL_FRAMEBUFFER_UNSUPPORTED:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: The combination of internal formats of the attached images violates an implementation-dependent set of restrictions (\"GL_FRAMEBUFFER_UNSUPPORTED\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Not all attached images have the same width and height (\"GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete formats framebuffer object (\"GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\")")
					break;

				default:
				case GL_FRAMEBUFFER_COMPLETE:
					// Nothing here
					break;
			}

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(openGLFramebufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FBO", 6)	// 6 = "FBO: " including terminating zero
					glObjectLabel(GL_FRAMEBUFFER, mOpenGLFramebuffer, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FramebufferBind() override
		{
			// Nothing here, texture reference handling is done within the base class "Framebuffer"
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FramebufferBind(const FramebufferBind& source) = delete;
		FramebufferBind& operator =(const FramebufferBind& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/RenderTarget/FramebufferDsa.h               ]
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
		*    least "Rhi::IRenderPass::getNumberOfColorAttachments()" textures in the provided C-array of pointers
		*  @param[in] depthStencilFramebufferAttachment
		*    The depth stencil render target texture, can be a null pointer
		*
		*  @note
		*    - The framebuffer keeps a reference to the provided texture instances
		*/
		FramebufferDsa(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Framebuffer(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// Texture reference handling is done within the base class "Framebuffer"
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(renderPass.getRhi());
			const bool isArbDsa = openGLRhi.getExtensions().isGL_ARB_direct_state_access();

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
			const Rhi::FramebufferAttachment* colorFramebufferAttachment    = colorFramebufferAttachments;
			const Rhi::FramebufferAttachment* colorFramebufferAttachmentEnd = colorFramebufferAttachments + mNumberOfColorTextures;
			for (GLenum openGLAttachment = GL_COLOR_ATTACHMENT0; colorFramebufferAttachment < colorFramebufferAttachmentEnd; ++colorFramebufferAttachment, ++openGLAttachment)
			{
				Rhi::ITexture* texture = colorFramebufferAttachment->texture;

				// Sanity check: Is the given resource owned by this RHI?
				RHI_ASSERT(openGLRhi.getContext(), &openGLRhi == &texture->getRhi(), "OpenGL error: The given color texture at index %u is owned by another RHI instance", colorFramebufferAttachment - colorFramebufferAttachments)

				// Evaluate the color texture type
				switch (texture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
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

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
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

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::SWAP_CHAIN:
					case Rhi::ResourceType::FRAMEBUFFER:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
					default:
						RHI_ASSERT(openGLRhi.getContext(), false, "The type of the given color texture at index %ld is not supported by the OpenGL RHI implementation", colorFramebufferAttachment - colorFramebufferAttachments)
						break;
				}
			}

			// Depth stencil texture
			if (nullptr != mDepthStencilTexture)
			{
				// Sanity check: Is the given resource owned by this RHI?
				RHI_ASSERT(openGLRhi.getContext(), &openGLRhi == &mDepthStencilTexture->getRhi(), "OpenGL error: The given depth stencil texture is owned by another RHI instance")

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<const Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(openGLRhi.getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(openGLRhi.getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL depth stencil framebuffer attachment layer index")

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

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
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

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::SWAP_CHAIN:
					case Rhi::ResourceType::FRAMEBUFFER:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
					default:
						RHI_ASSERT(openGLRhi.getContext(), false, "The type of the given depth stencil texture is not supported by the OpenGL RHI implementation")
						break;
				}
			}

			// Check the status of the OpenGL framebuffer
			const GLenum openGLStatus = isArbDsa ? glCheckNamedFramebufferStatus(mOpenGLFramebuffer, GL_FRAMEBUFFER) : glCheckNamedFramebufferStatusEXT(mOpenGLFramebuffer, GL_FRAMEBUFFER);
			switch (openGLStatus)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Not all framebuffer attachment points are framebuffer attachment complete (\"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: No images are attached to the framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete draw buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete read buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete multisample framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\")")
					break;

				case GL_FRAMEBUFFER_UNDEFINED:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Undefined framebuffer (\"GL_FRAMEBUFFER_UNDEFINED\")")
					break;

				case GL_FRAMEBUFFER_UNSUPPORTED:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: The combination of internal formats of the attached images violates an implementation-dependent set of restrictions (\"GL_FRAMEBUFFER_UNSUPPORTED\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Not all attached images have the same width and height (\"GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT\")")
					break;

				// From "GL_EXT_framebuffer_object" (should no longer matter, should)
				case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
					RHI_ASSERT(openGLRhi.getContext(), false, "OpenGL error: Incomplete formats framebuffer object (\"GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\")")
					break;

				default:
				case GL_FRAMEBUFFER_COMPLETE:
					// Nothing here
					break;
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FBO", 6)	// 6 = "FBO: " including terminating zero
					glObjectLabel(GL_FRAMEBUFFER, mOpenGLFramebuffer, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FramebufferDsa() override
		{
			// Nothing here, texture reference handling is done within the base class "Framebuffer"
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FramebufferDsa(const FramebufferDsa& source) = delete;
		FramebufferDsa& operator =(const FramebufferDsa& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Monolithic/VertexShaderMonolithic.h  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic vertex shader class
	*/
	class VertexShaderMonolithic final : public Rhi::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline VertexShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_VERTEX_SHADER_ARB, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5)	// 5 = "VS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexShaderMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/TessellationControlShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderMonolithic final : public Rhi::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationControlShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_TESS_CONTROL_SHADER, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6)	// 6 = "TCS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationControlShaderMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/TessellationEvaluationShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderMonolithic final : public Rhi::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationEvaluationShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_TESS_EVALUATION_SHADER, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6)	// 6 = "TES: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationEvaluationShaderMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/GeometryShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic geometry shader class
	*/
	class GeometryShaderMonolithic final : public Rhi::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		inline GeometryShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_GEOMETRY_SHADER_ARB, sourceCode)),
			mOpenGLGsInputPrimitiveTopology(static_cast<int>(gsInputPrimitiveTopology)),	// The "Rhi::GsInputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			mOpenGLGsOutputPrimitiveTopology(static_cast<int>(gsOutputPrimitiveTopology)),	// The "Rhi::GsOutputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			mNumberOfOutputVertices(numberOfOutputVertices)
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5)	// 5 = "GS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GeometryShaderMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/FragmentShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic fragment shader (FS, "pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderMonolithic final : public Rhi::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline FragmentShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_FRAGMENT_SHADER_ARB, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5)	// 5 = "FS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), FragmentShaderMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/TaskShaderMonolithic.h    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic task shader (TS, "amplification shader" in Direct3D terminology) class
	*/
	class TaskShaderMonolithic final : public Rhi::ITaskShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a task shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TaskShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITaskShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_TASK_SHADER_NV, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TS", 5)	// 5 = "TS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TaskShaderMonolithic() override
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TaskShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TaskShaderMonolithic(const TaskShaderMonolithic& source) = delete;
		TaskShaderMonolithic& operator =(const TaskShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Monolithic/MeshShaderMonolithic.h    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic mesh shader (MS) class
	*/
	class MeshShaderMonolithic final : public Rhi::IMeshShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a mesh shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline MeshShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IMeshShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_MESH_SHADER_NV, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "MS", 5)	// 5 = "MS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~MeshShaderMonolithic() override
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), MeshShaderMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MeshShaderMonolithic(const MeshShaderMonolithic& source) = delete;
		MeshShaderMonolithic& operator =(const MeshShaderMonolithic& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShader;	///< OpenGL shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Monolithic/ComputeShaderMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic compute shader (CS) class
	*/
	class ComputeShaderMonolithic final : public Rhi::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline ComputeShaderMonolithic(OpenGLRhi& openGLRhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShader(::detail::loadShaderFromSourcecode(openGLRhi.getContext(), GL_COMPUTE_SHADER, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShader && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5)	// 5 = "CS: " including terminating zero
					glObjectLabel(GL_SHADER, mOpenGLShader, -1, detailedDebugName);
				}
			#endif
		}

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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputeShaderMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/GraphicsProgramMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic graphics program class
	*/
	class GraphicsProgramMonolithic : public Rhi::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for traditional graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		GraphicsProgramMonolithic(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, VertexShaderMonolithic* vertexShaderMonolithic, TessellationControlShaderMonolithic* tessellationControlShaderMonolithic, TessellationEvaluationShaderMonolithic* tessellationEvaluationShaderMonolithic, GeometryShaderMonolithic* geometryShaderMonolithic, FragmentShaderMonolithic* fragmentShaderMonolithic RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsProgram(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
			linkProgram(openGLRhi, rootSignature);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics program", 19)	// 19 = "Graphics program: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for task and mesh shader based graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] taskShaderMonolithic
		*    Task shader the graphics program is using, can be a null pointer
		*  @param[in] meshShaderMonolithic
		*    Mesh shader the graphics program is using
		*  @param[in] fragmentShaderMonolithic
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramMonolithic(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, TaskShaderMonolithic* taskShaderMonolithic, MeshShaderMonolithic& meshShaderMonolithic, FragmentShaderMonolithic* fragmentShaderMonolithic RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsProgram(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLProgram(glCreateProgram()),
			mDrawIdUniformLocation(-1)
		{
			// Attach the shaders to the program
			// -> We don't need to keep a reference to the shader, to add and release at once to ensure a nice behaviour
			if (nullptr != taskShaderMonolithic)
			{
				taskShaderMonolithic->addReference();
				glAttachShader(mOpenGLProgram, taskShaderMonolithic->getOpenGLShader());
				taskShaderMonolithic->releaseReference();
			}
			meshShaderMonolithic.addReference();
			glAttachShader(mOpenGLProgram, meshShaderMonolithic.getOpenGLShader());
			meshShaderMonolithic.releaseReference();
			if (nullptr != fragmentShaderMonolithic)
			{
				fragmentShaderMonolithic->addReference();
				glAttachShader(mOpenGLProgram, fragmentShaderMonolithic->getOpenGLShader());
				fragmentShaderMonolithic->releaseReference();
			}

			// Link the program
			linkProgram(openGLRhi, rootSignature);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics program", 19)	// 19 = "Graphics program: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IGraphicsProgram methods          ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Rhi::handle getUniformHandle(const char* uniformName) override
		{
			return static_cast<Rhi::handle>(glGetUniformLocation(mOpenGLProgram, uniformName));
		}

		virtual void setUniform1i(Rhi::handle uniformHandle, int value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform1f(Rhi::handle uniformHandle, float value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform2fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform3fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform4fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniformMatrix3fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniformMatrix4fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsProgramMonolithic, this);
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

		void linkProgram(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature)
		{
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
				if (!openGLRhi.getExtensions().isGL_ARB_base_instance())
				{
					mDrawIdUniformLocation = glGetUniformLocation(mOpenGLProgram, "drawIdUniform");
				}

				// The actual locations assigned to uniform variables are not known until the program object is linked successfully
				// -> So we have to build a root signature parameter index -> uniform location mapping here
				const Rhi::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Rhi::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_ASSERT(openGLRhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
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
							else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
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
										#ifdef RHI_OPENGL_STATE_CLEANUP
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
					const Rhi::Context& context = openGLRhi.getContext();
					char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramInfoLog(mOpenGLProgram, informationLength, nullptr, informationLog);

					// Output the debug string
					RHI_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLint mDrawIdUniformLocation;	///< Draw ID uniform location, used for "GL_ARB_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Monolithic/GraphicsProgramMonolithicDsa.h ]
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
		*    Constructor for traditional graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		inline GraphicsProgramMonolithicDsa(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, VertexShaderMonolithic* vertexShaderMonolithic, TessellationControlShaderMonolithic* tessellationControlShaderMonolithic, TessellationEvaluationShaderMonolithic* tessellationEvaluationShaderMonolithic, GeometryShaderMonolithic* geometryShaderMonolithic, FragmentShaderMonolithic* fragmentShaderMonolithic RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			GraphicsProgramMonolithic(openGLRhi, rootSignature, vertexAttributes, vertexShaderMonolithic, tessellationControlShaderMonolithic, tessellationEvaluationShaderMonolithic, geometryShaderMonolithic, fragmentShaderMonolithic RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Constructor for task and mesh shader based graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] taskShaderMonolithic
		*    Task shader the graphics program is using, can be a null pointer
		*  @param[in] meshShaderMonolithic
		*    Mesh shader the graphics program is using
		*  @param[in] fragmentShaderMonolithic
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		inline GraphicsProgramMonolithicDsa(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, TaskShaderMonolithic* taskShaderMonolithic, MeshShaderMonolithic& meshShaderMonolithic, FragmentShaderMonolithic* fragmentShaderMonolithic RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			GraphicsProgramMonolithic(openGLRhi, rootSignature, taskShaderMonolithic, meshShaderMonolithic, fragmentShaderMonolithic RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GraphicsProgramMonolithicDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IGraphicsProgram methods          ]
	//[-------------------------------------------------------]
	public:
		virtual void setUniform1f(Rhi::handle uniformHandle, float value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform1f(mOpenGLProgram, static_cast<GLint>(uniformHandle), value);
			}
			else
			{
				glProgramUniform1fEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), value);
			}
		}

		virtual void setUniform2fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform2fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform2fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform3fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform3fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform3fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform4fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform4fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform4fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniformMatrix3fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniformMatrix3fv(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
			else
			{
				glProgramUniformMatrix3fvEXT(mOpenGLProgram, static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
		}

		virtual void setUniformMatrix4fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
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
	//[ OpenGLRhi/Shader/ComputePipelineState.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract compute pipeline state base class
	*/
	class ComputePipelineState : public Rhi::IComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		inline explicit ComputePipelineState(OpenGLRhi& openGLRhi, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IComputePipelineState(openGLRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ComputePipelineState() override
		{
			// Free the unique compact compute pipeline state ID
			static_cast<OpenGLRhi&>(getRhi()).ComputePipelineStateMakeId.DestroyID(getId());
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineState(const ComputePipelineState& source) = delete;
		ComputePipelineState& operator =(const ComputePipelineState& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Monolithic/ComputePipelineStateMonolithic.h ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		ComputePipelineStateMonolithic(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, ComputeShaderMonolithic& computeShaderMonolithic, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ComputePipelineState(openGLRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
				const Rhi::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Rhi::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_ASSERT(openGLRhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
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
							else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
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
										#ifdef RHI_OPENGL_STATE_CLEANUP
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
					const Rhi::Context& context = openGLRhi.getContext();
					char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramInfoLog(mOpenGLProgram, informationLength, nullptr, informationLog);

					// Output the debug string
					RHI_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Compute PSO", 14)	// 14 = "Compute PSO: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputePipelineStateMonolithic, this);
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
	//[ OpenGLRhi/Shader/Monolithic/ShaderLanguageMonolithic.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Monolithic shader language class
	*/
	class ShaderLanguageMonolithic final : public Rhi::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit ShaderLanguageMonolithic(OpenGLRhi& openGLRhi) :
			IShaderLanguage(openGLRhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageMonolithic() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShaderLanguage methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromBytecode(const Rhi::VertexAttributes&, const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's vertex shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_ARB_vertex_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), VertexShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no vertex shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's tessellation support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_ARB_tessellation_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), TessellationControlShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's tessellation support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_ARB_tessellation_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), TessellationEvaluationShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode(const Rhi::ShaderBytecode&, Rhi::GsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology, uint32_t RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's geometry shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_ARB_geometry_shader4())
			{
				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
				return RHI_NEW(openGLRhi.getContext(), GeometryShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no geometry shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's fragment shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_ARB_fragment_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), FragmentShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no fragment shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::ITaskShader* createTaskShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's mesh shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_NV_mesh_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), TaskShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no task shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IMeshShader* createMeshShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's mesh shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_NV_mesh_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), MeshShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no mesh shader support!
				return nullptr;
			}
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IComputeShader* createComputeShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's compute shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			if (openGLRhi.getExtensions().isGL_ARB_compute_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), ComputeShaderMonolithic)(openGLRhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no compute shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram(const Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, Rhi::ITessellationControlShader* tessellationControlShader, Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(openGLRhi.getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL vertex shader language mismatch")
			RHI_ASSERT(openGLRhi.getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL tessellation control shader language mismatch")
			RHI_ASSERT(openGLRhi.getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL tessellation evaluation shader language mismatch")
			RHI_ASSERT(openGLRhi.getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL geometry shader language mismatch")
			RHI_ASSERT(openGLRhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL fragment shader language mismatch")

			// Create the graphics program: Is "GL_EXT_direct_state_access" there?
			if (openGLRhi.getExtensions().isGL_EXT_direct_state_access() || openGLRhi.getExtensions().isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramMonolithicDsa)(openGLRhi, rootSignature, vertexAttributes, static_cast<VertexShaderMonolithic*>(vertexShader), static_cast<TessellationControlShaderMonolithic*>(tessellationControlShader), static_cast<TessellationEvaluationShaderMonolithic*>(tessellationEvaluationShader), static_cast<GeometryShaderMonolithic*>(geometryShader), static_cast<FragmentShaderMonolithic*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramMonolithic)(openGLRhi, rootSignature, vertexAttributes, static_cast<VertexShaderMonolithic*>(vertexShader), static_cast<TessellationControlShaderMonolithic*>(tessellationControlShader), static_cast<TessellationEvaluationShaderMonolithic*>(tessellationEvaluationShader), static_cast<GeometryShaderMonolithic*>(geometryShader), static_cast<FragmentShaderMonolithic*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram(const Rhi::IRootSignature& rootSignature, Rhi::ITaskShader* taskShader, Rhi::IMeshShader& meshShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER)
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(openGLRhi.getContext(), nullptr == taskShader || taskShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL task shader language mismatch")
			RHI_ASSERT(openGLRhi.getContext(), meshShader.getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL mesh shader language mismatch")
			RHI_ASSERT(openGLRhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::GLSL_NAME, "OpenGL fragment shader language mismatch")

			// Create the graphics program: Is "GL_EXT_direct_state_access" there?
			if (openGLRhi.getExtensions().isGL_EXT_direct_state_access() || openGLRhi.getExtensions().isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramMonolithicDsa)(openGLRhi, rootSignature, static_cast<TaskShaderMonolithic*>(taskShader), static_cast<MeshShaderMonolithic&>(meshShader), static_cast<FragmentShaderMonolithic*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramMonolithic)(openGLRhi, rootSignature, static_cast<TaskShaderMonolithic*>(taskShader), static_cast<MeshShaderMonolithic&>(meshShader), static_cast<FragmentShaderMonolithic*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ShaderLanguageMonolithic, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageMonolithic(const ShaderLanguageMonolithic& source) = delete;
		ShaderLanguageMonolithic& operator =(const ShaderLanguageMonolithic& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Separate/VertexShaderSeparate.h      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate vertex shader class
	*/
	class VertexShaderSeparate final : public Rhi::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline VertexShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), vertexAttributes, GL_VERTEX_SHADER_ARB, shaderBytecode)),
			mDrawIdUniformLocation(openGLRhi.getExtensions().isGL_ARB_base_instance() ? -1 : glGetUniformLocation(mOpenGLShaderProgram, "drawIdUniform"))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5)	// 5 = "VS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline VertexShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::VertexAttributes& vertexAttributes, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourcecode(openGLRhi.getContext(), vertexAttributes, GL_VERTEX_SHADER_ARB, sourceCode)),
			mDrawIdUniformLocation(openGLRhi.getExtensions().isGL_ARB_base_instance() ? -1 : glGetUniformLocation(mOpenGLShaderProgram, "drawIdUniform"))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_VERTEX_SHADER_ARB, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5)	// 5 = "VS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexShaderSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/TessellationControlShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderSeparate final : public Rhi::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline TessellationControlShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_TESS_CONTROL_SHADER, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6)	// 6 = "TCS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationControlShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_TESS_CONTROL_SHADER, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_TESS_CONTROL_SHADER, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6)	// 6 = "TCS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationControlShaderSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/TessellationEvaluationShaderSeparate.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderSeparate final : public Rhi::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline TessellationEvaluationShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_TESS_EVALUATION_SHADER, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6)	// 6 = "TES: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TessellationEvaluationShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_TESS_EVALUATION_SHADER, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_TESS_EVALUATION_SHADER, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6)	// 6 = "TES: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationEvaluationShaderSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/GeometryShaderSeparate.h    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate geometry shader class
	*/
	class GeometryShaderSeparate final : public Rhi::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		inline GeometryShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_GEOMETRY_SHADER_ARB, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5)	// 5 = "GS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		inline GeometryShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_GEOMETRY_SHADER_ARB, sourceCode))
		{
			// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
			//   "layout(triangles) in;"
			//   "layout(triangle_strip, max_vertices = 3) out;"
			// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
			glProgramParameteriARB(mOpenGLShaderProgram, GL_GEOMETRY_INPUT_TYPE_ARB, static_cast<int>(gsInputPrimitiveTopology));	// The "Rhi::GsInputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			glProgramParameteriARB(mOpenGLShaderProgram, GL_GEOMETRY_OUTPUT_TYPE_ARB, static_cast<int>(gsOutputPrimitiveTopology));	// The "Rhi::GsOutputPrimitiveTopology" values directly map to OpenGL constants, do not change them
			glProgramParameteriARB(mOpenGLShaderProgram, GL_GEOMETRY_VERTICES_OUT_ARB, static_cast<GLint>(numberOfOutputVertices));

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_GEOMETRY_SHADER_ARB, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5)	// 5 = "GS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GeometryShaderSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/FragmentShaderSeparate.h    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate fragment shader (FS, "pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderSeparate final : public Rhi::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline FragmentShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_FRAGMENT_SHADER_ARB, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5)	// 5 = "FS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline FragmentShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_FRAGMENT_SHADER_ARB, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_FRAGMENT_SHADER_ARB, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5)	// 5 = "FS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), FragmentShaderSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/TaskShaderSeparate.h        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate task shader (TS, "amplification shader" in Direct3D terminology) class
	*/
	class TaskShaderSeparate final : public Rhi::ITaskShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a task shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline TaskShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITaskShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_TASK_SHADER_NV, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TS", 5)	// 5 = "TS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a task shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline TaskShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITaskShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_TASK_SHADER_NV, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_TASK_SHADER_NV, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TS", 5)	// 5 = "TS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TaskShaderSeparate() override
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TaskShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TaskShaderSeparate(const TaskShaderSeparate& source) = delete;
		TaskShaderSeparate& operator =(const TaskShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Separate/MeshShaderSeparate.h        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate mesh shader (MS) class
	*/
	class MeshShaderSeparate final : public Rhi::IMeshShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a mesh shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline MeshShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IMeshShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_MESH_SHADER_NV, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "MS", 5)	// 5 = "MS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a mesh shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline MeshShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IMeshShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_MESH_SHADER_NV, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_MESH_SHADER_NV, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "MS", 5)	// 5 = "MS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~MeshShaderSeparate() override
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), MeshShaderSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MeshShaderSeparate(const MeshShaderSeparate& source) = delete;
		MeshShaderSeparate& operator =(const MeshShaderSeparate& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLShaderProgram;	///< OpenGL shader program, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Separate/ComputeShaderSeparate.h     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate compute shader (CS) class
	*/
	class ComputeShaderSeparate final : public Rhi::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader bytecode
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		inline ComputeShaderSeparate(OpenGLRhi& openGLRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromBytecode(openGLRhi.getContext(), GL_COMPUTE_SHADER, shaderBytecode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5)	// 5 = "CS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		inline ComputeShaderSeparate(OpenGLRhi& openGLRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLShaderProgram(::detail::loadShaderProgramFromSourceCode(openGLRhi.getContext(), GL_COMPUTE_SHADER, sourceCode))
		{
			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				::detail::shaderSourceCodeToShaderBytecode(openGLRhi.getContext(), GL_COMPUTE_SHADER, sourceCode, *shaderBytecode);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLShaderProgram && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5)	// 5 = "CS: " including terminating zero
					glObjectLabel(GL_PROGRAM, mOpenGLShaderProgram, -1, detailedDebugName);
				}
			#endif
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
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputeShaderSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/GraphicsProgramSeparate.h   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate graphics program class
	*/
	class GraphicsProgramSeparate : public Rhi::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for traditional graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		GraphicsProgramSeparate(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, VertexShaderSeparate* vertexShaderSeparate, TessellationControlShaderSeparate* tessellationControlShaderSeparate, TessellationEvaluationShaderSeparate* tessellationEvaluationShaderSeparate, GeometryShaderSeparate* geometryShaderSeparate, FragmentShaderSeparate* fragmentShaderSeparate RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsProgram(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLProgramPipeline(0),
			// Traditional graphics program
			mVertexShaderSeparate(vertexShaderSeparate),
			mTessellationControlShaderSeparate(tessellationControlShaderSeparate),
			mTessellationEvaluationShaderSeparate(tessellationEvaluationShaderSeparate),
			mGeometryShaderSeparate(geometryShaderSeparate),
			// Both graphics programs
			mFragmentShaderSeparate(fragmentShaderSeparate),
			// Task and mesh shader based graphics program
			mTaskShaderSeparate(nullptr),
			mMeshShaderSeparate(nullptr)
		{
			// Create the OpenGL program pipeline
			glGenProgramPipelines(1, &mOpenGLProgramPipeline);

			// If the "GL_ARB_direct_state_access" nor "GL_EXT_direct_state_access" extension is available, we need to change OpenGL states during resource creation (nasty thing)
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
			#endif
			if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
			{
				#ifdef RHI_OPENGL_STATE_CLEANUP
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
				const Rhi::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Rhi::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_ASSERT(openGLRhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								#define BIND_UNIFORM_BLOCK(ShaderSeparate) if (nullptr != ShaderSeparate) ::detail::bindUniformBlock(descriptorRange, ShaderSeparate->getOpenGLShaderProgram(), uniformBlockBindingIndex);
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::ALL_GRAPHICS:
										BIND_UNIFORM_BLOCK(mVertexShaderSeparate)
										BIND_UNIFORM_BLOCK(mTessellationControlShaderSeparate)
										BIND_UNIFORM_BLOCK(mTessellationEvaluationShaderSeparate)
										BIND_UNIFORM_BLOCK(mGeometryShaderSeparate)
										BIND_UNIFORM_BLOCK(mFragmentShaderSeparate)
										break;

									case Rhi::ShaderVisibility::VERTEX:
										BIND_UNIFORM_BLOCK(mVertexShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
										BIND_UNIFORM_BLOCK(mTessellationControlShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
										BIND_UNIFORM_BLOCK(mTessellationEvaluationShaderSeparate)
										break;

									case Rhi::ShaderVisibility::GEOMETRY:
										BIND_UNIFORM_BLOCK(mGeometryShaderSeparate)
										break;

									case Rhi::ShaderVisibility::FRAGMENT:
										BIND_UNIFORM_BLOCK(mFragmentShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TASK:
									case Rhi::ShaderVisibility::MESH:
									case Rhi::ShaderVisibility::COMPUTE:
										RHI_ASSERT(openGLRhi.getContext(), false, "Invalid OpenGL shader visibility")
										break;
								}
								#undef BIND_UNIFORM_BLOCK
								++uniformBlockBindingIndex;
							}
							else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								#define BIND_UNIFORM_LOCATION(ShaderSeparate) if (nullptr != ShaderSeparate) ::detail::bindUniformLocation(descriptorRange, mOpenGLProgramPipeline, ShaderSeparate->getOpenGLShaderProgram());
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::ALL_GRAPHICS:
										BIND_UNIFORM_LOCATION(mVertexShaderSeparate)
										BIND_UNIFORM_LOCATION(mTessellationControlShaderSeparate)
										BIND_UNIFORM_LOCATION(mTessellationEvaluationShaderSeparate)
										BIND_UNIFORM_LOCATION(mGeometryShaderSeparate)
										BIND_UNIFORM_LOCATION(mFragmentShaderSeparate)
										break;

									case Rhi::ShaderVisibility::VERTEX:
										BIND_UNIFORM_LOCATION(mVertexShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
										BIND_UNIFORM_LOCATION(mTessellationControlShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
										BIND_UNIFORM_LOCATION(mTessellationEvaluationShaderSeparate)
										break;

									case Rhi::ShaderVisibility::GEOMETRY:
										BIND_UNIFORM_LOCATION(mGeometryShaderSeparate)
										break;

									case Rhi::ShaderVisibility::FRAGMENT:
										BIND_UNIFORM_LOCATION(mFragmentShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TASK:
									case Rhi::ShaderVisibility::MESH:
									case Rhi::ShaderVisibility::COMPUTE:
										RHI_ASSERT(openGLRhi.getContext(), false, "Invalid OpenGL shader visibility")
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
					const Rhi::Context& context = openGLRhi.getContext();
					char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramPipelineInfoLog(mOpenGLProgramPipeline, informationLength, nullptr, informationLog);

					// Output the debug string
					RHI_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous used OpenGL program pipeline
				if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
				{
					glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
				}
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLProgramPipeline && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics program", 19)	// 19 = "Graphics program: " including terminating zero
					glObjectLabel(GL_PROGRAM_PIPELINE, mOpenGLProgramPipeline, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for task and mesh shader based graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] taskShaderSeparate
		*    Task shader the graphics program is using, can be a null pointer
		*  @param[in] meshShaderSeparate
		*    Mesh shader the graphics program is using
		*  @param[in] fragmentShaderSeparate
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramSeparate(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, TaskShaderSeparate* taskShaderSeparate, MeshShaderSeparate& meshShaderSeparate, FragmentShaderSeparate* fragmentShaderSeparate RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsProgram(openGLRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLProgramPipeline(0),
			// Traditional graphics program
			mVertexShaderSeparate(nullptr),
			mTessellationControlShaderSeparate(nullptr),
			mTessellationEvaluationShaderSeparate(nullptr),
			mGeometryShaderSeparate(nullptr),
			// Both graphics programs
			mFragmentShaderSeparate(fragmentShaderSeparate),
			// Task and mesh shader based graphics program
			mTaskShaderSeparate(taskShaderSeparate),
			mMeshShaderSeparate(&meshShaderSeparate)
		{
			// Create the OpenGL program pipeline
			glGenProgramPipelines(1, &mOpenGLProgramPipeline);

			// If the "GL_ARB_direct_state_access" nor "GL_EXT_direct_state_access" extension is available, we need to change OpenGL states during resource creation (nasty thing)
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
			#endif
			if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
			{
				#ifdef RHI_OPENGL_STATE_CLEANUP
					glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &openGLProgramPipelineBackup);
				#endif
				glBindProgramPipeline(mOpenGLProgramPipeline);
			}

			// Add references to the provided shaders
			#define USE_PROGRAM_STAGES(ShaderBit, ShaderSeparate) if (nullptr != ShaderSeparate) { ShaderSeparate->addReference(); glUseProgramStages(mOpenGLProgramPipeline, ShaderBit, ShaderSeparate->getOpenGLShaderProgram()); }
			USE_PROGRAM_STAGES(GL_TASK_SHADER_BIT_NV,  mTaskShaderSeparate)
			USE_PROGRAM_STAGES(GL_MESH_SHADER_BIT_NV,  mMeshShaderSeparate)
			USE_PROGRAM_STAGES(GL_FRAGMENT_SHADER_BIT, mFragmentShaderSeparate)
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
				const Rhi::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Rhi::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_ASSERT(openGLRhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								#define BIND_UNIFORM_BLOCK(ShaderSeparate) if (nullptr != ShaderSeparate) ::detail::bindUniformBlock(descriptorRange, ShaderSeparate->getOpenGLShaderProgram(), uniformBlockBindingIndex);
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::ALL_GRAPHICS:
										BIND_UNIFORM_BLOCK(mTaskShaderSeparate)
										BIND_UNIFORM_BLOCK(mMeshShaderSeparate)
										break;

									case Rhi::ShaderVisibility::FRAGMENT:
										BIND_UNIFORM_BLOCK(mFragmentShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TASK:
										BIND_UNIFORM_BLOCK(mTaskShaderSeparate)
										break;

									case Rhi::ShaderVisibility::MESH:
										BIND_UNIFORM_BLOCK(mMeshShaderSeparate)
										break;

									case Rhi::ShaderVisibility::VERTEX:
									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
									case Rhi::ShaderVisibility::GEOMETRY:
									case Rhi::ShaderVisibility::COMPUTE:
										RHI_ASSERT(openGLRhi.getContext(), false, "Invalid OpenGL shader visibility")
										break;
								}
								#undef BIND_UNIFORM_BLOCK
								++uniformBlockBindingIndex;
							}
							else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								#define BIND_UNIFORM_LOCATION(ShaderSeparate) if (nullptr != ShaderSeparate) ::detail::bindUniformLocation(descriptorRange, mOpenGLProgramPipeline, ShaderSeparate->getOpenGLShaderProgram());
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::ALL_GRAPHICS:
										BIND_UNIFORM_LOCATION(mTaskShaderSeparate)
										BIND_UNIFORM_LOCATION(mMeshShaderSeparate)
										break;

									case Rhi::ShaderVisibility::FRAGMENT:
										BIND_UNIFORM_LOCATION(mFragmentShaderSeparate)
										break;

									case Rhi::ShaderVisibility::TASK:
										BIND_UNIFORM_LOCATION(mTaskShaderSeparate)
										break;

									case Rhi::ShaderVisibility::MESH:
										BIND_UNIFORM_LOCATION(mMeshShaderSeparate)
										break;

									case Rhi::ShaderVisibility::VERTEX:
									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
									case Rhi::ShaderVisibility::GEOMETRY:
									case Rhi::ShaderVisibility::COMPUTE:
										RHI_ASSERT(openGLRhi.getContext(), false, "Invalid OpenGL shader visibility")
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
					const Rhi::Context& context = openGLRhi.getContext();
					char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramPipelineInfoLog(mOpenGLProgramPipeline, informationLength, nullptr, informationLog);

					// Output the debug string
					RHI_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous used OpenGL program pipeline
				if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
				{
					glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
				}
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLProgramPipeline && openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics program", 19)	// 19 = "Graphics program: " including terminating zero
					glObjectLabel(GL_PROGRAM_PIPELINE, mOpenGLProgramPipeline, -1, detailedDebugName);
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
			if (nullptr != mTaskShaderSeparate)
			{
				mTaskShaderSeparate->releaseReference();
			}
			if (nullptr != mMeshShaderSeparate)
			{
				mMeshShaderSeparate->releaseReference();
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
	//[ Public virtual Rhi::IGraphicsProgram methods          ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Rhi::handle getUniformHandle(const char* uniformName) override
		{
			GLint uniformLocation = -1;
			#define GET_UNIFORM_LOCATION(ShaderSeparate) if (uniformLocation < 0 && nullptr != ShaderSeparate) uniformLocation = glGetUniformLocation(ShaderSeparate->getOpenGLShaderProgram(), uniformName);
			GET_UNIFORM_LOCATION(mVertexShaderSeparate)
			GET_UNIFORM_LOCATION(mTessellationControlShaderSeparate)
			GET_UNIFORM_LOCATION(mTessellationEvaluationShaderSeparate)
			GET_UNIFORM_LOCATION(mGeometryShaderSeparate)
			GET_UNIFORM_LOCATION(mFragmentShaderSeparate)
			GET_UNIFORM_LOCATION(mTaskShaderSeparate)
			GET_UNIFORM_LOCATION(mMeshShaderSeparate)
			#undef GET_UNIFORM_LOCATION
			return static_cast<Rhi::handle>(uniformLocation);
		}

		virtual void setUniform1i(Rhi::handle uniformHandle, int value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform1f(Rhi::handle uniformHandle, float value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform2fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform3fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniform4fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniformMatrix3fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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

		virtual void setUniformMatrix4fv(Rhi::handle uniformHandle, const float* value) override
		{
			#ifdef RHI_OPENGL_STATE_CLEANUP
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsProgramSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint mOpenGLProgramPipeline;	///< OpenGL program pipeline ("container" object, not shared between OpenGL contexts), can be zero if no resource is allocated
		// Traditional graphics program
		VertexShaderSeparate*				  mVertexShaderSeparate;					///< Vertex shader the program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderSeparate*	  mTessellationControlShaderSeparate;		///< Tessellation control shader the program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderSeparate* mTessellationEvaluationShaderSeparate;	///< Tessellation evaluation shader the program is using (we keep a reference to it), can be a null pointer
		GeometryShaderSeparate*				  mGeometryShaderSeparate;					///< Geometry shader the program is using (we keep a reference to it), can be a null pointer
		// Both graphics programs
		FragmentShaderSeparate* mFragmentShaderSeparate;	///< Fragment shader the program is using (we keep a reference to it), can be a null pointer
		// Task and mesh shader based graphics program
		TaskShaderSeparate* mTaskShaderSeparate;	///< Task shader the program is using (we keep a reference to it), can be a null pointer
		MeshShaderSeparate* mMeshShaderSeparate;	///< Mesh shader the program is using (we keep a reference to it), can be a null pointer


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramSeparate(const GraphicsProgramSeparate& source) = delete;
		GraphicsProgramSeparate& operator =(const GraphicsProgramSeparate& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/Shader/Separate/GraphicsProgramSeparateDsa.h ]
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
		*    Constructor for traditional graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		inline GraphicsProgramSeparateDsa(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, VertexShaderSeparate* vertexShaderSeparate, TessellationControlShaderSeparate* tessellationControlShaderSeparate, TessellationEvaluationShaderSeparate* tessellationEvaluationShaderSeparate, GeometryShaderSeparate* geometryShaderSeparate, FragmentShaderSeparate* fragmentShaderSeparate RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			GraphicsProgramSeparate(openGLRhi, rootSignature, vertexShaderSeparate, tessellationControlShaderSeparate, tessellationEvaluationShaderSeparate, geometryShaderSeparate, fragmentShaderSeparate RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Constructor for task and mesh shader based graphics program
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] taskShaderSeparate
		*    Task shader the graphics program is using, can be a null pointer
		*  @param[in] meshShaderSeparate
		*    Mesh shader the graphics program is using
		*  @param[in] fragmentShaderSeparate
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		inline GraphicsProgramSeparateDsa(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, TaskShaderSeparate* taskShaderSeparate, MeshShaderSeparate& meshShaderSeparate, FragmentShaderSeparate* fragmentShaderSeparate RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			GraphicsProgramSeparate(openGLRhi, rootSignature, taskShaderSeparate, meshShaderSeparate, fragmentShaderSeparate RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GraphicsProgramSeparateDsa() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IGraphicsProgram methods          ]
	//[-------------------------------------------------------]
	public:
		virtual void setUniform1f(Rhi::handle uniformHandle, float value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform1f(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), value);
			}
			else
			{
				glProgramUniform1fEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), value);
			}
		}

		virtual void setUniform2fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform2fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform2fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform3fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform3fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform3fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniform4fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniform4fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
			else
			{
				glProgramUniform4fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, value);
			}
		}

		virtual void setUniformMatrix3fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
			{
				glProgramUniformMatrix3fv(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
			else
			{
				glProgramUniformMatrix3fvEXT(mVertexShaderSeparate->getOpenGLShaderProgram(), static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			}
		}

		virtual void setUniformMatrix4fv(Rhi::handle uniformHandle, const float* value) override
		{
			if (static_cast<OpenGLRhi&>(getRhi()).getExtensions().isGL_ARB_direct_state_access())
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
	//[ OpenGLRhi/Shader/Separate/ComputePipelineStateSeparate.h ]
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
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
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
		ComputePipelineStateSeparate(OpenGLRhi& openGLRhi, const Rhi::IRootSignature& rootSignature, ComputeShaderSeparate& computeShaderSeparate, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ComputePipelineState(openGLRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLProgramPipeline(0),
			mComputeShaderSeparate(computeShaderSeparate)
		{
			// Create the OpenGL program pipeline
			glGenProgramPipelines(1, &mOpenGLProgramPipeline);

			// If the "GL_ARB_direct_state_access" nor "GL_EXT_direct_state_access" extension is available, we need to change OpenGL states during resource creation (nasty thing)
			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently used OpenGL program pipeline
				GLint openGLProgramPipelineBackup = 0;
			#endif
			if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
			{
				#ifdef RHI_OPENGL_STATE_CLEANUP
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
				const Rhi::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				uint32_t uniformBlockBindingIndex = 0;
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					const Rhi::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_ASSERT(openGLRhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
						const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
						{
							const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

							// Ignore sampler range types in here (OpenGL handles samplers in a different way then Direct3D 10>=)
							if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
							{
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::ALL_GRAPHICS:
									case Rhi::ShaderVisibility::VERTEX:
									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
									case Rhi::ShaderVisibility::GEOMETRY:
									case Rhi::ShaderVisibility::FRAGMENT:
									case Rhi::ShaderVisibility::TASK:
									case Rhi::ShaderVisibility::MESH:
										RHI_ASSERT(openGLRhi.getContext(), false, "Invalid OpenGL shader visibility")
										break;

									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::COMPUTE:
										::detail::bindUniformBlock(descriptorRange, mComputeShaderSeparate.getOpenGLShaderProgram(), uniformBlockBindingIndex);
										break;
								}
								++uniformBlockBindingIndex;
							}
							else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
							{
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::ALL_GRAPHICS:
									case Rhi::ShaderVisibility::VERTEX:
									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
									case Rhi::ShaderVisibility::GEOMETRY:
									case Rhi::ShaderVisibility::FRAGMENT:
									case Rhi::ShaderVisibility::TASK:
									case Rhi::ShaderVisibility::MESH:
										RHI_ASSERT(openGLRhi.getContext(), false, "Invalid OpenGL shader visibility")
										break;

									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::COMPUTE:
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
					const Rhi::Context& context = openGLRhi.getContext();
					char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramPipelineInfoLog(mOpenGLProgramPipeline, informationLength, nullptr, informationLog);

					// Output the debug string
					RHI_LOG(context, CRITICAL, informationLog)

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous used OpenGL program pipeline
				if (nullptr == glProgramUniform1i && nullptr == glProgramUniform1iEXT)
				{
					glBindProgramPipeline(static_cast<GLuint>(openGLProgramPipelineBackup));
				}
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLRhi.getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Compute PSO", 14)	// 14 = "Compute PSO: " including terminating zero
					glObjectLabel(GL_PROGRAM_PIPELINE, mOpenGLProgramPipeline, -1, detailedDebugName);
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputePipelineStateSeparate, this);
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
	//[ OpenGLRhi/Shader/Separate/ShaderLanguageSeparate.h    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Separate shader language class
	*/
	class ShaderLanguageSeparate final : public Rhi::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*/
		inline explicit ShaderLanguageSeparate(OpenGLRhi& openGLRhi) :
			IShaderLanguage(openGLRhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ShaderLanguageSeparate() override
		{
			// De-initialize glslang, if necessary
			#ifdef RHI_OPENGL_GLSLTOSPIRV
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
	//[ Public virtual Rhi::IShaderLanguage methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSL_NAME;
		}

		[[nodiscard]] virtual Rhi::IVertexShader* createVertexShaderFromBytecode(const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL vertex shader bytecode is invalid")

			// Check whether or not there's vertex shader support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_vertex_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), VertexShaderSeparate)(openGLRhi, vertexAttributes, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no vertex shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IVertexShader* createVertexShaderFromSourceCode(const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's vertex shader support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_vertex_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), VertexShaderSeparate)(openGLRhi, vertexAttributes, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no vertex shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL tessellation control shader bytecode is invalid")

			// Check whether or not there's tessellation support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), TessellationControlShaderSeparate)(openGLRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no tessellation support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's tessellation support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), TessellationControlShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL tessellation evaluation shader bytecode is invalid")

			// Check whether or not there's tessellation support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), TessellationEvaluationShaderSeparate)(openGLRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no tessellation support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's tessellation support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_tessellation_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), TessellationEvaluationShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no tessellation support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL geometry shader bytecode is invalid")

			// Check whether or not there's geometry shader support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_geometry_shader4() && extensions.isGL_ARB_gl_spirv())
			{
				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
				return RHI_NEW(openGLRhi.getContext(), GeometryShaderSeparate)(openGLRhi, shaderBytecode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no geometry shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's geometry shader support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_geometry_shader4())
			{
				// In modern GLSL, "geometry shader input primitive topology" & "geometry shader output primitive topology" & "number of output vertices" can be directly set within GLSL by writing e.g.
				//   "layout(triangles) in;"
				//   "layout(triangle_strip, max_vertices = 3) out;"
				// -> To be able to support older GLSL versions, we have to provide this information also via OpenGL API functions
				return RHI_NEW(openGLRhi.getContext(), GeometryShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no geometry shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL fragment shader bytecode is invalid")

			// Check whether or not there's fragment shader support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_fragment_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), FragmentShaderSeparate)(openGLRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no fragment shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's fragment shader support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_fragment_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), FragmentShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no fragment shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITaskShader* createTaskShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL task shader bytecode is invalid")

			// Check whether or not there's task shader support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_NV_mesh_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), TaskShaderSeparate)(openGLRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no task shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::ITaskShader* createTaskShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's task shader support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_NV_mesh_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), TaskShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no task shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IMeshShader* createMeshShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL mesh shader bytecode is invalid")

			// Check whether or not there's mesh shader support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_NV_mesh_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), MeshShaderSeparate)(openGLRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no mesh shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IMeshShader* createMeshShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's mesh shader support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_NV_mesh_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), MeshShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no mesh shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IComputeShader* createComputeShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "OpenGL compute shader bytecode is invalid")

			// Check whether or not there's compute shader support
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_compute_shader() && extensions.isGL_ARB_gl_spirv())
			{
				return RHI_NEW(openGLRhi.getContext(), ComputeShaderSeparate)(openGLRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no compute shader support or no decent shader bytecode support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IComputeShader* createComputeShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// Check whether or not there's compute shader support
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());
			const Extensions& extensions = openGLRhi.getExtensions();
			if (extensions.isGL_ARB_compute_shader())
			{
				return RHI_NEW(openGLRhi.getContext(), ComputeShaderSeparate)(openGLRhi, shaderSourceCode.sourceCode, (extensions.isGL_ARB_gl_spirv() ? shaderBytecode : nullptr) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Error! There's no compute shader support!
				return nullptr;
			}
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram(const Rhi::IRootSignature& rootSignature, [[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, Rhi::ITessellationControlShader* tessellationControlShader, Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
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
			else if (openGLRhi.getExtensions().isGL_EXT_direct_state_access() || openGLRhi.getExtensions().isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramSeparateDsa)(openGLRhi, rootSignature, static_cast<VertexShaderSeparate*>(vertexShader), static_cast<TessellationControlShaderSeparate*>(tessellationControlShader), static_cast<TessellationEvaluationShaderSeparate*>(tessellationEvaluationShader), static_cast<GeometryShaderSeparate*>(geometryShader), static_cast<FragmentShaderSeparate*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramSeparate)(openGLRhi, rootSignature, static_cast<VertexShaderSeparate*>(vertexShader), static_cast<TessellationControlShaderSeparate*>(tessellationControlShader), static_cast<TessellationEvaluationShaderSeparate*>(tessellationEvaluationShader), static_cast<GeometryShaderSeparate*>(geometryShader), static_cast<FragmentShaderSeparate*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram(const Rhi::IRootSignature& rootSignature, Rhi::ITaskShader* taskShader, Rhi::IMeshShader& meshShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLRhi& openGLRhi = static_cast<OpenGLRhi&>(getRhi());

			// A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			if (nullptr != taskShader && taskShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Vertex shader language mismatch!
			}
			else if (meshShader.getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Fragment shader language mismatch!
			}
			else if (nullptr != fragmentShader && fragmentShader->getShaderLanguageName() != ::detail::GLSL_NAME)
			{
				// Error! Vertex shader language mismatch!
			}

			// Is "GL_EXT_direct_state_access" there?
			else if (openGLRhi.getExtensions().isGL_EXT_direct_state_access() || openGLRhi.getExtensions().isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramSeparateDsa)(openGLRhi, rootSignature, static_cast<TaskShaderSeparate*>(taskShader), static_cast<MeshShaderSeparate&>(meshShader), static_cast<FragmentShaderSeparate*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				return RHI_NEW(openGLRhi.getContext(), GraphicsProgramSeparate)(openGLRhi, rootSignature, static_cast<TaskShaderSeparate*>(taskShader), static_cast<MeshShaderSeparate&>(meshShader), static_cast<FragmentShaderSeparate*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}

			// Error! Shader language mismatch!
			// -> Ensure a correct reference counter behaviour, even in the situation of an error
			if (nullptr != taskShader)
			{
				taskShader->addReference();
				taskShader->releaseReference();
			}
			meshShader.addReference();
			meshShader.releaseReference();
			if (nullptr != fragmentShader)
			{
				fragmentShader->addReference();
				fragmentShader->releaseReference();
			}

			// Error!
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ShaderLanguageSeparate, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageSeparate(const ShaderLanguageSeparate& source) = delete;
		ShaderLanguageSeparate& operator =(const ShaderLanguageSeparate& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLRhi/State/GraphicsPipelineState.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL graphics pipeline state class
	*/
	class GraphicsPipelineState final : public Rhi::IGraphicsPipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLRhi
		*    Owner OpenGL RHI instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(OpenGLRhi& openGLRhi, const Rhi::GraphicsPipelineState& graphicsPipelineState, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsPipelineState(openGLRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLPrimitiveTopology(0xFFFF),	// Unknown default setting
			mNumberOfVerticesPerPatch(0),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mRasterizerState(graphicsPipelineState.rasterizerState),
			mDepthStencilState(graphicsPipelineState.depthStencilState),
			mBlendState(graphicsPipelineState.blendState)
		{
			// Tessellation support: Up to 32 vertices per patch are supported "Rhi::PrimitiveTopology::PATCH_LIST_1" ... "Rhi::PrimitiveTopology::PATCH_LIST_32"
			if (graphicsPipelineState.primitiveTopology >= Rhi::PrimitiveTopology::PATCH_LIST_1)
			{
				// Use tessellation

				// Get number of vertices that will be used to make up a single patch primitive
				// -> There's no need to check for the "GL_ARB_tessellation_shader" extension, it's there if "Rhi::Capabilities::maximumNumberOfPatchVertices" is not 0
				const int numberOfVerticesPerPatch = static_cast<int>(graphicsPipelineState.primitiveTopology) - static_cast<int>(Rhi::PrimitiveTopology::PATCH_LIST_1) + 1;
				if (numberOfVerticesPerPatch <= static_cast<int>(openGLRhi.getCapabilities().maximumNumberOfPatchVertices))
				{
					// Set number of vertices that will be used to make up a single patch primitive
					mNumberOfVerticesPerPatch = numberOfVerticesPerPatch;

					// Set OpenGL primitive topology
					mOpenGLPrimitiveTopology = GL_PATCHES;
				}
				else
				{
					// Error!
					RHI_ASSERT(openGLRhi.getContext(), false, "Invalid number of OpenGL vertices per patch")
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

			// Add a reference to the referenced RHI resources
			mGraphicsProgram->addReference();
			mRenderPass->addReference();
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsPipelineState() override
		{
			// Release referenced RHI resources
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Free the unique compact graphics pipeline state ID
			static_cast<OpenGLRhi&>(getRhi()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the graphics program
		*
		*  @return
		*    Graphics program, always valid
		*/
		[[nodiscard]] inline Rhi::IGraphicsProgram* getGraphicsProgram() const
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
			static_cast<OpenGLRhi&>(getRhi()).setOpenGLGraphicsProgram(mGraphicsProgram);

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
		[[nodiscard]] inline const Rhi::RasterizerState& getRasterizerState() const
		{
			return mRasterizerState.getRasterizerState();
		}

		[[nodiscard]] inline const Rhi::DepthStencilState& getDepthStencilState() const
		{
			return mDepthStencilState.getDepthStencilState();
		}

		[[nodiscard]] inline const Rhi::BlendState& getBlendState() const
		{
			return mBlendState.getBlendState();
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsPipelineState, this);
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
		GLenum				   mOpenGLPrimitiveTopology;	///< OpenGL primitive topology describing the type of primitive to render
		GLint				   mNumberOfVerticesPerPatch;	///< Number of vertices per patch
		Rhi::IGraphicsProgram* mGraphicsProgram;
		Rhi::IRenderPass*	   mRenderPass;
		RasterizerState		   mRasterizerState;
		DepthStencilState	   mDepthStencilState;
		BlendState			   mBlendState;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // OpenGLRhi




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

		[[nodiscard]] bool mapBuffer([[maybe_unused]] const Rhi::Context& context, const OpenGLRhi::Extensions& extensions, GLenum target, [[maybe_unused]] GLenum bindingTarget, GLuint openGLBuffer, Rhi::MapType mapType, Rhi::MappedSubresource& mappedSubresource)
		{
			// TODO(co) This buffer update isn't efficient, use e.g. persistent buffer mapping

			// Is "GL_ARB_direct_state_access" there?
			if (extensions.isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				mappedSubresource.data		 = glMapNamedBuffer(openGLBuffer, OpenGLRhi::Mapping::getOpenGLMapType(mapType));
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
			}
			// Is "GL_EXT_direct_state_access" there?
			else if (extensions.isGL_EXT_direct_state_access())
			{
				// Effective direct state access (DSA)
				mappedSubresource.data		 = glMapNamedBufferEXT(openGLBuffer, OpenGLRhi::Mapping::getOpenGLMapType(mapType));
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
			}
			else
			{
				// Traditional bind version

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL buffer
					GLint openGLBufferBackup = 0;
					glGetIntegerv(bindingTarget, &openGLBufferBackup);
				#endif

				// Bind this OpenGL buffer
				glBindBufferARB(target, openGLBuffer);

				// Map
				mappedSubresource.data		 = glMapBufferARB(target, OpenGLRhi::Mapping::getOpenGLMapType(mapType));
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL buffer
					glBindBufferARB(target, static_cast<GLuint>(openGLBufferBackup));
				#endif
			}

			// Done
			RHI_ASSERT(context, nullptr != mappedSubresource.data, "Mapping of OpenGL buffer failed")
			return (nullptr != mappedSubresource.data);
		}

		void unmapBuffer(const OpenGLRhi::Extensions& extensions, GLenum target, [[maybe_unused]] GLenum bindingTarget, GLuint openGLBuffer)
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

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Backup the currently bound OpenGL buffer
					GLint openGLBufferBackup = 0;
					glGetIntegerv(bindingTarget, &openGLBufferBackup);
				#endif

				// Bind this OpenGL buffer
				glBindBufferARB(target, openGLBuffer);

				// Unmap
				glUnmapBufferARB(target);

				#ifdef RHI_OPENGL_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL buffer
					glBindBufferARB(target, static_cast<GLuint>(openGLBufferBackup));
				#endif
			}
		}

		namespace ImplementationDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ExecuteCommandBuffer* realData = static_cast<const Rhi::Command::ExecuteCommandBuffer*>(data);
				RHI_ASSERT(rhi.getContext(), nullptr != realData->commandBufferToExecute, "The OpenGL command buffer to execute must be valid")
				rhi.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsRootSignature* realData = static_cast<const Rhi::Command::SetGraphicsRootSignature*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsPipelineState* realData = static_cast<const Rhi::Command::SetGraphicsPipelineState*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsResourceGroup* realData = static_cast<const Rhi::Command::SetGraphicsResourceGroup*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Rhi::IRhi& rhi)
			{
				// Input-assembler (IA) stage
				const Rhi::Command::SetGraphicsVertexArray* realData = static_cast<const Rhi::Command::SetGraphicsVertexArray*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsViewports* realData = static_cast<const Rhi::Command::SetGraphicsViewports*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Rhi::Viewport*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsScissorRectangles* realData = static_cast<const Rhi::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Rhi::ScissorRectangle*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Rhi::IRhi& rhi)
			{
				// Output-merger (OM) stage
				const Rhi::Command::SetGraphicsRenderTarget* realData = static_cast<const Rhi::Command::SetGraphicsRenderTarget*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ClearGraphics* realData = static_cast<const Rhi::Command::ClearGraphics*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).drawGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).drawIndexedGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawMeshTasks(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawMeshTasks* realData = static_cast<const Rhi::Command::DrawMeshTasks*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).drawMeshTasks(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).drawMeshTasksEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeRootSignature* realData = static_cast<const Rhi::Command::SetComputeRootSignature*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputePipelineState* realData = static_cast<const Rhi::Command::SetComputePipelineState*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeResourceGroup* realData = static_cast<const Rhi::Command::SetComputeResourceGroup*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCompute* realData = static_cast<const Rhi::Command::DispatchCompute*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).dispatchCompute(realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Rhi::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				RHI_ASSERT(static_cast<OpenGLRhi::OpenGLRhi&>(rhi).getContext(), realData->texture->getResourceType() == Rhi::ResourceType::TEXTURE_2D, "Unsupported OpenGL texture resource type")
				static_cast<OpenGLRhi::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
			}

			void ResolveMultisampleFramebuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Rhi::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyResource* realData = static_cast<const Rhi::Command::CopyResource*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::GenerateMipmaps* realData = static_cast<const Rhi::Command::GenerateMipmaps*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResetQueryPool* realData = static_cast<const Rhi::Command::ResetQueryPool*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::BeginQuery* realData = static_cast<const Rhi::Command::BeginQuery*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::EndQuery* realData = static_cast<const Rhi::Command::EndQuery*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::WriteTimestampQuery* realData = static_cast<const Rhi::Command::WriteTimestampQuery*>(data);
				static_cast<OpenGLRhi::OpenGLRhi&>(rhi).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RHI_DEBUG
				void SetDebugMarker(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::SetDebugMarker* realData = static_cast<const Rhi::Command::SetDebugMarker*>(data);
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::BeginDebugEvent* realData = static_cast<const Rhi::Command::BeginDebugEvent*>(data);
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Rhi::IRhi& rhi)
				{
					static_cast<OpenGLRhi::OpenGLRhi&>(rhi).endDebugEvent();
				}
			#else
				void SetDebugMarker(const void*, Rhi::IRhi&)
				{
					// Nothing here
				}

				void BeginDebugEvent(const void*, Rhi::IRhi&)
				{
					// Nothing here
				}

				void EndDebugEvent(const void*, Rhi::IRhi&)
				{
					// Nothing here
				}
			#endif


		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Rhi::ImplementationDispatchFunction DISPATCH_FUNCTIONS[static_cast<uint8_t>(Rhi::CommandDispatchFunctionIndex::NUMBER_OF_FUNCTIONS)] =
		{
			// Command buffer
			&ImplementationDispatch::ExecuteCommandBuffer,
			// Graphics
			&ImplementationDispatch::SetGraphicsRootSignature,
			&ImplementationDispatch::SetGraphicsPipelineState,
			&ImplementationDispatch::SetGraphicsResourceGroup,
			&ImplementationDispatch::SetGraphicsVertexArray,		// Input-assembler (IA) stage
			&ImplementationDispatch::SetGraphicsViewports,			// Rasterizer (RS) stage
			&ImplementationDispatch::SetGraphicsScissorRectangles,	// Rasterizer (RS) stage
			&ImplementationDispatch::SetGraphicsRenderTarget,		// Output-merger (OM) stage
			&ImplementationDispatch::ClearGraphics,
			&ImplementationDispatch::DrawGraphics,
			&ImplementationDispatch::DrawIndexedGraphics,
			&ImplementationDispatch::DrawMeshTasks,
			// Compute
			&ImplementationDispatch::SetComputeRootSignature,
			&ImplementationDispatch::SetComputePipelineState,
			&ImplementationDispatch::SetComputeResourceGroup,
			&ImplementationDispatch::DispatchCompute,
			// Resource
			&ImplementationDispatch::SetTextureMinimumMaximumMipmapIndex,
			&ImplementationDispatch::ResolveMultisampleFramebuffer,
			&ImplementationDispatch::CopyResource,
			&ImplementationDispatch::GenerateMipmaps,
			// Query
			&ImplementationDispatch::ResetQueryPool,
			&ImplementationDispatch::BeginQuery,
			&ImplementationDispatch::EndQuery,
			&ImplementationDispatch::WriteTimestampQuery,
			// Debug
			&ImplementationDispatch::SetDebugMarker,
			&ImplementationDispatch::BeginDebugEvent,
			&ImplementationDispatch::EndDebugEvent
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace OpenGLRhi
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	OpenGLRhi::OpenGLRhi(const Rhi::Context& context) :
		IRhi(Rhi::NameId::OPENGL, context),
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
		#ifdef RHI_DEBUG
			, mDebugBetweenBeginEndScene(false)
		#endif
	{
		// Is OpenGL available?
		mOpenGLRuntimeLinking = RHI_NEW(mContext, OpenGLRuntimeLinking)(*this);
		if (mOpenGLRuntimeLinking->isOpenGLAvaiable())
		{
			const Rhi::handle nativeWindowHandle = mContext.getNativeWindowHandle();
			const Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::Enum::R8G8B8A8;
			const RenderPass renderPass(*this, 1, &textureFormat, Rhi::TextureFormat::Enum::UNKNOWN, 1 RHI_RESOURCE_DEBUG_NAME("OpenGL Unknown"));
			#ifdef _WIN32
			{
				// TODO(co) Add external OpenGL context support
				mOpenGLContext = RHI_NEW(mContext, OpenGLContextWindows)(mOpenGLRuntimeLinking, renderPass.getDepthStencilAttachmentTextureFormat(), nativeWindowHandle);
			}
			#elif defined LINUX
				mOpenGLContext = RHI_NEW(mContext, OpenGLContextLinux)(*this, mOpenGLRuntimeLinking, renderPass.getDepthStencilAttachmentTextureFormat(), nativeWindowHandle, mContext.isUsingExternalContext());
			#else
				#error "Unsupported platform"
			#endif

			// We're using "this" in here, so we are not allowed to write the following within the initializer list
			mExtensions = RHI_NEW(mContext, Extensions)(*this, *mOpenGLContext);

			// Is the OpenGL context and extensions initialized?
			if (mOpenGLContext->isInitialized() && mExtensions->initialize())
			{
				#ifdef RHI_DEBUG
					// "GL_ARB_debug_output"-extension available?
					if (mExtensions->isGL_ARB_debug_output())
					{
						// Synchronous debug output, please
						// -> Makes it easier to find the place causing the issue
						glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

						// Disable severity notifications, most drivers print many things with this severity
						glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, false);

						// Set the debug message callback function
						glDebugMessageCallbackARB(&OpenGLRhi::debugMessageCallback, this);
					}
				#endif

				// Globally enable seamless cube map texture, e.g. Direct3D 11 has this enabled by default so do the same for OpenGL
				// -> The following is just for the sake of completeness: It's the year 2020 and OpenGL on Mac is officially dead. But if someone would still
				//    want to support it in a productive way, one has to take care of the situation that enabling seamless cube map texture can result on
				//    slow software rendering on Mac. For checking whether or not this is the case, see "GL_TEXTURE_CUBE_MAP_SEAMLESS on OS X" published at April 26, 2012 on http://distrustsimplicity.net/articles/gl_texture_cube_map_seamless-on-os-x/
				//    "
				//    GLint gpuVertex, gpuFragment;
				//    CGLGetParameter(CGLGetCurrentContext(), kCGLCPGPUVertexProcessing, &gpuVertex);
				//    CGLGetParameter(CGLGetCurrentContext(), kCGLCPGPUFragmentProcessing, &gpuFragment);
				//    "
				glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

				// Initialize the capabilities
				initializeCapabilities();

				// Create the default sampler state
				mDefaultSamplerState = createSamplerState(Rhi::ISamplerState::getDefaultSamplerState());

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

	OpenGLRhi::~OpenGLRhi()
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

		// Destroy the OpenGL framebuffer used by "OpenGLRhi::OpenGLRhi::copyResource()" if the "GL_ARB_copy_image"-extension isn't available
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

		#ifdef RHI_STATISTICS
		{ // For debugging: At this point there should be no resource instances left, validate this!
			// -> Are the currently any resource instances?
			const uint32_t numberOfCurrentResources = getStatistics().getNumberOfCurrentResources();
			if (numberOfCurrentResources > 0)
			{
				// Error!
				if (numberOfCurrentResources > 1)
				{
					RHI_ASSERT(mContext, false, "The OpenGL RHI implementation is going to be destroyed, but there are still %lu resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RHI_ASSERT(mContext, false, "The OpenGL RHI implementation is going to be destroyed, but there is still one resource instance left (memory leak)")
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
		RHI_DELETE(mContext, Extensions, mExtensions);

		// Destroy the OpenGL context instance
		RHI_DELETE(mContext, IOpenGLContext, mOpenGLContext);

		// Destroy the OpenGL runtime linking instance
		RHI_DELETE(mContext, OpenGLRuntimeLinking, mOpenGLRuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void OpenGLRhi::setGraphicsRootSignature(Rhi::IRootSignature* rootSignature)
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
			RHI_MATCH_CHECK(*this, *rootSignature)
		}
	}

	void OpenGLRhi::setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (mGraphicsPipelineState != graphicsPipelineState)
		{
			if (nullptr != graphicsPipelineState)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *graphicsPipelineState)

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

	void OpenGLRhi::setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mGraphicsRootSignature, "No OpenGL RHI implementation graphics root signature set")
			const Rhi::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The OpenGL RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The OpenGL RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The OpenGL RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		// Set graphics resource group
		setResourceGroup(*mGraphicsRootSignature, rootParameterIndex, resourceGroup);
	}

	void OpenGLRhi::setGraphicsVertexArray(Rhi::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage

		// New vertex array?
		if (mVertexArray != vertexArray)
		{
			// Set a vertex array?
			if (nullptr != vertexArray)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *vertexArray)

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

	void OpenGLRhi::setGraphicsViewports([[maybe_unused]] uint32_t numberOfViewports, const Rhi::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid OpenGL rasterizer state viewports")

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
		// TODO(co) "GL_ARB_viewport_array" support ("OpenGLRhi::setGraphicsViewports()")
		// TODO(co) Check for "numberOfViewports" out of range or are the debug events good enough?
		RHI_ASSERT(mContext, numberOfViewports <= 1, "OpenGL supports only one viewport")
		glViewport(static_cast<GLint>(viewports->topLeftX), static_cast<GLint>(static_cast<float>(renderTargetHeight) - viewports->topLeftY - viewports->height), static_cast<GLsizei>(viewports->width), static_cast<GLsizei>(viewports->height));
		glDepthRange(static_cast<GLclampd>(viewports->minDepth), static_cast<GLclampd>(viewports->maxDepth));
	}

	void OpenGLRhi::setGraphicsScissorRectangles([[maybe_unused]] uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid OpenGL rasterizer state scissor rectangles")

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
		// TODO(co) "GL_ARB_viewport_array" support ("OpenGLRhi::setGraphicsViewports()")
		// TODO(co) Check for "numberOfViewports" out of range or are the debug events good enough?
		RHI_ASSERT(mContext, numberOfScissorRectangles <= 1, "OpenGL supports only one scissor rectangle")
		const GLsizei width  = scissorRectangles->bottomRightX - scissorRectangles->topLeftX;
		const GLsizei height = scissorRectangles->bottomRightY - scissorRectangles->topLeftY;
		glScissor(static_cast<GLint>(scissorRectangles->topLeftX), static_cast<GLint>(renderTargetHeight - static_cast<uint32_t>(scissorRectangles->topLeftY) - height), width, height);
	}

	void OpenGLRhi::setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *renderTarget)

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					// Unbind OpenGL framebuffer?
					if (Rhi::ResourceType::FRAMEBUFFER == mRenderTarget->getResourceType() && Rhi::ResourceType::FRAMEBUFFER != renderTarget->getResourceType())
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
					case Rhi::ResourceType::SWAP_CHAIN:
					{
						static_cast<SwapChain*>(mRenderTarget)->getOpenGLContext().makeCurrent();
						clipControlOrigin = GL_LOWER_LEFT;	// Compensate OS window coordinate system y-flip
						break;
					}

					case Rhi::ResourceType::FRAMEBUFFER:
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

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_2D:
					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
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
				if (Rhi::ResourceType::FRAMEBUFFER == mRenderTarget->getResourceType())
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

	void OpenGLRhi::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Sanity check
		RHI_ASSERT(mContext, z >= 0.0f && z <= 1.0f, "The OpenGL clear graphics z value must be between [0, 1] (inclusive)")

		// Get API flags
		uint32_t flagsApi = 0;
		if (clearFlags & Rhi::ClearFlag::COLOR)
		{
			flagsApi |= GL_COLOR_BUFFER_BIT;
		}
		if (clearFlags & Rhi::ClearFlag::DEPTH)
		{
			flagsApi |= GL_DEPTH_BUFFER_BIT;
		}
		if (clearFlags & Rhi::ClearFlag::STENCIL)
		{
			flagsApi |= GL_STENCIL_BUFFER_BIT;
		}

		// Are API flags set?
		if (0 != flagsApi)
		{
			// Set clear settings
			if (clearFlags & Rhi::ClearFlag::COLOR)
			{
				glClearColor(color[0], color[1], color[2], color[3]);
			}
			if (clearFlags & Rhi::ClearFlag::DEPTH)
			{
				glClearDepth(static_cast<GLclampd>(z));
				if (nullptr != mGraphicsPipelineState && Rhi::DepthWriteMask::ALL != mGraphicsPipelineState->getDepthStencilState().depthWriteMask)
				{
					glDepthMask(GL_TRUE);
				}
			}
			if (clearFlags & Rhi::ClearFlag::STENCIL)
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
			if ((clearFlags & Rhi::ClearFlag::DEPTH) && nullptr != mGraphicsPipelineState && Rhi::DepthWriteMask::ALL != mGraphicsPipelineState->getDepthStencilState().depthWriteMask)
			{
				glDepthMask(GL_FALSE);
			}
		}
	}

	void OpenGLRhi::drawGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of OpenGL draws must not be zero")
		RHI_ASSERT(mContext, mExtensions->isGL_ARB_draw_indirect(), "The GL_ARB_draw_indirect OpenGL extension isn't supported")
		// It's possible to draw without "mVertexArray"

		// Tessellation support: "glPatchParameteri()" is called within "OpenGLRhi::iaSetPrimitiveTopology()"

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
				#ifdef RHI_DEBUG
					beginDebugEvent("Multi-draw-indirect emulation");
				#endif
				for (uint32_t i = 0; i < numberOfDraws; ++i)
				{
					glDrawArraysIndirect(mOpenGLPrimitiveTopology, reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)));
					indirectBufferOffset += sizeof(Rhi::DrawArguments);
				}
				#ifdef RHI_DEBUG
					endDebugEvent();
				#endif
			}
		}
	}

	void OpenGLRhi::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The OpenGL emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-draw-indirect emulation");
			}
		#endif
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Rhi::DrawArguments& drawArguments = *reinterpret_cast<const Rhi::DrawArguments*>(emulationData);
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
				RHI_ASSERT(mContext, drawArguments.instanceCount <= 1, "Invalid OpenGL instance count")
				glDrawArrays(mOpenGLPrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance));
			}
			emulationData += sizeof(Rhi::DrawArguments);
		}
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void OpenGLRhi::drawIndexedGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of OpenGL draws must not be zero")
		RHI_ASSERT(mContext, nullptr != mVertexArray, "OpenGL draw indexed needs a set vertex array")
		RHI_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "OpenGL draw indexed needs a set vertex array which contains an index buffer")
		RHI_ASSERT(mContext, mExtensions->isGL_ARB_draw_indirect(), "The GL_ARB_draw_indirect OpenGL extension isn't supported")

		// Tessellation support: "glPatchParameteri()" is called within "OpenGLRhi::iaSetPrimitiveTopology()"

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
				#ifdef RHI_DEBUG
					beginDebugEvent("Multi-indexed-draw-indirect emulation");
				#endif
				const uint32_t openGLType = mVertexArray->getIndexBuffer()->getOpenGLType();
				for (uint32_t i = 0; i < numberOfDraws; ++i)
				{
					glDrawElementsIndirect(mOpenGLPrimitiveTopology, openGLType, reinterpret_cast<void*>(static_cast<uintptr_t>(indirectBufferOffset)));
					indirectBufferOffset += sizeof(Rhi::DrawIndexedArguments);
				}
				#ifdef RHI_DEBUG
					endDebugEvent();
				#endif
			}
		}
	}

	void OpenGLRhi::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The OpenGL emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL draws must not be zero")
		RHI_ASSERT(mContext, nullptr != mVertexArray, "OpenGL draw indexed needs a set vertex array")
		RHI_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "OpenGL draw indexed needs a set vertex array which contains an index buffer")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			}
		#endif
		IndexBuffer* indexBuffer = mVertexArray->getIndexBuffer();
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Rhi::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Rhi::DrawIndexedArguments*>(emulationData);
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
						RHI_ASSERT(mContext, false, "Failed to OpenGL draw indexed emulated")
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
				RHI_ASSERT(mContext, drawIndexedArguments.instanceCount <= 1, "Invalid OpenGL instance count")

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
						RHI_ASSERT(mContext, false, "Failed to OpenGL draw indexed emulated")
					}
				}
				else
				{
					// Draw without base vertex location
					glDrawElements(mOpenGLPrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLType(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())));
				}
			}
			emulationData += sizeof(Rhi::DrawIndexedArguments);
		}
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void OpenGLRhi::drawMeshTasks([[maybe_unused]] const Rhi::IIndirectBuffer& indirectBuffer, [[maybe_unused]] uint32_t indirectBufferOffset, [[maybe_unused]] uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")

		// TODO(co) Implement me
		/*
		void MultiDrawMeshTasksIndirectNV(intptr indirect,
										sizei drawcount,
										sizei stride);

		void MultiDrawMeshTasksIndirectCountNV( intptr indirect,
												intptr drawcount,
												sizei maxdrawcount,
												sizei stride);
	  */
	}

	void OpenGLRhi::drawMeshTasksEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The OpenGL emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL draws must not be zero")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			}
		#endif
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Rhi::DrawMeshTasksArguments& drawMeshTasksArguments = *reinterpret_cast<const Rhi::DrawMeshTasksArguments*>(emulationData);

			// Draw and advance
			glDrawMeshTasksNV(drawMeshTasksArguments.firstTask, drawMeshTasksArguments.numberOfTasks);
			emulationData += sizeof(Rhi::DrawMeshTasksArguments);
		}
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}


	//[-------------------------------------------------------]
	//[ Compute                                               ]
	//[-------------------------------------------------------]
	void OpenGLRhi::setComputeRootSignature(Rhi::IRootSignature* rootSignature)
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
			RHI_MATCH_CHECK(*this, *rootSignature)
		}
	}

	void OpenGLRhi::setComputePipelineState(Rhi::IComputePipelineState* computePipelineState)
	{
		if (mComputePipelineState != computePipelineState)
		{
			if (nullptr != computePipelineState)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *computePipelineState)

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

	void OpenGLRhi::setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mComputeRootSignature, "No OpenGL RHI implementation compute root signature set")
			const Rhi::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The OpenGL RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The OpenGL RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The OpenGL RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		// Set compute resource group
		setResourceGroup(*mComputeRootSignature, rootParameterIndex, resourceGroup);
	}

	void OpenGLRhi::dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
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
	void OpenGLRhi::resolveMultisampleFramebuffer(Rhi::IRenderTarget& destinationRenderTarget, Rhi::IFramebuffer& sourceMultisampleFramebuffer)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, destinationRenderTarget)
		RHI_MATCH_CHECK(*this, sourceMultisampleFramebuffer)

		// Evaluate the render target type
		switch (destinationRenderTarget.getResourceType())
		{
			case Rhi::ResourceType::SWAP_CHAIN:
			{
				// Get the OpenGL swap chain instance
				// TODO(co) Implement me, not that important in practice so not directly implemented
				// SwapChain& swapChain = static_cast<SwapChain&>(destinationRenderTarget);
				break;
			}

			case Rhi::ResourceType::FRAMEBUFFER:
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

			case Rhi::ResourceType::ROOT_SIGNATURE:
			case Rhi::ResourceType::RESOURCE_GROUP:
			case Rhi::ResourceType::GRAPHICS_PROGRAM:
			case Rhi::ResourceType::VERTEX_ARRAY:
			case Rhi::ResourceType::RENDER_PASS:
			case Rhi::ResourceType::QUERY_POOL:
			case Rhi::ResourceType::VERTEX_BUFFER:
			case Rhi::ResourceType::INDEX_BUFFER:
			case Rhi::ResourceType::TEXTURE_BUFFER:
			case Rhi::ResourceType::STRUCTURED_BUFFER:
			case Rhi::ResourceType::INDIRECT_BUFFER:
			case Rhi::ResourceType::UNIFORM_BUFFER:
			case Rhi::ResourceType::TEXTURE_1D:
			case Rhi::ResourceType::TEXTURE_1D_ARRAY:
			case Rhi::ResourceType::TEXTURE_2D:
			case Rhi::ResourceType::TEXTURE_2D_ARRAY:
			case Rhi::ResourceType::TEXTURE_3D:
			case Rhi::ResourceType::TEXTURE_CUBE:
			case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
			case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
			case Rhi::ResourceType::SAMPLER_STATE:
			case Rhi::ResourceType::VERTEX_SHADER:
			case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Rhi::ResourceType::GEOMETRY_SHADER:
			case Rhi::ResourceType::FRAGMENT_SHADER:
			case Rhi::ResourceType::TASK_SHADER:
			case Rhi::ResourceType::MESH_SHADER:
			case Rhi::ResourceType::COMPUTE_SHADER:
			default:
				// Not handled in here
				break;
		}
	}

	void OpenGLRhi::copyResource(Rhi::IResource& destinationResource, Rhi::IResource& sourceResource)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, destinationResource)
		RHI_MATCH_CHECK(*this, sourceResource)

		// Evaluate the render target type
		switch (destinationResource.getResourceType())
		{
			case Rhi::ResourceType::TEXTURE_2D:
				if (sourceResource.getResourceType() == Rhi::ResourceType::TEXTURE_2D)
				{
					// Get the OpenGL texture 2D instances
					const Texture2D& openGlDestinationTexture2D = static_cast<const Texture2D&>(destinationResource);
					const Texture2D& openGlSourceTexture2D = static_cast<const Texture2D&>(sourceResource);
					RHI_ASSERT(mContext, openGlDestinationTexture2D.getWidth() == openGlSourceTexture2D.getWidth(), "OpenGL source and destination width must be identical for resource copy")
					RHI_ASSERT(mContext, openGlDestinationTexture2D.getHeight() == openGlSourceTexture2D.getHeight(), "OpenGL source and destination height must be identical for resource copy")

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
						#ifdef RHI_OPENGL_STATE_CLEANUP
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

						#ifdef RHI_OPENGL_STATE_CLEANUP
							// Be polite and restore the previous bound OpenGL framebuffer
							glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(openGLFramebufferBackup));
						#endif
					}
				}
				else
				{
					// Error!
					RHI_ASSERT(mContext, false, "Failed to copy OpenGL resource")
				}
				break;

			case Rhi::ResourceType::ROOT_SIGNATURE:
			case Rhi::ResourceType::RESOURCE_GROUP:
			case Rhi::ResourceType::GRAPHICS_PROGRAM:
			case Rhi::ResourceType::VERTEX_ARRAY:
			case Rhi::ResourceType::RENDER_PASS:
			case Rhi::ResourceType::QUERY_POOL:
			case Rhi::ResourceType::SWAP_CHAIN:
			case Rhi::ResourceType::FRAMEBUFFER:
			case Rhi::ResourceType::VERTEX_BUFFER:
			case Rhi::ResourceType::INDEX_BUFFER:
			case Rhi::ResourceType::TEXTURE_BUFFER:
			case Rhi::ResourceType::STRUCTURED_BUFFER:
			case Rhi::ResourceType::INDIRECT_BUFFER:
			case Rhi::ResourceType::UNIFORM_BUFFER:
			case Rhi::ResourceType::TEXTURE_1D:
			case Rhi::ResourceType::TEXTURE_1D_ARRAY:
			case Rhi::ResourceType::TEXTURE_2D_ARRAY:
			case Rhi::ResourceType::TEXTURE_3D:
			case Rhi::ResourceType::TEXTURE_CUBE:
			case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
			case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
			case Rhi::ResourceType::SAMPLER_STATE:
			case Rhi::ResourceType::VERTEX_SHADER:
			case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Rhi::ResourceType::GEOMETRY_SHADER:
			case Rhi::ResourceType::FRAGMENT_SHADER:
			case Rhi::ResourceType::TASK_SHADER:
			case Rhi::ResourceType::MESH_SHADER:
			case Rhi::ResourceType::COMPUTE_SHADER:
			default:
				// Not handled in here
				break;
		}
	}

	void OpenGLRhi::generateMipmaps(Rhi::IResource& resource)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, resource)
		RHI_ASSERT(mContext, resource.getResourceType() == Rhi::ResourceType::TEXTURE_2D, "TODO(co) Mipmaps can only be generated for OpenGL 2D texture resources")

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

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Backup the currently bound OpenGL texture
				// TODO(co) It's possible to avoid calling this multiple times
				GLint openGLTextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLTextureBackup);
			#endif

			// Generate mipmaps
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, texture2D.getOpenGLTexture());
			glGenerateMipmap(GL_TEXTURE_2D);

			#ifdef RHI_OPENGL_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL texture
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLTextureBackup));
			#endif
		}
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void OpenGLRhi::resetQueryPool([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, queryPool)
		RHI_ASSERT(mContext, firstQueryIndex < static_cast<const QueryPool&>(queryPool).getNumberOfQueries(), "OpenGL out-of-bounds query index")
		RHI_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= static_cast<const QueryPool&>(queryPool).getNumberOfQueries(), "OpenGL out-of-bounds query index")

		// Nothing to do in here for OpenGL
	}

	void OpenGLRhi::beginQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex, uint32_t)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& openGLQueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		switch (openGLQueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				// At this point in time we know that the "GL_ARB_occlusion_query"-extension is supported
				glBeginQueryARB(GL_SAMPLES_PASSED_ARB, static_cast<const OcclusionTimestampQueryPool&>(openGLQueryPool).getOpenGLQueries()[queryIndex]);
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				// At this point in time we know that the "GL_ARB_pipeline_statistics_query"-extension is supported
				static_cast<const PipelineStatisticsQueryPool&>(openGLQueryPool).beginQuery(queryIndex);
				break;

			case Rhi::QueryType::TIMESTAMP:
				RHI_ASSERT(mContext, false, "OpenGL begin query isn't allowed for timestamp queries, use \"Rhi::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void OpenGLRhi::endQuery(Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& openGLQueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		switch (openGLQueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				// At this point in time we know that the "GL_ARB_occlusion_query"-extension is supported
				glEndQueryARB(GL_SAMPLES_PASSED_ARB);
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				// At this point in time we know that the "GL_ARB_pipeline_statistics_query"-extension is supported
				static_cast<const PipelineStatisticsQueryPool&>(openGLQueryPool).endQuery();
				break;

			case Rhi::QueryType::TIMESTAMP:
				RHI_ASSERT(mContext, false, "OpenGL end query isn't allowed for timestamp queries, use \"Rhi::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void OpenGLRhi::writeTimestampQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& openGLQueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		switch (openGLQueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				RHI_ASSERT(mContext, false, "OpenGL write timestamp query isn't allowed for occlusion queries, use \"Rhi::Command::BeginQuery\" and \"Rhi::Command::EndQuery\" instead")
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				RHI_ASSERT(mContext, false, "OpenGL write timestamp query isn't allowed for pipeline statistics queries, use \"Rhi::Command::BeginQuery\" and \"Rhi::Command::EndQuery\" instead")
				break;

			case Rhi::QueryType::TIMESTAMP:
				// At this point in time we know that the "GL_ARB_timer_query"-extension is supported
				glQueryCounter(static_cast<const OcclusionTimestampQueryPool&>(openGLQueryPool).getOpenGLQueries()[queryIndex], GL_TIMESTAMP);
				break;
		}
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void OpenGLRhi::setDebugMarker(const char* name)
		{
			// "GL_KHR_debug"-extension required
			if (mExtensions->isGL_KHR_debug())
			{
				RHI_ASSERT(mContext, nullptr != name, "OpenGL debug marker names must not be a null pointer")
				glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, name);
			}
		}

		void OpenGLRhi::beginDebugEvent(const char* name)
		{
			// "GL_KHR_debug"-extension required
			if (mExtensions->isGL_KHR_debug())
			{
				RHI_ASSERT(mContext, nullptr != name, "OpenGL debug event names must not be a null pointer")
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, name);
			}
		}

		void OpenGLRhi::endDebugEvent()
		{
			// "GL_KHR_debug"-extension required
			if (mExtensions->isGL_KHR_debug())
			{
				glPopDebugGroup();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	const char* OpenGLRhi::getName() const
	{
		return "OpenGL";
	}

	bool OpenGLRhi::isInitialized() const
	{
		// Is the OpenGL context initialized?
		return (nullptr != mOpenGLContext && mOpenGLContext->isInitialized());
	}

	bool OpenGLRhi::isDebugEnabled()
	{
		// OpenGL has nothing that is similar to the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)

		// Debug disabled
		return false;
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t OpenGLRhi::getNumberOfShaderLanguages() const
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

	const char* OpenGLRhi::getShaderLanguageName(uint32_t index) const
	{
		RHI_ASSERT(mContext, index < getNumberOfShaderLanguages(), "OpenGL: Shader language index is out-of-bounds")

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

	Rhi::IShaderLanguage* OpenGLRhi::getShaderLanguage(const char* shaderLanguageName)
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
							mShaderLanguage = RHI_NEW(mContext, ShaderLanguageSeparate)(*this);
							mShaderLanguage->addReference();	// Internal RHI reference
						}

						// Return the shader language instance
						return mShaderLanguage;
					}
					else if (mExtensions->isGL_ARB_shader_objects())
					{
						// If required, create the monolithic shader language instance right now
						if (nullptr == mShaderLanguage)
						{
							mShaderLanguage = RHI_NEW(mContext, ShaderLanguageMonolithic)(*this);
							mShaderLanguage->addReference();	// Internal RHI reference
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
	Rhi::IRenderPass* OpenGLRhi::createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IQueryPool* OpenGLRhi::createQueryPool(Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		RHI_ASSERT(mContext, numberOfQueries > 0, "OpenGL: Number of queries mustn't be zero")
		switch (queryType)
		{
			case Rhi::QueryType::OCCLUSION:
				if (!mExtensions->isGL_ARB_occlusion_query())
				{
					RHI_LOG(mContext, CRITICAL, "OpenGL extension \"GL_ARB_occlusion_query\" isn't supported")
					return nullptr;
				}
				return RHI_NEW(mContext, OcclusionTimestampQueryPool)(*this, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER);

			case Rhi::QueryType::PIPELINE_STATISTICS:
				if (!mExtensions->isGL_ARB_pipeline_statistics_query())
				{
					RHI_LOG(mContext, CRITICAL, "OpenGL extension \"GL_ARB_pipeline_statistics_query\" isn't supported")
					return nullptr;
				}
				return RHI_NEW(mContext, PipelineStatisticsQueryPool)(*this, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER);

			case Rhi::QueryType::TIMESTAMP:
				if (!mExtensions->isGL_ARB_timer_query())
				{
					RHI_LOG(mContext, CRITICAL, "OpenGL extension \"GL_ARB_timer_query\" isn't supported")
					return nullptr;
				}
				return RHI_NEW(mContext, OcclusionTimestampQueryPool)(*this, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}
		return nullptr;
	}

	Rhi::ISwapChain* OpenGLRhi::createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool useExternalContext RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, renderPass)
		RHI_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle || nullptr != windowHandle.renderWindow, "OpenGL: The provided native window handle or render window must not be a null handle / null pointer")

		// Create the swap chain
		return RHI_NEW(mContext, SwapChain)(renderPass, windowHandle, useExternalContext RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IFramebuffer* OpenGLRhi::createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, renderPass)

		// "GL_ARB_framebuffer_object" required
		if (mExtensions->isGL_ARB_framebuffer_object())
		{
			// Is "GL_EXT_direct_state_access" there?
			if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
			{
				// Effective direct state access (DSA)
				// -> Validation is done inside the framebuffer implementation
				return RHI_NEW(mContext, FramebufferDsa)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else
			{
				// Traditional bind version
				// -> Validation is done inside the framebuffer implementation
				return RHI_NEW(mContext, FramebufferBind)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}
		else
		{
			// Error!
			return nullptr;
		}
	}

	Rhi::IBufferManager* OpenGLRhi::createBufferManager()
	{
		return RHI_NEW(mContext, BufferManager)(*this);
	}

	Rhi::ITextureManager* OpenGLRhi::createTextureManager()
	{
		return RHI_NEW(mContext, TextureManager)(*this);
	}

	Rhi::IRootSignature* OpenGLRhi::createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RootSignature)(*this, rootSignature RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IGraphicsPipelineState* OpenGLRhi::createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "OpenGL: Invalid graphics pipeline state root signature")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "OpenGL: Invalid graphics pipeline state graphics program")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "OpenGL: Invalid graphics pipeline state render pass")

		// Create graphics pipeline state
		uint16_t id = 0;
		if (GraphicsPipelineStateMakeId.CreateID(id))
		{
			return RHI_NEW(mContext, GraphicsPipelineState)(*this, graphicsPipelineState, id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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

	Rhi::IComputePipelineState* OpenGLRhi::createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, rootSignature)
		RHI_MATCH_CHECK(*this, computeShader)

		// Create the compute pipeline state
		uint16_t id = 0;
		if ((mExtensions->isGL_ARB_separate_shader_objects() || mExtensions->isGL_ARB_shader_objects()) && ComputePipelineStateMakeId.CreateID(id))
		{
			// Create the compute pipeline state
			// -> Prefer "GL_ARB_separate_shader_objects" over "GL_ARB_shader_objects"
			if (mExtensions->isGL_ARB_separate_shader_objects())
			{
				return RHI_NEW(mContext, ComputePipelineStateSeparate)(*this, rootSignature, static_cast<ComputeShaderSeparate&>(computeShader), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
			else if (mExtensions->isGL_ARB_shader_objects())
			{
				return RHI_NEW(mContext, ComputePipelineStateMonolithic)(*this, rootSignature, static_cast<ComputeShaderMonolithic&>(computeShader), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}
		}

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();
		return nullptr;
	}

	Rhi::ISamplerState* OpenGLRhi::createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Is "GL_ARB_sampler_objects" there?
		if (mExtensions->isGL_ARB_sampler_objects())
		{
			// Effective sampler object (SO)
			return RHI_NEW(mContext, SamplerStateSo)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		// Is "GL_EXT_direct_state_access" there?
		else if (mExtensions->isGL_EXT_direct_state_access() || mExtensions->isGL_ARB_direct_state_access())
		{
			// Direct state access (DSA) version to emulate a sampler object
			return RHI_NEW(mContext, SamplerStateDsa)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}
		else
		{
			// Traditional bind version to emulate a sampler object
			return RHI_NEW(mContext, SamplerStateBind)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool OpenGLRhi::map(Rhi::IResource& resource, uint32_t, Rhi::MapType mapType, uint32_t, Rhi::MappedSubresource& mappedSubresource)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_ARRAY_BUFFER_ARB, GL_ARRAY_BUFFER_BINDING_ARB, static_cast<VertexBuffer&>(resource).getOpenGLArrayBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::INDEX_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_ELEMENT_ARRAY_BUFFER_ARB, GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, static_cast<IndexBuffer&>(resource).getOpenGLElementArrayBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::TEXTURE_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<TextureBuffer&>(resource).getOpenGLTextureBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::STRUCTURED_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<StructuredBuffer&>(resource).getOpenGLStructuredBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::INDIRECT_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_DRAW_INDIRECT_BUFFER, GL_DRAW_INDIRECT_BUFFER_BINDING, static_cast<IndirectBuffer&>(resource).getOpenGLIndirectBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::UNIFORM_BUFFER:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, static_cast<UniformBuffer&>(resource).getOpenGLUniformBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::TEXTURE_1D:
			{
				// TODO(co) Implement me
				return false;
			}

			case Rhi::ResourceType::TEXTURE_1D_ARRAY:
			{
				// TODO(co) Implement me
				return false;
			}

			case Rhi::ResourceType::TEXTURE_2D:
			{
				bool result = false;

				// TODO(co) Implement me
				/*
				// Begin debug event
				RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

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
				RHI_END_DEBUG_EVENT(this)
				*/

				// Done
				return result;
			}

			case Rhi::ResourceType::TEXTURE_2D_ARRAY:
			{
				bool result = false;

				// TODO(co) Implement me
				/*
				// Begin debug event
				RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

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
				RHI_END_DEBUG_EVENT(this)
				*/

				// Done
				return result;
			}

			case Rhi::ResourceType::TEXTURE_3D:
				return ::detail::mapBuffer(mContext, *mExtensions, GL_PIXEL_UNPACK_BUFFER_ARB, GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, static_cast<Texture3D&>(resource).getOpenGLPixelUnpackBuffer(), mapType, mappedSubresource);

			case Rhi::ResourceType::TEXTURE_CUBE:
			{
				// TODO(co) Implement me
				return false;
			}

			case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
			{
				// TODO(co) Implement me
				return false;
			}

			case Rhi::ResourceType::ROOT_SIGNATURE:
			case Rhi::ResourceType::RESOURCE_GROUP:
			case Rhi::ResourceType::GRAPHICS_PROGRAM:
			case Rhi::ResourceType::VERTEX_ARRAY:
			case Rhi::ResourceType::RENDER_PASS:
			case Rhi::ResourceType::QUERY_POOL:
			case Rhi::ResourceType::SWAP_CHAIN:
			case Rhi::ResourceType::FRAMEBUFFER:
			case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
			case Rhi::ResourceType::SAMPLER_STATE:
			case Rhi::ResourceType::VERTEX_SHADER:
			case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Rhi::ResourceType::GEOMETRY_SHADER:
			case Rhi::ResourceType::FRAGMENT_SHADER:
			case Rhi::ResourceType::TASK_SHADER:
			case Rhi::ResourceType::MESH_SHADER:
			case Rhi::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can map, set known return values
				mappedSubresource.data		 = nullptr;
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;

				// Error!
				return false;
		}
	}

	void OpenGLRhi::unmap(Rhi::IResource& resource, uint32_t)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_ARRAY_BUFFER_ARB, GL_ARRAY_BUFFER_BINDING_ARB, static_cast<VertexBuffer&>(resource).getOpenGLArrayBuffer());
				break;

			case Rhi::ResourceType::INDEX_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_ELEMENT_ARRAY_BUFFER_ARB, GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, static_cast<IndexBuffer&>(resource).getOpenGLElementArrayBuffer());
				break;

			case Rhi::ResourceType::TEXTURE_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<TextureBuffer&>(resource).getOpenGLTextureBuffer());
				break;

			case Rhi::ResourceType::STRUCTURED_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_TEXTURE_BUFFER_ARB, GL_TEXTURE_BINDING_BUFFER_ARB, static_cast<StructuredBuffer&>(resource).getOpenGLStructuredBuffer());
				break;

			case Rhi::ResourceType::INDIRECT_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_DRAW_INDIRECT_BUFFER, GL_DRAW_INDIRECT_BUFFER_BINDING, static_cast<IndirectBuffer&>(resource).getOpenGLIndirectBuffer());
				break;

			case Rhi::ResourceType::UNIFORM_BUFFER:
				::detail::unmapBuffer(*mExtensions, GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, static_cast<UniformBuffer&>(resource).getOpenGLUniformBuffer());
				break;

			case Rhi::ResourceType::TEXTURE_1D:
			{
				// TODO(co) Implement me
				break;
			}

			case Rhi::ResourceType::TEXTURE_1D_ARRAY:
			{
				// TODO(co) Implement me
				break;
			}

			case Rhi::ResourceType::TEXTURE_2D:
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

			case Rhi::ResourceType::TEXTURE_2D_ARRAY:
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

			case Rhi::ResourceType::TEXTURE_3D:
			{
				// Unmap pixel unpack buffer
				const Texture3D& texture3D = static_cast<Texture3D&>(resource);
				const Rhi::TextureFormat::Enum textureFormat = texture3D.getTextureFormat();
				const uint32_t openGLPixelUnpackBuffer = texture3D.getOpenGLPixelUnpackBuffer();
				::detail::unmapBuffer(*mExtensions, GL_PIXEL_UNPACK_BUFFER_ARB, GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, openGLPixelUnpackBuffer);

				// Backup the currently set alignment and currently bound OpenGL pixel unpack buffer
				#ifdef RHI_OPENGL_STATE_CLEANUP
					GLint openGLAlignmentBackup = 0;
					glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLAlignmentBackup);
					GLint openGLUnpackBufferBackup = 0;
					glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &openGLUnpackBufferBackup);
				#endif
				glPixelStorei(GL_UNPACK_ALIGNMENT, (Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat) & 3) ? 1 : 4);

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
					#ifdef RHI_OPENGL_STATE_CLEANUP
						GLint openGLTextureBackup = 0;
						glGetIntegerv(GL_TEXTURE_BINDING_3D, &openGLTextureBackup);
					#endif

					// Copy pixel unpack buffer to texture
					glBindTexture(GL_TEXTURE_3D, texture3D.getOpenGLTexture());
					glTexSubImage3DEXT(GL_TEXTURE_3D, 0, 0, 0, 0, static_cast<GLsizei>(texture3D.getWidth()), static_cast<GLsizei>(texture3D.getHeight()), static_cast<GLsizei>(texture3D.getDepth()), Mapping::getOpenGLFormat(textureFormat), Mapping::getOpenGLType(textureFormat), 0);

					// Be polite and restore the previous bound OpenGL texture
					#ifdef RHI_OPENGL_STATE_CLEANUP
						glBindTexture(GL_TEXTURE_3D, static_cast<GLuint>(openGLTextureBackup));
					#endif
				}

				// Restore previous alignment and pixel unpack buffer
				#ifdef RHI_OPENGL_STATE_CLEANUP
					glPixelStorei(GL_UNPACK_ALIGNMENT, openGLAlignmentBackup);
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, static_cast<GLuint>(openGLUnpackBufferBackup));
				#else
					glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
				#endif
				break;
			}

			case Rhi::ResourceType::TEXTURE_CUBE:
			{
				// TODO(co) Implement me
				break;
			}

			case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
			{
				// TODO(co) Implement me
				break;
			}

			case Rhi::ResourceType::ROOT_SIGNATURE:
			case Rhi::ResourceType::RESOURCE_GROUP:
			case Rhi::ResourceType::GRAPHICS_PROGRAM:
			case Rhi::ResourceType::VERTEX_ARRAY:
			case Rhi::ResourceType::RENDER_PASS:
			case Rhi::ResourceType::QUERY_POOL:
			case Rhi::ResourceType::SWAP_CHAIN:
			case Rhi::ResourceType::FRAMEBUFFER:
			case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
			case Rhi::ResourceType::SAMPLER_STATE:
			case Rhi::ResourceType::VERTEX_SHADER:
			case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Rhi::ResourceType::GEOMETRY_SHADER:
			case Rhi::ResourceType::FRAGMENT_SHADER:
			case Rhi::ResourceType::TASK_SHADER:
			case Rhi::ResourceType::MESH_SHADER:
			case Rhi::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can unmap
				break;
		}
	}

	bool OpenGLRhi::getQueryPoolResults(Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, uint32_t strideInBytes, uint32_t queryResultFlags)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, queryPool)
		RHI_ASSERT(mContext, numberOfDataBytes >= sizeof(UINT64), "OpenGL out-of-memory query access")
		RHI_ASSERT(mContext, 1 == numberOfQueries || strideInBytes > 0, "OpenGL invalid stride in bytes")
		RHI_ASSERT(mContext, numberOfDataBytes >= strideInBytes * numberOfQueries, "OpenGL out-of-memory query access")
		RHI_ASSERT(mContext, nullptr != data, "OpenGL out-of-memory query access")
		RHI_ASSERT(mContext, numberOfQueries > 0, "OpenGL number of queries mustn't be zero")

		// Query pool type dependent processing
		bool resultAvailable = true;
		const QueryPool& openGLQueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, firstQueryIndex < openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		RHI_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= openGLQueryPool.getNumberOfQueries(), "OpenGL out-of-bounds query index")
		const bool waitForResult = ((queryResultFlags & Rhi::QueryResultFlags::WAIT) != 0);
		switch (openGLQueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
			case Rhi::QueryType::TIMESTAMP:	// OpenGL return the time in nanoseconds
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

			case Rhi::QueryType::PIPELINE_STATISTICS:
				RHI_ASSERT(mContext, numberOfDataBytes >= sizeof(Rhi::PipelineStatisticsQueryResult), "OpenGL out-of-memory query access")
				RHI_ASSERT(mContext, 1 == numberOfQueries || strideInBytes >= sizeof(Rhi::PipelineStatisticsQueryResult), "OpenGL out-of-memory query access")
				resultAvailable = static_cast<const PipelineStatisticsQueryPool&>(openGLQueryPool).getQueryPoolResults(data, firstQueryIndex, numberOfQueries, strideInBytes, waitForResult);
				break;
		}

		// Done
		return resultAvailable;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool OpenGLRhi::beginScene()
	{
		// Not required when using OpenGL

		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, false == mDebugBetweenBeginEndScene, "OpenGL: Begin scene was called while scene rendering is already in progress, missing end scene call?")
			mDebugBetweenBeginEndScene = true;
		#endif

		// Done
		return true;
	}

	void OpenGLRhi::submitCommandBuffer(const Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(mContext, !commandBuffer.isEmpty(), "The OpenGL command buffer to execute mustn't be empty")

		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Rhi::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Submit command packet
				const Rhi::CommandDispatchFunctionIndex commandDispatchFunctionIndex = Rhi::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket);
				const void* command = Rhi::CommandPacketHelper::loadCommand(constCommandPacket);
				detail::DISPATCH_FUNCTIONS[static_cast<uint32_t>(commandDispatchFunctionIndex)](command, *this);
			}

			{ // Next command
				const uint32_t nextCommandPacketByteIndex = Rhi::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
				constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
			}
		}
	}

	void OpenGLRhi::endScene()
	{
		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, true == mDebugBetweenBeginEndScene, "OpenGL: End scene was called while scene rendering isn't in progress, missing start scene call?")
			mDebugBetweenBeginEndScene = false;
		#endif
	}


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void OpenGLRhi::debugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int, const char* message, const void* userParam)
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
			Rhi::ILog::Type logType = Rhi::ILog::Type::CRITICAL;
			char debugType[25 + 1]{0};	// +1 for terminating zero
			switch (type)
			{
				case GL_DEBUG_TYPE_ERROR_ARB:
					strncpy(debugType, "Error", 25);
					break;

				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
					logType = Rhi::ILog::Type::COMPATIBILITY_WARNING;
					strncpy(debugType, "Deprecated behavior", 25);
					break;

				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
					strncpy(debugType, "Undefined behavior", 25);
					break;

				case GL_DEBUG_TYPE_PORTABILITY_ARB:
					logType = Rhi::ILog::Type::COMPATIBILITY_WARNING;
					strncpy(debugType, "Portability", 25);
					break;

				case GL_DEBUG_TYPE_PERFORMANCE_ARB:
					logType = Rhi::ILog::Type::PERFORMANCE_WARNING;
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
			if (static_cast<const OpenGLRhi*>(userParam)->getContext().getLog().print(logType, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), "OpenGL debug message\tSource:\"%s\"\tType:\"%s\"\tID:\"%u\"\tSeverity:\"%s\"\tMessage:\"%s\"", debugSource, debugType, id, debugSeverity, message))
			{
				DEBUG_BREAK;
			}
		}
	#else
		void OpenGLRhi::debugMessageCallback(uint32_t, uint32_t, uint32_t, uint32_t, int, const char*, const void*)
		{
			// Nothing here
		}
	#endif


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void OpenGLRhi::initializeCapabilities()
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
		mCapabilities.preferredSwapChainColorTextureFormat		  = Rhi::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		// TODO(co) "GL_ARB_viewport_array" support ("OpenGLRhi::setGraphicsViewports()")
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
		// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
		if (mExtensions->isGL_EXT_texture_array())
		{
			glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &openGLValue);
			mCapabilities.maximumNumberOf1DTextureArraySlices = static_cast<uint32_t>(openGLValue);
			mCapabilities.maximumNumberOf2DTextureArraySlices = static_cast<uint32_t>(openGLValue);
			mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;	// TODO(co) Implement me		 static_cast<uint32_t>(openGLValue);
		}
		else
		{
			mCapabilities.maximumNumberOf1DTextureArraySlices = 0;
			mCapabilities.maximumNumberOf2DTextureArraySlices = 0;
			mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;
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

		// Is there support for task shaders (TS) and mesh shaders (MS)?
		mCapabilities.meshShader = mExtensions->isGL_NV_mesh_shader();

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = mExtensions->isGL_ARB_compute_shader();
	}

	void OpenGLRhi::unsetGraphicsVertexArray()
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

	void OpenGLRhi::setResourceGroup(const RootSignature& rootSignature, uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Set resource group
			const ResourceGroup* openGLResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = openGLResourceGroup->getNumberOfResources();
			Rhi::IResource** resources = openGLResourceGroup->getResources();
			const Rhi::RootParameter& rootParameter = rootSignature.getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				Rhi::IResource* resource = *resources;
				RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL descriptor ranges")
				const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Rhi::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_2D:
					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					{
						// In OpenGL, all shaders share the same texture units (= "Rhi::RootParameter::shaderVisibility" stays unused)
						switch (descriptorRange.rangeType)
						{
							case Rhi::DescriptorRangeType::SRV:
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
										case Rhi::ResourceType::TEXTURE_BUFFER:
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

										case Rhi::ResourceType::TEXTURE_1D:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture1D*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_1D, static_cast<Texture1D*>(resource)->getOpenGLTexture());
											}
											break;

										case Rhi::ResourceType::TEXTURE_1D_ARRAY:
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

										case Rhi::ResourceType::TEXTURE_2D:
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

										case Rhi::ResourceType::TEXTURE_2D_ARRAY:
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

										case Rhi::ResourceType::TEXTURE_3D:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<Texture3D*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_3D, static_cast<Texture3D*>(resource)->getOpenGLTexture());
											}
											break;

										case Rhi::ResourceType::TEXTURE_CUBE:
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<TextureCube*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_CUBE_MAP, static_cast<TextureCube*>(resource)->getOpenGLTexture());
											}
											break;

										case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
											// TODO(co) Implement me
											/*
											if (isArbDsa)
											{
												glBindTextureUnit(unit, static_cast<TextureCubeArray*>(resource)->getOpenGLTexture());
											}
											else
											{
												// "GL_TEXTURE0_ARB" is the first texture unit, while the unit we received is zero based
												glBindMultiTextureEXT(GL_TEXTURE0_ARB + unit, GL_TEXTURE_CUBE_MAP, static_cast<TextureCubeArray*>(resource)->getOpenGLTexture());
											}
											*/
											break;

										case Rhi::ResourceType::ROOT_SIGNATURE:
										case Rhi::ResourceType::RESOURCE_GROUP:
										case Rhi::ResourceType::GRAPHICS_PROGRAM:
										case Rhi::ResourceType::VERTEX_ARRAY:
										case Rhi::ResourceType::RENDER_PASS:
										case Rhi::ResourceType::QUERY_POOL:
										case Rhi::ResourceType::SWAP_CHAIN:
										case Rhi::ResourceType::FRAMEBUFFER:
										case Rhi::ResourceType::VERTEX_BUFFER:
										case Rhi::ResourceType::INDEX_BUFFER:
										case Rhi::ResourceType::STRUCTURED_BUFFER:
										case Rhi::ResourceType::INDIRECT_BUFFER:
										case Rhi::ResourceType::UNIFORM_BUFFER:
										case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
										case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
										case Rhi::ResourceType::SAMPLER_STATE:
										case Rhi::ResourceType::VERTEX_SHADER:
										case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
										case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
										case Rhi::ResourceType::GEOMETRY_SHADER:
										case Rhi::ResourceType::FRAGMENT_SHADER:
										case Rhi::ResourceType::TASK_SHADER:
										case Rhi::ResourceType::MESH_SHADER:
										case Rhi::ResourceType::COMPUTE_SHADER:
											RHI_ASSERT(mContext, false, "Invalid OpenGL RHI implementation resource type")
											break;
									}

									// Set the OpenGL sampler states, if required (texture buffer has no sampler state), it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
									if (Rhi::ResourceType::TEXTURE_BUFFER != resourceType && nullptr != openGLResourceGroup->getSamplerState())
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
												#ifdef RHI_OPENGL_STATE_CLEANUP
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

												#ifdef RHI_OPENGL_STATE_CLEANUP
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
										#ifdef RHI_OPENGL_STATE_CLEANUP
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
											case Rhi::ResourceType::TEXTURE_BUFFER:
												glBindTexture(GL_TEXTURE_BUFFER_ARB, static_cast<TextureBuffer*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::TEXTURE_1D:
												glBindTexture(GL_TEXTURE_1D, static_cast<Texture1D*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::TEXTURE_1D_ARRAY:
												// No extension check required, if we in here we already know it must exist
												glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, static_cast<Texture1DArray*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::TEXTURE_2D:
											{
												const Texture2D* texture2D = static_cast<Texture2D*>(resource);
												glBindTexture(static_cast<GLenum>((texture2D->getNumberOfMultisamples() > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D), texture2D->getOpenGLTexture());
												break;
											}

											case Rhi::ResourceType::TEXTURE_2D_ARRAY:
												// No extension check required, if we in here we already know it must exist
												glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, static_cast<Texture2DArray*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::TEXTURE_3D:
												glBindTexture(GL_TEXTURE_3D, static_cast<Texture3D*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::TEXTURE_CUBE:
												glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<TextureCube*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
												// TODO(co) Implement me
												// glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<TextureCubeArray*>(resource)->getOpenGLTexture());
												break;

											case Rhi::ResourceType::ROOT_SIGNATURE:
											case Rhi::ResourceType::RESOURCE_GROUP:
											case Rhi::ResourceType::GRAPHICS_PROGRAM:
											case Rhi::ResourceType::VERTEX_ARRAY:
											case Rhi::ResourceType::RENDER_PASS:
											case Rhi::ResourceType::QUERY_POOL:
											case Rhi::ResourceType::SWAP_CHAIN:
											case Rhi::ResourceType::FRAMEBUFFER:
											case Rhi::ResourceType::VERTEX_BUFFER:
											case Rhi::ResourceType::INDEX_BUFFER:
											case Rhi::ResourceType::STRUCTURED_BUFFER:
											case Rhi::ResourceType::INDIRECT_BUFFER:
											case Rhi::ResourceType::UNIFORM_BUFFER:
											case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
											case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
											case Rhi::ResourceType::SAMPLER_STATE:
											case Rhi::ResourceType::VERTEX_SHADER:
											case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
											case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
											case Rhi::ResourceType::GEOMETRY_SHADER:
											case Rhi::ResourceType::FRAGMENT_SHADER:
											case Rhi::ResourceType::TASK_SHADER:
											case Rhi::ResourceType::MESH_SHADER:
											case Rhi::ResourceType::COMPUTE_SHADER:
												RHI_ASSERT(mContext, false, "Invalid OpenGL RHI implementation resource type")
												break;
										}

										// Set the OpenGL sampler states, if required (texture buffer has no sampler state), it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
										if (Rhi::ResourceType::TEXTURE_BUFFER != resourceType)
										{
											RHI_ASSERT(mContext, nullptr != openGLResourceGroup->getSamplerState(), "Invalid OpenGL sampler state")
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

										#ifdef RHI_OPENGL_STATE_CLEANUP
											// Be polite and restore the previous active OpenGL texture
											glActiveTextureARB(static_cast<GLenum>(openGLActiveTextureBackup));
										#endif
									}
								}
								break;
							}

							case Rhi::DescriptorRangeType::UAV:
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
										case Rhi::ResourceType::TEXTURE_BUFFER:
										{
											const TextureBuffer* textureBuffer = static_cast<TextureBuffer*>(resource);
											glBindImageTextureEXT(unit, textureBuffer->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(textureBuffer->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_1D:
										{
											const Texture1D* texture1D = static_cast<Texture1D*>(resource);
											glBindImageTextureEXT(unit, texture1D->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture1D->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_1D_ARRAY:
										{
											const Texture1DArray* texture1DArray = static_cast<Texture1DArray*>(resource);
											glBindImageTextureEXT(unit, texture1DArray->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture1DArray->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_2D:
										{
											const Texture2D* texture2D = static_cast<Texture2D*>(resource);
											glBindImageTextureEXT(unit, texture2D->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture2D->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_2D_ARRAY:
										{
											const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(resource);
											glBindImageTextureEXT(unit, texture2DArray->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture2DArray->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_3D:
										{
											const Texture3D* texture3D = static_cast<Texture3D*>(resource);
											glBindImageTextureEXT(unit, texture3D->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(texture3D->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_CUBE:
										{
											const TextureCube* textureCube = static_cast<TextureCube*>(resource);
											glBindImageTextureEXT(unit, textureCube->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(textureCube->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
										{
											// TODO(co) Implement me
											// const TextureCubeArray* textureCubeArray = static_cast<TextureCubeArray*>(resource);
											// glBindImageTextureEXT(unit, textureCubeArray->getOpenGLTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, static_cast<GLint>(textureCubeArray->getOpenGLInternalFormat()));
											break;
										}

										case Rhi::ResourceType::ROOT_SIGNATURE:
										case Rhi::ResourceType::RESOURCE_GROUP:
										case Rhi::ResourceType::GRAPHICS_PROGRAM:
										case Rhi::ResourceType::VERTEX_ARRAY:
										case Rhi::ResourceType::RENDER_PASS:
										case Rhi::ResourceType::QUERY_POOL:
										case Rhi::ResourceType::SWAP_CHAIN:
										case Rhi::ResourceType::FRAMEBUFFER:
										case Rhi::ResourceType::VERTEX_BUFFER:
										case Rhi::ResourceType::INDEX_BUFFER:
										case Rhi::ResourceType::STRUCTURED_BUFFER:
										case Rhi::ResourceType::INDIRECT_BUFFER:
										case Rhi::ResourceType::UNIFORM_BUFFER:
										case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
										case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
										case Rhi::ResourceType::SAMPLER_STATE:
										case Rhi::ResourceType::VERTEX_SHADER:
										case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
										case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
										case Rhi::ResourceType::GEOMETRY_SHADER:
										case Rhi::ResourceType::FRAGMENT_SHADER:
										case Rhi::ResourceType::TASK_SHADER:
										case Rhi::ResourceType::MESH_SHADER:
										case Rhi::ResourceType::COMPUTE_SHADER:
											RHI_ASSERT(mContext, false, "Invalid OpenGL RHI implementation resource type")
											break;
									}
								}
								break;
							}

							case Rhi::DescriptorRangeType::UBV:
							case Rhi::DescriptorRangeType::SAMPLER:
							case Rhi::DescriptorRangeType::NUMBER_OF_RANGE_TYPES:
								RHI_ASSERT(mContext, false, "Invalid OpenGL descriptor range type")
								break;
						}
						break;
					}

					case Rhi::ResourceType::VERTEX_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL vertex buffer must bound at SRV or UAV descriptor range type")
						RHI_ASSERT(mContext, Rhi::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Rhi::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "OpenGL descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")

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

					case Rhi::ResourceType::INDEX_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL index buffer must bound at SRV or UAV descriptor range type")
						RHI_ASSERT(mContext, Rhi::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Rhi::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "OpenGL descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")

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

					case Rhi::ResourceType::STRUCTURED_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL structured buffer must bound at SRV or UAV descriptor range type")

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

					case Rhi::ResourceType::INDIRECT_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "OpenGL indirect buffer must bound at SRV or UAV descriptor range type")
						RHI_ASSERT(mContext, Rhi::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Rhi::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "OpenGL descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")

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

					case Rhi::ResourceType::UNIFORM_BUFFER:
						// "GL_ARB_uniform_buffer_object" required
						if (mExtensions->isGL_ARB_uniform_buffer_object())
						{
							// Attach the buffer to the given UBO binding point
							// -> Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension
							// -> Direct3D 10 and Direct3D 11 have explicit binding points
							RHI_ASSERT(mContext, Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType, "OpenGL uniform buffer must bound at UBV descriptor range type")
							RHI_ASSERT(mContext, nullptr != openGLResourceGroup->getResourceIndexToUniformBlockBindingIndex(), "Invalid OpenGL resource index to uniform block binding index")
							glBindBufferBase(GL_UNIFORM_BUFFER, openGLResourceGroup->getResourceIndexToUniformBlockBindingIndex()[resourceIndex], static_cast<UniformBuffer*>(resource)->getOpenGLUniformBuffer());
						}
						break;

					case Rhi::ResourceType::SAMPLER_STATE:
						// Unlike Direct3D >=10, OpenGL directly attaches the sampler settings to the texture (unless the sampler object extension is used)
						break;

					case Rhi::ResourceType::ROOT_SIGNATURE:
					case Rhi::ResourceType::RESOURCE_GROUP:
					case Rhi::ResourceType::GRAPHICS_PROGRAM:
					case Rhi::ResourceType::VERTEX_ARRAY:
					case Rhi::ResourceType::RENDER_PASS:
					case Rhi::ResourceType::QUERY_POOL:
					case Rhi::ResourceType::SWAP_CHAIN:
					case Rhi::ResourceType::FRAMEBUFFER:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::TASK_SHADER:
					case Rhi::ResourceType::MESH_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
						RHI_ASSERT(mContext, false, "Invalid OpenGL RHI implementation resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void OpenGLRhi::setOpenGLGraphicsProgram(Rhi::IGraphicsProgram* graphicsProgram)
	{
		if (nullptr != graphicsProgram)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsProgram)

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

	void OpenGLRhi::setOpenGLComputePipelineState(ComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *computePipelineState)

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

	void OpenGLRhi::updateGL_ARB_base_instanceEmulation(uint32_t startInstanceLocation)
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
} // OpenGLRhi




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RHI_OPENGL_EXPORTS
	#define OPENGLRHI_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define OPENGLRHI_FUNCTION_EXPORT
#endif
OPENGLRHI_FUNCTION_EXPORT Rhi::IRhi* createOpenGLRhiInstance(const Rhi::Context& context)
{
	return RHI_NEW(context, OpenGLRhi::OpenGLRhi)(context);
}
#undef OPENGLRHI_FUNCTION_EXPORT
