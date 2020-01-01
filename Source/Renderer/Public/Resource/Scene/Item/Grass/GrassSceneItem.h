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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/Scene/Item/MaterialSceneItem.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Grass scene item
	*
	*  @todo
	*    - TODO(co) Work-in-progress
	*/
	class GrassSceneItem final : public MaterialSceneItem
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;	// Needs to be able to create scene item instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("GrassSceneItem");
		struct GrassDataStruct
		{
			float PositionSize[4];	// Object space grass xyz-position, w = grass size
			float ColorRotation[4];	// Linear RGB grass color and rotation in radians
		};


	//[-------------------------------------------------------]
	//[ Public Renderer::ISceneItem methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual SceneItemTypeId getSceneItemTypeId() const override
		{
			return TYPE_ID;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::MaterialSceneItem methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void initialize() override;
		virtual void onMaterialResourceCreated() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GrassSceneItem(SceneResource& sceneResource);

		inline virtual ~GrassSceneItem() override
		{
			// Nothing here
		}

		explicit GrassSceneItem(const GrassSceneItem&) = delete;
		GrassSceneItem& operator=(const GrassSceneItem&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t				  mMaximumNumberOfGrass;	///< Maximum number of grass
		Rhi::IStructuredBufferPtr mStructuredBufferPtr;		///< Structured buffer the data of the individual grass ("Renderer::GrassSceneItem::GrassDataStruct")
		Rhi::IIndirectBufferPtr   mIndirectBufferPtr;		///< Indirect buffer holding data related to the current grass "Rhi::DrawArguments" draw call


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
