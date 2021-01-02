/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Renderer/Public/Core/Platform/PlatformTypes.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	// Rigid skeleton animation tracks file format content:
	// - File format header
	// - Skeleton animation header
	// - Bone IDs
	// - ACL ( https://github.com/nfrechette/acl ) compressed skeleton animation tracks
	namespace v1SkeletonAnimation
	{


		//[-------------------------------------------------------]
		//[ Definitions                                           ]
		//[-------------------------------------------------------]
		static constexpr uint32_t FORMAT_TYPE	 = STRING_ID("SkeletonAnimation");
		static constexpr uint32_t FORMAT_VERSION = 3;

		#pragma pack(push)
		#pragma pack(1)
			struct SkeletonAnimationHeader final
			{
				uint8_t  numberOfChannels;			///< The number of bone animation channels; each channel affects a single bone
				float	 durationInTicks;			///< Duration of the animation in ticks
				float	 ticksPerSecond;			///< Ticks per second; 0 if not specified in the imported file
				uint32_t aclCompressedTracksSize;	///< ACL compressed tracks size in bytes
			};
		#pragma pack(pop)


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // v1SkeletonAnimation
} // Renderer
