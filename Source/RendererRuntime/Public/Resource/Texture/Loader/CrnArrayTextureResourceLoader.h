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
#include "RendererRuntime/Public/Resource/Texture/Loader/ITextureResourceLoader.h"
#include "RendererRuntime/Public/Core/File/MemoryFile.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class CrnArrayTextureResourceLoader final : public ITextureResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class TextureResourceManager;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("crn_array");


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		[[nodiscard]] virtual bool onDeserialization(IFile& file) override;
		virtual void onProcessing() override;


	//[-------------------------------------------------------]
	//[ Protected RendererRuntime::ITextureResourceLoader methods ]
	//[-------------------------------------------------------]
	protected:
		[[nodiscard]] virtual Renderer::ITexture* createRendererTexture() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline CrnArrayTextureResourceLoader(IResourceManager& resourceManager, IRendererRuntime& rendererRuntime) :
			ITextureResourceLoader(resourceManager, rendererRuntime),
			mWidth(0),
			mHeight(0),
			mNumberOfSlices(0),
			mTextureFormat(0),
			mCubeMap(false),
			mDataContainsMipmaps(false),
			mNumberOfFileDataBytes(0),
			mNumberOfUsedFileDataBytes(0),
			mFileData(nullptr),
			mNumberOfImageDataBytes(0),
			mNumberOfUsedImageDataBytes(0),
			mImageData(nullptr)
		{
			// Nothing here
		}

		inline virtual ~CrnArrayTextureResourceLoader() override
		{
			delete [] mFileData;
			delete [] mImageData;
		}

		explicit CrnArrayTextureResourceLoader(const CrnArrayTextureResourceLoader&) = delete;
		CrnArrayTextureResourceLoader& operator=(const CrnArrayTextureResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct SliceFileMetadata final
		{
			const Asset& asset;
			uint32_t	 offset;
			uint32_t	 numberOfBytes;

			inline SliceFileMetadata(const Asset& _asset, uint32_t _offset, uint32_t _numberOfBytes) :
				asset(_asset),
				offset(_offset),
				numberOfBytes(_numberOfBytes)
			{};
			SliceFileMetadata& operator =(const SliceFileMetadata& sliceFile) = delete;
		};


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Temporary data
		uint32_t mWidth;
		uint32_t mHeight;
		uint32_t mNumberOfSlices;
		uint8_t  mTextureFormat;	// "Renderer::TextureFormat", don't want to include the header in here
		bool	 mCubeMap;
		bool	 mDataContainsMipmaps;

		// Temporary file data
		MemoryFile					   mMemoryFile;
		uint32_t					   mNumberOfFileDataBytes;
		uint32_t					   mNumberOfUsedFileDataBytes;
		uint8_t*					   mFileData;
		std::vector<AssetId>		   mAssetIds;
		std::vector<SliceFileMetadata> mSliceFileMetadata;

		// Temporary image data
		uint32_t mNumberOfImageDataBytes;
		uint32_t mNumberOfUsedImageDataBytes;
		uint8_t* mImageData;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
