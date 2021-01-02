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
#include "Renderer/Public/Resource/Skeleton/SkeletonResourceManager.h"
#include "Renderer/Public/Resource/Skeleton/SkeletonResource.h"
#include "Renderer/Public/Resource/Skeleton/Loader/SkeletonResourceLoader.h"
#include "Renderer/Public/Resource/ResourceManagerTemplate.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	SkeletonResource* SkeletonResourceManager::getSkeletonResourceByAssetId(AssetId assetId) const
	{
		return mInternalResourceManager->getResourceByAssetId(assetId);
	}

	void SkeletonResourceManager::loadSkeletonResourceByAssetId(AssetId assetId, SkeletonResourceId& skeletonResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, skeletonResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	SkeletonResourceId SkeletonResourceManager::createSkeletonResourceByAssetId(AssetId assetId)
	{
		SkeletonResource& skeletonResource = mInternalResourceManager->createEmptyResourceByAssetId(assetId);
		setResourceLoadingState(skeletonResource, IResource::LoadingState::LOADED);
		return skeletonResource.getId();
	}

	void SkeletonResourceManager::setInvalidResourceId(SkeletonResourceId& skeletonResourceId, IResourceListener& resourceListener) const
	{
		SkeletonResource* skeletonResource = tryGetById(skeletonResourceId);
		if (nullptr != skeletonResource)
		{
			skeletonResource->disconnectResourceListener(resourceListener);
		}
		setInvalid(skeletonResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceManager methods     ]
	//[-------------------------------------------------------]
	uint32_t SkeletonResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& SkeletonResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& SkeletonResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* SkeletonResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void SkeletonResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		return mInternalResourceManager->reloadResourceByAssetId(assetId);
	}

	void SkeletonResourceManager::update()
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Private virtual Renderer::IResourceManager methods    ]
	//[-------------------------------------------------------]
	IResourceLoader* SkeletonResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	SkeletonResourceManager::SkeletonResourceManager(IRenderer& renderer)
	{
		mInternalResourceManager = new ResourceManagerTemplate<SkeletonResource, SkeletonResourceLoader, SkeletonResourceId, 2048>(renderer, *this);
	}

	SkeletonResourceManager::~SkeletonResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
