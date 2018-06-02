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
#include "RendererRuntime/Resource/Scene/Item/Sky/HosekWilkieSky.h"
#include "RendererRuntime/Core/Math/Math.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		#include "RendererRuntime/Resource/Scene/Item/Sky/ArHosekSkyModelData_RGB.h"


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		double evaluateSpline(const double* spline, size_t stride, double value)
		{
			return
				1 *  std::pow(1 - value, 5) *                      spline[0 * stride] +
				5 *  std::pow(1 - value, 4) * std::pow(value, 1) * spline[1 * stride] +
				10 * std::pow(1 - value, 3) * std::pow(value, 2) * spline[2 * stride] +
				10 * std::pow(1 - value, 2) * std::pow(value, 3) * spline[3 * stride] +
				5 *  std::pow(1 - value, 1) * std::pow(value, 4) * spline[4 * stride] +
				1 *                           std::pow(value, 5) * spline[5 * stride];
		}

		float evaluate(const double* dataset, size_t stride, float turbidity, float albedo, float sunTheta)
		{
			// Splines are functions of elevation^1/3
			const double elevationK = std::pow(std::max<float>(0.0f, 1.0f - sunTheta / (glm::pi<float>() / 2.0f)), 1.0f / 3.0f);

			// Table has values for turbidity 1..10
			const int turbidity0 = glm::clamp<int>(static_cast<int>(turbidity), 1, 10);
			const int turbidity1 = std::min(turbidity0 + 1, 10);
			const float turbidityK = glm::clamp(turbidity - turbidity0, 0.0f, 1.0f);

			const double * datasetA0 = dataset;
			const double * datasetA1 = dataset + stride * 6 * 10;

			const double a0t0 = evaluateSpline(datasetA0 + stride * 6 * (turbidity0 - 1), stride, elevationK);
			const double a1t0 = evaluateSpline(datasetA1 + stride * 6 * (turbidity0 - 1), stride, elevationK);
			const double a0t1 = evaluateSpline(datasetA0 + stride * 6 * (turbidity1 - 1), stride, elevationK);
			const double a1t1 = evaluateSpline(datasetA1 + stride * 6 * (turbidity1 - 1), stride, elevationK);

			return static_cast<float>(a0t0 * (1 - albedo) * (1 - turbidityK) + a1t0 * albedo * (1 - turbidityK) + a0t1 * (1 - albedo) * turbidityK + a1t1 * albedo * turbidityK);
		}

		glm::vec3 hosekWilkie(float cos_theta, float gamma, float cos_gamma, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, const glm::vec3& D, const glm::vec3& E, const glm::vec3& F, const glm::vec3& G, const glm::vec3& H, const glm::vec3& I)
		{
			const glm::vec3 chi = (1.0f + cos_gamma * cos_gamma) / glm::pow(1.0f + H * H - 2.0f * cos_gamma * H, glm::vec3(1.5f));
			return (1.0f + A * glm::exp(B / (cos_theta + 0.01f))) * (C + D * glm::exp(E * gamma) + F * (cos_gamma * cos_gamma) + G * chi + I * static_cast<float>(std::sqrt(std::max(0.0f, cos_theta))));
		}

		void compute(const glm::vec3& worldSpaceSunDirection, float turbidity, float albedo, float normalizedSunY, RendererRuntime::HosekWilkieSky::Coefficients& coefficients)
		{
			const float sunTheta = std::acos(glm::clamp(worldSpaceSunDirection.y, 0.0f, 1.0f));
			glm::vec3& A = coefficients.A;
			glm::vec3& B = coefficients.B;
			glm::vec3& C = coefficients.C;
			glm::vec3& D = coefficients.D;
			glm::vec3& E = coefficients.E;
			glm::vec3& F = coefficients.F;
			glm::vec3& G = coefficients.G;
			glm::vec3& H = coefficients.H;
			glm::vec3& I = coefficients.I;
			glm::vec3& Z = coefficients.Z;

			for (int i = 0; i < 3; ++i)
			{
				A[i] = evaluate(datasetsRGB[i] + 0, 9, turbidity, albedo, sunTheta);
				B[i] = evaluate(datasetsRGB[i] + 1, 9, turbidity, albedo, sunTheta);
				C[i] = evaluate(datasetsRGB[i] + 2, 9, turbidity, albedo, sunTheta);
				D[i] = evaluate(datasetsRGB[i] + 3, 9, turbidity, albedo, sunTheta);
				E[i] = evaluate(datasetsRGB[i] + 4, 9, turbidity, albedo, sunTheta);
				F[i] = evaluate(datasetsRGB[i] + 5, 9, turbidity, albedo, sunTheta);
				G[i] = evaluate(datasetsRGB[i] + 6, 9, turbidity, albedo, sunTheta);

				// Swapped in the dataset
				H[i] = evaluate(datasetsRGB[i] + 8, 9, turbidity, albedo, sunTheta);
				I[i] = evaluate(datasetsRGB[i] + 7, 9, turbidity, albedo, sunTheta);

				Z[i] = evaluate(datasetsRGBRad[i], 1, turbidity, albedo, sunTheta);
			}

			if (normalizedSunY)
			{
				const glm::vec3 S = hosekWilkie(std::cos(sunTheta), 0, 1.0f, A, B, C, D, E, F, G, H, I) * Z;
				Z /= glm::dot(S, glm::vec3(0.2126f, 0.7152f, 0.0722f));
				Z *= normalizedSunY;
			}
		}

		/**
		*  @brief
		*    Implementation of Peter Shirley's method for mapping from a unit square to a unit circle
		*
		*  @note
		*    - The implementation is basing on "Solar Radiance Calculation" - https://www.gamedev.net/topic/671214-simple-solar-radiance-calculation/
		*/
		glm::vec2 squareToConcentricDiskMapping(float x, float y)
		{
			float phi = 0.0f;
			float r = 0.0f;

			// -- (a,b) is now on [-1,1]ˆ2
			const float a = 2.0f * x - 1.0f;
			const float b = 2.0f * y - 1.0f;
			if (a > -b)		// Region 1 or 2
			{
				if (a > b)	// Region 1, also |a| > |b|
				{
					r = a;
					phi = (glm::pi<float>() * 0.25f) * (b / a);
				}
				else		// Region 2, also |b| > |a|
				{
					r = b;
					phi = (glm::pi<float>() * 0.25f) * (2.0f - (a / b));
				}
			}
			else			// region 3 or 4
			{
				if (a < b)	// Region 3, also |a| >= |b|, a != 0
				{
					r = -a;
					phi = (glm::pi<float>() * 0.25f) * (4.0f + (b / a));
				}
				else		// Region 4, |b| >= |a|, but a==0 and b==0 could occur.
				{
					r = -b;
					if (b != 0.0f)
					{
						phi = (glm::pi<float>() * 0.25f) * (6.0f - (a / b));
					}
					else
					{
						phi = 0.0f;
					}
				}
			}

			// Done
			return glm::vec2(r * std::cos(phi), r * std::sin(phi));
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	HosekWilkieSky::HosekWilkieSky() :
		mWorldSpaceSunDirection(Math::VEC3_UNIT_Z),
		mTurbidity(0.0f),
		mAlbedo(0.0f),
		mNormalizedSunY(0.0f),
		mCoefficients{},
		mSunColor(Math::VEC3_ONE)
	{
		// Nothing here
	}

	void HosekWilkieSky::recalculate(const glm::vec3& worldSpaceSunDirection, float turbidity, float albedo, float normalizedSunY)
	{
		if (mWorldSpaceSunDirection != worldSpaceSunDirection || mTurbidity != turbidity || mAlbedo != albedo || mNormalizedSunY != normalizedSunY)
		{
			mWorldSpaceSunDirection = worldSpaceSunDirection;
			mTurbidity = turbidity;
			mAlbedo = albedo;
			mNormalizedSunY = normalizedSunY;
			::detail::compute(mWorldSpaceSunDirection, mTurbidity, mAlbedo, mNormalizedSunY, mCoefficients);

			// Approximation of the sun color
			// TODO(co) This is a most simple hack, evaluate more accurate solutions like "Solar Radiance Calculation" - https://www.gamedev.net/topic/671214-simple-solar-radiance-calculation/
			mSunColor = Math::VEC3_ZERO;
			if (worldSpaceSunDirection.y > 0.0f)
			{
				// The idea is basing on "Solar Radiance Calculation" - https://www.gamedev.net/topic/671214-simple-solar-radiance-calculation/
				const float thetaS = std::acos(1.0f - worldSpaceSunDirection.y);
				const float elevation = (glm::pi<float>() * 0.5f) - thetaS;
				const float sunSize = glm::radians(0.27f);	// Angular radius of the sun from Earth
				static constexpr int NUMBER_OF_DISC_SAMPLES = 8;
				for (int x = 0; x < NUMBER_OF_DISC_SAMPLES; ++x)
				{
					for (int y = 0; y < NUMBER_OF_DISC_SAMPLES; ++y)
					{
						const float u = (x + 0.5f) / NUMBER_OF_DISC_SAMPLES;
						const float v = (y + 0.5f) / NUMBER_OF_DISC_SAMPLES;
						const glm::vec2 discSamplePos = ::detail::squareToConcentricDiskMapping(u, v);
						const float cos_theta = elevation + discSamplePos.y * sunSize;
						const float cos_gamma = discSamplePos.x * sunSize;
						const float gamma = acos(cos_gamma);
						mSunColor += ::detail::hosekWilkie(cos_theta, gamma, cos_gamma, mCoefficients.A, mCoefficients.B, mCoefficients.C, mCoefficients.D, mCoefficients.E, mCoefficients.F, mCoefficients.G, mCoefficients.H, mCoefficients.I);
					}
				}
				mSunColor /= NUMBER_OF_DISC_SAMPLES * NUMBER_OF_DISC_SAMPLES;
				mSunColor = glm::max(mSunColor, Math::VEC3_ZERO);

				// Some visual adjustments so the simple approximation doesn't look that wrong
				mSunColor = glm::vec3(mSunColor.b, mSunColor.g, mSunColor.r) * 0.75f;
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
