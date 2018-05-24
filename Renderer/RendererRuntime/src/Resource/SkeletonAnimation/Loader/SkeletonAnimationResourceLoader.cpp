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
#include "RendererRuntime/Resource/SkeletonAnimation/Loader/SkeletonAnimationResourceLoader.h"
#include "RendererRuntime/Resource/SkeletonAnimation/Loader/SkeletonAnimationFileFormat.h"
#include "RendererRuntime/Resource/SkeletonAnimation/SkeletonAnimationResource.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	void SkeletonAnimationResourceLoader::initialize(const Asset& asset, bool reload, IResource& resource)
	{
		IResourceLoader::initialize(asset, reload);
		mSkeletonAnimationResource = static_cast<SkeletonAnimationResource*>(&resource);
	}

	void SkeletonAnimationResourceLoader::onDeserialization(IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data
		mMemoryFile.loadLz4CompressedDataFromFile(v1SkeletonAnimation::FORMAT_TYPE, v1SkeletonAnimation::FORMAT_VERSION, file);
	}

	void SkeletonAnimationResourceLoader::onProcessing()
	{
		// Decompress LZ4 compressed data
		mMemoryFile.decompress();

		// Read in the skeleton animation header
		v1SkeletonAnimation::SkeletonAnimationHeader skeletonAnimationHeader;
		mMemoryFile.read(&skeletonAnimationHeader, sizeof(v1SkeletonAnimation::SkeletonAnimationHeader));
		mSkeletonAnimationResource->mNumberOfChannels = skeletonAnimationHeader.numberOfChannels;
		mSkeletonAnimationResource->mDurationInTicks  = skeletonAnimationHeader.durationInTicks;
		mSkeletonAnimationResource->mTicksPerSecond   = skeletonAnimationHeader.ticksPerSecond;

		// Sanity checks
		assert((skeletonAnimationHeader.numberOfChannels > 0) && "Invalid skeleton animation asset with zero channels detected");
		assert((skeletonAnimationHeader.numberOfChannelDataBytes > 0) && "Invalid skeleton animation asset with zero channel data bytes detected");

		// Read in the channel byte offsets
		mSkeletonAnimationResource->mChannelByteOffsets.resize(skeletonAnimationHeader.numberOfChannels);
		mMemoryFile.read(mSkeletonAnimationResource->mChannelByteOffsets.data(), sizeof(uint32_t) * mSkeletonAnimationResource->mChannelByteOffsets.size());

		// Read in the data of all bone channels in one big chunk
		mSkeletonAnimationResource->mChannelData.resize(skeletonAnimationHeader.numberOfChannelDataBytes);
		mMemoryFile.read(mSkeletonAnimationResource->mChannelData.data(), sizeof(uint8_t) * mSkeletonAnimationResource->mChannelData.size());

		// That's all folks. There are no more memory allocations to see here. Please go on.
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
