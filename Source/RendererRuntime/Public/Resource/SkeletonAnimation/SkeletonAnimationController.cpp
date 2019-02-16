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
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationController.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationEvaluator.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationResourceManager.h"
#include "RendererRuntime/Public/Resource/Skeleton/SkeletonResourceManager.h"
#include "RendererRuntime/Public/Resource/Skeleton/SkeletonResource.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void SkeletonAnimationController::startSkeletonAnimationByResourceId(SkeletonAnimationResourceId skeletonAnimationResourceId)
	{
		clear();
		mSkeletonAnimationResourceId = skeletonAnimationResourceId;
		if (isValid(skeletonAnimationResourceId))
		{
			mRendererRuntime.getSkeletonAnimationResourceManager().getResourceByResourceId(skeletonAnimationResourceId).connectResourceListener(*this);
		}
	}

	void SkeletonAnimationController::startSkeletonAnimationByAssetId(AssetId skeletonAnimationAssetId)
	{
		clear();
		mRendererRuntime.getSkeletonAnimationResourceManager().loadSkeletonAnimationResourceByAssetId(skeletonAnimationAssetId, mSkeletonAnimationResourceId, this);
	}

	void SkeletonAnimationController::clear()
	{
		if (isValid(mSkeletonAnimationResourceId))
		{
			disconnectFromResourceById(mSkeletonAnimationResourceId);
			setInvalid(mSkeletonAnimationResourceId);
		}
		destroySkeletonAnimationEvaluator();
		mTimeInSeconds = 0.0f;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	void SkeletonAnimationController::onLoadingStateChange(const IResource& resource)
	{
		if (resource.getLoadingState() == IResource::LoadingState::LOADED)
		{
			createSkeletonAnimationEvaluator();
		}
		else
		{
			destroySkeletonAnimationEvaluator();
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void SkeletonAnimationController::createSkeletonAnimationEvaluator()
	{
		RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr == mSkeletonAnimationEvaluator, "No useless update calls, please")
		SkeletonAnimationResourceManager& skeletonAnimationResourceManager = mRendererRuntime.getSkeletonAnimationResourceManager();
		mSkeletonAnimationEvaluator = new SkeletonAnimationEvaluator(mRendererRuntime.getContext().getAllocator(), skeletonAnimationResourceManager, mSkeletonAnimationResourceId);

		// Register skeleton animation controller
		skeletonAnimationResourceManager.mSkeletonAnimationControllers.push_back(this);
	}

	void SkeletonAnimationController::destroySkeletonAnimationEvaluator()
	{
		if (nullptr != mSkeletonAnimationEvaluator)
		{
			{ // Unregister skeleton animation controller
				SkeletonAnimationResourceManager::SkeletonAnimationControllers& skeletonAnimationControllers = mRendererRuntime.getSkeletonAnimationResourceManager().mSkeletonAnimationControllers;
				SkeletonAnimationResourceManager::SkeletonAnimationControllers::iterator iterator = std::find(skeletonAnimationControllers.begin(), skeletonAnimationControllers.end(), this);
				RENDERER_ASSERT(mRendererRuntime.getContext(), iterator != skeletonAnimationControllers.end(), "Invalid skeleton animation controller")
				skeletonAnimationControllers.erase(iterator);
			}

			// Destroy skeleton animation evaluator
			delete mSkeletonAnimationEvaluator;
			mSkeletonAnimationEvaluator = nullptr;
		}
	}

	void SkeletonAnimationController::update(float pastSecondsSinceLastFrame)
	{
		// Sanity check
		RENDERER_ASSERT(mRendererRuntime.getContext(), pastSecondsSinceLastFrame > 0.0f, "No negative time, please")
		RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != mSkeletonAnimationEvaluator, "No useless update calls, please")

		// Advance time and evaluate state
		mTimeInSeconds += pastSecondsSinceLastFrame;
		mSkeletonAnimationEvaluator->evaluate(mTimeInSeconds);

		{ // Tell the controlled skeleton resource about the new state
			SkeletonResource& skeletonResource = mRendererRuntime.getSkeletonResourceManager().getById(mSkeletonResourceId);
			const SkeletonAnimationEvaluator::BoneIds& boneIds = mSkeletonAnimationEvaluator->getBoneIds();
			const SkeletonAnimationEvaluator::TransformMatrices& transformMatrices = mSkeletonAnimationEvaluator->getTransformMatrices();
			glm::mat4* localBoneMatrices = skeletonResource.getLocalBoneMatrices();
			for (size_t i = 0; i < boneIds.size(); ++i)
			{
				const uint32_t boneIndex = skeletonResource.getBoneIndexByBoneId(boneIds[i]);
				if (isValid(boneIndex))
				{
					localBoneMatrices[boneIndex] = transformMatrices[i];
				}
			}
			skeletonResource.localToGlobalPose();
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
