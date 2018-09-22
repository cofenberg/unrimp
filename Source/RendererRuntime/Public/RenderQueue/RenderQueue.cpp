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
#include "RendererRuntime/Public/RenderQueue/RenderQueue.h"
#include "RendererRuntime/Public/RenderQueue/RenderableManager.h"
#include "RendererRuntime/Public/Resource/CompositorWorkspace/CompositorContextData.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Public/Resource/Material/MaterialTechnique.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/PassBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/LightBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/IndirectBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/UniformInstanceBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/TextureInstanceBufferManager.h"
#include "RendererRuntime/Public/Core/IProfiler.h"
#include "RendererRuntime/Public/Core/Math/Transform.h"
#include "RendererRuntime/Public/IRendererRuntime.h"

#include <array>
#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		const int DepthBits = 15;


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		// Flip the float to deal with negative & positive numbers
		// - See "Rough sorting by depth" - http://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/
		inline uint32_t floatFlip(uint32_t f)
		{
			const uint32_t mask = -int(f >> 31) | 0x80000000;
			return (f ^ mask);
		}

		// Taking highest 10 bits for rough sort of floats.
		// - 0.01 maps to 752; 0.1 to 759; 1.0 to 766; 10.0 to 772;
		// - 100.0 to 779 etc. Negative numbers go similarly in 0..511 range.
		// - See "Rough sorting by depth" - http://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/
		inline uint32_t depthToBits(float depth)
		{
			union { float f; uint32_t i; } f2i;
			f2i.f = depth;
			f2i.i = floatFlip(f2i.i);			// Flip bits to be sortable
			return (f2i.i >> (32 - DepthBits));	// Take highest n-bits
		}

		inline void setShaderPropertiesPropertyValue(RendererRuntime::MaterialPropertyId materialPropertyId, const RendererRuntime::MaterialPropertyValue& materialPropertyValue, RendererRuntime::ShaderProperties& shaderProperties)
		{
			switch (materialPropertyValue.getValueType())
			{
				case RendererRuntime::MaterialPropertyValue::ValueType::BOOLEAN:
					shaderProperties.setPropertyValue(materialPropertyId, materialPropertyValue.getBooleanValue());
					break;

				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER:
					shaderProperties.setPropertyValue(materialPropertyId, materialPropertyValue.getIntegerValue());
					break;

				case RendererRuntime::MaterialPropertyValue::ValueType::UNKNOWN:
				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_2:
				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_3:
				case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_4:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_2:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3_3:
				case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4_4:
				case RendererRuntime::MaterialPropertyValue::ValueType::FILL_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::CULL_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK:
				case RendererRuntime::MaterialPropertyValue::ValueType::STENCIL_OP:
				case RendererRuntime::MaterialPropertyValue::ValueType::COMPARISON_FUNC:
				case RendererRuntime::MaterialPropertyValue::ValueType::BLEND:
				case RendererRuntime::MaterialPropertyValue::ValueType::BLEND_OP:
				case RendererRuntime::MaterialPropertyValue::ValueType::FILTER_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE:
				case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID:
				case RendererRuntime::MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
				default:
					assert(false);	// TODO(co) Error handling
					break;
			}
		}

		FORCEINLINE void gatherShaderProperties(const RendererRuntime::MaterialResource& materialResource, const RendererRuntime::MaterialBlueprintResource& materialBlueprintResource, const RendererRuntime::MaterialProperties& globalMaterialProperties, const RendererRuntime::Renderable& renderable, bool singlePassStereoInstancing, RendererRuntime::ShaderProperties& shaderProperties, RendererRuntime::ShaderProperties& scratchOptimizedShaderProperties)
		{
			shaderProperties.clear();

			{ // Gather shader properties from static material properties generating shader combinations
				const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector = materialResource.getSortedPropertyVector();
				const size_t numberOfMaterialProperties = sortedMaterialPropertyVector.size();
				for (size_t i = 0; i < numberOfMaterialProperties; ++i)
				{
					const RendererRuntime::MaterialProperty& materialProperty = sortedMaterialPropertyVector[i];
					if (materialProperty.getUsage() == RendererRuntime::MaterialProperty::Usage::SHADER_COMBINATION)
					{
						switch (materialProperty.getValueType())
						{
							case RendererRuntime::MaterialPropertyValue::ValueType::BOOLEAN:
								shaderProperties.setPropertyValue(materialProperty.getMaterialPropertyId(), materialProperty.getBooleanValue());
								break;

							case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER:
								shaderProperties.setPropertyValue(materialProperty.getMaterialPropertyId(), materialProperty.getIntegerValue());
								break;

							case RendererRuntime::MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
							{
								const RendererRuntime::MaterialProperty* globalMaterialProperty = globalMaterialProperties.getPropertyById(materialProperty.getGlobalMaterialPropertyId());
								if (nullptr != globalMaterialProperty)
								{
									setShaderPropertiesPropertyValue(materialProperty.getMaterialPropertyId(), *globalMaterialProperty, shaderProperties);
								}
								else
								{
									// Try global material property reference fallback
									globalMaterialProperty = materialBlueprintResource.getMaterialProperties().getPropertyById(materialProperty.getGlobalMaterialPropertyId());
									if (nullptr != globalMaterialProperty)
									{
										setShaderPropertiesPropertyValue(materialProperty.getMaterialPropertyId(), *globalMaterialProperty, shaderProperties);
									}
									else
									{
										// Error, can't resolve reference
										assert(false);	// TODO(co) Error handling
									}
								}
								break;
							}

							case RendererRuntime::MaterialPropertyValue::ValueType::UNKNOWN:
							case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_2:
							case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_3:
							case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_4:
							case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT:
							case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_2:
							case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3:
							case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4:
							case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3_3:
							case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4_4:
							case RendererRuntime::MaterialPropertyValue::ValueType::FILL_MODE:
							case RendererRuntime::MaterialPropertyValue::ValueType::CULL_MODE:
							case RendererRuntime::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
							case RendererRuntime::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK:
							case RendererRuntime::MaterialPropertyValue::ValueType::STENCIL_OP:
							case RendererRuntime::MaterialPropertyValue::ValueType::COMPARISON_FUNC:
							case RendererRuntime::MaterialPropertyValue::ValueType::BLEND:
							case RendererRuntime::MaterialPropertyValue::ValueType::BLEND_OP:
							case RendererRuntime::MaterialPropertyValue::ValueType::FILTER_MODE:
							case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE:
							case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID:
							default:
								assert(false);	// TODO(co) Error handling
								break;
						}
					}
				}
			}

			// Automatic "UseGpuSkinning"-property setting
			if (RendererRuntime::isValid(renderable.getSkeletonResourceId()))
			{
				static constexpr uint32_t USE_GPU_SKINNING = STRING_ID("UseGpuSkinning");
				if (nullptr != materialBlueprintResource.getMaterialProperties().getPropertyById(USE_GPU_SKINNING))
				{
					shaderProperties.setPropertyValue(USE_GPU_SKINNING, 1);
				}
			}

			materialBlueprintResource.optimizeShaderProperties(shaderProperties, scratchOptimizedShaderProperties);

			// Automatic build-in "SinglePassStereoInstancing"-property setting
			if (singlePassStereoInstancing)
			{
				static constexpr uint32_t SINGLE_PASS_STEREO_INSTANCING = STRING_ID("SinglePassStereoInstancing");
				scratchOptimizedShaderProperties.setPropertyValue(SINGLE_PASS_STEREO_INSTANCING, 1);
			}
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	RenderQueue::RenderQueue(IndirectBufferManager& indirectBufferManager, uint8_t minimumRenderQueueIndex, uint8_t maximumRenderQueueIndex, bool transparentPass, bool doSort) :
		mRendererRuntime(indirectBufferManager.getRendererRuntime()),
		mIndirectBufferManager(indirectBufferManager),
		mNumberOfNullDrawCalls(0),
		mNumberOfDrawIndexedCalls(0),
		mNumberOfDrawCalls(0),
		mMinimumRenderQueueIndex(minimumRenderQueueIndex),
		mMaximumRenderQueueIndex(maximumRenderQueueIndex),
		mTransparentPass(transparentPass),
		mDoSort(doSort)
	{
		assert(mMaximumRenderQueueIndex >= mMinimumRenderQueueIndex);
		mQueues.resize(static_cast<size_t>(mMaximumRenderQueueIndex - mMinimumRenderQueueIndex + 1));
	}

	void RenderQueue::clear()
	{
		if (getNumberOfDrawCalls() > 0)
		{
			for (Queue& queue : mQueues)
			{
				queue.queuedRenderables.clear();
				queue.sorted = false;
			}
			mNumberOfNullDrawCalls = mNumberOfDrawIndexedCalls = mNumberOfDrawCalls = 0;
		}
	}

	void RenderQueue::addRenderablesFromRenderableManager(const RenderableManager& renderableManager, bool castShadows)
	{
		// Sanity check
		assert(renderableManager.isVisible());

		// Quantize the cached distance to camera
		// -> Solid: Sort from front to back to benefit from early z rejection
		// -> Transparent: Sort from back to front to have correct alpha blending
		const uint32_t quantizedDepth = ::detail::depthToBits(mTransparentPass ? -renderableManager.getCachedDistanceToCamera() : renderableManager.getCachedDistanceToCamera());

		// Register the renderables inside our renderables queue
		for (const Renderable& renderable : renderableManager.getRenderables())
		{
			if (!castShadows || renderable.getCastShadows())
			{
				// It's valid if one or more renderables inside a renderable manager don't fall into the range processed by this render queue
				// -> At least one renderable should fall into the range processed by this render queue or the render queue is used wrong
				const uint8_t renderQueueIndex = renderable.getRenderQueueIndex();
				if (renderQueueIndex >= mMinimumRenderQueueIndex && renderQueueIndex <= mMaximumRenderQueueIndex)
				{
					// Get the precalculated static part of the sorting key
					// -> Sort renderables back-to-front (for transparency) or front-to-back (for occlusion efficiency)
					// TODO(co) Depending on "mTransparentPass" the sorting key is used
					uint64_t sortingKey = renderable.getSortingKey();

					// The quantized depth is a dynamic part which is set now
					sortingKey = quantizedDepth;	// TODO(co) Just bits influenced

					// Register the renderable inside our renderables queue
					Queue& queue = mQueues[static_cast<size_t>(renderQueueIndex - mMinimumRenderQueueIndex)];
					assert(!queue.sorted);	// Ensure render queue is still in filling state and not already in rendering state
					queue.queuedRenderables.emplace_back(renderable, sortingKey);
					if (0 != renderable.getNumberOfIndices())
					{
						if (renderable.getDrawIndexed())
						{
							++mNumberOfDrawIndexedCalls;
						}
						else
						{
							++mNumberOfDrawCalls;
						}
					}
					else
					{
						++mNumberOfNullDrawCalls;
					}
				}
			}
		}
	}

	void RenderQueue::fillGraphicsCommandBuffer(const Renderer::IRenderTarget& renderTarget, MaterialTechniqueId materialTechniqueId, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		// Sanity check
		assert((getNumberOfDrawCalls() > 0) && "Don't call the fill command buffer method if there's no work to be done");
		assert(mScratchCommandBuffer.isEmpty() && "Scratch command buffer should be empty at this point in time");

		// No combined scoped profiler CPU and GPU sample as well as renderer debug event command by intent, this is something the caller has to take care of
		// RENDERER_SCOPED_PROFILER_EVENT(mRendererRuntime.getContext(), commandBuffer, "Graphics render queue")

		// TODO(co) This is just a dummy implementation. For example automatic instancing has to be incorporated as well as more efficient buffer management.
		const MaterialResourceManager& materialResourceManager = mRendererRuntime.getMaterialResourceManager();
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRendererRuntime.getMaterialBlueprintResourceManager();
		const MaterialProperties& globalMaterialProperties = materialBlueprintResourceManager.getGlobalMaterialProperties();
		UniformInstanceBufferManager& uniformInstanceBufferManager = materialBlueprintResourceManager.getUniformInstanceBufferManager();
		TextureInstanceBufferManager& textureInstanceBufferManager = materialBlueprintResourceManager.getTextureInstanceBufferManager();
		LightBufferManager& lightBufferManager = materialBlueprintResourceManager.getLightBufferManager();
		const bool singlePassStereoInstancing = compositorContextData.getSinglePassStereoInstancing();
		const uint32_t instanceCount = (singlePassStereoInstancing ? 2u : 1u);

		// Process all render queues
		// -> When adding renderables from renderable manager we could build up a minimum/maximum used render queue index to sometimes reduce
		//    the number of iterations. On the other hand, there are usually much more renderables added as iterations in here so this possible
		//    optimization might be a fact a performance degeneration while at the same time increasing the code complexity. So, not implemented by intent.
		if (mQueues.size() == 1 && mQueues[0].queuedRenderables.size() == 1)
		{
			// Material resource
			const Renderable& renderable = *mQueues[0].queuedRenderables[0].renderable;
			const MaterialResource* materialResource = materialResourceManager.tryGetById(renderable.getMaterialResourceId());
			if (nullptr != materialResource)
			{
				MaterialTechnique* materialTechnique = materialResource->getMaterialTechniqueById(materialTechniqueId);
				if (nullptr != materialTechnique)
				{
					MaterialBlueprintResource* materialBlueprintResource = materialBlueprintResourceManager.tryGetById(materialTechnique->getMaterialBlueprintResourceId());
					if (nullptr != materialBlueprintResource && IResource::LoadingState::LOADED == materialBlueprintResource->getLoadingState())
					{
						// TODO(co) Gather shader properties (later on we cache as much as possible of this work inside the renderable)
						::detail::gatherShaderProperties(*materialResource, *materialBlueprintResource, globalMaterialProperties, renderable, singlePassStereoInstancing, mScratchShaderProperties, mScratchOptimizedShaderProperties);

						Renderer::IGraphicsPipelineStatePtr graphicsPipelineStatePtr = materialBlueprintResource->getGraphicsPipelineStateCacheManager().getGraphicsPipelineStateCacheByCombination(materialTechnique->getSerializedGraphicsPipelineStateHash(), mScratchOptimizedShaderProperties, false);
						if (nullptr != graphicsPipelineStatePtr)
						{
							compositorContextData.mCurrentlyBoundMaterialBlueprintResource = materialBlueprintResource;

							// Set the used graphics pipeline state object (PSO)
							Renderer::Command::SetGraphicsPipelineState::create(commandBuffer, graphicsPipelineStatePtr);

							// Setup input assembly (IA): Set the used vertex array
							Renderer::Command::SetGraphicsVertexArray::create(commandBuffer, renderable.getVertexArrayPtr());

							{ // Fill the pass buffer manager
								PassBufferManager* passBufferManager = materialBlueprintResource->getPassBufferManager();
								if (nullptr != passBufferManager)
								{
									passBufferManager->fillBuffer(&renderTarget, compositorContextData, *materialResource);
								}
							}

							// Bind the graphics material blueprint resource and instance and light buffer manager to the used renderer
							materialBlueprintResource->fillGraphicsCommandBuffer(commandBuffer);
							const MaterialBlueprintResource::UniformBuffer* instanceUniformBuffer = materialBlueprintResource->getInstanceUniformBuffer();
							const MaterialBlueprintResource::TextureBuffer* instanceTextureBuffer = materialBlueprintResource->getInstanceTextureBuffer();
							if (nullptr != instanceTextureBuffer)
							{
								assert(nullptr != instanceUniformBuffer);
								textureInstanceBufferManager.startupBufferFilling(*materialBlueprintResource, commandBuffer);
							}
							else if (nullptr != instanceUniformBuffer)
							{
								uniformInstanceBufferManager.startupBufferFilling(*materialBlueprintResource, commandBuffer);
							}
							lightBufferManager.fillGraphicsCommandBuffer(*materialBlueprintResource, commandBuffer);

							// Cheap state change: Bind the material technique to the used renderer
							uint32_t textureResourceGroupRootParameterIndex = getInvalid<uint32_t>();
							Renderer::IResourceGroup* textureResourceGroup = nullptr;
							materialTechnique->fillGraphicsCommandBuffer(mRendererRuntime, commandBuffer, textureResourceGroupRootParameterIndex, &textureResourceGroup);
							if (isValid(textureResourceGroupRootParameterIndex) && nullptr != textureResourceGroup)
							{
								Renderer::Command::SetGraphicsResourceGroup::create(commandBuffer, textureResourceGroupRootParameterIndex, textureResourceGroup);
							}

							// Fill the instance buffer manager
							uint32_t startInstanceLocation = 0;
							if (nullptr != instanceTextureBuffer)
							{
								assert(nullptr != instanceUniformBuffer);
								startInstanceLocation = textureInstanceBufferManager.fillBuffer(compositorContextData.getWorldSpaceCameraPosition(), *materialBlueprintResource, materialBlueprintResource->getPassBufferManager(), *instanceUniformBuffer, renderable, *materialTechnique, commandBuffer);
							}
							else if (nullptr != instanceUniformBuffer)
							{
								startInstanceLocation = uniformInstanceBufferManager.fillBuffer(*materialBlueprintResource, materialBlueprintResource->getPassBufferManager(), *instanceUniformBuffer, renderable, *materialTechnique, commandBuffer);
							}

							// Render the specified geometric primitive, based on indexing into an array of vertices
							// -> Please note that it's valid that there are no indices, for example "RendererRuntime::CompositorInstancePassDebugGui" is using the render queue only to set the material resource blueprint
							if (0 != renderable.getNumberOfIndices())
							{
								// Fill indirect buffer
								if (renderable.getDrawIndexed())
								{
									Renderer::Command::DrawIndexedGraphics::create(commandBuffer, renderable.getNumberOfIndices(), instanceCount * renderable.getInstanceCount(), renderable.getStartIndexLocation(), 0,  startInstanceLocation);
								}
								else
								{
									Renderer::Command::DrawGraphics::create(commandBuffer, renderable.getNumberOfIndices(), instanceCount * renderable.getInstanceCount(), renderable.getStartIndexLocation(), startInstanceLocation);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			// Track currently bound renderer resources and states to void generating redundant commands
			bool vertexArraySet = false;
			Renderer::IVertexArray* currentVertexArray = nullptr;
			Renderer::IGraphicsPipelineState* currentGraphicsPipelineState = nullptr;

			// We try to minimize state changes across multiple render queue fill command buffer calls, but while doing so we still need to take into account
			// that pass data like world space to clip space transform might have been changed and needs to be updated inside the pass uniform buffer
			bool enforcePassBufferManagerFillBuffer = true;

			// Get indirect buffer
			Renderer::IIndirectBuffer* indirectBuffer = nullptr;
			uint32_t indirectBufferOffset = 0;
			uint8_t* indirectBufferData = nullptr;
			if (mNumberOfDrawIndexedCalls > 0 || mNumberOfDrawCalls > 0 )
			{
				IndirectBufferManager::IndirectBuffer* managedIndirectBuffer = mIndirectBufferManager.getIndirectBuffer(sizeof(Renderer::DrawIndexedArguments) * mNumberOfDrawIndexedCalls + sizeof(Renderer::DrawArguments) * mNumberOfDrawCalls);
				assert(nullptr != managedIndirectBuffer);
				indirectBuffer		 = managedIndirectBuffer->indirectBuffer;
				indirectBufferOffset = managedIndirectBuffer->indirectBufferOffset;
				indirectBufferData   = managedIndirectBuffer->mappedData;
			}

			// For gathering multi-draw-indirect data
			std::array<Renderer::IResourceGroup*, 16> currentSetGraphicsResourceGroup;	// TODO(co) Use maximum number of graphics resource groups here, 16 is considered a save number of root parameters
			uint32_t currentDrawIndirectBufferOffset = indirectBufferOffset;
			uint32_t currentNumberOfDraws = 0;
			bool currentDrawIndexed = false;

			for (Queue& queue : mQueues)
			{
				QueuedRenderables& queuedRenderables = queue.queuedRenderables;
				if (!queuedRenderables.empty())
				{
					// Sort queued renderables
					if (!queue.sorted && mDoSort)
					{
						// TODO(co) Exploit temporal coherence across frames then use insertion sorts as explained by L. Spiro in
						// http://www.gamedev.net/topic/661114-temporal-coherence-and-render-queue-sorting/?view=findpost&p=5181408
						// Keep a list of sorted indices from the previous frame (one per camera).
						// If we have the sorted list "5, 1, 4, 3, 2, 0":
						// * If it grew from last frame, append: 5, 1, 4, 3, 2, 0, 6, 7 and use insertion sort.
						// * If it's the same, leave it as is, and use insertion sort just in case.
						// * If it's shorter, reset the indices 0, 1, 2, 3, 4; probably use quicksort or other generic sort
						// TODO(co) Use radix sort? ( https://www.quora.com/What-is-the-most-efficient-way-to-sort-a-million-32-bit-integers )
						std::sort(queuedRenderables.begin(), queuedRenderables.end());
						queue.sorted = true;
					}

					// Inject queued renderables into the renderer
					for (const QueuedRenderable& queuedRenderable : queuedRenderables)
					{
						assert(nullptr != queuedRenderable.renderable);
						const Renderable& renderable = *queuedRenderable.renderable;

						// Material resource
						const MaterialResource* materialResource = materialResourceManager.tryGetById(renderable.getMaterialResourceId());
						if (nullptr != materialResource)
						{
							MaterialTechnique* materialTechnique = materialResource->getMaterialTechniqueById(materialTechniqueId);
							if (nullptr != materialTechnique)
							{
								MaterialBlueprintResource* materialBlueprintResource = materialBlueprintResourceManager.tryGetById(materialTechnique->getMaterialBlueprintResourceId());
								if (nullptr != materialBlueprintResource && IResource::LoadingState::LOADED == materialBlueprintResource->getLoadingState())
								{
									// TODO(co) Gather shader properties (later on we cache as much as possible of this work inside the renderable)
									::detail::gatherShaderProperties(*materialResource, *materialBlueprintResource, globalMaterialProperties, renderable, singlePassStereoInstancing, mScratchShaderProperties, mScratchOptimizedShaderProperties);

									Renderer::IGraphicsPipelineStatePtr graphicsPipelineStatePtr = materialBlueprintResource->getGraphicsPipelineStateCacheManager().getGraphicsPipelineStateCacheByCombination(materialTechnique->getSerializedGraphicsPipelineStateHash(), mScratchOptimizedShaderProperties, false);
									if (nullptr != graphicsPipelineStatePtr)
									{
										// Set the used graphics pipeline state object (PSO)
										if (currentGraphicsPipelineState != graphicsPipelineStatePtr)
										{
											currentGraphicsPipelineState = graphicsPipelineStatePtr;
											Renderer::Command::SetGraphicsPipelineState::create(mScratchCommandBuffer, currentGraphicsPipelineState);
										}

										{ // Setup input assembly (IA): Set the used vertex array
											Renderer::IVertexArrayPtr vertexArrayPtr = renderable.getVertexArrayPtr();
											if (!vertexArraySet || currentVertexArray != vertexArrayPtr)
											{
												vertexArraySet = true;
												currentVertexArray = vertexArrayPtr;
												Renderer::Command::SetGraphicsVertexArray::create(mScratchCommandBuffer, currentVertexArray);
											}
										}

										// Expensive state change: Handle material blueprint resource switches
										// -> Render queue should be sorted by material blueprint resource first to reduce those expensive state changes
										bool bindMaterialBlueprint = false;
										PassBufferManager* passBufferManager = nullptr;
										const MaterialBlueprintResource::UniformBuffer* instanceUniformBuffer = materialBlueprintResource->getInstanceUniformBuffer();
										const MaterialBlueprintResource::TextureBuffer* instanceTextureBuffer = materialBlueprintResource->getInstanceTextureBuffer();
										if (compositorContextData.mCurrentlyBoundMaterialBlueprintResource != materialBlueprintResource)
										{
											compositorContextData.mCurrentlyBoundMaterialBlueprintResource = materialBlueprintResource;
											std::fill(currentSetGraphicsResourceGroup.begin(), currentSetGraphicsResourceGroup.end(), nullptr);
											bindMaterialBlueprint = true;
										}
										if (bindMaterialBlueprint || enforcePassBufferManagerFillBuffer)
										{
											// Fill the pass buffer manager
											passBufferManager = materialBlueprintResource->getPassBufferManager();
											if (nullptr != passBufferManager)
											{
												passBufferManager->fillBuffer(&renderTarget, compositorContextData, *materialResource);
												enforcePassBufferManagerFillBuffer = false;
											}
										}
										if (bindMaterialBlueprint)
										{
											// Bind the graphics material blueprint resource and instance and light buffer manager to the used renderer
											materialBlueprintResource->fillGraphicsCommandBuffer(mScratchCommandBuffer);
											if (nullptr != instanceTextureBuffer)
											{
												assert(nullptr != instanceUniformBuffer);
												textureInstanceBufferManager.startupBufferFilling(*materialBlueprintResource, mScratchCommandBuffer);
											}
											else if (nullptr != instanceUniformBuffer)
											{
												uniformInstanceBufferManager.startupBufferFilling(*materialBlueprintResource, mScratchCommandBuffer);
											}
											lightBufferManager.fillGraphicsCommandBuffer(*materialBlueprintResource, mScratchCommandBuffer);
										}
										else if (nullptr != passBufferManager)
										{
											// Bind pass buffer manager since we filled the buffer
											passBufferManager->fillGraphicsCommandBuffer(mScratchCommandBuffer);
										}

										// Cheap state change: Bind the material technique to the used renderer
										uint32_t textureResourceGroupRootParameterIndex = getInvalid<uint32_t>();
										Renderer::IResourceGroup* textureResourceGroup = nullptr;
										materialTechnique->fillGraphicsCommandBuffer(mRendererRuntime, mScratchCommandBuffer, textureResourceGroupRootParameterIndex, &textureResourceGroup);
										if (isValid(textureResourceGroupRootParameterIndex) && nullptr != textureResourceGroup && currentSetGraphicsResourceGroup[textureResourceGroupRootParameterIndex] != textureResourceGroup)
										{
											currentSetGraphicsResourceGroup[textureResourceGroupRootParameterIndex] = textureResourceGroup;
											Renderer::Command::SetGraphicsResourceGroup::create(mScratchCommandBuffer, textureResourceGroupRootParameterIndex, textureResourceGroup);
										}

										// Fill the instance buffer manager
										uint32_t startInstanceLocation = 0;
										if (nullptr != instanceTextureBuffer)
										{
											assert(nullptr != instanceUniformBuffer);
											startInstanceLocation = textureInstanceBufferManager.fillBuffer(compositorContextData.getWorldSpaceCameraPosition(), *materialBlueprintResource, materialBlueprintResource->getPassBufferManager(), *instanceUniformBuffer, renderable, *materialTechnique, mScratchCommandBuffer);
										}
										else if (nullptr != instanceUniformBuffer)
										{
											startInstanceLocation = uniformInstanceBufferManager.fillBuffer(*materialBlueprintResource, materialBlueprintResource->getPassBufferManager(), *instanceUniformBuffer, renderable, *materialTechnique, mScratchCommandBuffer);
										}

										// Emit draw command, if necessary
										const Renderer::IIndirectBufferPtr renderableIndirectBufferPtr = renderable.getIndirectBufferPtr();
										if (renderable.getDrawIndexed() != currentDrawIndexed || !mScratchCommandBuffer.isEmpty() || nullptr != renderableIndirectBufferPtr)
										{
											if (currentDrawIndexed)
											{
												if (currentNumberOfDraws)
												{
													Renderer::Command::DrawIndexedGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
													currentNumberOfDraws = 0;
												}
											}
											else if (currentNumberOfDraws)
											{
												Renderer::Command::DrawGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
												currentNumberOfDraws = 0;
											}
											currentDrawIndirectBufferOffset = indirectBufferOffset;
										}

										// Inject scratch command buffer into the main command buffer
										if (!mScratchCommandBuffer.isEmpty())
										{
											mScratchCommandBuffer.submitToCommandBufferAndClear(commandBuffer);
										}

										// Render the specified geometric primitive, based on indexing into an array of vertices
										if (nullptr != renderableIndirectBufferPtr)
										{
											// Use a given indirect buffer which content is e.g. filled by a compute shader
											if (renderable.getDrawIndexed())
											{
												Renderer::Command::DrawIndexedGraphics::create(commandBuffer, *renderableIndirectBufferPtr, renderable.getIndirectBufferOffset(), renderable.getNumberOfDraws());
											}
											else
											{
												Renderer::Command::DrawGraphics::create(commandBuffer, *renderableIndirectBufferPtr, renderable.getIndirectBufferOffset(), renderable.getNumberOfDraws());
											}
										}
										// Please note that it's valid that there are no indices, for example "RendererRuntime::CompositorInstancePassDebugGui" is using the render queue only to set the material resource blueprint
										else if (0 != renderable.getNumberOfIndices())
										{
											// Sanity checks
											assert(nullptr != indirectBuffer);
											assert(nullptr != indirectBufferData);

											// Fill indirect buffer
											if (renderable.getDrawIndexed())
											{
												// Fill indirect buffer
												Renderer::DrawIndexedArguments* drawIndexedArguments = reinterpret_cast<Renderer::DrawIndexedArguments*>(indirectBufferData + indirectBufferOffset);
												drawIndexedArguments->indexCountPerInstance	= renderable.getNumberOfIndices();
												drawIndexedArguments->instanceCount			= instanceCount * renderable.getInstanceCount();
												drawIndexedArguments->startIndexLocation	= renderable.getStartIndexLocation();
												drawIndexedArguments->baseVertexLocation	= 0;
												drawIndexedArguments->startInstanceLocation	= startInstanceLocation;

												// Advance indirect buffer offset
												indirectBufferOffset += sizeof(Renderer::DrawIndexedArguments);
												currentDrawIndexed = true;
											}
											else
											{
												// Fill indirect buffer
												Renderer::DrawArguments* drawArguments = reinterpret_cast<Renderer::DrawArguments*>(indirectBufferData + indirectBufferOffset);
												drawArguments->vertexCountPerInstance = renderable.getNumberOfIndices();
												drawArguments->instanceCount		  = instanceCount * renderable.getInstanceCount();
												drawArguments->startVertexLocation	  = renderable.getStartIndexLocation();
												drawArguments->startInstanceLocation  = startInstanceLocation;

												// Advance indirect buffer offset
												indirectBufferOffset += sizeof(Renderer::DrawArguments);
												currentDrawIndexed = false;
											}
											++currentNumberOfDraws;
										}
									}
								}
							}
						}
					}
				}
			}

			// Emit last open draw command, if necessary
			if (currentNumberOfDraws)
			{
				if (currentDrawIndexed)
				{
					Renderer::Command::DrawIndexedGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
				}
				else
				{
					Renderer::Command::DrawGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
				}
			}
		}
	}

	// TODO(co) The "RendererRuntime::RenderQueue::fillComputeCommandBuffer"-method is heavily work in progress
	void RenderQueue::fillComputeCommandBuffer(MaterialTechniqueId materialTechniqueId, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		// Sanity check
		assert((getNumberOfDrawCalls() > 0) && "Don't call the fill command buffer method if there's no work to be done");
		assert(mScratchCommandBuffer.isEmpty() && "Scratch command buffer should be empty at this point in time");

		// No combined scoped profiler CPU and GPU sample as well as renderer debug event command by intent, this is something the caller has to take care of
		// RENDERER_SCOPED_PROFILER_EVENT(mRendererRuntime.getContext(), commandBuffer, "Compute render queue")

		// TODO(co) This is just a dummy implementation. For example automatic instancing has to be incorporated as well as more efficient buffer management.
		const TextureResourceManager& textureResourceManager = mRendererRuntime.getTextureResourceManager();
		const MaterialResourceManager& materialResourceManager = mRendererRuntime.getMaterialResourceManager();
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRendererRuntime.getMaterialBlueprintResourceManager();
		const MaterialProperties& globalMaterialProperties = materialBlueprintResourceManager.getGlobalMaterialProperties();
		// TextureInstanceBufferManager& textureInstanceBufferManager = materialBlueprintResourceManager.getTextureInstanceBufferManager();	// TODO(co) Think about compute instance buffer support
		LightBufferManager& lightBufferManager = materialBlueprintResourceManager.getLightBufferManager();
		const bool singlePassStereoInstancing = compositorContextData.getSinglePassStereoInstancing();

		// Process all render queues
		// -> When adding renderables from renderable manager we could build up a minimum/maximum used render queue index to sometimes reduce
		//    the number of iterations. On the other hand, there are usually much more renderables added as iterations in here so this possible
		//    optimization might be a fact a performance degeneration while at the same time increasing the code complexity. So, not implemented by intent.
		if (mQueues.size() == 1 && mQueues[0].queuedRenderables.size() == 1)
		{
			// Material resource
			const Renderable& renderable = *mQueues[0].queuedRenderables[0].renderable;
			const MaterialResource* materialResource = materialResourceManager.tryGetById(renderable.getMaterialResourceId());
			if (nullptr != materialResource)
			{
				MaterialTechnique* materialTechnique = materialResource->getMaterialTechniqueById(materialTechniqueId);
				if (nullptr != materialTechnique)
				{
					MaterialBlueprintResource* materialBlueprintResource = materialBlueprintResourceManager.tryGetById(materialTechnique->getMaterialBlueprintResourceId());
					if (nullptr != materialBlueprintResource && IResource::LoadingState::LOADED == materialBlueprintResource->getLoadingState())
					{
						// TODO(co) Gather shader properties (later on we cache as much as possible of this work inside the renderable)
						::detail::gatherShaderProperties(*materialResource, *materialBlueprintResource, globalMaterialProperties, renderable, singlePassStereoInstancing, mScratchShaderProperties, mScratchOptimizedShaderProperties);

						Renderer::IComputePipelineStatePtr computePipelineStatePtr = materialBlueprintResource->getComputePipelineStateCacheManager().getComputePipelineStateCacheByCombination(mScratchOptimizedShaderProperties, false);
						if (nullptr != computePipelineStatePtr)
						{
							compositorContextData.mCurrentlyBoundMaterialBlueprintResource = materialBlueprintResource;

							// Determine group count for dispatch compute
							uint32_t groupCountX = 0;
							uint32_t groupCountY = 0;
							uint32_t groupCountZ = 0;
							{
								// Use mandatory fixed build in material property "LocalComputeSize" for the compute shader local size (also known as number of threads)
								const MaterialProperty* materialProperty = materialResource->getPropertyById(MaterialResource::LOCAL_COMPUTE_SIZE_PROPERTY_ID);
								assert(nullptr != materialProperty);
								assert(materialProperty->getUsage() == MaterialProperty::Usage::STATIC);
								const int* localComputeSizeInteger3Value = materialProperty->getInteger3Value();

								// Use mandatory fixed build in material property "GlobalComputeSize" for the compute shader global size
								materialProperty = materialResource->getPropertyById(MaterialResource::GLOBAL_COMPUTE_SIZE_PROPERTY_ID);
								assert(nullptr != materialProperty);
								assert(materialProperty->getUsage() == MaterialProperty::Usage::STATIC || materialProperty->getUsage() == MaterialProperty::Usage::MATERIAL_REFERENCE);
								compositorContextData.mGlobalComputeSize[0] = 1;
								compositorContextData.mGlobalComputeSize[1] = 1;
								compositorContextData.mGlobalComputeSize[2] = 1;
								if (materialProperty->getUsage() == MaterialProperty::Usage::STATIC)
								{
									// Static value
									const int* globalComputeSizeInteger3Value = materialProperty->getInteger3Value();
									compositorContextData.mGlobalComputeSize[0] = static_cast<uint32_t>(globalComputeSizeInteger3Value[0]);
									compositorContextData.mGlobalComputeSize[1] = static_cast<uint32_t>(globalComputeSizeInteger3Value[1]);
									compositorContextData.mGlobalComputeSize[2] = static_cast<uint32_t>(globalComputeSizeInteger3Value[2]);
								}
								else
								{
									// Material property reference
									const MaterialPropertyId materialPropertyId = materialProperty->getReferenceValue();
									materialProperty = materialResource->getPropertyById(materialPropertyId);
									assert(materialProperty->getValueType() == MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID);
									assert(materialProperty->getUsage() == MaterialProperty::Usage::TEXTURE_REFERENCE);
									const TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(materialProperty->getTextureAssetIdValue());
									assert(nullptr != textureResource);
									const Renderer::ITexture* texture = textureResource->getTexture();
									assert(nullptr != texture);
									switch (texture->getResourceType())
									{
										case Renderer::ResourceType::TEXTURE_1D:
											compositorContextData.mGlobalComputeSize[0] = static_cast<const Renderer::ITexture1D*>(texture)->getWidth();
											break;

										case Renderer::ResourceType::TEXTURE_2D:
										{
											const Renderer::ITexture2D* texture2D = static_cast<const Renderer::ITexture2D*>(texture);
											compositorContextData.mGlobalComputeSize[0] = texture2D->getWidth();
											compositorContextData.mGlobalComputeSize[1] = texture2D->getHeight();
											break;
										}

										case Renderer::ResourceType::TEXTURE_2D_ARRAY:
										{
											const Renderer::ITexture2DArray* texture2DArray = static_cast<const Renderer::ITexture2DArray*>(texture);
											compositorContextData.mGlobalComputeSize[0] = texture2DArray->getWidth();
											compositorContextData.mGlobalComputeSize[1] = texture2DArray->getHeight();
											break;
										}

										case Renderer::ResourceType::TEXTURE_3D:
										{
											const Renderer::ITexture3D* texture3D = static_cast<const Renderer::ITexture3D*>(texture);
											compositorContextData.mGlobalComputeSize[0] = texture3D->getWidth();
											compositorContextData.mGlobalComputeSize[1] = texture3D->getHeight();
											compositorContextData.mGlobalComputeSize[2] = texture3D->getDepth();
											break;
										}

										case Renderer::ResourceType::TEXTURE_CUBE:
										{
											const Renderer::ITexture2D* texture2D = static_cast<const Renderer::ITexture2D*>(texture);
											compositorContextData.mGlobalComputeSize[0] = texture2D->getWidth();
											compositorContextData.mGlobalComputeSize[1] = texture2D->getHeight();
											compositorContextData.mGlobalComputeSize[2] = 6;	// TODO(co) Or better 1?
											break;
										}

										case Renderer::ResourceType::ROOT_SIGNATURE:
										case Renderer::ResourceType::RESOURCE_GROUP:
										case Renderer::ResourceType::GRAPHICS_PROGRAM:
										case Renderer::ResourceType::VERTEX_ARRAY:
										case Renderer::ResourceType::RENDER_PASS:
										case Renderer::ResourceType::SWAP_CHAIN:
										case Renderer::ResourceType::FRAMEBUFFER:
										case Renderer::ResourceType::INDEX_BUFFER:
										case Renderer::ResourceType::VERTEX_BUFFER:
										case Renderer::ResourceType::TEXTURE_BUFFER:
										case Renderer::ResourceType::STRUCTURED_BUFFER:
										case Renderer::ResourceType::INDIRECT_BUFFER:
										case Renderer::ResourceType::UNIFORM_BUFFER:
										case Renderer::ResourceType::GRAPHICS_PIPELINE_STATE:
										case Renderer::ResourceType::COMPUTE_PIPELINE_STATE:
										case Renderer::ResourceType::SAMPLER_STATE:
										case Renderer::ResourceType::VERTEX_SHADER:
										case Renderer::ResourceType::TESSELLATION_CONTROL_SHADER:
										case Renderer::ResourceType::TESSELLATION_EVALUATION_SHADER:
										case Renderer::ResourceType::GEOMETRY_SHADER:
										case Renderer::ResourceType::FRAGMENT_SHADER:
										case Renderer::ResourceType::COMPUTE_SHADER:
										default:
											assert(false);
											break;
									}
								}

								// Determine group count
								groupCountX = static_cast<uint32_t>(std::ceil(compositorContextData.mGlobalComputeSize[0] / static_cast<float>(localComputeSizeInteger3Value[0])));
								groupCountY = static_cast<uint32_t>(std::ceil(compositorContextData.mGlobalComputeSize[1] / static_cast<float>(localComputeSizeInteger3Value[1])));
								groupCountZ = static_cast<uint32_t>(std::ceil(compositorContextData.mGlobalComputeSize[2] / static_cast<float>(localComputeSizeInteger3Value[2])));
							}

							// Set the used compute pipeline state object (PSO)
							Renderer::Command::SetComputePipelineState::create(commandBuffer, computePipelineStatePtr);

							{ // Fill the pass buffer manager
								PassBufferManager* passBufferManager = materialBlueprintResource->getPassBufferManager();
								if (nullptr != passBufferManager)
								{
									passBufferManager->fillBuffer(nullptr, compositorContextData, *materialResource);
								}
							}

							// Bind the compute material blueprint resource and instance and light buffer manager to the used renderer
							materialBlueprintResource->fillComputeCommandBuffer(commandBuffer);
							const MaterialBlueprintResource::UniformBuffer* instanceUniformBuffer = materialBlueprintResource->getInstanceUniformBuffer();
							if (nullptr != instanceUniformBuffer)
							{
								// TODO(co) Think about compute instance buffer support
								assert(false);
								// textureInstanceBufferManager.startupBufferFilling(*materialBlueprintResource, commandBuffer);
							}
							lightBufferManager.fillComputeCommandBuffer(*materialBlueprintResource, commandBuffer);

							// Cheap state change: Bind the material technique to the used renderer
							uint32_t textureResourceGroupRootParameterIndex = getInvalid<uint32_t>();
							Renderer::IResourceGroup* textureResourceGroup = nullptr;
							materialTechnique->fillComputeCommandBuffer(mRendererRuntime, commandBuffer, textureResourceGroupRootParameterIndex, &textureResourceGroup);
							if (isValid(textureResourceGroupRootParameterIndex) && nullptr != textureResourceGroup)
							{
								Renderer::Command::SetComputeResourceGroup::create(commandBuffer, textureResourceGroupRootParameterIndex, textureResourceGroup);
							}

							// Fill the instance buffer manager
							// TODO(co) Think about compute instance buffer support
							// MAYBE_UNUSED const uint32_t startInstanceLocation = (nullptr != instanceUniformBuffer) ? textureInstanceBufferManager.fillBuffer(*materialBlueprintResource, materialBlueprintResource->getPassBufferManager(), *instanceUniformBuffer, renderable, *materialTechnique, commandBuffer) : 0;

							// Dispatch compute
							Renderer::Command::DispatchCompute::create(commandBuffer, groupCountX, groupCountY, groupCountZ);
						}
					}
				}
			}
		}
		else
		{
			// TODO(co)
			assert(false);
			/*
			// Track currently bound renderer resources and states to void generating redundant commands
			bool vertexArraySet = false;
			Renderer::IVertexArray* currentVertexArray = nullptr;
			Renderer::IGraphicsPipelineState* currentGraphicsPipelineState = nullptr;

			// We try to minimize state changes across multiple render queue fill command buffer calls, but while doing so we still need to take into account
			// that pass data like world space to clip space transform might have been changed and needs to be updated inside the pass uniform buffer
			bool enforcePassBufferManagerFillBuffer = true;

			// Get indirect buffer
			Renderer::IIndirectBuffer* indirectBuffer = nullptr;
			uint32_t indirectBufferOffset = 0;
			uint8_t* indirectBufferData = nullptr;
			if (mNumberOfDrawIndexedCalls > 0 || mNumberOfDrawCalls > 0 )
			{
				IndirectBufferManager::IndirectBuffer* managedIndirectBuffer = mIndirectBufferManager.getIndirectBuffer(sizeof(Renderer::DrawIndexedArguments) * mNumberOfDrawIndexedCalls + sizeof(Renderer::DrawArguments) * mNumberOfDrawCalls);
				assert(nullptr != managedIndirectBuffer);
				indirectBuffer		 = managedIndirectBuffer->indirectBuffer;
				indirectBufferOffset = managedIndirectBuffer->indirectBufferOffset;
				indirectBufferData   = managedIndirectBuffer->mappedData;
			}

			// For gathering multi-draw-indirect data
			std::array<Renderer::IResourceGroup*, 16> currentSetGraphicsResourceGroup;	// TODO(co) Use maximum number of graphics resource groups here, 16 is considered a save number of root parameters
			uint32_t currentDrawIndirectBufferOffset = indirectBufferOffset;
			uint32_t currentNumberOfDraws = 0;
			bool currentDrawIndexed = false;

			for (Queue& queue : mQueues)
			{
				QueuedRenderables& queuedRenderables = queue.queuedRenderables;
				if (!queuedRenderables.empty())
				{
					// Sort queued renderables
					if (!queue.sorted && mDoSort)
					{
						// TODO(co) Exploit temporal coherence across frames then use insertion sorts as explained by L. Spiro in
						// http://www.gamedev.net/topic/661114-temporal-coherence-and-render-queue-sorting/?view=findpost&p=5181408
						// Keep a list of sorted indices from the previous frame (one per camera).
						// If we have the sorted list "5, 1, 4, 3, 2, 0":
						// * If it grew from last frame, append: 5, 1, 4, 3, 2, 0, 6, 7 and use insertion sort.
						// * If it's the same, leave it as is, and use insertion sort just in case.
						// * If it's shorter, reset the indices 0, 1, 2, 3, 4; probably use quicksort or other generic sort
						// TODO(co) Use radix sort? ( https://www.quora.com/What-is-the-most-efficient-way-to-sort-a-million-32-bit-integers )
						std::sort(queuedRenderables.begin(), queuedRenderables.end());
						queue.sorted = true;
					}

					// Inject queued renderables into the renderer
					for (const QueuedRenderable& queuedRenderable : queuedRenderables)
					{
						assert(nullptr != queuedRenderable.renderable);
						const Renderable& renderable = *queuedRenderable.renderable;

						// Material resource
						const MaterialResource* materialResource = materialResourceManager.tryGetById(renderable.getMaterialResourceId());
						if (nullptr != materialResource)
						{
							MaterialTechnique* materialTechnique = materialResource->getMaterialTechniqueById(materialTechniqueId);
							if (nullptr != materialTechnique)
							{
								MaterialBlueprintResource* materialBlueprintResource = materialBlueprintResourceManager.tryGetById(materialTechnique->getMaterialBlueprintResourceId());
								if (nullptr != materialBlueprintResource && IResource::LoadingState::LOADED == materialBlueprintResource->getLoadingState())
								{
									// TODO(co) Gather shader properties (later on we cache as much as possible of this work inside the renderable)
									::detail::gatherShaderProperties(*materialResource, *materialBlueprintResource, globalMaterialProperties, renderable, singlePassStereoInstancing, mScratchShaderProperties, mScratchOptimizedShaderProperties);

									Renderer::IGraphicsPipelineStatePtr graphicsPipelineStatePtr = materialBlueprintResource->getGraphicsPipelineStateCacheManager().getGraphicsPipelineStateCacheByCombination(materialTechnique->getSerializedGraphicsPipelineStateHash(), mScratchOptimizedShaderProperties, false);
									if (nullptr != graphicsPipelineStatePtr)
									{
										// Set the used graphics pipeline state object (PSO)
										if (currentGraphicsPipelineState != graphicsPipelineStatePtr)
										{
											currentGraphicsPipelineState = graphicsPipelineStatePtr;
											Renderer::Command::SetGraphicsPipelineState::create(mScratchCommandBuffer, currentGraphicsPipelineState);
										}

										{ // Setup input assembly (IA): Set the used vertex array
											Renderer::IVertexArrayPtr vertexArrayPtr = renderable.getVertexArrayPtr();
											if (!vertexArraySet || currentVertexArray != vertexArrayPtr)
											{
												vertexArraySet = true;
												currentVertexArray = vertexArrayPtr;
												Renderer::Command::SetGraphicsVertexArray::create(mScratchCommandBuffer, currentVertexArray);
											}
										}

										// Expensive state change: Handle material blueprint resource switches
										// -> Render queue should be sorted by material blueprint resource first to reduce those expensive state changes
										bool bindMaterialBlueprint = false;
										PassBufferManager* passBufferManager = nullptr;
										const MaterialBlueprintResource::UniformBuffer* instanceUniformBuffer = materialBlueprintResource->getInstanceUniformBuffer();
										if (compositorContextData.mCurrentlyBoundMaterialBlueprintResource != materialBlueprintResource)
										{
											compositorContextData.mCurrentlyBoundMaterialBlueprintResource = materialBlueprintResource;
											std::fill(currentSetGraphicsResourceGroup.begin(), currentSetGraphicsResourceGroup.end(), nullptr);
											bindMaterialBlueprint = true;
										}
										if (bindMaterialBlueprint || enforcePassBufferManagerFillBuffer)
										{
											// Fill the pass buffer manager
											passBufferManager = materialBlueprintResource->getPassBufferManager();
											if (nullptr != passBufferManager)
											{
												passBufferManager->fillBuffer(nullptr, compositorContextData, *materialResource);
												enforcePassBufferManagerFillBuffer = false;
											}
										}
										if (bindMaterialBlueprint)
										{
											// Bind the material blueprint resource and instance and light buffer manager to the used renderer
											materialBlueprintResource->fillCommandBuffer(mScratchCommandBuffer);
											if (nullptr != instanceUniformBuffer)
											{
												textureInstanceBufferManager.startupBufferFilling(*materialBlueprintResource, mScratchCommandBuffer);
											}
											lightBufferManager.fillCommandBuffer(*materialBlueprintResource, mScratchCommandBuffer);
										}
										else if (nullptr != passBufferManager)
										{
											// Bind pass buffer manager since we filled the buffer
											passBufferManager->fillCommandBuffer(mScratchCommandBuffer);
										}

										// Cheap state change: Bind the material technique to the used renderer
										uint32_t textureResourceGroupRootParameterIndex = getInvalid<uint32_t>();
										Renderer::IResourceGroup* textureResourceGroup = nullptr;
										materialTechnique->fillCommandBuffer(mRendererRuntime, mScratchCommandBuffer, textureResourceGroupRootParameterIndex, &textureResourceGroup);
										if (isValid(textureResourceGroupRootParameterIndex) && nullptr != textureResourceGroup && currentSetGraphicsResourceGroup[textureResourceGroupRootParameterIndex] != textureResourceGroup)
										{
											currentSetGraphicsResourceGroup[textureResourceGroupRootParameterIndex] = textureResourceGroup;
											Renderer::Command::SetGraphicsResourceGroup::create(mScratchCommandBuffer, textureResourceGroupRootParameterIndex, textureResourceGroup);
										}

										// Fill the instance buffer manager
										const uint32_t startInstanceLocation = (nullptr != instanceUniformBuffer) ? textureInstanceBufferManager.fillBuffer(*materialBlueprintResource, materialBlueprintResource->getPassBufferManager(), *instanceUniformBuffer, renderable, *materialTechnique, mScratchCommandBuffer) : 0;

										// Emit draw command, if necessary
										const Renderer::IIndirectBufferPtr renderableIndirectBufferPtr = renderable.getIndirectBufferPtr();
										if (renderable.getDrawIndexed() != currentDrawIndexed || !mScratchCommandBuffer.isEmpty() || nullptr != renderableIndirectBufferPtr)
										{
											if (currentDrawIndexed)
											{
												if (currentNumberOfDraws)
												{
													Renderer::Command::DrawIndexedGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
													currentNumberOfDraws = 0;
												}
											}
											else if (currentNumberOfDraws)
											{
												Renderer::Command::DrawGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
												currentNumberOfDraws = 0;
											}
											currentDrawIndirectBufferOffset = indirectBufferOffset;
										}

										// Inject scratch command buffer into the main command buffer
										if (!mScratchCommandBuffer.isEmpty())
										{
											mScratchCommandBuffer.submitToCommandBufferAndClear(commandBuffer);
										}

										// Render the specified geometric primitive, based on indexing into an array of vertices
										if (nullptr != renderableIndirectBufferPtr)
										{
											// Use a given indirect buffer which content is e.g. filled by a compute shader
											if (renderable.getDrawIndexed())
											{
												Renderer::Command::DrawIndexedGraphics::create(commandBuffer, *renderableIndirectBufferPtr, renderable.getIndirectBufferOffset(), renderable.getNumberOfDraws());
											}
											else
											{
												Renderer::Command::DrawGraphics::create(commandBuffer, *renderableIndirectBufferPtr, renderable.getIndirectBufferOffset(), renderable.getNumberOfDraws());
											}
										}
										// Please note that it's valid that there are no indices, for example "RendererRuntime::CompositorInstancePassDebugGui" is using the render queue only to set the material resource blueprint
										else if (0 != renderable.getNumberOfIndices())
										{
											// Sanity checks
											assert(nullptr != indirectBuffer);
											assert(nullptr != indirectBufferData);

											// Fill indirect buffer
											if (renderable.getDrawIndexed())
											{
												// Fill indirect buffer
												Renderer::DrawIndexedArguments* drawIndexedArguments = reinterpret_cast<Renderer::DrawIndexedArguments*>(indirectBufferData + indirectBufferOffset);
												drawIndexedArguments->indexCountPerInstance	= renderable.getNumberOfIndices();
												drawIndexedArguments->instanceCount			= instanceCount * renderable.getInstanceCount();
												drawIndexedArguments->startIndexLocation	= renderable.getStartIndexLocation();
												drawIndexedArguments->baseVertexLocation	= 0;
												drawIndexedArguments->startInstanceLocation	= startInstanceLocation;

												// Advance indirect buffer offset
												indirectBufferOffset += sizeof(Renderer::DrawIndexedArguments);
												currentDrawIndexed = true;
											}
											else
											{
												// Fill indirect buffer
												Renderer::DrawArguments* drawArguments = reinterpret_cast<Renderer::DrawArguments*>(indirectBufferData + indirectBufferOffset);
												drawArguments->vertexCountPerInstance = renderable.getNumberOfIndices();
												drawArguments->instanceCount		  = instanceCount * renderable.getInstanceCount();
												drawArguments->startVertexLocation	  = renderable.getStartIndexLocation();
												drawArguments->startInstanceLocation  = startInstanceLocation;

												// Advance indirect buffer offset
												indirectBufferOffset += sizeof(Renderer::DrawArguments);
												currentDrawIndexed = false;
											}
											++currentNumberOfDraws;
										}
									}
								}
							}
						}
					}
				}
			}

			// Emit last open draw command, if necessary
			if (currentNumberOfDraws)
			{
				if (currentDrawIndexed)
				{
					Renderer::Command::DrawIndexedGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
				}
				else
				{
					Renderer::Command::DrawGraphics::create(commandBuffer, *indirectBuffer, currentDrawIndirectBufferOffset, currentNumberOfDraws);
				}
			}
			*/
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
