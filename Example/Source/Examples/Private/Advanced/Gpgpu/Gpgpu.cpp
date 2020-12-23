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
#include "Examples/Private/Advanced/Gpgpu/Gpgpu.h"
#include "Examples/Private/Framework/Color4.h"
#include "Examples/Private/ExampleRunner.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP

#include <Rhi/Public/DefaultLog.h>
#include <Rhi/Public/DefaultAssert.h>
#include <Rhi/Public/DefaultAllocator.h>
#include <Rhi/Public/RhiInstance.h>


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
Gpgpu::Gpgpu(ExampleRunner& exampleRunner, const char* rhiName, const std::string_view& exampleName) :
	mExampleRunner(exampleRunner),
	mRhiInstance(nullptr),
	mExampleName(exampleName)
{
	// Copy the given RHI name
	if (nullptr != rhiName)
	{
		strncpy(mRhiName, rhiName, 32);

		// In case the source string is longer then 32 bytes (including null terminator) make sure that the string is null terminated
		mRhiName[31] = '\0';
	}
	else
	{
		mRhiName[0] = '\0';
	}
}

int Gpgpu::run()
{
	// Create RHI instance
	Rhi::DefaultLog defaultLog;
	Rhi::DefaultAssert defaultAssert;
	Rhi::DefaultAllocator defaultAllocator;
	Rhi::Context rhiContext(defaultLog, defaultAssert, defaultAllocator);
	mRhiInstance = new Rhi::RhiInstance(mRhiName, rhiContext);

	// Get the RHI instance and ensure it's valid
	mRhi = mRhiInstance->getRhi();
	if (nullptr != mRhi && mRhi->isInitialized())
	{
		// Call initialization method
		onInitialization();

		// Let the application to its job
		onDoJob();

		// Call de-initialization method
		onDeinitialization();
	}

	// Destroy the RHI instance
	mRhi = nullptr;
	delete mRhiInstance;
	mRhiInstance = nullptr;

	// Done, no error
	mExampleRunner.switchExample("ImGuiExampleSelector");
	return 0;
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void Gpgpu::onInitialization()
{
	// Create the buffer and texture manager
	mBufferManager = mRhi->createBufferManager();
	mTextureManager = mRhi->createTextureManager();

	{ // Create the root signature
		Rhi::DescriptorRangeBuilder ranges[2];
		ranges[0].initialize(Rhi::ResourceType::TEXTURE_2D, 0, "ContentMap", Rhi::ShaderVisibility::FRAGMENT);
		ranges[1].initializeSampler(0, Rhi::ShaderVisibility::FRAGMENT);

		Rhi::RootParameterBuilder rootParameters[2];
		rootParameters[0].initializeAsDescriptorTable(1, &ranges[0]);
		rootParameters[1].initializeAsDescriptorTable(1, &ranges[1]);

		// Setup
		Rhi::RootSignatureBuilder rootSignatureBuilder;
		rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Create the instance
		mRootSignature = mRhi->createRootSignature(rootSignatureBuilder);
	}

	// Create the 2D texture and framebuffer object (FBO) instances
	const Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::Enum::R8G8B8A8;
	Rhi::IRenderPass* renderPass = mRhi->createRenderPass(1, &textureFormat);
	for (int i = 0; i < 2; ++i)
	{
		// Create the texture instance, but without providing texture data (we use the texture as render target)
		// -> Use the "Rhi::TextureFlag::RENDER_TARGET"-flag to mark this texture as a render target
		// -> Required for Vulkan, Direct3D 9, Direct3D 10, Direct3D 11 and Direct3D 12
		// -> Not required for OpenGL and OpenGL ES 3
		// -> The optimized texture clear value is a Direct3D 12 related option
		Rhi::ITexture* texture2D = mTexture2D[i] = mTextureManager->createTexture2D(64, 64, textureFormat, nullptr, Rhi::TextureFlag::SHADER_RESOURCE | Rhi::TextureFlag::RENDER_TARGET, Rhi::TextureUsage::DEFAULT, 1, reinterpret_cast<const Rhi::OptimizedTextureClearValue*>(&Color4::BLUE));

		// Create the framebuffer object (FBO) instance
		const Rhi::FramebufferAttachment colorFramebufferAttachment(texture2D);
		mFramebuffer[i] = mRhi->createFramebuffer(*renderPass, &colorFramebufferAttachment);
	}

	// Create sampler state and wrap it into a resource group instance: We don't use mipmaps
	Rhi::IResource* samplerStateResource = nullptr;
	{
		Rhi::SamplerState samplerState = Rhi::ISamplerState::getDefaultSamplerState();
		samplerState.maxLod = 0.0f;
		samplerStateResource = mRhi->createSamplerState(samplerState);
		mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
	}

	{ // Create texture group
		Rhi::IResource* resource = mTexture2D[0];
		Rhi::ISamplerState* samplerState = static_cast<Rhi::ISamplerState*>(samplerStateResource);
		mTextureGroup = mRootSignature->createResourceGroup(0, 1, &resource, &samplerState);
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

	{ // Create vertex array object (VAO) for content generation
		// Create the vertex buffer object (VBO)
		// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
		static constexpr float VERTEX_POSITION[] =
		{					// Vertex ID	Triangle on screen
			 0.0f, 1.0f,	// 0				0
			 1.0f, 0.0f,	// 1			   .   .
			-0.5f, 0.0f		// 2			  2.......1
		};
		Rhi::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));

		// Create vertex array object (VAO)
		// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		//    reference of the used vertex buffer objects (VBO). If the reference counter of a
		//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
		const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
		mVertexArrayContentGeneration = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
	}

	{ // Create vertex array object (VAO) for content processing
		// Create the vertex buffer object (VBO)
		// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
		static constexpr float VERTEX_POSITION[] =
		{					// Vertex ID	Triangle strip on screen
			-1.0f, -1.0f,	// 0			  1.......3
			-1.0f,  1.0f,	// 1			  .   .   .
			 1.0f, -1.0f,	// 2			  0.......2
			 1.0f,  1.0f	// 3
		};
		Rhi::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));

		// Create vertex array object (VAO)
		// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		//    reference of the used vertex buffer objects (VBO). If the reference counter of a
		//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
		const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
		mVertexArrayContentProcessing = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
	}

	{
		// Create the graphics programs
		Rhi::IGraphicsProgramPtr graphicsProgramContentGeneration;
		Rhi::IGraphicsProgramPtr graphicsProgramContentProcessing;
		{
			// Get the shader source code (outsourced to keep an overview)
			const char* vertexShaderSourceCode = nullptr;
			const char* fragmentShaderSourceCode_ContentGeneration = nullptr;
			const char* fragmentShaderSourceCode_ContentProcessing = nullptr;
			#include "Gpgpu_GLSL_410.h"
			#include "Gpgpu_GLSL_ES3.h"
			#include "Gpgpu_HLSL_D3D9.h"
			#include "Gpgpu_HLSL_D3D10_D3D11_D3D12.h"
			#include "Gpgpu_Null.h"

			// In order to keep this example simple and to show that it's possible, we use the same vertex shader for both graphics programs
			// -> Depending on the used graphics API and whether or not the shader compiler & linker is clever,
			//    the unused texture coordinate might get optimized out
			// -> In a real world application you shouldn't rely on shader compiler & linker behaviour assumptions
			Rhi::IShaderLanguage& shaderLanguage = mRhi->getDefaultShaderLanguage();
			Rhi::IVertexShaderPtr vertexShader(shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode));
			graphicsProgramContentGeneration = shaderLanguage.createGraphicsProgram(*mRootSignature, vertexAttributes, vertexShader, shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode_ContentGeneration));
			graphicsProgramContentProcessing = shaderLanguage.createGraphicsProgram(*mRootSignature, vertexAttributes, vertexShader, shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode_ContentProcessing));
		}

		// Create the graphics pipeline state objects (PSO)
		if (nullptr != graphicsProgramContentGeneration && nullptr != graphicsProgramContentProcessing)
		{
			{ // Content generation
				Rhi::GraphicsPipelineState graphicsPipelineState = Rhi::GraphicsPipelineStateBuilder(mRootSignature, graphicsProgramContentGeneration, vertexAttributes, mFramebuffer[0]->getRenderPass());
				graphicsPipelineState.depthStencilState.depthEnable = false;
				mGraphicsPipelineStateContentGeneration = mRhi->createGraphicsPipelineState(graphicsPipelineState);
			}
			{ // Content processing
				Rhi::GraphicsPipelineState graphicsPipelineState = Rhi::GraphicsPipelineStateBuilder(mRootSignature, graphicsProgramContentProcessing, vertexAttributes, mFramebuffer[0]->getRenderPass());
				graphicsPipelineState.primitiveTopology = Rhi::PrimitiveTopology::TRIANGLE_STRIP;
				graphicsPipelineState.depthStencilState.depthEnable = false;
				mGraphicsPipelineStateContentProcessing = mRhi->createGraphicsPipelineState(graphicsPipelineState);
			}
		}
	}

	// Since we're always dispatching the same commands to the RHI, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
	fillCommandBufferContentGeneration();
	fillCommandBufferContentProcessing();
}

