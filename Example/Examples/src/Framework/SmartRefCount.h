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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Framework/RefCount.h"


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Smart reference counter template
*/
template <class AType>
class SmartRefCount
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Default constructor
	*/
	inline SmartRefCount() :
		mPtr(nullptr)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] ptr
	*    Direct pointer to initialize with, can be a null pointer
	*/
	inline explicit SmartRefCount(AType* ptr) :
		mPtr(nullptr)
	{
		setPtr(ptr);
	}

	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] ptr
	*    Smart pointer to initialize with
	*/
	inline explicit SmartRefCount(const SmartRefCount<AType>& ptr) :
		mPtr(nullptr)
	{
		setPtr(ptr.getPtr());
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline ~SmartRefCount()
	{
		setPtr(nullptr);
	}

	/**
	*  @brief
	*    Assign a pointer
	*
	*  @param[in] ptr
	*    Direct pointer to assign, can be a null pointer
	*
	*  @return
	*    Reference to the smart pointer
	*/
	inline SmartRefCount<AType>& operator =(AType* ptr)
	{
		if (getPointer() != ptr)
		{
			setPtr(ptr);
		}
		return *this;
	}

	/**
	*  @brief
	*    Assign a smart pointer
	*
	*  @param[in] ptr
	*    Smart pointer to assign
	*
	*  @return
	*    Reference to the smart pointer
	*/
	inline SmartRefCount<AType>& operator =(const SmartRefCount<AType>& ptr)
	{
		if (getPointer() != ptr.getPointer())
		{
			setPtr(ptr.getPtr());
		}
		return *this;
	}

	/**
	*  @brief
	*    Get a direct pointer to the object
	*
	*  @return
	*    Pointer to the object, can be a null pointer
	*/
	inline AType* getPointer() const
	{
		return mPtr ? static_cast<AType*>(mPtr->getPointer()) : nullptr;
	}

	/**
	*  @brief
	*    Get a pointer to access the object
	*
	*  @return
	*    Pointer to the object, can be a null pointer
	*/
	inline AType* operator ->() const
	{
		return getPointer();
	}

	/**
	*  @brief
	*    Cast to a pointer to the object
	*
	*  @return
	*    Pointer to the object, can be a null pointer
	*/
	inline operator AType*() const
	{
		return getPointer();
	}

	/**
	*  @brief
	*    Check if the pointer is not a null pointer
	*
	*  @return
	*    "true" if the pointer is not a null pointer
	*/
	inline bool operator !() const
	{
		return (nullptr == getPointer());
	}

	/**
	*  @brief
	*    Check for equality
	*
	*  @param[in] ptr
	*    Direct pointer to compare with, can be a null pointer
	*
	*  @return
	*    "true" if the two pointers are equal
	*/
	inline bool operator ==(AType* ptr) const
	{
		return (getPointer() == ptr);
	}

	/**
	*  @brief
	*    Check for equality
	*
	*  @param[in] ptr
	*    Smart pointer to compare with
	*
	*  @return
	*    "true" if the two pointers are equal
	*/
	inline bool operator ==(const SmartRefCount<AType>& ptr) const
	{
		return (getPointer() == ptr.getPointer());
	}

	/**
	*  @brief
	*    Check for equality
	*
	*  @param[in] ptr
	*    Direct pointer to compare with, can be a null pointer
	*
	*  @return
	*    "true" if the two pointers are not equal
	*/
	inline bool operator !=(AType* ptr) const
	{
		return (getPointer() != ptr);
	}

	/**
	*  @brief
	*    Check for equality
	*
	*  @param[in] ptr
	*    Smart pointer to compare with
	*
	*  @return
	*    "true" if the two pointers are not equal
	*/
	inline bool operator !=(const SmartRefCount<AType>& ptr) const
	{
		return (getPointer() != ptr.getPointer());
	}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	/**
	*  @brief
	*    Assign a pointer to an object that implements RefCount
	*
	*  @param[in] ptr
	*    Pointer to assign, can be a null pointer
	*/
	inline void setPtr(AType* ptr)
	{
		// Release old pointer
		if (nullptr != mPtr)
		{
			mPtr->releaseReference();
		}

		// Assign new pointer
		if (nullptr != ptr)
		{
			ptr->addReference();
		}
		mPtr = ptr;
	}

	/**
	*  @brief
	*    Get pointer to the reference counted object
	*
	*  @return
	*    Pointer to the RefCount object, can be a null pointer
	*/
	inline AType* getPtr() const
	{
		// Return pointer
		return mPtr;
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	AType* mPtr; ///< Pointer to reference counter, can be a null pointer


};
