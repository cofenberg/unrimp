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
#include "Renderer/Public/Resource/Material/MaterialResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialResource.h"
#include "Renderer/Public/Resource/Material/MaterialTechnique.h"
#include "Renderer/Public/Resource/Material/Loader/MaterialResourceLoader.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "Renderer/Public/Resource/ResourceManagerTemplate.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	MaterialResource* MaterialResourceManager::getMaterialResourceByAssetId(AssetId assetId) const
	{
		return mInternalResourceManager->getResourceByAssetId(assetId);
	}

	MaterialResourceId MaterialResourceManager::getMaterialResourceIdByAssetId(AssetId assetId) const
	{
		const MaterialResource* materialResource = getMaterialResourceByAssetId(assetId);
		return (nullptr != materialResource) ? materialResource->getId() : getInvalid<MaterialResourceId>();
	}

	void MaterialResourceManager::loadMaterialResourceByAssetId(AssetId assetId, MaterialResourceId& materialResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, materialResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	MaterialResourceId MaterialResourceManager::createMaterialResourceByAssetId(AssetId assetId, AssetId materialBlueprintAssetId, MaterialTechniqueId materialTechniqueId)
	{
		// Sanity check
		RHI_ASSERT(mRenderer.getContext(), nullptr == getMaterialResourceByAssetId(assetId), "Material resource is not allowed to exist, yet")

		// Create the material resource instance
		MaterialResource& materialResource = mInternalResourceManager->getResources().addElement();
		materialResource.setResourceManager(this);
		materialResource.setAssetId(assetId);
		#ifdef RHI_DEBUG
		{
			const AssetManager& assetManager = mRenderer.getAssetManager();
			const VirtualFilename virtualFilename = assetManager.tryGetVirtualFilenameByAssetId(assetId);
			if (nullptr != virtualFilename)
			{
				if (assetId == materialBlueprintAssetId)
				{
					materialResource.setDebugName((IFileManager::INVALID_CHARACTER + std::string("[CreatedMaterial][InstanceOfMaterialBlueprintAsset=\"") + std::string(virtualFilename) + "\"]").c_str());
				}
				else
				{
					materialResource.setDebugName((IFileManager::INVALID_CHARACTER + std::string("[CreatedMaterial][Asset=\"") + std::string(virtualFilename) + "\"][MaterialBlueprintAsset=\"" + assetManager.getAssetByAssetId(materialBlueprintAssetId).virtualFilename + "\"]").c_str());
				}
			}
			else
			{
				materialResource.setDebugName((IFileManager::INVALID_CHARACTER + std::string("[CreatedMaterial][AssetId=") + std::to_string(assetId) + "][MaterialBlueprintAsset=\"" + assetManager.getAssetByAssetId(materialBlueprintAssetId).virtualFilename + "\"]").c_str());
			}
		}
		#endif

		{ // Setup material resource instance
			// Copy over the material properties of the material blueprint resource
			MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRenderer.getMaterialBlueprintResourceManager();
			MaterialBlueprintResourceId materialBlueprintResourceId = getInvalid<MaterialBlueprintResourceId>();
			materialBlueprintResourceManager.loadMaterialBlueprintResourceByAssetId(materialBlueprintAssetId, materialBlueprintResourceId);
			MaterialBlueprintResource* materialBlueprintResource = materialBlueprintResourceManager.tryGetById(materialBlueprintResourceId);
			if (nullptr != materialBlueprintResource)
			{
				// TODO(co) Possible optimization: Right now we don't filter for "Renderer::MaterialProperty::Usage::GLOBAL_REFERENCE_FALLBACK" properties.
				//          Only the material blueprint resource needs to store such properties while they're useless inside material resources. The filtering
				//          makes the following more complex and it might not bring any real benefit. So, review this place in here later when we have more pressure on the system.
				materialResource.mMaterialProperties = materialBlueprintResource->mMaterialProperties;

				// Create default material technique
				materialResource.mSortedMaterialTechniqueVector.push_back(new MaterialTechnique(materialTechniqueId, materialResource, materialBlueprintResourceId));
			}
			else
			{
				// Error!
				RHI_ASSERT(mRenderer.getContext(), false, "Invalid material blueprint resource")
			}
		}

		// Done
		setResourceLoadingState(materialResource, IResource::LoadingState::LOADED);
		return materialResource.getId();
	}

	MaterialResourceId MaterialResourceManager::createMaterialResourceByCloning(MaterialResourceId parentMaterialResourceId, AssetId assetId)
	{
		RHI_ASSERT(mRenderer.getContext(), mInternalResourceManager->getResources().getElementById(parentMaterialResourceId).getLoadingState() == IResource::LoadingState::LOADED, "Invalid parent material resource ID")

		// Create the material resource instance
		MaterialResource& materialResource = mInternalResourceManager->getResources().addElement();
		materialResource.setResourceManager(this);
		materialResource.setAssetId(assetId);
		materialResource.setParentMaterialResourceId(parentMaterialResourceId);
		#ifdef RHI_DEBUG
			materialResource.setDebugName((std::string(mInternalResourceManager->getResources().getElementById(parentMaterialResourceId).getDebugName()) + "[Clone]").c_str());
		#endif

		// Done
		setResourceLoadingState(materialResource, IResource::LoadingState::LOADED);
		return materialResource.getId();
	}

	void MaterialResourceManager::destroyMaterialResource(MaterialResourceId materialResourceId)
	{
		mInternalResourceManager->getResources().removeElement(materialResourceId);
	}

	void MaterialResourceManager::setInvalidResourceId(MaterialResourceId& materialResourceId, IResourceListener& resourceListener) const
	{
		MaterialResource* materialResource = tryGetById(materialResourceId);
		if (nullptr != materialResource)
		{
			materialResource->disconnectResourceListener(resourceListener);
		}
		setInvalid(materialResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceManager methods     ]
	//[-------------------------------------------------------]
	uint32_t MaterialResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& MaterialResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& MaterialResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* MaterialResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void MaterialResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		return mInternalResourceManager->reloadResourceByAssetId(assetId);
	}


	//[-------------------------------------------------------]
	//[ Private virtual Renderer::IResourceManager methods    ]
	//[-------------------------------------------------------]
	IResourceLoader* MaterialResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	MaterialResourceManager::MaterialResourceManager(IRenderer& renderer) :
		mRenderer(renderer)
	{
		mInternalResourceManager = new ResourceManagerTemplate<MaterialResource, MaterialResourceLoader, MaterialResourceId, 4096>(renderer, *this);
	}

	MaterialResourceManager::~MaterialResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
