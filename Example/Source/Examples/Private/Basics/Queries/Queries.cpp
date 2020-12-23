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
#include "Examples/Private/Basics/Queries/Queries.h"
#include "Examples/Private/Framework/Color4.h"


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void Queries::onInitialization()
{
	// Call the base implementation
	Triangle::onInitialization();

	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Create the queries
		mOcclusionQueryPool = rhi->createQueryPool(Rhi::QueryType::OCCLUSION);
		mPipelineStatisticsQueryPool = rhi->createQueryPool(Rhi::QueryType::PIPELINE_STATISTICS);
		mTimestampQueryPool = rhi->createQueryPool(Rhi::QueryType::TIMESTAMP, 2);

		// Since we're always dispatching the same commands to the RHI, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		mCommandBuffer.clear();	// Throw away "Triangle"-stuff
		fillCommandBuffer();
	}
}

void Queries::onDeinitialization()
{
	// Release the used resources
	mOcclusionQueryPool = nullptr;
	mPipelineStatisticsQueryPool = nullptr;
	mTimestampQueryPool = nullptr;

	// Call the base implementation
	Triangle::onDeinitialization();
}

void Queries::onDraw(Rhi::CommandBuffer& commandBuffer)
{
	// Call the base implementation
	Triangle::onDraw(commandBuffer);

	// Get query results
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Sanity checks
		RHI_ASSERT(rhi->getContext(), nullptr != mOcclusionQueryPool, "Invalid occlusion query pool")
		RHI_ASSERT(rhi->getContext(), nullptr != mPipelineStatisticsQueryPool, "Invalid pipeline statistics query pool")
		RHI_ASSERT(rhi->getContext(), nullptr != mTimestampQueryPool, "Invalid timestamp query pool")

		{ // Occlusion query pool
			uint64_t numberOfSamples = 0;
			if (rhi->getQueryPoolResults(*mOcclusionQueryPool, sizeof(uint64_t), reinterpret_cast<uint8_t*>(&numberOfSamples)))
			{
				NOP;	// TODO(co) Process result
			}
		}

		{ // Pipeline statistics query pool
			Rhi::PipelineStatisticsQueryResult pipelineStatisticsQueryResult = {};
			if (rhi->getQueryPoolResults(*mPipelineStatisticsQueryPool, sizeof(Rhi::PipelineStatisticsQueryResult), reinterpret_cast<uint8_t*>(&pipelineStatisticsQueryResult)))
			{
				NOP;	// TODO(co) Process result
			}
		}

		{ // Timestamp query pool
			uint64_t timestamp[2] = {};
			if (rhi->getQueryPoolResults(*mTimestampQueryPool, sizeof(uint64_t) * 2, reinterpret_cast<uint8_t*>(&timestamp), 0, 2, sizeof(uint64_t)))
			{
				NOP;	// TODO(co) Process result
			}
		}
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void Queries::fillCommandBuffer()
{
	// Sanity checks
	ASSERT(nullptr != getRhi(), "Invalid RHI instance")
	RHI_ASSERT(getRhi()->getContext(), mCommandBuffer.isEmpty(), "Command buffer is already filled")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mRootSignature, "Invalid root signature")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mGraphicsPipelineState, "Invalid graphics pipeline state")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mVertexArray, "Invalid vertex array")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mOcclusionQueryPool, "Invalid occlusion query pool")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mPipelineStatisticsQueryPool, "Invalid pipeline statistics query pool")
	RHI_ASSERT(getRhi()->getContext(), nullptr != mTimestampQueryPool, "Invalid timestamp query pool")

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Reset and begin queries
	Rhi::Command::ResetQueryPool::create(mCommandBuffer, *mTimestampQueryPool, 0, 2);
	Rhi::Command::WriteTimestampQuery::create(mCommandBuffer, *mTimestampQueryPool, 0);
	Rhi::Command::ResetAndBeginQuery::create(mCommandBuffer, *mOcclusionQueryPool);
	Rhi::Command::ResetAndBeginQuery::create(mCommandBuffer, *mPipelineStatisticsQueryPool, 0, Rhi::QueryControlFlags::PRECISE);

	// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
	Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Rhi::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Rhi::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

	// Input assembly (IA): Set the used vertex array
	Rhi::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

	// Set debug marker
	// -> Debug methods: When using Direct3D <11.1, these methods map to the Direct3D 9 PIX functions
	//    (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)
	COMMAND_SET_DEBUG_MARKER(mCommandBuffer, "Everyone ready for the upcoming triangle?")

	{
		// Scoped debug event
		COMMAND_SCOPED_DEBUG_EVENT(mCommandBuffer, "Drawing the fancy triangle")

		// Render the specified geometric primitive, based on an array of vertices
		Rhi::Command::DrawGraphics::create(mCommandBuffer, 3);
	}

	// End queries
	Rhi::Command::EndQuery::create(mCommandBuffer, *mOcclusionQueryPool);
	Rhi::Command::EndQuery::create(mCommandBuffer, *mPipelineStatisticsQueryPool);
	Rhi::Command::WriteTimestampQuery::create(mCommandBuffer, *mTimestampQueryPool, 1);
}