void Gpgpu::onDeinitialization()
{
	// Release the used resources
	mCommandBufferContentGeneration.clear();
	mCommandBufferContentProcessing.clear();
	mVertexArrayContentProcessing = nullptr;
	mGraphicsPipelineStateContentProcessing = nullptr;
	mVertexArrayContentGeneration = nullptr;
	mGraphicsPipelineStateContentGeneration = nullptr;
	mSamplerStateGroup = nullptr;
	mTextureGroup = nullptr;
	mRootSignature = nullptr;
	for (int i = 0; i < 2; ++i)
	{
		mFramebuffer[i] = nullptr;
		mTexture2D[i] = nullptr;
	}
	mBufferManager = nullptr;
	mTextureManager = nullptr;
}

void Gpgpu::fillCommandBufferContentGeneration()
{
	// Sanity checks
	ASSERT(nullptr != mRhi, "Invalid RHI instance")
	RHI_ASSERT(mRhi->getContext(), nullptr != mFramebuffer[0], "Invalid framebuffer at index 0")
	RHI_ASSERT(mRhi->getContext(), nullptr != mRootSignature, "Invalid root signature")
	RHI_ASSERT(mRhi->getContext(), nullptr != mGraphicsPipelineStateContentGeneration, "Invalid graphics pipeline state content generation")
	RHI_ASSERT(mRhi->getContext(), nullptr != mVertexArrayContentGeneration, "Invalid vertex array content generation")
	RHI_ASSERT(mRhi->getContext(), mCommandBufferContentGeneration.isEmpty(), "Command buffer content generation is already filled")

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT(mCommandBufferContentGeneration, "Generate the content of the 2D texture to process later on")

	// Set the graphics render target to render into
	Rhi::Command::SetGraphicsRenderTarget::create(mCommandBufferContentGeneration, mFramebuffer[0]);

	// Clear the graphics color buffer of the current render target with blue
	Rhi::Command::ClearGraphics::create(mCommandBufferContentGeneration, Rhi::ClearFlag::COLOR, Color4::BLUE);

	// Set the used graphics root signature
	Rhi::Command::SetGraphicsRootSignature::create(mCommandBufferContentGeneration, mRootSignature);

	{ // Set the viewport and scissor rectangle
		// Get the render target with and height
		uint32_t width  = 1;
		uint32_t height = 1;
		Rhi::IRenderTarget* renderTarget = mFramebuffer[0];
		if (nullptr != renderTarget)
		{
			renderTarget->getWidthAndHeight(width, height);
		}

		// Set the graphics viewport and scissor rectangle
		Rhi::Command::SetGraphicsViewportAndScissorRectangle::create(mCommandBufferContentGeneration, 0, 0, width, height);
	}

	// Set the used graphics pipeline state object (PSO)
	Rhi::Command::SetGraphicsPipelineState::create(mCommandBufferContentGeneration, mGraphicsPipelineStateContentGeneration);

	// Input assembly (IA): Set the used vertex array
	Rhi::Command::SetGraphicsVertexArray::create(mCommandBufferContentGeneration, mVertexArrayContentGeneration);

	// Render the specified geometric primitive, based on indexing into an array of vertices
	Rhi::Command::DrawGraphics::create(mCommandBufferContentGeneration, 3);
}

