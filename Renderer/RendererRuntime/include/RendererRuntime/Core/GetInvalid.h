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
#include "RendererRuntime/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <limits>	// For "std::numeric_limits()"
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	/**
	*  @brief
	*    Return invalid default value for a given type
	*/
	template <typename TYPE>
	TYPE getInvalid()
	{
		return std::numeric_limits<TYPE>::max();
	}

	/**
	*  @brief
	*    Return invalid default value for a type provided by a variable
	*/
	template <typename TYPE>
	TYPE getInvalid(const TYPE&)
	{
		return std::numeric_limits<TYPE>::max();
	}

	/**
	*  @brief
	*    Set the given variable to invalid default value
	*/
	template <typename TYPE>
	void setInvalid(TYPE& value)
	{
		value = getInvalid<TYPE>();
	}

	/**
	*  @brief
	*    Check whether or not the given variable has not the invalid default value
	*/
	template <typename TYPE>
	bool isValid(const TYPE& value)
	{
		return (getInvalid<TYPE>() != value);
	}

	/**
	*  @brief
	*    Check whether or not the given variable has the invalid default value
	*/
	template <typename TYPE>
	bool isInvalid(const TYPE& value)
	{
		return (getInvalid<TYPE>() == value);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
