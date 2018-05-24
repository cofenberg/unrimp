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
#include "RendererRuntime/Resource/MaterialBlueprint/Cache/ProgramCacheManager.h"
#include "RendererRuntime/Resource/MaterialBlueprint/Cache/ProgramCache.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/ShaderCache.h"
#include "RendererRuntime/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Resource/VertexAttributes/VertexAttributesResourceManager.h"
#include "RendererRuntime/Resource/VertexAttributes/VertexAttributesResource.h"
#include "RendererRuntime/Core/Math/Math.h"
#include "RendererRuntime/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	ProgramCacheId ProgramCacheManager::generateProgramCacheId(const PipelineStateSignature& pipelineStateSignature)
	{
		ProgramCacheId programCacheId = Math::FNV1a_INITIAL_HASH_32;
		for (uint8_t i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
		{
			const ShaderCombinationId shaderCombinationId = pipelineStateSignature.getShaderCombinationId(static_cast<ShaderType>(i));
			if (isInitialized(shaderCombinationId))
			{
				programCacheId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&shaderCombinationId), sizeof(ShaderCombinationId), programCacheId);
			}
		}

		// Done
		return programCacheId;
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	ProgramCache* ProgramCacheManager::getProgramCacheByPipelineStateSignature(const PipelineStateSignature& pipelineStateSignature)
	{
		// Does the program cache already exist?
		ProgramCache* programCache = nullptr;
		const ProgramCacheId programCacheId = generateProgramCacheId(pipelineStateSignature);
		std::unique_lock<std::mutex> mutexLock(mMutex);
		ProgramCacheById::const_iterator iterator = mProgramCacheById.find(programCacheId);
		if (iterator != mProgramCacheById.cend())
		{
			programCache = iterator->second;
		}
		else
		{
			// Create the renderer program: Decide which shader language should be used (for example "GLSL" or "HLSL")
			const MaterialBlueprintResource& materialBlueprintResource = mPipelineStateCacheManager.getMaterialBlueprintResource();
			const Renderer::IRootSignaturePtr rootSignaturePtr = materialBlueprintResource.getRootSignaturePtr();
			Renderer::IShaderLanguagePtr shaderLanguage(rootSignaturePtr->getRenderer().getShaderLanguage());
			if (nullptr != shaderLanguage)
			{
				const IRendererRuntime& rendererRuntime = materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime();

				// Create the shaders
				ShaderCacheManager& shaderCacheManager = rendererRuntime.getShaderBlueprintResourceManager().getShaderCacheManager();
				Renderer::IShader* shaders[NUMBER_OF_SHADER_TYPES] = {};
				for (uint8_t i = 0; i < NUMBER_OF_SHADER_TYPES; ++i)
				{
					ShaderCache* shaderCache = shaderCacheManager.getShaderCache(pipelineStateSignature, materialBlueprintResource, *shaderLanguage, static_cast<ShaderType>(i));
					if (nullptr != shaderCache)
					{
						shaders[i] = shaderCache->getShaderPtr();
					}
					else
					{
						// No error, just means there's no shader cache because e.g. there's no shader of the requested type
					}
				}

				// Create the program
				Renderer::IProgram* program = shaderLanguage->createProgram(*rootSignaturePtr,
					rendererRuntime.getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes(),
					static_cast<Renderer::IVertexShader*>(shaders[static_cast<int>(ShaderType::Vertex)]),
					static_cast<Renderer::ITessellationControlShader*>(shaders[static_cast<int>(ShaderType::TessellationControl)]),
					static_cast<Renderer::ITessellationEvaluationShader*>(shaders[static_cast<int>(ShaderType::TessellationEvaluation)]),
					static_cast<Renderer::IGeometryShader*>(shaders[static_cast<int>(ShaderType::Geometry)]),
					static_cast<Renderer::IFragmentShader*>(shaders[static_cast<int>(ShaderType::Fragment)]));
				RENDERER_SET_RESOURCE_DEBUG_NAME(program, "Program cache manager")

				// Create the new program cache instance
				if (nullptr != program)
				{
					programCache = new ProgramCache(programCacheId, *program);
					mProgramCacheById.emplace(programCacheId, programCache);
				}
				else
				{
					// TODO(co) Error handling
					assert(false);
				}
			}
		}

		// Done
		return programCache;
	}

	void ProgramCacheManager::clearCache()
	{
		std::unique_lock<std::mutex> mutexLock(mMutex);
		for (auto& programCacheElement : mProgramCacheById)
		{
			delete programCacheElement.second;
		}
		mProgramCacheById.clear();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
