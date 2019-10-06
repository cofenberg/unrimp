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
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationResourceManager.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationResource.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationController.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/Loader/SkeletonAnimationResourceLoader.h"
#include "RendererRuntime/Public/Resource/ResourceManagerTemplate.h"
#include "RendererRuntime/Public/Core/Time/TimeManager.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	SkeletonAnimationResource* SkeletonAnimationResourceManager::getSkeletonAnimationResourceByAssetId(AssetId assetId) const
	{
		return mInternalResourceManager->getResourceByAssetId(assetId);
	}

	void SkeletonAnimationResourceManager::loadSkeletonAnimationResourceByAssetId(AssetId assetId, SkeletonAnimationResourceId& skeletonAnimationResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, skeletonAnimationResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	SkeletonAnimationResourceId SkeletonAnimationResourceManager::createSkeletonAnimationResourceByAssetId(AssetId assetId)
	{
		SkeletonAnimationResource& skeletonAnimationResource = mInternalResourceManager->createEmptyResourceByAssetId(assetId);
		setResourceLoadingState(skeletonAnimationResource, IResource::LoadingState::LOADED);
		return skeletonAnimationResource.getId();
	}

	void SkeletonAnimationResourceManager::setInvalidResourceId(SkeletonAnimationResourceId& skeletonAnimationResourceId, IResourceListener& resourceListener) const
	{
		SkeletonAnimationResource* skeletonAnimationResource = tryGetById(skeletonAnimationResourceId);
		if (nullptr != skeletonAnimationResource)
		{
			skeletonAnimationResource->disconnectResourceListener(resourceListener);
		}
		setInvalid(skeletonAnimationResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	uint32_t SkeletonAnimationResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& SkeletonAnimationResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& SkeletonAnimationResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* SkeletonAnimationResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void SkeletonAnimationResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		return mInternalResourceManager->reloadResourceByAssetId(assetId);
	}

	void SkeletonAnimationResourceManager::update()
	{
		// Update skeleton animation controllers
		const float pastSecondsSinceLastFrame = mInternalResourceManager->getRendererRuntime().getTimeManager().getPastSecondsSinceLastFrame();
		for (SkeletonAnimationController* skeletonAnimationController : mSkeletonAnimationControllers)
		{
			skeletonAnimationController->update(pastSecondsSinceLastFrame);
		}
	}


	//[-------------------------------------------------------]
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	IResourceLoader* SkeletonAnimationResourceManager::createResourceLoaderInstance([[maybe_unused]] ResourceLoaderTypeId resourceLoaderTypeId)
	{
		// We only support our own format
		RENDERER_ASSERT(mInternalResourceManager->getRendererRuntime().getContext(), resourceLoaderTypeId == SkeletonAnimationResourceLoader::TYPE_ID, "Invalid resource loader type ID")
		#ifdef RENDERER_DEBUG
			return new SkeletonAnimationResourceLoader(mInternalResourceManager->getResourceManager(), mInternalResourceManager->getRendererRuntime());
		#else
			return new SkeletonAnimationResourceLoader(mInternalResourceManager->getResourceManager());
		#endif
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	SkeletonAnimationResourceManager::SkeletonAnimationResourceManager(IRendererRuntime& rendererRuntime)
	{
		mInternalResourceManager = new ResourceManagerTemplate<SkeletonAnimationResource, SkeletonAnimationResourceLoader, SkeletonAnimationResourceId, 2048>(rendererRuntime, *this);
	}

	SkeletonAnimationResourceManager::~SkeletonAnimationResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
