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
#include "RendererRuntime/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h"
#include "RendererRuntime/Resource/Scene/Loader/SceneFileFormat.h"
#include "RendererRuntime/Resource/Scene/SceneResource.h"
#include "RendererRuntime/Resource/SkeletonAnimation/SkeletonAnimationController.h"
#include "RendererRuntime/Resource/Mesh/MeshResourceManager.h"
#include "RendererRuntime/Resource/Mesh/MeshResource.h"
#include "RendererRuntime/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	SkeletonResourceId SkeletonMeshSceneItem::getSkeletonResourceId() const
	{
		const MeshResource* meshResource = getSceneResource().getRendererRuntime().getMeshResourceManager().tryGetById(getMeshResourceId());
		return (nullptr != meshResource) ? meshResource->getSkeletonResourceId() : getInvalid<SkeletonResourceId>();
	}


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void SkeletonMeshSceneItem::deserialize(uint32_t numberOfBytes, const uint8_t* data)
	{
		// Sanity check
		assert(sizeof(v1Scene::SkeletonMeshItem));
		const v1Scene::SkeletonMeshItem* skeletonMeshItem = reinterpret_cast<const v1Scene::SkeletonMeshItem*>(data);

		// Read data
		mSkeletonAnimationAssetId = skeletonMeshItem->skeletonAnimationAssetId;

		// Call base implementation
		MeshSceneItem::deserialize(numberOfBytes - sizeof(v1Scene::SkeletonMeshItem), data + sizeof(v1Scene::SkeletonMeshItem));
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	SkeletonMeshSceneItem::~SkeletonMeshSceneItem()
	{
		// Destroy the skeleton animation controller instance, if needed
		delete mSkeletonAnimationController;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	void SkeletonMeshSceneItem::onLoadingStateChange(const IResource& resource)
	{
		// Create/destroy skeleton animation controller
		if (resource.getId() == getMeshResourceId())
		{
			if (resource.getLoadingState() == IResource::LoadingState::LOADED)
			{
				if (isValid(mSkeletonAnimationAssetId))
				{
					assert(nullptr == mSkeletonAnimationController);
					mSkeletonAnimationController = new SkeletonAnimationController(getSceneResource().getRendererRuntime(), static_cast<const MeshResource&>(resource).getSkeletonResourceId());
					mSkeletonAnimationController->startSkeletonAnimationByAssetId(mSkeletonAnimationAssetId);
				}
			}
			else if (nullptr != mSkeletonAnimationController)
			{
				delete mSkeletonAnimationController;
				mSkeletonAnimationController = nullptr;
			}
		}

		// Call the base implementation
		MeshSceneItem::onLoadingStateChange(resource);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
