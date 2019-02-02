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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Framework/ApplicationImplWindows.h"
#include "Examples/Private/Framework/IApplication.h"

#ifdef RENDERER_RUNTIME_IMGUI
	#include <RendererRuntime/Public/DebugGui/Detail/DebugGuiManagerWindows.h>
#endif

#include <iostream>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		// The "setProcessDpiAware()"-method was taken from SFML ( http://www.sfml-dev.org/ ) - https://github.com/SFML/SFML/blob/master/src/SFML/Window/Win32/WindowImplWin32.cpp
		void setProcessDpiAware()
		{
			{ // Try "SetProcessDpiAwareness()" first
				const HINSTANCE shCoreDll = ::LoadLibrary(L"Shcore.dll");
				if (nullptr != shCoreDll)
				{
					enum ProcessDpiAwareness
					{
						ProcessDpiUnaware         = 0,
						ProcessSystemDpiAware     = 1,
						ProcessPerMonitorDpiAware = 2
					};

					typedef HRESULT (WINAPI* SetProcessDpiAwarenessFuncType)(ProcessDpiAwareness);
					PRAGMA_WARNING_PUSH
						PRAGMA_WARNING_DISABLE_MSVC(4191)	// warning C4191: 'reinterpret_cast': unsafe conversion from 'FARPROC' to 'SetProcessDpiAwarenessFuncType'
						SetProcessDpiAwarenessFuncType SetProcessDpiAwarenessFunc = reinterpret_cast<SetProcessDpiAwarenessFuncType>(::GetProcAddress(shCoreDll, "SetProcessDpiAwareness"));
					PRAGMA_WARNING_POP
					if (nullptr != SetProcessDpiAwarenessFunc)
					{
						// We only check for "E_INVALIDARG" because we would get "E_ACCESSDENIED" if the DPI was already set previously and "S_OK" means the call was successful
						if (SetProcessDpiAwarenessFunc(ProcessPerMonitorDpiAware) == E_INVALIDARG)
						{
							// TODO(co) Logging?
						}
						else
						{
							::FreeLibrary(shCoreDll);
							return;
						}
					}

					::FreeLibrary(shCoreDll);
				}
			}

			{ // Fall back to "SetProcessDPIAware()" if "SetProcessDpiAwareness()" is not available on this system
				const HINSTANCE user32Dll = ::LoadLibrary(L"user32.dll");
				if (nullptr != user32Dll)
				{
					typedef BOOL (WINAPI* SetProcessDPIAwareFuncType)(void);
					PRAGMA_WARNING_PUSH
						PRAGMA_WARNING_DISABLE_MSVC(4191)	// warning C4191: 'reinterpret_cast': unsafe conversion from 'FARPROC' to 'SetProcessDPIAwareFuncType'
						SetProcessDPIAwareFuncType SetProcessDPIAwareFunc = reinterpret_cast<SetProcessDPIAwareFuncType>(::GetProcAddress(user32Dll, "SetProcessDPIAware"));
					PRAGMA_WARNING_POP
					if (nullptr != SetProcessDPIAwareFunc && !SetProcessDPIAwareFunc())
					{
						// TODO(co) Logging?
					}

					::FreeLibrary(user32Dll);
				}
			}
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
ApplicationImplWindows::ApplicationImplWindows(IApplication& application, const char* windowTitle) :
	IApplicationImpl(application),
	mApplication(&application),
	mNativeWindowHandle(NULL_HANDLE),
	mFirstUpdate(true)
{
	// Copy the given window title
	if (nullptr != windowTitle)
	{
		strncpy(mWindowTitle, windowTitle, 64);
	}
	else
	{
		mWindowTitle[0] = '\0';
	}

	// Set that this process is DPI aware and can handle DPI scaling
	::detail::setProcessDpiAware();
}


//[-------------------------------------------------------]
//[ Public virtual IApplicationImpl methods               ]
//[-------------------------------------------------------]
void ApplicationImplWindows::onInitialization()
{
	{ // Setup and register the window class for the OpenGL dummy window
		WNDCLASS windowDummyClass;
		windowDummyClass.hInstance		= ::GetModuleHandle(nullptr);
		windowDummyClass.lpszClassName	= TEXT("ApplicationImplWindows");
		windowDummyClass.lpfnWndProc	= wndProc;
		windowDummyClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowDummyClass.hIcon			= nullptr;
		windowDummyClass.hCursor		= ::LoadCursor(nullptr, IDC_ARROW);
		windowDummyClass.lpszMenuName	= nullptr;
		windowDummyClass.cbClsExtra		= 0;
		windowDummyClass.cbWndExtra		= 0;
		windowDummyClass.hbrBackground	= nullptr;
		::RegisterClass(&windowDummyClass);
	}

	// Create the OS window instance
	mNativeWindowHandle = ::CreateWindowA("ApplicationImplWindows", mWindowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, ::GetModuleHandle(nullptr), this);
}

void ApplicationImplWindows::onDeinitialization()
{
	// Destroy the OS window instance, in case there's one
	if (NULL_HANDLE != mNativeWindowHandle)
	{
		// Destroy the OpenGL dummy window
		::DestroyWindow(mNativeWindowHandle);
		mNativeWindowHandle = NULL_HANDLE;
	}

	// Unregister the window class for the OpenGL dummy window
	::UnregisterClass(TEXT("ApplicationImplWindows"), ::GetModuleHandle(nullptr));

	// Flush messages
	[[maybe_unused]] const bool quit = processMessages();
}

bool ApplicationImplWindows::processMessages()
{
	// The window is made visible before the first processing of operation system messages, this way the concrete example has the
	// opportunity to e.g. restore the window position and size from a previous season without having a visible jumping window
	if (mFirstUpdate)
	{
		if (NULL_HANDLE != mNativeWindowHandle)
		{
			// Show the created OS window
			::ShowWindow(mNativeWindowHandle, SW_SHOWDEFAULT);
			::UpdateWindow(mNativeWindowHandle);
		}
		mFirstUpdate = false;
	}

	// By default, do not shut down the application
	bool quit = false;

	// Look if messages are waiting (non-blocking)
	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
	{
		// Get the waiting message
		::GetMessage(&msg, nullptr, 0, 0);
		if (WM_QUIT == msg.message)
		{
			// Shut down the application
			quit = true;
		}

		// Process message
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	// Done, tell the caller whether or not to shut down the application
	return quit;
}

void ApplicationImplWindows::getWindowSize(int& width, int& height) const
{
	// Is there a valid OS window?
	if (NULL_HANDLE != mNativeWindowHandle)
	{
		RECT rect;
		::GetClientRect(mNativeWindowHandle, &rect);
		width  = rect.right  - rect.left;
		height = rect.bottom - rect.top;
	}
	else
	{
		// There's no valid OS window, return known values
		width  = 0;
		height = 0;
	}
}

void ApplicationImplWindows::redraw()
{
	// Is there a valid OS window?
	if (NULL_HANDLE != mNativeWindowHandle)
	{
		// Redraw window
		::RedrawWindow(mNativeWindowHandle, nullptr, nullptr, RDW_INVALIDATE);
	}
}

void ApplicationImplWindows::showUrgentMessage(const char* message, const char* title) const
{
	// Define a helper macro: Convert UTF-8 string to UTF-16
	#define UTF8_TO_UTF16(utf8String, utf16String) \
		std::wstring utf16String; \
		utf16String.resize(static_cast<std::wstring::size_type>(::MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, nullptr , 0))); \
		::MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, utf16String.data(), static_cast<int>(utf16String.size()));

	// MS Windows message box
	UTF8_TO_UTF16(message, utf16Message)
	UTF8_TO_UTF16(title, utf16title)
	::MessageBoxW(nullptr, utf16Message.c_str(), utf16title.c_str(), MB_OK | MB_ICONERROR);

	// Do also feed the output stream
	std::wcout << utf16Message;

	// Undefine the helper macro
	#undef UTF8_TO_UTF16
}


