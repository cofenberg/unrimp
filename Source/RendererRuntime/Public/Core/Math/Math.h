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

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
	#include <glm/gtc/quaternion.hpp>
PRAGMA_WARNING_POP

#include <inttypes.h>	// For uint32_t, uint64_t etc.


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IRenderer;
}
namespace RendererRuntime
{
	class IFileManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef const char* VirtualFilename;	///< UTF-8 virtual filename, the virtual filename scheme is "<mount point = project name>/<asset directory>/<asset name>.<file extension>" (example "Example/Mesh/Monster/Squirrel.mesh"), never ever a null pointer and always finished by a terminating zero


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class Math final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t FNV1a_INITIAL_HASH_32 = 0xcbf29ce4u;
		static constexpr uint64_t FNV1a_INITIAL_HASH_64 = 0xcbf29ce484222325u;
		// "glm::vec3" constants
		static constexpr glm::vec3 VEC3_ZERO	= glm::vec3(0.0f, 0.0f, 0.0f);	///< 0 0 0
		static constexpr glm::vec3 VEC3_ONE		= glm::vec3(1.0f, 1.0f, 1.0f);	///< 1 1 1
		static constexpr glm::vec3 VEC3_UNIT_X	= glm::vec3(1.0f, 0.0f, 0.0f);	///< 1 0 0
		static constexpr glm::vec3 VEC3_RIGHT	= glm::vec3(1.0f, 0.0f, 0.0f);	///< 1 0 0; coordinate system axis terminology, don't remove
		static constexpr glm::vec3 VEC3_UNIT_Y	= glm::vec3(0.0f, 1.0f, 0.0f);	///< 0 1 0
		static constexpr glm::vec3 VEC3_UP		= glm::vec3(0.0f, 1.0f, 0.0f);	///< 0 1 0; coordinate system axis terminology, don't remove
		static constexpr glm::vec3 VEC3_UNIT_Z	= glm::vec3(0.0f, 0.0f, 1.0f);	///< 0 0 1
		static constexpr glm::vec3 VEC3_FORWARD	= glm::vec3(0.0f, 0.0f, 1.0f);	///< 0 0 1; coordinate system axis terminology, don't remove
		// "glm::dvec3" constants
		static constexpr glm::dvec3 DVEC3_ZERO	  = glm::dvec3(0.0, 0.0, 0.0);	///< 0 0 0
		static constexpr glm::dvec3 DVEC3_ONE	  = glm::dvec3(1.0, 1.0, 1.0);	///< 1 1 1
		static constexpr glm::dvec3 DVEC3_UNIT_X  = glm::dvec3(1.0, 0.0, 0.0);	///< 1 0 0
		static constexpr glm::dvec3 DVEC3_RIGHT	  = glm::dvec3(1.0, 0.0, 0.0);	///< 1 0 0; coordinate system axis terminology, don't remove
		static constexpr glm::dvec3 DVEC3_UNIT_Y  = glm::dvec3(0.0, 1.0, 0.0);	///< 0 1 0
		static constexpr glm::dvec3 DVEC3_UP	  = glm::dvec3(0.0, 1.0, 0.0);	///< 0 1 0; coordinate system axis terminology, don't remove
		static constexpr glm::dvec3 DVEC3_UNIT_Z  = glm::dvec3(0.0, 0.0, 1.0);	///< 0 0 1
		static constexpr glm::dvec3 DVEC3_FORWARD = glm::dvec3(0.0, 0.0, 1.0);	///< 0 0 1; coordinate system axis terminology, don't remove
		// "glm::vec4" constants
		static constexpr glm::vec4 VEC4_ZERO = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);	///< 0 0 0 0
		static constexpr glm::vec4 VEC4_ONE  = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);	///< 1 1 1 1
		// "glm::dvec4" constants
		static constexpr glm::dvec4 DVEC4_ZERO = glm::dvec4(0.0, 0.0, 0.0, 0.0);	///< 0 0 0 0
		static constexpr glm::dvec4 DVEC4_ONE  = glm::dvec4(1.0, 1.0, 1.0, 1.0);	///< 1 1 1 1
		// "glm::mat4" constants
		static constexpr glm::mat4 MAT4_IDENTITY = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
															 0.0f, 1.0f, 0.0f, 0.0f,
															 0.0f, 0.0f, 1.0f, 0.0f,
															 0.0f, 0.0f, 0.0f, 1.0f);
		// "glm::dmat4" constants
		static constexpr glm::dmat4 DMAT4_IDENTITY = glm::dmat4(1.0, 0.0, 0.0, 0.0,
																0.0, 1.0, 0.0, 0.0,
																0.0, 0.0, 1.0, 0.0,
																0.0, 0.0, 0.0, 1.0);
		// "glm::quat" constants
		static constexpr glm::quat QUAT_IDENTITY = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		// "glm::squat" constants
		static constexpr glm::dquat DQUAT_IDENTITY = glm::dquat(1.0, 0.0, 0.0, 0.0);


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Calculate tangent frame quaternion (QTangent) basing of a provided 3x3 tangent frame matrix
		*
		*  @param[in, out] tangentFrameMatrix
		*    3x3 tangent frame matrix, will be manipulated during calculation (no internal copy for performance reasons)
		*
		*  @return
		*    The calculated tangent frame quaternion (QTangent)
		*
		*  @note
		*  - QTangent basing on http://dev.theomader.com/qtangents/ "QTangents" which is basing on
		*    http://www.crytek.com/cryengine/presentations/spherical-skinning-with-dual-quaternions-and-qtangents "Spherical Skinning with Dual-Quaternions and QTangents"
		*/
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static glm::quat calculateTangentFrameQuaternion(glm::mat3& tangentFrameMatrix);

		/**
		*  @brief
		*    Calculate inner bounding sphere radius
		*
		*  @param[in] minimumBoundingBoxPosition
		*    The minimum bounding box position
		*  @param[in] maximumBoundingBoxPosition
		*    The maximum bounding box position
		*
		*  @return
		*    Inner bounding sphere radius
		*/
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static float calculateInnerBoundingSphereRadius(const glm::vec3& minimumBoundingBoxPosition, const glm::vec3& maximumBoundingBoxPosition);

		/**
		*  @brief
		*    Ensure that the given value is within the given interval [minimum, maximum] by wrapping the value
		*
		*  @param[in] value
		*    Value to check
		*  @param[in] minimum
		*    Minimum of the interval, must be < maximum
		*  @param[in] maximum
		*    Maximum of the interval, must be > minimum
		*
		*  @return
		*    The value within the interval [minimum, maximum]
		*
		*  @note
		*    - In case of violating the interface specification by swapping minimum/maximum the result will not be different to the one of the correct order
		*/
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static float wrapToInterval(float value, float minimum, float maximum);

		/**
		*  @brief
		*    Make a given value to a multiple of another given value
		*
		*  @param[in] value
		*    Value to make a multiple of the given second value (e.g. 0.9)
		*  @param[in] primaryValue
		*    Value the first one should be a multiple of (e.g. 1.5)
		*
		*  @return
		*    The updated value (e.g. 1.5)
		*/
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static float makeMultipleOf(float value, float primaryValue);
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static double makeMultipleOf(double value, float primaryValue);
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static uint32_t makeMultipleOf(uint32_t value, uint32_t primaryValue);

		/**
		*  @brief
		*    Get renderer dependent texture scale bias matrix
		*
		*  @param[in] renderer
		*    Used renderer
		*
		*  @return
		*    Renderer dependent texture scale bias matrix
		*/
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static const glm::mat4& getTextureScaleBiasMatrix(const Renderer::IRenderer& renderer);

		//[-------------------------------------------------------]
		//[ Hash                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static uint32_t calculateFNV1a32(const uint8_t* content, uint32_t numberOfBytes, uint32_t hash = FNV1a_INITIAL_HASH_32);
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static uint64_t calculateFNV1a64(const uint8_t* content, uint32_t numberOfBytes, uint64_t hash = FNV1a_INITIAL_HASH_64);
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT static uint64_t calculateFileFNV1a64ByVirtualFilename(const IFileManager& fileManager, VirtualFilename virtualFilename);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		Math() = delete;
		~Math() = delete;
		explicit Math(const Math&) = delete;
		Math& operator=(const Math&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
