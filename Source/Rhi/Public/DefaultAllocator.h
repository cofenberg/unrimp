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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

#include <stdlib.h>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] void* reallocate(Rhi::IAllocator&, void* oldPointer, size_t, size_t newNumberOfBytes, size_t alignment)
		{
			// Sanity check
			ASSERT((0 != alignment && !(alignment & (alignment - 1))) && "The alignment must be a power of two");

			// Do the work
			if (nullptr != oldPointer && 0 != newNumberOfBytes)
			{
				// Reallocate
				#ifdef _MSC_VER
					return _aligned_realloc(oldPointer, newNumberOfBytes, alignment);
				#else
					// TODO(co) Need aligned version, see e.g. https://github.com/philiptaylor/vulkan-sxs/blob/9cb21b3/common/AllocationCallbacks.cpp#L87 or XSIMD "xsimd_aligned_allocator"
					return realloc(oldPointer, newNumberOfBytes);
				#endif
			}
			else
			{
				// Malloc / free
				#ifdef _MSC_VER
					// Null pointer is valid in here, does nothing in this case
					::_aligned_free(oldPointer);

					// Allocate
					return (0 != newNumberOfBytes) ? ::_aligned_malloc(newNumberOfBytes, alignment) : nullptr;
				#elif defined(__ANDROID__)
					// Null pointer is valid in here, does nothing in this case
					::free(oldPointer);

					// Allocate
					if (0 != newNumberOfBytes)
					{
						void* memptr = nullptr;
						return (posix_memalign(&memptr, alignment, newNumberOfBytes) == 0) ? memptr : nullptr;
					}
					else
					{
						return nullptr;
					}
				#else
					// Null pointer is valid in here, does nothing in this case
					::free(oldPointer);

					// Allocate
					return (0 != newNumberOfBytes) ? ::aligned_alloc(alignment, newNumberOfBytes) : nullptr;
				#endif
			}
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Rhi
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Default memory allocator implementation class one can use
	*
	*  @note
	*    - Example: uint8_t* spirvOutputBuffer = RHI_MALLOC_TYPED(context, uint8_t, spirvOutputBufferSize);
	*    - Designed to be instanced and used inside a single C++ file
	*/
	class DefaultAllocator final : public IAllocator
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline DefaultAllocator() :
			IAllocator(&::detail::reallocate)
		{
			// Nothing here
		}

		inline virtual ~DefaultAllocator() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit DefaultAllocator(const DefaultAllocator&) = delete;
		DefaultAllocator& operator=(const DefaultAllocator&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Rhi
