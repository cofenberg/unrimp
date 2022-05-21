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


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class ITexture;
}
namespace Renderer
{
	class TextureResource;
	class IRenderer;
	class ITextureResourceLoader;
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
	typedef uint32_t			 TextureResourceId;	///< POD texture resource identifier
	typedef StringId			 AssetId;			///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef std::vector<AssetId> AssetIds;


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Texture resource manager class
	*/
	class TextureResourceManager final : public ResourceManager<TextureResource>
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererImpl;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the asset IDs of automatically generated dynamic default texture assets
		*
		*  @param[out] assetIds
		*    Receives the asset IDs of automatically generated dynamic default texture assets, the list is not cleared before new entries are added
		*
		*  @remarks
		*    The texture manager automatically generates some dynamic default texture assets one can reference e.g. inside material blueprint resources:
		*    - "Unrimp/Texture/DynamicByCode/WhiteMap1D"
		*    - "Unrimp/Texture/DynamicByCode/WhiteMap2D"
		*    - "Unrimp/Texture/DynamicByCode/WhiteMapCube"
		*    - "Unrimp/Texture/DynamicByCode/WhiteMap3D"
		*    - "Unrimp/Texture/DynamicByCode/BlackMap1D"
		*    - "Unrimp/Texture/DynamicByCode/BlackMap2D"
		*    - "Unrimp/Texture/DynamicByCode/BlackMapCube"
		*    - "Unrimp/Texture/DynamicByCode/BlackMap3D"
		*    - "Unrimp/Texture/DynamicByCode/IdentityAlbedoMap2D"
		*    - "Unrimp/Texture/DynamicByCode/IdentityAlphaMap2D"
		*    - "Unrimp/Texture/DynamicByCode/IdentityNormalMap2D"
		*    - "Unrimp/Texture/DynamicByCode/IdentityRoughnessMap2D"
		*    - "Unrimp/Texture/DynamicByCode/DielectricMetallicMap2D"
		*    - "Unrimp/Texture/DynamicByCode/IdentityEmissiveMap2D"
		*    - "Unrimp/Texture/DynamicByCode/Identity_argb_nxa2D"
		*    - "Unrimp/Texture/DynamicByCode/Identity_hr_rg_mb_nya2D"
		*/
		static void getDefaultTextureAssetIds(AssetIds& assetIds);


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline uint8_t getNumberOfTopMipmapsToRemove() const
		{
			return mNumberOfTopMipmapsToRemove;
		}

		RENDERER_API_EXPORT void setNumberOfTopMipmapsToRemove(uint8_t numberOfTopMipmapsToRemove);
		[[nodiscard]] RENDERER_API_EXPORT TextureResource* getTextureResourceByAssetId(AssetId assetId) const;		// Considered to be inefficient, avoid method whenever possible
		[[nodiscard]] RENDERER_API_EXPORT TextureResourceId getTextureResourceIdByAssetId(AssetId assetId) const;	// Considered to be inefficient, avoid method whenever possible
		RENDERER_API_EXPORT void loadTextureResourceByAssetId(AssetId assetId, AssetId fallbackTextureAssetId, TextureResourceId& textureResourceId, IResourceListener* resourceListener = nullptr, bool rgbHardwareGammaCorrection = false, bool reload = false, ResourceLoaderTypeId resourceLoaderTypeId = getInvalid<ResourceLoaderTypeId>());	// Asynchronous
		RENDERER_API_EXPORT TextureResourceId createTextureResourceByAssetId(AssetId assetId, Rhi::ITexture& texture, bool rgbHardwareGammaCorrection = false);	// Texture resource is not allowed to exist, yet
		RENDERER_API_EXPORT void destroyTextureResource(TextureResourceId textureResourceId);
		RENDERER_API_EXPORT void setInvalidResourceId(TextureResourceId& textureResourceId, IResourceListener& resourceListener) const;


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
		explicit TextureResourceManager(IRenderer& renderer);
		virtual ~TextureResourceManager() override;
		explicit TextureResourceManager(const TextureResourceManager&) = delete;
		TextureResourceManager& operator=(const TextureResourceManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint8_t mNumberOfTopMipmapsToRemove;	///< The number of top mipmaps to remove while loading textures for efficient texture quality reduction. By setting this to e.g. two a 4096x4096 texture will become 1024x1024.

		// Internal resource manager implementation
		ResourceManagerTemplate<TextureResource, ITextureResourceLoader, TextureResourceId, 2048>* mInternalResourceManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
