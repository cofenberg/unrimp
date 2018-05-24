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
	*    Particles scene item
	*
	*  @todo
	*    - TODO(co) Particles rendering part is implemented, implement GPU based particles simulation as well
	*/
	class ParticlesSceneItem final : public MaterialSceneItem
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;	// Needs to be able to create scene item instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("ParticlesSceneItem");


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
		// TODO(co) Particles structure


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ParticlesSceneItem(SceneResource& sceneResource);
		virtual ~ParticlesSceneItem() override;
		explicit ParticlesSceneItem(const ParticlesSceneItem&) = delete;
		ParticlesSceneItem& operator=(const ParticlesSceneItem&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		RenderableManager mRenderableManager;			///< Renderable manager
		uint32_t		  mMaximumNumberOfParticles;	///< Maximum number of particles


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
