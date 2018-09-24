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
#include "RendererRuntime/Public/Resource/Scene/Item/Volume/VolumeSceneItem.h"
#include "RendererRuntime/Public/Resource/Scene/SceneResource.h"
#include "RendererRuntime/Public/Resource/Scene/SceneNode.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


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
		static Renderer::IVertexArrayPtr VolumeVertexArrayPtr;	///< Vertex array object (VAO), can be a null pointer, shared between all volume instances


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		Renderer::IVertexArray* createVolumeVertexArray(const RendererRuntime::IRendererRuntime& rendererRuntime)
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
				},
				{ // Attribute 1, see "17/11/2012 Surviving without gl_DrawID" - https://www.g-truc.net/post-0518.html
					// Data destination
					Renderer::VertexAttributeFormat::UINT_1,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
					"drawId",									// name[32] (char)
					"DRAWID",									// semanticName[32] (char)
					0,											// semanticIndex (uint32_t)
					// Data source
					1,											// inputSlot (uint32_t)
					0,											// alignedByteOffset (uint32_t)
					sizeof(uint32_t),							// strideInBytes (uint32_t)
					1											// instancesPerElement (uint32_t)
				}
			};
			const Renderer::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayout)), vertexAttributesLayout);

			// Our cube is constructed like this
			/*
					3+------+2  y
					/|     /|   |
				   / |    / |   |
				  / 0+---/--+1  *---x
				7+------+6 /   /
				 | /    | /   z
				 |/     |/
				4+------+5
			*/

			// Create the vertex buffer object (VBO)
			// -> Object space vertex positions
			static constexpr float VERTEX_POSITION[] =
			{
				-0.5f, -0.5f, -0.5f,	// 0
				 0.5f, -0.5f, -0.5f,	// 1
				 0.5f,  0.5f, -0.5f,	// 2
				-0.5f,  0.5f, -0.5f,	// 3
				-0.5f, -0.5f,  0.5f,	// 4
				 0.5f, -0.5f,  0.5f,	// 5
				 0.5f,  0.5f,  0.5f,	// 6
				-0.5f,  0.5f,  0.5f,	// 7
			};
			Renderer::IBufferManager& bufferManager = rendererRuntime.getBufferManager();
			Renderer::IVertexBufferPtr vertexBuffer(bufferManager.createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, "Volume")

			// Create the index buffer object (IBO)
			static constexpr uint16_t INDICES[] =
			{
				// Back		Triangle
				2, 3, 0,	// 0
				0, 1, 2,	// 1
				// Front
				7, 6, 5,	// 0
				5, 4, 7,	// 1
				// Left
				3, 7, 4,	// 0
				4, 0, 3,	// 1
				// Right
				6, 2, 1,	// 0
				1, 5, 6,	// 1
				// Top
				3, 2, 6,	// 0
				6, 7, 3,	// 1
				// Bottom
				0, 4, 5,	// 0
				5, 1, 0		// 1
			};
			Renderer::IIndexBuffer* indexBuffer = bufferManager.createIndexBuffer(sizeof(INDICES), INDICES);
			RENDERER_SET_RESOURCE_DEBUG_NAME(indexBuffer, "Volume")

			// Create vertex array object (VAO)
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer, rendererRuntime.getMeshResourceManager().getDrawIdVertexBufferPtr() };
			Renderer::IVertexArray* vertexArray = bufferManager.createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBuffer);
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexArray, "Volume")

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
	void VolumeSceneItem::onAttachedToSceneNode(SceneNode& sceneNode)
	{
		mRenderableManager.setTransform(&sceneNode.getGlobalTransform());

		// Call the base implementation
		ISceneItem::onAttachedToSceneNode(sceneNode);
	}

	const RenderableManager* VolumeSceneItem::getRenderableManager() const
	{
		// Sanity check: Only uniform scale is supported to keep things simple
		assert(mRenderableManager.getTransform().scale.x == mRenderableManager.getTransform().scale.y && mRenderableManager.getTransform().scale.y == mRenderableManager.getTransform().scale.z);

		// Initialize, if necessary
		if (!isValid(getMaterialResourceId()))
		{
			// TODO(co) Get rid of the nasty delayed initialization in here, including the evil const-cast. For this, full asynchronous material blueprint loading must work. See "TODO(co) Currently material blueprint resource loading is a blocking process.".
			const_cast<VolumeSceneItem*>(this)->initialize();
		}

		// Done
		return &mRenderableManager;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void VolumeSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, ::detail::VolumeVertexArrayPtr, getSceneResource().getRendererRuntime().getMaterialResourceManager(), getMaterialResourceId(), getInvalid<SkeletonResourceId>(), true, 0, 36);
		mRenderableManager.updateCachedRenderablesData();
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	void VolumeSceneItem::onLoadingStateChange(const IResource& resource)
	{
		assert(resource.getAssetId() == getMaterialAssetId());
		if (resource.getLoadingState() == IResource::LoadingState::LOADED)
		{
			mRenderableManager.getRenderables().clear();
		}

		// Call the base implementation
		MaterialSceneItem::onLoadingStateChange(resource);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	VolumeSceneItem::VolumeSceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource)
	{
		// Add reference to vertex array object (VAO) shared between all volume instances
		if (nullptr == ::detail::VolumeVertexArrayPtr)
		{
			::detail::VolumeVertexArrayPtr = ::detail::createVolumeVertexArray(getSceneResource().getRendererRuntime());
			assert(nullptr != ::detail::VolumeVertexArrayPtr);
		}
		::detail::VolumeVertexArrayPtr->addReference();
	}

	VolumeSceneItem::~VolumeSceneItem()
	{
		if (isValid(getMaterialResourceId()))
		{
			// Clear the renderable manager right now so we have no more references to the shared vertex array
			mRenderableManager.getRenderables().clear();
		}

		// Release reference to vertex array object (VAO) shared between all volume instances
		if (nullptr != ::detail::VolumeVertexArrayPtr && 1 == ::detail::VolumeVertexArrayPtr->releaseReference())	// +1 for reference to global shared pointer
		{
			::detail::VolumeVertexArrayPtr = nullptr;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
