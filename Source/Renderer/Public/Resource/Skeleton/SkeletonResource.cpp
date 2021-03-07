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
#include "Renderer/Public/Resource/Skeleton/SkeletonResource.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(5214)	// warning C5214: applying '*=' to an operand with a volatile qualified type is deprecated in C++20 (compiling source file E:\private\unrimp\Source\RendererToolkit\Private\AssetCompiler\TextureAssetCompiler.cpp)
	#include <glm/glm.hpp>
	#include <glm/gtx/dual_quaternion.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	uint32_t SkeletonResource::getBoneIndexByBoneId(uint32_t boneId) const
	{
		// TODO(co) Maybe it makes sense to store the bone IDs in some order to speed up the following
		for (uint8_t boneIndex = 0; boneIndex < mNumberOfBones; ++boneIndex)
		{
			if (mBoneIds[boneIndex] == boneId)
			{
				return boneIndex;
			}
		}
		return getInvalid<uint32_t>();
	}

	void SkeletonResource::localToGlobalPose()
	{
		// The root has no parent
		mGlobalBoneMatrices[0] = mLocalBoneMatrices[0];

		// Due to cache friendly depth-first rolled up bone hierarchy, the global parent bone pose is already up-to-date
		// TODO(co) Ensure that in the end SIMD intrinsics in GLM are used in here
		for (uint8_t i = 1; i < mNumberOfBones; ++i)
		{
			mGlobalBoneMatrices[i] = mGlobalBoneMatrices[mBoneParentIndices[i]] * mLocalBoneMatrices[i];
		}

		/*
		{ // Linear blend skinning (LBS) using matrices; there's no runtime switch by intent since dual quaternion skinning (DQS) is the way to go, don't remove this reference comment
			glm::mat3x4* boneSpaceMatrices = reinterpret_cast<glm::mat3x4*>(mBoneSpaceData);
			for (uint8_t i = 0; i < mNumberOfBones; ++i)
			{
				boneSpaceMatrices[i] = glm::transpose(mGlobalBoneMatrices[i] * mBoneOffsetMatrices[i]);
			}
		}
		*/

		{ // The dual quaternion skinning (DQS) implementation is basing on https://gamedev.stackexchange.com/questions/164423/help-with-dual-quaternion-skinning
			glm::dualquat* boneSpaceDualQuaternions = reinterpret_cast<glm::dualquat*>(mBoneSpaceData);
			for (uint8_t i = 0; i < mNumberOfBones; ++i)
			{
				const glm::mat4 boneSpaceMatrix = mGlobalBoneMatrices[i] * mBoneOffsetMatrices[i];
				const glm::quat rotationQuaternion = glm::quat_cast(boneSpaceMatrix);
				const glm::vec4& translation = boneSpaceMatrix[3];
				glm::dualquat& boneSpaceDualQuaternion = boneSpaceDualQuaternions[i];
				boneSpaceDualQuaternion[0] = rotationQuaternion;
				boneSpaceDualQuaternion[1] = glm::quat(0.0f, translation.x, translation.y, translation.z) * rotationQuaternion * 0.5f;
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
