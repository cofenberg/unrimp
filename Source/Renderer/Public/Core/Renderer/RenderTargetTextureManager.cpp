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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Core/Renderer/RenderTargetTextureManager.h"
#include "Renderer/Public/Resource/Texture/TextureResourceManager.h"
#include "Renderer/Public/Resource/Texture/TextureResource.h"
#include "Renderer/Public/IRenderer.h"

#include <algorithm>


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
		[[nodiscard]] inline bool orderRenderTargetTextureElementByRenderTargetTextureSignatureId(const Renderer::RenderTargetTextureManager::RenderTargetTextureElement& left, const Renderer::RenderTargetTextureManager::RenderTargetTextureElement& right)
		{
			return (left.renderTargetTextureSignature.getRenderTargetTextureSignatureId() < right.renderTargetTextureSignature.getRenderTargetTextureSignatureId());
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void RenderTargetTextureManager::clear()
	{
		clearRhiResources();
		mSortedRenderTargetTextureVector.clear();
		mAssetIdToRenderTargetTextureSignatureId.clear();
		mAssetIdToIndex.clear();
	}

	void RenderTargetTextureManager::clearRhiResources()
	{
		TextureResourceManager& textureResourceManager = mRenderer.getTextureResourceManager();
		for (RenderTargetTextureElement& renderTargetTextureElement : mSortedRenderTargetTextureVector)
		{
			{ // Unload texture resource
				TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(renderTargetTextureElement.assetId);
				if (nullptr != textureResource)
				{
					textureResource->setTexture(nullptr);
				}
			}

			// Release RHI texture reference
			if (nullptr != renderTargetTextureElement.texture)
			{
				renderTargetTextureElement.texture->releaseReference();
				renderTargetTextureElement.texture = nullptr;
			}
		}
	}

	void RenderTargetTextureManager::addRenderTargetTexture(AssetId assetId, const RenderTargetTextureSignature& renderTargetTextureSignature)
	{
		RenderTargetTextureElement renderTargetTextureElement(assetId, renderTargetTextureSignature);

		// TODO(co) The render target texture and framebuffer handling is still under construction regarding recycling RHI resources etc. - so for now, just add render target textures to have something to start with
		{ // Add new render target texture
			// Register the new render target texture element
			++renderTargetTextureElement.numberOfReferences;
			mSortedRenderTargetTextureVector.push_back(renderTargetTextureElement);
			mAssetIdToIndex.emplace(assetId, static_cast<uint32_t>(mSortedRenderTargetTextureVector.size() - 1));
		}
		/*
		SortedRenderTargetTextureVector::iterator iterator = std::lower_bound(mSortedRenderTargetTextureVector.begin(), mSortedRenderTargetTextureVector.end(), renderTargetTextureElement, ::detail::orderRenderTargetTextureElementByRenderTargetTextureSignatureId);
		if (iterator == mSortedRenderTargetTextureVector.end() || iterator->renderTargetTextureSignature.getRenderTargetTextureSignatureId() != renderTargetTextureElement.renderTargetTextureSignature.getRenderTargetTextureSignatureId())
		{
			// Add new render target texture

			// Register the new render target texture element
			++renderTargetTextureElement.numberOfReferences;
			mSortedRenderTargetTextureVector.insert(iterator, renderTargetTextureElement);
		}
		else
		{
			// Just increase the number of references
			++iterator->numberOfReferences;
		}
		mAssetIdToRenderTargetTextureSignatureId.emplace(assetId, renderTargetTextureSignature.getRenderTargetTextureSignatureId());
		*/
	}

	Rhi::ITexture* RenderTargetTextureManager::getTextureByAssetId(AssetId assetId, const Rhi::IRenderTarget& renderTarget, uint8_t numberOfMultisamples, float resolutionScale, const RenderTargetTextureSignature** outRenderTargetTextureSignature)
	{
		Rhi::ITexture* texture = nullptr;

		// Map asset ID to render target texture signature ID
		// TODO(co) The render target texture and framebuffer handling is still under construction regarding recycling RHI resources etc. - so for now, just add render target textures to have something to start with
		/*
		AssetIdToRenderTargetTextureSignatureId::const_iterator iterator = mAssetIdToRenderTargetTextureSignatureId.find(assetId);
		if (mAssetIdToRenderTargetTextureSignatureId.cend() != iterator)
		{
			// TODO(co) Is there need for a more efficient search?
			const RenderTargetTextureSignatureId renderTargetTextureSignatureId = iterator->second;
			for (RenderTargetTextureElement& renderTargetTextureElement : mSortedRenderTargetTextureVector)
			*/
		AssetIdToIndex::const_iterator iterator = mAssetIdToIndex.find(assetId);
		if (mAssetIdToIndex.cend() != iterator)
		{
			RenderTargetTextureElement& renderTargetTextureElement = mSortedRenderTargetTextureVector[iterator->second];
			const RenderTargetTextureSignature& renderTargetTextureSignature = renderTargetTextureElement.renderTargetTextureSignature;
			if (nullptr != outRenderTargetTextureSignature)
			{
				*outRenderTargetTextureSignature = &renderTargetTextureSignature;
			}
			// if (renderTargetTextureSignature.getRenderTargetTextureSignatureId() == renderTargetTextureSignatureId)	// TODO(co) The render target texture and framebuffer handling is still under construction regarding recycling RHI resources etc. - so for now, just add render target textures to have something to start with
			{
				// Do we need to create the RHI texture instance right now?
				if (nullptr == renderTargetTextureElement.texture)
				{
					// Get the texture width and height and apply resolution scale in case the main compositor workspace render target is used
					uint32_t width = renderTargetTextureSignature.getWidth();
					uint32_t height = renderTargetTextureSignature.getHeight();
					if (isInvalid(width) || isInvalid(height))
					{
						uint32_t renderTargetWidth = 1;
						uint32_t renderTargetHeight = 1;
						renderTarget.getWidthAndHeight(renderTargetWidth, renderTargetHeight);
						if ((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::ALLOW_RESOLUTION_SCALE) == 0)
						{
							resolutionScale = 1.0f;
						}
						if (isInvalid(width))
						{
							width = static_cast<uint32_t>(static_cast<float>(renderTargetWidth) * resolutionScale * renderTargetTextureSignature.getWidthScale());
							if (width < 1)
							{
								width = 1;
							}
						}
						if (isInvalid(height))
						{
							height = static_cast<uint32_t>(static_cast<float>(renderTargetHeight) * resolutionScale * renderTargetTextureSignature.getHeightScale());
							if (height < 1)
							{
								height = 1;
							}
						}
					}

					// Get texture flags
					uint32_t textureFlags = 0;
					if ((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::UNORDERED_ACCESS) != 0)
					{
						textureFlags |= Rhi::TextureFlag::UNORDERED_ACCESS;
					}
					if ((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::SHADER_RESOURCE) != 0)
					{
						textureFlags |= Rhi::TextureFlag::SHADER_RESOURCE;
					}
					if ((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::RENDER_TARGET) != 0)
					{
						textureFlags |= Rhi::TextureFlag::RENDER_TARGET;
					}
					if ((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::GENERATE_MIPMAPS) != 0)
					{
						textureFlags |= Rhi::TextureFlag::GENERATE_MIPMAPS;
						textureFlags |= Rhi::TextureFlag::RENDER_TARGET;	// Needed when generating mipmaps
					}

					// Create the texture instance, but without providing texture data (we use the texture as render target)
					// -> Use the "Rhi::TextureFlag::RENDER_TARGET"-flag to mark this texture as a render target
					// -> Required for Vulkan, Direct3D 9, Direct3D 10, Direct3D 11 and Direct3D 12
					// -> Not required for OpenGL and OpenGL ES 3
					// -> The optimized texture clear value is a Direct3D 12 related option
					renderTargetTextureElement.texture = mRenderer.getTextureManager().createTexture2D(width, height, renderTargetTextureSignature.getTextureFormat(), nullptr, textureFlags, Rhi::TextureUsage::DEFAULT, (((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::ALLOW_MULTISAMPLE) != 0) ? numberOfMultisamples : 1u), nullptr RHI_RESOURCE_DEBUG_NAME("Render target texture manager"));
					renderTargetTextureElement.texture->addReference();

					{ // Tell the texture resource manager about our render target texture so it can be referenced inside e.g. compositor nodes
						TextureResourceManager& textureResourceManager = mRenderer.getTextureResourceManager();
						TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(assetId);
						if (nullptr == textureResource)
						{
							// Create texture resource
							textureResourceManager.createTextureResourceByAssetId(assetId, *renderTargetTextureElement.texture);
						}
						else
						{
							// Update texture resource
							textureResource->setTexture(renderTargetTextureElement.texture);
						}
					}
				}
				texture = renderTargetTextureElement.texture;
				// break;	// TODO(co) The render target texture and framebuffer handling is still under construction regarding recycling RHI resources etc. - so for now, just add render target textures to have something to start with
			}
			ASSERT(nullptr != texture, "Invalid texture")
		}
		else
		{
			// Error!
			ASSERT(false, "Unknown asset ID, this shouldn't have happened")
			if (nullptr != outRenderTargetTextureSignature)
			{
				*outRenderTargetTextureSignature = nullptr;
			}
		}

		// Done
		return texture;
	}

	void RenderTargetTextureManager::releaseRenderTargetTextureBySignature(const RenderTargetTextureSignature& renderTargetTextureSignature)
	{
		const RenderTargetTextureElement renderTargetTextureElement(renderTargetTextureSignature);
		SortedRenderTargetTextureVector::iterator iterator = std::lower_bound(mSortedRenderTargetTextureVector.begin(), mSortedRenderTargetTextureVector.end(), renderTargetTextureElement, ::detail::orderRenderTargetTextureElementByRenderTargetTextureSignatureId);
		if (iterator != mSortedRenderTargetTextureVector.end() && iterator->renderTargetTextureSignature.getRenderTargetTextureSignatureId() == renderTargetTextureElement.renderTargetTextureSignature.getRenderTargetTextureSignatureId())
		{
			// Was this the last reference?
			if (1 == iterator->numberOfReferences)
			{
				{ // Unload texture resource
					TextureResource* textureResource = mRenderer.getTextureResourceManager().getTextureResourceByAssetId(renderTargetTextureElement.assetId);
					if (nullptr != textureResource)
					{
						textureResource->setTexture(nullptr);
					}
				}

				// Release RHI texture reference
				if (nullptr != iterator->texture)
				{
					iterator->texture->releaseReference();
				}

				// Destroy render target texture instance
				mSortedRenderTargetTextureVector.erase(iterator);
			}
			else
			{
				--iterator->numberOfReferences;
			}
		}
		else
		{
			// Error!
			ASSERT(false, "Render target texture signature isn't registered")
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
