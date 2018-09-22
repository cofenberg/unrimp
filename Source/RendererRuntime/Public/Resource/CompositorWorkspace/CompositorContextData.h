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
#include "RendererRuntime/Public/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class LightSceneItem;
	class CameraSceneItem;
	class MaterialBlueprintResource;
	class CompositorWorkspaceInstance;
	class CompositorInstancePassShadowMap;
}


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
	*    Compositor context data used during compositor execution
	*/
	class CompositorContextData final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RenderQueue;	// Needs access to "mCurrentlyBoundMaterialBlueprintResource" and "mGlobalComputeSize"


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline CompositorContextData() :
			mCompositorWorkspaceInstance(nullptr),
			mCameraSceneItem(nullptr),
			mSinglePassStereoInstancing(false),
			mLightSceneItem(nullptr),
			mCompositorInstancePassShadowMap(nullptr),
			mWorldSpaceCameraPosition(0.0f, 0.0f, 0.0f),
			mCurrentlyBoundMaterialBlueprintResource(nullptr),
			mGlobalComputeSize{0, 0, 0}
		{
			// Nothing here
		}

		CompositorContextData(const CompositorWorkspaceInstance* compositorWorkspaceInstance, const CameraSceneItem* cameraSceneItem, bool singlePassStereoInstancing = false, const LightSceneItem* lightSceneItem = nullptr, const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = nullptr);

		inline ~CompositorContextData()
		{
			// Nothing here
		}

		inline const CompositorWorkspaceInstance* getCompositorWorkspaceInstance() const
		{
			return mCompositorWorkspaceInstance;
		}

		inline const CameraSceneItem* getCameraSceneItem() const
		{
			return mCameraSceneItem;
		}

		inline bool getSinglePassStereoInstancing() const
		{
			return mSinglePassStereoInstancing;
		}

		inline const LightSceneItem* getLightSceneItem() const
		{
			return mLightSceneItem;
		}

		inline const CompositorInstancePassShadowMap* getCompositorInstancePassShadowMap() const
		{
			return mCompositorInstancePassShadowMap;
		}

		inline const glm::vec3& getWorldSpaceCameraPosition() const
		{
			return mWorldSpaceCameraPosition;
		}

		inline void resetCurrentlyBoundMaterialBlueprintResource() const
		{
			mCurrentlyBoundMaterialBlueprintResource = nullptr;
		}

		inline MaterialBlueprintResource* getCurrentlyBoundMaterialBlueprintResource() const
		{
			return mCurrentlyBoundMaterialBlueprintResource;
		}

		inline uint32_t* getGlobalComputeSize() const
		{
			return mGlobalComputeSize;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit CompositorContextData(const CompositorContextData&) = delete;
		CompositorContextData& operator=(const CompositorContextData&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const CompositorWorkspaceInstance*	   mCompositorWorkspaceInstance;
		const CameraSceneItem*				   mCameraSceneItem;
		bool								   mSinglePassStereoInstancing;
		const LightSceneItem*				   mLightSceneItem;
		const CompositorInstancePassShadowMap* mCompositorInstancePassShadowMap;
		// Cached data
		glm::vec3 mWorldSpaceCameraPosition;	///< Cached world space camera position since often accessed due to camera relative rendering
		// Cached "RendererRuntime::RenderQueue" data to reduce the number of state changes across different render queue instances (beneficial for complex compositors with e.g. multiple Gaussian blur passes)
		mutable MaterialBlueprintResource* mCurrentlyBoundMaterialBlueprintResource;
		mutable uint32_t				   mGlobalComputeSize[3];


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
