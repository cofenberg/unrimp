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
#include "RendererRuntime/Export.h"
#include "RendererRuntime/Core/GetUninitialized.h"

#include <Renderer/Renderer.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t RenderTargetTextureSignatureId;	///< Render target texture signature identifier, result of hashing render target texture properties


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Render target texture signature
	*/
	class RenderTargetTextureSignature final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline RenderTargetTextureSignature() :
			mWidth(getUninitialized<uint32_t>()),
			mHeight(getUninitialized<uint32_t>()),
			mTextureFormat(Renderer::TextureFormat::UNKNOWN),
			mAllowMultisample(false),
			mGenerateMipmaps(false),
			mAllowResolutionScale(true),
			mWidthScale(1.0f),
			mHeightScale(1.0f),
			mRenderTargetTextureSignatureId(getUninitialized<RenderTargetTextureSignatureId>())
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] width
		*    Width
		*  @param[in] height
		*    Height
		*  @param[in] textureFormat
		*    Texture format
		*  @param[in] allowMultisample
		*    Allow multisample?
		*  @param[in] generateMipmaps
		*    Generate mipmaps?
		*  @param[in] allowResolutionScale
		*    Allow resolution scale?
		*  @param[in] widthScale
		*    Width scale
		*  @param[in] heightScale
		*    Height scale
		*/
		RENDERERRUNTIME_API_EXPORT RenderTargetTextureSignature(uint32_t width, uint32_t height, Renderer::TextureFormat::Enum textureFormat, bool allowMultisample, bool generateMipmaps, bool allowResolutionScale, float widthScale, float heightScale);

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] renderTargetTextureSignature
		*    Render target texture signature to copy from
		*/
		inline explicit RenderTargetTextureSignature(const RenderTargetTextureSignature& renderTargetTextureSignature) :
			mWidth(renderTargetTextureSignature.mWidth),
			mHeight(renderTargetTextureSignature.mHeight),
			mTextureFormat(renderTargetTextureSignature.mTextureFormat),
			mAllowMultisample(renderTargetTextureSignature.mAllowMultisample),
			mGenerateMipmaps(renderTargetTextureSignature.mGenerateMipmaps),
			mAllowResolutionScale(renderTargetTextureSignature.mAllowResolutionScale),
			mWidthScale(renderTargetTextureSignature.mWidthScale),
			mHeightScale(renderTargetTextureSignature.mHeightScale),
			mRenderTargetTextureSignatureId(renderTargetTextureSignature.mRenderTargetTextureSignatureId)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~RenderTargetTextureSignature()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy operator
		*/
		inline RenderTargetTextureSignature& operator=(const RenderTargetTextureSignature& renderTargetTextureSignature)
		{
			mWidth							= renderTargetTextureSignature.mWidth;
			mHeight							= renderTargetTextureSignature.mHeight;
			mTextureFormat					= renderTargetTextureSignature.mTextureFormat;
			mAllowMultisample				= renderTargetTextureSignature.mAllowMultisample;
			mGenerateMipmaps				= renderTargetTextureSignature.mGenerateMipmaps;
			mAllowResolutionScale			= renderTargetTextureSignature.mAllowResolutionScale;
			mWidthScale						= renderTargetTextureSignature.mWidthScale;
			mHeightScale					= renderTargetTextureSignature.mHeightScale;
			mRenderTargetTextureSignatureId	= renderTargetTextureSignature.mRenderTargetTextureSignatureId;

			// Done
			return *this;
		}

		//[-------------------------------------------------------]
		//[ Getter for input data                                 ]
		//[-------------------------------------------------------]
		inline uint32_t getWidth() const
		{
			return mWidth;
		}

		inline uint32_t getHeight() const
		{
			return mHeight;
		}

		inline Renderer::TextureFormat::Enum getTextureFormat() const
		{
			return mTextureFormat;
		}

		inline bool getAllowMultisample() const
		{
			return mAllowMultisample;
		}

		inline bool getGenerateMipmaps() const
		{
			return mGenerateMipmaps;
		}

		inline bool getAllowResolutionScale() const
		{
			return mAllowResolutionScale;
		}

		inline float getWidthScale() const
		{
			return mWidthScale;
		}

		inline float getHeightScale() const
		{
			return mHeightScale;
		}

		//[-------------------------------------------------------]
		//[ Getter for derived data                               ]
		//[-------------------------------------------------------]
		inline RenderTargetTextureSignatureId getRenderTargetTextureSignatureId() const
		{
			return mRenderTargetTextureSignatureId;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Input data
		uint32_t					  mWidth;
		uint32_t					  mHeight;
		Renderer::TextureFormat::Enum mTextureFormat;
		bool						  mAllowMultisample;
		bool						  mGenerateMipmaps;
		bool						  mAllowResolutionScale;
		float						  mWidthScale;
		float						  mHeightScale;
		// Derived data
		RenderTargetTextureSignatureId mRenderTargetTextureSignatureId;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
