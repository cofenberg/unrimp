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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResource.h"
#include "RendererRuntime/Public/Resource/Texture/Loader/CrnTextureResourceLoader.h"
#include "RendererRuntime/Public/Resource/Texture/Loader/KtxTextureResourceLoader.h"
#include "RendererRuntime/Public/Resource/Texture/Loader/Lz4DdsTextureResourceLoader.h"
#include "RendererRuntime/Public/Resource/ResourceManagerTemplate.h"
#ifdef RENDERER_RUNTIME_OPENVR
	#include "RendererRuntime/Public/Vr/OpenVR/Loader/OpenVRTextureResourceLoader.h"
#endif


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void createDefaultDynamicTextureAssets(RendererRuntime::IRendererRuntime& rendererRuntime, RendererRuntime::TextureResourceManager& textureResourceManager)
		{
			Renderer::ITextureManager& textureManager = rendererRuntime.getTextureManager();

			// White RGBA texture
			const uint8_t whiteRgbaData[] = {
				255, 255, 255, 255,	// Face 0
				255, 255, 255, 255,	// Face 1
				255, 255, 255, 255,	// Face 2
				255, 255, 255, 255,	// Face 3
				255, 255, 255, 255,	// Face 4
				255, 255, 255, 255	// Face 5
			};
			Renderer::ITexturePtr whiteRgba1DTexturePtr(textureManager.createTexture1D(1, Renderer::TextureFormat::R8G8B8A8, whiteRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(whiteRgba1DTexturePtr, "White 1D RGBA texture")
			Renderer::ITexturePtr whiteRgba2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8G8B8A8, whiteRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(whiteRgba2DTexturePtr, "White 2D RGBA texture")
			Renderer::ITexturePtr whiteRgbaCubeTexturePtr(textureManager.createTextureCube(1, 1, Renderer::TextureFormat::R8G8B8A8, whiteRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(whiteRgbaCubeTexturePtr, "White cube RGBA texture")
			Renderer::ITexturePtr whiteRgba3DTexturePtr(textureManager.createTexture3D(1, 1, 1, Renderer::TextureFormat::R8G8B8A8, whiteRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(whiteRgbaCubeTexturePtr, "White 3D RGBA texture")

			// Normal map identity texture
			const uint8_t normalMapIdentityData[] = { 128, 128, 255, 255 };
			Renderer::ITexturePtr normalMapIdentity2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8G8B8A8, normalMapIdentityData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(normalMapIdentity2DTexturePtr, "2D normal map identity texture")

			// White alpha texture
			const uint8_t whiteAData[] = { 255 };
			Renderer::ITexturePtr whiteA2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8, whiteAData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(whiteA2DTexturePtr, "White 2D alpha texture")

			// Black RGBA texture
			const uint8_t blackRgbaData[] = {
				0, 0, 0, 0,	// Face 0
				0, 0, 0, 0,	// Face 1
				0, 0, 0, 0,	// Face 2
				0, 0, 0, 0,	// Face 3
				0, 0, 0, 0,	// Face 4
				0, 0, 0, 0	// Face 5
			};
			Renderer::ITexturePtr blackRgba1DTexturePtr(textureManager.createTexture1D(1, Renderer::TextureFormat::R8G8B8A8, blackRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(blackRgba1DTexturePtr, "Black 1D RGBA texture")
			Renderer::ITexturePtr blackRgba2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8G8B8A8, blackRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(blackRgba2DTexturePtr, "Black 2D RGBA texture")
			Renderer::ITexturePtr blackRgbaCubeTexturePtr(textureManager.createTextureCube(1, 1, Renderer::TextureFormat::R8G8B8A8, blackRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(blackRgbaCubeTexturePtr, "Black cube RGBA texture")
			Renderer::ITexturePtr blackRgba3DTexturePtr(textureManager.createTexture3D(1, 1, 1, Renderer::TextureFormat::R8G8B8A8, blackRgbaData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(blackRgba3DTexturePtr, "Black 3D RGBA texture")

			// Black alpha texture
			const uint8_t blackAData[] = { 0 };
			Renderer::ITexturePtr blackA2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8, blackAData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(blackA2DTexturePtr, "Black 2D alpha texture")

			// "_argb_nxa"-texture
			// -> Must be white so e.g. albedo color can be multiplied in
			const uint8_t _argb_nxaIdentityData[] = { 255, 255, 255, 128 };
			Renderer::ITexturePtr _argb_nxaIdentity2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8G8B8A8, _argb_nxaIdentityData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(_argb_nxaIdentity2DTexturePtr, "2D _argb_nxa identity texture")

			// "_hr_rg_mb_nya"-texture
			const uint8_t _hr_rg_mb_nyaIdentityData[] = { 0, 255, 0, 128 };
			Renderer::ITexturePtr _hr_rg_mb_nyaIdentity2DTexturePtr(textureManager.createTexture2D(1, 1, Renderer::TextureFormat::R8G8B8A8, _hr_rg_mb_nyaIdentityData, Renderer::TextureFlag::SHADER_RESOURCE, Renderer::TextureUsage::IMMUTABLE));
			RENDERER_SET_RESOURCE_DEBUG_NAME(_hr_rg_mb_nyaIdentity2DTexturePtr, "2D _hr_rg_mb_nya identity texture")

			// Define helper macro
			#define CREATE_TEXTURE(name, texturePtr) textureResourceManager.createTextureResourceByAssetId(STRING_ID("Unrimp/Texture/DynamicByCode/"#name), *texturePtr);

			// Create default dynamic texture assets
			CREATE_TEXTURE(WhiteMap1D,				whiteRgba1DTexturePtr)
			CREATE_TEXTURE(WhiteMap2D,				whiteRgba2DTexturePtr)
			CREATE_TEXTURE(WhiteMapCube,			whiteRgbaCubeTexturePtr)
			CREATE_TEXTURE(WhiteMap3D,				whiteRgba3DTexturePtr)
			CREATE_TEXTURE(BlackMap1D,				blackRgba1DTexturePtr)
			CREATE_TEXTURE(BlackMap2D,				blackRgba2DTexturePtr)
			CREATE_TEXTURE(BlackMapCube,			blackRgbaCubeTexturePtr)
			CREATE_TEXTURE(BlackMap3D,				blackRgba3DTexturePtr)
			CREATE_TEXTURE(IdentityAlbedoMap2D,		whiteRgba2DTexturePtr)	// Must be white so e.g. albedo color can be multiplied in
			CREATE_TEXTURE(IdentityAlphaMap2D,		whiteA2DTexturePtr)
			CREATE_TEXTURE(IdentityNormalMap2D,		normalMapIdentity2DTexturePtr)
			CREATE_TEXTURE(IdentityRoughnessMap2D,	whiteA2DTexturePtr)
			CREATE_TEXTURE(DielectricMetallicMap2D,	blackA2DTexturePtr)
			CREATE_TEXTURE(IdentityEmissiveMap2D,	blackRgba2DTexturePtr)
			CREATE_TEXTURE(Identity_argb_nxa2D,		_argb_nxaIdentity2DTexturePtr)
			CREATE_TEXTURE(Identity_hr_rg_mb_nya2D,	_hr_rg_mb_nyaIdentity2DTexturePtr)

			// Undefine helper macro
			#undef CREATE_TEXTURE
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void TextureResourceManager::getDefaultTextureAssetIds(AssetIds& assetIds)
	{
		// Define helper macros
		#define ADD_ASSET_ID(name) assetIds.push_back(STRING_ID(name));

		// Add asset IDs
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/WhiteMap1D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/WhiteMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/WhiteMapCube")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/WhiteMap3D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/BlackMap1D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/BlackMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/BlackMapCube")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/BlackMap3D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityAlbedoMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityAlphaMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityNormalMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityRoughnessMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/DielectricMetallicMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityEmissiveMap2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/Identity_argb_nxa2D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/Identity_hr_rg_mb_nya2D")

		// Undefine helper macros
		#undef ADD_ASSET_ID
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void TextureResourceManager::setNumberOfTopMipmapsToRemove(uint8_t numberOfTopMipmapsToRemove)
	{
		if (mNumberOfTopMipmapsToRemove != numberOfTopMipmapsToRemove)
		{
			mNumberOfTopMipmapsToRemove = numberOfTopMipmapsToRemove;

			// Update the already loaded textures, but really only textures loaded from texture assets and not e.g. textures dynamically created during runtime
			const AssetManager& assetManager = mInternalResourceManager->getRendererRuntime().getAssetManager();
			const uint32_t numberOfElements = mInternalResourceManager->getResources().getNumberOfElements();
			for (uint32_t i = 0; i < numberOfElements; ++i)
			{
				const TextureResource& textureResource = mInternalResourceManager->getResources().getElementByIndex(i);
				const AssetId assetId = textureResource.getAssetId();
				if (nullptr != assetManager.tryGetAssetByAssetId(assetId) && textureResource.getLoadingState() == RendererRuntime::IResource::LoadingState::LOADED)
				{
					TextureResourceId textureResourceId = getInvalid<TextureResourceId>();
					loadTextureResourceByAssetId(assetId, getInvalid<AssetId>(), textureResourceId, nullptr, textureResource.isRgbHardwareGammaCorrection(), true);
				}
			}
		}
	}

	TextureResource* TextureResourceManager::getTextureResourceByAssetId(AssetId assetId) const
	{
		return mInternalResourceManager->getResourceByAssetId(assetId);
	}

	TextureResourceId TextureResourceManager::getTextureResourceIdByAssetId(AssetId assetId) const
	{
		const TextureResource* textureResource = getTextureResourceByAssetId(assetId);
		return (nullptr != textureResource) ? textureResource->getId() : getInvalid<TextureResourceId>();
	}

	void TextureResourceManager::loadTextureResourceByAssetId(AssetId assetId, AssetId fallbackTextureAssetId, TextureResourceId& textureResourceId, IResourceListener* resourceListener, bool rgbHardwareGammaCorrection, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		// Check whether or not the texture resource already exists
		TextureResource* textureResource = getTextureResourceByAssetId(assetId);

		// Create the resource instance
		// -> In case the texture asset ID is unknown it might be a runtime dynamic created texture which will be created by someone later one
		// -> Please note that the fallback texture asset ID is intentionally only used if the texture asset ID is valid, it's a fallback as long as the real texture data has not been loaded yet
		const IRendererRuntime& rendererRuntime = mInternalResourceManager->getRendererRuntime();
		const Asset* asset = rendererRuntime.getAssetManager().tryGetAssetByAssetId(assetId);
		bool load = (reload && nullptr != asset);
		if (nullptr == textureResource && nullptr != asset)
		{
			textureResource = &mInternalResourceManager->getResources().addElement();
			textureResource->setResourceManager(this);
			textureResource->setAssetId(assetId);
			textureResource->setResourceLoaderTypeId(resourceLoaderTypeId);
			textureResource->mRgbHardwareGammaCorrection = rgbHardwareGammaCorrection;
			load = true;
		}

		// Before connecting a resource listener, ensure we set the output resource ID at once so it can already directly be used inside the resource listener
		if (nullptr != textureResource)
		{
			textureResourceId = textureResource->getId();
			if (nullptr != resourceListener)
			{
				textureResource->connectResourceListener(*resourceListener);
			}
		}
		else
		{
			textureResourceId = getInvalid<TextureResourceId>();
		}

		// Load the resource, if required
		if (load)
		{
			// Prepare the resource loader
			if (isInvalid(resourceLoaderTypeId))
			{
				// The totally primitive texture resource loader type detection is sufficient for now
				const char* filenameExtension = strrchr(&asset->virtualFilename[0], '.');
				if (nullptr != filenameExtension)
				{
					resourceLoaderTypeId = StringId(filenameExtension + 1);
				}
				else
				{
					// Error! We should never ever be able to be in here, it's the renderer toolkit responsible to ensure the renderer runtime only works with sane data.
					assert(false);
				}
			}
			if (isValid(resourceLoaderTypeId))
			{
				// Commit resource streamer asset load request
				rendererRuntime.getResourceStreamer().commitLoadRequest(ResourceStreamer::LoadRequest(*asset, resourceLoaderTypeId, reload, *this, textureResourceId));

				// Since it might take a moment to load the texture resource, we'll use a fallback placeholder renderer texture resource so we don't have to wait until the real thing is there
				// -> In case there's already a renderer texture, keep that as long as possible (for example there might be a change in the number of top mipmaps to remove)
				if (nullptr == textureResource->mTexture)
				{
					if (isValid(fallbackTextureAssetId))
					{
						const TextureResource* fallbackTextureResource = getTextureResourceByAssetId(fallbackTextureAssetId);
						if (nullptr != fallbackTextureResource)
						{
							textureResource->mTexture = fallbackTextureResource->getTexture();
							textureResource->setLoadingState(IResource::LoadingState::LOADED);
						}
						else
						{
							// Error! Fallback texture asset ID not found.
							assert(false);
						}
					}
					else
					{
						// Hiccups / lags warning: There should always be a fallback texture asset ID (better be safe than sorry)
						assert(false);
					}
				}
			}
			else
			{
				// Error! We should never ever be able to be in here, it's the renderer toolkit responsible to ensure the renderer runtime only works with sane data.
				assert(false);
			}
		}
	}

	TextureResourceId TextureResourceManager::createTextureResourceByAssetId(AssetId assetId, Renderer::ITexture& texture, bool rgbHardwareGammaCorrection)
	{
		// Texture resource is not allowed to exist, yet
		assert(nullptr == getTextureResourceByAssetId(assetId));

		// Create the texture resource instance
		TextureResource& textureResource = mInternalResourceManager->getResources().addElement();
		textureResource.setResourceManager(this);
		textureResource.setAssetId(assetId);
		textureResource.mRgbHardwareGammaCorrection = rgbHardwareGammaCorrection;	// TODO(co) We might need to extend "Renderer::ITexture" so we can readback the texture format
		textureResource.mTexture = &texture;

		// Done
		setResourceLoadingState(textureResource, IResource::LoadingState::LOADED);
		return textureResource.getId();
	}

	void TextureResourceManager::destroyTextureResource(TextureResourceId textureResourceId)
	{
		mInternalResourceManager->getResources().removeElement(textureResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	uint32_t TextureResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& TextureResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& TextureResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* TextureResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void TextureResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		// TODO(co) Experimental implementation (take care of resource cleanup etc.)
		const uint32_t numberOfElements = mInternalResourceManager->getResources().getNumberOfElements();
		for (uint32_t i = 0; i < numberOfElements; ++i)
		{
			const TextureResource& textureResource = mInternalResourceManager->getResources().getElementByIndex(i);
			if (textureResource.getAssetId() == assetId)
			{
				TextureResourceId textureResourceId = getInvalid<TextureResourceId>();
				loadTextureResourceByAssetId(assetId, getInvalid<AssetId>(), textureResourceId, nullptr, textureResource.isRgbHardwareGammaCorrection(), true, textureResource.getResourceLoaderTypeId());
				break;
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	IResourceLoader* TextureResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		switch (resourceLoaderTypeId)
		{
			case CrnTextureResourceLoader::TYPE_ID:
				return new CrnTextureResourceLoader(*this, mInternalResourceManager->getRendererRuntime());

			case Lz4DdsTextureResourceLoader::TYPE_ID:
				return new Lz4DdsTextureResourceLoader(*this, mInternalResourceManager->getRendererRuntime());

			case DdsTextureResourceLoader::TYPE_ID:
				return new DdsTextureResourceLoader(*this, mInternalResourceManager->getRendererRuntime());

			case KtxTextureResourceLoader::TYPE_ID:
				return new KtxTextureResourceLoader(*this, mInternalResourceManager->getRendererRuntime());

			#ifdef RENDERER_RUNTIME_OPENVR
				case OpenVRTextureResourceLoader::TYPE_ID:
					return new OpenVRTextureResourceLoader(*this, mInternalResourceManager->getRendererRuntime());
			#endif

			default:
				// TODO(co) Error handling
				assert(false);
				return nullptr;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	TextureResourceManager::TextureResourceManager(IRendererRuntime& rendererRuntime) :
		mNumberOfTopMipmapsToRemove(0)
	{
		mInternalResourceManager = new ResourceManagerTemplate<TextureResource, ITextureResourceLoader, TextureResourceId, 2048>(rendererRuntime, *this);
		::detail::createDefaultDynamicTextureAssets(rendererRuntime, *this);
	}

	TextureResourceManager::~TextureResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
