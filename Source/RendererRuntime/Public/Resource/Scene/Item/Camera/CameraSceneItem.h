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
#include "RendererRuntime/Public/Export.h"
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
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class Transform;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class CameraSceneItem final : public ISceneItem
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;	// Needs to be able to create scene item instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID	  = STRING_ID("CameraSceneItem");
		static constexpr float DEFAULT_FOV_Y  = glm::radians(45.0f);	///< Default Y field of view in radians
		static constexpr float DEFAULT_NEAR_Z = 0.1f;
		static constexpr float DEFAULT_FAR_Z  = 5000.0f;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Data                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline float getFovY() const
		{
			return mFovY;
		}

		inline void setFovY(float fovY)
		{
			mFovY = fovY;
		}

		[[nodiscard]] inline float getNearZ() const
		{
			return mNearZ;
		}

		inline void setNearZ(float nearZ)
		{
			mNearZ = nearZ;
		}

		[[nodiscard]] inline float getFarZ() const
		{
			return mFarZ;
		}

		inline void setFarZ(float farZ)
		{
			mFarZ = farZ;
		}

		//[-------------------------------------------------------]
		//[ Derived or custom data                                ]
		//[-------------------------------------------------------]
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const glm::dvec3& getWorldSpaceCameraPosition() const;	// Ease-of-use method for camera relative rendering: 64 bit world space position of the camera

		// World space to view space matrix (Aka "view matrix")
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const Transform& getWorldSpaceToViewSpaceTransform() const;
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const Transform& getPreviousWorldSpaceToViewSpaceTransform() const;
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const glm::mat4& getCameraRelativeWorldSpaceToViewSpaceMatrix() const;
		RENDERERRUNTIME_API_EXPORT void getPreviousCameraRelativeWorldSpaceToViewSpaceMatrix(glm::mat4& previousCameraRelativeWorldSpaceToViewSpaceMatrix) const;

		[[nodiscard]] inline bool hasCustomWorldSpaceToViewSpaceMatrix() const
		{
			return mHasCustomWorldSpaceToViewSpaceMatrix;
		}

		inline void unsetCustomWorldSpaceToViewSpaceMatrix()
		{
			mHasCustomWorldSpaceToViewSpaceMatrix = false;
		}

		inline void setCustomWorldSpaceToViewSpaceMatrix(const glm::mat4& customWorldSpaceToViewSpaceMatrix)
		{
			mWorldSpaceToViewSpaceMatrix = customWorldSpaceToViewSpaceMatrix;
			mHasCustomWorldSpaceToViewSpaceMatrix = true;
		}

		// View space to clip space matrix (aka "projection matrix")
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const glm::mat4& getViewSpaceToClipSpaceMatrix(float aspectRatio) const;
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const glm::mat4& getViewSpaceToClipSpaceMatrixReversedZ(float aspectRatio) const;

		[[nodiscard]] inline bool hasCustomViewSpaceToClipSpaceMatrix() const
		{
			return mHasCustomViewSpaceToClipSpaceMatrix;
		}

		inline void unsetCustomViewSpaceToClipSpaceMatrix()
		{
			mHasCustomViewSpaceToClipSpaceMatrix = false;
		}

		inline void setCustomViewSpaceToClipSpaceMatrix(const glm::mat4& customViewSpaceToClipSpaceMatrix, const glm::mat4& customViewSpaceToClipSpaceMatrixReversedZ)
		{
			mViewSpaceToClipSpaceMatrix = customViewSpaceToClipSpaceMatrix;
			mViewSpaceToClipSpaceMatrixReversedZ = customViewSpaceToClipSpaceMatrixReversedZ;
			mHasCustomViewSpaceToClipSpaceMatrix = true;
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


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit CameraSceneItem(SceneResource& sceneResource);

		inline virtual ~CameraSceneItem() override
		{
			// Nothing here
		}

		explicit CameraSceneItem(const CameraSceneItem&) = delete;
		CameraSceneItem& operator=(const CameraSceneItem&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Data
		float mFovY;	///< Y field of view in radians
		float mNearZ;
		float mFarZ;
		// Derived or custom data
		mutable glm::mat4 mWorldSpaceToViewSpaceMatrix;				// Aka "view matrix"
		mutable glm::mat4 mViewSpaceToClipSpaceMatrix;				// Aka "projection matrix"
		mutable glm::mat4 mViewSpaceToClipSpaceMatrixReversedZ;		// Aka "projection matrix"
		bool			  mHasCustomWorldSpaceToViewSpaceMatrix;	// Aka "view matrix"
		bool			  mHasCustomViewSpaceToClipSpaceMatrix;		// Aka "projection matrix"


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
