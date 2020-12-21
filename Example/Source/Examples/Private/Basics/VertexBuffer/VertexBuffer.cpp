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
#include "Examples/Private/Basics/VertexBuffer/VertexBuffer.h"
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
void VertexBuffer::onInitialization()
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Create the buffer manager
		mBufferManager = rhi->createBufferManager();

		{ // Create the root signature
			// Setup
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(0, nullptr, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = rhi->createRootSignature(rootSignatureBuilder);
		}

		// Vertex input layout
		static constexpr Rhi::VertexAttribute vertexAttributesLayoutVBO[] =
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
				sizeof(float) * 5,						// strideInBytes (uint32_t)
				0										// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"Color",								// name[32] (char)
				"COLOR",								// semanticName[32] (char)
				0,										// semanticIndex (uint32_t)
				// Data source
				0,										// inputSlot (uint32_t)
				sizeof(float) * 2,						// alignedByteOffset (uint32_t)
				sizeof(float) * 5,						// strideInBytes (uint32_t)
				0										// instancesPerElement (uint32_t)
			}
		};
		const Rhi::VertexAttributes vertexAttributesVBO(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayoutVBO)), vertexAttributesLayoutVBO);
		static constexpr Rhi::VertexAttribute vertexAttributesLayoutVBOs[] =
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
			},
			{ // Attribute 1
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"Color",								// name[32] (char)
				"COLOR",								// semanticName[32] (char)
				0,										// semanticIndex (uint32_t)
				// Data source
				1,										// inputSlot (uint32_t)
				0,										// alignedByteOffset (uint32_t)
				sizeof(float) * 3,						// strideInBytes (uint32_t)
				0										// instancesPerElement (uint32_t)
			}
		};
		const Rhi::VertexAttributes vertexAttributesVBOs(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayoutVBOs)), vertexAttributesLayoutVBOs);

		// Vertex array object (VAO)
		// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		//    reference of the used vertex buffer objects (VBO). If the reference counter of a
		//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.

		{ // Create vertex array object (VAO)
			// Create the vertex buffer object (VBO) holding position and color data
			// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
			// -> Traditional normalized RGB vertex colors
			static constexpr float VERTEX_POSITION_COLOR[] =
			{	 // Position     Color				// Vertex ID	Triangle on screen
				 0.0f, 1.0f,	1.0f, 0.0f, 0.0f,	// 0				0
				 1.0f, 0.0f,	0.0f, 1.0f, 0.0f,	// 1			   .   .
				-0.5f, 0.0f,	0.0f, 0.0f, 1.0f	// 2			  2.......1
			};
			Rhi::IVertexBufferPtr vertexBufferPositionColor(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION_COLOR), VERTEX_POSITION_COLOR));

			// Create vertex array object (VAO)
			const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBufferPositionColor };
			mVertexArrayVbo = mBufferManager->createVertexArray(vertexAttributesVBO, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
		}

		{ // Create vertex array object (VAO) using multiple vertex buffer object (VBO)
			// Create the vertex buffer object (VBO) holding color data
			// -> Traditional normalized RGB vertex colors
			static constexpr float VERTEX_COLOR[] =
			{					// Vertex ID	Triangle on screen
				1.0f, 0.0f, 0.0f,	// 0			  0.......1
				0.0f, 1.0f, 0.0f,	// 1			   .   .
				0.0f, 0.0f, 1.0f	// 2			  	2
			};
			Rhi::IVertexBufferPtr vertexBufferColor(mBufferManager->createVertexBuffer(sizeof(VERTEX_COLOR), VERTEX_COLOR));

			// Create the vertex buffer object (VBO) holding position data
			// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
			static constexpr float VERTEX_POSITION[] =
			{					// Vertex ID	Triangle on screen
				-0.5f,  0.0f,	// 0			  0.......1
				 1.0f,  0.0f,	// 1			   .   .
				 0.0f, -1.0f	// 2			  	2
			};
			Rhi::IVertexBufferPtr vertexBufferPosition(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));

			// Create vertex array object (VAO)
			const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBufferPosition, vertexBufferColor };
			mVertexArrayVbos = mBufferManager->createVertexArray(vertexAttributesVBOs, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
		}

		{
			// Get the shader source code (outsourced to keep an overview)
			const char* vertexShaderSourceCode = nullptr;
			const char* fragmentShaderSourceCode = nullptr;
			#include "VertexBuffer_GLSL_450.h"	// For Vulkan
			#include "VertexBuffer_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
			#include "VertexBuffer_GLSL_ES3.h"
			#include "VertexBuffer_HLSL_D3D9_D3D10_D3D11_D3D12.h"
			#include "VertexBuffer_Null.h"
			Rhi::IShaderLanguage& shaderLanguage = rhi->getDefaultShaderLanguage();

			{ // Create pipeline state objects (PSO) using one vertex buffer object (VBO)
				// Create the graphics program
				Rhi::IGraphicsProgramPtr graphicsProgram;
				graphicsProgram = shaderLanguage.createGraphicsProgram(
					*mRootSignature,
					vertexAttributesVBO,
					shaderLanguage.createVertexShaderFromSourceCode(vertexAttributesVBO, vertexShaderSourceCode),
					shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode));

				// Create the graphics pipeline state objects (PSO)
				if (nullptr != graphicsProgram)
				{
					mGraphicsPipelineStateVbo = rhi->createGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder(mRootSignature, graphicsProgram, vertexAttributesVBO, getMainRenderTarget()->getRenderPass()));
				}
			}

			{ // Create graphics pipeline state objects (PSO) using multiple vertex buffer object (VBO)
				// Create the graphics program
				Rhi::IGraphicsProgramPtr graphicsProgram;
				graphicsProgram = shaderLanguage.createGraphicsProgram(
					*mRootSignature,
					vertexAttributesVBOs,
					shaderLanguage.createVertexShaderFromSourceCode(vertexAttributesVBOs, vertexShaderSourceCode),
					shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode));

				// Create the graphics pipeline state objects (PSO)
				if (nullptr != graphicsProgram)
				{
					mGraphicsPipelineStateVbos = rhi->createGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder(mRootSignature, graphicsProgram, vertexAttributesVBOs, getMainRenderTarget()->getRenderPass()));
				}
			}
		}

		// Since we're always submitting the same commands to the RHI, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void VertexBuffer::onDeinitialization()
{
	// Release the used resources
	mGraphicsPipelineStateVbos = nullptr;
	mVertexArrayVbos = nullptr;
	mVertexArrayVbo = nullptr;
	mGraphicsPipelineStateVbo = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mBufferManager = nullptr;
}

