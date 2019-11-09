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
*    Vulkan RHI amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    - Vulkan capable graphics driver
*    - Vulkan headers which can be found at "<unrimp>\External\Rhi\Vulkan\include\"
*    - smol-v (directly compiled and linked in)
*    - glslang if "RHI_VULKAN_GLSLTOSPIRV" is set (directly compiled and linked in)
*
*    == Preprocessor Definitions ==
*    - Set "RHI_VULKAN_EXPORTS" as preprocessor definition when building this library as shared library
*    - Set "RHI_VULKAN_GLSLTOSPIRV" as preprocessor definition when building this library to add support for compiling GLSL into SPIR-V, increases the binary size around one MiB
*    - Do also have a look into the RHI header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

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
	#define WIN32_LEAN_AND_MEAN
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
#elif defined __ANDROID__
	#include <link.h>
	#include <dlfcn.h>
#elif defined LINUX
	// TODO(co) Review which of the following headers can be removed
	#include <X11/Xlib.h>

	#include <GL/glx.h>
	#include <GL/glxext.h>

	#include <dlfcn.h>
	#include <link.h>
	#include <iostream>
#else
	#error "Unsupported platform"
#endif

#ifdef RHI_VULKAN_GLSLTOSPIRV
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
		PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
		#include <SPIRV/GlslangToSpv.h>
		#include <glslang/MachineIndependent/localintermediate.h>
	PRAGMA_WARNING_POP
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	#include <smol-v/smolv.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668 '<x>' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#if defined(_WIN32)
		#define VK_USE_PLATFORM_WIN32_KHR
	#elif defined(__ANDROID__)
		#define VK_USE_PLATFORM_ANDROID_KHR
	#elif defined(LINUX)
		// TODO(sw) Make this optional which platform we support? For now we support xlib and Wayland.
		#define VK_USE_PLATFORM_XLIB_KHR
		#define VK_USE_PLATFORM_WAYLAND_KHR
	#endif
	#define VK_NO_PROTOTYPES
	#include <vulkan/vulkan.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4574)	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <array>
	#include <vector>
	#include <sstream>
PRAGMA_WARNING_POP




//[-------------------------------------------------------]
//[ VulkanRhi/MakeID.h                                    ]
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
namespace VulkanRhi
{
	class VertexArray;
	class RootSignature;
	class VulkanContext;
	class VulkanRuntimeLinking;
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
		RHI_ASSERT(mContext, &rhiReference == &(resourceReference).getRhi(), "Vulkan error: The given resource is owned by another RHI instance")

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT , const char* debugName
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT , [[maybe_unused]] const char* debugName

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER , [[maybe_unused]] const char* debugName = ""

	/**
	*  @brief
	*    Pass resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*/
	#define RHI_RESOURCE_DEBUG_PASS_PARAMETER , debugName
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
	#define RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER

	/**
	*  @brief
	*    Pass resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*/
	#define RHI_RESOURCE_DEBUG_PASS_PARAMETER
#endif

#ifndef FNPTR
	#define FNPTR(name) PFN_##name name;
#endif

// Global Vulkan function pointers
FNPTR(vkGetInstanceProcAddr)
FNPTR(vkGetDeviceProcAddr)
FNPTR(vkEnumerateInstanceExtensionProperties)
FNPTR(vkEnumerateInstanceLayerProperties)
FNPTR(vkCreateInstance)

// Instance based Vulkan function pointers
FNPTR(vkDestroyInstance)
FNPTR(vkEnumeratePhysicalDevices)
FNPTR(vkEnumerateDeviceLayerProperties)
FNPTR(vkEnumerateDeviceExtensionProperties)
FNPTR(vkGetPhysicalDeviceQueueFamilyProperties)
FNPTR(vkGetPhysicalDeviceFeatures)
FNPTR(vkGetPhysicalDeviceFormatProperties)
FNPTR(vkGetPhysicalDeviceMemoryProperties)
FNPTR(vkGetPhysicalDeviceProperties)
FNPTR(vkCreateDevice)
// "VK_EXT_debug_report"-extension
FNPTR(vkCreateDebugReportCallbackEXT)
FNPTR(vkDestroyDebugReportCallbackEXT)
// "VK_KHR_surface"-extension
FNPTR(vkDestroySurfaceKHR)
FNPTR(vkGetPhysicalDeviceSurfaceSupportKHR)
FNPTR(vkGetPhysicalDeviceSurfaceFormatsKHR)
FNPTR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
FNPTR(vkGetPhysicalDeviceSurfacePresentModesKHR)
#ifdef VK_USE_PLATFORM_WIN32_KHR
	// "VK_KHR_win32_surface"-extension
	FNPTR(vkCreateWin32SurfaceKHR)
#elif defined VK_USE_PLATFORM_ANDROID_KHR
	// "VK_KHR_android_surface"-extension
	#warning "TODO(co) Not tested"
	FNPTR(vkCreateAndroidSurfaceKHR)
#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
	#if defined VK_USE_PLATFORM_XLIB_KHR
		// "VK_KHR_xlib_surface"-extension
		FNPTR(vkCreateXlibSurfaceKHR)
	#endif
	#if defined VK_USE_PLATFORM_WAYLAND_KHR
		// "VK_KHR_wayland_surface"-extension
		FNPTR(vkCreateWaylandSurfaceKHR)
	#endif
#elif defined VK_USE_PLATFORM_XCB_KHR
	// "VK_KHR_xcb_surface"-extension
	#warning "TODO(co) Not tested"
	FNPTR(vkCreateXcbSurfaceKHR)
#else
	#error "Unsupported platform"
#endif

// Device based Vulkan function pointers
FNPTR(vkDestroyDevice)
FNPTR(vkCreateShaderModule)
FNPTR(vkDestroyShaderModule)
FNPTR(vkCreateBuffer)
FNPTR(vkDestroyBuffer)
FNPTR(vkMapMemory)
FNPTR(vkUnmapMemory)
FNPTR(vkCreateBufferView)
FNPTR(vkDestroyBufferView)
FNPTR(vkAllocateMemory)
FNPTR(vkFreeMemory)
FNPTR(vkGetBufferMemoryRequirements)
FNPTR(vkBindBufferMemory)
FNPTR(vkCreateRenderPass)
FNPTR(vkDestroyRenderPass)
FNPTR(vkCreateImage)
FNPTR(vkDestroyImage)
FNPTR(vkGetImageSubresourceLayout)
FNPTR(vkGetImageMemoryRequirements)
FNPTR(vkBindImageMemory)
FNPTR(vkCreateImageView)
FNPTR(vkDestroyImageView)
FNPTR(vkCreateSampler)
FNPTR(vkDestroySampler)
FNPTR(vkCreateSemaphore)
FNPTR(vkDestroySemaphore)
FNPTR(vkCreateFence)
FNPTR(vkDestroyFence)
FNPTR(vkWaitForFences)
FNPTR(vkCreateCommandPool)
FNPTR(vkDestroyCommandPool)
FNPTR(vkAllocateCommandBuffers)
FNPTR(vkFreeCommandBuffers)
FNPTR(vkBeginCommandBuffer)
FNPTR(vkEndCommandBuffer)
FNPTR(vkGetDeviceQueue)
FNPTR(vkQueueSubmit)
FNPTR(vkQueueWaitIdle)
FNPTR(vkDeviceWaitIdle)
FNPTR(vkCreateFramebuffer)
FNPTR(vkDestroyFramebuffer)
FNPTR(vkCreatePipelineCache)
FNPTR(vkDestroyPipelineCache)
FNPTR(vkCreatePipelineLayout)
FNPTR(vkDestroyPipelineLayout)
FNPTR(vkCreateGraphicsPipelines)
FNPTR(vkCreateComputePipelines)
FNPTR(vkDestroyPipeline)
FNPTR(vkCreateDescriptorPool)
FNPTR(vkDestroyDescriptorPool)
FNPTR(vkCreateDescriptorSetLayout)
FNPTR(vkDestroyDescriptorSetLayout)
FNPTR(vkAllocateDescriptorSets)
FNPTR(vkFreeDescriptorSets)
FNPTR(vkUpdateDescriptorSets)
FNPTR(vkCreateQueryPool)
FNPTR(vkDestroyQueryPool)
FNPTR(vkGetQueryPoolResults)
FNPTR(vkCmdBeginQuery)
FNPTR(vkCmdEndQuery)
FNPTR(vkCmdResetQueryPool)
FNPTR(vkCmdWriteTimestamp)
FNPTR(vkCmdCopyQueryPoolResults)
FNPTR(vkCmdPipelineBarrier)
FNPTR(vkCmdBeginRenderPass)
FNPTR(vkCmdEndRenderPass)
FNPTR(vkCmdExecuteCommands)
FNPTR(vkCmdCopyImage)
FNPTR(vkCmdBlitImage)
FNPTR(vkCmdCopyBufferToImage)
FNPTR(vkCmdClearAttachments)
FNPTR(vkCmdCopyBuffer)
FNPTR(vkCmdBindDescriptorSets)
FNPTR(vkCmdBindPipeline)
FNPTR(vkCmdSetViewport)
FNPTR(vkCmdSetScissor)
FNPTR(vkCmdSetLineWidth)
FNPTR(vkCmdSetDepthBias)
FNPTR(vkCmdPushConstants)
FNPTR(vkCmdBindIndexBuffer)
FNPTR(vkCmdBindVertexBuffers)
FNPTR(vkCmdDraw)
FNPTR(vkCmdDrawIndexed)
FNPTR(vkCmdDrawIndirect)
FNPTR(vkCmdDrawIndexedIndirect)
FNPTR(vkCmdDispatch)
FNPTR(vkCmdClearColorImage)
FNPTR(vkCmdClearDepthStencilImage)
// "VK_EXT_debug_marker"-extension
FNPTR(vkDebugMarkerSetObjectTagEXT)
FNPTR(vkDebugMarkerSetObjectNameEXT)
FNPTR(vkCmdDebugMarkerBeginEXT)
FNPTR(vkCmdDebugMarkerEndEXT)
FNPTR(vkCmdDebugMarkerInsertEXT)
// "VK_KHR_swapchain"-extension
FNPTR(vkCreateSwapchainKHR)
FNPTR(vkDestroySwapchainKHR)
FNPTR(vkGetSwapchainImagesKHR)
FNPTR(vkAcquireNextImageKHR)
FNPTR(vkQueuePresentKHR)




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
		typedef std::vector<VkPhysicalDevice> VkPhysicalDevices;
		typedef std::vector<VkExtensionProperties> VkExtensionPropertiesVector;
		typedef std::array<VkPipelineShaderStageCreateInfo, 5> VkPipelineShaderStageCreateInfos;
		#ifdef __ANDROID__
			// On Android we need to explicitly select all layers
			#warning "TODO(co) Not tested"
			static constexpr uint32_t NUMBER_OF_VALIDATION_LAYERS = 6;
			static constexpr const char* VALIDATION_LAYER_NAMES[] =
			{
				"VK_LAYER_GOOGLE_threading",
				"VK_LAYER_LUNARG_parameter_validation",
				"VK_LAYER_LUNARG_object_tracker",
				"VK_LAYER_LUNARG_core_validation",
				"VK_LAYER_LUNARG_swapchain",
				"VK_LAYER_GOOGLE_unique_objects"
			};
		#else
			// On desktop the LunarG loaders exposes a meta layer that contains all layers
			static constexpr uint32_t NUMBER_OF_VALIDATION_LAYERS = 1;
			static constexpr const char* VALIDATION_LAYER_NAMES[] =
			{
				"VK_LAYER_LUNARG_standard_validation"
			};
		#endif

		#ifdef RHI_VULKAN_GLSLTOSPIRV
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

		void addVkPipelineShaderStageCreateInfo(VkShaderStageFlagBits vkShaderStageFlagBits, VkShaderModule vkShaderModule, VkPipelineShaderStageCreateInfos& vkPipelineShaderStageCreateInfos, uint32_t stageCount)
		{
			VkPipelineShaderStageCreateInfo& vkPipelineShaderStageCreateInfo = vkPipelineShaderStageCreateInfos[stageCount];
			vkPipelineShaderStageCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;	// sType (VkStructureType)
			vkPipelineShaderStageCreateInfo.pNext				= nullptr;												// pNext (const void*)
			vkPipelineShaderStageCreateInfo.flags				= 0;													// flags (VkPipelineShaderStageCreateFlags)
			vkPipelineShaderStageCreateInfo.stage				= vkShaderStageFlagBits;								// stage (VkShaderStageFlagBits)
			vkPipelineShaderStageCreateInfo.module				= vkShaderModule;										// module (VkShaderModule)
			vkPipelineShaderStageCreateInfo.pName				= "main";												// pName (const char*)
			vkPipelineShaderStageCreateInfo.pSpecializationInfo	= nullptr;												// pSpecializationInfo (const VkSpecializationInfo*)
		}

