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
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceResource.h"
#include "Renderer/Public/Resource/CompositorWorkspace/Loader/CompositorWorkspaceResourceLoader.h"
#include "Renderer/Public/Core/Renderer/RenderPassManager.h"
#include "Renderer/Public/Core/Renderer/FramebufferManager.h"
#include "Renderer/Public/Core/Renderer/RenderTargetTextureManager.h"
#include "Renderer/Public/Resource/ResourceManagerTemplate.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void CompositorWorkspaceResourceManager::loadCompositorWorkspaceResourceByAssetId(AssetId assetId, CompositorWorkspaceResourceId& compositorWorkspaceResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, compositorWorkspaceResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	void CompositorWorkspaceResourceManager::setInvalidResourceId(CompositorWorkspaceResourceId& compositorWorkspaceResourceId, IResourceListener& resourceListener) const
	{
		CompositorWorkspaceResource* compositorWorkspaceResource = tryGetById(compositorWorkspaceResourceId);
		if (nullptr != compositorWorkspaceResource)
		{
			compositorWorkspaceResource->disconnectResourceListener(resourceListener);
		}
		setInvalid(compositorWorkspaceResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceManager methods     ]
	//[-------------------------------------------------------]
	uint32_t CompositorWorkspaceResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& CompositorWorkspaceResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& CompositorWorkspaceResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* CompositorWorkspaceResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void CompositorWorkspaceResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		return mInternalResourceManager->reloadResourceByAssetId(assetId);
	}


	//[-------------------------------------------------------]
	//[ Private virtual Renderer::IResourceManager methods    ]
	//[-------------------------------------------------------]
	IResourceLoader* CompositorWorkspaceResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorWorkspaceResourceManager::CompositorWorkspaceResourceManager(IRenderer& renderer) :
		mRenderTargetTextureManager(new RenderTargetTextureManager(renderer)),
		mRenderPassManager(new RenderPassManager(renderer.getRhi())),
		mFramebufferManager(new FramebufferManager(*mRenderTargetTextureManager, *mRenderPassManager))
	{
		mInternalResourceManager = new ResourceManagerTemplate<CompositorWorkspaceResource, CompositorWorkspaceResourceLoader, CompositorWorkspaceResourceId, 32>(renderer, *this);
	}

	CompositorWorkspaceResourceManager::~CompositorWorkspaceResourceManager()
	{
		delete mFramebufferManager;
		delete mRenderPassManager;
		delete mRenderTargetTextureManager;
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
