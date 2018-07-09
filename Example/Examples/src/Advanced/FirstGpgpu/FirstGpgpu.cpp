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
#include "Advanced/FirstGpgpu/FirstGpgpu.h"
#include "Framework/Color4.h"
#include "ExampleRunner.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP

#include <Renderer/DefaultLog.h>
#include <Renderer/DefaultAssert.h>
#include <Renderer/DefaultAllocator.h>
#include <Renderer/RendererInstance.h>


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
FirstGpgpu::FirstGpgpu(ExampleRunner& exampleRunner, const char* rendererName) :
	mExampleRunner(exampleRunner),
	mRendererInstance(nullptr)
{
	// Copy the given renderer name
	if (nullptr != rendererName)
	{
		strncpy(mRendererName, rendererName, 32);

		// In case the source string is longer then 32 bytes (including null terminator) make sure that the string is null terminated
		mRendererName[31] = '\0';
	}
	else
	{
		mRendererName[0] = '\0';
	}
}

int FirstGpgpu::run()
{
	// Create renderer instance
	Renderer::DefaultLog defaultLog;
	Renderer::DefaultAssert defaultAssert;
	Renderer::DefaultAllocator defaultAllocator;
	Renderer::Context rendererContext(defaultLog, defaultAssert, defaultAllocator);
	mRendererInstance = new Renderer::RendererInstance(mRendererName, rendererContext);

	// Get the renderer instance and ensure it's valid
	mRenderer = mRendererInstance->getRenderer();
	if (nullptr != mRenderer && mRenderer->isInitialized())
	{
		// Call initialization method
		onInitialization();

		// Begin scene rendering
		// -> Required for Direct3D 9 and Direct3D 12
		// -> Not required for Direct3D 10, Direct3D 11, OpenGL and OpenGL ES 3
		if (mRenderer->beginScene())
		{
			// Let the application to its job
			onDoJob();

			// End scene rendering
			// -> Required for Direct3D 9 and Direct3D 12
			// -> Not required for Direct3D 10, Direct3D 11, OpenGL and OpenGL ES 3
			mRenderer->endScene();
		}

		// Call de-initialization method
		onDeinitialization();
	}

	// Destroy the renderer instance
	mRenderer = nullptr;
	delete mRendererInstance;
	mRendererInstance = nullptr;

	// Done, no error
	mExampleRunner.switchExample("ImGuiExampleSelector");
	return 0;
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstGpgpu::onInitialization()
{
	// Create the buffer and texture manager
	mBufferManager = mRenderer->createBufferManager();
	mTextureManager = mRenderer->createTextureManager();

	{ // Create the root signature
		Renderer::DescriptorRangeBuilder ranges[2];
		ranges[0].initialize(Renderer::DescriptorRangeType::SRV, 1, 0, "AlbedoMap", Renderer::ShaderVisibility::FRAGMENT);
		ranges[1].initializeSampler(1, 0, Renderer::ShaderVisibility::FRAGMENT);

		Renderer::RootParameterBuilder rootParameters[2];
		rootParameters[0].initializeAsDescriptorTable(1, &ranges[0]);
		rootParameters[1].initializeAsDescriptorTable(1, &ranges[1]);

		// Setup
		Renderer::RootSignatureBuilder rootSignature;
		rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Create the instance
		mRootSignature = mRenderer->createRootSignature(rootSignature);
	}

	// Create the 2D texture and framebuffer object (FBO) instances
	const Renderer::TextureFormat::Enum textureFormat = Renderer::TextureFormat::Enum::R8G8B8A8;
	Renderer::IRenderPass* renderPass = mRenderer->createRenderPass(1, &textureFormat);
	for (int i = 0; i < 2; ++i)
	{
		// Create the texture instance, but without providing texture data (we use the texture as render target)
		// -> Use the "Renderer::TextureFlag::RENDER_TARGET"-flag to mark this texture as a render target
		// -> Required for Vulkan, Direct3D 9, Direct3D 10, Direct3D 11 and Direct3D 12
		// -> Not required for OpenGL and OpenGL ES 3
		// -> The optimized texture clear value is a Direct3D 12 related option
		Renderer::ITexture* texture2D = mTexture2D[i] = mTextureManager->createTexture2D(64, 64, textureFormat, nullptr, Renderer::TextureFlag::RENDER_TARGET, Renderer::TextureUsage::DEFAULT, 1, reinterpret_cast<const Renderer::OptimizedTextureClearValue*>(&Color4::BLUE));

		// Create the framebuffer object (FBO) instance
		const Renderer::FramebufferAttachment colorFramebufferAttachment(texture2D);
		mFramebuffer[i] = mRenderer->createFramebuffer(*renderPass, &colorFramebufferAttachment);
	}

	// Create sampler state and wrap it into a resource group instance: We don't use mipmaps
	Renderer::IResource* samplerStateResource = nullptr;
	{
		Renderer::SamplerState samplerState = Renderer::ISamplerState::getDefaultSamplerState();
		samplerState.maxLOD = 0.0f;
		samplerStateResource = mRenderer->createSamplerState(samplerState);
		mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
	}

	{ // Create texture group
		Renderer::IResource* resource = mTexture2D[0];
		Renderer::ISamplerState* samplerState = static_cast<Renderer::ISamplerState*>(samplerStateResource);
		mTextureGroup = mRootSignature->createResourceGroup(0, 1, &resource, &samplerState);
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

	{ // Create vertex array object (VAO) for content generation
		// Create the vertex buffer object (VBO)
		// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
		static constexpr float VERTEX_POSITION[] =
		{					// Vertex ID	Triangle on screen
			 0.0f, 1.0f,	// 0				0
			 1.0f, 0.0f,	// 1			   .   .
			-0.5f, 0.0f		// 2			  2.......1
		};
		Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));

		// Create vertex array object (VAO)
		// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		//    reference of the used vertex buffer objects (VBO). If the reference counter of a
		//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
		const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
		mVertexArrayContentGeneration = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
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
		Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));

		// Create vertex array object (VAO)
		// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		//    reference of the used vertex buffer objects (VBO). If the reference counter of a
		//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
		const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
		mVertexArrayContentProcessing = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
	}

	// Create the programs: Decide which shader language should be used (for example "GLSL" or "HLSL")
	Renderer::IShaderLanguagePtr shaderLanguage(mRenderer->getShaderLanguage());
	if (nullptr != shaderLanguage)
	{
		// Create the programs
		Renderer::IProgramPtr programContentGeneration;
		Renderer::IProgramPtr programContentProcessing;
		{
			// Get the shader source code (outsourced to keep an overview)
			const char* vertexShaderSourceCode = nullptr;
			const char* fragmentShaderSourceCode_ContentGeneration = nullptr;
			const char* fragmentShaderSourceCode_ContentProcessing = nullptr;
			#include "FirstGpgpu_GLSL_410.h"
			#include "FirstGpgpu_GLSL_ES3.h"
			#include "FirstGpgpu_HLSL_D3D9.h"
			#include "FirstGpgpu_HLSL_D3D10_D3D11_D3D12.h"
			#include "FirstGpgpu_Null.h"

			// In order to keep this example simple and to show that it's possible, we use the same vertex shader for both programs
			// -> Depending on the used graphics API and whether or not the shader compiler & linker is clever,
			//    the unused texture coordinate might get optimized out
			// -> In a real world application you shouldn't rely on shader compiler & linker behaviour assumptions
			Renderer::IVertexShaderPtr vertexShader(shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode));
			programContentGeneration = shaderLanguage->createProgram(*mRootSignature, vertexAttributes, vertexShader, shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode_ContentGeneration));
			programContentProcessing = shaderLanguage->createProgram(*mRootSignature, vertexAttributes, vertexShader, shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode_ContentProcessing));
		}

		// Create the graphics pipeline state objects (PSO)
		if (nullptr != programContentGeneration && nullptr != programContentProcessing)
		{
			{ // Content generation
				Renderer::GraphicsPipelineState graphicsPipelineState = Renderer::GraphicsPipelineStateBuilder(mRootSignature, programContentGeneration, vertexAttributes, mFramebuffer[0]->getRenderPass());
				graphicsPipelineState.depthStencilState.depthEnable = false;
				mGraphicsPipelineStateContentGeneration = mRenderer->createGraphicsPipelineState(graphicsPipelineState);
			}
			{ // Content processing
				Renderer::GraphicsPipelineState graphicsPipelineState = Renderer::GraphicsPipelineStateBuilder(mRootSignature, programContentProcessing, vertexAttributes, mFramebuffer[0]->getRenderPass());
				graphicsPipelineState.primitiveTopology = Renderer::PrimitiveTopology::TRIANGLE_STRIP;
				graphicsPipelineState.depthStencilState.depthEnable = false;
				mGraphicsPipelineStateContentProcessing = mRenderer->createGraphicsPipelineState(graphicsPipelineState);
			}
		}
	}

	// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
	fillCommandBufferContentGeneration();
	fillCommandBufferContentProcessing();
}

