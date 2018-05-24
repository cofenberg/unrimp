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
*    Reference counted pointer template
*
*  @remarks
*    RefCountPtr contains a pointer to the reference counted object,
*    while RefCount is used in derived classes and therefore is the
*    reference counted object itself.
*/
template <class AType>
class RefCountPtr : public RefCount<AType>
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] pointer
	*    Pointer to the referenced object, can be a null pointer
	*/
	inline explicit RefCountPtr(AType* pointer) :
		mPointer(pointer)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~RefCountPtr() override
	{
		// Delete the referenced object, if needed
		delete mPointer;
	}

	/**
	*  @brief
	*    Get a pointer to the object
	*
	*  @return
	*    Pointer to the reference counter's object, can be a null pointer
	*/
	inline virtual const AType* getPointer() const override
	{
		return mPointer;
	}

	inline virtual AType* getPointer() override
	{
		return mPointer;
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	AType* mPointer; ///< Pointer to the referenced object, can be a null pointer


};
