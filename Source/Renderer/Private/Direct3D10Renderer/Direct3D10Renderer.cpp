/*********************************************************\
 * Copyright (c) 2012-2018 The Unrimp Team
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
*    Direct3D 10 renderer amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    Direct3D 10 runtime and Direct3D 10 capable graphics driver, nothing else.
*
*    == Preprocessor Definitions ==
*    - Set "RENDERER_DIRECT3D10_EXPORTS" as preprocessor definition when building this library as shared library
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

#include <VersionHelpers.h>	// For "IsWindows8OrGreater()"




//[-------------------------------------------------------]
//[ Direct3D10Renderer/D3D10.h                            ]
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
    use it inside cpp-files. It must still be possible that users of this renderer interface
    can use the Direct3D headers for features not covered by this renderer interface.
*/


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
struct IDXGIOutput;
struct ID3D10Query;
struct IDXGIDevice;
struct IDXGISurface;
struct IDXGIAdapter;
struct ID3D10Buffer;
struct IDXGIAdapter1;
struct ID3D10Counter;
struct ID3D10Resource;
struct IDXGISwapChain;
struct IDXGISwapChain1;
struct ID3D10Texture1D;
struct ID3D10Texture2D;
struct ID3D10Texture3D;
struct ID3D10Predicate;
struct ID3D10BlendState;
struct D3D10_BLEND_DESC;
struct D3D10_QUERY_DESC;
struct ID3DX10ThreadPump;
struct ID3D10PixelShader;
struct D3D10_BUFFER_DESC;
struct ID3D10InputLayout;
struct D3D10_COUNTER_INFO;
struct D3D10_COUNTER_TYPE;
struct D3D10_COUNTER_DESC;
struct ID3D10Asynchronous;
struct ID3D10VertexShader;
struct D3D10_SAMPLER_DESC;
struct ID3D10SamplerState;
struct ID3D10GeometryShader;
struct DXGI_FRAME_STATISTICS;
struct ID3D10RasterizerState;
struct D3D10_SUBRESOURCE_DATA;
struct ID3D10RenderTargetView;
struct ID3D10DepthStencilView;
struct ID3D10DepthStencilState;
struct D3D10_DEPTH_STENCIL_DESC;
struct ID3D10ShaderResourceView;
struct D3D10_RESOURCE_DIMENSION;
struct D3D10_SO_DECLARATION_ENTRY;
struct D3D10_DEPTH_STENCIL_VIEW_DESC;
struct D3D10_RENDER_TARGET_VIEW_DESC;
struct D3D10_SHADER_RESOURCE_VIEW_DESC;


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

// "Microsoft DirectX SDK (June 2010)" -> "d3d9types.h"
typedef DWORD D3DCOLOR;

// "Microsoft DirectX SDK (June 2010)" -> "d3d9types.h"
#define D3DCOLOR_ARGB(a,r,g,b) ((D3DCOLOR)((((a)&0xff)<<24) | (((r)&0xff)<<16) | (((g)&0xff)<<8) | ((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a, r, g, b)

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
#define D3D10_SDK_VERSION							(29)
#define	D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX	(15)
#define	D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT		(8)

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
#define	D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT	(16)

// "Microsoft DirectX SDK (June 2010)" -> "D3DX10.h"
#define D3DX10_DEFAULT	(static_cast<UINT>(-1))

// "Microsoft DirectX SDK (June 2010)" -> "DXGI.h"
#define DXGI_MWA_NO_ALT_ENTER	(1 << 1)

// "Microsoft DirectX SDK (June 2010)" -> "DXGIType.h"
#define DXGI_USAGE_RENDER_TARGET_OUTPUT	(1L << (1 + 4))

