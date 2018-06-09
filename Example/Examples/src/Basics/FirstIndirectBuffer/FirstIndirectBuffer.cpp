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
#include "Basics/FirstIndirectBuffer/FirstIndirectBuffer.h"
#include "Framework/Color4.h"

#include <cassert>


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstIndirectBuffer::onInitialization()
{
	// Call the base implementation
	FirstTriangle::onInitialization();

	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		{ // Create the indirect buffer
			const Renderer::DrawInstancedArguments drawInstancedArguments =
			{
				3,	// vertexCountPerInstance (uint32_t)
				1,	// instanceCount (uint32_t)
				0,	// startVertexLocation (uint32_t)
				0	// startInstanceLocation (uint32_t)
			};
			mIndirectBuffer = mBufferManager->createIndirectBuffer(sizeof(Renderer::DrawInstancedArguments), &drawInstancedArguments, Renderer::BufferUsage::STATIC_DRAW);
		}

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		mCommandBuffer.clear();	// Throw away "FirstTriangle"-stuff
		fillCommandBuffer();
	}
}

void FirstIndirectBuffer::onDeinitialization()
{
	// Release the used resources
	mIndirectBuffer = nullptr;

	// Call the base implementation
	FirstTriangle::onDeinitialization();
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstIndirectBuffer::fillCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mPipelineState);
	assert(nullptr != mVertexArray);
	assert(nullptr != mIndirectBuffer);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

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
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Drawing the fancy triangle")

		// Render the specified geometric primitive, based on an array of vertices
		Renderer::Command::Draw::create(mCommandBuffer, *mIndirectBuffer);
	}
}
