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


// TODO(co) The C++17 filesystem is still experimental and hence not handled in an uniform way cross platform. Until C++17 has been released, we need to use those helper.


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Export.h"


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#ifdef WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(push))
		__pragma(warning(disable: 4365))	// warning C4365: 'return': conversion from 'int' to 'std::_Rand_urng_from_func::result_type', signed/unsigned mismatch
		__pragma(warning(disable: 4530))	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
		__pragma(warning(disable: 4548))	// warning C4548: expression before comma has no effect; expected expression with side-effect
		__pragma(warning(disable: 4571))	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
		__pragma(warning(disable: 4625))	// warning C4625: 'std::messages_base': copy constructor was implicitly defined as deleted
		__pragma(warning(disable: 4626))	// warning C4626: 'std::messages<char>': assignment operator was implicitly defined as deleted
		__pragma(warning(disable: 4774))	// warning C4774: '_scprintf' : format string expected in argument 1 is not a string literal
		__pragma(warning(disable: 5026))	// warning C5026: 'std::messages_base': move constructor was implicitly defined as deleted
		__pragma(warning(disable: 5027))	// warning C5027: 'std::messages_base': move assignment operator was implicitly defined as deleted
		#include <filesystem>
	__pragma(warning(pop))
#else
	#ifdef UNRIMP_USE_BOOST_FILESYSTEM
		#include <boost/filesystem.hpp>
	#else
		#include <experimental/filesystem>
	#endif
#endif


//[-------------------------------------------------------]
//[ Global definitions                                    ]
//[-------------------------------------------------------]
#ifdef UNRIMP_USE_BOOST_FILESYSTEM
	namespace std_filesystem = boost::filesystem;
#else
	namespace std_filesystem = std::experimental::filesystem;
#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class FileSystemHelper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		RENDERERRUNTIME_API_EXPORT static std_filesystem::path lexicallyNormal(const std_filesystem::path& path);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		FileSystemHelper(const FileSystemHelper&) = delete;
		FileSystemHelper& operator=(const FileSystemHelper&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
