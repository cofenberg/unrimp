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
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/IndirectBufferManager.h"
#include "Renderer/Public/IRenderer.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static uint32_t DEFAULT_INDIRECT_BUFFER_NUMBER_OF_BYTES = 128 * 1024;	// 128 KiB


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	IndirectBufferManager::IndirectBufferManager(const IRenderer& renderer) :
		mRenderer(renderer),
		mMaximumIndirectBufferSize(std::min(renderer.getRhi().getCapabilities().maximumIndirectBufferSize, ::detail::DEFAULT_INDIRECT_BUFFER_NUMBER_OF_BYTES)),
		mCurrentIndirectBuffer(nullptr),
		mPreviouslyRequestedNumberOfBytes(0)
	{
		// The maximum indirect buffer size must be a multiple of "Rhi::DrawIndexedArguments"
		mMaximumIndirectBufferSize -= (mMaximumIndirectBufferSize % sizeof(Rhi::DrawIndexedArguments));
	}

	IndirectBufferManager::~IndirectBufferManager()
	{
		// At this point in time, no indirect buffers should be in use anymore
		RHI_ASSERT(mRenderer.getContext(), mUsedIndirectBuffers.empty(), "Invalid used indirect buffers")
		RHI_ASSERT(mRenderer.getContext(), nullptr == mCurrentIndirectBuffer, "Invalid current indirect buffer")
		RHI_ASSERT(mRenderer.getContext(), 0 == mPreviouslyRequestedNumberOfBytes, "Invalid previously requested number of bytes")

		// Destroy all indirect buffers
		for (IndirectBuffer& indirectBuffer : mFreeIndirectBuffers)
		{
			indirectBuffer.indirectBuffer->releaseReference();
		}
	}

	IndirectBufferManager::IndirectBuffer* IndirectBufferManager::getIndirectBuffer(uint32_t numberOfBytes)
	{
		// Sanity check
		RHI_ASSERT(mRenderer.getContext(), numberOfBytes > 0, "Don't call this method if there's no work to be done")
		RHI_ASSERT(mRenderer.getContext(), numberOfBytes <= mMaximumIndirectBufferSize, "Maximum indirect buffer size exceeded")

		// Is there enough space left inside the current indirect buffer?
		if (nullptr != mCurrentIndirectBuffer)
		{
			// Advance indirect buffer offset using the previously requested number of bytes which are consumed now
			mCurrentIndirectBuffer->indirectBufferOffset += mPreviouslyRequestedNumberOfBytes;
			if ((mCurrentIndirectBuffer->indirectBufferOffset + numberOfBytes) > mMaximumIndirectBufferSize)
			{
				// Out of space
				unmapCurrentIndirectBuffer();
			}
		}
		mPreviouslyRequestedNumberOfBytes = numberOfBytes;

		// Create new indirect buffer, if required
		if (nullptr == mCurrentIndirectBuffer)
		{
			if (mFreeIndirectBuffers.empty())
			{
				// Create new indirect buffer instance
				Rhi::IIndirectBuffer* rhiIndirectBuffer = mRenderer.getBufferManager().createIndirectBuffer(mMaximumIndirectBufferSize, nullptr, Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS, Rhi::BufferUsage::DYNAMIC_DRAW);
				RHI_SET_RESOURCE_DEBUG_NAME(rhiIndirectBuffer, "Indirect buffer manager")
				mUsedIndirectBuffers.emplace_back(rhiIndirectBuffer);
			}
			else
			{
				// Use existing free indirect buffer instance
				mUsedIndirectBuffers.push_back(mFreeIndirectBuffers.back());
				mFreeIndirectBuffers.pop_back();
			}
			mCurrentIndirectBuffer = &mUsedIndirectBuffers.back();

			{ // Map
				// Sanity checks
				RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentIndirectBuffer->indirectBuffer, "Invalid current indirect buffer")
				RHI_ASSERT(mRenderer.getContext(), 0 == mCurrentIndirectBuffer->indirectBufferOffset, "Invalid current indirect buffer")
				RHI_ASSERT(mRenderer.getContext(), nullptr == mCurrentIndirectBuffer->mappedData, "Invalid current indirect buffer")

				// Map
				Rhi::MappedSubresource mappedSubresource;
				if (mRenderer.getRhi().map(*mCurrentIndirectBuffer->indirectBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, mappedSubresource))
				{
					mCurrentIndirectBuffer->mappedData = static_cast<uint8_t*>(mappedSubresource.data);
				}
				RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentIndirectBuffer->mappedData, "Invalid current indirect buffer")
			}
		}

		// Done
		return mCurrentIndirectBuffer;
	}

	void IndirectBufferManager::onPreCommandBufferExecution()
	{
		// Unmap current indirect buffer
		if (nullptr != mCurrentIndirectBuffer)
		{
			unmapCurrentIndirectBuffer();
		}

		// Free all used indirect buffers
		mFreeIndirectBuffers.insert(mFreeIndirectBuffers.end(), mUsedIndirectBuffers.begin(), mUsedIndirectBuffers.end());
		mUsedIndirectBuffers.clear();
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void IndirectBufferManager::unmapCurrentIndirectBuffer()
	{
		// Sanity checks
		RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentIndirectBuffer, "Invalid current indirect buffer")
		RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentIndirectBuffer->mappedData, "Invalid current indirect buffer")

		// Unmap
		mRenderer.getRhi().unmap(*mCurrentIndirectBuffer->indirectBuffer, 0);
		mCurrentIndirectBuffer->indirectBufferOffset = 0;
		mCurrentIndirectBuffer->mappedData = nullptr;
		mCurrentIndirectBuffer = nullptr;
		mPreviouslyRequestedNumberOfBytes = 0;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
