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

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4121)	// warning C4121: 'JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2': alignment of a member was sensitive to packing
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <SDL.h>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class IApplication;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Simple DirectMedia Layer" (SDL, https://www.libsdl.org/ ) application implementation class
*/
class ApplicationImplSdl2 final : public IApplicationImpl
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
	ApplicationImplSdl2(IApplication& application, const char* windowTitle);

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~ApplicationImplSdl2() override
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
	[[nodiscard]] virtual handle getNativeWindowHandle() const override;
	virtual void redraw() override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit ApplicationImplSdl2(const ApplicationImplSdl2& source) = delete;
	ApplicationImplSdl2& operator =(const ApplicationImplSdl2& source) = delete;
	#ifdef RENDERER_RUNTIME_IMGUI
		void initializeImGuiKeyMap();
		void processImGuiSdl2Event(const SDL_Event& sdlEvent);
		void updateImGuiMousePositionAndButtons();
	#endif


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	IApplication* mApplication;			///< The owner application instance, always valid
	char		  mWindowTitle[64];		///< ASCII window title
	SDL_Window*	  mSdlWindow;			///< SDL2 handle, can be a null handler
	bool		  mFirstUpdate;
	#ifdef RENDERER_RUNTIME_IMGUI
		bool mImGuiMousePressed[3];
	#endif


};
