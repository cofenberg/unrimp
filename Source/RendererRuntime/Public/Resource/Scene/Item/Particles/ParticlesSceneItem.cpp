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
#include "RendererRuntime/Public/Resource/Scene/Item/Particles/ParticlesSceneItem.h"
#include "RendererRuntime/Public/Resource/Scene/SceneResource.h"
#include "RendererRuntime/Public/Resource/Scene/SceneNode.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void ParticlesSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager: Six vertices per particle, particle index = instance index
		const IRendererRuntime& rendererRuntime = getSceneResource().getRendererRuntime();
		const MaterialResourceManager& materialResourceManager = rendererRuntime.getMaterialResourceManager();
		const MaterialResourceId materialResourceId = getMaterialResourceId();
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, rendererRuntime.getMeshResourceManager().getDrawIdVertexArrayPtr(), materialResourceManager, materialResourceId, getInvalid<SkeletonResourceId>(), false, 0, 6, mMaximumNumberOfParticles);
		mRenderableManager.updateCachedRenderablesData();

		// Tell the used material resource about our structured buffer
		for (MaterialTechnique* materialTechnique : materialResourceManager.getById(materialResourceId).getSortedMaterialTechniqueVector())
		{
			materialTechnique->setStructuredBufferPtr(mStructuredBufferPtr);
		}
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	ParticlesSceneItem::ParticlesSceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource, false),	// TODO(co) Set bounding box
		mMaximumNumberOfParticles(8)	// TODO(co) Make this dynamic
	{
		{ // Create vertex array object (VAO)
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
			mStructuredBufferPtr = getSceneResource().getRendererRuntime().getBufferManager().createStructuredBuffer(sizeof(ParticleDataStruct) * mMaximumNumberOfParticles, particlesData, Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage::STATIC_DRAW, sizeof(ParticleDataStruct));
			RENDERER_SET_RESOURCE_DEBUG_NAME(mStructuredBufferPtr, "Particles structured buffer")
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
