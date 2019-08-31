/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "Examples/Private/Basics/FirstGeometryShader/FirstGeometryShader.h"
#include "Examples/Private/Framework/Color4.h"


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstGeometryShader::onInitialization()
{
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

		{
			// Create the graphics program
			Renderer::IGraphicsProgramPtr graphicsProgram;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* geometryShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "FirstGeometryShader_GLSL_450.h"	// For Vulkan
				#include "FirstGeometryShader_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "FirstGeometryShader_HLSL_D3D10_D3D11_D3D12.h"
				#include "FirstGeometryShader_Null.h"

				// Create the graphics program
				Renderer::IShaderLanguage& shaderLanguage = renderer->getDefaultShaderLanguage();
				graphicsProgram = shaderLanguage.createGraphicsProgram(
					*mRootSignature,
					vertexAttributes,
					shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage.createGeometryShaderFromSourceCode(geometryShaderSourceCode, Renderer::GsInputPrimitiveTopology::POINTS, Renderer::GsOutputPrimitiveTopology::TRIANGLES_STRIP, 3),
					shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
			}

			// Create the graphics pipeline state object (PSO)
			if (nullptr != graphicsProgram)
			{
				Renderer::GraphicsPipelineState graphicsPipelineState = Renderer::GraphicsPipelineStateBuilder(mRootSignature, graphicsProgram, vertexAttributes, getMainRenderTarget()->getRenderPass());
				graphicsPipelineState.primitiveTopology = Renderer::PrimitiveTopology::POINT_LIST;
				graphicsPipelineState.primitiveTopologyType = Renderer::PrimitiveTopologyType::POINT;
				mGraphicsPipelineState = renderer->createGraphicsPipelineState(graphicsPipelineState);
			}
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstGeometryShader::onDeinitialization()
{
	// Release the used resources
	mGraphicsPipelineState = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mBufferManager = nullptr;
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
	ASSERT(nullptr != getRenderer());
	RENDERER_ASSERT(getRenderer()->getContext(), mCommandBuffer.isEmpty(), "Command buffer is already filled");
	RENDERER_ASSERT(getRenderer()->getContext(), nullptr != mRootSignature, "Invalid root signature");
	RENDERER_ASSERT(getRenderer()->getContext(), nullptr != mGraphicsPipelineState, "Invalid graphics pipeline state");

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
	Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

	// Render the specified geometric primitive, based on an array of vertices
	// -> Emit a single point in order to generate a draw call, the geometry shader does the rest
	// -> Attribute-less rendering (aka "drawing without data")
	Renderer::Command::DrawGraphics::create(mCommandBuffer, 1);
}
