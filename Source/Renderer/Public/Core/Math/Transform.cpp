/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Renderer/Public/Core/Math/Transform.h"
#include "Renderer/Public/Core/Math/Math.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/gtc/matrix_transform.hpp>
	#include <glm/gtx/matrix_decompose.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const Transform Transform::IDENTITY;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void Transform::getAsMatrix(glm::dmat4& objectSpaceToWorldSpace) const
	{
		// TODO(co) Optimize
		objectSpaceToWorldSpace = glm::translate(Math::DMAT4_IDENTITY, position) * glm::mat4_cast(glm::dquat(rotation)) * glm::scale(Math::DMAT4_IDENTITY, glm::dvec3(scale));
	}

	void Transform::getAsMatrix(glm::mat4& objectSpaceToWorldSpace) const
	{
		// Do only use this 32 bit precision if you're certain it's sufficient (for example because you made the position camera relative)
		// TODO(co) Optimize
		objectSpaceToWorldSpace = glm::translate(Math::MAT4_IDENTITY, glm::vec3(position)) * glm::mat4_cast(rotation) * glm::scale(Math::MAT4_IDENTITY, scale);
	}

	void Transform::setByMatrix(const glm::dmat4& objectSpaceToWorldSpace)
	{
		// TODO(co) Optimize
		glm::dvec3 scale64Bit;
		glm::dquat rotation64Bit;
		glm::dvec3 skew;
		glm::dvec4 perspective;
		glm::decompose(objectSpaceToWorldSpace, scale64Bit, rotation64Bit, position, skew, perspective);
		scale = scale64Bit;
		rotation = rotation64Bit;
	}

	Transform& Transform::operator+=(const Transform& other)
	{
		// Set update position, rotation and scale
		position += glm::dquat(rotation) * other.position;
		rotation  = rotation * other.rotation;
		scale    *= other.scale;

		// Return a reference to this instance
		return *this;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
