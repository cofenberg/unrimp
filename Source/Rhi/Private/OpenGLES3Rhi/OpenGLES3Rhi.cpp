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
*    OpenGL ES 3 RHI amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    - OpenGL ES 3 capable graphics driver or emulator
*    - EGL, GLES3 and KHR headers which can be found at "<unrimp>\External\Rhi\OpenGLES\include\"
*
*    == Preprocessor Definitions ==
*    - Set "RHI_OPENGLES3_EXPORTS" as preprocessor definition when building this library as shared library
*    - If this RHI was compiled with "RHI_OPENGLES3_STATE_CLEANUP" set as preprocessor definition, the previous OpenGL ES 3 state will be restored after performing an operation (worse performance, increases the binary size slightly, might avoid unexpected behaviour when using OpenGL ES 3 directly beside this RHI)
*    - Do also have a look into the RHI header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

#define GL_GLES_PROTOTYPES 0
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
PRAGMA_WARNING_POP

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
#endif

#ifdef __ANDROID__
	#include <android/native_window.h>	// For "ANativeWindow_setBuffersGeometry()"
#endif

#if defined LINUX || defined(__ANDROID__)
	// Get rid of some nasty OS macros
	#undef None	// Linux: Undefine "None", this name is used inside enums defined by Unrimp (which gets defined inside "Xlib.h" pulled in by "egl.h")

	#include <dlfcn.h>	// For "dlopen()", "dlclose()" and so on
	#include <link.h>	// For getting the path to the library (for the error message)
#endif




