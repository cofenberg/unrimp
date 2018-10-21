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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <X11/Xlib.h>

#include <map>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class X11Window;
typedef Window WindowHandle;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    X11 application
*
*  @todo
*    - TODO(co) Code style cleanup and documentation
*/
class X11Application final
{


//[-------------------------------------------------------]
//[ Friends                                               ]
//[-------------------------------------------------------]
	friend class X11Window;


//[-------------------------------------------------------]
//[ Public static methods                                 ]
//[-------------------------------------------------------]
public:
	static X11Application* instance() {return _self;}


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	X11Application();
	~X11Application();
	int run();
	bool handlePendingEvents();
	[[nodiscard]] Display* getDisplay() { return mDisplay; }


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	bool HandleEvent(XEvent& event);
	void AddWindowToEventLoop(X11Window& window);
	void RemoveWindowFromEventLoop(const X11Window& window);

	inline X11Application(const X11Application& other) :
		mDisplay(nullptr)
	{
		// Nothing here
	}

	inline X11Application& operator=(const X11Application& other)
	{
		return *this;
	}


//[-------------------------------------------------------]
//[ Public static data                                    ]
//[-------------------------------------------------------]
private:
	static X11Application* _self;


//[-------------------------------------------------------]
//[ Private definitions                                   ]
//[-------------------------------------------------------]
private:
	struct WindowEntry final
	{
		WindowHandle window;
		X11Window* x11Window;
	};


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Display*							mDisplay;
	std::map<WindowHandle, WindowEntry>	mWindows;


};
