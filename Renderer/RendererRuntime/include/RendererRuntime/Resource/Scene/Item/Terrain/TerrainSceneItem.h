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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Resource/Scene/Item/MaterialSceneItem.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Terrain scene item
	*
	*  @remarks
	*    This software contains source code provided by NVIDIA Corporation. The height map terrain tessellation implementation is basing on "DirectX 11 Terrain Tessellation" by Iain Cantlay
	*    ( https://developer.nvidia.com/sites/default/files/akamai/gamedev/files/sdk/11/TerrainTessellation_WhitePaper.pdf ) and the concrete implementation "TerrainTessellation"-sample inside
	*    "NVIDIA Direct3D SDK 11" ( https://developer.nvidia.com/dx11-samples ).
	*
	*    A terrain tile ring is symmetrical in each direction. Don't read much into the exact numbers of #s in this following diagram:
	*    <-   outerWidth  ->
	*    ###################
	*    ###################
	*    ###             ###
	*    ###<-holeWidth->###
	*    ###             ###
	*    ###    (0,0)    ###
	*    ###             ###
	*    ###             ###
	*    ###             ###
	*    ###################
	*    ###################
	*/
	class TerrainSceneItem final : public MaterialSceneItem
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;	// Needs to be able to create scene item instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("TerrainSceneItem");
		RENDERERRUNTIME_API_EXPORT static const Renderer::VertexAttributes VERTEX_ATTRIBUTES;


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	public:
		inline virtual SceneItemTypeId getSceneItemTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void onAttachedToSceneNode(SceneNode& sceneNode) override;

		inline virtual void onDetachedFromSceneNode(SceneNode& sceneNode) override
		{
			mRenderableManager.setTransform(nullptr);

			// Call the base implementation
			ISceneItem::onDetachedFromSceneNode(sceneNode);
		}

		inline virtual void setVisible(bool visible) override
		{
			mRenderableManager.setVisible(visible);
		}

		virtual const RenderableManager* getRenderableManager() const override;


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onMaterialResourceCreated() override;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		static constexpr int MAXIMUM_NUMBER_OF_TERRAIN_TILE_RINGS = 6;
		struct TerrainTileRing final
		{
			int						  numberOfTiles;
			Renderer::IVertexArrayPtr vertexArrayPtr;	///< Vertex array object (VAO), considered to be always valid
		};


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TerrainSceneItem(SceneResource& sceneResource);
		virtual ~TerrainSceneItem() override;
		explicit TerrainSceneItem(const TerrainSceneItem&) = delete;
		TerrainSceneItem& operator=(const TerrainSceneItem&) = delete;
		void createIndexBuffer(Renderer::IBufferManager& bufferManager);
		void createTerrainTileRing(TerrainTileRing& terrainTileRing, Renderer::IBufferManager& bufferManager, int holeWidth, int outerWidth, float tileSize) const;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		RenderableManager		  mRenderableManager;			///< Renderable manager of all terrain tile rings
		Renderer::IIndexBufferPtr mIndexBufferPtr;				///< Index buffer which is shared between all terrain tile ring vertex array buffers
		int						  mNumberOfTerrainTileRings;	///< Number of terrain tile rings
		TerrainTileRing			  mTerrainTileRings[MAXIMUM_NUMBER_OF_TERRAIN_TILE_RINGS];


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
