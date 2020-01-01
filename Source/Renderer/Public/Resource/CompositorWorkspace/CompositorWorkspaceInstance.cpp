/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceResource.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorContextData.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeResource.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeResourceManager.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ICompositorPassFactory.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ICompositorResourcePass.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ICompositorInstancePass.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorResourcePassShadowMap.h"
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/IndirectBufferManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/LightBufferManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/PassBufferManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "Renderer/Public/Resource/Scene/SceneNode.h"
#include "Renderer/Public/Resource/Scene/SceneResource.h"
#include "Renderer/Public/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "Renderer/Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h"
#include "Renderer/Public/Resource/Scene/Culling/SceneCullingManager.h"
#include "Renderer/Public/Core/IProfiler.h"
#include "Renderer/Public/Core/Renderer/FramebufferManager.h"
#include "Renderer/Public/Core/Renderer/RenderTargetTextureManager.h"
#ifdef RENDERER_GRAPHICS_DEBUGGER
	#include "Renderer/Public/Core/IGraphicsDebugger.h"
#endif
#ifdef RENDERER_OPENVR
	#include "Renderer/Public/Vr/IVrManager.h"
#endif
#include "Renderer/Public/IRenderer.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CompositorWorkspaceInstance::CompositorWorkspaceInstance(IRenderer& renderer, AssetId compositorWorkspaceAssetId) :
		mRenderer(renderer),
		mNumberOfMultisamples(1),
		mCurrentlyUsedNumberOfMultisamples(1),
		mResolutionScale(1.0f),
		mRenderTargetWidth(getInvalid<uint32_t>()),
		mRenderTargetHeight(getInvalid<uint32_t>()),
		mExecutionRenderTarget(nullptr),
		mCompositorWorkspaceResourceId(getInvalid<CompositorWorkspaceResourceId>()),
		mFramebufferManagerInitialized(false),
		mCompositorInstancePassShadowMap(nullptr)
		#ifdef RHI_STATISTICS
			, mPipelineStatisticsQueryPoolPtr((renderer.getRhi().getNameId() == Rhi::NameId::OPENGL && strstr(renderer.getRhi().getCapabilities().deviceName, "AMD ") != nullptr) ? nullptr : renderer.getRhi().createQueryPool(Rhi::QueryType::PIPELINE_STATISTICS, 2 RHI_RESOURCE_DEBUG_NAME("Compositor workspace instance"))),	// TODO(co) When using OpenGL "GL_ARB_pipeline_statistics_query" features, "glCopyImageSubData()" will horribly stall/freeze on Windows using AMD Radeon 18.12.2 (tested on 16 December 2018). No issues with NVIDIA GeForce game ready driver 417.35 (release data 12/12/2018).
			mPreviousCurrentPipelineStatisticsQueryIndex(getInvalid<uint32_t>()),
			mCurrentPipelineStatisticsQueryIndex(0),
			mPipelineStatisticsQueryResult{}
		#endif
	{
		renderer.getCompositorWorkspaceResourceManager().loadCompositorWorkspaceResourceByAssetId(compositorWorkspaceAssetId, mCompositorWorkspaceResourceId, this);
	}

	CompositorWorkspaceInstance::~CompositorWorkspaceInstance()
	{
		// Cleanup
		destroySequentialCompositorNodeInstances();
	}

	void CompositorWorkspaceInstance::setNumberOfMultisamples(uint8_t numberOfMultisamples)
	{
		// Sanity checks
		RHI_ASSERT(mRenderer.getContext(), numberOfMultisamples == 1 || numberOfMultisamples == 2 || numberOfMultisamples == 4 || numberOfMultisamples == 8, "Invalid number of multisamples")
		RHI_ASSERT(mRenderer.getContext(), numberOfMultisamples <= mRenderer.getRhi().getCapabilities().maximumNumberOfMultisamples, "Invalid number of multisamples")

		// Set the value
		mNumberOfMultisamples = numberOfMultisamples;
	}

	const CompositorWorkspaceInstance::RenderQueueIndexRange* CompositorWorkspaceInstance::getRenderQueueIndexRangeByRenderQueueIndex(uint8_t renderQueueIndex) const
	{
		for (const RenderQueueIndexRange& renderQueueIndexRange : mRenderQueueIndexRanges)
		{
			if (renderQueueIndex >= renderQueueIndexRange.minimumRenderQueueIndex && renderQueueIndex <= renderQueueIndexRange.maximumRenderQueueIndex)
			{
				return &renderQueueIndexRange;
			}
		}
		return nullptr;
	}

	const ICompositorInstancePass* CompositorWorkspaceInstance::getFirstCompositorInstancePassByCompositorPassTypeId(CompositorPassTypeId compositorPassTypeId) const
	{
		for (const CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
		{
			for (const ICompositorInstancePass* compositorInstancePass : compositorNodeInstance->mCompositorInstancePasses)
			{
				if (compositorInstancePass->getCompositorResourcePass().getTypeId() == compositorPassTypeId)
				{
					return compositorInstancePass;
				}
			}
		}

		// No compositor instance pass with the provided compositor pass type ID found
		return nullptr;
	}

	void CompositorWorkspaceInstance::executeVr(Rhi::IRenderTarget& renderTarget, CameraSceneItem* cameraSceneItem, const LightSceneItem* lightSceneItem)
	{
		// Decide whether or not the VR-manager is used for rendering
		#ifdef RENDERER_OPENVR
			IVrManager& vrManager = mRenderer.getVrManager();
			if (vrManager.isRunning())
			{
				// Update the VR-manager just before rendering
				vrManager.updateHmdMatrixPose(cameraSceneItem);

				// Execute the compositor workspace instance
				vrManager.executeCompositorWorkspaceInstance(*this, renderTarget, cameraSceneItem, lightSceneItem);
			}
			else
		#endif
		{
			// Execute the compositor workspace instance
			execute(renderTarget, cameraSceneItem, lightSceneItem);
		}
	}

	void CompositorWorkspaceInstance::execute(Rhi::IRenderTarget& renderTarget, const CameraSceneItem* cameraSceneItem, const LightSceneItem* lightSceneItem, bool singlePassStereoInstancing)
	{
		// Clear the command buffer from the previous frame
		mCommandBuffer.clear();

		// We could directly clear the render queue index ranges renderable managers as soon as the frame rendering has been finished to avoid evil dangling pointers,
		// but on the other hand a responsible user might be interested in the potentially on-screen renderable managers to perform work which should only be performed
		// on potentially on-screen stuff
		// -> Ensure that this clear step is really always performed when calling this execute method (evil dangling alert)
		clearRenderQueueIndexRangesRenderableManagers();

		// Is the compositor workspace resource ready?
		const CompositorWorkspaceResource* compositorWorkspaceResource = mRenderer.getCompositorWorkspaceResourceManager().tryGetById(mCompositorWorkspaceResourceId);
		if (nullptr != compositorWorkspaceResource && compositorWorkspaceResource->getLoadingState() == IResource::LoadingState::LOADED)
		{
			// Tell the global material properties managed by the material blueprint resource manager about the number of multisamples
			// -> Since there can be multiple compositor workspace instances we can't do this once inside "Renderer::CompositorWorkspaceInstance::setNumberOfMultisamples()"
			MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRenderer.getMaterialBlueprintResourceManager();
			materialBlueprintResourceManager.getGlobalMaterialProperties().setPropertyById(STRING_ID("GlobalNumberOfMultisamples"), MaterialPropertyValue::fromInteger((mNumberOfMultisamples == 1) ? 0 : mNumberOfMultisamples));

			// Add reference to the render target
			renderTarget.addReference();
			mExecutionRenderTarget = &renderTarget;

			// Get the main render target size
			uint32_t renderTargetWidth  = 1;
			uint32_t renderTargetHeight = 1;
			renderTarget.getWidthAndHeight(renderTargetWidth, renderTargetHeight);

			{ // Do we need to destroy previous framebuffers and render target textures?
				bool destroy = false;
				if (mCurrentlyUsedNumberOfMultisamples != mNumberOfMultisamples)
				{
					mCurrentlyUsedNumberOfMultisamples = mNumberOfMultisamples;
					destroy = true;
				}
				{
					const uint32_t currentRenderTargetWidth  = static_cast<uint32_t>(static_cast<float>(renderTargetWidth) * mResolutionScale);
					const uint32_t currentRenderTargetHeight = static_cast<uint32_t>(static_cast<float>(renderTargetHeight) * mResolutionScale);
					if (mRenderTargetWidth != currentRenderTargetWidth || mRenderTargetHeight != currentRenderTargetHeight)
					{
						mRenderTargetWidth  = currentRenderTargetWidth;
						mRenderTargetHeight = currentRenderTargetHeight;
						destroy = true;
					}
				}
				if (destroy)
				{
					destroyFramebuffersAndRenderTargetTextures();
				}
			}

			// Create framebuffers and render target textures, if required
			if (!mFramebufferManagerInitialized)
			{
				createFramebuffersAndRenderTargetTextures(renderTarget);
			}

			// Begin scene rendering
			Rhi::IRhi& rhi = renderTarget.getRhi();
			if (rhi.beginScene())
			{
				#ifdef RENDERER_GRAPHICS_DEBUGGER
					IGraphicsDebugger& graphicsDebugger = mRenderer.getContext().getGraphicsDebugger();
					if (graphicsDebugger.getCaptureNextFrame())
					{
						graphicsDebugger.startFrameCapture((renderTarget.getResourceType() == Rhi::ResourceType::SWAP_CHAIN) ? static_cast<Rhi::ISwapChain&>(renderTarget).getNativeWindowHandle() : NULL_HANDLE);
					}
				#endif
				#ifdef RHI_STATISTICS
					if (nullptr != mPipelineStatisticsQueryPoolPtr)
					{
						Rhi::Command::ResetAndBeginQuery::create(mCommandBuffer, *mPipelineStatisticsQueryPoolPtr, mCurrentPipelineStatisticsQueryIndex);
					}
				#endif

				const CompositorContextData compositorContextData(this, cameraSceneItem, singlePassStereoInstancing, lightSceneItem, mCompositorInstancePassShadowMap);
				if (nullptr != cameraSceneItem)
				{
					// Gather render queue index ranges renderable managers
					cameraSceneItem->getSceneResource().getSceneCullingManager().gatherRenderQueueIndexRangesRenderableManagers(renderTarget, compositorContextData, mRenderQueueIndexRanges);

					// Fill the light buffer manager
					materialBlueprintResourceManager.getLightBufferManager().fillBuffer(compositorContextData.getWorldSpaceCameraPosition(), cameraSceneItem->getSceneResource(), mCommandBuffer);
				}

				{ // Scene rendering
					// Combined scoped profiler CPU and GPU sample as well as RHI debug event command
					RENDERER_SCOPED_PROFILER_EVENT(mRenderer.getContext(), mCommandBuffer, "Compositor workspace")

					// Fill command buffer
					Rhi::IRenderTarget* currentRenderTarget = &renderTarget;
					for (const CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
					{
						currentRenderTarget = &compositorNodeInstance->fillCommandBuffer(*currentRenderTarget, compositorContextData, mCommandBuffer);
					}
				}

				{ // Submit command buffer to the RHI implementation
					// The command buffer is about to be submitted, inform everyone who cares about this
					materialBlueprintResourceManager.onPreCommandBufferExecution();

					// Submit command buffer to the RHI implementation
					#ifdef RHI_STATISTICS
						if (nullptr != mPipelineStatisticsQueryPoolPtr)
						{
							Rhi::Command::EndQuery::create(mCommandBuffer, *mPipelineStatisticsQueryPoolPtr, mCurrentPipelineStatisticsQueryIndex);
						}
					#endif
					mCommandBuffer.submitToRhi(rhi);

					// The command buffer has been submitted, inform everyone who cares about this
					for (const CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
					{
						compositorNodeInstance->onPostCommandBufferExecution();
					}
					{
						const uint32_t numberOfResources = materialBlueprintResourceManager.getNumberOfResources();
						for (uint32_t i = 0; i < numberOfResources; ++i)
						{
							PassBufferManager* passBufferManager = materialBlueprintResourceManager.getByIndex(i).getPassBufferManager();
							if (nullptr != passBufferManager)
							{
								passBufferManager->onPostCommandBufferExecution();
							}
						}
					}
				}

				// End scene rendering
				#ifdef RENDERER_GRAPHICS_DEBUGGER
					if (graphicsDebugger.getCaptureNextFrame())
					{
						graphicsDebugger.endFrameCapture((renderTarget.getResourceType() == Rhi::ResourceType::SWAP_CHAIN) ? static_cast<Rhi::ISwapChain&>(renderTarget).getNativeWindowHandle() : NULL_HANDLE);
					}
				#endif
				rhi.endScene();
			}

			// In case the render target is a swap chain, present the content of the current back buffer
			if (renderTarget.getResourceType() == Rhi::ResourceType::SWAP_CHAIN)
			{
				static_cast<Rhi::ISwapChain&>(renderTarget).present();
			}

			// Pipeline statistics query pool
			#ifdef RHI_STATISTICS
				if (nullptr != mPipelineStatisticsQueryPoolPtr)
				{
					// We explicitly wait ("Rhi::QueryResultFlags::WAIT" default value) if the previous result isn't available yet to avoid
					// "D3D11 WARNING: ID3D10Query::Begin: Begin is being invoked on a Query, where the previous results have not been obtained with GetData. This is valid; but unusual. The previous results are being abandoned, and new Query results will be generated. [ EXECUTION WARNING #408: QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS]"
					if (isValid(mPreviousCurrentPipelineStatisticsQueryIndex) && !rhi.getQueryPoolResults(*mPipelineStatisticsQueryPoolPtr, sizeof(Rhi::PipelineStatisticsQueryResult), reinterpret_cast<uint8_t*>(&mPipelineStatisticsQueryResult), mPreviousCurrentPipelineStatisticsQueryIndex))
					{
						mPipelineStatisticsQueryResult = {};
					}
					mPreviousCurrentPipelineStatisticsQueryIndex = mCurrentPipelineStatisticsQueryIndex;
					mCurrentPipelineStatisticsQueryIndex = (0 == mCurrentPipelineStatisticsQueryIndex) ? 1u : 0u;
				}
			#endif

			// Release reference from the render target
			mExecutionRenderTarget = nullptr;
			renderTarget.releaseReference();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::IResourceListener methods ]
	//[-------------------------------------------------------]
	void CompositorWorkspaceInstance::onLoadingStateChange(const IResource& resource)
	{
		// Destroy the previous stuff
		destroySequentialCompositorNodeInstances();

		// Handle loaded state
		if (resource.getLoadingState() == IResource::LoadingState::LOADED)
		{
			// TODO(co) Just a first test, need to complete and refine the implementation
			CompositorNodeResourceManager& compositorNodeResourceManager = mRenderer.getCompositorNodeResourceManager();
			const ICompositorPassFactory& compositorPassFactory = compositorNodeResourceManager.getCompositorPassFactory();
			RenderTargetTextureManager& renderTargetTextureManager = mRenderer.getCompositorWorkspaceResourceManager().getRenderTargetTextureManager();
			FramebufferManager& framebufferManager = mRenderer.getCompositorWorkspaceResourceManager().getFramebufferManager();

			// For render queue index ranges gathering and merging
			typedef std::pair<uint8_t, uint8_t> LocalRenderQueueIndexRange;
			typedef std::vector<LocalRenderQueueIndexRange> LocalRenderQueueIndexRanges;
			LocalRenderQueueIndexRanges individualRenderQueueIndexRanges;

			// Compositor node resources
			const CompositorWorkspaceResource::CompositorNodeAssetIds& compositorNodeAssetIds = static_cast<const CompositorWorkspaceResource&>(resource).getCompositorNodeAssetIds();
			const size_t numberOfCompositorResourceNodes = compositorNodeAssetIds.size();
			for (size_t nodeIndex = 0; nodeIndex < numberOfCompositorResourceNodes; ++nodeIndex)
			{
				// Get the compositor node resource instance
				CompositorNodeResourceId compositorNodeResourceId = getInvalid<CompositorNodeResourceId>();
				compositorNodeResourceManager.loadCompositorNodeResourceByAssetId(compositorNodeAssetIds[nodeIndex], compositorNodeResourceId);
				CompositorNodeResource& compositorNodeResource = compositorNodeResourceManager.getById(compositorNodeResourceId);

				// TODO(co) Ensure compositor node resource loading is done. Such blocking waiting is no good thing.
				compositorNodeResource.enforceFullyLoaded();

				// Add render target textures and framebuffers (doesn't directly allocate RHI resources, just announces them)
				for (const CompositorRenderTargetTexture& compositorRenderTargetTexture : compositorNodeResource.getRenderTargetTextures())
				{
					renderTargetTextureManager.addRenderTargetTexture(compositorRenderTargetTexture.getAssetId(), compositorRenderTargetTexture.getRenderTargetTextureSignature());
				}
				for (const CompositorFramebuffer& compositorFramebuffer : compositorNodeResource.getFramebuffers())
				{
					framebufferManager.addFramebuffer(compositorFramebuffer.getCompositorFramebufferId(), compositorFramebuffer.getFramebufferSignature());
				}

				// Create the compositor node instance
				CompositorNodeInstance* compositorNodeInstance = new CompositorNodeInstance(compositorNodeResourceId, *this);
				mSequentialCompositorNodeInstances.push_back(compositorNodeInstance);

				{ // Compositor node resource targets
					const CompositorNodeResource::CompositorTargets& compositorTargets = compositorNodeResource.getCompositorTargets();
					const size_t numberOfCompositorTargets = compositorTargets.size();
					for (size_t targetIndex = 0; targetIndex < numberOfCompositorTargets; ++targetIndex)
					{
						// Get the compositor node resource target instance
						const CompositorTarget& compositorTarget = compositorTargets[targetIndex];

						{ // Compositor node resource target passes
							const CompositorTarget::CompositorResourcePasses& compositorResourcePasses = compositorTarget.getCompositorResourcePasses();
							const size_t numberOfCompositorResourcePasses = compositorResourcePasses.size();
							for (size_t passIndex = 0; passIndex < numberOfCompositorResourcePasses; ++passIndex)
							{
								// Get the compositor resource target instance
								const ICompositorResourcePass* compositorResourcePass = compositorResourcePasses[passIndex];

								// Create the compositor instance pass
								if (nullptr != compositorResourcePass)
								{
									// Create the compositor instance pass
									ICompositorInstancePass* compositorInstancePass = compositorPassFactory.createCompositorInstancePass(*compositorResourcePass, *compositorNodeInstance);
									if (compositorResourcePass->getTypeId() == CompositorResourcePassShadowMap::TYPE_ID)
									{
										RHI_ASSERT(mRenderer.getContext(), nullptr == mCompositorInstancePassShadowMap, "Invalid compositor instance pass shadow map")
										mCompositorInstancePassShadowMap = static_cast<CompositorInstancePassShadowMap*>(compositorInstancePass);
									}
									compositorNodeInstance->mCompositorInstancePasses.push_back(compositorInstancePass);

									// Gather render queue index range
									uint8_t minimumRenderQueueIndex = 0;
									uint8_t maximumRenderQueueIndex = 0;
									if (compositorResourcePass->getRenderQueueIndexRange(minimumRenderQueueIndex, maximumRenderQueueIndex))
									{
										individualRenderQueueIndexRanges.emplace_back(minimumRenderQueueIndex, maximumRenderQueueIndex);
									}
								}
							}
						}
					}
				}
			}

			// Merge the render queue index ranges using the algorithm described at http://stackoverflow.com/a/5276789
			if (!individualRenderQueueIndexRanges.empty())
			{
				LocalRenderQueueIndexRanges mergedRenderQueueIndexRanges;
				mergedRenderQueueIndexRanges.reserve(individualRenderQueueIndexRanges.size());
				std::sort(individualRenderQueueIndexRanges.begin(), individualRenderQueueIndexRanges.end());
				LocalRenderQueueIndexRanges::iterator iterator = individualRenderQueueIndexRanges.begin();
				LocalRenderQueueIndexRange current = *(iterator)++;
				while (iterator != individualRenderQueueIndexRanges.end())
				{
					if (current.second >= iterator->first)
					{
						current.second = std::max(current.second, iterator->second);
					}
					else
					{
						mergedRenderQueueIndexRanges.push_back(current);
						current = *iterator;
					}
					++iterator;
				}
				mergedRenderQueueIndexRanges.push_back(current);

				// Fill our final render queue index ranges data structure
				mRenderQueueIndexRanges.reserve(mergedRenderQueueIndexRanges.size());
				for (const LocalRenderQueueIndexRange& localRenderQueueIndexRange : mergedRenderQueueIndexRanges)
				{
					mRenderQueueIndexRanges.emplace_back(localRenderQueueIndexRange.first, localRenderQueueIndexRange.second);
				}
			}

			// Tell all compositor node instances that the compositor workspace instance loading has been finished
			for (const CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
			{
				compositorNodeInstance->compositorWorkspaceInstanceLoadingFinished();
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void CompositorWorkspaceInstance::destroySequentialCompositorNodeInstances()
	{
		for (CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
		{
			delete compositorNodeInstance;
		}
		mSequentialCompositorNodeInstances.clear();
		mRenderQueueIndexRanges.clear();
		mCompositorInstancePassShadowMap = nullptr;

		// Destroy framebuffers and render target textures
		destroyFramebuffersAndRenderTargetTextures(true);
	}

	void CompositorWorkspaceInstance::createFramebuffersAndRenderTargetTextures(const Rhi::IRenderTarget& mainRenderTarget)
	{
		RHI_ASSERT(mRenderer.getContext(), !mFramebufferManagerInitialized, "Framebuffer manager is already initialized")
		CompositorWorkspaceResourceManager& compositorWorkspaceResourceManager = mRenderer.getCompositorWorkspaceResourceManager();

		{ // Framebuffers
			FramebufferManager& framebufferManager = compositorWorkspaceResourceManager.getFramebufferManager();
			for (const CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
			{
				for (ICompositorInstancePass* compositorInstancePass : compositorNodeInstance->mCompositorInstancePasses)
				{
					const CompositorFramebufferId compositorFramebufferId = compositorInstancePass->getCompositorResourcePass().getCompositorTarget().getCompositorFramebufferId();
					if (isValid(compositorFramebufferId))
					{
						compositorInstancePass->mRenderTarget = framebufferManager.getFramebufferByCompositorFramebufferId(compositorFramebufferId, mainRenderTarget, mCurrentlyUsedNumberOfMultisamples, mResolutionScale);
					}
				}
			}
		}

		{ // Textures not referenced by a framebuffer (e.g. used for unordered access or resource copy)
			RenderTargetTextureManager& renderTargetTextureManager = compositorWorkspaceResourceManager.getRenderTargetTextureManager();
			const CompositorNodeResourceManager& compositorNodeResourceManager = mRenderer.getCompositorNodeResourceManager();
			for (const CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
			{
				const CompositorNodeResource& compositorNodeResource = compositorNodeResourceManager.getById(compositorNodeInstance->getCompositorNodeResourceId());
				for (const CompositorRenderTargetTexture& compositorRenderTargetTexture : compositorNodeResource.getRenderTargetTextures())
				{
					const RenderTargetTextureSignature& renderTargetTextureSignature = compositorRenderTargetTexture.getRenderTargetTextureSignature();
					if ((renderTargetTextureSignature.getFlags() & RenderTargetTextureSignature::Flag::RENDER_TARGET) == 0)
					{
						// Force creating the texture in case it doesn't exist yet
						[[maybe_unused]] Rhi::ITexture* texture = renderTargetTextureManager.getTextureByAssetId(compositorRenderTargetTexture.getAssetId(), mainRenderTarget, mCurrentlyUsedNumberOfMultisamples, mResolutionScale, nullptr);
					}
				}
			}
		}

		mFramebufferManagerInitialized = true;
	}

	void CompositorWorkspaceInstance::destroyFramebuffersAndRenderTargetTextures(bool clearManagers)
	{
		// All compositor instance passes need to forget about the render targets
		for (CompositorNodeInstance* compositorNodeInstance : mSequentialCompositorNodeInstances)
		{
			for (ICompositorInstancePass* compositorInstancePass : compositorNodeInstance->mCompositorInstancePasses)
			{
				compositorInstancePass->mRenderTarget = nullptr;
				compositorInstancePass->mNumberOfExecutionRequests = 0;
			}
		}

		// Destroy RHI resources of framebuffers and render target textures
		CompositorWorkspaceResourceManager& compositorWorkspaceResourceManager = mRenderer.getCompositorWorkspaceResourceManager();
		if (clearManagers)
		{
			compositorWorkspaceResourceManager.getFramebufferManager().clear();
			compositorWorkspaceResourceManager.getRenderTargetTextureManager().clear();
		}
		else
		{
			compositorWorkspaceResourceManager.getFramebufferManager().clearRhiResources();
			compositorWorkspaceResourceManager.getRenderTargetTextureManager().clearRhiResources();
		}
		mFramebufferManagerInitialized = false;
	}

	void CompositorWorkspaceInstance::clearRenderQueueIndexRangesRenderableManagers()
	{
		// Forget about all previously gathered renderable managers
		for (RenderQueueIndexRange& renderQueueIndexRange : mRenderQueueIndexRanges)
		{
			renderQueueIndexRange.renderableManagers.clear();
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
