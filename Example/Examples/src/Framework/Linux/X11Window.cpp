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
#include "Framework/Linux/X11Window.h"
#include "Framework/Linux/X11Application.h"

#include <cstring>


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
X11Window::X11Window() :
	mDestroyed(false)
{
	Display* display = X11Application::instance()->getDisplay();

	WM_DELETE_WINDOW	 = XInternAtom(display, "WM_DELETE_WINDOW",		True);
	UTF8_STRING			 = XInternAtom(display, "UTF8_STRING",			False);
	WM_NAME				 = XInternAtom(display, "WM_NAME",				False);
	_NET_WM_NAME		 = XInternAtom(display, "_NET_WM_NAME",			False);
	_NET_WM_VISIBLE_NAME = XInternAtom(display, "_NET_WM_VISIBLE_NAME",	False);

	const uint32_t  width  = 640;
	const uint32_t  height = 480;
	const int       screen = DefaultScreen(display);
	Visual*         visual = DefaultVisual(display, screen);
	const int       depth  = DefaultDepth(display, screen);

	// Create the native OS window instance with a black background (else we will see trash if nothing has been drawn, yet)
	XSetWindowAttributes sXSetWindowAttributes;
	sXSetWindowAttributes.background_pixel = 0;
	sXSetWindowAttributes.event_mask = ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask | VisibilityChangeMask | KeyReleaseMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	mWindowId = XCreateWindow(display, XRootWindow(display, screen), 0, 0, width, height, 0, depth, InputOutput, visual, CWBackPixel | CWEventMask, &sXSetWindowAttributes);
	XSetWMProtocols(display, mWindowId, &WM_DELETE_WINDOW, 1);

	X11Application::instance()->AddWindowToEventLoop(*this);
}

X11Window::~X11Window()
{
	if (mWindowId)
	{
		XDestroyWindow(X11Application::instance()->getDisplay(), mWindowId);
	}
}

bool X11Window::HandleEvent(XEvent &event)
{
	// Process message
	switch (event.type)
	{
		case DestroyNotify:
			// Mark window destroyed
			mDestroyed = true;
			X11Application::instance()->RemoveWindowFromEventLoop(*this);
			mWindowId = 0;
			break;

		// Window configuration changed
		case ConfigureNotify:
			// TODO(co) Anything to do in here?
			//mX11Application->onResize();
			break;

		case ClientMessage:
			// When the "WM_DELETE_WINDOW" client message is send, no "DestroyNotify"-message is generated because the
			// application itself should destroy/close the window to which the "WM_DELETE_WINDOW" client message was send to.
			// In this case, we will leave the event loop after this message was processed and no other messages are in the queue.
			// -> No "DestroyNotify"-message can be received
			if (WM_DELETE_WINDOW == static_cast<Atom>(event.xclient.data.l[0]))
			{
				XDestroyWindow(event.xany.display, mWindowId);
				mWindowId = 0;
				mDestroyed = true;
			}
			break;
	}
	return false;
}

void X11Window::setTitle(const char* title)
{
	Display* display = X11Application::instance()->getDisplay();
	const int numberOfElements = std::strlen(title);
	const unsigned char* windowTitle = reinterpret_cast<const unsigned char*>(title);
	XChangeProperty(display, mWindowId, WM_NAME,				UTF8_STRING, 8, PropModeReplace, windowTitle, numberOfElements);
	XChangeProperty(display, mWindowId, _NET_WM_NAME,		 	UTF8_STRING, 8, PropModeReplace, windowTitle, numberOfElements);
	XChangeProperty(display, mWindowId, _NET_WM_VISIBLE_NAME,	UTF8_STRING, 8, PropModeReplace, windowTitle, numberOfElements);
}

void X11Window::show()
{
	XMapWindow(X11Application::instance()->getDisplay(), mWindowId);
}

void X11Window::getWindowSize(int& width, int& height) const
{
	// Is there a valid OS window?
	if (mWindowId)
	{
		// Get X window geometry information
		::Window rootWindow = 0;
		int positionX = 0, positionY = 0;
		unsigned int unsignedWidth = 0, unsignedHeight = 0, border = 0, depth = 0;
		XGetGeometry(X11Application::instance()->getDisplay(), mWindowId, &rootWindow, &positionX, &positionY, &unsignedWidth, &unsignedHeight, &border, &depth);
		width = unsignedWidth;
		height = unsignedHeight;
	}
	else
	{
		// There's no valid OS window, return known values
		width  = 0;
		height = 0;
	}
}

void X11Window::refresh()
{
	// Is there a valid OS window?
	if (mWindowId && !mDestroyed)
	{
		// Send expose event
		XEvent sEvent;
		sEvent.type			 = Expose;
		sEvent.xany.window	 = mWindowId;
		sEvent.xexpose.count = 0;
		XSendEvent(X11Application::instance()->getDisplay(), mWindowId, False, 0, &sEvent);

		// Do it!
		XSync(X11Application::instance()->getDisplay(), False);
	}
}
