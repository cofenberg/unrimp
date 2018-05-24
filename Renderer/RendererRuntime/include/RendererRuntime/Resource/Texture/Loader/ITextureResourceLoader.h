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


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ITexture;
}
namespace RendererRuntime
{
	class TextureResource;
	class IRendererRuntime;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class ITextureResourceLoader : protected IResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("invalid_abstract_type");


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

		virtual bool onDispatch() override;

		inline virtual bool isFullyLoaded() override
		{
			// Fully loaded
			return true;
		}


	//[-------------------------------------------------------]
	//[ Protected RendererRuntime::ITextureResourceLoader methods ]
	//[-------------------------------------------------------]
	protected:
		virtual Renderer::ITexture* createRendererTexture() = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit ITextureResourceLoader(IResourceManager& resourceManager, IRendererRuntime& rendererRuntime) :
			IResourceLoader(resourceManager),
			mRendererRuntime(rendererRuntime),
			mTextureResource(nullptr),
			mTexture(nullptr)
		{
			// Nothing here
		}

		inline virtual ~ITextureResourceLoader() override
		{
			// Nothing here
		}

		explicit ITextureResourceLoader(const ITextureResourceLoader&) = delete;
		ITextureResourceLoader& operator=(const ITextureResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		IRendererRuntime&	mRendererRuntime;	///< Renderer runtime instance, do not destroy the instance
		TextureResource*	mTextureResource;	///< Destination resource
		Renderer::ITexture*	mTexture;			///< In case the used renderer backend supports native multi-threading we also create the renderer resource asynchronous, but the final resource pointer reassignment must still happen synchronous


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
