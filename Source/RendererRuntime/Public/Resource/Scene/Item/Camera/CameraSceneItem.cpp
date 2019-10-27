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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "RendererRuntime/Public/Resource/Scene/Loader/SceneFileFormat.h"
#include "RendererRuntime/Public/Resource/Scene/SceneNode.h"
#include "RendererRuntime/Public/Core/Math/Math.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	const glm::dvec3& CameraSceneItem::getWorldSpaceCameraPosition() const
	{
		const SceneNode* parentSceneNode = getParentSceneNode();
		return (nullptr != parentSceneNode) ? parentSceneNode->getGlobalTransform().position : Math::DVEC3_ZERO;
	}

	const Transform& CameraSceneItem::getWorldSpaceToViewSpaceTransform() const
	{
		const SceneNode* parentSceneNode = getParentSceneNode();
		return (nullptr != parentSceneNode) ? parentSceneNode->getGlobalTransform() : Transform::IDENTITY;
	}

	const Transform& CameraSceneItem::getPreviousWorldSpaceToViewSpaceTransform() const
	{
		const SceneNode* parentSceneNode = getParentSceneNode();
		return (nullptr != parentSceneNode) ? parentSceneNode->getPreviousGlobalTransform() : Transform::IDENTITY;
	}

	const glm::mat4& CameraSceneItem::getCameraRelativeWorldSpaceToViewSpaceMatrix() const
	{
		// Calculate the world space to view space matrix (Aka "view matrix")
		if (!mHasCustomWorldSpaceToViewSpaceMatrix)
		{
			const Transform& worldSpaceToViewSpaceTransform = getWorldSpaceToViewSpaceTransform();
			mWorldSpaceToViewSpaceMatrix = glm::lookAt(Math::VEC3_ZERO, worldSpaceToViewSpaceTransform.rotation * Math::VEC3_FORWARD, Math::VEC3_UP);
		}

		// Done
		return mWorldSpaceToViewSpaceMatrix;
	}

	void CameraSceneItem::getPreviousCameraRelativeWorldSpaceToViewSpaceMatrix(glm::mat4& previousCameraRelativeWorldSpaceToViewSpaceMatrix) const
	{
		// Calculate the previous world space to view space matrix (Aka "view matrix")
		const Transform& worldSpaceToViewSpaceTransform = getWorldSpaceToViewSpaceTransform();
		const Transform& previousWorldSpaceToViewSpaceTransform = getPreviousWorldSpaceToViewSpaceTransform();
		const glm::vec3 positionOffset = worldSpaceToViewSpaceTransform.position - previousWorldSpaceToViewSpaceTransform.position;
		previousCameraRelativeWorldSpaceToViewSpaceMatrix = glm::lookAt(positionOffset, positionOffset + previousWorldSpaceToViewSpaceTransform.rotation * Math::VEC3_FORWARD, Math::VEC3_UP);
	}

	const glm::mat4& CameraSceneItem::getViewSpaceToClipSpaceMatrix(float aspectRatio) const
	{
		// Calculate the view space to clip space matrix (aka "projection matrix")
		if (!mHasCustomViewSpaceToClipSpaceMatrix)
		{
			mViewSpaceToClipSpaceMatrix = glm::perspective(mFovY, aspectRatio, mNearZ, mFarZ);
		}

		// Done
		return mViewSpaceToClipSpaceMatrix;
	}

	const glm::mat4& CameraSceneItem::getViewSpaceToClipSpaceMatrixReversedZ(float aspectRatio) const
	{
		// Calculate the view space to clip space matrix (aka "projection matrix")
		if (!mHasCustomViewSpaceToClipSpaceMatrix)
		{
			// Near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
			mViewSpaceToClipSpaceMatrixReversedZ = glm::perspective(mFovY, aspectRatio, mFarZ, mNearZ);
		}

		// Done
		return mViewSpaceToClipSpaceMatrixReversedZ;
	}


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::ISceneItem methods            ]
	//[-------------------------------------------------------]
	void CameraSceneItem::deserialize([[maybe_unused]] uint32_t numberOfBytes, const uint8_t*)
	{
		RHI_ASSERT(getContext(), sizeof(v1Scene::CameraItem) == numberOfBytes, "Invalid number of bytes")

		// No FOV Y, near z and far z deserialization by intent, those are usually application controlled values
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	CameraSceneItem::CameraSceneItem(SceneResource& sceneResource) :
		ISceneItem(sceneResource),
		mFovY(DEFAULT_FOV_Y),
		mNearZ(DEFAULT_NEAR_Z),
		mFarZ(DEFAULT_FAR_Z),
		mWorldSpaceToViewSpaceMatrix(Math::MAT4_IDENTITY),
		mViewSpaceToClipSpaceMatrix(Math::MAT4_IDENTITY),
		mViewSpaceToClipSpaceMatrixReversedZ(Math::MAT4_IDENTITY),
		mHasCustomWorldSpaceToViewSpaceMatrix(false),
		mHasCustomViewSpaceToClipSpaceMatrix(false)
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
