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
#include "RendererRuntime/Public/Core/Renderer/FramebufferSignature.h"
#include "RendererRuntime/Public/Core/Renderer/RenderTargetTextureSignature.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;					///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef StringId CompositorChannelId;		///< Compositor channel identifier, internally just a POD "uint32_t"
	typedef StringId CompositorFramebufferId;	///< Compositor framebuffer identifier, internally just a POD "uint32_t"
	typedef StringId CompositorPassTypeId;		///< Compositor pass type identifier, internally just a POD "uint32_t"
	typedef uint32_t MaterialTechniqueId;		///< Material technique identifier, result of hashing the material technique name via "RendererRuntime::StringId"
	typedef StringId MaterialPropertyId;		///< Material property identifier, internally just a POD "uint32_t", result of hashing the property name


	// Compositor node file format content:
	// - File format header
	// - Compositor node header
	namespace v1CompositorNode
	{


		//[-------------------------------------------------------]
		//[ Definitions                                           ]
		//[-------------------------------------------------------]
		static constexpr uint32_t FORMAT_TYPE	 = STRING_ID("CompositorNode");
		static constexpr uint32_t FORMAT_VERSION = 9;

		#pragma pack(push)
		#pragma pack(1)
			struct CompositorNodeHeader final
			{
				uint32_t numberOfInputChannels;
				uint32_t numberOfRenderTargetTextures;
				uint32_t numberOfFramebuffers;
				uint32_t numberOfTargets;
				uint32_t numberOfOutputChannels;
			};

			struct Channel final
			{
				CompositorChannelId id;
			};

			struct RenderTargetTexture final
			{
				AssetId						 assetId;
				RenderTargetTextureSignature renderTargetTextureSignature;
			};

			struct Framebuffer final
			{
				CompositorFramebufferId compositorFramebufferId;
				FramebufferSignature	framebufferSignature;
			};

			struct Target final
			{
				CompositorChannelId		compositorChannelId;
				CompositorFramebufferId compositorFramebufferId;
				uint32_t				numberOfPasses;
			};

			struct PassHeader final
			{
				CompositorPassTypeId compositorPassTypeId;
				uint32_t			 numberOfBytes;
			};

			// Keep this in sync with "RendererRuntime::ICompositorResourcePass::deserialize() -> PassData"
			struct Pass
			{
				static constexpr uint32_t MAXIMUM_PASS_NAME_LENGTH = 63 + 1;	// +1 for the terminating zero

				char	 name[MAXIMUM_PASS_NAME_LENGTH] = { "Compositor pass" };	///< Human readable ASCII pass name for debugging and profiling, contains terminating zero
				float	 minimumDepth					= 0.0f;
				float	 maximumDepth					= 1.0f;
				uint32_t numberOfExecutions				= RendererRuntime::getInvalid<uint32_t>();
				bool	 skipFirstExecution				= false;
			};

			struct PassClear final : public Pass
			{
				uint32_t flags	  = 0;		///< Combination of "Renderer::ClearFlag"
				float	 color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				float	 z		  = 0.0f;	///< 0 instead of 1 due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				uint32_t stencil  = 0;
			};

			struct PassVrHiddenAreaMesh final : public Pass
			{
				uint32_t flags	 = 0;	///< Combination of "Renderer::ClearFlag", except for color-flag
				uint32_t stencil = 0;
			};

			struct PassScene : public Pass
			{
				uint8_t				minimumRenderQueueIndex = 0;	///< Inclusive
				uint8_t				maximumRenderQueueIndex = 255;	///< Inclusive
				bool				transparentPass			= false;
				MaterialTechniqueId	materialTechniqueId;
			};

			struct PassShadowMap final : public PassScene
			{
				AssetId textureAssetId;
				AssetId depthToExponentialVarianceMaterialBlueprintAssetId;
				AssetId blurMaterialBlueprintAssetId;
			};

			struct PassResolveMultisample final : public Pass
			{
				CompositorFramebufferId sourceMultisampleCompositorFramebufferId;
			};

			struct PassCopy final : public Pass
			{
				AssetId destinationTextureAssetId;
				AssetId sourceTextureAssetId;
			};

			struct PassGenerateMipmaps final : public Pass
			{
				AssetId			   textureAssetId;
				AssetId			   materialBlueprintAssetId;
				MaterialPropertyId textureMaterialBlueprintProperty;
			};

			struct PassCompute : public Pass
			{
				AssetId				materialAssetId;				///< If material blueprint asset ID is set, material asset ID must be invalid
				MaterialTechniqueId	materialTechniqueId;			///< Must always be valid
				AssetId				materialBlueprintAssetId;		///< If material asset ID is set, material blueprint asset ID must be invalid
				uint32_t			numberOfMaterialProperties = 0;
			};

			// The material definition is not mandatory for the debug GUI, if nothing is defined the fixed build in renderer configuration resources will be used instead
			struct PassDebugGui final : public PassCompute
			{
			};
		#pragma pack(pop)


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // v1CompositorNode
} // RendererRuntime
