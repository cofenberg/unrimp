/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "ExampleProjectCompiler/Private/CommandLineArguments.h"
#ifdef _WIN32
	#include <Renderer/Public/Core/Platform/WindowsHeader.h>

	#ifndef UNICODE
		PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'initializing': conversion from 'int' to '::size_t', signed/unsigned mismatch
			PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: '_scprintf' : format string expected in argument 1 is not a string literal
			#include <sstream>
			#include <iterator>
			#include <algorithm>
		PRAGMA_WARNING_POP
	#endif
#endif


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
CommandLineArguments::CommandLineArguments()
{
#if _WIN32
	#ifdef UNICODE
		int wargc = 0;
		wchar_t** wargv = ::CommandLineToArgvW(GetCommandLineW(), &wargc);
		if (wargc > 0)
		{
			// argv[0] is the path+name of the program
			// -> Ignore it
			mArguments.reserve(static_cast<size_t>(wargc - 1));
			std::vector<std::wstring_view> lines(wargv + 1, wargv + wargc);
			for (std::vector<std::wstring_view>::iterator iterator = lines.begin(); iterator != lines.end(); ++iterator)
			{
				// Convert UTF-16 string to UTF-8
				std::string utf8Line;
				utf8Line.resize(static_cast<size_t>(::WideCharToMultiByte(CP_UTF8, 0, iterator->data(), static_cast<int>(iterator->size()), nullptr, 0, nullptr, nullptr)));
				::WideCharToMultiByte(CP_UTF8, 0, iterator->data(), static_cast<int>(iterator->size()), utf8Line.data(), static_cast<int>(utf8Line.size()), nullptr, nullptr);

				// Backup argument
				mArguments.push_back(utf8Line);
			}
		}
		::LocalFree(wargv);
	#else
		std::string_view cmdLine(::GetCommandLineA());
		std::istringstream ss(cmdLine);
		std::istream_iterator<std::string_view> iss(ss);

		// The first token is the path+name of the program
		// -> Ignore it
		++iss;
		std::copy(iss,
			 std::istream_iterator<std::string_view>(),
			 std::back_inserter<std::vector<std::string_view>>(mArguments));
	#endif
#endif
}