//[-------------------------------------------------------]
//[ Private static Microsoft Windows callback function    ]
//[-------------------------------------------------------]
LRESULT CALLBACK ApplicationImplWindows::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Get pointer to application implementation
	ApplicationImplWindows* applicationImplWindows = nullptr;
	if (WM_CREATE == message)
	{
		applicationImplWindows = static_cast<ApplicationImplWindows*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
	}
	else if (NULL_HANDLE != hWnd)
	{
		applicationImplWindows = reinterpret_cast<ApplicationImplWindows*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	// Call the Microsoft Windows callback of the debug GUI
	#ifdef RENDERER_RUNTIME_IMGUI
		RendererRuntime::DebugGuiManagerWindows::wndProc(hWnd, message, wParam, lParam);
	#endif

	// Evaluate message
	switch (message)
	{
		// Initialize window
		case WM_CREATE:
			// Set window pointer and handle (SetWindowLongPtr is the 64bit equivalent to SetWindowLong)
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(applicationImplWindows));
			return 0;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			// Inform the owner application instance
			applicationImplWindows->mApplication->onResize();
			return 0;

		case WM_SYSKEYDOWN:
			// Toggle fullscreen right now? (Alt-Return)
			if (VK_RETURN == wParam && (lParam & (1 << 29)))	// Bit 29 = the ALT-key
			{
				// Inform the owner application instance
				applicationImplWindows->mApplication->onToggleFullscreenState();
			}
			return 0;

		case WM_KEYDOWN:
			if (27 == wParam)
			{
				applicationImplWindows->mApplication->onEscapeKey();
			}
			return 0;

		// Window paint request
		case WM_PAINT:
		{
			// Begin paint
			PAINTSTRUCT paintStruct;
			::BeginPaint(hWnd, &paintStruct);

			// Redraw, but only if the draw area isn't null
			if (!::IsRectEmpty(&paintStruct.rcPaint))
			{
				// Inform the owner application instance
				applicationImplWindows->mApplication->onDrawRequest();
			}

			// End paint
			::EndPaint(hWnd, &paintStruct);
			return 0;
		}

		// Let the OS handle this message
		default:
			return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
}
