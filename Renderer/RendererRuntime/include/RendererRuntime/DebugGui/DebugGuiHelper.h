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
#include "RendererRuntime/Export.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class Transform;
	class CameraSceneItem;
	class SkeletonMeshSceneItem;
	class CompositorWorkspaceInstance;
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
	*    Debug GUI helper generating ImGui ( https://github.com/ocornut/imgui ) commands
	*/
	class DebugGuiHelper final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class DebugGuiManager;	// Calls "RendererRuntime::DebugGuiHelper::beginFrame()"


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		enum class GizmoOperation
		{
			TRANSLATE,
			ROTATE,
			SCALE
		};
		enum class GizmoMode
		{
			LOCAL,
			WORLD
		};
		struct GizmoSettings final
		{
			GizmoOperation	currentGizmoOperation = GizmoOperation::TRANSLATE;
			GizmoMode		currentGizmoMode	  = GizmoMode::WORLD;
			bool			useSnap				  = false;
			float			snap[3]					{1.0f, 1.0f, 1.0f};
		};


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		RENDERERRUNTIME_API_EXPORT static void drawText(const char* text, float x, float y, bool drawBackground = true);
		RENDERERRUNTIME_API_EXPORT static void drawGizmo(const CameraSceneItem& cameraSceneItem, GizmoSettings& gizmoSettings, Transform& transform);	// Using "ImGuizmo" ( https://github.com/CedricGuillemet/ImGuizmo )
		RENDERERRUNTIME_API_EXPORT static void drawSkeleton(const CameraSceneItem& cameraSceneItem, const SkeletonMeshSceneItem& skeletonMeshSceneItem);
		RENDERERRUNTIME_API_EXPORT static void drawGrid(const CameraSceneItem& cameraSceneItem, float cellSize = 1.0f, float yPosition = 0.0f);


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	private:
		static void beginFrame();
		static void drawMetricsWindow(bool& open, CompositorWorkspaceInstance* compositorWorkspaceInstance);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		DebugGuiHelper() = delete;
		~DebugGuiHelper() = delete;
		explicit DebugGuiHelper(const DebugGuiHelper&) = delete;
		DebugGuiHelper& operator=(const DebugGuiHelper&) = delete;


	//[-------------------------------------------------------]
	//[ Private static data                                   ]
	//[-------------------------------------------------------]
	private:
		static uint32_t mDrawTextCounter;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
