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
#include "RendererRuntime/Resource/Detail/IResourceLoader.h"
#include "RendererRuntime/Core/File/MemoryFile.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IRenderer;
}
namespace RendererRuntime
{
	class IRendererRuntime;
	class CompositorWorkspaceResource;
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
	typedef uint32_t CompositorWorkspaceResourceId;	///< POD compositor workspace resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class CompositorWorkspaceResourceLoader final : public IResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend ResourceManagerTemplate<CompositorWorkspaceResource, CompositorWorkspaceResourceLoader, CompositorWorkspaceResourceId, 32>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("compositor_workspace");


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	public:
		inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void initialize(const Asset& asset, bool reload, IResource& resource) override;

		inline virtual bool hasDeserialization() const override
		{
			return true;
		}

		virtual void onDeserialization(IFile& file) override;
		virtual void onProcessing() override;

		inline virtual bool onDispatch() override
		{
			// Fully loaded
			return true;
		}

		inline virtual bool isFullyLoaded() override
		{
			// Fully loaded
			return true;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline CompositorWorkspaceResourceLoader(IResourceManager& resourceManager, IRendererRuntime& rendererRuntime) :
			IResourceLoader(resourceManager),
			mRendererRuntime(rendererRuntime),
			mCompositorWorkspaceResource(nullptr)
		{
			// Nothing here
		}

		inline virtual ~CompositorWorkspaceResourceLoader() override
		{
			// Nothing here
		}

		explicit CompositorWorkspaceResourceLoader(const CompositorWorkspaceResourceLoader&) = delete;
		CompositorWorkspaceResourceLoader& operator=(const CompositorWorkspaceResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&			 mRendererRuntime;				///< Renderer runtime instance, do not destroy the instance
		CompositorWorkspaceResource* mCompositorWorkspaceResource;	///< Destination resource
		// Temporary data
		MemoryFile mMemoryFile;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
