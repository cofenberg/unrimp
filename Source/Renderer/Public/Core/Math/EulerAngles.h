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
#include "Renderer/Public/Export.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/fwd.hpp>
PRAGMA_WARNING_POP


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
	*    Static Euler angles conversion tool class
	*
	*  @remarks
	*    The implementation of this class is based on the 'Euler Angle Conversion' gem from Ken Shoemake (1993)
	*    published in the 'Graphics Gems IV' book. The original code is available here:
	*    http://www.graphicsgems.org/gemsiv/euler_angle/
	*
	*  @note
	*    - GLM has Euler angles functions as well, but sadly those didn't match the required feature set; topic discussed here: https://github.com/g-truc/glm/issues/569
	*    - Class originally comes from the PixelLight engine ( https://github.com/PixelLightFoundation/pixellight )
	*    - In Unrimp we use the "YXZr" order as default:
	*       1.) "yaw" represents a rotation around the Y-axis (= up vector)
	*       2.) Then "pitch" is applied as a rotation around the local (i.e. already rotated) X-axis (= right vector)
	*       3.) Finally "roll" rotates around the local Z-axis (= front vector)
	*    - When dealing with Euler angles keep care of 'gimbal lock', at http://www.sjbaker.org/steve/omniv/eulers_are_evil.html
	*      you will find some good information about this topic.
	*/
	class RENDERER_API_EXPORT EulerAngles final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Euler angles order
		*/
		enum Order
		{
			// Static axes (takes axes from initial static frame)
			XYZs = 0,	///< XYZ static axis
			XYXs = 2,	///< XYX static axis
			XZYs = 4,	///< XZY static axis
			XZXs = 6,	///< XZX static axis
			YZXs = 8,	///< YZX static axis
			YZYs = 10,	///< YZY static axis
			YXZs = 12,	///< YXZ static axis
			YXYs = 14,	///< YXY static axis
			ZXYs = 16,	///< ZXY static axis
			ZXZs = 18,	///< ZXZ static axis
			ZYXs = 20,	///< ZYX static axis
			ZYZs = 22,	///< ZYZ static axis
			// Rotating axes
			ZYXr = 1,	///< ZYX rotating axis
			XYXr = 3,	///< XYX rotating axis
			YZXr = 5,	///< YZX rotating axis
			XZXr = 7,	///< XZX rotating axis
			XZYr = 9,	///< XZY rotating axis
			YZYr = 11,	///< YZY rotating axis
			ZXYr = 13,	///< ZXY rotating axis
			YXYr = 15,	///< YXY rotating axis
			YXZr = 17,	///< YXZ rotating axis
			ZXZr = 19,	///< ZXZ rotating axis
			XYZr = 21,	///< XYZ rotating axis
			ZYZr = 23	///< ZYZ rotating axis
		};
		static constexpr Order DefaultOrder = Order::YXZr;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Set a rotation quaternion by using three given Euler angles
		*
		*  @param[in] eulerAngles
		*    Euler angles in vector format: yaw, pitch, roll in radians; for more information, see notes on EulerAngles class
		*  @param[in]  order
		*    Order of the Euler angles
		*
		*  @return
		*    Resulting rotation quaternion
		*/
		[[nodiscard]] static glm::quat eulerToQuaternion(const glm::vec3& eulerAngles, Order order = DefaultOrder);

		/**
		*  @brief
		*    Set a rotation quaternion by using three given Euler angles
		*
		*  @param[in] yaw
		*    Yaw angle in radians; for more information, see notes on EulerAngles class
		*  @param[in] pitch
		*    Pitch angle in radians; for more information, see notes on EulerAngles class
		*  @param[in] roll
		*    Roll angle in radians; for more information, see notes on EulerAngles class
		*  @param[in]  order
		*    Order of the Euler angles
		*
		*  @return
		*    Resulting rotation quaternion
		*/
		[[nodiscard]] static glm::quat eulerToQuaternion(float yaw, float pitch, float roll, Order order = DefaultOrder);

		/**
		*  @brief
		*    Set a rotation matrix by using the given Euler angles
		*
		*  @param[in]  eulerAngles
		*    Euler angles in vector format: yaw, pitch, roll in radians; for more information, see notes on EulerAngles class
		*  @param[out] mRot
		*    Resulting rotation matrix
		*  @param[in]  order
		*    Order of the Euler angles
		*/
		static void eulerToMatrix(const glm::vec3& eulerAngles, glm::mat3& mRot, Order order = DefaultOrder);
		static void eulerToMatrix(const glm::vec3& eulerAngles, glm::mat3x4& mRot, Order order = DefaultOrder);
		static void eulerToMatrix(const glm::vec3& eulerAngles, glm::mat4& mRot, Order order = DefaultOrder);

		/**
		*  @brief
		*    Return the Euler angles from a rotation matrix
		*
		*  @param[in] mRot
		*    Rotation matrix
		*  @param[in] order
		*    Order of the Euler angles
		*
		*  @return
		*    Resulting Euler angles in vector format: yaw, pitch, roll in radians; for more information, see notes on EulerAngles class
		*/
		[[nodiscard]] static glm::vec3 matrixToEuler(const glm::mat3& mRot, Order order = DefaultOrder);
		[[nodiscard]] static glm::vec3 matrixToEuler(const glm::mat3x4& mRot, Order order = DefaultOrder);
		[[nodiscard]] static glm::vec3 matrixToEuler(const glm::mat4& mRot, Order order = DefaultOrder);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		EulerAngles() = delete;
		~EulerAngles() = delete;
		explicit EulerAngles(const EulerAngles&) = delete;
		EulerAngles& operator=(const EulerAngles&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
