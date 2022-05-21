/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Renderer/Public/Core/Manager.h"
#include "Renderer/Public/Core/Time/Stopwatch.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Time manager
	*/
	class TimeManager final : public Manager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		TimeManager();
		inline ~TimeManager()
		{
			// Nothing here
		}

		[[nodiscard]] inline const Stopwatch& getSinceStartStopwatch() const
		{
			return mSinceStartStopwatch;
		}

		[[nodiscard]] inline float getPastSecondsSinceLastFrame() const
		{
			return mPastSecondsSinceLastFrame;
		}

		[[nodiscard]] inline float getGlobalTimeInSeconds() const
		{
			return mGlobalTimeInSeconds;
		}

		[[nodiscard]] inline uint64_t getNumberOfRenderedFrames() const
		{
			return mNumberOfRenderedFrames;
		}

		[[nodiscard]] inline float getFramesPerSecond() const
		{
			return mFramesPerSecond;
		}

		/**
		*  @brief
		*    Time manager update
		*
		*  @note
		*    - Call this once per frame
		*/
		void update();


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit TimeManager(const TimeManager&) = delete;
		TimeManager& operator=(const TimeManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Stopwatch mSinceStartStopwatch;
		Stopwatch mPerUpdateStopwatch;
		float	  mPastSecondsSinceLastFrame;
		float	  mGlobalTimeInSeconds;
		uint64_t  mNumberOfRenderedFrames;
		float	  mFramesPerSecond;
		float	  mFramerateSecondsPerFrame[120];	// Calculate estimate of framerate over the last two seconds
		int		  mFramerateSecondsPerFrameIndex;
		float	  mFramerateSecondsPerFrameAccumulated;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
