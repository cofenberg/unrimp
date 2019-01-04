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


Basing on
- Originally written by Christian Ofenberg as part of his master thesis \"Scalable Realtime Volume Rendering\" ( http://ablazespace.sourceforge.net/as-page/projects/MasterThesis/Ofenberg2012.pdf - in German)."
- The integration of the practical master thesis part into the PixelLight engine ( https://github.com/PixelLightFoundation/pixellight )
- There are YouTube videos available of the PixelLight engine volume rendering integration:
	- "CT-dataset volume rendering in the dungeon demo": https://www.youtube.com/watch?v=ngWkB1fJvlo
	- "Volume rendering of a pig 1": https://www.youtube.com/watch?v=glhgBvVRT6c
	- "Volume rendering of a pig 2": https://www.youtube.com/watch?v=nIlVz2nMz44
	- "Volume rendering of a pig 3": https://www.youtube.com/watch?v=b0uH-hqUOrk
	- "Volume rendering visible male": https://www.youtube.com/watch?v=axT0k1oytF0
	- "Volume rendering using a PixelLight team member": https://www.youtube.com/watch?v=o20uce3DGG8
	- "First volume rendering experiment": https://www.youtube.com/watch?v=oQ-knkBms8s
	- "First volume rendering experiment on Android": https://www.youtube.com/watch?v=CBY-bPVzc1U


The volume rendering shader system is split up into separate components.
- Inspired by the book "Real-Time Volume Graphics", section "1.6 Volume-Rendering Pipeline and Basic Approaches" (page 25)
- The following function signatures are using HLSL syntax
- "2.2 - Reconstruction" and "2.2 - Fetch Scalar" are both pipeline step 2.2 by intent because "reconstruction" can be seen as a high-order texture filtering
- "2.5 - Gradient" and "2.5 - Gradient Input" are both pipeline step 2.5 by intent, just decoupled to avoid an combinatorial explosion of hand-written shader building blocks


//[-------------------------------------------------------]
//[ 1.0 - Ray Setup                                       ]
//[-------------------------------------------------------]
- Main component responsible for
	- The ray setup
	- Calling the clip ray function (see "1.1 - Clip Ray")
	- Calling the jitter position function (see "1.2 - Jitter Position") for adding jittering to the start position of the ray in order to reduce wooden grain effect (moire pattern)
	- Calling the ray traversal function (see "2.0 - Ray Traversal")
	- Writing out the final fragment data


//[-------------------------------------------------------]
//[ 1.1 - Clip Ray                                        ]
//[-------------------------------------------------------]
- Signature of the clip ray function:
	/**
	*  @brief
	*    Performs an clipping operation on the given ray
	*
	*  @param[in, out] rayOrigin
	*    Start position of the ray inside the volume, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*  @param[in]      tayDirection
	*    Normalized ray direction
	*  @param[in, out] maximumTravelLength
	*    Maximum travel length along the ray, within the interval [0 .. 1]
	*/
	void ClipRay(inout float3 rayOrigin, float3 rayDirection, inout float maximumTravelLength)


//[-------------------------------------------------------]
//[ 1.2 - Jitter Position                                 ]
//[-------------------------------------------------------]
- Signature of the jitter position function:
	/**
	*  @brief
	*    Jitter position to the start position of the ray in order to reduce wooden grain effect (moire pattern)
	*
	*  @param[in] fragmentPosition
	*    Fragment position
	*  @param[in] position
	*    Position inside the volume to perform jitter on, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    Jitter factor, usually in the interval [0 .. 1]
	*/
	float JitterPosition(float2 fragmentPosition, float3 position)


//[-------------------------------------------------------]
//[ 2.0 - Ray Traversal                                   ]
//[-------------------------------------------------------]
- Signature of the ray traversal function:
	/**
	*  @brief
	*    Integrates over the volume
	*
	*  @param[in] material
	*    Material
	*  @param[in] startPosition
	*    Start position of the ray inside the volume, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*  @param[in] numberOfSteps
	*    Number of steps along the ray (= number of samples to take), must be positive
	*  @param[in] stepPositionDelta
	*    Position advance per step along the ray, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*  @param[in] maximumTravelLength
	*    Maximum travel length along the ray, within the interval [0 .. 1]
	*
	*  @return
	*    RGBA result of the ray traversal
	*/
	float4 RayTraversal(Material material, float3 startPosition, int numberOfSteps, float3 stepPositionDelta, float maximumTravelLength)
- Calling the clip position function (see "2.1 - Clip Position")
- Calling the reconstruction function (see "2.2 - Reconstruction")
- Calling the shading function (see "2.3 - Shading")


//[-------------------------------------------------------]
//[ 2.1 - Clip Position                                   ]
//[-------------------------------------------------------]
- Signature of the clip position function:
	/**
	*  @brief
	*    Performs an clipping operation on the given position
	*
	*  @param[in] position
	*    Position inside the volume to perform clipping on, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    'true' if the given position was clipped, else 'false' if the given position survived the clipping operation
	*/
	bool ClipPosition(float3 position)


//[-------------------------------------------------------]
//[ 2.2 - Reconstruction                                  ]
//[-------------------------------------------------------]
- Signature of the reconstruction function:
	/**
	*  @brief
	*    Reconstructs a scalar by using a given position inside the volume
	*
	*  @param[in] position
	*    Position inside the volume were to reconstruct the scalar, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    Reconstructed scalar, usually in the interval [0 .. 1]
	*/
	float Reconstruction(float3 position)
- Calling the fetch scalar function (see "2.2 - Fetch Scalar")


//[-------------------------------------------------------]
//[ 2.2 - Fetch Scalar                                    ]
//[-------------------------------------------------------]
- Signature of the fetch scalar function:
	/**
	*  @brief
	*    Fetches a scalar by using a given position inside the volume
	*
	*  @param[in] position
	*    Position inside the volume to fetch the scalar from, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    Scalar, usually in the interval [0 .. 1]
	*/
	float FetchScalar(float3 position)
- Signature of the fetch scalar function by using a texel offset:
	/**
	*  @brief
	*    Fetches a scalar by using a given position inside the volume by using a texel offset
	*
	*  @param[in] position
	*    Position inside the volume to fetch the scalar from, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*  @param[in] offset
	*    Texel offset
	*
	*  @return
	*    Scalar, usually in the interval [0 .. 1]
	*/
	float FetchScalarOffset(float3 position, int3 offset)


//[-------------------------------------------------------]
//[ 2.3 - Shading                                         ]
//[-------------------------------------------------------]
- Signature of the shading function:
	/**
	*  @brief
	*    Shading
	*
	*  @param[in] scalar
	*    Current voxel scalar
	*  @param[in] position
	*    Current position along the ray inside the volume, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*  @param[in] stepPositionDelta
	*    Position advance per step along the ray, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    RGBA result of the shading
	*/
	float4 Shading(float scalar, float3 position, float3 stepPositionDelta)
- Calling the classification function (see "2.4 - Classification")
- Calling the gradient function (see "2.5 - Gradient")
- Calling the illumination function (see "2.6 - Illumination")


//[-------------------------------------------------------]
//[ 2.4 - Classification                                  ]
//[-------------------------------------------------------]
- Signature of the classification function:
	/**
	*  @brief
	*    Scalar classification
	*
	*  @param[in] scalar
	*    Scalar to perform a classification on
	*
	*  @return
	*    RGBA result of the classification
	*/
	float4 Classification(float scalar)
- Calling the fetch scalar function (see "2.2 - Fetch Scalar")


//[-------------------------------------------------------]
//[ 2.5 - Gradient                                        ]
//[-------------------------------------------------------]
- Signature of the gradient function:
	/**
	*  @brief
	*    Gradient
	*
	*  @param[in] position
	*    Current position along the ray inside the volume, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    Gradient (not normalized)
	*/
	float3 Gradient(float3 position)
- Calling the gradient input function (see "2.5 - Gradient Input")


//[-------------------------------------------------------]
//[ 2.5 - Gradient Input                                  ]
//[-------------------------------------------------------]
- Signature of the gradient function:
	/**
	*  @brief
	*    Fetches the gradient input by using a given position inside the volume
	*
	*  @param[in] position
	*    Current position along the ray inside the volume, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*
	*  @return
	*    Gradient input
	*/
	float GradientInput(float3 position)
- Signature of the gradient input function by using a texel offset:
	/**
	*  @brief
	*    Fetches the gradient input by using a given position inside the volume by using a texel offset
	*
	*  @param[in] position
	*    Current position along the ray inside the volume, within the interval [(0, 0, 0) .. (1, 1, 1)]
	*  @param[in] offset
	*    Texel offset
	*
	*  @return
	*    Gradient input
	*/
	float GradientInputOffset(float3 position, int3 offset)


//[-------------------------------------------------------]
//[ 2.6 - Illumination                                    ]
//[-------------------------------------------------------]
- Signature of the illumination function:
	/**
	*  @brief
	*    Illumination
	*
	*  @param[in] surfaceColor
	*    Surface color
	*  @param[in] surfaceNormal
	*    Normalized surface normal
	*  @param[in] viewingDirection
	*    Normalized viewing direction
	*  @param[in] lightDirection
	*    Normalized light direction
	*
	*  @return
	*    Illumination result
	*/
	float3 Illumination(float3 surfaceColor, float3 surfaceNormal, float3 viewingDirection, float3 lightDirection)
