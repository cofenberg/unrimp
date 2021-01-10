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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Loader/ShaderBlueprintResourceLoader.h"
#include "Renderer/Public/Resource/ResourceManagerTemplate.h"


// Disable warnings
// TODO(co) See "Renderer::ShaderBlueprintResourceManager::ShaderBlueprintResourceManager()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void ShaderBlueprintResourceManager::loadShaderBlueprintResourceByAssetId(AssetId assetId, ShaderBlueprintResourceId& shaderBlueprintResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, shaderBlueprintResourceId, resourceListener, reload, resourceLoaderTypeId);
	}

	void ShaderBlueprintResourceManager::setInvalidResourceId(ShaderBlueprintResourceId& shaderBlueprintResourceId, IResourceListener& resourceListener) const
	{
		ShaderBlueprintResource* shaderBlueprintResource = tryGetById(shaderBlueprintResourceId);
		if (nullptr != shaderBlueprintResource)
		{
			shaderBlueprintResource->disconnectResourceListener(resourceListener);
		}
		setInvalid(shaderBlueprintResourceId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceManager methods     ]
	//[-------------------------------------------------------]
	uint32_t ShaderBlueprintResourceManager::getNumberOfResources() const
	{
		return mInternalResourceManager->getResources().getNumberOfElements();
	}

	IResource& ShaderBlueprintResourceManager::getResourceByIndex(uint32_t index) const
	{
		return mInternalResourceManager->getResources().getElementByIndex(index);
	}

	IResource& ShaderBlueprintResourceManager::getResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().getElementById(resourceId);
	}

	IResource* ShaderBlueprintResourceManager::tryGetResourceByResourceId(ResourceId resourceId) const
	{
		return mInternalResourceManager->getResources().tryGetElementById(resourceId);
	}

	void ShaderBlueprintResourceManager::reloadResourceByAssetId(AssetId assetId)
	{
		return mInternalResourceManager->reloadResourceByAssetId(assetId);
	}


	//[-------------------------------------------------------]
	//[ Private virtual Renderer::IResourceManager methods    ]
	//[-------------------------------------------------------]
	IResourceLoader* ShaderBlueprintResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	ShaderBlueprintResourceManager::ShaderBlueprintResourceManager(IRenderer& renderer) :
		mRenderer(renderer),
		mShaderCacheManager(*this)
	{
		mInternalResourceManager = new ResourceManagerTemplate<ShaderBlueprintResource, ShaderBlueprintResourceLoader, ShaderBlueprintResourceId, 128>(renderer, *this);

		// Gather RHI shader properties
		// -> Write the RHI name as well as the shader language name into the shader properties so shaders can perform RHI specific handling if required
		// -> We really need both, usually shader language name is sufficient, but if more fine granular information is required it's accessible
		Rhi::IRhi& rhi = mRenderer.getRhi();
		const Rhi::Capabilities& capabilities = rhi.getCapabilities();
		mRhiShaderProperties.setPropertyValue(static_cast<uint32_t>(rhi.getNameId()), 1);
		mRhiShaderProperties.setPropertyValue(STRING_ID("ZeroToOneClipZ"), capabilities.zeroToOneClipZ ? 1 : 0);
		mRhiShaderProperties.setPropertyValue(STRING_ID("UpperLeftOrigin"), capabilities.upperLeftOrigin ? 1 : 0);
		mRhiShaderProperties.setPropertyValue(StringId(rhi.getDefaultShaderLanguage().getShaderLanguageName()), 1);
	}

	ShaderBlueprintResourceManager::~ShaderBlueprintResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
