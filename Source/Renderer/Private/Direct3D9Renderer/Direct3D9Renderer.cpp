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
*    Direct3D 9 renderer amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    Direct3D 9 runtime and Direct3D 9 capable graphics driver, nothing else.
*
*    == Preprocessor Definitions ==
*    - Set "RENDERER_DIRECT3D9_EXPORTS" as preprocessor definition when building this library as shared library
*    - Do also have a look into the renderer header file documentation
*
*    - == Direct3D 9 Debugging ==
*    - Unlike Direct3D 10 & 11, the Direct3D debug layer is not application controlled in Direct3D 9
*    - This means that it has to be configured outside of our application
*    - Use the tool "dxcpl.exe" from the DirectX SDK to switch to the debug version of Direct3D 9, do also setup the desired debug output level
*    - When running the application by using Visual Studio, you can now see Direct3D 9 debug information inside the output window
*/


// TODO(co) Add device lost handling if needed. Probably more complex to recreate all device resources.


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
//[ Direct3D9Renderer/MakeID.h                            ]
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
//[ Direct3D9Renderer/d3d9.h                              ]
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
struct D3DRECT;
struct D3DCAPS9;
struct D3DMATRIX;
struct D3DLIGHT9;
struct D3DXMACRO;
struct IDirect3D9;
struct D3DXMATRIX;
struct D3DXVECTOR4;
struct D3DVIEWPORT9;
struct D3DMATERIAL9;
struct D3DGAMMARAMP;
struct ID3DXInclude;
struct D3DLOCKED_RECT;
struct D3DCLIPSTATUS9;
struct IDirect3DQuery9;
struct D3DSURFACE_DESC;
struct D3DRASTER_STATUS;
struct IDirect3DDevice9;
struct D3DTRIPATCH_INFO;
struct D3DXCONSTANT_DESC;
struct D3DRECTPATCH_INFO;
struct IDirect3DSurface9;
struct IDirect3DTexture9;
struct D3DVERTEXELEMENT9;
struct LPDIRECT3DTEXTURE9;
struct D3DINDEXBUFFER_DESC;
struct IDirect3DSwapChain9;
struct D3DVERTEXBUFFER_DESC;
struct IDirect3DStateBlock9;
struct IDirect3DBaseTexture9;
struct D3DPRESENT_PARAMETERS;
struct IDirect3DCubeTexture9;
struct IDirect3DIndexBuffer9;
struct IDirect3DPixelShader9;
struct D3DXCONSTANTTABLE_DESC;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexShader9;
struct IDirect3DVolumeTexture9;
struct IDirect3DVertexDeclaration9;
struct D3DDEVICE_CREATION_PARAMETERS;


//[-------------------------------------------------------]
//[ Definitions                                           ]
//[-------------------------------------------------------]
// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
#define D3D_SDK_VERSION						(32 | 0x80000000)
#define D3D_OK								S_OK
#define D3DADAPTER_DEFAULT					0
#define D3DCREATE_HARDWARE_VERTEXPROCESSING	0x00000040L

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
typedef DWORD D3DCOLOR;
#define MAXD3DDECLLENGTH				64
#define D3DSTREAMSOURCE_INSTANCEDATA	(2<<30)
#define D3DLOCK_READONLY				0x00000010L
#define D3DUSAGE_WRITEONLY				0x00000008L
#define D3DUSAGE_DYNAMIC				0x00000200L
#define D3DUSAGE_AUTOGENMIPMAP			0x00000400L
#define D3DUSAGE_RENDERTARGET			0x00000001L
#ifndef MAKEFOURCC
	#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
				((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
				((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif
#define D3DCOLOR_ARGB(a,r,g,b)			((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a)			D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_COLORVALUE(r,g,b,a)	D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))
#define D3DPS_VERSION(_Major,_Minor)	(0xFFFF0000|((_Major)<<8)|(_Minor))

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
#define D3DDMAPSAMPLER				256
#define D3DVERTEXTEXTURESAMPLER1	(D3DDMAPSAMPLER+2)
#define D3DVERTEXTEXTURESAMPLER2	(D3DDMAPSAMPLER+3)
#define D3DVERTEXTEXTURESAMPLER3	(D3DDMAPSAMPLER+4)
#define D3DCLEAR_TARGET				0x00000001l
#define D3DCLEAR_ZBUFFER			0x00000002l
#define D3DCLEAR_STENCIL			0x00000004l
#define D3DSTREAMSOURCE_INDEXEDDATA	(1 << 30)
#define D3DISSUE_END				(1 << 0)
#define D3DGETDATA_FLUSH			(1 << 0)

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
#define MAX_DEVICE_IDENTIFIER_STRING 512
typedef struct _D3DADAPTER_IDENTIFIER9
{
	char				Driver[MAX_DEVICE_IDENTIFIER_STRING];
	char				Description[MAX_DEVICE_IDENTIFIER_STRING];
	char				DeviceName[32];
	#ifdef _WIN32
		LARGE_INTEGER	DriverVersion;
	#else
		DWORD			DriverVersionLowPart;
		DWORD			DriverVersionHighPart;
	#endif
	DWORD				VendorId;
	DWORD				DeviceId;
	DWORD				SubSysId;
	DWORD				Revision;
	GUID				DeviceIdentifier;
	DWORD				WHQLLevel;
} D3DADAPTER_IDENTIFIER9;

// "Microsoft Direct3D SDK (June 2010)" -> "d3dx9tex.h"
#define D3DX_FILTER_NONE	(1 << 0)

// "Microsoft Direct3D SDK (June 2010)" -> "d3dx9shader.h"
#define D3DXSHADER_DEBUG				(1 << 0)
#define D3DXSHADER_SKIPVALIDATION		(1 << 1)
#define D3DXSHADER_SKIPOPTIMIZATION		(1 << 2)
#define D3DXSHADER_IEEE_STRICTNESS		(1 << 13)
#define D3DXSHADER_OPTIMIZATION_LEVEL0	(1 << 14)
#define D3DXSHADER_OPTIMIZATION_LEVEL1	0
#define D3DXSHADER_OPTIMIZATION_LEVEL2	((1 << 14) | (1 << 15))
#define D3DXSHADER_OPTIMIZATION_LEVEL3	(1 << 15)

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DDEVTYPE
{
	D3DDEVTYPE_HAL			= 1,
	D3DDEVTYPE_REF			= 2,
	D3DDEVTYPE_SW			= 3,
	D3DDEVTYPE_NULLREF		= 4,
	D3DDEVTYPE_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DFORMAT
{
	D3DFMT_UNKNOWN				=  0,
	D3DFMT_R8G8B8				= 20,
	D3DFMT_A8R8G8B8				= 21,
	D3DFMT_X8R8G8B8				= 22,
	D3DFMT_R5G6B5				= 23,
	D3DFMT_X1R5G5B5				= 24,
	D3DFMT_A1R5G5B5				= 25,
	D3DFMT_A4R4G4B4				= 26,
	D3DFMT_R3G3B2				= 27,
	D3DFMT_A8					= 28,
	D3DFMT_A8R3G3B2				= 29,
	D3DFMT_X4R4G4B4				= 30,
	D3DFMT_A2B10G10R10			= 31,
	D3DFMT_A8B8G8R8				= 32,
	D3DFMT_X8B8G8R8				= 33,
	D3DFMT_G16R16				= 34,
	D3DFMT_A2R10G10B10			= 35,
	D3DFMT_A16B16G16R16			= 36,
	D3DFMT_A8P8					= 40,
	D3DFMT_P8					= 41,
	D3DFMT_L8					= 50,
	D3DFMT_A8L8					= 51,
	D3DFMT_A4L4					= 52,
	D3DFMT_V8U8					= 60,
	D3DFMT_L6V5U5				= 61,
	D3DFMT_X8L8V8U8				= 62,
	D3DFMT_Q8W8V8U8				= 63,
	D3DFMT_V16U16				= 64,
	D3DFMT_A2W10V10U10			= 67,
	D3DFMT_UYVY					= MAKEFOURCC('U', 'Y', 'V', 'Y'),
	D3DFMT_R8G8_B8G8			= MAKEFOURCC('R', 'G', 'B', 'G'),
	D3DFMT_YUY2					= MAKEFOURCC('Y', 'U', 'Y', '2'),
	D3DFMT_G8R8_G8B8			= MAKEFOURCC('G', 'R', 'G', 'B'),
	D3DFMT_DXT1					= MAKEFOURCC('D', 'X', 'T', '1'),
	D3DFMT_DXT2					= MAKEFOURCC('D', 'X', 'T', '2'),
	D3DFMT_DXT3					= MAKEFOURCC('D', 'X', 'T', '3'),
	D3DFMT_DXT4					= MAKEFOURCC('D', 'X', 'T', '4'),
	D3DFMT_DXT5					= MAKEFOURCC('D', 'X', 'T', '5'),
	D3DFMT_D16_LOCKABLE			= 70,
	D3DFMT_D32					= 71,
	D3DFMT_D15S1				= 73,
	D3DFMT_D24S8				= 75,
	D3DFMT_D24X8				= 77,
	D3DFMT_D24X4S4				= 79,
	D3DFMT_D16					= 80,
	D3DFMT_D32F_LOCKABLE		= 82,
	D3DFMT_D24FS8				= 83,
	D3DFMT_D32_LOCKABLE			= 84,
	D3DFMT_S8_LOCKABLE			= 85,
	D3DFMT_L16					= 81,
	D3DFMT_VERTEXDATA			= 100,
	D3DFMT_INDEX16				= 101,
	D3DFMT_INDEX32				= 102,
	D3DFMT_Q16W16V16U16			= 110,
	D3DFMT_MULTI2_ARGB8			= MAKEFOURCC('M','E','T','1'),
	D3DFMT_R16F					= 111,
	D3DFMT_G16R16F				= 112,
	D3DFMT_A16B16G16R16F		= 113,
	D3DFMT_R32F					= 114,
	D3DFMT_G32R32F				= 115,
	D3DFMT_A32B32G32R32F		= 116,
	D3DFMT_CxV8U8				= 117,
	D3DFMT_A1					= 118,
	D3DFMT_A2B10G10R10_XR_BIAS	= 119,
	D3DFMT_BINARYBUFFER			= 199,
	D3DFMT_FORCE_DWORD			= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DMULTISAMPLE_TYPE
{
	D3DMULTISAMPLE_NONE			=  0,
	D3DMULTISAMPLE_NONMASKABLE	=  1,
	D3DMULTISAMPLE_2_SAMPLES	=  2,
	D3DMULTISAMPLE_3_SAMPLES	=  3,
	D3DMULTISAMPLE_4_SAMPLES	=  4,
	D3DMULTISAMPLE_5_SAMPLES	=  5,
	D3DMULTISAMPLE_6_SAMPLES	=  6,
	D3DMULTISAMPLE_7_SAMPLES	=  7,
	D3DMULTISAMPLE_8_SAMPLES	=  8,
	D3DMULTISAMPLE_9_SAMPLES	=  9,
	D3DMULTISAMPLE_10_SAMPLES	= 10,
	D3DMULTISAMPLE_11_SAMPLES	= 11,
	D3DMULTISAMPLE_12_SAMPLES	= 12,
	D3DMULTISAMPLE_13_SAMPLES	= 13,
	D3DMULTISAMPLE_14_SAMPLES	= 14,
	D3DMULTISAMPLE_15_SAMPLES	= 15,
	D3DMULTISAMPLE_16_SAMPLES	= 16,
	D3DMULTISAMPLE_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DPOOL
{
	D3DPOOL_DEFAULT		= 0,
	D3DPOOL_MANAGED		= 1,
	D3DPOOL_SYSTEMMEM	= 2,
	D3DPOOL_SCRATCH		= 3,
	D3DPOOL_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DTEXTUREFILTERTYPE
{
	D3DTEXF_NONE			= 0,
	D3DTEXF_POINT			= 1,
	D3DTEXF_LINEAR			= 2,
	D3DTEXF_ANISOTROPIC		= 3,
	D3DTEXF_PYRAMIDALQUAD	= 6,
	D3DTEXF_GAUSSIANQUAD	= 7,
	D3DTEXF_CONVOLUTIONMONO	= 8,
	D3DTEXF_FORCE_DWORD		= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DTEXTUREADDRESS
{
	D3DTADDRESS_WRAP		= 1,
	D3DTADDRESS_MIRROR		= 2,
	D3DTADDRESS_CLAMP		= 3,
	D3DTADDRESS_BORDER		= 4,
	D3DTADDRESS_MIRRORONCE	= 5,
	D3DTADDRESS_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DDECLTYPE
{
	D3DDECLTYPE_FLOAT1		=  0,
	D3DDECLTYPE_FLOAT2		=  1,
	D3DDECLTYPE_FLOAT3		=  2,
	D3DDECLTYPE_FLOAT4		=  3,
	D3DDECLTYPE_D3DCOLOR	=  4,
	D3DDECLTYPE_UBYTE4		=  5,
	D3DDECLTYPE_SHORT2		=  6,
	D3DDECLTYPE_SHORT4		=  7,
	D3DDECLTYPE_UBYTE4N		=  8,
	D3DDECLTYPE_SHORT2N		=  9,
	D3DDECLTYPE_SHORT4N		= 10,
	D3DDECLTYPE_USHORT2N	= 11,
	D3DDECLTYPE_USHORT4N	= 12,
	D3DDECLTYPE_UDEC3		= 13,
	D3DDECLTYPE_DEC3N		= 14,
	D3DDECLTYPE_FLOAT16_2	= 15,
	D3DDECLTYPE_FLOAT16_4	= 16,
	D3DDECLTYPE_UNUSED		= 17
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DDECLUSAGE
{
	D3DDECLUSAGE_POSITION = 0,
	D3DDECLUSAGE_BLENDWEIGHT,
	D3DDECLUSAGE_BLENDINDICES,
	D3DDECLUSAGE_NORMAL,
	D3DDECLUSAGE_PSIZE,
	D3DDECLUSAGE_TEXCOORD,
	D3DDECLUSAGE_TANGENT,
	D3DDECLUSAGE_BINORMAL,
	D3DDECLUSAGE_TESSFACTOR,
	D3DDECLUSAGE_POSITIONT,
	D3DDECLUSAGE_COLOR,
	D3DDECLUSAGE_FOG,
	D3DDECLUSAGE_DEPTH,
	D3DDECLUSAGE_SAMPLE,
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DTRANSFORMSTATETYPE
{
	D3DTS_VIEW			= 2,
	D3DTS_PROJECTION	= 3,
	D3DTS_TEXTURE0		= 16,
	D3DTS_TEXTURE1		= 17,
	D3DTS_TEXTURE2		= 18,
	D3DTS_TEXTURE3		= 19,
	D3DTS_TEXTURE4		= 20,
	D3DTS_TEXTURE5		= 21,
	D3DTS_TEXTURE6		= 22,
	D3DTS_TEXTURE7		= 23,
	D3DTS_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DFILLMODE
{
	D3DFILL_POINT		= 1,
	D3DFILL_WIREFRAME	= 2,
	D3DFILL_SOLID		= 3,
	D3DFILL_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DCULL
{
	D3DCULL_NONE		= 1,
	D3DCULL_CW			= 2,
	D3DCULL_CCW			= 3,
	D3DCULL_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
typedef enum D3DCMPFUNC
{
	D3DCMP_NEVER		= 1,
	D3DCMP_LESS			= 2,
	D3DCMP_EQUAL		= 3,
	D3DCMP_LESSEQUAL	= 4,
	D3DCMP_GREATER		= 5,
	D3DCMP_NOTEQUAL		= 6,
	D3DCMP_GREATEREQUAL	= 7,
	D3DCMP_ALWAYS		= 8,
	D3DCMP_FORCE_DWORD	= 0x7fffffff
} D3DCMPFUNC, *LPD3DCMPFUNC;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DRENDERSTATETYPE
{
	D3DRS_ZENABLE						= 7,
	D3DRS_FILLMODE						= 8,
	D3DRS_SHADEMODE						= 9,
	D3DRS_ZWRITEENABLE					= 14,
	D3DRS_ALPHATESTENABLE				= 15,
	D3DRS_LASTPIXEL						= 16,
	D3DRS_SRCBLEND						= 19,
	D3DRS_DESTBLEND						= 20,
	D3DRS_CULLMODE						= 22,
	D3DRS_ZFUNC							= 23,
	D3DRS_ALPHAREF						= 24,
	D3DRS_ALPHAFUNC						= 25,
	D3DRS_DITHERENABLE					= 26,
	D3DRS_ALPHABLENDENABLE				= 27,
	D3DRS_FOGENABLE						= 28,
	D3DRS_SPECULARENABLE				= 29,
	D3DRS_FOGCOLOR						= 34,
	D3DRS_FOGTABLEMODE					= 35,
	D3DRS_FOGSTART						= 36,
	D3DRS_FOGEND						= 37,
	D3DRS_FOGDENSITY					= 38,
	D3DRS_RANGEFOGENABLE				= 48,
	D3DRS_STENCILENABLE					= 52,
	D3DRS_STENCILFAIL					= 53,
	D3DRS_STENCILZFAIL					= 54,
	D3DRS_STENCILPASS					= 55,
	D3DRS_STENCILFUNC					= 56,
	D3DRS_STENCILREF					= 57,
	D3DRS_STENCILMASK					= 58,
	D3DRS_STENCILWRITEMASK				= 59,
	D3DRS_TEXTUREFACTOR					= 60,
	D3DRS_WRAP0							= 128,
	D3DRS_WRAP1							= 129,
	D3DRS_WRAP2							= 130,
	D3DRS_WRAP3							= 131,
	D3DRS_WRAP4							= 132,
	D3DRS_WRAP5							= 133,
	D3DRS_WRAP6							= 134,
	D3DRS_WRAP7							= 135,
	D3DRS_CLIPPING						= 136,
	D3DRS_LIGHTING						= 137,
	D3DRS_AMBIENT						= 139,
	D3DRS_FOGVERTEXMODE					= 140,
	D3DRS_COLORVERTEX					= 141,
	D3DRS_LOCALVIEWER					= 142,
	D3DRS_NORMALIZENORMALS				= 143,
	D3DRS_DIFFUSEMATERIALSOURCE			= 145,
	D3DRS_SPECULARMATERIALSOURCE		= 146,
	D3DRS_AMBIENTMATERIALSOURCE			= 147,
	D3DRS_EMISSIVEMATERIALSOURCE		= 148,
	D3DRS_VERTEXBLEND					= 151,
	D3DRS_CLIPPLANEENABLE				= 152,
	D3DRS_POINTSIZE						= 154,
	D3DRS_POINTSIZE_MIN					= 155,
	D3DRS_POINTSPRITEENABLE				= 156,
	D3DRS_POINTSCALEENABLE				= 157,
	D3DRS_POINTSCALE_A					= 158,
	D3DRS_POINTSCALE_B					= 159,
	D3DRS_POINTSCALE_C					= 160,
	D3DRS_MULTISAMPLEANTIALIAS			= 161,
	D3DRS_MULTISAMPLEMASK				= 162,
	D3DRS_PATCHEDGESTYLE				= 163,
	D3DRS_DEBUGMONITORTOKEN				= 165,
	D3DRS_POINTSIZE_MAX					= 166,
	D3DRS_INDEXEDVERTEXBLENDENABLE		= 167,
	D3DRS_COLORWRITEENABLE				= 168,
	D3DRS_TWEENFACTOR					= 170,
	D3DRS_BLENDOP						= 171,
	D3DRS_POSITIONDEGREE				= 172,
	D3DRS_NORMALDEGREE					= 173,
	D3DRS_SCISSORTESTENABLE				= 174,
	D3DRS_SLOPESCALEDEPTHBIAS			= 175,
	D3DRS_ANTIALIASEDLINEENABLE			= 176,
	D3DRS_MINTESSELLATIONLEVEL			= 178,
	D3DRS_MAXTESSELLATIONLEVEL			= 179,
	D3DRS_ADAPTIVETESS_X				= 180,
	D3DRS_ADAPTIVETESS_Y				= 181,
	D3DRS_ADAPTIVETESS_Z				= 182,
	D3DRS_ADAPTIVETESS_W				= 183,
	D3DRS_ENABLEADAPTIVETESSELLATION	= 184,
	D3DRS_TWOSIDEDSTENCILMODE			= 185,
	D3DRS_CCW_STENCILFAIL				= 186,
	D3DRS_CCW_STENCILZFAIL				= 187,
	D3DRS_CCW_STENCILPASS				= 188,
	D3DRS_CCW_STENCILFUNC				= 189,
	D3DRS_COLORWRITEENABLE1				= 190,
	D3DRS_COLORWRITEENABLE2				= 191,
	D3DRS_COLORWRITEENABLE3				= 192,
	D3DRS_BLENDFACTOR					= 193,
	D3DRS_SRGBWRITEENABLE				= 194,
	D3DRS_DEPTHBIAS						= 195,
	D3DRS_WRAP8							= 198,
	D3DRS_WRAP9							= 199,
	D3DRS_WRAP10						= 200,
	D3DRS_WRAP11						= 201,
	D3DRS_WRAP12						= 202,
	D3DRS_WRAP13						= 203,
	D3DRS_WRAP14						= 204,
	D3DRS_WRAP15						= 205,
	D3DRS_SEPARATEALPHABLENDENABLE		= 206,
	D3DRS_SRCBLENDALPHA					= 207,
	D3DRS_DESTBLENDALPHA				= 208,
	D3DRS_BLENDOPALPHA					= 209,
	D3DRS_FORCE_DWORD					= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DSTATEBLOCKTYPE
{
	D3DSBT_ALL			= 1,
	D3DSBT_PIXELSTATE	= 2,
	D3DSBT_VERTEXSTATE	= 3,
	D3DSBT_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DTEXTURESTAGESTATETYPE
{
	D3DTSS_COLOROP					=  1,
	D3DTSS_COLORARG1				=  2,
	D3DTSS_COLORARG2				=  3,
	D3DTSS_ALPHAOP					=  4,
	D3DTSS_ALPHAARG1				=  5,
	D3DTSS_ALPHAARG2				=  6,
	D3DTSS_BUMPENVMAT00				=  7,
	D3DTSS_BUMPENVMAT01				=  8,
	D3DTSS_BUMPENVMAT10				=  9,
	D3DTSS_BUMPENVMAT11				= 10,
	D3DTSS_TEXCOORDINDEX			= 11,
	D3DTSS_BUMPENVLSCALE			= 22,
	D3DTSS_BUMPENVLOFFSET			= 23,
	D3DTSS_TEXTURETRANSFORMFLAGS	= 24,
	D3DTSS_COLORARG0				= 26,
	D3DTSS_ALPHAARG0				= 27,
	D3DTSS_RESULTARG				= 28,
	D3DTSS_CONSTANT					= 32,
	D3DTSS_FORCE_DWORD				= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DSAMPLERSTATETYPE
{
	D3DSAMP_ADDRESSU		= 1,
	D3DSAMP_ADDRESSV		= 2,
	D3DSAMP_ADDRESSW		= 3,
	D3DSAMP_BORDERCOLOR		= 4,
	D3DSAMP_MAGFILTER		= 5,
	D3DSAMP_MINFILTER		= 6,
	D3DSAMP_MIPFILTER		= 7,
	D3DSAMP_MIPMAPLODBIAS	= 8,
	D3DSAMP_MAXMIPLEVEL		= 9,
	D3DSAMP_MAXANISOTROPY	= 10,
	D3DSAMP_SRGBTEXTURE		= 11,
	D3DSAMP_ELEMENTINDEX	= 12,
	D3DSAMP_DMAPOFFSET		= 13,
	D3DSAMP_FORCE_DWORD		= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DPRIMITIVETYPE
{
	D3DPT_POINTLIST		= 1,
	D3DPT_LINELIST		= 2,
	D3DPT_LINESTRIP		= 3,
	D3DPT_TRIANGLELIST	= 4,
	D3DPT_TRIANGLESTRIP	= 5,
	D3DPT_TRIANGLEFAN	= 6,
	D3DPT_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DBACKBUFFER_TYPE
{
	D3DBACKBUFFER_TYPE_MONO			= 0,
	D3DBACKBUFFER_TYPE_LEFT			= 1,
	D3DBACKBUFFER_TYPE_RIGHT		= 2,
	D3DBACKBUFFER_TYPE_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DQUERYTYPE
{
	D3DQUERYTYPE_VCACHE				= 4,
	D3DQUERYTYPE_RESOURCEMANAGER	= 5,
	D3DQUERYTYPE_VERTEXSTATS		= 6,
	D3DQUERYTYPE_EVENT				= 8,
	D3DQUERYTYPE_OCCLUSION			= 9,
	D3DQUERYTYPE_TIMESTAMP			= 10,
	D3DQUERYTYPE_TIMESTAMPDISJOINT	= 11,
	D3DQUERYTYPE_TIMESTAMPFREQ		= 12,
	D3DQUERYTYPE_PIPELINETIMINGS	= 13,
	D3DQUERYTYPE_INTERFACETIMINGS	= 14,
	D3DQUERYTYPE_VERTEXTIMINGS		= 15,
	D3DQUERYTYPE_PIXELTIMINGS		= 16,
	D3DQUERYTYPE_BANDWIDTHTIMINGS	= 17,
	D3DQUERYTYPE_CACHEUTILIZATION	= 18,
	D3DQUERYTYPE_MEMORYPRESSURE		= 19
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DBLEND
{
	D3DBLEND_ZERO				= 1,
	D3DBLEND_ONE				= 2,
	D3DBLEND_SRCCOLOR			= 3,
	D3DBLEND_INVSRCCOLOR		= 4,
	D3DBLEND_SRCALPHA			= 5,
	D3DBLEND_INVSRCALPHA		= 6,
	D3DBLEND_DESTALPHA			= 7,
	D3DBLEND_INVDESTALPHA		= 8,
	D3DBLEND_DESTCOLOR			= 9,
	D3DBLEND_INVDESTCOLOR		= 10,
	D3DBLEND_SRCALPHASAT		= 11,
	D3DBLEND_BOTHSRCALPHA		= 12,
	D3DBLEND_BOTHINVSRCALPHA	= 13,
	D3DBLEND_BLENDFACTOR		= 14,
	D3DBLEND_INVBLENDFACTOR		= 15,
	D3DBLEND_SRCCOLOR2			= 16,
	D3DBLEND_INVSRCCOLOR2		= 17,
	D3DBLEND_FORCE_DWORD		= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DRESOURCETYPE
{
	D3DRTYPE_SURFACE		=  1,
	D3DRTYPE_VOLUME			=  2,
	D3DRTYPE_TEXTURE		=  3,
	D3DRTYPE_VOLUMETEXTURE	=  4,
	D3DRTYPE_CUBETEXTURE	=  5,
	D3DRTYPE_VERTEXBUFFER	=  6,
	D3DRTYPE_INDEXBUFFER	=  7,
	D3DRTYPE_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DSWAPEFFECT
{
	D3DSWAPEFFECT_DISCARD		= 1,
	D3DSWAPEFFECT_FLIP			= 2,
	D3DSWAPEFFECT_COPY			= 3,
	D3DSWAPEFFECT_OVERLAY		= 4,
	D3DSWAPEFFECT_FLIPEX		= 5,
	D3DSWAPEFFECT_FORCE_DWORD	= 0x7fffffff
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
struct D3DPRESENT_PARAMETERS
{
	UINT				BackBufferWidth;
	UINT				BackBufferHeight;
	D3DFORMAT			BackBufferFormat;
	UINT				BackBufferCount;
	D3DMULTISAMPLE_TYPE	MultiSampleType;
	DWORD				MultiSampleQuality;
	D3DSWAPEFFECT		SwapEffect;
	HWND				hDeviceWindow;
	BOOL				Windowed;
	BOOL				EnableAutoDepthStencil;
	D3DFORMAT			AutoDepthStencilFormat;
	DWORD				Flags;
	UINT				FullScreen_RefreshRateInHz;
	UINT				PresentationInterval;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
struct D3DLOCKED_RECT
{
	INT	  Pitch;
	void *pBits;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
struct D3DVIEWPORT9
{
	DWORD	X;
	DWORD	Y;
	DWORD	Width;
	DWORD	Height;
	float	MinZ;
	float	MaxZ;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
struct D3DSURFACE_DESC
{
	D3DFORMAT			Format;
	D3DRESOURCETYPE		Type;
	DWORD				Usage;
	D3DPOOL				Pool;
	D3DMULTISAMPLE_TYPE	MultiSampleType;
	DWORD				MultiSampleQuality;
	UINT				Width;
	UINT				Height;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
typedef struct IDirect3DSurface9 *LPDIRECT3DSURFACE9;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
enum D3DDECLMETHOD
{
	D3DDECLMETHOD_DEFAULT = 0,
	D3DDECLMETHOD_PARTIALU,
	D3DDECLMETHOD_PARTIALV,
	D3DDECLMETHOD_CROSSUV,
	D3DDECLMETHOD_UV,
	D3DDECLMETHOD_LOOKUP,
	D3DDECLMETHOD_LOOKUPPRESAMPLED
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
struct D3DVERTEXELEMENT9
{
	WORD	Stream;
	WORD	Offset;
	BYTE	Type;
	BYTE	Method;
	BYTE	Usage;
	BYTE	UsageIndex;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9types.h"
typedef struct _D3DDISPLAYMODE
{
	UINT		Width;
	UINT		Height;
	UINT		RefreshRate;
	D3DFORMAT	Format;
} D3DDISPLAYMODE;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9xshader.h"
typedef __interface ID3DXInclude *LPD3DXINCLUDE;
#ifndef D3DXFX_LARGEADDRESS_HANDLE
	typedef LPCSTR D3DXHANDLE;
#else
	typedef UINT_PTR D3DXHANDLE;
#endif
typedef D3DXHANDLE *LPD3DXHANDLE;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9caps.h"
#define D3DPRESENT_INTERVAL_DEFAULT		0x00000000L
#define D3DPRESENT_INTERVAL_ONE			0x00000001L
#define D3DPRESENT_INTERVAL_TWO			0x00000002L
#define D3DPRESENT_INTERVAL_THREE		0x00000004L
#define D3DPRESENT_INTERVAL_FOUR		0x00000008L
#define D3DPRESENT_INTERVAL_IMMEDIATE	0x80000000L

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9caps.h"
struct D3DVSHADERCAPS2_0
{
	DWORD Caps;
	INT DynamicFlowControlDepth;
	INT NumTemps;
	INT StaticFlowControlDepth;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9caps.h"
struct D3DPSHADERCAPS2_0
{
	DWORD Caps;
	INT DynamicFlowControlDepth;
	INT NumTemps;
	INT StaticFlowControlDepth;
	INT NumInstructionSlots;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9caps.h"
struct D3DCAPS9
{
	D3DDEVTYPE			DeviceType;
	UINT				AdapterOrdinal;
	DWORD				Caps;
	DWORD				Caps2;
	DWORD				Caps3;
	DWORD				PresentationIntervals;
	DWORD				CursorCaps;
	DWORD				DevCaps;
	DWORD				PrimitiveMiscCaps;
	DWORD				RasterCaps;
	DWORD				ZCmpCaps;
	DWORD				SrcBlendCaps;
	DWORD				DestBlendCaps;
	DWORD				AlphaCmpCaps;
	DWORD				ShadeCaps;
	DWORD				TextureCaps;
	DWORD				TextureFilterCaps;
	DWORD				CubeTextureFilterCaps;
	DWORD				VolumeTextureFilterCaps;
	DWORD				TextureAddressCaps;
	DWORD				VolumeTextureAddressCaps;
	DWORD				LineCaps;
	DWORD				MaxTextureWidth, MaxTextureHeight;
	DWORD				MaxVolumeExtent;
	DWORD				MaxTextureRepeat;
	DWORD				MaxTextureAspectRatio;
	DWORD				MaxAnisotropy;
	float				MaxVertexW;
	float				GuardBandLeft;
	float				GuardBandTop;
	float				GuardBandRight;
	float				GuardBandBottom;
	float				ExtentsAdjust;
	DWORD				StencilCaps;
	DWORD				FVFCaps;
	DWORD				TextureOpCaps;
	DWORD				MaxTextureBlendStages;
	DWORD				MaxSimultaneousTextures;
	DWORD				VertexProcessingCaps;
	DWORD				MaxActiveLights;
	DWORD				MaxUserClipPlanes;
	DWORD				MaxVertexBlendMatrices;
	DWORD				MaxVertexBlendMatrixIndex;
	float				MaxPointSize;
	DWORD				MaxPrimitiveCount;
	DWORD				MaxVertexIndex;
	DWORD				MaxStreams;
	DWORD				MaxStreamStride;
	DWORD				VertexShaderVersion;
	DWORD				MaxVertexShaderConst;
	DWORD				PixelShaderVersion;
	float				PixelShader1xMaxValue;
	DWORD				DevCaps2;
	float				MaxNpatchTessellationLevel;
	DWORD				Reserved5;
	UINT				MasterAdapterOrdinal;
	UINT				AdapterOrdinalInGroup;
	UINT				NumberOfAdaptersInGroup;
	DWORD				DeclTypes;
	DWORD				NumSimultaneousRTs;
	DWORD				StretchRectFilterCaps;
	D3DVSHADERCAPS2_0	VS20Caps;
	D3DPSHADERCAPS2_0	PS20Caps;
	DWORD				VertexTextureFilterCaps;
	DWORD				MaxVShaderInstructionsExecuted;
	DWORD				MaxPShaderInstructionsExecuted;
	DWORD				MaxVertexShader30InstructionSlots;
	DWORD				MaxPixelShader30InstructionSlots;
};


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3D9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(RegisterSoftwareDevice)(THIS_ void* pInitializeFunction) PURE;
	STDMETHOD_(UINT, GetAdapterCount)(THIS) PURE;
	STDMETHOD(GetAdapterIdentifier)(THIS_ UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier) PURE;
	STDMETHOD_(UINT, GetAdapterModeCount)(THIS_ UINT Adapter,D3DFORMAT Format) PURE;
	STDMETHOD(EnumAdapterModes)(THIS_ UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode) PURE;
	STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT Adapter,D3DDISPLAYMODE* pMode) PURE;
	STDMETHOD(CheckDeviceType)(THIS_ UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed) PURE;
	STDMETHOD(CheckDeviceFormat)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,D3DRESOURCETYPE RType,D3DFORMAT CheckFormat) PURE;
	STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels) PURE;
	STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,D3DFORMAT DepthStencilFormat) PURE;
	STDMETHOD(CheckDeviceFormatConversion)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SourceFormat,D3DFORMAT TargetFormat) PURE;
	STDMETHOD(GetDeviceCaps)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps) PURE;
	STDMETHOD_(HMONITOR, GetAdapterMonitor)(THIS_ UINT Adapter) PURE;
	STDMETHOD(CreateDevice)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DDevice9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(TestCooperativeLevel)(THIS) PURE;
	STDMETHOD_(UINT, GetAvailableTextureMem)(THIS) PURE;
	STDMETHOD(EvictManagedResources)(THIS) PURE;
	STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9) PURE;
	STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps) PURE;
	STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode) PURE;
	STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters) PURE;
	STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) PURE;
	STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags) PURE;
	STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow) PURE;
	STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) PURE;
	STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) PURE;
	STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS) PURE;
	STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) PURE;
	STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) PURE;
	STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) PURE;
	STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) PURE;
	STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs) PURE;
	STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) PURE;
	STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp) PURE;
	STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) PURE;
	STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) PURE;
	STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) PURE;
	STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) PURE;
	STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) PURE;
	STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) PURE;
	STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) PURE;
	STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) PURE;
	STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) PURE;
	STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) PURE;
	STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,IDirect3DSurface9* pDestSurface) PURE;
	STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) PURE;
	STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) PURE;
	STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) PURE;
	STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) PURE;
	STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) PURE;
	STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil) PURE;
	STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface) PURE;
	STDMETHOD(BeginScene)(THIS) PURE;
	STDMETHOD(EndScene)(THIS) PURE;
	STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) PURE;
	STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) PURE;
	STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) PURE;
	STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*) PURE;
	STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport) PURE;
	STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport) PURE;
	STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial) PURE;
	STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial) PURE;
	STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9*) PURE;
	STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9*) PURE;
	STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable) PURE;
	STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable) PURE;
	STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane) PURE;
	STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane) PURE;
	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value) PURE;
	STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue) PURE;
	STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) PURE;
	STDMETHOD(BeginStateBlock)(THIS) PURE;
	STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB) PURE;
	STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus) PURE;
	STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus) PURE;
	STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture) PURE;
	STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture) PURE;
	STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) PURE;
	STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) PURE;
	STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) PURE;
	STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) PURE;
	STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses) PURE;
	STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries) PURE;
	STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries) PURE;
	STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber) PURE;
	STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber) PURE;
	STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect) PURE;
	STDMETHOD(GetScissorRect)(THIS_ RECT* pRect) PURE;
	STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware) PURE;
	STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS) PURE;
	STDMETHOD(SetNPatchMode)(THIS_ float nSegments) PURE;
	STDMETHOD_(float, GetNPatchMode)(THIS) PURE;
	STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) PURE;
	STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) PURE;
	STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) PURE;
	STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) PURE;
	STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) PURE;
	STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) PURE;
	STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl) PURE;
	STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl) PURE;
	STDMETHOD(SetFVF)(THIS_ DWORD FVF) PURE;
	STDMETHOD(GetFVF)(THIS_ DWORD* pFVF) PURE;
	STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) PURE;
	STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader) PURE;
	STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader) PURE;
	STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) PURE;
	STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) PURE;
	STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) PURE;
	STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) PURE;
	STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) PURE;
	STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) PURE;
	STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) PURE;
	STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) PURE;
	STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Setting) PURE;
	STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* pSetting) PURE;
	STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData) PURE;
	STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData) PURE;
	STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) PURE;
	STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader) PURE;
	STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader) PURE;
	STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) PURE;
	STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) PURE;
	STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) PURE;
	STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) PURE;
	STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) PURE;
	STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) PURE;
	STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) PURE;
	STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) PURE;
	STDMETHOD(DeletePatch)(THIS_ UINT Handle) PURE;
	STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) PURE;
};
typedef struct IDirect3DDevice9 *LPDIRECT3DDEVICE9;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DVertexDeclaration9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9* pElement,UINT* pNumElements) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DVertexShader9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(GetFunction)(THIS_ void*,UINT* pSizeOfData) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DPixelShader9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(GetFunction)(THIS_ void*,UINT* pSizeOfData) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DResource9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) PURE;
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) PURE;
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
	STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
	STDMETHOD_(void, PreLoad)(THIS) PURE;
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DIndexBuffer9, IDirect3DResource9)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) PURE;
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) PURE;
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
	STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
	STDMETHOD_(void, PreLoad)(THIS) PURE;
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) PURE;
	STDMETHOD(Lock)(THIS_ UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags) PURE;
	STDMETHOD(Unlock)(THIS) PURE;
	STDMETHOD(GetDesc)(THIS_ D3DINDEXBUFFER_DESC *pDesc) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DVertexBuffer9, IDirect3DResource9)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) PURE;
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) PURE;
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
	STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
	STDMETHOD_(void, PreLoad)(THIS) PURE;
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) PURE;
	STDMETHOD(Lock)(THIS_ UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags) PURE;
	STDMETHOD(Unlock)(THIS) PURE;
	STDMETHOD(GetDesc)(THIS_ D3DVERTEXBUFFER_DESC *pDesc) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DSurface9, IDirect3DResource9)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) PURE;
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) PURE;
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
	STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
	STDMETHOD_(void, PreLoad)(THIS) PURE;
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) PURE;
	STDMETHOD(GetContainer)(THIS_ REFIID riid,void** ppContainer) PURE;
	STDMETHOD(GetDesc)(THIS_ D3DSURFACE_DESC *pDesc) PURE;
	STDMETHOD(LockRect)(THIS_ D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) PURE;
	STDMETHOD(UnlockRect)(THIS) PURE;
	STDMETHOD(GetDC)(THIS_ HDC *phdc) PURE;
	STDMETHOD(ReleaseDC)(THIS_ HDC hdc) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DBaseTexture9, IDirect3DResource9)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) PURE;
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) PURE;
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
	STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
	STDMETHOD_(void, PreLoad)(THIS) PURE;
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) PURE;
	STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew) PURE;
	STDMETHOD_(DWORD, GetLOD)(THIS) PURE;
	STDMETHOD_(DWORD, GetLevelCount)(THIS) PURE;
	STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType) PURE;
	STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS) PURE;
	STDMETHOD_(void, GenerateMipSubLevels)(THIS) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DTexture9, IDirect3DBaseTexture9)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) PURE;
	STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) PURE;
	STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
	STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
	STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
	STDMETHOD_(void, PreLoad)(THIS) PURE;
	STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) PURE;
	STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew) PURE;
	STDMETHOD_(DWORD, GetLOD)(THIS) PURE;
	STDMETHOD_(DWORD, GetLevelCount)(THIS) PURE;
	STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType) PURE;
	STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS) PURE;
	STDMETHOD_(void, GenerateMipSubLevels)(THIS) PURE;
	STDMETHOD(GetLevelDesc)(THIS_ UINT Level,D3DSURFACE_DESC *pDesc) PURE;
	STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel) PURE;
	STDMETHOD(LockRect)(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) PURE;
	STDMETHOD(UnlockRect)(THIS_ UINT Level) PURE;
	STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9xcore.h"
