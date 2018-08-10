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
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeResourceManager.h"
#include "RendererRuntime/Public/Resource/CompositorNode/CompositorNodeResource.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/CompositorPassFactory.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Loader/CompositorNodeResourceLoader.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceResource.h"
#include "RendererRuntime/Public/Resource/ResourceManagerTemplate.h"
#include "RendererRuntime/Public/Core/Renderer/RenderTargetTextureManager.h"
#include "RendererRuntime/Public/Core/Renderer/FramebufferManager.h"
#include "RendererRuntime/Public/Core/Renderer/RenderPassManager.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		static const RendererRuntime::CompositorPassFactory defaultCompositorPassFactory;


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void CompositorNodeResourceManager::loadCompositorNodeResourceByAssetId(AssetId assetId, CompositorNodeResourceId& compositorNodeResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, compositorNodeResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	void CompositorNodeResourceManager::setCompositorPassFactory(const ICompositorPassFactory* compositorPassFactory)
	{
		// There must always be a valid compositor pass factory instance
		mCompositorPassFactory = (nullptr != compositorPassFactory) ? compositorPassFactory : &::detail::defaultCompositorPassFactory;
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	uint32_t CompositorNodeResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& CompositorNodeResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& CompositorNodeResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* CompositorNodeResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void CompositorNodeResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		// TODO(co) Experimental implementation (take care of resource cleanup etc.)
		const uint32_t numberOfElements = mInternalResourceManager->getResources().getNumberOfElements();
		for (uint32_t i = 0; i < numberOfElements; ++i)
		{
			const CompositorNodeResource& compositorNodeResource = mInternalResourceManager->getResources().getElementByIndex(i);
			if (compositorNodeResource.getAssetId() == assetId)
			{
				CompositorNodeResourceId compositorNodeResourceId = getInvalid<CompositorNodeResourceId>();
				loadCompositorNodeResourceByAssetId(assetId, compositorNodeResourceId, nullptr, true, compositorNodeResource.getResourceLoaderTypeId());

				{ // Reload all compositor workspace resources using this compositor node resource
					CompositorWorkspaceResourceManager& compositorWorkspaceResourceManager = mRendererRuntime.getCompositorWorkspaceResourceManager();
					const uint32_t numberOfCompositorWorkspaceResources = compositorWorkspaceResourceManager.getNumberOfResources();
					for (uint32_t compositorWorkspaceResourceIndex = 0; compositorWorkspaceResourceIndex < numberOfCompositorWorkspaceResources; ++compositorWorkspaceResourceIndex)
					{
						const CompositorWorkspaceResource& compositorWorkspaceResource = compositorWorkspaceResourceManager.getByIndex(compositorWorkspaceResourceIndex);
						const CompositorWorkspaceResource::CompositorNodeAssetIds& compositorNodeAssetIds = compositorWorkspaceResource.getCompositorNodeAssetIds();
						for (AssetId currentAssetId : compositorNodeAssetIds)
						{
							if (currentAssetId == assetId)
							{
								compositorWorkspaceResourceManager.reloadResourceByAssetId(compositorWorkspaceResource.getAssetId());
								break;
							}
						}
					}
				}

				break;
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	IResourceLoader* CompositorNodeResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorNodeResourceManager::CompositorNodeResourceManager(IRendererRuntime& rendererRuntime) :
		mRendererRuntime(rendererRuntime),
		mCompositorPassFactory(&::detail::defaultCompositorPassFactory),
		mRenderTargetTextureManager(new RenderTargetTextureManager(mRendererRuntime)),
		mRenderPassManager(new RenderPassManager(mRendererRuntime.getRenderer())),
		mFramebufferManager(new FramebufferManager(*mRenderTargetTextureManager, *mRenderPassManager))
	{
		mInternalResourceManager = new ResourceManagerTemplate<CompositorNodeResource, CompositorNodeResourceLoader, CompositorNodeResourceId, 32>(rendererRuntime, *this);
	}

	CompositorNodeResourceManager::~CompositorNodeResourceManager()
	{
		delete mFramebufferManager;
		delete mRenderPassManager;
		delete mRenderTargetTextureManager;
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
