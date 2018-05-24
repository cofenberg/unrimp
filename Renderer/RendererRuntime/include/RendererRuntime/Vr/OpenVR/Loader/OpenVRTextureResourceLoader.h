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
#include "RendererRuntime/Resource/Texture/Loader/ITextureResourceLoader.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace vr
{
	struct RenderModel_TextureMap_t;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class OpenVRTextureResourceLoader final : public ITextureResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class TextureResourceManager;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("openvr_texture");


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	public:
		inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		inline virtual bool hasDeserialization() const override
		{
			return false;
		}

		inline virtual void onDeserialization(IFile&) override
		{
			// We're using the OpenVR API instead of reading from a file
		}

		virtual void onProcessing() override;


	//[-------------------------------------------------------]
	//[ Protected RendererRuntime::ITextureResourceLoader methods ]
	//[-------------------------------------------------------]
	protected:
		virtual Renderer::ITexture* createRendererTexture() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline OpenVRTextureResourceLoader(IResourceManager& resourceManager, IRendererRuntime& rendererRuntime) :
			ITextureResourceLoader(resourceManager, rendererRuntime),
			mVrRenderModelTextureMap(nullptr)
		{
			// Nothing here
		}

		inline virtual ~OpenVRTextureResourceLoader() override
		{
			// Nothing here
		}

		explicit OpenVRTextureResourceLoader(const OpenVRTextureResourceLoader&) = delete;
		OpenVRTextureResourceLoader& operator=(const OpenVRTextureResourceLoader&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Temporary data
		vr::RenderModel_TextureMap_t* mVrRenderModelTextureMap;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
