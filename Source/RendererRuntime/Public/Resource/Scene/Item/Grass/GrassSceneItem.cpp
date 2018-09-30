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
#include "RendererRuntime/Public/Resource/Scene/Item/Grass/GrassSceneItem.h"
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
	void GrassSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager
		const IRendererRuntime& rendererRuntime = getSceneResource().getRendererRuntime();
		const MaterialResourceManager& materialResourceManager = rendererRuntime.getMaterialResourceManager();
		const MaterialResourceId materialResourceId = getMaterialResourceId();
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, rendererRuntime.getMeshResourceManager().getDrawIdVertexArrayPtr(), materialResourceManager, materialResourceId, getInvalid<SkeletonResourceId>(), false, mIndirectBufferPtr);
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
	GrassSceneItem::GrassSceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource, false),	// TODO(co) Set bounding box
		mMaximumNumberOfGrass(3)	// TODO(co) Make this dynamic
	{
		{ // Create vertex array object (VAO)
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
			Renderer::IBufferManager& bufferManager = getSceneResource().getRendererRuntime().getBufferManager();

			// Create the structured buffer
			mStructuredBufferPtr = bufferManager.createStructuredBuffer(sizeof(GrassDataStruct) * mMaximumNumberOfGrass, grassData, Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage::STATIC_DRAW, sizeof(GrassDataStruct));
			RENDERER_SET_RESOURCE_DEBUG_NAME(mStructuredBufferPtr, "Grass structured buffer")

			{ // Create the indirect buffer: Twelve vertices per grass (two quads), grass index = instance index
				const Renderer::DrawArguments drawArguments =
				{
					12,						// vertexCountPerInstance (uint32_t)
					mMaximumNumberOfGrass,	// instanceCount (uint32_t)
					0,						// startVertexLocation (uint32_t)
					0						// startInstanceLocation (uint32_t)
				};
				mIndirectBufferPtr = bufferManager.createIndirectBuffer(sizeof(Renderer::DrawArguments), &drawArguments, Renderer::IndirectBufferFlag::UNORDERED_ACCESS | Renderer::IndirectBufferFlag::DRAW_ARGUMENTS);
				RENDERER_SET_RESOURCE_DEBUG_NAME(mIndirectBufferPtr, "Grass indirect buffer")
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
