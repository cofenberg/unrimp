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
#include "RendererRuntime/Public/Context.h"
#include "RendererRuntime/Public/Resource/Scene/Item/ISceneItem.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP


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
	*    Light scene item
	*/
	class LightSceneItem : public ISceneItem
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;			// Needs to be able to create scene item instances
		friend class LightBufferManager;	// Needs access to "RendererRuntime::LightSceneItem::mPackedShaderData"


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("LightSceneItem");
		enum class LightType
		{
			DIRECTIONAL = 0,
			POINT,
			SPOT
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline LightType getLightType() const
		{
			return static_cast<LightType>(static_cast<int>(mPackedShaderData.lightType));
		}

		inline void setLightType(LightType lightType)
		{
			mPackedShaderData.lightType = static_cast<float>(lightType);

			// Sanity checks
			RENDERER_ASSERT(getContext(), lightType == LightType::DIRECTIONAL || mPackedShaderData.radius > 0.0f, "Invalid data")
			RENDERER_ASSERT(getContext(), lightType != LightType::DIRECTIONAL || 0.0f == mPackedShaderData.radius, "Invalid data")
		}

		inline void setLightTypeAndRadius(LightType lightType, float radius)
		{
			mPackedShaderData.lightType = static_cast<float>(lightType);
			mPackedShaderData.radius = radius;

			// Sanity checks
			RENDERER_ASSERT(getContext(), lightType == LightType::DIRECTIONAL || mPackedShaderData.radius > 0.0f, "Invalid data")
			RENDERER_ASSERT(getContext(), lightType != LightType::DIRECTIONAL || 0.0f == mPackedShaderData.radius, "Invalid data")
		}

		[[nodiscard]] inline const glm::vec3& getColor() const
		{
			return mPackedShaderData.color;
		}

		inline void setColor(const glm::vec3& color)
		{
			mPackedShaderData.color = color;

			// Sanity checks
			RENDERER_ASSERT(getContext(), mPackedShaderData.color.x >= 0.0f && mPackedShaderData.color.y >= 0.0f && mPackedShaderData.color.z >= 0.0f, "Invalid data")
		}

		[[nodiscard]] inline float getRadius() const
		{
			return mPackedShaderData.radius;
		}

		inline void setRadius(float radius)
		{
			mPackedShaderData.radius = radius;

			// Sanity checks
			RENDERER_ASSERT(getContext(), mPackedShaderData.lightType == static_cast<float>(LightType::DIRECTIONAL) || mPackedShaderData.radius > 0.0f, "Invalid data")
			RENDERER_ASSERT(getContext(), mPackedShaderData.lightType != static_cast<float>(LightType::DIRECTIONAL) || 0.0f == mPackedShaderData.radius, "Invalid data")
		}

		[[nodiscard]] inline float getInnerAngle() const
		{
			return mInnerAngle;
		}

		inline void setInnerAngle(float innerAngle)
		{
			mInnerAngle = innerAngle;

			// Derive data
			mPackedShaderData.innerAngle = std::cos(mInnerAngle);

			// Sanity checks
			RENDERER_ASSERT(getContext(), mInnerAngle >= 0.0f, "Invalid data")
			RENDERER_ASSERT(getContext(), mInnerAngle < mOuterAngle, "Invalid data")
		}

		[[nodiscard]] inline float getOuterAngle() const
		{
			return mOuterAngle;
		}

		inline void setOuterAngle(float outerAngle)
		{
			mOuterAngle = outerAngle;

			// Derive data
			mPackedShaderData.outerAngle = std::cos(mOuterAngle);

			// Sanity checks
			RENDERER_ASSERT(getContext(), mOuterAngle < glm::radians(90.0f), "Invalid data")
			RENDERER_ASSERT(getContext(), mInnerAngle < mOuterAngle, "Invalid data")
		}

		inline void setInnerOuterAngle(float innerAngle, float outerAngle)
		{
			mInnerAngle = innerAngle;
			mOuterAngle = outerAngle;

			// Derive data
			mPackedShaderData.innerAngle = std::cos(mInnerAngle);
			mPackedShaderData.outerAngle = std::cos(mOuterAngle);

			// Sanity checks
			RENDERER_ASSERT(getContext(), mInnerAngle >= 0.0f, "Invalid data")
			RENDERER_ASSERT(getContext(), mOuterAngle < glm::radians(90.0f), "Invalid data")
			RENDERER_ASSERT(getContext(), mInnerAngle < mOuterAngle, "Invalid data")
		}

		[[nodiscard]] inline float getNearClipDistance() const
		{
			return mPackedShaderData.nearClipDistance;
		}

		inline void setNearClipDistance(float nearClipDistance)
		{
			mPackedShaderData.nearClipDistance = nearClipDistance;

			// Sanity check
			RENDERER_ASSERT(getContext(), mPackedShaderData.nearClipDistance >= 0.0f, "Invalid data")
		}

		[[nodiscard]] inline bool isVisible() const
		{
			return (mPackedShaderData.visible != 0);
		}


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual SceneItemTypeId getSceneItemTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void deserialize(uint32_t numberOfBytes, const uint8_t* data) override;

		inline virtual void setVisible(bool visible) override
		{
			mPackedShaderData.visible = static_cast<uint32_t>(visible);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit LightSceneItem(SceneResource& sceneResource) :
			ISceneItem(sceneResource),
			mInnerAngle(0.0f),
			mOuterAngle(0.1f)
		{
			setInnerOuterAngle(glm::radians(40.0f), glm::radians(50.0f));
		}

		inline virtual ~LightSceneItem() override
		{
			// Nothing here
		}

		explicit LightSceneItem(const LightSceneItem&) = delete;
		LightSceneItem& operator=(const LightSceneItem&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Light data packed into a form which can be directly 1:1 copied into a GPU buffer; don't change the layout in here without updating the shaders using the data
		*/
		struct PackedShaderData final
		{
			// float4 0: xyz = world space light position, w = light radius
			glm::vec3 position{0.0f, 0.0f, 0.0f};	///< Parent scene node world space position
			float	  radius = 1.0f;
			// float4 1: xyz = RGB light diffuse color, w = unused
			glm::vec3 color{1.0f, 1.0f, 1.0f};
			float	  lightType = static_cast<float>(LightType::POINT);
			// float4 2: Only used for spot-light: x = spot-light inner angle in radians, y = spot-light outer angle in radians, z = spot-light near clip distance, w = unused
			float innerAngle       = 0.0f;	///< Cosine of the inner angle in radians; interval in degrees: 0..90, must be smaller as the outer angle
			float outerAngle       = 0.0f;	///< Cosine of the outer angle in radians; interval in degrees: 0..90, must be greater as the inner angle
			float nearClipDistance = 0.0f;
			float unused           = 0.0f;
			// float4 3: Only used for spot-light: xyz = normalized view space light direction, w = unused
			glm::vec3 direction{0.0f, 0.0f, 1.0f};	///< Derived from the parent scene node world space rotation
			uint32_t  visible = 1;					///< Boolean, not used inside the shader but well, there's currently space left in here so we're using it
		};


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		PackedShaderData mPackedShaderData;
		float			 mInnerAngle;	///< Inner angle in radians; interval in degrees: 0..90, must be smaller as the outer angle
		float			 mOuterAngle;	///< Outer angle in radians; interval in degrees: 0..90, must be greater as the inner angle


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
