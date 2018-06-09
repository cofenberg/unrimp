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
#include "Basics/FirstGeometryShader/FirstGeometryShader.h"
#include "Framework/Color4.h"

#include <cassert>


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstGeometryShader::onInitialization()
{
	// Call the base implementation
	ExampleBase::onInitialization();

	// Get and check the renderer instance
	// -> Geometry shaders supported?
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer && renderer->getCapabilities().maximumNumberOfGsOutputVertices)
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
		const Renderer::VertexAttributes vertexAttributes(0, nullptr);

		// Create the program: Decide which shader language should be used (for example "GLSL" or "HLSL")
		Renderer::IShaderLanguagePtr shaderLanguage(renderer->getShaderLanguage());
		if (nullptr != shaderLanguage)
		{
			// Create the program
			Renderer::IProgramPtr program;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* geometryShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "FirstGeometryShader_GLSL_450.h"	// For Vulkan
				#include "FirstGeometryShader_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "FirstGeometryShader_HLSL_D3D10_D3D11_D3D12.h"
				#include "FirstGeometryShader_Null.h"

				// Create the program
				program = shaderLanguage->createProgram(
					*mRootSignature,
					vertexAttributes,
					shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage->createGeometryShaderFromSourceCode(geometryShaderSourceCode, Renderer::GsInputPrimitiveTopology::POINTS, Renderer::GsOutputPrimitiveTopology::TRIANGLES_STRIP, 3),
					shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
			}

			// Create the pipeline state object (PSO)
			if (nullptr != program)
			{
				Renderer::PipelineState pipelineState = Renderer::PipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass());
				pipelineState.primitiveTopology = Renderer::PrimitiveTopology::POINT_LIST;
				pipelineState.primitiveTopologyType = Renderer::PrimitiveTopologyType::POINT;
				mPipelineState = renderer->createPipelineState(pipelineState);
			}
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstGeometryShader::onDeinitialization()
{
	// Release the used resources
	mPipelineState = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mBufferManager = nullptr;

	// Call the base implementation
	ExampleBase::onDeinitialization();
}

void FirstGeometryShader::onDraw()
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
void FirstGeometryShader::fillCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mPipelineState);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the color buffer of the current render target with gray, do also clear the depth buffer
	Renderer::Command::Clear::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used pipeline state object (PSO)
	Renderer::Command::SetPipelineState::create(mCommandBuffer, mPipelineState);

	// Render the specified geometric primitive, based on an array of vertices
	// -> Emit a single point in order to generate a draw call, the geometry shader does the rest
	// -> Attribute-less rendering (aka "drawing without data")
	Renderer::Command::Draw::create(mCommandBuffer, 1);
}
