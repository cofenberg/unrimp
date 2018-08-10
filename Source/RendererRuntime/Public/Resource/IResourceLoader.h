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
#include "RendererRuntime/Public/Core/Loader.h"
#include "RendererRuntime/Public/Asset/Asset.h"

#include <cassert>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IFile;
	class IResource;
	class IResourceManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId ResourceLoaderTypeId;	///< Resource loader type identifier, internally just a POD "uint32_t", usually created by hashing the file format extension (if the resource loader is processing file data in the first place)


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class IResourceLoader : protected Loader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class ResourceStreamer;	// Needs to be able to destroy resource loader instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the owner resource manager
		*
		*  @return
		*    The owner resource manager
		*/
		inline IResourceManager& getResourceManager() const
		{
			return mResourceManager;
		}

		/**
		*  @brief
		*    Return the asset the resource is using
		*
		*  @return
		*    The asset the resource is using
		*/
		inline const Asset& getAsset() const
		{
			assert(nullptr != mAsset);
			return *mAsset;
		}

		/**
		*  @brief
		*    Return whether or not the resource gets reloaded or not
		*
		*  @return
		*    "true" if the resource is new in memory, else "false" for reload an already loaded resource (and e.g. update cache entries)
		*/
		inline bool getReload() const
		{
			return mReload;
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the resource loader type ID
		*/
		virtual ResourceLoaderTypeId getResourceLoaderTypeId() const = 0;

		/**
		*  @brief
		*    Initialize the resource loader type ID
		*
		*  @param[in] asset
		*    Asset to load
		*  @param[in] reload
		*    "true" if the resource is new in memory, else "false" for reload an already loaded resource (and e.g. update cache entries)
		*  @param[out] resource
		*    Resource instance to fill
		*/
		virtual void initialize(const Asset& asset, bool reload, IResource& resource) = 0;

		/**
		*  @brief
		*    Called to check whether or not the resource loader has to deserialize (usually from file)
		*
		*  @return
		*    "true" if deserialization has to be called, else "false" (for example a procedural resource or a resource received via an API like OpenVR)
		*/
		virtual bool hasDeserialization() const = 0;

		/**
		*  @brief
		*    Called when the resource loader has to deserialize (usually from file) the internal data into memory
		*
		*  @param[in] file
		*    File to read from
		*/
		virtual void onDeserialization(IFile& file) = 0;

		/**
		*  @brief
		*    Called when the resource loader has to perform internal in-memory data processing
		*/
		virtual void onProcessing() = 0;

		/**
		*  @brief
		*    Called when the resource loader has to dispatch the data (e.g. to the renderer backend)
		*
		*  @return
		*    "true" if the resource is fully loaded, else "false" (e.g. asset dependencies are not fully loaded, yet) meaning this method will be called later on again
		*/
		virtual bool onDispatch() = 0;

		/**
		*  @brief
		*    Called when the resource loader is about to switch the resource into the loaded state
		*
		*  @return
		*    "true" if the resource is fully loaded, else "false" (e.g. asset dependencies are not fully loaded, yet) meaning this method will be called later on again
		*/
		virtual bool isFullyLoaded() = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline IResourceLoader(IResourceManager& resourceManager) :
			mResourceManager(resourceManager),
			mAsset(nullptr),
			mReload(false)
		{
			// Nothing here
		}

		inline virtual ~IResourceLoader()
		{
			// Nothing here
		}

		explicit IResourceLoader(const IResourceLoader&) = delete;
		IResourceLoader& operator=(const IResourceLoader&) = delete;

		inline void initialize(const Asset& asset, bool reload)
		{
			mAsset  = &asset;
			mReload = reload;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IResourceManager& mResourceManager;	///< Owner resource manager
		const Asset*	  mAsset;			///< Used asset, must be valid
		bool			  mReload;			///< "true" if the resource is new in memory, else "false" for reload an already loaded resource (and e.g. update cache entries)


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
