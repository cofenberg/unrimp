/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Examples/Private/Renderer/Mesh/Mesh.h"
#include "Examples/Private/Framework/Color4.h"

#include <Renderer/Public/Context.h>
#include <Renderer/Public/IRenderer.h>
#include <Renderer/Public/Core/IProfiler.h>
#include <Renderer/Public/Resource/Mesh/MeshResource.h>
#include <Renderer/Public/Resource/Mesh/MeshResourceManager.h>
#include <Renderer/Public/Resource/Texture/TextureResource.h>
#include <Renderer/Public/Resource/Texture/TextureResourceManager.h>
#include <Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
Mesh::Mesh() :
	mMeshResourceId(Renderer::getInvalid<Renderer::MeshResourceId>()),
	m_argb_nxaTextureResourceId(Renderer::getInvalid<Renderer::TextureResourceId>()),
	m_hr_rg_mb_nyaTextureResourceId(Renderer::getInvalid<Renderer::TextureResourceId>()),
	mEmissiveTextureResourceId(Renderer::getInvalid<Renderer::TextureResourceId>()),
	mObjectSpaceToClipSpaceMatrixUniformHandle(NULL_HANDLE),
	mObjectSpaceToViewSpaceMatrixUniformHandle(NULL_HANDLE),
	mGlobalTimer(0.0f)
{
	// Nothing here
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void Mesh::onInitialization()
{
	// Get and check the renderer instance
	Renderer::IRenderer& renderer = getRendererSafe();
	Rhi::IRhiPtr rhi(getRhi());

	// Don't create initial pipeline state caches after a material blueprint has been loaded since this example isn't using the material blueprint system
	renderer.getMaterialBlueprintResourceManager().setCreateInitialPipelineStateCaches(false);

	{
		{ // Create the root signature
			Rhi::DescriptorRangeBuilder ranges[5];
			ranges[0].initialize(Rhi::ResourceType::UNIFORM_BUFFER, 0, "UniformBlockDynamicVs", Rhi::ShaderVisibility::VERTEX);
			ranges[1].initialize(Rhi::ResourceType::TEXTURE_2D,		0, "_argb_nxa",				Rhi::ShaderVisibility::FRAGMENT);
			ranges[2].initialize(Rhi::ResourceType::TEXTURE_2D,		1, "_hr_rg_mb_nya",			Rhi::ShaderVisibility::FRAGMENT);
			ranges[3].initialize(Rhi::ResourceType::TEXTURE_2D,		2, "EmissiveMap",			Rhi::ShaderVisibility::FRAGMENT);
			ranges[4].initializeSampler(0, Rhi::ShaderVisibility::FRAGMENT);

			Rhi::RootParameterBuilder rootParameters[2];
			rootParameters[0].initializeAsDescriptorTable(4, &ranges[0]);
			rootParameters[1].initializeAsDescriptorTable(1, &ranges[4]);

			// Setup
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = rhi->createRootSignature(rootSignatureBuilder);
		}

		// Create uniform buffer
		// -> Direct3D 9 does not support uniform buffers
		// -> Direct3D 10, 11 and 12 do not support individual uniforms
		// -> The RHI is just a light weight abstraction layer, so we need to handle the differences
		// -> Allocate enough memory for two 4x4 floating point matrices
		if (0 != rhi->getCapabilities().maximumUniformBufferSize)
		{
			mUniformBuffer = renderer.getBufferManager().createUniformBuffer(2 * 4 * 4 * sizeof(float), nullptr, Rhi::BufferUsage::DYNAMIC_DRAW);
		}

		// Vertex input layout
		static constexpr Rhi::VertexAttribute vertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_3,		// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"Position",									// name[32] (char)
				"POSITION",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,		// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_2,		// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"TexCoord",									// name[32] (char)
				"TEXCOORD",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 3,							// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,		// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 2
				// Data destination
				Rhi::VertexAttributeFormat::SHORT_4,		// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"QTangent",									// name[32] (char)
				"TEXCOORD",									// semanticName[32] (char)
				1,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 5,							// alignedByteOffset (uint32_t)
				sizeof(float) * 5 + sizeof(short) * 4,		// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			}
		};
		const Rhi::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayout)), vertexAttributesLayout);

		{ // Create sampler state and wrap it into a resource group instance
			Rhi::SamplerState samplerStateSettings = Rhi::ISamplerState::getDefaultSamplerState();
			samplerStateSettings.addressU = Rhi::TextureAddressMode::WRAP;
			samplerStateSettings.addressV = Rhi::TextureAddressMode::WRAP;
			Rhi::IResource* samplerStateResource = mSamplerStatePtr = rhi->createSamplerState(samplerStateSettings);
			mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
		}

		{ // Create the graphics program
			// Get the shader source code (outsourced to keep an overview)
			const char* vertexShaderSourceCode = nullptr;
			const char* fragmentShaderSourceCode = nullptr;
			#include "Mesh_GLSL_450.h"	// For Vulkan
			#include "Mesh_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
			#include "Mesh_GLSL_ES3.h"
			#include "Mesh_HLSL_D3D9.h"
			#include "Mesh_HLSL_D3D10_D3D11_D3D12.h"
			#include "Mesh_Null.h"

			// Create the graphics program
			Rhi::IShaderLanguage& shaderLanguage = rhi->getDefaultShaderLanguage();
			mGraphicsProgram = shaderLanguage.createGraphicsProgram(
				*mRootSignature,
				vertexAttributes,
				shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
				shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
		}

		// Is there a valid graphics program?
		if (nullptr != mGraphicsProgram)
		{
			// Create the graphics pipeline state object (PSO)
			mGraphicsPipelineState = rhi->createGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder(mRootSignature, mGraphicsProgram, vertexAttributes, getMainRenderTarget()->getRenderPass()));

			// Optimization: Cached data to not bother the RHI too much
			if (nullptr == mUniformBuffer)
			{
				mObjectSpaceToClipSpaceMatrixUniformHandle = mGraphicsProgram->getUniformHandle("ObjectSpaceToClipSpaceMatrix");
				mObjectSpaceToViewSpaceMatrixUniformHandle = mGraphicsProgram->getUniformHandle("ObjectSpaceToViewSpaceMatrix");
			}
		}

		// Create mesh instance
		renderer.getMeshResourceManager().loadMeshResourceByAssetId(ASSET_ID("Example/Mesh/Imrod/SM_Imrod"), mMeshResourceId, this);

		{ // Load in the albedo, emissive, normal and roughness texture
			Renderer::TextureResourceManager& textureResourceManager = renderer.getTextureResourceManager();
			textureResourceManager.loadTextureResourceByAssetId(ASSET_ID("Example/Mesh/Imrod/T_Imrod_argb_nxa"),     ASSET_ID("Unrimp/Texture/DynamicByCode/Identity_argb_nxa2D"),     m_argb_nxaTextureResourceId, this);
			textureResourceManager.loadTextureResourceByAssetId(ASSET_ID("Example/Mesh/Imrod/T_Imrod_hr_rg_mb_nya"), ASSET_ID("Unrimp/Texture/DynamicByCode/Identity_hr_rg_mb_nya2D"), m_hr_rg_mb_nyaTextureResourceId, this);
			textureResourceManager.loadTextureResourceByAssetId(ASSET_ID("Example/Mesh/Imrod/T_Imrod_e"),            ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityEmissiveMap2D"),   mEmissiveTextureResourceId, this);
		}
	}

	// Since we're always dispatching the same commands to the RHI, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
	fillCommandBuffer();
}

