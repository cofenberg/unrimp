/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Renderer/Public/Resource/ResourceManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderProperties.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderCacheManager.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IFile;
	class IRenderer;
	class ShaderBlueprintResource;
	class ShaderBlueprintResourceLoader;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t ShaderBlueprintResourceId;	///< POD shader blueprint resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Shader blueprint resource manager
	*/
	class ShaderBlueprintResourceManager final : public ResourceManager<ShaderBlueprintResource>
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererImpl;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline IRenderer& getRenderer() const
		{
			return mRenderer;
		}

		/**
		*  @brief
		*    Return the RHI shader properties
		*
		*  @return
		*    The RHI shader properties
		*
		*  @note
		*    - The RHI shader properties depend on the current RHI implementation, contains e.g. "OpenGL", "GLSL", "ZeroToOneClipZ", "UpperLeftOrigin" etc.
		*    - The RHI shader properties are added during shader source code building and hence are not part of the pipeline state signature
		*/
		[[nodiscard]] inline const ShaderProperties& getRhiShaderProperties() const
		{
			return mRhiShaderProperties;
		}

		RENDERER_API_EXPORT void loadShaderBlueprintResourceByAssetId(AssetId assetId, ShaderBlueprintResourceId& shaderBlueprintResourceId, IResourceListener* resourceListener = nullptr, bool reload = false, ResourceLoaderTypeId resourceLoaderTypeId = getInvalid<ResourceLoaderTypeId>());	// Asynchronous
		RENDERER_API_EXPORT void setInvalidResourceId(ShaderBlueprintResourceId& shaderBlueprintResourceId, IResourceListener& resourceListener) const;

		/**
		*  @brief
		*    Return the shader cache manager
		*
		*  @return
		*    The shader cache manager
		*/
		[[nodiscard]] inline ShaderCacheManager& getShaderCacheManager()
		{
			return mShaderCacheManager;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceManager methods     ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual uint32_t getNumberOfResources() const override;
		[[nodiscard]] virtual IResource& getResourceByIndex(uint32_t index) const override;
		[[nodiscard]] virtual IResource& getResourceByResourceId(ResourceId resourceId) const override;
		[[nodiscard]] virtual IResource* tryGetResourceByResourceId(ResourceId resourceId) const override;
		virtual void reloadResourceByAssetId(AssetId assetId) override;

		inline virtual void update() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private virtual Renderer::IResourceManager methods    ]
	//[-------------------------------------------------------]
	private:
		[[nodiscard]] virtual IResourceLoader* createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderBlueprintResourceManager(IRenderer& renderer);
		virtual ~ShaderBlueprintResourceManager() override;
		explicit ShaderBlueprintResourceManager(const ShaderBlueprintResourceManager&) = delete;
		ShaderBlueprintResourceManager& operator=(const ShaderBlueprintResourceManager&) = delete;

		//[-------------------------------------------------------]
		//[ Pipeline state object cache                           ]
		//[-------------------------------------------------------]
		inline void clearPipelineStateObjectCache()
		{
			mShaderCacheManager.clearCache();
		}

		inline void loadPipelineStateObjectCache(IFile& file)
		{
			mShaderCacheManager.loadCache(file);
		}

		[[nodiscard]] inline bool doesPipelineStateObjectCacheNeedSaving() const
		{
			return mShaderCacheManager.doesCacheNeedSaving();
		}

		inline void savePipelineStateObjectCache(IFile& file)
		{
			mShaderCacheManager.saveCache(file);
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRenderer&		   mRenderer;
		ShaderProperties   mRhiShaderProperties;
		ShaderCacheManager mShaderCacheManager;

		// Internal resource manager implementation
		ResourceManagerTemplate<ShaderBlueprintResource, ShaderBlueprintResourceLoader, ShaderBlueprintResourceId, 128>* mInternalResourceManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