DECLARE_INTERFACE_(ID3DXBuffer, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
	STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
};
typedef __interface ID3DXBuffer *LPD3DXBUFFER;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9xshader.h"
DECLARE_INTERFACE_(ID3DXConstantTable, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
	STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
	STDMETHOD(GetDesc)(THIS_ D3DXCONSTANTTABLE_DESC *pDesc) PURE;
	STDMETHOD(GetConstantDesc)(THIS_ D3DXHANDLE hConstant, D3DXCONSTANT_DESC *pConstantDesc, UINT *pCount) PURE;
	STDMETHOD_(UINT, GetSamplerIndex)(THIS_ D3DXHANDLE hConstant) PURE;
	STDMETHOD_(D3DXHANDLE, GetConstant)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
	STDMETHOD_(D3DXHANDLE, GetConstantByName)(THIS_ D3DXHANDLE hConstant, LPCSTR pName) PURE;
	STDMETHOD_(D3DXHANDLE, GetConstantElement)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
	STDMETHOD(SetDefaults)(THIS_ LPDIRECT3DDEVICE9 pDevice) PURE;
	STDMETHOD(SetValue)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, LPCVOID pData, UINT Bytes) PURE;
	STDMETHOD(SetBool)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, BOOL b) PURE;
	STDMETHOD(SetBoolArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST BOOL* pb, UINT Count) PURE;
	STDMETHOD(SetInt)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, INT n) PURE;
	STDMETHOD(SetIntArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST INT* pn, UINT Count) PURE;
	STDMETHOD(SetFloat)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, FLOAT f) PURE;
	STDMETHOD(SetFloatArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST FLOAT* pf, UINT Count) PURE;
	STDMETHOD(SetVector)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector) PURE;
	STDMETHOD(SetVectorArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
	STDMETHOD(SetMatrix)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
	STDMETHOD(SetMatrixArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
	STDMETHOD(SetMatrixPointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
	STDMETHOD(SetMatrixTranspose)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
	STDMETHOD(SetMatrixTransposeArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
	STDMETHOD(SetMatrixTransposePointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
};
typedef __interface ID3DXConstantTable *LPD3DXCONSTANTTABLE;

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DQuery9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD_(D3DQUERYTYPE, GetType)(THIS) PURE;
	STDMETHOD_(DWORD, GetDataSize)(THIS) PURE;
	STDMETHOD(Issue)(THIS_ DWORD dwIssueFlags) PURE;
	STDMETHOD(GetData)(THIS_ void* pData,DWORD dwSize,DWORD dwGetDataFlags) PURE;
};

// "Microsoft Direct3D SDK (June 2010)" -> "d3d9.h"
DECLARE_INTERFACE_(IDirect3DSwapChain9, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) PURE;
	STDMETHOD(GetFrontBufferData)(THIS_ IDirect3DSurface9* pDestSurface) PURE;
	STDMETHOD(GetBackBuffer)(THIS_ UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) PURE;
	STDMETHOD(GetRasterStatus)(THIS_ D3DRASTER_STATUS* pRasterStatus) PURE;
	STDMETHOD(GetDisplayMode)(THIS_ D3DDISPLAYMODE* pMode) PURE;
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) PURE;
	STDMETHOD(GetPresentParameters)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) PURE;
};

