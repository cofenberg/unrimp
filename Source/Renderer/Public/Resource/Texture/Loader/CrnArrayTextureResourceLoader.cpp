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
#include "Renderer/Public/Resource/Texture/Loader/CrnArrayTextureResourceLoader.h"
#include "Renderer/Public/Resource/Texture/Loader/CrnArrayFileFormat.h"
#include "Renderer/Public/Resource/Texture/TextureResourceManager.h"
#include "Renderer/Public/Resource/Texture/TextureResource.h"
#include "Renderer/Public/Core/File/IFile.h"
#include "Renderer/Public/IRenderer.h"

#ifndef RENDERER_CRN_INCLUDED
	#define RENDERER_CRN_INCLUDED

	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4018)	// warning C4018: '<': signed/unsigned mismatch
		PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'cCRNFmtTotal' in switch of enum 'crn_format' is not explicitly handled by a case label
		PRAGMA_WARNING_DISABLE_MSVC(4242)	// warning C4242: '=': conversion from 'crnd::uint32' to 'crnd::uint8', possible loss of data
		PRAGMA_WARNING_DISABLE_MSVC(4244)	// warning C4244: 'initializing': conversion from 'crnd::uint' to 'crnd::uint8', possible loss of data
		PRAGMA_WARNING_DISABLE_MSVC(4302)	// warning C4302: 'type cast': truncation from 'crnd::uint8 *' to 'crnd::uint32'
		PRAGMA_WARNING_DISABLE_MSVC(4311)	// warning C4311: 'type cast': pointer truncation from 'crnd::uint8 *' to 'crnd::uint32'
		PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '<x>': conversion from '<y>' to '<z>', signed/unsigned mismatch
		PRAGMA_WARNING_DISABLE_MSVC(4499)	// warning C4499: 'static': an explicit specialization cannot have a storage class (ignored)
		PRAGMA_WARNING_DISABLE_MSVC(4548)	// warning C4548: expression before comma has no effect; expected expression with side-effect
		PRAGMA_WARNING_DISABLE_MSVC(4555)	// warning C4555: expression has no effect; expected expression with side-effect
		PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '<x>' is not defined as a preprocessor macro, replacing with '<y>' for '<z>'
		PRAGMA_WARNING_DISABLE_MSVC(5220)	// warning C5220: 'crnlib::spinlock::m_flag': a non-static data member with a volatile qualified type no longer implies
		PRAGMA_WARNING_DISABLE_CLANG("-Wunused-value")	// warning: expression result unused [-Wunused-value]
		PRAGMA_WARNING_DISABLE_CLANG("-Warray-bounds")	// warning: array index 1 is past the end of the array (which contains 1 element) [-Warray-bounds]
		PRAGMA_WARNING_DISABLE_GCC("-Wunused-value")	// warning: expression result unused [-Wunused-value]
			#include <crn_decomp.h>
			#include <dds_defs.h>
		PRAGMA_WARNING_POP
#endif

