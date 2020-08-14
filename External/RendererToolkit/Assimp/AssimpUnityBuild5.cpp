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


// Amalgamated/unity build
#ifdef _WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(disable: 4355))	// warning C4355: 'this': used in base member initializer list
	__pragma(warning(disable: 4619))	// warning C4619: #pragma warning: there is no warning number '4351'
	__pragma(warning(disable: 4777))	// warning C4777: 'snprintf' : format string '%Iu' requires an argument of type 'unsigned int', but variadic argument 1 has type 'const size_t'
	__pragma(warning(disable: 4826))	// warning C4826: Conversion from 'aiBone **' to 'uint64_t' is sign-extended. This may cause unexpected runtime behavior.
#endif
#include "Configuration.h"
// AssetLib
	// FBX
	#include "code/AssetLib/FBX/FBXImporter.cpp"
	#include "code/AssetLib/FBX/FBXParser.cpp"
