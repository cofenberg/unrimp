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
#ifdef WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(disable: 4061))	// warning C4061: enumerator 'crnlib::PIXEL_FMT_DXT1' in switch of enum 'crnlib::pixel_format' is not explicitly handled by a case label
	__pragma(warning(disable: 4242))	// warning C4242: '=': conversion from 'sInt' to 'sU8', possible loss of data
	__pragma(warning(disable: 4244))	// warning C4244: 'initializing': conversion from 'crnlib::uint64' to 'crnlib::uint', possible loss of data
	__pragma(warning(disable: 4267))	// warning C4267: 'initializing': conversion from 'size_t' to 'crnlib::uint', possible loss of data
	__pragma(warning(disable: 4334))	// warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
	__pragma(warning(disable: 4365))	// warning C4365: '=': conversion from 'int' to '::size_t', signed/unsigned mismatch
	__pragma(warning(disable: 4456))	// warning C4456: declaration of 'lc' hides previous local declaration
	__pragma(warning(disable: 4456))	// warning C4456: declaration of 'lc' hides previous local declaration
	__pragma(warning(disable: 4457))	// warning C4457: declaration of 'm' hides function parameter
	__pragma(warning(disable: 4464))	// warning C4464: relative include path contains '..'
	__pragma(warning(disable: 4499))	// warning C4499: 'static': an explicit specialization cannot have a storage class (ignored)
	__pragma(warning(disable: 4548))	// warning C4548: expression before comma has no effect; expected expression with side-effect
	__pragma(warning(disable: 4555))	// warning C4555: expression has no effect; expected expression with side-effect
	__pragma(warning(disable: 4571))	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	__pragma(warning(disable: 4574))	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	__pragma(warning(disable: 4625))	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	__pragma(warning(disable: 4626))	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 4668))	// warning C4668: 'defined_WINBASE_' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	__pragma(warning(disable: 4774))	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	__pragma(warning(disable: 4838))	// warning C4838: conversion from 'const char' to 'mz_uint8' requires a narrowing conversion
	__pragma(warning(disable: 5026))	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	__pragma(warning(disable: 5027))	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 5039))	// warning C5039: 'tdefl_init': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	__pragma(warning(disable: 5054))	// warning C5054: operator '|': deprecated between enumerations of different types
	__pragma(warning(disable: 5204))	// warning C5204: 'crnlib::task_pool::executable_task': class has virtual functions, but its trivial destructor is not virtual; instances of objects derived from this class may not be destructed correctly
	__pragma(warning(disable: 5220))	// warning C5220: 'crnlib::spinlock::m_flag': a non-static data member with a volatile qualified type no longer implies
#endif
// api
#include "src/crnlib.cpp"
//# comp
#include "src/crn_dxt.cpp"
#include "src/crn_dxt_endpoint_refiner.cpp"
#include "src/crn_dxt_fast.cpp"
#include "src/crn_dxt_hc_common.cpp"
#include "src/crn_dxt1.cpp"
#include "src/crn_dxt5a.cpp"
#include "src/crn_etc.cpp"
#include "src/crn_huffman_codes.cpp"
#include "src/crn_lzma_codec.cpp"
#include "src/crn_miniz.cpp"
#include "src/crn_prefix_coding.cpp"
#include "src/crn_qdxt1.cpp"
// #include "src/crn_qdxt5.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/crn_rg_etc1.cpp"
#include "src/crn_ryg_dxt.cpp"
#include "src/crn_symbol_codec.cpp"
#include "src/crn_hash_map.cpp"
// console
#include "src/crn_colorized_console.cpp"
#include "src/crn_console.cpp"
// containers
#include "src/crn_sparse_bit_array.cpp"
#include "src/crn_value.cpp"
#include "src/crn_vector.cpp"
// crn
//#include "src/crn_comp.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/crn_dds_comp.cpp"
#ifdef RENDERER_TOOLKIT_EXPORTS
	#include "src/crn_decomp.cpp"
#endif
// #include "src/crn_dxt_hc.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/crn_texture_comp.cpp"
// file
#include "src/crn_file_utils.cpp"
#include "src/crn_find_files.cpp"
// image
#include "src/crn_image_utils.cpp"
#include "src/crn_jpgd.cpp"
//#include "src/crn_jpge.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/crn_pixel_format.cpp"
#include "src/crn_resample_filters.cpp"
#include "src/crn_resampler.cpp"
// #include "src/crn_stb_image.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/crn_threaded_resampler.cpp"
// lzmalib
#include "src/lzma_7zBuf.cpp"
#include "src/lzma_7zBuf2.cpp"
#include "src/lzma_7zCrc.cpp"
#include "src/lzma_7zFile.cpp"
#include "src/lzma_7zStream.cpp"
#include "src/lzma_Alloc.cpp"
#include "src/lzma_Bcj2.cpp"
#include "src/lzma_Bra.cpp"
#include "src/lzma_Bra86.cpp"
#include "src/lzma_BraIA64.cpp"
#include "src/lzma_LzFind.cpp"
// #include "src/lzma_LzFindMt.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
// #include "src/lzma_LzmaDec.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/lzma_LzmaEnc.cpp"
#include "src/lzma_LzmaLib.cpp"
#include "src/lzma_Threads.cpp"
// math
#include "src/crn_math.cpp"
#include "src/crn_rand.cpp"
// stream
#include "src/crn_data_stream.cpp"
// string
#include "src/crn_dynamic_string.cpp"
#include "src/crn_strutils.cpp"
// texture
#include "src/crn_dxt_image.cpp"
#include "src/crn_ktx_texture.cpp"
#include "src/crn_mipmapped_texture.cpp"
// texture_conversion
#include "src/crn_texture_conversion.cpp"
#include "src/crn_texture_file_types.cpp"
// utils
#include "src/crn_arealist.cpp"
// #include "src/crn_checksum.cpp"	// Moved into "crunchUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "src/crn_command_line_params.cpp"
#include "src/crn_hash.cpp"
#include "src/crn_timer.cpp"
#include "src/crn_utils.cpp"
// root
#include "src/crn_assert.cpp"
#include "src/crn_core.cpp"
#include "src/crn_mem.cpp"
#include "src/crn_platform.cpp"
// platform
#ifdef WIN32
	#include "src/crn_threading_win32.cpp"
#elif
	#include "src/crn_threading_pthreads.cpp"
#endif
