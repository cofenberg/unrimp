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


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	// Rigid skeleton file format content:
	// - Skeleton header
	// - Skeleton data block
	namespace v1Skeleton
	{


		//[-------------------------------------------------------]
		//[ Definitions                                           ]
		//[-------------------------------------------------------]
		static constexpr uint32_t FORMAT_TYPE	 = STRING_ID("Skeleton");
		static constexpr uint32_t FORMAT_VERSION = 1;

		#pragma pack(push)
		#pragma pack(1)
			struct Header final
			{
				uint32_t formatType;
				uint32_t formatVersion;
				// TODO(co) Right now, there's no standalone skeleton asset, only the skeleton which is part of a mesh. When there's one, don't forget to use LZ4 compression.
			};
		#pragma pack(pop)


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // v1Skeleton
} // RendererRuntime
