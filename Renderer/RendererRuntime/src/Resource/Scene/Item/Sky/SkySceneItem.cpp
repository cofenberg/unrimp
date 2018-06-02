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
#include "RendererRuntime/Resource/Scene/Item/Sky/SkySceneItem.h"
#include "RendererRuntime/Resource/Scene/SceneResource.h"
#include "RendererRuntime/Resource/Scene/SceneNode.h"
#include "RendererRuntime/IRendererRuntime.h"


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
		static Renderer::IVertexArrayPtr VertexArrayPtr;	///< Vertex array object (VAO), can be a null pointer, shared between all sky instances


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		Renderer::IVertexArray* createVertexArray(Renderer::IBufferManager& bufferManager)
		{
			// Vertex input layout
			static constexpr Renderer::VertexAttribute vertexAttributesLayout[] =
			{
				{ // Attribute 0
					// Data destination
					Renderer::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
					"Position",									// name[32] (char)
					"POSITION",									// semanticName[32] (char)
					0,											// semanticIndex (uint32_t)
					// Data source
					0,											// inputSlot (uint32_t)
					0,											// alignedByteOffset (uint32_t)
					sizeof(float) * 3,							// strideInBytes (uint32_t)
					0											// instancesPerElement (uint32_t)
				}
			};
			const Renderer::VertexAttributes vertexAttributes(static_cast<uint32_t>(glm::countof(vertexAttributesLayout)), vertexAttributesLayout);

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
			Renderer::IVertexBufferPtr vertexBuffer(bufferManager.createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, "Sky")

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
			Renderer::IIndexBuffer* indexBuffer = bufferManager.createIndexBuffer(sizeof(INDICES), Renderer::IndexBufferFormat::UNSIGNED_SHORT, INDICES, Renderer::BufferUsage::STATIC_DRAW);
			RENDERER_SET_RESOURCE_DEBUG_NAME(indexBuffer, "Sky")

			// Create vertex array object (VAO)
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			Renderer::IVertexArray* vertexArray = bufferManager.createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBuffer);
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexArray, "Sky")

			// Done
			return vertexArray;
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
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void SkySceneItem::onAttachedToSceneNode(SceneNode& sceneNode)
	{
		mRenderableManager.setTransform(&sceneNode.getGlobalTransform());

		// Call the base implementation
		ISceneItem::onAttachedToSceneNode(sceneNode);
	}

	const RenderableManager* SkySceneItem::getRenderableManager() const
	{
		if (!isInitialized(getMaterialResourceId()))
		{
			// TODO(co) Get rid of the nasty delayed initialization in here, including the evil const-cast. For this, full asynchronous material blueprint loading must work. See "TODO(co) Currently material blueprint resource loading is a blocking process.".
			const_cast<SkySceneItem*>(this)->initialize();
		}
		return &mRenderableManager;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void SkySceneItem::onMaterialResourceCreated()
	{
		const IRendererRuntime& rendererRuntime = getSceneResource().getRendererRuntime();

		// Add reference to vertex array object (VAO) shared between all sky instances
		if (nullptr == ::detail::VertexArrayPtr)
		{
			::detail::VertexArrayPtr = ::detail::createVertexArray(rendererRuntime.getBufferManager());
			assert(nullptr != ::detail::VertexArrayPtr);
		}
		::detail::VertexArrayPtr->addReference();

		// Setup renderable manager
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, ::detail::VertexArrayPtr, true, 0, 36, rendererRuntime.getMaterialResourceManager(), getMaterialResourceId(), getUninitialized<SkeletonResourceId>());
		mRenderableManager.updateCachedRenderablesData();
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	SkySceneItem::~SkySceneItem()
	{
		if (isInitialized(getMaterialResourceId()))
		{
			// Clear the renderable manager right now so we have no more references to the shared vertex array
			mRenderableManager.getRenderables().clear();

			// Release reference to vertex array object (VAO) shared between all sky instances
			if (nullptr != ::detail::VertexArrayPtr && 1 == ::detail::VertexArrayPtr->releaseReference())	// +1 for reference to global shared pointer
			{
				::detail::VertexArrayPtr = nullptr;
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
