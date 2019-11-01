/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "Renderer/Public/Core/Manager.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class IRenderTarget;
}
namespace Renderer
{
	class ISceneItem;
	struct SceneItemSet;
	class CompositorContextData;
}


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
	*    Scene culling manager
	*
	*  @note
	*    - The implementation is basing on "The Implementation of Frustum Culling in Stingray" - http://bitsquid.blogspot.de/2016/10/the-implementation-of-frustum-culling.html
	*/
	class SceneCullingManager final : public Manager
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<ISceneItem*> SceneItems;	// TODO(co) No raw-pointers (but no smart pointers either, use handles)


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		SceneCullingManager();
		~SceneCullingManager();
		void gatherRenderQueueIndexRangesRenderableManagers(const Rhi::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, CompositorWorkspaceInstance::RenderQueueIndexRanges& renderQueueIndexRanges);

		[[nodiscard]] inline SceneItemSet& getCullableSceneItemSet() const
		{
			// We know that this pointer is always valid
			ASSERT(nullptr != mCullableSceneItemSet);
			return *mCullableSceneItemSet;
		}

		[[nodiscard]] inline SceneItems& getUncullableSceneItems()
		{
			return mUncullableSceneItems;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SceneCullingManager(const SceneCullingManager&) = delete;
		SceneCullingManager& operator=(const SceneCullingManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		SceneItemSet*		  mCullableSceneItemSet;				///< Cullable scene item set, always valid, destroy the instance if you no longer need it
		SceneItemSet*		  mCullableShadowCastersSceneItemSet;	///< Cullable shadow casters scene item set, always valid, destroy the instance if you no longer need it	TODO(co) Implement me
		SceneItems			  mUncullableSceneItems;				///< Scene items which can't be culled and hence are always considered to be visible
		std::vector<uint32_t> mIndirection;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