void Mesh::onDeinitialization()
{
	{ // Disconnect resource listeners
		const Renderer::IRenderer& renderer = getRendererSafe();
		const Renderer::TextureResourceManager& textureResourceManager = renderer.getTextureResourceManager();
		textureResourceManager.setInvalidResourceId(m_argb_nxaTextureResourceId, *this);
		textureResourceManager.setInvalidResourceId(m_hr_rg_mb_nyaTextureResourceId, *this);
		textureResourceManager.setInvalidResourceId(mEmissiveTextureResourceId, *this);
		renderer.getMeshResourceManager().setInvalidResourceId(mMeshResourceId, *this);
	}

	// Release the used RHI resources
	mObjectSpaceToViewSpaceMatrixUniformHandle = NULL_HANDLE;
	mObjectSpaceToClipSpaceMatrixUniformHandle = NULL_HANDLE;
	mSamplerStateGroup = nullptr;
	mSamplerStatePtr = nullptr;
	mResourceGroup = nullptr;
	mGraphicsProgram = nullptr;
	mGraphicsPipelineState = nullptr;
	mUniformBuffer = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
}

void Mesh::onUpdate()
{
	// Stop the stopwatch
	mStopwatch.stop();

	// Update the global timer (FPS independent movement)
	mGlobalTimer += mStopwatch.getMilliseconds() * 0.0005f;

	// Start the stopwatch
	mStopwatch.start();
}

