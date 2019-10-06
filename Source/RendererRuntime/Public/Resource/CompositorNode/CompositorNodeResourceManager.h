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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/ResourceManager.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IRendererRuntime;
	class RenderPassManager;
	class FramebufferManager;
	class ICompositorPassFactory;
	class CompositorNodeResource;
	class RenderTargetTextureManager;
	class CompositorNodeResourceLoader;
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
	typedef uint32_t CompositorNodeResourceId;	///< POD compositor node resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class CompositorNodeResourceManager final : public ResourceManager<CompositorNodeResource>
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererRuntimeImpl;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline IRendererRuntime& getRendererRuntime() const
		{
			return mRendererRuntime;
		}

		RENDERERRUNTIME_API_EXPORT void loadCompositorNodeResourceByAssetId(AssetId assetId, CompositorNodeResourceId& compositorNodeResourceId, IResourceListener* resourceListener = nullptr, bool reload = false, ResourceLoaderTypeId resourceLoaderTypeId = getInvalid<ResourceLoaderTypeId>());	// Asynchronous
		RENDERERRUNTIME_API_EXPORT void setInvalidResourceId(CompositorNodeResourceId& compositorNodeResourceId, IResourceListener& resourceListener) const;

		[[nodiscard]] inline const ICompositorPassFactory& getCompositorPassFactory() const
		{
			// We know that this pointer is always valid
			return *mCompositorPassFactory;
		}

		RENDERERRUNTIME_API_EXPORT void setCompositorPassFactory(const ICompositorPassFactory* compositorPassFactory);

		[[nodiscard]] inline RenderTargetTextureManager& getRenderTargetTextureManager()
		{
			ASSERT(nullptr != mRenderTargetTextureManager);
			return *mRenderTargetTextureManager;
		}

		[[nodiscard]] inline RenderPassManager& getRenderPassManager()
		{
			ASSERT(nullptr != mRenderPassManager);
			return *mRenderPassManager;
		}

		[[nodiscard]] inline FramebufferManager& getFramebufferManager()
		{
			ASSERT(nullptr != mFramebufferManager);
			return *mFramebufferManager;
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
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
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	private:
		[[nodiscard]] virtual IResourceLoader* createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit CompositorNodeResourceManager(IRendererRuntime& rendererRuntime);
		virtual ~CompositorNodeResourceManager() override;
		explicit CompositorNodeResourceManager(const CompositorNodeResourceManager&) = delete;
		CompositorNodeResourceManager& operator=(const CompositorNodeResourceManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&			  mRendererRuntime;				///< Renderer runtime instance, do not destroy the instance
		const ICompositorPassFactory* mCompositorPassFactory;		///< Compositor pass factory, always valid, do not destroy the instance
		RenderTargetTextureManager*	  mRenderTargetTextureManager;	///< Render target texture manager, always valid, we're responsible for destroying it if we no longer need it
		RenderPassManager*			  mRenderPassManager;			///< Render pass manager, always valid, we're responsible for destroying it if we no longer need it
		FramebufferManager*			  mFramebufferManager;			///< Framebuffer manager, always valid, we're responsible for destroying it if we no longer need it

		// Internal resource manager implementation
		ResourceManagerTemplate<CompositorNodeResource, CompositorNodeResourceLoader, CompositorNodeResourceId, 32>* mInternalResourceManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
