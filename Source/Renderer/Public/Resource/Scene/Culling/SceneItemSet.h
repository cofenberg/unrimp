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


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4100)	// warning C4100: 'address': unreferenced formal parameter
	PRAGMA_WARNING_DISABLE_MSVC(4242)	// warning C4242: '=': conversion from 'int' to 'T', possible loss of data
	PRAGMA_WARNING_DISABLE_MSVC(4244)	// warning C4244: '=': conversion from 'int' to 'T', possible loss of data
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: 'xsimd::hadd::<unnamed-tag>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'uint32_t' to 'int32_t', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4505)	// warning C4505: 'xsimd::detail::__ieee754_rem_pio2': unreferenced local function has been removed
	PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#define XSIMD_INSTR_SET_NOT_AVAILABLE 0	// warning C4668: 'XSIMD_INSTR_SET_NOT_AVAILABLE' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#define XSIMD_FORCE_X86_INSTR_SET XSIMD_X86_SSE4_2_VERSION	// TODO(co) How to use xsimd correctly to get rid of errors like "error C2440: 'initializing': cannot convert from 'xsimd::simd_batch_traits<xsimd::batch<float,8>>::batch_bool_type' to 'xsimd::batch_bool<float,4>'" when using "Advanced Vector Extensions 2 (/arch:AVX2)"?
	#include <xsimd/xsimd.hpp>
PRAGMA_WARNING_POP

#include <vector>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ISceneItem;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Scene item set
	*
	*  @note
	*    - Basing on "The Implementation of Frustum Culling in Stingray" - http://bitsquid.blogspot.de/2016/10/the-implementation-of-frustum-culling.html
	*/
	struct SceneItemSet final
	{


		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		typedef std::vector<float, xsimd::aligned_allocator<float, XSIMD_DEFAULT_ALIGNMENT>>			 FloatVector;
		typedef std::vector<double, xsimd::aligned_allocator<double, XSIMD_DEFAULT_ALIGNMENT>>			 DoubleVector;
		typedef std::vector<uint32_t, xsimd::aligned_allocator<uint32_t, XSIMD_DEFAULT_ALIGNMENT>>		 IntegerVector;
		typedef std::vector<ISceneItem*, xsimd::aligned_allocator<ISceneItem*, XSIMD_DEFAULT_ALIGNMENT>> SceneItemVector;	// TODO(co) No raw pointers here (no smart pointers either, handles please)


		//[-------------------------------------------------------]
		//[ Public data                                           ]
		//[-------------------------------------------------------]
		// Minimum object space bounding box corner position
		FloatVector minimumX;
		FloatVector minimumY;
		FloatVector minimumZ;

		// Maximum object space bounding box corner position
		FloatVector maximumX;
		FloatVector maximumY;
		FloatVector maximumZ;

		// Object space to world space matrix
		// TODO(co) Add 64 bit world space position support
		FloatVector worldXX;
		FloatVector worldXY;
		FloatVector worldXZ;
		FloatVector worldXW;
		FloatVector worldYX;
		FloatVector worldYY;
		FloatVector worldYZ;
		FloatVector worldYW;
		FloatVector worldZX;
		FloatVector worldZY;
		FloatVector worldZZ;
		FloatVector worldZW;
		FloatVector worldWX;
		FloatVector worldWY;
		FloatVector worldWZ;
		FloatVector worldWW;

		// 32 bit world space position center of bounding sphere (the bounding sphere isn't always at the object center, so we need to store this beside the transform position)
		// TODO(co) Add 64 bit world space position support
		FloatVector spherePositionX;
		FloatVector spherePositionY;
		FloatVector spherePositionZ;

		// Negative world space radius of bounding sphere, the bounding sphere radius is enclosing the bounding box (don't use the inner bounding box radius)
		FloatVector negativeRadius;

		// Flag to indicate if an object is culled or not
		IntegerVector visibilityFlag;

		// The type and ID of an object
		SceneItemVector sceneItemVector;

		uint32_t numberOfSceneItems = 0;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