void Mesh::onDraw(Rhi::CommandBuffer& commandBuffer)
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		{ // Set uniform
			// Get the aspect ratio
			float aspectRatio = 4.0f / 3.0f;
			{
				// Get the render target with and height
				const Rhi::IRenderTarget* renderTarget = getMainRenderTarget();
				if (nullptr != renderTarget)
				{
					uint32_t width  = 1;
					uint32_t height = 1;
					renderTarget->getWidthAndHeight(width, height);

					// Get the aspect ratio
					aspectRatio = static_cast<float>(width) / static_cast<float>(height);
				}
			}

			// Calculate the object space to clip space matrix
			const glm::mat4 viewSpaceToClipSpace	= glm::perspective(45.0f, aspectRatio, 100.0f, 0.1f);	// Near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
			const glm::mat4 viewTranslate			= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -7.0f, 25.0f));
			const glm::mat4 worldSpaceToViewSpace	= glm::rotate(viewTranslate, mGlobalTimer, glm::vec3(0.0f, 1.0f, 0.0f));
			const glm::mat4 objectSpaceToWorldSpace	= glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
				  glm::mat4 objectSpaceToViewSpace	= worldSpaceToViewSpace * objectSpaceToWorldSpace;
			const glm::mat4 objectSpaceToClipSpace	= viewSpaceToClipSpace * objectSpaceToViewSpace;

			// Upload the uniform data
			// -> Two versions: One using an uniform buffer and one setting an individual uniform
			if (nullptr != mUniformBuffer)
			{
				struct UniformBlockDynamicVs final
				{
					float objectSpaceToClipSpaceMatrix[4 * 4];	// Object space to clip space matrix
					float objectSpaceToViewSpaceMatrix[4 * 4];	// Object space to view space matrix
				};
				UniformBlockDynamicVs uniformBlockDynamicVS;
				memcpy(uniformBlockDynamicVS.objectSpaceToClipSpaceMatrix, glm::value_ptr(objectSpaceToClipSpace), sizeof(float) * 4 * 4);

				// TODO(co) float3x3 (currently there are alignment issues when using Direct3D, have a look into possible solutions)
				glm::mat3 objectSpaceToViewSpace3x3 = glm::mat3(objectSpaceToViewSpace);
				objectSpaceToViewSpace = glm::mat4(objectSpaceToViewSpace3x3);
				memcpy(uniformBlockDynamicVS.objectSpaceToViewSpaceMatrix, glm::value_ptr(objectSpaceToViewSpace), sizeof(float) * 4 * 4);

				// Copy data
				Rhi::MappedSubresource mappedSubresource;
				if (rhi->map(*mUniformBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, mappedSubresource))
				{
					memcpy(mappedSubresource.data, &uniformBlockDynamicVS, sizeof(UniformBlockDynamicVs));
					rhi->unmap(*mUniformBuffer, 0);
				}
			}
			else
			{
				// TODO(co) Not compatible with command buffer: This certainly is going to be removed, we need to implement internal uniform buffer emulation
				// Set uniforms
				mGraphicsProgram->setUniformMatrix4fv(mObjectSpaceToClipSpaceMatrixUniformHandle, glm::value_ptr(objectSpaceToClipSpace));
				mGraphicsProgram->setUniformMatrix3fv(mObjectSpaceToViewSpaceMatrixUniformHandle, glm::value_ptr(glm::mat3(objectSpaceToViewSpace)));
			}
		}

		// Dispatch pre-recorded command buffer, in case the referenced assets have already been loaded and the command buffer has been filled as a consequence
		if (!mCommandBuffer.isEmpty())
		{
			Rhi::Command::DispatchCommandBuffer::create(commandBuffer, &mCommandBuffer);
		}
	}
}

