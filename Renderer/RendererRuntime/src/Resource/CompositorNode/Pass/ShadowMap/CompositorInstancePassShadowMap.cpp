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
#include "RendererRuntime/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/Quad/CompositorInstancePassQuad.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/Quad/CompositorResourcePassQuad.h"
#include "RendererRuntime/Resource/CompositorNode/CompositorNodeInstance.h"
#include "RendererRuntime/Resource/CompositorWorkspace/CompositorContextData.h"
#include "RendererRuntime/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "RendererRuntime/Resource/Scene/Item/Light/LightSceneItem.h"
#include "RendererRuntime/Resource/Scene/SceneNode.h"
#include "RendererRuntime/RenderQueue/RenderableManager.h"
#include "RendererRuntime/Core/Math/Math.h"
#include "RendererRuntime/IRendererRuntime.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP

#include <algorithm>


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
		static constexpr float	  SHADOW_MAP_FILTER_SIZE							  = 7.0f;
		static constexpr uint32_t DEPTH_SHADOW_MAP_TEXTURE_ASSET_ID					  = STRING_ID("Unrimp/Texture/DynamicByCode/DepthShadowMap");
		static constexpr uint32_t INTERMEDIATE_DEPTH_BLUR_SHADOW_MAP_TEXTURE_ASSET_ID = STRING_ID("Unrimp/Texture/DynamicByCode/IntermediateDepthBlurShadowMap");


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		glm::vec4 transformVectorByMatrix(const glm::mat4& matrix, const glm::vec4& vector)
		{
			const glm::vec4 temporaryVector = matrix * vector;
			return temporaryVector / temporaryVector.w;
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
	//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	void CompositorInstancePassShadowMap::onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer)
	{
		const CameraSceneItem* cameraSceneItem = compositorContextData.getCameraSceneItem();
		const LightSceneItem* lightSceneItem = compositorContextData.getLightSceneItem();
		if (nullptr != mDepthFramebufferPtr && nullptr != cameraSceneItem && cameraSceneItem->getParentSceneNode() && nullptr != lightSceneItem && lightSceneItem->getParentSceneNode())
		{
			const glm::vec3 worldSpaceSunlightDirection = lightSceneItem->getParentSceneNode()->getGlobalTransform().rotation * Math::VEC3_FORWARD;
			const CompositorResourcePassShadowMap& compositorResourcePassShadowMap = static_cast<const CompositorResourcePassShadowMap&>(getCompositorResourcePass());
			const uint32_t shadowMapSize = compositorResourcePassShadowMap.getShadowMapSize();
			mPassData.shadowMapSize = static_cast<int>(shadowMapSize);
			const uint8_t numberOfShadowCascades = compositorResourcePassShadowMap.getNumberOfShadowCascades();
			const float shadowFilterSize = compositorResourcePassShadowMap.getShadowFilterSize();

			// TODO(co) Stabilize cascades Reversed-Z update
			const bool stabilizeCascades = false;
//			const bool stabilizeCascades = compositorResourcePassShadowMap.getStabilizeCascades();

			// TODO(co) The minimum and maximum distance need to be calculated dynamically via depth buffer reduction as seen inside e.g. https://github.com/TheRealMJP/MSAAFilter/tree/master/MSAAFilter
			const float minimumDistance = 0.0f;
			const float maximumDistance = 0.5f;

			// Compute the split distances based on the partitioning mode
			float cascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			{
				const float nearClip = cameraSceneItem->getNearZ();
				const float farClip = cameraSceneItem->getFarZ();
				const float clipRange = farClip - nearClip;
				const float minimumZ = nearClip + minimumDistance * clipRange;
				const float maximumZ = nearClip + maximumDistance * clipRange;
				const float range = maximumZ - minimumZ;
				const float ratio = maximumZ / minimumZ;

				for (uint8_t cascadeIndex = 0; cascadeIndex < numberOfShadowCascades; ++cascadeIndex)
				{
					const float p = (cascadeIndex + 1) / static_cast<float>(numberOfShadowCascades);
					const float log = minimumZ * std::pow(ratio, p);
					const float uniform = minimumZ + range * p;
					const float d = compositorResourcePassShadowMap.getCascadeSplitsLambda() * (log - uniform) + uniform;
					cascadeSplits[cascadeIndex] = (d - nearClip) / clipRange;
				}
			}

			// Coordinate system related adjustments
			// -> Vulkan and Direct3D: Left-handed coordinate system with clip space depth value range 0..1
			// -> OpenGL without "GL_ARB_clip_control"-extension: Right-handed coordinate system with clip space depth value range -1..1
			const float nearZ = renderTarget.getRenderer().getCapabilities().zeroToOneClipZ ? 0.0f : -1.0f;

			// Get the 8 points of the view frustum in world space
			glm::vec4 worldSpaceFrustumCorners[8] =
			{
				// Near
				{-1.0f,  1.0f, nearZ, 1.0f},	// 0: Near top left
				{ 1.0f,  1.0f, nearZ, 1.0f},	// 1: Near top right
				{-1.0f, -1.0f, nearZ, 1.0f},	// 2: Near bottom left
				{ 1.0f, -1.0f, nearZ, 1.0f},	// 3: Near bottom right
				// Far
				{-1.0f,  1.0f, 1.0f, 1.0f},		// 4: Far top left
				{ 1.0f,  1.0f, 1.0f, 1.0f},		// 5: Far top right
				{-1.0f, -1.0f, 1.0f, 1.0f},		// 6: Far bottom left
				{ 1.0f, -1.0f, 1.0f, 1.0f}		// 7: Far bottom right
			};
			{
				uint32_t renderTargetWidth = 0;
				uint32_t renderTargetHeight = 0;
				renderTarget.getWidthAndHeight(renderTargetWidth, renderTargetHeight);
				const glm::mat4 worldSpaceToClipSpaceMatrix = cameraSceneItem->getViewSpaceToClipSpaceMatrix(static_cast<float>(renderTargetWidth) / renderTargetHeight) * cameraSceneItem->getWorldSpaceToViewSpaceMatrix();
				const glm::mat4 clipSpaceToWorldSpaceMatrix = glm::inverse(worldSpaceToClipSpaceMatrix);
				for (int i = 0; i < 8; ++i)
				{
					worldSpaceFrustumCorners[i] = ::detail::transformVectorByMatrix(clipSpaceToWorldSpaceMatrix, worldSpaceFrustumCorners[i]);
				}
			}

			// Begin debug event
			COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(commandBuffer)

			// Render the meshes to each cascade
			// -> Shadows should never be rendered via single pass stereo instancing
			const CompositorContextData shadowCompositorContextData(compositorContextData.getCompositorWorkspaceInstance(), compositorContextData.getCameraSceneItem(), false, compositorContextData.getLightSceneItem(), compositorContextData.getCompositorInstancePassShadowMap());
			for (uint8_t cascadeIndex = 0; cascadeIndex < numberOfShadowCascades; ++cascadeIndex)
			{
				COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, ("Shadow cascade " + std::to_string(cascadeIndex)).c_str())

				// Compute the MVP matrix from the light's point of view
				glm::mat4 depthProjectionMatrix;
				glm::mat4 depthViewMatrix;
				glm::vec3 minimumExtents;
				glm::vec3 maximumExtents;
				glm::vec3 cascadeExtents;
				const float splitDistance = cascadeSplits[cascadeIndex];
				{
					const float previousSplitDistance = (0 == cascadeIndex) ? minimumDistance : cascadeSplits[cascadeIndex - 1];

					// Get the corners of the current cascade slice of the view frustum
					glm::vec4 cascadeSliceWorldSpaceFrustumCorners[8];
					for (int i = 0; i < 4; ++i)
					{
						const glm::vec4 cornerRay = worldSpaceFrustumCorners[i + 4] - worldSpaceFrustumCorners[i];
						const glm::vec4 nearCornerRay = cornerRay * previousSplitDistance;
						const glm::vec4 farCornerRay = cornerRay * splitDistance;
						cascadeSliceWorldSpaceFrustumCorners[i + 4] = worldSpaceFrustumCorners[i] + farCornerRay;
						cascadeSliceWorldSpaceFrustumCorners[i] = worldSpaceFrustumCorners[i] + nearCornerRay;
					}

					// Calculate the centroid of the view frustum slice
					glm::vec4 temporaryFrustumCenter = Math::VEC4_ZERO;
					for (int i = 0; i < 8; ++i)
					{
						temporaryFrustumCenter += cascadeSliceWorldSpaceFrustumCorners[i];
					}
					const glm::vec3 frustumCenter = temporaryFrustumCenter / 8.0f;

					// Pick the right vector to use for the light camera, this needs to be constant for it to be stable
					const glm::vec3 rightDirection = stabilizeCascades ? Math::VEC3_RIGHT : (cameraSceneItem->getParentSceneNodeSafe().getTransform().rotation * Math::VEC3_RIGHT);

					// Calculate the minimum and maximum extents
					if (stabilizeCascades)
					{
						// Calculate the radius of a bounding sphere surrounding the frustum corners
						float sphereRadius = 0.0f;
						for (int i = 0; i < 8; ++i)
						{
							const float distance = glm::distance(glm::vec3(cascadeSliceWorldSpaceFrustumCorners[i]), frustumCenter);
							sphereRadius = std::max(sphereRadius, distance);
						}
						sphereRadius = std::ceil(sphereRadius * 16.0f) / 16.0f;
						maximumExtents = glm::vec3(sphereRadius, sphereRadius, sphereRadius);
						minimumExtents = -maximumExtents;
					}
					else
					{
						// Create a temporary view matrix for the light
						const glm::vec3& lightCameraPosition = frustumCenter;
						const glm::vec3 lightCameraTarget = frustumCenter - worldSpaceSunlightDirection;
						const glm::mat4 lightView = glm::lookAt(lightCameraPosition, lightCameraTarget, rightDirection);

						// Calculate an AABB around the frustum corners
						glm::vec4 mins(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
						glm::vec4 maxes(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
						for (int i = 0; i < 8; ++i)
						{
							const glm::vec4 corner = ::detail::transformVectorByMatrix(lightView, cascadeSliceWorldSpaceFrustumCorners[i]);
							mins = glm::min(mins, corner);
							maxes = glm::max(maxes, corner);
						}
						minimumExtents = mins;
						maximumExtents = maxes;

						// Adjust the minimum/maximum to accommodate the filtering size
						const float scale = (static_cast<float>(shadowMapSize) + ::detail::SHADOW_MAP_FILTER_SIZE) / static_cast<float>(shadowMapSize);
						minimumExtents.x *= scale;
						minimumExtents.y *= scale;
						maximumExtents.x *= scale;
						maximumExtents.x *= scale;
					}
					cascadeExtents = maximumExtents - minimumExtents;

					// Get position of the shadow camera
					const glm::vec3 shadowCameraPosition = frustumCenter + worldSpaceSunlightDirection * -minimumExtents.z;

					// Come up with a new orthographic camera for the shadow caster
					depthProjectionMatrix = glm::ortho(minimumExtents.x, maximumExtents.x, minimumExtents.y, maximumExtents.y, 0.0f, cascadeExtents.z);
					depthViewMatrix = glm::lookAt(shadowCameraPosition, frustumCenter, rightDirection);
				}

				// Create the rounding matrix, by projecting the world-space origin and determining the fractional offset in texel space
				glm::mat4 viewSpaceToClipSpace = depthProjectionMatrix * depthViewMatrix;
				if (stabilizeCascades)
				{
					glm::vec4 shadowOrigin(0.0f, 0.0f, 0.0f, 1.0f);
					shadowOrigin = ::detail::transformVectorByMatrix(viewSpaceToClipSpace, shadowOrigin);
					shadowOrigin *= static_cast<float>(shadowMapSize) * 0.5f;

					const glm::vec4 roundedOrigin = glm::round(shadowOrigin);
					glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
					roundOffset *= 2.0f / static_cast<float>(shadowMapSize);
					roundOffset.z = 0.0f;
					roundOffset.w = 0.0f;

					depthProjectionMatrix[3] += roundOffset;
					viewSpaceToClipSpace = depthProjectionMatrix * depthViewMatrix;
				}

				// Set custom camera matrices
				const_cast<CameraSceneItem*>(cameraSceneItem)->setCustomWorldSpaceToViewSpaceMatrix(depthViewMatrix);
				const_cast<CameraSceneItem*>(cameraSceneItem)->setCustomViewSpaceToClipSpaceMatrix(depthProjectionMatrix, glm::ortho(minimumExtents.x, maximumExtents.x, minimumExtents.y, maximumExtents.y, cascadeExtents.z, 0.0f));

				{ // Render shadow casters
					COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, "Render shadow casters")

					// Set render target
					Renderer::Command::SetRenderTarget::create(commandBuffer, mDepthFramebufferPtr);

					// Set the viewport and scissor rectangle
					Renderer::Command::SetViewportAndScissorRectangle::create(commandBuffer, 0, 0, shadowMapSize, shadowMapSize);

					{ // Clear the depth buffer of the current render target
						const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
						Renderer::Command::Clear::create(commandBuffer, Renderer::ClearFlag::DEPTH, color);
					}

					// Render shadow casters
					// TODO(co) Optimization: Do only render stuff which calls into the current shadow cascade
					assert(nullptr != mRenderQueueIndexRange);
					for (const RenderableManager* renderableManager : mRenderQueueIndexRange->renderableManagers)
					{
						// The render queue index range covered by this compositor instance pass scene might be smaller than the range of the
						// cached render queue index range. So, we could add a range check in here to reject renderable managers, but it's not
						// really worth to do so since the render queue only considers renderables inside the render queue range anyway.
						if (renderableManager->getCastShadows())
						{
							mRenderQueue.addRenderablesFromRenderableManager(*renderableManager, true);
						}
					}
					if (mRenderQueue.getNumberOfDrawCalls() > 0)
					{
						mRenderQueue.fillCommandBuffer(renderTarget, static_cast<const CompositorResourcePassScene&>(getCompositorResourcePass()).getMaterialTechniqueId(), shadowCompositorContextData, commandBuffer);
						mRenderQueue.clear();
					}

					// End debug event
					COMMAND_END_DEBUG_EVENT(commandBuffer)
				}

				// Unset custom camera matrices
				const_cast<CameraSceneItem*>(cameraSceneItem)->unsetCustomWorldSpaceToViewSpaceMatrix();
				const_cast<CameraSceneItem*>(cameraSceneItem)->unsetCustomViewSpaceToClipSpaceMatrix();

				// Apply the scale/offset matrix, which transforms from [-1,1] post-projection space to [0,1] UV space
				const glm::mat4 shadowMatrix = Math::getTextureScaleBiasMatrix(getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getRenderer()) * viewSpaceToClipSpace;

				// Store the split distance in terms of view space depth
				const float clipDistance = cameraSceneItem->getFarZ() - cameraSceneItem->getNearZ();
				mPassData.shadowCascadeSplits[cascadeIndex] = cameraSceneItem->getNearZ() + splitDistance * clipDistance;
				if (0 == cascadeIndex)
				{
					mPassData.shadowMatrix = shadowMatrix;
					mPassData.shadowCascadeOffsets[0] = Math::VEC4_ZERO;
					mPassData.shadowCascadeScales[0] = Math::VEC4_ONE;
				}
				else
				{
					// Calculate the position of the lower corner of the cascade partition, in the UV space of the first cascade partition
					const glm::mat4 inverseShadowMatrix = glm::inverse(shadowMatrix);
					glm::vec4 cascadeCorner = ::detail::transformVectorByMatrix(inverseShadowMatrix, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
					cascadeCorner = ::detail::transformVectorByMatrix(mPassData.shadowMatrix, cascadeCorner);

					// Do the same for the upper corner
					glm::vec4 otherCorner = ::detail::transformVectorByMatrix(inverseShadowMatrix, Math::VEC4_ONE);
					otherCorner = ::detail::transformVectorByMatrix(mPassData.shadowMatrix, otherCorner);

					// Calculate the scale and offset
					const glm::vec4 cascadeScale = Math::VEC4_ONE / (otherCorner - cascadeCorner);
					mPassData.shadowCascadeOffsets[cascadeIndex] = glm::vec4(-glm::vec3(cascadeCorner), 0.0f);
					mPassData.shadowCascadeScales[cascadeIndex] = glm::vec4(glm::vec3(cascadeScale), 1.0f);
				}
				mPassData.currentShadowCascadeIndex = cascadeIndex;

				// Calculate exponential variance shadow map (EVSM) and blur if necessary
				const glm::vec4& cascadeScale = mPassData.shadowCascadeScales[cascadeIndex];
				const float filterSizeX = std::max(shadowFilterSize * cascadeScale.x, 1.0f);
				const float filterSizeY = std::max(shadowFilterSize * cascadeScale.y, 1.0f);
				if (filterSizeX > 1.0f || filterSizeY > 1.0f)
				{
					// Execute compositor instance pass quad, use cascade index three as intermediate render target
					const uint8_t INTERMEDIATE_CASCADE_INDEX = 3;
					assert(nullptr != mVarianceFramebufferPtr[INTERMEDIATE_CASCADE_INDEX]);
					COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, "Depth to exponential variance")
					Renderer::Command::SetRenderTarget::create(commandBuffer, mVarianceFramebufferPtr[INTERMEDIATE_CASCADE_INDEX]);
					mDepthToExponentialVarianceCompositorInstancePassQuad->onFillCommandBuffer(*mVarianceFramebufferPtr[INTERMEDIATE_CASCADE_INDEX], shadowCompositorContextData, commandBuffer);
					mDepthToExponentialVarianceCompositorInstancePassQuad->onPostCommandBufferExecution();
					COMMAND_END_DEBUG_EVENT(commandBuffer)

					// Horizontal blur
					mPassData.shadowFilterSize = filterSizeX;
					COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, "Horizontal blur")
					Renderer::Command::SetRenderTarget::create(commandBuffer, mIntermediateFramebufferPtr);
					mHorizontalBlurCompositorInstancePassQuad->onFillCommandBuffer(*mIntermediateFramebufferPtr, shadowCompositorContextData, commandBuffer);
					mHorizontalBlurCompositorInstancePassQuad->onPostCommandBufferExecution();
					COMMAND_END_DEBUG_EVENT(commandBuffer)

					// Vertical blur
					mPassData.shadowFilterSize = filterSizeY;
					assert(nullptr != mVarianceFramebufferPtr[cascadeIndex]);
					COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, "Vertical blur")
					Renderer::Command::SetRenderTarget::create(commandBuffer, mVarianceFramebufferPtr[cascadeIndex]);
					mVerticalBlurCompositorInstancePassQuad->onFillCommandBuffer(*mVarianceFramebufferPtr[cascadeIndex], shadowCompositorContextData, commandBuffer);
					mVerticalBlurCompositorInstancePassQuad->onPostCommandBufferExecution();
					COMMAND_END_DEBUG_EVENT(commandBuffer)
				}
				else
				{
					// Execute compositor instance pass quad
					COMMAND_BEGIN_DEBUG_EVENT(commandBuffer, "Depth to exponential variance")
					assert(nullptr != mVarianceFramebufferPtr[cascadeIndex]);
					Renderer::Command::SetRenderTarget::create(commandBuffer, mVarianceFramebufferPtr[cascadeIndex]);
					mDepthToExponentialVarianceCompositorInstancePassQuad->onFillCommandBuffer(*mVarianceFramebufferPtr[cascadeIndex], shadowCompositorContextData, commandBuffer);
					mDepthToExponentialVarianceCompositorInstancePassQuad->onPostCommandBufferExecution();
					COMMAND_END_DEBUG_EVENT(commandBuffer)
				}

				// End debug event
				COMMAND_END_DEBUG_EVENT(commandBuffer)
			}

			// Reset to previous render target
			// TODO(co) Get rid of this
			Renderer::Command::SetRenderTarget::create(commandBuffer, &const_cast<Renderer::IRenderTarget&>(renderTarget));

			// End debug event
			COMMAND_END_DEBUG_EVENT(commandBuffer)
		}
		else
		{
			// Error!
			assert(false);
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	CompositorInstancePassShadowMap::CompositorInstancePassShadowMap(const CompositorResourcePassShadowMap& compositorResourcePassShadowMap, const CompositorNodeInstance& compositorNodeInstance) :
		CompositorInstancePassScene(compositorResourcePassShadowMap, compositorNodeInstance),
		mDepthTextureResourceId(getUninitialized<TextureResourceId>()),
		mVarianceTextureResourceId(getUninitialized<TextureResourceId>()),
		mIntermediateDepthBlurTextureResourceId(getUninitialized<TextureResourceId>()),
		mDepthToExponentialVarianceCompositorResourcePassQuad(nullptr),
		mDepthToExponentialVarianceCompositorInstancePassQuad(nullptr),
		mHorizontalBlurCompositorResourcePassQuad(nullptr),
		mHorizontalBlurCompositorInstancePassQuad(nullptr),
		mVerticalBlurCompositorResourcePassQuad(nullptr),
		mVerticalBlurCompositorInstancePassQuad(nullptr)
	{
		mPassData.shadowMatrix = Math::MAT4_IDENTITY;
		for (int i = 0; i < CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES; ++i)
		{
			mPassData.shadowCascadeScales[i] = Math::VEC4_ONE;
		}
		createShadowMapRenderTarget();
	}

	void CompositorInstancePassShadowMap::createShadowMapRenderTarget()
	{
		const CompositorResourcePassShadowMap& compositorResourcePassShadowMap = static_cast<const CompositorResourcePassShadowMap&>(getCompositorResourcePass());
		const IRendererRuntime& rendererRuntime = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime();
		const AssetId assetId = compositorResourcePassShadowMap.getTextureAssetId();

		// Tell the texture resource manager about our render target texture so it can be referenced inside e.g. compositor nodes
		TextureResourceManager& textureResourceManager = rendererRuntime.getTextureResourceManager();
		TextureResource* textureResource = textureResourceManager.getTextureResourceByAssetId(assetId);
		if (nullptr == textureResource)
		{
			Renderer::IRenderer& renderer = rendererRuntime.getRenderer();
			const uint32_t shadowMapSize = compositorResourcePassShadowMap.getShadowMapSize();
			const uint8_t numberOfShadowCascades = compositorResourcePassShadowMap.getNumberOfShadowCascades();
			assert(numberOfShadowCascades <= CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES);
			uint8_t numberOfShadowMultisamples = compositorResourcePassShadowMap.getNumberOfShadowMultisamples();
			{ // Multisamples sanity check
				const uint8_t maximumNumberOfMultisamples = renderer.getCapabilities().maximumNumberOfMultisamples;
				if (numberOfShadowMultisamples > maximumNumberOfMultisamples)
				{
					assert(false && "Number of shadow multisamples not supported by the renderer backend");
					numberOfShadowMultisamples = maximumNumberOfMultisamples;
				}
			}

			{ // Depth shadow map
				const Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::D32_FLOAT;
				Renderer::ITexture* texture = rendererRuntime.getTextureManager().createTexture2D(shadowMapSize, shadowMapSize, textureFormat, nullptr, Renderer::TextureFlag::RENDER_TARGET, Renderer::TextureUsage::DEFAULT, numberOfShadowMultisamples);
				RENDERER_SET_RESOURCE_DEBUG_NAME(texture, "Compositor instance pass depth shadow map")

				// Create the framebuffer object (FBO) instance
				const Renderer::FramebufferAttachment depthStencilFramebufferAttachment(texture);
				mDepthFramebufferPtr = renderer.createFramebuffer(*renderer.createRenderPass(0, nullptr, textureFormat), nullptr, &depthStencilFramebufferAttachment);
				RENDERER_SET_RESOURCE_DEBUG_NAME(mDepthFramebufferPtr, "Compositor instance pass depth shadow map")

				// Create texture resource
				mDepthTextureResourceId = textureResourceManager.createTextureResourceByAssetId(::detail::DEPTH_SHADOW_MAP_TEXTURE_ASSET_ID, *texture);
			}

			{ // Depth to exponential variance
				MaterialProperties materialProperties;
				materialProperties.setPropertyById(STRING_ID("DepthMap"), MaterialPropertyValue::fromTextureAssetId(::detail::DEPTH_SHADOW_MAP_TEXTURE_ASSET_ID), MaterialProperty::Usage::UNKNOWN, true);
				materialProperties.setPropertyById(STRING_ID("NumberOfMultisamples"), MaterialPropertyValue::fromInteger((numberOfShadowMultisamples == 1) ? 0 : numberOfShadowMultisamples), MaterialProperty::Usage::UNKNOWN, true);
				mDepthToExponentialVarianceCompositorResourcePassQuad = new CompositorResourcePassQuad(compositorResourcePassShadowMap.getCompositorTarget(), compositorResourcePassShadowMap.getDepthToExponentialVarianceMaterialBlueprintAssetId(), materialProperties);
				mDepthToExponentialVarianceCompositorInstancePassQuad = new CompositorInstancePassQuad(*mDepthToExponentialVarianceCompositorResourcePassQuad, getCompositorNodeInstance());
			}

			{ // Variance shadow map
				const Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::R32G32B32A32F;
				Renderer::ITexture* texture = rendererRuntime.getTextureManager().createTexture2DArray(shadowMapSize, shadowMapSize, numberOfShadowCascades, textureFormat, nullptr, Renderer::TextureFlag::RENDER_TARGET);
				RENDERER_SET_RESOURCE_DEBUG_NAME(texture, "Compositor instance pass variance shadow map")

				// Create the framebuffer object (FBO) instances
				Renderer::IRenderPass* renderPass = renderer.createRenderPass(1, &textureFormat);
				for (uint8_t cascadeIndex = 0; cascadeIndex < numberOfShadowCascades; ++cascadeIndex)
				{
					const Renderer::FramebufferAttachment colorFramebufferAttachment(texture, 0, cascadeIndex);
					mVarianceFramebufferPtr[cascadeIndex] = renderer.createFramebuffer(*renderPass, &colorFramebufferAttachment);
					RENDERER_SET_RESOURCE_DEBUG_NAME(mVarianceFramebufferPtr[cascadeIndex], ("Compositor instance pass variance shadow map " + std::to_string(cascadeIndex)).c_str())
				}
				for (uint8_t cascadeIndex = numberOfShadowCascades; cascadeIndex < CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES; ++cascadeIndex)
				{
					mVarianceFramebufferPtr[cascadeIndex] = nullptr;
				}

				// Create texture resource
				mVarianceTextureResourceId = textureResourceManager.createTextureResourceByAssetId(assetId, *texture);
			}

			{ // Intermediate depth blur shadow map
				const Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::R32G32B32A32F;
				Renderer::ITexture* texture = rendererRuntime.getTextureManager().createTexture2D(shadowMapSize, shadowMapSize, textureFormat, nullptr, Renderer::TextureFlag::RENDER_TARGET);
				RENDERER_SET_RESOURCE_DEBUG_NAME(texture, "Compositor instance pass intermediate depth blur shadow map")

				// Create the framebuffer object (FBO) instance
				const Renderer::FramebufferAttachment colorFramebufferAttachment(texture);
				mIntermediateFramebufferPtr = renderer.createFramebuffer(*renderer.createRenderPass(1, &textureFormat), &colorFramebufferAttachment);
				RENDERER_SET_RESOURCE_DEBUG_NAME(mIntermediateFramebufferPtr, "Compositor instance pass intermediate depth blur shadow map")

				// Create texture resource
				mIntermediateDepthBlurTextureResourceId = textureResourceManager.createTextureResourceByAssetId(::detail::INTERMEDIATE_DEPTH_BLUR_SHADOW_MAP_TEXTURE_ASSET_ID, *texture);
			}

			{ // Horizontal blur
				MaterialProperties materialProperties;
				materialProperties.setPropertyById(STRING_ID("VerticalBlur"), MaterialPropertyValue::fromBoolean(false), MaterialProperty::Usage::UNKNOWN, true);
				materialProperties.setPropertyById(STRING_ID("ColorMap"), MaterialPropertyValue::fromTextureAssetId(assetId), MaterialProperty::Usage::UNKNOWN, true);
				mHorizontalBlurCompositorResourcePassQuad = new CompositorResourcePassQuad(compositorResourcePassShadowMap.getCompositorTarget(), compositorResourcePassShadowMap.getBlurMaterialBlueprintAssetId(), materialProperties);
				mHorizontalBlurCompositorInstancePassQuad = new CompositorInstancePassQuad(*mHorizontalBlurCompositorResourcePassQuad, getCompositorNodeInstance());
			}

			{ // Vertical blur
				MaterialProperties materialProperties;
				materialProperties.setPropertyById(STRING_ID("VerticalBlur"), MaterialPropertyValue::fromBoolean(true), MaterialProperty::Usage::UNKNOWN, true);
				materialProperties.setPropertyById(STRING_ID("ColorMap"), MaterialPropertyValue::fromTextureAssetId(::detail::INTERMEDIATE_DEPTH_BLUR_SHADOW_MAP_TEXTURE_ASSET_ID), MaterialProperty::Usage::UNKNOWN, true);
				mVerticalBlurCompositorResourcePassQuad = new CompositorResourcePassQuad(compositorResourcePassShadowMap.getCompositorTarget(), compositorResourcePassShadowMap.getBlurMaterialBlueprintAssetId(), materialProperties);
				mVerticalBlurCompositorInstancePassQuad = new CompositorInstancePassQuad(*mVerticalBlurCompositorResourcePassQuad, getCompositorNodeInstance());
			}
		}
		else
		{
			// This is not allowed to happen
			assert(false);
		}
	}

	void CompositorInstancePassShadowMap::destroyShadowMapRenderTarget()
	{
		assert(isInitialized(mDepthTextureResourceId) && isInitialized(mVarianceTextureResourceId) && isInitialized(mIntermediateDepthBlurTextureResourceId) && nullptr != mDepthFramebufferPtr);

		// Depth to exponential variance
		delete mDepthToExponentialVarianceCompositorInstancePassQuad;
		mDepthToExponentialVarianceCompositorInstancePassQuad = nullptr;
		delete mDepthToExponentialVarianceCompositorResourcePassQuad;
		mDepthToExponentialVarianceCompositorResourcePassQuad = nullptr;

		// Horizontal blur
		delete mHorizontalBlurCompositorResourcePassQuad;
		mHorizontalBlurCompositorResourcePassQuad = nullptr;
		delete mHorizontalBlurCompositorInstancePassQuad;
		mHorizontalBlurCompositorInstancePassQuad = nullptr;

		// Vertical blur
		delete mVerticalBlurCompositorResourcePassQuad;
		mVerticalBlurCompositorResourcePassQuad = nullptr;
		delete mVerticalBlurCompositorInstancePassQuad;
		mVerticalBlurCompositorInstancePassQuad = nullptr;

		// Release the framebuffers and other renderer resources referenced by the framebuffers
		mDepthFramebufferPtr = nullptr;
		for (uint8_t cascadeIndex = 0; cascadeIndex < CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES; ++cascadeIndex)
		{
			mVarianceFramebufferPtr[cascadeIndex] = nullptr;
		}
		mIntermediateFramebufferPtr = nullptr;

		// Inform the texture resource manager that our render target texture is gone now
		TextureResourceManager& textureResourceManager = getCompositorNodeInstance().getCompositorWorkspaceInstance().getRendererRuntime().getTextureResourceManager();
		textureResourceManager.destroyTextureResource(mDepthTextureResourceId);
		textureResourceManager.destroyTextureResource(mVarianceTextureResourceId);
		textureResourceManager.destroyTextureResource(mIntermediateDepthBlurTextureResourceId);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
