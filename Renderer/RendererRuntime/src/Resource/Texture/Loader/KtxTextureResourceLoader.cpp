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
#include "RendererRuntime/Resource/Texture/Loader/KtxTextureResourceLoader.h"
#include "RendererRuntime/Resource/Texture/TextureResource.h"
#include "RendererRuntime/Core/File/IFile.h"
#include "RendererRuntime/IRendererRuntime.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		// These codes are basing on https://github.com/KhronosGroup/KTX/tree/master/lib


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		// From "gl.h"
		#define GL_TEXTURE_1D		0x0DE0
		#define GL_TEXTURE_2D		0x0DE1
		#define GL_TEXTURE_3D		0x806F
		#define GL_TEXTURE_CUBE_MAP	0x8513
		#define GL_RGBA8			0x8058

		// From "gl2ext.h"
		#define GL_ETC1_RGB8_OES 0x8D64

		// From "khrplatform.h"
		typedef uint32_t khronos_uint32_t;

		// From https://github.com/KhronosGroup/KTX/tree/master/lib
		#define KTX_IDENTIFIER_REF	{ 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
		#define KTX_ENDIAN_REF		0x04030201
		#define KTX_ENDIAN_REF_REV	0x01020304

		#pragma pack(push)
		#pragma pack(1)
			/**
			*  @brief
			*    Read in the image header
			*/
			struct KtxHeader final
			{
				uint8_t  identifier[12];
				uint32_t endianness;
				uint32_t glType;
				uint32_t glTypeSize;
				uint32_t glFormat;
				uint32_t glInternalFormat;
				uint32_t glBaseInternalFormat;
				uint32_t pixelWidth;
				uint32_t pixelHeight;
				uint32_t pixelDepth;
				uint32_t numberOfArrayElements;
				uint32_t numberOfFaces;
				uint32_t numberOfMipmapLevels;
				uint32_t bytesOfKeyValueData;
			};
		#pragma pack(pop)

		// TODO(sw) cleanup and simplify it when possible
		struct KTX_texinfo final
		{
			// Data filled in by _ktxCheckHeader()
			uint32_t textureDimensions;
			uint32_t glTarget;
			uint32_t compressed;
			uint32_t generateMipmaps;
		};


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void ktxSwapEndian16(uint16_t* data, uint32_t count)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint16_t x = *data;
				*data++ = static_cast<uint16_t>((x << 8) | (x >> 8));
			}
		}

		/**
		*  @brief
		*    SwapEndian32: Swap endianness in an array of 32-bit values
		*/
		void ktxSwapEndian32(uint32_t* data, uint32_t count)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				const uint32_t x = *data;
				*data++ = (x << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | (x >> 24);
			}
		}

		bool checkHeader(KtxHeader& ktxHeader, KTX_texinfo& ktxTexinfo)
		{
			{ // Compare identifier, is this a KTX file?
				const uint8_t identifierReference[12] = KTX_IDENTIFIER_REF;
				if (memcmp(ktxHeader.identifier, identifierReference, 12) != 0)
				{
					// KTX_UNKNOWN_FILE_FORMAT
					return false;
				}
			}

			if (KTX_ENDIAN_REF_REV == ktxHeader.endianness)
			{
				// Convert endianness of header fields
				ktxSwapEndian32(&ktxHeader.glType, 12);
				if (1 != ktxHeader.glTypeSize && 2 != ktxHeader.glTypeSize && 4 != ktxHeader.glTypeSize)
				{
					// Only 8-, 16-, and 32-bit types supported so far
					// KTX_INVALID_VALUE
					return false;
				}
			}
			else if (KTX_ENDIAN_REF != ktxHeader.endianness)
			{
				// KTX_INVALID_VALUE
				return false;
			}

			// Check "glType" and "glFormat"
			ktxTexinfo.compressed = 0;
			if (0 == ktxHeader.glType || 0 == ktxHeader.glFormat)
			{
				if ((ktxHeader.glType + ktxHeader.glFormat) != 0)
				{
					// Either both or none of glType, glFormat must be zero
					// KTX_INVALID_VALUE
					return false;
				}
				ktxTexinfo.compressed = 1;
			}

			// Check texture dimensions. KTX files can store 8 types of textures:
			// 1D, 2D, 3D, cube, and array variants of these. There is currently no GL extension for 3D array textures.
			if ((0 == ktxHeader.pixelWidth) || (ktxHeader.pixelDepth > 0 && 0 == ktxHeader.pixelHeight))
			{
				// Texture must have width
				// Texture must have height if it has depth
				// KTX_INVALID_VALUE
				return false;
			}

			ktxTexinfo.textureDimensions = 1;
			ktxTexinfo.glTarget			 = GL_TEXTURE_1D;
			ktxTexinfo.generateMipmaps	 = 0;
			if (ktxHeader.pixelHeight > 0)
			{
				ktxTexinfo.textureDimensions = 2;
				ktxTexinfo.glTarget			 = GL_TEXTURE_2D;
			}
			if (ktxHeader.pixelDepth > 0)
			{
				ktxTexinfo.textureDimensions = 3;
				ktxTexinfo.glTarget			 = GL_TEXTURE_3D;
			}

			if (6 == ktxHeader.numberOfFaces)
			{
				if (2 == ktxTexinfo.textureDimensions)
				{
					ktxTexinfo.glTarget = GL_TEXTURE_CUBE_MAP;
				}
				else
				{
					// KTX_INVALID_VALUE
					return false;
				}
			}
			else if (1 != ktxHeader.numberOfFaces)
			{
				// "numberOfFaces" must be either 1 or 6
				// KTX_INVALID_VALUE
				return false;
			}

			// Check number of mipmap levels
			if (0 == ktxHeader.numberOfMipmapLevels)
			{
				ktxTexinfo.generateMipmaps = 1;
				ktxHeader.numberOfMipmapLevels = 1;
			}

			// This test works for arrays too because height or depth will be 0
			const khronos_uint32_t maximumDimension = std::max(std::max(ktxHeader.pixelWidth, ktxHeader.pixelHeight), ktxHeader.pixelDepth);
			if (maximumDimension < (1u << (ktxHeader.numberOfMipmapLevels - 1)))
			{
				// Can't have more mip levels than 1 + log2(max(width, height, depth))
				// KTX_INVALID_VALUE
				return false;
			}

			// Done
			return true;
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	void KtxTextureResourceLoader::onDeserialization(IFile& file)
	{
		// TODO(co) Add optional top mipmap removal support (see "RendererRuntime::TextureResourceManager::NumberOfTopMipmapsToRemove")
		// TODO(co) Add support for 3D textures (if supported by the KTX format)
		// TODO(co) Add support for array textures (if supported by the KTX format)

		// KTX header
		::detail::KtxHeader ktxHeader;
		file.read(&ktxHeader, sizeof(::detail::KtxHeader));

		{ // KTX texture information
			::detail::KTX_texinfo ktxTexinfo;
			if (!checkHeader(ktxHeader, ktxTexinfo))
			{
				assert(false && "KTX header invalid");
			}
		}
		file.skip(ktxHeader.bytesOfKeyValueData);

		// Texture dimension
		mWidth  = ktxHeader.pixelWidth;
		mHeight = ktxHeader.pixelHeight;

		// Check if the file contains data for one texture or for 6 textures
		if (1 != ktxHeader.numberOfFaces && 6 != ktxHeader.numberOfFaces)
		{
			assert(false && "Don't support more then one faces or exactly 6 faces");
		}

		// Texture format
		if (0 == ktxHeader.glFormat)
		{
			// When "glFormat == 0" -> compressed version
			// -> For now we only support ETC1 compression
			if (GL_ETC1_RGB8_OES != ktxHeader.glInternalFormat)
			{
				assert(false && "Unsupported compressed \"glInternalFormat\"");
			}
			mTextureFormat = Renderer::TextureFormat::ETC1;
		}
		else
		{
			// "glInternalFormat" defines the format of the texture
			// -> For now we only support R8G8B8A8
			if (GL_RGBA8 == ktxHeader.glInternalFormat)
			{
				mTextureFormat = Renderer::TextureFormat::R8G8B8A8;
			}
			else
			{
				assert(false && "Unsupported uncompressed \"glInternalFormat\"");
			}
		}

		// Does the data contain mipmaps?
		mDataContainsMipmaps = (ktxHeader.numberOfMipmapLevels > 1);
		mCubeMap			 = (ktxHeader.numberOfFaces > 1);

		// Get the size of the compressed image
		mNumberOfUsedImageDataBytes = 0;
		{
			uint32_t width  = mWidth;
			uint32_t height = mHeight;
			for (uint32_t mipmap = 0; mipmap < ktxHeader.numberOfMipmapLevels; ++mipmap)
			{
				for (uint32_t face = 0; face < ktxHeader.numberOfFaces; ++face)
				{
					if (GL_ETC1_RGB8_OES == ktxHeader.glInternalFormat)
					{
						mNumberOfUsedImageDataBytes += std::max((width * height) >> 1, 8u);
					}
					else if (GL_RGBA8 == ktxHeader.glInternalFormat)
					{
						mNumberOfUsedImageDataBytes += (width * height * 4);
					}
				}
				width = Renderer::ITexture::getHalfSize(width);
				height = Renderer::ITexture::getHalfSize(height);
			}
		}
		if (mNumberOfImageDataBytes < mNumberOfUsedImageDataBytes)
		{
			mNumberOfImageDataBytes = mNumberOfUsedImageDataBytes;
			delete [] mImageData;
			mImageData = new uint8_t[mNumberOfImageDataBytes];
		}

		// Data layout: The renderer interface expects: CRN and KTX files are organized in mip-major order, like this:
		//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
		//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
		//   etc.

		// Load in the image data
		uint8_t* currentImageData = mImageData;
		uint32_t width = mWidth;
		uint32_t height = mHeight;
		for (uint32_t mipmap = 0; mipmap < ktxHeader.numberOfMipmapLevels; ++mipmap)
		{
			uint32_t imageSize = 0;
			file.read(&imageSize, sizeof(uint32_t));

			// Perform endianness conversion on image size data
			if (KTX_ENDIAN_REF_REV == ktxHeader.endianness)
			{
				::detail::ktxSwapEndian32(&imageSize, 1);
			}

			for (uint32_t face = 0; face < ktxHeader.numberOfFaces; ++face)
			{
				// Read the image data per face
				file.read(currentImageData, imageSize);

				// Perform endianness conversion on texture data
				if (KTX_ENDIAN_REF_REV == ktxHeader.endianness && 2 == ktxHeader.glTypeSize)
				{
					::detail::ktxSwapEndian16(reinterpret_cast<uint16_t*>(currentImageData), imageSize / 2);
				}
				else if (KTX_ENDIAN_REF_REV == ktxHeader.endianness && 4 == ktxHeader.glTypeSize)
				{
					::detail::ktxSwapEndian32(reinterpret_cast<uint32_t*>(currentImageData), imageSize / 4);
				}

				// Move on to the next face of the current mipmap
				currentImageData += imageSize;
			}

			// An mipmap level data might have padding bytes (up to 3) formula from https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
			const uint32_t paddingBytes = 3 - ((imageSize + 3) % 4);
			file.skip(paddingBytes);

			// Move on to the next mipmap
			width = Renderer::ITexture::getHalfSize(width);
			height = Renderer::ITexture::getHalfSize(height);
		}

		// Can we create the renderer resource asynchronous as well?
		if (mRendererRuntime.getRenderer().getCapabilities().nativeMultiThreading)
		{
			mTexture = createRendererTexture();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected RendererRuntime::ITextureResourceLoader methods ]
	//[-------------------------------------------------------]
	Renderer::ITexture* KtxTextureResourceLoader::createRendererTexture()
	{
		Renderer::ITexture* texture = nullptr;
		if (mCubeMap)
		{
			// Cube texture
			texture = mRendererRuntime.getTextureManager().createTextureCube(mWidth, mHeight, static_cast<Renderer::TextureFormat::Enum>(mTextureFormat), mImageData, mDataContainsMipmaps ? Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS : 0u, Renderer::TextureUsage::IMMUTABLE);
		}
		else if (1 == mWidth || 1 == mHeight)
		{
			// 1D texture
			texture = mRendererRuntime.getTextureManager().createTexture1D((1 == mWidth) ? mHeight : mWidth, static_cast<Renderer::TextureFormat::Enum>(mTextureFormat), mImageData, mDataContainsMipmaps ? Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS : 0u, Renderer::TextureUsage::IMMUTABLE);
		}
		else
		{
			// 2D texture
			texture = mRendererRuntime.getTextureManager().createTexture2D(mWidth, mHeight, static_cast<Renderer::TextureFormat::Enum>(mTextureFormat), mImageData, mDataContainsMipmaps ? Renderer::TextureFlag::DATA_CONTAINS_MIPMAPS : 0u, Renderer::TextureUsage::IMMUTABLE);
		}
		RENDERER_SET_RESOURCE_DEBUG_NAME(texture, getAsset().virtualFilename)
		return texture;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
