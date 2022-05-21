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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Core/Math/Math.h"


// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4251)	// warning C4251: "needs to have dll-interface to be used by clients of class "


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
	*    Transform class containing 64 bit position, 32 bit rotation and 32 bit scale
	*/
	class RENDERER_API_EXPORT Transform final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static const Transform IDENTITY;	///< Identity transform


	//[-------------------------------------------------------]
	//[ Public data                                           ]
	//[-------------------------------------------------------]
	public:
		glm::dvec3 position;	// 64 bit world space position, or depending on the use-case in another coordinate system
		glm::quat  rotation;
		glm::vec3  scale;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline Transform() :
			position(Math::DVEC3_ZERO),
			rotation(Math::QUAT_IDENTITY),
			scale(Math::VEC3_ONE)
		{
			// Nothing here
		}

		inline explicit Transform(const glm::dvec3& _position) :
			position(_position),
			rotation(Math::QUAT_IDENTITY),
			scale(Math::VEC3_ONE)
		{
			// Nothing here
		}

		inline explicit Transform(const glm::dmat4& transformMatrix)
		{
			setByMatrix(transformMatrix);
		}

		inline Transform(const glm::dvec3& _position, const glm::quat& _rotation) :
			position(_position),
			rotation(_rotation),
			scale(Math::VEC3_ONE)
		{
			// Nothing here
		}

		inline Transform(const glm::dvec3& _position, const glm::quat& _rotation, const glm::vec3& _scale) :
			position(_position),
			rotation(_rotation),
			scale(_scale)
		{
			// Nothing here
		}

		void getAsMatrix(glm::dmat4& objectSpaceToWorldSpace) const;
		void getAsMatrix(glm::mat4& objectSpaceToWorldSpace) const;	// Do only use this 32 bit precision if you're certain it's sufficient (for example because you made the position camera relative)
		void setByMatrix(const glm::dmat4& objectSpaceToWorldSpace);
		Transform& operator+=(const Transform& other);


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer


// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_POP
