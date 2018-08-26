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


// Amalgamated/unity build
#ifdef WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(disable: 4061))	// warning C4061: enumerator 'crnlib::PIXEL_FMT_DXT1' in switch of enum 'crnlib::pixel_format' is not explicitly handled by a case label
	__pragma(warning(disable: 4242))	// warning C4242: '=': conversion from 'sInt' to 'sU8', possible loss of data
	__pragma(warning(disable: 4244))	// warning C4244: 'initializing': conversion from 'crnlib::uint64' to 'crnlib::uint', possible loss of data
	__pragma(warning(disable: 4267))	// warning C4267: 'initializing': conversion from 'size_t' to 'crnlib::uint', possible loss of data
	__pragma(warning(disable: 4334))	// warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
	__pragma(warning(disable: 4365))	// warning C4365: '=': conversion from 'int' to '::size_t', signed/unsigned mismatch
	__pragma(warning(disable: 4456))	// warning C4456: declaration of 'lc' hides previous local declaration
	__pragma(warning(disable: 4458))	// warning C4458: declaration of 'c' hides class member
	__pragma(warning(disable: 4464))	// warning C4464: relative include path contains '..'
	__pragma(warning(disable: 4548))	// warning C4548: expression before comma has no effect; expected expression with side-effect
	__pragma(warning(disable: 4555))	// warning C4555: expression has no effect; expected expression with side-effect
	__pragma(warning(disable: 4571))	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	__pragma(warning(disable: 4574))	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	__pragma(warning(disable: 4457))	// warning C4457: declaration of 'n' hides function parameter
	__pragma(warning(disable: 4625))	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	__pragma(warning(disable: 4626))	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 4668))	// warning C4668: 'defined_WINBASE_' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	__pragma(warning(disable: 4774))	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	__pragma(warning(disable: 4838))	// warning C4838: conversion from 'const char' to 'mz_uint8' requires a narrowing conversion
	__pragma(warning(disable: 5026))	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	__pragma(warning(disable: 5027))	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 5039))	// warning C5039: 'tdefl_init': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
#endif
//# comp
#include "src/crn_qdxt5.cpp"
// crn
#include "src/crn_comp.cpp"
#include "src/crn_dxt_hc.cpp"
// image
#include "src/crn_jpge.cpp"
#include "src/crn_stb_image.cpp"
// lzmalib
#include "src/lzma_LzFindMt.cpp"
#include "src/lzma_LzmaDec.cpp"
// utils
#include "src/crn_checksum.cpp"
#include "src/crn_zeng.cpp"
