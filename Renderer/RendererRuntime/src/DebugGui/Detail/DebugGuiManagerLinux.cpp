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
#include "RendererRuntime/DebugGui/Detail/DebugGuiManagerLinux.h"

#include <imgui/imgui.h>

// TODO(sw) Implement a android version
#ifndef __ANDROID__
	#include <X11/Xutil.h>
#endif
#include <sys/time.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void DebugGuiManagerLinux::onWindowResize(uint32_t width, uint32_t heigth)
	{
		mWindowWidth = width;
		mWindowHeigth = heigth;
	}

	void DebugGuiManagerLinux::onKeyInput(uint32_t keySym, char character, bool pressed)
	{
		#ifndef __ANDROID__
			ImGuiIO& imGuiIo = ImGui::GetIO();
			if (keySym < 512)
			{
				imGuiIo.KeysDown[keySym] = pressed;
			}
			else if (XK_Alt_L == keySym)
			{
				imGuiIo.KeyAlt = pressed;
			}
			else if (XK_Shift_L == keySym)
			{
				imGuiIo.KeyShift = pressed;
			}
			else if (XK_Control_L == keySym)
			{
				imGuiIo.KeyCtrl = pressed;
			}
			else if (XK_Super_L == keySym)
			{
				imGuiIo.KeySuper = pressed;
			}
			else if ((keySym & 0xff00) == 0xff00)
			{
				// It is a special key (e.g. tab key) map the value to a range between 0x0ff and 0x1ff
				imGuiIo.KeysDown[(keySym & 0x1ff)] = pressed;
			}
			if (pressed && character > 0)
			{
				imGuiIo.AddInputCharacter(character);
			}
		#endif
	}

	void DebugGuiManagerLinux::onMouseMoveInput(int x, int y)
	{
		float windowWidth  = 1.0f;
		float windowHeight = 1.0f;
		{
			// Ensure that none of them is ever zero
			if (mWindowWidth >= 1)
			{
				windowWidth = static_cast<float>(mWindowWidth);
			}
			if (mWindowHeigth >= 1)
			{
				windowHeight = static_cast<float>(mWindowHeigth);
			}
		}

		{
			ImGuiIO& imGuiIo = ImGui::GetIO();
			imGuiIo.MousePos.x = static_cast<float>(x) * (imGuiIo.DisplaySize.x / windowWidth);
			imGuiIo.MousePos.y = static_cast<float>(y) * (imGuiIo.DisplaySize.y / windowHeight);
		}
	}

	void DebugGuiManagerLinux::onMouseButtonInput(uint32_t button, bool pressed)
	{
		// The mouse buttons on X11 starts at index 1 for the left mouse button. In ImGui the left mouse button is at index 0. Compensate it.
		if (button > 0 && button <= 5)
		{
			ImGui::GetIO().MouseDown[button - 1] = pressed;
		}
	}

	void DebugGuiManagerLinux::onMouseWheelInput(bool scrollUp)
	{
		ImGui::GetIO().MouseWheel += scrollUp ? -1.0f : 1.0f;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::DebugGuiManager methods ]
	//[-------------------------------------------------------]
	void DebugGuiManagerLinux::initializeImGuiKeyMap()
	{
		// Keyboard mapping: ImGui will use those indices to peek into the "ImGuiIO::KeyDown[]" array that we will update during the application lifetime
		#ifndef __ANDROID__
			ImGuiIO& imGuiIo = ImGui::GetIO();
			// TODO(sw) These keysyms are 16bit values with an value > 512. We map them to a range between 0x0ff and 0x1ff
			imGuiIo.KeyMap[ImGuiKey_Tab]		= (XK_Tab & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_LeftArrow]	= (XK_Left & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_RightArrow]	= (XK_Right & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_UpArrow]	= (XK_Up & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_DownArrow]	= (XK_Down & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_PageUp]		= (XK_Page_Up & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_PageDown]	= (XK_Page_Down & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Home]		= (XK_Home & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_End]		= (XK_End & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Insert]		= (XK_Insert & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Delete]		= (XK_Delete & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Backspace]	= (XK_BackSpace & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Space]		= (XK_Space & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Enter]		= (XK_Return & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_Escape]		= (XK_Escape & 0x1ff);
			imGuiIo.KeyMap[ImGuiKey_A]			= XK_a;
			imGuiIo.KeyMap[ImGuiKey_C]			= XK_c;
			imGuiIo.KeyMap[ImGuiKey_V]			= XK_v;
			imGuiIo.KeyMap[ImGuiKey_X]			= XK_x;
			imGuiIo.KeyMap[ImGuiKey_Y]			= XK_y;
			imGuiIo.KeyMap[ImGuiKey_Z]			= XK_z;
		#endif
	}

	void DebugGuiManagerLinux::onNewFrame(Renderer::IRenderTarget& renderTarget)
	{
		ImGuiIO& imGuiIo = ImGui::GetIO();

		{ // Setup display size (every frame to accommodate for render target resizing)
			uint32_t width = 0;
			uint32_t height = 0;
			renderTarget.getWidthAndHeight(width, height);
			imGuiIo.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
		}
		
		{ // Setup time step
			timeval currentTimeValue;
			gettimeofday(&currentTimeValue, nullptr);
			const uint64_t currentTime = currentTimeValue.tv_sec * 1000000 + currentTimeValue.tv_usec;
			imGuiIo.DeltaTime = static_cast<float>(currentTime - mTime) / 1000000.0f;
			mTime = currentTime;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