void FirstGpgpu::onDeinitialization()
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

void FirstGpgpu::fillCommandBufferContentGeneration()
{
	// Sanity checks
	assert(nullptr != mFramebuffer[0]);
	assert(nullptr != mRootSignature);
	assert(nullptr != mGraphicsPipelineStateContentGeneration);
	assert(nullptr != mVertexArrayContentGeneration);
	assert(mCommandBufferContentGeneration.isEmpty());

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT(mCommandBufferContentGeneration, "Generate the content of the 2D texture to process later on")

	// Set the graphics render target to render into
	Renderer::Command::SetGraphicsRenderTarget::create(mCommandBufferContentGeneration, mFramebuffer[0]);

	// Clear the graphics color buffer of the current render target with blue
	Renderer::Command::ClearGraphics::create(mCommandBufferContentGeneration, Renderer::ClearFlag::COLOR, Color4::BLUE);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBufferContentGeneration, mRootSignature);

	{ // Set the viewport and scissor rectangle
		// Get the render target with and height
		uint32_t width  = 1;
		uint32_t height = 1;
		Renderer::IRenderTarget* renderTarget = mFramebuffer[0];
		if (nullptr != renderTarget)
		{
			renderTarget->getWidthAndHeight(width, height);
		}

		// Set the graphics viewport and scissor rectangle
		Renderer::Command::SetGraphicsViewportAndScissorRectangle::create(mCommandBufferContentGeneration, 0, 0, width, height);
	}

	// Set the used graphics pipeline state object (PSO)
	Renderer::Command::SetGraphicsPipelineState::create(mCommandBufferContentGeneration, mGraphicsPipelineStateContentGeneration);

	// Input assembly (IA): Set the used vertex array
	Renderer::Command::SetGraphicsVertexArray::create(mCommandBufferContentGeneration, mVertexArrayContentGeneration);

	// Render the specified geometric primitive, based on indexing into an array of vertices
	Renderer::Command::DrawGraphics::create(mCommandBufferContentGeneration, 3);
}

