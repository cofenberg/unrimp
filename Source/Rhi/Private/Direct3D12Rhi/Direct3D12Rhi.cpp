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
*    Direct3D 12 RHI amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    Direct3D 12 runtime and Direct3D 12 capable graphics driver, nothing else.
*
*    == Preprocessor Definitions ==
*    - Set "RHI_DIRECT3D12_EXPORTS" as preprocessor definition when building this library as shared library
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




//[-------------------------------------------------------]
//[ Direct3D12Rhi/MakeID.h                                ]
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
//[ Direct3D11Rhi/D3D12.h                                 ]
//[-------------------------------------------------------]
/*
  We don't use the Direct3D headers from the DirectX SDK because there are several issues:
  - Licensing: It's not allowed to redistribute the Direct3D headers, meaning everyone would
    have to get them somehow before compiling this project
  - The Direct3D headers are somewhat chaotic and include tons of other headers.
    This slows down compilation and the more headers are included, the higher the risk of
    naming or redefinition conflicts.

    Do not include this header within headers which are usually used by users as well, do only
    use it inside cpp-files. It must still be possible that users of this RHI
    can use the Direct3D headers for features not covered by this RHI.
*/


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
struct DXGI_RGBA;
struct DXGI_RESIDENCY;
struct DXGI_MATRIX_3X2_F;
struct DXGI_SURFACE_DESC;
struct DXGI_ADAPTER_DESC;
struct DXGI_SHARED_RESOURCE;
struct DXGI_FRAME_STATISTICS;
struct DXGI_SWAP_CHAIN_DESC1;
struct DXGI_PRESENT_PARAMETERS;
struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC;
struct IDXGIOutput;
struct IDXGISurface;
struct IDXGIAdapter;
struct IDXGIAdapter1;
struct D3D12_HEAP_DESC;
struct D3D12_TILE_SHAPE;
struct D3D12_SAMPLER_DESC;
struct D3D12_DISCARD_REGION;
struct D3D12_PACKED_MIP_INFO;
struct D3D12_TILE_REGION_SIZE;
struct D3D12_TILE_RANGE_FLAGS;
struct D3D12_SUBRESOURCE_TILING;
struct D3D12_SO_DECLARATION_ENTRY;
struct D3D12_STREAM_OUTPUT_BUFFER_VIEW;
struct D3D12_TILED_RESOURCE_COORDINATE;
struct D3D12_UNORDERED_ACCESS_VIEW_DESC;
struct D3D12_COMPUTE_PIPELINE_STATE_DESC;
struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT;
struct ID3D12Heap;
struct ID3D12Resource;
struct ID3D12RootSignature;

// TODO(co) Direct3D 12 update
struct ID3D10Blob;
struct D3D10_SHADER_MACRO;
typedef __interface ID3DInclude *LPD3D10INCLUDE;
struct ID3DX12ThreadPump;
typedef DWORD D3DCOLOR;


//[-------------------------------------------------------]
//[ Definitions                                           ]
//[-------------------------------------------------------]
// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgiformat.h"
typedef enum DXGI_FORMAT
{
	DXGI_FORMAT_UNKNOWN						= 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS		= 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT			= 2,
	DXGI_FORMAT_R32G32B32A32_UINT			= 3,
	DXGI_FORMAT_R32G32B32A32_SINT			= 4,
	DXGI_FORMAT_R32G32B32_TYPELESS			= 5,
	DXGI_FORMAT_R32G32B32_FLOAT				= 6,
	DXGI_FORMAT_R32G32B32_UINT				= 7,
	DXGI_FORMAT_R32G32B32_SINT				= 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS		= 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT			= 10,
	DXGI_FORMAT_R16G16B16A16_UNORM			= 11,
	DXGI_FORMAT_R16G16B16A16_UINT			= 12,
	DXGI_FORMAT_R16G16B16A16_SNORM			= 13,
	DXGI_FORMAT_R16G16B16A16_SINT			= 14,
	DXGI_FORMAT_R32G32_TYPELESS				= 15,
	DXGI_FORMAT_R32G32_FLOAT				= 16,
	DXGI_FORMAT_R32G32_UINT					= 17,
	DXGI_FORMAT_R32G32_SINT					= 18,
	DXGI_FORMAT_R32G8X24_TYPELESS			= 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT		= 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS	= 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT		= 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS		= 23,
	DXGI_FORMAT_R10G10B10A2_UNORM			= 24,
	DXGI_FORMAT_R10G10B10A2_UINT			= 25,
	DXGI_FORMAT_R11G11B10_FLOAT				= 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS			= 27,
	DXGI_FORMAT_R8G8B8A8_UNORM				= 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB			= 29,
	DXGI_FORMAT_R8G8B8A8_UINT				= 30,
	DXGI_FORMAT_R8G8B8A8_SNORM				= 31,
	DXGI_FORMAT_R8G8B8A8_SINT				= 32,
	DXGI_FORMAT_R16G16_TYPELESS				= 33,
	DXGI_FORMAT_R16G16_FLOAT				= 34,
	DXGI_FORMAT_R16G16_UNORM				= 35,
	DXGI_FORMAT_R16G16_UINT					= 36,
	DXGI_FORMAT_R16G16_SNORM				= 37,
	DXGI_FORMAT_R16G16_SINT					= 38,
	DXGI_FORMAT_R32_TYPELESS				= 39,
	DXGI_FORMAT_D32_FLOAT					= 40,
	DXGI_FORMAT_R32_FLOAT					= 41,
	DXGI_FORMAT_R32_UINT					= 42,
	DXGI_FORMAT_R32_SINT					= 43,
	DXGI_FORMAT_R24G8_TYPELESS				= 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT			= 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS		= 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT		= 47,
	DXGI_FORMAT_R8G8_TYPELESS				= 48,
	DXGI_FORMAT_R8G8_UNORM					= 49,
	DXGI_FORMAT_R8G8_UINT					= 50,
	DXGI_FORMAT_R8G8_SNORM					= 51,
	DXGI_FORMAT_R8G8_SINT					= 52,
	DXGI_FORMAT_R16_TYPELESS				= 53,
	DXGI_FORMAT_R16_FLOAT					= 54,
	DXGI_FORMAT_D16_UNORM					= 55,
	DXGI_FORMAT_R16_UNORM					= 56,
	DXGI_FORMAT_R16_UINT					= 57,
	DXGI_FORMAT_R16_SNORM					= 58,
	DXGI_FORMAT_R16_SINT					= 59,
	DXGI_FORMAT_R8_TYPELESS					= 60,
	DXGI_FORMAT_R8_UNORM					= 61,
	DXGI_FORMAT_R8_UINT						= 62,
	DXGI_FORMAT_R8_SNORM					= 63,
	DXGI_FORMAT_R8_SINT						= 64,
	DXGI_FORMAT_A8_UNORM					= 65,
	DXGI_FORMAT_R1_UNORM					= 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP			= 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM				= 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM				= 69,
	DXGI_FORMAT_BC1_TYPELESS				= 70,
	DXGI_FORMAT_BC1_UNORM					= 71,
	DXGI_FORMAT_BC1_UNORM_SRGB				= 72,
	DXGI_FORMAT_BC2_TYPELESS				= 73,
	DXGI_FORMAT_BC2_UNORM					= 74,
	DXGI_FORMAT_BC2_UNORM_SRGB				= 75,
	DXGI_FORMAT_BC3_TYPELESS				= 76,
	DXGI_FORMAT_BC3_UNORM					= 77,
	DXGI_FORMAT_BC3_UNORM_SRGB				= 78,
	DXGI_FORMAT_BC4_TYPELESS				= 79,
	DXGI_FORMAT_BC4_UNORM					= 80,
	DXGI_FORMAT_BC4_SNORM					= 81,
	DXGI_FORMAT_BC5_TYPELESS				= 82,
	DXGI_FORMAT_BC5_UNORM					= 83,
	DXGI_FORMAT_BC5_SNORM					= 84,
	DXGI_FORMAT_B5G6R5_UNORM				= 85,
	DXGI_FORMAT_B5G5R5A1_UNORM				= 86,
	DXGI_FORMAT_B8G8R8A8_UNORM				= 87,
	DXGI_FORMAT_B8G8R8X8_UNORM				= 8,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM	= 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS			= 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB			= 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS			= 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB			= 93,
	DXGI_FORMAT_BC6H_TYPELESS				= 94,
	DXGI_FORMAT_BC6H_UF16					= 95,
	DXGI_FORMAT_BC6H_SF16					= 96,
	DXGI_FORMAT_BC7_TYPELESS				= 97,
	DXGI_FORMAT_BC7_UNORM					= 98,
	DXGI_FORMAT_BC7_UNORM_SRGB				= 99,
	DXGI_FORMAT_AYUV						= 100,
	DXGI_FORMAT_Y410						= 101,
	DXGI_FORMAT_Y416						= 102,
	DXGI_FORMAT_NV12						= 103,
	DXGI_FORMAT_P010						= 104,
	DXGI_FORMAT_P016						= 105,
	DXGI_FORMAT_420_OPAQUE					= 106,
	DXGI_FORMAT_YUY2						= 107,
	DXGI_FORMAT_Y210						= 108,
	DXGI_FORMAT_Y216						= 109,
	DXGI_FORMAT_NV11						= 110,
	DXGI_FORMAT_AI44						= 111,
	DXGI_FORMAT_IA44						= 112,
	DXGI_FORMAT_P8							= 113,
	DXGI_FORMAT_A8P8						= 114,
	DXGI_FORMAT_B4G4R4A4_UNORM				= 115,
	DXGI_FORMAT_P208						= 130,
	DXGI_FORMAT_V208						= 131,
	DXGI_FORMAT_V408						= 132,
	DXGI_FORMAT_FORCE_UINT					= 0xffffffff
} DXGI_FORMAT;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef struct DXGI_RATIONAL
{
	UINT Numerator;
	UINT Denominator;
} DXGI_RATIONAL;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef enum DXGI_MODE_SCANLINE_ORDER
{
	DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED		= 0,
	DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE		= 1,
	DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST	= 2,
	DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST	= 3
} DXGI_MODE_SCANLINE_ORDER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef enum DXGI_MODE_SCALING
{
	DXGI_MODE_SCALING_UNSPECIFIED	= 0,
	DXGI_MODE_SCALING_CENTERED		= 1,
	DXGI_MODE_SCALING_STRETCHED		= 2
} DXGI_MODE_SCALING;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef struct DXGI_MODE_DESC
{
	UINT Width;
	UINT Height;
	DXGI_RATIONAL RefreshRate;
	DXGI_FORMAT Format;
	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
	DXGI_MODE_SCALING Scaling;
} DXGI_MODE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef struct DXGI_SAMPLE_DESC
{
	UINT Count;
	UINT Quality;
} DXGI_SAMPLE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef enum DXGI_MODE_ROTATION
{
	DXGI_MODE_ROTATION_UNSPECIFIED	= 0,
	DXGI_MODE_ROTATION_IDENTITY		= 1,
	DXGI_MODE_ROTATION_ROTATE90		= 2,
	DXGI_MODE_ROTATION_ROTATE180	= 3,
	DXGI_MODE_ROTATION_ROTATE270	= 4
} DXGI_MODE_ROTATION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgitype.h"
typedef enum DXGI_COLOR_SPACE_TYPE
{
	DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709			= 0,
	DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709			= 1,
	DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709		= 2,
	DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020		= 3,
	DXGI_COLOR_SPACE_RESERVED						= 4,
	DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601	= 5,
	DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601		= 6,
	DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601		= 7,
	DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709		= 8,
	DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709		= 9,
	DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020	= 10,
	DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020		= 11,
	DXGI_COLOR_SPACE_CUSTOM							= 0xFFFFFFFF
} DXGI_COLOR_SPACE_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
typedef UINT DXGI_USAGE;
#define DXGI_MWA_NO_ALT_ENTER			(1 << 1)
#define DXGI_USAGE_RENDER_TARGET_OUTPUT	0x00000020UL

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
typedef enum DXGI_SWAP_EFFECT
{
	DXGI_SWAP_EFFECT_DISCARD			= 0,
	DXGI_SWAP_EFFECT_SEQUENTIAL			= 1,
	DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL	= 3,
	DXGI_SWAP_EFFECT_FLIP_DISCARD		= 4
} DXGI_SWAP_EFFECT;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
typedef struct DXGI_SWAP_CHAIN_DESC
{
	DXGI_MODE_DESC BufferDesc;
	DXGI_SAMPLE_DESC SampleDesc;
	DXGI_USAGE BufferUsage;
	UINT BufferCount;
	HWND OutputWindow;
	BOOL Windowed;
	DXGI_SWAP_EFFECT SwapEffect;
	UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3Dcommon.h"
enum D3D_FEATURE_LEVEL
{
	D3D_FEATURE_LEVEL_9_1	= 0x9100,
	D3D_FEATURE_LEVEL_9_2	= 0x9200,
	D3D_FEATURE_LEVEL_9_3	= 0x9300,
	D3D_FEATURE_LEVEL_10_0	= 0xa000,
	D3D_FEATURE_LEVEL_10_1	= 0xa100,
	D3D_FEATURE_LEVEL_11_0	= 0xb000,
	D3D_FEATURE_LEVEL_11_1	= 0xb100,
	D3D_FEATURE_LEVEL_12_0	= 0xc000,
	D3D_FEATURE_LEVEL_12_1	= 0xc100
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3Dcommon.h"
typedef enum D3D_PRIMITIVE_TOPOLOGY
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
} D3D_PRIMITIVE_TOPOLOGY;

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

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3Dcommon.h"
typedef ID3D10Blob ID3DBlob;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3dcompiler.h"
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

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_VIEWPORT
{
	FLOAT TopLeftX;
	FLOAT TopLeftY;
	FLOAT Width;
	FLOAT Height;
	FLOAT MinDepth;
	FLOAT MaxDepth;
} D3D12_VIEWPORT;
typedef RECT D3D12_RECT;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_BOX
{
	UINT left;
	UINT top;
	UINT front;
	UINT right;
	UINT bottom;
	UINT back;
} D3D12_BOX;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_INPUT_CLASSIFICATION
{
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA		= 0,
	D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA	= 1
} D3D12_INPUT_CLASSIFICATION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_INPUT_ELEMENT_DESC
{
	LPCSTR SemanticName;
	UINT SemanticIndex;
	DXGI_FORMAT Format;
	UINT InputSlot;
	UINT AlignedByteOffset;
	D3D12_INPUT_CLASSIFICATION InputSlotClass;
	UINT InstanceDataStepRate;
} D3D12_INPUT_ELEMENT_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_RESOURCE_DIMENSION
{
	D3D12_RESOURCE_DIMENSION_UNKNOWN	= 0,
	D3D12_RESOURCE_DIMENSION_BUFFER		= 1,
	D3D12_RESOURCE_DIMENSION_TEXTURE1D	= 2,
	D3D12_RESOURCE_DIMENSION_TEXTURE2D	= 3,
	D3D12_RESOURCE_DIMENSION_TEXTURE3D	= 4
} D3D12_RESOURCE_DIMENSION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_TEXTURE_LAYOUT
{
	D3D12_TEXTURE_LAYOUT_UNKNOWN				= 0,
	D3D12_TEXTURE_LAYOUT_ROW_MAJOR				= 1,
	D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE	= 2,
	D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE	= 3
} D3D12_TEXTURE_LAYOUT;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_RESOURCE_FLAGS
{
	D3D12_RESOURCE_FLAG_NONE						= 0,
	D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET			= 0x1,
	D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL			= 0x2,
	D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS		= 0x4,
	D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE		= 0x8,
	D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER			= 0x10,
	D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS	= 0x20
} D3D12_RESOURCE_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RESOURCE_DESC
{
	D3D12_RESOURCE_DIMENSION Dimension;
	UINT64 Alignment;
	UINT64 Width;
	UINT Height;
	UINT16 DepthOrArraySize;
	UINT16 MipLevels;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
	D3D12_TEXTURE_LAYOUT Layout;
	D3D12_RESOURCE_FLAGS Flags;
} D3D12_RESOURCE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_RESOURCE_STATES
{
	D3D12_RESOURCE_STATE_COMMON						= 0,
	D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER	= 0x1,
	D3D12_RESOURCE_STATE_INDEX_BUFFER				= 0x2,
	D3D12_RESOURCE_STATE_RENDER_TARGET				= 0x4,
	D3D12_RESOURCE_STATE_UNORDERED_ACCESS			= 0x8,
	D3D12_RESOURCE_STATE_DEPTH_WRITE				= 0x10,
	D3D12_RESOURCE_STATE_DEPTH_READ					= 0x20,
	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE	= 0x40,
	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE		= 0x80,
	D3D12_RESOURCE_STATE_STREAM_OUT					= 0x100,
	D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT			= 0x200,
	D3D12_RESOURCE_STATE_COPY_DEST					= 0x400,
	D3D12_RESOURCE_STATE_COPY_SOURCE				= 0x800,
	D3D12_RESOURCE_STATE_RESOLVE_DEST				= 0x1000,
	D3D12_RESOURCE_STATE_RESOLVE_SOURCE				= 0x2000,
	D3D12_RESOURCE_STATE_GENERIC_READ				= ((((( 0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
	D3D12_RESOURCE_STATE_PRESENT					= 0,
	D3D12_RESOURCE_STATE_PREDICATION				= 0x200
} D3D12_RESOURCE_STATES;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_RESOURCE_BARRIER_TYPE
{
	D3D12_RESOURCE_BARRIER_TYPE_TRANSITION	= 0,
	D3D12_RESOURCE_BARRIER_TYPE_ALIASING	= (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION + 1),
	D3D12_RESOURCE_BARRIER_TYPE_UAV			= (D3D12_RESOURCE_BARRIER_TYPE_ALIASING + 1)
} D3D12_RESOURCE_BARRIER_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_RESOURCE_BARRIER_FLAGS
{
	D3D12_RESOURCE_BARRIER_FLAG_NONE		= 0,
	D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY	= 0x1,
	D3D12_RESOURCE_BARRIER_FLAG_END_ONLY	= 0x2
} D3D12_RESOURCE_BARRIER_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RESOURCE_TRANSITION_BARRIER
{
	ID3D12Resource *pResource;
	UINT Subresource;
	D3D12_RESOURCE_STATES StateBefore;
	D3D12_RESOURCE_STATES StateAfter;
} D3D12_RESOURCE_TRANSITION_BARRIER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RESOURCE_ALIASING_BARRIER
{
	ID3D12Resource *pResourceBefore;
	ID3D12Resource *pResourceAfter;
} D3D12_RESOURCE_ALIASING_BARRIER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RESOURCE_UAV_BARRIER
{
	ID3D12Resource *pResource;
} D3D12_RESOURCE_UAV_BARRIER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RESOURCE_BARRIER
{
	D3D12_RESOURCE_BARRIER_TYPE Type;
	D3D12_RESOURCE_BARRIER_FLAGS Flags;
	union
	{
		D3D12_RESOURCE_TRANSITION_BARRIER Transition;
		D3D12_RESOURCE_ALIASING_BARRIER Aliasing;
		D3D12_RESOURCE_UAV_BARRIER UAV;
	};
} D3D12_RESOURCE_BARRIER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_RTV_DIMENSION
{
	D3D12_RTV_DIMENSION_UNKNOWN				= 0,
	D3D12_RTV_DIMENSION_BUFFER				= 1,
	D3D12_RTV_DIMENSION_TEXTURE1D			= 2,
	D3D12_RTV_DIMENSION_TEXTURE1DARRAY		= 3,
	D3D12_RTV_DIMENSION_TEXTURE2D			= 4,
	D3D12_RTV_DIMENSION_TEXTURE2DARRAY		= 5,
	D3D12_RTV_DIMENSION_TEXTURE2DMS			= 6,
	D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
	D3D12_RTV_DIMENSION_TEXTURE3D			= 8
} D3D12_RTV_DIMENSION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_BUFFER_RTV
{
	UINT64 FirstElement;
	UINT NumElements;
} D3D12_BUFFER_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX1D_RTV
{
	UINT MipSlice;
} D3D12_TEX1D_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX1D_ARRAY_RTV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D12_TEX1D_ARRAY_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2D_RTV
{
	UINT MipSlice;
	UINT PlaneSlice;
} D3D12_TEX2D_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2DMS_RTV
{
	UINT UnusedField_NothingToDefine;
} D3D12_TEX2DMS_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2D_ARRAY_RTV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
	UINT PlaneSlice;
} D3D12_TEX2D_ARRAY_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2DMS_ARRAY_RTV
{
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D12_TEX2DMS_ARRAY_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX3D_RTV
{
	UINT MipSlice;
	UINT FirstWSlice;
	UINT WSize;
} D3D12_TEX3D_RTV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RENDER_TARGET_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D12_RTV_DIMENSION ViewDimension;
	union
	{
		D3D12_BUFFER_RTV Buffer;
		D3D12_TEX1D_RTV Texture1D;
		D3D12_TEX1D_ARRAY_RTV Texture1DArray;
		D3D12_TEX2D_RTV Texture2D;
		D3D12_TEX2D_ARRAY_RTV Texture2DArray;
		D3D12_TEX2DMS_RTV Texture2DMS;
		D3D12_TEX2DMS_ARRAY_RTV Texture2DMSArray;
		D3D12_TEX3D_RTV Texture3D;
	};
} D3D12_RENDER_TARGET_VIEW_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_COMMAND_LIST_TYPE
{
	D3D12_COMMAND_LIST_TYPE_DIRECT	= 0,
	D3D12_COMMAND_LIST_TYPE_BUNDLE	= 1,
	D3D12_COMMAND_LIST_TYPE_COMPUTE	= 2,
	D3D12_COMMAND_LIST_TYPE_COPY	= 3
} D3D12_COMMAND_LIST_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RANGE
{
	SIZE_T Begin;
	SIZE_T End;
} D3D12_RANGE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef UINT64 D3D12_GPU_VIRTUAL_ADDRESS;
#define	D3D12_REQ_SUBRESOURCES					30720
#define D3D12_FLOAT32_MAX						3.402823466e+38f
#define D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES	0xffffffff
#define	D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND	0xffffffff
#define	D3D12_DEFAULT_DEPTH_BIAS				0
#define D3D12_DEFAULT_DEPTH_BIAS_CLAMP			0.0f
#define D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS	0.0f
#define	D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT	8
#define D3D12_SHADER_COMPONENT_MAPPING_MASK 0x7 
#define D3D12_SHADER_COMPONENT_MAPPING_SHIFT 3 
#define D3D12_SHADER_COMPONENT_MAPPING_ALWAYS_SET_BIT_AVOIDING_ZEROMEM_MISTAKES (1<<(D3D12_SHADER_COMPONENT_MAPPING_SHIFT*4)) 
#define D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(Src0,Src1,Src2,Src3) ((((Src0)&D3D12_SHADER_COMPONENT_MAPPING_MASK)| \
                                                                (((Src1)&D3D12_SHADER_COMPONENT_MAPPING_MASK)<<D3D12_SHADER_COMPONENT_MAPPING_SHIFT)| \
                                                                (((Src2)&D3D12_SHADER_COMPONENT_MAPPING_MASK)<<(D3D12_SHADER_COMPONENT_MAPPING_SHIFT*2))| \
                                                                (((Src3)&D3D12_SHADER_COMPONENT_MAPPING_MASK)<<(D3D12_SHADER_COMPONENT_MAPPING_SHIFT*3))| \
                                                                D3D12_SHADER_COMPONENT_MAPPING_ALWAYS_SET_BIT_AVOIDING_ZEROMEM_MISTAKES))
#define D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(ComponentToExtract,Mapping) ((D3D12_SHADER_COMPONENT_MAPPING)(Mapping >> (D3D12_SHADER_COMPONENT_MAPPING_SHIFT*ComponentToExtract) & D3D12_SHADER_COMPONENT_MAPPING_MASK))
#define D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0,1,2,3)

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_COMMAND_QUEUE_FLAGS
{
	D3D12_COMMAND_QUEUE_FLAG_NONE					= 0,
	D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT	= 0x1
} D3D12_COMMAND_QUEUE_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_COMMAND_QUEUE_PRIORITY
{
	D3D12_COMMAND_QUEUE_PRIORITY_NORMAL	= 0,
	D3D12_COMMAND_QUEUE_PRIORITY_HIGH	= 100
} D3D12_COMMAND_QUEUE_PRIORITY;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_COMMAND_QUEUE_DESC
{
	D3D12_COMMAND_LIST_TYPE Type;
	INT Priority;
	D3D12_COMMAND_QUEUE_FLAGS Flags;
	UINT NodeMask;
} D3D12_COMMAND_QUEUE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RESOURCE_ALLOCATION_INFO
{
	UINT64 SizeInBytes;
	UINT64 Alignment;
} D3D12_RESOURCE_ALLOCATION_INFO;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_HEAP_TYPE
{
	D3D12_HEAP_TYPE_DEFAULT		= 1,
	D3D12_HEAP_TYPE_UPLOAD		= 2,
	D3D12_HEAP_TYPE_READBACK	= 3,
	D3D12_HEAP_TYPE_CUSTOM		= 4
} D3D12_HEAP_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_CPU_PAGE_PROPERTY
{
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN			= 0,
	D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE	= 1,
	D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE	= 2,
	D3D12_CPU_PAGE_PROPERTY_WRITE_BACK		= 3
} D3D12_CPU_PAGE_PROPERTY;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_MEMORY_POOL
{
	D3D12_MEMORY_POOL_UNKNOWN	= 0,
	D3D12_MEMORY_POOL_L0		= 1,
	D3D12_MEMORY_POOL_L1		= 2
} D3D12_MEMORY_POOL;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_HEAP_PROPERTIES
{
	D3D12_HEAP_TYPE Type;
	D3D12_CPU_PAGE_PROPERTY CPUPageProperty;
	D3D12_MEMORY_POOL MemoryPoolPreference;
	UINT CreationNodeMask;
	UINT VisibleNodeMask;
} D3D12_HEAP_PROPERTIES;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_HEAP_FLAGS
{
	D3D12_HEAP_FLAG_NONE							= 0,
	D3D12_HEAP_FLAG_SHARED							= 0x1,
	D3D12_HEAP_FLAG_DENY_BUFFERS					= 0x4,
	D3D12_HEAP_FLAG_ALLOW_DISPLAY					= 0x8,
	D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER			= 0x20,
	D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES				= 0x40,
	D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES			= 0x80,
	D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES	= 0,
	D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS				= 0xc0,
	D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES	= 0x44,
	D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES		= 0x84
} D3D12_HEAP_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_DEPTH_STENCIL_VALUE
{
	FLOAT Depth;
	UINT8 Stencil;
} D3D12_DEPTH_STENCIL_VALUE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_CLEAR_VALUE
{
	DXGI_FORMAT Format;
	union
	{
		FLOAT Color[4];
		D3D12_DEPTH_STENCIL_VALUE DepthStencil;
	};
} D3D12_CLEAR_VALUE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_FENCE_FLAGS
{
	D3D12_FENCE_FLAG_NONE					= 0,
	D3D12_FENCE_FLAG_SHARED					= 0x1,
	D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER	= 0x2
} D3D12_FENCE_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_FEATURE
{
	D3D12_FEATURE_D3D12_OPTIONS					= 0,
	D3D12_FEATURE_ARCHITECTURE					= (D3D12_FEATURE_D3D12_OPTIONS + 1),
	D3D12_FEATURE_FEATURE_LEVELS				= (D3D12_FEATURE_ARCHITECTURE + 1),
	D3D12_FEATURE_FORMAT_SUPPORT				= (D3D12_FEATURE_FEATURE_LEVELS + 1),
	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS	= (D3D12_FEATURE_FORMAT_SUPPORT + 1),
	D3D12_FEATURE_FORMAT_INFO					= (D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS + 1),
	D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT	= (D3D12_FEATURE_FORMAT_INFO + 1) 
} D3D12_FEATURE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_TILE_MAPPING_FLAGS
{
	D3D12_TILE_MAPPING_FLAG_NONE		= 0,
	D3D12_TILE_MAPPING_FLAG_NO_HAZARD	= 0x1
} D3D12_TILE_MAPPING_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_CPU_DESCRIPTOR_HANDLE
{
	SIZE_T ptr;
} D3D12_CPU_DESCRIPTOR_HANDLE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_GPU_DESCRIPTOR_HANDLE
{
	UINT64 ptr;
} D3D12_GPU_DESCRIPTOR_HANDLE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_DESCRIPTOR_HEAP_TYPE
{
	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV	= 0,
	D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER		= (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV + 1),
	D3D12_DESCRIPTOR_HEAP_TYPE_RTV			= (D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1),
	D3D12_DESCRIPTOR_HEAP_TYPE_DSV			= (D3D12_DESCRIPTOR_HEAP_TYPE_RTV + 1),
	D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES	= (D3D12_DESCRIPTOR_HEAP_TYPE_DSV + 1) 
} D3D12_DESCRIPTOR_HEAP_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_DESCRIPTOR_HEAP_FLAGS
{
	D3D12_DESCRIPTOR_HEAP_FLAG_NONE				= 0,
	D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE	= 0x1
} D3D12_DESCRIPTOR_HEAP_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_DESCRIPTOR_HEAP_DESC
{
	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	UINT NumDescriptors;
	D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
	UINT NodeMask;
} D3D12_DESCRIPTOR_HEAP_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_TILE_COPY_FLAGS
{
	D3D12_TILE_COPY_FLAG_NONE										= 0,
	D3D12_TILE_COPY_FLAG_NO_HAZARD									= 0x1,
	D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE	= 0x2,
	D3D12_TILE_COPY_FLAG_SWIZZLED_TILED_RESOURCE_TO_LINEAR_BUFFER	= 0x4
} D3D12_TILE_COPY_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_PRIMITIVE_TOPOLOGY_TYPE
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT		= 1,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE		= 2,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH		= 4
} D3D12_PRIMITIVE_TOPOLOGY_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef D3D_PRIMITIVE_TOPOLOGY D3D12_PRIMITIVE_TOPOLOGY;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_CLEAR_FLAGS
{
	D3D12_CLEAR_FLAG_DEPTH		= 0x1,
	D3D12_CLEAR_FLAG_STENCIL	= 0x2
} D3D12_CLEAR_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_QUERY_HEAP_TYPE
{
	D3D12_QUERY_HEAP_TYPE_OCCLUSION				= 0,
	D3D12_QUERY_HEAP_TYPE_TIMESTAMP				= 1,
	D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS	= 2,
	D3D12_QUERY_HEAP_TYPE_SO_STATISTICS			= 3
} D3D12_QUERY_HEAP_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_QUERY_TYPE
{
	D3D12_QUERY_TYPE_OCCLUSION				= 0,
	D3D12_QUERY_TYPE_BINARY_OCCLUSION		= 1,
	D3D12_QUERY_TYPE_TIMESTAMP				= 2,
	D3D12_QUERY_TYPE_PIPELINE_STATISTICS	= 3,
	D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0	= 4,
	D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1	= 5,
	D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2	= 6,
	D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3	= 7
} D3D12_QUERY_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_QUERY_HEAP_DESC
{
	D3D12_QUERY_HEAP_TYPE Type;
	UINT Count;
	UINT NodeMask;
} D3D12_QUERY_HEAP_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_PREDICATION_OP
{
	D3D12_PREDICATION_OP_EQUAL_ZERO		= 0,
	D3D12_PREDICATION_OP_NOT_EQUAL_ZERO	= 1
} D3D12_PREDICATION_OP;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_QUERY_DATA_PIPELINE_STATISTICS
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
} D3D12_QUERY_DATA_PIPELINE_STATISTICS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_VERTEX_BUFFER_VIEW
{
	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	UINT SizeInBytes;
	UINT StrideInBytes;
} D3D12_VERTEX_BUFFER_VIEW;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_INDEX_BUFFER_VIEW
{
	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	UINT SizeInBytes;
	DXGI_FORMAT Format;
} D3D12_INDEX_BUFFER_VIEW;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC
{
	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	UINT SizeInBytes;
} D3D12_CONSTANT_BUFFER_VIEW_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_DSV_DIMENSION
{
	D3D12_DSV_DIMENSION_UNKNOWN				= 0,
	D3D12_DSV_DIMENSION_TEXTURE1D			= 1,
	D3D12_DSV_DIMENSION_TEXTURE1DARRAY		= 2,
	D3D12_DSV_DIMENSION_TEXTURE2D			= 3,
	D3D12_DSV_DIMENSION_TEXTURE2DARRAY		= 4,
	D3D12_DSV_DIMENSION_TEXTURE2DMS			= 5,
	D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY	= 6
} D3D12_DSV_DIMENSION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_DSV_FLAGS
{
	D3D12_DSV_FLAG_NONE					= 0,
	D3D12_DSV_FLAG_READ_ONLY_DEPTH		= 0x1,
	D3D12_DSV_FLAG_READ_ONLY_STENCIL	= 0x2
} D3D12_DSV_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX1D_DSV
{
	UINT MipSlice;
} D3D12_TEX1D_DSV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX1D_ARRAY_DSV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D12_TEX1D_ARRAY_DSV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2D_DSV
{
	UINT MipSlice;
} D3D12_TEX2D_DSV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2D_ARRAY_DSV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D12_TEX2D_ARRAY_DSV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2DMS_DSV
{
	UINT UnusedField_NothingToDefine;
} D3D12_TEX2DMS_DSV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2DMS_ARRAY_DSV
{
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D12_TEX2DMS_ARRAY_DSV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_DEPTH_STENCIL_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D12_DSV_DIMENSION ViewDimension;
	D3D12_DSV_FLAGS Flags;
	union
	{
		D3D12_TEX1D_DSV Texture1D;
		D3D12_TEX1D_ARRAY_DSV Texture1DArray;
		D3D12_TEX2D_DSV Texture2D;
		D3D12_TEX2D_ARRAY_DSV Texture2DArray;
		D3D12_TEX2DMS_DSV Texture2DMS;
		D3D12_TEX2DMS_ARRAY_DSV Texture2DMSArray;
	};
} D3D12_DEPTH_STENCIL_VIEW_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_SRV_DIMENSION
{
	D3D12_SRV_DIMENSION_UNKNOWN				= 0,
	D3D12_SRV_DIMENSION_BUFFER				= 1,
	D3D12_SRV_DIMENSION_TEXTURE1D			= 2,
	D3D12_SRV_DIMENSION_TEXTURE1DARRAY		= 3,
	D3D12_SRV_DIMENSION_TEXTURE2D			= 4,
	D3D12_SRV_DIMENSION_TEXTURE2DARRAY		= 5,
	D3D12_SRV_DIMENSION_TEXTURE2DMS			= 6,
	D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY	= 7,
	D3D12_SRV_DIMENSION_TEXTURE3D			= 8,
	D3D12_SRV_DIMENSION_TEXTURECUBE			= 9,
	D3D12_SRV_DIMENSION_TEXTURECUBEARRAY	= 10
} D3D12_SRV_DIMENSION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_BUFFER_SRV_FLAGS
{
	D3D12_BUFFER_SRV_FLAG_NONE	= 0,
	D3D12_BUFFER_SRV_FLAG_RAW	= 0x1
} D3D12_BUFFER_SRV_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_BUFFER_SRV
{
	UINT64 FirstElement;
	UINT NumElements;
	UINT StructureByteStride;
	D3D12_BUFFER_SRV_FLAGS Flags;
} D3D12_BUFFER_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX1D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	FLOAT ResourceMinLODClamp;
} D3D12_TEX1D_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX1D_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT FirstArraySlice;
	UINT ArraySize;
	FLOAT ResourceMinLODClamp;
} D3D12_TEX1D_ARRAY_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT PlaneSlice;
	FLOAT ResourceMinLODClamp;
} D3D12_TEX2D_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2D_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT FirstArraySlice;
	UINT ArraySize;
	UINT PlaneSlice;
	FLOAT ResourceMinLODClamp;
} D3D12_TEX2D_ARRAY_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2DMS_SRV
{
	UINT UnusedField_NothingToDefine;
} D3D12_TEX2DMS_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX2DMS_ARRAY_SRV
{
	UINT FirstArraySlice;
	UINT ArraySize;
} D3D12_TEX2DMS_ARRAY_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEX3D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	FLOAT ResourceMinLODClamp;
} D3D12_TEX3D_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEXCUBE_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	FLOAT ResourceMinLODClamp;
} D3D12_TEXCUBE_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEXCUBE_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT First2DArrayFace;
	UINT NumCubes;
	FLOAT ResourceMinLODClamp;
} D3D12_TEXCUBE_ARRAY_SRV;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_SHADER_RESOURCE_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D12_SRV_DIMENSION ViewDimension;
	UINT Shader4ComponentMapping;
	union
	{
		D3D12_BUFFER_SRV Buffer;
		D3D12_TEX1D_SRV Texture1D;
		D3D12_TEX1D_ARRAY_SRV Texture1DArray;
		D3D12_TEX2D_SRV Texture2D;
		D3D12_TEX2D_ARRAY_SRV Texture2DArray;
		D3D12_TEX2DMS_SRV Texture2DMS;
		D3D12_TEX2DMS_ARRAY_SRV Texture2DMSArray;
		D3D12_TEX3D_SRV Texture3D;
		D3D12_TEXCUBE_SRV TextureCube;
		D3D12_TEXCUBE_ARRAY_SRV TextureCubeArray;
	};
} D3D12_SHADER_RESOURCE_VIEW_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_SUBRESOURCE_DATA
{
	const void *pData;
	LONG_PTR RowPitch;
	LONG_PTR SlicePitch;
} D3D12_SUBRESOURCE_DATA;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_MEMCPY_DEST
{
	void *pData;
	SIZE_T RowPitch;
	SIZE_T SlicePitch;
} D3D12_MEMCPY_DEST;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_SUBRESOURCE_FOOTPRINT
{
	DXGI_FORMAT Format;
	UINT Width;
	UINT Height;
	UINT Depth;
	UINT RowPitch;
} D3D12_SUBRESOURCE_FOOTPRINT;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT
{
	UINT64 Offset;
	D3D12_SUBRESOURCE_FOOTPRINT Footprint;
} D3D12_PLACED_SUBRESOURCE_FOOTPRINT;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_TEXTURE_COPY_TYPE
{
	D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX	= 0,
	D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT	= 1
} D3D12_TEXTURE_COPY_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_TEXTURE_COPY_LOCATION
{
	ID3D12Resource *pResource;
	D3D12_TEXTURE_COPY_TYPE Type;
	union
	{
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
		UINT SubresourceIndex;
	};
} D3D12_TEXTURE_COPY_LOCATION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_DESCRIPTOR_RANGE_TYPE
{
	D3D12_DESCRIPTOR_RANGE_TYPE_SRV		= 0,
	D3D12_DESCRIPTOR_RANGE_TYPE_UAV		= (D3D12_DESCRIPTOR_RANGE_TYPE_SRV + 1),
	D3D12_DESCRIPTOR_RANGE_TYPE_CBV		= (D3D12_DESCRIPTOR_RANGE_TYPE_UAV + 1),
	D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER	= (D3D12_DESCRIPTOR_RANGE_TYPE_CBV + 1) 
} D3D12_DESCRIPTOR_RANGE_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_DESCRIPTOR_RANGE
{
	D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
	UINT NumDescriptors;
	UINT BaseShaderRegister;
	UINT RegisterSpace;
	UINT OffsetInDescriptorsFromTableStart;
} D3D12_DESCRIPTOR_RANGE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D_ROOT_SIGNATURE_VERSION
{
	D3D_ROOT_SIGNATURE_VERSION_1	= 0x1
} D3D_ROOT_SIGNATURE_VERSION;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_ROOT_PARAMETER_TYPE
{
	D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE	= 0,
	D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS	= (D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE + 1),
	D3D12_ROOT_PARAMETER_TYPE_CBV				= (D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS + 1),
	D3D12_ROOT_PARAMETER_TYPE_SRV				= (D3D12_ROOT_PARAMETER_TYPE_CBV + 1),
	D3D12_ROOT_PARAMETER_TYPE_UAV				= (D3D12_ROOT_PARAMETER_TYPE_SRV + 1)
} D3D12_ROOT_PARAMETER_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_ROOT_DESCRIPTOR_TABLE
{
	UINT NumDescriptorRanges;
	_Field_size_full_(NumDescriptorRanges)  const D3D12_DESCRIPTOR_RANGE *pDescriptorRanges;
} D3D12_ROOT_DESCRIPTOR_TABLE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_ROOT_CONSTANTS
{
	UINT ShaderRegister;
	UINT RegisterSpace;
	UINT Num32BitValues;
} D3D12_ROOT_CONSTANTS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_ROOT_DESCRIPTOR
{
	UINT ShaderRegister;
	UINT RegisterSpace;
} D3D12_ROOT_DESCRIPTOR;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_SHADER_VISIBILITY
{
	D3D12_SHADER_VISIBILITY_ALL			= 0,
	D3D12_SHADER_VISIBILITY_VERTEX		= 1,
	D3D12_SHADER_VISIBILITY_HULL		= 2,
	D3D12_SHADER_VISIBILITY_DOMAIN		= 3,
	D3D12_SHADER_VISIBILITY_GEOMETRY	= 4,
	D3D12_SHADER_VISIBILITY_PIXEL		= 5
} D3D12_SHADER_VISIBILITY;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_ROOT_PARAMETER
{
	D3D12_ROOT_PARAMETER_TYPE ParameterType;
	union
	{
		D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
		D3D12_ROOT_CONSTANTS Constants;
		D3D12_ROOT_DESCRIPTOR Descriptor;
	};
	D3D12_SHADER_VISIBILITY ShaderVisibility;
} D3D12_ROOT_PARAMETER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_STATIC_BORDER_COLOR
{
	D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK	= 0,
	D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK		= (D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK + 1),
	D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE		= (D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK + 1)
} D3D12_STATIC_BORDER_COLOR;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_FILTER
{
	D3D12_FILTER_MIN_MAG_MIP_POINT							= 0,
	D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR					= 0x1,
	D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT				= 0x4,
	D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR					= 0x5,
	D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT					= 0x10,
	D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR			= 0x11,
	D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT					= 0x14,
	D3D12_FILTER_MIN_MAG_MIP_LINEAR							= 0x15,
	D3D12_FILTER_ANISOTROPIC								= 0x55,
	D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT				= 0x80,
	D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR		= 0x81,
	D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
	D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR		= 0x85,
	D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT		= 0x90,
	D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x91,
	D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT		= 0x94,
	D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR				= 0x95,
	D3D12_FILTER_COMPARISON_ANISOTROPIC						= 0xd5,
	D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT					= 0x100,
	D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR			= 0x101,
	D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT		= 0x104,
	D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR			= 0x105,
	D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT			= 0x110,
	D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x111,
	D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT			= 0x114,
	D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR					= 0x115,
	D3D12_FILTER_MINIMUM_ANISOTROPIC						= 0x155,
	D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT					= 0x180,
	D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR			= 0x181,
	D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT		= 0x184,
	D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR			= 0x185,
	D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT			= 0x190,
	D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x191,
	D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT			= 0x194,
	D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR					= 0x195,
	D3D12_FILTER_MAXIMUM_ANISOTROPIC						= 0x1d5
} D3D12_FILTER;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_TEXTURE_ADDRESS_MODE
{
	D3D12_TEXTURE_ADDRESS_MODE_WRAP			= 1,
	D3D12_TEXTURE_ADDRESS_MODE_MIRROR		= 2,
	D3D12_TEXTURE_ADDRESS_MODE_CLAMP		= 3,
	D3D12_TEXTURE_ADDRESS_MODE_BORDER		= 4,
	D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE	= 5
} D3D12_TEXTURE_ADDRESS_MODE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_COMPARISON_FUNC
{
	D3D12_COMPARISON_FUNC_NEVER			= 1,
	D3D12_COMPARISON_FUNC_LESS			= 2,
	D3D12_COMPARISON_FUNC_EQUAL			= 3,
	D3D12_COMPARISON_FUNC_LESS_EQUAL	= 4,
	D3D12_COMPARISON_FUNC_GREATER		= 5,
	D3D12_COMPARISON_FUNC_NOT_EQUAL		= 6,
	D3D12_COMPARISON_FUNC_GREATER_EQUAL	= 7,
	D3D12_COMPARISON_FUNC_ALWAYS		= 8
} D3D12_COMPARISON_FUNC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_STATIC_SAMPLER_DESC
{
	D3D12_FILTER Filter;
	D3D12_TEXTURE_ADDRESS_MODE AddressU;
	D3D12_TEXTURE_ADDRESS_MODE AddressV;
	D3D12_TEXTURE_ADDRESS_MODE AddressW;
	FLOAT MipLODBias;
	UINT MaxAnisotropy;
	D3D12_COMPARISON_FUNC ComparisonFunc;
	D3D12_STATIC_BORDER_COLOR BorderColor;
	FLOAT MinLOD;
	FLOAT MaxLOD;
	UINT ShaderRegister;
	UINT RegisterSpace;
	D3D12_SHADER_VISIBILITY ShaderVisibility;
} D3D12_STATIC_SAMPLER_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_ROOT_SIGNATURE_FLAGS
{
	D3D12_ROOT_SIGNATURE_FLAG_NONE									= 0,
	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	= 0x1,
	D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS		= 0x2,
	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS			= 0x4,
	D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS		= 0x8,
	D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS		= 0x10,
	D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS			= 0x20,
	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT					= 0x40
} D3D12_ROOT_SIGNATURE_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_ROOT_SIGNATURE_DESC
{
	UINT NumParameters;
	_Field_size_full_(NumParameters)  const D3D12_ROOT_PARAMETER *pParameters;
	UINT NumStaticSamplers;
	_Field_size_full_(NumStaticSamplers)  const D3D12_STATIC_SAMPLER_DESC *pStaticSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS Flags;
} D3D12_ROOT_SIGNATURE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_SHADER_BYTECODE
{
	_Field_size_bytes_full_(BytecodeLength)  const void *pShaderBytecode;
	SIZE_T BytecodeLength;
} D3D12_SHADER_BYTECODE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_STREAM_OUTPUT_DESC
{
	_Field_size_full_(NumEntries) const D3D12_SO_DECLARATION_ENTRY *pSODeclaration;
	UINT NumEntries;
	_Field_size_full_(NumStrides) const UINT *pBufferStrides;
	UINT NumStrides;
	UINT RasterizedStream;
} D3D12_STREAM_OUTPUT_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_BLEND
{
	D3D12_BLEND_ZERO				= 1,
	D3D12_BLEND_ONE					= 2,
	D3D12_BLEND_SRC_COLOR			= 3,
	D3D12_BLEND_INV_SRC_COLOR		= 4,
	D3D12_BLEND_SRC_ALPHA			= 5,
	D3D12_BLEND_INV_SRC_ALPHA		= 6,
	D3D12_BLEND_DEST_ALPHA			= 7,
	D3D12_BLEND_INV_DEST_ALPHA		= 8,
	D3D12_BLEND_DEST_COLOR			= 9,
	D3D12_BLEND_INV_DEST_COLOR		= 10,
	D3D12_BLEND_SRC_ALPHA_SAT		= 11,
	D3D12_BLEND_BLEND_FACTOR		= 14,
	D3D12_BLEND_INV_BLEND_FACTOR	= 15,
	D3D12_BLEND_SRC1_COLOR			= 16,
	D3D12_BLEND_INV_SRC1_COLOR		= 17,
	D3D12_BLEND_SRC1_ALPHA			= 18,
	D3D12_BLEND_INV_SRC1_ALPHA		= 19
} D3D12_BLEND;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_BLEND_OP
{
	D3D12_BLEND_OP_ADD			= 1,
	D3D12_BLEND_OP_SUBTRACT		= 2,
	D3D12_BLEND_OP_REV_SUBTRACT	= 3,
	D3D12_BLEND_OP_MIN			= 4,
	D3D12_BLEND_OP_MAX			= 5
} D3D12_BLEND_OP;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_LOGIC_OP
{
	D3D12_LOGIC_OP_CLEAR			= 0,
	D3D12_LOGIC_OP_SET				= (D3D12_LOGIC_OP_CLEAR + 1),
	D3D12_LOGIC_OP_COPY				= (D3D12_LOGIC_OP_SET + 1),
	D3D12_LOGIC_OP_COPY_INVERTED	= (D3D12_LOGIC_OP_COPY + 1),
	D3D12_LOGIC_OP_NOOP				= (D3D12_LOGIC_OP_COPY_INVERTED + 1),
	D3D12_LOGIC_OP_INVERT			= (D3D12_LOGIC_OP_NOOP + 1),
	D3D12_LOGIC_OP_AND				= (D3D12_LOGIC_OP_INVERT + 1),
	D3D12_LOGIC_OP_NAND				= (D3D12_LOGIC_OP_AND + 1),
	D3D12_LOGIC_OP_OR				= (D3D12_LOGIC_OP_NAND + 1),
	D3D12_LOGIC_OP_NOR				= (D3D12_LOGIC_OP_OR + 1),
	D3D12_LOGIC_OP_XOR				= (D3D12_LOGIC_OP_NOR + 1),
	D3D12_LOGIC_OP_EQUIV			= (D3D12_LOGIC_OP_XOR + 1),
	D3D12_LOGIC_OP_AND_REVERSE		= (D3D12_LOGIC_OP_EQUIV + 1),
	D3D12_LOGIC_OP_AND_INVERTED		= (D3D12_LOGIC_OP_AND_REVERSE + 1),
	D3D12_LOGIC_OP_OR_REVERSE		= (D3D12_LOGIC_OP_AND_INVERTED + 1),
	D3D12_LOGIC_OP_OR_INVERTED		= (D3D12_LOGIC_OP_OR_REVERSE + 1) 
} D3D12_LOGIC_OP;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RENDER_TARGET_BLEND_DESC
{
	BOOL BlendEnable;
	BOOL LogicOpEnable;
	D3D12_BLEND SrcBlend;
	D3D12_BLEND DestBlend;
	D3D12_BLEND_OP BlendOp;
	D3D12_BLEND SrcBlendAlpha;
	D3D12_BLEND DestBlendAlpha;
	D3D12_BLEND_OP BlendOpAlpha;
	D3D12_LOGIC_OP LogicOp;
	UINT8 RenderTargetWriteMask;
} D3D12_RENDER_TARGET_BLEND_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_BLEND_DESC
{
	BOOL AlphaToCoverageEnable;
	BOOL IndependentBlendEnable;
	D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[8];
} D3D12_BLEND_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_COLOR_WRITE_ENABLE
{
	D3D12_COLOR_WRITE_ENABLE_RED	= 1,
	D3D12_COLOR_WRITE_ENABLE_GREEN	= 2,
	D3D12_COLOR_WRITE_ENABLE_BLUE	= 4,
	D3D12_COLOR_WRITE_ENABLE_ALPHA	= 8,
	D3D12_COLOR_WRITE_ENABLE_ALL	= (((D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN) | D3D12_COLOR_WRITE_ENABLE_BLUE | D3D12_COLOR_WRITE_ENABLE_ALPHA))
} D3D12_COLOR_WRITE_ENABLE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_CONSERVATIVE_RASTERIZATION_MODE
{
	D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
	D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
} D3D12_CONSERVATIVE_RASTERIZATION_MODE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_FILL_MODE
{
	D3D12_FILL_MODE_WIREFRAME	= 2,
	D3D12_FILL_MODE_SOLID		= 3
} D3D12_FILL_MODE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_CULL_MODE
{
	D3D12_CULL_MODE_NONE	= 1,
	D3D12_CULL_MODE_FRONT	= 2,
	D3D12_CULL_MODE_BACK	= 3
} D3D12_CULL_MODE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_RASTERIZER_DESC
{
	D3D12_FILL_MODE FillMode;
	D3D12_CULL_MODE CullMode;
	BOOL FrontCounterClockwise;
	INT DepthBias;
	FLOAT DepthBiasClamp;
	FLOAT SlopeScaledDepthBias;
	BOOL DepthClipEnable;
	BOOL MultisampleEnable;
	BOOL AntialiasedLineEnable;
	UINT ForcedSampleCount;
	D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
} D3D12_RASTERIZER_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_STENCIL_OP
{
	D3D12_STENCIL_OP_KEEP		= 1,
	D3D12_STENCIL_OP_ZERO		= 2,
	D3D12_STENCIL_OP_REPLACE	= 3,
	D3D12_STENCIL_OP_INCR_SAT	= 4,
	D3D12_STENCIL_OP_DECR_SAT	= 5,
	D3D12_STENCIL_OP_INVERT		= 6,
	D3D12_STENCIL_OP_INCR		= 7,
	D3D12_STENCIL_OP_DECR		= 8
} D3D12_STENCIL_OP;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_DEPTH_STENCILOP_DESC
{
	D3D12_STENCIL_OP StencilFailOp;
	D3D12_STENCIL_OP StencilDepthFailOp;
	D3D12_STENCIL_OP StencilPassOp;
	D3D12_COMPARISON_FUNC StencilFunc;
} D3D12_DEPTH_STENCILOP_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_DEPTH_WRITE_MASK
{
	D3D12_DEPTH_WRITE_MASK_ZERO	= 0,
	D3D12_DEPTH_WRITE_MASK_ALL	= 1
} D3D12_DEPTH_WRITE_MASK;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_DEPTH_STENCIL_DESC
{
	BOOL DepthEnable;
	D3D12_DEPTH_WRITE_MASK DepthWriteMask;
	D3D12_COMPARISON_FUNC DepthFunc;
	BOOL StencilEnable;
	UINT8 StencilReadMask;
	UINT8 StencilWriteMask;
	D3D12_DEPTH_STENCILOP_DESC FrontFace;
	D3D12_DEPTH_STENCILOP_DESC BackFace;
} D3D12_DEPTH_STENCIL_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_INPUT_LAYOUT_DESC
{
	_Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs;
	UINT NumElements;
} D3D12_INPUT_LAYOUT_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_INDEX_BUFFER_STRIP_CUT_VALUE
{
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED		= 0,
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF		= 1,
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF	= 2
} D3D12_INDEX_BUFFER_STRIP_CUT_VALUE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_CACHED_PIPELINE_STATE
{
	_Field_size_bytes_full_(CachedBlobSizeInBytes) const void *pCachedBlob;
	SIZE_T CachedBlobSizeInBytes;
} D3D12_CACHED_PIPELINE_STATE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_PIPELINE_STATE_FLAGS
{
	D3D12_PIPELINE_STATE_FLAG_NONE			= 0,
	D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG	= 0x1
} D3D12_PIPELINE_STATE_FLAGS;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_GRAPHICS_PIPELINE_STATE_DESC
{
	ID3D12RootSignature *pRootSignature;
	D3D12_SHADER_BYTECODE VS;
	D3D12_SHADER_BYTECODE PS;
	D3D12_SHADER_BYTECODE DS;
	D3D12_SHADER_BYTECODE HS;
	D3D12_SHADER_BYTECODE GS;
	D3D12_STREAM_OUTPUT_DESC StreamOutput;
	D3D12_BLEND_DESC BlendState;
	UINT SampleMask;
	D3D12_RASTERIZER_DESC RasterizerState;
	D3D12_DEPTH_STENCIL_DESC DepthStencilState;
	D3D12_INPUT_LAYOUT_DESC InputLayout;
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
	UINT NumRenderTargets;
	DXGI_FORMAT RTVFormats[8];
	DXGI_FORMAT DSVFormat;
	DXGI_SAMPLE_DESC SampleDesc;
	UINT NodeMask;
	D3D12_CACHED_PIPELINE_STATE CachedPSO;
	D3D12_PIPELINE_STATE_FLAGS Flags;
} D3D12_GRAPHICS_PIPELINE_STATE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_COMPUTE_PIPELINE_STATE_DESC
{
	ID3D12RootSignature *pRootSignature;
	D3D12_SHADER_BYTECODE CS;
	UINT NodeMask;
	D3D12_CACHED_PIPELINE_STATE CachedPSO;
	D3D12_PIPELINE_STATE_FLAGS Flags;
} D3D12_COMPUTE_PIPELINE_STATE_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef enum D3D12_INDIRECT_ARGUMENT_TYPE
{
	D3D12_INDIRECT_ARGUMENT_TYPE_DRAW					= 0,
	D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED			= (D3D12_INDIRECT_ARGUMENT_TYPE_DRAW + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH				= (D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW		= (D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW		= (D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT				= (D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW	= (D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW	= (D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW + 1),
	D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW	= (D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW + 1)
} D3D12_INDIRECT_ARGUMENT_TYPE;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_INDIRECT_ARGUMENT_DESC
{
	D3D12_INDIRECT_ARGUMENT_TYPE Type;
	union
	{
		struct
		{
			UINT Slot;
		} VertexBuffer;
		struct
		{
			UINT RootParameterIndex;
			UINT DestOffsetIn32BitValues;
			UINT Num32BitValuesToSet;
		} Constant;
		struct
		{
			UINT RootParameterIndex;
		} ConstantBufferView;
		struct
		{
			UINT RootParameterIndex;
		} ShaderResourceView;
		struct
		{
			UINT RootParameterIndex;
		} UnorderedAccessView;
	};
} D3D12_INDIRECT_ARGUMENT_DESC;

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
typedef struct D3D12_COMMAND_SIGNATURE_DESC
{
	UINT ByteStride;
	UINT NumArgumentDescs;
	_Field_size_full_(NumArgumentDescs) const D3D12_INDIRECT_ARGUMENT_DESC *pArgumentDescs;
	UINT NodeMask;
} D3D12_COMMAND_SIGNATURE_DESC;

#ifdef RHI_DEBUG
	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3d12sdklayers.h"
	typedef enum D3D12_DEBUG_FEATURE
	{
		D3D12_DEBUG_FEATURE_NONE						= 0,
		D3D12_DEBUG_FEATURE_TREAT_BUNDLE_AS_DRAW		= 0x1,
		D3D12_DEBUG_FEATURE_TREAT_BUNDLE_AS_DISPATCH	= 0x2
	} D3D12_DEBUG_FEATURE;

	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3d12sdklayers.h"
	typedef enum D3D12_RLDO_FLAGS
	{
		D3D12_RLDO_NONE				= 0,
		D3D12_RLDO_SUMMARY			= 0x1,
		D3D12_RLDO_DETAIL			= 0x2,
		D3D12_RLDO_IGNORE_INTERNAL	= 0x4
	} D3D12_RLDO_FLAGS;

	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "pix_win.h"
	static constexpr UINT PIX_EVENT_ANSI_VERSION = 1;
#endif


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("aec22fb8-76f3-4639-9be0-28eb43a67a2e")
IDXGIObject : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID Name, UINT DataSize, _In_reads_bytes_(DataSize) const void *pData) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID Name, _In_ const IUnknown *pUnknown) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID Name, _Inout_ UINT *pDataSize, _Out_writes_bytes_(*pDataSize) void *pData) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetParent(_In_ REFIID riid, _COM_Outptr_ void **ppParent) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("3d3e0379-f9de-4d58-bb6c-18d62992f1a6")
IDXGIDeviceSubObject : public IDXGIObject
{
	virtual HRESULT STDMETHODCALLTYPE GetDevice(_In_ REFIID riid, _COM_Outptr_ void **ppDevice) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("310d36a0-d2e7-4c0a-aa04-6a9d23b8886a")
IDXGISwapChain : public IDXGIDeviceSubObject
{
	virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, _In_ REFIID riid, _COM_Outptr_ void **ppSurface) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, _In_opt_ IDXGIOutput *pTarget) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(_Out_opt_ BOOL *pFullscreen, _COM_Outptr_opt_result_maybenull_ IDXGIOutput **ppTarget) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDesc(_Out_ DXGI_SWAP_CHAIN_DESC *pDesc) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(_In_ const DXGI_MODE_DESC *pNewTargetParameters) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(_COM_Outptr_ IDXGIOutput **ppOutput) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(_Out_ DXGI_FRAME_STATISTICS *pStats) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(_Out_ UINT *pLastPresentCount) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgi1_2.h"
MIDL_INTERFACE("790a45f7-0d42-4876-983a-0a55cfe6f4aa")
IDXGISwapChain1 : public IDXGISwapChain
{
	virtual HRESULT STDMETHODCALLTYPE GetDesc1(_Out_ DXGI_SWAP_CHAIN_DESC1 *pDesc) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(_Out_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHwnd(_Out_ HWND *pHwnd) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(_In_ REFIID refiid, _COM_Outptr_ void **ppUnk) = 0;
	virtual HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, _In_ const DXGI_PRESENT_PARAMETERS *pPresentParameters) = 0;
	virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(_Out_ IDXGIOutput **ppRestrictToOutput) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(_In_ const DXGI_RGBA *pColor) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(_Out_ DXGI_RGBA *pColor) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetRotation(_In_ DXGI_MODE_ROTATION Rotation) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRotation(_Out_ DXGI_MODE_ROTATION *pRotation) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgi1_3.h"
MIDL_INTERFACE("a8be2ac4-199f-4946-b331-79599fb98de7")
IDXGISwapChain2 : public IDXGISwapChain1
{
	virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSourceSize(_Out_ UINT *pWidth, _Out_ UINT *pHeight) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(_Out_ UINT *pMaxLatency) = 0;
	virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(_Out_ DXGI_MATRIX_3X2_F *pMatrix) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "dxgi1_4.h"
MIDL_INTERFACE("94d99bdb-f1f8-4ab0-b236-7da0170edab1")
IDXGISwapChain3 : public IDXGISwapChain2
{
	virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace, _Out_ UINT *pColorSpaceSupport) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(_In_ UINT BufferCount, _In_ UINT Width, _In_ UINT Height, _In_ DXGI_FORMAT Format, _In_ UINT SwapChainFlags, _In_reads_(BufferCount) const UINT *pCreationNodeMask, _In_reads_(BufferCount) IUnknown *const *ppPresentQueue) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("7b7166ec-21c7-44ae-b21a-c9ae321ae369")
IDXGIFactory : public IDXGIObject
{
	virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, _COM_Outptr_ IDXGIAdapter **ppAdapter) = 0;
	virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(_Out_  HWND *pWindowHandle) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(_In_ IUnknown *pDevice, _In_ DXGI_SWAP_CHAIN_DESC *pDesc, _COM_Outptr_ IDXGISwapChain **ppSwapChain) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, _COM_Outptr_ IDXGIAdapter **ppAdapter) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("770aae78-f26f-4dba-a829-253c83d1b387")
IDXGIFactory1 : public IDXGIFactory
{
	virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, __out IDXGIAdapter1 **ppAdapter) = 0;
	virtual BOOL STDMETHODCALLTYPE IsCurrent(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
IDXGIFactory2 : public IDXGIFactory1
{
	virtual BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(_In_ IUnknown *pDevice, _In_ HWND hWnd, _In_ const DXGI_SWAP_CHAIN_DESC1 *pDesc, _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, _In_opt_ IDXGIOutput *pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1 **ppSwapChain) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(_In_ IUnknown *pDevice, _In_ IUnknown *pWindow, _In_ const DXGI_SWAP_CHAIN_DESC1 *pDesc, _In_opt_ IDXGIOutput *pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1 **ppSwapChain) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(_In_ HANDLE hResource, _Out_  LUID *pLuid) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(_In_ HWND WindowHandle, _In_ UINT wMsg, _Out_ DWORD *pdwCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(_In_ HANDLE hEvent, _Out_ DWORD *pdwCookie) = 0;
	virtual void STDMETHODCALLTYPE UnregisterStereoStatus(_In_ DWORD dwCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(_In_ HWND WindowHandle, _In_ UINT wMsg, _Out_ DWORD *pdwCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(_In_ HANDLE hEvent, _Out_ DWORD *pdwCookie) = 0;
	virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus(_In_ DWORD dwCookie) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(_In_ IUnknown *pDevice, _In_ const DXGI_SWAP_CHAIN_DESC1 *pDesc, _In_opt_ IDXGIOutput *pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1 **ppSwapChain) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("25483823-cd46-4c7d-86ca-47aa95b837bd")
IDXGIFactory3 : public IDXGIFactory2
{
	virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("1bc6ea02-ef36-464f-bf0c-21ca39e5168a")
IDXGIFactory4 : public IDXGIFactory3
{
	virtual HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(_In_ LUID AdapterLuid, _In_ REFIID riid, _COM_Outptr_ void **ppvAdapter) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumWarpAdapter(_In_ REFIID riid, _COM_Outptr_ void **ppvAdapter) = 0;
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

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("2411e7e1-12ac-4ccf-bd14-9798e8534dc0")
IDXGIAdapter : public IDXGIObject
{
	virtual HRESULT STDMETHODCALLTYPE EnumOutputs(UINT Output, __out IDXGIOutput **ppOutput) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDesc(__out DXGI_ADAPTER_DESC *pDesc) = 0;
	virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(__in REFGUID InterfaceName, __out LARGE_INTEGER *pUMDVersion) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("54ec77fa-1377-44e6-8c32-88fd5f44c84c")
IDXGIDevice : public IDXGIObject
{
	virtual HRESULT STDMETHODCALLTYPE GetAdapter(_COM_Outptr_ IDXGIAdapter **pAdapter) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSurface(_In_ const DXGI_SURFACE_DESC *pDesc, UINT NumSurfaces, DXGI_USAGE Usage, _In_opt_ const DXGI_SHARED_RESOURCE *pSharedResource, _COM_Outptr_ IDXGISurface **ppSurface) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryResourceResidency(_In_reads_(NumResources) IUnknown *const *ppResources, _Out_writes_(NumResources) DXGI_RESIDENCY *pResidencyStatus, UINT NumResources) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT Priority) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(_Out_ INT *pPriority) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3Dcommon.h"
MIDL_INTERFACE("8BA5FB08-5195-40e2-AC58-0D989C3A0102")
ID3D10Blob : public IUnknown
{
	virtual LPVOID STDMETHODCALLTYPE GetBufferPointer(void) = 0;
	virtual SIZE_T STDMETHODCALLTYPE GetBufferSize(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("c4fec28f-7966-4e95-9f94-f431cb56c3b8")
ID3D12Object : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID guid, _Inout_ UINT *pDataSize, _Out_writes_bytes_opt_( *pDataSize ) void *pData) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID guid, _In_ UINT DataSize, _In_reads_bytes_opt_(DataSize) const void *pData) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID guid, _In_opt_ const IUnknown *pData) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetName(_In_z_ LPCWSTR Name) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("905db94b-a00c-4140-9df5-2b64ca9ea357")
ID3D12DeviceChild : public ID3D12Object
{
	virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, _COM_Outptr_opt_ void **ppvDevice) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("c54a6b66-72df-4ee8-8be5-a946a1429214")
ID3D12RootSignature : public ID3D12DeviceChild
{
	// Nothing here
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("63ee58fb-1268-4835-86da-f008ce62f0d6")
ID3D12Pageable : public ID3D12DeviceChild
{
	// Nothing here
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("0d9658ae-ed45-469e-a61d-970ec583cab4")
ID3D12QueryHeap : public ID3D12Pageable
{
	// Nothing here
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("0a753dcf-c4d8-4b91-adf6-be5a60d95a76")
ID3D12Fence : public ID3D12Pageable
{
	virtual UINT64 STDMETHODCALLTYPE GetCompletedValue(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64 Value, HANDLE hEvent) = 0;
	virtual HRESULT STDMETHODCALLTYPE Signal(UINT64 Value) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("696442be-a72e-4059-bc79-5b5c98040fad")
ID3D12Resource : public ID3D12Pageable
{
	virtual HRESULT STDMETHODCALLTYPE Map(UINT Subresource, _In_opt_ const D3D12_RANGE *pReadRange, _Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource")) void **ppData) = 0;
	virtual void STDMETHODCALLTYPE Unmap(UINT Subresource, _In_opt_ const D3D12_RANGE *pWrittenRange) = 0;
	virtual D3D12_RESOURCE_DESC STDMETHODCALLTYPE GetDesc(void) = 0;
	virtual D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE GetGPUVirtualAddress( void) = 0;
	virtual HRESULT STDMETHODCALLTYPE WriteToSubresource(UINT DstSubresource, _In_opt_ const D3D12_BOX *pDstBox, _In_ const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReadFromSubresource(_Out_ void *pDstData, UINT DstRowPitch, UINT DstDepthPitch, UINT SrcSubresource, _In_opt_ const D3D12_BOX *pSrcBox) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetHeapProperties(_Out_opt_ D3D12_HEAP_PROPERTIES *pHeapProperties, _Out_opt_ D3D12_HEAP_FLAGS *pHeapFlags) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("765a30f3-f624-4c6f-a828-ace948622445")
ID3D12PipelineState : public ID3D12Pageable
{
	virtual HRESULT STDMETHODCALLTYPE GetCachedBlob(_COM_Outptr_ ID3DBlob* *ppBlob) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("8efb471d-616c-4f49-90f7-127bb763fa51")
ID3D12DescriptorHeap : public ID3D12Pageable
{
	virtual D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE GetDesc(void) = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetCPUDescriptorHandleForHeapStart(void) = 0;
	virtual D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetGPUDescriptorHandleForHeapStart(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("6102dee4-af59-4b09-b999-b44d73f09b24")
ID3D12CommandAllocator : public ID3D12Pageable
{
	virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("c36a797c-ec80-4f0a-8985-a7b2475082d1")
ID3D12CommandSignature : public ID3D12Pageable
{
	// Nothing here
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("7116d91c-e7e4-47ce-b8c6-ec8168f437e5")
ID3D12CommandList : public ID3D12DeviceChild
{
	virtual D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE GetType(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("5b160d0f-ac1b-4185-8ba8-b3ae42a5a455")
ID3D12GraphicsCommandList : public ID3D12CommandList
{
	virtual HRESULT STDMETHODCALLTYPE Close(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Reset(_In_ ID3D12CommandAllocator *pAllocator, _In_opt_ ID3D12PipelineState *pInitialState) = 0;
	virtual void STDMETHODCALLTYPE ClearState(_In_opt_ ID3D12PipelineState *pPipelineState) = 0;
	virtual void STDMETHODCALLTYPE DrawInstanced(_In_ UINT VertexCountPerInstance, _In_ UINT InstanceCount, _In_ UINT StartVertexLocation, _In_ UINT StartInstanceLocation) = 0;
	virtual void STDMETHODCALLTYPE DrawIndexedInstanced(_In_ UINT IndexCountPerInstance, _In_ UINT InstanceCount, _In_ UINT StartIndexLocation, _In_ INT BaseVertexLocation, _In_ UINT StartInstanceLocation) = 0;
	virtual void STDMETHODCALLTYPE Dispatch(_In_ UINT ThreadGroupCountX, _In_ UINT ThreadGroupCountY, _In_ UINT ThreadGroupCountZ) = 0;
	virtual void STDMETHODCALLTYPE CopyBufferRegion(_In_ ID3D12Resource *pDstBuffer, UINT64 DstOffset, _In_ ID3D12Resource *pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) = 0;
	virtual void STDMETHODCALLTYPE CopyTextureRegion(_In_ const D3D12_TEXTURE_COPY_LOCATION *pDst, UINT DstX, UINT DstY, UINT DstZ, _In_ const D3D12_TEXTURE_COPY_LOCATION *pSrc, _In_opt_ const D3D12_BOX *pSrcBox) = 0;
	virtual void STDMETHODCALLTYPE CopyResource(_In_ ID3D12Resource *pDstResource, _In_ ID3D12Resource *pSrcResource) = 0;
	virtual void STDMETHODCALLTYPE CopyTiles(_In_ ID3D12Resource *pTiledResource, _In_ const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate, _In_ const D3D12_TILE_REGION_SIZE *pTileRegionSize, _In_ ID3D12Resource *pBuffer, UINT64 BufferStartOffsetInBytes, D3D12_TILE_COPY_FLAGS Flags) = 0;
	virtual void STDMETHODCALLTYPE ResolveSubresource(_In_ ID3D12Resource *pDstResource, _In_ UINT DstSubresource, _In_ ID3D12Resource *pSrcResource, _In_ UINT SrcSubresource, _In_ DXGI_FORMAT Format) = 0;
	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(_In_ D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) = 0;
	virtual void STDMETHODCALLTYPE RSSetViewports(_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumViewports, _In_reads_( NumViewports) const D3D12_VIEWPORT *pViewports) = 0;
	virtual void STDMETHODCALLTYPE RSSetScissorRects(_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumRects, _In_reads_( NumRects) const D3D12_RECT *pRects) = 0;
	virtual void STDMETHODCALLTYPE OMSetBlendFactor(_In_opt_ const FLOAT BlendFactor[4]) = 0;
	virtual void STDMETHODCALLTYPE OMSetStencilRef(_In_ UINT StencilRef) = 0;
	virtual void STDMETHODCALLTYPE SetPipelineState(_In_ ID3D12PipelineState *pPipelineState) = 0;
	virtual void STDMETHODCALLTYPE ResourceBarrier(_In_ UINT NumBarriers, _In_reads_(NumBarriers) const D3D12_RESOURCE_BARRIER *pBarriers) = 0;
	virtual void STDMETHODCALLTYPE ExecuteBundle(_In_ ID3D12GraphicsCommandList *pCommandList) = 0;
	virtual void STDMETHODCALLTYPE SetDescriptorHeaps(_In_ UINT NumDescriptorHeaps, _In_reads_(NumDescriptorHeaps) ID3D12DescriptorHeap **ppDescriptorHeaps) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRootSignature(_In_ ID3D12RootSignature *pRootSignature) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRootSignature(_In_ ID3D12RootSignature *pRootSignature) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRootDescriptorTable(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRootDescriptorTable(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRoot32BitConstant(_In_ UINT RootParameterIndex, _In_ UINT SrcData, _In_ UINT DestOffsetIn32BitValues) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRoot32BitConstant(_In_ UINT RootParameterIndex, _In_ UINT SrcData, _In_ UINT DestOffsetIn32BitValues) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRoot32BitConstants(_In_ UINT RootParameterIndex, _In_ UINT Num32BitValuesToSet, _In_reads_(Num32BitValuesToSet * sizeof(UINT)) const void *pSrcData, _In_ UINT DestOffsetIn32BitValues) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRoot32BitConstants(_In_ UINT RootParameterIndex, _In_ UINT Num32BitValuesToSet, _In_reads_(Num32BitValuesToSet * sizeof(UINT)) const void *pSrcData, _In_ UINT DestOffsetIn32BitValues) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRootConstantBufferView(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRootConstantBufferView(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRootShaderResourceView(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRootShaderResourceView(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
	virtual void STDMETHODCALLTYPE SetComputeRootUnorderedAccessView(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
	virtual void STDMETHODCALLTYPE SetGraphicsRootUnorderedAccessView(_In_ UINT RootParameterIndex, _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
	virtual void STDMETHODCALLTYPE IASetIndexBuffer(_In_opt_ const D3D12_INDEX_BUFFER_VIEW *pView) = 0;
	virtual void STDMETHODCALLTYPE IASetVertexBuffers(_In_ UINT StartSlot, _In_ UINT NumViews, _In_reads_opt_(NumViews) const D3D12_VERTEX_BUFFER_VIEW *pViews) = 0;
	virtual void STDMETHODCALLTYPE SOSetTargets(_In_ UINT StartSlot, _In_ UINT NumViews, _In_reads_opt_(NumViews) const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews) = 0;
	virtual void STDMETHODCALLTYPE OMSetRenderTargets(_In_ UINT NumRenderTargetDescriptors, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors, _In_ BOOL RTsSingleHandleToDescriptorRange, _In_opt_ const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor) = 0;
	virtual void STDMETHODCALLTYPE ClearDepthStencilView(_In_ D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, _In_ D3D12_CLEAR_FLAGS ClearFlags, _In_ FLOAT Depth, _In_ UINT8 Stencil, _In_ UINT NumRects, _In_reads_(NumRects) const D3D12_RECT *pRects) = 0;
	virtual void STDMETHODCALLTYPE ClearRenderTargetView(_In_ D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, _In_ const FLOAT ColorRGBA[4], _In_ UINT NumRects, _In_reads_(NumRects) const D3D12_RECT *pRects) = 0;
	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(_In_ D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, _In_ D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, _In_ ID3D12Resource *pResource, _In_ const UINT Values[4], _In_ UINT NumRects, _In_reads_(NumRects) const D3D12_RECT *pRects) = 0;
	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(_In_ D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, _In_ D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, _In_ ID3D12Resource *pResource, _In_ const FLOAT Values[4], _In_ UINT NumRects, _In_reads_(NumRects) const D3D12_RECT *pRects) = 0;
	virtual void STDMETHODCALLTYPE DiscardResource(_In_ ID3D12Resource *pResource, _In_opt_ const D3D12_DISCARD_REGION *pRegion) = 0;
	virtual void STDMETHODCALLTYPE BeginQuery(_In_ ID3D12QueryHeap *pQueryHeap, _In_ D3D12_QUERY_TYPE Type, _In_ UINT Index) = 0;
	virtual void STDMETHODCALLTYPE EndQuery(_In_ ID3D12QueryHeap *pQueryHeap, _In_ D3D12_QUERY_TYPE Type, _In_ UINT Index) = 0;
	virtual void STDMETHODCALLTYPE ResolveQueryData(_In_ ID3D12QueryHeap *pQueryHeap, _In_ D3D12_QUERY_TYPE Type, _In_ UINT StartIndex, _In_ UINT NumQueries, _In_ ID3D12Resource *pDestinationBuffer, _In_ UINT64 AlignedDestinationBufferOffset) = 0;
	virtual void STDMETHODCALLTYPE SetPredication(_In_opt_ ID3D12Resource *pBuffer, _In_ UINT64 AlignedBufferOffset, _In_ D3D12_PREDICATION_OP Operation) = 0;
	virtual void STDMETHODCALLTYPE SetMarker(UINT Metadata, _In_reads_bytes_opt_(Size) const void *pData, UINT Size) = 0;
	virtual void STDMETHODCALLTYPE BeginEvent(UINT Metadata, _In_reads_bytes_opt_(Size) const void *pData, UINT Size) = 0;
	virtual void STDMETHODCALLTYPE EndEvent(void) = 0;
	virtual void STDMETHODCALLTYPE ExecuteIndirect(_In_ ID3D12CommandSignature *pCommandSignature, _In_ UINT MaxCommandCount, _In_ ID3D12Resource *pArgumentBuffer, _In_ UINT64 ArgumentBufferOffset, _In_opt_ ID3D12Resource *pCountBuffer, _In_ UINT64 CountBufferOffset) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("0ec870a6-5d7e-4c22-8cfc-5baae07616ed")
ID3D12CommandQueue : public ID3D12Pageable
{
	virtual void STDMETHODCALLTYPE UpdateTileMappings(_In_ ID3D12Resource *pResource, UINT NumResourceRegions, _In_reads_opt_(NumResourceRegions) const D3D12_TILED_RESOURCE_COORDINATE *pResourceRegionStartCoordinates, _In_reads_opt_(NumResourceRegions) const D3D12_TILE_REGION_SIZE *pResourceRegionSizes, _In_opt_ ID3D12Heap *pHeap, UINT NumRanges, _In_reads_opt_(NumRanges) const D3D12_TILE_RANGE_FLAGS *pRangeFlags, _In_reads_opt_(NumRanges) const UINT *pHeapRangeStartOffsets, _In_reads_opt_(NumRanges) const UINT *pRangeTileCounts, D3D12_TILE_MAPPING_FLAGS Flags) = 0;
	virtual void STDMETHODCALLTYPE CopyTileMappings(_In_ ID3D12Resource *pDstResource, _In_ const D3D12_TILED_RESOURCE_COORDINATE *pDstRegionStartCoordinate, _In_ ID3D12Resource *pSrcResource, _In_ const D3D12_TILED_RESOURCE_COORDINATE *pSrcRegionStartCoordinate, _In_ const D3D12_TILE_REGION_SIZE *pRegionSize, D3D12_TILE_MAPPING_FLAGS Flags) = 0;
	virtual void STDMETHODCALLTYPE ExecuteCommandLists(_In_ UINT NumCommandLists, _In_reads_(NumCommandLists) ID3D12CommandList *const *ppCommandLists) = 0;
	virtual void STDMETHODCALLTYPE SetMarker(UINT Metadata, _In_reads_bytes_opt_(Size) const void *pData, UINT Size) = 0;
	virtual void STDMETHODCALLTYPE BeginEvent(UINT Metadata, _In_reads_bytes_opt_(Size) const void *pData, UINT Size) = 0;
	virtual void STDMETHODCALLTYPE EndEvent(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Signal(ID3D12Fence *pFence, UINT64 Value) = 0;
	virtual HRESULT STDMETHODCALLTYPE Wait(ID3D12Fence *pFence, UINT64 Value) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetTimestampFrequency(_Out_ UINT64 *pFrequency) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetClockCalibration(_Out_ UINT64 *pGpuTimestamp, _Out_ UINT64 *pCpuTimestamp) = 0;
	virtual D3D12_COMMAND_QUEUE_DESC STDMETHODCALLTYPE GetDesc(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("189819f1-1db6-4b57-be54-1821339b85f7")
ID3D12Device : public ID3D12Object
{
	virtual UINT STDMETHODCALLTYPE GetNodeCount(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateCommandQueue(_In_ const D3D12_COMMAND_QUEUE_DESC *pDesc, REFIID riid, _COM_Outptr_ void **ppCommandQueue) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateCommandAllocator(_In_ D3D12_COMMAND_LIST_TYPE type, REFIID riid, _COM_Outptr_ void **ppCommandAllocator) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateGraphicsPipelineState(_In_ const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc, REFIID riid, _COM_Outptr_ void **ppPipelineState) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateComputePipelineState(_In_ const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc, REFIID riid, _COM_Outptr_ void **ppPipelineState) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateCommandList(_In_ UINT nodeMask, _In_ D3D12_COMMAND_LIST_TYPE type, _In_ ID3D12CommandAllocator *pCommandAllocator, _In_opt_ ID3D12PipelineState *pInitialState, REFIID riid, _COM_Outptr_ void **ppCommandList) = 0;
	virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(D3D12_FEATURE Feature, _Inout_updates_bytes_(FeatureSupportDataSize) void *pFeatureSupportData, UINT FeatureSupportDataSize) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateDescriptorHeap(_In_ const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc, REFIID riid, _COM_Outptr_ void **ppvHeap) = 0;
	virtual UINT STDMETHODCALLTYPE GetDescriptorHandleIncrementSize(_In_ D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateRootSignature(_In_ UINT nodeMask, _In_reads_(blobLengthInBytes) const void *pBlobWithRootSignature, _In_ SIZE_T blobLengthInBytes, REFIID riid, _COM_Outptr_ void **ppvRootSignature) = 0;
	virtual void STDMETHODCALLTYPE CreateConstantBufferView(_In_opt_ const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;
	virtual void STDMETHODCALLTYPE CreateShaderResourceView(_In_opt_ ID3D12Resource *pResource, _In_opt_ const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;
	virtual void STDMETHODCALLTYPE CreateUnorderedAccessView(_In_opt_ ID3D12Resource *pResource, _In_opt_ ID3D12Resource *pCounterResource, _In_opt_ const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;
	virtual void STDMETHODCALLTYPE CreateRenderTargetView(_In_opt_ ID3D12Resource *pResource, _In_opt_ const D3D12_RENDER_TARGET_VIEW_DESC *pDesc, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;
	virtual void STDMETHODCALLTYPE CreateDepthStencilView(_In_opt_ ID3D12Resource *pResource, _In_opt_ const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;
	virtual void STDMETHODCALLTYPE CreateSampler(_In_ const D3D12_SAMPLER_DESC *pDesc, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) = 0;
	virtual void STDMETHODCALLTYPE CopyDescriptors(_In_ UINT NumDestDescriptorRanges, _In_reads_(NumDestDescriptorRanges) const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts, _In_reads_opt_(NumDestDescriptorRanges) const UINT *pDestDescriptorRangeSizes, _In_ UINT NumSrcDescriptorRanges, _In_reads_(NumSrcDescriptorRanges) const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts, _In_reads_opt_(NumSrcDescriptorRanges) const UINT *pSrcDescriptorRangeSizes, _In_ D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) = 0;
	virtual void STDMETHODCALLTYPE CopyDescriptorsSimple(_In_ UINT NumDescriptors, _In_ D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart, _In_ D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, _In_ D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) = 0;
	virtual D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo(_In_ UINT visibleMask, _In_ UINT numResourceDescs, _In_reads_(numResourceDescs) const D3D12_RESOURCE_DESC *pResourceDescs) = 0;
	virtual D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE GetCustomHeapProperties(_In_ UINT nodeMask, D3D12_HEAP_TYPE heapType) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource(_In_ const D3D12_HEAP_PROPERTIES *pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, _In_ const D3D12_RESOURCE_DESC *pResourceDesc, D3D12_RESOURCE_STATES InitialResourceState, _In_opt_ const D3D12_CLEAR_VALUE *pOptimizedClearValue, REFIID riidResource, _COM_Outptr_opt_ void **ppvResource) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateHeap(_In_ const D3D12_HEAP_DESC *pDesc, REFIID riid, _COM_Outptr_opt_ void **ppvHeap) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreatePlacedResource(_In_ ID3D12Heap *pHeap, UINT64 HeapOffset, _In_ const D3D12_RESOURCE_DESC *pDesc, D3D12_RESOURCE_STATES InitialState, _In_opt_ const D3D12_CLEAR_VALUE *pOptimizedClearValue, REFIID riid, _COM_Outptr_opt_ void **ppvResource) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateReservedResource(_In_ const D3D12_RESOURCE_DESC *pDesc, D3D12_RESOURCE_STATES InitialState, _In_opt_ const D3D12_CLEAR_VALUE *pOptimizedClearValue, REFIID riid, _COM_Outptr_opt_ void **ppvResource) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle(_In_ ID3D12DeviceChild *pObject, _In_opt_ const SECURITY_ATTRIBUTES *pAttributes, DWORD Access, _In_opt_ LPCWSTR Name, _Out_ HANDLE *pHandle) = 0;
	virtual HRESULT STDMETHODCALLTYPE OpenSharedHandle(_In_ HANDLE NTHandle, REFIID riid, _COM_Outptr_opt_ void **ppvObj) = 0;
	virtual HRESULT STDMETHODCALLTYPE OpenSharedHandleByName(_In_ LPCWSTR Name, DWORD Access, _Out_ HANDLE *pNTHandle) = 0;
	virtual HRESULT STDMETHODCALLTYPE MakeResident(UINT NumObjects, _In_reads_(NumObjects) ID3D12Pageable *const *ppObjects) = 0;
	virtual HRESULT STDMETHODCALLTYPE Evict(UINT NumObjects, _In_reads_(NumObjects) ID3D12Pageable *const *ppObjects) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateFence(UINT64 InitialValue, D3D12_FENCE_FLAGS Flags, REFIID riid, _COM_Outptr_ void **ppFence) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void) = 0;
	virtual void STDMETHODCALLTYPE GetCopyableFootprints(_In_ const D3D12_RESOURCE_DESC *pResourceDesc, _In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource, _In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources, UINT64 BaseOffset, _Out_writes_opt_(NumSubresources) D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts, _Out_writes_opt_(NumSubresources) UINT *pNumRows, _Out_writes_opt_(NumSubresources) UINT64 *pRowSizeInBytes, _Out_opt_ UINT64 *pTotalBytes) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateQueryHeap(_In_ const D3D12_QUERY_HEAP_DESC *pDesc, REFIID riid, _COM_Outptr_opt_ void **ppvHeap) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetStablePowerState(BOOL Enable) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateCommandSignature(_In_ const D3D12_COMMAND_SIGNATURE_DESC *pDesc, _In_opt_ ID3D12RootSignature *pRootSignature, REFIID riid, _COM_Outptr_opt_ void **ppvCommandSignature) = 0;
	virtual void STDMETHODCALLTYPE GetResourceTiling(_In_ ID3D12Resource *pTiledResource, _Out_opt_ UINT *pNumTilesForEntireResource, _Out_opt_ D3D12_PACKED_MIP_INFO *pPackedMipDesc, _Out_opt_ D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips, _Inout_opt_ UINT *pNumSubresourceTilings, _In_ UINT FirstSubresourceTilingToGet, _Out_writes_(*pNumSubresourceTilings) D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips) = 0;
	virtual LUID STDMETHODCALLTYPE GetAdapterLuid(void) = 0;
};

#ifdef RHI_DEBUG
	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3d12sdklayers.h"
	MIDL_INTERFACE("344488b7-6846-474b-b989-f027448245e0")
	ID3D12Debug : public IUnknown
	{
		virtual void STDMETHODCALLTYPE EnableDebugLayer(void) = 0;
	};

	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3d12sdklayers.h"
	MIDL_INTERFACE("3febd6dd-4973-4787-8194-e45f9e28923e")
	ID3D12DebugDevice : public IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE SetFeatureMask(D3D12_DEBUG_FEATURE Mask) = 0;
		virtual D3D12_DEBUG_FEATURE STDMETHODCALLTYPE GetFeatureMask(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE ReportLiveDeviceObjects(D3D12_RLDO_FLAGS Flags) = 0;
	};
#endif




//[-------------------------------------------------------]
//[ Direct3D12Rhi/D3D12X.h                                ]
//[-------------------------------------------------------]
// TODO(co) Remove unused stuff when done with the Direct3D 12 RHI implementation


//[-------------------------------------------------------]
//[ Global variables                                      ]
//[-------------------------------------------------------]
struct CD3DX12_DEFAULT {};
extern const DECLSPEC_SELECTANY CD3DX12_DEFAULT D3D12_DEFAULT;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
//------------------------------------------------------------------------------------------------
struct CD3DX12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
{

	CD3DX12_CPU_DESCRIPTOR_HANDLE()
	{}

	explicit CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE &o) :
		D3D12_CPU_DESCRIPTOR_HANDLE(o)
	{}

	CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &other, INT offsetScaledByIncrementSize)
	{
		InitOffsetted(other, offsetScaledByIncrementSize);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &other, INT offsetInDescriptors, UINT descriptorIncrementSize)
	{
		InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize)
	{
		ptr += offsetInDescriptors * descriptorIncrementSize;
		return *this;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize) 
	{
		ptr += offsetScaledByIncrementSize;
		return *this;
	}

	[[nodiscard]] bool operator==(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other)
	{
		return (ptr == other.ptr);
	}

	[[nodiscard]] bool operator!=(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other)
	{
		return (ptr != other.ptr);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE &operator=(const D3D12_CPU_DESCRIPTOR_HANDLE &other)
	{
		ptr = other.ptr;
		return *this;
	}

	inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetScaledByIncrementSize)
	{
		InitOffsetted(*this, base, offsetScaledByIncrementSize);
	}

	inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetInDescriptors, UINT descriptorIncrementSize)
	{
		InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
	}

	static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE &handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetScaledByIncrementSize)
	{
		handle.ptr = base.ptr + offsetScaledByIncrementSize;
	}

	static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE &handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetInDescriptors, UINT descriptorIncrementSize)
	{
		handle.ptr = base.ptr + offsetInDescriptors * descriptorIncrementSize;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
	CD3DX12_RESOURCE_BARRIER()
	{}

	explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER &o) :
		D3D12_RESOURCE_BARRIER(o)
	{}

	[[nodiscard]] static inline CD3DX12_RESOURCE_BARRIER Transition(
		_In_ ID3D12Resource* pResource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE)
	{
		CD3DX12_RESOURCE_BARRIER result = {};
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		result.Flags = flags;
		barrier.Transition.pResource = pResource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;
		barrier.Transition.Subresource = subresource;
		return result;
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_BARRIER Aliasing(
		_In_ ID3D12Resource* pResourceBefore,
		_In_ ID3D12Resource* pResourceAfter)
	{
		CD3DX12_RESOURCE_BARRIER result = {};
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Aliasing.pResourceBefore = pResourceBefore;
		barrier.Aliasing.pResourceAfter = pResourceAfter;
		return result;
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_BARRIER UAV(_In_ ID3D12Resource* pResource)
	{
		CD3DX12_RESOURCE_BARRIER result = {};
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = pResource;
		return result;
	}

	[[nodiscard]] operator const D3D12_RESOURCE_BARRIER&() const
	{
		return *this;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
{
	CD3DX12_HEAP_PROPERTIES()
	{}

	explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES &o) :
		D3D12_HEAP_PROPERTIES(o)
	{}

	CD3DX12_HEAP_PROPERTIES( 
		D3D12_CPU_PAGE_PROPERTY cpuPageProperty, 
		D3D12_MEMORY_POOL memoryPoolPreference,
		UINT creationNodeMask = 1, 
		UINT nodeMask = 1 )
	{
		Type = D3D12_HEAP_TYPE_CUSTOM;
		CPUPageProperty = cpuPageProperty;
		MemoryPoolPreference = memoryPoolPreference;
		CreationNodeMask = creationNodeMask;
		VisibleNodeMask = nodeMask;
	}

	explicit CD3DX12_HEAP_PROPERTIES( 
		D3D12_HEAP_TYPE type, 
		UINT creationNodeMask = 1, 
		UINT nodeMask = 1 )
	{
		Type = type;
		CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		CreationNodeMask = creationNodeMask;
		VisibleNodeMask = nodeMask;
	}

	[[nodiscard]] operator const D3D12_HEAP_PROPERTIES&() const
	{
		return *this;
	}

	[[nodiscard]] bool IsCPUAccessible() const
	{
		return Type == D3D12_HEAP_TYPE_UPLOAD || Type == D3D12_HEAP_TYPE_READBACK || (Type == D3D12_HEAP_TYPE_CUSTOM &&
			(CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE || CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC
{
	CD3DX12_RESOURCE_DESC()
	{}

	explicit CD3DX12_RESOURCE_DESC( const D3D12_RESOURCE_DESC& o ) :
		D3D12_RESOURCE_DESC( o )
	{}

	CD3DX12_RESOURCE_DESC( 
		D3D12_RESOURCE_DIMENSION dimension,
		UINT64 alignment,
		UINT64 width,
		UINT height,
		UINT16 depthOrArraySize,
		UINT16 mipLevels,
		DXGI_FORMAT format,
		UINT sampleCount,
		UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout,
		D3D12_RESOURCE_FLAGS flags )
	{
		Dimension = dimension;
		Alignment = alignment;
		Width = width;
		Height = height;
		DepthOrArraySize = depthOrArraySize;
		MipLevels = mipLevels;
		Format = format;
		SampleDesc.Count = sampleCount;
		SampleDesc.Quality = sampleQuality;
		Layout = layout;
		Flags = flags;
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_DESC Buffer( 
		const D3D12_RESOURCE_ALLOCATION_INFO& resAllocInfo,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE )
	{
		return CD3DX12_RESOURCE_DESC( D3D12_RESOURCE_DIMENSION_BUFFER, resAllocInfo.Alignment, resAllocInfo.SizeInBytes, 
			1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags );
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_DESC Buffer( 
		UINT64 width,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		UINT64 alignment = 0 )
	{
		return CD3DX12_RESOURCE_DESC( D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1, 
			DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags );
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_DESC Tex1D( 
		DXGI_FORMAT format,
		UINT64 width,
		UINT16 arraySize = 1,
		UINT16 mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0 )
	{
		return CD3DX12_RESOURCE_DESC( D3D12_RESOURCE_DIMENSION_TEXTURE1D, alignment, width, 1, arraySize, 
			mipLevels, format, 1, 0, layout, flags );
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_DESC Tex2D( 
		DXGI_FORMAT format,
		UINT64 width,
		UINT height,
		UINT16 arraySize = 1,
		UINT16 mipLevels = 0,
		UINT sampleCount = 1,
		UINT sampleQuality = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0 )
	{
		return CD3DX12_RESOURCE_DESC( D3D12_RESOURCE_DIMENSION_TEXTURE2D, alignment, width, height, arraySize, 
			mipLevels, format, sampleCount, sampleQuality, layout, flags );
	}

	[[nodiscard]] static inline CD3DX12_RESOURCE_DESC Tex3D( 
		DXGI_FORMAT format,
		UINT64 width,
		UINT height,
		UINT16 depth,
		UINT16 mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0 )
	{
		return CD3DX12_RESOURCE_DESC( D3D12_RESOURCE_DIMENSION_TEXTURE3D, alignment, width, height, depth, 
			mipLevels, format, 1, 0, layout, flags );
	}

	[[nodiscard]] inline UINT16 Depth() const
	{
		return (Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1u);
	}

	[[nodiscard]] inline UINT16 ArraySize() const
	{
		return (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1u);
	}

	[[nodiscard]] inline UINT8 PlaneCount(_In_ ID3D12Device*) const
	{
		// TODO(co) Implement me
		// return D3D12GetFormatPlaneCount(pDevice, Format);
		return 0;
	}

	[[nodiscard]] inline UINT Subresources(_In_ ID3D12Device* pDevice) const
	{
		return static_cast<UINT>(MipLevels * ArraySize() * PlaneCount(pDevice));
	}

	[[nodiscard]] inline UINT CalcSubresource(UINT, UINT, UINT)
	{
		// TODO(co) Implement me
		// return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, MipLevels, ArraySize());
		return 0;
	}

	[[nodiscard]] operator const D3D12_RESOURCE_DESC&() const
	{
		return *this;
	}
};

[[nodiscard]] inline bool operator==( const D3D12_RESOURCE_DESC& l, const D3D12_RESOURCE_DESC& r )
{
	return l.Dimension == r.Dimension &&
		l.Alignment == r.Alignment &&
		l.Width == r.Width &&
		l.Height == r.Height &&
		l.DepthOrArraySize == r.DepthOrArraySize &&
		l.MipLevels == r.MipLevels &&
		l.Format == r.Format &&
		l.SampleDesc.Count == r.SampleDesc.Count &&
		l.SampleDesc.Quality == r.SampleDesc.Quality &&
		l.Layout == r.Layout &&
		l.Flags == r.Flags;
}

[[nodiscard]] inline bool operator!=( const D3D12_RESOURCE_DESC& l, const D3D12_RESOURCE_DESC& r )
{
	return !( l == r );
}

//------------------------------------------------------------------------------------------------
struct CD3DX12_RANGE : public D3D12_RANGE
{
	CD3DX12_RANGE()
	{}

	explicit CD3DX12_RANGE(const D3D12_RANGE &o) :
		D3D12_RANGE(o)
	{}

	CD3DX12_RANGE( 
		SIZE_T begin, 
		SIZE_T end )
	{
		Begin = begin;
		End = end;
	}

	[[nodiscard]] operator const D3D12_RANGE&() const
	{
		return *this;
	}

};

//------------------------------------------------------------------------------------------------
struct CD3DX12_DESCRIPTOR_RANGE : public D3D12_DESCRIPTOR_RANGE
{
	CD3DX12_DESCRIPTOR_RANGE()
	{}

	explicit CD3DX12_DESCRIPTOR_RANGE(const D3D12_DESCRIPTOR_RANGE &o) :
		D3D12_DESCRIPTOR_RANGE(o)
	{}

	CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpace = 0,
		UINT offsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		Init(rangeType, numDescriptors, baseShaderRegister, registerSpace, offsetInDescriptorsFromTableStart);
	}

	inline void Init(
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpace = 0,
		UINT offsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		Init(*this, rangeType, numDescriptors, baseShaderRegister, registerSpace, offsetInDescriptorsFromTableStart);
	}

	static inline void Init(
		_Out_ D3D12_DESCRIPTOR_RANGE &range,
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpace = 0,
		UINT offsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		range.RangeType = rangeType;
		range.NumDescriptors = numDescriptors;
		range.BaseShaderRegister = baseShaderRegister;
		range.RegisterSpace = registerSpace;
		range.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_ROOT_DESCRIPTOR_TABLE : public D3D12_ROOT_DESCRIPTOR_TABLE
{
	CD3DX12_ROOT_DESCRIPTOR_TABLE()
	{}
	explicit CD3DX12_ROOT_DESCRIPTOR_TABLE(const D3D12_ROOT_DESCRIPTOR_TABLE &o) :
		D3D12_ROOT_DESCRIPTOR_TABLE(o)
	{}
	CD3DX12_ROOT_DESCRIPTOR_TABLE(
		UINT numDescriptorRanges,
		_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* _pDescriptorRanges)
	{
		Init(numDescriptorRanges, _pDescriptorRanges);
	}

	inline void Init(
		UINT numDescriptorRanges,
		_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* _pDescriptorRanges)
	{
		Init(*this, numDescriptorRanges, _pDescriptorRanges);
	}

	static inline void Init(
		_Out_ D3D12_ROOT_DESCRIPTOR_TABLE &rootDescriptorTable,
		UINT numDescriptorRanges,
		_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* _pDescriptorRanges)
	{
		rootDescriptorTable.NumDescriptorRanges = numDescriptorRanges;
		rootDescriptorTable.pDescriptorRanges = _pDescriptorRanges;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_ROOT_CONSTANTS : public D3D12_ROOT_CONSTANTS
{
	CD3DX12_ROOT_CONSTANTS()
	{}
	explicit CD3DX12_ROOT_CONSTANTS(const D3D12_ROOT_CONSTANTS &o) :
		D3D12_ROOT_CONSTANTS(o)
	{}
	CD3DX12_ROOT_CONSTANTS(
		UINT num32BitValues,
		UINT shaderRegister,
		UINT registerSpace = 0)
	{
		Init(num32BitValues, shaderRegister, registerSpace);
	}

	inline void Init(
		UINT num32BitValues,
		UINT shaderRegister,
		UINT registerSpace = 0)
	{
		Init(*this, num32BitValues, shaderRegister, registerSpace);
	}

	static inline void Init(
		_Out_ D3D12_ROOT_CONSTANTS &rootConstants,
		UINT num32BitValues,
		UINT shaderRegister,
		UINT registerSpace = 0)
	{
		rootConstants.Num32BitValues = num32BitValues;
		rootConstants.ShaderRegister = shaderRegister;
		rootConstants.RegisterSpace = registerSpace;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_ROOT_DESCRIPTOR : public D3D12_ROOT_DESCRIPTOR
{
	CD3DX12_ROOT_DESCRIPTOR()
	{}
	explicit CD3DX12_ROOT_DESCRIPTOR(const D3D12_ROOT_DESCRIPTOR &o) :
		D3D12_ROOT_DESCRIPTOR(o)
	{}
	CD3DX12_ROOT_DESCRIPTOR(
		UINT shaderRegister,
		UINT registerSpace = 0)
	{
		Init(shaderRegister, registerSpace);
	}

	inline void Init(
		UINT shaderRegister,
		UINT registerSpace = 0)
	{
		Init(*this, shaderRegister, registerSpace);
	}

	static inline void Init(_Out_ D3D12_ROOT_DESCRIPTOR &table, UINT shaderRegister, UINT registerSpace = 0)
	{
		table.ShaderRegister = shaderRegister;
		table.RegisterSpace = registerSpace;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_ROOT_PARAMETER : public D3D12_ROOT_PARAMETER
{
	CD3DX12_ROOT_PARAMETER() {}
	explicit CD3DX12_ROOT_PARAMETER(const D3D12_ROOT_PARAMETER &o) :
		D3D12_ROOT_PARAMETER(o)
	{}

	static inline void InitAsDescriptorTable(
		_Out_ D3D12_ROOT_PARAMETER &rootParam,
		UINT numDescriptorRanges,
		_In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = visibility;
		CD3DX12_ROOT_DESCRIPTOR_TABLE::Init(rootParam.DescriptorTable, numDescriptorRanges, pDescriptorRanges);
	}

	static inline void InitAsConstants(
		_Out_ D3D12_ROOT_PARAMETER &rootParam,
		UINT num32BitValues,
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParam.ShaderVisibility = visibility;
		CD3DX12_ROOT_CONSTANTS::Init(rootParam.Constants, num32BitValues, shaderRegister, registerSpace);
	}

	static inline void InitAsConstantBufferView(
		_Out_ D3D12_ROOT_PARAMETER &rootParam,
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.ShaderVisibility = visibility;
		CD3DX12_ROOT_DESCRIPTOR::Init(rootParam.Descriptor, shaderRegister, registerSpace);
	}

	static inline void InitAsShaderResourceView(
		_Out_ D3D12_ROOT_PARAMETER &rootParam,
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam.ShaderVisibility = visibility;
		CD3DX12_ROOT_DESCRIPTOR::Init(rootParam.Descriptor, shaderRegister, registerSpace);
	}

	static inline void InitAsUnorderedAccessView(
		_Out_ D3D12_ROOT_PARAMETER &rootParam,
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		rootParam.ShaderVisibility = visibility;
		CD3DX12_ROOT_DESCRIPTOR::Init(rootParam.Descriptor, shaderRegister, registerSpace);
	}

	inline void InitAsDescriptorTable(
		UINT numDescriptorRanges,
		_In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsDescriptorTable(*this, numDescriptorRanges, pDescriptorRanges, visibility);
	}

	inline void InitAsConstants(
		UINT num32BitValues,
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsConstants(*this, num32BitValues, shaderRegister, registerSpace, visibility);
	}

	inline void InitAsConstantBufferView(
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsConstantBufferView(*this, shaderRegister, registerSpace, visibility);
	}

	inline void InitAsShaderResourceView(
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsShaderResourceView(*this, shaderRegister, registerSpace, visibility);
	}

	inline void InitAsUnorderedAccessView(
		UINT shaderRegister,
		UINT registerSpace = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsUnorderedAccessView(*this, shaderRegister, registerSpace, visibility);
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_ROOT_SIGNATURE_DESC : public D3D12_ROOT_SIGNATURE_DESC
{
	CD3DX12_ROOT_SIGNATURE_DESC()
	{}

	explicit CD3DX12_ROOT_SIGNATURE_DESC(const D3D12_ROOT_SIGNATURE_DESC &o) :
		D3D12_ROOT_SIGNATURE_DESC(o)
	{}

	CD3DX12_ROOT_SIGNATURE_DESC(
		UINT numParameters,
		_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		Init(numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
	}

	CD3DX12_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT)
	{
		Init(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	}

	inline void Init(
		UINT numParameters,
		_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		Init(*this, numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
	}

	static inline void Init(
		_Out_ D3D12_ROOT_SIGNATURE_DESC &desc,
		UINT numParameters,
		_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = NULL,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		desc.NumParameters = numParameters;
		desc.pParameters = _pParameters;
		desc.NumStaticSamplers = numStaticSamplers;
		desc.pStaticSamplers = _pStaticSamplers;
		desc.Flags = flags;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_RASTERIZER_DESC : public D3D12_RASTERIZER_DESC
{
	CD3DX12_RASTERIZER_DESC()
	{}

	explicit CD3DX12_RASTERIZER_DESC( const D3D12_RASTERIZER_DESC& o ) :
		D3D12_RASTERIZER_DESC( o )
	{}

	explicit CD3DX12_RASTERIZER_DESC( CD3DX12_DEFAULT )
	{
		FillMode = D3D12_FILL_MODE_SOLID;
		CullMode = D3D12_CULL_MODE_BACK;
		FrontCounterClockwise = FALSE;
		DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		DepthClipEnable = TRUE;
		MultisampleEnable = FALSE;
		AntialiasedLineEnable = FALSE;
		ForcedSampleCount = 0;
		ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	explicit CD3DX12_RASTERIZER_DESC(
		D3D12_FILL_MODE fillMode,
		D3D12_CULL_MODE cullMode,
		BOOL frontCounterClockwise,
		INT depthBias,
		FLOAT depthBiasClamp,
		FLOAT slopeScaledDepthBias,
		BOOL depthClipEnable,
		BOOL multisampleEnable,
		BOOL antialiasedLineEnable, 
		UINT forcedSampleCount, 
		D3D12_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster)
	{
		FillMode = fillMode;
		CullMode = cullMode;
		FrontCounterClockwise = frontCounterClockwise;
		DepthBias = depthBias;
		DepthBiasClamp = depthBiasClamp;
		SlopeScaledDepthBias = slopeScaledDepthBias;
		DepthClipEnable = depthClipEnable;
		MultisampleEnable = multisampleEnable;
		AntialiasedLineEnable = antialiasedLineEnable;
		ForcedSampleCount = forcedSampleCount;
		ConservativeRaster = conservativeRaster;
	}

	~CD3DX12_RASTERIZER_DESC()
	{}

	[[nodiscard]] operator const D3D12_RASTERIZER_DESC&() const
	{
		return *this;
	}

};

//------------------------------------------------------------------------------------------------
struct CD3DX12_BLEND_DESC : public D3D12_BLEND_DESC
{
	CD3DX12_BLEND_DESC()
	{}

	explicit CD3DX12_BLEND_DESC( const D3D12_BLEND_DESC& o ) :
		D3D12_BLEND_DESC( o )
	{}

	explicit CD3DX12_BLEND_DESC( CD3DX12_DEFAULT )
	{
		AlphaToCoverageEnable = FALSE;
		IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			RenderTarget[ i ] = defaultRenderTargetBlendDesc;
	}

	~CD3DX12_BLEND_DESC()
	{}

	[[nodiscard]] operator const D3D12_BLEND_DESC&() const
	{
		return *this;
	}
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_BOX : public D3D12_BOX
{
	CD3DX12_BOX()
	{}
	explicit CD3DX12_BOX( const D3D12_BOX& o ) :
		D3D12_BOX( o )
	{}
	explicit CD3DX12_BOX(
		UINT Left,
		UINT Right )
	{
		left = Left;
		top = 0;
		front = 0;
		right = Right;
		bottom = 1;
		back = 1;
	}
	explicit CD3DX12_BOX(
		UINT Left,
		UINT Top,
		UINT Right,
		UINT Bottom )
	{
		left = Left;
		top = Top;
		front = 0;
		right = Right;
		bottom = Bottom;
		back = 1;
	}
	explicit CD3DX12_BOX(
		UINT Left,
		UINT Top,
		UINT Front,
		UINT Right,
		UINT Bottom,
		UINT Back )
	{
		left = Left;
		top = Top;
		front = Front;
		right = Right;
		bottom = Bottom;
		back = Back;
	}
	~CD3DX12_BOX() {}
	[[nodiscard]] operator const D3D12_BOX&() const
	{
		return *this;
	}
};
[[nodiscard]] inline bool operator==( const D3D12_BOX& l, const D3D12_BOX& r )
{
	return l.left == r.left && l.top == r.top && l.front == r.front &&
		l.right == r.right && l.bottom == r.bottom && l.back == r.back;
}
[[nodiscard]] inline bool operator!=( const D3D12_BOX& l, const D3D12_BOX& r )
{ return !( l == r ); }

//------------------------------------------------------------------------------------------------
struct CD3DX12_TEXTURE_COPY_LOCATION : public D3D12_TEXTURE_COPY_LOCATION
{
	CD3DX12_TEXTURE_COPY_LOCATION()
	{}
	explicit CD3DX12_TEXTURE_COPY_LOCATION(const D3D12_TEXTURE_COPY_LOCATION &o) :
		D3D12_TEXTURE_COPY_LOCATION(o)
	{}
	CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes)
	{
		pResource = pRes;
	}
	CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& Footprint)
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		PlacedFootprint = Footprint;
	}
	CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes, UINT Sub)
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		SubresourceIndex = Sub;
	}
};

//------------------------------------------------------------------------------------------------
// Returns required size of a buffer to be used for data upload
[[nodiscard]] inline UINT64 GetRequiredIntermediateSize(
	_In_ ID3D12Resource* pDestinationResource,
	_In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
	_In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources)
{
	D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
	UINT64 RequiredSize = 0;

	ID3D12Device* pDevice = nullptr;
	pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
	pDevice->Release();

	return RequiredSize;
}

//------------------------------------------------------------------------------------------------
// Row-by-row memcpy
inline void MemcpySubresource(
	_In_ const D3D12_MEMCPY_DEST* pDest,
	_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
	SIZE_T RowSizeInBytes,
	UINT NumRows,
	UINT NumSlices)
{
	for (UINT z = 0; z < NumSlices; ++z)
	{
		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
		for (UINT y = 0; y < NumRows; ++y)
		{
			memcpy(pDestSlice + pDest->RowPitch * y,
					pSrcSlice + pSrc->RowPitch * y,
					RowSizeInBytes);
		}
	}
}

//------------------------------------------------------------------------------------------------
// All arrays must be populated (e.g. by calling GetCopyableFootprints)
[[nodiscard]] inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	_In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
	_In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
	UINT64 RequiredSize,
	_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	_In_reads_(NumSubresources) const UINT* pNumRows,
	_In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
	_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData)
{
	// Minor validation
	D3D12_RESOURCE_DESC IntermediateDesc = pIntermediate->GetDesc();
	D3D12_RESOURCE_DESC DestinationDesc = pDestinationResource->GetDesc();
	if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || 
		IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset || 
		RequiredSize > (SIZE_T)-1 || 
		(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && 
			(FirstSubresource != 0 || NumSubresources != 1)))
	{
		return 0;
	}

	BYTE* pData;
	HRESULT hr = pIntermediate->Map(0, NULL, reinterpret_cast<void**>(&pData));
	if (FAILED(hr))
	{
		return 0;
	}

	for (UINT i = 0; i < NumSubresources; ++i)
	{
		if (pRowSizesInBytes[i] > (SIZE_T)-1) return 0;
		D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
		MemcpySubresource(&DestData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
	}
	pIntermediate->Unmap(0, NULL);

	if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		CD3DX12_BOX SrcBox( UINT( pLayouts[0].Offset ), UINT( pLayouts[0].Offset + pLayouts[0].Footprint.Width ) );
		pCmdList->CopyBufferRegion(
			pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
	}
	else
	{
		for (UINT i = 0; i < NumSubresources; ++i)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Dst(pDestinationResource, i + FirstSubresource);
			CD3DX12_TEXTURE_COPY_LOCATION Src(pIntermediate, pLayouts[i]);
			pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return RequiredSize;
}

//------------------------------------------------------------------------------------------------
// Heap-allocating UpdateSubresources implementation
[[nodiscard]] inline UINT64 UpdateSubresources( 
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	UINT64 IntermediateOffset,
	_In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
	_In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
	_In_reads_(NumSubresources) D3D12_SUBRESOURCE_DATA* pSrcData)
{
	UINT64 RequiredSize = 0;
	UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
	if (MemToAlloc > SIZE_MAX)
	{
		return 0;
	}
	void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
	if (pMem == NULL)
	{
		return 0;
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
	UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

	D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
	ID3D12Device* pDevice = nullptr;
	pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
	pDevice->Release();

	UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
	HeapFree(GetProcessHeap(), 0, pMem);
	return Result;
}




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Direct3D12Rhi
{
	class VertexArray;
	class RootSignature;
	class Direct3D12RuntimeLinking;
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
		RHI_ASSERT(mContext, &rhiReference == &(resourceReference).getRhi(), "Direct3D 12 error: The given resource is owned by another RHI instance")

	/*
	*  @brief
	*    Debug break on execution failure
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
	*    Debug break on execution failure
	*/
	#define FAILED_DEBUG_BREAK(toExecute) toExecute;
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
		static constexpr const char* HLSL_NAME = "HLSL";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)
		static constexpr const uint32_t NUMBER_OF_BUFFERED_FRAMES = 2;


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		template <typename T0, typename T1>
		inline T0 align(const T0 x, const T1 a)
		{
			return (x + (a-1)) & (T0)(~(a-1));
		}

		inline void updateWidthHeight(uint32_t mipmapIndex, uint32_t textureWidth, uint32_t textureHeight, uint32_t& width, uint32_t& height)
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
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		class UploadCommandListAllocator final
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline UploadCommandListAllocator() :
				mD3D12Device(nullptr),
				mD3D12CommandAllocator(nullptr),
				mD3D12GraphicsCommandList(nullptr),
				mD3D12ResourceUploadBuffer(nullptr),
				mD3D12GpuVirtualAddress{},
				mData(nullptr),
				mOffset(0),
				mNumberOfUploadBufferBytes(0)
			{}
	
			inline ID3D12GraphicsCommandList* getD3D12GraphicsCommandList() const
			{
				return mD3D12GraphicsCommandList;
			}

			inline ID3D12Resource* getD3D12ResourceUploadBuffer() const
			{
				return mD3D12ResourceUploadBuffer;
			}

			inline uint8_t* getData() const
			{
				return mData;
			}

			void create(ID3D12Device& d3d12Device)
			{
				mD3D12Device = &d3d12Device;
				[[maybe_unused]] HRESULT result = d3d12Device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(&mD3D12CommandAllocator));
				ASSERT(SUCCEEDED(result), "Direct3D 12 create command allocator failed")

				// Create the command list
				result = d3d12Device.CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mD3D12CommandAllocator, nullptr, IID_PPV_ARGS(&mD3D12GraphicsCommandList));
				ASSERT(SUCCEEDED(result), "Direct3D 12 create command list failed")

				// Command lists are created in the recording state, but there is nothing to record yet. The main loop expects it to be closed, so close it now.
				result = mD3D12GraphicsCommandList->Close();
				ASSERT(SUCCEEDED(result), "Direct3D 12 close command list failed")
			}

			void destroy()
			{
				if (nullptr != mD3D12GraphicsCommandList)
				{
					mD3D12GraphicsCommandList->Release();
				}
				if (nullptr != mD3D12CommandAllocator)
				{
					mD3D12CommandAllocator->Release();
				}
				if (nullptr != mD3D12ResourceUploadBuffer)
				{
					mD3D12ResourceUploadBuffer->Release();
				}
			}

			void begin(uint32_t numberOfUploadBufferBytes)
			{
				ASSERT(nullptr != mD3D12Device, "Invalid Direct3D 12 device")
				mD3D12CommandAllocator->Reset();
				mD3D12GraphicsCommandList->Reset(mD3D12CommandAllocator, nullptr);
				if (numberOfUploadBufferBytes != mNumberOfUploadBufferBytes)
				{
					mNumberOfUploadBufferBytes = numberOfUploadBufferBytes;
					if (nullptr != mD3D12ResourceUploadBuffer)
					{
						mD3D12ResourceUploadBuffer->Release();
					}
					mD3D12ResourceUploadBuffer = createBuffer(*mD3D12Device, D3D12_HEAP_TYPE_UPLOAD, numberOfUploadBufferBytes);
				}
				mOffset = 0;
				mData = nullptr;
			}

			void end()
			{
				if (nullptr != mData)
				{
					const D3D12_RANGE d3d12Range = { 0, mOffset };
					mD3D12ResourceUploadBuffer->Unmap(0, &d3d12Range);
				}
				[[maybe_unused]] HRESULT result = mD3D12GraphicsCommandList->Close();
				ASSERT(SUCCEEDED(result), "Direct3D 12 close command list failed")
			}

			uint32_t allocateUploadBuffer(uint32_t size, uint32_t alignment)
			{
				const uint32_t alignedOffset = align(mOffset, alignment);
				if (alignedOffset + size > mNumberOfUploadBufferBytes)
				{
					// TODO(co) Reallocate
					ASSERT(false, "Direct3D 12 allocate upload buffer failed")
				}
				if (nullptr == mData)
				{
					mD3D12GpuVirtualAddress = mD3D12ResourceUploadBuffer->GetGPUVirtualAddress();
					const D3D12_RANGE d3d12Range = { 0, 0 };
					[[maybe_unused]] HRESULT result = mD3D12ResourceUploadBuffer->Map(0, &d3d12Range, reinterpret_cast<void**>(&mData));
					ASSERT(SUCCEEDED(result), "Direct3D 12 map buffer failed")
				}
				mOffset = alignedOffset + size;
				return alignedOffset;
			}


		//[-------------------------------------------------------]
		//[ Private static methods                                ]
		//[-------------------------------------------------------]
		private:
			static ID3D12Resource* createBuffer(ID3D12Device& d3d12Device, D3D12_HEAP_TYPE d3dHeapType, size_t numberOfBytes)
			{
				D3D12_HEAP_PROPERTIES d3d12HeapProperties = {};
				d3d12HeapProperties.Type = d3dHeapType;

				D3D12_RESOURCE_DESC d3d12ResourceDesc = {};
				d3d12ResourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
				d3d12ResourceDesc.Width            = numberOfBytes;
				d3d12ResourceDesc.Height           = 1;
				d3d12ResourceDesc.DepthOrArraySize = 1;
				d3d12ResourceDesc.MipLevels        = 1;
				d3d12ResourceDesc.SampleDesc.Count = 1;
				d3d12ResourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

				ID3D12Resource* d3d12Resource = nullptr;
				const D3D12_RESOURCE_STATES d3d12ResourceStates = (d3dHeapType == D3D12_HEAP_TYPE_READBACK) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
				[[maybe_unused]] HRESULT result = d3d12Device.CreateCommittedResource(&d3d12HeapProperties, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc, d3d12ResourceStates, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&d3d12Resource));
				ASSERT(SUCCEEDED(result), "Direct3D 12 create committed resource failed")

				return d3d12Resource;
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			ID3D12Device*			   mD3D12Device;
			ID3D12CommandAllocator*	   mD3D12CommandAllocator;
			ID3D12GraphicsCommandList* mD3D12GraphicsCommandList;
			ID3D12Resource*			   mD3D12ResourceUploadBuffer;
			D3D12_GPU_VIRTUAL_ADDRESS  mD3D12GpuVirtualAddress;
			uint8_t*				   mData;
			uint32_t				   mOffset;
			uint32_t				   mNumberOfUploadBufferBytes;


		};

		struct UploadContext final
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline UploadContext() :
				mCurrentFrameIndex(0),
				mCurrentUploadCommandListAllocator(nullptr),
				mCurrentD3d12GraphicsCommandList(nullptr)
			{}

			inline void create(ID3D12Device& d3d12Device)
			{
				for (uint32_t i = 0; i < NUMBER_OF_BUFFERED_FRAMES; ++i)
				{
					mUploadCommandListAllocator[i].create(d3d12Device);
				}
				begin();
			}

			inline void destroy()
			{
				for (uint32_t i = 0; i < NUMBER_OF_BUFFERED_FRAMES; ++i)
				{
					mUploadCommandListAllocator[i].destroy();
				}
			}

			inline UploadCommandListAllocator* getUploadCommandListAllocator() const
			{
				return mCurrentUploadCommandListAllocator;
			}

			inline ID3D12GraphicsCommandList* getD3d12GraphicsCommandList() const
			{
				return mCurrentD3d12GraphicsCommandList;
			}

			void begin()
			{
				// End previous upload command list allocator
				if (nullptr != mCurrentUploadCommandListAllocator)
				{
					mCurrentUploadCommandListAllocator->end();
					mCurrentFrameIndex = (mCurrentFrameIndex + 1) % NUMBER_OF_BUFFERED_FRAMES;
				}

				// Begin new upload command list allocator
				const uint32_t numberOfUploadBufferBytes = 1024 * 1024 * 1024;	// TODO(co) This must be a decent size with emergency reallocation if really necessary
				mCurrentUploadCommandListAllocator = &mUploadCommandListAllocator[mCurrentFrameIndex];
				mCurrentD3d12GraphicsCommandList = mCurrentUploadCommandListAllocator->getD3D12GraphicsCommandList();
				mCurrentUploadCommandListAllocator->begin(numberOfUploadBufferBytes);
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			UploadCommandListAllocator mUploadCommandListAllocator[NUMBER_OF_BUFFERED_FRAMES];
			// Current
			uint32_t							  mCurrentFrameIndex;
			::detail::UploadCommandListAllocator* mCurrentUploadCommandListAllocator;
			ID3D12GraphicsCommandList*			  mCurrentD3d12GraphicsCommandList;


		};

		/*
		*  @brief
		*    Descriptor heap
		*/
		class DescriptorHeap final
		{
		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline DescriptorHeap(Rhi::IAllocator& allocator, ID3D12Device& d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE d3d12DescriptorHeapType, uint16_t size, bool shaderVisible) :
				mD3D12DescriptorHeap(nullptr),
				mD3D12CpuDescriptorHandleForHeapStart{},
				mD3D12GpuDescriptorHandleForHeapStart{},
				mDescriptorSize(0),
				mMakeIDAllocator(allocator, size - 1u)
			{
				D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeadDescription = {};
				d3d12DescriptorHeadDescription.Type			  = d3d12DescriptorHeapType;
				d3d12DescriptorHeadDescription.NumDescriptors = size;
				d3d12DescriptorHeadDescription.Flags		  = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeadDescription, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&mD3D12DescriptorHeap));
				mD3D12CpuDescriptorHandleForHeapStart = mD3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				mD3D12GpuDescriptorHandleForHeapStart = mD3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
				mDescriptorSize = d3d12Device.GetDescriptorHandleIncrementSize(d3d12DescriptorHeapType);
			}

			inline ~DescriptorHeap()
			{
				mD3D12DescriptorHeap->Release();
			}

			inline uint16_t allocate(uint16_t count)
			{
				uint16_t index = 0;
				[[maybe_unused]] const bool result = mMakeIDAllocator.CreateRangeID(index, count);
				ASSERT(result, "Direct3D 12 create range ID failed")
				return index;
			}

			inline void release(uint16_t offset, uint16_t count)
			{
				[[maybe_unused]] const bool result = mMakeIDAllocator.DestroyRangeID(offset, count);
				ASSERT(result, "Direct3D 12 destroy range ID failed")
			}

			inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
			{
				return mD3D12DescriptorHeap;
			}

			inline D3D12_CPU_DESCRIPTOR_HANDLE getD3D12CpuDescriptorHandleForHeapStart() const
			{
				return mD3D12CpuDescriptorHandleForHeapStart;
			}

			inline D3D12_CPU_DESCRIPTOR_HANDLE getOffsetD3D12CpuDescriptorHandleForHeapStart(uint16_t offset) const
			{
				D3D12_CPU_DESCRIPTOR_HANDLE d3d12CpuDescriptorHandle = mD3D12CpuDescriptorHandleForHeapStart;
				d3d12CpuDescriptorHandle.ptr += offset * mDescriptorSize;
				return d3d12CpuDescriptorHandle;
			}

			inline D3D12_GPU_DESCRIPTOR_HANDLE getD3D12GpuDescriptorHandleForHeapStart() const
			{
				return mD3D12GpuDescriptorHandleForHeapStart;
			}

			inline uint32_t getDescriptorSize() const
			{
				return mDescriptorSize;
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit DescriptorHeap(const DescriptorHeap& source) = delete;
			DescriptorHeap& operator =(const DescriptorHeap& source) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			ID3D12DescriptorHeap*		mD3D12DescriptorHeap;
			D3D12_CPU_DESCRIPTOR_HANDLE mD3D12CpuDescriptorHandleForHeapStart;
			D3D12_GPU_DESCRIPTOR_HANDLE mD3D12GpuDescriptorHandleForHeapStart;
			uint32_t					mDescriptorSize;
			MakeID						mMakeIDAllocator;


		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Direct3D12Rhi
{




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Direct3D12Rhi.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 RHI class
	*/
	class Direct3D12Rhi final : public Rhi::IRhi
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t NUMBER_OF_FRAMES = 2;


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
		explicit Direct3D12Rhi(const Rhi::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Direct3D12Rhi() override;

		/**
		*  @brief
		*    Return the DXGI factory instance
		*
		*  @return
		*    The DXGI factory instance, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDXGIFactory4& getDxgiFactory4() const
		{
			RHI_ASSERT(mContext, nullptr != mDxgiFactory4, "Invalid Direct3D 12 DXGI factory 4")
			return *mDxgiFactory4;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 device
		*
		*  @return
		*    The Direct3D 12 device, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Device& getD3D12Device() const
		{
			RHI_ASSERT(mContext, nullptr != mD3D12Device, "Invalid Direct3D 12 device")
			return *mD3D12Device;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 command queue
		*
		*  @return
		*    The Direct3D 12 command queue, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12CommandQueue* getD3D12CommandQueue() const
		{
			return mD3D12CommandQueue;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 graphics command list
		*
		*  @return
		*    The Direct3D 12 graphics command list, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12GraphicsCommandList* getD3D12GraphicsCommandList() const
		{
			return mD3D12GraphicsCommandList;
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

		/**
		*  @brief
		*    Get the upload context
		*
		*  @return
		*    Upload context
		*/
		[[nodiscard]] inline ::detail::UploadContext& getUploadContext()
		{
			return mUploadContext;
		}

		//[-------------------------------------------------------]
		//[ Descriptor heaps                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline ::detail::DescriptorHeap& getShaderResourceViewDescriptorHeap() const
		{
			RHI_ASSERT(mContext, nullptr != mShaderResourceViewDescriptorHeap, "Invalid Direct3D 12 shader resource view descriptor heap")
			return *mShaderResourceViewDescriptorHeap;
		}

		[[nodiscard]] inline ::detail::DescriptorHeap& getRenderTargetViewDescriptorHeap() const
		{
			RHI_ASSERT(mContext, nullptr != mShaderResourceViewDescriptorHeap, "Invalid Direct3D 12 render target view descriptor heap")
			return *mRenderTargetViewDescriptorHeap;
		}

		[[nodiscard]] inline ::detail::DescriptorHeap& getDepthStencilViewDescriptorHeap() const
		{
			RHI_ASSERT(mContext, nullptr != mShaderResourceViewDescriptorHeap, "Invalid Direct3D 12 depth stencil target view descriptor heap")
			return *mDepthStencilViewDescriptorHeap;
		}

		[[nodiscard]] inline ::detail::DescriptorHeap& getSamplerDescriptorHeap() const
		{
			RHI_ASSERT(mContext, nullptr != mSamplerDescriptorHeap, "Invalid Direct3D 12 sampler descriptor heap")
			return *mSamplerDescriptorHeap;
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
		[[nodiscard]] inline virtual const char* getName() const override
		{
			return "Direct3D12";
		}

		[[nodiscard]] inline virtual bool isInitialized() const override
		{
			// Is there a Direct3D 12 command queue?
			return (nullptr != mD3D12CommandQueue);
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
			RHI_DELETE(mContext, Direct3D12Rhi, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D12Rhi(const Direct3D12Rhi& source) = delete;
		Direct3D12Rhi& operator =(const Direct3D12Rhi& source) = delete;

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

		#ifdef RHI_DEBUG
			/**
			*  @brief
			*    Reports information about a device object's lifetime for debugging
			*/
			void debugReportLiveDeviceObjects();
		#endif


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D12RuntimeLinking*  mDirect3D12RuntimeLinking;	///< Direct3D 12 runtime linking instance, always valid
		IDXGIFactory4*			   mDxgiFactory4;				///< DXGI factors instance, always valid for a correctly initialized RHI
		ID3D12Device*			   mD3D12Device;				///< The Direct3D 12 device, null pointer on error (we don't check because this would be a total overhead, the user has to use "Rhi::IRhi::isInitialized()" and is asked to never ever use a not properly initialized RHI)
		ID3D12CommandQueue*		   mD3D12CommandQueue;			///< The Direct3D 12 command queue, null pointer on error (we don't check because this would be a total overhead, the user has to use "Rhi::IRhi::isInitialized()" and is asked to never ever use a not properly initialized RHI)
		ID3D12CommandAllocator*	   mD3D12CommandAllocator;
		ID3D12GraphicsCommandList* mD3D12GraphicsCommandList;
		Rhi::IShaderLanguage*	   mShaderLanguageHlsl;			///< HLSL shader language instance (we keep a reference to it), can be a null pointer
		::detail::UploadContext	   mUploadContext;
		::detail::DescriptorHeap*  mShaderResourceViewDescriptorHeap;
		::detail::DescriptorHeap*  mRenderTargetViewDescriptorHeap;
		::detail::DescriptorHeap*  mDepthStencilViewDescriptorHeap;
		::detail::DescriptorHeap*  mSamplerDescriptorHeap;
		//[-------------------------------------------------------]
		//[ State related                                         ]
		//[-------------------------------------------------------]
		Rhi::IRenderTarget*		 mRenderTarget;				///< Output-merger (OM) stage: Currently set render target (we keep a reference to it), can be a null pointer
		D3D12_PRIMITIVE_TOPOLOGY mD3D12PrimitiveTopology;	///< State cache to avoid making redundant Direct3D 12 calls
		RootSignature*			 mGraphicsRootSignature;	///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*			 mComputeRootSignature;		///< Currently set compute root signature (we keep a reference to it), can be a null pointer
		VertexArray*			 mVertexArray;				///< Currently set vertex array (we keep a reference to it), can be a null pointer
		#ifdef RHI_DEBUG
			bool mDebugBetweenBeginEndScene;	///< Just here for state tracking in debug builds
		#endif


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Direct3D12RuntimeLinking.h              ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Macros & definitions                                  ]
	//[-------------------------------------------------------]
	// Redirect D3D12* function calls to funcPtr_D3D12*
	#ifndef FNPTR
		#define FNPTR(name) funcPtr_##name
	#endif

	//[-------------------------------------------------------]
	//[ DXGI core functions                                   ]
	//[-------------------------------------------------------]
	#define FNDEF_DXGI(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_DXGI(HRESULT,	CreateDXGIFactory1,	(REFIID riid, _COM_Outptr_ void **ppFactory));
	#define CreateDXGIFactory1	FNPTR(CreateDXGIFactory1)

	//[-------------------------------------------------------]
	//[ D3D12 core functions                                  ]
	//[-------------------------------------------------------]
	#define FNDEF_D3D12(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_D3D12(HRESULT,	D3D12CreateDevice,				(_In_opt_ IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, _In_ REFIID riid, _COM_Outptr_opt_ void** ppDevice));
	FNDEF_D3D12(HRESULT,	D3D12SerializeRootSignature,	(_In_ const D3D12_ROOT_SIGNATURE_DESC* pRootSignature, _In_ D3D_ROOT_SIGNATURE_VERSION Version, _Out_ ID3DBlob** ppBlob, _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob));
	#define D3D12CreateDevice			FNPTR(D3D12CreateDevice)
	#define D3D12SerializeRootSignature	FNPTR(D3D12SerializeRootSignature)
	#ifdef RHI_DEBUG
		FNDEF_D3D12(HRESULT,	D3D12GetDebugInterface,	(_In_ REFIID riid, _COM_Outptr_opt_ void** ppvDebug));
		#define D3D12GetDebugInterface	FNPTR(D3D12GetDebugInterface)
	#endif

	//[-------------------------------------------------------]
	//[ D3DCompiler functions                                 ]
	//[-------------------------------------------------------]
	#define FNDEF_D3DX12(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	typedef __interface ID3D10Blob *LPD3D10BLOB;	// "__interface" is no keyword of the ISO C++ standard, shouldn't be a problem because this in here is Microsoft Windows only and it's also within the Direct3D headers we have to use
	typedef ID3D10Blob ID3DBlob;
	FNDEF_D3DX12(HRESULT,	D3DCompile,		(LPCVOID, SIZE_T, LPCSTR, CONST D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**));
	FNDEF_D3DX12(HRESULT,	D3DCreateBlob,	(SIZE_T Size, ID3DBlob** ppBlob));
	#define D3DCompile		FNPTR(D3DCompile)
	#define D3DCreateBlob	FNPTR(D3DCreateBlob)

	/**
	*  @brief
	*    Direct3D 12 runtime linking
	*/
	class Direct3D12RuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*/
		inline explicit Direct3D12RuntimeLinking(Direct3D12Rhi& direct3D12Rhi) :
			mDirect3D12Rhi(direct3D12Rhi),
			mDxgiSharedLibrary(nullptr),
			mD3D12SharedLibrary(nullptr),
			mD3DCompilerSharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		~Direct3D12RuntimeLinking()
		{
			// Destroy the shared library instances
			if (nullptr != mDxgiSharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mDxgiSharedLibrary));
			}
			if (nullptr != mD3D12SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3D12SharedLibrary));
			}
			if (nullptr != mD3DCompilerSharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3DCompilerSharedLibrary));
			}
		}

		/**
		*  @brief
		*    Return whether or not Direct3D 12 is available
		*
		*  @return
		*    "true" if Direct3D 12 is available, else "false"
		*/
		[[nodiscard]] bool isDirect3D12Avaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the DXGI, D3D12 and D3DCompiler entry points
					mEntryPointsRegistered = (loadDxgiEntryPoints() && loadD3D12EntryPoints() && loadD3DCompilerEntryPoints());
				}
			}

			// Entry points successfully registered?
			return mEntryPointsRegistered;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D12RuntimeLinking(const Direct3D12RuntimeLinking& source) = delete;
		Direct3D12RuntimeLinking& operator =(const Direct3D12RuntimeLinking& source) = delete;

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
				mD3D12SharedLibrary = ::LoadLibraryExA("d3d12.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr != mD3D12SharedLibrary)
				{
					mD3DCompilerSharedLibrary = ::LoadLibraryExA("D3DCompiler_47.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr == mD3DCompilerSharedLibrary)
					{
						RHI_LOG(mDirect3D12Rhi.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"D3DCompiler_47.dll\"")
					}
				}
				else
				{
					RHI_LOG(mDirect3D12Rhi.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"d3d12.dll\"")
				}
			}
			else
			{
				RHI_LOG(mDirect3D12Rhi.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"dxgi.dll\"")
			}

			// Done
			return (nullptr != mDxgiSharedLibrary && nullptr != mD3D12SharedLibrary && nullptr != mD3DCompilerSharedLibrary);
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
						RHI_LOG(mDirect3D12Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 DXGI shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																							\
					}																																											\
				}

			// Load the entry points
			IMPORT_FUNC(CreateDXGIFactory1)

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the D3D12 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3D12EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																					\
				if (result)																																									\
				{																																											\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3D12SharedLibrary), #funcName);																					\
					if (nullptr != symbol)																																					\
					{																																										\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																													\
					}																																										\
					else																																									\
					{																																										\
						wchar_t moduleFilename[MAX_PATH];																																	\
						moduleFilename[0] = '\0';																																			\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3D12SharedLibrary), moduleFilename, MAX_PATH);																			\
						RHI_LOG(mDirect3D12Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																						\
					}																																										\
				}

			// Load the entry points
			IMPORT_FUNC(D3D12CreateDevice)
			IMPORT_FUNC(D3D12SerializeRootSignature)
			#ifdef RHI_DEBUG
				IMPORT_FUNC(D3D12GetDebugInterface)
			#endif

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
						RHI_LOG(mDirect3D12Rhi.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 shared library \"%s\"", #funcName, moduleFilename)	\
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


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D12Rhi&	mDirect3D12Rhi;				///< Owner Direct3D 12 RHI instance
		void*			mDxgiSharedLibrary;			///< DXGI shared library, can be a null pointer
		void*			mD3D12SharedLibrary;		///< D3D12 shared library, can be a null pointer
		void*			mD3DCompilerSharedLibrary;	///< D3DCompiler shared library, can be a null pointer
		bool			mEntryPointsRegistered;		///< Entry points successfully registered?
		bool			mInitialized;				///< Already initialized?


	};




	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	// In order to assign debug names to Direct3D resources we need to use the "WKPDID_D3DDebugObjectName"-GUID. This GUID
	// is defined within the "D3Dcommon.h" header and it's required to add the library "dxguid.lib" in which the symbol
	// is defined.
	// -> See "ID3D12Device::SetPrivateData method"-documentation at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/ff476533%28v=vs.85%29.aspx
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
	*    ASCII shader model (for example "vs_5_0", "gs_5_0", "ps_5_0"), must be a valid pointer
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
		RHI_ASSERT(context, nullptr != shaderModel, "Invalid Direct3D 12 shader model")
		RHI_ASSERT(context, nullptr != sourceCode, "Invalid Direct3D 12 shader source code")

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

	void handleDeviceLost(const Direct3D12Rhi& direct3D12Rhi, HRESULT result)
	{
		// If the device was removed either by a disconnection or a driver upgrade, we must recreate all device resources
		if (DXGI_ERROR_DEVICE_REMOVED == result || DXGI_ERROR_DEVICE_RESET == result)
		{
			if (DXGI_ERROR_DEVICE_REMOVED == result)
			{
				result = direct3D12Rhi.getD3D12Device().GetDeviceRemovedReason();
			}
			RHI_LOG(direct3D12Rhi.getContext(), CRITICAL, "Direct3D 12 device lost on present: Reason code 0x%08X", static_cast<unsigned int>(result))

			// TODO(co) Add device lost handling if needed. Probably more complex to recreate all device resources.
		}
	}




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Mapping.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 mapping
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
		*    "Rhi::VertexAttributeFormat" to Direct3D 12 format
		*
		*  @param[in] vertexAttributeFormat
		*    "Rhi::VertexAttributeFormat" to map
		*
		*  @return
		*    Direct3D 12 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D12Format(Rhi::VertexAttributeFormat vertexAttributeFormat)
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
		*    "Rhi::BufferUsage" to Direct3D 12 usage and CPU access flags
		*
		*  @param[in]  bufferUsage
		*    "Rhi::BufferUsage" to map
		*  @param[out] cpuAccessFlags
		*    Receives the CPU access flags
		*
		*  @return
		*    Direct3D 12 usage // TODO(co) Use correct Direct3D 12 type
		*/
		[[nodiscard]] static uint32_t getDirect3D12UsageAndCPUAccessFlags([[maybe_unused]] Rhi::BufferUsage bufferUsage, [[maybe_unused]] uint32_t& cpuAccessFlags)
		{
			// TODO(co) Direct3D 12
			/*
			// Direct3D 12 only supports a subset of the OpenGL usage indications
			// -> See "D3D12_USAGE enumeration "-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/ff476259%28v=vs.85%29.aspx
			switch (bufferUsage)
			{
				case Rhi::BufferUsage::STREAM_DRAW:
				case Rhi::BufferUsage::STREAM_COPY:
				case Rhi::BufferUsage::STATIC_DRAW:
				case Rhi::BufferUsage::STATIC_COPY:
					cpuAccessFlags = 0;
					return D3D12_USAGE_IMMUTABLE;

				case Rhi::BufferUsage::STREAM_READ:
				case Rhi::BufferUsage::STATIC_READ:
					cpuAccessFlags = D3D12_CPU_ACCESS_READ;
					return D3D12_USAGE_STAGING;

				case Rhi::BufferUsage::DYNAMIC_DRAW:
				case Rhi::BufferUsage::DYNAMIC_COPY:
					cpuAccessFlags = D3D12_CPU_ACCESS_WRITE;
					return D3D12_USAGE_DYNAMIC;

				default:
				case Rhi::BufferUsage::DYNAMIC_READ:
					cpuAccessFlags = 0;
					return D3D12_USAGE_DEFAULT;
			}
			*/
			return 0;
		}

		//[-------------------------------------------------------]
		//[ Rhi::IndexBufferFormat                                ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Rhi::IndexBufferFormat" to Direct3D 12 format
		*
		*  @param[in] indexBufferFormat
		*    "Rhi::IndexBufferFormat" to map
		*
		*  @return
		*    Direct3D 12 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D12Format(Rhi::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R32_UINT,	// Rhi::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Direct3D 12
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
		*    "Rhi::TextureFormat" to Direct3D 12 format
		*
		*  @param[in] textureFormat
		*    "Rhi::TextureFormat" to map
		*
		*  @return
		*    Direct3D 12 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D12Format(Rhi::TextureFormat::Enum textureFormat)
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
				DXGI_FORMAT_UNKNOWN,				// Rhi::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 12
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


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/TextureHelper.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 texture helper
	*/
	class TextureHelper final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		enum class TextureType
		{
			TEXTURE_1D,
			TEXTURE_1D_ARRAY,
			TEXTURE_2D,
			TEXTURE_2D_ARRAY,
			TEXTURE_CUBE,
			TEXTURE_CUBE_ARRAY,
			TEXTURE_3D
		};


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] static ID3D12Resource* CreateTexture(ID3D12Device& d3d12Device, TextureType textureType, uint32_t width, uint32_t height, uint32_t depth, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, uint8_t numberOfMultisamples, uint32_t numberOfMipmaps, uint32_t textureFlags, const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue)
		{
			D3D12_HEAP_PROPERTIES d3d12HeapProperties = {};
			d3d12HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

			// Get Direct3D 12 resource description
			D3D12_RESOURCE_DESC d3d12ResourceDesc = {};
			d3d12ResourceDesc.Dimension			= (textureType <= TextureType::TEXTURE_1D_ARRAY) ? D3D12_RESOURCE_DIMENSION_TEXTURE1D : ((textureType <= TextureType::TEXTURE_CUBE_ARRAY) ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE3D);
			d3d12ResourceDesc.Width				= width;
			d3d12ResourceDesc.Height			= height;
			d3d12ResourceDesc.DepthOrArraySize	= static_cast<uint16_t>((TextureType::TEXTURE_3D == textureType) ? depth : numberOfSlices);
			d3d12ResourceDesc.MipLevels			= static_cast<uint16_t>(numberOfMipmaps);
			d3d12ResourceDesc.Format			= Mapping::getDirect3D12Format(textureFormat);
			d3d12ResourceDesc.SampleDesc.Count	= numberOfMultisamples;
			d3d12ResourceDesc.Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN;

			{ // Get Direct3D 12 resource description flags
				uint32_t descriptionFlags = 0;
				if (textureFlags & Rhi::TextureFlag::RENDER_TARGET)
				{
					descriptionFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
					if (Rhi::TextureFormat::isDepth(textureFormat))
					{
						descriptionFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
						if ((textureFlags & Rhi::TextureFlag::SHADER_RESOURCE) == 0)
						{
							descriptionFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
						}
					}
				}
				if (textureFlags & Rhi::TextureFlag::UNORDERED_ACCESS)
				{
					descriptionFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				}
				d3d12ResourceDesc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(descriptionFlags);
			}

			// Get Direct3D 12 resource states and clear value
			D3D12_RESOURCE_STATES d3d12ResourceStates = D3D12_RESOURCE_STATE_COPY_DEST;
			D3D12_CLEAR_VALUE d3d12ClearValue = {};
			if (textureFlags & Rhi::TextureFlag::RENDER_TARGET)
			{
				if (Rhi::TextureFormat::isDepth(textureFormat))
				{
					d3d12ResourceStates = D3D12_RESOURCE_STATE_DEPTH_WRITE;
					d3d12ClearValue.Format = d3d12ResourceDesc.Format;
					if (nullptr != optimizedTextureClearValue)
					{
						d3d12ClearValue.DepthStencil.Depth = optimizedTextureClearValue->DepthStencil.depth;
					}
				}
				else
				{
					d3d12ResourceStates = D3D12_RESOURCE_STATE_RENDER_TARGET;
					if (nullptr != optimizedTextureClearValue)
					{
						d3d12ClearValue.Format = d3d12ResourceDesc.Format;
						memcpy(d3d12ClearValue.Color, optimizedTextureClearValue->color, sizeof(float) * 4);
					}
				}
			}

			// Create the Direct3D 12 texture resource
			ID3D12Resource* d3d12Texture = nullptr;
			const HRESULT result = d3d12Device.CreateCommittedResource(&d3d12HeapProperties, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc, d3d12ResourceStates, d3d12ClearValue.Format ? &d3d12ClearValue : nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&d3d12Texture));
			return (SUCCEEDED(result) ? d3d12Texture : nullptr);
		}

		static void SetTextureData(::detail::UploadContext& uploadContext, ID3D12Resource& d3d12Resource, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, uint32_t numberOfMipmaps, uint32_t mip, uint32_t slice, const void* data, [[maybe_unused]] uint32_t size, uint32_t pitch)
		{
			// TODO(co) This should never ever happen
			if (nullptr == uploadContext.getUploadCommandListAllocator())
			{
				return;
			}

			const D3D12_RESOURCE_DESC d3d12ResourceDesc = d3d12Resource.GetDesc();

			// Texture copy destination
			D3D12_TEXTURE_COPY_LOCATION d3d12TextureCopyLocationDestination;
			d3d12TextureCopyLocationDestination.pResource = &d3d12Resource;
			d3d12TextureCopyLocationDestination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

			// Texture copy source
			D3D12_TEXTURE_COPY_LOCATION d3d12TextureCopyLocationSource;
			d3d12TextureCopyLocationSource.pResource = uploadContext.getUploadCommandListAllocator()->getD3D12ResourceUploadBuffer();
			d3d12TextureCopyLocationSource.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.Format = d3d12ResourceDesc.Format;

			// Get the number of rows
			uint32_t numberOfColumns = width;
			uint32_t numberOfRows = height;
			const bool isCompressed = Rhi::TextureFormat::isCompressed(textureFormat);
			if (isCompressed)
			{
				numberOfColumns = (numberOfColumns + 3) >> 2;
				numberOfRows = (numberOfRows + 3) >> 2;
			}
			numberOfRows *= depth;
			ASSERT(pitch * numberOfRows == size, "Direct3D 12: Invalid size")

			// Grab upload buffer space
			static constexpr uint32_t D3D12_TEXTURE_DATA_PITCH_ALIGNMENT	 = 256;	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
			static constexpr uint32_t D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT = 512;	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
			const uint32_t destinationPitch = ::detail::align(pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			const uint32_t destinationOffset = uploadContext.getUploadCommandListAllocator()->allocateUploadBuffer(destinationPitch * numberOfRows, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

			// Copy data in place
			const uint8_t* sourceData = reinterpret_cast<const uint8_t*>(data);
			uint8_t* destinationData = uploadContext.getUploadCommandListAllocator()->getData() + destinationOffset;
			const uint32_t sourcePitch = pitch;
			for (uint32_t r = 0; r < numberOfRows; ++r)
			{
				memcpy(destinationData, sourceData, sourcePitch);
				destinationData += destinationPitch;
				sourceData += sourcePitch;
			}

			// Issue a copy from upload buffer to texture
			d3d12TextureCopyLocationDestination.SubresourceIndex = mip + slice * numberOfMipmaps;
			d3d12TextureCopyLocationSource.PlacedFootprint.Offset = destinationOffset;
			if (isCompressed)
			{
				d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.Width  = ::detail::align(width, 4);
				d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.Height = ::detail::align(height, 4);
			}
			else
			{
				d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.Width  = width;
				d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.Height = height;
			}
			d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.Depth  = depth;
			d3d12TextureCopyLocationSource.PlacedFootprint.Footprint.RowPitch = destinationPitch;
			uploadContext.getD3d12GraphicsCommandList()->CopyTextureRegion(&d3d12TextureCopyLocationDestination, 0, 0, 0, &d3d12TextureCopyLocationSource, nullptr);

			D3D12_RESOURCE_BARRIER d3d12ResourceBarrier;
			d3d12ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			d3d12ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3d12ResourceBarrier.Transition.pResource   = &d3d12Resource;
			d3d12ResourceBarrier.Transition.Subresource = d3d12TextureCopyLocationDestination.SubresourceIndex;
			d3d12ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			d3d12ResourceBarrier.Transition.StateAfter  = static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			uploadContext.getD3d12GraphicsCommandList()->ResourceBarrier(1, &d3d12ResourceBarrier);
		}


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/RootSignature.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(Direct3D12Rhi& direct3D12Rhi, const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IRootSignature(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature),
			mD3D12RootSignature(nullptr)
		{
			const Rhi::Context& context = direct3D12Rhi.getContext();

			{ // We need a backup of the given root signature
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

			// Create temporary Direct3D 12 root signature instance data
			// -> "Rhi::RootSignature" is not identical to "D3D12_ROOT_SIGNATURE_DESC" because it had to be extended by information required by OpenGL
			D3D12_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc;
			{
				{ // Copy the parameter data
					const uint32_t numberOfRootParameters = rootSignature.numberOfParameters;
					d3d12RootSignatureDesc.NumParameters = numberOfRootParameters;
					if (numberOfRootParameters > 0)
					{
						d3d12RootSignatureDesc.pParameters = RHI_MALLOC_TYPED(context, D3D12_ROOT_PARAMETER, numberOfRootParameters);
						D3D12_ROOT_PARAMETER* d3dRootParameters = const_cast<D3D12_ROOT_PARAMETER*>(d3d12RootSignatureDesc.pParameters);
						for (uint32_t parameterIndex = 0; parameterIndex < numberOfRootParameters; ++parameterIndex)
						{
							D3D12_ROOT_PARAMETER& d3dRootParameter = d3dRootParameters[parameterIndex];
							const Rhi::RootParameter& rootParameter = rootSignature.parameters[parameterIndex];

							// Copy the descriptor table data and determine the shader visibility of the Direct3D 12 root parameter
							uint32_t shaderVisibility = ~0u;
							if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
							{
								const uint32_t numberOfDescriptorRanges = rootParameter.descriptorTable.numberOfDescriptorRanges;
								d3dRootParameter.DescriptorTable.NumDescriptorRanges = numberOfDescriptorRanges;
								d3dRootParameter.DescriptorTable.pDescriptorRanges = RHI_MALLOC_TYPED(context, D3D12_DESCRIPTOR_RANGE, numberOfDescriptorRanges);

								// "Rhi::DescriptorRange" is not identical to "D3D12_DESCRIPTOR_RANGE" because it had to be extended by information required by OpenGL
								for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
								{
									const Rhi::DescriptorRange& descriptorRange = reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];
									memcpy(const_cast<D3D12_DESCRIPTOR_RANGE*>(&d3dRootParameter.DescriptorTable.pDescriptorRanges[descriptorRangeIndex]), &descriptorRange, sizeof(D3D12_DESCRIPTOR_RANGE));
									if (~0u == shaderVisibility)
									{
										shaderVisibility = static_cast<uint32_t>(descriptorRange.shaderVisibility);
										if (shaderVisibility == static_cast<uint32_t>(Rhi::ShaderVisibility::COMPUTE) || shaderVisibility == static_cast<uint32_t>(Rhi::ShaderVisibility::ALL_GRAPHICS))
										{
											shaderVisibility = static_cast<uint32_t>(Rhi::ShaderVisibility::ALL);
										}
									}
									else if (shaderVisibility != static_cast<uint32_t>(descriptorRange.shaderVisibility))
									{
										shaderVisibility = static_cast<uint32_t>(Rhi::ShaderVisibility::ALL);
									}
								}
							}
							if (~0u == shaderVisibility)
							{
								shaderVisibility = static_cast<uint32_t>(Rhi::ShaderVisibility::ALL);
							}

							// Set root parameter
							d3dRootParameter.ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(rootParameter.parameterType);
							d3dRootParameter.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shaderVisibility);
						}
					}
					else
					{
						d3d12RootSignatureDesc.pParameters = nullptr;
					}
				}

				{ // Copy the static sampler data
				  // -> "Rhi::StaticSampler" is identical to "D3D12_STATIC_SAMPLER_DESC" so there's no additional mapping work to be done in here
					const uint32_t numberOfStaticSamplers = rootSignature.numberOfStaticSamplers;
					d3d12RootSignatureDesc.NumStaticSamplers = numberOfStaticSamplers;
					if (numberOfStaticSamplers > 0)
					{
						d3d12RootSignatureDesc.pStaticSamplers = RHI_MALLOC_TYPED(context, D3D12_STATIC_SAMPLER_DESC, numberOfStaticSamplers);
						memcpy(const_cast<D3D12_STATIC_SAMPLER_DESC*>(d3d12RootSignatureDesc.pStaticSamplers), rootSignature.staticSamplers, sizeof(Rhi::StaticSampler) * numberOfStaticSamplers);
					}
					else
					{
						d3d12RootSignatureDesc.pStaticSamplers = nullptr;
					}
				}

				// Copy flags
				// -> "Rhi::RootSignatureFlags" is identical to "D3D12_ROOT_SIGNATURE_FLAGS" so there's no additional mapping work to be done in here
				d3d12RootSignatureDesc.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(rootSignature.flags);
			}

			{ // Create the Direct3D 12 root signature instance
				ID3DBlob* d3dBlobSignature = nullptr;
				ID3DBlob* d3dBlobError = nullptr;
				if (SUCCEEDED(D3D12SerializeRootSignature(&d3d12RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &d3dBlobSignature, &d3dBlobError)))
				{
					if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateRootSignature(0, d3dBlobSignature->GetBufferPointer(), d3dBlobSignature->GetBufferSize(), IID_PPV_ARGS(&mD3D12RootSignature))))
					{
						// Assign a default name to the resource for debugging purposes
						#ifdef RHI_DEBUG
							RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Root signature", 17)	// 17 = "Root signature: " including terminating zero
							FAILED_DEBUG_BREAK(mD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
						#endif
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create the Direct3D 12 root signature instance")
					}
					d3dBlobSignature->Release();
				}
				else
				{
					RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create the Direct3D 12 root signature instance: %s", (nullptr != d3dBlobError) ? reinterpret_cast<char*>(d3dBlobError->GetBufferPointer()) : "Unknown error")
					if (nullptr != d3dBlobError)
					{
						d3dBlobError->Release();
					}
				}
			}

			// Free temporary Direct3D 12 root signature instance data
			if (nullptr != d3d12RootSignatureDesc.pParameters)
			{
				for (uint32_t i = 0; i < d3d12RootSignatureDesc.NumParameters; ++i)
				{
					const D3D12_ROOT_PARAMETER& d3d12RootParameter = d3d12RootSignatureDesc.pParameters[i];
					if (D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE == d3d12RootParameter.ParameterType)
					{
						RHI_FREE(context, const_cast<D3D12_DESCRIPTOR_RANGE*>(d3d12RootParameter.DescriptorTable.pDescriptorRanges));
					}
				}
				RHI_FREE(context, const_cast<D3D12_ROOT_PARAMETER*>(d3d12RootSignatureDesc.pParameters));
			}
			RHI_FREE(context, const_cast<D3D12_STATIC_SAMPLER_DESC*>(d3d12RootSignatureDesc.pStaticSamplers));
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RootSignature() override
		{
			// Release the Direct3D 12 root signature
			if (nullptr != mD3D12RootSignature)
			{
				mD3D12RootSignature->Release();
			}

			// Destroy the backup of the given root signature
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

		/**
		*  @brief
		*    Return the Direct3D 12 root signature
		*
		*  @return
		*    The Direct3D 12 root signature, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12RootSignature* getD3D12RootSignature() const
		{
			return mD3D12RootSignature;
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
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::RootSignature	 mRootSignature;
		ID3D12RootSignature* mD3D12RootSignature;	///< Direct3D 12 root signature, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/VertexBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(Direct3D12Rhi& direct3D12Rhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IVertexBuffer(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfBytes(numberOfBytes),
			mD3D12Resource(nullptr)
		{
			// TODO(co) This is only meant for the Direct3D 12 RHI implementation kickoff.
			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.

			// TODO(co) Add buffer usage setting support

			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mNumberOfBytes);
			if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12XResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			{
				// Data given?
				if (nullptr != data)
				{
					// Copy the data to the vertex buffer
					UINT8* pVertexDataBegin;
					CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
					if (SUCCEEDED(mD3D12Resource->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin))))
					{
						memcpy(pVertexDataBegin, data, mNumberOfBytes);
						mD3D12Resource->Unmap(0, nullptr);
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to map Direct3D 12 vertex buffer")
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "VBO", 6)	// 6 = "VBO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 vertex buffer resource")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexBuffer() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the number of bytes within the vertex buffer
		*
		*  @return
		*    The number of bytes within the vertex buffer
		*/
		[[nodiscard]] inline uint32_t getNumberOfBytes() const
		{
			return mNumberOfBytes;
		}

		/**
		*  @brief
		*    Return the Direct3D vertex buffer resource instance
		*
		*  @return
		*    The Direct3D vertex buffer resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		uint32_t		mNumberOfBytes;	///< Number of bytes within the vertex buffer
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/IndexBuffer.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 index buffer object (IBO, "element array buffer" in OpenGL terminology) class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(Direct3D12Rhi& direct3D12Rhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage, Rhi::IndexBufferFormat::Enum indexBufferFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndexBuffer(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D12Resource(nullptr)
		{
			// Sanity check
			// TODO(co) Check this, there's "DXGI_FORMAT_R8_UINT" which might work in Direct3D 12
			RHI_ASSERT(direct3D12Rhi.getContext(), Rhi::IndexBufferFormat::UNSIGNED_CHAR != indexBufferFormat, "\"Rhi::IndexBufferFormat::UNSIGNED_CHAR\" is not supported by Direct3D 12")

			// TODO(co) This is only meant for the Direct3D 12 RHI implementation kickoff.
			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.

			// TODO(co) Add buffer usage setting support

			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfBytes);
			if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12XResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			{
				// Data given?
				if (nullptr != data)
				{
					// Copy the data to the index buffer
					UINT8* pIndexDataBegin;
					CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
					if (SUCCEEDED(mD3D12Resource->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin))))
					{
						memcpy(pIndexDataBegin, data, numberOfBytes);
						mD3D12Resource->Unmap(0, nullptr);
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to map Direct3D 12 index buffer")
					}
				}

				// Fill the Direct3D 12 index buffer view
				mD3D12IndexBufferView.BufferLocation = mD3D12Resource->GetGPUVirtualAddress();
				mD3D12IndexBufferView.SizeInBytes	 = numberOfBytes;
				mD3D12IndexBufferView.Format		 = Mapping::getDirect3D12Format(indexBufferFormat);

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IBO", 6)	// 6 = "IBO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 index buffer resource")
				mD3D12IndexBufferView.BufferLocation = 0;
				mD3D12IndexBufferView.SizeInBytes	 = 0;
				mD3D12IndexBufferView.Format		 = DXGI_FORMAT_UNKNOWN;
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~IndexBuffer() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D index buffer resource instance
		*
		*  @return
		*    The Direct3D index buffer resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 index buffer view
		*
		*  @return
		*    The Direct3D 12 index buffer view
		*/
		[[nodiscard]] inline const D3D12_INDEX_BUFFER_VIEW& getD3D12IndexBufferView() const
		{
			return mD3D12IndexBufferView;
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
		ID3D12Resource*			mD3D12Resource;
		D3D12_INDEX_BUFFER_VIEW	mD3D12IndexBufferView;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/VertexArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 vertex array class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
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
		VertexArray(Direct3D12Rhi& direct3D12Rhi, const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexArray(direct3D12Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mIndexBuffer(indexBuffer),
			mNumberOfSlots(numberOfVertexBuffers),
			mD3D12VertexBufferViews(nullptr),
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
				const Rhi::Context& context = direct3D12Rhi.getContext();
				mD3D12VertexBufferViews = RHI_MALLOC_TYPED(context, D3D12_VERTEX_BUFFER_VIEW, mNumberOfSlots);
				mVertexBuffers = RHI_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					D3D12_VERTEX_BUFFER_VIEW* currentD3D12VertexBufferView = mD3D12VertexBufferViews;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentD3D12VertexBufferView, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used RHI?
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						(*currentVertexBuffer)->addReference();
						currentD3D12VertexBufferView->BufferLocation = (*currentVertexBuffer)->getD3D12Resource()->GetGPUVirtualAddress();
						currentD3D12VertexBufferView->SizeInBytes = (*currentVertexBuffer)->getNumberOfBytes();
					}
				}

				{ // Gather slot related data
					const Rhi::VertexAttribute* attribute = vertexAttributes.attributes;
					const Rhi::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
					for (; attribute < attributesEnd; ++attribute)
					{
						mD3D12VertexBufferViews[attribute->inputSlot].StrideInBytes = attribute->strideInBytes;
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

			// Cleanup Direct3D 12 input slot data, if needed
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			const Rhi::Context& context = direct3D12Rhi.getContext();
			RHI_FREE(context, mD3D12VertexBufferViews);

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
			direct3D12Rhi.VertexArrayMakeId.DestroyID(getId());
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
		*    Set the Direct3D 12 vertex declaration and stream source
		*
		*  @param[in] d3d12GraphicsCommandList
		*    Direct3D 12 graphics command list to feed
		*/
		void setDirect3DIASetInputLayoutAndStreamSource(ID3D12GraphicsCommandList& d3d12GraphicsCommandList) const
		{
			d3d12GraphicsCommandList.IASetVertexBuffers(0, mNumberOfSlots, mD3D12VertexBufferViews);

			// Set the used index buffer
			// -> In case of no index buffer we don't set null indices, there's not really a point in it
			if (nullptr != mIndexBuffer)
			{
				// Set the Direct3D 12 indices
				d3d12GraphicsCommandList.IASetIndexBuffer(&static_cast<IndexBuffer*>(mIndexBuffer)->getD3D12IndexBufferView());
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
		IndexBuffer*			  mIndexBuffer;				///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Direct3D 12 input slots
		UINT					  mNumberOfSlots;			///< Number of used Direct3D 12 input slots
		D3D12_VERTEX_BUFFER_VIEW* mD3D12VertexBufferViews;
		// For proper vertex buffer reference counter behaviour
		VertexBuffer**			  mVertexBuffers;			///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/TextureBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 texture buffer object (TBO) class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBuffer(Direct3D12Rhi& direct3D12Rhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage, Rhi::TextureFormat::Enum textureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureBuffer(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfBytes(numberOfBytes),
			mTextureFormat(textureFormat),
			mD3D12Resource(nullptr)
		{
			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), (numberOfBytes % Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The Direct3D 12 texture buffer size must be a multiple of the selected texture format bytes per texel")

			// TODO(co) This is only meant for the Direct3D 12 RHI implementation kickoff.
			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.
			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfBytes);
			ID3D12Device& d3d12Device = direct3D12Rhi.getD3D12Device();
			if (SUCCEEDED(d3d12Device.CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12XResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			{
				// Data given?
				if (nullptr != data)
				{
					// Copy the data to the texture buffer
					UINT8* pTextureDataBegin;
					CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
					if (SUCCEEDED(mD3D12Resource->Map(0, &readRange, reinterpret_cast<void**>(&pTextureDataBegin))))
					{
						memcpy(pTextureDataBegin, data, numberOfBytes);
						mD3D12Resource->Unmap(0, nullptr);
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to map Direct3D 12 texture buffer")
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "TBO", 6)	// 6 = "TBO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 texture buffer resource")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureBuffer() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the number of bytes
		*
		*  @return
		*    The number of bytes
		*/
		[[nodiscard]] inline uint32_t getNumberOfBytes() const
		{
			return mNumberOfBytes;
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
		*    Return the Direct3D 12 texture buffer resource instance
		*
		*  @return
		*    The Direct3D 12 texture buffer resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		uint32_t				 mNumberOfBytes;
		Rhi::TextureFormat::Enum mTextureFormat;
		ID3D12Resource*			 mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/StructuredBuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 structured buffer object (SBO) class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBuffer(Direct3D12Rhi& direct3D12Rhi, [[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IStructuredBuffer(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
			// TODO(co) Direct3D 12 update
		//	mD3D12Buffer(nullptr),
		//	mD3D12ShaderResourceViewTexture(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The Direct3D 12 structured buffer size must be a multiple of the given number of structure bytes")
			RHI_ASSERT(direct3D12Rhi.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The Direct3D 12 structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// TODO(co) Direct3D 12 update
			/*
			{ // Buffer part
				// Direct3D 12 buffer description
				D3D12_BUFFER_DESC d3d12BufferDesc;
				d3d12BufferDesc.ByteWidth           = numberOfBytes;
				d3d12BufferDesc.Usage               = Mapping::getDirect3D12UsageAndCPUAccessFlags(bufferUsage, d3d12BufferDesc.CPUAccessFlags);
				d3d12BufferDesc.BindFlags           = D3D12_BIND_SHADER_RESOURCE;
				//d3d12BufferDesc.CPUAccessFlags    = <filled above>;
				d3d12BufferDesc.MiscFlags           = 0;
				d3d12BufferDesc.StructureByteStride = 0;

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 12 subresource data
					D3D12_SUBRESOURCE_DATA d3d12SubresourceData;
					d3d12SubresourceData.pSysMem          = data;
					d3d12SubresourceData.SysMemPitch      = 0;
					d3d12SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 12 constant buffer
					FAILED_DEBUG_BREAK(direct3D12Rhi.getD3D12Device().CreateBuffer(&d3d12BufferDesc, &d3d12SubresourceData, &mD3D12Buffer))
				}
				else
				{
					// Create the Direct3D 12 constant buffer
					FAILED_DEBUG_BREAK(direct3D12Rhi.getD3D12Device().CreateBuffer(&d3d12BufferDesc, nullptr, &mD3D12Buffer))
				}
			}

			{ // Shader resource view part
				// Direct3D 12 shader resource view description
				D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
				d3d12ShaderResourceViewDesc.Format				 = Mapping::getDirect3D12Format(textureFormat);
				d3d12ShaderResourceViewDesc.ViewDimension		 = D3D12_SRV_DIMENSION_BUFFER;
				d3d12ShaderResourceViewDesc.Buffer.ElementOffset = 0;
				d3d12ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 12 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D12Rhi.getD3D12Device().CreateShaderResourceView(mD3D12Buffer, &d3d12ShaderResourceViewDesc, &mD3D12ShaderResourceViewTexture))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				if (nullptr != mD3D12Resource)
				{
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "SBO", 6)	// 6 = "SBO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
			*/
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~StructuredBuffer() override
		{
			// TODO(co) Direct3D 12 update
			/*
			// Release the used resources
			if (nullptr != mD3D12Buffer)
			{
				mD3D12Buffer->Release();
				mD3D12Buffer = nullptr;
			}
			*/
		}

		/**
		*  @brief
		*    Return the Direct3D structured buffer instance
		*
		*  @return
		*    The Direct3D structured buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12Buffer* getD3D12Buffer() const
		//{
		//	return mD3D12Buffer;
		//}


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
		// TODO(co) Direct3D 12 update
		//ID3D12Buffer* mD3D12Buffer;	///< Direct3D texture buffer instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/IndirectBuffer.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 indirect buffer object class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Rhi::IndirectBufferFlag"
		*/
		IndirectBuffer(Direct3D12Rhi& direct3D12Rhi, uint32_t numberOfBytes, const void* data, uint32_t indirectBufferFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IIndirectBuffer(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D12CommandSignature(nullptr),
			mD3D12Resource(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Direct3D 12 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RHI_ASSERT(direct3D12Rhi.getContext(), !((indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Direct3D 12 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RHI_ASSERT(direct3D12Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawArguments)) == 0, "Direct3D 12 indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RHI_ASSERT(direct3D12Rhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawIndexedArguments)) == 0, "Direct3D 12 indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// TODO(co) This is only meant for the Direct3D 12 RHI implementation kickoff.
			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.
			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfBytes);
			ID3D12Device& d3d12Device = direct3D12Rhi.getD3D12Device();
			if (SUCCEEDED(d3d12Device.CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12XResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			{
				// Data given?
				if (nullptr != data)
				{
					// Copy the data to the indirect buffer
					UINT8* pIndirectDataBegin;
					CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
					if (SUCCEEDED(mD3D12Resource->Map(0, &readRange, reinterpret_cast<void**>(&pIndirectDataBegin))))
					{
						memcpy(pIndirectDataBegin, data, numberOfBytes);
						mD3D12Resource->Unmap(0, nullptr);
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to map Direct3D 12 indirect buffer")
					}
				}

				D3D12_INDIRECT_ARGUMENT_DESC d3dIndirectArgumentDescription[1];
				d3dIndirectArgumentDescription[0].Type = (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) ? D3D12_INDIRECT_ARGUMENT_TYPE_DRAW : D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

				D3D12_COMMAND_SIGNATURE_DESC d3d12CommandSignatureDescription;
				d3d12CommandSignatureDescription.ByteStride		  = (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) ? sizeof(Rhi::DrawArguments) : sizeof(Rhi::DrawIndexedArguments);
				d3d12CommandSignatureDescription.NumArgumentDescs = 1;
				d3d12CommandSignatureDescription.pArgumentDescs   = d3dIndirectArgumentDescription;
				d3d12CommandSignatureDescription.NodeMask = 0;

				if (FAILED(d3d12Device.CreateCommandSignature(&d3d12CommandSignatureDescription, nullptr, IID_PPV_ARGS(&mD3D12CommandSignature))))
				{
					RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 command signature")
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "IndirectBufferObject", 23)	// 23 = "IndirectBufferObject: " including terminating zero
					if (nullptr != mD3D12CommandSignature)
					{
						FAILED_DEBUG_BREAK(mD3D12CommandSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 indirect buffer resource")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~IndirectBuffer() override
		{
			// Release the used resources
			if (nullptr != mD3D12CommandSignature)
			{
				mD3D12CommandSignature->Release();
			}
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 command signature instance
		*
		*  @return
		*    The Direct3D 12 command signature instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12CommandSignature* getD3D12CommandSignature() const
		{
			return mD3D12CommandSignature;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 indirect buffer resource instance
		*
		*  @return
		*    The Direct3D 12 indirect buffer resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		ID3D12CommandSignature* mD3D12CommandSignature;
		ID3D12Resource*			mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/UniformBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(Direct3D12Rhi& direct3D12Rhi, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Rhi::BufferUsage bufferUsage RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			Rhi::IUniformBuffer(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfBytesOnGpu(::detail::align(numberOfBytes, 256)),	// Constant buffer size is required to be 256-byte aligned, no assert because other RHI implementations have another alignment (DirectX 11 e.g. 16), see "ID3D12Device::CreateConstantBufferView method" at https://msdn.microsoft.com/de-de/library/windows/desktop/dn788659%28v=vs.85%29.aspx
			mD3D12Resource(nullptr),
			mMappedData(nullptr)
		{
			// TODO(co) Add buffer usage setting support
			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mNumberOfBytesOnGpu);
			if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12XResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			{
				// Data given?
				if (nullptr != data)
				{
					CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
					if (SUCCEEDED(mD3D12Resource->Map(0, &readRange, reinterpret_cast<void**>(&mMappedData))))
					{
						memcpy(mMappedData, data, numberOfBytes);
						mD3D12Resource->Unmap(0, nullptr);
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to map Direct3D 12 uniform buffer")
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "UBO", 6)	// 6 = "UBO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 uniform buffer resource")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~UniformBuffer() override
		{
			// Release the Direct3D 12 constant buffer
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the number of bytes on GPU
		*
		*  @return
		*    The number of bytes on GPU
		*/
		[[nodiscard]] inline uint32_t getNumberOfBytesOnGpu() const
		{
			return mNumberOfBytesOnGpu;
		}

		/**
		*  @brief
		*    Return the Direct3D uniform buffer resource instance
		*
		*  @return
		*    The Direct3D uniform buffer resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		uint32_t		mNumberOfBytesOnGpu;
		ID3D12Resource* mD3D12Resource;
		uint8_t*		mMappedData;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Buffer/BufferManager.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 buffer manager interface
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*/
		inline explicit BufferManager(Direct3D12Rhi& direct3D12Rhi) :
			IBufferManager(direct3D12Rhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BufferManager() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IBufferManager methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Rhi::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), VertexBuffer)(direct3D12Rhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::IndexBufferFormat::Enum indexBufferFormat = Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), IndexBuffer)(direct3D12Rhi, numberOfBytes, data, bufferUsage, indexBufferFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexArray* createVertexArray(const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, Rhi::IIndexBuffer* indexBuffer = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity checks
			#ifdef RHI_DEBUG
			{
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RHI_ASSERT(direct3D12Rhi.getContext(), &direct3D12Rhi == &vertexBuffer->vertexBuffer->getRhi(), "Direct3D 12 error: The given vertex buffer resource is owned by another RHI instance")
				}
			}
			#endif
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == indexBuffer || &direct3D12Rhi == &indexBuffer->getRhi(), "Direct3D 12 error: The given index buffer resource is owned by another RHI instance")

			// Create vertex array
			uint16_t id = 0;
			if (direct3D12Rhi.VertexArrayMakeId.CreateID(id))
			{
				return RHI_NEW(direct3D12Rhi.getContext(), VertexArray)(direct3D12Rhi, vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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

		[[nodiscard]] inline virtual Rhi::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t = Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::R32G32B32A32F RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), TextureBuffer)(direct3D12Rhi, numberOfBytes, data, bufferUsage, textureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t bufferFlags, Rhi::BufferUsage bufferUsage, uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), StructuredBuffer)(direct3D12Rhi, numberOfBytes, data, bufferUsage, numberOfStructureBytes RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), IndirectBuffer)(direct3D12Rhi, numberOfBytes, data, indirectBufferFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// RHI_ASSERT(direct3D12Rhi.getContext(), (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid Direct3D 12 buffer flags, uniform buffer can't be used for unordered access")
			// RHI_ASSERT(direct3D12Rhi.getContext(), (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0, "Invalid Direct3D 12 buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RHI_NEW(direct3D12Rhi.getContext(), UniformBuffer)(direct3D12Rhi, numberOfBytes, data, bufferUsage RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
	//[ Direct3D12Rhi/Texture/Texture1D.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 1D texture class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		Texture1D(Direct3D12Rhi& direct3D12Rhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1D(direct3D12Rhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mNumberOfMipmaps(0),
			mD3D12Resource(nullptr)
		{
			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			mNumberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(width) : 1;
			RHI_ASSERT(direct3D12Rhi.getContext(), !generateMipmaps, "TODO(co) Direct3D 12 texture mipmap generation isn't implemented, yet")
			if (generateMipmaps)
			{
				mNumberOfMipmaps = 1;
			}

			// Create the Direct3D 12 texture resource
			mD3D12Resource = TextureHelper::CreateTexture(direct3D12Rhi.getD3D12Device(), TextureHelper::TextureType::TEXTURE_1D, width, 1, 1, 1, textureFormat, 1, mNumberOfMipmaps, textureFlags, nullptr);
			if (nullptr != mD3D12Resource)
			{
				// Upload all mipmaps in case the user provided us with texture data
				if (nullptr != data)
				{
					for (uint32_t mipmap = 0; mipmap < mNumberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1);
						TextureHelper::SetTextureData(direct3D12Rhi.getUploadContext(), *mD3D12Resource, width, 1, 1, textureFormat, mNumberOfMipmaps, mipmap, 0, data, numberOfBytesPerSlice, numberOfBytesPerRow);

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture", 13)	// 13 = "1D texture: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture1D() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI format
		*
		*  @return
		*    The DDXGI format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDxgiFormat() const
		{
			return mDxgiFormat;
		}

		/**
		*  @brief
		*    Return the number of mipmaps
		*
		*  @return
		*    The number of mipmaps
		*/
		[[nodiscard]] inline uint32_t getNumberOfMipmaps() const
		{
			return mNumberOfMipmaps;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource instance
		*
		*  @return
		*    The Direct3D 12 resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		DXGI_FORMAT		mDxgiFormat;
		uint32_t		mNumberOfMipmaps;
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Texture/Texture1DArray.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 1D array texture class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
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
		Texture1DArray(Direct3D12Rhi& direct3D12Rhi, uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture1DArray(direct3D12Rhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mNumberOfMipmaps(0),
			mNumberOfSlices(numberOfSlices),
			mD3D12Resource(nullptr)
		{
			// TODO(co) Add "Rhi::TextureFlag::GENERATE_MIPMAPS" support, also for render target textures

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			mNumberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(width) : 1;
			RHI_ASSERT(direct3D12Rhi.getContext(), !generateMipmaps, "TODO(co) Direct3D 12 texture mipmap generation isn't implemented, yet")
			if (generateMipmaps)
			{
				mNumberOfMipmaps = 1;
			}

			// Create the Direct3D 12 texture resource
			mD3D12Resource = TextureHelper::CreateTexture(direct3D12Rhi.getD3D12Device(), TextureHelper::TextureType::TEXTURE_1D_ARRAY, width, 1, 1, numberOfSlices, textureFormat, 1, mNumberOfMipmaps, textureFlags, nullptr);
			if (nullptr != mD3D12Resource)
			{
				// Upload all mipmaps in case the user provided us with texture data
				if (nullptr != data)
				{
					// Data layout
					// - Direct3D 12 wants: DDS files are organized in slice-major order, like this:
					//     Slice0: Mip0, Mip1, Mip2, etc.
					//     Slice1: Mip0, Mip1, Mip2, etc.
					//     etc.
					// - The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//     Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     etc.
					for (uint32_t mipmap = 0; mipmap < mNumberOfMipmaps; ++mipmap)
					{
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							// Upload the current mipmap
							TextureHelper::SetTextureData(direct3D12Rhi.getUploadContext(), *mD3D12Resource, width, 1, 1, textureFormat, mNumberOfMipmaps, mipmap, arraySlice, data, numberOfBytesPerSlice, numberOfBytesPerRow);

							// Move on to the next slice
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "1D texture array", 19)	// 19 = "1D texture array: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture1DArray() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI format
		*
		*  @return
		*    The DDXGI format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDxgiFormat() const
		{
			return mDxgiFormat;
		}

		/**
		*  @brief
		*    Return the number of mipmaps
		*
		*  @return
		*    The number of mipmaps
		*/
		[[nodiscard]] inline uint32_t getNumberOfMipmaps() const
		{
			return mNumberOfMipmaps;
		}

		/**
		*  @brief
		*    Return the number of slices
		*
		*  @return
		*    The number of slices
		*/
		[[nodiscard]] inline uint32_t getNumberOfSlices() const
		{
			return mNumberOfSlices;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource instance
		*
		*  @return
		*    The Direct3D 12 resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		DXGI_FORMAT		mDxgiFormat;
		uint32_t		mNumberOfMipmaps;
		uint32_t		mNumberOfSlices;
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Texture/Texture2D.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 2D texture class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
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
		*  @param[in] optimizedTextureClearValue
		*    Optional optimized texture clear value
		*/
		Texture2D(Direct3D12Rhi& direct3D12Rhi, uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, uint8_t numberOfMultisamples, const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2D(direct3D12Rhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mNumberOfMipmaps(0),
			mD3D12Resource(nullptr)
		{
			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid Direct3D 12 texture parameters")
			RHI_ASSERT(direct3D12Rhi.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid Direct3D 12 texture parameters")
			RHI_ASSERT(direct3D12Rhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid Direct3D 12 texture parameters")
			RHI_ASSERT(direct3D12Rhi.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS), "Invalid Direct3D 12 texture parameters")
			RHI_ASSERT(direct3D12Rhi.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Rhi::TextureFlag::RENDER_TARGET), "Invalid Direct3D 12 texture parameters")
			RHI_ASSERT(direct3D12Rhi.getContext(), 0 == (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 12 texture parameters")
			RHI_ASSERT(direct3D12Rhi.getContext(), (textureFlags & Rhi::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 12 render target textures can't be filled using provided data")

			// TODO(co) Add "Rhi::TextureFlag::GENERATE_MIPMAPS" support, also for render target textures

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			mNumberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(width, height) : 1;
			RHI_ASSERT(direct3D12Rhi.getContext(), !generateMipmaps, "TODO(co) Direct3D 12 texture mipmap generation isn't implemented, yet")
			if (generateMipmaps)
			{
				mNumberOfMipmaps = 1;
			}

			// Create the Direct3D 12 texture resource
			mD3D12Resource = TextureHelper::CreateTexture(direct3D12Rhi.getD3D12Device(), TextureHelper::TextureType::TEXTURE_2D, width, height, 1, 1, textureFormat, numberOfMultisamples, mNumberOfMipmaps, textureFlags, optimizedTextureClearValue);
			if (nullptr != mD3D12Resource)
			{
				// Upload all mipmaps in case the user provided us with texture data
				if (nullptr != data)
				{
					for (uint32_t mipmap = 0; mipmap < mNumberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						TextureHelper::SetTextureData(direct3D12Rhi.getUploadContext(), *mD3D12Resource, width, height, 1, textureFormat, mNumberOfMipmaps, mipmap, 0, data, numberOfBytesPerSlice, numberOfBytesPerRow);

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture", 13)	// 13 = "2D texture: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2D() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI format
		*
		*  @return
		*    The DDXGI format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDxgiFormat() const
		{
			return mDxgiFormat;
		}

		/**
		*  @brief
		*    Return the number of mipmaps
		*
		*  @return
		*    The number of mipmaps
		*/
		[[nodiscard]] inline uint32_t getNumberOfMipmaps() const
		{
			return mNumberOfMipmaps;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource instance
		*
		*  @return
		*    The Direct3D 12 resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		DXGI_FORMAT		mDxgiFormat;
		uint32_t		mNumberOfMipmaps;
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Texture/Texture2DArray.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 2D array texture class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
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
		Texture2DArray(Direct3D12Rhi& direct3D12Rhi, uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture2DArray(direct3D12Rhi, width, height, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mNumberOfMipmaps(0),
			mNumberOfSlices(numberOfSlices),
			mD3D12Resource(nullptr)
		{
			// TODO(co) Add "Rhi::TextureFlag::GENERATE_MIPMAPS" support, also for render target textures

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			mNumberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(width, height) : 1;
			RHI_ASSERT(direct3D12Rhi.getContext(), !generateMipmaps, "TODO(co) Direct3D 12 texture mipmap generation isn't implemented, yet")
			if (generateMipmaps)
			{
				mNumberOfMipmaps = 1;
			}

			// Create the Direct3D 12 texture resource
			mD3D12Resource = TextureHelper::CreateTexture(direct3D12Rhi.getD3D12Device(), TextureHelper::TextureType::TEXTURE_2D_ARRAY, width, height, 1, numberOfSlices, textureFormat, 1, mNumberOfMipmaps, textureFlags, nullptr);
			if (nullptr != mD3D12Resource)
			{
				// Upload all mipmaps in case the user provided us with texture data
				if (nullptr != data)
				{
					// Data layout
					// - Direct3D 12 wants: DDS files are organized in slice-major order, like this:
					//     Slice0: Mip0, Mip1, Mip2, etc.
					//     Slice1: Mip0, Mip1, Mip2, etc.
					//     etc.
					// - The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//     Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//     etc.
					for (uint32_t mipmap = 0; mipmap < mNumberOfMipmaps; ++mipmap)
					{
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							// Upload the current mipmap
							TextureHelper::SetTextureData(direct3D12Rhi.getUploadContext(), *mD3D12Resource, width, height, 1, textureFormat, mNumberOfMipmaps, mipmap, arraySlice, data, numberOfBytesPerSlice, numberOfBytesPerRow);

							// Move on to the next slice
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "2D texture array", 19)	// 19 = "2D texture array: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2DArray() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI format
		*
		*  @return
		*    The DDXGI format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDxgiFormat() const
		{
			return mDxgiFormat;
		}

		/**
		*  @brief
		*    Return the number of mipmaps
		*
		*  @return
		*    The number of mipmaps
		*/
		[[nodiscard]] inline uint32_t getNumberOfMipmaps() const
		{
			return mNumberOfMipmaps;
		}

		/**
		*  @brief
		*    Return the number of slices
		*
		*  @return
		*    The number of slices
		*/
		[[nodiscard]] inline uint32_t getNumberOfSlices() const
		{
			return mNumberOfSlices;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource instance
		*
		*  @return
		*    The Direct3D 12 resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		DXGI_FORMAT		mDxgiFormat;
		uint32_t		mNumberOfMipmaps;
		uint32_t		mNumberOfSlices;
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Texture/Texture3D.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 3D texture class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
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
		Texture3D(Direct3D12Rhi& direct3D12Rhi, uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITexture3D(direct3D12Rhi, width, height, depth RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mNumberOfMipmaps(0),
			mD3D12Resource(nullptr)
		{
			// TODO(co) Add "Rhi::TextureFlag::GENERATE_MIPMAPS" support, also for render target textures

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			mNumberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(width, height) : 1;
			RHI_ASSERT(direct3D12Rhi.getContext(), !generateMipmaps, "TODO(co) Direct3D 12 texture mipmap generation isn't implemented, yet")
			if (generateMipmaps)
			{
				mNumberOfMipmaps = 1;
			}

			// Create the Direct3D 12 texture resource
			mD3D12Resource = TextureHelper::CreateTexture(direct3D12Rhi.getD3D12Device(), TextureHelper::TextureType::TEXTURE_3D, width, height, depth, 1, textureFormat, 1, mNumberOfMipmaps, textureFlags, nullptr);
			if (nullptr != mD3D12Resource)
			{
				// Upload all mipmaps in case the user provided us with texture data
				if (nullptr != data)
				{
					// Data layout: The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//   Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
					//   etc.
					for (uint32_t mipmap = 0; mipmap < mNumberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height) * depth;
						TextureHelper::SetTextureData(direct3D12Rhi.getUploadContext(), *mD3D12Resource, width, height, depth, textureFormat, mNumberOfMipmaps, mipmap, 0, data, numberOfBytesPerSlice, numberOfBytesPerRow);

						// Move on to the next mipmap and ensure the size is always at least 1x1x1
						data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						width = getHalfSize(width);
						height = getHalfSize(height);
						depth = getHalfSize(depth);
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "3D texture", 13)	// 13 = "3D texture: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture3D() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI format
		*
		*  @return
		*    The DDXGI format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDxgiFormat() const
		{
			return mDxgiFormat;
		}

		/**
		*  @brief
		*    Return the number of mipmaps
		*
		*  @return
		*    The number of mipmaps
		*/
		[[nodiscard]] inline uint32_t getNumberOfMipmaps() const
		{
			return mNumberOfMipmaps;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource instance
		*
		*  @return
		*    The Direct3D 12 resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		DXGI_FORMAT		mDxgiFormat;
		uint32_t		mNumberOfMipmaps;
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Texture/TextureCube.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 cube texture class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Rhi::TextureFlag::Enum"
		*/
		TextureCube(Direct3D12Rhi& direct3D12Rhi, uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			ITextureCube(direct3D12Rhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mNumberOfMipmaps(0),
			mD3D12Resource(nullptr)
		{
			static constexpr uint32_t NUMBER_OF_SLICES = 6;	// In Direct3D 12, a cube map is a 2D array texture with six slices

			// TODO(co) Add "Rhi::TextureFlag::GENERATE_MIPMAPS" support, also for render target textures

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Rhi::TextureFlag::GENERATE_MIPMAPS));
			mNumberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? Rhi::ITexture::getNumberOfMipmaps(width) : 1;
			RHI_ASSERT(direct3D12Rhi.getContext(), !generateMipmaps, "TODO(co) Direct3D 12 texture mipmap generation isn't implemented, yet")
			if (generateMipmaps)
			{
				mNumberOfMipmaps = 1;
			}

			// Create the Direct3D 12 texture resource
			mD3D12Resource = TextureHelper::CreateTexture(direct3D12Rhi.getD3D12Device(), TextureHelper::TextureType::TEXTURE_CUBE, width, width, 1, NUMBER_OF_SLICES, textureFormat, 1, mNumberOfMipmaps, textureFlags, nullptr);
			if (nullptr != mD3D12Resource)
			{
				// Upload all mipmaps in case the user provided us with texture data
				if (nullptr != data)
				{
					// Data layout
					// - Direct3D 12 wants: DDS files are organized in face-major order, like this:
					//     Face0: Mip0, Mip1, Mip2, etc.
					//     Face1: Mip0, Mip1, Mip2, etc.
					//     etc.
					// - The RHI provides: CRN and KTX files are organized in mip-major order, like this:
					//     Mip0: Face0, Face1, Face2, Face3, Face4, Face5
					//     Mip1: Face0, Face1, Face2, Face3, Face4, Face5
					//     etc.
					for (uint32_t mipmap = 0; mipmap < mNumberOfMipmaps; ++mipmap)
					{
						// TODO(co) Is it somehow possible to upload a whole cube texture mipmap in one burst?
						const uint32_t numberOfBytesPerRow   = Rhi::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t numberOfBytesPerSlice = Rhi::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, width);
						for (uint32_t arraySlice = 0; arraySlice < NUMBER_OF_SLICES; ++arraySlice)
						{
							// Upload the current mipmap
							TextureHelper::SetTextureData(direct3D12Rhi.getUploadContext(), *mD3D12Resource, width, width, 1, textureFormat, mNumberOfMipmaps, mipmap, arraySlice, data, numberOfBytesPerSlice, numberOfBytesPerRow);

							// Move on to the next slice
							data = static_cast<const uint8_t*>(data) + numberOfBytesPerSlice;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						width = getHalfSize(width);
					}
				}

				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Cube texture", 15)	// 15 = "Cube texture: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureCube() override
		{
			if (nullptr != mD3D12Resource)
			{
				mD3D12Resource->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI format
		*
		*  @return
		*    The DXGI format
		*/
		[[nodiscard]] inline DXGI_FORMAT getDxgiFormat() const
		{
			return mDxgiFormat;
		}

		/**
		*  @brief
		*    Return the number of mipmaps
		*
		*  @return
		*    The number of mipmaps
		*/
		[[nodiscard]] inline uint32_t getNumberOfMipmaps() const
		{
			return mNumberOfMipmaps;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource instance
		*
		*  @return
		*    The Direct3D 12 resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Resource* getD3D12Resource() const
		{
			return mD3D12Resource;
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
		DXGI_FORMAT		mDxgiFormat;
		uint32_t		mNumberOfMipmaps;
		ID3D12Resource* mD3D12Resource;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Texture/TextureManager.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 texture manager interface
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*/
		inline explicit TextureManager(Direct3D12Rhi& direct3D12Rhi) :
			ITextureManager(direct3D12Rhi)
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
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), width > 0, "Direct3D 12 create texture 1D was called with invalid parameters")

			// Create 1D texture resource, texture usage isn't supported
			return RHI_NEW(direct3D12Rhi.getContext(), Texture1D)(direct3D12Rhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), width > 0 && numberOfSlices > 0, "Direct3D 12 create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource, texture usage isn't supported
			return RHI_NEW(direct3D12Rhi.getContext(), Texture1DArray)(direct3D12Rhi, width, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), width > 0 && height > 0, "Direct3D 12 create texture 2D was called with invalid parameters")

			// Create 2D texture resource, texture usage isn't supported
			return RHI_NEW(direct3D12Rhi.getContext(), Texture2D)(direct3D12Rhi, width, height, textureFormat, data, textureFlags, numberOfMultisamples, optimizedTextureClearValue RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), width > 0 && height > 0 && numberOfSlices > 0, "Direct3D 12 create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource, texture usage isn't supported
			return RHI_NEW(direct3D12Rhi.getContext(), Texture2DArray)(direct3D12Rhi, width, height, numberOfSlices, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), width > 0 && height > 0 && depth > 0, "Direct3D 12 create texture 3D was called with invalid parameters")

			// Create 3D texture resource, texture usage isn't supported
			return RHI_NEW(direct3D12Rhi.getContext(), Texture3D)(direct3D12Rhi, width, height, depth, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCube* createTextureCube(uint32_t width, Rhi::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), width > 0, "Direct3D 12 create texture cube was called with invalid parameters")

			// Create cube texture resource, texture usage isn't supported
			return RHI_NEW(direct3D12Rhi.getContext(), TextureCube)(direct3D12Rhi, width, textureFormat, data, textureFlags RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/State/SamplerState.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 sampler state class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(Direct3D12Rhi& direct3D12Rhi, const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISamplerState(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mSamplerState(samplerState)
		{
			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), samplerState.filter != Rhi::FilterMode::UNKNOWN, "Direct3D 12 filter mode must not be unknown")
			RHI_ASSERT(direct3D12Rhi.getContext(), samplerState.maxAnisotropy <= direct3D12Rhi.getCapabilities().maximumAnisotropy, "Maximum Direct3D 12 anisotropy value violated")
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SamplerState() override
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the sampler state
		*
		*  @return
		*    The sampler state
		*/
		[[nodiscard]] inline const Rhi::SamplerState& getSamplerState() const
		{
			return mSamplerState;
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
		Rhi::SamplerState mSamplerState;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/RenderTarget/RenderPass.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 render pass interface
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
		*/
		RenderPass(Rhi::IRhi& rhi, uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRenderPass(rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat)
		{
			RHI_ASSERT(rhi.getContext(), mNumberOfColorAttachments < 8, "Invalid number of Direct3D 12 color attachments")
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
			RHI_ASSERT(getRhi().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid Direct3D 12 color attachment index")
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


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/QueryPool.h                             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 asynchronous query pool interface
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] queryType
		*    Query type
		*  @param[in] numberOfQueries
		*    Number of queries
		*/
		QueryPool(Direct3D12Rhi& direct3D12Rhi, Rhi::QueryType queryType, uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IQueryPool(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mQueryType(queryType),
			mNumberOfQueries(numberOfQueries),
			mD3D12QueryHeap(nullptr),
			mD3D12ResourceQueryHeapResultReadback(nullptr),
			mResolveToFrameNumber(0)
		{
			ID3D12Device& d3d12Device = direct3D12Rhi.getD3D12Device();
			uint32_t numberOfBytesPerQuery = 0;

			{ // Get Direct3D 12 query description
				D3D12_QUERY_HEAP_DESC d3d12QueryHeapDesc = {};
				switch (queryType)
				{
					case Rhi::QueryType::OCCLUSION:
						d3d12QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
						numberOfBytesPerQuery = sizeof(uint64_t);
						break;

					case Rhi::QueryType::PIPELINE_STATISTICS:
						d3d12QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
						numberOfBytesPerQuery = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
						break;

					case Rhi::QueryType::TIMESTAMP:
						d3d12QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
						numberOfBytesPerQuery = sizeof(uint64_t);
						break;
				}
				d3d12QueryHeapDesc.Count = numberOfQueries;

				// Create Direct3D 12 query heap
				FAILED_DEBUG_BREAK(d3d12Device.CreateQueryHeap(&d3d12QueryHeapDesc, IID_PPV_ARGS(&mD3D12QueryHeap)))
			}

			{ // Create the Direct3D 12 resource for query heap result readback
			  // -> Due to the asynchronous nature of queries (see "ID3D12GraphicsCommandList::ResolveQueryData()"), we need a result readback buffer which can hold enough frames
			  // +1 = One more frame as an instance is guaranteed to be written to if "Direct3D12Rhi::NUMBER_OF_FRAMES" frames have been submitted since. This is due to a fact that present stalls when none of the maximum number of frames are done/available.
				const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_READBACK);
				const D3D12_RESOURCE_DESC d3d12ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(numberOfBytesPerQuery) * numberOfQueries * (Direct3D12Rhi::NUMBER_OF_FRAMES + 1));
				FAILED_DEBUG_BREAK(d3d12Device.CreateCommittedResource(&d3d12XHeapProperties, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mD3D12ResourceQueryHeapResultReadback)))
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				switch (queryType)
				{
					case Rhi::QueryType::OCCLUSION:
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Occlusion query", 18)	// 18 = "Occlusion query: " including terminating zero
						if (nullptr != mD3D12QueryHeap)
						{
							FAILED_DEBUG_BREAK(mD3D12QueryHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
						}
						if (nullptr != mD3D12ResourceQueryHeapResultReadback)
						{
							FAILED_DEBUG_BREAK(mD3D12ResourceQueryHeapResultReadback->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
						}
						break;
					}

					case Rhi::QueryType::PIPELINE_STATISTICS:
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Pipeline statistics query", 28)	// 28 = "Pipeline statistics query: " including terminating zero
						if (nullptr != mD3D12QueryHeap)
						{
							FAILED_DEBUG_BREAK(mD3D12QueryHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
						}
						if (nullptr != mD3D12ResourceQueryHeapResultReadback)
						{
							FAILED_DEBUG_BREAK(mD3D12ResourceQueryHeapResultReadback->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
						}
						break;
					}

					case Rhi::QueryType::TIMESTAMP:
					{
						RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Timestamp query", 18)	// 18 = "Timestamp query: " including terminating zero
						if (nullptr != mD3D12QueryHeap)
						{
							FAILED_DEBUG_BREAK(mD3D12QueryHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
						}
						if (nullptr != mD3D12ResourceQueryHeapResultReadback)
						{
							FAILED_DEBUG_BREAK(mD3D12ResourceQueryHeapResultReadback->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
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
			if (nullptr != mD3D12QueryHeap)
			{
				mD3D12QueryHeap->Release();
			}
			if (nullptr != mD3D12ResourceQueryHeapResultReadback)
			{
				mD3D12ResourceQueryHeapResultReadback->Release();
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
		*    Return the Direct3D 12 query heap
		*
		*  @return
		*    The Direct3D 12 query heap
		*/
		[[nodiscard]] inline ID3D12QueryHeap* getD3D12QueryHeap() const
		{
			return mD3D12QueryHeap;
		}

		/**
		*  @brief
		*    Get asynchronous query pool results
		*
		*  @param[in] numberOfDataBytes
		*    Number of data bytes
		*  @param[out] data
		*    Receives the query data
		*  @param[in] firstQueryIndex
		*    First query index (e.g. 0)
		*  @param[in] numberOfQueries
		*    Number of queries (e.g. 1)
		*  @param[in] strideInBytes
		*    Stride in bytes, 0 is only valid in case there's just a single query
		*  @param[in] d3d12GraphicsCommandList
		*    Direct3D 12 graphics command list
		*/
		void getQueryPoolResults([[maybe_unused]] uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, [[maybe_unused]] uint32_t strideInBytes, ID3D12GraphicsCommandList& d3d12GraphicsCommandList)
		{
			// Query pool type dependent processing
			// -> We don't support "Rhi::QueryResultFlags::WAIT"
			RHI_ASSERT(getRhi().getContext(), firstQueryIndex < mNumberOfQueries, "Direct3D 12 out-of-bounds query index")
			RHI_ASSERT(getRhi().getContext(), (firstQueryIndex + numberOfQueries) <= mNumberOfQueries, "Direct3D 12 out-of-bounds query index")
			D3D12_QUERY_TYPE d3d12QueryType = D3D12_QUERY_TYPE_OCCLUSION;
			uint32_t numberOfBytesPerQuery = 0;
			switch (mQueryType)
			{
				case Rhi::QueryType::OCCLUSION:
				{
					RHI_ASSERT(getRhi().getContext(), 1 == numberOfQueries || sizeof(uint64_t) == strideInBytes, "Direct3D 12 stride in bytes must be 8 bytes for occlusion query type")
					d3d12QueryType = D3D12_QUERY_TYPE_OCCLUSION;
					numberOfBytesPerQuery = sizeof(uint64_t);
					break;
				}

				case Rhi::QueryType::PIPELINE_STATISTICS:
				{
					static_assert(sizeof(Rhi::PipelineStatisticsQueryResult) == sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS), "Direct3D 12 structure mismatch detected");
					RHI_ASSERT(getRhi().getContext(), numberOfDataBytes >= sizeof(Rhi::PipelineStatisticsQueryResult), "Direct3D 12 out-of-memory query access")
					RHI_ASSERT(getRhi().getContext(), 1 == numberOfQueries || strideInBytes >= sizeof(Rhi::PipelineStatisticsQueryResult), "Direct3D 12 out-of-memory query access")
					RHI_ASSERT(getRhi().getContext(), 1 == numberOfQueries || sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS) == strideInBytes, "Direct3D 12 stride in bytes must be 88 bytes for pipeline statistics query type")
					d3d12QueryType = D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
					numberOfBytesPerQuery = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
					break;
				}

				case Rhi::QueryType::TIMESTAMP:	// TODO(co) Convert time to nanoseconds, see e.g. http://reedbeta.com/blog/gpu-profiling-101/
				{
					RHI_ASSERT(getRhi().getContext(), 1 == numberOfQueries || sizeof(uint64_t) == strideInBytes, "Direct3D 12 stride in bytes must be 8 bytes for timestamp query type")
					d3d12QueryType = D3D12_QUERY_TYPE_TIMESTAMP;
					numberOfBytesPerQuery = sizeof(uint64_t);
					break;
				}
			}

 			{ // Resolve query data from the current frame
				const uint64_t resolveToBaseAddress = static_cast<uint64_t>(numberOfBytesPerQuery) * mNumberOfQueries * mResolveToFrameNumber + static_cast<uint64_t>(numberOfBytesPerQuery) * firstQueryIndex;
				d3d12GraphicsCommandList.ResolveQueryData(mD3D12QueryHeap, d3d12QueryType, firstQueryIndex, numberOfQueries, mD3D12ResourceQueryHeapResultReadback, resolveToBaseAddress);
			}

			// Readback query result by grabbing readback data for the queries from a finished frame "Direct3D12Rhi::NUMBER_OF_FRAMES" ago
			// +1 = One more frame as an instance is guaranteed to be written to if "Direct3D12Rhi::NUMBER_OF_FRAMES" frames have been submitted since. This is due to a fact that present stalls when none of the maximum number of frames are done/available.
			const uint32_t readbackFrameNumber = (mResolveToFrameNumber + 1) % (Direct3D12Rhi::NUMBER_OF_FRAMES + 1);
			const uint32_t readbackBaseOffset = numberOfBytesPerQuery * mNumberOfQueries * readbackFrameNumber + numberOfBytesPerQuery * firstQueryIndex;
			const D3D12_RANGE d3d12Range = { readbackBaseOffset, readbackBaseOffset + numberOfBytesPerQuery * numberOfQueries };
			uint64_t* timingData = nullptr;
			FAILED_DEBUG_BREAK(mD3D12ResourceQueryHeapResultReadback->Map(0, &d3d12Range, reinterpret_cast<void**>(&timingData)))
			memcpy(data, timingData, numberOfBytesPerQuery * numberOfQueries);
			mD3D12ResourceQueryHeapResultReadback->Unmap(0, nullptr);
			mResolveToFrameNumber = readbackFrameNumber;
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
		Rhi::QueryType   mQueryType;
		uint32_t		 mNumberOfQueries;
		ID3D12QueryHeap* mD3D12QueryHeap;
		ID3D12Resource*	 mD3D12ResourceQueryHeapResultReadback;
		uint32_t		 mResolveToFrameNumber;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/RenderTarget/SwapChain.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 swap chain class
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
			mDxgiSwapChain3(nullptr),
			mD3D12DescriptorHeapRenderTargetView(nullptr),
			mD3D12DescriptorHeapDepthStencilView(nullptr),
			mRenderTargetViewDescriptorSize(0),
			mD3D12ResourceRenderTargets{},
			mD3D12ResourceDepthStencil(nullptr),
			mSynchronizationInterval(0),
			mFrameIndex(0),
			mFenceEvent(nullptr),
			mD3D12Fence(nullptr),
			mFenceValue(0)
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(renderPass.getRhi());
			const RenderPass& d3d12RenderPass = static_cast<RenderPass&>(renderPass);

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), 1 == d3d12RenderPass.getNumberOfColorAttachments(), "There must be exactly one Direct3D 12 render pass color attachment")

			// Get the native window handle
			const HWND hWnd = reinterpret_cast<HWND>(windowHandle.nativeWindowHandle);

			// Get our IDXGI factory instance
			IDXGIFactory4& dxgiFactory4 = direct3D12Rhi.getDxgiFactory4();

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

			// TODO(co) Add tearing support, see Direct3D 11 RHI implementation
			// Determines whether tearing support is available for fullscreen borderless windows
			// -> To unlock frame rates of UWP applications on the Windows Store and providing support for both AMD Freesync and NVIDIA's G-SYNC we must explicitly allow tearing
			// -> See "Windows Dev Center" -> "Variable refresh rate displays": https://msdn.microsoft.com/en-us/library/windows/desktop/mt742104(v=vs.85).aspx

			// Create the swap chain
			DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc = {};
			dxgiSwapChainDesc.BufferCount							= Direct3D12Rhi::NUMBER_OF_FRAMES;
			dxgiSwapChainDesc.BufferDesc.Width						= static_cast<UINT>(width);
			dxgiSwapChainDesc.BufferDesc.Height						= static_cast<UINT>(height);
			dxgiSwapChainDesc.BufferDesc.Format						= Mapping::getDirect3D12Format(d3d12RenderPass.getColorAttachmentTextureFormat(0));
			dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator		= 60;
			dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator	= 1;
			dxgiSwapChainDesc.BufferUsage							= DXGI_USAGE_RENDER_TARGET_OUTPUT;
			dxgiSwapChainDesc.SwapEffect							= DXGI_SWAP_EFFECT_FLIP_DISCARD;
			dxgiSwapChainDesc.OutputWindow							= hWnd;
			dxgiSwapChainDesc.SampleDesc.Count						= 1;
			dxgiSwapChainDesc.Windowed								= TRUE;
			IDXGISwapChain* dxgiSwapChain = nullptr;
			FAILED_DEBUG_BREAK(dxgiFactory4.CreateSwapChain(direct3D12Rhi.getD3D12CommandQueue(), &dxgiSwapChainDesc, &dxgiSwapChain))
			if (FAILED(dxgiSwapChain->QueryInterface(IID_PPV_ARGS(&mDxgiSwapChain3))))
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to retrieve the Direct3D 12 DXGI swap chain 3")
			}
			dxgiSwapChain->Release();

			// Disable alt-return for automatic fullscreen state change
			// -> We handle this manually to have more control over it
			FAILED_DEBUG_BREAK(dxgiFactory4.MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER))

			// Create the Direct3D 12 views
			if (nullptr != mDxgiSwapChain3)
			{
				createDirect3D12Views();
			}

			// Create synchronization objects
			if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mD3D12Fence))))
			{
				mFenceValue = 1;

				// Create an event handle to use for frame synchronization
				mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
				RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != mFenceEvent, "Failed to create an Direct3D 12 event handle to use for frame synchronization. Error code %u", ::GetLastError())
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create Direct3D 12 fence instance")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Swap chain", 13)	// 13 = "Swap chain: " including terminating zero

				// Assign a debug name to the DXGI swap chain
				if (nullptr != mDxgiSwapChain3)
				{
					FAILED_DEBUG_BREAK(mDxgiSwapChain3->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}

				// Assign a debug name to the Direct3D 12 frame resources
				for (int frame = 0; frame < Direct3D12Rhi::NUMBER_OF_FRAMES; ++frame)
				{
					if (nullptr != mD3D12ResourceRenderTargets[frame])
					{
						FAILED_DEBUG_BREAK(mD3D12ResourceRenderTargets[frame]->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
					}
				}
				if (nullptr != mD3D12ResourceDepthStencil)
				{
					FAILED_DEBUG_BREAK(mD3D12ResourceDepthStencil->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}

				// Assign a debug name to the Direct3D 12 descriptor heaps
				if (nullptr != mD3D12DescriptorHeapRenderTargetView)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapRenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
				if (nullptr != mD3D12DescriptorHeapDepthStencilView)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				}
			#endif
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
			destroyDirect3D12Views();
			if (nullptr != mDxgiSwapChain3)
			{
				mDxgiSwapChain3->Release();
			}

			// Destroy synchronization objects
			if (nullptr != mFenceEvent)
			{
				::CloseHandle(mFenceEvent);
			}
			if (nullptr != mD3D12Fence)
			{
				mD3D12Fence->Release();
			}
		}

		/**
		*  @brief
		*    Return the DXGI swap chain 3 instance
		*
		*  @return
		*    The DXGI swap chain 3 instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDXGISwapChain3* getDxgiSwapChain3() const
		{
			return mDxgiSwapChain3;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 render target view descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 render target view descriptor heap instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeapRenderTargetView() const
		{
			return mD3D12DescriptorHeapRenderTargetView;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 depth stencil view descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 depth stencil view descriptor heap instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeapDepthStencilView() const
		{
			return mD3D12DescriptorHeapDepthStencilView;
		}

		/**
		*  @brief
		*    Return the render target view descriptor size
		*
		*  @return
		*    The render target view descriptor size
		*
		*  @note
		*    - It's highly recommended to not keep any backups of this value, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline UINT getRenderTargetViewDescriptorSize() const
		{
			return mRenderTargetViewDescriptorSize;
		}

		/**
		*  @brief
		*    Return the index of the Direct3D 12 resource render target which is currently used as back buffer
		*
		*  @return
		*    The index of the Direct3D 12 resource render target which is currently used as back buffer
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline UINT getBackD3D12ResourceRenderTargetFrameIndex() const
		{
			return mFrameIndex;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 resource render target which is currently used as back buffer
		*
		*  @return
		*    The Direct3D 12 resource render target which is currently used as back buffer, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D12Resource* getBackD3D12ResourceRenderTarget() const
		{
			return mD3D12ResourceRenderTargets[mFrameIndex];
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRenderTarget methods             ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				// Get the Direct3D 12 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDesc(&dxgiSwapChainDesc))

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
			if (nullptr != mDxgiSwapChain3)
			{
				// Get the Direct3D 12 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDesc(&dxgiSwapChainDesc))

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
			if (nullptr != mDxgiSwapChain3)
			{
				handleDeviceLost(static_cast<Direct3D12Rhi&>(getRenderPass().getRhi()), mDxgiSwapChain3->Present(mSynchronizationInterval, 0));

				// Wait for the GPU to be done with all resources
				waitForPreviousFrame();
			}
		}

		virtual void resizeBuffers() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

				// Get the currently set render target
				Rhi::IRenderTarget* renderTargetBackup = direct3D12Rhi.omGetRenderTarget();

				// In case this swap chain is the current render target, we have to unset it before continuing
				if (this == renderTargetBackup)
				{
					direct3D12Rhi.setGraphicsRenderTarget(nullptr);
				}
				else
				{
					renderTargetBackup = nullptr;
				}

				// Release the views
				destroyDirect3D12Views();

				// Get the swap chain width and height, ensures they are never ever zero
				UINT width  = 1;
				UINT height = 1;
				getSafeWidthAndHeight(width, height);

				// Resize the Direct3D 12 swap chain
				// -> Preserve the existing buffer count and format
				const HRESULT result = mDxgiSwapChain3->ResizeBuffers(Direct3D12Rhi::NUMBER_OF_FRAMES, width, height, Mapping::getDirect3D12Format(static_cast<RenderPass&>(getRenderPass()).getColorAttachmentTextureFormat(0)), 0);
				if (SUCCEEDED(result))
				{
					// Create the Direct3D 12 views
					// TODO(co) Rescue and reassign the resource debug name
					createDirect3D12Views();

					// If required, restore the previously set render target
					if (nullptr != renderTargetBackup)
					{
						direct3D12Rhi.setGraphicsRenderTarget(renderTargetBackup);
					}
				}
				else
				{
					handleDeviceLost(direct3D12Rhi, result);
				}
			}
		}

		[[nodiscard]] virtual bool getFullscreenState() const override
		{
			// Window mode by default
			BOOL fullscreen = FALSE;

			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetFullscreenState(&fullscreen, nullptr))
			}

			// Done
			return (FALSE != fullscreen);
		}

		virtual void setFullscreenState(bool fullscreen) override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				const HRESULT result = mDxgiSwapChain3->SetFullscreenState(fullscreen, nullptr);
				if (FAILED(result))
				{
					// TODO(co) Better error handling
					RHI_ASSERT(static_cast<Direct3D12Rhi&>(getRhi()).getContext(), false, "Failed to set Direct3D 12 fullscreen state")
				}
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
		*     "D3D12: ERROR: ID3D12Device::CreateTexture2D: The Dimensions are invalid. For feature level D3D_FEATURE_LEVEL_12_0,
		*      the Width (value = 116) must be between 1 and 16384, inclusively. The Height (value = 0) must be between 1 and 16384,
		*      inclusively. And, the ArraySize (value = 1) must be between 1 and 2048, inclusively. [ STATE_CREATION ERROR #101: CREATETEXTURE2D_INVALIDDIMENSIONS ]"
		*   including an evil memory leak. So, best to use this method which gets the width and height of the native output
		*   window manually and ensures it's never zero.
		*
		*  @note
		*    - "mDxgiSwapChain3" must be valid when calling this method
		*/
		void getSafeWidthAndHeight(uint32_t& width, uint32_t& height) const
		{
			// Get the Direct3D 12 swap chain description
			DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
			FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDesc(&dxgiSwapChainDesc))

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
		*    Create the Direct3D 12 views
		*/
		void createDirect3D12Views()
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != mDxgiSwapChain3, "Invalid Direct3D 12 DXGI swap chain 3")

			// TODO(co) Debug name gets lost when resizing a window, fix this

			// Get the Direct3D 12 device instance
			ID3D12Device& d3d12Device = direct3D12Rhi.getD3D12Device();

			{ // Describe and create a render target view (RTV) descriptor heap
				D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
				d3d12DescriptorHeapDesc.NumDescriptors	= Direct3D12Rhi::NUMBER_OF_FRAMES;
				d3d12DescriptorHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				d3d12DescriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				if (SUCCEEDED(d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapRenderTargetView))))
				{
					mRenderTargetViewDescriptorSize = d3d12Device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

					{ // Create frame resources
						CD3DX12_CPU_DESCRIPTOR_HANDLE d3d12XCpuDescriptorHandle(mD3D12DescriptorHeapRenderTargetView->GetCPUDescriptorHandleForHeapStart());

						// Create a RTV for each frame
						for (UINT frame = 0; frame < Direct3D12Rhi::NUMBER_OF_FRAMES; ++frame)
						{
							if (SUCCEEDED(mDxgiSwapChain3->GetBuffer(frame, IID_PPV_ARGS(&mD3D12ResourceRenderTargets[frame]))))
							{
								d3d12Device.CreateRenderTargetView(mD3D12ResourceRenderTargets[frame], nullptr, d3d12XCpuDescriptorHandle);
								d3d12XCpuDescriptorHandle.Offset(1, mRenderTargetViewDescriptorSize);
							}
							else
							{
								RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to retrieve frame buffer from Direct3D 12 DXGI swap chain")
							}
						}
					}

					mFrameIndex = mDxgiSwapChain3->GetCurrentBackBufferIndex();
				}
				else
				{
					RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to describe and create a Direct3D 12 render target view (RTV) descriptor heap")
				}
			}

			// Describe and create a depth stencil view (DSV) descriptor heap
			const Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat = static_cast<RenderPass&>(getRenderPass()).getDepthStencilAttachmentTextureFormat();
			if (Rhi::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat)
			{
				D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
				d3d12DescriptorHeapDesc.NumDescriptors	= 1;
				d3d12DescriptorHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				d3d12DescriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				if (SUCCEEDED(d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapDepthStencilView))))
				{
					D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
					depthStencilDesc.Format = Mapping::getDirect3D12Format(depthStencilAttachmentTextureFormat);
					depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

					D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
					depthOptimizedClearValue.Format = depthStencilDesc.Format;
					depthOptimizedClearValue.DepthStencil.Depth = 0.0f;	// z = 0 instead of 1 due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
					depthOptimizedClearValue.DepthStencil.Stencil = 0;

					// Get the swap chain width and height, ensures they are never ever zero
					UINT width  = 1;
					UINT height = 1;
					getSafeWidthAndHeight(width, height);

					const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
					const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthStencilDesc.Format, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
					if (SUCCEEDED(d3d12Device.CreateCommittedResource(
						&d3d12XHeapProperties,
						D3D12_HEAP_FLAG_NONE,
						&d3d12XResourceDesc,
						D3D12_RESOURCE_STATE_DEPTH_WRITE,
						&depthOptimizedClearValue,
						IID_PPV_ARGS(&mD3D12ResourceDepthStencil)
						)))
					{
						d3d12Device.CreateDepthStencilView(mD3D12ResourceDepthStencil, &depthStencilDesc, mD3D12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart());
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create the Direct3D 12 depth stencil view (DSV) resource")
					}
				}
				else
				{
					RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to describe and create a Direct3D 12 depth stencil view (DSV) descriptor heap")
				}
			}
		}

		/**
		*  @brief
		*    Destroy the Direct3D 12 views
		*/
		void destroyDirect3D12Views()
		{
			// Wait for the GPU to be done with all resources
			waitForPreviousFrame();

			// Release Direct3D 12 resources
			for (int frame = 0; frame < Direct3D12Rhi::NUMBER_OF_FRAMES; ++frame)
			{
				if (nullptr != mD3D12ResourceRenderTargets[frame])
				{
					mD3D12ResourceRenderTargets[frame]->Release();
					mD3D12ResourceRenderTargets[frame] = nullptr;
				}
			}
			if (nullptr != mD3D12ResourceDepthStencil)
			{
				mD3D12ResourceDepthStencil->Release();
				mD3D12ResourceDepthStencil = nullptr;
			}

			// Release Direct3D 12 descriptor heap
			if (nullptr != mD3D12DescriptorHeapRenderTargetView)
			{
				mD3D12DescriptorHeapRenderTargetView->Release();
				mD3D12DescriptorHeapRenderTargetView = nullptr;
			}
			if (nullptr != mD3D12DescriptorHeapDepthStencilView)
			{
				mD3D12DescriptorHeapDepthStencilView->Release();
				mD3D12DescriptorHeapDepthStencilView = nullptr;
			}
		}

		/**
		*  @brief
		*    Wait for the GPU to be done with all resources
		*/
		void waitForPreviousFrame()
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != mDxgiSwapChain3, "Invalid Direct3D 12 DXGI swap chain 3")

			// TODO(co) This is the most simple but least effective approach and only meant for the Direct3D 12 RHI implementation kickoff.

			// Signal and increment the fence value
			const UINT64 fence = mFenceValue;
			if (SUCCEEDED(direct3D12Rhi.getD3D12CommandQueue()->Signal(mD3D12Fence, fence)))
			{
				++mFenceValue;

				// Wait until the previous frame is finished
				if (mD3D12Fence->GetCompletedValue() < fence)
				{
					if (SUCCEEDED(mD3D12Fence->SetEventOnCompletion(fence, mFenceEvent)))
					{
						::WaitForSingleObject(mFenceEvent, INFINITE);
					}
					else
					{
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to set Direct3D 12 event on completion")
					}
				}

				mFrameIndex = mDxgiSwapChain3->GetCurrentBackBufferIndex();
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IDXGISwapChain3*	  mDxgiSwapChain3;												///< The DXGI swap chain 3 instance, null pointer on error
		ID3D12DescriptorHeap* mD3D12DescriptorHeapRenderTargetView;							///< The Direct3D 12 render target view descriptor heap instance, null pointer on error
		ID3D12DescriptorHeap* mD3D12DescriptorHeapDepthStencilView;							///< The Direct3D 12 depth stencil view descriptor heap instance, null pointer on error
		UINT				  mRenderTargetViewDescriptorSize;								///< Render target view descriptor size
		ID3D12Resource*		  mD3D12ResourceRenderTargets[Direct3D12Rhi::NUMBER_OF_FRAMES];	///< The Direct3D 12 render target instances, null pointer on error
		ID3D12Resource*		  mD3D12ResourceDepthStencil;									///< The Direct3D 12 depth stencil instance, null pointer on error

		// Synchronization objects
		uint32_t	 mSynchronizationInterval;
		UINT		 mFrameIndex;
		HANDLE		 mFenceEvent;
		ID3D12Fence* mD3D12Fence;
		UINT64		 mFenceValue;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/RenderTarget/Framebuffer.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 framebuffer class
	*
	*  @todo
	*    - TODO(co) "D3D12GraphicsCommandList::OMSetRenderTargets()" supports using a single Direct3D 12 render target view descriptor heap instance with multiple targets in it, use it
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
			mD3D12DescriptorHeapRenderTargetViews(nullptr),
			mD3D12DescriptorHeapDepthStencilView(nullptr)
		{
			// Get the Direct3D 12 device instance
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(renderPass.getRhi());
			ID3D12Device& d3d12Device = direct3D12Rhi.getD3D12Device();

			// Add a reference to the used color textures
			if (mNumberOfColorTextures > 0)
			{
				const Rhi::Context& context = direct3D12Rhi.getContext();
				mColorTextures = RHI_MALLOC_TYPED(context, Rhi::ITexture*, mNumberOfColorTextures);
				mD3D12DescriptorHeapRenderTargetViews = RHI_MALLOC_TYPED(context, ID3D12DescriptorHeap*, mNumberOfColorTextures);

				// Loop through all color textures
				ID3D12DescriptorHeap** d3d12DescriptorHeapRenderTargetView = mD3D12DescriptorHeapRenderTargetViews;
				Rhi::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Rhi::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments, ++d3d12DescriptorHeapRenderTargetView)
				{
					// Sanity check
					RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid Direct3D 12 color framebuffer attachment texture")

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
							RHI_ASSERT(direct3D12Rhi.getContext(), colorFramebufferAttachments->mipmapIndex < Rhi::ITexture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 12 color framebuffer attachment mipmap index")
							RHI_ASSERT(direct3D12Rhi.getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid Direct3D 12 color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

							// Get the Direct3D 12 resource
							ID3D12Resource* d3d12Resource = texture2D->getD3D12Resource();

							// Create the Direct3D 12 render target view instance
							D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
							d3d12DescriptorHeapDesc.NumDescriptors = 1;
							d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
							if (SUCCEEDED(d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(d3d12DescriptorHeapRenderTargetView))))
							{
								D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
								d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2D->getDxgiFormat());
								d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
								d3d12RenderTargetViewDesc.Texture2D.MipSlice = colorFramebufferAttachments->mipmapIndex;
								d3d12Device.CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, (*d3d12DescriptorHeapRenderTargetView)->GetCPUDescriptorHandleForHeapStart());
							}
							break;
						}

						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);

							// Get the Direct3D 12 resource
							ID3D12Resource* d3d12Resource = texture2DArray->getD3D12Resource();

							// Create the Direct3D 12 render target view instance
							D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
							d3d12DescriptorHeapDesc.NumDescriptors = 1;
							d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
							if (SUCCEEDED(d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(d3d12DescriptorHeapRenderTargetView))))
							{
								D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
								d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2DArray->getDxgiFormat());
								d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
								d3d12RenderTargetViewDesc.Texture2DArray.MipSlice		 = colorFramebufferAttachments->mipmapIndex;
								d3d12RenderTargetViewDesc.Texture2DArray.FirstArraySlice = colorFramebufferAttachments->layerIndex;
								d3d12RenderTargetViewDesc.Texture2DArray.ArraySize		 = 1;
								d3d12RenderTargetViewDesc.Texture2DArray.PlaneSlice		 = 0;
								d3d12Device.CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, (*d3d12DescriptorHeapRenderTargetView)->GetCPUDescriptorHandleForHeapStart());
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
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "The type of the given color texture at index %u is not supported by the Direct3D 12 RHI implementation", colorTexture - mColorTextures)
							*d3d12DescriptorHeapRenderTargetView = nullptr;
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != mDepthStencilTexture, "Invalid Direct3D 12 depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Rhi::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RHI_ASSERT(direct3D12Rhi.getContext(), depthStencilFramebufferAttachment->mipmapIndex < Rhi::ITexture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 12 depth stencil framebuffer attachment mipmap index")
						RHI_ASSERT(direct3D12Rhi.getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Direct3D 12 depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

						// Get the Direct3D 12 resource
						ID3D12Resource* d3d12Resource = texture2D->getD3D12Resource();

						// Create the Direct3D 12 render target view instance
						D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
						d3d12DescriptorHeapDesc.NumDescriptors = 1;
						d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
						if (SUCCEEDED(d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapDepthStencilView))))
						{
							D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
							d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2D->getDxgiFormat());
							d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
							d3d12RenderTargetViewDesc.Texture2D.MipSlice = depthStencilFramebufferAttachment->mipmapIndex;
							d3d12Device.CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, mD3D12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart());
						}
						break;
					}

					case Rhi::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);

						// Get the Direct3D 12 resource
						ID3D12Resource* d3d12Resource = texture2DArray->getD3D12Resource();

						// Create the Direct3D 12 render target view instance
						D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
						d3d12DescriptorHeapDesc.NumDescriptors = 1;
						d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
						if (SUCCEEDED(d3d12Device.CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapDepthStencilView))))
						{
							D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
							d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2DArray->getDxgiFormat());
							d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							d3d12RenderTargetViewDesc.Texture2DArray.MipSlice		 = depthStencilFramebufferAttachment->mipmapIndex;
							d3d12RenderTargetViewDesc.Texture2DArray.FirstArraySlice = depthStencilFramebufferAttachment->layerIndex;
							d3d12RenderTargetViewDesc.Texture2DArray.ArraySize		 = 1;
							d3d12RenderTargetViewDesc.Texture2DArray.PlaneSlice		 = 0;
							d3d12Device.CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, mD3D12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart());
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
						RHI_ASSERT(direct3D12Rhi.getContext(), false, "The type of the given depth stencil texture is not supported by the Direct3D 12 RHI implementation")
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Invalid Direct3D 12 framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Invalid Direct3D 12 framebuffer height")
				mHeight = 1;
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RHI_DEBUG
				RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "FBO", 6)	// 6 = "FBO: " including terminating zero
				const size_t detailedDebugNameLength = strlen(detailedDebugName);

				{ // Assign a debug name to the Direct3D 12 render target view, do also add the index to the name
					const size_t adjustedDetailedDebugNameLength = detailedDebugNameLength + 5;	// Direct3D 12 supports 8 render targets ("D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT", so: One digit + one [ + one ] + one space + terminating zero = 5 characters)
					const Rhi::Context& context = direct3D12Rhi.getContext();
					char* nameWithIndex = RHI_MALLOC_TYPED(context, char, detailedDebugNameLength);
					ID3D12DescriptorHeap** d3d12DescriptorHeapRenderTargetViewsEnd = mD3D12DescriptorHeapRenderTargetViews + mNumberOfColorTextures;
					for (ID3D12DescriptorHeap** d3d12DescriptorHeapRenderTargetView = mD3D12DescriptorHeapRenderTargetViews; d3d12DescriptorHeapRenderTargetView < d3d12DescriptorHeapRenderTargetViewsEnd; ++d3d12DescriptorHeapRenderTargetView)
					{
						sprintf_s(nameWithIndex, detailedDebugNameLength, "%s [%u]", detailedDebugName, static_cast<uint32_t>(d3d12DescriptorHeapRenderTargetView - mD3D12DescriptorHeapRenderTargetViews));
						FAILED_DEBUG_BREAK((*d3d12DescriptorHeapRenderTargetView)->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(adjustedDetailedDebugNameLength), nameWithIndex))
					}
					RHI_FREE(context, nameWithIndex);
				}

				// Assign a debug name to the Direct3D 12 depth stencil view
				if (nullptr != mD3D12DescriptorHeapDepthStencilView)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(detailedDebugNameLength), detailedDebugName))
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
			if (nullptr != mD3D12DescriptorHeapRenderTargetViews)
			{
				// Release references
				ID3D12DescriptorHeap** d3d12DescriptorHeapRenderTargetViewsEnd = mD3D12DescriptorHeapRenderTargetViews + mNumberOfColorTextures;
				for (ID3D12DescriptorHeap** d3d12DescriptorHeapRenderTargetView = mD3D12DescriptorHeapRenderTargetViews; d3d12DescriptorHeapRenderTargetView < d3d12DescriptorHeapRenderTargetViewsEnd; ++d3d12DescriptorHeapRenderTargetView)
				{
					(*d3d12DescriptorHeapRenderTargetView)->Release();
				}

				// Cleanup
				RHI_FREE(context, mD3D12DescriptorHeapRenderTargetViews);
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
			if (nullptr != mD3D12DescriptorHeapDepthStencilView)
			{
				// Release reference
				mD3D12DescriptorHeapDepthStencilView->Release();
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
		*    The color textures, null pointer on error, do not release the returned instance unless you added an own reference to it
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
		*    The depth stencil texture, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::ITexture* getDepthStencilTexture() const
		{
			return mDepthStencilTexture;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 render target view descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 render target view descriptor heap instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap** getD3D12DescriptorHeapRenderTargetViews() const
		{
			return mD3D12DescriptorHeapRenderTargetViews;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 depth stencil view descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 depth stencil view descriptor heap instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeapDepthStencilView() const
		{
			return mD3D12DescriptorHeapDepthStencilView;
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
		// Direct3D 12 part
		ID3D12DescriptorHeap** mD3D12DescriptorHeapRenderTargetViews;	///< The Direct3D 12 render target view descriptor heap instance, null pointer on error
		ID3D12DescriptorHeap*  mD3D12DescriptorHeapDepthStencilView;	///< The Direct3D 12 depth stencil view descriptor heap instance, null pointer on error


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/VertexShaderHlsl.h               ]
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobVertexShader(nullptr)
		{
			// Backup the vertex shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobVertexShader))
			memcpy(mD3DBlobVertexShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobVertexShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the vertex shader
			mD3DBlobVertexShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "vs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobVertexShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobVertexShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobVertexShader)
			{
				mD3DBlobVertexShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 vertex shader blob
		*
		*  @return
		*    Direct3D 12 vertex shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobVertexShader() const
		{
			return mD3DBlobVertexShader;
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
		ID3DBlob* mD3DBlobVertexShader;	///< Direct3D 12 vertex shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/TessellationControlShaderHlsl.h  ]
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
		*    Constructor for creating a tessellation control shader from shader bytecode
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationControlShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITessellationControlShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobHullShader(nullptr)
		{
			// Backup the hull shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobHullShader))
			memcpy(mD3DBlobHullShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationControlShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITessellationControlShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobHullShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the hull shader
			mD3DBlobHullShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "hs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobHullShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobHullShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationControlShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobHullShader)
			{
				mD3DBlobHullShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 hull shader blob
		*
		*  @return
		*    Direct3D 12 hull shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobHullShader() const
		{
			return mD3DBlobHullShader;
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
		ID3DBlob* mD3DBlobHullShader;	///< Direct3D 12 hull shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/TessellationEvaluationShaderHlsl.h ]
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
		*    Constructor for creating a tessellation evaluation shader from shader bytecode
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationEvaluationShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITessellationEvaluationShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobDomainShader(nullptr)
		{
			// Backup the domain shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobDomainShader))
			memcpy(mD3DBlobDomainShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationEvaluationShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITessellationEvaluationShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobDomainShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the domain shader
			mD3DBlobDomainShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "ds_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobDomainShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobDomainShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TessellationEvaluationShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobDomainShader)
			{
				mD3DBlobDomainShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 domain shader blob
		*
		*  @return
		*    Direct3D 12 domain shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobDomainShader() const
		{
			return mD3DBlobDomainShader;
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
		ID3DBlob* mD3DBlobDomainShader;	///< Direct3D 12 domain shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/GeometryShaderHlsl.h             ]
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		GeometryShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGeometryShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobGeometryShader(nullptr)
		{
			// Backup the geometry shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobGeometryShader))
			memcpy(mD3DBlobGeometryShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		GeometryShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGeometryShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobGeometryShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the geometry shader
			mD3DBlobGeometryShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "gs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobGeometryShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobGeometryShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GeometryShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobGeometryShader)
			{
				mD3DBlobGeometryShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 geometry shader blob
		*
		*  @return
		*    Direct3D 12 geometry shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobGeometryShader() const
		{
			return mD3DBlobGeometryShader;
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
		ID3DBlob* mD3DBlobGeometryShader;	///< Direct3D 12 geometry shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/FragmentShaderHlsl.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL fragment shader class (FS, "pixel shader" in Direct3D terminology)
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IFragmentShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobFragmentShader(nullptr)
		{
			// Backup the fragment shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobFragmentShader))
			memcpy(mD3DBlobFragmentShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IFragmentShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobFragmentShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the fragment shader
			mD3DBlobFragmentShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "ps_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobFragmentShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobFragmentShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~FragmentShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobFragmentShader)
			{
				mD3DBlobFragmentShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 fragment shader blob
		*
		*  @return
		*    Direct3D 12 fragment shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobFragmentShader() const
		{
			return mD3DBlobFragmentShader;
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
		ID3DBlob* mD3DBlobFragmentShader;	///< Direct3D 12 mesh shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/TaskShaderHlsl.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL task shader class (TS, "amplification shader" in Direct3D terminology)
	*/
	class TaskShaderHlsl final : public Rhi::ITaskShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a task shader from shader bytecode
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TaskShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITaskShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobTaskShader(nullptr)
		{
			// Backup the task shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobTaskShader))
			memcpy(mD3DBlobTaskShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a task shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TaskShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITaskShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobTaskShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the task shader
			mD3DBlobTaskShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "ps_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobTaskShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobTaskShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TaskShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobTaskShader)
			{
				mD3DBlobTaskShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 task shader blob
		*
		*  @return
		*    Direct3D 12 task shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobTaskShader() const
		{
			return mD3DBlobTaskShader;
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
			RHI_DELETE(getRhi().getContext(), TaskShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TaskShaderHlsl(const TaskShaderHlsl& source) = delete;
		TaskShaderHlsl& operator =(const TaskShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3DBlob* mD3DBlobTaskShader;	///< Direct3D 12 task shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/MeshShaderHlsl.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL mesh shader class (MS)
	*/
	class MeshShaderHlsl final : public Rhi::IMeshShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a mesh shader from shader bytecode
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		MeshShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IMeshShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobMeshShader(nullptr)
		{
			// Backup the mesh shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobMeshShader))
			memcpy(mD3DBlobMeshShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a mesh shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		MeshShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IMeshShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobMeshShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the mesh shader
			mD3DBlobMeshShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "ps_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobMeshShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobMeshShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~MeshShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobMeshShader)
			{
				mD3DBlobMeshShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 mesh shader blob
		*
		*  @return
		*    Direct3D 12 mesh shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobMeshShader() const
		{
			return mD3DBlobMeshShader;
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
			RHI_DELETE(getRhi().getContext(), MeshShaderHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MeshShaderHlsl(const MeshShaderHlsl& source) = delete;
		MeshShaderHlsl& operator =(const MeshShaderHlsl& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3DBlob* mD3DBlobMeshShader;	///< Direct3D 12 mesh shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/ComputeShaderHlsl.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL compute shader class (CS)
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		ComputeShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IComputeShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobComputeShader(nullptr)
		{
			// Backup the compute shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobComputeShader))
			memcpy(mD3DBlobComputeShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		ComputeShaderHlsl(Direct3D12Rhi& direct3D12Rhi, const char* sourceCode, Rhi::IShaderLanguage::OptimizationLevel optimizationLevel, Rhi::ShaderBytecode* shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IComputeShader(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3DBlobComputeShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the compute shader
			mD3DBlobComputeShader = loadShaderFromSourcecode(direct3D12Rhi.getContext(), "cs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobComputeShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobComputeShader->GetBufferPointer()));
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputeShaderHlsl() override
		{
			// Release the Direct3D 12 shader binary large object
			if (nullptr != mD3DBlobComputeShader)
			{
				mD3DBlobComputeShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 12 compute shader blob
		*
		*  @return
		*    Direct3D 12 compute shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobComputeShader() const
		{
			return mD3DBlobComputeShader;
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
		ID3DBlob* mD3DBlobComputeShader;	///< Direct3D 12 compute shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/GraphicsProgramHlsl.h            ]
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
		*    Constructor for traditional graphics program
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
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
		GraphicsProgramHlsl(Direct3D12Rhi& direct3D12Rhi, VertexShaderHlsl* vertexShaderHlsl, TessellationControlShaderHlsl* tessellationControlShaderHlsl, TessellationEvaluationShaderHlsl* tessellationEvaluationShaderHlsl, GeometryShaderHlsl* geometryShaderHlsl, FragmentShaderHlsl* fragmentShaderHlsl RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsProgram(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
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
		*    Constructor for task and mesh shader based graphics program
		*
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] taskShaderHlsl
		*    Task shader the graphics program is using, can be a null pointer
		*  @param[in] meshShaderHlsl
		*    Mesh shader the graphics program is using
		*  @param[in] fragmentShaderHlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramHlsl(Direct3D12Rhi& direct3D12Rhi, TaskShaderHlsl* taskShaderHlsl, MeshShaderHlsl& meshShaderHlsl, FragmentShaderHlsl* fragmentShaderHlsl RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsProgram(direct3D12Rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mVertexShaderHlsl(nullptr),
			mTessellationControlShaderHlsl(nullptr),
			mTessellationEvaluationShaderHlsl(nullptr),
			mGeometryShaderHlsl(nullptr),
			mFragmentShaderHlsl(fragmentShaderHlsl),
			mTaskShaderHlsl(taskShaderHlsl),
			mMeshShaderHlsl(&meshShaderHlsl)
		{
			// Add references to the provided shaders
			if (nullptr != mFragmentShaderHlsl)
			{
				mFragmentShaderHlsl->addReference();
			}
			if (nullptr != mTaskShaderHlsl)
			{
				mTaskShaderHlsl->addReference();
			}
			mMeshShaderHlsl->addReference();
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
			if (nullptr != mTaskShaderHlsl)
			{
				mTaskShaderHlsl->releaseReference();
			}
			if (nullptr != mMeshShaderHlsl)
			{
				mMeshShaderHlsl->releaseReference();
			}
		}


		//[-------------------------------------------------------]
		//[ Traditional graphics program                          ]
		//[-------------------------------------------------------]
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

		//[-------------------------------------------------------]
		//[ Both graphics programs                                ]
		//[-------------------------------------------------------]
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
		//[ Task and mesh shader based graphics program           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the HLSL task shader the graphics program is using
		*
		*  @return
		*    The HLSL task shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline TaskShaderHlsl* getTaskShaderHlsl() const
		{
			return mTaskShaderHlsl;
		}

		/**
		*  @brief
		*    Return the HLSL mesh shader the graphics program is using
		*
		*  @return
		*    The HLSL mesh shader the graphics program is using, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline MeshShaderHlsl* getMeshShaderHlsl() const
		{
			return mMeshShaderHlsl;
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
		// Traditional graphics program
		VertexShaderHlsl*				  mVertexShaderHlsl;					///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderHlsl*	  mTessellationControlShaderHlsl;		///< Tessellation control shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderHlsl* mTessellationEvaluationShaderHlsl;	///< Tessellation evaluation shader the graphics program is using (we keep a reference to it), can be a null pointer
		GeometryShaderHlsl*				  mGeometryShaderHlsl;					///< Geometry shader the graphics program is using (we keep a reference to it), can be a null pointer
		// Both graphics programs
		FragmentShaderHlsl* mFragmentShaderHlsl;	///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer
		// Task and mesh shader based graphics program
		TaskShaderHlsl* mTaskShaderHlsl;	///< Task shader the graphics program is using (we keep a reference to it), can be a null pointer
		MeshShaderHlsl* mMeshShaderHlsl;	///< Mesh shader the graphics program is using (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/Shader/ShaderLanguageHlsl.h             ]
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*/
		inline explicit ShaderLanguageHlsl(Direct3D12Rhi& direct3D12Rhi) :
			IShaderLanguage(direct3D12Rhi)
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
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 vertex shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			return RHI_NEW(direct3D12Rhi.getContext(), VertexShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), VertexShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "hull shader" in Direct3D terminology
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 tessellation control shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			return RHI_NEW(direct3D12Rhi.getContext(), TessellationControlShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "hull shader" in Direct3D terminology

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), TessellationControlShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "domain shader" in Direct3D terminology
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 tessellation evaluation shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			return RHI_NEW(direct3D12Rhi.getContext(), TessellationEvaluationShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// "domain shader" in Direct3D terminology

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), TessellationEvaluationShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 geometry shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			return RHI_NEW(direct3D12Rhi.getContext(), GeometryShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), GeometryShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 fragment shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			return RHI_NEW(direct3D12Rhi.getContext(), FragmentShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), FragmentShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), direct3D12Rhi.getCapabilities().meshShader, "Direct3D 12 task shader support is unavailable, DirectX 12 Ultimate needed")
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 task shader bytecode is invalid")

			// Create the task shader
			return RHI_NEW(direct3D12Rhi.getContext(), TaskShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), direct3D12Rhi.getCapabilities().meshShader, "Direct3D 12 task shader support is unavailable, DirectX 12 Ultimate needed")

			// Create the task shader
			return RHI_NEW(direct3D12Rhi.getContext(), TaskShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), direct3D12Rhi.getCapabilities().meshShader, "Direct3D 12 mesh shader support is unavailable, DirectX 12 Ultimate needed")
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 mesh shader bytecode is invalid")

			// Create the task shader
			return RHI_NEW(direct3D12Rhi.getContext(), MeshShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(direct3D12Rhi.getContext(), direct3D12Rhi.getCapabilities().meshShader, "Direct3D 12 mesh shader support is unavailable, DirectX 12 Ultimate needed")

			// Create the task shader
			return RHI_NEW(direct3D12Rhi.getContext(), MeshShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromBytecode(const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(direct3D12Rhi.getContext(), shaderBytecode.getNumberOfBytes() > 0 && nullptr != shaderBytecode.getBytecode(), "Direct3D 12 compute shader bytecode is invalid")

			// There's no need to check for "Rhi::Capabilities::computeShader", we know there's compute shader support
			return RHI_NEW(direct3D12Rhi.getContext(), ComputeShaderHlsl)(direct3D12Rhi, shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromSourceCode(const Rhi::ShaderSourceCode& shaderSourceCode, Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::computeShader", we know there's compute shader support
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			return RHI_NEW(direct3D12Rhi.getContext(), ComputeShaderHlsl)(direct3D12Rhi, shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Rhi::IRootSignature& rootSignature, [[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, Rhi::ITessellationControlShader* tessellationControlShader, Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 vertex shader language mismatch")
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 tessellation control shader language mismatch")
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 tessellation evaluation shader language mismatch")
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 geometry shader language mismatch")
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(direct3D12Rhi.getContext(), GraphicsProgramHlsl)(direct3D12Rhi, static_cast<VertexShaderHlsl*>(vertexShader), static_cast<TessellationControlShaderHlsl*>(tessellationControlShader), static_cast<TessellationEvaluationShaderHlsl*>(tessellationEvaluationShader), static_cast<GeometryShaderHlsl*>(geometryShader), static_cast<FragmentShaderHlsl*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Rhi::IRootSignature& rootSignature, Rhi::ITaskShader* taskShader, Rhi::IMeshShader& meshShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER)
		{
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == taskShader || taskShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 task shader language mismatch")
			RHI_ASSERT(direct3D12Rhi.getContext(), meshShader.getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 mesh shader language mismatch")
			RHI_ASSERT(direct3D12Rhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(direct3D12Rhi.getContext(), GraphicsProgramHlsl)(direct3D12Rhi, static_cast<TaskShaderHlsl*>(taskShader), static_cast<MeshShaderHlsl&>(meshShader), static_cast<FragmentShaderHlsl*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
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
	//[ Direct3D12Rhi/State/GraphicsPipelineState.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 graphics pipeline state class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(Direct3D12Rhi& direct3D12Rhi, const Rhi::GraphicsPipelineState& graphicsPipelineState, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IGraphicsPipelineState(direct3D12Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D12PrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(graphicsPipelineState.primitiveTopology)),
			mD3D12GraphicsPipelineState(nullptr),
			mRootSignature(graphicsPipelineState.rootSignature),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass)
		{
			// Add a reference to the referenced RHI resources
			mRootSignature->addReference();
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			// Define the vertex input layout
			// -> No dynamic allocations/deallocations in here, a fixed maximum number of supported attributes must be sufficient
			static constexpr uint32_t MAXIMUM_NUMBER_OF_ATTRIBUTES = 16;	// 16 elements per vertex are already pretty many
			const uint32_t numberOfVertexAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
			RHI_ASSERT(direct3D12Rhi.getContext(), numberOfVertexAttributes < MAXIMUM_NUMBER_OF_ATTRIBUTES, "Too many vertex attributes (%u) provided. The limit of the Direct3D 12 RHI implementation is %u.", numberOfVertexAttributes, MAXIMUM_NUMBER_OF_ATTRIBUTES)
			D3D12_INPUT_ELEMENT_DESC d3d12InputElementDescs[MAXIMUM_NUMBER_OF_ATTRIBUTES];
			for (uint32_t vertexAttribute = 0; vertexAttribute < numberOfVertexAttributes; ++vertexAttribute)
			{
				const Rhi::VertexAttribute& currentVertexAttribute = graphicsPipelineState.vertexAttributes.attributes[vertexAttribute];
				D3D12_INPUT_ELEMENT_DESC& d3d12InputElementDesc = d3d12InputElementDescs[vertexAttribute];

				d3d12InputElementDesc.SemanticName		= currentVertexAttribute.semanticName;
				d3d12InputElementDesc.SemanticIndex		= currentVertexAttribute.semanticIndex;
				d3d12InputElementDesc.Format			= Mapping::getDirect3D12Format(currentVertexAttribute.vertexAttributeFormat);
				d3d12InputElementDesc.InputSlot			= currentVertexAttribute.inputSlot;
				d3d12InputElementDesc.AlignedByteOffset	= currentVertexAttribute.alignedByteOffset;

				// Per-instance instead of per-vertex?
				if (currentVertexAttribute.instancesPerElement > 0)
				{
					d3d12InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;	// Input classification (D3D12_INPUT_CLASSIFICATION)
					d3d12InputElementDesc.InstanceDataStepRate = currentVertexAttribute.instancesPerElement;	// Instance data step rate (UINT)
				}
				else
				{
					d3d12InputElementDesc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// Input classification (D3D12_INPUT_CLASSIFICATION)
					d3d12InputElementDesc.InstanceDataStepRate = 0;												// Instance data step rate (UINT)
				}
			}

			// Describe and create the graphics pipeline state object (PSO)
			D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12GraphicsPipelineState = {};
			d3d12GraphicsPipelineState.InputLayout = { d3d12InputElementDescs, numberOfVertexAttributes };
			d3d12GraphicsPipelineState.pRootSignature = static_cast<RootSignature*>(mRootSignature)->getD3D12RootSignature();
			{ // Set shaders
				GraphicsProgramHlsl* graphicsProgramHlsl = static_cast<GraphicsProgramHlsl*>(mGraphicsProgram);
				const MeshShaderHlsl* meshShaderHlsl = graphicsProgramHlsl->getMeshShaderHlsl();
				if (nullptr != meshShaderHlsl)
				{
					// Task and mesh shader based graphics program

					{ // Task shader
						const TaskShaderHlsl* taskShaderHlsl = graphicsProgramHlsl->getTaskShaderHlsl();
						if (nullptr != taskShaderHlsl)
						{
							// TODO(co) "DirectX 12 Ultimate" needed
							// ID3DBlob* d3dBlobTaskShader = taskShaderHlsl->getD3DBlobTaskShader();
							// d3d12GraphicsPipelineState.AS = { reinterpret_cast<UINT8*>(d3dBlobTaskShader->GetBufferPointer()), d3dBlobTaskShader->GetBufferSize() };
						}
					}
					{ // Mesh shader
						// TODO(co) "DirectX 12 Ultimate" needed
						// ID3DBlob* d3dBlobMeshShader = meshShaderHlsl->getD3DBlobMeshShader();
						// d3d12GraphicsPipelineState.MS = { reinterpret_cast<UINT8*>(d3dBlobMeshShader->GetBufferPointer()), d3dBlobMeshShader->GetBufferSize() };
					}
					{ // Fragment shader (FS, "pixel shader" in Direct3D terminology)
						const FragmentShaderHlsl* fragmentShaderHlsl = graphicsProgramHlsl->getFragmentShaderHlsl();
						if (nullptr != fragmentShaderHlsl)
						{
							ID3DBlob* d3dBlobFragmentShader = graphicsProgramHlsl->getFragmentShaderHlsl()->getD3DBlobFragmentShader();
							d3d12GraphicsPipelineState.PS = { reinterpret_cast<UINT8*>(d3dBlobFragmentShader->GetBufferPointer()), d3dBlobFragmentShader->GetBufferSize() };
						}
					}
				}
				else
				{
					// Traditional graphics program

					{ // Vertex shader
						const VertexShaderHlsl* vertexShaderHlsl = graphicsProgramHlsl->getVertexShaderHlsl();
						if (nullptr != vertexShaderHlsl)
						{
							ID3DBlob* d3dBlobVertexShader = vertexShaderHlsl->getD3DBlobVertexShader();
							d3d12GraphicsPipelineState.VS = { reinterpret_cast<UINT8*>(d3dBlobVertexShader->GetBufferPointer()), d3dBlobVertexShader->GetBufferSize() };
						}
					}
					{ // Tessellation control shader (TCS, "hull shader" in Direct3D terminology)
						const TessellationControlShaderHlsl* tessellationControlShaderHlsl = graphicsProgramHlsl->getTessellationControlShaderHlsl();
						if (nullptr != tessellationControlShaderHlsl)
						{
							ID3DBlob* d3dBlobHullShader = tessellationControlShaderHlsl->getD3DBlobHullShader();
							d3d12GraphicsPipelineState.HS = { reinterpret_cast<UINT8*>(d3dBlobHullShader->GetBufferPointer()), d3dBlobHullShader->GetBufferSize() };
						}
					}
					{ // Tessellation evaluation shader (TES, "domain shader" in Direct3D terminology)
						const TessellationEvaluationShaderHlsl* tessellationEvaluationShaderHlsl = graphicsProgramHlsl->getTessellationEvaluationShaderHlsl();
						if (nullptr != tessellationEvaluationShaderHlsl)
						{
							ID3DBlob* d3dBlobDomainShader = tessellationEvaluationShaderHlsl->getD3DBlobDomainShader();
							d3d12GraphicsPipelineState.DS = { reinterpret_cast<UINT8*>(d3dBlobDomainShader->GetBufferPointer()), d3dBlobDomainShader->GetBufferSize() };
						}
					}
					{ // Geometry shader
						const GeometryShaderHlsl* geometryShaderHlsl = graphicsProgramHlsl->getGeometryShaderHlsl();
						if (nullptr != geometryShaderHlsl)
						{
							ID3DBlob* d3dBlobGeometryShader = geometryShaderHlsl->getD3DBlobGeometryShader();
							d3d12GraphicsPipelineState.GS = { reinterpret_cast<UINT8*>(d3dBlobGeometryShader->GetBufferPointer()), d3dBlobGeometryShader->GetBufferSize() };
						}
					}
					{ // Fragment shader (FS, "pixel shader" in Direct3D terminology)
						const FragmentShaderHlsl* fragmentShaderHlsl = graphicsProgramHlsl->getFragmentShaderHlsl();
						if (nullptr != fragmentShaderHlsl)
						{
							ID3DBlob* d3dBlobFragmentShader = graphicsProgramHlsl->getFragmentShaderHlsl()->getD3DBlobFragmentShader();
							d3d12GraphicsPipelineState.PS = { reinterpret_cast<UINT8*>(d3dBlobFragmentShader->GetBufferPointer()), d3dBlobFragmentShader->GetBufferSize() };
						}
					}
				}
			}
			d3d12GraphicsPipelineState.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(graphicsPipelineState.primitiveTopologyType);
			memcpy(&d3d12GraphicsPipelineState.RasterizerState, &graphicsPipelineState.rasterizerState, sizeof(D3D12_RASTERIZER_DESC));
			memcpy(&d3d12GraphicsPipelineState.DepthStencilState, &graphicsPipelineState.depthStencilState, sizeof(D3D12_DEPTH_STENCIL_DESC));
			{
				// TODO(co) "D3D12_RENDER_TARGET_BLEND_DESC" and "D3D11_RENDER_TARGET_BLEND_DESC" are different, we probably want to switch to "D3D12_RENDER_TARGET_BLEND_DESC"
				/*
					typedef struct D3D12_RENDER_TARGET_BLEND_DESC
					{
						BOOL BlendEnable;
						BOOL LogicOpEnable;
						D3D12_BLEND SrcBlend;
						D3D12_BLEND DestBlend;
						D3D12_BLEND_OP BlendOp;
						D3D12_BLEND SrcBlendAlpha;
						D3D12_BLEND DestBlendAlpha;
						D3D12_BLEND_OP BlendOpAlpha;
						D3D12_LOGIC_OP LogicOp;
						UINT8 RenderTargetWriteMask;
					} D3D12_RENDER_TARGET_BLEND_DESC;

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
				*/
				D3D12_BLEND_DESC& d3d12BlendDesc = d3d12GraphicsPipelineState.BlendState;
				const Rhi::BlendState& blendState = graphicsPipelineState.blendState;
				d3d12BlendDesc.AlphaToCoverageEnable = blendState.alphaToCoverageEnable;
				d3d12BlendDesc.IndependentBlendEnable = blendState.independentBlendEnable;
				for (uint32_t renderTargetIndex = 0; renderTargetIndex < 8; ++renderTargetIndex)
				{
					D3D12_RENDER_TARGET_BLEND_DESC& d3d12RenderTargetBlendDesc = d3d12BlendDesc.RenderTarget[renderTargetIndex];
					const Rhi::RenderTargetBlendDesc& renderTargetBlendDesc = blendState.renderTarget[renderTargetIndex];
					d3d12RenderTargetBlendDesc.BlendEnable			 = renderTargetBlendDesc.blendEnable;
					d3d12RenderTargetBlendDesc.LogicOpEnable		 = false;
					d3d12RenderTargetBlendDesc.SrcBlend				 = static_cast<D3D12_BLEND>(renderTargetBlendDesc.srcBlend);
					d3d12RenderTargetBlendDesc.DestBlend			 = static_cast<D3D12_BLEND>(renderTargetBlendDesc.destBlend);
					d3d12RenderTargetBlendDesc.BlendOp				 = static_cast<D3D12_BLEND_OP>(renderTargetBlendDesc.blendOp);
					d3d12RenderTargetBlendDesc.SrcBlendAlpha		 = static_cast<D3D12_BLEND>(renderTargetBlendDesc.srcBlendAlpha);
					d3d12RenderTargetBlendDesc.DestBlendAlpha		 = static_cast<D3D12_BLEND>(renderTargetBlendDesc.destBlendAlpha);
					d3d12RenderTargetBlendDesc.BlendOpAlpha			 = static_cast<D3D12_BLEND_OP>(renderTargetBlendDesc.blendOpAlpha);
					d3d12RenderTargetBlendDesc.LogicOp				 = D3D12_LOGIC_OP_CLEAR;
					d3d12RenderTargetBlendDesc.RenderTargetWriteMask = renderTargetBlendDesc.renderTargetWriteMask;
				}
			}
			d3d12GraphicsPipelineState.SampleMask = UINT_MAX;
			d3d12GraphicsPipelineState.NumRenderTargets = graphicsPipelineState.numberOfRenderTargets;
			for (uint32_t i = 0; i < graphicsPipelineState.numberOfRenderTargets; ++i)
			{
				d3d12GraphicsPipelineState.RTVFormats[i] = Mapping::getDirect3D12Format(graphicsPipelineState.renderTargetViewFormats[i]);
			}
			d3d12GraphicsPipelineState.DSVFormat = Mapping::getDirect3D12Format(graphicsPipelineState.depthStencilViewFormat);
			d3d12GraphicsPipelineState.SampleDesc.Count = 1;
			if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateGraphicsPipelineState(&d3d12GraphicsPipelineState, IID_PPV_ARGS(&mD3D12GraphicsPipelineState))))
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Graphics PSO", 15)	// 15 = "Graphics PSO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12GraphicsPipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create the Direct3D 12 graphics pipeline state object")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsPipelineState() override
		{
			// Release the Direct3D 12 graphics pipeline state
			if (nullptr != mD3D12GraphicsPipelineState)
			{
				mD3D12GraphicsPipelineState->Release();
			}

			// Release referenced RHI resources
			mRootSignature->releaseReference();
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Free the unique compact graphics pipeline state ID
			static_cast<Direct3D12Rhi&>(getRhi()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the Direct3D 12 primitive topology
		*
		*  @return
		*    The Direct3D 12 primitive topology
		*/
		[[nodiscard]] inline D3D12_PRIMITIVE_TOPOLOGY getD3D12PrimitiveTopology() const
		{
			return mD3D12PrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 graphics pipeline state
		*
		*  @return
		*    The Direct3D 12 graphics pipeline state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12PipelineState* getD3D12GraphicsPipelineState() const
		{
			return mD3D12GraphicsPipelineState;
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
		D3D12_PRIMITIVE_TOPOLOGY mD3D12PrimitiveTopology;
		ID3D12PipelineState*	 mD3D12GraphicsPipelineState;	///< Direct3D 12 graphics pipeline state, can be a null pointer
		Rhi::IRootSignature*	 mRootSignature;
		Rhi::IGraphicsProgram*	 mGraphicsProgram;
		Rhi::IRenderPass*		 mRenderPass;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/State/ComputePipelineState.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 compute pipeline state class
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
		*  @param[in] direct3D12Rhi
		*    Owner Direct3D 12 RHI instance
		*  @param[in] rootSignature
		*    Root signature shader to use
		*  @param[in] computeShader
		*    Compute shader to use
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		ComputePipelineState(Direct3D12Rhi& direct3D12Rhi, Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER) :
			IComputePipelineState(direct3D12Rhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mD3D12ComputePipelineState(nullptr),
			mRootSignature(rootSignature),
			mComputeShader(computeShader)
		{
			// Add a reference to the given root signature and compute shader
			rootSignature.addReference();
			computeShader.addReference();

			// Describe and create the compute pipeline state object (PSO)
			D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12ComputePipelineState = {};
			d3d12ComputePipelineState.pRootSignature = static_cast<RootSignature&>(rootSignature).getD3D12RootSignature();
			{ // Set compute shader
				ID3DBlob* d3dBlobComputeShader = static_cast<ComputeShaderHlsl&>(computeShader).getD3DBlobComputeShader();
				d3d12ComputePipelineState.CS = { reinterpret_cast<UINT8*>(d3dBlobComputeShader->GetBufferPointer()), d3dBlobComputeShader->GetBufferSize() };
			}
			if (SUCCEEDED(direct3D12Rhi.getD3D12Device().CreateComputePipelineState(&d3d12ComputePipelineState, IID_PPV_ARGS(&mD3D12ComputePipelineState))))
			{
				// Assign a default name to the resource for debugging purposes
				#ifdef RHI_DEBUG
					RHI_DECORATED_DEBUG_NAME(debugName, detailedDebugName, "Compute PSO", 14)	// 14 = "Compute PSO: " including terminating zero
					FAILED_DEBUG_BREAK(mD3D12ComputePipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedDebugName)), detailedDebugName))
				#endif
			}
			else
			{
				RHI_ASSERT(direct3D12Rhi.getContext(), false, "Failed to create the Direct3D 12 compute pipeline state object")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputePipelineState() override
		{
			// Release the Direct3D 12 compute pipeline state
			if (nullptr != mD3D12ComputePipelineState)
			{
				mD3D12ComputePipelineState->Release();
			}

			// Release the root signature and compute shader reference
			mRootSignature.releaseReference();
			mComputeShader.releaseReference();

			// Free the unique compact compute pipeline state ID
			static_cast<Direct3D12Rhi&>(getRhi()).ComputePipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the Direct3D 12 compute pipeline state
		*
		*  @return
		*    The Direct3D 12 compute pipeline state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12PipelineState* getD3D12ComputePipelineState() const
		{
			return mD3D12ComputePipelineState;
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
		ID3D12PipelineState* mD3D12ComputePipelineState;	///< Direct3D 12 compute pipeline state, can be a null pointer
		Rhi::IRootSignature& mRootSignature;
		Rhi::IComputeShader& mComputeShader;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Rhi/ResourceGroup.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 resource group class
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
		*  @param[in] d3d12DescriptorHeapType
		*    Direct3D 12 descriptor heap type
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(RootSignature& rootSignature, D3D12_DESCRIPTOR_HEAP_TYPE d3d12DescriptorHeapType, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IResourceGroup(rootSignature.getRhi() RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature),
			mD3D12DescriptorHeapType(d3d12DescriptorHeapType),
			mNumberOfResources(numberOfResources),
			mResources(RHI_MALLOC_TYPED(rootSignature.getRhi().getContext(), Rhi::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr),
			mDescriptorHeapOffset(0),
			mDescriptorHeapSize(static_cast<uint16_t>(numberOfResources))
		{
			mRootSignature.addReference();

			// Sanity check
			const Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			RHI_ASSERT(direct3D12Rhi.getContext(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == mD3D12DescriptorHeapType || D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == mD3D12DescriptorHeapType, "Invalid Direct3D 12 descriptor heap type, must be \"D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV\" or \"D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER\"")

			// Process all resources and add our reference to the RHI resource
			if (nullptr != samplerStates)
			{
				mSamplerStates = RHI_MALLOC_TYPED(direct3D12Rhi.getContext(), Rhi::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Rhi::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->addReference();
					}
				}
			}
			ID3D12Device& d3d12Device = direct3D12Rhi.getD3D12Device();
			if (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == d3d12DescriptorHeapType)
			{
				::detail::DescriptorHeap& descriptorHeap = direct3D12Rhi.getShaderResourceViewDescriptorHeap();
				mDescriptorHeapOffset = descriptorHeap.allocate(static_cast<uint16_t>(numberOfResources));
				const uint32_t descriptorSize = descriptorHeap.getDescriptorSize();
				D3D12_CPU_DESCRIPTOR_HANDLE d3d12CpuDescriptorHandle = descriptorHeap.getD3D12CpuDescriptorHandleForHeapStart();
				d3d12CpuDescriptorHandle.ptr += mDescriptorHeapOffset * descriptorSize;
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
				{
					Rhi::IResource* resource = *resources;
					RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != resource, "Invalid Direct3D 12 resource")
					mResources[resourceIndex] = resource;
					resource->addReference();

					// Check the type of resource to set
					// TODO(co) Some additional resource type root signature security checks in debug build?
					const Rhi::ResourceType resourceType = resource->getResourceType();
					switch (resourceType)
					{
						case Rhi::ResourceType::INDEX_BUFFER:
						{
							// TODO(co)
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "TODO(co) Implement me")
							/*
							const IndexBuffer* indexBuffer = static_cast<IndexBuffer*>(resource);
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != indexBuffer->getD3D12Resource(), "Invalid Direct3D 12 index buffer resource")
							const D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc = { indexBuffer->getD3D12Resource()->GetGPUVirtualAddress(), indexBuffer->getNumberOfBytesOnGpu() };
							d3d12Device.CreateConstantBufferView(&d3d12ConstantBufferViewDesc, d3d12CpuDescriptorHandle);
							*/
							break;
						}

						case Rhi::ResourceType::VERTEX_BUFFER:
						{
							// TODO(co)
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "TODO(co) Implement me")
							/*
							const VertexBuffer* vertexBuffer = static_cast<VertexBuffer*>(resource);
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != vertexBuffer->getD3D12Resource(), "Invalid Direct3D 12 vertex buffer resource")
							const D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc = { vertexBuffer->getD3D12Resource()->GetGPUVirtualAddress(), vertexBuffer->getNumberOfBytesOnGpu() };
							d3d12Device.CreateConstantBufferView(&d3d12ConstantBufferViewDesc, d3d12CpuDescriptorHandle);
							*/
							break;
						}

						case Rhi::ResourceType::TEXTURE_BUFFER:
						{
							const TextureBuffer* textureBuffer = static_cast<TextureBuffer*>(resource);
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != textureBuffer->getD3D12Resource(), "Invalid Direct3D 12 texture buffer resource")
							const Rhi::TextureFormat::Enum textureFormat = textureBuffer->getTextureFormat();
							D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
							d3d12ShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
							d3d12ShaderResourceViewDesc.Format = Mapping::getDirect3D12Format(textureFormat);
							d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
							d3d12ShaderResourceViewDesc.Buffer.FirstElement = 0;
							d3d12ShaderResourceViewDesc.Buffer.NumElements = textureBuffer->getNumberOfBytes() / Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat);
							d3d12Device.CreateShaderResourceView(textureBuffer->getD3D12Resource(), &d3d12ShaderResourceViewDesc, d3d12CpuDescriptorHandle);
							break;
						}

						case Rhi::ResourceType::STRUCTURED_BUFFER:
						{
							// TODO(co)
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "TODO(co) Implement me")
							/*
							const StructuredBuffer* structuredBuffer = static_cast<StructuredBuffer*>(resource);
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != structuredBuffer->getD3D12Resource(), "Invalid Direct3D 12 structured buffer resource")
							const D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc = { structuredBuffer->getD3D12Resource()->GetGPUVirtualAddress(), structuredBuffer->getNumberOfBytesOnGpu() };
							d3d12Device.CreateConstantBufferView(&d3d12ConstantBufferViewDesc, d3d12CpuDescriptorHandle);
							*/
							break;
						}

						case Rhi::ResourceType::INDIRECT_BUFFER:
						{
							// TODO(co)
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "TODO(co) Implement me")
							/*
							const IndirectBuffer* indirectBuffer = static_cast<IndirectBuffer*>(resource);
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != indirectBuffer->getD3D12Resource(), "Invalid Direct3D 12 indirect buffer resource")
							const D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc = { indirectBuffer->getD3D12Resource()->GetGPUVirtualAddress(), indirectBuffer->getNumberOfBytesOnGpu() };
							d3d12Device.CreateConstantBufferView(&d3d12ConstantBufferViewDesc, d3d12CpuDescriptorHandle);
							*/
							break;
						}

						case Rhi::ResourceType::UNIFORM_BUFFER:
						{
							const UniformBuffer* uniformBuffer = static_cast<UniformBuffer*>(resource);
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != uniformBuffer->getD3D12Resource(), "Invalid Direct3D 12 uniform buffer resource")
							const D3D12_CONSTANT_BUFFER_VIEW_DESC d3d12ConstantBufferViewDesc = { uniformBuffer->getD3D12Resource()->GetGPUVirtualAddress(), uniformBuffer->getNumberOfBytesOnGpu() };
							d3d12Device.CreateConstantBufferView(&d3d12ConstantBufferViewDesc, d3d12CpuDescriptorHandle);
							break;
						}

						case Rhi::ResourceType::TEXTURE_1D:
						case Rhi::ResourceType::TEXTURE_1D_ARRAY:
						case Rhi::ResourceType::TEXTURE_2D:
						case Rhi::ResourceType::TEXTURE_2D_ARRAY:
						case Rhi::ResourceType::TEXTURE_3D:
						case Rhi::ResourceType::TEXTURE_CUBE:
						case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
						{
							// Evaluate the texture type and create the Direct3D 12 shader resource view
							ID3D12Resource* d3d12Resource = nullptr;
							D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
							d3d12ShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
							switch (resourceType)
							{
								case Rhi::ResourceType::TEXTURE_1D:
								{
									const Texture1D* texture1D = static_cast<Texture1D*>(resource);
									d3d12ShaderResourceViewDesc.Format = texture1D->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
									d3d12ShaderResourceViewDesc.Texture1D.MipLevels = texture1D->getNumberOfMipmaps();
									d3d12Resource = texture1D->getD3D12Resource();
									break;
								}

								case Rhi::ResourceType::TEXTURE_1D_ARRAY:
								{
									const Texture1DArray* texture1DArray = static_cast<Texture1DArray*>(resource);
									d3d12ShaderResourceViewDesc.Format = texture1DArray->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
									d3d12ShaderResourceViewDesc.Texture1DArray.MipLevels = texture1DArray->getNumberOfMipmaps();
									d3d12ShaderResourceViewDesc.Texture1DArray.ArraySize = texture1DArray->getNumberOfSlices();
									d3d12Resource = texture1DArray->getD3D12Resource();
									break;
								}

								case Rhi::ResourceType::TEXTURE_2D:
								{
									const Texture2D* texture2D = static_cast<Texture2D*>(resource);
									d3d12ShaderResourceViewDesc.Format = texture2D->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
									d3d12ShaderResourceViewDesc.Texture2D.MipLevels = texture2D->getNumberOfMipmaps();
									d3d12Resource = texture2D->getD3D12Resource();
									break;
								}

								case Rhi::ResourceType::TEXTURE_2D_ARRAY:
								{
									const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(resource);
									d3d12ShaderResourceViewDesc.Format = texture2DArray->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
									d3d12ShaderResourceViewDesc.Texture2DArray.MipLevels = texture2DArray->getNumberOfMipmaps();
									d3d12ShaderResourceViewDesc.Texture2DArray.ArraySize = texture2DArray->getNumberOfSlices();
									d3d12Resource = texture2DArray->getD3D12Resource();
									break;
								}

								case Rhi::ResourceType::TEXTURE_3D:
								{
									const Texture3D* texture3D = static_cast<Texture3D*>(resource);
									d3d12ShaderResourceViewDesc.Format = texture3D->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
									d3d12ShaderResourceViewDesc.Texture3D.MipLevels = texture3D->getNumberOfMipmaps();
									d3d12Resource = texture3D->getD3D12Resource();
									break;
								}

								case Rhi::ResourceType::TEXTURE_CUBE:
								{
									const TextureCube* textureCube = static_cast<TextureCube*>(resource);
									d3d12ShaderResourceViewDesc.Format = textureCube->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
									d3d12ShaderResourceViewDesc.TextureCube.MipLevels = textureCube->getNumberOfMipmaps();
									d3d12Resource = textureCube->getD3D12Resource();
									break;
								}

								case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:
								{
									// TODO(co) Implement me
									/*
									const TextureCubeArray* textureCubeArray = static_cast<TextureCubeArray*>(resource);
									d3d12ShaderResourceViewDesc.Format = textureCubeArray->getDxgiFormat();
									d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
									d3d12ShaderResourceViewDesc.TextureCubeArray.MipLevels = textureCubeArray->getNumberOfMipmaps();
									d3d12Resource = textureCubeArray->getD3D12Resource();
									*/
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
								case Rhi::ResourceType::TASK_SHADER:
								case Rhi::ResourceType::MESH_SHADER:
								case Rhi::ResourceType::COMPUTE_SHADER:
									RHI_ASSERT(direct3D12Rhi.getContext(), false, "Invalid Direct3D 12 RHI implementation resource type")
									break;
							}
							RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != d3d12Resource, "Invalid Direct3D 12 resource")
							d3d12Device.CreateShaderResourceView(d3d12Resource, &d3d12ShaderResourceViewDesc, d3d12CpuDescriptorHandle);
							break;
						}

						case Rhi::ResourceType::SAMPLER_STATE:
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
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "Invalid Direct3D 12 RHI implementation resource type")
							break;
					}
					d3d12CpuDescriptorHandle.ptr += descriptorSize;
				}
				RHI_ASSERT(direct3D12Rhi.getContext(), d3d12CpuDescriptorHandle.ptr == descriptorHeap.getD3D12CpuDescriptorHandleForHeapStart().ptr + (mDescriptorHeapOffset + mNumberOfResources) * descriptorSize, "Direct3D 12 descriptor heap invalid")
			}
			else
			{
				::detail::DescriptorHeap& descriptorHeap = direct3D12Rhi.getSamplerDescriptorHeap();
				mDescriptorHeapOffset = descriptorHeap.allocate(static_cast<uint16_t>(numberOfResources));
				const uint32_t descriptorSize = descriptorHeap.getDescriptorSize();
				D3D12_CPU_DESCRIPTOR_HANDLE d3d12CpuDescriptorHandle = descriptorHeap.getD3D12CpuDescriptorHandleForHeapStart();
				d3d12CpuDescriptorHandle.ptr += mDescriptorHeapOffset * descriptorSize;
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
				{
					Rhi::IResource* resource = *resources;
					RHI_ASSERT(direct3D12Rhi.getContext(), nullptr != resource, "Invalid Direct3D 12 resource")
					mResources[resourceIndex] = resource;
					resource->addReference();

					// Check the type of resource to set
					// TODO(co) Some additional resource type root signature security checks in debug build?
					switch (resource->getResourceType())
					{
						case Rhi::ResourceType::SAMPLER_STATE:
							d3d12Device.CreateSampler(reinterpret_cast<const D3D12_SAMPLER_DESC*>(&static_cast<SamplerState*>(resource)->getSamplerState()), d3d12CpuDescriptorHandle);
							break;

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
							RHI_ASSERT(direct3D12Rhi.getContext(), false, "Invalid Direct3D 12 RHI implementation resource type")
							break;
					}
					d3d12CpuDescriptorHandle.ptr += descriptorSize;
				}
				RHI_ASSERT(direct3D12Rhi.getContext(), d3d12CpuDescriptorHandle.ptr == descriptorHeap.getD3D12CpuDescriptorHandleForHeapStart().ptr + (mDescriptorHeapOffset + mNumberOfResources) * descriptorSize, "Direct3D 12 descriptor heap invalid")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ResourceGroup() override
		{
			// Remove our reference from the RHI resources
			Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
			const Rhi::Context& context = direct3D12Rhi.getContext();
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
			mRootSignature.releaseReference();

			// Release descriptor heap
			((D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == mD3D12DescriptorHeapType) ? direct3D12Rhi.getShaderResourceViewDescriptorHeap() : direct3D12Rhi.getSamplerDescriptorHeap()).release(mDescriptorHeapOffset, mDescriptorHeapSize);
		}

		/**
		*  @brief
		*    Return the Direct3D 12 descriptor heap type
		*
		*  @return
		*    The Direct3D 12 descriptor heap type ("D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV" or "D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER")
		*/
		[[nodiscard]] inline D3D12_DESCRIPTOR_HEAP_TYPE getD3D12DescriptorHeapType() const
		{
			return mD3D12DescriptorHeapType;
		}

		/**
		*  @brief
		*    Return the descriptor heap offset
		*
		*  @return
		*    The descriptor heap offset
		*/
		[[nodiscard]] inline uint16_t getDescriptorHeapOffset() const
		{
			return mDescriptorHeapOffset;
		}

		/**
		*  @brief
		*    Return the descriptor heap size
		*
		*  @return
		*    The descriptor heap size
		*/
		[[nodiscard]] inline uint16_t getDescriptorHeapSize() const
		{
			return mDescriptorHeapSize;
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
		RootSignature&			   mRootSignature;				///< Root signature
		D3D12_DESCRIPTOR_HEAP_TYPE mD3D12DescriptorHeapType;	///< The Direct3D 12 descriptor heap type ("D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV" or "D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER")
		uint32_t				   mNumberOfResources;			///< Number of resources this resource group groups together
		Rhi::IResource**		   mResources;					///< RHI resource, we keep a reference to it
		Rhi::ISamplerState**	   mSamplerStates;				///< Sampler states, we keep a reference to it
		uint16_t				   mDescriptorHeapOffset;
		uint16_t				   mDescriptorHeapSize;


	};

	Rhi::IResourceGroup* RootSignature::createResourceGroup([[maybe_unused]] uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, [[maybe_unused]] Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		Direct3D12Rhi& direct3D12Rhi = static_cast<Direct3D12Rhi&>(getRhi());
		const Rhi::Context& context = direct3D12Rhi.getContext();
		RHI_ASSERT(context, numberOfResources > 0, "The number of Direct3D 12 resources must not be zero")
		RHI_ASSERT(context, nullptr != resources, "The Direct3D 12 resource pointers must be valid")

		// Figure out the Direct3D 12 descriptor heap type
		D3D12_DESCRIPTOR_HEAP_TYPE d3d12DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex)
		{
			Rhi::IResource* resource = *resources;
			RHI_ASSERT(context, nullptr != resource, "Invalid Direct3D 12 resource")
			const Rhi::ResourceType resourceType = resource->getResourceType();
			if (Rhi::ResourceType::SAMPLER_STATE == resourceType)
			{
				RHI_ASSERT(context, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES == d3d12DescriptorHeapType || D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == d3d12DescriptorHeapType, "Direct3D 12 resource groups can't mix samplers with other resource types")
				d3d12DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			}
			else
			{
				RHI_ASSERT(context, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES == d3d12DescriptorHeapType || D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == d3d12DescriptorHeapType, "Direct3D 12 resource groups can't mix samplers with other resource types")
				d3d12DescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			}
		}

		// Create resource group
		return RHI_NEW(context, ResourceGroup)(*this, d3d12DescriptorHeapType, numberOfResources, resources, samplerStates RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D12Rhi




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
		namespace ImplementationDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ExecuteCommandBuffer* realData = static_cast<const Rhi::Command::ExecuteCommandBuffer*>(data);
				RHI_ASSERT(rhi.getContext(), nullptr != realData->commandBufferToExecute, "The Direct3D 12 command buffer to execute must be valid")
				rhi.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsRootSignature* realData = static_cast<const Rhi::Command::SetGraphicsRootSignature*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsPipelineState* realData = static_cast<const Rhi::Command::SetGraphicsPipelineState*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsResourceGroup* realData = static_cast<const Rhi::Command::SetGraphicsResourceGroup*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Rhi::IRhi& rhi)
			{
				// Input-assembler (IA) stage
				const Rhi::Command::SetGraphicsVertexArray* realData = static_cast<const Rhi::Command::SetGraphicsVertexArray*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsViewports* realData = static_cast<const Rhi::Command::SetGraphicsViewports*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Rhi::Viewport*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsScissorRectangles* realData = static_cast<const Rhi::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Rhi::ScissorRectangle*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Rhi::IRhi& rhi)
			{
				// Output-merger (OM) stage
				const Rhi::Command::SetGraphicsRenderTarget* realData = static_cast<const Rhi::Command::SetGraphicsRenderTarget*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ClearGraphics* realData = static_cast<const Rhi::Command::ClearGraphics*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).drawGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).drawIndexedGraphics(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawMeshTasks(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawMeshTasks* realData = static_cast<const Rhi::Command::DrawMeshTasks*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).drawMeshTasks(*realData->indirectBuffer, realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).drawMeshTasksEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeRootSignature* realData = static_cast<const Rhi::Command::SetComputeRootSignature*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputePipelineState* realData = static_cast<const Rhi::Command::SetComputePipelineState*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeResourceGroup* realData = static_cast<const Rhi::Command::SetComputeResourceGroup*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCompute* realData = static_cast<const Rhi::Command::DispatchCompute*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).dispatchCompute(realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, [[maybe_unused]] Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Rhi::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				RHI_ASSERT(static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).getContext(), realData->texture->getResourceType() == Rhi::ResourceType::TEXTURE_2D, "Unsupported Direct3D 12 texture resource type")
				static_cast<Direct3D12Rhi::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
			}

			void ResolveMultisampleFramebuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Rhi::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyResource* realData = static_cast<const Rhi::Command::CopyResource*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::GenerateMipmaps* realData = static_cast<const Rhi::Command::GenerateMipmaps*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResetQueryPool* realData = static_cast<const Rhi::Command::ResetQueryPool*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::BeginQuery* realData = static_cast<const Rhi::Command::BeginQuery*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::EndQuery* realData = static_cast<const Rhi::Command::EndQuery*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::WriteTimestampQuery* realData = static_cast<const Rhi::Command::WriteTimestampQuery*>(data);
				static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}


			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RHI_DEBUG
				void SetDebugMarker(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::SetDebugMarker* realData = static_cast<const Rhi::Command::SetDebugMarker*>(data);
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::BeginDebugEvent* realData = static_cast<const Rhi::Command::BeginDebugEvent*>(data);
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Rhi::IRhi& rhi)
				{
					static_cast<Direct3D12Rhi::Direct3D12Rhi&>(rhi).endDebugEvent();
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
namespace Direct3D12Rhi
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Direct3D12Rhi::Direct3D12Rhi(const Rhi::Context& context) :
		IRhi(Rhi::NameId::DIRECT3D12, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		ComputePipelineStateMakeId(context.getAllocator()),
		mDirect3D12RuntimeLinking(nullptr),
		mDxgiFactory4(nullptr),
		mD3D12Device(nullptr),
		mD3D12CommandQueue(nullptr),
		mD3D12CommandAllocator(nullptr),
		mD3D12GraphicsCommandList(nullptr),
		mShaderLanguageHlsl(nullptr),
		mShaderResourceViewDescriptorHeap(nullptr),
		mRenderTargetViewDescriptorHeap(nullptr),
		mDepthStencilViewDescriptorHeap(nullptr),
		mSamplerDescriptorHeap(nullptr),
		mRenderTarget(nullptr),
		mD3D12PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
		mGraphicsRootSignature(nullptr),
		mComputeRootSignature(nullptr),
		mVertexArray(nullptr)
		#ifdef RHI_DEBUG
			, mDebugBetweenBeginEndScene(false)
		#endif
	{
		mDirect3D12RuntimeLinking = RHI_NEW(mContext, Direct3D12RuntimeLinking)(*this);

		// Is Direct3D 12 available?
		if (mDirect3D12RuntimeLinking->isDirect3D12Avaiable())
		{
			// Create the DXGI factory instance
			if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory4))))
			{
				// Enable the Direct3D 12 debug layer
				#ifdef RHI_DEBUG
				{
					ID3D12Debug* d3d12Debug = nullptr;
					if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
					{
						d3d12Debug->EnableDebugLayer();
						d3d12Debug->Release();
					}
				}
				#endif

				// Create the Direct3D 12 device
				// -> In case of failure, create an emulated device instance so we can at least test the DirectX 12 API
				if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mD3D12Device))))
				{
					RHI_LOG(mContext, CRITICAL, "Failed to create Direct3D 12 device instance. Creating an emulated Direct3D 11 device instance instead.")

					// Create the DXGI adapter instance
					IDXGIAdapter* dxgiAdapter = nullptr;
					if (SUCCEEDED(mDxgiFactory4->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter))))
					{
						// Create the emulated Direct3D 12 device
						if (FAILED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3D12Device))))
						{
							RHI_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 device instance")
						}

						// Release the DXGI adapter instance
						dxgiAdapter->Release();
					}
					else
					{
						RHI_LOG(mContext, CRITICAL, "Failed to create Direct3D 12 DXGI adapter instance")
					}
				}
			}
			else
			{
				RHI_LOG(mContext, CRITICAL, "Failed to create Direct3D 12 DXGI factory instance")
			}

			// Is there a valid Direct3D 12 device instance?
			if (nullptr != mD3D12Device)
			{
				// Describe and create the command queue
				D3D12_COMMAND_QUEUE_DESC d3d12CommandQueueDesc;
				d3d12CommandQueueDesc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;
				d3d12CommandQueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
				d3d12CommandQueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
				d3d12CommandQueueDesc.NodeMask	= 0;
				if (SUCCEEDED(mD3D12Device->CreateCommandQueue(&d3d12CommandQueueDesc, IID_PPV_ARGS(&mD3D12CommandQueue))))
				{
					// Create the command allocator
					if (SUCCEEDED(mD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mD3D12CommandAllocator))))
					{
						// Create the command list
						if (SUCCEEDED(mD3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mD3D12CommandAllocator, nullptr, IID_PPV_ARGS(&mD3D12GraphicsCommandList))))
						{
							// Command lists are created in the recording state, but there is nothing to record yet. The main loop expects it to be closed, so close it now.
							if (SUCCEEDED(mD3D12GraphicsCommandList->Close()))
							{
								// Initialize the capabilities
								initializeCapabilities();

								// Create and begin upload context
								mUploadContext.create(*mD3D12Device);

								// Create descriptor heaps
								// TODO(co) The initial descriptor heap sizes are probably too small, additionally the descriptor heap should be able to dynamically grow during runtime (in case it can't be avoided)
								mShaderResourceViewDescriptorHeap = RHI_NEW(mContext, ::detail::DescriptorHeap)(mContext.getAllocator(), *mD3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,	64, true);
								mRenderTargetViewDescriptorHeap   = RHI_NEW(mContext, ::detail::DescriptorHeap)(mContext.getAllocator(), *mD3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,			16, false);
								mDepthStencilViewDescriptorHeap	  = RHI_NEW(mContext, ::detail::DescriptorHeap)(mContext.getAllocator(), *mD3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,			16, false);
								mSamplerDescriptorHeap			  = RHI_NEW(mContext, ::detail::DescriptorHeap)(mContext.getAllocator(), *mD3D12Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,		16, true);
							}
							else
							{
								RHI_LOG(mContext, CRITICAL, "Failed to close the Direct3D 12 command list instance")
							}
						}
						else
						{
							RHI_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 command list instance")
						}
					}
					else
					{
						RHI_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 command allocator instance")
					}
				}
				else
				{
					RHI_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 command queue instance")
				}
			}
		}
	}

	Direct3D12Rhi::~Direct3D12Rhi()
	{
		// Set no vertex array reference, in case we have one
		setGraphicsVertexArray(nullptr);

		// Release instances
		if (nullptr != mRenderTarget)
		{
			mRenderTarget->releaseReference();
			mRenderTarget = nullptr;
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
					RHI_LOG(mContext, CRITICAL, "The Direct3D 12 RHI implementation is going to be destroyed, but there are still %u resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RHI_LOG(mContext, CRITICAL, "The Direct3D 12 RHI implementation is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the graphics and compute root signature instance, in case we have one
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
		}

		// Destroy upload context
		mUploadContext.destroy();

		// Release descriptor heaps
		RHI_DELETE(mContext, ::detail::DescriptorHeap, mShaderResourceViewDescriptorHeap);
		RHI_DELETE(mContext, ::detail::DescriptorHeap, mRenderTargetViewDescriptorHeap);
		RHI_DELETE(mContext, ::detail::DescriptorHeap, mDepthStencilViewDescriptorHeap);
		RHI_DELETE(mContext, ::detail::DescriptorHeap, mSamplerDescriptorHeap);

		// Release the HLSL shader language instance, in case we have one
		if (nullptr != mShaderLanguageHlsl)
		{
			mShaderLanguageHlsl->releaseReference();
		}

		// Release the Direct3D 12 command queue we've created
		if (nullptr != mD3D12GraphicsCommandList)
		{
			mD3D12GraphicsCommandList->Release();
			mD3D12GraphicsCommandList = nullptr;
		}
		if (nullptr != mD3D12CommandAllocator)
		{
			mD3D12CommandAllocator->Release();
			mD3D12CommandAllocator = nullptr;
		}
		if (nullptr != mD3D12CommandQueue)
		{
			mD3D12CommandQueue->Release();
			mD3D12CommandQueue = nullptr;
		}

		// Release the Direct3D 12 device we've created
		if (nullptr != mD3D12Device)
		{
			mD3D12Device->Release();
			mD3D12Device = nullptr;
		}

		// Release the DXGI factory instance
		if (nullptr != mDxgiFactory4)
		{
			mDxgiFactory4->Release();
			mDxgiFactory4 = nullptr;
		}

		// Destroy the Direct3D 12 runtime linking instance
		RHI_DELETE(mContext, Direct3D12RuntimeLinking, mDirect3D12RuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void Direct3D12Rhi::setGraphicsRootSignature(Rhi::IRootSignature* rootSignature)
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

			// Set graphics root signature
			mD3D12GraphicsCommandList->SetGraphicsRootSignature(mGraphicsRootSignature->getD3D12RootSignature());
		}
	}

	void Direct3D12Rhi::setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsPipelineState)

			// Set primitive topology
			// -> The "Rhi::PrimitiveTopology" values directly map to Direct3D 9 & 10 & 11 && 12 constants, do not change them
			const GraphicsPipelineState* direct3D12GraphicsPipelineState = static_cast<const GraphicsPipelineState*>(graphicsPipelineState);
			if (mD3D12PrimitiveTopology != direct3D12GraphicsPipelineState->getD3D12PrimitiveTopology())
			{
				mD3D12PrimitiveTopology = direct3D12GraphicsPipelineState->getD3D12PrimitiveTopology();
				mD3D12GraphicsCommandList->IASetPrimitiveTopology(mD3D12PrimitiveTopology);
			}

			// Set graphics pipeline state
			mD3D12GraphicsCommandList->SetPipelineState(direct3D12GraphicsPipelineState->getD3D12GraphicsPipelineState());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Rhi::setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mGraphicsRootSignature, "No Direct3D 12 RHI implementation graphics root signature set")
			const Rhi::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The Direct3D 12 RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The Direct3D 12 RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The Direct3D 12 RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Set Direct3D 12 graphics root descriptor table
			const ResourceGroup* direct3D12ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const ::detail::DescriptorHeap& descriptorHeap = (direct3D12ResourceGroup->getD3D12DescriptorHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? *mShaderResourceViewDescriptorHeap : *mSamplerDescriptorHeap;
			D3D12_GPU_DESCRIPTOR_HANDLE d3d12GpuDescriptorHandle = descriptorHeap.getD3D12GpuDescriptorHandleForHeapStart();
			d3d12GpuDescriptorHandle.ptr += static_cast<UINT64>(direct3D12ResourceGroup->getDescriptorHeapOffset()) * descriptorHeap.getDescriptorSize();
			mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3d12GpuDescriptorHandle);
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Rhi::setGraphicsVertexArray(Rhi::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage

		// New vertex array?
		if (mVertexArray != vertexArray)
		{
			if (nullptr != vertexArray)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *vertexArray)

				// Begin debug event
				RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

				// Unset the currently used vertex array
				unsetGraphicsVertexArray();

				// Set new vertex array and add a reference to it
				mVertexArray = static_cast<VertexArray*>(vertexArray);
				mVertexArray->addReference();

				mVertexArray->setDirect3DIASetInputLayoutAndStreamSource(*mD3D12GraphicsCommandList);

				// End debug event
				RHI_END_DEBUG_EVENT(this)
			}
			else
			{
				// Unset the currently used vertex array
				unsetGraphicsVertexArray();
			}
		}
	}

	void Direct3D12Rhi::setGraphicsViewports(uint32_t numberOfViewports, const Rhi::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Direct3D 12 rasterizer state viewports")

		// Set the Direct3D 12 viewports
		// -> "Rhi::Viewport" directly maps to Direct3D 12, do not change it
		// -> Let Direct3D 12 perform the index validation for us (the Direct3D 12 debug features are pretty good)
		mD3D12GraphicsCommandList->RSSetViewports(numberOfViewports, reinterpret_cast<const D3D12_VIEWPORT*>(viewports));
	}

	void Direct3D12Rhi::setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Direct3D 12 rasterizer state scissor rectangles")

		// Set the Direct3D 12 scissor rectangles
		// -> "Rhi::ScissorRectangle" directly maps to Direct3D 9 & 10 & 11 & 12, do not change it
		// -> Let Direct3D 12 perform the index validation for us (the Direct3D 12 debug features are pretty good)
		mD3D12GraphicsCommandList->RSSetScissorRects(numberOfScissorRectangles, reinterpret_cast<const D3D12_RECT*>(scissorRectangles));
	}

	void Direct3D12Rhi::setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Unset the previous render target
			if (nullptr != mRenderTarget)
			{
				// Evaluate the render target type
				switch (mRenderTarget->getResourceType())
				{
					case Rhi::ResourceType::SWAP_CHAIN:
					{
						// Get the Direct3D 12 swap chain instance
						SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

						// Inform Direct3D 12 about the resource transition
						CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain->getBackD3D12ResourceRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
						mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
						break;
					}

					case Rhi::ResourceType::FRAMEBUFFER:
					{
						// TODO(co) Implement resource transition handling (first "Direct3D12Rhi::Texture2D" needs to be cleaned up)
						/*
						// Get the Direct3D 12 framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Inform Direct3D 12 about the resource transitions
						const uint32_t numberOfColorTextures = framebuffer->getNumberOfColorTextures();
						for (uint32_t i = 0; i < numberOfColorTextures; ++i)
						{
							// TODO(co) Resource type handling, currently only 2D texture is supported
							CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(static_cast<Texture2D*>(framebuffer->getColorTextures()[i])->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
							mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
						}
						if (nullptr != framebuffer->getDepthStencilTexture())
						{
							// TODO(co) Resource type handling, currently only 2D texture is supported
							CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(static_cast<Texture2D*>(framebuffer->getDepthStencilTexture())->getD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
							mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
						}
						*/
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

				// Release the render target reference, in case we have one
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

				// Evaluate the render target type
				switch (mRenderTarget->getResourceType())
				{
					case Rhi::ResourceType::SWAP_CHAIN:
					{
						// Get the Direct3D 12 swap chain instance
						SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

						{ // Inform Direct3D 12 about the resource transition
							CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain->getBackD3D12ResourceRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
							mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
						}

						// Set Direct3D 12 render target
						CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapChain->getD3D12DescriptorHeapRenderTargetView()->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(swapChain->getBackD3D12ResourceRenderTargetFrameIndex()), swapChain->getRenderTargetViewDescriptorSize());
						CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(swapChain->getD3D12DescriptorHeapDepthStencilView()->GetCPUDescriptorHandleForHeapStart());
						mD3D12GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
						break;
					}

					case Rhi::ResourceType::FRAMEBUFFER:
					{
						// Get the Direct3D 12 framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Set the Direct3D 12 render targets
						const uint32_t numberOfColorTextures = framebuffer->getNumberOfColorTextures();
						D3D12_CPU_DESCRIPTOR_HANDLE d3d12CpuDescriptorHandlesRenderTarget[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
						for (uint32_t i = 0; i < numberOfColorTextures; ++i)
						{
							d3d12CpuDescriptorHandlesRenderTarget[i] = framebuffer->getD3D12DescriptorHeapRenderTargetViews()[i]->GetCPUDescriptorHandleForHeapStart();

							// TODO(co) Implement resource transition handling (first "Direct3D12Rhi::Texture2D" needs to be cleaned up)
							/*
							{ // Inform Direct3D 12 about the resource transition
								// TODO(co) Resource type handling, currently only 2D texture is supported
								CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(static_cast<Texture2D*>(framebuffer->getColorTextures()[i])->getD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
								mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
							}
							*/
						}
						ID3D12DescriptorHeap* d3d12DescriptorHeapDepthStencilView = framebuffer->getD3D12DescriptorHeapDepthStencilView();
						if (nullptr != d3d12DescriptorHeapDepthStencilView)
						{
							// TODO(co) Implement resource transition handling (first "Direct3D12Rhi::Texture2D" needs to be cleaned up)
							/*
							{ // Inform Direct3D 12 about the resource transition
								// TODO(co) Resource type handling, currently only 2D texture is supported
								CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(static_cast<Texture2D*>(framebuffer->getDepthStencilTexture())->getD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
								mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
							}
							*/

							// Set the Direct3D 12 render targets
							D3D12_CPU_DESCRIPTOR_HANDLE d3d12CpuDescriptorHandlesDepthStencil = d3d12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart();
							mD3D12GraphicsCommandList->OMSetRenderTargets(numberOfColorTextures, d3d12CpuDescriptorHandlesRenderTarget, FALSE, &d3d12CpuDescriptorHandlesDepthStencil);
						}
						else
						{
							mD3D12GraphicsCommandList->OMSetRenderTargets(numberOfColorTextures, d3d12CpuDescriptorHandlesRenderTarget, FALSE, nullptr);
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
			}
			else
			{
				mD3D12GraphicsCommandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
			}
		}
	}

	void Direct3D12Rhi::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Unlike Direct3D 9, OpenGL or OpenGL ES 3, Direct3D 12 clears a given render target view and not the currently bound
		// -> No resource transition required in here, it's handled inside "Direct3D12Rhi::omSetRenderTarget()"

		// Sanity check
		RHI_ASSERT(mContext, z >= 0.0f && z <= 1.0f, "The Direct3D 12 clear graphics z value must be between [0, 1] (inclusive)")

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
					// Get the Direct3D 12 swap chain instance
					SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

					// Clear the Direct3D 12 render target view?
					if (clearFlags & Rhi::ClearFlag::COLOR)
					{
						CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapChain->getD3D12DescriptorHeapRenderTargetView()->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(swapChain->getBackD3D12ResourceRenderTargetFrameIndex()), swapChain->getRenderTargetViewDescriptorSize());
						mD3D12GraphicsCommandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
					}

					{ // Clear the Direct3D 12 depth stencil view?
						ID3D12DescriptorHeap* d3d12DescriptorHeapDepthStencilView = swapChain->getD3D12DescriptorHeapDepthStencilView();
						if (nullptr != d3d12DescriptorHeapDepthStencilView)
						{
							// Get the Direct3D 12 clear flags
							UINT direct3D12ClearFlags = (clearFlags & Rhi::ClearFlag::DEPTH) ? D3D12_CLEAR_FLAG_DEPTH : 0u;
							if (clearFlags & Rhi::ClearFlag::STENCIL)
							{
								direct3D12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
							}
							if (0 != direct3D12ClearFlags)
							{
								// Clear the Direct3D 12 depth stencil view
								mD3D12GraphicsCommandList->ClearDepthStencilView(d3d12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart(), static_cast<D3D12_CLEAR_FLAGS>(direct3D12ClearFlags), z, static_cast<UINT8>(stencil), 0, nullptr);
							}
						}
					}
					break;
				}

				case Rhi::ResourceType::FRAMEBUFFER:
				{
					// Get the Direct3D 12 framebuffer instance
					Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

					// Clear all Direct3D 12 render target views?
					if (clearFlags & Rhi::ClearFlag::COLOR)
					{
						// Loop through all Direct3D 12 render target views
						ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetViews = framebuffer->getD3D12DescriptorHeapRenderTargetViews() + framebuffer->getNumberOfColorTextures();
						for (ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetView = framebuffer->getD3D12DescriptorHeapRenderTargetViews(); d3d12DescriptorHeapRenderTargetView < d3d12DescriptorHeapRenderTargetViews; ++d3d12DescriptorHeapRenderTargetView)
						{
							// Valid Direct3D 12 render target view?
							if (nullptr != *d3d12DescriptorHeapRenderTargetView)
							{
								mD3D12GraphicsCommandList->ClearRenderTargetView((*d3d12DescriptorHeapRenderTargetView)->GetCPUDescriptorHandleForHeapStart(), color, 0, nullptr);
							}
						}
					}

					// Clear the Direct3D 12 depth stencil view?
					ID3D12DescriptorHeap* d3d12DescriptorHeapDepthStencilView = framebuffer->getD3D12DescriptorHeapDepthStencilView();
					if (nullptr != d3d12DescriptorHeapDepthStencilView)
					{
						// Get the Direct3D 12 clear flags
						UINT direct3D12ClearFlags = (clearFlags & Rhi::ClearFlag::DEPTH) ? D3D12_CLEAR_FLAG_DEPTH : 0u;
						if (clearFlags & Rhi::ClearFlag::STENCIL)
						{
							direct3D12ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
						}
						if (0 != direct3D12ClearFlags)
						{
							// Clear the Direct3D 12 depth stencil view
							mD3D12GraphicsCommandList->ClearDepthStencilView(d3d12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart(), static_cast<D3D12_CLEAR_FLAGS>(direct3D12ClearFlags), z, static_cast<UINT8>(stencil), 0, nullptr);
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
		else
		{
			// In case no render target is currently set we don't have to do anything in here
		}

		// End debug event
		RHI_END_DEBUG_EVENT(this)
	}

	void Direct3D12Rhi::drawGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 12 draws must not be zero")
		// It's possible to draw without "mVertexArray"

		// Execute Direct3D 12 indirect
		const IndirectBuffer& d3d12IndirectBuffer = static_cast<const IndirectBuffer&>(indirectBuffer);
		mD3D12GraphicsCommandList->ExecuteIndirect(d3d12IndirectBuffer.getD3D12CommandSignature(), numberOfDraws, d3d12IndirectBuffer.getD3D12Resource(), indirectBufferOffset, nullptr, 0);
	}

	void Direct3D12Rhi::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The Direct3D 12 emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 12 draws must not be zero")

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
			mD3D12GraphicsCommandList->DrawInstanced(
				drawArguments.vertexCountPerInstance,	// Vertex count per instance (UINT)
				drawArguments.instanceCount,			// Instance count (UINT)
				drawArguments.startVertexLocation,		// Start vertex location (UINT)
				drawArguments.startInstanceLocation		// Start instance location (UINT)
			);

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

	void Direct3D12Rhi::drawIndexedGraphics(const Rhi::IIndirectBuffer& indirectBuffer, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, indirectBuffer)
		RHI_ASSERT(mContext, numberOfDraws > 0, "Number of Direct3D 12 draws must not be zero")
		RHI_ASSERT(mContext, nullptr != mVertexArray, "Direct3D 12 draw indexed needs a set vertex array")
		RHI_ASSERT(mContext, nullptr != mVertexArray->getIndexBuffer(), "Direct3D 12 draw indexed needs a set vertex array which contains an index buffer")

		// Execute Direct3D 12 indirect
		const IndirectBuffer& d3d12IndirectBuffer = static_cast<const IndirectBuffer&>(indirectBuffer);
		mD3D12GraphicsCommandList->ExecuteIndirect(d3d12IndirectBuffer.getD3D12CommandSignature(), numberOfDraws, d3d12IndirectBuffer.getD3D12Resource(), indirectBufferOffset, nullptr, 0);
	}

	void Direct3D12Rhi::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The Direct3D 12 emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 12 draws must not be zero")

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
			mD3D12GraphicsCommandList->DrawIndexedInstanced(
				drawIndexedArguments.indexCountPerInstance,	// Index count per instance (UINT)
				drawIndexedArguments.instanceCount,			// Instance count (UINT)
				drawIndexedArguments.startIndexLocation,	// Start index location (UINT)
				drawIndexedArguments.baseVertexLocation,	// Base vertex location (INT)
				drawIndexedArguments.startInstanceLocation	// Start instance location (UINT)
			);

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

	void Direct3D12Rhi::drawMeshTasks([[maybe_unused]] const Rhi::IIndirectBuffer& indirectBuffer, [[maybe_unused]] uint32_t indirectBufferOffset, [[maybe_unused]] uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")

		// TODO(co) Implement me
	}

	void Direct3D12Rhi::drawMeshTasksEmulated([[maybe_unused]] const uint8_t* emulationData, uint32_t, [[maybe_unused]] uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The null emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")

		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Compute                                               ]
	//[-------------------------------------------------------]
	void Direct3D12Rhi::setComputeRootSignature(Rhi::IRootSignature* rootSignature)
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

			// Set compute root signature
			mD3D12GraphicsCommandList->SetComputeRootSignature(mComputeRootSignature->getD3D12RootSignature());
		}
	}

	void Direct3D12Rhi::setComputePipelineState(Rhi::IComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *computePipelineState)

			// Set compute pipeline state
			mD3D12GraphicsCommandList->SetPipelineState(static_cast<ComputePipelineState*>(computePipelineState)->getD3D12ComputePipelineState());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Rhi::setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mComputeRootSignature, "No Direct3D 12 RHI implementation compute root signature set")
			const Rhi::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The Direct3D 12 RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The Direct3D 12 RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The Direct3D 12 RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// Set Direct3D 12 compute root descriptor table
			const ResourceGroup* direct3D12ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const ::detail::DescriptorHeap& descriptorHeap = (direct3D12ResourceGroup->getD3D12DescriptorHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ? *mShaderResourceViewDescriptorHeap : *mSamplerDescriptorHeap;
			D3D12_GPU_DESCRIPTOR_HANDLE d3d12GpuDescriptorHandle = descriptorHeap.getD3D12GpuDescriptorHandleForHeapStart();
			d3d12GpuDescriptorHandle.ptr += static_cast<UINT64>(direct3D12ResourceGroup->getDescriptorHeapOffset()) * descriptorHeap.getDescriptorSize();
			mD3D12GraphicsCommandList->SetComputeRootDescriptorTable(rootParameterIndex, d3d12GpuDescriptorHandle);
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Rhi::dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		mD3D12GraphicsCommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void Direct3D12Rhi::resolveMultisampleFramebuffer(Rhi::IRenderTarget&, Rhi::IFramebuffer&)
	{
		// TODO(co) Implement me
	}

	void Direct3D12Rhi::copyResource(Rhi::IResource&, Rhi::IResource&)
	{
		// TODO(co) Implement me
	}

	void Direct3D12Rhi::generateMipmaps(Rhi::IResource&)
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void Direct3D12Rhi::resetQueryPool([[maybe_unused]] Rhi::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, queryPool)
		RHI_ASSERT(mContext, firstQueryIndex < static_cast<const QueryPool&>(queryPool).getNumberOfQueries(), "Direct3D 12 out-of-bounds query index")
		RHI_ASSERT(mContext, (firstQueryIndex + numberOfQueries) <= static_cast<const QueryPool&>(queryPool).getNumberOfQueries(), "Direct3D 12 out-of-bounds query index")

		// Nothing to do in here for Direct3D 12
	}

	void Direct3D12Rhi::beginQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex, uint32_t)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& d3d12QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < d3d12QueryPool.getNumberOfQueries(), "Direct3D 12 out-of-bounds query index")
		switch (d3d12QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				mD3D12GraphicsCommandList->BeginQuery(d3d12QueryPool.getD3D12QueryHeap(), D3D12_QUERY_TYPE_OCCLUSION, queryIndex);
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				mD3D12GraphicsCommandList->BeginQuery(d3d12QueryPool.getD3D12QueryHeap(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, queryIndex);
				break;

			case Rhi::QueryType::TIMESTAMP:
				RHI_ASSERT(mContext, false, "Direct3D 12 begin query isn't allowed for timestamp queries, use \"Rhi::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void Direct3D12Rhi::endQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& d3d12QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < d3d12QueryPool.getNumberOfQueries(), "Direct3D 12 out-of-bounds query index")
		switch (d3d12QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				mD3D12GraphicsCommandList->EndQuery(d3d12QueryPool.getD3D12QueryHeap(), D3D12_QUERY_TYPE_OCCLUSION, queryIndex);
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				mD3D12GraphicsCommandList->EndQuery(d3d12QueryPool.getD3D12QueryHeap(), D3D12_QUERY_TYPE_PIPELINE_STATISTICS, queryIndex);
				break;

			case Rhi::QueryType::TIMESTAMP:
				RHI_ASSERT(mContext, false, "Direct3D 12 end query isn't allowed for timestamp queries, use \"Rhi::Command::WriteTimestampQuery\" instead")
				break;
		}
	}

	void Direct3D12Rhi::writeTimestampQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, queryPool)

		// Query pool type dependent processing
		const QueryPool& d3d12QueryPool = static_cast<const QueryPool&>(queryPool);
		RHI_ASSERT(mContext, queryIndex < d3d12QueryPool.getNumberOfQueries(), "Direct3D 12 out-of-bounds query index")
		switch (d3d12QueryPool.getQueryType())
		{
			case Rhi::QueryType::OCCLUSION:
				RHI_ASSERT(mContext, false, "Direct3D 12 write timestamp query isn't allowed for occlusion queries, use \"Rhi::Command::BeginQuery\" and \"Rhi::Command::EndQuery\" instead")
				break;

			case Rhi::QueryType::PIPELINE_STATISTICS:
				RHI_ASSERT(mContext, false, "Direct3D 12 write timestamp query isn't allowed for pipeline statistics queries, use \"Rhi::Command::BeginQuery\" and \"Rhi::Command::EndQuery\" instead")
				break;

			case Rhi::QueryType::TIMESTAMP:
				mD3D12GraphicsCommandList->EndQuery(d3d12QueryPool.getD3D12QueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
				break;
		}
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void Direct3D12Rhi::setDebugMarker(const char* name)
		{
			if (nullptr != mD3D12GraphicsCommandList)
			{
				RHI_ASSERT(mContext, nullptr != name, "Direct3D 12 debug marker names must not be a null pointer")
				const UINT size = static_cast<UINT>((strlen(name) + 1) * sizeof(name[0]));
				mD3D12GraphicsCommandList->SetMarker(PIX_EVENT_ANSI_VERSION, name, size);
			}
		}

		void Direct3D12Rhi::beginDebugEvent(const char* name)
		{
			if (nullptr != mD3D12GraphicsCommandList)
			{
				RHI_ASSERT(mContext, nullptr != name, "Direct3D 12 debug event names must not be a null pointer")
				const UINT size = static_cast<UINT>((strlen(name) + 1) * sizeof(name[0]));
				mD3D12GraphicsCommandList->BeginEvent(PIX_EVENT_ANSI_VERSION, name, size);
			}
		}

		void Direct3D12Rhi::endDebugEvent()
		{
			if (nullptr != mD3D12GraphicsCommandList)
			{
				mD3D12GraphicsCommandList->EndEvent();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	bool Direct3D12Rhi::isDebugEnabled()
	{
		#ifdef RHI_DEBUG
			return true;
		#else
			return false;
		#endif
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t Direct3D12Rhi::getNumberOfShaderLanguages() const
	{
		uint32_t numberOfShaderLanguages = 1;	// HLSL support is always there

		// Done, return the number of supported shader languages
		return numberOfShaderLanguages;
	}

	const char* Direct3D12Rhi::getShaderLanguageName([[maybe_unused]] uint32_t index) const
	{
		RHI_ASSERT(mContext, index < getNumberOfShaderLanguages(), "Direct3D 12: Shader language index is out-of-bounds")
		return ::detail::HLSL_NAME;
	}

	Rhi::IShaderLanguage* Direct3D12Rhi::getShaderLanguage(const char* shaderLanguageName)
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
	Rhi::IRenderPass* Direct3D12Rhi::createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, [[maybe_unused]] uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IQueryPool* Direct3D12Rhi::createQueryPool([[maybe_unused]] Rhi::QueryType queryType, [[maybe_unused]] uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		RHI_ASSERT(mContext, numberOfQueries > 0, "Direct3D 12: Number of queries mustn't be zero")
		return RHI_NEW(mContext, QueryPool)(*this, queryType, numberOfQueries RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::ISwapChain* Direct3D12Rhi::createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, renderPass)
		RHI_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Direct3D 12: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RHI_NEW(mContext, SwapChain)(renderPass, windowHandle RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IFramebuffer* Direct3D12Rhi::createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, renderPass)

		// Create the framebuffer
		return RHI_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IBufferManager* Direct3D12Rhi::createBufferManager()
	{
		return RHI_NEW(mContext, BufferManager)(*this);
	}

	Rhi::ITextureManager* Direct3D12Rhi::createTextureManager()
	{
		return RHI_NEW(mContext, TextureManager)(*this);
	}

	Rhi::IRootSignature* Direct3D12Rhi::createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RootSignature)(*this, rootSignature RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IGraphicsPipelineState* Direct3D12Rhi::createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "Direct3D 12: Invalid graphics pipeline state root signature")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "Direct3D 12: Invalid graphics pipeline state graphics program")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "Direct3D 12: Invalid graphics pipeline state render pass")

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

	Rhi::IComputePipelineState* Direct3D12Rhi::createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
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

	Rhi::ISamplerState* Direct3D12Rhi::createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// No debug name possible since all sampler states are inside a descriptor heap
		return RHI_NEW(mContext, SamplerState)(*this, samplerState RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool Direct3D12Rhi::map(Rhi::IResource& resource, uint32_t, Rhi::MapType, uint32_t, Rhi::MappedSubresource& mappedSubresource)
	{
		// The "Rhi::MapType" values directly map to Direct3D 10 & 11 constants, do not change them
		// The "Rhi::MappedSubresource" structure directly maps to Direct3D 11, do not change it

		// Define helper macro
		// TODO(co) Port to Direct3D 12
		#define TEXTURE_RESOURCE(type, typeClass) \
			case type: \
			{ \
				return false;	\
			}

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
			{
				mappedSubresource.rowPitch = 0;
				mappedSubresource.depthPitch = 0;
				CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
				return SUCCEEDED(static_cast<VertexBuffer&>(resource).getD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&mappedSubresource.data)));
			}

			case Rhi::ResourceType::INDEX_BUFFER:
			{
				mappedSubresource.rowPitch = 0;
				mappedSubresource.depthPitch = 0;
				CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
				return SUCCEEDED(static_cast<IndexBuffer&>(resource).getD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&mappedSubresource.data)));
			}

			case Rhi::ResourceType::TEXTURE_BUFFER:
			{
				mappedSubresource.rowPitch = 0;
				mappedSubresource.depthPitch = 0;
				CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
				// TODO(co) Port to Direct3D 12
				return false;
				// return SUCCEEDED(static_cast<TextureBuffer&>(resource).getD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&mappedSubresource.data)));
			}

			case Rhi::ResourceType::STRUCTURED_BUFFER:
			{
				mappedSubresource.rowPitch = 0;
				mappedSubresource.depthPitch = 0;
				CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
				// TODO(co) Port to Direct3D 12
				return false;
				// return SUCCEEDED(static_cast<StructuredBuffer&>(resource).getD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&mappedSubresource.data)));
			}

			case Rhi::ResourceType::INDIRECT_BUFFER:
			{
				mappedSubresource.rowPitch = 0;
				mappedSubresource.depthPitch = 0;
				CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
				return SUCCEEDED(static_cast<IndirectBuffer&>(resource).getD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&mappedSubresource.data)));
			}

			case Rhi::ResourceType::UNIFORM_BUFFER:
			{
				mappedSubresource.rowPitch = 0;
				mappedSubresource.depthPitch = 0;
				CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
				return SUCCEEDED(static_cast<UniformBuffer&>(resource).getD3D12Resource()->Map(0, &readRange, reinterpret_cast<void**>(&mappedSubresource.data)));
			}

			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D, Texture1D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D_ARRAY, Texture1DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D, Texture2D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D_ARRAY, Texture2DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_3D, Texture3D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_CUBE, TextureCube)
			//TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_CUBE_ARRAY, TextureCubeArray)	// TODO(co) Implement me
			case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:

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

		// Undefine helper macro
		#undef TEXTURE_RESOURCE
	}

	void Direct3D12Rhi::unmap(Rhi::IResource& resource, uint32_t)
	{
		// Define helper macro
		// TODO(co) Port to Direct3D 12
		#define TEXTURE_RESOURCE(type, typeClass) \
			case type: \
			{ \
				break; \
			}

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Rhi::ResourceType::VERTEX_BUFFER:
				static_cast<VertexBuffer&>(resource).getD3D12Resource()->Unmap(0, nullptr);
				break;

			case Rhi::ResourceType::INDEX_BUFFER:
				static_cast<IndexBuffer&>(resource).getD3D12Resource()->Unmap(0, nullptr);
				break;

			case Rhi::ResourceType::TEXTURE_BUFFER:
				// TODO(co) Port to Direct3D 12
				// static_cast<TextureBuffer&>(resource).getD3D12Resource()->Unmap(0, nullptr);
				break;

			case Rhi::ResourceType::STRUCTURED_BUFFER:
				// TODO(co) Port to Direct3D 12
				// static_cast<StructuredBuffer&>(resource).getD3D12Resource()->Unmap(0, nullptr);
				break;

			case Rhi::ResourceType::INDIRECT_BUFFER:
				static_cast<IndirectBuffer&>(resource).getD3D12Resource()->Unmap(0, nullptr);
				break;

			case Rhi::ResourceType::UNIFORM_BUFFER:
				static_cast<UniformBuffer&>(resource).getD3D12Resource()->Unmap(0, nullptr);
				break;

			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D, Texture1D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_1D_ARRAY, Texture1DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D, Texture2D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_2D_ARRAY, Texture2DArray)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_3D, Texture3D)
			TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_CUBE, TextureCube)
			// TEXTURE_RESOURCE(Rhi::ResourceType::TEXTURE_CUBE_ARRAY, TextureCubeArray)	// TODO(co) Implement me
			case Rhi::ResourceType::TEXTURE_CUBE_ARRAY:

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

		// Undefine helper macro
		#undef TEXTURE_RESOURCE
	}

	bool Direct3D12Rhi::getQueryPoolResults(Rhi::IQueryPool& queryPool, uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex, uint32_t numberOfQueries, uint32_t strideInBytes, uint32_t)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, queryPool)
		RHI_ASSERT(mContext, numberOfDataBytes >= sizeof(UINT64), "Direct3D 12 out-of-memory query access")
		RHI_ASSERT(mContext, 1 == numberOfQueries || strideInBytes > 0, "Direct3D 12 invalid stride in bytes")
		RHI_ASSERT(mContext, numberOfDataBytes >= strideInBytes * numberOfQueries, "Direct3D 12 out-of-memory query access")
		RHI_ASSERT(mContext, nullptr != data, "Direct3D 12 out-of-memory query access")
		RHI_ASSERT(mContext, numberOfQueries > 0, "Direct3D 12 number of queries mustn't be zero")

		// Get query pool results
		static_cast<QueryPool&>(queryPool).getQueryPoolResults(numberOfDataBytes, data, firstQueryIndex, numberOfQueries, strideInBytes, *mD3D12GraphicsCommandList);

		// Done
		return true;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool Direct3D12Rhi::beginScene()
	{
		bool result = false;	// Error by default

		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, false == mDebugBetweenBeginEndScene, "Direct3D 12: Begin scene was called while scene rendering is already in progress, missing end scene call?")
			mDebugBetweenBeginEndScene = true;
		#endif

		// Not required when using Direct3D 12
		// TODO(co) Until we have a command list interface, we must perform the command list handling in here

		// Command list allocators can only be reset when the associated
		// command lists have finished execution on the GPU; apps should use
		// fences to determine GPU execution progress.
		if (SUCCEEDED(mD3D12CommandAllocator->Reset()))
		{
			// However, when ExecuteCommandList() is called on a particular command
			// list, that command list can then be reset at any time and must be before
			// re-recording.
			result = SUCCEEDED(mD3D12GraphicsCommandList->Reset(mD3D12CommandAllocator, nullptr));
			if (result)
			{
				// Set descriptor heaps
				ID3D12DescriptorHeap* d3d12DescriptorHeaps[] = { mShaderResourceViewDescriptorHeap->getD3D12DescriptorHeap(), mSamplerDescriptorHeap->getD3D12DescriptorHeap() };
				mD3D12GraphicsCommandList->SetDescriptorHeaps(2, d3d12DescriptorHeaps);
			}
		}

		// Reset our cached states where needed
		mD3D12PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		// Done
		return result;
	}

	void Direct3D12Rhi::submitCommandBuffer(const Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(mContext, !commandBuffer.isEmpty(), "The Direct3D 12 command buffer to execute mustn't be empty")

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

	void Direct3D12Rhi::endScene()
	{
		// Sanity check
		#ifdef RHI_DEBUG
			RHI_ASSERT(mContext, true == mDebugBetweenBeginEndScene, "Direct3D 12: End scene was called while scene rendering isn't in progress, missing start scene call?")
			mDebugBetweenBeginEndScene = false;
		#endif

		// Begin debug event
		RHI_BEGIN_DEBUG_EVENT_FUNCTION(this)

		// Finish previous uploads and start new ones
		ID3D12CommandList* uploadD3d12CommandList = mUploadContext.getD3d12GraphicsCommandList();
		mUploadContext.begin();

		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);

		// We need to forget about the currently set vertex array
		unsetGraphicsVertexArray();

		// End debug event
		RHI_END_DEBUG_EVENT(this)

		// Close and execute the command list
		if (SUCCEEDED(mD3D12GraphicsCommandList->Close()))
		{
			if (nullptr != uploadD3d12CommandList)
			{
				ID3D12CommandList* commandLists[] = { uploadD3d12CommandList, mD3D12GraphicsCommandList };
				mD3D12CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
			}
			else
			{
				ID3D12CommandList* commandLists[] = { mD3D12GraphicsCommandList };
				mD3D12CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
			}
		}

		// Release the graphics and compute root signature instance, in case we have one
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
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void Direct3D12Rhi::initializeCapabilities()
	{
		// TODO(co) Direct3D 12 update

		// There are no Direct3D 12 device capabilities we could query on runtime, they depend on the chosen feature level
		// -> Have a look at "Devices -> Direct3D 12 on Downlevel Hardware -> Introduction" at MSDN http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx
		//    for a table with a list of the minimum resources supported by Direct3D 12 at the different feature levels

		{ // Get device name
			// Get DXGI adapter
			IDXGIAdapter* dxgiAdapter = nullptr;
			mDxgiFactory4->EnumAdapterByLuid(mD3D12Device->GetAdapterLuid(), IID_PPV_ARGS(&dxgiAdapter));

			// The adapter contains a description like "AMD Radeon R9 200 Series"
			DXGI_ADAPTER_DESC dxgiAdapterDesc = {};
			dxgiAdapter->GetDesc(&dxgiAdapterDesc);

			// Convert UTF-16 string to UTF-8
			const size_t numberOfCharacters = _countof(mCapabilities.deviceName) - 1;
			::WideCharToMultiByte(CP_UTF8, 0, dxgiAdapterDesc.Description, static_cast<int>(wcslen(dxgiAdapterDesc.Description)), mCapabilities.deviceName, static_cast<int>(numberOfCharacters), nullptr, nullptr);
			mCapabilities.deviceName[numberOfCharacters] = '\0';

			// Release references
			dxgiAdapter->Release();
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Rhi::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;

		// Evaluate the chosen feature level
		switch (D3D_FEATURE_LEVEL_12_0)
		// switch (mD3D12Device->GetFeatureLevel())	// TODO(co) Direct3D 12 update
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

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 0;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

				// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
				mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;

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

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 0;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

				// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
				mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;

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

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 0;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

				// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
				mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;

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
				// TODO(co) Direct3D 12 update
				// mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
				mCapabilities.maximumNumberOfViewports = 8;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 8192;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 512;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

				// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
				mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;

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
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
				mCapabilities.maximumNumberOfViewports = 8;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 8192;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 512;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

				// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
				mCapabilities.maximumNumberOfCubeTextureArraySlices = 512;

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
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
				// Maximum number of viewports (always at least 1)
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfViewports = D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
				mCapabilities.maximumNumberOfViewports = 8;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 16384;

				// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
				mCapabilities.maximumNumberOf1DTextureArraySlices = 512;

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

				// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
				mCapabilities.maximumNumberOfCubeTextureArraySlices = 512;

				// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
				mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 128 * 1024 * 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/ff476876%28v=vs.85%29.aspx does not mention the texture buffer? Currently the OpenGL 3 minimum is used: 128 MiB.

				// Maximum number of multisamples (always at least 1, usually 8)
				mCapabilities.maximumNumberOfMultisamples = 8;

				// Maximum anisotropy (always at least 1, usually 16)
				mCapabilities.maximumAnisotropy = 16;

				// Maximum indirect buffer size in bytes
				mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

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

		// TODO(co) Implement me, remove this when done
		mCapabilities.maximumNumberOfCubeTextureArraySlices = 0;

		// The rest is the same for all feature levels

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		// -> Same as DirectX 11: See https://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx - "Resource Limits (Direct3D 11)" - "Number of elements in a constant buffer D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT (4096)"
		// -> One element = float4 = 16 bytes
		mCapabilities.maximumUniformBufferSize = 4096 * 16;

		// Left-handed coordinate system with clip space depth value range 0..1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = true;

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = false;

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = true;

		// Direct3D 12 has native multithreading // TODO(co) But do only set this to true if it has been tested
		mCapabilities.nativeMultithreading = false;

		// Direct3D 12 has shader bytecode support
		// TODO(co) Implement shader bytecode support
		mCapabilities.shaderBytecode = false;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for task shaders (TS) and mesh shaders (MS)?
		mCapabilities.meshShader = false;	// TODO(co) "DirectX 12 Ultimate" needed

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = true;
	}

	void Direct3D12Rhi::unsetGraphicsVertexArray()
	{
		// Release the currently used vertex array reference, in case we have one
		if (nullptr != mVertexArray)
		{
			// Set no Direct3D 12 input layout
			mD3D12GraphicsCommandList->IASetVertexBuffers(0, 0, nullptr);

			// Release reference
			mVertexArray->releaseReference();
			mVertexArray = nullptr;
		}
	}

	#ifdef RHI_DEBUG
		void Direct3D12Rhi::debugReportLiveDeviceObjects()
		{
			ID3D12DebugDevice* d3d12DebugDevice = nullptr;
			if (SUCCEEDED(mD3D12Device->QueryInterface(IID_PPV_ARGS(&d3d12DebugDevice))))
			{
				d3d12DebugDevice->ReportLiveDeviceObjects(static_cast<D3D12_RLDO_FLAGS>(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
				d3d12DebugDevice->Release();
			}
		}
	#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D12Rhi




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RHI_DIRECT3D12_EXPORTS
	#define DIRECT3D12RHI_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define DIRECT3D12RHI_FUNCTION_EXPORT
#endif
DIRECT3D12RHI_FUNCTION_EXPORT Rhi::IRhi* createDirect3D12RhiInstance(const Rhi::Context& context)
{
	return RHI_NEW(context, Direct3D12Rhi::Direct3D12Rhi)(context);
}
#undef DIRECT3D12RHI_FUNCTION_EXPORT
