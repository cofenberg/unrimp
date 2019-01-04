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
#include "Examples/Private/Basics/FirstQueries/FirstQueries.h"
#include "Examples/Private/Framework/Color4.h"

#include <cassert>


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstQueries::onInitialization()
{
	// Call the base implementation
	FirstTriangle::onInitialization();

	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Create the queries
		mOcclusionQueryPool = renderer->createQueryPool(Renderer::QueryType::OCCLUSION);
		mPipelineStatisticsQueryPool = renderer->createQueryPool(Renderer::QueryType::PIPELINE_STATISTICS);
		mTimestampQueryPool = renderer->createQueryPool(Renderer::QueryType::TIMESTAMP, 2);

		// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		mCommandBuffer.clear();	// Throw away "FirstTriangle"-stuff
		fillCommandBuffer();
	}
}

void FirstQueries::onDeinitialization()
{
	// Release the used resources
	mOcclusionQueryPool = nullptr;
	mPipelineStatisticsQueryPool = nullptr;
	mTimestampQueryPool = nullptr;

	// Call the base implementation
	FirstTriangle::onDeinitialization();
}

void FirstQueries::onDraw()
{
	// Call the base implementation
	FirstTriangle::onDraw();

	// Get query results
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Sanity checks
		assert(nullptr != mOcclusionQueryPool);
		assert(nullptr != mPipelineStatisticsQueryPool);
		assert(nullptr != mTimestampQueryPool);

		{ // Occlusion query pool
			uint64_t numberOfSamples = 0;
			if (renderer->getQueryPoolResults(*mOcclusionQueryPool, sizeof(uint64_t), reinterpret_cast<uint8_t*>(&numberOfSamples)))
			{
				NOP;	// TODO(co) Process result
			}
		}

		{ // Pipeline statistics query pool
			Renderer::PipelineStatisticsQueryResult pipelineStatisticsQueryResult = {};
			if (renderer->getQueryPoolResults(*mPipelineStatisticsQueryPool, sizeof(Renderer::PipelineStatisticsQueryResult), reinterpret_cast<uint8_t*>(&pipelineStatisticsQueryResult)))
			{
				NOP;	// TODO(co) Process result
			}
		}

		{ // Timestamp query pool
			uint64_t timestamp[2] = {};
			if (renderer->getQueryPoolResults(*mTimestampQueryPool, sizeof(uint64_t) * 2, reinterpret_cast<uint8_t*>(&timestamp), 0, 2, sizeof(uint64_t)))
			{
				NOP;	// TODO(co) Process result
			}
		}
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstQueries::fillCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mGraphicsPipelineState);
	assert(nullptr != mVertexArray);
	assert(nullptr != mOcclusionQueryPool);
	assert(nullptr != mPipelineStatisticsQueryPool);
	assert(nullptr != mTimestampQueryPool);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Reset and begin queries
	Renderer::Command::ResetQueryPool::create(mCommandBuffer, *mTimestampQueryPool, 0, 2);
	Renderer::Command::WriteTimestampQuery::create(mCommandBuffer, *mTimestampQueryPool, 0);
	Renderer::Command::ResetAndBeginQuery::create(mCommandBuffer, *mOcclusionQueryPool);
	Renderer::Command::ResetAndBeginQuery::create(mCommandBuffer, *mPipelineStatisticsQueryPool, 0, Renderer::QueryControlFlags::PRECISE);

	// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
	Renderer::Command::ClearGraphics::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Renderer::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

	// Input assembly (IA): Set the used vertex array
	Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

	// Set debug marker
	// -> Debug methods: When using Direct3D <11.1, these methods map to the Direct3D 9 PIX functions
	//    (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)
	COMMAND_SET_DEBUG_MARKER(mCommandBuffer, "Everyone ready for the upcoming triangle?")

	{
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Drawing the fancy triangle")

		// Render the specified geometric primitive, based on an array of vertices
		Renderer::Command::DrawGraphics::create(mCommandBuffer, 3);
	}

	// End queries
	Renderer::Command::EndQuery::create(mCommandBuffer, *mOcclusionQueryPool);
	Renderer::Command::EndQuery::create(mCommandBuffer, *mPipelineStatisticsQueryPool);
	Renderer::Command::WriteTimestampQuery::create(mCommandBuffer, *mTimestampQueryPool, 1);
}
