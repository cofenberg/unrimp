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
*    Direct3D 12 renderer amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    Direct3D 12 runtime and Direct3D 12 capable graphics driver, nothing else.
*
*    == Preprocessor Definitions ==
*    - Set "RENDERER_DIRECT3D12_EXPORTS" as preprocessor definition when building this library as shared library
*    - Do also have a look into the renderer header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Public/Renderer.h>

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
#include <windows.h>

// Get rid of some nasty OS macros
#undef max




//[-------------------------------------------------------]
//[ Direct3D12Renderer/MakeID.h                           ]
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
//[ Direct3D11Renderer/D3D12.h                            ]
//[-------------------------------------------------------]
/*
  We don't use the Direct3D headers from the DirectX SDK because there are several issues:
  - Licensing: It's not allowed to redistribute the Direct3D headers, meaning everyone would
    have to get them somehow before compiling this project
  - The Direct3D headers are somewhat chaotic and include tons of other headers.
    This slows down compilation and the more headers are included, the higher the risk of
    naming or redefinition conflicts.

    Do not include this header within headers which are usually used by users as well, do only
    use it inside cpp-files. It must still be possible that users of this renderer interface
    can use the Direct3D headers for features not covered by this renderer interface.
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
struct D3D12_QUERY_HEAP_DESC;
struct D3D12_PACKED_MIP_INFO;
struct D3D12_TILE_REGION_SIZE;
struct D3D12_TILE_RANGE_FLAGS;
struct D3D12_SUBRESOURCE_TILING;
struct D3D12_SO_DECLARATION_ENTRY;
struct D3D12_COMMAND_SIGNATURE_DESC;
struct D3D12_STREAM_OUTPUT_BUFFER_VIEW;
struct D3D12_TILED_RESOURCE_COORDINATE;
struct D3D12_UNORDERED_ACCESS_VIEW_DESC;
struct D3D12_COMPUTE_PIPELINE_STATE_DESC;
struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT;
struct ID3D12Heap;
struct ID3D12Resource;
struct ID3D12QueryHeap;
struct ID3D12RootSignature;
struct ID3D12CommandSignature;

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
typedef enum D3D12_PREDICATION_OP
{
	D3D12_PREDICATION_OP_EQUAL_ZERO		= 0,
	D3D12_PREDICATION_OP_NOT_EQUAL_ZERO	= 1
} D3D12_PREDICATION_OP;

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

#ifdef RENDERER_DEBUG
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
	public:
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID Name, UINT DataSize, _In_reads_bytes_(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID Name, _In_ const IUnknown *pUnknown) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID Name, _Inout_ UINT *pDataSize, _Out_writes_bytes_(*pDataSize) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetParent(_In_ REFIID riid, _COM_Outptr_ void **ppParent) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("3d3e0379-f9de-4d58-bb6c-18d62992f1a6")
IDXGIDeviceSubObject : public IDXGIObject
{
	public:
		virtual HRESULT STDMETHODCALLTYPE GetDevice(_In_ REFIID riid, _COM_Outptr_ void **ppDevice) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("310d36a0-d2e7-4c0a-aa04-6a9d23b8886a")
IDXGISwapChain : public IDXGIDeviceSubObject
{
	public:
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
	public:
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
public:
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
public:
	virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace, _Out_ UINT *pColorSpaceSupport) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace) = 0;
	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(_In_ UINT BufferCount, _In_ UINT Width, _In_ UINT Height, _In_ DXGI_FORMAT Format, _In_ UINT SwapChainFlags, _In_reads_(BufferCount) const UINT *pCreationNodeMask, _In_reads_(BufferCount) IUnknown *const *ppPresentQueue) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("7b7166ec-21c7-44ae-b21a-c9ae321ae369")
IDXGIFactory : public IDXGIObject
{
	public:
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
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, __out IDXGIAdapter1 **ppAdapter) = 0;
		virtual BOOL STDMETHODCALLTYPE IsCurrent(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
IDXGIFactory2 : public IDXGIFactory1
{
	public:
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
	public:
		virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("1bc6ea02-ef36-464f-bf0c-21ca39e5168a")
IDXGIFactory4 : public IDXGIFactory3
{
	public:
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
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumOutputs(UINT Output, __out IDXGIOutput **ppOutput) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetDesc(__out DXGI_ADAPTER_DESC *pDesc) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(__in REFGUID InterfaceName, __out LARGE_INTEGER *pUMDVersion) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "DXGI.h"
MIDL_INTERFACE("54ec77fa-1377-44e6-8c32-88fd5f44c84c")
IDXGIDevice : public IDXGIObject
{
	public:
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
	public:
		virtual LPVOID STDMETHODCALLTYPE GetBufferPointer(void) = 0;
		virtual SIZE_T STDMETHODCALLTYPE GetBufferSize(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("c4fec28f-7966-4e95-9f94-f431cb56c3b8")
ID3D12Object : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID guid, _Inout_ UINT *pDataSize, _Out_writes_bytes_opt_( *pDataSize ) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID guid, _In_ UINT DataSize, _In_reads_bytes_opt_(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID guid, _In_opt_ const IUnknown *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetName(_In_z_ LPCWSTR Name) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("905db94b-a00c-4140-9df5-2b64ca9ea357")
ID3D12DeviceChild : public ID3D12Object
{
	public:
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
MIDL_INTERFACE("0a753dcf-c4d8-4b91-adf6-be5a60d95a76")
ID3D12Fence : public ID3D12Pageable
{
	public:
		virtual UINT64 STDMETHODCALLTYPE GetCompletedValue(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64 Value, HANDLE hEvent) = 0;
		virtual HRESULT STDMETHODCALLTYPE Signal(UINT64 Value) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("696442be-a72e-4059-bc79-5b5c98040fad")
ID3D12Resource : public ID3D12Pageable
{
	public:
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
	public:
		virtual HRESULT STDMETHODCALLTYPE GetCachedBlob(_COM_Outptr_ ID3DBlob* *ppBlob) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("8efb471d-616c-4f49-90f7-127bb763fa51")
ID3D12DescriptorHeap : public ID3D12Pageable
{
	public:
		virtual D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE GetDesc(void) = 0;
		virtual D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetCPUDescriptorHandleForHeapStart(void) = 0;
		virtual D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE GetGPUDescriptorHandleForHeapStart(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("6102dee4-af59-4b09-b999-b44d73f09b24")
ID3D12CommandAllocator : public ID3D12Pageable
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("7116d91c-e7e4-47ce-b8c6-ec8168f437e5")
ID3D12CommandList : public ID3D12DeviceChild
{
	public:
		virtual D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE GetType(void) = 0;
};

// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "D3D12.h"
MIDL_INTERFACE("5b160d0f-ac1b-4185-8ba8-b3ae42a5a455")
ID3D12GraphicsCommandList : public ID3D12CommandList
{
	public:
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
	public:
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
	public:
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

#ifdef RENDERER_DEBUG
	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3d12sdklayers.h"
	MIDL_INTERFACE("344488b7-6846-474b-b989-f027448245e0")
	ID3D12Debug : public IUnknown
	{
		public:
			virtual void STDMETHODCALLTYPE EnableDebugLayer(void) = 0;
	};

	// "Microsoft Windows 10 SDK" -> "10.0.10240.0" -> "d3d12sdklayers.h"
	MIDL_INTERFACE("3febd6dd-4973-4787-8194-e45f9e28923e")
	ID3D12DebugDevice : public IUnknown
	{
		public:
			virtual HRESULT STDMETHODCALLTYPE SetFeatureMask(D3D12_DEBUG_FEATURE Mask) = 0;
			virtual D3D12_DEBUG_FEATURE STDMETHODCALLTYPE GetFeatureMask(void) = 0;
			virtual HRESULT STDMETHODCALLTYPE ReportLiveDeviceObjects(D3D12_RLDO_FLAGS Flags) = 0;
	};
#endif




//[-------------------------------------------------------]
//[ Direct3D11Renderer/D3D12X.h                           ]
//[-------------------------------------------------------]
// TODO(co) Remove unused stuff when done with the Direct3D 12 renderer backend implementation


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
namespace Direct3D12Renderer
{
	class Direct3D12RuntimeLinking;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
#ifdef RENDERER_DEBUG
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given renderer
	*/
	#define DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference) \
		RENDERER_ASSERT(mContext, &rendererReference == &(resourceReference).getRenderer(), "Direct3D 12 error: The given resource is owned by another renderer instance")

	/*
	*  @brief
	*    Debug break on execution failure
	*/
	#define FAILED_DEBUG_BREAK(toExecute) if (FAILED(toExecute)) { DEBUG_BREAK; }
