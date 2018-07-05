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
#include "Advanced/IcosahedronTessellation/IcosahedronTessellation.h"
#include "Framework/Color4.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP

#include <float.h> // For FLT_MAX
#include <stdlib.h> // For rand()


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void IcosahedronTessellation::onInitialization()
{
	// Get and check the renderer instance
	// -> Uniform buffer object (UBO, "constant buffer" in Direct3D terminology) supported?
	// -> Geometry shaders supported?
	// -> Tessellation control and tessellation evaluation shaders supported?
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer && renderer->getCapabilities().maximumUniformBufferSize > 0 && renderer->getCapabilities().maximumNumberOfGsOutputVertices > 0 && renderer->getCapabilities().maximumNumberOfPatchVertices > 0)
	{
		// Create the buffer manager
		mBufferManager = renderer->createBufferManager();

		{ // Create the root signature
			// Setup
			Renderer::DescriptorRangeBuilder ranges[4];
			ranges[0].initialize(Renderer::DescriptorRangeType::UBV, 1, 0, "UniformBlockDynamicTcs", Renderer::ShaderVisibility::TESSELLATION_CONTROL);
			ranges[1].initialize(Renderer::DescriptorRangeType::UBV, 1, 0, "UniformBlockStaticTes", Renderer::ShaderVisibility::TESSELLATION_EVALUATION);
			ranges[2].initialize(Renderer::DescriptorRangeType::UBV, 1, 0, "UniformBlockStaticGs", Renderer::ShaderVisibility::GEOMETRY);
			ranges[3].initialize(Renderer::DescriptorRangeType::UBV, 1, 0, "UniformBlockStaticFs", Renderer::ShaderVisibility::FRAGMENT);

			Renderer::RootParameterBuilder rootParameters[1];
			rootParameters[0].initializeAsDescriptorTable(4, &ranges[0]);

			// Setup
			Renderer::RootSignatureBuilder rootSignature;
			rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = renderer->createRootSignature(rootSignature);
		}

		// Vertex input layout
		static constexpr Renderer::VertexAttribute vertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Position",									// name[32] (char)
				"POSITION",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(float) * 3,							// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			}
		};
		const Renderer::VertexAttributes vertexAttributes(static_cast<uint32_t>(glm::countof(vertexAttributesLayout)), vertexAttributesLayout);

		{ // Create vertex array object (VAO)
			// Create the vertex buffer object (VBO)
			// -> Geometry is from: http://prideout.net/blog/?p=48 (Philip Rideout, "The Little Grasshopper - Graphics Programming Tips")
			static constexpr float VERTEX_POSITION[] =
			{								// Vertex ID
				 0.000f,  0.000f,  1.000f,	// 0
				 0.894f,  0.000f,  0.447f,	// 1
				 0.276f,  0.851f,  0.447f,	// 2
				-0.724f,  0.526f,  0.447f,	// 3
				-0.724f, -0.526f,  0.447f,	// 4
				 0.276f, -0.851f,  0.447f,	// 5
				 0.724f,  0.526f, -0.447f,	// 6
				-0.276f,  0.851f, -0.447f,	// 7
				-0.894f,  0.000f, -0.447f,	// 8
				-0.276f, -0.851f, -0.447f,	// 9
				 0.724f, -0.526f, -0.447f,	// 10
				 0.000f,  0.000f, -1.000f	// 11
			};
			Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, Renderer::BufferUsage::STATIC_DRAW));

			// Create the index buffer object (IBO)
			// -> Geometry is from: http://prideout.net/blog/?p=48 (Philip Rideout, "The Little Grasshopper - Graphics Programming Tips")
			static constexpr uint16_t INDICES[] =
			{				// Triangle ID
				0,  1,  2,	// 0
				0,  2,  3,	// 1
				0,  3,  4,	// 2
				0,  4,  5,	// 3
				0,  5,  1,	// 4
				7,  6,  11,	// 5
				8,  7,  11,	// 6
				9,  8,  11,	// 7
				10,  9, 11,	// 8
				6, 10,  11,	// 9
				6,  2,  1,	// 10
				7,  3,  2,	// 11
				8,  4,  3,	// 12
				9,  5,  4,	// 13
				10,  1, 5,	// 14
				6,  7,  2,	// 15
				7,  8,  3,	// 16
				8,  9,  4,	// 17
				9, 10,  5,	// 18
				10,  6, 1	// 19
			};
			Renderer::IIndexBuffer* indexBuffer = mBufferManager->createIndexBuffer(sizeof(INDICES), Renderer::IndexBufferFormat::UNSIGNED_SHORT, INDICES, Renderer::BufferUsage::STATIC_DRAW);

			// Create vertex array object (VAO)
			// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
			// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
			// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
			//    reference of the used vertex buffer objects (VBO). If the reference counter of a
			//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			mVertexArray = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBuffer);
		}

		{ // Create the uniform buffer group
			Renderer::IResource* resources[4];

			// Create uniform buffers and fill the static buffers at once
			resources[0] = mUniformBufferDynamicTcs = mBufferManager->createUniformBuffer(sizeof(float) * 2, nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);
			{ // "ObjectSpaceToClipSpaceMatrix"
				glm::mat4 worldSpaceToViewSpaceMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 3.0f));		// Also known as "view matrix"
				glm::mat4 viewSpaceToClipSpaceMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 1000.0f, 0.001f);	// Also known as "projection matrix", near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				glm::mat4 objectSpaceToClipSpaceMatrix = viewSpaceToClipSpaceMatrix * worldSpaceToViewSpaceMatrix;			// Also known as "model view projection matrix"
				resources[1] = mBufferManager->createUniformBuffer(sizeof(float) * 4 * 4, glm::value_ptr(objectSpaceToClipSpaceMatrix), Renderer::BufferUsage::STATIC_DRAW);
				{ // "NormalMatrix"
					worldSpaceToViewSpaceMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
					viewSpaceToClipSpaceMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 1000.0f, 0.001f);	// Near and far flipped due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
					objectSpaceToClipSpaceMatrix = viewSpaceToClipSpaceMatrix * worldSpaceToViewSpaceMatrix;
					glm::mat3 nMVP(objectSpaceToClipSpaceMatrix);
					glm::mat4 tMVP(nMVP);
					resources[2] = mBufferManager->createUniformBuffer(sizeof(float) * 4 * 4, glm::value_ptr(tMVP), Renderer::BufferUsage::STATIC_DRAW);
				}
			}
			{ // Light and material
				static constexpr float LIGHT_AND_MATERIAL[] =
				{
					0.25f, 0.25f, 1.0f,  1.0,	// "LightPosition"
					 0.0f, 0.75f, 0.75f, 1.0, 	// "DiffuseMaterial"
					0.04f, 0.04f, 0.04f, 1.0,	// "AmbientMaterial"
				};
				resources[3] = mBufferManager->createUniformBuffer(sizeof(LIGHT_AND_MATERIAL), LIGHT_AND_MATERIAL, Renderer::BufferUsage::STATIC_DRAW);
			}

			// Create the uniform buffer group
			mUniformBufferGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources);
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
				const char* tessellationControlShaderSourceCode = nullptr;
				const char* tessellationEvaluationShaderSourceCode = nullptr;
				const char* geometryShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "IcosahedronTessellation_GLSL_450.h"	// For Vulkan
				#include "IcosahedronTessellation_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "IcosahedronTessellation_HLSL_D3D11_D3D12.h"
				#include "IcosahedronTessellation_Null.h"

				// Create the program
				program = shaderLanguage->createProgram(
					*mRootSignature,
					vertexAttributes,
					shaderLanguage->createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage->createTessellationControlShaderFromSourceCode(tessellationControlShaderSourceCode),
					shaderLanguage->createTessellationEvaluationShaderFromSourceCode(tessellationEvaluationShaderSourceCode),
					shaderLanguage->createGeometryShaderFromSourceCode(geometryShaderSourceCode, Renderer::GsInputPrimitiveTopology::TRIANGLES, Renderer::GsOutputPrimitiveTopology::TRIANGLES_STRIP, 3),
					shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
			}

			// Create the pipeline state object (PSO)
			if (nullptr != program)
			{
				Renderer::PipelineState pipelineState = Renderer::PipelineStateBuilder(mRootSignature, program, vertexAttributes, getMainRenderTarget()->getRenderPass());
				pipelineState.primitiveTopology = Renderer::PrimitiveTopology::PATCH_LIST_3;	// Patch list with 3 vertices per patch (tessellation relevant topology type) - "Renderer::PrimitiveTopology::TriangleList" used for tessellation
				pipelineState.primitiveTopologyType = Renderer::PrimitiveTopologyType::PATCH;
				mPipelineState = renderer->createPipelineState(pipelineState);
			}
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void IcosahedronTessellation::onDeinitialization()
{
	// Release the used resources
	mVertexArray = nullptr;
	mPipelineState = nullptr;
	mUniformBufferGroup = nullptr;
	mUniformBufferDynamicTcs = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mBufferManager = nullptr;
}

