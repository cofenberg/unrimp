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
	#ifndef __ANDROID__
		#include <filesystem>
		#include <fstream>
	#endif
	#include <string>
	#include <mutex>
PRAGMA_WARNING_POP

#ifdef _WIN32
	// Disable warnings in external headers, we can't fix them
	PRAGMA_WARNING_PUSH
		PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
		PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
		PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
		#include <iostream>
		#include <cstdarg>
	PRAGMA_WARNING_POP

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

		// Disable warnings in external headers, we can't fix them
		__pragma(warning(push))
			__pragma(warning(disable: 5039))	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception. (compiling source file src\CommandLineArguments.cpp)
			#include <Windows.h>
		__pragma(warning(pop))

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
	*    Default log implementation class one can use
	*
	*  @note
	*    - Example: RHI_LOG(mContext, DEBUG, "Direct3D 11 RHI implementation startup")
	*    - Designed to be instanced and used inside a single C++ file
	*    - On Microsoft Windows it will print with colors on the console and a file in a separate thread, it will also print to the Visual Studio output console, on critical message the debugger will break
	*    - On Linux it will print with colors on the console and a file in a separate thread
	*    - On Android it will print into the Android system log
	*/
	class DefaultLog : public ILog
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline DefaultLog([[maybe_unused]] const std::string& absoluteLogDirectory = "", const std::string& prefix = "", [[maybe_unused]] bool verbose = false)
			#ifdef _DEBUG
				: mVerbose(verbose)
			#endif
			#ifndef __ANDROID__
				, mAbsoluteLogDirectory(absoluteLogDirectory)
				, mPrefix(prefix)
			#endif
		{
			#ifndef __ANDROID__
				// Create the thread responsible for writing into the log file
				mWorkerThread = new std::thread(DefaultLog::StaticThreadFunction, this);
			#endif
		}

		inline virtual ~DefaultLog() override
		{
			#ifndef __ANDROID__
				mThreadShouldBeRunning = false;
				mConditionVariable.notify_one();
				mWorkerThread->join();
				delete mWorkerThread;
				mOfstream.close();
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::ILog methods                      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual bool print(Type type, const char* attachment, const char* file, uint32_t line, const char* format, ...) override
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
				requestDebugBreak = printInternal(type, attachment, file, line, formattedText, textLength);
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
				requestDebugBreak = printInternal(type, attachment, file, line, formattedText, textLength);

				// Cleanup
				delete [] formattedText;
			}

			// Done
			return requestDebugBreak;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::DefaultLog methods             ]
	//[-------------------------------------------------------]
	protected:
		/*
		*  @brief
		*    Receives an already formatted message for further processing
		*
		*  @param[in] type
		*    Log message type
		*  @param[in] attachment
		*    Optional attachment (for example build shader source code), can be a null pointer
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
		[[nodiscard]] inline virtual bool printInternal(Type type, const char*, [[maybe_unused]] const char* file, [[maybe_unused]] uint32_t line, const char* message, uint32_t)
		{
			std::lock_guard<std::mutex> mutexLock(mMutex);
			bool requestDebugBreak = false;

			// Timestamp
			// -> No timestamp in case one writes e.g. "RHI_LOG(INFORMATION, "")" -> This will just produce a new line
			std::string timestamp;
			if (*message != '\0')
			{
				struct tm tstruct;
				char buffer[128];
				const time_t now = ::time(0);
				::localtime_s(&tstruct, &now);
				::strftime(buffer, sizeof(buffer), "%Y-%m-%d.%X", &tstruct);	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime for more information about date/time format
				timestamp = buffer;
			}

			// Get type as string
			// -> Don't show the regular "information"
			std::string typeAsString;
			if (Type::INFORMATION != type)
			{
				typeAsString = std::string(typeToString(type));
			}

			// Construct the full UTF-8 message text
			#ifdef RHI_DEBUG
				std::string fullMessage = mVerbose ? ("File \"" + std::string(file) + "\" | Line " + std::to_string(line) + " | " + timestamp + ' ' + typeAsString + message) : (timestamp + ' ' + typeAsString + message);
			#else
				std::string fullMessage = timestamp + ' ' + typeAsString + message;
			#endif
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

				{ // Write into standard output stream with font color depending on type
					const HANDLE handle = (Type::CRITICAL == type) ? ::GetStdHandle(STD_ERROR_HANDLE) : ::GetStdHandle(STD_OUTPUT_HANDLE);
					static const constexpr WORD COLOR[7] =
					{
						FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,	// Trace, also known as verbose logging = magenta
						FOREGROUND_GREEN | FOREGROUND_INTENSITY,					// Debug = green
						FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,		// Information = white = reset
						FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,	// General warning = yellow
						FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,	// Performance related warning = yellow
						FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,	// Compatibility related warning = yellow
						FOREGROUND_RED | FOREGROUND_INTENSITY						// Critical = red
					};
					if (Type::INFORMATION != type)
					{
						::SetConsoleTextAttribute(handle, COLOR[static_cast<int>(type)]);
					}
					if (Type::CRITICAL == type)
					{
						std::wcerr << utf16Line.c_str();
					}
					else
					{
						std::wcout << utf16Line.c_str();
					}
					if (Type::INFORMATION != type)
					{
						::SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);	// Reset
					}
				}

				// On Microsoft Windows, ensure the output can be seen inside the Visual Studio output window as well
				::OutputDebugStringW(utf16Line.c_str());
				if (Type::CRITICAL == type && ::IsDebuggerPresent())
				{
					requestDebugBreak = true;
				}
			}
			#elif __ANDROID__
			{
				int prio = ANDROID_LOG_DEFAULT;
				switch (type)
				{
					case Type::TRACE:
						prio = ANDROID_LOG_VERBOSE;
						break;

					case Type::DEBUG:
						prio = ANDROID_LOG_DEBUG;
						break;

					case Type::INFORMATION:
						prio = ANDROID_LOG_INFO;
						break;

					case Type::WARNING:
					case Type::PERFORMANCE_WARNING:
					case Type::COMPATIBILITY_WARNING:
						prio = ANDROID_LOG_WARN;
						break;

					case Type::CRITICAL:
						prio = ANDROID_LOG_ERROR;
						break;
				}
				__android_log_write(prio, "Unrimp", fullMessage.c_str());	// TODO(co) Might make sense to make the app-name customizable
			}
			#elif LINUX
				{ // Write into standard output stream with font color depending on type
					static const constexpr wchar_t RESET_COLOR[9] = L"\033[39m";
					static const constexpr wchar_t COLOR[7][9] =
					{
						L"\033[35m",	// Trace, also known as verbose logging = magenta
						L"\033[32m",	// Debug = green
						L"\033[39m",	// Information = white = reset
						L"\033[33m",	// General warning = yellow
						L"\033[33m",	// Performance related warning = yellow
						L"\033[33m",	// Compatibility related warning = yellow
						L"\033[31m"		// Critical = red
					};
					if (Type::CRITICAL == type)
					{
						std::cerr << COLOR[static_cast<int>(type)] << fullMessage.c_str() << RESET_COLOR;
					}
					else
					{
						std::cout << COLOR[static_cast<int>(type)] << fullMessage.c_str() << RESET_COLOR;
					}
				}
			#else
				#error "Unsupported platform"
			#endif

			#ifndef __ANDROID__
			{ // Add to log file write queue
				std::lock_guard<std::mutex> guard(mQueueMutex);
				mLogFileQueue.emplace_back(std::move(fullMessage));
			}
			#endif

			// Done
			return requestDebugBreak;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		[[nodiscard]] inline const char* typeToString(Type type) const
		{
			switch (type)
			{
				case Type::TRACE:
					return "Trace: ";

				case Type::DEBUG:
					return "Debug: ";

				case Type::INFORMATION:
					return "Information: ";

				case Type::WARNING:
					return "Warning: ";

				case Type::PERFORMANCE_WARNING:
					return "Performance warning: ";

				case Type::COMPATIBILITY_WARNING:
					return "Compatibility warning: ";

				case Type::CRITICAL:
					return "Critical: ";

				default:
					return "Unknown: ";
			}
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		std::mutex mMutex;
		#ifdef _DEBUG
			bool mVerbose = false;
		#endif
		#ifndef __ANDROID__
			std::string mAbsoluteLogDirectory;	// Get the absolute UTF-8 base directory, with "/" at the end
			std::string mPrefix;
			std::ofstream mOfstream;

			// Thread-related members
			std::thread*			 mWorkerThread = nullptr;
			bool					 mThreadShouldBeRunning = true;
			std::vector<std::string> mLogFileQueue;		// Not using an actual std::queue, because that's not needed here and std::vector makes things a hit easier
			std::mutex				 mQueueMutex;
			std::condition_variable  mConditionVariable;
		#endif


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	private:
		#ifndef __ANDROID__
			// From https://stackoverflow.com/a/61067330
			template <typename TP>
			std::time_t to_time_t(TP tp)
			{
				using namespace std::chrono;
				auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
				return system_clock::to_time_t(sctp);
			}

			inline static std::string getAbsoluteFilename(const std::string& absoluteLogDirectory, const std::string& prefix, int index)
			{
				std::string FILENAME = prefix + ((index <= 0) ? "Log.log" : "Log_" + std::to_string(index) + ".log");
				return absoluteLogDirectory + FILENAME;
			}

			inline static void StaticThreadFunction(DefaultLog* defaultLog)
			{
				defaultLog->ThreadFunction();
			}
		#endif


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit DefaultLog(const DefaultLog&) = delete;
		DefaultLog& operator=(const DefaultLog&) = delete;

		#ifndef __ANDROID__
			inline void ThreadFunction()
			{
				std::vector<std::string> linesToLog;
				while (mThreadShouldBeRunning)
				{
					{
						// Wait until there's something new in the queue
						//  -> Wake up once a second in any case, just to be on the safe side (these condition variables are not always 100% reliable, there's things like "lost wakeups")
						std::unique_lock<std::mutex> lock(mQueueMutex);
						mConditionVariable.wait_for(lock, std::chrono::milliseconds(1000));

						// We own the lock now, use it only very briefly to do a quick swap
						linesToLog.swap(mLogFileQueue);
					}

					// Now write to file
					if (!linesToLog.empty())
					{
						// Open file, if necessary
						if (!mOfstream.is_open())
						{
							// Get the absolute UTF-8 base directory, with "/" at the end
							const std::string selectedAbsoluteLogDirectory = mAbsoluteLogDirectory.empty() ? (std::filesystem::current_path() / "../LocalData/Log/").generic_string() : mAbsoluteLogDirectory;

							// Rename an already existing log file
							std::string absoluteFilename;
							{
								int index = 0;
								bool success = false;
								while (!success)
								{
									absoluteFilename = getAbsoluteFilename(selectedAbsoluteLogDirectory, mPrefix, index);
									success = true;
									if (std::filesystem::exists(absoluteFilename))
									{
										const std::time_t fileTime = to_time_t(std::filesystem::last_write_time(absoluteFilename));
										std::string fileTimeAsString;
										{
											struct tm tstruct;
											char buffer[128];
											::localtime_s(&tstruct, &fileTime);
											::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%X", &tstruct);	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime for more information about date/time format
											fileTimeAsString = buffer;
											std::replace(fileTimeAsString.begin(), fileTimeAsString.end(), ':', '-');
										}
										try
										{
											std::filesystem::rename(absoluteFilename, std::filesystem::path(absoluteFilename).replace_extension().generic_string() + '_' + fileTimeAsString + ".log");
										}
										catch (const std::exception&)
										{
											// If that failed, we should try again with a different file name
											//  -> This will happen e.g. when starting multiple instances, which is not unusual for the lobby client
											success = false;
											++index;
										}
									}
								}
							}

							// Ensure the directory exists
							std::filesystem::create_directories(std::filesystem::path(absoluteFilename).parent_path().generic_string());

							// Open log file
							mOfstream.open(absoluteFilename);
						}

						// Write to file
						for (const std::string& line : linesToLog)
						{
							mOfstream << line;
						}
						mOfstream.flush();
						linesToLog.clear();
					}
				}
			}
		#endif


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Rhi