void FirstGpgpu::fillCommandBufferContentProcessing()
{
	// Sanity checks
	assert(nullptr != mFramebuffer[1]);
	assert(nullptr != mRootSignature);
	assert(nullptr != mGraphicsPipelineStateContentProcessing);
	assert(nullptr != mTextureGroup);
	assert(nullptr != mSamplerStateGroup);
	assert(nullptr != mTexture2D[0]);
	assert(mCommandBufferContentProcessing.isEmpty());

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT(mCommandBufferContentProcessing, "Content processing")

	// Set the graphics render target to render into
	Renderer::Command::SetGraphicsRenderTarget::create(mCommandBufferContentProcessing, mFramebuffer[1]);

	// We don't need to clear the current render target because our fullscreen quad covers the full screen

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBufferContentProcessing, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Renderer::Command::SetGraphicsPipelineState::create(mCommandBufferContentProcessing, mGraphicsPipelineStateContentProcessing);

	// Set graphics resource groups
	Renderer::Command::SetGraphicsResourceGroup::create(mCommandBufferContentProcessing, 0, mTextureGroup);
	Renderer::Command::SetGraphicsResourceGroup::create(mCommandBufferContentProcessing, 1, mSamplerStateGroup);

	// Input assembly (IA): Set the used vertex array
	Renderer::Command::SetGraphicsVertexArray::create(mCommandBufferContentProcessing, mVertexArrayContentProcessing);

	// Render the specified geometric primitive, based on indexing into an array of vertices
	Renderer::Command::DrawGraphics::create(mCommandBufferContentProcessing, 4);
}

void FirstGpgpu::onDoJob()
{
	// Generate the content of the 2D texture to process later on
	// -> After this step, "mTexture2D[0]" holds the content we want to process later on
	mCommandBufferContentGeneration.submitToRenderer(*mRenderer);

	// Content processing
	// -> After this step, "mTexture2D[1]" holds the processed content
	mCommandBufferContentProcessing.submitToRenderer(*mRenderer);

	// TODO(co) "Renderer::IRenderer::map()"/"Renderer::IRenderer::unmap()" are currently under construction
	// Map the texture holding the processed content
	Renderer::MappedSubresource mappedSubresource;
	if (mRenderer->map(*mTexture2D[1], 0, Renderer::MapType::READ, 0, mappedSubresource))
	{
		// Get the processed content pointer
//		const uint8_t* data = static_cast<uint8_t*>(mappedSubresource.data);

		// TODO(co) Write it out as image?

		// Unmap the texture holding the processed content
		mRenderer->unmap(*mTexture2D[1], 0);
	}
}
