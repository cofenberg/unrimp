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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to '::size_t', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_UInt_is_zero': default constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <string>
	#include <mutex>
PRAGMA_WARNING_POP

#ifdef _WIN32
	#include <iostream>
	#include <cstdarg>

	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		// Set Windows version to Windows Vista (0x0600), we don't support Windows XP (0x0501)
		#ifdef WINVER
			#undef WINVER
		#endif
		#define WINVER			0x0600
		#ifdef _WIN32_WINNT
			#undef _WIN32_WINNT
		#endif
		#define _WIN32_WINNT	0x0600

		// Exclude some stuff from "windows.h" to speed up compilation a bit
		#define WIN32_LEAN_AND_MEAN
		#define NOGDICAPMASKS
		#define NOMENUS
		#define NOICONS
		#define NOKEYSTATES
		#define NOSYSCOMMANDS
		#define NORASTEROPS
		#define OEMRESOURCE
		#define NOATOM
		#define NOMEMMGR
		#define NOMETAFILE
		#define NOOPENFILE
		#define NOSCROLL
		#define NOSERVICE
		#define NOSOUND
		#define NOWH
		#define NOCOMM
		#define NOKANJI
		#define NOHELP
		#define NOPROFILER
		#define NODEFERWINDOWPOS
		#define NOMCX
		#define NOCRYPT
		#include <Windows.h>

		// Get rid of some nasty OS macros
		#undef min
		#undef max
	PRAGMA_WARNING_POP
#elif __ANDROID__
	#include <android/log.h>
#elif LINUX
	#include <iostream>
	#include <cstdarg>
#else
	#error "Unsupported platform"
#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Rhi
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Default assert implementation class one can use
	*
	*  @note
	*    - Example: RHI_ASSERT(mContext, isInitialized, "Direct3D 11 RHI implementation assert failed")
	*    - Designed to be instanced and used inside a single C++ file
	*    - On Microsoft Windows it will print to the Visual Studio output console and the debugger will break
	*    - On Linux it will print on the console
	*/
	class DefaultAssert final : public IAssert
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline DefaultAssert()
		{
			// Nothing here
		}

		inline virtual ~DefaultAssert() override
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IAssert methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual bool handleAssert(const char* expression, const char* file, uint32_t line, const char* format, ...) override
		{
			bool requestDebugBreak = false;

			// Get the required buffer length, does not include the terminating zero character
			va_list vaList;
			va_start(vaList, format);
			const uint32_t textLength = static_cast<uint32_t>(vsnprintf(nullptr, 0, format, vaList));
			va_end(vaList);
			if (256 > textLength)
			{
				// Fast path: C-runtime stack

				// Construct the formatted text
				char formattedText[256];	// 255 +1 for the terminating zero character
				va_start(vaList, format);
				vsnprintf(formattedText, 256, format, vaList);
				va_end(vaList);

				// Internal processing
				requestDebugBreak = handleAssertInternal(expression, file, line, formattedText, textLength);
			}
			else
			{
				// Slow path: Heap
				// -> No reused scratch buffer in order to reduce memory allocation/deallocations in here to not make things more complex and to reduce the mutex locked scope

				// Construct the formatted text
				char* formattedText = new char[textLength + 1];	// 1+ for the terminating zero character
				va_start(vaList, format);
				vsnprintf(formattedText, textLength + 1, format, vaList);
				va_end(vaList);

				// Internal processing
				requestDebugBreak = handleAssertInternal(expression, file, line, formattedText, textLength);

				// Cleanup
				delete [] formattedText;
			}

			// Done
			return requestDebugBreak;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::DefaultAssert methods          ]
	//[-------------------------------------------------------]
	protected:
		/*
		*  @brief
		*    Receives an already formatted message for further processing
		*
		*  @param[in] expression
		*    Expression as ASCII string
		*  @param[in] file
		*    File as ASCII string
		*  @param[in] line
		*    Line number
		*  @param[in] message
		*    UTF-8 message
		*  @param[in] numberOfCharacters
		*    Number of characters inside the message, does not include the terminating zero character
		*
		*  @return
		*    "true" to request debug break, else "false"
		*/
		[[nodiscard]] inline virtual bool handleAssertInternal(const char* expression, const char* file, uint32_t line, const char* message, uint32_t)
		{
			std::lock_guard<std::mutex> mutexLock(mMutex);
			bool requestDebugBreak = false;

			// Construct the full UTF-8 message text
			std::string fullMessage = "Assert message \"" + std::string(message) + "\" | Expression \"" + std::string(expression) + "\" | File \"" + std::string(file) + "\" | Line " + std::to_string(line);
			if ('\n' != fullMessage.back())
			{
				fullMessage += '\n';
			}

			// Platform specific handling
			#ifdef _WIN32
			{
				// Convert UTF-8 string to UTF-16
				std::wstring utf16Line;
				utf16Line.resize(static_cast<std::wstring::size_type>(::MultiByteToWideChar(CP_UTF8, 0, fullMessage.c_str(), static_cast<int>(fullMessage.size()), nullptr , 0)));
				::MultiByteToWideChar(CP_UTF8, 0, fullMessage.c_str(), static_cast<int>(fullMessage.size()), utf16Line.data(), static_cast<int>(utf16Line.size()));

				// Write into standard output stream
				std::wcerr << utf16Line.c_str();

				// On Microsoft Windows, ensure the output can be seen inside the Visual Studio output window as well
				::OutputDebugStringW(utf16Line.c_str());
				if (::IsDebuggerPresent())
				{
					requestDebugBreak = true;
				}
			}
			#elif __ANDROID__
				__android_log_write(ANDROID_LOG_DEBUG, "Unrimp", fullMessage.c_str());	// TODO(co) Might make sense to make the app-name customizable
				requestDebugBreak = true;
			#elif LINUX
				// Write into standard output stream
				std::cerr << fullMessage.c_str();
				requestDebugBreak = true;
			#else
				#error "Unsupported platform"
			#endif

			// Done
			return requestDebugBreak;
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		std::mutex mMutex;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit DefaultAssert(const DefaultAssert&) = delete;
		DefaultAssert& operator=(const DefaultAssert&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Rhi
