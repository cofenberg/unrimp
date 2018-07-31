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
#include "Basics/FirstMultipleRenderTargets/FirstMultipleRenderTargets.h"
#include "Framework/Color4.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstMultipleRenderTargets::onInitialization()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Sanity check
		assert(nullptr != getMainRenderTarget());

		// Create the buffer and texture manager
		mBufferManager = renderer->createBufferManager();
		mTextureManager = renderer->createTextureManager();

		// Check whether or not multiple simultaneous render targets are supported
		if (renderer->getCapabilities().maximumNumberOfSimultaneousRenderTargets > 1)
		{
			{ // Create the root signature
				Renderer::DescriptorRangeBuilder ranges[3];
				ranges[0].initialize(Renderer::ResourceType::TEXTURE_2D, 0, "AlbedoMap0", Renderer::ShaderVisibility::FRAGMENT, Renderer::DescriptorRangeType::SRV, 1);
				ranges[1].initialize(Renderer::ResourceType::TEXTURE_2D, 1, "AlbedoMap1", Renderer::ShaderVisibility::FRAGMENT, Renderer::DescriptorRangeType::SRV, 1);
				ranges[2].initializeSampler(0, Renderer::ShaderVisibility::FRAGMENT);

				Renderer::RootParameterBuilder rootParameters[2];
				rootParameters[0].initializeAsDescriptorTable(2, &ranges[0]);
				rootParameters[1].initializeAsDescriptorTable(1, &ranges[2]);

				// Setup
				Renderer::RootSignatureBuilder rootSignature;
				rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

				// Create the instance
				mRootSignature = renderer->createRootSignature(rootSignature);
			}

			// Create sampler state and wrap it into a resource group instance: We don't use mipmaps
			Renderer::IResource* samplerStateResource = nullptr;
			{
				Renderer::SamplerState samplerState = Renderer::ISamplerState::getDefaultSamplerState();
				samplerState.filter = Renderer::FilterMode::MIN_MAG_MIP_POINT;
				samplerState.maxLOD = 0.0f;
				samplerStateResource = renderer->createSamplerState(samplerState);
				mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
			}

			{ // Texture resource related
				// Create the texture instances, but without providing texture data (we use the texture as render target)
				// -> Use the "Renderer::TextureFlag::RENDER_TARGET"-flag to mark this texture as a render target
				// -> Required for Vulkan, Direct3D 9, Direct3D 10, Direct3D 11 and Direct3D 12
				// -> Not required for OpenGL and OpenGL ES 3
				// -> The optimized texture clear value is a Direct3D 12 related option
				Renderer::TextureFormat::Enum textureFormats[NUMBER_OF_TEXTURES];
				Renderer::IResource* textureResource[NUMBER_OF_TEXTURES] = {};
				Renderer::ISamplerState* samplerStates[NUMBER_OF_TEXTURES] = {};
				Renderer::FramebufferAttachment colorFramebufferAttachments[NUMBER_OF_TEXTURES];
				for (uint32_t i = 0; i < NUMBER_OF_TEXTURES; ++i)
				{
					textureFormats[i] = Renderer::TextureFormat::R8G8B8A8;
					textureResource[i] = colorFramebufferAttachments[i].texture = mTextureManager->createTexture2D(TEXTURE_SIZE, TEXTURE_SIZE, textureFormats[i], nullptr, Renderer::TextureFlag::SHADER_RESOURCE | Renderer::TextureFlag::RENDER_TARGET, Renderer::TextureUsage::DEFAULT, 1, reinterpret_cast<const Renderer::OptimizedTextureClearValue*>(&Color4::BLACK));
					samplerStates[i] = static_cast<Renderer::ISamplerState*>(samplerStateResource);
				}

				// Create texture group
				mTextureGroup = mRootSignature->createResourceGroup(0, NUMBER_OF_TEXTURES, textureResource, samplerStates);

				// Create the framebuffer object (FBO) instance
				mFramebuffer = renderer->createFramebuffer(*renderer->createRenderPass(NUMBER_OF_TEXTURES, textureFormats), colorFramebufferAttachments);
			}

			// Vertex input layout
			static constexpr Renderer::VertexAttribute vertexAttributesLayout[] =
			{
				{ // Attribute 0
					// Data destination
					Renderer::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
					"Position",									// name[32] (char)
					"POSITION",									// semanticName[32] (char)
					0,											// semanticIndex (uint32_t)
					// Data source
					0,											// inputSlot (uint32_t)
					0,											// alignedByteOffset (uint32_t)
					sizeof(float) * 2,							// strideInBytes (uint32_t)
					0											// instancesPerElement (uint32_t)
				}
			};
			const Renderer::VertexAttributes vertexAttributes(static_cast<uint32_t>(glm::countof(vertexAttributesLayout)), vertexAttributesLayout);

			{ // Create vertex array object (VAO)
				// Create the vertex buffer object (VBO)
				// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
				static constexpr float VERTEX_POSITION[] =
				{					// Vertex ID	Triangle on screen
					 0.0f, 1.0f,	// 0				0
					 1.0f, 0.0f,	// 1			   .   .
					-0.5f, 0.0f		// 2			  2.......1
				};
				Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));

				// Create vertex array object (VAO)
				// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
				// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
				// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
				//    reference of the used vertex buffer objects (VBO). If the reference counter of a
				//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
				const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
				mVertexArray = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
			}

			// Create the programs: Decide which shader language should be used (for example "GLSL" or "HLSL")
			Renderer::IShaderLanguagePtr shaderLanguage(renderer->getShaderLanguage());
			if (nullptr != shaderLanguage)
			{
				// Create the programs
				Renderer::IProgramPtr programMultipleRenderTargets;
				Renderer::IProgramPtr program;
				{
					// Get the shader source code (outsourced to keep an overview)
					const char* vertexShaderSourceCode = nullptr;
					const char* fragmentShaderSourceCode_MultipleRenderTargets = nullptr;
					const char* fragmentShaderSourceCode = nullptr;
					#include "FirstMultipleRenderTargets_GLSL_450.h"	// For Vulkan
					#include "FirstMultipleRenderTargets_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
					#include "FirstMultipleRenderTargets_GLSL_ES3.h"
					#include "FirstMultipleRenderTargets_HLSL_D3D9.h"
					#include "FirstMultipleRenderTargets_HLSL_D3D10_D3D11_D3D12.h"
					#include "FirstMultipleRenderTargets_Null.h"

					// In order to keep this example simple and to show that it's possible, we use the same vertex shader for both programs
					// -> Depending on the used graphics API and whether or not the shader compiler & linker is clever,
					//    the unused texture coordinate might get optimized out
					// -> In a real world application you shouldn't rely on shader compiler & linker behaviour assumptions
					Renderer::IVertexShaderPtr vertexShader(shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode));
					programMultipleRenderTargets = shaderLanguage->createProgram(*mRootSignature, vertexAttributes, vertexShader, shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode_MultipleRenderTargets));
					program = shaderLanguage->createProgram(*mRootSignature, vertexAttributes, vertexShader, shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
				}

				// Create the graphics pipeline state objects (PSO)
				if (nullptr != programMultipleRenderTargets && nullptr != program)
				{
					{
						Renderer::GraphicsPipelineState graphicsPipelineState = Renderer::GraphicsPipelineStateBuilder(mRootSignature, programMultipleRenderTargets, vertexAttributes, mFramebuffer->getRenderPass());
						graphicsPipelineState.numberOfRenderTargets = NUMBER_OF_TEXTURES;
						graphicsPipelineState.depthStencilState.depthEnable = 0;
						graphicsPipelineState.depthStencilViewFormat = Renderer::TextureFormat::UNKNOWN;
						mGraphicsPipelineStateMultipleRenderTargets = renderer->createGraphicsPipelineState(graphicsPipelineState);
					}
					mGraphicsPipelineState = renderer->createGraphicsPipelineState(Renderer::GraphicsPipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass()));
				}
			}
		}
		else
		{
			// Error!
			RENDERER_LOG(renderer->getContext(), CRITICAL, "This example requires support for multiple simultaneous render targets")
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstMultipleRenderTargets::onDeinitialization()
{
	// Release the used resources
	mVertexArray = nullptr;
	mGraphicsPipelineState = nullptr;
	mGraphicsPipelineStateMultipleRenderTargets = nullptr;
	mSamplerStateGroup = nullptr;
	mTextureGroup = nullptr;
	mFramebuffer = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mTextureManager = nullptr;
	mBufferManager = nullptr;
}

void FirstMultipleRenderTargets::onDraw()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Submit command buffer to the renderer backend
		mCommandBuffer.submitToRenderer(*renderer);
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstMultipleRenderTargets::fillCommandBuffer()
{
	// Sanity checks
	assert(nullptr != getRenderer());
	assert(nullptr != getMainRenderTarget());
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mFramebuffer);
	assert(nullptr != mTextureGroup);
	assert(nullptr != mSamplerStateGroup);
	assert(nullptr != mGraphicsPipelineStateMultipleRenderTargets);
	assert(nullptr != mGraphicsPipelineState);
	assert(nullptr != mVertexArray);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	{ // Render to multiple render targets
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Render to multiple render targets")

		// This in here is of course just an example. In a real application
		// there would be no point in constantly updating texture content
		// without having any real change.

		// Set the graphics render target to render into
		Renderer::Command::SetGraphicsRenderTarget::create(mCommandBuffer, mFramebuffer);

		// Set the graphics viewport and scissor rectangle
		Renderer::Command::SetGraphicsViewportAndScissorRectangle::create(mCommandBuffer, 0, 0, TEXTURE_SIZE, TEXTURE_SIZE);

		// Clear the graphics color buffer of the current render targets with black
		Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR, Color4::BLACK);

		// Set the used graphics root signature
		Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

		// Set the used graphics pipeline state object (PSO)
		Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineStateMultipleRenderTargets);

		// Input assembly (IA): Set the used vertex array
		Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

		// Render the specified geometric primitive, based on an array of vertices
		Renderer::Command::DrawGraphics::create(mCommandBuffer, 3);

		// Restore graphics main swap chain as current render target
		Renderer::Command::SetGraphicsRenderTarget::create(mCommandBuffer, getMainRenderTarget());
	}

	{ // Use the render to multiple render targets result
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Use the render to multiple render targets result")

		{ // Set the viewport
			// Get the render target with and height
			uint32_t width  = 1;
			uint32_t height = 1;
			const Renderer::IRenderTarget* renderTarget = getMainRenderTarget();
			if (nullptr != renderTarget)
			{
				renderTarget->getWidthAndHeight(width, height);
			}

			// Set the graphics viewport and scissor rectangle
			Renderer::Command::SetGraphicsViewportAndScissorRectangle::create(mCommandBuffer, 0, 0, width, height);
		}

		// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
		Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		// Set the used graphics root signature
		Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

		// Set the used graphics pipeline state object (PSO)
		Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

		// Set graphics resource groups
		Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mTextureGroup);
		Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 1, mSamplerStateGroup);

		// Input assembly (IA): Set the used vertex array
		Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

		// Render the specified geometric primitive, based on an array of vertices
		Renderer::Command::DrawGraphics::create(mCommandBuffer, 3);
	}
}
