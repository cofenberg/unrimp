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
#include "Runtime/FirstMesh/FirstMesh.h"
#include "Framework/Color4.h"

#include <RendererRuntime/Context.h>
#include <RendererRuntime/IRendererRuntime.h>
#include <RendererRuntime/Core/IProfiler.h>
#include <RendererRuntime/Resource/Mesh/MeshResource.h>
#include <RendererRuntime/Resource/Mesh/MeshResourceManager.h>
#include <RendererRuntime/Resource/Texture/TextureResource.h>
#include <RendererRuntime/Resource/Texture/TextureResourceManager.h>
#include <RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h>

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
FirstMesh::FirstMesh() :
	mMeshResourceId(RendererRuntime::getInvalid<RendererRuntime::MeshResourceId>()),
	m_argb_nxaTextureResourceId(RendererRuntime::getInvalid<RendererRuntime::TextureResourceId>()),
	m_hr_rg_mb_nyaTextureResourceId(RendererRuntime::getInvalid<RendererRuntime::TextureResourceId>()),
	mEmissiveTextureResourceId(RendererRuntime::getInvalid<RendererRuntime::TextureResourceId>()),
	mObjectSpaceToClipSpaceMatrixUniformHandle(NULL_HANDLE),
	mObjectSpaceToViewSpaceMatrixUniformHandle(NULL_HANDLE),
	mGlobalTimer(0.0f)
{
	// Nothing here
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstMesh::onInitialization()
{
	// Get and check the renderer runtime instance
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		Renderer::IRendererPtr renderer(getRenderer());

		// Don't create initial pipeline state caches after a material blueprint has been loaded since this example isn't using the material blueprint system
		rendererRuntime->getMaterialBlueprintResourceManager().setCreateInitialPipelineStateCaches(false);

		// Decide which shader language should be used (for example "GLSL" or "HLSL")
		Renderer::IShaderLanguagePtr shaderLanguage(renderer->getShaderLanguage());
		if (nullptr != shaderLanguage)
		{
			{ // Create the root signature
				Renderer::DescriptorRangeBuilder ranges[5];
				ranges[0].initialize(Renderer::DescriptorRangeType::UBV, 1, 0, "UniformBlockDynamicVs", Renderer::ShaderVisibility::VERTEX);
				ranges[1].initialize(Renderer::DescriptorRangeType::SRV, 1, 0, "_argb_nxa", Renderer::ShaderVisibility::FRAGMENT);
				ranges[2].initialize(Renderer::DescriptorRangeType::SRV, 1, 1, "_hr_rg_mb_nya", Renderer::ShaderVisibility::FRAGMENT);
				ranges[3].initialize(Renderer::DescriptorRangeType::SRV, 1, 2, "EmissiveMap", Renderer::ShaderVisibility::FRAGMENT);
				ranges[4].initializeSampler(1, 0, Renderer::ShaderVisibility::FRAGMENT);

				Renderer::RootParameterBuilder rootParameters[2];
				rootParameters[0].initializeAsDescriptorTable(4, &ranges[0]);
				rootParameters[1].initializeAsDescriptorTable(1, &ranges[4]);

				// Setup
				Renderer::RootSignatureBuilder rootSignature;
				rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

				// Create the instance
				mRootSignature = renderer->createRootSignature(rootSignature);
			}

			// Create uniform buffer
			// -> Direct3D 9 does not support uniform buffers
			// -> Direct3D 10, 11 and 12 do not support individual uniforms
			// -> The renderer is just a light weight abstraction layer, so we need to handle the differences
			// -> Allocate enough memory for two 4x4 floating point matrices
			mUniformBuffer = rendererRuntime->getBufferManager().createUniformBuffer(2 * 4 * 4 * sizeof(float), nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);

			// Vertex input layout
			static constexpr Renderer::VertexAttribute vertexAttributesLayout[] =
			{
				{ // Attribute 0
					// Data destination
					Renderer::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
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
					Renderer::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
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
					Renderer::VertexAttributeFormat::SHORT_4,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
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
			const Renderer::VertexAttributes vertexAttributes(static_cast<uint32_t>(glm::countof(vertexAttributesLayout)), vertexAttributesLayout);

			{ // Create sampler state and wrap it into a resource group instance
				Renderer::SamplerState samplerStateSettings = Renderer::ISamplerState::getDefaultSamplerState();
				samplerStateSettings.addressU = Renderer::TextureAddressMode::WRAP;
				samplerStateSettings.addressV = Renderer::TextureAddressMode::WRAP;
				Renderer::IResource* samplerStateResource = mSamplerStatePtr = renderer->createSamplerState(samplerStateSettings);
				mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
			}

			// Create the program
			Renderer::IProgramPtr program;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "FirstMesh_GLSL_450.h"	// For Vulkan
				#include "FirstMesh_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "FirstMesh_GLSL_ES3.h"
				#include "FirstMesh_HLSL_D3D9.h"
				#include "FirstMesh_HLSL_D3D10_D3D11_D3D12.h"
				#include "FirstMesh_Null.h"

				// Create the program
				mProgram = program = shaderLanguage->createProgram(
					*mRootSignature,
					vertexAttributes,
					shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
			}

			// Is there a valid program?
			if (nullptr != program)
			{
				// Create the graphics pipeline state object (PSO)
				mGraphicsPipelineState = renderer->createGraphicsPipelineState(Renderer::GraphicsPipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass()));

				// Optimization: Cached data to not bother the renderer API too much
				if (nullptr == mUniformBuffer)
				{
					mObjectSpaceToClipSpaceMatrixUniformHandle = program->getUniformHandle("ObjectSpaceToClipSpaceMatrix");
					mObjectSpaceToViewSpaceMatrixUniformHandle = program->getUniformHandle("ObjectSpaceToViewSpaceMatrix");
				}
			}

			// Create mesh instance
			rendererRuntime->getMeshResourceManager().loadMeshResourceByAssetId(STRING_ID("Example/Mesh/Character/Imrod"), mMeshResourceId);

			{ // Load in the albedo, emissive, normal and roughness texture
				RendererRuntime::TextureResourceManager& textureResourceManager = rendererRuntime->getTextureResourceManager();
				textureResourceManager.loadTextureResourceByAssetId(STRING_ID("Example/Texture/Character/Imrod_argb_nxa"),     STRING_ID("Unrimp/Texture/DynamicByCode/Identity_argb_nxa2D"),     m_argb_nxaTextureResourceId, this);
				textureResourceManager.loadTextureResourceByAssetId(STRING_ID("Example/Texture/Character/Imrod_hr_rg_mb_nya"), STRING_ID("Unrimp/Texture/DynamicByCode/Identity_hr_rg_mb_nya2D"), m_hr_rg_mb_nyaTextureResourceId, this);
				textureResourceManager.loadTextureResourceByAssetId(STRING_ID("Example/Texture/Character/Imrod_e"),            STRING_ID("Unrimp/Texture/DynamicByCode/IdentityEmissiveMap2D"),   mEmissiveTextureResourceId, this);
			}
		}
	}
}

void FirstMesh::onDeinitialization()
{
	// Release the used renderer resources
	mObjectSpaceToViewSpaceMatrixUniformHandle = NULL_HANDLE;
	mObjectSpaceToClipSpaceMatrixUniformHandle = NULL_HANDLE;
	mSamplerStateGroup = nullptr;
	mSamplerStatePtr = nullptr;
	mResourceGroup = nullptr;
	RendererRuntime::setInvalid(mEmissiveTextureResourceId);
	RendererRuntime::setInvalid(m_hr_rg_mb_nyaTextureResourceId);
	RendererRuntime::setInvalid(m_argb_nxaTextureResourceId);
	RendererRuntime::setInvalid(mMeshResourceId);
	mProgram = nullptr;
	mGraphicsPipelineState = nullptr;
	mUniformBuffer = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
}

void FirstMesh::onUpdate()
{
	// Stop the stopwatch
	mStopwatch.stop();

	// Update the global timer (FPS independent movement)
	mGlobalTimer += mStopwatch.getMilliseconds() * 0.0005f;

	// Start the stopwatch
	mStopwatch.start();
}

void FirstMesh::onDraw()
{
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr == rendererRuntime)
	{
		return;
	}

	// Due to background texture loading, some textures might not be ready, yet
	const RendererRuntime::TextureResourceManager& textureResourceManager = rendererRuntime->getTextureResourceManager();
	const RendererRuntime::TextureResource* _argb_nxaTextureResource = textureResourceManager.tryGetById(m_argb_nxaTextureResourceId);
	const RendererRuntime::TextureResource* _hr_rg_mb_nyaTextureResource = textureResourceManager.tryGetById(m_hr_rg_mb_nyaTextureResourceId);
	const RendererRuntime::TextureResource* emissiveTextureResource = textureResourceManager.tryGetById(mEmissiveTextureResourceId);
	if (nullptr == _argb_nxaTextureResource || nullptr == _argb_nxaTextureResource->getTexture() ||
		nullptr == _hr_rg_mb_nyaTextureResource || nullptr == _hr_rg_mb_nyaTextureResource->getTexture() ||
		nullptr == emissiveTextureResource || nullptr == emissiveTextureResource->getTexture())
	{
		return;
	}
	if (nullptr == mResourceGroup)
	{
		// Create resource group
		Renderer::IResource* resources[4] = { mUniformBuffer, _argb_nxaTextureResource->getTexture(), _hr_rg_mb_nyaTextureResource->getTexture(), emissiveTextureResource->getTexture() };
		Renderer::ISamplerState* samplerStates[4] = { nullptr, mSamplerStatePtr, mSamplerStatePtr, mSamplerStatePtr };
		mResourceGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources, samplerStates);
	}

	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer && nullptr != mGraphicsPipelineState)
	{
		// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
		RENDERER_SCOPED_PROFILER_EVENT_FUNCTION(rendererRuntime->getContext(), mCommandBuffer)

		// Set the viewport and get the aspect ratio
		float aspectRatio = 4.0f / 3.0f;
		{
			// Get the render target with and height
			const Renderer::IRenderTarget* renderTarget = getMainRenderTarget();
			if (nullptr != renderTarget)
			{
				uint32_t width  = 1;
				uint32_t height = 1;
				renderTarget->getWidthAndHeight(width, height);

				// Get the aspect ratio
				aspectRatio = static_cast<float>(width) / height;
			}
		}

		// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
		Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		// Set the used graphics root signature
		Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

		// Set the used graphics pipeline state object (PSO)
		Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

		// Set graphics resource groups
		Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mResourceGroup);
		Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 1, mSamplerStateGroup);

		{ // Set uniform
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
				Renderer::MappedSubresource mappedSubresource;
				if (renderer->map(*mUniformBuffer, 0, Renderer::MapType::WRITE_DISCARD, 0, mappedSubresource))
				{
					memcpy(mappedSubresource.data, &uniformBlockDynamicVS, sizeof(UniformBlockDynamicVs));
					renderer->unmap(*mUniformBuffer, 0);
				}
			}
			else
			{
				// TODO(co) Not compatible with command buffer: This certainly is going to be removed, we need to implement internal uniform buffer emulation
				// Set uniforms
				mProgram->setUniformMatrix4fv(mObjectSpaceToClipSpaceMatrixUniformHandle, glm::value_ptr(objectSpaceToClipSpace));
				mProgram->setUniformMatrix3fv(mObjectSpaceToViewSpaceMatrixUniformHandle, glm::value_ptr(glm::mat3(objectSpaceToViewSpace)));
			}
		}

		{ // Draw mesh instance
			const RendererRuntime::MeshResource* meshResource = rendererRuntime->getMeshResourceManager().tryGetById(mMeshResourceId);
			if (nullptr != meshResource && nullptr != meshResource->getVertexArrayPtr())
			{
				// Input assembly (IA): Set the used vertex array
				Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, meshResource->getVertexArrayPtr());

				// Render the specified geometric primitive, based on indexing into an array of vertices
				Renderer::Command::DrawIndexedGraphics::create(mCommandBuffer, meshResource->getNumberOfIndices());
			}
		}

		// Submit command buffer to the renderer backend
		mCommandBuffer.submitToRendererAndClear(*renderer);
	}
}
