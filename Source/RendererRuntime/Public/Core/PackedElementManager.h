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
#include "RendererRuntime/Public/Core/Manager.h"
#include "RendererRuntime/Public/Core/GetInvalid.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Packed element manager template
	*
	*  @note
	*    - Basing on "Managing Decoupling Part 4 -- The ID Lookup Table" https://github.com/niklasfrykholm/blog/blob/master/2011/managing-decoupling-4.md by Niklas Frykholm ( http://www.frykholm.se/ )
	*/
	template <class ELEMENT_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS>
	class PackedElementManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline PackedElementManager() :
			mNumberOfElements(0),
			mFreeListEnqueue(MAXIMUM_NUMBER_OF_ELEMENTS - 1),
			mFreeListDequeue(0)
		{
			for (uint32_t i = 0; i < MAXIMUM_NUMBER_OF_ELEMENTS; ++i)
			{
				mIndices[i].id = i;
				mIndices[i].next = static_cast<uint16_t>(i + 1);
			}
		}

		inline ~PackedElementManager()
		{
			// If there are any elements left alive, smash them
			for (size_t i = 0; i < mNumberOfElements; ++i)
			{
				mElements[i].deinitializeElement();
			}
		}

		[[nodiscard]] inline uint32_t getNumberOfElements() const
		{
			return mNumberOfElements;
		}

		[[nodiscard]] inline ELEMENT_TYPE& getElementByIndex(uint32_t index) const
		{
			return mElements[index];
		}

		[[nodiscard]] inline bool isElementIdValid(ID_TYPE id) const
		{
			if (isValid(id))
			{
				const Index& index = mIndices[id & INDEX_MASK];
				return (index.id == id && index.index != USHRT_MAX);
			}
			return false;
		}

		[[nodiscard]] inline ELEMENT_TYPE& getElementById(ID_TYPE id) const
		{
			ASSERT(isElementIdValid(id));
			return mElements[mIndices[id & INDEX_MASK].index];
		}

		[[nodiscard]] inline ELEMENT_TYPE* tryGetElementById(ID_TYPE id) const
		{
			if (isValid(id))
			{
				const Index& index = mIndices[id & INDEX_MASK];
				return (index.id == id && index.index != USHRT_MAX) ? &mElements[index.index] : nullptr;
			}
			return nullptr;
		}

		[[nodiscard]] inline ELEMENT_TYPE& addElement()
		{
			Index& index = mIndices[mFreeListDequeue];
			mFreeListDequeue = index.next;
			index.id += NEW_OBJECT_ID_ADD;
			index.index = static_cast<uint16_t>(mNumberOfElements++);

			// Initialize the added element
			// -> "placement new" ("new (static_cast<void*>(&element)) ELEMENT_TYPE(index.id);") is not used by intent to avoid some nasty STL issues
			ELEMENT_TYPE& element = mElements[index.index];
			element.initializeElement(index.id);

			// Return the added element
			return element;
		}

		inline void removeElement(ID_TYPE id)
		{
			ASSERT(isElementIdValid(id));
			Index& index = mIndices[id & INDEX_MASK];
			ELEMENT_TYPE& element = mElements[index.index];

			// Deinitialize the removed element
			// -> Calling the destructor ("element.~ELEMENT_TYPE();") is not used by intent to avoid some nasty STL issues
			element.deinitializeElement();
			--mNumberOfElements;

			// If this is the last element, there's no need to swap it with itself
			if (index.index != mNumberOfElements)
			{
				element = std::move(mElements[mNumberOfElements]);
				mIndices[element.getId() & INDEX_MASK].index = index.index;
			}

			// Update free list
			index.index = USHRT_MAX;
			mIndices[mFreeListEnqueue].next = (id & INDEX_MASK);
			mFreeListEnqueue = (id & INDEX_MASK);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit PackedElementManager(const PackedElementManager&) = delete;
		PackedElementManager& operator=(const PackedElementManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		static constexpr uint32_t INDEX_MASK		= 0xffff;
		static constexpr uint32_t NEW_OBJECT_ID_ADD	= 0x10000;

		struct Index final
		{
			ID_TYPE  id;
			uint16_t index;
			uint16_t next;
		};


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t			 mNumberOfElements;
		mutable ELEMENT_TYPE mElements[MAXIMUM_NUMBER_OF_ELEMENTS];
		Index				 mIndices[MAXIMUM_NUMBER_OF_ELEMENTS];
		uint16_t			 mFreeListEnqueue;
		uint16_t			 mFreeListDequeue;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
