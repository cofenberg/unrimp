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
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCacheManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCompiler.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCache.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	const ComputePipelineStateCache* ComputePipelineStateCacheManager::getComputePipelineStateCacheByCombination(const ShaderProperties& shaderProperties, [[maybe_unused]] bool allowEmergencySynchronousCompilation)
	{
		// TODO(co) Asserts whether or not e.g. the material resource is using the owning material resource blueprint
		assert(IResource::LoadingState::LOADED == mMaterialBlueprintResource.getLoadingState());

		// Generate the compute pipeline state signature
		mTemporaryComputePipelineStateSignature.set(mMaterialBlueprintResource, shaderProperties);
		{
			ComputePipelineStateCacheByComputePipelineStateSignatureId::const_iterator iterator = mComputePipelineStateCacheByComputePipelineStateSignatureId.find(mTemporaryComputePipelineStateSignature.getComputePipelineStateSignatureId());
			if (iterator != mComputePipelineStateCacheByComputePipelineStateSignatureId.cend())
			{
				// There's already a pipeline state cache for the pipeline state signature ID
				// -> We don't care whether or not the pipeline state cache is currently using fallback data due to asynchronous complication
				return iterator->second;
			}
		}

		// Fallback: OK, the pipeline state signature is unknown and we now have to perform more complex and time consuming work. So first, check whether or not this work
		// should be performed asynchronous (usually the case). If asynchronous, we need to return a fallback pipeline state cache while the pipeline state compiler is working.
		ComputePipelineStateCache* fallbackComputePipelineStateCache = nullptr;
		ComputePipelineStateCompiler& computePipelineStateCompiler = mMaterialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getComputePipelineStateCompiler();
		if (computePipelineStateCompiler.isAsynchronousCompilationEnabled())
		{
			// Asynchronous
			if (!shaderProperties.getSortedPropertyVector().empty())
			{
				fallbackComputePipelineStateCache = getFallbackComputePipelineStateCache(shaderProperties);
			}
		}
		else
		{
			allowEmergencySynchronousCompilation = true;
		}

		// Create the new compute pipeline state cache instance
		ComputePipelineStateCache* computePipelineStateCache = new ComputePipelineStateCache(mTemporaryComputePipelineStateSignature);
		mComputePipelineStateCacheByComputePipelineStateSignatureId.emplace(mTemporaryComputePipelineStateSignature.getComputePipelineStateSignatureId(), computePipelineStateCache);
		mPipelineStateObjectCacheNeedSaving = true;

		// If we've got a fallback compute pipeline state cache then commit the asynchronous pipeline state compiler request now, else we must proceed synchronous (risk of notable runtime hiccups)
		if (nullptr != fallbackComputePipelineStateCache)
		{
			// Asynchronous, the light side
			computePipelineStateCache->mComputePipelineStateObjectPtr = fallbackComputePipelineStateCache->mComputePipelineStateObjectPtr;
			computePipelineStateCache->mIsUsingFallback = true;
			computePipelineStateCompiler.addAsynchronousCompilerRequest(*computePipelineStateCache);
		}
		else if (allowEmergencySynchronousCompilation)
		{
			// Synchronous, the dark side
			computePipelineStateCompiler.instantSynchronousCompilerRequest(mMaterialBlueprintResource, *computePipelineStateCache);
		}
		else
		{
			// Compute won't work as long as there's no compute pipeline state instance
			computePipelineStateCompiler.addAsynchronousCompilerRequest(*computePipelineStateCache);
		}

		// Done
		return computePipelineStateCache;
	}

	void ComputePipelineStateCacheManager::clearCache()
	{
		if (!mComputePipelineStateCacheByComputePipelineStateSignatureId.empty())
		{
			for (auto& computePipelineStateCacheElement : mComputePipelineStateCacheByComputePipelineStateSignatureId)
			{
				delete computePipelineStateCacheElement.second;
			}
			mComputePipelineStateCacheByComputePipelineStateSignatureId.clear();
			mPipelineStateObjectCacheNeedSaving = true;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	ComputePipelineStateCache* ComputePipelineStateCacheManager::getFallbackComputePipelineStateCache(const ShaderProperties& shaderProperties)
	{
		// Look for a suitable already available pipeline state cache which content we can use as fallback while the pipeline state compiler is working. We
		// do this by reducing the shader properties set until we find something, hopefully. In case no fallback can be found we have to switch to synchronous processing.

		// Start with the full shader properties and then clear one shader property after another
		mFallbackShaderProperties = shaderProperties;
		ShaderProperties::SortedPropertyVector& sortedFallbackPropertyVector = mFallbackShaderProperties.getSortedPropertyVector();
		while (!sortedFallbackPropertyVector.empty())
		{
			{ // Remove a fallback shader property
				// Find the most useless shader property, we're going to sacrifice it
				ShaderProperties::SortedPropertyVector::iterator worstHitShaderPropertyIterator = sortedFallbackPropertyVector.end();
				int32_t worstHitVisualImportanceOfShaderProperty = getInvalid<int32_t>();
				ShaderProperties::SortedPropertyVector::iterator iterator = sortedFallbackPropertyVector.begin();
				while (iterator != sortedFallbackPropertyVector.end())
				{
					// Do not remove mandatory shader combination shader properties, at least not inside this pass
					const int32_t visualImportanceOfShaderProperty = mMaterialBlueprintResource.getVisualImportanceOfShaderProperty(iterator->shaderPropertyId);
					if (MaterialBlueprintResource::MANDATORY_SHADER_PROPERTY != visualImportanceOfShaderProperty)
					{
						if (isValid(worstHitVisualImportanceOfShaderProperty))
						{
							// Lower visual importance value = lower probability that someone will miss the shader property
							if (worstHitVisualImportanceOfShaderProperty > visualImportanceOfShaderProperty)
							{
								worstHitVisualImportanceOfShaderProperty = visualImportanceOfShaderProperty;
								worstHitShaderPropertyIterator = iterator;
							}
						}
						else
						{
							worstHitShaderPropertyIterator = iterator;
							worstHitVisualImportanceOfShaderProperty = visualImportanceOfShaderProperty;
						}
					}

					// Next shader property on the to-kill-list, please
					++iterator;
				}

				// Sacrifice our victim
				if (sortedFallbackPropertyVector.end() == worstHitShaderPropertyIterator)
				{
					// No chance, no goats left
					break;
				}
				sortedFallbackPropertyVector.erase(worstHitShaderPropertyIterator);
			}

			// Generate the current fallback compute pipeline state signature
			mFallbackComputePipelineStateSignature.set(mMaterialBlueprintResource, mFallbackShaderProperties);
			ComputePipelineStateCacheByComputePipelineStateSignatureId::const_iterator iterator = mComputePipelineStateCacheByComputePipelineStateSignatureId.find(mFallbackComputePipelineStateSignature.getComputePipelineStateSignatureId());
			if (iterator != mComputePipelineStateCacheByComputePipelineStateSignatureId.cend())
			{
				// We don't care whether or not the compute pipeline state cache is currently using fallback data due to asynchronous complication
				return iterator->second;
			}
		}

		// No fallback compute pipeline state cache found
		return nullptr;
	}

	void ComputePipelineStateCacheManager::loadPipelineStateObjectCache(IFile& file)
	{
		// Material blueprint resource ID, all compute pipeline state cache share the same material blueprint resource ID
		MaterialBlueprintResourceId materialBlueprintResourceId = getInvalid<MaterialBlueprintResourceId>();
		file.read(&materialBlueprintResourceId, sizeof(uint32_t));
		assert(mMaterialBlueprintResource.getId() == materialBlueprintResourceId);

		// TODO(co) Currently only the compute pipeline state signature ID is loaded, not the resulting binary pipeline state cache
		uint32_t numberOfComputePipelineStateCaches = getInvalid<uint32_t>();
		file.read(&numberOfComputePipelineStateCaches, sizeof(uint32_t));
		mComputePipelineStateCacheByComputePipelineStateSignatureId.reserve(numberOfComputePipelineStateCaches);
		ShaderProperties shaderProperties;
		ShaderProperties::SortedPropertyVector& sortedPropertyVector = shaderProperties.getSortedPropertyVector();
		sortedPropertyVector.reserve(10);
		ComputePipelineStateCompiler& computePipelineStateCompiler = mMaterialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getComputePipelineStateCompiler();
		for (uint32_t computePipelineStateCacheIndex = 0; computePipelineStateCacheIndex < numberOfComputePipelineStateCaches; ++computePipelineStateCacheIndex)
		{
			// Read shader properties
			uint32_t numberOfShaderProperties = getInvalid<uint32_t>();
			file.read(&numberOfShaderProperties, sizeof(uint32_t));
			sortedPropertyVector.resize(numberOfShaderProperties);
			if (numberOfShaderProperties > 0)
			{
				file.read(sortedPropertyVector.data(), sizeof(ShaderProperties::Property) * numberOfShaderProperties);
			}

			// Register
			mTemporaryComputePipelineStateSignature.set(mMaterialBlueprintResource, shaderProperties);
			ComputePipelineStateCache* computePipelineStateCache = new ComputePipelineStateCache(mTemporaryComputePipelineStateSignature);
			mComputePipelineStateCacheByComputePipelineStateSignatureId.emplace(mTemporaryComputePipelineStateSignature.getComputePipelineStateSignatureId(), computePipelineStateCache);
			computePipelineStateCompiler.instantSynchronousCompilerRequest(mMaterialBlueprintResource, *computePipelineStateCache);
		}

		// Done
		mPipelineStateObjectCacheNeedSaving = false;
	}

	void ComputePipelineStateCacheManager::savePipelineStateObjectCache(IFile& file)
	{
		// Material blueprint resource ID, all compute pipeline state cache share the same material blueprint resource ID
		const MaterialBlueprintResourceId materialBlueprintResourceId = mMaterialBlueprintResource.getId();
		file.write(&materialBlueprintResourceId, sizeof(uint32_t));

		// TODO(co) Currently only the compute pipeline state signature ID is saved, not the resulting binary pipeline state cache
		const uint32_t numberOfComputePipelineStateCaches = static_cast<uint32_t>(mComputePipelineStateCacheByComputePipelineStateSignatureId.size());
		file.write(&numberOfComputePipelineStateCaches, sizeof(uint32_t));
		for (const auto& elementPair : mComputePipelineStateCacheByComputePipelineStateSignatureId)
		{
			const ComputePipelineStateSignature& computePipelineStateSignature = elementPair.second->getComputePipelineStateSignature();

			// Sanity check: All compute pipeline state cache share the same material blueprint resource ID
			assert(computePipelineStateSignature.getMaterialBlueprintResourceId() == materialBlueprintResourceId);

			{ // Write shader properties
				const ShaderProperties::SortedPropertyVector& sortedPropertyVector = computePipelineStateSignature.getShaderProperties().getSortedPropertyVector();
				const uint32_t numberOfShaderProperties = static_cast<uint32_t>(sortedPropertyVector.size());
				file.write(&numberOfShaderProperties, sizeof(uint32_t));
				if (numberOfShaderProperties > 0)
				{
					file.write(sortedPropertyVector.data(), sizeof(ShaderProperties::Property) * numberOfShaderProperties);
				}
			}
		}

		// Done
		mPipelineStateObjectCacheNeedSaving = false;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