#include <algorithm>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	bool CrnArrayTextureResourceLoader::onDeserialization(IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data
		if (mMemoryFile.loadLz4CompressedDataFromFile(v1CrnArray::FORMAT_TYPE, v1CrnArray::FORMAT_VERSION, file))
		{
			// Decompress LZ4 compressed data
			mMemoryFile.decompress();

			// Read CRN array
			mMemoryFile.read(&mNumberOfSlices, sizeof(uint32_t));
			RHI_ASSERT(mRenderer.getContext(), mNumberOfSlices > 0, "Invalid number of slices")
			mAssetIds.resize(mNumberOfSlices);
			mMemoryFile.read(mAssetIds.data(), sizeof(AssetId) * mNumberOfSlices);

			// Get the accumulated file size
			const AssetManager& assetManager = mRenderer.getAssetManager();
			const IFileManager& fileManager = mRenderer.getFileManager();
			mNumberOfUsedFileDataBytes = 0;
			mSliceFileMetadata.clear();
			mSliceFileMetadata.reserve(mNumberOfSlices);
			for (uint32_t i = 0; i < mNumberOfSlices; ++i)
			{
				const Asset& asset = assetManager.getAssetByAssetId(mAssetIds[i]);	// TODO(co) Usually considered to be multithreading safe, but better review this
				const int64_t fileSize = fileManager.getFileSize(asset.virtualFilename);
				RHI_ASSERT(mRenderer.getContext(), fileSize > 0, "Invalid file size")
				mSliceFileMetadata.emplace_back(asset, mNumberOfUsedFileDataBytes, static_cast<uint32_t>(fileSize));
				mNumberOfUsedFileDataBytes += static_cast<uint32_t>(fileSize);
			}

			// Load the source image file into memory: Get file size and file data
			if (mNumberOfFileDataBytes < mNumberOfUsedFileDataBytes)
			{
				mNumberOfFileDataBytes = mNumberOfUsedFileDataBytes;
				delete [] mFileData;
				mFileData = new uint8_t[mNumberOfFileDataBytes];
			}
			for (const SliceFileMetadata& sliceFileMetadata : mSliceFileMetadata)
			{
				IFile* sliceFile = fileManager.openFile(IFileManager::FileMode::READ, sliceFileMetadata.asset.virtualFilename);
				if (nullptr != sliceFile)
				{
					sliceFile->read(mFileData + sliceFileMetadata.offset, sliceFileMetadata.numberOfBytes);
					fileManager.closeFile(*sliceFile);
				}
				else
				{
					// Error! This is horrible, now we've got a zombie inside the resource streamer. We could let it crash, but maybe the zombie won't directly eat brains.
					RHI_ASSERT(mRenderer.getContext(), false, "We should never end up in here")
				}
			}

			// Done
			return true;
		}

		// Error!
		return false;
	}

	void CrnArrayTextureResourceLoader::onProcessing()
	{
		// TODO(co) Error handling

		// The first slice is used as the master which determines the texture properties like the texture format
		const SliceFileMetadata& masterSliceFileMetadata = mSliceFileMetadata[0];

		// Decompress/transcode CRN to DDS
		crnd::crn_texture_info masterCrnTextureInfo;
		if (!crnd::crnd_get_texture_info(mFileData + masterSliceFileMetadata.offset, masterSliceFileMetadata.numberOfBytes, &masterCrnTextureInfo))
		{
			RHI_ASSERT(mRenderer.getContext(), false, "crnd_get_texture_info() failed")
			return;
		}
		mWidth  = masterCrnTextureInfo.m_width;
		mHeight = masterCrnTextureInfo.m_height;
		const uint32_t numberOfFaces = masterCrnTextureInfo.m_faces;
		mCubeMap = (numberOfFaces > 1);

		// Sanity check
		RHI_ASSERT(mRenderer.getContext(), !mCubeMap || mWidth == mHeight, "The width and height of a cube map must be identical")

		// Get the RHI texture format
		switch (masterCrnTextureInfo.m_format)
		{
			// DXT1 compression (known as BC1 in DirectX 10, RGB compression: 8:1, 8 bytes per block)
			case cCRNFmtDXT1:
				mTextureFormat = static_cast<uint8_t>(mTextureResource->isRgbHardwareGammaCorrection() ? Rhi::TextureFormat::BC1_SRGB : Rhi::TextureFormat::BC1);
				break;

			// DXT3 compression (known as BC2 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
			case cCRNFmtDXT3:
				mTextureFormat = static_cast<uint8_t>(mTextureResource->isRgbHardwareGammaCorrection() ? Rhi::TextureFormat::BC2_SRGB : Rhi::TextureFormat::BC2);
				break;

			// DXT5 compression (known as BC3 in DirectX 10, RGBA compression: 4:1, 16 bytes per block)
			case cCRNFmtDXT5:
			case cCRNFmtDXT5_CCxY:
			case cCRNFmtDXT5_xGxR:
			case cCRNFmtDXT5_xGBR:
			case cCRNFmtDXT5_AGBR:
				mTextureFormat = static_cast<uint8_t>(mTextureResource->isRgbHardwareGammaCorrection() ? Rhi::TextureFormat::BC3_SRGB : Rhi::TextureFormat::BC3);
				break;

			// 2 component texture compression (luminance & alpha compression 4:1 -> normal map compression, also known as 3DC/ATI2N, known as BC5 in DirectX 10, 16 bytes per block)
			case cCRNFmtDXN_XY:
			case cCRNFmtDXN_YX:
				mTextureFormat = Rhi::TextureFormat::BC5;
				break;

			case cCRNFmtDXT5A:
			case cCRNFmtETC1:
			case cCRNFmtETC2:
			case cCRNFmtETC2A:
			case cCRNFmtETC1S:
			case cCRNFmtETC2AS:
			case cCRNFmtTotal:
			case cCRNFmtForceDWORD:
			default:
				// Error!
				// TODO(co)
				RHI_ASSERT(mRenderer.getContext(), false, "Invalid format")
				return;
		}

		// Does the data contain mipmaps?
		const uint32_t numberOfLevels = masterCrnTextureInfo.m_levels;
		mDataContainsMipmaps = (numberOfLevels > 1);

		crnd::crnd_unpack_context crndUnpackContext = crnd::crnd_unpack_begin(mFileData + masterSliceFileMetadata.offset, masterSliceFileMetadata.numberOfBytes);
		if (nullptr == crndUnpackContext)
		{
			RHI_ASSERT(mRenderer.getContext(), false, "crnd_unpack_begin() failed")
			return;
		}

		// Handle optional top mipmap removal
		// TODO(co) Possible optimization of optional top mipmap removal: Don't load in the skipped mipmaps into memory in the first place ("mFileData")
		int startLevelIndex = mRenderer.getTextureResourceManager().getNumberOfTopMipmapsToRemove();
		if (startLevelIndex >= static_cast<int>(numberOfLevels))
		{
			startLevelIndex = static_cast<int>(numberOfLevels) - 1;
		}

		// Optional top mipmap removal security checks
		// -> Ensure we don't go below 4x4 to not get into troubles with 4x4 blocked based compression
		// -> Ensure the base mipmap we tell the RHI about is a multiple of four. Even if the original base mipmap is a multiple of four, one of the lower mipmaps might not be.
		while (startLevelIndex > 0 && (std::max(1U, mWidth >> startLevelIndex) < 4 || std::max(1U, mHeight >> startLevelIndex) < 4))
		{
			--startLevelIndex;
		}
		while (startLevelIndex > 0 && (0 != (std::max(1U, mWidth >> startLevelIndex) % 4) || (0 != std::max(1U, mHeight >> startLevelIndex) % 4)))
		{
			--startLevelIndex;
		}

		// Allocate resulting image data
		const crn_uint32 numberOfBytesPerDxtBlock = crnd::crnd_get_bytes_per_dxt_block(masterCrnTextureInfo.m_format);
		{
			mNumberOfUsedImageDataBytes = 0;
			for (crn_uint32 levelIndex = static_cast<crn_uint32>(startLevelIndex); levelIndex < numberOfLevels; ++levelIndex)
			{
				const crn_uint32 width = std::max(1U, mWidth >> levelIndex);
				const crn_uint32 height = std::max(1U, mHeight >> levelIndex);
				const crn_uint32 blocksX = std::max(1U, (width + 3) >> 2);
				const crn_uint32 blocksY = std::max(1U, (height + 3) >> 2);
				const crn_uint32 rowPitch = blocksX * numberOfBytesPerDxtBlock;
				const crn_uint32 totalFaceSize = rowPitch * blocksY;
				mNumberOfUsedImageDataBytes += totalFaceSize;
			}
			mNumberOfUsedImageDataBytes *= numberOfFaces * mNumberOfSlices;

			if (mNumberOfImageDataBytes < mNumberOfUsedImageDataBytes)
			{
				mNumberOfImageDataBytes = mNumberOfUsedImageDataBytes;
				delete [] mImageData;
				mImageData = new uint8_t[mNumberOfImageDataBytes];
			}
		}

		// Data layout: The RHI expects: CRN and KTX files are organized in mip-major order, like this:
		//   Mip0: Face0, Face1, Face2, Face3, Face4, Face5
		//   Mip1: Face0, Face1, Face2, Face3, Face4, Face5
		//   etc.

		// Now transcode all face and mipmap levels into memory, one mip level at a time
		for (uint32_t sliceIndex = 0; sliceIndex < mNumberOfSlices; ++sliceIndex)
		{
			// Start CRN unpack context
			if (sliceIndex > 0)
			{
				const SliceFileMetadata& sliceFileMetadata = mSliceFileMetadata[sliceIndex];

				// Ensure the texture data matches the master texture data
				#ifdef RHI_DEBUG
				{
					crnd::crn_texture_info crnTextureInfo;
					if (!crnd::crnd_get_texture_info(mFileData + sliceFileMetadata.offset, sliceFileMetadata.numberOfBytes, &crnTextureInfo))
					{
						RHI_ASSERT(mRenderer.getContext(), false, "crnd_get_texture_info() failed")
						return;
					}
					RHI_ASSERT(mRenderer.getContext(), memcmp(&masterCrnTextureInfo, &crnTextureInfo, sizeof(crnd::crn_texture_info)) == 0, "CRN texture information mismatch")
				}
				#endif

				// Start CRN unpack context
				crndUnpackContext = crnd::crnd_unpack_begin(mFileData + sliceFileMetadata.offset, sliceFileMetadata.numberOfBytes);
				if (nullptr == crndUnpackContext)
				{
					RHI_ASSERT(mRenderer.getContext(), false, "crnd_unpack_begin() failed")
					return;
				}
			}

			// Transcode slice
			void* decompressedImages[cCRNMaxFaces];
			uint8_t* currentImageData = mImageData;
			for (crn_uint32 levelIndex = static_cast<crn_uint32>(startLevelIndex); levelIndex < numberOfLevels; ++levelIndex)
			{
				// Compute the face's width, height, number of DXT blocks per row/col, etc.
				const crn_uint32 width = std::max(1U, mWidth >> levelIndex);
				const crn_uint32 height = std::max(1U, mHeight >> levelIndex);
				const crn_uint32 blocksX = std::max(1U, (width + 3) >> 2);
				const crn_uint32 blocksY = std::max(1U, (height + 3) >> 2);
				const crn_uint32 rowPitch = blocksX * numberOfBytesPerDxtBlock;
				const crn_uint32 totalFaceSize = rowPitch * blocksY;

				// Update the face pointer array needed by "crnd_unpack_level()"
				for (crn_uint32 faceIndex = 0; faceIndex < numberOfFaces; ++faceIndex)
				{
					decompressedImages[faceIndex] = currentImageData + totalFaceSize * sliceIndex;
					currentImageData += totalFaceSize * mNumberOfSlices;
				}

				// Now transcode the level to raw DXTn
				if (!crnd::crnd_unpack_level(crndUnpackContext, decompressedImages, totalFaceSize, rowPitch, levelIndex))
				{
					// Free allocated memory
					crnd::crnd_unpack_end(crndUnpackContext);
					RHI_ASSERT(mRenderer.getContext(), false, "Failed transcoding texture")
					return;
				}
			}

			// Free allocated memory
			crnd::crnd_unpack_end(crndUnpackContext);
		}

		// In case we removed top level mipmaps, we need to update the texture dimension
		if (0 != startLevelIndex)
		{
			mWidth = std::max(1U, mWidth >> startLevelIndex);
			mHeight = std::max(1U, mHeight >> startLevelIndex);
		}

		// Can we create the RHI resource asynchronous as well?
		if (mRenderer.getRhi().getCapabilities().nativeMultithreading)
		{
			mTexture = createRhiTexture();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected Renderer::ITextureResourceLoader methods    ]
	//[-------------------------------------------------------]
	Rhi::ITexture* CrnArrayTextureResourceLoader::createRhiTexture()
	{
		const uint32_t flags = (mDataContainsMipmaps ? (Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE) : Rhi::TextureFlag::SHADER_RESOURCE);
		if (mCubeMap)
		{
			// TODO(co) Cube array texture
			RHI_ASSERT(mRenderer.getContext(), false, "Renderer CRN array cube texture isn't implemented, yet")
			return nullptr;
		}
		else if (1 == mWidth || 1 == mHeight)
		{
			// TODO(co) 1D array texture
			RHI_ASSERT(mRenderer.getContext(), false, "Renderer CRN array 1D texture isn't implemented, yet")
			return nullptr;
		}
		else
		{
			// 2D texture array
			return mRenderer.getTextureManager().createTexture2DArray(mWidth, mHeight, mNumberOfSlices, static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), mImageData, flags, Rhi::TextureUsage::IMMUTABLE RHI_RESOURCE_DEBUG_NAME(getAsset().virtualFilename));
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