// See "Advanced DX9 Capabilities for ATI Radeon Cards" by "AMD Graphics Products Group" - "Texture Formats: ATI2N and ATI1N" - http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2012/10/Advanced-DX9-Capabilities-for-ATI-Radeon-Cards_v2.pdf
#define FOURCC_ATI1N ((D3DFORMAT)MAKEFOURCC('A', 'T', 'I', '1'))
#define FOURCC_ATI2N ((D3DFORMAT)MAKEFOURCC('A', 'T', 'I', '2'))




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Direct3D9Renderer
{
	class RootSignature;
	class Direct3D9RuntimeLinking;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
#ifdef RENDERER_DEBUG
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given renderer
	*/
	#define DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference) \
		RENDERER_ASSERT(mContext, &rendererReference == &(resourceReference).getRenderer(), "Direct3D 9 error: The given resource is owned by another renderer instance")

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
	#define DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference)

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
namespace Direct3D9Renderer
{




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Direct3D9Renderer.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 renderer class
	*/
	class Direct3D9Renderer final : public Renderer::IRenderer
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
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*
		*  @note
		*    - Do never ever use a not properly initialized renderer! Use "Renderer::IRenderer::isInitialized()" to check the initialization state.
		*/
		explicit Direct3D9Renderer(const Renderer::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Direct3D9Renderer() override;

		/**
		*  @brief
		*    Return the Direct3D 9 instance
		*
		*  @return
		*    The Direct3D 9 instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3D9* getDirect3D9() const
		{
			return mDirect3D9;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 device instance
		*
		*  @return
		*    The Direct3D 9 device instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DDevice9* getDirect3DDevice9() const
		{
			return mDirect3DDevice9;
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
			return "Direct3D9";
		}

		[[nodiscard]] inline virtual bool isInitialized() const override
		{
			// Is there a Direct3D 9 instance?
			return (nullptr != mDirect3D9);
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
			RENDERER_DELETE(mContext, Direct3D9Renderer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D9Renderer(const Direct3D9Renderer& source) = delete;
		Direct3D9Renderer& operator =(const Direct3D9Renderer& source) = delete;

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
		void setGraphicsProgram(Renderer::IGraphicsProgram* graphicsProgram);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D9RuntimeLinking*   mDirect3D9RuntimeLinking;	///< Direct3D 9 runtime linking instance, always valid
		IDirect3D9*				   mDirect3D9;					///< Instance of the Direct3D 9 object, can be a null pointer (we don't check because this would be a total overhead, the user has to use "Renderer::IRenderer::isInitialized()" and is asked to never ever use a not properly initialized renderer!)
		IDirect3DDevice9*		   mDirect3DDevice9;			///< Direct3D 9 rendering device, can be a null pointer (we don't check because this would be a total overhead, the user has to use "Renderer::IRenderer::isInitialized()" and is asked to never ever use a not properly initialized renderer!)
		Renderer::IShaderLanguage* mShaderLanguageHlsl;			///< HLSL shader language instance (we keep a reference to it), can be a null pointer
		IDirect3DQuery9*		   mDirect3DQuery9Flush;		///< Direct3D 9 query used for flush, can be a null pointer
		RootSignature*			   mGraphicsRootSignature;		///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		Renderer::ISamplerState*   mDefaultSamplerState;		///< Default rasterizer state (we keep a reference to it), can be a null pointer
		// Input-assembler (IA) stage
		Renderer::PrimitiveTopology mPrimitiveTopology;	///< Primitive topology describing the type of primitive to render
		// Output-merger (OM) stage
		Renderer::IRenderTarget* mRenderTarget;	///< Currently set render target (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant Direct3D 9 calls
		IDirect3DVertexShader9* mDirect3DVertexShader9;
		IDirect3DPixelShader9*  mDirect3DPixelShader9;


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Direct3D9RuntimeLinking.h           ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Macros & definitions                                  ]
	//[-------------------------------------------------------]
	// Redirect D3D9* and D3DX9* function calls to funcPtr_D3D9* and funcPtr_D3DX9*
	#ifndef FNPTR
		#define FNPTR(name) funcPtr_##name
	#endif

	//[-------------------------------------------------------]
	//[ D3D9 core functions                                   ]
	//[-------------------------------------------------------]
	#define FNDEF_D3D9(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_D3D9(IDirect3D9*,	Direct3DCreate9,	(UINT));
	#define Direct3DCreate9	FNPTR(Direct3DCreate9)
	// Debug
	FNDEF_D3D9(DWORD,		D3DPERF_GetStatus,	(void));
	FNDEF_D3D9(void,		D3DPERF_SetOptions,	(DWORD));
	#define D3DPERF_GetStatus	FNPTR(D3DPERF_GetStatus)
	#define D3DPERF_SetOptions	FNPTR(D3DPERF_SetOptions)
	#ifdef RENDERER_DEBUG
		FNDEF_D3D9(void,	D3DPERF_SetMarker,	(D3DCOLOR, LPCWSTR));
		FNDEF_D3D9(int,		D3DPERF_BeginEvent,	(D3DCOLOR, LPCWSTR));
		FNDEF_D3D9(int,		D3DPERF_EndEvent,	(void));
		#define D3DPERF_SetMarker	FNPTR(D3DPERF_SetMarker)
		#define D3DPERF_BeginEvent	FNPTR(D3DPERF_BeginEvent)
		#define D3DPERF_EndEvent	FNPTR(D3DPERF_EndEvent)
	#endif

	//[-------------------------------------------------------]
	//[ D3DX9 functions                                       ]
	//[-------------------------------------------------------]
	#define FNDEF_D3DX9(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_D3DX9(HRESULT,	D3DXLoadSurfaceFromMemory,	(LPDIRECT3DSURFACE9, CONST PALETTEENTRY*, CONST RECT*, LPCVOID, D3DFORMAT, UINT, CONST PALETTEENTRY*, CONST RECT*, DWORD, D3DCOLOR));
	FNDEF_D3DX9(HRESULT,	D3DXCompileShader,			(LPCSTR, UINT, CONST D3DXMACRO*, LPD3DXINCLUDE, LPCSTR, LPCSTR, DWORD, LPD3DXBUFFER*, LPD3DXBUFFER*, LPD3DXCONSTANTTABLE*));
	FNDEF_D3DX9(HRESULT,	D3DXGetShaderConstantTable,	(const DWORD*, LPD3DXCONSTANTTABLE*));
	#define D3DXLoadSurfaceFromMemory	FNPTR(D3DXLoadSurfaceFromMemory)
	#define D3DXCompileShader			FNPTR(D3DXCompileShader)
	#define D3DXGetShaderConstantTable	FNPTR(D3DXGetShaderConstantTable)

	/**
	*  @brief
	*    Direct3D 9 runtime linking
	*/
	class Direct3D9RuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*/
		inline explicit Direct3D9RuntimeLinking(Direct3D9Renderer& direct3D9Renderer) :
			mDirect3D9Renderer(direct3D9Renderer),
			mD3D9SharedLibrary(nullptr),
			mD3DX9SharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		~Direct3D9RuntimeLinking()
		{
			// Destroy the shared library instances
			if (nullptr != mD3D9SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3D9SharedLibrary));
			}
			if (nullptr != mD3DX9SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3DX9SharedLibrary));
			}
		}

		/**
		*  @brief
		*    Return whether or not Direct3D 9 is available
		*
		*  @return
		*    "true" if Direct3D 9 is available, else "false"
		*/
		[[nodiscard]] bool isDirect3D9Avaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the D3D9 and D3DX9 entry points
					mEntryPointsRegistered = (loadD3D9EntryPoints() && loadD3DX9EntryPoints());
				}
			}

			// Entry points successfully registered?
			return mEntryPointsRegistered;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D9RuntimeLinking(const Direct3D9RuntimeLinking& source) = delete;
		Direct3D9RuntimeLinking& operator =(const Direct3D9RuntimeLinking& source) = delete;

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
			mD3D9SharedLibrary = ::LoadLibraryExA("d3d9.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (nullptr != mD3D9SharedLibrary)
			{
				mD3DX9SharedLibrary = ::LoadLibraryExA("d3dx9_43.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr == mD3DX9SharedLibrary)
				{
					RENDERER_LOG(mDirect3D9Renderer.getContext(), CRITICAL, "Failed to load in the Direct3D 9 shared library \"d3dx9_43.dll\"")
				}
			}
			else
			{
				RENDERER_LOG(mDirect3D9Renderer.getContext(), CRITICAL, "Failed to load in the Direct3D 9 shared library \"d3d9.dll\"")
			}

			// Done
			return (nullptr != mD3D9SharedLibrary && nullptr != mD3DX9SharedLibrary);
		}

		/**
		*  @brief
		*    Load the D3D9 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3D9EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3D9SharedLibrary), #funcName);																							\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3D9SharedLibrary), moduleFilename, MAX_PATH);																					\
						RENDERER_LOG(mDirect3D9Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 9 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			IMPORT_FUNC(Direct3DCreate9);
			IMPORT_FUNC(D3DPERF_GetStatus);
			IMPORT_FUNC(D3DPERF_SetOptions);
			#ifdef RENDERER_DEBUG
				IMPORT_FUNC(D3DPERF_SetMarker);
				IMPORT_FUNC(D3DPERF_BeginEvent);
				IMPORT_FUNC(D3DPERF_EndEvent);
			#endif

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the D3DX9 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3DX9EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3DX9SharedLibrary), #funcName);																							\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3DX9SharedLibrary), moduleFilename, MAX_PATH);																					\
						RENDERER_LOG(mDirect3D9Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 9 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			IMPORT_FUNC(D3DXLoadSurfaceFromMemory);
			IMPORT_FUNC(D3DXCompileShader);
			IMPORT_FUNC(D3DXGetShaderConstantTable);

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Direct3D9Renderer& mDirect3D9Renderer;
		void*			   mD3D9SharedLibrary;		///< D3D9 shared library, can be a null pointer
		void*			   mD3DX9SharedLibrary;		///< D3DX9 shared library, can be a null pointer
		bool			   mEntryPointsRegistered;	///< Entry points successfully registered?
		bool			   mInitialized;			///< Already initialized?


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
	*  @param[in]  shaderModel
	*    ASCII shader model (for example "vs_3_0", "ps_3_0")
	*  @param[in] sourceCode
	*    Shader ASCII source code, must be a valid pointer
	*  @param[in]  entryPoint
	*    Optional ASCII entry point, if null pointer "main" is used
	*  @param[in] optimizationLevel
	*    Optimization level
	*  @param[out] d3dXConstantTable
	*    Optional constant table, can be a null pointer
	*
	*  @return
	*    The loaded and compiled shader, can be a null pointer, release the instance if you no longer need it
	*/
	[[nodiscard]] ID3DXBuffer* loadShaderFromSourcecode(const Renderer::Context& context, const char* shaderModel, const char* sourceCode, const char* entryPoint, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, ID3DXConstantTable** d3dXConstantTable)
	{
		// Sanity checks
		RENDERER_ASSERT(context, nullptr != shaderModel, "Invalid Direct3D 9 shader model")
		RENDERER_ASSERT(context, nullptr != sourceCode, "Invalid Direct3D 9 shader source code")

		// Get compile flags
		UINT compileFlags = D3DXSHADER_IEEE_STRICTNESS;
		switch (optimizationLevel)
		{
			case Renderer::IShaderLanguage::OptimizationLevel::Debug:
				compileFlags |= D3DXSHADER_DEBUG;
				compileFlags |= D3DXSHADER_SKIPOPTIMIZATION;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::None:
				compileFlags |= D3DXSHADER_SKIPVALIDATION;
				compileFlags |= D3DXSHADER_SKIPOPTIMIZATION;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::Low:
				compileFlags |= D3DXSHADER_SKIPVALIDATION;
				compileFlags |= D3DXSHADER_OPTIMIZATION_LEVEL0;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::Medium:
				compileFlags |= D3DXSHADER_SKIPVALIDATION;
				compileFlags |= D3DXSHADER_OPTIMIZATION_LEVEL1;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::High:
				compileFlags |= D3DXSHADER_SKIPVALIDATION;
				compileFlags |= D3DXSHADER_OPTIMIZATION_LEVEL2;
				break;

			case Renderer::IShaderLanguage::OptimizationLevel::Ultra:
				compileFlags |= D3DXSHADER_OPTIMIZATION_LEVEL3;
				break;
		}

		ID3DXBuffer* d3dXBuffer = nullptr;
		ID3DXBuffer* d3dXBufferErrorMessages = nullptr;
		if (D3D_OK != D3DXCompileShader(sourceCode, static_cast<UINT>(strlen(sourceCode)), nullptr, nullptr, entryPoint ? entryPoint : "main", shaderModel, compileFlags, &d3dXBuffer, &d3dXBufferErrorMessages, d3dXConstantTable))
		{
			if (context.getLog().print(Renderer::ILog::Type::CRITICAL, sourceCode, __FILE__, static_cast<uint32_t>(__LINE__), static_cast<char*>(d3dXBufferErrorMessages->GetBufferPointer())))
			{
				DEBUG_BREAK;
			}
			d3dXBufferErrorMessages->Release();
		}

		// Done
		return d3dXBuffer;
	}




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Mapping.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 mapping
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
		*    "Renderer::FilterMode" to Direct3D 9 magnification filter mode
		*
		*  @param[in] context
		*    Used renderer context
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    Direct3D 9 magnification filter mode
		*/
		[[nodiscard]] static D3DTEXTUREFILTERTYPE getDirect3D9MagFilterMode([[maybe_unused]] const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return D3DTEXF_ANISOTROPIC;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "Direct3D 9 filter mode must not be unknown")
					return D3DTEXF_POINT;

				default:
					return D3DTEXF_POINT;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Renderer::FilterMode" to Direct3D 9 minification filter mode
		*
		*  @param[in] context
		*    Used renderer context
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    Direct3D 9 minification filter mode
		*/
		[[nodiscard]] static D3DTEXTUREFILTERTYPE getDirect3D9MinFilterMode([[maybe_unused]] const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return D3DTEXF_ANISOTROPIC;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return D3DTEXF_ANISOTROPIC;

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "Direct3D 9 filter mode must not be unknown")
					return D3DTEXF_POINT;

				default:
					return D3DTEXF_POINT;	// We should never be in here
			}
		}

		/**
		*  @brief
		*    "Renderer::FilterMode" to Direct3D 9 mipmapping filter mode
		*
		*  @param[in] context
		*    Used renderer context
		*  @param[in] filterMode
		*    "Renderer::FilterMode" to map
		*
		*  @return
		*    Direct3D 9 mipmapping filter mode
		*/
		[[nodiscard]] static D3DTEXTUREFILTERTYPE getDirect3D9MipFilterMode([[maybe_unused]] const Renderer::Context& context, Renderer::FilterMode filterMode)
		{
			switch (filterMode)
			{
				case Renderer::FilterMode::MIN_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_POINT_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_POINT_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_LINEAR_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::MIN_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::ANISOTROPIC:
					return D3DTEXF_ANISOTROPIC;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
					return D3DTEXF_POINT;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_MIN_MAG_MIP_LINEAR:
					return D3DTEXF_LINEAR;

				case Renderer::FilterMode::COMPARISON_ANISOTROPIC:
					return D3DTEXF_ANISOTROPIC;

				case Renderer::FilterMode::UNKNOWN:
					RENDERER_ASSERT(context, false, "Direct3D 9 filter mode must not be unknown")
					return D3DTEXF_POINT;

				default:
					return D3DTEXF_POINT;	// We should never be in here
			}
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureAddressMode                          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureAddressMode" to Direct3D 9 texture address mode
		*
		*  @param[in] textureAddressMode
		*    "Renderer::TextureAddressMode" to map
		*
		*  @return
		*    Direct3D 9 texture address mode
		*/
		[[nodiscard]] static D3DTEXTUREADDRESS getDirect3D9TextureAddressMode(Renderer::TextureAddressMode textureAddressMode)
		{
			static constexpr D3DTEXTUREADDRESS MAPPING[] =
			{
				D3DTADDRESS_WRAP,		// Renderer::TextureAddressMode::WRAP
				D3DTADDRESS_MIRROR,		// Renderer::TextureAddressMode::MIRROR
				D3DTADDRESS_CLAMP,		// Renderer::TextureAddressMode::CLAMP
				D3DTADDRESS_BORDER,		// Renderer::TextureAddressMode::BORDER
				D3DTADDRESS_MIRRORONCE	// Renderer::TextureAddressMode::MIRROR_ONCE
			};
			return MAPPING[static_cast<int>(textureAddressMode) - 1];	// Lookout! The "Renderer::TextureAddressMode"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::ComparisonFunc                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::ComparisonFunc" to Direct3D 9 comparison function
		*
		*  @param[in] comparisonFunc
		*    "Renderer::ComparisonFunc" to map
		*
		*  @return
		*    Direct3D 9 comparison function
		*/
		[[nodiscard]] static D3DCMPFUNC getDirect3D9ComparisonFunc(Renderer::ComparisonFunc comparisonFunc)
		{
			static constexpr D3DCMPFUNC MAPPING[] =
			{
				D3DCMP_NEVER,			// Renderer::ComparisonFunc::NEVER
				D3DCMP_LESS,			// Renderer::ComparisonFunc::LESS
				D3DCMP_EQUAL,			// Renderer::ComparisonFunc::EQUAL
				D3DCMP_LESSEQUAL,		// Renderer::ComparisonFunc::LESS_EQUAL
				D3DCMP_GREATER,			// Renderer::ComparisonFunc::GREATER
				D3DCMP_NOTEQUAL,		// Renderer::ComparisonFunc::NOT_EQUAL
				D3DCMP_GREATEREQUAL,	// Renderer::ComparisonFunc::GREATER_EQUAL
				D3DCMP_ALWAYS			// Renderer::ComparisonFunc::ALWAYS
			};
			return MAPPING[static_cast<int>(comparisonFunc) - 1];	// Lookout! The "Renderer::ComparisonFunc"-values start with 1, not 0
		}

		//[-------------------------------------------------------]
		//[ Renderer::VertexAttributeFormat and semantic          ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::VertexAttributeFormat" to Direct3D 9 type
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to map
		*
		*  @return
		*    Direct3D 9 type
		*/
		[[nodiscard]] static D3DDECLTYPE getDirect3D9Type(Renderer::VertexAttributeFormat vertexAttributeFormat)
		{
			static constexpr D3DDECLTYPE MAPPING[] =
			{
				D3DDECLTYPE_FLOAT1,		// Renderer::VertexAttributeFormat::FLOAT_1
				D3DDECLTYPE_FLOAT2,		// Renderer::VertexAttributeFormat::FLOAT_2
				D3DDECLTYPE_FLOAT3,		// Renderer::VertexAttributeFormat::FLOAT_3
				D3DDECLTYPE_FLOAT4,		// Renderer::VertexAttributeFormat::FLOAT_4
				D3DDECLTYPE_UBYTE4N,	// Renderer::VertexAttributeFormat::R8G8B8A8_UNORM
				D3DDECLTYPE_UBYTE4,		// Renderer::VertexAttributeFormat::R8G8B8A8_UINT
				D3DDECLTYPE_SHORT2,		// Renderer::VertexAttributeFormat::SHORT_2
				D3DDECLTYPE_SHORT4,		// Renderer::VertexAttributeFormat::SHORT_4
				D3DDECLTYPE_UNUSED		// Renderer::VertexAttributeFormat::UINT_1 - not supported by DirectX 9
			};
			return MAPPING[static_cast<int>(vertexAttributeFormat)];
		}

		/**
		*  @brief
		*    "Semantic as string" to Direct3D 9 semantic
		*
		*  @param[in] semanticName
		*    Semantic name as string, must be a valid pointer
		*
		*  @return
		*    Direct3D 9 semantic, "D3DDECLUSAGE_POSITION" as fallback if no match was found
		*/
		[[nodiscard]] static D3DDECLUSAGE getDirect3D9Semantic(const char* semanticName)
		{
			D3DDECLUSAGE direct3D9Semantic = D3DDECLUSAGE_POSITION;
			if (0 == stricmp("POSITION", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_POSITION;
			}
			else if (0 == stricmp("BLENDWEIGHT", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_BLENDWEIGHT;
			}
			else if (0 == stricmp("BLENDINDICES", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_BLENDINDICES;
			}
			else if (0 == stricmp("NORMAL", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_NORMAL;
			}
			else if (0 == stricmp("PSIZE", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_PSIZE;
			}
			else if (0 == stricmp("TEXCOORD", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_TEXCOORD;
			}
			else if (0 == stricmp("TANGENT", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_TANGENT;
			}
			else if (0 == stricmp("BINORMAL", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_BINORMAL;
			}
			else if (0 == stricmp("TESSFACTOR", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_TESSFACTOR;
			}
			else if (0 == stricmp("POSITIONT", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_POSITIONT;
			}
			else if (0 == stricmp("COLOR", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_COLOR;
			}
			else if (0 == stricmp("FOG", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_FOG;
			}
			else if (0 == stricmp("DEPTH", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_DEPTH;
			}
			else if (0 == stricmp("SAMPLE", semanticName))
			{
				direct3D9Semantic = D3DDECLUSAGE_SAMPLE;
			}
			return direct3D9Semantic;
		}

		//[-------------------------------------------------------]
		//[ Renderer::BufferUsage                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::BufferUsage" to Direct3D 9 usage
		*
		*  @param[in] bufferUsage
		*    "Renderer::BufferUsage" to map
		*
		*  @return
		*    Direct3D 9 usage
		*/
		[[nodiscard]] static uint32_t getDirect3D9Usage(Renderer::BufferUsage bufferUsage)
		{
			// Direct3D 9 only supports a subset of the OpenGL usage indications
			// -> See "D3DUSAGE"-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/bb172625%28v=vs.85%29.aspx
			switch (bufferUsage)
			{
				case Renderer::BufferUsage::STREAM_DRAW:
				case Renderer::BufferUsage::STREAM_COPY:
				case Renderer::BufferUsage::STATIC_DRAW:
				case Renderer::BufferUsage::STATIC_COPY:
					return D3DUSAGE_WRITEONLY;

				case Renderer::BufferUsage::STREAM_READ:
				case Renderer::BufferUsage::STATIC_READ:
					return 0;

				case Renderer::BufferUsage::DYNAMIC_DRAW:
				case Renderer::BufferUsage::DYNAMIC_COPY:
					return (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY);

				default:
				case Renderer::BufferUsage::DYNAMIC_READ:
					return D3DUSAGE_DYNAMIC;
			}
		}

		//[-------------------------------------------------------]
		//[ Renderer::IndexBufferFormat                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::IndexBufferFormat" to Direct3D 9 format
		*
		*  @param[in] indexBufferFormat
		*    "Renderer::IndexBufferFormat" to map
		*
		*  @return
		*    Direct3D 9 format
		*/
		[[nodiscard]] static D3DFORMAT getDirect3D9Format(Renderer::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr D3DFORMAT MAPPING[] =
			{
				D3DFMT_INDEX32,	// Renderer::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Direct3D 9
				D3DFMT_INDEX16,	// Renderer::IndexBufferFormat::UNSIGNED_SHORT - Two bytes per element, uint16_t
				D3DFMT_INDEX32	// Renderer::IndexBufferFormat::UNSIGNED_INT   - Four bytes per element, uint32_t (may not be supported by each API)
			};
			return MAPPING[indexBufferFormat];
		}

		//[-------------------------------------------------------]
		//[ Renderer::TextureFormat                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::TextureFormat" to Direct3D 9 format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    Direct3D 9 format
		*/
		[[nodiscard]] static D3DFORMAT getDirect3D9Format(Renderer::TextureFormat::Enum textureFormat)
		{
			static constexpr D3DFORMAT MAPPING[] =
			{
				D3DFMT_L8,					// Renderer::TextureFormat::R8            - 8-bit pixel format, all bits red
				D3DFMT_X8R8G8B8,			// Renderer::TextureFormat::R8G8B8        - 24-bit pixel format, 8 bits for red, green and blue - "D3DFMT_R8G8B8" is usually not supported
				D3DFMT_A8R8G8B8,			// Renderer::TextureFormat::R8G8B8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				D3DFMT_A8R8G8B8,			// Renderer::TextureFormat::R8G8B8A8_SRGB - 32-bit pixel format, 8 bits for red, green, blue and alpha; sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) DirectX 9 sRGB format
				D3DFMT_A8B8G8R8,			// Renderer::TextureFormat::B8G8R8A8      - 32-bit pixel format, 8 bits for red, green, blue and alpha
				D3DFMT_A16B16G16R16F,		// Renderer::TextureFormat::R11G11B10F    - 32-bit float format using 11 bits the red and green channel, 10 bits the blue channel; red and green channels have a 6 bits mantissa and a 5 bits exponent and blue has a 5 bits mantissa and 5 bits exponent - "DXGI_FORMAT_R11G11B10_FLOAT" doesn't exist in Direct3D 9
				D3DFMT_A16B16G16R16F,		// Renderer::TextureFormat::R16G16B16A16F - 64-bit float format using 16 bits for the each channel (red, green, blue, alpha)
				D3DFMT_A32B32G32R32F,		// Renderer::TextureFormat::R32G32B32A32F - 128-bit float format using 32 bits for the each channel (red, green, blue, alpha)
				D3DFMT_DXT1,				// Renderer::TextureFormat::BC1           - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
				D3DFMT_DXT1,				// Renderer::TextureFormat::BC1_SRGB      - DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) DirectX 9 sRGB format
				D3DFMT_DXT3,				// Renderer::TextureFormat::BC2           - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				D3DFMT_DXT3,				// Renderer::TextureFormat::BC2_SRGB      - DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) DirectX 9 sRGB format
				D3DFMT_DXT5,				// Renderer::TextureFormat::BC3           - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
				D3DFMT_DXT5,				// Renderer::TextureFormat::BC3_SRGB      - DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block); sRGB = RGB hardware gamma correction, the alpha channel always remains linear	- TODO(co) DirectX 9 sRGB format
				FOURCC_ATI1N,				// Renderer::TextureFormat::BC4           - 1 component texture compression (also known as 3DC+/ATI1N, known as BC4 in DirectX 10, 8 bytes per block) - See "Advanced DX9 Capabilities for ATI Radeon Cards" by "AMD Graphics Products Group" - "Texture Formats: ATI2N and ATI1N" - http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2012/10/Advanced-DX9-Capabilities-for-ATI-Radeon-Cards_v2.pdf
				FOURCC_ATI2N,				// Renderer::TextureFormat::BC5           - 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block) - See "Advanced DX9 Capabilities for ATI Radeon Cards" by "AMD Graphics Products Group" - "Texture Formats: ATI2N and ATI1N" - http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2012/10/Advanced-DX9-Capabilities-for-ATI-Radeon-Cards_v2.pdf
				D3DFMT_UNKNOWN,				// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 9
				D3DFMT_L16,					// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				D3DFMT_UNKNOWN,				// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format	TODO(co) Not available in Direct3D 9 as it looks like
				D3DFMT_R32F,				// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				D3DFMT_D32F_LOCKABLE,		// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format	TODO(co) Check depth texture format INTZ: http://aras-p.info/texts/D3D9GPUHacks.html and http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2012/10/Advanced-DX9-Capabilities-for-ATI-Radeon-Cards_v2.pdf
				D3DFMT_UNKNOWN,				// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				D3DFMT_UNKNOWN,				// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				D3DFMT_UNKNOWN				// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		//[-------------------------------------------------------]
		//[ Miscellaneous                                         ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Synchronization interval to Direct3D 9 presentation interval
		*
		*  @param[in] context
		*    Used renderer context
		*  @param[in] synchronizationInterval
		*    Synchronization interval to map
		*
		*  @return
		*    Direct3D 9 presentation interval
		*/
		[[nodiscard]] static uint32_t getDirect3D9PresentationInterval([[maybe_unused]] const Renderer::Context& context, uint32_t synchronizationInterval)
		{
			RENDERER_ASSERT(context, synchronizationInterval <= 4, "Direct3D 9 supports a maximum synchronization interval of four")
			static constexpr uint32_t MAPPING[] =
			{
				D3DPRESENT_INTERVAL_IMMEDIATE,
				D3DPRESENT_INTERVAL_ONE,
				D3DPRESENT_INTERVAL_TWO,
				D3DPRESENT_INTERVAL_THREE,
				D3DPRESENT_INTERVAL_FOUR
			};
			return MAPPING[synchronizationInterval];
		}


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/ResourceGroup.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 resource group class
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
		*  @param[in] rootParameterIndex
		*    The root parameter index number for binding
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(Renderer::IRenderer& renderer, uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates) :
			IResourceGroup(renderer),
			mRootParameterIndex(rootParameterIndex),
			mNumberOfResources(numberOfResources),
			mResources(RENDERER_MALLOC_TYPED(renderer.getContext(), Renderer::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr)
		{
			// Process all resources and add our reference to the renderer resource
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				// Since Direct3D 9 doesn't support e.g. uniform buffer we need to check for null pointers here
				Renderer::IResource* resource = *resources;
				mResources[resourceIndex] = resource;
				if (nullptr != resource)
				{
					resource->addReference();
				}
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RENDERER_MALLOC_TYPED(renderer.getContext(), Renderer::ISamplerState*, mNumberOfResources);
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
				// Since Direct3D 9 doesn't support e.g. uniform buffer we need to check for null pointers here
				if (nullptr != mResources[resourceIndex])
				{
					mResources[resourceIndex]->releaseReference();
				}
			}
			RENDERER_FREE(context, mResources);
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
		uint32_t				  mRootParameterIndex;	///< The root parameter index number for binding
		uint32_t				  mNumberOfResources;	///< Number of resources this resource group groups together
		Renderer::IResource**	  mResources;			///< Renderer resources, we keep a reference to it
		Renderer::ISamplerState** mSamplerStates;		///< Sampler states, we keep a reference to it


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/RootSignature.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(Direct3D9Renderer& direct3D9Renderer, const Renderer::RootSignature& rootSignature) :
			IRootSignature(direct3D9Renderer),
			mRootSignature(rootSignature)
		{
			const Renderer::Context& context = direct3D9Renderer.getContext();

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
		[[nodiscard]] virtual Renderer::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates = nullptr) override
		{
			// Sanity checks
			RENDERER_ASSERT(getRenderer().getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The Direct3D 9 root parameter index is out-of-bounds")
			RENDERER_ASSERT(getRenderer().getContext(), numberOfResources > 0, "The number of Direct3D 9 resources must not be zero")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr != resources, "The Direct3D 9 resource pointers must be valid")

			// Create resource group
			return RENDERER_NEW(getRenderer().getContext(), ResourceGroup)(getRenderer(), rootParameterIndex, numberOfResources, resources, samplerStates);
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
	//[ Direct3D9Renderer/Buffer/IndexBuffer.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 index buffer object (IBO, "element array buffer" in OpenGL terminology) class
	*/
	class IndexBuffer final : public Renderer::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(Direct3D9Renderer& direct3D9Renderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IIndexBuffer(direct3D9Renderer),
			mDirect3DIndexBuffer9(nullptr)
		{
			// "Renderer::IndexBufferFormat::UNSIGNED_CHAR" is not supported by Direct3D 9
			if (Renderer::IndexBufferFormat::UNSIGNED_CHAR == indexBufferFormat)
			{
				RENDERER_LOG(direct3D9Renderer.getContext(), CRITICAL, "\"Renderer::IndexBufferFormat::UNSIGNED_CHAR\" is not supported by Direct3D 9")
			}
			else
			{
				// Create the Direct3D 9 index buffer
				FAILED_DEBUG_BREAK(direct3D9Renderer.getDirect3DDevice9()->CreateIndexBuffer(numberOfBytes, Mapping::getDirect3D9Usage(bufferUsage), static_cast<D3DFORMAT>(Mapping::getDirect3D9Format(indexBufferFormat)), D3DPOOL_DEFAULT, &mDirect3DIndexBuffer9, nullptr));

				// Copy the data, if required
				if (nullptr != data)
				{
					void* indices = nullptr;
					if (SUCCEEDED(mDirect3DIndexBuffer9->Lock(0, numberOfBytes, static_cast<void**>(&indices), 0)))
					{
						memcpy(indices, data, numberOfBytes);
						mDirect3DIndexBuffer9->Unlock();
					}
				}
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
			if (nullptr != mDirect3DIndexBuffer9)
			{
				mDirect3DIndexBuffer9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D index buffer instance
		*
		*  @return
		*    The Direct3D index buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DIndexBuffer9* getDirect3DIndexBuffer9() const
		{
			return mDirect3DIndexBuffer9;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid Direct3D 9 index buffer?
				if (nullptr != mDirect3DIndexBuffer9)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "IBO", 6);	// 6 = "IBO: " including terminating zero
					FAILED_DEBUG_BREAK(mDirect3DIndexBuffer9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DIndexBuffer9->SetPrivateData(WKPDID_D3DDebugObjectName, detailedName, static_cast<UINT>(strlen(detailedName)), 0));
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
		IDirect3DIndexBuffer9* mDirect3DIndexBuffer9;	///< Direct3D index buffer instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Buffer/VertexBuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
	*/
	class VertexBuffer final : public Renderer::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(Direct3D9Renderer& direct3D9Renderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			IVertexBuffer(direct3D9Renderer),
			mDirect3DVertexBuffer9(nullptr)
		{
			// Create the Direct3D 9 vertex buffer
			FAILED_DEBUG_BREAK(direct3D9Renderer.getDirect3DDevice9()->CreateVertexBuffer(numberOfBytes, Mapping::getDirect3D9Usage(bufferUsage), 0, D3DPOOL_DEFAULT, &mDirect3DVertexBuffer9, nullptr));

			// Copy the data, if required
			if (nullptr != data)
			{
				void* vertices = nullptr;
				if (SUCCEEDED(mDirect3DVertexBuffer9->Lock(0, numberOfBytes, static_cast<void**>(&vertices), 0)))
				{
					memcpy(vertices, data, numberOfBytes);
					mDirect3DVertexBuffer9->Unlock();
				}
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
		virtual ~VertexBuffer() override
		{
			if (nullptr != mDirect3DVertexBuffer9)
			{
				mDirect3DVertexBuffer9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D vertex buffer instance
		*
		*  @return
		*    The Direct3D vertex buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DVertexBuffer9* getDirect3DVertexBuffer9() const
		{
			return mDirect3DVertexBuffer9;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid Direct3D 9 vertex buffer?
				if (nullptr != mDirect3DVertexBuffer9)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "VBO", 6);	// 6 = "VBO: " including terminating zero
					FAILED_DEBUG_BREAK(mDirect3DVertexBuffer9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DVertexBuffer9->SetPrivateData(WKPDID_D3DDebugObjectName, detailedName, static_cast<UINT>(strlen(detailedName)), 0));
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
		IDirect3DVertexBuffer9* mDirect3DVertexBuffer9;	///< Direct3D vertex buffer instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Buffer/VertexArray.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 vertex array class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
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
		VertexArray(Direct3D9Renderer& direct3D9Renderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer, uint16_t id) :
			IVertexArray(direct3D9Renderer, id),
			mDirect3DDevice9(direct3D9Renderer.getDirect3DDevice9()),
			mIndexBuffer(indexBuffer),
			mNumberOfSlots(numberOfVertexBuffers),
			mDirect3DVertexBuffer9(nullptr),
			mStrides(nullptr),
			mInstancesPerElement(nullptr),
			mVertexBuffers(nullptr)
		{
			// Acquire our Direct3D 9 device reference
			mDirect3DDevice9->AddRef();

			// Add a reference to the given index buffer
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->addReference();
			}

			// Add a reference to the used vertex buffers
			if (mNumberOfSlots > 0)
			{
				const Renderer::Context& context = direct3D9Renderer.getContext();
				mDirect3DVertexBuffer9 = RENDERER_MALLOC_TYPED(context, IDirect3DVertexBuffer9*, mNumberOfSlots);
				mStrides = RENDERER_MALLOC_TYPED(context, uint32_t, mNumberOfSlots);
				mInstancesPerElement = RENDERER_MALLOC_TYPED(context, uint32_t, mNumberOfSlots);
				mVertexBuffers = RENDERER_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					IDirect3DVertexBuffer9** currentDirect3DVertexBuffer9 = mDirect3DVertexBuffer9;
					UINT* currentInstancesPerElement = mInstancesPerElement;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentDirect3DVertexBuffer9, ++currentInstancesPerElement, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used renderer?
						*currentInstancesPerElement = 0;
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						*currentDirect3DVertexBuffer9 = (*currentVertexBuffer)->getDirect3DVertexBuffer9();
						(*currentVertexBuffer)->addReference();
					}
				}

				{ // Gather slot related data
					// TODO(co) This will not work when multiple attributes using the same slot, but with a different setting. On the other hand, Direct3D 9 is totally out-of-date and this is just a proof-of-concept.
					const Renderer::VertexAttribute* attribute = vertexAttributes.attributes;
					const Renderer::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
					for (; attribute < attributesEnd;  ++attribute)
					{
						mStrides[attribute->inputSlot] = attribute->strideInBytes;
						mInstancesPerElement[attribute->inputSlot] = attribute->instancesPerElement;
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

			// Cleanup Direct3D 9 input slot data
			const Renderer::Context& context = getRenderer().getContext();
			if (mNumberOfSlots > 0)
			{
				RENDERER_FREE(context, mDirect3DVertexBuffer9);
				RENDERER_FREE(context, mStrides);
				RENDERER_FREE(context, mInstancesPerElement);
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
				RENDERER_FREE(context, mVertexBuffers);
			}

			// Release our Direct3D 9 device reference
			mDirect3DDevice9->Release();

			// Free the unique compact vertex array ID
			static_cast<Direct3D9Renderer&>(getRenderer()).VertexArrayMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Enable the Direct3D 9 vertex declaration and stream source
		*/
		void enableDirect3DVertexDeclarationAndStreamSource() const
		{
			{ // Set the Direct3D 9 stream sources
				IDirect3DVertexBuffer9 **currentDirect3DVertexBuffer9AtSlot = mDirect3DVertexBuffer9;
				uint32_t* currentStrideAtSlot = mStrides;
				uint32_t* currentInstancesPerElementAtSlot = mInstancesPerElement;
				for (uint32_t slot = 0; slot < mNumberOfSlots; ++slot, ++currentDirect3DVertexBuffer9AtSlot, ++currentStrideAtSlot, ++currentInstancesPerElementAtSlot)
				{
					// Vertex buffer offset is not supported by OpenGL, so our renderer API doesn't support it either
					FAILED_DEBUG_BREAK(mDirect3DDevice9->SetStreamSource(slot, *currentDirect3DVertexBuffer9AtSlot, 0, *currentStrideAtSlot));

					// "D3DSTREAMSOURCE_INDEXEDDATA" is set within "Direct3D9Renderer::Direct3D9Renderer::DrawIndexedInstanced()"
					FAILED_DEBUG_BREAK(mDirect3DDevice9->SetStreamSourceFreq(1, (0 == *currentInstancesPerElementAtSlot) ? 1 : (D3DSTREAMSOURCE_INSTANCEDATA | *currentInstancesPerElementAtSlot)));
				}
			}

			// Set the used index buffer
			// -> In case of no index buffer we don't set null indices, there's not really a point in it
			if (nullptr != mIndexBuffer)
			{
				// Set the Direct3D 9 indices
				FAILED_DEBUG_BREAK(mDirect3DDevice9->SetIndices(mIndexBuffer->getDirect3DIndexBuffer9()));
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
		IDirect3DDevice9*		 mDirect3DDevice9;			///< The Direct3D 9 device instance (we keep a reference to it), null pointer on horrible error (so we don't check)
		IndexBuffer*			 mIndexBuffer;				///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Direct3D 9 input slots
		uint32_t				 mNumberOfSlots;			///< Number of used Direct3D 9 input slots
		IDirect3DVertexBuffer9** mDirect3DVertexBuffer9;	///< Direct3D 9 vertex buffers, if "mDirect3DVertexDeclaration9" is no null pointer this is no null pointer as well
		uint32_t*				 mStrides;					///< Strides in bytes, if "mDirect3DVertexBuffer9" is no null pointer this is no null pointer as well
		uint32_t*				 mInstancesPerElement;		///< Instances per element, if "mDirect3DVertexBuffer9" is no null pointer this is no null pointer as well
		// For proper vertex buffer reference counter behaviour
		VertexBuffer**			 mVertexBuffers;			///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Buffer/IndirectBuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 indirect buffer object emulation class
	*/
	class IndirectBuffer final : public Renderer::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Renderer::IndirectBufferFlag"
		*/
		IndirectBuffer(Direct3D9Renderer& direct3D9Renderer, uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t indirectBufferFlags) :
			IIndirectBuffer(direct3D9Renderer),
			mNumberOfBytes(numberOfBytes),
			mData(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D9Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Direct3D 9 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RENDERER_ASSERT(direct3D9Renderer.getContext(), !((indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Direct3D 9 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RENDERER_ASSERT(direct3D9Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawArguments)) == 0, "Direct3D 9 indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RENDERER_ASSERT(direct3D9Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawIndexedArguments)) == 0, "Direct3D 9 indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// Copy data
			if (mNumberOfBytes > 0)
			{
				mData = RENDERER_MALLOC_TYPED(direct3D9Renderer.getContext(), uint8_t, mNumberOfBytes);
				if (nullptr != data)
				{
					memcpy(mData, data, mNumberOfBytes);
				}
			}
			else
			{
				RENDERER_ASSERT(direct3D9Renderer.getContext(), nullptr == data, "Invalid Direct3D 9 indirect buffer data")
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{
			RENDERER_FREE(getRenderer().getContext(), mData);
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
		uint32_t mNumberOfBytes;
		uint8_t* mData;				///< Indirect buffer data, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Buffer/BufferManager.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 buffer manager interface
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*/
		inline explicit BufferManager(Direct3D9Renderer& direct3D9Renderer) :
			IBufferManager(direct3D9Renderer)
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
		[[nodiscard]] inline virtual Renderer::IVertexBuffer* createVertexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// TODO(co) Security checks
			return RENDERER_NEW(getRenderer().getContext(), VertexBuffer)(static_cast<Direct3D9Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
		}

		[[nodiscard]] inline virtual Renderer::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::IndexBufferFormat::Enum indexBufferFormat = Renderer::IndexBufferFormat::UNSIGNED_SHORT) override
		{
			// TODO(co) Security checks
			return RENDERER_NEW(getRenderer().getContext(), IndexBuffer)(static_cast<Direct3D9Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage, indexBufferFormat);
		}

		[[nodiscard]] inline virtual Renderer::IVertexArray* createVertexArray(const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, Renderer::IIndexBuffer* indexBuffer = nullptr) override
		{
			// Sanity checks
			#ifdef RENDERER_DEBUG
			{
				const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RENDERER_ASSERT(getRenderer().getContext(), &getRenderer() == &vertexBuffer->vertexBuffer->getRenderer(), "Direct3D 9 error: The given vertex buffer resource is owned by another renderer instance")
				}
			}
			#endif
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == indexBuffer || &getRenderer() == &indexBuffer->getRenderer(), "Direct3D 9 error: The given index buffer resource is owned by another renderer instance")

			// Create vertex array
			uint16_t id = 0;
			if (static_cast<Direct3D9Renderer&>(getRenderer()).VertexArrayMakeId.CreateID(id))
			{
				return RENDERER_NEW(getRenderer().getContext(), VertexArray)(static_cast<Direct3D9Renderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer), id);
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

		[[nodiscard]] inline virtual Renderer::ITextureBuffer* createTextureBuffer(uint32_t, const void*, uint32_t, Renderer::BufferUsage, Renderer::TextureFormat::Enum) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 doesn't support texture buffer")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IStructuredBuffer* createStructuredBuffer(uint32_t, const void*, uint32_t, Renderer::BufferUsage, uint32_t) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 doesn't support structured buffer")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, [[maybe_unused]] Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndirectBuffer)(static_cast<Direct3D9Renderer&>(getRenderer()), numberOfBytes, data, indirectBufferFlags);
		}

		[[nodiscard]] inline virtual Renderer::IUniformBuffer* createUniformBuffer(uint32_t, const void*, Renderer::BufferUsage) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 doesn't support uniform buffer")
			return nullptr;
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
	//[ Direct3D9Renderer/Texture/Texture1D.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 1D texture class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
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
		Texture1D(Direct3D9Renderer& direct3D9Renderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) :
			ITexture1D(direct3D9Renderer, width),
			mDirect3DTexture9(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D9Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 9 texture parameters")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D9Renderer)

			// Get the Direct3D 9 usage indication
			// TODO(co) Add "Renderer::TextureFlag::GENERATE_MIPMAPS" support for render target textures
			DWORD direct3D9Usage = (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) ? D3DUSAGE_AUTOGENMIPMAP : 0u;
			switch (textureUsage)
			{
				case Renderer::TextureUsage::DYNAMIC:
					direct3D9Usage |= D3DUSAGE_DYNAMIC;
					break;

				case Renderer::TextureUsage::DEFAULT:
				case Renderer::TextureUsage::IMMUTABLE:
				case Renderer::TextureUsage::STAGING:
				default:
					// "Renderer::TextureUsage::DEFAULT", "Renderer::TextureUsage::IMMUTABLE" and "Renderer::TextureUsage::STAGING" have no Direct3D 9 equivalent
					// -> See "D3DUSAGE"-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/bb172625%28v=vs.85%29.aspx
					break;
			}

			// Use this texture as render target?
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				RENDERER_ASSERT(direct3D9Renderer.getContext(), nullptr == data, "Direct3D 9 render target textures can't be filled using provided data")
				direct3D9Usage |= D3DUSAGE_RENDERTARGET;
			}

			// Get the DirextX 9 format
			const D3DFORMAT d3dFormat = static_cast<D3DFORMAT>(Mapping::getDirect3D9Format(textureFormat));

			// Create Direct3D 9 texture, let Direct3D create the mipmaps for us if requested by the user
			if (direct3D9Renderer.getDirect3DDevice9()->CreateTexture(width, 1, 0, direct3D9Usage, d3dFormat, D3DPOOL_DEFAULT, &mDirect3DTexture9, nullptr) == D3D_OK && nullptr != data)
			{
				// Upload the texture data

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS)
				{
					// Calculate the number of mipmaps
					const uint32_t numberOfMipmaps = getNumberOfMipmaps(width);

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap

						// Get the surface
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						mDirect3DTexture9->GetSurfaceLevel(mipmap, &direct3DSurface9);
						if (nullptr != direct3DSurface9)
						{
							// Upload the texture data
							const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(1) };
							FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

							// Release the surface
							direct3DSurface9->Release();
						}

						// Move on to the next mipmap and ensure the size is always at least 1
						data = static_cast<const uint8_t*>(data) + Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1);
						width = getHalfSize(width);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps

					// Get the surface
					IDirect3DSurface9* direct3DSurface9 = nullptr;
					mDirect3DTexture9->GetSurfaceLevel(0, &direct3DSurface9);
					if (nullptr != direct3DSurface9)
					{
						// Upload the texture data
						const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(1) };
						FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

						// Release the surface
						direct3DSurface9->Release();
					}
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("1D texture");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D9Renderer)
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture1D() override
		{
			if (nullptr != mDirect3DTexture9)
			{
				mDirect3DTexture9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D texture instance
		*
		*  @return
		*    The Direct3D texture instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DTexture9* getDirect3DTexture9() const
		{
			return mDirect3DTexture9;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid Direct3D 9 texture?
				if (nullptr != mDirect3DTexture9)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(strlen(name)), 0));

					// Set debug name of the texture surfaces
					const DWORD levelCount = mDirect3DTexture9->GetLevelCount();
					for (DWORD level = 0; level < levelCount; ++level)
					{
						// Get the Direct3D 9 surface
						const size_t nameLength = strlen(name);
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(level, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Set the debug name
							// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(nameLength), 0));

							// Release the Direct3D 9 surface
							direct3DSurface9->Release();
						}
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
		IDirect3DTexture9* mDirect3DTexture9;	///< Direct3D 9 texture instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Texture/Texture2D.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 2D texture class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
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
		Texture2D(Direct3D9Renderer& direct3D9Renderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) :
			ITexture2D(direct3D9Renderer, width, height),
			mDirect3DTexture9(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D9Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 9 texture parameters")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D9Renderer)

			// Get the Direct3D 9 usage indication
			// TODO(co) Add "Renderer::TextureFlag::GENERATE_MIPMAPS" support for render target textures
			DWORD direct3D9Usage = (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) ? D3DUSAGE_AUTOGENMIPMAP : 0u;
			switch (textureUsage)
			{
				case Renderer::TextureUsage::DYNAMIC:
					direct3D9Usage |= D3DUSAGE_DYNAMIC;
					break;

				case Renderer::TextureUsage::DEFAULT:
				case Renderer::TextureUsage::IMMUTABLE:
				case Renderer::TextureUsage::STAGING:
				default:
					// "Renderer::TextureUsage::DEFAULT", "Renderer::TextureUsage::IMMUTABLE" and "Renderer::TextureUsage::STAGING" have no Direct3D 9 equivalent
					// -> See "D3DUSAGE"-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/bb172625%28v=vs.85%29.aspx
					break;
			}

			// Use this texture as render target?
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				RENDERER_ASSERT(direct3D9Renderer.getContext(), nullptr == data, "Direct3D 9 render target textures can't be filled using provided data")
				direct3D9Usage |= D3DUSAGE_RENDERTARGET;
			}

			// Get the DirextX 9 format
			const D3DFORMAT d3dFormat = static_cast<D3DFORMAT>(Mapping::getDirect3D9Format(textureFormat));

			// Create Direct3D 9 texture, let Direct3D create the mipmaps for us if requested by the user
			if (direct3D9Renderer.getDirect3DDevice9()->CreateTexture(width, height, 0, direct3D9Usage, d3dFormat, D3DPOOL_DEFAULT, &mDirect3DTexture9, nullptr) == D3D_OK && nullptr != data)
			{
				// Upload the texture data

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS)
				{
					// Calculate the number of mipmaps
					const uint32_t numberOfMipmaps = getNumberOfMipmaps(width, height);

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap

						// Get the surface
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(mipmap, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Upload the texture data
							const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
							FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

							// Release the surface
							direct3DSurface9->Release();
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps

					// Get the surface
					IDirect3DSurface9* direct3DSurface9 = nullptr;
					FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(0, &direct3DSurface9));
					if (nullptr != direct3DSurface9)
					{
						// Upload the texture data
						const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
						FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

						// Release the surface
						direct3DSurface9->Release();
					}
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("2D texture");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D9Renderer)
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2D() override
		{
			if (nullptr != mDirect3DTexture9)
			{
				mDirect3DTexture9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D texture instance
		*
		*  @return
		*    The Direct3D texture instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DTexture9* getDirect3DTexture9() const
		{
			return mDirect3DTexture9;
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
				// Valid Direct3D 9 texture?
				if (nullptr != mDirect3DTexture9)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(strlen(name)), 0));

					// Set debug name of the texture surfaces
					const DWORD levelCount = mDirect3DTexture9->GetLevelCount();
					for (DWORD level = 0; level < levelCount; ++level)
					{
						// Get the Direct3D 9 surface
						const size_t nameLength = strlen(name);
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(level, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Set the debug name
							// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(nameLength), 0));

							// Release the Direct3D 9 surface
							direct3DSurface9->Release();
						}
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
		IDirect3DTexture9* mDirect3DTexture9;	///< Direct3D 9 texture instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Texture/Texture3D.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 3D texture class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
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
		Texture3D(Direct3D9Renderer& direct3D9Renderer, uint32_t width, uint32_t height, uint32_t depth, [[maybe_unused]] Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) :
			ITexture3D(direct3D9Renderer, width, height, depth),
			mDirect3DTexture9(nullptr)
		{
			// TODO(co) Implement Direct3D 9 volume texture
			/*
			// Sanity checks
			RENDERER_ASSERT(direct3D9Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 9 texture parameters")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D9Renderer)

			// Get the Direct3D 9 usage indication
			// TODO(co) Add "Renderer::TextureFlag::GENERATE_MIPMAPS" support for render target textures
			DWORD direct3D9Usage = (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) ? D3DUSAGE_AUTOGENMIPMAP : 0u;
			switch (textureUsage)
			{
				case Renderer::TextureUsage::DYNAMIC:
					direct3D9Usage |= D3DUSAGE_DYNAMIC;
					break;

				case Renderer::TextureUsage::DEFAULT:
				case Renderer::TextureUsage::IMMUTABLE:
				case Renderer::TextureUsage::STAGING:
				default:
					// "Renderer::TextureUsage::DEFAULT", "Renderer::TextureUsage::IMMUTABLE" and "Renderer::TextureUsage::STAGING" have no Direct3D 9 equivalent
					// -> See "D3DUSAGE"-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/bb172625%28v=vs.85%29.aspx
					break;
			}

			// Use this texture as render target?
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				RENDERER_ASSERT(direct3D9Renderer.getContext(), nullptr == data, "Direct3D 9 render target textures can't be filled using provided data")
				direct3D9Usage |= D3DUSAGE_RENDERTARGET;
			}

			// Get the DirextX 9 format
			const D3DFORMAT d3dFormat = static_cast<D3DFORMAT>(Mapping::getDirect3D9Format(textureFormat));

			// Create Direct3D 9 texture, let Direct3D create the mipmaps for us if requested by the user
			if (direct3D9Renderer.getDirect3DDevice9()->CreateTexture(width, height, 0, direct3D9Usage, d3dFormat, D3DPOOL_DEFAULT, &mDirect3DTexture9, nullptr) == D3D_OK && nullptr != data)
			{
				// Upload the texture data

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS)
				{
					// Calculate the number of mipmaps
					const uint32_t numberOfMipmaps = getNumberOfMipmaps(width, height, depth);

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap

						// Get the surface
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(mipmap, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Upload the texture data
							const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
							FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

							// Release the surface
							direct3DSurface9->Release();
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps

					// Get the surface
					IDirect3DSurface9* direct3DSurface9 = nullptr;
					FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(0, &direct3DSurface9));
					if (nullptr != direct3DSurface9)
					{
						// Upload the texture data
						const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
						FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

						// Release the surface
						direct3DSurface9->Release();
					}
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("3D texture");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D9Renderer)
			*/
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture3D() override
		{
			if (nullptr != mDirect3DTexture9)
			{
				mDirect3DTexture9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D texture instance
		*
		*  @return
		*    The Direct3D texture instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DTexture9* getDirect3DTexture9() const
		{
			return mDirect3DTexture9;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid Direct3D 9 texture?
				if (nullptr != mDirect3DTexture9)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(strlen(name)), 0));

					// Set debug name of the texture surfaces
					const DWORD levelCount = mDirect3DTexture9->GetLevelCount();
					for (DWORD level = 0; level < levelCount; ++level)
					{
						// Get the Direct3D 9 surface
						const size_t nameLength = strlen(name);
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(level, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Set the debug name
							// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(nameLength), 0));

							// Release the Direct3D 9 surface
							direct3DSurface9->Release();
						}
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
		IDirect3DTexture9* mDirect3DTexture9;	///< Direct3D 9 texture instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Texture/TextureCube.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 cube texture class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
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
		TextureCube(Direct3D9Renderer& direct3D9Renderer, uint32_t width, uint32_t height, [[maybe_unused]] Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t textureFlags, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) :
			ITextureCube(direct3D9Renderer, width, height),
			mDirect3DTexture9(nullptr)
		{
			// TODO(co) Implement Direct3D 9 cube texture
			/*
			// Sanity checks
			RENDERER_ASSERT(direct3D9Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 9 texture parameters")

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D9Renderer)

			// Get the Direct3D 9 usage indication
			// TODO(co) Add "Renderer::TextureFlag::GENERATE_MIPMAPS" support for render target textures
			DWORD direct3D9Usage = (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS) ? D3DUSAGE_AUTOGENMIPMAP : 0u;
			switch (textureUsage)
			{
				case Renderer::TextureUsage::DYNAMIC:
					direct3D9Usage |= D3DUSAGE_DYNAMIC;
					break;

				case Renderer::TextureUsage::DEFAULT:
				case Renderer::TextureUsage::IMMUTABLE:
				case Renderer::TextureUsage::STAGING:
				default:
					// "Renderer::TextureUsage::DEFAULT", "Renderer::TextureUsage::IMMUTABLE" and "Renderer::TextureUsage::STAGING" have no Direct3D 9 equivalent
					// -> See "D3DUSAGE"-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/bb172625%28v=vs.85%29.aspx
					break;
			}

			// Use this texture as render target?
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				RENDERER_ASSERT(direct3D9Renderer.getContext(), nullptr == data, "Direct3D 9 render target textures can't be filled using provided data")
				direct3D9Usage |= D3DUSAGE_RENDERTARGET;
			}

			// Get the DirextX 9 format
			const D3DFORMAT d3dFormat = static_cast<D3DFORMAT>(Mapping::getDirect3D9Format(textureFormat));

			// Create Direct3D 9 texture, let Direct3D create the mipmaps for us if requested by the user
			if (direct3D9Renderer.getDirect3DDevice9()->CreateTexture(width, height, 0, direct3D9Usage, d3dFormat, D3DPOOL_DEFAULT, &mDirect3DTexture9, nullptr) == D3D_OK && nullptr != data)
			{
				// Upload the texture data

				// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
				if (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS)
				{
					// Calculate the number of mipmaps
					const uint32_t numberOfMipmaps = getNumberOfMipmaps(width, height);

					// Upload all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
						// Upload the current mipmap

						// Get the surface
						IDirect3DSurface9* direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(mipmap, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Upload the texture data
							const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
							FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

							// Release the surface
							direct3DSurface9->Release();
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						data = static_cast<const uint8_t*>(data) + Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						width = getHalfSize(width);
						height = getHalfSize(height);
					}
				}
				else
				{
					// The user only provided us with the base texture, no mipmaps

					// Get the surface
					IDirect3DSurface9* direct3DSurface9 = nullptr;
					FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(0, &direct3DSurface9));
					if (nullptr != direct3DSurface9)
					{
						// Upload the texture data
						const RECT sourceRect[] = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
						FAILED_DEBUG_BREAK(D3DXLoadSurfaceFromMemory(direct3DSurface9, nullptr, nullptr, data, d3dFormat, Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width), nullptr, sourceRect, D3DX_FILTER_NONE, 0));

						// Release the surface
						direct3DSurface9->Release();
					}
				}
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Cube texture");
			#endif

			// End debug event
			RENDERER_END_DEBUG_EVENT(&direct3D9Renderer)
			*/
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TextureCube() override
		{
			if (nullptr != mDirect3DTexture9)
			{
				mDirect3DTexture9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D texture instance
		*
		*  @return
		*    The Direct3D texture instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DTexture9* getDirect3DTexture9() const
		{
			return mDirect3DTexture9;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Valid Direct3D 9 texture?
				if (nullptr != mDirect3DTexture9)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DTexture9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(strlen(name)), 0));

					// Set debug name of the texture surfaces
					const DWORD levelCount = mDirect3DTexture9->GetLevelCount();
					for (DWORD level = 0; level < levelCount; ++level)
					{
						// Get the Direct3D 9 surface
						const size_t nameLength = strlen(name);
						IDirect3DSurface9 *direct3DSurface9 = nullptr;
						FAILED_DEBUG_BREAK(mDirect3DTexture9->GetSurfaceLevel(level, &direct3DSurface9));
						if (nullptr != direct3DSurface9)
						{
							// Set the debug name
							// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
							FAILED_DEBUG_BREAK(direct3DSurface9->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(nameLength), 0));

							// Release the Direct3D 9 surface
							direct3DSurface9->Release();
						}
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
		IDirect3DTexture9* mDirect3DTexture9;	///< Direct3D 9 texture instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Texture/TextureManager.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 texture manager interface
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*/
		inline explicit TextureManager(Direct3D9Renderer& direct3D9Renderer) :
			ITextureManager(direct3D9Renderer)
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
			RENDERER_ASSERT(getRenderer().getContext(), width > 0, "Direct3D 9 create texture 1D was called with invalid parameters")

			// Create 1D texture resource
			return RENDERER_NEW(getRenderer().getContext(), Texture1D)(static_cast<Direct3D9Renderer&>(getRenderer()), width, textureFormat, data, textureFlags, textureUsage);
		}

		[[nodiscard]] virtual Renderer::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT, [[maybe_unused]] uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0, "Direct3D 9 create texture 2D was called with invalid parameters")

			// Create 2D texture resource
			return RENDERER_NEW(getRenderer().getContext(), Texture2D)(static_cast<Direct3D9Renderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, textureUsage);
		}

		[[nodiscard]] virtual Renderer::ITexture2DArray* createTexture2DArray([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height, [[maybe_unused]] uint32_t numberOfSlices, [[maybe_unused]] Renderer::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no 2D texture arrays")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0 && depth > 0, "Direct3D 9 create texture 3D was called with invalid parameters")

			// Create 3D texture resource
			return RENDERER_NEW(getRenderer().getContext(), Texture3D)(static_cast<Direct3D9Renderer&>(getRenderer()), width, height, depth, textureFormat, data, textureFlags, textureUsage);
		}

		[[nodiscard]] virtual Renderer::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Sanity check
			RENDERER_ASSERT(getRenderer().getContext(), width > 0 && height > 0, "Direct3D 9 create texture cube was called with invalid parameters")

			// Create cube texture resource
			return RENDERER_NEW(getRenderer().getContext(), TextureCube)(static_cast<Direct3D9Renderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, textureUsage);
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
	//[ Direct3D9Renderer/State/SamplerState.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 sampler state class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(Direct3D9Renderer& direct3D9Renderer, const Renderer::SamplerState& samplerState) :
			ISamplerState(direct3D9Renderer),
			mDirect3D9MagFilterMode(Mapping::getDirect3D9MagFilterMode(direct3D9Renderer.getContext(), samplerState.filter)),
			mDirect3D9MinFilterMode(Mapping::getDirect3D9MinFilterMode(direct3D9Renderer.getContext(), samplerState.filter)),
			mDirect3D9MipFilterMode((0.0f == samplerState.maxLOD) ? D3DTEXF_NONE : Mapping::getDirect3D9MipFilterMode(direct3D9Renderer.getContext(), samplerState.filter)),	// In case "Renderer::SamplerState::maxLOD" is zero, disable mipmapping in order to ensure a correct behaviour when using Direct3D 9, float equal check is valid in here
			mDirect3D9AddressModeU(Mapping::getDirect3D9TextureAddressMode(samplerState.addressU)),
			mDirect3D9AddressModeV(Mapping::getDirect3D9TextureAddressMode(samplerState.addressV)),
			mDirect3D9AddressModeW(Mapping::getDirect3D9TextureAddressMode(samplerState.addressW)),
			mDirect3D9MipLODBias(*(reinterpret_cast<const DWORD*>(&samplerState.mipLODBias))),	// Direct3D 9 type is float, but has to be handed over by using DWORD
			mDirect3D9MaxAnisotropy(samplerState.maxAnisotropy),
			mDirect3DBorderColor(0),	// Set below
			mDirect3DMaxMipLevel((samplerState.minLOD > 0.0f) ? static_cast<unsigned long>(samplerState.minLOD) : 0)	// Direct3D 9 type is unsigned long, lookout the Direct3D 9 name is twisted and implies "Renderer::SamplerState::maxLOD" but it's really "Renderer::SamplerState::minLOD"
		{
			// Sanity check
			RENDERER_ASSERT(direct3D9Renderer.getContext(), samplerState.maxAnisotropy <= direct3D9Renderer.getCapabilities().maximumAnisotropy, "Maximum Direct3D 9 anisotropy value violated")

			{ // Renderer::SamplerState::borderColor[4]
				// For Direct3D 9, the clear color must be between [0..1]
				float normalizedColor[4] = { samplerState.borderColor[0], samplerState.borderColor[1], samplerState.borderColor[2], samplerState.borderColor[3] } ;
				for (int i = 0; i < 4; ++i)
				{
					if (normalizedColor[i] < 0.0f)
					{
						normalizedColor[i] = 0.0f;
					}
					if (normalizedColor[i] > 1.0f)
					{
						normalizedColor[i] = 1.0f;
					}
				}
				#ifdef RENDERER_DEBUG
					if (normalizedColor[0] != samplerState.borderColor[0] || normalizedColor[1] != samplerState.borderColor[1] || normalizedColor[2] != samplerState.borderColor[2] || normalizedColor[3] != samplerState.borderColor[3])
					{
						RENDERER_LOG(direct3D9Renderer.getContext(), CRITICAL, "The given border color was clamped to [0, 1] because Direct3D 9 does not support values outside this range")
					}
				#endif
				mDirect3DBorderColor = D3DCOLOR_COLORVALUE(normalizedColor[0], normalizedColor[1], normalizedColor[2], normalizedColor[3]);
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerState() override
		{}

		/**
		*  @brief
		*    Set the Direct3D 9 sampler states
		*
		*  @param[in] sampler
		*    Sampler stage index
		*  @param[in] direct3DDevice9
		*    Direct3D 9 device instance to use
		*/
		void setDirect3D9SamplerStates(uint32_t sampler, IDirect3DDevice9& direct3DDevice9) const
		{
			// "IDirect3DDevice9::SetSamplerState()"-documentation: "D3DSAMPLERSTATETYPE Enumerated Type" at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/bb172602%28v=vs.85%29.aspx

			// Renderer::SamplerState::filter
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_MAGFILTER, mDirect3D9MagFilterMode));
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_MINFILTER, mDirect3D9MinFilterMode));
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_MIPFILTER, mDirect3D9MipFilterMode));

			// Renderer::SamplerState::addressU
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_ADDRESSU, mDirect3D9AddressModeU));

			// Renderer::SamplerState::addressV
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_ADDRESSV, mDirect3D9AddressModeV));

			// Renderer::SamplerState::addressW
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_ADDRESSW, mDirect3D9AddressModeW));

			// Renderer::SamplerState::mipLODBias
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS, mDirect3D9MipLODBias));

			// Renderer::SamplerState::maxAnisotropy
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_MAXANISOTROPY, mDirect3D9MaxAnisotropy));

			// Renderer::SamplerState::comparisonFunc
			// -> Not available in Direct3D 9

			// Renderer::SamplerState::borderColor[4]
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_BORDERCOLOR, mDirect3DBorderColor));

			// Renderer::SamplerState::minLOD
			FAILED_DEBUG_BREAK(direct3DDevice9.SetSamplerState(sampler, D3DSAMP_MAXMIPLEVEL, mDirect3DMaxMipLevel));

			// Renderer::SamplerState::maxLOD
			// -> Not available in Direct3D 9
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			inline virtual void setDebugName(const char*) override
			{
				// There's no Direct3D 9 resource we could assign a debug name to
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
		DWORD mDirect3D9MagFilterMode;	///< Direct3D 9 magnification filter mode
		DWORD mDirect3D9MinFilterMode;	///< Direct3D 9 minification filter mode
		DWORD mDirect3D9MipFilterMode;	///< Direct3D 9 mipmapping filter mode
		DWORD mDirect3D9AddressModeU;	///< Direct3D 9 u address mode
		DWORD mDirect3D9AddressModeV;	///< Direct3D 9 v address mode
		DWORD mDirect3D9AddressModeW;	///< Direct3D 9 w address mode
		DWORD mDirect3D9MipLODBias;		///< Direct3D 9 mipmap LOD bias
		DWORD mDirect3D9MaxAnisotropy;	///< Direct3D 9 maximum anisotropy
		DWORD mDirect3DBorderColor;		///< Direct3D 9 border color
		DWORD mDirect3DMaxMipLevel;		///< Direct3D 9 maximum mipmap level


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/State/IState.h                      ]
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
	//[ Direct3D9Renderer/State/RasterizerState.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 rasterizer state class
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
		explicit RasterizerState(const Renderer::RasterizerState& rasterizerState) :
			mDirect3DFillMode(0),	// Set below
			mDirect3DCullMode(0),	// Set below
			mDirect3DDepthBias(*(reinterpret_cast<const DWORD*>(&rasterizerState.depthBias))),	// Direct3D 9 type is float, but has to be handed over by using DWORD
			mDirect3DSlopeScaledDepthBias(*(reinterpret_cast<const DWORD*>(&rasterizerState.slopeScaledDepthBias))),	// Direct3D 9 type is float, but has to be handed over by using DWORD
			mDirect3DScissorEnable(static_cast<DWORD>(rasterizerState.scissorEnable)),
			mDirect3DMultisampleEnable(static_cast<DWORD>(rasterizerState.multisampleEnable)),
			mDirect3DAntialiasedLineEnable(static_cast<DWORD>(rasterizerState.antialiasedLineEnable))
		{
			// Renderer::RasterizerState::fillMode
			switch (rasterizerState.fillMode)
			{
				// Wireframe
				case Renderer::FillMode::WIREFRAME:
					mDirect3DFillMode = D3DFILL_WIREFRAME;
					break;

				// Solid
				default:
				case Renderer::FillMode::SOLID:
					mDirect3DFillMode = D3DFILL_SOLID;
					break;
			}

			// Renderer::RasterizerState::cullMode
			// Renderer::RasterizerState::frontCounterClockwise
			switch (rasterizerState.cullMode)
			{
				// No culling
				default:
				case Renderer::CullMode::NONE:
					mDirect3DCullMode = D3DCULL_NONE;
					break;

				// Selects clockwise polygons as front-facing
				case Renderer::CullMode::FRONT:
					mDirect3DCullMode = rasterizerState.frontCounterClockwise ? D3DCULL_CCW : D3DCULL_CW;
					break;

				// Selects counterclockwise polygons as front-facing
				case Renderer::CullMode::BACK:
					mDirect3DCullMode = rasterizerState.frontCounterClockwise ? D3DCULL_CW : D3DCULL_CCW;
					break;
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~RasterizerState()
		{}

		/**
		*  @brief
		*    Set the Direct3D 9 rasterizer states
		*
		*  @param[in] direct3DDevice9
		*    Direct3D 9 device instance to use
		*/
		void setDirect3D9RasterizerStates(IDirect3DDevice9& direct3DDevice9) const
		{
			// Renderer::RasterizerState::fillMode
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_FILLMODE, mDirect3DFillMode));

			// Renderer::RasterizerState::cullMode
			// Renderer::RasterizerState::frontCounterClockwise
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_CULLMODE, mDirect3DCullMode));

			// RasterizerState::depthBias
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_DEPTHBIAS, mDirect3DDepthBias));

			// RasterizerState::depthBiasClamp
			// -> Not available in Direct3D 9

			// RasterizerState::slopeScaledDepthBias
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, mDirect3DSlopeScaledDepthBias));

			// RasterizerState::depthClipEnable
			// TODO(co) Supported in Direct3D 9? I assume it's not...

			// RasterizerState::scissorEnable
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_SCISSORTESTENABLE, mDirect3DScissorEnable));

			// RasterizerState::multisampleEnable
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, mDirect3DMultisampleEnable));

			// RasterizerState::antialiasedLineEnable
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, mDirect3DAntialiasedLineEnable));
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		DWORD mDirect3DFillMode;				///< Direct3D 9 fill mode
		DWORD mDirect3DCullMode;				///< Direct3D 9 cull mode
		DWORD mDirect3DDepthBias;				///< Direct3D 9 depth bias
		DWORD mDirect3DSlopeScaledDepthBias;	///< Direct3D 9 slope scaled depth bias
		DWORD mDirect3DScissorEnable;			///< Direct3D 9 scissor enable
		DWORD mDirect3DMultisampleEnable;		///< Direct3D 9 multisample enable
		DWORD mDirect3DAntialiasedLineEnable;	///< Direct3D 9 antialiased line enable


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/State/DepthStencilState.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 depth stencil state class
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
			mDepthStencilState(depthStencilState)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~DepthStencilState()
		{}

		/**
		*  @brief
		*    Set the Direct3D 9 depth stencil states
		*
		*  @param[in] direct3DDevice9
		*    Direct3D 9 device instance to use
		*/
		void setDirect3D9DepthStencilStates(IDirect3DDevice9& direct3DDevice9) const
		{
			// Renderer::DepthStencilState::depthEnable
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_ZENABLE, static_cast<DWORD>(mDepthStencilState.depthEnable)));

			// Renderer::DepthStencilState::depthWriteMask
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_ZWRITEENABLE, static_cast<DWORD>((Renderer::DepthWriteMask::ALL == mDepthStencilState.depthWriteMask) ? TRUE : FALSE)));

			// Renderer::DepthStencilState::depthFunc
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_ZFUNC, Mapping::getDirect3D9ComparisonFunc(mDepthStencilState.depthFunc)));

			// TODO(co) Map the rest of the depth stencil states, store mapped values instead of mapping over and over again during runtime
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::DepthStencilState mDepthStencilState;	///< Depth stencil state


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/State/BlendState.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 blend state class
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
			mBlendState(blendState)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~BlendState()
		{}

		/**
		*  @brief
		*    Set the Direct3D 9 blend states
		*
		*  @param[in] direct3DDevice9
		*    Direct3D 9 device instance to use
		*/
		void setDirect3D9BlendStates(IDirect3DDevice9& direct3DDevice9) const
		{
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_ALPHABLENDENABLE, static_cast<DWORD>(mBlendState.renderTarget[0].blendEnable)));

			// TODO(co) Add more blend state options: Due to time limitations for now only fixed build in alpha blend setup in order to see a change
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA));
			FAILED_DEBUG_BREAK(direct3DDevice9.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));

			// TODO(co) Map the rest of the blend states
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::BlendState mBlendState;	///< Blend state


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/RenderTarget/RenderPass.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 render pass interface
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
			RENDERER_ASSERT(renderer.getContext(), mNumberOfColorAttachments < 8, "Invalid number of Direct3D 9 color attachments")
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
			RENDERER_ASSERT(getRenderer().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid Direct3D 9 color attachment index")
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
	//[ Direct3D9Renderer/RenderTarget/SwapChain.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 swap chain class
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
			mDirect3DSwapChain9(nullptr),
			mDirect3DSurface9RenderTarget(nullptr),
			mDirect3DSurface9DepthStencil(nullptr),
			mVerticalSynchronizationInterval(0)
		{
			const RenderPass& d3d9RenderPass = static_cast<RenderPass&>(renderPass);

			// Sanity check
			RENDERER_ASSERT(renderPass.getRenderer().getContext(), 1 == d3d9RenderPass.getNumberOfColorAttachments(), "There must be exactly one Direct3D 9 render pass color attachment")

			// Get the Direct3D 9 device instance
			IDirect3DDevice9* direct3DDevice9 = static_cast<Direct3D9Renderer&>(renderPass.getRenderer()).getDirect3DDevice9();

			// Get the native window handle
			const HWND hWnd = reinterpret_cast<HWND>(windowHandle.nativeWindowHandle);

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

			// Set up the structure used to create the Direct3D 9 swap chain instance
			// -> It appears that receiving and manually accessing the automatic depth stencil surface instance is not possible, so, we don't use the automatic depth stencil thing
			D3DPRESENT_PARAMETERS d3dPresentParameters = {};
			d3dPresentParameters.BackBufferWidth		= static_cast<UINT>(width);
			d3dPresentParameters.BackBufferHeight		= static_cast<UINT>(height);
			d3dPresentParameters.BackBufferFormat		= static_cast<D3DFORMAT>(Mapping::getDirect3D9Format(d3d9RenderPass.getColorAttachmentTextureFormat(0)));
			d3dPresentParameters.BackBufferCount		= 1;
			d3dPresentParameters.SwapEffect				= D3DSWAPEFFECT_DISCARD;
			d3dPresentParameters.hDeviceWindow			= hWnd;
			d3dPresentParameters.Windowed				= TRUE;
			d3dPresentParameters.EnableAutoDepthStencil = FALSE;
			d3dPresentParameters.PresentationInterval	= Mapping::getDirect3D9PresentationInterval(renderPass.getRenderer().getContext(), mVerticalSynchronizationInterval);

			// Create the Direct3D 9 swap chain
			// -> Direct3D 9 now also automatically fills the given present parameters instance with the chosen settings
			FAILED_DEBUG_BREAK(direct3DDevice9->CreateAdditionalSwapChain(&d3dPresentParameters, &mDirect3DSwapChain9));

			// Get the Direct3D 9 render target surface instance
			FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mDirect3DSurface9RenderTarget));

			// Create the Direct3D 9 depth stencil surface
			const Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat = static_cast<RenderPass&>(getRenderPass()).getDepthStencilAttachmentTextureFormat();
			if (Renderer::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat)
			{
				FAILED_DEBUG_BREAK(direct3DDevice9->CreateDepthStencilSurface(d3dPresentParameters.BackBufferWidth, d3dPresentParameters.BackBufferHeight,
					D3DFMT_D24S8, d3dPresentParameters.MultiSampleType, d3dPresentParameters.MultiSampleQuality, FALSE,
					&mDirect3DSurface9DepthStencil, nullptr));
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
			// Release the used resources
			if (nullptr != mDirect3DSurface9DepthStencil)
			{
				mDirect3DSurface9DepthStencil->Release();
			}
			if (nullptr != mDirect3DSurface9RenderTarget)
			{
				mDirect3DSurface9RenderTarget->Release();
			}
			if (nullptr != mDirect3DSwapChain9)
			{
				mDirect3DSwapChain9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 9 swap chain instance
		*
		*  @return
		*    The Direct3D 9 swap chain instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DSwapChain9* getDirect3DSwapChain9() const
		{
			return mDirect3DSwapChain9;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 render target surface instance
		*
		*  @return
		*    The Direct3D 9 render target surface instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline IDirect3DSurface9* getDirect3DSurface9RenderTarget() const
		{
			return mDirect3DSurface9RenderTarget;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 depth stencil surface instance
		*
		*  @return
		*    The Direct3D 9 depth stencil surface instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline IDirect3DSurface9* getDirect3DSurface9DepthStencil() const
		{
			return mDirect3DSurface9DepthStencil;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// "IDirect3DSwapChain9" is not derived from "IDirect3DResource9", meaning we can't use the "IDirect3DResource9::SetPrivateData()"-method

				// Assign a debug name to the Direct3D 9 render target surface
				if (nullptr != mDirect3DSurface9RenderTarget)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDirect3DSurface9RenderTarget->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DSurface9RenderTarget->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(strlen(name)), 0));
				}

				// Assign a debug name to the Direct3D 9 depth stencil surface
				if (nullptr != mDirect3DSurface9DepthStencil)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mDirect3DSurface9DepthStencil->SetPrivateData(WKPDID_D3DDebugObjectName, nullptr, 0, 0));
					FAILED_DEBUG_BREAK(mDirect3DSurface9DepthStencil->SetPrivateData(WKPDID_D3DDebugObjectName, name, static_cast<UINT>(strlen(name)), 0));
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
			if (nullptr != mDirect3DSwapChain9)
			{
				// Get the Direct3D 9 present parameters
				D3DPRESENT_PARAMETERS d3dPresentParameters;
				FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetPresentParameters(&d3dPresentParameters));

				// Get the width and height
				long swapChainWidth  = 1;
				long swapChainHeight = 1;
				{
					// Get the client rectangle of the native output window
					// -> Don't use the width and height stored in "DXGI_SWAP_CHAIN_DESC" -> "DXGI_MODE_DESC"
					//    because it might have been modified in order to avoid zero values
					RECT rect;
					::GetClientRect(d3dPresentParameters.hDeviceWindow, &rect);

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
			if (nullptr != mDirect3DSwapChain9)
			{
				// Get the Direct3D 9 present parameters
				D3DPRESENT_PARAMETERS d3dPresentParameters;
				FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetPresentParameters(&d3dPresentParameters));

				// Return the native window handle
				return reinterpret_cast<Renderer::handle>(d3dPresentParameters.hDeviceWindow);
			}

			// Error!
			return NULL_HANDLE;
		}

		virtual void setVerticalSynchronizationInterval(uint32_t synchronizationInterval) override
		{
			// TODO(co) Direct3D 9 supports a maximum synchronization interval of four. Need to add some security checks.
			if (mVerticalSynchronizationInterval != synchronizationInterval)
			{
				mVerticalSynchronizationInterval = synchronizationInterval;
				resizeBuffers();
			}
		}

		virtual void present() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDirect3DSwapChain9)
			{
				FAILED_DEBUG_BREAK(mDirect3DSwapChain9->Present(nullptr, nullptr, nullptr, nullptr, 0));
			}
		}

		virtual void resizeBuffers() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDirect3DSwapChain9)
			{
				// Get the Direct3D 9 device instance
				IDirect3DDevice9* direct3DDevice9 = nullptr;
				FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetDevice(&direct3DDevice9));

				// Get the Direct3D 9 present parameters to query the native window handle
				D3DPRESENT_PARAMETERS d3dPresentParameters;
				FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetPresentParameters(&d3dPresentParameters));
				const HWND nativeWindowHandle = d3dPresentParameters.hDeviceWindow;

				// Get the swap chain width and height, ensures they are never ever zero
				UINT width  = 1;
				UINT height = 1;
				getSafeWidthAndHeight(width, height);

				// Get the currently set render target
				Renderer::IRenderTarget* renderTargetBackup = static_cast<Direct3D9Renderer&>(getRenderer()).omGetRenderTarget();

				// In case this swap chain is the current render target, we have to unset it before continuing
				if (this == renderTargetBackup)
				{
					static_cast<Direct3D9Renderer&>(getRenderer()).setGraphicsRenderTarget(nullptr);
				}
				else
				{
					renderTargetBackup = nullptr;
				}

				// Release the surfaces
				if (nullptr != mDirect3DSurface9DepthStencil)
				{
					mDirect3DSurface9DepthStencil->Release();
					mDirect3DSurface9DepthStencil = nullptr;
				}
				if (nullptr != mDirect3DSurface9RenderTarget)
				{
					mDirect3DSurface9RenderTarget->Release();
					mDirect3DSurface9RenderTarget = nullptr;
				}
				if (nullptr != mDirect3DSwapChain9)
				{
					mDirect3DSwapChain9->Release();
					mDirect3DSwapChain9 = nullptr;
				}

				// Set up the structure used to create the Direct3D 9 swap chain instance
				// -> It appears that receiving and manually accessing the automatic depth stencil surface instance is not possible, so, we don't use the automatic depth stencil thing
				::ZeroMemory(&d3dPresentParameters, sizeof(D3DPRESENT_PARAMETERS));
				d3dPresentParameters.BackBufferWidth		= width;
				d3dPresentParameters.BackBufferHeight		= height;
				d3dPresentParameters.BackBufferCount		= 1;
				d3dPresentParameters.SwapEffect				= D3DSWAPEFFECT_DISCARD;
				d3dPresentParameters.hDeviceWindow			= nativeWindowHandle;
				d3dPresentParameters.Windowed				= TRUE;
				d3dPresentParameters.EnableAutoDepthStencil = TRUE;
				d3dPresentParameters.AutoDepthStencilFormat = D3DFMT_D24X8;
				d3dPresentParameters.PresentationInterval	= Mapping::getDirect3D9PresentationInterval(getRenderer().getContext(), mVerticalSynchronizationInterval);

				// Create the Direct3D 9 swap chain
				// -> Direct3D 9 now also automatically fills the given present parameters instance with the chosen settings
				FAILED_DEBUG_BREAK(direct3DDevice9->CreateAdditionalSwapChain(&d3dPresentParameters, &mDirect3DSwapChain9));

				// Get the Direct3D 9 render target surface instance
				FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mDirect3DSurface9RenderTarget));

				// Create the Direct3D 9 depth stencil surface
				FAILED_DEBUG_BREAK(direct3DDevice9->CreateDepthStencilSurface(d3dPresentParameters.BackBufferWidth, d3dPresentParameters.BackBufferHeight,
					D3DFMT_D24S8, d3dPresentParameters.MultiSampleType, d3dPresentParameters.MultiSampleQuality, FALSE,
					&mDirect3DSurface9DepthStencil, nullptr));
			}
		}

		[[nodiscard]] inline virtual bool getFullscreenState() const override
		{
			// TODO(co) Implement me
			return false;
		}

		inline virtual void setFullscreenState([[maybe_unused]] bool fullscreen) override
		{
			// TODO(co) Implement me
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
		*    For instance "IDirect3DDevice9::CreateAdditionalSwapChain()" can automatically choose the width and height to match the client
		*    rectangle of the native window, but as soon as the width or height is zero we will get the error message
		*      "Direct3D9: (ERROR) :Failed to create driver surface"
		*      "Direct3D9: (ERROR) :Failure initializing swap chain. CreateAdditionalSwapChain fails"
		*      "D3D9 Helper: IDirect3DDevice9::CreateAdditionalSwapChain failed: E_NOTIMPL"
		*   So, best to use this method which gets the width and height of the native output
		*   window manually and ensures it's never zero.
		*
		*  @note
		*    - "mDXGISwapChain" must be valid when calling this method
		*/
		void getSafeWidthAndHeight(uint32_t& width, uint32_t& height) const
		{
			// Get the Direct3D 9 present parameters
			D3DPRESENT_PARAMETERS d3dPresentParameters;
			FAILED_DEBUG_BREAK(mDirect3DSwapChain9->GetPresentParameters(&d3dPresentParameters));

			// Get the client rectangle of the native output window
			RECT rect;
			::GetClientRect(d3dPresentParameters.hDeviceWindow, &rect);

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


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IDirect3DSwapChain9* mDirect3DSwapChain9;			///< The Direct3D 9 swap chain instance, null pointer on error
		IDirect3DSurface9*	 mDirect3DSurface9RenderTarget;	///< The Direct3D 9 render target surface instance, null pointer on error
		IDirect3DSurface9*	 mDirect3DSurface9DepthStencil;	///< The Direct3D 9 depth stencil surface instance, null pointer on error
		uint32_t			 mVerticalSynchronizationInterval;


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/RenderTarget/Framebuffer.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 framebuffer class
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
			mDirect3D9ColorSurfaces(nullptr),
			mDirect3D9DepthStencilSurface(nullptr)
		{
			// The Direct3D 9 documentation says the following about the framebuffer width and height when using multiple render targets
			//   "" (!)
			// So, in here I use the smallest width and height as the size of the framebuffer.

			// Add a reference to the used color textures
			Direct3D9Renderer& direct3D9Renderer = static_cast<Direct3D9Renderer&>(renderPass.getRenderer());
			if (mNumberOfColorTextures > 0)
			{
				const Renderer::Context& context = direct3D9Renderer.getContext();
				mColorTextures = RENDERER_MALLOC_TYPED(context, Renderer::ITexture*, mNumberOfColorTextures);
				mDirect3D9ColorSurfaces = RENDERER_MALLOC_TYPED(context, IDirect3DSurface9*, mNumberOfColorTextures);

				// Loop through all color textures
				IDirect3DSurface9** direct3D9ColorSurface = mDirect3D9ColorSurfaces;
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments, ++direct3D9ColorSurface)
				{
					// Sanity check
					RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid Direct3D 9 color framebuffer attachment texture")

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
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 9 color framebuffer attachment mipmap index")
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid Direct3D 9 color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

							// Get the Direct3D 9 surface
							FAILED_DEBUG_BREAK(texture2D->getDirect3DTexture9()->GetSurfaceLevel(colorFramebufferAttachments->mipmapIndex, direct3D9ColorSurface));
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
							RENDERER_LOG(direct3D9Renderer.getContext(), CRITICAL, "The type of the given color texture at index %u is not supported by the Direct3D 9 renderer backend", colorTexture - mColorTextures)
							*direct3D9ColorSurface = nullptr;
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != mDepthStencilTexture, "Invalid Direct3D 9 depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 9 depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Direct3D 9 depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

						// Get the Direct3D 9 surface
						FAILED_DEBUG_BREAK(texture2D->getDirect3DTexture9()->GetSurfaceLevel(depthStencilFramebufferAttachment->mipmapIndex, &mDirect3D9DepthStencilSurface));
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
						RENDERER_LOG(direct3D9Renderer.getContext(), CRITICAL, "The type of the given depth stencil texture is not supported by the Direct3D 9 renderer backend")
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Direct3D 9 framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Direct3D 9 framebuffer height")
				mHeight = 1;
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Framebuffer() override
		{
			// Release the reference to the used color textures
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mDirect3D9ColorSurfaces)
			{
				// Release references
				IDirect3DSurface9** direct3D9ColorSurfacesEnd = mDirect3D9ColorSurfaces + mNumberOfColorTextures;
				for (IDirect3DSurface9** direct3D9ColorSurface = mDirect3D9ColorSurfaces; direct3D9ColorSurface < direct3D9ColorSurfacesEnd; ++direct3D9ColorSurface)
				{
					(*direct3D9ColorSurface)->Release();
				}

				// Cleanup
				RENDERER_FREE(context, mDirect3D9ColorSurfaces);
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
			if (nullptr != mDirect3D9DepthStencilSurface)
			{
				// Release reference
				mDirect3D9DepthStencilSurface->Release();
			}
			if (nullptr != mDepthStencilTexture)
			{
				// Release reference
				mDepthStencilTexture->releaseReference();
			}
		}

		/**
		*  @brief
		*    Return the number of Direct3D 9 render target surfaces
		*
		*  @return
		*    The number of Direct3D 9 render target surfaces
		*/
		[[nodiscard]] inline uint32_t getNumberOfDirect3DSurface9Colors() const
		{
			return mNumberOfColorTextures;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 render target surfaces
		*
		*  @return
		*    The Direct3D 9 render target surfaces, can be a null pointer, do not release the returned instances unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DSurface9** getDirect3DSurface9Colors() const
		{
			return mDirect3D9ColorSurfaces;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 depth stencil surface
		*
		*  @return
		*    The Direct3D 9 depth stencil surface, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DSurface9* getDirect3DSurface9DepthStencil() const
		{
			return mDirect3D9DepthStencilSurface;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			inline virtual void setDebugName(const char*) override
			{
				// In here we could assign the given debug name to all surfaces assigned to the
				// framebuffer, but this might end up within a naming chaos due to overwriting
				// possible already set names... don't do this...
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
		Renderer::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Renderer::ITexture*  mDepthStencilTexture;		///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t			 mWidth;					///< The framebuffer width
		uint32_t			 mHeight;					///< The framebuffer height
		// Direct3D 9 part
		IDirect3DSurface9** mDirect3D9ColorSurfaces;		///< The Direct3D 9 color render target surfaces (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		IDirect3DSurface9*  mDirect3D9DepthStencilSurface;	///< The Direct3D 9 depth stencil render target surface (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Shader/VertexShaderHlsl.h           ]
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderHlsl(Direct3D9Renderer& direct3D9Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IVertexShader(direct3D9Renderer),
			mDirect3DVertexShader9(nullptr),
			mD3DXConstantTable(nullptr)
		{
			// Create the Direct3D 9 vertex shader
			direct3D9Renderer.getDirect3DDevice9()->CreateVertexShader(reinterpret_cast<const DWORD*>(shaderBytecode.getBytecode()), &mDirect3DVertexShader9);
			FAILED_DEBUG_BREAK(D3DXGetShaderConstantTable(reinterpret_cast<const DWORD*>(shaderBytecode.getBytecode()), &mD3DXConstantTable));
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderHlsl(Direct3D9Renderer& direct3D9Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IVertexShader(direct3D9Renderer),
			mDirect3DVertexShader9(nullptr),
			mD3DXConstantTable(nullptr)
		{
			// Create the Direct3D 9 buffer object for the vertex shader
			ID3DXBuffer* d3dXBuffer = loadShaderFromSourcecode(direct3D9Renderer.getContext(), "vs_3_0", sourceCode, nullptr, optimizationLevel, &mD3DXConstantTable);
			if (nullptr != d3dXBuffer)
			{
				// Create the Direct3D 9 vertex shader
				FAILED_DEBUG_BREAK(direct3D9Renderer.getDirect3DDevice9()->CreateVertexShader(static_cast<DWORD*>(d3dXBuffer->GetBufferPointer()), &mDirect3DVertexShader9));

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dXBuffer->GetBufferSize()), static_cast<uint8_t*>(d3dXBuffer->GetBufferPointer()));
				}

				// Release the Direct3D 9 shader buffer object
				d3dXBuffer->Release();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexShaderHlsl() override
		{
			// Release the Direct3D 9 constant table
			if (nullptr != mD3DXConstantTable)
			{
				mD3DXConstantTable->Release();
			}

			// Release the Direct3D 9 vertex shader
			if (nullptr != mDirect3DVertexShader9)
			{
				mDirect3DVertexShader9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 9 vertex shader
		*
		*  @return
		*    Direct3D 9 vertex shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DVertexShader9* getDirect3DVertexShader9() const
		{
			return mDirect3DVertexShader9;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 constant table
		*
		*  @return
		*    Direct3D 9 constant table shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DXConstantTable* getD3DXConstantTable() const
		{
			return mD3DXConstantTable;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			inline virtual void setDebugName(const char*) override
			{
				// "IDirect3DVertexShader9" and "ID3DXConstantTable" are not derived from "IDirect3DResource9", meaning we can't use the "IDirect3DResource9::SetPrivateData()"-method
			}
		#endif


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
		IDirect3DVertexShader9*	mDirect3DVertexShader9;	///< Direct3D 9 vertex shader, can be a null pointer
		ID3DXConstantTable*		mD3DXConstantTable;		///< Constant table, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Shader/FragmentShaderHlsl.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    HLSL fragment shader ("pixel shader" in Direct3D terminology) class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderHlsl(Direct3D9Renderer& direct3D9Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IFragmentShader(direct3D9Renderer),
			mDirect3DPixelShader9(nullptr),
			mD3DXConstantTable(nullptr)
		{
			// Create the Direct3D 9 pixel shader
			direct3D9Renderer.getDirect3DDevice9()->CreatePixelShader(reinterpret_cast<const DWORD*>(shaderBytecode.getBytecode()), &mDirect3DPixelShader9);
			FAILED_DEBUG_BREAK(D3DXGetShaderConstantTable(reinterpret_cast<const DWORD*>(shaderBytecode.getBytecode()), &mD3DXConstantTable));
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderHlsl(Direct3D9Renderer& direct3D9Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IFragmentShader(direct3D9Renderer),
			mDirect3DPixelShader9(nullptr),
			mD3DXConstantTable(nullptr)
		{
			// Create the Direct3D 9 buffer object for the pixel shader
			ID3DXBuffer* d3dXBuffer = loadShaderFromSourcecode(direct3D9Renderer.getContext(), "ps_3_0", sourceCode, nullptr, optimizationLevel, &mD3DXConstantTable);
			if (nullptr != d3dXBuffer)
			{
				// Create the Direct3D 9 pixel shader
				FAILED_DEBUG_BREAK(direct3D9Renderer.getDirect3DDevice9()->CreatePixelShader(static_cast<DWORD*>(d3dXBuffer->GetBufferPointer()), &mDirect3DPixelShader9));

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dXBuffer->GetBufferSize()), static_cast<uint8_t*>(d3dXBuffer->GetBufferPointer()));
				}

				// Release the Direct3D 9 shader buffer object
				d3dXBuffer->Release();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~FragmentShaderHlsl() override
		{
			// Release the Direct3D 9 constant table
			if (nullptr != mD3DXConstantTable)
			{
				mD3DXConstantTable->Release();
			}

			// Release the Direct3D 9 pixel shader
			if (nullptr != mDirect3DPixelShader9)
			{
				mDirect3DPixelShader9->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 9 pixel shader
		*
		*  @return
		*    Direct3D 9 pixel shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DPixelShader9* getDirect3DPixelShader9() const
		{
			return mDirect3DPixelShader9;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 constant table
		*
		*  @return
		*    Direct3D 9 constant table shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DXConstantTable* getD3DXConstantTable() const
		{
			return mD3DXConstantTable;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			inline virtual void setDebugName(const char*) override
			{
				// "IDirect3DPixelShader9" and "ID3DXConstantTable" are not derived from "IDirect3DResource9", meaning we can't use the "IDirect3DResource9::SetPrivateData()"-method
			}
		#endif


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
		IDirect3DPixelShader9* mDirect3DPixelShader9;	///< Direct3D 9 pixel shader, can be a null pointer
		ID3DXConstantTable*	   mD3DXConstantTable;		///< Constant table, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Shader/GraphicsProgramHlsl.h        ]
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] vertexShaderHlsl
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderHlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramHlsl(Direct3D9Renderer& direct3D9Renderer, VertexShaderHlsl* vertexShaderHlsl, FragmentShaderHlsl* fragmentShaderHlsl) :
			IGraphicsProgram(direct3D9Renderer),
			mDirect3D9Renderer(&direct3D9Renderer),
			mVertexShaderHlsl(vertexShaderHlsl),
			mFragmentShaderHlsl(fragmentShaderHlsl),
			mDirect3DDevice9(nullptr),
			mD3DXConstantTable(nullptr)
		{
			// Add references to the provided shaders
			if (nullptr != mVertexShaderHlsl)
			{
				mVertexShaderHlsl->addReference();

				// Valid Direct3D 9 vertex shader?
				IDirect3DVertexShader9* direct3DVertexShader9 = mVertexShaderHlsl->getDirect3DVertexShader9();
				if (nullptr != direct3DVertexShader9)
				{
					// Get the Direct3D 9 device
					// -> The "IDirect3DVertexShader9::GetDevice()"-method documentation does not mention whether
					//    or not the Direct3D 9 device reference counter is increased automatically
					// -> The "IDirect3DResource9::GetDevice()"-method documentation on the other hand states
					//    that the Direct3D 9 device reference counter is increased automatically
					// -> So, I just have to assume that Direct3D 9 has a consistent interface, hopefully...
					FAILED_DEBUG_BREAK(direct3DVertexShader9->GetDevice(&mDirect3DDevice9));

					// Get the Direct3D 9 constant table and acquire our reference
					mD3DXConstantTable = mVertexShaderHlsl->getD3DXConstantTable();
					if (nullptr != mD3DXConstantTable)
					{
						mD3DXConstantTable->AddRef();
					}
				}
			}
			if (nullptr != mFragmentShaderHlsl)
			{
				mFragmentShaderHlsl->addReference();

				// If required, get the Direct3D 9 device
				// -> See reference counter behaviour documentation above
				if (nullptr == mDirect3DDevice9 && mFragmentShaderHlsl->getDirect3DPixelShader9())
				{
					FAILED_DEBUG_BREAK(mFragmentShaderHlsl->getDirect3DPixelShader9()->GetDevice(&mDirect3DDevice9));
				}

				// If required, get the Direct3D 9 constant table and acquire our reference
				if (nullptr == mD3DXConstantTable)
				{
					mD3DXConstantTable = mFragmentShaderHlsl->getD3DXConstantTable();
					if (nullptr != mD3DXConstantTable)
					{
						mD3DXConstantTable->AddRef();
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsProgramHlsl() override
		{
			// Release the Direct3D 9 constant table
			if (nullptr != mD3DXConstantTable)
			{
				mD3DXConstantTable->Release();
			}

			// Release the shader references
			if (nullptr != mVertexShaderHlsl)
			{
				mVertexShaderHlsl->releaseReference();
			}
			if (nullptr != mFragmentShaderHlsl)
			{
				mFragmentShaderHlsl->releaseReference();
			}

			// Release our Direct3D 9 device reference
			if (nullptr != mDirect3DDevice9)
			{
				mDirect3DDevice9->Release();
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
	//[ Public virtual Renderer::IGraphicsProgram methods     ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Renderer::handle getUniformHandle(const char* uniformName) override
		{
			// Get the uniform handle
			if (nullptr != mVertexShaderHlsl && nullptr != mVertexShaderHlsl->getD3DXConstantTable())
			{
				const D3DXHANDLE d3dXHandle = mVertexShaderHlsl->getD3DXConstantTable()->GetConstantByName(nullptr, uniformName);
				if (nullptr != d3dXHandle)
				{
					// Done
					return reinterpret_cast<Renderer::handle>(d3dXHandle);
				}
			}
			if (nullptr != mFragmentShaderHlsl && nullptr != mFragmentShaderHlsl->getD3DXConstantTable())
			{
				const D3DXHANDLE d3dXHandle = mFragmentShaderHlsl->getD3DXConstantTable()->GetConstantByName(nullptr, uniformName);
				if (nullptr != d3dXHandle)
				{
					// Done
					return reinterpret_cast<Renderer::handle>(d3dXHandle);
				}
			}

			// Error!
			return NULL_HANDLE;
		}

		inline virtual void setUniform1i([[maybe_unused]] Renderer::handle uniformHandle, [[maybe_unused]] int value) override
		{
			// TODO(co) Implement me
		}

		inline virtual void setUniform1f(Renderer::handle uniformHandle, float value) override
		{
			if (nullptr != mDirect3DDevice9)
			{
				FAILED_DEBUG_BREAK(mD3DXConstantTable->SetFloat(mDirect3DDevice9, reinterpret_cast<D3DXHANDLE>(uniformHandle), value));
			}
		}

		inline virtual void setUniform2fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (nullptr != mDirect3DDevice9)
			{
				FAILED_DEBUG_BREAK(mD3DXConstantTable->SetFloatArray(mDirect3DDevice9, reinterpret_cast<D3DXHANDLE>(uniformHandle), value, 2));
			}
		}

		inline virtual void setUniform3fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (nullptr != mDirect3DDevice9)
			{
				FAILED_DEBUG_BREAK(mD3DXConstantTable->SetFloatArray(mDirect3DDevice9, reinterpret_cast<D3DXHANDLE>(uniformHandle), value, 3));
			}
		}

		inline virtual void setUniform4fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (nullptr != mDirect3DDevice9)
			{
				FAILED_DEBUG_BREAK(mD3DXConstantTable->SetFloatArray(mDirect3DDevice9, reinterpret_cast<D3DXHANDLE>(uniformHandle), value, 4));
			}
		}

		inline virtual void setUniformMatrix3fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (nullptr != mDirect3DDevice9)
			{
				FAILED_DEBUG_BREAK(mD3DXConstantTable->SetFloatArray(mDirect3DDevice9, reinterpret_cast<D3DXHANDLE>(uniformHandle), value, 3 * 3));
			}
		}

		inline virtual void setUniformMatrix4fv(Renderer::handle uniformHandle, const float* value) override
		{
			if (nullptr != mDirect3DDevice9)
			{
				FAILED_DEBUG_BREAK(mD3DXConstantTable->SetFloatArray(mDirect3DDevice9, reinterpret_cast<D3DXHANDLE>(uniformHandle), value, 4 * 4));
			}
		}


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
		Direct3D9Renderer*  mDirect3D9Renderer;		///< Owner Direct3D 9 renderer instance, always valid
		VertexShaderHlsl*   mVertexShaderHlsl;		///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		FragmentShaderHlsl* mFragmentShaderHlsl;	///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer
		IDirect3DDevice9*   mDirect3DDevice9;		///< The Direct3D 9 device instance (we keep a reference to it), can be a null pointer
		ID3DXConstantTable* mD3DXConstantTable;		/**< The Direct3D 9 constant table instance (we keep a reference to it), null pointer on horrible error (so we don't check).
														 I noticed that as soon as working with "D3DXHANDLE", we no longer need to make a difference between vertex/pixel shaders.
														 I was unable to find this behaviour within the documentation, but it simplifies the implementation so I exploit it in here. */


	};




	//[-------------------------------------------------------]
	//[ Direct3D9Renderer/Shader/ShaderLanguageHlsl.h         ]
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*/
		inline explicit ShaderLanguageHlsl(Direct3D9Renderer& direct3D9Renderer) :
			IShaderLanguage(direct3D9Renderer)
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
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderHlsl)(static_cast<Direct3D9Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::vertexShader", we know there's vertex shader support
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderHlsl)(static_cast<Direct3D9Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode([[maybe_unused]] const Renderer::ShaderBytecode& shaderBytecode) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no tessellation control shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode([[maybe_unused]] const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no tessellation control shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode([[maybe_unused]] const Renderer::ShaderBytecode& shaderBytecode) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no tessellation evaluation shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode([[maybe_unused]] const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no tessellation evaluation shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode([[maybe_unused]] const Renderer::ShaderBytecode& shaderBytecode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no geometry shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode([[maybe_unused]] const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no geometry shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderHlsl)(static_cast<Direct3D9Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderHlsl)(static_cast<Direct3D9Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromBytecode(const Renderer::ShaderBytecode&) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no compute shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromSourceCode(const Renderer::ShaderSourceCode&, Renderer::ShaderBytecode* = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 9 has no compute shader support")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Renderer::IRootSignature& rootSignature, [[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, [[maybe_unused]] Renderer::IVertexShader* vertexShader, [[maybe_unused]] Renderer::ITessellationControlShader* tessellationControlShader, [[maybe_unused]] Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, [[maybe_unused]] Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 9 vertex shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationControlShader, "Direct3D 9 has no tessellation control shader support")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationEvaluationShader, "Direct3D 9 has no tessellation evaluation shader support")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == geometryShader, "Direct3D 9 has no geometry shader support")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 9 fragment shader language mismatch")

			// Create the graphics program
			return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramHlsl)(static_cast<Direct3D9Renderer&>(getRenderer()), static_cast<VertexShaderHlsl*>(vertexShader), static_cast<FragmentShaderHlsl*>(fragmentShader));
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
	//[ Direct3D9Renderer/State/GraphicsPipelineState.h       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 9 graphics pipeline state class
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
		*  @param[in] direct3D9Renderer
		*    Owner Direct3D 9 renderer instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(Direct3D9Renderer& direct3D9Renderer, const Renderer::GraphicsPipelineState& graphicsPipelineState, uint16_t id) :
			IGraphicsPipelineState(direct3D9Renderer, id),
			mDirect3DDevice9(direct3D9Renderer.getDirect3DDevice9()),
			mPrimitiveTopology(graphicsPipelineState.primitiveTopology),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mDirect3DVertexDeclaration9(nullptr),
			mRasterizerState(graphicsPipelineState.rasterizerState),
			mDepthStencilState(graphicsPipelineState.depthStencilState),
			mBlendState(graphicsPipelineState.blendState)
		{
			// Acquire our Direct3D 9 device reference
			mDirect3DDevice9->AddRef();

			// Ensure a correct reference counter behaviour
			graphicsPipelineState.rootSignature->addReference();
			graphicsPipelineState.rootSignature->releaseReference();

			// Add a reference to the referenced renderer resources
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			{ // Create Direct3D 9 vertex elements
				const uint32_t numberOfAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
				const Renderer::VertexAttribute* attributes = graphicsPipelineState.vertexAttributes.attributes;

				// TODO(co) We could manage in here without new/delete when using a fixed maximum supported number of elements
				const Renderer::Context& context = direct3D9Renderer.getContext();
				D3DVERTEXELEMENT9* d3dVertexElements   = RENDERER_MALLOC_TYPED(context, D3DVERTEXELEMENT9, numberOfAttributes + 1);	// +1 for D3DDECL_END()
				D3DVERTEXELEMENT9* d3dVertexElement    = d3dVertexElements;
				D3DVERTEXELEMENT9* d3dVertexElementEnd = d3dVertexElements + numberOfAttributes;
				for (; d3dVertexElement < d3dVertexElementEnd; ++d3dVertexElement, ++attributes)
				{
					// Fill the "D3DVERTEXELEMENT9"-content
					d3dVertexElement->Stream     = static_cast<WORD>(attributes->inputSlot);										// Stream index (WORD)
					d3dVertexElement->Offset     = static_cast<WORD>(attributes->alignedByteOffset);								// Offset in the stream in bytes (WORD)
					d3dVertexElement->Type       = static_cast<BYTE>(Mapping::getDirect3D9Type(attributes->vertexAttributeFormat));	// Data type (BYTE)
					d3dVertexElement->Method     = D3DDECLMETHOD_DEFAULT;															// Processing method (BYTE)
					d3dVertexElement->Usage      = static_cast<BYTE>(Mapping::getDirect3D9Semantic(attributes->semanticName));		// Semantic name (BYTE)
					d3dVertexElement->UsageIndex = static_cast<BYTE>(attributes->semanticIndex);									// Semantic index (BYTE)
				}
				// D3DDECL_END()
				d3dVertexElement->Stream     = 0xFF;				// Stream index (WORD)
				d3dVertexElement->Offset     = 0;					// Offset in the stream in bytes (WORD)
				d3dVertexElement->Type       = D3DDECLTYPE_UNUSED;	// Data type (BYTE)
				d3dVertexElement->Method     = 0;					// Processing method (BYTE)
				d3dVertexElement->Usage      = 0;					// Semantics (BYTE)
				d3dVertexElement->UsageIndex = 0;					// Semantic index (BYTE)

				// Create the Direct3D 9 vertex declaration
				FAILED_DEBUG_BREAK(direct3D9Renderer.getDirect3DDevice9()->CreateVertexDeclaration(d3dVertexElements, &mDirect3DVertexDeclaration9));

				// Destroy Direct3D 9 vertex elements
				RENDERER_FREE(context, d3dVertexElements);
			}
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

			// Release the Direct3D 9 vertex declaration
			if (nullptr != mDirect3DVertexDeclaration9)
			{
				mDirect3DVertexDeclaration9->Release();
			}

			// Release our Direct3D 9 device reference
			mDirect3DDevice9->Release();

			// Free the unique compact graphics pipeline state ID
			static_cast<Direct3D9Renderer&>(getRenderer()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}

		/**
		*  @brief
		*    Return the primitive topology
		*
		*  @return
		*    The primitive topology
		*/
		[[nodiscard]] inline Renderer::PrimitiveTopology getPrimitiveTopology() const
		{
			return mPrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the Direct3D 9 vertex declaration instance
		*
		*  @return
		*    Direct3D 9 vertex declaration instance, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDirect3DVertexDeclaration9* getDirect3DVertexDeclaration9() const
		{
			return mDirect3DVertexDeclaration9;
		}

		/**
		*  @brief
		*    Bind the graphics pipeline state
		*/
		void bindGraphicsPipelineState() const
		{
			// Set the Direct3D 9 vertex declaration
			FAILED_DEBUG_BREAK(mDirect3DDevice9->SetVertexDeclaration(mDirect3DVertexDeclaration9));

			// Set the graphics program
			static_cast<Direct3D9Renderer&>(getRenderer()).setGraphicsProgram(mGraphicsProgram);

			// Set the Direct3D 9 rasterizer state
			mRasterizerState.setDirect3D9RasterizerStates(*mDirect3DDevice9);

			// Set Direct3D 9 depth stencil state
			mDepthStencilState.setDirect3D9DepthStencilStates(*mDirect3DDevice9);

			// Set Direct3D 9 blend state
			mBlendState.setDirect3D9BlendStates(*mDirect3DDevice9);
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			inline virtual void setDebugName(const char*) override
			{
				// "IDirect3DVertexDeclaration9" is not derived from "IDirect3DResource9", meaning we can't use the "IDirect3DResource9::SetPrivateData()"-method
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
		IDirect3DDevice9*			 mDirect3DDevice9;				///< The Direct3D 9 device instance (we keep a reference to it), null pointer on horrible error (so we don't check)
		Renderer::PrimitiveTopology  mPrimitiveTopology;
		Renderer::IGraphicsProgram*	 mGraphicsProgram;
		Renderer::IRenderPass*		 mRenderPass;
		IDirect3DVertexDeclaration9* mDirect3DVertexDeclaration9;	///< Direct3D 9 vertex declaration instance, can be a null pointer
		RasterizerState				 mRasterizerState;
		DepthStencilState			 mDepthStencilState;
		BlendState					 mBlendState;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D9Renderer




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
				RENDERER_ASSERT(renderer.getContext(), nullptr != realData->commandBufferToExecute, "The Direct3D 9 command buffer to execute must be valid")
				renderer.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsRootSignature* realData = static_cast<const Renderer::Command::SetGraphicsRootSignature*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsPipelineState* realData = static_cast<const Renderer::Command::SetGraphicsPipelineState*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsResourceGroup* realData = static_cast<const Renderer::Command::SetGraphicsResourceGroup*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Renderer::IRenderer& renderer)
			{
				// Input-assembler (IA) stage
				const Renderer::Command::SetGraphicsVertexArray* realData = static_cast<const Renderer::Command::SetGraphicsVertexArray*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsViewports* realData = static_cast<const Renderer::Command::SetGraphicsViewports*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Renderer::Viewport*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsScissorRectangles* realData = static_cast<const Renderer::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Renderer::ScissorRectangle*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Renderer::IRenderer& renderer)
			{
				// Output-merger (OM) stage
				const Renderer::Command::SetGraphicsRenderTarget* realData = static_cast<const Renderer::Command::SetGraphicsRenderTarget*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ClearGraphics* realData = static_cast<const Renderer::Command::ClearGraphics*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawGraphics* realData = static_cast<const Renderer::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).drawGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).drawGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawIndexedGraphics* realData = static_cast<const Renderer::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).drawIndexedGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).drawIndexedGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 9 doesn't support compute root signature")
			}

			void SetComputePipelineState(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 9 doesn't support compute pipeline state")
			}

			void SetComputeResourceGroup(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 9 doesn't support compute resource group")
			}

			void DispatchCompute(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 9 doesn't support compute dispatch")
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Renderer::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					static_cast<Direct3D9Renderer::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RENDERER_LOG(static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).getContext(), CRITICAL, "Unsupported Direct3D 9 texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Renderer::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::CopyResource* realData = static_cast<const Renderer::Command::CopyResource*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::GenerateMipmaps* realData = static_cast<const Renderer::Command::GenerateMipmaps*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResetQueryPool* realData = static_cast<const Renderer::Command::ResetQueryPool*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::BeginQuery* realData = static_cast<const Renderer::Command::BeginQuery*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::EndQuery* realData = static_cast<const Renderer::Command::EndQuery*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::WriteTimestampQuery* realData = static_cast<const Renderer::Command::WriteTimestampQuery*>(data);
				static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RENDERER_DEBUG
				void SetDebugMarker(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::SetDebugMarker* realData = static_cast<const Renderer::Command::SetDebugMarker*>(data);
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::BeginDebugEvent* realData = static_cast<const Renderer::Command::BeginDebugEvent*>(data);
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Renderer::IRenderer& renderer)
				{
					static_cast<Direct3D9Renderer::Direct3D9Renderer&>(renderer).endDebugEvent();
				}
			#else
				void SetDebugMarker(const void*, Renderer::IRenderer&)
				{}

				void BeginDebugEvent(const void*, Renderer::IRenderer&)
				{}

				void EndDebugEvent(const void*, Renderer::IRenderer&)
				{}
			#endif


		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Renderer::BackendDispatchFunction DISPATCH_FUNCTIONS[Renderer::CommandDispatchFunctionIndex::NumberOfFunctions] =
		{
			// Command buffer
			&BackendDispatch::ExecuteCommandBuffer,
			// Graphics states
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
namespace Direct3D9Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Direct3D9Renderer::Direct3D9Renderer(const Renderer::Context& context) :
		IRenderer(Renderer::NameId::DIRECT3D9, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		mDirect3D9RuntimeLinking(nullptr),
		mDirect3D9(nullptr),
		mDirect3DDevice9(nullptr),
		mShaderLanguageHlsl(nullptr),
		mDirect3DQuery9Flush(nullptr),
		mGraphicsRootSignature(nullptr),
		mDefaultSamplerState(nullptr),
		// Input-assembler (IA) stage
		mPrimitiveTopology(Renderer::PrimitiveTopology::UNKNOWN),
		// Output-merger (OM) stage
		mRenderTarget(nullptr),
		// State cache to avoid making redundant Direct3D 9 calls
		mDirect3DVertexShader9(nullptr),
		mDirect3DPixelShader9(nullptr)
	{
		// Is Direct3D 9 available?
		mDirect3D9RuntimeLinking = RENDERER_NEW(mContext, Direct3D9RuntimeLinking)(*this);
		if (mDirect3D9RuntimeLinking->isDirect3D9Avaiable())
		{
			// Create the Direct3D instance
			mDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
			if (nullptr != mDirect3D9)
			{
				// Set up the structure used to create the D3DDevice instance
				// -> It appears that receiving and manually accessing the automatic depth stencil surface instance is not possible, so, we don't use the automatic depth stencil thing
				D3DPRESENT_PARAMETERS d3dPresentParameters = {};
				d3dPresentParameters.BackBufferWidth		= 1;
				d3dPresentParameters.BackBufferHeight		= 1;
				d3dPresentParameters.BackBufferCount		= 1;
				d3dPresentParameters.SwapEffect				= D3DSWAPEFFECT_DISCARD;
				d3dPresentParameters.Windowed				= TRUE;
				d3dPresentParameters.EnableAutoDepthStencil = FALSE;

				// Create the Direct3D 9 device instance
				// -> In Direct3D 9, there is always at least one swap chain for each device, known as the implicit swap chain
				// -> The size of the swap chain can be changed by using "IDirect3DDevice9::Reset()", this results in a
				//    loss of all resources and everything has to be rebuild and configured from scratch
				// -> We really don't want to use the implicit swap chain, so we're creating a tiny one (because we have to)
				//    and then using "IDirect3DDevice9::CreateAdditionalSwapChain()" later on for the real main swap chain
				FAILED_DEBUG_BREAK(mDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL_HANDLE, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dPresentParameters, &mDirect3DDevice9));
				if (nullptr != mDirect3DDevice9)
				{
					#ifndef RENDERER_DEBUG
						// Disable debugging
						D3DPERF_SetOptions(1);
					#endif

					// Initialize the capabilities
					initializeCapabilities();

					// Create the default sampler state
					mDefaultSamplerState = createSamplerState(Renderer::ISamplerState::getDefaultSamplerState());

					// Add references to the default sampler state and set it
					if (nullptr != mDefaultSamplerState)
					{
						mDefaultSamplerState->addReference();
						// TODO(co) Set default sampler states
					}
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 9 device instance")
				}
			}
			else
			{
				RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 9 instance")
			}
		}
	}

	Direct3D9Renderer::~Direct3D9Renderer()
	{
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

		// Release the graphics root signature instance
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
			mGraphicsRootSignature = nullptr;
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
					RENDERER_LOG(mContext, CRITICAL, "The Direct3D 9 renderer backend is going to be destroyed, but there are still %u resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "The Direct3D 9 renderer backend is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the Direct3D 9 query instance used for flush, in case we have one
		if (nullptr != mDirect3DQuery9Flush)
		{
			mDirect3DQuery9Flush->Release();
		}

		// Release the HLSL shader language instance, in case we have one
		if (nullptr != mShaderLanguageHlsl)
		{
			mShaderLanguageHlsl->releaseReference();
		}

		// Release the Direct3D 9 device we've created
		if (nullptr != mDirect3DDevice9)
		{
			mDirect3DDevice9->Release();
			mDirect3DDevice9 = nullptr;
		}
		if (nullptr != mDirect3D9)
		{
			mDirect3D9->Release();
			mDirect3D9 = nullptr;
		}

		// Destroy the Direct3D 9 runtime linking instance
		RENDERER_DELETE(mContext, Direct3D9RuntimeLinking, mDirect3D9RuntimeLinking);
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void Direct3D9Renderer::setGraphicsRootSignature(Renderer::IRootSignature* rootSignature)
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
			DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void Direct3D9Renderer::setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Sanity check
			DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsPipelineState)

			// Set graphics pipeline state
			const GraphicsPipelineState* direct3D9GraphicsPipelineState = static_cast<const GraphicsPipelineState*>(graphicsPipelineState);
			mPrimitiveTopology = direct3D9GraphicsPipelineState->getPrimitiveTopology();
			direct3D9GraphicsPipelineState->bindGraphicsPipelineState();
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D9Renderer::setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No Direct3D 9 renderer backend graphics root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Direct3D 9 renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Direct3D 9 renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The Direct3D 9 renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resourceGroup)

			// Set graphics resource group
			const ResourceGroup* d3d9ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = d3d9ResourceGroup->getNumberOfResources();
			Renderer::IResource** resources = d3d9ResourceGroup->getResources();
			const Renderer::RootParameter& rootParameter = mGraphicsRootSignature->getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				// Since Direct3D 9 doesn't support e.g. uniform buffer we need to check for null pointers here
				Renderer::IResource* resource = *resources;
				if (nullptr == resource)
				{
					continue;
				}
				RENDERER_ASSERT(mContext, nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid Direct3D 9 descriptor table")
				const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Renderer::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Renderer::ResourceType::TEXTURE_BUFFER:
						RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no texture buffer support")
						break;

					case Renderer::ResourceType::STRUCTURED_BUFFER:
						RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no structured buffer support")
						break;

					case Renderer::ResourceType::UNIFORM_BUFFER:
						RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no uniform buffer support")
						break;

					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_2D:
					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					{
						const UINT startSlot = descriptorRange.baseShaderRegister;

						// Get Direct3D 9 texture
						IDirect3DBaseTexture9* direct3DBaseTexture9 = nullptr;
						switch (resource->getResourceType())
						{
							case Renderer::ResourceType::TEXTURE_1D:
								direct3DBaseTexture9 = static_cast<Texture1D*>(resource)->getDirect3DTexture9();
								break;

							case Renderer::ResourceType::TEXTURE_2D:
								direct3DBaseTexture9 = static_cast<Texture2D*>(resource)->getDirect3DTexture9();
								break;

							case Renderer::ResourceType::TEXTURE_2D_ARRAY:
								RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no 2D array textures support")
								break;

							case Renderer::ResourceType::TEXTURE_3D:
								direct3DBaseTexture9 = static_cast<Texture3D*>(resource)->getDirect3DTexture9();
								break;

							case Renderer::ResourceType::TEXTURE_CUBE:
								direct3DBaseTexture9 = static_cast<TextureCube*>(resource)->getDirect3DTexture9();
								break;

							case Renderer::ResourceType::TEXTURE_BUFFER:
							case Renderer::ResourceType::STRUCTURED_BUFFER:
							case Renderer::ResourceType::SAMPLER_STATE:
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
							case Renderer::ResourceType::INDIRECT_BUFFER:
							case Renderer::ResourceType::UNIFORM_BUFFER:
							case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
							case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
							case Renderer::ResourceType::VERTEX_SHADER:
							case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
							case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
							case Renderer::ResourceType::GEOMETRY_SHADER:
							case Renderer::ResourceType::FRAGMENT_SHADER:
							case Renderer::ResourceType::COMPUTE_SHADER:
								// Nothing here
								break;
						}

						// Information about vertex texture fetch in Direct3D 9 can be found within:
						// Whitepaper: ftp://download.nvidia.com/developer/Papers/2004/Vertex_Textures/Vertex_Textures.pdf
						//    "Shader Model 3.0
						//     Using Vertex Textures"
						//    (DA-01373-001_v00 1 - 06/24/04)
						// From
						//    Philipp Gerasimov
						//    Randima (Randy) Fernando
						//    Simon Green
						//    NVIDIA Corporation
						// Four texture samplers are supported:
						//     D3DVERTEXTEXTURESAMPLER1
						//     D3DVERTEXTEXTURESAMPLER2
						//     D3DVERTEXTEXTURESAMPLER3
						//     D3DVERTEXTEXTURESAMPLER4
						// -> Update the given zero based texture unit (the constants are linear, so the following is fine)
						const UINT vertexFetchStartSlot = startSlot + D3DVERTEXTEXTURESAMPLER1;

						switch (descriptorRange.shaderVisibility)
						{
							case Renderer::ShaderVisibility::ALL:
							case Renderer::ShaderVisibility::ALL_GRAPHICS:
							{
								// Begin debug event
								RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

								// Set texture
								FAILED_DEBUG_BREAK(mDirect3DDevice9->SetTexture(vertexFetchStartSlot, direct3DBaseTexture9));
								FAILED_DEBUG_BREAK(mDirect3DDevice9->SetTexture(startSlot, direct3DBaseTexture9));

								{ // Set sampler, it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
									RENDERER_ASSERT(mContext, nullptr != d3d9ResourceGroup->getSamplerState(), "Invalid Direct3D 9 sampler state")
									const SamplerState* samplerState = static_cast<const SamplerState*>(d3d9ResourceGroup->getSamplerState()[resourceIndex]);
									if (nullptr != samplerState)
									{
										samplerState->setDirect3D9SamplerStates(vertexFetchStartSlot, *mDirect3DDevice9);
										samplerState->setDirect3D9SamplerStates(startSlot, *mDirect3DDevice9);
									}
								}

								// End debug event
								RENDERER_END_DEBUG_EVENT(this)
								break;
							}

							case Renderer::ShaderVisibility::VERTEX:
								// Begin debug event
								RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

								// Set texture
								FAILED_DEBUG_BREAK(mDirect3DDevice9->SetTexture(vertexFetchStartSlot, direct3DBaseTexture9));

								{ // Set sampler, it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
									RENDERER_ASSERT(mContext, nullptr != d3d9ResourceGroup->getSamplerState(), "Invalid Direct3D 9 sampler state")
									const SamplerState* samplerState = static_cast<const SamplerState*>(d3d9ResourceGroup->getSamplerState()[resourceIndex]);
									if (nullptr != samplerState)
									{
										samplerState->setDirect3D9SamplerStates(vertexFetchStartSlot, *mDirect3DDevice9);
									}
								}

								// End debug event
								RENDERER_END_DEBUG_EVENT(this)
								break;

							case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
								RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no tessellation control shader support (hull shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
								RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no tessellation evaluation shader support (domain shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::GEOMETRY:
								RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no geometry shader support")
								break;

							case Renderer::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology

								// Begin debug event
								RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

								// Set texture
								FAILED_DEBUG_BREAK(mDirect3DDevice9->SetTexture(startSlot, direct3DBaseTexture9));

								{ // Set sampler, it's valid that there's no sampler state (e.g. texel fetch instead of sampling might be used)
									RENDERER_ASSERT(mContext, nullptr != d3d9ResourceGroup->getSamplerState(), "Invalid Direct3D 9 sampler state")
									const SamplerState* samplerState = static_cast<const SamplerState*>(d3d9ResourceGroup->getSamplerState()[resourceIndex]);
									if (nullptr != samplerState)
									{
										samplerState->setDirect3D9SamplerStates(startSlot, *mDirect3DDevice9);
									}
								}

								// End debug event
								RENDERER_END_DEBUG_EVENT(this)
								break;

							case Renderer::ShaderVisibility::COMPUTE:
								RENDERER_LOG(mContext, CRITICAL, "Direct3D 9 has no compute shader support")
								break;
						}
						break;
					}

					case Renderer::ResourceType::SAMPLER_STATE:
						// Unlike Direct3D >=10, Direct3D 9 directly attaches the sampler settings to texture stages
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
					case Renderer::ResourceType::INDIRECT_BUFFER:
					case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
					case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
					case Renderer::ResourceType::VERTEX_SHADER:
					case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
					case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
					case Renderer::ResourceType::GEOMETRY_SHADER:
					case Renderer::ResourceType::FRAGMENT_SHADER:
					case Renderer::ResourceType::COMPUTE_SHADER:
						RENDERER_LOG(mContext, CRITICAL, "Invalid Direct3D 9 renderer backend resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D9Renderer::setGraphicsVertexArray(Renderer::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage
		if (nullptr != vertexArray)
		{
			// Sanity check
			DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *vertexArray)

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

			// Enable the Direct3D 9 vertex declaration and stream source
			static_cast<VertexArray*>(vertexArray)->enableDirect3DVertexDeclarationAndStreamSource();

			// End debug event
			RENDERER_END_DEBUG_EVENT(this)
		}
		else
		{
			mDirect3DDevice9->SetVertexDeclaration(nullptr);
		}
	}

	void Direct3D9Renderer::setGraphicsViewports([[maybe_unused]] uint32_t numberOfViewports, const Renderer::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Direct3D 9 rasterizer state viewports")

		// Set the Direct3D 9 viewport
		// -> Direct3D 9 supports only one viewport
		RENDERER_ASSERT(mContext, numberOfViewports <= 1, "Direct3D 9 supports only one viewport")
		const D3DVIEWPORT9 direct3D9Viewport =
		{
			static_cast<DWORD>(viewports->topLeftX),	// X (DWORD)
			static_cast<DWORD>(viewports->topLeftY),	// Y (DWORD)
			static_cast<DWORD>(viewports->width),		// Width (DWORD)
			static_cast<DWORD>(viewports->height),		// Height (DWORD)
			viewports->minDepth,						// MinZ (float)
			viewports->maxDepth							// MaxZ (float)
		};
		FAILED_DEBUG_BREAK(mDirect3DDevice9->SetViewport(&direct3D9Viewport));
	}

	void Direct3D9Renderer::setGraphicsScissorRectangles([[maybe_unused]] uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Direct3D 9 rasterizer state scissor rectangles")

		// Set the Direct3D 9 scissor rectangles
		// -> "Renderer::ScissorRectangle" directly maps to Direct3D 9 & 10 & 11, do not change it
		// -> Direct3D 9 supports only one viewport
		RENDERER_ASSERT(mContext, numberOfScissorRectangles <= 1, "Direct3D 9 supports only one scissor rectangle")
		FAILED_DEBUG_BREAK(mDirect3DDevice9->SetScissorRect(reinterpret_cast<const RECT*>(scissorRectangles)));
	}

	void Direct3D9Renderer::setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

			// The "IDirect3DDevice9::SetRenderTarget method"-documentation at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/bb174455%28v=vs.85%29.aspx states:
			//   "Setting a new render target will cause the viewport (see Viewports and Clipping (Direct3D 9)) to be set to the full size of the new render target."
			// -> Although it's not mentioned within the documentation, the same behaviour is true for the scissor rectangle
			// -> This behaviour is different from Direct3D 10, Direct3D 11, OpenGL and OpenGL ES 3
			// -> We have to compensate the Direct3D 9 behaviour in here

			// Backup the currently set Direct3D 9 viewport and scissor rectangle
			D3DVIEWPORT9 direct3D9ViewportBackup;
			FAILED_DEBUG_BREAK(mDirect3DDevice9->GetViewport(&direct3D9ViewportBackup));
			RECT direct3D9ScissorRectangleBackup;
			FAILED_DEBUG_BREAK(mDirect3DDevice9->GetScissorRect(&direct3D9ScissorRectangleBackup));

			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Sanity check
				DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *renderTarget)

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					mRenderTarget->releaseReference();
				}

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// Evaluate the render target type
				switch (mRenderTarget->getResourceType())
				{
					case Renderer::ResourceType::SWAP_CHAIN:
					{
						// Get the Direct3D 9 swap chain instance
						SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

						// Set the Direct3D 9 default color surfaces
						FAILED_DEBUG_BREAK(mDirect3DDevice9->SetRenderTarget(0, swapChain->getDirect3DSurface9RenderTarget()));
						for (DWORD direct3D9RenderTargetIndex = 1; direct3D9RenderTargetIndex < mCapabilities.maximumNumberOfSimultaneousRenderTargets; ++direct3D9RenderTargetIndex)
						{
							FAILED_DEBUG_BREAK(mDirect3DDevice9->SetRenderTarget(direct3D9RenderTargetIndex, nullptr));
						}

						// Set the Direct3D 9 default depth stencil surface
						FAILED_DEBUG_BREAK(mDirect3DDevice9->SetDepthStencilSurface(swapChain->getDirect3DSurface9DepthStencil()));
						break;
					}

					case Renderer::ResourceType::FRAMEBUFFER:
					{
						// Get the Direct3D 9 framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Set the Direct3D 9 color surfaces
						DWORD direct3D9RenderTargetIndex = 0;
						IDirect3DSurface9** direct3D9ColorSurfacesEnd = framebuffer->getDirect3DSurface9Colors() + framebuffer->getNumberOfDirect3DSurface9Colors();
						for (IDirect3DSurface9** direct3D9ColorSurface = framebuffer->getDirect3DSurface9Colors(); direct3D9ColorSurface < direct3D9ColorSurfacesEnd; ++direct3D9ColorSurface, ++direct3D9RenderTargetIndex)
						{
							FAILED_DEBUG_BREAK(mDirect3DDevice9->SetRenderTarget(direct3D9RenderTargetIndex, *direct3D9ColorSurface));
						}

						// Set the Direct3D 9 depth stencil surface
						FAILED_DEBUG_BREAK(mDirect3DDevice9->SetDepthStencilSurface(framebuffer->getDirect3DSurface9DepthStencil()));
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
				// Set no Direct3D 9 color surfaces
				for (DWORD direct3D9RenderTargetIndex = 0; direct3D9RenderTargetIndex < mCapabilities.maximumNumberOfSimultaneousRenderTargets; ++direct3D9RenderTargetIndex)
				{
					FAILED_DEBUG_BREAK(mDirect3DDevice9->SetRenderTarget(direct3D9RenderTargetIndex, nullptr));
				}

				// Set no Direct3D 9 depth stencil surface
				FAILED_DEBUG_BREAK(mDirect3DDevice9->SetDepthStencilSurface(nullptr));

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					mRenderTarget->releaseReference();
					mRenderTarget = nullptr;
				}
			}

			// Restore the previously set Direct3D 9 viewport and scissor rectangle
			FAILED_DEBUG_BREAK(mDirect3DDevice9->SetViewport(&direct3D9ViewportBackup));
			FAILED_DEBUG_BREAK(mDirect3DDevice9->SetScissorRect(&direct3D9ScissorRectangleBackup));

			// End debug event
			RENDERER_END_DEBUG_EVENT(this)
		}
	}

	void Direct3D9Renderer::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Begin debug event
		RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

		// For Direct3D 9, the clear color must be between [0..1]
		float normalizedColor[4] = { color[0], color[1], color[2], color[3] };
		for (int i = 0; i < 4; ++i)
		{
			if (normalizedColor[i] < 0.0f)
			{
				normalizedColor[i] = 0.0f;
			}
			if (normalizedColor[i] > 1.0f)
			{
				normalizedColor[i] = 1.0f;
			}
		}
		#ifdef RENDERER_DEBUG
			if (normalizedColor[0] != color[0] || normalizedColor[1] != color[1] || normalizedColor[2] != color[2] || normalizedColor[3] != color[3])
			{
				RENDERER_LOG(mContext, CRITICAL, "The given clear color was clamped to [0, 1] because Direct3D 9 does not support values outside this range")
			}
		#endif

		// Unlike Direct3D 9, when using Direct3D 10, Direct3D 11, OpenGL or OpenGL ES 3, the viewport(s) and scissor rectangle(s) do not affect the clear operation
		// -> We have to compensate the Direct3D 9 behaviour in here

		// Backup the currently set Direct3D 9 viewport
		D3DVIEWPORT9 direct3D9ViewportBackup;
		FAILED_DEBUG_BREAK(mDirect3DDevice9->GetViewport(&direct3D9ViewportBackup));

		// Backup the currently set Direct3D 9 scissor test state
		DWORD direct3D9ScissorTestBackup = 0;
		FAILED_DEBUG_BREAK(mDirect3DDevice9->GetRenderState(D3DRS_SCISSORTESTENABLE, &direct3D9ScissorTestBackup));

		// Get the current primary render target
		IDirect3DSurface9* direct3DSurface9 = nullptr;
		if (D3D_OK == mDirect3DDevice9->GetRenderTarget(0, &direct3DSurface9))
		{
			// Get the surface description of the primary render target
			D3DSURFACE_DESC d3dSurfaceDesc;
			FAILED_DEBUG_BREAK(direct3DSurface9->GetDesc(&d3dSurfaceDesc));

			// Set a Direct3D 9 viewport which covers the whole current render target
			const D3DVIEWPORT9 direct3D9Viewport =
			{
				0,						// X (DWORD)
				0,						// Y (DWORD)
				d3dSurfaceDesc.Width,	// Width (DWORD)
				d3dSurfaceDesc.Height,	// Height (DWORD)
				0.0f,					// MinZ (float)
				1.0f					// MaxZ (float)
			};
			FAILED_DEBUG_BREAK(mDirect3DDevice9->SetViewport(&direct3D9Viewport));

			// Release the render target
			direct3DSurface9->Release();
		}

		// Disable Direct3D 9 scissor test
		FAILED_DEBUG_BREAK(mDirect3DDevice9->SetRenderState(D3DRS_SCISSORTESTENABLE, 0));

		// Get API flags
		uint32_t flagsApi = 0;
		if (clearFlags & Renderer::ClearFlag::COLOR)
		{
			flagsApi |= D3DCLEAR_TARGET;
		}
		if (clearFlags & Renderer::ClearFlag::DEPTH)
		{
			flagsApi |= D3DCLEAR_ZBUFFER;
		}
		if (clearFlags & Renderer::ClearFlag::STENCIL)
		{
			flagsApi |= D3DCLEAR_STENCIL;
		}

		// Clear
		FAILED_DEBUG_BREAK(mDirect3DDevice9->Clear(0, nullptr, flagsApi, D3DCOLOR_COLORVALUE(normalizedColor[0], normalizedColor[1], normalizedColor[2], normalizedColor[3]), z, stencil));

		// Restore the previously set Direct3D 9 viewport
		FAILED_DEBUG_BREAK(mDirect3DDevice9->SetViewport(&direct3D9ViewportBackup));

		// Restore previously set Direct3D 9 scissor test state
		FAILED_DEBUG_BREAK(mDirect3DDevice9->SetRenderState(D3DRS_SCISSORTESTENABLE, direct3D9ScissorTestBackup));

		// End debug event
		RENDERER_END_DEBUG_EVENT(this)
	}

	void Direct3D9Renderer::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Direct3D 9 emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 9 draws must not be zero")

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

			// No instancing supported here
			// -> In Direct3D 9, instanced arrays is only possible when drawing indexed primitives, see
			//    "Efficiently Drawing Multiple Instances of Geometry (Direct3D 9)"-article at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb173349%28v=vs.85%29.aspx#Drawing_Non_Indexed_Geometry
			// -> This document states that this is not supported by hardware acceleration on any device, and it's long winded anyway
			RENDERER_ASSERT(mContext, 1 == drawArguments.instanceCount, "Direct3D 9 instance count must be one")
			RENDERER_ASSERT(mContext, 0 == drawArguments.startInstanceLocation, "Direct3D 9 start instance location must be zero")

			{ // Draw
				// Get number of primitives
				uint32_t primitiveCount;
				switch (mPrimitiveTopology)
				{
					case Renderer::PrimitiveTopology::POINT_LIST:
						primitiveCount = drawArguments.vertexCountPerInstance;
						break;

					case Renderer::PrimitiveTopology::LINE_LIST:
						primitiveCount = drawArguments.vertexCountPerInstance - 1;
						break;

					case Renderer::PrimitiveTopology::LINE_STRIP:
						primitiveCount = drawArguments.vertexCountPerInstance - 1;
						break;

					case Renderer::PrimitiveTopology::TRIANGLE_LIST:
						primitiveCount = drawArguments.vertexCountPerInstance / 3;
						break;

					case Renderer::PrimitiveTopology::TRIANGLE_STRIP:
						primitiveCount = drawArguments.vertexCountPerInstance - 2;
						break;

					case Renderer::PrimitiveTopology::UNKNOWN:
					case Renderer::PrimitiveTopology::PATCH_LIST_1:
					case Renderer::PrimitiveTopology::PATCH_LIST_2:
					case Renderer::PrimitiveTopology::PATCH_LIST_3:
					case Renderer::PrimitiveTopology::PATCH_LIST_4:
					case Renderer::PrimitiveTopology::PATCH_LIST_5:
					case Renderer::PrimitiveTopology::PATCH_LIST_6:
					case Renderer::PrimitiveTopology::PATCH_LIST_7:
					case Renderer::PrimitiveTopology::PATCH_LIST_8:
					case Renderer::PrimitiveTopology::PATCH_LIST_9:
					case Renderer::PrimitiveTopology::PATCH_LIST_10:
					case Renderer::PrimitiveTopology::PATCH_LIST_11:
					case Renderer::PrimitiveTopology::PATCH_LIST_12:
					case Renderer::PrimitiveTopology::PATCH_LIST_13:
					case Renderer::PrimitiveTopology::PATCH_LIST_14:
					case Renderer::PrimitiveTopology::PATCH_LIST_15:
					case Renderer::PrimitiveTopology::PATCH_LIST_16:
					case Renderer::PrimitiveTopology::PATCH_LIST_17:
					case Renderer::PrimitiveTopology::PATCH_LIST_18:
					case Renderer::PrimitiveTopology::PATCH_LIST_19:
					case Renderer::PrimitiveTopology::PATCH_LIST_20:
					case Renderer::PrimitiveTopology::PATCH_LIST_21:
					case Renderer::PrimitiveTopology::PATCH_LIST_22:
					case Renderer::PrimitiveTopology::PATCH_LIST_23:
					case Renderer::PrimitiveTopology::PATCH_LIST_24:
					case Renderer::PrimitiveTopology::PATCH_LIST_25:
					case Renderer::PrimitiveTopology::PATCH_LIST_26:
					case Renderer::PrimitiveTopology::PATCH_LIST_27:
					case Renderer::PrimitiveTopology::PATCH_LIST_28:
					case Renderer::PrimitiveTopology::PATCH_LIST_29:
					case Renderer::PrimitiveTopology::PATCH_LIST_30:
					case Renderer::PrimitiveTopology::PATCH_LIST_31:
					case Renderer::PrimitiveTopology::PATCH_LIST_32:
					default:
						return;	// Error!
				}

				// The "Renderer::PrimitiveTopology" values directly map to Direct3D 9 & 10 & 11 constants, do not change them
				FAILED_DEBUG_BREAK(mDirect3DDevice9->DrawPrimitive(static_cast<D3DPRIMITIVETYPE>(mPrimitiveTopology), drawArguments.startVertexLocation, primitiveCount));
			}

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

	void Direct3D9Renderer::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Direct3D 9 emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 9 draws must not be zero")

		// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		if (mCapabilities.instancedArrays)
		{
			// TODO(co) Currently no buffer overflow check due to lack of interface provided data
			emulationData += indirectBufferOffset;

			// Emit the draw calls
			#ifdef RENDERER_DEBUG
				if (numberOfDraws > 1)
				{
					beginDebugEvent("Multi-indexed-draw-indirect emulation");
				}
			#endif
			for (uint32_t i = 0; i < numberOfDraws; ++i)
			{
				const Renderer::DrawIndexedArguments& drawIndexedArguments = *reinterpret_cast<const Renderer::DrawIndexedArguments*>(emulationData);
				RENDERER_ASSERT(mContext, 0 == drawIndexedArguments.startInstanceLocation, "Start instance location isn't supported by Direct3D 9")	// Not supported by Direct3D 9

				// The "Efficiently Drawing Multiple Instances of Geometry (Direct3D 9)"-article at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/bb173349%28v=vs.85%29.aspx#Drawing_Non_Indexed_Geometry
				// states: "Note that D3DSTREAMSOURCE_INDEXEDDATA and the number of instances to draw must always be set in stream zero."
				// -> "D3DSTREAMSOURCE_INSTANCEDATA" is set within "Direct3D9Renderer::VertexArray::enableDirect3DVertexDeclarationAndStreamSource()"
				FAILED_DEBUG_BREAK(mDirect3DDevice9->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | drawIndexedArguments.instanceCount));

				{ // Draw
					// Get number of primitives
					uint32_t primitiveCount;
					switch (mPrimitiveTopology)
					{
						case Renderer::PrimitiveTopology::POINT_LIST:
							primitiveCount = drawIndexedArguments.indexCountPerInstance;
							break;

						case Renderer::PrimitiveTopology::LINE_LIST:
							primitiveCount = drawIndexedArguments.indexCountPerInstance - 1;
							break;

						case Renderer::PrimitiveTopology::LINE_STRIP:
							primitiveCount = drawIndexedArguments.indexCountPerInstance - 1;
							break;

						case Renderer::PrimitiveTopology::TRIANGLE_LIST:
							primitiveCount = drawIndexedArguments.indexCountPerInstance / 3;
							break;

						case Renderer::PrimitiveTopology::TRIANGLE_STRIP:
							primitiveCount = drawIndexedArguments.indexCountPerInstance - 2;
							break;

						case Renderer::PrimitiveTopology::UNKNOWN:
						case Renderer::PrimitiveTopology::PATCH_LIST_1:
						case Renderer::PrimitiveTopology::PATCH_LIST_2:
						case Renderer::PrimitiveTopology::PATCH_LIST_3:
						case Renderer::PrimitiveTopology::PATCH_LIST_4:
						case Renderer::PrimitiveTopology::PATCH_LIST_5:
						case Renderer::PrimitiveTopology::PATCH_LIST_6:
						case Renderer::PrimitiveTopology::PATCH_LIST_7:
						case Renderer::PrimitiveTopology::PATCH_LIST_8:
						case Renderer::PrimitiveTopology::PATCH_LIST_9:
						case Renderer::PrimitiveTopology::PATCH_LIST_10:
						case Renderer::PrimitiveTopology::PATCH_LIST_11:
						case Renderer::PrimitiveTopology::PATCH_LIST_12:
						case Renderer::PrimitiveTopology::PATCH_LIST_13:
						case Renderer::PrimitiveTopology::PATCH_LIST_14:
						case Renderer::PrimitiveTopology::PATCH_LIST_15:
						case Renderer::PrimitiveTopology::PATCH_LIST_16:
						case Renderer::PrimitiveTopology::PATCH_LIST_17:
						case Renderer::PrimitiveTopology::PATCH_LIST_18:
						case Renderer::PrimitiveTopology::PATCH_LIST_19:
						case Renderer::PrimitiveTopology::PATCH_LIST_20:
						case Renderer::PrimitiveTopology::PATCH_LIST_21:
						case Renderer::PrimitiveTopology::PATCH_LIST_22:
						case Renderer::PrimitiveTopology::PATCH_LIST_23:
						case Renderer::PrimitiveTopology::PATCH_LIST_24:
						case Renderer::PrimitiveTopology::PATCH_LIST_25:
						case Renderer::PrimitiveTopology::PATCH_LIST_26:
						case Renderer::PrimitiveTopology::PATCH_LIST_27:
						case Renderer::PrimitiveTopology::PATCH_LIST_28:
						case Renderer::PrimitiveTopology::PATCH_LIST_29:
						case Renderer::PrimitiveTopology::PATCH_LIST_30:
						case Renderer::PrimitiveTopology::PATCH_LIST_31:
						case Renderer::PrimitiveTopology::PATCH_LIST_32:
						default:
							return; // Error!
					}

					// The "Renderer::PrimitiveTopology" values directly map to Direct3D 9 & 10 & 11 constants, do not change them
					const UINT numberOfVertices = drawIndexedArguments.indexCountPerInstance * 3;	// TODO(co) Review "numberOfVertices", might be wrong
					FAILED_DEBUG_BREAK(mDirect3DDevice9->DrawIndexedPrimitive(static_cast<D3DPRIMITIVETYPE>(mPrimitiveTopology), static_cast<INT>(drawIndexedArguments.baseVertexLocation), 0, numberOfVertices, drawIndexedArguments.startIndexLocation, primitiveCount));
				}

				// Advance
				emulationData += sizeof(Renderer::DrawIndexedArguments);
			}
			#ifdef RENDERER_DEBUG
				if (numberOfDraws > 1)
				{
					endDebugEvent();
				}
			#endif

			// Reset the stream source frequency
			FAILED_DEBUG_BREAK(mDirect3DDevice9->SetStreamSourceFreq(0, 1));
		}
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void Direct3D9Renderer::resolveMultisampleFramebuffer(Renderer::IRenderTarget&, Renderer::IFramebuffer&)
	{
		// TODO(co) Implement me
	}

	void Direct3D9Renderer::copyResource(Renderer::IResource&, Renderer::IResource&)
	{
		// TODO(co) Implement me
	}

	void Direct3D9Renderer::generateMipmaps(Renderer::IResource&)
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void Direct3D9Renderer::resetQueryPool([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// Sanity check
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}

	void Direct3D9Renderer::beginQuery([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex, [[maybe_unused]] uint32_t queryControlFlags)
	{
		// Sanity check
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}

	void Direct3D9Renderer::endQuery([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}

	void Direct3D9Renderer::writeTimestampQuery([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t queryIndex)
	{
		// Sanity check
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
		NOP;
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void Direct3D9Renderer::setDebugMarker(const char* name)
		{
			if (nullptr != D3DPERF_SetMarker)
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Direct3D 9 debug marker names must not be a null pointer")
				RENDERER_ASSERT(mContext, strlen(name) < 256, "Direct3D 9 debug marker names must not have more than 255 characters")
				wchar_t unicodeName[256];
				std::mbstowcs(unicodeName, name, 256);
				D3DPERF_SetMarker(D3DCOLOR_RGBA(255, 0, 255, 255), unicodeName);
			}
		}

		void Direct3D9Renderer::beginDebugEvent(const char* name)
		{
			if (nullptr != D3DPERF_BeginEvent)
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Direct3D 9 debug event names must not be a null pointer")
				RENDERER_ASSERT(mContext, strlen(name) < 256, "Direct3D 9 debug event names must not have more than 255 characters")
				wchar_t unicodeName[256];
				std::mbstowcs(unicodeName, name, 256);
				D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 255, 255), unicodeName);
			}
		}

		void Direct3D9Renderer::endDebugEvent()
		{
			if (nullptr != D3DPERF_EndEvent)
			{
				D3DPERF_EndEvent();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	bool Direct3D9Renderer::isDebugEnabled()
	{
		// Don't check for the "RENDERER_DEBUG" preprocessor definition, even if debug
		// is disabled it has to be possible to use this function for an additional security check
		// -> Maybe a debugger/profiler ignores the debug state
		// -> Maybe someone manipulated the binary to enable the debug state, adding a second check
		//    makes it a little bit more time consuming to hack the binary :D (but of course, this is no 100% security)
		return (nullptr != D3DPERF_GetStatus && D3DPERF_GetStatus() != 0);
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t Direct3D9Renderer::getNumberOfShaderLanguages() const
	{
		uint32_t numberOfShaderLanguages = 1;	// HLSL support is always there

		// Done, return the number of supported shader languages
		return numberOfShaderLanguages;
	}

	const char* Direct3D9Renderer::getShaderLanguageName(uint32_t index) const
	{
		// HLSL supported
		if (0 == index)
		{
			return ::detail::HLSL_NAME;
		}

		// Error!
		return nullptr;
	}

	Renderer::IShaderLanguage* Direct3D9Renderer::getShaderLanguage(const char* shaderLanguageName)
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
	Renderer::IRenderPass* Direct3D9Renderer::createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples)
	{
		return RENDERER_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples);
	}

	Renderer::IQueryPool* Direct3D9Renderer::createQueryPool([[maybe_unused]] Renderer::QueryType queryType, [[maybe_unused]] uint32_t numberOfQueries)
	{
		// TODO(co) Implement me
		return nullptr;
	}

	Renderer::ISwapChain* Direct3D9Renderer::createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool)
	{
		// Sanity checks
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)
		RENDERER_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Direct3D 9: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RENDERER_NEW(mContext, SwapChain)(renderPass, windowHandle);
	}

	Renderer::IFramebuffer* Direct3D9Renderer::createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment)
	{
		// Sanity check
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)

		// Create the framebuffer
		return RENDERER_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment);
	}

	Renderer::IBufferManager* Direct3D9Renderer::createBufferManager()
	{
		return RENDERER_NEW(mContext, BufferManager)(*this);
	}

	Renderer::ITextureManager* Direct3D9Renderer::createTextureManager()
	{
		return RENDERER_NEW(mContext, TextureManager)(*this);
	}

	Renderer::IRootSignature* Direct3D9Renderer::createRootSignature(const Renderer::RootSignature& rootSignature)
	{
		return RENDERER_NEW(mContext, RootSignature)(*this, rootSignature);
	}

	Renderer::IGraphicsPipelineState* Direct3D9Renderer::createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "Direct3D 9: Invalid graphics pipeline state root signature")
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "Direct3D 9: Invalid graphics pipeline state graphics program")
		RENDERER_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "Direct3D 9: Invalid graphics pipeline state render pass")

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

	Renderer::IComputePipelineState* Direct3D9Renderer::createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader)
	{
		// Sanity checks
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, rootSignature)
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, computeShader)

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();

		// Error! Direct3D 9 has no compute shader support.
		return nullptr;
	}

	Renderer::ISamplerState* Direct3D9Renderer::createSamplerState(const Renderer::SamplerState& samplerState)
	{
		return RENDERER_NEW(mContext, SamplerState)(*this, samplerState);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool Direct3D9Renderer::map(Renderer::IResource& resource, uint32_t subresource, Renderer::MapType mapType, uint32_t, Renderer::MappedSubresource& mappedSubresource)
	{
		// The "Renderer::MapType" values directly map to Direct3D 10 & 11 constants, do not change them
		// The "Renderer::MappedSubresource" structure directly maps to Direct3D 11, do not change it

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
			{
				// Lock the Direct3D 9 resource
				DWORD flags = 0;
				// TODO(co) Map all flags correctly
				if (Renderer::MapType::READ == mapType)
				{
					flags = D3DLOCK_READONLY;
				}
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (D3D_OK == static_cast<IndexBuffer&>(resource).getDirect3DIndexBuffer9()->Lock(0, 0, &mappedSubresource.data, flags));
			}

			case Renderer::ResourceType::VERTEX_BUFFER:
			{
				// Lock the Direct3D 9 resource
				DWORD flags = 0;
				// TODO(co) Map all flags correctly
				if (Renderer::MapType::READ == mapType)
				{
					flags = D3DLOCK_READONLY;
				}
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (D3D_OK == static_cast<VertexBuffer&>(resource).getDirect3DVertexBuffer9()->Lock(0, 0, &mappedSubresource.data, flags));
			}

			case Renderer::ResourceType::INDIRECT_BUFFER:
				mappedSubresource.data		 = static_cast<IndirectBuffer&>(resource).getWritableEmulationData();
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return true;

			case Renderer::ResourceType::TEXTURE_1D:
				// TODO(co) Implement Direct3D 9 1D texture
				RENDERER_LOG(mContext, CRITICAL, "The 1D texture support is not yet implemented inside the Direct3D 9 renderer backend")
				return false;

			case Renderer::ResourceType::TEXTURE_2D:
			{
				bool result = false;

				// TODO(co) In case this texture is a render target, we need to use "IDirect3DDevice9::GetRenderTargetData"-method http://msdn.microsoft.com/en-us/library/bb174405%28VS.85%29.aspx
				// Possible implementation hints from http://stackoverflow.com/questions/120066/doing-readback-from-direct3d-textures-and-surfaces
				/*
					bool GfxDeviceD3D9::ReadbackImage(  params  )
					{
						HRESULT hr;
						IDirect3DDevice9* dev = GetD3DDevice();
						SurfacePointer renderTarget;
						hr = dev->GetRenderTarget( 0, &renderTarget );
						if( !renderTarget || FAILED(hr) )
							return false;

						D3DSURFACE_DESC rtDesc;
						renderTarget->GetDesc( &rtDesc );

						SurfacePointer resolvedSurface;
						if( rtDesc.MultiSampleType != D3DMULTISAMPLE_NONE )
						{
							hr = dev->CreateRenderTarget( rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &resolvedSurface, NULL );
							if( FAILED(hr) )
								return false;
							hr = dev->StretchRect( renderTarget, NULL, resolvedSurface, NULL, D3DTEXF_NONE );
							if( FAILED(hr) )
								return false;
							renderTarget = resolvedSurface;
						}

						SurfacePointer offscreenSurface;
						hr = dev->CreateOffscreenPlainSurface( rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DPOOL_SYSTEMMEM, &offscreenSurface, NULL );
						if( FAILED(hr) )
							return false;

						hr = dev->GetRenderTargetData( renderTarget, offscreenSurface );
						bool ok = SUCCEEDED(hr);
						if( ok )
						{
							// Here we have data in offscreenSurface.
							D3DLOCKED_RECT lr;
							RECT rect;
							rect.left = 0;
							rect.right = rtDesc.Width;
							rect.top = 0;
							rect.bottom = rtDesc.Height;
							// Lock the surface to read pixels
							hr = offscreenSurface->LockRect( &lr, &rect, D3DLOCK_READONLY );
							if( SUCCEEDED(hr) )
							{
								// Pointer to data is lt.pBits, each row is
								// lr.Pitch bytes apart (often it is the same as width*bpp, but
								// can be larger if driver uses padding)

								// Read the data here!
								offscreenSurface->UnlockRect();
							}
							else
							{
								ok = false;
							}
						}

						return ok;
					}
				*/

				// Lock the Direct3D 9 resource
				DWORD flags = 0;
				// TODO(co) Map all flags correctly
				if (Renderer::MapType::READ == mapType)
				{
					flags = D3DLOCK_READONLY;
				}
				D3DLOCKED_RECT d3dLockedRect;
				result = (D3D_OK == static_cast<Texture2D&>(resource).getDirect3DTexture9()->LockRect(subresource, &d3dLockedRect, nullptr, flags));

				// Copy over the data
				mappedSubresource.data		 = d3dLockedRect.pBits;
				mappedSubresource.rowPitch   = static_cast<UINT>(d3dLockedRect.Pitch);
				mappedSubresource.depthPitch = 0;

				// Done
				return result;
			}

			case Renderer::ResourceType::TEXTURE_3D:
				// TODO(co) Implement Direct3D 9 3D texture
				RENDERER_LOG(mContext, CRITICAL, "The 3D texture support is not yet implemented inside the Direct3D 9 renderer backend")
				return false;

			case Renderer::ResourceType::TEXTURE_CUBE:
				// TODO(co) Implement Direct3D 9 cube texture
				RENDERER_LOG(mContext, CRITICAL, "The cube texture support is not yet implemented inside the Direct3D 9 renderer backend")
				return false;

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::TEXTURE_BUFFER:
			case Renderer::ResourceType::STRUCTURED_BUFFER:
			case Renderer::ResourceType::UNIFORM_BUFFER:
			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
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

	void Direct3D9Renderer::unmap(Renderer::IResource& resource, uint32_t subresource)
	{
		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				static_cast<IndexBuffer&>(resource).getDirect3DIndexBuffer9()->Unlock();
				break;

			case Renderer::ResourceType::VERTEX_BUFFER:
				static_cast<VertexBuffer&>(resource).getDirect3DVertexBuffer9()->Unlock();
				break;

			case Renderer::ResourceType::INDIRECT_BUFFER:
				// Nothing here, it's a software emulated indirect buffer
				break;

			case Renderer::ResourceType::TEXTURE_1D:
				// TODO(co) Implement Direct3D 9 1D texture
				RENDERER_LOG(mContext, CRITICAL, "The 1D texture support is not yet implemented inside the Direct3D 9 renderer backend")
				break;

			case Renderer::ResourceType::TEXTURE_2D:
				static_cast<Texture2D&>(resource).getDirect3DTexture9()->UnlockRect(subresource);
				break;

			case Renderer::ResourceType::TEXTURE_3D:
				// TODO(co) Implement Direct3D 9 3D texture
				RENDERER_LOG(mContext, CRITICAL, "The 3D texture support is not yet implemented inside the Direct3D 9 renderer backend")
				break;

			case Renderer::ResourceType::TEXTURE_CUBE:
				// TODO(co) Implement Direct3D 9 cube texture
				RENDERER_LOG(mContext, CRITICAL, "The cube texture support is not yet implemented inside the Direct3D 9 renderer backend")
				break;

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
			case Renderer::ResourceType::QUERY_POOL:
			case Renderer::ResourceType::SWAP_CHAIN:
			case Renderer::ResourceType::FRAMEBUFFER:
			case Renderer::ResourceType::TEXTURE_BUFFER:
			case Renderer::ResourceType::STRUCTURED_BUFFER:
			case Renderer::ResourceType::UNIFORM_BUFFER:
			case Renderer::ResourceType::TEXTURE_2D_ARRAY:
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

	bool Direct3D9Renderer::getQueryPoolResults([[maybe_unused]] Renderer::IQueryPool& queryPool, [[maybe_unused]] uint32_t numberOfDataBytes, [[maybe_unused]] uint8_t* data, [[maybe_unused]] uint32_t firstQueryIndex, [[maybe_unused]] uint32_t numberOfQueries, [[maybe_unused]] uint32_t strideInBytes, [[maybe_unused]] uint32_t queryResultFlags)
	{
		// Sanity check
		DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, queryPool)

		// TODO(co) Implement me
		return false;
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool Direct3D9Renderer::beginScene()
	{
		return SUCCEEDED(mDirect3DDevice9->BeginScene());
	}

	void Direct3D9Renderer::submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer)
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

	void Direct3D9Renderer::endScene()
	{
		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);

		FAILED_DEBUG_BREAK(mDirect3DDevice9->EndScene());
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void Direct3D9Renderer::flush()
	{
		// Create the Direct3D 9 query instance used for flush right now?
		if (nullptr == mDirect3DQuery9Flush)
		{
			FAILED_DEBUG_BREAK(mDirect3DDevice9->CreateQuery(D3DQUERYTYPE_EVENT, &mDirect3DQuery9Flush));

			// "IDirect3DQuery9" is not derived from "IDirect3DResource9", meaning we can't use the "IDirect3DResource9::SetPrivateData()"-method in order to set a debug name
		}
		if (nullptr != mDirect3DQuery9Flush)
		{
			// Perform the flush
			FAILED_DEBUG_BREAK(mDirect3DQuery9Flush->Issue(D3DISSUE_END));
			FAILED_DEBUG_BREAK(mDirect3DQuery9Flush->GetData(nullptr, 0, D3DGETDATA_FLUSH));
		}
	}

	void Direct3D9Renderer::finish()
	{
		// Create the Direct3D 9 query instance used for flush right now?
		if (nullptr == mDirect3DQuery9Flush)
		{
			FAILED_DEBUG_BREAK(mDirect3DDevice9->CreateQuery(D3DQUERYTYPE_EVENT, &mDirect3DQuery9Flush));

			// "IDirect3DQuery9" is not derived from "IDirect3DResource9", meaning we can't use the "IDirect3DResource9::SetPrivateData()"-method in order to set a debug name
		}
		if (nullptr != mDirect3DQuery9Flush)
		{
			// Perform the flush and wait
			FAILED_DEBUG_BREAK(mDirect3DQuery9Flush->Issue(D3DISSUE_END));
			while (mDirect3DQuery9Flush->GetData(nullptr, 0, D3DGETDATA_FLUSH) == S_FALSE)
			{
				// Spin-wait
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void Direct3D9Renderer::initializeCapabilities()
	{
		// Get Direct3D 9 device capabilities
		D3DCAPS9 d3dCaps9;
		FAILED_DEBUG_BREAK(mDirect3DDevice9->GetDeviceCaps(&d3dCaps9));

		{ // Get device name
		  // -> The adapter contains a description like "AMD Radeon R9 200 Series"
			D3DADAPTER_IDENTIFIER9 d3dAdapterIdentifier9;
			FAILED_DEBUG_BREAK(mDirect3D9->GetAdapterIdentifier(d3dCaps9.AdapterOrdinal, 0, &d3dAdapterIdentifier9));
			const size_t numberOfCharacters = _countof(mCapabilities.deviceName) - 1;
			strncpy(mCapabilities.deviceName, d3dAdapterIdentifier9.Description, numberOfCharacters);
			mCapabilities.deviceName[numberOfCharacters] = '\0';
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Renderer::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		mCapabilities.maximumNumberOfViewports = 1;	// Direct3D 9 only supports a single viewport

		// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
		// -> Direct3D 9 supports a maximum number of 4 simultaneous render targets
		mCapabilities.maximumNumberOfSimultaneousRenderTargets = d3dCaps9.NumSimultaneousRTs;

		// Maximum texture dimension
		mCapabilities.maximumTextureDimension = d3dCaps9.MaxTextureWidth;	// Width and height are usually identical, usually...

		// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
		mCapabilities.maximumNumberOf2DTextureArraySlices = 0;

		// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		mCapabilities.maximumTextureBufferSize = 0;

		// Direct3D 9 doesn't support structured buffer
		mCapabilities.maximumStructuredBufferSize = 0;

		// Maximum indirect buffer size in bytes
		mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		mCapabilities.maximumUniformBufferSize = 0;

		// Maximum number of multisamples (always at least 1, usually 8)
		mCapabilities.maximumNumberOfMultisamples = 1;	// Don't want to support the legacy DirectX 9 multisample support

		// Maximum anisotropy (always at least 1, usually 16)
		mCapabilities.maximumAnisotropy = 16;

		// Left-handed coordinate system with clip space depth value range 0..1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = true;

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = true;

		// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		mCapabilities.instancedArrays = (d3dCaps9.PixelShaderVersion >= D3DPS_VERSION(3, 0));

		// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
		mCapabilities.drawInstanced = false;

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = true;

		// Direct3D 9 has no native multi-threading
		mCapabilities.nativeMultiThreading = false;

		// Direct3D 9 has shader bytecode support
		// TODO(co) Direct3D 9 shader bytecode support is under construction
		mCapabilities.shaderBytecode = false;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 9 has no tessellation support

		// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
		mCapabilities.maximumNumberOfGsOutputVertices = 0;	// Direct3D 9 has no support for geometry shaders

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = false;

		// We only target graphics hardware which also supports ATI1N and ATI2N, so no need to add this inside the capabilities
		// -> The following is for debugging only, don't delete it
		#if 0
		{
			D3DDISPLAYMODE d3dDisplayMode;
			mDirect3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3dDisplayMode);

			// Check if ATI1N is supported
			bool ati1NSupported = (mDirect3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dDisplayMode.Format, 0, D3DRTYPE_TEXTURE, FOURCC_ATI1N) == D3D_OK);
			ati1NSupported = ati1NSupported;

			// Check if ATI2N is supported
			bool ati2NSupported = (mDirect3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dDisplayMode.Format, 0, D3DRTYPE_TEXTURE, FOURCC_ATI2N) == D3D_OK);
			ati2NSupported = ati2NSupported;
			NOP;
		}
		#endif
	}

	void Direct3D9Renderer::setGraphicsProgram(Renderer::IGraphicsProgram* graphicsProgram)
	{
		// Begin debug event
		RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

		if (nullptr != graphicsProgram)
		{
			// Sanity check
			DIRECT3D9RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsProgram)

			// Get shaders
			const GraphicsProgramHlsl* graphicsProgramHlsl	 = static_cast<GraphicsProgramHlsl*>(graphicsProgram);
			const VertexShaderHlsl*	   vertexShaderHlsl		 = graphicsProgramHlsl->getVertexShaderHlsl();
			const FragmentShaderHlsl*  fragmentShaderHlsl	 = graphicsProgramHlsl->getFragmentShaderHlsl();
			IDirect3DVertexShader9*    direct3DVertexShader9 = vertexShaderHlsl  ? vertexShaderHlsl->getDirect3DVertexShader9()  : nullptr;
			IDirect3DPixelShader9*     direct3DPixelShader9  = fragmentShaderHlsl ? fragmentShaderHlsl->getDirect3DPixelShader9() : nullptr;

			// Set shaders
			if (mDirect3DVertexShader9 != direct3DVertexShader9)
			{
				mDirect3DVertexShader9 = direct3DVertexShader9;
				mDirect3DDevice9->SetVertexShader(mDirect3DVertexShader9);
			}
			if (mDirect3DPixelShader9 != direct3DPixelShader9)
			{
				mDirect3DPixelShader9 = direct3DPixelShader9;
				mDirect3DDevice9->SetPixelShader(mDirect3DPixelShader9);
			}
		}
		else
		{
			if (nullptr != mDirect3DVertexShader9)
			{
				mDirect3DDevice9->SetVertexShader(nullptr);
				mDirect3DVertexShader9 = nullptr;
			}
			if (nullptr != mDirect3DPixelShader9)
			{
				mDirect3DDevice9->SetPixelShader(nullptr);
				mDirect3DPixelShader9 = nullptr;
			}
		}

		// End debug event
		RENDERER_END_DEBUG_EVENT(this)
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D9Renderer




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RENDERER_DIRECT3D9_EXPORTS
	#define DIRECT3D9RENDERER_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define DIRECT3D9RENDERER_FUNCTION_EXPORT
#endif
DIRECT3D9RENDERER_FUNCTION_EXPORT Renderer::IRenderer* createDirect3D9RendererInstance(const Renderer::Context& context)
{
	return RENDERER_NEW(context, Direct3D9Renderer::Direct3D9Renderer)(context);
}
#undef DIRECT3D9RENDERER_FUNCTION_EXPORT
