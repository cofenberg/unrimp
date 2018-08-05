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
#include "Basics/FirstComputeShader/FirstComputeShader.h"
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
void FirstComputeShader::onInitialization()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Create the buffer and texture manager
		mBufferManager = renderer->createBufferManager();
		mTextureManager = renderer->createTextureManager();

		{ // Create the graphics root signature
			// TODO(co) Compute shader: Get rid of the OpenGL/Direct3D 11 variation here
			const uint32_t offset = (renderer->getNameId() == Renderer::NameId::VULKAN || renderer->getNameId() == Renderer::NameId::OPENGL) ? 1u : 0u;
			Renderer::DescriptorRangeBuilder ranges[5];
			ranges[0].initialize(Renderer::ResourceType::UNIFORM_BUFFER,	0,			 "UniformBuffer",		  Renderer::ShaderVisibility::FRAGMENT);
			ranges[1].initialize(Renderer::ResourceType::TEXTURE_BUFFER,	0,			 "InputTextureBuffer",	  Renderer::ShaderVisibility::VERTEX);
			ranges[2].initialize(Renderer::ResourceType::STRUCTURED_BUFFER, 1u + offset, "InputStructuredBuffer", Renderer::ShaderVisibility::VERTEX);
			ranges[3].initialize(Renderer::ResourceType::TEXTURE_2D,		1,			 "AlbedoMap",			  Renderer::ShaderVisibility::FRAGMENT);
			ranges[4].initializeSampler(0, Renderer::ShaderVisibility::FRAGMENT);

			Renderer::RootParameterBuilder rootParameters[2];
			rootParameters[0].initializeAsDescriptorTable(4, &ranges[0]);
			rootParameters[1].initializeAsDescriptorTable(1, &ranges[4]);

			// Setup
			Renderer::RootSignatureBuilder rootSignature;
			rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mGraphicsRootSignature = renderer->createRootSignature(rootSignature);
		}

		{ // Create the first compute root signature
			Renderer::DescriptorRangeBuilder ranges[7];
			// Input
			ranges[0].initialize(Renderer::ResourceType::TEXTURE_2D,	  0,			"InputTexture2D",	  Renderer::ShaderVisibility::COMPUTE);
			ranges[1].initialize(Renderer::ResourceType::INDEX_BUFFER,    1,			"InputIndexBuffer",	  Renderer::ShaderVisibility::COMPUTE);
			ranges[2].initialize(Renderer::ResourceType::VERTEX_BUFFER,   2,			"InputVertexBuffer",  Renderer::ShaderVisibility::COMPUTE);
			ranges[3].initialize(Renderer::ResourceType::UNIFORM_BUFFER,  0,			"InputUniformBuffer", Renderer::ShaderVisibility::COMPUTE);
			// Output
			// TODO(co) Compute shader: Get rid of the OpenGL/Direct3D 11 variation here
			const uint32_t offset = (renderer->getNameId() == Renderer::NameId::VULKAN || renderer->getNameId() == Renderer::NameId::OPENGL) ? 4u : 0u;
			ranges[4].initialize(Renderer::ResourceType::TEXTURE_2D,	  0u + offset, "OutputTexture2D",	  Renderer::ShaderVisibility::COMPUTE, Renderer::DescriptorRangeType::UAV);
			ranges[5].initialize(Renderer::ResourceType::INDEX_BUFFER,    1u + offset, "OutputIndexBuffer",	  Renderer::ShaderVisibility::COMPUTE, Renderer::DescriptorRangeType::UAV);
			ranges[6].initialize(Renderer::ResourceType::VERTEX_BUFFER,   2u + offset, "OutputVertexBuffer",  Renderer::ShaderVisibility::COMPUTE, Renderer::DescriptorRangeType::UAV);

			Renderer::RootParameterBuilder rootParameters[1];
			rootParameters[0].initializeAsDescriptorTable(7, &ranges[0]);

			// Setup
			Renderer::RootSignatureBuilder rootSignature;
			rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::NONE);

			// Create the instance
			mComputeRootSignature1 = renderer->createRootSignature(rootSignature);
		}

		{ // Create the second compute root signature
			Renderer::DescriptorRangeBuilder ranges[6];
			// Input
			ranges[0].initialize(Renderer::ResourceType::TEXTURE_BUFFER,	0,			 "InputTextureBuffer",	   Renderer::ShaderVisibility::COMPUTE);
			ranges[1].initialize(Renderer::ResourceType::STRUCTURED_BUFFER, 1,			 "InputStructuredBuffer",  Renderer::ShaderVisibility::COMPUTE);
			ranges[2].initialize(Renderer::ResourceType::INDIRECT_BUFFER,	2,			 "InputIndirectBuffer",	   Renderer::ShaderVisibility::COMPUTE);
			// Output
			// TODO(co) Compute shader: Get rid of the OpenGL/Direct3D 11 variation here
			const uint32_t offset = (renderer->getNameId() == Renderer::NameId::VULKAN || renderer->getNameId() == Renderer::NameId::OPENGL) ? 3u : 0u;
			ranges[3].initialize(Renderer::ResourceType::TEXTURE_BUFFER,	0u + offset, "OutputTextureBuffer",	   Renderer::ShaderVisibility::COMPUTE, Renderer::DescriptorRangeType::UAV);
			ranges[4].initialize(Renderer::ResourceType::STRUCTURED_BUFFER, 1u + offset, "OutputStructuredBuffer", Renderer::ShaderVisibility::COMPUTE, Renderer::DescriptorRangeType::UAV);
			ranges[5].initialize(Renderer::ResourceType::INDIRECT_BUFFER,	2u + offset, "OutputIndirectBuffer",   Renderer::ShaderVisibility::COMPUTE, Renderer::DescriptorRangeType::UAV);

			Renderer::RootParameterBuilder rootParameters[1];
			rootParameters[0].initializeAsDescriptorTable(6, &ranges[0]);

			// Setup
			Renderer::RootSignatureBuilder rootSignature;
			rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::NONE);

			// Create the instance
			mComputeRootSignature2 = renderer->createRootSignature(rootSignature);
		}

		// Create sampler state and wrap it into a resource group instance
		Renderer::IResource* samplerStateResource = nullptr;
		{
			Renderer::SamplerState samplerState = Renderer::ISamplerState::getDefaultSamplerState();
			samplerState.maxLOD = 0.0f;
			samplerStateResource = renderer->createSamplerState(samplerState);
			mGraphicsSamplerStateGroup = mGraphicsRootSignature->createResourceGroup(1, 1, &samplerStateResource);
		}

		{ // Texture buffer
			static constexpr float VERTEX_POSITION_OFFSET[] =
			{								// Vertex ID	Triangle on screen
				0.5f, -0.5f, 0.0f, 0.0f,	// 0				0
				0.5f, -0.5f, 0.0f, 0.0f,	// 1			   .   .
				0.5f, -0.5f, 0.0f, 0.0f,	// 2			  2.......1
			};

			// Create the texture buffer which will be read by a compute shader
			mComputeInputTextureBuffer = mBufferManager->createTextureBuffer(sizeof(VERTEX_POSITION_OFFSET), VERTEX_POSITION_OFFSET);

			// Create the texture buffer which will be filled by a compute shader
			mComputeOutputTextureBuffer = mBufferManager->createTextureBuffer(sizeof(VERTEX_POSITION_OFFSET), nullptr, Renderer::BufferFlag::UNORDERED_ACCESS | Renderer::BufferFlag::SHADER_RESOURCE);
		}

		{ // Structured buffer
			struct Vertex
			{
				float position[2];
				float padding[2];
			};
			static constexpr Vertex VERTICES[] =
			{									// Vertex ID	Triangle on screen
				{ -0.5f, 0.5f, 0.0f, 0.0f },	// 0				0
				{ -0.5f, 0.5f, 0.0f, 0.0f },	// 1			   .   .
				{ -0.5f, 0.5f, 0.0f, 0.0f },	// 2			  2.......1
			};

			// Create the structured buffer which will be read by a compute shader
			mComputeInputStructuredBuffer = mBufferManager->createStructuredBuffer(sizeof(VERTICES), VERTICES, Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage::STATIC_DRAW, sizeof(Vertex));

			// Create the structured buffer which will be filled by a compute shader
			mComputeOutputStructuredBuffer = mBufferManager->createStructuredBuffer(sizeof(VERTICES), nullptr, Renderer::BufferFlag::UNORDERED_ACCESS | Renderer::BufferFlag::SHADER_RESOURCE, Renderer::BufferUsage::STATIC_DRAW, sizeof(Vertex));
		}

		{ // Indirect buffer
			{ // Create the indirect buffer which will be read by a compute shader
				const Renderer::DrawIndexedInstancedArguments drawIndexedInstancedArguments =
				{
					0, // indexCountPerInstance (uint32_t)	- Filled by compute shader via atomics counting
					1, // instanceCount (uint32_t)
					0, // startIndexLocation (uint32_t)
					0, // baseVertexLocation (int32_t)
					0  // startInstanceLocation (uint32_t)
				};
				mComputeInputIndirectBuffer = mBufferManager->createIndirectBuffer(sizeof(Renderer::DrawIndexedInstancedArguments), &drawIndexedInstancedArguments, Renderer::IndirectBufferFlag::SHADER_RESOURCE | Renderer::IndirectBufferFlag::DRAW_INDEXED_INSTANCED_ARGUMENTS);
			}

			// Create the indirect buffer which will be filled by a compute shader
			mComputeOutputIndirectBuffer = mBufferManager->createIndirectBuffer(sizeof(Renderer::DrawIndexedInstancedArguments), nullptr, Renderer::IndirectBufferFlag::UNORDERED_ACCESS | Renderer::IndirectBufferFlag::DRAW_INDEXED_INSTANCED_ARGUMENTS);
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

		{ // Create the index buffer object (IBO)
			static constexpr uint16_t INDICES[] =
			{
				0, 1, 2
			};
			mComputeInputIndexBuffer = mBufferManager->createIndexBuffer(sizeof(INDICES), INDICES, Renderer::BufferFlag::SHADER_RESOURCE);
			mComputeOutputIndexBuffer = mBufferManager->createIndexBuffer(sizeof(INDICES), nullptr, Renderer::BufferFlag::UNORDERED_ACCESS);
		}

		{ // Create vertex array object (VAO)
			// Create the vertex buffer object (VBO)
			// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
			static constexpr float VERTEX_POSITION[] =
			{					// Vertex ID	Triangle on screen
				 0.0f, 1.0f,	// 0				0
				 1.0f, 0.0f,	// 1			   .   .
				-0.5f, 0.0f		// 2			  2.......1
			};
			mComputeInputVertexBuffer = mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferFlag::SHADER_RESOURCE);
			mComputeOutputVertexBuffer = mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), nullptr, Renderer::BufferFlag::UNORDERED_ACCESS);
		}

		{ // Create vertex array object (VAO)
		  // -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		  // -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		  // -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		  //    reference of the used vertex buffer objects (VBO). If the reference counter of a
		  //    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { mComputeOutputVertexBuffer };
			mVertexArray = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, mComputeOutputIndexBuffer);
		}

		{ // Create the uniform buffer which will be read by a compute shader
			static constexpr float RGBA_COLOR[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			mComputeInputUniformBuffer = mBufferManager->createUniformBuffer(sizeof(RGBA_COLOR), RGBA_COLOR);
		}

		{ // Resource group related
			// Create the texture instance, but without providing texture data (we use the texture as render target)
			// -> Use the "Renderer::TextureFlag::RENDER_TARGET"-flag to mark this texture as a render target
			// -> Required for Vulkan, Direct3D 9, Direct3D 10, Direct3D 11 and Direct3D 12
			// -> Not required for OpenGL and OpenGL ES 3
			// -> The optimized texture clear value is a Direct3D 12 related option
			const Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::Enum::R8G8B8A8;
			Renderer::ITexture* computeInputTexture2D = mTextureManager->createTexture2D(16, 16, textureFormat, nullptr, Renderer::TextureFlag::SHADER_RESOURCE | Renderer::TextureFlag::RENDER_TARGET, Renderer::TextureUsage::DEFAULT, 1, reinterpret_cast<const Renderer::OptimizedTextureClearValue*>(&Color4::GREEN));
			Renderer::ITexture* computeOutputTexture2D = mTextureManager->createTexture2D(16, 16, textureFormat, nullptr, Renderer::TextureFlag::SHADER_RESOURCE | Renderer::TextureFlag::UNORDERED_ACCESS);

			{ // Create the framebuffer object (FBO) instance
				const Renderer::FramebufferAttachment colorFramebufferAttachment(computeInputTexture2D);
				mFramebuffer = renderer->createFramebuffer(*renderer->createRenderPass(1, &textureFormat), &colorFramebufferAttachment);
			}

			{ // Create first compute resource group
				Renderer::IResource* resources[7] = {
					// Input
					computeInputTexture2D, mComputeInputIndexBuffer, mComputeInputVertexBuffer, mComputeInputUniformBuffer,
					// Output
					computeOutputTexture2D, mComputeOutputIndexBuffer, mComputeOutputVertexBuffer
				};
				Renderer::ISamplerState* samplerStates[7] = {
					// Input
					static_cast<Renderer::ISamplerState*>(samplerStateResource), nullptr, nullptr, nullptr,
					// Output
					nullptr, nullptr, nullptr
				};
				mComputeResourceGroup1 = mComputeRootSignature1->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources, samplerStates);
			}

			{ // Create second compute resource group
				Renderer::IResource* resources[6] = {
					// Input
					mComputeInputTextureBuffer, mComputeInputStructuredBuffer, mComputeInputIndirectBuffer,
					// Output
					mComputeOutputTextureBuffer, mComputeOutputStructuredBuffer, mComputeOutputIndirectBuffer
				};
				mComputeResourceGroup2 = mComputeRootSignature2->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources, nullptr);
			}

			{ // Create graphics resource group
				Renderer::IResource* resources[4] = { mComputeInputUniformBuffer, mComputeOutputTextureBuffer, mComputeOutputStructuredBuffer, computeOutputTexture2D };
				Renderer::ISamplerState* samplerStates[4] = { nullptr, nullptr, nullptr, static_cast<Renderer::ISamplerState*>(samplerStateResource) };
				mGraphicsResourceGroup = mGraphicsRootSignature->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources, samplerStates);
			}
		}

		// Create the graphics program: Decide which shader language should be used (for example "GLSL" or "HLSL")
		Renderer::IShaderLanguagePtr shaderLanguage(renderer->getShaderLanguage());
		if (nullptr != shaderLanguage)
		{
			// Create the graphics program
			Renderer::IGraphicsProgramPtr graphicsProgram;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				const char* computeShaderSourceCode1 = nullptr;
				const char* computeShaderSourceCode2 = nullptr;
				#include "FirstComputeShader_GLSL_450.h"	// For Vulkan
				#include "FirstComputeShader_GLSL_430.h"	// macOS 10.11 only supports OpenGL 4.1 and hence can't be supported by this example
				#include "FirstComputeShader_HLSL_D3D11_D3D12.h"
				#include "FirstComputeShader_Null.h"

				// Create the graphics program
				graphicsProgram = shaderLanguage->createGraphicsProgram(
					*mGraphicsRootSignature,
					vertexAttributes,
					shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));

				// Create the compute pipeline state objects (PSO)
				mComputePipelineState1 = renderer->createComputePipelineState(*mComputeRootSignature1, *shaderLanguage->createComputeShaderFromSourceCode(computeShaderSourceCode1));
				mComputePipelineState2 = renderer->createComputePipelineState(*mComputeRootSignature2, *shaderLanguage->createComputeShaderFromSourceCode(computeShaderSourceCode2));
			}

			// Create the graphics pipeline state object (PSO)
			if (nullptr != graphicsProgram)
			{
				mGraphicsPipelineState = renderer->createGraphicsPipelineState(Renderer::GraphicsPipelineStateBuilder(mGraphicsRootSignature, graphicsProgram, vertexAttributes, getMainRenderTarget()->getRenderPass()));
			}
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstComputeShader::onDeinitialization()
{
	// Release the used resources
	mComputeInputUniformBuffer = nullptr;
	mComputeOutputIndirectBuffer = nullptr;
	mComputeInputIndirectBuffer = nullptr;
	mComputeOutputStructuredBuffer = nullptr;
	mComputeInputStructuredBuffer = nullptr;
	mComputeOutputTextureBuffer = nullptr;
	mComputeInputTextureBuffer = nullptr;
	mVertexArray = nullptr;
	mComputeOutputVertexBuffer = nullptr;
	mComputeInputVertexBuffer = nullptr;
	mComputeOutputIndexBuffer = nullptr;
	mComputeInputIndexBuffer = nullptr;
	mComputePipelineState2 = nullptr;
	mComputePipelineState1 = nullptr;
	mGraphicsPipelineState = nullptr;
	mGraphicsSamplerStateGroup = nullptr;
	mGraphicsResourceGroup = nullptr;
	mComputeResourceGroup2 = nullptr;
	mComputeResourceGroup1 = nullptr;
	mFramebuffer = nullptr;
	mComputeRootSignature2 = nullptr;
	mComputeRootSignature1 = nullptr;
	mGraphicsRootSignature = nullptr;
	mCommandBuffer.clear();
	mTextureManager = nullptr;
	mBufferManager = nullptr;
}

