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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/PrecompiledHeader.h"
#include "RendererRuntime/Core/Renderer/RenderTargetTextureSignature.h"
#include "RendererRuntime/Core/Math/Math.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	RenderTargetTextureSignature::RenderTargetTextureSignature(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, bool allowMultisample, bool generateMipmaps, bool allowResolutionScale, float widthScale, float heightScale) :
		mWidth(width),
		mHeight(height),
		mTextureFormat(textureFormat),
		mAllowMultisample(allowMultisample),
		mGenerateMipmaps(generateMipmaps),
		mAllowResolutionScale(allowResolutionScale),
		mWidthScale(widthScale),
		mHeightScale(heightScale),
		mRenderTargetTextureSignatureId(Math::FNV1a_INITIAL_HASH_32)
	{
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mWidth), sizeof(uint32_t), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mHeight), sizeof(uint32_t), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mTextureFormat), sizeof(Renderer::TextureFormat::Enum), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mAllowMultisample), sizeof(bool), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mGenerateMipmaps), sizeof(bool), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mWidthScale), sizeof(float), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mHeightScale), sizeof(float), mRenderTargetTextureSignatureId);
		mRenderTargetTextureSignatureId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&mAllowResolutionScale), sizeof(bool), mRenderTargetTextureSignatureId);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
