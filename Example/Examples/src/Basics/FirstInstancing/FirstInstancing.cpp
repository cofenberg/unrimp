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
#include "Basics/FirstInstancing/FirstInstancing.h"
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
void FirstInstancing::onInitialization()
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

		// Decide which shader language should be used (for example "GLSL" or "HLSL")
		Renderer::IShaderLanguagePtr shaderLanguage(renderer->getShaderLanguage());
		if (nullptr != shaderLanguage)
		{
			// There are two instancing approaches available
			// - Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
			// - Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
			//
			// In general, instanced arrays will probably run on the most systems:
			// -> Direct3D 10, Direct3D 11 and Direct3D 12 support both approaches
			// -> Direct3D 9 has support for instanced arrays, but does not support draw instanced
			// -> OpenGL 3.1 introduced draw instance ("GL_ARB_draw_instanced"-extension)
			// -> OpenGL 3.3 introduced instance array ("GL_ARB_instanced_arrays"-extension)
			// -> OpenGL ES 3.0 support both approaches

			// Left side (green): Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
			if (renderer->getCapabilities().instancedArrays)
			{
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
					},
					{ // Attribute 1
						// Data destination
						Renderer::VertexAttributeFormat::FLOAT_1,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
						"InstanceID",								// name[32] (char)
						"TEXCOORD",									// semanticName[32] (char)
						0,											// semanticIndex (uint32_t)
						// Data source
						1,											// inputSlot (uint32_t)
						0,											// alignedByteOffset (uint32_t)
						sizeof(float),								// strideInBytes (uint32_t)
						1											// instancesPerElement (uint32_t)
					}
				};
				const Renderer::VertexAttributes vertexAttributes(static_cast<uint32_t>(glm::countof(vertexAttributesLayout)), vertexAttributesLayout);

				{ // Create vertex array object (VAO)
					// Create the vertex buffer object (VBO)
					// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
					static constexpr float VERTEX_POSITION[] =
					{					// Vertex ID	Triangle on screen
						 0.0f, 1.0f,	// 0					 .0
						 0.0f, 0.0f,	// 1				 .    .
						-1.0f, 0.0f		// 2			  2.......1
					};
					Renderer::IVertexBufferPtr vertexBufferPosition(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));

					// Create the per-instance-data vertex buffer object (VBO)
					// -> Simple instance ID in order to keep it similar to the "draw instanced" version on the right side (blue)
					static constexpr float INSTANCE_ID[] =
					{
						0.0f, 1.0f
					};
					Renderer::IVertexBufferPtr vertexBufferInstanceId(mBufferManager->createVertexBuffer(sizeof(INSTANCE_ID), INSTANCE_ID, Renderer::BufferUsage::STATIC_DRAW));

					// Create the index buffer object (IBO)
					// -> In this example, we only draw a simple triangle and therefore usually do not need an index buffer
					// -> In Direct3D 9, instanced arrays with hardware support is only possible when drawing indexed primitives, see
					//    "Efficiently Drawing Multiple Instances of Geometry (Direct3D 9)"-article at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb173349%28v=vs.85%29.aspx#Drawing_Non_Indexed_Geometry
					static constexpr uint16_t INDICES[] =
					{
						0, 1, 2
					};
					Renderer::IIndexBuffer* indexBufferInstancedArrays = mBufferManager->createIndexBuffer(sizeof(INDICES), Renderer::IndexBufferFormat::UNSIGNED_SHORT, INDICES, Renderer::BufferUsage::STATIC_DRAW);

					// Create vertex array object (VAO)
					// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
					// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
					// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
					//    reference of the used vertex buffer objects (VBO). If the reference counter of a
					//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
					const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBufferPosition, vertexBufferInstanceId, };
					mVertexArrayInstancedArrays = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBufferInstancedArrays);
				}

				// Create the program
				Renderer::IProgramPtr program;
				{
					// Get the shader source code (outsourced to keep an overview)
					const char* vertexShaderSourceCode = nullptr;
					const char* fragmentShaderSourceCode = nullptr;
					#include "FirstInstancing_InstancedArrays_GLSL_450.h"	// For Vulkan
					#include "FirstInstancing_InstancedArrays_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
					#include "FirstInstancing_InstancedArrays_GLSL_ES3.h"
					#include "FirstInstancing_InstancedArrays_HLSL_D3D9_D3D10_D3D11_D3D12.h"
					#include "FirstInstancing_InstancedArrays_Null.h"

					// Create the program
					program = shaderLanguage->createProgram(
						*mRootSignature,
						vertexAttributes,
						shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
						shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
				}

				// Create the pipeline state object (PSO)
				if (nullptr != program)
				{
					mPipelineStateInstancedArrays = renderer->createPipelineState(Renderer::PipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass()));
				}
			}

			// Right side (blue): Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
			if (renderer->getCapabilities().drawInstanced)
			{
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
					{				// Vertex ID	Triangle on screen
						0.0f, 1.0f,	// 0			  0.	
						1.0f, 0.0f,	// 1			  .    .
						0.0f, 0.0f	// 2			  2.......1
					};
					Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));

					// Create vertex array object (VAO)
					// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
					// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
					// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
					//    reference of the used vertex buffer objects (VBO). If the reference counter of a
					//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
					const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
					mVertexArrayDrawInstanced = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
				}

				// Create the program
				Renderer::IProgramPtr program;
				{
					// Get the shader source code (outsourced to keep an overview)
					const char* vertexShaderSourceCode = nullptr;
					const char* fragmentShaderSourceCode = nullptr;
					#include "FirstInstancing_DrawInstanced_GLSL_450.h"	// For Vulkan
					#include "FirstInstancing_DrawInstanced_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
					#include "FirstInstancing_DrawInstanced_GLSL_ES3.h"
					#include "FirstInstancing_DrawInstanced_HLSL_D3D10_D3D11_D3D12.h"
					#include "FirstInstancing_DrawInstanced_Null.h"

					// Create the program
					program = shaderLanguage->createProgram(
						*mRootSignature,
						vertexAttributes,
						shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
						shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
				}

				// Create the pipeline state object (PSO)
				if (nullptr != program)
				{
					mPipelineStateDrawInstanced = renderer->createPipelineState(Renderer::PipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass()));
				}
			}
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstInstancing::onDeinitialization()
{
	// Release the used resources
	mVertexArrayDrawInstanced = nullptr;
	mPipelineStateDrawInstanced = nullptr;
	mVertexArrayInstancedArrays = nullptr;
	mPipelineStateInstancedArrays = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mBufferManager = nullptr;

	// Call the base implementation
	ExampleBase::onDeinitialization();
}

