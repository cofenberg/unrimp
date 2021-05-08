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


// Amalgamated/unity build
#ifdef _WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(disable: 4191))	// warning C4191: 'type cast': unsafe conversion from 'FARPROC' to 'void (__cdecl *)(void)'
	__pragma(warning(disable: 5219))	// warning C5219: implicit conversion from 'mi_msecs_t' to 'double', possible loss of data
	__pragma(warning(disable: 4296))	// warning C4296: '>': expression is always false
	__pragma(warning(disable: 4365))	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	__pragma(warning(disable: 4625))	// warning C4625: 'mi_page_s': copy constructor was implicitly defined as deleted
	__pragma(warning(disable: 4626))	// warning C4626: 'mi_page_s': assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 4668))	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	__pragma(warning(disable: 5026))	// warning C5026: 'mi_page_s': move constructor was implicitly defined as deleted
	__pragma(warning(disable: 5027))	// warning C5027: 'mi_page_s': move assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 5039))	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception
#endif
#include "src/alloc.c"
#include "src/alloc-aligned.c"
#include "src/arena.c"
#include "src/bitmap.c"
#include "src/heap.c"
#include "src/init.c"
#include "src/options.c"
#include "src/os.c"
#include "src/page.c"
#include "src/random.c"
#include "src/region.c"
#include "src/segment.c"
#include "src/stats.c"