void FirstComputeShader::onDraw()
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
void FirstComputeShader::fillCommandBuffer()
{
	// Sanity checks
	assert(nullptr != getRenderer());
	assert(nullptr != getMainRenderTarget());
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mGraphicsRootSignature);
	assert(nullptr != mComputeRootSignature1);
	assert(nullptr != mComputeRootSignature2);
	assert(nullptr != mFramebuffer);
	assert(nullptr != mComputeResourceGroup1);
	assert(nullptr != mComputeResourceGroup2);
	assert(nullptr != mGraphicsResourceGroup);
	assert(nullptr != mGraphicsSamplerStateGroup);
	assert(nullptr != mGraphicsPipelineState);
	assert(nullptr != mComputePipelineState1);
	assert(nullptr != mComputePipelineState2);
	assert(nullptr != mComputeInputIndexBuffer);
	assert(nullptr != mComputeOutputIndexBuffer);
	assert(nullptr != mComputeInputVertexBuffer);
	assert(nullptr != mComputeOutputVertexBuffer);
	assert(nullptr != mVertexArray);
	assert(nullptr != mComputeOutputTextureBuffer);
	assert(nullptr != mComputeInputTextureBuffer);
	assert(nullptr != mComputeOutputStructuredBuffer);
	assert(nullptr != mComputeInputStructuredBuffer);
	assert(nullptr != mComputeOutputIndirectBuffer);
	assert(nullptr != mComputeInputIndirectBuffer);
	assert(nullptr != mComputeInputUniformBuffer);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	{ // Graphics: Render to texture
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Render to texture")

		// This in here is of course just an example. In a real application
		// there would be no point in constantly updating texture content
		// without having any real change.

		// Set the graphics render target to render into
		Renderer::Command::SetGraphicsRenderTarget::create(mCommandBuffer, mFramebuffer);

		// Clear the graphics color buffer of the current render target with green
		Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR, Color4::GREEN);

		// Restore graphics main swap chain as current render target
		Renderer::Command::SetGraphicsRenderTarget::create(mCommandBuffer, getMainRenderTarget());
	}

	{ // Compute: Use the graphics render to texture result for compute
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Use the render to texture result for compute")

		// Set the used compute root signature
		Renderer::Command::SetComputeRootSignature::create(mCommandBuffer, mComputeRootSignature1);

		// Set the used compute pipeline state object (PSO)
		Renderer::Command::SetComputePipelineState::create(mCommandBuffer, mComputePipelineState1);

		// Set compute resource groups
		Renderer::Command::SetComputeResourceGroup::create(mCommandBuffer, 0, mComputeResourceGroup1);

		// Dispatch compute call
		Renderer::Command::DispatchCompute::create(mCommandBuffer, 1, 1, 1);

		// Repeat everything for the second compute shader
		Renderer::Command::SetComputeRootSignature::create(mCommandBuffer, mComputeRootSignature2);
		Renderer::Command::SetComputePipelineState::create(mCommandBuffer, mComputePipelineState2);
		Renderer::Command::SetComputeResourceGroup::create(mCommandBuffer, 0, mComputeResourceGroup2);
		Renderer::Command::DispatchCompute::create(mCommandBuffer, 1, 1, 1);
	}

	{ // Graphics: Use the compute result for graphics
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Use the compute result")

		// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
		Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		// Set the used graphics root signature
		Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mGraphicsRootSignature);

		// Set the used graphics pipeline state object (PSO)
		Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

		// Set graphics resource groups
		Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mGraphicsResourceGroup);
		Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 1, mGraphicsSamplerStateGroup);

		// Input assembly (IA): Set the used vertex array
		Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

		// Render the specified geometric primitive, based on indexing into an array of vertices
		Renderer::Command::DrawIndexedGraphics::create(mCommandBuffer, *mComputeOutputIndirectBuffer);
	}
}
