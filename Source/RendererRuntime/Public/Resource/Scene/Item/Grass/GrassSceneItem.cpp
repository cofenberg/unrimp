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
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		// Vertex input layout
		static constexpr Renderer::VertexAttribute GrassVertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_4,			// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"PositionSize",										// name[32] (char)
				"POSITION",											// semanticName[32] (char)
				0,													// semanticIndex (uint32_t)
				// Data source
				0,													// inputSlot (uint32_t)
				0,													// alignedByteOffset (uint32_t)
				sizeof(RendererRuntime::GrassSceneItem::GrassData),	// strideInBytes (uint32_t)
				1													// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_4,			// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"ColorRotation",									// name[32] (char)
				"COLOR",											// semanticName[32] (char)
				0,													// semanticIndex (uint32_t)
				// Data source
				0,													// inputSlot (uint32_t)
				sizeof(float) * 4,									// alignedByteOffset (uint32_t)
				sizeof(RendererRuntime::GrassSceneItem::GrassData),	// strideInBytes (uint32_t)
				1													// instancesPerElement (uint32_t)
			}
		};


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
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const Renderer::VertexAttributes GrassSceneItem::VERTEX_ATTRIBUTES(static_cast<uint32_t>(glm::countof(::detail::GrassVertexAttributesLayout)), ::detail::GrassVertexAttributesLayout);


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void GrassSceneItem::onAttachedToSceneNode(SceneNode& sceneNode)
	{
		mRenderableManager.setTransform(&sceneNode.getGlobalTransform());

		// Call the base implementation
		ISceneItem::onAttachedToSceneNode(sceneNode);
	}

	const RenderableManager* GrassSceneItem::getRenderableManager() const
	{
		if (!isValid(getMaterialResourceId()))
		{
			// TODO(co) Get rid of the nasty delayed initialization in here, including the evil const-cast. For this, full asynchronous material blueprint loading must work. See "TODO(co) Currently material blueprint resource loading is a blocking process.".
			const_cast<GrassSceneItem*>(this)->initialize();
		}
		return &mRenderableManager;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void GrassSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, mVertexArrayPtr, getSceneResource().getRendererRuntime().getMaterialResourceManager(), getMaterialResourceId(), getInvalid<SkeletonResourceId>(), false, mIndirectBufferPtr);
		mRenderableManager.updateCachedRenderablesData();
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
			const GrassData grassData[3] =
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
			Renderer::IVertexBufferPtr vertexBuffer(bufferManager.createVertexBuffer(sizeof(GrassData) * mMaximumNumberOfGrass, grassData));
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, "Grass VBO")

			// Create vertex array object (VAO)
			// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
			// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
			// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
			//    reference of the used vertex buffer objects (VBO). If the reference counter of a
			//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			mVertexArrayPtr = bufferManager.createVertexArray(VERTEX_ATTRIBUTES, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
			RENDERER_SET_RESOURCE_DEBUG_NAME(mVertexArrayPtr, "Grass VAO")

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
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	GrassSceneItem::~GrassSceneItem()
	{
		if (isValid(getMaterialResourceId()))
		{
			// Clear the renderable manager right now
			mRenderableManager.getRenderables().clear();
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
