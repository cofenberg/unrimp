/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    X11 window
*
*  @todo
*    - TODO(co) Code style cleanup and documentation
*/
class X11Window final
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	X11Window();
	virtual ~X11Window();
	virtual bool HandleEvent(XEvent& event);
	void setTitle(const char* title);
	void show();
	void getWindowSize(int& width, int& height) const;
	void refresh();
	[[nodiscard]] Window winId() const { return mWindowId; }
	[[nodiscard]] bool isDestroyed() const { return mDestroyed; }


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	inline X11Window(const X11Window& other)
	{
		// Nothing here
	}

	inline virtual X11Window& operator=(const X11Window& other)
	{
		return *this;
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	bool   mDestroyed;
	Window mWindowId;
	::Atom WM_DELETE_WINDOW;		///< System atom for delete
	::Atom UTF8_STRING;				///< Atom for the type of a window title
	::Atom WM_NAME;					///< Window title (old?)
	::Atom _NET_WM_NAME;			///< Window title
	::Atom _NET_WM_VISIBLE_NAME;	///< Window title (visible title, can be different)


};
