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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "PrecompiledHeader.h"
#include "Framework/CommandLineArguments.h"
#ifdef _WIN32
	#include "Framework/WindowsHeader.h"
	#ifdef UNICODE
		// Disable warnings in external headers, we can't fix them
		PRAGMA_WARNING_PUSH
			PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'const char' to 'utf8::uint8_t', signed/unsigned mismatch
			#include <utf8/utf8.h>	// To convert UTF-8 strings to UTF-16
		PRAGMA_WARNING_POP
	#else
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
			std::vector<std::wstring> lines(wargv + 1, wargv + wargc);
			for (std::vector<std::wstring>::iterator iterator = lines.begin(); iterator != lines.end(); ++iterator)
			{
				std::string utf8Line;
				utf8::utf16to8((*iterator).begin(), (*iterator).end(), std::back_inserter(utf8Line));
				mArguments.push_back(utf8Line);
			}
		}
		::LocalFree(wargv);
	#else
		std::string cmdLine(::GetCommandLineA());
		std::istringstream ss(cmdLine);
		std::istream_iterator<std::string> iss(ss);

		// The first token is the path+name of the program
		// -> Ignore it
		++iss;
		std::copy(iss,
			 std::istream_iterator<std::string>(),
			 std::back_inserter<std::vector<std::string>>(mArguments));
	#endif
#endif
}
