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
#include "RendererRuntime/Public/Resource/ShaderPiece/Loader/ShaderPieceResourceLoader.h"
#include "RendererRuntime/Public/Resource/ShaderPiece/Loader/ShaderPieceFileFormat.h"
#include "RendererRuntime/Public/Resource/ShaderPiece/ShaderPieceResource.h"
#include "RendererRuntime/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/IRendererRuntime.h"

#include <unordered_set>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	void ShaderPieceResourceLoader::initialize(const Asset& asset, bool reload, IResource& resource)
	{
		IResourceLoader::initialize(asset, reload);
		mShaderPieceResource = static_cast<ShaderPieceResource*>(&resource);
	}

	void ShaderPieceResourceLoader::onDeserialization(IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data
		mMemoryFile.loadLz4CompressedDataFromFile(v1ShaderPiece::FORMAT_TYPE, v1ShaderPiece::FORMAT_VERSION, file);
	}

	void ShaderPieceResourceLoader::onProcessing()
	{
		// Decompress LZ4 compressed data
		mMemoryFile.decompress();

		// Read in the shader piece header
		v1ShaderPiece::ShaderPieceHeader shaderPieceHeader;
		mMemoryFile.read(&shaderPieceHeader, sizeof(v1ShaderPiece::ShaderPieceHeader));

		// Sanity check
		assert((shaderPieceHeader.numberOfShaderSourceCodeBytes > 0) && "Invalid shader piece asset without any shader source code detected");

		// Allocate more temporary memory, if required
		if (mMaximumNumberOfShaderSourceCodeBytes < shaderPieceHeader.numberOfShaderSourceCodeBytes)
		{
			mMaximumNumberOfShaderSourceCodeBytes = shaderPieceHeader.numberOfShaderSourceCodeBytes;
			delete [] mShaderSourceCode;
			mShaderSourceCode = new char[mMaximumNumberOfShaderSourceCodeBytes];
		}

		// Read the shader piece ASCII source code
		mMemoryFile.read(mShaderSourceCode, shaderPieceHeader.numberOfShaderSourceCodeBytes);
		mShaderPieceResource->mShaderSourceCode.assign(mShaderSourceCode, mShaderSourceCode + shaderPieceHeader.numberOfShaderSourceCodeBytes);
	}

	bool ShaderPieceResourceLoader::onDispatch()
	{
		// TODO(co) Cleanup: Get all influenced material blueprint resources
		if (getReload())
		{
			const ShaderPieceResourceId shaderPieceResourceId = mShaderPieceResource->getId();
			const ShaderBlueprintResourceManager& shaderBlueprintResourceManager = mRendererRuntime.getShaderBlueprintResourceManager();
			typedef std::unordered_set<MaterialBlueprintResource*> MaterialBlueprintResourcePointers;
			MaterialBlueprintResourcePointers materialBlueprintResourcePointers;
			const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRendererRuntime.getMaterialBlueprintResourceManager();
			const uint32_t numberOfElements = materialBlueprintResourceManager.getNumberOfResources();
			for (uint32_t i = 0; i < numberOfElements; ++i)
			{
				MaterialBlueprintResource& materialBlueprintResource = materialBlueprintResourceManager.getByIndex(i);
				ShaderBlueprintResourceId shaderBlueprintResourceId = materialBlueprintResource.getComputeShaderBlueprintResourceId();
				if (isValid(shaderBlueprintResourceId))
				{
					// Compute pipeline state object (PSO)
					const ShaderBlueprintResource::IncludeShaderPieceResourceIds& includeShaderPieceResourceIds = shaderBlueprintResourceManager.getById(shaderBlueprintResourceId).getIncludeShaderPieceResourceIds();
					if (std::find(includeShaderPieceResourceIds.cbegin(), includeShaderPieceResourceIds.cend(), shaderPieceResourceId) != includeShaderPieceResourceIds.cend())
					{
						materialBlueprintResourcePointers.insert(&materialBlueprintResource);
					}
				}
				else
				{
					// Graphics pipeline state object (PSO)
					for (uint8_t graphicsShaderType = 0; graphicsShaderType < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++graphicsShaderType)
					{
						shaderBlueprintResourceId = materialBlueprintResource.getGraphicsShaderBlueprintResourceId(static_cast<GraphicsShaderType>(graphicsShaderType));
						if (isValid(shaderBlueprintResourceId))
						{
							const ShaderBlueprintResource::IncludeShaderPieceResourceIds& includeShaderPieceResourceIds = shaderBlueprintResourceManager.getById(shaderBlueprintResourceId).getIncludeShaderPieceResourceIds();
							if (std::find(includeShaderPieceResourceIds.cbegin(), includeShaderPieceResourceIds.cend(), shaderPieceResourceId) != includeShaderPieceResourceIds.cend())
							{
								materialBlueprintResourcePointers.insert(&materialBlueprintResource);
								break;
							}
						}
					}
				}
			}
			for (MaterialBlueprintResource* materialBlueprintResource : materialBlueprintResourcePointers)
			{
				materialBlueprintResource->getGraphicsPipelineStateCacheManager().clearCache();
				materialBlueprintResource->getGraphicsPipelineStateCacheManager().getGraphicsProgramCacheManager().clearCache();
				materialBlueprintResource->getComputePipelineStateCacheManager().clearCache();
			}

			// TODO(co) Do only clear the influenced shader cache entries
			mRendererRuntime.getShaderBlueprintResourceManager().getShaderCacheManager().clearCache();
		}

		// Fully loaded
		return true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
