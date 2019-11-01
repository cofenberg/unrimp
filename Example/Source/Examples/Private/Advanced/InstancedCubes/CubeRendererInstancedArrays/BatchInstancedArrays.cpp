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
#include "Examples/Private/Advanced/InstancedCubes/CubeRendererInstancedArrays/BatchInstancedArrays.h"

#include <Renderer/Public/Core/Math/EulerAngles.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
	#include <glm/gtc/quaternion.hpp>
PRAGMA_WARNING_POP

#include <stdlib.h> // For rand()


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
void BatchInstancedArrays::initialize(Rhi::IBufferManager& bufferManager, Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexBuffer& vertexBuffer, Rhi::IIndexBuffer& indexBuffer, Rhi::IGraphicsProgram& graphicsProgram, Rhi::IRenderPass& renderPass, uint32_t numberOfCubeInstances, bool alphaBlending, uint32_t numberOfTextures, uint32_t sceneRadius)
{
	// Set owner RHI instance
	mRhi = &graphicsProgram.getRhi();

	// Release previous data if required
	mVertexArray = nullptr;

	// Set the number of cube instance
	mNumberOfCubeInstances = numberOfCubeInstances;

	{ // Create the texture buffer instance
		// Allocate the local per instance data
		const uint32_t numberOfElements = mNumberOfCubeInstances * 2 * 4;
		float *data = new float[numberOfElements];
		float *dataCurrent = data;

		// Set data
		// -> Layout: [Position][Rotation][Position][Rotation]...
		//    - Position: xyz=Position, w=Slice of the 2D texture array to use
		//    - Rotation: Rotation quaternion (xyz) and scale (w)
		//      -> We don't need to store the w component of the quaternion. It's normalized and storing
		//         three components while recomputing the fourths component is be sufficient.
		glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);	// Identity rotation quaternion
		for (uint32_t i = 0; i < mNumberOfCubeInstances; ++i)
		{
			{ // Position
				// r=x
				*dataCurrent = -static_cast<int>(sceneRadius) + 2.0f * sceneRadius * (rand() % 65536) / 65536.0f;
				++dataCurrent;
				// g=y
				*dataCurrent = -static_cast<int>(sceneRadius) + 2.0f * sceneRadius * (rand() % 65536) / 65536.0f;
				++dataCurrent;
				// b=z
				*dataCurrent = -static_cast<int>(sceneRadius) + 2.0f * sceneRadius * (rand() % 65536) / 65536.0f;
				++dataCurrent;
				// a=Slice of the 2D texture array to use
				*dataCurrent = static_cast<float>(rand() % numberOfTextures); // Choose a random texture
				++dataCurrent;
			}

			{ // Rotation
				rotation = Renderer::EulerAngles::eulerToQuaternion((rand() % 65536) / 65536.0f, (rand() % 65536) / 65536.0f * 2.0f, (rand() % 65536) / 65536.0f * 3.0f);

				// r=x
				*dataCurrent = rotation.x;
				++dataCurrent;
				// g=y
				*dataCurrent = rotation.y;
				++dataCurrent;
				// b=z
				*dataCurrent = rotation.z;
				++dataCurrent;
				// a=scale
				*dataCurrent = 2.0f * (rand() % 65536) / 65536.0f;
				++dataCurrent;
			}
		}

		// Create the vertex buffer object (VBO) instance containing the per-instance-data
		Rhi::IVertexBuffer *vertexBufferPerInstanceData = bufferManager.createVertexBuffer(sizeof(float) * numberOfElements, data);

		{ // Create vertex array object (VAO)
			// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
			// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
			// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
			//    reference of the used vertex buffer objects (VBO). If the reference counter of a
			//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { &vertexBuffer, vertexBufferPerInstanceData };
			mVertexArray = bufferManager.createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, &indexBuffer);
		}

		// Free local per instance data
		delete [] data;
	}

	{ // Create the graphics pipeline state object (PSO)
		Rhi::GraphicsPipelineState graphicsPipelineState = Rhi::GraphicsPipelineStateBuilder(&rootSignature, &graphicsProgram, vertexAttributes, renderPass);
		graphicsPipelineState.blendState.renderTarget[0].blendEnable = alphaBlending;
		graphicsPipelineState.blendState.renderTarget[0].srcBlend    = Rhi::Blend::SRC_ALPHA;
		graphicsPipelineState.blendState.renderTarget[0].destBlend   = Rhi::Blend::ONE;
		mGraphicsPipelineState = mRhi->createGraphicsPipelineState(graphicsPipelineState);
	}
}

void BatchInstancedArrays::fillCommandBuffer(Rhi::CommandBuffer& commandBuffer) const
{
	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(commandBuffer)

	// Set the used graphics pipeline state object (PSO)
	Rhi::Command::SetGraphicsPipelineState::create(commandBuffer, mGraphicsPipelineState);

	// Setup input assembly (IA): Set the used vertex array
	Rhi::Command::SetGraphicsVertexArray::create(commandBuffer, mVertexArray);

	// Use instancing in order to draw multiple cubes with just a single draw call
	// -> Draw calls are one of the most expensive rendering, avoid them if possible
	Rhi::Command::DrawIndexedGraphics::create(commandBuffer, 36, mNumberOfCubeInstances);
}
