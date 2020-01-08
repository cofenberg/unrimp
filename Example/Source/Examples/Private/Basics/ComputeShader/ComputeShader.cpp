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
#include "Examples/Private/Basics/ComputeShader/ComputeShader.h"
#include "Examples/Private/Framework/Color4.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void ComputeShader::onInitialization()
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Create the buffer and texture manager
		mBufferManager = rhi->createBufferManager();
		mTextureManager = rhi->createTextureManager();

		{ // Create the graphics root signature
			// TODO(co) Compute shader: Get rid of the OpenGL/Direct3D 11 variation here
			const uint32_t offset = (rhi->getNameId() == Rhi::NameId::VULKAN || rhi->getNameId() == Rhi::NameId::OPENGL) ? 1u : 0u;
			Rhi::DescriptorRangeBuilder ranges[5];
			ranges[0].initialize(Rhi::ResourceType::UNIFORM_BUFFER,	   0,			"UniformBuffer",		 Rhi::ShaderVisibility::FRAGMENT);
			ranges[1].initialize(Rhi::ResourceType::TEXTURE_BUFFER,	   0,			"InputTextureBuffer",	 Rhi::ShaderVisibility::VERTEX);
			ranges[2].initialize(Rhi::ResourceType::STRUCTURED_BUFFER, 1u + offset, "InputStructuredBuffer", Rhi::ShaderVisibility::VERTEX);
			ranges[3].initialize(Rhi::ResourceType::TEXTURE_2D,		   1,			"AlbedoMap",			 Rhi::ShaderVisibility::FRAGMENT);
			ranges[4].initializeSampler(0, Rhi::ShaderVisibility::FRAGMENT);

			Rhi::RootParameterBuilder rootParameters[2];
			rootParameters[0].initializeAsDescriptorTable(4, &ranges[0]);
			rootParameters[1].initializeAsDescriptorTable(1, &ranges[4]);

			// Setup
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mGraphicsRootSignature = rhi->createRootSignature(rootSignatureBuilder);
		}

		{ // Create the first compute root signature
			Rhi::DescriptorRangeBuilder ranges[7];
			// Input
			ranges[0].initialize(Rhi::ResourceType::TEXTURE_2D,		 0,	"InputTexture2D",	  Rhi::ShaderVisibility::COMPUTE);
			ranges[1].initialize(Rhi::ResourceType::VERTEX_BUFFER,   1,	"InputVertexBuffer",  Rhi::ShaderVisibility::COMPUTE);
			ranges[2].initialize(Rhi::ResourceType::INDEX_BUFFER,    2,	"InputIndexBuffer",	  Rhi::ShaderVisibility::COMPUTE);
			ranges[3].initialize(Rhi::ResourceType::UNIFORM_BUFFER,  0,	"InputUniformBuffer", Rhi::ShaderVisibility::COMPUTE);
			// Output
			// TODO(co) Compute shader: Get rid of the OpenGL/Direct3D 11 variation here
			const uint32_t offset = (rhi->getNameId() == Rhi::NameId::VULKAN || rhi->getNameId() == Rhi::NameId::OPENGL) ? 4u : 0u;
			ranges[4].initialize(Rhi::ResourceType::TEXTURE_2D,		 0u + offset, "OutputTexture2D",	 Rhi::ShaderVisibility::COMPUTE, Rhi::DescriptorRangeType::UAV);
			ranges[5].initialize(Rhi::ResourceType::VERTEX_BUFFER,   1u + offset, "OutputVertexBuffer",  Rhi::ShaderVisibility::COMPUTE, Rhi::DescriptorRangeType::UAV);
			ranges[6].initialize(Rhi::ResourceType::INDEX_BUFFER,    2u + offset, "OutputIndexBuffer",	 Rhi::ShaderVisibility::COMPUTE, Rhi::DescriptorRangeType::UAV);

			Rhi::RootParameterBuilder rootParameters[1];
			rootParameters[0].initializeAsDescriptorTable(7, &ranges[0]);

			// Setup
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::NONE);

			// Create the instance
			mComputeRootSignature1 = rhi->createRootSignature(rootSignatureBuilder);
		}

		{ // Create the second compute root signature
			Rhi::DescriptorRangeBuilder ranges[6];
			// Input
			ranges[0].initialize(Rhi::ResourceType::TEXTURE_BUFFER,	   0, "InputTextureBuffer",	    Rhi::ShaderVisibility::COMPUTE);
			ranges[1].initialize(Rhi::ResourceType::STRUCTURED_BUFFER, 1, "InputStructuredBuffer",  Rhi::ShaderVisibility::COMPUTE);
			ranges[2].initialize(Rhi::ResourceType::INDIRECT_BUFFER,   2, "InputIndirectBuffer",	Rhi::ShaderVisibility::COMPUTE);
			// Output
			// TODO(co) Compute shader: Get rid of the OpenGL/Direct3D 11 variation here
			const uint32_t offset = (rhi->getNameId() == Rhi::NameId::VULKAN || rhi->getNameId() == Rhi::NameId::OPENGL) ? 3u : 0u;
			ranges[3].initialize(Rhi::ResourceType::TEXTURE_BUFFER,	   0u + offset, "OutputTextureBuffer",	  Rhi::ShaderVisibility::COMPUTE, Rhi::DescriptorRangeType::UAV);
			ranges[4].initialize(Rhi::ResourceType::STRUCTURED_BUFFER, 1u + offset, "OutputStructuredBuffer", Rhi::ShaderVisibility::COMPUTE, Rhi::DescriptorRangeType::UAV);
			ranges[5].initialize(Rhi::ResourceType::INDIRECT_BUFFER,   2u + offset, "OutputIndirectBuffer",   Rhi::ShaderVisibility::COMPUTE, Rhi::DescriptorRangeType::UAV);

			Rhi::RootParameterBuilder rootParameters[1];
			rootParameters[0].initializeAsDescriptorTable(6, &ranges[0]);

			// Setup
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::NONE);

			// Create the instance
			mComputeRootSignature2 = rhi->createRootSignature(rootSignatureBuilder);
		}

		// Create sampler state and wrap it into a resource group instance
		Rhi::IResource* samplerStateResource = nullptr;
		{
			Rhi::SamplerState samplerState = Rhi::ISamplerState::getDefaultSamplerState();
			samplerState.maxLod = 0.0f;
			samplerStateResource = rhi->createSamplerState(samplerState);
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
			mComputeOutputTextureBuffer = mBufferManager->createTextureBuffer(sizeof(VERTEX_POSITION_OFFSET), nullptr, Rhi::BufferFlag::UNORDERED_ACCESS | Rhi::BufferFlag::SHADER_RESOURCE);
		}

		{ // Structured buffer
			struct Vertex final
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
			mComputeInputStructuredBuffer = mBufferManager->createStructuredBuffer(sizeof(VERTICES), VERTICES, Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage::STATIC_DRAW, sizeof(Vertex));

			// Create the structured buffer which will be filled by a compute shader
			mComputeOutputStructuredBuffer = mBufferManager->createStructuredBuffer(sizeof(VERTICES), nullptr, Rhi::BufferFlag::UNORDERED_ACCESS | Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage::STATIC_DRAW, sizeof(Vertex));
		}

		{ // Indirect buffer
			{ // Create the indirect buffer which will be read by a compute shader
				const Rhi::DrawIndexedArguments drawIndexedArguments =
				{
					0, // indexCountPerInstance (uint32_t)	- Filled by compute shader via atomics counting
					1, // instanceCount (uint32_t)
					0, // startIndexLocation (uint32_t)
					0, // baseVertexLocation (int32_t)
					0  // startInstanceLocation (uint32_t)
				};
				mComputeInputIndirectBuffer = mBufferManager->createIndirectBuffer(sizeof(Rhi::DrawIndexedArguments), &drawIndexedArguments, Rhi::IndirectBufferFlag::SHADER_RESOURCE | Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS);
			}

			// Create the indirect buffer which will be filled by a compute shader
			mComputeOutputIndirectBuffer = mBufferManager->createIndirectBuffer(sizeof(Rhi::DrawIndexedArguments), nullptr, Rhi::IndirectBufferFlag::UNORDERED_ACCESS | Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS);
		}

		// Vertex input layout
		static constexpr Rhi::VertexAttribute vertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"Position",								// name[32] (char)
				"POSITION",								// semanticName[32] (char)
				0,										// semanticIndex (uint32_t)
				// Data source
				0,										// inputSlot (uint32_t)
				0,										// alignedByteOffset (uint32_t)
				sizeof(float) * 2,						// strideInBytes (uint32_t)
				0										// instancesPerElement (uint32_t)
			}
		};
		const Rhi::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayout)), vertexAttributesLayout);

		{ // Create the index buffer object (IBO)
			static constexpr uint16_t INDICES[] =
			{
				0, 1, 2
			};
			mComputeInputIndexBuffer = mBufferManager->createIndexBuffer(sizeof(INDICES), INDICES, Rhi::BufferFlag::SHADER_RESOURCE);
			mComputeOutputIndexBuffer = mBufferManager->createIndexBuffer(sizeof(INDICES), nullptr, Rhi::BufferFlag::UNORDERED_ACCESS);
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
			mComputeInputVertexBuffer = mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Rhi::BufferFlag::SHADER_RESOURCE);
			mComputeOutputVertexBuffer = mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), nullptr, Rhi::BufferFlag::UNORDERED_ACCESS);
		}

		{ // Create vertex array object (VAO)
		  // -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		  // -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		  // -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		  //    reference of the used vertex buffer objects (VBO). If the reference counter of a
		  //    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { mComputeOutputVertexBuffer };
			mVertexArray = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, mComputeOutputIndexBuffer);
		}

		{ // Create the uniform buffer which will be read by a compute shader
			static constexpr float RGBA_COLOR[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			mComputeInputUniformBuffer = mBufferManager->createUniformBuffer(sizeof(RGBA_COLOR), RGBA_COLOR);
		}

		{ // Resource group related
			// Create the texture instance, but without providing texture data (we use the texture as render target)
			// -> Use the "Rhi::TextureFlag::RENDER_TARGET"-flag to mark this texture as a render target
			// -> Required for Vulkan, Direct3D 9, Direct3D 10, Direct3D 11 and Direct3D 12
			// -> Not required for OpenGL and OpenGL ES 3
			// -> The optimized texture clear value is a Direct3D 12 related option
			const Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::Enum::R8G8B8A8;
			Rhi::ITexture* computeInputTexture2D = mTextureManager->createTexture2D(16, 16, textureFormat, nullptr, Rhi::TextureFlag::SHADER_RESOURCE | Rhi::TextureFlag::RENDER_TARGET, Rhi::TextureUsage::DEFAULT, 1, reinterpret_cast<const Rhi::OptimizedTextureClearValue*>(&Color4::GREEN));
			Rhi::ITexture* computeOutputTexture2D = mTextureManager->createTexture2D(16, 16, textureFormat, nullptr, Rhi::TextureFlag::SHADER_RESOURCE | Rhi::TextureFlag::UNORDERED_ACCESS);

			{ // Create the framebuffer object (FBO) instance
				const Rhi::FramebufferAttachment colorFramebufferAttachment(computeInputTexture2D);
				mFramebuffer = rhi->createFramebuffer(*rhi->createRenderPass(1, &textureFormat), &colorFramebufferAttachment);
			}

			{ // Create first compute resource group
				Rhi::IResource* resources[7] = {
					// Input
					computeInputTexture2D, mComputeInputVertexBuffer, mComputeInputIndexBuffer, mComputeInputUniformBuffer,
					// Output
					computeOutputTexture2D, mComputeOutputVertexBuffer, mComputeOutputIndexBuffer
				};
				Rhi::ISamplerState* samplerStates[7] = {
					// Input
					static_cast<Rhi::ISamplerState*>(samplerStateResource), nullptr, nullptr, nullptr,
					// Output
					nullptr, nullptr, nullptr
				};
				mComputeResourceGroup1 = mComputeRootSignature1->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, samplerStates);
			}

			{ // Create second compute resource group
				Rhi::IResource* resources[6] = {
					// Input
					mComputeInputTextureBuffer, mComputeInputStructuredBuffer, mComputeInputIndirectBuffer,
					// Output
					mComputeOutputTextureBuffer, mComputeOutputStructuredBuffer, mComputeOutputIndirectBuffer
				};
				mComputeResourceGroup2 = mComputeRootSignature2->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, nullptr);
			}

			{ // Create graphics resource group
				Rhi::IResource* resources[4] = { mComputeInputUniformBuffer, mComputeOutputTextureBuffer, mComputeOutputStructuredBuffer, computeOutputTexture2D };
				Rhi::ISamplerState* samplerStates[4] = { nullptr, nullptr, nullptr, static_cast<Rhi::ISamplerState*>(samplerStateResource) };
				mGraphicsResourceGroup = mGraphicsRootSignature->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, samplerStates);
			}
		}

		{
			// Create the graphics program
			Rhi::IGraphicsProgramPtr graphicsProgram;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				const char* computeShaderSourceCode1 = nullptr;
				const char* computeShaderSourceCode2 = nullptr;
				#include "ComputeShader_GLSL_450.h"	// For Vulkan
				#include "ComputeShader_GLSL_430.h"	// macOS 10.11 only supports OpenGL 4.1 and hence can't be supported by this example
				#include "ComputeShader_HLSL_D3D11_D3D12.h"
				#include "ComputeShader_Null.h"

				// Create the graphics program
				Rhi::IShaderLanguage& shaderLanguage = rhi->getDefaultShaderLanguage();
				graphicsProgram = shaderLanguage.createGraphicsProgram(
					*mGraphicsRootSignature,
					vertexAttributes,
					shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode));

				// Create the compute pipeline state objects (PSO)
				mComputePipelineState1 = rhi->createComputePipelineState(*mComputeRootSignature1, *shaderLanguage.createComputeShaderFromSourceCode(computeShaderSourceCode1));
				mComputePipelineState2 = rhi->createComputePipelineState(*mComputeRootSignature2, *shaderLanguage.createComputeShaderFromSourceCode(computeShaderSourceCode2));
			}

			// Create the graphics pipeline state object (PSO)
			if (nullptr != graphicsProgram)
			{
				mGraphicsPipelineState = rhi->createGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder(mGraphicsRootSignature, graphicsProgram, vertexAttributes, getMainRenderTarget()->getRenderPass()));
			}
		}

		// Since we're always submitting the same commands to the RHI, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void ComputeShader::onDeinitialization()
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

