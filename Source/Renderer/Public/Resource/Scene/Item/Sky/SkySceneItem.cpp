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
#include "Renderer/Public/Resource/Scene/Item/Sky/SkySceneItem.h"
#include "Renderer/Public/Resource/Scene/SceneResource.h"
#include "Renderer/Public/Resource/Scene/SceneNode.h"
#include "Renderer/Public/IRenderer.h"


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		static Rhi::IVertexArrayPtr SkyVertexArrayPtr;	///< Vertex array object (VAO), can be a null pointer, shared between all sky instances


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] Rhi::IVertexArray* createSkyVertexArray(Rhi::IBufferManager& bufferManager)
		{
			// Vertex input layout
			static constexpr Rhi::VertexAttribute vertexAttributesLayout[] =
			{
				{ // Attribute 0
					// Data destination
					Rhi::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
					"Position",								// name[32] (char)
					"POSITION",								// semanticName[32] (char)
					0,										// semanticIndex (uint32_t)
					// Data source
					0,										// inputSlot (uint32_t)
					0,										// alignedByteOffset (uint32_t)
					sizeof(float) * 3,						// strideInBytes (uint32_t)
					0										// instancesPerElement (uint32_t)
				}
			};
			const Rhi::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayout)), vertexAttributesLayout);

			// Create the vertex buffer object (VBO)
			// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
			static constexpr float VERTEX_POSITION[] =
			{
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,- 1.0f
			};
			Rhi::IVertexBufferPtr vertexBuffer(bufferManager.createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, 0, Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME("Sky")));

			// Create the index buffer object (IBO)
			static constexpr uint16_t INDICES[] =
			{
				0, 1, 2, 2, 3, 0,	// Front
				1, 4, 7, 7, 2, 1,	// Right
				4, 5, 6, 6, 7, 4,	// Back
				5, 0, 3, 3, 6, 5,	// Left
				5, 4, 1, 1, 0, 5,	// Top
				3, 2, 7, 7, 6, 3	// Bottom
			};
			Rhi::IIndexBuffer* indexBuffer = bufferManager.createIndexBuffer(sizeof(INDICES), INDICES, 0, Rhi::BufferUsage::STATIC_DRAW, Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME("Sky"));

			// Create vertex array object (VAO)
			const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			return bufferManager.createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBuffer RHI_RESOURCE_DEBUG_NAME("Sky"));
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void SkySceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager
		#ifdef RHI_DEBUG
			const char* debugName = "Sky";
			mRenderableManager.setDebugName(debugName);
		#endif
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, ::detail::SkyVertexArrayPtr, getSceneResource().getRenderer().getMaterialResourceManager(), getMaterialResourceId(), getInvalid<SkeletonResourceId>(), true, 0, 36, 1 RHI_RESOURCE_DEBUG_NAME(debugName));
		mRenderableManager.updateCachedRenderablesData();
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	SkySceneItem::SkySceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource, false)	///< The sky isn't allowed to be culled
	{
		// Add reference to vertex array object (VAO) shared between all sky instances
		if (nullptr == ::detail::SkyVertexArrayPtr)
		{
			::detail::SkyVertexArrayPtr = ::detail::createSkyVertexArray(getSceneResource().getRenderer().getBufferManager());
			RHI_ASSERT(getContext(), nullptr != ::detail::SkyVertexArrayPtr, "Invalid sky vertex array")
		}
		::detail::SkyVertexArrayPtr->addReference();
	}

	SkySceneItem::~SkySceneItem()
	{
		if (isValid(getMaterialResourceId()))
		{
			// Clear the renderable manager right now so we have no more references to the shared vertex array
			mRenderableManager.getRenderables().clear();
		}

		// Release reference to vertex array object (VAO) shared between all sky instances
		if (nullptr != ::detail::SkyVertexArrayPtr && 1 == ::detail::SkyVertexArrayPtr->releaseReference())	// +1 for reference to global shared pointer
		{
			::detail::SkyVertexArrayPtr = nullptr;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
