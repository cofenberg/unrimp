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


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Export.h"
#include "RendererRuntime/Public/Core/GetInvalid.h"


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
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct Flag final
		{
			enum Enum
			{
				UNORDERED_ACCESS       = 1u << 0u,	///< This render target texture can be used for unordered access which is needed for compute shader read/write textures (when using Direct3D 11 a unordered access view (UAV) will be generated)
				SHADER_RESOURCE        = 1u << 1u,	///< This render target texture can be used as shader resource (when using Direct3D 11 a shader resource view (SRV) will be generated)
				RENDER_TARGET          = 1u << 2u,	///< This texture can be used as framebuffer object (FBO) attachment render target
				ALLOW_MULTISAMPLE      = 1u << 3u,	///< Allow multisample
				GENERATE_MIPMAPS       = 1u << 4u,	///< Generate mipmaps
				ALLOW_RESOLUTION_SCALE = 1u << 5u	///< Allow resolution scale
			};
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline RenderTargetTextureSignature() :
			mWidth(getInvalid<uint32_t>()),
			mHeight(getInvalid<uint32_t>()),
			mTextureFormat(Rhi::TextureFormat::UNKNOWN),
			mFlags(Flag::SHADER_RESOURCE | Flag::RENDER_TARGET | Flag::ALLOW_RESOLUTION_SCALE),
			mWidthScale(1.0f),
			mHeightScale(1.0f),
			mRenderTargetTextureSignatureId(getInvalid<RenderTargetTextureSignatureId>())
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
		*  @param[in] flags
		*    Flags (see "RendererRuntime::RenderTargetTextureSignature::Flag")
		*  @param[in] widthScale
		*    Width scale
		*  @param[in] heightScale
		*    Height scale
		*/
		RENDERERRUNTIME_API_EXPORT RenderTargetTextureSignature(uint32_t width, uint32_t height, Rhi::TextureFormat::Enum textureFormat, uint8_t flags, float widthScale, float heightScale);

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
			mFlags(renderTargetTextureSignature.mFlags),
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
			mFlags							= renderTargetTextureSignature.mFlags;
			mWidthScale						= renderTargetTextureSignature.mWidthScale;
			mHeightScale					= renderTargetTextureSignature.mHeightScale;
			mRenderTargetTextureSignatureId	= renderTargetTextureSignature.mRenderTargetTextureSignatureId;

			// Done
			return *this;
		}

		//[-------------------------------------------------------]
		//[ Getter for input data                                 ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline uint32_t getWidth() const
		{
			return mWidth;
		}

		[[nodiscard]] inline uint32_t getHeight() const
		{
			return mHeight;
		}

		[[nodiscard]] inline Rhi::TextureFormat::Enum getTextureFormat() const
		{
			return mTextureFormat;
		}

		[[nodiscard]] inline uint8_t getFlags() const
		{
			return mFlags;
		}

		[[nodiscard]] inline float getWidthScale() const
		{
			return mWidthScale;
		}

		[[nodiscard]] inline float getHeightScale() const
		{
			return mHeightScale;
		}

		//[-------------------------------------------------------]
		//[ Getter for derived data                               ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline RenderTargetTextureSignatureId getRenderTargetTextureSignatureId() const
		{
			return mRenderTargetTextureSignatureId;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Input data
		uint32_t				 mWidth;
		uint32_t				 mHeight;
		Rhi::TextureFormat::Enum mTextureFormat;
		uint8_t					 mFlags;			///< Flags (see "RendererRuntime::RenderTargetTextureSignature::Flag")
		float					 mWidthScale;
		float					 mHeightScale;
		// Derived data
		RenderTargetTextureSignatureId mRenderTargetTextureSignatureId;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
