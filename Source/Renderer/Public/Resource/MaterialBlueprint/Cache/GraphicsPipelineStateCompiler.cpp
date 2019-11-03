/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCompiler.h"
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCache.h"
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/GraphicsProgramCache.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderBuilder.h"
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderCache.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "Renderer/Public/Resource/ShaderBlueprint/ShaderBlueprintResource.h"
#include "Renderer/Public/Resource/VertexAttributes/VertexAttributesResourceManager.h"
#include "Renderer/Public/Resource/VertexAttributes/VertexAttributesResource.h"
#include "Renderer/Public/Core/Platform/PlatformManager.h"
#include "Renderer/Public/IRenderer.h"
#include "Renderer/Public/Core/Math/Math.h"


// Disable warnings
// TODO(co) See "Renderer::GraphicsPipelineStateCompiler::GraphicsPipelineStateCompiler()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void GraphicsPipelineStateCompiler::setAsynchronousCompilationEnabled(bool enabled)
	{
		// State change?
		if (mAsynchronousCompilationEnabled != enabled)
		{
			mAsynchronousCompilationEnabled = enabled;

			// Ensure the internal queues are flushes so that we can guarantee that everything is synchronous available
			if (!mAsynchronousCompilationEnabled)
			{
				flushAllQueues();
			}
		}
	}

	void GraphicsPipelineStateCompiler::setNumberOfCompilerThreads(uint32_t numberOfCompilerThreads)
	{
		if (mNumberOfCompilerThreads != numberOfCompilerThreads)
		{
			// Compiler threads shutdown
			mShutdownCompilerThread = true;
			mCompilerConditionVariable.notify_all();
			for (std::thread& thread : mCompilerThreads)
			{
				thread.join();
			}

			// Create the compiler threads crunching the shaders into bytecode
			mNumberOfCompilerThreads = numberOfCompilerThreads;
			mCompilerThreads.clear();
			mCompilerThreads.reserve(mNumberOfCompilerThreads);
			mShutdownCompilerThread = false;
			for (uint32_t i = 0; i < mNumberOfCompilerThreads; ++i)
			{
				mCompilerThreads.push_back(std::thread(&GraphicsPipelineStateCompiler::compilerThreadWorker, this));
			}
		}
	}

	void GraphicsPipelineStateCompiler::dispatch()
	{
		// Synchronous dispatch
		// TODO(co) Add maximum dispatch time budget
		// TODO(co) More clever mutex usage in order to reduce pipeline state compiler stalls due to synchronization
		std::lock_guard<std::mutex> dispatchMutexLock(mDispatchMutex);
		while (!mDispatchQueue.empty())
		{
			// Get the compiler request
			CompilerRequest compilerRequest(mDispatchQueue.back());
			mDispatchQueue.pop_back();

			// Tell the graphics pipeline state cache about the real compiled graphics pipeline state object
			GraphicsPipelineStateCache& graphicsPipelineStateCache = compilerRequest.graphicsPipelineStateCache;
			graphicsPipelineStateCache.mGraphicsPipelineStateObjectPtr = compilerRequest.graphicsPipelineStateObject;
			graphicsPipelineStateCache.mIsUsingFallback = false;
			RHI_ASSERT(mRenderer.getContext(), 0 != mNumberOfInFlightCompilerRequests, "Invalid number of in flight compiler requests")
			--mNumberOfInFlightCompilerRequests;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	GraphicsPipelineStateCompiler::GraphicsPipelineStateCompiler(IRenderer& renderer) :
		mRenderer(renderer),
		mAsynchronousCompilationEnabled(renderer.getRhi().getCapabilities().nativeMultithreading),
		mNumberOfCompilerThreads(0),
		mNumberOfInFlightCompilerRequests(0),
		mShutdownBuilderThread(false),
		mBuilderThread(&GraphicsPipelineStateCompiler::builderThreadWorker, this),
		mShutdownCompilerThread(false)
	{
		// Create and start the threads
		setNumberOfCompilerThreads(2);
	}

	GraphicsPipelineStateCompiler::~GraphicsPipelineStateCompiler()
	{
		// Builder thread shutdown
		mShutdownBuilderThread = true;
		mBuilderConditionVariable.notify_one();
		mBuilderThread.join();

		// Compiler threads shutdown
		setNumberOfCompilerThreads(0);
	}

	void GraphicsPipelineStateCompiler::addAsynchronousCompilerRequest(GraphicsPipelineStateCache& graphicsPipelineStateCache)
	{
		// Push the load request into the builder queue
		RHI_ASSERT(mRenderer.getContext(), mAsynchronousCompilationEnabled, "Asynchronous compilation isn't enabled")
		++mNumberOfInFlightCompilerRequests;
		std::unique_lock<std::mutex> builderMutexLock(mBuilderMutex);
		mBuilderQueue.emplace_back(CompilerRequest(graphicsPipelineStateCache));
		builderMutexLock.unlock();
		mBuilderConditionVariable.notify_one();
	}

	void GraphicsPipelineStateCompiler::instantSynchronousCompilerRequest(MaterialBlueprintResource& materialBlueprintResource, GraphicsPipelineStateCache& graphicsPipelineStateCache)
	{
		// Get the graphics program cache; synchronous processing
		const GraphicsPipelineStateSignature& graphicsPipelineStateSignature = graphicsPipelineStateCache.getGraphicsPipelineStateSignature();
		const GraphicsProgramCache* graphicsProgramCache = materialBlueprintResource.getGraphicsPipelineStateCacheManager().getGraphicsProgramCacheManager().getGraphicsProgramCacheByGraphicsPipelineStateSignature(graphicsPipelineStateSignature);
		if (nullptr != graphicsProgramCache)
		{
			const Rhi::IGraphicsProgramPtr& graphicsProgramPtr = graphicsProgramCache->getGraphicsProgramPtr();
			if (nullptr != graphicsProgramPtr)
			{
				graphicsPipelineStateCache.mGraphicsPipelineStateObjectPtr = createGraphicsPipelineState(materialBlueprintResource, graphicsPipelineStateSignature.getSerializedGraphicsPipelineStateHash(), *graphicsProgramPtr);
			}
		}
	}

	void GraphicsPipelineStateCompiler::flushQueue(std::mutex& mutex, const CompilerRequests& compilerRequests)
	{
		bool everythingFlushed = false;
		do
		{
			{ // Process
				std::lock_guard<std::mutex> compilerMutexLock(mutex);
				everythingFlushed = compilerRequests.empty();
			}
			dispatch();

			// Wait for a moment to not totally pollute the CPU
			if (!everythingFlushed)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1ms);
			}
		} while (!everythingFlushed);
	}

	void GraphicsPipelineStateCompiler::builderThreadWorker()
	{
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRenderer.getMaterialBlueprintResourceManager();
		ShaderBlueprintResourceManager& shaderBlueprintResourceManager = mRenderer.getShaderBlueprintResourceManager();
		ShaderCacheManager& shaderCacheManager = shaderBlueprintResourceManager.getShaderCacheManager();
		const ShaderPieceResourceManager& shaderPieceResourceManager = mRenderer.getShaderPieceResourceManager();
		ShaderBuilder shaderBuilder(mRenderer.getRhi().getContext());

		RENDERER_SET_CURRENT_THREAD_DEBUG_NAME("PSC: Stage 1", "Renderer: Pipeline state compiler stage: 1. Asynchronous shader building");
		while (!mShutdownBuilderThread)
		{
			// Continue as long as there's a compiler request left inside the queue, if it's empty go to sleep
			std::unique_lock<std::mutex> builderMutexLock(mBuilderMutex);
			mBuilderConditionVariable.wait(builderMutexLock);
			while (!mBuilderQueue.empty() && !mShutdownBuilderThread)
			{
				// Get the compiler request
				CompilerRequest compilerRequest(mBuilderQueue.back());
				mBuilderQueue.pop_back();
				builderMutexLock.unlock();
				bool pushToCompilerQueue = true;
				bool needToWaitForGraphicsProgramCache = false;

				{ // Do the work: Building the shader source code for the required combination
					const GraphicsPipelineStateSignature& graphicsPipelineStateSignature = compilerRequest.graphicsPipelineStateCache.getGraphicsPipelineStateSignature();
					MaterialBlueprintResource& materialBlueprintResource = materialBlueprintResourceManager.getById(graphicsPipelineStateSignature.getMaterialBlueprintResourceId());

					// First at all, check whether or not the graphics program cache entry we need already exists, if so we can take a shortcut and only have to care about creating the graphics pipeline state
					GraphicsProgramCacheManager& graphicsProgramCacheManager = materialBlueprintResource.getGraphicsPipelineStateCacheManager().getGraphicsProgramCacheManager();
					GraphicsProgramCacheId graphicsProgramCacheId = compilerRequest.graphicsProgramCacheId;
					if (isInvalid(graphicsProgramCacheId))
					{
						graphicsProgramCacheId = compilerRequest.graphicsProgramCacheId = GraphicsProgramCacheManager::generateGraphicsProgramCacheId(graphicsPipelineStateSignature);
					}
					{ // In flight graphics program caches handling
						std::unique_lock<std::mutex> inFlightGraphicsProgramCachesMutexLock(mInFlightGraphicsProgramCachesMutex);
						if (mInFlightGraphicsProgramCaches.find(graphicsProgramCacheId) != mInFlightGraphicsProgramCaches.end())
						{
							needToWaitForGraphicsProgramCache = true;
						}
					}
					if (!needToWaitForGraphicsProgramCache)
					{
						std::unique_lock<std::mutex> graphicsProgramCacheManagerMutexLock(graphicsProgramCacheManager.mMutex);
						const GraphicsProgramCacheManager::GraphicsProgramCacheById::const_iterator iterator = graphicsProgramCacheManager.mGraphicsProgramCacheById.find(graphicsProgramCacheId);
						if (graphicsProgramCacheManager.mGraphicsProgramCacheById.cend() != iterator)
						{
							// Shortcut since the graphics program cache entry already exists: Just create the graphics pipeline state and be done with it

							// Create the graphics pipeline state object (PSO)
							compilerRequest.graphicsPipelineStateObject = createGraphicsPipelineState(materialBlueprintResource, graphicsPipelineStateSignature.getSerializedGraphicsPipelineStateHash(), *iterator->second->getGraphicsProgramPtr());
							pushToCompilerQueue = false;
						}
						else
						{
							// Build the shader source code for the required combination
							graphicsProgramCacheManagerMutexLock.unlock();
							{ // Graphics program cache is now in flight
								std::unique_lock<std::mutex> inFlightGraphicsProgramCachesMutexLock(mInFlightGraphicsProgramCachesMutex);
								mInFlightGraphicsProgramCaches.insert(graphicsProgramCacheId);
							}
							for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
							{
								// Get the shader blueprint resource ID
								const GraphicsShaderType graphicsShaderType = static_cast<GraphicsShaderType>(i);
								const ShaderBlueprintResourceId shaderBlueprintResourceId = materialBlueprintResource.getGraphicsShaderBlueprintResourceId(graphicsShaderType);
								if (isValid(shaderBlueprintResourceId))
								{
									// Get the shader cache identifier, often but not always identical to the shader combination ID
									const ShaderCacheId shaderCacheId = graphicsPipelineStateSignature.getShaderCombinationId(graphicsShaderType);

									// Does the shader cache already exist?
									ShaderCache* shaderCache = nullptr;
									std::unique_lock<std::mutex> shaderCacheManagerMutexLock(shaderCacheManager.mMutex);
									ShaderCacheManager::ShaderCacheByShaderCacheId::const_iterator shaderCacheIdIterator = shaderCacheManager.mShaderCacheByShaderCacheId.find(shaderCacheId);
									if (shaderCacheIdIterator != shaderCacheManager.mShaderCacheByShaderCacheId.cend())
									{
										shaderCache = shaderCacheIdIterator->second;
									}
									else
									{
										// Try to create the new graphics shader cache instance
										const ShaderBlueprintResource* shaderBlueprintResource = shaderBlueprintResourceManager.tryGetById(shaderBlueprintResourceId);
										if (nullptr != shaderBlueprintResource)
										{
											// Build the shader source code
											ShaderBuilder::BuildShader buildShader;
											shaderBuilder.createSourceCode(shaderPieceResourceManager, *shaderBlueprintResource, graphicsPipelineStateSignature.getShaderProperties(), buildShader);
											std::string& sourceCode = buildShader.sourceCode;
											if (sourceCode.empty())
											{
												// TODO(co) Error handling
												RHI_ASSERT(mRenderer.getContext(), false, "Invalid source code")
											}
											else
											{
												// Add the virtual filename of the shader blueprint asset as first shader source code line to make shader debugging easier
												sourceCode = std::string("// ") + mRenderer.getAssetManager().getAssetByAssetId(shaderBlueprintResource->getAssetId()).virtualFilename + '\n' + sourceCode;

												// Generate the shader source code ID
												// -> Especially in complex shaders, there are situations where different shader combinations result in one and the same shader source code
												// -> Shader compilation is considered to be expensive, so we need to be pretty sure that we really need to perform this heavy work
												const ShaderSourceCodeId shaderSourceCodeId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(sourceCode.c_str()), static_cast<uint32_t>(sourceCode.size()));
												ShaderCacheManager::ShaderCacheByShaderSourceCodeId::const_iterator shaderSourceCodeIdIterator = shaderCacheManager.mShaderCacheByShaderSourceCodeId.find(shaderSourceCodeId);
												if (shaderSourceCodeIdIterator != shaderCacheManager.mShaderCacheByShaderSourceCodeId.cend())
												{
													// Reuse already existing shader instance
													// -> We still have to create a shader cache instance so we don't need to build the shader source code again next time
													shaderCache = new ShaderCache(shaderCacheId, shaderCacheManager.mShaderCacheByShaderCacheId.find(shaderSourceCodeIdIterator->second)->second);
													shaderCacheManager.mShaderCacheByShaderCacheId.emplace(shaderCacheId, shaderCache);
												}
												else
												{
													// Create the new shader cache instance
													shaderCache = new ShaderCache(shaderCacheId);
													shaderCache->mAssetIds = buildShader.assetIds;
													shaderCache->mCombinedAssetFileHashes = buildShader.combinedAssetFileHashes;
													shaderCacheManager.mShaderCacheByShaderCacheId.emplace(shaderCacheId, shaderCache);
													shaderCacheManager.mShaderCacheByShaderSourceCodeId.emplace(shaderSourceCodeId, shaderCacheId);
													compilerRequest.shaderSourceCode[i] = sourceCode;
												}
											}
										}
										else
										{
											// TODO(co) Error handling
											RHI_ASSERT(mRenderer.getContext(), false, "Invalid shader blueprint resource")
										}
									}
									compilerRequest.shaderCache[i] = shaderCache;
								}
							}
						}
					}
				}

				// Push the compiler request into the correct queue
				if (needToWaitForGraphicsProgramCache)
				{
					// Throw the fish back into the see
					builderMutexLock.lock();
					mBuilderQueue.emplace_front(compilerRequest);
				}
				else
				{
					if (pushToCompilerQueue)
					{
						// Push the compiler request into the queue of the asynchronous shader compilation
						std::unique_lock<std::mutex> compilerMutexLock(mCompilerMutex);
						mCompilerQueue.emplace_back(compilerRequest);
						compilerMutexLock.unlock();
						mCompilerConditionVariable.notify_one();
					}
					else
					{
						// Shortcut: Push the compiler request into the queue of the synchronous shader dispatch
						std::lock_guard<std::mutex> dispatchMutexLock(mDispatchMutex);
						mDispatchQueue.emplace_back(compilerRequest);
					}

					// We're ready for the next round
					builderMutexLock.lock();
				}
			}
		}
	}

	void GraphicsPipelineStateCompiler::compilerThreadWorker()
	{
		Rhi::IShaderLanguage& shaderLanguage = mRenderer.getRhi().getDefaultShaderLanguage();
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRenderer.getMaterialBlueprintResourceManager();
		RENDERER_SET_CURRENT_THREAD_DEBUG_NAME("PSC: Stage 2", "Renderer: Pipeline state compiler stage: 2. Asynchronous shader compilation");
		while (!mShutdownCompilerThread)
		{
			// Continue as long as there's a compiler request left inside the queue, if it's empty go to sleep
			std::unique_lock<std::mutex> compilerMutexLock(mCompilerMutex);
			mCompilerConditionVariable.wait(compilerMutexLock);
			while (!mCompilerQueue.empty() && !mShutdownCompilerThread)
			{
				// Get the compiler request
				CompilerRequest compilerRequest(mCompilerQueue.back());
				mCompilerQueue.pop_back();
				compilerMutexLock.unlock();

				// Do the work: Compiling the shader source code it in order to get the shader bytecode
				bool needToWaitForShaderCache = false;
				Rhi::IShader* shaders[NUMBER_OF_GRAPHICS_SHADER_TYPES] = {};
				for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES && !needToWaitForShaderCache; ++i)
				{
					ShaderCache* shaderCache = compilerRequest.shaderCache[i];
					if (nullptr != shaderCache)
					{
						shaders[i] = shaderCache->getShaderPtr();
						if (nullptr == shaders[i])
						{
							// The shader instance is not ready, do we need to compile it right now or is this the job of a shader cache master?
							const std::string& shaderSourceCode = compilerRequest.shaderSourceCode[i];
							if (shaderSourceCode.empty())
							{
								// We're not aware of any shader source code but we need a shader cache, so, there must be a shader cache master we need to wait for
								// RHI_ASSERT(mRenderer.getContext(), nullptr != shaderCache->getMasterShaderCache(), "Invalid master shader cache")	// No assert by intent
								needToWaitForShaderCache = true;
							}
							else
							{
								// Create the shader instance
								Rhi::IShader* shader = nullptr;
								switch (static_cast<GraphicsShaderType>(i))
								{
									case GraphicsShaderType::Vertex:
									{
										const MaterialBlueprintResource& materialBlueprintResource = materialBlueprintResourceManager.getById(compilerRequest.graphicsPipelineStateCache.getGraphicsPipelineStateSignature().getMaterialBlueprintResourceId());
										const Rhi::VertexAttributes& vertexAttributes = mRenderer.getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes();
										shader = shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, shaderSourceCode.c_str(), &shaderCache->mShaderBytecode);
										break;
									}

									case GraphicsShaderType::TessellationControl:
										shader = shaderLanguage.createTessellationControlShaderFromSourceCode(shaderSourceCode.c_str(), &shaderCache->mShaderBytecode);
										break;

									case GraphicsShaderType::TessellationEvaluation:
										shader = shaderLanguage.createTessellationEvaluationShaderFromSourceCode(shaderSourceCode.c_str(), &shaderCache->mShaderBytecode);
										break;

									case GraphicsShaderType::Geometry:
										// TODO(co) "Renderer::ShaderCacheManager::getGraphicsShaderCache()" needs to provide additional geometry shader information
										// shader = shaderLanguage->createGeometryShaderFromSourceCode(shaderSourceCode.c_str(), &shaderCache->mShaderBytecode);
										break;

									case GraphicsShaderType::Fragment:
										shader = shaderLanguage.createFragmentShaderFromSourceCode(shaderSourceCode.c_str(), &shaderCache->mShaderBytecode);
										break;
								}
								RHI_ASSERT(mRenderer.getContext(), nullptr != shader, "Invalid shader")	// TODO(co) Error handling
								RHI_SET_RESOURCE_DEBUG_NAME(shader, "Pipeline state compiler")
								shaderCache->mShaderPtr = shaders[i] = shader;
							}
						}
					}
				}

				// Are all required shader caches ready for rumble?
				if (!needToWaitForShaderCache)
				{
					{ // Create the graphics pipeline state object (PSO)
						const GraphicsPipelineStateSignature& graphicsPipelineStateSignature = compilerRequest.graphicsPipelineStateCache.getGraphicsPipelineStateSignature();
						MaterialBlueprintResource& materialBlueprintResource = materialBlueprintResourceManager.getById(graphicsPipelineStateSignature.getMaterialBlueprintResourceId());

						// Create the graphics program
						Rhi::IGraphicsProgram* graphicsProgram = shaderLanguage.createGraphicsProgram(*materialBlueprintResource.getRootSignaturePtr(),
							mRenderer.getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes(),
							static_cast<Rhi::IVertexShader*>(shaders[static_cast<int>(GraphicsShaderType::Vertex)]),
							static_cast<Rhi::ITessellationControlShader*>(shaders[static_cast<int>(GraphicsShaderType::TessellationControl)]),
							static_cast<Rhi::ITessellationEvaluationShader*>(shaders[static_cast<int>(GraphicsShaderType::TessellationEvaluation)]),
							static_cast<Rhi::IGeometryShader*>(shaders[static_cast<int>(GraphicsShaderType::Geometry)]),
							static_cast<Rhi::IFragmentShader*>(shaders[static_cast<int>(GraphicsShaderType::Fragment)]));
						RHI_SET_RESOURCE_DEBUG_NAME(graphicsProgram, "Graphics pipeline state compiler")

						// Create the graphics pipeline state object (PSO)
						compilerRequest.graphicsPipelineStateObject = createGraphicsPipelineState(materialBlueprintResource, graphicsPipelineStateSignature.getSerializedGraphicsPipelineStateHash(), *graphicsProgram);

						{ // Graphics program cache entry
							GraphicsProgramCacheManager& graphicsProgramCacheManager = materialBlueprintResource.getGraphicsPipelineStateCacheManager().getGraphicsProgramCacheManager();
							const GraphicsProgramCacheId graphicsProgramCacheId = compilerRequest.graphicsProgramCacheId;
							RHI_ASSERT(mRenderer.getContext(), isValid(graphicsProgramCacheId), "Invalid graphics program cache ID")
							std::unique_lock<std::mutex> mutexLock(graphicsProgramCacheManager.mMutex);
							RHI_ASSERT(mRenderer.getContext(), graphicsProgramCacheManager.mGraphicsProgramCacheById.find(graphicsProgramCacheId) == graphicsProgramCacheManager.mGraphicsProgramCacheById.cend(), "Invalid graphics program cache ID")	// TODO(co) Error handling
							graphicsProgramCacheManager.mGraphicsProgramCacheById.emplace(graphicsProgramCacheId, new GraphicsProgramCache(graphicsProgramCacheId, *graphicsProgram));

							{ // The graphics program cache is no longer in flight
								std::unique_lock<std::mutex> inFlightGraphicsProgramCachesMutexLock(mInFlightGraphicsProgramCachesMutex);
								const InFlightGraphicsProgramCaches::const_iterator iterator = mInFlightGraphicsProgramCaches.find(graphicsProgramCacheId);
								RHI_ASSERT(mRenderer.getContext(), mInFlightGraphicsProgramCaches.end() != iterator, "Invalid graphics program cache ID")
								mInFlightGraphicsProgramCaches.erase(iterator);
							}
							mBuilderConditionVariable.notify_one();
						}
					}

					// Push the compiler request into the queue of the synchronous shader dispatch
					std::lock_guard<std::mutex> dispatchMutexLock(mDispatchMutex);
					mDispatchQueue.emplace_back(compilerRequest);
				}

				// We're ready for the next round
				compilerMutexLock.lock();
				if (needToWaitForShaderCache)
				{
					// At least one shader cache instance we need is referencing a master shader cache which hasn't finished processing yet, so we need to wait a while before we can continue with our request
					mCompilerQueue.emplace_front(compilerRequest);
				}
			}
		}
	}

	Rhi::IGraphicsPipelineState* GraphicsPipelineStateCompiler::createGraphicsPipelineState(const MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedGraphicsPipelineStateHash, Rhi::IGraphicsProgram& graphicsProgram) const
	{
		// Start with the graphics pipeline state of the material blueprint resource, then copy over serialized graphics pipeline state
		Rhi::GraphicsPipelineState graphicsPipelineState = materialBlueprintResource.getGraphicsPipelineState();
		materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().applySerializedGraphicsPipelineState(serializedGraphicsPipelineStateHash, graphicsPipelineState);

		// Setup the dynamic part of the pipeline state
		const IRenderer& renderer = materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRenderer();
		const Rhi::IRootSignaturePtr& rootSignaturePtr = materialBlueprintResource.getRootSignaturePtr();
		graphicsPipelineState.rootSignature	   = rootSignaturePtr;
		graphicsPipelineState.graphicsProgram  = &graphicsProgram;
		graphicsPipelineState.vertexAttributes = renderer.getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes();

		{ // TODO(co) Render pass related update, the render pass in here is currently just a dummy so the debug compositor works
			Rhi::IRhi& rhi = rootSignaturePtr->getRhi();
			graphicsPipelineState.renderPass = rhi.createRenderPass(1, &rhi.getCapabilities().preferredSwapChainColorTextureFormat, rhi.getCapabilities().preferredSwapChainDepthStencilTextureFormat);
		}

		// Create the graphics pipeline state object (PSO)
		Rhi::IGraphicsPipelineState* graphicsPipelineStateResource = rootSignaturePtr->getRhi().createGraphicsPipelineState(graphicsPipelineState);
		RHI_SET_RESOURCE_DEBUG_NAME(graphicsPipelineStateResource, "Graphics pipeline state compiler")

		// Done
		return graphicsPipelineStateResource;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
