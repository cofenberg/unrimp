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


// Not compatible with "Advanced Vector Extensions 2" (/arch:AVX2)


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Resource/Scene/Culling/SceneCullingManager.h"
#include "RendererRuntime/Resource/Scene/Culling/SceneItemSet.h"
#include "RendererRuntime/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "RendererRuntime/Resource/Scene/SceneNode.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorContextData.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "RendererRuntime/RenderQueue/RenderableManager.h"
#include "RendererRuntime/Core/Thread/ThreadPool.h"
#include "RendererRuntime/Core/Math/Math.h"
#include "RendererRuntime/Core/Math/Frustum.h"
#include "RendererRuntime/Vr/IVrManager.h"
#include "RendererRuntime/IRendererRuntime.h"

#include <Renderer/Renderer.h>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr size_t SCENE_ITEMS_SPLIT_COUNT = 256;	///< Package size for each thread to work on	TODO(co) This value needs to be fine-tuned
		typedef xsimd::batch_bool<float, 4> bool4;
		typedef xsimd::simd_type<float> float4;
		static const float4 FLOAT4_ALL_ZERO(0.0f);
		static const bool4 BOOL4_ALL_FALSE(false);
		static const bool4 BOOL4_ALL_TRUE(true);
		struct SimdPlane final
		{
			float4 normalX;	///< The normal's x value replicated 4 times
			float4 normalY;	///< The normal's y value replicated 4 times
			float4 normalZ;	///< etc.
			float4 d;
		};
		struct SimdVector final
		{
			float4 x;	///< Stores x0, x1, x2, x3
			float4 y;	///< Stores y0, y1, y2, y3
			float4 z;	///< etc.
			float4 w;
		};
		struct SimdMatrix final
		{
			SimdVector x;
			SimdVector y;
			SimdVector z;
			SimdVector w;
		};


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		uint32_t alignToSimdLaneCount(uint32_t value)
		{
			return RendererRuntime::Math::makeMultipleOf(value, xsimd::simd_type<float>::size);
		}

		uint32_t removeNotVisible(const RendererRuntime::SceneItemSet& sceneItemSet, uint32_t count, const uint32_t* inputIndirection, uint32_t* outputIndirection)
		{
			const uint32_t* RESTRICT visibilityFlag = sceneItemSet.visibilityFlag.data();
			uint32_t numberOfVisibleItems = 0u;
			if (nullptr != inputIndirection)
			{
				for (uint32_t i = 0; i < count; ++i)
				{
					const uint32_t index = inputIndirection[i];
					if (visibilityFlag[index])
					{
						outputIndirection[numberOfVisibleItems] = index;
						++numberOfVisibleItems;
					}
				}
			}
			else
			{
				for (uint32_t i = 0; i < count; ++i)
				{
					if (visibilityFlag[i])
					{
						outputIndirection[numberOfVisibleItems] = i;
						++numberOfVisibleItems;
					}
				}
			}

			// Pad out to the SIMD alignment
			const uint32_t numberOfVisibleItemsAligned = alignToSimdLaneCount(numberOfVisibleItems);
			const uint32_t lastVisibleItem = numberOfVisibleItems ? outputIndirection[numberOfVisibleItems - 1] : 0;
			for (uint32_t i = numberOfVisibleItems; i < numberOfVisibleItemsAligned; ++i)
			{
				outputIndirection[i] = lastVisibleItem;
			}

			return numberOfVisibleItems;
		}

		SimdVector simdMultiply(const SimdVector& v, const SimdMatrix& m)
		{
			float4 x = v.x * m.x.x;     x = v.y * m.y.x + x;    x = v.z * m.z.x + x;    x = v.w * m.w.x + x;
			float4 y = v.x * m.x.y;     y = v.y * m.y.y + y;    y = v.z * m.z.y + y;    y = v.w * m.w.y + y;
			float4 z = v.x * m.x.z;     z = v.y * m.y.z + z;    z = v.z * m.z.z + z;    z = v.w * m.w.z + z;
			float4 w = v.x * m.x.w;     w = v.y * m.y.w + w;    w = v.z * m.z.w + w;    w = v.w * m.w.w + w;
			return { x, y, z, w };
		}

		SimdMatrix simdMultiply(const SimdMatrix& lhs, const SimdMatrix& rhs)
		{
			const SimdVector x = simdMultiply(lhs.x, rhs);
			const SimdVector y = simdMultiply(lhs.y, rhs);
			const SimdVector z = simdMultiply(lhs.z, rhs);
			const SimdVector w = simdMultiply(lhs.w, rhs);
			return { x, y, z, w };
		}

		void simdMinimumMaximumTransform(const SimdMatrix& m, const SimdVector& minimum, const SimdVector& maximum, SimdVector result[])
		{
			float4 m_xx_x = m.x.x * minimum.x;    m_xx_x = m_xx_x + m.w.x;
			float4 m_xy_x = m.x.y * minimum.x;    m_xy_x = m_xy_x + m.w.y;
			float4 m_xz_x = m.x.z * minimum.x;    m_xz_x = m_xz_x + m.w.z;
			float4 m_xw_x = m.x.w * minimum.x;    m_xw_x = m_xw_x + m.w.w;

			float4 m_xx_X = m.x.x * maximum.x;    m_xx_X = m_xx_X + m.w.x;
			float4 m_xy_X = m.x.y * maximum.x;    m_xy_X = m_xy_X + m.w.y;
			float4 m_xz_X = m.x.z * maximum.x;    m_xz_X = m_xz_X + m.w.z;
			float4 m_xw_X = m.x.w * maximum.x;    m_xw_X = m_xw_X + m.w.w;

			float4 m_yx_y = m.y.x * minimum.y;
			float4 m_yy_y = m.y.y * minimum.y;
			float4 m_yz_y = m.y.z * minimum.y;
			float4 m_yw_y = m.y.w * minimum.y;

			float4 m_yx_Y = m.y.x * maximum.y;
			float4 m_yy_Y = m.y.y * maximum.y;
			float4 m_yz_Y = m.y.z * maximum.y;
			float4 m_yw_Y = m.y.w * maximum.y;

			float4 m_zx_z = m.z.x * minimum.z;
			float4 m_zy_z = m.z.y * minimum.z;
			float4 m_zz_z = m.z.z * minimum.z;
			float4 m_zw_z = m.z.w * minimum.z;

			float4 m_zx_Z = m.z.x * maximum.z;
			float4 m_zy_Z = m.z.y * maximum.z;
			float4 m_zz_Z = m.z.z * maximum.z;
			float4 m_zw_Z = m.z.w * maximum.z;

			{
				float4 xyz_x = m_xx_x + m_yx_y;   xyz_x = xyz_x + m_zx_z;
				float4 xyz_y = m_xy_x + m_yy_y;   xyz_y = xyz_y + m_zy_z;
				float4 xyz_z = m_xz_x + m_yz_y;   xyz_z = xyz_z + m_zz_z;
				float4 xyz_w = m_xw_x + m_yw_y;   xyz_w = xyz_w + m_zw_z;
				result[0].x = xyz_x;
				result[0].y = xyz_y;
				result[0].z = xyz_z;
				result[0].w = xyz_w;
			}

			{
				float4 Xyz_x = m_xx_X + m_yx_y;   Xyz_x = Xyz_x + m_zx_z;
				float4 Xyz_y = m_xy_X + m_yy_y;   Xyz_y = Xyz_y + m_zy_z;
				float4 Xyz_z = m_xz_X + m_yz_y;   Xyz_z = Xyz_z + m_zz_z;
				float4 Xyz_w = m_xw_X + m_yw_y;   Xyz_w = Xyz_w + m_zw_z;
				result[1].x = Xyz_x;
				result[1].y = Xyz_y;
				result[1].z = Xyz_z;
				result[1].w = Xyz_w;
			}

			{
				float4 xYz_x = m_xx_x + m_yx_Y;   xYz_x = xYz_x + m_zx_z;
				float4 xYz_y = m_xy_x + m_yy_Y;   xYz_y = xYz_y + m_zy_z;
				float4 xYz_z = m_xz_x + m_yz_Y;   xYz_z = xYz_z + m_zz_z;
				float4 xYz_w = m_xw_x + m_yw_Y;   xYz_w = xYz_w + m_zw_z;
				result[2].x = xYz_x;
				result[2].y = xYz_y;
				result[2].z = xYz_z;
				result[2].w = xYz_w;
			}

			{
				float4 XYz_x = m_xx_X + m_yx_Y;   XYz_x = XYz_x + m_zx_z;
				float4 XYz_y = m_xy_X + m_yy_Y;   XYz_y = XYz_y + m_zy_z;
				float4 XYz_z = m_xz_X + m_yz_Y;   XYz_z = XYz_z + m_zz_z;
				float4 XYz_w = m_xw_X + m_yw_Y;   XYz_w = XYz_w + m_zw_z;
				result[3].x = XYz_x;
				result[3].y = XYz_y;
				result[3].z = XYz_z;
				result[3].w = XYz_w;
			}

			{
				float4 xyZ_x = m_xx_x + m_yx_y;   xyZ_x = xyZ_x + m_zx_Z;
				float4 xyZ_y = m_xy_x + m_yy_y;   xyZ_y = xyZ_y + m_zy_Z;
				float4 xyZ_z = m_xz_x + m_yz_y;   xyZ_z = xyZ_z + m_zz_Z;
				float4 xyZ_w = m_xw_x + m_yw_y;   xyZ_w = xyZ_w + m_zw_Z;
				result[4].x = xyZ_x;
				result[4].y = xyZ_y;
				result[4].z = xyZ_z;
				result[4].w = xyZ_w;
			}

			{
				float4 XyZ_x = m_xx_X + m_yx_y;   XyZ_x = XyZ_x + m_zx_Z;
				float4 XyZ_y = m_xy_X + m_yy_y;   XyZ_y = XyZ_y + m_zy_Z;
				float4 XyZ_z = m_xz_X + m_yz_y;   XyZ_z = XyZ_z + m_zz_Z;
				float4 XyZ_w = m_xw_X + m_yw_y;   XyZ_w = XyZ_w + m_zw_Z;
				result[5].x = XyZ_x;
				result[5].y = XyZ_y;
				result[5].z = XyZ_z;
				result[5].w = XyZ_w;
			}

			{
				float4 xYZ_x = m_xx_x + m_yx_Y;   xYZ_x = xYZ_x + m_zx_Z;
				float4 xYZ_y = m_xy_x + m_yy_Y;   xYZ_y = xYZ_y + m_zy_Z;
				float4 xYZ_z = m_xz_x + m_yz_Y;   xYZ_z = xYZ_z + m_zz_Z;
				float4 xYZ_w = m_xw_x + m_yw_Y;   xYZ_w = xYZ_w + m_zw_Z;
				result[6].x = xYZ_x;
				result[6].y = xYZ_y;
				result[6].z = xYZ_z;
				result[6].w = xYZ_w;
			}

			{
				float4 XYZ_x = m_xx_X + m_yx_Y;   XYZ_x = XYZ_x + m_zx_Z;
				float4 XYZ_y = m_xy_X + m_yy_Y;   XYZ_y = XYZ_y + m_zy_Z;
				float4 XYZ_z = m_xz_X + m_yz_Y;   XYZ_z = XYZ_z + m_zz_Z;
				float4 XYZ_w = m_xw_X + m_yw_Y;   XYZ_w = XYZ_w + m_zw_Z;
				result[7].x = XYZ_x;
				result[7].y = XYZ_y;
				result[7].z = XYZ_z;
				result[7].w = XYZ_w;
			}
		}

		FORCEINLINE void gatherRenderQueueIndexRangesRenderableManagersBySceneItem(const RendererRuntime::ISceneItem& sceneItem, const glm::vec3& cameraPosition, RendererRuntime::CompositorWorkspaceInstance::RenderQueueIndexRanges& renderQueueIndexRanges)
		{
			RendererRuntime::RenderableManager* renderableManager = const_cast<RendererRuntime::RenderableManager*>(sceneItem.getRenderableManager());	// TODO(co) Get rid of the evil const-cast
			if (nullptr != renderableManager && renderableManager->isVisible() && !renderableManager->getRenderables().empty())
			{
				// Calculate the distance to the camera
				renderableManager->setCachedDistanceToCamera(glm::distance(cameraPosition, sceneItem.getParentSceneNodeSafe().getGlobalTransform().position));

				// A renderable manager can be inside multiple render queue index ranges
				for (RendererRuntime::CompositorWorkspaceInstance::RenderQueueIndexRange& renderQueueIndexRange : renderQueueIndexRanges)
				{
					const uint8_t minimumRenderQueueIndex = renderableManager->getMinimumRenderQueueIndex();
					const uint8_t maximumRenderQueueIndex = renderableManager->getMaximumRenderQueueIndex();
					if ((minimumRenderQueueIndex >= renderQueueIndexRange.minimumRenderQueueIndex && minimumRenderQueueIndex <= renderQueueIndexRange.maximumRenderQueueIndex) ||
						(maximumRenderQueueIndex >= renderQueueIndexRange.minimumRenderQueueIndex && maximumRenderQueueIndex <= renderQueueIndexRange.maximumRenderQueueIndex))
					{
						renderQueueIndexRange.renderableManagers.push_back(renderableManager);
					}
				}
			}
		}


		//[-------------------------------------------------------]
		//[ Global thread functions                               ]
		//[-------------------------------------------------------]
		void simdSphereCulling(const SimdPlane planes[6], const RendererRuntime::SceneItemSet& sceneItemSet, size_t threadSceneItemIndexStart, size_t threadSceneItemIndexEnd, uint32_t* RESTRICT visibilityFlag)
		{
			// Get pointers to the necessary members of the object set
			const float* RESTRICT spherePositionXData = sceneItemSet.spherePositionX.data();
			const float* RESTRICT spherePositionYData = sceneItemSet.spherePositionY.data();
			const float* RESTRICT spherePositionZData = sceneItemSet.spherePositionZ.data();
			const float* RESTRICT negativeRadiusData = sceneItemSet.negativeRadius.data();

			// Test each plane of the frustum against each sphere
			constexpr std::size_t simdSize = xsimd::simd_type<float>::size;
			for (size_t sceneItemIndex = threadSceneItemIndexStart; sceneItemIndex < threadSceneItemIndexEnd; sceneItemIndex += simdSize)
			{
				#if defined(XSIMD_X86_INSTR_SET_AVAILABLE)
				{ // Prefetch data for the next loop iteration in order to try to hide memory latency
					// TODO(co) Optimization: This has been added without profiling. As soon as there's enough data do profiling here.
					const size_t nextIndex = sceneItemIndex + simdSize;
					xsimd::prefetch(&spherePositionXData[nextIndex]);
					xsimd::prefetch(&spherePositionYData[nextIndex]);
					xsimd::prefetch(&spherePositionZData[nextIndex]);
					xsimd::prefetch(&negativeRadiusData[nextIndex]);
					xsimd::prefetch(&visibilityFlag[nextIndex]);
				}
				#endif

				// Get world space center position of bounding sphere
				const float4 spherePositionX = xsimd::load_aligned(&spherePositionXData[sceneItemIndex]);
				const float4 spherePositionY = xsimd::load_aligned(&spherePositionYData[sceneItemIndex]);
				const float4 spherePositionZ = xsimd::load_aligned(&spherePositionZData[sceneItemIndex]);

				// Get negative world space radius of bounding sphere
				const float4 negativeRadius = xsimd::load_aligned(&negativeRadiusData[sceneItemIndex]);

				bool4 inside = BOOL4_ALL_TRUE;
				for (uint32_t p = 0; p < 6; ++p)
				{
					const float4& RESTRICT n_x = planes[p].normalX;
					const float4& RESTRICT n_y = planes[p].normalY;
					const float4& RESTRICT n_z = planes[p].normalZ;
					const float4 n_dot_pos = (spherePositionX * n_x) + (spherePositionY * n_y) + (spherePositionZ * n_z);

					// "The Implementation of Frustum Culling in Stingray" - http://bitsquid.blogspot.de/2016/10/the-implementation-of-frustum-culling.html is using the following
					// float4 planeTestPoint = n_dot_pos + radius;
					// bool4 planeTest = (planeTestPoint >= planes[p].d);

					// "Frustum Culling" by Dion Picco - http://www.flipcode.com/archives/Frustum_Culling.shtml worked TODO(co) Figure out the difference
					const float4 planeTestPoint = n_dot_pos + planes[p].d;
					const bool4 planeTest = (planeTestPoint > negativeRadius);

					inside = (planeTest & inside);
				}

				// Store 0 for spheres that didn't intersect or ended up on the positive side of the frustum planes
				// -> Store 0xffffffff for spheres that are visible
				// TODO(co) No direct usage of "__m128"
				xsimd::store_aligned(reinterpret_cast<__m128*>(&visibilityFlag[sceneItemIndex]), inside);
			}
		}

		void simdOobbCulling(const SimdMatrix& viewSpaceToClipSpaceMatrix, const RendererRuntime::SceneItemSet& sceneItemSet, const uint32_t* RESTRICT indirection, size_t threadSceneItemIndexStart, size_t threadSceneItemIndexEnd, uint32_t* RESTRICT visibilityFlag)
		{
			// Get pointers to the necessary members of the object set

			// Get minimum object space bounding box corner position
			const float* RESTRICT minimumX = sceneItemSet.minimumX.data();
			const float* RESTRICT minimumY = sceneItemSet.minimumY.data();
			const float* RESTRICT minimumZ = sceneItemSet.minimumZ.data();

			// Get maximum object space bounding box corner position
			const float* RESTRICT maximumX = sceneItemSet.maximumX.data();
			const float* RESTRICT maximumY = sceneItemSet.maximumY.data();
			const float* RESTRICT maximumZ = sceneItemSet.maximumZ.data();

			// Get object space to world space matrix
			const float* RESTRICT worldXX = sceneItemSet.worldXX.data();
			const float* RESTRICT worldXY = sceneItemSet.worldXY.data();
			const float* RESTRICT worldXZ = sceneItemSet.worldXZ.data();
			const float* RESTRICT worldXW = sceneItemSet.worldXW.data();
			const float* RESTRICT worldYX = sceneItemSet.worldYX.data();
			const float* RESTRICT worldYY = sceneItemSet.worldYY.data();
			const float* RESTRICT worldYZ = sceneItemSet.worldYZ.data();
			const float* RESTRICT worldYW = sceneItemSet.worldYW.data();
			const float* RESTRICT worldZX = sceneItemSet.worldZX.data();
			const float* RESTRICT worldZY = sceneItemSet.worldZY.data();
			const float* RESTRICT worldZZ = sceneItemSet.worldZZ.data();
			const float* RESTRICT worldZW = sceneItemSet.worldZW.data();
			const float* RESTRICT worldWX = sceneItemSet.worldWX.data();
			const float* RESTRICT worldWY = sceneItemSet.worldWY.data();
			const float* RESTRICT worldWZ = sceneItemSet.worldWZ.data();
			const float* RESTRICT worldWW = sceneItemSet.worldWW.data();

			constexpr std::size_t simdSize = xsimd::simd_type<float>::size;
			for (size_t sceneItemIndex = threadSceneItemIndexStart; sceneItemIndex < threadSceneItemIndexEnd; sceneItemIndex += simdSize)
			{
				// Load the world transform matrix for four objects via the indirection table
				const uint32_t i0 = indirection[sceneItemIndex];
				const uint32_t i1 = indirection[sceneItemIndex + 1];
				const uint32_t i2 = indirection[sceneItemIndex + 2];
				const uint32_t i3 = indirection[sceneItemIndex + 3];

				#if defined(XSIMD_X86_INSTR_SET_AVAILABLE)
				{ // Prefetch data for the next loop iteration in order to try to hide memory latency
					// TODO(co) Optimization: This has been added without profiling. As soon as there's enough data do profiling here.
					const size_t nextIndirectionIndex = sceneItemIndex + simdSize;
					for (size_t componentIndex = 0; componentIndex < 4; ++componentIndex)
					{
						const uint32_t nextIndex = indirection[nextIndirectionIndex + componentIndex];

						// Minimum object space bounding box corner position
						xsimd::prefetch(&minimumX[nextIndex]);
						xsimd::prefetch(&minimumY[nextIndex]);
						xsimd::prefetch(&minimumZ[nextIndex]);

						// Maximum object space bounding box corner position
						xsimd::prefetch(&maximumX[nextIndex]);
						xsimd::prefetch(&maximumY[nextIndex]);
						xsimd::prefetch(&maximumZ[nextIndex]);

						// Object space to world space matrix
						xsimd::prefetch(&worldXX[nextIndex]);
						xsimd::prefetch(&worldXY[nextIndex]);
						xsimd::prefetch(&worldXZ[nextIndex]);
						xsimd::prefetch(&worldXW[nextIndex]);
						xsimd::prefetch(&worldYX[nextIndex]);
						xsimd::prefetch(&worldYY[nextIndex]);
						xsimd::prefetch(&worldYZ[nextIndex]);
						xsimd::prefetch(&worldYW[nextIndex]);
						xsimd::prefetch(&worldZX[nextIndex]);
						xsimd::prefetch(&worldZY[nextIndex]);
						xsimd::prefetch(&worldZZ[nextIndex]);
						xsimd::prefetch(&worldZW[nextIndex]);
						xsimd::prefetch(&worldWX[nextIndex]);
						xsimd::prefetch(&worldWY[nextIndex]);
						xsimd::prefetch(&worldWZ[nextIndex]);
						xsimd::prefetch(&worldWW[nextIndex]);

						// Visibility flag
						xsimd::prefetch(&visibilityFlag[nextIndex]);
					}
				}
				#endif

				SimdMatrix world;
				world.x.x = float4(worldXX[i0], worldXX[i1], worldXX[i2], worldXX[i3]);
				world.x.y = float4(worldXY[i0], worldXY[i1], worldXY[i2], worldXY[i3]);
				world.x.z = float4(worldXZ[i0], worldXZ[i1], worldXZ[i2], worldXZ[i3]);
				world.x.w = float4(worldXW[i0], worldXW[i1], worldXW[i2], worldXW[i3]);

				world.y.x = float4(worldYX[i0], worldYX[i1], worldYX[i2], worldYX[i3]);
				world.y.y = float4(worldYY[i0], worldYY[i1], worldYY[i2], worldYY[i3]);
				world.y.z = float4(worldYZ[i0], worldYZ[i1], worldYZ[i2], worldYZ[i3]);
				world.y.w = float4(worldYW[i0], worldYW[i1], worldYW[i2], worldYW[i3]);

				world.z.x = float4(worldZX[i0], worldZX[i1], worldZX[i2], worldZX[i3]);
				world.z.y = float4(worldZY[i0], worldZY[i1], worldZY[i2], worldZY[i3]);
				world.z.z = float4(worldZZ[i0], worldZZ[i1], worldZZ[i2], worldZZ[i3]);
				world.z.w = float4(worldZW[i0], worldZW[i1], worldZW[i2], worldZW[i3]);

				world.w.x = float4(worldWX[i0], worldWX[i1], worldWX[i2], worldWX[i3]);
				world.w.y = float4(worldWY[i0], worldWY[i1], worldWY[i2], worldWY[i3]);
				world.w.z = float4(worldWZ[i0], worldWZ[i1], worldWZ[i2], worldWZ[i3]);
				world.w.w = float4(worldWW[i0], worldWW[i1], worldWW[i2], worldWW[i3]);

				// Create the matrix to go from object->world->view->clip space
				const SimdMatrix clip = simdMultiply(viewSpaceToClipSpaceMatrix, world);

				// Load the minimum and maximum corner positions of the bounding box in object space
				SimdVector minimumPosition;
				minimumPosition.x = float4(minimumX[i0], minimumX[i1], minimumX[i2], minimumX[i3]);
				minimumPosition.y = float4(minimumY[i0], minimumY[i1], minimumY[i2], minimumY[i3]);
				minimumPosition.z = float4(minimumZ[i0], minimumZ[i1], minimumZ[i2], minimumZ[i3]);
				minimumPosition.w = float4(1.0f);

				SimdVector maximumPosition;
				maximumPosition.x = float4(maximumX[i0], maximumX[i1], maximumX[i2], maximumX[i3]);
				maximumPosition.y = float4(maximumY[i0], maximumY[i1], maximumY[i2], maximumY[i3]);
				maximumPosition.z = float4(maximumZ[i0], maximumZ[i1], maximumZ[i2], maximumZ[i3]);
				maximumPosition.w = float4(1.0f);

				// Transform each bounding box corner from object to clip space by sharing calculations
				SimdVector clipPosition[8];
				simdMinimumMaximumTransform(clip, minimumPosition, maximumPosition, clipPosition);

				// Initialize test conditions
				bool4 allXLess = BOOL4_ALL_TRUE;
				bool4 allXGreater = BOOL4_ALL_TRUE;
				bool4 allYLess = BOOL4_ALL_TRUE;
				bool4 allYGreater = BOOL4_ALL_TRUE;
				bool4 allZLess = BOOL4_ALL_TRUE;
				bool4 anyZLess = BOOL4_ALL_FALSE;
				bool4 allZGreater = BOOL4_ALL_TRUE;

				// Test each corner of the OOBB and if any corner intersects the frustum that object is visible
				for (uint32_t cs = 0; cs < 8; ++cs)
				{
					const float4 neg_cs_w = -clipPosition[cs].w;

					const bool4 x_le = (clipPosition[cs].x <= neg_cs_w);
					const bool4 x_ge = (clipPosition[cs].x >= clipPosition[cs].w);
					allXLess = (x_le & allXLess);
					allXGreater = (x_ge & allXGreater);

					const bool4 y_le = (clipPosition[cs].y <= neg_cs_w);
					const bool4 y_ge = (clipPosition[cs].y >= clipPosition[cs].w);
					allYLess = (y_le & allYLess);
					allYGreater = (y_ge & allYGreater);

					const bool4 z_le = (clipPosition[cs].z <= FLOAT4_ALL_ZERO);
					const bool4 z_ge = (clipPosition[cs].z >= clipPosition[cs].w);
					allZLess = (z_le & allZLess);
					allZGreater = (z_ge & allZGreater);
					anyZLess = (z_le | anyZLess);
				}

				const bool4 anyXOutside = (allXLess | allXGreater);
				const bool4 anyYOutside = (allYLess | allYGreater);
				const bool4 anyZOutside = (allZLess | allZGreater);
				bool4 outside = (anyXOutside | anyYOutside);
				outside = (outside | anyZOutside);
				bool4 inside = (outside ^ BOOL4_ALL_TRUE);

				// TODO(co) Add "contribution culling" as mentioned at http://bitsquid.blogspot.de/2016/10/the-implementation-of-frustum-culling.html - "Conclusion"

				// Store the result in the "visibilityFlag"-array in a compacted way
				// TODO(co) No direct usage of "__m128"
				xsimd::store_aligned(reinterpret_cast<__m128*>(&visibilityFlag[sceneItemIndex]), inside);
			}
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
	SceneCullingManager::SceneCullingManager() :
		mCullableSceneItemSet(new SceneItemSet()),
		mCullableShadowCastersSceneItemSet(new SceneItemSet())
	{
		// Nothing here
	}

	SceneCullingManager::~SceneCullingManager()
	{
		delete mCullableSceneItemSet;
		delete mCullableShadowCastersSceneItemSet;
	}

	void SceneCullingManager::gatherRenderQueueIndexRangesRenderableManagers(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, CompositorWorkspaceInstance::RenderQueueIndexRanges& renderQueueIndexRanges)
	{
		// Overview over the basic workflow of "The Implementation of Frustum Culling in Stingray" - http://bitsquid.blogspot.de/2016/10/the-implementation-of-frustum-culling.html
		// - Kick jobs to do frustum vs sphere culling
		//   - For each frustum plane, test plane vs sphere
		// - Wait for sphere culling to finish
		// - For objects that pass sphere test, kick jobs to do frustum vs object-oriented bounding box (OOBB) culling
		//   - For each frustum plane, test plane vs OOBB
		// - Wait for OOBB culling to finish

		// Get the camera scene item
		const CameraSceneItem* cameraSceneItem = compositorContextData.getCameraSceneItem();
		assert(nullptr != cameraSceneItem);

		// Get view space to clip space matrix
		assert(nullptr != compositorContextData.getCompositorWorkspaceInstance());
		const IRendererRuntime& rendererRuntime = compositorContextData.getCompositorWorkspaceInstance()->getRendererRuntime();
		glm::mat4 viewSpaceToClipSpaceMatrix;
		{
			const IVrManager& vrManager = rendererRuntime.getVrManager();
			if (compositorContextData.getSinglePassStereoInstancing() && vrManager.isRunning() && !cameraSceneItem->hasCustomWorldSpaceToViewSpaceMatrix() && !cameraSceneItem->hasCustomViewSpaceToClipSpaceMatrix())
			{
				// TODO(co) There are currently multiple culling issues notable when using stereo rendering, so disabled culling for now until this has been resolved
				// Fill render queue index ranges with the visible stuff
				const glm::vec3& cameraPosition = cameraSceneItem->getParentSceneNodeSafe().getGlobalTransform().position;
				for (uint32_t i = 0; i < mCullableSceneItemSet->numberOfSceneItems; ++i)
				{
					::detail::gatherRenderQueueIndexRangesRenderableManagersBySceneItem(*mCullableSceneItemSet->sceneItemVector[i], cameraPosition, renderQueueIndexRanges);
				}
				// Fill render queue index ranges with the always-visible stuff
				for (const ISceneItem* sceneItem : mUncullableSceneItems)
				{
					::detail::gatherRenderQueueIndexRangesRenderableManagersBySceneItem(*sceneItem, cameraPosition, renderQueueIndexRanges);
				}
				return;

				// TODO(co) Single pass stereo rendering: "You must conservatively cull on the CPU by about 5 degrees": http://media.steampowered.com/apps/valve/2015/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
				// viewSpaceToClipSpaceMatrix = vrManager.getHmdViewSpaceToClipSpaceMatrix(IVrManager::VrEye::LEFT, cameraSceneItem->getNearZ(), cameraSceneItem->getFarZ());
			}
			else
			{
				// Get the render target with and height
				uint32_t renderTargetWidth = 0;
				uint32_t renderTargetHeight = 0;
				renderTarget.getWidthAndHeight(renderTargetWidth, renderTargetHeight);

				// Get view space to clip space matrix
				viewSpaceToClipSpaceMatrix = cameraSceneItem->getViewSpaceToClipSpaceMatrix(static_cast<float>(renderTargetWidth) / renderTargetHeight);
			}
		}

		// Calculate frustum using a world space to clip space matrix
		const Frustum frustum(viewSpaceToClipSpaceMatrix * cameraSceneItem->getWorldSpaceToViewSpaceMatrix());

		// Splat out the planes to be able to do plane-sphere test with SIMD
		const ::detail::SimdPlane planes[6] =
		{
			// Left clipping plane
			::detail::float4(frustum.planes[Frustum::PLANE_LEFT].normal.x),
			::detail::float4(frustum.planes[Frustum::PLANE_LEFT].normal.y),
			::detail::float4(frustum.planes[Frustum::PLANE_LEFT].normal.z),
			::detail::float4(frustum.planes[Frustum::PLANE_LEFT].d),

			// Right clipping plane
			::detail::float4(frustum.planes[Frustum::PLANE_RIGHT].normal.x),
			::detail::float4(frustum.planes[Frustum::PLANE_RIGHT].normal.y),
			::detail::float4(frustum.planes[Frustum::PLANE_RIGHT].normal.z),
			::detail::float4(frustum.planes[Frustum::PLANE_RIGHT].d),

			// Top clipping plane
			::detail::float4(frustum.planes[Frustum::PLANE_TOP].normal.x),
			::detail::float4(frustum.planes[Frustum::PLANE_TOP].normal.y),
			::detail::float4(frustum.planes[Frustum::PLANE_TOP].normal.z),
			::detail::float4(frustum.planes[Frustum::PLANE_TOP].d),

			// Bottom clipping plane
			::detail::float4(frustum.planes[Frustum::PLANE_BOTTOM].normal.x),
			::detail::float4(frustum.planes[Frustum::PLANE_BOTTOM].normal.y),
			::detail::float4(frustum.planes[Frustum::PLANE_BOTTOM].normal.z),
			::detail::float4(frustum.planes[Frustum::PLANE_BOTTOM].d),

			// Near clipping plane
			::detail::float4(frustum.planes[Frustum::PLANE_NEAR].normal.x),
			::detail::float4(frustum.planes[Frustum::PLANE_NEAR].normal.y),
			::detail::float4(frustum.planes[Frustum::PLANE_NEAR].normal.z),
			::detail::float4(frustum.planes[Frustum::PLANE_NEAR].d),

			// Far clipping plane
			::detail::float4(frustum.planes[Frustum::PLANE_FAR].normal.x),
			::detail::float4(frustum.planes[Frustum::PLANE_FAR].normal.y),
			::detail::float4(frustum.planes[Frustum::PLANE_FAR].normal.z),
			::detail::float4(frustum.planes[Frustum::PLANE_FAR].d),
		};

		// Make sure to align the size to the SIMD lane count
		const uint32_t n_aligned_objects = ::detail::alignToSimdLaneCount(mCullableSceneItemSet->numberOfSceneItems);

		// TODO(co) We need to ensure that scene item set fits the SIMD lane count, this is only done at this place for the culling kickoff
		if (mCullableSceneItemSet->minimumX.size() != n_aligned_objects)
		{
			// Determine the needed vector size which takes alignment as well as prefetch ("xsimd::prefetch()" -> "_mm_prefetch()") into account
			const uint32_t size = n_aligned_objects + xsimd::simd_type<float>::size;

			// Minimum object space bounding box corner position
			mCullableSceneItemSet->minimumX.resize(size);
			mCullableSceneItemSet->minimumY.resize(size);
			mCullableSceneItemSet->minimumZ.resize(size);

			// Maximum object space bounding box corner position
			mCullableSceneItemSet->maximumX.resize(size);
			mCullableSceneItemSet->maximumY.resize(size);
			mCullableSceneItemSet->maximumZ.resize(size);

			// Object space to world space matrix
			mCullableSceneItemSet->worldXX.resize(size);
			mCullableSceneItemSet->worldXY.resize(size);
			mCullableSceneItemSet->worldXZ.resize(size);
			mCullableSceneItemSet->worldXW.resize(size);
			mCullableSceneItemSet->worldYX.resize(size);
			mCullableSceneItemSet->worldYY.resize(size);
			mCullableSceneItemSet->worldYZ.resize(size);
			mCullableSceneItemSet->worldYW.resize(size);
			mCullableSceneItemSet->worldZX.resize(size);
			mCullableSceneItemSet->worldZY.resize(size);
			mCullableSceneItemSet->worldZZ.resize(size);
			mCullableSceneItemSet->worldZW.resize(size);
			mCullableSceneItemSet->worldWX.resize(size);
			mCullableSceneItemSet->worldWY.resize(size);
			mCullableSceneItemSet->worldWZ.resize(size);
			mCullableSceneItemSet->worldWW.resize(size);

			// World space center position of bounding sphere
			mCullableSceneItemSet->spherePositionX.resize(size);
			mCullableSceneItemSet->spherePositionY.resize(size);
			mCullableSceneItemSet->spherePositionZ.resize(size);

			// Negative world space radius of bounding sphere
			mCullableSceneItemSet->negativeRadius.resize(size);

			mCullableSceneItemSet->visibilityFlag.resize(size);
			mCullableSceneItemSet->sceneItemVector.resize(size);
		}

		// Get the thread pool instance
		DefaultThreadPool& defaultThreadPool = rendererRuntime.getDefaultThreadPool();

		{ // Do SIMD multi-threaded frustum-sphere culling
			size_t itemCount = mCullableSceneItemSet->numberOfSceneItems;
			size_t splitCount = ::detail::SCENE_ITEMS_SPLIT_COUNT;	// Package size for each thread to work on (will change when maximum number of threads is reached)
			const size_t threadCount = defaultThreadPool.getThreadCountAndSplitCount(itemCount, splitCount);
			if (1 == threadCount)
			{
				// Just execute it directly inside the current thread, not worth the additional threading effort
				::detail::simdSphereCulling(planes, *mCullableSceneItemSet, 0, itemCount, mCullableSceneItemSet->visibilityFlag.data());
			}
			else
			{
				// Multi-threaded
				size_t threadSceneItemIndexOffset = 0;
				for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
				{
					const size_t numberOfItemsToProcess = (threadIndex >= threadCount - 1) ? itemCount : splitCount;	// The last thread has to do all the rest of the remaining work
					defaultThreadPool.queueTask(std::bind(::detail::simdSphereCulling, planes, *mCullableSceneItemSet, threadSceneItemIndexOffset, threadSceneItemIndexOffset + numberOfItemsToProcess, mCullableSceneItemSet->visibilityFlag.data()));
					itemCount -= splitCount;
					threadSceneItemIndexOffset += splitCount;
				}

				// Wait that all worker threads have done their part of the calculation
				defaultThreadPool.process();
			}
		}

		// Store the indices of the objects that passed the frustum-sphere culling in the `indirection` array
		mIndirection.resize(n_aligned_objects);
		const uint32_t numberOfVisibleItems = ::detail::removeNotVisible(*mCullableSceneItemSet, mCullableSceneItemSet->numberOfSceneItems, nullptr, mIndirection.data());

		// Construct the SimdMatrix "simd_view_proj"
		const ::detail::SimdMatrix simd_view_proj =
		{
			::detail::float4(viewSpaceToClipSpaceMatrix[0][0]),
			::detail::float4(viewSpaceToClipSpaceMatrix[0][1]),
			::detail::float4(viewSpaceToClipSpaceMatrix[0][2]),
			::detail::float4(viewSpaceToClipSpaceMatrix[0][3]),

			::detail::float4(viewSpaceToClipSpaceMatrix[1][0]),
			::detail::float4(viewSpaceToClipSpaceMatrix[1][1]),
			::detail::float4(viewSpaceToClipSpaceMatrix[1][2]),
			::detail::float4(viewSpaceToClipSpaceMatrix[1][3]),

			::detail::float4(viewSpaceToClipSpaceMatrix[2][0]),
			::detail::float4(viewSpaceToClipSpaceMatrix[2][1]),
			::detail::float4(viewSpaceToClipSpaceMatrix[2][2]),
			::detail::float4(viewSpaceToClipSpaceMatrix[2][3]),

			::detail::float4(viewSpaceToClipSpaceMatrix[3][0]),
			::detail::float4(viewSpaceToClipSpaceMatrix[3][1]),
			::detail::float4(viewSpaceToClipSpaceMatrix[3][2]),
			::detail::float4(viewSpaceToClipSpaceMatrix[3][3]),
		};

		{ // Do SIMD multi-threaded frustum-OOBB culling
			size_t itemCount = numberOfVisibleItems;
			size_t splitCount = ::detail::SCENE_ITEMS_SPLIT_COUNT;	// Package size for each thread to work on (will change when maximum number of threads is reached)
			const size_t threadCount = defaultThreadPool.getThreadCountAndSplitCount(itemCount, splitCount);
			if (1 == threadCount)
			{
				// Just execute it directly inside the current thread, not worth the additional threading effort
				::detail::simdOobbCulling(simd_view_proj, *mCullableSceneItemSet, mIndirection.data(), 0, itemCount, mCullableSceneItemSet->visibilityFlag.data());
			}
			else
			{
				// Multi-threaded
				size_t threadSceneItemIndexOffset = 0;
				for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
				{
					const size_t numberOfItemsToProcess = (threadIndex >= threadCount - 1) ? itemCount : splitCount;	// The last thread has to do all the rest of the remaining work
					defaultThreadPool.queueTask(std::bind(::detail::simdOobbCulling, simd_view_proj, *mCullableSceneItemSet, mIndirection.data(), threadSceneItemIndexOffset, threadSceneItemIndexOffset + numberOfItemsToProcess, mCullableSceneItemSet->visibilityFlag.data()));
					itemCount -= splitCount;
					threadSceneItemIndexOffset += splitCount;
				}

				// Wait that all worker threads have done their part of the calculation
				defaultThreadPool.process();
			}
		}

		// Build up the indirection array that represents the objects that survived the frustum-oobb culling
		const uint32_t numberOfOobbVisible = ::detail::removeNotVisible(*mCullableSceneItemSet, numberOfVisibleItems, mIndirection.data(), mIndirection.data());

		// Fill render queue index ranges with the visible stuff
		const glm::vec3& cameraPosition = cameraSceneItem->getParentSceneNodeSafe().getGlobalTransform().position;
		for (uint32_t indirectionIndex = 0; indirectionIndex < numberOfOobbVisible; ++indirectionIndex)
		{
			::detail::gatherRenderQueueIndexRangesRenderableManagersBySceneItem(*mCullableSceneItemSet->sceneItemVector[mIndirection[indirectionIndex]], cameraPosition, renderQueueIndexRanges);
		}

		// Fill render queue index ranges with the always-visible stuff
		for (const ISceneItem* sceneItem : mUncullableSceneItems)
		{
			::detail::gatherRenderQueueIndexRangesRenderableManagersBySceneItem(*sceneItem, cameraPosition, renderQueueIndexRanges);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
