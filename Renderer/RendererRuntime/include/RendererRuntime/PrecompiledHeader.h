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


// Do never ever include this optional precompiled header within another public header.
// The precompiled header is only used to speed up compilation, it's no replacement for
// a decent header design. Be clever, don't use precompiled headers like a sledgehammer
// or you increase instead of decrease the compile time faster as you can say "bad idea".


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
// We can't use pragma once here since precompiled headers get a special treatment on Mac OS X that is incompatible with #pragma once
#ifndef _RENDERER_RUNTIME_PRECOMPILED_HEADER_H_
#define _RENDERER_RUNTIME_PRECOMPILED_HEADER_H_


// This here should recognize when you try to include multiple precompiled headers.
// Use this block without modification of "_RENDERER_RUNTIME_PRECOMPILED_HEADER_INCLUDED_"
// in all precompiled headers of your project.
#ifdef _RENDERER_RUNTIME_PRECOMPILED_HEADER_INCLUDED_
	#error Multiple precompiled headers included. The reason could be inclusion of a precompiled header in a public header file.
#else
	#define _RENDERER_RUNTIME_PRECOMPILED_HEADER_INCLUDED_
#endif


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
// Unrimp headers which are considered to be stable and fundamental
#include "RendererRuntime/Core/Platform/PlatformTypes.h"

// GLM
// -> Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4574)	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4701)	// warning C4701: potentially uninitialized local variable 'Result' used
	#include <glm/glm.hpp>
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/quaternion.hpp>
	#include <glm/gtc/matrix_transform.hpp>
	#include <glm/gtx/matrix_decompose.hpp>
	#include <glm/gtx/quaternion.hpp>
PRAGMA_WARNING_POP

// C++ standard headers
// -> Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator '<x>' in switch of enum '<y>' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '<x>': conversion from '<y>' to '<z>', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4548)	// warning C4548: expression before comma has no effect; expected expression with side-effect
	PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: '<x>': default constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'Concurrency::details::_ExceptionHolder': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'Concurrency::details::_ExceptionHolder': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: '<x>' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(4987)	// warning C4987: nonstandard extension used: 'throw (...)'
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'Concurrency::details::_RunAllParam<Concurrency::details::_Unit_type>': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'Concurrency::details::_RunAllParam<Concurrency::details::_Unit_type>': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <future>
	#include <vector>
	#include <iterator>
	#include <algorithm>
	#include <inttypes.h>
	#include <cassert>
	#include <limits>
	#include <cstring>
	#include <queue>
	#include <deque>
	#include <string>
	#include <map>
	#include <unordered_map>
	#include <unordered_set>
	#include <type_traits>
	#include <mutex>
	#include <atomic>
	#include <thread>
	#include <condition_variable>
	#include <functional>
	#include <chrono>
	#include <ctime>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#endif // _RENDERER_RUNTIME_PRECOMPILED_HEADER_H_
