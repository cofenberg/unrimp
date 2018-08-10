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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/ResourceManager.h"

#include <cassert>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class ISceneFactory;
	class SceneResource;
	class IRendererRuntime;
	class SceneResourceLoader;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t SceneResourceId;	///< POD scene resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class SceneResourceManager final : public ResourceManager<SceneResource>
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererRuntimeImpl;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline IRendererRuntime& getRendererRuntime() const
		{
			return mRendererRuntime;
		}

		inline const ISceneFactory& getSceneFactory() const
		{
			// We know that this pointer is always valid
			assert(nullptr != mSceneFactory);
			return *mSceneFactory;
		}

		RENDERERRUNTIME_API_EXPORT void setSceneFactory(const ISceneFactory* sceneFactory);
		RENDERERRUNTIME_API_EXPORT SceneResource* getSceneResourceByAssetId(AssetId assetId) const;		// Considered to be inefficient, avoid method whenever possible
		RENDERERRUNTIME_API_EXPORT SceneResourceId getSceneResourceIdByAssetId(AssetId assetId) const;	// Considered to be inefficient, avoid method whenever possible
		RENDERERRUNTIME_API_EXPORT void loadSceneResourceByAssetId(AssetId assetId, SceneResourceId& sceneResourceId, IResourceListener* resourceListener = nullptr, bool reload = false, ResourceLoaderTypeId resourceLoaderTypeId = getInvalid<ResourceLoaderTypeId>());	// Asynchronous
		RENDERERRUNTIME_API_EXPORT void destroySceneResource(SceneResourceId sceneResourceId);


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	public:
		virtual uint32_t getNumberOfResources() const override;
		virtual IResource& getResourceByIndex(uint32_t index) const override;
		virtual IResource& getResourceByResourceId(ResourceId resourceId) const override;
		virtual IResource* tryGetResourceByResourceId(ResourceId resourceId) const override;
		virtual void reloadResourceByAssetId(AssetId assetId) override;

		inline virtual void update() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	private:
		virtual IResourceLoader* createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SceneResourceManager(IRendererRuntime& rendererRuntime);
		virtual ~SceneResourceManager() override;
		explicit SceneResourceManager(const SceneResourceManager&) = delete;
		SceneResourceManager& operator=(const SceneResourceManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&	 mRendererRuntime;	///< Renderer runtime instance, do not destroy the instance
		const ISceneFactory* mSceneFactory;		///< Scene factory, always valid, do not destroy the instance

		// Internal resource manager implementation
		ResourceManagerTemplate<SceneResource, SceneResourceLoader, SceneResourceId, 16>* mInternalResourceManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
