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
#include "Framework/Linux/X11Application.h"
#include "Framework/Linux/X11Window.h"
#include "Framework/PlatformTypes.h"


//[-------------------------------------------------------]
//[ Public static data                                    ]
//[-------------------------------------------------------]
X11Application* X11Application::_self = nullptr;


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
X11Application::X11Application() :
	mDisplay(nullptr)
{
	XInitThreads(); // Required by Vulkan, when using XLib. (Vulkan spec 1.0.57 section: 29.2.6 Xlib Platform)
	mDisplay = XOpenDisplay(0);
	_self = this;
}

X11Application::~X11Application()
{
	XCloseDisplay(mDisplay);
}

int X11Application::run()
{
	XEvent event;

	while (!mWindows.empty())
	{
		XNextEvent(mDisplay, &event);
		HandleEvent(event);
	}

	return 0;
}

bool X11Application::handlePendingEvents()
{
	XEvent event;
	while (!mWindows.empty() && XPending(mDisplay) > 0)
	{
		XNextEvent(mDisplay, &event);
		if (HandleEvent(event))
		{
			return true;
		}
	}

	return false;
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
bool X11Application::HandleEvent(XEvent& event)
{
	std::map<WindowHandle, WindowEntry>::iterator it = mWindows.find(event.xany.window);
	return (it != mWindows.end()) ? it->second.x11Window->HandleEvent(event) : false;
}

void X11Application::AddWindowToEventLoop(X11Window& window)
{
	// Ensure that the window was not already added
	std::map<WindowHandle, WindowEntry>::iterator it = mWindows.find(window.winId());
	if (it == mWindows.end())
	{
		WindowEntry entry = { window.winId(), &window };
		mWindows.insert(std::pair<WindowHandle, WindowEntry>(window.winId(), entry));
	}
}

void X11Application::RemoveWindowFromEventLoop(const X11Window& window)
{
	std::map<WindowHandle, WindowEntry>::iterator it = mWindows.find(window.winId());
	if (it != mWindows.end())
	{
		mWindows.erase(it);
	}
}
