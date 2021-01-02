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
#include "Renderer/Public/Resource/Scene/Item/Particles/ParticlesSceneItem.h"
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
	const RenderableManager* ParticlesSceneItem::getRenderableManager() const
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
	void ParticlesSceneItem::onExecuteOnRendering([[maybe_unused]] const Rhi::IRenderTarget& renderTarget, [[maybe_unused]] const CompositorContextData& compositorContextData, [[maybe_unused]] Rhi::CommandBuffer& commandBuffer) const
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void ParticlesSceneItem::initialize()
	{
		// Call the base implementation
		if (mMaximumNumberOfParticles > 0)
		{
			MaterialSceneItem::initialize();
		}
	}

	void ParticlesSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager: Six vertices per particle, particle index = instance index
		#ifdef RHI_DEBUG
			const char* debugName = "Particles";
			mRenderableManager.setDebugName(debugName);
		#endif
		const IRenderer& renderer = getSceneResource().getRenderer();
		const MaterialResourceManager& materialResourceManager = renderer.getMaterialResourceManager();
		const MaterialResourceId materialResourceId = getMaterialResourceId();
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, renderer.getMeshResourceManager().getDrawIdVertexArrayPtr(), materialResourceManager, materialResourceId, getInvalid<SkeletonResourceId>(), false, 0, 6, mMaximumNumberOfParticles RHI_RESOURCE_DEBUG_NAME(debugName));
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
	ParticlesSceneItem::ParticlesSceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource, false),	// TODO(co) Set bounding box
		mMaximumNumberOfParticles(8)				// TODO(co) Make this dynamic
	{
		// The RHI implementation must support structured buffers
		const IRenderer& renderer = getSceneResource().getRenderer();
		if (renderer.getRhi().getCapabilities().maximumStructuredBufferSize > 0)
		{
			// Create vertex array object (VAO)
			// Create the vertex buffer object (VBO)
			// TODO(co) Make this dynamic
			const ParticleDataStruct particlesData[8] =
			{
				{
					4.88f,  1.4f, -1.44f, 0.5f,
					 1.0f,  1.0f,   1.0f, 0.3f
				},
				{
					-6.2f,  1.4f, -1.44f, 0.5f,
					 1.0f,  1.0f,   1.0f, 0.3f
				},
				{
					4.88f,  1.4f,   2.2f, 0.5f,
					 1.0f,  1.0f,   1.0f, 0.3f
				},
				{
					-6.2f,  1.4f,   2.2f, 0.5f,
					 1.0f,  1.0f,   1.0f, 0.3f
				},
				{
					-12.0f, 1.39f,  -4.0f, 1.0f,
					  1.0f,  0.0f,   0.0f, 1.0f
				},
				{
					11.2f, 1.39f,  -4.0f, 1.0f,
					 0.0f,  1.0f,   0.0f, 1.0f
				},
				{
					-12.0f, 1.39f,   4.5f, 1.0f,
					  0.0f,  0.0f,   1.0f, 1.0f
				},
				{
					11.2f, 1.39f,   4.5f, 1.0f,
					 1.0f,  1.0f,   1.0f, 1.0f
				}
			};

			// Create the structured buffer
			mStructuredBufferPtr = renderer.getBufferManager().createStructuredBuffer(sizeof(ParticleDataStruct) * mMaximumNumberOfParticles, particlesData, Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage::STATIC_DRAW, sizeof(ParticleDataStruct) RHI_RESOURCE_DEBUG_NAME("Particles"));
		}
		else
		{
			mMaximumNumberOfParticles = 0;
			RHI_LOG_ONCE(renderer.getContext(), COMPATIBILITY_WARNING, "The renderer particles scene item needs a RHI implementation with structured buffer support")
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
