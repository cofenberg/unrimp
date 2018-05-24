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
#include "PrecompiledHeader.h"
#include "Basics/FirstTriangle/FirstTriangle.h"
#include "Framework/Color4.h"

#include <string.h>


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstTriangle::onInitialization()
{
	// Call the base implementation
	ExampleBase::onInitialization();

	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Create the buffer manager
		mBufferManager = renderer->createBufferManager();

		{ // Create the root signature
			// Setup
			Renderer::RootSignatureBuilder rootSignature;
			rootSignature.initialize(0, nullptr, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = renderer->createRootSignature(rootSignature);
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
			Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));
			RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, "Triangle VBO")

			// Create vertex array object (VAO)
			// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
			// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
			// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
			//    reference of the used vertex buffer objects (VBO). If the reference counter of a
			//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			mVertexArray = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
			RENDERER_SET_RESOURCE_DEBUG_NAME(mVertexArray, "Triangle VAO")
		}

		// Decide which shader language should be used (for example "GLSL" or "HLSL")
		Renderer::IShaderLanguagePtr shaderLanguage(renderer->getShaderLanguage());
		if (nullptr != shaderLanguage)
		{
			// Create the program
			Renderer::IProgramPtr program;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "FirstTriangle_GLSL_450.h"	// For Vulkan
				#include "FirstTriangle_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "FirstTriangle_GLSL_ES3.h"
				#include "FirstTriangle_HLSL_D3D9_D3D10_D3D11_D3D12.h"
				#include "FirstTriangle_Null.h"

				// Create the vertex shader
				Renderer::IVertexShader* vertexShader = shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode);
				RENDERER_SET_RESOURCE_DEBUG_NAME(vertexShader, "Triangle VS")

				// Create the fragment shader
				Renderer::IFragmentShader* fragmentShader = shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode);
				RENDERER_SET_RESOURCE_DEBUG_NAME(fragmentShader, "Triangle FS")

				// Create the program
				program = shaderLanguage->createProgram(*mRootSignature, vertexAttributes, vertexShader, fragmentShader);
				RENDERER_SET_RESOURCE_DEBUG_NAME(program, "Triangle program")
			}

			// Create the pipeline state object (PSO)
			if (nullptr != program)
			{
				mPipelineState = renderer->createPipelineState(Renderer::PipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass()));
				RENDERER_SET_RESOURCE_DEBUG_NAME(mPipelineState, "Triangle PSO")
			}
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstTriangle::onDeinitialization()
{
	// Release the used resources
	mVertexArray = nullptr;
	mPipelineState = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mBufferManager = nullptr;

	// Call the base implementation
	ExampleBase::onDeinitialization();
}

void FirstTriangle::onDraw()
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
void FirstTriangle::fillCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mPipelineState);
	assert(nullptr != mVertexArray);

	// Begin debug event
	COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the color buffer of the current render target with gray, do also clear the depth buffer
	Renderer::Command::Clear::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used pipeline state object (PSO)
	Renderer::Command::SetPipelineState::create(mCommandBuffer, mPipelineState);

	// Input assembly (IA): Set the used vertex array
	Renderer::Command::SetVertexArray::create(mCommandBuffer, mVertexArray);

	// Set debug marker
	// -> Debug methods: When using Direct3D <11.1, these methods map to the Direct3D 9 PIX functions
	//    (D3DPERF_* functions, also works directly within VisualStudio 2012 out-of-the-box)
	COMMAND_SET_DEBUG_MARKER(mCommandBuffer, "Everyone ready for the upcoming triangle?")

	{
		// Begin debug event
		COMMAND_BEGIN_DEBUG_EVENT(mCommandBuffer, "Drawing the fancy triangle")

		// Render the specified geometric primitive, based on an array of vertices
		Renderer::Command::Draw::create(mCommandBuffer, 3);

		// End debug event
		COMMAND_END_DEBUG_EVENT(mCommandBuffer)
	}

	// End debug event
	COMMAND_END_DEBUG_EVENT(mCommandBuffer)
}
