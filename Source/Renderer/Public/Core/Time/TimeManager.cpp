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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Core/Time/TimeManager.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void TimeManager::update()
	{
		// Stop the stopwatch and get the past milliseconds
		mStopwatch.stop();
		mPastSecondsSinceLastFrame = mStopwatch.getSeconds();
		if (mPastSecondsSinceLastFrame <= 0.0f)
		{
			// Don't allow zero or time advancing enforcement asserts will get more complicated
			mPastSecondsSinceLastFrame = std::numeric_limits<float>::min();
		}
		else if (mPastSecondsSinceLastFrame > 0.06f)
		{
			// No one likes huge time jumps
			mPastSecondsSinceLastFrame = 0.06f;
		}
		mGlobalTimeInSeconds += mPastSecondsSinceLastFrame;	// TODO(co) Add some kind of wrapping to avoid that issues with a too huge global time can come up?
		++mNumberOfRenderedFrames;

		// Calculate frames per second
		// -> Using the approach from ImGui 1.78 WIP (latest commit 76ddacd2a12f713a218116c849928ef2274d3f8b - July 29, 2020)
		mFramerateSecondsPerFrameAccumulated += mPastSecondsSinceLastFrame - mFramerateSecondsPerFrame[mFramerateSecondsPerFrameIndex];
		mFramerateSecondsPerFrame[mFramerateSecondsPerFrameIndex] = mPastSecondsSinceLastFrame;
		mFramerateSecondsPerFrameIndex = (mFramerateSecondsPerFrameIndex + 1) % static_cast<int>(GLM_COUNTOF(mFramerateSecondsPerFrame));
		mFramesPerSecond = (mFramerateSecondsPerFrameAccumulated > 0.0f) ? (1.0f / (mFramerateSecondsPerFrameAccumulated / static_cast<float>(GLM_COUNTOF(mFramerateSecondsPerFrame)))) : std::numeric_limits<float>::max();

		// Start the stopwatch
		mStopwatch.start();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
