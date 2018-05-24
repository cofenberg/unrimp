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
#include "RendererRuntime/PrecompiledHeader.h"
#include "RendererRuntime/Resource/Scene/Factory/SceneFactory.h"
#include "RendererRuntime/Resource/Scene/Item/Sky/SkySceneItem.h"
#include "RendererRuntime/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "RendererRuntime/Resource/Scene/Item/Light/SunlightSceneItem.h"
#include "RendererRuntime/Resource/Scene/Item/Terrain/TerrainSceneItem.h"
#include "RendererRuntime/Resource/Scene/Item/Particles/ParticlesSceneItem.h"
#include "RendererRuntime/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ISceneFactory methods ]
	//[-------------------------------------------------------]
	ISceneItem* SceneFactory::createSceneItem(const SceneItemTypeId& sceneItemTypeId, SceneResource& sceneResource) const
	{
		ISceneItem* sceneItem = nullptr;

		// Define helper macro
		#define CASE_VALUE(name) case name::TYPE_ID: sceneItem = new name(sceneResource); break;

		// Evaluate the scene item type, sorted by usual frequency
		switch (sceneItemTypeId)
		{
			CASE_VALUE(MeshSceneItem)
			CASE_VALUE(LightSceneItem)
			CASE_VALUE(SkeletonMeshSceneItem)
			CASE_VALUE(ParticlesSceneItem)
			CASE_VALUE(CameraSceneItem)
			CASE_VALUE(SunlightSceneItem)
			CASE_VALUE(SkySceneItem)
			CASE_VALUE(TerrainSceneItem)
		}

		// Undefine helper macro
		#undef CASE_VALUE

		// Done
		return sceneItem;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
