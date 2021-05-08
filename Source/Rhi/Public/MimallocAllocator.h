/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <mimalloc.h>
PRAGMA_WARNING_POP

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
			ASSERT(0 != alignment && !(alignment & (alignment - 1)), "The alignment must be a power of two")

			// Do the work
			if (nullptr != oldPointer && 0 != newNumberOfBytes)
			{
				// Reallocate
				return mi_realloc_aligned(oldPointer, newNumberOfBytes, alignment);
			}
			else
			{
				// Malloc / free

				// Null pointer is valid in here, does nothing in this case
				if (nullptr != oldPointer)
				{
					mi_free(oldPointer);
				}

				// Allocate
				return (0 != newNumberOfBytes) ? mi_malloc_aligned(newNumberOfBytes, alignment) : nullptr;
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
	*    Optional "mimalloc" ( https://github.com/microsoft/mimalloc ) memory allocator implementation class one can use
	*
	*  @note
	*    - Example: uint8_t* spirvOutputBuffer = RHI_MALLOC_TYPED(context, uint8_t, spirvOutputBufferSize);
	*    - Designed to be instanced and used inside a single C++ file
	*/
	class MimallocAllocator final : public IAllocator
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline MimallocAllocator() :
			IAllocator(&::detail::reallocate)
		{
			// Nothing here
		}

		inline virtual ~MimallocAllocator() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MimallocAllocator(const MimallocAllocator&) = delete;
		MimallocAllocator& operator=(const MimallocAllocator&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Rhi
