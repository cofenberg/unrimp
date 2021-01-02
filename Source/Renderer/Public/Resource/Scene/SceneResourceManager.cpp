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
#include "Renderer/Public/Resource/Scene/SceneResourceManager.h"
#include "Renderer/Public/Resource/Scene/SceneResource.h"
#include "Renderer/Public/Resource/Scene/Factory/SceneFactory.h"
#include "Renderer/Public/Resource/Scene/Loader/SceneResourceLoader.h"
#include "Renderer/Public/Resource/ResourceManagerTemplate.h"


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
		static const Renderer::SceneFactory defaultSceneFactory;


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
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void SceneResourceManager::setSceneFactory(const ISceneFactory* sceneFactory)
	{
		// There must always be a valid scene factory instance
		mSceneFactory = (nullptr != sceneFactory) ? sceneFactory : &::detail::defaultSceneFactory;

		// Tell the scene resource instances about the new scene factory in town
		const uint32_t numberOfElements = mInternalResourceManager->getResources().getNumberOfElements();
		for (uint32_t i = 0; i < numberOfElements; ++i)
		{
			mInternalResourceManager->getResources().getElementByIndex(i).mSceneFactory = mSceneFactory;
		}
	}

	SceneResource* SceneResourceManager::getSceneResourceByAssetId(AssetId assetId) const
	{
		return mInternalResourceManager->getResourceByAssetId(assetId);
	}

	SceneResourceId SceneResourceManager::getSceneResourceIdByAssetId(AssetId assetId) const
	{
		const SceneResource* sceneResource = getSceneResourceByAssetId(assetId);
		return (nullptr != sceneResource) ? sceneResource->getId() : getInvalid<SceneResourceId>();
	}

	void SceneResourceManager::loadSceneResourceByAssetId(AssetId assetId, SceneResourceId& sceneResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, sceneResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	void SceneResourceManager::destroySceneResource(SceneResourceId sceneResourceId)
	{
		mInternalResourceManager->getResources().removeElement(sceneResourceId);
	}

	void SceneResourceManager::setInvalidResourceId(SceneResourceId& sceneResourceId, IResourceListener& resourceListener) const
	{
		SceneResource* sceneResource = tryGetById(sceneResourceId);
		if (nullptr != sceneResource)
		{
			sceneResource->disconnectResourceListener(resourceListener);
		}
		setInvalid(sceneResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceManager methods     ]
	//[-------------------------------------------------------]
	uint32_t SceneResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& SceneResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& SceneResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* SceneResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void SceneResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		SceneResource* sceneResource = mInternalResourceManager->getResourceByAssetId(assetId);
		if (nullptr != sceneResource)
		{
			sceneResource->destroyAllSceneNodesAndItems();
		}
		return mInternalResourceManager->reloadResourceByAssetId(assetId);
	}


	//[-------------------------------------------------------]
	//[ Private virtual Renderer::IResourceManager methods    ]
	//[-------------------------------------------------------]
	IResourceLoader* SceneResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	SceneResourceManager::SceneResourceManager(IRenderer& renderer) :
		mRenderer(renderer),
		mSceneFactory(&::detail::defaultSceneFactory)
	{
		mInternalResourceManager = new ResourceManagerTemplate<SceneResource, SceneResourceLoader, SceneResourceId, 16>(renderer, *this);
	}

	SceneResourceManager::~SceneResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
