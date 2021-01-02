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
#include "Renderer/Public/Resource/Scene/Item/Grass/GrassSceneItem.h"
#include "Renderer/Public/Resource/Scene/SceneResource.h"
#include "Renderer/Public/Resource/Scene/SceneNode.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ISceneItem methods           ]
	//[-------------------------------------------------------]
	const RenderableManager* GrassSceneItem::getRenderableManager() const
	{
		// Sanity checks
		RHI_ASSERT(getContext(), Math::QUAT_IDENTITY == mRenderableManager.getTransform().rotation, "No rotation is supported to keep things simple")
		RHI_ASSERT(getContext(), Math::VEC3_ONE == mRenderableManager.getTransform().scale, "No scale is supported to keep things simple")

		// Call the base implementation
		return MaterialSceneItem::getRenderableManager();
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::ISceneItem methods        ]
	//[-------------------------------------------------------]
	void GrassSceneItem::onExecuteOnRendering([[maybe_unused]] const Rhi::IRenderTarget& renderTarget, [[maybe_unused]] const CompositorContextData& compositorContextData, [[maybe_unused]] Rhi::CommandBuffer& commandBuffer) const
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void GrassSceneItem::initialize()
	{
		// Call the base implementation
		if (mMaximumNumberOfGrass > 0)
		{
			MaterialSceneItem::initialize();
		}
	}

	void GrassSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager
		#ifdef RHI_DEBUG
			const char* debugName = "Grass";
			mRenderableManager.setDebugName(debugName);
		#endif
		const IRenderer& renderer = getSceneResource().getRenderer();
		const MaterialResourceManager& materialResourceManager = renderer.getMaterialResourceManager();
		const MaterialResourceId materialResourceId = getMaterialResourceId();
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, renderer.getMeshResourceManager().getDrawIdVertexArrayPtr(), materialResourceManager, materialResourceId, getInvalid<SkeletonResourceId>(), false, mIndirectBufferPtr, 0, 1 RHI_RESOURCE_DEBUG_NAME(debugName));
		mRenderableManager.updateCachedRenderablesData();

		// Tell the used material resource about our structured buffer
		for (MaterialTechnique* materialTechnique : materialResourceManager.getById(materialResourceId).getSortedMaterialTechniqueVector())
		{
			materialTechnique->setStructuredBufferPtr(2, mStructuredBufferPtr);
		}

		// We need "Renderer::ISceneItem::onExecuteOnRendering()" calls during runtime
		setCallExecuteOnRendering(true);
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	GrassSceneItem::GrassSceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource, false),	// TODO(co) Set bounding box
		mMaximumNumberOfGrass(3)	// TODO(co) Make this dynamic
	{
		// The RHI implementation must support structured buffers
		const IRenderer& renderer = getSceneResource().getRenderer();
		if (renderer.getRhi().getCapabilities().maximumStructuredBufferSize > 0)
		{
			// Create vertex array object (VAO)
			// Create the vertex buffer object (VBO)
			// TODO(co) Make this dynamic
			const GrassDataStruct grassData[3] =
			{
				{
					3.0f, -1.781f, 20.0f, 0.5f,
					1.0f,  1.0f,   1.0f, 0.4f
				},
				{
					5.0f, -1.781f, 19.0f, 1.0f,
					1.0f,  1.0f,   1.0f, 0.8f
				},
				{
					4.0f, -1.781f, 21.0f, 1.5f,
					1.0f,  1.0f,   1.0f, 1.2f
				}
			};
			Rhi::IBufferManager& bufferManager = renderer.getBufferManager();

			// Create the structured buffer
			mStructuredBufferPtr = bufferManager.createStructuredBuffer(sizeof(GrassDataStruct) * mMaximumNumberOfGrass, grassData, Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage::STATIC_DRAW, sizeof(GrassDataStruct) RHI_RESOURCE_DEBUG_NAME("Grass"));

			{ // Create the indirect buffer: Twelve vertices per grass (two quads), grass index = instance index
				const Rhi::DrawArguments drawArguments =
				{
					12,						// vertexCountPerInstance (uint32_t)
					mMaximumNumberOfGrass,	// instanceCount (uint32_t)
					0,						// startVertexLocation (uint32_t)
					0						// startInstanceLocation (uint32_t)
				};
				mIndirectBufferPtr = bufferManager.createIndirectBuffer(sizeof(Rhi::DrawArguments), &drawArguments, Rhi::IndirectBufferFlag::UNORDERED_ACCESS | Rhi::IndirectBufferFlag::DRAW_ARGUMENTS, Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME("Grass"));
			}
		}
		else
		{
			mMaximumNumberOfGrass = 0;
			RHI_LOG_ONCE(renderer.getContext(), COMPATIBILITY_WARNING, "The renderer grass scene item needs a RHI implementation with structured buffer support")
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
