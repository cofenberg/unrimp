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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Scene/CompositorInstancePassScene.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorResourcePassShadowMap.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class CompositorResourcePassCompute;
	class CompositorInstancePassCompute;
	class CompositorResourcePassShadowMap;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t TextureResourceId;	///< POD texture resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compositor instance pass shadow map
	*
	*  @note
	*    - Basing on https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/ - https://github.com/TheRealMJP/Shadows
	*/
	class CompositorInstancePassShadowMap final : public CompositorInstancePassScene
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorPassFactory;	// The only one allowed to create instances of this class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct PassData final
		{
			int			shadowMapSize = 0;
			glm::mat4	shadowMatrix;
			float		shadowCascadeSplits[CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES] = {};
			glm::vec4	shadowCascadeOffsets[CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES] = {};
			glm::vec4	shadowCascadeScales[CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES];
			uint8_t		currentShadowCascadeIndex = 0;
			float		shadowFilterSize = 0.0f;
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Settings                                              ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline bool isEnabled() const
		{
			return mEnabled;
		}

		inline void setEnabled(bool enabled)
		{
			if (mEnabled != enabled)
			{
				mEnabled = enabled;
				++mSettingsGenerationCounter;
			}
		}

		[[nodiscard]] inline uint32_t getShadowMapSize() const
		{
			return mShadowMapSize;
		}

		inline void setShadowMapSize(uint32_t shadowMapSize)
		{
			if (mShadowMapSize != shadowMapSize)
			{
				mShadowMapSize = shadowMapSize;
				++mSettingsGenerationCounter;
			}
		}

		[[nodiscard]] inline uint8_t getNumberOfShadowCascades() const
		{
			return mNumberOfShadowCascades;
		}

		RENDERERRUNTIME_API_EXPORT void setNumberOfShadowCascades(uint8_t numberOfShadowCascades);

		[[nodiscard]] inline uint8_t getNumberOfShadowMultisamples() const
		{
			return mNumberOfShadowMultisamples;
		}

		RENDERERRUNTIME_API_EXPORT void setNumberOfShadowMultisamples(uint8_t numberOfShadowMultisamples);

		[[nodiscard]] inline float getCascadeSplitsLambda() const
		{
			return mCascadeSplitsLambda;
		}

		inline void setCascadeSplitsLambda(float cascadeSplitsLambda)
		{
			mCascadeSplitsLambda = cascadeSplitsLambda;
		}

		[[nodiscard]] inline float getShadowFilterSize() const
		{
			return mShadowFilterSize;
		}

		inline void setShadowFilterSize(float shadowFilterSize)
		{
			mShadowFilterSize = shadowFilterSize;
		}

		[[nodiscard]] inline bool getStabilizeCascades() const
		{
			return mStabilizeCascades;
		}

		inline void setStabilizeCascades(bool stabilizeCascades)
		{
			mStabilizeCascades = stabilizeCascades;
		}

		//[-------------------------------------------------------]
		//[ Internal                                              ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const PassData& getPassData() const
		{
			return mPassData;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onFillCommandBuffer(const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		CompositorInstancePassShadowMap(const CompositorResourcePassShadowMap& compositorResourcePassShadowMap, const CompositorNodeInstance& compositorNodeInstance);

		inline virtual ~CompositorInstancePassShadowMap() override
		{
			destroyShadowMapRenderTarget();
		}

		explicit CompositorInstancePassShadowMap(const CompositorInstancePassShadowMap&) = delete;
		CompositorInstancePassShadowMap& operator=(const CompositorInstancePassShadowMap&) = delete;
		void createShadowMapRenderTarget();
		void destroyShadowMapRenderTarget();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Settings
		bool	 mEnabled;						///< Shadow enabled?
		uint32_t mShadowMapSize;				///< The shadow map size is usually 512, 1024 or 2048
		uint8_t  mNumberOfShadowCascades;		///< Number of shadow cascades, usually 4
		uint8_t  mNumberOfShadowMultisamples;	///< The number of shadow multisamples per pixel (valid values: 1, 2, 4, 8)
		float	 mCascadeSplitsLambda;			///< Cascade splits lambda
		float	 mShadowFilterSize;				///< Shadow filter size
		bool	 mStabilizeCascades;			///< Keeps consistent sizes for each cascade, and snaps each cascade so that they move in texel-sized increments. Reduces temporal aliasing artifacts, but reduces the effective resolution of the cascades. See Valient, M., "Stable Rendering of Cascaded Shadow Maps", In: Engel, W. F ., et al., "ShaderX6: Advanced Rendering Techniques", Charles River Media, 2008, ISBN 1-58450-544-3.
		// Internal
		uint32_t					   mSettingsGenerationCounter;	// Most simple solution to detect settings changes which make internal data invalid
		uint32_t					   mUsedSettingsGenerationCounter;
		PassData					   mPassData;
		Rhi::IFramebufferPtr		   mDepthFramebufferPtr;
		Rhi::IFramebufferPtr		   mVarianceFramebufferPtr[CompositorResourcePassShadowMap::MAXIMUM_NUMBER_OF_SHADOW_CASCADES];
		Rhi::IFramebufferPtr		   mIntermediateFramebufferPtr;
		TextureResourceId			   mDepthTextureResourceId;
		TextureResourceId			   mVarianceTextureResourceId;
		TextureResourceId			   mIntermediateDepthBlurTextureResourceId;
		CompositorResourcePassCompute* mDepthToExponentialVarianceCompositorResourcePassCompute;
		CompositorInstancePassCompute* mDepthToExponentialVarianceCompositorInstancePassCompute;
		CompositorResourcePassCompute* mHorizontalBlurCompositorResourcePassCompute;
		CompositorInstancePassCompute* mHorizontalBlurCompositorInstancePassCompute;
		CompositorResourcePassCompute* mVerticalBlurCompositorResourcePassCompute;
		CompositorInstancePassCompute* mVerticalBlurCompositorInstancePassCompute;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