void ComputeShader::onDraw()
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Submit command buffer to the RHI implementation
		mCommandBuffer.submitToRhi(*rhi);
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void ComputeShader::fillCommandBuffer()
{
	// Sanity checks
	ASSERT(nullptr != getRhi(), "Invalid RHI instance")
	RHI_ASSERT(getRhi()->getContext(), nullptr != getMainRenderTarget(), "Invalid main render target")
	RHI_ASSERT(getRhi()->getContext(), mCommandBuffer.isEmpty(), "The command buffer is already filled")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsRootSignature, "Invalid graphics root signature")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeRootSignature1, "Invalid compute root signature 1")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeRootSignature2, "Invalid compute root signature 2")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mFramebuffer, "Invalid framebuffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeResourceGroup1, "Invalid compute resource group 1")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeResourceGroup2, "Invalid compute resource group 2")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsResourceGroup, "Invalid graphics resource group")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsSamplerStateGroup, "Invalid graphics sampler state group")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsPipelineState, "Invalid graphics pipeline state")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputePipelineState1, "Invalid compute pipeline state 1")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputePipelineState2, "Invalid compute pipeline state 2")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeInputVertexBuffer, "Invalid compute input vertex buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeOutputVertexBuffer, "Invalid compute output vertex buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeInputIndexBuffer, "Invalid compute input index buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeOutputIndexBuffer, "Invalid compute output index buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mVertexArray, "Invalid vertex array")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeOutputTextureBuffer, "Invalid compute output texture buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeInputTextureBuffer, "Invalid compute input texture buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeOutputStructuredBuffer, "Invalid compute output structured buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeInputStructuredBuffer, "Invalid compute input structured buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeOutputIndirectBuffer, "Invalid compute output indirect buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeInputIndirectBuffer, "Invalid compute input indirect buffer")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mComputeInputUniformBuffer, "Invalid compute input uniform buffer")

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	{ // Graphics: Render to texture
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Render to texture")

		// This in here is of course just an example. In a real application
		// there would be no point in constantly updating texture content
		// without having any real change.

		// Set the graphics render target to render into
		Rhi::Command::SetGraphicsRenderTarget::create(mCommandBuffer, mFramebuffer);

		// Clear the graphics color buffer of the current render target with green
		Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR, Color4::GREEN);

		// Restore graphics main swap chain as current render target
		Rhi::Command::SetGraphicsRenderTarget::create(mCommandBuffer, getMainRenderTarget());
	}

	{ // Compute: Use the graphics render to texture result for compute
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Use the render to texture result for compute")

		// Set the used compute root signature
		Rhi::Command::SetComputeRootSignature::create(mCommandBuffer, mComputeRootSignature1);

		// Set the used compute pipeline state object (PSO)
		Rhi::Command::SetComputePipelineState::create(mCommandBuffer, mComputePipelineState1);

		// Set compute resource groups
		Rhi::Command::SetComputeResourceGroup::create(mCommandBuffer, 0, mComputeResourceGroup1);

		// Dispatch compute call
		Rhi::Command::DispatchCompute::create(mCommandBuffer, 1, 1, 1);

		// Repeat everything for the second compute shader
		Rhi::Command::SetComputeRootSignature::create(mCommandBuffer, mComputeRootSignature2);
		Rhi::Command::SetComputePipelineState::create(mCommandBuffer, mComputePipelineState2);
		Rhi::Command::SetComputeResourceGroup::create(mCommandBuffer, 0, mComputeResourceGroup2);
		Rhi::Command::DispatchCompute::create(mCommandBuffer, 1, 1, 1);
	}

	{ // Graphics: Use the compute result for graphics
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Use the compute result")

		// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
		Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		// Set the used graphics root signature
		Rhi::Command::SetGraphicsRootSignature::create(mCommandBuffer, mGraphicsRootSignature);

		// Set the used graphics pipeline state object (PSO)
		Rhi::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

		// Set graphics resource groups
		Rhi::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mGraphicsResourceGroup);
		Rhi::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 1, mGraphicsSamplerStateGroup);

		// Input assembly (IA): Set the used vertex array
		Rhi::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

		// Render the specified geometric primitive, based on indexing into an array of vertices
		Rhi::Command::DrawIndexedGraphics::create(mCommandBuffer, *mComputeOutputIndirectBuffer);
	}
}
