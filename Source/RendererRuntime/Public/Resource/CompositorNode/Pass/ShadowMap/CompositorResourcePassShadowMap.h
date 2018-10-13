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
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;	///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compositor resource pass shadow map
	*/
	class CompositorResourcePassShadowMap final : public CompositorResourcePassScene
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorPassFactory;	// The only one allowed to create instances of this class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("ShadowMap");
		static constexpr uint32_t MAXIMUM_NUMBER_OF_SHADOW_CASCADES = 4;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline AssetId getTextureAssetId() const
		{
			return mTextureAssetId;
		}

		inline uint32_t getShadowMapSize() const
		{
			return mShadowMapSize;
		}

		inline uint8_t getNumberOfShadowCascades() const
		{
			return mNumberOfShadowCascades;
		}

		inline uint8_t getNumberOfShadowMultisamples() const
		{
			return mNumberOfShadowMultisamples;
		}

		inline float getCascadeSplitsLambda() const
		{
			return mCascadeSplitsLambda;
		}

		inline AssetId getDepthToExponentialVarianceMaterialBlueprintAssetId() const
		{
			return mDepthToExponentialVarianceMaterialBlueprintAssetId;
		}

		inline AssetId getBlurMaterialBlueprintAssetId() const
		{
			return mBlurMaterialBlueprintAssetId;
		}

		inline float getShadowFilterSize() const
		{
			return mShadowFilterSize;
		}

		inline bool getStabilizeCascades() const
		{
			return mStabilizeCascades;
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorResourcePass methods ]
	//[-------------------------------------------------------]
	public:
		inline virtual CompositorPassTypeId getTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void deserialize(uint32_t numberOfBytes, const uint8_t* data) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit CompositorResourcePassShadowMap(const CompositorTarget& compositorTarget) :
			CompositorResourcePassScene(compositorTarget),
			mShadowMapSize(1024),
			mNumberOfShadowCascades(4),
			mNumberOfShadowMultisamples(4),
			mCascadeSplitsLambda(0.99f),
			mShadowFilterSize(8.0f),
			mStabilizeCascades(true)
		{
			// Nothing here
		}

		inline virtual ~CompositorResourcePassShadowMap() override
		{
			// Nothing here
		}

		explicit CompositorResourcePassShadowMap(const CompositorResourcePassShadowMap&) = delete;
		CompositorResourcePassShadowMap& operator=(const CompositorResourcePassShadowMap&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		AssetId	 mTextureAssetId;										///< Shadow map texture asset ID
		uint32_t mShadowMapSize;										///< The shadow map size is usually 512, 1024 or 2048
		uint8_t  mNumberOfShadowCascades;								///< Number of shadow cascades, usually 4
		uint8_t  mNumberOfShadowMultisamples;							///< The number of shadow multisamples per pixel (valid values: 1, 2, 4, 8)
		float	 mCascadeSplitsLambda;									///< Cascade splits lambda
		AssetId  mDepthToExponentialVarianceMaterialBlueprintAssetId;	///< Depth to exponential variance material blueprint asset ID
		AssetId  mBlurMaterialBlueprintAssetId;							///< Blur material blueprint asset ID
		float	 mShadowFilterSize;										///< Shadow filter size
		bool	 mStabilizeCascades;									///< Keeps consistent sizes for each cascade, and snaps each cascade so that they move in texel-sized increments. Reduces temporal aliasing artifacts, but reduces the effective resolution of the cascades. See Valient, M., "Stable Rendering of Cascaded Shadow Maps", In: Engel, W. F ., et al., "ShaderX6: Advanced Rendering Techniques", Charles River Media, 2008, ISBN 1-58450-544-3.


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
