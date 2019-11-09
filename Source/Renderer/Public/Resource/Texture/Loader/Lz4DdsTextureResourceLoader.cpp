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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/Texture/Loader/Lz4DdsTextureResourceLoader.h"
#include "Renderer/Public/Resource/Texture/TextureResource.h"
#include "Renderer/Public/Core/File/IFile.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr uint32_t DDSCAPS2_CUBEMAP = 0x00000200;
		static constexpr uint32_t DDS_FOURCC = 0x00000004;
		static constexpr uint32_t DDS_LUMINANCE = 0x00020000;
		static constexpr uint32_t DDS_ALPHAPIXELS = 0x00000001;
		static constexpr uint32_t DDS_LINEARSIZE = 0x00080000;
		static constexpr uint32_t DDS_PITCH = 0x00000008;
		static constexpr uint32_t DDSD_CAPS = 0x00000001;
		static constexpr uint32_t DDSD_PIXELFORMAT = 0x00001000;
		static constexpr uint32_t DDSD_HEIGHT = 0x00000002;
		static constexpr uint32_t DDSD_WIDTH = 0x00000004;
		static constexpr uint32_t DDSD_MIPMAPCOUNT = 0x00020000;
		static constexpr uint32_t DDSD_DEPTH = 0x00800000;
		static constexpr uint32_t DDPF_FOURCC = 0x00000004;
		static constexpr uint32_t DDSCAPS_TEXTURE = 0x00001000;
		static constexpr uint32_t DDSCAPS_MIPMAP = 0x00400000;
		static constexpr uint32_t DDSCAPS_COMPLEX = 0x00000008;
		static constexpr uint32_t DDSCAPS2_VOLUME = 0x00200000;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000;
		static constexpr uint32_t DDSCAPS2_CUBEMAP_ALL_FACES = (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ);


		//[-------------------------------------------------------]
		//[ Structures                                            ]
		//[-------------------------------------------------------]
		struct DdsHeader final
		{
			uint8_t magic[4];
			uint32_t size;
			uint32_t flags;
			uint32_t height;
			uint32_t width;
			uint32_t pitchOrLinearSize;
			uint32_t depth;
			uint32_t mipMapCount;
			uint32_t reserved[11];
			struct
			{
				uint32_t size;
				uint32_t flags;
				uint32_t fourCC;
				uint32_t RGBBitCount;
				uint32_t RBitMask;
				uint32_t GBitMask;
				uint32_t BBitMask;
				uint32_t RGBAlphaBitMask;
			} ddpfPixelFormat;
			struct
			{
				uint32_t caps1;
				uint32_t caps2;
				uint32_t reserved[2];
			} ddsCaps;
			uint32_t reserved2;
		};

		struct DdsHeaderDX10 final
		{
			uint32_t DXGIFormat; // See http://msdn.microsoft.com/en-us/library/bb173059.aspx
			uint32_t resourceDimension;
			uint32_t miscFlag;
			uint32_t arraySize;
			uint32_t reserved;
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	void Lz4DdsTextureResourceLoader::onProcessing()
	{
		// Decompress LZ4 compressed data
		mMemoryFile.decompress();

		// TODO(co) Cleanup and complete, currently just a prototype
		// TODO(co) Add optional top mipmap removal support (see "Renderer::TextureResourceManager::NumberOfTopMipmapsToRemove")

		#define MCHAR4(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

		// Read the header
		::detail::DdsHeader ddsHeader;
		mMemoryFile.read(&ddsHeader, sizeof(::detail::DdsHeader));
		if (ddsHeader.magic[0] == 'D' && ddsHeader.magic[1] == 'D' && ddsHeader.magic[2] == 'S' && ddsHeader.magic[3] == ' ' &&
			// Note that if "size" is "DDS " this is not a valid dds file according
			// to the file spec. Some broken tool out there seems to produce files
			// with this value in the size field, so we support reading them...
			(ddsHeader.size == 124 || ddsHeader.size != MCHAR4('D', 'D', 'S', ' ')))
		{
			mWidth = ddsHeader.width;
			mHeight = ddsHeader.height;
			mDepth = ddsHeader.depth ? ddsHeader.depth : 1;
			mNumberOfSlices = 1;

			// Check for DX10 extension
			bool hasDX10Header = false;
			::detail::DdsHeaderDX10 ddsHeaderDX10 = {};
			if (((ddsHeader.ddpfPixelFormat.flags & ::detail::DDPF_FOURCC)) && ddsHeader.ddpfPixelFormat.fourCC == MCHAR4('D', 'X', '1', '0'))
			{
				// Read the DX10 header
				mMemoryFile.read(&ddsHeaderDX10, sizeof(::detail::DdsHeaderDX10));
				mNumberOfSlices = ddsHeaderDX10.arraySize;
				hasDX10Header = true;
			}

			// Get the color format and compression
			// TODO(co)
		//	EDataFormat  nDataFormat = DataByte;
		//	EColorFormat nColorFormat;
		//	EColorFormat nInternalColorFormat;
		//	ECompression nCompression = CompressionNone;

			// Does the data contain mipmaps?
			mDataContainsMipmaps = (ddsHeader.mipMapCount > 0);

			// Is this image compressed?
			if (ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_FOURCC)
			{
				// The image is compressed
				if (hasDX10Header)
				{
					// Get the color format and compression
					switch (ddsHeaderDX10.DXGIFormat)
					{
					// Integer
						// R8 UNORM
						case 61:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							break;

						// RG8 UNORM
						case 49:
							// TODO(co)
							// nInternalColorFormat = (ddsHeader.ddpfPixelFormat.BBitMask == 0xFF) ? ColorBGR : ColorRGB;
							// nColorFormat = ColorRGB;	// Store it as RGB
							break;

						// RGBA8 UNORM
						case 28:
							// TODO(co)
							// nInternalColorFormat = (ddsHeader.ddpfPixelFormat.BBitMask == 0xFF) ? ColorBGRA : ColorRGBA;
							// nColorFormat = ColorRGBA;
							break;

					// 16 bit float
						// R16F
						case 54:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							// nDataFormat = DataHalf;
							break;

						// RG16F
						case 34:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
							// nDataFormat = DataHalf;
							break;

						// RGBA16F
						case 10:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nDataFormat = DataHalf;
							break;

					// IEEE 32 bit float
						// R32F
						case 41:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							// nDataFormat = DataFloat;
							break;

						// RG32F
						case 16:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
							// nDataFormat = DataFloat;
							break;

						// RGB32F
						case 6:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGB;
							// nDataFormat = DataFloat;
							break;

						// RGBA32F
						case 2:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nDataFormat = DataFloat;
							break;

					// Compressed
						// DXT1 (BC1 UNORM)
						case 71:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGB;
							// nCompression = CompressionDXT1;
							break;

						// DXT3 (BC2 UNORM)
						case 74:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nCompression = CompressionDXT3;
							break;

						// DXT5 (BC3 UNORM)
						case 77:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nCompression = CompressionDXT5;
							break;

						// LATC1 (BC4 UNORM, previously known as ATI1N)
						case 80:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							// nCompression = CompressionLATC1;
							break;

						// LATC2 (BC5 UNORM, previously known as ATI2N)
						case 83:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
							// nCompression = CompressionLATC2;
							break;

						default:
							// Error!
							RHI_ASSERT(mRenderer.getContext(), false, "Unsupported format")
							return;
					}
				}
				else
				{
					switch (ddsHeader.ddpfPixelFormat.fourCC)
					{
					// 16 bit float
						// R16F
						case 111:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							// nDataFormat = DataHalf;
							break;

						// RG16F
						case 112:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
							// nDataFormat = DataHalf;
							break;

						// RGBA16F
						case 113:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nDataFormat = DataHalf;
							break;

					// IEEE 32 bit float
						// R32F
						case 114:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							// nDataFormat = DataFloat;
							break;

						// RG32F
						case 115:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
							// nDataFormat = DataFloat;
							break;

						// RGBA32F
						case 116:
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nDataFormat = DataFloat;
							break;

					// Compressed
						// DXT1 (BC1 UNORM)
						case MCHAR4('D', 'X', 'T', '1'):
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGB;
							// nCompression = CompressionDXT1;
							break;

						// DXT3 (BC2 UNORM)
						case MCHAR4('D', 'X', 'T', '3'):
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nCompression = CompressionDXT3;
							break;

						// DXT5 (BC3 UNORM)
						case MCHAR4('D', 'X', 'T', '5'):
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorRGBA;
							// nCompression = CompressionDXT5;
							break;

						// LATC1 (BC4 UNORM, previously known as ATI1N)
						case MCHAR4('A', 'T', 'I', '1'):
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscale;
							// nCompression = CompressionLATC1;
							break;

						// LATC2 (BC5 UNORM, previously known as ATI2N)
						case MCHAR4('A', 'T', 'I', '2'):
							// TODO(co)
							// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
							// nCompression = CompressionLATC2;
							break;

					// Uncompressed
						default:
							switch (ddsHeader.ddpfPixelFormat.RGBBitCount)
							{
								// R8
								case 8:
									// TODO(co)
									// nInternalColorFormat = nColorFormat = ColorGrayscale;
									break;

								// LA8
								case 16:
									if (ddsHeader.ddpfPixelFormat.RGBAlphaBitMask == 0xFF00)
									{
										// TODO(co)
										// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
									}
									else
									{
										// Error
										RHI_ASSERT(mRenderer.getContext(), false, "Unsupported format")
										return;
									}
									break;

								// RGB8
								case 24:
									// TODO(co)
									// nInternalColorFormat = (ddsHeader.ddpfPixelFormat.BBitMask == 0xFF) ? ColorBGR : ColorRGB;
									// nColorFormat         = ColorRGB;
									break;

								// RGBA8
								case 32:
									if (ddsHeader.ddpfPixelFormat.RBitMask != 0x3FF00000)
									{
										// TODO(co)
										// nInternalColorFormat = (ddsHeader.ddpfPixelFormat.BBitMask == 0xFF) ? ColorBGRA : ColorRGBA;
										// nColorFormat         = ColorRGBA;
									}
									else
									{
										// Error
										RHI_ASSERT(mRenderer.getContext(), false, "Unsupported format")
										return;
									}
									break;

								default:
									// Error
									RHI_ASSERT(mRenderer.getContext(), false, "Unsupported format")
									return;
							}
					}
				}
			}
			else
			{
				// The image is not compressed
				if (ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_LUMINANCE)
				{
					if (ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_ALPHAPIXELS)
					{
						// TODO(co)
						// nInternalColorFormat = nColorFormat = ColorGrayscaleA;
					}
					else
					{
						// TODO(co)
						// nInternalColorFormat = nColorFormat = ColorGrayscale;
					}
				}
				else
				{
					if (ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_ALPHAPIXELS)
					{
						// Set color format, please not that all bit mask relevant stuff is done inside "DecompressRGBA()"
						// TODO(co)
						// nInternalColorFormat = nColorFormat = ColorRGBA;
					}
					else
					{
						// Set color format, please not that all bit mask relevant stuff is done inside "DecompressRGBA()"
						// TODO(co)
						// nInternalColorFormat = nColorFormat = ColorRGB;
					}
				}

				// Microsoft bug, they're not following their own documentation
				if (!(ddsHeader.ddpfPixelFormat.flags & (::detail::DDS_LINEARSIZE | ::detail::DDS_PITCH)) || !ddsHeader.pitchOrLinearSize)
				{
					ddsHeader.ddpfPixelFormat.flags |= ::detail::DDS_LINEARSIZE;
				}
			}

			// Get the number of mipmaps
			const uint32_t numberOfMipmaps = (!ddsHeader.mipMapCount) ? 1 : ddsHeader.mipMapCount;

			// Cube map?
			const uint32_t numberOfFaces = (ddsHeader.ddsCaps.caps2 & ::detail::DDSCAPS2_CUBEMAP) ? 6u : 1u;

			// TODO(co) Make this dynamic
			if (1 == mWidth || 1 == mHeight)
			{
				if ((ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_LUMINANCE) != 0)
				{
					// 32-bit floating point as used for IES light profile
					mTextureFormat = Rhi::TextureFormat::R32_FLOAT;
				}
				else
				{
					// The 4x4 block size based DXT compression format has no support for 1D textures
					mTextureFormat = static_cast<uint8_t>(mTextureResource->isRgbHardwareGammaCorrection() ? Rhi::TextureFormat::R8G8B8A8_SRGB : Rhi::TextureFormat::R8G8B8A8);
				}
			}
			else
			{
				if (mDepth > 1)
				{
					mTextureFormat = (8 == ddsHeader.ddpfPixelFormat.RGBBitCount) ? Rhi::TextureFormat::R8 : Rhi::TextureFormat::R8G8B8A8;
				}
				else
				{
					if ((ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_LUMINANCE) != 0)
					{
						// 16-bit height map
						mTextureFormat = Rhi::TextureFormat::R16_UNORM;
					}
					else if (ddsHeader.ddpfPixelFormat.flags & ::detail::DDS_FOURCC)
					{
						mTextureFormat = static_cast<uint8_t>(mTextureResource->isRgbHardwareGammaCorrection() ? Rhi::TextureFormat::BC1_SRGB : Rhi::TextureFormat::BC1);
					}
					else
					{
						mTextureFormat = Rhi::TextureFormat::R8G8B8A8;
					}
				}
			}

			{ // Loop through all faces
				uint32_t width = mWidth;
				uint32_t height = mHeight;
				uint32_t depth = mDepth;
				mNumberOfUsedImageDataBytes = 0;
				if (numberOfMipmaps > 1)
				{
					// Take mipmaps into account
					while (width > 1 && height > 1 && depth > 1)
					{
						mNumberOfUsedImageDataBytes += Rhi::TextureFormat::getNumberOfBytesPerSlice(static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), width, height) * depth * mNumberOfSlices;

						width /= 2;
						height /= 2;
						depth /= 2;
					}
				}
				mNumberOfUsedImageDataBytes += Rhi::TextureFormat::getNumberOfBytesPerSlice(static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), width, height) * depth * mNumberOfSlices;

				if (mNumberOfImageDataBytes < mNumberOfUsedImageDataBytes)
				{
					mNumberOfImageDataBytes = mNumberOfUsedImageDataBytes;
					delete [] mImageData;
					mImageData = new uint8_t[mNumberOfImageDataBytes];
				}

				// Avoid slow division by using bit-shift
	//			const size_t compressedSize = ((ddsHeader.width + 3) >> 2) * ((ddsHeader.height + 3) >> 2) * 8;
				// m_nCompressedSize = ((ddsHeader.width + 3) / 4) * ((ddsHeader.height + 3) / 4) * 8;

				// Data layout
				// - DDS files are organized in face-major order, like this:
				//     Face0: Mip0, Mip1, Mip2, etc.
				//     Face1: Mip0, Mip1, Mip2, etc.
				//     etc.
				// - The RHI expects: CRN and KTX files are organized in mip-major order, like this:
				//     Mip0: Face0, Face1, Face2, Face3, Face4, Face5
				//     Mip1: Face0, Face1, Face2, Face3, Face4, Face5
				//     etc.

				// TODO(co) Data layout handling
				// A simple one: Just read in the whole compressed data
				mMemoryFile.read(mImageData, mNumberOfUsedImageDataBytes);



				//uint8_t* tempData = nullptr;	// Used when "DDS_LINEARSIZE" is set
				for (uint32_t face = 0; face < numberOfFaces; ++face)
				{
					// Load in all mipmaps
					for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
					{
					}

					// TODO(co)
					/*
					// Create image part with reasonable semantic
					ImagePart *pImagePart = cImage.CreatePart((numberOfFaces == 6) ? (static_cast<uint32_t>(ImagePartCubeSidePosX) + face) : 0);
					if (pImagePart)
					{
						// Load in all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Create image buffer
							ImageBuffer *pImageBuffer = pImagePart->CreateMipmap();
							pImageBuffer->CreateImage(nDataFormat,
														nColorFormat,
														Vector3i(ImageBuffer::GetMipmapSize(ddsHeader.nWidth,  mipmap),
																ImageBuffer::GetMipmapSize(ddsHeader.nHeight, mipmap),
																ImageBuffer::GetMipmapSize(nDepth,		   mipmap)),
														nCompression);

							// Read in compressed data?
							if (nCompression == CompressionNone)
							{
								// If the "DDS_LINEARSIZE" flag is set we need to do some more work...
								if (ddsHeader.ddpfPixelFormat.flags & DDS_LINEARSIZE)
								{
									// Calculate the current linear size
									const uint32_t pitchOrLinearSize = (pImageBuffer->GetSize().x * pImageBuffer->GetSize().y * nDepth * (ddsHeader.ddpfPixelFormat.nRGBBitCount >> 3));

									// Allocate temp data right now? (we can reuse it for the following smaller mipmaps)
									if (!tempData)
									{
										tempData = new uint8_t[pitchOrLinearSize];
									}

									// Read the data
									fread(tempData, 1, pitchOrLinearSize, file);

									// Decompress the image data
									DecompressRGBA(ddsHeader, *pImageBuffer, tempData);
								}
								else
								{
									// A simple one: Just read in the whole uncompressed data
									fread(pImageBuffer->GetData(), 1, pImageBuffer->GetDataSize(), file);
								}
							}
							else
							{
								// A simple one: Just read in the whole compressed data
								fread(pImageBuffer->GetCompressedData(), 1, pImageBuffer->GetCompressedDataSize(), file);
							}
						}
					}
				}

				// Cleanup temp data
				if (tempData)
				{
					delete [] tempData;
				}
				*/
			}

			// TODO(co)
			/*
			// Convert BGR(A) to RGB(A)
			if (nInternalColorFormat == ColorBGR)
			{
				// Loop through all faces
				for (uint32_t face = 0; face < numberOfFaces; ++face)
				{
					// Create image part with reasonable semantic
					ImagePart *pImagePart = cImage.GetPartBySemantics((numberOfFaces == 6) ? (static_cast<uint32_t>(ImagePartCubeSidePosX) + face) : 0);
					if (pImagePart)
					{
						// Load in all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Get image buffer
							ImageBuffer *pImageBuffer = pImagePart->GetMipmap(mipmap);

							// Swap R/B
							Color3::SwapRB(pImageBuffer->GetData(), pImageBuffer->GetNumOfPixels());
						}
					}
				}
			}
			else if (nInternalColorFormat == ColorBGRA)
			{
				// Loop through all faces
				for (uint32_t face = 0; face < numberOfFaces; ++face)
				{
					// Create image part with reasonable semantic
					ImagePart *pImagePart = cImage.GetPartBySemantics((numberOfFaces == 6) ? (static_cast<uint32_t>(ImagePartCubeSidePosX) + face) : 0);
					if (pImagePart)
					{
						// Load in all mipmaps
						for (uint32_t mipmap = 0; mipmap < numberOfMipmaps; ++mipmap)
						{
							// Get image buffer
							ImageBuffer *pImageBuffer = pImagePart->GetMipmap(mipmap);

							// Swap R/B
							Color4::SwapRB(pImageBuffer->GetData(), pImageBuffer->GetNumOfPixels());
						}
					}
				}
			*/
			}
		}
		else
		{
			// Error: Invalid magic number
		}

		#undef MCHAR4

		// Can we create the RHI resource asynchronous as well?
		if (mRenderer.getRhi().getCapabilities().nativeMultithreading)
		{
			mTexture = createRhiTexture();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected Renderer::ITextureResourceLoader methods    ]
	//[-------------------------------------------------------]
	Rhi::ITexture* Lz4DdsTextureResourceLoader::createRhiTexture()
	{
		const uint32_t flags = (mDataContainsMipmaps ? (Rhi::TextureFlag::DATA_CONTAINS_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE) : Rhi::TextureFlag::SHADER_RESOURCE);
		if (1 == mWidth || 1 == mHeight)
		{
			// 1D texture
			if (mNumberOfSlices > 0)
			{
				return mRenderer.getTextureManager().createTexture1DArray((1 == mWidth) ? mHeight : mWidth, mNumberOfSlices, static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), mImageData, flags, Rhi::TextureUsage::IMMUTABLE RHI_RESOURCE_DEBUG_NAME(getAsset().virtualFilename));
			}
			else
			{
				return mRenderer.getTextureManager().createTexture1D((1 == mWidth) ? mHeight : mWidth, static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), mImageData, flags, Rhi::TextureUsage::IMMUTABLE RHI_RESOURCE_DEBUG_NAME(getAsset().virtualFilename));
			}
		}
		else if (mDepth > 1)
		{
			// 3D texture
			return mRenderer.getTextureManager().createTexture3D(mWidth, mHeight, mDepth, static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), mImageData, flags, Rhi::TextureUsage::IMMUTABLE RHI_RESOURCE_DEBUG_NAME(getAsset().virtualFilename));
		}
		else
		{
			// 2D texture
			return mRenderer.getTextureManager().createTexture2D(mWidth, mHeight, static_cast<Rhi::TextureFormat::Enum>(mTextureFormat), mImageData, flags, Rhi::TextureUsage::IMMUTABLE, 1, nullptr RHI_RESOURCE_DEBUG_NAME(getAsset().virtualFilename));
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
