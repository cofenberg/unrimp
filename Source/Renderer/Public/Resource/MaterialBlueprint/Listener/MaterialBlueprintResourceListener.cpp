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
#include "Renderer/Public/Resource/MaterialBlueprint/Listener/MaterialBlueprintResourceListener.h"
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/LightBufferManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialTechnique.h"
#include "Renderer/Public/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "Renderer/Public/Resource/Scene/Item/Light/LightSceneItem.h"
#include "Renderer/Public/Resource/Scene/Item/Sky/HosekWilkieSky.h"
#include "Renderer/Public/Resource/Scene/SceneNode.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorContextData.h"
#include "Renderer/Public/Resource/Texture/TextureResourceManager.h"
#include "Renderer/Public/Core/Time/TimeManager.h"
#include "Renderer/Public/Core/Math/Transform.h"
#include "Renderer/Public/Core/Math/Math.h"
#ifdef RENDERER_OPENVR
	#include "Renderer/Public/Vr/IVrManager.h"
#endif
#include "Renderer/Public/IRenderer.h"

#ifdef RENDERER_IMGUI
	#include <imgui/imgui.h>
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/matrix_transform.hpp>
	#include <glm/gtx/quaternion.hpp>
PRAGMA_WARNING_POP

#include <random>


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
		#define DEFINE_CONSTANT(name) static constexpr uint32_t name = STRING_ID(#name);
			// Pass data influenced by single pass stereo rendering via instancing as described in "High Performance Stereo Rendering For VR", Timothy Wilson, San Diego, Virtual Reality Meetup
			DEFINE_CONSTANT(WORLD_SPACE_TO_VIEW_SPACE_MATRIX)				///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(WORLD_SPACE_TO_VIEW_SPACE_MATRIX_2)				///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(VIEW_SPACE_TO_WORLD_SPACE_MATRIX)				///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(WORLD_SPACE_TO_VIEW_SPACE_QUATERNION)			///< "FLOAT_4"-type
			DEFINE_CONSTANT(WORLD_SPACE_TO_VIEW_SPACE_QUATERNION_2)			///< "FLOAT_4"-type
			DEFINE_CONSTANT(VIEW_SPACE_TO_WORLD_SPACE_QUATERNION)			///< "FLOAT_4"-type
			DEFINE_CONSTANT(VIEW_SPACE_TO_WORLD_SPACE_QUATERNION_2)			///< "FLOAT_4"-type
			DEFINE_CONSTANT(WORLD_SPACE_TO_CLIP_SPACE_MATRIX)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(WORLD_SPACE_TO_CLIP_SPACE_MATRIX_2)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(PREVIOUS_WORLD_SPACE_TO_CLIP_SPACE_MATRIX)		///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(PREVIOUS_WORLD_SPACE_TO_CLIP_SPACE_MATRIX_2)	///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(PREVIOUS_WORLD_SPACE_TO_VIEW_SPACE_MATRIX)		///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(PREVIOUS_WORLD_SPACE_TO_VIEW_SPACE_MATRIX_2)	///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(VIEW_SPACE_TO_CLIP_SPACE_MATRIX)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(VIEW_SPACE_TO_CLIP_SPACE_MATRIX_2)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(VIEW_SPACE_TO_TEXTURE_SPACE_MATRIX)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(CLIP_SPACE_TO_VIEW_SPACE_MATRIX)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(CLIP_SPACE_TO_WORLD_SPACE_MATRIX)				///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(WORLD_SPACE_CAMERA_POSITION)					///< "FLOAT_3"-type, camera relative world space camera position, only relevant for stereo rendering were each eye has an offset to the head and the head itself might not be identical to the camera scene node transform
			DEFINE_CONSTANT(WORLD_SPACE_CAMERA_POSITION_2)					///< "FLOAT_3"-type, camera relative world space camera position, only relevant for stereo rendering were each eye has an offset to the head and the head itself might not be identical to the camera scene node transform
			DEFINE_CONSTANT(UNMODIFIED_WORLD_SPACE_CAMERA_POSITION)			///< "FLOAT_3"-type, original unmodified world space camera position which isn't adjusted for camera relative rendering, try to avoid using this parameter - TODO(co) "UNMODIFIED_WORLD_SPACE_CAMERA_POSITION" shouldn't be used due to the loss of precision
			DEFINE_CONSTANT(WRAPPED_WORLD_SPACE_CAMERA_POSITION)			///< "FLOAT_3"-type, original wrapped world space camera position which isn't adjusted for camera relative rendering, try to avoid using this parameter since due to the value wrapping to avoid huge values, there might be visible artefact's depending on the use-case
			DEFINE_CONSTANT(VIEW_SPACE_FRUSTUM_CORNERS)						///< "FLOAT_4_4"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(VIEW_SPACE_SUNLIGHT_DIRECTION)					///< "FLOAT_3"-type

			// Pass data not influenced by single pass stereo rendering via instancing as described in "High Performance Stereo Rendering For VR", Timothy Wilson, San Diego, Virtual Reality Meetup
			DEFINE_CONSTANT(GLOBAL_COMPUTE_SIZE)						///< "INTEGER_3"-type, only valid for compute pipeline
			DEFINE_CONSTANT(IMGUI_OBJECT_SPACE_TO_CLIP_SPACE_MATRIX)	///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(WORLD_SPACE_SUNLIGHT_DIRECTION)				///< "FLOAT_3"-type
			DEFINE_CONSTANT(PROJECTION_PARAMETERS)						///< "FLOAT_2"-type
			DEFINE_CONSTANT(PROJECTION_PARAMETERS_REVERSED_Z)			///< "FLOAT_2"-type
			DEFINE_CONSTANT(NEAR_FAR_Z)									///< "FLOAT_2"-type
			DEFINE_CONSTANT(SUNLIGHT_COLOR)								///< "FLOAT_3"-type
			DEFINE_CONSTANT(VIEWPORT_SIZE)								///< "FLOAT_2"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(INVERSE_VIEWPORT_SIZE)						///< "FLOAT_2"-type, only valid for graphics pipeline
			DEFINE_CONSTANT(LIGHT_CLUSTERS_SCALE)						///< "FLOAT_3"-type
			DEFINE_CONSTANT(LIGHT_CLUSTERS_BIAS)						///< "FLOAT_3"-type
			DEFINE_CONSTANT(FULL_COVERAGE_MASK)							///< "INTEGER"-type
			DEFINE_CONSTANT(SHADOW_MATRIX)								///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(SHADOW_CASCADE_SPLITS)						///< "FLOAT_4"-type
			DEFINE_CONSTANT(SHADOW_CASCADE_OFFSETS)						///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(SHADOW_CASCADE_SCALES)						///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(CURRENT_SHADOW_CASCADE_SCALE)				///< "FLOAT_3"-type
			DEFINE_CONSTANT(SHADOW_MAP_SIZE)							///< "INTEGER"-type
			DEFINE_CONSTANT(SHADOW_FILTER_SIZE)							///< "FLOAT"-type
			DEFINE_CONSTANT(SHADOW_SAMPLE_RADIUS)						///< "INTEGER"-type
			DEFINE_CONSTANT(LENS_STAR_MATRIX)							///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(JITTER_OFFSET)								///< "FLOAT_2"-type, xy = Jitter offset using "Hammersley 4x" from "MSAA Resolve + Temporal AA" from https://github.com/TheRealMJP/MSAAFilter with background information at https://mynameismjp.wordpress.com/2012/10/28/msaa-resolve-filters/
			DEFINE_CONSTANT(HOSEK_WILKIE_SKY_COEFFICIENTS_1)			///< "FLOAT_4_4"-type
			DEFINE_CONSTANT(HOSEK_WILKIE_SKY_COEFFICIENTS_2)			///< "FLOAT_4_4"-type

			// Instance
			DEFINE_CONSTANT(INSTANCE_INDICES)							///< "INTEGER_4"-type, x = The instance texture buffer start index, y = The assigned material slot inside the material uniform buffer, z = The custom parameters start index inside the instance texture buffer
			DEFINE_CONSTANT(WORLD_POSITION_MATERIAL_INDEX)				///< "INTEGER_4"-type, xyz = Camera relative world space position, w = The assigned material slot inside the material uniform buffer
		#undef DEFINE_CONSTANT


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Create 3D identity color correction lookup table (LUT)
		*
		*  @note
		*    - Basing on "GPU Gems 2" - "Chapter 24. Using Lookup Tables to Accelerate Color Transformations" by Jeremy Selan, Sony Pictures Imageworks - http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter24.html
		*    - A way for artists to create color correction lookup tables is described at https://docs.unrealengine.com/latest/INT/Engine/Rendering/PostProcessEffects/ColorGrading/
		*    - Color correction lookup table size is 16
		*    - Resulting texture asset ID is "Unrimp/Texture/DynamicByCode/IdentityColorCorrectionLookupTable3D"
		*/
		[[nodiscard]] Renderer::TextureResourceId createIdentityColorCorrectionLookupTable3D(const Renderer::IRenderer& renderer)
		{
			static constexpr uint8_t SIZE = 16;
			static constexpr uint8_t NUMBER_OF_COMPONENTS = 4;
			static constexpr uint32_t NUMBER_OF_BYTES = SIZE * SIZE * SIZE * NUMBER_OF_COMPONENTS;
			uint8_t data[NUMBER_OF_BYTES];

			{ // Create the identity color correction lookup table 3D data
				uint8_t* currentData = data;
				for (uint8_t z = 0; z < SIZE; ++z)
				{
					for (uint8_t y = 0; y < SIZE; ++y)
					{
						for (uint8_t x = 0; x < SIZE; ++x)
						{
							currentData[0] = static_cast<uint8_t>((static_cast<float>(x) / static_cast<float>(SIZE)) * 255.0f);
							currentData[1] = static_cast<uint8_t>((static_cast<float>(y) / static_cast<float>(SIZE)) * 255.0f);
							currentData[2] = static_cast<uint8_t>((static_cast<float>(z) / static_cast<float>(SIZE)) * 255.0f);
							// currentData[3] = 0;	// Unused
							currentData += NUMBER_OF_COMPONENTS;
						}
					}
				}
			}

			// Create dynamic texture asset
			return renderer.getTextureResourceManager().createTextureResourceByAssetId(
				ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityColorCorrectionLookupTable3D"),
				*renderer.getTextureManager().createTexture3D(SIZE, SIZE, SIZE, Rhi::TextureFormat::R8G8B8A8, data, Rhi::TextureFlag::SHADER_RESOURCE, Rhi::TextureUsage::IMMUTABLE RHI_RESOURCE_DEBUG_NAME("Identity color correction lookup table (LUT)")));
		}

		/**
		*  @brief
		*    Create 1D screen space ambient occlusion sample kernel texture
		*
		*  @remarks
		*    The sample kernel requirements are that:
		*    - Sample positions fall within the unit hemisphere
		*    - Sample positions are more densely clustered towards the origin. This effectively attenuates the occlusion contribution
		*      according to distance from the kernel center - samples closer to a point occlude it more than samples further away
		*
		*  @note
		*    - Basing on "SSAO Tutorial" from John Chapman - http://john-chapman-graphics.blogspot.de/2013/01/ssao-tutorial.html
		*    - Kernel size is 16, since the samples are randomly distributed this doesn't mean that a shader has to use all samples
		*    - Resulting texture asset ID is "Unrimp/Texture/DynamicByCode/SsaoSampleKernel"
		*/
		[[nodiscard]] Renderer::TextureResourceId createSsaoSampleKernelTexture(const Renderer::IRenderer& renderer)
		{
			static constexpr uint32_t KERNEL_SIZE = 16;
			glm::vec4 kernel[KERNEL_SIZE];

			{ // Create the kernel
				std::mt19937 randomGenerator;
				std::uniform_real_distribution<float> randomDistributionHalf(0.0f, 1.0f);
				std::uniform_real_distribution<float> randomDistributionFull(-1.0f, 1.0f);
				for (uint32_t i = 0; i < KERNEL_SIZE; ++i)
				{
					// Create a sample point on the surface of a hemisphere oriented along the z axis
					kernel[i] = glm::vec4(randomDistributionFull(randomGenerator), randomDistributionFull(randomGenerator), randomDistributionHalf(randomGenerator), 0.0f);
					kernel[i] = glm::normalize(kernel[i]);

					// Distribute the sample position within the hemisphere
					kernel[i] *= randomDistributionHalf(randomGenerator);

					// Apply accelerating interpolation function to generate more points closer to the origin
					float scale = float(i) / float(KERNEL_SIZE);
					scale = glm::mix(0.1f, 1.0f, scale * scale);	// Linear interpolation (= "lerp" = "mix")
					kernel[i] *= scale;
				}
			}

			// Create dynamic texture asset
			return renderer.getTextureResourceManager().createTextureResourceByAssetId(
				ASSET_ID("Unrimp/Texture/DynamicByCode/SsaoSampleKernel"),
				*renderer.getTextureManager().createTexture1D(KERNEL_SIZE, Rhi::TextureFormat::R32G32B32A32F, kernel, Rhi::TextureFlag::SHADER_RESOURCE, Rhi::TextureUsage::IMMUTABLE RHI_RESOURCE_DEBUG_NAME("Screen space ambient occlusion sample kernel")));
		}

		/**
		*  @brief
		*    Create 2D screen space ambient occlusion 4x4 noise texture
		*
		*  @remarks
		*    When used for screen space ambient occlusion, the noise which is tiled over the screen is used to rotate the sample kernel. This will effectively increase the
		*    sample count and minimize "banding" artifacts. The tiling of the texture causes the orientation of the kernel to be repeated and introduces regularity into the
		*    result. By keeping the texture size small we can make this regularity occur at a high frequency, which can then be removed with a blur step that preserves the
		*    low-frequency detail of the image. Using a 4x4 texture and blur kernel produces excellent results at minimal cost. This is the same approach as used in Crysis.
		*
		*  @note
		*    - Basing on "SSAO Tutorial" from John Chapman - http://john-chapman-graphics.blogspot.de/2013/01/ssao-tutorial.html
		*    - Noise texture size is 4x4
		*    - Resulting texture asset ID is "Unrimp/Texture/DynamicByCode/SsaoNoise4x4"
		*/
		[[nodiscard]] Renderer::TextureResourceId createSsaoNoiseTexture4x4(const Renderer::IRenderer& renderer)
		{
			static constexpr uint32_t NOISE_SIZE = 4;
			static constexpr uint32_t SQUARED_NOISE_SIZE = NOISE_SIZE * NOISE_SIZE;
			glm::vec4 noise[SQUARED_NOISE_SIZE];

			{ // Create the noise
				std::mt19937 randomGenerator;
				std::uniform_real_distribution<float> randomDistribution(-1.0f, 1.0f);
				for (uint32_t i = 0; i < SQUARED_NOISE_SIZE; ++i)
				{
					noise[i] = glm::vec4(randomDistribution(randomGenerator), randomDistribution(randomGenerator), 0.0f, 0.0f);
					noise[i] = glm::normalize(noise[i]);
				}
			}

			// Create dynamic texture asset
			return renderer.getTextureResourceManager().createTextureResourceByAssetId(
				ASSET_ID("Unrimp/Texture/DynamicByCode/SsaoNoise4x4"),
				*renderer.getTextureManager().createTexture2D(NOISE_SIZE, NOISE_SIZE, Rhi::TextureFormat::R32G32B32A32F, noise, Rhi::TextureFlag::SHADER_RESOURCE, Rhi::TextureUsage::IMMUTABLE, 1, nullptr RHI_RESOURCE_DEBUG_NAME("Screen space ambient occlusion 4x4 noise")));
		}

		/**
		*  @brief
		*    Compute a radical inverse with base 2 using crazy bit-twiddling from "Hacker's Delight"
		*/
		[[nodiscard]] inline float radicalInverseBase2(uint32_t bits)
		{
			bits = (bits << 16u) | (bits >> 16u);
			bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
			bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
			bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
			bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
			return float(bits) * 2.3283064365386963e-10f;	// / 0x100000000
		}

		/**
		*  @brief
		*    Return a single 2D point in a Hammersley sequence of length "numberOfSamples", using base 1 and base 2
		*
		*  @note
		*    - From "MSAA Resolve + Temporal AA" from https://github.com/TheRealMJP/MSAAFilter with background information at https://mynameismjp.wordpress.com/2012/10/28/msaa-resolve-filters/
		*/
		[[nodiscard]] inline glm::vec2 hammersley2D(uint64_t sampleIndex, uint64_t numberOfSamples)
		{
			return glm::vec2(float(sampleIndex) / float(numberOfSamples), radicalInverseBase2(uint32_t(sampleIndex)));
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void MaterialBlueprintResourceListener::getDefaultTextureAssetIds(AssetIds& assetIds)
	{
		// Define helper macro
		#define ADD_ASSET_ID(name) assetIds.push_back(ASSET_ID(name));

		// Add asset IDs
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityColorCorrectionLookupTable3D")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/SsaoSampleKernel")
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/SsaoNoise4x4")

		// Undefine helper macro
		#undef ADD_ASSET_ID
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void MaterialBlueprintResourceListener::clear()
	{
		if (nullptr != mHosekWilkieSky)
		{
			delete mHosekWilkieSky;
			mHosekWilkieSky = nullptr;
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::IMaterialBlueprintResourceListener methods ]
	//[-------------------------------------------------------]
	void MaterialBlueprintResourceListener::onStartup(const IRenderer& renderer)
	{
		mIdentityColorCorrectionLookupTable3D = ::detail::createIdentityColorCorrectionLookupTable3D(renderer);
		mSsaoSampleKernelTextureResourceId = ::detail::createSsaoSampleKernelTexture(renderer);
		mSsaoNoiseTexture4x4ResourceId = ::detail::createSsaoNoiseTexture4x4(renderer);
	}

	void MaterialBlueprintResourceListener::onShutdown(const IRenderer& renderer)
	{
		TextureResourceManager& textureResourceManager = renderer.getTextureResourceManager();
		textureResourceManager.destroyTextureResource(mIdentityColorCorrectionLookupTable3D);
		textureResourceManager.destroyTextureResource(mSsaoSampleKernelTextureResourceId);
		textureResourceManager.destroyTextureResource(mSsaoNoiseTexture4x4ResourceId);
	}

	void MaterialBlueprintResourceListener::beginFillPass(IRenderer& renderer, const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, PassBufferManager::PassData& passData)
	{
		// Sanity checks: The render target to render into must be valid for graphics pipeline and must be a null pointer for compute pipeline
		RHI_ASSERT(mRenderer->getContext(), compositorContextData.getCurrentlyBoundMaterialBlueprintResource() != nullptr, "Invalid currently bound material blueprint resource")
		RHI_ASSERT(mRenderer->getContext(), isValid(compositorContextData.getCurrentlyBoundMaterialBlueprintResource()->getComputeShaderBlueprintResourceId()) || nullptr != renderTarget, "Graphics pipeline used but render target is invalid")
		RHI_ASSERT(mRenderer->getContext(), isInvalid(compositorContextData.getCurrentlyBoundMaterialBlueprintResource()->getComputeShaderBlueprintResourceId()) || nullptr == renderTarget, "Compute pipeline used but render target is valid")

		// Remember the pass data memory address of the current scope
		mRenderer = &renderer;
		mPassData = &passData;
		mCompositorContextData = &compositorContextData;
		mWorldSpaceCameraPosition = compositorContextData.getWorldSpaceCameraPosition();

		// Get the render target with and height
		if (nullptr != renderTarget)
		{
			// Graphics pipeline
			#ifdef RHI_DEBUG
				mIsComputePipeline = false;
			#endif
			renderTarget->getWidthAndHeight(mRenderTargetWidth, mRenderTargetHeight);
		}
		else
		{
			// Compute pipeline: Just a fallback render target width and height to not having things horrible broken in case of misuse or an error
			#ifdef RHI_DEBUG
				mIsComputePipeline = true;
			#endif
			mRenderTargetWidth = compositorContextData.getGlobalComputeSize()[0];
			mRenderTargetHeight = compositorContextData.getGlobalComputeSize()[1];
		}
		const bool singlePassStereoInstancing = mCompositorContextData->getSinglePassStereoInstancing();
		const uint32_t renderTargetWidth = singlePassStereoInstancing ? (mRenderTargetWidth / 2) : mRenderTargetWidth;

		// Get camera settings
		const CameraSceneItem* cameraSceneItem = compositorContextData.getCameraSceneItem();
		mNearZ = (nullptr != cameraSceneItem) ? cameraSceneItem->getNearZ() : CameraSceneItem::DEFAULT_NEAR_Z;
		mFarZ  = (nullptr != cameraSceneItem) ? cameraSceneItem->getFarZ()  : CameraSceneItem::DEFAULT_FAR_Z;

		// Calculate required matrices basing whether or not the VR-manager is currently running
		glm::mat4 viewSpaceToClipSpaceMatrix;
		glm::mat4 viewSpaceToClipSpaceMatrixReversedZ;
		#ifdef RENDERER_OPENVR
			const IVrManager& vrManager = renderer.getVrManager();
			const bool vrRendering = (singlePassStereoInstancing && vrManager.isRunning() && !cameraSceneItem->hasCustomWorldSpaceToViewSpaceMatrix() && !cameraSceneItem->hasCustomViewSpaceToClipSpaceMatrix());
		#else
			static constexpr bool vrRendering = false;
		#endif
		const uint32_t numberOfEyes = vrRendering ? 2u : 1u;
		for (uint32_t eyeIndex = 0; eyeIndex < numberOfEyes; ++eyeIndex)
		{
			if (nullptr != cameraSceneItem)
			{
				#ifdef RENDERER_OPENVR
					if (vrRendering)
					{
						// Virtual reality rendering

						// Ask the virtual reality manager for the HMD transformation
						// -> Near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
						const IVrManager::VrEye vrEye = (0 == eyeIndex) ? IVrManager::VrEye::RIGHT : IVrManager::VrEye::LEFT;
						viewSpaceToClipSpaceMatrix = vrManager.getHmdViewSpaceToClipSpaceMatrix(vrEye, mNearZ, mFarZ);
						viewSpaceToClipSpaceMatrixReversedZ = vrManager.getHmdViewSpaceToClipSpaceMatrix(vrEye, mFarZ, mNearZ);

						// Calculate the world space to view space matrix (Aka "view matrix")
						const glm::mat4 hmdEyeSpaceToHeadSpaceMatrix = vrManager.getHmdEyeSpaceToHeadSpaceMatrix(vrEye);
						const glm::mat4 inverseHmdEyeSpaceToHeadSpaceMatrix = glm::inverse(hmdEyeSpaceToHeadSpaceMatrix);
						mCameraRelativeWorldSpaceCameraPosition[eyeIndex] = glm::vec3((hmdEyeSpaceToHeadSpaceMatrix * vrManager.getHmdHeadSpaceToWorldSpaceMatrix())[3]);
						mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex] = (inverseHmdEyeSpaceToHeadSpaceMatrix * glm::inverse(vrManager.getHmdHeadSpaceToWorldSpaceMatrix())) * cameraSceneItem->getCameraRelativeWorldSpaceToViewSpaceMatrix();
						cameraSceneItem->getPreviousCameraRelativeWorldSpaceToViewSpaceMatrix(mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex]);
						mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex] = (inverseHmdEyeSpaceToHeadSpaceMatrix * glm::inverse(vrManager.getPreviousHmdHeadSpaceToWorldSpaceMatrix())) * mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex];
					}
					else
				#endif
				{
					// Standard rendering using a camera scene item
					mCameraRelativeWorldSpaceCameraPosition[eyeIndex] = Math::VEC3_ZERO;

					// Get world space to view space matrix (Aka "view matrix")
					mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex] = cameraSceneItem->getCameraRelativeWorldSpaceToViewSpaceMatrix();
					cameraSceneItem->getPreviousCameraRelativeWorldSpaceToViewSpaceMatrix(mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex]);

					// Get view space to clip space matrix (aka "projection matrix")
					viewSpaceToClipSpaceMatrix = cameraSceneItem->getViewSpaceToClipSpaceMatrix(static_cast<float>(renderTargetWidth) / mRenderTargetHeight);
					viewSpaceToClipSpaceMatrixReversedZ = cameraSceneItem->getViewSpaceToClipSpaceMatrixReversedZ(static_cast<float>(renderTargetWidth) / mRenderTargetHeight);
				}
			}
			else
			{
				// Standard rendering
				mCameraRelativeWorldSpaceCameraPosition[eyeIndex] = Math::VEC3_ZERO;

				// Get world space to view space matrix (Aka "view matrix")
				mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex] = mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex] = glm::lookAt(Math::VEC3_ZERO, Math::VEC3_FORWARD, Math::VEC3_UP);

				// Get view space to clip space matrix (aka "projection matrix")
				// -> Near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				viewSpaceToClipSpaceMatrix = glm::perspective(CameraSceneItem::DEFAULT_FOV_Y, static_cast<float>(renderTargetWidth) / mRenderTargetHeight, CameraSceneItem::DEFAULT_NEAR_Z, CameraSceneItem::DEFAULT_FAR_Z);
				viewSpaceToClipSpaceMatrixReversedZ = glm::perspective(CameraSceneItem::DEFAULT_FOV_Y, static_cast<float>(renderTargetWidth) / mRenderTargetHeight, CameraSceneItem::DEFAULT_FAR_Z, CameraSceneItem::DEFAULT_NEAR_Z);
			}
			mPassData->cameraRelativeWorldSpaceToViewSpaceQuaternion[eyeIndex] = glm::quat(mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex]);
			mPassData->cameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[eyeIndex] = viewSpaceToClipSpaceMatrixReversedZ * mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex];
			mPassData->previousCameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[eyeIndex] = viewSpaceToClipSpaceMatrixReversedZ * mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[eyeIndex];	// TODO(co) Do also support the previous view space to clip space matrix so e.g. FOV changes have an influence?
			mPassData->viewSpaceToClipSpaceMatrix[eyeIndex] = viewSpaceToClipSpaceMatrix;
			mPassData->viewSpaceToClipSpaceMatrixReversedZ[eyeIndex] = viewSpaceToClipSpaceMatrixReversedZ;
		}
	}

	bool MaterialBlueprintResourceListener::fillPassValue(uint32_t referenceValue, uint8_t* buffer, uint32_t numberOfBytes)
	{
		bool valueFilled = true;

		// Resolve the reference value
		switch (referenceValue)
		{
			case ::detail::WORLD_SPACE_TO_VIEW_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[0]), numberOfBytes);
				break;

			case ::detail::WORLD_SPACE_TO_VIEW_SPACE_MATRIX_2:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[1]), numberOfBytes);
				break;

			case ::detail::VIEW_SPACE_TO_WORLD_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(glm::inverse(mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[0])), numberOfBytes);
				break;

			case ::detail::WORLD_SPACE_TO_VIEW_SPACE_QUATERNION:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->cameraRelativeWorldSpaceToViewSpaceQuaternion[0]), numberOfBytes);
				break;

			case ::detail::WORLD_SPACE_TO_VIEW_SPACE_QUATERNION_2:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->cameraRelativeWorldSpaceToViewSpaceQuaternion[1]), numberOfBytes);
				break;

			case ::detail::VIEW_SPACE_TO_WORLD_SPACE_QUATERNION:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(glm::inverse(mPassData->cameraRelativeWorldSpaceToViewSpaceQuaternion[0])), numberOfBytes);
				break;

			case ::detail::VIEW_SPACE_TO_WORLD_SPACE_QUATERNION_2:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(glm::inverse(mPassData->cameraRelativeWorldSpaceToViewSpaceQuaternion[1])), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::WORLD_SPACE_TO_CLIP_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"WORLD_SPACE_TO_CLIP_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->cameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[0]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::WORLD_SPACE_TO_CLIP_SPACE_MATRIX_2:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"WORLD_SPACE_TO_CLIP_SPACE_MATRIX_2\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->cameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[1]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::PREVIOUS_WORLD_SPACE_TO_CLIP_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"PREVIOUS_WORLD_SPACE_TO_CLIP_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->previousCameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[0]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::PREVIOUS_WORLD_SPACE_TO_CLIP_SPACE_MATRIX_2:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"PREVIOUS_WORLD_SPACE_TO_CLIP_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->previousCameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[1]), numberOfBytes);
				break;

			case ::detail::PREVIOUS_WORLD_SPACE_TO_VIEW_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[0]), numberOfBytes);
				break;

			case ::detail::PREVIOUS_WORLD_SPACE_TO_VIEW_SPACE_MATRIX_2:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->previousCameraRelativeWorldSpaceToViewSpaceMatrix[1]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::VIEW_SPACE_TO_CLIP_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"VIEW_SPACE_TO_CLIP_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->viewSpaceToClipSpaceMatrixReversedZ[0]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::VIEW_SPACE_TO_CLIP_SPACE_MATRIX_2:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"VIEW_SPACE_TO_CLIP_SPACE_MATRIX2\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mPassData->viewSpaceToClipSpaceMatrixReversedZ[1]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::VIEW_SPACE_TO_TEXTURE_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"VIEW_SPACE_TO_TEXTURE_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(Math::getTextureScaleBiasMatrix(mRenderer->getRhi()) * mPassData->viewSpaceToClipSpaceMatrixReversedZ[0]), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::CLIP_SPACE_TO_VIEW_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"CLIP_SPACE_TO_VIEW_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(glm::inverse(mPassData->viewSpaceToClipSpaceMatrixReversedZ[0])), numberOfBytes);
				break;

			// Only valid for graphics pipeline
			case ::detail::CLIP_SPACE_TO_WORLD_SPACE_MATRIX:
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"CLIP_SPACE_TO_WORLD_SPACE_MATRIX\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(glm::inverse(mPassData->cameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[0])), numberOfBytes);
				break;

			case ::detail::WORLD_SPACE_CAMERA_POSITION:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mCameraRelativeWorldSpaceCameraPosition[0]), numberOfBytes);
				break;
			}

			case ::detail::WORLD_SPACE_CAMERA_POSITION_2:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mCameraRelativeWorldSpaceCameraPosition[1]), numberOfBytes);
				break;
			}

			case ::detail::UNMODIFIED_WORLD_SPACE_CAMERA_POSITION:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				// TODO(co) "UNMODIFIED_WORLD_SPACE_CAMERA_POSITION" shouldn't be used due to the loss of precision
				const glm::vec3 worldSpaceCameraPosition = mWorldSpaceCameraPosition;
				memcpy(buffer, glm::value_ptr(worldSpaceCameraPosition), numberOfBytes);
				break;
			}

			case ::detail::WRAPPED_WORLD_SPACE_CAMERA_POSITION:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				const double minimum = -1000.0;
				const double maximum =  1000.0;
				const glm::vec3 worldSpaceCameraPosition(Math::wrapToInterval(mWorldSpaceCameraPosition.x, minimum, maximum), Math::wrapToInterval(mWorldSpaceCameraPosition.y, minimum, maximum), Math::wrapToInterval(mWorldSpaceCameraPosition.z, minimum, maximum));
				memcpy(buffer, glm::value_ptr(worldSpaceCameraPosition), numberOfBytes);
				break;
			}

			// Only valid for graphics pipeline
			case ::detail::VIEW_SPACE_FRUSTUM_CORNERS:
			{
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"VIEW_SPACE_FRUSTUM_CORNERS\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")

				// Coordinate system related adjustments
				// -> Vulkan and Direct3D: Left-handed coordinate system with clip space depth value range 0..1
				// -> OpenGL without "GL_ARB_clip_control"-extension: Right-handed coordinate system with clip space depth value range -1..1
				const float nearZ = mRenderer->getRhi().getCapabilities().zeroToOneClipZ ? 0.0f : -1.0f;
				static constexpr float FAR_Z = 1.0f;

				// Calculate the view space frustum corners
				glm::vec4 viewSpaceFrustumCorners[8] =
				{
					// Near
					{-1.0f,  1.0f, nearZ, 1.0f},	// 0: Near top left
					{ 1.0f,  1.0f, nearZ, 1.0f},	// 1: Near top right
					{-1.0f, -1.0f, nearZ, 1.0f},	// 2: Near bottom left
					{ 1.0f, -1.0f, nearZ, 1.0f},	// 3: Near bottom right
					// Far
					{-1.0f,  1.0f, FAR_Z, 1.0f},	// 4: Far top left
					{ 1.0f,  1.0f, FAR_Z, 1.0f},	// 5: Far top right
					{-1.0f, -1.0f, FAR_Z, 1.0f},	// 6: Far bottom left
					{ 1.0f, -1.0f, FAR_Z, 1.0f}		// 7: Far bottom right
				};
				const glm::mat4 clipSpaceToViewSpaceMatrix = glm::inverse(mPassData->viewSpaceToClipSpaceMatrix[0]);
				for (int i = 0; i < 8; ++i)
				{
					viewSpaceFrustumCorners[i] = clipSpaceToViewSpaceMatrix * viewSpaceFrustumCorners[i];
					viewSpaceFrustumCorners[i] /= viewSpaceFrustumCorners[i].w;
				}
				for (int i = 0; i < 4; ++i)
				{
					viewSpaceFrustumCorners[i + 4] -= viewSpaceFrustumCorners[i];
				}

				{ // Clip space vertex positions of the full screen triangle, left/bottom is (-1,-1) and right/top is (1,1)
				  //								Vertex ID	Triangle on screen
				  //	-1.0f,  1.0f, 0.0f, 0.0f,	0			  0.......1
				  //	 3.0f,  1.0f, 2.0f, 0.0f,	1			  .   .
				  //	-1.0f, -3.0f, 0.0f, 2.0f	2			  2
					glm::vec4& topLeft	  = viewSpaceFrustumCorners[4];	// Vertex ID 0
					glm::vec4& topRight	  = viewSpaceFrustumCorners[5];	// Vertex ID 1
					glm::vec4& bottomLeft = viewSpaceFrustumCorners[6];	// Vertex ID 2
					topRight.x   = glm::mix(topLeft.x, topRight.x, 2.0f);
					bottomLeft.y = glm::mix(topLeft.y, bottomLeft.y, 2.0f);
					if (mRenderer->getRhi().getCapabilities().upperLeftOrigin)
					{
						topLeft.y = 1.0f - topLeft.y;
						topRight.y = 1.0f - topRight.y;
						bottomLeft.y = 1.0f - bottomLeft.y;
					}
				}

				// Copy over the data, we're using 4 * float4 by intent in order to avoid alignment problems, 3 * float3 would be sufficient for our full screen triangle
				memcpy(buffer, glm::value_ptr(viewSpaceFrustumCorners[4]), numberOfBytes);
				break;
			}

			case ::detail::VIEW_SPACE_SUNLIGHT_DIRECTION:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				const glm::vec3 viewSpaceSunlightDirection = glm::normalize(mPassData->cameraRelativeWorldSpaceToViewSpaceQuaternion[0] * getWorldSpaceSunlightDirection());	// Normalize shouldn't be necessary, but last chance here to correct rounding errors before the shader is using the normalized direction vector
				memcpy(buffer, glm::value_ptr(viewSpaceSunlightDirection), numberOfBytes);
				break;
			}

			// Only valid for compute pipeline
			case ::detail::GLOBAL_COMPUTE_SIZE:
			{
				RHI_ASSERT(mRenderer->getContext(), mIsComputePipeline, "\"GLOBAL_COMPUTE_SIZE\" is only valid for compute pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(int32_t) * 3 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, mCompositorContextData->getGlobalComputeSize(), numberOfBytes);
				break;
			}

			case ::detail::IMGUI_OBJECT_SPACE_TO_CLIP_SPACE_MATRIX:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				#ifdef RENDERER_IMGUI
					const ImGuiIO& imGuiIo = ImGui::GetIO();
					const float objectSpaceToClipSpaceMatrix[4][4] =
					{
						{  2.0f / imGuiIo.DisplaySize.x, 0.0f,                          0.0f, 0.0f },
						{  0.0f,                         2.0f / -imGuiIo.DisplaySize.y, 0.0f, 0.0f },
						{  0.0f,                         0.0f,                          0.5f, 0.0f },
						{ -1.0f,                         1.0f,                          0.5f, 1.0f }
					};
				#else
					static constexpr float objectSpaceToClipSpaceMatrix[4][4] =
					{
						{ 1.0f, 0.0f, 0.0f, 0.0f },
						{ 0.0f, 1.0f, 0.0f, 0.0f },
						{ 0.0f, 0.0f, 1.0f, 0.0f },
						{ 0.0f, 0.0f, 0.0f, 1.0f }
					};
				#endif
				memcpy(buffer, objectSpaceToClipSpaceMatrix, numberOfBytes);
				break;
			}

			case ::detail::WORLD_SPACE_SUNLIGHT_DIRECTION:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(getWorldSpaceSunlightDirection()), numberOfBytes);
				break;

			case ::detail::PROJECTION_PARAMETERS:
			{
				// For details see "The Danger Zone" - "Position From Depth 3: Back In The Habit" - "Written by MJPSeptember 5, 2010" - https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 2 == numberOfBytes, "Invalid number of bytes")
				const float projectionParameters[2] = { mFarZ / (mFarZ - mNearZ), (-mFarZ * mNearZ) / (mFarZ - mNearZ) };
				memcpy(buffer, &projectionParameters[0], numberOfBytes);
				break;
			}

			case ::detail::PROJECTION_PARAMETERS_REVERSED_Z:
			{
				// For details see "The Danger Zone" - "Position From Depth 3: Back In The Habit" - "Written by MJPSeptember 5, 2010" - https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
				// -> Near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 2 == numberOfBytes, "Invalid number of bytes")
				const float projectionParameters[2] = { mNearZ / (mNearZ - mFarZ), (-mNearZ * mFarZ) / (mNearZ - mFarZ) };
				memcpy(buffer, &projectionParameters[0], numberOfBytes);
				break;
			}

			case ::detail::NEAR_FAR_Z:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 2 == numberOfBytes, "Invalid number of bytes")
				const float nearFarZ[2] = { mNearZ, mFarZ };
				memcpy(buffer, &nearFarZ[0], numberOfBytes);
				break;
			}

			case ::detail::SUNLIGHT_COLOR:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				const LightSceneItem* lightSceneItem = mCompositorContextData->getLightSceneItem();
				if (nullptr != lightSceneItem)
				{
					memcpy(buffer, glm::value_ptr(lightSceneItem->getColor()), numberOfBytes);
				}
				else
				{
					memcpy(buffer, glm::value_ptr(Math::VEC3_ONE), numberOfBytes);
				}
				break;
			}

			// Only valid for graphics pipeline
			case ::detail::VIEWPORT_SIZE:
			{
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"VIEWPORT_SIZE\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 2 == numberOfBytes, "Invalid number of bytes")
				float* floatBuffer = reinterpret_cast<float*>(buffer);

				// 0 = Viewport width
				// 1 = Viewport height
				floatBuffer[0] = static_cast<float>(mRenderTargetWidth);
				floatBuffer[1] = static_cast<float>(mRenderTargetHeight);
				break;
			}

			// Only valid for graphics pipeline
			case ::detail::INVERSE_VIEWPORT_SIZE:
			{
				RHI_ASSERT(mRenderer->getContext(), !mIsComputePipeline, "\"INVERSE_VIEWPORT_SIZE\" is only valid for graphics pipeline")
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 2 == numberOfBytes, "Invalid number of bytes")
				float* floatBuffer = reinterpret_cast<float*>(buffer);

				// 0 = Inverse viewport width
				// 1 = Inverse viewport height
				floatBuffer[0] = 1.0f / static_cast<float>(mRenderTargetWidth);
				floatBuffer[1] = 1.0f / static_cast<float>(mRenderTargetHeight);
				break;
			}

			case ::detail::LIGHT_CLUSTERS_SCALE:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mRenderer->getMaterialBlueprintResourceManager().getLightBufferManager().getLightClustersScale()), numberOfBytes);
				break;

			case ::detail::LIGHT_CLUSTERS_BIAS:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				memcpy(buffer, glm::value_ptr(mRenderer->getMaterialBlueprintResourceManager().getLightBufferManager().getLightClustersBias()), numberOfBytes);
				break;

			case ::detail::FULL_COVERAGE_MASK:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(int) == numberOfBytes, "Invalid number of bytes")
				const int fullCoverageMask = (1 << mCompositorContextData->getCompositorWorkspaceInstance()->getNumberOfMultisamples()) - 1;	// 0xF for 4x MSAA
				memcpy(buffer, &fullCoverageMask, numberOfBytes);
				break;
			}

			case ::detail::SHADOW_MATRIX:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					memcpy(buffer, glm::value_ptr(compositorInstancePassShadowMap->getPassData().shadowMatrix), numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::SHADOW_CASCADE_SPLITS:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					memcpy(buffer, compositorInstancePassShadowMap->getPassData().shadowCascadeSplits, numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::SHADOW_CASCADE_OFFSETS:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					memcpy(buffer, compositorInstancePassShadowMap->getPassData().shadowCascadeOffsets, numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::SHADOW_CASCADE_SCALES:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					memcpy(buffer, compositorInstancePassShadowMap->getPassData().shadowCascadeScales, numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::CURRENT_SHADOW_CASCADE_SCALE:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 3 == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					const CompositorInstancePassShadowMap::PassData& passData = compositorInstancePassShadowMap->getPassData();
					memcpy(buffer, &passData.shadowCascadeScales[passData.currentShadowCascadeIndex], numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::SHADOW_MAP_SIZE:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(int) == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					memcpy(buffer, &compositorInstancePassShadowMap->getPassData().shadowMapSize, numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::SHADOW_FILTER_SIZE:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					memcpy(buffer, &compositorInstancePassShadowMap->getPassData().shadowFilterSize, numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::SHADOW_SAMPLE_RADIUS:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(int) == numberOfBytes, "Invalid number of bytes")
				const CompositorInstancePassShadowMap* compositorInstancePassShadowMap = mCompositorContextData->getCompositorInstancePassShadowMap();
				if (nullptr != compositorInstancePassShadowMap)
				{
					const int shadowSampleRadius = static_cast<int>((compositorInstancePassShadowMap->getPassData().shadowFilterSize * 0.5f) + 0.499f);
					memcpy(buffer, &shadowSampleRadius, numberOfBytes);
				}
				else
				{
					// Error!
					RHI_ASSERT(mRenderer->getContext(), false, "Invalid compositor instance pass shadow map")
					memset(buffer, 0, numberOfBytes);
				}
				break;
			}

			case ::detail::LENS_STAR_MATRIX:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")

				// The following is basing on 'Pseudo Lens Flare' from John Chapman - http://john-chapman-graphics.blogspot.de/2013/02/pseudo-lens-flare.html

				// Get the camera rotation; it just needs to change continuously as the camera rotates
				const glm::mat4& cameraRelativeWorldSpaceToViewSpaceMatrix = mPassData->cameraRelativeWorldSpaceToViewSpaceMatrix[0];
				const glm::vec3 cameraX = cameraRelativeWorldSpaceToViewSpaceMatrix[0];	// Camera x (left) vector
				const glm::vec3 cameraZ = cameraRelativeWorldSpaceToViewSpaceMatrix[1];	// Camera z (forward) vector
				const float cameraRotation = glm::dot(cameraX, Math::VEC3_UNIT_Z) + glm::dot(cameraZ, Math::VEC3_UNIT_Y);

				// Calculate the lens star matrix
				const glm::mat3 scaleBias1(
					2.0f, 0.0f, -1.0f,
					0.0f, 2.0f, -1.0f,
					0.0f, 0.0f,  1.0f
				);
				const glm::mat3 rotation(
					std::cos(cameraRotation), -std::sin(cameraRotation),  0.0f,
					std::sin(cameraRotation),  std::cos(cameraRotation),  0.0f,
					0.0f,                      0.0f,                      1.0f
				);
				const glm::mat3 scaleBias2(
					0.5f, 0.0f, 0.5f,
					0.0f, 0.5f, 0.5f,
					0.0f, 0.0f, 1.0f
				);
				const glm::mat4 uLensStarMatrix = scaleBias1 * rotation * scaleBias2;

				// Copy the matrix over
				memcpy(buffer, glm::value_ptr(uLensStarMatrix), numberOfBytes);
				break;
			}

			case ::detail::JITTER_OFFSET:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 2 == numberOfBytes, "Invalid number of bytes")

				// Calculate the jitter offset using "Hammersley 4x" from "MSAA Resolve + Temporal AA" from https://github.com/TheRealMJP/MSAAFilter with background information at https://mynameismjp.wordpress.com/2012/10/28/msaa-resolve-filters/
				const uint64_t numberOfRenderedFrames = mRenderer->getTimeManager().getNumberOfRenderedFrames();
				if (numberOfRenderedFrames != mPreviousNumberOfRenderedFrames)
				{
					const uint64_t index = (numberOfRenderedFrames % 4);
					glm::vec2 jitter = ::detail::hammersley2D(index, 4) * 2.0f - glm::vec2(1.0f);
					jitter *= 0.2f;
					const glm::vec2 jitterOffset = (jitter - mPreviousJitter) * 0.5f;
					mPreviousJitter = jitter;
					mPreviousNumberOfRenderedFrames = numberOfRenderedFrames;

					// Copy over
					memcpy(buffer, glm::value_ptr(jitterOffset), numberOfBytes);
				}
				else
				{
					// Copy over
					memcpy(buffer, glm::value_ptr(mPreviousJitter), numberOfBytes);
				}
				break;
			}

			case ::detail::HOSEK_WILKIE_SKY_COEFFICIENTS_1:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")

				// Calculate the data
				if (nullptr == mHosekWilkieSky)
				{
					mHosekWilkieSky = new HosekWilkieSky();
				}
				mHosekWilkieSky->recalculate(getWorldSpaceSunlightDirection());

				// Copy the data
				memcpy(buffer, glm::value_ptr(mHosekWilkieSky->getCoefficients().A), numberOfBytes);
				break;

			case ::detail::HOSEK_WILKIE_SKY_COEFFICIENTS_2:
				RHI_ASSERT(mRenderer->getContext(), sizeof(float) * 4 * 4 == numberOfBytes, "Invalid number of bytes")

				// Calculate the data
				if (nullptr == mHosekWilkieSky)
				{
					mHosekWilkieSky = new HosekWilkieSky();
				}
				mHosekWilkieSky->recalculate(getWorldSpaceSunlightDirection());
				{ // Set sunlight color
					// TODO(co) Get rid of the evil const-cast
					LightSceneItem* lightSceneItem = const_cast<LightSceneItem*>(mCompositorContextData->getLightSceneItem());
					if (nullptr != lightSceneItem)
					{
						lightSceneItem->setColor(mHosekWilkieSky->getSunColor());
					}
				}

				// Copy the data
				memcpy(buffer, glm::value_ptr(mHosekWilkieSky->getCoefficients().F) + 1, numberOfBytes);
				break;

			default:
				// Value not filled
				valueFilled = false;
				break;
		}

		// Done
		return valueFilled;
	}

	bool MaterialBlueprintResourceListener::fillInstanceValue(uint32_t referenceValue, uint8_t* buffer, [[maybe_unused]] uint32_t numberOfBytes, uint32_t instanceTextureBufferStartIndex)
	{
		bool valueFilled = true;

		// Resolve the reference value
		switch (referenceValue)
		{
			case ::detail::INSTANCE_INDICES:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(uint32_t) * 4 == numberOfBytes, "Invalid number of bytes")
				RHI_ASSERT(mRenderer->getContext(), ~0u != instanceTextureBufferStartIndex, "Invalid instance texture buffer start index")
				uint32_t* integerBuffer = reinterpret_cast<uint32_t*>(buffer);

				// 0 = x = The instance texture buffer start index
				integerBuffer[0] = instanceTextureBufferStartIndex;

				// 1 = y = The assigned material slot inside the material uniform buffer
				integerBuffer[1] = mMaterialTechnique->getAssignedMaterialSlot();

				// 2 = z = The custom parameters start index inside the instance texture buffer
				integerBuffer[2] = 0;

				// 3 = w = Unused
				integerBuffer[3] = 0;
				break;
			}

			case ::detail::WORLD_POSITION_MATERIAL_INDEX:
			{
				RHI_ASSERT(mRenderer->getContext(), sizeof(uint32_t) * 4 == numberOfBytes, "Invalid number of bytes")
				RHI_ASSERT(mRenderer->getContext(), ~0u == instanceTextureBufferStartIndex, "Invalid instance texture buffer start index")
				uint32_t* integerBuffer = reinterpret_cast<uint32_t*>(buffer);

				// xyz world position adjusted for camera relative rendering: While we're using a 64 bit world space position in general, for relative positions 32 bit are sufficient
				// -> 0 = World space x position
				// -> 1 = World space y position
				// -> 2 = World space z position
				*reinterpret_cast<float*>(integerBuffer)	 = static_cast<float>(mObjectSpaceToWorldSpaceTransform->position.x - mWorldSpaceCameraPosition.x);
				*reinterpret_cast<float*>(integerBuffer + 1) = static_cast<float>(mObjectSpaceToWorldSpaceTransform->position.y - mWorldSpaceCameraPosition.y);
				*reinterpret_cast<float*>(integerBuffer + 2) = static_cast<float>(mObjectSpaceToWorldSpaceTransform->position.z - mWorldSpaceCameraPosition.z);

				// 3 = w = The assigned material slot inside the material uniform buffer
				integerBuffer[3] = mMaterialTechnique->getAssignedMaterialSlot();
				break;
			}

			default:
				// Value not filled
				valueFilled = false;
				break;
		}

		// Done
		return valueFilled;
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	glm::vec3 MaterialBlueprintResourceListener::getWorldSpaceSunlightDirection() const
	{
		const LightSceneItem* lightSceneItem = mCompositorContextData->getLightSceneItem();
		if (nullptr != lightSceneItem && nullptr != lightSceneItem->getParentSceneNode())
		{
			return lightSceneItem->getParentSceneNode()->getGlobalTransform().rotation * Math::VEC3_FORWARD;
		}
		else
		{
			// Error!
			RHI_ASSERT(mRenderer->getContext(), false, "Invalid light scene item")
			return Math::VEC3_FORWARD;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
