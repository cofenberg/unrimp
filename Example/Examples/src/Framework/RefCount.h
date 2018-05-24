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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Reference counter template
*
*  @note
*    - Initially the reference counter is 0
*/
template <class AType>
class RefCount
{


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Default constructor
	*/
	inline RefCount() :
		mRefCount(0)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~RefCount()
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Get a pointer to the object
	*
	*  @return
	*    Pointer to the reference counter's object, never a null pointer!
	*/
	inline const AType* getPointer() const
	{
		return reinterpret_cast<const AType*>(this);
	}

	inline AType* getPointer()
	{
		return reinterpret_cast<AType*>(this);
	}

	/**
	*  @brief
	*    Increases the reference count
	*
	*  @return
	*    Current reference count
	*/
	inline uint32_t addReference()
	{
		// Increment reference count
		++mRefCount;

		// Return current reference count
		return mRefCount;
	}

	/**
	*  @brief
	*    Decreases the reference count
	*
	*  @return
	*    Current reference count
	*
	*  @note
	*    - When the last reference was released, the instance is destroyed automatically
	*/
	inline uint32_t releaseReference()
	{
		// Decrement reference count
		if (mRefCount > 1)
		{
			--mRefCount;

			// Return current reference count
			return mRefCount;

		// Destroy object when no references are left
		}
		else
		{
			delete this;

			// This object is no longer
			return 0;
		}
	}

	/**
	*  @brief
	*    Gets the current reference count
	*
	*  @return
	*    Current reference count
	*/
	inline uint32_t getRefCount() const
	{
		// Return current reference count
		return mRefCount;
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	uint32_t mRefCount; ///< Reference count


};
