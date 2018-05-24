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
#include "RendererRuntime/Resource/Detail/ResourceManager.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IRendererRuntime;
	class ShaderPieceResource;
	class ShaderPieceResourceLoader;
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
	typedef uint32_t ShaderPieceResourceId;	///< POD shader piece resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Shader piece resource manager
	*/
	class ShaderPieceResourceManager final : public ResourceManager<ShaderPieceResource>
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

		RENDERERRUNTIME_API_EXPORT void loadShaderPieceResourceByAssetId(AssetId assetId, ShaderPieceResourceId& shaderPieceResourceId, IResourceListener* resourceListener = nullptr, bool reload = false, ResourceLoaderTypeId resourceLoaderTypeId = getUninitialized<ResourceLoaderTypeId>());	// Asynchronous


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	public:
		virtual uint32_t getNumberOfResources() const override;
		virtual IResource& getResourceByIndex(uint32_t index) const override;
		virtual IResource& getResourceByResourceId(ResourceId resourceId) const override;
		virtual IResource* tryGetResourceByResourceId(ResourceId resourceId) const override;
		virtual void reloadResourceByAssetId(AssetId assetId) override;
		virtual void update() override;


	//[-------------------------------------------------------]
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	private:
		virtual IResourceLoader* createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderPieceResourceManager(IRendererRuntime& rendererRuntime);
		virtual ~ShaderPieceResourceManager() override;
		explicit ShaderPieceResourceManager(const ShaderPieceResourceManager&) = delete;
		ShaderPieceResourceManager& operator=(const ShaderPieceResourceManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&																					mRendererRuntime;
		ResourceManagerTemplate<ShaderPieceResource, ShaderPieceResourceLoader, ShaderPieceResourceId, 64>*	mInternalResourceManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
