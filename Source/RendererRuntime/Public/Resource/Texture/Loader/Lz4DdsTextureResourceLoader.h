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
#include "RendererRuntime/Public/Resource/Texture/Loader/DdsTextureResourceLoader.h"
#include "RendererRuntime/Public/Core/File/MemoryFile.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class Lz4DdsTextureResourceLoader final : public DdsTextureResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class TextureResourceManager;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID		 = STRING_ID("lz4dds");
		static constexpr uint32_t FORMAT_TYPE	 = TYPE_ID;
		static constexpr uint32_t FORMAT_VERSION = 1;


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		[[nodiscard]] inline virtual bool onDeserialization(IFile& file) override
		{
			// Tell the memory mapped file about the LZ4 compressed data
			return mMemoryFile.loadLz4CompressedDataFromFile(FORMAT_TYPE, FORMAT_VERSION, file);
		}

		inline virtual void onProcessing() override
		{
			// Decompress LZ4 compressed data
			mMemoryFile.decompress();

			// Call the deserialization base implementation
			[[maybe_unused]] const bool result = DdsTextureResourceLoader::onDeserialization(mMemoryFile);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline Lz4DdsTextureResourceLoader(IResourceManager& resourceManager, IRendererRuntime& rendererRuntime) :
			DdsTextureResourceLoader(resourceManager, rendererRuntime)
		{
			// Nothing here
		}

		inline virtual ~Lz4DdsTextureResourceLoader() override
		{
			// Nothing here
		}

		explicit Lz4DdsTextureResourceLoader(const Lz4DdsTextureResourceLoader&) = delete;
		Lz4DdsTextureResourceLoader& operator=(const Lz4DdsTextureResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Temporary data
		MemoryFile mMemoryFile;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
