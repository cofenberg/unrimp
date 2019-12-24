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
*    Direct3D 11 RHI amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    - Direct3D 11 runtime and Direct3D 11 capable graphics driver
*    - Optional AMD AGS for multi-draw-indirect on AMD GPUs ("amd_ags_x64.dll" and "amd_ags_x86.dll" aren't automatically installed with the GPU driver)
*    - Optional NVIDIA NvAPI for multi-draw-indirect on NVIDIA GPUs ("nvapi.dll" is automatically installed with the GPU driver)
*
*    == Preprocessor Definitions ==
*    - Set "RHI_DIRECT3D11_EXPORTS" as preprocessor definition when building this library as shared library
*    - Do also have a look into the RHI header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

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

#include <VersionHelpers.h>	// For "IsWindows8OrGreater()"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch (compiling source file E:\private\unrimp\Source\Rhi\Private\Direct3D11Rhi\Direct3D11Rhi.cpp)
	PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_UInt_is_zero': default constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Generic_error_category': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::_UInt_is_zero': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_UInt_is_zero': move assignment operator was implicitly defined as deleted
	#include <mutex>
	#include <vector>
PRAGMA_WARNING_POP




//[-------------------------------------------------------]
//[ Direct3D11Rhi/MakeID.h                                ]
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
//[ Direct3D11Rhi/D3D11.h                                 ]
//[-------------------------------------------------------]
/*
  We don't use the Direct3D headers from the DirectX SDK because there are several issues:
  - Licensing: It's not allowed to redistribute the Direct3D headers, meaning everyone would
    have to get them somehow before compiling this project
  - The Direct3D headers are somewhat chaotic and include tons of other headers.
    This slows down compilation and the more headers are included, the higher the risk of
    naming or redefinition conflicts.
  - Starting with Windows 8, Direct3D is part of the Windows SDK. When using VisualStudio 2017
    and the Direct3D headers from "Microsoft DirectX SDK (June 2010)" you will get a lot of
    "
      <path>\external\directx\include\dxgitype.h(12): warning C4005: 'DXGI_STATUS_OCCLUDED' : macro redefinition
      c:\program files (x86)\windows kits\8.0\include\shared\winerror.h(49449) : see previous definition of 'DXGI_STATUS_OCCLUDED'
    "
    warnings.

    Do not include this header within headers which are usually used by users as well, do only
    use it inside cpp-files. It must still be possible that users of this RHI
    can use the Direct3D headers for features not covered by this RHI.
*/


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
struct ID3D11View;
struct IDXGIOutput;
struct ID3D11Query;
struct IDXGIDevice;
struct IDXGISurface;
struct IDXGIAdapter;
struct ID3D11Buffer;
struct IDXGIAdapter1;
struct IDXGIFactory1;
struct D3D11_FEATURE;
struct ID3D11Counter;
struct ID3D11Resource;
struct IDXGISwapChain;
struct IDXGISwapChain1;
struct ID3D11Texture1D;
struct ID3D11Texture2D;
struct ID3D11Texture3D;
struct ID3D11Predicate;
struct ID3D11BlendState;
struct D3D11_BLEND_DESC;
struct D3D11_QUERY_DESC;
struct ID3D11HullShader;
struct ID3D11PixelShader;
struct D3D11_BUFFER_DESC;
struct ID3D11InputLayout;
struct ID3D11CommandList;
struct D3D11_COUNTER_INFO;
struct D3D11_COUNTER_TYPE;
struct D3D11_COUNTER_DESC;
struct ID3D11ClassLinkage;
struct ID3D11Asynchronous;
struct ID3D11DomainShader;
struct ID3D11VertexShader;
struct D3D11_SAMPLER_DESC;
struct ID3D11SamplerState;
struct ID3D11DeviceContext;
struct ID3D11ClassInstance;
struct ID3D11ComputeShader;
struct ID3D11GeometryShader;
struct DXGI_FRAME_STATISTICS;
struct ID3D11RasterizerState;
struct ID3DDeviceContextState;
struct D3D11_SUBRESOURCE_DATA;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11DepthStencilState;
struct D3D11_DEPTH_STENCIL_DESC;
struct ID3D11ShaderResourceView;
struct D3D11_RESOURCE_DIMENSION;
struct D3D11_INPUT_ELEMENT_DESC;
struct ID3D11UnorderedAccessView;
struct D3D11_SO_DECLARATION_ENTRY;
struct D3D11_DEPTH_STENCIL_VIEW_DESC;
struct D3D11_RENDER_TARGET_VIEW_DESC;
struct D3D11_SHADER_RESOURCE_VIEW_DESC;
struct D3D11_UNORDERED_ACCESS_VIEW_DESC;


//[-------------------------------------------------------]
//[ Definitions                                           ]
//[-------------------------------------------------------]
// "Microsoft DirectX SDK (June 2010)" -> "D3Dcompiler.h"
#define D3DCOMPILE_DEBUG				(1 << 0)
#define D3DCOMPILE_SKIP_VALIDATION		(1 << 1)
#define D3DCOMPILE_SKIP_OPTIMIZATION	(1 << 2)
#define D3DCOMPILE_ENABLE_STRICTNESS	(1 << 11)
#define D3DCOMPILE_OPTIMIZATION_LEVEL0	(1 << 14)
#define D3DCOMPILE_OPTIMIZATION_LEVEL1	0
#define D3DCOMPILE_OPTIMIZATION_LEVEL2	((1 << 14) | (1 << 15))
#define D3DCOMPILE_OPTIMIZATION_LEVEL3	(1 << 15)
#define D3DCOMPILE_WARNINGS_ARE_ERRORS	(1 << 18)
#define D3DCOMPILE_ALL_RESOURCES_BOUND	(1 << 21)

// "Microsoft DirectX SDK (June 2010)" -> "d3d9types.h"
typedef DWORD D3DCOLOR;

// "Microsoft DirectX SDK (June 2010)" -> "d3d9types.h"
#define D3DCOLOR_ARGB(a,r,g,b) ((D3DCOLOR)((((a)&0xff)<<24) | (((r)&0xff)<<16) | (((g)&0xff)<<8) | ((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a, r, g, b)

// "Microsoft DirectX SDK (June 2010)" -> "D3X11.h"
#define D3D11_SDK_VERSION							(7)
#define	D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX	(15)
#define	D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT		(8)

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
#define	D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX	(15)
#define	D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT		(8)

// "Microsoft DirectX SDK (June 2010)" -> "D3X11.h"
#define	D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT	(32)

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
#define DXGI_MWA_NO_ALT_ENTER	(1 << 1)

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
#define DXGI_USAGE_RENDER_TARGET_OUTPUT	(1L << (1 + 4))

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
#define DXGI_PRESENT_ALLOW_TEARING	0x00000200UL

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
enum D3D_DRIVER_TYPE
{
	D3D_DRIVER_TYPE_UNKNOWN		= 0,
	D3D_DRIVER_TYPE_HARDWARE	= (D3D_DRIVER_TYPE_UNKNOWN + 1),
	D3D_DRIVER_TYPE_REFERENCE	= (D3D_DRIVER_TYPE_HARDWARE + 1),
	D3D_DRIVER_TYPE_NULL		= (D3D_DRIVER_TYPE_REFERENCE + 1),
	D3D_DRIVER_TYPE_SOFTWARE	= (D3D_DRIVER_TYPE_NULL + 1),
	D3D_DRIVER_TYPE_WARP		= (D3D_DRIVER_TYPE_SOFTWARE + 1)
};

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
enum D3D_FEATURE_LEVEL
{
	D3D_FEATURE_LEVEL_9_1	= 0x9100,
	D3D_FEATURE_LEVEL_9_2	= 0x9200,
	D3D_FEATURE_LEVEL_9_3	= 0x9300,
	D3D_FEATURE_LEVEL_10_0	= 0xa000,
	D3D_FEATURE_LEVEL_10_1	= 0xa100,
	D3D_FEATURE_LEVEL_11_0	= 0xb000,
	D3D_FEATURE_LEVEL_11_1	= 0xb100
};

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
enum D3D_SRV_DIMENSION
{
	D3D_SRV_DIMENSION_UNKNOWN				= 0,
	D3D_SRV_DIMENSION_BUFFER				= 1,
	D3D_SRV_DIMENSION_TEXTURE1D				= 2,
	D3D_SRV_DIMENSION_TEXTURE1DARRAY		= 3,
	D3D_SRV_DIMENSION_TEXTURE2D				= 4,
	D3D_SRV_DIMENSION_TEXTURE2DARRAY		= 5,
	D3D_SRV_DIMENSION_TEXTURE2DMS			= 6,
	D3D_SRV_DIMENSION_TEXTURE2DMSARRAY		= 7,
	D3D_SRV_DIMENSION_TEXTURE3D				= 8,
	D3D_SRV_DIMENSION_TEXTURECUBE			= 9,
	D3D_SRV_DIMENSION_TEXTURECUBEARRAY		= 10,
	D3D_SRV_DIMENSION_BUFFEREX				= 11,
	D3D11_SRV_DIMENSION_UNKNOWN				= D3D_SRV_DIMENSION_UNKNOWN,
	D3D11_SRV_DIMENSION_BUFFER				= D3D_SRV_DIMENSION_BUFFER,
	D3D11_SRV_DIMENSION_TEXTURE1D			= D3D_SRV_DIMENSION_TEXTURE1D,
	D3D11_SRV_DIMENSION_TEXTURE1DARRAY		= D3D_SRV_DIMENSION_TEXTURE1DARRAY,
	D3D11_SRV_DIMENSION_TEXTURE2D			= D3D_SRV_DIMENSION_TEXTURE2D,
	D3D11_SRV_DIMENSION_TEXTURE2DARRAY		= D3D_SRV_DIMENSION_TEXTURE2DARRAY,
	D3D11_SRV_DIMENSION_TEXTURE2DMS			= D3D_SRV_DIMENSION_TEXTURE2DMS,
	D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY	= D3D_SRV_DIMENSION_TEXTURE2DMSARRAY,
	D3D11_SRV_DIMENSION_TEXTURE3D			= D3D_SRV_DIMENSION_TEXTURE3D,
	D3D11_SRV_DIMENSION_TEXTURECUBE			= D3D_SRV_DIMENSION_TEXTURECUBE,
	D3D11_SRV_DIMENSION_TEXTURECUBEARRAY	= D3D_SRV_DIMENSION_TEXTURECUBEARRAY,
	D3D11_SRV_DIMENSION_BUFFEREX			= D3D_SRV_DIMENSION_BUFFEREX
};
typedef D3D_SRV_DIMENSION D3D11_SRV_DIMENSION;

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
enum D3D11_PRIMITIVE_TOPOLOGY
{
	D3D_PRIMITIVE_TOPOLOGY_UNDEFINED					= 0,
	D3D_PRIMITIVE_TOPOLOGY_POINTLIST					= 1,
	D3D_PRIMITIVE_TOPOLOGY_LINELIST						= 2,
	D3D_PRIMITIVE_TOPOLOGY_LINESTRIP					= 3,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST					= 4,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP				= 5,
	D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ					= 10,
	D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ				= 11,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ				= 12,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ			= 13,
	D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST	= 33,
	D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST	= 34,
	D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST	= 35,
	D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST	= 36,
	D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST	= 37,
	D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST	= 38,
	D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST	= 39,
	D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST	= 40,
	D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST	= 41,
	D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST	= 42,
	D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST	= 43,
	D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST	= 44,
	D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST	= 45,
	D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST	= 46,
	D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST	= 47,
	D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST	= 48,
	D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST	= 49,
	D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST	= 50,
	D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST	= 51,
	D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST	= 52,
	D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST	= 53,
	D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST	= 54,
	D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST	= 55,
	D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST	= 56,
	D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST	= 57,
	D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST	= 58,
	D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST	= 59,
	D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST	= 60,
	D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST	= 61,
	D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST	= 62,
	D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST	= 63,
	D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST	= 64,
	D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED					= D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
	D3D10_PRIMITIVE_TOPOLOGY_POINTLIST					= D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
	D3D10_PRIMITIVE_TOPOLOGY_LINELIST					= D3D_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP					= D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST				= D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP				= D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ				= D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
	D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ				= D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ			= D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
	D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ			= D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,
	D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED					= D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
	D3D11_PRIMITIVE_TOPOLOGY_POINTLIST					= D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST					= D3D_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP					= D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST				= D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP				= D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ				= D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ				= D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ			= D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ			= D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,
	D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST,
	D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST	= D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST
};

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
struct D3D_SHADER_MACRO
{
	LPCSTR Name;
	LPCSTR Definition;
};
enum D3D_INCLUDE_TYPE
{
	D3D_INCLUDE_LOCAL		= 0,
	D3D_INCLUDE_SYSTEM		= (D3D_INCLUDE_LOCAL + 1),
	D3D10_INCLUDE_LOCAL		= D3D_INCLUDE_LOCAL,
	D3D10_INCLUDE_SYSTEM	= D3D_INCLUDE_SYSTEM,
	D3D_INCLUDE_FORCE_DWORD	= 0x7fffffff
};
DECLARE_INTERFACE(ID3DInclude)
{
	STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) PURE;
	STDMETHOD(Close)(THIS_ LPCVOID pData) PURE;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10shader.h"
typedef D3D_SHADER_MACRO D3D10_SHADER_MACRO;
typedef __interface ID3DInclude *LPD3D10INCLUDE;

// "Microsoft DirectX SDK (June 2010)" -> "DXGIFormat.h"
enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN	                    = 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
	DXGI_FORMAT_R32G32B32A32_UINT           = 3,
	DXGI_FORMAT_R32G32B32A32_SINT           = 4,
	DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
	DXGI_FORMAT_R32G32B32_FLOAT             = 6,
	DXGI_FORMAT_R32G32B32_UINT              = 7,
	DXGI_FORMAT_R32G32B32_SINT              = 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
	DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
	DXGI_FORMAT_R16G16B16A16_UINT           = 12,
	DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
	DXGI_FORMAT_R16G16B16A16_SINT           = 14,
	DXGI_FORMAT_R32G32_TYPELESS             = 15,
	DXGI_FORMAT_R32G32_FLOAT                = 16,
	DXGI_FORMAT_R32G32_UINT                 = 17,
	DXGI_FORMAT_R32G32_SINT                 = 18,
	DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
	DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
	DXGI_FORMAT_R10G10B10A2_UINT            = 25,
	DXGI_FORMAT_R11G11B10_FLOAT             = 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
	DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
	DXGI_FORMAT_R8G8B8A8_UINT               = 30,
	DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
	DXGI_FORMAT_R8G8B8A8_SINT               = 32,
	DXGI_FORMAT_R16G16_TYPELESS             = 33,
	DXGI_FORMAT_R16G16_FLOAT                = 34,
	DXGI_FORMAT_R16G16_UNORM                = 35,
	DXGI_FORMAT_R16G16_UINT                 = 36,
	DXGI_FORMAT_R16G16_SNORM                = 37,
	DXGI_FORMAT_R16G16_SINT                 = 38,
	DXGI_FORMAT_R32_TYPELESS                = 39,
	DXGI_FORMAT_D32_FLOAT                   = 40,
	DXGI_FORMAT_R32_FLOAT                   = 41,
	DXGI_FORMAT_R32_UINT                    = 42,
	DXGI_FORMAT_R32_SINT                    = 43,
	DXGI_FORMAT_R24G8_TYPELESS              = 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
	DXGI_FORMAT_R8G8_TYPELESS               = 48,
	DXGI_FORMAT_R8G8_UNORM                  = 49,
	DXGI_FORMAT_R8G8_UINT                   = 50,
	DXGI_FORMAT_R8G8_SNORM                  = 51,
	DXGI_FORMAT_R8G8_SINT                   = 52,
	DXGI_FORMAT_R16_TYPELESS                = 53,
	DXGI_FORMAT_R16_FLOAT                   = 54,
	DXGI_FORMAT_D16_UNORM                   = 55,
	DXGI_FORMAT_R16_UNORM                   = 56,
	DXGI_FORMAT_R16_UINT                    = 57,
	DXGI_FORMAT_R16_SNORM                   = 58,
	DXGI_FORMAT_R16_SINT                    = 59,
	DXGI_FORMAT_R8_TYPELESS                 = 60,
	DXGI_FORMAT_R8_UNORM                    = 61,
	DXGI_FORMAT_R8_UINT                     = 62,
	DXGI_FORMAT_R8_SNORM                    = 63,
	DXGI_FORMAT_R8_SINT                     = 64,
	DXGI_FORMAT_A8_UNORM                    = 65,
	DXGI_FORMAT_R1_UNORM                    = 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
	DXGI_FORMAT_BC1_TYPELESS                = 70,
	DXGI_FORMAT_BC1_UNORM                   = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
	DXGI_FORMAT_BC2_TYPELESS                = 73,
	DXGI_FORMAT_BC2_UNORM                   = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
	DXGI_FORMAT_BC3_TYPELESS                = 76,
	DXGI_FORMAT_BC3_UNORM                   = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
	DXGI_FORMAT_BC4_TYPELESS                = 79,
	DXGI_FORMAT_BC4_UNORM                   = 80,
	DXGI_FORMAT_BC4_SNORM                   = 81,
	DXGI_FORMAT_BC5_TYPELESS                = 82,
	DXGI_FORMAT_BC5_UNORM                   = 83,
	DXGI_FORMAT_BC5_SNORM                   = 84,
	DXGI_FORMAT_B5G6R5_UNORM                = 85,
	DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
	DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
	DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
	DXGI_FORMAT_BC6H_TYPELESS               = 94,
	DXGI_FORMAT_BC6H_UF16                   = 95,
	DXGI_FORMAT_BC6H_SF16                   = 96,
	DXGI_FORMAT_BC7_TYPELESS                = 97,
	DXGI_FORMAT_BC7_UNORM                   = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
	DXGI_FORMAT_FORCE_UINT                  = 0xffffffff
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
inline UINT D3D11CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT MipLevels)
{
	return MipSlice + ArraySlice * MipLevels;
}

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_CREATE_DEVICE_FLAG
{
	D3D11_CREATE_DEVICE_SINGLETHREADED								= 0x1,
	D3D11_CREATE_DEVICE_DEBUG										= 0x2,
	D3D11_CREATE_DEVICE_SWITCH_TO_REF								= 0x4,
	D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS	= 0x8,
	D3D11_CREATE_DEVICE_BGRA_SUPPORT								= 0x20
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_BOX
{
	UINT left;
	UINT top;
	UINT front;
	UINT right;
	UINT bottom;
	UINT back;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_VIEWPORT
{
	FLOAT TopLeftX;
	FLOAT TopLeftY;
	FLOAT Width;
	FLOAT Height;
	FLOAT MinDepth;
	FLOAT MaxDepth;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef RECT D3D11_RECT;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_RTV_DIMENSION
{
	D3D11_RTV_DIMENSION_UNKNOWN				= 0,
	D3D11_RTV_DIMENSION_BUFFER				= 1,
	D3D11_RTV_DIMENSION_TEXTURE1D			= 2,
	D3D11_RTV_DIMENSION_TEXTURE1DARRAY		= 3,
	D3D11_RTV_DIMENSION_TEXTURE2D			= 4,
	D3D11_RTV_DIMENSION_TEXTURE2DARRAY		= 5,
	D3D11_RTV_DIMENSION_TEXTURE2DMS			= 6,
	D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
	D3D11_RTV_DIMENSION_TEXTURE3D			= 8
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_DSV_DIMENSION
{
	D3D11_DSV_DIMENSION_UNKNOWN				= 0,
	D3D11_DSV_DIMENSION_TEXTURE1D			= 1,
	D3D11_DSV_DIMENSION_TEXTURE1DARRAY		= 2,
	D3D11_DSV_DIMENSION_TEXTURE2D			= 3,
	D3D11_DSV_DIMENSION_TEXTURE2DARRAY		= 4,
	D3D11_DSV_DIMENSION_TEXTURE2DMS			= 5,
	D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY	= 6
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
struct DXGI_SAMPLE_DESC
{
	UINT Count;
	UINT Quality;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct IDXGIObject : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(__in REFGUID Name, UINT DataSize, __in_bcount(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(__in REFGUID Name, __in const IUnknown *pUnknown) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(__in REFGUID Name, __inout UINT *pDataSize, __out_bcount(*pDataSize) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetParent(__in REFIID riid, __out void **ppParent) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
struct DXGI_RATIONAL
{
	UINT Numerator;
	UINT Denominator;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
enum DXGI_MODE_SCANLINE_ORDER
{
	DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED		= 0,
	DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE		= 1,
	DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST	= 2,
	DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST	= 3
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
enum DXGI_MODE_SCALING
{
	DXGI_MODE_SCALING_UNSPECIFIED	= 0,
	DXGI_MODE_SCALING_CENTERED		= 1,
	DXGI_MODE_SCALING_STRETCHED		= 2
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
struct DXGI_MODE_DESC
{
	UINT Width;
	UINT Height;
	DXGI_RATIONAL RefreshRate;
	DXGI_FORMAT Format;
	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
	DXGI_MODE_SCALING Scaling;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
typedef UINT DXGI_USAGE;

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
enum DXGI_SWAP_EFFECT
{
	DXGI_SWAP_EFFECT_DISCARD			= 0,
	DXGI_SWAP_EFFECT_SEQUENTIAL			= 1,
	DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL	= 3,
	DXGI_SWAP_EFFECT_FLIP_DISCARD		= 4
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
typedef enum DXGI_SWAP_CHAIN_FLAG
{
	DXGI_SWAP_CHAIN_FLAG_NONPREROTATED							= 1,
	DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH						= 2,
	DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE							= 4,
	DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT						= 8,
	DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER		= 16,
	DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY							= 32,
	DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT			= 64,
	DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER						= 128,
	DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO						= 256,
	DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO								= 512,
	DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED							= 1024,
	DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING							= 2048,
	DXGI_SWAP_CHAIN_FLAG_RESTRICTED_TO_ALL_HOLOGRAPHIC_DISPLAYS	= 4096
} DXGI_SWAP_CHAIN_FLAG;

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct DXGI_SWAP_CHAIN_DESC
{
	DXGI_MODE_DESC BufferDesc;
	DXGI_SAMPLE_DESC SampleDesc;
	DXGI_USAGE BufferUsage;
	UINT BufferCount;
	HWND OutputWindow;
	BOOL Windowed;
	DXGI_SWAP_EFFECT SwapEffect;
	UINT Flags;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
MIDL_INTERFACE("7b7166ec-21c7-44ae-b21a-c9ae321ae369")
IDXGIFactory : public IDXGIObject
{
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, __out IDXGIAdapter **ppAdapter) = 0;
		virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(__out HWND *pWindowHandle) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(__in IUnknown *pDevice, __in DXGI_SWAP_CHAIN_DESC *pDesc, __out IDXGISwapChain **ppSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, __out IDXGIAdapter **ppAdapter) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
MIDL_INTERFACE("770aae78-f26f-4dba-a829-253c83d1b387")
IDXGIFactory1 : public IDXGIFactory
{
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, __out IDXGIAdapter1 **ppAdapter) = 0;
		virtual BOOL STDMETHODCALLTYPE IsCurrent(void) = 0;
};

// "Windows 10 SDK" -> "dxgi1_2.h"
typedef enum DXGI_SCALING
{
	DXGI_SCALING_STRETCH				= 0,
	DXGI_SCALING_NONE					= 1,
	DXGI_SCALING_ASPECT_RATIO_STRETCH	= 2
} DXGI_SCALING;

// "Windows 10 SDK" -> "dxgi1_2.h"
typedef enum DXGI_ALPHA_MODE
{
	DXGI_ALPHA_MODE_UNSPECIFIED		= 0,
	DXGI_ALPHA_MODE_PREMULTIPLIED	= 1,
	DXGI_ALPHA_MODE_STRAIGHT		= 2,
	DXGI_ALPHA_MODE_IGNORE			= 3,
	DXGI_ALPHA_MODE_FORCE_DWORD		= 0xffffffff
} DXGI_ALPHA_MODE;

// "Windows 10 SDK" -> "dxgi1_2.h"
typedef struct DXGI_SWAP_CHAIN_DESC1
{
	UINT Width;
	UINT Height;
	DXGI_FORMAT Format;
	BOOL Stereo;
	DXGI_SAMPLE_DESC SampleDesc;
	DXGI_USAGE BufferUsage;
	UINT BufferCount;
	DXGI_SCALING Scaling;
	DXGI_SWAP_EFFECT SwapEffect;
	DXGI_ALPHA_MODE AlphaMode;
	UINT Flags;
} DXGI_SWAP_CHAIN_DESC1;

// "Windows 10 SDK" -> "dxgi1_2.h"
typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC
{
	DXGI_RATIONAL RefreshRate;
	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
	DXGI_MODE_SCALING Scaling;
	BOOL Windowed;
} DXGI_SWAP_CHAIN_FULLSCREEN_DESC;

// "Windows 10 SDK" -> "dxgi1_2.h"
MIDL_INTERFACE("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
IDXGIFactory2 : public IDXGIFactory1
{
	public:
		virtual BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(_In_ IUnknown *pDevice, _In_ HWND hWnd, _In_ const DXGI_SWAP_CHAIN_DESC1 *pDesc, _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, _In_opt_ IDXGIOutput *pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1 **ppSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(_In_ IUnknown *pDevice, _In_ IUnknown *pWindow, _In_ const DXGI_SWAP_CHAIN_DESC1 *pDesc, _In_opt_ IDXGIOutput *pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1 **ppSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(_In_ HANDLE hResource, _Out_ LUID *pLuid) = 0;
		virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(_In_ HWND WindowHandle, _In_ UINT wMsg, _Out_ DWORD *pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(_In_ HANDLE hEvent, _Out_ DWORD *pdwCookie) = 0;
		virtual void STDMETHODCALLTYPE UnregisterStereoStatus(_In_ DWORD dwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(_In_ HWND WindowHandle, _In_ UINT wMsg, _Out_ DWORD *pdwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(_In_ HANDLE hEvent, _Out_ DWORD *pdwCookie) = 0;
		virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus(_In_ DWORD dwCookie) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(_In_ IUnknown *pDevice, _In_ const DXGI_SWAP_CHAIN_DESC1 *pDesc, _In_opt_ IDXGIOutput *pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1 **ppSwapChain) = 0;
};

// "Windows 10 SDK" -> "dxgi1_3.h"
MIDL_INTERFACE("25483823-cd46-4c7d-86ca-47aa95b837bd")
IDXGIFactory3 : public IDXGIFactory2
{
	public:
		virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;
};

// "Windows 10 SDK" -> "dxgi1_4.h"
MIDL_INTERFACE("1bc6ea02-ef36-464f-bf0c-21ca39e5168a")
IDXGIFactory4 : public IDXGIFactory3
{
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(_In_ LUID AdapterLuid, _In_ REFIID riid, _COM_Outptr_ void **ppvAdapter) = 0;
		virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter(_In_ REFIID riid, _COM_Outptr_ void **ppvAdapter) = 0;
};

// "Windows 10 SDK" -> "dxgi1_5.h"
typedef enum DXGI_FEATURE
{
	DXGI_FEATURE_PRESENT_ALLOW_TEARING = 0
} DXGI_FEATURE;

// "Windows 10 SDK" -> "dxgi1_5.h"
MIDL_INTERFACE("7632e1f5-ee65-4dca-87fd-84cd75f8838d")
IDXGIFactory5 : public IDXGIFactory4
{
	public:
		virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(DXGI_FEATURE Feature, _Inout_updates_bytes_(FeatureSupportDataSize) void *pFeatureSupportData, UINT FeatureSupportDataSize) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct DXGI_ADAPTER_DESC
{
	WCHAR Description[128];
	UINT VendorId;
	UINT DeviceId;
	UINT SubSysId;
	UINT Revision;
	SIZE_T DedicatedVideoMemory;
	SIZE_T DedicatedSystemMemory;
	SIZE_T SharedSystemMemory;
	LUID AdapterLuid;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct IDXGIAdapter : public IDXGIObject
{
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumOutputs(UINT Output, __out IDXGIOutput **ppOutput) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetDesc(__out DXGI_ADAPTER_DESC *pDesc) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(__in REFGUID InterfaceName, __out LARGE_INTEGER *pUMDVersion) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct DXGI_SURFACE_DESC
{
	UINT Width;
	UINT Height;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct DXGI_SHARED_RESOURCE
{
	HANDLE Handle;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
enum DXGI_RESIDENCY
{
	DXGI_RESIDENCY_FULLY_RESIDENT				= 1,
	DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY	= 2,
	DXGI_RESIDENCY_EVICTED_TO_DISK				= 3
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
MIDL_INTERFACE("54ec77fa-1377-44e6-8c32-88fd5f44c84c")
IDXGIDevice : public IDXGIObject
{
	public:
		virtual HRESULT STDMETHODCALLTYPE GetAdapter(__out IDXGIAdapter **pAdapter) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSurface(__in const DXGI_SURFACE_DESC *pDesc, UINT NumSurfaces, DXGI_USAGE Usage, __in_opt const DXGI_SHARED_RESOURCE *pSharedResource, __out IDXGISurface **ppSurface) = 0;
		virtual HRESULT STDMETHODCALLTYPE QueryResourceResidency(__in_ecount(NumResources) IUnknown *const *ppResources, __out_ecount(NumResources) DXGI_RESIDENCY *pResidencyStatus, UINT NumResources) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT Priority) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(__out INT *pPriority) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct IDXGIDeviceSubObject : public IDXGIObject
{
	public:
		virtual HRESULT STDMETHODCALLTYPE GetDevice(__in REFIID riid, __out void **ppDevice) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
struct IDXGISwapChain : public IDXGIDeviceSubObject
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, __in REFIID riid, __out void **ppSurface) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, __in_opt IDXGIOutput *pTarget) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(__out BOOL *pFullscreen, __out IDXGIOutput **ppTarget) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetDesc(__out DXGI_SWAP_CHAIN_DESC *pDesc) = 0;
		virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) = 0;
		virtual HRESULT STDMETHODCALLTYPE ResizeTarget(__in const DXGI_MODE_DESC *pNewTargetParameters) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(__out IDXGIOutput **ppOutput) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(__out DXGI_FRAME_STATISTICS *pStats) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(__out UINT *pLastPresentCount) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_DEVICE_CONTEXT_TYPE
{
	D3D11_DEVICE_CONTEXT_IMMEDIATE	= 0,
	D3D11_DEVICE_CONTEXT_DEFERRED	= (D3D11_DEVICE_CONTEXT_IMMEDIATE + 1)
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_CLEAR_FLAG
{
	D3D11_CLEAR_DEPTH	= 0x1L,
	D3D11_CLEAR_STENCIL	= 0x2L
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_USAGE
{
	D3D11_USAGE_DEFAULT		= 0,
	D3D11_USAGE_IMMUTABLE	= 1,
	D3D11_USAGE_DYNAMIC		= 2,
	D3D11_USAGE_STAGING		= 3
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_MAP
{
	D3D11_MAP_READ					= 1,
	D3D11_MAP_WRITE					= 2,
	D3D11_MAP_READ_WRITE			= 3,
	D3D11_MAP_WRITE_DISCARD			= 4,
	D3D11_MAP_WRITE_NO_OVERWRITE	= 5
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_BIND_FLAG
{
	D3D11_BIND_VERTEX_BUFFER	= 0x1L,
	D3D11_BIND_INDEX_BUFFER		= 0x2L,
	D3D11_BIND_CONSTANT_BUFFER	= 0x4L,
	D3D11_BIND_SHADER_RESOURCE	= 0x8L,
	D3D11_BIND_STREAM_OUTPUT	= 0x10L,
	D3D11_BIND_RENDER_TARGET	= 0x20L,
	D3D11_BIND_DEPTH_STENCIL	= 0x40L,
	D3D11_BIND_UNORDERED_ACCESS	= 0x80L
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_CPU_ACCESS_FLAG
{
	D3D11_CPU_ACCESS_WRITE	= 0x10000L,
	D3D11_CPU_ACCESS_READ	= 0x20000L
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_RESOURCE_MISC_FLAG
{
	D3D11_RESOURCE_MISC_GENERATE_MIPS					= 0x1L,
	D3D11_RESOURCE_MISC_SHARED							= 0x2L,
	D3D11_RESOURCE_MISC_TEXTURECUBE						= 0x4L,
	D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS				= 0x10L,
	D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS			= 0x20L,
	D3D11_RESOURCE_MISC_BUFFER_STRUCTURED				= 0x40L,
	D3D11_RESOURCE_MISC_RESOURCE_CLAMP					= 0x80L,
	D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX				= 0x100L,
	D3D11_RESOURCE_MISC_GDI_COMPATIBLE					= 0x200L,
	D3D11_RESOURCE_MISC_SHARED_NTHANDLE					= 0x800L,
	D3D11_RESOURCE_MISC_RESTRICTED_CONTENT				= 0x1000L,
	D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE		= 0x2000L,
	D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER	= 0x4000L,
	D3D11_RESOURCE_MISC_GUARDED							= 0x8000L,
	D3D11_RESOURCE_MISC_TILE_POOL						= 0x20000L,
	D3D11_RESOURCE_MISC_TILED							= 0x40000L
} D3D11_RESOURCE_MISC_FLAG;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_BUFFER_DESC
{
	UINT ByteWidth;
	D3D11_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
	UINT StructureByteStride;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_BUFFER_RTV
{
	union
	{
		UINT FirstElement;
		UINT ElementOffset;
	};
	union
	{
		UINT NumElements;
		UINT ElementWidth;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX1D_RTV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX1D_ARRAY_RTV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2D_RTV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2DMS_RTV
{
	UINT UnusedField_NothingToDefine;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2D_ARRAY_RTV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2DMS_ARRAY_RTV
{
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX3D_RTV
{
	UINT MipSlice;
	UINT FirstWSlice;
	UINT WSize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEXTURE1D_DESC
{
	UINT Width;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	D3D11_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
} D3D11_TEXTURE1D_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEXTURE2D_DESC
{
	UINT Width;
	UINT Height;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
	D3D11_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEXTURE3D_DESC
{
	UINT Width;
	UINT Height;
	UINT Depth;
	UINT MipLevels;
	DXGI_FORMAT Format;
	D3D11_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
} D3D11_TEXTURE3D_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX1D_DSV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX1D_ARRAY_DSV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2D_DSV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2D_ARRAY_DSV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2DMS_DSV
{
	UINT UnusedField_NothingToDefine;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2DMS_ARRAY_DSV
{
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_SUBRESOURCE_DATA
{
	const void *pSysMem;
	UINT SysMemPitch;
	UINT SysMemSlicePitch;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_MAPPED_SUBRESOURCE
{
	void *pData;
	UINT RowPitch;
	UINT DepthPitch;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_BUFFER_SRV
{
	union
	{
		UINT FirstElement;
		UINT ElementOffset;
	};
	union
	{
		UINT NumElements;
		UINT ElementWidth;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_BUFFEREX_SRV_FLAG
{
	D3D11_BUFFEREX_SRV_FLAG_RAW = 0x1
} D3D11_BUFFEREX_SRV_FLAG;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX1D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX1D_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2D_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX3D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEXCUBE_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEXCUBE_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT First2DArrayFace;
	UINT NumCubes;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2DMS_SRV
{
	UINT UnusedField_NothingToDefine;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_TEX2DMS_ARRAY_SRV
{
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_BUFFEREX_SRV
{
	UINT FirstElement;
	UINT NumElements;
	UINT Flags;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_SHADER_RESOURCE_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D11_SRV_DIMENSION ViewDimension;
	union
	{
		D3D11_BUFFER_SRV Buffer;
		D3D11_TEX1D_SRV Texture1D;
		D3D11_TEX1D_ARRAY_SRV Texture1DArray;
		D3D11_TEX2D_SRV Texture2D;
		D3D11_TEX2D_ARRAY_SRV Texture2DArray;
		D3D11_TEX2DMS_SRV Texture2DMS;
		D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
		D3D11_TEX3D_SRV Texture3D;
		D3D11_TEXCUBE_SRV TextureCube;
		D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
		D3D11_BUFFEREX_SRV BufferEx;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_UAV_DIMENSION
{
	D3D11_UAV_DIMENSION_UNKNOWN			= 0,
	D3D11_UAV_DIMENSION_BUFFER			= 1,
	D3D11_UAV_DIMENSION_TEXTURE1D		= 2,
	D3D11_UAV_DIMENSION_TEXTURE1DARRAY	= 3,
	D3D11_UAV_DIMENSION_TEXTURE2D		= 4,
	D3D11_UAV_DIMENSION_TEXTURE2DARRAY	= 5,
	D3D11_UAV_DIMENSION_TEXTURE3D		= 8
} D3D11_UAV_DIMENSION;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_BUFFER_UAV_FLAG
{
	D3D11_BUFFER_UAV_FLAG_RAW		= 0x1,
	D3D11_BUFFER_UAV_FLAG_APPEND	= 0x2,
	D3D11_BUFFER_UAV_FLAG_COUNTER	= 0x4
} D3D11_BUFFER_UAV_FLAG;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_BUFFER_UAV
{
	UINT FirstElement;
	UINT NumElements;
	UINT Flags;
} D3D11_BUFFER_UAV;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEX1D_UAV
{
	UINT MipSlice;
} D3D11_TEX1D_UAV;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEX1D_ARRAY_UAV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D11_TEX1D_ARRAY_UAV;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEX2D_UAV
{
	UINT MipSlice;
} D3D11_TEX2D_UAV;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEX2D_ARRAY_UAV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D11_TEX2D_ARRAY_UAV;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_TEX3D_UAV
{
	UINT MipSlice;
	UINT FirstWSlice;
	UINT WSize;
} D3D11_TEX3D_UAV;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_UNORDERED_ACCESS_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D11_UAV_DIMENSION ViewDimension;
	union
	{
		D3D11_BUFFER_UAV Buffer;
		D3D11_TEX1D_UAV Texture1D;
		D3D11_TEX1D_ARRAY_UAV Texture1DArray;
		D3D11_TEX2D_UAV Texture2D;
		D3D11_TEX2D_ARRAY_UAV Texture2DArray;
		D3D11_TEX3D_UAV Texture3D;
	};
} D3D11_UNORDERED_ACCESS_VIEW_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_DEPTH_STENCIL_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D11_DSV_DIMENSION ViewDimension;
	UINT Flags;
	union
	{
		D3D11_TEX1D_DSV Texture1D;
		D3D11_TEX1D_ARRAY_DSV Texture1DArray;
		D3D11_TEX2D_DSV Texture2D;
		D3D11_TEX2D_ARRAY_DSV Texture2DArray;
		D3D11_TEX2DMS_DSV Texture2DMS;
		D3D11_TEX2DMS_ARRAY_DSV Texture2DMSArray;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_RENDER_TARGET_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D11_RTV_DIMENSION ViewDimension;
	union
	{
		D3D11_BUFFER_RTV Buffer;
		D3D11_TEX1D_RTV Texture1D;
		D3D11_TEX1D_ARRAY_RTV Texture1DArray;
		D3D11_TEX2D_RTV Texture2D;
		D3D11_TEX2D_ARRAY_RTV Texture2DArray;
		D3D11_TEX2DMS_RTV Texture2DMS;
		D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
		D3D11_TEX3D_RTV Texture3D;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_INPUT_CLASSIFICATION
{
	D3D11_INPUT_PER_VERTEX_DATA		= 0,
	D3D11_INPUT_PER_INSTANCE_DATA	= 1
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_INPUT_ELEMENT_DESC
{
	LPCSTR SemanticName;
	UINT SemanticIndex;
	DXGI_FORMAT Format;
	UINT InputSlot;
	UINT AlignedByteOffset;
	D3D11_INPUT_CLASSIFICATION InputSlotClass;
	UINT InstanceDataStepRate;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_FILL_MODE
{
	D3D11_FILL_WIREFRAME	= 2,
	D3D11_FILL_SOLID		= 3
} D3D11_FILL_MODE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_CULL_MODE
{
	D3D11_CULL_NONE		= 1,
	D3D11_CULL_FRONT	= 2,
	D3D11_CULL_BACK		= 3 
} D3D11_CULL_MODE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_RASTERIZER_DESC
{
	D3D11_FILL_MODE FillMode;
	D3D11_CULL_MODE CullMode;
	BOOL            FrontCounterClockwise;
	INT             DepthBias;
	FLOAT           DepthBiasClamp;
	FLOAT           SlopeScaledDepthBias;
	BOOL            DepthClipEnable;
	BOOL            ScissorEnable;
	BOOL            MultisampleEnable;
	BOOL            AntialiasedLineEnable;
} D3D11_RASTERIZER_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_FILTER
{
	D3D11_FILTER_MIN_MAG_MIP_POINT							= 0,
	D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR					= 0x1,
	D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT				= 0x4,
	D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR					= 0x5,
	D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT					= 0x10,
	D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR			= 0x11,
	D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT					= 0x14,
	D3D11_FILTER_MIN_MAG_MIP_LINEAR							= 0x15,
	D3D11_FILTER_ANISOTROPIC								= 0x55,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT				= 0x80,
	D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR		= 0x81,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR		= 0x85,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT		= 0x90,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x91,
	D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT		= 0x94,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR				= 0x95,
	D3D11_FILTER_COMPARISON_ANISOTROPIC						= 0xd5,
	D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT					= 0x100,
	D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR			= 0x101,
	D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT		= 0x104,
	D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR			= 0x105,
	D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT			= 0x110,
	D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x111,
	D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT			= 0x114,
	D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR					= 0x115,
	D3D11_FILTER_MINIMUM_ANISOTROPIC						= 0x155,
	D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT					= 0x180,
	D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR			= 0x181,
	D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT		= 0x184,
	D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR			= 0x185,
	D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT			= 0x190,
	D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x191,
	D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT			= 0x194,
	D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR					= 0x195,
	D3D11_FILTER_MAXIMUM_ANISOTROPIC						= 0x1d5
} D3D11_FILTER;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_TEXTURE_ADDRESS_MODE
{
	D3D11_TEXTURE_ADDRESS_WRAP			= 1,
	D3D11_TEXTURE_ADDRESS_MIRROR		= 2,
	D3D11_TEXTURE_ADDRESS_CLAMP			= 3,
	D3D11_TEXTURE_ADDRESS_BORDER		= 4,
	D3D11_TEXTURE_ADDRESS_MIRROR_ONCE	= 5
} D3D11_TEXTURE_ADDRESS_MODE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_COMPARISON_FUNC
{
	D3D11_COMPARISON_NEVER			= 1,
	D3D11_COMPARISON_LESS			= 2,
	D3D11_COMPARISON_EQUAL			= 3,
	D3D11_COMPARISON_LESS_EQUAL		= 4,
	D3D11_COMPARISON_GREATER		= 5,
	D3D11_COMPARISON_NOT_EQUAL		= 6,
	D3D11_COMPARISON_GREATER_EQUAL	= 7,
	D3D11_COMPARISON_ALWAYS			= 8
} D3D11_COMPARISON_FUNC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_SAMPLER_DESC
{
	D3D11_FILTER				Filter;
	D3D11_TEXTURE_ADDRESS_MODE	AddressU;
	D3D11_TEXTURE_ADDRESS_MODE	AddressV;
	D3D11_TEXTURE_ADDRESS_MODE	AddressW;
	FLOAT						MipLODBias;
	UINT						MaxAnisotropy;
	D3D11_COMPARISON_FUNC		ComparisonFunc;
	FLOAT						BorderColor[4];
	FLOAT						MinLOD;
	FLOAT						MaxLOD;
} D3D11_SAMPLER_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_BLEND
{
	D3D11_BLEND_ZERO				= 1,
	D3D11_BLEND_ONE					= 2,
	D3D11_BLEND_SRC_COLOR			= 3,
	D3D11_BLEND_INV_SRC_COLOR		= 4,
	D3D11_BLEND_SRC_ALPHA			= 5,
	D3D11_BLEND_INV_SRC_ALPHA		= 6,
	D3D11_BLEND_DEST_ALPHA			= 7,
	D3D11_BLEND_INV_DEST_ALPHA		= 8,
	D3D11_BLEND_DEST_COLOR			= 9,
	D3D11_BLEND_INV_DEST_COLOR		= 10,
	D3D11_BLEND_SRC_ALPHA_SAT		= 11,
	D3D11_BLEND_BLEND_FACTOR		= 14,
	D3D11_BLEND_INV_BLEND_FACTOR	= 15,
	D3D11_BLEND_SRC1_COLOR			= 16,
	D3D11_BLEND_INV_SRC1_COLOR		= 17,
	D3D11_BLEND_SRC1_ALPHA			= 18,
	D3D11_BLEND_INV_SRC1_ALPHA		= 19
} D3D11_BLEND;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef enum D3D11_BLEND_OP
{
	D3D11_BLEND_OP_ADD			= 1,
	D3D11_BLEND_OP_SUBTRACT		= 2,
	D3D11_BLEND_OP_REV_SUBTRACT	= 3,
	D3D11_BLEND_OP_MIN			= 4,
	D3D11_BLEND_OP_MAX			= 5
} D3D11_BLEND_OP;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_RENDER_TARGET_BLEND_DESC
{
	BOOL			BlendEnable;
	D3D11_BLEND		SrcBlend;
	D3D11_BLEND		DestBlend;
	D3D11_BLEND_OP	BlendOp;
	D3D11_BLEND		SrcBlendAlpha;
	D3D11_BLEND		DestBlendAlpha;
	D3D11_BLEND_OP	BlendOpAlpha;
	UINT8			RenderTargetWriteMask;
} D3D11_RENDER_TARGET_BLEND_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_BLEND_DESC
{
	BOOL							AlphaToCoverageEnable;
	BOOL							IndependentBlendEnable;
	D3D11_RENDER_TARGET_BLEND_DESC	RenderTarget[8];
} D3D11_BLEND_DESC;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11Device : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE CreateBuffer(__in const D3D11_BUFFER_DESC *pDesc, __in_opt const D3D11_SUBRESOURCE_DATA *pInitialData, __out_opt ID3D11Buffer* *ppBuffer) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(__in const D3D11_TEXTURE1D_DESC *pDesc, __in_xcount_opt(pDesc->MipLevels * pDesc->ArraySize) const D3D11_SUBRESOURCE_DATA *pInitialData, __out_opt ID3D11Texture1D **ppTexture1D) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(__in const D3D11_TEXTURE2D_DESC *pDesc, __in_xcount_opt(pDesc->MipLevels * pDesc->ArraySize) const D3D11_SUBRESOURCE_DATA *pInitialData, __out_opt ID3D11Texture2D **ppTexture2D) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(__in const D3D11_TEXTURE3D_DESC *pDesc, __in_xcount_opt(pDesc->MipLevels) const D3D11_SUBRESOURCE_DATA *pInitialData, __out_opt ID3D11Texture3D **ppTexture3D) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(__in ID3D11Resource *pResource, __in_opt const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, __out_opt ID3D11ShaderResourceView* *ppSRView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(__in ID3D11Resource *pResource, __in_opt const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, __out_opt ID3D11UnorderedAccessView **ppUAView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(__in ID3D11Resource *pResource, __in_opt const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, __out_opt ID3D11RenderTargetView **ppRTView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(__in ID3D11Resource *pResource, __in_opt const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, __out_opt ID3D11DepthStencilView **ppDepthStencilView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(__in_ecount(NumElements) const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, __in_range(0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT) UINT NumElements, __in const void *pShaderBytecodeWithInputSignature, __in SIZE_T BytecodeLength, __out_opt ID3D11InputLayout **ppInputLayout) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11VertexShader **ppVertexShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11GeometryShader **ppGeometryShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_ecount_opt(NumEntries) const D3D11_SO_DECLARATION_ENTRY *pSODeclaration, __in_range(0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT) UINT NumEntries, __in_ecount_opt(NumStrides) const UINT *pBufferStrides, __in_range(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumStrides, __in UINT RasterizedStream, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11GeometryShader **ppGeometryShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11PixelShader **ppPixelShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateHullShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11HullShader **ppHullShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateDomainShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11DomainShader **ppDomainShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateComputeShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_opt ID3D11ClassLinkage *pClassLinkage, __out_opt ID3D11ComputeShader **ppComputeShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(__out ID3D11ClassLinkage **ppLinkage) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateBlendState(__in const D3D11_BLEND_DESC *pBlendStateDesc, __out_opt ID3D11BlendState **ppBlendState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(__in const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, __out_opt ID3D11DepthStencilState **ppDepthStencilState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(__in const D3D11_RASTERIZER_DESC *pRasterizerDesc, __out_opt ID3D11RasterizerState **ppRasterizerState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(__in const D3D11_SAMPLER_DESC *pSamplerDesc, __out_opt ID3D11SamplerState **ppSamplerState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateQuery(__in const D3D11_QUERY_DESC *pQueryDesc, __out_opt ID3D11Query **ppQuery) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreatePredicate(__in const D3D11_QUERY_DESC *pPredicateDesc, __out_opt ID3D11Predicate **ppPredicate) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateCounter(__in const D3D11_COUNTER_DESC *pCounterDesc, __out_opt ID3D11Counter **ppCounter) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext(UINT ContextFlags, __out_opt ID3D11DeviceContext **ppDeferredContext) = 0;
		virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(__in HANDLE hResource, __in REFIID ReturnedInterface, __out_opt void **ppResource) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(__in DXGI_FORMAT Format, __out UINT *pFormatSupport) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(__in DXGI_FORMAT Format, __in UINT SampleCount, __out UINT *pNumQualityLevels) = 0;
		virtual void STDMETHODCALLTYPE CheckCounterInfo(__out D3D11_COUNTER_INFO *pCounterInfo) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckCounter(__in const D3D11_COUNTER_DESC *pDesc, __out D3D11_COUNTER_TYPE *pType, __out UINT *pActiveCounters, __out_ecount_opt(*pNameLength) LPSTR szName, __inout_opt UINT *pNameLength, __out_ecount_opt(*pUnitsLength) LPSTR szUnits, __inout_opt UINT *pUnitsLength, __out_ecount_opt(*pDescriptionLength) LPSTR szDescription, __inout_opt UINT *pDescriptionLength) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(D3D11_FEATURE Feature, __out_bcount(FeatureSupportDataSize) void *pFeatureSupportData, UINT FeatureSupportDataSize) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(__in REFGUID guid, __inout UINT *pDataSize, __out_bcount_opt(*pDataSize) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(__in REFGUID guid, __in UINT DataSize, __in_bcount_opt(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(__in REFGUID guid, __in_opt const IUnknown *pData) = 0;
		virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel(void) = 0;
		virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void) = 0;
		virtual void STDMETHODCALLTYPE GetImmediateContext(__out ID3D11DeviceContext **ppImmediateContext) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(UINT RaiseFlags) = 0;
		virtual UINT STDMETHODCALLTYPE GetExceptionMode(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11DeviceChild : public IUnknown
{
	public:
		virtual void STDMETHODCALLTYPE GetDevice(__out ID3D11Device **ppDevice) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(__in REFGUID guid, __inout UINT *pDataSize, __out_bcount_opt(*pDataSize) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(__in REFGUID guid, __in UINT DataSize, __in_bcount_opt(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(__in REFGUID guid, __in_opt const IUnknown *pData) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11DeviceContext : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE VSSetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE PSSetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D11ShaderResourceView* const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE PSSetShader(__in_opt ID3D11PixelShader *pPixelShader, __in_ecount_opt(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE PSSetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D11SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE VSSetShader(__in_opt ID3D11VertexShader *pVertexShader, __in_ecount_opt(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE DrawIndexed(__in UINT IndexCount, __in UINT StartIndexLocation, __in INT BaseVertexLocation) = 0;
		virtual void STDMETHODCALLTYPE Draw(__in UINT VertexCount, __in UINT StartVertexLocation) = 0;
		virtual HRESULT STDMETHODCALLTYPE Map(__in ID3D11Resource *pResource, __in UINT Subresource, __in D3D11_MAP MapType, __in UINT MapFlags, __out D3D11_MAPPED_SUBRESOURCE *pMappedResource) = 0;
		virtual void STDMETHODCALLTYPE Unmap(__in ID3D11Resource *pResource, __in UINT Subresource) = 0;
		virtual void STDMETHODCALLTYPE PSSetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE IASetInputLayout(__in_opt ID3D11InputLayout *pInputLayout) = 0;
		virtual void STDMETHODCALLTYPE IASetVertexBuffers(__in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppVertexBuffers, __in_ecount(NumBuffers) const UINT *pStrides, __in_ecount(NumBuffers) const UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE IASetIndexBuffer(__in_opt ID3D11Buffer* pIndexBuffer, __in DXGI_FORMAT Format, __in UINT Offset) = 0;
		virtual void STDMETHODCALLTYPE DrawIndexedInstanced(__in UINT IndexCountPerInstance, __in UINT InstanceCount, __in UINT StartIndexLocation, __in INT BaseVertexLocation, __in UINT StartInstanceLocation) = 0;
		virtual void STDMETHODCALLTYPE DrawInstanced(__in UINT VertexCountPerInstance, __in UINT InstanceCount, __in UINT StartVertexLocation, __in UINT StartInstanceLocation) = 0;
		virtual void STDMETHODCALLTYPE GSSetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE GSSetShader(__in_opt ID3D11GeometryShader *pShader, __in_ecount_opt(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(__in D3D11_PRIMITIVE_TOPOLOGY Topology) = 0;
		virtual void STDMETHODCALLTYPE VSSetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D11ShaderResourceView* const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE VSSetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D11SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE Begin(__in ID3D11Asynchronous *pAsync) = 0;
		virtual void STDMETHODCALLTYPE End(__in ID3D11Asynchronous *pAsync) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetData(__in ID3D11Asynchronous *pAsync, __out_bcount_opt(DataSize) void *pData, __in UINT DataSize, __in UINT GetDataFlags) = 0;
		virtual void STDMETHODCALLTYPE SetPredication(__in_opt ID3D11Predicate *pPredicate, __in BOOL PredicateValue) = 0;
		virtual void STDMETHODCALLTYPE GSSetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D11ShaderResourceView* const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE GSSetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D11SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE OMSetRenderTargets(__in_range(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews, __in_ecount_opt(NumViews) ID3D11RenderTargetView *const *ppRenderTargetViews, __in_opt ID3D11DepthStencilView *pDepthStencilView) = 0;
		virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(__in UINT NumRTVs, __in_ecount_opt(NumRTVs) ID3D11RenderTargetView *const *ppRenderTargetViews, __in_opt ID3D11DepthStencilView *pDepthStencilView, __in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1) UINT UAVStartSlot, __in UINT NumUAVs, __in_ecount_opt(NumUAVs) ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, __in_ecount_opt(NumUAVs) const UINT *pUAVInitialCounts) = 0;
		virtual void STDMETHODCALLTYPE OMSetBlendState(__in_opt ID3D11BlendState *pBlendState, __in_opt const FLOAT BlendFactor[4], __in UINT SampleMask) = 0;
		virtual void STDMETHODCALLTYPE OMSetDepthStencilState(__in_opt ID3D11DepthStencilState *pDepthStencilState, __in UINT StencilRef) = 0;
		virtual void STDMETHODCALLTYPE SOSetTargets(__in_range(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumBuffers, __in_ecount_opt(NumBuffers) ID3D11Buffer* const *ppSOTargets, __in_ecount_opt(NumBuffers) const UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE DrawAuto(void) = 0;
		virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(__in ID3D11Buffer* pBufferForArgs, __in UINT AlignedByteOffsetForArgs) = 0;
		virtual void STDMETHODCALLTYPE DrawInstancedIndirect(__in ID3D11Buffer* pBufferForArgs, __in UINT AlignedByteOffsetForArgs) = 0;
		virtual void STDMETHODCALLTYPE Dispatch(__in UINT ThreadGroupCountX, __in UINT ThreadGroupCountY, __in UINT ThreadGroupCountZ) = 0;
		virtual void STDMETHODCALLTYPE DispatchIndirect(__in ID3D11Buffer* pBufferForArgs, __in UINT AlignedByteOffsetForArgs) = 0;
		virtual void STDMETHODCALLTYPE RSSetState(__in_opt ID3D11RasterizerState *pRasterizerState) = 0;
		virtual void STDMETHODCALLTYPE RSSetViewports(__in_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumViewports, __in_ecount_opt(NumViewports) const D3D11_VIEWPORT *pViewports) = 0;
		virtual void STDMETHODCALLTYPE RSSetScissorRects(__in_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumRects, __in_ecount_opt(NumRects) const D3D11_RECT *pRects) = 0;
		virtual void STDMETHODCALLTYPE CopySubresourceRegion(__in ID3D11Resource *pDstResource, __in UINT DstSubresource, __in UINT DstX, __in UINT DstY, __in UINT DstZ, __in ID3D11Resource *pSrcResource, __in UINT SrcSubresource, __in_opt const D3D11_BOX *pSrcBox) = 0;
		virtual void STDMETHODCALLTYPE CopyResource(__in ID3D11Resource *pDstResource, __in ID3D11Resource *pSrcResource) = 0;
		virtual void STDMETHODCALLTYPE UpdateSubresource(__in ID3D11Resource *pDstResource, __in UINT DstSubresource, __in_opt const D3D11_BOX *pDstBox, __in const void *pSrcData, __in UINT SrcRowPitch, __in UINT SrcDepthPitch) = 0;
		virtual void STDMETHODCALLTYPE CopyStructureCount(__in ID3D11Buffer* pDstBuffer, __in UINT DstAlignedByteOffset, __in ID3D11UnorderedAccessView *pSrcView) = 0;
		virtual void STDMETHODCALLTYPE ClearRenderTargetView(__in ID3D11RenderTargetView *pRenderTargetView, __in const FLOAT ColorRGBA[4]) = 0;
		virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(__in ID3D11UnorderedAccessView *pUnorderedAccessView, __in const UINT Values[4]) = 0;
		virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(__in ID3D11UnorderedAccessView *pUnorderedAccessView, __in const FLOAT Values[4]) = 0;
		virtual void STDMETHODCALLTYPE ClearDepthStencilView(__in ID3D11DepthStencilView *pDepthStencilView, __in UINT ClearFlags, __in FLOAT Depth, __in UINT8 Stencil) = 0;
		virtual void STDMETHODCALLTYPE GenerateMips(__in ID3D11ShaderResourceView* pShaderResourceView) = 0;
		virtual void STDMETHODCALLTYPE SetResourceMinLOD(__in ID3D11Resource *pResource, FLOAT MinLOD) = 0;
		virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD(__in ID3D11Resource *pResource) = 0;
		virtual void STDMETHODCALLTYPE ResolveSubresource(__in ID3D11Resource *pDstResource, __in UINT DstSubresource, __in ID3D11Resource *pSrcResource, __in UINT SrcSubresource, __in DXGI_FORMAT Format) = 0;
		virtual void STDMETHODCALLTYPE ExecuteCommandList(__in ID3D11CommandList *pCommandList, BOOL RestoreContextState) = 0;
		virtual void STDMETHODCALLTYPE HSSetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D11ShaderResourceView* const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE HSSetShader(__in_opt ID3D11HullShader *pHullShader, __in_ecount_opt(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE HSSetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D11SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE HSSetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE DSSetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D11ShaderResourceView* const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE DSSetShader(__in_opt ID3D11DomainShader *pDomainShader, __in_ecount_opt(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE DSSetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D11SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE DSSetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE CSSetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D11ShaderResourceView* const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews(__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot) UINT NumUAVs, __in_ecount(NumUAVs) ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, __in_ecount(NumUAVs) const UINT *pUAVInitialCounts) = 0;
		virtual void STDMETHODCALLTYPE CSSetShader(__in_opt ID3D11ComputeShader *pComputeShader, __in_ecount_opt(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE CSSetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D11SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE CSSetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D11Buffer* const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE VSGetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE PSGetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D11ShaderResourceView* *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE PSGetShader(__out ID3D11PixelShader **ppPixelShader, __out_ecount_opt(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, __inout_opt UINT *pNumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE PSGetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D11SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE VSGetShader(__out ID3D11VertexShader **ppVertexShader, __out_ecount_opt(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, __inout_opt UINT *pNumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE PSGetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE IAGetInputLayout(__out ID3D11InputLayout **ppInputLayout) = 0;
		virtual void STDMETHODCALLTYPE IAGetVertexBuffers(__in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount_opt(NumBuffers) ID3D11Buffer* *ppVertexBuffers, __out_ecount_opt(NumBuffers) UINT *pStrides, __out_ecount_opt(NumBuffers) UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE IAGetIndexBuffer(__out_opt ID3D11Buffer* *pIndexBuffer, __out_opt DXGI_FORMAT *Format, __out_opt UINT *Offset) = 0;
		virtual void STDMETHODCALLTYPE GSGetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE GSGetShader(__out ID3D11GeometryShader **ppGeometryShader, __out_ecount_opt(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, __inout_opt UINT *pNumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(__out D3D11_PRIMITIVE_TOPOLOGY *pTopology) = 0;
		virtual void STDMETHODCALLTYPE VSGetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D11ShaderResourceView* *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE VSGetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D11SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE GetPredication(__out_opt ID3D11Predicate **ppPredicate, __out_opt BOOL *pPredicateValue) = 0;
		virtual void STDMETHODCALLTYPE GSGetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D11ShaderResourceView* *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE GSGetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D11SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE OMGetRenderTargets(__in_range(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews, __out_ecount_opt(NumViews) ID3D11RenderTargetView **ppRenderTargetViews, __out_opt ID3D11DepthStencilView **ppDepthStencilView) = 0;
		virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(__in_range(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumRTVs, __out_ecount_opt(NumRTVs) ID3D11RenderTargetView **ppRenderTargetViews, __out_opt ID3D11DepthStencilView **ppDepthStencilView, __in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1) UINT UAVStartSlot, __in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot) UINT NumUAVs, __out_ecount_opt(NumUAVs) ID3D11UnorderedAccessView **ppUnorderedAccessViews) = 0;
		virtual void STDMETHODCALLTYPE OMGetBlendState(__out_opt ID3D11BlendState **ppBlendState, __out_opt FLOAT BlendFactor[4], __out_opt UINT *pSampleMask) = 0;
		virtual void STDMETHODCALLTYPE OMGetDepthStencilState(__out_opt ID3D11DepthStencilState **ppDepthStencilState, __out_opt UINT *pStencilRef) = 0;
		virtual void STDMETHODCALLTYPE SOGetTargets(__in_range(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppSOTargets) = 0;
		virtual void STDMETHODCALLTYPE RSGetState(__out ID3D11RasterizerState **ppRasterizerState) = 0;
		virtual void STDMETHODCALLTYPE RSGetViewports(__inout UINT *pNumViewports, __out_ecount_opt(*pNumViewports) D3D11_VIEWPORT *pViewports) = 0;
		virtual void STDMETHODCALLTYPE RSGetScissorRects(__inout UINT *pNumRects, __out_ecount_opt(*pNumRects) D3D11_RECT *pRects) = 0;
		virtual void STDMETHODCALLTYPE HSGetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D11ShaderResourceView* *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE HSGetShader(__out ID3D11HullShader **ppHullShader, __out_ecount_opt(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, __inout_opt UINT *pNumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE HSGetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D11SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE HSGetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE DSGetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D11ShaderResourceView* *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE DSGetShader(__out ID3D11DomainShader **ppDomainShader, __out_ecount_opt(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, __inout_opt UINT *pNumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE DSGetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D11SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE DSGetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE CSGetShaderResources(__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D11ShaderResourceView* *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews(__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot) UINT NumUAVs, __out_ecount(NumUAVs) ID3D11UnorderedAccessView **ppUnorderedAccessViews) = 0;
		virtual void STDMETHODCALLTYPE CSGetShader(__out ID3D11ComputeShader **ppComputeShader, __out_ecount_opt(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, __inout_opt UINT *pNumClassInstances) = 0;
		virtual void STDMETHODCALLTYPE CSGetSamplers(__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D11SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE CSGetConstantBuffers(__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D11Buffer* *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE ClearState(void) = 0;
		virtual void STDMETHODCALLTYPE Flush(void) = 0;
		virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType(void) = 0;
		virtual UINT STDMETHODCALLTYPE GetContextFlags(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE FinishCommandList(BOOL RestoreDeferredContextState, __out_opt ID3D11CommandList **ppCommandList) = 0;
};

// "Windows 10 SDK" -> "d3d11_1.h"
MIDL_INTERFACE("bb2c6faa-b5fb-4082-8e6b-388b8cfa90e1")
ID3D11DeviceContext1 : public ID3D11DeviceContext
{
	public:
		virtual void STDMETHODCALLTYPE CopySubresourceRegion1(_In_ ID3D11Resource *pDstResource, _In_ UINT DstSubresource, _In_ UINT DstX, _In_ UINT DstY, _In_ UINT DstZ, _In_ ID3D11Resource *pSrcResource, _In_ UINT SrcSubresource, _In_opt_ const D3D11_BOX *pSrcBox, _In_ UINT CopyFlags) = 0;
		virtual void STDMETHODCALLTYPE UpdateSubresource1(_In_ ID3D11Resource *pDstResource, _In_ UINT DstSubresource, _In_opt_ const D3D11_BOX *pDstBox, _In_ const void *pSrcData, _In_ UINT SrcRowPitch, _In_ UINT SrcDepthPitch, _In_ UINT CopyFlags) = 0;
		virtual void STDMETHODCALLTYPE DiscardResource(_In_ ID3D11Resource *pResource) = 0;
		virtual void STDMETHODCALLTYPE DiscardView(_In_ ID3D11View *pResourceView) = 0;
		virtual void STDMETHODCALLTYPE VSSetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers, _In_reads_opt_(NumBuffers) const UINT *pFirstConstant, _In_reads_opt_(NumBuffers) const UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE HSSetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers, _In_reads_opt_(NumBuffers) const UINT *pFirstConstant, _In_reads_opt_(NumBuffers) const UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE DSSetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers, _In_reads_opt_(NumBuffers) const UINT *pFirstConstant, _In_reads_opt_(NumBuffers) const UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE GSSetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers, _In_reads_opt_(NumBuffers) const UINT *pFirstConstant, _In_reads_opt_(NumBuffers) const UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE PSSetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers, _In_reads_opt_(NumBuffers) const UINT *pFirstConstant, _In_reads_opt_(NumBuffers) const UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE CSSetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers, _In_reads_opt_(NumBuffers) const UINT *pFirstConstant, _In_reads_opt_(NumBuffers) const UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE VSGetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers, _Out_writes_opt_(NumBuffers) UINT *pFirstConstant, _Out_writes_opt_(NumBuffers) UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE HSGetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers, _Out_writes_opt_(NumBuffers) UINT *pFirstConstant, _Out_writes_opt_(NumBuffers) UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE DSGetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers, _Out_writes_opt_(NumBuffers) UINT *pFirstConstant, _Out_writes_opt_(NumBuffers) UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE GSGetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers, _Out_writes_opt_(NumBuffers) UINT *pFirstConstant, _Out_writes_opt_(NumBuffers) UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE PSGetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers, _Out_writes_opt_(NumBuffers) UINT *pFirstConstant, _Out_writes_opt_(NumBuffers) UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE CSGetConstantBuffers1(_In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 ) UINT StartSlot, _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers, _Out_writes_opt_(NumBuffers) UINT *pFirstConstant, _Out_writes_opt_(NumBuffers) UINT *pNumConstants) = 0;
		virtual void STDMETHODCALLTYPE SwapDeviceContextState(_In_ ID3DDeviceContextState *pState, _Outptr_opt_ ID3DDeviceContextState **ppPreviousState) = 0;
		virtual void STDMETHODCALLTYPE ClearView(_In_ ID3D11View *pView, _In_ const FLOAT Color[ 4 ], _In_reads_opt_(NumRects) const D3D11_RECT *pRect, UINT NumRects) = 0;
		virtual void STDMETHODCALLTYPE DiscardView1(_In_ ID3D11View *pResourceView, _In_reads_opt_(NumRects) const D3D11_RECT *pRects, UINT NumRects) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11InputLayout : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11SamplerState : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_SAMPLER_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11RasterizerState : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_RASTERIZER_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11BlendState : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_BLEND_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11DepthStencilState : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_DEPTH_STENCIL_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11VertexShader : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11HullShader : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11DomainShader : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11GeometryShader : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11PixelShader : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11ComputeShader : public ID3D11DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11Resource : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetType(__out D3D11_RESOURCE_DIMENSION *pResourceDimension) = 0;
		virtual void STDMETHODCALLTYPE SetEvictionPriority(__in UINT EvictionPriority) = 0;
		virtual UINT STDMETHODCALLTYPE GetEvictionPriority(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11Buffer : public ID3D11Resource
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_BUFFER_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
MIDL_INTERFACE("f8fb5c27-c6b3-4f75-a4c8-439af2ef564c")
ID3D11Texture1D : public ID3D11Resource
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(_Out_ D3D11_TEXTURE1D_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
MIDL_INTERFACE("6f15aaf2-d208-4e89-9ab4-489535d34f9c")
ID3D11Texture2D : public ID3D11Resource
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_TEXTURE2D_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
MIDL_INTERFACE("037e866e-f56d-4357-a8af-9dabbe6e250e")
ID3D11Texture3D : public ID3D11Resource
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(_Out_ D3D11_TEXTURE3D_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11View : public ID3D11DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetResource(__out ID3D11Resource **ppResource) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11ShaderResourceView : public ID3D11View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11UnorderedAccessView : public ID3D11View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11RenderTargetView : public ID3D11View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_RENDER_TARGET_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11DepthStencilView : public ID3D11View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
struct ID3D10Blob : public IUnknown
{
	public:
		virtual LPVOID STDMETHODCALLTYPE GetBufferPointer(void) = 0;
		virtual SIZE_T STDMETHODCALLTYPE GetBufferSize(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11Asynchronous : public ID3D11DeviceChild
{
	public:
		virtual UINT STDMETHODCALLTYPE GetDataSize(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
enum D3D11_QUERY
{
	D3D11_QUERY_EVENT							= 0,
	D3D11_QUERY_OCCLUSION						= (D3D11_QUERY_EVENT + 1),
	D3D11_QUERY_TIMESTAMP						= (D3D11_QUERY_OCCLUSION + 1),
	D3D11_QUERY_TIMESTAMP_DISJOINT				= (D3D11_QUERY_TIMESTAMP + 1),
	D3D11_QUERY_PIPELINE_STATISTICS				= (D3D11_QUERY_TIMESTAMP_DISJOINT + 1),
	D3D11_QUERY_OCCLUSION_PREDICATE				= (D3D11_QUERY_PIPELINE_STATISTICS + 1),
	D3D11_QUERY_SO_STATISTICS					= (D3D11_QUERY_OCCLUSION_PREDICATE + 1),
	D3D11_QUERY_SO_OVERFLOW_PREDICATE			= (D3D11_QUERY_SO_STATISTICS + 1),
	D3D11_QUERY_SO_STATISTICS_STREAM0			= (D3D11_QUERY_SO_OVERFLOW_PREDICATE + 1),
	D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0	= (D3D11_QUERY_SO_STATISTICS_STREAM0 + 1),
	D3D11_QUERY_SO_STATISTICS_STREAM1			= (D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0 + 1),
	D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1	= (D3D11_QUERY_SO_STATISTICS_STREAM1 + 1),
	D3D11_QUERY_SO_STATISTICS_STREAM2			= (D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1 + 1),
	D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2	= (D3D11_QUERY_SO_STATISTICS_STREAM2 + 1),
	D3D11_QUERY_SO_STATISTICS_STREAM3			= (D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2 + 1),
	D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3	= (D3D11_QUERY_SO_STATISTICS_STREAM3 + 1) 
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct D3D11_QUERY_DESC
{
	D3D11_QUERY Query;
	UINT MiscFlags;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
struct ID3D11Query : public ID3D11Asynchronous
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D11_QUERY_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11.h"
typedef struct D3D11_QUERY_DATA_PIPELINE_STATISTICS
{
	UINT64 IAVertices;
	UINT64 IAPrimitives;
	UINT64 VSInvocations;
	UINT64 GSInvocations;
	UINT64 GSPrimitives;
	UINT64 CInvocations;
	UINT64 CPrimitives;
	UINT64 PSInvocations;
	UINT64 HSInvocations;
	UINT64 DSInvocations;
	UINT64 CSInvocations;
} D3D11_QUERY_DATA_PIPELINE_STATISTICS;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
typedef enum D3D11_RLDO_FLAGS
{
	D3D11_RLDO_SUMMARY	= 0x1,
	D3D11_RLDO_DETAIL	= 0x2
} D3D11_RLDO_FLAGS;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
MIDL_INTERFACE("79cf2233-7536-4948-9d36-1e4692dc5760")
ID3D11Debug : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetFeatureMask(UINT Mask) = 0;
		virtual UINT STDMETHODCALLTYPE GetFeatureMask(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPresentPerRenderOpDelay(UINT Milliseconds) = 0;
		virtual UINT STDMETHODCALLTYPE GetPresentPerRenderOpDelay(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetSwapChain(__in_opt IDXGISwapChain *pSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetSwapChain(__out IDXGISwapChain **ppSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE ValidateContext(__in ID3D11DeviceContext *pContext) = 0;
		virtual HRESULT STDMETHODCALLTYPE ReportLiveDeviceObjects(D3D11_RLDO_FLAGS Flags) = 0;
		virtual HRESULT STDMETHODCALLTYPE ValidateContextForDispatch(__in ID3D11DeviceContext *pContext) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
typedef enum D3D11_MESSAGE_CATEGORY
{
	D3D11_MESSAGE_CATEGORY_APPLICATION_DEFINED		= 0,
	D3D11_MESSAGE_CATEGORY_MISCELLANEOUS			= D3D11_MESSAGE_CATEGORY_APPLICATION_DEFINED + 1,
	D3D11_MESSAGE_CATEGORY_INITIALIZATION			= D3D11_MESSAGE_CATEGORY_MISCELLANEOUS + 1,
	D3D11_MESSAGE_CATEGORY_CLEANUP					= D3D11_MESSAGE_CATEGORY_INITIALIZATION + 1,
	D3D11_MESSAGE_CATEGORY_COMPILATION				= D3D11_MESSAGE_CATEGORY_CLEANUP + 1,
	D3D11_MESSAGE_CATEGORY_STATE_CREATION			= D3D11_MESSAGE_CATEGORY_COMPILATION + 1,
	D3D11_MESSAGE_CATEGORY_STATE_SETTING			= D3D11_MESSAGE_CATEGORY_STATE_CREATION + 1,
	D3D11_MESSAGE_CATEGORY_STATE_GETTING			= D3D11_MESSAGE_CATEGORY_STATE_SETTING + 1,
	D3D11_MESSAGE_CATEGORY_RESOURCE_MANIPULATION	= D3D11_MESSAGE_CATEGORY_STATE_GETTING + 1,
	D3D11_MESSAGE_CATEGORY_EXECUTION				= D3D11_MESSAGE_CATEGORY_RESOURCE_MANIPULATION + 1,
	D3D11_MESSAGE_CATEGORY_SHADER					= D3D11_MESSAGE_CATEGORY_EXECUTION + 1
} D3D11_MESSAGE_CATEGORY;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
typedef enum D3D11_MESSAGE_SEVERITY
{
	D3D11_MESSAGE_SEVERITY_CORRUPTION	= 0,
	D3D11_MESSAGE_SEVERITY_ERROR		= D3D11_MESSAGE_SEVERITY_CORRUPTION + 1,
	D3D11_MESSAGE_SEVERITY_WARNING		= D3D11_MESSAGE_SEVERITY_ERROR + 1,
	D3D11_MESSAGE_SEVERITY_INFO			= D3D11_MESSAGE_SEVERITY_WARNING + 1
} D3D11_MESSAGE_SEVERITY;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h" -> But only the few definitions we need
typedef enum D3D11_MESSAGE_ID
{
	D3D11_MESSAGE_ID_UNKNOWN								 = 0,
	D3D11_MESSAGE_ID_DEVICE_IASETVERTEXBUFFERS_HAZARD		 = 1,
	D3D11_MESSAGE_ID_DEVICE_IASETINDEXBUFFER_HAZARD			 = 2,
	D3D11_MESSAGE_ID_DEVICE_OMSETRENDERTARGETS_HAZARD		 = 9,
	D3D11_MESSAGE_ID_DEVICE_VSSETSHADERRESOURCES_HAZARD		 = 3,
	D3D11_MESSAGE_ID_DEVICE_GSSETSHADERRESOURCES_HAZARD		 = 5,
	D3D11_MESSAGE_ID_DEVICE_PSSETSHADERRESOURCES_HAZARD		 = 7,
	D3D11_MESSAGE_ID_DEVICE_HSSETSHADERRESOURCES_HAZARD		 = 2097173,
	D3D11_MESSAGE_ID_DEVICE_DSSETSHADERRESOURCES_HAZARD		 = 2097189,
	D3D11_MESSAGE_ID_DEVICE_CSSETSHADERRESOURCES_HAZARD		 = 2097316,
	D3D11_MESSAGE_ID_DEVICE_CSSETUNORDEREDACCESSVIEWS_HAZARD = 2097354
} D3D11_MESSAGE_ID;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
typedef struct D3D11_MESSAGE
{
	D3D11_MESSAGE_CATEGORY Category;
	D3D11_MESSAGE_SEVERITY Severity;
	D3D11_MESSAGE_ID ID;
	const char* pDescription;
	SIZE_T DescriptionByteLength;
} D3D11_MESSAGE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
typedef struct D3D11_INFO_QUEUE_FILTER_DESC
{
	UINT NumCategories;
	D3D11_MESSAGE_CATEGORY *pCategoryList;
	UINT NumSeverities;
	D3D11_MESSAGE_SEVERITY *pSeverityList;
	UINT NumIDs;
	D3D11_MESSAGE_ID *pIDList;
} D3D11_INFO_QUEUE_FILTER_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
typedef struct D3D11_INFO_QUEUE_FILTER
{
	D3D11_INFO_QUEUE_FILTER_DESC AllowList;
	D3D11_INFO_QUEUE_FILTER_DESC DenyList;
} D3D11_INFO_QUEUE_FILTER;

// "Microsoft DirectX SDK (June 2010)" -> "D3D11SDKLayers.h"
MIDL_INTERFACE("6543dbb6-1b48-42f5-ab82-e97ec74326f6")
ID3D11InfoQueue : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetMessageCountLimit(_In_ UINT64 MessageCountLimit) = 0;
		virtual void STDMETHODCALLTYPE ClearStoredMessages(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetMessage(_In_ UINT64 MessageIndex, _Out_writes_bytes_opt_(*pMessageByteLength) D3D11_MESSAGE *pMessage, _Inout_ SIZE_T *pMessageByteLength) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesAllowedByStorageFilter(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesDeniedByStorageFilter(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumStoredMessages(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumStoredMessagesAllowedByRetrievalFilter(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesDiscardedByMessageCountLimit(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetMessageCountLimit(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddStorageFilterEntries(_In_ D3D11_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetStorageFilter(_Out_writes_bytes_opt_(*pFilterByteLength) D3D11_INFO_QUEUE_FILTER *pFilter, _Inout_ SIZE_T *pFilterByteLength) = 0;
		virtual void STDMETHODCALLTYPE ClearStorageFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushEmptyStorageFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushCopyOfStorageFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushStorageFilter(_In_ D3D11_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual void STDMETHODCALLTYPE PopStorageFilter(void) = 0;
		virtual UINT STDMETHODCALLTYPE GetStorageFilterStackSize(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddRetrievalFilterEntries(_In_ D3D11_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetRetrievalFilter(_Out_writes_bytes_opt_(*pFilterByteLength) D3D11_INFO_QUEUE_FILTER *pFilter, _Inout_ SIZE_T *pFilterByteLength) = 0;
		virtual void STDMETHODCALLTYPE ClearRetrievalFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushEmptyRetrievalFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushCopyOfRetrievalFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushRetrievalFilter(_In_ D3D11_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual void STDMETHODCALLTYPE PopRetrievalFilter(void) = 0;
		virtual UINT STDMETHODCALLTYPE GetRetrievalFilterStackSize(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddMessage(_In_ D3D11_MESSAGE_CATEGORY Category, _In_ D3D11_MESSAGE_SEVERITY Severity, _In_ D3D11_MESSAGE_ID ID, _In_ LPCSTR pDescription) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddApplicationMessage(_In_ D3D11_MESSAGE_SEVERITY Severity, _In_ LPCSTR pDescription) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnCategory(_In_ D3D11_MESSAGE_CATEGORY Category, _In_ BOOL bEnable) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnSeverity(_In_ D3D11_MESSAGE_SEVERITY Severity, _In_ BOOL bEnable) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnID(_In_ D3D11_MESSAGE_ID ID, _In_ BOOL bEnable) = 0;
		virtual BOOL STDMETHODCALLTYPE GetBreakOnCategory(_In_ D3D11_MESSAGE_CATEGORY Category) = 0;
		virtual BOOL STDMETHODCALLTYPE GetBreakOnSeverity(_In_ D3D11_MESSAGE_SEVERITY Severity) = 0;
		virtual BOOL STDMETHODCALLTYPE GetBreakOnID(_In_ D3D11_MESSAGE_ID ID) = 0;
		virtual void STDMETHODCALLTYPE SetMuteDebugOutput(_In_ BOOL bMute) = 0;
		virtual BOOL STDMETHODCALLTYPE GetMuteDebugOutput(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "d3d11_1.h"
MIDL_INTERFACE("b2daad8b-03d4-4dbf-95eb-32ab4b63d0ab")
ID3DUserDefinedAnnotation : public IUnknown
{
	public:
		virtual INT STDMETHODCALLTYPE BeginEvent(_In_ LPCWSTR Name) = 0;
		virtual INT STDMETHODCALLTYPE EndEvent(void) = 0;
		virtual void STDMETHODCALLTYPE SetMarker(_In_ LPCWSTR Name) = 0;
		virtual BOOL STDMETHODCALLTYPE GetStatus(void) = 0;
};




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
struct AGSContext;	// AMD AGS
namespace Direct3D11Rhi
{
	class RootSignature;
	class Direct3D11RuntimeLinking;
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
		RHI_ASSERT(mContext, &rhiReference == &(resourceReference).getRhi(), "Direct3D 11 error: The given resource is owned by another RHI instance")

	/*
	*  @brief
	*    Debug break on execution failure, replacement for "ID3D11InfoQueue::SetBreakOnSeverity()" which is creating a confusing callstack
	*/
	#define FAILED_DEBUG_BREAK(toExecute) if (FAILED(toExecute)) { DEBUG_BREAK; }
#else
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given RHI
	*/
	#define RHI_MATCH_CHECK(rhiReference, resourceReference)

	/*
	*  @brief
	*    Debug break on execution failure, replacement for "ID3D11InfoQueue::SetBreakOnSeverity()" which is creating a confusing callstack
	*/
	#define FAILED_DEBUG_BREAK(toExecute) toExecute
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
		typedef LONG NTSTATUS, *PNTSTATUS;
		typedef NTSTATUS (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
		static constexpr const char* HLSL_NAME = "HLSL";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)


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

		// From https://stackoverflow.com/a/36545162
		[[nodiscard]] RTL_OSVERSIONINFOW getRealOSVersion()
		{
			const HMODULE hModule = ::GetModuleHandleW(L"ntdll.dll");
			if (hModule)
			{
				PRAGMA_WARNING_PUSH
					PRAGMA_WARNING_DISABLE_MSVC(4191)	// warning C4191: 'reinterpret_cast': unsafe conversion from 'FARPROC' to '`anonymous-namespace'::detail::RtlGetVersionPtr'
					const RtlGetVersionPtr functionPointer = reinterpret_cast<RtlGetVersionPtr>(::GetProcAddress(hModule, "RtlGetVersion"));
				PRAGMA_WARNING_POP
				if (nullptr != functionPointer)
				{
					RTL_OSVERSIONINFOW rovi = { };
					rovi.dwOSVersionInfoSize = sizeof(rovi);
					if (0x00000000 == functionPointer(&rovi))
					{
						return rovi;
					}
				}
			}
			RTL_OSVERSIONINFOW rovi = { };
			return rovi;
		}

		// "IsWindows10OrGreater()" isn't practically usable
		// - See "Windows Dev Center" -> "Version Helper functions" -> "IsWindows10OrGreater" at https://msdn.microsoft.com/en-us/library/windows/desktop/dn424972(v=vs.85).aspx
		//   "For Windows 10, IsWindows10OrGreater returns false unless the application contains a manifest that includes a compatibility section that contains the GUID that designates Windows 10."
		[[nodiscard]] inline bool isWindows10OrGreater()
		{
			return (getRealOSVersion().dwMajorVersion >= 10);
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Direct3D11Rhi
{




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Direct3D11Rhi.h                         ]
	//[-------------------------------------------------------]
	struct CurrentGraphicsPipelineState final
	{
		Rhi::IGraphicsProgram*	 graphicsProgram		= nullptr;
		ID3D11InputLayout*		 d3d11InputLayout		= nullptr;
		ID3D11RasterizerState*	 d3d11RasterizerState	= nullptr;
		ID3D11DepthStencilState* d3d11DepthStencilState	= nullptr;
		ID3D11BlendState*		 d3d11BlendState		= nullptr;
	};

	/**
	*  @brief
	*    Direct3D 11 RHI class
	*/
	class Direct3D11Rhi final : public Rhi::IRhi
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
		explicit Direct3D11Rhi(const Rhi::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Direct3D11Rhi() override;

		/**
		*  @brief
		*    Return the Direct3D 11 device
		*
		*  @return
		*    The Direct3D 11 device, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Device* getD3D11Device() const
		{
			return mD3D11Device;
		}

		/**
		*  @brief
		*    Return the immediate Direct3D 11 device context instance
		*
		*  @return
		*    The immediate Direct3D 11 device context instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11DeviceContext* getD3D11DeviceContext() const
		{
			return mD3D11DeviceContext;
		}

		/**
		*  @brief
		*    Return the Direct3D feature level
		*
		*  @return
		*    The Direct3D feature level
		*/
		[[nodiscard]] inline D3D_FEATURE_LEVEL getD3DFeatureLevel() const
		{
			return mD3DFeatureLevel;
		}

		/**
		*  @brief
		*    Get the render target to render into
		*
		*  @return
		*    Render target currently bound to the output-merger state, a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::IRenderTarget* omGetRenderTarget() const
		{
			return mRenderTarget;
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
		void clearGraphics(uint32_t flags, const float color[4], float z, uint32_t stencil);
		void drawGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawGraphicsAgs(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawGraphicsNvApi(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsAgs(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsNvApi(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
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
		void generateAsynchronousDeferredMipmaps(Rhi::ITexture& texture, ID3D11ShaderResourceView& d3d11ShaderResourceView);
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
		[[nodiscard]] inline virtual const char* getName() const override
		{
			return "Direct3D11";
		}

		[[nodiscard]] inline virtual bool isInitialized() const override
		{
			// Is there a Direct3D 11 device?
			return (nullptr != mD3D11Device);
		}

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
		//[ Implementation specific                               ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline virtual void* getD3D11DevicePointer() const override
		{
			return mD3D11Device;
		}

		[[nodiscard]] inline virtual void* getD3D11ImmediateContextPointer() const override
		{
			return mD3D11DeviceContext;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(mContext, Direct3D11Rhi, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D11Rhi(const Direct3D11Rhi& source) = delete;
		Direct3D11Rhi& operator =(const Direct3D11Rhi& source) = delete;

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


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D11RuntimeLinking*  mDirect3D11RuntimeLinking;				///< Direct3D 11 runtime linking instance, always valid
		ID3D11Device*			   mD3D11Device;							///< The Direct3D 11 device, null pointer on error (we don't check because this would be a total overhead, the user has to use "Rhi::IRhi::isInitialized()" and is asked to never ever use a not properly initialized RHI)
		ID3D11DeviceContext*	   mD3D11DeviceContext;						///< The Direct3D 11 device context instance, null pointer on error (we don't check because this would be a total overhead, the user has to use "Rhi::IRhi::isInitialized()" and is asked to never ever use a not properly initialized RHI)
		D3D_FEATURE_LEVEL		   mD3DFeatureLevel;
		ID3DUserDefinedAnnotation* mD3DUserDefinedAnnotation;				///< The Direct3D 11 user defined annotation interface, null pointer on error, Direct3D 11.1 feature
		Rhi::IShaderLanguage*	   mShaderLanguageHlsl;						///< HLSL shader language instance (we keep a reference to it), can be a null pointer
		ID3D11Query*			   mD3D11QueryFlush;						///< Direct3D 11 query used for flush, can be a null pointer
		Rhi::IRenderTarget*		   mRenderTarget;							///< Currently set render target (we keep a reference to it), can be a null pointer
		RootSignature*			   mGraphicsRootSignature;					///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*			   mComputeRootSignature;					///< Currently set compute root signature (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant Direct3D 11 calls
		CurrentGraphicsPipelineState mCurrentGraphicsPipelineState;
		D3D11_PRIMITIVE_TOPOLOGY	 mD3D11PrimitiveTopology;
		ID3D11VertexShader*			 mD3d11VertexShader;
		ID3D11HullShader*			 mD3d11HullShader;
		ID3D11DomainShader*			 mD3d11DomainShader;
		ID3D11GeometryShader*		 mD3d11GeometryShader;
		ID3D11PixelShader*			 mD3d11PixelShader;
		ID3D11ComputeShader*		 mD3d11ComputeShader;
		// Generate asynchronous mipmaps for textures
		std::mutex					mGenerateAsynchronousMipmapsForTexturesMutex;
		std::vector<Rhi::ITexture*> mGenerateAsynchronousMipmapsForTextures;
		#ifdef RHI_DEBUG
			bool mDebugBetweenBeginEndScene;	///< Just here for state tracking in debug builds
		#endif


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Direct3D11RuntimeLinking.h              ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Macros & definitions                                  ]
	//[-------------------------------------------------------]
	// Redirect D3D11* function calls to funcPtr_D3D11*
	#ifndef FNPTR
		#define FNPTR(name) funcPtr_##name
	#endif

	//[-------------------------------------------------------]
	//[ DXGI core functions                                   ]
	//[-------------------------------------------------------]
	#define FNDEF_DXGI(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_DXGI(HRESULT,	CreateDXGIFactory,	(REFIID riid, _COM_Outptr_ void **ppFactory));
	#define CreateDXGIFactory	FNPTR(CreateDXGIFactory)

	//[-------------------------------------------------------]
	//[ D3D11 core functions                                  ]
	//[-------------------------------------------------------]
	#define FNDEF_D3D11(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_D3D11(HRESULT,	D3D11CreateDevice,	(__in_opt IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, __in_ecount_opt( FeatureLevels ) CONST D3D_FEATURE_LEVEL*, UINT, UINT, __out_opt ID3D11Device**, __out_opt D3D_FEATURE_LEVEL*, __out_opt ID3D11DeviceContext**));
	#define D3D11CreateDevice	FNPTR(D3D11CreateDevice)

	//[-------------------------------------------------------]
	//[ D3DCompiler functions                                 ]
	//[-------------------------------------------------------]
	#define FNDEF_D3DCOMPILER(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	typedef __interface ID3D10Blob *LPD3D10BLOB;	// "__interface" is no keyword of the ISO C++ standard, shouldn't be a problem because this in here is Microsoft Windows only and it's also within the Direct3D headers we have to use
	typedef ID3D10Blob ID3DBlob;
	FNDEF_D3DCOMPILER(HRESULT,	D3DCompile,		(LPCVOID, SIZE_T, LPCSTR, CONST D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**));
	FNDEF_D3DCOMPILER(HRESULT,	D3DCreateBlob,	(SIZE_T Size, ID3DBlob** ppBlob));
	#define D3DCompile		FNPTR(D3DCompile)
	#define D3DCreateBlob	FNPTR(D3DCreateBlob)

	//[-------------------------------------------------------]
	//[ AMD AGS functions                                     ]
	//[-------------------------------------------------------]
	}
	extern "C" {
	// -> Using AMD AGS v5.3.0 - November 22, 2018
	// -> From https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK and https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/Barycentrics12/master/ags_lib/inc/amd_ags.h
	// -> We're using the static linked version of AMD AGS in order to reduce the number of individual shipped files
	// -> For debug builds there are no static AMD AGS libraries available
	#ifdef RHI_DEBUG
		#define DYNAMIC_AMD_AGS
	#endif
	struct AGSGPUInfo;
	typedef void* (__stdcall *AGS_ALLOC_CALLBACK)(size_t allocationSize);
	typedef void (__stdcall *AGS_FREE_CALLBACK)(void* allocationPtr);
	struct AGSConfiguration
	{
		AGS_ALLOC_CALLBACK allocCallback;
		AGS_FREE_CALLBACK  freeCallback;
	};
	struct AGSDX11DeviceCreationParams
	{
		IDXGIAdapter*               pAdapter;
		D3D_DRIVER_TYPE             DriverType;
		HMODULE                     Software;
		UINT                        Flags;
		const D3D_FEATURE_LEVEL*    pFeatureLevels;
		UINT                        FeatureLevels;
		UINT                        SDKVersion;
		const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc;
	};
	enum AGSCrossfireMode
	{
		AGS_CROSSFIRE_MODE_DRIVER_AFR = 0,
		AGS_CROSSFIRE_MODE_EXPLICIT_AFR,
		AGS_CROSSFIRE_MODE_DISABLE
	};
	struct AGSDX11ExtensionParams
	{
		const WCHAR*     pAppName;
		const WCHAR*     pEngineName;
		unsigned int     appVersion;
		unsigned int     engineVersion;
		unsigned int     numBreadcrumbMarkers;
		unsigned int     uavSlot;
		AGSCrossfireMode crossfireMode;
	};
	struct AGSDX11ReturnedParams
	{
		ID3D11Device*        pDevice;
		ID3D11DeviceContext* pImmediateContext;
		IDXGISwapChain*      pSwapChain;
		D3D_FEATURE_LEVEL    FeatureLevel;
		unsigned int         extensionsSupported;
		unsigned int         crossfireGPUCount;
		void*                breadcrumbBuffer;
	};
	enum AGSReturnCode
	{
		AGS_SUCCESS,
		AGS_FAILURE,
		AGS_INVALID_ARGS,
		AGS_OUT_OF_MEMORY,
		AGS_ERROR_MISSING_DLL,
		AGS_ERROR_LEGACY_DRIVER,
		AGS_EXTENSION_NOT_SUPPORTED,
		AGS_ADL_FAILURE,
		AGS_DX_FAILURE
	};
	#ifdef DYNAMIC_AMD_AGS
		#define FNDEF_AMD_AGS(retType, funcName, args) retType (WINAPIV *funcPtr_##funcName) args
	#else
		#define FNDEF_AMD_AGS(retType, funcName, args) retType WINAPIV funcName args
	#endif
	FNDEF_AMD_AGS(AGSReturnCode,	agsInit,													(AGSContext** context, const AGSConfiguration* config, AGSGPUInfo* gpuInfo));
	FNDEF_AMD_AGS(AGSReturnCode,	agsDeInit,													(AGSContext* context));
	FNDEF_AMD_AGS(AGSReturnCode,	agsDriverExtensionsDX11_CreateDevice,						(AGSContext* context, AGSDX11DeviceCreationParams* creationParams, AGSDX11ExtensionParams* extensionParams, AGSDX11ReturnedParams* returnedParams));
	FNDEF_AMD_AGS(AGSReturnCode,	agsDriverExtensionsDX11_DestroyDevice,						(AGSContext* context, ID3D11Device* device, unsigned int* deviceReferences, ID3D11DeviceContext* immediateContext, unsigned int* immediateContextReferences));
	FNDEF_AMD_AGS(AGSReturnCode,	agsDriverExtensionsDX11_MultiDrawInstancedIndirect,			(AGSContext* context, ID3D11DeviceContext* dxContext, unsigned int drawCount, ID3D11Buffer* pBufferForArgs, unsigned int alignedByteOffsetForArgs, unsigned int byteStrideForArgs));
	FNDEF_AMD_AGS(AGSReturnCode,	agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect,	(AGSContext* context, ID3D11DeviceContext* dxContext, unsigned int drawCount, ID3D11Buffer* pBufferForArgs, unsigned int alignedByteOffsetForArgs, unsigned int byteStrideForArgs));
	#ifdef DYNAMIC_AMD_AGS
		#define agsInit														FNPTR(agsInit)
		#define agsDeInit													FNPTR(agsDeInit)
		#define agsDriverExtensionsDX11_CreateDevice						FNPTR(agsDriverExtensionsDX11_CreateDevice)
		#define agsDriverExtensionsDX11_DestroyDevice						FNPTR(agsDriverExtensionsDX11_DestroyDevice)
		#define agsDriverExtensionsDX11_MultiDrawInstancedIndirect			FNPTR(agsDriverExtensionsDX11_MultiDrawInstancedIndirect)
		#define agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect	FNPTR(agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect)
	#else
		#ifdef ARCHITECTURE_X64
			#ifdef RHI_DIRECT3D11_EXPORTS
				// Dynamic library is using "Multi-threaded DLL (/MD)"
				#pragma comment(lib, "amd_ags_x64_2017_MD.lib")
			#else
				// Static library is using "Multi-threaded (/MT)"
				#pragma comment(lib, "amd_ags_x64_2017_MT.lib")
			#endif
		#else
			#ifdef RHI_DIRECT3D11_EXPORTS
				// Dynamic library is using "Multi-threaded DLL (/MD)"
				#pragma comment(lib, "amd_ags_x86_2017_MD.lib")
			#else
				// Static library is using "Multi-threaded (/MT)"
				#pragma comment(lib, "amd_ags_x86_2017_MT.lib")
			#endif
		#endif
	#endif
	Rhi::IAllocator* g_AmdAgsAllocator = nullptr;	///< Evil global variable since AMD AGS doesn't allow to pass in user data to the allocator functions
	[[nodiscard]] void* __stdcall AmdAgsAllocCallback(size_t allocationSize)
	{
		return g_AmdAgsAllocator->reallocate(nullptr, 0, allocationSize, 1);
	}
	void __stdcall AmdAgsFreeCallback(void* allocationPtr)
	{
		g_AmdAgsAllocator->reallocate(allocationPtr, 0, 0, 1);
	}
	}
	namespace Direct3D11Rhi
	{


	//[-------------------------------------------------------]
	//[ NvAPI functions                                       ]
	//[-------------------------------------------------------]
	// From https://developer.nvidia.com/nvapi and http://developer.download.nvidia.com/NVAPI/PG-5116-001_v01_public.pdf
	#define FNDEF_NvAPI(retType, funcName, args) retType (WINAPIV *funcPtr_##funcName) args
	typedef int NvAPI_Status;
	typedef unsigned int NvU32;
	FNDEF_NvAPI(NvAPI_Status,	NvAPI_Initialize,								());
	FNDEF_NvAPI(NvAPI_Status,	NvAPI_Unload,									());
	FNDEF_NvAPI(NvAPI_Status,	NvAPI_D3D11_MultiDrawInstancedIndirect,			(__in ID3D11DeviceContext* pDevContext11, __in NvU32 drawCount, __in ID3D11Buffer* pBuffer, __in NvU32 alignedByteOffsetForArgs, __in NvU32 alignedByteStrideForArgs));
	FNDEF_NvAPI(NvAPI_Status,	NvAPI_D3D11_MultiDrawIndexedInstancedIndirect,	(__in ID3D11DeviceContext* pDevContext11, __in NvU32 drawCount, __in ID3D11Buffer* pBuffer, __in NvU32 alignedByteOffsetForArgs, __in NvU32 alignedByteStrideForArgs));
	#define NvAPI_Initialize								FNPTR(NvAPI_Initialize)
	#define NvAPI_Unload									FNPTR(NvAPI_Unload)
	#define NvAPI_D3D11_MultiDrawInstancedIndirect			FNPTR(NvAPI_D3D11_MultiDrawInstancedIndirect)
	#define NvAPI_D3D11_MultiDrawIndexedInstancedIndirect	FNPTR(NvAPI_D3D11_MultiDrawIndexedInstancedIndirect)

	/**
	*  @brief
	*    Direct3D 11 runtime linking
	*/
	class Direct3D11RuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*/
		inline explicit Direct3D11RuntimeLinking(Direct3D11Rhi& direct3D11Rhi) :
			mDirect3D11Rhi(direct3D11Rhi),
			mDxgiSharedLibrary(nullptr),
			mD3D11SharedLibrary(nullptr),
			mD3DCompilerSharedLibrary(nullptr),
			#ifdef DYNAMIC_AMD_AGS
				mAmdAgsSharedLibrary(nullptr),
			#endif
			mAgsContext(nullptr),
			mNvAPISharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		~Direct3D11RuntimeLinking()
		{
			// Destroy the shared library instances
			if (nullptr != mDxgiSharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mDxgiSharedLibrary));
			}
			if (nullptr != mD3D11SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3D11SharedLibrary));
			}
			if (nullptr != mD3DCompilerSharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3DCompilerSharedLibrary));
			}
			#ifdef DYNAMIC_AMD_AGS
				if (nullptr != mAmdAgsSharedLibrary)
				{
					if (nullptr != agsDeInit && AGS_SUCCESS != agsDeInit(mAgsContext))
					{
						RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to unload AMG AGS")
					}
					::FreeLibrary(static_cast<HMODULE>(mAmdAgsSharedLibrary));
				}
			#else
				if (AGS_SUCCESS != agsDeInit(mAgsContext))
				{
					RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to unload AMG AGS")
				}
			#endif
			if (nullptr != mNvAPISharedLibrary)
			{
				if (nullptr != NvAPI_Unload && 0 != NvAPI_Unload())
				{
					RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to unload NvAPI")
				}
				::FreeLibrary(static_cast<HMODULE>(mNvAPISharedLibrary));
			}
		}

		/**
		*  @brief
		*    Return whether or not Direct3D 11 is available
		*
		*  @return
		*    "true" if Direct3D 11 is available, else "false"
		*/
		[[nodiscard]] bool isDirect3D11Avaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the DXGI, D3D11 and D3DCompiler entry points
					mEntryPointsRegistered = (loadDxgiEntryPoints() && loadD3D11EntryPoints() && loadD3DCompilerEntryPoints());

					// AMD AGS and NvAPI for e.g. multi-draw-indirect support
					if (mEntryPointsRegistered)
					{
						// Check whether or not the primary DXGI adapter is an AMD GPU
						bool amdDxgiAdapter = false;
						bool nvidiaDxgiAdapter = false;
						{
							// Get the primary DXGI adapter
							IDXGIFactory* dxgiFactory = nullptr;
							FAILED_DEBUG_BREAK(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory))
							if (nullptr == dxgiFactory)
							{
								// Error!
								return false;
							}
							IDXGIAdapter* dxgiAdapter = nullptr;
							FAILED_DEBUG_BREAK(dxgiFactory->EnumAdapters(0, &dxgiAdapter))
							DXGI_ADAPTER_DESC dxgiAdapterDesc = {};
							FAILED_DEBUG_BREAK(dxgiAdapter->GetDesc(&dxgiAdapterDesc))
							if (0x1414 == dxgiAdapterDesc.VendorId)	// 0x1414 = "Capture Adapter" when using Visual Studio graphics debugger
							{
								RHI_LOG(mDirect3D11Rhi.getContext(), COMPATIBILITY_WARNING, "Direct3D 11 capture adapter used (e.g. Visual Studio graphics debugger), AMD AGS and NvAPI support disabled")
							}
							else
							{
								amdDxgiAdapter = (0x1002 == dxgiAdapterDesc.VendorId);		// 0x1002 -> See "How-To Identify the Manufacturer and Model of an AMD Graphics Card" at http://support.amd.com/en-us/kb-articles/Pages/HowtoidentifythemodelofanATIgraphicscard.aspx
								nvidiaDxgiAdapter = (0x10DE == dxgiAdapterDesc.VendorId);	// 0x10DE -> See "Device IDs" at http://www.nvidia.com/object/device_ids.html
							}
							dxgiAdapter->Release();
							dxgiFactory->Release();
						}

						// Optional vendor specific part: AMD AGS
						if (amdDxgiAdapter)
						{
							g_AmdAgsAllocator = &mDirect3D11Rhi.getContext().getAllocator();
							#ifdef DYNAMIC_AMD_AGS
								#ifdef ARCHITECTURE_X64
									static constexpr const char* AMD_AGS_SHARED_LIBRARY_NAME = "amd_ags_x64.dll";
								#else
									static constexpr const char* AMD_AGS_SHARED_LIBRARY_NAME = "amd_ags_x86.dll";
								#endif
								mAmdAgsSharedLibrary = ::LoadLibraryExA(AMD_AGS_SHARED_LIBRARY_NAME, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
								if (nullptr != mAmdAgsSharedLibrary)
								{
									if (!loadAmdAgsEntryPoints())
									{
										RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to load AMD AGS function entry points")
										::FreeLibrary(static_cast<HMODULE>(mAmdAgsSharedLibrary));
										mAmdAgsSharedLibrary									  = nullptr;
										agsInit													  = nullptr;
										agsDeInit												  = nullptr;
										agsDriverExtensionsDX11_CreateDevice					  = nullptr;
										agsDriverExtensionsDX11_DestroyDevice					  = nullptr;
										agsDriverExtensionsDX11_MultiDrawInstancedIndirect		  = nullptr;
										agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect = nullptr;
									}
								}
								else
								{
									RHI_LOG(mDirect3D11Rhi.getContext(), PERFORMANCE_WARNING, "Direct3D 11: Failed to load the AMD AGS shared library \"%s\"", AMD_AGS_SHARED_LIBRARY_NAME)
								}
							#else
							{
								// Initialize AMD AGS (e.g. for multi-indirect-draw support)
								const AGSConfiguration agsConfiguration = { &AmdAgsAllocCallback, &AmdAgsFreeCallback };
								if (AGS_SUCCESS == agsInit(&mAgsContext, &agsConfiguration, nullptr))
								{
									RHI_LOG(mDirect3D11Rhi.getContext(), TRACE, "Direct3D 11: Successfully initialized AMD AGS")
								}
								else
								{
									RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to initialize AMD AGS")
								}
							}
							#endif
						}

						// Optional vendor specific part: NvAPI
						if (nvidiaDxgiAdapter)
						{
							#ifdef ARCHITECTURE_X64
								static constexpr const char* NVAPI_SHARED_LIBRARY_NAME = "nvapi64.dll";
							#else
								static constexpr const char* NVAPI_SHARED_LIBRARY_NAME = "nvapi.dll";
							#endif
							mNvAPISharedLibrary = ::LoadLibraryExA(NVAPI_SHARED_LIBRARY_NAME, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
							if (nullptr != mNvAPISharedLibrary)
							{
								if (!loadNvAPIEntryPoints())
								{
									RHI_LOG(mDirect3D11Rhi.getContext(), PERFORMANCE_WARNING, "Direct3D 11: Failed to load NvAPI function entry points, maybe a graphics debugger like RenderDoc disabled NvAPI")
									::FreeLibrary(static_cast<HMODULE>(mNvAPISharedLibrary));
									mNvAPISharedLibrary							  = nullptr;
									NvAPI_Initialize							  = nullptr;
									NvAPI_Unload								  = nullptr;
									NvAPI_D3D11_MultiDrawInstancedIndirect		  = nullptr;
									NvAPI_D3D11_MultiDrawIndexedInstancedIndirect = nullptr;
								}
							}
							else
							{
								RHI_LOG(mDirect3D11Rhi.getContext(), PERFORMANCE_WARNING, "Direct3D 11: Failed to load the NvAPI shared library \"%s\"", NVAPI_SHARED_LIBRARY_NAME)
							}
						}
					}
				}
			}

			// Entry points successfully registered?
			return mEntryPointsRegistered;
		}

		/**
		*  @brief
		*    Return the AMD AGS instance
		*
		*  @return
		*    The AMD AGS instance, can be a null pointer, don't destroy the returned instance
		*/
		[[nodiscard]] inline AGSContext* getAgsContext() const
		{
			return mAgsContext;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D11RuntimeLinking(const Direct3D11RuntimeLinking& source) = delete;
		Direct3D11RuntimeLinking& operator =(const Direct3D11RuntimeLinking& source) = delete;

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
			mDxgiSharedLibrary = ::LoadLibraryExA("dxgi.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (nullptr != mDxgiSharedLibrary)
			{
				mD3D11SharedLibrary = ::LoadLibraryExA("d3d11.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr != mD3D11SharedLibrary)
				{
					mD3DCompilerSharedLibrary = ::LoadLibraryExA("D3DCompiler_47.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr == mD3DCompilerSharedLibrary)
					{
						RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to load in the shared Direct3D 11 library \"D3DCompiler_47.dll\"")
					}
				}
				else
				{
					RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to load in the shared Direct3D 11 library \"d3d11.dll\"")
				}
			}
			else
			{
				RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to load in the shared Direct3D 11 library \"dxgi.dll\"")
			}

			// Done
			return (nullptr != mDxgiSharedLibrary && nullptr != mD3D11SharedLibrary && nullptr != mD3DCompilerSharedLibrary);
		}

		/**
		*  @brief
		*    Load the DXGI entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadDxgiEntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																						\
				if (result)																																										\
				{																																												\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mDxgiSharedLibrary), #funcName);																						\
					if (nullptr != symbol)																																						\
					{																																											\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																														\
					}																																											\
					else																																										\
					{																																											\
						wchar_t moduleFilename[MAX_PATH];																																		\
						moduleFilename[0] = '\0';																																				\
						::GetModuleFileNameW(static_cast<HMODULE>(mDxgiSharedLibrary), moduleFilename, MAX_PATH);																				\
						RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 11 DXGI shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																							\
					}																																											\
				}

			// Load the entry points
			IMPORT_FUNC(CreateDXGIFactory)

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the D3D11 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3D11EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																					\
				if (result)																																									\
				{																																											\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3D11SharedLibrary), #funcName);																					\
					if (nullptr != symbol)																																					\
					{																																										\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
					}																																										\
					else																																									\
					{																																										\
						wchar_t moduleFilename[MAX_PATH];																																	\
						moduleFilename[0] = '\0';																																			\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3D11SharedLibrary), moduleFilename, MAX_PATH);																			\
						RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 11 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																						\
					}																																										\
				}

			// Load the entry points
			IMPORT_FUNC(D3D11CreateDevice)

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the D3DCompiler entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3DCompilerEntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																					\
				if (result)																																									\
				{																																											\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3DCompilerSharedLibrary), #funcName);																			\
					if (nullptr != symbol)																																					\
					{																																										\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
					}																																										\
					else																																									\
					{																																										\
						wchar_t moduleFilename[MAX_PATH];																																	\
						moduleFilename[0] = '\0';																																			\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3DCompilerSharedLibrary), moduleFilename, MAX_PATH);																	\
						RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 11 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																						\
					}																																										\
				}

			// Load the entry points
			IMPORT_FUNC(D3DCompile)
			IMPORT_FUNC(D3DCreateBlob)

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		#ifdef DYNAMIC_AMD_AGS
			/**
			*  @brief
			*    Load the AMD AGS entry points
			*
			*  @return
			*    "true" if all went fine, else "false"
			*/
			[[nodiscard]] bool loadAmdAgsEntryPoints()
			{
				bool result = true;	// Success by default

				// Define a helper macro
				#define IMPORT_FUNC(funcName)																																				\
					if (result)																																								\
					{																																										\
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mAmdAgsSharedLibrary), #funcName);																				\
						if (nullptr != symbol)																																				\
						{																																									\
							*(reinterpret_cast<void**>(&(funcName))) = symbol;																												\
						}																																									\
						else																																								\
						{																																									\
							wchar_t moduleFilename[MAX_PATH];																																\
							moduleFilename[0] = '\0';																																		\
							::GetModuleFileNameW(static_cast<HMODULE>(mAmdAgsSharedLibrary), moduleFilename, MAX_PATH);																		\
							RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the AMD AGS shared library \"%s\"", #funcName, moduleFilename)	\
							result = false;																																					\
						}																																									\
					}

				// Load the entry points
				IMPORT_FUNC(agsInit)
				IMPORT_FUNC(agsDeInit)
				IMPORT_FUNC(agsDriverExtensionsDX11_CreateDevice)
				IMPORT_FUNC(agsDriverExtensionsDX11_DestroyDevice)
				IMPORT_FUNC(agsDriverExtensionsDX11_MultiDrawInstancedIndirect)
				IMPORT_FUNC(agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect)

				// Undefine the helper macro
				#undef IMPORT_FUNC

				// Initialize AMD AGS (e.g. for multi-indirect-draw support)
				if (nullptr != agsInit)
				{
					const AGSConfiguration agsConfiguration = { &AmdAgsAllocCallback, &AmdAgsFreeCallback };
					if (AGS_SUCCESS == agsInit(&mAgsContext, &agsConfiguration, nullptr))
					{
						RHI_LOG(mDirect3D11Rhi.getContext(), TRACE, "Direct3D 11: Successfully initialized AMD AGS")
					}
					else
					{
						RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to initialize AMD AGS")
						result = false;
					}
				}

				// Done
				return result;
			}
		#endif

		/**
		*  @brief
		*    Load the NvAPI entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadNvAPIEntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																		\
				void* symbol = ::GetProcAddress(static_cast<HMODULE>(mNvAPISharedLibrary), #funcName);																			\
				if (nullptr != symbol)																																			\
				{																																								\
					*(reinterpret_cast<void**>(&(funcName))) = symbol;																											\
				}																																								\
				else																																							\
				{																																								\
					wchar_t moduleFilename[MAX_PATH];																															\
					moduleFilename[0] = '\0';																																	\
					::GetModuleFileNameW(static_cast<HMODULE>(mNvAPISharedLibrary), moduleFilename, MAX_PATH);																	\
					RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the NvAPI shared library \"%s\"", #funcName, moduleFilename)	\
					result = false;																																				\
				}
			FNDEF_NvAPI(void*,	nvapi_QueryInterface,	(unsigned int offset));
			#define nvapi_QueryInterface	FNPTR(nvapi_QueryInterface)
			#define IMPORT_NVAPI_FUNC(funcName, ID) \
				*(reinterpret_cast<void**>(&(funcName))) = nvapi_QueryInterface(ID); \
				if (nullptr == funcName) \
				{ \
					result = false; \
				}

			// Load the entry points
			nvapi_QueryInterface = nullptr;
			IMPORT_FUNC(nvapi_QueryInterface)

			// Query function pointers
			if (nullptr != nvapi_QueryInterface)
			{
				IMPORT_NVAPI_FUNC(NvAPI_Initialize, 0x150E828UL)
				IMPORT_NVAPI_FUNC(NvAPI_Unload, 0xD22BDD7EUL)
				IMPORT_NVAPI_FUNC(NvAPI_D3D11_MultiDrawInstancedIndirect, 0xD4E26BBF)
				IMPORT_NVAPI_FUNC(NvAPI_D3D11_MultiDrawIndexedInstancedIndirect, 0x59E890F9)
			}

			// Undefine the helper macro
			#undef IMPORT_NVAPI_FUNC
			#undef NvAPI_QueryInterface
			#undef IMPORT_FUNC

			// Initialize NvAPI (e.g. for multi-indirect-draw support)
			if (result)
			{
				if (nullptr != NvAPI_Initialize && 0 == NvAPI_Initialize())
				{
					RHI_LOG(mDirect3D11Rhi.getContext(), TRACE, "Direct3D 11: Successfully initialized NvAPI")
				}
				else
				{
					RHI_LOG(mDirect3D11Rhi.getContext(), CRITICAL, "Direct3D 11: Failed to initialize NvAPI")
					result = false;
				}
			}

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D11Rhi&	mDirect3D11Rhi;				///< Owner Direct3D 11 RHI instance
		void*			mDxgiSharedLibrary;			///< DXGI shared library, can be a null pointer
		void*			mD3D11SharedLibrary;		///< D3D11 shared library, can be a null pointer
		void*			mD3DCompilerSharedLibrary;	///< D3DCompiler shared library, can be a null pointer
		#ifdef DYNAMIC_AMD_AGS
			void*		mAmdAgsSharedLibrary;		///< AMD AGS shared library, can be a null pointer
		#endif
		AGSContext*		mAgsContext;				///< AMD AGS context, can be a null pointer
		void*			mNvAPISharedLibrary;		///< NvAPI shared library, can be a null pointer
		bool			mEntryPointsRegistered;		///< Entry points successfully registered?
		bool			mInitialized;				///< Already initialized?


	};




	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	// In order to assign debug names to Direct3D resources we need to use the "WKPDID_D3DDebugObjectName"-GUID. This GUID
	// is defined within the "D3Dcommon.h" header and it's required to add the library "dxguid.lib" in which the symbol
	// is defined.
	// -> See "ID3D11Device::SetPrivateData method"-documentation at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/ff476533%28v=vs.85%29.aspx
	//    The "Community Additions" states: "If you get a missing symbol error: Note that WKPDID_D3DDebugObjectName
	//    requires both that you include D3Dcommon.h, and that you link against dxguid.lib."
	// -> We don't want to deal with a 800 KB library "just" for such a tiny symbol for several reasons. For once it's not
	//    allowed to redistribute "dxguid.lib" due to DirectX SDK licensing terms. Another reason for avoiding libraries
	//    were ever possible is that every library will increase the complexity of the build system and will also make
	//    it harder to port to other platforms - we already would need 32 bit and 64 bit versions for standard Windows
	//    systems. We don't want that just for resolving a tiny symbol.
	//
	// "WKPDID_D3DDebugObjectName" is defined within the "D3Dcommon.h"-header as
	//   DEFINE_GUID(WKPDID_D3DDebugObjectName,0x429b8c22,0x9188,0x4b0c,0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00);
	//
	// While the "DEFINE_GUID"-macro is defined within the "Guiddef.h"-header as
	//   #ifdef INITGUID
	//   #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	//           EXTERN_C const GUID DECLSPEC_SELECTANY name \
	//                   = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
	//   #else
	//   #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	//       EXTERN_C const GUID FAR name
	//   #endif // INITGUID
	//
	// "GUID" is a structure defined within the "Guiddef.h"-header as
	//   typedef struct _GUID {
	//       unsigned long  Data1;
	//       unsigned short Data2;
	//       unsigned short Data3;
	//       unsigned char  Data4[ 8 ];
	//   } GUID;
	#define RHI_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) constexpr GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
	RHI_DEFINE_GUID(WKPDID_D3DDebugObjectName, 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00);




	//[-------------------------------------------------------]
	//[ Global functions                                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Creates, loads and compiles a shader from source code
	*
	*  @param[in] context
	*    RHI context
	*  @param[in] shaderModel
	*    ASCII shader model (for example "vs_4_0", "gs_4_0", "ps_4_0"), must be a valid pointer
	*  @param[in] sourceCode
	*    ASCII shader ASCII source code, must be a valid pointer
	*  @param[in] entryPoint
	*    Optional ASCII entry point, if null pointer "main" is used
	*  @param[in] optimizationLevel
	*    Optimization level
	*
	*  @return
	*    The loaded and compiled shader, can be a null pointer, release the instance if you no longer need it
	*/
	[[nodiscard]] ID3DBlob* loadShaderFromSourcecode(const Rhi::Context& context, const char* shaderModel, const char* sourceCode, const char* entryPoint, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel)
	{
		// Sanity checks
		RHI_ASSERT(context, nullptr != shaderModel, "Invalid Direct3D 11 shader model")
		RHI_ASSERT(context, nullptr != sourceCode, "Invalid Direct3D 11 shader source code")

		// Get compile flags
		// -> "DX12 Do's And Don'ts" ( https://developer.nvidia.com/dx12-dos-and-donts ) "Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible"
		UINT compileFlags = (D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ALL_RESOURCES_BOUND);
		switch (optimizationLevel)
		{
			case Rhi::IShaderLanguage::OptimizationLevel::DEBUG:
				compileFlags |= D3DCOMPILE_DEBUG;
				compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
				break;

			case Rhi::IShaderLanguage::OptimizationLevel::NONE:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
				break;

			case Rhi::IShaderLanguage::OptimizationLevel::LOW:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
				break;

			case Rhi::IShaderLanguage::OptimizationLevel::MEDIUM:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
				break;

			case Rhi::IShaderLanguage::OptimizationLevel::HIGH:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
				break;

			case Rhi::IShaderLanguage::OptimizationLevel::ULTRA:
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
				break;
		}

		// Compile
		ID3DBlob* d3dBlob = nullptr;
		ID3DBlob* errorD3dBlob = nullptr;
		if (FAILED(D3DCompile(sourceCode, strlen(sourceCode), nullptr, nullptr, nullptr, entryPoint ? entryPoint : "main", shaderModel, compileFlags, 0, &d3dBlob, &errorD3dBlob)))
		{
			if (nullptr != errorD3dBlob)
			{
				if (context.getLog().print(Rhi::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), static_cast<char*>(errorD3dBlob->GetBufferPointer())))
				{
					DEBUG_BREAK;
				}
				errorD3dBlob->Release();
			}
			return nullptr;
		}
		if (nullptr != errorD3dBlob)
		{
			errorD3dBlob->Release();
		}

		// Done
		return d3dBlob;
	}

	void handleDeviceLost(const Direct3D11Rhi& direct3D11Rhi, HRESULT result)
	{
		// If the device was removed either by a disconnection or a driver upgrade, we must recreate all device resources
		if (DXGI_ERROR_DEVICE_REMOVED == result || DXGI_ERROR_DEVICE_RESET == result)
		{
			if (DXGI_ERROR_DEVICE_REMOVED == result)
			{
				result = direct3D11Rhi.getD3D11Device()->GetDeviceRemovedReason();
			}
			RHI_LOG(direct3D11Rhi.getContext(), CRITICAL, "Direct3D 11 device lost on present: Reason code 0x%08X", static_cast<unsigned int>(result))

			// TODO(co) Add device lost handling if needed. Probably more complex to recreate all device resources.
		}
	}




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Mapping.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 mapping
	*/
	class Mapping final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Rhi::VertexAttributeFormat and semantic               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::VertexAttributeFormat" to Direct3D 11 format
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    Direct3D 11 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D11Format(Rhi::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R32_FLOAT,			// Rhi::VertexAttributeFormat::FLOAT_1
				DXGI_FORMAT_R32G32_FLOAT,		// Rhi::VertexAttributeFormat::FLOAT_2
				DXGI_FORMAT_R32G32B32_FLOAT,	// Rhi::VertexAttributeFormat::FLOAT_3
				DXGI_FORMAT_R32G32B32A32_FLOAT,	// Rhi::VertexAttributeFormat::FLOAT_4
				DXGI_FORMAT_R8G8B8A8_UNORM,		// Rhi::VertexAttributeFormat::R8G8B8A8_UNORM
				DXGI_FORMAT_R8G8B8A8_UINT,		// Rhi::VertexAttributeFormat::R8G8B8A8_UINT
				DXGI_FORMAT_R16G16_SINT,		// Rhi::VertexAttributeFormat::SHORT_2
				DXGI_FORMAT_R16G16B16A16_SINT,	// Rhi::VertexAttributeFormat::SHORT_4
				DXGI_FORMAT_R32_UINT			// Rhi::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		//[-------------------------------------------------------]
		//[ Rhi::BufferUsage                                      ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::BufferUsage" to Direct3D 11 usage and CPU access flags
		*
		*  @param[in]  bufferUsage
		*    "Rhi::BufferUsage" to map
		*  @param[out] cpuAccessFlags
		*    Receives the CPU access flags
		*
		*  @return
		*    Direct3D 11 usage
		*/
		[[nodiscard]] static D3D11_USAGE getDirect3D11UsageAndCPUAccessFlags(Rhi::BufferUsage bufferUsage, uint32_t& cpuAccessFlags)
		{
			// Direct3D 11 only supports a subset of the OpenGL usage indications
			// -> See "D3D11_USAGE enumeration "-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/ff476259%28v=vs.85%29.aspx
			switch (bufferUsage)
			{
				case Rhi::BufferUsage::STREAM_DRAW:
				case Rhi::BufferUsage::STREAM_COPY:
				case Rhi::BufferUsage::STATIC_DRAW:
				case Rhi::BufferUsage::STATIC_COPY:
					cpuAccessFlags = 0;
					return D3D11_USAGE_IMMUTABLE;

				case Rhi::BufferUsage::STREAM_READ:
				case Rhi::BufferUsage::STATIC_READ:
					cpuAccessFlags = D3D11_CPU_ACCESS_READ;
					return D3D11_USAGE_STAGING;

				case Rhi::BufferUsage::DYNAMIC_DRAW:
				case Rhi::BufferUsage::DYNAMIC_COPY:
					cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
					return D3D11_USAGE_DYNAMIC;

				default:
				case Rhi::BufferUsage::DYNAMIC_READ:
					cpuAccessFlags = 0;
					return D3D11_USAGE_DEFAULT;
			}
		}

		//[-------------------------------------------------------]
		//[ Rhi::IndexBufferFormat                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::IndexBufferFormat" to Direct3D 11 format
		*
		*  @param[in] indexBufferFormat
		*    "Rhi::IndexBufferFormat" to map
		*
		*  @return
		*    Direct3D 11 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D11Format(Rhi::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R32_UINT,	// Rhi::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Direct3D 11
				DXGI_FORMAT_R16_UINT,	// Rhi::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				DXGI_FORMAT_R32_UINT	// Rhi::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Rhi::TextureFormat                                    ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::TextureFormat" to Direct3D 11 format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    Direct3D 11 format
		*
		*  @remarks
		*    For textures used as depth stencil render target, Direct3D 11 format handling becomes a little bit more complex due to
		*    the offered flexibility. For example the abstract texture format "Rhi::TextureFormat::D32_FLOAT" translates into
		*    - Direct3D 11 resource format is "DXGI_FORMAT_R32_TYPELESS"
		*    - Direct3D 11 shader resource view format is "DXGI_FORMAT_R32_FLOAT"
		*    - Direct3D 11 depth stencil view format is "DXGI_FORMAT_D32_FLOAT"
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D11Format(Rhi::TextureFormat::Enum textureFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R8_UNORM,				// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				DXGI_FORMAT_B8G8R8X8_UNORM,			// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				DXGI_FORMAT_R8G8B8A8_UNORM,			// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,	// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_B8G8R8A8_UNORM,			// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R11G11B10_FLOAT,		// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				DXGI_FORMAT_R16G16B16A16_FLOAT,		// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_R32G32B32A32_FLOAT,		// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_BC1_UNORM,				// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				DXGI_FORMAT_BC1_UNORM_SRGB,			// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC2_UNORM,				// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC2_UNORM_SRGB,			// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC3_UNORM,				// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC3_UNORM_SRGB,			// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC4_UNORM,				// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				DXGI_FORMAT_BC5_UNORM,				// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				DXGI_FORMAT_UNKNOWN,				// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 11
				DXGI_FORMAT_R16_UNORM,				// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				DXGI_FORMAT_R32_UINT,				// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				DXGI_FORMAT_R32_FLOAT,				// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				DXGI_FORMAT_D32_FLOAT,				// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				DXGI_FORMAT_R16G16_SNORM,			// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_R16G16_FLOAT,			// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_UNKNOWN					// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		static DXGI_FORMAT getDirect3D11ResourceFormat(Rhi::TextureFormat::Enum textureFormat)
		{
			// Only "Rhi::TextureFormat::D32_FLOAT" has to be handled in a different way
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R8_UNORM,				// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				DXGI_FORMAT_B8G8R8X8_UNORM,			// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				DXGI_FORMAT_R8G8B8A8_UNORM,			// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,	// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_B8G8R8A8_UNORM,			// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R11G11B10_FLOAT,		// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				DXGI_FORMAT_R16G16B16A16_FLOAT,		// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_R32G32B32A32_FLOAT,		// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_BC1_UNORM,				// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				DXGI_FORMAT_BC1_UNORM_SRGB,			// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC2_UNORM,				// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC2_UNORM_SRGB,			// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC3_UNORM,				// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC3_UNORM_SRGB,			// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC4_UNORM,				// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				DXGI_FORMAT_BC5_UNORM,				// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				DXGI_FORMAT_UNKNOWN,				// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 11
				DXGI_FORMAT_R16_UNORM,				// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				DXGI_FORMAT_R32_UINT,				// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				DXGI_FORMAT_R32_FLOAT,				// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				DXGI_FORMAT_R32_TYPELESS,			// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				DXGI_FORMAT_R16G16_SNORM,			// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_R16G16_FLOAT,			// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_UNKNOWN					// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		[[nodiscard]] static DXGI_FORMAT getDirect3D11ShaderResourceViewFormat(Rhi::TextureFormat::Enum textureFormat)
		{
			// Only "Rhi::TextureFormat::D32_FLOAT" has to be handled in a different way
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R8_UNORM,				// Rhi::TextureFormat::R8            - 8-bit pixel format, all bits red
				DXGI_FORMAT_B8G8R8X8_UNORM,			// Rhi::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				DXGI_FORMAT_R8G8B8A8_UNORM,			// Rhi::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,	// Rhi::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_B8G8R8A8_UNORM,			// Rhi::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R11G11B10_FLOAT,		// Rhi::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				DXGI_FORMAT_R16G16B16A16_FLOAT,		// Rhi::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_R32G32B32A32_FLOAT,		// Rhi::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_BC1_UNORM,				// Rhi::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				DXGI_FORMAT_BC1_UNORM_SRGB,			// Rhi::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC2_UNORM,				// Rhi::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC2_UNORM_SRGB,			// Rhi::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC3_UNORM,				// Rhi::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC3_UNORM_SRGB,			// Rhi::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC4_UNORM,				// Rhi::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				DXGI_FORMAT_BC5_UNORM,				// Rhi::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				DXGI_FORMAT_UNKNOWN,				// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 11
				DXGI_FORMAT_R16_UNORM,				// Rhi::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				DXGI_FORMAT_R32_UINT,				// Rhi::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				DXGI_FORMAT_R32_FLOAT,				// Rhi::TextureFormat::R32_FLOAT     - 32-bit float format
				DXGI_FORMAT_R32_FLOAT,				// Rhi::TextureFormat::D32_FLOAT     - 32-bit float depth format
				DXGI_FORMAT_R16G16_SNORM,			// Rhi::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_R16G16_FLOAT,			// Rhi::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_UNKNOWN					// Rhi::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/ResourceGroup.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 resource group class
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
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(Rhi::IRhi& rhi, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IResourceGroup(rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfResources(numberOfResources),
			mResources(RHI_MALLOC_TYPED(rhi.getContext(), Rhi::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr)
		{
			// Process all resources and add our reference to the RHI resource
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Rhi::IResource* resource = *resources;
				RHI_ASSERT(rhi.getContext(), nullptr != resource, "Invalid Direct3D 11 resource")
				mResources[resourceIndex] = resource;
				resource->addReference();
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RHI_MALLOC_TYPED(rhi.getContext(), Rhi::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
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
		uint32_t			 mNumberOfResources;	///< Number of resources this resource group groups together
		Rhi::IResource**	 mResources;			///< RHI resources, we keep a reference to it
		Rhi::ISamplerState** mSamplerStates;		///< Sampler states, we keep a reference to it


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/RootSignature.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(Direct3D11Rhi& direct3D11Rhi, const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRootSignature(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature)
		{
			const Rhi::Context& context = direct3D11Rhi.getContext();

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
		[[nodiscard]] virtual Rhi::IResourceGroup* createResourceGroup([[maybe_unused]] uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Rhi::IRhi& rhi = getRhi();

			// Sanity checks
			RHI_ASSERT(rhi.getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The Direct3D 11 root parameter index is out-of-bounds")
			RHI_ASSERT(rhi.getContext(), numberOfResources > 0, "The number of Direct3D 11 resources must not be zero")
			RHI_ASSERT(rhi.getContext(), nullptr != resources, "The Direct3D 11 resource pointers must be valid")

			// Create resource group
			return RHI_NEW(rhi.getContext(), ResourceGroup)(rhi, numberOfResources, resources, samplerStates RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
	//[ Direct3D11Rhi/Buffer/VertexBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Rhi::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(Direct3D11Rhi& direct3D11Rhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexBuffer(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11Buffer(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Direct3D 11 buffer description
			D3D11_BUFFER_DESC d3d11BufferDesc;
			d3d11BufferDesc.ByteWidth           = numberOfBytes;
			d3d11BufferDesc.Usage               = Mapping::getDirect3D11UsageAndCPUAccessFlags(bufferUsage, d3d11BufferDesc.CPUAccessFlags);
			d3d11BufferDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
			//d3d11BufferDesc.CPUAccessFlags    = <filled above>;
			d3d11BufferDesc.MiscFlags           = 0;
			d3d11BufferDesc.StructureByteStride = 0;

			// Set bind flags
			// -> Using a structured vertex buffer would be handy inside shader source codes, sadly this isn't possible with Direct3D 11 and will result in the following error:
			//    D3D11 ERROR: ID3D11Device::CreateBuffer: Buffers created with D3D11_RESOURCE_MISC_BUFFER_STRUCTURED cannot specify any of the following listed bind flags.  The following BindFlags bits (0x9) are set: D3D11_BIND_VERTEX_BUFFER (1), D3D11_BIND_INDEX_BUFFER (0), D3D11_BIND_CONSTANT_BUFFER (0), D3D11_BIND_STREAM_OUTPUT (0), D3D11_BIND_RENDER_TARGET (0), or D3D11_BIND_DEPTH_STENCIL (0). [ STATE_CREATION ERROR #68: CREATEBUFFER_INVALIDMISCFLAGS]
			if (bufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE)
			{
				d3d11BufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				d3d11BufferDesc.MiscFlags  = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (bufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS)
			{
				d3d11BufferDesc.Usage			= D3D11_USAGE_DEFAULT;
				d3d11BufferDesc.BindFlags	   |= D3D11_BIND_UNORDERED_ACCESS;
				d3d11BufferDesc.CPUAccessFlags  = 0;
				d3d11BufferDesc.MiscFlags       = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}

			// Data given?
			if (nullptr != data)
			{
				// Direct3D 11 subresource data
				D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
				d3d11SubresourceData.pSysMem          = data;
				d3d11SubresourceData.SysMemPitch      = 0;
				d3d11SubresourceData.SysMemSlicePitch = 0;

				// Create the Direct3D 11 vertex buffer
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mD3D11Buffer))
			}
			else
			{
				// Create the Direct3D 11 vertex buffer
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mD3D11Buffer))
			}

			// Create the Direct3D 11 shader resource view instance
			if (bufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE)
			{
				// Direct3D 11 shader resource view description
				D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
				d3d11ShaderResourceViewDesc.Format				 = DXGI_FORMAT_R32_TYPELESS;
				d3d11ShaderResourceViewDesc.ViewDimension		 = D3D11_SRV_DIMENSION_BUFFEREX;
				d3d11ShaderResourceViewDesc.BufferEx.NumElements = numberOfBytes / sizeof(uint32_t);
				d3d11ShaderResourceViewDesc.BufferEx.Flags		 = D3D11_BUFFEREX_SRV_FLAG_RAW;

				// Create the Direct3D 11 shader resource view instance
				// -> HLSL usage example: "ByteAddressBuffer InputVertexBuffer : register(t0);"
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Buffer, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
			}

			// Create the Direct3D 11 unordered access view instance
			if (bufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS)
			{
				// Direct3D 11 unordered access view description
				D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
				d3d11UnorderedAccessViewDesc.Format				= DXGI_FORMAT_R32_TYPELESS;
				d3d11UnorderedAccessViewDesc.ViewDimension		= D3D11_UAV_DIMENSION_BUFFER;
				d3d11UnorderedAccessViewDesc.Buffer.NumElements = numberOfBytes / sizeof(uint32_t);
				d3d11UnorderedAccessViewDesc.Buffer.Flags		= D3D11_BUFFER_UAV_FLAG_RAW;

				// Create the Direct3D 11 unordered access view instance
				// -> HLSL usage example: "RWByteAddressBuffer OutputVertexBuffer : register(u0);"
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Buffer, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VBO", 6)	// 6 = "VBO: " including terminating zero
				if (nullptr != mD3D11Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexBuffer() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
				mD3D11ShaderResourceView = nullptr;
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
				mD3D11UnorderedAccessView = nullptr;
			}
			if (nullptr != mD3D11Buffer)
			{
				mD3D11Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D vertex buffer instance
		*
		*  @return
		*    The Direct3D vertex buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Buffer* getD3D11Buffer() const
		{
			return mD3D11Buffer;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
		ID3D11Buffer*			   mD3D11Buffer;				///< Direct3D vertex buffer instance, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/IndexBuffer.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 index buffer object (IBO, "element array buffer" in OpenGL terminology) class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 rhi instance
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
		IndexBuffer(Direct3D11Rhi& direct3D11Rhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndexBuffer(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11Buffer(nullptr),
			mDXGIFormat(DXGI_FORMAT_UNKNOWN),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// "Rhi::IndexBufferFormat::UnsignedChar" is not supported by Direct3D 11
			// -> See "Input assembler index buffer resources" ("D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER"): https://msdn.microsoft.com/en-us/library/windows/desktop/ff471325%28v=vs.85%29.aspx
			if (Rhi::IndexBufferFormat::UNSIGNED_CHAR == indexBufferFormat)
			{
				RHI_LOG(direct3D11Rhi.getContext(), CRITICAL, "\"Rhi::IndexBufferFormat::UNSIGNED_CHAR\" is not supported by Direct3D 11")
			}
			else
			{
				// Set the DXGI format
				mDXGIFormat = Mapping::getDirect3D11Format(indexBufferFormat);

				// Direct3D 11 buffer description
				D3D11_BUFFER_DESC d3d11BufferDesc;
				d3d11BufferDesc.ByteWidth           = numberOfBytes;
				d3d11BufferDesc.Usage               = Mapping::getDirect3D11UsageAndCPUAccessFlags(bufferUsage, d3d11BufferDesc.CPUAccessFlags);
				d3d11BufferDesc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
				//d3d11BufferDesc.CPUAccessFlags    = <filled above>;
				d3d11BufferDesc.MiscFlags           = 0;
				d3d11BufferDesc.StructureByteStride = 0;

				// Set bind flags
				if (bufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE)
				{
					d3d11BufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				}
				if (bufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS)
				{
					d3d11BufferDesc.Usage			= D3D11_USAGE_DEFAULT;
					d3d11BufferDesc.BindFlags	   |= D3D11_BIND_UNORDERED_ACCESS;
					d3d11BufferDesc.CPUAccessFlags  = 0;
				}

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 11 subresource data
					D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
					d3d11SubresourceData.pSysMem          = data;
					d3d11SubresourceData.SysMemPitch      = 0;
					d3d11SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 11 index buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mD3D11Buffer))
				}
				else
				{
					// Create the Direct3D 11 index buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mD3D11Buffer))
				}

				// Create the Direct3D 11 shader resource view instance
				if (bufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format				 = mDXGIFormat;
					d3d11ShaderResourceViewDesc.ViewDimension		 = D3D11_SRV_DIMENSION_BUFFER;
					d3d11ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Rhi::IndexBufferFormat::getNumberOfBytesPerElement(indexBufferFormat);

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Buffer, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (bufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format				= mDXGIFormat;
					d3d11UnorderedAccessViewDesc.ViewDimension		= D3D11_UAV_DIMENSION_BUFFER;
					d3d11UnorderedAccessViewDesc.Buffer.NumElements = numberOfBytes / Rhi::IndexBufferFormat::getNumberOfBytesPerElement(indexBufferFormat);

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Buffer, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IBO", 6)	// 6 = "IBO: " including terminating zero
					if (nullptr != mD3D11Buffer)
					{
						FAILED_DEBUG_BREAK(mD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
					if (nullptr != mD3D11ShaderResourceView)
					{
						FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
					if (nullptr != mD3D11UnorderedAccessView)
					{
						FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~IndexBuffer() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
				mD3D11ShaderResourceView = nullptr;
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
				mD3D11UnorderedAccessView = nullptr;
			}
			if (nullptr != mD3D11Buffer)
			{
				mD3D11Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D index buffer instance
		*
		*  @return
		*    The Direct3D index buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Buffer* getD3D11Buffer() const
		{
			return mD3D11Buffer;
		}

		/**
		*  @brief
		*    Return the DXGI index buffer data format
		*
		*  @return
		*    The DXGI index buffer data format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDXGIFormat() const
		{
			return mDXGIFormat;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
		ID3D11Buffer*			   mD3D11Buffer;				///< Direct3D index buffer instance, can be a null pointer
		DXGI_FORMAT				   mDXGIFormat;					///< DXGI index buffer data format
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/VertexArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 vertex array class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		VertexArray(Direct3D11Rhi& direct3D11Rhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexArray(direct3D11Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mIndexBuffer(indexBuffer),
			mNumberOfSlots(numberOfVertexBuffers),
			mD3D11Buffers(nullptr),
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
				const Rhi::Context& context = direct3D11Rhi.getContext();
				mD3D11Buffers = RHI_MALLOC_TYPED(context, ID3D11Buffer*, mNumberOfSlots);
				mStrides = RHI_MALLOC_TYPED(context, UINT, mNumberOfSlots);
				mOffsets = RHI_MALLOC_TYPED(context, UINT, mNumberOfSlots);
				memset(mOffsets, 0, sizeof(UINT) * mNumberOfSlots);	// Vertex buffer offset is not supported by OpenGL, so our RHI implementation doesn't support it either, set everything to zero
				mVertexBuffers = RHI_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					ID3D11Buffer** currentD3D11Buffer = mD3D11Buffers;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentD3D11Buffer, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used RHI?
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						*currentD3D11Buffer = (*currentVertexBuffer)->getD3D11Buffer();
						(*currentVertexBuffer)->addReference();
					}
				}

				{ // Gather slot related data
					const Rhi::VertexAttribute* attribute = vertexAttributes.attributes;
					const Rhi::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
					for (; attribute < attributesEnd; ++attribute)
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

			// Cleanup Direct3D 11 input slot data
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			const Rhi::Context& context = direct3D11Rhi.getContext();
			if (mNumberOfSlots > 0)
			{
				RHI_FREE(context, mD3D11Buffers);
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
			direct3D11Rhi.VertexArrayMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Set the Direct3D 11 vertex declaration and stream source
		*
		*  @param[in] d3d11DeviceContext
		*    The Direct3D 11 device context instance
		*/
		void setDirect3DIASetInputLayoutAndStreamSource(ID3D11DeviceContext& d3d11DeviceContext) const
		{
			// Set the Direct3D 11 vertex buffers
			if (nullptr != mD3D11Buffers)
			{
				{ // TODO(co) Work in progress: Compute shader writing into vertex buffer: "D3D11 WARNING: ID3D11DeviceContext::IASetVertexBuffers: Resource being set to Vertex Buffer slot 0 is still bound on output! Forcing to NULL. [ STATE_SETTING WARNING #1: DEVICE_IASETVERTEXBUFFERS_HAZARD]"
					ID3D11UnorderedAccessView* d3d11UnorderedAccessView = nullptr;
					d3d11DeviceContext.CSSetUnorderedAccessViews(2, 1, &d3d11UnorderedAccessView, nullptr);
				}

				// Just make a single API call
				d3d11DeviceContext.IASetVertexBuffers(0, mNumberOfSlots, mD3D11Buffers, mStrides, mOffsets);
			}
			else
			{
				// Direct3D 10 says: "D3D10: INFO: ID3D10Device::IASetVertexBuffers: Since NumBuffers is 0, the operation effectively does nothing. This is probably not intentional, nor is the most efficient way to achieve this operation. Avoid calling the routine at all. [ STATE_SETTING INFO #240: DEVICE_IASETVERTEXBUFFERS_BUFFERS_EMPTY ]"
				// -> Direct3D 11 does not give us this message, but it's probably still no good thing to do
			}

			// Set the used index buffer
			// -> In case of no index buffer we don't set null indices, there's not really a point in it
			if (nullptr != mIndexBuffer)
			{
				{ // TODO(co) Work in progress: Compute shader writing into vertex buffer: "D3D11 WARNING: ID3D11DeviceContext::IASetIndexBuffer: Resource being set to Index Buffer is still bound on output! Forcing to NULL. [ STATE_SETTING WARNING #2: DEVICE_IASETINDEXBUFFER_HAZARD]"
					ID3D11UnorderedAccessView* d3d11UnorderedAccessView = nullptr;
					d3d11DeviceContext.CSSetUnorderedAccessViews(1, 1, &d3d11UnorderedAccessView, nullptr);
				}

				// Set the Direct3D 11 indices
				d3d11DeviceContext.IASetIndexBuffer(mIndexBuffer->getD3D11Buffer(), static_cast<DXGI_FORMAT>(mIndexBuffer->getDXGIFormat()), 0);
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
		IndexBuffer*   mIndexBuffer;	///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Direct3D 11 input slots
		UINT		   mNumberOfSlots;	///< Number of used Direct3D 11 input slots
		ID3D11Buffer** mD3D11Buffers;	///< Direct3D 11 vertex buffers, if "mD3D11InputLayout" is no null pointer this is no null pointer as well
		UINT*		   mStrides;		///< Strides in bytes, if "mD3D11Buffers" is no null pointer this is no null pointer as well
		UINT*		   mOffsets;		///< Offsets in bytes, if "mD3D11Buffers" is no null pointer this is no null pointer as well
		// For proper vertex buffer reference counter behaviour
		VertexBuffer** mVertexBuffers;	///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/TextureBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 texture buffer object (TBO) class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		TextureBuffer(Direct3D11Rhi& direct3D11Rhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureBuffer(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11Buffer(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), (numberOfBytes % Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The Direct3D 11 texture buffer size must be a multiple of the selected texture format bytes per texel")

			{ // Buffer part
				// Direct3D 11 buffer description
				D3D11_BUFFER_DESC d3d11BufferDesc;
				d3d11BufferDesc.ByteWidth           = numberOfBytes;
				d3d11BufferDesc.Usage               = Mapping::getDirect3D11UsageAndCPUAccessFlags(bufferUsage, d3d11BufferDesc.CPUAccessFlags);
				d3d11BufferDesc.BindFlags           = 0;
				//d3d11BufferDesc.CPUAccessFlags    = <filled above>;
				d3d11BufferDesc.MiscFlags           = 0;
				d3d11BufferDesc.StructureByteStride = 0;

				// Set bind flags
				if (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE)
				{
					d3d11BufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				}
				if (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS)
				{
					d3d11BufferDesc.Usage			= D3D11_USAGE_DEFAULT;
					d3d11BufferDesc.BindFlags	   |= D3D11_BIND_UNORDERED_ACCESS;
					d3d11BufferDesc.CPUAccessFlags  = 0;
				}

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 11 subresource data
					D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
					d3d11SubresourceData.pSysMem          = data;
					d3d11SubresourceData.SysMemPitch      = 0;
					d3d11SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 11 constant buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mD3D11Buffer))
				}
				else
				{
					// Create the Direct3D 11 constant buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mD3D11Buffer))
				}
			}

			// Create the Direct3D 11 shader resource view instance
			if (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE)
			{
				// Direct3D 11 shader resource view description
				D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
				d3d11ShaderResourceViewDesc.Format				 = Mapping::getDirect3D11ResourceFormat(textureFormat);
				d3d11ShaderResourceViewDesc.ViewDimension		 = D3D11_SRV_DIMENSION_BUFFER;
				d3d11ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 11 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Buffer, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
			}

			// Create the Direct3D 11 unordered access view instance
			if (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS)
			{
				// Direct3D 11 unordered access view description
				D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
				d3d11UnorderedAccessViewDesc.Format				= Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
				d3d11UnorderedAccessViewDesc.ViewDimension		= D3D11_UAV_DIMENSION_BUFFER;
				d3d11UnorderedAccessViewDesc.Buffer.NumElements = numberOfBytes / Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 11 unordered access view instance
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Buffer, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6)	// 6 = "TBO: " including terminating zero
				if (nullptr != mD3D11Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureBuffer() override
		{
			// Release the used resources
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
				mD3D11ShaderResourceView = nullptr;
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
				mD3D11UnorderedAccessView = nullptr;
			}
			if (nullptr != mD3D11Buffer)
			{
				mD3D11Buffer->Release();
				mD3D11Buffer = nullptr;
			}
		}

		/**
		*  @brief
		*    Return the Direct3D texture buffer instance
		*
		*  @return
		*    The Direct3D texture buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Buffer* getD3D11Buffer() const
		{
			return mD3D11Buffer;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
		ID3D11Buffer*			   mD3D11Buffer;				///< Direct3D texture buffer instance, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/StructuredBuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 structured buffer object (SBO) class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Rhi::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBuffer(Direct3D11Rhi& direct3D11Rhi, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IStructuredBuffer(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11Buffer(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The Direct3D 11 structured buffer size must be a multiple of the given number of structure bytes")
			RHI_ASSERT(direct3D11Rhi.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The Direct3D 11 structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			{ // Buffer part
				// Direct3D 11 buffer description
				D3D11_BUFFER_DESC d3d11BufferDesc;
				d3d11BufferDesc.ByteWidth           = numberOfBytes;
				d3d11BufferDesc.Usage               = Mapping::getDirect3D11UsageAndCPUAccessFlags(bufferUsage, d3d11BufferDesc.CPUAccessFlags);
				d3d11BufferDesc.BindFlags           = 0;
				//d3d11BufferDesc.CPUAccessFlags    = <filled above>;
				d3d11BufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				d3d11BufferDesc.StructureByteStride = numberOfStructureBytes;

				// Set bind flags
				if (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE)
				{
					d3d11BufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				}
				if (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS)
				{
					d3d11BufferDesc.Usage			= D3D11_USAGE_DEFAULT;
					d3d11BufferDesc.BindFlags	   |= D3D11_BIND_UNORDERED_ACCESS;
					d3d11BufferDesc.CPUAccessFlags  = 0;
				}

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 11 subresource data
					D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
					d3d11SubresourceData.pSysMem          = data;
					d3d11SubresourceData.SysMemPitch      = 0;
					d3d11SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 11 constant buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mD3D11Buffer))
				}
				else
				{
					// Create the Direct3D 11 constant buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mD3D11Buffer))
				}
			}

			// Create the Direct3D 11 shader resource view instance
			if (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE)
			{
				// Direct3D 11 shader resource view description
				D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
				d3d11ShaderResourceViewDesc.Format				= DXGI_FORMAT_UNKNOWN;
				d3d11ShaderResourceViewDesc.ViewDimension		= D3D11_SRV_DIMENSION_BUFFER;
				d3d11ShaderResourceViewDesc.Buffer.ElementWidth	= numberOfBytes / numberOfStructureBytes;

				// Create the Direct3D 11 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Buffer, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
			}

			// Create the Direct3D 11 unordered access view instance
			if (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS)
			{
				// Direct3D 11 unordered access view description
				// -> There's no support for Direct3D 11 style "append" ("D3D11_BUFFER_UAV_FLAG_APPEND") or "counter" ("D3D11_BUFFER_UAV_FLAG_COUNTER") structured buffer by intent since neither OpenGL nor Vulkan support something similar, use atomic shader intrinsics instead
				D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
				d3d11UnorderedAccessViewDesc.Format				= DXGI_FORMAT_UNKNOWN;
				d3d11UnorderedAccessViewDesc.ViewDimension		= D3D11_UAV_DIMENSION_BUFFER;
				d3d11UnorderedAccessViewDesc.Buffer.NumElements	= numberOfBytes / numberOfStructureBytes;

				// Create the Direct3D 11 unordered access view instance
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Buffer, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "SBO", 6)	// 6 = "SBO: " including terminating zero
				if (nullptr != mD3D11Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~StructuredBuffer() override
		{
			// Release the used resources
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
				mD3D11ShaderResourceView = nullptr;
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
				mD3D11UnorderedAccessView = nullptr;
			}
			if (nullptr != mD3D11Buffer)
			{
				mD3D11Buffer->Release();
				mD3D11Buffer = nullptr;
			}
		}

		/**
		*  @brief
		*    Return the Direct3D structured buffer instance
		*
		*  @return
		*    The Direct3D structured buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Buffer* getD3D11Buffer() const
		{
			return mD3D11Buffer;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
		ID3D11Buffer*			   mD3D11Buffer;				///< Direct3D structured buffer instance, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/IndirectBuffer.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 indirect buffer object class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Rhi::IndirectBufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBuffer(Direct3D11Rhi& direct3D11Rhi, uint32_t numberOfBytes, const void* data, uint32_t indirectBufferFlags, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndirectBuffer(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11Buffer(nullptr),
			mStagingD3D11Buffer(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Direct3D 11 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RHI_ASSERT(direct3D11Rhi.getContext(), !((indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Direct3D 11 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RHI_ASSERT(direct3D11Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawArguments)) == 0, "Direct3D 11 indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RHI_ASSERT(direct3D11Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawIndexedArguments)) == 0, "Direct3D 11 indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			{ // Buffer part: Indirect buffers can't be mapped in Direct3D 11 since considered to be exclusively written by GPU
				// Direct3D 11 buffer description
				D3D11_BUFFER_DESC d3d11BufferDesc;
				d3d11BufferDesc.ByteWidth			= numberOfBytes;
				d3d11BufferDesc.Usage				= D3D11_USAGE_DEFAULT;
				d3d11BufferDesc.BindFlags			= 0;
				d3d11BufferDesc.CPUAccessFlags		= 0;
				d3d11BufferDesc.MiscFlags			= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
				d3d11BufferDesc.StructureByteStride	= 0;

				// Set bind flags
				// -> Using a structured indirect buffer would be handy inside shader source codes, sadly this isn't possible with Direct3D 11 and will result in the following error:
				//    "D3D11 ERROR: ID3D11Device::CreateBuffer: A resource cannot created with both D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS and D3D11_RESOURCE_MISC_BUFFER_STRUCTURED. [ STATE_CREATION ERROR #68: CREATEBUFFER_INVALIDMISCFLAGS]"
				//    d3d11BufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				//    d3d11BufferDesc.StructureByteStride = (flags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) ? sizeof(Rhi::DrawArguments) : sizeof(Rhi::DrawIndexedArguments);
				if (indirectBufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE)
				{
					d3d11BufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				}
				if (indirectBufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS)
				{
					d3d11BufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				}

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 11 subresource data
					D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
					d3d11SubresourceData.pSysMem          = data;
					d3d11SubresourceData.SysMemPitch      = 0;
					d3d11SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 11 indirect buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mD3D11Buffer))
				}
				else
				{
					// Create the Direct3D 11 indirect buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mD3D11Buffer))
				}
			}

			// Staging buffer part: Indirect buffers can't be mapped in Direct3D 11 since considered to be exclusively written by GPU, so we need an additional staging buffer to send dynamic data from CPU to GPU
			if (Rhi::BufferUsage::STATIC_DRAW != bufferUsage && Rhi::BufferUsage::STATIC_READ != bufferUsage && Rhi::BufferUsage::STATIC_COPY != bufferUsage)
			{
				// Direct3D 11 buffer description
				D3D11_BUFFER_DESC d3d11BufferDesc;
				d3d11BufferDesc.ByteWidth			= numberOfBytes;
				d3d11BufferDesc.Usage               = Mapping::getDirect3D11UsageAndCPUAccessFlags(bufferUsage, d3d11BufferDesc.CPUAccessFlags);
				d3d11BufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
				//d3d11BufferDesc.CPUAccessFlags    = <filled above>;
				d3d11BufferDesc.MiscFlags			= 0;
				d3d11BufferDesc.StructureByteStride	= 0;

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 11 subresource data
					D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
					d3d11SubresourceData.pSysMem          = data;
					d3d11SubresourceData.SysMemPitch      = 0;
					d3d11SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 11 indirect buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mStagingD3D11Buffer))
				}
				else
				{
					// Create the Direct3D 11 indirect buffer
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mStagingD3D11Buffer))
				}
			}

			// Create the Direct3D 11 shader resource view instance
			if (indirectBufferFlags & Rhi::IndirectBufferFlag::SHADER_RESOURCE)
			{
				// Direct3D 11 shader resource view description
				D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
				d3d11ShaderResourceViewDesc.Format				 = DXGI_FORMAT_R32_UINT;
				d3d11ShaderResourceViewDesc.ViewDimension		 = D3D11_SRV_DIMENSION_BUFFER;
				d3d11ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / sizeof(uint32_t);

				// Create the Direct3D 11 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Buffer, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
			}

			// Create the Direct3D 11 unordered access view instance
			if (indirectBufferFlags & Rhi::IndirectBufferFlag::UNORDERED_ACCESS)
			{
				// Direct3D 11 unordered access view description
				D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
				d3d11UnorderedAccessViewDesc.Format				= DXGI_FORMAT_R32_UINT;
				d3d11UnorderedAccessViewDesc.ViewDimension		= D3D11_UAV_DIMENSION_BUFFER;
				d3d11UnorderedAccessViewDesc.Buffer.NumElements = numberOfBytes / sizeof(uint32_t);

				// Create the Direct3D 11 unordered access view instance
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Buffer, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IndirectBufferObject", 23)	// 23 = "IndirectBufferObject: " including terminating zero
				if (nullptr != mD3D11Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mStagingD3D11Buffer)
				{
					FAILED_DEBUG_BREAK(mStagingD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~IndirectBuffer() override
		{
			// Release the used resources
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
				mD3D11ShaderResourceView = nullptr;
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
				mD3D11UnorderedAccessView = nullptr;
			}
			if (nullptr != mD3D11Buffer)
			{
				mD3D11Buffer->Release();
				mD3D11Buffer = nullptr;
			}
			if (nullptr != mStagingD3D11Buffer)
			{
				mStagingD3D11Buffer->Release();
				mStagingD3D11Buffer = nullptr;
			}
		}

		/**
		*  @brief
		*    Return the Direct3D indirect buffer instance
		*
		*  @return
		*    The Direct3D indirect buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Buffer* getD3D11Buffer() const
		{
			return mD3D11Buffer;
		}

		[[nodiscard]] inline ID3D11Buffer* getStagingD3D11Buffer() const
		{
			return mStagingD3D11Buffer;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
	//[ Public virtual Rhi::IIndirectBuffer methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const uint8_t* getEmulationData() const override
		{
			return nullptr;
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
		ID3D11Buffer*			   mD3D11Buffer;				///< Direct3D indirect buffer instance, can be a null pointer
		ID3D11Buffer*			   mStagingD3D11Buffer;			///< Staging Direct3D indirect buffer instance, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/UniformBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(Direct3D11Rhi& direct3D11Rhi, uint32_t numberOfBytes, const void* data, Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Rhi::IUniformBuffer(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11Buffer(nullptr)
		{
			{ // Sanity check
				// Check the given number of bytes, if we don't do this we might get told
				//   "... the ByteWidth (value = <x>) must be a multiple of 16 and be less than or equal to 65536"
				// by Direct3D 11
				const uint32_t leftOverBytes = (numberOfBytes % 16);
				if (0 != leftOverBytes)
				{
					// Fix the byte alignment, no assert because other RHI implementations have another alignment (DirectX 12 e.g. 256)
					numberOfBytes += 16 - (numberOfBytes % 16);
				}
			}

			// Direct3D 11 buffer description
			D3D11_BUFFER_DESC d3d11BufferDesc;
			d3d11BufferDesc.ByteWidth           = numberOfBytes;
			d3d11BufferDesc.Usage               = Mapping::getDirect3D11UsageAndCPUAccessFlags(bufferUsage, d3d11BufferDesc.CPUAccessFlags);
			d3d11BufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
			//d3d11BufferDescd3d11BufferDesc.CPUAccessFlags    = <filled above>;
			d3d11BufferDesc.MiscFlags           = 0;
			d3d11BufferDesc.StructureByteStride = 0;

			// Data given?
			if (nullptr != data)
			{
				// Direct3D 11 subresource data
				D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
				d3d11SubresourceData.pSysMem          = data;
				d3d11SubresourceData.SysMemPitch      = 0;
				d3d11SubresourceData.SysMemSlicePitch = 0;

				// Create the Direct3D 11 constant buffer
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &mD3D11Buffer))
			}
			else
			{
				// Create the Direct3D 11 constant buffer
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBuffer(&d3d11BufferDesc, nullptr, &mD3D11Buffer))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11Buffer)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "UBO", 6)	// 6 = "UBO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~UniformBuffer() override
		{
			// Release the Direct3D 11 constant buffer
			if (nullptr != mD3D11Buffer)
			{
				mD3D11Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 constant buffer instance
		*
		*  @return
		*    The Direct3D 11 constant buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Buffer* getD3D11Buffer() const
		{
			return mD3D11Buffer;
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
		ID3D11Buffer* mD3D11Buffer;	///< Direct3D 11 constant buffer instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Buffer/BufferManager.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 buffer manager interface
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*/
		inline explicit BufferManager(Direct3D11Rhi& direct3D11Rhi) :
			IBufferManager(direct3D11Rhi)
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
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), VertexBuffer)(direct3D11Rhi, numberOfBytes, data, bufferFlags, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::IndexBufferFormat::Enum indexBufferFormat = Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), IndexBuffer)(direct3D11Rhi, numberOfBytes, data, bufferFlags, bufferUsage, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexArray* createVertexArray(const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, Rhi::IIndexBuffer* indexBuffer = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity checks
			#ifdef RHI_DEBUG
			{
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RHI_ASSERT(direct3D11Rhi.getContext(), &direct3D11Rhi == &vertexBuffer->vertexBuffer->getRhi(), "Direct3D 11 error: The given vertex buffer resource is owned by another RHI instance")
				}
			}
			#endif
			RHI_ASSERT(direct3D11Rhi.getContext(), nullptr == indexBuffer || &direct3D11Rhi == &indexBuffer->getRhi(), "Direct3D 11 error: The given index buffer resource is owned by another RHI instance")

			// Create vertex array
			uint16_t id = 0;
			if (direct3D11Rhi.VertexArrayMakeId.CreateID(id))
			{
				return RHI_NEW(direct3D11Rhi.getContext(), VertexArray)(direct3D11Rhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), TextureBuffer)(direct3D11Rhi, numberOfBytes, data, bufferFlags, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), StructuredBuffer)(direct3D11Rhi, numberOfBytes, data, bufferFlags, bufferUsage, numberOfStructureBytes RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), IndirectBuffer)(direct3D11Rhi, numberOfBytes, data, indirectBufferFlags, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// -> "Bind a buffer as a constant buffer to a shader stage; this flag may NOT be combined with any other bind flag." - https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ne-d3d11-d3d11_bind_flag
			// RHI_ASSERT(direct3D11Rhi.getContext(), (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid Direct3D 11 buffer flags, uniform buffer can't be used for unordered access")
			// RHI_ASSERT(direct3D11Rhi.getContext(), (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0, "Invalid Direct3D 11 buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RHI_NEW(direct3D11Rhi.getContext(), UniformBuffer)(direct3D11Rhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
	//[ Direct3D11Rhi/Texture/Texture1D.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 1D texture class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture1D(Direct3D11Rhi& direct3D11Rhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1D(direct3D11Rhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mD3D11Texture1D(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 11 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 11 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;
			const bool isDepthFormat = Rhi::TextureFormat::isDepth(textureFormat);

			// Direct3D 11 1D texture description
			D3D11_TEXTURE1D_DESC d3d11Texture1DDesc;
			d3d11Texture1DDesc.Width		  = width;
			d3d11Texture1DDesc.MipLevels	  = numberOfMipmaps;
			d3d11Texture1DDesc.ArraySize	  = 1;
			d3d11Texture1DDesc.Format		  = Mapping::getDirect3D11ResourceFormat(textureFormat);
			d3d11Texture1DDesc.Usage		  = static_cast<D3D11_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d11Texture1DDesc.BindFlags	  = 0;
			d3d11Texture1DDesc.CPUAccessFlags = (Rhi::TextureUsage::DYNAMIC == textureUsage) ? D3D11_CPU_ACCESS_WRITE : 0u;
			d3d11Texture1DDesc.MiscFlags	  = (generateMipmaps && !isDepthFormat) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				d3d11Texture1DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if ((textureFlags & Rhi::TextureFlag::RENDER_TARGET) || generateMipmaps)
			{
				if (isDepthFormat)
				{
					d3d11Texture1DDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d11Texture1DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				d3d11Texture1DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}

			// Create the Direct3D 11 1D texture instance: Did the user provided us with any texture data?
			if (nullptr != data)
			{
				// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
				static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
				RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 11 number of mipmaps")
				D3D11_SUBRESOURCE_DATA d3d11SubresourceData[MAXIMUM_NUMBER_OF_MIPMAPS];

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[mipmap];
						currentD3d11SubresourceData.pSysMem			 = data;
						currentD3d11SubresourceData.SysMemPitch		 = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

						// Move on to the next mipmap and ensure the size is always at least 1
						// -> If the data doesn't contain mipmaps, we don't need to care about this in here
						data = static_cast<const uint8_t*>(data) + currentD3d11SubresourceData.SysMemPitch;
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					// -> When uploading data, we still need to upload the whole mipmap chain, so provide dummy data
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[mipmap];
						currentD3d11SubresourceData.pSysMem			 = data;
						currentD3d11SubresourceData.SysMemPitch		 = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture1D(&d3d11Texture1DDesc, d3d11SubresourceData, &mD3D11Texture1D))
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture1D(&d3d11Texture1DDesc, nullptr, &mD3D11Texture1D))
			}

			// Create requested views
			if (nullptr != mD3D11Texture1D)
			{
				// Create the Direct3D 11 shader resource view instance
				if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format				= Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11ShaderResourceViewDesc.ViewDimension		= D3D11_SRV_DIMENSION_TEXTURE1D;
					d3d11ShaderResourceViewDesc.Texture1D.MipLevels = numberOfMipmaps;

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Texture1D, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format		   = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11UnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Texture1D, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}
			}

			// Let Direct3D 11 generate the mipmaps for us automatically, if necessary
			if (nullptr != data && generateMipmaps)
			{
				direct3D11Rhi.generateAsynchronousDeferredMipmaps(*this, *mD3D11ShaderResourceView);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture", 13)	// 13 = "1D texture: " including terminating zero
				if (nullptr != mD3D11Texture1D)
				{
					FAILED_DEBUG_BREAK(mD3D11Texture1D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture1D() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
			}
			if (nullptr != mD3D11Texture1D)
			{
				mD3D11Texture1D->Release();
			}
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
		*    Return the Direct3D texture 1D resource instance
		*
		*  @return
		*    The Direct3D texture 1D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Texture1D* getD3D11Texture1D() const
		{
			return mD3D11Texture1D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D11Texture1D;
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
		Rhi::TextureFormat::Enum   mTextureFormat;
		ID3D11Texture1D*		   mD3D11Texture1D;				///< Direct3D 11 texture 1D resource, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Texture/Texture1DArray.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 1D array texture class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture1DArray(Direct3D11Rhi& direct3D11Rhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1DArray(direct3D11Rhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mD3D11Texture1D(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 11 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 11 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;
			const bool isDepthFormat = Rhi::TextureFormat::isDepth(textureFormat);

			// Direct3D 11 1D array texture description
			D3D11_TEXTURE1D_DESC d3d11Texture1DDesc;
			d3d11Texture1DDesc.Width		  = width;
			d3d11Texture1DDesc.MipLevels	  = numberOfMipmaps;
			d3d11Texture1DDesc.ArraySize	  = numberOfSlices;
			d3d11Texture1DDesc.Format		  = Mapping::getDirect3D11ResourceFormat(textureFormat);
			d3d11Texture1DDesc.Usage		  = static_cast<D3D11_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d11Texture1DDesc.BindFlags	  = 0;
			d3d11Texture1DDesc.CPUAccessFlags = (Rhi::TextureUsage::DYNAMIC == textureUsage) ? D3D11_CPU_ACCESS_WRITE : 0u;
			d3d11Texture1DDesc.MiscFlags	  = (generateMipmaps && !isDepthFormat) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				d3d11Texture1DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if ((textureFlags & Rhi::TextureFlag::RENDER_TARGET) || generateMipmaps)
			{
				if (isDepthFormat)
				{
					d3d11Texture1DDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d11Texture1DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				d3d11Texture1DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}

			// Create the Direct3D 11 1D texture instance: Did the user provided us with any texture data?
			if (nullptr != data)
			{
				// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
				static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
				static constexpr uint32_t MAXIMUM_NUMBER_OF_SLICES = 10;
				const Rhi::Context& context = direct3D11Rhi.getContext();
				RHI_ASSERT(context, numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 11 number of mipmaps")
				D3D11_SUBRESOURCE_DATA d3d11SubresourceDataStack[MAXIMUM_NUMBER_OF_SLICES * MAXIMUM_NUMBER_OF_MIPMAPS];
				D3D11_SUBRESOURCE_DATA* d3d11SubresourceData = (numberOfSlices <= MAXIMUM_NUMBER_OF_SLICES) ? d3d11SubresourceDataStack : RHI_MALLOC_TYPED(context, D3D11_SUBRESOURCE_DATA, numberOfSlices);

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout
					// - Direct3D 11 wants: DDS files are organized in slice-major order, like this:
					//     Slice0: Mip0, Mip1, Mip2, etc.
					//     Slice1: Mip0, Mip1, Mip2, etc.
					//     etc.
					// - The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//     Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     etc.

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const uint32_t numberOfBytesPerRow = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							// Upload the current slice
							D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[arraySlice * numberOfMipmaps + mipmap];
							currentD3d11SubresourceData.pSysMem			 = data;
							currentD3d11SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
							currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							// -> If the data doesn't contain mipmaps, we don't need to care about this in here
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					// -> When uploading data, we still need to upload the whole mipmap chain, so provide dummy data
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const void* currentData = data;
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[arraySlice * numberOfMipmaps + mipmap];
							currentD3d11SubresourceData.pSysMem			 = currentData;
							currentD3d11SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
							currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							currentData = static_cast<const uint8_t*>(currentData) + numberOfBytesPerSlice;
						}
					}
				}
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture1D(&d3d11Texture1DDesc, d3d11SubresourceData, &mD3D11Texture1D))
				if (numberOfSlices > MAXIMUM_NUMBER_OF_SLICES)
				{
					RHI_FREE(context, d3d11SubresourceData);
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture1D(&d3d11Texture1DDesc, nullptr, &mD3D11Texture1D))
			}

			// Create requested views
			if (nullptr != mD3D11Texture1D)
			{
				// Create the Direct3D 11 shader resource view instance
				if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format					 = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11ShaderResourceViewDesc.ViewDimension			 = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
					d3d11ShaderResourceViewDesc.Texture1DArray.MipLevels = numberOfMipmaps;
					d3d11ShaderResourceViewDesc.Texture1DArray.ArraySize = numberOfSlices;

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Texture1D, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format					  = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11UnorderedAccessViewDesc.ViewDimension			  = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
					d3d11UnorderedAccessViewDesc.Texture1DArray.ArraySize = numberOfSlices;

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Texture1D, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}
			}

			// Let Direct3D 11 generate the mipmaps for us automatically, if necessary
			if (nullptr != data && generateMipmaps)
			{
				direct3D11Rhi.generateAsynchronousDeferredMipmaps(*this, *mD3D11ShaderResourceView);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture array", 19)	// 19 = "1D texture array: " including terminating zero
				if (nullptr != mD3D11Texture1D)
				{
					FAILED_DEBUG_BREAK(mD3D11Texture1D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture1DArray() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
			}
			if (nullptr != mD3D11Texture1D)
			{
				mD3D11Texture1D->Release();
			}
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
		*    Return the Direct3D texture 1D resource instance
		*
		*  @return
		*    The Direct3D texture 1D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Texture1D* getD3D11Texture1D() const
		{
			return mD3D11Texture1D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
		Rhi::TextureFormat::Enum   mTextureFormat;
		ID3D11Texture1D*		   mD3D11Texture1D;				///< Direct3D 11 texture 1D resource, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Texture/Texture2D.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 2D texture class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 6, 8)
		*/
		Texture2D(Direct3D11Rhi& direct3D11Rhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2D(direct3D11Rhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mNumberOfMultisamples(numberOfMultisamples),
			mD3D11Texture2D(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS), "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Rhi::TextureFlag::RENDER_TARGET), "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 11 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 11 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;
			const bool isDepthFormat = Rhi::TextureFormat::isDepth(textureFormat);

			// Direct3D 11 2D texture description
			D3D11_TEXTURE2D_DESC d3d11Texture2DDesc;
			d3d11Texture2DDesc.Width			  = width;
			d3d11Texture2DDesc.Height			  = height;
			d3d11Texture2DDesc.MipLevels		  = numberOfMipmaps;
			d3d11Texture2DDesc.ArraySize		  = 1;
			d3d11Texture2DDesc.Format			  = Mapping::getDirect3D11ResourceFormat(textureFormat);
			d3d11Texture2DDesc.SampleDesc.Count	  = numberOfMultisamples;
			d3d11Texture2DDesc.SampleDesc.Quality = 0;
			d3d11Texture2DDesc.Usage			  = static_cast<D3D11_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d11Texture2DDesc.BindFlags		  = 0;
			d3d11Texture2DDesc.CPUAccessFlags	  = (Rhi::TextureUsage::DYNAMIC == textureUsage) ? D3D11_CPU_ACCESS_WRITE : 0u;
			d3d11Texture2DDesc.MiscFlags		  = (generateMipmaps && !isDepthFormat) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if ((textureFlags & Rhi::TextureFlag::RENDER_TARGET) || generateMipmaps)
			{
				if (isDepthFormat)
				{
					d3d11Texture2DDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d11Texture2DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}

			// Create the Direct3D 11 2D texture instance: Did the user provided us with any texture data?
			if (nullptr != data)
			{
				// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
				static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
				RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 11 number of mipmaps")
				D3D11_SUBRESOURCE_DATA d3d11SubresourceData[MAXIMUM_NUMBER_OF_MIPMAPS];

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[mipmap];
						currentD3d11SubresourceData.pSysMem			 = data;
						currentD3d11SubresourceData.SysMemPitch		 = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

						// Move on to the next mipmap and ensure the size is always at least 1x1
						// -> If the data doesn't contain mipmaps, we don't need to care about this in here
						data = static_cast<const uint8_t*>(data) + Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					// -> When uploading data, we still need to upload the whole mipmap chain, so provide dummy data
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[mipmap];
						currentD3d11SubresourceData.pSysMem			 = data;
						currentD3d11SubresourceData.SysMemPitch		 = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture2D(&d3d11Texture2DDesc, d3d11SubresourceData, &mD3D11Texture2D))
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture2D(&d3d11Texture2DDesc, nullptr, &mD3D11Texture2D))
			}

			// Create requested views
			if (nullptr != mD3D11Texture2D)
			{
				// Create the Direct3D 11 shader resource view instance
				if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format				= Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11ShaderResourceViewDesc.ViewDimension		= (numberOfMultisamples > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
					d3d11ShaderResourceViewDesc.Texture2D.MipLevels	= numberOfMipmaps;

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Texture2D, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format		   = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11UnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Texture2D, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}
			}

			// Let Direct3D 11 generate the mipmaps for us automatically, if necessary
			if (nullptr != data && generateMipmaps)
			{
				direct3D11Rhi.generateAsynchronousDeferredMipmaps(*this, *mD3D11ShaderResourceView);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture", 13)	// 13 = "2D texture: " including terminating zero
				if (nullptr != mD3D11Texture2D)
				{
					FAILED_DEBUG_BREAK(mD3D11Texture2D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2D() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
			}
			if (nullptr != mD3D11Texture2D)
			{
				mD3D11Texture2D->Release();
			}
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
		*    Return the Direct3D texture 2D resource instance
		*
		*  @return
		*    The Direct3D texture 2D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Texture2D* getD3D11Texture2D() const
		{
			return mD3D11Texture2D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
			// Re-create the Direct3D 11 shader resource view instance
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11Texture2D)
			{
				// Direct3D 11 shader resource view description
				D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
				d3d11ShaderResourceViewDesc.Format					  = Mapping::getDirect3D11ShaderResourceViewFormat(mTextureFormat);
				d3d11ShaderResourceViewDesc.ViewDimension			  = (mNumberOfMultisamples > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
				d3d11ShaderResourceViewDesc.Texture2D.MipLevels		  = maximumMipmapIndex - minimumMipmapIndex + 1;
				d3d11ShaderResourceViewDesc.Texture2D.MostDetailedMip = minimumMipmapIndex;

				// Create the Direct3D 11 shader resource view instance
				FAILED_DEBUG_BREAK(static_cast<Direct3D11Rhi&>(getRhi()).getD3D11Device()->CreateShaderResourceView(mD3D11Texture2D, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
			}
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D11Texture2D;
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
		Rhi::TextureFormat::Enum   mTextureFormat;
		uint8_t					   mNumberOfMultisamples;
		ID3D11Texture2D*		   mD3D11Texture2D;				///< Direct3D 11 texture 2D resource, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Texture/Texture2DArray.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 2D array texture class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture2DArray(Direct3D11Rhi& direct3D11Rhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2DArray(direct3D11Rhi, width, height, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mNumberOfMultisamples(1),	// TODO(co) Currently no MSAA support for 2D array textures
			mD3D11Texture2D(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 11 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 11 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;
			const bool isDepthFormat = Rhi::TextureFormat::isDepth(textureFormat);

			// Direct3D 11 2D array texture description
			D3D11_TEXTURE2D_DESC d3d11Texture2DDesc;
			d3d11Texture2DDesc.Width			  = width;
			d3d11Texture2DDesc.Height			  = height;
			d3d11Texture2DDesc.MipLevels		  = numberOfMipmaps;
			d3d11Texture2DDesc.ArraySize		  = numberOfSlices;
			d3d11Texture2DDesc.Format			  = Mapping::getDirect3D11ResourceFormat(textureFormat);
			d3d11Texture2DDesc.SampleDesc.Count	  = 1;
			d3d11Texture2DDesc.SampleDesc.Quality = 0;
			d3d11Texture2DDesc.Usage			  = static_cast<D3D11_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d11Texture2DDesc.BindFlags		  = 0;
			d3d11Texture2DDesc.CPUAccessFlags	  = (Rhi::TextureUsage::DYNAMIC == textureUsage) ? D3D11_CPU_ACCESS_WRITE : 0u;
			d3d11Texture2DDesc.MiscFlags		  = (generateMipmaps && !isDepthFormat) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if ((textureFlags & Rhi::TextureFlag::RENDER_TARGET) || generateMipmaps)
			{
				if (isDepthFormat)
				{
					d3d11Texture2DDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d11Texture2DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}

			// Create the Direct3D 11 2D texture instance: Did the user provided us with any texture data?
			if (nullptr != data)
			{
				// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
				static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
				static constexpr uint32_t MAXIMUM_NUMBER_OF_SLICES = 10;
				const Rhi::Context& context = direct3D11Rhi.getContext();
				RHI_ASSERT(context, numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 11 number of mipmaps")
				D3D11_SUBRESOURCE_DATA d3d11SubresourceDataStack[MAXIMUM_NUMBER_OF_SLICES * MAXIMUM_NUMBER_OF_MIPMAPS];
				D3D11_SUBRESOURCE_DATA* d3d11SubresourceData = (numberOfSlices <= MAXIMUM_NUMBER_OF_SLICES) ? d3d11SubresourceDataStack : RHI_MALLOC_TYPED(context, D3D11_SUBRESOURCE_DATA, numberOfSlices);

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout
					// - Direct3D 11 wants: DDS files are organized in slice-major order, like this:
					//     Slice0: Mip0, Mip1, Mip2, etc.
					//     Slice1: Mip0, Mip1, Mip2, etc.
					//     etc.
					// - The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//     Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     etc.

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const uint32_t numberOfBytesPerRow = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							// Upload the current slice
							D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[arraySlice * numberOfMipmaps + mipmap];
							currentD3d11SubresourceData.pSysMem			 = data;
							currentD3d11SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
							currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							// -> If the data doesn't contain mipmaps, we don't need to care about this in here
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
					// -> When uploading data, we still need to upload the whole mipmap chain, so provide dummy data
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const void* currentData = data;
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[arraySlice * numberOfMipmaps + mipmap];
							currentD3d11SubresourceData.pSysMem			 = currentData;
							currentD3d11SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
							currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							currentData = static_cast<const uint8_t*>(currentData) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture2D(&d3d11Texture2DDesc, d3d11SubresourceData, &mD3D11Texture2D))
				if (numberOfSlices > MAXIMUM_NUMBER_OF_SLICES)
				{
					RHI_FREE(context, d3d11SubresourceData);
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture2D(&d3d11Texture2DDesc, nullptr, &mD3D11Texture2D))
			}

			// Create requested views
			if (nullptr != mD3D11Texture2D)
			{
				// Create the Direct3D 11 shader resource view instance
				if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format					 = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11ShaderResourceViewDesc.ViewDimension			 = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
					d3d11ShaderResourceViewDesc.Texture2DArray.MipLevels = numberOfMipmaps;
					d3d11ShaderResourceViewDesc.Texture2DArray.ArraySize = numberOfSlices;

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Texture2D, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format					  = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11UnorderedAccessViewDesc.ViewDimension			  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
					d3d11UnorderedAccessViewDesc.Texture2DArray.ArraySize = numberOfSlices;

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Texture2D, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}
			}

			// Let Direct3D 11 generate the mipmaps for us automatically, if necessary
			if (nullptr != data && generateMipmaps)
			{
				direct3D11Rhi.generateAsynchronousDeferredMipmaps(*this, *mD3D11ShaderResourceView);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture array", 19)	// 19 = "2D texture array: " including terminating zero
				if (nullptr != mD3D11Texture2D)
				{
					FAILED_DEBUG_BREAK(mD3D11Texture2D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2DArray() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
			}
			if (nullptr != mD3D11Texture2D)
			{
				mD3D11Texture2D->Release();
			}
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
		*    Return the Direct3D texture 2D resource instance
		*
		*  @return
		*    The Direct3D texture 2D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Texture2D* getD3D11Texture2D() const
		{
			return mD3D11Texture2D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
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
		Rhi::TextureFormat::Enum   mTextureFormat;
		uint8_t					   mNumberOfMultisamples;
		ID3D11Texture2D*		   mD3D11Texture2D;				///< Direct3D 11 texture 2D resource, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Texture/Texture3D.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 3D texture class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		Texture3D(Direct3D11Rhi& direct3D11Rhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture3D(direct3D11Rhi, width, height, depth RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mD3D11Texture3D(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 11 texture parameters")
			RHI_ASSERT(direct3D11Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 11 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 11 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;
			const bool isDepthFormat = Rhi::TextureFormat::isDepth(textureFormat);

			// Direct3D 11 3D texture description
			D3D11_TEXTURE3D_DESC d3d11Texture3DDesc;
			d3d11Texture3DDesc.Width		  = width;
			d3d11Texture3DDesc.Height		  = height;
			d3d11Texture3DDesc.Depth		  = depth;
			d3d11Texture3DDesc.MipLevels	  = numberOfMipmaps;
			d3d11Texture3DDesc.Format		  = Mapping::getDirect3D11ResourceFormat(textureFormat);
			d3d11Texture3DDesc.Usage		  = static_cast<D3D11_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d11Texture3DDesc.BindFlags	  = 0;
			d3d11Texture3DDesc.CPUAccessFlags = (Rhi::TextureUsage::DYNAMIC == textureUsage) ? D3D11_CPU_ACCESS_WRITE : 0u;
			d3d11Texture3DDesc.MiscFlags	  = (generateMipmaps && !isDepthFormat) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				d3d11Texture3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if ((textureFlags & Rhi::TextureFlag::RENDER_TARGET) || generateMipmaps)
			{
				if (isDepthFormat)
				{
					d3d11Texture3DDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d11Texture3DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				}
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				d3d11Texture3DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}

			// Create the Direct3D 11 3D texture instance: Did the user provided us with any texture data?
			if (nullptr != data)
			{
				// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
				static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
				RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 11 number of mipmaps")
				D3D11_SUBRESOURCE_DATA d3d11SubresourceData[MAXIMUM_NUMBER_OF_MIPMAPS];

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
						D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[mipmap];
						currentD3d11SubresourceData.pSysMem			 = data;
						currentD3d11SubresourceData.SysMemPitch		 = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						currentD3d11SubresourceData.SysMemSlicePitch = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);

						// Move on to the next mipmap and ensure the size is always at least 1x1x1
						// -> If the data doesn't contain mipmaps, we don't need to care about this in here
						data = static_cast<const uint8_t*>(data) + currentD3d11SubresourceData.SysMemSlicePitch * depth;
						width = getHalfSize(width);
						height = getHalfSize(height);
						depth = getHalfSize(depth);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps
					// -> When uploading data, we still need to upload the whole mipmap chain, so provide dummy data
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[mipmap];
						currentD3d11SubresourceData.pSysMem			 = data;
						currentD3d11SubresourceData.SysMemPitch		 = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						currentD3d11SubresourceData.SysMemSlicePitch = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture3D(&d3d11Texture3DDesc, d3d11SubresourceData, &mD3D11Texture3D))
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture3D(&d3d11Texture3DDesc, nullptr, &mD3D11Texture3D))
			}

			// Create requested views
			if (nullptr != mD3D11Texture3D)
			{
				// Create the Direct3D 11 shader resource view instance
				if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format				= Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11ShaderResourceViewDesc.ViewDimension		= D3D11_SRV_DIMENSION_TEXTURE3D;
					d3d11ShaderResourceViewDesc.Texture3D.MipLevels	= numberOfMipmaps;

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11Texture3D, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format			 = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11UnorderedAccessViewDesc.ViewDimension	 = D3D11_UAV_DIMENSION_TEXTURE3D;
					d3d11UnorderedAccessViewDesc.Texture3D.WSize = depth;

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11Texture3D, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}
			}

			// Let Direct3D 11 generate the mipmaps for us automatically, if necessary
			if (nullptr != data && generateMipmaps)
			{
				direct3D11Rhi.generateAsynchronousDeferredMipmaps(*this, *mD3D11ShaderResourceView);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "3D texture", 13)	// 13 = "3D texture: " including terminating zero
				if (nullptr != mD3D11Texture3D)
				{
					FAILED_DEBUG_BREAK(mD3D11Texture3D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture3D() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
			}
			if (nullptr != mD3D11Texture3D)
			{
				mD3D11Texture3D->Release();
			}
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
		*    Return the Direct3D texture 3D resource instance
		*
		*  @return
		*    The Direct3D texture 3D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Texture3D* getD3D11Texture3D() const
		{
			return mD3D11Texture3D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D11Texture3D;
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
		Rhi::TextureFormat::Enum   mTextureFormat;
		ID3D11Texture3D*		   mD3D11Texture3D;				///< Direct3D 11 texture 3D resource, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Texture/TextureCube.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 cube texture class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		TextureCube(Direct3D11Rhi& direct3D11Rhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Rhi::TextureUsage textureUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureCube(direct3D11Rhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mTextureFormat(textureFormat),
			mD3D11TextureCube(nullptr),
			mD3D11ShaderResourceView(nullptr),
			mD3D11UnorderedAccessView(nullptr)
		{
			static constexpr uint32_t NUMBER_OF_SLICES = 6;	// In Direct3D 11, a cube map is a 2D array texture with six slices

			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 11 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 11 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Direct3D 11 2D array texture description
			D3D11_TEXTURE2D_DESC d3d11Texture2DDesc;
			d3d11Texture2DDesc.Width			  = width;
			d3d11Texture2DDesc.Height			  = height;
			d3d11Texture2DDesc.MipLevels		  = numberOfMipmaps;
			d3d11Texture2DDesc.ArraySize		  = NUMBER_OF_SLICES;
			d3d11Texture2DDesc.Format			  = Mapping::getDirect3D11ResourceFormat(textureFormat);
			d3d11Texture2DDesc.SampleDesc.Count	  = 1;
			d3d11Texture2DDesc.SampleDesc.Quality = 0;
			d3d11Texture2DDesc.Usage			  = static_cast<D3D11_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d11Texture2DDesc.BindFlags		  = 0;
			d3d11Texture2DDesc.CPUAccessFlags	  = (Rhi::TextureUsage::DYNAMIC == textureUsage) ? D3D11_CPU_ACCESS_WRITE : 0u;
			d3d11Texture2DDesc.MiscFlags		  = (generateMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0u) | D3D11_RESOURCE_MISC_TEXTURECUBE;

			// Set bind flags
			if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if ((textureFlags & Rhi::TextureFlag::RENDER_TARGET) || generateMipmaps)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			}
			if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
			{
				d3d11Texture2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}

			// Create the Direct3D 11 2D texture instance: Did the user provided us with any texture data?
			if (nullptr != data)
			{
				// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
				static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
				RHI_ASSERT(direct3D11Rhi.getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 11 number of mipmaps")
				D3D11_SUBRESOURCE_DATA d3d11SubresourceData[NUMBER_OF_SLICES * MAXIMUM_NUMBER_OF_MIPMAPS];

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (dataContainsMipmaps)
				{
					// Data layout
					// - Direct3D 11 wants: DDS files are organized in face-major order, like this:
					//     Face0: Mip0, Mip1, Mip2, etc.
					//     Face1: Mip0, Mip1, Mip2, etc.
					//     etc.
					// - The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//     Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//     Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//     etc.

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const uint32_t numberOfBytesPerRow = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < NUMBER_OF_SLICES; ++arraySlice)
						{
							// Upload the current mipmap
							D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[arraySlice * numberOfMipmaps + mipmap];
							currentD3d11SubresourceData.pSysMem			 = data;
							currentD3d11SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
							currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the cube map face
							// -> If the data doesn't contain mipmaps, we don't need to care about this in here
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
					// -> When uploading data, we still need to upload the whole mipmap chain, so provide dummy data
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						const void* currentData = data;
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < NUMBER_OF_SLICES; ++arraySlice)
						{
							D3D11_SUBRESOURCE_DATA& currentD3d11SubresourceData = d3d11SubresourceData[arraySlice];
							currentD3d11SubresourceData.pSysMem			 = currentData;
							currentD3d11SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
							currentD3d11SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							currentData = static_cast<const uint8_t*>(currentData) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture2D(&d3d11Texture2DDesc, d3d11SubresourceData, &mD3D11TextureCube))
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateTexture2D(&d3d11Texture2DDesc, nullptr, &mD3D11TextureCube))
			}

			// Create requested views
			if (nullptr != mD3D11TextureCube)
			{
				// Create the Direct3D 11 shader resource view instance
				if (textureFlags & Rhi::TextureFlag::SHADER_RESOURCE)
				{
					// Direct3D 11 shader resource view description
					D3D11_SHADER_RESOURCE_VIEW_DESC d3d11ShaderResourceViewDesc = {};
					d3d11ShaderResourceViewDesc.Format						= Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11ShaderResourceViewDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURECUBE;
					d3d11ShaderResourceViewDesc.TextureCube.MipLevels		= numberOfMipmaps;
					d3d11ShaderResourceViewDesc.TextureCube.MostDetailedMip	= 0;

					// Create the Direct3D 11 shader resource view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateShaderResourceView(mD3D11TextureCube, &d3d11ShaderResourceViewDesc, &mD3D11ShaderResourceView))
				}

				// Create the Direct3D 11 unordered access view instance
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					// Direct3D 11 unordered access view description
					D3D11_UNORDERED_ACCESS_VIEW_DESC d3d11UnorderedAccessViewDesc = {};
					d3d11UnorderedAccessViewDesc.Format					  = Mapping::getDirect3D11ShaderResourceViewFormat(textureFormat);
					d3d11UnorderedAccessViewDesc.ViewDimension			  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
					d3d11UnorderedAccessViewDesc.Texture2DArray.ArraySize = NUMBER_OF_SLICES;

					// Create the Direct3D 11 unordered access view instance
					FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateUnorderedAccessView(mD3D11TextureCube, &d3d11UnorderedAccessViewDesc, &mD3D11UnorderedAccessView))
				}
			}

			// Let Direct3D 11 generate the mipmaps for us automatically, if necessary
			if (nullptr != data && generateMipmaps)
			{
				direct3D11Rhi.generateAsynchronousDeferredMipmaps(*this, *mD3D11ShaderResourceView);
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Cube texture", 15)	// 15 = "Cube texture: " including terminating zero
				if (nullptr != mD3D11TextureCube)
				{
					FAILED_DEBUG_BREAK(mD3D11TextureCube->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D11ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D11UnorderedAccessView)
				{
					FAILED_DEBUG_BREAK(mD3D11UnorderedAccessView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureCube() override
		{
			if (nullptr != mD3D11ShaderResourceView)
			{
				mD3D11ShaderResourceView->Release();
			}
			if (nullptr != mD3D11UnorderedAccessView)
			{
				mD3D11UnorderedAccessView->Release();
			}
			if (nullptr != mD3D11TextureCube)
			{
				mD3D11TextureCube->Release();
			}
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
		*    Return the Direct3D texture cube resource instance
		*
		*  @return
		*    The Direct3D texture cube resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11Texture2D* getD3D11TextureCube() const
		{
			return mD3D11TextureCube;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11ShaderResourceView* getD3D11ShaderResourceView() const
		{
			return mD3D11ShaderResourceView;
		}

		/**
		*  @brief
		*    Return the Direct3D unordered access view instance
		*
		*  @return
		*    The Direct3D unordered access view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 11 resource
		*      view by e.g. assigning another Direct3D 11 resource to it
		*/
		[[nodiscard]] inline ID3D11UnorderedAccessView* getD3D11UnorderedAccessView() const
		{
			return mD3D11UnorderedAccessView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IResource methods                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D11TextureCube;
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
		Rhi::TextureFormat::Enum   mTextureFormat;
		ID3D11Texture2D*		   mD3D11TextureCube;			///< Direct3D 11 texture cube resource, can be a null pointer
		ID3D11ShaderResourceView*  mD3D11ShaderResourceView;	///< Direct3D 11 shader resource view, can be a null pointer
		ID3D11UnorderedAccessView* mD3D11UnorderedAccessView;	///< Direct3D 11 unordered access view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Texture/TextureManager.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 texture manager interface
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*/
		inline explicit TextureManager(Direct3D11Rhi& direct3D11Rhi) :
			ITextureManager(direct3D11Rhi)
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
		[[nodiscard]] virtual Rhi::ITexture1D* createTexture1D(uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), width > 0, "Direct3D 11 create texture 1D was called with invalid parameters")

			// Create 1D texture resource
			return RHI_NEW(direct3D11Rhi.getContext(), Texture1D)(direct3D11Rhi, width, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), width > 0 && numberOfSlices > 0, "Direct3D 11 create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource
			return RHI_NEW(direct3D11Rhi.getContext(), Texture1DArray)(direct3D11Rhi, width, numberOfSlices, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), width > 0 && height > 0, "Direct3D 11 create texture 2D was called with invalid parameters")

			// Create 2D texture resource
			return RHI_NEW(direct3D11Rhi.getContext(), Texture2D)(direct3D11Rhi, width, height, textureFormat, data, textureFlags, textureUsage, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), width > 0 && height > 0 && numberOfSlices > 0, "Direct3D 11 create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource
			return RHI_NEW(direct3D11Rhi.getContext(), Texture2DArray)(direct3D11Rhi, width, height, numberOfSlices, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), width > 0 && height > 0 && depth > 0, "Direct3D 11 create texture 3D was called with invalid parameters")

			// Create 3D texture resource
			return RHI_NEW(direct3D11Rhi.getContext(), Texture3D)(direct3D11Rhi, width, height, depth, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), width > 0 && height > 0, "Direct3D 11 create texture cube was called with invalid parameters")

			// Create cube texture resource
			return RHI_NEW(direct3D11Rhi.getContext(), TextureCube)(direct3D11Rhi, width, height, textureFormat, data, textureFlags, textureUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
	//[ Direct3D11Rhi/State/SamplerState.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 sampler state class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(Direct3D11Rhi& direct3D11Rhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ISamplerState(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11SamplerState(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D11Rhi.getContext(), Rhi::FilterMode::UNKNOWN != samplerState.filter, "Direct3D 11 filter mode must not be unknown")
			RHI_ASSERT(direct3D11Rhi.getContext(), samplerState.maxAnisotropy <= direct3D11Rhi.getCapabilities().maximumAnisotropy, "Maximum Direct3D 11 anisotropy value violated")

			// Create the Direct3D 11 sampler state
			// -> "Rhi::SamplerState" maps directly to Direct3D 10 & 11, do not change it
			static_assert(sizeof(Rhi::SamplerState) == sizeof(D3D11_SAMPLER_DESC), "Direct3D 11 structure mismatch detected");
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateSamplerState(reinterpret_cast<const D3D11_SAMPLER_DESC*>(&samplerState), &mD3D11SamplerState))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11SamplerState)
				{
					// Avoid "D3D11 WARNING: ID3D11SamplerState::SetPrivateData: Existing private data of same name with different size found! [ STATE_SETTING WARNING #55: SETPRIVATEDATA_CHANGINGPARAMS]" messages by checking whether or not a debug name is already set (first wins)
					// -> This can happen since Direct3D 11 internally automatically checks if a sampler state instance with the requested configuration already exists
					// -> The result of "ID3D11DeviceChild::GetPrivateData()" is not checked by intent since this probably means that no debug name has been set, yet
					UINT existingDataSize = 0;
					mD3D11SamplerState->GetPrivateData(WKPDID_D3DDebugObjectName, &existingDataSize, nullptr);
					if (0 == existingDataSize)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Sampler state", 16)	// 16 = "Sampler state: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11SamplerState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SamplerState() override
		{
			// Release the Direct3D 11 sampler state
			if (nullptr != mD3D11SamplerState)
			{
				mD3D11SamplerState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 sampler state
		*
		*  @return
		*    The Direct3D 11 sampler state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11SamplerState* getD3D11SamplerState() const
		{
			return mD3D11SamplerState;
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
		ID3D11SamplerState* mD3D11SamplerState;	///< Direct3D 11 sampler state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/State/IState.h                          ]
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
	//[ Direct3D11Rhi/State/RasterizerState.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 rasterizer state class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] rasterizerState
		*    Rasterizer state to use
		*/
		RasterizerState(Direct3D11Rhi& direct3D11Rhi, const Rhi::RasterizerState& rasterizerState) :
			mD3D11RasterizerState(nullptr)
		{
			// Create the Direct3D 11 rasterizer state
			// -> "ID3D11Device::CreateRasterizerState()" takes automatically care of duplicate rasterizer state handling
			// -> Thank's to Direct3D 12, "Rhi::RasterizerState" doesn't map directly to Direct3D 10 & 11 - but at least the constants directly still map
			D3D11_RASTERIZER_DESC d3d11RasterizerDesc;
			d3d11RasterizerDesc.FillMode				= static_cast<D3D11_FILL_MODE>(rasterizerState.fillMode);
			d3d11RasterizerDesc.CullMode				= static_cast<D3D11_CULL_MODE>(rasterizerState.cullMode);
			d3d11RasterizerDesc.FrontCounterClockwise	= rasterizerState.frontCounterClockwise;
			d3d11RasterizerDesc.DepthBias				= rasterizerState.depthBias;
			d3d11RasterizerDesc.DepthBiasClamp			= rasterizerState.depthBiasClamp;
			d3d11RasterizerDesc.SlopeScaledDepthBias	= rasterizerState.slopeScaledDepthBias;
			d3d11RasterizerDesc.DepthClipEnable			= rasterizerState.depthClipEnable;
			d3d11RasterizerDesc.ScissorEnable			= rasterizerState.scissorEnable;
			d3d11RasterizerDesc.MultisampleEnable		= rasterizerState.multisampleEnable;
			d3d11RasterizerDesc.AntialiasedLineEnable	= rasterizerState.antialiasedLineEnable;
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateRasterizerState(&d3d11RasterizerDesc, &mD3D11RasterizerState))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11RasterizerState)
				{
					static constexpr char* NAME = "Rasterizer state";
					FAILED_DEBUG_BREAK(mD3D11RasterizerState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(NAME)), NAME))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~RasterizerState()
		{
			// Release the Direct3D 11 rasterizer state
			if (nullptr != mD3D11RasterizerState)
			{
				mD3D11RasterizerState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 rasterizer state
		*
		*  @return
		*    The Direct3D 11 rasterizer state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11RasterizerState* getD3D11RasterizerState() const
		{
			return mD3D11RasterizerState;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11RasterizerState* mD3D11RasterizerState;	///< Direct3D 11 rasterizer state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/State/DepthStencilState.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 depth stencil state class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] depthStencilState
		*    Depth stencil state to use
		*/
		DepthStencilState(Direct3D11Rhi& direct3D11Rhi, const Rhi::DepthStencilState& depthStencilState) :
			mD3D11DepthStencilState(nullptr)
		{
			// Create the Direct3D 11 depth stencil state
			// -> "ID3D11Device::CreateDepthStencilState()" takes automatically care of duplicate depth stencil state handling
			// -> "Rhi::DepthStencilState" maps directly to Direct3D 10 & 11 & 12, do not change it
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateDepthStencilState(reinterpret_cast<const D3D11_DEPTH_STENCIL_DESC*>(&depthStencilState), &mD3D11DepthStencilState))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11DepthStencilState)
				{
					static constexpr char* NAME = "Depth stencil state";
					FAILED_DEBUG_BREAK(mD3D11DepthStencilState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(NAME)), NAME))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~DepthStencilState()
		{
			// Release the Direct3D 11 depth stencil state
			if (nullptr != mD3D11DepthStencilState)
			{
				mD3D11DepthStencilState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 depth stencil state
		*
		*  @return
		*    The Direct3D 11 depth stencil state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11DepthStencilState* getD3D11DepthStencilState() const
		{
			return mD3D11DepthStencilState;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11DepthStencilState* mD3D11DepthStencilState;	///< Direct3D 11 depth stencil state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/State/BlendState.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 blend state class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] blendState
		*    Blend state to use
		*/
		BlendState(Direct3D11Rhi& direct3D11Rhi, const Rhi::BlendState& blendState) :
			mD3D11BlendState(nullptr)
		{
			// Create the Direct3D 11 depth stencil state
			// -> "ID3D11Device::CreateBlendState()" takes automatically care of duplicate blend state handling
			// -> "Rhi::DepthStencilState" maps directly to Direct3D 10 & 11, do not change it
			static_assert(sizeof(Rhi::BlendState) == sizeof(D3D11_BLEND_DESC), "Direct3D 11 structure mismatch detected");
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateBlendState(reinterpret_cast<const D3D11_BLEND_DESC*>(&blendState), &mD3D11BlendState))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11BlendState)
				{
					static constexpr char* NAME = "Blend state";
					FAILED_DEBUG_BREAK(mD3D11BlendState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(NAME)), NAME))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~BlendState()
		{
			// Release the Direct3D 11 blend state
			if (nullptr != mD3D11BlendState)
			{
				mD3D11BlendState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 blend state
		*
		*  @return
		*    The Direct3D 11 blend state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11BlendState* getD3D11BlendState() const
		{
			return mD3D11BlendState;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11BlendState* mD3D11BlendState;	///< Direct3D 11 blend state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/RenderTarget/RenderPass.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 render pass interface
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
			RHI_ASSERT(rhi.getContext(), mNumberOfColorAttachments < 8, "Invalid number of Direct3D 11 color attachments")
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
			RHI_ASSERT(getRhi().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid Direct3D 11 color attachment index")
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

		/**
		*  @brief
		*    Return the number of multisamples
		*
		*  @return
		*    The number of multisamples
		*/
		[[nodiscard]] inline uint8_t getNumberOfMultisamples() const
		{
			return mNumberOfMultisamples;
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
	//[ Direct3D11Rhi/QueryPool.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 asynchronous query pool interface
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		QueryPool(Direct3D11Rhi& direct3D11Rhi, Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IQueryPool(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mQueryType(queryType),
			mNumberOfQueries(numberOfQueries),
			mD3D11Queries(RHI_MALLOC_TYPED(direct3D11Rhi.getContext(), ID3D11Query*, numberOfQueries))
		{
			// Get Direct3D 11 query description
			D3D11_QUERY_DESC d3d11QueryDesc = {};
			switch (queryType)
			{
				case Rhi::QueryType::OCCLUSION:
					d3d11QueryDesc.Query = D3D11_QUERY_OCCLUSION;
					break;

				case Rhi::QueryType::PIPELINE_STATISTICS:
					d3d11QueryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
					break;

				case Rhi::QueryType::TIMESTAMP:
					d3d11QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
					break;
			}

			{ // Create Direct3D 11 queries
				ID3D11Device* d3d11Device = direct3D11Rhi.getD3D11Device();
				for (uint32_t i = 0; i < numberOfQueries; ++i)
				{
					FAILED_DEBUG_BREAK(d3d11Device->CreateQuery(&d3d11QueryDesc, &mD3D11Queries[i]))
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				switch (queryType)
				{
					case Rhi::QueryType::OCCLUSION:
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Occlusion query", 18)	// 18 = "Occlusion query: " including terminating zero
						const UINT detailedDebugNameLength = static_cast<UINT>(strlen(detailedDebugName));
						for (uint32_t i = 0; i < mNumberOfQueries; ++i)
						{
							ID3D11Query* d3d11Query = mD3D11Queries[i];
							if (nullptr != d3d11Query)
							{
								FAILED_DEBUG_BREAK(d3d11Query->SetPrivateData(WKPDID_D3DDebugObjectName, detailedDebugNameLength, detailedDebugName))
							}
						}
						break;
					}

					case Rhi::QueryType::PIPELINE_STATISTICS:
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Pipeline statistics query", 28)	// 28 = "Pipeline statistics query: " including terminating zero
						const UINT detailedDebugNameLength = static_cast<UINT>(strlen(detailedDebugName));
						for (uint32_t i = 0; i < mNumberOfQueries; ++i)
						{
							ID3D11Query* d3d11Query = mD3D11Queries[i];
							if (nullptr != d3d11Query)
							{
								FAILED_DEBUG_BREAK(d3d11Query->SetPrivateData(WKPDID_D3DDebugObjectName, detailedDebugNameLength, detailedDebugName))
							}
						}
						break;
					}

					case Rhi::QueryType::TIMESTAMP:
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Timestamp query", 18)	// 18 = "Timestamp query: " including terminating zero
						const UINT detailedDebugNameLength = static_cast<UINT>(strlen(detailedDebugName));
						for (uint32_t i = 0; i < mNumberOfQueries; ++i)
						{
							ID3D11Query* d3d11Query = mD3D11Queries[i];
							if (nullptr != d3d11Query)
							{
								FAILED_DEBUG_BREAK(d3d11Query->SetPrivateData(WKPDID_D3DDebugObjectName, detailedDebugNameLength, detailedDebugName))
							}
						}
						break;
					}
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~QueryPool() override
		{
			for (uint32_t i = 0; i < mNumberOfQueries; ++i)
			{
				mD3D11Queries[i]->Release();
			}
			RHI_FREE(getRhi().getContext(), mD3D11Queries);
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
		*    Return the number of queries
		*
		*  @return
		*    The number of queries
		*/
		[[nodiscard]] inline uint32_t getNumberOfQueries() const
		{
			return mNumberOfQueries;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 queries
		*
		*  @return
		*    The Direct3D 11 queries, considered to be always valid
		*/
		[[nodiscard]] inline ID3D11Query** getD3D11Queries() const
		{
			return mD3D11Queries;
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
		uint32_t	   mNumberOfQueries;
		ID3D11Query**  mD3D11Queries;	// The Direct3D 11 queries, considered to be always valid


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/RenderTarget/SwapChain.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 swap chain class
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
		SwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ISwapChain(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11DeviceContext1(nullptr),
			mDxgiSwapChain(nullptr),
			mD3D11RenderTargetView(nullptr),
			mD3D11DepthStencilView(nullptr),
			mSynchronizationInterval(0),
			mAllowTearing(false)
		{
			const RenderPass& d3d11RenderPass = static_cast<RenderPass&>(renderPass);
			const Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(d3d11RenderPass.getRhi());
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11DeviceContext()->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&mD3D11DeviceContext1)))

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), 1 == d3d11RenderPass.getNumberOfColorAttachments(), "There must be exactly one Direct3D 11 render pass color attachment")

			// Get the Direct3D 11 device instance
			ID3D11Device* d3d11Device = direct3D11Rhi.getD3D11Device();

			// Get the native window handle
			const HWND hWnd = reinterpret_cast<HWND>(windowHandle.nativeWindowHandle);

			// Get a DXGI factory instance
			const bool isWindows10OrGreater = ::detail::isWindows10OrGreater();
			IDXGIFactory1* dxgiFactory1 = nullptr;
			IDXGIFactory2* dxgiFactory2 = nullptr;
			{
				IDXGIDevice* dxgiDevice = nullptr;
				IDXGIAdapter* dxgiAdapter = nullptr;
				FAILED_DEBUG_BREAK(d3d11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))
				FAILED_DEBUG_BREAK(dxgiDevice->GetAdapter(&dxgiAdapter))
				FAILED_DEBUG_BREAK(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory1)))
				FAILED_DEBUG_BREAK(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory2)))

				// Determines whether tearing support is available for fullscreen borderless windows
				// -> To unlock frame rates of UWP applications on the Windows Store and providing support for both AMD Freesync and NVIDIA's G-SYNC we must explicitly allow tearing
				// -> See "Windows Dev Center" -> "Variable refresh rate displays": https://msdn.microsoft.com/en-us/library/windows/desktop/mt742104(v=vs.85).aspx
				if (isWindows10OrGreater)
				{
					IDXGIFactory5* dxgiFactory5 = nullptr;
					FAILED_DEBUG_BREAK(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory5)))
					if (nullptr != dxgiFactory5)
					{
						BOOL allowTearing = FALSE;
						if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
						{
							mAllowTearing = true;
						}
						dxgiFactory5->Release();
					}
				}

				// Release references
				dxgiAdapter->Release();
				dxgiDevice->Release();
			}

			// Get the width and height of the given native window and ensure they are never ever zero
			// -> See "getSafeWidthAndHeight()"-method comments for details
			long width  = 1;
			long height = 1;
			{
				// Get the client rectangle of the given native window
				RECT rect;
				::GetClientRect(hWnd, &rect);

				// Get the width and height...
				width  = rect.right  - rect.left;
				height = rect.bottom - rect.top;

				// ... and ensure that none of them is ever zero
				if (width < 1)
				{
					width = 1;
				}
				if (height < 1)
				{
					height = 1;
				}
			}

			{ // Create the swap chain
				UINT bufferCount = 1;
				DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_DISCARD;
				const bool isWindows8OrGreater = ::IsWindows8OrGreater();
				if (isWindows10OrGreater)
				{
					RHI_ASSERT(direct3D11Rhi.getContext(), d3d11RenderPass.getNumberOfMultisamples() == 1, "Direct3D 11 doesn't support multisampling if the flip model vertical synchronization is used")
					bufferCount = 2;
					swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				}
				else if (isWindows8OrGreater)
				{
					RHI_ASSERT(direct3D11Rhi.getContext(), d3d11RenderPass.getNumberOfMultisamples() == 1, "Direct3D 11 doesn't support multisampling if the flip model vertical synchronization is used")
					bufferCount = 2;
					swapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
				}

				// Quote from https://msdn.microsoft.com/de-de/library/windows/desktop/hh404557(v=vs.85).aspx : "Platform Update for Windows 7:  DXGI_SCALING_NONE is not supported on Windows 7 or Windows Server 2008 R2"
				if (direct3D11Rhi.getD3DFeatureLevel() == D3D_FEATURE_LEVEL_11_1 && nullptr != dxgiFactory2 && isWindows8OrGreater)
				{
					// Fill DXGI swap chain description
					DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc1 = {};
					dxgiSwapChainDesc1.Width			  = static_cast<UINT>(width);
					dxgiSwapChainDesc1.Height			  = static_cast<UINT>(height);
					dxgiSwapChainDesc1.Format			  = Mapping::getDirect3D11Format(d3d11RenderPass.getColorAttachmentTextureFormat(0));
					dxgiSwapChainDesc1.SampleDesc.Count	  = 1;
					dxgiSwapChainDesc1.SampleDesc.Quality = 0;
					dxgiSwapChainDesc1.BufferUsage		  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
					dxgiSwapChainDesc1.BufferCount		  = bufferCount;
					dxgiSwapChainDesc1.SwapEffect		  = swapEffect;
					dxgiSwapChainDesc1.Flags			  = mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

					// Fill DXGI swap chain fullscreen description
					DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullscreenDesc = {};
					dxgiSwapChainFullscreenDesc.RefreshRate.Numerator	= 60;
					dxgiSwapChainFullscreenDesc.RefreshRate.Denominator	= 1;
					dxgiSwapChainFullscreenDesc.Windowed				= TRUE;

					// Create swap chain
					IDXGISwapChain1* dxgiSwapChain = nullptr;
					FAILED_DEBUG_BREAK(dxgiFactory2->CreateSwapChainForHwnd(d3d11Device, hWnd, &dxgiSwapChainDesc1, &dxgiSwapChainFullscreenDesc, nullptr, &dxgiSwapChain))
					mDxgiSwapChain = reinterpret_cast<IDXGISwapChain*>(dxgiSwapChain);
				}
				else
				{
					// Fill DXGI swap chain description
					DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc = {};
					dxgiSwapChainDesc.BufferCount						 = bufferCount;
					dxgiSwapChainDesc.BufferDesc.Width					 = static_cast<UINT>(width);
					dxgiSwapChainDesc.BufferDesc.Height					 = static_cast<UINT>(height);
					dxgiSwapChainDesc.BufferDesc.Format					 = Mapping::getDirect3D11Format(d3d11RenderPass.getColorAttachmentTextureFormat(0));
					dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator	 = 60;
					dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
					dxgiSwapChainDesc.BufferUsage						 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
					dxgiSwapChainDesc.OutputWindow						 = hWnd;
					dxgiSwapChainDesc.SampleDesc.Count					 = 1;
					dxgiSwapChainDesc.SampleDesc.Quality				 = 0;
					dxgiSwapChainDesc.Windowed							 = TRUE;
					dxgiSwapChainDesc.SwapEffect						 = swapEffect;
					dxgiSwapChainDesc.Flags								 = mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

					// Create swap chain
					FAILED_DEBUG_BREAK(dxgiFactory1->CreateSwapChain(d3d11Device, &dxgiSwapChainDesc, &mDxgiSwapChain))
				}
			}

			// Disable alt-return for automatic fullscreen state change
			// -> We handle this manually to have more control over it
			FAILED_DEBUG_BREAK(dxgiFactory1->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER))

			// Release our DXGI factory
			dxgiFactory1->Release();
			dxgiFactory2->Release();

			// Create the Direct3D 11 views
			if (nullptr != mDxgiSwapChain)
			{
				createDirect3D11Views();

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Swap chain", 13)	// 13 = "Swap chain: " including terminating zero
					FAILED_DEBUG_BREAK(mDxgiSwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					if (nullptr != mD3D11RenderTargetView)
					{
						FAILED_DEBUG_BREAK(mD3D11RenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
					if (nullptr != mD3D11DepthStencilView)
					{
						FAILED_DEBUG_BREAK(mD3D11DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SwapChain() override
		{
			// "DXGI Overview - Destroying a Swap Chain" at MSDN http://msdn.microsoft.com/en-us/library/bb205075.aspx states
			//   "You may not release a swap chain in full-screen mode because doing so may create thread contention (which will
			//    cause DXGI to raise a non-continuable exception). Before releasing a swap chain, first switch to windowed mode
			//    (using IDXGISwapChain::SetFullscreenState( FALSE, NULL )) and then call IUnknown::Release."
			if (getFullscreenState())
			{
				setFullscreenState(false);
			}

			// Release the used resources
			if (nullptr != mD3D11DepthStencilView)
			{
				mD3D11DepthStencilView->Release();
			}
			if (nullptr != mD3D11RenderTargetView)
			{
				mD3D11RenderTargetView->Release();
			}
			if (nullptr != mDxgiSwapChain)
			{
				mDxgiSwapChain->Release();
			}
			if (nullptr != mD3D11DeviceContext1)
			{
				mD3D11DeviceContext1->Release();
			}

			// After releasing references to these resources, we need to call "Flush()" to ensure that Direct3D also releases any references it might still have to the same resources - such as pipeline bindings
			static_cast<Direct3D11Rhi&>(getRhi()).getD3D11DeviceContext()->Flush();
		}

		/**
		*  @brief
		*    Return the DXGI swap chain instance
		*
		*  @return
		*    The DXGI swap chain instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDXGISwapChain* getDxgiSwapChain() const
		{
			return mDxgiSwapChain;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 render target view instance
		*
		*  @return
		*    The Direct3D 11 render target view instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D11RenderTargetView* getD3D11RenderTargetView() const
		{
			return mD3D11RenderTargetView;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 depth stencil view instance
		*
		*  @return
		*    The Direct3D 11 depth stencil view instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D11DepthStencilView* getD3D11DepthStencilView() const
		{
			return mD3D11DepthStencilView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRenderTarget methods             ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				// Get the Direct3D 11 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain->GetDesc(&dxgiSwapChainDesc))

				// Get the width and height
				long swapChainWidth  = 1;
				long swapChainHeight = 1;
				{
					// Get the client rectangle of the native output window
					// -> Don't use the width and height stored in "DXGI_SWAP_CHAIN_DESC" -> "DXGI_MODE_DESC"
					//    because it might have been modified in order to avoid zero values
					RECT rect;
					::GetClientRect(dxgiSwapChainDesc.OutputWindow, &rect);

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
		[[nodiscard]] virtual Rhi::handle getNativeWindowHandle() const override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				// Get the Direct3D 11 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain->GetDesc(&dxgiSwapChainDesc))

				// Return the native window handle
				return reinterpret_cast<Rhi::handle>(dxgiSwapChainDesc.OutputWindow);
			}

			// Error!
			return NULL_HANDLE;
		}

		inline virtual void setVerticalSynchronizationInterval(uint32_t synchronizationInterval) override
		{
			mSynchronizationInterval = synchronizationInterval;
		}

		virtual void present() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				// TODO(co) "!getFullscreenState()": Add support for borderless window to get rid of this
				const Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRenderPass().getRhi());
				const UINT flags = ((mAllowTearing && 0 == mSynchronizationInterval && !getFullscreenState()) ? DXGI_PRESENT_ALLOW_TEARING : 0);
				handleDeviceLost(direct3D11Rhi, mDxgiSwapChain->Present(mSynchronizationInterval, flags));

				// Discard the contents of the render target
				// -> This is a valid operation only when the existing contents will be entirely overwritten. If dirty or scroll rectangles are used, this call should be removed.
				if (nullptr != mD3D11DeviceContext1)
				{
					mD3D11DeviceContext1->DiscardView(mD3D11RenderTargetView);
					if (nullptr != mD3D11DepthStencilView)
					{
						// Discard the contents of the depth stencil
						mD3D11DeviceContext1->DiscardView(mD3D11DepthStencilView);
					}
				}
			}
		}

		virtual void resizeBuffers() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

				// Get the currently set render target
				Rhi::IRenderTarget* renderTargetBackup = direct3D11Rhi.omGetRenderTarget();

				// In case this swap chain is the current render target, we have to unset it before continuing
				if (this == renderTargetBackup)
				{
					direct3D11Rhi.setGraphicsRenderTarget(nullptr);
				}
				else
				{
					renderTargetBackup = nullptr;
				}

				// Release the views
				if (nullptr != mD3D11DepthStencilView)
				{
					mD3D11DepthStencilView->Release();
					mD3D11DepthStencilView = nullptr;
				}
				if (nullptr != mD3D11RenderTargetView)
				{
					mD3D11RenderTargetView->Release();
					mD3D11RenderTargetView = nullptr;
				}

				// Get the swap chain width and height, ensures they are never ever zero
				UINT width  = 1;
				UINT height = 1;
				getSafeWidthAndHeight(width, height);

				// Resize the Direct3D 11 swap chain
				// -> Preserve the existing buffer count and format
				const HRESULT result = mDxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u);
				if (SUCCEEDED(result))
				{
					// Create the Direct3D 11 views
					// TODO(co) Rescue and reassign the resource debug name
					createDirect3D11Views();

					// If required, restore the previously set render target
					if (nullptr != renderTargetBackup)
					{
						direct3D11Rhi.setGraphicsRenderTarget(renderTargetBackup);
					}
				}
				else
				{
					handleDeviceLost(direct3D11Rhi, result);
				}
			}
		}

		[[nodiscard]] virtual bool getFullscreenState() const override
		{
			// Window mode by default
			BOOL fullscreen = FALSE;

			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				FAILED_DEBUG_BREAK(mDxgiSwapChain->GetFullscreenState(&fullscreen, nullptr))
			}

			// Done
			return (FALSE != fullscreen);
		}

		virtual void setFullscreenState(bool fullscreen) override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				FAILED_DEBUG_BREAK(mDxgiSwapChain->SetFullscreenState(fullscreen, nullptr))
			}
		}

		inline virtual void setRenderWindow([[maybe_unused]] Rhi::IRenderWindow* renderWindow) override
		{
			// TODO(sw) implement me
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

		/**
		*  @brief
		*    Return the swap chain width and height
		*
		*  @param[out] width
		*    Receives the swap chain width
		*  @param[out] height
		*    Receives the swap chain height
		*
		*  @remarks
		*    For instance "IDXGISwapChain::ResizeBuffers()" can automatically choose the width and height to match the client
		*    rectangle of the native window, but as soon as the width or height is zero we will get the error message
		*     "DXGI Error: The buffer height inferred from the output window is zero. Taking 8 as a reasonable default instead"
		*     "D3D11: ERROR: ID3D11Device::CreateTexture2D: The Dimensions are invalid. For feature level D3D_FEATURE_LEVEL_11_0,
		*      the Width (value = 116) must be between 1 and 16384, inclusively. The Height (value = 0) must be between 1 and 16384,
		*      inclusively. And, the ArraySize (value = 1) must be between 1 and 2048, inclusively. [ STATE_CREATION ERROR #101: CREATETEXTURE2D_INVALIDDIMENSIONS ]"
		*   including an evil memory leak. So, best to use this method which gets the width and height of the native output
		*   window manually and ensures it's never zero.
		*
		*  @note
		*    - "mDxgiSwapChain" must be valid when calling this method
		*/
		void getSafeWidthAndHeight(uint32_t& width, uint32_t& height) const
		{
			// Get the Direct3D 11 swap chain description
			DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
			FAILED_DEBUG_BREAK(mDxgiSwapChain->GetDesc(&dxgiSwapChainDesc))

			// Get the client rectangle of the native output window
			RECT rect;
			::GetClientRect(dxgiSwapChainDesc.OutputWindow, &rect);

			// Get the width and height...
			long swapChainWidth  = rect.right  - rect.left;
			long swapChainHeight = rect.bottom - rect.top;

			// ... and ensure that none of them is ever zero
			if (swapChainWidth < 1)
			{
				swapChainWidth = 1;
			}
			if (swapChainHeight < 1)
			{
				swapChainHeight = 1;
			}

			// Write out the width and height
			width  = static_cast<UINT>(swapChainWidth);
			height = static_cast<UINT>(swapChainHeight);
		}

		/**
		*  @brief
		*    Create the Direct3D 11 views
		*/
		void createDirect3D11Views()
		{
			// Create a render target view
			ID3D11Texture2D* d3d11Texture2DBackBuffer = nullptr;
			FAILED_DEBUG_BREAK(mDxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&d3d11Texture2DBackBuffer)))

			// Get the Direct3D 11 device instance
			ID3D11Device* d3d11Device = static_cast<Direct3D11Rhi&>(getRhi()).getD3D11Device();

			// Create a render target view
			FAILED_DEBUG_BREAK(d3d11Device->CreateRenderTargetView(d3d11Texture2DBackBuffer, nullptr, &mD3D11RenderTargetView))
			d3d11Texture2DBackBuffer->Release();

			// Create depth stencil texture
			const Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat = static_cast<RenderPass&>(getRenderPass()).getDepthStencilAttachmentTextureFormat();
			if (Rhi::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat)
			{
				// Get the swap chain width and height, ensures they are never ever zero
				UINT width  = 1;
				UINT height = 1;
				getSafeWidthAndHeight(width, height);

				// Create depth stencil texture
				ID3D11Texture2D* d3d11Texture2DDepthStencil = nullptr;
				D3D11_TEXTURE2D_DESC d3d11Texture2DDesc = {};
				d3d11Texture2DDesc.Width			  = width;
				d3d11Texture2DDesc.Height			  = height;
				d3d11Texture2DDesc.MipLevels		  = 1;
				d3d11Texture2DDesc.ArraySize		  = 1;
				d3d11Texture2DDesc.Format			  = Mapping::getDirect3D11Format(depthStencilAttachmentTextureFormat);
				d3d11Texture2DDesc.SampleDesc.Count   = 1;
				d3d11Texture2DDesc.SampleDesc.Quality = 0;
				d3d11Texture2DDesc.Usage			  = D3D11_USAGE_DEFAULT;
				d3d11Texture2DDesc.BindFlags		  = D3D11_BIND_DEPTH_STENCIL;
				d3d11Texture2DDesc.CPUAccessFlags	  = 0;
				d3d11Texture2DDesc.MiscFlags		  = 0;
				FAILED_DEBUG_BREAK(d3d11Device->CreateTexture2D(&d3d11Texture2DDesc, nullptr, &d3d11Texture2DDepthStencil))

				// Create the depth stencil view
				D3D11_DEPTH_STENCIL_VIEW_DESC d3d11DepthStencilViewDesc = {};
				d3d11DepthStencilViewDesc.Format			 = d3d11Texture2DDesc.Format;
				d3d11DepthStencilViewDesc.ViewDimension		 = D3D11_DSV_DIMENSION_TEXTURE2D;
				d3d11DepthStencilViewDesc.Texture2D.MipSlice = 0;
				FAILED_DEBUG_BREAK(d3d11Device->CreateDepthStencilView(d3d11Texture2DDepthStencil, &d3d11DepthStencilViewDesc, &mD3D11DepthStencilView))
				d3d11Texture2DDepthStencil->Release();
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11DeviceContext1*	mD3D11DeviceContext1;
		IDXGISwapChain*			mDxgiSwapChain;			///< The DXGI swap chain instance, null pointer on error
		ID3D11RenderTargetView* mD3D11RenderTargetView;	///< The Direct3D 11 render target view instance, null pointer on error
		ID3D11DepthStencilView* mD3D11DepthStencilView;	///< The Direct3D 11 depth stencil view instance, null pointer on error
		uint32_t				mSynchronizationInterval;
		bool					mAllowTearing;


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/RenderTarget/Framebuffer.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 framebuffer class
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
			mNumberOfColorTextures(static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments()),
			mColorTextures(nullptr),	// Set below
			mDepthStencilTexture(nullptr),
			mWidth(UINT_MAX),
			mHeight(UINT_MAX),
			mD3D11RenderTargetViews(nullptr),
			mD3D11DepthStencilView(nullptr)
		{
			// The Direct3D 11 "ID3D11DeviceContext::OMSetRenderTargets method"-documentation at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/ff476464%28v=vs.85%29.aspx
			// says the following about the framebuffer width and height when using multiple render targets
			//   "All render targets must have the same size in all dimensions (width and height, and depth for 3D or array size for *Array types)"
			// So, in here I use the smallest width and height as the size of the framebuffer and let Direct3D 11 handle the rest regarding errors.

			// Add a reference to the used color textures
			const Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(renderPass.getRhi());
			const Rhi::Context& context = direct3D11Rhi.getContext();
			if (mNumberOfColorTextures > 0)
			{
				mColorTextures = RHI_MALLOC_TYPED(context, Rhi::ITexture*, mNumberOfColorTextures);
				mD3D11RenderTargetViews = RHI_MALLOC_TYPED(context, ID3D11RenderTargetView*, mNumberOfColorTextures);

				// Loop through all color textures
				ID3D11RenderTargetView** d3d11RenderTargetView = mD3D11RenderTargetViews;
				Rhi::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Rhi::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments, ++d3d11RenderTargetView)
				{
					// Sanity check
					RHI_ASSERT(context, nullptr != colorFramebufferAttachments->texture, "Invalid Direct3D 11 color framebuffer attachment texture")

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
							RHI_ASSERT(context, colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 11 color framebuffer attachment mipmap index")
							RHI_ASSERT(context, 0 == colorFramebufferAttachments->layerIndex, "Invalid Direct3D 11 color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

							// Create the Direct3D 11 render target view instance
							D3D11_RENDER_TARGET_VIEW_DESC d3d11RenderTargetViewDesc = {};
							d3d11RenderTargetViewDesc.Format			 = Mapping::getDirect3D11Format(texture2D->getTextureFormat());
							d3d11RenderTargetViewDesc.ViewDimension		 = (texture2D->getNumberOfMultisamples() > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
							d3d11RenderTargetViewDesc.Texture2D.MipSlice = colorFramebufferAttachments->mipmapIndex;
							FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateRenderTargetView(texture2D->getD3D11Texture2D(), &d3d11RenderTargetViewDesc, d3d11RenderTargetView))
							break;
						}

						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);

							// Create the Direct3D 11 render target view instance
							D3D11_RENDER_TARGET_VIEW_DESC d3d11RenderTargetViewDesc = {};
							d3d11RenderTargetViewDesc.Format			 = Mapping::getDirect3D11Format(texture2DArray->getTextureFormat());
							d3d11RenderTargetViewDesc.ViewDimension		 = (texture2DArray->getNumberOfMultisamples() > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
							d3d11RenderTargetViewDesc.Texture2DArray.MipSlice		 = colorFramebufferAttachments->mipmapIndex;
							d3d11RenderTargetViewDesc.Texture2DArray.FirstArraySlice = colorFramebufferAttachments->layerIndex;
							d3d11RenderTargetViewDesc.Texture2DArray.ArraySize		 = 1;
							FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateRenderTargetView(texture2DArray->getD3D11Texture2D(), &d3d11RenderTargetViewDesc, d3d11RenderTargetView))
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
							RHI_LOG(direct3D11Rhi.getContext(), CRITICAL, "The type of the given color texture at index %u is not supported by the Direct3D 11 RHI implementation", colorTexture - mColorTextures)
							*d3d11RenderTargetView = nullptr;
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RHI_ASSERT(context, nullptr != mDepthStencilTexture, "Invalid Direct3D 11 depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(context, depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 11 depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(context, 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Direct3D 11 depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

						// Create the Direct3D 11 render target view instance
						D3D11_DEPTH_STENCIL_VIEW_DESC d3d11DepthStencilViewDesc = {};
						d3d11DepthStencilViewDesc.Format			 = Mapping::getDirect3D11Format(texture2D->getTextureFormat());
						d3d11DepthStencilViewDesc.ViewDimension		 = (texture2D->getNumberOfMultisamples() > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
						d3d11DepthStencilViewDesc.Texture2D.MipSlice = depthStencilFramebufferAttachment->mipmapIndex;
						FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateDepthStencilView(texture2D->getD3D11Texture2D(), &d3d11DepthStencilViewDesc, &mD3D11DepthStencilView))
						break;
					}

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);

						// Create the Direct3D 11 render target view instance
						D3D11_DEPTH_STENCIL_VIEW_DESC d3d11DepthStencilViewDesc = {};
						d3d11DepthStencilViewDesc.Format			 = Mapping::getDirect3D11Format(texture2DArray->getTextureFormat());
						d3d11DepthStencilViewDesc.ViewDimension		 = (texture2DArray->getNumberOfMultisamples() > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
						d3d11DepthStencilViewDesc.Texture2DArray.MipSlice		 = depthStencilFramebufferAttachment->mipmapIndex;
						d3d11DepthStencilViewDesc.Texture2DArray.FirstArraySlice = depthStencilFramebufferAttachment->layerIndex;
						d3d11DepthStencilViewDesc.Texture2DArray.ArraySize		 = 1;
						FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateDepthStencilView(texture2DArray->getD3D11Texture2D(), &d3d11DepthStencilViewDesc, &mD3D11DepthStencilView))
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
						RHI_LOG(direct3D11Rhi.getContext(), CRITICAL, "The type of the given depth stencil texture is not supported by the Direct3D 11 RHI implementation")
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RHI_ASSERT(context, false, "Invalid Direct3D 11 framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RHI_ASSERT(context, false, "Invalid Direct3D 11 framebuffer height")
				mHeight = 1;
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FBO", 6)	// 6 = "FBO: " including terminating zero
				const size_t detailedDebugNameLength = strlen(detailedDebugName);

				{ // Assign a debug name to the Direct3D 11 render target view, do also add the index to the name
					const size_t adjustedDetailedDebugNameLength = detailedDebugNameLength + 5;	// Direct3D 11 supports 8 render targets ("D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT", so: One digit + one [ + one ] + one space + terminating zero = 5 characters)
					char* nameWithIndex = RHI_MALLOC_TYPED(context, char, adjustedDetailedDebugNameLength);
					ID3D11RenderTargetView** d3d11RenderTargetViewsEnd = mD3D11RenderTargetViews + mNumberOfColorTextures;
					for (ID3D11RenderTargetView** d3d11RenderTargetView = mD3D11RenderTargetViews; d3d11RenderTargetView < d3d11RenderTargetViewsEnd; ++d3d11RenderTargetView)
					{
						sprintf_s(nameWithIndex, adjustedDetailedDebugNameLength, "%s [%u]", detailedDebugName, static_cast<uint32_t>(d3d11RenderTargetView - mD3D11RenderTargetViews));
						FAILED_DEBUG_BREAK((*d3d11RenderTargetView)->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(adjustedDetailedDebugNameLength), nameWithIndex))
					}
					RHI_FREE(context, nameWithIndex);
				}

				// Assign a debug name to the Direct3D 11 depth stencil view
				if (nullptr != mD3D11DepthStencilView)
				{
					FAILED_DEBUG_BREAK(mD3D11DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(detailedDebugNameLength), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			// Release the reference to the used color textures
			const Rhi::Context& context = getRhi().getContext();
			if (nullptr != mD3D11RenderTargetViews)
			{
				// Release references
				ID3D11RenderTargetView** d3d11RenderTargetViewsEnd = mD3D11RenderTargetViews + mNumberOfColorTextures;
				for (ID3D11RenderTargetView** d3d11RenderTargetView = mD3D11RenderTargetViews; d3d11RenderTargetView < d3d11RenderTargetViewsEnd; ++d3d11RenderTargetView)
				{
					(*d3d11RenderTargetView)->Release();
				}

				// Cleanup
				RHI_FREE(context, mD3D11RenderTargetViews);
			}
			if (nullptr != mColorTextures)
			{
				// Release references
				Rhi::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Rhi::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture)
				{
					(*colorTexture)->releaseReference();
				}

				// Cleanup
				RHI_FREE(context, mColorTextures);
			}

			// Release the reference to the used depth stencil texture
			if (nullptr != mD3D11DepthStencilView)
			{
				// Release reference
				mD3D11DepthStencilView->Release();
			}
			if (nullptr != mDepthStencilTexture)
			{
				// Release reference
				mDepthStencilTexture->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the number of color textures
		*
		*  @return
		*    The number of color textures
		*/
		[[nodiscard]] inline uint32_t getNumberOfColorTextures() const
		{
			return mNumberOfColorTextures;
		}

		/**
		*  @brief
		*    Return the color textures
		*
		*  @return
		*    The color textures, can be a null pointer, do not release the returned instances unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::ITexture** getColorTextures() const
		{
			return mColorTextures;
		}

		/**
		*  @brief
		*    Return the depth stencil texture
		*
		*  @return
		*    The depth stencil texture, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::ITexture* getDepthStencilTexture() const
		{
			return mDepthStencilTexture;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 render target views
		*
		*  @return
		*    The Direct3D 11 render target views, can be a null pointer, do not release the returned instances unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11RenderTargetView** getD3D11RenderTargetViews() const
		{
			return mD3D11RenderTargetViews;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 depth stencil view
		*
		*  @return
		*    The Direct3D 11 depth stencil view, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11DepthStencilView* getD3D11DepthStencilView() const
		{
			return mD3D11DepthStencilView;
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
		// Generic part
		uint32_t		mNumberOfColorTextures;	///< Number of color render target textures
		Rhi::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "m_nNumberOfColorTextures" textures in the provided C-array of pointers
		Rhi::ITexture*  mDepthStencilTexture;	///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t		mWidth;					///< The framebuffer width
		uint32_t		mHeight;				///< The framebuffer height
		// Direct3D 11 part
		ID3D11RenderTargetView** mD3D11RenderTargetViews;	///< The Direct3D 11 render target views (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "m_nNumberOfColorTextures" views in the provided C-array of pointers
		ID3D11DepthStencilView*  mD3D11DepthStencilView;	///< The Direct3D 11 depth stencil view (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/VertexShaderHlsl.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL vertex shader class
	*/
	class VertexShaderHlsl final : public Rhi::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader bytecode
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobVertexShader(nullptr),
			mD3D11VertexShader(nullptr)
		{
			// Backup the vertex shader bytecode
			D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobVertexShader);
			memcpy(mD3DBlobVertexShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());

			// Create the Direct3D 11 vertex shader
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateVertexShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), nullptr, &mD3D11VertexShader))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11VertexShader)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5)	// 5 = "VS: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11VertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobVertexShader(nullptr),
			mD3D11VertexShader(nullptr)
		{
			// Create the Direct3D 11 binary large object for the vertex shader
			mD3DBlobVertexShader = loadShaderFromSourcecode(direct3D11Rhi.getContext(), "vs_5_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != mD3DBlobVertexShader)
			{
				// Create the Direct3D 11 vertex shader
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateVertexShader(mD3DBlobVertexShader->GetBufferPointer(), mD3DBlobVertexShader->GetBufferSize(), nullptr, &mD3D11VertexShader))

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobVertexShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobVertexShader->GetBufferPointer()));
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != mD3D11VertexShader)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VS", 5)	// 5 = "VS: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11VertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexShaderHlsl() override
		{
			// Release the Direct3D 11 shader binary large object
			if (nullptr != mD3DBlobVertexShader)
			{
				mD3DBlobVertexShader->Release();
			}

			// Release the Direct3D 11 vertex shader
			if (nullptr != mD3D11VertexShader)
			{
				mD3D11VertexShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 vertex shader blob
		*
		*  @return
		*    Direct3D 11 vertex shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobVertexShader() const
		{
			return mD3DBlobVertexShader;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 vertex shader
		*
		*  @return
		*    Direct3D 11 vertex shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11VertexShader* getD3D11VertexShader() const
		{
			return mD3D11VertexShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShaderHlsl(const VertexShaderHlsl& source) = delete;
		VertexShaderHlsl& operator =(const VertexShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3DBlob*		    mD3DBlobVertexShader;	///< Direct3D 11 vertex shader blob, can be a null pointer
		ID3D11VertexShader* mD3D11VertexShader;		///< Direct3D 11 vertex shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/TessellationControlShaderHlsl.h  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderHlsl final : public Rhi::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) from shader bytecode
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationControlShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11HullShader(nullptr)
		{
			// Create the Direct3D 11 hull shader
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateHullShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), nullptr, &mD3D11HullShader))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11HullShader)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6)	// 6 = "TCS: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11HullShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader ("hull shader" in Direct3D terminology) from shader source code
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationControlShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationControlShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11HullShader(nullptr)
		{
			// Create the Direct3D 11 binary large object for the hull shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D11Rhi.getContext(), "hs_5_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 11 hull shader
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateHullShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), nullptr, &mD3D11HullShader))

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 11 shader binary large object
				d3dBlob->Release();

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != mD3D11HullShader)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TCS", 6)	// 6 = "TCS: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11HullShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationControlShaderHlsl() override
		{
			// Release the Direct3D 11 hull shader
			if (nullptr != mD3D11HullShader)
			{
				mD3D11HullShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 hull shader
		*
		*  @return
		*    Direct3D 11 hull shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11HullShader* getD3D11HullShader() const
		{
			return mD3D11HullShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationControlShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShaderHlsl(const TessellationControlShaderHlsl& source) = delete;
		TessellationControlShaderHlsl& operator =(const TessellationControlShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11HullShader* mD3D11HullShader;	///< Direct3D 11 hull shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/TessellationEvaluationShaderHlsl.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderHlsl final : public Rhi::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) from shader bytecode
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationEvaluationShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11DomainShader(nullptr)
		{
			// Create the Direct3D 11 domain shader
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateDomainShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), nullptr, &mD3D11DomainShader))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11DomainShader)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6)	// 6 = "TES: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11DomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader ("domain shader" in Direct3D terminology) from shader source code
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationEvaluationShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITessellationEvaluationShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11DomainShader(nullptr)
		{
			// Create the Direct3D 11 binary large object for the domain shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D11Rhi.getContext(), "ds_5_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 11 domain shader
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateDomainShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), nullptr, &mD3D11DomainShader))

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 11 shader binary large object
				d3dBlob->Release();

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != mD3D11DomainShader)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TES", 6)	// 6 = "TES: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11DomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationEvaluationShaderHlsl() override
		{
			// Release the Direct3D 11 domain shader
			if (nullptr != mD3D11DomainShader)
			{
				mD3D11DomainShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 domain shader
		*
		*  @return
		*    Direct3D 11 domain shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11DomainShader* getD3D11DomainShader() const
		{
			return mD3D11DomainShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationEvaluationShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShaderHlsl(const TessellationEvaluationShaderHlsl& source) = delete;
		TessellationEvaluationShaderHlsl& operator =(const TessellationEvaluationShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11DomainShader* mD3D11DomainShader;	///< Direct3D 11 domain shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/GeometryShaderHlsl.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL geometry shader class
	*/
	class GeometryShaderHlsl final : public Rhi::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader bytecode
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		GeometryShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11GeometryShader(nullptr)
		{
			// Create the Direct3D 11 geometry shader
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateGeometryShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), nullptr, &mD3D11GeometryShader))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11GeometryShader)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5)	// 5 = "GS: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11GeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		GeometryShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGeometryShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11GeometryShader(nullptr)
		{
			// Create the Direct3D 11 binary large object for the geometry shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D11Rhi.getContext(), "gs_5_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 11 geometry shader
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateGeometryShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), nullptr, &mD3D11GeometryShader))

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 11 shader binary large object
				d3dBlob->Release();

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != mD3D11GeometryShader)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "GS", 5)	// 5 = "GS: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11GeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GeometryShaderHlsl() override
		{
			// Release the Direct3D 11 geometry shader
			if (nullptr != mD3D11GeometryShader)
			{
				mD3D11GeometryShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 geometry shader
		*
		*  @return
		*    Direct3D 11 geometry shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11GeometryShader* getD3D11GeometryShader() const
		{
			return mD3D11GeometryShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GeometryShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShaderHlsl(const GeometryShaderHlsl& source) = delete;
		GeometryShaderHlsl& operator =(const GeometryShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11GeometryShader* mD3D11GeometryShader;	///< Direct3D 11 geometry shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/FragmentShaderHlsl.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL fragment shader ("pixel shader" in Direct3D terminology) class
	*/
	class FragmentShaderHlsl final : public Rhi::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader bytecode
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11PixelShader(nullptr)
		{
			// Create the Direct3D 11 pixel shader
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreatePixelShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), nullptr, &mD3D11PixelShader))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11PixelShader)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5)	// 5 = "FS: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11PixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IFragmentShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11PixelShader(nullptr)
		{
			// Create the Direct3D 11 binary large object for the pixel shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D11Rhi.getContext(), "ps_5_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 11 pixel shader
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreatePixelShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), nullptr, &mD3D11PixelShader))

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 11 shader binary large object
				d3dBlob->Release();

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != mD3D11PixelShader)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FS", 5)	// 5 = "FS: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11PixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~FragmentShaderHlsl() override
		{
			// Release the Direct3D 11 pixel shader
			if (nullptr != mD3D11PixelShader)
			{
				mD3D11PixelShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 pixel shader
		*
		*  @return
		*    Direct3D 11 pixel shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11PixelShader* getD3D11PixelShader() const
		{
			return mD3D11PixelShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), FragmentShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShaderHlsl(const FragmentShaderHlsl& source) = delete;
		FragmentShaderHlsl& operator =(const FragmentShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11PixelShader* mD3D11PixelShader;	///< Direct3D 11 pixel shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/ComputeShaderHlsl.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL compute shader (CS) class
	*/
	class ComputeShaderHlsl final : public Rhi::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader bytecode
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		ComputeShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11ComputeShader(nullptr)
		{
			// Create the Direct3D 11 compute shader
			FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateComputeShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), nullptr, &mD3D11ComputeShader))

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D11ComputeShader)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5)	// 5 = "CS: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D11ComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
		}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		ComputeShaderHlsl(Direct3D11Rhi& direct3D11Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputeShader(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11ComputeShader(nullptr)
		{
			// Create the Direct3D 11 binary large object for the compute shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D11Rhi.getContext(), "cs_5_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 11 compute shader
				FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateComputeShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), nullptr, &mD3D11ComputeShader))

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 11 shader binary large object
				d3dBlob->Release();

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					if (nullptr != mD3D11ComputeShader)
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "CS", 5)	// 5 = "CS: " including terminating zero
						FAILED_DEBUG_BREAK(mD3D11ComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputeShaderHlsl() override
		{
			// Release the Direct3D 11 compute shader
			if (nullptr != mD3D11ComputeShader)
			{
				mD3D11ComputeShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 11 compute shader
		*
		*  @return
		*    Direct3D 11 compute shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ComputeShader* getD3D11ComputeShader() const
		{
			return mD3D11ComputeShader;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputeShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputeShaderHlsl(const ComputeShaderHlsl& source) = delete;
		ComputeShaderHlsl& operator =(const ComputeShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D11ComputeShader* mD3D11ComputeShader;	///< Direct3D 11 compute shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/GraphicsProgramHlsl.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL graphics program class
	*/
	class GraphicsProgramHlsl final : public Rhi::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] vertexShaderHlsl
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShaderHlsl
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShaderHlsl
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderHlsl
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderHlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramHlsl(Direct3D11Rhi& direct3D11Rhi, VertexShaderHlsl* vertexShaderHlsl, TessellationControlShaderHlsl* tessellationControlShaderHlsl, TessellationEvaluationShaderHlsl* tessellationEvaluationShaderHlsl, GeometryShaderHlsl* geometryShaderHlsl, FragmentShaderHlsl* fragmentShaderHlsl RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsProgram(direct3D11Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mVertexShaderHlsl(vertexShaderHlsl),
			mTessellationControlShaderHlsl(tessellationControlShaderHlsl),
			mTessellationEvaluationShaderHlsl(tessellationEvaluationShaderHlsl),
			mGeometryShaderHlsl(geometryShaderHlsl),
			mFragmentShaderHlsl(fragmentShaderHlsl)
		{
			// Add references to the provided shaders
			if (nullptr != mVertexShaderHlsl)
			{
				mVertexShaderHlsl->addReference();
			}
			if (nullptr != mTessellationControlShaderHlsl)
			{
				mTessellationControlShaderHlsl->addReference();
			}
			if (nullptr != mTessellationEvaluationShaderHlsl)
			{
				mTessellationEvaluationShaderHlsl->addReference();
			}
			if (nullptr != mGeometryShaderHlsl)
			{
				mGeometryShaderHlsl->addReference();
			}
			if (nullptr != mFragmentShaderHlsl)
			{
				mFragmentShaderHlsl->addReference();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsProgramHlsl() override
		{
			// Release the shader references
			if (nullptr != mVertexShaderHlsl)
			{
				mVertexShaderHlsl->releaseReference();
			}
			if (nullptr != mTessellationControlShaderHlsl)
			{
				mTessellationControlShaderHlsl->releaseReference();
			}
			if (nullptr != mTessellationEvaluationShaderHlsl)
			{
				mTessellationEvaluationShaderHlsl->releaseReference();
			}
			if (nullptr != mGeometryShaderHlsl)
			{
				mGeometryShaderHlsl->releaseReference();
			}
			if (nullptr != mFragmentShaderHlsl)
			{
				mFragmentShaderHlsl->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the HLSL vertex shader the graphics program is using
		*
		*  @return
		*    The HLSL vertex shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline VertexShaderHlsl* getVertexShaderHlsl() const
		{
			return mVertexShaderHlsl;
		}

		/**
		*  @brief
		*    Return the HLSL tessellation control shader the graphics program is using
		*
		*  @return
		*    The HLSL tessellation control shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline TessellationControlShaderHlsl* getTessellationControlShaderHlsl() const
		{
			return mTessellationControlShaderHlsl;
		}

		/**
		*  @brief
		*    Return the HLSL tessellation evaluation shader the graphics program is using
		*
		*  @return
		*    The HLSL tessellation evaluation shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline TessellationEvaluationShaderHlsl* getTessellationEvaluationShaderHlsl() const
		{
			return mTessellationEvaluationShaderHlsl;
		}

		/**
		*  @brief
		*    Return the HLSL geometry shader the graphics program is using
		*
		*  @return
		*    The HLSL geometry shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline GeometryShaderHlsl* getGeometryShaderHlsl() const
		{
			return mGeometryShaderHlsl;
		}

		/**
		*  @brief
		*    Return the HLSL fragment shader the graphics program is using
		*
		*  @return
		*    The HLSL fragment shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline FragmentShaderHlsl* getFragmentShaderHlsl() const
		{
			return mFragmentShaderHlsl;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsProgramHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgramHlsl(const GraphicsProgramHlsl& source) = delete;
		GraphicsProgramHlsl& operator =(const GraphicsProgramHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		VertexShaderHlsl*				  mVertexShaderHlsl;					///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderHlsl*	  mTessellationControlShaderHlsl;		///< Tessellation control shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderHlsl* mTessellationEvaluationShaderHlsl;	///< Tessellation evaluation shader the graphics program is using (we keep a reference to it), can be a null pointer
		GeometryShaderHlsl*				  mGeometryShaderHlsl;					///< Geometry shader the graphics program is using (we keep a reference to it), can be a null pointer
		FragmentShaderHlsl*				  mFragmentShaderHlsl;					///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/Shader/ShaderLanguageHlsl.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL shader language class
	*/
	class ShaderLanguageHlsl final : public Rhi::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*/
		inline explicit ShaderLanguageHlsl(Direct3D11Rhi& direct3D11Rhi) :
			IShaderLanguage(direct3D11Rhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageHlsl() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShaderLanguage methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromBytecode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 11 vertex shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			return RHI_NEW(direct3D11Rhi.getContext(), VertexShaderHlsl)(direct3D11Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), VertexShaderHlsl)(direct3D11Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "hull shader" in Direct3D terminology
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 11 tessellation control shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			return RHI_NEW(direct3D11Rhi.getContext(), TessellationControlShaderHlsl)(direct3D11Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "hull shader" in Direct3D terminology

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), TessellationControlShaderHlsl)(direct3D11Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "domain shader" in Direct3D terminology
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 11 tessellation evaluation shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			return RHI_NEW(direct3D11Rhi.getContext(), TessellationEvaluationShaderHlsl)(direct3D11Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "domain shader" in Direct3D terminology

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), TessellationEvaluationShaderHlsl)(direct3D11Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 11 geometry shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			return RHI_NEW(direct3D11Rhi.getContext(), GeometryShaderHlsl)(direct3D11Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), GeometryShaderHlsl)(direct3D11Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 11 fragment shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			return RHI_NEW(direct3D11Rhi.getContext(), FragmentShaderHlsl)(direct3D11Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), FragmentShaderHlsl)(direct3D11Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D11Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 11 compute shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::computeShader", we know there's compute shader support
			return RHI_NEW(direct3D11Rhi.getContext(), ComputeShaderHlsl)(direct3D11Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::computeShader", we know there's compute shader support
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());
			return RHI_NEW(direct3D11Rhi.getContext(), ComputeShaderHlsl)(direct3D11Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Rhi::IRootSignature& rootSignature, [[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, Rhi::ITessellationControlShader* tessellationControlShader, Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D11Rhi& direct3D11Rhi = static_cast<Direct3D11Rhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(direct3D11Rhi.getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 11 vertex shader language mismatch")
			RHI_ASSERT(direct3D11Rhi.getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 11 tessellation control shader language mismatch")
			RHI_ASSERT(direct3D11Rhi.getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 11 tessellation evaluation shader language mismatch")
			RHI_ASSERT(direct3D11Rhi.getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 11 geometry shader language mismatch")
			RHI_ASSERT(direct3D11Rhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 11 fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(direct3D11Rhi.getContext(), GraphicsProgramHlsl)(direct3D11Rhi, static_cast<VertexShaderHlsl*>(vertexShader), static_cast<TessellationControlShaderHlsl*>(tessellationControlShader), static_cast<TessellationEvaluationShaderHlsl*>(tessellationEvaluationShader), static_cast<GeometryShaderHlsl*>(geometryShader), static_cast<FragmentShaderHlsl*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ShaderLanguageHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageHlsl(const ShaderLanguageHlsl& source) = delete;
		ShaderLanguageHlsl& operator =(const ShaderLanguageHlsl& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/State/GraphicsPipelineState.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 graphics pipeline state class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(Direct3D11Rhi& direct3D11Rhi, const Rhi::GraphicsPipelineState& graphicsPipelineState, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsPipelineState(direct3D11Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mD3D11PrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(graphicsPipelineState.primitiveTopology)),
			mD3D11InputLayout(nullptr),
			mRasterizerState(direct3D11Rhi, graphicsPipelineState.rasterizerState),
			mDepthStencilState(direct3D11Rhi, graphicsPipelineState.depthStencilState),
			mBlendState(direct3D11Rhi, graphicsPipelineState.blendState)
		{
			// Ensure a correct reference counter behaviour
			graphicsPipelineState.rootSignature->addReference();
			graphicsPipelineState.rootSignature->releaseReference();

			// Add a reference to the referenced RHI resources
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			// Create Direct3D 11 input element descriptions with support for attribute-less rendering
			const uint32_t numberOfAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
			if (numberOfAttributes > 0)
			{
				const VertexShaderHlsl* vertexShaderHlsl = static_cast<GraphicsProgramHlsl*>(mGraphicsProgram)->getVertexShaderHlsl();
				if (nullptr != vertexShaderHlsl)
				{
					ID3DBlob* d3dBlobVertexShader = vertexShaderHlsl->getD3DBlobVertexShader();
					if (nullptr != d3dBlobVertexShader)
					{
						const Rhi::VertexAttribute* attributes = graphicsPipelineState.vertexAttributes.attributes;

						// TODO(co) We could manage in here without new/delete when using a fixed maximum supported number of elements
						const Rhi::Context& context = direct3D11Rhi.getContext();
						D3D11_INPUT_ELEMENT_DESC* d3d11InputElementDescs   = numberOfAttributes ? RHI_MALLOC_TYPED(context, D3D11_INPUT_ELEMENT_DESC, numberOfAttributes) : RHI_MALLOC_TYPED(context, D3D11_INPUT_ELEMENT_DESC, 1);
						D3D11_INPUT_ELEMENT_DESC* d3d11InputElementDesc    = d3d11InputElementDescs;
						D3D11_INPUT_ELEMENT_DESC* d3d11InputElementDescEnd = d3d11InputElementDescs + numberOfAttributes;
						for (; d3d11InputElementDesc < d3d11InputElementDescEnd; ++d3d11InputElementDesc, ++attributes)
						{
							// Fill the "D3D11_INPUT_ELEMENT_DESC"-content
							d3d11InputElementDesc->SemanticName      = attributes->semanticName;										// Semantic name (LPCSTR)
							d3d11InputElementDesc->SemanticIndex     = attributes->semanticIndex;										// Semantic index (UINT)
							d3d11InputElementDesc->Format            = Mapping::getDirect3D11Format(attributes->vertexAttributeFormat);	// Format (DXGI_FORMAT)
							d3d11InputElementDesc->InputSlot         = static_cast<UINT>(attributes->inputSlot);						// Input slot (UINT)
							d3d11InputElementDesc->AlignedByteOffset = attributes->alignedByteOffset;									// Aligned byte offset (UINT)

							// Per-instance instead of per-vertex?
							if (attributes->instancesPerElement > 0)
							{
								d3d11InputElementDesc->InputSlotClass       = D3D11_INPUT_PER_INSTANCE_DATA;	// Input classification (D3D11_INPUT_CLASSIFICATION)
								d3d11InputElementDesc->InstanceDataStepRate = attributes->instancesPerElement;	// Instance data step rate (UINT)
							}
							else
							{
								d3d11InputElementDesc->InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;	// Input classification (D3D11_INPUT_CLASSIFICATION)
								d3d11InputElementDesc->InstanceDataStepRate = 0;							// Instance data step rate (UINT)
							}
						}

						// Create the Direct3D 11 input layout
						FAILED_DEBUG_BREAK(direct3D11Rhi.getD3D11Device()->CreateInputLayout(d3d11InputElementDescs, numberOfAttributes, d3dBlobVertexShader->GetBufferPointer(), d3dBlobVertexShader->GetBufferSize(), &mD3D11InputLayout))

						// Destroy Direct3D 11 input element descriptions
						RHI_FREE(context, d3d11InputElementDescs);

						// Assign a default name to the resource for debugging purposes
						#ifdef RHI_DEBUG
							if (nullptr != mD3D11InputLayout)
							{
								RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics PSO", 15)	// 15 = "Graphics PSO: " including terminating zero
								FAILED_DEBUG_BREAK(mD3D11InputLayout->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
							}
						#endif
					}
				}
				else
				{
					RHI_LOG(direct3D11Rhi.getContext(), CRITICAL, "Failed to create the Direct3D 11 graphics pipeline stage input layout because there's no vertex shader")
				}
			}
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

			// Release the Direct3D 11 input layout
			if (nullptr != mD3D11InputLayout)
			{
				mD3D11InputLayout->Release();
			}

			// Free the unique compact graphics pipeline state ID
			static_cast<Direct3D11Rhi&>(getRhi()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the Direct3D 11 primitive topology
		*
		*  @return
		*    The Direct3D 11 primitive topology
		*/
		[[nodiscard]] inline D3D11_PRIMITIVE_TOPOLOGY getD3D11PrimitiveTopology() const
		{
			return mD3D11PrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the Direct3D 11 input layout
		*
		*  @return
		*    Direct3D 11 input layout instance, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11InputLayout* getD3D11InputLayout() const
		{
			return mD3D11InputLayout;
		}

		/**
		*  @brief
		*    Bind the graphics pipeline state
		*
		*  @param[in] d3d11DeviceContext
		*    The Direct3D 11 device context instance
		*/
		void bindGraphicsPipelineState(ID3D11DeviceContext& d3d11DeviceContext, CurrentGraphicsPipelineState& currentGraphicsPipelineState) const
		{
			// Set the graphics program
			if (currentGraphicsPipelineState.graphicsProgram != mGraphicsProgram)
			{
				currentGraphicsPipelineState.graphicsProgram = mGraphicsProgram;
				static_cast<Direct3D11Rhi&>(getRhi()).setGraphicsProgram(mGraphicsProgram);
			}

			// Set the Direct3D 11 input layout
			if (nullptr != mD3D11InputLayout && currentGraphicsPipelineState.d3d11InputLayout != mD3D11InputLayout)
			{
				currentGraphicsPipelineState.d3d11InputLayout = mD3D11InputLayout;
				d3d11DeviceContext.IASetInputLayout(mD3D11InputLayout);
			}

			// Set the Direct3D 11 rasterizer state
			if (currentGraphicsPipelineState.d3d11RasterizerState != mRasterizerState.getD3D11RasterizerState())
			{
				currentGraphicsPipelineState.d3d11RasterizerState = mRasterizerState.getD3D11RasterizerState();
				d3d11DeviceContext.RSSetState(currentGraphicsPipelineState.d3d11RasterizerState);
			}

			// Set Direct3D 11 depth stencil state
			if (currentGraphicsPipelineState.d3d11DepthStencilState != mDepthStencilState.getD3D11DepthStencilState())
			{
				currentGraphicsPipelineState.d3d11DepthStencilState = mDepthStencilState.getD3D11DepthStencilState();
				d3d11DeviceContext.OMSetDepthStencilState(currentGraphicsPipelineState.d3d11DepthStencilState, 0);
			}

			// Set Direct3D 11 blend state
			if (currentGraphicsPipelineState.d3d11BlendState != mBlendState.getD3D11BlendState())
			{
				currentGraphicsPipelineState.d3d11BlendState = mBlendState.getD3D11BlendState();
				d3d11DeviceContext.OMSetBlendState(currentGraphicsPipelineState.d3d11BlendState, 0, 0xffffffff);
			}
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
		Rhi::IGraphicsProgram*	 mGraphicsProgram;
		Rhi::IRenderPass*		 mRenderPass;
		D3D11_PRIMITIVE_TOPOLOGY mD3D11PrimitiveTopology;
		ID3D11InputLayout*		 mD3D11InputLayout;		///< Direct3D 11 input layout, can be a null pointer
		RasterizerState			 mRasterizerState;
		DepthStencilState		 mDepthStencilState;
		BlendState				 mBlendState;


	};




	//[-------------------------------------------------------]
	//[ Direct3D11Rhi/State/ComputePipelineState.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 11 compute pipeline state class
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
		*  @param[in] direct3D11Rhi
		*    Owner Direct3D 11 RHI instance
		*  @param[in] computeShader
		*    Compute shader to use
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		ComputePipelineState(Direct3D11Rhi& direct3D11Rhi, Rhi::IComputeShader& computeShader, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IComputePipelineState(direct3D11Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D11ComputeShader(static_cast<ComputeShaderHlsl&>(computeShader).getD3D11ComputeShader())
		{
			// Ensure a correct reference counter behaviour
			if (nullptr != mD3D11ComputeShader)
			{
				mD3D11ComputeShader->AddRef();
			}
			computeShader.addReference();
			computeShader.releaseReference();
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputePipelineState() override
		{
			// Release the Direct3D 11 compute shader
			if (nullptr != mD3D11ComputeShader)
			{
				mD3D11ComputeShader->Release();
			}

			// Free the unique compact compute pipeline state ID
			static_cast<Direct3D11Rhi&>(getRhi()).ComputePipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the Direct3D 11 compute shader
		*
		*  @return
		*    Direct3D 11 compute shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D11ComputeShader* getD3D11ComputeShader() const
		{
			return mD3D11ComputeShader;
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
		ID3D11ComputeShader* mD3D11ComputeShader;	///< Direct3D 11 compute shader, can be a null pointer


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D11Rhi




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
		bool createDevice(AGSContext* agsContext, UINT flags, ID3D11Device** d3d11Device, ID3D11DeviceContext** d3d11DeviceContext, D3D_FEATURE_LEVEL& d3dFeatureLevel)
		{
			// Driver types
			static constexpr D3D_DRIVER_TYPE D3D_DRIVER_TYPES[] =
			{
				D3D_DRIVER_TYPE_HARDWARE,
				D3D_DRIVER_TYPE_WARP,
				D3D_DRIVER_TYPE_REFERENCE,
			};
			static constexpr UINT NUMBER_OF_DRIVER_TYPES = _countof(D3D_DRIVER_TYPES);

			// Feature levels
			static constexpr D3D_FEATURE_LEVEL D3D_FEATURE_LEVELS[] =
			{
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
			};
			static constexpr UINT NUMBER_OF_FEATURE_LEVELS = _countof(D3D_FEATURE_LEVELS);

			// Create the Direct3D 11 device
			if (nullptr != agsContext)
			{
				for (UINT deviceType = 0; deviceType < NUMBER_OF_DRIVER_TYPES; ++deviceType)
				{
					AGSDX11ExtensionParams agsDx11ExtensionParams = {};
					AGSDX11ReturnedParams agsDx11ReturnedParams = {};
					AGSDX11DeviceCreationParams agsDx11DeviceCreationParams1 = { nullptr, D3D_DRIVER_TYPES[deviceType], nullptr, flags, D3D_FEATURE_LEVELS, NUMBER_OF_FEATURE_LEVELS, D3D11_SDK_VERSION, nullptr };
					if (agsDriverExtensionsDX11_CreateDevice(agsContext, &agsDx11DeviceCreationParams1, &agsDx11ExtensionParams, &agsDx11ReturnedParams) == AGS_SUCCESS)
					{
						*d3d11Device = agsDx11ReturnedParams.pDevice;
						*d3d11DeviceContext = agsDx11ReturnedParams.pImmediateContext;

						// Done
						return true;
					}
					else
					{
						// Maybe the system doesn't support Direct3D 11.1, try again requesting Direct3D 11
						AGSDX11DeviceCreationParams agsDx11DeviceCreationParams2 = { nullptr, D3D_DRIVER_TYPES[deviceType], nullptr, flags, &D3D_FEATURE_LEVELS[1], NUMBER_OF_FEATURE_LEVELS - 1, D3D11_SDK_VERSION, nullptr };
						if (agsDriverExtensionsDX11_CreateDevice(agsContext, &agsDx11DeviceCreationParams2, &agsDx11ExtensionParams, &agsDx11ReturnedParams) == AGS_SUCCESS)
						{
							*d3d11Device = agsDx11ReturnedParams.pDevice;
							*d3d11DeviceContext = agsDx11ReturnedParams.pImmediateContext;

							// Done
							return true;
						}
					}
				}
			}
			else
			{
				for (UINT deviceType = 0; deviceType < NUMBER_OF_DRIVER_TYPES; ++deviceType)
				{
					const HRESULT result = Direct3D11Rhi::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPES[deviceType], nullptr, flags, D3D_FEATURE_LEVELS, NUMBER_OF_FEATURE_LEVELS, D3D11_SDK_VERSION, d3d11Device, &d3dFeatureLevel, d3d11DeviceContext);
					if (SUCCEEDED(result))
					{
						// Done
						return true;
					}
					else if (E_INVALIDARG == result)
					{
						// Maybe the system doesn't support Direct3D 11.1, try again requesting Direct3D 11
						if (SUCCEEDED(Direct3D11Rhi::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPES[deviceType], nullptr, flags, &D3D_FEATURE_LEVELS[1], NUMBER_OF_FEATURE_LEVELS - 1, D3D11_SDK_VERSION, d3d11Device, &d3dFeatureLevel, d3d11DeviceContext)))
						{
							// Done
							return true;
						}
					}
				}
			}

			// Error!
			return false;
		}


		namespace ImplementationDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ExecuteCommandBuffer* realData = static_cast<const Rhi::Command::ExecuteCommandBuffer*>(data);
				RHI_ASSERT(rhi.getContext(), nullptr != realData->commandBufferToExecute, "The Direct3D 11 command buffer to execute must be valid")
				rhi.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsRootSignature* realData = static_cast<const Rhi::Command::SetGraphicsRootSignature*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsPipelineState* realData = static_cast<const Rhi::Command::SetGraphicsPipelineState*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsResourceGroup* realData = static_cast<const Rhi::Command::SetGraphicsResourceGroup*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Rhi::IRhi& rhi)
			{
				// Input-assembler (IA) stage
				const Rhi::Command::SetGraphicsVertexArray* realData = static_cast<const Rhi::Command::SetGraphicsVertexArray*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsViewports* realData = static_cast<const Rhi::Command::SetGraphicsViewports*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Rhi::Viewport*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsScissorRectangles* realData = static_cast<const Rhi::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Rhi::ScissorRectangle*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Rhi::IRhi& rhi)
			{
				// Output-merger (OM) stage
				const Rhi::Command::SetGraphicsRenderTarget* realData = static_cast<const Rhi::Command::SetGraphicsRenderTarget*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ClearGraphics* realData = static_cast<const Rhi::Command::ClearGraphics*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawGraphicsAgs(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawGraphicsAgs(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawGraphicsNvApi(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawGraphicsNvApi(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawIndexedGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphicsAgs(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawIndexedGraphicsAgs(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphicsNvApi(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawIndexedGraphicsNvApi(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeRootSignature* realData = static_cast<const Rhi::Command::SetComputeRootSignature*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputePipelineState* realData = static_cast<const Rhi::Command::SetComputePipelineState*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeResourceGroup* realData = static_cast<const Rhi::Command::SetComputeResourceGroup*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCompute* realData = static_cast<const Rhi::Command::DispatchCompute*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).dispatchCompute(realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Rhi::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Rhi::ResourceType::TEXTURE_2D)
				{
					static_cast<Direct3D11Rhi::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RHI_LOG(static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).getContext(), CRITICAL, "Unsupported Direct3D 11 texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Rhi::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyResource* realData = static_cast<const Rhi::Command::CopyResource*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::GenerateMipmaps* realData = static_cast<const Rhi::Command::GenerateMipmaps*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResetQueryPool* realData = static_cast<const Rhi::Command::ResetQueryPool*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::BeginQuery* realData = static_cast<const Rhi::Command::BeginQuery*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::EndQuery* realData = static_cast<const Rhi::Command::EndQuery*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::WriteTimestampQuery* realData = static_cast<const Rhi::Command::WriteTimestampQuery*>(data);
				static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RHI_DEBUG
				void SetDebugMarker(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::SetDebugMarker* realData = static_cast<const Rhi::Command::SetDebugMarker*>(data);
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::BeginDebugEvent* realData = static_cast<const Rhi::Command::BeginDebugEvent*>(data);
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Rhi::IRhi& rhi)
				{
					static_cast<Direct3D11Rhi::Direct3D11Rhi&>(rhi).endDebugEvent();
				}
			#else
				inline void SetDebugMarker(const void*, Rhi::IRhi&)
				{}

				inline void BeginDebugEvent(const void*, Rhi::IRhi&)
				{}

				inline void EndDebugEvent(const void*, Rhi::IRhi&)
				{}
			#endif


		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static Rhi::ImplementationDispatchFunction DISPATCH_FUNCTIONS[static_cast<uint8_t>(Rhi::CommandDispatchFunctionIndex::NUMBER_OF_FUNCTIONS)] =
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
namespace Direct3D11Rhi
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Direct3D11Rhi::Direct3D11Rhi(const Rhi::Context& context) :
		IRhi(Rhi::NameId::DIRECT3D11, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		ComputePipelineStateMakeId(context.getAllocator()),
		mDirect3D11RuntimeLinking(nullptr),
		mD3D11Device(nullptr),
		mD3D11DeviceContext(nullptr),
		mD3DFeatureLevel(D3D_FEATURE_LEVEL_11_0),
		mD3DUserDefinedAnnotation(nullptr),
		mShaderLanguageHlsl(nullptr),
		mD3D11QueryFlush(nullptr),
		mRenderTarget(nullptr),
		mGraphicsRootSignature(nullptr),
		mComputeRootSignature(nullptr),
		mD3D11PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
		mD3d11VertexShader(nullptr),
		mD3d11HullShader(nullptr),
		mD3d11DomainShader(nullptr),
		mD3d11GeometryShader(nullptr),
		mD3d11PixelShader(nullptr),
		mD3d11ComputeShader(nullptr)
		#ifdef RHI_DEBUG
			, mDebugBetweenBeginEndScene(false)
		#endif
	{
		mDirect3D11RuntimeLinking = RHI_NEW(context, Direct3D11RuntimeLinking)(*this);

		// Is Direct3D 11 available?
		if (mDirect3D11RuntimeLinking->isDirect3D11Avaiable())
		{
			// Flags
			UINT flags = 0;
			#ifdef RHI_DEBUG
				flags |= D3D11_CREATE_DEVICE_DEBUG;
			#endif

			// Create the Direct3D 11 device
			AGSContext* agsContext = mDirect3D11RuntimeLinking->getAgsContext();
			if (!detail::createDevice(agsContext, flags, &mD3D11Device, &mD3D11DeviceContext, mD3DFeatureLevel) && (flags & D3D11_CREATE_DEVICE_DEBUG))
			{
				RHI_LOG(mContext, CRITICAL, "Failed to create the Direct3D 11 device instance, retrying without debug flag (maybe no Windows SDK is installed)")
				flags &= ~D3D11_CREATE_DEVICE_DEBUG;
				detail::createDevice(agsContext, flags, &mD3D11Device, &mD3D11DeviceContext, mD3DFeatureLevel);
			}

			// Update dispatch draw function pointers, if needed
			if (nullptr != agsContext)
			{
				detail::DISPATCH_FUNCTIONS[static_cast<uint32_t>(Rhi::CommandDispatchFunctionIndex::DRAW_GRAPHICS)]			= &detail::ImplementationDispatch::DrawGraphicsAgs;
				detail::DISPATCH_FUNCTIONS[static_cast<uint32_t>(Rhi::CommandDispatchFunctionIndex::DRAW_INDEXED_GRAPHICS)] = &detail::ImplementationDispatch::DrawIndexedGraphicsAgs;
			}
			else
			{
				detail::DISPATCH_FUNCTIONS[static_cast<uint32_t>(Rhi::CommandDispatchFunctionIndex::DRAW_GRAPHICS)]			= (nullptr != NvAPI_D3D11_MultiDrawInstancedIndirect)		 ? &detail::ImplementationDispatch::DrawGraphicsNvApi		 : &detail::ImplementationDispatch::DrawGraphics;
				detail::DISPATCH_FUNCTIONS[static_cast<uint32_t>(Rhi::CommandDispatchFunctionIndex::DRAW_INDEXED_GRAPHICS)] = (nullptr != NvAPI_D3D11_MultiDrawIndexedInstancedIndirect) ? &detail::ImplementationDispatch::DrawIndexedGraphicsNvApi : &detail::ImplementationDispatch::DrawIndexedGraphics;
			}

			// Is there a valid Direct3D 11 device and device context?
			if (nullptr != mD3D11Device && nullptr != mD3D11DeviceContext)
			{
				// Direct3D 11 debug related stuff
				if (flags & D3D11_CREATE_DEVICE_DEBUG)
				{
					#ifdef RHI_DEBUG
						// Try to get the Direct3D 11 user defined annotation interface, Direct3D 11.1 feature
						FAILED_DEBUG_BREAK(mD3D11DeviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<LPVOID*>(&mD3DUserDefinedAnnotation)))
					#endif

					// Direct3D 11 debug settings
					ID3D11Debug* d3d11Debug = nullptr;
					if (SUCCEEDED(mD3D11Device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<LPVOID*>(&d3d11Debug))))
					{
						ID3D11InfoQueue* d3d11InfoQueue = nullptr;
						if (SUCCEEDED(d3d11Debug->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<LPVOID*>(&d3d11InfoQueue))))
						{
							// When using render-to-texture, Direct3D 11 will quickly spam the log with
							//   "
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets: Resource being set to OM RenderTarget slot 0 is still bound on input! [ STATE_SETTING WARNING #9: DEVICE_OMSETRENDERTARGETS_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing VS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #3: DEVICE_VSSETSHADERRESOURCES_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing HS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #2097173: DEVICE_HSSETSHADERRESOURCES_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing DS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #2097189: DEVICE_DSSETSHADERRESOURCES_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing GS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #5: DEVICE_GSSETSHADERRESOURCES_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing PS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #7: DEVICE_PSSETSHADERRESOURCES_HAZARD]
							//   "
							// When not unbinding render targets from shader resources, even if shaders never access the render target by reading. We could add extra
							// logic to avoid this situation, but on the other hand, the RHI implementation should be as slim as possible. Since those Direct3D 11 warnings
							// are pretty annoying and introduce the risk of missing relevant warnings, let's suppress those warnings. Thought about this for a while, feels
							// like the best solution considering the alternatives even if suppressing warnings is not always the best idea.
							D3D11_MESSAGE_ID d3d11MessageIds[] =
							{
								D3D11_MESSAGE_ID_DEVICE_IASETVERTEXBUFFERS_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_IASETINDEXBUFFER_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_OMSETRENDERTARGETS_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_VSSETSHADERRESOURCES_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_GSSETSHADERRESOURCES_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_PSSETSHADERRESOURCES_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_HSSETSHADERRESOURCES_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_DSSETSHADERRESOURCES_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_CSSETSHADERRESOURCES_HAZARD,
								D3D11_MESSAGE_ID_DEVICE_CSSETUNORDEREDACCESSVIEWS_HAZARD
							};
							D3D11_INFO_QUEUE_FILTER d3d11InfoQueueFilter = {};
							d3d11InfoQueueFilter.DenyList.NumIDs = _countof(d3d11MessageIds);
							d3d11InfoQueueFilter.DenyList.pIDList = d3d11MessageIds;
							FAILED_DEBUG_BREAK(d3d11InfoQueue->AddStorageFilterEntries(&d3d11InfoQueueFilter))

							// Sadly, when using the Direct3D 11 break feature we're having a confusing call stack, so we don't use this and use "FAILED_DEBUG_BREAK()" instead
							// d3d11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
							// d3d11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
							// d3d11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
							// d3d11InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_INFO, true);

							d3d11InfoQueue->Release();
						}
						d3d11Debug->Release();
					}
				}

				// Initialize the capabilities
				initializeCapabilities();
			}
			else
			{
				RHI_LOG(mContext, CRITICAL, "Failed to create the Direct3D 11 device and device context instance")
			}
		}
	}

	Direct3D11Rhi::~Direct3D11Rhi()
	{
		// Release instances
		for (Rhi::ITexture* texture : mGenerateAsynchronousMipmapsForTextures)
		{
			texture->releaseReference();
		}
		if (nullptr != mRenderTarget)
		{
			mRenderTarget->releaseReference();
			mRenderTarget = nullptr;
		}
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
			mGraphicsRootSignature = nullptr;
		}
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
			mComputeRootSignature = nullptr;
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
					RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation is going to be destroyed, but there are still %u resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the Direct3D 11 query instance used for flush, in case we have one
		if (nullptr != mD3D11QueryFlush)
		{
			mD3D11QueryFlush->Release();
		}

		// Release the HLSL shader language instance, in case we have one
		if (nullptr != mShaderLanguageHlsl)
		{
			mShaderLanguageHlsl->releaseReference();
		}

		// Release the Direct3D 11 device we've created
		if (nullptr != mD3DUserDefinedAnnotation)
		{
			mD3DUserDefinedAnnotation->Release();
			mD3DUserDefinedAnnotation = nullptr;
		}
		{
			AGSContext* agsContext = mDirect3D11RuntimeLinking->getAgsContext();
			if (nullptr != agsContext)
			{
				unsigned int deviceReferences = 0;
				unsigned int immediateContextReferences = 0;
				agsDriverExtensionsDX11_DestroyDevice(agsContext, mD3D11Device, &deviceReferences, mD3D11DeviceContext, &immediateContextReferences);
				// TODO(co) AMD AGS v5.3.0 - November 22, 2018 behaves odd when it comes to the reference counters, "deviceReferences" handles as "immediateContextReferences"?
				//          When done in any other way there will be crashes or resource leaks.
				if (nullptr != mD3D11DeviceContext && deviceReferences > 0)
				{
					mD3D11DeviceContext->Release();
				}
				mD3D11DeviceContext = nullptr;
				mD3D11Device = nullptr;
			}
			else
			{
				if (nullptr != mD3D11DeviceContext)
				{
					mD3D11DeviceContext->Release();
					mD3D11DeviceContext = nullptr;
				}
				if (nullptr != mD3D11Device)
				{
					mD3D11Device->Release();
					mD3D11Device = nullptr;
				}
			}
		}

		// Destroy the Direct3D 11 runtime linking instance
		RHI_DELETE(mContext, Direct3D11RuntimeLinking, mDirect3D11RuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void Direct3D11Rhi::setGraphicsRootSignature(Rhi::IRootSignature* rootSignature)
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

	void Direct3D11Rhi::setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsPipelineState)

			// Set primitive topology
			// -> The "Rhi::PrimitiveTopology" values directly map to Direct3D 9 & 10 & 11 constants, do not change them
			const GraphicsPipelineState* direct3D11GraphicsPipelineState = static_cast<const GraphicsPipelineState*>(graphicsPipelineState);
			if (mD3D11PrimitiveTopology != direct3D11GraphicsPipelineState->getD3D11PrimitiveTopology())
			{
				mD3D11PrimitiveTopology = direct3D11GraphicsPipelineState->getD3D11PrimitiveTopology();
				mD3D11DeviceContext->IASetPrimitiveTopology(mD3D11PrimitiveTopology);
			}

			// Set graphics pipeline state
			direct3D11GraphicsPipelineState->bindGraphicsPipelineState(*mD3D11DeviceContext, mCurrentGraphicsPipelineState);
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D11Rhi::setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RHI_LOG(mContext, CRITICAL, "No Direct3D 11 RHI implementation graphics root signature set")
				return;
			}
			const Rhi::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation root parameter index is out of bounds")
				return;
			}
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Rhi::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Set graphics resource group
			const ResourceGroup* d3d11ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = d3d11ResourceGroup->getNumberOfResources();
			Rhi::IResource** resources = d3d11ResourceGroup->getResources();
			const Rhi::RootParameter& rootParameter = mGraphicsRootSignature->getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				const Rhi::IResource* resource = *resources;
				RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid Direct3D 11 descriptor ranges")
				const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Rhi::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Rhi::ResourceType::UNIFORM_BUFFER:
					{
						ID3D11Buffer* d3d11Buffers = static_cast<const UniformBuffer*>(resource)->getD3D11Buffer();
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Rhi::ShaderVisibility::ALL:
								mD3D11DeviceContext->VSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->HSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->DSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->GSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->PSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::VERTEX:
								mD3D11DeviceContext->VSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
								// "hull shader" in Direct3D terminology
								mD3D11DeviceContext->HSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
								// "domain shader" in Direct3D terminology
								mD3D11DeviceContext->DSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::GEOMETRY:
								mD3D11DeviceContext->GSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology
								mD3D11DeviceContext->PSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::COMPUTE:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 compute shader visibility")
								break;

							case Rhi::ShaderVisibility::ALL_GRAPHICS:
								mD3D11DeviceContext->VSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->HSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->DSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->GSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								mD3D11DeviceContext->PSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;
						}
						break;
					}

					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_2D:
					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					{
						ID3D11ShaderResourceView* d3d11ShaderResourceView = nullptr;
						switch (resourceType)
						{
							case Rhi::ResourceType::TEXTURE_BUFFER:
								d3d11ShaderResourceView = static_cast<const TextureBuffer*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::STRUCTURED_BUFFER:
								d3d11ShaderResourceView = static_cast<const StructuredBuffer*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::TEXTURE_1D:
								d3d11ShaderResourceView = static_cast<const Texture1D*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::TEXTURE_1D_ARRAY:
								d3d11ShaderResourceView = static_cast<const Texture1DArray*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::TEXTURE_2D:
								d3d11ShaderResourceView = static_cast<const Texture2D*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::TEXTURE_2D_ARRAY:
								d3d11ShaderResourceView = static_cast<const Texture2DArray*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::TEXTURE_3D:
								d3d11ShaderResourceView = static_cast<const Texture3D*>(resource)->getD3D11ShaderResourceView();
								break;

							case Rhi::ResourceType::TEXTURE_CUBE:
								d3d11ShaderResourceView = static_cast<const TextureCube*>(resource)->getD3D11ShaderResourceView();
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
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 RHI implementation resource type")
								break;
						}
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Rhi::ShaderVisibility::ALL:
								mD3D11DeviceContext->VSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->HSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->DSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->GSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->PSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;

							case Rhi::ShaderVisibility::VERTEX:
								mD3D11DeviceContext->VSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;

							case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
								// "hull shader" in Direct3D terminology
								mD3D11DeviceContext->HSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;

							case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
								// "domain shader" in Direct3D terminology
								mD3D11DeviceContext->DSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;

							case Rhi::ShaderVisibility::GEOMETRY:
								mD3D11DeviceContext->GSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;

							case Rhi::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology
								mD3D11DeviceContext->PSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;

							case Rhi::ShaderVisibility::COMPUTE:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 compute shader visibility")
								break;

							case Rhi::ShaderVisibility::ALL_GRAPHICS:
								mD3D11DeviceContext->VSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->HSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->DSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->GSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								mD3D11DeviceContext->PSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
								break;
						}
						break;
					}

					case Rhi::ResourceType::SAMPLER_STATE:
					{
						ID3D11SamplerState* d3d11SamplerState = static_cast<const SamplerState*>(resource)->getD3D11SamplerState();
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Rhi::ShaderVisibility::ALL:
								mD3D11DeviceContext->VSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->HSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->DSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->GSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->PSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::VERTEX:
								mD3D11DeviceContext->VSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
								// "hull shader" in Direct3D terminology
								mD3D11DeviceContext->HSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
								// "domain shader" in Direct3D terminology
								mD3D11DeviceContext->DSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::GEOMETRY:
								mD3D11DeviceContext->GSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology
								mD3D11DeviceContext->PSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::COMPUTE:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 compute shader visibility")
								break;

							case Rhi::ShaderVisibility::ALL_GRAPHICS:
								mD3D11DeviceContext->VSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->HSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->DSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->GSSetSamplers(startSlot, 1, &d3d11SamplerState);
								mD3D11DeviceContext->PSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;
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
					case Rhi::ResourceType::INDIRECT_BUFFER:
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
						RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 RHI implementation resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D11Rhi::setGraphicsVertexArray(Rhi::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage
		if (nullptr != vertexArray)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *vertexArray)

			// Begin debug event
			RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

			static_cast<VertexArray*>(vertexArray)->setDirect3DIASetInputLayoutAndStreamSource(*mD3D11DeviceContext);

			// End debug event
			RHI_END_DEBUG_EVENT(this)
		}
		else
		{
			// Set no Direct3D 11 input layout
			mD3D11DeviceContext->IASetInputLayout(nullptr);
			mCurrentGraphicsPipelineState.d3d11InputLayout = nullptr;
		}
	}

	void Direct3D11Rhi::setGraphicsViewports(uint32_t numberOfViewports, const Rhi::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Direct3D 11 rasterizer state viewports")

		// Set the Direct3D 11 viewports
		// -> "Rhi::Viewport" directly maps to Direct3D 11, do not change it
		// -> Let Direct3D 11 perform the index validation for us (the Direct3D 11 debug features are pretty good)
		static_assert(sizeof(Rhi::Viewport) == sizeof(D3D11_VIEWPORT), "Direct3D 11 structure mismatch detected");
		mD3D11DeviceContext->RSSetViewports(numberOfViewports, reinterpret_cast<const D3D11_VIEWPORT*>(viewports));
	}

	void Direct3D11Rhi::setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Direct3D 11 rasterizer state scissor rectangles")

		// Set the Direct3D 11 scissor rectangles
		// -> "Rhi::ScissorRectangle" directly maps to Direct3D 9 & 10 & 11, do not change it
		// -> Let Direct3D 11 perform the index validation for us (the Direct3D 11 debug features are pretty good)
		static_assert(sizeof(Rhi::ScissorRectangle) == sizeof(D3D11_RECT), "Direct3D 11 structure mismatch detected");
		mD3D11DeviceContext->RSSetScissorRects(numberOfScissorRectangles, reinterpret_cast<const D3D11_RECT*>(scissorRectangles));
	}

	void Direct3D11Rhi::setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget)
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
					// Release reference
					mRenderTarget->releaseReference();
				}

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Evaluate the render target type
				switch (mRenderTarget->getResourceType())
				{
					case Rhi::ResourceType::SWAP_CHAIN:
					{
						// Get the Direct3D 11 swap chain instance
						const SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

						// Direct3D 11 needs a pointer to a pointer, so give it one
						ID3D11RenderTargetView* d3d11RenderTargetView = swapChain->getD3D11RenderTargetView();
						mD3D11DeviceContext->OMSetRenderTargets(1, &d3d11RenderTargetView, swapChain->getD3D11DepthStencilView());
						break;
					}

					case Rhi::ResourceType::FRAMEBUFFER:
					{
						// Get the Direct3D 11 framebuffer instance
						const Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Set the Direct3D 11 render targets
						mD3D11DeviceContext->OMSetRenderTargets(framebuffer->getNumberOfColorTextures(), framebuffer->getD3D11RenderTargetViews(), framebuffer->getD3D11DepthStencilView());
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
			}
			else
			{
				// Set the Direct3D 11 render targets
				mD3D11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					mRenderTarget->releaseReference();
					mRenderTarget = nullptr;
				}
			}
		}
	}

	void Direct3D11Rhi::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Unlike Direct3D 9, OpenGL or OpenGL ES 3, Direct3D 11 clears a given render target view and not the currently bound

		// Sanity check
		RHI_ASSERT(mContext, z >= 0.0f && z <= 1.0f, "The Direct3D 11 clear graphics z value must be between [0, 1] (inclusive)")

		// Begin debug event
		RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

		// Render target set?
		if (nullptr != mRenderTarget)
		{
			// Evaluate the render target type
			switch (mRenderTarget->getResourceType())
			{
				case Rhi::ResourceType::SWAP_CHAIN:
				{
					// Get the Direct3D 11 swap chain instance
					SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

					// Clear the Direct3D 11 render target view?
					if (clearFlags & Rhi::ClearFlag::COLOR)
					{
						mD3D11DeviceContext->ClearRenderTargetView(swapChain->getD3D11RenderTargetView(), color);
					}

					// Clear the Direct3D 11 depth stencil view?
					if (nullptr != swapChain->getD3D11DepthStencilView())
					{
						// Get the Direct3D 11 clear flags
						UINT direct3D11ClearFlags = (clearFlags & Rhi::ClearFlag::DEPTH) ? D3D11_CLEAR_DEPTH : 0u;
						if (clearFlags & Rhi::ClearFlag::STENCIL)
						{
							direct3D11ClearFlags |= D3D11_CLEAR_STENCIL;
						}
						if (0 != direct3D11ClearFlags)
						{
							// Clear the Direct3D 11 depth stencil view
							mD3D11DeviceContext->ClearDepthStencilView(swapChain->getD3D11DepthStencilView(), direct3D11ClearFlags, z, static_cast<UINT8>(stencil));
						}
					}
					break;
				}

				case Rhi::ResourceType::FRAMEBUFFER:
				{
					// Get the Direct3D 11 framebuffer instance
					Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

					// Clear all Direct3D 11 render target views?
					if (clearFlags & Rhi::ClearFlag::COLOR)
					{
						// Loop through all Direct3D 11 render target views
						ID3D11RenderTargetView** d3d11RenderTargetViewsEnd = framebuffer->getD3D11RenderTargetViews() + framebuffer->getNumberOfColorTextures();
						for (ID3D11RenderTargetView** d3d11RenderTargetView = framebuffer->getD3D11RenderTargetViews(); d3d11RenderTargetView < d3d11RenderTargetViewsEnd; ++d3d11RenderTargetView)
						{
							// Valid Direct3D 11 render target view?
							if (nullptr != *d3d11RenderTargetView)
							{
								mD3D11DeviceContext->ClearRenderTargetView(*d3d11RenderTargetView, color);
							}
						}
					}

					// Clear the Direct3D 11 depth stencil view?
					if (nullptr != framebuffer->getD3D11DepthStencilView())
					{
						// Get the Direct3D 11 clear flags
						UINT direct3D11ClearFlags = (clearFlags & Rhi::ClearFlag::DEPTH) ? D3D11_CLEAR_DEPTH : 0u;
						if (clearFlags & Rhi::ClearFlag::STENCIL)
						{
							direct3D11ClearFlags |= D3D11_CLEAR_STENCIL;
						}
						if (0 != direct3D11ClearFlags)
						{
							// Clear the Direct3D 11 depth stencil view
							mD3D11DeviceContext->ClearDepthStencilView(framebuffer->getD3D11DepthStencilView(), direct3D11ClearFlags, z, static_cast<UINT8>(stencil));
						}
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
		}
		else
		{
			// In case no render target is currently set we don't have to do anything in here
		}

		// End debug event
		RHI_END_DEBUG_EVENT(this)
	}

	void Direct3D11Rhi::drawGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 11 draws must not be zero")

		// Draw indirect
		ID3D11Buffer* d3D11Buffer = static_cast<const IndirectBuffer&>(indirectBuffer).getD3D11Buffer();
		if (1 == numberOfDraws)
		{
			mD3D11DeviceContext->DrawInstancedIndirect(d3D11Buffer, indirectBufferOffset);
		}
		else if (numberOfDraws > 1)
		{
			// Emulate multi-draw-indirect
			#ifdef RHI_DEBUG
				beginDebugEvent("Multi-draw-indirect emulation");
			#endif
			for (uint32_t i = 0; i < numberOfDraws; ++i)
			{
				mD3D11DeviceContext->DrawInstancedIndirect(d3D11Buffer, indirectBufferOffset);
				indirectBufferOffset += sizeof(Rhi::DrawArguments);
			}
			#ifdef RHI_DEBUG
				endDebugEvent();
			#endif
		}
	}

	void Direct3D11Rhi::drawGraphicsAgs(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 11 draws must not be zero")
		RHI_ASSERT(mContext, nullptr != agsDriverExtensionsDX11_MultiDrawInstancedIndirect, "Direct3D 11: AMD AGS function \"agsDriverExtensionsDX11_MultiDrawInstancedIndirect()\" not found")

		// Draw indirect
		ID3D11Buffer* d3D11Buffer = static_cast<const IndirectBuffer&>(indirectBuffer).getD3D11Buffer();
		if (1 == numberOfDraws)
		{
			mD3D11DeviceContext->DrawInstancedIndirect(d3D11Buffer, indirectBufferOffset);
		}
		else if (numberOfDraws > 1)
		{
			// AMD: "agsDriverExtensionsDX11_MultiDrawInstancedIndirect()" - https://gpuopen-librariesandsdks.github.io/ags/group__mdi.html
			agsDriverExtensionsDX11_MultiDrawInstancedIndirect(mDirect3D11RuntimeLinking->getAgsContext(), mD3D11DeviceContext, numberOfDraws, d3D11Buffer, indirectBufferOffset, sizeof(Rhi::DrawArguments));
		}
	}

	void Direct3D11Rhi::drawGraphicsNvApi(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 11 draws must not be zero")
		RHI_ASSERT(mContext, nullptr != NvAPI_D3D11_MultiDrawInstancedIndirect, "Direct3D 11: NvAPI function \"NvAPI_D3D11_MultiDrawInstancedIndirect()\" not found")

		// Draw indirect
		ID3D11Buffer* d3D11Buffer = static_cast<const IndirectBuffer&>(indirectBuffer).getD3D11Buffer();
		if (1 == numberOfDraws)
		{
			mD3D11DeviceContext->DrawInstancedIndirect(d3D11Buffer, indirectBufferOffset);
		}
		else if (numberOfDraws > 1)
		{
			// NVIDIA: "NvAPI_D3D11_MultiDrawInstancedIndirect()" - http://docs.nvidia.com/gameworks/content/gameworkslibrary/coresdk/nvapi/group__dx.html#gaf417228a716d10efcb29fa592795f160
			NvAPI_D3D11_MultiDrawInstancedIndirect(mD3D11DeviceContext, numberOfDraws, d3D11Buffer, indirectBufferOffset, sizeof(Rhi::DrawArguments));
		}
	}

	void Direct3D11Rhi::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The Direct3D 11 emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 11 draws must not be zero")

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

			// Draw
			if (drawArguments.instanceCount > 1 || drawArguments.startInstanceLocation > 0)
			{
				// With instancing
				mD3D11DeviceContext->DrawInstanced(
					drawArguments.vertexCountPerInstance,	// Vertex count per instance (UINT)
					drawArguments.instanceCount,			// Instance count (UINT)
					drawArguments.startVertexLocation,		// Start vertex location (UINT)
					drawArguments.startInstanceLocation		// Start instance location (UINT)
				);
			}
			else
			{
				// Without instancing
				mD3D11DeviceContext->Draw(
					drawArguments.vertexCountPerInstance,	// Vertex count (UINT)
					drawArguments.startVertexLocation		// Start index location (UINT)
				);
			}

			// Advance
			emulationData += sizeof(Rhi::DrawArguments);
		}
		#ifdef RHI_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void Direct3D11Rhi::drawIndexedGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 11 draws must not be zero")

		// Draw indirect
		ID3D11Buffer* d3D11Buffer = static_cast<const IndirectBuffer&>(indirectBuffer).getD3D11Buffer();
		if (1 == numberOfDraws)
		{
			mD3D11DeviceContext->DrawIndexedInstancedIndirect(d3D11Buffer, indirectBufferOffset);
		}
		else if (numberOfDraws > 1)
		{
			// Emulate multi-draw-indirect
			#ifdef RHI_DEBUG
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			#endif
			for (uint32_t i = 0; i < numberOfDraws; ++i)
			{
				mD3D11DeviceContext->DrawIndexedInstancedIndirect(d3D11Buffer, indirectBufferOffset);
				indirectBufferOffset += sizeof(Rhi::DrawIndexedArguments);
			}
			#ifdef RHI_DEBUG
				endDebugEvent();
			#endif
		}
	}

	void Direct3D11Rhi::drawIndexedGraphicsAgs(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 11 draws must not be zero")
		RHI_ASSERT(mContext, nullptr != agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect, "Direct3D 11: NvAPI function \"agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect()\" not found")

		// Draw indirect
		ID3D11Buffer* d3D11Buffer = static_cast<const IndirectBuffer&>(indirectBuffer).getD3D11Buffer();
		if (1 == numberOfDraws)
		{
			mD3D11DeviceContext->DrawIndexedInstancedIndirect(d3D11Buffer, indirectBufferOffset);
		}
		else if (numberOfDraws > 1)
		{
			// AMD: "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect()" - https://gpuopen-librariesandsdks.github.io/ags/group__mdi.html
			agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect(mDirect3D11RuntimeLinking->getAgsContext(), mD3D11DeviceContext, numberOfDraws, d3D11Buffer, indirectBufferOffset, sizeof(Rhi::DrawIndexedArguments));
		}
	}

	void Direct3D11Rhi::drawIndexedGraphicsNvApi(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 11 draws must not be zero")
		RHI_ASSERT(mContext, nullptr != NvAPI_D3D11_MultiDrawIndexedInstancedIndirect, "Direct3D 11: NvAPI function \"NvAPI_D3D11_MultiDrawIndexedInstancedIndirect()\" not found")

		// Draw indirect
		ID3D11Buffer* d3D11Buffer = static_cast<const IndirectBuffer&>(indirectBuffer).getD3D11Buffer();
		if (1 == numberOfDraws)
		{
			mD3D11DeviceContext->DrawIndexedInstancedIndirect(d3D11Buffer, indirectBufferOffset);
		}
		else if (numberOfDraws > 1)
		{
			// NVIDIA: "NvAPI_D3D11_MultiDrawIndexedInstancedIndirect()" - http://docs.nvidia.com/gameworks/content/gameworkslibrary/coresdk/nvapi/group__dx.html#ga04cbd1b776a391e45d38377bd3156f9e
			NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(mD3D11DeviceContext, numberOfDraws, d3D11Buffer, indirectBufferOffset, sizeof(Rhi::DrawIndexedArguments));
		}
	}

	void Direct3D11Rhi::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The Direct3D 11 emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 11 draws must not be zero")

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
			const Rhi::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Rhi::DrawIndexedArguments*>(emulationData);

			// Draw
			if (drawIndexedArguments.instanceCount > 1 || drawIndexedArguments.startInstanceLocation > 0)
			{
				// With instancing
				mD3D11DeviceContext->DrawIndexedInstanced(
					drawIndexedArguments.indexCountPerInstance,	// Index count per instance (UINT)
					drawIndexedArguments.instanceCount,			// Instance count (UINT)
					drawIndexedArguments.startIndexLocation,	// Start index location (UINT)
					drawIndexedArguments.baseVertexLocation,	// Base vertex location (INT)
					drawIndexedArguments.startInstanceLocation	// Start instance location (UINT)
				);
			}
			else
			{
				// Without instancing
				mD3D11DeviceContext->DrawIndexed(
					drawIndexedArguments.indexCountPerInstance,	// Index count (UINT)
					drawIndexedArguments.startIndexLocation,	// Start index location (UINT)
					drawIndexedArguments.baseVertexLocation		// Base vertex location (INT)
				);
			}

			// Advance
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
	void Direct3D11Rhi::setComputeRootSignature(Rhi::IRootSignature* rootSignature)
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

	void Direct3D11Rhi::setComputePipelineState(Rhi::IComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *computePipelineState)

			// Set compute pipeline state
			ID3D11ComputeShader* d3d11ComputeShader = static_cast<const ComputePipelineState*>(computePipelineState)->getD3D11ComputeShader();
			if (mD3d11ComputeShader != d3d11ComputeShader)
			{
				mD3d11ComputeShader = d3d11ComputeShader;
				mD3D11DeviceContext->CSSetShader(mD3d11ComputeShader, nullptr, 0);
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D11Rhi::setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			if (nullptr == mComputeRootSignature)
			{
				RHI_LOG(mContext, CRITICAL, "No Direct3D 11 RHI implementation compute root signature set")
				return;
			}
			const Rhi::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation root parameter index is out of bounds")
				return;
			}
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Rhi::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RHI_LOG(mContext, CRITICAL, "The Direct3D 11 RHI implementation descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Set compute resource group
			const ResourceGroup* d3d11ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = d3d11ResourceGroup->getNumberOfResources();
			Rhi::IResource** resources = d3d11ResourceGroup->getResources();
			const Rhi::RootParameter& rootParameter = mComputeRootSignature->getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				const Rhi::IResource* resource = *resources;
				RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid Direct3D 11 descriptor ranges")
				const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Rhi::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Rhi::ResourceType::UNIFORM_BUFFER:
					{
						ID3D11Buffer* d3d11Buffers = static_cast<const UniformBuffer*>(resource)->getD3D11Buffer();
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Rhi::ShaderVisibility::VERTEX:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 vertex shader visibility")
								break;

							case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation control shader visibility")
								break;

							case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation evaluation shader visibility")
								break;

							case Rhi::ShaderVisibility::GEOMETRY:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 geometry shader visibility")
								break;

							case Rhi::ShaderVisibility::FRAGMENT:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 fragment shader visibility")
								break;

							case Rhi::ShaderVisibility::ALL:
							case Rhi::ShaderVisibility::COMPUTE:
								mD3D11DeviceContext->CSSetConstantBuffers(startSlot, 1, &d3d11Buffers);
								break;

							case Rhi::ShaderVisibility::ALL_GRAPHICS:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 all graphics shader visibility")
								break;
						}
						break;
					}

					case Rhi::ResourceType::TEXTURE_BUFFER:
					case Rhi::ResourceType::STRUCTURED_BUFFER:
					case Rhi::ResourceType::TEXTURE_1D:
					case Rhi::ResourceType::TEXTURE_1D_ARRAY:
					case Rhi::ResourceType::TEXTURE_2D:
					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					case Rhi::ResourceType::TEXTURE_3D:
					case Rhi::ResourceType::TEXTURE_CUBE:
					{
						switch (descriptorRange.rangeType)
						{
							case Rhi::DescriptorRangeType::SRV:
							{
								ID3D11ShaderResourceView* d3d11ShaderResourceView = nullptr;
								switch (resourceType)
								{
									case Rhi::ResourceType::TEXTURE_BUFFER:
										d3d11ShaderResourceView = static_cast<const TextureBuffer*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::STRUCTURED_BUFFER:
										d3d11ShaderResourceView = static_cast<const StructuredBuffer*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::TEXTURE_1D:
										d3d11ShaderResourceView = static_cast<const Texture1D*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::TEXTURE_1D_ARRAY:
										d3d11ShaderResourceView = static_cast<const Texture1DArray*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::TEXTURE_2D:
										d3d11ShaderResourceView = static_cast<const Texture2D*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::TEXTURE_2D_ARRAY:
										d3d11ShaderResourceView = static_cast<const Texture2DArray*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::TEXTURE_3D:
										d3d11ShaderResourceView = static_cast<const Texture3D*>(resource)->getD3D11ShaderResourceView();
										break;

									case Rhi::ResourceType::TEXTURE_CUBE:
										d3d11ShaderResourceView = static_cast<const TextureCube*>(resource)->getD3D11ShaderResourceView();
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
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 RHI implementation resource type")
										break;
								}
								const UINT startSlot = descriptorRange.baseShaderRegister;
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::VERTEX:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 vertex shader visibility")
										break;

									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation control shader visibility")
										break;

									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation evaluation shader visibility")
										break;

									case Rhi::ShaderVisibility::GEOMETRY:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 geometry shader visibility")
										break;

									case Rhi::ShaderVisibility::FRAGMENT:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 fragment shader visibility")
										break;

									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::COMPUTE:
										mD3D11DeviceContext->CSSetShaderResources(startSlot, 1, &d3d11ShaderResourceView);
										break;

									case Rhi::ShaderVisibility::ALL_GRAPHICS:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 all graphics shader visibility")
										break;
								}
								break;
							}

							case Rhi::DescriptorRangeType::UAV:
							{
								ID3D11UnorderedAccessView* d3d11UnorderedAccessView = nullptr;
								switch (resourceType)
								{
									case Rhi::ResourceType::TEXTURE_BUFFER:
										d3d11UnorderedAccessView = static_cast<const TextureBuffer*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::STRUCTURED_BUFFER:
										d3d11UnorderedAccessView = static_cast<const StructuredBuffer*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::TEXTURE_1D:
										d3d11UnorderedAccessView = static_cast<const Texture1D*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::TEXTURE_1D_ARRAY:
										d3d11UnorderedAccessView = static_cast<const Texture1DArray*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::TEXTURE_2D:
										d3d11UnorderedAccessView = static_cast<const Texture2D*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::TEXTURE_2D_ARRAY:
										d3d11UnorderedAccessView = static_cast<const Texture2DArray*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::TEXTURE_3D:
										d3d11UnorderedAccessView = static_cast<const Texture3D*>(resource)->getD3D11UnorderedAccessView();
										break;

									case Rhi::ResourceType::TEXTURE_CUBE:
										d3d11UnorderedAccessView = static_cast<const TextureCube*>(resource)->getD3D11UnorderedAccessView();
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
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 RHI implementation resource type")
										break;
								}
								const UINT startSlot = descriptorRange.baseShaderRegister;
								switch (descriptorRange.shaderVisibility)
								{
									case Rhi::ShaderVisibility::VERTEX:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 vertex shader visibility")
										break;

									case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation control shader visibility")
										break;

									case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation evaluation shader visibility")
										break;

									case Rhi::ShaderVisibility::GEOMETRY:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 geometry shader visibility")
										break;

									case Rhi::ShaderVisibility::FRAGMENT:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 fragment shader visibility")
										break;

									case Rhi::ShaderVisibility::ALL:
									case Rhi::ShaderVisibility::COMPUTE:
										mD3D11DeviceContext->CSSetUnorderedAccessViews(startSlot, 1, &d3d11UnorderedAccessView, nullptr);
										break;

									case Rhi::ShaderVisibility::ALL_GRAPHICS:
										RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 all graphics shader visibility")
										break;
								}
								break;
							}

							case Rhi::DescriptorRangeType::UBV:
							case Rhi::DescriptorRangeType::SAMPLER:
							case Rhi::DescriptorRangeType::NUMBER_OF_RANGE_TYPES:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 descriptor range type")
								break;
						}
						break;
					}

					case Rhi::ResourceType::VERTEX_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "Direct3D 11 vertex buffer must bound at SRV or UAV descriptor range type")
						RHI_ASSERT(mContext, Rhi::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Rhi::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "Direct3D 11 descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")
						if (Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType)
						{
							ID3D11ShaderResourceView* d3d11ShaderResourceView = static_cast<const VertexBuffer*>(resource)->getD3D11ShaderResourceView();
							mD3D11DeviceContext->CSSetShaderResources(descriptorRange.baseShaderRegister, 1, &d3d11ShaderResourceView);
						}
						else
						{
							// Set UAV
							ID3D11UnorderedAccessView* d3d11UnorderedAccessView = static_cast<const VertexBuffer*>(resource)->getD3D11UnorderedAccessView();
							mD3D11DeviceContext->CSSetUnorderedAccessViews(descriptorRange.baseShaderRegister, 1, &d3d11UnorderedAccessView, nullptr);
						}
						break;
					}

					case Rhi::ResourceType::INDEX_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "Direct3D 11 index buffer must bound at SRV or UAV descriptor range type")
						RHI_ASSERT(mContext, Rhi::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Rhi::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "Direct3D 11 descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")
						if (Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType)
						{
							ID3D11ShaderResourceView* d3d11ShaderResourceView = static_cast<const IndexBuffer*>(resource)->getD3D11ShaderResourceView();
							mD3D11DeviceContext->CSSetShaderResources(descriptorRange.baseShaderRegister, 1, &d3d11ShaderResourceView);
						}
						else
						{
							ID3D11UnorderedAccessView* d3d11UnorderedAccessView = static_cast<const IndexBuffer*>(resource)->getD3D11UnorderedAccessView();
							mD3D11DeviceContext->CSSetUnorderedAccessViews(descriptorRange.baseShaderRegister, 1, &d3d11UnorderedAccessView, nullptr);
						}
						break;
					}

					case Rhi::ResourceType::INDIRECT_BUFFER:
					{
						RHI_ASSERT(mContext, Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType || Rhi::DescriptorRangeType::UAV == descriptorRange.rangeType, "Direct3D 11 indirect buffer must bound at SRV or UAV descriptor range type")
						RHI_ASSERT(mContext, Rhi::ShaderVisibility::ALL == descriptorRange.shaderVisibility || Rhi::ShaderVisibility::COMPUTE == descriptorRange.shaderVisibility, "Direct3D 11 descriptor range shader visibility must be \"ALL\" or \"COMPUTE\"")
						if (Rhi::DescriptorRangeType::SRV == descriptorRange.rangeType)
						{
							ID3D11ShaderResourceView* d3d11ShaderResourceView = static_cast<const IndirectBuffer*>(resource)->getD3D11ShaderResourceView();
							mD3D11DeviceContext->CSSetShaderResources(descriptorRange.baseShaderRegister, 1, &d3d11ShaderResourceView);
						}
						else
						{
							ID3D11UnorderedAccessView* d3d11UnorderedAccessView = static_cast<const IndirectBuffer*>(resource)->getD3D11UnorderedAccessView();
							mD3D11DeviceContext->CSSetUnorderedAccessViews(descriptorRange.baseShaderRegister, 1, &d3d11UnorderedAccessView, nullptr);
						}
						break;
					}

					case Rhi::ResourceType::SAMPLER_STATE:
					{
						ID3D11SamplerState* d3d11SamplerState = static_cast<const SamplerState*>(resource)->getD3D11SamplerState();
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Rhi::ShaderVisibility::VERTEX:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 vertex shader visibility")
								break;

							case Rhi::ShaderVisibility::TESSELLATION_CONTROL:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation control shader visibility")
								break;

							case Rhi::ShaderVisibility::TESSELLATION_EVALUATION:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 tessellation evaluation shader visibility")
								break;

							case Rhi::ShaderVisibility::GEOMETRY:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 geometry shader visibility")
								break;

							case Rhi::ShaderVisibility::FRAGMENT:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 fragment shader visibility")
								break;

							case Rhi::ShaderVisibility::ALL:
							case Rhi::ShaderVisibility::COMPUTE:
								mD3D11DeviceContext->CSSetSamplers(startSlot, 1, &d3d11SamplerState);
								break;

							case Rhi::ShaderVisibility::ALL_GRAPHICS:
								RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 all graphics shader visibility")
								break;
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
					case Rhi::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Rhi::ResourceType::COMPUTE_PIPELINE_STATE:
					case Rhi::ResourceType::VERTEX_SHADER:
					case Rhi::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Rhi::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Rhi::ResourceType::GEOMETRY_SHADER:
					case Rhi::ResourceType::FRAGMENT_SHADER:
					case Rhi::ResourceType::COMPUTE_SHADER:
						RHI_LOG(mContext, CRITICAL, "Invalid Direct3D 11 RHI implementation resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D11Rhi::dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		mD3D11DeviceContext->Dispatch(groupCountX, groupCountY, groupCountZ);
		{ // TODO(co) Compute shader: "D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing CS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #2097316: DEVICE_CSSETSHADERRESOURCES_HAZARD]"
			ID3D11UnorderedAccessView* d3d11UnorderedAccessView = nullptr;
			for (uint32_t i = 0; i < 8; ++i)
			{
				mD3D11DeviceContext->CSSetUnorderedAccessViews(i, 1, &d3d11UnorderedAccessView, nullptr);
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void Direct3D11Rhi::resolveMultisampleFramebuffer(Rhi::IRenderTarget& destinationRenderTarget, Rhi::IFramebuffer& sourceMultisampleFramebuffer)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, destinationRenderTarget)
		RHI_MATCH_CHECK(*this, sourceMultisampleFramebuffer)

		// Evaluate the render target type
		switch (destinationRenderTarget.getResourceType())
		{
			case Rhi::ResourceType::SWAP_CHAIN:
			{
				// Get the Direct3D 11 swap chain instance
				// TODO(co) Implement me, not that important in practice so not directly implemented
				// SwapChain& swapChain = static_cast<SwapChain&>(destinationRenderTarget);
				break;
			}

			case Rhi::ResourceType::FRAMEBUFFER:
			{
				// Get the Direct3D 11 framebuffer instances
				const Framebuffer& direct3D11DestinationFramebuffer = static_cast<const Framebuffer&>(destinationRenderTarget);
				const Framebuffer& direct3D11SourceMultisampleFramebuffer = static_cast<const Framebuffer&>(sourceMultisampleFramebuffer);

				// Process all Direct3D 11 render target textures
				if (direct3D11DestinationFramebuffer.getNumberOfColorTextures() > 0 && direct3D11SourceMultisampleFramebuffer.getNumberOfColorTextures() > 0)
				{
					const uint32_t numberOfColorTextures = (direct3D11DestinationFramebuffer.getNumberOfColorTextures() < direct3D11SourceMultisampleFramebuffer.getNumberOfColorTextures()) ? direct3D11DestinationFramebuffer.getNumberOfColorTextures() : direct3D11SourceMultisampleFramebuffer.getNumberOfColorTextures();
					Rhi::ITexture** destinationTexture = direct3D11DestinationFramebuffer.getColorTextures();
					Rhi::ITexture** sourceTexture = direct3D11SourceMultisampleFramebuffer.getColorTextures();
					Rhi::ITexture** sourceTextureEnd = sourceTexture + numberOfColorTextures;
					for (; sourceTexture < sourceTextureEnd; ++sourceTexture, ++destinationTexture)
					{
						// Valid Direct3D 11 render target views?
						if (nullptr != *destinationTexture && nullptr != *sourceTexture)
						{
							const Texture2D* d3d11DestinationTexture2D = static_cast<const Texture2D*>(*destinationTexture);
							const Texture2D* d3d11SourceTexture2D = static_cast<const Texture2D*>(*sourceTexture);
							mD3D11DeviceContext->ResolveSubresource(d3d11DestinationTexture2D->getD3D11Texture2D(), D3D11CalcSubresource(0, 0, 1), d3d11SourceTexture2D->getD3D11Texture2D(), D3D11CalcSubresource(0, 0, 1), Mapping::getDirect3D11Format(d3d11DestinationTexture2D->getTextureFormat()));
						}
					}
				}

				// Process Direct3D 11 depth stencil texture
				if (nullptr != direct3D11DestinationFramebuffer.getDepthStencilTexture() && nullptr != direct3D11SourceMultisampleFramebuffer.getDepthStencilTexture())
				{
					const Texture2D* d3d11DestinationTexture2D = static_cast<const Texture2D*>(direct3D11DestinationFramebuffer.getDepthStencilTexture());
					const Texture2D* d3d11SourceTexture2D = static_cast<const Texture2D*>(direct3D11SourceMultisampleFramebuffer.getDepthStencilTexture());
					mD3D11DeviceContext->ResolveSubresource(d3d11DestinationTexture2D->getD3D11Texture2D(), D3D11CalcSubresource(0, 0, 1), d3d11SourceTexture2D->getD3D11Texture2D(), D3D11CalcSubresource(0, 0, 1), Mapping::getDirect3D11Format(d3d11DestinationTexture2D->getTextureFormat()));
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
	}

	void Direct3D11Rhi::copyResource(Rhi::IResource& destinationResource, Rhi::IResource& sourceResource)
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
					// Get the Direct3D 11 texture 2D instances
					const Texture2D& direct3D11DestinationTexture2D = static_cast<const Texture2D&>(destinationResource);
					const Texture2D& direct3D11SourceTexture2D = static_cast<const Texture2D&>(sourceResource);

					// Copy resource, but only the top-level mipmap
					mD3D11DeviceContext->CopySubresourceRegion(direct3D11DestinationTexture2D.getD3D11Texture2D(), 0, 0, 0, 0, direct3D11SourceTexture2D.getD3D11Texture2D(), 0, nullptr);
				}
				else
				{
					// Error!
					RHI_ASSERT(mContext, false, "Failed to copy Direct3D 11 resource")
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
	}

	void Direct3D11Rhi::generateMipmaps(Rhi::IResource& resource)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, resource)
		RHI_ASSERT(mContext, resource.getResourceType() == Rhi::ResourceType::TEXTURE_2D, "TODO(co) Mipmaps can only be generated for Direct3D 11 2D texture resources")

		// Generate mipmaps
		mD3D11DeviceContext->GenerateMips(static_cast<Texture2D&>(resource).getD3D11ShaderResourceView());
	}

	void Direct3D11Rhi::generateAsynchronousDeferredMipmaps(Rhi::ITexture& texture, ID3D11ShaderResourceView& d3d11ShaderResourceView)
	{
		if (mCapabilities.nativeMultithreading)
		{
			std::lock_guard<std::mutex> generateAsynchronousMipmapsForTexturesMutexLock(mGenerateAsynchronousMipmapsForTexturesMutex);
			mGenerateAsynchronousMipmapsForTextures.push_back(&texture);
			texture.addReference();
		}
		else
		{
			mD3D11DeviceContext->GenerateMips(&d3d11ShaderResourceView);
		}
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void Direct3D11Rhi::resetQueryPool([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, queryPool)
		RHI_ASSERT(mContext, firstQueryIndex < static_cast<const QueryPool&>(queryPool).getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")
		RHI_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= static_cast<const QueryPool&>(queryPool).getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")

		// Nothing to do in here for Direct3D 11
	}

	void Direct3D11Rhi::beginQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex, uint32_t)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& d3d11QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < d3d11QueryPool.getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")
		switch (d3d11QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
			case Rhi::QueryType::PIPELINE_STATISTICS:
				mD3D11DeviceContext->Begin(d3d11QueryPool.getD3D11Queries()[queryIndex]);
				break;

			case Rhi::QueryType::TIMESTAMP:
				RHI_ASSERT(mContext, false, "Direct3D 11 begin query isn't allowed for timestamp queries, use \"Rhi::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void Direct3D11Rhi::endQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& d3d11QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < d3d11QueryPool.getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")
		switch (d3d11QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
			case Rhi::QueryType::PIPELINE_STATISTICS:
				mD3D11DeviceContext->End(d3d11QueryPool.getD3D11Queries()[queryIndex]);
				break;

			case Rhi::QueryType::TIMESTAMP:
				RHI_ASSERT(mContext, false, "Direct3D 11 end query isn't allowed for timestamp queries, use \"Rhi::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void Direct3D11Rhi::writeTimestampQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& d3d11QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < d3d11QueryPool.getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")
		switch (d3d11QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				RHI_ASSERT(mContext, false, "Direct3D 11 write timestamp query isn't allowed for occlusion queries, use \"Rhi::Command::BeginQuery\" and \"Rhi::Command::EndQuery\" instead")
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				RHI_ASSERT(mContext, false, "Direct3D 11 write timestamp query isn't allowed for pipeline statistics queries, use \"Rhi::Command::BeginQuery\" and \"Rhi::Command::EndQuery\" instead")
				break;

			case Rhi::QueryType::TIMESTAMP:
				mD3D11DeviceContext->End(d3d11QueryPool.getD3D11Queries()[queryIndex]);
				break;
		}
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void Direct3D11Rhi::setDebugMarker(const char* name)
		{
			if (nullptr != mD3DUserDefinedAnnotation)
			{
				RHI_ASSERT(mContext, nullptr != name, "Direct3D 11 debug marker names must not be a null pointer")
				RHI_ASSERT(mContext, strlen(name) < 256, "Direct3D 11 debug marker names must not have more than 255 characters")
				wchar_t unicodeName[256];
				std::mbstowcs(unicodeName, name, 256);
				mD3DUserDefinedAnnotation->SetMarker(unicodeName);
			}
		}

		void Direct3D11Rhi::beginDebugEvent(const char* name)
		{
			if (nullptr != mD3DUserDefinedAnnotation)
			{
				RHI_ASSERT(mContext, nullptr != name, "Direct3D 11 debug event names must not be a null pointer")
				RHI_ASSERT(mContext, strlen(name) < 256, "Direct3D 11 debug event names must not have more than 255 characters")
				wchar_t unicodeName[256];
				std::mbstowcs(unicodeName, name, 256);
				mD3DUserDefinedAnnotation->BeginEvent(unicodeName);
			}
		}

		void Direct3D11Rhi::endDebugEvent()
		{
			if (nullptr != mD3DUserDefinedAnnotation)
			{
				mD3DUserDefinedAnnotation->EndEvent();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	bool Direct3D11Rhi::isDebugEnabled()
	{
		// Don't check for the "RHI_DEBUG" preprocessor definition, even if debug
		// is disabled it has to be possible to use this function for an additional security check
		// -> Maybe a debugger/profiler ignores the debug state
		// -> Maybe someone manipulated the binary to enable the debug state, adding a second check
		//    makes it a little bit more time consuming to hack the binary :D (but of course, this is no 100% security)
		return (nullptr != mD3DUserDefinedAnnotation && mD3DUserDefinedAnnotation->GetStatus() != 0);
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t Direct3D11Rhi::getNumberOfShaderLanguages() const
	{
		uint32_t numberOfShaderLanguages = 1;	// HLSL support is always there

		// Done, return the number of supported shader languages
		return numberOfShaderLanguages;
	}

	const char* Direct3D11Rhi::getShaderLanguageName([[maybe_unused]] uint32_t index) const
	{
		RHI_ASSERT(mContext, index < getNumberOfShaderLanguages(), "Direct3D 11: Shader language index is out-of-bounds")
		return ::detail::HLSL_NAME;
	}

	Rhi::IShaderLanguage* Direct3D11Rhi::getShaderLanguage(const char* shaderLanguageName)
	{
		// In case "shaderLanguage" is a null pointer, use the default shader language
		if (nullptr != shaderLanguageName)
		{
			// Optimization: Check for shader language name pointer match, first
			if (::detail::HLSL_NAME == shaderLanguageName || !stricmp(shaderLanguageName, ::detail::HLSL_NAME))
			{
				// If required, create the HLSL shader language instance right now
				if (nullptr == mShaderLanguageHlsl)
				{
					mShaderLanguageHlsl = RHI_NEW(mContext, ShaderLanguageHlsl)(*this);
					mShaderLanguageHlsl->addReference();	// Internal RHI reference
				}

				// Return the shader language instance
				return mShaderLanguageHlsl;
			}

			// Error!
			return nullptr;
		}

		// Return the HLSL shader language instance as default
		return getShaderLanguage(::detail::HLSL_NAME);
	}


	//[-------------------------------------------------------]
	//[ Resource creation                                     ]
	//[-------------------------------------------------------]
	Rhi::IRenderPass* Direct3D11Rhi::createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IQueryPool* Direct3D11Rhi::createQueryPool(Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		RHI_ASSERT(mContext, numberOfQueries > 0, "Direct3D 11: Number of queries mustn't be zero")
		return RHI_NEW(mContext, QueryPool)(*this, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::ISwapChain* Direct3D11Rhi::createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, renderPass)
		RHI_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Direct3D 11: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RHI_NEW(mContext, SwapChain)(renderPass, windowHandle RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IFramebuffer* Direct3D11Rhi::createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, renderPass)

		// Create the framebuffer
		return RHI_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IBufferManager* Direct3D11Rhi::createBufferManager()
	{
		return RHI_NEW(mContext, BufferManager)(*this);
	}

	Rhi::ITextureManager* Direct3D11Rhi::createTextureManager()
	{
		return RHI_NEW(mContext, TextureManager)(*this);
	}

	Rhi::IRootSignature* Direct3D11Rhi::createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RootSignature)(*this, rootSignature RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IGraphicsPipelineState* Direct3D11Rhi::createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "Direct3D 11: Invalid graphics pipeline state root signature")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "Direct3D 11: Invalid graphics pipeline state graphics program")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "Direct3D 11: Invalid graphics pipeline state render pass")

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

	Rhi::IComputePipelineState* Direct3D11Rhi::createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, rootSignature)
		RHI_MATCH_CHECK(*this, computeShader)

		// Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();

		// Create the compute pipeline state
		uint16_t id = 0;
		if (ComputePipelineStateMakeId.CreateID(id))
		{
			return RHI_NEW(mContext, ComputePipelineState)(*this, computeShader, id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		// Error: Ensure a correct reference counter behaviour
		computeShader.addReference();
		computeShader.releaseReference();
		return nullptr;
	}

	Rhi::ISamplerState* Direct3D11Rhi::createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, SamplerState)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool Direct3D11Rhi::map(Rhi::IResource& resource, uint32_t subresource, Rhi::MapType mapType, uint32_t mapFlags, Rhi::MappedSubresource& mappedSubresource)
	{
		// The "Rhi::MapType" values directly map to Direct3D 10 & 11 constants, do not change them
		// The "Rhi::MappedSubresource" structure directly maps to Direct3D 11, do not change it

		// Define helper macro
		#define TEXTURE_RESOURCE(type, typeClass) \
			case type: \
			{ \
				bool result = false; \
				RHI_BEGIN_DEBUG_EVENT_FUNCTION(this) \
				ID3D11Resource* d3d11Resource = nullptr; \
				static_cast<typeClass&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource); \
				if (nullptr != d3d11Resource) \
				{ \
					result = (S_OK == mD3D11DeviceContext->Map(d3d11Resource, subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource))); \
					d3d11Resource->Release(); \
				} \
				RHI_END_DEBUG_EVENT(this) \
				return result; \
			}

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
				return (S_OK == mD3D11DeviceContext->Map(static_cast<VertexBuffer&>(resource).getD3D11Buffer(), subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Rhi::ResourceType::INDEX_BUFFER:
				return (S_OK == mD3D11DeviceContext->Map(static_cast<IndexBuffer&>(resource).getD3D11Buffer(), subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Rhi::ResourceType::TEXTURE_BUFFER:
				return (S_OK == mD3D11DeviceContext->Map(static_cast<TextureBuffer&>(resource).getD3D11Buffer(), subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Rhi::ResourceType::STRUCTURED_BUFFER:
				return (S_OK == mD3D11DeviceContext->Map(static_cast<StructuredBuffer&>(resource).getD3D11Buffer(), subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Rhi::ResourceType::INDIRECT_BUFFER:
				return (S_OK == mD3D11DeviceContext->Map(static_cast<IndirectBuffer&>(resource).getStagingD3D11Buffer(), subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Rhi::ResourceType::UNIFORM_BUFFER:
				return (S_OK == mD3D11DeviceContext->Map(static_cast<UniformBuffer&>(resource).getD3D11Buffer(), subresource, static_cast<D3D11_MAP>(mapType), mapFlags, reinterpret_cast<D3D11_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D, Texture1D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D_ARRAY, Texture1DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D, Texture2D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D_ARRAY, Texture2DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_3D, Texture3D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_CUBE, TextureCube)

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

		// Undefine helper macro
		#undef TEXTURE_RESOURCE
	}

	void Direct3D11Rhi::unmap(Rhi::IResource& resource, uint32_t subresource)
	{
		// Define helper macro
		#define TEXTURE_RESOURCE(type, typeClass) \
			case type: \
			{ \
				ID3D11Resource* d3d11Resource = nullptr; \
				static_cast<typeClass&>(resource).getD3D11ShaderResourceView()->GetResource(&d3d11Resource); \
				if (nullptr != d3d11Resource) \
				{ \
					mD3D11DeviceContext->Unmap(d3d11Resource, subresource); \
					d3d11Resource->Release(); \
				} \
				break; \
			}

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
				mD3D11DeviceContext->Unmap(static_cast<VertexBuffer&>(resource).getD3D11Buffer(), subresource);
				break;

			case Rhi::ResourceType::INDEX_BUFFER:
				mD3D11DeviceContext->Unmap(static_cast<IndexBuffer&>(resource).getD3D11Buffer(), subresource);
				break;

			case Rhi::ResourceType::TEXTURE_BUFFER:
				mD3D11DeviceContext->Unmap(static_cast<TextureBuffer&>(resource).getD3D11Buffer(), subresource);
				break;

			case Rhi::ResourceType::STRUCTURED_BUFFER:
				mD3D11DeviceContext->Unmap(static_cast<StructuredBuffer&>(resource).getD3D11Buffer(), subresource);
				break;

			case Rhi::ResourceType::INDIRECT_BUFFER:
			{
				IndirectBuffer& indirectBuffer = static_cast<IndirectBuffer&>(resource);
				ID3D11Buffer* stagingD3D11Buffer = indirectBuffer.getStagingD3D11Buffer();
				mD3D11DeviceContext->Unmap(stagingD3D11Buffer, subresource);
				mD3D11DeviceContext->CopyResource(indirectBuffer.getD3D11Buffer(), stagingD3D11Buffer);
				break;
			}

			case Rhi::ResourceType::UNIFORM_BUFFER:
				mD3D11DeviceContext->Unmap(static_cast<UniformBuffer&>(resource).getD3D11Buffer(), subresource);
				break;

			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D, Texture1D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D_ARRAY, Texture1DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D, Texture2D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D_ARRAY, Texture2DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_3D, Texture3D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_CUBE, TextureCube)

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

		// Undefine helper macro
		#undef TEXTURE_RESOURCE
	}

	bool Direct3D11Rhi::getQueryPoolResults(Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, uint32_t strideInBytes, uint32_t queryResultFlags)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, queryPool)
		RHI_ASSERT(mContext, numberOfDataBytes >= sizeof(UINT64), "Direct3D 11 out-of-memory query access")
		RHI_ASSERT(mContext, 1 == numberOfQueries || strideInBytes > 0, "Direct3D 11 invalid stride in bytes")
		RHI_ASSERT(mContext, numberOfDataBytes >= strideInBytes * numberOfQueries, "Direct3D 11 out-of-memory query access")
		RHI_ASSERT(mContext, nullptr != data, "Direct3D 11 out-of-memory query access")
		RHI_ASSERT(mContext, numberOfQueries > 0, "Direct3D 11 number of queries mustn't be zero")

		// Query pool type dependent processing
		bool resultAvailable = true;
		const QueryPool& d3d11QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, firstQueryIndex < d3d11QueryPool.getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")
		RHI_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= d3d11QueryPool.getNumberOfQueries(), "Direct3D 11 out-of-bounds query index")
		const bool waitForResult = ((queryResultFlags & Rhi::QueryResultFlags::WAIT) != 0);
		switch (d3d11QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
			case Rhi::QueryType::TIMESTAMP:	// TODO(co) Convert time to nanoseconds, see e.g. http://reedbeta.com/blog/gpu-profiling-101/
			{
				uint8_t* currentData = data;
				ID3D11Query** d3D11Queries = d3d11QueryPool.getD3D11Queries();
				for (uint32_t i = 0; i  < numberOfQueries; ++i)
				{
					HRESULT d3d11QueryResult = S_FALSE;
					do
					{
						d3d11QueryResult = mD3D11DeviceContext->GetData(d3D11Queries[firstQueryIndex + i], currentData, sizeof(UINT64), 0);
					}
					while (waitForResult && S_OK != d3d11QueryResult);
					if (S_FALSE == d3d11QueryResult)
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
			{
				static_assert(sizeof(Rhi::PipelineStatisticsQueryResult) == sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), "Direct3D 11 structure mismatch detected");
				RHI_ASSERT(mContext, numberOfDataBytes >= sizeof(Rhi::PipelineStatisticsQueryResult), "Direct3D 11 out-of-memory query access")
				RHI_ASSERT(mContext, 1 == numberOfQueries || strideInBytes >= sizeof(Rhi::PipelineStatisticsQueryResult), "Direct3D 11 out-of-memory query access")
				uint8_t* currentData = data;
				ID3D11Query** d3D11Queries = d3d11QueryPool.getD3D11Queries();
				for (uint32_t i = 0; i  < numberOfQueries; ++i)
				{
					HRESULT d3d11QueryResult = S_FALSE;
					do
					{
						d3d11QueryResult = mD3D11DeviceContext->GetData(d3D11Queries[firstQueryIndex + i], currentData, sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), 0);
					}
					while (waitForResult && S_OK != d3d11QueryResult);
					if (S_FALSE == d3d11QueryResult)
					{
						// Result not ready
						resultAvailable = false;
						break;
					}
					currentData += strideInBytes;
				}
				break;
			}
		}

		// Done
		return resultAvailable;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool Direct3D11Rhi::beginScene()
	{
		// Not required when using Direct3D 11

		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, false == mDebugBetweenBeginEndScene, "Direct3D 11: Begin scene was called while scene rendering is already in progress, missing end scene call?")
			mDebugBetweenBeginEndScene = true;
		#endif

		// Done
		return true;
	}

	void Direct3D11Rhi::submitCommandBuffer(const Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(mContext, !commandBuffer.isEmpty(), "The Direct3D 11 command buffer to execute mustn't be empty")

		// Generate asynchronous mipmaps for textures
		// -> For multithreading we could also use a deferred context, but in first tests there were random "FinishCommandList()"/"ExecuteCommandList()" state glitches
		//    when not fully resetting the context states. On the other hand, fully resetting the context states isn't recommended. Since we just need to be able to
		//    trigger the generation of mipmaps for textures asynchronously it's not really worth using those more complex deferred contexts for such a simple task.
		if (mCapabilities.nativeMultithreading)
		{
			std::lock_guard<std::mutex> generateAsynchronousMipmapsForTexturesMutexLock(mGenerateAsynchronousMipmapsForTexturesMutex);
			if (!mGenerateAsynchronousMipmapsForTextures.empty())
			{
				for (Rhi::ITexture* texture : mGenerateAsynchronousMipmapsForTextures)
				{
					ID3D11ShaderResourceView* d3d11ShaderResourceView = nullptr;
					switch (texture->getResourceType())
					{
						case Rhi::ResourceType::TEXTURE_1D:
							d3d11ShaderResourceView = static_cast<Texture1D*>(texture)->getD3D11ShaderResourceView();
							break;

						case Rhi::ResourceType::TEXTURE_1D_ARRAY:
							d3d11ShaderResourceView = static_cast<Texture1DArray*>(texture)->getD3D11ShaderResourceView();
							break;

						case Rhi::ResourceType::TEXTURE_2D:
							d3d11ShaderResourceView = static_cast<Texture2D*>(texture)->getD3D11ShaderResourceView();
							break;

						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
							d3d11ShaderResourceView = static_cast<Texture2DArray*>(texture)->getD3D11ShaderResourceView();
							break;

						case Rhi::ResourceType::TEXTURE_3D:
							d3d11ShaderResourceView = static_cast<Texture3D*>(texture)->getD3D11ShaderResourceView();
							break;

						case Rhi::ResourceType::TEXTURE_CUBE:
							d3d11ShaderResourceView = static_cast<TextureCube*>(texture)->getD3D11ShaderResourceView();
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
							RHI_ASSERT(mContext, false, "Direct3D 11: Invalid resource type")
							break;
					}
					RHI_ASSERT(mContext, nullptr != d3d11ShaderResourceView, "Direct3D 11: Invalid shader resource view")
					mD3D11DeviceContext->GenerateMips(d3d11ShaderResourceView);
					texture->releaseReference();
				}
				mGenerateAsynchronousMipmapsForTextures.clear();
			}
		}

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

	void Direct3D11Rhi::endScene()
	{
		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, true == mDebugBetweenBeginEndScene, "Direct3D 11: End scene was called while scene rendering isn't in progress, missing start scene call?")
			mDebugBetweenBeginEndScene = false;
		#endif

		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void Direct3D11Rhi::flush()
	{
		mD3D11DeviceContext->Flush();
	}

	void Direct3D11Rhi::finish()
	{
		// Create the Direct3D 11 query instance used for flush right now?
		if (nullptr == mD3D11QueryFlush)
		{
			D3D11_QUERY_DESC d3d11QueryDesc;
			d3d11QueryDesc.Query	 = D3D11_QUERY_EVENT;
			d3d11QueryDesc.MiscFlags = 0;
			FAILED_DEBUG_BREAK(mD3D11Device->CreateQuery(&d3d11QueryDesc, &mD3D11QueryFlush))

			#ifdef RHI_DEBUG
				// Set the debug name
				if (nullptr != mD3D11QueryFlush)
				{
					// No need to reset the previous private data, there shouldn't be any...
					FAILED_DEBUG_BREAK(mD3D11QueryFlush->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(__FUNCTION__)), __FUNCTION__))
				}
			#endif
		}
		if (nullptr != mD3D11QueryFlush)
		{
			// Perform the flush and wait
			mD3D11DeviceContext->End(mD3D11QueryFlush);
			mD3D11DeviceContext->Flush();
			BOOL result = FALSE;
			do
			{
				// Spin-wait
				FAILED_DEBUG_BREAK(mD3D11DeviceContext->GetData(mD3D11QueryFlush, &result, sizeof(BOOL), 0))
			} while (!result);
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void Direct3D11Rhi::initializeCapabilities()
	{
		// There are no Direct3D 11 device capabilities we could query on runtime, they depend on the chosen feature level
		// -> Have a look at "Devices -> Direct3D 11 on Downlevel Hardware -> Introduction" at MSDN http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx
		//    for a table with a list of the minimum resources supported by Direct3D 11 at the different feature levels

		{ // Get device name
			// Get DXGI adapter
			IDXGIDevice* dxgiDevice = nullptr;
			IDXGIAdapter* dxgiAdapter = nullptr;
			FAILED_DEBUG_BREAK(mD3D11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))
			FAILED_DEBUG_BREAK(dxgiDevice->GetAdapter(&dxgiAdapter))

			// The adapter contains a description like "AMD Radeon R9 200 Series"
			DXGI_ADAPTER_DESC dxgiAdapterDesc = {};
			FAILED_DEBUG_BREAK(dxgiAdapter->GetDesc(&dxgiAdapterDesc))

			// Convert UTF-16 string to UTF-8
			const size_t numberOfCharacters = _countof(mCapabilities.deviceName) - 1;
			::WideCharToMultiByte(CP_UTF8, 0, dxgiAdapterDesc.Description, static_cast<int>(wcslen(dxgiAdapterDesc.Description)), mCapabilities.deviceName, static_cast<int>(numberOfCharacters), nullptr, nullptr);
			mCapabilities.deviceName[numberOfCharacters] = '\0';

			// Release references
			dxgiAdapter->Release();
			dxgiDevice->Release();
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Rhi::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;

		// Evaluate the chosen feature level
		switch (mD3D11Device->GetFeatureLevel())
		{
			case D3D_FEATURE_LEVEL_9_1:
				// Maximum number of viewports (always at least 1)
				mCapabilities.maximumNumberOfViewports = 1;	// Direct3D 9 only supports a single viewport

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 1;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 2048;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 0;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

				// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
				mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 0;

				// Maximum indirect buffer size in bytes
				mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

				// Maximum number of multisamples (always at least 1, usually 8)
				mCapabilities.maximumNumberOfMultisamples = 1;	// Don't want to support the legacy DirectX 9 multisample support

				// Maximum anisotropy (always at least 1, usually 16)
				mCapabilities.maximumAnisotropy = 16;

				// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
				mCapabilities.instancedArrays = false;

				// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
				mCapabilities.drawInstanced = false;

				// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
				mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 9.1 has no tessellation support

				// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
				mCapabilities.maximumNumberOfGsOutputVertices = 0;	// Direct3D 9.1 has no geometry shader support
				break;

			case D3D_FEATURE_LEVEL_9_2:
				// Maximum number of viewports (always at least 1)
				mCapabilities.maximumNumberOfViewports = 1;	// Direct3D 9 only supports a single viewport

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 1;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 2048;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 0;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

				// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
				mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 0;

				// Maximum indirect buffer size in bytes
				mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

				// Maximum number of multisamples (always at least 1, usually 8)
				mCapabilities.maximumNumberOfMultisamples = 1;	// Don't want to support the legacy DirectX 9 multisample support

				// Maximum anisotropy (always at least 1, usually 16)
				mCapabilities.maximumAnisotropy = 16;

				// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
				mCapabilities.instancedArrays = false;

				// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
				mCapabilities.drawInstanced = false;

				// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
				mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 9.2 has no tessellation support

				// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
				mCapabilities.maximumNumberOfGsOutputVertices = 0;	// Direct3D 9.2 has no geometry shader support
				break;

			case D3D_FEATURE_LEVEL_9_3:
				// Maximum number of viewports (always at least 1)
				mCapabilities.maximumNumberOfViewports = 1;	// Direct3D 9 only supports a single viewport

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 4;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 4096;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 0;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

				// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
				mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 0;

				// Maximum indirect buffer size in bytes
				mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

				// Maximum number of multisamples (always at least 1, usually 8)
				mCapabilities.maximumNumberOfMultisamples = 1;	// Don't want to support the legacy DirectX 9 multisample support

				// Maximum anisotropy (always at least 1, usually 16)
				mCapabilities.maximumAnisotropy = 16;

				// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
				mCapabilities.instancedArrays = true;

				// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
				mCapabilities.drawInstanced = false;

				// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
				mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 9.3 has no tessellation support

				// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
				mCapabilities.maximumNumberOfGsOutputVertices = 0;	// Direct3D 9.3 has no geometry shader support
				break;

			case D3D_FEATURE_LEVEL_10_0:
				// Maximum number of viewports (always at least 1)
				mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 8192;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 512;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

				// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
				mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 128 * 1024 * 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention the texture buffer? Currently the OpenGL 3 minimum is used: 128 MiB.

				// Maximum indirect buffer size in bytes
				mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

				// Maximum number of multisamples (always at least 1, usually 8)
				mCapabilities.maximumNumberOfMultisamples = 8;

				// Maximum anisotropy (always at least 1, usually 16)
				mCapabilities.maximumAnisotropy = 16;

				// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
				mCapabilities.instancedArrays = true;

				// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
				mCapabilities.drawInstanced = true;

				// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
				mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 10 has no tessellation support

				// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
				mCapabilities.maximumNumberOfGsOutputVertices = 1024;
				break;

			case D3D_FEATURE_LEVEL_10_1:
				// Maximum number of viewports (always at least 1)
				mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 8192;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 512;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

				// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
				mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 128 * 1024 * 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention the texture buffer? Currently the OpenGL 3 minimum is used: 128 MiB.

				// Maximum indirect buffer size in bytes
				mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

				// Maximum number of multisamples (always at least 1, usually 8)
				mCapabilities.maximumNumberOfMultisamples = 8;

				// Maximum anisotropy (always at least 1, usually 16)
				mCapabilities.maximumAnisotropy = 16;

				// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
				mCapabilities.instancedArrays = true;

				// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
				mCapabilities.drawInstanced = true;

				// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
				mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 10.1 has no tessellation support

				// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
				mCapabilities.maximumNumberOfGsOutputVertices = 1024;
				break;

			case D3D_FEATURE_LEVEL_11_0:
			case D3D_FEATURE_LEVEL_11_1:
				// Maximum number of viewports (always at least 1)
				mCapabilities.maximumNumberOfViewports = D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;

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
				mCapabilities.maximumNumberOfMultisamples = 8;

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
				break;
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

		// Direct3D 11 has native multithreading
		mCapabilities.nativeMultithreading = true;

		// Direct3D 11 has shader bytecode support
		mCapabilities.shaderBytecode = true;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = true;
	}

	void Direct3D11Rhi::setGraphicsProgram(Rhi::IGraphicsProgram* graphicsProgram)
	{
		// Begin debug event
		RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

		if (nullptr != graphicsProgram)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsProgram)

			// Get shaders
			const GraphicsProgramHlsl*			    graphicsProgramHlsl				 = static_cast<GraphicsProgramHlsl*>(graphicsProgram);
			const VertexShaderHlsl*				    vertexShaderHlsl				 = graphicsProgramHlsl->getVertexShaderHlsl();
			const TessellationControlShaderHlsl*	tessellationControlShaderHlsl	 = graphicsProgramHlsl->getTessellationControlShaderHlsl();
			const TessellationEvaluationShaderHlsl* tessellationEvaluationShaderHlsl = graphicsProgramHlsl->getTessellationEvaluationShaderHlsl();
			const GeometryShaderHlsl*			    geometryShaderHlsl				 = graphicsProgramHlsl->getGeometryShaderHlsl();
			const FragmentShaderHlsl*			    fragmentShaderHlsl				 = graphicsProgramHlsl->getFragmentShaderHlsl();
			ID3D11VertexShader*   d3d11VertexShader   = (nullptr != vertexShaderHlsl)				  ? vertexShaderHlsl->getD3D11VertexShader()				 : nullptr;
			ID3D11HullShader*     d3d11HullShader     = (nullptr != tessellationControlShaderHlsl)	  ? tessellationControlShaderHlsl->getD3D11HullShader()		 : nullptr;
			ID3D11DomainShader*   d3d11DomainShader   = (nullptr != tessellationEvaluationShaderHlsl) ? tessellationEvaluationShaderHlsl->getD3D11DomainShader() : nullptr;
			ID3D11GeometryShader* d3d11GeometryShader = (nullptr != geometryShaderHlsl)				  ? geometryShaderHlsl->getD3D11GeometryShader()			 : nullptr;
			ID3D11PixelShader*	  d3d11PixelShader    = (nullptr != fragmentShaderHlsl)				  ? fragmentShaderHlsl->getD3D11PixelShader()				 : nullptr;

			// Set shaders
			if (mD3d11VertexShader != d3d11VertexShader)
			{
				mD3d11VertexShader = d3d11VertexShader;
				mD3D11DeviceContext->VSSetShader(mD3d11VertexShader, nullptr, 0);
			}
			if (mD3d11HullShader != d3d11HullShader)
			{
				mD3d11HullShader = d3d11HullShader;
				mD3D11DeviceContext->HSSetShader(mD3d11HullShader, nullptr, 0);
			}
			if (mD3d11DomainShader != d3d11DomainShader)
			{
				mD3d11DomainShader = d3d11DomainShader;
				mD3D11DeviceContext->DSSetShader(mD3d11DomainShader, nullptr, 0);
			}
			if (mD3d11GeometryShader != d3d11GeometryShader)
			{
				mD3d11GeometryShader = d3d11GeometryShader;
				mD3D11DeviceContext->GSSetShader(mD3d11GeometryShader, nullptr, 0);
			}
			if (mD3d11PixelShader != d3d11PixelShader)
			{
				mD3d11PixelShader = d3d11PixelShader;
				mD3D11DeviceContext->PSSetShader(mD3d11PixelShader, nullptr, 0);
			}
		}
		else
		{
			if (nullptr != mD3d11VertexShader)
			{
				mD3D11DeviceContext->VSSetShader(nullptr, nullptr, 0);
				mD3d11VertexShader = nullptr;
			}
			if (nullptr != mD3d11HullShader)
			{
				mD3D11DeviceContext->HSSetShader(nullptr, nullptr, 0);
				mD3d11HullShader = nullptr;
			}
			if (nullptr != mD3d11DomainShader)
			{
				mD3D11DeviceContext->DSSetShader(nullptr, nullptr, 0);
				mD3d11DomainShader = nullptr;
			}
			if (nullptr != mD3d11GeometryShader)
			{
				mD3D11DeviceContext->GSSetShader(nullptr, nullptr, 0);
				mD3d11GeometryShader = nullptr;
			}
			if (nullptr != mD3d11PixelShader)
			{
				mD3D11DeviceContext->PSSetShader(nullptr, nullptr, 0);
				mD3d11PixelShader = nullptr;
			}
		}

		// End debug event
		RHI_END_DEBUG_EVENT(this)
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D11Rhi




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RHI_DIRECT3D11_EXPORTS
	#define DIRECT3D11RHI_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define DIRECT3D11RHI_FUNCTION_EXPORT
#endif
DIRECT3D11RHI_FUNCTION_EXPORT Rhi::IRhi* createDirect3D11RhiInstance(const Rhi::Context& context)
{
	return RHI_NEW(context, Direct3D11Rhi::Direct3D11Rhi)(context);
}
#undef DIRECT3D11RHI_FUNCTION_EXPORT
