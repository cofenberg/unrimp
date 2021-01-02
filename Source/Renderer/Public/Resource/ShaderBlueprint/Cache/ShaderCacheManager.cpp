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
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderCacheManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderCache.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderBuilder.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "Renderer/Public/Resource/VertexAttributes/VertexAttributesResourceManager.h"
#include "Renderer/Public/Resource/VertexAttributes/VertexAttributesResource.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "Renderer/Public/Asset/AssetManager.h"
#include "Renderer/Public/Core/File/MemoryFile.h"
#include "Renderer/Public/Core/Math/Math.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	ShaderCache* ShaderCacheManager::getGraphicsShaderCache(const GraphicsPipelineStateSignature& graphicsPipelineStateSignature, const MaterialBlueprintResource& materialBlueprintResource, Rhi::IShaderLanguage& shaderLanguage, GraphicsShaderType graphicsShaderType)
	{
		ShaderCache* shaderCache = nullptr;

		// Get the shader blueprint resource ID
		const ShaderBlueprintResourceId shaderBlueprintResourceId = materialBlueprintResource.getGraphicsShaderBlueprintResourceId(graphicsShaderType);
		if (isValid(shaderBlueprintResourceId))
		{
			// Get the shader cache identifier, often but not always identical to the shader combination ID
			const ShaderCacheId shaderCacheId = graphicsPipelineStateSignature.getShaderCombinationId(graphicsShaderType);

			// Does the shader cache already exist?
			std::unique_lock<std::mutex> mutexLock(mMutex);
			ShaderCacheByShaderCacheId::const_iterator shaderCacheIdIterator = mShaderCacheByShaderCacheId.find(shaderCacheId);
			if (shaderCacheIdIterator != mShaderCacheByShaderCacheId.cend())
			{
				shaderCache = shaderCacheIdIterator->second;
				if (nullptr != shaderCache->getMasterShaderCache())
				{
					shaderCache = shaderCache->getMasterShaderCache();
				}

				// Create RHI shader instance using the shader bytecode, if necessary
				if (nullptr == shaderCache->mShaderPtr.getPointer())
				{
					ASSERT(0 != shaderCache->mShaderBytecode.getNumberOfBytes(), "A shader cache must always have a valid shader bytecode, else it's a pointless shader cache. This might be the result of a shader compilation error.")
					if (0 != shaderCache->mShaderBytecode.getNumberOfBytes())
					{
						switch (graphicsShaderType)
						{
							case GraphicsShaderType::Vertex:
							{
								const Rhi::VertexAttributes& vertexAttributes = mShaderBlueprintResourceManager.getRenderer().getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes();
								shaderCache->mShaderPtr = shaderLanguage.createVertexShaderFromBytecode(vertexAttributes, shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME("From bytecode"));
								break;
							}

							case GraphicsShaderType::TessellationControl:
								shaderCache->mShaderPtr = shaderLanguage.createTessellationControlShaderFromBytecode(shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME("From bytecode"));
								break;

							case GraphicsShaderType::TessellationEvaluation:
								shaderCache->mShaderPtr = shaderLanguage.createTessellationEvaluationShaderFromBytecode(shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME("From bytecode"));
								break;

							case GraphicsShaderType::Geometry:
								// TODO(co) "Renderer::ShaderCacheManager::getGraphicsShaderCache()" needs to provide additional geometry shader information
								// shaderCache->mShaderPtr = shaderLanguage.createGeometryShaderFromBytecode(shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME("From bytecode"));
								break;

							case GraphicsShaderType::Fragment:
								shaderCache->mShaderPtr = shaderLanguage.createFragmentShaderFromBytecode(shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME("From bytecode"));
								break;
						}
					}
				}
			}
			else
			{
				// Try to create the new graphics shader cache instance
				const ShaderBlueprintResource* shaderBlueprintResource = mShaderBlueprintResourceManager.tryGetById(shaderBlueprintResourceId);
				if (nullptr != shaderBlueprintResource)
				{
					// Build the shader source code
					const IRenderer& renderer = mShaderBlueprintResourceManager.getRenderer();
					ShaderBuilder shaderBuilder(renderer.getRhi().getContext());
					ShaderBuilder::BuildShader buildShader;
					shaderBuilder.createSourceCode(renderer.getShaderPieceResourceManager(), *shaderBlueprintResource, graphicsPipelineStateSignature.getShaderProperties(), buildShader);
					std::string& sourceCode = buildShader.sourceCode;
					if (sourceCode.empty())
					{
						// TODO(co) Error handling
						ASSERT(false, "Invalid source code")
					}
					else
					{
						// Add the virtual filename of the shader blueprint asset as first shader source code line to make shader debugging easier
						const Asset& shaderBlueprintAsset = renderer.getAssetManager().getAssetByAssetId(shaderBlueprintResource->getAssetId());
						sourceCode = std::string("// ") + shaderBlueprintAsset.virtualFilename + '\n' + sourceCode;

						// Generate the shader source code ID
						// -> Especially in complex shaders, there are situations where different shader combinations result in one and the same shader source code
						// -> Shader compilation is considered to be expensive, so we need to be pretty sure that we really need to perform this heavy work
						const ShaderSourceCodeId shaderSourceCodeId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(sourceCode.c_str()), static_cast<uint32_t>(sourceCode.size()));
						ShaderCacheByShaderSourceCodeId::const_iterator shaderSourceCodeIdIterator = mShaderCacheByShaderSourceCodeId.find(shaderSourceCodeId);
						if (shaderSourceCodeIdIterator != mShaderCacheByShaderSourceCodeId.cend())
						{
							// Reuse already existing shader instance
							// -> We still have to create a shader cache instance so we don't need to build the shader source code again next time
							shaderCache = new ShaderCache(shaderCacheId, mShaderCacheByShaderCacheId.find(shaderSourceCodeIdIterator->second)->second);
							mShaderCacheByShaderCacheId.emplace(shaderCacheId, shaderCache);
							mCacheNeedsSaving = true;
						}
						else
						{
							// Create the RHI shader instance
							Rhi::IShader* shader = nullptr;
							shaderCache = new ShaderCache(shaderCacheId);
							shaderCache->mAssetIds = buildShader.assetIds;
							shaderCache->mCombinedAssetFileHashes = buildShader.combinedAssetFileHashes;
							switch (graphicsShaderType)
							{
								case GraphicsShaderType::Vertex:
								{
									const Rhi::VertexAttributes& vertexAttributes = mShaderBlueprintResourceManager.getRenderer().getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes();
									shader = shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, sourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME(shaderBlueprintAsset.virtualFilename));
									break;
								}

								case GraphicsShaderType::TessellationControl:
									shader = shaderLanguage.createTessellationControlShaderFromSourceCode(sourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME(shaderBlueprintAsset.virtualFilename));
									break;

								case GraphicsShaderType::TessellationEvaluation:
									shader = shaderLanguage.createTessellationEvaluationShaderFromSourceCode(sourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME(shaderBlueprintAsset.virtualFilename));
									break;

								case GraphicsShaderType::Geometry:
									// TODO(co) "Renderer::ShaderCacheManager::getShaderCache()" needs to provide additional geometry shader information
									// shader = shaderLanguage.createGeometryShaderFromSourceCode(sourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME(shaderBlueprintAsset.virtualFilename));
									break;

								case GraphicsShaderType::Fragment:
									shader = shaderLanguage.createFragmentShaderFromSourceCode(sourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME(shaderBlueprintAsset.virtualFilename));
									break;
							}

							// Create the new shader cache instance
							if (nullptr != shader)
							{
								ASSERT(!shaderLanguage.getRhi().getCapabilities().shaderBytecode || 0 != shaderCache->mShaderBytecode.getNumberOfBytes(), "Invalid shader bytecode received from RHI implementation")
								shaderCache->mShaderPtr = shader;
								mShaderCacheByShaderCacheId.emplace(shaderCacheId, shaderCache);
								mShaderCacheByShaderSourceCodeId.emplace(shaderSourceCodeId, shaderCacheId);
								mCacheNeedsSaving = true;
							}
							else
							{
								// TODO(co) Error handling
								delete shaderCache;
								shaderCache = nullptr;
								ASSERT(false, "Invalid shader")
							}
						}
					}
				}
				else
				{
					// TODO(co) Error handling
					ASSERT(false, "Invalid shader blueprint resource")
				}
			}
		}

		// Done
		return shaderCache;
	}

	ShaderCache* ShaderCacheManager::getComputeShaderCache(const ComputePipelineStateSignature& computePipelineStateSignature, const MaterialBlueprintResource& materialBlueprintResource, Rhi::IShaderLanguage& shaderLanguage)
	{
		ShaderCache* shaderCache = nullptr;

		// Get the shader blueprint resource ID
		const ShaderBlueprintResourceId shaderBlueprintResourceId = materialBlueprintResource.getComputeShaderBlueprintResourceId();
		if (isValid(shaderBlueprintResourceId))
		{
			// Get the shader cache identifier, often but not always identical to the shader combination ID
			const ShaderCacheId shaderCacheId = computePipelineStateSignature.getShaderCombinationId();

			// Does the shader cache already exist?
			std::unique_lock<std::mutex> mutexLock(mMutex);
			ShaderCacheByShaderCacheId::const_iterator shaderCacheIdIterator = mShaderCacheByShaderCacheId.find(shaderCacheId);
			if (shaderCacheIdIterator != mShaderCacheByShaderCacheId.cend())
			{
				shaderCache = shaderCacheIdIterator->second;
				if (nullptr != shaderCache->getMasterShaderCache())
				{
					shaderCache = shaderCache->getMasterShaderCache();
				}

				// Create RHI shader instance using the shader bytecode, if necessary
				if (nullptr == shaderCache->mShaderPtr.getPointer())
				{
					ASSERT(0 != shaderCache->mShaderBytecode.getNumberOfBytes(), "A shader cache must always have a valid shader bytecode, else it's a pointless shader cache. This might be the result of a shader compilation error.")
					shaderCache->mShaderPtr = shaderLanguage.createComputeShaderFromBytecode(shaderCache->mShaderBytecode);
				}
			}
			else
			{
				// Try to create the new compute shader cache instance
				const ShaderBlueprintResource* shaderBlueprintResource = mShaderBlueprintResourceManager.tryGetById(shaderBlueprintResourceId);
				if (nullptr != shaderBlueprintResource)
				{
					// Build the shader source code
					const IRenderer& renderer = mShaderBlueprintResourceManager.getRenderer();
					ShaderBuilder shaderBuilder(renderer.getRhi().getContext());
					ShaderBuilder::BuildShader buildShader;
					shaderBuilder.createSourceCode(renderer.getShaderPieceResourceManager(), *shaderBlueprintResource, computePipelineStateSignature.getShaderProperties(), buildShader);
					std::string& sourceCode = buildShader.sourceCode;
					if (sourceCode.empty())
					{
						// TODO(co) Error handling
						ASSERT(false, "Invalid source code")
					}
					else
					{
						// Add the virtual filename of the shader blueprint asset as first shader source code line to make shader debugging easier
						const Asset& shaderBlueprintAsset = renderer.getAssetManager().getAssetByAssetId(shaderBlueprintResource->getAssetId());
						sourceCode = std::string("// ") + shaderBlueprintAsset.virtualFilename + '\n' + sourceCode;

						// Generate the shader source code ID
						// -> Especially in complex shaders, there are situations where different shader combinations result in one and the same shader source code
						// -> Shader compilation is considered to be expensive, so we need to be pretty sure that we really need to perform this heavy work
						const ShaderSourceCodeId shaderSourceCodeId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(sourceCode.c_str()), static_cast<uint32_t>(sourceCode.size()));
						ShaderCacheByShaderSourceCodeId::const_iterator shaderSourceCodeIdIterator = mShaderCacheByShaderSourceCodeId.find(shaderSourceCodeId);
						if (shaderSourceCodeIdIterator != mShaderCacheByShaderSourceCodeId.cend())
						{
							// Reuse already existing shader instance
							// -> We still have to create a shader cache instance so we don't need to build the shader source code again next time
							shaderCache = new ShaderCache(shaderCacheId, mShaderCacheByShaderCacheId.find(shaderSourceCodeIdIterator->second)->second);
							mShaderCacheByShaderCacheId.emplace(shaderCacheId, shaderCache);
							mCacheNeedsSaving = true;
						}
						else
						{
							// Create the RHI shader instance
							shaderCache = new ShaderCache(shaderCacheId);
							shaderCache->mAssetIds = buildShader.assetIds;
							shaderCache->mCombinedAssetFileHashes = buildShader.combinedAssetFileHashes;
							Rhi::IShader* shader = shaderLanguage.createComputeShaderFromSourceCode(sourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME(shaderBlueprintAsset.virtualFilename));

							// Create the new shader cache instance
							if (nullptr != shader)
							{
								ASSERT(!shaderLanguage.getRhi().getCapabilities().shaderBytecode || 0 != shaderCache->mShaderBytecode.getNumberOfBytes(), "Invalid shader bytecode received from RHI implementation")
								shaderCache->mShaderPtr = shader;
								mShaderCacheByShaderCacheId.emplace(shaderCacheId, shaderCache);
								mShaderCacheByShaderSourceCodeId.emplace(shaderSourceCodeId, shaderCacheId);
								mCacheNeedsSaving = true;
							}
							else
							{
								// TODO(co) Error handling
								delete shaderCache;
								shaderCache = nullptr;
								ASSERT(false, "Invalid shader")
							}
						}
					}
				}
				else
				{
					// TODO(co) Error handling
					ASSERT(false, "Invalid shader blueprint resource")
				}
			}
		}

		// Done
		return shaderCache;
	}

	void ShaderCacheManager::clearCache()
	{
		std::unique_lock<std::mutex> mutexLock(mMutex);
		if (!mShaderCacheByShaderCacheId.empty() || !mShaderCacheByShaderSourceCodeId.empty())
		{
			for (auto& shaderCacheElement : mShaderCacheByShaderCacheId)
			{
				delete shaderCacheElement.second;
			}
			mShaderCacheByShaderCacheId.clear();
			mShaderCacheByShaderSourceCodeId.clear();
			mCacheNeedsSaving = true;
		}
	}

	void ShaderCacheManager::loadCache(IFile& file)
	{
		// TODO(co) This can certainly be implemented in a more efficient way. For instance, we could store all shader bytecodes inside a LZ4 compressed buffer and let shader caches directly point into this buffer.
		typedef std::unordered_set<ShaderCacheId> OutOfDateShaderCacheIds;
		OutOfDateShaderCacheIds outOfDateShaderCacheIds;
		const AssetManager& assetManager = mShaderBlueprintResourceManager.getRenderer().getAssetManager();

		{ // Load shader caches
			uint32_t numberOfShaderCaches = getInvalid<uint32_t>();
			file.read(&numberOfShaderCaches, sizeof(uint32_t));
			mShaderCacheByShaderCacheId.reserve(numberOfShaderCaches);
			std::vector<uint8_t> bytecode;
			AssetIds assetIds;
			for (uint32_t i = 0; i < numberOfShaderCaches; ++i)
			{
				ShaderCache* shaderCache = nullptr;

				// Load shader cache
				ShaderCacheId shaderCacheId = getInvalid<ShaderCacheId>();
				file.read(&shaderCacheId, sizeof(ShaderCacheId));
				uint32_t numberOfBytes = getInvalid<uint32_t>();
				file.read(&numberOfBytes, sizeof(uint32_t));
				if (isValid(numberOfBytes))
				{
					// Master shader cache

					// Load list of IDs of the assets (shader blueprint, shader piece) which took part in the shader cache creation
					uint32_t numberOfAssetIds = getInvalid<uint32_t>();
					file.read(&numberOfAssetIds, sizeof(uint32_t));
					ASSERT(0 != numberOfAssetIds, "Invalid number of asset IDs")
					assetIds.resize(numberOfAssetIds);
					file.read(assetIds.data(), sizeof(uint32_t) * numberOfAssetIds);
					uint64_t combinedAssetFileHashes = getInvalid<uint64_t>();
					file.read(&combinedAssetFileHashes, sizeof(uint64_t));

					// Check whether or not the shader cache is still valid
					uint64_t currentCombinedAssetFileHashes = Math::FNV1a_INITIAL_HASH_64;
					for (AssetId assetId : assetIds)
					{
						const Asset* asset = assetManager.tryGetAssetByAssetId(assetId);
						if (nullptr != asset)
						{
							currentCombinedAssetFileHashes = Math::calculateFNV1a64(reinterpret_cast<const uint8_t*>(&asset->fileHash), sizeof(uint64_t), currentCombinedAssetFileHashes);
						}
					}
					if (currentCombinedAssetFileHashes != combinedAssetFileHashes)
					{
						// Shader cache is out-of-date
						file.skip(numberOfBytes);
						outOfDateShaderCacheIds.insert(shaderCacheId);
					}
					else
					{
						// Shader cache is still valid

						// Create shader cache instance
						shaderCache = new ShaderCache(shaderCacheId);
						shaderCache->mAssetIds = assetIds;
						shaderCache->mCombinedAssetFileHashes = combinedAssetFileHashes;

						// Load shader bytecode
						ASSERT(0 != numberOfBytes, "A shader cache must always have a valid shader bytecode, else it's a pointless shader cache. This might be the result of a shader compilation error.")
						if (0 != numberOfBytes)
						{
							bytecode.resize(numberOfBytes);
							file.read(bytecode.data(), numberOfBytes);
							shaderCache->mShaderBytecode.setBytecodeCopy(numberOfBytes, bytecode.data());
						}
					}
				}
				else
				{
					ShaderCacheId masterShaderCacheId = getInvalid<ShaderCacheId>();
					file.read(&masterShaderCacheId, sizeof(ShaderCacheId));
					if (outOfDateShaderCacheIds.find(masterShaderCacheId) == outOfDateShaderCacheIds.cend())
					{
						// Shader cache is still valid
						ShaderCacheByShaderCacheId::const_iterator masterShaderCacheIdIterator = mShaderCacheByShaderCacheId.find(masterShaderCacheId);
						if (masterShaderCacheIdIterator != mShaderCacheByShaderCacheId.cend())
						{
							// Create shader cache instance
							shaderCache = new ShaderCache(shaderCacheId);
							shaderCache->mMasterShaderCache = masterShaderCacheIdIterator->second;
						}
						else
						{
							// Error!
							ASSERT(false, "The shader cache is corrupt since a master shader cache is referenced which doesn't exist")
						}
					}
					else
					{
						// Shader cache is out-of-date
						outOfDateShaderCacheIds.insert(shaderCacheId);
					}
				}

				// Register shader cache
				if (nullptr != shaderCache)
				{
					mShaderCacheByShaderCacheId.emplace(shaderCache->mShaderCacheId, shaderCache);
				}
			}
		}

		{ // Load shader source code ID to shader cache ID mapping
			uint32_t numberOfElements = getInvalid<uint32_t>();
			file.read(&numberOfElements, sizeof(uint32_t));
			mShaderCacheByShaderSourceCodeId.reserve(numberOfElements);
			for (uint32_t i = 0; i < numberOfElements; ++i)
			{
				ShaderSourceCodeId shaderSourceCodeId = getInvalid<ShaderSourceCodeId>();
				file.read(&shaderSourceCodeId, sizeof(ShaderSourceCodeId));
				ShaderCacheId shaderCacheId = getInvalid<ShaderCacheId>();
				file.read(&shaderCacheId, sizeof(ShaderCacheId));
				if (outOfDateShaderCacheIds.find(shaderCacheId) == outOfDateShaderCacheIds.cend())
				{
					// Shader cache is still valid
					mShaderCacheByShaderSourceCodeId.emplace(shaderSourceCodeId, shaderCacheId);
				}
			}
		}

		// Done
		mCacheNeedsSaving = false;
	}

	void ShaderCacheManager::saveCache(IFile& file)
	{
		{ // Save shader caches
		  // -> Shader caches with a master shader cache must come last to ensure the master is already loaded
			const uint32_t numberOfShaderCaches = static_cast<uint32_t>(mShaderCacheByShaderCacheId.size());
			file.write(&numberOfShaderCaches, sizeof(uint32_t));
			std::vector<const ShaderCache*> shaderCachesWithMaster;
			for (auto& shaderCacheElement : mShaderCacheByShaderCacheId)
			{
				const ShaderCache* shaderCache = shaderCacheElement.second;
				if (nullptr == shaderCache->getMasterShaderCache())
				{
					// Master shader cache
					const Rhi::ShaderBytecode& shaderBytecode = shaderCache->mShaderBytecode;
					const uint32_t numberOfBytes = shaderBytecode.getNumberOfBytes();
					ASSERT(0 != numberOfBytes, "A shader cache must always have a valid shader bytecode, else it's a pointless shader cache. This might be the result of a shader compilation error.")
					file.write(&shaderCache->mShaderCacheId, sizeof(ShaderCacheId));
					file.write(&numberOfBytes, sizeof(uint32_t));

					// Write list of IDs of the assets (shader blueprint, shader piece) which took part in the shader cache creation
					const uint32_t numberOfAssetIds = static_cast<uint32_t>(shaderCache->mAssetIds.size());
					ASSERT(0 != numberOfAssetIds, "Invalid number of asset IDs")
					file.write(&numberOfAssetIds, sizeof(uint32_t));
					file.write(shaderCache->mAssetIds.data(), sizeof(uint32_t) * numberOfAssetIds);
					file.write(&shaderCache->mCombinedAssetFileHashes, sizeof(uint64_t));

					// Write shader bytecode
					if (0 != numberOfBytes)
					{
						file.write(shaderBytecode.getBytecode(), numberOfBytes);
					}
				}
				else
				{
					shaderCachesWithMaster.push_back(shaderCache);
				}
			}
			for (const ShaderCache* shaderCache : shaderCachesWithMaster)
			{
				const ShaderCache* masterShaderCache = shaderCache->getMasterShaderCache();
				ASSERT(nullptr != masterShaderCache->getShaderPtr().getPointer(), "A shader cache must always have a valid shader instance, else it's a pointless shader cache")
				file.write(&shaderCache->mShaderCacheId, sizeof(ShaderCacheId));
				const uint32_t numberOfBytes = getInvalid<uint32_t>();
				file.write(&numberOfBytes, sizeof(uint32_t));
				file.write(&masterShaderCache->mShaderCacheId, sizeof(ShaderCacheId));
			}
		}

		{ // Save shader source code ID to shader cache ID mapping
			const uint32_t numberOfElements = static_cast<uint32_t>(mShaderCacheByShaderSourceCodeId.size());
			file.write(&numberOfElements, sizeof(uint32_t));
			for (auto& element : mShaderCacheByShaderSourceCodeId)
			{
				file.write(&element.first, sizeof(ShaderSourceCodeId));
				file.write(&element.second, sizeof(ShaderCacheId));
			}
		}

		// Done
		mCacheNeedsSaving = false;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