void Gpgpu::fillCommandBufferContentProcessing()
{
	// Sanity checks
	ASSERT(nullptr != mRhi, "Invalid RHI instance")
	RHI_ASSERT(mRhi->getContext(), nullptr != mFramebuffer[1], "Invalid framebuffer at index 1")
	RHI_ASSERT(mRhi->getContext(), nullptr != mRootSignature, "Invalid root signature")
	RHI_ASSERT(mRhi->getContext(), nullptr != mGraphicsPipelineStateContentProcessing, "Invalid graphics pipeline state content processing")
	RHI_ASSERT(mRhi->getContext(), nullptr != mTextureGroup, "Invalid texture group")
	RHI_ASSERT(mRhi->getContext(), nullptr != mSamplerStateGroup, "Invalid sampler state group")
	RHI_ASSERT(mRhi->getContext(), nullptr != mTexture2D[0], "Invalid texture 2D at index 0")
	RHI_ASSERT(mRhi->getContext(), mCommandBufferContentProcessing.isEmpty(), "Command buffer content processing is already filled")

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT(mCommandBufferContentProcessing, "Content processing")

	// Set the graphics render target to render into
	Rhi::Command::SetGraphicsRenderTarget::create(mCommandBufferContentProcessing, mFramebuffer[1]);

	// We don't need to clear the current render target because our fullscreen quad covers the full screen

	// Set the used graphics root signature
	Rhi::Command::SetGraphicsRootSignature::create(mCommandBufferContentProcessing, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Rhi::Command::SetGraphicsPipelineState::create(mCommandBufferContentProcessing, mGraphicsPipelineStateContentProcessing);

	// Set graphics resource groups
	Rhi::Command::SetGraphicsResourceGroup::create(mCommandBufferContentProcessing, 0, mTextureGroup);
	Rhi::Command::SetGraphicsResourceGroup::create(mCommandBufferContentProcessing, 1, mSamplerStateGroup);

	// Input assembly (IA): Set the used vertex array
	Rhi::Command::SetGraphicsVertexArray::create(mCommandBufferContentProcessing, mVertexArrayContentProcessing);

	// Render the specified geometric primitive, based on indexing into an array of vertices
	Rhi::Command::DrawGraphics::create(mCommandBufferContentProcessing, 4);
}

void Gpgpu::onDoJob()
{
	// Generate the content of the 2D texture to process later on
	// -> After this step, "mTexture2D[0]" holds the content we want to process later on
	mCommandBufferContentGeneration.dispatchToRhi(*mRhi);

	// Content processing
	// -> After this step, "mTexture2D[1]" holds the processed content
	mCommandBufferContentProcessing.dispatchToRhi(*mRhi);

	// TODO(co) "Rhi::IRhi::map()"/"Rhi::IRhi::unmap()" are currently under construction
	// Map the texture holding the processed content
	Rhi::MappedSubresource mappedSubresource;
	if (mRhi->map(*mTexture2D[1], 0, Rhi::MapType::READ, 0, mappedSubresource))
	{
		// Get the processed content pointer
//		const uint8_t* data = static_cast<uint8_t*>(mappedSubresource.data);

		// TODO(co) Write it out as image?

		// Unmap the texture holding the processed content
		mRhi->unmap(*mTexture2D[1], 0);
	}
}
