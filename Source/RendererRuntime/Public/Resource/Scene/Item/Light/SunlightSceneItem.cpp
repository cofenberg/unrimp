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
#include "RendererRuntime/Public/Resource/Scene/Item/Light/SunlightSceneItem.h"
#include "RendererRuntime/Public/Resource/Scene/Loader/SceneFileFormat.h"
#include "RendererRuntime/Public/Resource/Scene/SceneNode.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	void SunlightSceneItem::calculatedDerivedSunlightProperties()
	{
		SceneNode* parentSceneNode = getParentSceneNode();
		if (nullptr != parentSceneNode)
		{
			// Calculate normalized world space sun direction vector
			// -> Basing on https://raw.githubusercontent.com/aoighost/SkyX/master/SkyX/Source/BasicController.cpp - "SkyX::BasicController::update()"
			// -> TODO(co) Review "Simulating a day’s sky" - "Calculating solar position" - https://nicoschertler.wordpress.com/2013/04/03/simulating-a-days-sky/
			// -> 24h day: 0______A(Sunrise)_______B(Sunset)______24
			float y = 0.0f;
			const float X = mTimeOfDay;
			const float A = mSunriseTime;
			const float B = mSunsetTime;
			const float AB  = A + 24.0f - B;
			const float AB_ = B - A;
			const float XB  = X + 24.0f - B;
			if (X < A || X > B)
			{
				if (X < A)
				{
					y = -XB / AB;
				}
				else
				{
					y = -(X - B) / AB;
				}
				if (y > -0.5f)
				{
					y *= 2.0f;
				}
				else
				{
					y = -(1.0f + y) * 2.0f;
				}
			}
			else
			{
				y = (X - A) / (B - A);
				if (y < 0.5f)
				{
					y *= 2.0f;
				}
				else
				{
					y = (1.0f - y) * 2.0f;
				}
			}

			// Get the east direction vector, clockwise orientation starting from north for zero
			glm::vec2 eastDirection = glm::vec2(-std::sin(mEastDirection), std::cos(mEastDirection));
			if (X > A && X < B)
			{
				if (X > (A + AB_ * 0.5f))
				{
					eastDirection = -eastDirection;
				}
			}
			else if (X <= A)
			{
				if (XB < (24.0f - AB_) * 0.5f)
				{
					eastDirection = -eastDirection;
				}
			}
			else if ((X - B) < (24.0f - AB_) * 0.5f)
			{
				eastDirection = -eastDirection;
			}

			// Calculate the sun direction vector
			const float ydeg = glm::pi<float>() * 0.5f * y;
			const float sn = std::sin(ydeg);
			const float cs = std::cos(ydeg);
			glm::vec3 sunDirection = glm::vec3(eastDirection.x * cs, sn, eastDirection.y * cs);

			// Modify the sun direction vector so one can control whether or not the light comes perpendicularly at 12 o'clock
			sunDirection = glm::angleAxis(mAngleOfIncidence, Math::VEC3_FORWARD) * sunDirection;

			// Tell the owner scene node about the new rotation
			// TODO(co) Can we simplify this?
			parentSceneNode->setRotation(glm::inverse(glm::quat(glm::lookAt(Math::VEC3_ZERO, sunDirection, Math::VEC3_UP))));
		}
	}


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void SunlightSceneItem::deserialize([[maybe_unused]] uint32_t numberOfBytes, const uint8_t* data)
	{
		RENDERER_ASSERT(getContext(), sizeof(v1Scene::SunlightItem) == numberOfBytes, "Invalid number of bytes")

		// Read data
		const v1Scene::SunlightItem* sunlightItem = reinterpret_cast<const v1Scene::SunlightItem*>(data);
		mSunriseTime	  = sunlightItem->sunriseTime;
		mSunsetTime		  = sunlightItem->sunsetTime;
		mEastDirection	  = sunlightItem->eastDirection;
		mAngleOfIncidence = sunlightItem->angleOfIncidence;
		mTimeOfDay		  = sunlightItem->timeOfDay;

		// Sanity checks (units in O'clock)
		RENDERER_ASSERT(getContext(), mSunriseTime >= 00.00f && mSunriseTime < 24.00f, "Invalid sunrise time")
		RENDERER_ASSERT(getContext(), mSunsetTime >= 00.00f && mSunsetTime < 24.00f, "Invalid sunset time")
		RENDERER_ASSERT(getContext(), mTimeOfDay >= 00.00f && mTimeOfDay < 24.00f, "Invalid time of day")

		// Calculated derived sunlight properties
		calculatedDerivedSunlightProperties();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
