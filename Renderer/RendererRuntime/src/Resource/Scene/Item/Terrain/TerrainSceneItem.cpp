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
#include "RendererRuntime/Resource/Scene/Item/Terrain/TerrainSceneItem.h"
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
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		const int VERTICES_PER_TILE_EDGE = 9;	///< Overlap => -2
		const int NUMBER_OF_INDICES = (VERTICES_PER_TILE_EDGE - 1) * (VERTICES_PER_TILE_EDGE - 1) * 4;

		/**
		*  @brief
		*    These are the size of the neighbours along +/- x or y axes. For interior tiles this is 1. For edge tiles it is 0.5 or 2.0.
		*/
		struct Adjacency final
		{
			float neighbourMinusX;
			float neighbourMinusY;
			float neighbourPlusX;
			float neighbourPlusY;
		};

		struct InstanceData final
		{
			float	  x, y;
			float	  tileSize;
			Adjacency adjacency;
		};


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		// Vertex input layout
		// TODO(co) Optimization: We could probably reduce stuff to 16-bit instead of 32-bit to save a little bit of memory, might not really be worth it
		static constexpr Renderer::VertexAttribute TerrainVertexAttributesLayout[] =
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
				sizeof(float) * 7,							// strideInBytes (uint32_t)
				1											// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_4,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Adjacency",								// name[32] (char)
				"TEXCOORD",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 3,							// alignedByteOffset (uint32_t)
				sizeof(float) * 7,							// strideInBytes (uint32_t)
				1											// instancesPerElement (uint32_t)
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
	const Renderer::VertexAttributes TerrainSceneItem::VERTEX_ATTRIBUTES(static_cast<uint32_t>(glm::countof(::detail::TerrainVertexAttributesLayout)), ::detail::TerrainVertexAttributesLayout);


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void TerrainSceneItem::onAttachedToSceneNode(SceneNode& sceneNode)
	{
		mRenderableManager.setTransform(&sceneNode.getGlobalTransform());

		// Call the base implementation
		ISceneItem::onAttachedToSceneNode(sceneNode);
	}

	const RenderableManager* TerrainSceneItem::getRenderableManager() const
	{
		if (!isValid(getMaterialResourceId()))
		{
			// TODO(co) Get rid of the nasty delayed initialization in here, including the evil const-cast. For this, full asynchronous material blueprint loading must work. See "TODO(co) Currently material blueprint resource loading is a blocking process.".
			const_cast<TerrainSceneItem*>(this)->initialize();
		}
		return &mRenderableManager;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	void TerrainSceneItem::onMaterialResourceCreated()
	{
		// Setup renderable manager: Instancing is used
		// -> One tiles is one instance and the index buffer describes all the NxN patches within one tile
		const IRendererRuntime& rendererRuntime = getSceneResource().getRendererRuntime();
		// TODO(co) The terrain scene item isn't supported by the OpenGL ES 3 renderer, yet
		if (rendererRuntime.getRenderer().getNameId() != Renderer::NameId::OPENGLES3)
		{
			RenderableManager::Renderables& renderables = mRenderableManager.getRenderables();
			renderables.reserve(static_cast<size_t>(mNumberOfTerrainTileRings));
			for (int i = 0; i != mNumberOfTerrainTileRings; ++i)
			{
				const TerrainTileRing& terrainTileRing = mTerrainTileRings[i];
				renderables.emplace_back(mRenderableManager, terrainTileRing.vertexArrayPtr, true, 0, ::detail::NUMBER_OF_INDICES, rendererRuntime.getMaterialResourceManager(), getMaterialResourceId(), getInvalid<SkeletonResourceId>(), terrainTileRing.numberOfTiles);
			}
			mRenderableManager.updateCachedRenderablesData();
		}
		else
		{
			RENDERER_LOG(rendererRuntime.getContext(), COMPATIBILITY_WARNING, "The renderer runtime terrain scene item isn't supported by the OpenGL ES 3 renderer, yet")
		}
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	TerrainSceneItem::TerrainSceneItem(SceneResource& sceneResource) :
		MaterialSceneItem(sceneResource, false),	///< The GPU will automatically cull terrain patches	TODO(co) Later on there might be multiple smaller terrains which then would make terrain frustum culling useful, we'll take care of this as soon as it's really needed
		mNumberOfTerrainTileRings(0),
		mTerrainTileRings{}
	{
		// This array defines the outer width of each successive ring
		const int widths[] = { 0, 16, 16, 16, 16, 16 };
		mNumberOfTerrainTileRings = sizeof(widths) / sizeof(widths[0]) - 1;	// "widths[0]" doesn't define a ring hence -1
		assert(mNumberOfTerrainTileRings <= MAXIMUM_NUMBER_OF_TERRAIN_TILE_RINGS);
		Renderer::IBufferManager& bufferManager = getSceneResource().getRendererRuntime().getBufferManager();
		createIndexBuffer(bufferManager);
		float tileWidth = 0.125f;
		for (int i = 0; i != mNumberOfTerrainTileRings && i != MAXIMUM_NUMBER_OF_TERRAIN_TILE_RINGS; ++i)
		{
			createTerrainTileRing(mTerrainTileRings[i], bufferManager, widths[i] / 2, widths[i + 1], tileWidth);
			tileWidth *= 2.0f;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	TerrainSceneItem::~TerrainSceneItem()
	{
		if (isValid(getMaterialResourceId()))
		{
			// Clear the renderable manager right now so we have no more references to the shared vertex array
			mRenderableManager.getRenderables().clear();
		}
	}

	void TerrainSceneItem::createIndexBuffer(Renderer::IBufferManager& bufferManager)
	{
		// The index buffer describes one tile of NxN patches: Four vertices per quad, with "::detail::VERTICES_PER_TILE_EDGE" - 1 quads per tile edge
		int index = 0;
		uint16_t indices[::detail::NUMBER_OF_INDICES];
		for (int y = 0; y < ::detail::VERTICES_PER_TILE_EDGE - 1; ++y)
		{
			const int rowStart = y * ::detail::VERTICES_PER_TILE_EDGE;
			for (int x = 0; x < ::detail::VERTICES_PER_TILE_EDGE - 1; ++x)
			{
				indices[index++] = static_cast<uint16_t>(rowStart + x);
				indices[index++] = static_cast<uint16_t>(rowStart + x + ::detail::VERTICES_PER_TILE_EDGE);
				indices[index++] = static_cast<uint16_t>(rowStart + x + ::detail::VERTICES_PER_TILE_EDGE + 1);
				indices[index++] = static_cast<uint16_t>(rowStart + x + 1);
			}
		}
		assert(::detail::NUMBER_OF_INDICES == index);

		// Create the index buffer object (IBO)
		mIndexBufferPtr = bufferManager.createIndexBuffer(sizeof(uint16_t) * ::detail::NUMBER_OF_INDICES, Renderer::IndexBufferFormat::UNSIGNED_SHORT, indices, Renderer::BufferUsage::STATIC_DRAW);
		RENDERER_SET_RESOURCE_DEBUG_NAME(mIndexBufferPtr, "Terrain tile ring")
	}

	void TerrainSceneItem::createTerrainTileRing(TerrainTileRing& terrainTileRing, Renderer::IBufferManager& bufferManager, int holeWidth, int outerWidth, float tileSize) const
	{
		// Sanity checks
		assert((nullptr != mIndexBufferPtr) && "The index buffer must be created before this method is called");
		assert((outerWidth - holeWidth) % 2 == 0);

		// Derive data
		const int ringWidth = (outerWidth - holeWidth) / 2;	// No remainder - see assert above
		const int numberOfTiles = terrainTileRing.numberOfTiles = outerWidth * outerWidth - holeWidth * holeWidth;

		// Create the vertex buffer object (VBO)
		Renderer::IVertexBufferPtr vertexBuffer;
		{
			// Create the vertex buffer data
			int index = 0;
			::detail::InstanceData* vertexBufferData = new ::detail::InstanceData[static_cast<uint32_t>(numberOfTiles)];
			const float halfWidth = 0.5f * static_cast<float>(outerWidth);
			for (int y = 0; y < outerWidth; ++y)
			{
				for (int x = 0; x < outerWidth; ++x)
				{
					// Is in ring?
					assert(x >= 0 && x < outerWidth);
					assert(y >= 0 && y < outerWidth);
					if (x < ringWidth || y < ringWidth || x >= outerWidth - ringWidth || y >= outerWidth - ringWidth)
					{
						::detail::InstanceData& instanceData = vertexBufferData[index];
						instanceData.x = tileSize * (static_cast<float>(x) - halfWidth);
						instanceData.y = tileSize * (static_cast<float>(y) - halfWidth);
						instanceData.tileSize = tileSize;

						{ // Assign neighbour sizes
							::detail::Adjacency& adjacency = instanceData.adjacency;
							adjacency.neighbourPlusX  = 1.0f;
							adjacency.neighbourPlusY  = 1.0f;
							adjacency.neighbourMinusX = 1.0f;
							adjacency.neighbourMinusY = 1.0f;

							// TODO(co) These aren't necessarily 2x different, depends on the supplied relative tiles sizes
							const float innerNeighbourSize = 0.5f;
							const float outerNeighbourSize = 2.0f;

							// Inner edges abut tiles that are smaller (but not on the inner-most)
							if (holeWidth > 0)
							{
								if (y >= ringWidth && y < (outerWidth - ringWidth))
								{
									if (ringWidth - 1 == x)
									{
										adjacency.neighbourPlusX = innerNeighbourSize;
									}
									else if (outerWidth - ringWidth == x)
									{
										adjacency.neighbourMinusX = innerNeighbourSize;
									}
								}
								if (x >= ringWidth && x < (outerWidth - ringWidth))
								{
									if (ringWidth - 1 == y)
									{
										adjacency.neighbourPlusY = innerNeighbourSize;
									}
									else if (outerWidth - ringWidth == y)
									{
										adjacency.neighbourMinusY = innerNeighbourSize;
									}
								}
							}

							// Outer edges abut tiles that are larger. We could skip this on the outer-most ring. But it will make almost zero visual or performance difference.
							if (0 == x)
							{
								adjacency.neighbourMinusX = outerNeighbourSize;
							}
							if (0 == y)
							{
								adjacency.neighbourMinusY = outerNeighbourSize;
							}
							if (outerWidth - 1 == x)
							{
								adjacency.neighbourPlusX  = outerNeighbourSize;
							}
							if (outerWidth - 1 == y)
							{
								adjacency.neighbourPlusY  = outerNeighbourSize;
							}
						}

						// Next, please
						++index;
					}
				}
			}
			assert(index == numberOfTiles);

			// Create the vertex buffer object (VBO)
			vertexBuffer = bufferManager.createVertexBuffer(sizeof(::detail::InstanceData) * numberOfTiles, vertexBufferData, Renderer::BufferUsage::STATIC_DRAW);
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, "Terrain tile ring")

			// Destroy temporary vertex buffer data
			delete[] vertexBufferData;
		}

		// Create vertex array object (VAO)
		const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
		terrainTileRing.vertexArrayPtr = bufferManager.createVertexArray(TerrainSceneItem::VERTEX_ATTRIBUTES, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, mIndexBufferPtr);
		RENDERER_SET_RESOURCE_DEBUG_NAME(terrainTileRing.vertexArrayPtr, "Terrain tile ring")
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