// "Microsoft DirectX SDK (June 2010)" -> "d3d10misc.h"
enum D3D10_DRIVER_TYPE
{
	D3D10_DRIVER_TYPE_HARDWARE  = 0,
	D3D10_DRIVER_TYPE_REFERENCE = 1,
	D3D10_DRIVER_TYPE_NULL      = 2,
	D3D10_DRIVER_TYPE_SOFTWARE  = 3,
	D3D10_DRIVER_TYPE_WARP      = 5
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
	D3D10_SRV_DIMENSION_UNKNOWN				= D3D_SRV_DIMENSION_UNKNOWN,
	D3D10_SRV_DIMENSION_BUFFER				= D3D_SRV_DIMENSION_BUFFER,
	D3D10_SRV_DIMENSION_TEXTURE1D			= D3D_SRV_DIMENSION_TEXTURE1D,
	D3D10_SRV_DIMENSION_TEXTURE1DARRAY		= D3D_SRV_DIMENSION_TEXTURE1DARRAY,
	D3D10_SRV_DIMENSION_TEXTURE2D			= D3D_SRV_DIMENSION_TEXTURE2D,
	D3D10_SRV_DIMENSION_TEXTURE2DARRAY		= D3D_SRV_DIMENSION_TEXTURE2DARRAY,
	D3D10_SRV_DIMENSION_TEXTURE2DMS			= D3D_SRV_DIMENSION_TEXTURE2DMS,
	D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY	= D3D_SRV_DIMENSION_TEXTURE2DMSARRAY,
	D3D10_SRV_DIMENSION_TEXTURE3D			= D3D_SRV_DIMENSION_TEXTURE3D,
	D3D10_SRV_DIMENSION_TEXTURECUBE			= D3D_SRV_DIMENSION_TEXTURECUBE
};
typedef D3D_SRV_DIMENSION D3D10_SRV_DIMENSION;

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
enum D3D10_PRIMITIVE_TOPOLOGY
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

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
inline UINT D3D10CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT MipLevels)
{
	return MipSlice + ArraySlice * MipLevels;
}

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_CREATE_DEVICE_FLAG
{
	D3D10_CREATE_DEVICE_SINGLETHREADED								= 0x1,
	D3D10_CREATE_DEVICE_DEBUG										= 0x2,
	D3D10_CREATE_DEVICE_SWITCH_TO_REF								= 0x4,
	D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS	= 0x8,
	D3D10_CREATE_DEVICE_ALLOW_NULL_FROM_MAP							= 0x10,
	D3D10_CREATE_DEVICE_BGRA_SUPPORT								= 0x20,
	D3D10_CREATE_DEVICE_STRICT_VALIDATION							= 0x200
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_BOX
{
	UINT left;
	UINT top;
	UINT front;
	UINT right;
	UINT bottom;
	UINT back;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_VIEWPORT
{
	INT TopLeftX;
	INT TopLeftY;
	UINT Width;
	UINT Height;
	FLOAT MinDepth;
	FLOAT MaxDepth;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef RECT D3D10_RECT;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_RTV_DIMENSION
{
	D3D10_RTV_DIMENSION_UNKNOWN				= 0,
	D3D10_RTV_DIMENSION_BUFFER				= 1,
	D3D10_RTV_DIMENSION_TEXTURE1D			= 2,
	D3D10_RTV_DIMENSION_TEXTURE1DARRAY		= 3,
	D3D10_RTV_DIMENSION_TEXTURE2D			= 4,
	D3D10_RTV_DIMENSION_TEXTURE2DARRAY		= 5,
	D3D10_RTV_DIMENSION_TEXTURE2DMS			= 6,
	D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
	D3D10_RTV_DIMENSION_TEXTURE3D			= 8
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_DSV_DIMENSION
{
	D3D10_DSV_DIMENSION_UNKNOWN				= 0,
	D3D10_DSV_DIMENSION_TEXTURE1D			= 1,
	D3D10_DSV_DIMENSION_TEXTURE1DARRAY		= 2,
	D3D10_DSV_DIMENSION_TEXTURE2D			= 3,
	D3D10_DSV_DIMENSION_TEXTURE2DARRAY		= 4,
	D3D10_DSV_DIMENSION_TEXTURE2DMS			= 5,
	D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY	= 6
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
#define DXGI_PRESENT_ALLOW_TEARING	0x00000200UL

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

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_CLEAR_FLAG
{
	D3D10_CLEAR_DEPTH	= 0x1L,
	D3D10_CLEAR_STENCIL	= 0x2L
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_USAGE
{
	D3D10_USAGE_DEFAULT		= 0,
	D3D10_USAGE_IMMUTABLE	= 1,
	D3D10_USAGE_DYNAMIC		= 2,
	D3D10_USAGE_STAGING		= 3
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_MAP
{
	D3D10_MAP_READ					= 1,
	D3D10_MAP_WRITE					= 2,
	D3D10_MAP_READ_WRITE			= 3,
	D3D10_MAP_WRITE_DISCARD			= 4,
	D3D10_MAP_WRITE_NO_OVERWRITE	= 5
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_BIND_FLAG
{
	D3D10_BIND_VERTEX_BUFFER	= 0x1L,
	D3D10_BIND_INDEX_BUFFER		= 0x2L,
	D3D10_BIND_CONSTANT_BUFFER	= 0x4L,
	D3D10_BIND_SHADER_RESOURCE	= 0x8L,
	D3D10_BIND_STREAM_OUTPUT	= 0x10L,
	D3D10_BIND_RENDER_TARGET	= 0x20L,
	D3D10_BIND_DEPTH_STENCIL	= 0x40L
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_CPU_ACCESS_FLAG
{
	D3D10_CPU_ACCESS_WRITE	= 0x10000L,
	D3D10_CPU_ACCESS_READ	= 0x20000L
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef enum D3D10_RESOURCE_MISC_FLAG
{
	D3D10_RESOURCE_MISC_GENERATE_MIPS		= 0x1L,
	D3D10_RESOURCE_MISC_SHARED				= 0x2L,
	D3D10_RESOURCE_MISC_TEXTURECUBE			= 0x4L,
	D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX	= 0x10L,
	D3D10_RESOURCE_MISC_GDI_COMPATIBLE		= 0x20L
} D3D10_RESOURCE_MISC_FLAG;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_BUFFER_DESC
{
	UINT ByteWidth;
	D3D10_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_BUFFER_RTV
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

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX1D_RTV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX1D_ARRAY_RTV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2D_RTV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2DMS_RTV
{
	UINT UnusedField_NothingToDefine;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2D_ARRAY_RTV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2DMS_ARRAY_RTV
{
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX3D_RTV
{
	UINT MipSlice;
	UINT FirstWSlice;
	UINT WSize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef struct D3D10_TEXTURE1D_DESC
{
	UINT Width;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	D3D10_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
} D3D10_TEXTURE1D_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEXTURE2D_DESC
{
	UINT Width;
	UINT Height;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
	D3D10_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef struct D3D10_TEXTURE3D_DESC
{
	UINT Width;
	UINT Height;
	UINT Depth;
	UINT MipLevels;
	DXGI_FORMAT Format;
	D3D10_USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
} D3D10_TEXTURE3D_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX1D_DSV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX1D_ARRAY_DSV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2D_DSV
{
	UINT MipSlice;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2D_ARRAY_DSV
{
	UINT MipSlice;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2DMS_DSV
{
	UINT UnusedField_NothingToDefine;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2DMS_ARRAY_DSV
{
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_SUBRESOURCE_DATA
{
	const void *pSysMem;
	UINT SysMemPitch;
	UINT SysMemSlicePitch;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_MAPPED_TEXTURE2D
{
	void *pData;
	UINT RowPitch;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef struct D3D10_MAPPED_TEXTURE3D
{
	void *pData;
	UINT RowPitch;
	UINT DepthPitch;
} D3D10_MAPPED_TEXTURE3D;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_BUFFER_SRV
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

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX1D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX1D_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D01.h"
struct D3D10_TEX2D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2D_ARRAY_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX3D_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEXCUBE_SRV
{
	UINT MostDetailedMip;
	UINT MipLevels;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2DMS_SRV
{
	UINT UnusedField_NothingToDefine;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_TEX2DMS_ARRAY_SRV
{
	UINT FirstArraySlice;
	UINT ArraySize;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_SHADER_RESOURCE_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D10_SRV_DIMENSION ViewDimension;
	union
	{
		D3D10_BUFFER_SRV Buffer;
		D3D10_TEX1D_SRV Texture1D;
		D3D10_TEX1D_ARRAY_SRV Texture1DArray;
		D3D10_TEX2D_SRV Texture2D;
		D3D10_TEX2D_ARRAY_SRV Texture2DArray;
		D3D10_TEX2DMS_SRV Texture2DMS;
		D3D10_TEX2DMS_ARRAY_SRV Texture2DMSArray;
		D3D10_TEX3D_SRV Texture3D;
		D3D10_TEXCUBE_SRV TextureCube;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_DEPTH_STENCIL_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D10_DSV_DIMENSION ViewDimension;
	UINT Flags;
	union
	{
		D3D10_TEX1D_DSV Texture1D;
		D3D10_TEX1D_ARRAY_DSV Texture1DArray;
		D3D10_TEX2D_DSV Texture2D;
		D3D10_TEX2D_ARRAY_DSV Texture2DArray;
		D3D10_TEX2DMS_DSV Texture2DMS;
		D3D10_TEX2DMS_ARRAY_DSV Texture2DMSArray;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_RENDER_TARGET_VIEW_DESC
{
	DXGI_FORMAT Format;
	D3D10_RTV_DIMENSION ViewDimension;
	union
	{
		D3D10_BUFFER_RTV Buffer;
		D3D10_TEX1D_RTV Texture1D;
		D3D10_TEX1D_ARRAY_RTV Texture1DArray;
		D3D10_TEX2D_RTV Texture2D;
		D3D10_TEX2D_ARRAY_RTV Texture2DArray;
		D3D10_TEX2DMS_RTV Texture2DMS;
		D3D10_TEX2DMS_ARRAY_RTV Texture2DMSArray;
		D3D10_TEX3D_RTV Texture3D;
	};
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_INPUT_CLASSIFICATION
{
	D3D10_INPUT_PER_VERTEX_DATA		= 0,
	D3D10_INPUT_PER_INSTANCE_DATA	= 1
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_INPUT_ELEMENT_DESC
{
	LPCSTR SemanticName;
	UINT SemanticIndex;
	DXGI_FORMAT Format;
	UINT InputSlot;
	UINT AlignedByteOffset;
	D3D10_INPUT_CLASSIFICATION InputSlotClass;
	UINT InstanceDataStepRate;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef enum D3D10_FILL_MODE
{
	D3D10_FILL_WIREFRAME	= 2,
	D3D10_FILL_SOLID		= 3
} D3D10_FILL_MODE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef enum D3D10_CULL_MODE
{
	D3D10_CULL_NONE		= 1,
	D3D10_CULL_FRONT	= 2,
	D3D10_CULL_BACK		= 3 
} D3D10_CULL_MODE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
typedef struct D3D10_RASTERIZER_DESC
{
	D3D10_FILL_MODE FillMode;
	D3D10_CULL_MODE CullMode;
	BOOL            FrontCounterClockwise;
	INT             DepthBias;
	FLOAT           DepthBiasClamp;
	FLOAT           SlopeScaledDepthBias;
	BOOL            DepthClipEnable;
	BOOL            ScissorEnable;
	BOOL            MultisampleEnable;
	BOOL            AntialiasedLineEnable;
} D3D10_RASTERIZER_DESC;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10Device : public IUnknown
{
	public:
		virtual void STDMETHODCALLTYPE VSSetConstantBuffers(__in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D10Buffer *const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE PSSetShaderResources(__in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D10ShaderResourceView *const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE PSSetShader(__in_opt ID3D10PixelShader *pPixelShader) = 0;
		virtual void STDMETHODCALLTYPE PSSetSamplers(__in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D10SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE VSSetShader(__in_opt ID3D10VertexShader *pVertexShader) = 0;
		virtual void STDMETHODCALLTYPE DrawIndexed(__in UINT IndexCount, __in UINT StartIndexLocation, __in INT BaseVertexLocation) = 0;
		virtual void STDMETHODCALLTYPE Draw(__in UINT VertexCount, __in UINT StartVertexLocation) = 0;
		virtual void STDMETHODCALLTYPE PSSetConstantBuffers(__in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D10Buffer *const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE IASetInputLayout(__in_opt ID3D10InputLayout *pInputLayout) = 0;
		virtual void STDMETHODCALLTYPE IASetVertexBuffers(__in_range(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D10Buffer *const *ppVertexBuffers, __in_ecount(NumBuffers) const UINT *pStrides, __in_ecount(NumBuffers) const UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE IASetIndexBuffer(__in_opt ID3D10Buffer *pIndexBuffer, __in DXGI_FORMAT Format, __in UINT Offset) = 0;
		virtual void STDMETHODCALLTYPE DrawIndexedInstanced(__in UINT IndexCountPerInstance, __in UINT InstanceCount, __in UINT StartIndexLocation, __in INT BaseVertexLocation, __in UINT StartInstanceLocation) = 0;
		virtual void STDMETHODCALLTYPE DrawInstanced(__in UINT VertexCountPerInstance, __in UINT InstanceCount, __in UINT StartVertexLocation, __in UINT StartInstanceLocation) = 0;
		virtual void STDMETHODCALLTYPE GSSetConstantBuffers(__in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __in_ecount(NumBuffers) ID3D10Buffer *const *ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE GSSetShader(__in_opt ID3D10GeometryShader *pShader) = 0;
		virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(__in D3D10_PRIMITIVE_TOPOLOGY Topology) = 0;
		virtual void STDMETHODCALLTYPE VSSetShaderResources(__in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D10ShaderResourceView *const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE VSSetSamplers(__in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D10SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE SetPredication(__in_opt ID3D10Predicate *pPredicate, __in BOOL PredicateValue) = 0;
		virtual void STDMETHODCALLTYPE GSSetShaderResources(__in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __in_ecount(NumViews) ID3D10ShaderResourceView *const *ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE GSSetSamplers(__in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __in_ecount(NumSamplers) ID3D10SamplerState *const *ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE OMSetRenderTargets(__in_range(0, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews, __in_ecount_opt(NumViews) ID3D10RenderTargetView *const *ppRenderTargetViews, __in_opt ID3D10DepthStencilView *pDepthStencilView) = 0;
		virtual void STDMETHODCALLTYPE OMSetBlendState(__in_opt ID3D10BlendState *pBlendState, __in const FLOAT BlendFactor[4], __in UINT SampleMask) = 0;
		virtual void STDMETHODCALLTYPE OMSetDepthStencilState(__in_opt ID3D10DepthStencilState *pDepthStencilState, __in UINT StencilRef) = 0;
		virtual void STDMETHODCALLTYPE SOSetTargets(__in_range(0, D3D10_SO_BUFFER_SLOT_COUNT) UINT NumBuffers, __in_ecount_opt(NumBuffers) ID3D10Buffer *const *ppSOTargets, __in_ecount_opt(NumBuffers) const UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE DrawAuto(void) = 0;
		virtual void STDMETHODCALLTYPE RSSetState(__in_opt ID3D10RasterizerState *pRasterizerState) = 0;
		virtual void STDMETHODCALLTYPE RSSetViewports(__in_range(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumViewports, __in_ecount_opt(NumViewports) const D3D10_VIEWPORT *pViewports) = 0;
		virtual void STDMETHODCALLTYPE RSSetScissorRects(__in_range(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumRects, __in_ecount_opt(NumRects) const D3D10_RECT *pRects) = 0;
		virtual void STDMETHODCALLTYPE CopySubresourceRegion(__in ID3D10Resource *pDstResource, __in UINT DstSubresource, __in UINT DstX, __in UINT DstY, __in UINT DstZ, __in ID3D10Resource *pSrcResource, __in UINT SrcSubresource, __in_opt const D3D10_BOX *pSrcBox) = 0;
		virtual void STDMETHODCALLTYPE CopyResource(__in ID3D10Resource *pDstResource, __in ID3D10Resource *pSrcResource) = 0;
		virtual void STDMETHODCALLTYPE UpdateSubresource(__in ID3D10Resource *pDstResource, __in UINT DstSubresource, __in_opt const D3D10_BOX *pDstBox, __in const void *pSrcData, __in UINT SrcRowPitch, __in UINT SrcDepthPitch) = 0;
		virtual void STDMETHODCALLTYPE ClearRenderTargetView(__in ID3D10RenderTargetView *pRenderTargetView, __in const FLOAT ColorRGBA[4]) = 0;
		virtual void STDMETHODCALLTYPE ClearDepthStencilView(__in ID3D10DepthStencilView *pDepthStencilView, __in UINT ClearFlags, __in FLOAT Depth, __in UINT8 Stencil) = 0;
		virtual void STDMETHODCALLTYPE GenerateMips(__in ID3D10ShaderResourceView *pShaderResourceView) = 0;
		virtual void STDMETHODCALLTYPE ResolveSubresource(__in ID3D10Resource *pDstResource, __in UINT DstSubresource, __in ID3D10Resource *pSrcResource, __in UINT SrcSubresource, __in DXGI_FORMAT Format) = 0;
		virtual void STDMETHODCALLTYPE VSGetConstantBuffers(__in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D10Buffer **ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE PSGetShaderResources(__in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D10ShaderResourceView **ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE PSGetShader(__out ID3D10PixelShader **ppPixelShader) = 0;
		virtual void STDMETHODCALLTYPE PSGetSamplers(__in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D10SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE VSGetShader(__out ID3D10VertexShader **ppVertexShader) = 0;
		virtual void STDMETHODCALLTYPE PSGetConstantBuffers(__in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D10Buffer **ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE IAGetInputLayout(__out ID3D10InputLayout **ppInputLayout) = 0;
		virtual void STDMETHODCALLTYPE IAGetVertexBuffers(__in_range(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount_opt(NumBuffers) ID3D10Buffer **ppVertexBuffers, __out_ecount_opt(NumBuffers) UINT *pStrides, __out_ecount_opt(NumBuffers) UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE IAGetIndexBuffer(__out_opt ID3D10Buffer **pIndexBuffer, __out_opt DXGI_FORMAT *Format, __out_opt UINT *Offset) = 0;
		virtual void STDMETHODCALLTYPE GSGetConstantBuffers(__in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, __out_ecount(NumBuffers) ID3D10Buffer **ppConstantBuffers) = 0;
		virtual void STDMETHODCALLTYPE GSGetShader(__out ID3D10GeometryShader **ppGeometryShader) = 0;
		virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(__out D3D10_PRIMITIVE_TOPOLOGY *pTopology) = 0;
		virtual void STDMETHODCALLTYPE VSGetShaderResources(__in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D10ShaderResourceView **ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE VSGetSamplers(__in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D10SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE GetPredication(__out_opt ID3D10Predicate **ppPredicate, __out_opt BOOL *pPredicateValue) = 0;
		virtual void STDMETHODCALLTYPE GSGetShaderResources(__in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, __out_ecount(NumViews) ID3D10ShaderResourceView **ppShaderResourceViews) = 0;
		virtual void STDMETHODCALLTYPE GSGetSamplers(__in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, __in_range(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, __out_ecount(NumSamplers) ID3D10SamplerState **ppSamplers) = 0;
		virtual void STDMETHODCALLTYPE OMGetRenderTargets(__in_range(0, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews, __out_ecount_opt(NumViews) ID3D10RenderTargetView **ppRenderTargetViews, __out_opt ID3D10DepthStencilView **ppDepthStencilView) = 0;
		virtual void STDMETHODCALLTYPE OMGetBlendState(__out_opt ID3D10BlendState **ppBlendState, __out_opt FLOAT BlendFactor[4], __out_opt UINT *pSampleMask) = 0;
		virtual void STDMETHODCALLTYPE OMGetDepthStencilState(__out_opt ID3D10DepthStencilState **ppDepthStencilState, __out_opt UINT *pStencilRef) = 0;
		virtual void STDMETHODCALLTYPE SOGetTargets(__in_range(0, D3D10_SO_BUFFER_SLOT_COUNT) UINT NumBuffers, __out_ecount_opt(NumBuffers) ID3D10Buffer **ppSOTargets, __out_ecount_opt(NumBuffers) UINT *pOffsets) = 0;
		virtual void STDMETHODCALLTYPE RSGetState(__out ID3D10RasterizerState **ppRasterizerState) = 0;
		virtual void STDMETHODCALLTYPE RSGetViewports(__inout UINT *NumViewports, __out_ecount_opt(*NumViewports) D3D10_VIEWPORT *pViewports) = 0;
		virtual void STDMETHODCALLTYPE RSGetScissorRects(__inout UINT *NumRects, __out_ecount_opt(*NumRects) D3D10_RECT *pRects) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(UINT RaiseFlags) = 0;
		virtual UINT STDMETHODCALLTYPE GetExceptionMode(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(__in REFGUID guid, __inout UINT *pDataSize, __out_bcount_opt(*pDataSize) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(__in REFGUID guid, __in UINT DataSize, __in_bcount_opt(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(__in REFGUID guid, __in_opt const IUnknown *pData) = 0;
		virtual void STDMETHODCALLTYPE ClearState(void) = 0;
		virtual void STDMETHODCALLTYPE Flush(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateBuffer(__in const D3D10_BUFFER_DESC *pDesc, __in_opt const D3D10_SUBRESOURCE_DATA *pInitialData, __out_opt ID3D10Buffer **ppBuffer) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(__in const D3D10_TEXTURE1D_DESC *pDesc, __in_xcount_opt(pDesc->MipLevels * pDesc->ArraySize) const D3D10_SUBRESOURCE_DATA *pInitialData, __out ID3D10Texture1D **ppTexture1D) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(__in const D3D10_TEXTURE2D_DESC *pDesc, __in_xcount_opt(pDesc->MipLevels * pDesc->ArraySize) const D3D10_SUBRESOURCE_DATA *pInitialData, __out ID3D10Texture2D **ppTexture2D) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(__in const D3D10_TEXTURE3D_DESC *pDesc, __in_xcount_opt(pDesc->MipLevels) const D3D10_SUBRESOURCE_DATA *pInitialData, __out ID3D10Texture3D **ppTexture3D) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(__in ID3D10Resource *pResource, __in_opt const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc, __out_opt ID3D10ShaderResourceView **ppSRView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(__in ID3D10Resource *pResource, __in_opt const D3D10_RENDER_TARGET_VIEW_DESC *pDesc, __out_opt ID3D10RenderTargetView **ppRTView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(__in ID3D10Resource *pResource, __in_opt const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc, __out_opt ID3D10DepthStencilView **ppDepthStencilView) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(__in_ecount(NumElements) const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs, __in_range(0, D3D10_1_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT) UINT NumElements, __in const void *pShaderBytecodeWithInputSignature, __in SIZE_T BytecodeLength, __out_opt ID3D10InputLayout **ppInputLayout) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __out_opt ID3D10VertexShader **ppVertexShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __out_opt ID3D10GeometryShader **ppGeometryShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __in_ecount_opt(NumEntries) const D3D10_SO_DECLARATION_ENTRY *pSODeclaration, __in_range(0, D3D10_SO_SINGLE_BUFFER_COMPONENT_LIMIT) UINT NumEntries, __in UINT OutputStreamStride, __out_opt ID3D10GeometryShader **ppGeometryShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(__in const void *pShaderBytecode, __in SIZE_T BytecodeLength, __out_opt ID3D10PixelShader **ppPixelShader) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateBlendState(__in const D3D10_BLEND_DESC *pBlendStateDesc, __out_opt ID3D10BlendState **ppBlendState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(__in const D3D10_DEPTH_STENCIL_DESC *pDepthStencilDesc, __out_opt ID3D10DepthStencilState **ppDepthStencilState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(__in const D3D10_RASTERIZER_DESC *pRasterizerDesc, __out_opt ID3D10RasterizerState **ppRasterizerState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(__in const D3D10_SAMPLER_DESC *pSamplerDesc, __out_opt ID3D10SamplerState **ppSamplerState) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateQuery(__in const D3D10_QUERY_DESC *pQueryDesc, __out_opt ID3D10Query **ppQuery) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreatePredicate(__in const D3D10_QUERY_DESC *pPredicateDesc, __out_opt ID3D10Predicate **ppPredicate) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateCounter(__in const D3D10_COUNTER_DESC *pCounterDesc, __out_opt ID3D10Counter **ppCounter) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(__in DXGI_FORMAT Format, __out UINT *pFormatSupport) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(__in DXGI_FORMAT Format, __in UINT SampleCount, __out UINT *pNumQualityLevels) = 0;
		virtual void STDMETHODCALLTYPE CheckCounterInfo(__out D3D10_COUNTER_INFO *pCounterInfo) = 0;
		virtual HRESULT STDMETHODCALLTYPE CheckCounter(__in const D3D10_COUNTER_DESC *pDesc, __out D3D10_COUNTER_TYPE *pType, __out UINT *pActiveCounters, __out_ecount_opt(*pNameLength) LPSTR szName, __inout_opt UINT *pNameLength, __out_ecount_opt(*pUnitsLength) LPSTR szUnits, __inout_opt UINT *pUnitsLength, __out_ecount_opt(*pDescriptionLength) LPSTR szDescription, __inout_opt UINT *pDescriptionLength) = 0;
		virtual UINT STDMETHODCALLTYPE GetCreationFlags(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(__in HANDLE hResource, __in REFIID ReturnedInterface, __out_opt void **ppResource) = 0;
		virtual void STDMETHODCALLTYPE SetTextFilterSize(__in UINT Width, __in UINT Height) = 0;
		virtual void STDMETHODCALLTYPE GetTextFilterSize(__out_opt UINT *pWidth, __out_opt UINT *pHeight) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10DeviceChild : public IUnknown
{
	public:
		virtual void STDMETHODCALLTYPE GetDevice(__out ID3D10Device **ppDevice) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(__in REFGUID guid, __inout UINT *pDataSize, __out_bcount_opt(*pDataSize) void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(__in REFGUID guid, __in UINT DataSize, __in_bcount_opt(DataSize) const void *pData) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(__in REFGUID guid, __in_opt const IUnknown *pData) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10InputLayout : public ID3D10DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10SamplerState : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_SAMPLER_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10RasterizerState : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_RASTERIZER_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_BLEND
{
	D3D10_BLEND_ZERO				= 1,
	D3D10_BLEND_ONE					= 2,
	D3D10_BLEND_SRC_COLOR			= 3,
	D3D10_BLEND_INV_SRC_COLOR		= 4,
	D3D10_BLEND_SRC_ALPHA			= 5,
	D3D10_BLEND_INV_SRC_ALPHA		= 6,
	D3D10_BLEND_DEST_ALPHA			= 7,
	D3D10_BLEND_INV_DEST_ALPHA		= 8,
	D3D10_BLEND_DEST_COLOR			= 9,
	D3D10_BLEND_INV_DEST_COLOR		= 10,
	D3D10_BLEND_SRC_ALPHA_SAT		= 11,
	D3D10_BLEND_BLEND_FACTOR		= 14,
	D3D10_BLEND_INV_BLEND_FACTOR	= 15,
	D3D10_BLEND_SRC1_COLOR			= 16,
	D3D10_BLEND_INV_SRC1_COLOR		= 17,
	D3D10_BLEND_SRC1_ALPHA			= 18,
	D3D10_BLEND_INV_SRC1_ALPHA		= 19
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_BLEND_OP
{
	D3D10_BLEND_OP_ADD			= 1,
	D3D10_BLEND_OP_SUBTRACT		= 2,
	D3D10_BLEND_OP_REV_SUBTRACT	= 3,
	D3D10_BLEND_OP_MIN			= 4,
	D3D10_BLEND_OP_MAX			= 5
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_BLEND_DESC
{
	BOOL AlphaToCoverageEnable;
	BOOL BlendEnable[8];
	D3D10_BLEND SrcBlend;
	D3D10_BLEND DestBlend;
	D3D10_BLEND_OP BlendOp;
	D3D10_BLEND SrcBlendAlpha;
	D3D10_BLEND DestBlendAlpha;
	D3D10_BLEND_OP BlendOpAlpha;
	UINT8 RenderTargetWriteMask[8];
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10BlendState : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_BLEND_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10DepthStencilState : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_DEPTH_STENCIL_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10VertexShader : public ID3D10DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10GeometryShader : public ID3D10DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10PixelShader : public ID3D10DeviceChild
{
	// Nothing here
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10Resource : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetType(__out D3D10_RESOURCE_DIMENSION *pResourceDimension) = 0;
		virtual void STDMETHODCALLTYPE SetEvictionPriority(__in UINT EvictionPriority) = 0;
		virtual UINT STDMETHODCALLTYPE GetEvictionPriority(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10Buffer : public ID3D10Resource
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Map(__in D3D10_MAP MapType, __in UINT MapFlags, __out void **ppData) = 0;
		virtual void STDMETHODCALLTYPE Unmap(void) = 0;
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_BUFFER_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
MIDL_INTERFACE("9B7E4C03-342C-4106-A19F-4F2704F689F0")
ID3D10Texture1D : public ID3D10Resource
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Map(_In_ UINT Subresource, _In_ D3D10_MAP MapType, _In_ UINT MapFlags, _Out_ void **ppData) = 0;
		virtual void STDMETHODCALLTYPE Unmap(_In_ UINT Subresource) = 0;
		virtual void STDMETHODCALLTYPE GetDesc(_Out_ D3D10_TEXTURE1D_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
MIDL_INTERFACE("9B7E4C04-342C-4106-A19F-4F2704F689F0")
ID3D10Texture2D : public ID3D10Resource
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Map(__in UINT Subresource, __in D3D10_MAP MapType, __in UINT MapFlags, __out D3D10_MAPPED_TEXTURE2D *pMappedTex2D) = 0;
		virtual void STDMETHODCALLTYPE Unmap(__in UINT Subresource) = 0;
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_TEXTURE2D_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
MIDL_INTERFACE("9B7E4C05-342C-4106-A19F-4F2704F689F0")
ID3D10Texture3D : public ID3D10Resource
{
	public:
		virtual HRESULT STDMETHODCALLTYPE Map(_In_ UINT Subresource, _In_ D3D10_MAP MapType, _In_ UINT MapFlags, _Out_ D3D10_MAPPED_TEXTURE3D *pMappedTex3D) = 0;
		virtual void STDMETHODCALLTYPE Unmap(_In_ UINT Subresource) = 0;
		virtual void STDMETHODCALLTYPE GetDesc(_Out_ D3D10_TEXTURE3D_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10View : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE GetResource(__out ID3D10Resource **ppResource) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10ShaderResourceView : public ID3D10View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10RenderTargetView : public ID3D10View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_RENDER_TARGET_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10DepthStencilView : public ID3D10View
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3Dcommon.h"
struct ID3D10Blob : public IUnknown
{
	public:
		virtual LPVOID STDMETHODCALLTYPE GetBufferPointer(void) = 0;
		virtual SIZE_T STDMETHODCALLTYPE GetBufferSize(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10Asynchronous : public ID3D10DeviceChild
{
	public:
		virtual void STDMETHODCALLTYPE Begin(void) = 0;
		virtual void STDMETHODCALLTYPE End(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetData(__out_bcount_opt(DataSize) void *pData, __in UINT DataSize, __in UINT GetDataFlags) = 0;
		virtual UINT STDMETHODCALLTYPE GetDataSize(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
enum D3D10_QUERY
{
	D3D10_QUERY_EVENT					= 0,
	D3D10_QUERY_OCCLUSION				= (D3D10_QUERY_EVENT + 1),
	D3D10_QUERY_TIMESTAMP				= (D3D10_QUERY_OCCLUSION + 1),
	D3D10_QUERY_TIMESTAMP_DISJOINT		= (D3D10_QUERY_TIMESTAMP + 1),
	D3D10_QUERY_PIPELINE_STATISTICS		= (D3D10_QUERY_TIMESTAMP_DISJOINT + 1),
	D3D10_QUERY_OCCLUSION_PREDICATE		= (D3D10_QUERY_PIPELINE_STATISTICS + 1),
	D3D10_QUERY_SO_STATISTICS			= (D3D10_QUERY_OCCLUSION_PREDICATE + 1),
	D3D10_QUERY_SO_OVERFLOW_PREDICATE	= (D3D10_QUERY_SO_STATISTICS + 1)
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct D3D10_QUERY_DESC
{
	D3D10_QUERY Query;
	UINT MiscFlags;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10.h"
struct ID3D10Query : public ID3D10Asynchronous
{
	public:
		virtual void STDMETHODCALLTYPE GetDesc(__out D3D10_QUERY_DESC *pDesc) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
MIDL_INTERFACE("9B7E4E01-342C-4106-A19F-4F2704F689F0")
ID3D10Debug : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetFeatureMask(UINT Mask) = 0;
		virtual UINT STDMETHODCALLTYPE GetFeatureMask(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetPresentPerRenderOpDelay(UINT Milliseconds) = 0;
		virtual UINT STDMETHODCALLTYPE GetPresentPerRenderOpDelay(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetSwapChain(__in_opt IDXGISwapChain *pSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetSwapChain(__out IDXGISwapChain **ppSwapChain) = 0;
		virtual HRESULT STDMETHODCALLTYPE Validate(void) = 0;
};

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
typedef enum D3D10_MESSAGE_CATEGORY
{
	D3D10_MESSAGE_CATEGORY_APPLICATION_DEFINED		= 0,
	D3D10_MESSAGE_CATEGORY_MISCELLANEOUS			= D3D10_MESSAGE_CATEGORY_APPLICATION_DEFINED + 1,
	D3D10_MESSAGE_CATEGORY_INITIALIZATION			= D3D10_MESSAGE_CATEGORY_MISCELLANEOUS + 1,
	D3D10_MESSAGE_CATEGORY_CLEANUP					= D3D10_MESSAGE_CATEGORY_INITIALIZATION + 1,
	D3D10_MESSAGE_CATEGORY_COMPILATION				= D3D10_MESSAGE_CATEGORY_CLEANUP + 1,
	D3D10_MESSAGE_CATEGORY_STATE_CREATION			= D3D10_MESSAGE_CATEGORY_COMPILATION + 1,
	D3D10_MESSAGE_CATEGORY_STATE_SETTING			= D3D10_MESSAGE_CATEGORY_STATE_CREATION + 1,
	D3D10_MESSAGE_CATEGORY_STATE_GETTING			= D3D10_MESSAGE_CATEGORY_STATE_SETTING + 1,
	D3D10_MESSAGE_CATEGORY_RESOURCE_MANIPULATION	= D3D10_MESSAGE_CATEGORY_STATE_GETTING + 1,
	D3D10_MESSAGE_CATEGORY_EXECUTION				= D3D10_MESSAGE_CATEGORY_RESOURCE_MANIPULATION + 1
} D3D10_MESSAGE_CATEGORY;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
typedef enum D3D10_MESSAGE_SEVERITY
{
	D3D10_MESSAGE_SEVERITY_CORRUPTION	= 0,
	D3D10_MESSAGE_SEVERITY_ERROR		= D3D10_MESSAGE_SEVERITY_CORRUPTION + 1,
	D3D10_MESSAGE_SEVERITY_WARNING		= D3D10_MESSAGE_SEVERITY_ERROR + 1,
	D3D10_MESSAGE_SEVERITY_INFO			= D3D10_MESSAGE_SEVERITY_WARNING + 1
} D3D10_MESSAGE_SEVERITY;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h" -> But only the few definitions we need
typedef enum D3D10_MESSAGE_ID
{
	D3D10_MESSAGE_ID_DEVICE_OMSETRENDERTARGETS_HAZARD	= 9,
	D3D10_MESSAGE_ID_DEVICE_VSSETSHADERRESOURCES_HAZARD	= 3,
	D3D10_MESSAGE_ID_DEVICE_GSSETSHADERRESOURCES_HAZARD	= 5,
	D3D10_MESSAGE_ID_DEVICE_PSSETSHADERRESOURCES_HAZARD	= 7
} D3D10_MESSAGE_ID;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
typedef struct D3D10_MESSAGE
{
	D3D10_MESSAGE_CATEGORY Category;
	D3D10_MESSAGE_SEVERITY Severity;
	D3D10_MESSAGE_ID ID;
	const char* pDescription;
	SIZE_T DescriptionByteLength;
} D3D10_MESSAGE;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
typedef struct D3D10_INFO_QUEUE_FILTER_DESC
{
	UINT NumCategories;
	D3D10_MESSAGE_CATEGORY *pCategoryList;
	UINT NumSeverities;
	D3D10_MESSAGE_SEVERITY *pSeverityList;
	UINT NumIDs;
	D3D10_MESSAGE_ID *pIDList;
} D3D10_INFO_QUEUE_FILTER_DESC;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
typedef struct D3D10_INFO_QUEUE_FILTER
{
	D3D10_INFO_QUEUE_FILTER_DESC AllowList;
	D3D10_INFO_QUEUE_FILTER_DESC DenyList;
} D3D10_INFO_QUEUE_FILTER;

// "Microsoft DirectX SDK (June 2010)" -> "D3D10SDKLayers.h"
MIDL_INTERFACE("1b940b17-2642-4d1f-ab1f-b99bad0c395f")
ID3D10InfoQueue : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetMessageCountLimit(__in UINT64 MessageCountLimit) = 0;
		virtual void STDMETHODCALLTYPE ClearStoredMessages(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetMessage(__in UINT64 MessageIndex, __out_bcount_opt(*pMessageByteLength) D3D10_MESSAGE *pMessage, __inout SIZE_T *pMessageByteLength) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesAllowedByStorageFilter(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesDeniedByStorageFilter(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumStoredMessages(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumStoredMessagesAllowedByRetrievalFilter(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesDiscardedByMessageCountLimit(void) = 0;
		virtual UINT64 STDMETHODCALLTYPE GetMessageCountLimit(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddStorageFilterEntries(__in D3D10_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetStorageFilter(__out_bcount_opt(*pFilterByteLength) D3D10_INFO_QUEUE_FILTER *pFilter, __inout SIZE_T *pFilterByteLength) = 0;
		virtual void STDMETHODCALLTYPE ClearStorageFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushEmptyStorageFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushCopyOfStorageFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushStorageFilter(__in D3D10_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual void STDMETHODCALLTYPE PopStorageFilter(void) = 0;
		virtual UINT STDMETHODCALLTYPE GetStorageFilterStackSize(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddRetrievalFilterEntries(__in D3D10_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual HRESULT STDMETHODCALLTYPE GetRetrievalFilter(__out_bcount_opt(*pFilterByteLength) D3D10_INFO_QUEUE_FILTER *pFilter, __inout SIZE_T *pFilterByteLength) = 0;
		virtual void STDMETHODCALLTYPE ClearRetrievalFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushEmptyRetrievalFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushCopyOfRetrievalFilter(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE PushRetrievalFilter(__in D3D10_INFO_QUEUE_FILTER *pFilter) = 0;
		virtual void STDMETHODCALLTYPE PopRetrievalFilter(void) = 0;
		virtual UINT STDMETHODCALLTYPE GetRetrievalFilterStackSize(void) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddMessage(__in D3D10_MESSAGE_CATEGORY Category, __in D3D10_MESSAGE_SEVERITY Severity, __in D3D10_MESSAGE_ID ID, __in LPCSTR pDescription) = 0;
		virtual HRESULT STDMETHODCALLTYPE AddApplicationMessage(__in D3D10_MESSAGE_SEVERITY Severity, __in LPCSTR pDescription) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnCategory(__in D3D10_MESSAGE_CATEGORY Category, __in BOOL bEnable) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnSeverity(__in D3D10_MESSAGE_SEVERITY Severity, __in BOOL bEnable) = 0;
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnID(__in D3D10_MESSAGE_ID ID, __in BOOL bEnable) = 0;
		virtual BOOL STDMETHODCALLTYPE GetBreakOnCategory(__in D3D10_MESSAGE_CATEGORY Category) = 0;
		virtual BOOL STDMETHODCALLTYPE GetBreakOnSeverity(__in D3D10_MESSAGE_SEVERITY Severity) = 0;
		virtual BOOL STDMETHODCALLTYPE GetBreakOnID(__in D3D10_MESSAGE_ID ID) = 0;
		virtual void STDMETHODCALLTYPE SetMuteDebugOutput(__in BOOL bMute) = 0;
		virtual BOOL STDMETHODCALLTYPE GetMuteDebugOutput(void) = 0;
};




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Direct3D10Renderer
{
	class RootSignature;
	class Direct3D9RuntimeLinking;
	class Direct3D10RuntimeLinking;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
#ifdef RENDERER_DEBUG
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given renderer
	*/
	#define DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference) \
		RENDERER_ASSERT(mContext, &rendererReference == &(resourceReference).getRenderer(), "Direct3D 10 error: The given resource is owned by another renderer instance")

	/*
	*  @brief
	*    Debug break on execution failure, replacement for "ID3D10InfoQueue::SetBreakOnSeverity()" which is creating a confusing callstack
	*/
	#define FAILED_DEBUG_BREAK(toExecute) if (FAILED(toExecute)) { DEBUG_BREAK; }
#else
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given renderer
	*/
	#define DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference)

	/*
	*  @brief
	*    Debug break on execution failure, replacement for "ID3D10InfoQueue::SetBreakOnSeverity()" which is creating a confusing callstack
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
					RTL_OSVERSIONINFOW rovi = { 0 };
					rovi.dwOSVersionInfoSize = sizeof(rovi);
					if (0x00000000 == functionPointer(&rovi))
					{
						return rovi;
					}
				}
			}
			RTL_OSVERSIONINFOW rovi = { 0 };
			return rovi;
		}

		// "IsWindows10OrGreater()" isn't practically usable
		// - See "Windows Dev Center" -> "Version Helper functions" -> "IsWindows10OrGreater" at https://msdn.microsoft.com/en-us/library/windows/desktop/dn424972(v=vs.85).aspx
		//   "For Windows 10, IsWindows10OrGreater returns false unless the application contains a manifest that includes a compatibility section that contains the GUID that designates Windows 10."
		[[nodiscard]] bool isWindows10OrGreater()
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
namespace Direct3D10Renderer
{




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Direct3D10Renderer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 renderer class
	*/
	class Direct3D10Renderer final : public Renderer::IRenderer
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class GraphicsPipelineState;


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
		explicit Direct3D10Renderer(const Renderer::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Direct3D10Renderer() override;

		/**
		*  @brief
		*    Return the Direct3D 10 device
		*
		*  @return
		*    The Direct3D 10 device, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Device* getD3D10Device() const
		{
			return mD3D10Device;
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
			return "Direct3D10";
		}

		[[nodiscard]] inline virtual bool isInitialized() const override
		{
			// Is there a Direct3D 10 device?
			return (nullptr != mD3D10Device);
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
			RENDERER_DELETE(mContext, Direct3D10Renderer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D10Renderer(const Direct3D10Renderer& source) = delete;
		Direct3D10Renderer& operator =(const Direct3D10Renderer& source) = delete;

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
		Direct3D10RuntimeLinking*  mDirect3D10RuntimeLinking;	///< Direct3D 10 runtime linking instance, always valid
		ID3D10Device*			   mD3D10Device;				///< The Direct3D 10 device, null pointer on error (we don't check because this would be a total overhead, the user has to use "Renderer::IRenderer::isInitialized()" and is asked to never ever use a not properly initialized renderer)
		Direct3D9RuntimeLinking*   mDirect3D9RuntimeLinking;	///< Direct3D 9 runtime linking instance, can be a null pointer
		Renderer::IShaderLanguage* mShaderLanguageHlsl;			///< HLSL shader language instance (we keep a reference to it), can be a null pointer
		ID3D10Query*			   mD3D10QueryFlush;			///< Direct3D 10 query used for flush, can be a null pointer
		Renderer::IRenderTarget*   mRenderTarget;				///< Currently set render target (we keep a reference to it), can be a null pointer
		RootSignature*			   mGraphicsRootSignature;		///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		// State cache to avoid making redundant Direct3D 10 calls
		D3D10_PRIMITIVE_TOPOLOGY mD3D10PrimitiveTopology;
		ID3D10VertexShader*		 mD3d10VertexShader;
		ID3D10GeometryShader*	 mD3d10GeometryShader;
		ID3D10PixelShader*		 mD3d10PixelShader;


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Direct3D10RuntimeLinking.h         ]
	//[-------------------------------------------------------]
	//[-------------------------------------------------------]
	//[ Macros & definitions                                  ]
	//[-------------------------------------------------------]
	// Redirect D3D10* and D3DX10* function calls to funcPtr_D3D10* and funcPtr_D3DX10*
	#ifndef FNPTR
		#define FNPTR(name) funcPtr_##name
	#endif

	//[-------------------------------------------------------]
	//[ D3D10 core functions                                  ]
	//[-------------------------------------------------------]
	#define FNDEF_D3D10(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_D3D10(HRESULT,	D3D10CreateDevice,	(IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device**));
	#define D3D10CreateDevice	FNPTR(D3D10CreateDevice)
	#define D3D10CreateBlob		FNPTR(D3D10CreateBlob)

	//[-------------------------------------------------------]
	//[ D3DX10 functions                                      ]
	//[-------------------------------------------------------]
	#define FNDEF_D3DX10(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	FNDEF_D3DX10(HRESULT,	D3DX10FilterTexture,	(ID3D10Resource*, UINT, UINT));
	#define D3DX10FilterTexture		FNPTR(D3DX10FilterTexture)

	//[-------------------------------------------------------]
	//[ D3DCompiler functions                                 ]
	//[-------------------------------------------------------]
	#define FNDEF_D3DX10(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
	typedef __interface ID3D10Blob *LPD3D10BLOB;	// "__interface" is no keyword of the ISO C++ standard, shouldn't be a problem because this in here is Microsoft Windows only and it's also within the Direct3D headers we have to use
	typedef ID3D10Blob ID3DBlob;
	FNDEF_D3DX10(HRESULT,	D3DCompile,		(LPCVOID, SIZE_T, LPCSTR, CONST D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**));
	FNDEF_D3DX10(HRESULT,	D3DCreateBlob,	(SIZE_T Size, ID3DBlob** ppBlob));
	#define D3DCompile		FNPTR(D3DCompile)
	#define D3DCreateBlob	FNPTR(D3DCreateBlob)

	/**
	*  @brief
	*    Direct3D 10 runtime linking
	*/
	class Direct3D10RuntimeLinking final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*/
		inline explicit Direct3D10RuntimeLinking(Direct3D10Renderer& direct3D10Renderer) :
			mDirect3D10Renderer(direct3D10Renderer),
			mD3D10SharedLibrary(nullptr),
			mD3DX10SharedLibrary(nullptr),
			mD3DCompilerSharedLibrary(nullptr),
			mEntryPointsRegistered(false),
			mInitialized(false)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		~Direct3D10RuntimeLinking()
		{
			// Destroy the shared library instances
			if (nullptr != mD3D10SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3D10SharedLibrary));
			}
			if (nullptr != mD3DX10SharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3DX10SharedLibrary));
			}
			if (nullptr != mD3DCompilerSharedLibrary)
			{
				::FreeLibrary(static_cast<HMODULE>(mD3DCompilerSharedLibrary));
			}
		}

		/**
		*  @brief
		*    Return whether or not Direct3D 10 is available
		*
		*  @return
		*    "true" if Direct3D 10 is available, else "false"
		*/
		[[nodiscard]] bool isDirect3D10Avaiable()
		{
			// Already initialized?
			if (!mInitialized)
			{
				// We're now initialized
				mInitialized = true;

				// Load the shared libraries
				if (loadSharedLibraries())
				{
					// Load the D3D10, D3DX10 and D3DCompiler entry points
					mEntryPointsRegistered = (loadD3D10EntryPoints() && loadD3DX10EntryPoints() && loadD3DCompilerEntryPoints());
				}
			}

			// Entry points successfully registered?
			return mEntryPointsRegistered;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Direct3D10RuntimeLinking(const Direct3D10RuntimeLinking& source) = delete;
		Direct3D10RuntimeLinking& operator =(const Direct3D10RuntimeLinking& source) = delete;

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
			mD3D10SharedLibrary = ::LoadLibraryExA("d3d10.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (nullptr != mD3D10SharedLibrary)
			{
				mD3DX10SharedLibrary = ::LoadLibraryExA("d3dx10_43.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr != mD3DX10SharedLibrary)
				{
					mD3DCompilerSharedLibrary = ::LoadLibraryExA("D3DCompiler_47.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr == mD3DCompilerSharedLibrary)
					{
						RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to load in the shared Direct3D 10 library \"D3DCompiler_47.dll\"")
					}
				}
				else
				{
					RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to load in the shared Direct3D 10 library \"d3dx10_43.dll\"")
				}
			}
			else
			{
				RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to load in the Direct3D 10 shared library \"d3d10.dll\"")
			}

			// Done
			return (nullptr != mD3D10SharedLibrary && nullptr != mD3DX10SharedLibrary && nullptr != mD3DCompilerSharedLibrary);
		}

		/**
		*  @brief
		*    Load the D3D10 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3D10EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3D10SharedLibrary), #funcName);																							\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3D10SharedLibrary), moduleFilename, MAX_PATH);																					\
						RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 10 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			IMPORT_FUNC(D3D10CreateDevice);

			// Undefine the helper macro
			#undef IMPORT_FUNC

			// Done
			return result;
		}

		/**
		*  @brief
		*    Load the D3DX10 entry points
		*
		*  @return
		*    "true" if all went fine, else "false"
		*/
		[[nodiscard]] bool loadD3DX10EntryPoints()
		{
			bool result = true;	// Success by default

			// Define a helper macro
			#define IMPORT_FUNC(funcName)																																							\
				if (result)																																											\
				{																																													\
					void* symbol = ::GetProcAddress(static_cast<HMODULE>(mD3DX10SharedLibrary), #funcName);																							\
					if (nullptr != symbol)																																							\
					{																																												\
						*(reinterpret_cast<void**>(&(funcName))) = symbol;																															\
					}																																												\
					else																																											\
					{																																												\
						wchar_t moduleFilename[MAX_PATH];																																			\
						moduleFilename[0] = '\0';																																					\
						::GetModuleFileNameW(static_cast<HMODULE>(mD3DX10SharedLibrary), moduleFilename, MAX_PATH);																					\
						RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 10 shared library \"%s\"", #funcName, moduleFilename)	\
						result = false;																																								\
					}																																												\
				}

			// Load the entry points
			IMPORT_FUNC(D3DX10FilterTexture);

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
						RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 10 shared library \"%s\"", #funcName, moduleFilename)	\
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
		Direct3D10Renderer&	mDirect3D10Renderer;		///< Owner Direct3D 10 renderer instance
		void*				mD3D10SharedLibrary;		///< D3D10 shared library, can be a null pointer
		void*				mD3DX10SharedLibrary;		///< D3DX10 shared library, can be a null pointer
		void*				mD3DCompilerSharedLibrary;	///< D3DCompiler shared library, can be a null pointer
		bool				mEntryPointsRegistered;		///< Entry points successfully registered?
		bool				mInitialized;				///< Already initialized?


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Direct3D9RuntimeLinking.h          ]
	//[-------------------------------------------------------]
	// For the Direct3D 9 PIX functions (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box) used for debugging
	#ifdef RENDERER_DEBUG
		//[-------------------------------------------------------]
		//[ D3D9 core functions                                   ]
		//[-------------------------------------------------------]
		#define FNDEF_D3D9(retType, funcName, args) retType (WINAPI *funcPtr_##funcName) args
		// Debug
		FNDEF_D3D9(DWORD,	D3DPERF_GetStatus,	(void));
		FNDEF_D3D9(void,	D3DPERF_SetOptions,	(DWORD));
		FNDEF_D3D9(void,	D3DPERF_SetMarker,	(D3DCOLOR, LPCWSTR));
		FNDEF_D3D9(int,		D3DPERF_BeginEvent,	(D3DCOLOR, LPCWSTR));
		FNDEF_D3D9(int,		D3DPERF_EndEvent,	(void));

		//[-------------------------------------------------------]
		//[ Macros & definitions                                  ]
		//[-------------------------------------------------------]
		#ifndef FNPTR
			#define FNPTR(name) funcPtr_##name
		#endif

		// Redirect D3D9* function calls to funcPtr_D3D9*

		// Debug
		#define D3DPERF_GetStatus	FNPTR(D3DPERF_GetStatus)
		#define D3DPERF_SetOptions	FNPTR(D3DPERF_SetOptions)
		#define D3DPERF_SetMarker	FNPTR(D3DPERF_SetMarker)
		#define D3DPERF_BeginEvent	FNPTR(D3DPERF_BeginEvent)
		#define D3DPERF_EndEvent	FNPTR(D3DPERF_EndEvent)

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
			*  @param[in] direct3D10Renderer
			*    Owner Direct3D 10 renderer instance
			*/
			inline explicit Direct3D9RuntimeLinking(Direct3D10Renderer& direct3D10Renderer) :
				mDirect3D10Renderer(direct3D10Renderer),
				mD3D9SharedLibrary(nullptr),
				mEntryPointsRegistered(false),
				mInitialized(false)
			{}

			/**
			*  @brief
			*    Destructor
			*/
			~Direct3D9RuntimeLinking()
			{
				// Destroy the shared library instance
				if (nullptr != mD3D9SharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mD3D9SharedLibrary));
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

					// Load the shared library
					if (loadSharedLibrary())
					{
						// Load the D3D9 entry points
						mEntryPointsRegistered = loadD3D9EntryPoints();
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
			*    Load the shared library
			*
			*  @return
			*    "true" if all went fine, else "false"
			*/
			[[nodiscard]] bool loadSharedLibrary()
			{
				// Load the shared library
				mD3D9SharedLibrary = ::LoadLibraryExA("d3d9.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (nullptr == mD3D9SharedLibrary)
				{
					RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to load in the Direct3D 9 shared library \"d3d9.dll\"")
				}

				// Done
				return (nullptr != mD3D9SharedLibrary);
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
							RENDERER_LOG(mDirect3D10Renderer.getContext(), CRITICAL, "Failed to locate the entry point \"%s\" within the Direct3D 9 shared library \"%s\"", #funcName, moduleFilename)	\
							result = false;																																								\
						}																																												\
					}

				// Load the entry points
				IMPORT_FUNC(D3DPERF_GetStatus);
				IMPORT_FUNC(D3DPERF_SetOptions);
				IMPORT_FUNC(D3DPERF_SetMarker);
				IMPORT_FUNC(D3DPERF_BeginEvent);
				IMPORT_FUNC(D3DPERF_EndEvent);

				// Undefine the helper macro
				#undef IMPORT_FUNC

				// Done
				return result;
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Direct3D10Renderer&	mDirect3D10Renderer;	///< Owner Direct3D 10 renderer instance
			void*				mD3D9SharedLibrary;		///< D3D9 shared library, can be a null pointer
			bool				mEntryPointsRegistered;	///< Entry points successfully registered?
			bool				mInitialized;			///< Already initialized?


		};
	#endif




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
	[[nodiscard]] ID3DBlob* loadShaderFromSourcecode(const Renderer::Context& context, const char* shaderModel, const char* sourceCode, const char* entryPoint, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel)
	{
		// Sanity checks
		RENDERER_ASSERT(context, nullptr != shaderModel, "Invalid Direct3D 10 shader model")
		RENDERER_ASSERT(context, nullptr != sourceCode, "Invalid Direct3D 10 shader source code")

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

	void handleDeviceLost(const Direct3D10Renderer& direct3D11Renderer, HRESULT result)
	{
		// If the device was removed either by a disconnection or a driver upgrade, we must recreate all device resources
		if (DXGI_ERROR_DEVICE_REMOVED == result || DXGI_ERROR_DEVICE_RESET == result)
		{
			if (DXGI_ERROR_DEVICE_REMOVED == result)
			{
				result = direct3D11Renderer.getD3D10Device()->GetDeviceRemovedReason();
			}
			RENDERER_LOG(direct3D11Renderer.getContext(), CRITICAL, "Direct3D 10 device lost on present: Reason code 0x%08X", static_cast<unsigned int>(result))

			// TODO(co) Add device lost handling if needed. Probably more complex to recreate all device resources.
		}
	}




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Mapping.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 mapping
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
		*    "Renderer::VertexAttributeFormat" to Direct3D 10 format
		*
		*  @param[in] vertexAttributeFormat
		*    "Renderer::VertexAttributeFormat" to map
		*
		*  @return
		*    Direct3D 10 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D10Format(Renderer::VertexAttributeFormat vertexAttributeFormat)
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
		*    "Renderer::BufferUsage" to Direct3D 10 usage and CPU access flags
		*
		*  @param[in]  bufferUsage
		*    "Renderer::BufferUsage" to map
		*  @param[out] cpuAccessFlags
		*    Receives the CPU access flags
		*
		*  @return
		*    Direct3D 10 usage
		*/
		[[nodiscard]] static D3D10_USAGE getDirect3D10UsageAndCPUAccessFlags(Renderer::BufferUsage bufferUsage, uint32_t& cpuAccessFlags)
		{
			// Direct3D 10 only supports a subset of the OpenGL usage indications
			// -> See "D3D10_USAGE enumeration "-documentation at http://msdn.microsoft.com/en-us/library/windows/desktop/bb172499%28v=vs.85%29.aspx
			switch (bufferUsage)
			{
				case Renderer::BufferUsage::STREAM_DRAW:
				case Renderer::BufferUsage::STREAM_COPY:
				case Renderer::BufferUsage::STATIC_DRAW:
				case Renderer::BufferUsage::STATIC_COPY:
					cpuAccessFlags = 0;
					return D3D10_USAGE_IMMUTABLE;

				case Renderer::BufferUsage::STREAM_READ:
				case Renderer::BufferUsage::STATIC_READ:
					cpuAccessFlags = D3D10_CPU_ACCESS_READ;
					return D3D10_USAGE_STAGING;

				case Renderer::BufferUsage::DYNAMIC_DRAW:
				case Renderer::BufferUsage::DYNAMIC_COPY:
					cpuAccessFlags = D3D10_CPU_ACCESS_WRITE;
					return D3D10_USAGE_DYNAMIC;

				default:
				case Renderer::BufferUsage::DYNAMIC_READ:
					cpuAccessFlags = 0;
					return D3D10_USAGE_DEFAULT;
			}
		}

		//[-------------------------------------------------------]
		//[ Renderer::IndexBufferFormat                           ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    "Renderer::IndexBufferFormat" to Direct3D 10 format
		*
		*  @param[in] indexBufferFormat
		*    "Renderer::IndexBufferFormat" to map
		*
		*  @return
		*    Direct3D 10 format
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D10Format(Renderer::IndexBufferFormat::Enum indexBufferFormat)
		{
			static constexpr DXGI_FORMAT MAPPING[] =
			{
				DXGI_FORMAT_R32_UINT,	// Renderer::IndexBufferFormat::UNSIGNED_CHAR  - One byte per element, uint8_t (may not be supported by each API) - Not supported by Direct3D 10
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
		*    "Renderer::TextureFormat" to Direct3D 10 format
		*
		*  @param[in] textureFormat
		*    "Renderer::TextureFormat" to map
		*
		*  @return
		*    Direct3D 10 format
		*
		*  @remarks
		*    For textures used as depth stencil render target, Direct3D 10 format handling becomes a little bit more complex due to
		*    the offered flexibility. For example the abstract texture format "Renderer::TextureFormat::D32_FLOAT" translates into
		*    - Direct3D 10 resource format is "DXGI_FORMAT_R32_TYPELESS"
		*    - Direct3D 10 shader resource view format is "DXGI_FORMAT_R32_FLOAT"
		*    - Direct3D 10 depth stencil view format is "DXGI_FORMAT_D32_FLOAT"
		*/
		[[nodiscard]] static DXGI_FORMAT getDirect3D10Format(Renderer::TextureFormat::Enum textureFormat)
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
				DXGI_FORMAT_UNKNOWN,				// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 10
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

		[[nodiscard]] static DXGI_FORMAT getDirect3D10ResourceFormat(Renderer::TextureFormat::Enum textureFormat)
		{
			// Only "Renderer::TextureFormat::D32_FLOAT" has to be handled in a different way
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
				DXGI_FORMAT_UNKNOWN,				// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 10
				DXGI_FORMAT_R16_UNORM,				// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				DXGI_FORMAT_R32_UINT,				// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				DXGI_FORMAT_R32_FLOAT,				// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				DXGI_FORMAT_R32_TYPELESS,			// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				DXGI_FORMAT_R16G16_SNORM,			// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_R16G16_FLOAT,			// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_UNKNOWN					// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}

		[[nodiscard]] static DXGI_FORMAT getDirect3D10ShaderResourceViewFormat(Renderer::TextureFormat::Enum textureFormat)
		{
			// Only "Renderer::TextureFormat::D32_FLOAT" has to be handled in a different way
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
				DXGI_FORMAT_UNKNOWN,				// Renderer::TextureFormat::ETC1          - 3 component texture compression meant for mobile devices - not supported in Direct3D 10
				DXGI_FORMAT_R16_UNORM,				// Renderer::TextureFormat::R16_UNORM     - 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel
				DXGI_FORMAT_R32_UINT,				// Renderer::TextureFormat::R32_UINT      - 32-bit unsigned integer format
				DXGI_FORMAT_R32_FLOAT,				// Renderer::TextureFormat::R32_FLOAT     - 32-bit float format
				DXGI_FORMAT_R32_FLOAT,				// Renderer::TextureFormat::D32_FLOAT     - 32-bit float depth format
				DXGI_FORMAT_R16G16_SNORM,			// Renderer::TextureFormat::R16G16_SNORM  - A two-component, 32-bit signed-normalized-integer format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_R16G16_FLOAT,			// Renderer::TextureFormat::R16G16_FLOAT  - A two-component, 32-bit floating-point format that supports 16 bits for the red channel and 16 bits for the green channel
				DXGI_FORMAT_UNKNOWN					// Renderer::TextureFormat::UNKNOWN       - Unknown
			};
			return MAPPING[textureFormat];
		}


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/ResourceGroup.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 resource group class
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
				Renderer::IResource* resource = *resources;
				RENDERER_ASSERT(renderer.getContext(), nullptr != resource, "Invalid Direct3D 10 resource")
				mResources[resourceIndex] = resource;
				resource->addReference();
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RENDERER_MALLOC_TYPED(renderer.getContext(), Renderer::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
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
	//[ Direct3D10Renderer/RootSignature.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 root signature ("pipeline layout" in Vulkan terminology) class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(Direct3D10Renderer& direct3D10Renderer, const Renderer::RootSignature& rootSignature) :
			IRootSignature(direct3D10Renderer),
			mRootSignature(rootSignature)
		{
			const Renderer::Context& context = direct3D10Renderer.getContext();

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
			RENDERER_ASSERT(getRenderer().getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The Direct3D 10 root parameter index is out-of-bounds")
			RENDERER_ASSERT(getRenderer().getContext(), numberOfResources > 0, "The number of Direct3D 10 resources must not be zero")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr != resources, "The Direct3D 10 resource pointers must be valid")

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
	//[ Direct3D10Renderer/Buffer/IndexBuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 index buffer object (IBO, "element array buffer" in OpenGL terminology) class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the index buffer, must be valid
		*  @param[in] data
		*    Index buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] indexBufferFormat
		*    Index buffer data format
		*/
		IndexBuffer(Direct3D10Renderer& direct3D10Renderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage, Renderer::IndexBufferFormat::Enum indexBufferFormat) :
			IIndexBuffer(direct3D10Renderer),
			mD3D10Buffer(nullptr),
			mDXGIFormat(DXGI_FORMAT_UNKNOWN)
		{
			// "Renderer::IndexBufferFormat::UNSIGNED_CHAR" is not supported by Direct3D 10
			if (Renderer::IndexBufferFormat::UNSIGNED_CHAR == indexBufferFormat)
			{
				RENDERER_LOG(direct3D10Renderer.getContext(), CRITICAL, "\"Renderer::IndexBufferFormat::UNSIGNED_CHAR\" is not supported by Direct3D 10")
			}
			else
			{
				// Set the DXGI format
				mDXGIFormat = Mapping::getDirect3D10Format(indexBufferFormat);

				// Direct3D 10 buffer description
				D3D10_BUFFER_DESC d3d10BufferDesc;
				d3d10BufferDesc.ByteWidth        = numberOfBytes;
				d3d10BufferDesc.Usage            = Mapping::getDirect3D10UsageAndCPUAccessFlags(bufferUsage, d3d10BufferDesc.CPUAccessFlags);
				d3d10BufferDesc.BindFlags        = D3D10_BIND_INDEX_BUFFER;
				//d3d10BufferDesc.CPUAccessFlags = <filled above>;
				d3d10BufferDesc.MiscFlags        = 0;

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 10 subresource data
					D3D10_SUBRESOURCE_DATA d3d10SubresourceData;
					d3d10SubresourceData.pSysMem          = data;
					d3d10SubresourceData.SysMemPitch      = 0;
					d3d10SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 10 index buffer
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, &d3d10SubresourceData, &mD3D10Buffer));
				}
				else
				{
					// Create the Direct3D 10 index buffer
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, nullptr, &mD3D10Buffer));
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
			if (nullptr != mD3D10Buffer)
			{
				mD3D10Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D index buffer instance
		*
		*  @return
		*    The Direct3D index buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Buffer* getD3D10Buffer() const
		{
			return mD3D10Buffer;
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


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D10Buffer)
				{
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "IBO", 6);	// 6 = "IBO: " including terminating zero!
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
		ID3D10Buffer* mD3D10Buffer;	///< Direct3D index buffer instance, can be a null pointer
		DXGI_FORMAT	  mDXGIFormat;	///< DXGI index buffer data format


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Buffer/VertexBuffer.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the vertex buffer, must be valid
		*  @param[in] data
		*    Vertex buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		VertexBuffer(Direct3D10Renderer& direct3D10Renderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			IVertexBuffer(direct3D10Renderer),
			mD3D10Buffer(nullptr)
		{
			// Direct3D 10 buffer description
			D3D10_BUFFER_DESC d3d10BufferDesc;
			d3d10BufferDesc.ByteWidth        = numberOfBytes;
			d3d10BufferDesc.Usage            = Mapping::getDirect3D10UsageAndCPUAccessFlags(bufferUsage, d3d10BufferDesc.CPUAccessFlags);
			d3d10BufferDesc.BindFlags        = D3D10_BIND_VERTEX_BUFFER;
			//d3d10BufferDesc.CPUAccessFlags = <filled above>;
			d3d10BufferDesc.MiscFlags        = 0;

			// Data given?
			if (nullptr != data)
			{
				// Direct3D 10 subresource data
				D3D10_SUBRESOURCE_DATA d3d10SubresourceData;
				d3d10SubresourceData.pSysMem          = data;
				d3d10SubresourceData.SysMemPitch      = 0;
				d3d10SubresourceData.SysMemSlicePitch = 0;

				// Create the Direct3D 10 vertex buffer
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, &d3d10SubresourceData, &mD3D10Buffer));
			}
			else
			{
				// Create the Direct3D 10 vertex buffer
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, nullptr, &mD3D10Buffer));
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
			if (nullptr != mD3D10Buffer)
			{
				mD3D10Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D vertex buffer instance
		*
		*  @return
		*    The Direct3D vertex buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Buffer* getD3D10Buffer() const
		{
			return mD3D10Buffer;
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
				if (nullptr != mD3D10Buffer)
				{
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "VBO", 6);	// 6 = "VBO: " including terminating zero!
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
		ID3D10Buffer* mD3D10Buffer;	///< Direct3D vertex buffer instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Buffer/VertexArray.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 vertex array class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] vertexAttributes
		*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
		*  @param[in] numberOfVertexBuffers
		*    Number of vertex buffers, having zero vertex buffers is valid
		*  @param[in] vertexBuffers
		*    At least numberOfVertexBuffers instances of vertex array vertex buffers, can be a null pointer in case there are zero vertex buffers, the data is internally copied and you have to free your memory if you no longer need it
		*  @param[in] indexBuffer
		*    Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		*/
		VertexArray(Direct3D10Renderer& direct3D10Renderer, const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, IndexBuffer* indexBuffer) :
			IVertexArray(direct3D10Renderer),
			mD3D10Device(direct3D10Renderer.getD3D10Device()),
			mIndexBuffer(indexBuffer),
			mNumberOfSlots(numberOfVertexBuffers),
			mD3D10Buffers(nullptr),
			mStrides(nullptr),
			mOffsets(nullptr),
			mVertexBuffers(nullptr)
		{
			// Acquire our Direct3D 10 device reference
			mD3D10Device->AddRef();

			// Add a reference to the given index buffer
			if (nullptr != mIndexBuffer)
			{
				mIndexBuffer->addReference();
			}

			// Add a reference to the used vertex buffers
			if (mNumberOfSlots > 0)
			{
				const Renderer::Context& context = direct3D10Renderer.getContext();
				mD3D10Buffers = RENDERER_MALLOC_TYPED(context, ID3D10Buffer*, mNumberOfSlots);
				mStrides = RENDERER_MALLOC_TYPED(context, UINT, mNumberOfSlots);
				mOffsets = RENDERER_MALLOC_TYPED(context, UINT, mNumberOfSlots);
				memset(mOffsets, 0, sizeof(UINT) * mNumberOfSlots);	// Vertex buffer offset is not supported by OpenGL, so our renderer API doesn't support it either, set everything to zero
				mVertexBuffers = RENDERER_MALLOC_TYPED(context, VertexBuffer*, mNumberOfSlots);

				{ // Loop through all vertex buffers
					ID3D10Buffer** currentD3D10Buffer = mD3D10Buffers;
					VertexBuffer** currentVertexBuffer = mVertexBuffers;
					const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + mNumberOfSlots;
					for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer, ++currentD3D10Buffer, ++currentVertexBuffer)
					{
						// TODO(co) Add security check: Is the given resource one of the currently used renderer?
						*currentVertexBuffer = static_cast<VertexBuffer*>(vertexBuffer->vertexBuffer);
						*currentD3D10Buffer = (*currentVertexBuffer)->getD3D10Buffer();
						(*currentVertexBuffer)->addReference();
					}
				}

				{ // Gather slot related data
					const Renderer::VertexAttribute* attribute = vertexAttributes.attributes;
					const Renderer::VertexAttribute* attributesEnd = attribute + vertexAttributes.numberOfAttributes;
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

			// Cleanup Direct3D 10 input slot data
			const Renderer::Context& context = getRenderer().getContext();
			if (mNumberOfSlots > 0)
			{
				RENDERER_FREE(context, mD3D10Buffers);
				RENDERER_FREE(context, mStrides);
				RENDERER_FREE(context, mOffsets);
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

			// Release our Direct3D 10 device reference
			mD3D10Device->Release();
		}

		/**
		*  @brief
		*    Set the Direct3D 10 vertex declaration and stream source
		*/
		void setDirect3DIASetInputLayoutAndStreamSource() const
		{
			// Set the Direct3D 10 vertex buffers
			if (nullptr != mD3D10Buffers)
			{
				// Just make a single API call
				mD3D10Device->IASetVertexBuffers(0, mNumberOfSlots, mD3D10Buffers, mStrides, mOffsets);
			}
			else
			{
				// Direct3D 10 says: "D3D10: INFO: ID3D10Device::IASetVertexBuffers: Since NumBuffers is 0, the operation effectively does nothing. This is probably not intentional, nor is the most efficient way to achieve this operation. Avoid calling the routine at all. [ STATE_SETTING INFO #240: DEVICE_IASETVERTEXBUFFERS_BUFFERS_EMPTY ]"
				// -> Direct3D 10 does not give us this message, but it's probably still no good thing to do
			}

			// Set the used index buffer
			// -> In case of no index buffer we don't set null indices, there's not really a point in it
			if (nullptr != mIndexBuffer)
			{
				// Set the Direct3D 10 indices
				mD3D10Device->IASetIndexBuffer(mIndexBuffer->getD3D10Buffer(), static_cast<DXGI_FORMAT>(mIndexBuffer->getDXGIFormat()), 0);
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
		ID3D10Device*  mD3D10Device;	///< The Direct3D 10 device context instance (we keep a reference to it), null pointer on horrible error (so we don't check)
		IndexBuffer*   mIndexBuffer;	///< Optional index buffer to use, can be a null pointer, the vertex array instance keeps a reference to the index buffer
		// Direct3D 10 input slots
		UINT		   mNumberOfSlots;	///< Number of used Direct3D 10 input slots
		ID3D10Buffer** mD3D10Buffers;	///< Direct3D 10 vertex buffers, if "mD3D10InputLayout" is no null pointer this is no null pointer as well
		UINT*		   mStrides;		///< Strides in bytes, if "mD3D10Buffers" is no null pointer this is no null pointer as well
		UINT*		   mOffsets;		///< Offsets in bytes, if "mD3D10Buffers" is no null pointer this is no null pointer as well
		// For proper vertex buffer reference counter behaviour
		VertexBuffer** mVertexBuffers;	///< Vertex buffers (we keep a reference to it) used by this vertex array, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Buffer/TextureBuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 texture buffer object (TBO) class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the texture buffer, must be valid
		*  @param[in] data
		*    Texture buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferFlags
		*    Buffer flags, see "Renderer::BufferFlag"
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*  @param[in] textureFormat
		*    Texture buffer data format
		*/
		TextureBuffer(Direct3D10Renderer& direct3D10Renderer, uint32_t numberOfBytes, const void* data, uint32_t bufferFlags, Renderer::BufferUsage bufferUsage, Renderer::TextureFormat::Enum textureFormat) :
			ITextureBuffer(direct3D10Renderer),
			mD3D10Buffer(nullptr),
			mD3D10ShaderResourceView(nullptr)
		{
			// Sanity check
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (numberOfBytes % Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The Direct3D 10 texture buffer size must be a multiple of the selected texture format bytes per texel")

			{ // Buffer part
				// Direct3D 10 buffer description
				D3D10_BUFFER_DESC d3d10BufferDesc;
				d3d10BufferDesc.ByteWidth        = numberOfBytes;
				d3d10BufferDesc.Usage            = Mapping::getDirect3D10UsageAndCPUAccessFlags(bufferUsage, d3d10BufferDesc.CPUAccessFlags);
				d3d10BufferDesc.BindFlags        = 0;
				//d3d10BufferDesc.CPUAccessFlags = <filled above>;
				d3d10BufferDesc.MiscFlags        = 0;

				// Set bind flags
				if (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE)
				{
					d3d10BufferDesc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
				}

				// Data given?
				if (nullptr != data)
				{
					// Direct3D 10 subresource data
					D3D10_SUBRESOURCE_DATA d3d10SubresourceData;
					d3d10SubresourceData.pSysMem          = data;
					d3d10SubresourceData.SysMemPitch      = 0;
					d3d10SubresourceData.SysMemSlicePitch = 0;

					// Create the Direct3D 10 constant buffer
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, &d3d10SubresourceData, &mD3D10Buffer));
				}
				else
				{
					// Create the Direct3D 10 constant buffer
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, nullptr, &mD3D10Buffer));
				}
			}

			// Shader resource view part
			if (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format				 = Mapping::getDirect3D10ShaderResourceViewFormat(textureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension		 = D3D10_SRV_DIMENSION_BUFFER;
				d3d10ShaderResourceViewDesc.Buffer.ElementOffset = 0;
				d3d10ShaderResourceViewDesc.Buffer.ElementWidth	 = numberOfBytes / Renderer::TextureFormat::getNumberOfBytesPerElement(textureFormat);

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateShaderResourceView(mD3D10Buffer, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
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
		virtual ~TextureBuffer() override
		{
			// Release the used resources
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10Buffer)
			{
				mD3D10Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D texture buffer instance
		*
		*  @return
		*    The Direct3D texture buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Buffer* getD3D10Buffer() const
		{
			return mD3D10Buffer;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10ShaderResourceView* getD3D10ShaderResourceView() const
		{
			return mD3D10ShaderResourceView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "TBO", 6);	// 6 = "TBO: " including terminating zero!

				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D10Buffer)
				{
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
				}
				if (nullptr != mD3D10ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D10Buffer*			  mD3D10Buffer;				///< Direct3D texture buffer instance, can be a null pointer
		ID3D10ShaderResourceView* mD3D10ShaderResourceView;	///< Direct3D 10 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Buffer/IndirectBuffer.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 indirect buffer object emulation class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the indirect buffer, must be valid
		*  @param[in] data
		*    Indirect buffer data, can be a null pointer (empty buffer)
		*  @param[in] indirectBufferFlags
		*    Indirect buffer flags, see "Renderer::IndirectBufferFlag"
		*/
		IndirectBuffer(Direct3D10Renderer& direct3D10Renderer, uint32_t numberOfBytes, const void* data, [[maybe_unused]] uint32_t indirectBufferFlags) :
			IIndirectBuffer(direct3D10Renderer),
			mNumberOfBytes(numberOfBytes),
			mData(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid Direct3D 10 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), !((indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid Direct3D 10 flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawArguments)) == 0, "Direct3D 10 indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (indirectBufferFlags & Renderer::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Renderer::DrawIndexedArguments)) == 0, "Direct3D 10 indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// Copy data
			if (mNumberOfBytes > 0)
			{
				mData = RENDERER_MALLOC_TYPED(direct3D10Renderer.getContext(), uint8_t, mNumberOfBytes);
				if (nullptr != data)
				{
					memcpy(mData, data, mNumberOfBytes);
				}
			}
			else
			{
				RENDERER_ASSERT(direct3D10Renderer.getContext(), nullptr == data, "Invalid Direct3D 10 indirect buffer data")
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
	//[ Direct3D10Renderer/Buffer/UniformBuffer.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 uniform buffer object (UBO, "constant buffer" in Direct3D terminology) interface
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] numberOfBytes
		*    Number of bytes within the uniform buffer, must be valid
		*  @param[in] data
		*    Uniform buffer data, can be a null pointer (empty buffer)
		*  @param[in] bufferUsage
		*    Indication of the buffer usage
		*/
		UniformBuffer(Direct3D10Renderer& direct3D10Renderer, uint32_t numberOfBytes, const void* data, Renderer::BufferUsage bufferUsage) :
			Renderer::IUniformBuffer(direct3D10Renderer),
			mD3D10Buffer(nullptr)
		{
			{ // Sanity check
				// Check the given number of bytes, if we don't do this we might get told
				//   "... the ByteWidth (value = <x>) must be a multiple of 16 and be less than or equal to 65536"
				// by Direct3D 10
				const uint32_t leftOverBytes = (numberOfBytes % 16);
				if (0 != leftOverBytes)
				{
					// Fix the byte alignment, no assert because other renderer APIs have another alignment (DirectX 12 e.g. 256)
					numberOfBytes += 16 - (numberOfBytes % 16);
				}
			}

			// Direct3D 10 buffer description
			D3D10_BUFFER_DESC d3d10BufferDesc;
			d3d10BufferDesc.ByteWidth		 = numberOfBytes;
			d3d10BufferDesc.Usage			 = Mapping::getDirect3D10UsageAndCPUAccessFlags(bufferUsage, d3d10BufferDesc.CPUAccessFlags);
			d3d10BufferDesc.BindFlags		 = D3D10_BIND_CONSTANT_BUFFER;
			//d3d10BufferDesc.CPUAccessFlags = <filled above>;
			d3d10BufferDesc.MiscFlags		 = 0;

			// Data given?
			if (nullptr != data)
			{
				// Direct3D 10 subresource data
				D3D10_SUBRESOURCE_DATA d3d10SubresourceData;
				d3d10SubresourceData.pSysMem          = data;
				d3d10SubresourceData.SysMemPitch      = 0;
				d3d10SubresourceData.SysMemSlicePitch = 0;

				// Create the Direct3D 10 constant buffer
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, &d3d10SubresourceData, &mD3D10Buffer));
			}
			else
			{
				// Create the Direct3D 10 constant buffer
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBuffer(&d3d10BufferDesc, nullptr, &mD3D10Buffer));
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
		virtual ~UniformBuffer() override
		{
			// Release the Direct3D 10 constant buffer
			if (nullptr != mD3D10Buffer)
			{
				mD3D10Buffer->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 constant buffer instance
		*
		*  @return
		*    The Direct3D 10 constant buffer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Buffer* getD3D10Buffer() const
		{
			return mD3D10Buffer;
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
				if (nullptr != mD3D10Buffer)
				{
					RENDERER_DECORATED_DEBUG_NAME(name, detailedName, "UBO", 6);	// 6 = "UBO: " including terminating zero!
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(detailedName)), detailedName));
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
		ID3D10Buffer* mD3D10Buffer;	///< Direct3D 10 constant buffer instance, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Buffer/BufferManager.h             ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 buffer manager interface
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*/
		inline explicit BufferManager(Direct3D10Renderer& direct3D10Renderer) :
			IBufferManager(direct3D10Renderer)
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
			return RENDERER_NEW(getRenderer().getContext(), VertexBuffer)(static_cast<Direct3D10Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
		}

		[[nodiscard]] inline virtual Renderer::IIndexBuffer* createIndexBuffer(uint32_t numberOfBytes, const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::IndexBufferFormat::Enum indexBufferFormat = Renderer::IndexBufferFormat::UNSIGNED_SHORT) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndexBuffer)(static_cast<Direct3D10Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage, indexBufferFormat);
		}

		[[nodiscard]] inline virtual Renderer::IVertexArray* createVertexArray(const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, Renderer::IIndexBuffer* indexBuffer = nullptr) override
		{
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			return RENDERER_NEW(getRenderer().getContext(), VertexArray)(static_cast<Direct3D10Renderer&>(getRenderer()), vertexAttributes, numberOfVertexBuffers, vertexBuffers, static_cast<IndexBuffer*>(indexBuffer));
		}

		[[nodiscard]] inline virtual Renderer::ITextureBuffer* createTextureBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t bufferFlags = Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW, Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::R32G32B32A32F) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TextureBuffer)(static_cast<Direct3D10Renderer&>(getRenderer()), numberOfBytes, data, bufferFlags, bufferUsage, textureFormat);
		}

		[[nodiscard]] inline virtual Renderer::IStructuredBuffer* createStructuredBuffer(uint32_t, const void*, uint32_t, Renderer::BufferUsage, uint32_t) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 doesn't support structured buffer")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IIndirectBuffer* createIndirectBuffer(uint32_t numberOfBytes, const void* data = nullptr, uint32_t indirectBufferFlags = 0, [[maybe_unused]] Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndirectBuffer)(static_cast<Direct3D10Renderer&>(getRenderer()), numberOfBytes, data, indirectBufferFlags);
		}

		[[nodiscard]] inline virtual Renderer::IUniformBuffer* createUniformBuffer(uint32_t numberOfBytes, const void* data = nullptr, Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::STATIC_DRAW) override
		{
			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// -> "Bind a buffer as a constant buffer to a shader stage; this flag may NOT be combined with any other bind flag." - https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ne-d3d11-d3d11_bind_flag
			// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid Direct3D 10 buffer flags, uniform buffer can't be used for unordered access")
			// RENDERER_ASSERT(getRenderer().getContext(), (bufferFlags & Renderer::BufferFlag::SHADER_RESOURCE) != 0, "Invalid Direct3D 10 buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RENDERER_NEW(getRenderer().getContext(), UniformBuffer)(static_cast<Direct3D10Renderer&>(getRenderer()), numberOfBytes, data, bufferUsage);
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
	//[ Direct3D10Renderer/Texture/Texture1D.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 1D texture class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
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
		Texture1D(Direct3D10Renderer& direct3D10Renderer, uint32_t width, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage) :
			ITexture1D(direct3D10Renderer, width),
			mTextureFormat(textureFormat),
			mD3D10Texture1D(nullptr),
			mD3D10ShaderResourceView(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 10 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(direct3D10Renderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 10 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width) : 1;
			const bool isDepthFormat = Renderer::TextureFormat::isDepth(textureFormat);

			// Direct3D 10 1D texture description
			D3D10_TEXTURE1D_DESC d3d10Texture1DDesc;
			d3d10Texture1DDesc.Width		  = width;
			d3d10Texture1DDesc.MipLevels	  = (generateMipmaps ? 0u : numberOfMipmaps);	// 0 = Let Direct3D 10 allocate the complete mipmap chain for us
			d3d10Texture1DDesc.ArraySize	  = 1;
			d3d10Texture1DDesc.Format		  = Mapping::getDirect3D10ResourceFormat(textureFormat);
			d3d10Texture1DDesc.Usage		  = static_cast<D3D10_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d10Texture1DDesc.BindFlags	  = 0;
			d3d10Texture1DDesc.CPUAccessFlags = (Renderer::TextureUsage::DYNAMIC == textureUsage) ? D3D10_CPU_ACCESS_WRITE : 0u;
			d3d10Texture1DDesc.MiscFlags	  = (generateMipmaps && (textureFlags & Renderer::TextureFlag::RENDER_TARGET) && !isDepthFormat) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Renderer::TextureFlag::SHADER_RESOURCE)
			{
				d3d10Texture1DDesc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
			}
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				if (isDepthFormat)
				{
					d3d10Texture1DDesc.BindFlags |= D3D10_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d10Texture1DDesc.BindFlags |= D3D10_BIND_RENDER_TARGET;
				}
			}

			// Create the Direct3D 10 1D texture instance
			// Did the user provided us with any texture data?
			if (nullptr != data)
			{
				if (generateMipmaps)
				{
					// Let Direct3D 10 generate the mipmaps for us automatically
					// -> Sadly, it's impossible to use initialization data in this use-case
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture1D(&d3d10Texture1DDesc, nullptr, &mD3D10Texture1D));
					if (nullptr != mD3D10Texture1D)
					{
						// Begin debug event
						RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D10Renderer)

						{ // Update Direct3D 10 subresource data of the base-map
							const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, 1);
							direct3D10Renderer.getD3D10Device()->UpdateSubresource(mD3D10Texture1D, 0, nullptr, data, bytesPerRow, bytesPerSlice);
						}

						// Let Direct3D 10 generate the mipmaps for us automatically
						D3DX10FilterTexture(mD3D10Texture1D, 0, D3DX10_DEFAULT);

						// End debug event
						RENDERER_END_DEBUG_EVENT(&direct3D10Renderer)
					}
				}
				else
				{
					// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
					static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
					RENDERER_ASSERT(getRenderer().getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 10 number of mipmaps")
					D3D10_SUBRESOURCE_DATA d3d10SubresourceData[MAXIMUM_NUMBER_OF_MIPMAPS];

					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[mipmap];
							currentD3d10SubresourceData.pSysMem			 = data;
							currentD3d10SubresourceData.SysMemPitch		 = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							currentD3d10SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next mipmap and ensure the size is always at least 1
							data = static_cast<const uint8_t*>(data) + currentD3d10SubresourceData.SysMemPitch;
							width = getHalfSize(width);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						d3d10SubresourceData->pSysMem		   = data;
						d3d10SubresourceData->SysMemPitch	   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						d3d10SubresourceData->SysMemSlicePitch = 0;	// Only relevant for 3D textures
					}
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture1D(&d3d10Texture1DDesc, d3d10SubresourceData, &mD3D10Texture1D));
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture1D(&d3d10Texture1DDesc, nullptr, &mD3D10Texture1D));
			}

			// Create the Direct3D 10 shader resource view instance
			if (nullptr != mD3D10Texture1D)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format					  = Mapping::getDirect3D10ShaderResourceViewFormat(textureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension			  = D3D10_SRV_DIMENSION_TEXTURE1D;
				d3d10ShaderResourceViewDesc.Texture1D.MipLevels		  = numberOfMipmaps;
				d3d10ShaderResourceViewDesc.Texture1D.MostDetailedMip = 0;

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateShaderResourceView(mD3D10Texture1D, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
			}

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
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10Texture1D)
			{
				mD3D10Texture1D->Release();
			}
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
		*    Return the Direct3D texture 1D resource instance
		*
		*  @return
		*    The Direct3D texture 1D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Texture1D* getD3D10Texture1D() const
		{
			return mD3D10Texture1D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 10 resource
		*      view by e.g. assigning another Direct3D 10 resource to it
		*/
		[[nodiscard]] inline ID3D10ShaderResourceView* getD3D10ShaderResourceView() const
		{
			return mD3D10ShaderResourceView;
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
				if (nullptr != mD3D10Texture1D)
				{
					FAILED_DEBUG_BREAK(mD3D10Texture1D->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Texture1D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D10Texture1D;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1D(const Texture1D& source) = delete;
		Texture1D& operator =(const Texture1D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::TextureFormat::Enum mTextureFormat;
		ID3D10Texture1D*			  mD3D10Texture1D;			///< Direct3D 10 texture 1D resource, can be a null pointer
		ID3D10ShaderResourceView*	  mD3D10ShaderResourceView;	///< Direct3D 10 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Texture/Texture2D.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 2D texture class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
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
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 6, 8)
		*/
		Texture2D(Direct3D10Renderer& direct3D10Renderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage, uint8_t numberOfMultisamples) :
			ITexture2D(direct3D10Renderer, width, height),
			mTextureFormat(textureFormat),
			mNumberOfMultisamples(numberOfMultisamples),
			mD3D10Texture2D(nullptr),
			mD3D10ShaderResourceView(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMultisamples == 1 || nullptr == data, "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS), "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMultisamples == 1 || 0 == (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS), "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMultisamples == 1 || 0 != (textureFlags & Renderer::TextureFlag::RENDER_TARGET), "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 10 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(direct3D10Renderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 10 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;
			const bool isDepthFormat = Renderer::TextureFormat::isDepth(textureFormat);

			// Direct3D 10 2D texture description
			D3D10_TEXTURE2D_DESC d3d10Texture2DDesc;
			d3d10Texture2DDesc.Width			  = width;
			d3d10Texture2DDesc.Height			  = height;
			d3d10Texture2DDesc.MipLevels		  = (generateMipmaps ? 0u : numberOfMipmaps);	// 0 = Let Direct3D 10 allocate the complete mipmap chain for us
			d3d10Texture2DDesc.ArraySize		  = 1;
			d3d10Texture2DDesc.Format			  = Mapping::getDirect3D10ResourceFormat(textureFormat);
			d3d10Texture2DDesc.SampleDesc.Count	  = numberOfMultisamples;
			d3d10Texture2DDesc.SampleDesc.Quality = 0;
			d3d10Texture2DDesc.Usage			  = static_cast<D3D10_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d10Texture2DDesc.BindFlags		  = 0;
			d3d10Texture2DDesc.CPUAccessFlags	  = (Renderer::TextureUsage::DYNAMIC == textureUsage) ? D3D10_CPU_ACCESS_WRITE : 0u;
			d3d10Texture2DDesc.MiscFlags		  = (generateMipmaps && (textureFlags & Renderer::TextureFlag::RENDER_TARGET) && !isDepthFormat) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Renderer::TextureFlag::SHADER_RESOURCE)
			{
				d3d10Texture2DDesc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
			}
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				if (isDepthFormat)
				{
					d3d10Texture2DDesc.BindFlags |= D3D10_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d10Texture2DDesc.BindFlags |= D3D10_BIND_RENDER_TARGET;
				}
			}

			// Create the Direct3D 10 2D texture instance
			// Did the user provided us with any texture data?
			if (nullptr != data)
			{
				if (generateMipmaps)
				{
					// Let Direct3D 10 generate the mipmaps for us automatically
					// -> Sadly, it's impossible to use initialization data in this use-case
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &mD3D10Texture2D));
					if (nullptr != mD3D10Texture2D)
					{
						// Begin debug event
						RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D10Renderer)

						{ // Update Direct3D 10 subresource data of the base-map
							const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							direct3D10Renderer.getD3D10Device()->UpdateSubresource(mD3D10Texture2D, 0, nullptr, data, bytesPerRow, bytesPerSlice);
						}

						// Let Direct3D 10 generate the mipmaps for us automatically
						D3DX10FilterTexture(mD3D10Texture2D, 0, D3DX10_DEFAULT);

						// End debug event
						RENDERER_END_DEBUG_EVENT(&direct3D10Renderer)
					}
				}
				else
				{
					// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
					static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
					RENDERER_ASSERT(getRenderer().getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 10 number of mipmaps")
					D3D10_SUBRESOURCE_DATA d3d10SubresourceData[MAXIMUM_NUMBER_OF_MIPMAPS];

					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Upload all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Upload the current mipmap
							D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[mipmap];
							currentD3d10SubresourceData.pSysMem			 = data;
							currentD3d10SubresourceData.SysMemPitch		 = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							currentD3d10SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next mipmap and ensure the size is always at least 1x1
							data = static_cast<const uint8_t*>(data) + Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							width = getHalfSize(width);
							height = getHalfSize(height);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						d3d10SubresourceData->pSysMem		   = data;
						d3d10SubresourceData->SysMemPitch	   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						d3d10SubresourceData->SysMemSlicePitch = 0;	// Only relevant for 3D textures
					}
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture2D(&d3d10Texture2DDesc, d3d10SubresourceData, &mD3D10Texture2D));
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &mD3D10Texture2D));
			}

			// Create the Direct3D 10 shader resource view instance
			if (nullptr != mD3D10Texture2D)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format					  = Mapping::getDirect3D10ShaderResourceViewFormat(textureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension			  = (numberOfMultisamples > 1) ? D3D10_SRV_DIMENSION_TEXTURE2DMS : D3D10_SRV_DIMENSION_TEXTURE2D;
				d3d10ShaderResourceViewDesc.Texture2D.MipLevels		  = numberOfMipmaps;
				d3d10ShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateShaderResourceView(mD3D10Texture2D, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("2D texture");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2D() override
		{
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10Texture2D)
			{
				mD3D10Texture2D->Release();
			}
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
		[[nodiscard]] inline ID3D10Texture2D* getD3D10Texture2D() const
		{
			return mD3D10Texture2D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 10 resource
		*      view by e.g. assigning another Direct3D 10 resource to it
		*/
		[[nodiscard]] inline ID3D10ShaderResourceView* getD3D10ShaderResourceView() const
		{
			return mD3D10ShaderResourceView;
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
			// Re-create the Direct3D 10 shader resource view instance
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10Texture2D)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format					  = Mapping::getDirect3D10ShaderResourceViewFormat(mTextureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension			  = (mNumberOfMultisamples > 1) ? D3D10_SRV_DIMENSION_TEXTURE2DMS : D3D10_SRV_DIMENSION_TEXTURE2D;
				d3d10ShaderResourceViewDesc.Texture2D.MipLevels		  = maximumMipmapIndex - minimumMipmapIndex;
				d3d10ShaderResourceViewDesc.Texture2D.MostDetailedMip = minimumMipmapIndex;

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(static_cast<Direct3D10Renderer&>(getRenderer()).getD3D10Device()->CreateShaderResourceView(mD3D10Texture2D, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
			}
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
				if (nullptr != mD3D10Texture2D)
				{
					FAILED_DEBUG_BREAK(mD3D10Texture2D->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Texture2D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D10Texture2D;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2D(const Texture2D& source) = delete;
		Texture2D& operator =(const Texture2D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::TextureFormat::Enum mTextureFormat;
		uint8_t						  mNumberOfMultisamples;
		ID3D10Texture2D*			  mD3D10Texture2D;			///< Direct3D 10 texture 2D resource, can be a null pointer
		ID3D10ShaderResourceView*	  mD3D10ShaderResourceView;	///< Direct3D 10 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Texture/Texture2DArray.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 2D array texture class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
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
		Texture2DArray(Direct3D10Renderer& direct3D10Renderer, uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) :
			ITexture2DArray(direct3D10Renderer, width, height, numberOfSlices),
			mTextureFormat(textureFormat),
			mNumberOfMultisamples(1),	// TODO(co) Currently no MSAA support for 2D array textures
			mD3D10Texture2D(nullptr),
			mD3D10ShaderResourceView(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 10 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(direct3D10Renderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 10 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;
			const bool isDepthFormat = Renderer::TextureFormat::isDepth(textureFormat);

			// Direct3D 10 2D array texture description
			D3D10_TEXTURE2D_DESC d3d10Texture2DDesc;
			d3d10Texture2DDesc.Width			  = width;
			d3d10Texture2DDesc.Height			  = height;
			d3d10Texture2DDesc.MipLevels		  = (generateMipmaps ? 0u : numberOfMipmaps);	// 0 = Let Direct3D 10 allocate the complete mipmap chain for us
			d3d10Texture2DDesc.ArraySize		  = numberOfSlices;
			d3d10Texture2DDesc.Format			  = Mapping::getDirect3D10ResourceFormat(textureFormat);
			d3d10Texture2DDesc.SampleDesc.Count	  = 1;
			d3d10Texture2DDesc.SampleDesc.Quality = 0;
			d3d10Texture2DDesc.Usage			  = static_cast<D3D10_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d10Texture2DDesc.BindFlags		  = 0;
			d3d10Texture2DDesc.CPUAccessFlags	  = (Renderer::TextureUsage::DYNAMIC == textureUsage) ? D3D10_CPU_ACCESS_WRITE : 0u;
			d3d10Texture2DDesc.MiscFlags		  = (generateMipmaps && (textureFlags & Renderer::TextureFlag::RENDER_TARGET) && !isDepthFormat) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Renderer::TextureFlag::SHADER_RESOURCE)
			{
				d3d10Texture2DDesc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
			}
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				if (isDepthFormat)
				{
					d3d10Texture2DDesc.BindFlags |= D3D10_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d10Texture2DDesc.BindFlags |= D3D10_BIND_RENDER_TARGET;
				}
			}

			// Create the Direct3D 10 2D texture instance
			// Did the user provided us with any texture data?
			ID3D10Device *d3d10Device = direct3D10Renderer.getD3D10Device();
			if (nullptr != data)
			{
				if (generateMipmaps)
				{
					// Let Direct3D 10 generate the mipmaps for us automatically
					// -> Sadly, it's impossible to use initialization data in this use-case
					FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &mD3D10Texture2D));
					if (nullptr != mD3D10Texture2D)
					{
						// Begin debug event
						RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D10Renderer)

						{ // Update Direct3D 10 subresource data of the base-map
							const uint32_t  bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t  bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
							{
								d3d10Device->UpdateSubresource(mD3D10Texture2D, D3D10CalcSubresource(0, arraySlice, numberOfMipmaps), nullptr, data, bytesPerRow, bytesPerSlice);

								// Move on to the next slice
								data = static_cast<const uint8_t*>(data) + bytesPerSlice;
							}
						}

						// Let Direct3D 10 generate the mipmaps for us automatically
						D3DX10FilterTexture(mD3D10Texture2D, 0, D3DX10_DEFAULT);

						// End debug event
						RENDERER_END_DEBUG_EVENT(&direct3D10Renderer)
					}
				}
				else
				{
					// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
					static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
					static constexpr uint32_t MAXIMUM_NUMBER_OF_SLICES = 10;
					RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 10 number of mipmaps")
					D3D10_SUBRESOURCE_DATA d3d10SubresourceDataStack[MAXIMUM_NUMBER_OF_SLICES * MAXIMUM_NUMBER_OF_MIPMAPS];
					const Renderer::Context& context = getRenderer().getContext();
					D3D10_SUBRESOURCE_DATA* d3d10SubresourceData = (numberOfSlices <= MAXIMUM_NUMBER_OF_SLICES) ? d3d10SubresourceDataStack : RENDERER_MALLOC_TYPED(context, D3D10_SUBRESOURCE_DATA, numberOfSlices);

					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Data layout
						// - Direct3D 10 wants: DDS files are organized in slice-major order, like this:
						//     Slice0: Mip0, Mip1, Mip2, etc.
						//     Slice1: Mip0, Mip1, Mip2, etc.
						//     etc.
						// - The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
						//     Mip0: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
						//     Mip1: Slice0, Slice1, Slice2, Slice3, Slice4, Slice5
						//     etc.

						// Upload all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							const uint32_t numberOfBytesPerRow = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t numberOfBytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
							{
								// Upload the current slice
								D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[arraySlice * numberOfMipmaps + mipmap];
								currentD3d10SubresourceData.pSysMem			 = data;
								currentD3d10SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
								currentD3d10SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

								// Move on to the next slice
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
						const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < numberOfSlices; ++arraySlice)
						{
							D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[arraySlice];
							currentD3d10SubresourceData.pSysMem			 = data;
							currentD3d10SubresourceData.SysMemPitch		 = bytesPerRow;
							currentD3d10SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							data = static_cast<const uint8_t*>(data) + bytesPerSlice;
						}
					}
					FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, d3d10SubresourceData, &mD3D10Texture2D));
					if (numberOfSlices > MAXIMUM_NUMBER_OF_SLICES)
					{
						RENDERER_FREE(context, d3d10SubresourceData);
					}
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &mD3D10Texture2D));
			}

			// Create the Direct3D 10 shader resource view instance
			if (nullptr != mD3D10Texture2D)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format							= Mapping::getDirect3D10ShaderResourceViewFormat(textureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension					= D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
				d3d10ShaderResourceViewDesc.Texture2DArray.MostDetailedMip	= 0;
				d3d10ShaderResourceViewDesc.Texture2DArray.MipLevels		= numberOfMipmaps;
				d3d10ShaderResourceViewDesc.Texture2DArray.FirstArraySlice	= 0;
				d3d10ShaderResourceViewDesc.Texture2DArray.ArraySize		= numberOfSlices;

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateShaderResourceView(mD3D10Texture2D, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
			}

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("2D texture array");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Texture2DArray() override
		{
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10Texture2D)
			{
				mD3D10Texture2D->Release();
			}
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
		[[nodiscard]] inline ID3D10Texture2D* getD3D10Texture2D() const
		{
			return mD3D10Texture2D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 10 resource
		*      view by e.g. assigning another Direct3D 10 resource to it
		*/
		[[nodiscard]] inline ID3D10ShaderResourceView* getD3D10ShaderResourceView() const
		{
			return mD3D10ShaderResourceView;
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
				if (nullptr != mD3D10Texture2D)
				{
					FAILED_DEBUG_BREAK(mD3D10Texture2D->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Texture2D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::TextureFormat::Enum mTextureFormat;
		uint8_t						  mNumberOfMultisamples;
		ID3D10Texture2D*			  mD3D10Texture2D;			///< Direct3D 10 texture 2D resource, can be a null pointer
		ID3D10ShaderResourceView*	  mD3D10ShaderResourceView;	///< Direct3D 10 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Texture/Texture3D.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 3D texture class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
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
		Texture3D(Direct3D10Renderer& direct3D10Renderer, uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage) :
			ITexture3D(direct3D10Renderer, width, height, depth),
			mTextureFormat(textureFormat),
			mD3D10Texture3D(nullptr),
			mD3D10ShaderResourceView(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), 0 == (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS) || nullptr != data, "Invalid Direct3D 10 texture parameters")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 10 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(direct3D10Renderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 10 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height, depth) : 1;
			const bool isDepthFormat = Renderer::TextureFormat::isDepth(textureFormat);

			// Direct3D 10 3D texture description
			D3D10_TEXTURE3D_DESC d3d10Texture3DDesc;
			d3d10Texture3DDesc.Width		  = width;
			d3d10Texture3DDesc.Height		  = height;
			d3d10Texture3DDesc.Depth		  = depth;
			d3d10Texture3DDesc.MipLevels	  = (generateMipmaps ? 0u : numberOfMipmaps);	// 0 = Let Direct3D 10 allocate the complete mipmap chain for us
			d3d10Texture3DDesc.Format		  = Mapping::getDirect3D10ResourceFormat(textureFormat);
			d3d10Texture3DDesc.Usage		  = static_cast<D3D10_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d10Texture3DDesc.BindFlags	  = 0;
			d3d10Texture3DDesc.CPUAccessFlags = (Renderer::TextureUsage::DYNAMIC == textureUsage) ? D3D10_CPU_ACCESS_WRITE : 0u;
			d3d10Texture3DDesc.MiscFlags	  = (generateMipmaps && (textureFlags & Renderer::TextureFlag::RENDER_TARGET) && !isDepthFormat) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0u;

			// Set bind flags
			if (textureFlags & Renderer::TextureFlag::SHADER_RESOURCE)
			{
				d3d10Texture3DDesc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
			}
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				if (isDepthFormat)
				{
					d3d10Texture3DDesc.BindFlags |= D3D10_BIND_DEPTH_STENCIL;
				}
				else
				{
					d3d10Texture3DDesc.BindFlags |= D3D10_BIND_RENDER_TARGET;
				}
			}

			// Create the Direct3D 10 3D texture instance
			// Did the user provided us with any texture data?
			if (nullptr != data)
			{
				if (generateMipmaps)
				{
					// Let Direct3D 10 generate the mipmaps for us automatically
					// -> Sadly, it's impossible to use initialization data in this use-case
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture3D(&d3d10Texture3DDesc, nullptr, &mD3D10Texture3D));
					if (nullptr != mD3D10Texture3D)
					{
						// Begin debug event
						RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D10Renderer)

						{ // Update Direct3D 10 subresource data of the base-map
							const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							direct3D10Renderer.getD3D10Device()->UpdateSubresource(mD3D10Texture3D, 0, nullptr, data, bytesPerRow, bytesPerSlice);
						}

						// Let Direct3D 10 generate the mipmaps for us automatically
						D3DX10FilterTexture(mD3D10Texture3D, 0, D3DX10_DEFAULT);

						// End debug event
						RENDERER_END_DEBUG_EVENT(&direct3D10Renderer)
					}
				}
				else
				{
					// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
					static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
					RENDERER_ASSERT(getRenderer().getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 10 number of mipmaps")
					D3D10_SUBRESOURCE_DATA d3d10SubresourceData[MAXIMUM_NUMBER_OF_MIPMAPS];

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
							D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[mipmap];
							currentD3d10SubresourceData.pSysMem			 = data;
							currentD3d10SubresourceData.SysMemPitch		 = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							currentD3d10SubresourceData.SysMemSlicePitch = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);

							// Move on to the next mipmap and ensure the size is always at least 1x1x1
							data = static_cast<const uint8_t*>(data) + currentD3d10SubresourceData.SysMemSlicePitch * depth;
							width = getHalfSize(width);
							height = getHalfSize(height);
							depth = getHalfSize(depth);
						}
					}
					else
					{
						// The user only provided us with the base texture, no mipmaps
						d3d10SubresourceData->pSysMem		   = data;
						d3d10SubresourceData->SysMemPitch	   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						d3d10SubresourceData->SysMemSlicePitch = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
					}
					FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture3D(&d3d10Texture3DDesc, d3d10SubresourceData, &mD3D10Texture3D));
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateTexture3D(&d3d10Texture3DDesc, nullptr, &mD3D10Texture3D));
			}

			// Create the Direct3D 10 shader resource view instance
			if (nullptr != mD3D10Texture3D)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format					  = Mapping::getDirect3D10ShaderResourceViewFormat(textureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension			  = D3D10_SRV_DIMENSION_TEXTURE3D;
				d3d10ShaderResourceViewDesc.Texture3D.MipLevels		  = numberOfMipmaps;
				d3d10ShaderResourceViewDesc.Texture3D.MostDetailedMip = 0;

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateShaderResourceView(mD3D10Texture3D, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
			}

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
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10Texture3D)
			{
				mD3D10Texture3D->Release();
			}
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
		*    Return the Direct3D texture 3D resource instance
		*
		*  @return
		*    The Direct3D texture 3D resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Texture3D* getD3D10Texture3D() const
		{
			return mD3D10Texture3D;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 10 resource
		*      view by e.g. assigning another Direct3D 10 resource to it
		*/
		[[nodiscard]] inline ID3D10ShaderResourceView* getD3D10ShaderResourceView() const
		{
			return mD3D10ShaderResourceView;
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
				if (nullptr != mD3D10Texture3D)
				{
					FAILED_DEBUG_BREAK(mD3D10Texture3D->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10Texture3D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D10Texture3D;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::TextureFormat::Enum mTextureFormat;
		ID3D10Texture3D*			  mD3D10Texture3D;			///< Direct3D 10 texture 3D resource, can be a null pointer
		ID3D10ShaderResourceView*	  mD3D10ShaderResourceView;	///< Direct3D 10 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Texture/TextureCube.h              ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 cube texture class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
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
		TextureCube(Direct3D10Renderer& direct3D10Renderer, uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data, uint32_t textureFlags, Renderer::TextureUsage textureUsage) :
			ITextureCube(direct3D10Renderer, width, height),
			mTextureFormat(textureFormat),
			mD3D10TextureCube(nullptr),
			mD3D10ShaderResourceView(nullptr)
		{
			static constexpr uint32_t NUMBER_OF_SLICES = 6;	// In Direct3D 10, a cube map is a 2D array texture with six slices

			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), (textureFlags & Renderer::TextureFlag::RENDER_TARGET) == 0 || nullptr == data, "Direct3D 10 render target textures can't be filled using provided data")

			// Calculate the number of mipmaps
			const bool dataContainsMipmaps = (textureFlags & Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS);
			const bool generateMipmaps = (!dataContainsMipmaps && (textureFlags & Renderer::TextureFlag::GENERATE_MIPMAPS));
			RENDERER_ASSERT(direct3D10Renderer.getContext(), Renderer::TextureUsage::IMMUTABLE != textureUsage || !generateMipmaps, "Direct3D 10 immutable texture usage can't be combined with automatic mipmap generation")
			const uint32_t numberOfMipmaps = (dataContainsMipmaps || generateMipmaps) ? getNumberOfMipmaps(width, height) : 1;

			// Direct3D 10 2D array texture description
			D3D10_TEXTURE2D_DESC d3d10Texture2DDesc;
			d3d10Texture2DDesc.Width			  = width;
			d3d10Texture2DDesc.Height			  = height;
			d3d10Texture2DDesc.MipLevels		  = (generateMipmaps ? 0u : numberOfMipmaps);	// 0 = Let Direct3D 10 allocate the complete mipmap chain for us
			d3d10Texture2DDesc.ArraySize		  = NUMBER_OF_SLICES;
			d3d10Texture2DDesc.Format			  = Mapping::getDirect3D10ResourceFormat(textureFormat);
			d3d10Texture2DDesc.SampleDesc.Count	  = 1;
			d3d10Texture2DDesc.SampleDesc.Quality = 0;
			d3d10Texture2DDesc.Usage			  = static_cast<D3D10_USAGE>(textureUsage);	// These constants directly map to Direct3D constants, do not change them
			d3d10Texture2DDesc.BindFlags		  = 0;
			d3d10Texture2DDesc.CPUAccessFlags	  = (Renderer::TextureUsage::DYNAMIC == textureUsage) ? D3D10_CPU_ACCESS_WRITE : 0u;
			d3d10Texture2DDesc.MiscFlags		  = ((generateMipmaps && (textureFlags & Renderer::TextureFlag::RENDER_TARGET)) ? D3D10_RESOURCE_MISC_GENERATE_MIPS : 0u) | D3D10_RESOURCE_MISC_TEXTURECUBE;

			// Set bind flags
			if (textureFlags & Renderer::TextureFlag::SHADER_RESOURCE)
			{
				d3d10Texture2DDesc.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
			}
			if (textureFlags & Renderer::TextureFlag::RENDER_TARGET)
			{
				d3d10Texture2DDesc.BindFlags |= D3D10_BIND_RENDER_TARGET;
			}

			// Create the Direct3D 10 2D texture instance
			// Did the user provided us with any texture data?
			ID3D10Device *d3d10Device = direct3D10Renderer.getD3D10Device();
			if (nullptr != data)
			{
				if (generateMipmaps)
				{
					// Let Direct3D 10 generate the mipmaps for us automatically
					// -> Sadly, it's impossible to use initialization data in this use-case
					FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &mD3D10TextureCube));
					if (nullptr != mD3D10TextureCube)
					{
						// Begin debug event
						RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(&direct3D10Renderer)

						{ // Update Direct3D 10 subresource data of the base-map
							const uint32_t  bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t  bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							for (uint32_t arraySlice = 0; arraySlice < NUMBER_OF_SLICES; ++arraySlice)
							{
								d3d10Device->UpdateSubresource(mD3D10TextureCube, D3D10CalcSubresource(0, arraySlice, numberOfMipmaps), nullptr, data, bytesPerRow, bytesPerSlice);

								// Move on to the next slice
								data = static_cast<const uint8_t*>(data) + bytesPerSlice;
							}
						}

						// Let Direct3D 10 generate the mipmaps for us automatically
						D3DX10FilterTexture(mD3D10TextureCube, 0, D3DX10_DEFAULT);

						// End debug event
						RENDERER_END_DEBUG_EVENT(&direct3D10Renderer)
					}
				}
				else
				{
					// We don't want dynamic allocations, so we limit the maximum number of mipmaps and hence are able to use the efficient C runtime stack
					static constexpr uint32_t MAXIMUM_NUMBER_OF_MIPMAPS = 15;	// A 16384x16384 texture has 15 mipmaps
					RENDERER_ASSERT(direct3D10Renderer.getContext(), numberOfMipmaps <= MAXIMUM_NUMBER_OF_MIPMAPS, "Invalid Direct3D 10 number of mipmaps")
					D3D10_SUBRESOURCE_DATA d3d10SubresourceData[NUMBER_OF_SLICES * MAXIMUM_NUMBER_OF_MIPMAPS];

					// Did the user provided data containing mipmaps from 0-n down to 1x1 linearly in memory?
					if (dataContainsMipmaps)
					{
						// Data layout
						// - Direct3D 10 wants: DDS files are organized in face-major order, like this:
						//     Face0: Mip0, Mip1, Mip2, etc.
						//     Face1: Mip0, Mip1, Mip2, etc.
						//     etc.
						// - The renderer interface provides: CRN and KTX files are organized in mip-major order, like this:
						//     Mip0: Face0, Face1, Face2, Face3, Face4, Face5
						//     Mip1: Face0, Face1, Face2, Face3, Face4, Face5
						//     etc.

						// Upload all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							const uint32_t numberOfBytesPerRow = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
							const uint32_t numberOfBytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
							for (uint32_t arraySlice = 0; arraySlice < NUMBER_OF_SLICES; ++arraySlice)
							{
								// Upload the current mipmap
								D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[arraySlice * numberOfMipmaps + mipmap];
								currentD3d10SubresourceData.pSysMem			 = data;
								currentD3d10SubresourceData.SysMemPitch		 = numberOfBytesPerRow;
								currentD3d10SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

								// Move on to the cube map face
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
						const uint32_t bytesPerRow   = Renderer::TextureFormat::getNumberOfBytesPerRow(textureFormat, width);
						const uint32_t bytesPerSlice = Renderer::TextureFormat::getNumberOfBytesPerSlice(textureFormat, width, height);
						for (uint32_t arraySlice = 0; arraySlice < NUMBER_OF_SLICES; ++arraySlice)
						{
							D3D10_SUBRESOURCE_DATA& currentD3d10SubresourceData = d3d10SubresourceData[arraySlice];
							currentD3d10SubresourceData.pSysMem			 = data;
							currentD3d10SubresourceData.SysMemPitch		 = bytesPerRow;
							currentD3d10SubresourceData.SysMemSlicePitch = 0;	// Only relevant for 3D textures

							// Move on to the next slice
							data = static_cast<const uint8_t*>(data) + bytesPerSlice;
						}
					}
					FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, d3d10SubresourceData, &mD3D10TextureCube));
				}
			}
			else
			{
				// The user did not provide us with texture data
				FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &mD3D10TextureCube));
			}

			// Create the Direct3D 10 shader resource view instance
			if (nullptr != mD3D10TextureCube)
			{
				// Direct3D 10 shader resource view description
				D3D10_SHADER_RESOURCE_VIEW_DESC d3d10ShaderResourceViewDesc = {};
				d3d10ShaderResourceViewDesc.Format						= Mapping::getDirect3D10ShaderResourceViewFormat(textureFormat);
				d3d10ShaderResourceViewDesc.ViewDimension				= D3D10_SRV_DIMENSION_TEXTURECUBE;
				d3d10ShaderResourceViewDesc.TextureCube.MipLevels		= numberOfMipmaps;
				d3d10ShaderResourceViewDesc.TextureCube.MostDetailedMip	= 0;

				// Create the Direct3D 10 shader resource view instance
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateShaderResourceView(mD3D10TextureCube, &d3d10ShaderResourceViewDesc, &mD3D10ShaderResourceView));
			}

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
			if (nullptr != mD3D10ShaderResourceView)
			{
				mD3D10ShaderResourceView->Release();
			}
			if (nullptr != mD3D10TextureCube)
			{
				mD3D10TextureCube->Release();
			}
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
		*    Return the Direct3D texture cube resource instance
		*
		*  @return
		*    The Direct3D texture cube resource instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10Texture2D* getD3D10TextureCube() const
		{
			return mD3D10TextureCube;
		}

		/**
		*  @brief
		*    Return the Direct3D shader resource view instance
		*
		*  @return
		*    The Direct3D shader resource view instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's not recommended to manipulate the returned Direct3D 10 resource
		*      view by e.g. assigning another Direct3D 10 resource to it
		*/
		[[nodiscard]] inline ID3D10ShaderResourceView* getD3D10ShaderResourceView() const
		{
			return mD3D10ShaderResourceView;
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
				if (nullptr != mD3D10TextureCube)
				{
					FAILED_DEBUG_BREAK(mD3D10TextureCube->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10TextureCube->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10ShaderResourceView)
				{
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10ShaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif

		[[nodiscard]] inline virtual void* getInternalResourceHandle() const override
		{
			return mD3D10TextureCube;
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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCube(const TextureCube& source) = delete;
		TextureCube& operator =(const TextureCube& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::TextureFormat::Enum mTextureFormat;
		ID3D10Texture2D*			  mD3D10TextureCube;		///< Direct3D 10 texture cube resource, can be a null pointer
		ID3D10ShaderResourceView*	  mD3D10ShaderResourceView;	///< Direct3D 10 shader resource view, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Texture/TextureManager.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 texture manager interface
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*/
		inline explicit TextureManager(Direct3D10Renderer& direct3D10Renderer) :
			ITextureManager(direct3D10Renderer)
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
			// Check whether or not the given texture dimension is valid
			if (width > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture1D)(static_cast<Direct3D10Renderer&>(getRenderer()), width, textureFormat, data, textureFlags, textureUsage);
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITexture2D* createTexture2D(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT, uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture2D)(static_cast<Direct3D10Renderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, textureUsage, numberOfMultisamples);
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0 && numberOfSlices > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture2DArray)(static_cast<Direct3D10Renderer&>(getRenderer()), width, height, numberOfSlices, textureFormat, data, textureFlags, textureUsage);
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0 && depth > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture3D)(static_cast<Direct3D10Renderer&>(getRenderer()), width, height, depth, textureFormat, data, textureFlags, textureUsage);
			}
			else
			{
				return nullptr;
			}
		}

		[[nodiscard]] virtual Renderer::ITextureCube* createTextureCube(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, const void* data = nullptr, uint32_t textureFlags = 0, Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), TextureCube)(static_cast<Direct3D10Renderer&>(getRenderer()), width, height, textureFormat, data, textureFlags, textureUsage);
			}
			else
			{
				return nullptr;
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


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/State/SamplerState.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 sampler state class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] samplerState
		*    Sampler state to use
		*/
		SamplerState(Direct3D10Renderer& direct3D10Renderer, const Renderer::SamplerState& samplerState) :
			ISamplerState(direct3D10Renderer),
			mD3D10SamplerState(nullptr)
		{
			// Sanity checks
			RENDERER_ASSERT(direct3D10Renderer.getContext(), samplerState.filter != Renderer::FilterMode::UNKNOWN, "Direct3D 10 filter mode must not be unknown")
			RENDERER_ASSERT(direct3D10Renderer.getContext(), samplerState.maxAnisotropy <= direct3D10Renderer.getCapabilities().maximumAnisotropy, "Direct3D 10 maximum anisotropy value violated")

			// Create the Direct3D 10 sampler state
			// -> "Renderer::SamplerState" maps directly to Direct3D 10 & 11, do not change it
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateSamplerState(reinterpret_cast<const D3D10_SAMPLER_DESC*>(&samplerState), &mD3D10SamplerState));

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Sampler state");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~SamplerState() override
		{
			// Release the Direct3D 10 sampler state
			if (nullptr != mD3D10SamplerState)
			{
				mD3D10SamplerState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 sampler state
		*
		*  @return
		*    The Direct3D 10 sampler state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10SamplerState* getD3D10SamplerState() const
		{
			return mD3D10SamplerState;
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
				if (nullptr != mD3D10SamplerState)
				{
					FAILED_DEBUG_BREAK(mD3D10SamplerState->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10SamplerState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
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
		ID3D10SamplerState* mD3D10SamplerState;	///< Direct3D 10 sampler state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/State/IState.h                     ]
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
	//[ Direct3D10Renderer/State/RasterizerState.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 rasterizer state class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] rasterizerState
		*    Rasterizer state to use
		*/
		RasterizerState(Direct3D10Renderer& direct3D10Renderer, const Renderer::RasterizerState& rasterizerState) :
			mD3D10RasterizerState(nullptr)
		{
			// Create the Direct3D 10 rasterizer state
			// -> Thank's to Direct3D 12, "Renderer::RasterizerState" doesn't map directly to Direct3D 10 & 11 - but at least the constants directly still map
			D3D10_RASTERIZER_DESC d3d10RasterizerDesc;
			d3d10RasterizerDesc.FillMode				= static_cast<D3D10_FILL_MODE>(rasterizerState.fillMode);
			d3d10RasterizerDesc.CullMode				= static_cast<D3D10_CULL_MODE>(rasterizerState.cullMode);
			d3d10RasterizerDesc.FrontCounterClockwise	= rasterizerState.frontCounterClockwise;
			d3d10RasterizerDesc.DepthBias				= rasterizerState.depthBias;
			d3d10RasterizerDesc.DepthBiasClamp			= rasterizerState.depthBiasClamp;
			d3d10RasterizerDesc.SlopeScaledDepthBias	= rasterizerState.slopeScaledDepthBias;
			d3d10RasterizerDesc.DepthClipEnable			= rasterizerState.depthClipEnable;
			d3d10RasterizerDesc.ScissorEnable			= rasterizerState.scissorEnable;
			d3d10RasterizerDesc.MultisampleEnable		= rasterizerState.multisampleEnable;
			d3d10RasterizerDesc.AntialiasedLineEnable	= rasterizerState.antialiasedLineEnable;
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateRasterizerState(&d3d10RasterizerDesc, &mD3D10RasterizerState));

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Rasterizer state");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~RasterizerState()
		{
			// Release the Direct3D 10 rasterizer state
			if (nullptr != mD3D10RasterizerState)
			{
				mD3D10RasterizerState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 rasterizer state
		*
		*  @return
		*    The Direct3D 10 rasterizer state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10RasterizerState* getD3D10RasterizerState() const
		{
			return mD3D10RasterizerState;
		}

		#ifdef RENDERER_DEBUG
			/**
			*  @brief
			*    Assign a name to the state for debugging purposes
			*
			*  @param[in] name
			*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
			*
			*  @see
			*    - "Renderer::IRenderer::isDebugEnabled()"
			*/
			void setDebugName(const char* name)
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D10RasterizerState)
				{
					FAILED_DEBUG_BREAK(mD3D10RasterizerState->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10RasterizerState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D10RasterizerState* mD3D10RasterizerState;	///< Direct3D 10 rasterizer state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/State/DepthStencilState.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 depth stencil state class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] depthStencilState
		*    Depth stencil state to use
		*/
		DepthStencilState(Direct3D10Renderer& direct3D10Renderer, const Renderer::DepthStencilState& depthStencilState) :
			mD3D10DepthStencilState(nullptr)
		{
			// Create the Direct3D 10 depth stencil state
			// -> "Renderer::DepthStencilState" maps directly to Direct3D 10 & 11, do not change it
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateDepthStencilState(reinterpret_cast<const D3D10_DEPTH_STENCIL_DESC*>(&depthStencilState), &mD3D10DepthStencilState));

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Depth stencil state");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~DepthStencilState()
		{
			// Release the Direct3D 10 depth stencil state
			if (nullptr != mD3D10DepthStencilState)
			{
				mD3D10DepthStencilState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 depth stencil state
		*
		*  @return
		*    The Direct3D 10 depth stencil state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10DepthStencilState* getD3D10DepthStencilState() const
		{
			return mD3D10DepthStencilState;
		}

		#ifdef RENDERER_DEBUG
			/**
			*  @brief
			*    Assign a name to the state for debugging purposes
			*
			*  @param[in] name
			*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
			*
			*  @see
			*    - "Renderer::IRenderer::isDebugEnabled()"
			*/
			void setDebugName(const char* name)
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D10DepthStencilState)
				{
					FAILED_DEBUG_BREAK(mD3D10DepthStencilState->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10DepthStencilState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D10DepthStencilState* mD3D10DepthStencilState;	///< Direct3D 10 depth stencil state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/State/BlendState.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 blend state class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] blendState
		*    Blend state to use
		*/
		BlendState(Direct3D10Renderer& direct3D10Renderer, const Renderer::BlendState& blendState) :
			mD3D10BlendState(nullptr)
		{
			// Create the Direct3D 10 blend state
			const D3D10_BLEND_DESC d3d10BlendDesc =
			{
				blendState.alphaToCoverageEnable,										// AlphaToCoverageEnable (BOOL)
				blendState.renderTarget[0].blendEnable,									// BlendEnable[0] (BOOL)
				blendState.renderTarget[1].blendEnable,									// BlendEnable[1] (BOOL)
				blendState.renderTarget[2].blendEnable,									// BlendEnable[2] (BOOL)
				blendState.renderTarget[3].blendEnable,									// BlendEnable[3] (BOOL)
				blendState.renderTarget[4].blendEnable,									// BlendEnable[4] (BOOL)
				blendState.renderTarget[5].blendEnable,									// BlendEnable[5] (BOOL)
				blendState.renderTarget[6].blendEnable,									// BlendEnable[6] (BOOL)
				blendState.renderTarget[7].blendEnable,									// BlendEnable[7] (BOOL)
				static_cast<D3D10_BLEND>(blendState.renderTarget[0].srcBlend),			// SrcBlend (D3D10_BLEND)
				static_cast<D3D10_BLEND>(blendState.renderTarget[0].destBlend),			// DestBlend (D3D10_BLEND)
				static_cast<D3D10_BLEND_OP>(blendState.renderTarget[0].blendOp),		// BlendOp (D3D10_BLEND_OP)
				static_cast<D3D10_BLEND>(blendState.renderTarget[0].srcBlendAlpha),		// SrcBlendAlpha (D3D10_BLEND)
				static_cast<D3D10_BLEND>(blendState.renderTarget[0].destBlendAlpha),	// DestBlendAlpha (D3D10_BLEND)
				static_cast<D3D10_BLEND_OP>(blendState.renderTarget[0].blendOpAlpha),	// BlendOpAlpha (D3D10_BLEND_OP)
				blendState.renderTarget[0].renderTargetWriteMask,						// RenderTargetWriteMask[0] (UINT8)
				blendState.renderTarget[1].renderTargetWriteMask,						// RenderTargetWriteMask[1] (UINT8)
				blendState.renderTarget[2].renderTargetWriteMask,						// RenderTargetWriteMask[2] (UINT8)
				blendState.renderTarget[3].renderTargetWriteMask,						// RenderTargetWriteMask[3] (UINT8)
				blendState.renderTarget[4].renderTargetWriteMask,						// RenderTargetWriteMask[4] (UINT8)
				blendState.renderTarget[5].renderTargetWriteMask,						// RenderTargetWriteMask[5] (UINT8)
				blendState.renderTarget[6].renderTargetWriteMask,						// RenderTargetWriteMask[6] (UINT8)
				blendState.renderTarget[7].renderTargetWriteMask						// RenderTargetWriteMask[7] (UINT8)
			};
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateBlendState(&d3d10BlendDesc, &mD3D10BlendState));

			// Assign a default name to the resource for debugging purposes
			#ifdef RENDERER_DEBUG
				setDebugName("Blend state");
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~BlendState()
		{
			// Release the Direct3D 10 blend state
			if (nullptr != mD3D10BlendState)
			{
				mD3D10BlendState->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 blend state
		*
		*  @return
		*    The Direct3D 10 blend state, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10BlendState* getD3D10BlendState() const
		{
			return mD3D10BlendState;
		}

		#ifdef RENDERER_DEBUG
			/**
			*  @brief
			*    Assign a name to the state for debugging purposes
			*
			*  @param[in] name
			*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
			*
			*  @see
			*    - "Renderer::IRenderer::isDebugEnabled()"
			*/
			void setDebugName(const char* name)
			{
				// Set the debug name
				// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
				if (nullptr != mD3D10BlendState)
				{
					FAILED_DEBUG_BREAK(mD3D10BlendState->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10BlendState->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
			}
		#endif


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ID3D10BlendState* mD3D10BlendState;	///< Direct3D 10 blend state, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/RenderTarget/RenderPass.h          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 render pass interface
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
			RENDERER_ASSERT(renderer.getContext(), mNumberOfColorAttachments < 8, "Invalid number of Direct3D 10 color attachments")
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
			RENDERER_ASSERT(getRenderer().getContext(), colorAttachmentIndex < mNumberOfColorAttachments, "Invalid Direct3D 10 color attachment index")
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
	//[ Direct3D10Renderer/RenderTarget/SwapChain.h           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 swap chain class
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
			mDxgiSwapChain(nullptr),
			mD3D10RenderTargetView(nullptr),
			mD3D10DepthStencilView(nullptr),
			mSynchronizationInterval(0),
			mAllowTearing(false)
		{
			const RenderPass& d3d10RenderPass = static_cast<RenderPass&>(renderPass);

			// Sanity check
			RENDERER_ASSERT(renderPass.getRenderer().getContext(), 1 == d3d10RenderPass.getNumberOfColorAttachments(), "There must be exactly one Direct3D 10 render pass color attachment")

			// Get the Direct3D 10 device instance
			ID3D10Device* d3d10Device = static_cast<Direct3D10Renderer&>(renderPass.getRenderer()).getD3D10Device();

			// Get the native window handle
			const HWND hWnd = reinterpret_cast<HWND>(windowHandle.nativeWindowHandle);

			// Get a DXGI factory instance
			const bool isWindows10OrGreater = ::detail::isWindows10OrGreater();
			IDXGIFactory* dxgiFactory = nullptr;
			{
				IDXGIDevice* dxgiDevice = nullptr;
				IDXGIAdapter* dxgiAdapter = nullptr;
				FAILED_DEBUG_BREAK(d3d10Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
				FAILED_DEBUG_BREAK(dxgiDevice->GetAdapter(&dxgiAdapter));
				FAILED_DEBUG_BREAK(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

				// Determines whether tearing support is available for fullscreen borderless windows
				// -> To unlock frame rates of UWP applications on the Windows Store and providing support for both AMD Freesync and NVIDIA's G-SYNC we must explicitly allow tearing
				// -> See "Windows Dev Center" -> "Variable refresh rate displays": https://msdn.microsoft.com/en-us/library/windows/desktop/mt742104(v=vs.85).aspx
				if (isWindows10OrGreater)
				{
					IDXGIFactory5* dxgiFactory5 = nullptr;
					FAILED_DEBUG_BREAK(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory5)));
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

			// Create the swap chain
			DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc = {};
			dxgiSwapChainDesc.BufferCount						 = 1;
			dxgiSwapChainDesc.BufferDesc.Width					 = static_cast<UINT>(width);
			dxgiSwapChainDesc.BufferDesc.Height					 = static_cast<UINT>(height);
			dxgiSwapChainDesc.BufferDesc.Format					 = Mapping::getDirect3D10Format(d3d10RenderPass.getColorAttachmentTextureFormat(0));
			dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator	 = 60;
			dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
			dxgiSwapChainDesc.BufferUsage						 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			dxgiSwapChainDesc.OutputWindow						 = hWnd;
			dxgiSwapChainDesc.SampleDesc.Count					 = 1;
			dxgiSwapChainDesc.SampleDesc.Quality				 = 0;
			dxgiSwapChainDesc.Windowed							 = TRUE;
			if (isWindows10OrGreater)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), d3d10RenderPass.getNumberOfMultisamples() == 1, "Direct3D 10 doesn't support multisampling if the flip model vertical synchronization is used")
				dxgiSwapChainDesc.BufferCount = 2;
				dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			}
			else if (::IsWindows8OrGreater())
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), d3d10RenderPass.getNumberOfMultisamples() == 1, "Direct3D 10 doesn't support multisampling if the flip model vertical synchronization is used")
				dxgiSwapChainDesc.BufferCount = 2;
				dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			}
			dxgiSwapChainDesc.Flags = (mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u);
			FAILED_DEBUG_BREAK(dxgiFactory->CreateSwapChain(d3d10Device, &dxgiSwapChainDesc, &mDxgiSwapChain));

			// Disable alt-return for automatic fullscreen state change
			// -> We handle this manually to have more control over it
			FAILED_DEBUG_BREAK(dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

			// Release our DXGI factory
			dxgiFactory->Release();

			// Create the Direct3D 10 views
			if (nullptr != mDxgiSwapChain)
			{
				createDirect3D10Views();
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
			if (nullptr != mD3D10DepthStencilView)
			{
				mD3D10DepthStencilView->Release();
			}
			if (nullptr != mD3D10RenderTargetView)
			{
				mD3D10RenderTargetView->Release();
			}
			if (nullptr != mDxgiSwapChain)
			{
				mDxgiSwapChain->Release();
			}

			// After releasing references to these resources, we need to call "Flush()" to ensure that Direct3D also releases any references it might still have to the same resources - such as pipeline bindings
			static_cast<Direct3D10Renderer&>(getRenderer()).getD3D10Device()->Flush();
		}

		/**
		*  @brief
		*    Return the DXGI swap chain instance
		*
		*  @return
		*    The DXGI swap chain instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline IDXGISwapChain* getDXGISwapChain() const
		{
			return mDxgiSwapChain;
		}

		/**
		*  @brief
		*    Return the Direct3D 10 render target view instance
		*
		*  @return
		*    The Direct3D 10 render target view instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D10RenderTargetView* getD3D10RenderTargetView() const
		{
			return mD3D10RenderTargetView;
		}

		/**
		*  @brief
		*    Return the Direct3D 10 depth stencil view instance
		*
		*  @return
		*    The Direct3D 10 depth stencil view instance, null pointer on error, do not release the returned instance unless you added an own reference to it
		*
		*  @note
		*    - It's highly recommended to not keep any references to the returned instance, else issues may occur when resizing the swap chain
		*/
		[[nodiscard]] inline ID3D10DepthStencilView* getD3D10DepthStencilView() const
		{
			return mD3D10DepthStencilView;
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
				if (nullptr != mDxgiSwapChain)
				{
					FAILED_DEBUG_BREAK(mDxgiSwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mDxgiSwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10RenderTargetView)
				{
					FAILED_DEBUG_BREAK(mD3D10RenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10RenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
				if (nullptr != mD3D10DepthStencilView)
				{
					FAILED_DEBUG_BREAK(mD3D10DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
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
			if (nullptr != mDxgiSwapChain)
			{
				// Get the Direct3D 10 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain->GetDesc(&dxgiSwapChainDesc));

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
			if (nullptr != mDxgiSwapChain)
			{
				// Get the Direct3D 10 swap chain description
				DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
				FAILED_DEBUG_BREAK(mDxgiSwapChain->GetDesc(&dxgiSwapChainDesc));

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
			if (nullptr != mDxgiSwapChain)
			{
				// TODO(co) "!getFullscreenState()": Add support for borderless window to get rid of this
				const Direct3D10Renderer& direct3D10Renderer = static_cast<Direct3D10Renderer&>(getRenderPass().getRenderer());
				const UINT flags = ((mAllowTearing && 0 == mSynchronizationInterval && !getFullscreenState()) ? DXGI_PRESENT_ALLOW_TEARING : 0);
				handleDeviceLost(direct3D10Renderer, mDxgiSwapChain->Present(mSynchronizationInterval, flags));
			}
		}

		virtual void resizeBuffers() override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				Direct3D10Renderer& direct3D10Renderer = static_cast<Direct3D10Renderer&>(getRenderer());

				// Get the currently set render target
				Renderer::IRenderTarget* renderTargetBackup = direct3D10Renderer.omGetRenderTarget();

				// In case this swap chain is the current render target, we have to unset it before continuing
				if (this == renderTargetBackup)
				{
					direct3D10Renderer.setGraphicsRenderTarget(nullptr);
				}
				else
				{
					renderTargetBackup = nullptr;
				}

				// Release the views
				if (nullptr != mD3D10DepthStencilView)
				{
					mD3D10DepthStencilView->Release();
					mD3D10DepthStencilView = nullptr;
				}
				if (nullptr != mD3D10RenderTargetView)
				{
					mD3D10RenderTargetView->Release();
					mD3D10RenderTargetView = nullptr;
				}

				// Get the swap chain width and height, ensures they are never ever zero
				UINT width  = 1;
				UINT height = 1;
				getSafeWidthAndHeight(width, height);

				// Resize the Direct3D 10 swap chain
				// -> Preserve the existing buffer count and format
				// -> Automatically choose the width and height to match the client rectangle of the native window
				const HRESULT result = mDxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u);
				if (SUCCEEDED(result))
				{
					// Create the Direct3D 10 views
					createDirect3D10Views();

					// If required, restore the previously set render target
					if (nullptr != renderTargetBackup)
					{
						direct3D10Renderer.setGraphicsRenderTarget(renderTargetBackup);
					}
				}
				else
				{
					handleDeviceLost(direct3D10Renderer, result);
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
				FAILED_DEBUG_BREAK(mDxgiSwapChain->GetFullscreenState(&fullscreen, nullptr));
			}

			// Done
			return (FALSE != fullscreen);
		}

		inline virtual void setFullscreenState(bool fullscreen) override
		{
			// Is there a valid swap chain?
			if (nullptr != mDxgiSwapChain)
			{
				FAILED_DEBUG_BREAK(mDxgiSwapChain->SetFullscreenState(fullscreen, nullptr));
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
		*      "DXGI Error: The buffer height inferred from the output window is zero. Taking 8 as a reasonable default instead"
		*      "D3D10: ERROR: ID3D10Device::CreateTexture2D: The Dimensions are invalid. The Width (value = 1005) must be between
		*       1 and 8192, inclusively. The Height (value = 0) must be between 1 and 8192, inclusively. And, the ArraySize (value = 1)
		*       must be between 1 and 512, inclusively. [ STATE_CREATION ERROR #101: CREATETEXTURE2D_INVALIDDIMENSIONS ]"
		*   including an evil memory leak. So, best to use this method which gets the width and height of the native output
		*   window manually and ensures it's never zero.
		*
		*  @note
		*    - "mDXGISwapChain" must be valid when calling this method
		*/
		void getSafeWidthAndHeight(uint32_t& width, uint32_t& height) const
		{
			// Get the Direct3D 10 swap chain description
			DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
			FAILED_DEBUG_BREAK(mDxgiSwapChain->GetDesc(&dxgiSwapChainDesc));

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
		*    Create the Direct3D 10 views
		*/
		void createDirect3D10Views()
		{
			// Create a render target view
			ID3D10Texture2D* d3d10Texture2DBackBuffer = nullptr;
			FAILED_DEBUG_BREAK(mDxgiSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<LPVOID*>(&d3d10Texture2DBackBuffer)));

			// Get the Direct3D 10 device instance
			ID3D10Device* d3d10Device = static_cast<Direct3D10Renderer&>(getRenderer()).getD3D10Device();

			// Create a render target view
			FAILED_DEBUG_BREAK(d3d10Device->CreateRenderTargetView(d3d10Texture2DBackBuffer, nullptr, &mD3D10RenderTargetView));
			d3d10Texture2DBackBuffer->Release();

			// Create depth stencil texture
			const Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat = static_cast<RenderPass&>(getRenderPass()).getDepthStencilAttachmentTextureFormat();
			if (Renderer::TextureFormat::Enum::UNKNOWN != depthStencilAttachmentTextureFormat)
			{
				// Get the swap chain width and height, ensures they are never ever zero
				UINT width  = 1;
				UINT height = 1;
				getSafeWidthAndHeight(width, height);

				// Create depth stencil texture
				ID3D10Texture2D* d3d10Texture2DDepthStencil = nullptr;
				D3D10_TEXTURE2D_DESC d3d10Texture2DDesc = {};
				d3d10Texture2DDesc.Width			  = width;
				d3d10Texture2DDesc.Height			  = height;
				d3d10Texture2DDesc.MipLevels		  = 1;
				d3d10Texture2DDesc.ArraySize		  = 1;
				d3d10Texture2DDesc.Format			  = Mapping::getDirect3D10Format(depthStencilAttachmentTextureFormat);
				d3d10Texture2DDesc.SampleDesc.Count   = 1;
				d3d10Texture2DDesc.SampleDesc.Quality = 0;
				d3d10Texture2DDesc.Usage			  = D3D10_USAGE_DEFAULT;
				d3d10Texture2DDesc.BindFlags		  = D3D10_BIND_DEPTH_STENCIL;
				d3d10Texture2DDesc.CPUAccessFlags	  = 0;
				d3d10Texture2DDesc.MiscFlags		  = 0;
				FAILED_DEBUG_BREAK(d3d10Device->CreateTexture2D(&d3d10Texture2DDesc, nullptr, &d3d10Texture2DDepthStencil));

				// Create the depth stencil view
				D3D10_DEPTH_STENCIL_VIEW_DESC d3d10DepthStencilViewDesc = {};
				d3d10DepthStencilViewDesc.Format			 = d3d10Texture2DDesc.Format;
				d3d10DepthStencilViewDesc.ViewDimension		 = D3D10_DSV_DIMENSION_TEXTURE2D;
				d3d10DepthStencilViewDesc.Texture2D.MipSlice = 0;
				FAILED_DEBUG_BREAK(d3d10Device->CreateDepthStencilView(d3d10Texture2DDepthStencil, &d3d10DepthStencilViewDesc, &mD3D10DepthStencilView));
				d3d10Texture2DDepthStencil->Release();
			}
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IDXGISwapChain*			mDxgiSwapChain;			///< The DXGI swap chain instance, null pointer on error
		ID3D10RenderTargetView*	mD3D10RenderTargetView;	///< The Direct3D 10 render target view instance, null pointer on error
		ID3D10DepthStencilView*	mD3D10DepthStencilView;	///< The Direct3D 10 depth stencil view instance, null pointer on error
		uint32_t				mSynchronizationInterval;
		bool					mAllowTearing;


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/RenderTarget/Framebuffer.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 framebuffer class
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
			mD3D10RenderTargetViews(nullptr),
			mD3D10DepthStencilView(nullptr)
		{
			// The Direct3D 10 "ID3D10Device::OMSetRenderTargets method"-documentation at MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/bb173597%28v=vs.85%29.aspx
			// says the following about the framebuffer width and height when using multiple render targets
			//   "All render targets must have the same size in all dimensions (width and height, and depth for 3D or array size for *Array types)"
			// So, in here I use the smallest width and height as the size of the framebuffer and let Direct3D 10 handle the rest regarding errors.

			// Add a reference to the used color textures
			Direct3D10Renderer& direct3D10Renderer = static_cast<Direct3D10Renderer&>(renderPass.getRenderer());
			if (mNumberOfColorTextures > 0)
			{
				const Renderer::Context& context = direct3D10Renderer.getContext();
				mColorTextures = RENDERER_MALLOC_TYPED(context, Renderer::ITexture*, mNumberOfColorTextures);
				mD3D10RenderTargetViews = RENDERER_MALLOC_TYPED(context, ID3D10RenderTargetView*, mNumberOfColorTextures);

				// Loop through all color textures
				ID3D10RenderTargetView** d3d10RenderTargetView = mD3D10RenderTargetViews;
				Renderer::ITexture** colorTexturesEnd = mColorTextures + mNumberOfColorTextures;
				for (Renderer::ITexture** colorTexture = mColorTextures; colorTexture < colorTexturesEnd; ++colorTexture, ++colorFramebufferAttachments, ++d3d10RenderTargetView)
				{
					// Sanity check
					RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != colorFramebufferAttachments->texture, "Invalid Direct3D 10 color framebuffer attachment texture")

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
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), colorFramebufferAttachments->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 10 color framebuffer attachment mipmap index")
							RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == colorFramebufferAttachments->layerIndex, "Invalid Direct3D 10 color framebuffer attachment layer index")

							// Update the framebuffer width and height if required
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

							// Create the Direct3D 10 render target view instance
							D3D10_RENDER_TARGET_VIEW_DESC d3d10RenderTargetViewDesc = {};
							d3d10RenderTargetViewDesc.Format			 = Mapping::getDirect3D10Format(texture2D->getTextureFormat());
							d3d10RenderTargetViewDesc.ViewDimension		 = (texture2D->getNumberOfMultisamples() > 1) ? D3D10_RTV_DIMENSION_TEXTURE2DMS : D3D10_RTV_DIMENSION_TEXTURE2D;
							d3d10RenderTargetViewDesc.Texture2D.MipSlice = colorFramebufferAttachments->mipmapIndex;
							FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateRenderTargetView(texture2D->getD3D10Texture2D(), &d3d10RenderTargetViewDesc, d3d10RenderTargetView));
							break;
						}

						case Renderer::ResourceType::TEXTURE_2D_ARRAY:
						{
							// Update the framebuffer width and height if required
							const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(*colorTexture);
							::detail::updateWidthHeight(colorFramebufferAttachments->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);

							// Create the Direct3D 10 render target view instance
							D3D10_RENDER_TARGET_VIEW_DESC d3d10RenderTargetViewDesc = {};
							d3d10RenderTargetViewDesc.Format			 = Mapping::getDirect3D10Format(texture2DArray->getTextureFormat());
							d3d10RenderTargetViewDesc.ViewDimension		 = (texture2DArray->getNumberOfMultisamples() > 1) ? D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D10_RTV_DIMENSION_TEXTURE2DARRAY;
							d3d10RenderTargetViewDesc.Texture2DArray.MipSlice		 = colorFramebufferAttachments->mipmapIndex;
							d3d10RenderTargetViewDesc.Texture2DArray.FirstArraySlice = colorFramebufferAttachments->layerIndex;
							d3d10RenderTargetViewDesc.Texture2DArray.ArraySize		 = 1;
							FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateRenderTargetView(texture2DArray->getD3D10Texture2D(), &d3d10RenderTargetViewDesc, d3d10RenderTargetView));
							break;
						}

						case Renderer::ResourceType::ROOT_SIGNATURE:
						case Renderer::ResourceType::RESOURCE_GROUP:
						case Renderer::ResourceType::GRAPHICS_PROGRAM:
						case Renderer::ResourceType::VERTEX_ARRAY:
						case Renderer::ResourceType::RENDER_PASS:
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
							RENDERER_LOG(direct3D10Renderer.getContext(), CRITICAL, "The type of the given color texture at index %d is not supported by the Direct3D 10 renderer backend", colorTexture - mColorTextures)
							*d3d10RenderTargetView = nullptr;
							break;
					}
				}
			}

			// Add a reference to the used depth stencil texture
			if (nullptr != depthStencilFramebufferAttachment)
			{
				mDepthStencilTexture = depthStencilFramebufferAttachment->texture;
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), nullptr != mDepthStencilTexture, "Invalid Direct3D 10 depth stencil framebuffer attachment texture")
				mDepthStencilTexture->addReference();

				// Evaluate the depth stencil texture type
				switch (mDepthStencilTexture->getResourceType())
				{
					case Renderer::ResourceType::TEXTURE_2D:
					{
						const Texture2D* texture2D = static_cast<Texture2D*>(mDepthStencilTexture);

						// Sanity checks
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), depthStencilFramebufferAttachment->mipmapIndex < Texture2D::getNumberOfMipmaps(texture2D->getWidth(), texture2D->getHeight()), "Invalid Direct3D 10 depth stencil framebuffer attachment mipmap index")
						RENDERER_ASSERT(renderPass.getRenderer().getContext(), 0 == depthStencilFramebufferAttachment->layerIndex, "Invalid Direct3D 10 depth stencil framebuffer attachment layer index")

						// Update the framebuffer width and height if required
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2D->getWidth(), texture2D->getHeight(), mWidth, mHeight);

						// Create the Direct3D 10 render target view instance
						D3D10_DEPTH_STENCIL_VIEW_DESC d3d10DepthStencilViewDesc = {};
						d3d10DepthStencilViewDesc.Format			 = Mapping::getDirect3D10Format(texture2D->getTextureFormat());
						d3d10DepthStencilViewDesc.ViewDimension		 = (texture2D->getNumberOfMultisamples() > 1) ? D3D10_DSV_DIMENSION_TEXTURE2DMS : D3D10_DSV_DIMENSION_TEXTURE2D;
						d3d10DepthStencilViewDesc.Texture2D.MipSlice = depthStencilFramebufferAttachment->mipmapIndex;
						FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateDepthStencilView(texture2D->getD3D10Texture2D(), &d3d10DepthStencilViewDesc, &mD3D10DepthStencilView));
						break;
					}

					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					{
						// Update the framebuffer width and height if required
						const Texture2DArray* texture2DArray = static_cast<Texture2DArray*>(mDepthStencilTexture);
						::detail::updateWidthHeight(depthStencilFramebufferAttachment->mipmapIndex, texture2DArray->getWidth(), texture2DArray->getHeight(), mWidth, mHeight);

						// Create the Direct3D 10 render target view instance
						D3D10_DEPTH_STENCIL_VIEW_DESC d3d10DepthStencilViewDesc = {};
						d3d10DepthStencilViewDesc.Format			 = Mapping::getDirect3D10Format(texture2DArray->getTextureFormat());
						d3d10DepthStencilViewDesc.ViewDimension		 = (texture2DArray->getNumberOfMultisamples() > 1) ? D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D10_DSV_DIMENSION_TEXTURE2DARRAY;
						d3d10DepthStencilViewDesc.Texture2DArray.MipSlice		 = depthStencilFramebufferAttachment->mipmapIndex;
						d3d10DepthStencilViewDesc.Texture2DArray.FirstArraySlice = depthStencilFramebufferAttachment->layerIndex;
						d3d10DepthStencilViewDesc.Texture2DArray.ArraySize		 = 1;
						FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateDepthStencilView(texture2DArray->getD3D10Texture2D(), &d3d10DepthStencilViewDesc, &mD3D10DepthStencilView));
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
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
						RENDERER_LOG(direct3D10Renderer.getContext(), CRITICAL, "The type of the given depth stencil texture is not supported by the Direct3D 10 renderer backend")
						break;
				}
			}

			// Validate the framebuffer width and height
			if (0 == mWidth || UINT_MAX == mWidth)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Direct3D 10 framebuffer width")
				mWidth = 1;
			}
			if (0 == mHeight || UINT_MAX == mHeight)
			{
				RENDERER_ASSERT(renderPass.getRenderer().getContext(), false, "Invalid Direct3D 10 framebuffer height")
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
			if (nullptr != mD3D10RenderTargetViews)
			{
				// Release references
				ID3D10RenderTargetView** d3d10RenderTargetViewsEnd = mD3D10RenderTargetViews + mNumberOfColorTextures;
				for (ID3D10RenderTargetView** d3d10RenderTargetView = mD3D10RenderTargetViews; d3d10RenderTargetView < d3d10RenderTargetViewsEnd; ++d3d10RenderTargetView)
				{
					(*d3d10RenderTargetView)->Release();
				}

				// Cleanup
				RENDERER_FREE(context, mD3D10RenderTargetViews);
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
			if (nullptr != mD3D10DepthStencilView)
			{
				// Release reference
				mD3D10DepthStencilView->Release();
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
		[[nodiscard]] inline Renderer::ITexture** getColorTextures() const
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
		[[nodiscard]] inline Renderer::ITexture* getDepthStencilTexture() const
		{
			return mDepthStencilTexture;
		}

		/**
		*  @brief
		*    Return the Direct3D 10 render target views
		*
		*  @return
		*    The Direct3D 10 render target views, can be a null pointer, do not release the returned instances unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10RenderTargetView** getD3D10RenderTargetViews() const
		{
			return mD3D10RenderTargetViews;
		}

		/**
		*  @brief
		*    Return the Direct3D 10 depth stencil view
		*
		*  @return
		*    The Direct3D 10 depth stencil view, can be a null pointer, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10DepthStencilView* getD3D10DepthStencilView() const
		{
			return mD3D10DepthStencilView;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResource methods            ]
	//[-------------------------------------------------------]
	public:
		#ifdef RENDERER_DEBUG
			virtual void setDebugName(const char* name) override
			{
				{ // Assign a debug name to the Direct3D 10 render target view, do also add the index to the name
					const size_t nameLength = strlen(name) + 5;	// Direct3D 10 supports 8 render targets ("D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT", so: One digit + one [ + one ] + one space + terminating zero = 5 characters)
					const Renderer::Context& context = getRenderer().getContext();
					char* nameWithIndex = RENDERER_MALLOC_TYPED(context, char, nameLength);
					ID3D10RenderTargetView** d3d10RenderTargetViewsEnd = mD3D10RenderTargetViews + mNumberOfColorTextures;
					for (ID3D10RenderTargetView** d3d10RenderTargetView = mD3D10RenderTargetViews; d3d10RenderTargetView < d3d10RenderTargetViewsEnd; ++d3d10RenderTargetView)
					{
						// Set the debug name
						// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
						sprintf_s(nameWithIndex, nameLength, "%s [%d]", name, static_cast<uint32_t>(d3d10RenderTargetView - mD3D10RenderTargetViews));
						FAILED_DEBUG_BREAK((*d3d10RenderTargetView)->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
						FAILED_DEBUG_BREAK((*d3d10RenderTargetView)->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(nameLength), nameWithIndex));
					}
					RENDERER_FREE(context, nameWithIndex);
				}

				// Assign a debug name to the Direct3D 10 depth stencil view
				if (nullptr != mD3D10DepthStencilView)
				{
					// Set the debug name
					// -> First: Ensure that there's no previous private data, else we might get slapped with a warning
					FAILED_DEBUG_BREAK(mD3D10DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
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
		Renderer::ITexture** mColorTextures;			///< The color render target textures (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" textures in the provided C-array of pointers
		Renderer::ITexture*  mDepthStencilTexture;		///< The depth stencil render target texture (we keep a reference to it), can be a null pointer
		uint32_t			 mWidth;					///< The framebuffer width
		uint32_t			 mHeight;					///< The framebuffer height
		// Direct3D 10 part
		ID3D10RenderTargetView** mD3D10RenderTargetViews;	///< The Direct3D 10 render target views (we keep a reference to it), can be a null pointer or can contain null pointers, if not a null pointer there must be at least "mNumberOfColorTextures" views in the provided C-array of pointers
		ID3D10DepthStencilView*  mD3D10DepthStencilView;	///< The Direct3D 10 depth stencil view (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Shader/VertexShaderHlsl.h          ]
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		VertexShaderHlsl(Direct3D10Renderer& direct3D10Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IVertexShader(direct3D10Renderer),
			mD3DBlobVertexShader(nullptr),
			mD3D10VertexShader(nullptr)
		{
			// Backup the vertex shader bytecode
			D3DCreateBlob(shaderBytecode.getNumberOfBytes(), &mD3DBlobVertexShader);
			memcpy(mD3DBlobVertexShader->GetBufferPointer(), shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes());

			// Create the Direct3D 10 vertex shader
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateVertexShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), &mD3D10VertexShader));

			// Don't assign a default name to the resource for debugging purposes, Direct3D 10 automatically sets a decent default name
		}

		/**
		*  @brief
		*    Constructor for creating a vertex shader from shader source code
		*
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		VertexShaderHlsl(Direct3D10Renderer& direct3D10Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IVertexShader(direct3D10Renderer),
			mD3DBlobVertexShader(nullptr),
			mD3D10VertexShader(nullptr)
		{
			// Create the Direct3D 10 binary large object for the vertex shader
			mD3DBlobVertexShader = loadShaderFromSourcecode(direct3D10Renderer.getContext(), "vs_4_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != mD3DBlobVertexShader)
			{
				// Create the Direct3D 10 vertex shader
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateVertexShader(mD3DBlobVertexShader->GetBufferPointer(), mD3DBlobVertexShader->GetBufferSize(), &mD3D10VertexShader));

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(mD3DBlobVertexShader->GetBufferSize()), static_cast<uint8_t*>(mD3DBlobVertexShader->GetBufferPointer()));
				}
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 10 automatically sets a decent default name
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~VertexShaderHlsl() override
		{
			// Release the Direct3D 10 shader binary large object
			if (nullptr != mD3DBlobVertexShader)
			{
				mD3DBlobVertexShader->Release();
			}

			// Release the Direct3D 10 vertex shader
			if (nullptr != mD3D10VertexShader)
			{
				mD3D10VertexShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 vertex shader blob
		*
		*  @return
		*    Direct3D 10 vertex shader blob, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3DBlob* getD3DBlobVertexShader() const
		{
			return mD3DBlobVertexShader;
		}

		/**
		*  @brief
		*    Return the Direct3D 10 vertex shader
		*
		*  @return
		*    Direct3D 10 vertex shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10VertexShader* getD3D10VertexShader() const
		{
			return mD3D10VertexShader;
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
				if (nullptr != mD3D10VertexShader)
				{
					FAILED_DEBUG_BREAK(mD3D10VertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10VertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
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
		ID3DBlob*		    mD3DBlobVertexShader;	///< Direct3D 10 vertex shader blob, can be a null pointer
		ID3D10VertexShader* mD3D10VertexShader;		///< Direct3D 10 vertex shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Shader/GeometryShaderHlsl.h        ]
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		GeometryShaderHlsl(Direct3D10Renderer& direct3D10Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IGeometryShader(direct3D10Renderer),
			mD3D10GeometryShader(nullptr)
		{
			// Create the Direct3D 10 geometry shader
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateGeometryShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), &mD3D10GeometryShader));

			// Don't assign a default name to the resource for debugging purposes, Direct3D 10 automatically sets a decent default name
		}

		/**
		*  @brief
		*    Constructor for creating a geometry shader from shader source code
		*
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		GeometryShaderHlsl(Direct3D10Renderer& direct3D10Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IGeometryShader(direct3D10Renderer),
			mD3D10GeometryShader(nullptr)
		{
			// Create the Direct3D 10 binary large object for the geometry shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D10Renderer.getContext(), "gs_4_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 10 geometry shader
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateGeometryShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), &mD3D10GeometryShader));

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 10 shader binary large object
				d3dBlob->Release();
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 10 automatically sets a decent default name
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GeometryShaderHlsl() override
		{
			// Release the Direct3D 10 geometry shader
			if (nullptr != mD3D10GeometryShader)
			{
				mD3D10GeometryShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 geometry shader
		*
		*  @return
		*    Direct3D 10 geometry shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10GeometryShader* getD3D10GeometryShader() const
		{
			return mD3D10GeometryShader;
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
				if (nullptr != mD3D10GeometryShader)
				{
					FAILED_DEBUG_BREAK(mD3D10GeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10GeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
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
		ID3D10GeometryShader* mD3D10GeometryShader;	///< Direct3D 10 geometry shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Shader/FragmentShaderHlsl.h        ]
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] shaderBytecode
		*    Shader bytecode
		*/
		FragmentShaderHlsl(Direct3D10Renderer& direct3D10Renderer, const Renderer::ShaderBytecode& shaderBytecode) :
			IFragmentShader(direct3D10Renderer),
			mD3D10PixelShader(nullptr)
		{
			// Create the Direct3D 10 pixel shader
			FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreatePixelShader(shaderBytecode.getBytecode(), shaderBytecode.getNumberOfBytes(), &mD3D10PixelShader));

			// Don't assign a default name to the resource for debugging purposes, Direct3D 10 automatically sets a decent default name
		}

		/**
		*  @brief
		*    Constructor for creating a fragment shader from shader source code
		*
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] sourceCode
		*    Shader ASCII source code, must be valid
		*/
		FragmentShaderHlsl(Direct3D10Renderer& direct3D10Renderer, const char* sourceCode, Renderer::IShaderLanguage::OptimizationLevel optimizationLevel, Renderer::ShaderBytecode* shaderBytecode = nullptr) :
			IFragmentShader(direct3D10Renderer),
			mD3D10PixelShader(nullptr)
		{
			// Create the Direct3D 10 binary large object for the pixel shader
			ID3DBlob* d3dBlob = loadShaderFromSourcecode(direct3D10Renderer.getContext(), "ps_4_0", sourceCode, nullptr, optimizationLevel);
			if (nullptr != d3dBlob)
			{
				// Create the Direct3D 10 pixel shader
				FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreatePixelShader(d3dBlob->GetBufferPointer(), d3dBlob->GetBufferSize(), &mD3D10PixelShader));

				// Return shader bytecode, if requested do to so
				if (nullptr != shaderBytecode)
				{
					shaderBytecode->setBytecodeCopy(static_cast<uint32_t>(d3dBlob->GetBufferSize()), static_cast<uint8_t*>(d3dBlob->GetBufferPointer()));
				}

				// Release the Direct3D 10 shader binary large object
				d3dBlob->Release();
			}

			// Don't assign a default name to the resource for debugging purposes, Direct3D 10 automatically sets a decent default name
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~FragmentShaderHlsl() override
		{
			// Release the Direct3D 10 pixel shader
			if (nullptr != mD3D10PixelShader)
			{
				mD3D10PixelShader->Release();
			}
		}

		/**
		*  @brief
		*    Return the Direct3D 10 pixel shader
		*
		*  @return
		*    Direct3D 10 pixel shader, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10PixelShader* getD3D10PixelShader() const
		{
			return mD3D10PixelShader;
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
				if (nullptr != mD3D10PixelShader)
				{
					FAILED_DEBUG_BREAK(mD3D10PixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10PixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
				}
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
		ID3D10PixelShader* mD3D10PixelShader;	///< Direct3D 10 pixel shader, can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Shader/GraphicsProgramHlsl.h       ]
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] vertexShaderHlsl
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShaderHlsl
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShaderHlsl
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgramHlsl(Direct3D10Renderer& direct3D10Renderer, VertexShaderHlsl* vertexShaderHlsl, GeometryShaderHlsl* geometryShaderHlsl, FragmentShaderHlsl* fragmentShaderHlsl) :
			IGraphicsProgram(direct3D10Renderer),
			mDirect3D10Renderer(&direct3D10Renderer),
			mVertexShaderHlsl(vertexShaderHlsl),
			mGeometryShaderHlsl(geometryShaderHlsl),
			mFragmentShaderHlsl(fragmentShaderHlsl)
		{
			// Add references to the provided shaders
			if (nullptr != mVertexShaderHlsl)
			{
				mVertexShaderHlsl->addReference();
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
		Direct3D10Renderer* mDirect3D10Renderer;	///< Owner Direct3D 10 renderer instance, always valid
		VertexShaderHlsl*   mVertexShaderHlsl;		///< Vertex shader the graphics program is using (we keep a reference to it), can be a null pointer
		GeometryShaderHlsl* mGeometryShaderHlsl;	///< Geometry shader the graphics program is using (we keep a reference to it), can be a null pointer
		FragmentShaderHlsl* mFragmentShaderHlsl;	///< Fragment shader the graphics program is using (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ Direct3D10Renderer/Shader/ShaderLanguageHlsl.h        ]
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*/
		inline explicit ShaderLanguageHlsl(Direct3D10Renderer& direct3D10Renderer) :
			IShaderLanguage(direct3D10Renderer)
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
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::vertexShader", we know there's vertex shader support
			return RENDERER_NEW(getRenderer().getContext(), VertexShaderHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode([[maybe_unused]] const Renderer::ShaderBytecode& shaderBytecode) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 has no tessellation control shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode([[maybe_unused]] const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 has no tessellation control shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode([[maybe_unused]] const Renderer::ShaderBytecode& shaderBytecode) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 has no tessellation evaluation shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode([[maybe_unused]] const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 has no tessellation evaluation shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			return RENDERER_NEW(getRenderer().getContext(), GeometryShaderHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			// Ignore "gsInputPrimitiveTopology", it's directly set within HLSL
			// Ignore "gsOutputPrimitiveTopology", it's directly set within HLSL
			// Ignore "numberOfOutputVertices", it's directly set within HLSL
			return RENDERER_NEW(getRenderer().getContext(), GeometryShaderHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(const Renderer::ShaderSourceCode& shaderSourceCode, Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShaderHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), shaderSourceCode.sourceCode, getOptimizationLevel(), shaderBytecode);
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromBytecode(const Renderer::ShaderBytecode&) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 has no compute shader support")
			return nullptr;
		}

		[[nodiscard]] inline virtual Renderer::IComputeShader* createComputeShaderFromSourceCode(const Renderer::ShaderSourceCode&, Renderer::ShaderBytecode* = nullptr) override
		{
			RENDERER_ASSERT(getRenderer().getContext(), false, "Direct3D 10 has no compute shader support")
			return nullptr;
		}

		[[nodiscard]] virtual Renderer::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Renderer::IRootSignature& rootSignature, [[maybe_unused]] const Renderer::VertexAttributes& vertexAttributes, Renderer::IVertexShader* vertexShader, [[maybe_unused]] Renderer::ITessellationControlShader* tessellationControlShader, [[maybe_unused]] Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 10 vertex shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationControlShader, "Direct3D 10 has no tessellation control shader support")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == tessellationEvaluationShader, "Direct3D 10 has no tessellation evaluation shader support")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 10 geometry shader language mismatch")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::HLSL_NAME, "Direct3D 10 fragment shader language mismatch")

			// Create the graphics program
			return RENDERER_NEW(getRenderer().getContext(), GraphicsProgramHlsl)(static_cast<Direct3D10Renderer&>(getRenderer()), static_cast<VertexShaderHlsl*>(vertexShader), static_cast<GeometryShaderHlsl*>(geometryShader), static_cast<FragmentShaderHlsl*>(fragmentShader));
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
	//[ Direct3D10Renderer/State/GraphicsPipelineState.h      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Direct3D 10 graphics pipeline state class
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
		*  @param[in] direct3D10Renderer
		*    Owner Direct3D 10 renderer instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*/
		GraphicsPipelineState(Direct3D10Renderer& direct3D10Renderer, const Renderer::GraphicsPipelineState& graphicsPipelineState) :
			IGraphicsPipelineState(direct3D10Renderer),
			mD3D10Device(direct3D10Renderer.getD3D10Device()),
			mD3D10PrimitiveTopology(static_cast<D3D10_PRIMITIVE_TOPOLOGY>(graphicsPipelineState.primitiveTopology)),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass),
			mD3D10InputLayout(nullptr),
			mRasterizerState(direct3D10Renderer, graphicsPipelineState.rasterizerState),
			mDepthStencilState(direct3D10Renderer, graphicsPipelineState.depthStencilState),
			mBlendState(direct3D10Renderer, graphicsPipelineState.blendState)
		{
			// Acquire our Direct3D 10 device reference
			mD3D10Device->AddRef();

			// Add a reference to the given graphics program and render pass
			mGraphicsProgram->addReference();
			mRenderPass->addReference();

			// Create Direct3D 10 input element descriptions with support for attribute-less rendering
			const uint32_t numberOfAttributes = graphicsPipelineState.vertexAttributes.numberOfAttributes;
			if (numberOfAttributes > 0)
			{
				const VertexShaderHlsl* vertexShaderHlsl = static_cast<GraphicsProgramHlsl*>(mGraphicsProgram)->getVertexShaderHlsl();
				if (nullptr != vertexShaderHlsl)
				{
					const Renderer::VertexAttribute* attributes = graphicsPipelineState.vertexAttributes.attributes;

					// Create Direct3D 10 input element descriptions
					// TODO(co) We could manage in here without new/delete when using a fixed maximum supported number of elements
					const Renderer::Context& context = getRenderer().getContext();
					D3D10_INPUT_ELEMENT_DESC* d3d10InputElementDescs   = numberOfAttributes ? RENDERER_MALLOC_TYPED(context, D3D10_INPUT_ELEMENT_DESC, numberOfAttributes) : RENDERER_MALLOC_TYPED(context, D3D10_INPUT_ELEMENT_DESC, 1);
					D3D10_INPUT_ELEMENT_DESC* d3d10InputElementDesc    = d3d10InputElementDescs;
					D3D10_INPUT_ELEMENT_DESC* d3d10InputElementDescEnd = d3d10InputElementDescs + numberOfAttributes;
					for (; d3d10InputElementDesc < d3d10InputElementDescEnd; ++d3d10InputElementDesc, ++attributes)
					{
						// Fill the "D3D10_INPUT_ELEMENT_DESC"-content
						d3d10InputElementDesc->SemanticName      = attributes->semanticName;										// Semantic name (LPCSTR)
						d3d10InputElementDesc->SemanticIndex     = attributes->semanticIndex;										// Semantic index (UINT)
						d3d10InputElementDesc->Format            = Mapping::getDirect3D10Format(attributes->vertexAttributeFormat);	// Format (DXGI_FORMAT)
						d3d10InputElementDesc->InputSlot         = static_cast<UINT>(attributes->inputSlot);						// Input slot (UINT)
						d3d10InputElementDesc->AlignedByteOffset = attributes->alignedByteOffset;									// Aligned byte offset (UINT)

						// Per-instance instead of per-vertex?
						if (attributes->instancesPerElement > 0)
						{
							d3d10InputElementDesc->InputSlotClass       = D3D10_INPUT_PER_INSTANCE_DATA;	// Input classification (D3D10_INPUT_CLASSIFICATION)
							d3d10InputElementDesc->InstanceDataStepRate = attributes->instancesPerElement;	// Instance data step rate (UINT)
						}
						else
						{
							d3d10InputElementDesc->InputSlotClass       = D3D10_INPUT_PER_VERTEX_DATA;	// Input classification (D3D10_INPUT_CLASSIFICATION)
							d3d10InputElementDesc->InstanceDataStepRate = 0;							// Instance data step rate (UINT)
						}
					}

					{ // Create the Direct3D 10 input layout
						ID3DBlob* d3dBlobVertexShader = vertexShaderHlsl->getD3DBlobVertexShader();
						FAILED_DEBUG_BREAK(direct3D10Renderer.getD3D10Device()->CreateInputLayout(d3d10InputElementDescs, numberOfAttributes, d3dBlobVertexShader->GetBufferPointer(), d3dBlobVertexShader->GetBufferSize(), &mD3D10InputLayout));
					}

					// Destroy Direct3D 10 input element descriptions
					RENDERER_FREE(context, d3d10InputElementDescs);
				}
				else
				{
					RENDERER_LOG(direct3D10Renderer.getContext(), CRITICAL, "Failed to create the Direct3D 10 graphics pipeline stage input layout because there's no vertex shader")
				}
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
			// Release the graphics program and render pass reference
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Release the Direct3D 10 input layout
			if (nullptr != mD3D10InputLayout)
			{
				mD3D10InputLayout->Release();
			}

			// Release our Direct3D 10 device reference
			mD3D10Device->Release();
		}

		/**
		*  @brief
		*    Return the Direct3D 10 primitive topology
		*
		*  @return
		*    The Direct3D 10 primitive topology
		*/
		[[nodiscard]] inline D3D10_PRIMITIVE_TOPOLOGY getD3D10PrimitiveTopology() const
		{
			return mD3D10PrimitiveTopology;
		}

		/**
		*  @brief
		*    Return the Direct3D 10 input layout
		*
		*  @return
		*    Direct3D 10 input layout instance, can be a null pointer on error, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline ID3D10InputLayout* getD3D10InputLayout() const
		{
			return mD3D10InputLayout;
		}

		/**
		*  @brief
		*    Bind the graphics pipeline state
		*/
		void bindGraphicsPipelineState() const
		{
			// Set the Direct3D 10 input layout
			if (nullptr != mD3D10InputLayout)
			{
				mD3D10Device->IASetInputLayout(mD3D10InputLayout);
			}

			// Set the graphics program
			static_cast<Direct3D10Renderer&>(getRenderer()).setGraphicsProgram(mGraphicsProgram);

			// Set the Direct3D 10 rasterizer state
			mD3D10Device->RSSetState(mRasterizerState.getD3D10RasterizerState());

			// Set Direct3D 10 depth stencil state
			mD3D10Device->OMSetDepthStencilState(mDepthStencilState.getD3D10DepthStencilState(), 0);

			// Set Direct3D 10 blend state
			mD3D10Device->OMSetBlendState(mBlendState.getD3D10BlendState(), 0, 0xffffffff);
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
				if (nullptr != mD3D10InputLayout)
				{
					FAILED_DEBUG_BREAK(mD3D10InputLayout->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
					FAILED_DEBUG_BREAK(mD3D10InputLayout->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name));
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
		ID3D10Device*				mD3D10Device;				///< The Direct3D 10 device context instance (we keep a reference to it), null pointer on horrible error (so we don't check)
		D3D10_PRIMITIVE_TOPOLOGY	mD3D10PrimitiveTopology;
		Renderer::IGraphicsProgram*	mGraphicsProgram;
		Renderer::IRenderPass*		mRenderPass;
		ID3D10InputLayout*			mD3D10InputLayout;			///< Direct3D 10 input layout, can be a null pointer
		RasterizerState				mRasterizerState;
		DepthStencilState			mDepthStencilState;
		BlendState					mBlendState;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D10Renderer




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
		bool createDevice(UINT flags, ID3D10Device** d3d10Device)
		{
			// Driver types
			static constexpr D3D10_DRIVER_TYPE D3D10_DRIVER_TYPES[] =
			{
				D3D10_DRIVER_TYPE_HARDWARE,
				D3D10_DRIVER_TYPE_WARP,
				D3D10_DRIVER_TYPE_REFERENCE,
			};
			static constexpr UINT NUMBER_OF_DRIVER_TYPES = _countof(D3D10_DRIVER_TYPES);

			// Create the Direct3D 10 device
			for (UINT deviceType = 0; deviceType < NUMBER_OF_DRIVER_TYPES; ++deviceType)
			{
				if (SUCCEEDED(Direct3D10Renderer::D3D10CreateDevice(nullptr, D3D10_DRIVER_TYPES[deviceType], nullptr, flags, D3D10_SDK_VERSION, d3d10Device)))
				{
					// Done
					return true;
				}
			}

			// Error!
			return false;
		}


		namespace BackendDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ExecuteCommandBuffer* realData = static_cast<const Renderer::Command::ExecuteCommandBuffer*>(data);
				RENDERER_ASSERT(renderer.getContext(), nullptr != realData->commandBufferToExecute, "The Direct3D 10 command buffer to execute must be valid")
				renderer.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsRootSignature* realData = static_cast<const Renderer::Command::SetGraphicsRootSignature*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsPipelineState* realData = static_cast<const Renderer::Command::SetGraphicsPipelineState*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsResourceGroup* realData = static_cast<const Renderer::Command::SetGraphicsResourceGroup*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Renderer::IRenderer& renderer)
			{
				// Input-assembler (IA) stage
				const Renderer::Command::SetGraphicsVertexArray* realData = static_cast<const Renderer::Command::SetGraphicsVertexArray*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsViewports* realData = static_cast<const Renderer::Command::SetGraphicsViewports*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Renderer::Viewport*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Renderer::IRenderer& renderer)
			{
				// Rasterizer (RS) stage
				const Renderer::Command::SetGraphicsScissorRectangles* realData = static_cast<const Renderer::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Renderer::ScissorRectangle*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Renderer::IRenderer& renderer)
			{
				// SetGraphicsRenderTarget
				const Renderer::Command::SetGraphicsRenderTarget* realData = static_cast<const Renderer::Command::SetGraphicsRenderTarget*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ClearGraphics* realData = static_cast<const Renderer::Command::ClearGraphics*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawGraphics* realData = static_cast<const Renderer::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).drawGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).drawGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::DrawIndexedGraphics* realData = static_cast<const Renderer::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).drawIndexedGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).drawIndexedGraphicsEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 10 doesn't support compute root signature")
			}

			void SetComputePipelineState(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 10 doesn't support compute pipeline state")
			}

			void SetComputeResourceGroup(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 10 doesn't support compute resource group")
			}

			void DispatchCompute(const void*, Renderer::IRenderer& renderer)
			{
				RENDERER_LOG(static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).getContext(), CRITICAL, "Direct3D 10 doesn't support compute dispatch")
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetTextureMinimumMaximumMipmapIndex* realData = static_cast<const Renderer::Command::SetTextureMinimumMaximumMipmapIndex*>(data);
				if (realData->texture->getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					static_cast<Direct3D10Renderer::Texture2D*>(realData->texture)->setMinimumMaximumMipmapIndex(realData->minimumMipmapIndex, realData->maximumMipmapIndex);
				}
				else
				{
					RENDERER_LOG(static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).getContext(), CRITICAL, "Unsupported Direct3D 10 texture resource type")
				}
			}

			void ResolveMultisampleFramebuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Renderer::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::CopyResource* realData = static_cast<const Renderer::Command::CopyResource*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::GenerateMipmaps* realData = static_cast<const Renderer::Command::GenerateMipmaps*>(data);
				static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RENDERER_DEBUG
				void SetDebugMarker(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::SetDebugMarker* realData = static_cast<const Renderer::Command::SetDebugMarker*>(data);
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::BeginDebugEvent* realData = static_cast<const Renderer::Command::BeginDebugEvent*>(data);
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Renderer::IRenderer& renderer)
				{
					static_cast<Direct3D10Renderer::Direct3D10Renderer&>(renderer).endDebugEvent();
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
namespace Direct3D10Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Direct3D10Renderer::Direct3D10Renderer(const Renderer::Context& context) :
		IRenderer(Renderer::NameId::DIRECT3D10, context),
		mDirect3D10RuntimeLinking(nullptr),
		mD3D10Device(nullptr),
		mDirect3D9RuntimeLinking(nullptr),
		mShaderLanguageHlsl(nullptr),
		mD3D10QueryFlush(nullptr),
		mRenderTarget(nullptr),
		mGraphicsRootSignature(nullptr),
		mD3D10PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
		mD3d10VertexShader(nullptr),
		mD3d10GeometryShader(nullptr),
		mD3d10PixelShader(nullptr)
	{
		mDirect3D10RuntimeLinking = RENDERER_NEW(mContext, Direct3D10RuntimeLinking)(*this);

		// Is Direct3D 10 available?
		if (mDirect3D10RuntimeLinking->isDirect3D10Avaiable())
		{
			// Flags
			UINT flags = 0;
			#ifdef RENDERER_DEBUG
				flags |= D3D10_CREATE_DEVICE_DEBUG;
			#endif

			// Create the Direct3D 10 device
			if (!detail::createDevice(flags, &mD3D10Device) && (flags & D3D10_CREATE_DEVICE_DEBUG))
			{
				RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 10 device instance, retrying without debug flag (maybe no Windows SDK is installed)")
				flags &= ~D3D10_CREATE_DEVICE_DEBUG;
				detail::createDevice(flags, &mD3D10Device);
			}

			// Is there a Direct3D 10 device?
			if (nullptr != mD3D10Device)
			{
				#ifdef RENDERER_DEBUG
					// Create the Direct3D 9 runtime linking instance, we know there can't be one, yet
					mDirect3D9RuntimeLinking = RENDERER_NEW(mContext, Direct3D9RuntimeLinking)(*this);

					// Call the Direct3D 9 PIX function
					if (mDirect3D9RuntimeLinking->isDirect3D9Avaiable())
					{
						// Disable debugging
						D3DPERF_SetOptions(1);
					}
				#endif

				// Direct3D 10 debug settings
				if (flags & D3D10_CREATE_DEVICE_DEBUG)
				{
					ID3D10Debug* d3d10Debug = nullptr;
					if (SUCCEEDED(mD3D10Device->QueryInterface(__uuidof(ID3D10Debug), reinterpret_cast<LPVOID*>(&d3d10Debug))))
					{
						ID3D10InfoQueue* d3d10InfoQueue = nullptr;
						if (SUCCEEDED(d3d10Debug->QueryInterface(__uuidof(ID3D10InfoQueue), reinterpret_cast<LPVOID*>(&d3d10InfoQueue))))
						{
							// When using render-to-texture, Direct3D 10 will quickly spam the log with
							//   "
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets: Resource being set to OM RenderTarget slot 0 is still bound on input! [ STATE_SETTING WARNING #9: DEVICE_OMSETRENDERTARGETS_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing VS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #3: DEVICE_VSSETSHADERRESOURCES_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing GS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #5: DEVICE_GSSETSHADERRESOURCES_HAZARD]
							//   D3D11 WARNING: ID3D11DeviceContext::OMSetRenderTargets[AndUnorderedAccessViews]: Forcing PS shader resource slot 0 to NULL. [ STATE_SETTING WARNING #7: DEVICE_PSSETSHADERRESOURCES_HAZARD]
							//   "
							// (yes there's really D3D11 visible when using Windows 10 64 bit)
							// When not unbinding render targets from shader resources, even if shaders never access the render target by reading. We could add extra
							// logic to avoid this situation, but on the other hand, the renderer backend should be as slim as possible. Since those Direct3D 10 warnings
							// are pretty annoying and introduce the risk of missing relevant warnings, let's suppress those warnings. Thought about this for a while, feels
							// like the best solution considering the alternatives even if suppressing warnings is not always the best idea.
							D3D10_MESSAGE_ID d3d10MessageIds[] =
							{
								D3D10_MESSAGE_ID_DEVICE_OMSETRENDERTARGETS_HAZARD,
								D3D10_MESSAGE_ID_DEVICE_VSSETSHADERRESOURCES_HAZARD,
								D3D10_MESSAGE_ID_DEVICE_GSSETSHADERRESOURCES_HAZARD,
								D3D10_MESSAGE_ID_DEVICE_PSSETSHADERRESOURCES_HAZARD
							};
							D3D10_INFO_QUEUE_FILTER d3d10InfoQueueFilter = {};
							d3d10InfoQueueFilter.DenyList.NumIDs = _countof(d3d10MessageIds);
							d3d10InfoQueueFilter.DenyList.pIDList = d3d10MessageIds;
							FAILED_DEBUG_BREAK(d3d10InfoQueue->AddStorageFilterEntries(&d3d10InfoQueueFilter));

							// Sadly, when using the Direct3D 10 break feature we're having a confusing call stack, so we don't use this and use "FAILED_DEBUG_BREAK()" instead
							// d3d10InfoQueue->SetBreakOnSeverity(D3D10_MESSAGE_SEVERITY_CORRUPTION, true);
							// d3d10InfoQueue->SetBreakOnSeverity(D3D10_MESSAGE_SEVERITY_ERROR, true);
							// d3d10InfoQueue->SetBreakOnSeverity(D3D10_MESSAGE_SEVERITY_WARNING, true);
							// d3d10InfoQueue->SetBreakOnSeverity(D3D10_MESSAGE_SEVERITY_INFO, true);

							d3d10InfoQueue->Release();
						}
						d3d10Debug->Release();
					}
				}

				// Initialize the capabilities
				initializeCapabilities();
			}
			else
			{
				RENDERER_LOG(mContext, CRITICAL, "Failed to create the Direct3D 10 device instance")
			}
		}
	}

	Direct3D10Renderer::~Direct3D10Renderer()
	{
		// Release instances
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

		#ifdef RENDERER_STATISTICS
		{ // For debugging: At this point there should be no resource instances left, validate this!
			// -> Are the currently any resource instances?
			const uint32_t numberOfCurrentResources = getStatistics().getNumberOfCurrentResources();
			if (numberOfCurrentResources > 0)
			{
				// Error!
				if (numberOfCurrentResources > 1)
				{
					RENDERER_LOG(mContext, CRITICAL, "The Direct3D 10 renderer backend is going to be destroyed, but there are still %d resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "The Direct3D 10 renderer backend is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the Direct3D 10 query instance used for flush, in case we have one
		if (nullptr != mD3D10QueryFlush)
		{
			mD3D10QueryFlush->Release();
		}

		// Release the HLSL shader language instance, in case we have one
		if (nullptr != mShaderLanguageHlsl)
		{
			mShaderLanguageHlsl->releaseReference();
		}

		// Release the Direct3D 10 we've created
		if (nullptr != mD3D10Device)
		{
			mD3D10Device->Release();
			mD3D10Device = nullptr;
		}

		// Destroy the Direct3D 10 runtime linking instance
		RENDERER_DELETE(mContext, Direct3D10RuntimeLinking, mDirect3D10RuntimeLinking);

		// Destroy the Direct3D 9 runtime linking instance, in case there's one
		#ifdef RENDERER_DEBUG
			RENDERER_DELETE(mContext, Direct3D9RuntimeLinking, mDirect3D9RuntimeLinking);
		#endif
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void Direct3D10Renderer::setGraphicsRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		mGraphicsRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->addReference();

			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void Direct3D10Renderer::setGraphicsPipelineState(Renderer::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsPipelineState)

			// Set primitive topology
			// -> The "Renderer::PrimitiveTopology" values directly map to Direct3D 9 & 10 & 11 constants, do not change them
			const GraphicsPipelineState* direct3D10GraphicsPipelineState = static_cast<const GraphicsPipelineState*>(graphicsPipelineState);
			if (mD3D10PrimitiveTopology != direct3D10GraphicsPipelineState->getD3D10PrimitiveTopology())
			{
				mD3D10PrimitiveTopology = direct3D10GraphicsPipelineState->getD3D10PrimitiveTopology();
				mD3D10Device->IASetPrimitiveTopology(mD3D10PrimitiveTopology);
			}

			// Set graphics pipeline state
			direct3D10GraphicsPipelineState->bindGraphicsPipelineState();
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D10Renderer::setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No Direct3D 10 renderer backend graphics root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Direct3D 10 renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The Direct3D 10 renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The Direct3D 10 renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resourceGroup)

			// Set graphics resource group
			const ResourceGroup* d3d10ResourceGroup = static_cast<ResourceGroup*>(resourceGroup);
			const uint32_t numberOfResources = d3d10ResourceGroup->getNumberOfResources();
			Renderer::IResource** resources = d3d10ResourceGroup->getResources();
			const Renderer::RootParameter& rootParameter = mGraphicsRootSignature->getRootSignature().parameters[rootParameterIndex];
			for (uint32_t resourceIndex = 0; resourceIndex < numberOfResources; ++resourceIndex, ++resources)
			{
				const Renderer::IResource* resource = *resources;
				RENDERER_ASSERT(mContext, nullptr != reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "Invalid Direct3D 10 descriptor ranges")
				const Renderer::DescriptorRange& descriptorRange = reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges)[resourceIndex];

				// Check the type of resource to set
				// TODO(co) Some additional resource type root signature security checks in debug build?
				const Renderer::ResourceType resourceType = resource->getResourceType();
				switch (resourceType)
				{
					case Renderer::ResourceType::UNIFORM_BUFFER:
					{
						ID3D10Buffer* d3d10Buffers = static_cast<const UniformBuffer*>(resource)->getD3D10Buffer();
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Renderer::ShaderVisibility::ALL:
							case Renderer::ShaderVisibility::ALL_GRAPHICS:
								mD3D10Device->VSSetConstantBuffers(startSlot, 1, &d3d10Buffers);
								// Direct3D 10 has no tessellation control shader support
								// Direct3D 10 has no tessellation evaluation shader support
								mD3D10Device->GSSetConstantBuffers(startSlot, 1, &d3d10Buffers);
								mD3D10Device->PSSetConstantBuffers(startSlot, 1, &d3d10Buffers);
								// Direct3D 10 has no compute shader support
								break;

							case Renderer::ShaderVisibility::VERTEX:
								mD3D10Device->VSSetConstantBuffers(startSlot, 1, &d3d10Buffers);
								break;

							case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no tessellation control shader support (hull shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no tessellation evaluation shader support (domain shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::GEOMETRY:
								mD3D10Device->GSSetConstantBuffers(startSlot, 1, &d3d10Buffers);
								break;

							case Renderer::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology
								mD3D10Device->PSSetConstantBuffers(startSlot, 1, &d3d10Buffers);
								break;

							case Renderer::ShaderVisibility::COMPUTE:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no compute shader support")
								break;
						}
						break;
					}

					case Renderer::ResourceType::TEXTURE_BUFFER:
					case Renderer::ResourceType::STRUCTURED_BUFFER:
					case Renderer::ResourceType::TEXTURE_1D:
					case Renderer::ResourceType::TEXTURE_2D:
					case Renderer::ResourceType::TEXTURE_2D_ARRAY:
					case Renderer::ResourceType::TEXTURE_3D:
					case Renderer::ResourceType::TEXTURE_CUBE:
					{
						ID3D10ShaderResourceView* d3d10ShaderResourceView = nullptr;
						switch (resourceType)
						{
							case Renderer::ResourceType::TEXTURE_BUFFER:
								d3d10ShaderResourceView = static_cast<const TextureBuffer*>(resource)->getD3D10ShaderResourceView();
								break;

							case Renderer::ResourceType::STRUCTURED_BUFFER:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no structured buffer support")
								break;

							case Renderer::ResourceType::TEXTURE_1D:
								d3d10ShaderResourceView = static_cast<const Texture1D*>(resource)->getD3D10ShaderResourceView();
								break;

							case Renderer::ResourceType::TEXTURE_2D:
								d3d10ShaderResourceView = static_cast<const Texture2D*>(resource)->getD3D10ShaderResourceView();
								break;

							case Renderer::ResourceType::TEXTURE_2D_ARRAY:
								d3d10ShaderResourceView = static_cast<const Texture2DArray*>(resource)->getD3D10ShaderResourceView();
								break;

							case Renderer::ResourceType::TEXTURE_3D:
								d3d10ShaderResourceView = static_cast<const Texture3D*>(resource)->getD3D10ShaderResourceView();
								break;

							case Renderer::ResourceType::TEXTURE_CUBE:
								d3d10ShaderResourceView = static_cast<const TextureCube*>(resource)->getD3D10ShaderResourceView();
								break;

							case Renderer::ResourceType::ROOT_SIGNATURE:
							case Renderer::ResourceType::RESOURCE_GROUP:
							case Renderer::ResourceType::GRAPHICS_PROGRAM:
							case Renderer::ResourceType::VERTEX_ARRAY:
							case Renderer::ResourceType::RENDER_PASS:
							case Renderer::ResourceType::SWAP_CHAIN:
							case Renderer::ResourceType::FRAMEBUFFER:
							case Renderer::ResourceType::INDEX_BUFFER:
							case Renderer::ResourceType::VERTEX_BUFFER:
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
								RENDERER_ASSERT(mContext, false, "Invalid Direct3D 10 renderer backend resource type")
								break;
						}
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Renderer::ShaderVisibility::ALL:
							case Renderer::ShaderVisibility::ALL_GRAPHICS:
								mD3D10Device->VSSetShaderResources(startSlot, 1, &d3d10ShaderResourceView);
								// Direct3D 10 has no tessellation control shader support
								// Direct3D 10 has no tessellation evaluation shader support
								mD3D10Device->GSSetShaderResources(startSlot, 1, &d3d10ShaderResourceView);
								mD3D10Device->PSSetShaderResources(startSlot, 1, &d3d10ShaderResourceView);
								// Direct3D 10 has no compute shader support
								break;

							case Renderer::ShaderVisibility::VERTEX:
								mD3D10Device->VSSetShaderResources(startSlot, 1, &d3d10ShaderResourceView);
								break;

							case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no tessellation control shader support (hull shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no tessellation evaluation shader support (domain shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::GEOMETRY:
								mD3D10Device->GSSetShaderResources(startSlot, 1, &d3d10ShaderResourceView);
								break;

							case Renderer::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology
								mD3D10Device->PSSetShaderResources(startSlot, 1, &d3d10ShaderResourceView);
								break;

							case Renderer::ShaderVisibility::COMPUTE:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no compute shader support")
								break;
						}
						break;
					}

					case Renderer::ResourceType::SAMPLER_STATE:
					{
						ID3D10SamplerState* d3d10SamplerState = static_cast<const SamplerState*>(resource)->getD3D10SamplerState();
						const UINT startSlot = descriptorRange.baseShaderRegister;
						switch (descriptorRange.shaderVisibility)
						{
							case Renderer::ShaderVisibility::ALL:
							case Renderer::ShaderVisibility::ALL_GRAPHICS:
								mD3D10Device->VSSetSamplers(startSlot, 1, &d3d10SamplerState);
								// Direct3D 10 has no tessellation control shader support
								// Direct3D 10 has no tessellation evaluation shader support
								mD3D10Device->GSSetSamplers(startSlot, 1, &d3d10SamplerState);
								mD3D10Device->PSSetSamplers(startSlot, 1, &d3d10SamplerState);
								// Direct3D 10 has no compute shader support
								break;

							case Renderer::ShaderVisibility::VERTEX:
								mD3D10Device->VSSetSamplers(startSlot, 1, &d3d10SamplerState);
								break;

							case Renderer::ShaderVisibility::TESSELLATION_CONTROL:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no tessellation control shader support (hull shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::TESSELLATION_EVALUATION:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no tessellation evaluation shader support (domain shader in Direct3D terminology)")
								break;

							case Renderer::ShaderVisibility::GEOMETRY:
								mD3D10Device->GSSetSamplers(startSlot, 1, &d3d10SamplerState);
								break;

							case Renderer::ShaderVisibility::FRAGMENT:
								// "pixel shader" in Direct3D terminology
								mD3D10Device->PSSetSamplers(startSlot, 1, &d3d10SamplerState);
								break;

							case Renderer::ShaderVisibility::COMPUTE:
								RENDERER_ASSERT(mContext, false, "Direct3D 10 has no compute shader support")
								break;
						}
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
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
						RENDERER_ASSERT(mContext, false, "Invalid Direct3D 10 renderer backend resource type")
						break;
				}
			}
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void Direct3D10Renderer::setGraphicsVertexArray(Renderer::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage
		if (nullptr != vertexArray)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *vertexArray)

			// Begin debug event
			RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

			static_cast<VertexArray*>(vertexArray)->setDirect3DIASetInputLayoutAndStreamSource();

			// End debug event
			RENDERER_END_DEBUG_EVENT(this)
		}
		else
		{
			mD3D10Device->IASetInputLayout(nullptr);
		}
	}

	void Direct3D10Renderer::setGraphicsViewports(uint32_t numberOfViewports, const Renderer::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid Direct3D 10 rasterizer state viewports")

		#ifdef RENDERER_DEBUG
			// Is the given number of viewports valid?
			if (numberOfViewports > (D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1))
			{
				RENDERER_LOG(mContext, CRITICAL, "Direct3D 10 supports only %d viewports", D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX)
				numberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
			}
		#endif

		// Set the Direct3D 10 viewports
		D3D10_VIEWPORT d3dViewports[D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
		D3D10_VIEWPORT* d3dViewport = d3dViewports;
		for (uint32_t i = 0; i < numberOfViewports; ++i, ++d3dViewport, ++viewports)
		{
			d3dViewport->TopLeftX = static_cast<INT> (viewports->topLeftX);
			d3dViewport->TopLeftY = static_cast<INT> (viewports->topLeftY);
			d3dViewport->Width    = static_cast<UINT>(viewports->width);
			d3dViewport->Height   = static_cast<UINT>(viewports->height);
			d3dViewport->MinDepth = viewports->minDepth;
			d3dViewport->MaxDepth = viewports->maxDepth;
		}
		mD3D10Device->RSSetViewports(numberOfViewports, d3dViewports);
	}

	void Direct3D10Renderer::setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RENDERER_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid Direct3D 10 rasterizer state scissor rectangles")

		// Set the Direct3D 10 scissor rectangles
		// -> "Renderer::ScissorRectangle" directly maps to Direct3D 9 & 10 & 11, do not change it
		// -> Let Direct3D 10 perform the index validation for us (the Direct3D 10 debug features are pretty good)
		mD3D10Device->RSSetScissorRects(numberOfScissorRectangles, reinterpret_cast<const D3D10_RECT*>(scissorRectangles));
	}

	void Direct3D10Renderer::setGraphicsRenderTarget(Renderer::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
				DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *renderTarget)

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
					case Renderer::ResourceType::SWAP_CHAIN:
					{
						// Get the Direct3D 10 swap chain instance
						SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

						// Direct3D 10 needs a pointer to a pointer, so give it one
						ID3D10RenderTargetView* d3d10RenderTargetView = swapChain->getD3D10RenderTargetView();
						mD3D10Device->OMSetRenderTargets(1, &d3d10RenderTargetView, swapChain->getD3D10DepthStencilView());
						break;
					}

					case Renderer::ResourceType::FRAMEBUFFER:
					{
						// Get the Direct3D 10 framebuffer instance
						Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

						// Set the Direct3D 10 render targets
						mD3D10Device->OMSetRenderTargets(framebuffer->getNumberOfColorTextures(), framebuffer->getD3D10RenderTargetViews(), framebuffer->getD3D10DepthStencilView());
						break;
					}

					case Renderer::ResourceType::ROOT_SIGNATURE:
					case Renderer::ResourceType::RESOURCE_GROUP:
					case Renderer::ResourceType::GRAPHICS_PROGRAM:
					case Renderer::ResourceType::VERTEX_ARRAY:
					case Renderer::ResourceType::RENDER_PASS:
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
				// Set the Direct3D 10 render targets
				mD3D10Device->OMSetRenderTargets(0, nullptr, nullptr);

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					mRenderTarget->releaseReference();
					mRenderTarget = nullptr;
				}
			}
		}
	}

	void Direct3D10Renderer::clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil)
	{
		// Unlike Direct3D 9, OpenGL or OpenGL ES 3, Direct3D 10 clears a given render target view and not the currently bound

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
					// Get the Direct3D 10 swap chain instance
					SwapChain* swapChain = static_cast<SwapChain*>(mRenderTarget);

					// Clear the Direct3D 10 render target view?
					if (clearFlags & Renderer::ClearFlag::COLOR)
					{
						mD3D10Device->ClearRenderTargetView(swapChain->getD3D10RenderTargetView(), color);
					}

					// Clear the Direct3D 10 depth stencil view?
					if (nullptr != swapChain->getD3D10DepthStencilView())
					{
						// Get the Direct3D 10 clear flags
						UINT direct3D10ClearFlags = (clearFlags & Renderer::ClearFlag::DEPTH) ? D3D10_CLEAR_DEPTH : 0u;
						if (clearFlags & Renderer::ClearFlag::STENCIL)
						{
							direct3D10ClearFlags |= D3D10_CLEAR_STENCIL;
						}
						if (0 != direct3D10ClearFlags)
						{
							// Clear the Direct3D 10 depth stencil view
							mD3D10Device->ClearDepthStencilView(swapChain->getD3D10DepthStencilView(), direct3D10ClearFlags, z, static_cast<UINT8>(stencil));
						}
					}
					break;
				}

				case Renderer::ResourceType::FRAMEBUFFER:
				{
					// Get the Direct3D 10 framebuffer instance
					Framebuffer* framebuffer = static_cast<Framebuffer*>(mRenderTarget);

					// Clear all Direct3D 10 render target views?
					if (clearFlags & Renderer::ClearFlag::COLOR)
					{
						// Loop through all Direct3D 10 render target views
						ID3D10RenderTargetView** d3d10RenderTargetViewsEnd = framebuffer->getD3D10RenderTargetViews() + framebuffer->getNumberOfColorTextures();
						for (ID3D10RenderTargetView** d3d10RenderTargetView = framebuffer->getD3D10RenderTargetViews(); d3d10RenderTargetView < d3d10RenderTargetViewsEnd; ++d3d10RenderTargetView)
						{
							// Valid Direct3D 10 render target view?
							if (nullptr != *d3d10RenderTargetView)
							{
								mD3D10Device->ClearRenderTargetView(*d3d10RenderTargetView, color);
							}
						}
					}

					// Clear the Direct3D 10 depth stencil view?
					if (nullptr != framebuffer->getD3D10DepthStencilView())
					{
						// Get the Direct3D 10 clear flags
						UINT direct3D10ClearFlags = (clearFlags & Renderer::ClearFlag::DEPTH) ? D3D10_CLEAR_DEPTH : 0u;
						if (clearFlags & Renderer::ClearFlag::STENCIL)
						{
							direct3D10ClearFlags |= D3D10_CLEAR_STENCIL;
						}
						if (0 != direct3D10ClearFlags)
						{
							// Clear the Direct3D 10 depth stencil view
							mD3D10Device->ClearDepthStencilView(framebuffer->getD3D10DepthStencilView(), direct3D10ClearFlags, z, static_cast<UINT8>(stencil));
						}
					}
					break;
				}

				case Renderer::ResourceType::ROOT_SIGNATURE:
				case Renderer::ResourceType::RESOURCE_GROUP:
				case Renderer::ResourceType::GRAPHICS_PROGRAM:
				case Renderer::ResourceType::VERTEX_ARRAY:
				case Renderer::ResourceType::RENDER_PASS:
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

	void Direct3D10Renderer::drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Direct3D 10 emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 10 draws must not be zero")

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

			// Draw
			if (drawArguments.instanceCount > 1 || drawArguments.startInstanceLocation > 0)
			{
				// With instancing
				mD3D10Device->DrawInstanced(
					drawArguments.vertexCountPerInstance,	// Vertex count per instance (UINT)
					drawArguments.instanceCount,			// Instance count (UINT)
					drawArguments.startVertexLocation,		// Start vertex location (UINT)
					drawArguments.startInstanceLocation		// Start instance location (UINT)
				);
			}
			else
			{
				// Without instancing
				mD3D10Device->Draw(
					drawArguments.vertexCountPerInstance,	// Vertex count (UINT)
					drawArguments.startVertexLocation		// Start index location (UINT)
				);
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

	void Direct3D10Renderer::drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset, uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The Direct3D 10 emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of Direct3D 10 draws must not be zero")

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

			// Draw
			if (drawIndexedArguments.instanceCount > 1 || drawIndexedArguments.startInstanceLocation > 0)
			{
				// With instancing
				mD3D10Device->DrawIndexedInstanced(
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
				mD3D10Device->DrawIndexed(
					drawIndexedArguments.indexCountPerInstance,	// Index count (UINT)
					drawIndexedArguments.startIndexLocation,	// Start index location (UINT)
					drawIndexedArguments.baseVertexLocation		// Base vertex location (INT)
				);
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
	}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void Direct3D10Renderer::resolveMultisampleFramebuffer(Renderer::IRenderTarget& destinationRenderTarget, Renderer::IFramebuffer& sourceMultisampleFramebuffer)
	{
		// Security check: Are the given resources owned by this renderer? (calls "return" in case of a mismatch)
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, destinationRenderTarget)
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, sourceMultisampleFramebuffer)

		// Evaluate the render target type
		switch (destinationRenderTarget.getResourceType())
		{
			case Renderer::ResourceType::SWAP_CHAIN:
			{
				// Get the Direct3D 10 swap chain instance
				// TODO(co) Implement me, not that important in practice so not directly implemented
				// SwapChain& swapChain = static_cast<SwapChain&>(destinationRenderTarget);
				break;
			}

			case Renderer::ResourceType::FRAMEBUFFER:
			{
				// Get the Direct3D 10 framebuffer instances
				const Framebuffer& direct3D10DestinationFramebuffer = static_cast<const Framebuffer&>(destinationRenderTarget);
				const Framebuffer& direct3D10SourceMultisampleFramebuffer = static_cast<const Framebuffer&>(sourceMultisampleFramebuffer);

				// Process all Direct3D 10 render target textures
				if (direct3D10DestinationFramebuffer.getNumberOfColorTextures() > 0 && direct3D10SourceMultisampleFramebuffer.getNumberOfColorTextures() > 0)
				{
					const uint32_t numberOfColorTextures = (direct3D10DestinationFramebuffer.getNumberOfColorTextures() < direct3D10SourceMultisampleFramebuffer.getNumberOfColorTextures()) ? direct3D10DestinationFramebuffer.getNumberOfColorTextures() : direct3D10SourceMultisampleFramebuffer.getNumberOfColorTextures();
					Renderer::ITexture** destinationTexture = direct3D10DestinationFramebuffer.getColorTextures();
					Renderer::ITexture** sourceTexture = direct3D10SourceMultisampleFramebuffer.getColorTextures();
					Renderer::ITexture** sourceTextureEnd = sourceTexture + numberOfColorTextures;
					for (; sourceTexture < sourceTextureEnd; ++sourceTexture, ++destinationTexture)
					{
						// Valid Direct3D 10 render target views?
						if (nullptr != *destinationTexture && nullptr != *sourceTexture)
						{
							const Texture2D* d3d10DestinationTexture2D = static_cast<const Texture2D*>(*destinationTexture);
							const Texture2D* d3d10SourceTexture2D = static_cast<const Texture2D*>(*sourceTexture);
							mD3D10Device->ResolveSubresource(d3d10DestinationTexture2D->getD3D10Texture2D(), D3D10CalcSubresource(0, 0, 1), d3d10SourceTexture2D->getD3D10Texture2D(), D3D10CalcSubresource(0, 0, 1), Mapping::getDirect3D10Format(d3d10DestinationTexture2D->getTextureFormat()));
						}
					}
				}

				// Process Direct3D 10 depth stencil texture
				if (nullptr != direct3D10DestinationFramebuffer.getDepthStencilTexture() && nullptr != direct3D10SourceMultisampleFramebuffer.getDepthStencilTexture())
				{
					const Texture2D* d3d10DestinationTexture2D = static_cast<const Texture2D*>(direct3D10DestinationFramebuffer.getDepthStencilTexture());
					const Texture2D* d3d10SourceTexture2D = static_cast<const Texture2D*>(direct3D10SourceMultisampleFramebuffer.getDepthStencilTexture());
					mD3D10Device->ResolveSubresource(d3d10DestinationTexture2D->getD3D10Texture2D(), D3D10CalcSubresource(0, 0, 1), d3d10SourceTexture2D->getD3D10Texture2D(), D3D10CalcSubresource(0, 0, 1), Mapping::getDirect3D10Format(d3d10DestinationTexture2D->getTextureFormat()));
				}
				break;
			}

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
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

	void Direct3D10Renderer::copyResource(Renderer::IResource& destinationResource, Renderer::IResource& sourceResource)
	{
		// Security check: Are the given resources owned by this renderer? (calls "return" in case of a mismatch)
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, destinationResource)
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, sourceResource)

		// Evaluate the render target type
		switch (destinationResource.getResourceType())
		{
			case Renderer::ResourceType::TEXTURE_2D:
				if (sourceResource.getResourceType() == Renderer::ResourceType::TEXTURE_2D)
				{
					// Get the Direct3D 10 texture 2D instances
					const Texture2D& direct3D10DestinationTexture2D = static_cast<const Texture2D&>(destinationResource);
					const Texture2D& direct3D10SourceTexture2D = static_cast<const Texture2D&>(sourceResource);

					// Copy resource, but only the top-level mipmap
					mD3D10Device->CopySubresourceRegion(direct3D10DestinationTexture2D.getD3D10Texture2D(), 0, 0, 0, 0, direct3D10SourceTexture2D.getD3D10Texture2D(), 0, nullptr);
				}
				else
				{
					// Error!
					RENDERER_ASSERT(mContext, false, "Failed to copy the Direct3D 10 resource")
				}
				break;

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
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
				// Not handled in here
				break;
		}
	}

	void Direct3D10Renderer::generateMipmaps(Renderer::IResource& resource)
	{
		// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, resource)

		RENDERER_ASSERT(mContext, resource.getResourceType() == Renderer::ResourceType::TEXTURE_2D, "TODO(co) Mipmaps can only be generated for Direct3D 10 2D texture resources")
		Texture2D& texture2D = static_cast<Texture2D&>(resource);
		mD3D10Device->GenerateMips(texture2D.getD3D10ShaderResourceView());
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void Direct3D10Renderer::setDebugMarker(const char* name)
		{
			// Create the Direct3D 9 runtime linking instance, in case there's no one, yet
			if (nullptr == mDirect3D9RuntimeLinking)
			{
				mDirect3D9RuntimeLinking = RENDERER_NEW(mContext, Direct3D9RuntimeLinking)(*this);
			}

			// Call the Direct3D 9 PIX function
			if (mDirect3D9RuntimeLinking->isDirect3D9Avaiable())
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Direct3D 10 debug marker names must not be a null pointer")
				RENDERER_ASSERT(mContext, strlen(name) < 256, "Direct3D 10 debug marker names must not have more than 255 characters")
				wchar_t unicodeName[256];
				std::mbstowcs(unicodeName, name, 256);
				D3DPERF_SetMarker(D3DCOLOR_RGBA(255, 0, 255, 255), unicodeName);
			}
		}

		void Direct3D10Renderer::beginDebugEvent(const char* name)
		{
			// Create the Direct3D 9 runtime linking instance, in case there's no one, yet
			if (nullptr == mDirect3D9RuntimeLinking)
			{
				mDirect3D9RuntimeLinking = RENDERER_NEW(mContext, Direct3D9RuntimeLinking)(*this);
			}

			// Call the Direct3D 9 PIX function
			if (mDirect3D9RuntimeLinking->isDirect3D9Avaiable())
			{
				RENDERER_ASSERT(mContext, nullptr != name, "Direct3D 10 debug event names must not be a null pointer")
				RENDERER_ASSERT(mContext, strlen(name) < 256, "Direct3D 10 debug event names must not have more than 255 characters")
				wchar_t unicodeName[256];
				std::mbstowcs(unicodeName, name, 256);
				D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 255, 255), unicodeName);
			}
		}

		void Direct3D10Renderer::endDebugEvent()
		{
			// Create the Direct3D 9 runtime linking instance, in case there's no one, yet
			if (nullptr == mDirect3D9RuntimeLinking)
			{
				mDirect3D9RuntimeLinking = RENDERER_NEW(mContext, Direct3D9RuntimeLinking)(*this);
			}

			// Call the Direct3D 9 PIX function
			if (mDirect3D9RuntimeLinking->isDirect3D9Avaiable())
			{
				D3DPERF_EndEvent();
			}
		}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	bool Direct3D10Renderer::isDebugEnabled()
	{
		// Don't check for the "RENDERER_DEBUG" preprocessor definition, even if debug
		// is disabled it has to be possible to use this function for an additional security check
		// -> Maybe a debugger/profiler ignores the debug state
		// -> Maybe someone manipulated the binary to enable the debug state, adding a second check
		//    makes it a little bit more time consuming to hack the binary :D (but of course, this is no 100% security)
		#ifdef RENDERER_DEBUG
			return (nullptr != D3DPERF_GetStatus && D3DPERF_GetStatus() != 0);
		#else
			return false;
		#endif
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t Direct3D10Renderer::getNumberOfShaderLanguages() const
	{
		uint32_t numberOfShaderLanguages = 1;	// HLSL support is always there

		// Done, return the number of supported shader languages
		return numberOfShaderLanguages;
	}

	const char* Direct3D10Renderer::getShaderLanguageName(uint32_t index) const
	{
		// HLSL supported
		if (0 == index)
		{
			return ::detail::HLSL_NAME;
		}

		// Error!
		return nullptr;
	}

	Renderer::IShaderLanguage* Direct3D10Renderer::getShaderLanguage(const char* shaderLanguageName)
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
	Renderer::IRenderPass* Direct3D10Renderer::createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples)
	{
		return RENDERER_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples);
	}

	Renderer::ISwapChain* Direct3D10Renderer::createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool)
	{
		// Sanity checks
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)
		RENDERER_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Direct3D 10: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RENDERER_NEW(mContext, SwapChain)(renderPass, windowHandle);
	}

	Renderer::IFramebuffer* Direct3D10Renderer::createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment)
	{
		// Sanity check
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)

		// Create the framebuffer
		return RENDERER_NEW(mContext, Framebuffer)(renderPass, colorFramebufferAttachments, depthStencilFramebufferAttachment);
	}

	Renderer::IBufferManager* Direct3D10Renderer::createBufferManager()
	{
		return RENDERER_NEW(mContext, BufferManager)(*this);
	}

	Renderer::ITextureManager* Direct3D10Renderer::createTextureManager()
	{
		return RENDERER_NEW(mContext, TextureManager)(*this);
	}

	Renderer::IRootSignature* Direct3D10Renderer::createRootSignature(const Renderer::RootSignature& rootSignature)
	{
		return RENDERER_NEW(mContext, RootSignature)(*this, rootSignature);
	}

	Renderer::IGraphicsPipelineState* Direct3D10Renderer::createGraphicsPipelineState(const Renderer::GraphicsPipelineState& graphicsPipelineState)
	{
		return RENDERER_NEW(mContext, GraphicsPipelineState)(*this, graphicsPipelineState);
	}

	Renderer::IComputePipelineState* Direct3D10Renderer::createComputePipelineState(Renderer::IRootSignature& rootSignature, Renderer::IComputeShader& computeShader)
	{
		// Sanity checks
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, rootSignature)
		DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, computeShader)

		// Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();

		// Error! Direct3D 10 has no compute shader support.
		return nullptr;
	}

	Renderer::ISamplerState* Direct3D10Renderer::createSamplerState(const Renderer::SamplerState& samplerState)
	{
		return RENDERER_NEW(mContext, SamplerState)(*this, samplerState);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool Direct3D10Renderer::map(Renderer::IResource& resource, uint32_t subresource, Renderer::MapType mapType, uint32_t mapFlags, Renderer::MappedSubresource& mappedSubresource)
	{
		// The "Renderer::MapType" values directly map to Direct3D 10 & 11 constants, do not change them
		// The "Renderer::MappedSubresource" structure directly maps to Direct3D 11, do not change it

		// Define helper macro
		#define TEXTURE_RESOURCE(type, typeClass, d3dClass, d3dStructure) \
			case type: \
			{ \
				bool result = false; \
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this) \
				d3dClass* d3d10Texture = nullptr; \
				d3dStructure d3d10MappedTexture; \
				static_cast<typeClass&>(resource).getD3D10ShaderResourceView()->GetResource(reinterpret_cast<ID3D10Resource**>(&d3d10Texture)); \
				if (nullptr != d3d10Texture) \
				{ \
					result = (S_OK == d3d10Texture->Map(subresource, static_cast<D3D10_MAP>(mapType), mapFlags, &d3d10MappedTexture)); \
					d3d10Texture->Release(); \
				} \
				else \
				{ \
					memset(&d3d10MappedTexture, 0, sizeof(d3dStructure)); \
				} \
				if (result) \
				{ \
					mappedSubresource.data		 = d3d10MappedTexture.pData; \
					mappedSubresource.rowPitch   = d3d10MappedTexture.RowPitch; \
					mappedSubresource.depthPitch = 0; \
				} \
				else \
				{ \
					mappedSubresource.data		 = nullptr; \
					mappedSubresource.rowPitch   = 0; \
					mappedSubresource.depthPitch = 0; \
				} \
				RENDERER_END_DEBUG_EVENT(this) \
				return result; \
			}

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (S_OK == static_cast<IndexBuffer&>(resource).getD3D10Buffer()->Map(static_cast<D3D10_MAP>(mapType), mapFlags, &mappedSubresource.data));

			case Renderer::ResourceType::VERTEX_BUFFER:
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (S_OK == static_cast<VertexBuffer&>(resource).getD3D10Buffer()->Map(static_cast<D3D10_MAP>(mapType), mapFlags, &mappedSubresource.data));

			case Renderer::ResourceType::TEXTURE_BUFFER:
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (S_OK == static_cast<TextureBuffer&>(resource).getD3D10Buffer()->Map(static_cast<D3D10_MAP>(mapType), mapFlags, &mappedSubresource.data));

			case Renderer::ResourceType::STRUCTURED_BUFFER:
				RENDERER_ASSERT(mContext, false, "Direct3D 10 has no structured buffer support")
				return false;

			case Renderer::ResourceType::INDIRECT_BUFFER:
				mappedSubresource.data		 = static_cast<IndirectBuffer&>(resource).getWritableEmulationData();
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return true;

			case Renderer::ResourceType::UNIFORM_BUFFER:
				mappedSubresource.rowPitch   = 0;
				mappedSubresource.depthPitch = 0;
				return (S_OK == static_cast<UniformBuffer&>(resource).getD3D10Buffer()->Map(static_cast<D3D10_MAP>(mapType), mapFlags, &mappedSubresource.data));

			case Renderer::ResourceType::TEXTURE_1D:
				// TODO(co) Implement Direct3D 10 1D texture
				return false;

			TEXTURE_RESOURCE(Renderer::ResourceType::TEXTURE_2D, Texture2D, ID3D10Texture2D, D3D10_MAPPED_TEXTURE2D)
			TEXTURE_RESOURCE(Renderer::ResourceType::TEXTURE_2D_ARRAY, Texture2DArray, ID3D10Texture2D, D3D10_MAPPED_TEXTURE2D)
			TEXTURE_RESOURCE(Renderer::ResourceType::TEXTURE_3D, Texture3D, ID3D10Texture3D, D3D10_MAPPED_TEXTURE3D)

			case Renderer::ResourceType::TEXTURE_CUBE:
				// TODO(co) Implement Direct3D 10 cube texture
				return false;

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
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

		// Undefine helper macro
		#undef TEXTURE_RESOURCE
	}

	void Direct3D10Renderer::unmap(Renderer::IResource& resource, uint32_t subresource)
	{
		// Define helper macro
		#define TEXTURE_RESOURCE(type, typeClass, d3dClass) \
			case type: \
			{ \
				RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this) \
				d3dClass* d3d10Texture = nullptr; \
				static_cast<typeClass&>(resource).getD3D10ShaderResourceView()->GetResource(reinterpret_cast<ID3D10Resource**>(&d3d10Texture)); \
				if (nullptr != d3d10Texture) \
				{ \
					d3d10Texture->Unmap(subresource); \
					d3d10Texture->Release(); \
				} \
				RENDERER_END_DEBUG_EVENT(this) \
				break; \
			}

		// Evaluate the resource type
		switch (resource.getResourceType())
		{
			case Renderer::ResourceType::INDEX_BUFFER:
				static_cast<IndexBuffer&>(resource).getD3D10Buffer()->Unmap();
				break;

			case Renderer::ResourceType::VERTEX_BUFFER:
				static_cast<VertexBuffer&>(resource).getD3D10Buffer()->Unmap();
				break;

			case Renderer::ResourceType::TEXTURE_BUFFER:
				static_cast<TextureBuffer&>(resource).getD3D10Buffer()->Unmap();
				break;

			case Renderer::ResourceType::STRUCTURED_BUFFER:
				RENDERER_ASSERT(mContext, false, "Direct3D 10 has no structured buffer support")
				break;

			case Renderer::ResourceType::INDIRECT_BUFFER:
				// Nothing here, it's a software emulated indirect buffer
				break;

			case Renderer::ResourceType::UNIFORM_BUFFER:
				static_cast<UniformBuffer&>(resource).getD3D10Buffer()->Unmap();
				break;

			case Renderer::ResourceType::TEXTURE_1D:
				// TODO(co) Implement Direct3D 10 1D texture
				break;

			TEXTURE_RESOURCE(Renderer::ResourceType::TEXTURE_2D, Texture2D, ID3D10Texture2D)
			TEXTURE_RESOURCE(Renderer::ResourceType::TEXTURE_2D_ARRAY, Texture2DArray, ID3D10Texture2D)
			TEXTURE_RESOURCE(Renderer::ResourceType::TEXTURE_3D, Texture3D, ID3D10Texture3D)

			case Renderer::ResourceType::TEXTURE_CUBE:
				// TODO(co) Implement Direct3D 10 cube texture
				break;

			case Renderer::ResourceType::ROOT_SIGNATURE:
			case Renderer::ResourceType::RESOURCE_GROUP:
			case Renderer::ResourceType::GRAPHICS_PROGRAM:
			case Renderer::ResourceType::VERTEX_ARRAY:
			case Renderer::ResourceType::RENDER_PASS:
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

		// Undefine helper macro
		#undef TEXTURE_RESOURCE
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool Direct3D10Renderer::beginScene()
	{
		// Not required when using Direct3D 10

		// Done
		return true;
	}

	void Direct3D10Renderer::submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer)
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

	void Direct3D10Renderer::endScene()
	{
		// We need to forget about the currently set render target
		setGraphicsRenderTarget(nullptr);
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void Direct3D10Renderer::flush()
	{
		mD3D10Device->Flush();
	}

	void Direct3D10Renderer::finish()
	{
		// Create the Direct3D 10 query instance used for flush right now?
		if (nullptr == mD3D10QueryFlush)
		{
			D3D10_QUERY_DESC d3d10QueryDesc;
			d3d10QueryDesc.Query	  = D3D10_QUERY_EVENT;
			d3d10QueryDesc.MiscFlags = 0;
			FAILED_DEBUG_BREAK(mD3D10Device->CreateQuery(&d3d10QueryDesc, &mD3D10QueryFlush));

			#ifdef RENDERER_DEBUG
				// Set the debug name
				if (nullptr != mD3D10QueryFlush)
				{
					// No need to reset the previous private data, there shouldn't be any...
					FAILED_DEBUG_BREAK(mD3D10QueryFlush->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(__FUNCTION__)), __FUNCTION__));
				}
			#endif
		}
		if (nullptr != mD3D10QueryFlush)
		{
			// Perform the flush and wait
			mD3D10QueryFlush->End();
			mD3D10Device->Flush();
			BOOL result = FALSE;
			do
			{
				// Spin-wait
				FAILED_DEBUG_BREAK(mD3D10QueryFlush->GetData(&result, sizeof(BOOL), 0));
			} while (!result);
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void Direct3D10Renderer::initializeCapabilities()
	{
		// There are no Direct3D 10 device capabilities we could query on runtime
		// -> Have a look at "Resource Limits (Direct3D 10)" at MSDN http://msdn.microsoft.com/en-us/library/cc308052%28VS.85%29.aspx
		//    for a table with a list of the minimum resources supported by Direct3D 10

		{ // Get device name
			// Get DXGI adapter
			IDXGIDevice* dxgiDevice = nullptr;
			IDXGIAdapter* dxgiAdapter = nullptr;
			FAILED_DEBUG_BREAK(mD3D10Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
			FAILED_DEBUG_BREAK(dxgiDevice->GetAdapter(&dxgiAdapter));

			// The adapter contains a description like "AMD Radeon R9 200 Series"
			DXGI_ADAPTER_DESC dxgiAdapterDesc = {};
			FAILED_DEBUG_BREAK(dxgiAdapter->GetDesc(&dxgiAdapterDesc));

			// Convert UTF-16 string to UTF-8
			const size_t numberOfCharacters = _countof(mCapabilities.deviceName) - 1;
			::WideCharToMultiByte(CP_UTF8, 0, dxgiAdapterDesc.Description, static_cast<int>(wcslen(dxgiAdapterDesc.Description)), mCapabilities.deviceName, static_cast<int>(numberOfCharacters), nullptr, nullptr);
			mCapabilities.deviceName[numberOfCharacters] = '\0';

			// Release references
			dxgiAdapter->Release();
			dxgiDevice->Release();
		}

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Renderer::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		mCapabilities.maximumNumberOfViewports = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;

		// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
		mCapabilities.maximumNumberOfSimultaneousRenderTargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;

		// Maximum texture dimension
		mCapabilities.maximumTextureDimension = 8192;

		// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
		mCapabilities.maximumNumberOf2DTextureArraySlices = 512;

		// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		mCapabilities.maximumTextureBufferSize = 128 * 1024 * 1024;	// TODO(co) http://msdn.microsoft.com/en-us/library/cc308052%28VS.85%29.aspx does not mention the texture buffer? Figure out the correct size! Currently the OpenGL 3 minimum is used: 128 MiB.

		// Direct3D 10 doesn't support structured buffer
		mCapabilities.maximumStructuredBufferSize = 0;

		// Maximum indirect buffer size in bytes
		// -> DirectX 10 has no indirect buffer
		mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		// -> See https://msdn.microsoft.com/en-us/library/windows/desktop/cc308052(v=vs.85).aspx - "Resource Limits (Direct3D 10)" - "Number of elements in a constant buffer 4096"
		// -> One element = float4 = 16 bytes
		mCapabilities.maximumUniformBufferSize = 4096 * 16;

		// Maximum number of multisamples (always at least 1, usually 8)
		// TODO(co) Currently Direct3D 10 instead of Direct3D 10.1 is used causing
		// "D3D11 ERROR: ID3D10Device::CreateTexture2D: If the feature level is less than D3D_FEATURE_LEVEL_10_1, a Texture2D with sample count > 1 cannot have both D3D11_BIND_DEPTH_STENCIL and D3D11_BIND_SHADER_RESOURCE.  This call may appear to incorrectly return success on older/current D3D runtimes due to missing validation, despite this debug layer message.  [ STATE_CREATION ERROR #99: CREATETEXTURE2D_INVALIDBINDFLAGS]"
		// error messages when trying to create a depth texture render target which one also wants to read from inside shaders. The Direct3D 10 renderer backend is still maintained for curiosity reasons,
		// but it's not really worth to put more effort into it to be able to handle the lack of certain features. So, just say this renderer backend doesn't support multisampling at all.
		mCapabilities.maximumNumberOfMultisamples = 1;

		// Maximum anisotropy (always at least 1, usually 16)
		mCapabilities.maximumAnisotropy = 16;

		// Left-handed coordinate system with clip space depth value range 0..1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = true;

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = false;

		// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		mCapabilities.instancedArrays = true;

		// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
		mCapabilities.drawInstanced = true;

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = true;

		// Direct3D 10 has native multi-threading
		// -> https://msdn.microsoft.com/de-de/library/windows/desktop/bb205068(v=vs.85).aspx - "Unlike Direct3D 9, the Direct3D 10 API defaults to fully thread-safe"
		mCapabilities.nativeMultiThreading = true;

		// Direct3D 10 has shader bytecode support
		mCapabilities.shaderBytecode = true;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		mCapabilities.maximumNumberOfPatchVertices = 0;	// Direct3D 10 has no tessellation support

		// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
		mCapabilities.maximumNumberOfGsOutputVertices = 1024;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = false;
	}

	void Direct3D10Renderer::setGraphicsProgram(Renderer::IGraphicsProgram* graphicsProgram)
	{
		// Begin debug event
		RENDERER_BEGIN_DEBUG_EVENT_FUNCTION(this)

		if (nullptr != graphicsProgram)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			DIRECT3D10RENDERER_RENDERERMATCHCHECK_ASSERT(*this, *graphicsProgram)

			// Get shaders
			const GraphicsProgramHlsl* graphicsProgramHlsl = static_cast<GraphicsProgramHlsl*>(graphicsProgram);
			const VertexShaderHlsl*	   vertexShaderHlsl	   = graphicsProgramHlsl->getVertexShaderHlsl();
			const GeometryShaderHlsl*  geometryShaderHlsl  = graphicsProgramHlsl->getGeometryShaderHlsl();
			const FragmentShaderHlsl*  fragmentShaderHlsl  = graphicsProgramHlsl->getFragmentShaderHlsl();
			ID3D10VertexShader*		   d3d10VertexShader   = (nullptr != vertexShaderHlsl)	? vertexShaderHlsl->getD3D10VertexShader()	   : nullptr;
			ID3D10GeometryShader*	   d3d10GeometryShader = (nullptr != geometryShaderHlsl) ? geometryShaderHlsl->getD3D10GeometryShader() : nullptr;
			ID3D10PixelShader*		   d3d10PixelShader    = (nullptr != fragmentShaderHlsl) ? fragmentShaderHlsl->getD3D10PixelShader()	   : nullptr;

			// Set shaders
			if (mD3d10VertexShader != d3d10VertexShader)
			{
				mD3d10VertexShader = d3d10VertexShader;
				mD3D10Device->VSSetShader(mD3d10VertexShader);
			}
			if (mD3d10GeometryShader != d3d10GeometryShader)
			{
				mD3d10GeometryShader = d3d10GeometryShader;
				mD3D10Device->GSSetShader(mD3d10GeometryShader);
			}
			if (mD3d10PixelShader != d3d10PixelShader)
			{
				mD3d10PixelShader = d3d10PixelShader;
				mD3D10Device->PSSetShader(mD3d10PixelShader);
			}
		}
		else
		{
			if (nullptr != mD3d10VertexShader)
			{
				mD3D10Device->VSSetShader(nullptr);
				mD3d10VertexShader = nullptr;
			}
			if (nullptr != mD3d10GeometryShader)
			{
				mD3D10Device->GSSetShader(nullptr);
				mD3d10GeometryShader = nullptr;
			}
			if (nullptr != mD3d10PixelShader)
			{
				mD3D10Device->PSSetShader(nullptr);
				mD3d10PixelShader = nullptr;
			}
		}

		// End debug event
		RENDERER_END_DEBUG_EVENT(this)
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Direct3D10Renderer




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RENDERER_DIRECT3D10_EXPORTS
	#define DIRECT3D10RENDERER_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define DIRECT3D10RENDERER_FUNCTION_EXPORT
#endif
DIRECT3D10RENDERER_FUNCTION_EXPORT Renderer::IRenderer* createDirect3D10RendererInstance(const Renderer::Context& context)
{
	return RENDERER_NEW(context, Direct3D10Renderer::Direct3D10Renderer(context));
}
#undef DIRECT3D10RENDERER_FUNCTION_EXPORT
