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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/IResourceLoader.h"
#include "Renderer/Public/Core/File/MemoryFile.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class SceneResource;
	class IRenderer;
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
	typedef uint32_t SceneResourceId;	///< POD scene resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class SceneResourceLoader final : public IResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend ResourceManagerTemplate<SceneResource, SceneResourceLoader, SceneResourceId, 16>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("scene");


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void initialize(const Asset& asset, bool reload, IResource& resource) override;

		[[nodiscard]] inline virtual bool hasDeserialization() const override
		{
			return true;
		}

		[[nodiscard]] virtual bool onDeserialization(IFile& file) override;

		[[nodiscard]] inline virtual bool hasProcessing() const override
		{
			return true;
		}

		virtual void onProcessing() override;
		[[nodiscard]] virtual bool onDispatch() override;

		[[nodiscard]] inline virtual bool isFullyLoaded() override
		{
			// Fully loaded
			return true;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline SceneResourceLoader(IResourceManager& resourceManager, IRenderer& renderer) :
			IResourceLoader(resourceManager),
			mRenderer(renderer),
			mSceneResource(nullptr)
		{
			// Nothing here
		}

		inline virtual ~SceneResourceLoader() override
		{
			// Nothing here
		}

		explicit SceneResourceLoader(const SceneResourceLoader&) = delete;
		SceneResourceLoader& operator=(const SceneResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRenderer&	   mRenderer;		///< Renderer instance, do not destroy the instance
		SceneResource* mSceneResource;	///< Destination resource

		// Temporary data
		MemoryFile mMemoryFile;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
