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
#include "RendererRuntime/Public/Export.h"
#include "RendererRuntime/Public/Core/Math/Plane.h"


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
	*    Frustum
	*/
	class Frustum final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*   Enumeration for clip planes
		*/
		enum PlaneIndex
		{
			PLANE_LEFT = 0,
			PLANE_RIGHT,
			PLANE_TOP,
			PLANE_BOTTOM,
			PLANE_NEAR,
			PLANE_FAR,
			NUMBER_OF_PLANES
		};


	//[-------------------------------------------------------]
	//[ Public data                                           ]
	//[-------------------------------------------------------]
	public:
		Plane planes[NUMBER_OF_PLANES];	///< Plane normals point into frustum


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		Frustum() = delete;

		/**
		*  @brief
		*    Constructor creating frustum from matrix
		*
		*  @remarks
		*    - If extracted from projection matrix, planes will be in view space
		*    - If extracted from world space to clip space matrix (aka view projection matrix), planes will be in world space
		*    - If extracted from object space to clip space matrix (aka model view projection), planes will be in object space
		*/
		RENDERERRUNTIME_API_EXPORT explicit Frustum(const glm::mat4& matrix);

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Frustum()
		{
			// Nothing here
		}


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
