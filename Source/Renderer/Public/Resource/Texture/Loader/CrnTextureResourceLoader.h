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
#include "Renderer/Public/Resource/Texture/Loader/ITextureResourceLoader.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class CrnTextureResourceLoader : public ITextureResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class TextureResourceManager;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("crn");


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		[[nodiscard]] virtual bool onDeserialization(IFile& file) override;
		virtual void onProcessing() override;


	//[-------------------------------------------------------]
	//[ Protected Renderer::ITextureResourceLoader methods    ]
	//[-------------------------------------------------------]
	protected:
		[[nodiscard]] virtual Rhi::ITexture* createRhiTexture() override;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		CrnTextureResourceLoader(IResourceManager& resourceManager, IRenderer& renderer);

		inline virtual ~CrnTextureResourceLoader() override
		{
			delete [] mFileData;
			delete [] mImageData;
		}

		explicit CrnTextureResourceLoader(const CrnTextureResourceLoader&) = delete;
		CrnTextureResourceLoader& operator=(const CrnTextureResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		// Temporary data
		uint32_t mWidth;
		uint32_t mHeight;
		uint8_t  mTextureFormat;	// "Rhi::TextureFormat", don't want to include the header in here
		bool	 mCubeMap;
		bool	 mDataContainsMipmaps;

		// Temporary file data
		uint32_t mNumberOfFileDataBytes;
		uint32_t mNumberOfUsedFileDataBytes;
		uint8_t* mFileData;

		// Temporary image data
		uint32_t mNumberOfImageDataBytes;
		uint32_t mNumberOfUsedImageDataBytes;
		uint8_t* mImageData;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