void FirstInstancing::onDraw()
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
void FirstInstancing::fillCommandBuffer()
{
	// Sanity checks
	assert(nullptr != getRenderer());
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mPipelineStateInstancedArrays);
	assert(nullptr != mVertexArrayInstancedArrays);
	assert(!getRenderer()->getCapabilities().drawInstanced || nullptr != mPipelineStateDrawInstanced);
	assert(!getRenderer()->getCapabilities().drawInstanced || nullptr != mVertexArrayDrawInstanced);

	// Begin debug event
	COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the color buffer of the current render target with gray, do also clear the depth buffer
	Renderer::Command::Clear::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Left side (green): Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
	if (getRenderer()->getCapabilities().instancedArrays)
	{
		// Begin debug event
		COMMAND_BEGIN_DEBUG_EVENT(mCommandBuffer, "Draw using instanced arrays")

		// Set the used pipeline state object (PSO)
		Renderer::Command::SetPipelineState::create(mCommandBuffer, mPipelineStateInstancedArrays);

		// Input assembly (IA): Set the used vertex array
		Renderer::Command::SetVertexArray::create(mCommandBuffer, mVertexArrayInstancedArrays);

		// Render the specified geometric primitive, based on an array of vertices
		// -> In this example, we only draw a simple triangle and therefore usually do not need an index buffer
		// -> In Direct3D 9, instanced arrays with hardware support is only possible when drawing indexed primitives, see
		//    "Efficiently Drawing Multiple Instances of Geometry (Direct3D 9)"-article at MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/bb173349%28v=vs.85%29.aspx#Drawing_Non_Indexed_Geometry
		Renderer::Command::DrawIndexed::create(mCommandBuffer, 3, 2);

		// End debug event
		COMMAND_END_DEBUG_EVENT(mCommandBuffer)
	}

	// Right side (blue): Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
	if (getRenderer()->getCapabilities().drawInstanced)
	{
		// Begin debug event
		COMMAND_BEGIN_DEBUG_EVENT(mCommandBuffer, "Draw instanced")

		// Set the used pipeline state object (PSO)
		Renderer::Command::SetPipelineState::create(mCommandBuffer, mPipelineStateDrawInstanced);

		// Input assembly (IA): Set the used vertex array
		Renderer::Command::SetVertexArray::create(mCommandBuffer, mVertexArrayDrawInstanced);

		// Render the specified geometric primitive, based on an array of vertices
		Renderer::Command::Draw::create(mCommandBuffer, 3, 2);

		// End debug event
		COMMAND_END_DEBUG_EVENT(mCommandBuffer)
	}

	// End debug event
	COMMAND_END_DEBUG_EVENT(mCommandBuffer)
}