#else
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given renderer
	*/
	#define DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference)

	/*
	*  @brief
	*    Debug break on execution failure
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
		static constexpr const char* HLSL_NAME = "HLSL";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)


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


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Direct3D12Renderer
{




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Direct3D12Renderer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 renderer class
	*/
	class Direct3D12Renderer final : public Renderer::IRenderer
	{


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
		explicit Direct3D12Renderer(const Renderer::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Direct3D12Renderer() override;

		/**
		*  @brief
		*    Return the DXGI factory instance as pointer
		*
		*  @return
		*    The DXGI factory instance, null pointer on error (but always valid for a correctly initialized renderer), do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDXGIFactory4 *getDxgiFactory4() const
		{
			return mDxgiFactory4;
		}

		/**
		*  @brief
		*    Return the DXGI factory instance as reference
		*
		*  @return
		*    The DXGI factory instance, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDXGIFactory4 &getDxgiFactory4Safe() const
		{
			RENDERER_ASSERT(mContext, nullptr != mDxgiFactory4, "Invalid Direct3D 12 DXGI factory 3")
			return *mDxgiFactory4;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 device
		*
		*  @return
		*    The Direct3D 12 device, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12Device *getD3D12Device() const
		{
			return mD3D12Device;
		}

		/**
		*  @brief
		*    Return the Direct3D 12 command queue
		*
		*  @return
		*    The Direct3D 12 command queue, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12CommandQueue *getD3D12CommandQueue() const
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
		[[nodiscard]] inline ID3D12GraphicsCommandList *getD3D12GraphicsCommandList() const
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
		[[nodiscard]] inline Renderer::IRenderTarget* omGetRenderTarget() const
		{
			return mRenderTarget;
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
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
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
			RENDERER_DELETE(mContext, Direct3D12Renderer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D12Renderer(const Direct3D12Renderer& source) = delete;
		Direct3D12Renderer& operator =(const Direct3D12Renderer& source) = delete;

		/**
		*  @brief
		*    Initialize the capabilities
		*/
		void initializeCapabilities();

		#ifdef RENDERER_DEBUG
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
		IDXGIFactory4*			   mDxgiFactory4;				///< DXGI factors instance, always valid for a correctly initialized renderer
		ID3D12Device*			   mD3D12Device;				///< The Direct3D 12 device, null pointer on error (we don't check because this would be a total overhead, the user has to use "Renderer::IRenderer::isInitialized()" and is asked to never ever use a not properly initialized renderer!)
		ID3D12CommandQueue*		   mD3D12CommandQueue;			///< The Direct3D 12 command queue, null pointer on error (we don't check because this would be a total overhead, the user has to use "Renderer::IRenderer::isInitialized()" and is asked to never ever use a not properly initialized renderer!)
		ID3D12CommandAllocator*	   mD3D12CommandAllocator;
		ID3D12GraphicsCommandList* mD3D12GraphicsCommandList;
		Renderer::IShaderLanguage* mShaderLanguageHlsl;			///< HLSL shader language instance (we keep a reference to it), can be a null pointer
		// ID3D12Query*			   mD3D12QueryFlush;			///< Direct3D 12 query used for flush, can be a null pointer	// TODO(co) Direct3D 12 update
		//[-------------------------------------------------------]
		//[ Output-merger (OM) stage                              ]
		//[-------------------------------------------------------]
		Renderer::IRenderTarget* mRenderTarget;		///< Currently set render target (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant Direct3D 11 calls
		D3D12_PRIMITIVE_TOPOLOGY mD3D12PrimitiveTopology;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Direct3D12RuntimeLinking.h         ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Macros & definitions                                  ]
	//[-------------------------------------------------------]
	// Redirect D3D12* and D3DX11* function calls to funcPtr_D3D12* and funcPtr_D3DX11*
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
	#ifdef RENDERER_DEBUG
		FNDEF_D3D12(HRESULT,	D3D12GetDebugInterface,	(_In_ REFIID riid, _COM_Outptr_opt_ void** ppvDebug));
		#define D3D12GetDebugInterface	FNPTR(D3D12GetDebugInterface)
	#endif

	//[-------------------------------------------------------]
	//[ D3DX11 functions                                      ]
	//[-------------------------------------------------------]
	#define FNDEF_D3DX11(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	// FNDEF_D3DX11(HRESULT,	D3DX11FilterTexture,		(ID3D12DeviceContext *, ID3D12Resource *, UINT, UINT));	// TODO(co) Direct3D 12 update
	// #define D3DX11FilterTexture		FNPTR(D3DX11FilterTexture)	// TODO(co) Direct3D 12 update

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
	*
	*  @todo
	*    - TODO(co) Looks like there's no "D3DX12", so we stick to "D3DX11" for now
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
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*/
		inline explicit Direct3D12RuntimeLinking(Direct3D12Renderer& direct3D12Renderer) :
			mDirect3D12Renderer(direct3D12Renderer),
			mDxgiSharedLibrary(nullptr),
			mD3D12SharedLibrary(nullptr),
			mD3DX11SharedLibrary(nullptr),
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
			if (nullptr != mD3DX11SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3DX11SharedLibrary));
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
					// Load the DXGI, D3D12, D3DX11 and D3DCompiler entry points
					mEntryPointsRegistered = (loadDxgiEntryPoints() && loadD3D12EntryPoints() && loadD3DX11EntryPoints() && loadD3DCompilerEntryPoints());
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
					mD3DX11SharedLibrary = ::LoadLibraryExA("d3dx11_43.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr != mD3DX11SharedLibrary)
					{
						mD3DCompilerSharedLibrary = ::LoadLibraryExA("D3DCompiler_47.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
						if (nullptr == mD3DCompilerSharedLibrary)
						{
							RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"D3DCompiler_47.dll\"")
						}
					}
					else
					{
						RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"d3dx11_43.dll\"")
					}
				}
				else
				{
					RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"d3d12.dll\"")
				}
			}
			else
			{
				RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to load in the shared Direct3D 12 library \"dxgi.dll\"")
			}

			// Done
			return (nullptr != mDxgiSharedLibrary && nullptr != mD3D12SharedLibrary && nullptr != mD3DX11SharedLibrary && nullptr != mD3DCompilerSharedLibrary);
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
			#define IMPORT_FUNC(funcName)																																									\
				if (result)																																													\
				{																																															\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mDxgiSharedLibrary), #funcName);																									\
					if (nullptr != symbol)																																									\
					{																																														\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																																	\
					}																																														\
					else																																													\
					{																																														\
						wchar_t moduleFilename[MAX_PATH];																																					\
						moduleFilename[0] = '\0';																																							\
						::GetModuleFileNameW(static_cast<HMODULE>(mDxgiSharedLibrary), moduleFilename, MAX_PATH);																							\
						RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 DXGI shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																										\
					}																																														\
				}

			// Load the entry points
			IMPORT_FUNC(CreateDXGIFactory1);

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
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3D12SharedLibrary), #funcName);																							\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3D12SharedLibrary), moduleFilename, MAX_PATH);																					\
						RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			IMPORT_FUNC(D3D12CreateDevice);
			IMPORT_FUNC(D3D12SerializeRootSignature);
			#ifdef RENDERER_DEBUG
				IMPORT_FUNC(D3D12GetDebugInterface);
			#endif

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the D3DX11 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3DX11EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3DX11SharedLibrary), #funcName);																							\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3DX11SharedLibrary), moduleFilename, MAX_PATH);																					\
						RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			// IMPORT_FUNC(D3DX11FilterTexture);	// TODO(co) Direct3D 12 update

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
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3DCompilerSharedLibrary), #funcName);																					\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3DCompilerSharedLibrary), moduleFilename, MAX_PATH);																			\
						RENDERER_LOG(mDirect3D12Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 12 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			IMPORT_FUNC(D3DCompile);
			IMPORT_FUNC(D3DCreateBlob);

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D12Renderer&	mDirect3D12Renderer;		///< Owner Direct3D 12 renderer instance
		void*				mDxgiSharedLibrary;			///< DXGI shared library, can be a null pointer
		void*				mD3D12SharedLibrary;		///< D3D12 shared library, can be a null pointer
		void*				mD3DX11SharedLibrary;		///< D3DX11 shared library, can be a null pointer
		void*				mD3DCompilerSharedLibrary;	///< D3DCompiler shared library, can be a null pointer
		bool				mEntryPointsRegistered;		///< Entry points successfully registered?
		bool				mInitialized;				///< Already initialized?


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
	#define RENDERER_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) constexpr GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
	RENDERER_DEFINE_GUID(WKPDID_D3DDebugObjectName, 0x429b8c22, 0x9188, 0x4b0c, 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00);




	//[-------------------------------------------------------]
	//[ Global functions                                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Creates, loads and compiles a shader from source code
	*
	*  @param[in] context
	*    Renderer context
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
	[[nodiscard]] ID3DBlob* loadShaderFromSourcecode(const Renderer::Context& context, const char* shaderModel, const char* sourceCode, const char* entryPoint, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel)
	{
		// Sanity checks
		RENDERER_ASSERT(context, nullptr != shaderModel, "Invalid Direct3D 12 shader model")
		RENDERER_ASSERT(context, nullptr != sourceCode, "Invalid Direct3D 12 shader source code")

		// Get compile flags
		UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
		switch (optimizationLevel)
		{
			case Renderer::IShaderLanguage::OptimizationLevel::Debug:
				compileFlags |= D3DCOMPILE_DEBUG;
				compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::None:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::Low:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::Medium:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::High:
				compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
				compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::Ultra:
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
				if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), static_cast<char*>(errorD3dBlob->GetBufferPointer())))
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

	void handleDeviceLost(const Direct3D12Renderer& direct3D12Renderer, HRESULT result)
	{
		// If the device was removed either by a disconnection or a driver upgrade, we must recreate all device resources
		if (DXGI_ERROR_DEVICE_REMOVED == result || DXGI_ERROR_DEVICE_RESET == result)
		{
			if (DXGI_ERROR_DEVICE_REMOVED == result)
			{
				result = direct3D12Renderer.getD3D12Device()->GetDeviceRemovedReason();
			}
			RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Direct3D 12 device lost on present: Reason code 0x%08X", static_cast<unsigned int>(result))

			// TODO(co) Add device lost handling if needed. Probably more complex to recreate all device resources.
		}
	}




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Mapping.h                          ]
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
		//[ Renderer::VertexAttributeFormat and semantic          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::VertexAttributeFormat" to Direct3D 12 format
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to map
		*
		*  @return
		*    Direct3D 12 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D12Format(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R32_FLOAT,			// Renderer::VertexAttributeFormat::FLOAT_1
				DXGI_FORMAT_R32G32_FLOAT,		// Renderer::VertexAttributeFormat::FLOAT_2
				DXGI_FORMAT_R32G32B32_FLOAT,	// Renderer::VertexAttributeFormat::FLOAT_3
				DXGI_FORMAT_R32G32B32A32_FLOAT,	// Renderer::VertexAttributeFormat::FLOAT_4
				DXGI_FORMAT_R8G8B8A8_UNORM,		// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				DXGI_FORMAT_R8G8B8A8_UINT,		// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				DXGI_FORMAT_R16G16_SINT,		// Renderer::VertexAttributeFormat::SHORT_2
				DXGI_FORMAT_R16G16B16A16_SINT,	// Renderer::VertexAttributeFormat::SHORT_4
				DXGI_FORMAT_R32_UINT			// Renderer::VertexAttributeFormat::UINT_1
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		//[-------------------------------------------------------]
		//[ Renderer::BufferUsage                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::BufferUsage" to Direct3D 12 usage and CPU access flags
		*
		*  @param[in]  bufferUsage
		*    "Renderer::BufferUsage" to map
		*  @param[out] cpuAccessFlags
		*    Receives the CPU access flags
		*
		*  @return
		*    Direct3D 12 usage // TODO(co) Use correct Direct3D 12 type
		*/
		[[nodiscard]] static uint32_t getDirect3D12UsageAndCPUAccessFlags([[maybe_unused]] Renderer::BufferUsage bufferUsage, [[maybe_unused]] uint32_t& cpuAccessFlags)
		{
			// TODO(co) Direct3D 12
			/*
			// Direct3D 12 only supports a subset of the OpenGL usage indications
			// -> See "D3D12_USAGE enumeration "-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/ff476259%28v=vs.85%29.aspx
			switch (bufferUsage)
			{
				case Renderer::BufferUsage::STREAM_DRAW:
				case Renderer::BufferUsage::STREAM_COPY:
				case Renderer::BufferUsage::STATIC_DRAW:
				case Renderer::BufferUsage::STATIC_COPY:
					cpuAccessFlags = 0;
					return D3D12_USAGE_IMMUTABLE;

				case Renderer::BufferUsage::STREAM_READ:
				case Renderer::BufferUsage::STATIC_READ:
					cpuAccessFlags = D3D12_CPU_ACCESS_READ;
					return D3D12_USAGE_STAGING;

				case Renderer::BufferUsage::DYNAMIC_DRAW:
				case Renderer::BufferUsage::DYNAMIC_COPY:
					cpuAccessFlags = D3D12_CPU_ACCESS_WRITE;
					return D3D12_USAGE_DYNAMIC;

				default:
				case Renderer::BufferUsage::DYNAMIC_READ:
					cpuAccessFlags = 0;
					return D3D12_USAGE_DEFAULT;
			}
			*/
			return 0;
		}

		//[-------------------------------------------------------]
		//[ Renderer::IndexBufferFormat                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::IndexBufferFormat" to Direct3D 12 format
		*
		*  @param[in] indexBufferFormat
		*    "Renderer::IndexBufferFormat" to map
		*
		*  @return
		*    Direct3D 12 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D12Format(Renderer::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R32_UINT,	// Renderer::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Direct3D 12
				DXGI_FORMAT_R16_UINT,	// Renderer::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				DXGI_FORMAT_R32_UINT	// Renderer::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureFormat                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureFormat" to Direct3D 12 format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    Direct3D 12 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D12Format(Renderer::TextureFormat::Enum textureFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R8_UNORM,				// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				DXGI_FORMAT_B8G8R8X8_UNORM,			// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue
				DXGI_FORMAT_R8G8B8A8_UNORM,			// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,	// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_B8G8R8A8_UNORM,			// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				DXGI_FORMAT_R11G11B10_FLOAT,		// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent
				DXGI_FORMAT_R16G16B16A16_FLOAT,		// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_R32G32B32A32_FLOAT,		// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				DXGI_FORMAT_BC1_UNORM,				// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				DXGI_FORMAT_BC1_UNORM_SRGB,			// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC2_UNORM,				// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC2_UNORM_SRGB,			// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC3_UNORM,				// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				DXGI_FORMAT_BC3_UNORM_SRGB,			// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear
				DXGI_FORMAT_BC4_UNORM,				// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block)
				DXGI_FORMAT_BC5_UNORM,				// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
				DXGI_FORMAT_UNKNOWN,				// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 12
				DXGI_FORMAT_R16_UNORM,				// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				DXGI_FORMAT_R32_UINT,				// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				DXGI_FORMAT_R32_FLOAT,				// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				DXGI_FORMAT_D32_FLOAT,				// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				DXGI_FORMAT_R16G16_SNORM,			// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_R16G16_FLOAT,			// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_UNKNOWN					// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/ResourceGroup.h                    ]
	//[-------------------------------------------------------]
	// TODO(co) Direct3D 12 resource group creation isn't implemented, yet




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/RootSignature.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(Direct3D12Renderer& direct3D12Renderer, const Renderer::RootSignature& rootSignature) :
			IRootSignature(direct3D12Renderer),
			mD3D12RootSignature(nullptr)
		{
			const Renderer::Context& context = direct3D12Renderer.getContext();

			// Create temporary Direct3D 12 root signature instance data
			// -> "Renderer::RootSignature" is not identical to "D3D12_ROOT_SIGNATURE_DESC" because it had to be extended by information required by OpenGL
			D3D12_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc;
			{
				{ // Copy the parameter data
					const uint32_t numberOfRootParameters = rootSignature.numberOfParameters;
					d3d12RootSignatureDesc.NumParameters = numberOfRootParameters;
					if (numberOfRootParameters > 0)
					{
						d3d12RootSignatureDesc.pParameters = RENDERER_MALLOC_TYPED(context, D3D12_ROOT_PARAMETER, numberOfRootParameters);
						D3D12_ROOT_PARAMETER* d3dRootParameters = const_cast<D3D12_ROOT_PARAMETER*>(d3d12RootSignatureDesc.pParameters);
						for (uint32_t parameterIndex = 0; parameterIndex < numberOfRootParameters; ++parameterIndex)
						{
							D3D12_ROOT_PARAMETER& d3dRootParameter = d3dRootParameters[parameterIndex];
							const Renderer::RootParameter& rootParameter = rootSignature.parameters[parameterIndex];

							// Copy the descriptor table data and determine the shader visibility of the Direct3D 12 root parameter
							uint32_t shaderVisibility = ~0u;
							if (D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE == d3dRootParameter.ParameterType)
							{
								const uint32_t numberOfDescriptorRanges = d3dRootParameter.DescriptorTable.NumDescriptorRanges;
								d3dRootParameter.DescriptorTable.NumDescriptorRanges = numberOfDescriptorRanges;
								d3dRootParameter.DescriptorTable.pDescriptorRanges = RENDERER_MALLOC_TYPED(context, D3D12_DESCRIPTOR_RANGE, numberOfDescriptorRanges);

								// "Renderer::DescriptorRange" is not identical to "D3D12_DESCRIPTOR_RANGE" because it had to be extended by information required by OpenGL
								for (uint32_t descriptorRangeIndex = 0; descriptorRangeIndex < numberOfDescriptorRanges; ++descriptorRangeIndex)
								{
									const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[descriptorRangeIndex];
									memcpy(const_cast<D3D12_DESCRIPTOR_RANGE*>(&d3dRootParameter.DescriptorTable.pDescriptorRanges[descriptorRangeIndex]), &descriptorRange, sizeof(D3D12_DESCRIPTOR_RANGE));
									if (~0u == shaderVisibility)
									{
										shaderVisibility = static_cast<uint32_t>(descriptorRange.shaderVisibility);
									}
									else if (shaderVisibility != static_cast<uint32_t>(descriptorRange.shaderVisibility))
									{
										shaderVisibility = static_cast<uint32_t>(Renderer::ShaderVisibility::ALL);
									}
								}
							}
							if (~0u == shaderVisibility)
							{
								shaderVisibility = static_cast<uint32_t>(Renderer::ShaderVisibility::ALL);
							}

							// Set root parameter
							d3dRootParameters->ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(rootParameter.parameterType);
							d3dRootParameters->ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shaderVisibility);
						}
					}
					else
					{
						d3d12RootSignatureDesc.pParameters = nullptr;
					}
				}

				{ // Copy the static sampler data
				  // -> "Renderer::StaticSampler" is identical to "D3D12_STATIC_SAMPLER_DESC" so there's no additional mapping work to be done in here
					const uint32_t numberOfStaticSamplers = rootSignature.numberOfStaticSamplers;
					d3d12RootSignatureDesc.NumStaticSamplers = numberOfStaticSamplers;
					if (numberOfStaticSamplers > 0)
					{
						d3d12RootSignatureDesc.pStaticSamplers = RENDERER_MALLOC_TYPED(context, D3D12_STATIC_SAMPLER_DESC, numberOfStaticSamplers);
						memcpy(const_cast<D3D12_STATIC_SAMPLER_DESC*>(d3d12RootSignatureDesc.pStaticSamplers), rootSignature.staticSamplers, sizeof(Renderer::StaticSampler) * numberOfStaticSamplers);
					}
					else
					{
						d3d12RootSignatureDesc.pStaticSamplers = nullptr;
					}
				}

				// Copy flags
				// -> "Renderer::RootSignatureFlags" is identical to "D3D12_ROOT_SIGNATURE_FLAGS" so there's no additional mapping work to be done in here
				d3d12RootSignatureDesc.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(rootSignature.flags);
			}

			{ // Create the Direct3D 12 root signature instance
				ID3DBlob* d3dBlobSignature = nullptr;
				ID3DBlob* d3dBlobError = nullptr;
				if (SUCCEEDED(D3D12SerializeRootSignature(&d3d12RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &d3dBlobSignature, &d3dBlobError)))
				{
					if (FAILED(direct3D12Renderer.getD3D12Device()->CreateRootSignature(0, d3dBlobSignature->GetBufferPointer(), d3dBlobSignature->GetBufferSize(), IID_PPV_ARGS(&mD3D12RootSignature))))
					{
						RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 root signature instance")
					}
					d3dBlobSignature->Release();
				}
				else
				{
					RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 root signature instance")
					d3dBlobError->Release();
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
						RENDERER_FREE(context, const_cast<D3D12_DESCRIPTOR_RANGE*>(d3d12RootParameter.DescriptorTable.pDescriptorRanges));
					}
				}
				RENDERER_FREE(context, const_cast<D3D12_ROOT_PARAMETER*>(d3d12RootSignatureDesc.pParameters));
			}
			RENDERER_FREE(context, const_cast<D3D12_STATIC_SAMPLER_DESC*>(d3d12RootSignatureDesc.pStaticSamplers));

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Root signature");
			#endif
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
		}

		/**
		*  @brief
		*    Return the Direct3D 12 root signature
		*
		*  @return
		*    The Direct3D 12 root signature, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12RootSignature *getD3D12RootSignature() const
		{
			return mD3D12RootSignature;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRootSignature methods       ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Renderer::IResourceGroup* createResourceGroup([[maybe_unused]] uint32_t rootParameterIndex, [[maybe_unused]] uint32_t numberOfResources, [[maybe_unused]] Renderer::IResource** resources, [[maybe_unused]] Renderer::ISamplerState** samplerStates = nullptr) override
		{
			// TODO(co) Implement resource group
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 12 resource group creation isn't implemented, yet")
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12RootSignature)
				{
					mD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
					mD3D12RootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
				}
			}
		#endif


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
		ID3D12RootSignature* mD3D12RootSignature;	///< Direct3D 12 root signature, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Buffer/IndexBuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 index buffer object (IBO, "element array buffer" in OpenGL terminology) class
	*/
	class IndexBuffer final : public Renderer::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(Direct3D12Renderer& direct3D12Renderer, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Renderer::BufferUsage bufferUsage, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IIndexBuffer(direct3D12Renderer),
			mD3D12Resource(nullptr)
		{
			// "Renderer::IndexBufferFormat::UnsignedChar" is not supported by Direct3D 12
			// TODO(co) Check this, there's "DXGI_FORMAT_R8_UINT" which might work in Direct3D 12
			if (Renderer::IndexBufferFormat::UNSIGNED_CHAR == indexBufferFormat)
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "\"Renderer::IndexBufferFormat::UNSIGNED_CHAR\" is not supported by Direct3D 12")
				mD3D12IndexBufferView.BufferLocation = 0;
				mD3D12IndexBufferView.SizeInBytes	 = 0;
				mD3D12IndexBufferView.Format		 = DXGI_FORMAT_UNKNOWN;
			}
			else
			{
				// Begin debug event
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D12Renderer)

				// TODO(co) This is only meant for the Direct3D 12 renderer backend kickoff.
				// Note: using upload heaps to transfer static data like vert buffers is not 
				// recommended. Every time the GPU needs it, the upload heap will be marshalled 
				// over. Please read up on Default Heap usage. An upload heap is used here for 
				// code simplicity and because there are very few verts to actually transfer.

				// TODO(co) Add buffer usage setting support

				const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
				const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfBytes);
				if (SUCCEEDED(direct3D12Renderer.getD3D12Device()->CreateCommittedResource(
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
							RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to map Direct3D 12 index buffer")
						}
					}

					// Fill the Direct3D 12 index buffer view
					mD3D12IndexBufferView.BufferLocation = mD3D12Resource->GetGPUVirtualAddress();
					mD3D12IndexBufferView.SizeInBytes	 = numberOfBytes;
					mD3D12IndexBufferView.Format		 = Mapping::getDirect3D12Format(indexBufferFormat);
				}
				else
				{
					RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create Direct3D 12 index buffer resource")
					mD3D12IndexBufferView.BufferLocation = 0;
					mD3D12IndexBufferView.SizeInBytes	 = 0;
					mD3D12IndexBufferView.Format		 = DXGI_FORMAT_UNKNOWN;
				}

				// End debug event
				RENDERER_END_DEBUG_EVENT(&direct3D12Renderer)
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("");
			#endif
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
		[[nodiscard]] inline ID3D12Resource* getID3D12Resource() const
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
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "IBO", 6);	// 6 = "IBO: " including terminating zero!
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
	//[ Direct3D12Renderer/Buffer/VertexBuffer.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
	*/
	class VertexBuffer final : public Renderer::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(Direct3D12Renderer& direct3D12Renderer, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Renderer::BufferUsage bufferUsage) :
			IVertexBuffer(direct3D12Renderer),
			mNumberOfBytes(numberOfBytes),
			mD3D12Resource(nullptr)
		{
			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D12Renderer)

			// TODO(co) This is only meant for the Direct3D 12 renderer backend kickoff.
			// Note: using upload heaps to transfer static data like vert buffers is not 
			// recommended. Every time the GPU needs it, the upload heap will be marshalled 
			// over. Please read up on Default Heap usage. An upload heap is used here for 
			// code simplicity and because there are very few verts to actually transfer.

			// TODO(co) Add buffer usage setting support

			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mNumberOfBytes);
			if (SUCCEEDED(direct3D12Renderer.getD3D12Device()->CreateCommittedResource(
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
						RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to map Direct3D 12 vertex buffer")
					}
				}
			}
			else
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create Direct3D 12 vertex buffer resource")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D12Renderer)
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
		[[nodiscard]] inline ID3D12Resource *getID3D12Resource() const
		{
			return mD3D12Resource;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "VBO", 6);	// 6 = "VBO: " including terminating zero!
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
	//[ Direct3D12Renderer/Buffer/VertexArray.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 vertex array class
	*/
	class VertexArray final : public Renderer::IVertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
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
		VertexArray(Direct3D12Renderer& direct3D12Renderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			IVertexArray(direct3D12Renderer, id),
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
				const Renderer::Context& context = direct3D12Renderer.getContext();
				mD3D12VertexBufferViews = RENDERER_MALLOC_TYPED(context, D3D12_VERTEX_BUFFER_VIEW, mNumberOfSlots);
				mVertexBuffers = RENDERER_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					D3D12_VERTEX_BUFFER_VIEW* currentD3D12VertexBufferView = mD3D12VertexBufferViews;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentD3D12VertexBufferView, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used renderer?
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						(*currentVertexBuffer)->addReference();
						currentD3D12VertexBufferView->BufferLocation = (*currentVertexBuffer)->getID3D12Resource()->GetGPUVirtualAddress();
						currentD3D12VertexBufferView->SizeInBytes = (*currentVertexBuffer)->getNumberOfBytes();
					}
				}

				{ // Gather slot related data
					const Renderer::VertexAttribute* attribute = vertexAttributes.attributes;
					const Renderer::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
					for (; attribute < attributesEnd;  ++attribute)
					{
						mD3D12VertexBufferViews[attribute->inputSlot].StrideInBytes = attribute->strideInBytes;
					}
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("VAO");
			#endif
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
			const Renderer::Context& context = getRenderer().getContext();
			RENDERER_FREE(context, mD3D12VertexBufferViews);

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
				RENDERER_FREE(context, mVertexBuffers);
			}

			// Free the unique compact vertex array ID
			static_cast<Direct3D12Renderer&>(getRenderer()).VertexArrayMakeId.DestroyID(getId());
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
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexArray, this);
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
		IndexBuffer				 *mIndexBuffer;				///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Direct3D 12 input slots
		UINT					  mNumberOfSlots;			///< Number of used Direct3D 12 input slots
		D3D12_VERTEX_BUFFER_VIEW* mD3D12VertexBufferViews;
		// For proper vertex buffer reference counter behaviour
		VertexBuffer**			  mVertexBuffers;			///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Buffer/TextureBuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 texture buffer object (TBO) class
	*/
	class TextureBuffer final : public Renderer::ITextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBuffer(Direct3D12Renderer& direct3D12Renderer, [[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data, [[maybe_unused]] Renderer::BufferUsage bufferUsage, [[maybe_unused]] Renderer::TextureFormat::Enum textureFormat) :
			ITextureBuffer(direct3D12Renderer)
			// TODO(co) Direct3D 12 update
		//	mD3D12Buffer(nullptr),
		//	mD3D12ShaderResourceViewTexture(nullptr)
		{
			// Sanity check
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (numberOfBytes % Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The Direct3D 12 texture buffer size must be a multiple of the selected texture format bytes per texel")

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
					FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateBuffer(&d3d12BufferDesc, &d3d12SubresourceData, &mD3D12Buffer));
				}
				else
				{
					// Create the Direct3D 12 constant buffer
					FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateBuffer(&d3d12BufferDesc, nullptr, &mD3D12Buffer));
				}
			}

			{ // Shader resource view part
				// Direct3D 12 shader resource view description
				D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
				d3d12ShaderResourceViewDesc.Format				 = Mapping::getDirect3D12Format(textureFormat);
				d3d12ShaderResourceViewDesc.ViewDimension		 = D3D12_SRV_DIMENSION_BUFFER;
				d3d12ShaderResourceViewDesc.Buffer.ElementOffset = 0;
				d3d12ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 12 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateShaderResourceView(mD3D12Buffer, &d3d12ShaderResourceViewDesc, &mD3D12ShaderResourceViewTexture));
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("");
			#endif
			*/
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureBuffer() override
		{
			// TODO(co) Direct3D 12 update
			/*
			// Release the used resources
			if (nullptr != mD3D12ShaderResourceViewTexture)
			{
				mD3D12ShaderResourceViewTexture->Release();
				mD3D12ShaderResourceViewTexture = nullptr;
			}
			if (nullptr != mD3D12Buffer)
			{
				mD3D12Buffer->Release();
				mD3D12Buffer = nullptr;
			}
			*/
		}

		/**
		*  @brief
		*    Return the Direct3D texture buffer instance
		*
		*  @return
		*    The Direct3D texture buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12Buffer *getD3D12Buffer() const
		//{
		//	return mD3D12Buffer;
		//}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12ShaderResourceView *getD3D12ShaderResourceView() const
		//{
		//	return mD3D12ShaderResourceViewTexture;
		//}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char*) override
			{
				// TODO(co) Direct3D 12 update
				/*
				RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "TBO", 6);	// 6 = "TBO: " including terminating zero!

				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12ShaderResourceViewTexture)
				{
					FAILED_DEBUG_BREAK(mD3D12ShaderResourceViewTexture->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12ShaderResourceViewTexture->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
				if (nullptr != mD3D12Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
			*/
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// TODO(co) Direct3D 12 update
		//ID3D12Buffer			 *mD3D12Buffer;						///< Direct3D texture buffer instance, can be a null pointer
		//ID3D12ShaderResourceView *mD3D12ShaderResourceViewTexture;	///< Direct3D 12 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Buffer/StructuredBuffer.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 structured buffer object class
	*/
	class StructuredBuffer final : public Renderer::IStructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the structured buffer, must be valid
		*  @param[in] data
		*    Structured buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] numberOfStructureBytes
		*    Number of structure bytes
		*/
		StructuredBuffer(Direct3D12Renderer& direct3D12Renderer, [[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data, [[maybe_unused]] Renderer::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes) :
			IStructuredBuffer(direct3D12Renderer)
			// TODO(co) Direct3D 12 update
		//	mD3D12Buffer(nullptr),
		//	mD3D12ShaderResourceViewTexture(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The Direct3D 12 structured buffer size must be a multiple of the given number of structure bytes")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The Direct3D 12 structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

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
					FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateBuffer(&d3d12BufferDesc, &d3d12SubresourceData, &mD3D12Buffer));
				}
				else
				{
					// Create the Direct3D 12 constant buffer
					FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateBuffer(&d3d12BufferDesc, nullptr, &mD3D12Buffer));
				}
			}

			{ // Shader resource view part
				// Direct3D 12 shader resource view description
				D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
				d3d12ShaderResourceViewDesc.Format				 = Mapping::getDirect3D12Format(textureFormat);
				d3d12ShaderResourceViewDesc.ViewDimension		 = D3D12_SRV_DIMENSION_BUFFER;
				d3d12ShaderResourceViewDesc.Buffer.ElementOffset = 0;
				d3d12ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 12 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateShaderResourceView(mD3D12Buffer, &d3d12ShaderResourceViewDesc, &mD3D12ShaderResourceViewTexture));
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("");
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
			if (nullptr != mD3D12ShaderResourceViewTexture)
			{
				mD3D12ShaderResourceViewTexture->Release();
				mD3D12ShaderResourceViewTexture = nullptr;
			}
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
		//[[nodiscard]] inline ID3D12Buffer *getD3D12Buffer() const
		//{
		//	return mD3D12Buffer;
		//}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12ShaderResourceView *getD3D12ShaderResourceView() const
		//{
		//	return mD3D12ShaderResourceViewTexture;
		//}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char*) override
			{
				// TODO(co) Direct3D 12 update
				/*
				RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "TBO", 6);	// 6 = "TBO: " including terminating zero!

				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12ShaderResourceViewTexture)
				{
					FAILED_DEBUG_BREAK(mD3D12ShaderResourceViewTexture->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12ShaderResourceViewTexture->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
				if (nullptr != mD3D12Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
			*/
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
		//ID3D12Buffer			 *mD3D12Buffer;						///< Direct3D texture buffer instance, can be a null pointer
		//ID3D12ShaderResourceView *mD3D12ShaderResourceViewTexture;	///< Direct3D 12 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Buffer/IndirectBuffer.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 indirect buffer object class
	*/
	class IndirectBuffer final : public Renderer::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Renderer::IndirectBufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		IndirectBuffer(Direct3D12Renderer& direct3D12Renderer, uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t indirectBufferFlags, [[maybe_unused]] Renderer::BufferUsage bufferUsage) :
			IIndirectBuffer(direct3D12Renderer),
			mNumberOfBytes(numberOfBytes),
			mData(nullptr)
			// TODO(co) Direct3D 12 update
		//	mD3D12Buffer(nullptr),
		//	mD3D12ShaderResourceViewIndirect(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Direct3D 12 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), !((indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Direct3D 12 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawArguments)) == 0, "Direct3D 12 indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawIndexedArguments)) == 0, "Direct3D 12 indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// TODO(co) Direct3D 12 update
			if (mNumberOfBytes > 0)
			{
				mData = RENDERER_MALLOC_TYPED(direct3D12Renderer.getContext(), uint8_t, mNumberOfBytes);
				if (nullptr != data)
				{
					memcpy(mData, data, mNumberOfBytes);
				}
			}
			else
			{
				RENDERER_ASSERT(direct3D12Renderer.getContext(), nullptr == data, "Invalid Direct3D 12 indirect buffer data")
			}

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
					FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateBuffer(&d3d12BufferDesc, &d3d12SubresourceData, &mD3D12Buffer));
				}
				else
				{
					// Create the Direct3D 12 constant buffer
					FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateBuffer(&d3d12BufferDesc, nullptr, &mD3D12Buffer));
				}
			}

			{ // Shader resource view part
				// Direct3D 12 shader resource view description
				D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
				d3d12ShaderResourceViewDesc.Format				 = Mapping::getDirect3D12Format(textureFormat);
				d3d12ShaderResourceViewDesc.ViewDimension		 = D3D12_SRV_DIMENSION_BUFFER;
				d3d12ShaderResourceViewDesc.Buffer.ElementOffset = 0;
				d3d12ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 12 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D12Renderer.getD3D12Device()->CreateShaderResourceView(mD3D12Buffer, &d3d12ShaderResourceViewDesc, &mD3D12ShaderResourceViewTexture));
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("");
			#endif
			*/
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~IndirectBuffer() override
		{
			RENDERER_FREE(getRenderer().getContext(), mData);

			// TODO(co) Direct3D 12 update
			/*
			// Release the used resources
			if (nullptr != mD3D12ShaderResourceViewIndirect)
			{
				mD3D12ShaderResourceViewIndirect->Release();
				mD3D12ShaderResourceViewIndirect = nullptr;
			}
			if (nullptr != mD3D12Buffer)
			{
				mD3D12Buffer->Release();
				mD3D12Buffer = nullptr;
			}
			*/
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

		/**
		*  @brief
		*    Return the Direct3D indirect buffer instance
		*
		*  @return
		*    The Direct3D indirect buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12Buffer *getD3D12Buffer() const
		//{
		//	return mD3D12Buffer;
		//}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12ShaderResourceView *getD3D12ShaderResourceView() const
		//{
		//	return mD3D12ShaderResourceViewIndirect;
		//}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char*) override
			{
				// TODO(co) Direct3D 12 update
				/*
				RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "IndirectBufferObject", 23);	// 23 = "IndirectBufferObject: " including terminating zero

				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12ShaderResourceViewIndirect)
				{
					FAILED_DEBUG_BREAK(mD3D12ShaderResourceViewIndirect->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12ShaderResourceViewIndirect->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
				if (nullptr != mD3D12Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
			*/
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IIndirectBuffer methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const uint8_t* getEmulationData() const override
		{
			return mData;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndirectBuffer, this);
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
		uint32_t				  mNumberOfBytes;
		uint8_t*				  mData;							///< Indirect buffer data, can be a null pointer
		// TODO(co) Direct3D 12 update
		//ID3D12Buffer			 *mD3D12Buffer;							///< Direct3D indirect buffer instance, can be a null pointer
		//ID3D12ShaderResourceView *mD3D12ShaderResourceViewIndirect;	///< Direct3D 12 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Buffer/UniformBuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
	*/
	class UniformBuffer final : public Renderer::IUniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(Direct3D12Renderer& direct3D12Renderer, uint32_t numberOfBytes, const void* data, [[maybe_unused]] Renderer::BufferUsage bufferUsage) :
			Renderer::IUniformBuffer(direct3D12Renderer),
			mD3D12Resource(nullptr),
			mD3D12DescriptorHeap(nullptr),
			mMappedData(nullptr)
		{
			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D12Renderer)

			ID3D12Device* d3d12Device = direct3D12Renderer.getD3D12Device();

			// Constant buffer size is required to be 256-byte aligned
			// - See "ID3D12Device::CreateConstantBufferView method": https://msdn.microsoft.com/de-de/library/windows/desktop/dn788659%28v=vs.85%29.aspx
			// - No assert because other renderer APIs have another alignment (DirectX 11 e.g. 16)
			const uint32_t numberOfBytesOnGpu = (numberOfBytes + 255) & ~255;

			// TODO(co) Add buffer usage setting support

			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfBytesOnGpu);
			if (SUCCEEDED(d3d12Device->CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12XResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			{
				// Describe and create a constant buffer view (CBV) descriptor heap.
				// Flags indicate that this descriptor heap can be bound to the pipeline 
				// and that descriptors contained in it can be referenced by a root table.
				D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
				d3d12DescriptorHeapDesc.NumDescriptors = 1;
				d3d12DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeap))))
				{
					// Describe and create a constant buffer view
					D3D12_CONSTANT_BUFFER_VIEW_DESC d3dConstantBufferViewDesc = {};
					d3dConstantBufferViewDesc.BufferLocation = mD3D12Resource->GetGPUVirtualAddress();
					d3dConstantBufferViewDesc.SizeInBytes = numberOfBytesOnGpu;
					d3d12Device->CreateConstantBufferView(&d3dConstantBufferViewDesc, mD3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

					CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU
					if (SUCCEEDED(mD3D12Resource->Map(0, &readRange, reinterpret_cast<void**>(&mMappedData))))
					{
						// Data given?
						if (nullptr != data)
						{
							memcpy(mMappedData, &data, numberOfBytes);
						}
					}
					else
					{
						RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to map Direct3D 12 uniform buffer")
					}
				}
				else
				{
					RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create Direct3D 12 uniform buffer descriptor heap")
				}
			}
			else
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create Direct3D 12 uniform buffer resource")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D12Renderer)
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
			if (nullptr != mD3D12DescriptorHeap)
			{
				mD3D12DescriptorHeap->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D descriptor heap instance
		*
		*  @return
		*    The Direct3D descriptor heap instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
		{
			return mD3D12DescriptorHeap;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "UBO", 6);	// 6 = "UBO: " including terminating zero!

				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
				if (nullptr != mD3D12DescriptorHeap)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBuffer(const UniformBuffer& source) = delete;
		UniformBuffer& operator =(const UniformBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D12Resource*		  mD3D12Resource;
		ID3D12DescriptorHeap* mD3D12DescriptorHeap;
		uint8_t*			  mMappedData;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Buffer/BufferManager.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 buffer manager interface
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
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*/
		inline explicit BufferManager(Direct3D12Renderer& direct3D12Renderer) :
			IBufferManager(direct3D12Renderer)
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
	//[ Public virtual Renderer::IBufferManager methods       ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Renderer::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), VertexBuffer)(static_cast<Direct3D12Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
		}

		[[nodiscard]] inline virtual Renderer::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::IndexBufferFormat::Enum indexBufferFormat = Renderer::IndexBufferFormat::UNSIGNED_SHORT) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndexBuffer)(static_cast<Direct3D12Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage, indexBufferFormat);
		}

		[[nodiscard]] inline virtual Renderer::IVertexArray* createVertexArray(const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, Renderer::IIndexBuffer* indexBuffer = nullptr) override
		{
			// Sanity checks
			#ifdef RENDERER_DEBUG
			{
				const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RENDERER_ASSERT(getRenderer().getContext(), &getRenderer() == &vertexBuffer->vertexBuffer->getRenderer(), "Direct3D 12 error: The given vertex buffer resource is owned by another renderer instance")
				}
			}
			#endif
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == indexBuffer || &getRenderer() == &indexBuffer->getRenderer(), "Direct3D 12 error: The given index buffer resource is owned by another renderer instance")

			// Create vertex array
			uint16_t id = 0;
			if (static_cast<Direct3D12Renderer&>(getRenderer()).VertexArrayMakeId.CreateID(id))
			{
				return RENDERER_NEW(getRenderer().getContext(), VertexArray)(static_cast<Direct3D12Renderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id);
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

		[[nodiscard]] inline virtual Renderer::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t = Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::R32G32B32A32F) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TextureBuffer)(static_cast<Direct3D12Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage, textureFormat);
		}

		[[nodiscard]] inline virtual Renderer::IStructuredBuffer* createStructuredBuffer(uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t bufferFlags, Renderer::BufferUsage bufferUsage, uint32_t numberOfStructureBytes) override
		{
			return RENDERER_NEW(getRenderer().getContext(), StructuredBuffer)(static_cast<Direct3D12Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage, numberOfStructureBytes);
		}

		[[nodiscard]] inline virtual Renderer::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndirectBuffer)(static_cast<Direct3D12Renderer&>(getRenderer()), numberOfBytes, data, indirectBufferFlags, bufferUsage);
		}

		[[nodiscard]] inline virtual Renderer::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid Direct3D 12 buffer flags, uniform buffer can't be used for unordered access")
			// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0, "Invalid Direct3D 12 buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RENDERER_NEW(getRenderer().getContext(), UniformBuffer)(static_cast<Direct3D12Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
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


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Texture/Texture1D.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 1D texture class
	*/
	class Texture1D final : public Renderer::ITexture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] width
		*    Texture width, must be >0
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] data
		*    Texture data, can be a null pointer
		*  @param[in] textureFlags
		*    Texture flags, see "Renderer::TextureFlag::Enum"
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture1D(Direct3D12Renderer& direct3D12Renderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage) :
			ITexture1D(direct3D12Renderer, width),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mD3D12Resource(nullptr),
			mD3D12DescriptorHeap(nullptr)
		{
			// TODO(co) Implement me

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("1D texture");
			#endif
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
			if (nullptr != mD3D12DescriptorHeap)
			{
				mD3D12DescriptorHeap->Release();
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
		*    Return the Direct3D 12 descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 descriptor heap instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
		{
			return mD3D12DescriptorHeap;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D12DescriptorHeap)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture1D, this);
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
		DXGI_FORMAT			  mDxgiFormat;	//// DXGI format
		ID3D12Resource*		  mD3D12Resource;
		ID3D12DescriptorHeap* mD3D12DescriptorHeap;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Texture/Texture2D.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 2D texture class
	*/
	class Texture2D final : public Renderer::ITexture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*  @param[in] optimizedTextureClearValue
		*    Optional optimized texture clear value
		*/
		Texture2D(Direct3D12Renderer& direct3D12Renderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage, uint8_t numberOfMultisamples, const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue) :
			ITexture2D(direct3D12Renderer, width, height),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mD3D12Resource(nullptr),
			mD3D12DescriptorHeap(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D12Renderer.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid Direct3D 12 texture parameters")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid Direct3D 12 texture parameters")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid Direct3D 12 texture parameters")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS), "Invalid Direct3D 12 texture parameters")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Renderer::TextureFlag::RENDER_TARGET), "Invalid Direct3D 12 texture parameters")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 12 texture parameters")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 12 render target textures can't be filled using provided data")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D12Renderer)

			ID3D12Device* d3d12Device = direct3D12Renderer.getD3D12Device();

			// TODO(co) Add buffer usage setting support
			// TODO(co) Add "Renderer::TextureFlag::GENERATE_MIPMAPS" support, also for render target textures

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Describe and create a texture 2D
			D3D12_RESOURCE_DESC d3d12ResourceDesc = {};
			d3d12ResourceDesc.MipLevels = static_cast<UINT16>(numberOfMipmaps);
			d3d12ResourceDesc.Format = static_cast<DXGI_FORMAT>(mDxgiFormat);
			d3d12ResourceDesc.Width = width;
			d3d12ResourceDesc.Height = height;
			d3d12ResourceDesc.Flags = (textureFlags & Renderer::TextureFlag::RENDER_TARGET) ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE;
			d3d12ResourceDesc.DepthOrArraySize = 1;
			d3d12ResourceDesc.SampleDesc.Count = numberOfMultisamples;
			d3d12ResourceDesc.SampleDesc.Quality = 0;
			d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			// If we don't pass a clear value, we later on get the following debug message: "ID3D12CommandList::ClearRenderTargetView: The application did not pass any clear value to resource creation. The clear operation is typically slower as a result; but will still clear to the desired value. [ EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE]"
			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = d3d12ResourceDesc.Format;
			if (nullptr != optimizedTextureClearValue)
			{
				memcpy(depthOptimizedClearValue.Color, optimizedTextureClearValue->color, sizeof(float) * 4);
			}

			// TODO(co) This is just first Direct3D 12 texture test, so don't wonder about the nasty synchronization handling
			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			if (SUCCEEDED(d3d12Device->CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12ResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				(textureFlags & Renderer::TextureFlag::RENDER_TARGET) ? &depthOptimizedClearValue : nullptr,	// Avoid: "Direct3D 12 error: Failed to create texture 2D resourceD3D12 ERROR: ID3D12Device::CreateCommittedResource: pOptimizedClearValue must be NULL when D3D12_RESOURCE_DESC::Dimension is not D3D12_RESOURCE_DIMENSION_BUFFER and neither D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET nor D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL are set in D3D12_RESOURCE_DESC::Flags. [ STATE_CREATION ERROR #815: CREATERESOURCE_INVALIDCLEARVALUE]"
				IID_PPV_ARGS(&mD3D12Resource))))
			/*
			const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
			if (SUCCEEDED(d3d12Device->CreateCommittedResource(
				&d3d12XHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&d3d12ResourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&mD3D12Resource))))
			*/
			{
				// Describe and create a shader resource view (SRV) heap for the texture
				D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
				d3d12DescriptorHeapDesc.NumDescriptors = 1;
				d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				d3d12DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeap))))
				{
					// Upload the texture data?
					if (nullptr != data)
					{
						// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
						if (dataContainsMipmaps)
						{
							// Upload all mipmaps
							for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
							{
								// Upload the current mipmap
								const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
								const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
								FAILED_DEBUG_BREAK(mD3D12Resource->WriteToSubresource(mipmap, nullptr, data, bytesPerRow, bytesPerSlice));

								// Move on to the next mipmap and ensure the size is always at least 1x1
								data = static_cast<const uint8_t*>(data) + bytesPerSlice;
								width = getHalfSize(width);
								height = getHalfSize(height);
							}
						}
						else if (generateMipmaps)
						{
							// TODO(co) Implement me
						}
						else
						{
							// The user only provided us with the base texture, no mipmaps
							const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							FAILED_DEBUG_BREAK(mD3D12Resource->WriteToSubresource(0, nullptr, data, bytesPerRow, bytesPerSlice));
						}
					}

					// TODO(co) This is just first Direct3D 12 texture test, so don't wonder about the nasty synchronization handling
					/*
					if (nullptr != data)
					{
						ID3D12GraphicsCommandList* d3d12GraphicsCommandList = direct3D12Renderer.getD3D12GraphicsCommandList();
					//	direct3D12Renderer.beginScene();

						// Create an upload heap to load the texture onto the GPU. ComPtr's are CPU objects
						// but this heap needs to stay in scope until the GPU work is complete. We will
						// synchronize with the GPU at the end of this method before the ComPtr is destroyed.
						ID3D12Resource* d3d12ResourceTextureUploadHeap = nullptr;

						{ // Upload the texture data
							const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mD3D12Resource, 0, 1);

							// Create the GPU upload buffer
							const CD3DX12_HEAP_PROPERTIES d3d12XHeapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
							const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
							if (SUCCEEDED(d3d12Device->CreateCommittedResource(
								&d3d12XHeapPropertiesUpload,
								D3D12_HEAP_FLAG_NONE,
								&d3d12XResourceDesc,
								D3D12_RESOURCE_STATE_GENERIC_READ,
								nullptr,
								IID_PPV_ARGS(&d3d12ResourceTextureUploadHeap))))
							{
								// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the texture 2D
								D3D12_SUBRESOURCE_DATA textureData = {};
								textureData.pData = data;
								textureData.RowPitch = static_cast<LONG_PTR>(width * Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat));
								textureData.SlicePitch = static_cast<LONG_PTR>(textureData.RowPitch * height);

								UpdateSubresources(d3d12GraphicsCommandList, mD3D12Resource, d3d12ResourceTextureUploadHeap, 0, 0, 1, &textureData);

			//					CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mD3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			//					d3d12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
							}
						}
	
		//				direct3D12Renderer.endScene();
						<Swap Chain>->waitForPreviousFrame();
						d3d12ResourceTextureUploadHeap->Release();
					}
					*/

					// Describe and create a SRV for the texture
					D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
					d3d12ShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					d3d12ShaderResourceViewDesc.Format = d3d12ResourceDesc.Format;
					d3d12ShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					d3d12ShaderResourceViewDesc.Texture2D.MipLevels = numberOfMipmaps;
					d3d12Device->CreateShaderResourceView(mD3D12Resource, &d3d12ShaderResourceViewDesc, mD3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				}
				else
				{
					RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 texture 2D descriptor heap")
				}
			}
			else
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 texture 2D resource")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("2D texture");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D12Renderer)
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
			if (nullptr != mD3D12DescriptorHeap)
			{
				mD3D12DescriptorHeap->Release();
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
		*    Return the Direct3D 12 descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 descriptor heap instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
		{
			return mD3D12DescriptorHeap;
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
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D12DescriptorHeap)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2D, this);
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
		DXGI_FORMAT			  mDxgiFormat;	//// DXGI format
		ID3D12Resource*		  mD3D12Resource;
		ID3D12DescriptorHeap* mD3D12DescriptorHeap;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Texture/Texture2DArray.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 2D array texture class
	*/
	class Texture2DArray final : public Renderer::ITexture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		Texture2DArray(Direct3D12Renderer& direct3D12Renderer, uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) :
			ITexture2DArray(direct3D12Renderer, width, height, numberOfSlices),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mD3D12Resource(nullptr)
		//	mD3D12ShaderResourceViewTexture(nullptr)	// TODO(co) Direct3D 12 update
		{
			// TODO(co) Direct3D 12 update
			/*
			// Sanity checks
			RENDERER_ASSERT(direct3D12Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 12 render target textures can't be filled using provided data")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D12Renderer)

			// Generate mipmaps?
			const bool mipmaps = (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) != 0;

			// Direct3D 12 2D array texture description
			D3D12_TEXTURE2D_DESC d3d12Texture2DDesc;
			d3d12Texture2DDesc.Width			  = width;
			d3d12Texture2DDesc.Height			  = height;
			d3d12Texture2DDesc.MipLevels		  = mipmaps ? 0u : 1u;	// 0 = Let Direct3D 12 allocate the complete mipmap chain for us
			d3d12Texture2DDesc.ArraySize		  = numberOfSlices;
			d3d12Texture2DDesc.Format			  = Mapping::getDirect3D12ResourceFormat(textureFormat);
			d3d12Texture2DDesc.SampleDesc.Count	  = 1;
			d3d12Texture2DDesc.SampleDesc.Quality = 0;
			d3d12Texture2DDesc.Usage			  = static_cast<D3D12_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d12Texture2DDesc.BindFlags		  = D3D12_BIND_SHADER_RESOURCE;
			d3d12Texture2DDesc.CPUAccessFlags	  = (Renderer::TextureUsage::DYNAMIC == textureUsage) ? D3D12_CPU_ACCESS_WRITE : 0u;
			d3d12Texture2DDesc.MiscFlags		  = 0;

			// Use this texture as render target?
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				d3d12Texture2DDesc.BindFlags |= D3D12_BIND_RENDER_TARGET;
			}

			// Create the Direct3D 12 2D array texture instance
			// -> Do not provide the data at once or creating mipmaps will get somewhat complicated
			ID3D12Texture2D *d3d12Texture2D = nullptr;
			direct3D12Renderer.getD3D12Device()->CreateTexture2D(&d3d12Texture2DDesc, nullptr, &d3d12Texture2D);
			if (nullptr != d3d12Texture2D)
			{
				// Calculate the number of mipmaps
				const uint32_t numberOfMipmaps = mipmaps ? getNumberOfMipmaps(width, height) : 1;

				// Data given?
				if (nullptr != data)
				{
					{ // Update Direct3D 12 subresource data of the base-map
						const uint32_t  bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t  bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						const uint8_t  *dataCurrent   = static_cast<uint8_t*>(data);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice, dataCurrent += bytesPerSlice)
						{
							direct3D12Renderer.getD3D12DeviceContext()->UpdateSubresource(d3d12Texture2D, D3D12CalcSubresource(0, arraySlice, numberOfMipmaps), nullptr, dataCurrent, bytesPerRow, bytesPerSlice);
						}
					}

					// Let Direct3D 12 generate the mipmaps for us automatically?
					if (mipmaps)
					{
						// TODO(co) Direct3D 12 update
						// D3DX12FilterTexture(direct3D12Renderer.getD3D12DeviceContext(), d3d12Texture2D, 0, D3DX12_DEFAULT);
					}
				}

				// Direct3D 12 shader resource view description
				D3D12_SHADER_RESOURCE_VIEW_DESC d3d12ShaderResourceViewDesc = {};
				d3d12ShaderResourceViewDesc.Format							= d3d12Texture2DDesc.Format;
				d3d12ShaderResourceViewDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				d3d12ShaderResourceViewDesc.Texture2DArray.MipLevels		= numberOfMipmaps;
				d3d12ShaderResourceViewDesc.Texture2DArray.MostDetailedMip	= 0;
				d3d12ShaderResourceViewDesc.Texture2DArray.ArraySize		= numberOfSlices;

				// Create the Direct3D 12 shader resource view instance
				direct3D12Renderer.getD3D12Device()->CreateShaderResourceView(d3d12Texture2D, &d3d12ShaderResourceViewDesc, &mD3D12ShaderResourceViewTexture);

				// Release the Direct3D 12 2D array texture instance
				d3d12Texture2D->Release();
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("2D texture array");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D12Renderer)
			*/
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2DArray() override
		{
			// TODO(co) Direct3D 12 update
			/*
			if (nullptr != mD3D12ShaderResourceViewTexture)
			{
				mD3D12ShaderResourceViewTexture->Release();
			}
			if (nullptr != mD3D12Texture2D)
			{
				mD3D12Texture2D->Release();
			}
			*/
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
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 12 resource
		*      view by e.g. assigning another Direct3D 12 resource to it
		*/
		// TODO(co) Direct3D 12 update
		//[[nodiscard]] inline ID3D12ShaderResourceView *getD3D12ShaderResourceView() const
		//{
		//	return mD3D12ShaderResourceViewTexture;
		//}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char*) override
			{
				// TODO(co) Direct3D 12 update
				/*
				// Valid Direct3D 12 shader resource view?
				if (nullptr != mD3D12ShaderResourceViewTexture)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					mD3D12ShaderResourceViewTexture->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
					mD3D12ShaderResourceViewTexture->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);

					// Do also set the given debug name to the Direct3D 12 resource referenced by the Direct3D resource view
					// -> In our use case, this resource is tightly coupled with the view
					// -> In principle the user can assign another resource to the view, but our interface documentation
					//    asks the user to not do so, so we ignore this situation when assigning the name
					ID3D12Resource *d3d12Resource = nullptr;
					mD3D12ShaderResourceViewTexture->GetResource(&d3d12Resource);
					if (nullptr != d3d12Resource)
					{
						// Set the debug name
						// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
						d3d12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
						d3d12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);

						// Release the Direct3D 12 resource instance
						d3d12Resource->Release();
					}
				}
			*/
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		DXGI_FORMAT			  mDxgiFormat;	//// DXGI format
		ID3D12Resource*		  mD3D12Resource;
		// TODO(co) Direct3D 12 update
		//ID3D12ShaderResourceView *mD3D12ShaderResourceViewTexture;	///< Direct3D 12 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Texture/Texture3D.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 3D texture class
	*/
	class Texture3D final : public Renderer::ITexture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
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
		Texture3D(Direct3D12Renderer& direct3D12Renderer, uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage) :
			ITexture3D(direct3D12Renderer, width, height, depth),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mD3D12Resource(nullptr),
			mD3D12DescriptorHeap(nullptr)
		{
			// TODO(co) Implement me

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("3D texture");
			#endif
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
			if (nullptr != mD3D12DescriptorHeap)
			{
				mD3D12DescriptorHeap->Release();
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
		*    Return the Direct3D 12 descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 descriptor heap instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
		{
			return mD3D12DescriptorHeap;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
					mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
				}
				if (nullptr != mD3D12DescriptorHeap)
				{
					mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
					mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture3D, this);
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
		DXGI_FORMAT			  mDxgiFormat;	//// DXGI format
		ID3D12Resource*		  mD3D12Resource;
		ID3D12DescriptorHeap* mD3D12DescriptorHeap;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Texture/TextureCube.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 cube texture class
	*/
	class TextureCube final : public Renderer::ITextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
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
		*  @param[in] textureUsage
		*    Indication of the texture usage
		*/
		TextureCube(Direct3D12Renderer& direct3D12Renderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage) :
			ITextureCube(direct3D12Renderer, width, height),
			mDxgiFormat(Mapping::getDirect3D12Format(textureFormat)),
			mD3D12Resource(nullptr),
			mD3D12DescriptorHeap(nullptr)
		{
			// TODO(co) Implement me

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Cube texture");
			#endif
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
			if (nullptr != mD3D12DescriptorHeap)
			{
				mD3D12DescriptorHeap->Release();
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
		*    Return the Direct3D 12 descriptor heap instance
		*
		*  @return
		*    The Direct3D 12 descriptor heap instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
		{
			return mD3D12DescriptorHeap;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12Resource)
				{
					mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
					mD3D12Resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
				}
				if (nullptr != mD3D12DescriptorHeap)
				{
					mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
					mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureCube, this);
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
		DXGI_FORMAT			  mDxgiFormat;	//// DXGI format
		ID3D12Resource*		  mD3D12Resource;
		ID3D12DescriptorHeap* mD3D12DescriptorHeap;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Texture/TextureManager.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 texture manager interface
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
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*/
		inline explicit TextureManager(Direct3D12Renderer& direct3D12Renderer) :
			ITextureManager(direct3D12Renderer)
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
		[[nodiscard]] virtual Renderer::ITexture1D* createTexture1D(uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0, "Direct3D 12 create texture 1D was called with invalid parameters")

			// Create 1D texture resource
			return RENDERER_NEW(getRenderer().getContext(), Texture1D)(static_cast<Direct3D12Renderer&>(getRenderer()), width, textureFormat, data, textureFlags, textureUsage);
		}

		[[nodiscard]] virtual Renderer::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0, "Direct3D 12 create texture 2D was called with invalid parameters")

			// Create 2D texture resource
			return RENDERER_NEW(getRenderer().getContext(), Texture2D)(static_cast<Direct3D12Renderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, textureUsage, numberOfMultisamples, optimizedTextureClearValue);
		}

		[[nodiscard]] virtual Renderer::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0 && numberOfSlices > 0, "Direct3D 12 create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource
			return RENDERER_NEW(getRenderer().getContext(), Texture2DArray)(static_cast<Direct3D12Renderer&>(getRenderer()), width, height, numberOfSlices, textureFormat, data, textureFlags, textureUsage);
		}

		[[nodiscard]] virtual Renderer::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0 && depth > 0, "Direct3D 12 create texture 3D was called with invalid parameters")

			// Create 3D texture resource
			return RENDERER_NEW(getRenderer().getContext(), Texture3D)(static_cast<Direct3D12Renderer&>(getRenderer()), width, height, depth, textureFormat, data, textureFlags, textureUsage);
		}

		[[nodiscard]] virtual Renderer::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0, "Direct3D 12 create texture cube was called with invalid parameters")

			// Create cube texture resource
			return RENDERER_NEW(getRenderer().getContext(), TextureCube)(static_cast<Direct3D12Renderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, textureUsage);
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


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/State/SamplerState.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 sampler state class
	*/
	class SamplerState final : public Renderer::ISamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(Direct3D12Renderer& direct3D12Renderer, const Renderer::SamplerState& samplerState) :
			ISamplerState(direct3D12Renderer),
			mD3D12DescriptorHeap(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D12Renderer.getContext(), samplerState.filter != Renderer::FilterMode::UNKNOWN, "Direct3D 12 filter mode must not be unknown")
			RENDERER_ASSERT(direct3D12Renderer.getContext(), samplerState.maxAnisotropy <= direct3D12Renderer.getCapabilities().maximumAnisotropy, "Maximum Direct3D 12 anisotropy value violated")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D12Renderer)

			ID3D12Device* d3d12Device = direct3D12Renderer.getD3D12Device();

			// Describe and create a sampler object descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
			D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
			d3d12DescriptorHeapDesc.NumDescriptors = 1;
			d3d12DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeap))))
			{
				// Create the sampler
				d3d12Device->CreateSampler(reinterpret_cast<const D3D12_SAMPLER_DESC*>(&samplerState), mD3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			}
			else
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 sampler state descriptor heap")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Sampler state");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D12Renderer)
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SamplerState() override
		{
			// Release the Direct3D 12 sampler state
			if (nullptr != mD3D12DescriptorHeap)
			{
				mD3D12DescriptorHeap->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D descriptor heap instance
		*
		*  @return
		*    The Direct3D descriptor heap instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D12DescriptorHeap* getD3D12DescriptorHeap() const
		{
			return mD3D12DescriptorHeap;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12DescriptorHeap)
				{
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeap->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SamplerState, this);
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
		ID3D12DescriptorHeap* mD3D12DescriptorHeap;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/RenderTarget/RenderPass.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 render pass interface
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
			RENDERER_ASSERT(renderer.getContext(), mNumberOfColorAttachments < 8, "Invalid number of Direct3D 12 color attachments")
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
			RENDERER_ASSERT(getRenderer().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid Direct3D 12 color attachment index")
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
	//[ Direct3D12Renderer/RenderTarget/SwapChain.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 swap chain class
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
		*/
		SwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle) :
			ISwapChain(renderPass),
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
			Direct3D12Renderer& direct3D12Renderer = static_cast<Direct3D12Renderer&>(renderPass.getRenderer());
			const RenderPass& d3d12RenderPass = static_cast<RenderPass&>(renderPass);

			// Sanity check
			RENDERER_ASSERT(direct3D12Renderer.getContext(), 1 == d3d12RenderPass.getNumberOfColorAttachments(), "There must be exactly one Direct3D 12 render pass color attachment")

			// Get the native window handle
			const HWND hWnd = reinterpret_cast<HWND>(windowHandle.nativeWindowHandle);

			// Get our IDXGI factory instance
			IDXGIFactory4& dxgiFactory4 = direct3D12Renderer.getDxgiFactory4Safe();

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

			// TODO(co) Add tearing support, see Direct3D 11 backend
			// Determines whether tearing support is available for fullscreen borderless windows
			// -> To unlock frame rates of UWP applications on the Windows Store and providing support for both AMD Freesync and NVIDIA's G-SYNC we must explicitly allow tearing
			// -> See "Windows Dev Center" -> "Variable refresh rate displays": https://msdn.microsoft.com/en-us/library/windows/desktop/mt742104(v=vs.85).aspx

			// Create the swap chain
			DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc = {};
			dxgiSwapChainDesc.BufferCount							= NUMBER_OF_FRAMES;
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
			FAILED_DEBUG_BREAK(dxgiFactory4.CreateSwapChain(direct3D12Renderer.getD3D12CommandQueue(), &dxgiSwapChainDesc, &dxgiSwapChain));
			if (FAILED(dxgiSwapChain->QueryInterface(IID_PPV_ARGS(&mDxgiSwapChain3))))
			{
				dxgiSwapChain->Release();
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to retrieve the Direct3D 12 DXGI swap chain 3")
			}

			// Disable alt-return for automatic fullscreen state change
			// -> We handle this manually to have more control over it
			FAILED_DEBUG_BREAK(dxgiFactory4.MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

			// Create the Direct3D 12 views
			if (nullptr != mDxgiSwapChain3)
			{
				createDirect3D12Views();
			}

			{ // Create synchronization objects
				// Get the Direct3D 12 device instance
				ID3D12Device* d3d12Device = nullptr;
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12Device));
				if (nullptr != d3d12Device)
				{
					if (SUCCEEDED(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mD3D12Fence))))
					{
						mFenceValue = 1;

						// Create an event handle to use for frame synchronization
						mFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
						if (nullptr == mFenceEvent)
						{
							RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create an Direct3D 12 event handle to use for frame synchronization. Error code %u", ::GetLastError())
						}
					}
					else
					{
						RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create Direct3D 12 fence instance")
					}
				}
				else
				{
					RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to retrieve the Direct3D 12 device instance from the swap chain")
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Swap chain");
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
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Assign a debug name to the DXGI swap chain
				if (nullptr != mDxgiSwapChain3)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDxgiSwapChain3->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mDxgiSwapChain3->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}

				// Assign a debug name to the Direct3D 12 frame resources
				for (int frame = 0; frame < NUMBER_OF_FRAMES; ++frame)
				{
					if (nullptr != mD3D12ResourceRenderTargets[frame])
					{
						// Set the debug name
						// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
						FAILED_DEBUG_BREAK(mD3D12ResourceRenderTargets[frame]->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
						FAILED_DEBUG_BREAK(mD3D12ResourceRenderTargets[frame]->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
					}
				}
				if (nullptr != mD3D12ResourceDepthStencil)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mD3D12ResourceDepthStencil->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12ResourceDepthStencil->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}

				// Assign a debug name to the Direct3D 12 descriptor heaps
				if (nullptr != mD3D12DescriptorHeapRenderTargetView)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapRenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapRenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D12DescriptorHeapDepthStencilView)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				// Get the Direct3D 12 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDesc(&dxgiSwapChainDesc));

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
	//[ Public virtual Renderer::ISwapChain methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Renderer::handle getNativeWindowHandle() const override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				// Get the Direct3D 12 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDesc(&dxgiSwapChainDesc));

				// Return the native window handle
				return reinterpret_cast<Renderer::handle>(dxgiSwapChainDesc.OutputWindow);
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
				const Direct3D12Renderer& direct3D12Renderer = static_cast<Direct3D12Renderer&>(getRenderPass().getRenderer());
				handleDeviceLost(direct3D12Renderer, mDxgiSwapChain3->Present(mSynchronizationInterval, 0));

				// Wait for the GPU to be done with all resources
				waitForPreviousFrame();
			}
		}

		virtual void resizeBuffers() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain3)
			{
				Direct3D12Renderer& direct3D12Renderer = static_cast<Direct3D12Renderer&>(getRenderer());

				// Get the currently set render target
				Renderer::IRenderTarget* renderTargetBackup = direct3D12Renderer.omGetRenderTarget();

				// In case this swap chain is the current render target, we have to unset it before continuing
				if (this == renderTargetBackup)
				{
					direct3D12Renderer.setGraphicsRenderTarget(nullptr);
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
				const HRESULT result = mDxgiSwapChain3->ResizeBuffers(NUMBER_OF_FRAMES, width, height, Mapping::getDirect3D12Format(static_cast<RenderPass&>(getRenderPass()).getColorAttachmentTextureFormat(0)), 0);
				if (SUCCEEDED(result))
				{
					// Create the Direct3D 12 views
					createDirect3D12Views();

					// If required, restore the previously set render target
					if (nullptr != renderTargetBackup)
					{
						direct3D12Renderer.setGraphicsRenderTarget(renderTargetBackup);
					}
				}
				else
				{
					handleDeviceLost(direct3D12Renderer, result);
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
				FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetFullscreenState(&fullscreen, nullptr));
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
					RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to set Direct3D 12 fullscreen state")
				}
			}
		}

		inline virtual void setRenderWindow([[maybe_unused]] Renderer::IRenderWindow* renderWindow) override
		{
			// TODO(sw) implement me
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
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		static constexpr uint32_t NUMBER_OF_FRAMES = 2;


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
			FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDesc(&dxgiSwapChainDesc));

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
			RENDERER_ASSERT(getRenderer().getContext(), nullptr != mDxgiSwapChain3, "Invalid Direct3D 12 DXGI swap chain 3")

			// TODO(co) Debug name gets lost when resizing a window, fix this

			// Get the Direct3D 12 device instance
			ID3D12Device* d3d12Device = nullptr;
			FAILED_DEBUG_BREAK(mDxgiSwapChain3->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12Device));
			if (nullptr != d3d12Device)
			{
				{ // Describe and create a render target view (RTV) descriptor heap
					D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
					d3d12DescriptorHeapDesc.NumDescriptors	= NUMBER_OF_FRAMES;
					d3d12DescriptorHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
					d3d12DescriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapRenderTargetView))))
					{
						mRenderTargetViewDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

						{ // Create frame resources
							CD3DX12_CPU_DESCRIPTOR_HANDLE d3d12XCpuDescriptorHandle(mD3D12DescriptorHeapRenderTargetView->GetCPUDescriptorHandleForHeapStart());

							// Create a RTV for each frame
							for (UINT frame = 0; frame < NUMBER_OF_FRAMES; ++frame)
							{
								if (SUCCEEDED(mDxgiSwapChain3->GetBuffer(frame, IID_PPV_ARGS(&mD3D12ResourceRenderTargets[frame]))))
								{
									d3d12Device->CreateRenderTargetView(mD3D12ResourceRenderTargets[frame], nullptr, d3d12XCpuDescriptorHandle);
									d3d12XCpuDescriptorHandle.Offset(1, mRenderTargetViewDescriptorSize);
								}
								else
								{
									RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to retrieve frame buffer from Direct3D 12 DXGI swap chain")
								}
							}
						}

						mFrameIndex = mDxgiSwapChain3->GetCurrentBackBufferIndex();
					}
					else
					{
						RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to describe and create a Direct3D 12 render target view (RTV) descriptor heap")
					}
				}

				// Describe and create a depth stencil view (DSV) descriptor heap
				const Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat = static_cast<RenderPass&>(getRenderPass()).getDepthStencilAttachmentTextureFormat();
				if (Renderer::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat)
				{
					D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
					d3d12DescriptorHeapDesc.NumDescriptors	= 1;
					d3d12DescriptorHeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
					d3d12DescriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
					if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapDepthStencilView))))
					{
						D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
						depthStencilDesc.Format = Mapping::getDirect3D12Format(depthStencilAttachmentTextureFormat);
						depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
						depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

						D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
						depthOptimizedClearValue.Format = depthStencilDesc.Format;
						depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
						depthOptimizedClearValue.DepthStencil.Stencil = 0;

						// Get the swap chain width and height, ensures they are never ever zero
						UINT width  = 1;
						UINT height = 1;
						getSafeWidthAndHeight(width, height);

						const CD3DX12_HEAP_PROPERTIES d3d12XHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
						const CD3DX12_RESOURCE_DESC d3d12XResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthStencilDesc.Format, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
						if (SUCCEEDED(d3d12Device->CreateCommittedResource(
							&d3d12XHeapProperties,
							D3D12_HEAP_FLAG_NONE,
							&d3d12XResourceDesc,
							D3D12_RESOURCE_STATE_DEPTH_WRITE,
							&depthOptimizedClearValue,
							IID_PPV_ARGS(&mD3D12ResourceDepthStencil)
							)))
						{
							d3d12Device->CreateDepthStencilView(mD3D12ResourceDepthStencil, &depthStencilDesc, mD3D12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart());
						}
						else
						{
							RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to create the Direct3D 12 depth stencil view (DSV) resource")
						}
					}
					else
					{
						RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to describe and create a Direct3D 12 depth stencil view (DSV) descriptor heap")
					}
				}
			}
			else
			{
				RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to retrieve the Direct3D 12 device instance from the swap chain")
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
			for (int frame = 0; frame < NUMBER_OF_FRAMES; ++frame)
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
			RENDERER_ASSERT(getRenderer().getContext(), nullptr != mDxgiSwapChain3, "Invalid Direct3D 12 DXGI swap chain 3")

			// TODO(co) This is the most simple but least effective approach and only meant for the Direct3D 12 renderer backend kickoff.

			// Signal and increment the fence value
			const UINT64 fence = mFenceValue;
			if (SUCCEEDED(static_cast<Direct3D12Renderer&>(getRenderer()).getD3D12CommandQueue()->Signal(mD3D12Fence, fence)))
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
						RENDERER_LOG(static_cast<Direct3D12Renderer&>(getRenderer()).getContext(), CRITICAL, "Failed to set Direct3D 12 event on completion")
					}
				}

				mFrameIndex = mDxgiSwapChain3->GetCurrentBackBufferIndex();
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IDXGISwapChain3*	  mDxgiSwapChain3;									///< The DXGI swap chain 3 instance, null pointer on error
		ID3D12DescriptorHeap* mD3D12DescriptorHeapRenderTargetView;				///< The Direct3D 12 render target view descriptor heap instance, null pointer on error
		ID3D12DescriptorHeap* mD3D12DescriptorHeapDepthStencilView;				///< The Direct3D 12 depth stencil view descriptor heap instance, null pointer on error
		UINT				  mRenderTargetViewDescriptorSize;					///< Render target view descriptor size
		ID3D12Resource*		  mD3D12ResourceRenderTargets[NUMBER_OF_FRAMES];	///< The Direct3D 12 render target instances, null pointer on error
		ID3D12Resource*		  mD3D12ResourceDepthStencil;						///< The Direct3D 12 depth stencil instance, null pointer on error

		// Synchronization objects
		uint32_t	 mSynchronizationInterval;
		UINT		 mFrameIndex;
		HANDLE		 mFenceEvent;
		ID3D12Fence* mD3D12Fence;
		UINT64		 mFenceValue;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/RenderTarget/Framebuffer.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 framebuffer class
	*
	*  @todo
	*    - TODO(co) "D3D12GraphicsCommandList::OMSetRenderTargets()" supports using a single Direct3D 12 render target view descriptor heap instance with multiple targets in it, use it
	*/
	class Framebuffer final : public Renderer::IFramebuffer
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
		Framebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment) :
			IFramebuffer(renderPass),
			mNumberOfColorTextures(static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments()),
			mColorTextures(nullptr),	// Set below
			mDepthStencilTexture(nullptr),
			mWidth(UINT_MAX),
			mHeight(UINT_MAX),
			mD3D12DescriptorHeapRenderTargetViews(nullptr),
			mD3D12DescriptorHeapDepthStencilView(nullptr)
		{
			// Get the Direct3D 12 device instance
			Direct3D12Renderer& direct3D12Renderer = static_cast<Direct3D12Renderer&>(renderPass.getRenderer());
			ID3D12Device* d3d12Device = direct3D12Renderer.getD3D12Device();

			// Add a reference to the used color textures
			if (mNumberOfColorTextures > 0)
			{
				const Renderer::Context& context = direct3D12Renderer.getContext();
				mColorTextures = RENDERER_MALLOC_TYPED(context, Renderer::ITexture*, mNumberOfColorTextures);
				mD3D12DescriptorHeapRenderTargetViews = RENDERER_MALLOC_TYPED(context, ID3D12DescriptorHeap*, mNumberOfColorTextures);

				// Loop through all color textures
				ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetView = mD3D12DescriptorHeapRenderTargetViews;
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments, ++d3d12DescriptorHeapRenderTargetView)
				{
					// Sanity check
					RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid Direct3D 12 color framebuffer attachment texture")

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
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 12 color framebuffer attachment mipmap index")
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid Direct3D 12 color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

							// Get the Direct3D 12 resource
							ID3D12Resource* d3d12Resource = texture2D->getD3D12Resource();

							// Create the Direct3D 12 render target view instance
							D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
							d3d12DescriptorHeapDesc.NumDescriptors = 1;
							d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
							if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(d3d12DescriptorHeapRenderTargetView))))
							{
								D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
								d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2D->getDxgiFormat());
								d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
								d3d12RenderTargetViewDesc.Texture2D.MipSlice = colorFramebufferAttachments->mipmapIndex;
								d3d12Device->CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, (*d3d12DescriptorHeapRenderTargetView)->GetCPUDescriptorHandleForHeapStart());
							}
							break;
						}

						case Renderer::ResourceType::TEXTURE_2D_ARRAY:
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
							if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(d3d12DescriptorHeapRenderTargetView))))
							{
								D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
								d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2DArray->getDxgiFormat());
								d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
								d3d12RenderTargetViewDesc.Texture2DArray.MipSlice		 = colorFramebufferAttachments->mipmapIndex;
								d3d12RenderTargetViewDesc.Texture2DArray.FirstArraySlice = colorFramebufferAttachments->layerIndex;
								d3d12RenderTargetViewDesc.Texture2DArray.ArraySize		 = 1;
								d3d12RenderTargetViewDesc.Texture2DArray.PlaneSlice		 = 0;
								d3d12Device->CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, (*d3d12DescriptorHeapRenderTargetView)->GetCPUDescriptorHandleForHeapStart());
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
							RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "The type of the given color texture at index %u is not supported by the Direct3D 12 renderer backend", colorTexture - mColorTextures)
							*d3d12DescriptorHeapRenderTargetView = nullptr;
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != mDepthStencilTexture, "Invalid Direct3D 12 depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 12 depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Direct3D 12 depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

						// Get the Direct3D 12 resource
						ID3D12Resource* d3d12Resource = texture2D->getD3D12Resource();

						// Create the Direct3D 12 render target view instance
						D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDesc = {};
						d3d12DescriptorHeapDesc.NumDescriptors = 1;
						d3d12DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
						if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapDepthStencilView))))
						{
							D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
							d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2D->getDxgiFormat());
							d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
							d3d12RenderTargetViewDesc.Texture2D.MipSlice = depthStencilFramebufferAttachment->mipmapIndex;
							d3d12Device->CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, mD3D12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart());
						}
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
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
						if (SUCCEEDED(d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDesc, IID_PPV_ARGS(&mD3D12DescriptorHeapDepthStencilView))))
						{
							D3D12_RENDER_TARGET_VIEW_DESC d3d12RenderTargetViewDesc = {};
							d3d12RenderTargetViewDesc.Format = static_cast<DXGI_FORMAT>(texture2DArray->getDxgiFormat());
							d3d12RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							d3d12RenderTargetViewDesc.Texture2DArray.MipSlice		 = depthStencilFramebufferAttachment->mipmapIndex;
							d3d12RenderTargetViewDesc.Texture2DArray.FirstArraySlice = depthStencilFramebufferAttachment->layerIndex;
							d3d12RenderTargetViewDesc.Texture2DArray.ArraySize		 = 1;
							d3d12RenderTargetViewDesc.Texture2DArray.PlaneSlice		 = 0;
							d3d12Device->CreateRenderTargetView(d3d12Resource, &d3d12RenderTargetViewDesc, mD3D12DescriptorHeapDepthStencilView->GetCPUDescriptorHandleForHeapStart());
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
						RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "The type of the given depth stencil texture is not supported by the Direct3D 12 renderer backend")
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Direct3D 12 framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Direct3D 12 framebuffer height")
				mHeight = 1;
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("FBO");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			// Release the reference to the used color textures
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mD3D12DescriptorHeapRenderTargetViews)
			{
				// Release references
				ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetViewsEnd = mD3D12DescriptorHeapRenderTargetViews + mNumberOfColorTextures;
				for (ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetView = mD3D12DescriptorHeapRenderTargetViews; d3d12DescriptorHeapRenderTargetView < d3d12DescriptorHeapRenderTargetViewsEnd; ++d3d12DescriptorHeapRenderTargetView)
				{
					(*d3d12DescriptorHeapRenderTargetView)->Release();
				}

				// Cleanup
				RENDERER_FREE(context, mD3D12DescriptorHeapRenderTargetViews);
			}
			if (nullptr != mColorTextures)
			{
				// Release references
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture)
				{
					(*colorTexture)->releaseReference();
				}

				// Cleanup
				RENDERER_FREE(context, mColorTextures);
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
		[[nodiscard]] inline Renderer::ITexture** getColorTextures() const
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
		[[nodiscard]] inline Renderer::ITexture* getDepthStencilTexture() const
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
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				{ // Assign a debug name to the Direct3D 12 render target view, do also add the index to the name
					const size_t nameLength = strlen(name) + 5;	// Direct3D 12 supports 8 render targets ("D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT", so: One digit + one [ + one ] + one space + terminating zero = 5 characters)
					const Renderer::Context& context = getRenderer().getContext();
					char* nameWithIndex = RENDERER_MALLOC_TYPED(context, char, nameLength);
					ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetViewsEnd = mD3D12DescriptorHeapRenderTargetViews + mNumberOfColorTextures;
					for (ID3D12DescriptorHeap **d3d12DescriptorHeapRenderTargetView = mD3D12DescriptorHeapRenderTargetViews; d3d12DescriptorHeapRenderTargetView < d3d12DescriptorHeapRenderTargetViewsEnd; ++d3d12DescriptorHeapRenderTargetView)
					{
						// Set the debug name
						// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
						sprintf_s(nameWithIndex, nameLength, "%s [%u]", name, static_cast<uint32_t>(d3d12DescriptorHeapRenderTargetView - mD3D12DescriptorHeapRenderTargetViews));
						FAILED_DEBUG_BREAK((*d3d12DescriptorHeapRenderTargetView)->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
						FAILED_DEBUG_BREAK((*d3d12DescriptorHeapRenderTargetView)->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(nameLength), nameWithIndex));
					}
					RENDERER_FREE(context, nameWithIndex);
				}

				// Assign a debug name to the Direct3D 12 depth stencil view
				if (nullptr != mD3D12DescriptorHeapDepthStencilView)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12DescriptorHeapDepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


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
		uint32_t			 mNumberOfColorTextures;	///< Number of color render target textures
		Renderer::ITexture **mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "m_nNumberOfColorTextures" textures in the provided C-array of pointers
		Renderer::ITexture  *mDepthStencilTexture;		///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t			 mWidth;					///< The framebuffer width
		uint32_t			 mHeight;					///< The framebuffer height
		// Direct3D 12 part
		ID3D12DescriptorHeap** mD3D12DescriptorHeapRenderTargetViews;	///< The Direct3D 12 render target view descriptor heap instance, null pointer on error
		ID3D12DescriptorHeap*  mD3D12DescriptorHeapDepthStencilView;	///< The Direct3D 12 depth stencil view descriptor heap instance, null pointer on error


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Shader/VertexShaderHlsl.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL vertex shader class
	*/
	class VertexShaderHlsl final : public Renderer::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader bytecode
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IVertexShader(direct3D12Renderer),
			mD3DBlobVertexShader(nullptr)
		{
			// Backup the vertex shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobVertexShader));
			memcpy(mD3DBlobVertexShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IVertexShader(direct3D12Renderer),
			mD3DBlobVertexShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the vertex shader
			mD3DBlobVertexShader = loadShaderFromSourcecode(direct3D12Renderer.getContext(), "vs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobVertexShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobVertexShader->GetBufferPointer()));
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 12 automatically sets a decent default name
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
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexShaderHlsl, this);
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
	//[ Direct3D12Renderer/Shader/TessellationControlShaderHlsl.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShaderHlsl final : public Renderer::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation control shader from shader bytecode
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationControlShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			ITessellationControlShader(direct3D12Renderer),
			mD3DBlobHullShader(nullptr)
		{
			// Backup the hull shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobHullShader));
			memcpy(mD3DBlobHullShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation control shader from shader source code
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationControlShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			ITessellationControlShader(direct3D12Renderer),
			mD3DBlobHullShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the hull shader
			mD3DBlobHullShader = loadShaderFromSourcecode(direct3D12Renderer.getContext(), "hs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobHullShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobHullShader->GetBufferPointer()));
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 12 automatically sets a decent default name
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
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationControlShaderHlsl, this);
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
	//[ Direct3D12Renderer/Shader/TessellationEvaluationShaderHlsl.h ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShaderHlsl final : public Renderer::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader from shader bytecode
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		TessellationEvaluationShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			ITessellationEvaluationShader(direct3D12Renderer),
			mD3DBlobDomainShader(nullptr)
		{
			// Backup the domain shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobDomainShader));
			memcpy(mD3DBlobDomainShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a tessellation evaluation shader from shader source code
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		TessellationEvaluationShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			ITessellationEvaluationShader(direct3D12Renderer),
			mD3DBlobDomainShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the domain shader
			mD3DBlobDomainShader = loadShaderFromSourcecode(direct3D12Renderer.getContext(), "ds_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobDomainShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobDomainShader->GetBufferPointer()));
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 12 automatically sets a decent default name
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
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationEvaluationShaderHlsl, this);
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
	//[ Direct3D12Renderer/Shader/GeometryShaderHlsl.h        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL geometry shader class
	*/
	class GeometryShaderHlsl final : public Renderer::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader bytecode
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		GeometryShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IGeometryShader(direct3D12Renderer),
			mD3DBlobGeometryShader(nullptr)
		{
			// Backup the geometry shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobGeometryShader));
			memcpy(mD3DBlobGeometryShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		GeometryShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IGeometryShader(direct3D12Renderer),
			mD3DBlobGeometryShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the geometry shader
			mD3DBlobGeometryShader = loadShaderFromSourcecode(direct3D12Renderer.getContext(), "gs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobGeometryShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobGeometryShader->GetBufferPointer()));
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 12 automatically sets a decent default name
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
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GeometryShaderHlsl, this);
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
	//[ Direct3D12Renderer/Shader/FragmentShaderHlsl.h        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL fragment shader class (FS, "pixel shader" in Direct3D terminology)
	*/
	class FragmentShaderHlsl final : public Renderer::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader bytecode
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IFragmentShader(direct3D12Renderer),
			mD3DBlobFragmentShader(nullptr)
		{
			// Backup the fragment shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobFragmentShader));
			memcpy(mD3DBlobFragmentShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IFragmentShader(direct3D12Renderer),
			mD3DBlobFragmentShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the fragment shader
			mD3DBlobFragmentShader = loadShaderFromSourcecode(direct3D12Renderer.getContext(), "ps_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobFragmentShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobFragmentShader->GetBufferPointer()));
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 12 automatically sets a decent default name
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
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), FragmentShaderHlsl, this);
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
		ID3DBlob* mD3DBlobFragmentShader;	///< Direct3D 12 fragment shader blob, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Shader/ComputeShaderHlsl.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL compute shader class (CS)
	*/
	class ComputeShaderHlsl final : public Renderer::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for creating a compute shader from shader bytecode
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		ComputeShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IComputeShader(direct3D12Renderer),
			mD3DBlobComputeShader(nullptr)
		{
			// Backup the compute shader bytecode
			FAILED_DEBUG_BREAK(D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobComputeShader));
			memcpy(mD3DBlobComputeShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());
		}

		/**
		*  @brief
		*    Constructor for creating a compute shader from shader source code
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		ComputeShaderHlsl(Direct3D12Renderer& direct3D12Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IComputeShader(direct3D12Renderer),
			mD3DBlobComputeShader(nullptr)
		{
			// Create the Direct3D 12 binary large object for the compute shader
			mD3DBlobComputeShader = loadShaderFromSourcecode(direct3D12Renderer.getContext(), "cs_5_0", sourceCode, nullptr, optimizationLevel);

			// Return shader bytecode, if requested do to so
			if (nullptr != shaderBytecode)
			{
				shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobComputeShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobComputeShader->GetBufferPointer()));
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 12 automatically sets a decent default name
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
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputeShaderHlsl, this);
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
	//[ Direct3D12Renderer/Shader/GraphicsProgramHlsl.h       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL graphics program class
	*/
	class GraphicsProgramHlsl final : public Renderer::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
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
		GraphicsProgramHlsl(Direct3D12Renderer& direct3D12Renderer, VertexShaderHlsl* vertexShaderHlsl, TessellationControlShaderHlsl* tessellationControlShaderHlsl, TessellationEvaluationShaderHlsl* tessellationEvaluationShaderHlsl, GeometryShaderHlsl* geometryShaderHlsl, FragmentShaderHlsl* fragmentShaderHlsl) :
			IGraphicsProgram(direct3D12Renderer),
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
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			inline virtual void setDebugName(const char*) override
			{
				// In here we could assign the given debug name to all shaders assigned to the graphics program,
				// but this might end up within a naming chaos due to overwriting possible already set
				// names... don't do this...
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GraphicsProgramHlsl, this);
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
		VertexShaderHlsl				 *mVertexShaderHlsl;					///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationControlShaderHlsl	 *mTessellationControlShaderHlsl;		///< Tessellation control shader the graphics program is using (we keep a reference to it), can be a null pointer
		TessellationEvaluationShaderHlsl* mTessellationEvaluationShaderHlsl;	///< Tessellation evaluation shader the graphics program is using (we keep a reference to it), can be a null pointer
		GeometryShaderHlsl				 *mGeometryShaderHlsl;					///< Geometry shader the graphics program is using (we keep a reference to it), can be a null pointer
		FragmentShaderHlsl				 *mFragmentShaderHlsl;					///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/Shader/ShaderLanguageHlsl.h        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL shader language class
	*/
	class ShaderLanguageHlsl final : public Renderer::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*/
		inline explicit ShaderLanguageHlsl(Direct3D12Renderer& direct3D12Renderer) :
			IShaderLanguage(direct3D12Renderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguageHlsl() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShaderLanguage methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::HLSL_NAME;
		}

		[[nodiscard]] inline virtual Renderer::IVertexShader* createVertexShaderFromBytecode([[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::vertexShader", we know there's vertex shader support
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::vertexShader", we know there's vertex shader support
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// "hull shader" in Direct3D terminology

			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// "hull shader" in Direct3D terminology

			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationControlShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// "domain shader" in Direct3D terminology

			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// "domain shader" in Direct3D terminology

			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			return RENDERER_NEW(getRenderer().getContext(), GeometryShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			return RENDERER_NEW(getRenderer().getContext(), GeometryShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::computeShader", we know there's compute shader support
			return RENDERER_NEW(getRenderer().getContext(), ComputeShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::computeShader", we know there's compute shader support
			return RENDERER_NEW(getRenderer().getContext(), ComputeShaderHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] virtual Renderer::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Renderer::IRootSignature& rootSignature, [[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, Renderer::IVertexShader* vertexShader, Renderer::ITessellationControlShader* tessellationControlShader, Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 vertex shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 tessellation control shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 tessellation evaluation shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 geometry shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 12 fragment shader language mismatch")

			// Create the graphics program
			return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramHlsl)(static_cast<Direct3D12Renderer&>(getRenderer()), static_cast<VertexShaderHlsl*>(vertexShader), static_cast<TessellationControlShaderHlsl*>(tessellationControlShader), static_cast<TessellationEvaluationShaderHlsl*>(tessellationEvaluationShader), static_cast<GeometryShaderHlsl*>(geometryShader), static_cast<FragmentShaderHlsl*>(fragmentShader));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ShaderLanguageHlsl, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguageHlsl(const ShaderLanguageHlsl& source) = delete;
		ShaderLanguageHlsl& operator =(const ShaderLanguageHlsl& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/State/GraphicsPipelineState.h      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 graphics pipeline state class
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
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(Direct3D12Renderer& direct3D12Renderer, const Renderer::GraphicsPipelineState& graphicsPipelineState, uint16_t id) :
			IGraphicsPipelineState(direct3D12Renderer, id),
			mD3D12PrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(graphicsPipelineState.primitiveTopology)),
			mD3D12GraphicsPipelineState(nullptr),
			mRootSignature(graphicsPipelineState.rootSignature),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass)
		{
			// Add a reference to the referenced renderer resources
			mRootSignature->addReference();
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			// Define the vertex input layout
			// -> No dynamic allocations/deallocations in here, a fixed maximum number of supported attributes must be sufficient
			static constexpr uint32_t MAXIMUM_NUMBER_OF_ATTRIBUTES = 16;	// 16 elements per vertex are already pretty many
			uint32_t numberOfVertexAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
			if (numberOfVertexAttributes > MAXIMUM_NUMBER_OF_ATTRIBUTES)
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Too many vertex attributes (%u) provided. The limit of the Direct3D 12 renderer backend is %u.", numberOfVertexAttributes, MAXIMUM_NUMBER_OF_ATTRIBUTES)
				numberOfVertexAttributes = MAXIMUM_NUMBER_OF_ATTRIBUTES;
			}
			D3D12_INPUT_ELEMENT_DESC d3d12InputElementDescs[MAXIMUM_NUMBER_OF_ATTRIBUTES];
			for (uint32_t vertexAttribute = 0; vertexAttribute < numberOfVertexAttributes; ++vertexAttribute)
			{
				const Renderer::VertexAttribute& currentVertexAttribute = graphicsPipelineState.vertexAttributes.attributes[vertexAttribute];
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
			d3d12GraphicsPipelineState.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(graphicsPipelineState.primitiveTopologyType);
			memcpy(&d3d12GraphicsPipelineState.RasterizerState, &graphicsPipelineState.rasterizerState, sizeof(D3D12_RASTERIZER_DESC));
			memcpy(&d3d12GraphicsPipelineState.DepthStencilState, &graphicsPipelineState.depthStencilState, sizeof(D3D12_DEPTH_STENCIL_DESC));
			d3d12GraphicsPipelineState.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			d3d12GraphicsPipelineState.SampleMask = UINT_MAX;
			d3d12GraphicsPipelineState.NumRenderTargets = graphicsPipelineState.numberOfRenderTargets;
			for (uint32_t i = 0; i < graphicsPipelineState.numberOfRenderTargets; ++i)
			{
				d3d12GraphicsPipelineState.RTVFormats[i] = Mapping::getDirect3D12Format(graphicsPipelineState.renderTargetViewFormats[i]);
			}
			d3d12GraphicsPipelineState.DSVFormat = Mapping::getDirect3D12Format(graphicsPipelineState.depthStencilViewFormat);
			d3d12GraphicsPipelineState.SampleDesc.Count = 1;
			if (FAILED(direct3D12Renderer.getD3D12Device()->CreateGraphicsPipelineState(&d3d12GraphicsPipelineState, IID_PPV_ARGS(&mD3D12GraphicsPipelineState))))
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 graphics pipeline state object")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Graphics pipeline state");
			#endif
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

			// Release referenced renderer resources
			mRootSignature->releaseReference();
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Free the unique compact graphics pipeline state ID
			static_cast<Direct3D12Renderer&>(getRenderer()).GraphicsPipelineStateMakeId.DestroyID(getId());
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
		[[nodiscard]] inline ID3D12PipelineState *getD3D12GraphicsPipelineState() const
		{
			return mD3D12GraphicsPipelineState;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12GraphicsPipelineState)
				{
					FAILED_DEBUG_BREAK(mD3D12GraphicsPipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12GraphicsPipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


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
		D3D12_PRIMITIVE_TOPOLOGY	mD3D12PrimitiveTopology;
		ID3D12PipelineState*		mD3D12GraphicsPipelineState;	///< Direct3D 12 graphics pipeline state, can be a null pointer
		Renderer::IRootSignature*	mRootSignature;
		Renderer::IGraphicsProgram*	mGraphicsProgram;
		Renderer::IRenderPass*		mRenderPass;


	};




	//[-------------------------------------------------------]
	//[ Direct3D12Renderer/State/ComputePipelineState.h       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 12 compute pipeline state class
	*/
	class ComputePipelineState final : public Renderer::IComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D12Renderer
		*    Owner Direct3D 12 renderer instance
		*  @param[in] rootSignature
		*    Root signature shader to use
		*  @param[in] computeShader
		*    Compute shader to use
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		ComputePipelineState(Direct3D12Renderer& direct3D12Renderer, Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader, uint16_t id) :
			IComputePipelineState(direct3D12Renderer, id),
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
			if (FAILED(direct3D12Renderer.getD3D12Device()->CreateComputePipelineState(&d3d12ComputePipelineState, IID_PPV_ARGS(&mD3D12ComputePipelineState))))
			{
				RENDERER_LOG(direct3D12Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 12 compute pipeline state object")
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Compute pipeline state");
			#endif
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
			static_cast<Direct3D12Renderer&>(getRenderer()).ComputePipelineStateMakeId.DestroyID(getId());
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
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D12ComputePipelineState)
				{
					FAILED_DEBUG_BREAK(mD3D12ComputePipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D12ComputePipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ComputePipelineState, this);
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
		ID3D12PipelineState*	  mD3D12ComputePipelineState;	///< Direct3D 12 compute pipeline state, can be a null pointer
		Renderer::IRootSignature& mRootSignature;
		Renderer::IComputeShader& mComputeShader;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D12Renderer




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
		namespace BackendDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ExecuteCommandBuffer* realData = static_cast<const Renderer::Command::ExecuteCommandBuffer*>(data);
				RENDERER_ASSERT(renderer.getContext(), nullptr != realData->commandBufferToExecute, "The Direct3D 12 command buffer to execute must be valid")
				renderer.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsRootSignature* realData = static_cast<const Renderer::Command::SetGraphicsRootSignature*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsPipelineState* realData = static_cast<const Renderer::Command::SetGraphicsPipelineState*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsResourceGroup* realData = static_cast<const Renderer::Command::SetGraphicsResourceGroup*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Renderer::IRenderer& renderer)
			{
				// Input-assembler (IA) stage
				const Renderer::Command::SetGraphicsVertexArray* realData = static_cast<const Renderer::Command::SetGraphicsVertexArray*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsViewports* realData = static_cast<const Renderer::Command::SetGraphicsViewports*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Renderer::Viewport*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsScissorRectangles* realData = static_cast<const Renderer::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Renderer::ScissorRectangle*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Renderer::IRenderer& renderer)
			{
				// Output-merger (OM) stage
				const Renderer::Command::SetGraphicsRenderTarget* realData = static_cast<const Renderer::Command::SetGraphicsRenderTarget*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ClearGraphics* realData = static_cast<const Renderer::Command::ClearGraphics*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawGraphics* realData = static_cast<const Renderer::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					// TODO(co) Implement indirect buffer support, see e.g. "Voxel visualization using DrawIndexedInstancedIndirect" - http://www.alexandre-pestana.com/tag/directx/ for hints
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).drawGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).drawGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawIndexedGraphics* realData = static_cast<const Renderer::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					// TODO(co) Implement indirect buffer support, see e.g. "Voxel visualization using DrawIndexedInstancedIndirect" - http://www.alexandre-pestana.com/tag/directx/ for hints
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).drawIndexedGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).drawIndexedGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputeRootSignature* realData = static_cast<const Renderer::Command::SetComputeRootSignature*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputePipelineState* realData = static_cast<const Renderer::Command::SetComputePipelineState*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetComputeResourceGroup* realData = static_cast<const Renderer::Command::SetComputeResourceGroup*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DispatchCompute* realData = static_cast<const Renderer::Command::DispatchCompute*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).dispatchCompute(realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Renderer::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					static_cast<Direct3D12Renderer::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RENDERER_LOG(static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).getContext(), CRITICAL, "Unsupported Direct3D 12 texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Renderer::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::CopyResource* realData = static_cast<const Renderer::Command::CopyResource*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::GenerateMipmaps* realData = static_cast<const Renderer::Command::GenerateMipmaps*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResetQueryPool* realData = static_cast<const Renderer::Command::ResetQueryPool*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::BeginQuery* realData = static_cast<const Renderer::Command::BeginQuery*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::EndQuery* realData = static_cast<const Renderer::Command::EndQuery*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::WriteTimestampQuery* realData = static_cast<const Renderer::Command::WriteTimestampQuery*>(data);
				static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}


			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RENDERER_DEBUG
				void SetDebugMarker(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::SetDebugMarker* realData = static_cast<const Renderer::Command::SetDebugMarker*>(data);
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::BeginDebugEvent* realData = static_cast<const Renderer::Command::BeginDebugEvent*>(data);
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Renderer::IRenderer& renderer)
				{
					static_cast<Direct3D12Renderer::Direct3D12Renderer&>(renderer).endDebugEvent();
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
namespace Direct3D12Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Direct3D12Renderer::Direct3D12Renderer(const Renderer::Context& context) :
		IRenderer(Renderer::NameId::DIRECT3D12, context),
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
		// mD3D12QueryFlush(nullptr),	// TODO(co) Direct3D 12 update
		mRenderTarget(nullptr),
		mD3D12PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
	{
		mDirect3D12RuntimeLinking = RENDERER_NEW(mContext, Direct3D12RuntimeLinking)(*this);

		// Is Direct3D 12 available?
		if (mDirect3D12RuntimeLinking->isDirect3D12Avaiable())
		{
			// Create the DXGI factory instance
			if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory4))))
			{
				// Enable the Direct3D 12 debug layer
				#ifdef RENDERER_DEBUG
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
					RENDERER_LOG(mContext, CRITICAL, "Failed to create Direct3D 12 device instance. Creating an emulated Direct3D 11 device instance instead.")

					// Create the DXGI adapter instance
					IDXGIAdapter* dxgiAdapter = nullptr;
					if (SUCCEEDED(mDxgiFactory4->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter))))
					{
						// Create the emulated Direct3D 12 device
						if (FAILED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3D12Device))))
						{
							RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 device instance")
						}

						// Release the DXGI adapter instance
						dxgiAdapter->Release();
					}
					else
					{
						RENDERER_LOG(mContext, CRITICAL, "Failed to create Direct3D 12 DXGI adapter instance")
					}
				}
			}
			else
			{
				RENDERER_LOG(mContext, CRITICAL, "Failed to create Direct3D 12 DXGI factory instance")
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
							}
							else
							{
								RENDERER_LOG(mContext, CRITICAL, "Failed to close the Direct3D 12 command list instance")
							}
						}
						else
						{
							RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 command list instance")
						}
					}
					else
					{
						RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 command allocator instance")
					}
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 12 command queue instance")
				}
			}
		}
	}

	Direct3D12Renderer::~Direct3D12Renderer()
	{
		// Release instances
		if (nullptr != mRenderTarget)
		{
			mRenderTarget->releaseReference();
			mRenderTarget = nullptr;
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
					RENDERER_LOG(mContext, CRITICAL, "The Direct3D 12 renderer backend is going to be destroyed, but there are still %u resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "The Direct3D 12 renderer backend is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the Direct3D 12 query instance used for flush, in case we have one
		// TODO(co) Direct3D 12 update
		/*
		if (nullptr != mD3D12QueryFlush)
		{
			mD3D12QueryFlush->Release();
		}
		*/

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
		RENDERER_DELETE(mContext, Direct3D12RuntimeLinking, mDirect3D12RuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void Direct3D12Renderer::setGraphicsRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != rootSignature)
		{
			// Sanity check
			DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)

			// Set graphics root signature
			mD3D12GraphicsCommandList->SetGraphicsRootSignature(static_cast<RootSignature*>(rootSignature)->getD3D12RootSignature());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Renderer::setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Sanity check
			DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsPipelineState)

			// Set primitive topology
			// -> The "Renderer::PrimitiveTopology" values directly map to Direct3D 9 & 10 & 11 && 12 constants, do not change them
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

	void Direct3D12Renderer::setGraphicsResourceGroup(uint32_t, Renderer::IResourceGroup*)
	{
		// TODO(co) Implement resource group
		RENDERER_ASSERT(mContext, false, "Direct3D 12 setGraphicsResourceGroup() isn't implemented, yet")
		/*
		if (nullptr != resource)
		{
			// Sanity check
			DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resource)

			switch (resource->getResourceType())
			{
				case Renderer::ResourceType::UNIFORM_BUFFER:
				{
					ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<UniformBuffer*>(resource)->getD3D12DescriptorHeap();
					if (nullptr != d3D12DescriptorHeap)
					{
						// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
						ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
						mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

						mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					}
					break;
				}

				case Renderer::ResourceType::TEXTURE_1D:
				{
					ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<Texture1D*>(resource)->getD3D12DescriptorHeap();
					if (nullptr != d3D12DescriptorHeap)
					{
						// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
						ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
						mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

						mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					}
					break;
				}

				case Renderer::ResourceType::TEXTURE_2D:
				{
					ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<Texture2D*>(resource)->getD3D12DescriptorHeap();
					if (nullptr != d3D12DescriptorHeap)
					{
						// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
						ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
						mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

						mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					}
					break;
				}

				case Renderer::ResourceType::TEXTURE_2D_ARRAY:
				{
					// TODO(co) Implement 2D texture array
				//	ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<Texture2DArray*>(resource)->getD3D12DescriptorHeap();
				//	if (nullptr != d3D12DescriptorHeap)
				//	{
				//		// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
				//		ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
				//		mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

				//		mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
				//	}
					break;
				}

				case Renderer::ResourceType::TEXTURE_3D:
				{
					ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<Texture3D*>(resource)->getD3D12DescriptorHeap();
					if (nullptr != d3D12DescriptorHeap)
					{
						// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
						ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
						mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

						mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					}
					break;
				}

				case Renderer::ResourceType::TEXTURE_CUBE:
				{
					ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<TextureCube*>(resource)->getD3D12DescriptorHeap();
					if (nullptr != d3D12DescriptorHeap)
					{
						// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
						ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
						mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

						mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					}
					break;
				}

				case Renderer::ResourceType::SAMPLER_STATE:
				{
					ID3D12DescriptorHeap* d3D12DescriptorHeap = static_cast<SamplerState*>(resource)->getD3D12DescriptorHeap();
					if (nullptr != d3D12DescriptorHeap)
					{
						// TODO(co) Just a first Direct3D 12 test, don't call "ID3D12GraphicsCommandList::SetDescriptorHeaps()" that often (pipeline flush)
						ID3D12DescriptorHeap* ppHeaps[] = { d3D12DescriptorHeap };
						mD3D12GraphicsCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

						mD3D12GraphicsCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
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
				case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
				case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
				case Renderer::ResourceType::VERTEX_SHADER:
				case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
				case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
				case Renderer::ResourceType::GEOMETRY_SHADER:
				case Renderer::ResourceType::FRAGMENT_SHADER:
				case Renderer::ResourceType::COMPUTE_SHADER:
					RENDERER_LOG(mContext, CRITICAL, "Invalid Direct3D 12 renderer backend resource type")
					break;
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
		*/
	}

	void Direct3D12Renderer::setGraphicsVertexArray(Renderer::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage
		if (nullptr != vertexArray)
		{
			// Sanity check
			DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *vertexArray)

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

			static_cast<VertexArray*>(vertexArray)->setDirect3DIASetInputLayoutAndStreamSource(*mD3D12GraphicsCommandList);

			// End debug event
			RENDERER_END_DEBUG_EVENT(this)
		}
		else
		{
			// Set no Direct3D 12 input layout
			mD3D12GraphicsCommandList->IASetVertexBuffers(0, 0, nullptr);
		}
	}

	void Direct3D12Renderer::setGraphicsViewports(uint32_t numberOfViewports, const Renderer::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Direct3D 12 rasterizer state viewports")

		// Set the Direct3D 12 viewports
		// -> "Renderer::Viewport" directly maps to Direct3D 12, do not change it
		// -> Let Direct3D 12 perform the index validation for us (the Direct3D 12 debug features are pretty good)
		mD3D12GraphicsCommandList->RSSetViewports(numberOfViewports, reinterpret_cast<const D3D12_VIEWPORT*>(viewports));
	}

	void Direct3D12Renderer::setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Direct3D 12 rasterizer state scissor rectangles")

		// Set the Direct3D 12 scissor rectangles
		// -> "Renderer::ScissorRectangle" directly maps to Direct3D 9 & 10 & 11 & 12, do not change it
		// -> Let Direct3D 12 perform the index validation for us (the Direct3D 12 debug features are pretty good)
		mD3D12GraphicsCommandList->RSSetScissorRects(numberOfScissorRectangles, reinterpret_cast<const D3D12_RECT*>(scissorRectangles));
	}

	void Direct3D12Renderer::setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget)
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
					case Renderer::ResourceType::SWAP_CHAIN:
					{
						// Get the Direct3D 12 swap chain instance
						SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

						// Inform Direct3D 12 about the resource transition
						CD3DX12_RESOURCE_BARRIER d3d12XResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChain->getBackD3D12ResourceRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
						mD3D12GraphicsCommandList->ResourceBarrier(1, &d3d12XResourceBarrier);
						break;
					}

					case Renderer::ResourceType::FRAMEBUFFER:
					{
						// TODO(co) Implement resource transition handling (first "Direct3D12Renderer::Texture2D" needs to be cleaned up)
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

				// Release the render target reference, in case we have one
				mRenderTarget->releaseReference();
				mRenderTarget = nullptr;
			}

			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Sanity check
				DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *renderTarget)

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Evaluate the render target type
				switch (mRenderTarget->getResourceType())
				{
					case Renderer::ResourceType::SWAP_CHAIN:
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

					case Renderer::ResourceType::FRAMEBUFFER:
					{
						// Get the Direct3D 12 framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Set the Direct3D 12 render targets
						const uint32_t numberOfColorTextures = framebuffer->getNumberOfColorTextures();
						D3D12_CPU_DESCRIPTOR_HANDLE d3d12CpuDescriptorHandlesRenderTarget[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
						for (uint32_t i = 0; i < numberOfColorTextures; ++i)
						{
							d3d12CpuDescriptorHandlesRenderTarget[i] = framebuffer->getD3D12DescriptorHeapRenderTargetViews()[i]->GetCPUDescriptorHandleForHeapStart();

							// TODO(co) Implement resource transition handling (first "Direct3D12Renderer::Texture2D" needs to be cleaned up)
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
							// TODO(co) Implement resource transition handling (first "Direct3D12Renderer::Texture2D" needs to be cleaned up)
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
			else
			{
				mD3D12GraphicsCommandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
			}
		}
	}

	void Direct3D12Renderer::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Unlike Direct3D 9, OpenGL or OpenGL ES 3, Direct3D 12 clears a given render target view and not the currently bound
		// -> No resource transition required in here, it's handled inside "Direct3D12Renderer::omSetRenderTarget()"

		// Begin debug event
		RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

		// Render target set?
		if (nullptr != mRenderTarget)
		{
			// Evaluate the render target type
			switch (mRenderTarget->getResourceType())
			{
				case Renderer::ResourceType::SWAP_CHAIN:
				{
					// Get the Direct3D 12 swap chain instance
					SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

					// Clear the Direct3D 12 render target view?
					if (clearFlags & Renderer::ClearFlag::COLOR)
					{
						CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapChain->getD3D12DescriptorHeapRenderTargetView()->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(swapChain->getBackD3D12ResourceRenderTargetFrameIndex()), swapChain->getRenderTargetViewDescriptorSize());
						mD3D12GraphicsCommandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
					}

					// Clear the Direct3D 12 depth stencil view?
					ID3D12DescriptorHeap* d3d12DescriptorHeapDepthStencilView = swapChain->getD3D12DescriptorHeapDepthStencilView();
					if (nullptr != d3d12DescriptorHeapDepthStencilView)
					{
						// Get the Direct3D 12 clear flags
						UINT direct3D12ClearFlags = (clearFlags & Renderer::ClearFlag::DEPTH) ? D3D12_CLEAR_FLAG_DEPTH : 0u;
						if (clearFlags & Renderer::ClearFlag::STENCIL)
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

				case Renderer::ResourceType::FRAMEBUFFER:
				{
					// Get the Direct3D 12 framebuffer instance
					Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

					// Clear all Direct3D 12 render target views?
					if (clearFlags & Renderer::ClearFlag::COLOR)
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
						UINT direct3D12ClearFlags = (clearFlags & Renderer::ClearFlag::DEPTH) ? D3D12_CLEAR_FLAG_DEPTH : 0u;
						if (clearFlags & Renderer::ClearFlag::STENCIL)
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
		else
		{
			// In case no render target is currently set we don't have to do anything in here
		}

		// End debug event
		RENDERER_END_DEBUG_EVENT(this)
	}

	void Direct3D12Renderer::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Direct3D 12 emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 12 draws must not be zero")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		// TODD(co) Add multi-draw-indirect support
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-draw-indirect emulation");
			}
		#endif
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Renderer::DrawArguments& drawArguments = *reinterpret_cast<const Renderer::DrawArguments*>(emulationData);

			// Draw
			mD3D12GraphicsCommandList->DrawInstanced(
				drawArguments.vertexCountPerInstance,	// Vertex count per instance (UINT)
				drawArguments.instanceCount,			// Instance count (UINT)
				drawArguments.startVertexLocation,		// Start vertex location (UINT)
				drawArguments.startInstanceLocation		// Start instance location (UINT)
			);

			// Advance
			emulationData += sizeof(Renderer::DrawArguments);
		}
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				endDebugEvent();
			}
		#endif
	}

	void Direct3D12Renderer::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Direct3D 12 emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 12 draws must not be zero")

		// TODO(co) Currently no buffer overflow check due to lack of interface provided data
		emulationData += indirectBufferOffset;

		// Emit the draw calls
		// TODD(co) Add multi-indexed-draw-indirect support
		#ifdef RENDERER_DEBUG
			if (numberOfDraws > 1)
			{
				beginDebugEvent("Multi-indexed-draw-indirect emulation");
			}
		#endif
		for (uint32_t i = 0; i < numberOfDraws; ++i)
		{
			const Renderer::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Renderer::DrawIndexedArguments*>(emulationData);

			// Draw
			mD3D12GraphicsCommandList->DrawIndexedInstanced(
				drawIndexedArguments.indexCountPerInstance,	// Index count per instance (UINT)
				drawIndexedArguments.instanceCount,			// Instance count (UINT)
				drawIndexedArguments.startIndexLocation,	// Start index location (UINT)
				drawIndexedArguments.baseVertexLocation,	// Base vertex location (INT)
				drawIndexedArguments.startInstanceLocation	// Start instance location (UINT)
			);

			// Advance
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
	void Direct3D12Renderer::setComputeRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != rootSignature)
		{
			// Sanity check
			DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)

			// Set compute root signature
			mD3D12GraphicsCommandList->SetComputeRootSignature(static_cast<RootSignature*>(rootSignature)->getD3D12RootSignature());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Renderer::setComputePipelineState(Renderer::IComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *computePipelineState)

			// Set compute pipeline state
			mD3D12GraphicsCommandList->SetPipelineState(static_cast<ComputePipelineState*>(computePipelineState)->getD3D12ComputePipelineState());
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D12Renderer::setComputeResourceGroup(uint32_t, Renderer::IResourceGroup*)
	{
		// TODO(co) Implement resource group
		RENDERER_ASSERT(mContext, false, "Direct3D 12 setComputeResourceGroup() isn't implemented, yet")
	}

	void Direct3D12Renderer::dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		mD3D12GraphicsCommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void Direct3D12Renderer::resolveMultisampleFramebuffer(Renderer::IRenderTarget&, Renderer::IFramebuffer&)
	{
		// TODO(co) Implement me
	}

	void Direct3D12Renderer::copyResource(Renderer::IResource&, Renderer::IResource&)
	{
		// TODO(co) Implement me
	}

	void Direct3D12Renderer::generateMipmaps(Renderer::IResource&)
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void Direct3D12Renderer::resetQueryPool([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity check
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
	}

	void Direct3D12Renderer::beginQuery([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex, [[maybe_unused]] uint32_t queryControlFlags)
	{
		// Sanity check
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
	}

	void Direct3D12Renderer::endQuery([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
	}

	void Direct3D12Renderer::writeTimestampQuery([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void Direct3D12Renderer::setDebugMarker(const char* name)
		{
			if (nullptr != mD3D12GraphicsCommandList)
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Direct3D 12 debug marker names must not be a null pointer")
				const UINT size = static_cast<UINT>((strlen(name) + 1) * sizeof(name[0]));
				mD3D12GraphicsCommandList->SetMarker(PIX_EVENT_ANSI_VERSION, name, size);
			}
		}

		void Direct3D12Renderer::beginDebugEvent(const char* name)
		{
			if (nullptr != mD3D12GraphicsCommandList)
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Direct3D 12 debug event names must not be a null pointer")
				const UINT size = static_cast<UINT>((strlen(name) + 1) * sizeof(name[0]));
				mD3D12GraphicsCommandList->BeginEvent(PIX_EVENT_ANSI_VERSION, name, size);
			}
		}

		void Direct3D12Renderer::endDebugEvent()
		{
			if (nullptr != mD3D12GraphicsCommandList)
			{
				mD3D12GraphicsCommandList->EndEvent();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	bool Direct3D12Renderer::isDebugEnabled()
	{
		#ifdef RENDERER_DEBUG
			return true;
		#else
			return false;
		#endif
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t Direct3D12Renderer::getNumberOfShaderLanguages() const
	{
		uint32_t numberOfShaderLanguages = 1;	// HLSL support is always there

		// Done, return the number of supported shader languages
		return numberOfShaderLanguages;
	}

	const char* Direct3D12Renderer::getShaderLanguageName(uint32_t index) const
	{
		// HLSL supported
		if (0 == index)
		{
			return ::detail::HLSL_NAME;
		}

		// Error!
		return nullptr;
	}

	Renderer::IShaderLanguage* Direct3D12Renderer::getShaderLanguage(const char* shaderLanguageName)
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
					mShaderLanguageHlsl = RENDERER_NEW(mContext, ShaderLanguageHlsl)(*this);
					mShaderLanguageHlsl->addReference();	// Internal renderer reference
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
	Renderer::IRenderPass* Direct3D12Renderer::createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples)
	{
		return RENDERER_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples);
	}

	Renderer::IQueryPool* Direct3D12Renderer::createQueryPool([[maybe_unused]] Renderer::QueryType queryType, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// TODO(co) Implement me
		return nullptr;
	}

	Renderer::ISwapChain* Direct3D12Renderer::createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool)
	{
		// Sanity checks
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)
		RENDERER_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Direct3D 12: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RENDERER_NEW(mContext, SwapChain)(renderPass, windowHandle);
	}

	Renderer::IFramebuffer* Direct3D12Renderer::createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment)
	{
		// Sanity check
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)

		// Create the framebuffer
		return RENDERER_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment);
	}

	Renderer::IBufferManager* Direct3D12Renderer::createBufferManager()
	{
		return RENDERER_NEW(mContext, BufferManager)(*this);
	}

	Renderer::ITextureManager* Direct3D12Renderer::createTextureManager()
	{
		return RENDERER_NEW(mContext, TextureManager)(*this);
	}

	Renderer::IRootSignature* Direct3D12Renderer::createRootSignature(const Renderer::RootSignature& rootSignature)
	{
		return RENDERER_NEW(mContext, RootSignature)(*this, rootSignature);
	}

	Renderer::IGraphicsPipelineState* Direct3D12Renderer::createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "Direct3D 12: Invalid graphics pipeline state root signature")
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "Direct3D 12: Invalid graphics pipeline state graphics program")
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "Direct3D 12: Invalid graphics pipeline state render pass")

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

	Renderer::IComputePipelineState* Direct3D12Renderer::createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader)
	{
		// Sanity checks
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, rootSignature)
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, computeShader)

		// Create the compute pipeline state
		uint16_t id = 0;
		if (ComputePipelineStateMakeId.CreateID(id))
		{
			return RENDERER_NEW(mContext, ComputePipelineState)(*this, rootSignature, computeShader, id);
		}

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();
		return nullptr;
	}

	Renderer::ISamplerState* Direct3D12Renderer::createSamplerState(const Renderer::SamplerState& samplerState)
	{
		return RENDERER_NEW(mContext, SamplerState)(*this, samplerState);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool Direct3D12Renderer::map(Renderer::IResource&, uint32_t, Renderer::MapType, uint32_t, Renderer::MappedSubresource&)
	{
		// TODO(co) Direct3D 12 update
		/*
		// The "Renderer::MapType" values directly map to Direct3D 10 & 11 & 12 constants, do not change them
		// The "Renderer::MappedSubresource" structure directly maps to Direct3D 12, do not change it

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				return (S_OK == mD3D12DeviceContext->Map(static_cast<IndexBuffer&>(resource).getD3D12Buffer(), subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Renderer::ResourceType::VERTEX_BUFFER:
				return (S_OK == mD3D12DeviceContext->Map(static_cast<VertexBuffer&>(resource).getD3D12Buffer(), subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Renderer::ResourceType::TEXTURE_BUFFER:
				return (S_OK == mD3D12DeviceContext->Map(static_cast<TextureBuffer&>(resource).getD3D12Buffer(), subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Renderer::ResourceType::STRUCTURED_BUFFER:
				return (S_OK == mD3D12DeviceContext->Map(static_cast<StructuredBuffer&>(resource).getD3D12Buffer(), subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Renderer::ResourceType::UNIFORM_BUFFER:
				return (S_OK == mD3D12DeviceContext->Map(static_cast<UniformBuffer&>(resource).getD3D12Buffer(), subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

			case Renderer::ResourceType::TEXTURE_2D:
			{
				bool result = false;

				// Begin debug event
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

				// Get the Direct3D 12 resource instance
				ID3D12Resource *d3d12Resource = nullptr;
				static_cast<Texture2D&>(resource).getD3D12ShaderResourceView()->GetResource(&d3d12Resource);
				if (nullptr != d3d12Resource)
				{
					// Map the Direct3D 12 resource
					result = (S_OK == mD3D12DeviceContext->Map(d3d12Resource, subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

					// Release the Direct3D 12 resource instance
					d3d12Resource->Release();
				}

				// End debug event
				RENDERER_END_DEBUG_EVENT(this)

				// Done
				return result;
			}

			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			{
				bool result = false;

				// Begin debug event
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

				// Get the Direct3D 12 resource instance
				ID3D12Resource *d3d12Resource = nullptr;
				static_cast<Texture2DArray&>(resource).getD3D12ShaderResourceView()->GetResource(&d3d12Resource);
				if (nullptr != d3d12Resource)
				{
					// Map the Direct3D 12 resource
					result = (S_OK == mD3D12DeviceContext->Map(d3d12Resource, subresource, static_cast<D3D12_MAP>(mapType), mapFlags, reinterpret_cast<D3D12_MAPPED_SUBRESOURCE*>(&mappedSubresource)));

					// Release the Direct3D 12 resource instance
					d3d12Resource->Release();
				}

				// End debug event
				RENDERER_END_DEBUG_EVENT(this)

				// Done
				return result;
			}

			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
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
		*/
		return false;
	}

	void Direct3D12Renderer::unmap(Renderer::IResource&, uint32_t)
	{
		// TODO(co) Direct3D 12 update
		/*
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				mD3D12DeviceContext->Unmap(static_cast<IndexBuffer&>(resource).getD3D12Buffer(), subresource);
				break;

			case Renderer::ResourceType::VERTEX_BUFFER:
				mD3D12DeviceContext->Unmap(static_cast<VertexBuffer&>(resource).getD3D12Buffer(), subresource);
				break;

			case Renderer::ResourceType::TEXTURE_BUFFER:
				mD3D12DeviceContext->Unmap(static_cast<TextureBuffer&>(resource).getD3D12Buffer(), subresource);
				break;

			case Renderer::ResourceType::STRUCTURED_BUFFER:
				mD3D12DeviceContext->Unmap(static_cast<StructuredBuffer&>(resource).getD3D12Buffer(), subresource);
				break;

			case Renderer::ResourceType::UNIFORM_BUFFER:
				mD3D12DeviceContext->Unmap(static_cast<UniformBuffer&>(resource).getD3D12Buffer(), subresource);
				break;

			case Renderer::ResourceType::TEXTURE_2D:
			{
				// Get the Direct3D 12 resource instance
				ID3D12Resource *d3d12Resource = nullptr;
				static_cast<Texture2D&>(resource).getD3D12ShaderResourceView()->GetResource(&d3d12Resource);
				if (nullptr != d3d12Resource)
				{
					// Unmap the Direct3D 12 resource
					mD3D12DeviceContext->Unmap(d3d12Resource, subresource);

					// Release the Direct3D 12 resource instance
					d3d12Resource->Release();
				}
				break;
			}

			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
			{
				// Get the Direct3D 12 resource instance
				ID3D12Resource *d3d12Resource = nullptr;
				static_cast<Texture2DArray&>(resource).getD3D12ShaderResourceView()->GetResource(&d3d12Resource);
				if (nullptr != d3d12Resource)
				{
					// Unmap the Direct3D 12 resource
					mD3D12DeviceContext->Unmap(d3d12Resource, subresource);

					// Release the Direct3D 12 resource instance
					d3d12Resource->Release();
				}
				break;
			}

			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
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
		*/
	}

	bool Direct3D12Renderer::getQueryPoolResults([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t numberOfDataBytes, [[maybe_unused]] uint8_t* data, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries, [[maybe_unused]] uint32_t strideInBytes, [[maybe_unused]] uint32_t queryResultFlags)
	{
		// Sanity check
		DIRECT3D12RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
		return false;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool Direct3D12Renderer::beginScene()
	{
		bool result = false;	// Error by default

		// Begin debug event
		RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

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
		}

		// End debug event
		RENDERER_END_DEBUG_EVENT(this)

		// Done
		return result;
	}

	void Direct3D12Renderer::submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer)
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

	void Direct3D12Renderer::endScene()
	{
		// Not required when using Direct3D 12
		// TODO(co) Until we have a command list interface, we must perform the command list handling in here

		// Begin debug event
		RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);

		// Close and execute the command list
		if (SUCCEEDED(mD3D12GraphicsCommandList->Close()))
		{
			ID3D12CommandList* commandLists[] = { mD3D12GraphicsCommandList };
			mD3D12CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		}

		// End debug event
		RENDERER_END_DEBUG_EVENT(this)
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void Direct3D12Renderer::flush()
	{
		// TODO(co) Direct3D 12 update
	//	mD3D12DeviceContext->Flush();
	}

	void Direct3D12Renderer::finish()
	{
		// TODO(co) Direct3D 12 update
		/*
		// Create the Direct3D 12 query instance used for flush right now?
		if (nullptr == mD3D12QueryFlush)
		{
			D3D12_QUERY_DESC d3d12QueryDesc;
			d3d12QueryDesc.Query	 = D3D12_QUERY_EVENT;
			d3d12QueryDesc.MiscFlags = 0;
			mD3D12Device->CreateQuery(&d3d12QueryDesc, &mD3D12QueryFlush);

			#ifdef RENDERER_DEBUG
				// Set the debug name
				if (nullptr != mD3D12QueryFlush)
				{
					// No need to reset the previous private data, there shouldn't be any...
					mD3D12QueryFlush->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(__FUNCTION__)), __FUNCTION__);
				}
			#endif
		}
		if (nullptr != mD3D12QueryFlush)
		{
			// Perform the flush and wait
			mD3D12DeviceContext->End(mD3D12QueryFlush);
			mD3D12DeviceContext->Flush();
			BOOL result = FALSE;
			do
			{
				// Spin-wait
				mD3D12DeviceContext->GetData(mD3D12QueryFlush, &result, sizeof(BOOL), 0);
			} while (!result);
		}
		*/
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void Direct3D12Renderer::initializeCapabilities()
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
		mCapabilities.preferredSwapChainColorTextureFormat		  = Renderer::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;

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
				// TODO(co) Direct3D 12 update
				// mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
				mCapabilities.maximumNumberOfViewports = 8;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 8192;

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
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
				mCapabilities.maximumNumberOfViewports = 8;

				// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
				// TODO(co) Direct3D 12 update
				//mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
				mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

				// Maximum texture dimension
				mCapabilities.maximumTextureDimension = 8192;

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

				// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
				mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

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

		// Direct3D 12 has native multi-threading // TODO(co) But do only set this to true if it has been tested
		mCapabilities.nativeMultiThreading = false;

		// Direct3D 12 has shader bytecode support
		// TODO(co) Implement shader bytecode support
		mCapabilities.shaderBytecode = false;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = true;
	}

	#ifdef RENDERER_DEBUG
		void Direct3D12Renderer::debugReportLiveDeviceObjects()
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
} // Direct3D12Renderer




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RENDERER_DIRECT3D12_EXPORTS
	#define DIRECT3D12RENDERER_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define DIRECT3D12RENDERER_FUNCTION_EXPORT
#endif
DIRECT3D12RENDERER_FUNCTION_EXPORT Renderer::IRenderer* createDirect3D12RendererInstance(const Renderer::Context& context)
{
	return RENDERER_NEW(context, Direct3D12Renderer::Direct3D12Renderer)(context);
}
#undef DIRECT3D12RENDERER_FUNCTION_EXPORT
