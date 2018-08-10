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
		static constexpr Renderer::VertexAttribute ParticlesVertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_4,					// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"PositionSize",												// name[32] (char)
				"POSITION",													// semanticName[32] (char)
				0,															// semanticIndex (uint32_t)
				// Data source
				0,															// inputSlot (uint32_t)
				0,															// alignedByteOffset (uint32_t)
				sizeof(RendererRuntime::ParticlesSceneItem::ParticleData),	// strideInBytes (uint32_t)
				1															// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_4,					// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Color",													// name[32] (char)
				"COLOR",													// semanticName[32] (char)
				0,															// semanticIndex (uint32_t)
				// Data source
				0,															// inputSlot (uint32_t)
				sizeof(float) * 4,											// alignedByteOffset (uint32_t)
				sizeof(RendererRuntime::ParticlesSceneItem::ParticleData),	// strideInBytes (uint32_t)
				1															// instancesPerElement (uint32_t)
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
	const Renderer::VertexAttributes ParticlesSceneItem::VERTEX_ATTRIBUTES(static_cast<uint32_t>(glm::countof(::detail::ParticlesVertexAttributesLayout)), ::detail::ParticlesVertexAttributesLayout);


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void ParticlesSceneItem::onAttachedToSceneNode(SceneNode& sceneNode)
	{
		mRenderableManager.setTransform(&sceneNode.getGlobalTransform());

		// Call the base implementation
		ISceneItem::onAttachedToSceneNode(sceneNode);
	}

	const RenderableManager* ParticlesSceneItem::getRenderableManager() const
	{
		if (!isValid(getMaterialResourceId()))
		{
			// TODO(co) Get rid of the nasty delayed initialization in here, including the evil const-cast. For this, full asynchronous material blueprint loading must work. See "TODO(co) Currently material blueprint resource loading is a blocking process.".
			const_cast<ParticlesSceneItem*>(this)->initialize();
		}
		return &mRenderableManager;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void ParticlesSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager: Six vertices per particle, particle index = instance index
		mRenderableManager.getRenderables().emplace_back(mRenderableManager, mVertexArrayPtr, getSceneResource().getRendererRuntime().getMaterialResourceManager(), getMaterialResourceId(), getInvalid<SkeletonResourceId>(), false, 0, 6, mMaximumNumberOfParticles);
		mRenderableManager.updateCachedRenderablesData();
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
			const ParticleData particlesData[8] =
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
			Renderer::IBufferManager& bufferManager = getSceneResource().getRendererRuntime().getBufferManager();
			Renderer::IVertexBufferPtr vertexBuffer(bufferManager.createVertexBuffer(sizeof(ParticleData) * mMaximumNumberOfParticles, particlesData));
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, "Particles VBO")

			// Create vertex array object (VAO)
			// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
			// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
			// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
			//    reference of the used vertex buffer objects (VBO). If the reference counter of a
			//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			mVertexArrayPtr = bufferManager.createVertexArray(VERTEX_ATTRIBUTES, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
			RENDERER_SET_RESOURCE_DEBUG_NAME(mVertexArrayPtr, "Particles VAO")
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	ParticlesSceneItem::~ParticlesSceneItem()
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
