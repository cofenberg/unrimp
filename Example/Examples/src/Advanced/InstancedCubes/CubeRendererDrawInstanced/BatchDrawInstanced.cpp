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
#include "Advanced/InstancedCubes/CubeRendererDrawInstanced/BatchDrawInstanced.h"

#include <RendererRuntime/Core/Math/EulerAngles.h>

#include <stdlib.h> // For rand()


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
void BatchDrawInstanced::initialize(Renderer::IBufferManager& bufferManager, Renderer::IRootSignature& rootSignature, const Renderer::VertexAttributes& vertexAttributes, Renderer::IProgram& program, Renderer::IRenderPass& renderPass, uint32_t numberOfCubeInstances, bool alphaBlending, uint32_t numberOfTextures, uint32_t sceneRadius)
{
	// Set owner renderer instance
	mRenderer = &program.getRenderer();

	// Release previous data if required
	mTextureBufferGroup = nullptr;

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
				rotation = RendererRuntime::EulerAngles::eulerToQuaternion((rand() % 65536) / 65536.0f, (rand() % 65536) / 65536.0f * 2.0f, (rand() % 65536) / 65536.0f * 3.0f);

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

		{ // Create the texture buffer instance and wrap it into a resource group instance
			Renderer::IResource* resource = bufferManager.createTextureBuffer(sizeof(float) * numberOfElements, Renderer::TextureFormat::R32G32B32A32F, data, Renderer::BufferUsage::STATIC_DRAW);
			mTextureBufferGroup = rootSignature.createResourceGroup(1, 1, &resource);
		}

		// Free local per instance data
		delete [] data;
	}

	{ // Create the pipeline state object (PSO)
		Renderer::PipelineState pipelineState = Renderer::PipelineStateBuilder(&rootSignature, &program, vertexAttributes, renderPass);
		pipelineState.blendState.renderTarget[0].blendEnable = alphaBlending;
		pipelineState.blendState.renderTarget[0].srcBlend    = Renderer::Blend::SRC_ALPHA;
		pipelineState.blendState.renderTarget[0].destBlend   = Renderer::Blend::ONE;
		mPipelineState = mRenderer->createPipelineState(pipelineState);
	}
}

void BatchDrawInstanced::fillCommandBuffer(Renderer::CommandBuffer& commandBuffer) const
{
	// Begin debug event
	COMMAND_BEGIN_DEBUG_EVENT_FUNCTION(commandBuffer)

	// Set the used pipeline state object (PSO)
	Renderer::Command::SetPipelineState::create(commandBuffer, mPipelineState);

	// Set resource groups
	Renderer::Command::SetGraphicsResourceGroup::create(commandBuffer, 1, mTextureBufferGroup);

	// Use instancing in order to draw multiple cubes with just a single draw call
	// -> Draw calls are one of the most expensive rendering, avoid them if possible
	Renderer::Command::DrawIndexed::create(commandBuffer, 36, mNumberOfCubeInstances);

	// End debug event
	COMMAND_END_DEBUG_EVENT(commandBuffer)
}
