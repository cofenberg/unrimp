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
#include "RendererRuntime/Public/Core/Math/EulerAngles.h"
#include "RendererRuntime/Public/Core/Math/Math.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtc/quaternion.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Definitions                                           ]
	//[-------------------------------------------------------]
	static constexpr float floatEpsilon = 1.192092896e-07f;	// Smallest such that 1.0f+floatEpsilon != 1.0

	// EULER_ANGLES_INIT_ORDER unpacks all useful information about order simultaneously
	#define EULER_ANGLES_SAFE "\000\001\002\000"
	#define EULER_ANGLES_NEXT "\001\002\000\001"

	#define EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating) \
	{ \
		unsigned o = order; \
		rotating = (o&1);   o >>= 1; \
		repetition = (o&1); o >>= 1; \
		parityOdd = (o&1);  o >>= 1; \
		i = EULER_ANGLES_SAFE[(o&3)]; \
		j = EULER_ANGLES_NEXT[i + (parityOdd ? 1 : 0)]; \
		k = EULER_ANGLES_NEXT[i + (parityOdd ? 0 : 1)]; \
	}


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	glm::quat EulerAngles::eulerToQuaternion(const glm::vec3& eulerAngles, Order order)
	{
		glm::vec3 angles = eulerAngles;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (rotating)
		{
			angles.x = eulerAngles.z;
			angles.z = eulerAngles.x;
		}
		if (parityOdd)
		{
			angles.y = -angles.y;
		}

		const double ti = angles.x * 0.5;
		const double tj = angles.y * 0.5;
		const double th = angles.z * 0.5;
		const double ci = std::cos(ti);
		const double cj = std::cos(tj);
		const double ch = std::cos(th);
		const double si = std::sin(ti);
		const double sj = std::sin(tj);
		const double sh = std::sin(th);
		const double cc = ci * ch;
		const double cs = ci * sh;
		const double sc = si * ch;
		const double ss = si * sh;

		float a[3];
		float w;
		if (repetition)
		{
			a[i] = static_cast<float>(cj * (cs + sc));
			a[j] = static_cast<float>(sj * (cc + ss));
			a[k] = static_cast<float>(sj * (cs - sc));
			w    = static_cast<float>(cj * (cc - ss));
		}
		else
		{
			a[i] = static_cast<float>(cj * sc - sj * cs);
			a[j] = static_cast<float>(cj * ss + sj * cc);
			a[k] = static_cast<float>(cj * cs - sj * sc);
			w    = static_cast<float>(cj * cc + sj * ss);
		}
		if (parityOdd)
		{
			a[j] = -a[j];
		}

		return glm::quat(w, a[0], a[1], a[2]);
	}

	glm::quat EulerAngles::eulerToQuaternion(float yaw, float pitch, float roll, Order order)
	{
		return eulerToQuaternion(glm::vec3(yaw, pitch, roll), order);
	}

	void EulerAngles::eulerToMatrix(const glm::vec3& eulerAngles, glm::mat3& mRot, Order order)
	{
		glm::vec3 angles = eulerAngles;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (rotating)
		{
			angles.x = eulerAngles.z;
			angles.z = eulerAngles.x;
		}
		if (parityOdd)
		{
			angles.x = -angles.x;
			angles.y = -angles.y;
			angles.z = -angles.z;
		}

		const double ti = angles.x;
		const double tj = angles.y;
		const double th = angles.z;
		const double ci = std::cos(ti);
		const double cj = std::cos(tj);
		const double ch = std::cos(th);
		const double si = std::sin(ti);
		const double sj = std::sin(tj);
		const double sh = std::sin(th);
		const double cc = ci*ch;
		const double cs = ci*sh;
		const double sc = si*ch;
		const double ss = si*sh;

		if (repetition)
		{
			mRot[i][i] = static_cast<float>(cj);       mRot[j][i] = static_cast<float>(sj * si);       mRot[k][i] = static_cast<float>(sj * ci);
			mRot[i][j] = static_cast<float>(sj * sh);  mRot[j][j] = static_cast<float>(-cj * ss + cc); mRot[k][j] = static_cast<float>(-cj * cs - sc);
			mRot[i][k] = static_cast<float>(-sj * ch); mRot[j][k] = static_cast<float>(cj * sc + cs);  mRot[k][k] = static_cast<float>(cj * cc - ss);
		}
		else
		{
			mRot[i][i] = static_cast<float>(cj * ch);  mRot[j][i] = static_cast<float>(sj * sc - cs);  mRot[k][i] = static_cast<float>(sj * cc + ss);
			mRot[i][j] = static_cast<float>(cj * sh);  mRot[j][j] = static_cast<float>(sj * ss + cc);  mRot[k][j] = static_cast<float>(sj * cs - sc);
			mRot[i][k] = static_cast<float>(-sj);      mRot[j][k] = static_cast<float>(cj * si);       mRot[k][k] = static_cast<float>(cj * ci);
		}
	}

	void EulerAngles::eulerToMatrix(const glm::vec3& eulerAngles, glm::mat3x4& mRot, Order order)
	{
		glm::vec3 angles = eulerAngles;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (rotating)
		{
			angles.x = eulerAngles.z;
			angles.z = eulerAngles.x;
		}
		if (parityOdd)
		{
			angles.x = -angles.x;
			angles.y = -angles.y;
			angles.z = -angles.z;
		}

		const double ti = angles.x;
		const double tj = angles.y;
		const double th = angles.z;
		const double ci = std::cos(ti);
		const double cj = std::cos(tj);
		const double ch = std::cos(th);
		const double si = std::sin(ti);
		const double sj = std::sin(tj);
		const double sh = std::sin(th);
		const double cc = ci * ch;
		const double cs = ci * sh;
		const double sc = si * ch;
		const double ss = si * sh;

		if (repetition)
		{
			mRot[i][i] = static_cast<float>(cj);       mRot[j][i] = static_cast<float>(sj * si);       mRot[k][i] = static_cast<float>(sj * ci);       mRot[0][3] = 0.0f;
			mRot[i][j] = static_cast<float>(sj * sh);  mRot[j][j] = static_cast<float>(-cj * ss + cc); mRot[k][j] = static_cast<float>(-cj * cs - sc); mRot[1][3] = 0.0f;
			mRot[i][k] = static_cast<float>(-sj * ch); mRot[j][k] = static_cast<float>(cj * sc + cs);  mRot[k][k] = static_cast<float>(cj * cc - ss);  mRot[2][3] = 0.0f;
		}
		else
		{
			mRot[i][i] = static_cast<float>(cj * ch);  mRot[j][i] = static_cast<float>(sj * sc - cs);  mRot[k][i] = static_cast<float>(sj * cc + ss);  mRot[0][3] = 0.0f;
			mRot[i][j] = static_cast<float>(cj * sh);  mRot[j][j] = static_cast<float>(sj * ss + cc);  mRot[k][j] = static_cast<float>(sj * cs - sc);  mRot[1][3] = 0.0f;
			mRot[i][k] = static_cast<float>(-sj);      mRot[j][k] = static_cast<float>(cj * si);       mRot[k][k] = static_cast<float>(cj * ci);       mRot[2][3] = 0.0f;
		}
	}

	void EulerAngles::eulerToMatrix(const glm::vec3& eulerAngles, glm::mat4& mRot, Order order)
	{
		glm::vec3 angles = eulerAngles;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (rotating)
		{
			angles.x = eulerAngles.z;
			angles.z = eulerAngles.x;
		}
		if (parityOdd)
		{
			angles.x = -angles.x;
			angles.y = -angles.y;
			angles.z = -angles.z;
		}

		const double ti = angles.x;
		const double tj = angles.y;
		const double th = angles.z;
		const double ci = std::cos(ti);
		const double cj = std::cos(tj);
		const double ch = std::cos(th);
		const double si = std::sin(ti);
		const double sj = std::sin(tj);
		const double sh = std::sin(th);
		const double cc = ci * ch;
		const double cs = ci * sh;
		const double sc = si * ch;
		const double ss = si * sh;

		if (repetition)
		{
			mRot[i][i] = static_cast<float>(cj);       mRot[j][i] = static_cast<float>(sj * si);       mRot[k][i] = static_cast<float>(sj * ci);       mRot[0][3] = 0.0f;
			mRot[i][j] = static_cast<float>(sj * sh);  mRot[j][j] = static_cast<float>(-cj * ss + cc); mRot[k][j] = static_cast<float>(-cj * cs - sc); mRot[1][3] = 0.0f;
			mRot[i][k] = static_cast<float>(-sj * ch); mRot[j][k] = static_cast<float>(cj * sc + cs);  mRot[k][k] = static_cast<float>(cj * cc - ss);  mRot[2][3] = 0.0f;
			mRot[3][0] = 0.0f;                         mRot[3][1] = 0.0f;                              mRot[3][2] = 0.0f;                              mRot[3][3] = 1.0f;
		}
		else
		{
			mRot[i][i] = static_cast<float>(cj * ch);  mRot[j][i] = static_cast<float>(sj * sc - cs);  mRot[k][i] = static_cast<float>(sj * cc + ss);  mRot[0][3] = 0.0f;
			mRot[i][j] = static_cast<float>(cj * sh);  mRot[j][j] = static_cast<float>(sj * ss + cc);  mRot[k][j] = static_cast<float>(sj * cs - sc);  mRot[1][3] = 0.0f;
			mRot[i][k] = static_cast<float>(-sj);      mRot[j][k] = static_cast<float>(cj * si);       mRot[k][k] = static_cast<float>(cj * ci);       mRot[2][3] = 0.0f;
			mRot[3][0] = 0.0f;                         mRot[3][1] = 0.0f;                              mRot[3][2] = 0.0f;                              mRot[3][3] = 1.0f;
		}
	}

	glm::vec3 EulerAngles::matrixToEuler(const glm::mat3& mRot, Order order)
	{
		glm::vec3 eulerAngles = Math::VEC3_ZERO;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (repetition)
		{
			const double sy = std::sqrt(mRot[j][i] * mRot[j][i] + mRot[k][i] * mRot[k][i]);
			if (sy > 16*floatEpsilon)
			{
				eulerAngles.x = std::atan2(mRot[j][i], mRot[k][i]);
				eulerAngles.y = static_cast<float>(std::atan2(sy, static_cast<double>(mRot[i][i])));
				eulerAngles.z = std::atan2(mRot[i][j], -mRot[i][k]);
			}
			else
			{
				eulerAngles.x = std::atan2(-mRot[k][j], mRot[j][j]);
				eulerAngles.y = static_cast<float>(std::atan2(sy, static_cast<double>(mRot[i][i])));
				eulerAngles.z = 0.0f;
			}
		}
		else
		{
			const double cy = std::sqrt(mRot[i][i] * mRot[i][i] + mRot[i][j] * mRot[i][j]);
			if (cy > 16 * floatEpsilon)
			{
				eulerAngles.x = std::atan2(mRot[j][k], mRot[k][k]);
				eulerAngles.y = static_cast<float>(std::atan2(static_cast<double>(-mRot[i][k]), cy));
				eulerAngles.z = std::atan2(mRot[i][j], mRot[i][i]);
			}
			else
			{
				eulerAngles.x = std::atan2(-mRot[k][j], mRot[j][j]);
				eulerAngles.y = static_cast<float>(std::atan2(static_cast<double>(-mRot[i][k]), cy));
				eulerAngles.z = 0.0f;
			}
		}

		if (parityOdd)
		{
			eulerAngles.x = -eulerAngles.x;
			eulerAngles.y = -eulerAngles.y;
			eulerAngles.z = -eulerAngles.z;
		}
		if (rotating)
		{
			const float t = eulerAngles.x;
			eulerAngles.x = eulerAngles.z;
			eulerAngles.z = t;
		}
		return eulerAngles;
	}

	glm::vec3 EulerAngles::matrixToEuler(const glm::mat3x4& mRot, Order order)
	{
		glm::vec3 eulerAngles;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (repetition)
		{
			const double sy = std::sqrt(mRot[j][i] * mRot[j][i] + mRot[k][i] * mRot[k][i]);
			if (sy > 16 * floatEpsilon)
			{
				eulerAngles.x = std::atan2(mRot[j][i], mRot[k][i]);
				eulerAngles.y = static_cast<float>(std::atan2(sy, static_cast<double>(mRot[i][i])));
				eulerAngles.z = std::atan2(mRot[i][j], -mRot[i][k]);
			}
			else
			{
				eulerAngles.x = std::atan2(-mRot[k][j], mRot[j][j]);
				eulerAngles.y = static_cast<float>(std::atan2(sy, static_cast<double>(mRot[i][i])));
				eulerAngles.z = 0.0f;
			}
		}
		else
		{
			const double cy = std::sqrt(mRot[i][i] * mRot[i][i] + mRot[i][j] * mRot[i][j]);
			if (cy > 16*floatEpsilon)
			{
				eulerAngles.x = std::atan2(mRot[j][k], mRot[k][k]);
				eulerAngles.y = static_cast<float>(std::atan2(static_cast<double>(-mRot[i][k]), cy));
				eulerAngles.z = std::atan2(mRot[i][j], mRot[i][i]);
			}
			else
			{
				eulerAngles.x = std::atan2(-mRot[k][j], mRot[j][j]);
				eulerAngles.y = static_cast<float>(std::atan2(static_cast<double>(-mRot[i][k]), cy));
				eulerAngles.z = 0.0f;
			}
		}

		if (parityOdd)
		{
			eulerAngles.x = -eulerAngles.x;
			eulerAngles.y = -eulerAngles.y;
			eulerAngles.z = -eulerAngles.z;
		}
		if (rotating)
		{
			const float t = eulerAngles.x;
			eulerAngles.x = eulerAngles.z;
			eulerAngles.z = t;
		}
		return eulerAngles;
	}

	glm::vec3 EulerAngles::matrixToEuler(const glm::mat4& mRot, Order order)
	{
		glm::vec3 eulerAngles;

		int i, j, k;
		bool parityOdd, repetition, rotating;
		EULER_ANGLES_INIT_ORDER(order, i, j, k, parityOdd, repetition, rotating);

		if (repetition)
		{
			const double sy = std::sqrt(mRot[j][i] * mRot[j][i] + mRot[k][i] * mRot[k][i]);
			if (sy > 16 * floatEpsilon)
			{
				eulerAngles.x = std::atan2(mRot[j][i], mRot[k][i]);
				eulerAngles.y = static_cast<float>(std::atan2(sy, static_cast<double>(mRot[i][i])));
				eulerAngles.z = std::atan2(mRot[i][j], -mRot[i][k]);
			}
			else
			{
				eulerAngles.x = std::atan2(-mRot[k][j], mRot[j][j]);
				eulerAngles.y = static_cast<float>(std::atan2(sy, static_cast<double>(mRot[i][i])));
				eulerAngles.z = 0.0f;
			}
		}
		else
		{
			const double cy = std::sqrt(mRot[i][i]*mRot[i][i] + mRot[i][j]*mRot[i][j]);
			if (cy > 16 * floatEpsilon)
			{
				eulerAngles.x = std::atan2(mRot[j][k], mRot[k][k]);
				eulerAngles.y = static_cast<float>(std::atan2(static_cast<double>(-mRot[i][k]), cy));
				eulerAngles.z = std::atan2(mRot[i][j], mRot[i][i]);
			}
			else
			{
				eulerAngles.x = std::atan2(-mRot[k][j], mRot[j][j]);
				eulerAngles.y = static_cast<float>(std::atan2(static_cast<double>(-mRot[i][k]), cy));
				eulerAngles.z = 0.0f;
			}
		}

		if (parityOdd)
		{
			eulerAngles.x = -eulerAngles.x;
			eulerAngles.y = -eulerAngles.y;
			eulerAngles.z = -eulerAngles.z;
		}
		if (rotating)
		{
			const float t = eulerAngles.x;
			eulerAngles.x = eulerAngles.z;
			eulerAngles.z = t;
		}
		return eulerAngles;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
