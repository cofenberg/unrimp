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
#include "RendererRuntime/Public/Core/StringId.h"
#include "RendererRuntime/Public/Core/PackedElementManager.h"
#include "RendererRuntime/Public/Asset/AssetManager.h"
#include "RendererRuntime/Public/Resource/ResourceStreamer.h"
#include "RendererRuntime/Public/IRendererRuntime.h"

#include <cassert>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IResourceManager;
	class IRendererRuntime;
	class IResourceListener;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;				///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset type>/<asset category>/<asset name>"
	typedef StringId ResourceLoaderTypeId;	///< Resource loader type identifier, internally just a POD "uint32_t", usually created by hashing the file format extension (if the resource loader is processing file data in the first place)


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Internal resource manager template; not public used to keep template instantiation overhead under control
	*/
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS>
	class ResourceManagerTemplate : private Manager
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef PackedElementManager<TYPE, ID_TYPE, MAXIMUM_NUMBER_OF_ELEMENTS> Resources;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline ResourceManagerTemplate(IRendererRuntime& rendererRuntime, IResourceManager& resourceManager) :
			mRendererRuntime(rendererRuntime),
			mResourceManager(resourceManager)
		{
			// Nothing here
		}

		inline ~ResourceManagerTemplate()
		{
			// Nothing here
		}

		inline IRendererRuntime& getRendererRuntime() const
		{
			return mRendererRuntime;
		}

		inline IResourceManager& getResourceManager() const
		{
			return mResourceManager;
		}

		inline LOADER_TYPE* createResourceLoaderInstance(MAYBE_UNUSED ResourceLoaderTypeId resourceLoaderTypeId)
		{
			// We only support our own format
			assert(resourceLoaderTypeId == LOADER_TYPE::TYPE_ID);
			return new LOADER_TYPE(mResourceManager, mRendererRuntime);
		}

		inline TYPE* getResourceByAssetId(AssetId assetId) const	// Considered to be inefficient, avoid method whenever possible
		{
			// TODO(co) Implement more efficient solution later on
			const uint32_t numberOfElements = mResources.getNumberOfElements();
			for (uint32_t i = 0; i < numberOfElements; ++i)
			{
				TYPE& resource = mResources.getElementByIndex(i);
				if (resource.getAssetId() == assetId)
				{
					return &resource;
				}
			}

			// There's no resource using the given asset ID
			return nullptr;
		}

		inline TYPE& createEmptyResourceByAssetId(AssetId assetId)	// Resource is not allowed to exist, yet
		{
			// Resource is not allowed to exist, yet
			assert(nullptr == getResourceByAssetId(assetId));

			// Create the resource instance
			TYPE& resource = mResources.addElement();
			resource.setResourceManager(&mResourceManager);
			resource.setAssetId(assetId);

			// Done
			return resource;
		}

		inline void loadResourceByAssetId(AssetId assetId, ID_TYPE& resourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)	// Asynchronous
		{
			// Choose default resource loader type ID, if necessary
			if (isInvalid(resourceLoaderTypeId))
			{
				resourceLoaderTypeId = LOADER_TYPE::TYPE_ID;
			}

			// Get or create the instance
			TYPE* resource = getResourceByAssetId(assetId);

			// Create the resource instance
			const Asset* asset = mRendererRuntime.getAssetManager().tryGetAssetByAssetId(assetId);
			bool load = (reload && nullptr != asset);
			if (nullptr == resource && nullptr != asset)
			{
				resource = &mResources.addElement();
				resource->setResourceManager(&mResourceManager);
				resource->setAssetId(assetId);
				resource->setResourceLoaderTypeId(resourceLoaderTypeId);
				load = true;
			}

			// Before connecting a resource listener, ensure we set the output resource ID at once so it can already directly be used inside the resource listener
			if (nullptr != resource)
			{
				resourceId = resource->getId();
				if (nullptr != resourceListener)
				{
					resource->connectResourceListener(*resourceListener);
				}
			}
			else
			{
				resourceId = getInvalid<ID_TYPE>();
			}

			// Load the resource, if required
			if (load)
			{
				// Commit resource streamer asset load request
				mRendererRuntime.getResourceStreamer().commitLoadRequest(ResourceStreamer::LoadRequest(*asset, resourceLoaderTypeId, reload, mResourceManager, resourceId));
			}
		}

		inline void reloadResourceByAssetId(AssetId assetId)
		{
			// TODO(co) Experimental implementation (take care of resource cleanup etc.)
			const uint32_t numberOfElements = mResources.getNumberOfElements();
			for (uint32_t i = 0; i < numberOfElements; ++i)
			{
				const TYPE& resource = mResources.getElementByIndex(i);
				if (resource.getAssetId() == assetId)
				{
					ID_TYPE resourceId = getInvalid<ID_TYPE>();
					loadResourceByAssetId(assetId, resourceId, nullptr, true, resource.getResourceLoaderTypeId());
					break;
				}
			}
		}

		inline Resources& getResources()
		{
			return mResources;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ResourceManagerTemplate(const ResourceManagerTemplate&) = delete;
		ResourceManagerTemplate& operator=(const ResourceManagerTemplate&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime& mRendererRuntime;	///< Renderer runtime instance, do not destroy the instance
		IResourceManager& mResourceManager;
		Resources		  mResources;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
