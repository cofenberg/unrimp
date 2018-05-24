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
#include "Framework/IApplication.h"
#ifdef WIN32
	#include "Framework/ApplicationImplWindows.h"
#elif defined LINUX
	#include "Framework/ApplicationImplLinux.h"
#endif


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
IApplication::~IApplication()
{
	// Destroy the application instance
	delete mApplicationImpl;
}

int IApplication::run()
{
	// Call application implementation initialization method
	mApplicationImpl->onInitialization();
	onInitialization();

	// Main loop - Process OS messages (non-blocking) first
	while (!mExit && !mApplicationImpl->processMessages())
	{
		// Update the application logic
		onUpdate();

		// Redraw request
		redraw();
	}

	// Call application implementation de-initialization method
	onDeinitialization();
	mApplicationImpl->onDeinitialization();

	// Done, no error
	return 0;
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void IApplication::onDeinitialization()
{
	// Added the call to the implementation here so that a derived class can do the base deinit at a stage which fits better
	mApplicationImpl->onDeinitialization();
}


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
IApplication::IApplication(const char* windowTitle) :
	mApplicationImpl(nullptr),
	mExit(false)
{
	// We're using "this" in here, so we are not allowed to write the following within the initializer list
	#ifdef WIN32
		mApplicationImpl = new ApplicationImplWindows(*this, windowTitle);
	#elif defined LINUX
		mApplicationImpl = new ApplicationImplLinux(*this, windowTitle);
	#else
		#error "Unsupported platform"
	#endif
}
