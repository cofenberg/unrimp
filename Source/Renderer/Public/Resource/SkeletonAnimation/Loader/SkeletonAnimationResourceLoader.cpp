/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Renderer/Public/Resource/SkeletonAnimation/Loader/SkeletonAnimationResourceLoader.h"
#include "Renderer/Public/Resource/SkeletonAnimation/Loader/SkeletonAnimationFileFormat.h"
#include "Renderer/Public/Resource/SkeletonAnimation/SkeletonAnimationResource.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	void SkeletonAnimationResourceLoader::initialize(const Asset& asset, bool reload, IResource& resource)
	{
		IResourceLoader::initialize(asset, reload);
		mSkeletonAnimationResource = static_cast<SkeletonAnimationResource*>(&resource);
	}

	bool SkeletonAnimationResourceLoader::onDeserialization(IFile& file)
	{
		// Read in the file format header
		struct FileFormatHeader final
		{
			uint32_t formatType;
			uint32_t formatVersion;
		};
		FileFormatHeader fileFormatHeader;
		file.read(&fileFormatHeader, sizeof(FileFormatHeader));
		if (v1SkeletonAnimation::FORMAT_TYPE == fileFormatHeader.formatType && v1SkeletonAnimation::FORMAT_VERSION == fileFormatHeader.formatVersion)
		{
			// Read in the skeleton animation header
			v1SkeletonAnimation::SkeletonAnimationHeader skeletonAnimationHeader;
			file.read(&skeletonAnimationHeader, sizeof(v1SkeletonAnimation::SkeletonAnimationHeader));
			mSkeletonAnimationResource->mNumberOfChannels = skeletonAnimationHeader.numberOfChannels;
			mSkeletonAnimationResource->mDurationInTicks  = skeletonAnimationHeader.durationInTicks;
			mSkeletonAnimationResource->mTicksPerSecond   = skeletonAnimationHeader.ticksPerSecond;

			// Sanity checks
			RHI_ASSERT(mRenderer.getContext(), skeletonAnimationHeader.numberOfChannels > 0, "Invalid skeleton animation asset with zero channels detected")
			RHI_ASSERT(mRenderer.getContext(), skeletonAnimationHeader.aclCompressedTracksSize > 0, "Invalid skeleton animation asset with zero ACL compressed tracks size detected")

			// Read in bone IDs
			mSkeletonAnimationResource->mBoneIds.resize(skeletonAnimationHeader.numberOfChannels);
			file.read(mSkeletonAnimationResource->mBoneIds.data(), sizeof(uint32_t) * skeletonAnimationHeader.numberOfChannels);

			// Read in the ACL ( https://github.com/nfrechette/acl ) compressed skeleton animation tracks
			mSkeletonAnimationResource->mAclCompressedTracks.resize(skeletonAnimationHeader.aclCompressedTracksSize);
			file.read(mSkeletonAnimationResource->mAclCompressedTracks.data(), skeletonAnimationHeader.aclCompressedTracksSize);

			// Done
			return true;
		}
		else
		{
			// Error!
			return false;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
