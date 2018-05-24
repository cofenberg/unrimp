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
#include "RendererRuntime/PrecompiledHeader.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "RendererRuntime/Resource/ShaderBlueprint/Loader/ShaderBlueprintResourceLoader.h"
#include "RendererRuntime/Resource/Detail/ResourceManagerTemplate.h"


// Disable warnings
// TODO(co) See "RendererRuntime::ShaderBlueprintResourceManager::ShaderBlueprintResourceManager()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void ShaderBlueprintResourceManager::loadShaderBlueprintResourceByAssetId(AssetId assetId, ShaderBlueprintResourceId& shaderBlueprintResourceId, IResourceListener* resourceListener, bool reload, ResourceLoaderTypeId resourceLoaderTypeId)
	{
		mInternalResourceManager->loadResourceByAssetId(assetId, shaderBlueprintResourceId, resourceListener, reload, resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceManager methods ]
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
	//[ Private virtual RendererRuntime::IResourceManager methods ]
	//[-------------------------------------------------------]
	IResourceLoader* ShaderBlueprintResourceManager::createResourceLoaderInstance(ResourceLoaderTypeId resourceLoaderTypeId)
	{
		return mInternalResourceManager->createResourceLoaderInstance(resourceLoaderTypeId);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	ShaderBlueprintResourceManager::ShaderBlueprintResourceManager(IRendererRuntime& rendererRuntime) :
		mRendererRuntime(rendererRuntime),
		mShaderCacheManager(*this)
	{
		mInternalResourceManager = new ResourceManagerTemplate<ShaderBlueprintResource, ShaderBlueprintResourceLoader, ShaderBlueprintResourceId, 64>(rendererRuntime, *this);

		// Gather renderer shader properties
		// -> Write the renderer name as well as the shader language name into the shader properties so shaders can perform renderer specific handling if required
		// -> We really need both, usually shader language name is sufficient, but if more fine granular information is required it's accessible
		Renderer::IRenderer& renderer = mRendererRuntime.getRenderer();
		const Renderer::Capabilities& capabilities = mRendererRuntime.getRenderer().getCapabilities();
		mRendererShaderProperties.setPropertyValue(static_cast<uint32_t>(renderer.getNameId()), 1);
		mRendererShaderProperties.setPropertyValue(STRING_ID("ZeroToOneClipZ"), capabilities.zeroToOneClipZ ? 1 : 0);
		mRendererShaderProperties.setPropertyValue(STRING_ID("UpperLeftOrigin"), capabilities.upperLeftOrigin ? 1 : 0);
		const Renderer::IShaderLanguage* shaderLanguage = renderer.getShaderLanguage();
		if (nullptr != shaderLanguage)
		{
			mRendererShaderProperties.setPropertyValue(StringId(shaderLanguage->getShaderLanguageName()), 1);
		}
	}

	ShaderBlueprintResourceManager::~ShaderBlueprintResourceManager()
	{
		delete mInternalResourceManager;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
