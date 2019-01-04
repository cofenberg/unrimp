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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Vr/OpenVR/Loader/OpenVRTextureResourceLoader.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResource.h"
#include "RendererRuntime/Public/IRendererRuntime.h"
#include "RendererRuntime/Public/Context.h"

#include <openvr/openvr.h>

#include <chrono>
#include <thread>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	void OpenVRTextureResourceLoader::onProcessing()
	{
		// Load the render model texture
		vr::IVRRenderModels* vrRenderModels = vr::VRRenderModels();
		vr::EVRRenderModelError vrRenderModelError = vr::VRRenderModelError_Loading;
		const vr::TextureID_t albedoTextureId = static_cast<vr::TextureID_t>(std::atoi(getAsset().virtualFilename));
		while (vrRenderModelError == vr::VRRenderModelError_Loading)
		{
			vrRenderModelError = vrRenderModels->LoadTexture_Async(albedoTextureId, &mVrRenderModelTextureMap);
			if (vrRenderModelError == vr::VRRenderModelError_Loading)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1ms);
			}
		}
		if (vr::VRRenderModelError_None != vrRenderModelError)
		{
			RENDERER_LOG(mRendererRuntime.getContext(), CRITICAL, "The renderer runtime was unable to load OpenVR albedo texture %d: %s", albedoTextureId, vrRenderModels->GetRenderModelErrorNameFromEnum(vrRenderModelError))
			return;
		}

		{ // The "_argb_nxa" texture channel packing stores the x channel of a normal map inside the alpha channel, set identity normal map x value
			uint8_t* rubTextureMapData = const_cast<uint8_t*>(mVrRenderModelTextureMap->rubTextureMapData);	// Evil const-cast since I don't want to copy the data
			const uint8_t* rubTextureMapDataEnd = rubTextureMapData + mVrRenderModelTextureMap->unWidth * mVrRenderModelTextureMap->unHeight * 4;
			for (; rubTextureMapData < rubTextureMapDataEnd; rubTextureMapData += 4)
			{
				rubTextureMapData[3] = 128;
			}
		}

		// Can we create the renderer resource asynchronous as well?
		if (mRendererRuntime.getRenderer().getCapabilities().nativeMultiThreading)
		{
			mTexture = createRendererTexture();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected RendererRuntime::ITextureResourceLoader methods ]
	//[-------------------------------------------------------]
	Renderer::ITexture* OpenVRTextureResourceLoader::createRendererTexture()
	{
		Renderer::ITexture2D* texture2D = nullptr;
		if (nullptr != mVrRenderModelTextureMap)
		{
			// Create the renderer texture instance
			const bool rgbHardwareGammaCorrection = true;	// TODO(co) It must be possible to set the property name from the outside: Ask the material blueprint whether or not hardware gamma correction should be used
			texture2D = mRendererRuntime.getTextureManager().createTexture2D(mVrRenderModelTextureMap->unWidth, mVrRenderModelTextureMap->unHeight, rgbHardwareGammaCorrection ? Renderer::TextureFormat::R8G8B8A8_SRGB : Renderer::TextureFormat::R8G8B8A8, static_cast<const void*>(mVrRenderModelTextureMap->rubTextureMapData), Renderer::TextureFlag::GENERATE_MIPMAPS | Renderer::TextureFlag::SHADER_RESOURCE);
			RENDERER_SET_RESOURCE_DEBUG_NAME(texture2D, getAsset().virtualFilename)

			// Free the render model texture
			vr::VRRenderModels()->FreeTexture(mVrRenderModelTextureMap);
			mVrRenderModelTextureMap = nullptr;
		}

		// Done
		return texture2D;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
