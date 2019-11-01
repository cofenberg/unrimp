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
#include "Renderer/Public/Core/Platform/PlatformTypes.h"


//[-------------------------------------------------------]
//[ Export/import                                         ]
//[-------------------------------------------------------]
// Is this project build as shared library?
#ifdef SHARED_LIBRARIES
	// Build as shared library
	#ifdef RENDERER_EXPORTS
		// Export
		#define RENDERER_API_EXPORT			GENERIC_API_EXPORT
		#define RENDERER_FUNCTION_EXPORT	GENERIC_FUNCTION_EXPORT
	#else
		// Import
		#define RENDERER_API_EXPORT			GENERIC_API_IMPORT
		#define RENDERER_FUNCTION_EXPORT
	#endif
#else
	// Build as static library
	#define RENDERER_API_EXPORT
	#define RENDERER_FUNCTION_EXPORT
#endif
