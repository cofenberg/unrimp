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
#include "Examples/Private/Framework/IApplicationImpl.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class X11Window;
class IApplication;
class X11Application;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Linux application implementation class
*/
class ApplicationImplLinux final : public IApplicationImpl
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] application
	*    The owner application instance
	*  @param[in] windowTitle
	*    ASCII window title, can be a null pointer
	*/
	ApplicationImplLinux(IApplication& application, const char* windowTitle);

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~ApplicationImplLinux() override
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Public virtual IApplicationImpl methods               ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual bool processMessages() override;
	virtual void getWindowSize(int& width, int& height) const override;
	[[nodiscard]] virtual handle getNativeWindowHandle() const override;
	virtual void redraw() override;
	[[nodiscard]] virtual Display* getX11Display() const override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit ApplicationImplLinux(const ApplicationImplLinux& source) = delete;
	ApplicationImplLinux& operator =(const ApplicationImplLinux& source) = delete;


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	IApplication*   mApplication;		///< The owner application instance, always valid
	char			mWindowTitle[64];	///< ASCII window title
	X11Application* mX11EventLoop;		///< X11 event loop application, can be a null pointer, destroy the instance in case you no longer need it
	X11Window*	    mMainWindow;		///< X11 window, can be a null pointer, destroy the instance in case you no longer need it


};
