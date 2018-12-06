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
#include "RendererRuntime/Public/Resource/Scene/Item/Light/LightSceneItem.h"
#include "RendererRuntime/Public/Resource/Scene/Loader/SceneFileFormat.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtc/type_ptr.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void LightSceneItem::deserialize([[maybe_unused]] uint32_t numberOfBytes, const uint8_t* data)
	{
		RENDERER_ASSERT(getContext(), sizeof(v1Scene::LightItem) == numberOfBytes, "Invalid number of bytes")

		// Read data
		const v1Scene::LightItem* lightItem = reinterpret_cast<const v1Scene::LightItem*>(data);
		mPackedShaderData.lightType = static_cast<float>(lightItem->lightType);
		memcpy(glm::value_ptr(mPackedShaderData.color), lightItem->color, sizeof(float) * 3);
		mPackedShaderData.radius = lightItem->radius;
		setInnerOuterAngle(lightItem->innerAngle, lightItem->outerAngle);
		mPackedShaderData.nearClipDistance = lightItem->nearClipDistance;

		// Sanity checks
		RENDERER_ASSERT(getContext(), mPackedShaderData.color.x >= 0.0f && mPackedShaderData.color.y >= 0.0f && mPackedShaderData.color.z >= 0.0f, "Invalid data")
		RENDERER_ASSERT(getContext(), lightItem->lightType == LightType::DIRECTIONAL || mPackedShaderData.radius > 0.0f, "Invalid data")
		RENDERER_ASSERT(getContext(), lightItem->lightType != LightType::DIRECTIONAL || 0.0f == mPackedShaderData.radius, "Invalid data")
		RENDERER_ASSERT(getContext(), mInnerAngle >= 0.0f, "Invalid data")
		RENDERER_ASSERT(getContext(), mOuterAngle < glm::radians(90.0f), "Invalid data")
		RENDERER_ASSERT(getContext(), mInnerAngle < mOuterAngle, "Invalid data")
		RENDERER_ASSERT(getContext(), mPackedShaderData.nearClipDistance >= 0.0f, "Invalid data")
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
