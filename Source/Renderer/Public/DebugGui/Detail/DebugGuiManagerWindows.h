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
#include "Renderer/Public/DebugGui/DebugGuiManager.h"
#include "Renderer/Public/Core/Platform/WindowsHeader.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class DebugGuiManagerWindows final : public DebugGuiManager
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererImpl;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Microsoft Windows callback method to be called from the outside for input handling
		*/
		RENDERER_API_EXPORT static LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::DebugGuiManager methods   ]
	//[-------------------------------------------------------]
	protected:
		virtual void initializeImGuiKeyMap() override;
		virtual void startup() override;
		virtual void onNewFrame(Rhi::IRenderTarget& renderTarget) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit DebugGuiManagerWindows(IRenderer& renderer) :
			DebugGuiManager(renderer),
			mTime(0),
			mTicksPerSecond(0)
		{
			// Nothing here
		}

		inline virtual ~DebugGuiManagerWindows() override
		{
			// Nothing here
		}

		explicit DebugGuiManagerWindows(const DebugGuiManagerWindows&) = delete;
		DebugGuiManagerWindows& operator=(const DebugGuiManagerWindows&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		INT64 mTime;
		INT64 mTicksPerSecond;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