void IcosahedronTessellation::onDraw()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Update the uniform buffer content
		if (nullptr != mUniformBufferDynamicTcs)
		{
			// Copy data into the uniform buffer
			Renderer::MappedSubresource mappedSubresource;
			if (renderer->map(*mUniformBufferDynamicTcs, 0, Renderer::MapType::WRITE_DISCARD, 0, mappedSubresource))
			{
				const float data[] =
				{
					mTessellationLevelOuter,	// "TessellationLevelOuter"
					mTessellationLevelInner		// "TessellationLevelInner"
				};
				memcpy(mappedSubresource.data, data, sizeof(data));
				renderer->unmap(*mUniformBufferDynamicTcs, 0);
			}
		}

		// Submit command buffer to the renderer backend
		mCommandBuffer.submitToRenderer(*renderer);
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void IcosahedronTessellation::fillCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mUniformBufferDynamicTcs);
	assert(nullptr != mUniformBufferGroup);
	assert(nullptr != mPipelineState);
	assert(nullptr != mVertexArray);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
	Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mPipelineState);

	// Set graphics resource groups
	Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mUniformBufferGroup);

	// Input assembly (IA): Set the used vertex array
	Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

	// Render the specified geometric primitive, based on indexing into an array of vertices
	Renderer::Command::DrawIndexedGraphics::create(mCommandBuffer, 60);
}