void VertexBuffer::onDraw(Rhi::CommandBuffer& commandBuffer)
{
	// Submit command buffer to the given command buffer
	mCommandBuffer.submitToCommandBuffer(commandBuffer);
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void VertexBuffer::fillCommandBuffer()
{
	// Sanity checks
	ASSERT(nullptr != getRhi(), "Invalid RHI instance")
	RHI_ASSERT(getRhi()->getContext(), mCommandBuffer.isEmpty(), "The command buffer is already filled")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mRootSignature, "Invalid root signature")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsPipelineStateVbo, "Invalid graphics pipeline state VBO")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mVertexArrayVbo, "Invalid vertex array VBO")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsPipelineStateVbos, "Invalid graphics pipeline state VBOs")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mVertexArrayVbos, "Invalid vertex array VBOs")

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
	Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Rhi::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// First lower triangle using one vertex buffer object (VBO)
	if (nullptr != mGraphicsPipelineStateVbo)
	{
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Draw using one VBO")

		// Set the used graphics pipeline state object (PSO)
		Rhi::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineStateVbo);

		// Input assembly (IA): Set the used vertex array
		Rhi::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArrayVbo);

		// Render the specified geometric primitive, based on an array of vertices
		Rhi::Command::DrawGraphics::create(mCommandBuffer, 3);
	}

	// Second upper triangle using multiple vertex buffer object (VBO)
	if (nullptr != mGraphicsPipelineStateVbos)
	{
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Draw using multiple VBOs")

		// Set the used graphics pipeline state object (PSO)
		Rhi::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineStateVbos);

		// Input assembly (IA): Set the used vertex array
		Rhi::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArrayVbos);

		// Render the specified geometric primitive, based on an array of vertices
		Rhi::Command::DrawGraphics::create(mCommandBuffer, 3);
	}
}