void Mesh::fillCommandBuffer()
{
	const Renderer::IRenderer& renderer = getRendererSafe();
	const Renderer::MeshResource* meshResource = renderer.getMeshResourceManager().tryGetById(mMeshResourceId);
	if (nullptr != meshResource && nullptr != meshResource->getVertexArrayPtr())
	{
		// Due to background texture loading, some textures might not be ready yet resulting in fallback texture usage
		// -> "Mesh::onLoadingStateChange()" will invalidate the resource group as soon as a texture resource finishes loading 
		if (nullptr == mResourceGroup)
		{
			const Renderer::TextureResourceManager& textureResourceManager = renderer.getTextureResourceManager();
			const Renderer::TextureResource* _argb_nxaTextureResource = textureResourceManager.tryGetById(m_argb_nxaTextureResourceId);
			const Renderer::TextureResource* _hr_rg_mb_nyaTextureResource = textureResourceManager.tryGetById(m_hr_rg_mb_nyaTextureResourceId);
			const Renderer::TextureResource* emissiveTextureResource = textureResourceManager.tryGetById(mEmissiveTextureResourceId);
			if (nullptr == _argb_nxaTextureResource || nullptr == _argb_nxaTextureResource->getTexturePtr() ||
				nullptr == _hr_rg_mb_nyaTextureResource || nullptr == _hr_rg_mb_nyaTextureResource->getTexturePtr() ||
				nullptr == emissiveTextureResource || nullptr == emissiveTextureResource->getTexturePtr())
			{
				return;
			}

			// Create resource group
			Rhi::IResource* resources[4] = { mUniformBuffer, _argb_nxaTextureResource->getTexturePtr(), _hr_rg_mb_nyaTextureResource->getTexturePtr(), emissiveTextureResource->getTexturePtr() };
			Rhi::ISamplerState* samplerStates[4] = { nullptr, mSamplerStatePtr, mSamplerStatePtr, mSamplerStatePtr };
			mResourceGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, samplerStates);
		}

		// Get and check the RHI instance
		Rhi::IRhiPtr rhi(getRhi());
		if (nullptr != rhi && nullptr != mGraphicsPipelineState)
		{
			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			RENDERER_SCOPED_PROFILER_EVENT(renderer.getContext(), mCommandBuffer, "Mesh")

			// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
			Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR_DEPTH, Color4::GRAY);

			// Set the used graphics root signature
			Rhi::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

			// Set the used graphics pipeline state object (PSO)
			Rhi::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

			// Set graphics resource groups
			Rhi::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mResourceGroup);
			Rhi::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 1, mSamplerStateGroup);

			{ // Draw mesh instance
				// Input assembly (IA): Set the used vertex array
				Rhi::Command::SetGraphicsVertexArray::create(mCommandBuffer, meshResource->getVertexArrayPtr());

				// Render the specified geometric primitive, based on indexing into an array of vertices
				Rhi::Command::DrawIndexedGraphics::create(mCommandBuffer, meshResource->getNumberOfIndices());
			}
		}
	}
}
