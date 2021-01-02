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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Core/Renderer/FramebufferSignature.h"
#include "Renderer/Public/Core/Math/Math.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	FramebufferSignature::FramebufferSignature(uint8_t numberOfColorAttachments, const FramebufferSignatureAttachment colorFramebufferSignatureAttachments[8], const FramebufferSignatureAttachment& depthStencilFramebufferSignatureAttachment) :
		mNumberOfColorAttachments(numberOfColorAttachments),
		mColorFramebufferSignatureAttachments{colorFramebufferSignatureAttachments[0], colorFramebufferSignatureAttachments[1], colorFramebufferSignatureAttachments[2], colorFramebufferSignatureAttachments[3], colorFramebufferSignatureAttachments[4], colorFramebufferSignatureAttachments[5], colorFramebufferSignatureAttachments[6], colorFramebufferSignatureAttachments[7]},
		mDepthStencilFramebufferSignatureAttachment(depthStencilFramebufferSignatureAttachment),
		mFramebufferSignatureId(Math::FNV1a_INITIAL_HASH_32)
	{
		mFramebufferSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mNumberOfColorAttachments), sizeof(uint32_t), mFramebufferSignatureId);
		for (uint8_t i = 0; i < mNumberOfColorAttachments; ++i)
		{
			mFramebufferSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mColorFramebufferSignatureAttachments[i]), sizeof(FramebufferSignatureAttachment), mFramebufferSignatureId);
		}
		mFramebufferSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mDepthStencilFramebufferSignatureAttachment), sizeof(FramebufferSignatureAttachment), mFramebufferSignatureId);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