//[-------------------------------------------------------]
//[ OpenGLES3Rhi/MakeID.h                                 ]
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
namespace OpenGLES3Rhi
{
	class VertexArray;
	class IExtensions;
	class RootSignature;
	class IOpenGLES3Context;
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
		RHI_ASSERT(mContext, &rhiReference == &(resourceReference).getRhi(), "OpenGL ES 3 error: The given resource is owned by another RHI instance")

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER , [[maybe_unused]] const char debugName[] = ""
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT , [[maybe_unused]] const char debugName[]
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
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT
#endif




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
		static constexpr const char* GLSLES_NAME = "GLSLES";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)


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


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace OpenGLES3Rhi
{




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/OpenGLES3ContextRuntimeLinking.h         ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Type definitions                                      ]
	//[-------------------------------------------------------]
	#ifndef GLchar
		#define GLchar char
	#endif
	#ifndef GLenum
		#define GLenum GLuint
	#endif

	//[-------------------------------------------------------]
	//[ EGL functions                                         ]
	//[-------------------------------------------------------]
	#define FNDEF_EGL(retType, funcName, args) retType (EGLAPIENTRY *funcPtr_##funcName) args
	FNDEF_EGL(void*,		eglGetProcAddress,		(const char* procname));
	FNDEF_EGL(EGLint,		eglGetError,			(void));
	FNDEF_EGL(EGLDisplay,	eglGetDisplay,			(NativeDisplayType display));
	FNDEF_EGL(EGLBoolean,	eglInitialize,			(EGLDisplay dpy, EGLint* major, EGLint* minor));
	FNDEF_EGL(EGLBoolean,	eglTerminate,			(EGLDisplay dpy));
	FNDEF_EGL(const char*,	eglQueryString,			(EGLDisplay dpy, EGLint name));
	FNDEF_EGL(EGLBoolean,	eglGetConfigs,			(EGLDisplay dpy, EGLConfig* configs, EGLint config_size, EGLint* num_config));
	FNDEF_EGL(EGLBoolean,	eglChooseConfig,		(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config));
	FNDEF_EGL(EGLBoolean,	eglGetConfigAttrib,		(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value));
	FNDEF_EGL(EGLSurface,	eglCreateWindowSurface,	(EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint* attrib_list));
	FNDEF_EGL(EGLBoolean,	eglDestroySurface,		(EGLDisplay dpy, EGLSurface surface));
	FNDEF_EGL(EGLBoolean,	eglQuerySurface,		(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value));
	FNDEF_EGL(EGLBoolean,	eglBindAPI,				(EGLenum api));
	FNDEF_EGL(EGLenum,		eglQueryAPI,			(void));
	FNDEF_EGL(EGLBoolean,	eglWaitClient,			(void));
	FNDEF_EGL(EGLBoolean,	eglReleaseThread,		(void));
	FNDEF_EGL(EGLBoolean,	eglSurfaceAttrib,		(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value));
	FNDEF_EGL(EGLBoolean,	eglBindTexImage,		(EGLDisplay dpy, EGLSurface surface, EGLint buffer));
	FNDEF_EGL(EGLBoolean,	eglReleaseTexImage,		(EGLDisplay dpy, EGLSurface surface, EGLint buffer));
	FNDEF_EGL(EGLBoolean,	eglSwapInterval,		(EGLDisplay dpy, EGLint interval));
	FNDEF_EGL(EGLContext,	eglCreateContext,		(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint* attrib_list));
	FNDEF_EGL(EGLBoolean,	eglDestroyContext,		(EGLDisplay dpy, EGLContext ctx));
	FNDEF_EGL(EGLBoolean,	eglMakeCurrent,			(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx));
	FNDEF_EGL(EGLContext,	eglGetCurrentContext,	(void));
	FNDEF_EGL(EGLSurface,	eglGetCurrentSurface,	(EGLint readdraw));
	FNDEF_EGL(EGLDisplay,	eglGetCurrentDisplay,	(void));
	FNDEF_EGL(EGLBoolean,	eglQueryContext,		(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value));
	FNDEF_EGL(EGLBoolean,	eglWaitGL,				(void));
	FNDEF_EGL(EGLBoolean,	eglWaitNative,			(EGLint engine));
	FNDEF_EGL(EGLBoolean,	eglSwapBuffers,			(EGLDisplay dpy, EGLSurface draw));
	FNDEF_EGL(EGLBoolean,	eglCopyBuffers,			(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target));
	#undef FNDEF_EGL

	//[-------------------------------------------------------]
	//[ GL core functions                                     ]
	//[-------------------------------------------------------]
	#define FNDEF_GL(retType, funcName, args) retType (GL_APIENTRY *funcPtr_##funcName) args
	FNDEF_GL(void,				glActiveTexture,						(GLenum texture));
	FNDEF_GL(void,				glAttachShader,							(GLuint program, GLuint shader));
	FNDEF_GL(void,				glBindAttribLocation,					(GLuint program, GLuint index, const GLchar* name));
	FNDEF_GL(void,				glBindBuffer,							(GLenum target, GLuint buffer));
	FNDEF_GL(void,				glBindFramebuffer,						(GLenum target, GLuint framebuffer));
	FNDEF_GL(void,				glBindRenderbuffer,						(GLenum target, GLuint renderbuffer));
	FNDEF_GL(void,				glBindTexture,							(GLenum target, GLuint texture));
	FNDEF_GL(void,				glBlendColor,							(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha));
	FNDEF_GL(void,				glBlendEquation,						(GLenum mode));
	FNDEF_GL(void,				glBlendEquationSeparate,				(GLenum modeRGB, GLenum modeAlpha));
	FNDEF_GL(void,				glBlendFunc,							(GLenum sfactor, GLenum dfactor));
	FNDEF_GL(void,				glBlendFuncSeparate,					(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha));
	FNDEF_GL(void,				glBufferData,							(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage));
	FNDEF_GL(void,				glBufferSubData,						(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data));
	FNDEF_GL(GLenum,			glCheckFramebufferStatus,				(GLenum target));
	FNDEF_GL(void,				glClear,								(GLbitfield mask));
	FNDEF_GL(void,				glClearColor,							(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha));
	FNDEF_GL(void,				glClearDepthf,							(GLclampf depth));
	FNDEF_GL(void,				glClearStencil,							(GLint s));
	FNDEF_GL(void,				glColorMask,							(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha));
	FNDEF_GL(void,				glCompileShader,						(GLuint shader));
	FNDEF_GL(void,				glCompressedTexImage2D,					(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data));
	FNDEF_GL(void,				glCompressedTexImage3D,					(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data));
	FNDEF_GL(void,				glCompressedTexSubImage2D,				(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data));
	FNDEF_GL(void,				glCopyTexImage2D,						(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border));
	FNDEF_GL(void,				glCopyTexSubImage2D,					(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height));
	FNDEF_GL(GLuint,			glCreateProgram,						(void));
	FNDEF_GL(GLuint,			glCreateShader,							(GLenum type));
	FNDEF_GL(void,				glCullFace,								(GLenum mode));
	FNDEF_GL(void,				glDeleteBuffers,						(GLsizei n, const GLuint* buffers));
	FNDEF_GL(void,				glDeleteFramebuffers,					(GLsizei n, const GLuint* framebuffers));
	FNDEF_GL(void,				glDeleteProgram,						(GLuint program));
	FNDEF_GL(void,				glDeleteRenderbuffers,					(GLsizei n, const GLuint* renderbuffers));
	FNDEF_GL(void,				glDeleteShader,							(GLuint shader));
	FNDEF_GL(void,				glDeleteTextures,						(GLsizei n, const GLuint* textures));
	FNDEF_GL(void,				glDepthFunc,							(GLenum func));
	FNDEF_GL(void,				glDepthMask,							(GLboolean flag));
	FNDEF_GL(void,				glDepthRangef,							(GLclampf zNear, GLclampf zFar));
	FNDEF_GL(void,				glDetachShader,							(GLuint program, GLuint shader));
	FNDEF_GL(void,				glDisable,								(GLenum cap));
	FNDEF_GL(void,				glDisableVertexAttribArray,				(GLuint index));
	FNDEF_GL(void,				glDrawArrays,							(GLenum mode, GLint first, GLsizei count));
	FNDEF_GL(void,				glDrawArraysInstanced,					(GLenum mode, GLint first, GLsizei count, GLsizei primcount));
	FNDEF_GL(void,				glDrawElements,							(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices));
	FNDEF_GL(void,				glDrawElementsInstanced,				(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount));
	FNDEF_GL(void,				glEnable,								(GLenum cap));
	FNDEF_GL(void,				glEnableVertexAttribArray,				(GLuint index));
	FNDEF_GL(void,				glFinish,								(void));
	FNDEF_GL(void,				glFlush,								(void));
	FNDEF_GL(void,				glFramebufferRenderbuffer,				(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer));
	FNDEF_GL(void,				glFramebufferTexture2D,					(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level));
	FNDEF_GL(void,				glFramebufferTextureLayer,				(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer));
	FNDEF_GL(void,				glBlitFramebuffer,						(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter));
	FNDEF_GL(void,				glFrontFace,							(GLenum mode));
	FNDEF_GL(void,				glGenBuffers,							(GLsizei n, GLuint* buffers));
	FNDEF_GL(void,				glGenerateMipmap,						(GLenum target));
	FNDEF_GL(void,				glGenFramebuffers,						(GLsizei n, GLuint* framebuffers));
	FNDEF_GL(void,				glGenRenderbuffers,						(GLsizei n, GLuint* renderbuffers));
	FNDEF_GL(void,				glGenTextures,							(GLsizei n, GLuint* textures));
	FNDEF_GL(void,				glGetActiveAttrib,						(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name));
	FNDEF_GL(void,				glGetActiveUniform,						(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name));
	FNDEF_GL(void,				glGetAttachedShaders,					(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders));
	FNDEF_GL(int,				glGetAttribLocation,					(GLuint program, const GLchar* name));
	FNDEF_GL(void,				glGetBooleanv,							(GLenum pname, GLboolean* params));
	FNDEF_GL(void,				glGetBufferParameteriv,					(GLenum target, GLenum pname, GLint* params));
	FNDEF_GL(GLenum,			glGetError,								(void));
	FNDEF_GL(void,				glGetFloatv,							(GLenum pname, GLfloat* params));
	FNDEF_GL(void,				glGetFramebufferAttachmentParameteriv,	(GLenum target, GLenum attachment, GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetIntegerv,							(GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetProgramiv,							(GLuint program, GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetProgramInfoLog,					(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog));
	FNDEF_GL(void,				glGetRenderbufferParameteriv,			(GLenum target, GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetShaderiv,							(GLuint shader, GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetShaderInfoLog,						(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog));
	FNDEF_GL(void,				glGetShaderPrecisionFormat,				(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision));
	FNDEF_GL(void,				glGetShaderSource,						(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source));
	FNDEF_GL(const GLubyte*,	glGetString,							(GLenum name));
	FNDEF_GL(void,				glGetTexParameterfv,					(GLenum target, GLenum pname, GLfloat* params));
	FNDEF_GL(void,				glGetTexParameteriv,					(GLenum target, GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetUniformfv,							(GLuint program, GLint location, GLfloat* params));
	FNDEF_GL(void,				glGetUniformiv,							(GLuint program, GLint location, GLint* params));
	FNDEF_GL(int,				glGetUniformLocation,					(GLuint program, const GLchar* name));
	FNDEF_GL(GLuint,			glGetUniformBlockIndex,					(GLuint program, const GLchar *uniformBlockName));
	FNDEF_GL(void,				glUniformBlockBinding,					(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding));
	FNDEF_GL(void,				glGetVertexAttribfv,					(GLuint index, GLenum pname, GLfloat* params));
	FNDEF_GL(void,				glGetVertexAttribiv,					(GLuint index, GLenum pname, GLint* params));
	FNDEF_GL(void,				glGetVertexAttribPointerv,				(GLuint index, GLenum pname, GLvoid** pointer));
	FNDEF_GL(void,				glHint,									(GLenum target, GLenum mode));
	FNDEF_GL(GLboolean,			glIsBuffer,								(GLuint buffer));
	FNDEF_GL(GLboolean,			glIsEnabled,							(GLenum cap));
	FNDEF_GL(GLboolean,			glIsFramebuffer,						(GLuint framebuffer));
	FNDEF_GL(GLboolean,			glIsProgram,							(GLuint program));
	FNDEF_GL(GLboolean,			glIsRenderbuffer,						(GLuint renderbuffer));
	FNDEF_GL(GLboolean,			glIsShader,								(GLuint shader));
	FNDEF_GL(GLboolean,			glIsTexture,							(GLuint texture));
	FNDEF_GL(void,				glLineWidth,							(GLfloat width));
	FNDEF_GL(void,				glLinkProgram,							(GLuint program));
	FNDEF_GL(void,				glPixelStorei,							(GLenum pname, GLint param));
	FNDEF_GL(void,				glPolygonOffset,						(GLfloat factor, GLfloat units));
	FNDEF_GL(void,				glReadPixels,							(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels));
	FNDEF_GL(void,				glReleaseShaderCompiler,				(void));
	FNDEF_GL(void,				glRenderbufferStorage,					(GLenum target, GLenum internalformat, GLsizei width, GLsizei height));
	FNDEF_GL(void,				glSampleCoverage,						(GLclampf value, GLboolean invert));
	FNDEF_GL(void,				glScissor,								(GLint x, GLint y, GLsizei width, GLsizei height));
	FNDEF_GL(void,				glShaderBinary,							(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length));
	FNDEF_GL(void,				glShaderSource,							(GLuint shader, GLsizei count, const GLchar** string, const GLint* length));
	FNDEF_GL(void,				glStencilFunc,							(GLenum func, GLint ref, GLuint mask));
	FNDEF_GL(void,				glStencilFuncSeparate,					(GLenum face, GLenum func, GLint ref, GLuint mask));
	FNDEF_GL(void,				glStencilMask,							(GLuint mask));
	FNDEF_GL(void,				glStencilMaskSeparate,					(GLenum face, GLuint mask));
	FNDEF_GL(void,				glStencilOp,							(GLenum fail, GLenum zfail, GLenum zpass));
	FNDEF_GL(void,				glStencilOpSeparate,					(GLenum face, GLenum fail, GLenum zfail, GLenum zpass));
	FNDEF_GL(void,				glTexImage2D,							(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels));
	FNDEF_GL(void,				glTexImage3D,							(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels));
	FNDEF_GL(void,				glTexParameterf,						(GLenum target, GLenum pname, GLfloat param));
	FNDEF_GL(void,				glTexParameterfv,						(GLenum target, GLenum pname, const GLfloat* params));
	FNDEF_GL(void,				glTexParameteri,						(GLenum target, GLenum pname, GLint param));
	FNDEF_GL(void,				glTexParameteriv,						(GLenum target, GLenum pname, const GLint* params));
	FNDEF_GL(void,				glTexSubImage2D,						(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels));
	FNDEF_GL(void,				glUniform1f,							(GLint location, GLfloat x));
	FNDEF_GL(void,				glUniform1fv,							(GLint location, GLsizei count, const GLfloat* v));
	FNDEF_GL(void,				glUniform1i,							(GLint location, GLint x));
	FNDEF_GL(void,				glUniform1iv,							(GLint location, GLsizei count, const GLint* v));
	FNDEF_GL(void,				glUniform1ui,							(GLint location, GLuint x));
	FNDEF_GL(void,				glUniform2f,							(GLint location, GLfloat x, GLfloat y));
	FNDEF_GL(void,				glUniform2fv,							(GLint location, GLsizei count, const GLfloat* v));
	FNDEF_GL(void,				glUniform2i,							(GLint location, GLint x, GLint y));
	FNDEF_GL(void,				glUniform2iv,							(GLint location, GLsizei count, const GLint* v));
	FNDEF_GL(void,				glUniform3f,							(GLint location, GLfloat x, GLfloat y, GLfloat z));
	FNDEF_GL(void,				glUniform3fv,							(GLint location, GLsizei count, const GLfloat* v));
	FNDEF_GL(void,				glUniform3i,							(GLint location, GLint x, GLint y, GLint z));
	FNDEF_GL(void,				glUniform3iv,							(GLint location, GLsizei count, const GLint* v));
	FNDEF_GL(void,				glUniform4f,							(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w));
	FNDEF_GL(void,				glUniform4fv,							(GLint location, GLsizei count, const GLfloat* v));
	FNDEF_GL(void,				glUniform4i,							(GLint location, GLint x, GLint y, GLint z, GLint w));
	FNDEF_GL(void,				glUniform4iv,							(GLint location, GLsizei count, const GLint* v));
	FNDEF_GL(void,				glUniformMatrix2fv,						(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
	FNDEF_GL(void,				glUniformMatrix3fv,						(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
	FNDEF_GL(void,				glUniformMatrix4fv,						(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
	FNDEF_GL(void,				glUseProgram,							(GLuint program));
	FNDEF_GL(void,				glValidateProgram,						(GLuint program));
	FNDEF_GL(void,				glVertexAttrib1f,						(GLuint indx, GLfloat x));
	FNDEF_GL(void,				glVertexAttrib1fv,						(GLuint indx, const GLfloat* values));
	FNDEF_GL(void,				glVertexAttrib2f,						(GLuint indx, GLfloat x, GLfloat y));
	FNDEF_GL(void,				glVertexAttrib2fv,						(GLuint indx, const GLfloat* values));
	FNDEF_GL(void,				glVertexAttrib3f,						(GLuint indx, GLfloat x, GLfloat y, GLfloat z));
	FNDEF_GL(void,				glVertexAttrib3fv,						(GLuint indx, const GLfloat* values));
	FNDEF_GL(void,				glVertexAttrib4f,						(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w));
	FNDEF_GL(void,				glVertexAttrib4fv,						(GLuint indx, const GLfloat* values));
	FNDEF_GL(void,				glVertexAttribPointer,					(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr));
	FNDEF_GL(void,				glVertexAttribIPointer,					(GLuint indx, GLint size, GLenum type, GLsizei stride, const GLvoid* ptr));
	FNDEF_GL(void,				glVertexAttribDivisor,					(GLuint index, GLuint divisor));
	FNDEF_GL(void,				glViewport,								(GLint x, GLint y, GLsizei width, GLsizei height));
	FNDEF_GL(void,				glBindBufferBase,						(GLenum target, GLuint index, GLuint buffer));
	FNDEF_GL(void,				glUnmapBuffer,							(GLenum target));
	FNDEF_GL(void*,				glMapBufferRange,						(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access));
	FNDEF_GL(void*,				glDrawBuffers,							(GLsizei n, const GLenum* bufs));
	FNDEF_GL(void,				glTexSubImage3D,						(GLenum target, int level, int xoffset, int yoffset, int zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels));
	FNDEF_GL(void,				glCopyTexSubImage3D,					(GLenum target, int level, int xoffset, int yoffset, int zoffset, int x, int y, GLsizei width, GLsizei height));
	FNDEF_GL(void,				glCompressedTexSubImage3D,				(GLenum target, int level, int xoffset, int yoffset, int zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data));
	FNDEF_GL(void,				glGetBufferPointerv,					(GLenum target, GLenum pname, void** params));
	FNDEF_GL(void,				glBindVertexArray,						(GLuint array));
	FNDEF_GL(void,				glDeleteVertexArrays,					(GLsizei n, const GLuint* arrays));
	FNDEF_GL(void,				glGenVertexArrays,						(GLsizei n, GLuint* arrays));
	#undef FNDEF_GL

	//[-------------------------------------------------------]
	//[ Macros & definitions                                  ]
	//[-------------------------------------------------------]
	#ifndef FNPTR
		#define FNPTR(name) funcPtr_##name
	#endif

	// Redirect egl* and gl* function calls to funcPtr_egl* and funcPtr_gl*

	// EGL 1.4
	#define eglGetProcAddress		FNPTR(eglGetProcAddress)
	#define eglGetError				FNPTR(eglGetError)
	#define eglGetDisplay			FNPTR(eglGetDisplay)
	#define eglInitialize			FNPTR(eglInitialize)
	#define eglTerminate			FNPTR(eglTerminate)
	#define eglQueryString			FNPTR(eglQueryString)
	#define eglGetConfigs			FNPTR(eglGetConfigs)
	#define eglChooseConfig			FNPTR(eglChooseConfig)
	#define eglGetConfigAttrib		FNPTR(eglGetConfigAttrib)
	#define eglCreateWindowSurface	FNPTR(eglCreateWindowSurface)
	#define eglDestroySurface		FNPTR(eglDestroySurface)
	#define eglQuerySurface			FNPTR(eglQuerySurface)
	#define eglBindAPI				FNPTR(eglBindAPI)
	#define eglQueryAPI				FNPTR(eglQueryAPI)
	#define eglWaitClient			FNPTR(eglWaitClient)
	#define eglReleaseThread		FNPTR(eglReleaseThread)
	#define eglSurfaceAttrib		FNPTR(eglSurfaceAttrib)
	#define eglBindTexImage			FNPTR(eglBindTexImage)
	#define eglReleaseTexImage		FNPTR(eglReleaseTexImage)
	#define eglSwapInterval			FNPTR(eglSwapInterval)
	#define eglCreateContext		FNPTR(eglCreateContext)
	#define eglDestroyContext		FNPTR(eglDestroyContext)
	#define eglMakeCurrent			FNPTR(eglMakeCurrent)
	#define eglGetCurrentContext	FNPTR(eglGetCurrentContext)
	#define eglGetCurrentSurface	FNPTR(eglGetCurrentSurface)
	#define eglGetCurrentDisplay	FNPTR(eglGetCurrentDisplay)
	#define eglQueryContext			FNPTR(eglQueryContext)
	#define eglWaitGL				FNPTR(eglWaitGL)
	#define eglWaitNative			FNPTR(eglWaitNative)
	#define eglSwapBuffers			FNPTR(eglSwapBuffers)
	#define eglCopyBuffers			FNPTR(eglCopyBuffers)

	// ES 2
	#define glActiveTexture							FNPTR(glActiveTexture)
	#define glAttachShader							FNPTR(glAttachShader)
	#define glBindAttribLocation					FNPTR(glBindAttribLocation)
	#define glBindBuffer							FNPTR(glBindBuffer)
	#define glBindFramebuffer						FNPTR(glBindFramebuffer)
	#define glBindRenderbuffer						FNPTR(glBindRenderbuffer)
	#define glBindTexture							FNPTR(glBindTexture)
	#define glBlendColor							FNPTR(glBlendColor)
	#define glBlendEquation							FNPTR(glBlendEquation)
	#define glBlendEquationSeparate					FNPTR(glBlendEquationSeparate)
	#define glBlendFunc								FNPTR(glBlendFunc)
	#define glBlendFuncSeparate						FNPTR(glBlendFuncSeparate)
	#define glBufferData							FNPTR(glBufferData)
	#define glBufferSubData							FNPTR(glBufferSubData)
	#define glCheckFramebufferStatus				FNPTR(glCheckFramebufferStatus)
	#define glClear									FNPTR(glClear)
	#define glClearColor							FNPTR(glClearColor)
	#define glClearDepthf							FNPTR(glClearDepthf)
	#define glClearStencil							FNPTR(glClearStencil)
	#define glColorMask								FNPTR(glColorMask)
	#define glCompileShader							FNPTR(glCompileShader)
	#define glCompressedTexImage2D					FNPTR(glCompressedTexImage2D)
	#define glCompressedTexImage3D					FNPTR(glCompressedTexImage3D)
	#define glCompressedTexSubImage2D				FNPTR(glCompressedTexSubImage2D)
	#define glCopyTexImage2D						FNPTR(glCopyTexImage2D)
	#define glCopyTexSubImage2D						FNPTR(glCopyTexSubImage2D)
	#define glCreateProgram							FNPTR(glCreateProgram)
	#define glCreateShader							FNPTR(glCreateShader)
	#define glCullFace								FNPTR(glCullFace)
	#define glDeleteBuffers							FNPTR(glDeleteBuffers)
	#define glDeleteFramebuffers					FNPTR(glDeleteFramebuffers)
	#define glDeleteProgram							FNPTR(glDeleteProgram)
	#define glDeleteRenderbuffers					FNPTR(glDeleteRenderbuffers)
	#define glDeleteShader							FNPTR(glDeleteShader)
	#define glDeleteTextures						FNPTR(glDeleteTextures)
	#define glDepthFunc								FNPTR(glDepthFunc)
	#define glDepthMask								FNPTR(glDepthMask)
	#define glDepthRangef							FNPTR(glDepthRangef)
	#define glDetachShader							FNPTR(glDetachShader)
	#define glDisable								FNPTR(glDisable)
	#define glDisableVertexAttribArray				FNPTR(glDisableVertexAttribArray)
	#define glDrawArrays							FNPTR(glDrawArrays)
	#define glDrawArraysInstanced					FNPTR(glDrawArraysInstanced)
	#define glDrawElements							FNPTR(glDrawElements)
	#define glDrawElementsInstanced					FNPTR(glDrawElementsInstanced)
	#define glEnable								FNPTR(glEnable)
	#define glEnableVertexAttribArray				FNPTR(glEnableVertexAttribArray)
	#define glFinish								FNPTR(glFinish)
	#define glFlush									FNPTR(glFlush)
	#define glFramebufferRenderbuffer				FNPTR(glFramebufferRenderbuffer)
	#define glFramebufferTexture2D					FNPTR(glFramebufferTexture2D)
	#define glFramebufferTextureLayer				FNPTR(glFramebufferTextureLayer)
	#define glBlitFramebuffer						FNPTR(glBlitFramebuffer)
	#define glFrontFace								FNPTR(glFrontFace)
	#define glGenBuffers							FNPTR(glGenBuffers)
	#define glGenerateMipmap						FNPTR(glGenerateMipmap)
	#define glGenFramebuffers						FNPTR(glGenFramebuffers)
	#define glGenRenderbuffers						FNPTR(glGenRenderbuffers)
	#define glGenTextures							FNPTR(glGenTextures)
	#define glGetActiveAttrib						FNPTR(glGetActiveAttrib)
	#define glGetActiveUniform						FNPTR(glGetActiveUniform)
	#define glGetAttachedShaders					FNPTR(glGetAttachedShaders)
	#define glGetAttribLocation						FNPTR(glGetAttribLocation)
	#define glGetBooleanv							FNPTR(glGetBooleanv)
	#define glGetBufferParameteriv					FNPTR(glGetBufferParameteriv)
	#define glGetError								FNPTR(glGetError)
	#define glGetFloatv								FNPTR(glGetFloatv)
	#define glGetFramebufferAttachmentParameteriv	FNPTR(glGetFramebufferAttachmentParameteriv)
	#define glGetIntegerv							FNPTR(glGetIntegerv)
	#define glGetProgramiv							FNPTR(glGetProgramiv)
	#define glGetProgramInfoLog						FNPTR(glGetProgramInfoLog)
	#define glGetRenderbufferParameteriv			FNPTR(glGetRenderbufferParameteriv)
	#define glGetShaderiv							FNPTR(glGetShaderiv)
	#define glGetShaderInfoLog						FNPTR(glGetShaderInfoLog)
	#define glGetShaderPrecisionFormat				FNPTR(glGetShaderPrecisionFormat)
	#define glGetShaderSource						FNPTR(glGetShaderSource)
	#define glGetString								FNPTR(glGetString)
	#define glGetTexParameterfv						FNPTR(glGetTexParameterfv)
	#define glGetTexParameteriv						FNPTR(glGetTexParameteriv)
	#define glGetUniformfv							FNPTR(glGetUniformfv)
	#define glGetUniformiv							FNPTR(glGetUniformiv)
	#define glGetUniformLocation					FNPTR(glGetUniformLocation)
	#define glGetUniformBlockIndex					FNPTR(glGetUniformBlockIndex)
	#define glUniformBlockBinding					FNPTR(glUniformBlockBinding)
	#define glGetVertexAttribfv						FNPTR(glGetVertexAttribfv)
	#define glGetVertexAttribiv						FNPTR(glGetVertexAttribiv)
	#define glGetVertexAttribPointerv				FNPTR(glGetVertexAttribPointerv)
	#define glHint									FNPTR(glHint)
	#define glIsBuffer								FNPTR(glIsBuffer)
	#define glIsEnabled								FNPTR(glIsEnabled)
	#define glIsFramebuffer							FNPTR(glIsFramebuffer)
	#define glIsProgram								FNPTR(glIsProgram)
	#define glIsRenderbuffer						FNPTR(glIsRenderbuffer)
	#define glIsShader								FNPTR(glIsShader)
	#define glIsTexture								FNPTR(glIsTexture)
	#define glLineWidth								FNPTR(glLineWidth)
	#define glLinkProgram							FNPTR(glLinkProgram)
	#define glPixelStorei							FNPTR(glPixelStorei)
	#define glPolygonOffset							FNPTR(glPolygonOffset)
	#define glReadPixels							FNPTR(glReadPixels)
	#define glReleaseShaderCompiler					FNPTR(glReleaseShaderCompiler)
	#define glRenderbufferStorage					FNPTR(glRenderbufferStorage)
	#define glSampleCoverage						FNPTR(glSampleCoverage)
	#define glScissor								FNPTR(glScissor)
	#define glShaderBinary							FNPTR(glShaderBinary)
	#define glShaderSource							FNPTR(glShaderSource)
	#define glStencilFunc							FNPTR(glStencilFunc)
	#define glStencilFuncSeparate					FNPTR(glStencilFuncSeparate)
	#define glStencilMask							FNPTR(glStencilMask)
	#define glStencilMaskSeparate					FNPTR(glStencilMaskSeparate)
	#define glStencilOp								FNPTR(glStencilOp)
	#define glStencilOpSeparate						FNPTR(glStencilOpSeparate)
	#define glTexImage2D							FNPTR(glTexImage2D)
	#define glTexImage3D							FNPTR(glTexImage3D)
	#define glTexParameterf							FNPTR(glTexParameterf)
	#define glTexParameterfv						FNPTR(glTexParameterfv)
	#define glTexParameteri							FNPTR(glTexParameteri)
	#define glTexParameteriv						FNPTR(glTexParameteriv)
	#define glTexSubImage2D							FNPTR(glTexSubImage2D)
	#define glUniform1f								FNPTR(glUniform1f)
	#define glUniform1fv							FNPTR(glUniform1fv)
	#define glUniform1i								FNPTR(glUniform1i)
	#define glUniform1iv							FNPTR(glUniform1iv)
	#define glUniform1ui							FNPTR(glUniform1ui)
	#define glUniform2f								FNPTR(glUniform2f)
	#define glUniform2fv							FNPTR(glUniform2fv)
	#define glUniform2i								FNPTR(glUniform2i)
	#define glUniform2iv							FNPTR(glUniform2iv)
	#define glUniform3f								FNPTR(glUniform3f)
	#define glUniform3fv							FNPTR(glUniform3fv)
	#define glUniform3i								FNPTR(glUniform3i)
	#define glUniform3iv							FNPTR(glUniform3iv)
	#define glUniform4f								FNPTR(glUniform4f)
	#define glUniform4fv							FNPTR(glUniform4fv)
	#define glUniform4i								FNPTR(glUniform4i)
	#define glUniform4iv							FNPTR(glUniform4iv)
	#define glUniformMatrix2fv						FNPTR(glUniformMatrix2fv)
	#define glUniformMatrix3fv						FNPTR(glUniformMatrix3fv)
	#define glUniformMatrix4fv						FNPTR(glUniformMatrix4fv)
	#define glUseProgram							FNPTR(glUseProgram)
	#define glValidateProgram						FNPTR(glValidateProgram)
	#define glVertexAttrib1f						FNPTR(glVertexAttrib1f)
	#define glVertexAttrib1fv						FNPTR(glVertexAttrib1fv)
	#define glVertexAttrib2f						FNPTR(glVertexAttrib2f)
	#define glVertexAttrib2fv						FNPTR(glVertexAttrib2fv)
	#define glVertexAttrib3f						FNPTR(glVertexAttrib3f)
	#define glVertexAttrib3fv						FNPTR(glVertexAttrib3fv)
	#define glVertexAttrib4f						FNPTR(glVertexAttrib4f)
	#define glVertexAttrib4fv						FNPTR(glVertexAttrib4fv)
	#define glVertexAttribPointer					FNPTR(glVertexAttribPointer)
	#define glVertexAttribIPointer					FNPTR(glVertexAttribIPointer)
	#define glVertexAttribDivisor					FNPTR(glVertexAttribDivisor)
	#define glViewport								FNPTR(glViewport)

	// ES 3.0
	#define glBindBufferBase			FNPTR(glBindBufferBase)
	#define glUnmapBuffer				FNPTR(glUnmapBuffer)
	#define glMapBufferRange			FNPTR(glMapBufferRange)
	#define glDrawBuffers				FNPTR(glDrawBuffers)
	#define glTexImage3D				FNPTR(glTexImage3D)
	#define glTexSubImage3D				FNPTR(glTexSubImage3D)
	#define glCopyTexSubImage3D			FNPTR(glCopyTexSubImage3D)
	#define glCompressedTexSubImage3D	FNPTR(glCompressedTexSubImage3D)
	#define glGetBufferPointerv			FNPTR(glGetBufferPointerv)
	#define glBindVertexArray			FNPTR(glBindVertexArray)
	#define glDeleteVertexArrays		FNPTR(glDeleteVertexArrays)
	#define glGenVertexArrays			FNPTR(glGenVertexArrays)




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/ExtensionsRuntimeLinking.h               ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Define helper macro                                   ]
	//[-------------------------------------------------------]
	#define FNDEF_EX(retType, funcName, args) retType (GL_APIENTRY *funcPtr_##funcName) args = nullptr
	#ifndef FNPTR
		#define FNPTR(name) funcPtr_##name
	#endif

	//[-------------------------------------------------------]
	//[ EXT definitions                                       ]
	//[-------------------------------------------------------]
	// GL_EXT_texture_compression_s3tc
	#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT		0x83F0
	#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT	0x83F1
	#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT	0x83F2
	#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT	0x83F3

	// GL_EXT_texture_compression_dxt1
	// #define GL_COMPRESSED_RGB_S3TC_DXT1_EXT	0x83F0	// Already defined for GL_EXT_texture_compression_s3tc

	// GL_EXT_texture_compression_latc
	#define GL_COMPRESSED_LUMINANCE_LATC1_EXT				0x8C70
	#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT		0x8C71
	#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT			0x8C72
	#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT	0x8C73

	// GL_EXT_texture_buffer
	FNDEF_EX(void,	glTexBufferEXT,	(GLenum target, GLenum internalformat, GLuint buffer));
	#define glTexBufferEXT	FNPTR(glTexBufferEXT)

	// GL_EXT_draw_elements_base_vertex
	FNDEF_EX(void,	glDrawElementsBaseVertexEXT,			(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex));
	FNDEF_EX(void,	glDrawElementsInstancedBaseVertexEXT,	(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex));
	#define glDrawElementsBaseVertexEXT				FNPTR(glDrawElementsBaseVertexEXT)
	#define glDrawElementsInstancedBaseVertexEXT	FNPTR(glDrawElementsInstancedBaseVertexEXT)

	// GL_EXT_base_instance
	FNDEF_EX(void,	glDrawArraysInstancedBaseInstanceEXT,				(GLenum mode, int first, GLsizei count, GLsizei instancecount, GLuint baseinstance));
	FNDEF_EX(void,	glDrawElementsInstancedBaseInstanceEXT,				(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLuint baseinstance));
	FNDEF_EX(void,	glDrawElementsInstancedBaseVertexBaseInstanceEXT,	(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance));
	#define glDrawArraysInstancedBaseInstanceEXT				FNPTR(glDrawArraysInstancedBaseInstanceEXT)
	#define glDrawElementsInstancedBaseInstanceEXT				FNPTR(glDrawElementsInstancedBaseInstanceEXT)
	#define glDrawElementsInstancedBaseVertexBaseInstanceEXT	FNPTR(glDrawElementsInstancedBaseVertexBaseInstanceEXT)

	// GL_EXT_clip_control
	FNDEF_EX(void,	glClipControlEXT,	(GLenum origin, GLenum depth));
	#define glClipControlEXT			FNPTR(glClipControlEXT)

	//[-------------------------------------------------------]
	//[ AMD definitions                                       ]
	//[-------------------------------------------------------]
	// GL_AMD_compressed_3DC_texture
	#define GL_3DC_X_AMD	0x87F9
	#define GL_3DC_XY_AMD	0x87FA

	//[-------------------------------------------------------]
	//[ NV definitions                                        ]
	//[-------------------------------------------------------]
	// GL_NV_fbo_color_attachments
	#define GL_COLOR_ATTACHMENT0_NV		0x8CE0	// Same value as GL_COLOR_ATTACHMENT0
	#define GL_COLOR_ATTACHMENT1_NV		0x8CE1
	#define GL_COLOR_ATTACHMENT2_NV		0x8CE2
	#define GL_COLOR_ATTACHMENT3_NV		0x8CE3
	#define GL_COLOR_ATTACHMENT4_NV		0x8CE4
	#define GL_COLOR_ATTACHMENT5_NV		0x8CE5
	#define GL_COLOR_ATTACHMENT6_NV		0x8CE6
	#define GL_COLOR_ATTACHMENT7_NV		0x8CE7
	#define GL_COLOR_ATTACHMENT8_NV		0x8CE8
	#define GL_COLOR_ATTACHMENT9_NV		0x8CE9
	#define GL_COLOR_ATTACHMENT10_NV	0x8CEA
	#define GL_COLOR_ATTACHMENT11_NV	0x8CEB
	#define GL_COLOR_ATTACHMENT12_NV	0x8CEC
	#define GL_COLOR_ATTACHMENT13_NV	0x8CED
	#define GL_COLOR_ATTACHMENT14_NV	0x8CEE
	#define GL_COLOR_ATTACHMENT15_NV	0x8CEF

	//[-------------------------------------------------------]
	//[ OES definitions                                       ]
	//[-------------------------------------------------------]
	// GL_OES_element_index_uint
	#define GL_UNSIGNED_INT	0x1405

	// GL_OES_packed_depth_stencil
	#define GL_DEPTH_STENCIL_OES		0x84F9
	#define GL_UNSIGNED_INT_24_8_OES	0x84FA
	#define GL_DEPTH24_STENCIL8_OES		0x88F0

	// GL_OES_depth24
	#define GL_DEPTH_COMPONENT24_OES	0x81A6

	// GL_OES_depth32
	#define GL_DEPTH_COMPONENT32_OES	0x81A7

	//[-------------------------------------------------------]
	//[ KHR definitions                                       ]
	//[-------------------------------------------------------]
	// GL_KHR_debug
	FNDEF_EX(void,	glDebugMessageCallbackKHR,	(GLDEBUGPROCKHR callback, const void* userParam));
	FNDEF_EX(void,	glDebugMessageControlKHR,	(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled));
	FNDEF_EX(void,	glDebugMessageInsertKHR,	(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf));
	FNDEF_EX(void,	glPushDebugGroupKHR,		(GLenum source, GLuint id, GLsizei length, const GLchar* message));
	FNDEF_EX(void,	glPopDebugGroupKHR,			(void));
	FNDEF_EX(void,	glObjectLabelKHR,			(GLenum identifier, GLuint name, GLsizei length, const GLchar* label));
	#define glDebugMessageCallbackKHR	FNPTR(glDebugMessageCallbackKHR)
	#define glDebugMessageControlKHR	FNPTR(glDebugMessageControlKHR)
	#define glDebugMessageInsertKHR		FNPTR(glDebugMessageInsertKHR)
	#define glPushDebugGroupKHR			FNPTR(glPushDebugGroupKHR)
	#define glPopDebugGroupKHR			FNPTR(glPopDebugGroupKHR)
	#define glObjectLabelKHR			FNPTR(glObjectLabelKHR)

	//[-------------------------------------------------------]
	//[ Undefine helper macro                                 ]
	//[-------------------------------------------------------]
	#undef FNDEF_EX




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/OpenGLES3Rhi.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 RHI class
	*/
	class OpenGLES3Rhi final : public Rhi::IRhi
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
		explicit OpenGLES3Rhi(const Rhi::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~OpenGLES3Rhi() override;

		/**
		*  @brief
		*    Return the OpenGL ES 3 context instance
		*
		*  @return
		*    The OpenGL ES 3 context instance, do not free the memory the reference is pointing to
		*/
		[[nodiscard]] inline IOpenGLES3Context& getOpenGLES3Context() const
		{
			return *mOpenGLES3Context;
		}

		void dispatchCommandBufferInternal(const Rhi::CommandBuffer& commandBuffer);

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
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
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
		//[ Operation                                             ]
		//[-------------------------------------------------------]
		virtual void dispatchCommandBuffer(const Rhi::CommandBuffer& commandBuffer) override;


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(mContext, OpenGLES3Rhi, this);
		}


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Debug message callback function called by the "GL_KHR_debug"-extension
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
		static void GL_APIENTRY debugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int length, const char* message, const void* userParam);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit OpenGLES3Rhi(const OpenGLES3Rhi& source) = delete;
		OpenGLES3Rhi& operator =(const OpenGLES3Rhi& source) = delete;

		/**
		*  @brief
		*    Initialize the capabilities
		*/
		void initializeCapabilities();

		/**
		*  @brief
		*    Set graphics program
		*
		*  @param[in] graphicsProgram
		*    Graphics program to set
		*/
		void setGraphicsProgram(Rhi::IGraphicsProgram* graphicsProgram);

		/**
		*  @brief
		*    Update "GL_EXT_base_instance" emulation
		*
		*  @param[in] startInstanceLocation
		*    Start instance location
		*/
		void updateGL_EXT_base_instanceEmulation(uint32_t startInstanceLocation);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IOpenGLES3Context*	  mOpenGLES3Context;					///< OpenGL ES 3 context instance, always valid
		Rhi::IShaderLanguage* mShaderLanguageGlsl;					///< GLSL shader language instance (we keep a reference to it), can be a null pointer
		RootSignature*		  mGraphicsRootSignature;				///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		Rhi::ISamplerState*   mDefaultSamplerState;					///< Default rasterizer state (we keep a reference to it), can be a null pointer
		GLuint				  mOpenGLES3CopyResourceFramebuffer;	///< OpenGL ES 3 framebuffer ("container" object, not shared between OpenGL ES 3 contexts) used by "OpenGLES3Rhi::OpenGLES3Rhi::copyResource()", can be zero if no resource is allocated
		GLuint				  mDefaultOpenGLES3VertexArray;			///< Default OpenGL ES 3 vertex array ("container" object, not shared between OpenGL contexts) to enable attribute-less rendering, can be zero if no resource is allocated
		// States
		GraphicsPipelineState* mGraphicsPipelineState;	///< Currently set graphics pipeline state (we keep a reference to it), can be a null pointer
		// Input-assembler (IA) stage
		VertexArray* mVertexArray;					///< Currently set vertex array (we keep a reference to it), can be a null pointer
		GLenum		 mOpenGLES3PrimitiveTopology;	///< OpenGL ES 3 primitive topology describing the type of primitive to render
		// Output-merger (OM) stage
		Rhi::IRenderTarget* mRenderTarget;	///< Currently set render target (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant OpenGL ES 3 calls
		GLenum	 mOpenGLES3ClipControlOrigin;	///< Currently set OpenGL ES 3 clip control origin
		GLuint	 mOpenGLES3Program;				///< Currently set OpenGL ES 3 program, can be zero if no resource is set
		// Draw ID uniform location for "GL_EXT_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
		GLint	 mDrawIdUniformLocation;		///< Draw ID uniform location
		uint32_t mCurrentStartInstanceLocation;	///< Currently set start instance location


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/IOpenGLES3Context.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL ES 3 context base interface
	*/
	class IOpenGLES3Context
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IOpenGLES3Context()
		{
			// Everything must be done in "deinitialize()" because when we're in here, the shared libraries
			// are already unloaded and we are no longer allowed to use EGL or OpenGL ES 3 functions
		}

		/**
		*  @brief
		*    Return whether or not the context is properly initialized
		*/
		[[nodiscard]] inline bool isInitialized() const
		{
			return (mUseExternalContext || EGL_NO_CONTEXT != getEGLContext());
		}

		/**
		*  @brief
		*    Return the handle of a native OS window which is valid as long as the RHI instance exists
		*
		*  @return
		*    The handle of a native OS window which is valid as long as the RHI instance exists, "NULL_HANDLE" if there's no such window
		*/
		[[nodiscard]] inline Rhi::handle getNativeWindowHandle() const
		{
			return mNativeWindowHandle;
		}

		/**
		*  @brief
		*    Return the used EGL display
		*
		*  @return
		*    The used EGL display, "EGL_NO_DISPLAY" on error
		*/
		[[nodiscard]] inline EGLDisplay getEGLDisplay() const
		{
			return mEGLDisplay;
		}

		/**
		*  @brief
		*    Return the used EGL configuration
		*
		*  @return
		*    The used EGL configuration, null pointer on error
		*/
		[[nodiscard]] inline EGLConfig getEGLConfig() const
		{
			return mEGLConfig;
		}

		/**
		*  @brief
		*    Return the used EGL context
		*
		*  @return
		*    The used EGL context, "EGL_NO_CONTEXT" on error
		*/
		[[nodiscard]] inline EGLContext getEGLContext() const
		{
			return mEGLContext;
		}

		/**
		*  @brief
		*    Return the used EGL dummy surface
		*
		*  @return
		*    The used EGL dummy surface, "EGL_NO_SURFACE" on error
		*/
		[[nodiscard]] inline EGLSurface getEGLDummySurface() const
		{
			return mDummySurface;
		}

		/**
		*  @brief
		*    Makes a given EGL surface to the currently used one
		*
		*  @param[in] eglSurface
		*    EGL surface to make to the current one, can be a null pointer, in this case an internal dummy surface is set
		*
		*  @return
		*    "EGL_TRUE" if all went fine, else "EGL_FALSE"
		*/
		[[nodiscard]] EGLBoolean makeCurrent(EGLSurface eglSurface)
		{
			// Use the EGL dummy surface?
			if (nullptr == eglSurface)
			{
				eglSurface = mDummySurface;
			}

			// Make the EGL surface to the current one
			return eglMakeCurrent(mEGLDisplay, eglSurface, eglSurface, mEGLContext);
		}


		//[-------------------------------------------------------]
		//[ Platform specific                                     ]
		//[-------------------------------------------------------]
		#if defined(LINUX) && !defined(__ANDROID__)
			[[nodiscard]] inline ::Display* getX11Display() const
			{
				return mX11Display;
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual OpenGLES3Rhi::IOpenGLES3Context methods ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Initialize the context
		*
		*  @param[in] multisampleAntialiasingSamples
		*    Multisample antialiasing samples per pixel, <=1 means no antialiasing
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] virtual bool initialize(uint32_t multisampleAntialiasingSamples)
		{
			if (mUseExternalContext)
			{
				return true;
			}

			// Get display
			#if (defined(LINUX) && !defined(__ANDROID__))
				mEGLDisplay = eglGetDisplay(static_cast<EGLNativeDisplayType>(mX11Display));
			#else
				mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
			#endif
			if (EGL_NO_DISPLAY != mEGLDisplay)
			{
				// Initialize EGL
				EGLint eglMajorVersion, eglMinorVersion;
				if (eglInitialize(mEGLDisplay, &eglMajorVersion, &eglMinorVersion) == EGL_TRUE)
				{
					// Choose a EGL configuration
					mEGLConfig = chooseConfig(multisampleAntialiasingSamples);

					// We can only go on if a EGL configuration was chosen properly
					if (mEGLConfig)
					{
						// Create context (request an version 3 client)
						// TODO(co) Add support for the "GL_KHR_no_error"-extension
						const EGLint contextAttribs[] = {
							EGL_CONTEXT_CLIENT_VERSION, 3,
							#ifdef RHI_DEBUG
								EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR, // TODO(sw) make it possible to enable it from outside during runtime
							#endif
							EGL_NONE
						};
						mEGLContext = eglCreateContext(mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, contextAttribs);
						if (EGL_NO_CONTEXT != mEGLContext)
						{
							// Create a dummy native window?
							if (NULL_HANDLE != mNativeWindowHandle)
							{
								// There's no need to create a dummy native window, we've got a real native window to work with :D
								mDummyNativeWindow = (EGLNativeWindowType)mNativeWindowHandle;	// Interesting - in here, we have an OS dependent cast issue when using C++ casts: While we would need
																								// reinterpret_cast<EGLNativeWindowType>(nativeWindowHandle) under Microsoft Windows ("HWND"), we would need static_cast<EGLNativeWindowType>(nativeWindowHandle)
																								// under Linux ("int")... so, to avoid #ifdefs, we just use old school c-style casts in here...

								#ifdef __ANDROID__
									// Reconfigure the ANativeWindow buffers to match
									EGLint format;
									eglGetConfigAttrib(mEGLDisplay, mEGLConfig, EGL_NATIVE_VISUAL_ID, &format);
									ANativeWindow_setBuffersGeometry(reinterpret_cast<ANativeWindow*>(mNativeWindowHandle), 0, 0, format);
								#endif
							}
							else
							{
								// Create the dummy native window
								#ifdef _WIN32
									HINSTANCE moduleHandle = ::GetModuleHandle(nullptr);
									WNDCLASS windowClass;
									windowClass.hInstance	  = moduleHandle;
									windowClass.lpszClassName = TEXT("OpenGLES3DummyNativeWindow");
									windowClass.lpfnWndProc	  = DefWindowProc;
									windowClass.style		  = 0;
									windowClass.hIcon		  = nullptr;
									windowClass.hCursor		  = nullptr;
									windowClass.lpszMenuName  = nullptr;
									windowClass.cbClsExtra	  = 0;
									windowClass.cbWndExtra	  = 0;
									windowClass.hbrBackground = nullptr;
									::RegisterClass(&windowClass);
									mDummyNativeWindow = ::CreateWindow(TEXT("OpenGLES3DummyNativeWindow"), TEXT("PFormat"), WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, nullptr, moduleHandle, nullptr);
								#elif (defined(LINUX) && !defined(__ANDROID__))
									// Create dummy window
									XSetWindowAttributes xSetWindowAttributes;
									xSetWindowAttributes.event_mask   = 0;
									xSetWindowAttributes.border_pixel = 0;
									mDummyNativeWindow = ::XCreateWindow(mX11Display, DefaultRootWindow(mX11Display), 0, 0, 300, 300, 0, CopyFromParent, InputOutput, CopyFromParent, CWBorderPixel | CWEventMask, &xSetWindowAttributes);
								#endif
							}

							// Create an EGL dummy surface
							mDummySurface = eglCreateWindowSurface(mEGLDisplay, mEGLConfig, mDummyNativeWindow, nullptr);
							if (EGL_NO_SURFACE == mDummySurface)
							{
								// Error! Failed to create EGL dummy surface!
							}

							// Make the internal dummy surface to the currently used one
							if (makeCurrent(EGL_NO_SURFACE) == EGL_FALSE)
							{
								// Error! Failed to make the EGL dummy surface to the current one!
							}

							// Done
							return true;
						}
						else
						{
							// Error! Failed to create EGL context!
						}
					}
					else
					{
						// Error! Failed to choose EGL configuration! (OpenGL ES 3 not supported?)
					}
				}
				else
				{
					// Error! Failed to initialize EGL!
				}
			}
			else
			{
				// Error! Failed to get EGL default display!
			}

			// Error!
			return false;
		}

		/**
		*  @brief
		*    Return the available extensions
		*
		*  @return
		*    The available extensions
		*/
		[[nodiscard]] virtual const IExtensions& getExtensions() const = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] nativeWindowHandle
		*    Handle of a native OS window which is valid as long as the RHI instance exists, "NULL_HANDLE" if there's no such window
		*  @param[in] useExternalContext
		*    When true an own OpenGL ES context won't be created
		*/
		IOpenGLES3Context([[maybe_unused]] OpenGLES3Rhi& openGLES3Rhi, Rhi::handle nativeWindowHandle, bool useExternalContext) :
			mNativeWindowHandle(nativeWindowHandle),
			#if (defined(LINUX) && !defined(__ANDROID__))
				mX11Display(nullptr),
				mOwnsX11Display(true),
			#endif
			mEGLDisplay(EGL_NO_DISPLAY),
			mEGLConfig(nullptr),
			mEGLContext(EGL_NO_CONTEXT),
			mDummyNativeWindow(NULL_HANDLE),
			mDummySurface(EGL_NO_SURFACE),
			mUseExternalContext(useExternalContext)
		{
			#if (defined(LINUX) && !defined(__ANDROID__))
				const Rhi::Context& context = openGLES3Rhi.getContext();

				// If the given RHI context is an X11 context use the display connection object provided by the context
				if (context.getType() == Rhi::Context::ContextType::X11)
				{
					mX11Display = static_cast<const Rhi::X11Context&>(context).getDisplay();
					mOwnsX11Display = mX11Display == nullptr;
				}

				if (mOwnsX11Display)
				{
					mX11Display = XOpenDisplay(nullptr);
				}
			#endif
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] source
		*    Source to copy from
		*/
		explicit IOpenGLES3Context(const IOpenGLES3Context& source) = delete;

		/**
		*  @brief
		*    De-initialize the context
		*/
		void deinitialize()
		{
			// Don't touch anything in case we don't have a display
			if (EGL_NO_DISPLAY != mEGLDisplay)
			{
				// Make "nothing" current
				eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

				// Destroy the EGL dummy surface
				if (EGL_NO_SURFACE != mDummySurface && eglDestroySurface(mEGLDisplay, mDummySurface) == EGL_FALSE)
				{
					// Error! Failed to destroy the used EGL dummy surface!
				}
				mDummySurface = EGL_NO_SURFACE;

				// Destroy the EGL context
				if (EGL_NO_CONTEXT != mEGLContext)
				{
					// Release all resources allocated by the shader compiler
					glReleaseShaderCompiler();

					// Destroy the EGL context
					if (eglDestroyContext(mEGLDisplay, mEGLContext) == EGL_FALSE)
					{
						// Error! Failed to destroy the used EGL context!
					}
					mEGLContext = EGL_NO_CONTEXT;
				}

				// Return EGL to it's state at thread initialization
				if (eglReleaseThread() == EGL_FALSE)
				{
					// Error! Failed to release the EGL thread!
				}

				// Terminate the EGL display
				if (eglTerminate(mEGLDisplay) == EGL_FALSE)
				{
					// Error! Failed to terminate the used EGL display!
				}
				mEGLDisplay = EGL_NO_DISPLAY;
				mEGLConfig  = nullptr;

				// Destroy the dummy native window, if required
				#ifdef _WIN32
					if (NULL_HANDLE == mNativeWindowHandle && NULL_HANDLE != mDummyNativeWindow)
					{
						::DestroyWindow(mDummyNativeWindow);
						::UnregisterClass(TEXT("OpenGLES3DummyNativeWindow"), ::GetModuleHandle(nullptr));
					}
				#elif (defined(LINUX) && !defined(__ANDROID__))
					// Destroy the dummy native window
					if (NULL_HANDLE == mNativeWindowHandle && NULL_HANDLE != mDummyNativeWindow)
					{
						::XDestroyWindow(mX11Display, mDummyNativeWindow);
					}

					// Close the X server display connection
					if (nullptr != mX11Display && mOwnsX11Display)
					{
						::XCloseDisplay(mX11Display);
						mX11Display = nullptr;
					}
				#endif
				mDummyNativeWindow = NULL_HANDLE;
			}
		}

		/**
		*  @brief
		*    Copy operator
		*
		*  @param[in] source
		*    Source to copy from
		*
		*  @return
		*    Reference to this instance
		*/
		IOpenGLES3Context& operator =(const IOpenGLES3Context&) = delete;


	//[-------------------------------------------------------]
	//[ Protected virtual OpenGLES3Rhi::IOpenGLES3Context methods ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Choose a EGL configuration
		*
		*  @param[in] multisampleAntialiasingSamples
		*    Multisample antialiasing samples per pixel
		*
		*  @return
		*    The chosen EGL configuration, a null pointer on error
		*
		*  @note
		*    - Automatically tries to find fallback configurations
		*/
		[[nodiscard]] virtual EGLConfig chooseConfig(uint32_t multisampleAntialiasingSamples) const
		{
			// Try to find a working EGL configuration
			EGLConfig eglConfig = nullptr;
			EGLint numberOfConfigurations = 0;
			bool chooseConfigCapitulated = false;
			EGLint multisampleAntialiasingSampleBuffers = 0;
			EGLint multisampleAntialiasingSamplesCurrent = static_cast<EGLint>(multisampleAntialiasingSamples);
			do
			{
				// Get the current multisample antialiasing settings
				const bool multisampleAntialiasing = (multisampleAntialiasingSamplesCurrent > 1);	// Multisample antialiasing with just one sample per pixel isn't real multisample, is it? :D
				multisampleAntialiasingSampleBuffers = multisampleAntialiasing ? 1 : 0;

				// Set desired configuration
				const EGLint configAttribs[] =
				{
					EGL_LEVEL,				0,										// Frame buffer level
					EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,							// Which types of EGL surfaces are supported
					EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES3_BIT_KHR,					// Which client APIs are supported
					EGL_DEPTH_SIZE,			EGL_DONT_CARE,							// Bits of Z in the depth buffer
					EGL_SAMPLE_BUFFERS,		multisampleAntialiasingSampleBuffers,	// Number of multisample buffers (enable/disable multisample antialiasing)
					EGL_SAMPLES,			multisampleAntialiasingSamplesCurrent,	// Number of samples per pixel (multisample antialiasing samples)
					EGL_BUFFER_SIZE,		16,
					EGL_NONE
				};

				// Choose exactly one matching configuration
				if (eglChooseConfig(mEGLDisplay, configAttribs, &eglConfig, 1, &numberOfConfigurations) == EGL_FALSE || numberOfConfigurations < 1)
				{
					// Can we change something on the multisample antialiasing? (may be the cause that no configuration was found!)
					if (multisampleAntialiasing)
					{
						if (multisampleAntialiasingSamplesCurrent > 8)
						{
							multisampleAntialiasingSamplesCurrent = 8;
						}
						else if (multisampleAntialiasingSamplesCurrent > 4)
						{
							multisampleAntialiasingSamplesCurrent = 4;
						}
						else if (multisampleAntialiasingSamplesCurrent > 2)
						{
							multisampleAntialiasingSamplesCurrent = 2;
						}
						else if (2 == multisampleAntialiasingSamplesCurrent)
						{
							multisampleAntialiasingSamplesCurrent = 0;
						}
					} else
					{
						// Don't mind, forget it...
						chooseConfigCapitulated = true;
					}
				}
			} while (numberOfConfigurations < 1 && !chooseConfigCapitulated);

			// Done
			return eglConfig;
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		Rhi::handle	mNativeWindowHandle;	///< Handle of a native OS window which is valid as long as the RHI instance exists, "NULL_HANDLE" if there's no such window
		// X11
		#if (defined(LINUX) && !defined(__ANDROID__))
			::Display	   *mX11Display;
			bool 			mOwnsX11Display;
		#endif
		// EGL
		EGLDisplay			mEGLDisplay;
		// EGL
		EGLConfig			mEGLConfig;
		EGLContext			mEGLContext;
		EGLNativeWindowType	mDummyNativeWindow;	///< Native dummy window handle, can be identical to "mNativeWindowHandle" if it's in fact no dummy at all, can be "NULL_HANDLE"
		EGLSurface			mDummySurface;
		bool				mUseExternalContext;


	};





	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/IExtensions.h                            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL ES 3 extensions base interface
	*
	*  @note
	*    - Extensions are only optional, so do always take into account that an extension may not be available
	*/
	class IExtensions
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IExtensions()
		{}


	//[-------------------------------------------------------]
	//[ Public virtual OpenGLES3Rhi::IExtensions methods      ]
	//[-------------------------------------------------------]
	public:
		///////////////////////////////////////////////////////////
		// Returns whether an extension is supported or not
		///////////////////////////////////////////////////////////
		// EXT
		[[nodiscard]] virtual bool isGL_EXT_texture_compression_s3tc() const = 0;
		[[nodiscard]] virtual bool isGL_EXT_texture_compression_dxt1() const = 0;
		[[nodiscard]] virtual bool isGL_EXT_texture_compression_latc() const = 0;
		[[nodiscard]] virtual bool isGL_EXT_texture_buffer() const = 0;
		[[nodiscard]] virtual bool isGL_EXT_draw_elements_base_vertex() const = 0;
		[[nodiscard]] virtual bool isGL_EXT_base_instance() const = 0;
		[[nodiscard]] virtual bool isGL_EXT_clip_control() const = 0;
		// AMD
		[[nodiscard]] virtual bool isGL_AMD_compressed_3DC_texture() const = 0;
		// NV
		[[nodiscard]] virtual bool isGL_NV_fbo_color_attachments() const = 0;
		// OES
		[[nodiscard]] virtual bool isGL_OES_element_index_uint() const = 0;
		[[nodiscard]] virtual bool isGL_OES_packed_depth_stencil() const = 0;
		[[nodiscard]] virtual bool isGL_OES_depth24() const = 0;
		[[nodiscard]] virtual bool isGL_OES_depth32() const = 0;
		// KHR
		[[nodiscard]] virtual bool isGL_KHR_debug() const = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Default constructor
		*/
		inline IExtensions()
		{}

		explicit IExtensions(const IExtensions& source) = delete;
		IExtensions& operator =(const IExtensions& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/ExtensionsRuntimeLinking.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 extensions runtime linking
	*/
	class ExtensionsRuntimeLinking final : public IExtensions
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*/
		inline explicit ExtensionsRuntimeLinking(OpenGLES3Rhi& openGLES3Rhi) :
			mOpenGLES3Rhi(openGLES3Rhi),
			// EXT
			mGL_EXT_texture_compression_s3tc(false),
			mGL_EXT_texture_compression_dxt1(false),
			mGL_EXT_texture_compression_latc(false),
			mGL_EXT_texture_buffer(false),
			mGL_EXT_draw_elements_base_vertex(false),
			mGL_EXT_base_instance(false),
			mGL_EXT_clip_control(false),
			// AMD
			mGL_AMD_compressed_3DC_texture(false),
			// NV
			mGL_NV_fbo_color_attachments(false),
			// OES
			mGL_OES_element_index_uint(false),
			mGL_OES_packed_depth_stencil(false),
			mGL_OES_depth24(false),
			mGL_OES_depth32(false),
			// KHR
			mGL_KHR_debug(false)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ExtensionsRuntimeLinking() override
		{}

		/**
		*  @brief
		*    Initialize the supported extensions
		*
		*  @note
		*    - Do only call this method if the EGL functions initialization was successful ("glGetString()" is used)
		*      and there's an active render context
		*/
		void initialize()
		{
			// Define a helper macro
			#define IMPORT_FUNC(funcName)																															\
				if (result)																																			\
				{																																					\
					void* symbol = eglGetProcAddress(#funcName);																									\
					if (nullptr != symbol)																															\
					{																																				\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																							\
					}																																				\
					else																																			\
					{																																				\
						RHI_LOG(mOpenGLES3Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the OpenGL ES 3 shared library", #funcName)	\
						result = false;																																\
					}																																				\
				}

			// Get the extensions string and the OpenGL ES version
			const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
			GLint majorVersion = 0, minorVersion = 0; 
			glGetIntegerv(GL_MAJOR_VERSION, &majorVersion); 
			glGetIntegerv(GL_MINOR_VERSION, &minorVersion);

			//[-------------------------------------------------------]
			//[ EXT                                                   ]
			//[-------------------------------------------------------]
			// TODO(co) Review whether or not those extensions are already inside the OpenGL ES 3 core
			mGL_EXT_texture_compression_s3tc = (nullptr != strstr(extensions, "GL_EXT_texture_compression_s3tc"));
			mGL_EXT_texture_compression_dxt1 = (nullptr != strstr(extensions, "GL_EXT_texture_compression_dxt1"));
			mGL_EXT_texture_compression_latc = (nullptr != strstr(extensions, "GL_EXT_texture_compression_latc"));

			// "GL_EXT_texture_buffer"
			// TODO(sw) Core in opengles 3.2
			//mGL_EXT_texture_buffer = (nullptr != strstr(extensions, "GL_EXT_texture_buffer"));
			// TODO(sw) Disabled for now. With mesa 17.1.3 the OpenGLES driver supports version 3.1 + texture buffer. But currently the shader of the Example project supports only the emulation path
			mGL_EXT_texture_buffer = false;
			if (mGL_EXT_texture_buffer)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glTexBufferEXT)
				mGL_EXT_texture_buffer = result;
			}

			// "GL_EXT_draw_elements_base_vertex" is part of OpenGL ES 3.2
			if (majorVersion >= 3 && minorVersion >= 2)
			{
				#define FNDEF_EX(retType, funcName, args) retType (GL_APIENTRY *funcPtr_##funcName) args = nullptr
				FNDEF_EX(void,	glDrawElementsBaseVertex,			(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex));
				FNDEF_EX(void,	glDrawElementsInstancedBaseVertex,	(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount, GLint basevertex));
				#define glDrawElementsBaseVertex			FNPTR(glDrawElementsBaseVertex)
				#define glDrawElementsInstancedBaseVertex	FNPTR(glDrawElementsInstancedBaseVertex)

				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawElementsBaseVertex)
				IMPORT_FUNC(glDrawElementsInstancedBaseVertex)
				mGL_EXT_draw_elements_base_vertex = result;
				glDrawElementsBaseVertexEXT = glDrawElementsBaseVertex;
				glDrawElementsInstancedBaseVertexEXT = glDrawElementsInstancedBaseVertex;

				#undef FNDEF_EX
				#undef glDrawElementsBaseVertex
				#undef glDrawElementsInstancedBaseVertex
			}
			else
			{
				mGL_EXT_draw_elements_base_vertex = (nullptr != strstr(extensions, "GL_EXT_draw_elements_base_vertex"));
				if (mGL_EXT_draw_elements_base_vertex)
				{
					// Load the entry points
					bool result = true;	// Success by default
					IMPORT_FUNC(glDrawElementsBaseVertexEXT)
					IMPORT_FUNC(glDrawElementsInstancedBaseVertexEXT)
					mGL_EXT_draw_elements_base_vertex = result;
				}
			}

			// "GL_EXT_base_instance"
			mGL_EXT_base_instance = (nullptr != strstr(extensions, "GL_EXT_base_instance"));
			if (mGL_EXT_base_instance)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDrawArraysInstancedBaseInstanceEXT)
				IMPORT_FUNC(glDrawElementsInstancedBaseInstanceEXT)
				IMPORT_FUNC(glDrawElementsInstancedBaseVertexBaseInstanceEXT)
				mGL_EXT_base_instance = result;
			}

			// "GL_EXT_clip_control"
			mGL_EXT_clip_control = (nullptr != strstr(extensions, "GL_EXT_clip_control"));
			if (mGL_EXT_clip_control)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glClipControlEXT)
				mGL_EXT_clip_control = result;
			}


			//[-------------------------------------------------------]
			//[ AMD                                                   ]
			//[-------------------------------------------------------]
			mGL_AMD_compressed_3DC_texture = (nullptr != strstr(extensions, "GL_AMD_compressed_3DC_texture"));

			//[-------------------------------------------------------]
			//[ NV                                                    ]
			//[-------------------------------------------------------]
			mGL_NV_fbo_color_attachments = (nullptr != strstr(extensions, "GL_NV_fbo_color_attachments"));

			//[-------------------------------------------------------]
			//[ OES                                                   ]
			//[-------------------------------------------------------]
			mGL_OES_element_index_uint	 = (nullptr != strstr(extensions, "GL_OES_element_index_uint"));
			mGL_OES_packed_depth_stencil = (nullptr != strstr(extensions, "GL_OES_packed_depth_stencil"));
			mGL_OES_depth24				 = (nullptr != strstr(extensions, "GL_OES_depth24"));
			mGL_OES_depth32				 = (nullptr != strstr(extensions, "GL_OES_depth32"));

			//[-------------------------------------------------------]
			//[ KHR                                                   ]
			//[-------------------------------------------------------]
			mGL_KHR_debug = (nullptr != strstr(extensions, "GL_KHR_debug"));
			if (mGL_KHR_debug)
			{
				// Load the entry points
				bool result = true;	// Success by default
				IMPORT_FUNC(glDebugMessageCallbackKHR)
				IMPORT_FUNC(glDebugMessageControlKHR)
				IMPORT_FUNC(glDebugMessageInsertKHR)
				IMPORT_FUNC(glPushDebugGroupKHR)
				IMPORT_FUNC(glPopDebugGroupKHR)
				IMPORT_FUNC(glObjectLabelKHR)
				mGL_KHR_debug = result;
			}

			// Undefine the helper macro
			#undef IMPORT_FUNC
		}


	//[-------------------------------------------------------]
	//[ Public virtual OpenGLES3Rhi::IExtensions methods      ]
	//[-------------------------------------------------------]
	public:
		///////////////////////////////////////////////////////////
		// Returns whether an extension is supported or not
		///////////////////////////////////////////////////////////
		// EXT
		[[nodiscard]] inline virtual bool isGL_EXT_texture_compression_s3tc() const override
		{
			return mGL_EXT_texture_compression_s3tc;
		}

		[[nodiscard]] inline virtual bool isGL_EXT_texture_compression_dxt1() const override
		{
			return mGL_EXT_texture_compression_dxt1;
		}

		[[nodiscard]] inline virtual bool isGL_EXT_texture_compression_latc() const override
		{
			return mGL_EXT_texture_compression_latc;
		}

		[[nodiscard]] inline virtual bool isGL_EXT_texture_buffer() const override
		{
			return mGL_EXT_texture_buffer;
		}

		[[nodiscard]] inline virtual bool isGL_EXT_draw_elements_base_vertex() const override
		{
			return mGL_EXT_draw_elements_base_vertex;
		}

		[[nodiscard]] inline virtual bool isGL_EXT_base_instance() const override
		{
			return mGL_EXT_base_instance;
		}

		[[nodiscard]] inline virtual bool isGL_EXT_clip_control() const override
		{
			return mGL_EXT_clip_control;
		}

		// AMD
		[[nodiscard]] inline virtual bool isGL_AMD_compressed_3DC_texture() const override
		{
			return mGL_AMD_compressed_3DC_texture;
		}

		// NV
		[[nodiscard]] inline virtual bool isGL_NV_fbo_color_attachments() const override
		{
			return mGL_NV_fbo_color_attachments;
		}

		// OES
		[[nodiscard]] inline virtual bool isGL_OES_element_index_uint() const override
		{
			return mGL_OES_element_index_uint;
		}

		[[nodiscard]] inline virtual bool isGL_OES_packed_depth_stencil() const override
		{
			return mGL_OES_packed_depth_stencil;
		}

		[[nodiscard]] inline virtual bool isGL_OES_depth24() const override
		{
			return mGL_OES_depth24;
		}

		[[nodiscard]] inline virtual bool isGL_OES_depth32() const override
		{
			return mGL_OES_depth32;
		}

		// KHR
		[[nodiscard]] inline virtual bool isGL_KHR_debug() const override
		{
			return mGL_KHR_debug;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ExtensionsRuntimeLinking(const ExtensionsRuntimeLinking& source) = delete;
		ExtensionsRuntimeLinking& operator =(const ExtensionsRuntimeLinking& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLES3Rhi& mOpenGLES3Rhi;	///< Owner OpenGL ES 3 RHI instance
		// EXT
		bool mGL_EXT_texture_compression_s3tc;
		bool mGL_EXT_texture_compression_dxt1;
		bool mGL_EXT_texture_compression_latc;
		bool mGL_EXT_texture_buffer;
		bool mGL_EXT_draw_elements_base_vertex;
		bool mGL_EXT_base_instance;
		bool mGL_EXT_clip_control;
		// AMD
		bool mGL_AMD_compressed_3DC_texture;
		// NV
		bool mGL_NV_fbo_color_attachments;
		// OES
		bool mGL_OES_element_index_uint;
		bool mGL_OES_packed_depth_stencil;
		bool mGL_OES_depth24;
		bool mGL_OES_depth32;
		// KHR
		bool mGL_KHR_debug;


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/OpenGLES3ContextRuntimeLinking.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 runtime linking context
	*
	*  @remarks
	*    This context implementation links against the OpenGL ES 3 shared libraries at runtime. There are
	*    three typical variations of this OpenGL ES 3 shared libraries:
	*
	*    - Implementation for OpenGL ES 3 on mobile devices.
	*
	*    - Implementation for OpenGL ES 3 on desktop PC by using a OpenGL ES 3 capable graphics driver.
	*      Basing on the example http://developer.amd.com/samples/assets/egl_sample.zip from
	*      http://blogs.amd.com/developer/2010/07/26/opengl-es-2-0-coming-to-a-desktop-near-you/
	*      Tested with "AMD Catalyst 11.8" on a "ATI Mobility Radeon HD 4850", no errors, but just got
	*      a white screen when Windows Aero is active. As soon as I disabled Windows Aero all went fine.
	*
	*    - Implementation for testing OpenGL ES 3 on a desktop PC using OpenGL ES 3 Emulator from ARM
	*      (http://www.malideveloper.com/tools/software-development/opengl-es-20-emulator.php). If you have
	*      a OpenGL ES 3 capable graphics driver, you may want to use the "ContextNative"-implementation
	*      instead.
	*/
	class OpenGLES3ContextRuntimeLinking final : public IOpenGLES3Context
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] nativeWindowHandle
		*    Handle of a native OS window which is valid as long as the RHI instance exists, "NULL_HANDLE" if there's no such window
		*  @param[in] useExternalContext
		*    When true an own OpenGL ES context won't be created
		*/
		OpenGLES3ContextRuntimeLinking(OpenGLES3Rhi& openGLES3Rhi, Rhi::handle nativeWindowHandle, bool useExternalContext) :
			IOpenGLES3Context(openGLES3Rhi, nativeWindowHandle, useExternalContext),
			mOpenGLES3Rhi(openGLES3Rhi),
			mEGLSharedLibrary(nullptr),
			mGLESSharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mExtensions(RHI_NEW(openGLES3Rhi.getContext(), ExtensionsRuntimeLinking)(openGLES3Rhi))
		{
			// Load the shared libraries
			if (loadSharedLibraries())
			{
				// Load the EGL entry points
				if (loadEGLEntryPoints())
				{
					// Load the OpenGL ES 3 entry points
					if (loadGLESEntryPoints())
					{
						// Entry points successfully registered
						mEntryPointsRegistered = true;
					}
					else
					{
						RHI_LOG(openGLES3Rhi.getContext(), CRITICAL, "Failed to load in the OpenGL ES 3 entry points")
					}
				}
				else
				{
					RHI_LOG(openGLES3Rhi.getContext(), CRITICAL, "Failed to load in the OpenGL ES 3 EGL entry points")
				}
			}
			else
			{
				RHI_LOG(openGLES3Rhi.getContext(), CRITICAL, "Failed to load in the OpenGL ES 3 shared libraries")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~OpenGLES3ContextRuntimeLinking() override
		{
			// De-initialize the context while we still can
			deinitialize();

			// Destroy the extensions instance
			RHI_DELETE(mOpenGLES3Rhi.getContext(), ExtensionsRuntimeLinking, mExtensions);

			// Destroy the shared library instances
			#ifdef _WIN32
				if (nullptr != mEGLSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mEGLSharedLibrary));
				}
				if (nullptr != mGLESSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mGLESSharedLibrary));
				}
			#elif defined LINUX
				if (nullptr != mEGLSharedLibrary)
				{
					::dlclose(mEGLSharedLibrary);
				}
				if (nullptr != mGLESSharedLibrary)
				{
					::dlclose(mGLESSharedLibrary);
				}
			#else
				#error "Unsupported platform"
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual OpenGLES3Rhi::IOpenGLES3Context methods ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual bool initialize(uint32_t multisampleAntialiasingSamples) override
		{
			// Entry points successfully registered?
			if (mEntryPointsRegistered)
			{
				// Call base implementation
				if (IOpenGLES3Context::initialize(multisampleAntialiasingSamples))
				{
					// Initialize the supported extensions
					mExtensions->initialize();

					// Done
					return true;
				}
			}

			// Error!
			return false;
		}

		[[nodiscard]] inline virtual const IExtensions& getExtensions() const override
		{
			return *mExtensions;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual OpenGLES3Rhi::IOpenGLES3Context methods ]
	//[-------------------------------------------------------]
	protected:
		[[nodiscard]] virtual EGLConfig chooseConfig(uint32_t multisampleAntialiasingSamples) const override
		{
			// Try to find a working EGL configuration
			EGLConfig eglConfig = nullptr;
			EGLint numberOfConfigurations = 0;
			bool chooseConfigCapitulated = false;
			EGLint multisampleAntialiasingSamplesCurrent = static_cast<EGLint>(multisampleAntialiasingSamples);
			do
			{
				// Get the current multisample antialiasing settings
				const bool multisampleAntialiasing = (multisampleAntialiasingSamplesCurrent > 1);	// Multisample antialiasing with just one sample per pixel isn't real multisample, is it? :D
				// const EGLint multisampleAntialiasingSampleBuffers = multisampleAntialiasing ? 1 : 0;

				// Set desired configuration
				const EGLint configAttribs[] =
				{
					EGL_LEVEL,				0,										// Frame buffer level
					EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,							// Which types of EGL surfaces are supported
					EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES3_BIT_KHR,					// Which client APIs are supported
					EGL_RED_SIZE,			8,										// Bits of red color component
					EGL_GREEN_SIZE,			8,										// Bits of red color component
					EGL_BLUE_SIZE,			8,										// Bits of red color component
					EGL_DEPTH_SIZE,			16,										// Bits of Z in the depth buffer TODO(co) Make it possible to set this from the outside, but do also automatically go down if it fails, e.g. 24 doesn't work for me
					// TODO(co) Currently something looks wrong when using the desktop drivers - just black screen when using multisample ("AMD Catalyst 11.8" on a "ATI Mobility Radeon HD 4850")
					//       -> No issues with Android on the device (but it looks like there's no antialiasing, check it later in detail)
				//	EGL_SAMPLE_BUFFERS,		multisampleAntialiasingSampleBuffers,	// Number of multisample buffers (enable/disable multisample antialiasing)
				//	EGL_SAMPLES,			multisampleAntialiasingSamplesCurrent,	// Number of samples per pixel (multisample antialiasing samples)
					EGL_NONE
				};

				// Choose exactly one matching configuration
				if (eglChooseConfig(mEGLDisplay, configAttribs, &eglConfig, 1, &numberOfConfigurations) == EGL_FALSE || numberOfConfigurations < 1)
				{
					// Can we change something on the multisample antialiasing? (may be the cause that no configuration was found!)
					if (multisampleAntialiasing)
					{
						if (multisampleAntialiasingSamplesCurrent > 8)
						{
							multisampleAntialiasingSamplesCurrent = 8;
						}
						else if (multisampleAntialiasingSamplesCurrent > 4)
						{
							multisampleAntialiasingSamplesCurrent = 4;
						}
						else if (multisampleAntialiasingSamplesCurrent > 2)
						{
							multisampleAntialiasingSamplesCurrent = 2;
						}
						else if (2 == multisampleAntialiasingSamplesCurrent)
						{
							multisampleAntialiasingSamplesCurrent = 0;
						}
					}
					else
					{
						// Don't mind, forget it...
						chooseConfigCapitulated = true;
					}
				}
			} while (numberOfConfigurations < 1 && !chooseConfigCapitulated);

			// Done
			return eglConfig;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit OpenGLES3ContextRuntimeLinking(const OpenGLES3ContextRuntimeLinking& source) = delete;
		OpenGLES3ContextRuntimeLinking& operator =(const OpenGLES3ContextRuntimeLinking& source) = delete;

		/**
		*  @brief
		*    Loads the shared libraries
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadSharedLibraries()
		{
			// We don't need to check m_pEGLSharedLibrary and m_pGLESSharedLibrary at this point because we know they must contain a null pointer

			// EGL and OpenGL ES 3 may be within a single shared library, or within two separate shared libraries
			#ifdef _WIN32
				// First, try the OpenGL ES 3 emulator from ARM (it's possible to move around this dll without issues, so, this one first)
				mEGLSharedLibrary = ::LoadLibraryExA("libEGL.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr != mEGLSharedLibrary)
				{
					mGLESSharedLibrary = ::LoadLibraryExA("libGLESv2.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				}
				else
				{
					// Second, try the AMD/ATI driver
					mEGLSharedLibrary = ::LoadLibraryExA("atioglxx.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr != mEGLSharedLibrary)
					{
						mGLESSharedLibrary = ::LoadLibraryExA("atioglxx.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					}
					else
					{
						// Third, try the NVIDIA driver
						mEGLSharedLibrary = ::LoadLibraryExA("nvoglv32.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
						if (nullptr != mEGLSharedLibrary)
						{
							mGLESSharedLibrary = ::LoadLibraryExA("nvoglv32.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
						}
					}
				}
			#elif defined __ANDROID__
				// On Android we have "libEGL.so"
				mEGLSharedLibrary = ::dlopen("libEGL.so", RTLD_LAZY);
				if (nullptr != mEGLSharedLibrary)
				{
					mGLESSharedLibrary = ::dlopen("libGLESv2.so", RTLD_LAZY);
				}
			#elif defined LINUX
				// First "libGL.so": The closed source drivers doesn't provide separate libraries for GLES and EGL (at least the drivers from AMD)
				// but the separate EGL/GLES3 libs might be present on the system
				mEGLSharedLibrary = ::dlopen("libGL.so", RTLD_LAZY);
				if (nullptr != mEGLSharedLibrary)
				{
					// Try finding the eglGetProcAddress to determine if this library contains EGL/GLES support.
					// This check is needed because only the closed source drivers have the EGL/GLES support in "libGL.so".
					// The open source drivers (mesa) have separate libraries for this and they can be present on the system even the closed source drivers are used.
					void* symbol = ::dlsym(mEGLSharedLibrary, "eglGetProcAddress");
					if (nullptr != symbol)
					{
						mGLESSharedLibrary = ::dlopen("libGL.so", RTLD_LAZY);
					}
					else
					{
						// Unload the library
						::dlclose(mEGLSharedLibrary);
						mEGLSharedLibrary = nullptr;
					}
				}
				if (nullptr == mEGLSharedLibrary)
				{
					// Then we try the separate libs for EGL and GLES (provided either by an emulator or native from mesa)
					mEGLSharedLibrary = ::dlopen("libEGL.so", RTLD_LAZY);
					if (nullptr != mEGLSharedLibrary)
					{
						mGLESSharedLibrary = ::dlopen("libGLESv2.so", RTLD_LAZY);
					}
				}
			#else
				#error "Unsupported platform"
			#endif

			// Done
			return (nullptr != mEGLSharedLibrary && nullptr != mGLESSharedLibrary);
		}

		/**
		*  @brief
		*    Loads the EGL entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @note
		*    - Do only call this method if the OpenGL ES 3 shared library was loaded successfully
		*/
		[[nodiscard]] bool loadEGLEntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#ifdef _WIN32
				#define IMPORT_FUNC(funcName)																																						\
					if (result)																																										\
					{																																												\
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mEGLSharedLibrary), #funcName);																						\
						if (nullptr == symbol)																																						\
						{																																											\
							/* The specification states that "eglGetProcAddress" is only for extension functions, but when using OpenGL ES 3 on desktop PC by using a								\
							   native OpenGL ES 3 capable graphics driver under Linux (tested with "AMD Catalyst 11.8"), only this way will work */													\
							if (nullptr != eglGetProcAddress)																																		\
							{																																										\
								symbol = eglGetProcAddress(#funcName);																																\
							}																																										\
						}																																											\
						if (nullptr != symbol)																																						\
						{																																											\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																														\
						}																																											\
						else																																										\
						{																																											\
							wchar_t moduleFilename[MAX_PATH];																																		\
							moduleFilename[0] = '\0';																																				\
							::GetModuleFileNameW(static_cast<HMODULE>(mEGLSharedLibrary), moduleFilename, MAX_PATH);																				\
							RHI_LOG(mOpenGLES3Rhi.getContext(), CRITICAL, "Failed to locate the OpenGL ES 3 entry point \"%s\" within the EGL shared library \"%s\"", #funcName, moduleFilename)	\
							result = false;																																							\
						}																																											\
					}
			#elif defined(__ANDROID__)
				#define IMPORT_FUNC(funcName)																																					\
					if (result)																																									\
					{																																											\
						void* symbol = ::dlsym(mEGLSharedLibrary, #funcName);																													\
						if (nullptr == symbol)																																					\
						{																																										\
							/* The specification states that "eglGetProcAddress" is only for extension functions, but when using OpenGL ES 3 on desktop PC by using a							\
							   native OpenGL ES 3 capable graphics driver under Linux (tested with "AMD Catalyst 11.8"), only this way will work */												\
							if (nullptr != eglGetProcAddress)																																	\
							{																																									\
								symbol = eglGetProcAddress(#funcName);																															\
							}																																									\
						}																																										\
						if (nullptr != symbol)																																					\
						{																																										\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
						}																																										\
						else																																									\
						{																																										\
							const char* libraryName = "unknown";																																\
							RHI_LOG(mOpenGLES3Rhi.getContext(), CRITICAL, "Failed to locate the OpenGL ES 3 entry point \"%s\" within the EGL shared library \"%s\"", #funcName, libraryName)	\
							result = false;																																						\
						}																																										\
					}
			#elif defined LINUX
				#define IMPORT_FUNC(funcName)																																					\
					if (result)																																									\
					{																																											\
						void* symbol = ::dlsym(mEGLSharedLibrary, #funcName);																													\
						if (nullptr == symbol)																																					\
						{																																										\
							/* The specification states that "eglGetProcAddress" is only for extension functions, but when using OpenGL ES 3 on desktop PC by using a							\
							   native OpenGL ES 3 capable graphics driver under Linux (tested with "AMD Catalyst 11.8"), only this way will work */												\
							if (nullptr != eglGetProcAddress)																																	\
							{																																									\
								symbol = eglGetProcAddress(#funcName);																															\
							}																																									\
						}																																										\
						if (nullptr != symbol)																																					\
						{																																										\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
						}																																										\
						else																																									\
						{																																										\
							link_map* linkMap = nullptr;																																		\
							const char* libraryName = "unknown";																																\
							if (dlinfo(mEGLSharedLibrary, RTLD_DI_LINKMAP, &linkMap))																											\
							{																																									\
								libraryName = linkMap->l_name;																																	\
							}																																									\
							libraryName = libraryName; /* To avoid -Wunused-but-set-variable warning when RHI_LOG is defined empty */															\
							RHI_LOG(mOpenGLES3Rhi.getContext(), CRITICAL, "Failed to locate the OpenGL ES 3 entry point \"%s\" within the EGL shared library \"%s\"", #funcName, libraryName)	\
							result = false;																																						\
						}																																										\
					}
			#else
				#error "Unsupported platform"
			#endif

			// Load the entry points
			IMPORT_FUNC(eglGetProcAddress)
			IMPORT_FUNC(eglGetError)
			IMPORT_FUNC(eglGetDisplay)
			IMPORT_FUNC(eglInitialize)
			IMPORT_FUNC(eglTerminate)
			IMPORT_FUNC(eglQueryString)
			IMPORT_FUNC(eglGetConfigs)
			IMPORT_FUNC(eglChooseConfig)
			IMPORT_FUNC(eglGetConfigAttrib)
			IMPORT_FUNC(eglCreateWindowSurface)
			IMPORT_FUNC(eglDestroySurface)
			IMPORT_FUNC(eglQuerySurface)
			IMPORT_FUNC(eglBindAPI)
			IMPORT_FUNC(eglQueryAPI)
			IMPORT_FUNC(eglWaitClient)
			IMPORT_FUNC(eglReleaseThread)
			IMPORT_FUNC(eglSurfaceAttrib)
			IMPORT_FUNC(eglBindTexImage)
			IMPORT_FUNC(eglReleaseTexImage)
			IMPORT_FUNC(eglSwapInterval)
			IMPORT_FUNC(eglCreateContext)
			IMPORT_FUNC(eglDestroyContext)
			IMPORT_FUNC(eglMakeCurrent)
			IMPORT_FUNC(eglGetCurrentContext)
			IMPORT_FUNC(eglGetCurrentSurface)
			IMPORT_FUNC(eglGetCurrentDisplay)
			IMPORT_FUNC(eglQueryContext)
			IMPORT_FUNC(eglWaitGL)
			IMPORT_FUNC(eglWaitNative)
			IMPORT_FUNC(eglSwapBuffers)
			IMPORT_FUNC(eglCopyBuffers)

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Loads the OpenGL ES 3 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*
		*  @note
		*    - Do only call this method if the OpenGL ES 3 shared library was loaded successfully
		*/
		[[nodiscard]] bool loadGLESEntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#ifdef __ANDROID__
				// Native OpenGL ES 3 on mobile device
				#define IMPORT_FUNC(funcName)																														\
					if (result)																																		\
					{																																				\
						void* symbol = ::dlsym(mGLESSharedLibrary, #funcName);																						\
						if (nullptr != symbol)																														\
						{																																			\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																						\
						}																																			\
						else																																		\
						{																																			\
							result = false;																															\
						}																																			\
					}
			#else
				// Native OpenGL ES 3 on desktop PC, we need an function entry point work around for this
				#define IMPORT_FUNC(funcName)																																\
					if (result)																																				\
					{																																						\
						/* The specification states that "eglGetProcAddress" is only for extension functions, but when using OpenGL ES 3 on desktop PC by using a			\
						   native OpenGL ES 3 capable graphics driver (tested with "AMD Catalyst 11.8"), only this way will work */											\
						void* symbol = eglGetProcAddress(#funcName);																										\
						if (nullptr != symbol)																																\
						{																																					\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																								\
						}																																					\
						else																																				\
						{																																					\
							RHI_LOG(mOpenGLES3Rhi.getContext(), CRITICAL, "Failed to locate the OpenGL ES 3 entry point \"%s\" within the GLES shared library", #funcName)	\
							result = false;																																	\
						}																																					\
					}
			#endif

			// Load the entry points
			IMPORT_FUNC(glActiveTexture)
			IMPORT_FUNC(glAttachShader)
			IMPORT_FUNC(glBindAttribLocation)
			IMPORT_FUNC(glBindBuffer)
			IMPORT_FUNC(glBindFramebuffer)
			IMPORT_FUNC(glBindRenderbuffer)
			IMPORT_FUNC(glBindTexture)
			IMPORT_FUNC(glBlendColor)
			IMPORT_FUNC(glBlendEquation)
			IMPORT_FUNC(glBlendEquationSeparate)
			IMPORT_FUNC(glBlendFunc)
			IMPORT_FUNC(glBlendFuncSeparate)
			IMPORT_FUNC(glBufferData)
			IMPORT_FUNC(glBufferSubData)
			IMPORT_FUNC(glCheckFramebufferStatus)
			IMPORT_FUNC(glClear)
			IMPORT_FUNC(glClearColor)
			IMPORT_FUNC(glClearDepthf)
			IMPORT_FUNC(glClearStencil)
			IMPORT_FUNC(glColorMask)
			IMPORT_FUNC(glCompileShader)
			IMPORT_FUNC(glCompressedTexImage2D)
			IMPORT_FUNC(glCompressedTexImage3D)
			IMPORT_FUNC(glCompressedTexSubImage2D)
			IMPORT_FUNC(glCopyTexImage2D)
			IMPORT_FUNC(glCopyTexSubImage2D)
			IMPORT_FUNC(glCreateProgram)
			IMPORT_FUNC(glCreateShader)
			IMPORT_FUNC(glCullFace)
			IMPORT_FUNC(glDeleteBuffers)
			IMPORT_FUNC(glDeleteFramebuffers)
			IMPORT_FUNC(glDeleteProgram)
			IMPORT_FUNC(glDeleteRenderbuffers)
			IMPORT_FUNC(glDeleteShader)
			IMPORT_FUNC(glDeleteTextures)
			IMPORT_FUNC(glDepthFunc)
			IMPORT_FUNC(glDepthMask)
			IMPORT_FUNC(glDepthRangef)
			IMPORT_FUNC(glDetachShader)
			IMPORT_FUNC(glDisable)
			IMPORT_FUNC(glDisableVertexAttribArray)
			IMPORT_FUNC(glDrawArrays)
			IMPORT_FUNC(glDrawArraysInstanced)
			IMPORT_FUNC(glDrawElements)
			IMPORT_FUNC(glDrawElementsInstanced)
			IMPORT_FUNC(glEnable)
			IMPORT_FUNC(glEnableVertexAttribArray)
			IMPORT_FUNC(glFinish)
			IMPORT_FUNC(glFlush)
			IMPORT_FUNC(glFramebufferRenderbuffer)
			IMPORT_FUNC(glFramebufferTexture2D)
			IMPORT_FUNC(glFramebufferTextureLayer)
			IMPORT_FUNC(glBlitFramebuffer)
			IMPORT_FUNC(glFrontFace)
			IMPORT_FUNC(glGenBuffers)
			IMPORT_FUNC(glGenerateMipmap)
			IMPORT_FUNC(glGenFramebuffers)
			IMPORT_FUNC(glGenRenderbuffers)
			IMPORT_FUNC(glGenTextures)
			IMPORT_FUNC(glGetActiveAttrib)
			IMPORT_FUNC(glGetActiveUniform)
			IMPORT_FUNC(glGetAttachedShaders)
			IMPORT_FUNC(glGetAttribLocation)
			IMPORT_FUNC(glGetBooleanv)
			IMPORT_FUNC(glGetBufferParameteriv)
			IMPORT_FUNC(glGetError)
			IMPORT_FUNC(glGetFloatv)
			IMPORT_FUNC(glGetFramebufferAttachmentParameteriv)
			IMPORT_FUNC(glGetIntegerv)
			IMPORT_FUNC(glGetProgramiv)
			IMPORT_FUNC(glGetProgramInfoLog)
			IMPORT_FUNC(glGetRenderbufferParameteriv)
			IMPORT_FUNC(glGetShaderiv)
			IMPORT_FUNC(glGetShaderInfoLog)
			IMPORT_FUNC(glGetShaderPrecisionFormat)
			IMPORT_FUNC(glGetShaderSource)
			IMPORT_FUNC(glGetString)
			IMPORT_FUNC(glGetTexParameterfv)
			IMPORT_FUNC(glGetTexParameteriv)
			IMPORT_FUNC(glGetUniformfv)
			IMPORT_FUNC(glGetUniformiv)
			IMPORT_FUNC(glGetUniformLocation)
			IMPORT_FUNC(glGetUniformBlockIndex)
			IMPORT_FUNC(glUniformBlockBinding)
			IMPORT_FUNC(glGetVertexAttribfv)
			IMPORT_FUNC(glGetVertexAttribiv)
			IMPORT_FUNC(glGetVertexAttribPointerv)
			IMPORT_FUNC(glHint)
			IMPORT_FUNC(glIsBuffer)
			IMPORT_FUNC(glIsEnabled)
			IMPORT_FUNC(glIsFramebuffer)
			IMPORT_FUNC(glIsProgram)
			IMPORT_FUNC(glIsRenderbuffer)
			IMPORT_FUNC(glIsShader)
			IMPORT_FUNC(glIsTexture)
			IMPORT_FUNC(glLineWidth)
			IMPORT_FUNC(glLinkProgram)
			IMPORT_FUNC(glPixelStorei)
			IMPORT_FUNC(glPolygonOffset)
			IMPORT_FUNC(glReadPixels)
			IMPORT_FUNC(glReleaseShaderCompiler)
			IMPORT_FUNC(glRenderbufferStorage)
			IMPORT_FUNC(glSampleCoverage)
			IMPORT_FUNC(glScissor)
			IMPORT_FUNC(glShaderBinary)
			IMPORT_FUNC(glShaderSource)
			IMPORT_FUNC(glStencilFunc)
			IMPORT_FUNC(glStencilFuncSeparate)
			IMPORT_FUNC(glStencilMask)
			IMPORT_FUNC(glStencilMaskSeparate)
			IMPORT_FUNC(glStencilOp)
			IMPORT_FUNC(glStencilOpSeparate)
			IMPORT_FUNC(glTexImage2D)
			IMPORT_FUNC(glTexImage3D)
			IMPORT_FUNC(glTexParameterf)
			IMPORT_FUNC(glTexParameterfv)
			IMPORT_FUNC(glTexParameteri)
			IMPORT_FUNC(glTexParameteriv)
			IMPORT_FUNC(glTexSubImage2D)
			IMPORT_FUNC(glUniform1f)
			IMPORT_FUNC(glUniform1fv)
			IMPORT_FUNC(glUniform1i)
			IMPORT_FUNC(glUniform1iv)
			IMPORT_FUNC(glUniform1ui)
			IMPORT_FUNC(glUniform2f)
			IMPORT_FUNC(glUniform2fv)
			IMPORT_FUNC(glUniform2i)
			IMPORT_FUNC(glUniform2iv)
			IMPORT_FUNC(glUniform3f)
			IMPORT_FUNC(glUniform3fv)
			IMPORT_FUNC(glUniform3i)
			IMPORT_FUNC(glUniform3iv)
			IMPORT_FUNC(glUniform4f)
			IMPORT_FUNC(glUniform4fv)
			IMPORT_FUNC(glUniform4i)
			IMPORT_FUNC(glUniform4iv)
			IMPORT_FUNC(glUniformMatrix2fv)
			IMPORT_FUNC(glUniformMatrix3fv)
			IMPORT_FUNC(glUniformMatrix4fv)
			IMPORT_FUNC(glUseProgram)
			IMPORT_FUNC(glValidateProgram)
			IMPORT_FUNC(glVertexAttrib1f)
			IMPORT_FUNC(glVertexAttrib1fv)
			IMPORT_FUNC(glVertexAttrib2f)
			IMPORT_FUNC(glVertexAttrib2fv)
			IMPORT_FUNC(glVertexAttrib3f)
			IMPORT_FUNC(glVertexAttrib3fv)
			IMPORT_FUNC(glVertexAttrib4f)
			IMPORT_FUNC(glVertexAttrib4fv)
			IMPORT_FUNC(glVertexAttribPointer)
			IMPORT_FUNC(glVertexAttribIPointer)
			IMPORT_FUNC(glVertexAttribDivisor)
			IMPORT_FUNC(glViewport)
			IMPORT_FUNC(glBindBufferBase)
			IMPORT_FUNC(glUnmapBuffer)
			IMPORT_FUNC(glMapBufferRange)
			IMPORT_FUNC(glDrawBuffers)
			IMPORT_FUNC(glTexImage3D)
			IMPORT_FUNC(glTexSubImage3D)
			IMPORT_FUNC(glCopyTexSubImage3D)
			IMPORT_FUNC(glCompressedTexSubImage3D)
			IMPORT_FUNC(glGetBufferPointerv)
			IMPORT_FUNC(glBindVertexArray)
			IMPORT_FUNC(glDeleteVertexArrays)
			IMPORT_FUNC(glGenVertexArrays)

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		OpenGLES3Rhi&			  mOpenGLES3Rhi;			///< Owner OpenGL ES 3 RHI instance
		void*					  mEGLSharedLibrary;		///< EGL shared library, can be a null pointer
		void*					  mGLESSharedLibrary;		///< OpenGL ES 3 shared library, can be a null pointer
		bool					  mEntryPointsRegistered;	///< Entry points successfully registered?
		ExtensionsRuntimeLinking* mExtensions;				///< Extensions instance, always valid!


	};




	//[-------------------------------------------------------]
	//[ Global functions                                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Creates, loads and compiles a shader from source code
	*
	*  @param[in] openGLES3Rhi
	*    Owner OpenGL ES 3 RHI instance
	*  @param[in] shaderType
	*    Shader type (for example "GL_VERTEX_SHADER"
	*  @param[in] sourceCode
	*    Shader ASCII source code, must be a valid pointer
	*
	*  @return
	*    The OpenGL ES 3 shader, 0 on error, destroy the returned resource if you no longer need it
	*/
	[[nodiscard]] GLuint loadShaderFromSourcecode(OpenGLES3Rhi& openGLES3Rhi, GLenum shaderType, const GLchar* sourceCode)
	{
		// Create the shader object
		const GLuint openGLES3Shader = glCreateShader(shaderType);

		// Load the shader source
		glShaderSource(openGLES3Shader, 1, &sourceCode, nullptr);

		// Compile the shader
		glCompileShader(openGLES3Shader);

		// Check the compile status
		GLint compiled = GL_FALSE;
		glGetShaderiv(openGLES3Shader, GL_COMPILE_STATUS, &compiled);
		if (GL_TRUE == compiled)
		{
			// All went fine, return the shader
			return openGLES3Shader;
		}
		else
		{
			// Error, failed to compile the shader!

			{ // Get the length of the information
				GLint informationLength = 0;
				glGetShaderiv(openGLES3Shader, GL_INFO_LOG_LENGTH, &informationLength);
				if (informationLength > 1)
				{
					// Allocate memory for the information
					const Rhi::Context& context = openGLES3Rhi.getContext();
					GLchar* informationLog = RHI_MALLOC_TYPED(context, GLchar, informationLength);

					// Get the information
					glGetShaderInfoLog(openGLES3Shader, informationLength, nullptr, informationLog);

					// Output the debug string
					if (openGLES3Rhi.getContext().getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), informationLog))
					{
						DEBUG_BREAK;
					}

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}

			// Destroy the shader
			// -> A value of 0 for shader will be silently ignored
			glDeleteShader(openGLES3Shader);

			// Error!
			return 0;
		}
	}




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Mapping.h                                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 mapping
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
		*    "Rhi::FilterMode" to OpenGL ES 3 magnification filter mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    OpenGL ES 3 magnification filter mode
		*/
		[[nodiscard]] static GLint getOpenGLES3MagFilterMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode)
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
					return GL_LINEAR;	// There's no special setting in OpenGL ES 3

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
					return GL_LINEAR;	// There's no special setting in OpenGL ES 3

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "OpenGL ES 3 filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Rhi::FilterMode" to OpenGL ES 3 minification filter mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*  @param[in] hasMipmaps
		*    Are mipmaps available?
		*
		*  @return
		*    OpenGL ES 3 minification filter mode
		*/
		[[nodiscard]] static GLint getOpenGLES3MinFilterMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode, bool hasMipmaps)
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
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;	// There's no special setting in OpenGL ES 3

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
					return hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;	// There's no special setting in OpenGL ES 3

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "OpenGL ES 3 filter mode must not be unknown")
					return GL_NEAREST;

				default:
					return GL_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Rhi::FilterMode" to OpenGL ES 3 compare mode
		*
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    OpenGL ES 3 compare mode
		*/
		[[nodiscard]] inline static GLint getOpenGLES3CompareMode([[maybe_unused]] Rhi::FilterMode filterMode)
		{
			// "GL_COMPARE_REF_TO_TEXTURE" is not supported by OpenGL ES 3
			return GL_NONE;
		}

		//[-------------------------------------------------------]
		//[ Rhi::TextureAddressMode                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::TextureAddressMode" to OpenGL ES 3 texture address mode
		*
		*  @param[in] textureAddressMode
		*    "Rhi::TextureAddressMode" to map
		*
		*  @return
		*    OpenGL ES 3 texture address mode
		*/
		[[nodiscard]] static GLint getOpenGLES3TextureAddressMode(Rhi::TextureAddressMode textureAddressMode)
		{
			static constexpr GLint MAPPING[] =
			{
				GL_REPEAT,			// Rhi::TextureAddressMode::WRAP
				GL_MIRRORED_REPEAT,	// Rhi::TextureAddressMode::MIRROR
				GL_CLAMP_TO_EDGE,	// Rhi::TextureAddressMode::CLAMP
				GL_CLAMP_TO_EDGE,	// Rhi::TextureAddressMode::BORDER - Not supported by OpenGL ES 3
				GL_MIRRORED_REPEAT	// Rhi::TextureAddressMode::MIRROR_ONCE	// TODO(co) OpenGL ES 3 equivalent?
			};
			return MAPPING[static_cast<int>(textureAddressMode) - 1];	// Lookout! The "Rhi::TextureAddressMode"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::ComparisonFunc                                   ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::ComparisonFunc" to OpenGL ES 3 comparison function
		*
		*  @param[in] comparisonFunc
		*    "Rhi::ComparisonFunc" to map
		*
		*  @return
		*    OpenGL ES 3 comparison function
		*/
		[[nodiscard]] static GLenum getOpenGLES3ComparisonFunc(Rhi::ComparisonFunc comparisonFunc)
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
		*    "Rhi::VertexAttributeFormat" to OpenGL ES 3 size (number of elements)
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    OpenGL ES 3 size (number of elements)
		*/
		[[nodiscard]] static GLint getOpenGLES3Size(Rhi::VertexAttributeFormat vertexAttributeFormat)
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
		*    "Rhi::VertexAttributeFormat" to OpenGL ES 3 type
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    OpenGL ES 3 type
		*/
		[[nodiscard]] static GLenum getOpenGLES3Type(Rhi::VertexAttributeFormat vertexAttributeFormat)
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
		[[nodiscard]] static GLboolean isOpenGLES3VertexAttributeFormatNormalized(Rhi::VertexAttributeFormat vertexAttributeFormat)
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
		[[nodiscard]] static GLboolean isOpenGLES3VertexAttributeFormatInteger(Rhi::VertexAttributeFormat vertexAttributeFormat)
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
		//[ Rhi::BufferUsage                                      ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::BufferUsage" to OpenGL ES 3 usage
		*
		*  @param[in] bufferUsage
		*    "Rhi::BufferUsage" to map
		*
		*  @return
		*    OpenGL ES 3 usage
		*/
		[[nodiscard]] static GLenum getOpenGLES3Type(Rhi::BufferUsage bufferUsage)
		{
			// OpenGL ES 3 only supports: "STREAM_DRAW", "STATIC_DRAW" and "DYNAMIC_DRAW"

			// These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
			// -> This also means that we have to use a switch statement for the mapping
			switch (bufferUsage)
			{
				case Rhi::BufferUsage::STREAM_DRAW:
				case Rhi::BufferUsage::STREAM_READ:
				case Rhi::BufferUsage::STREAM_COPY:
					return GL_STREAM_DRAW;

				case Rhi::BufferUsage::STATIC_DRAW:
				case Rhi::BufferUsage::STATIC_READ:
				case Rhi::BufferUsage::STATIC_COPY:
					return GL_STATIC_DRAW;

				case Rhi::BufferUsage::DYNAMIC_DRAW:
				case Rhi::BufferUsage::DYNAMIC_READ:
				case Rhi::BufferUsage::DYNAMIC_COPY:
				default:
					return GL_DYNAMIC_DRAW;
			}
		}

		//[-------------------------------------------------------]
		//[ Rhi::IndexBufferFormat                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::IndexBufferFormat" to OpenGL ES 3 type
		*
		*  @param[in] indexBufferFormat
		*    "Rhi::IndexBufferFormat" to map
		*
		*  @return
		*    OpenGL ES 3 type
		*/
		[[nodiscard]] static GLenum getOpenGLES3Type(Rhi::IndexBufferFormat::Enum indexBufferFormat)
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
		*    "Rhi::TextureFormat" to OpenGL ES 3 internal format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    OpenGL ES 3 internal format
		*/
		[[nodiscard]] static GLenum getOpenGLES3InternalFormat(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_R8,								// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_RGB,								// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_RGBA,							// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_RGBA,							// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) OpenGL ES 3 sRGB format
				GL_RGBA,							// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha	TODO(co) Texture format isn't supported
				GL_R11F_G11F_B10F,					// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - available in OpenGL ES 3
				GL_RGBA16F,							// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				GL_RGBA32F,							// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,	// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - "GL_EXT_texture_compression_dxt1" OpenGL ES extension
				GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,	// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - "GL_EXT_texture_compression_dxt1" OpenGL ES extension; sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) OpenGL ES 3 sRGB format
				GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,	// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - "GL_EXT_texture_compression_s3tc" OpenGL ES extension
				GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,	// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - "GL_EXT_texture_compression_s3tc" OpenGL ES extension; sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) OpenGL ES 3 sRGB format
				GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,	// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - "GL_EXT_texture_compression_s3tc" OpenGL ES extension
				GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,	// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - "GL_EXT_texture_compression_s3tc" OpenGL ES extension; sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) OpenGL ES 3 sRGB format
				GL_3DC_X_AMD,						// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - "GL_AMD_compressed_3DC_texture" OpenGL ES extension
				GL_3DC_XY_AMD,						// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - "GL_AMD_compressed_3DC_texture" OpenGL ES extension
				GL_ETC1_RGB8_OES,					// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices
				GL_R16_EXT,							// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_R32UI,							// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_R32F,							// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_DEPTH_COMPONENT32F,				// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				0,									// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel	TODO(co) "GL_RG16_SNORM" OpenGL ES 3 needs https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_norm16.txt
				GL_RG16F,							// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0									// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Rhi::TextureFormat" to OpenGL ES 3 format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    OpenGL ES 3 format
		*/
		[[nodiscard]] static GLenum getOpenGLES3Format(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_RED,				// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_RGB,				// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_RGBA,			// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_RGBA,			// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_RGBA,			// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha	TODO(co) Texture format isn't supported
				GL_RGB,				// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - available in OpenGL ES 3
				GL_RGBA,			// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha) - Not supported by OpenGL ES 3
				GL_RGBA,			// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha) - Not supported by OpenGL ES 3
				0,					// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - Compressed format, so not supported in here
				0,					// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - Compressed format, so not supported in here
				GL_RED,				// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_RED_INTEGER,		// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_RED,				// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_DEPTH_COMPONENT,	// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_RG,				// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_RG,				// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0					// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    "Rhi::TextureFormat" to OpenGL ES 3 type
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    OpenGL ES 3 type
		*/
		[[nodiscard]] static GLenum getOpenGLES3Type(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr GLenum MAPPING[] =
			{
				GL_UNSIGNED_BYTE,					// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				GL_UNSIGNED_BYTE,					// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				GL_UNSIGNED_BYTE,					// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_UNSIGNED_BYTE,					// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				GL_UNSIGNED_BYTE,					// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				GL_UNSIGNED_INT_10F_11F_11F_REV,	// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - available in OpenGL ES 3
				GL_FLOAT,							// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha) - Not supported by OpenGL ES 3
				GL_FLOAT,							// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha) - Not supported by OpenGL ES 3
				0,									// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block) - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = hardware gamma correction - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block) - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - Compressed format, so not supported in here
				0,									// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - Compressed format, so not supported in here
				GL_UNSIGNED_SHORT,					// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				GL_UNSIGNED_INT,					// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				GL_FLOAT,							// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				GL_FLOAT,							// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				GL_BYTE,							// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				GL_FLOAT,							// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				0									// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		//[-------------------------------------------------------]
		//[ Rhi::PrimitiveTopology                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::PrimitiveTopology" to OpenGL ES 3 type
		*
		*  @param[in] primitiveTopology
		*    "Rhi::PrimitiveTopology" to map
		*
		*  @return
		*    OpenGL ES 3 type
		*/
		[[nodiscard]] static GLenum getOpenGLES3Type(Rhi::PrimitiveTopology primitiveTopology)
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
		*    "Rhi::MapType" to OpenGL ES 3 access bit field for "glMapBufferRange()"
		*
		*  @param[in] mapType
		*    "Rhi::MapType" to map
		*
		*  @return
		*    OpenGL ES 3 type
		*/
		[[nodiscard]] static GLbitfield getOpenGLES3MapRangeType(Rhi::MapType mapType)
		{
			// OpenGL ES 3 defines access bits for "glMapBufferRange()"
			static constexpr GLbitfield MAPPING[] =
			{
				GL_MAP_READ_BIT,					// Rhi::MapType::READ
				GL_MAP_WRITE_BIT,					// Rhi::MapType::WRITE
				GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,	// Rhi::MapType::READ_WRITE
				GL_MAP_WRITE_BIT,					// Rhi::MapType::WRITE_DISCARD
				GL_MAP_WRITE_BIT					// Rhi::MapType::WRITE_NO_OVERWRITE
			};
			return MAPPING[static_cast<int>(mapType) - 1];	// Lookout! The "Rhi::MapType"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::MapType                                          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::Blend" to OpenGL ES 3 type
		*
		*  @param[in] blend
		*    "Rhi::Blend" to map
		*
		*  @return
		*    OpenGL ES 3 type
		*/
		[[nodiscard]] static GLenum getOpenGLES3BlendType(Rhi::Blend blend)
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
					GL_SRC_COLOR,			// Rhi::Blend::BLEND_FACTOR			TODO(co) Mapping "Rhi::Blend::BLEND_FACTOR" to OpenGL ES 3 possible?
					GL_ONE_MINUS_SRC_COLOR,	// Rhi::Blend::INV_BLEND_FACTOR		TODO(co) Mapping "Rhi::Blend::INV_BLEND_FACTOR" to OpenGL ES 3 possible?
					GL_SRC_COLOR,			// Rhi::Blend::SRC_1_COLOR			TODO(co) Mapping "Rhi::Blend::SRC_1_COLOR" to OpenGL ES 3 possible?
					GL_ONE_MINUS_SRC_COLOR,	// Rhi::Blend::INV_SRC_1_COLOR		TODO(co) Mapping "Rhi::Blend::INV_SRC_1_COLOR" to OpenGL ES 3 possible?
					GL_SRC_COLOR,			// Rhi::Blend::SRC_1_ALPHA			TODO(co) Mapping "Rhi::Blend::SRC_1_ALPHA" to OpenGL ES 3 possible?
					GL_ONE_MINUS_SRC_COLOR,	// Rhi::Blend::INV_SRC_1_ALPHA		TODO(co) Mapping "Rhi::Blend::INV_SRC_1_ALPHA" to OpenGL ES 3 possible?
				};
				return MAPPING[static_cast<int>(blend) - static_cast<int>(Rhi::Blend::BLEND_FACTOR)];
			}
		}


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/ResourceGroup.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 resource group class
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
		*  @param[in] openGLES3Rhi
		*    OpenGL ES 3 RHI owner
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
		ResourceGroup(OpenGLES3Rhi& openGLES3Rhi, const Rhi::RootSignature& rootSignature, uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IResourceGroup(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootParameterIndex(rootParameterIndex),
			mNumberOfResources(numberOfResources),
			mResources(RHI_MALLOC_TYPED(openGLES3Rhi.getContext(), Rhi::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr),
			mResourceIndexToUniformBlockBindingIndex(nullptr)
		{
			// Get the uniform block binding start index
			const Rhi::Context& context = openGLES3Rhi.getContext();
			const bool isGL_EXT_texture_buffer = openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_EXT_texture_buffer();
			uint32_t uniformBlockBindingIndex = 0;
			for (uint32_t currentRootParameterIndex = 0; currentRootParameterIndex < rootParameterIndex; ++currentRootParameterIndex)
			{
				const Rhi::RootParameter& rootParameter = rootSignature.parameters[currentRootParameterIndex];
				if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
				{
					RHI_ASSERT(openGLES3Rhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL ES 3 descriptor ranges")
					const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
					for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
					{
						const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];
						if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
						{
							++uniformBlockBindingIndex;
						}
						else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType && !isGL_EXT_texture_buffer && nullptr != strstr(descriptorRange.baseShaderRegisterName, "TextureBuffer"))
						{
							// Texture buffer emulation using uniform buffer
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
				RHI_ASSERT(openGLES3Rhi.getContext(), nullptr != resource, "Invalid OpenGL ES 3 resource")
				mResources[resourceIndex] = resource;
				resource->addReference();

				// Uniform block binding index handling
				bool isUniformBuffer = false;
				const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];
				if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
				{
					isUniformBuffer = true;
				}
				else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType && !isGL_EXT_texture_buffer && nullptr != strstr(descriptorRange.baseShaderRegisterName, "TextureBuffer"))
				{
					// Texture buffer emulation using uniform buffer
					isUniformBuffer = true;
				}
				if (isUniformBuffer)
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
	//[ OpenGLES3Rhi/RootSignature.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(OpenGLES3Rhi& openGLES3Rhi, const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRootSignature(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature)
		{
			const Rhi::Context& context = openGLES3Rhi.getContext();

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
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(openGLES3Rhi.getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The OpenGL ES 3 root parameter index is out-of-bounds")
			RHI_ASSERT(openGLES3Rhi.getContext(), numberOfResources > 0, "The number of OpenGL ES 3 resources must not be zero")
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr != resources, "The OpenGL ES 3 resource pointers must be valid")

			// Create resource group
			return RHI_NEW(openGLES3Rhi.getContext(), ResourceGroup)(openGLES3Rhi, getRootSignature(), rootParameterIndex, numberOfResources, resources, samplerStates RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
	//[ OpenGLES3Rhi/Buffer/VertexBuffer.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
	*/
	class VertexBuffer final : public Rhi::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexBuffer(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3ArrayBuffer(0),
			mBufferSize(numberOfBytes)
		{
			// Create the OpenGL ES 3 array buffer
			glGenBuffers(1, &mOpenGLES3ArrayBuffer);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 array buffer
				GLint openGLES3ArrayBufferBackup = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &openGLES3ArrayBufferBackup);
			#endif

			// Bind this OpenGL ES 3 array buffer and upload the data
			glBindBuffer(GL_ARRAY_BUFFER, mOpenGLES3ArrayBuffer);
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(numberOfBytes), data, Mapping::getOpenGLES3Type(bufferUsage));

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 array buffer
				glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(openGLES3ArrayBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VBO", 6)	// 6 = "VBO: " including terminating zero
					glObjectLabelKHR(GL_BUFFER_KHR, mOpenGLES3ArrayBuffer, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBuffer() override
		{
			// Destroy the OpenGL ES 3 array buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffers(1, &mOpenGLES3ArrayBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 array buffer
		*
		*  @return
		*    The OpenGL ES 3 array buffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3ArrayBuffer() const
		{
			return mOpenGLES3ArrayBuffer;
		}

		[[nodiscard]] inline uint32_t getBufferSize() const
		{
			return mBufferSize;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBuffer(const VertexBuffer& source) = delete;
		VertexBuffer& operator =(const VertexBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint	 mOpenGLES3ArrayBuffer;	///< OpenGL ES 3 array buffer, can be zero if no resource is allocated
		uint32_t mBufferSize;			///< Holds the size of the buffer


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/IndexBuffer.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 index buffer object (IBO, "element array buffer" in OpenGL terminology) class
	*/
	class IndexBuffer final : public Rhi::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndexBuffer(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3ElementArrayBuffer(0),
			mOpenGLES3Type(GL_UNSIGNED_SHORT),
			mIndexSizeInBytes(Rhi::IndexBufferFormat::getNumberOfBytesPerElement(indexBufferFormat)),
			mBufferSize(numberOfBytes)
		{
			// "GL_UNSIGNED_INT" is only allowed when the "GL_OES_element_index_uint" extension is there
			if (Rhi::IndexBufferFormat::UNSIGNED_INT != indexBufferFormat || openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_OES_element_index_uint())
			{
				// Create the OpenGL ES 3 element array buffer
				glGenBuffers(1, &mOpenGLES3ElementArrayBuffer);

				// Set the OpenGL ES 3 index buffer data type
				mOpenGLES3Type = Mapping::getOpenGLES3Type(indexBufferFormat);

				#ifdef RHI_OPENGLES3_STATE_CLEANUP
					// Backup the currently bound OpenGL ES 3 element array buffer
					GLint openGLES3ElementArrayBufferBackup = 0;
					glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &openGLES3ElementArrayBufferBackup);
				#endif

				// Bind this OpenGL ES 3 element array buffer and upload the data
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mOpenGLES3ElementArrayBuffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(numberOfBytes), data, Mapping::getOpenGLES3Type(bufferUsage));

				#ifdef RHI_OPENGLES3_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL ES 3 element array buffer
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(openGLES3ElementArrayBufferBackup));
				#endif

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IBO", 6)	// 6 = "IBO: " including terminating zero
						glObjectLabelKHR(GL_BUFFER_KHR, mOpenGLES3ElementArrayBuffer, -1, detailedDebugName);
					}
				#endif
			}
			else
			{
				// Error!
				RHI_ASSERT(openGLES3Rhi.getContext(), false, "\"GL_UNSIGNED_INT\" is only allowed in case the \"GL_OES_element_index_uint\" extension is there")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBuffer() override
		{
			// Destroy the OpenGL ES 3 element array buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffers(1, &mOpenGLES3ElementArrayBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 element array buffer
		*
		*  @return
		*    The OpenGL ES 3 element array buffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3ElementArrayBuffer() const
		{
			return mOpenGLES3ElementArrayBuffer;
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 element array buffer data type
		*
		*  @return
		*    The OpenGL ES 3 element array buffer data type
		*/
		[[nodiscard]] inline GLenum getOpenGLES3Type() const
		{
			return mOpenGLES3Type;
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

		[[nodiscard]] inline uint32_t getBufferSize() const
		{
			return mBufferSize;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBuffer(const IndexBuffer& source) = delete;
		IndexBuffer& operator =(const IndexBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint	 mOpenGLES3ElementArrayBuffer;	///< OpenGL ES 3 element array buffer, can be zero if no resource is allocated
		GLenum   mOpenGLES3Type;				///< OpenGL ES 3 element array buffer data type
		uint32_t mIndexSizeInBytes;				///< Number of bytes of an index
		uint32_t mBufferSize;					///< Holds the size of the buffer


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/VertexArray.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 vertex array class, effective vertex array object (VAO)
	*/
	class VertexArray final : public Rhi::IVertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
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
		VertexArray(OpenGLES3Rhi& openGLES3Rhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexArray(openGLES3Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3VertexArray(0),
			mNumberOfVertexBuffers(numberOfVertexBuffers),
			mVertexBuffers((mNumberOfVertexBuffers > 0) ? RHI_MALLOC_TYPED(openGLES3Rhi.getContext(), VertexBuffer*, mNumberOfVertexBuffers) : nullptr),	// Guaranteed to be filled below, so we don't need to care to initialize the content in here
			mIndexBuffer(indexBuffer)
		{
			// Create the OpenGL ES 3 vertex array
			glGenVertexArrays(1, &mOpenGLES3VertexArray);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 array buffer
				GLint openGLES3ArrayBufferBackup = 0;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &openGLES3ArrayBufferBackup);

				// Backup the currently bound OpenGL ES 3 element array buffer
				GLint openGLES3ElementArrayBufferBackup = 0;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &openGLES3ElementArrayBufferBackup);

				// Backup the currently bound OpenGL ES 3 vertex array
				GLint openGLES3VertexArrayBackup = 0;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING_OES, &openGLES3VertexArrayBackup);
			#endif

			// Bind this OpenGL ES 3 vertex array
			glBindVertexArray(mOpenGLES3VertexArray);

			{ // Add a reference to the used vertex buffers
				VertexBuffer** currentVertexBuffers = mVertexBuffers;
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentVertexBuffers)
				{
					// Add a reference to the used vertex buffer
					// TODO(co) Add security check: Is the given resource one of the currently used RHI?
					*currentVertexBuffers = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
					(*currentVertexBuffers)->addReference();
				}
			}

			{ // Enable OpenGL ES 3 vertex attribute arrays
				// Loop through all attributes
				// -> We're using "glBindAttribLocation()" when linking the program so we have known attribute locations (the vertex array can't know about the program)
				GLuint attributeLocation = 0;
				const Rhi::VertexAttribute* attributeEnd = vertexAttributes.attributes + vertexAttributes.numberOfAttributes;
				for (const Rhi::VertexAttribute* attribute = vertexAttributes.attributes; attribute < attributeEnd; ++attribute, ++attributeLocation)
				{
					// Set the OpenGL ES 3 vertex attribute pointer
					const Rhi::VertexArrayVertexBuffer& vertexArrayVertexBuffer = vertexBuffers[attribute->inputSlot];
					glBindBuffer(GL_ARRAY_BUFFER, static_cast<VertexBuffer*>(vertexArrayVertexBuffer.vertexBuffer)->getOpenGLES3ArrayBuffer());
					if (Mapping::isOpenGLES3VertexAttributeFormatInteger(attribute->vertexAttributeFormat))
					{
						glVertexAttribIPointer(attributeLocation,
											   Mapping::getOpenGLES3Size(attribute->vertexAttributeFormat),
											   Mapping::getOpenGLES3Type(attribute->vertexAttributeFormat),
											   static_cast<GLsizei>(attribute->strideInBytes),
											   reinterpret_cast<void*>(static_cast<uintptr_t>(attribute->alignedByteOffset)));
					}
					else
					{
						glVertexAttribPointer(attributeLocation,
											  Mapping::getOpenGLES3Size(attribute->vertexAttributeFormat),
											  Mapping::getOpenGLES3Type(attribute->vertexAttributeFormat),
											  Mapping::isOpenGLES3VertexAttributeFormatNormalized(attribute->vertexAttributeFormat),
											  static_cast<GLsizei>(attribute->strideInBytes),
											  reinterpret_cast<void*>(static_cast<uintptr_t>(attribute->alignedByteOffset)));
					}

					// Set divisor
					if (attribute->instancesPerElement > 0)
					{
						glVertexAttribDivisor(attributeLocation, attribute->instancesPerElement);
					}

					// Enable OpenGL ES 3 vertex attribute array
					glEnableVertexAttribArray(attributeLocation);
				}

				// Set the used index buffer
				// -> In case of no index buffer we don't bind buffer 0, there's not really a point in it
				if (nullptr != indexBuffer)
				{
					// Bind OpenGL ES 3 element array buffer
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getOpenGLES3ElementArrayBuffer());
				}
			}

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 vertex array
				glBindVertexArray(static_cast<GLuint>(openGLES3VertexArrayBackup));

				// Be polite and restore the previous bound OpenGL ES 3 element array buffer
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(openGLES3ElementArrayBufferBackup));

				// Be polite and restore the previous bound OpenGL ES 3 array buffer
				glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(openGLES3ArrayBufferBackup));
			#endif

			// Add a reference to the given index buffer
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->addReference();
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VAO", 6)	// 6 = "VAO: " including terminating zero
					glObjectLabelKHR(GL_VERTEX_ARRAY_KHR, mOpenGLES3VertexArray, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexArray() override
		{
			// Destroy the OpenGL ES 3 vertex array
			// -> Silently ignores 0's and names that do not correspond to existing vertex array objects
			glDeleteVertexArrays(1, &mOpenGLES3VertexArray);

			// Release the reference to the used vertex buffers
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());
			if (nullptr != mVertexBuffers)
			{
				// Release references
				VertexBuffer** vertexBuffersEnd = mVertexBuffers + mNumberOfVertexBuffers;
				for (VertexBuffer** vertexBuffer = mVertexBuffers; vertexBuffer < vertexBuffersEnd; ++vertexBuffer)
				{
					(*vertexBuffer)->releaseReference();
				}

				// Cleanup
				RHI_FREE(openGLES3Rhi.getContext(), mVertexBuffers);
			}

			// Release the index buffer reference
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->releaseReference();
			}

			// Free the unique compact vertex array ID
			openGLES3Rhi.VertexArrayMakeId.DestroyID(getId());
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
		*    Return the OpenGL ES 3 vertex array
		*
		*  @return
		*    The OpenGL ES 3 vertex array, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3VertexArray() const
		{
			return mOpenGLES3VertexArray;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArray(const VertexArray& source) = delete;
		VertexArray& operator =(const VertexArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint		   mOpenGLES3VertexArray;	///< OpenGL ES 3 vertex array, can be zero if no resource is allocated
		uint32_t	   mNumberOfVertexBuffers;	///< Number of vertex buffers
		VertexBuffer** mVertexBuffers;			///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer
		IndexBuffer*   mIndexBuffer;			///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/TextureBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL ES 3 texture buffer object (TBO) interface
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
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);

			// Destroy the OpenGL ES 3  texture buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffers(1, &mOpenGLES3TextureBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture buffer instance
		*
		*  @return
		*    The OpenGL ES 3 texture buffer instance, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3TextureBuffer() const
		{
			return mOpenGLES3TextureBuffer;
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture instance
		*
		*  @return
		*    The OpenGL ES 3 texture instance, can be zero if no resource is allocated
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
		}

		[[nodiscard]] inline uint32_t getBufferSize() const
		{
			return mBufferSize;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*/
		TextureBuffer(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITextureBuffer(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3TextureBuffer(0),
			mOpenGLES3Texture(0),
			mBufferSize(numberOfBytes)
		{
			// Create the OpenGL ES 3 texture buffer
			glGenBuffers(1, &mOpenGLES3TextureBuffer);

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
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
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		GLuint	 mOpenGLES3TextureBuffer;	///< OpenGL ES 3 texture buffer, can be zero if no resource is allocated
		GLuint	 mOpenGLES3Texture;			///< OpenGL ES 3 texture, can be zero if no resource is allocated
		uint32_t mBufferSize;				///< Holds the size of the buffer


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/TextureBufferBind.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 texture buffer object (TBO) class, traditional bind version
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
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBufferBind(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			TextureBuffer(openGLES3Rhi, numberOfBytes RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			{ // Buffer part
				#ifdef RHI_OPENGLES3_STATE_CLEANUP
					// Backup the currently bound OpenGL ES 3 texture buffer
					GLint openGLES3TextureBufferBackup = 0;
					glGetIntegerv(GL_TEXTURE_BINDING_BUFFER_EXT, &openGLES3TextureBufferBackup);
				#endif

				// Bind this OpenGL ES 3 texture buffer and upload the data
				glBindBuffer(GL_TEXTURE_BUFFER_EXT, mOpenGLES3TextureBuffer);
				// -> Usage: These constants directly map to "GL_ARB_vertex_buffer_object" and OpenGL ES 3 constants, do not change them
				glBufferData(GL_TEXTURE_BUFFER_EXT, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

				#ifdef RHI_OPENGLES3_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL ES 3 texture buffer
					glBindBuffer(GL_TEXTURE_BUFFER_EXT, static_cast<GLuint>(openGLES3TextureBufferBackup));
				#endif
			}

			{ // Texture part
				#ifdef RHI_OPENGLES3_STATE_CLEANUP
					// Backup the currently bound OpenGL ES 3 texture
					GLint openGLESTextureBackup = 0;
					glGetIntegerv(GL_TEXTURE_BUFFER_BINDING_EXT, &openGLESTextureBackup);
				#endif

				// Make this OpenGL ES 3 texture instance to the currently used one
				glBindTexture(GL_TEXTURE_BUFFER_EXT, mOpenGLES3Texture);

				// Attaches the storage for the buffer object to the active buffer texture
				glTexBufferEXT(GL_TEXTURE_BUFFER_EXT, Mapping::getOpenGLES3InternalFormat(textureFormat), mOpenGLES3TextureBuffer);

				#ifdef RHI_OPENGLES3_STATE_CLEANUP
					// Be polite and restore the previous bound OpenGL ES 3 texture
					glBindTexture(GL_TEXTURE_BUFFER_EXT, static_cast<GLuint>(openGLESTextureBackup));
				#endif
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6)	// 6 = "TBO: " including terminating zero
					if (0 != mOpenGLES3Texture)
					{
						glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
					}
					if (0 != mOpenGLES3TextureBuffer)
					{
						glObjectLabelKHR(GL_BUFFER_KHR, mOpenGLES3TextureBuffer, -1, detailedDebugName);
					}
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
	//[ OpenGLES3Rhi/Buffer/TextureBufferBindEmulation.h      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 texture buffer object (TBO) class, traditional bind version and uniform buffer to emulate texture buffer behaviour (with limitations, of course)
	*/
	class TextureBufferBindEmulation final : public TextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBufferBindEmulation(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			TextureBuffer(openGLES3Rhi, numberOfBytes RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 uniform buffer
				GLint openGLES3UniformBufferBackup = 0;
				glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &openGLES3UniformBufferBackup);
			#endif

			// TODO(co) Review OpenGL ES 3 uniform buffer alignment topic

			// Bind this OpenGL ES 3 uniform buffer and upload the data
			glBindBuffer(GL_UNIFORM_BUFFER, mOpenGLES3TextureBuffer);
			// -> Usage: These constants directly map to GL_EXT_vertex_buffer_object and OpenGL ES 3 constants, do not change them
			glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 uniform buffer
				glBindBuffer(GL_UNIFORM_BUFFER, static_cast<GLuint>(openGLES3UniformBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6)	// 6 = "TBO: " including terminating zero
					glObjectLabelKHR(GL_BUFFER_KHR, mOpenGLES3TextureBuffer, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureBufferBindEmulation() override
		{}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBufferBindEmulation(const TextureBufferBindEmulation& source) = delete;
		TextureBufferBindEmulation& operator =(const TextureBufferBindEmulation& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/IndirectBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 indirect buffer object emulation class
	*/
	class IndirectBuffer final : public Rhi::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Rhi::IndirectBufferFlag"
		*/
		IndirectBuffer(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t indirectBufferFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IIndirectBuffer(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfBytes(numberOfBytes),
			mData(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(openGLES3Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid OpenGL ES 3 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RHI_ASSERT(openGLES3Rhi.getContext(), !((indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid OpenGL ES 3 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RHI_ASSERT(openGLES3Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawArguments)) == 0, "OpenGL ES 3 indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RHI_ASSERT(openGLES3Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawIndexedArguments)) == 0, "OpenGL ES 3 indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// Copy data
			if (mNumberOfBytes > 0)
			{
				mData = RHI_MALLOC_TYPED(openGLES3Rhi.getContext(), uint8_t, mNumberOfBytes);
				if (nullptr != data)
				{
					memcpy(mData, data, mNumberOfBytes);
				}
			}
			else
			{
				RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == data, "Invalid OpenGL ES 3 indirect buffer data")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{
			RHI_FREE(getRhi().getContext(), mData);
		}

		/**
		*  @brief
		*    Return writable indirect buffer emulation data pointer
		*
		*  @return
		*    Writable indirect buffer emulation data pointer, can be a null pointer, don't destroy the returned instance
		*/
		[[nodiscard]] inline uint8_t* getWritableEmulationData() const
		{
			return mData;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IIndirectBuffer methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const uint8_t* getEmulationData() const override
		{
			return mData;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBuffer(const IndirectBuffer& source) = delete;
		IndirectBuffer& operator =(const IndirectBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t mNumberOfBytes;
		uint8_t* mData;				///< Indirect buffer data, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/UniformBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenGL ES uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
	*/
	class UniformBuffer final : public Rhi::IUniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES3 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(OpenGLES3Rhi& openGLES3Rhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IUniformBuffer(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3UniformBuffer(0),
			mBufferSize(numberOfBytes)
		{
			// Create the OpenGL ES 3 uniform buffer
			glGenBuffers(1, &mOpenGLES3UniformBuffer);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 uniform buffer
				GLint openGLES3UniformBufferBackup = 0;
				glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &openGLES3UniformBufferBackup);
			#endif

			// TODO(co) Review OpenGL ES 3 uniform buffer alignment topic

			// Bind this OpenGL ES 3 uniform buffer and upload the data
			glBindBuffer(GL_UNIFORM_BUFFER, mOpenGLES3UniformBuffer);
			// -> Usage: These constants directly map to GL_EXT_vertex_buffer_object and OpenGL ES 3 constants, do not change them
			glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(numberOfBytes), data, static_cast<GLenum>(bufferUsage));

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 uniform buffer
				glBindBuffer(GL_UNIFORM_BUFFER, static_cast<GLuint>(openGLES3UniformBufferBackup));
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "UBO", 6)	// 6 = "UBO: " including terminating zero
					glObjectLabelKHR(GL_BUFFER_KHR, mOpenGLES3UniformBuffer, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBuffer() override
		{
			// Destroy the OpenGL ES 3 uniform buffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteBuffers(1, &mOpenGLES3UniformBuffer);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 uniform buffer instance
		*
		*  @return
		*    The OpenGL ES 3 uniform buffer instance, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3UniformBuffer() const
		{
			return mOpenGLES3UniformBuffer;
		}

		[[nodiscard]] inline uint32_t getBufferSize() const
		{
			return mBufferSize;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBuffer(const UniformBuffer& source) = delete;
		UniformBuffer& operator =(const UniformBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint	 mOpenGLES3UniformBuffer;	///< OpenGL ES 3 uniform buffer, can be zero if no resource is allocated
		uint32_t mBufferSize;				///< Holds the size of the buffer


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Buffer/BufferManager.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 buffer manager interface
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
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*/
		inline explicit BufferManager(OpenGLES3Rhi& openGLES3Rhi) :
			IBufferManager(openGLES3Rhi),
			mExtensions(&openGLES3Rhi.getOpenGLES3Context().getExtensions())
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
		[[nodiscard]] inline virtual Rhi::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());
			return RHI_NEW(openGLES3Rhi.getContext(), VertexBuffer)(openGLES3Rhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::IndexBufferFormat::Enum indexBufferFormat = Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());
			return RHI_NEW(openGLES3Rhi.getContext(), IndexBuffer)(openGLES3Rhi, numberOfBytes, data, bufferUsage, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexArray* createVertexArray(const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, Rhi::IIndexBuffer* indexBuffer = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity checks
			#ifdef RHI_DEBUG
			{
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RHI_ASSERT(openGLES3Rhi.getContext(), &openGLES3Rhi == &vertexBuffer->vertexBuffer->getRhi(), "OpenGL ES 3 error: The given vertex buffer resource is owned by another RHI instance")
				}
			}
			#endif
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == indexBuffer || &openGLES3Rhi == &indexBuffer->getRhi(), "OpenGL ES 3 error: The given index buffer resource is owned by another RHI instance")

			// Create vertex array
			uint16_t id = 0;
			if (openGLES3Rhi.VertexArrayMakeId.CreateID(id))
			{
				return RHI_NEW(openGLES3Rhi.getContext(), VertexArray)(openGLES3Rhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), (numberOfBytes % Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The OpenGL ES 3 texture buffer size must be a multiple of the selected texture format bytes per texel")

			// Is "GL_EXT_texture_buffer" there?
			if (mExtensions->isGL_EXT_texture_buffer())
			{
				// TODO(co) Add security check: Is the given resource one of the currently used RHI?
				return RHI_NEW(openGLES3Rhi.getContext(), TextureBufferBind)(openGLES3Rhi, numberOfBytes, data, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}

			// We can only emulate the "Rhi::TextureFormat::R32G32B32A32F" texture format using an uniform buffer
			else if (Rhi::TextureFormat::R32G32B32A32F == textureFormat)
			{
				// TODO(co) Add security check: Is the given resource one of the currently used RHI?
				return RHI_NEW(openGLES3Rhi.getContext(), TextureBufferBindEmulation)(openGLES3Rhi, numberOfBytes, data, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
			}

			// Error!
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IStructuredBuffer* createStructuredBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t bufferFlags, [[maybe_unused]] Rhi::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Sanity checks
			RHI_ASSERT(getRhi().getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The OpenGL ES 3 structured buffer size must be a multiple of the given number of structure bytes")
			RHI_ASSERT(getRhi().getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The OpenGL ES 3 structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// TODO(co) Add OpenGL ES structured buffer support ("GL_EXT_buffer_storage"-extension)
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());
			return RHI_NEW(openGLES3Rhi.getContext(), IndirectBuffer)(openGLES3Rhi, numberOfBytes, data, indirectBufferFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// -> Inside GLSL "layout(binding = 0, std140) writeonly uniform OutputUniformBuffer" will result in the GLSL compiler error "Failed to parse the GLSL shader source code: ERROR: 0:85: 'assign' :  l-value required "anon@6" (can't modify a uniform)"
			// -> Inside GLSL "layout(binding = 0, std430) writeonly buffer  OutputUniformBuffer" will work in OpenGL but will fail in Vulkan with "Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT" Object: "0" Location: "0" Message code: "13" Layer prefix: "Validation" Message: "Object: VK_NULL_HANDLE (Type = 0) | Type mismatch on descriptor slot 0.0 (used as type `ptr to uniform struct of (vec4 of float32)`) but descriptor of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER""
			// RHI_ASSERT(openGLES3Rhi.getContext(), (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid OpenGL ES 3 buffer flags, uniform buffer can't be used for unordered access")
			// RHI_ASSERT(openGLES3Rhi.getContext(), (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0, "Invalid OpenGL ES 3 buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RHI_NEW(openGLES3Rhi.getContext(), UniformBuffer)(openGLES3Rhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
		const IExtensions* mExtensions;	///< Extensions instance, always valid


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/Texture1D.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 1D texture class
	*/
	class Texture1D final : public Rhi::ITexture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1D(OpenGLES3Rhi& openGLES3Rhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1D(openGLES3Rhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Texture(0)
		{
			// OpenGL ES 3 has no 1D textures, just use a 2D texture with a height of one

			// Sanity checks
			RHI_ASSERT(openGLES3Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL ES 3 texture parameters")
			RHI_ASSERT(openGLES3Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL ES 3 render target textures can't be filled using provided data")

			// TODO(co) Check support formats

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLES3AlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLES3AlignmentBackup);

				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLES3TextureBackup);
			#endif

			// Set correct alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
			glBindTexture(GL_TEXTURE_2D, mOpenGLES3Texture);

			// Upload the texture data
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(1), 0, numberOfBytesPerSlice, data);

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glCompressedTexImage2D(GL_TEXTURE_2D, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(1), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1)), data);
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					const GLenum format = Mapping::getOpenGLES3Format(textureFormat);
					const GLenum type = Mapping::getOpenGLES3Type(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1));
						glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(1), 0, format, type, data);

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glTexImage2D(GL_TEXTURE_2D, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(1), 0, Mapping::getOpenGLES3Format(textureFormat), Mapping::getOpenGLES3Type(textureFormat), data);
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLES3TextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLES3AlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture", 13)	// 13 = "1D texture: " including terminating zero
					glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1D() override
		{
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture
		*
		*  @return
		*    The OpenGL ES 3 texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLES3Texture));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1D(const Texture1D& source) = delete;
		Texture1D& operator =(const Texture1D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLES3Texture;	///< OpenGL ES 3 texture, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/Texture1DArray.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 1D array texture class
	*/
	class Texture1DArray final : public Rhi::ITexture1DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
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
		Texture1DArray(OpenGLES3Rhi& openGLES3Rhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1DArray(openGLES3Rhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Texture(0)
		{
			// OpenGL ES 3 has no 1D texture arrays, just use a 2D texture array with a height of one

			// TODO(co) Check support formats

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLES3AlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLES3AlignmentBackup);

				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &openGLES3TextureBackup);
			#endif

			// Set correct alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, mOpenGLES3Texture);

			// TODO(co) Add support for user provided mipmaps
			// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
			//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   etc.

			// Upload the base map of the texture (mipmaps are automatically created as soon as the base map is changed)
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), 1, static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLES3Format(textureFormat), Mapping::getOpenGLES3Type(textureFormat), data);

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
			{
				glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(openGLES3TextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLES3AlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture array", 19)	// 19 = "1D texture array: " including terminating zero
					glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DArray() override
		{
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture
		*
		*  @return
		*    The OpenGL ES 3 texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DArray(const Texture1DArray& source) = delete;
		Texture1DArray& operator =(const Texture1DArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLES3Texture;	///< OpenGL ES 3 texture, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/Texture2D.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 2D texture class
	*/
	class Texture2D final : public Rhi::ITexture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
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
		*/
		Texture2D(OpenGLES3Rhi& openGLES3Rhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2D(openGLES3Rhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Texture(0)
		{
			// Sanity checks
			RHI_ASSERT(openGLES3Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL ES 3 texture parameters")
			RHI_ASSERT(openGLES3Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL ES 3 render target textures can't be filled using provided data")

			// TODO(co) Check support formats

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLES3AlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLES3AlignmentBackup);

				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLES3TextureBackup);
			#endif

			// Set correct alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
			glBindTexture(GL_TEXTURE_2D, mOpenGLES3Texture);

			// Upload the texture data
			if (Rhi::TextureFormat::isCompressed(textureFormat))
			{
				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, numberOfBytesPerSlice, data);

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glCompressedTexImage2D(GL_TEXTURE_2D, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
				}
			}
			else
			{
				// Texture format is not compressed

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					const GLenum format = Mapping::getOpenGLES3Format(textureFormat);
					const GLenum type = Mapping::getOpenGLES3Type(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height));
						glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, format, type, data);

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					glTexImage2D(GL_TEXTURE_2D, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, Mapping::getOpenGLES3Format(textureFormat), Mapping::getOpenGLES3Type(textureFormat), data);
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLES3TextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLES3AlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture", 13)	// 13 = "2D texture: " including terminating zero
					glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2D() override
		{
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture
		*
		*  @return
		*    The OpenGL ES 3 texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
		}

		/**
		*  @brief
		*    Set minimum maximum mipmap index
		*
		*  @param[in] minimumMipmapIndex
		*    Minimum mipmap index, the most detailed mipmap, also known as base mipmap, 0 by default
		*  @param[in] maximumMipmapIndex
		*    Maximum mipmap index, the least detailed mipmap, <number of mipmaps> by default
		*/
		void setMinimumMaximumMipmapIndex(uint32_t minimumMipmapIndex, uint32_t maximumMipmapIndex)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLES3TextureBackup);
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
			glBindTexture(GL_TEXTURE_2D, mOpenGLES3Texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, static_cast<GLint>(minimumMipmapIndex));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(maximumMipmapIndex));

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLES3TextureBackup));
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLES3Texture));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2D(const Texture2D& source) = delete;
		Texture2D& operator =(const Texture2D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLES3Texture;	///< OpenGL ES 3 texture, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/Texture2DArray.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 2D array texture class
	*/
	class Texture2DArray final : public Rhi::ITexture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
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
		Texture2DArray(OpenGLES3Rhi& openGLES3Rhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2DArray(openGLES3Rhi, width, height, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Texture(0)
		{
			// TODO(co) Check support formats

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLES3AlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLES3AlignmentBackup);

				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &openGLES3TextureBackup);
			#endif

			// Set correct alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, mOpenGLES3Texture);

			// TODO(co) Add support for user provided mipmaps
			// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
			//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
			//   etc.

			// Upload the base map of the texture (mipmaps are automatically created as soon as the base map is changed)
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(numberOfSlices), 0, Mapping::getOpenGLES3Format(textureFormat), Mapping::getOpenGLES3Type(textureFormat), data);

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
			{
				glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(openGLES3TextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLES3AlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture array", 19)	// 19 = "2D texture array: " including terminating zero
					glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArray() override
		{
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture
		*
		*  @return
		*    The OpenGL ES 3 texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLES3Texture;	///< OpenGL ES 3 texture, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/Texture3D.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 3D texture class
	*/
	class Texture3D final : public Rhi::ITexture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
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
		*/
		inline Texture3D(OpenGLES3Rhi& openGLES3Rhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture3D(openGLES3Rhi, width, height, depth RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mOpenGLES3Texture(0)
		{
			// Sanity checks
			RHI_ASSERT(openGLES3Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL ES 3 texture parameters")
			RHI_ASSERT(openGLES3Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL ES 3 render target textures can't be filled using provided data")

			// TODO(co) Check support formats

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLES3AlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLES3AlignmentBackup);

				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_3D, &openGLES3TextureBackup);
			#endif

			// Set correct alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
			glBindTexture(GL_TEXTURE_3D, mOpenGLES3Texture);

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
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						glCompressedTexImage3D(GL_TEXTURE_3D, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, numberOfBytesPerMipmap, data);

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
					glCompressedTexImage3D(GL_TEXTURE_3D, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height)), data);
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
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					const GLenum format = Mapping::getOpenGLES3Format(textureFormat);
					const GLenum type = Mapping::getOpenGLES3Type(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const GLsizei numberOfBytesPerMipmap = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth);
						glTexImage3D(GL_TEXTURE_3D, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, format, type, data);

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
					glTexImage3D(GL_TEXTURE_3D, 0, Mapping::getOpenGLES3InternalFormat(textureFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth), 0, Mapping::getOpenGLES3Format(textureFormat), Mapping::getOpenGLES3Type(textureFormat), data);
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
			{
				glGenerateMipmap(GL_TEXTURE_3D);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_3D, static_cast<GLuint>(openGLES3TextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLES3AlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "3D texture", 13)	// 13 = "3D texture: " including terminating zero
					glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3D() override
		{
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture
		*
		*  @return
		*    The OpenGL ES 3 texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLES3Texture));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::TextureFormat::Enum mTextureFormat;
		GLuint					 mOpenGLES3Texture;	///< OpenGL ES 3 texture, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/TextureCube.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 cube texture class
	*/
	class TextureCube final : public Rhi::ITextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		TextureCube(OpenGLES3Rhi& openGLES3Rhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureCube(openGLES3Rhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Texture(0)
		{
			// Sanity checks
			RHI_ASSERT(openGLES3Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid OpenGL ES 3 texture parameters")
			RHI_ASSERT(openGLES3Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "OpenGL ES 3 render target textures can't be filled using provided data")

			// TODO(co) Check support formats

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently set alignment
				GLint openGLES3AlignmentBackup = 0;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &openGLES3AlignmentBackup);

				// Backup the currently bound OpenGL ES 3 texture
				GLint openGLES3TextureBackup = 0;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &openGLES3TextureBackup);
			#endif

			// Set correct alignment
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;

			// Create the OpenGL ES 3 texture instance
			glGenTextures(1, &mOpenGLES3Texture);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mOpenGLES3Texture);

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
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);

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
					const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
					const GLenum openGLES3InternalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					for (uint32_t face = 0; face < 6; ++face)
					{
						glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, openGLES3InternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, numberOfBytesPerSlice, data);
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
					const GLenum internalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					const GLenum format = Mapping::getOpenGLES3Format(textureFormat);
					const GLenum type = Mapping::getOpenGLES3Type(textureFormat);
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const GLsizei numberOfBytesPerSlice = static_cast<GLsizei>(Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width));
						for (uint32_t face = 0; face < 6; ++face)
						{
							// Upload the current face
							glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipmap), internalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, format, type, data);

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
					const GLenum openGLES3InternalFormat = Mapping::getOpenGLES3InternalFormat(textureFormat);
					const GLenum openGLES3Format = Mapping::getOpenGLES3Format(textureFormat);
					const GLenum openGLES3Type = Mapping::getOpenGLES3Type(textureFormat);
					for (uint32_t face = 0; face < 6; ++face)
					{
						glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, openGLES3InternalFormat, static_cast<GLsizei>(width), static_cast<GLsizei>(width), 0, openGLES3Format, openGLES3Type, data);
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
					}
				}
			}

			// Build mipmaps automatically on the GPU? (or GPU driver)
			if (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS)
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 texture
				glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(openGLES3TextureBackup));

				// Restore previous alignment
				glPixelStorei(GL_UNPACK_ALIGNMENT, openGLES3AlignmentBackup);
			#endif

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Cube texture", 15)	// 15 = "Cube texture: " including terminating zero
					glObjectLabelKHR(GL_TEXTURE, mOpenGLES3Texture, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCube() override
		{
			// Destroy the OpenGL ES 3 texture instance
			// -> Silently ignores 0's and names that do not correspond to existing textures
			glDeleteTextures(1, &mOpenGLES3Texture);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 texture
		*
		*  @return
		*    The OpenGL ES 3 texture, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Texture() const
		{
			return mOpenGLES3Texture;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLES3Texture));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCube(const TextureCube& source) = delete;
		TextureCube& operator =(const TextureCube& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLES3Texture;	///< OpenGL ES 3 texture, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Texture/TextureManager.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 texture manager interface
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
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*/
		inline explicit TextureManager(OpenGLES3Rhi& openGLES3Rhi) :
			ITextureManager(openGLES3Rhi),
			mExtensions(&openGLES3Rhi.getOpenGLES3Context().getExtensions())
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
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), width > 0, "OpenGL ES 3 create texture 1D was called with invalid parameters")

			// Create 1D texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL ES 3 has no texture usage indication
			return RHI_NEW(openGLES3Rhi.getContext(), Texture1D)(openGLES3Rhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), width > 0 && numberOfSlices > 0, "OpenGL ES 3 create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL ES 3 has no texture usage indication
			return RHI_NEW(openGLES3Rhi.getContext(), Texture1DArray)(openGLES3Rhi, width, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT, [[maybe_unused]] uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), width > 0 && height > 0, "OpenGL ES 3 create texture 2D was called with invalid parameters")

			// Create 2D texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL ES 3 has no texture usage indication
			return RHI_NEW(openGLES3Rhi.getContext(), Texture2D)(openGLES3Rhi, width, height, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), width > 0 && height > 0 && numberOfSlices > 0, "OpenGL ES 3 create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL ES 3 has no texture usage indication
			return RHI_NEW(openGLES3Rhi.getContext(), Texture2DArray)(openGLES3Rhi, width, height, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), width > 0 && height > 0 && depth > 0, "OpenGL ES 3 create texture 3D was called with invalid parameters")

			// Create 3D texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL ES 3 has no texture usage indication
			return RHI_NEW(openGLES3Rhi.getContext(), Texture3D)(openGLES3Rhi, width, height, depth, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCube* createTextureCube(uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), width > 0, "OpenGL ES 3 create texture cube was called with invalid parameters")

			// Create cube texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, OpenGL ES 3 has no texture usage indication
			return RHI_NEW(openGLES3Rhi.getContext(), TextureCube)(openGLES3Rhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCubeArray* createTextureCubeArray([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t numberOfSlices, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// TODO(co) Implement me, OpenGL ES 3.1 "GL_EXT_texture_cube_map_array"-extension
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
		const IExtensions* mExtensions;	///< Extensions instance, always valid


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/State/SamplerState.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 sampler state class
	*/
	class SamplerState final : public Rhi::ISamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(OpenGLES3Rhi& openGLES3Rhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISamplerState(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLMagFilterMode(Mapping::getOpenGLES3MagFilterMode(openGLES3Rhi.getContext(), samplerState.filter)),
			mOpenGLMinFilterMode(Mapping::getOpenGLES3MinFilterMode(openGLES3Rhi.getContext(), samplerState.filter, samplerState.maxLod > 0.0f)),
			mOpenGLTextureAddressModeS(Mapping::getOpenGLES3TextureAddressMode(samplerState.addressU)),
			mOpenGLTextureAddressModeT(Mapping::getOpenGLES3TextureAddressMode(samplerState.addressV)),
			mOpenGLTextureAddressModeR(Mapping::getOpenGLES3TextureAddressMode(samplerState.addressW)),
			mMipLodBias(samplerState.mipLodBias),
			mMaxAnisotropy(static_cast<float>(samplerState.maxAnisotropy)),	// Maximum anisotropy is "uint32_t" in Direct3D 10 & 11
			mOpenGLCompareMode(Mapping::getOpenGLES3CompareMode(samplerState.filter)),
			mOpenGLComparisonFunc(Mapping::getOpenGLES3ComparisonFunc(samplerState.comparisonFunc)),
			mMinLod(samplerState.minLod),
			mMaxLod(samplerState.maxLod)
		{
			// Sanity check
			RHI_ASSERT(openGLES3Rhi.getContext(), samplerState.maxAnisotropy <= openGLES3Rhi.getCapabilities().maximumAnisotropy, "Maximum OpenGL ES 3 anisotropy value violated")

			// Ignore "Rhi::SamplerState.borderColor", border color is not supported by OpenGL ES 3

			// TODO(co)  "GL_COMPARE_REF_TO_TEXTURE" is not supported by OpenGL ES 3, check/inform the user?
			// TODO(co)  "GL_CLAMP_TO_BORDER" is not supported by OpenGL ES 3, check/inform the user?
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerState() override
		{}

		/**
		*  @brief
		*    Set the OpenGL ES 3 sampler states
		*/
		void setOpenGLES3SamplerStates() const
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
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_LOD_BIAS, samplerState.mipLodBias);

			// Rhi::SamplerState::maxAnisotropy
			glSamplerParameterf(mOpenGLSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerState.maxAnisotropy);

			// Rhi::SamplerState::comparisonFunc
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
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), SamplerState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerState(const SamplerState& source) = delete;
		SamplerState& operator =(const SamplerState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// "Rhi::SamplerState" translated into OpenGL ES 3
		GLint  mOpenGLMagFilterMode;		///< Rhi::SamplerState::filter
		GLint  mOpenGLMinFilterMode;		///< Rhi::SamplerState::filter
		GLint  mOpenGLTextureAddressModeS;	///< Rhi::SamplerState::addressU
		GLint  mOpenGLTextureAddressModeT;	///< Rhi::SamplerState::addressV
		GLint  mOpenGLTextureAddressModeR;	///< Rhi::SamplerState::addressW
		float  mMipLodBias;					///< Rhi::SamplerState::mipLodBias
		float  mMaxAnisotropy;				///< Rhi::SamplerState::maxAnisotropy
		GLint  mOpenGLCompareMode;			///< Rhi::SamplerState::comparisonFunc
		GLenum mOpenGLComparisonFunc;		///< Rhi::SamplerState::comparisonFunc
		float  mMinLod;						///< Rhi::SamplerState::minLod
		float  mMaxLod;						///< Rhi::SamplerState::maxLod


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/State/IState.h                           ]
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
	//[ OpenGLES3Rhi/State/RasterizerState.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 rasterizer state class
	*/
	class RasterizerState : public IState
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
			mOpenGLES3FrontFaceMode(static_cast<GLenum>(mRasterizerState.frontCounterClockwise ? GL_CCW : GL_CW))
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
		*    Set the OpenGL ES 3 rasterizer states
		*/
		void setOpenGLES3RasterizerStates() const
		{
			// Rhi::RasterizerState::fillMode
			switch (mRasterizerState.fillMode)
			{
				// Wireframe
				case Rhi::FillMode::WIREFRAME:
					// OpenGL ES 3 has no support for polygon mode
					// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					break;

				// Solid
				default:
				case Rhi::FillMode::SOLID:
					// OpenGL ES 3 has no support for polygon mode
					// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
			glFrontFace(mOpenGLES3FrontFaceMode);

			// TODO(co) Map the rest of the rasterizer states

			// RasterizerState::depthBias

			// RasterizerState::depthBiasClamp

			// RasterizerState::slopeScaledDepthBias

			// RasterizerState::depthClipEnable
				/*
				TODO(co) OpenGL has the "GL_ARB_depth_clamp"-extension
				if (mRasterizerState.depthClipEnable)
				{
					glDisable(GL_DEPTH_CLAMP);
				}
				else
				{
					glEnable(GL_DEPTH_CLAMP);
				}

				Odd possible workaround mentioned at https://stackoverflow.com/questions/5960757/how-to-emulate-gl-depth-clamp-nv
				"
				You can emulate ARB_depth_clamp by using a separate varying for the z-component.

				Vertex Shader:

				varying float z;
				void main()
				{
					gl_Position = ftransform();

					// transform z to window coordinates
					z = gl_Position.z / gl_Position.w;
					z = (gl_DepthRange.diff * z + gl_DepthRange.near + gl_DepthRange.far) * 0.5;

					// prevent z-clipping
					gl_Position.z = 0.0;
				}

				Fragment shader:

				varying float z;
				void main()
				{
					gl_FragColor = vec4(vec3(z), 1.0);
					gl_FragDepth = clamp(z, 0.0, 1.0);
				}
				"
				*/

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
			// -> Anti-aliased lines are not supported by OpenGL ES 3
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::RasterizerState mRasterizerState;			///< Rasterizer state
		GLenum				 mOpenGLES3FrontFaceMode;	///< OpenGL ES 3 front face mode


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/State/DepthStencilState.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 depth stencil state class
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
			mOpenGLES3DepthMaskEnabled(static_cast<GLboolean>((Rhi::DepthWriteMask::ALL == mDepthStencilState.depthWriteMask) ? GL_TRUE : GL_FALSE)),
			mOpenGLES3DepthFunc(Mapping::getOpenGLES3ComparisonFunc(depthStencilState.depthFunc))
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
		*    Set the OpenGL ES 3 depth stencil states
		*/
		void setOpenGLES3DepthStencilStates() const
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
			glDepthMask(mOpenGLES3DepthMaskEnabled);

			// Rhi::DepthStencilState::depthFunc
			glDepthFunc(mOpenGLES3DepthFunc);

			// TODO(co) Map the rest of the depth stencil states
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::DepthStencilState mDepthStencilState;			///< Depth stencil state
		GLboolean			   mOpenGLES3DepthMaskEnabled;	///< OpenGL ES 3 depth mask enabled state
		GLenum				   mOpenGLES3DepthFunc;			///< OpenGL ES 3 depth function


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/State/BlendState.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 blend state class
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
		explicit BlendState(const Rhi::BlendState& blendState) :
			mBlendState(blendState),
			mOpenGLES3SrcBlend(Mapping::getOpenGLES3BlendType(mBlendState.renderTarget[0].srcBlend)),
			mOpenGLES3DstBlend(Mapping::getOpenGLES3BlendType(mBlendState.renderTarget[0].destBlend))
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
		*    Set the OpenGL ES 3 blend states
		*/
		void setOpenGLES3BlendStates() const
		{
			if (mBlendState.alphaToCoverageEnable)
			{
				glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			}
			else
			{
				glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			}

			if (mBlendState.renderTarget[0].blendEnable)
			{
				glEnable(GL_BLEND);
				glBlendFunc(mOpenGLES3SrcBlend, mOpenGLES3DstBlend);
			}
			else
			{
				glDisable(GL_BLEND);
			}

			// TODO(co) Map the rest of the blend states
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::BlendState mBlendState;		///< Blend state
		GLenum			mOpenGLES3SrcBlend;	///< OpenGL ES 3 source blend function
		GLenum			mOpenGLES3DstBlend;	///< OpenGL ES 3 destination blend function


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/RenderTarget/RenderPass.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 render pass interface
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
			RHI_ASSERT(rhi.getContext(), mNumberOfColorAttachments < 8, "Invalid number of OpenGL ES 3 color attachments")
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
	//[ OpenGLES3Rhi/RenderTarget/SwapChain.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 swap chain class
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
		*/
		inline SwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISwapChain(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNativeWindowHandle(windowHandle.nativeWindowHandle),
			mRenderWindow(windowHandle.renderWindow),
			mNewVerticalSynchronizationInterval(0)	// 0 instead of ~0u to ensure that we always set the swap interval at least once to have a known initial setting
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SwapChain() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRenderTarget methods             ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// TODO(co) Use "eglQuerySurface()"
		//	EGLint renderTargetWidth  = 1;
		//	EGLint renderTargetHeight = 1;
		//	eglQuerySurface(mOpenGLES3Context->getEGLDisplay(), mOpenGLES3Context->getEGLDummySurface(), EGL_HEIGHT, &renderTargetHeight);
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
			#elif defined(__ANDROID__)
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					//TODO(sw) get size on Android
					width = height = 1;
				}
				else
			#elif defined LINUX
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					IOpenGLES3Context& openGLES3Context = static_cast<OpenGLES3Rhi&>(getRhi()).getOpenGLES3Context();

					// TODO(sw) Reuse X11 display from "Frontend" -> for now reuse it from the OpenGL ES 3 context
					Display* display = openGLES3Context.getX11Display();

					// Get the width and height...
					::Window rootWindow = 0;
					int positionX = 0, positionY = 0;
					unsigned int unsignedWidth = 0, unsignedHeight = 0, border = 0, depth = 0;
					XGetGeometry(display, mNativeWindowHandle, &rootWindow, &positionX, &positionY, &unsignedWidth, &unsignedHeight, &border, &depth);

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
			}
			else
			{
				const IOpenGLES3Context& openGLES3Context = static_cast<OpenGLES3Rhi&>(getRhi()).getOpenGLES3Context();
				const EGLDisplay eglDisplay = openGLES3Context.getEGLDisplay();

				// Set new vertical synchronization interval?
				// -> We do this in here to avoid having to use "eglMakeCurrent()" to often at multiple places
				if (~0u != mNewVerticalSynchronizationInterval)
				{
					eglSwapInterval(eglDisplay, static_cast<EGLint>(mNewVerticalSynchronizationInterval));
					mNewVerticalSynchronizationInterval = ~0u;
				}

				// Swap buffers
				eglSwapBuffers(eglDisplay, openGLES3Context.getEGLDummySurface());
			}
		}

		inline virtual void resizeBuffers() override
		{}

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
		Rhi::IRenderWindow* mRenderWindow;			///< Render window instance, can be a null pointer, don't destroy the instance since we don't own it
		uint32_t			mNewVerticalSynchronizationInterval;


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/RenderTarget/Framebuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 framebuffer class
	*/
	class Framebuffer final : public Rhi::IFramebuffer
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
		Framebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFramebuffer(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Framebuffer(0),
			mDepthRenderbuffer(0),
			mNumberOfColorTextures(static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments()),
			mColorTextures(nullptr),	// Set below
			mDepthStencilTexture(nullptr),
			mWidth(1),
			mHeight(1)
		{
			// Unlike the "GL_ARB_framebuffer_object"-extension of OpenGL, in OpenGL ES 3 all
			// textures attached to the framebuffer must have the same width and height

			// Create the OpenGL ES 3 framebuffer
			glGenFramebuffers(1, &mOpenGLES3Framebuffer);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 framebuffer
				GLint openGLES3FramebufferBackup = 0;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &openGLES3FramebufferBackup);
			#endif

			// Bind this OpenGL ES 3 framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, mOpenGLES3Framebuffer);

			// Are there any color textures? (usually there are, so we just keep the "glBindFramebuffer()" above without trying to make this method implementation more complex)
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(renderPass.getRhi());
			if (mNumberOfColorTextures > 0)
			{
				mColorTextures = RHI_MALLOC_TYPED(openGLES3Rhi.getContext(), Rhi::ITexture*, mNumberOfColorTextures);

				// Loop through all framebuffer color attachments
				// -> "GL_COLOR_ATTACHMENT0" and "GL_COLOR_ATTACHMENT0_NV" have the same value
				Rhi::ITexture** colorTexture = mColorTextures;
				const Rhi::FramebufferAttachment* colorFramebufferAttachment	 = colorFramebufferAttachments;
				const Rhi::FramebufferAttachment* colorFramebufferAttachmentEnd = colorFramebufferAttachments + mNumberOfColorTextures;
				for (GLenum openGLES3Attachment = GL_COLOR_ATTACHMENT0; colorFramebufferAttachment < colorFramebufferAttachmentEnd; ++colorFramebufferAttachment, ++openGLES3Attachment, ++colorTexture)
				{
					// Sanity check
					RHI_ASSERT(openGLES3Rhi.getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid OpenGL ES 3 color framebuffer attachment texture")

					// TODO(co) Add security check: Is the given resource one of the currently used RHI?
					*colorTexture = colorFramebufferAttachment->texture;
					(*colorTexture)->addReference();

					// Security check: Is the given resource owned by this RHI?
					#ifdef RHI_DEBUG
						if (&openGLES3Rhi != &(*colorTexture)->getRhi())
						{
							// Output an error message and keep on going in order to keep a reasonable behaviour even in case on an error
							RHI_LOG(openGLES3Rhi.getContext(), CRITICAL, "OpenGL ES 3 error: The given color texture at index %u is owned by another RHI instance", colorTexture - mColorTextures)

							// Continue, there's no point in trying to do any error handling in here
							continue;
						}
					#endif

					// Evaluate the color texture type
					switch ((*colorTexture)->getResourceType())
					{
						case Rhi::ResourceType::TEXTURE_2D:
						{
							const Texture2D* texture2D = static_cast<Texture2D*>(*colorTexture);

							// Sanity checks
							RHI_ASSERT(openGLES3Rhi.getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL ES 3 color framebuffer attachment mipmap index")
							RHI_ASSERT(openGLES3Rhi.getContext(), 0 == colorFramebufferAttachment->layerIndex, "Invalid OpenGL ES 3 color framebuffer attachment layer index")

							// Set the OpenGL ES 3 framebuffer color attachment
							glFramebufferTexture2D(GL_FRAMEBUFFER, openGLES3Attachment, GL_TEXTURE_2D, texture2D->getOpenGLES3Texture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex));

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
							break;
						}

						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Set the OpenGL ES 3 framebuffer color attachment
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							glFramebufferTextureLayer(GL_FRAMEBUFFER, openGLES3Attachment, texture2DArray->getOpenGLES3Texture(), static_cast<GLint>(colorFramebufferAttachment->mipmapIndex), static_cast<GLint>(colorFramebufferAttachment->layerIndex));

							// If this is the primary render target, get the framebuffer width and height
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
							RHI_ASSERT(openGLES3Rhi.getContext(), false, "The type of the given color texture at index %ld is not supported by the OpenGL ES 3 RHI implementation", colorTexture - mColorTextures)
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RHI_ASSERT(openGLES3Rhi.getContext(), nullptr != mDepthStencilTexture, "Invalid OpenGL ES 3 depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the color texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<const Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(openGLES3Rhi.getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid OpenGL ES 3 depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(openGLES3Rhi.getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid OpenGL ES 3 depth stencil framebuffer attachment layer index")

						// Bind the depth stencil texture to framebuffer
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture2D->getOpenGLES3Texture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex));

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
						break;
					}

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Bind the depth stencil texture to framebuffer
						const Texture2DArray* texture2DArray = static_cast<const Texture2DArray*>(mDepthStencilTexture);
						glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture2DArray->getOpenGLES3Texture(), static_cast<GLint>(depthStencilFramebufferAttachment->mipmapIndex), static_cast<GLint>(depthStencilFramebufferAttachment->layerIndex));

						// Update the framebuffer width and height if required
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
						RHI_ASSERT(openGLES3Rhi.getContext(), false, "The type of the given depth stencil texture is not supported by the OpenGL ES 3 RHI implementation")
						break;
				}
			}

			// Check the status of the OpenGL ES 3 framebuffer
			const GLenum openGLES3Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			switch (openGLES3Status)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Not all framebuffer attachment points are framebuffer attachment complete (\"GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\")")
					break;

				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: No images are attached to the framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\")")
					break;

			// Not supported by OpenGL ES 3
			//	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			//		RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Incomplete draw buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\")")
			//		break;

			// Not supported by OpenGL ES 3
			//	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			//		RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Incomplete read buffer framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\")")
			//		break;

				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Incomplete multisample framebuffer (\"GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\")")
					break;

				case GL_FRAMEBUFFER_UNDEFINED:
					RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Undefined framebuffer (\"GL_FRAMEBUFFER_UNDEFINED\")")
					break;

				case GL_FRAMEBUFFER_UNSUPPORTED:
					RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: The combination of internal formats of the attached images violates an implementation-dependent set of restrictions (\"GL_FRAMEBUFFER_UNSUPPORTED\")")
					break;

				// Not supported by OpenGL ES 3
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
					RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Not all attached images have the same width and height (\"GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS\")")
					break;

				// Not supported by OpenGL ES 3
				// OpenGL: From "GL_EXT_framebuffer_object" (should no longer matter, should)
			//	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			//		RHI_ASSERT(openGLES3Rhi.getContext(), false, "OpenGL ES 3 error: Incomplete formats framebuffer object (\"GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\")")
			//		break;

				default:
				case GL_FRAMEBUFFER_COMPLETE:
					// Nothing here
					break;
			}

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(openGLES3FramebufferBackup));
			#endif

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RHI_ASSERT(openGLES3Rhi.getContext(), false, "Invalid OpenGL ES 3 framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RHI_ASSERT(openGLES3Rhi.getContext(), false, "Invalid OpenGL ES 3 framebuffer height")
				mHeight = 1;
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FBO", 6)	// 6 = "FBO: " including terminating zero
					glObjectLabelKHR(GL_FRAMEBUFFER, mOpenGLES3Framebuffer, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			// Destroy the OpenGL ES 3 framebuffer
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteFramebuffers(1, &mOpenGLES3Framebuffer);
			glDeleteRenderbuffers(1, &mDepthRenderbuffer);

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
		*    Return the OpenGL ES 3 framebuffer
		*
		*  @return
		*    The OpenGL ES 3 framebuffer, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Framebuffer() const
		{
			return mOpenGLES3Framebuffer;
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


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(static_cast<uintptr_t>(mOpenGLES3Framebuffer));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Framebuffer(const Framebuffer& source) = delete;
		Framebuffer& operator =(const Framebuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint			mOpenGLES3Framebuffer;	///< OpenGL ES 3 framebuffer, can be zero if no resource is allocated
		GLuint			mDepthRenderbuffer;		///< OpenGL ES render buffer for the depth component, can be zero if no resource is allocated
		uint32_t		mNumberOfColorTextures;	///< Number of color render target textures
		Rhi::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Rhi::ITexture*  mDepthStencilTexture;	///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t		mWidth;					///< The framebuffer width
		uint32_t		mHeight;				///< The framebuffer height


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Shader/VertexShaderHlsl.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL vertex shader class
	*/
	class VertexShaderGlsl final : public Rhi::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderGlsl(OpenGLES3Rhi& openGLES3Rhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Shader(loadShaderFromSourcecode(openGLES3Rhi, GL_VERTEX_SHADER, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLES3Shader && openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5)	// 5 = "VS: " including terminating zero
					glObjectLabelKHR(GL_SHADER_KHR, mOpenGLES3Shader, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexShaderGlsl() override
		{
			// Destroy the OpenGL ES 3 shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLES3Shader);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 shader
		*
		*  @return
		*    The OpenGL ES 3 shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Shader() const
		{
			return mOpenGLES3Shader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSLES_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShaderGlsl(const VertexShaderGlsl& source) = delete;
		VertexShaderGlsl& operator =(const VertexShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GLuint mOpenGLES3Shader;	///< OpenGL ES 3 shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Shader/FragmentShaderHlsl.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL fragment shader ("pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderGlsl final : public Rhi::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderGlsl(OpenGLES3Rhi& openGLES3Rhi, const char* sourceCode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3Shader(loadShaderFromSourcecode(openGLES3Rhi, GL_FRAGMENT_SHADER, sourceCode))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLES3Shader && openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5)	// 5 = "FS: " including terminating zero
					glObjectLabelKHR(GL_SHADER_KHR, mOpenGLES3Shader, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FragmentShaderGlsl() override
		{
			// Destroy the OpenGL ES 3 shader
			// -> Silently ignores 0's and names that do not correspond to existing buffer objects
			glDeleteShader(mOpenGLES3Shader);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 shader
		*
		*  @return
		*    The OpenGL ES 3 shader, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline uint32_t getOpenGLES3Shader() const
		{
			return mOpenGLES3Shader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSLES_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), FragmentShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShaderGlsl(const FragmentShaderGlsl& source) = delete;
		FragmentShaderGlsl& operator =(const FragmentShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t mOpenGLES3Shader;	///< OpenGL ES 3 shader, can be zero if no resource is allocated


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Shader/GraphicsProgramGlsl.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL graphics program class
	*/
	class GraphicsProgramGlsl final : public Rhi::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShaderGlsl
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderGlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramGlsl(OpenGLES3Rhi& openGLES3Rhi, const Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, VertexShaderGlsl* vertexShaderGlsl, FragmentShaderGlsl* fragmentShaderGlsl RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsProgram(openGLES3Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfRootSignatureParameters(0),
			mOpenGLES3Program(glCreateProgram()),
			mDrawIdUniformLocation(-1)
		{
			{ // Define the vertex array attribute binding locations ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
				const uint32_t numberOfVertexAttributes = vertexAttributes.numberOfAttributes;
				for (uint32_t vertexAttribute = 0; vertexAttribute < numberOfVertexAttributes; ++vertexAttribute)
				{
					glBindAttribLocation(mOpenGLES3Program, vertexAttribute, vertexAttributes.attributes[vertexAttribute].name);
				}
			}

			// Attach the shaders to the program
			// -> We don't need to keep a reference to the shader, to add and release at once to ensure a nice behaviour
			if (nullptr != vertexShaderGlsl)
			{
				vertexShaderGlsl->addReference();
				glAttachShader(mOpenGLES3Program, vertexShaderGlsl->getOpenGLES3Shader());
				vertexShaderGlsl->releaseReference();
			}
			if (nullptr != fragmentShaderGlsl)
			{
				fragmentShaderGlsl->addReference();
				glAttachShader(mOpenGLES3Program, fragmentShaderGlsl->getOpenGLES3Shader());
				fragmentShaderGlsl->releaseReference();
			}

			// Link the program
			glLinkProgram(mOpenGLES3Program);

			// Check the link status
			GLint linked = GL_FALSE;
			glGetProgramiv(mOpenGLES3Program, GL_LINK_STATUS, &linked);
			if (GL_TRUE == linked)
			{
				// Get draw ID uniform location
				if (!openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_EXT_base_instance())
				{
					mDrawIdUniformLocation = glGetUniformLocation(mOpenGLES3Program, "drawIdUniform");
				}

				// The actual locations assigned to uniform variables are not known until the program object is linked successfully
				// -> So we have to build a root signature parameter index -> uniform location mapping here
				const Rhi::RootSignature& rootSignatureData = static_cast<const RootSignature&>(rootSignature).getRootSignature();
				const uint32_t numberOfRootParameters = rootSignatureData.numberOfParameters;
				if (numberOfRootParameters > 0)
				{
					uint32_t uniformBlockBindingIndex = 0;
					const bool isGL_EXT_texture_buffer = openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_EXT_texture_buffer();
					for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
					{
						const Rhi::RootParameter& rootParameter = rootSignatureData.parameters[rootParameterIndex];
						if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
						{
							RHI_ASSERT(openGLES3Rhi.getContext(), nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL ES 3 descriptor ranges")
							const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
							for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
							{
								const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];

								// Ignore sampler range types in here (OpenGL ES 3 handles samplers in a different way then Direct3D 10>=)
								if (Rhi::DescriptorRangeType::UBV == descriptorRange.rangeType)
								{
									// Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension,
									// for backward compatibility, ask for the uniform block index
									const GLuint uniformBlockIndex = glGetUniformBlockIndex(mOpenGLES3Program, descriptorRange.baseShaderRegisterName);
									if (GL_INVALID_INDEX != uniformBlockIndex)
									{
										// Associate the uniform block with the given binding point
										glUniformBlockBinding(mOpenGLES3Program, uniformBlockIndex, uniformBlockBindingIndex);
										++uniformBlockBindingIndex;
									}
								}
								else if (Rhi::DescriptorRangeType::SAMPLER != descriptorRange.rangeType)
								{
									// We can only emulate the "Rhi::TextureFormat::R32G32B32A32F" texture format using an uniform buffer
									// -> Check for something like "InstanceTextureBuffer". Yes, this only works when one sticks to the naming convention.
									if (!isGL_EXT_texture_buffer && nullptr != strstr(descriptorRange.baseShaderRegisterName, "TextureBuffer"))
									{
										// Texture buffer emulation using uniform buffer

										// Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension,
										// for backward compatibility, ask for the uniform block index
										const GLuint uniformBlockIndex = glGetUniformBlockIndex(mOpenGLES3Program, descriptorRange.baseShaderRegisterName);
										if (GL_INVALID_INDEX != uniformBlockIndex)
										{
											// Associate the uniform block with the given binding point
											glUniformBlockBinding(mOpenGLES3Program, uniformBlockIndex, uniformBlockBindingIndex);
											++uniformBlockBindingIndex;
										}
									}
									else
									{
										const GLint uniformLocation = glGetUniformLocation(mOpenGLES3Program, descriptorRange.baseShaderRegisterName);
										if (uniformLocation >= 0)
										{
											// OpenGL ES 3/GLSL is not automatically assigning texture units to samplers, so, we have to take over this job
											// -> When using OpenGL or OpenGL ES 3 this is required
											// -> OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension supports explicit binding points ("layout(binding = 0)"
											//    in GLSL shader) , for backward compatibility we don't use it in here
											// -> When using Direct3D 9, Direct3D 10 or Direct3D 11, the texture unit
											//    to use is usually defined directly within the shader by using the "register"-keyword
											// TODO(co) There's room for binding API call related optimization in here (will certainly be no huge overall efficiency gain)
											#ifdef RHI_OPENGLES3_STATE_CLEANUP
												// Backup the currently used OpenGL ES 3 program
												GLint openGLES3ProgramBackup = 0;
												glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
												if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
												{
													// Set uniform, please note that for this our program must be the currently used one
													glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
												}
												else
												{
													// Set uniform, please note that for this our program must be the currently used one
													glUseProgram(mOpenGLES3Program);
													glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));

													// Be polite and restore the previous used OpenGL ES 3 program
													glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
												}
											#else
												// Set uniform, please note that for this our program must be the currently used one
												glUseProgram(mOpenGLES3Program);
												glUniform1i(uniformLocation, static_cast<GLint>(descriptorRange.baseShaderRegister));
											#endif
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				// Get the length of the information (including a null termination)
				GLint informationLength = 0;
				glGetProgramiv(mOpenGLES3Program, GL_INFO_LOG_LENGTH, &informationLength);
				if (informationLength > 1)
				{
					// Allocate memory for the information
					const Rhi::Context& context = openGLES3Rhi.getContext();
					char* informationLog = RHI_MALLOC_TYPED(context, char, informationLength);

					// Get the information
					glGetProgramInfoLog(mOpenGLES3Program, informationLength, nullptr, informationLog);

					// Output the debug string
					RHI_LOG(openGLES3Rhi.getContext(), CRITICAL, informationLog)

					// Cleanup information memory
					RHI_FREE(context, informationLog);
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (0 != mOpenGLES3Program && openGLES3Rhi.getOpenGLES3Context().getExtensions().isGL_KHR_debug())
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics program", 19)	// 19 = "Graphics program: " including terminating zero
					glObjectLabelKHR(GL_SHADER_KHR, mOpenGLES3Program, -1, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GraphicsProgramGlsl() override
		{
			// Destroy the OpenGL ES 3 program
			// -> A value of 0 for program will be silently ignored
			glDeleteProgram(mOpenGLES3Program);
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 program
		*
		*  @return
		*    The OpenGL ES 3 program, can be zero if no resource is allocated, do not destroy the returned resource
		*/
		[[nodiscard]] inline GLuint getOpenGLES3Program() const
		{
			return mOpenGLES3Program;
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
		//[ Setters                                               ]
		//[-------------------------------------------------------]
		void setUniform1i(Rhi::handle uniformHandle, int value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform1i(static_cast<GLint>(uniformHandle), value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniform1i(static_cast<GLint>(uniformHandle), value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniform1i(static_cast<GLint>(uniformHandle), value);
			#endif
		}

		void setUniform1f(Rhi::handle uniformHandle, float value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform1f(static_cast<GLint>(uniformHandle), value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniform1f(static_cast<GLint>(uniformHandle), value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniform1f(static_cast<GLint>(uniformHandle), value);
			#endif
		}

		void setUniform2fv(Rhi::handle uniformHandle, const float* value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniform2fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		void setUniform3fv(Rhi::handle uniformHandle, const float* value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniform3fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		void setUniform4fv(Rhi::handle uniformHandle, const float* value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniform4fv(static_cast<GLint>(uniformHandle), 1, value);
			#endif
		}

		void setUniformMatrix3fv(Rhi::handle uniformHandle, const float* value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniformMatrix3fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			#endif
		}

		void setUniformMatrix4fv(Rhi::handle uniformHandle, const float* value)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently used OpenGL ES 3 program
				GLint openGLES3ProgramBackup = 0;
				glGetIntegerv(GL_CURRENT_PROGRAM, &openGLES3ProgramBackup);
				if (openGLES3ProgramBackup == static_cast<GLint>(mOpenGLES3Program))
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
				}
				else
				{
					// Set uniform, please note that for this our program must be the currently used one
					glUseProgram(mOpenGLES3Program);
					glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);

					// Be polite and restore the previous used OpenGL ES 3 program
					glUseProgram(static_cast<GLuint>(openGLES3ProgramBackup));
				}
			#else
				// Set uniform, please note that for this our program must be the currently used one
				glUseProgram(mOpenGLES3Program);
				glUniformMatrix4fv(static_cast<GLint>(uniformHandle), 1, GL_FALSE, value);
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IGraphicsProgram methods          ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Rhi::handle getUniformHandle(const char* uniformName) override
		{
			return static_cast<Rhi::handle>(glGetUniformLocation(mOpenGLES3Program, uniformName));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsProgramGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramGlsl(const GraphicsProgramGlsl& source) = delete;
		GraphicsProgramGlsl& operator =(const GraphicsProgramGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t mNumberOfRootSignatureParameters;	///< Number of root signature parameters
		GLuint	 mOpenGLES3Program;					///< OpenGL ES 3 program, can be zero if no resource is allocated
		GLint	 mDrawIdUniformLocation;			///< Draw ID uniform location, used for "GL_EXT_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/Shader/ShaderLanguageGlsl.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL shader language class
	*/
	class ShaderLanguageGlsl final : public Rhi::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*/
		inline explicit ShaderLanguageGlsl(OpenGLES3Rhi& openGLES3Rhi) :
			IShaderLanguage(openGLES3Rhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageGlsl() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShaderLanguage methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::GLSLES_NAME;
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromBytecode(const Rhi::VertexAttributes&, const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());
			return RHI_NEW(openGLES3Rhi.getContext(), VertexShaderGlsl)(openGLES3Rhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Rhi::ShaderSourceCode&, Rhi::ShaderBytecode* RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no tessellation control shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Rhi::ShaderSourceCode&, Rhi::ShaderBytecode* RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no tessellation evaluation shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode(const Rhi::ShaderBytecode&, Rhi::GsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology, uint32_t RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode(const Rhi::ShaderSourceCode&, Rhi::GsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology, uint32_t, Rhi::ShaderBytecode* RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no geometry shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			// -> Monolithic shaders have no shader bytecode, only a monolithic program bytecode
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());
			return RHI_NEW(openGLES3Rhi.getContext(), FragmentShaderGlsl)(openGLES3Rhi, shaderSourceCode.sourceCode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders has no task shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromSourceCode(const Rhi::ShaderSourceCode&, Rhi::ShaderBytecode* = nullptr RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no task shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders has no mesh shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromSourceCode(const Rhi::ShaderSourceCode&, Rhi::ShaderBytecode* = nullptr RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no mesh shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromBytecode(const Rhi::ShaderBytecode& RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			// Error!
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 monolithic shaders have no shader bytecode, only a monolithic program bytecode")
			return nullptr;
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromSourceCode(const Rhi::ShaderSourceCode&, Rhi::ShaderBytecode* = nullptr RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no compute shader support")
			return nullptr;
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram(const Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, [[maybe_unused]] Rhi::ITessellationControlShader* tessellationControlShader, [[maybe_unused]] Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, [[maybe_unused]] Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			OpenGLES3Rhi& openGLES3Rhi = static_cast<OpenGLES3Rhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::GLSLES_NAME, "OpenGL ES 3 vertex shader language mismatch")
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == tessellationControlShader, "OpenGL ES 3 has no tessellation control shader support")
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == tessellationEvaluationShader, "OpenGL ES 3 has no tessellation evaluation shader support")
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == geometryShader, "OpenGL ES 3 has no geometry shader support")
			RHI_ASSERT(openGLES3Rhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::GLSLES_NAME, "OpenGL ES 3 fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(openGLES3Rhi.getContext(), GraphicsProgramGlsl)(openGLES3Rhi, rootSignature, vertexAttributes, static_cast<VertexShaderGlsl*>(vertexShader), static_cast<FragmentShaderGlsl*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Rhi::IRootSignature& rootSignature, [[maybe_unused]] Rhi::ITaskShader* taskShader, [[maybe_unused]] Rhi::IMeshShader& meshShader, [[maybe_unused]] Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER)
		{
			RHI_ASSERT(getRhi().getContext(), false, "OpenGL ES 3 has no mesh shader support")
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ShaderLanguageGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageGlsl(const ShaderLanguageGlsl& source) = delete;
		ShaderLanguageGlsl& operator =(const ShaderLanguageGlsl& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ OpenGLES3Rhi/State/GraphicsPipelineState.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenGL ES 3 graphics pipeline state class
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
		*  @param[in] openGLES3Rhi
		*    Owner OpenGL ES 3 RHI instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(OpenGLES3Rhi& openGLES3Rhi, const Rhi::GraphicsPipelineState& graphicsPipelineState, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsPipelineState(openGLES3Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mOpenGLES3PrimitiveTopology(Mapping::getOpenGLES3Type(graphicsPipelineState.primitiveTopology)),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mRasterizerState(graphicsPipelineState.rasterizerState),
			mDepthStencilState(graphicsPipelineState.depthStencilState),
			mBlendState(graphicsPipelineState.blendState)
		{
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
			static_cast<OpenGLES3Rhi&>(getRhi()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the OpenGL ES 3 primitive topology describing the type of primitive to render
		*
		*  @return
		*    OpenGL ES 3 primitive topology describing the type of primitive to render
		*/
		[[nodiscard]] inline GLenum getOpenGLES3PrimitiveTopology() const
		{
			return mOpenGLES3PrimitiveTopology;
		}

		/**
		*  @brief
		*    Bind the graphics pipeline state
		*/
		void bindGraphicsPipelineState() const
		{
			static_cast<OpenGLES3Rhi&>(getRhi()).setGraphicsProgram(mGraphicsProgram);

			// Set the OpenGL ES 3 rasterizer state
			mRasterizerState.setOpenGLES3RasterizerStates();

			// Set OpenGL ES 3 depth stencil state
			mDepthStencilState.setOpenGLES3DepthStencilStates();

			// Set OpenGL ES 3 blend state
			mBlendState.setOpenGLES3BlendStates();
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
		GLenum					mOpenGLES3PrimitiveTopology;	///< OpenGL ES 3 primitive topology describing the type of primitive to render
		Rhi::IGraphicsProgram*	mGraphicsProgram;
		Rhi::IRenderPass*		mRenderPass;
		RasterizerState			mRasterizerState;
		DepthStencilState		mDepthStencilState;
		BlendState				mBlendState;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // OpenGLES3Rhi




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

		[[nodiscard]] bool mapBuffer([[maybe_unused]] const Rhi::Context& context, GLenum target, GLenum bindingTarget, GLuint openGLES3Buffer, uint32_t bufferSize, Rhi::MapType mapType, Rhi::MappedSubresource& mappedSubresource)
		{
			// TODO(co) This buffer update isn't efficient, use e.g. persistent buffer mapping

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 buffer
				GLint openGLES3BufferBackup = 0;
				OpenGLES3Rhi::glGetIntegerv(bindingTarget, &openGLES3BufferBackup);
			#endif

			// Bind this OpenGL ES 3 buffer
			OpenGLES3Rhi::glBindBuffer(target, openGLES3Buffer);

			// Map
			mappedSubresource.data		 = OpenGLES3Rhi::glMapBufferRange(target, 0, static_cast<GLsizeiptr>(bufferSize), OpenGLES3Rhi::Mapping::getOpenGLES3MapRangeType(mapType));
			mappedSubresource.rowPitch   = 0;
			mappedSubresource.depthPitch = 0;

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 buffer
				OpenGLES3Rhi::glBindBuffer(target, static_cast<GLuint>(openGLES3BufferBackup));
			#endif

			// Done
			RHI_ASSERT(context, nullptr != mappedSubresource.data, "Mapping of OpenGL ES 3 buffer failed")
			return (nullptr != mappedSubresource.data);
		}

		void unmapBuffer(GLenum target, GLenum bindingTarget, GLuint openGLES3Buffer)
		{
			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Backup the currently bound OpenGL ES 3 buffer
				GLint openGLES3BufferBackup = 0;
				OpenGLES3Rhi::glGetIntegerv(bindingTarget, &openGLES3BufferBackup);
			#endif

			// Bind this OpenGL ES 3 buffer
			OpenGLES3Rhi::glBindBuffer(target, openGLES3Buffer);

			// Unmap
			OpenGLES3Rhi::glUnmapBuffer(target);

			#ifdef RHI_OPENGLES3_STATE_CLEANUP
				// Be polite and restore the previous bound OpenGL ES 3 buffer
				OpenGLES3Rhi::glBindBuffer(target, static_cast<GLuint>(openGLES3BufferBackup));
			#endif
		}

		namespace ImplementationDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void DispatchCommandBuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCommandBuffer* realData = static_cast<const Rhi::Command::DispatchCommandBuffer*>(data);
				RHI_ASSERT(rhi.getContext(), nullptr != realData->commandBufferToDispatch, "The OpenGL ES 3 command buffer to dispatch must be valid")
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).dispatchCommandBufferInternal(*realData->commandBufferToDispatch);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsRootSignature* realData = static_cast<const Rhi::Command::SetGraphicsRootSignature*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsPipelineState* realData = static_cast<const Rhi::Command::SetGraphicsPipelineState*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsResourceGroup* realData = static_cast<const Rhi::Command::SetGraphicsResourceGroup*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Rhi::IRhi& rhi)
			{
				// Input-assembler (IA) stage
				const Rhi::Command::SetGraphicsVertexArray* realData = static_cast<const Rhi::Command::SetGraphicsVertexArray*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsViewports* realData = static_cast<const Rhi::Command::SetGraphicsViewports*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Rhi::Viewport*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsScissorRectangles* realData = static_cast<const Rhi::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Rhi::ScissorRectangle*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Rhi::IRhi& rhi)
			{
				// Output-merger (OM) stage
				const Rhi::Command::SetGraphicsRenderTarget* realData = static_cast<const Rhi::Command::SetGraphicsRenderTarget*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ClearGraphics* realData = static_cast<const Rhi::Command::ClearGraphics*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).drawGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).drawIndexedGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawMeshTasks(const void*, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				RHI_ASSERT(static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).getContext(), false, "OpenGL ES 3 doesn't support mesh shaders")
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void*, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				RHI_ASSERT(static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).getContext(), false, "OpenGL ES 3 doesn't support compute root signature")
			}

			void SetComputePipelineState(const void*, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				RHI_ASSERT(static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).getContext(), false, "OpenGL ES 3 doesn't support compute pipeline state")
			}

			void SetComputeResourceGroup(const void*, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				RHI_ASSERT(static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).getContext(), false, "OpenGL ES 3 doesn't support compute resource group")
			}

			void DispatchCompute(const void*, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				RHI_ASSERT(static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).getContext(), false, "OpenGL ES 3 doesn't support compute dispatch")
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Rhi::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				RHI_ASSERT(static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).getContext(), realData->texture->getResourceType() == Rhi::ResourceType::TEXTURE_2D, "Unsupported OpenGL ES 3 texture resource type")
				static_cast<OpenGLES3Rhi::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
			}

			void ResolveMultisampleFramebuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Rhi::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyResource* realData = static_cast<const Rhi::Command::CopyResource*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::GenerateMipmaps* realData = static_cast<const Rhi::Command::GenerateMipmaps*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).generateMipmaps(*realData->resource);
			}

			void CopyUniformBufferData(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyUniformBufferData* realData = static_cast<const Rhi::Command::CopyUniformBufferData*>(data);
				Rhi::MappedSubresource mappedSubresource;
				if (rhi.map(*realData->uniformBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, mappedSubresource))
				{
					memcpy(mappedSubresource.data, Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->numberOfBytes);
					rhi.unmap(*realData->uniformBuffer, 0);
				}
			}

			void SetUniform(const void* data, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetUniform* realData = static_cast<const Rhi::Command::SetUniform*>(data);
				switch (realData->type)
				{
					case Rhi::Command::SetUniform::Type::UNIFORM_1I:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniform1i(realData->uniformHandle, *reinterpret_cast<const int*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					case Rhi::Command::SetUniform::Type::UNIFORM_1F:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniform1f(realData->uniformHandle, *reinterpret_cast<const float*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					case Rhi::Command::SetUniform::Type::UNIFORM_2FV:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniform2fv(realData->uniformHandle, reinterpret_cast<const float*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					case Rhi::Command::SetUniform::Type::UNIFORM_3FV:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniform3fv(realData->uniformHandle, reinterpret_cast<const float*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					case Rhi::Command::SetUniform::Type::UNIFORM_4FV:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniform4fv(realData->uniformHandle, reinterpret_cast<const float*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					case Rhi::Command::SetUniform::Type::UNIFORM_MATRIX_3FV:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniformMatrix3fv(realData->uniformHandle, reinterpret_cast<const float*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					case Rhi::Command::SetUniform::Type::UNIFORM_MATRIX_4FV:
						static_cast<OpenGLES3Rhi::GraphicsProgramGlsl*>(realData->graphicsProgram)->setUniformMatrix4fv(realData->uniformHandle, reinterpret_cast<const float*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
						break;

					default:
						RHI_ASSERT(rhi.getContext(), false, "Invalid set uniform type inside the OpenGLES 3 RHI implementation")
						break;
				}
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResetQueryPool* realData = static_cast<const Rhi::Command::ResetQueryPool*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::BeginQuery* realData = static_cast<const Rhi::Command::BeginQuery*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::EndQuery* realData = static_cast<const Rhi::Command::EndQuery*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::WriteTimestampQuery* realData = static_cast<const Rhi::Command::WriteTimestampQuery*>(data);
				static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RHI_DEBUG
				void SetDebugMarker(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::SetDebugMarker* realData = static_cast<const Rhi::Command::SetDebugMarker*>(data);
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::BeginDebugEvent* realData = static_cast<const Rhi::Command::BeginDebugEvent*>(data);
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Rhi::IRhi& rhi)
				{
					static_cast<OpenGLES3Rhi::OpenGLES3Rhi&>(rhi).endDebugEvent();
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
			&ImplementationDispatch::DispatchCommandBuffer,
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
			&ImplementationDispatch::CopyUniformBufferData,
			&ImplementationDispatch::SetUniform,
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
namespace OpenGLES3Rhi
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	OpenGLES3Rhi::OpenGLES3Rhi(const Rhi::Context& context) :
		IRhi(Rhi::NameId::OPENGLES3, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		mOpenGLES3Context(nullptr),
		mShaderLanguageGlsl(nullptr),
		mGraphicsRootSignature(nullptr),
		mDefaultSamplerState(nullptr),
		mOpenGLES3CopyResourceFramebuffer(0),
		mDefaultOpenGLES3VertexArray(0),
		// States
		mGraphicsPipelineState(nullptr),
		// Input-assembler (IA) stage
		mVertexArray(nullptr),
		mOpenGLES3PrimitiveTopology(0xFFFF),	// Unknown default setting
		// Output-merger (OM) stage
		mRenderTarget(nullptr),
		// State cache to avoid making redundant OpenGL ES 3 calls
		mOpenGLES3ClipControlOrigin(GL_INVALID_ENUM),
		mOpenGLES3Program(0),
		// Draw ID uniform location for "GL_EXT_base_instance"-emulation (see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html)
		mDrawIdUniformLocation(-1),
		mCurrentStartInstanceLocation(~0u)
	{
		// Initialize the OpenGL ES 3 context
		mOpenGLES3Context = RHI_NEW(mContext, OpenGLES3ContextRuntimeLinking)(*this, context.getNativeWindowHandle(), context.isUsingExternalContext());
		if (mOpenGLES3Context->initialize(0))
		{
			#ifdef RHI_DEBUG
				// "GL_KHR_debug"-extension available?
				if (mOpenGLES3Context->getExtensions().isGL_KHR_debug())
				{
					// Synchronous debug output, please
					// -> Makes it easier to find the place causing the issue
					glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);

					// Disable severity notifications, most drivers print many things with this severity
					glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, 0, 0, false);

					// Set the debug message callback function
					glDebugMessageCallbackKHR(&OpenGLES3Rhi::debugMessageCallback, this);
				}
			#endif

			// Initialize the capabilities
			initializeCapabilities();

			// Create the default sampler state
			mDefaultSamplerState = createSamplerState(Rhi::ISamplerState::getDefaultSamplerState());

			// Create default OpenGL ES 3 vertex array
			glGenVertexArrays(1, &mDefaultOpenGLES3VertexArray);
			glBindVertexArray(mDefaultOpenGLES3VertexArray);

			// Add references to the default sampler state and set it
			if (nullptr != mDefaultSamplerState)
			{
				mDefaultSamplerState->addReference();
				// TODO(co) Set default sampler states
			}
		}
	}

	OpenGLES3Rhi::~OpenGLES3Rhi()
	{
		// Set no graphics pipeline state reference, in case we have one
		if (nullptr != mGraphicsPipelineState)
		{
			setGraphicsPipelineState(nullptr);
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

		// Destroy the OpenGL ES 3 framebuffer used by "OpenGLES3Rhi::OpenGLES3Rhi::copyResource()"
		// -> Silently ignores 0's and names that do not correspond to existing buffer objects
		// -> Null test in here only to handle the situation of OpenGL ES 3 initialization failure, meaning "glDeleteFramebuffers" itself is a null pointer
		if (0 != mOpenGLES3CopyResourceFramebuffer)
		{
			glDeleteFramebuffers(1, &mOpenGLES3CopyResourceFramebuffer);
		}

		// Set no vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			setGraphicsVertexArray(nullptr);
		}

		// Destroy the OpenGL ES 3 default vertex array
		// -> Silently ignores 0's and names that do not correspond to existing vertex array objects
		glDeleteVertexArrays(1, &mDefaultOpenGLES3VertexArray);

		// Release the graphics root signature instance, in case we have one
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
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
					RHI_ASSERT(mContext, false, "The OpenGL ES 3 RHI implementation is going to be destroyed, but there are still %lu resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RHI_ASSERT(mContext, false, "The OpenGL ES 3 RHI implementation is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the GLSL shader language instance, in case we have one
		if (nullptr != mShaderLanguageGlsl)
		{
			mShaderLanguageGlsl->releaseReference();
		}

		// Destroy the OpenGL ES 3 context instance
		RHI_DELETE(mContext, IOpenGLES3Context, mOpenGLES3Context);
	}

	void OpenGLES3Rhi::dispatchCommandBufferInternal(const Rhi::CommandBuffer& commandBuffer)
	{
		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Rhi::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Dispatch command packet
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


	//[-------------------------------------------------------]
	//[ Graphics states                                       ]
	//[-------------------------------------------------------]
	void OpenGLES3Rhi::setGraphicsRootSignature(Rhi::IRootSignature* rootSignature)
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

	void OpenGLES3Rhi::setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState)
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

				// Set graphics pipeline state
				mOpenGLES3PrimitiveTopology = mGraphicsPipelineState->getOpenGLES3PrimitiveTopology();
				mGraphicsPipelineState->bindGraphicsPipelineState();
			}
			else if (nullptr != mGraphicsPipelineState)
			{
				// TODO(co) Handle this situation by resetting OpenGL states?
				mGraphicsPipelineState->releaseReference();
				mGraphicsPipelineState = nullptr;
			}
		}
	}

	void OpenGLES3Rhi::setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mGraphicsRootSignature, "No OpenGL ES 3 RHI implementation graphics root signature set")
			const Rhi::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The OpenGL ES 3 RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The OpenGL ES 3 RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The OpenGL ES 3 RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Set graphics resource group
			const ResourceGroup* openGLES3ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = openGLES3ResourceGroup->getNumberOfResources();
			Rhi::IResource** resources = openGLES3ResourceGroup->getResources();
			const Rhi::RootParameter& rootParameter = mGraphicsRootSignature->getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				Rhi::IResource* resource = *resources;
				RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid OpenGL ES 3 descriptor ranges")
				const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Rhi::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Rhi::ResourceType::TEXTURE_BUFFER:
						if (mOpenGLES3Context->getExtensions().isGL_EXT_texture_buffer())
						{
							// Fall through by intent
						}
						else
						{
							// We can only emulate the "Rhi::TextureFormat::R32G32B32A32F" texture format using an uniform buffer

							// Attach the buffer to the given UBO binding point
							// -> Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension
							// -> Direct3D 10 and Direct3D 11 have explicit binding points
							RHI_ASSERT(mContext, nullptr != openGLES3ResourceGroup->getResourceIndexToUniformBlockBindingIndex(), "Invalid OpenGL ES 3 resource index to uniform block binding index")
							glBindBufferBase(GL_UNIFORM_BUFFER, openGLES3ResourceGroup->getResourceIndexToUniformBlockBindingIndex()[resourceIndex], static_cast<TextureBuffer*>(resource)->getOpenGLES3TextureBuffer());
							break;
						}

					case Rhi::ResourceType::STRUCTURED_BUFFER:
						// TODO(co) Add OpenGL ES structured buffer support ("GL_EXT_buffer_storage"-extension)
						break;

					case Rhi::ResourceType::UNIFORM_BUFFER:
						// Attach the buffer to the given UBO binding point
						// -> Explicit binding points ("layout(binding = 0)" in GLSL shader) requires OpenGL 4.2 or the "GL_ARB_explicit_uniform_location"-extension
						// -> Direct3D 10 and Direct3D 11 have explicit binding points
						RHI_ASSERT(mContext, nullptr != openGLES3ResourceGroup->getResourceIndexToUniformBlockBindingIndex(), "Invalid OpenGL ES 3 resource index to uniform block binding index")
						glBindBufferBase(GL_UNIFORM_BUFFER, openGLES3ResourceGroup->getResourceIndexToUniformBlockBindingIndex()[resourceIndex], static_cast<UniformBuffer*>(resource)->getOpenGLES3UniformBuffer());
						break;

					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_2D:
					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
					{
						switch (descriptorRange.shaderVisibility)
						{
							// In OpenGL ES 3, all shaders share the same texture units
							case Rhi::ShaderVisibility::ALL:
							case Rhi::ShaderVisibility::ALL_GRAPHICS:
							case Rhi::ShaderVisibility::VERTEX:
							case Rhi::ShaderVisibility::FRAGMENT:
							{
								#ifdef RHI_OPENGLES3_STATE_CLEANUP
									// Backup the currently active OpenGL ES 3 texture
									GLint openGLES3ActiveTextureBackup = 0;
									glGetIntegerv(GL_ACTIVE_TEXTURE, &openGLES3ActiveTextureBackup);
								#endif

								// TODO(co) Some security checks might be wise *maximum number of texture units*
								glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + descriptorRange.baseShaderRegister));

								// Bind texture or texture buffer
								if (Rhi::ResourceType::TEXTURE_BUFFER == resourceType)
								{
									glBindTexture(GL_TEXTURE_BUFFER_EXT, static_cast<TextureBuffer*>(resource)->getOpenGLES3Texture());
								}
								else if (Rhi::ResourceType::TEXTURE_1D == resourceType)
								{
									// OpenGL ES 3 has no 1D textures, just use a 2D texture with a height of one
									glBindTexture(GL_TEXTURE_2D, static_cast<Texture1D*>(resource)->getOpenGLES3Texture());
								}
								else if (Rhi::ResourceType::TEXTURE_1D_ARRAY == resourceType)
								{
									// OpenGL ES 3 has no 1D textures, just use a 2D texture with a height of one
									// No extension check required, if we in here we already know it must exist
									glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<Texture1DArray*>(resource)->getOpenGLES3Texture());
								}
								else if (Rhi::ResourceType::TEXTURE_2D_ARRAY == resourceType)
								{
									// No extension check required, if we in here we already know it must exist
									glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<Texture2DArray*>(resource)->getOpenGLES3Texture());
								}
								else if (Rhi::ResourceType::TEXTURE_3D == resourceType)
								{
									// No extension check required, if we in here we already know it must exist
									glBindTexture(GL_TEXTURE_3D, static_cast<Texture3D*>(resource)->getOpenGLES3Texture());
								}
								else if (Rhi::ResourceType::TEXTURE_CUBE == resourceType)
								{
									// No extension check required, if we in here we already know it must exist
									glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<TextureCube*>(resource)->getOpenGLES3Texture());
								}
								else if (Rhi::ResourceType::TEXTURE_CUBE_ARRAY == resourceType)
								{
									// No extension check required, if we in here we already know it must exist
									// TODO(co) Implement me
									// glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<TextureCubeArray*>(resource)->getOpenGLES3Texture());
								}
								else
								{
									glBindTexture(GL_TEXTURE_2D, static_cast<Texture2D*>(resource)->getOpenGLES3Texture());
								}

								// Set the OpenGL ES 3 sampler states, if required (texture buffer has no sampler state), it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
								if (Rhi::ResourceType::TEXTURE_BUFFER != resourceType)
								{
									RHI_ASSERT(mContext, nullptr != openGLES3ResourceGroup->getSamplerState(), "Invalid OpenGL ES 3 sampler state")
									const SamplerState* samplerState = static_cast<const SamplerState*>(openGLES3ResourceGroup->getSamplerState()[resourceIndex]);
									if (nullptr != samplerState)
									{
										// Traditional bind version to emulate a sampler object
										samplerState->setOpenGLES3SamplerStates();
									}
								}

								#ifdef RHI_OPENGLES3_STATE_CLEANUP
									// Be polite and restore the previous active OpenGL ES 3 texture
									glActiveTexture(static_cast<GLuint>(openGLES3ActiveTextureBackup));
								#endif
								break;
							}

							case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
								RHI_ASSERT(mContext, false, "OpenGL ES 3 has no tessellation control shader support (hull shader in Direct3D terminology)")
								break;

							case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
								RHI_ASSERT(mContext, false, "OpenGL ES 3 has no tessellation evaluation shader support (domain shader in Direct3D terminology)")
								break;

							case Rhi::ShaderVisibility::GEOMETRY:
								RHI_ASSERT(mContext, false, "OpenGL ES 3 has no geometry shader support")
								break;

							case Rhi::ShaderVisibility::TASK:
								RHI_ASSERT(mContext, false, "OpenGL ES 3 has no task shader support")
								break;

							case Rhi::ShaderVisibility::MESH:
								RHI_ASSERT(mContext, false, "OpenGL ES 3 has no mesh shader support")
								break;

							case Rhi::ShaderVisibility::COMPUTE:
								RHI_ASSERT(mContext, false, "OpenGL ES 3 has no compute shader support")
								break;
						}
						break;
					}

					case Rhi::ResourceType::SAMPLER_STATE:
						// Unlike Direct3D >=10, OpenGL ES 3 directly attaches the sampler settings to the texture
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
					case Rhi::ResourceType::INDIRECT_BUFFER:
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
						RHI_ASSERT(mContext, false, "Invalid OpenGL ES 3 RHI implementation resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void OpenGLES3Rhi::setGraphicsVertexArray(Rhi::IVertexArray* vertexArray)
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

				// Release the vertex array reference, in case we have one
				if (nullptr != mVertexArray)
				{
					// Release reference
					mVertexArray->releaseReference();
				}

				// Set new vertex array and add a reference to it
				mVertexArray = static_cast<VertexArray*>(vertexArray);
				mVertexArray->addReference();

				// Bind OpenGL ES 3 vertex array
				glBindVertexArray(static_cast<VertexArray*>(mVertexArray)->getOpenGLES3VertexArray());
			}

			// Release the vertex array reference, in case we have one
			else if (nullptr != mVertexArray)
			{
				// Unbind OpenGL ES 3 vertex array
				glBindVertexArray(mDefaultOpenGLES3VertexArray);

				// Release reference
				mVertexArray->releaseReference();
				mVertexArray = nullptr;
			}
		}
	}

	void OpenGLES3Rhi::setGraphicsViewports([[maybe_unused]] uint32_t numberOfViewports, const Rhi::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid OpenGL ES 3 rasterizer state viewports")

		// In OpenGL ES 3, the origin of the viewport is left bottom while Direct3D is using a left top origin. To make the
		// Direct3D 11 implementation as efficient as possible the Direct3D convention is used and we have to convert in here.
		// -> This isn't influenced by the "GL_EXT_clip_control"-extension

		// Get the width and height of the current render target
		uint32_t renderTargetHeight = 1;
		if (nullptr != mRenderTarget)
		{
			uint32_t renderTargetWidth = 1;
			mRenderTarget->getWidthAndHeight(renderTargetWidth, renderTargetHeight);
		}

		// Set the OpenGL ES 3 viewport
		// -> OpenGL ES 3 supports only one viewport
		RHI_ASSERT(mContext, numberOfViewports <= 1, "OpenGL ES 3 supports only one viewport")
		glViewport(static_cast<GLint>(viewports->topLeftX), static_cast<GLint>(static_cast<float>(renderTargetHeight) - viewports->topLeftY - viewports->height), static_cast<GLsizei>(viewports->width), static_cast<GLsizei>(viewports->height));
		glDepthRangef(static_cast<GLclampf>(viewports->minDepth), static_cast<GLclampf>(viewports->maxDepth));
	}

	void OpenGLES3Rhi::setGraphicsScissorRectangles([[maybe_unused]] uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid OpenGL ES 3 rasterizer state scissor rectangles")

		// In OpenGL ES 3, the origin of the scissor rectangle is left bottom while Direct3D is using a left top origin. To make the
		// Direct3D 9 & 10 & 11 implementation as efficient as possible the Direct3D convention is used and we have to convert in here.
		// -> This isn't influenced by the "GL_EXT_clip_control"-extension

		// Get the width and height of the current render target
		uint32_t renderTargetHeight = 1;
		if (nullptr != mRenderTarget)
		{
			uint32_t renderTargetWidth = 1;
			mRenderTarget->getWidthAndHeight(renderTargetWidth, renderTargetHeight);
		}

		// Set the OpenGL ES 3 scissor rectangle
		RHI_ASSERT(mContext, numberOfScissorRectangles <= 1, "OpenGL ES 3 supports only one scissor rectangle")
		const GLsizei width  = scissorRectangles->bottomRightX - scissorRectangles->topLeftX;
		const GLsizei height = scissorRectangles->bottomRightY - scissorRectangles->topLeftY;
		glScissor(static_cast<GLint>(scissorRectangles->topLeftX), static_cast<GLint>(renderTargetHeight - scissorRectangles->topLeftY - height), width, height);
	}

	void OpenGLES3Rhi::setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget)
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
					// Unbind OpenGL ES 3 framebuffer?
					if (Rhi::ResourceType::FRAMEBUFFER == mRenderTarget->getResourceType() && Rhi::ResourceType::FRAMEBUFFER != renderTarget->getResourceType())
					{
						// We do not render into a OpenGL ES 3 framebuffer
						glBindFramebuffer(GL_FRAMEBUFFER, 0);
					}

					// Release
					mRenderTarget->releaseReference();
				}

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Evaluate the render target type
				GLenum clipControlOrigin = GL_UPPER_LEFT_EXT;
				switch (mRenderTarget->getResourceType())
				{
					case Rhi::ResourceType::SWAP_CHAIN:
					{
						clipControlOrigin = GL_LOWER_LEFT_EXT;	// Compensate OS window coordinate system y-flip
						// TODO(co) Implement me
						break;
					}

					case Rhi::ResourceType::FRAMEBUFFER:
					{
						// Get the OpenGL ES 3 framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Bind the OpenGL ES 3 framebuffer
						glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->getOpenGLES3Framebuffer());

						// Define the OpenGL buffers to draw into
						{
							// https://www.opengl.org/registry/specs/ARB/draw_buffers.txt - "The draw buffer for output colors beyond <n> is set to NONE."
							// -> Meaning depth only rendering which has no color textures at all will work as well, no need for "glDrawBuffer(GL_NONE)"
							// -> https://www.khronos.org/opengles/sdk/docs/man3/html/glDrawBuffers.xhtml for OpenGL ES 3 specifies the same behaviour
							// -> "GL_COLOR_ATTACHMENT0" and "GL_COLOR_ATTACHMENT0_NV" have the same value
							static constexpr GLenum OPENGL_DRAW_BUFFER[16] =
							{
								GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1,  GL_COLOR_ATTACHMENT2,  GL_COLOR_ATTACHMENT3,
								GL_COLOR_ATTACHMENT4,  GL_COLOR_ATTACHMENT5,  GL_COLOR_ATTACHMENT6,  GL_COLOR_ATTACHMENT7,
								GL_COLOR_ATTACHMENT8,  GL_COLOR_ATTACHMENT9,  GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
								GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15
							};
							glDrawBuffers(static_cast<GLsizei>(framebuffer->getNumberOfColorTextures()), OPENGL_DRAW_BUFFER);
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
				if (mOpenGLES3ClipControlOrigin != clipControlOrigin && mOpenGLES3Context->getExtensions().isGL_EXT_clip_control())
				{
					// OpenGL ES 3 default is "GL_LOWER_LEFT_EXT" and "GL_NEGATIVE_ONE_TO_ONE_EXT", change it to match Vulkan and Direct3D
					mOpenGLES3ClipControlOrigin = clipControlOrigin;
					glClipControlEXT(mOpenGLES3ClipControlOrigin, GL_ZERO_TO_ONE_EXT);
				}
			}
			else if (nullptr != mRenderTarget)
			{
				// Evaluate the render target type
				if (Rhi::ResourceType::FRAMEBUFFER == mRenderTarget->getResourceType())
				{
					// We do not render into a OpenGL ES 3 framebuffer
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}

				// TODO(co) Set no active render target

				// Release the render target reference, in case we have one
				mRenderTarget->releaseReference();
				mRenderTarget = nullptr;
			}
		}
	}

	void OpenGLES3Rhi::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Sanity check
		RHI_ASSERT(mContext, z >= 0.0f && z <= 1.0f, "The OpenGL ES 3 clear graphics z value must be between [0, 1] (inclusive)")

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
				glClearDepthf(z);
				if (nullptr != mGraphicsPipelineState && Rhi::DepthWriteMask::ALL != mGraphicsPipelineState->getDepthStencilState().depthWriteMask)
				{
					glDepthMask(GL_TRUE);
				}
			}
			if (clearFlags & Rhi::ClearFlag::STENCIL)
			{
				glClearStencil(static_cast<GLint>(stencil));
			}

			// Unlike OpenGL ES 3, when using Direct3D 10 & 11 the scissor rectangle(s) do not affect the clear operation
			// -> We have to compensate the OpenGL ES 3 behaviour in here

			// Disable OpenGL ES 3 scissor test, in case it's not disabled, yet
			if (nullptr != mGraphicsPipelineState && mGraphicsPipelineState->getRasterizerState().scissorEnable)
			{
				glDisable(GL_SCISSOR_TEST);
			}

			// Clear
			glClear(flagsApi);

			// Restore the previously set OpenGL ES 3 states
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

	void OpenGLES3Rhi::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The OpenGL ES 3 emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL ES 3 draws must not be zero")
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
			updateGL_EXT_base_instanceEmulation(drawArguments.startInstanceLocation);

			// Draw and advance
			if (drawArguments.instanceCount > 1 || (drawArguments.startInstanceLocation > 0 && mOpenGLES3Context->getExtensions().isGL_EXT_base_instance()))
			{
				// With instancing
				if (drawArguments.startInstanceLocation > 0 && mOpenGLES3Context->getExtensions().isGL_EXT_base_instance())
				{
					glDrawArraysInstancedBaseInstanceEXT(mOpenGLES3PrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance), static_cast<GLsizei>(drawArguments.instanceCount), drawArguments.startInstanceLocation);
				}
				else
				{
					glDrawArraysInstanced(mOpenGLES3PrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance), static_cast<GLsizei>(drawArguments.instanceCount));
				}
			}
			else
			{
				// Without instancing
				RHI_ASSERT(mContext, drawArguments.instanceCount <= 1, "Invalid OpenGL ES 3 instance count")
				glDrawArrays(mOpenGLES3PrimitiveTopology, static_cast<GLint>(drawArguments.startVertexLocation), static_cast<GLsizei>(drawArguments.vertexCountPerInstance));
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

	void OpenGLES3Rhi::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The OpenGL ES 3 emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of OpenGL ES 3 draws must not be zero")
		RHI_ASSERT(mContext, nullptr != mVertexArray, "Draw OpenGL ES 3 indexed needs a set vertex array")
		RHI_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "Draw OpenGL ES 3 indexed needs a set vertex array which contains an index buffer")

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
			updateGL_EXT_base_instanceEmulation(drawIndexedArguments.startInstanceLocation);

			// Draw and advance
			if (drawIndexedArguments.instanceCount > 1 || (drawIndexedArguments.startInstanceLocation > 0 && mOpenGLES3Context->getExtensions().isGL_EXT_base_instance()))
			{
				// With instancing
				if (drawIndexedArguments.baseVertexLocation > 0)
				{
					// Use start instance location?
					if (drawIndexedArguments.startInstanceLocation > 0 && mOpenGLES3Context->getExtensions().isGL_EXT_base_instance())
					{
						// Draw with base vertex location and start instance location
						glDrawElementsInstancedBaseVertexBaseInstanceEXT(mOpenGLES3PrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLES3Type(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount), static_cast<GLint>(drawIndexedArguments.baseVertexLocation), drawIndexedArguments.startInstanceLocation);
					}

					// Is the "GL_EXT_draw_elements_base_vertex" extension there?
					else if (mOpenGLES3Context->getExtensions().isGL_EXT_draw_elements_base_vertex())
					{
						// Draw with base vertex location
						glDrawElementsInstancedBaseVertexEXT(mOpenGLES3PrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLES3Type(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount), static_cast<GLint>(drawIndexedArguments.baseVertexLocation));
					}
					else
					{
						// Error!
						RHI_ASSERT(mContext, false, "Failed to OpenGL ES 3 draw indexed emulated")
					}
				}
				else if (drawIndexedArguments.startInstanceLocation > 0 && mOpenGLES3Context->getExtensions().isGL_EXT_base_instance())
				{
					// Draw without base vertex location and with start instance location
					glDrawElementsInstancedBaseInstanceEXT(mOpenGLES3PrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLES3Type(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount), drawIndexedArguments.startInstanceLocation);
				}
				else
				{
					// Draw without base vertex location
					glDrawElementsInstanced(mOpenGLES3PrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLES3Type(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLsizei>(drawIndexedArguments.instanceCount));
				}
			}
			else
			{
				// Without instancing

				// Use base vertex location?
				if (drawIndexedArguments.baseVertexLocation > 0)
				{
					// Is the "GL_EXT_draw_elements_base_vertex" extension there?
					if (mOpenGLES3Context->getExtensions().isGL_EXT_draw_elements_base_vertex())
					{
						// Draw with base vertex location
						glDrawElementsBaseVertexEXT(mOpenGLES3PrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLES3Type(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())), static_cast<GLint>(drawIndexedArguments.baseVertexLocation));
					}
					else
					{
						// Error!
						RHI_ASSERT(mContext, false, "Failed to OpenGL ES 3 draw indexed emulated")
					}
				}
				else
				{
					// Draw and advance
					glDrawElements(mOpenGLES3PrimitiveTopology, static_cast<GLsizei>(drawIndexedArguments.indexCountPerInstance), indexBuffer->getOpenGLES3Type(), reinterpret_cast<void*>(static_cast<uintptr_t>(drawIndexedArguments.startIndexLocation * indexBuffer->getIndexSizeInBytes())));
					emulationData += sizeof(Rhi::DrawIndexedArguments);
				}
			}
		}
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void OpenGLES3Rhi::resolveMultisampleFramebuffer(Rhi::IRenderTarget&, Rhi::IFramebuffer&)
	{
		// TODO(co) Implement me
	}

	void OpenGLES3Rhi::copyResource(Rhi::IResource& destinationResource, Rhi::IResource& sourceResource)
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
					// Get the OpenGL ES 3 texture 2D instances
					const Texture2D& openGlEs3DestinationTexture2D = static_cast<const Texture2D&>(destinationResource);
					const Texture2D& openGlEs3SourceTexture2D = static_cast<const Texture2D&>(sourceResource);
					RHI_ASSERT(mContext, openGlEs3DestinationTexture2D.getWidth() == openGlEs3SourceTexture2D.getWidth(), "OpenGL source and destination width must be identical for resource copy")
					RHI_ASSERT(mContext, openGlEs3DestinationTexture2D.getHeight() == openGlEs3SourceTexture2D.getHeight(), "OpenGL source and destination height must be identical for resource copy")

					#ifdef RHI_OPENGLES3_STATE_CLEANUP
						// Backup the currently bound OpenGL ES 3 framebuffer
						GLint openGLES3FramebufferBackup = 0;
						glGetIntegerv(GL_FRAMEBUFFER_BINDING, &openGLES3FramebufferBackup);
					#endif

					// Copy resource by using a framebuffer, but only the top-level mipmap
					const GLint width = static_cast<GLint>(openGlEs3DestinationTexture2D.getWidth());
					const GLint height = static_cast<GLint>(openGlEs3DestinationTexture2D.getHeight());
					if (0 == mOpenGLES3CopyResourceFramebuffer)
					{
						glGenFramebuffers(1, &mOpenGLES3CopyResourceFramebuffer);
					}
					glBindFramebuffer(GL_FRAMEBUFFER, mOpenGLES3CopyResourceFramebuffer);
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, openGlEs3SourceTexture2D.getOpenGLES3Texture(), 0);
					glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, openGlEs3DestinationTexture2D.getOpenGLES3Texture(), 0);
					static constexpr GLenum OPENGL_DRAW_BUFFER[1] =
					{
						GL_COLOR_ATTACHMENT1
					};
					glDrawBuffers(1, OPENGL_DRAW_BUFFER);
					glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

					#ifdef RHI_OPENGLES3_STATE_CLEANUP
						// Be polite and restore the previous bound OpenGL ES 3 framebuffer
						glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(openGLES3FramebufferBackup));
					#endif
				}
				else
				{
					// Error!
					RHI_ASSERT(mContext, false, "Failed to copy OpenGL ES 3 resource")
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

	void OpenGLES3Rhi::generateMipmaps(Rhi::IResource& resource)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, resource)
		RHI_ASSERT(mContext, resource.getResourceType() == Rhi::ResourceType::TEXTURE_2D, "TODO(co) Mipmaps can only be generated for OpenGL ES 3 2D texture resources")

		Texture2D& texture2D = static_cast<Texture2D&>(resource);

		#ifdef RHI_OPENGLES3_STATE_CLEANUP
			// Backup the currently bound OpenGL ES 3 texture
			// TODO(co) It's possible to avoid calling this multiple times
			GLint openGLES3TextureBackup = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &openGLES3TextureBackup);
		#endif

		// Generate mipmaps
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture2D.getOpenGLES3Texture());
		glGenerateMipmap(GL_TEXTURE_2D);

		#ifdef RHI_OPENGLES3_STATE_CLEANUP
			// Be polite and restore the previous bound OpenGL ES 3 texture
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(openGLES3TextureBackup));
		#endif
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void OpenGLES3Rhi::resetQueryPool([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}

	void OpenGLES3Rhi::beginQuery([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex, [[maybe_unused]] uint32_t queryControlFlags)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}

	void OpenGLES3Rhi::endQuery([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}

	void OpenGLES3Rhi::writeTimestampQuery([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void OpenGLES3Rhi::setDebugMarker(const char* name)
		{
			// "GL_KHR_debug"-extension required
			if (mOpenGLES3Context->getExtensions().isGL_KHR_debug())
			{
				RHI_ASSERT(mContext, nullptr != name, "OpenGL ES 3 debug marker names must not be a null pointer")
				glDebugMessageInsertKHR(GL_DEBUG_SOURCE_APPLICATION_KHR, GL_DEBUG_TYPE_MARKER_KHR, 1, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, -1, name);
			}
		}

		void OpenGLES3Rhi::beginDebugEvent(const char* name)
		{
			// "GL_KHR_debug"-extension required
			if (mOpenGLES3Context->getExtensions().isGL_KHR_debug())
			{
				RHI_ASSERT(mContext, nullptr != name, "OpenGL ES 3 debug event names must not be a null pointer")
				glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION_KHR, 1, -1, name);
			}
		}

		void OpenGLES3Rhi::endDebugEvent()
		{
			// "GL_KHR_debug"-extension required
			if (mOpenGLES3Context->getExtensions().isGL_KHR_debug())
			{
				glPopDebugGroupKHR();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	const char* OpenGLES3Rhi::getName() const
	{
		return "OpenGLES3";
	}

	bool OpenGLES3Rhi::isInitialized() const
	{
		// Is the OpenGL ES 3 context initialized?
		return mOpenGLES3Context->isInitialized();
	}

	bool OpenGLES3Rhi::isDebugEnabled()
	{
		// OpenGL ES 3 has nothing that is similar to the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)

		// Debug disabled
		return false;
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t OpenGLES3Rhi::getNumberOfShaderLanguages() const
	{
		return 1;
	}

	const char* OpenGLES3Rhi::getShaderLanguageName([[maybe_unused]] uint32_t index) const
	{
		RHI_ASSERT(mContext, index < getNumberOfShaderLanguages(), "OpenGL ES 3: Shader language index is out-of-bounds")
		return ::detail::GLSLES_NAME;
	}

	Rhi::IShaderLanguage* OpenGLES3Rhi::getShaderLanguage(const char* shaderLanguageName)
	{
		// In case "shaderLanguage" is a null pointer, use the default shader language
		if (nullptr != shaderLanguageName)
		{
			// Optimization: Check for shader language name pointer match, first
			if (::detail::GLSLES_NAME == shaderLanguageName || !stricmp(shaderLanguageName, ::detail::GLSLES_NAME))
			{
				// If required, create the GLSL shader language instance right now
				if (nullptr == mShaderLanguageGlsl)
				{
					mShaderLanguageGlsl = RHI_NEW(mContext, ShaderLanguageGlsl)(*this);
					mShaderLanguageGlsl->addReference();	// Internal RHI reference
				}

				// Return the shader language instance
				return mShaderLanguageGlsl;
			}

			// Error!
			return nullptr;
		}

		// Return the GLSL shader language instance as default
		return getShaderLanguage(::detail::GLSLES_NAME);
	}


	//[-------------------------------------------------------]
	//[ Resource creation                                     ]
	//[-------------------------------------------------------]
	Rhi::IRenderPass* OpenGLES3Rhi::createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IQueryPool* OpenGLES3Rhi::createQueryPool([[maybe_unused]] Rhi::QueryType queryType, [[maybe_unused]] uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT)
	{
		// TODO(co) Implement me
		return nullptr;
	}

	Rhi::ISwapChain* OpenGLES3Rhi::createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, renderPass)
		RHI_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle || nullptr != windowHandle.renderWindow, "OpenGL ES 3: The provided native window handle or render window must not be a null handle / null pointer")

		// Create the swap chain
		return RHI_NEW(mContext, SwapChain)(renderPass, windowHandle RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IFramebuffer* OpenGLES3Rhi::createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, renderPass)

		// Create the framebuffer
		return RHI_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IBufferManager* OpenGLES3Rhi::createBufferManager()
	{
		return RHI_NEW(mContext, BufferManager)(*this);
	}

	Rhi::ITextureManager* OpenGLES3Rhi::createTextureManager()
	{
		return RHI_NEW(mContext, TextureManager)(*this);
	}

	Rhi::IRootSignature* OpenGLES3Rhi::createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RootSignature)(*this, rootSignature RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IGraphicsPipelineState* OpenGLES3Rhi::createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "OpenGL ES 3: Invalid graphics pipeline state root signature")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "OpenGL ES 3: Invalid graphics pipeline state graphics program")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "OpenGL ES 3: Invalid graphics pipeline state render pass")

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

	Rhi::IComputePipelineState* OpenGLES3Rhi::createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, rootSignature)
		RHI_MATCH_CHECK(*this, computeShader)

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();

		// Error! OpenGL ES 3 has no compute shader support.
		return nullptr;
	}

	Rhi::ISamplerState* OpenGLES3Rhi::createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, SamplerState)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool OpenGLES3Rhi::map(Rhi::IResource& resource, uint32_t, Rhi::MapType mapType, uint32_t, Rhi::MappedSubresource& mappedSubresource)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
			{
				const VertexBuffer& vertexBuffer = static_cast<VertexBuffer&>(resource);
				return ::detail::mapBuffer(mContext, GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, vertexBuffer.getOpenGLES3ArrayBuffer(), vertexBuffer.getBufferSize(), mapType, mappedSubresource);
			}

			case Rhi::ResourceType::INDEX_BUFFER:
			{
				const IndexBuffer& indexBuffer = static_cast<IndexBuffer&>(resource);
				return ::detail::mapBuffer(mContext, GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, indexBuffer.getOpenGLES3ElementArrayBuffer(), indexBuffer.getBufferSize(), mapType, mappedSubresource);
			}

			case Rhi::ResourceType::TEXTURE_BUFFER:
			{
				const TextureBuffer& textureBuffer = static_cast<TextureBuffer&>(resource);
				return ::detail::mapBuffer(mContext, GL_TEXTURE_BUFFER_EXT, GL_TEXTURE_BINDING_BUFFER_EXT, textureBuffer.getOpenGLES3TextureBuffer(), textureBuffer.getBufferSize(), mapType, mappedSubresource);
			}

			case Rhi::ResourceType::STRUCTURED_BUFFER:
			{
				// TODO(co) Add OpenGL ES structured buffer support ("GL_EXT_buffer_storage"-extension)
				return false;
			}

			case Rhi::ResourceType::INDIRECT_BUFFER:
				mappedSubresource.data		 = static_cast<IndirectBuffer&>(resource).getWritableEmulationData();
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return true;

			case Rhi::ResourceType::UNIFORM_BUFFER:
			{
				const UniformBuffer& uniformBuffer = static_cast<UniformBuffer&>(resource);
				return ::detail::mapBuffer(mContext, GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, uniformBuffer.getOpenGLES3UniformBuffer(), uniformBuffer.getBufferSize(), mapType, mappedSubresource);
			}

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
			{
				// TODO(co) Implement me
				return false;
			}

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

	void OpenGLES3Rhi::unmap(Rhi::IResource& resource, uint32_t)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
				::detail::unmapBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, static_cast<VertexBuffer&>(resource).getOpenGLES3ArrayBuffer());
				break;

			case Rhi::ResourceType::INDEX_BUFFER:
				::detail::unmapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, static_cast<IndexBuffer&>(resource).getOpenGLES3ElementArrayBuffer());
				break;

			case Rhi::ResourceType::TEXTURE_BUFFER:
				::detail::unmapBuffer(GL_TEXTURE_BUFFER_EXT, GL_TEXTURE_BINDING_BUFFER_EXT, static_cast<TextureBuffer&>(resource).getOpenGLES3TextureBuffer());
				break;

			case Rhi::ResourceType::STRUCTURED_BUFFER:
				// TODO(co) Add OpenGL ES structured buffer support ("GL_EXT_buffer_storage"-extension)
				break;

			case Rhi::ResourceType::INDIRECT_BUFFER:
				// Nothing here, it's a software emulated indirect buffer
				break;

			case Rhi::ResourceType::UNIFORM_BUFFER:
				::detail::unmapBuffer(GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, static_cast<UniformBuffer&>(resource).getOpenGLES3UniformBuffer());
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
				// TODO(co) Implement me
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

	bool OpenGLES3Rhi::getQueryPoolResults([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t numberOfDataBytes, [[maybe_unused]] uint8_t* data, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries, [[maybe_unused]] uint32_t strideInBytes, [[maybe_unused]] uint32_t queryResultFlags)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// TODO(co) Implement me
		return false;
	}


	//[-------------------------------------------------------]
	//[ Operation                                             ]
	//[-------------------------------------------------------]
	void OpenGLES3Rhi::dispatchCommandBuffer(const Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(mContext, !commandBuffer.isEmpty(), "The OpenGL ES 3 command buffer to dispatch mustn't be empty")

		// Dispatch command buffer
		dispatchCommandBufferInternal(commandBuffer);
	}


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void OpenGLES3Rhi::debugMessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int, const char* message, const void* userParam)
		{
			// Source to string
			char debugSource[20 + 1]{0};	// +1 for terminating zero
			switch (source)
			{
				case GL_DEBUG_SOURCE_API_KHR:
					strncpy(debugSource, "OpenGL", 20);
					break;

				case GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR:
					strncpy(debugSource, "Windows", 20);
					break;

				case GL_DEBUG_SOURCE_SHADER_COMPILER_KHR:
					strncpy(debugSource, "Shader compiler", 20);
					break;

				case GL_DEBUG_SOURCE_THIRD_PARTY_KHR:
					strncpy(debugSource, "Third party", 20);
					break;

				case GL_DEBUG_SOURCE_APPLICATION_KHR:
					strncpy(debugSource, "Application", 20);
					break;

				case GL_DEBUG_SOURCE_OTHER_KHR:
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
				case GL_DEBUG_TYPE_ERROR_KHR:
					strncpy(debugType, "Error", 25);
					break;

				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR:
					logType = Rhi::ILog::Type::COMPATIBILITY_WARNING;
					strncpy(debugType, "Deprecated behavior", 25);
					break;

				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR:
					strncpy(debugType, "Undefined behavior", 25);
					break;

				case GL_DEBUG_TYPE_PORTABILITY_KHR:
					logType = Rhi::ILog::Type::COMPATIBILITY_WARNING;
					strncpy(debugType, "Portability", 25);
					break;

				case GL_DEBUG_TYPE_PERFORMANCE_KHR:
					logType = Rhi::ILog::Type::PERFORMANCE_WARNING;
					strncpy(debugType, "Performance", 25);
					break;

				case GL_DEBUG_TYPE_OTHER_KHR:
					strncpy(debugType, "Other", 25);
					break;

				case GL_DEBUG_TYPE_MARKER_KHR:
					strncpy(debugType, "Marker", 25);
					break;

				case GL_DEBUG_TYPE_PUSH_GROUP_KHR:
					// TODO(co) How to ignore "glPushDebugGroupKHR()" via "glDebugMessageControlKHR()" by default in here to have the same behaviour as OpenGL "glPushDebugGroup()"?
					return;
					// strncpy(debugType, "Push group", 25);
					// break;

				case GL_DEBUG_TYPE_POP_GROUP_KHR:
					// TODO(co) How to ignore "glPopDebugGroupKHR()" via "glDebugMessageControlKHR()" by default in here to have the same behaviour as OpenGL "glPopDebugGroup()"?
					return;
					// strncpy(debugType, "Pop group", 25);
					// break;

				default:
					strncpy(debugType, "?", 25);
					break;
			}

			// Debug severity to string
			char debugSeverity[20 + 1]{0};	// +1 for terminating zero
			switch (severity)
			{
				case GL_DEBUG_SEVERITY_HIGH_KHR:
					strncpy(debugSeverity, "High", 20);
					break;

				case GL_DEBUG_SEVERITY_MEDIUM_KHR:
					strncpy(debugSeverity, "Medium", 20);
					break;

				case GL_DEBUG_SEVERITY_LOW_KHR:
					strncpy(debugSeverity, "Low", 20);
					break;

				case GL_DEBUG_SEVERITY_NOTIFICATION_KHR:
					strncpy(debugSeverity, "Notification", 20);
					break;

				default:
					strncpy(debugSeverity, "?", 20);
					break;
			}

			// Print into log
			if (static_cast<const OpenGLES3Rhi*>(userParam)->getContext().getLog().print(logType, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), "OpenGL ES 3 debug message\tSource:\"%s\"\tType:\"%s\"\tID:\"%u\"\tSeverity:\"%s\"\tMessage:\"%s\"", debugSource, debugType, id, debugSeverity, message))
			{
				DEBUG_BREAK;
			}
		}
	#else
		void OpenGLES3Rhi::debugMessageCallback(uint32_t, uint32_t, uint32_t, uint32_t, int, const char*, const void*)
		{
			// Nothing here
		}
	#endif


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void OpenGLES3Rhi::initializeCapabilities()
	{
		GLint openGLValue = 0;

		{ // Get device name
		  // -> OpenGL ES Version 3.2 (November 3, 2016) Specification, section 20.2, page 440: "String queries return pointers to UTF-8 encoded, null-terminated static strings describing properties of the current GL context."
		  // -> For example "PVRVFrame 10.6 - None (Host : AMD Radeon R9 200 Series) (SDK Build: 17.1@4673912)" (used PowerVR_SDK OpenGL ES emulator)
			const size_t numberOfCharacters = ::detail::countof(mCapabilities.deviceName) - 1;
			strncpy(mCapabilities.deviceName, reinterpret_cast<const char*>(glGetString(GL_RENDERER)), numberOfCharacters);
			mCapabilities.deviceName[numberOfCharacters] = '\0';
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Rhi::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		mCapabilities.maximumNumberOfViewports = 1;	// OpenGL ES 3 only supports a single viewport

		// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &openGLValue);
		mCapabilities.maximumNumberOfSimultaneousRenderTargets = static_cast<uint32_t>(openGLValue);

		// Maximum texture dimension
		openGLValue = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &openGLValue);
		mCapabilities.maximumTextureDimension = static_cast<uint32_t>(openGLValue);

		// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &openGLValue);
		mCapabilities.maximumNumberOf1DTextureArraySlices = static_cast<uint32_t>(openGLValue);

		// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
		mCapabilities.maximumNumberOf2DTextureArraySlices = static_cast<uint32_t>(openGLValue);

		// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
		mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;	// TODO(co) Implement me		static_cast<uint32_t>(openGLValue);

		// Maximum uniform buffer (UBO) size in bytes (usually at least 16384 bytes, in case there's no support for uniform buffer it's 0)
		openGLValue = 0;
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &openGLValue);
		mCapabilities.maximumUniformBufferSize = static_cast<uint32_t>(openGLValue);

		// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		if (mOpenGLES3Context->getExtensions().isGL_EXT_texture_buffer())
		{
			openGLValue = 0;
			glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE_EXT, &openGLValue);
			mCapabilities.maximumTextureBufferSize = static_cast<uint32_t>(openGLValue);
		}
		else
		{
			// We can only emulate the "Rhi::TextureFormat::R32G32B32A32F" texture format using an uniform buffer
			mCapabilities.maximumTextureBufferSize = sizeof(float) * 4 * 4096;	// 64 KiB
		}

		// TODO(co) Add OpenGL ES structured buffer support ("GL_EXT_buffer_storage"-extension)
		mCapabilities.maximumStructuredBufferSize = 0;

		// Maximum indirect buffer size in bytes
		mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

		// Maximum number of multisamples (always at least 4 according to https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml )
		glGetIntegerv(GL_MAX_SAMPLES, &openGLValue);
		if (openGLValue > 8)
		{
			// Limit to known maximum we can test
			openGLValue = 8;
		}
		mCapabilities.maximumNumberOfMultisamples = static_cast<uint8_t>(openGLValue);
		// TODO(co) Implement multisample support
		mCapabilities.maximumNumberOfMultisamples = 1;

		// Maximum anisotropy (always at least 1, usually 16)
		// -> "GL_EXT_texture_filter_anisotropic"-extension
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &openGLValue);
		mCapabilities.maximumAnisotropy = static_cast<uint8_t>(openGLValue);

		// Coordinate system
		// -> If the "GL_EXT_clip_control"-extension is available: Left-handed coordinate system with clip space depth value range 0..1
		// -> If the "GL_EXT_clip_control"-extension isn't available: Right-handed coordinate system with clip space depth value range -1..1
		// -> For background theory see "Depth Precision Visualized" by Nathan Reed - https://developer.nvidia.com/content/depth-precision-visualized
		// -> For practical information see "Reversed-Z in OpenGL" by Nicolas Guillemot - https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
		// -> Shaders might want to take the following into account: "Mac computers that use OpenCL and OpenGL graphics" - https://support.apple.com/en-us/HT202823 - "iMac (Retina 5K, 27-inch, 2017)" - OpenGL 4.1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = mOpenGLES3Context->getExtensions().isGL_EXT_clip_control();

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = true;

		// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		mCapabilities.instancedArrays = true;	// Is core feature in OpenGL ES 3.0

		// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID, OpenGL ES 3 has no "GL_ARB_draw_instanced" extension)
		mCapabilities.drawInstanced = true;	// Is core feature in OpenGL ES 3.0

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = mOpenGLES3Context->getExtensions().isGL_EXT_draw_elements_base_vertex();

		// OpenGL ES 3 has no native multithreading
		mCapabilities.nativeMultithreading = false;

		// We don't support the OpenGL ES 3 program binaries since those are operation system and graphics driver version dependent, which renders them useless for pre-compiled shaders shipping
		mCapabilities.shaderBytecode = false;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		mCapabilities.maximumNumberOfPatchVertices = 0;	// OpenGL ES 3 has no tessellation support

		// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
		mCapabilities.maximumNumberOfGsOutputVertices = 0;	// OpenGL ES 3 has no support for geometry shaders

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = false;
	}

	void OpenGLES3Rhi::setGraphicsProgram(Rhi::IGraphicsProgram* graphicsProgram)
	{
		if (nullptr != graphicsProgram)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsProgram)

			// Bind the graphics program, if required
			const GraphicsProgramGlsl* graphicsProgramGlsl = static_cast<GraphicsProgramGlsl*>(graphicsProgram);
			const uint32_t openGLES3Program = graphicsProgramGlsl->getOpenGLES3Program();
			if (openGLES3Program != mOpenGLES3Program)
			{
				mOpenGLES3Program = openGLES3Program;
				mDrawIdUniformLocation = graphicsProgramGlsl->getDrawIdUniformLocation();
				mCurrentStartInstanceLocation = ~0u;
				glUseProgram(mOpenGLES3Program);
			}
		}
		else if (0 != mOpenGLES3Program)
		{
			// Unbind the program
			glUseProgram(0);
			mOpenGLES3Program = 0;
			mDrawIdUniformLocation = -1;
			mCurrentStartInstanceLocation = ~0u;
		}
	}

	void OpenGLES3Rhi::updateGL_EXT_base_instanceEmulation(uint32_t startInstanceLocation)
	{
		if (mDrawIdUniformLocation != -1 && 0 != mOpenGLES3Program && mCurrentStartInstanceLocation != startInstanceLocation)
		{
			glUniform1ui(mDrawIdUniformLocation, startInstanceLocation);
			mCurrentStartInstanceLocation = startInstanceLocation;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // OpenGLES3Rhi




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RHI_OPENGLES3_EXPORTS
	#define OPENGLES3RHI_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define OPENGLES3RHI_FUNCTION_EXPORT
#endif
OPENGLES3RHI_FUNCTION_EXPORT Rhi::IRhi* createOpenGLES3RhiInstance(const Rhi::Context& context)
{
	return RHI_NEW(context, OpenGLES3Rhi::OpenGLES3Rhi)(context);
}
#undef OPENGLES3RHI_FUNCTION_EXPORT
