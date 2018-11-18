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
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCacheManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCompiler.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCache.h"
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
	Renderer::IGraphicsPipelineStatePtr GraphicsPipelineStateCacheManager::getGraphicsPipelineStateCacheByCombination(uint32_t serializedGraphicsPipelineStateHash, const ShaderProperties& shaderProperties, bool allowEmergencySynchronousCompilation)
	{
		// TODO(co) Asserts whether or not e.g. the material resource is using the owning material resource blueprint
		assert(IResource::LoadingState::LOADED == mMaterialBlueprintResource.getLoadingState());

		// Generate the graphics pipeline state signature
		mTemporaryGraphicsPipelineStateSignature.set(mMaterialBlueprintResource, serializedGraphicsPipelineStateHash, shaderProperties);
		{
			GraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId::const_iterator iterator = mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.find(mTemporaryGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId());
			if (iterator != mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.cend())
			{
				// There's already a pipeline state cache for the pipeline state signature ID
				// -> We don't care whether or not the pipeline state cache is currently using fallback data due to asynchronous complication
				return iterator->second->getGraphicsPipelineStateObjectPtr();
			}
		}

		// Fallback: OK, the pipeline state signature is unknown and we now have to perform more complex and time consuming work. So first, check whether or not this work
		// should be performed asynchronous (usually the case). If asynchronous, we need to return a fallback pipeline state cache while the pipeline state compiler is working.
		GraphicsPipelineStateCache* fallbackGraphicsPipelineStateCache = nullptr;
		GraphicsPipelineStateCompiler& graphicsPipelineStateCompiler = mMaterialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getGraphicsPipelineStateCompiler();
		if (graphicsPipelineStateCompiler.isAsynchronousCompilationEnabled())
		{
			// Asynchronous
			if (!shaderProperties.getSortedPropertyVector().empty())
			{
				fallbackGraphicsPipelineStateCache = getFallbackGraphicsPipelineStateCache(serializedGraphicsPipelineStateHash, shaderProperties);
			}

			// If we're here and still not having any fallback graphics pipeline state cache we'll end up with a runtime hiccup, we don't want that
			// -> Kids, don't try this at home: We'll trade the runtime hiccup against a nasty major graphics artifact. If we're in luck no one
			//    will notice it, depends on the situation. A runtime hiccup on the other hand will always be notable. So this trade in here
			//    might not involve our first born.
			if (!allowEmergencySynchronousCompilation && nullptr == fallbackGraphicsPipelineStateCache && !mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.empty())
			{
				fallbackGraphicsPipelineStateCache = getFallbackGraphicsPipelineStateCache(getInvalid<uint32_t>(), shaderProperties);
			}
		}
		else
		{
			allowEmergencySynchronousCompilation = true;
		}

		// Create the new graphics pipeline state cache instance
		GraphicsPipelineStateCache* graphicsPipelineStateCache = new GraphicsPipelineStateCache(mTemporaryGraphicsPipelineStateSignature);
		mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.emplace(mTemporaryGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId(), graphicsPipelineStateCache);
		mPipelineStateObjectCacheNeedSaving = true;

		// If we've got a fallback graphics pipeline state cache then commit the asynchronous pipeline state compiler request now, else we must proceed synchronous (risk of notable runtime hiccups)
		if (nullptr != fallbackGraphicsPipelineStateCache)
		{
			// Asynchronous, the light side
			graphicsPipelineStateCache->mGraphicsPipelineStateObjectPtr = fallbackGraphicsPipelineStateCache->mGraphicsPipelineStateObjectPtr;
			graphicsPipelineStateCache->mIsUsingFallback = true;
			graphicsPipelineStateCompiler.addAsynchronousCompilerRequest(*graphicsPipelineStateCache);
		}
		else if (allowEmergencySynchronousCompilation)
		{
			// Synchronous, the dark side
			graphicsPipelineStateCompiler.instantSynchronousCompilerRequest(mMaterialBlueprintResource, *graphicsPipelineStateCache);
		}
		else
		{
			// Graphics won't work as long as there's no compute pipeline state instance
			graphicsPipelineStateCompiler.addAsynchronousCompilerRequest(*graphicsPipelineStateCache);
		}

		// Done
		return graphicsPipelineStateCache->getGraphicsPipelineStateObjectPtr();
	}

	void GraphicsPipelineStateCacheManager::clearCache()
	{
		if (!mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.empty())
		{
			for (auto& graphicsPipelineStateCacheElement : mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId)
			{
				delete graphicsPipelineStateCacheElement.second;
			}
			mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.clear();
			mPipelineStateObjectCacheNeedSaving = true;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	GraphicsPipelineStateCache* GraphicsPipelineStateCacheManager::getFallbackGraphicsPipelineStateCache(uint32_t serializedGraphicsPipelineStateHash, const ShaderProperties& shaderProperties)
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

			// Generate the current fallback graphics pipeline state signature
			mFallbackGraphicsPipelineStateSignature.set(mMaterialBlueprintResource, serializedGraphicsPipelineStateHash, mFallbackShaderProperties);
			GraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId::const_iterator iterator = mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.find(mFallbackGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId());
			if (iterator != mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.cend())
			{
				// We don't care whether or not the graphics pipeline state cache is currently using fallback data due to asynchronous complication
				return iterator->second;
			}
		}

		// No fallback graphics pipeline state cache found
		return nullptr;
	}

	void GraphicsPipelineStateCacheManager::loadPipelineStateObjectCache(IFile& file)
	{
		// Material blueprint resource ID, all graphics pipeline state cache share the same material blueprint resource ID
		MaterialBlueprintResourceId materialBlueprintResourceId = getInvalid<MaterialBlueprintResourceId>();
		file.read(&materialBlueprintResourceId, sizeof(uint32_t));
		assert(mMaterialBlueprintResource.getId() == materialBlueprintResourceId);

		// TODO(co) Currently only the graphics pipeline state signature ID is loaded, not the resulting binary pipeline state cache
		uint32_t numberOfGraphicsPipelineStateCaches = getInvalid<uint32_t>();
		file.read(&numberOfGraphicsPipelineStateCaches, sizeof(uint32_t));
		mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.reserve(numberOfGraphicsPipelineStateCaches);
		ShaderProperties shaderProperties;
		ShaderProperties::SortedPropertyVector& sortedPropertyVector = shaderProperties.getSortedPropertyVector();
		sortedPropertyVector.reserve(10);
		GraphicsPipelineStateCompiler& graphicsPipelineStateCompiler = mMaterialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getGraphicsPipelineStateCompiler();
		for (uint32_t graphicsPipelineStateCacheIndex = 0; graphicsPipelineStateCacheIndex < numberOfGraphicsPipelineStateCaches; ++graphicsPipelineStateCacheIndex)
		{
			// Read serialized graphics pipeline state hash
			uint32_t serializedGraphicsPipelineStateHash = getInvalid<uint32_t>();
			file.read(&serializedGraphicsPipelineStateHash, sizeof(uint32_t));

			// Read shader properties
			uint32_t numberOfShaderProperties = getInvalid<uint32_t>();
			file.read(&numberOfShaderProperties, sizeof(uint32_t));
			sortedPropertyVector.resize(numberOfShaderProperties);
			if (numberOfShaderProperties > 0)
			{
				file.read(sortedPropertyVector.data(), sizeof(ShaderProperties::Property) * numberOfShaderProperties);
			}

			// Register
			mTemporaryGraphicsPipelineStateSignature.set(mMaterialBlueprintResource, serializedGraphicsPipelineStateHash, shaderProperties);
			GraphicsPipelineStateCache* graphicsPipelineStateCache = new GraphicsPipelineStateCache(mTemporaryGraphicsPipelineStateSignature);
			mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.emplace(mTemporaryGraphicsPipelineStateSignature.getGraphicsPipelineStateSignatureId(), graphicsPipelineStateCache);
			graphicsPipelineStateCompiler.instantSynchronousCompilerRequest(mMaterialBlueprintResource, *graphicsPipelineStateCache);
		}

		// Done
		mPipelineStateObjectCacheNeedSaving = false;
	}

	void GraphicsPipelineStateCacheManager::savePipelineStateObjectCache(IFile& file)
	{
		// Material blueprint resource ID, all graphics pipeline state cache share the same material blueprint resource ID
		const MaterialBlueprintResourceId materialBlueprintResourceId = mMaterialBlueprintResource.getId();
		file.write(&materialBlueprintResourceId, sizeof(uint32_t));

		// TODO(co) Currently only the graphics pipeline state signature ID is saved, not the resulting binary pipeline state cache
		const uint32_t numberOfGraphicsPipelineStateCaches = static_cast<uint32_t>(mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId.size());
		file.write(&numberOfGraphicsPipelineStateCaches, sizeof(uint32_t));
		for (const auto& elementPair : mGraphicsPipelineStateCacheByGraphicsPipelineStateSignatureId)
		{
			const GraphicsPipelineStateSignature& graphicsPipelineStateSignature = elementPair.second->getGraphicsPipelineStateSignature();

			// Sanity check: All graphics pipeline state cache share the same material blueprint resource ID
			assert(graphicsPipelineStateSignature.getMaterialBlueprintResourceId() == materialBlueprintResourceId);

			{ // Write serialized graphics pipeline state hash
				const uint32_t serializedGraphicsPipelineStateHash = graphicsPipelineStateSignature.getSerializedGraphicsPipelineStateHash();
				file.write(&serializedGraphicsPipelineStateHash, sizeof(uint32_t));
			}

			{ // Write shader properties
				const ShaderProperties::SortedPropertyVector& sortedPropertyVector = graphicsPipelineStateSignature.getShaderProperties().getSortedPropertyVector();
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
