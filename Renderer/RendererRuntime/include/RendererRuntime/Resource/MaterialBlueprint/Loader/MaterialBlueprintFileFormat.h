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
#include "RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResource.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	// Material blueprint file format content:
	// - File format header
	// - Material blueprint header
	// - Material blueprint properties
	// - Visual importance of shader properties
	// - Root signature
	// - Pipeline state object (PSO)
	//   - Shader blueprints, rasterization state etc.
	// - Resources
	//   - Uniform buffers
	//   - Texture buffers
	//   - Sampler states
	//   - Textures
	namespace v1MaterialBlueprint
	{


		//[-------------------------------------------------------]
		//[ Definitions                                           ]
		//[-------------------------------------------------------]
		static constexpr uint32_t FORMAT_TYPE	 = STRING_ID("MaterialBlueprint");
		static constexpr uint32_t FORMAT_VERSION = 9;

		#pragma pack(push)
		#pragma pack(1)
			struct MaterialBlueprintHeader final
			{
				uint32_t numberOfProperties;
				uint32_t numberOfShaderCombinationProperties;
				uint32_t numberOfIntegerShaderCombinationProperties;
				uint32_t numberOfUniformBuffers;
				uint32_t numberOfTextureBuffers;
				uint32_t numberOfSamplerStates;
				uint32_t numberOfTextures;
			};

			struct RootSignatureHeader final
			{
				uint32_t numberOfRootParameters;
				uint32_t numberOfDescriptorRanges;
				uint32_t numberOfStaticSamplers;
				uint32_t flags;
			};

			struct UniformBufferHeader final
			{
				uint32_t							   rootParameterIndex;			///< Root parameter index = resource group index
				MaterialBlueprintResource::BufferUsage bufferUsage;
				uint32_t							   numberOfElements;
				uint32_t							   numberOfElementProperties;
				uint32_t							   uniformBufferNumberOfBytes;	///< Includes handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
			};

			struct TextureBufferHeader final
			{
				MaterialPropertyValue				   materialPropertyValue = MaterialPropertyValue::fromUnknown();
				uint32_t							   rootParameterIndex	 = getUninitialized<uint32_t>();	///< Root parameter index = resource group index
				MaterialBlueprintResource::BufferUsage bufferUsage			 = MaterialBlueprintResource::BufferUsage::UNKNOWN;
			};

			struct SamplerState final
			{
				Renderer::SamplerState samplerState;
				uint32_t			   rootParameterIndex;	///< Root parameter index = resource group index
			};

			struct Texture final
			{
				uint32_t		 rootParameterIndex;	///< Root parameter index = resource group index
				MaterialProperty materialProperty;
				AssetId			 fallbackTextureAssetId;
				bool			 rgbHardwareGammaCorrection;
				uint32_t		 samplerStateIndex;		///< Index of the material blueprint sampler state resource to use, can be uninitialized (e.g. texel fetch instead of sampling might be used)

				Texture() :
					rootParameterIndex(getUninitialized<uint32_t>()),
					rgbHardwareGammaCorrection(false),
					samplerStateIndex(getUninitialized<uint32_t>())
				{
					// Nothing here
				}

				Texture(uint32_t _rootParameterIndex, MaterialProperty _materialProperty, AssetId _fallbackTextureAssetId, bool _rgbHardwareGammaCorrection, uint32_t _samplerStateIndex) :
					rootParameterIndex(_rootParameterIndex),
					materialProperty(_materialProperty),
					fallbackTextureAssetId(_fallbackTextureAssetId),
					rgbHardwareGammaCorrection(_rgbHardwareGammaCorrection),
					samplerStateIndex(_samplerStateIndex)
				{
					// Nothing here
				}
			};
		#pragma pack(pop)


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // v1Material
} // RendererRuntime
