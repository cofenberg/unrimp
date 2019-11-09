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
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCompiler.h"
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCache.h"
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
// TODO(co) See "Renderer::ComputePipelineStateCompiler::ComputePipelineStateCompiler()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void ComputePipelineStateCompiler::setAsynchronousCompilationEnabled(bool enabled)
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

	void ComputePipelineStateCompiler::setNumberOfCompilerThreads(uint32_t numberOfCompilerThreads)
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
				mCompilerThreads.push_back(std::thread(&ComputePipelineStateCompiler::compilerThreadWorker, this));
			}
		}
	}

	void ComputePipelineStateCompiler::dispatch()
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

			// Tell the compute pipeline state cache about the real compiled compute pipeline state object
			ComputePipelineStateCache& computePipelineStateCache = compilerRequest.computePipelineStateCache;
			computePipelineStateCache.mComputePipelineStateObjectPtr = compilerRequest.computePipelineStateObject;
			computePipelineStateCache.mIsUsingFallback = false;
			RHI_ASSERT(mRenderer.getContext(), 0 != mNumberOfInFlightCompilerRequests, "Invalid number of in flight compiler requests")
			--mNumberOfInFlightCompilerRequests;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	ComputePipelineStateCompiler::ComputePipelineStateCompiler(IRenderer& renderer) :
		mRenderer(renderer),
		mAsynchronousCompilationEnabled(renderer.getRhi().getCapabilities().nativeMultithreading),
		mNumberOfCompilerThreads(0),
		mNumberOfInFlightCompilerRequests(0),
		mShutdownBuilderThread(false),
		mBuilderThread(&ComputePipelineStateCompiler::builderThreadWorker, this),
		mShutdownCompilerThread(false)
	{
		// Create and start the threads
		setNumberOfCompilerThreads(2);
	}

	ComputePipelineStateCompiler::~ComputePipelineStateCompiler()
	{
		// Builder thread shutdown
		mShutdownBuilderThread = true;
		mBuilderConditionVariable.notify_one();
		mBuilderThread.join();

		// Compiler threads shutdown
		setNumberOfCompilerThreads(0);
	}

	void ComputePipelineStateCompiler::addAsynchronousCompilerRequest(ComputePipelineStateCache& computePipelineStateCache)
	{
		// Push the load request into the builder queue
		RHI_ASSERT(mRenderer.getContext(), mAsynchronousCompilationEnabled, "Asynchronous compilation isn't enabled")
		++mNumberOfInFlightCompilerRequests;
		std::unique_lock<std::mutex> builderMutexLock(mBuilderMutex);
		mBuilderQueue.emplace_back(CompilerRequest(computePipelineStateCache));
		builderMutexLock.unlock();
		mBuilderConditionVariable.notify_one();
	}

	void ComputePipelineStateCompiler::instantSynchronousCompilerRequest(MaterialBlueprintResource& materialBlueprintResource, ComputePipelineStateCache& computePipelineStateCache)
	{
		// Get the compute program cache; synchronous processing
		const Rhi::IRootSignaturePtr& rootSignaturePtr = materialBlueprintResource.getRootSignaturePtr();
		if (nullptr != rootSignaturePtr)
		{
			ShaderCache* shaderCache = mRenderer.getShaderBlueprintResourceManager().getShaderCacheManager().getComputeShaderCache(computePipelineStateCache.getComputePipelineStateSignature(), materialBlueprintResource, rootSignaturePtr->getRhi().getDefaultShaderLanguage());
			if (nullptr != shaderCache)
			{
				Rhi::IShader* shader = shaderCache->getShaderPtr();
				if (nullptr != shader)
				{
					computePipelineStateCache.mComputePipelineStateObjectPtr = createComputePipelineState(materialBlueprintResource, *shader);
				}
			}
		}
	}

	void ComputePipelineStateCompiler::flushQueue(std::mutex& mutex, const CompilerRequests& compilerRequests)
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

	void ComputePipelineStateCompiler::builderThreadWorker()
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

				{ // Do the work: Building the shader source code for the required combination
					const ComputePipelineStateSignature& computePipelineStateSignature = compilerRequest.computePipelineStateCache.getComputePipelineStateSignature();
					const ShaderBlueprintResourceId shaderBlueprintResourceId = materialBlueprintResourceManager.getById(computePipelineStateSignature.getMaterialBlueprintResourceId()).getComputeShaderBlueprintResourceId();
					if (isValid(shaderBlueprintResourceId))
					{
						// Get the shader cache identifier, often but not always identical to the shader combination ID
						const ShaderCacheId shaderCacheId = computePipelineStateSignature.getShaderCombinationId();

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
							// Try to create the new compute shader cache instance
							const ShaderBlueprintResource* shaderBlueprintResource = shaderBlueprintResourceManager.tryGetById(shaderBlueprintResourceId);
							if (nullptr != shaderBlueprintResource)
							{
								// Build the shader source code
								ShaderBuilder::BuildShader buildShader;
								shaderBuilder.createSourceCode(shaderPieceResourceManager, *shaderBlueprintResource, computePipelineStateSignature.getShaderProperties(), buildShader);
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
										compilerRequest.shaderSourceCode = sourceCode;
									}
								}
							}
							else
							{
								// TODO(co) Error handling
								RHI_ASSERT(mRenderer.getContext(), false, "Invalid shader blueprint resource")
							}
						}
						compilerRequest.shaderCache = shaderCache;
					}
				}

				{ // Push the compiler request into the queue of the asynchronous shader compilation
					std::unique_lock<std::mutex> compilerMutexLock(mCompilerMutex);
					mCompilerQueue.emplace_back(compilerRequest);
					compilerMutexLock.unlock();
					mCompilerConditionVariable.notify_one();
				}

				// We're ready for the next round
				builderMutexLock.lock();
			}
		}
	}

	void ComputePipelineStateCompiler::compilerThreadWorker()
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
				ShaderCache* shaderCache = compilerRequest.shaderCache;
				if (nullptr != shaderCache)
				{
					Rhi::IShader* shader = shaderCache->getShaderPtr();
					if (nullptr == shader)
					{
						// The shader instance is not ready, do we need to compile it right now or is this the job of a shader cache master?
						const std::string& shaderSourceCode = compilerRequest.shaderSourceCode;
						if (shaderSourceCode.empty())
						{
							// We're not aware of any shader source code but we need a shader cache, so, there must be a shader cache master we need to wait for
							// RHI_ASSERT(mRenderer.getContext(), nullptr != shaderCache->getMasterShaderCache(), "Invalid master shader cache")	// No assert by intent
							needToWaitForShaderCache = true;
						}
						else
						{
							// Create the shader instance
							shader = shaderLanguage.createComputeShaderFromSourceCode(shaderSourceCode.c_str(), &shaderCache->mShaderBytecode RHI_RESOURCE_DEBUG_NAME("Compute pipeline state compiler"));
							RHI_ASSERT(mRenderer.getContext(), nullptr != shader, "Invalid shader")	// TODO(co) Error handling
							shaderCache->mShaderPtr = shader;

							// Create the compute pipeline state object (PSO)
							compilerRequest.computePipelineStateObject = createComputePipelineState(materialBlueprintResourceManager.getById(compilerRequest.computePipelineStateCache.getComputePipelineStateSignature().getMaterialBlueprintResourceId()), *shader);

							// Push the compiler request into the queue of the synchronous shader dispatch
							std::lock_guard<std::mutex> dispatchMutexLock(mDispatchMutex);
							mDispatchQueue.emplace_back(compilerRequest);
						}
					}
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

	Rhi::IComputePipelineState* ComputePipelineStateCompiler::createComputePipelineState(const MaterialBlueprintResource& materialBlueprintResource, Rhi::IShader& shader) const
	{
		// Create the compute pipeline state object (PSO)
		const Rhi::IRootSignaturePtr& rootSignaturePtr = materialBlueprintResource.getRootSignaturePtr();
		RHI_ASSERT(mRenderer.getContext(), shader.getResourceType() == Rhi::ResourceType::COMPUTE_SHADER, "Invalid shader resource type")
		return rootSignaturePtr->getRhi().createComputePipelineState(*rootSignaturePtr, static_cast<Rhi::IComputeShader&>(shader) RHI_RESOURCE_DEBUG_NAME("Compute pipeline state compiler"));
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