		void enumeratePhysicalDevices(const Rhi::Context& context, VkInstance vkInstance, VkPhysicalDevices& vkPhysicalDevices)
		{
			// Get the number of available physical devices
			uint32_t physicalDeviceCount = 0;
			VkResult vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
			if (VK_SUCCESS == vkResult)
			{
				if (physicalDeviceCount > 0)
				{
					// Enumerate physical devices
					vkPhysicalDevices.resize(physicalDeviceCount);
					vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, vkPhysicalDevices.data());
					if (VK_SUCCESS != vkResult)
					{
						// Error!
						RHI_LOG(context, CRITICAL, "Failed to enumerate physical Vulkan devices")
					}
				}
				else
				{
					// Error!
					RHI_LOG(context, CRITICAL, "There are no physical Vulkan devices")
				}
			}
			else
			{
				// Error!
				RHI_LOG(context, CRITICAL, "Failed to get the number of physical Vulkan devices")
			}
		}

		[[nodiscard]] bool isExtensionAvailable(const char* extensionName, const VkExtensionPropertiesVector& vkExtensionPropertiesVector)
		{
			for (const VkExtensionProperties& vkExtensionProperties : vkExtensionPropertiesVector)
			{
				if (strcmp(vkExtensionProperties.extensionName, extensionName) == 0)
				{
					// The extension is available
					return true;
				}
			}

			// The extension isn't available
			return false;
		}

		[[nodiscard]] VkPhysicalDevice selectPhysicalDevice(const Rhi::Context& context, const VkPhysicalDevices& vkPhysicalDevices, bool validationEnabled, bool& enableDebugMarker)
		{
			// TODO(co) I'am sure this selection can be improved (rating etc.)
			for (const VkPhysicalDevice& vkPhysicalDevice : vkPhysicalDevices)
			{
				// Get of device extensions
				uint32_t propertyCount = 0;
				if ((vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &propertyCount, nullptr) != VK_SUCCESS) || (0 == propertyCount))
				{
					// Reject physical Vulkan device
					continue;
				}
				VkExtensionPropertiesVector vkExtensionPropertiesVector(propertyCount);
				if (vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &propertyCount, vkExtensionPropertiesVector.data()) != VK_SUCCESS)
				{
					// Reject physical Vulkan device
					continue;
				}

				{ // Reject physical Vulkan devices basing on swap chain support
					// Check device extensions
					static constexpr std::array<const char*, 2> deviceExtensions =
					{
						VK_KHR_SWAPCHAIN_EXTENSION_NAME,
						VK_KHR_MAINTENANCE1_EXTENSION_NAME	// We want to be able to specify a negative viewport height, this way we don't have to apply "<output position>.y = -<output position>.y" inside vertex shaders to compensate for the Vulkan coordinate system
					};
					bool rejectDevice = false;
					for (const char* deviceExtension : deviceExtensions)
					{
						if (!isExtensionAvailable(deviceExtension, vkExtensionPropertiesVector))
						{
							rejectDevice = true;
							break;
						}
					}
					if (rejectDevice)
					{
						// Reject physical Vulkan device
						continue;
					}
				}

				{ // Reject physical Vulkan devices basing on supported API version and some basic limits
					VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
					vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProperties);
					const uint32_t majorVersion = VK_VERSION_MAJOR(vkPhysicalDeviceProperties.apiVersion);
					if ((majorVersion < 1) || (vkPhysicalDeviceProperties.limits.maxImageDimension2D < 4096))
					{
						// Reject physical Vulkan device
						continue;
					}
				}

				// Reject physical Vulkan devices basing on supported queue family
				uint32_t queueFamilyPropertyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, nullptr);
				if (0 == queueFamilyPropertyCount)
				{
					// Reject physical Vulkan device
					continue;
				}
				std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
				for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
				{
					if ((queueFamilyProperties[i].queueCount > 0) && (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
					{
						// Check whether or not the "VK_EXT_debug_marker"-extension is available
						// -> The "VK_EXT_debug_marker"-extension is only available when the application gets started by tools like RenderDoc ( https://renderdoc.org/ )
						// -> See "Offline debugging in Vulkan with VK_EXT_debug_marker and RenderDoc" - https://www.saschawillems.de/?page_id=2017
						if (enableDebugMarker)
						{
							// Check whether or not the "VK_EXT_debug_marker"-extension is available
							if (isExtensionAvailable(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, vkExtensionPropertiesVector))
							{
								// TODO(co) Currently, when trying to use RenderDoc ( https://renderdoc.org/ ) while having Vulkan debug layers enabled, RenderDoc crashes
								// -> Windows 10 x64
								// -> Radeon software 17.7.2
								// -> GPU: AMD 290X
								// -> LunarG® Vulkan™ SDK 1.0.54.0
								// -> Tried RenderDoc v0.91 as well as "Nightly v0.x @ 2017-08-21 (Win x64)" ("RenderDoc_2017_08_21_177d595d_64.zip")
								if (validationEnabled)
								{
									enableDebugMarker = false;
									RHI_LOG(context, WARNING, "Vulkan validation layers are enabled: If you want to use debug markers (\"VK_EXT_debug_marker\"-extension) please disable the validation layers")
								}
							}
							else
							{
								// Silently disable debug marker
								enableDebugMarker = false;
							}
						}

						// Select physical Vulkan device
						return vkPhysicalDevice;
					}
				}
			}

			// Error!
			RHI_LOG(context, CRITICAL, "Failed to select a physical Vulkan device")
			return VK_NULL_HANDLE;
		}

		[[nodiscard]] VkResult createVkDevice(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkPhysicalDevice vkPhysicalDevice, const VkDeviceQueueCreateInfo& vkDeviceQueueCreateInfo, bool enableValidation, bool enableDebugMarker, VkDevice& vkDevice)
		{
			// See http://vulkan.gpuinfo.org/listfeatures.php to check out GPU hardware capabilities
			static constexpr std::array<const char*, 3> enabledExtensions =
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_MAINTENANCE1_EXTENSION_NAME,	// We want to be able to specify a negative viewport height, this way we don't have to apply "<output position>.y = -<output position>.y" inside vertex shaders to compensate for the Vulkan coordinate system
				VK_EXT_DEBUG_MARKER_EXTENSION_NAME
			};
			static constexpr VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures =
			{
				VK_FALSE,	// robustBufferAccess (VkBool32)
				VK_FALSE,	// fullDrawIndexUint32 (VkBool32)
				VK_FALSE,	// imageCubeArray (VkBool32)
				VK_FALSE,	// independentBlend (VkBool32)
				VK_TRUE,	// geometryShader (VkBool32)
				VK_TRUE,	// tessellationShader (VkBool32)
				VK_FALSE,	// sampleRateShading (VkBool32)
				VK_FALSE,	// dualSrcBlend (VkBool32)
				VK_FALSE,	// logicOp (VkBool32)
				VK_TRUE,	// multiDrawIndirect (VkBool32)
				VK_FALSE,	// drawIndirectFirstInstance (VkBool32)
				VK_TRUE,	// depthClamp (VkBool32)
				VK_FALSE,	// depthBiasClamp (VkBool32)
				VK_TRUE,	// fillModeNonSolid (VkBool32)
				VK_FALSE,	// depthBounds (VkBool32)
				VK_FALSE,	// wideLines (VkBool32)
				VK_FALSE,	// largePoints (VkBool32)
				VK_FALSE,	// alphaToOne (VkBool32)
				VK_FALSE,	// multiViewport (VkBool32)
				VK_TRUE,	// samplerAnisotropy (VkBool32)
				VK_FALSE,	// textureCompressionETC2 (VkBool32)
				VK_FALSE,	// textureCompressionASTC_LDR (VkBool32)
				VK_TRUE,	// textureCompressionBC (VkBool32)
				VK_TRUE,	// occlusionQueryPrecise (VkBool32)
				VK_TRUE,	// pipelineStatisticsQuery (VkBool32)
				VK_FALSE,	// vertexPipelineStoresAndAtomics (VkBool32)
				VK_FALSE,	// fragmentStoresAndAtomics (VkBool32)
				VK_FALSE,	// shaderTessellationAndGeometryPointSize (VkBool32)
				VK_FALSE,	// shaderImageGatherExtended (VkBool32)
				VK_FALSE,	// shaderStorageImageExtendedFormats (VkBool32)
				VK_FALSE,	// shaderStorageImageMultisample (VkBool32)
				VK_FALSE,	// shaderStorageImageReadWithoutFormat (VkBool32)
				VK_FALSE,	// shaderStorageImageWriteWithoutFormat (VkBool32)
				VK_FALSE,	// shaderUniformBufferArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderSampledImageArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderStorageBufferArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderStorageImageArrayDynamicIndexing (VkBool32)
				VK_FALSE,	// shaderClipDistance (VkBool32)
				VK_FALSE,	// shaderCullDistance (VkBool32)
				VK_FALSE,	// shaderFloat64 (VkBool32)
				VK_FALSE,	// shaderInt64 (VkBool32)
				VK_FALSE,	// shaderInt16 (VkBool32)
				VK_FALSE,	// shaderResourceResidency (VkBool32)
				VK_FALSE,	// shaderResourceMinLod (VkBool32)
				VK_FALSE,	// sparseBinding (VkBool32)
				VK_FALSE,	// sparseResidencyBuffer (VkBool32)
				VK_FALSE,	// sparseResidencyImage2D (VkBool32)
				VK_FALSE,	// sparseResidencyImage3D (VkBool32)
				VK_FALSE,	// sparseResidency2Samples (VkBool32)
				VK_FALSE,	// sparseResidency4Samples (VkBool32)
				VK_FALSE,	// sparseResidency8Samples (VkBool32)
				VK_FALSE,	// sparseResidency16Samples (VkBool32)
				VK_FALSE,	// sparseResidencyAliased (VkBool32)
				VK_FALSE,	// variableMultisampleRate (VkBool32)
				VK_FALSE	// inheritedQueries (VkBool32)
			};
			const VkDeviceCreateInfo vkDeviceCreateInfo =
			{
				VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,							// sType (VkStructureType)
				nullptr,														// pNext (const void*)
				0,																// flags (VkDeviceCreateFlags)
				1,																// queueCreateInfoCount (uint32_t)
				&vkDeviceQueueCreateInfo,										// pQueueCreateInfos (const VkDeviceQueueCreateInfo*)
				enableValidation ? NUMBER_OF_VALIDATION_LAYERS : 0,				// enabledLayerCount (uint32_t)
				enableValidation ? VALIDATION_LAYER_NAMES : nullptr,			// ppEnabledLayerNames (const char* const*)
				enableDebugMarker ? 3u : 2u,									// enabledExtensionCount (uint32_t)
				enabledExtensions.empty() ? nullptr : enabledExtensions.data(),	// ppEnabledExtensionNames (const char* const*)
				&vkPhysicalDeviceFeatures										// pEnabledFeatures (const VkPhysicalDeviceFeatures*)
			};
			const VkResult vkResult = vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, vkAllocationCallbacks, &vkDevice);
			if (VK_SUCCESS == vkResult && enableDebugMarker)
			{
				// Get "VK_EXT_debug_marker"-extension function pointers

				// Define a helper macro
				PRAGMA_WARNING_PUSH
				PRAGMA_WARNING_DISABLE_MSVC(4191)	// 'reinterpret_cast': unsafe conversion from 'PFN_vkVoidFunction' to '<x>'
				#define IMPORT_FUNC(funcName)																					\
					funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(vkDevice, #funcName));						\
					if (nullptr == funcName)																					\
					{																											\
						RHI_LOG(context, CRITICAL, "Failed to load instance based Vulkan function pointer \"%s\"", #funcName)	\
					}																											\

				// "VK_EXT_debug_marker"-extension
				IMPORT_FUNC(vkDebugMarkerSetObjectTagEXT);
				IMPORT_FUNC(vkDebugMarkerSetObjectNameEXT);
				IMPORT_FUNC(vkCmdDebugMarkerBeginEXT);
				IMPORT_FUNC(vkCmdDebugMarkerEndEXT);
				IMPORT_FUNC(vkCmdDebugMarkerInsertEXT);

				// Undefine the helper macro
				#undef IMPORT_FUNC
				PRAGMA_WARNING_POP
			}

			// Done
			return vkResult;
		}

		[[nodiscard]] VkDevice createVkDevice(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkPhysicalDevice vkPhysicalDevice, bool enableValidation, bool enableDebugMarker, uint32_t& graphicsQueueFamilyIndex, uint32_t& presentQueueFamilyIndex)
		{
			VkDevice vkDevice = VK_NULL_HANDLE;

			// Get physical device queue family properties
			uint32_t queueFamilyPropertyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, nullptr);
			if (queueFamilyPropertyCount > 0)
			{
				std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties;
				vkQueueFamilyProperties.resize(queueFamilyPropertyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertyCount, vkQueueFamilyProperties.data());

				// Find a queue that supports graphics operations
				uint32_t graphicsQueueIndex = 0;
				for (; graphicsQueueIndex < queueFamilyPropertyCount; ++graphicsQueueIndex)
				{
					if (vkQueueFamilyProperties[graphicsQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						// Create logical Vulkan device instance
						static constexpr std::array<float, 1> queuePriorities = { 0.0f };
						const VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo =
						{
							VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,	// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							0,											// flags (VkDeviceQueueCreateFlags)
							graphicsQueueIndex,							// queueFamilyIndex (uint32_t)
							1,											// queueCount (uint32_t)
							queuePriorities.data()						// pQueuePriorities (const float*)
						};
						VkResult vkResult = createVkDevice(context, vkAllocationCallbacks, vkPhysicalDevice, vkDeviceQueueCreateInfo, enableValidation, enableDebugMarker, vkDevice);
						if (VK_ERROR_LAYER_NOT_PRESENT == vkResult && enableValidation)
						{
							// Error! Since the show must go on, try creating a Vulkan device instance without validation enabled...
							RHI_LOG(context, WARNING, "Failed to create the Vulkan device instance with validation enabled, layer is not present")
							vkResult = createVkDevice(context, vkAllocationCallbacks, vkPhysicalDevice, vkDeviceQueueCreateInfo, false, enableDebugMarker, vkDevice);
						}
						// TODO(co) Error handling: Evaluate "vkResult"?
						graphicsQueueFamilyIndex = graphicsQueueIndex;
						presentQueueFamilyIndex = graphicsQueueIndex;	// TODO(co) Handle the case of the graphics queue doesn't support present

						// We're done, get us out of the loop
						graphicsQueueIndex = queueFamilyPropertyCount;
					}
				}
			}
			else
			{
				// Error!
				RHI_LOG(context, CRITICAL, "Failed to get physical Vulkan device queue family properties")
			}

			// Done
			return vkDevice;
		}

		[[nodiscard]] VkCommandPool createVkCommandPool(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, uint32_t graphicsQueueFamilyIndex)
		{
			VkCommandPool vkCommandPool = VK_NULL_HANDLE;

			// Create Vulkan command pool instance
			const VkCommandPoolCreateInfo vkCommandPoolCreateInfo =
			{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,			// sType (VkStructureType)
				nullptr,											// pNext (const void*)
				VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,	// flags (VkCommandPoolCreateFlags)
				graphicsQueueFamilyIndex,							/// queueFamilyIndex (uint32_t)
			};
			const VkResult vkResult = vkCreateCommandPool(vkDevice, &vkCommandPoolCreateInfo, vkAllocationCallbacks, &vkCommandPool);
			if (VK_SUCCESS != vkResult)
			{
				// Error!
				RHI_LOG(context, CRITICAL, "Failed to create Vulkan command pool instance")
			}

			// Done
			return vkCommandPool;
		}

		[[nodiscard]] VkCommandBuffer createVkCommandBuffer(const Rhi::Context& context, VkDevice vkDevice, VkCommandPool vkCommandPool)
		{
			VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;

			// Create Vulkan command buffer instance
			const VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo =
			{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// sType (VkStructureType)
				nullptr,										// pNext (const void*)
				vkCommandPool,									// commandPool (VkCommandPool)
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// level (VkCommandBufferLevel)
				1												// commandBufferCount (uint32_t)
			};
			VkResult vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer);
			if (VK_SUCCESS != vkResult)
			{
				// Error!
				RHI_LOG(context, CRITICAL, "Failed to create Vulkan command buffer instance")
			}

			// Done
			return vkCommandBuffer;
		}

		[[nodiscard]] bool hasVkFormatStencilComponent(VkFormat vkFormat)
		{
			return (VK_FORMAT_D32_SFLOAT_S8_UINT == vkFormat || VK_FORMAT_D24_UNORM_S8_UINT == vkFormat);
		}

		[[nodiscard]] const char* vkDebugReportObjectTypeToString(VkDebugReportObjectTypeEXT vkDebugReportObjectTypeEXT)
		{
			// Define helper macro
			#define VALUE(value) case value: return #value;

			// Evaluate
			switch (vkDebugReportObjectTypeEXT)
			{
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT)
			//	VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT)	not possible
			//	VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT)	not possible	
			//	VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT)	not possible
			//	VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR_EXT)	not possible
			//	VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_BEGIN_RANGE_EXT)	not possible
			//	VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_END_RANGE_EXT)	not possible
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT)
				VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT)
			}

			// Undefine helper macro
			#undef VALUE

			// Error!
			return nullptr;
		}

		[[nodiscard]] VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
		{
			const Rhi::Context* context = static_cast<const Rhi::Context*>(pUserData);

			// TODO(co) Inside e.g. the "InstancedCubes"-example the log gets currently flooded with
			//          "Warning: Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT" Object: "120" Location: "5460" Message code: "0" Layer prefix: "DS" Message: "DescriptorSet 0x78 previously bound as set #0 is incompatible with set 0xc82f498 newly bound as set #0 so set #1 and any subsequent sets were disturbed by newly bound pipelineLayout (0x8b)" ".
			//          It's a known Vulkan API issue regarding validation. See https://github.com/KhronosGroup/Vulkan-Docs/issues/305 - "vkCmdBindDescriptorSets should be able to take NULL sets. #305".
			//          Currently I see no other way then ignoring this message.
			if (VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT == objectType && 5460 == location && 0 == messageCode)
			{
				// The Vulkan call should not be aborted to have the same behavior with and without validation layers enabled
				return VK_FALSE;
			}

			// TODO(co) File "unrimp\source\rhi\private\vulkanrhi\vulkanrhi.cpp" | Line 1029 | Critical: Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT" Object: "4963848" Location: "0" Message code: "0" Layer prefix: "Loader Message" Message: "loader_create_device_chain: Failed to find 'vkGetInstanceProcAddr' in layer C:\Program Files (x86)\Steam\.\SteamOverlayVulkanLayer.dll.  Skipping layer." 
			if (VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT == objectType && object && 0 == location && 0 == messageCode && nullptr != strstr(pMessage, "SteamOverlayVulkanLayer.dll"))
			{
				return VK_FALSE;
			}

			// Get log message type
			// -> Vulkan is using a flags combination, map it to our log message type enumeration
			Rhi::ILog::Type type = Rhi::ILog::Type::TRACE;
			if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
			{
				type = Rhi::ILog::Type::CRITICAL;
			}
			else if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0)
			{
				type = Rhi::ILog::Type::WARNING;
			}
			else if ((flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0)
			{
				type = Rhi::ILog::Type::PERFORMANCE_WARNING;
			}
			else if ((flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0)
			{
				type = Rhi::ILog::Type::INFORMATION;
			}
			else if ((flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0)
			{
				type = Rhi::ILog::Type::DEBUG;
			}

			// Construct the log message
			std::stringstream message;
			message << "Vulkan debug report callback: ";
			message << "Object type: \"" << vkDebugReportObjectTypeToString(objectType) << "\" ";
			message << "Object: \"" << object << "\" ";
			message << "Location: \"" << location << "\" ";
			message << "Message code: \"" << messageCode << "\" ";
			message << "Layer prefix: \"" << pLayerPrefix << "\" ";
			message << "Message: \"" << pMessage << "\" ";

			// Print log message
			if (context->getLog().print(type, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), message.str().c_str()))
			{
				DEBUG_BREAK;
			}

			// The Vulkan call should not be aborted to have the same behavior with and without validation layers enabled
			return VK_FALSE;
		}

		[[nodiscard]] VkSurfaceKHR createPresentationSurface(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkInstance vkInstance, VkPhysicalDevice vkPhysicalDevice, uint32_t graphicsQueueFamilyIndex, Rhi::WindowHandle windoInfo)
		{
			VkSurfaceKHR vkSurfaceKHR = VK_NULL_HANDLE;

			#ifdef VK_USE_PLATFORM_WIN32_KHR
				const VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,																		// sType (VkStructureType)
					nullptr,																												// pNext (const void*)
					0,																														// flags (VkWin32SurfaceCreateFlagsKHR)
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(reinterpret_cast<HWND>(windoInfo.nativeWindowHandle), GWLP_HINSTANCE)),	// hinstance (HINSTANCE)
					reinterpret_cast<HWND>(windoInfo.nativeWindowHandle)																	// hwnd (HWND)
				};
				if (vkCreateWin32SurfaceKHR(vkInstance, &vkWin32SurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
				{
					// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateWin32SurfaceKHR()" in case of failure?
					vkSurfaceKHR = VK_NULL_HANDLE;
				}
			#elif defined VK_USE_PLATFORM_ANDROID_KHR
				#warning "TODO(co) Not tested"
				const VkAndroidSurfaceCreateInfoKHR vkAndroidSurfaceCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,				// sType (VkStructureType)
					nullptr,														// pNext (const void*)
					0,																// flags (VkAndroidSurfaceCreateFlagsKHR)
					reinterpret_cast<ANativeWindow*>(windoInfo.nativeWindowHandle)	// window (ANativeWindow*)
				};
				if (vkCreateAndroidSurfaceKHR(vkInstance, &vkAndroidSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
				{
					// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateAndroidSurfaceKHR()" in case of failure?
					vkSurfaceKHR = VK_NULL_HANDLE;
				}
			#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
				RHI_ASSERT(context, context.getType() == Rhi::Context::ContextType::X11 || context.getType() == Rhi::Context::ContextType::WAYLAND, "Invalid Vulkan context type")

				// If the given RHI context is an X11 context use the display connection object provided by the context
				if (context.getType() == Rhi::Context::ContextType::X11)
				{
					const Rhi::X11Context& x11Context = static_cast<const Rhi::X11Context&>(context);
					const VkXlibSurfaceCreateInfoKHR vkXlibSurfaceCreateInfoKHR =
					{
						VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,	// sType (VkStructureType)
						nullptr,										// pNext (const void*)
						0,												// flags (VkXlibSurfaceCreateFlagsKHR)
						x11Context.getDisplay(),						// dpy (Display*)
						windoInfo.nativeWindowHandle					// window (Window)
					};
					if (vkCreateXlibSurfaceKHR(vkInstance, &vkXlibSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
					{
						// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateXlibSurfaceKHR()" in case of failure?
						vkSurfaceKHR = VK_NULL_HANDLE;
					}
				}
				else if (context.getType() == Rhi::Context::ContextType::WAYLAND)
				{
					const Rhi::WaylandContext& waylandContext = static_cast<const Rhi::WaylandContext&>(context);
					const VkWaylandSurfaceCreateInfoKHR vkWaylandSurfaceCreateInfoKHR =
					{
						VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,	// sType (VkStructureType)
						nullptr,											// pNext (const void*)
						0,													// flags (VkWaylandSurfaceCreateInfoKHR)
						waylandContext.getDisplay(),						// display (wl_display*)
						windoInfo.waylandSurface							// surface (wl_surface*)
					};
					if (vkCreateWaylandSurfaceKHR(vkInstance, &vkWaylandSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
					{
						// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateWaylandSurfaceKHR()" in case of failure?
						vkSurfaceKHR = VK_NULL_HANDLE;
					}
				}
			#elif defined VK_USE_PLATFORM_XCB_KHR
				#error "TODO(co) Complete implementation"
				const VkXcbSurfaceCreateInfoKHR vkXcbSurfaceCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					0,												// flags (VkXcbSurfaceCreateFlagsKHR)
					TODO(co)										// connection (xcb_connection_t*)
					TODO(co)										// window (xcb_window_t)
				};
				if (vkCreateXcbSurfaceKHR(vkInstance, &vkXcbSurfaceCreateInfoKHR, vkAllocationCallbacks, &vkSurfaceKHR) != VK_SUCCESS)
				{
					// TODO(co) Can we ensure "vkSurfaceKHR" doesn't get touched by "vkCreateXcbSurfaceKHR()" in case of failure?
					vkSurfaceKHR = VK_NULL_HANDLE;
				}
			#else
				#error "Unsupported platform"
			#endif

			{ // Sanity check: Does the physical Vulkan device support the Vulkan presentation surface?
			  // TODO(co) Inside our RHI implementation the swap chain is physical device independent, which is a nice thing usability wise.
			  //          On the other hand, the sanity check here can only detect issues but it would be better to not get into such issues in the first place.
				VkBool32 queuePresentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, graphicsQueueFamilyIndex, vkSurfaceKHR, &queuePresentSupport);
				if (VK_FALSE == queuePresentSupport)
				{
					RHI_LOG(context, CRITICAL, "The created Vulkan presentation surface has no queue present support")
				}
			}

			// Done
			return vkSurfaceKHR;
		}

		[[nodiscard]] uint32_t getNumberOfSwapChainImages(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// Set of images defined in a swap chain may not always be available for application to render to:
			// - One may be displayed and one may wait in a queue to be presented
			// - If application wants to use more images at the same time it must ask for more images
			uint32_t numberOfImages = vkSurfaceCapabilitiesKHR.minImageCount + 1;
			if ((vkSurfaceCapabilitiesKHR.maxImageCount > 0) && (numberOfImages > vkSurfaceCapabilitiesKHR.maxImageCount))
			{
				numberOfImages = vkSurfaceCapabilitiesKHR.maxImageCount;
			}
			return numberOfImages;
		}

		[[nodiscard]] VkSurfaceFormatKHR getSwapChainFormat(const Rhi::Context& context, VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurfaceKHR)
		{
			uint32_t surfaceFormatCount = 0;
			if ((vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &surfaceFormatCount, nullptr) != VK_SUCCESS) || (0 == surfaceFormatCount))
			{
				RHI_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface formats")
				return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
			}

			std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
			if (vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurfaceKHR, &surfaceFormatCount, surfaceFormats.data()) != VK_SUCCESS)
			{
				RHI_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface formats")
				return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
			}

			// If the list contains only one entry with undefined format it means that there are no preferred surface formats and any can be chosen
			if ((surfaceFormats.size() == 1) && (VK_FORMAT_UNDEFINED == surfaceFormats[0].format))
			{
				return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			}

			// Check if list contains most widely used R8 G8 B8 A8 format with nonlinear color space
			// -> Not all implementations support RGBA8, some only support BGRA8 formats (e.g. xlib surface under Linux with RADV), so check for both
			for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
			{
				if (VK_FORMAT_R8G8B8A8_UNORM == surfaceFormat.format || VK_FORMAT_B8G8R8A8_UNORM == surfaceFormat.format)
				{
					return surfaceFormat;
				}
			}

			// Return the first format from the list
			return surfaceFormats[0];
		}

		[[nodiscard]] VkExtent2D getSwapChainExtent(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// Special value of surface extent is width == height == -1
			// -> If this is so we define the size by ourselves but it must fit within defined confines, else it's already set to the operation window dimension
			if (-1 == vkSurfaceCapabilitiesKHR.currentExtent.width)
			{
				VkExtent2D swapChainExtent = { 640, 480 };
				if (swapChainExtent.width < vkSurfaceCapabilitiesKHR.minImageExtent.width)
				{
					swapChainExtent.width = vkSurfaceCapabilitiesKHR.minImageExtent.width;
				}
				if (swapChainExtent.height < vkSurfaceCapabilitiesKHR.minImageExtent.height)
				{
					swapChainExtent.height = vkSurfaceCapabilitiesKHR.minImageExtent.height;
				}
				if (swapChainExtent.width > vkSurfaceCapabilitiesKHR.maxImageExtent.width)
				{
					swapChainExtent.width = vkSurfaceCapabilitiesKHR.maxImageExtent.width;
				}
				if (swapChainExtent.height > vkSurfaceCapabilitiesKHR.maxImageExtent.height)
				{
					swapChainExtent.height = vkSurfaceCapabilitiesKHR.maxImageExtent.height;
				}
				return swapChainExtent;
			}

			// Most of the cases we define size of the swap chain images equal to current window's size
			return vkSurfaceCapabilitiesKHR.currentExtent;
		}

		[[nodiscard]] VkImageUsageFlags getSwapChainUsageFlags(const Rhi::Context& context, const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// Color attachment flag must always be supported. We can define other usage flags but we always need to check if they are supported.
			if (vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			{
				return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			// Construct the log message
			std::stringstream message;
			message << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain: Supported swap chain image usages include:\n";
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)				? "  VK_IMAGE_USAGE_TRANSFER_SRC\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)				? "  VK_IMAGE_USAGE_TRANSFER_DST\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT)						? "  VK_IMAGE_USAGE_SAMPLED\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT)						? "  VK_IMAGE_USAGE_STORAGE\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)			? "  VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)	? "  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)		? "  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : "");
			message << ((vkSurfaceCapabilitiesKHR.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)			? "  VK_IMAGE_USAGE_INPUT_ATTACHMENT" : "");

			// Print log message
			RHI_LOG(context, CRITICAL, message.str().c_str())

			// Error!
			return static_cast<VkImageUsageFlags>(-1);
		}

		[[nodiscard]] VkSurfaceTransformFlagBitsKHR getSwapChainTransform(const VkSurfaceCapabilitiesKHR& vkSurfaceCapabilitiesKHR)
		{
			// - Sometimes images must be transformed before they are presented (i.e. due to device's orientation being other than default orientation)
			// - If the specified transform is other than current transform, presentation engine will transform image during presentation operation; this operation may hit performance on some platforms
			// - Here we don't want any transformations to occur so if the identity transform is supported use it otherwise just use the same transform as current transform
			return (vkSurfaceCapabilitiesKHR.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : vkSurfaceCapabilitiesKHR.currentTransform;
		}

		[[nodiscard]] VkPresentModeKHR getSwapChainPresentMode(const Rhi::Context& context, VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR vkSurfaceKHR)
		{
			uint32_t presentModeCount = 0;
			if ((vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &presentModeCount, nullptr) != VK_SUCCESS) || (0 == presentModeCount))
			{
				RHI_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface present modes")
				return VK_PRESENT_MODE_MAX_ENUM_KHR;
			}

			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			if (vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurfaceKHR, &presentModeCount, presentModes.data()) != VK_SUCCESS)
			{
				RHI_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface present modes")
				return VK_PRESENT_MODE_MAX_ENUM_KHR;
			}

			// - FIFO present mode is always available
			// - MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
			for (const VkPresentModeKHR& presentMode : presentModes)
			{
				if (VK_PRESENT_MODE_MAILBOX_KHR == presentMode)
				{
					return presentMode;
				}
			}
			for (const VkPresentModeKHR& presentMode : presentModes)
			{
				if (VK_PRESENT_MODE_FIFO_KHR == presentMode)
				{
					return presentMode;
				}
			}

			// Error!
			RHI_LOG(context, CRITICAL, "FIFO present mode is not supported by the Vulkan swap chain")
			return VK_PRESENT_MODE_MAX_ENUM_KHR;
		}

		[[nodiscard]] VkRenderPass createRenderPass(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, VkFormat colorVkFormat, VkFormat depthVkFormat, VkSampleCountFlagBits vkSampleCountFlagBits)
		{
			const bool hasDepthStencilAttachment = (VK_FORMAT_UNDEFINED != depthVkFormat);

			// Render pass configuration
			const std::array<VkAttachmentDescription, 2> vkAttachmentDescriptions =
			{{
				{
					0,									// flags (VkAttachmentDescriptionFlags)
					colorVkFormat,						// format (VkFormat)
					vkSampleCountFlagBits,				// samples (VkSampleCountFlagBits)
					VK_ATTACHMENT_LOAD_OP_CLEAR,		// loadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_STORE,		// storeOp (VkAttachmentStoreOp)
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,	// stencilLoadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_DONT_CARE,	// stencilStoreOp (VkAttachmentStoreOp)
					VK_IMAGE_LAYOUT_UNDEFINED,			// initialLayout (VkImageLayout)
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR		// finalLayout (VkImageLayout)
				},
				{
					0,													// flags (VkAttachmentDescriptionFlags)
					depthVkFormat,										// format (VkFormat)
					vkSampleCountFlagBits,								// samples (VkSampleCountFlagBits)
					VK_ATTACHMENT_LOAD_OP_CLEAR,						// loadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_STORE,						// storeOp (VkAttachmentStoreOp)
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// stencilLoadOp (VkAttachmentLoadOp)
					VK_ATTACHMENT_STORE_OP_DONT_CARE,					// stencilStoreOp (VkAttachmentStoreOp)
					VK_IMAGE_LAYOUT_UNDEFINED,							// initialLayout (VkImageLayout)
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// finalLayout (VkImageLayout)
				}
			}};
			static constexpr VkAttachmentReference colorVkAttachmentReference =
			{
				0,											// attachment (uint32_t)
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// layout (VkImageLayout)
			};
			static constexpr VkAttachmentReference depthVkAttachmentReference =
			{
				1,													// attachment (uint32_t)
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// layout (VkImageLayout)
			};
			const VkSubpassDescription vkSubpassDescription =
			{
				0,																	// flags (VkSubpassDescriptionFlags)
				VK_PIPELINE_BIND_POINT_GRAPHICS,									// pipelineBindPoint (VkPipelineBindPoint)
				0,																	// inputAttachmentCount (uint32_t)
				nullptr,															// pInputAttachments (const VkAttachmentReference*)
				1,																	// colorAttachmentCount (uint32_t)
				&colorVkAttachmentReference,										// pColorAttachments (const VkAttachmentReference*)
				nullptr,															// pResolveAttachments (const VkAttachmentReference*)
				hasDepthStencilAttachment ? &depthVkAttachmentReference : nullptr,	// pDepthStencilAttachment (const VkAttachmentReference*)
				0,																	// preserveAttachmentCount (uint32_t)
				nullptr																// pPreserveAttachments (const uint32_t*)
			};
			static constexpr VkSubpassDependency vkSubpassDependency =
			{
				VK_SUBPASS_EXTERNAL,														// srcSubpass (uint32_t)
				0,																			// dstSubpass (uint32_t)
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// srcStageMask (VkPipelineStageFlags)
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// dstStageMask (VkPipelineStageFlags)
				0,																			// srcAccessMask (VkAccessFlags)
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// dstAccessMask (VkAccessFlags)
				0																			// dependencyFlags (VkDependencyFlags)
			};
			const VkRenderPassCreateInfo vkRenderPassCreateInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				0,											// flags (VkRenderPassCreateFlags)
				hasDepthStencilAttachment ? 2u : 1u,		// attachmentCount (uint32_t)
				vkAttachmentDescriptions.data(),			// pAttachments (const VkAttachmentDescription*)
				1,											// subpassCount (uint32_t)
				&vkSubpassDescription,						// pSubpasses (const VkSubpassDescription*)
				1,											// dependencyCount (uint32_t)
				&vkSubpassDependency						// pDependencies (const VkSubpassDependency*)
			};

			// Create render pass
			VkRenderPass vkRenderPass = VK_NULL_HANDLE;
			if (vkCreateRenderPass(vkDevice, &vkRenderPassCreateInfo, vkAllocationCallbacks, &vkRenderPass) != VK_SUCCESS)
			{
				RHI_LOG(context, CRITICAL, "Failed to create Vulkan render pass")
			}

			// Done
			return vkRenderPass;
		}

		[[nodiscard]] VkFormat findSupportedVkFormat(VkPhysicalDevice vkPhysicalDevice, const std::vector<VkFormat>& vkFormatCandidates, VkImageTiling vkImageTiling, VkFormatFeatureFlags vkFormatFeatureFlags)
		{
			for (VkFormat vkFormat : vkFormatCandidates)
			{
				VkFormatProperties vkFormatProperties;
				vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, vkFormat, &vkFormatProperties);
				if (VK_IMAGE_TILING_LINEAR == vkImageTiling && (vkFormatProperties.linearTilingFeatures & vkFormatFeatureFlags) == vkFormatFeatureFlags)
				{
					return vkFormat;
				}
				else if (VK_IMAGE_TILING_OPTIMAL == vkImageTiling && (vkFormatProperties.optimalTilingFeatures & vkFormatFeatureFlags) == vkFormatFeatureFlags)
				{
					return vkFormat;
				}
			}

			// Failed to find supported Vulkan depth format
			return VK_FORMAT_UNDEFINED;
		}

		/**
		*  @brief
		*    Create Vulkan shader module from bytecode
		*
		*  @param[in] context
		*    RHI context
		*  @param[in] vkAllocationCallbacks
		*    Vulkan allocation callbacks
		*  @param[in] vkDevice
		*    Vulkan device
		*  @param[in] shaderBytecode
		*    Shader SPIR-V bytecode compressed via SMOL-V
		*
		*  @return
		*    The Vulkan shader module, null handle on error
		*/
		[[nodiscard]] VkShaderModule createVkShaderModuleFromBytecode(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, const Rhi::ShaderBytecode& shaderBytecode)
		{
			// Decode from SMOL-V: like Vulkan/Khronos SPIR-V, but smaller
			// -> https://github.com/aras-p/smol-v
			// -> http://aras-p.info/blog/2016/09/01/SPIR-V-Compression/
			const size_t spirvOutputBufferSize = smolv::GetDecodedBufferSize(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
			// TODO(co) Try to avoid new/delete by trying to use the C-runtime stack if there aren't too many bytes
			uint8_t* spirvOutputBuffer = RHI_MALLOC_TYPED(context, uint8_t, spirvOutputBufferSize);
			smolv::Decode(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), spirvOutputBuffer, spirvOutputBufferSize);

			// Create the Vulkan shader module
			const VkShaderModuleCreateInfo vkShaderModuleCreateInfo =
			{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,			// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkShaderModuleCreateFlags)
				spirvOutputBufferSize,									// codeSize (size_t)
				reinterpret_cast<const uint32_t*>(spirvOutputBuffer)	// pCode (const uint32_t*)
			};
			VkShaderModule vkShaderModule = VK_NULL_HANDLE;
			if (vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, vkAllocationCallbacks, &vkShaderModule) != VK_SUCCESS)
			{
				RHI_LOG(context, CRITICAL, "Failed to create the Vulkan shader module")
			}

			// Done
			RHI_FREE(context, spirvOutputBuffer);
			return vkShaderModule;
		}

		/**
		*  @brief
		*    Create Vulkan shader module from source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] vkShaderStageFlagBits
		*    Vulkan shader stage flag bits (only a single set bit allowed)
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be a valid pointer
		*  @param[out] shaderBytecode
		*    If not a null pointer, this receives the shader SPIR-V bytecode compressed via SMOL-V
		*
		*  @return
		*    The Vulkan shader module, null handle on error
		*/
		// TODO(co) Visual Studio 2017 compile settings: For some reasons I need to disable optimization for the following method or else "glslang::TShader::parse()" will output the error "ERROR: 0:1: '€' : unexpected token" (glslang (latest commit c325f4364666eedb94c20a13670df058a38a14ab - April 20, 2018), Visual Studio 2017 15.7.1)
		#ifdef _MSC_VER
			#pragma optimize("", off)
		#endif
		[[nodiscard]] VkShaderModule createVkShaderModuleFromSourceCode(const Rhi::Context& context, const VkAllocationCallbacks* vkAllocationCallbacks, VkDevice vkDevice, VkShaderStageFlagBits vkShaderStageFlagBits, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode)
		{
			#ifdef RHI_VULKAN_GLSLTOSPIRV
				// Initialize glslang, if necessary
				if (!GlslangInitialized)
				{
					glslang::InitializeProcess();
					GlslangInitialized = true;
				}

				// GLSL to intermediate
				// -> OpenGL 4.5
				static constexpr int glslVersion = 450;
				EShLanguage shLanguage = EShLangCount;
				if ((vkShaderStageFlagBits & VK_SHADER_STAGE_VERTEX_BIT) != 0)
				{
					shLanguage = EShLangVertex;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) != 0)
				{
					shLanguage = EShLangTessControl;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) != 0)
				{
					shLanguage = EShLangTessEvaluation;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_GEOMETRY_BIT) != 0)
				{
					shLanguage = EShLangGeometry;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_FRAGMENT_BIT) != 0)
				{
					shLanguage = EShLangFragment;
				}
				else if ((vkShaderStageFlagBits & VK_SHADER_STAGE_COMPUTE_BIT) != 0)
				{
					shLanguage = EShLangCompute;
				}
				else
				{
					RHI_ASSERT(context, false, "Invalid Vulkan shader stage flag bits")
				}
				glslang::TShader shader(shLanguage);
				shader.setEnvInput(glslang::EShSourceGlsl, shLanguage, glslang::EShClientVulkan, glslVersion);
				shader.setEntryPoint("main");
				{
					const char* sourcePointers[] = { sourceCode };
					shader.setStrings(sourcePointers, 1);
				}
				static constexpr EShMessages shMessages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
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

							// Optional shader bytecode output
							if (nullptr != shaderBytecode)
							{
								// Encode to SMOL-V: like Vulkan/Khronos SPIR-V, but smaller
								// -> https://github.com/aras-p/smol-v
								// -> http://aras-p.info/blog/2016/09/01/SPIR-V-Compression/
								// -> Don't apply "spv::spirvbin_t::remap()" or the SMOL-V result will be bigger
								smolv::ByteArray byteArray;
								smolv::Encode(spirv.data(), sizeof(unsigned int) * spirv.size(), byteArray, smolv::kEncodeFlagStripDebugInfo);

								// Done
								shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(byteArray.size()), reinterpret_cast<uint8_t*>(byteArray.data()));
							}

							// Create the Vulkan shader module
							const VkShaderModuleCreateInfo vkShaderModuleCreateInfo =
							{
								VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,	// sType (VkStructureType)
								nullptr,										// pNext (const void*)
								0,												// flags (VkShaderModuleCreateFlags)
								sizeof(unsigned int) * spirv.size(),			// codeSize (size_t)
								spirv.data()									// pCode (const uint32_t*)
							};
							VkShaderModule vkShaderModule = VK_NULL_HANDLE;
							if (vkCreateShaderModule(vkDevice, &vkShaderModuleCreateInfo, vkAllocationCallbacks, &vkShaderModule) != VK_SUCCESS)
							{
								RHI_LOG(context, CRITICAL, "Failed to create the Vulkan shader module")
							}
							return vkShaderModule;
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

			// Error!
			return VK_NULL_HANDLE;
		}
		// TODO(co) Visual Studio 2017 compile settings: For some reasons I need to disable optimization for the following method or else "glslang::TShader::parse()" will output the error "ERROR: 0:1: '€' : unexpected token" (glslang (latest commit c325f4364666eedb94c20a13670df058a38a14ab - April 20, 2018), Visual Studio 2017 15.7.1)
		#ifdef _MSC_VER
			#pragma optimize("", on)
		#endif


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace VulkanRhi
{




	//[-------------------------------------------------------]
	//[ VulkanRhi/VulkanRhi.h                                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan RHI class
	*/
	class VulkanRhi final : public Rhi::IRhi
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class GraphicsPipelineState;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::array<VkClearValue, 9>	VkClearValues;	///< 8 color render targets and one depth stencil render target


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
		explicit VulkanRhi(const Rhi::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VulkanRhi() override;

		/**
		*  @brief
		*    Return the Vulkan allocation callbacks
		*
		*  @return
		*    Vulkan allocation callbacks, can be a null pointer, don't destroy the instance
		*/
		[[nodiscard]] inline const VkAllocationCallbacks* getVkAllocationCallbacks() const
		{
			#ifdef VK_USE_PLATFORM_WIN32_KHR
				return &mVkAllocationCallbacks;
			#else
				#warning TODO(co) The "Rhi::DefaultAllocator" implementation is currently only tested on MS Window, since Vulkan is using aligment it must be sure the custom standard implemtation runs fine
				return nullptr;
			#endif
		}

		/**
		*  @brief
		*    Return the Vulkan runtime linking instance
		*
		*  @return
		*    The Vulkan runtime linking instance, do not free the memory the reference is pointing to
		*/
		[[nodiscard]] inline const VulkanRuntimeLinking& getVulkanRuntimeLinking() const
		{
			return *mVulkanRuntimeLinking;
		}

		/**
		*  @brief
		*    Return the Vulkan context instance
		*
		*  @return
		*    The Vulkan context instance, do not free the memory the reference is pointing to
		*/
		[[nodiscard]] inline const VulkanContext& getVulkanContext() const
		{
			return *mVulkanContext;
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
		//[-------------------------------------------------------]
		//[ Compute                                               ]
		//[-------------------------------------------------------]
		void setComputeRootSignature(Rhi::IRootSignature* rootSignature);
		void setComputePipelineState(Rhi::IComputePipelineState* computePipelineState);
		void setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup);
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
		[[nodiscard]] virtual bool getQueryPoolResults(Rhi::IQueryPool& queryPool, uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex = 0, uint32_t numberOfQueries = 1, uint32_t strideInBytes = 0, uint32_t queryResultFlags = Rhi::QueryResultFlags::WAIT) override;
		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual bool beginScene() override;
		virtual void submitCommandBuffer(const Rhi::CommandBuffer& commandBuffer) override;
		virtual void endScene() override;
		//[-------------------------------------------------------]
		//[ Synchronization                                       ]
		//[-------------------------------------------------------]
		virtual void flush() override;
		virtual void finish() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(mContext, VulkanRhi, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VulkanRhi(const VulkanRhi& source) = delete;
		VulkanRhi& operator =(const VulkanRhi& source) = delete;

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
		*    Begin Vulkan render pass
		*/
		void beginVulkanRenderPass();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkAllocationCallbacks mVkAllocationCallbacks;	///< Vulkan allocation callbacks
		VulkanRuntimeLinking* mVulkanRuntimeLinking;	///< Vulkan runtime linking instance, always valid
		VulkanContext*		  mVulkanContext;			///< Vulkan context instance, always valid
		Rhi::IShaderLanguage* mShaderLanguageGlsl;		///< GLSL shader language instance (we keep a reference to it), can be a null pointer
		RootSignature*		  mGraphicsRootSignature;	///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*		  mComputeRootSignature;	///< Currently set compute root signature (we keep a reference to it), can be a null pointer
		Rhi::ISamplerState*	  mDefaultSamplerState;		///< Default rasterizer state (we keep a reference to it), can be a null pointer
		bool				  mInsideVulkanRenderPass;	///< Some Vulkan commands like "vkCmdClearColorImage()" can only be executed outside a Vulkan render pass, so need to delay starting a Vulkan render pass
		VkClearValues		  mVkClearValues;
		//[-------------------------------------------------------]
		//[ Input-assembler (IA) stage                            ]
		//[-------------------------------------------------------]
		VertexArray* mVertexArray;	///< Currently set vertex array (we keep a reference to it), can be a null pointer
		//[-------------------------------------------------------]
		//[ Output-merger (OM) stage                              ]
		//[-------------------------------------------------------]
		Rhi::IRenderTarget* mRenderTarget;	///< Currently set render target (we keep a reference to it), can be a null pointer
		#ifdef RHI_DEBUG
			bool mDebugBetweenBeginEndScene;	///< Just here for state tracking in debug builds
		#endif


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/VulkanRuntimeLinking.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan runtime linking for creating and managing the Vulkan instance ("VkInstance")
	*/
	class VulkanRuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] enableValidation
		*    Enable validation?
		*/
		inline VulkanRuntimeLinking(VulkanRhi& vulkanRhi, bool enableValidation) :
			mVulkanRhi(vulkanRhi),
			mValidationEnabled(enableValidation),
			mVulkanSharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mVkInstance(VK_NULL_HANDLE),
			mVkDebugReportCallbackEXT(VK_NULL_HANDLE),
			mInstanceLevelFunctionsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		~VulkanRuntimeLinking()
		{
			// Destroy the Vulkan debug report callback
			if (VK_NULL_HANDLE != mVkDebugReportCallbackEXT)
			{
				vkDestroyDebugReportCallbackEXT(mVkInstance, mVkDebugReportCallbackEXT, mVulkanRhi.getVkAllocationCallbacks());
			}

			// Destroy the Vulkan instance
			if (VK_NULL_HANDLE != mVkInstance)
			{
				vkDestroyInstance(mVkInstance, mVulkanRhi.getVkAllocationCallbacks());
			}

			// Destroy the shared library instances
			#ifdef _WIN32
				if (nullptr != mVulkanSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mVulkanSharedLibrary));
				}
			#elif defined LINUX
				if (nullptr != mVulkanSharedLibrary)
				{
					::dlclose(mVulkanSharedLibrary);
				}
			#else
				#error "Unsupported platform"
			#endif
		}

		/**
		*  @brief
		*    Return whether or not validation is enabled
		*
		*  @return
		*    "true" if validation is enabled, else "false"
		*/
		[[nodiscard]] inline bool isValidationEnabled() const
		{
			return mValidationEnabled;
		}

		/**
		*  @brief
		*    Return whether or not Vulkan is available
		*
		*  @return
		*    "true" if Vulkan is available, else "false"
		*/
		[[nodiscard]] bool isVulkanAvaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the global level Vulkan function entry points
					mEntryPointsRegistered = loadGlobalLevelVulkanEntryPoints();
					if (mEntryPointsRegistered)
					{
						// Create the Vulkan instance
						const VkResult vkResult = createVulkanInstance(mValidationEnabled);
						if (VK_SUCCESS == vkResult)
						{
							// Load instance based instance level Vulkan function pointers
							mInstanceLevelFunctionsRegistered = loadInstanceLevelVulkanEntryPoints();

							// Setup debug callback
							if (mInstanceLevelFunctionsRegistered && mValidationEnabled)
							{
								setupDebugCallback();
							}
						}
						else
						{
							// Error!
							RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan instance")
						}
					}
				}
			}

			// Entry points successfully registered?
			return (mEntryPointsRegistered && (VK_NULL_HANDLE != mVkInstance) && mInstanceLevelFunctionsRegistered);
		}

		/**
		*  @brief
		*    Return the Vulkan instance
		*
		*  @return
		*    Vulkan instance
		*/
		[[nodiscard]] inline VkInstance getVkInstance() const
		{
			return mVkInstance;
		}

		/**
		*  @brief
		*    Load the device level Vulkan function entry points
		*
		*  @param[in] vkDevice
		*    Vulkan device instance to load the function entry pointers for
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadDeviceLevelVulkanEntryPoints(VkDevice vkDevice) const
		{
			bool result = true;	// Success by default

			// Define a helper macro
			PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(4191)	// 'reinterpret_cast': unsafe conversion from 'PFN_vkVoidFunction' to '<x>'
			#define IMPORT_FUNC(funcName)																										\
				if (result)																														\
				{																																\
					funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(vkDevice, #funcName));										\
					if (nullptr == funcName)																									\
					{																															\
						RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to load instance based Vulkan function pointer \"%s\"", #funcName)	\
						result = false;																											\
					}																															\
				}

			// Load the Vulkan device level function entry points
			IMPORT_FUNC(vkDestroyDevice)
			IMPORT_FUNC(vkCreateShaderModule)
			IMPORT_FUNC(vkDestroyShaderModule)
			IMPORT_FUNC(vkCreateBuffer)
			IMPORT_FUNC(vkDestroyBuffer)
			IMPORT_FUNC(vkMapMemory)
			IMPORT_FUNC(vkUnmapMemory)
			IMPORT_FUNC(vkCreateBufferView)
			IMPORT_FUNC(vkDestroyBufferView)
			IMPORT_FUNC(vkAllocateMemory)
			IMPORT_FUNC(vkFreeMemory)
			IMPORT_FUNC(vkGetBufferMemoryRequirements)
			IMPORT_FUNC(vkBindBufferMemory)
			IMPORT_FUNC(vkCreateRenderPass)
			IMPORT_FUNC(vkDestroyRenderPass)
			IMPORT_FUNC(vkCreateImage)
			IMPORT_FUNC(vkDestroyImage)
			IMPORT_FUNC(vkGetImageSubresourceLayout)
			IMPORT_FUNC(vkGetImageMemoryRequirements)
			IMPORT_FUNC(vkBindImageMemory)
			IMPORT_FUNC(vkCreateImageView)
			IMPORT_FUNC(vkDestroyImageView)
			IMPORT_FUNC(vkCreateSampler)
			IMPORT_FUNC(vkDestroySampler)
			IMPORT_FUNC(vkCreateSemaphore)
			IMPORT_FUNC(vkDestroySemaphore)
			IMPORT_FUNC(vkCreateFence)
			IMPORT_FUNC(vkDestroyFence)
			IMPORT_FUNC(vkWaitForFences)
			IMPORT_FUNC(vkCreateCommandPool)
			IMPORT_FUNC(vkDestroyCommandPool)
			IMPORT_FUNC(vkAllocateCommandBuffers)
			IMPORT_FUNC(vkFreeCommandBuffers)
			IMPORT_FUNC(vkBeginCommandBuffer)
			IMPORT_FUNC(vkEndCommandBuffer)
			IMPORT_FUNC(vkGetDeviceQueue)
			IMPORT_FUNC(vkQueueSubmit)
			IMPORT_FUNC(vkQueueWaitIdle)
			IMPORT_FUNC(vkDeviceWaitIdle)
			IMPORT_FUNC(vkCreateFramebuffer)
			IMPORT_FUNC(vkDestroyFramebuffer)
			IMPORT_FUNC(vkCreatePipelineCache)
			IMPORT_FUNC(vkDestroyPipelineCache)
			IMPORT_FUNC(vkCreatePipelineLayout)
			IMPORT_FUNC(vkDestroyPipelineLayout)
			IMPORT_FUNC(vkCreateGraphicsPipelines)
			IMPORT_FUNC(vkCreateComputePipelines)
			IMPORT_FUNC(vkDestroyPipeline)
			IMPORT_FUNC(vkCreateDescriptorPool)
			IMPORT_FUNC(vkDestroyDescriptorPool)
			IMPORT_FUNC(vkCreateDescriptorSetLayout)
			IMPORT_FUNC(vkDestroyDescriptorSetLayout)
			IMPORT_FUNC(vkAllocateDescriptorSets)
			IMPORT_FUNC(vkFreeDescriptorSets)
			IMPORT_FUNC(vkUpdateDescriptorSets)
			IMPORT_FUNC(vkCreateQueryPool)
			IMPORT_FUNC(vkDestroyQueryPool)
			IMPORT_FUNC(vkGetQueryPoolResults)
			IMPORT_FUNC(vkCmdBeginQuery)
			IMPORT_FUNC(vkCmdEndQuery)
			IMPORT_FUNC(vkCmdResetQueryPool)
			IMPORT_FUNC(vkCmdWriteTimestamp)
			IMPORT_FUNC(vkCmdCopyQueryPoolResults)
			IMPORT_FUNC(vkCmdPipelineBarrier)
			IMPORT_FUNC(vkCmdBeginRenderPass)
			IMPORT_FUNC(vkCmdEndRenderPass)
			IMPORT_FUNC(vkCmdExecuteCommands)
			IMPORT_FUNC(vkCmdCopyImage)
			IMPORT_FUNC(vkCmdBlitImage)
			IMPORT_FUNC(vkCmdCopyBufferToImage)
			IMPORT_FUNC(vkCmdClearAttachments)
			IMPORT_FUNC(vkCmdCopyBuffer)
			IMPORT_FUNC(vkCmdBindDescriptorSets)
			IMPORT_FUNC(vkCmdBindPipeline)
			IMPORT_FUNC(vkCmdSetViewport)
			IMPORT_FUNC(vkCmdSetScissor)
			IMPORT_FUNC(vkCmdSetLineWidth)
			IMPORT_FUNC(vkCmdSetDepthBias)
			IMPORT_FUNC(vkCmdPushConstants)
			IMPORT_FUNC(vkCmdBindIndexBuffer)
			IMPORT_FUNC(vkCmdBindVertexBuffers)
			IMPORT_FUNC(vkCmdDraw)
			IMPORT_FUNC(vkCmdDrawIndexed)
			IMPORT_FUNC(vkCmdDrawIndirect)
			IMPORT_FUNC(vkCmdDrawIndexedIndirect)
			IMPORT_FUNC(vkCmdDispatch)
			IMPORT_FUNC(vkCmdClearColorImage)
			IMPORT_FUNC(vkCmdClearDepthStencilImage)
			// "VK_KHR_swapchain"-extension
			IMPORT_FUNC(vkCreateSwapchainKHR)
			IMPORT_FUNC(vkDestroySwapchainKHR)
			IMPORT_FUNC(vkGetSwapchainImagesKHR)
			IMPORT_FUNC(vkAcquireNextImageKHR)
			IMPORT_FUNC(vkQueuePresentKHR)

			// Undefine the helper macro
			#undef IMPORT_FUNC
			PRAGMA_WARNING_POP

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VulkanRuntimeLinking(const VulkanRuntimeLinking& source) = delete;
		VulkanRuntimeLinking& operator =(const VulkanRuntimeLinking& source) = delete;

		/**
		*  @brief
		*    Load the shared libraries
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadSharedLibraries()
		{
			// Load the shared library
			#ifdef _WIN32
				mVulkanSharedLibrary = ::LoadLibraryExA("vulkan-1.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr == mVulkanSharedLibrary)
				{
					RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to load in the shared Vulkan library \"vulkan-1.dll\"")
				}
			#elif defined LINUX
				mVulkanSharedLibrary = ::dlopen("libvulkan.so", RTLD_NOW);
				if (nullptr == mVulkanSharedLibrary)
				{
					RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to load in the shared Vulkan library \"libvulkan-1.so\"")
				}
			#else
				#error "Unsupported platform"
			#endif

			// Done
			return (nullptr != mVulkanSharedLibrary);
		}

		/**
		*  @brief
		*    Load the global level Vulkan function entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadGlobalLevelVulkanEntryPoints() const
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#ifdef _WIN32
				#define IMPORT_FUNC(funcName)																																			\
					if (result)																																							\
					{																																									\
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mVulkanSharedLibrary), #funcName);																			\
						if (nullptr != symbol)																																			\
						{																																								\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																											\
						}																																								\
						else																																							\
						{																																								\
							wchar_t moduleFilename[MAX_PATH];																															\
							moduleFilename[0] = '\0';																																	\
							::GetModuleFileNameW(static_cast<HMODULE>(mVulkanSharedLibrary), moduleFilename, MAX_PATH);																	\
							RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the shared Vulkan library \"%s\"", #funcName, moduleFilename)	\
							result = false;																																				\
						}																																								\
					}
			#elif defined(__ANDROID__)
				#define IMPORT_FUNC(funcName)																																				\
					if (result)																																								\
					{																																										\
						void* symbol = ::dlsym(mVulkanSharedLibrary, #funcName);																											\
						if (nullptr != symbol)																																				\
						{																																									\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																												\
						}																																									\
						else																																								\
						{																																									\
							const char* libraryName = "unknown";																															\
							RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to locate the Vulkan entry point \"%s\" within the Vulkan shared library \"%s\"", #funcName, libraryName)	\
							result = false;																																					\
						}																																									\
					}
			#elif defined LINUX
				#define IMPORT_FUNC(funcName)																																		\
					if (result)																																						\
					{																																								\
						void* symbol = ::dlsym(mVulkanSharedLibrary, #funcName);																									\
						if (nullptr != symbol)																																		\
						{																																							\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																										\
						}																																							\
						else																																						\
						{																																							\
							link_map *linkMap = nullptr;																															\
							const char* libraryName = "unknown";																													\
							if (dlinfo(mVulkanSharedLibrary, RTLD_DI_LINKMAP, &linkMap))																							\
							{																																						\
								libraryName = linkMap->l_name;																														\
							}																																						\
							RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the shared Vulkan library \"%s\"", #funcName, libraryName)	\
							result = false;																																			\
						}																																							\
					}
			#else
				#error "Unsupported platform"
			#endif

			// Load the Vulkan global level function entry points
			IMPORT_FUNC(vkGetInstanceProcAddr);
			IMPORT_FUNC(vkGetDeviceProcAddr);
			IMPORT_FUNC(vkEnumerateInstanceExtensionProperties);
			IMPORT_FUNC(vkEnumerateInstanceLayerProperties);
			IMPORT_FUNC(vkCreateInstance);

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Create the Vulkan instance
		*
		*  @param[in] enableValidation
		*    Enable validation layer? (don't do this for shipped products)
		*
		*  @return
		*    Vulkan instance creation result
		*/
		[[nodiscard]] VkResult createVulkanInstance(bool enableValidation)
		{
			// Enable surface extensions depending on OS
			std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
			#ifdef VK_USE_PLATFORM_WIN32_KHR
				enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			#elif defined VK_USE_PLATFORM_ANDROID_KHR
				#warning "TODO(co) Not tested"
				enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
			#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
				#if defined VK_USE_PLATFORM_XLIB_KHR
					enabledExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
				#endif
				#if defined VK_USE_PLATFORM_WAYLAND_KHR
					enabledExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
				#endif
			#elif defined VK_USE_PLATFORM_XCB_KHR
				#warning "TODO(co) Not tested"
				enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
			#else
				#error "Unsupported platform"
			#endif
			if (enableValidation)
			{
				enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}

			{ // Ensure the extensions we need are supported
				uint32_t propertyCount = 0;
				if ((vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr) != VK_SUCCESS) || (0 == propertyCount))
				{
					RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to enumerate Vulkan instance extension properties")
					return VK_ERROR_EXTENSION_NOT_PRESENT;
				}
				::detail::VkExtensionPropertiesVector vkExtensionPropertiesVector(propertyCount);
				if (vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, &vkExtensionPropertiesVector[0]) != VK_SUCCESS)
				{
					RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to enumerate Vulkan instance extension properties")
					return VK_ERROR_EXTENSION_NOT_PRESENT;
				}
				for (const char* enabledExtension : enabledExtensions)
				{
					if (!::detail::isExtensionAvailable(enabledExtension, vkExtensionPropertiesVector))
					{
						RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Couldn't find Vulkan instance extension named \"%s\"", enabledExtension)
						return VK_ERROR_EXTENSION_NOT_PRESENT;
					}
				}
			}

			// TODO(co) Make it possible for the user to provide application related information?
			static constexpr VkApplicationInfo vkApplicationInfo =
			{
				VK_STRUCTURE_TYPE_APPLICATION_INFO,	// sType (VkStructureType)
				nullptr,							// pNext (const void*)
				"Unrimp Application",				// pApplicationName (const char*)
				VK_MAKE_VERSION(0, 0, 0),			// applicationVersion (uint32_t)
				"Unrimp",							// pEngineName (const char*)
				VK_MAKE_VERSION(0, 0, 0),			// engineVersion (uint32_t)
				VK_API_VERSION_1_0					// apiVersion (uint32_t)
			};

			const VkInstanceCreateInfo vkInstanceCreateInfo =
			{
				VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,							// sType (VkStructureType)
				nullptr,														// pNext (const void*)
				0,																// flags (VkInstanceCreateFlags)
				&vkApplicationInfo,												// pApplicationInfo (const VkApplicationInfo*)
				enableValidation ? ::detail::NUMBER_OF_VALIDATION_LAYERS : 0,	// enabledLayerCount (uint32_t)
				enableValidation ? ::detail::VALIDATION_LAYER_NAMES : nullptr,	// ppEnabledLayerNames (const char* const*)
				static_cast<uint32_t>(enabledExtensions.size()),				// enabledExtensionCount (uint32_t)
				enabledExtensions.data()										// ppEnabledExtensionNames (const char* const*)
			};
			VkResult vkResult = vkCreateInstance(&vkInstanceCreateInfo, mVulkanRhi.getVkAllocationCallbacks(), &mVkInstance);
			if (VK_ERROR_LAYER_NOT_PRESENT == vkResult && enableValidation)
			{
				// Error! Since the show must go on, try creating a Vulkan instance without validation enabled...
				RHI_LOG(mVulkanRhi.getContext(), WARNING, "Failed to create the Vulkan instance with validation enabled, layer is not present. Install e.g. the LunarG Vulkan SDK and see e.g. https://vulkan.lunarg.com/doc/view/1.0.51.0/windows/layers.html .")
				mValidationEnabled = false;
				vkResult = createVulkanInstance(mValidationEnabled);
			}

			// Done
			return vkResult;
		}

		/**
		*  @brief
		*    Load the instance level Vulkan function entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadInstanceLevelVulkanEntryPoints() const
		{
			bool result = true;	// Success by default

			// Define a helper macro
			PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(4191)	// 'reinterpret_cast': unsafe conversion from 'PFN_vkVoidFunction' to '<x>'
			#define IMPORT_FUNC(funcName)																										\
				if (result)																														\
				{																																\
					funcName = reinterpret_cast<PFN_##funcName>(vkGetInstanceProcAddr(mVkInstance, #funcName));									\
					if (nullptr == funcName)																									\
					{																															\
						RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to load instance based Vulkan function pointer \"%s\"", #funcName)	\
						result = false;																											\
					}																															\
				}

			// Load the Vulkan instance level function entry points
			IMPORT_FUNC(vkDestroyInstance)
			IMPORT_FUNC(vkEnumeratePhysicalDevices)
			IMPORT_FUNC(vkEnumerateDeviceLayerProperties)
			IMPORT_FUNC(vkEnumerateDeviceExtensionProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceFeatures)
			IMPORT_FUNC(vkGetPhysicalDeviceFormatProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceMemoryProperties)
			IMPORT_FUNC(vkGetPhysicalDeviceProperties)
			IMPORT_FUNC(vkCreateDevice)
			if (mValidationEnabled)
			{
				// "VK_EXT_debug_report"-extension
				IMPORT_FUNC(vkCreateDebugReportCallbackEXT)
				IMPORT_FUNC(vkDestroyDebugReportCallbackEXT)
			}
			// "VK_KHR_surface"-extension
			IMPORT_FUNC(vkDestroySurfaceKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
			IMPORT_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR)
			#ifdef VK_USE_PLATFORM_WIN32_KHR
				// "VK_KHR_win32_surface"-extension
				IMPORT_FUNC(vkCreateWin32SurfaceKHR)
			#elif defined VK_USE_PLATFORM_ANDROID_KHR
				// "VK_KHR_android_surface"-extension
				#warning "TODO(co) Not tested"
				IMPORT_FUNC(vkCreateAndroidSurfaceKHR)
			#elif defined VK_USE_PLATFORM_XLIB_KHR || defined VK_USE_PLATFORM_WAYLAND_KHR
				#if defined VK_USE_PLATFORM_XLIB_KHR
					// "VK_KHR_xlib_surface"-extension
					IMPORT_FUNC(vkCreateXlibSurfaceKHR)
				#endif
				#if defined VK_USE_PLATFORM_WAYLAND_KHR
					// "VK_KHR_wayland_surface"-extension
					IMPORT_FUNC(vkCreateWaylandSurfaceKHR)
				#endif
			#elif defined VK_USE_PLATFORM_XCB_KHR
				// "VK_KHR_xcb_surface"-extension
				#warning "TODO(co) Not tested"
				IMPORT_FUNC(vkCreateXcbSurfaceKHR)
			#else
				#error "Unsupported platform"
			#endif

			// Undefine the helper macro
			#undef IMPORT_FUNC
			PRAGMA_WARNING_POP

			// Done
			return result;
		}

		/**
		*  @brief
		*    Setup debug callback
		*/
		void setupDebugCallback()
		{
			// Sanity check
			RHI_ASSERT(mVulkanRhi.getContext(), mValidationEnabled, "Do only call this Vulkan method if validation is enabled")

			// The report flags determine what type of messages for the layers will be displayed
			// -> Use "VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT" to get everything, quite verbose
			static constexpr VkDebugReportFlagsEXT vkDebugReportFlagsEXT = (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);

			// Setup debug callback
			const VkDebugReportCallbackCreateInfoEXT vkDebugReportCallbackCreateInfoEXT =
			{
				VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,	// sType (VkStructureType)
				nullptr,													// pNext (const void*)
				vkDebugReportFlagsEXT,										// flags (VkDebugReportFlagsEXT)
				::detail::debugReportCallback,								// pfnCallback (PFN_vkDebugReportCallbackEXT)
				const_cast<Rhi::Context*>(&mVulkanRhi.getContext())			// pUserData (void*)
			};
			if (vkCreateDebugReportCallbackEXT(mVkInstance, &vkDebugReportCallbackCreateInfoEXT, mVulkanRhi.getVkAllocationCallbacks(), &mVkDebugReportCallbackEXT) != VK_SUCCESS)
			{
				RHI_LOG(mVulkanRhi.getContext(), WARNING, "Failed to create the Vulkan debug report callback")
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VulkanRhi&				 mVulkanRhi;						///< Owner Vulkan RHI instance
		bool					 mValidationEnabled;				///< Validation enabled?
		void*					 mVulkanSharedLibrary;				///< Vulkan shared library, can be a null pointer
		bool					 mEntryPointsRegistered;			///< Entry points successfully registered?
		VkInstance				 mVkInstance;						///< Vulkan instance, stores all per-application states
		VkDebugReportCallbackEXT mVkDebugReportCallbackEXT;			///< Vulkan debug report callback, can be a null handle
		bool					 mInstanceLevelFunctionsRegistered;	///< Instance level Vulkan function pointers registered?
		bool					 mInitialized;						///< Already initialized?


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/VulkanContext.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan context class
	*/
	class VulkanContext final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*/
		explicit VulkanContext(VulkanRhi& vulkanRhi) :
			mVulkanRhi(vulkanRhi),
			mVkPhysicalDevice(VK_NULL_HANDLE),
			mVkDevice(VK_NULL_HANDLE),
			mGraphicsQueueFamilyIndex(~0u),
			mPresentQueueFamilyIndex(~0u),
			mGraphicsVkQueue(VK_NULL_HANDLE),
			mPresentVkQueue(VK_NULL_HANDLE),
			mVkCommandPool(VK_NULL_HANDLE),
			mVkCommandBuffer(VK_NULL_HANDLE)
		{
			const VulkanRuntimeLinking& vulkanRuntimeLinking = mVulkanRhi.getVulkanRuntimeLinking();

			// Get the physical Vulkan device this context should use
			bool enableDebugMarker = true;	// TODO(co) Make it possible to setup from the outside whether or not the "VK_EXT_debug_marker"-extension should be used (e.g. retail shipped games might not want to have this enabled)
			{
				detail::VkPhysicalDevices vkPhysicalDevices;
				::detail::enumeratePhysicalDevices(vulkanRhi.getContext(), vulkanRuntimeLinking.getVkInstance(), vkPhysicalDevices);
				if (!vkPhysicalDevices.empty())
				{
					mVkPhysicalDevice = ::detail::selectPhysicalDevice(vulkanRhi.getContext(), vkPhysicalDevices, vulkanRhi.getVulkanRuntimeLinking().isValidationEnabled(), enableDebugMarker);
				}
			}

			// Create the logical Vulkan device instance
			if (VK_NULL_HANDLE != mVkPhysicalDevice)
			{
				mVkDevice = ::detail::createVkDevice(mVulkanRhi.getContext(), mVulkanRhi.getVkAllocationCallbacks(), mVkPhysicalDevice, vulkanRuntimeLinking.isValidationEnabled(), enableDebugMarker, mGraphicsQueueFamilyIndex, mPresentQueueFamilyIndex);
				if (VK_NULL_HANDLE != mVkDevice)
				{
					// Load device based instance level Vulkan function pointers
					if (mVulkanRhi.getVulkanRuntimeLinking().loadDeviceLevelVulkanEntryPoints(mVkDevice))
					{
						// Get the Vulkan device graphics queue that command buffers are submitted to
						vkGetDeviceQueue(mVkDevice, mGraphicsQueueFamilyIndex, 0, &mGraphicsVkQueue);
						if (VK_NULL_HANDLE != mGraphicsVkQueue)
						{
							// Get the Vulkan device present queue
							vkGetDeviceQueue(mVkDevice, mPresentQueueFamilyIndex, 0, &mPresentVkQueue);
							if (VK_NULL_HANDLE != mPresentVkQueue)
							{
								// Create Vulkan command pool instance
								mVkCommandPool = ::detail::createVkCommandPool(mVulkanRhi.getContext(), mVulkanRhi.getVkAllocationCallbacks(), mVkDevice, mGraphicsQueueFamilyIndex);
								if (VK_NULL_HANDLE != mVkCommandPool)
								{
									// Create Vulkan command buffer instance
									mVkCommandBuffer = ::detail::createVkCommandBuffer(mVulkanRhi.getContext(), mVkDevice, mVkCommandPool);
								}
								else
								{
									// Error!
									RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create Vulkan command pool instance")
								}
							}
						}
						else
						{
							// Error!
							RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to get the Vulkan device graphics queue that command buffers are submitted to")
						}
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		~VulkanContext()
		{
			if (VK_NULL_HANDLE != mVkDevice)
			{
				if (VK_NULL_HANDLE != mVkCommandPool)
				{
					if (VK_NULL_HANDLE != mVkCommandBuffer)
					{
						vkFreeCommandBuffers(mVkDevice, mVkCommandPool, 1, &mVkCommandBuffer);
					}
					vkDestroyCommandPool(mVkDevice, mVkCommandPool, mVulkanRhi.getVkAllocationCallbacks());
				}
				vkDeviceWaitIdle(mVkDevice);
				vkDestroyDevice(mVkDevice, mVulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return whether or not the content is initialized
		*
		*  @return
		*    "true" if the context is initialized, else "false"
		*/
		[[nodiscard]] inline bool isInitialized() const
		{
			return (VK_NULL_HANDLE != mVkCommandBuffer);
		}

		/**
		*  @brief
		*    Return the owner Vulkan RHI instance
		*
		*  @return
		*    Owner Vulkan RHI instance
		*/
		[[nodiscard]] inline VulkanRhi& getVulkanRhi() const
		{
			return mVulkanRhi;
		}

		/**
		*  @brief
		*    Return the Vulkan physical device this context is using
		*
		*  @return
		*    The Vulkan physical device this context is using
		*/
		[[nodiscard]] inline VkPhysicalDevice getVkPhysicalDevice() const
		{
			return mVkPhysicalDevice;
		}

		/**
		*  @brief
		*    Return the Vulkan device this context is using
		*
		*  @return
		*    The Vulkan device this context is using
		*/
		[[nodiscard]] inline VkDevice getVkDevice() const
		{
			return mVkDevice;
		}

		/**
		*  @brief
		*    Return the used graphics queue family index
		*
		*  @return
		*    Graphics queue family index, ~0u if invalid
		*/
		[[nodiscard]] inline uint32_t getGraphicsQueueFamilyIndex() const
		{
			return mGraphicsQueueFamilyIndex;
		}

		/**
		*  @brief
		*    Return the used present queue family index
		*
		*  @return
		*    Present queue family index, ~0u if invalid
		*/
		[[nodiscard]] inline uint32_t getPresentQueueFamilyIndex() const
		{
			return mPresentQueueFamilyIndex;
		}

		/**
		*  @brief
		*    Return the handle to the Vulkan device graphics queue that command buffers are submitted to
		*
		*  @return
		*    Handle to the Vulkan device graphics queue that command buffers are submitted to
		*/
		[[nodiscard]] inline VkQueue getGraphicsVkQueue() const
		{
			return mGraphicsVkQueue;
		}

		/**
		*  @brief
		*    Return the handle to the Vulkan device present queue
		*
		*  @return
		*    Handle to the Vulkan device present queue
		*/
		[[nodiscard]] inline VkQueue getPresentVkQueue() const
		{
			return mPresentVkQueue;
		}

		/**
		*  @brief
		*    Return the used Vulkan command buffer pool instance
		*
		*  @return
		*    The used Vulkan command buffer pool instance
		*/
		[[nodiscard]] inline VkCommandPool getVkCommandPool() const
		{
			return mVkCommandPool;
		}

		/**
		*  @brief
		*    Return the Vulkan command buffer instance
		*
		*  @return
		*    The Vulkan command buffer instance
		*/
		[[nodiscard]] inline VkCommandBuffer getVkCommandBuffer() const
		{
			return mVkCommandBuffer;
		}

		// TODO(co) Trivial implementation to have something to start with. Need to use more clever memory management and stating buffers later on.
		[[nodiscard]] uint32_t findMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags vkMemoryPropertyFlags) const
		{
			VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
			vkGetPhysicalDeviceMemoryProperties(mVkPhysicalDevice, &vkPhysicalDeviceMemoryProperties);
			for (uint32_t i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; ++i)
			{
				if ((typeFilter & (1 << i)) && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & vkMemoryPropertyFlags) == vkMemoryPropertyFlags)
				{
					return i;
				}
			}

			// Error!
			RHI_LOG(mVulkanRhi.getContext(), CRITICAL, "Failed to find suitable Vulkan memory type")
			return ~0u;
		}

		[[nodiscard]] inline VkCommandBuffer createVkCommandBuffer() const
		{
			return ::detail::createVkCommandBuffer(mVulkanRhi.getContext(), mVkDevice, mVkCommandPool);
		}

		void destroyVkCommandBuffer(VkCommandBuffer vkCommandBuffer) const
		{
			if (VK_NULL_HANDLE != mVkCommandBuffer)
			{
				vkFreeCommandBuffers(mVkDevice, mVkCommandPool, 1, &vkCommandBuffer);
			}
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit VulkanContext(const VulkanContext& source) = delete;
		VulkanContext& operator =(const VulkanContext& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VulkanRhi&		 mVulkanRhi;				///< Owner Vulkan RHI instance
		VkPhysicalDevice mVkPhysicalDevice;			///< Vulkan physical device this context is using
		VkDevice		 mVkDevice;					///< Vulkan device instance this context is using (equivalent of a OpenGL context or Direct3D 11 device)
		uint32_t		 mGraphicsQueueFamilyIndex;	///< Graphics queue family index, ~0u if invalid
		uint32_t		 mPresentQueueFamilyIndex;	///< Present queue family index, ~0u if invalid
		VkQueue			 mGraphicsVkQueue;			///< Handle to the Vulkan device graphics queue that command buffers are submitted to
		VkQueue			 mPresentVkQueue;			///< Handle to the Vulkan device present queue
		VkCommandPool	 mVkCommandPool;			///< Vulkan command buffer pool instance
		VkCommandBuffer  mVkCommandBuffer;			///< Vulkan command buffer instance


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Mapping.h                                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan mapping
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
		*    "Rhi::FilterMode" to Vulkan magnification filter mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    Vulkan magnification filter mode
		*/
		[[nodiscard]] static VkFilter getVulkanMagFilterMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Rhi::FilterMode::MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "Vulkan filter mode must not be unknown")
					return VK_FILTER_NEAREST;

				default:
					return VK_FILTER_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Rhi::FilterMode" to Vulkan minification filter mode
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    Vulkan minification filter mode
		*/
		[[nodiscard]] static VkFilter getVulkanMinFilterMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Rhi::FilterMode::MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return VK_FILTER_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return VK_FILTER_LINEAR;

				case Rhi::FilterMode::COMPARISON_ANISOTROPIC:
					return VK_FILTER_LINEAR;	// There's no special setting in Vulkan

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "Vulkan filter mode must not be unknown")
					return VK_FILTER_NEAREST;

				default:
					return VK_FILTER_NEAREST;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Rhi::FilterMode" to Vulkan sampler mipmap mode
		*
		*  @param[in] context
		*    Rhi context to use
		*  @param[in] filterMode
		*    "Rhi::FilterMode" to map
		*
		*  @return
		*    Vulkan sampler mipmap mode
		*/
		[[nodiscard]] static VkSamplerMipmapMode getVulkanMipmapMode([[maybe_unused]] const Rhi::Context& context, Rhi::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Rhi::FilterMode::MIN_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::MIN_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::ANISOTROPIC:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;	// There's no special setting in Vulkan

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				case Rhi::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;

				case Rhi::FilterMode::COMPARISON_ANISOTROPIC:
					return VK_SAMPLER_MIPMAP_MODE_LINEAR;	// There's no special setting in Vulkan

				case Rhi::FilterMode::UNKNOWN:
					RHI_ASSERT(context, false, "Vulkan filter mode must not be unknown")
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;

				default:
					return VK_SAMPLER_MIPMAP_MODE_NEAREST;	// We should never be in here
			}
		}

		//[-------------------------------------------------------]
		//[ Rhi::TextureAddressMode                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::TextureAddressMode" to Vulkan texture address mode
		*
		*  @param[in] textureAddressMode
		*    "Rhi::TextureAddressMode" to map
		*
		*  @return
		*    Vulkan texture address mode
		*/
		[[nodiscard]] static VkSamplerAddressMode getVulkanTextureAddressMode(Rhi::TextureAddressMode textureAddressMode)
		{
			static constexpr VkSamplerAddressMode MAPPING[] =
			{
				VK_SAMPLER_ADDRESS_MODE_REPEAT,					// Rhi::TextureAddressMode::WRAP
				VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,		// Rhi::TextureAddressMode::MIRROR
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,			// Rhi::TextureAddressMode::CLAMP
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,		// Rhi::TextureAddressMode::BORDER
				VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE	// Rhi::TextureAddressMode::MIRROR_ONCE
			};
			return MAPPING[static_cast<int>(textureAddressMode) - 1];	// Lookout! The "Rhi::TextureAddressMode"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::Blend                                            ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::Blend" to Vulkan blend factor
		*
		*  @param[in] blend
		*    "Rhi::Blend" to map
		*
		*  @return
		*    Vulkan blend factor
		*/
		[[nodiscard]] static VkBlendFactor getVulkanBlendFactor(Rhi::Blend blend)
		{
			static constexpr VkBlendFactor MAPPING[] =
			{
				VK_BLEND_FACTOR_ZERO,						// Rhi::Blend::ZERO				 = 1
				VK_BLEND_FACTOR_ONE,						// Rhi::Blend::ONE				 = 2
				VK_BLEND_FACTOR_SRC_COLOR,					// Rhi::Blend::SRC_COLOR		 = 3
				VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,		// Rhi::Blend::INV_SRC_COLOR	 = 4
				VK_BLEND_FACTOR_SRC_ALPHA,					// Rhi::Blend::SRC_ALPHA		 = 5
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,		// Rhi::Blend::INV_SRC_ALPHA	 = 6
				VK_BLEND_FACTOR_DST_ALPHA,					// Rhi::Blend::DEST_ALPHA		 = 7
				VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,		// Rhi::Blend::INV_DEST_ALPHA	 = 8
				VK_BLEND_FACTOR_DST_COLOR,					// Rhi::Blend::DEST_COLOR		 = 9
				VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,		// Rhi::Blend::INV_DEST_COLOR	 = 10
				VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,			// Rhi::Blend::SRC_ALPHA_SAT	 = 11
				VK_BLEND_FACTOR_MAX_ENUM,					// <undefined>					 = 12 !
				VK_BLEND_FACTOR_MAX_ENUM,					// <undefined>					 = 13 !
				VK_BLEND_FACTOR_CONSTANT_COLOR,				// Rhi::Blend::BLEND_FACTOR		 = 14
				VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,	// Rhi::Blend::INV_BLEND_FACTOR  = 15
				VK_BLEND_FACTOR_SRC1_COLOR,					// Rhi::Blend::SRC_1_COLOR		 = 16
				VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,		// Rhi::Blend::INV_SRC_1_COLOR	 = 17
				VK_BLEND_FACTOR_SRC1_ALPHA,					// Rhi::Blend::SRC_1_ALPHA		 = 18
				VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA		// Rhi::Blend::INV_SRC_1_ALPHA	 = 19
			};
			return MAPPING[static_cast<int>(blend) - 1];	// Lookout! The "Rhi::Blend"-values start with 1, not 0, there are also holes
		}

		/**
		*  @brief
		*    "Rhi::BlendOp" to Vulkan blend operation
		*
		*  @param[in] blendOp
		*    "Rhi::BlendOp" to map
		*
		*  @return
		*    Vulkan blend operation
		*/
		[[nodiscard]] static VkBlendOp getVulkanBlendOp(Rhi::BlendOp blendOp)
		{
			static constexpr VkBlendOp MAPPING[] =
			{
				VK_BLEND_OP_ADD,				// Rhi::BlendOp::ADD
				VK_BLEND_OP_SUBTRACT,			// Rhi::BlendOp::SUBTRACT
				VK_BLEND_OP_REVERSE_SUBTRACT,	// Rhi::BlendOp::REV_SUBTRACT
				VK_BLEND_OP_MIN,				// Rhi::BlendOp::MIN
				VK_BLEND_OP_MAX					// Rhi::BlendOp::MAX
			};
			return MAPPING[static_cast<int>(blendOp) - 1];	// Lookout! The "Rhi::Blend"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::ComparisonFunc                                   ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::ComparisonFunc" to Vulkan comparison function
		*
		*  @param[in] comparisonFunc
		*    "Rhi::ComparisonFunc" to map
		*
		*  @return
		*    Vulkan comparison function
		*/
		[[nodiscard]] static VkCompareOp getVulkanComparisonFunc(Rhi::ComparisonFunc comparisonFunc)
		{
			static constexpr VkCompareOp MAPPING[] =
			{
				VK_COMPARE_OP_NEVER,			// Rhi::ComparisonFunc::NEVER
				VK_COMPARE_OP_LESS,				// Rhi::ComparisonFunc::LESS
				VK_COMPARE_OP_EQUAL,			// Rhi::ComparisonFunc::EQUAL
				VK_COMPARE_OP_LESS_OR_EQUAL,	// Rhi::ComparisonFunc::LESS_EQUAL
				VK_COMPARE_OP_GREATER,			// Rhi::ComparisonFunc::GREATER
				VK_COMPARE_OP_NOT_EQUAL,		// Rhi::ComparisonFunc::NOT_EQUAL
				VK_COMPARE_OP_GREATER_OR_EQUAL,	// Rhi::ComparisonFunc::GREATER_EQUAL
				VK_COMPARE_OP_ALWAYS			// Rhi::ComparisonFunc::ALWAYS
			};
			return MAPPING[static_cast<int>(comparisonFunc) - 1];	// Lookout! The "Rhi::ComparisonFunc"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Rhi::VertexAttributeFormat and semantic               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::VertexAttributeFormat" to Vulkan format
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    Vulkan format
		*/
		[[nodiscard]] static VkFormat getVulkanFormat(Rhi::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr VkFormat MAPPING[] =
			{
				VK_FORMAT_R32_SFLOAT,			// Rhi::VertexAttributeFormat::FLOAT_1
				VK_FORMAT_R32G32_SFLOAT,		// Rhi::VertexAttributeFormat::FLOAT_2
				VK_FORMAT_R32G32B32_SFLOAT,		// Rhi::VertexAttributeFormat::FLOAT_3
				VK_FORMAT_R32G32B32A32_SFLOAT,	// Rhi::VertexAttributeFormat::FLOAT_4
				VK_FORMAT_R8G8B8A8_UNORM,		// Rhi::VertexAttributeFormat::R8G8B8A8_UNORM
				VK_FORMAT_R8G8B8A8_UINT,		// Rhi::VertexAttributeFormat::R8G8B8A8_UINT
				VK_FORMAT_R16G16_SINT,			// Rhi::VertexAttributeFormat::SHORT_2
				VK_FORMAT_R16G16B16A16_SINT,	// Rhi::VertexAttributeFormat::SHORT_4
				VK_FORMAT_R32_UINT				// Rhi::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		//[-------------------------------------------------------]
		//[ Rhi::IndexBufferFormat                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::IndexBufferFormat" to Vulkan type
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] indexBufferFormat
		*    "Rhi::IndexBufferFormat" to map
		*
		*  @return
		*    Vulkan index type
		*/
		[[nodiscard]] static VkIndexType getVulkanType([[maybe_unused]] const Rhi::Context& context, Rhi::IndexBufferFormat::Enum indexBufferFormat)
		{
			RHI_ASSERT(context, Rhi::IndexBufferFormat::UNSIGNED_CHAR != indexBufferFormat, "One byte per element index buffer format isn't supported by Vulkan")
			static constexpr VkIndexType MAPPING[] =
			{
				VK_INDEX_TYPE_MAX_ENUM,	// Rhi::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Vulkan
				VK_INDEX_TYPE_UINT16,	// Rhi::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				VK_INDEX_TYPE_UINT32	// Rhi::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Rhi::PrimitiveTopology                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::PrimitiveTopology" to Vulkan type
		*
		*  @param[in] primitiveTopology
		*    "Rhi::PrimitiveTopology" to map
		*
		*  @return
		*    Vulkan type
		*/
		[[nodiscard]] static VkPrimitiveTopology getVulkanType(Rhi::PrimitiveTopology primitiveTopology)
		{
			// Tessellation support: Up to 32 vertices per patch are supported "Rhi::PrimitiveTopology::PATCH_LIST_1" ... "Rhi::PrimitiveTopology::PATCH_LIST_32"
			if (primitiveTopology >= Rhi::PrimitiveTopology::PATCH_LIST_1)
			{
				// Use tessellation
				return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}
			else
			{
				static constexpr VkPrimitiveTopology MAPPING[] =
				{
					VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		// Rhi::PrimitiveTopology::POINT_LIST
					VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		// Rhi::PrimitiveTopology::LINE_LIST
					VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		// Rhi::PrimitiveTopology::LINE_STRIP
					VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	// Rhi::PrimitiveTopology::TRIANGLE_LIST
					VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	// Rhi::PrimitiveTopology::TRIANGLE_STRIP
				};
				return MAPPING[static_cast<int>(primitiveTopology) - 1];	// Lookout! The "Rhi::PrimitiveTopology"-values start with 1, not 0
			}
		}

		//[-------------------------------------------------------]
		//[ Rhi::TextureFormat                                    ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::TextureFormat" to Vulkan format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    Vulkan format
		*/
		[[nodiscard]] static VkFormat getVulkanFormat(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr VkFormat MAPPING[] =
			{
				VK_FORMAT_R8_UNORM,					// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				VK_FORMAT_R8G8B8_UNORM,				// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				VK_FORMAT_R8G8B8A8_UNORM,			// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				VK_FORMAT_R8G8B8A8_SRGB,			// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_B8G8R8A8_UNORM,			// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				VK_FORMAT_B10G11R11_UFLOAT_PACK32,	// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				VK_FORMAT_R16G16B16A16_SFLOAT,		// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				VK_FORMAT_R32G32B32A32_SFLOAT,		// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				VK_FORMAT_BC1_RGB_UNORM_BLOCK,		// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				VK_FORMAT_BC1_RGB_SRGB_BLOCK,		// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_BC2_UNORM_BLOCK,			// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				VK_FORMAT_BC2_SRGB_BLOCK,			// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_BC3_UNORM_BLOCK,			// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				VK_FORMAT_BC3_SRGB_BLOCK,			// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				VK_FORMAT_BC4_UNORM_BLOCK,			// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				VK_FORMAT_BC5_UNORM_BLOCK,			// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				VK_FORMAT_UNDEFINED,				// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 11 - TODO(co) Check for Vulkan format
				VK_FORMAT_R16_UNORM,				// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				VK_FORMAT_R32_UINT,					// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				VK_FORMAT_R32_SFLOAT,				// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				VK_FORMAT_D32_SFLOAT,				// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				VK_FORMAT_R16G16_UNORM,				// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				VK_FORMAT_R16G16_SFLOAT,			// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				VK_FORMAT_UNDEFINED					// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		/**
		*  @brief
		*    Number of multisamples to Vulkan sample count flag bits
		*
		*  @param[in] context
		*    RHI context to use
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*
		*  @return
		*    Vulkan sample count flag bits
		*/
		[[nodiscard]] static VkSampleCountFlagBits getVulkanSampleCountFlagBits([[maybe_unused]] const Rhi::Context& context, uint8_t numberOfMultisamples)
		{
			RHI_ASSERT(context, numberOfMultisamples <= 8, "Invalid number of Vulkan multisamples")
			static constexpr VkSampleCountFlagBits MAPPING[] =
			{
				VK_SAMPLE_COUNT_1_BIT,
				VK_SAMPLE_COUNT_2_BIT,
				VK_SAMPLE_COUNT_4_BIT,
				VK_SAMPLE_COUNT_8_BIT
			};
			return MAPPING[numberOfMultisamples - 1];	// Lookout! The "numberOfMultisamples"-values start with 1, not 0
		}


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Helper.h                                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan helper
	*/
	class Helper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Command                                               ]
		//[-------------------------------------------------------]
		[[nodiscard]] static VkCommandBuffer beginSingleTimeCommands(const VulkanRhi& vulkanRhi)
		{
			// Create and begin Vulkan command buffer
			VkCommandBuffer vkCommandBuffer = vulkanRhi.getVulkanContext().createVkCommandBuffer();
			static constexpr VkCommandBufferBeginInfo vkCommandBufferBeginInfo =
			{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType (VkStructureType)
				nullptr,										// pNext (const void*)
				0,												// flags (VkCommandBufferUsageFlags)
				nullptr											// pInheritanceInfo (const VkCommandBufferInheritanceInfo*)
			};
			if (vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo) == VK_SUCCESS)
			{
				// Done
				return vkCommandBuffer;
			}
			else
			{
				// Error!
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to begin Vulkan command buffer instance")
				return VK_NULL_HANDLE;
			}
		}

		static void endSingleTimeCommands(const VulkanRhi& vulkanRhi, VkCommandBuffer vkCommandBuffer)
		{
			const VulkanContext& vulkanContext = vulkanRhi.getVulkanContext();
			const VkQueue vkQueue = vulkanContext.getGraphicsVkQueue();

			// End Vulkan command buffer
			vkEndCommandBuffer(vkCommandBuffer);

			// Submit Vulkan command buffer
			const VkSubmitInfo vkSubmitInfo =
			{
				VK_STRUCTURE_TYPE_SUBMIT_INFO,	// sType (VkStructureType)
				nullptr,						// pNext (const void*)
				0,								// waitSemaphoreCount (uint32_t)
				nullptr,						// pWaitSemaphores (const VkSemaphore*)
				nullptr,						// pWaitDstStageMask (const VkPipelineStageFlags*)
				1,								// commandBufferCount (uint32_t)
				&vkCommandBuffer,				// pCommandBuffers (const VkCommandBuffer*)
				0,								// signalSemaphoreCount (uint32_t)
				nullptr							// pSignalSemaphores (const VkSemaphore*)
			};
			if (vkQueueSubmit(vkQueue, 1, &vkSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			{
				// Error!
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Vulkan queue submit failed")
				return;
			}
			if (vkQueueWaitIdle(vkQueue) != VK_SUCCESS)
			{
				// Error!
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Vulkan Queue wait idle failed")
				return;
			}

			// Destroy Vulkan command buffer
			vulkanContext.destroyVkCommandBuffer(vkCommandBuffer);
		}

		//[-------------------------------------------------------]
		//[ Transition                                            ]
		//[-------------------------------------------------------]
		static void transitionVkImageLayout(const VulkanRhi& vulkanRhi, VkImage vkImage, VkImageAspectFlags vkImageAspectFlags, VkImageLayout oldVkImageLayout, VkImageLayout newVkImageLayout)
		{
			// Create and begin Vulkan command buffer
			VkCommandBuffer vkCommandBuffer = beginSingleTimeCommands(vulkanRhi);

			// Vulkan image memory barrier
			transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, vkImageAspectFlags, 1, 1, oldVkImageLayout, newVkImageLayout);

			// End and destroy Vulkan command buffer
			endSingleTimeCommands(vulkanRhi, vkCommandBuffer);
		}

		static void transitionVkImageLayout(const VulkanRhi& vulkanRhi, VkCommandBuffer vkCommandBuffer, VkImage vkImage, VkImageAspectFlags vkImageAspectFlags, uint32_t levelCount, uint32_t layerCount, VkImageLayout oldVkImageLayout, VkImageLayout newVkImageLayout)
		{
			VkImageMemoryBarrier vkImageMemoryBarrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// sType (VkStructureType)
				nullptr,								// pNext (const void*)
				0,										// srcAccessMask (VkAccessFlags)
				0,										// dstAccessMask (VkAccessFlags)
				oldVkImageLayout,						// oldLayout (VkImageLayout)
				newVkImageLayout,						// newLayout (VkImageLayout)
				VK_QUEUE_FAMILY_IGNORED,				// srcQueueFamilyIndex (uint32_t)
				VK_QUEUE_FAMILY_IGNORED,				// dstQueueFamilyIndex (uint32_t)
				vkImage,								// image (VkImage)
				{ // subresourceRange (VkImageSubresourceRange)
					vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
					0,					// baseMipLevel (uint32_t)
					levelCount,			// levelCount (uint32_t)
					0,					// baseArrayLayer (uint32_t)
					layerCount			// layerCount (uint32_t)
				}
			};

			// "srcAccessMask" and "dstAccessMask" configuration
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			if (VK_IMAGE_LAYOUT_PREINITIALIZED == oldVkImageLayout && VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = 0;
				vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == oldVkImageLayout && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = 0;
				vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == oldVkImageLayout && VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				vkImageMemoryBarrier.dstAccessMask = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_HOST_BIT;
			}
			else if (VK_IMAGE_LAYOUT_UNDEFINED == oldVkImageLayout && VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL == newVkImageLayout)
			{
				vkImageMemoryBarrier.srcAccessMask = 0;
				vkImageMemoryBarrier.dstAccessMask = (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Unsupported Vulkan image layout transition")
			}

			// Create Vulkan pipeline barrier command
			vkCmdPipelineBarrier(vkCommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &vkImageMemoryBarrier);
		}

		static void transitionVkImageLayout(const VulkanRhi& vulkanRhi, VkCommandBuffer vkCommandBuffer, VkImage vkImage, VkImageLayout oldVkImageLayout, VkImageLayout newVkImageLayout, VkImageSubresourceRange vkImageSubresourceRange, VkPipelineStageFlags sourceVkPipelineStageFlags, VkPipelineStageFlags destinationVkPipelineStageFlags)
		{
			// Basing on https://github.com/SaschaWillems/Vulkan/tree/master

			// Create an image barrier object
			VkImageMemoryBarrier vkImageMemoryBarrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,	// sType (VkStructureType)
				nullptr,								// pNext (const void*)
				0,										// srcAccessMask (VkAccessFlags)
				0,										// dstAccessMask (VkAccessFlags)
				oldVkImageLayout,						// oldLayout (VkImageLayout)
				newVkImageLayout,						// newLayout (VkImageLayout)
				VK_QUEUE_FAMILY_IGNORED,				// srcQueueFamilyIndex (uint32_t)
				VK_QUEUE_FAMILY_IGNORED,				// dstQueueFamilyIndex (uint32_t)
				vkImage,								// image (VkImage)
				vkImageSubresourceRange					// subresourceRange (VkImageSubresourceRange)
			};

			// Source layouts (old)
			// -> Source access mask controls actions that have to be finished on the old layout before it will be transitioned to the new layout
			switch (oldVkImageLayout)
			{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					vkImageMemoryBarrier.srcAccessMask = 0;
					break;

				case VK_IMAGE_LAYOUT_PREINITIALIZED:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image is a transfer source 
					// Make sure any reads from the image have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					vkImageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_GENERAL:
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
				case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
				case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
				// case VK_IMAGE_LAYOUT_BEGIN_RANGE:	not possible
				// case VK_IMAGE_LAYOUT_END_RANGE:		not possible
				case VK_IMAGE_LAYOUT_RANGE_SIZE:
				case VK_IMAGE_LAYOUT_MAX_ENUM:
				default:
					// Other source layouts aren't handled (yet)
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Unsupported Vulkan image old layout transition")
					break;
			}

			// Target layouts (new)
			// -> Destination access mask controls the dependency for the new image layout
			switch (newVkImageLayout)
			{
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image will be used as a transfer destination
					// Make sure any writes to the image have been finished
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image will be used as a transfer source
					// Make sure any reads from the image have been finished
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image will be used as a color attachment
					// Make sure any writes to the color buffer have been finished
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image layout will be used as a depth/stencil attachment
					// Make sure any writes to depth/stencil buffer have been finished
					vkImageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image will be read in a shader (sampler, input attachment)
					// Make sure any writes to the image have been finished
					if (vkImageMemoryBarrier.srcAccessMask == 0)
					{
						vkImageMemoryBarrier.srcAccessMask = (VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
					}
					vkImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_UNDEFINED:
				case VK_IMAGE_LAYOUT_GENERAL:
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				case VK_IMAGE_LAYOUT_PREINITIALIZED:
				case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
				case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
				case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
				// case VK_IMAGE_LAYOUT_BEGIN_RANGE:	not possible
				// case VK_IMAGE_LAYOUT_END_RANGE:		not possible
				case VK_IMAGE_LAYOUT_RANGE_SIZE:
				case VK_IMAGE_LAYOUT_MAX_ENUM:
				default:
					// Other source layouts aren't handled (yet)
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Unsupported Vulkan image new layout transition")
					break;
			}

			// Put barrier inside setup command buffer
			// -> "Table 4. Supported access types": https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#synchronization-access-types-supported
			vkCmdPipelineBarrier(vkCommandBuffer, sourceVkPipelineStageFlags, destinationVkPipelineStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vkImageMemoryBarrier);
		}

		//[-------------------------------------------------------]
		//[ Buffer                                                ]
		//[-------------------------------------------------------]
		// TODO(co) Trivial implementation to have something to start with. Need to use more clever memory management and stating buffers later on.
		static void createAndAllocateVkBuffer(const VulkanRhi& vulkanRhi, VkBufferUsageFlagBits vkBufferUsageFlagBits, VkMemoryPropertyFlags vkMemoryPropertyFlags, VkDeviceSize numberOfBytes, const void* data, VkBuffer& vkBuffer, VkDeviceMemory& vkDeviceMemory)
		{
			const VulkanContext& vulkanContext = vulkanRhi.getVulkanContext();
			const VkDevice vkDevice = vulkanContext.getVkDevice();

			// Create the Vulkan buffer
			const VkBufferCreateInfo vkBufferCreateInfo =
			{
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,					// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkBufferCreateFlags)
				numberOfBytes,											// size (VkDeviceSize)
				static_cast<VkBufferUsageFlags>(vkBufferUsageFlagBits),	// usage (VkBufferUsageFlags)
				VK_SHARING_MODE_EXCLUSIVE,								// sharingMode (VkSharingMode)
				0,														// queueFamilyIndexCount (uint32_t)
				nullptr													// pQueueFamilyIndices (const uint32_t*)
			};
			if (vkCreateBuffer(vkDevice, &vkBufferCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &vkBuffer) != VK_SUCCESS)
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan buffer")
			}

			// Allocate memory for the Vulkan buffer
			VkMemoryRequirements vkMemoryRequirements = {};
			vkGetBufferMemoryRequirements(vkDevice, vkBuffer, &vkMemoryRequirements);
			const VkMemoryAllocateInfo vkMemoryAllocateInfo =
			{
				VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,															// sType (VkStructureType)
				nullptr,																						// pNext (const void*)
				vkMemoryRequirements.size,																		// allocationSize (VkDeviceSize)
				vulkanContext.findMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkMemoryPropertyFlags)	// memoryTypeIndex (uint32_t)
			};
			if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, vulkanRhi.getVkAllocationCallbacks(), &vkDeviceMemory) != VK_SUCCESS)
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to allocate the Vulkan buffer memory")
			}

			// Bind and fill memory
			vkBindBufferMemory(vkDevice, vkBuffer, vkDeviceMemory, 0);
			if (nullptr != data)
			{
				void* mappedData = nullptr;
				if (vkMapMemory(vkDevice, vkDeviceMemory, 0, vkBufferCreateInfo.size, 0, &mappedData) == VK_SUCCESS)
				{
					memcpy(mappedData, data, static_cast<size_t>(vkBufferCreateInfo.size));
					vkUnmapMemory(vkDevice, vkDeviceMemory);
				}
				else
				{
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to map the Vulkan memory")
				}
			}
		}

		static void destroyAndFreeVkBuffer(const VulkanRhi& vulkanRhi, VkBuffer& vkBuffer, VkDeviceMemory& vkDeviceMemory)
		{
			if (VK_NULL_HANDLE != vkBuffer)
			{
				const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
				vkDestroyBuffer(vkDevice, vkBuffer, vulkanRhi.getVkAllocationCallbacks());
				if (VK_NULL_HANDLE != vkDeviceMemory)
				{
					vkFreeMemory(vkDevice, vkDeviceMemory, vulkanRhi.getVkAllocationCallbacks());
				}
			}
		}

		//[-------------------------------------------------------]
		//[ Image                                                 ]
		//[-------------------------------------------------------]
		[[nodiscard]] static VkImageLayout getVkImageLayoutByTextureFlags(uint32_t textureFlags)
		{
			if (textureFlags & Rhi::TextureFlag::RENDER_TARGET)
			{
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				return VK_IMAGE_LAYOUT_GENERAL;
			}
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		// TODO(co) Trivial implementation to have something to start with. Need to use more clever memory management and stating buffers later on.
		static VkFormat createAndFillVkImage(const VulkanRhi& vulkanRhi, VkImageType vkImageType, VkImageViewType vkImageViewType, const VkExtent3D& vkExtent3D, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, VkImageView& vkImageView)
		{
			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(vkExtent3D.width, vkExtent3D.height) : 1;

			// Get Vulkan image usage flags
			RHI_ASSERT(vulkanRhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Vulkan render target textures can't be filled using provided data")
			const bool isDepthTextureFormat = Rhi::TextureFormat::isDepth(textureFormat);
			VkImageUsageFlags vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				vkImageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				vkImageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
			}
			if (textureFlags & Rhi::TextureFlag::RENDER_TARGET)
			{
				if (isDepthTextureFormat)
				{
					vkImageUsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				}
				else
				{
					vkImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				}
			}
			if (generateMipmaps)
			{
				vkImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			}

			// Get Vulkan format
			const VkFormat vkFormat   = Mapping::getVulkanFormat(textureFormat);
			const bool     layered    = (VK_IMAGE_VIEW_TYPE_2D_ARRAY == vkImageViewType || VK_IMAGE_VIEW_TYPE_CUBE == vkImageViewType);
			const uint32_t layerCount = layered ? vkExtent3D.depth : 1;
			const uint32_t depth	  = layered ? 1 : vkExtent3D.depth;
			const VkSampleCountFlagBits vkSampleCountFlagBits = Mapping::getVulkanSampleCountFlagBits(vulkanRhi.getContext(), numberOfMultisamples);
			VkImageAspectFlags vkImageAspectFlags = isDepthTextureFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			if (::detail::hasVkFormatStencilComponent(vkFormat))
			{
				vkImageAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			// Calculate the number of bytes
			uint32_t numberOfBytes = 0;
			if (dataContainsMipmaps)
			{
				uint32_t currentWidth  = vkExtent3D.width;
				uint32_t currentHeight = vkExtent3D.height;
				uint32_t currentDepth  = depth;
				for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
				{
					numberOfBytes += Rhi::TextureFormat::getNumberOfBytesPerSlice(static_cast<Rhi::TextureFormat::Enum>(textureFormat), currentWidth, currentHeight) * currentDepth;
					currentWidth = Rhi::ITexture::getHalfSize(currentWidth);
					currentHeight = Rhi::ITexture::getHalfSize(currentHeight);
					currentDepth = Rhi::ITexture::getHalfSize(currentDepth);
				}
				numberOfBytes *= vkExtent3D.depth;
			}
			else
			{
				numberOfBytes = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, vkExtent3D.width, vkExtent3D.height) * vkExtent3D.depth;
			}

			{ // Create and fill Vulkan image
				const VkImageCreateFlags vkImageCreateFlags = (VK_IMAGE_VIEW_TYPE_CUBE == vkImageViewType) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u;
				createAndAllocateVkImage(vulkanRhi, vkImageCreateFlags, vkImageType, VkExtent3D{vkExtent3D.width, vkExtent3D.height, depth}, numberOfMipmaps, layerCount, vkFormat, vkSampleCountFlagBits, VK_IMAGE_TILING_OPTIMAL, vkImageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkDeviceMemory);
			}

			// Create the Vulkan image view
			if ((textureFlags & Rhi::TextureFlag::SHADER_RESOURCE) != 0 || (textureFlags & Rhi::TextureFlag::RENDER_TARGET) != 0 || (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS) != 0)
			{
				createVkImageView(vulkanRhi, vkImage, vkImageViewType, numberOfMipmaps, layerCount, vkFormat, vkImageAspectFlags, vkImageView);
			}

			// Upload all mipmaps
			if (nullptr != data)
			{
				// Create Vulkan staging buffer
				VkBuffer stagingVkBuffer = VK_NULL_HANDLE;
				VkDeviceMemory stagingVkDeviceMemory = VK_NULL_HANDLE;
				createAndAllocateVkBuffer(vulkanRhi, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, stagingVkBuffer, stagingVkDeviceMemory);

				{ // Upload all mipmaps
					const uint32_t numberOfUploadedMipmaps = generateMipmaps ? 1 : numberOfMipmaps;

					// Create and begin Vulkan command buffer
					VkCommandBuffer vkCommandBuffer = beginSingleTimeCommands(vulkanRhi);
					transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, vkImageAspectFlags, numberOfUploadedMipmaps, layerCount, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

					// Upload all mipmaps
					uint32_t bufferOffset  = 0;
					uint32_t currentWidth  = vkExtent3D.width;
					uint32_t currentHeight = vkExtent3D.height;
					uint32_t currentDepth  = depth;

					// Allocate list of VkBufferImageCopy and setup VkBufferImageCopy data for each mipmap level
					std::vector<VkBufferImageCopy> vkBufferImageCopyList;
					vkBufferImageCopyList.reserve(numberOfUploadedMipmaps);
					for (uint32_t mipmap = 0; mipmap < numberOfUploadedMipmaps; ++mipmap)
					{
						vkBufferImageCopyList.push_back({
							bufferOffset,									// bufferOffset (VkDeviceSize)
							0,												// bufferRowLength (uint32_t)
							0,												// bufferImageHeight (uint32_t)
							{ // imageSubresource (VkImageSubresourceLayers)
								vkImageAspectFlags,							// aspectMask (VkImageAspectFlags)
								mipmap,										// mipLevel (uint32_t)
								0,											// baseArrayLayer (uint32_t)
								layerCount									// layerCount (uint32_t)
							},
							{ 0, 0, 0 },									// imageOffset (VkOffset3D)
							{ currentWidth, currentHeight, currentDepth }	// imageExtent (VkExtent3D)
						});

						// Move on to the next mipmap
						bufferOffset += Rhi::TextureFormat::getNumberOfBytesPerSlice(static_cast<Rhi::TextureFormat::Enum>(textureFormat), currentWidth, currentHeight) * currentDepth;
						currentWidth = Rhi::ITexture::getHalfSize(currentWidth);
						currentHeight = Rhi::ITexture::getHalfSize(currentHeight);
						currentDepth = Rhi::ITexture::getHalfSize(currentDepth);
					}

					// Copy Vulkan buffer to Vulkan image
					vkCmdCopyBufferToImage(vkCommandBuffer, stagingVkBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(vkBufferImageCopyList.size()), vkBufferImageCopyList.data());

					// End and destroy Vulkan command buffer
					if (generateMipmaps)
					{
						const VkImageSubresourceRange vkImageSubresourceRange =
						{
							vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
							0,					// baseMipLevel (uint32_t)
							1,					// levelCount (uint32_t)
							0,					// baseArrayLayer (uint32_t)
							layerCount			// layerCount (uint32_t)
						};
						transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
					}
					else
					{
						transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, vkImageAspectFlags, numberOfUploadedMipmaps, layerCount, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}
					endSingleTimeCommands(vulkanRhi, vkCommandBuffer);
				}

				// Destroy Vulkan staging buffer
				destroyAndFreeVkBuffer(vulkanRhi, stagingVkBuffer, stagingVkDeviceMemory);

				// Generate a complete texture mip-chain at runtime from a base image using image blits and proper image barriers
				// -> Basing on https://github.com/SaschaWillems/Vulkan/tree/master/examples/texturemipmapgen and "Mipmap generation : Transfers, transition layout" by Antoine MORRIER published January 12, 2017 at http://cpp-rendering.io/mipmap-generation/
				// -> We copy down the whole mip chain doing a blit from mip-1 to mip. An alternative way would be to always blit from the first mip level and sample that one down.
				// TODO(co) Some GPUs also offer "asynchronous transfer queues" (check for queue families with only the "VK_QUEUE_TRANSFER_BIT" set) that may be used to speed up such operations
				if (generateMipmaps)
				{
					#ifdef RHI_DEBUG
					{
						// Get device properties for the requested Vulkan texture format
						VkFormatProperties vkFormatProperties;
						vkGetPhysicalDeviceFormatProperties(vulkanRhi.getVulkanContext().getVkPhysicalDevice(), vkFormat, &vkFormatProperties);

						// Mip-chain generation requires support for blit source and destination
						RHI_ASSERT(vulkanRhi.getContext(), vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT, "Invalid Vulkan optimal tiling features")
						RHI_ASSERT(vulkanRhi.getContext(), vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT, "Invalid Vulkan optimal tiling features")
					}
					#endif

					// Create and begin Vulkan command buffer
					VkCommandBuffer vkCommandBuffer = beginSingleTimeCommands(vulkanRhi);

					// Copy down mips from n-1 to n
					for (uint32_t i = 1; i < numberOfMipmaps; ++i)
					{
						const VkImageBlit VkImageBlit =
						{
							{ // srcSubresource (VkImageSubresourceLayers)
								vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
								i - 1,				// mipLevel (uint32_t)
								0,					// baseArrayLayer (uint32_t)
								layerCount			// layerCount (uint32_t)
							},
							{ // srcOffsets[2] (VkOffset3D)
								{ 0, 0, 0 },
								{ std::max(int32_t(vkExtent3D.width >> (i - 1)), 1), std::max(int32_t(vkExtent3D.height >> (i - 1)), 1), 1 }
							},
							{ // dstSubresource (VkImageSubresourceLayers)
								vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
								i,					// mipLevel (uint32_t)
								0,					// baseArrayLayer (uint32_t)
								layerCount			// layerCount (uint32_t)
							},
							{ // dstOffsets[2] (VkOffset3D)
								{ 0, 0, 0 },
								{ std::max(int32_t(vkExtent3D.width >> i), 1), std::max(int32_t(vkExtent3D.height >> i), 1), 1 }
							}
						};
						const VkImageSubresourceRange vkImageSubresourceRange =
						{
							vkImageAspectFlags,	// aspectMask (VkImageAspectFlags)
							i,					// baseMipLevel (uint32_t)
							1,					// levelCount (uint32_t)
							0,					// baseArrayLayer (uint32_t)
							layerCount			// layerCount (uint32_t)
						};

						// Transition current mip level to transfer destination
						transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

						// Blit from previous level
						vkCmdBlitImage(vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &VkImageBlit, VK_FILTER_LINEAR);

						// Transition current mip level to transfer source for read in next iteration
						transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
					}

					{ // After the loop, all mip layers are in "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL"-layout, so transition all to "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL"-layout
						const VkImageSubresourceRange vkImageSubresourceRange =
						{
							vkImageAspectFlags,		// aspectMask (VkImageAspectFlags)
							0,						// baseMipLevel (uint32_t)
							numberOfMipmaps,		// levelCount (uint32_t)
							0,						// baseArrayLayer (uint32_t)
							layerCount				// layerCount (uint32_t)
						};
						transitionVkImageLayout(vulkanRhi, vkCommandBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkImageSubresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
					}

					// End and destroy Vulkan command buffer
					endSingleTimeCommands(vulkanRhi, vkCommandBuffer);
				}
			}

			// Done
			return vkFormat;
		}

		static void createAndAllocateVkImage(const VulkanRhi& vulkanRhi, VkImageCreateFlags vkImageCreateFlags, VkImageType vkImageType, const VkExtent3D& vkExtent3D, uint32_t mipLevels, uint32_t arrayLayers, VkFormat vkFormat, VkSampleCountFlagBits vkSampleCountFlagBits, VkImageTiling vkImageTiling, VkImageUsageFlags vkImageUsageFlags, VkMemoryPropertyFlags vkMemoryPropertyFlags, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory)
		{
			const VulkanContext& vulkanContext = vulkanRhi.getVulkanContext();
			const VkDevice vkDevice = vulkanContext.getVkDevice();

			{ // Create Vulkan image
				const VkImageCreateInfo vkImageCreateInfo =
				{
					VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// sType (VkStructureType)
					nullptr,								// pNext (const void*)
					vkImageCreateFlags,						// flags (VkImageCreateFlags)
					vkImageType,							// imageType (VkImageType)
					vkFormat,								// format (VkFormat)
					vkExtent3D,								// extent (VkExtent3D)
					mipLevels,								// mipLevels (uint32_t)
					arrayLayers,							// arrayLayers (uint32_t)
					vkSampleCountFlagBits,					// samples (VkSampleCountFlagBits)
					vkImageTiling,							// tiling (VkImageTiling)
					vkImageUsageFlags,						// usage (VkImageUsageFlags)
					VK_SHARING_MODE_EXCLUSIVE,				// sharingMode (VkSharingMode)
					0,										// queueFamilyIndexCount (uint32_t)
					nullptr,								// pQueueFamilyIndices (const uint32_t*)
					VK_IMAGE_LAYOUT_PREINITIALIZED			// initialLayout (VkImageLayout)
				};
				if (vkCreateImage(vkDevice, &vkImageCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &vkImage) != VK_SUCCESS)
				{
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan image")
				}
			}

			{ // Allocate Vulkan memory
				VkMemoryRequirements vkMemoryRequirements = {};
				vkGetImageMemoryRequirements(vkDevice, vkImage, &vkMemoryRequirements);
				const VkMemoryAllocateInfo vkMemoryAllocateInfo =
				{
					VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,															// sType (VkStructureType)
					nullptr,																						// pNext (const void*)
					vkMemoryRequirements.size,																		// allocationSize (VkDeviceSize)
					vulkanContext.findMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkMemoryPropertyFlags)	// memoryTypeIndex (uint32_t)
				};
				if (vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, vulkanRhi.getVkAllocationCallbacks(), &vkDeviceMemory) != VK_SUCCESS)
				{
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to allocate the Vulkan memory")
				}
				if (vkBindImageMemory(vkDevice, vkImage, vkDeviceMemory, 0) != VK_SUCCESS)
				{
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to bind the Vulkan image memory")
				}
			}
		}

		static void destroyAndFreeVkImage(const VulkanRhi& vulkanRhi, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory)
		{
			if (VK_NULL_HANDLE != vkImage)
			{
				const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
				vkDestroyImage(vkDevice, vkImage, vulkanRhi.getVkAllocationCallbacks());
				vkImage = VK_NULL_HANDLE;
				if (VK_NULL_HANDLE != vkDeviceMemory)
				{
					vkFreeMemory(vkDevice, vkDeviceMemory, vulkanRhi.getVkAllocationCallbacks());
					vkDeviceMemory = VK_NULL_HANDLE;
				}
			}
		}

		static void destroyAndFreeVkImage(const VulkanRhi& vulkanRhi, VkImage& vkImage, VkDeviceMemory& vkDeviceMemory, VkImageView& vkImageView)
		{
			if (VK_NULL_HANDLE != vkImageView)
			{
				vkDestroyImageView(vulkanRhi.getVulkanContext().getVkDevice(), vkImageView, vulkanRhi.getVkAllocationCallbacks());
				vkImageView = VK_NULL_HANDLE;
			}
			destroyAndFreeVkImage(vulkanRhi, vkImage, vkDeviceMemory);
		}

		static void createVkImageView(const VulkanRhi& vulkanRhi, VkImage vkImage, VkImageViewType vkImageViewType, uint32_t levelCount, uint32_t layerCount, VkFormat vkFormat, VkImageAspectFlags vkImageAspectFlags, VkImageView& vkImageView)
		{
			const VkImageViewCreateInfo vkImageViewCreateInfo =
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				0,											// flags (VkImageViewCreateFlags)
				vkImage,									// image (VkImage)
				vkImageViewType,							// viewType (VkImageViewType)
				vkFormat,									// format (VkFormat)
				{ // components (VkComponentMapping)
					VK_COMPONENT_SWIZZLE_IDENTITY,			// r (VkComponentSwizzle)
					VK_COMPONENT_SWIZZLE_IDENTITY,			// g (VkComponentSwizzle)
					VK_COMPONENT_SWIZZLE_IDENTITY,			// b (VkComponentSwizzle)
					VK_COMPONENT_SWIZZLE_IDENTITY			// a (VkComponentSwizzle)
				},
				{ // subresourceRange (VkImageSubresourceRange)
					vkImageAspectFlags,						// aspectMask (VkImageAspectFlags)
					0,										// baseMipLevel (uint32_t)
					levelCount,								// levelCount (uint32_t)
					0,										// baseArrayLayer (uint32_t)
					layerCount								// layerCount (uint32_t)
				}
			};
			if (vkCreateImageView(vulkanRhi.getVulkanContext().getVkDevice(), &vkImageViewCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &vkImageView) != VK_SUCCESS)
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create Vulkan image view")
			}
		}

		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RHI_DEBUG
			static void setDebugObjectName(VkDevice vkDevice, VkDebugReportObjectTypeEXT vkDebugReportObjectTypeEXT, uint64_t object, const char* objectName)
			{
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					const VkDebugMarkerObjectNameInfoEXT vkDebugMarkerObjectNameInfoEXT =
					{
						VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,	// sType (VkStructureType)
						nullptr,												// pNext (const void*)
						vkDebugReportObjectTypeEXT,								// objectType (VkDebugReportObjectTypeEXT)
						object,													// object (uint64_t)
						objectName												// pObjectName (const char*)
					};
					vkDebugMarkerSetObjectNameEXT(vkDevice, &vkDebugMarkerObjectNameInfoEXT);
				}
			}
		#endif


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/RootSignature.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(VulkanRhi& vulkanRhi, const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IRootSignature(vulkanRhi),
			mRootSignature(rootSignature),
			mVkPipelineLayout(VK_NULL_HANDLE),
			mVkDescriptorPool(VK_NULL_HANDLE)
		{
			static constexpr uint32_t maxSets = 4242;	// TODO(co) We probably need to get this provided from the outside

			// Copy the parameter data
			const Rhi::Context& context = vulkanRhi.getContext();
			const uint32_t numberOfRootParameters = mRootSignature.numberOfParameters;
			if (numberOfRootParameters > 0)
			{
				mRootSignature.parameters = RHI_MALLOC_TYPED(context, Rhi::RootParameter, numberOfRootParameters);
				Rhi::RootParameter* destinationRootParameters = const_cast<Rhi::RootParameter*>(mRootSignature.parameters);
				memcpy(destinationRootParameters, rootSignature.parameters, sizeof(Rhi::RootParameter) * numberOfRootParameters);

				// Copy the descriptor table data
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					Rhi::RootParameter& destinationRootParameter = destinationRootParameters[rootParameterIndex];
					const Rhi::RootParameter& sourceRootParameter = rootSignature.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == destinationRootParameter.parameterType)
					{
						const uint32_t numberOfDescriptorRanges = destinationRootParameter.descriptorTable.numberOfDescriptorRanges;
						destinationRootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(RHI_MALLOC_TYPED(context, Rhi::DescriptorRange, numberOfDescriptorRanges));
						memcpy(reinterpret_cast<Rhi::DescriptorRange*>(destinationRootParameter.descriptorTable.descriptorRanges), reinterpret_cast<const Rhi::DescriptorRange*>(sourceRootParameter.descriptorTable.descriptorRanges), sizeof(Rhi::DescriptorRange) * numberOfDescriptorRanges);
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

			// Create the Vulkan descriptor set layout
			const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
			VkDescriptorSetLayouts vkDescriptorSetLayouts;
			uint32_t numberOfUniformTexelBuffers = 0;	// "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER"
			uint32_t numberOfStorageTexelBuffers = 0;	// "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER"
			uint32_t numberOfStorageImage = 0;			// "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE"
			uint32_t numberOfStorageBuffers = 0;		// "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER"
			uint32_t numberOfUniformBuffers = 0;		// "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER"
			uint32_t numberOfCombinedImageSamplers = 0;	// "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"
			if (numberOfRootParameters > 0)
			{
				// Fill the Vulkan descriptor set layout bindings
				vkDescriptorSetLayouts.reserve(numberOfRootParameters);
				mVkDescriptorSetLayouts.resize(numberOfRootParameters);
				std::fill(mVkDescriptorSetLayouts.begin(), mVkDescriptorSetLayouts.end(), static_cast<VkDescriptorSetLayout>(VK_NULL_HANDLE));	// TODO(co) Get rid of this
				typedef std::vector<VkDescriptorSetLayoutBinding> VkDescriptorSetLayoutBindings;
				VkDescriptorSetLayoutBindings vkDescriptorSetLayoutBindings;
				vkDescriptorSetLayoutBindings.reserve(numberOfRootParameters);
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < numberOfRootParameters; ++rootParameterIndex)
				{
					vkDescriptorSetLayoutBindings.clear();

					// TODO(co) For now we only support descriptor tables
					const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						// Process descriptor ranges
						const Rhi::DescriptorRange* descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges);
						for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < rootParameter.descriptorTable.numberOfDescriptorRanges; ++descriptorRangeIndex, ++descriptorRange)
						{
							// Evaluate parameter type
							VkDescriptorType vkDescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
							switch (descriptorRange->resourceType)
							{
								case Rhi::ResourceType::TEXTURE_BUFFER:
									RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::SRV == descriptorRange->rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan RHI implementation: Invalid descriptor range type")
									if (Rhi::DescriptorRangeType::SRV == descriptorRange->rangeType)
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
										++numberOfUniformTexelBuffers;
									}
									else
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
										++numberOfStorageTexelBuffers;
									}
									break;

								case Rhi::ResourceType::INDEX_BUFFER:
								case Rhi::ResourceType::VERTEX_BUFFER:
								case Rhi::ResourceType::STRUCTURED_BUFFER:
								case Rhi::ResourceType::INDIRECT_BUFFER:
									RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::SRV == descriptorRange->rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan RHI implementation: Invalid descriptor range type")
									vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
									++numberOfStorageBuffers;
									break;

								case Rhi::ResourceType::UNIFORM_BUFFER:
									RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::UBV == descriptorRange->rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan RHI implementation: Invalid descriptor range type")
									vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
									++numberOfUniformBuffers;
									break;

								case Rhi::ResourceType::TEXTURE_1D:
								case Rhi::ResourceType::TEXTURE_1D_ARRAY:
								case Rhi::ResourceType::TEXTURE_2D:
								case Rhi::ResourceType::TEXTURE_2D_ARRAY:
								case Rhi::ResourceType::TEXTURE_3D:
								case Rhi::ResourceType::TEXTURE_CUBE:
									RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::SRV == descriptorRange->rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange->rangeType, "Vulkan RHI implementation: Invalid descriptor range type")
									if (Rhi::DescriptorRangeType::SRV == descriptorRange->rangeType)
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
										++numberOfCombinedImageSamplers;
									}
									else
									{
										vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
										++numberOfStorageImage;
									}
									break;

								case Rhi::ResourceType::SAMPLER_STATE:
									// Nothing here due to usage of "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"
									RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::SAMPLER == descriptorRange->rangeType, "Vulkan RHI implementation: Invalid descriptor range type")
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
								case Rhi::ResourceType::COMPUTE_SHADER:
									RHI_ASSERT(vulkanRhi.getContext(), false, "Vulkan RHI implementation: Invalid resource type")
									break;
							}

							// Evaluate shader visibility
							VkShaderStageFlags vkShaderStageFlags = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
							switch (descriptorRange->shaderVisibility)
							{
								case Rhi::ShaderVisibility::ALL:
									vkShaderStageFlags = VK_SHADER_STAGE_ALL;
									break;

								case Rhi::ShaderVisibility::VERTEX:
									vkShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT;
									break;

								case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
									vkShaderStageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
									break;

								case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
									vkShaderStageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
									break;

								case Rhi::ShaderVisibility::GEOMETRY:
									vkShaderStageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
									break;

								case Rhi::ShaderVisibility::FRAGMENT:
									vkShaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
									break;

								case Rhi::ShaderVisibility::COMPUTE:
									vkShaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
									break;

								case Rhi::ShaderVisibility::ALL_GRAPHICS:
									vkShaderStageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
									break;
							}

							// Add the Vulkan descriptor set layout binding
							if (VK_DESCRIPTOR_TYPE_MAX_ENUM != vkDescriptorType)
							{
								const VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding =
								{
									descriptorRangeIndex,	// binding (uint32_t)
									vkDescriptorType,		// descriptorType (VkDescriptorType)
									1,						// descriptorCount (uint32_t)
									vkShaderStageFlags,		// stageFlags (VkShaderStageFlags)
									nullptr					// pImmutableSamplers (const VkSampler*)
								};
								vkDescriptorSetLayoutBindings.push_back(vkDescriptorSetLayoutBinding);
							}
						}
					}

					// Create the Vulkan descriptor set layout
					if (!vkDescriptorSetLayoutBindings.empty())
					{
						const VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo =
						{
							VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,			// sType (VkStructureType)
							nullptr,														// pNext (const void*)
							0,																// flags (VkDescriptorSetLayoutCreateFlags)
							static_cast<uint32_t>(vkDescriptorSetLayoutBindings.size()),	// bindingCount (uint32_t)
							vkDescriptorSetLayoutBindings.data()							// pBindings (const VkDescriptorSetLayoutBinding*)
						};
						if (vkCreateDescriptorSetLayout(vkDevice, &vkDescriptorSetLayoutCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkDescriptorSetLayouts[rootParameterIndex]) != VK_SUCCESS)
						{
							RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan descriptor set layout")
						}
						vkDescriptorSetLayouts.push_back(mVkDescriptorSetLayouts[rootParameterIndex]);
					}
				}
			}

			{ // Create the Vulkan pipeline layout
				const VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo =
				{
					VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,								// sType (VkStructureType)
					nullptr,																	// pNext (const void*)
					0,																			// flags (VkPipelineLayoutCreateFlags)
					static_cast<uint32_t>(vkDescriptorSetLayouts.size()),						// setLayoutCount (uint32_t)
					vkDescriptorSetLayouts.empty() ? nullptr : vkDescriptorSetLayouts.data(),	// pSetLayouts (const VkDescriptorSetLayout*)
					0,																			// pushConstantRangeCount (uint32_t)
					nullptr																		// pPushConstantRanges (const VkPushConstantRange*)
				};
				if (vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkPipelineLayout) != VK_SUCCESS)
				{
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan pipeline layout")
				}
			}

			{ // Create the Vulkan descriptor pool
				typedef std::array<VkDescriptorPoolSize, 3> VkDescriptorPoolSizes;
				VkDescriptorPoolSizes vkDescriptorPoolSizes;
				uint32_t numberOfVkDescriptorPoolSizes = 0;

				// "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"
				if (numberOfCombinedImageSamplers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfCombinedImageSamplers;		// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER"
				if (numberOfUniformTexelBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfUniformTexelBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER"
				if (numberOfStorageTexelBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfStorageTexelBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER"
				if (numberOfUniformBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfUniformBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE"
				if (numberOfStorageImage > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfStorageImage;		// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER"
				if (numberOfStorageBuffers > 0)
				{
					VkDescriptorPoolSize& vkDescriptorPoolSize = vkDescriptorPoolSizes[numberOfVkDescriptorPoolSizes];
					vkDescriptorPoolSize.type			 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;	// type (VkDescriptorType)
					vkDescriptorPoolSize.descriptorCount = maxSets * numberOfStorageBuffers;	// descriptorCount (uint32_t)
					++numberOfVkDescriptorPoolSizes;
				}

				// Create the Vulkan descriptor pool
				if (numberOfVkDescriptorPoolSizes > 0)
				{
					const VkDescriptorPoolCreateInfo VkDescriptorPoolCreateInfo =
					{
						VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,		// sType (VkStructureType)
						nullptr,											// pNext (const void*)
						VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,	// flags (VkDescriptorPoolCreateFlags)
						maxSets,											// maxSets (uint32_t)
						numberOfVkDescriptorPoolSizes,						// poolSizeCount (uint32_t)
						vkDescriptorPoolSizes.data()						// pPoolSizes (const VkDescriptorPoolSize*)
					};
					if (vkCreateDescriptorPool(vkDevice, &VkDescriptorPoolCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkDescriptorPool) != VK_SUCCESS)
					{
						RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan descriptor pool")
					}
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Root signature", 17);	// 17 = "Root signature: " including terminating zero
					for (VkDescriptorSetLayout vkDescriptorSetLayout : mVkDescriptorSetLayouts)
					{
						Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, (uint64_t)vkDescriptorSetLayout, detailedDebugName);
					}
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, (uint64_t)mVkPipelineLayout, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, (uint64_t)mVkDescriptorPool, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RootSignature() override
		{
			const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();

			// Destroy the Vulkan descriptor pool
			if (VK_NULL_HANDLE != mVkDescriptorPool)
			{
				vkDestroyDescriptorPool(vkDevice, mVkDescriptorPool, vulkanRhi.getVkAllocationCallbacks());
			}

			// Destroy the Vulkan pipeline layout
			if (VK_NULL_HANDLE != mVkPipelineLayout)
			{
				vkDestroyPipelineLayout(vkDevice, mVkPipelineLayout, vulkanRhi.getVkAllocationCallbacks());
			}

			// Destroy the Vulkan descriptor set layout
			for (VkDescriptorSetLayout vkDescriptorSetLayout : mVkDescriptorSetLayouts)
			{
				if (VK_NULL_HANDLE != vkDescriptorSetLayout)
				{
					vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, vulkanRhi.getVkAllocationCallbacks());
				}
			}

			// Destroy the root signature data
			const Rhi::Context& context = vulkanRhi.getContext();
			if (nullptr != mRootSignature.parameters)
			{
				for (uint32_t rootParameterIndex = 0; rootParameterIndex < mRootSignature.numberOfParameters; ++rootParameterIndex)
				{
					const Rhi::RootParameter& rootParameter = mRootSignature.parameters[rootParameterIndex];
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

		/**
		*  @brief
		*    Return the Vulkan pipeline layout
		*
		*  @return
		*    The Vulkan pipeline layout
		*/
		[[nodiscard]] inline VkPipelineLayout getVkPipelineLayout() const
		{
			return mVkPipelineLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan descriptor pool
		*
		*  @return
		*    The Vulkan descriptor pool
		*/
		[[nodiscard]] inline VkDescriptorPool getVkDescriptorPool() const
		{
			return mVkDescriptorPool;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRootSignature methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Rhi::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;


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
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<VkDescriptorSetLayout> VkDescriptorSetLayouts;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::RootSignature	   mRootSignature;
		VkDescriptorSetLayouts mVkDescriptorSetLayouts;
		VkPipelineLayout	   mVkPipelineLayout;
		VkDescriptorPool	   mVkDescriptorPool;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/IndexBuffer.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan index buffer object (IBO) interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Rhi::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(VulkanRhi& vulkanRhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, [[maybe_unused]] Rhi::BufferUsage bufferUsage, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndexBuffer(vulkanRhi),
			mVkIndexType(Mapping::getVulkanType(vulkanRhi.getContext(), indexBufferFormat)),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			int vkBufferUsageFlagBits = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			if ((bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) != 0 || (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRhi, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IBO", 6);	// 6 = "IBO: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRhi&>(getRhi()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan index type
		*
		*  @return
		*    The Vulkan index type
		*/
		[[nodiscard]] inline VkIndexType getVkIndexType() const
		{
			return mVkIndexType;
		}

		/**
		*  @brief
		*    Return the Vulkan index buffer
		*
		*  @return
		*    The Vulkan index buffer
		*/
		[[nodiscard]] inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		[[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
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
		VkIndexType	   mVkIndexType;	///< Vulkan vertex type
		VkBuffer	   mVkBuffer;		///< Vulkan vertex buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan vertex memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/VertexBuffer.h                              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan vertex buffer object (VBO) interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Rhi::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(VulkanRhi& vulkanRhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, [[maybe_unused]] Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexBuffer(vulkanRhi),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			int vkBufferUsageFlagBits = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			if ((bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) != 0 || (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRhi, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VBO", 6);	// 6 = "VBO: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRhi&>(getRhi()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan vertex buffer
		*
		*  @return
		*    The Vulkan vertex buffer
		*/
		[[nodiscard]] inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		[[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
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
		VkBuffer	   mVkBuffer;		///< Vulkan vertex buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan vertex memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/VertexArray.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan vertex array interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		VertexArray(VulkanRhi& vulkanRhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			IVertexArray(static_cast<Rhi::IRhi&>(vulkanRhi), id),
			mIndexBuffer(indexBuffer),
			mNumberOfSlots(numberOfVertexBuffers),
			mVertexVkBuffers(nullptr),
			mStrides(nullptr),
			mOffsets(nullptr),
			mVertexBuffers(nullptr)
		{
			// Add a reference to the given index buffer
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->addReference();
			}

			// Add a reference to the used vertex buffers
			if (mNumberOfSlots > 0)
			{
				const Rhi::Context& context = vulkanRhi.getContext();
				mVertexVkBuffers = RHI_MALLOC_TYPED(context, VkBuffer, mNumberOfSlots);
				mStrides = RHI_MALLOC_TYPED(context, uint32_t, mNumberOfSlots);
				mOffsets = RHI_MALLOC_TYPED(context, VkDeviceSize, mNumberOfSlots);
				memset(mOffsets, 0, sizeof(VkDeviceSize) * mNumberOfSlots);	// Vertex buffer offset is not supported by OpenGL, so our RHI implementation doesn't support it either, set everything to zero
				mVertexBuffers = RHI_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					VkBuffer* currentVertexVkBuffer = mVertexVkBuffers;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentVertexVkBuffer, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used RHI?
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						*currentVertexVkBuffer = (*currentVertexBuffer)->getVkBuffer();
						(*currentVertexBuffer)->addReference();
					}
				}

				{ // Gather slot related data
					const Rhi::VertexAttribute* attribute = vertexAttributes.attributes;
					const Rhi::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
					for (; attribute < attributesEnd;  ++attribute)
					{
						mStrides[attribute->inputSlot] = attribute->strideInBytes;
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexArray() override
		{
			// Release the index buffer reference
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->releaseReference();
			}

			// Cleanup Vulkan input slot data
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			const Rhi::Context& context = vulkanRhi.getContext();
			if (mNumberOfSlots > 0)
			{
				RHI_FREE(context, mVertexVkBuffers);
				RHI_FREE(context, mStrides);
				RHI_FREE(context, mOffsets);
			}

			// Release the reference to the used vertex buffers
			if (nullptr != mVertexBuffers)
			{
				// Release references
				VertexBuffer** vertexBuffersEnd = mVertexBuffers + mNumberOfSlots;
				for (VertexBuffer** vertexBuffer = mVertexBuffers; vertexBuffer < vertexBuffersEnd; ++vertexBuffer)
				{
					(*vertexBuffer)->releaseReference();
				}

				// Cleanup
				RHI_FREE(context, mVertexBuffers);
			}

			// Free the unique compact vertex array ID
			vulkanRhi.VertexArrayMakeId.DestroyID(getId());
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
		*    Bind Vulkan buffers
		*
		*  @param[in] vkCommandBuffer
		*    Vulkan command buffer to write into
		*/
		void bindVulkanBuffers(VkCommandBuffer vkCommandBuffer) const
		{
			// Set the Vulkan vertex buffers
			if (nullptr != mVertexVkBuffers)
			{
				vkCmdBindVertexBuffers(vkCommandBuffer, 0, mNumberOfSlots, mVertexVkBuffers, mOffsets);
			}
			else
			{
				// Do nothing since the Vulkan specification says "bindingCount must be greater than 0"
				// vkCmdBindVertexBuffers(vkCommandBuffer, 0, 0, nullptr, nullptr);
			}

			// Set the used index buffer
			// -> In case of no index buffer we don't set null indices, there's not really a point in it
			if (nullptr != mIndexBuffer)
			{
				vkCmdBindIndexBuffer(vkCommandBuffer, mIndexBuffer->getVkBuffer(), 0, mIndexBuffer->getVkIndexType());
			}
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
		IndexBuffer*   mIndexBuffer;		///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Vulkan input slots
		uint32_t	   mNumberOfSlots;		///< Number of used Vulkan input slots
		VkBuffer*	   mVertexVkBuffers;	///< Vulkan vertex buffers
		uint32_t*	   mStrides;			///< Strides in bytes, if "mVertexVkBuffers" is no null pointer this is no null pointer as well
		VkDeviceSize*  mOffsets;			///< Offsets in bytes, if "mVertexVkBuffers" is no null pointer this is no null pointer as well
		// For proper vertex buffer reference counter behaviour
		VertexBuffer** mVertexBuffers;		///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/TextureBuffer.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan texture buffer object (TBO) interface
	*/
	class TextureBuffer final : public Rhi::ITextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Rhi::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBuffer(VulkanRhi& vulkanRhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, [[maybe_unused]] Rhi::BufferUsage bufferUsage, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureBuffer(vulkanRhi),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkBufferView(VK_NULL_HANDLE)
		{
			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), (numberOfBytes % Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The Vulkan texture buffer size must be a multiple of the selected texture format bytes per texel")

			// Create the texture buffer
			uint32_t vkBufferUsageFlagBits = 0;
			if (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
			}
			if (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRhi, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Create Vulkan buffer view
			if ((bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0 || (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) != 0)
			{
				const VkBufferViewCreateInfo vkBufferViewCreateInfo =
				{
					VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,	// sType (VkStructureType)
					nullptr,									// pNext (const void*)
					0,											// flags (VkBufferViewCreateFlags)
					mVkBuffer,									// buffer (VkBuffer)
					Mapping::getVulkanFormat(textureFormat),	// format (VkFormat)
					0,											// offset (VkDeviceSize)
					VK_WHOLE_SIZE								// range (VkDeviceSize)
				};
				if (vkCreateBufferView(vulkanRhi.getVulkanContext().getVkDevice(), &vkBufferViewCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkBufferView) != VK_SUCCESS)
				{
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan buffer view")
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6);	// 6 = "TBO: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, (uint64_t)mVkBufferView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureBuffer() override
		{
			const VulkanRhi& vulkanRhi = static_cast<const VulkanRhi&>(getRhi());
			if (VK_NULL_HANDLE != mVkBufferView)
			{
				vkDestroyBufferView(vulkanRhi.getVulkanContext().getVkDevice(), mVkBufferView, vulkanRhi.getVkAllocationCallbacks());
			}
			Helper::destroyAndFreeVkBuffer(vulkanRhi, mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan texture buffer
		*
		*  @return
		*    The Vulkan texture buffer
		*/
		[[nodiscard]] inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		[[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
		}

		/**
		*  @brief
		*    Return the Vulkan buffer view
		*
		*  @return
		*    The Vulkan buffer view
		*/
		[[nodiscard]] inline VkBufferView getVkBufferView() const
		{
			return mVkBufferView;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan uniform texel buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan uniform texel memory
		VkBufferView   mVkBufferView;	///< Vulkan buffer view


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/StructuredBuffer.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan structured buffer object interface
	*/
	class StructuredBuffer final : public Rhi::IStructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBuffer(VulkanRhi& vulkanRhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IStructuredBuffer(vulkanRhi),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			// Sanity checks
			RHI_ASSERT(vulkanRhi.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The Vulkan structured buffer size must be a multiple of the given number of structure bytes")
			RHI_ASSERT(vulkanRhi.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The Vulkan structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// Create the structured buffer
			Helper::createAndAllocateVkBuffer(vulkanRhi, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "SBO", 6);	// 6 = "SBO: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~StructuredBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRhi&>(getRhi()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan structured buffer
		*
		*  @return
		*    The Vulkan structured buffer
		*/
		[[nodiscard]] inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		[[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit StructuredBuffer(const StructuredBuffer& source) = delete;
		StructuredBuffer& operator =(const StructuredBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan uniform texel buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan uniform texel memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/IndirectBuffer.h                            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan indirect buffer object interface
	*/
	class IndirectBuffer final : public Rhi::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Rhi::IndirectBufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBuffer(VulkanRhi& vulkanRhi, uint32_t numberOfBytes, const void* data, uint32_t indirectBufferFlags, [[maybe_unused]] Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndirectBuffer(vulkanRhi),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			// Sanity checks
			RHI_ASSERT(vulkanRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Vulkan flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RHI_ASSERT(vulkanRhi.getContext(), !((indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Vulkan flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RHI_ASSERT(vulkanRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawArguments)) == 0, "Vulkan indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RHI_ASSERT(vulkanRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawIndexedArguments)) == 0, "Vulkan indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// Create indirect buffer
			int vkBufferUsageFlagBits = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			if ((indirectBufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE) != 0)
			{
				vkBufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			Helper::createAndAllocateVkBuffer(vulkanRhi, static_cast<VkBufferUsageFlagBits>(vkBufferUsageFlagBits), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IndirectBufferObject", 23);	// 23 = "IndirectBufferObject: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRhi&>(getRhi()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan indirect buffer
		*
		*  @return
		*    The Vulkan indirect buffer
		*/
		[[nodiscard]] inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		[[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBuffer(const IndirectBuffer& source) = delete;
		IndirectBuffer& operator =(const IndirectBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkBuffer	   mVkBuffer;		///< Vulkan indirect buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan indirect memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/UniformBuffer.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(VulkanRhi& vulkanRhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IUniformBuffer(vulkanRhi),
			mVkBuffer(VK_NULL_HANDLE),
			mVkDeviceMemory(VK_NULL_HANDLE)
		{
			Helper::createAndAllocateVkBuffer(vulkanRhi, static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, numberOfBytes, data, mVkBuffer, mVkDeviceMemory);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "UBO", 6);	// 6 = "UBO: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)mVkBuffer, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBuffer() override
		{
			Helper::destroyAndFreeVkBuffer(static_cast<const VulkanRhi&>(getRhi()), mVkBuffer, mVkDeviceMemory);
		}

		/**
		*  @brief
		*    Return the Vulkan uniform buffer
		*
		*  @return
		*    The Vulkan uniform buffer
		*/
		[[nodiscard]] inline VkBuffer getVkBuffer() const
		{
			return mVkBuffer;
		}

		/**
		*  @brief
		*    Return the Vulkan device memory
		*
		*  @return
		*    The Vulkan device memory
		*/
		[[nodiscard]] inline VkDeviceMemory getVkDeviceMemory() const
		{
			return mVkDeviceMemory;
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
		VkBuffer	   mVkBuffer;		///< Vulkan uniform buffer
		VkDeviceMemory mVkDeviceMemory;	///< Vulkan uniform memory


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/BufferManager.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan buffer manager interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*/
		inline explicit BufferManager(VulkanRhi& vulkanRhi) :
			IBufferManager(vulkanRhi)
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
		[[nodiscard]] inline virtual Rhi::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), VertexBuffer)(vulkanRhi, numberOfBytes, data, bufferFlags, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::IndexBufferFormat::Enum indexBufferFormat = Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), IndexBuffer)(vulkanRhi, numberOfBytes, data, bufferFlags, bufferUsage, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexArray* createVertexArray(const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, Rhi::IIndexBuffer* indexBuffer = nullptr RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity checks
			#ifdef RHI_DEBUG
			{
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RHI_ASSERT(vulkanRhi.getContext(), &vulkanRhi == &vertexBuffer->vertexBuffer->getRhi(), "Vulkan error: The given vertex buffer resource is owned by another RHI instance")
				}
			}
			#endif
			RHI_ASSERT(vulkanRhi.getContext(), nullptr == indexBuffer || &vulkanRhi == &indexBuffer->getRhi(), "Vulkan error: The given index buffer resource is owned by another RHI instance")

			// Create vertex array
			uint16_t id = 0;
			if (vulkanRhi.VertexArrayMakeId.CreateID(id))
			{
				return RHI_NEW(vulkanRhi.getContext(), VertexArray)(vulkanRhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id);
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

		[[nodiscard]] inline virtual Rhi::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::R32G32B32A32F RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), TextureBuffer)(vulkanRhi, numberOfBytes, data, bufferFlags, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), StructuredBuffer)(vulkanRhi, numberOfBytes, data, bufferUsage, numberOfStructureBytes RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), IndirectBuffer)(vulkanRhi, numberOfBytes, data, indirectBufferFlags, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// -> Inside GLSL "layout(binding = 0, std140) writeonly uniform OutputUniformBuffer" will result in the GLSL compiler error "Failed to parse the GLSL shader source code: ERROR: 0:85: 'assign' :  l-value required "anon@6" (can't modify a uniform)"
			// -> Inside GLSL "layout(binding = 0, std430) writeonly buffer  OutputUniformBuffer" will work in OpenGL but will fail in Vulkan with "Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT" Object: "0" Location: "0" Message code: "13" Layer prefix: "Validation" Message: "Object: VK_NULL_HANDLE (Type = 0) | Type mismatch on descriptor slot 0.0 (used as type `ptr to uniform struct of (vec4 of float32)`) but descriptor of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER""
			// RHI_ASSERT(vulkanRhi.getContext(), (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid Vulkan buffer flags, uniform buffer can't be used for unordered access")
			// RHI_ASSERT(vulkanRhi.getContext(), (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0, "Invalid Vulkan buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RHI_NEW(vulkanRhi.getContext(), UniformBuffer)(vulkanRhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/Texture1D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 1D texture interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1D(VulkanRhi& vulkanRhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1D(vulkanRhi, width),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			Helper::createAndFillVkImage(vulkanRhi, VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D, { width, 1, 1 }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture", 13);	// 13 = "1D texture: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVkImage, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1D() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		[[nodiscard]] inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		[[nodiscard]] inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
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
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/Texture1DArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 1D array texture interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		Texture1DArray(VulkanRhi& vulkanRhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1DArray(vulkanRhi, width, numberOfSlices),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE),
			mVkFormat(Helper::createAndFillVkImage(vulkanRhi, VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D_ARRAY, { width, 1, numberOfSlices }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture array", 19);	// 19 = "1D texture array: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVkImage, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DArray() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		[[nodiscard]] inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		[[nodiscard]] inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan format
		*
		*  @return
		*    The Vulkan format
		*/
		[[nodiscard]] inline VkFormat getVkFormat() const
		{
			return mVkFormat;
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
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;
		VkFormat	   mVkFormat;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/Texture2D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    OpenVR-support: Data required for passing Vulkan textures to IVRCompositor::Submit; Be sure to call OpenVR_Shutdown before destroying these resources
	*
	*  @note
	*    - From OpenVR SDK 1.0.7 "openvr.h"-header
	*/
	struct VRVulkanTextureData_t
	{
		VkImage			 m_nImage;
		VkDevice		 m_pDevice;
		VkPhysicalDevice m_pPhysicalDevice;
		VkInstance		 m_pInstance;
		VkQueue			 m_pQueue;
		uint32_t		 m_nQueueFamilyIndex;
		uint32_t		 m_nWidth;
		uint32_t		 m_nHeight;
		VkFormat		 m_nFormat;
		uint32_t		 m_nSampleCount;
	};

	/**
	*  @brief
	*    Vulkan 2D texture interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		Texture2D(VulkanRhi& vulkanRhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2D(vulkanRhi, width, height),
			mVrVulkanTextureData{},
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			mVrVulkanTextureData.m_nFormat = Helper::createAndFillVkImage(vulkanRhi, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, { width, height, 1 }, textureFormat, data, textureFlags, numberOfMultisamples, mVrVulkanTextureData.m_nImage, mVkDeviceMemory, mVkImageView);

			// Fill the rest of the "VRVulkanTextureData_t"-structure
			const VulkanContext& vulkanContext = vulkanRhi.getVulkanContext();
			const VulkanRuntimeLinking& vulkanRuntimeLinking = vulkanRhi.getVulkanRuntimeLinking();
																									// m_nImage (VkImage) was set by "VulkanRhi::Helper::createAndFillVkImage()" above
			mVrVulkanTextureData.m_pDevice			 = vulkanContext.getVkDevice();					// m_pDevice (VkDevice)
			mVrVulkanTextureData.m_pPhysicalDevice	 = vulkanContext.getVkPhysicalDevice();			// m_pPhysicalDevice (VkPhysicalDevice)
			mVrVulkanTextureData.m_pInstance		 = vulkanRuntimeLinking.getVkInstance();		// m_pInstance (VkInstance)
			mVrVulkanTextureData.m_pQueue			 = vulkanContext.getGraphicsVkQueue();			// m_pQueue (VkQueue)
			mVrVulkanTextureData.m_nQueueFamilyIndex = vulkanContext.getGraphicsQueueFamilyIndex();	// m_nQueueFamilyIndex (uint32_t)
			mVrVulkanTextureData.m_nWidth			 = width;										// m_nWidth (uint32_t)
			mVrVulkanTextureData.m_nHeight			 = height;										// m_nHeight (uint32_t)
																									// m_nFormat (VkFormat)  was set by "VulkanRhi::Helper::createAndFillVkImage()" above
			mVrVulkanTextureData.m_nSampleCount		 = numberOfMultisamples;						// m_nSampleCount (uint32_t)

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture", 13);	// 13 = "2D texture: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVrVulkanTextureData.m_nImage, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2D() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mVrVulkanTextureData.m_nImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		[[nodiscard]] inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		[[nodiscard]] inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan format
		*
		*  @return
		*    The Vulkan format
		*/
		[[nodiscard]] inline VkFormat getVkFormat() const
		{
			return mVrVulkanTextureData.m_nFormat;
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
		inline void setMinimumMaximumMipmapIndex([[maybe_unused]] uint32_t minimumMipmapIndex, [[maybe_unused]] uint32_t maximumMipmapIndex)
		{
			// TODO(co) Implement me
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return reinterpret_cast<void*>(const_cast<VRVulkanTextureData_t*>(&mVrVulkanTextureData));
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
		VRVulkanTextureData_t mVrVulkanTextureData;
		VkImageLayout		  mVkImageLayout;
		VkDeviceMemory		  mVkDeviceMemory;
		VkImageView			  mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/Texture2DArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 2D array texture interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		Texture2DArray(VulkanRhi& vulkanRhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2DArray(vulkanRhi, width, height, numberOfSlices),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE),
			mVkFormat(Helper::createAndFillVkImage(vulkanRhi, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY, { width, height, numberOfSlices }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView))
		{
			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture array", 19);	// 19 = "2D texture array: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVkImage, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArray() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		[[nodiscard]] inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		[[nodiscard]] inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
		}

		/**
		*  @brief
		*    Return the Vulkan format
		*
		*  @return
		*    The Vulkan format
		*/
		[[nodiscard]] inline VkFormat getVkFormat() const
		{
			return mVkFormat;
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
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;
		VkFormat	   mVkFormat;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/Texture3D.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan 3D texture interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		Texture3D(VulkanRhi& vulkanRhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture3D(vulkanRhi, width, height, depth),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			Helper::createAndFillVkImage(vulkanRhi, VK_IMAGE_TYPE_3D, VK_IMAGE_VIEW_TYPE_3D, { width, height, depth }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "3D texture", 13);	// 13 = "3D texture: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVkImage, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3D() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		[[nodiscard]] inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		[[nodiscard]] inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
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
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/TextureCube.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan cube texture interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		TextureCube(VulkanRhi& vulkanRhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureCube(vulkanRhi, width, height),
			mVkImage(VK_NULL_HANDLE),
			mVkImageLayout(Helper::getVkImageLayoutByTextureFlags(textureFlags)),
			mVkDeviceMemory(VK_NULL_HANDLE),
			mVkImageView(VK_NULL_HANDLE)
		{
			Helper::createAndFillVkImage(vulkanRhi, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_CUBE, { width, height, 6 }, textureFormat, data, textureFlags, 1, mVkImage, mVkDeviceMemory, mVkImageView);

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Cube texture", 15);	// 15 = "Cube texture: " including terminating zero
					const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)mVkImage, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mVkDeviceMemory, detailedDebugName);
					Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t)mVkImageView, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCube() override
		{
			Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mVkImage, mVkDeviceMemory, mVkImageView);
		}

		/**
		*  @brief
		*    Return the Vulkan image view
		*
		*  @return
		*    The Vulkan image view
		*/
		[[nodiscard]] inline VkImageView getVkImageView() const
		{
			return mVkImageView;
		}

		/**
		*  @brief
		*    Return the Vulkan image layout
		*
		*  @return
		*    The Vulkan image layout
		*/
		[[nodiscard]] inline VkImageLayout getVkImageLayout() const
		{
			return mVkImageLayout;
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
		VkImage		   mVkImage;
		VkImageLayout  mVkImageLayout;
		VkDeviceMemory mVkDeviceMemory;
		VkImageView	   mVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Texture/TextureManager.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan texture manager interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*/
		inline explicit TextureManager(VulkanRhi& vulkanRhi) :
			ITextureManager(vulkanRhi)
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
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), width > 0, "Vulkan create texture 1D was called with invalid parameters")

			// Create 1D texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication
			return RHI_NEW(vulkanRhi.getContext(), Texture1D)(vulkanRhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), width > 0 && numberOfSlices > 0, "Vulkan create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource
			// -> The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication
			return RHI_NEW(vulkanRhi.getContext(), Texture1DArray)(vulkanRhi, width, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), width > 0 && height > 0, "Vulkan create texture 2D was called with invalid parameters")

			// Create 2D texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication
			return RHI_NEW(vulkanRhi.getContext(), Texture2D)(vulkanRhi, width, height, textureFormat, data, textureFlags, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), width > 0 && height > 0 && numberOfSlices > 0, "Vulkan create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource
			// -> The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication
			return RHI_NEW(vulkanRhi.getContext(), Texture2DArray)(vulkanRhi, width, height, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), width > 0 && height > 0 && depth > 0, "Vulkan create texture 3D was called with invalid parameters")

			// Create 3D texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication
			return RHI_NEW(vulkanRhi.getContext(), Texture3D)(vulkanRhi, width, height, depth, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), width > 0 && height > 0, "Vulkan create texture cube was called with invalid parameters")

			// Create cube texture resource
			// -> The indication of the texture usage is only relevant for Direct3D, Vulkan has no texture usage indication
			return RHI_NEW(vulkanRhi.getContext(), TextureCube)(vulkanRhi, width, height, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/State/SamplerState.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan sampler state interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(VulkanRhi& vulkanRhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ISamplerState(vulkanRhi),
			mVkSampler(VK_NULL_HANDLE)
		{
			// Sanity checks
			RHI_ASSERT(vulkanRhi.getContext(), samplerState.filter != Rhi::FilterMode::UNKNOWN, "Vulkan filter mode must not be unknown")
			RHI_ASSERT(vulkanRhi.getContext(), samplerState.maxAnisotropy <= vulkanRhi.getCapabilities().maximumAnisotropy, "Maximum Vulkan anisotropy value violated")

			// TODO(co) Map "Rhi::SamplerState" to VkSamplerCreateInfo
			const bool anisotropyEnable = (Rhi::FilterMode::ANISOTROPIC == samplerState.filter || Rhi::FilterMode::COMPARISON_ANISOTROPIC == samplerState.filter);
			const VkSamplerCreateInfo vkSamplerCreateInfo =
			{
				VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,											// sType (VkStructureType)
				nullptr,																		// pNext (const void*)
				0,																				// flags (VkSamplerCreateFlags)
				Mapping::getVulkanMagFilterMode(vulkanRhi.getContext(), samplerState.filter),	// magFilter (VkFilter)
				Mapping::getVulkanMinFilterMode(vulkanRhi.getContext(), samplerState.filter),	// minFilter (VkFilter)
				Mapping::getVulkanMipmapMode(vulkanRhi.getContext(), samplerState.filter),		// mipmapMode (VkSamplerMipmapMode)
				Mapping::getVulkanTextureAddressMode(samplerState.addressU),					// addressModeU (VkSamplerAddressMode)
				Mapping::getVulkanTextureAddressMode(samplerState.addressV),					// addressModeV (VkSamplerAddressMode)
				Mapping::getVulkanTextureAddressMode(samplerState.addressW),					// addressModeW (VkSamplerAddressMode)
				samplerState.mipLodBias,														// mipLodBias (float)
				static_cast<VkBool32>(anisotropyEnable),										// anisotropyEnable (VkBool32)
				static_cast<float>(samplerState.maxAnisotropy),									// maxAnisotropy (float)
				VK_FALSE,																		// compareEnable (VkBool32)
				VK_COMPARE_OP_ALWAYS,															// compareOp (VkCompareOp)
				samplerState.minLod,															// minLod (float)
				samplerState.maxLod,															// maxLod (float)
				VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,										// borderColor (VkBorderColor)
				VK_FALSE																		// unnormalizedCoordinates (VkBool32)
			};
			if (vkCreateSampler(vulkanRhi.getVulkanContext().getVkDevice(), &vkSamplerCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkSampler) == VK_SUCCESS)
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != vkDebugMarkerSetObjectNameEXT)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Sampler state", 16);	// 16 = "Sampler state: " including terminating zero
						Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, (uint64_t)mVkSampler, detailedDebugName);
					}
				#endif
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create Vulkan sampler instance")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SamplerState() override
		{
			if (VK_NULL_HANDLE != mVkSampler)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroySampler(vulkanRhi.getVulkanContext().getVkDevice(), mVkSampler, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan sampler
		*
		*  @return
		*    The Vulkan sampler
		*/
		[[nodiscard]] inline VkSampler getVkSampler() const
		{
			return mVkSampler;
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
		VkSampler mVkSampler;	///< Vulkan sampler instance, "VK_NULL_HANDLE" in case of error


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/RenderTarget/RenderPass.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan render pass interface
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
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
		RenderPass(VulkanRhi& vulkanRhi, uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IRenderPass(vulkanRhi),
			mVkRenderPass(VK_NULL_HANDLE),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat),
			mVkSampleCountFlagBits(Mapping::getVulkanSampleCountFlagBits(vulkanRhi.getContext(), numberOfMultisamples))
		{
			const bool hasDepthStencilAttachment = (Rhi::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat);

			// Vulkan attachment descriptions
			std::vector<VkAttachmentDescription> vkAttachmentDescriptions;
			vkAttachmentDescriptions.resize(mNumberOfColorAttachments + (hasDepthStencilAttachment ? 1u : 0u));
			uint32_t currentVkAttachmentDescriptionIndex = 0;

			// Handle color attachments
			typedef std::vector<VkAttachmentReference> VkAttachmentReferences;
			VkAttachmentReferences colorVkAttachmentReferences;
			if (mNumberOfColorAttachments > 0)
			{
				colorVkAttachmentReferences.resize(mNumberOfColorAttachments);
				for (uint32_t i = 0; i < mNumberOfColorAttachments; ++i)
				{
					{ // Setup Vulkan color attachment references
						VkAttachmentReference& vkAttachmentReference = colorVkAttachmentReferences[currentVkAttachmentDescriptionIndex];
						vkAttachmentReference.attachment = currentVkAttachmentDescriptionIndex;			// attachment (uint32_t)
						vkAttachmentReference.layout	 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// layout (VkImageLayout)
					}

					{ // Setup Vulkan color attachment description
						VkAttachmentDescription& vkAttachmentDescription = vkAttachmentDescriptions[currentVkAttachmentDescriptionIndex];
						vkAttachmentDescription.flags		   = 0;																// flags (VkAttachmentDescriptionFlags)
						vkAttachmentDescription.format		   = Mapping::getVulkanFormat(colorAttachmentTextureFormats[i]);	// format (VkFormat)
						vkAttachmentDescription.samples		   = mVkSampleCountFlagBits;										// samples (VkSampleCountFlagBits)
						vkAttachmentDescription.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;									// loadOp (VkAttachmentLoadOp)
						vkAttachmentDescription.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;									// storeOp (VkAttachmentStoreOp)
						vkAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;								// stencilLoadOp (VkAttachmentLoadOp)
						vkAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;								// stencilStoreOp (VkAttachmentStoreOp)
						vkAttachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;										// initialLayout (VkImageLayout)
						vkAttachmentDescription.finalLayout	   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;						// finalLayout (VkImageLayout)
					}

					// Advance current Vulkan attachment description index
					++currentVkAttachmentDescriptionIndex;
				}
			}

			// Handle depth stencil attachments
			const VkAttachmentReference depthVkAttachmentReference =
			{
				currentVkAttachmentDescriptionIndex,				// attachment (uint32_t)
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// layout (VkImageLayout)
			};
			if (hasDepthStencilAttachment)
			{
				// Setup Vulkan depth attachment description
				VkAttachmentDescription& vkAttachmentDescription = vkAttachmentDescriptions[currentVkAttachmentDescriptionIndex];
				vkAttachmentDescription.flags		   = 0;																// flags (VkAttachmentDescriptionFlags)
				vkAttachmentDescription.format		   = Mapping::getVulkanFormat(depthStencilAttachmentTextureFormat);	// format (VkFormat)
				vkAttachmentDescription.samples		   = mVkSampleCountFlagBits;										// samples (VkSampleCountFlagBits)
				vkAttachmentDescription.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;									// loadOp (VkAttachmentLoadOp)
				vkAttachmentDescription.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;									// storeOp (VkAttachmentStoreOp)
				vkAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;								// stencilLoadOp (VkAttachmentLoadOp)
				vkAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;								// stencilStoreOp (VkAttachmentStoreOp)
				vkAttachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;										// initialLayout (VkImageLayout)
				vkAttachmentDescription.finalLayout	   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;				// finalLayout (VkImageLayout)
				// ++currentVkAttachmentDescriptionIndex;	// Not needed since we're the last
			}

			// Create Vulkan create render pass
			const VkSubpassDescription vkSubpassDescription =
			{
				0,																				// flags (VkSubpassDescriptionFlags)
				VK_PIPELINE_BIND_POINT_GRAPHICS,												// pipelineBindPoint (VkPipelineBindPoint)
				0,																				// inputAttachmentCount (uint32_t)
				nullptr,																		// pInputAttachments (const VkAttachmentReference*)
				mNumberOfColorAttachments,														// colorAttachmentCount (uint32_t)
				(mNumberOfColorAttachments > 0) ? colorVkAttachmentReferences.data() : nullptr,	// pColorAttachments (const VkAttachmentReference*)
				nullptr,																		// pResolveAttachments (const VkAttachmentReference*)
				hasDepthStencilAttachment ? &depthVkAttachmentReference : nullptr,				// pDepthStencilAttachment (const VkAttachmentReference*)
				0,																				// preserveAttachmentCount (uint32_t)
				nullptr																			// pPreserveAttachments (const uint32_t*)
			};
			static constexpr std::array<VkSubpassDependency, 2> vkSubpassDependencies =
			{{
				{
					VK_SUBPASS_EXTERNAL,														// srcSubpass (uint32_t)
					0,																			// dstSubpass (uint32_t)
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,										// srcStageMask (VkPipelineStageFlags)
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// dstStageMask (VkPipelineStageFlags)
					VK_ACCESS_MEMORY_READ_BIT,													// srcAccessMask (VkAccessFlags)
					VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// dstAccessMask (VkAccessFlags)
					VK_DEPENDENCY_BY_REGION_BIT													// dependencyFlags (VkDependencyFlags)
				},
				{
					0,																			// srcSubpass (uint32_t)
					VK_SUBPASS_EXTERNAL,														// dstSubpass (uint32_t)
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,								// srcStageMask (VkPipelineStageFlags)
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,										// dstStageMask (VkPipelineStageFlags)
					VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	// srcAccessMask (VkAccessFlags)
					VK_ACCESS_MEMORY_READ_BIT,													// dstAccessMask (VkAccessFlags)
					VK_DEPENDENCY_BY_REGION_BIT													// dependencyFlags (VkDependencyFlags)
				}
			}};
			const VkRenderPassCreateInfo vkRenderPassCreateInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,				// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkRenderPassCreateFlags)
				static_cast<uint32_t>(vkAttachmentDescriptions.size()),	// attachmentCount (uint32_t)
				vkAttachmentDescriptions.data(),						// pAttachments (const VkAttachmentDescription*)
				1,														// subpassCount (uint32_t)
				&vkSubpassDescription,									// pSubpasses (const VkSubpassDescription*)
				static_cast<uint32_t>(vkSubpassDependencies.size()),	// dependencyCount (uint32_t)
				vkSubpassDependencies.data()							// pDependencies (const VkSubpassDependency*)
			};
			if (vkCreateRenderPass(vulkanRhi.getVulkanContext().getVkDevice(), &vkRenderPassCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkRenderPass) == VK_SUCCESS)
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != vkDebugMarkerSetObjectNameEXT)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Render pass", 14);	// 14 = "Render pass: " including terminating zero
						Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, (uint64_t)mVkRenderPass, detailedDebugName);
					}
				#endif
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create Vulkan render pass")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RenderPass() override
		{
			// Destroy Vulkan render pass instance
			if (VK_NULL_HANDLE != mVkRenderPass)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyRenderPass(vulkanRhi.getVulkanContext().getVkDevice(), mVkRenderPass, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan render pass
		*
		*  @return
		*    The Vulkan render pass
		*/
		[[nodiscard]] inline VkRenderPass getVkRenderPass() const
		{
			return mVkRenderPass;
		}

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
		*    Return the depth stencil attachment texture format
		*
		*  @return
		*    The depth stencil attachment texture format
		*/
		[[nodiscard]] inline Rhi::TextureFormat::Enum getDepthStencilAttachmentTextureFormat() const
		{
			return mDepthStencilAttachmentTextureFormat;
		}

		/**
		*  @brief
		*    Return the Vulkan sample count flag bits
		*
		*  @return
		*    The Vulkan sample count flag bits
		*/
		[[nodiscard]] inline VkSampleCountFlagBits getVkSampleCountFlagBits() const
		{
			return mVkSampleCountFlagBits;
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
		VkRenderPass			 mVkRenderPass;							///< Vulkan render pass instance, can be a null handle
		uint32_t				 mNumberOfColorAttachments;				///< Number of color render target textures
		Rhi::TextureFormat::Enum mDepthStencilAttachmentTextureFormat;	///< The depth stencil attachment texture format
		VkSampleCountFlagBits	 mVkSampleCountFlagBits;				///< Vulkan sample count flag bits


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/QueryPool.h                                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan asynchronous query pool interface
	*/
	class QueryPool final : public Rhi::IQueryPool
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		QueryPool(VulkanRhi& vulkanRhi, Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IQueryPool(vulkanRhi),
			mQueryType(queryType),
			mVkQueryPool(VK_NULL_HANDLE)
		{
			// Get Vulkan query pool create information
			VkQueryPoolCreateInfo vkQueryPoolCreateInfo;
			vkQueryPoolCreateInfo.sType		 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;	// VkStructureType
			vkQueryPoolCreateInfo.pNext		 = nullptr;										// const void*
			vkQueryPoolCreateInfo.flags		 = 0;											// VkQueryPoolCreateFlags
			vkQueryPoolCreateInfo.queryCount = numberOfQueries;								// uint32_t
			switch (queryType)
			{
				case Rhi::QueryType::OCCLUSION:
					vkQueryPoolCreateInfo.queryType			 = VK_QUERY_TYPE_OCCLUSION;	// VkQueryType
					vkQueryPoolCreateInfo.pipelineStatistics = 0;						// VkQueryPipelineStatisticFlags
					break;

				case Rhi::QueryType::PIPELINE_STATISTICS:
					// This setup results in the same structure layout as used by "D3D11_QUERY_DATA_PIPELINE_STATISTICS" which we use for "Rhi::PipelineStatisticsQueryResult"
					vkQueryPoolCreateInfo.queryType			 = VK_QUERY_TYPE_PIPELINE_STATISTICS;	// VkQueryType
					vkQueryPoolCreateInfo.pipelineStatistics = 										// VkQueryPipelineStatisticFlags
						VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT						|
						VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT					|
						VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT					|
						VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT					|
						VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT					|
						VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT						|
						VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT							|
						VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT					|
						VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT			|
						VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT	|
						VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
					break;

				case Rhi::QueryType::TIMESTAMP:
					vkQueryPoolCreateInfo.queryType			 = VK_QUERY_TYPE_TIMESTAMP;	// VkQueryType
					vkQueryPoolCreateInfo.pipelineStatistics = 0;						// VkQueryPipelineStatisticFlags
					break;
			}

			// Create Vulkan query pool
			if (vkCreateQueryPool(vulkanRhi.getVulkanContext().getVkDevice(), &vkQueryPoolCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkQueryPool) == VK_SUCCESS)
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != vkDebugMarkerSetObjectNameEXT)
					{
						switch (queryType)
						{
							case Rhi::QueryType::OCCLUSION:
							{
								RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Occlusion query", 18);	// 18 = "Occlusion query: " including terminating zero
								Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, (uint64_t)mVkQueryPool, detailedDebugName);
								break;
							}

							case Rhi::QueryType::PIPELINE_STATISTICS:
							{
								RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Pipeline statistics query", 28);	// 28 = "Pipeline statistics query: " including terminating zero
								Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, (uint64_t)mVkQueryPool, detailedDebugName);
								break;
							}

							case Rhi::QueryType::TIMESTAMP:
							{
								RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Timestamp query", 18);	// 18 = "Timestamp query: " including terminating zero
								Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, (uint64_t)mVkQueryPool, detailedDebugName);
								break;
							}
						}
					}
				#endif
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create Vulkan query pool")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~QueryPool() override
		{
			// Destroy Vulkan query pool instance
			if (VK_NULL_HANDLE != mVkQueryPool)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyQueryPool(vulkanRhi.getVulkanContext().getVkDevice(), mVkQueryPool, vulkanRhi.getVkAllocationCallbacks());
			}
		}

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
		*    Return the Vulkan query pool
		*
		*  @return
		*    The Vulkan query pool
		*/
		[[nodiscard]] inline VkQueryPool getVkQueryPool() const
		{
			return mVkQueryPool;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), QueryPool, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit QueryPool(const QueryPool& source) = delete;
		QueryPool& operator =(const QueryPool& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::QueryType mQueryType;
		VkQueryPool	   mVkQueryPool;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/RenderTarget/SwapChain.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan swap chain class
	*
	*  @todo
	*    - TODO(co) Add support for debug name (not that important while at the same time more complex to implement here, but lets keep the TODO here to know there's room for improvement)
	*/
	class SwapChain final : public Rhi::ISwapChain
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] static VkFormat findColorVkFormat(const Rhi::Context& context, VkInstance vkInstance, const VulkanContext& vulkanContext)
		{
			const VkPhysicalDevice vkPhysicalDevice = vulkanContext.getVkPhysicalDevice();
			const VkSurfaceKHR vkSurfaceKHR = detail::createPresentationSurface(context, vulkanContext.getVulkanRhi().getVkAllocationCallbacks(), vkInstance, vkPhysicalDevice, vulkanContext.getGraphicsQueueFamilyIndex(), Rhi::WindowHandle{context.getNativeWindowHandle(), nullptr, nullptr});
			const VkSurfaceFormatKHR desiredVkSurfaceFormatKHR = ::detail::getSwapChainFormat(context, vkPhysicalDevice, vkSurfaceKHR);
			vkDestroySurfaceKHR(vkInstance, vkSurfaceKHR, vulkanContext.getVulkanRhi().getVkAllocationCallbacks());
			return desiredVkSurfaceFormatKHR.format;
		}

		[[nodiscard]] inline static VkFormat findDepthVkFormat(VkPhysicalDevice vkPhysicalDevice)
		{
			return detail::findSupportedVkFormat(vkPhysicalDevice, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}


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
		SwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle) :
			ISwapChain(renderPass),
			// Operation system window
			mNativeWindowHandle(windowHandle.nativeWindowHandle),
			mRenderWindow(windowHandle.renderWindow),
			// Vulkan presentation surface
			mVkSurfaceKHR(VK_NULL_HANDLE),
			// Vulkan swap chain and color render target related
			mVkSwapchainKHR(VK_NULL_HANDLE),
			mVkRenderPass(VK_NULL_HANDLE),
			mImageAvailableVkSemaphore(VK_NULL_HANDLE),
			mRenderingFinishedVkSemaphore(VK_NULL_HANDLE),
			mCurrentImageIndex(~0u),
			// Depth render target related
			mDepthVkFormat(Mapping::getVulkanFormat(static_cast<RenderPass&>(renderPass).getDepthStencilAttachmentTextureFormat())),
			mDepthVkImage(VK_NULL_HANDLE),
			mDepthVkDeviceMemory(VK_NULL_HANDLE),
			mDepthVkImageView(VK_NULL_HANDLE)
		{
			// Create the Vulkan presentation surface instance depending on the operation system
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(renderPass.getRhi());
			const VulkanContext&   vulkanContext	= vulkanRhi.getVulkanContext();
			const VkInstance	   vkInstance		= vulkanRhi.getVulkanRuntimeLinking().getVkInstance();
			const VkPhysicalDevice vkPhysicalDevice	= vulkanContext.getVkPhysicalDevice();
			mVkSurfaceKHR = detail::createPresentationSurface(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vkInstance, vkPhysicalDevice, vulkanContext.getGraphicsQueueFamilyIndex(), windowHandle);
			if (VK_NULL_HANDLE != mVkSurfaceKHR)
			{
				// Create the Vulkan swap chain
				createVulkanSwapChain();
			}
			else
			{
				// Error!
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "The swap chain failed to create the Vulkan presentation surface")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SwapChain() override
		{
			if (VK_NULL_HANDLE != mVkSurfaceKHR)
			{
				destroyVulkanSwapChain();
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroySurfaceKHR(vulkanRhi.getVulkanRuntimeLinking().getVkInstance(), mVkSurfaceKHR, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan render pass
		*
		*  @return
		*    The Vulkan render pass
		*/
		[[nodiscard]] inline VkRenderPass getVkRenderPass() const
		{
			return mVkRenderPass;
		}

		/**
		*  @brief
		*    Return the current Vulkan image to render color into
		*
		*  @return
		*    The current Vulkan image to render color into
		*/
		[[nodiscard]] inline VkImage getColorCurrentVkImage() const
		{
			RHI_ASSERT(getRhi().getContext(), ~0u != mCurrentImageIndex, "Invalid index of the current Vulkan swap chain image to render into (Vulkan swap chain creation failed?)")
			RHI_ASSERT(getRhi().getContext(), mCurrentImageIndex < mSwapChainBuffer.size(), "Out-of-bounds index of the current Vulkan swap chain image to render into")
			return mSwapChainBuffer[mCurrentImageIndex].vkImage;
		}

		/**
		*  @brief
		*    Return the Vulkan image to render depth into
		*
		*  @return
		*    The Vulkan image to render depth into
		*/
		[[nodiscard]] inline VkImage getDepthVkImage() const
		{
			return mDepthVkImage;
		}

		/**
		*  @brief
		*    Return the current Vulkan framebuffer to render into
		*
		*  @return
		*    The current Vulkan framebuffer to render into
		*/
		[[nodiscard]] inline VkFramebuffer getCurrentVkFramebuffer() const
		{
			RHI_ASSERT(getRhi().getContext(), ~0u != mCurrentImageIndex, "Invalid index of the current Vulkan swap chain image to render into (Vulkan swap chain creation failed?)")
			RHI_ASSERT(getRhi().getContext(), mCurrentImageIndex < mSwapChainBuffer.size(), "Out-of-bounds index of the current Vulkan swap chain image to render into")
			return mSwapChainBuffer[mCurrentImageIndex].vkFramebuffer;
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
			#elif defined(__ANDROID__)
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					// TODO(co) Get size on Android
					width = height = 1;
				}
				else
			#elif defined LINUX
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
					const Rhi::Context& context = vulkanRhi.getContext();
					RHI_ASSERT(context, context.getType() == Rhi::Context::ContextType::X11, "Invalid Vulkan context type")

					// If the given RHI context is an X11 context use the display connection object provided by the context
					if (context.getType() == Rhi::Context::ContextType::X11)
					{
						const Rhi::X11Context& x11Context = static_cast<const Rhi::X11Context&>(context);
						Display* display = x11Context.getDisplay();

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

		inline virtual void setVerticalSynchronizationInterval(uint32_t) override
		{
			// TODO(co) Implement usage of "synchronizationInterval"
		}

		virtual void present() override
		{
			// TODO(co) "Rhi::IRenderWindow::present()" support
			/*
			if (nullptr != mRenderWindow)
			{
				mRenderWindow->present();
				return;
			}
			*/

			// Get the Vulkan context
			const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			const VulkanContext& vulkanContext = vulkanRhi.getVulkanContext();

			{ // Queue submit
				const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				const VkCommandBuffer vkCommandBuffer = vulkanContext.getVkCommandBuffer();
				const VkSubmitInfo vkSubmitInfo =
				{
					VK_STRUCTURE_TYPE_SUBMIT_INFO,	// sType (VkStructureType)
					nullptr,						// pNext (const void*)
					1,								// waitSemaphoreCount (uint32_t)
					&mImageAvailableVkSemaphore,	// pWaitSemaphores (const VkSemaphore*)
					&waitDstStageMask,				// pWaitDstStageMask (const VkPipelineStageFlags*)
					1,								// commandBufferCount (uint32_t)
					&vkCommandBuffer,				// pCommandBuffers (const VkCommandBuffer*)
					1,								// signalSemaphoreCount (uint32_t)
					&mRenderingFinishedVkSemaphore	// pSignalSemaphores (const VkSemaphore*)
				};
				if (vkQueueSubmit(vulkanContext.getGraphicsVkQueue(), 1, &vkSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
				{
					// Error!
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Vulkan queue submit failed")
					return;
				}
			}

			{ // Queue present
				const VkPresentInfoKHR vkPresentInfoKHR =
				{
					VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,	// sType (VkStructureType)
					nullptr,							// pNext (const void*)
					1,									// waitSemaphoreCount (uint32_t)
					&mRenderingFinishedVkSemaphore,		// pWaitSemaphores (const VkSemaphore*)
					1,									// swapchainCount (uint32_t)
					&mVkSwapchainKHR,					// pSwapchains (const VkSwapchainKHR*)
					&mCurrentImageIndex,				// pImageIndices (const uint32_t*)
					nullptr								// pResults (VkResult*)
				};
				const VkResult vkResult = vkQueuePresentKHR(vulkanContext.getPresentVkQueue(), &vkPresentInfoKHR);
				if (VK_SUCCESS != vkResult)
				{
					if (VK_ERROR_OUT_OF_DATE_KHR == vkResult || VK_SUBOPTIMAL_KHR == vkResult)
					{
						// Recreate the Vulkan swap chain
						createVulkanSwapChain();
						return;
					}
					else
					{
						// Error!
						RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to present Vulkan queue")
						return;
					}
				}
				vkQueueWaitIdle(vulkanContext.getPresentVkQueue());
			}

			// Acquire next image
			acquireNextImage(true);
		}

		inline virtual void resizeBuffers() override
		{
			// Recreate the Vulkan swap chain
			createVulkanSwapChain();
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

		void createVulkanSwapChain()
		{
			// Get the Vulkan physical device
			const VulkanRhi&	   vulkanRhi		= static_cast<const VulkanRhi&>(getRhi());
			const Rhi::Context&	   context			= vulkanRhi.getContext();
			const VulkanContext&   vulkanContext	= vulkanRhi.getVulkanContext();
			const VkPhysicalDevice vkPhysicalDevice	= vulkanContext.getVkPhysicalDevice();
			const VkDevice		   vkDevice			= vulkanContext.getVkDevice();

			// Sanity checks
			RHI_ASSERT(context, VK_NULL_HANDLE != vkPhysicalDevice, "Invalid physical Vulkan device")
			RHI_ASSERT(context, VK_NULL_HANDLE != vkDevice, "Invalid Vulkan device")

			// Wait for the Vulkan device to become idle
			vkDeviceWaitIdle(vkDevice);

			// Get Vulkan surface capabilities
			VkSurfaceCapabilitiesKHR vkSurfaceCapabilitiesKHR;
			if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, mVkSurfaceKHR, &vkSurfaceCapabilitiesKHR) != VK_SUCCESS)
			{
				RHI_LOG(context, CRITICAL, "Failed to get physical Vulkan device surface capabilities")
				return;
			}

			// Get Vulkan swap chain settings
			const uint32_t                      desiredNumberOfImages				 = ::detail::getNumberOfSwapChainImages(vkSurfaceCapabilitiesKHR);
			const VkSurfaceFormatKHR            desiredVkSurfaceFormatKHR			 = ::detail::getSwapChainFormat(context, vkPhysicalDevice, mVkSurfaceKHR);
			const VkExtent2D                    desiredVkExtent2D					 = ::detail::getSwapChainExtent(vkSurfaceCapabilitiesKHR);
			const VkImageUsageFlags             desiredVkImageUsageFlags			 = ::detail::getSwapChainUsageFlags(context, vkSurfaceCapabilitiesKHR);
			const VkSurfaceTransformFlagBitsKHR desiredVkSurfaceTransformFlagBitsKHR = ::detail::getSwapChainTransform(vkSurfaceCapabilitiesKHR);
			const VkPresentModeKHR              desiredVkPresentModeKHR				 = ::detail::getSwapChainPresentMode(context, vkPhysicalDevice, mVkSurfaceKHR);

			// Validate Vulkan swap chain settings
			if (-1 == static_cast<int>(desiredVkImageUsageFlags))
			{
				RHI_LOG(context, CRITICAL, "Invalid desired Vulkan image usage flags")
				return;
			}
			if (VK_PRESENT_MODE_MAX_ENUM_KHR == desiredVkPresentModeKHR)
			{
				RHI_LOG(context, CRITICAL, "Invalid desired Vulkan presentation mode")
				return;
			}
			if ((0 == desiredVkExtent2D.width) || (0 == desiredVkExtent2D.height))
			{
				// Current surface size is (0, 0) so we can't create a swap chain and render anything (CanRender == false)
				// But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
				destroyVulkanSwapChain();
				return;
			}

			{ // Create Vulkan swap chain
				VkSwapchainKHR newVkSwapchainKHR = VK_NULL_HANDLE;
				const VkSwapchainCreateInfoKHR vkSwapchainCreateInfoKHR =
				{
					VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					0,												// flags (VkSwapchainCreateFlagsKHR)
					mVkSurfaceKHR,									// surface (VkSurfaceKHR)
					desiredNumberOfImages,							// minImageCount (uint32_t)
					desiredVkSurfaceFormatKHR.format,				// imageFormat (VkFormat)
					desiredVkSurfaceFormatKHR.colorSpace,			// imageColorSpace (VkColorSpaceKHR)
					desiredVkExtent2D,								// imageExtent (VkExtent2D)
					1,												// imageArrayLayers (uint32_t)
					desiredVkImageUsageFlags,						// imageUsage (VkImageUsageFlags)
					VK_SHARING_MODE_EXCLUSIVE,						// imageSharingMode (VkSharingMode)
					0,												// queueFamilyIndexCount (uint32_t)
					nullptr,										// pQueueFamilyIndices (const uint32_t*)
					desiredVkSurfaceTransformFlagBitsKHR,			// preTransform (VkSurfaceTransformFlagBitsKHR)
					VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,				// compositeAlpha (VkCompositeAlphaFlagBitsKHR)
					desiredVkPresentModeKHR,						// presentMode (VkPresentModeKHR)
					VK_TRUE,										// clipped (VkBool32)
					mVkSwapchainKHR									// oldSwapchain (VkSwapchainKHR)
				};
				if (vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreateInfoKHR, vulkanRhi.getVkAllocationCallbacks(), &newVkSwapchainKHR) != VK_SUCCESS)
				{
					RHI_LOG(context, CRITICAL, "Failed to create Vulkan swap chain")
					return;
				}
				destroyVulkanSwapChain();
				mVkSwapchainKHR = newVkSwapchainKHR;
			}

			// Create depth render target
			createDepthRenderTarget(desiredVkExtent2D);

			// Create render pass
			mVkRenderPass = ::detail::createRenderPass(context, vulkanRhi.getVkAllocationCallbacks(), vkDevice, desiredVkSurfaceFormatKHR.format, mDepthVkFormat, static_cast<RenderPass&>(getRenderPass()).getVkSampleCountFlagBits());

			// Vulkan swap chain image handling
			if (VK_NULL_HANDLE != mVkRenderPass)
			{
				// Get the swap chain images
				uint32_t swapchainImageCount = 0;
				if (vkGetSwapchainImagesKHR(vkDevice, mVkSwapchainKHR, &swapchainImageCount, nullptr) != VK_SUCCESS)
				{
					RHI_LOG(context, CRITICAL, "Failed to get Vulkan swap chain images")
					return;
				}
				std::vector<VkImage> vkImages(swapchainImageCount);
				if (vkGetSwapchainImagesKHR(vkDevice, mVkSwapchainKHR, &swapchainImageCount, vkImages.data()) != VK_SUCCESS)
				{
					RHI_LOG(context, CRITICAL, "Failed to get Vulkan swap chain images")
					return;
				}

				// Get the swap chain buffers containing the image and image view
				mSwapChainBuffer.resize(swapchainImageCount);
				const bool hasDepthStencilAttachment = (VK_FORMAT_UNDEFINED != mDepthVkFormat);
				for (uint32_t i = 0; i < swapchainImageCount; ++i)
				{
					SwapChainBuffer& swapChainBuffer = mSwapChainBuffer[i];
					swapChainBuffer.vkImage = vkImages[i];

					// Create the Vulkan image view
					Helper::createVkImageView(vulkanRhi, swapChainBuffer.vkImage, VK_IMAGE_VIEW_TYPE_2D, 1, 1, desiredVkSurfaceFormatKHR.format, VK_IMAGE_ASPECT_COLOR_BIT, swapChainBuffer.vkImageView);

					{ // Create the Vulkan framebuffer
						const std::array<VkImageView, 2> vkImageViews =
						{
							swapChainBuffer.vkImageView,
							mDepthVkImageView
						};
						const VkFramebufferCreateInfo vkFramebufferCreateInfo =
						{
							VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							0,											// flags (VkFramebufferCreateFlags)
							mVkRenderPass,								// renderPass (VkRenderPass)
							hasDepthStencilAttachment ? 2u : 1u,		// attachmentCount (uint32_t)
							vkImageViews.data(),						// pAttachments (const VkImageView*)
							desiredVkExtent2D.width,					// width (uint32_t)
							desiredVkExtent2D.height,					// height (uint32_t)
							1											// layers (uint32_t)
						};
						if (vkCreateFramebuffer(vkDevice, &vkFramebufferCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &swapChainBuffer.vkFramebuffer) != VK_SUCCESS)
						{
							RHI_LOG(context, CRITICAL, "Failed to create Vulkan framebuffer")
						}
					}
				}
			}

			{ // Create the Vulkan semaphores
				static constexpr VkSemaphoreCreateInfo vkSemaphoreCreateInfo =
				{
					VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	// sType (VkStructureType)
					nullptr,									// pNext (const void*)
					0											// flags (VkSemaphoreCreateFlags)
				};
				if ((vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mImageAvailableVkSemaphore) != VK_SUCCESS) ||
					(vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mRenderingFinishedVkSemaphore) != VK_SUCCESS))
				{
					RHI_LOG(context, CRITICAL, "Failed to create Vulkan semaphore")
				}
			}

			// Acquire next image
			acquireNextImage(false);
		}

		void destroyVulkanSwapChain()
		{
			// Destroy Vulkan swap chain
			if (VK_NULL_HANDLE != mVkRenderPass || !mSwapChainBuffer.empty() || VK_NULL_HANDLE != mVkSwapchainKHR)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
				vkDeviceWaitIdle(vkDevice);
				if (VK_NULL_HANDLE != mVkRenderPass)
				{
					vkDestroyRenderPass(vkDevice, mVkRenderPass, vulkanRhi.getVkAllocationCallbacks());
					mVkRenderPass = VK_NULL_HANDLE;
				}
				if (!mSwapChainBuffer.empty())
				{
					for (const SwapChainBuffer& swapChainBuffer : mSwapChainBuffer)
					{
						vkDestroyFramebuffer(vkDevice, swapChainBuffer.vkFramebuffer, vulkanRhi.getVkAllocationCallbacks());
						vkDestroyImageView(vkDevice, swapChainBuffer.vkImageView, vulkanRhi.getVkAllocationCallbacks());
					}
					mSwapChainBuffer.clear();
				}
				if (VK_NULL_HANDLE != mVkSwapchainKHR)
				{
					vkDestroySwapchainKHR(vkDevice, mVkSwapchainKHR, vulkanRhi.getVkAllocationCallbacks());
					mVkSwapchainKHR = VK_NULL_HANDLE;
				}
				if (VK_NULL_HANDLE != mImageAvailableVkSemaphore)
				{
					vkDestroySemaphore(vulkanRhi.getVulkanContext().getVkDevice(), mImageAvailableVkSemaphore, vulkanRhi.getVkAllocationCallbacks());
					mImageAvailableVkSemaphore = VK_NULL_HANDLE;
				}
				if (VK_NULL_HANDLE != mRenderingFinishedVkSemaphore)
				{
					vkDestroySemaphore(vulkanRhi.getVulkanContext().getVkDevice(), mRenderingFinishedVkSemaphore, vulkanRhi.getVkAllocationCallbacks());
					mRenderingFinishedVkSemaphore = VK_NULL_HANDLE;
				}
			}

			// Destroy depth render target
			destroyDepthRenderTarget();
		}

		void acquireNextImage(bool recreateSwapChainIfNeeded)
		{
			const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			const VkResult vkResult = vkAcquireNextImageKHR(vulkanRhi.getVulkanContext().getVkDevice(), mVkSwapchainKHR, UINT64_MAX, mImageAvailableVkSemaphore, VK_NULL_HANDLE, &mCurrentImageIndex);
			if (VK_SUCCESS != vkResult && VK_SUBOPTIMAL_KHR != vkResult)
			{
				if (VK_ERROR_OUT_OF_DATE_KHR == vkResult)
				{
					// Recreate the Vulkan swap chain
					if (recreateSwapChainIfNeeded)
					{
						createVulkanSwapChain();
					}
				}
				else
				{
					// Error!
					RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to acquire next Vulkan image from swap chain")
				}
			}
		}

		void createDepthRenderTarget(const VkExtent2D& vkExtent2D)
		{
			if (VK_FORMAT_UNDEFINED != mDepthVkFormat)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				Helper::createAndAllocateVkImage(vulkanRhi, 0, VK_IMAGE_TYPE_2D, { vkExtent2D.width, vkExtent2D.height, 1 }, 1, 1, mDepthVkFormat, static_cast<RenderPass&>(getRenderPass()).getVkSampleCountFlagBits(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthVkImage, mDepthVkDeviceMemory);
				Helper::createVkImageView(vulkanRhi, mDepthVkImage, VK_IMAGE_VIEW_TYPE_2D, 1, 1, mDepthVkFormat, VK_IMAGE_ASPECT_DEPTH_BIT, mDepthVkImageView);
				// TODO(co) File "unrimp\source\rhi\private\vulkanrhi\vulkanrhi.cpp" | Line 1036 | Critical: Vulkan debug report callback: Object type: "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT" Object: "103612336" Location: "0" Message code: "461375810" Layer prefix: "Validation" Message: " [ VUID-vkCmdPipelineBarrier-pMemoryBarriers-01185 ] Object: 0x62cffb0 (Type = 6) | vkCmdPipelineBarrier(): pImageMemBarriers[0].dstAccessMask (0x600) is not supported by dstStageMask (0x1). The spec valid usage text states 'Each element of pMemoryBarriers, pBufferMemoryBarriers and pImageMemoryBarriers must not have any access flag included in its dstAccessMask member if that bit is not supported by any of the pipeline stages in dstStageMask, as specified in the table of supported access types.' (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-vkCmdPipelineBarrier-pMemoryBarriers-01185)" 
				//Helper::transitionVkImageLayout(vulkanRhi, mDepthVkImage, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}
		}

		void destroyDepthRenderTarget()
		{
			if (VK_NULL_HANDLE != mDepthVkImage)
			{
				RHI_ASSERT(getRhi().getContext(), VK_NULL_HANDLE != mDepthVkDeviceMemory, "Invalid Vulkan depth device memory")
				RHI_ASSERT(getRhi().getContext(), VK_NULL_HANDLE != mDepthVkImageView, "Invalid Vulkan depth image view")
				Helper::destroyAndFreeVkImage(static_cast<VulkanRhi&>(getRhi()), mDepthVkImage, mDepthVkDeviceMemory, mDepthVkImageView);
			}
		}


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct SwapChainBuffer
		{
			VkImage		  vkImage		= VK_NULL_HANDLE;	///< Vulkan image, don't destroy since we don't own it
			VkImageView   vkImageView	= VK_NULL_HANDLE;	///< Vulkan image view, destroy if no longer needed
			VkFramebuffer vkFramebuffer	= VK_NULL_HANDLE;	///< Vulkan framebuffer, destroy if no longer needed
		};
		typedef std::vector<SwapChainBuffer> SwapChainBuffers;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Operation system window
		Rhi::handle			mNativeWindowHandle;	///< Native window handle window, can be a null handle
		Rhi::IRenderWindow* mRenderWindow;			///< Render window instance, can be a null pointer, don't destroy the instance since we don't own it
		// Vulkan presentation surface
		VkSurfaceKHR mVkSurfaceKHR;	///< Vulkan presentation surface, destroy if no longer needed
		// Vulkan swap chain and color render target related
		VkSwapchainKHR	 mVkSwapchainKHR;				///< Vulkan swap chain, destroy if no longer needed
		VkRenderPass	 mVkRenderPass;					///< Vulkan render pass, destroy if no longer needed (due to "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR" we need an own Vulkan render pass instance)
		SwapChainBuffers mSwapChainBuffer;				///< Swap chain buffer for managing the color render targets
		VkSemaphore		 mImageAvailableVkSemaphore;	///< Vulkan semaphore, destroy if no longer needed
		VkSemaphore		 mRenderingFinishedVkSemaphore;	///< Vulkan semaphore, destroy if no longer needed
		uint32_t		 mCurrentImageIndex;			///< The index of the current Vulkan swap chain image to render into, ~0 if invalid
		// Depth render target related
		VkFormat		mDepthVkFormat;	///< Can be "VK_FORMAT_UNDEFINED" if no depth stencil buffer is needed
		VkImage			mDepthVkImage;
		VkDeviceMemory  mDepthVkDeviceMemory;
		VkImageView		mDepthVkImageView;


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/RenderTarget/Framebuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan framebuffer interface
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
			IFramebuffer(renderPass),
			mNumberOfColorTextures(static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments()),
			mColorTextures(nullptr),	// Set below
			mDepthStencilTexture(nullptr),
			mWidth(UINT_MAX),
			mHeight(UINT_MAX),
			mVkRenderPass(static_cast<RenderPass&>(renderPass).getVkRenderPass()),
			mVkFramebuffer(VK_NULL_HANDLE)
		{
			const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(renderPass.getRhi());

			// Vulkan attachment descriptions and views to fill
			std::vector<VkImageView> vkImageViews;
			vkImageViews.resize(mNumberOfColorTextures + ((nullptr != depthStencilFramebufferAttachment) ? 1u : 0u));
			uint32_t currentVkAttachmentDescriptionIndex = 0;

			// Add a reference to the used color textures
			if (mNumberOfColorTextures > 0)
			{
				mColorTextures = RHI_MALLOC_TYPED(vulkanRhi.getContext(), Rhi::ITexture*, mNumberOfColorTextures);

				// Loop through all color textures
				Rhi::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Rhi::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments)
				{
					// Sanity check
					RHI_ASSERT(vulkanRhi.getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid Vulkan color framebuffer attachment texture")

					// TODO(co) Add security check: Is the given resource one of the currently used RHI?
					*colorTexture = colorFramebufferAttachments->texture;
					(*colorTexture)->addReference();

					// Evaluate the color texture type
					VkImageView vkImageView = VK_NULL_HANDLE;
					switch ((*colorTexture)->getResourceType())
					{
						case Rhi::ResourceType::TEXTURE_2D:
						{
							const Texture2D* texture2D = static_cast<Texture2D*>(*colorTexture);

							// Sanity checks
							RHI_ASSERT(vulkanRhi.getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Vulkan color framebuffer attachment mipmap index")
							RHI_ASSERT(vulkanRhi.getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid Vulkan color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							vkImageView = texture2D->getVkImageView();
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
							break;
						}

						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							vkImageView = texture2DArray->getVkImageView();
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
						case Rhi::ResourceType::INDEX_BUFFER:
						case Rhi::ResourceType::VERTEX_BUFFER:
						case Rhi::ResourceType::TEXTURE_BUFFER:
						case Rhi::ResourceType::STRUCTURED_BUFFER:
						case Rhi::ResourceType::INDIRECT_BUFFER:
						case Rhi::ResourceType::UNIFORM_BUFFER:
						case Rhi::ResourceType::TEXTURE_1D:
						case Rhi::ResourceType::TEXTURE_1D_ARRAY:
						case Rhi::ResourceType::TEXTURE_3D:
						case Rhi::ResourceType::TEXTURE_CUBE:
						case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
						case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
						case Rhi::ResourceType::SAMPLER_STATE:
						case Rhi::ResourceType::VERTEX_SHADER:
						case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
						case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
						case Rhi::ResourceType::GEOMETRY_SHADER:
						case Rhi::ResourceType::FRAGMENT_SHADER:
						case Rhi::ResourceType::COMPUTE_SHADER:
						default:
							// Nothing here
							break;
					}

					// Remember the Vulkan image view
					vkImageViews[currentVkAttachmentDescriptionIndex] = vkImageView;

					// Advance current Vulkan attachment description index
					++currentVkAttachmentDescriptionIndex;
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RHI_ASSERT(vulkanRhi.getContext(), nullptr != mDepthStencilTexture, "Invalid Vulkan depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				VkImageView vkImageView = VK_NULL_HANDLE;
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(vulkanRhi.getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Vulkan depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(vulkanRhi.getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Vulkan depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						vkImageView = texture2D->getVkImageView();
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);
						break;
					}

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						vkImageView = texture2DArray->getVkImageView();
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
					case Rhi::ResourceType::INDEX_BUFFER:
					case Rhi::ResourceType::VERTEX_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::UNIFORM_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::SAMPLER_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
					default:
						// Nothing here
						break;
				}

				// Remember the Vulkan image view
				vkImageViews[currentVkAttachmentDescriptionIndex] = vkImageView;
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RHI_ASSERT(vulkanRhi.getContext(), false, "Invalid Vulkan framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RHI_ASSERT(vulkanRhi.getContext(), false, "Invalid Vulkan framebuffer height")
				mHeight = 1;
			}

			// Create Vulkan framebuffer
			const VkFramebufferCreateInfo vkFramebufferCreateInfo =
			{
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				0,											// flags (VkFramebufferCreateFlags)
				mVkRenderPass,								// renderPass (VkRenderPass)
				static_cast<uint32_t>(vkImageViews.size()),	// attachmentCount (uint32_t)
				vkImageViews.data(),						// pAttachments (const VkImageView*)
				mWidth,										// width (uint32_t)
				mHeight,									// height (uint32_t
				1											// layers (uint32_t)
			};
			if (vkCreateFramebuffer(vulkanRhi.getVulkanContext().getVkDevice(), &vkFramebufferCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkFramebuffer) == VK_SUCCESS)
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != vkDebugMarkerSetObjectNameEXT)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FBO", 6);	// 6 = "FBO: " including terminating zero
						const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
						Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, (uint64_t)mVkRenderPass, detailedDebugName);
						Helper::setDebugObjectName(vkDevice, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, (uint64_t)mVkFramebuffer, detailedDebugName);
					}
				#endif
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create Vulkan framebuffer")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();

			// Destroy Vulkan framebuffer instance
			if (VK_NULL_HANDLE != mVkFramebuffer)
			{
				vkDestroyFramebuffer(vkDevice, mVkFramebuffer, vulkanRhi.getVkAllocationCallbacks());
			}

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
				RHI_FREE(vulkanRhi.getContext(), mColorTextures);
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
		*    Return the Vulkan render pass
		*
		*  @return
		*    The Vulkan render pass
		*/
		[[nodiscard]] inline VkRenderPass getVkRenderPass() const
		{
			return mVkRenderPass;
		}

		/**
		*  @brief
		*    Return the Vulkan framebuffer to render into
		*
		*  @return
		*    The Vulkan framebuffer to render into
		*/
		[[nodiscard]] inline VkFramebuffer getVkFramebuffer() const
		{
			return mVkFramebuffer;
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
		uint32_t		mNumberOfColorTextures;	///< Number of color render target textures
		Rhi::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Rhi::ITexture*  mDepthStencilTexture;	///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t		mWidth;					///< The framebuffer width
		uint32_t		mHeight;				///< The framebuffer height
		VkRenderPass	mVkRenderPass;			///< Vulkan render pass instance, can be a null handle, we don't own it
		VkFramebuffer	mVkFramebuffer;			///< Vulkan framebuffer instance, can be a null handle


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/VertexShaderGlsl.h                   ]
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
		*    Constructor for creating a vertex shader from shader bytecode
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderGlsl(VulkanRhi& vulkanRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5);	// 5 = "VS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderGlsl(VulkanRhi& vulkanRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_VERTEX_BIT, sourceCode, shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5);	// 5 = "VS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyShaderModule(vulkanRhi.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		[[nodiscard]] inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
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
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/TessellationControlShaderGlsl.h      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderGlsl final : public Rhi::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationControlShaderGlsl(VulkanRhi& vulkanRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6);	// 6 = "TCS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationControlShaderGlsl(VulkanRhi& vulkanRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, sourceCode, shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6);	// 6 = "TCS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationControlShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyShaderModule(vulkanRhi.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		[[nodiscard]] inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
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
			RHI_DELETE(getRhi().getContext(), TessellationControlShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShaderGlsl(const TessellationControlShaderGlsl& source) = delete;
		TessellationControlShaderGlsl& operator =(const TessellationControlShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/TessellationEvaluationShaderGlsl.h   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderGlsl final : public Rhi::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader bytecode
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationEvaluationShaderGlsl(VulkanRhi& vulkanRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6);	// 6 = "TES: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) shader from shader source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationEvaluationShaderGlsl(VulkanRhi& vulkanRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, sourceCode, shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6);	// 6 = "TES: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationEvaluationShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyShaderModule(vulkanRhi.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		[[nodiscard]] inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
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
			RHI_DELETE(getRhi().getContext(), TessellationEvaluationShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShaderGlsl(const TessellationEvaluationShaderGlsl& source) = delete;
		TessellationEvaluationShaderGlsl& operator =(const TessellationEvaluationShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/GeometryShaderGlsl.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL geometry shader class
	*/
	class GeometryShaderGlsl final : public Rhi::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader bytecode
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		// TODO(co) Remove unused parameters
		GeometryShaderGlsl(VulkanRhi& vulkanRhi, const Rhi::ShaderBytecode& shaderBytecode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5);	// 5 = "GS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*  @param[in] gsInputPrimitiveTopology
		*    Geometry shader input primitive topology
		*  @param[in] gsOutputPrimitiveTopology
		*    Geometry shader output primitive topology
		*  @param[in] numberOfOutputVertices
		*    Number of output vertices
		*/
		// TODO(co) Remove unused parameters
		GeometryShaderGlsl(VulkanRhi& vulkanRhi, const char* sourceCode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_GEOMETRY_BIT, sourceCode, shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5);	// 5 = "GS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GeometryShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyShaderModule(vulkanRhi.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		[[nodiscard]] inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
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
			RHI_DELETE(getRhi().getContext(), GeometryShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShaderGlsl(const GeometryShaderGlsl& source) = delete;
		GeometryShaderGlsl& operator =(const GeometryShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/FragmentShaderGlsl.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL fragment shader (FS, "pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderGlsl final : public Rhi::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader bytecode
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderGlsl(VulkanRhi& vulkanRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5);	// 5 = "FS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderGlsl(VulkanRhi& vulkanRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, sourceCode, shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5);	// 5 = "FS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~FragmentShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyShaderModule(vulkanRhi.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		[[nodiscard]] inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
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
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/ComputeShaderGlsl.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    GLSL compute shader (CS) class
	*/
	class ComputeShaderGlsl final : public Rhi::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader bytecode
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		ComputeShaderGlsl(VulkanRhi& vulkanRhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromBytecode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5);	// 5 = "CS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		ComputeShaderGlsl(VulkanRhi& vulkanRhi, const char* sourceCode, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(vulkanRhi),
			mVkShaderModule(::detail::createVkShaderModuleFromSourceCode(vulkanRhi.getContext(), vulkanRhi.getVkAllocationCallbacks(), vulkanRhi.getVulkanContext().getVkDevice(), VK_SHADER_STAGE_COMPUTE_BIT, sourceCode, shaderBytecode))
		{
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5);	// 5 = "CS: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, (uint64_t)mVkShaderModule, detailedDebugName);
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputeShaderGlsl() override
		{
			if (VK_NULL_HANDLE != mVkShaderModule)
			{
				const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
				vkDestroyShaderModule(vulkanRhi.getVulkanContext().getVkDevice(), mVkShaderModule, vulkanRhi.getVkAllocationCallbacks());
			}
		}

		/**
		*  @brief
		*    Return the Vulkan shader module
		*
		*  @return
		*    The Vulkan shader module
		*/
		[[nodiscard]] inline VkShaderModule getVkShaderModule() const
		{
			return mVkShaderModule;
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
			RHI_DELETE(getRhi().getContext(), ComputeShaderGlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputeShaderGlsl(const ComputeShaderGlsl& source) = delete;
		ComputeShaderGlsl& operator =(const ComputeShaderGlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VkShaderModule mVkShaderModule;	///< Vulkan shader module, destroy it if you no longer need it


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/GraphicsProgramGlsl.h                ]
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] vertexShaderGlsl
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderGlsl
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderGlsl
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderGlsl
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderGlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		// TODO(co) Remove unused parameters
		GraphicsProgramGlsl(VulkanRhi& vulkanRhi, [[maybe_unused]] const Rhi::IRootSignature& rootSignature, [[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, VertexShaderGlsl *vertexShaderGlsl, TessellationControlShaderGlsl *tessellationControlShaderGlsl, TessellationEvaluationShaderGlsl *tessellationEvaluationShaderGlsl, GeometryShaderGlsl *geometryShaderGlsl, FragmentShaderGlsl *fragmentShaderGlsl) :
			IGraphicsProgram(vulkanRhi),
			mVertexShaderGlsl(vertexShaderGlsl),
			mTessellationControlShaderGlsl(tessellationControlShaderGlsl),
			mTessellationEvaluationShaderGlsl(tessellationEvaluationShaderGlsl),
			mGeometryShaderGlsl(geometryShaderGlsl),
			mFragmentShaderGlsl(fragmentShaderGlsl)
		{
			// Add references to the provided shaders
			if (nullptr != mVertexShaderGlsl)
			{
				mVertexShaderGlsl->addReference();
			}
			if (nullptr != mTessellationControlShaderGlsl)
			{
				mTessellationControlShaderGlsl->addReference();
			}
			if (nullptr != mTessellationEvaluationShaderGlsl)
			{
				mTessellationEvaluationShaderGlsl->addReference();
			}
			if (nullptr != mGeometryShaderGlsl)
			{
				mGeometryShaderGlsl->addReference();
			}
			if (nullptr != mFragmentShaderGlsl)
			{
				mFragmentShaderGlsl->addReference();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsProgramGlsl() override
		{
			// Release the shader references
			if (nullptr != mVertexShaderGlsl)
			{
				mVertexShaderGlsl->releaseReference();
			}
			if (nullptr != mTessellationControlShaderGlsl)
			{
				mTessellationControlShaderGlsl->releaseReference();
			}
			if (nullptr != mTessellationEvaluationShaderGlsl)
			{
				mTessellationEvaluationShaderGlsl->releaseReference();
			}
			if (nullptr != mGeometryShaderGlsl)
			{
				mGeometryShaderGlsl->releaseReference();
			}
			if (nullptr != mFragmentShaderGlsl)
			{
				mFragmentShaderGlsl->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the GLSL vertex shader the graphics program is using
		*
		*  @return
		*    The GLSL vertex shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline VertexShaderGlsl* getVertexShaderGlsl() const
		{
			return mVertexShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL tessellation control shader the graphics program is using
		*
		*  @return
		*    The GLSL tessellation control shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline TessellationControlShaderGlsl* getTessellationControlShaderGlsl() const
		{
			return mTessellationControlShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL tessellation evaluation shader the graphics program is using
		*
		*  @return
		*    The GLSL tessellation evaluation shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline TessellationEvaluationShaderGlsl* getTessellationEvaluationShaderGlsl() const
		{
			return mTessellationEvaluationShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL geometry shader the graphics program is using
		*
		*  @return
		*    The GLSL geometry shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline GeometryShaderGlsl* getGeometryShaderGlsl() const
		{
			return mGeometryShaderGlsl;
		}

		/**
		*  @brief
		*    Return the GLSL fragment shader the graphics program is using
		*
		*  @return
		*    The GLSL fragment shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline FragmentShaderGlsl* getFragmentShaderGlsl() const
		{
			return mFragmentShaderGlsl;
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
		VertexShaderGlsl*				  mVertexShaderGlsl;					///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderGlsl*	  mTessellationControlShaderGlsl;		///< Tessellation control shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderGlsl* mTessellationEvaluationShaderGlsl;	///< Tessellation evaluation shader the graphics program is using (we keep a reference to it), can be a null pointer
		GeometryShaderGlsl*				  mGeometryShaderGlsl;					///< Geometry shader the graphics program is using (we keep a reference to it), can be a null pointer
		FragmentShaderGlsl*				  mFragmentShaderGlsl;					///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/Shader/ShaderLanguageGlsl.h                 ]
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*/
		inline explicit ShaderLanguageGlsl(VulkanRhi& vulkanRhi) :
			IShaderLanguage(vulkanRhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageGlsl() override
		{
			// De-initialize glslang, if necessary
			#ifdef RHI_VULKAN_GLSLTOSPIRV
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

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromBytecode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Vulkan vertex shader bytecode is invalid")

			// Create shader instance
			return RHI_NEW(vulkanRhi.getContext(), VertexShaderGlsl)(vulkanRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), VertexShaderGlsl)(vulkanRhi, shaderSourceCode.sourceCode, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Vulkan tessellation control shader bytecode is invalid")

			// Create shader instance
			return RHI_NEW(vulkanRhi.getContext(), TessellationControlShaderGlsl)(vulkanRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), TessellationControlShaderGlsl)(vulkanRhi, shaderSourceCode.sourceCode, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Vulkan tessellation evaluation shader bytecode is invalid")

			// Create shader instance
			return RHI_NEW(vulkanRhi.getContext(), TessellationEvaluationShaderGlsl)(vulkanRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), TessellationEvaluationShaderGlsl)(vulkanRhi, shaderSourceCode.sourceCode, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Vulkan geometry shader bytecode is invalid")

			// Create shader instance
			return RHI_NEW(vulkanRhi.getContext(), GeometryShaderGlsl)(vulkanRhi, shaderBytecode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, uint32_t numberOfOutputVertices, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), GeometryShaderGlsl)(vulkanRhi, shaderSourceCode.sourceCode, gsInputPrimitiveTopology, gsOutputPrimitiveTopology, numberOfOutputVertices, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Vulkan fragment shader bytecode is invalid")

			// Create shader instance
			return RHI_NEW(vulkanRhi.getContext(), FragmentShaderGlsl)(vulkanRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), FragmentShaderGlsl)(vulkanRhi, shaderSourceCode.sourceCode, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(vulkanRhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Vulkan compute shader bytecode is invalid")

			// Create shader instance
			return RHI_NEW(vulkanRhi.getContext(), ComputeShaderGlsl)(vulkanRhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			return RHI_NEW(vulkanRhi.getContext(), ComputeShaderGlsl)(vulkanRhi, shaderSourceCode.sourceCode, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram(const Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, Rhi::ITessellationControlShader* tessellationControlShader, Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER) override
		{
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(vulkanRhi.getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::GLSL_NAME, "Vulkan vertex shader language mismatch")
			RHI_ASSERT(vulkanRhi.getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::GLSL_NAME, "Vulkan tessellation control shader language mismatch")
			RHI_ASSERT(vulkanRhi.getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::GLSL_NAME, "Vulkan tessellation evaluation shader language mismatch")
			RHI_ASSERT(vulkanRhi.getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::GLSL_NAME, "Vulkan geometry shader language mismatch")
			RHI_ASSERT(vulkanRhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::GLSL_NAME, "Vulkan fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(vulkanRhi.getContext(), GraphicsProgramGlsl)(vulkanRhi, rootSignature, vertexAttributes, static_cast<VertexShaderGlsl*>(vertexShader), static_cast<TessellationControlShaderGlsl*>(tessellationControlShader), static_cast<TessellationEvaluationShaderGlsl*>(tessellationEvaluationShader), static_cast<GeometryShaderGlsl*>(geometryShader), static_cast<FragmentShaderGlsl*>(fragmentShader));
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
	//[ VulkanRhi/State/GraphicsPipelineState.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan graphics pipeline state class
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
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(VulkanRhi& vulkanRhi, const Rhi::GraphicsPipelineState& graphicsPipelineState, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsPipelineState(vulkanRhi, id),
			mRootSignature(graphicsPipelineState.rootSignature),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mVkPipeline(VK_NULL_HANDLE)
		{
			// Add a reference to the referenced RHI resources
			mRootSignature->addReference();
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			// Our pipeline state needs to be independent of concrete render targets, so we're using dynamic viewport ("VK_DYNAMIC_STATE_VIEWPORT") and scissor ("VK_DYNAMIC_STATE_SCISSOR") states
			static constexpr uint32_t WIDTH  = 42;
			static constexpr uint32_t HEIGHT = 42;

			// Shaders
			GraphicsProgramGlsl* graphicsProgramGlsl = static_cast<GraphicsProgramGlsl*>(mGraphicsProgram);
			uint32_t stageCount = 0;
			::detail::VkPipelineShaderStageCreateInfos vkPipelineShaderStageCreateInfos;
			{
				// Define helper macro
				#define SHADER_STAGE(vkShaderStageFlagBits, shaderGlsl) \
					if (nullptr != shaderGlsl) \
					{ \
						::detail::addVkPipelineShaderStageCreateInfo(vkShaderStageFlagBits, shaderGlsl->getVkShaderModule(), vkPipelineShaderStageCreateInfos, stageCount); \
						++stageCount; \
					}

				// Shader stages
				SHADER_STAGE(VK_SHADER_STAGE_VERTEX_BIT,				  graphicsProgramGlsl->getVertexShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	  graphicsProgramGlsl->getTessellationControlShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, graphicsProgramGlsl->getTessellationEvaluationShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_GEOMETRY_BIT,				  graphicsProgramGlsl->getGeometryShaderGlsl())
				SHADER_STAGE(VK_SHADER_STAGE_FRAGMENT_BIT,				  graphicsProgramGlsl->getFragmentShaderGlsl())

				// Undefine helper macro
				#undef SHADER_STAGE
			}

			// Vertex attributes
			const uint32_t numberOfAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
			std::vector<VkVertexInputBindingDescription> vkVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions(numberOfAttributes);
			for (uint32_t attribute = 0; attribute < numberOfAttributes; ++attribute)
			{
				const Rhi::VertexAttribute* attributes = &graphicsPipelineState.vertexAttributes.attributes[attribute];
				const uint32_t inputSlot = attributes->inputSlot;

				{ // Map to Vulkan vertex input binding description
					if (vkVertexInputBindingDescriptions.size() <= inputSlot)
					{
						vkVertexInputBindingDescriptions.resize(inputSlot + 1);
					}
					VkVertexInputBindingDescription& vkVertexInputBindingDescription = vkVertexInputBindingDescriptions[inputSlot];
					vkVertexInputBindingDescription.binding   = inputSlot;
					vkVertexInputBindingDescription.stride    = attributes->strideInBytes;
					vkVertexInputBindingDescription.inputRate = (attributes->instancesPerElement > 0) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				}

				{ // Map to Vulkan vertex input attribute description
					VkVertexInputAttributeDescription& vkVertexInputAttributeDescription = vkVertexInputAttributeDescriptions[attribute];
					vkVertexInputAttributeDescription.location = attribute;
					vkVertexInputAttributeDescription.binding  = inputSlot;
					vkVertexInputAttributeDescription.format   = Mapping::getVulkanFormat(attributes->vertexAttributeFormat);
					vkVertexInputAttributeDescription.offset   = attributes->alignedByteOffset;
				}
			}

			// Create the Vulkan graphics pipeline
			// TODO(co) Implement the rest of the value mappings
			const VkPipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,			// sType (VkStructureType)
				nullptr,															// pNext (const void*)
				0,																	// flags (VkPipelineVertexInputStateCreateFlags)
				static_cast<uint32_t>(vkVertexInputBindingDescriptions.size()),		// vertexBindingDescriptionCount (uint32_t)
				vkVertexInputBindingDescriptions.data(),							// pVertexBindingDescriptions (const VkVertexInputBindingDescription*)
				static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size()),	// vertexAttributeDescriptionCount (uint32_t)
				vkVertexInputAttributeDescriptions.data()							// pVertexAttributeDescriptions (const VkVertexInputAttributeDescription*)
			};
			const VkPipelineInputAssemblyStateCreateInfo vkPipelineInputAssemblyStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,		// sType (VkStructureType)
				nullptr,															// pNext (const void*)
				0,																	// flags (VkPipelineInputAssemblyStateCreateFlags)
				Mapping::getVulkanType(graphicsPipelineState.primitiveTopology),	// topology (VkPrimitiveTopology)
				VK_FALSE															// primitiveRestartEnable (VkBool32)
			};
			const VkViewport vkViewport =
			{
				0.0f,						// x (float)
				0.0f,						// y (float)
				static_cast<float>(WIDTH),	// width (float)
				static_cast<float>(HEIGHT),	// height (float)
				0.0f,						// minDepth (float)
				1.0f						// maxDepth (float)
			};
			const VkRect2D scissorVkRect2D =
			{
				{ // offset (VkOffset2D)
					0,	// x (int32_t)
					0	// y (int32_t)
				},
				{ // extent (VkExtent2D)
					WIDTH,	// width (uint32_t)
					HEIGHT	// height (uint32_t)
				}
			};
			const VkPipelineTessellationStateCreateInfo vkPipelineTessellationStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,																																									// sType (VkStructureType)
				nullptr,																																																					// pNext (const void*)
				0,																																																							// flags (VkPipelineTessellationStateCreateFlags)
				(graphicsPipelineState.primitiveTopology >= Rhi::PrimitiveTopology::PATCH_LIST_1) ? static_cast<uint32_t>(graphicsPipelineState.primitiveTopology) - static_cast<uint32_t>(Rhi::PrimitiveTopology::PATCH_LIST_1) + 1 : 1	// patchControlPoints (uint32_t)
			};
			const VkPipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkPipelineViewportStateCreateFlags)
				1,														// viewportCount (uint32_t)
				&vkViewport,											// pViewports (const VkViewport*)
				1,														// scissorCount (uint32_t)
				&scissorVkRect2D										// pScissors (const VkRect2D*)
			};
			const float depthBias = static_cast<float>(graphicsPipelineState.rasterizerState.depthBias);
			const float depthBiasClamp = graphicsPipelineState.rasterizerState.depthBiasClamp;
			const float slopeScaledDepthBias = graphicsPipelineState.rasterizerState.slopeScaledDepthBias;
			const VkPipelineRasterizationStateCreateInfo vkPipelineRasterizationStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,																		// sType (VkStructureType)
				nullptr,																														// pNext (const void*)
				0,																																// flags (VkPipelineRasterizationStateCreateFlags)
				static_cast<VkBool32>(graphicsPipelineState.rasterizerState.depthClipEnable),													// depthClampEnable (VkBool32)
				VK_FALSE,																														// rasterizerDiscardEnable (VkBool32)
				(Rhi::FillMode::WIREFRAME == graphicsPipelineState.rasterizerState.fillMode) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,		// polygonMode (VkPolygonMode)
				static_cast<VkCullModeFlags>(static_cast<int>(graphicsPipelineState.rasterizerState.cullMode) - 1),								// cullMode (VkCullModeFlags)
				(1 == graphicsPipelineState.rasterizerState.frontCounterClockwise) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,	// frontFace (VkFrontFace)
				static_cast<VkBool32>(0.0f != depthBias || 0.0f != depthBiasClamp || 0.0f != slopeScaledDepthBias),								// depthBiasEnable (VkBool32)
				depthBias,																														// depthBiasConstantFactor (float)
				depthBiasClamp,																													// depthBiasClamp (float)
				slopeScaledDepthBias,																											// depthBiasSlopeFactor (float)
				1.0f																															// lineWidth (float)
			};
			const RenderPass* renderPass = static_cast<const RenderPass*>(mRenderPass);
			const VkPipelineMultisampleStateCreateInfo vkPipelineMultisampleStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,													// pNext (const void*)
				0,															// flags (VkPipelineMultisampleStateCreateFlags)
				renderPass->getVkSampleCountFlagBits(),						// rasterizationSamples (VkSampleCountFlagBits)
				VK_FALSE,													// sampleShadingEnable (VkBool32)
				0.0f,														// minSampleShading (float)
				nullptr,													// pSampleMask (const VkSampleMask*)
				VK_FALSE,													// alphaToCoverageEnable (VkBool32)
				VK_FALSE													// alphaToOneEnable (VkBool32)
			};
			const VkPipelineDepthStencilStateCreateInfo vkPipelineDepthStencilStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,													// sType (VkStructureType)
				nullptr,																									// pNext (const void*)
				0,																											// flags (VkPipelineDepthStencilStateCreateFlags)
				static_cast<VkBool32>(0 != graphicsPipelineState.depthStencilState.depthEnable),							// depthTestEnable (VkBool32)
				static_cast<VkBool32>(Rhi::DepthWriteMask::ALL == graphicsPipelineState.depthStencilState.depthWriteMask),	// depthWriteEnable (VkBool32)
				Mapping::getVulkanComparisonFunc(graphicsPipelineState.depthStencilState.depthFunc),						// depthCompareOp (VkCompareOp)
				VK_FALSE,																									// depthBoundsTestEnable (VkBool32)
				static_cast<VkBool32>(0 != graphicsPipelineState.depthStencilState.stencilEnable),							// stencilTestEnable (VkBool32)
				{ // front (VkStencilOpState)
					VK_STENCIL_OP_KEEP,																						// failOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																						// passOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																						// depthFailOp (VkStencilOp)
					VK_COMPARE_OP_NEVER,																					// compareOp (VkCompareOp)
					0,																										// compareMask (uint32_t)
					0,																										// writeMask (uint32_t)
					0																										// reference (uint32_t)
				},
				{ // back (VkStencilOpState)
					VK_STENCIL_OP_KEEP,																						// failOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																						// passOp (VkStencilOp)
					VK_STENCIL_OP_KEEP,																						// depthFailOp (VkStencilOp)
					VK_COMPARE_OP_NEVER,																					// compareOp (VkCompareOp)
					0,																										// compareMask (uint32_t)
					0,																										// writeMask (uint32_t)
					0																										// reference (uint32_t)
				},
				0.0f,																										// minDepthBounds (float)
				1.0f																										// maxDepthBounds (float)
			};
			const uint32_t numberOfColorAttachments = renderPass->getNumberOfColorAttachments();
			RHI_ASSERT(vulkanRhi.getContext(), numberOfColorAttachments < 8, "Invalid number of Vulkan color attachments")
			RHI_ASSERT(vulkanRhi.getContext(), numberOfColorAttachments == graphicsPipelineState.numberOfRenderTargets, "Invalid number of Vulkan color attachments")
			std::array<VkPipelineColorBlendAttachmentState, 8> vkPipelineColorBlendAttachmentStates;
			for (uint8_t i = 0; i < numberOfColorAttachments; ++i)
			{
				const Rhi::RenderTargetBlendDesc& renderTargetBlendDesc = graphicsPipelineState.blendState.renderTarget[i];
				VkPipelineColorBlendAttachmentState& vkPipelineColorBlendAttachmentState = vkPipelineColorBlendAttachmentStates[i];
				vkPipelineColorBlendAttachmentState.blendEnable			= static_cast<VkBool32>(renderTargetBlendDesc.blendEnable);				// blendEnable (VkBool32)
				vkPipelineColorBlendAttachmentState.srcColorBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.srcBlend);		// srcColorBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.dstColorBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.destBlend);		// dstColorBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.colorBlendOp		= Mapping::getVulkanBlendOp(renderTargetBlendDesc.blendOp);				// colorBlendOp (VkBlendOp)
				vkPipelineColorBlendAttachmentState.srcAlphaBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.srcBlendAlpha);	// srcAlphaBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.dstAlphaBlendFactor	= Mapping::getVulkanBlendFactor(renderTargetBlendDesc.destBlendAlpha);	// dstAlphaBlendFactor (VkBlendFactor)
				vkPipelineColorBlendAttachmentState.alphaBlendOp		= Mapping::getVulkanBlendOp(renderTargetBlendDesc.blendOpAlpha);		// alphaBlendOp (VkBlendOp)
				vkPipelineColorBlendAttachmentState.colorWriteMask		= renderTargetBlendDesc.renderTargetWriteMask;							// colorWriteMask (VkColorComponentFlags)
			}
			const VkPipelineColorBlendStateCreateInfo vkPipelineColorBlendStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,													// pNext (const void*)
				0,															// flags (VkPipelineColorBlendStateCreateFlags)
				VK_FALSE,													// logicOpEnable (VkBool32)
				VK_LOGIC_OP_COPY,											// logicOp (VkLogicOp)
				numberOfColorAttachments,									// attachmentCount (uint32_t)
				vkPipelineColorBlendAttachmentStates.data(),				// pAttachments (const VkPipelineColorBlendAttachmentState*)
				{ 0.0f, 0.0f, 0.0f, 0.0f }									// blendConstants[4] (float)
			};
			static constexpr std::array<VkDynamicState, 2> vkDynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			static const VkPipelineDynamicStateCreateInfo vkPipelineDynamicStateCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,	// sType (VkStructureType)
				nullptr,												// pNext (const void*)
				0,														// flags (VkPipelineDynamicStateCreateFlags)
				static_cast<uint32_t>(vkDynamicStates.size()),			// dynamicStateCount (uint32_t)
				vkDynamicStates.data()									// pDynamicStates (const VkDynamicState*)
			};
			const VkGraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo =
			{
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,					// sType (VkStructureType)
				nullptr,															// pNext (const void*)
				0,																	// flags (VkPipelineCreateFlags)
				stageCount,															// stageCount (uint32_t)
				vkPipelineShaderStageCreateInfos.data(),							// pStages (const VkPipelineShaderStageCreateInfo*)
				&vkPipelineVertexInputStateCreateInfo,								// pVertexInputState (const VkPipelineVertexInputStateCreateInfo*)
				&vkPipelineInputAssemblyStateCreateInfo,							// pInputAssemblyState (const VkPipelineInputAssemblyStateCreateInfo*)
				&vkPipelineTessellationStateCreateInfo,								// pTessellationState (const VkPipelineTessellationStateCreateInfo*)
				&vkPipelineViewportStateCreateInfo,									// pViewportState (const VkPipelineViewportStateCreateInfo*)
				&vkPipelineRasterizationStateCreateInfo,							// pRasterizationState (const VkPipelineRasterizationStateCreateInfo*)
				&vkPipelineMultisampleStateCreateInfo,								// pMultisampleState (const VkPipelineMultisampleStateCreateInfo*)
				&vkPipelineDepthStencilStateCreateInfo,								// pDepthStencilState (const VkPipelineDepthStencilStateCreateInfo*)
				&vkPipelineColorBlendStateCreateInfo,								// pColorBlendState (const VkPipelineColorBlendStateCreateInfo*)
				&vkPipelineDynamicStateCreateInfo,									// pDynamicState (const VkPipelineDynamicStateCreateInfo*)
				static_cast<RootSignature*>(mRootSignature)->getVkPipelineLayout(),	// layout (VkPipelineLayout)
				renderPass->getVkRenderPass(),										// renderPass (VkRenderPass)
				0,																	// subpass (uint32_t)
				VK_NULL_HANDLE,														// basePipelineHandle (VkPipeline)
				0																	// basePipelineIndex (int32_t)
			};
			if (vkCreateGraphicsPipelines(vulkanRhi.getVulkanContext().getVkDevice(), VK_NULL_HANDLE, 1, &vkGraphicsPipelineCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkPipeline) == VK_SUCCESS)
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != vkDebugMarkerSetObjectNameEXT)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics PSO", 15);	// 15 = "Graphics PSO: " including terminating zero
						Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, (uint64_t)mVkPipeline, detailedDebugName);
					}
				#endif
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan graphics pipeline")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsPipelineState() override
		{
			// Destroy the Vulkan graphics pipeline
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			if (VK_NULL_HANDLE != mVkPipeline)
			{
				vkDestroyPipeline(vulkanRhi.getVulkanContext().getVkDevice(), mVkPipeline, vulkanRhi.getVkAllocationCallbacks());
			}

			// Release referenced RHI resources
			mRootSignature->releaseReference();
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Free the unique compact graphics pipeline state ID
			vulkanRhi.GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the Vulkan graphics pipeline
		*
		*  @return
		*    The Vulkan graphics pipeline
		*/
		[[nodiscard]] inline VkPipeline getVkPipeline() const
		{
			return mVkPipeline;
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
		Rhi::IRootSignature*   mRootSignature;
		Rhi::IGraphicsProgram* mGraphicsProgram;
		Rhi::IRenderPass*	   mRenderPass;
		VkPipeline			   mVkPipeline;	///< The Vulkan graphics pipeline


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/State/ComputePipelineState.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan compute pipeline state class
	*/
	class ComputePipelineState final : public Rhi::IComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] vulkanRhi
		*    Owner Vulkan RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*  @param[in] computeShader
		*    Compute shader to use
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		ComputePipelineState(VulkanRhi& vulkanRhi, Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputePipelineState(vulkanRhi, id),
			mRootSignature(rootSignature),
			mComputeShader(computeShader),
			mVkPipeline(VK_NULL_HANDLE)
		{
			// Add a reference to the given root signature and compute shader
			rootSignature.addReference();
			computeShader.addReference();

			// Create the Vulkan compute pipeline
			const VkComputePipelineCreateInfo vkComputePipelineCreateInfo =
			{
				VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,							// sType (VkStructureType)
				nullptr,																// pNext (const void*)
				0,																		// flags (VkPipelineCreateFlags)
				{																		// stage (VkPipelineShaderStageCreateInfo)
					VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,				// sType (VkStructureType)
					nullptr,															// pNext (const void*)
					0,																	// flags (VkPipelineShaderStageCreateFlags)
					VK_SHADER_STAGE_COMPUTE_BIT,										// stage (VkShaderStageFlagBits)
					static_cast<ComputeShaderGlsl&>(computeShader).getVkShaderModule(),	// module (VkShaderModule)
					"main",																// pName (const char*)
					nullptr																// pSpecializationInfo (const VkSpecializationInfo*)
				},
				static_cast<RootSignature&>(rootSignature).getVkPipelineLayout(),		// layout (VkPipelineLayout)
				VK_NULL_HANDLE,															// basePipelineHandle (VkPipeline)
				0																		// basePipelineIndex (int32_t)
			};
			if (vkCreateComputePipelines(vulkanRhi.getVulkanContext().getVkDevice(), VK_NULL_HANDLE, 1, &vkComputePipelineCreateInfo, vulkanRhi.getVkAllocationCallbacks(), &mVkPipeline) == VK_SUCCESS)
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != vkDebugMarkerSetObjectNameEXT)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Compute PSO", 14);	// 14 = "Compute PSO: " including terminating zero
						Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, (uint64_t)mVkPipeline, detailedDebugName);
					}
				#endif
			}
			else
			{
				RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Failed to create the Vulkan compute pipeline")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputePipelineState() override
		{
			// Destroy the Vulkan compute pipeline
			VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			if (VK_NULL_HANDLE != mVkPipeline)
			{
				vkDestroyPipeline(vulkanRhi.getVulkanContext().getVkDevice(), mVkPipeline, vulkanRhi.getVkAllocationCallbacks());
			}

			// Release the root signature and compute shader reference
			mRootSignature.releaseReference();
			mComputeShader.releaseReference();

			// Free the unique compact compute pipeline state ID
			vulkanRhi.ComputePipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the Vulkan compute pipeline
		*
		*  @return
		*    The Vulkan compute pipeline
		*/
		[[nodiscard]] inline VkPipeline getVkPipeline() const
		{
			return mVkPipeline;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputePipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineState(const ComputePipelineState& source) = delete;
		ComputePipelineState& operator =(const ComputePipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::IRootSignature& mRootSignature;
		Rhi::IComputeShader& mComputeShader;
		VkPipeline			 mVkPipeline;		///< The Vulkan compute pipeline


	};




	//[-------------------------------------------------------]
	//[ VulkanRhi/ResourceGroup.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Vulkan resource group class
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
		*  @param[in] rootSignature
		*    Root signature
		*  @param[in] rootParameterIndex
		*    Root parameter index
		*  @param[in] vkDescriptorSet
		*    Wrapped Vulkan descriptor set
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(RootSignature& rootSignature, uint32_t rootParameterIndex, VkDescriptorSet vkDescriptorSet, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IResourceGroup(static_cast<VulkanRhi&>(rootSignature.getRhi())),
			mRootSignature(rootSignature),
			mVkDescriptorSet(vkDescriptorSet),
			mNumberOfResources(numberOfResources),
			mResources(RHI_MALLOC_TYPED(rootSignature.getRhi().getContext(), Rhi::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr)
		{
			mRootSignature.addReference();

			// Process all resources and add our reference to the RHI resource
			const VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
			const VkDevice vkDevice = vulkanRhi.getVulkanContext().getVkDevice();
			if (nullptr != samplerStates)
			{
				mSamplerStates = RHI_MALLOC_TYPED(vulkanRhi.getContext(), Rhi::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Rhi::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->addReference();
					}
				}
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Rhi::IResource* resource = *resources;
				RHI_ASSERT(vulkanRhi.getContext(), nullptr != resource, "Invalid Vulkan resource")
				mResources[resourceIndex] = resource;
				resource->addReference();

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Rhi::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Rhi::ResourceType::INDEX_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<IndexBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,													// offset (VkDeviceSize)
							VK_WHOLE_SIZE										// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							mVkDescriptorSet,							// dstSet (VkDescriptorSet)
							resourceIndex,								// dstBinding (uint32_t)
							0,											// dstArrayElement (uint32_t)
							1,											// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			// descriptorType (VkDescriptorType)
							nullptr,									// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,					// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::VERTEX_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<VertexBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,														// offset (VkDeviceSize)
							VK_WHOLE_SIZE											// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							mVkDescriptorSet,							// dstSet (VkDescriptorSet)
							resourceIndex,								// dstBinding (uint32_t)
							0,											// dstArrayElement (uint32_t)
							1,											// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			// descriptorType (VkDescriptorType)
							nullptr,									// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,					// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::TEXTURE_BUFFER:
					{
						const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootSignature.getRootSignature().parameters[rootParameterIndex].descriptorTable.descriptorRanges)[resourceIndex];
						RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "Vulkan texture buffer must bound at SRV or UAV descriptor range type")
						const VkBufferView vkBufferView = static_cast<TextureBuffer*>(resource)->getVkBufferView();
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,																												// sType (VkStructureType)
							nullptr,																																			// pNext (const void*)
							mVkDescriptorSet,																																	// dstSet (VkDescriptorSet)
							resourceIndex,																																		// dstBinding (uint32_t)
							0,																																					// dstArrayElement (uint32_t)
							1,																																					// descriptorCount (uint32_t)
							(Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType) ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,	// descriptorType (VkDescriptorType)
							nullptr,																																			// pImageInfo (const VkDescriptorImageInfo*)
							nullptr,																																			// pBufferInfo (const VkDescriptorBufferInfo*)
							&vkBufferView																																		// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::STRUCTURED_BUFFER:
					{
						[[maybe_unused]] const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootSignature.getRootSignature().parameters[rootParameterIndex].descriptorTable.descriptorRanges)[resourceIndex];
						RHI_ASSERT(vulkanRhi.getContext(), Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "Vulkan structured buffer must bound at SRV or UAV descriptor range type")
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<StructuredBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,															// offset (VkDeviceSize)
							VK_WHOLE_SIZE												// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// sType (VkStructureType)
							nullptr,								// pNext (const void*)
							mVkDescriptorSet,						// dstSet (VkDescriptorSet)
							resourceIndex,							// dstBinding (uint32_t)
							0,										// dstArrayElement (uint32_t)
							1,										// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		// descriptorType (VkDescriptorType)
							nullptr,								// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,				// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr									// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::INDIRECT_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<IndirectBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,														// offset (VkDeviceSize)
							VK_WHOLE_SIZE											// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,		// sType (VkStructureType)
							nullptr,									// pNext (const void*)
							mVkDescriptorSet,							// dstSet (VkDescriptorSet)
							resourceIndex,								// dstBinding (uint32_t)
							0,											// dstArrayElement (uint32_t)
							1,											// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			// descriptorType (VkDescriptorType)
							nullptr,									// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,					// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::UNIFORM_BUFFER:
					{
						const VkDescriptorBufferInfo vkDescriptorBufferInfo =
						{
							static_cast<UniformBuffer*>(resource)->getVkBuffer(),	// buffer (VkBuffer)
							0,														// offset (VkDeviceSize)
							VK_WHOLE_SIZE											// range (VkDeviceSize)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,	// sType (VkStructureType)
							nullptr,								// pNext (const void*)
							mVkDescriptorSet,						// dstSet (VkDescriptorSet)
							resourceIndex,							// dstBinding (uint32_t)
							0,										// dstArrayElement (uint32_t)
							1,										// descriptorCount (uint32_t)
							VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		// descriptorType (VkDescriptorType)
							nullptr,								// pImageInfo (const VkDescriptorImageInfo*)
							&vkDescriptorBufferInfo,				// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr									// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_2D:
					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					{
						// Evaluate the texture type and get the Vulkan image view
						VkImageView vkImageView = VK_NULL_HANDLE;
						VkImageLayout vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						switch (resourceType)
						{
							case Rhi::ResourceType::TEXTURE_1D:
							{
								const Texture1D* texture1D = static_cast<Texture1D*>(resource);
								vkImageView = texture1D->getVkImageView();
								vkImageLayout = texture1D->getVkImageLayout();
								break;
							}

							case Rhi::ResourceType::TEXTURE_1D_ARRAY:
							{
								const Texture1DArray* texture1DArray = static_cast<Texture1DArray*>(resource);
								vkImageView = texture1DArray->getVkImageView();
								vkImageLayout = texture1DArray->getVkImageLayout();
								break;
							}

							case Rhi::ResourceType::TEXTURE_2D:
							{
								const Texture2D* texture2D = static_cast<Texture2D*>(resource);
								vkImageView = texture2D->getVkImageView();
								vkImageLayout = texture2D->getVkImageLayout();
								break;
							}

							case Rhi::ResourceType::TEXTURE_2D_ARRAY:
							{
								const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(resource);
								vkImageView = texture2DArray->getVkImageView();
								vkImageLayout = texture2DArray->getVkImageLayout();
								break;
							}

							case Rhi::ResourceType::TEXTURE_3D:
							{
								const Texture3D* texture3D = static_cast<Texture3D*>(resource);
								vkImageView = texture3D->getVkImageView();
								vkImageLayout = texture3D->getVkImageLayout();
								break;
							}

							case Rhi::ResourceType::TEXTURE_CUBE:
							{
								const TextureCube* textureCube = static_cast<TextureCube*>(resource);
								vkImageView = textureCube->getVkImageView();
								vkImageLayout = textureCube->getVkImageLayout();
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
							case Rhi::ResourceType::INDEX_BUFFER:
							case Rhi::ResourceType::VERTEX_BUFFER:
							case Rhi::ResourceType::INDIRECT_BUFFER:
							case Rhi::ResourceType::TEXTURE_BUFFER:
							case Rhi::ResourceType::STRUCTURED_BUFFER:
							case Rhi::ResourceType::UNIFORM_BUFFER:
							case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
							case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
							case Rhi::ResourceType::SAMPLER_STATE:
							case Rhi::ResourceType::VERTEX_SHADER:
							case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
							case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
							case Rhi::ResourceType::GEOMETRY_SHADER:
							case Rhi::ResourceType::FRAGMENT_SHADER:
							case Rhi::ResourceType::COMPUTE_SHADER:
								RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Invalid Vulkan RHI implementation resource type")
								break;
						}

						// Get the sampler state
						const SamplerState* samplerState = (nullptr != mSamplerStates) ? static_cast<const SamplerState*>(mSamplerStates[resourceIndex]) : nullptr;

						// Update Vulkan descriptor sets
						const VkDescriptorImageInfo vkDescriptorImageInfo =
						{
							(nullptr != samplerState) ? samplerState->getVkSampler() : VK_NULL_HANDLE,	// sampler (VkSampler)
							vkImageView,																// imageView (VkImageView)
							vkImageLayout																// imageLayout (VkImageLayout)
						};
						const VkWriteDescriptorSet vkWriteDescriptorSet =
						{
							VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,																		// sType (VkStructureType)
							nullptr,																									// pNext (const void*)
							mVkDescriptorSet,																							// dstSet (VkDescriptorSet)
							resourceIndex,																								// dstBinding (uint32_t)
							0,																											// dstArrayElement (uint32_t)
							1,																											// descriptorCount (uint32_t)
							(nullptr != samplerState) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	// descriptorType (VkDescriptorType)
							&vkDescriptorImageInfo,																						// pImageInfo (const VkDescriptorImageInfo*)
							nullptr,																									// pBufferInfo (const VkDescriptorBufferInfo*)
							nullptr																										// pTexelBufferView (const VkBufferView*)
						};
						vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
						break;
					}

					case Rhi::ResourceType::SAMPLER_STATE:
						// Nothing to do in here, Vulkan is using combined image samplers
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
					case Rhi::ResourceType::COMPUTE_SHADER:
						RHI_LOG(vulkanRhi.getContext(), CRITICAL, "Invalid Vulkan RHI implementation resource type")
						break;
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != vkDebugMarkerSetObjectNameEXT)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Resource group", 17);	// 17 = "Resource group: " including terminating zero
					Helper::setDebugObjectName(vulkanRhi.getVulkanContext().getVkDevice(), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)mVkDescriptorSet, detailedDebugName);
				}
			#endif
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

			// Free Vulkan descriptor set
			if (VK_NULL_HANDLE != mVkDescriptorSet)
			{
				vkFreeDescriptorSets(static_cast<VulkanRhi&>(mRootSignature.getRhi()).getVulkanContext().getVkDevice(), mRootSignature.getVkDescriptorPool(), 1, &mVkDescriptorSet);
			}
			mRootSignature.releaseReference();
		}

		/**
		*  @brief
		*    Return the Vulkan descriptor set
		*
		*  @return
		*    The Vulkan descriptor set, can be a null handle
		*/
		[[nodiscard]] inline VkDescriptorSet getVkDescriptorSet() const
		{
			return mVkDescriptorSet;
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
		RootSignature&		 mRootSignature;		///< Root signature
		VkDescriptorSet		 mVkDescriptorSet;		///< "mVkDescriptorPool" of the root signature is the owner which manages the memory, can be a null handle (e.g. for a sampler resource group)
		uint32_t			 mNumberOfResources;	///< Number of resources this resource group groups together
		Rhi::IResource**	 mResources;			///< RHI resource, we keep a reference to it
		Rhi::ISamplerState** mSamplerStates;		///< Sampler states, we keep a reference to it


	};

	// TODO(co) Try to somehow simplify the internal dependencies to be able to put this method directly into the class
	Rhi::IResourceGroup* RootSignature::createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		VulkanRhi& vulkanRhi = static_cast<VulkanRhi&>(getRhi());
		const Rhi::Context& context = vulkanRhi.getContext();

		// Sanity checks
		RHI_ASSERT(context, VK_NULL_HANDLE != mVkDescriptorPool, "The Vulkan descriptor pool instance must be valid")
		RHI_ASSERT(context, rootParameterIndex < mVkDescriptorSetLayouts.size(), "The Vulkan root parameter index is out-of-bounds")
		RHI_ASSERT(context, numberOfResources > 0, "The number of Vulkan resources must not be zero")
		RHI_ASSERT(context, nullptr != resources, "The Vulkan resource pointers must be valid")

		// Allocate Vulkan descriptor set
		VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
		if ((*resources)->getResourceType() != Rhi::ResourceType::SAMPLER_STATE)
		{
			const VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo =
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,	// sType (VkStructureType)
				nullptr,										// pNext (const void*)
				mVkDescriptorPool,								// descriptorPool (VkDescriptorPool)
				1,												// descriptorSetCount (uint32_t)
				&mVkDescriptorSetLayouts[rootParameterIndex]	// pSetLayouts (const VkDescriptorSetLayout*)
			};
			if (vkAllocateDescriptorSets(vulkanRhi.getVulkanContext().getVkDevice(), &vkDescriptorSetAllocateInfo, &vkDescriptorSet) != VK_SUCCESS)
			{
				RHI_LOG(context, CRITICAL, "Failed to allocate the Vulkan descriptor set")
			}
		}

		// Create resource group
		return RHI_NEW(context, ResourceGroup)(*this, rootParameterIndex, vkDescriptorSet, numberOfResources, resources, samplerStates RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // VulkanRhi




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

		[[nodiscard]] VKAPI_ATTR void* VKAPI_CALL vkAllocationFunction(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope)
		{
			return reinterpret_cast<Rhi::IAllocator*>(pUserData)->reallocate(nullptr, 0, size, alignment);
		}

		[[nodiscard]] VKAPI_ATTR void* VKAPI_CALL vkReallocationFunction(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope)
		{
			return reinterpret_cast<Rhi::IAllocator*>(pUserData)->reallocate(pOriginal, 0, size, alignment);
		}

		VKAPI_ATTR void VKAPI_CALL vkFreeFunction(void* pUserData, void* pMemory)
		{
			reinterpret_cast<Rhi::IAllocator*>(pUserData)->reallocate(pMemory, 0, 0, 1);
		}

		namespace ImplementationDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ExecuteCommandBuffer* realData = static_cast<const Rhi::Command::ExecuteCommandBuffer*>(data);
				RHI_ASSERT(rhi.getContext(), nullptr != realData->commandBufferToExecute, "The Vulkan command buffer to execute must be valid")
				rhi.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics states                                       ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsRootSignature* realData = static_cast<const Rhi::Command::SetGraphicsRootSignature*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsPipelineState* realData = static_cast<const Rhi::Command::SetGraphicsPipelineState*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsResourceGroup* realData = static_cast<const Rhi::Command::SetGraphicsResourceGroup*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Rhi::IRhi& rhi)
			{
				// Input-assembler (IA) stage
				const Rhi::Command::SetGraphicsVertexArray* realData = static_cast<const Rhi::Command::SetGraphicsVertexArray*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsViewports* realData = static_cast<const Rhi::Command::SetGraphicsViewports*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Rhi::Viewport*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsScissorRectangles* realData = static_cast<const Rhi::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Rhi::ScissorRectangle*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Rhi::IRhi& rhi)
			{
				// Output-merger (OM) stage
				const Rhi::Command::SetGraphicsRenderTarget* realData = static_cast<const Rhi::Command::SetGraphicsRenderTarget*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ClearGraphics* realData = static_cast<const Rhi::Command::ClearGraphics*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<VulkanRhi::VulkanRhi&>(rhi).drawGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<VulkanRhi::VulkanRhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<VulkanRhi::VulkanRhi&>(rhi).drawIndexedGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<VulkanRhi::VulkanRhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeRootSignature* realData = static_cast<const Rhi::Command::SetComputeRootSignature*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputePipelineState* realData = static_cast<const Rhi::Command::SetComputePipelineState*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeResourceGroup* realData = static_cast<const Rhi::Command::SetComputeResourceGroup*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCompute* realData = static_cast<const Rhi::Command::DispatchCompute*>(data);
				vkCmdDispatch(static_cast<VulkanRhi::VulkanRhi&>(rhi).getVulkanContext().getVkCommandBuffer(), realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Rhi::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Rhi::ResourceType::TEXTURE_2D)
				{
					static_cast<VulkanRhi::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RHI_LOG(static_cast<VulkanRhi::VulkanRhi&>(rhi).getContext(), CRITICAL, "Unsupported Vulkan texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Rhi::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyResource* realData = static_cast<const Rhi::Command::CopyResource*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::GenerateMipmaps* realData = static_cast<const Rhi::Command::GenerateMipmaps*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResetQueryPool* realData = static_cast<const Rhi::Command::ResetQueryPool*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::BeginQuery* realData = static_cast<const Rhi::Command::BeginQuery*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::EndQuery* realData = static_cast<const Rhi::Command::EndQuery*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::WriteTimestampQuery* realData = static_cast<const Rhi::Command::WriteTimestampQuery*>(data);
				static_cast<VulkanRhi::VulkanRhi&>(rhi).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RHI_DEBUG
				void SetDebugMarker(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::SetDebugMarker* realData = static_cast<const Rhi::Command::SetDebugMarker*>(data);
					static_cast<VulkanRhi::VulkanRhi&>(rhi).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::BeginDebugEvent* realData = static_cast<const Rhi::Command::BeginDebugEvent*>(data);
					static_cast<VulkanRhi::VulkanRhi&>(rhi).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Rhi::IRhi& rhi)
				{
					static_cast<VulkanRhi::VulkanRhi&>(rhi).endDebugEvent();
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

		void beginVulkanRenderPass(const Rhi::IRenderTarget& renderTarget, VkRenderPass vkRenderPass, VkFramebuffer vkFramebuffer, uint32_t numberOfAttachments, const VulkanRhi::VulkanRhi::VkClearValues& vkClearValues, VkCommandBuffer vkCommandBuffer)
		{
			// Get render target dimension
			uint32_t width = 1;
			uint32_t height = 1;
			renderTarget.getWidthAndHeight(width, height);

			// Begin Vulkan render pass
			const VkRenderPassBeginInfo vkRenderPassBeginInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// sType (VkStructureType)
				nullptr,									// pNext (const void*)
				vkRenderPass,								// renderPass (VkRenderPass)
				vkFramebuffer,								// framebuffer (VkFramebuffer)
				{ // renderArea (VkRect2D)
					{ 0, 0 },								// offset (VkOffset2D)
					{ width, height }						// extent (VkExtent2D)
				},
				numberOfAttachments,						// clearValueCount (uint32_t)
				vkClearValues.data()						// pClearValues (const VkClearValue*)
			};
			vkCmdBeginRenderPass(vkCommandBuffer, &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Rhi::ImplementationDispatchFunction DISPATCH_FUNCTIONS[Rhi::CommandDispatchFunctionIndex::NumberOfFunctions] =
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
namespace VulkanRhi
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	VulkanRhi::VulkanRhi(const Rhi::Context& context) :
		IRhi(Rhi::NameId::VULKAN, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		ComputePipelineStateMakeId(context.getAllocator()),
		mVkAllocationCallbacks{&context.getAllocator(), &::detail::vkAllocationFunction, &::detail::vkReallocationFunction, &::detail::vkFreeFunction, nullptr, nullptr},
		mVulkanRuntimeLinking(nullptr),
		mVulkanContext(nullptr),
		mShaderLanguageGlsl(nullptr),
		mGraphicsRootSignature(nullptr),
		mComputeRootSignature(nullptr),
		mDefaultSamplerState(nullptr),
		mInsideVulkanRenderPass(false),
		mVkClearValues{},
		mVertexArray(nullptr),
		mRenderTarget(nullptr)
		#ifdef RHI_DEBUG
			, mDebugBetweenBeginEndScene(false)
		#endif
	{
		// TODO(co) Make it possible to enable/disable validation from the outside?
		#ifdef RHI_DEBUG
			const bool enableValidation = true;
		#else
			const bool enableValidation = false;
		#endif

		// Is Vulkan available?
		mVulkanRuntimeLinking = RHI_NEW(mContext, VulkanRuntimeLinking)(*this, enableValidation);
		if (mVulkanRuntimeLinking->isVulkanAvaiable())
		{
			// TODO(co) Add external Vulkan context support
			mVulkanContext = RHI_NEW(mContext, VulkanContext)(*this);

			// Is the Vulkan context initialized?
			if (mVulkanContext->isInitialized())
			{
				// Initialize the capabilities
				initializeCapabilities();

				// Create the default sampler state
				mDefaultSamplerState = createSamplerState(Rhi::ISamplerState::getDefaultSamplerState());

				// Add references to the default sampler state and set it
				if (nullptr != mDefaultSamplerState)
				{
					mDefaultSamplerState->addReference();
					// TODO(co) Set default sampler states
				}
			}
		}
	}

	VulkanRhi::~VulkanRhi()
	{
		// Set no vertex array reference, in case we have one
		setGraphicsVertexArray(nullptr);

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
					RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation is going to be destroyed, but there are still %u resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation is going to be destroyed, but there is still one resource instance left (memory leak)")
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

		// Destroy the Vulkan context instance
		RHI_DELETE(mContext, VulkanContext, mVulkanContext);

		// Destroy the Vulkan runtime linking instance
		RHI_DELETE(mContext, VulkanRuntimeLinking, mVulkanRuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void VulkanRhi::setGraphicsRootSignature(Rhi::IRootSignature* rootSignature)
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

	void VulkanRhi::setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsPipelineState)

			// Bind Vulkan graphics pipeline
			vkCmdBindPipeline(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<GraphicsPipelineState*>(graphicsPipelineState)->getVkPipeline());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void VulkanRhi::setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RHI_LOG(mContext, CRITICAL, "No Vulkan RHI implementation graphics root signature set")
				return;
			}
			const Rhi::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation root parameter index is out of bounds")
				return;
			}
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Rhi::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Bind Vulkan descriptor set
			const VkDescriptorSet vkDescriptorSet = static_cast<ResourceGroup*>(resourceGroup)->getVkDescriptorSet();
			if (VK_NULL_HANDLE != vkDescriptorSet)
			{
				vkCmdBindDescriptorSets(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsRootSignature->getVkPipelineLayout(), rootParameterIndex, 1, &vkDescriptorSet, 0, nullptr);
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void VulkanRhi::setGraphicsVertexArray(Rhi::IVertexArray* vertexArray)
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

				// Bind Vulkan buffers
				mVertexArray->bindVulkanBuffers(getVulkanContext().getVkCommandBuffer());
			}
			else
			{
				// Unset the currently used vertex array
				unsetGraphicsVertexArray();
			}
		}
	}

	void VulkanRhi::setGraphicsViewports([[maybe_unused]] uint32_t numberOfViewports, const Rhi::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Vulkan rasterizer state viewports")

		// Set Vulkan viewport
		// -> We're using the "VK_KHR_maintenance1"-extension ("VK_KHR_MAINTENANCE1_EXTENSION_NAME"-definition) to be able to specify a negative viewport height,
		//    this way we don't have to apply "<output position>.y = -<output position>.y" inside vertex shaders to compensate for the Vulkan coordinate system
		// TODO(co) Add support for multiple viewports
		VkViewport vkViewport = reinterpret_cast<const VkViewport*>(viewports)[0];
		vkViewport.y += vkViewport.height;
		vkViewport.height = -vkViewport.height;
		vkCmdSetViewport(getVulkanContext().getVkCommandBuffer(), 0, 1, &vkViewport);
	}

	void VulkanRhi::setGraphicsScissorRectangles([[maybe_unused]] uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Vulkan rasterizer state scissor rectangles")

		// Set Vulkan scissor
		// TODO(co) Add support for multiple scissor rectangles. Change "Rhi::ScissorRectangle" to Vulkan style to make it the primary API on the long run?
		const VkRect2D vkRect2D =
		{
			{ static_cast<int32_t>(scissorRectangles[0].topLeftX), static_cast<int32_t>(scissorRectangles[0].topLeftY) },
			{ static_cast<uint32_t>(scissorRectangles[0].bottomRightX - scissorRectangles[0].topLeftX), static_cast<uint32_t>(scissorRectangles[0].bottomRightY - scissorRectangles[0].topLeftY) }
		};
		vkCmdSetScissor(getVulkanContext().getVkCommandBuffer(), 0, 1, &vkRect2D);
	}

	void VulkanRhi::setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Release the render target reference, in case we have one
			if (nullptr != mRenderTarget)
			{
				// Start Vulkan render pass, if necessary (for e.g. clearing)
				if (!mInsideVulkanRenderPass && ((mRenderTarget->getResourceType() == Rhi::ResourceType::SWAP_CHAIN && nullptr == renderTarget) || mRenderTarget->getResourceType() == Rhi::ResourceType::FRAMEBUFFER))
				{
					beginVulkanRenderPass();
				}

				// End Vulkan render pass, if necessary
				if (mInsideVulkanRenderPass)
				{
					vkCmdEndRenderPass(getVulkanContext().getVkCommandBuffer());
					mInsideVulkanRenderPass = false;
				}

				// Release
				mRenderTarget->releaseReference();
				mRenderTarget = nullptr;
			}

			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *renderTarget)

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Set clear color and clear depth stencil values
				const uint32_t numberOfColorAttachments = static_cast<const RenderPass&>(mRenderTarget->getRenderPass()).getNumberOfColorAttachments();
				RHI_ASSERT(mContext, numberOfColorAttachments < 8, "Vulkan only supports 7 render pass color attachments")
				for (uint32_t i = 0; i < numberOfColorAttachments; ++i)
				{
					mVkClearValues[i] = VkClearValue{0.0f, 0.0f, 0.0f, 1.0f};
				}
				mVkClearValues[numberOfColorAttachments] = VkClearValue{ 1.0f, 0 };
			}
		}
	}

	void VulkanRhi::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != mRenderTarget, "Can't execute Vulkan clear command without a render target set")
		RHI_ASSERT(mContext, !mInsideVulkanRenderPass, "Can't execute clear command inside a Vulkan render pass")
		RHI_ASSERT(mContext, z >= 0.0f && z <= 1.0f, "The Vulkan clear graphics z value must be between [0, 1] (inclusive)")

		// Clear color
		const uint32_t numberOfColorAttachments = static_cast<const RenderPass&>(mRenderTarget->getRenderPass()).getNumberOfColorAttachments();
		RHI_ASSERT(mContext, numberOfColorAttachments < 8, "Vulkan only supports 7 render pass color attachments")
		if (clearFlags & Rhi::ClearFlag::COLOR)
		{
			for (uint32_t i = 0; i < numberOfColorAttachments; ++i)
			{
				memcpy(mVkClearValues[i].color.float32, &color[0], sizeof(float) * 4);
			}
		}

		// Clear depth stencil
		if ((clearFlags & Rhi::ClearFlag::DEPTH) || (clearFlags & Rhi::ClearFlag::STENCIL))
		{
			mVkClearValues[numberOfColorAttachments].depthStencil.depth = z;
			mVkClearValues[numberOfColorAttachments].depthStencil.stencil = stencil;
		}
	}

	void VulkanRhi::drawGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Vulkan draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Vulkan draw indirect command
		vkCmdDrawIndirect(getVulkanContext().getVkCommandBuffer(), static_cast<const IndirectBuffer&>(indirectBuffer).getVkBuffer(), indirectBufferOffset, numberOfDraws, sizeof(VkDrawIndirectCommand));
	}

	void VulkanRhi::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The Vulkan emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of Vulkan draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Emit the draw calls
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-draw-indirect emulation");
			}
		#endif
		const VkCommandBuffer vkCommandBuffer = getVulkanContext().getVkCommandBuffer();
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			// Draw and advance
			const Rhi::DrawArguments& drawArguments = *reinterpret_cast<const Rhi::DrawArguments*>(emulationData);
			vkCmdDraw(vkCommandBuffer, drawArguments.vertexCountPerInstance, drawArguments.instanceCount, drawArguments.startVertexLocation, drawArguments.startInstanceLocation);
			emulationData += sizeof(Rhi::DrawArguments);
		}
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void VulkanRhi::drawIndexedGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Vulkan draws must not be zero")
		RHI_ASSERT(mContext, nullptr != mVertexArray, "Vulkan draw indexed needs a set vertex array")
		RHI_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "Vulkan draw indexed needs a set vertex array which contains an index buffer")

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Vulkan draw indexed indirect command
		vkCmdDrawIndexedIndirect(getVulkanContext().getVkCommandBuffer(), static_cast<const IndirectBuffer&>(indirectBuffer).getVkBuffer(), indirectBufferOffset, numberOfDraws, sizeof(VkDrawIndexedIndirectCommand));
	}

	void VulkanRhi::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The Vulkan emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of Vulkan draws must not be zero")
		RHI_ASSERT(mContext, nullptr != mVertexArray, "Vulkan draw indexed needs a set vertex array")
		RHI_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "Vulkan draw indexed needs a set vertex array which contains an index buffer")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Start Vulkan render pass, if necessary
		if (!mInsideVulkanRenderPass)
		{
			beginVulkanRenderPass();
		}

		// Emit the draw calls
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			}
		#endif
		const VkCommandBuffer vkCommandBuffer = getVulkanContext().getVkCommandBuffer();
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			// Draw and advance
			const Rhi::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Rhi::DrawIndexedArguments*>(emulationData);
			vkCmdDrawIndexed(vkCommandBuffer, drawIndexedArguments.indexCountPerInstance, drawIndexedArguments.instanceCount, drawIndexedArguments.startIndexLocation, drawIndexedArguments.baseVertexLocation, drawIndexedArguments.startInstanceLocation);
			emulationData += sizeof(Rhi::DrawIndexedArguments);
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
	void VulkanRhi::setComputeRootSignature(Rhi::IRootSignature* rootSignature)
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

	void VulkanRhi::setComputePipelineState(Rhi::IComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *computePipelineState)

			// Bind Vulkan compute pipeline
			vkCmdBindPipeline(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, static_cast<ComputePipelineState*>(computePipelineState)->getVkPipeline());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void VulkanRhi::setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			if (nullptr == mComputeRootSignature)
			{
				RHI_LOG(mContext, CRITICAL, "No Vulkan RHI implementation compute root signature set")
				return;
			}
			const Rhi::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation root parameter index is out of bounds")
				return;
			}
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Rhi::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RHI_LOG(mContext, CRITICAL, "The Vulkan RHI implementation descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Bind Vulkan descriptor set
			const VkDescriptorSet vkDescriptorSet = static_cast<ResourceGroup*>(resourceGroup)->getVkDescriptorSet();
			if (VK_NULL_HANDLE != vkDescriptorSet)
			{
				vkCmdBindDescriptorSets(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, mComputeRootSignature->getVkPipelineLayout(), rootParameterIndex, 1, &vkDescriptorSet, 0, nullptr);
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void VulkanRhi::resolveMultisampleFramebuffer(Rhi::IRenderTarget&, Rhi::IFramebuffer&)
	{
		// TODO(co) Implement me
	}

	void VulkanRhi::copyResource(Rhi::IResource&, Rhi::IResource&)
	{
		// TODO(co) Implement me
	}

	void VulkanRhi::generateMipmaps(Rhi::IResource&)
	{
		// TODO(co) Implement me: Search for https://github.com/SaschaWillems/Vulkan/tree/master/texturemipmapgen inside this cpp file and unify the code to be able to reuse it in here
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void VulkanRhi::resetQueryPool(Rhi::IQueryPool& queryPool, uint32_t firstQueryIndex, uint32_t numberOfQueries)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Reset Vulkan query pool
		vkCmdResetQueryPool(getVulkanContext().getVkCommandBuffer(), static_cast<const QueryPool&>(queryPool).getVkQueryPool(), firstQueryIndex, numberOfQueries);
	}

	void VulkanRhi::beginQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex, uint32_t queryControlFlags)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Begin Vulkan query
		vkCmdBeginQuery(getVulkanContext().getVkCommandBuffer(), static_cast<const QueryPool&>(queryPool).getVkQueryPool(), queryIndex, ((queryControlFlags & Rhi::QueryControlFlags::PRECISE) != 0) ? VK_QUERY_CONTROL_PRECISE_BIT : 0u);
	}

	void VulkanRhi::endQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// End Vulkan query
		vkCmdEndQuery(getVulkanContext().getVkCommandBuffer(), static_cast<const QueryPool&>(queryPool).getVkQueryPool(), queryIndex);
	}

	void VulkanRhi::writeTimestampQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Write Vulkan timestamp query
		vkCmdWriteTimestamp(getVulkanContext().getVkCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, static_cast<const QueryPool&>(queryPool).getVkQueryPool(), queryIndex);
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void VulkanRhi::setDebugMarker(const char* name)
		{
			if (nullptr != vkCmdDebugMarkerInsertEXT)
			{
				RHI_ASSERT(mContext, nullptr != name, "Vulkan debug marker names must not be a null pointer")
				const VkDebugMarkerMarkerInfoEXT vkDebugMarkerMarkerInfoEXT =
				{
					VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					name,											// pMarkerName (const char*)
					{ // color[4] (float)
						0.0f,
						0.0f,
						1.0f,	// Blue
						1.0f
					}
				};
				vkCmdDebugMarkerInsertEXT(getVulkanContext().getVkCommandBuffer(), &vkDebugMarkerMarkerInfoEXT);
			}
		}

		void VulkanRhi::beginDebugEvent(const char* name)
		{
			if (nullptr != vkCmdDebugMarkerBeginEXT)
			{
				RHI_ASSERT(mContext, nullptr != name, "Vulkan debug event names must not be a null pointer")
				const VkDebugMarkerMarkerInfoEXT vkDebugMarkerMarkerInfoEXT =
				{
					VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,	// sType (VkStructureType)
					nullptr,										// pNext (const void*)
					name,											// pMarkerName (const char*)
					{ // color[4] (float)
						0.0f,
						1.0f,	// Green
						0.0f,
						1.0f
					}
				};
				vkCmdDebugMarkerBeginEXT(getVulkanContext().getVkCommandBuffer(), &vkDebugMarkerMarkerInfoEXT);
			}
		}

		void VulkanRhi::endDebugEvent()
		{
			if (nullptr != vkCmdDebugMarkerEndEXT)
			{
				vkCmdDebugMarkerEndEXT(getVulkanContext().getVkCommandBuffer());
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	const char* VulkanRhi::getName() const
	{
		return "Vulkan";
	}

	bool VulkanRhi::isInitialized() const
	{
		// Is the Vulkan context initialized?
		return (nullptr != mVulkanContext && mVulkanContext->isInitialized());
	}

	bool VulkanRhi::isDebugEnabled()
	{
		// Check for any "VK_EXT_debug_marker"-extension function pointer
		return (nullptr != vkDebugMarkerSetObjectTagEXT || nullptr != vkDebugMarkerSetObjectNameEXT || nullptr != vkCmdDebugMarkerBeginEXT || nullptr != vkCmdDebugMarkerEndEXT || nullptr != vkCmdDebugMarkerInsertEXT);
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t VulkanRhi::getNumberOfShaderLanguages() const
	{
		// Done, return the number of supported shader languages
		return 1;
	}

	const char* VulkanRhi::getShaderLanguageName([[maybe_unused]] uint32_t index) const
	{
		RHI_ASSERT(mContext, index < getNumberOfShaderLanguages(), "Vulkan: Shader language index is out-of-bounds")
		return ::detail::GLSL_NAME;
	}

	Rhi::IShaderLanguage* VulkanRhi::getShaderLanguage(const char* shaderLanguageName)
	{
		// In case "shaderLanguage" is a null pointer, use the default shader language
		if (nullptr != shaderLanguageName)
		{
			// Optimization: Check for shader language name pointer match, first
			// -> "ShaderLanguageSeparate::NAME" has the same value
			if (::detail::GLSL_NAME == shaderLanguageName || !stricmp(shaderLanguageName, ::detail::GLSL_NAME))
			{
				// If required, create the GLSL shader language instance right now
				if (nullptr == mShaderLanguageGlsl)
				{
					mShaderLanguageGlsl = RHI_NEW(mContext, ShaderLanguageGlsl(*this));
					mShaderLanguageGlsl->addReference();	// Internal RHI reference
				}
				return mShaderLanguageGlsl;
			}
		}
		else
		{
			// Return the shader language instance as default
			return getShaderLanguage(::detail::GLSL_NAME);
		}

		// Error!
		return nullptr;
	}


	//[-------------------------------------------------------]
	//[ Resource creation                                     ]
	//[-------------------------------------------------------]
	Rhi::IRenderPass* VulkanRhi::createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IQueryPool* VulkanRhi::createQueryPool(Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		RHI_ASSERT(mContext, numberOfQueries > 0, "Vulkan: Number of queries mustn't be zero")
		return RHI_NEW(mContext, QueryPool)(*this, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::ISwapChain* VulkanRhi::createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, renderPass)
		RHI_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle || nullptr != windowHandle.renderWindow, "Vulkan: The provided native window handle or render window must not be a null handle / null pointer")

		// Create the swap chain
		return RHI_NEW(mContext, SwapChain)(renderPass, windowHandle);
	}

	Rhi::IFramebuffer* VulkanRhi::createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, renderPass)

		// Create the framebuffer
		return RHI_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IBufferManager* VulkanRhi::createBufferManager()
	{
		return RHI_NEW(mContext, BufferManager)(*this);
	}

	Rhi::ITextureManager* VulkanRhi::createTextureManager()
	{
		return RHI_NEW(mContext, TextureManager)(*this);
	}

	Rhi::IRootSignature* VulkanRhi::createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RootSignature)(*this, rootSignature RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IGraphicsPipelineState* VulkanRhi::createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "Vulkan: Invalid graphics pipeline state root signature")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "Vulkan: Invalid graphics pipeline state graphics program")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "Vulkan: Invalid graphics pipeline state render pass")

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

	Rhi::IComputePipelineState* VulkanRhi::createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, rootSignature)
		RHI_MATCH_CHECK(*this, computeShader)

		// Create the compute pipeline state
		uint16_t id = 0;
		if (ComputePipelineStateMakeId.CreateID(id))
		{
			return RHI_NEW(mContext, ComputePipelineState)(*this, rootSignature, computeShader, id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();
		return nullptr;
	}

	Rhi::ISamplerState* VulkanRhi::createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, SamplerState)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool VulkanRhi::map(Rhi::IResource& resource, uint32_t, Rhi::MapType, uint32_t, Rhi::MappedSubresource& mappedSubresource)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::INDEX_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<IndexBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Rhi::ResourceType::VERTEX_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<VertexBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Rhi::ResourceType::TEXTURE_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<TextureBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Rhi::ResourceType::STRUCTURED_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<StructuredBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Rhi::ResourceType::INDIRECT_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<IndirectBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
			}

			case Rhi::ResourceType::UNIFORM_BUFFER:
			{
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (vkMapMemory(getVulkanContext().getVkDevice(), static_cast<UniformBuffer&>(resource).getVkDeviceMemory(), 0, VK_WHOLE_SIZE, 0, &mappedSubresource.data) == VK_SUCCESS);
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
				// TODO(co) Implement me
				return false;
			}

			case Rhi::ResourceType::TEXTURE_2D_ARRAY:
			{
				// TODO(co) Implement me
				return false;
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

	void VulkanRhi::unmap(Rhi::IResource& resource, uint32_t)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::INDEX_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<IndexBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Rhi::ResourceType::VERTEX_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<VertexBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Rhi::ResourceType::TEXTURE_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<TextureBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Rhi::ResourceType::STRUCTURED_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<StructuredBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Rhi::ResourceType::INDIRECT_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<IndirectBuffer&>(resource).getVkDeviceMemory());
				break;
			}

			case Rhi::ResourceType::UNIFORM_BUFFER:
			{
				vkUnmapMemory(getVulkanContext().getVkDevice(), static_cast<UniformBuffer&>(resource).getVkDeviceMemory());
				break;
			}

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
			case Rhi::ResourceType::COMPUTE_SHADER:
			default:
				// Nothing we can unmap
				break;
		}
	}

	bool VulkanRhi::getQueryPoolResults(Rhi::IQueryPool& queryPool, uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, uint32_t strideInBytes, [[maybe_unused]] uint32_t queryResultFlags)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& vulkanQueryPool = static_cast<const QueryPool&>(queryPool);
		switch (vulkanQueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
			case Rhi::QueryType::TIMESTAMP:	// TODO(co) Convert time to nanoseconds, see e.g. http://vulkan-spec-chunked.ahcox.com/ch16s05.html - VkPhysicalDeviceLimits::timestampPeriod - The number of nanoseconds it takes for a timestamp value to be incremented by 1
			{
				// Get Vulkan query pool results
				const VkQueryResultFlags vkQueryResultFlags = 0u;
				// const VkQueryResultFlags vkQueryResultFlags = ((queryResultFlags & Rhi::QueryResultFlags::WAIT) != 0) ? VK_QUERY_RESULT_WAIT_BIT : 0u;	// TODO(co)
				return (vkGetQueryPoolResults(getVulkanContext().getVkDevice(), vulkanQueryPool.getVkQueryPool(), firstQueryIndex, numberOfQueries, numberOfDataBytes, data, strideInBytes, VK_QUERY_RESULT_64_BIT | vkQueryResultFlags) == VK_SUCCESS);
			}

			case Rhi::QueryType::PIPELINE_STATISTICS:
			{
				// Our setup results in the same structure layout as used by "D3D11_QUERY_DATA_PIPELINE_STATISTICS" which we use for "Rhi::PipelineStatisticsQueryResult"
				const VkQueryResultFlags vkQueryResultFlags = 0u;
				// const VkQueryResultFlags vkQueryResultFlags = ((queryResultFlags & Rhi::QueryResultFlags::WAIT) != 0) ? VK_QUERY_RESULT_WAIT_BIT : 0u;	// TODO(co)
				return (vkGetQueryPoolResults(getVulkanContext().getVkDevice(), vulkanQueryPool.getVkQueryPool(), firstQueryIndex, numberOfQueries, numberOfDataBytes, data, strideInBytes, VK_QUERY_RESULT_64_BIT | vkQueryResultFlags) == VK_SUCCESS);
			}
		}

		// Result not ready
		return false;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool VulkanRhi::beginScene()
	{
		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, false == mDebugBetweenBeginEndScene, "Vulkan: Begin scene was called while scene rendering is already in progress, missing end scene call?")
			mDebugBetweenBeginEndScene = true;
		#endif

		// Begin Vulkan command buffer
		// -> This automatically resets the Vulkan command buffer in case it was previously already recorded
		static constexpr VkCommandBufferBeginInfo vkCommandBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType (VkStructureType)
			nullptr,										// pNext (const void*)
			0,												// flags (VkCommandBufferUsageFlags)
			nullptr											// pInheritanceInfo (const VkCommandBufferInheritanceInfo*)
		};
		if (vkBeginCommandBuffer(getVulkanContext().getVkCommandBuffer(), &vkCommandBufferBeginInfo) == VK_SUCCESS)
		{
			// Done
			return true;
		}
		else
		{
			// Error!
			RHI_LOG(getContext(), CRITICAL, "Failed to begin Vulkan command buffer instance")
			return false;
		}
	}

	void VulkanRhi::submitCommandBuffer(const Rhi::CommandBuffer& commandBuffer)
	{
		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Rhi::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Submit command packet
				const Rhi::CommandDispatchFunctionIndex commandDispatchFunctionIndex = Rhi::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket);
				const void* command = Rhi::CommandPacketHelper::loadCommand(constCommandPacket);
				detail::DISPATCH_FUNCTIONS[commandDispatchFunctionIndex](command, *this);
			}

			{ // Next command
				const uint32_t nextCommandPacketByteIndex = Rhi::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
				constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
			}
		}
	}

	void VulkanRhi::endScene()
	{
		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, true == mDebugBetweenBeginEndScene, "Vulkan: End scene was called while scene rendering isn't in progress, missing start scene call?")
			mDebugBetweenBeginEndScene = false;
		#endif

		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);

		// We need to forget about the currently set vertex array
		unsetGraphicsVertexArray();

		// End Vulkan command buffer
		if (vkEndCommandBuffer(getVulkanContext().getVkCommandBuffer()) != VK_SUCCESS)
		{
			// Error!
			RHI_LOG(getContext(), CRITICAL, "Failed to end Vulkan command buffer instance")
		}
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void VulkanRhi::flush()
	{
		// TODO(co) Implement me
	}

	void VulkanRhi::finish()
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void VulkanRhi::initializeCapabilities()
	{
		{ // Get device name
			VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
			vkGetPhysicalDeviceProperties(mVulkanContext->getVkPhysicalDevice(), &vkPhysicalDeviceProperties);
			const size_t numberOfCharacters = ::detail::countof(mCapabilities.deviceName) - 1;
			strncpy(mCapabilities.deviceName, vkPhysicalDeviceProperties.deviceName, numberOfCharacters);
			mCapabilities.deviceName[numberOfCharacters] = '\0';
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat = (SwapChain::findColorVkFormat(getContext(), mVulkanRuntimeLinking->getVkInstance(), *mVulkanContext) == VK_FORMAT_R8G8B8A8_UNORM) ? Rhi::TextureFormat::Enum::R8G8B8A8 : Rhi::TextureFormat::Enum::B8G8R8A8;

		{ // Preferred swap chain depth stencil texture format
			const VkFormat depthVkFormat = SwapChain::findDepthVkFormat(mVulkanContext->getVkPhysicalDevice());
			if (VK_FORMAT_D32_SFLOAT == depthVkFormat)
			{
				mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;
			}
			else
			{
				// TODO(co) Add support for "VK_FORMAT_D32_SFLOAT_S8_UINT" and "VK_FORMAT_D24_UNORM_S8_UINT"
				mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;
			}
		}

		// TODO(co) Implement me, this in here is just a placeholder implementation

		{ // "D3D_FEATURE_LEVEL_11_0"
			// Maximum number of viewports (always at least 1)
			mCapabilities.maximumNumberOfViewports = 16;

			// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
			mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

			// Maximum texture dimension
			mCapabilities.maximumTextureDimension = 16384;

			// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
			mCapabilities.maximumNumberOf1DTextureArraySlices = 512;

			// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
			mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

			// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
			mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 128 * 1024 * 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention the texture buffer? Currently the OpenGL 3 minimum is used: 128 MiB.

			// Maximum indirect buffer size in bytes
			mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

			// Maximum number of multisamples (always at least 1, usually 8)
			mCapabilities.maximumNumberOfMultisamples = 1;	// TODO(co) Add multisample support, when setting "VkPhysicalDeviceFeatures" -> "sampleRateShading" -> VK_TRUE don't forget to check whether or not the device sup pots the feature

			// Maximum anisotropy (always at least 1, usually 16)
			mCapabilities.maximumAnisotropy = 16;

			// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
			mCapabilities.instancedArrays = true;

			// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
			mCapabilities.drawInstanced = true;

			// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
			mCapabilities.maximumNumberOfPatchVertices = 32;

			// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
			mCapabilities.maximumNumberOfGsOutputVertices = 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention it, so I assume it's 1024
		}

		// The rest is the same for all feature levels

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		// -> See https://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx - "Resource Limits (Direct3D 11)" - "Number of elements in a constant buffer D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT (4096)"
		// -> One element = float4 = 16 bytes
		mCapabilities.maximumUniformBufferSize = 4096 * 16;

		// Left-handed coordinate system with clip space depth value range 0..1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = true;

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = false;

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = true;

		// Vulkan has native multithreading
		mCapabilities.nativeMultithreading = false;	// TODO(co) Enable native multithreading when done

		// Vulkan has shader bytecode support
		mCapabilities.shaderBytecode = false;	// TODO(co) Vulkan has shader bytecode support, set "mCapabilities.shaderBytecode" to true later on

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = true;
	}

	void VulkanRhi::unsetGraphicsVertexArray()
	{
		// Release the currently used vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			// Do nothing since the Vulkan specification says "bindingCount must be greater than 0"
			// vkCmdBindVertexBuffers(getVulkanContext().getVkCommandBuffer(), 0, 0, nullptr, nullptr);

			// Release reference
			mVertexArray->releaseReference();
			mVertexArray = nullptr;
		}
	}

	void VulkanRhi::beginVulkanRenderPass()
	{
		// Sanity checks
		RHI_ASSERT(mContext, !mInsideVulkanRenderPass, "We're already inside a Vulkan render pass")
		RHI_ASSERT(mContext, nullptr != mRenderTarget, "Can't begin a Vulkan render pass without a render target set")

		// Start Vulkan render pass
		const uint32_t numberOfAttachments = static_cast<const RenderPass&>(mRenderTarget->getRenderPass()).getNumberOfAttachments();
		RHI_ASSERT(mContext, numberOfAttachments < 9, "Vulkan only supports 8 render pass attachments")
		switch (mRenderTarget->getResourceType())
		{
			case Rhi::ResourceType::SWAP_CHAIN:
			{
				const SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);
				::detail::beginVulkanRenderPass(*mRenderTarget, swapChain->getVkRenderPass(), swapChain->getCurrentVkFramebuffer(), numberOfAttachments, mVkClearValues, getVulkanContext().getVkCommandBuffer());
				break;
			}

			case Rhi::ResourceType::FRAMEBUFFER:
			{
				const Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);
				::detail::beginVulkanRenderPass(*mRenderTarget, framebuffer->getVkRenderPass(), framebuffer->getVkFramebuffer(), numberOfAttachments, mVkClearValues, getVulkanContext().getVkCommandBuffer());
				break;
			}

			case Rhi::ResourceType::ROOT_SIGNATURE:
			case Rhi::ResourceType::RESOURCE_GROUP:
			case Rhi::ResourceType::GRAPHICS_PROGRAM:
			case Rhi::ResourceType::VERTEX_ARRAY:
			case Rhi::ResourceType::RENDER_PASS:
			case Rhi::ResourceType::QUERY_POOL:
			case Rhi::ResourceType::INDEX_BUFFER:
			case Rhi::ResourceType::VERTEX_BUFFER:
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
			case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
			case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
			case Rhi::ResourceType::SAMPLER_STATE:
			case Rhi::ResourceType::VERTEX_SHADER:
			case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
			case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
			case Rhi::ResourceType::GEOMETRY_SHADER:
			case Rhi::ResourceType::FRAGMENT_SHADER:
			case Rhi::ResourceType::COMPUTE_SHADER:
			default:
				// Not handled in here
				break;
		}
		mInsideVulkanRenderPass = true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // VulkanRhi




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RHI_VULKAN_EXPORTS
	#define VULKANRHI_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define VULKANRHI_FUNCTION_EXPORT
#endif
VULKANRHI_FUNCTION_EXPORT Rhi::IRhi* createVulkanRhiInstance(const Rhi::Context& context)
{
	return RHI_NEW(context, VulkanRhi::VulkanRhi)(context);
}
#undef VULKANRHI_FUNCTION_EXPORT
