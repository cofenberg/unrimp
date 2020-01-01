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
#include "Examples/Private/Framework/IApplicationImpl.h"

#include <Renderer/Public/Core/Platform/WindowsHeader.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class IApplication;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Windows application implementation class
*/
class ApplicationImplWindows final : public IApplicationImpl
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
	ApplicationImplWindows(IApplication& application, const char* windowTitle);

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~ApplicationImplWindows() override
	{
		// Nothing here
		// mNativeWindowHandle is destroyed within onDeinitialization()
	}


//[-------------------------------------------------------]
//[ Public virtual IApplicationImpl methods               ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	[[nodiscard]] virtual bool processMessages() override;
	virtual void getWindowSize(int &width, int &height) const override;

	[[nodiscard]] inline virtual handle getNativeWindowHandle() const override
	{
		return reinterpret_cast<handle>(mNativeWindowHandle);
	}

	virtual void redraw() override;
	virtual void showUrgentMessage(const char* message, const char* title = "Urgent Message") const override;


//[-------------------------------------------------------]
//[ Private static Microsoft Windows callback function    ]
//[-------------------------------------------------------]
private:
	[[nodiscard]] static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit ApplicationImplWindows(const ApplicationImplWindows& source) = delete;
	ApplicationImplWindows& operator =(const ApplicationImplWindows& source) = delete;


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	IApplication* mApplication;			///< The owner application instance, always valid
	char		  mWindowTitle[64];		///< ASCII window title
	HWND		  mNativeWindowHandle;	///< OS window handle, can be a null handler
	bool		  mFirstUpdate;


};
