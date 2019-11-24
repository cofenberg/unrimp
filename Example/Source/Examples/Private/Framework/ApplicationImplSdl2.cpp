/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "Examples/Private/Framework/ApplicationImplSdl2.h"
#include "Examples/Private/Framework/IApplication.h"
#include "Examples/Private/Framework/IApplicationRenderer.h"

#ifdef RENDERER_IMGUI
	#include <imgui/imgui.h>
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <SDL_syswm.h>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
ApplicationImplSdl2::ApplicationImplSdl2(IApplication& application, const char* windowTitle) :
	IApplicationImpl(application),
	mApplication(&application),
	mSdlWindow(nullptr),
	mFirstUpdate(true)
	#ifdef RENDERER_IMGUI
		, mImGuiMousePressed{ false, false, false }
	#endif
{
	// Copy the given window title
	if (nullptr != windowTitle)
	{
		strncpy(mWindowTitle, windowTitle, 64);
	}
	else
	{
		mWindowTitle[0] = '\0';
	}
}


//[-------------------------------------------------------]
//[ Public virtual IApplicationImpl methods               ]
//[-------------------------------------------------------]
void ApplicationImplSdl2::onInitialization()
{
	if (SDL_Init(0) == 0)
	{
		mSdlWindow = SDL_CreateWindow(mWindowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	}
}

void ApplicationImplSdl2::onDeinitialization()
{
	if (nullptr != mSdlWindow)
	{
		SDL_DestroyWindow(mSdlWindow);
		mSdlWindow = nullptr;
	}
	// SDL_Quit(); TODO(co) SDL doesn't like "SDL_Quit()" -> "SDL_Init()", after this "SDL_PollEvent()" no longer gets any events
}

bool ApplicationImplSdl2::processMessages()
{
	// The window is made visible before the first processing of operation system messages, this way the concrete example has the
	// opportunity to e.g. restore the window position and size from a previous season without having a visible jumping window
	if (mFirstUpdate)
	{
		if (nullptr != mSdlWindow)
		{
			// Show the created SDL2 window
			SDL_ShowWindow(mSdlWindow);
		}
		#ifdef RENDERER_IMGUI
			initializeImGuiKeyMap();
		#endif
		mFirstUpdate = false;
	}

	// By default, do not shut down the application
	bool quit = false;

	// Look if messages are waiting (non-blocking)
	SDL_Event sdlEvent;
	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		switch (sdlEvent.type)
		{
			case SDL_QUIT:
				// Shut down the application
				quit = true;
				break;

			case SDL_WINDOWEVENT:
				switch (sdlEvent.window.event)
				{
					case SDL_WINDOWEVENT_EXPOSED:
						mApplication->onDrawRequest();
						break;

					case SDL_WINDOWEVENT_SIZE_CHANGED:
						mApplication->onResize();
						break;
				}
				break;

			case SDL_KEYDOWN:
				if (!sdlEvent.key.repeat && SDLK_RETURN == sdlEvent.key.keysym.sym && (SDL_GetModState() & KMOD_ALT))
				{
					mApplication->onToggleFullscreenState();
				}
				else if (!sdlEvent.key.repeat && SDLK_ESCAPE == sdlEvent.key.keysym.sym)
				{
					mApplication->onEscapeKey();
				}
				break;
		}
		#ifdef RENDERER_IMGUI
			if (ImGui::GetCurrentContext() != nullptr)
			{
				processImGuiSdl2Event(sdlEvent);
			}
		#endif
	}
	#ifdef RENDERER_IMGUI
		if (ImGui::GetCurrentContext() != nullptr)
		{
			updateImGuiMousePositionAndButtons();
		}
	#endif

	// Done, tell the caller whether or not to shut down the application
	return quit;
}

void ApplicationImplSdl2::getWindowSize(int& width, int& height) const
{
	// Is there a valid SDL2 window?
	if (nullptr != mSdlWindow)
	{
		SDL_GL_GetDrawableSize(mSdlWindow, &width, &height);
	}
	else
	{
		// There's no valid SDL2 window, return known values
		width  = 0;
		height = 0;
	}
}

handle ApplicationImplSdl2::getNativeWindowHandle() const
{
	handle nativeWindowHandle = NULL_HANDLE;
	if (nullptr != mSdlWindow)
	{
		SDL_SysWMinfo sdlSysWMinfo;
		SDL_VERSION(&sdlSysWMinfo.version);
		if (SDL_GetWindowWMInfo(mSdlWindow, &sdlSysWMinfo))
		{
			switch (sdlSysWMinfo.subsystem)
			{
				#if defined _WIN32
					case SDL_SYSWM_UNKNOWN:
						assert(false);
						break;

					case SDL_SYSWM_WINDOWS:
						nativeWindowHandle = reinterpret_cast<handle>(sdlSysWMinfo.info.win.window);
						break;

					case SDL_SYSWM_X11:
					case SDL_SYSWM_DIRECTFB:
					case SDL_SYSWM_COCOA:
					case SDL_SYSWM_UIKIT:
					case SDL_SYSWM_WAYLAND:
					case SDL_SYSWM_MIR:
					case SDL_SYSWM_WINRT:
					case SDL_SYSWM_ANDROID:
					case SDL_SYSWM_VIVANTE:
					case SDL_SYSWM_OS2:
						assert(false);
						break;
				#elif defined __ANDROID__
					#warning TODO(co) The Android support is work-in-progress
				#elif defined LINUX
					case SDL_SYSWM_UNKNOWN:
					case SDL_SYSWM_WINDOWS:
						assert(false);
						break;

					case SDL_SYSWM_X11:
						nativeWindowHandle = reinterpret_cast<handle>(sdlSysWMinfo.info.x11.window);
						break;

					case SDL_SYSWM_DIRECTFB:
					case SDL_SYSWM_COCOA:
					case SDL_SYSWM_UIKIT:
						assert(false);
						break;

					case SDL_SYSWM_WAYLAND:
						nativeWindowHandle = reinterpret_cast<handle>(sdlSysWMinfo.info.wl.surface);
						break;

					case SDL_SYSWM_MIR:
					case SDL_SYSWM_WINRT:
					case SDL_SYSWM_ANDROID:
					case SDL_SYSWM_VIVANTE:
					case SDL_SYSWM_OS2:
						assert(false);
						break;
				#else
					#error "Unsupported platform"
				#endif
			}
		}
	}

	// Done
	return nativeWindowHandle;
}

void ApplicationImplSdl2::redraw()
{
	mApplication->onDrawRequest();
}

void ApplicationImplSdl2::showUrgentMessage(const char* message, const char* title) const
{
	static const constexpr SDL_MessageBoxButtonData sdlMessageBoxButtonData[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK" }
	};
	const SDL_MessageBoxData sdlMessageBoxData = { SDL_MESSAGEBOX_ERROR, nullptr, title, message, SDL_arraysize(sdlMessageBoxButtonData), sdlMessageBoxButtonData, nullptr };
	int buttonid = 0;
	SDL_ShowMessageBox(&sdlMessageBoxData, &buttonid);
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
// Basing on https://github.com/ocornut/imgui/blob/master/examples/imgui_impl_sdl.cpp
#ifdef RENDERER_IMGUI
	void ApplicationImplSdl2::initializeImGuiKeyMap()
	{
		// Keyboard mapping: ImGui will use those indices to peek into the "ImGuiIO::KeyDown[]" array that we will update during the application lifetime
		ImGuiIO& imGuiIo = ImGui::GetIO();
		imGuiIo.KeyMap[ImGuiKey_Tab]		= SDL_SCANCODE_TAB;
		imGuiIo.KeyMap[ImGuiKey_LeftArrow]	= SDL_SCANCODE_LEFT;
		imGuiIo.KeyMap[ImGuiKey_RightArrow]	= SDL_SCANCODE_RIGHT;
		imGuiIo.KeyMap[ImGuiKey_UpArrow]	= SDL_SCANCODE_UP;
		imGuiIo.KeyMap[ImGuiKey_DownArrow]	= SDL_SCANCODE_DOWN;
		imGuiIo.KeyMap[ImGuiKey_PageUp]		= SDL_SCANCODE_PAGEUP;
		imGuiIo.KeyMap[ImGuiKey_PageDown]	= SDL_SCANCODE_PAGEDOWN;
		imGuiIo.KeyMap[ImGuiKey_Home]		= SDL_SCANCODE_HOME;
		imGuiIo.KeyMap[ImGuiKey_End]		= SDL_SCANCODE_END;
		imGuiIo.KeyMap[ImGuiKey_Insert]		= SDL_SCANCODE_INSERT;
		imGuiIo.KeyMap[ImGuiKey_Delete]		= SDL_SCANCODE_DELETE;
		imGuiIo.KeyMap[ImGuiKey_Backspace]	= SDL_SCANCODE_BACKSPACE;
		imGuiIo.KeyMap[ImGuiKey_Space]		= SDL_SCANCODE_SPACE;
		imGuiIo.KeyMap[ImGuiKey_Enter]		= SDL_SCANCODE_RETURN;
		imGuiIo.KeyMap[ImGuiKey_Escape]		= SDL_SCANCODE_ESCAPE;
		imGuiIo.KeyMap[ImGuiKey_A]			= SDL_SCANCODE_A;
		imGuiIo.KeyMap[ImGuiKey_C]			= SDL_SCANCODE_C;
		imGuiIo.KeyMap[ImGuiKey_V]			= SDL_SCANCODE_V;
		imGuiIo.KeyMap[ImGuiKey_X]			= SDL_SCANCODE_X;
		imGuiIo.KeyMap[ImGuiKey_Y]			= SDL_SCANCODE_Y;
		imGuiIo.KeyMap[ImGuiKey_Z]			= SDL_SCANCODE_Z;
	}

	void ApplicationImplSdl2::processImGuiSdl2Event(const SDL_Event& sdlEvent)
	{
		assert(ImGui::GetCurrentContext() != nullptr);
		ImGuiIO& imGuiIo = ImGui::GetIO();
		switch (sdlEvent.type)
		{
			case SDL_MOUSEWHEEL:
			{
				if (sdlEvent.wheel.x > 0)
				{
					imGuiIo.MouseWheelH += 1.0f;
				}
				if (sdlEvent.wheel.x < 0)
				{
					imGuiIo.MouseWheelH -= 1.0f;
				}
				if (sdlEvent.wheel.y > 0)
				{
					imGuiIo.MouseWheel += 1.0f;
				}
				if (sdlEvent.wheel.y < 0)
				{
					imGuiIo.MouseWheel -= 1.0f;
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			{
				switch (sdlEvent.button.button)
				{
					case SDL_BUTTON_LEFT:
						mImGuiMousePressed[0] = true;
						break;

					case SDL_BUTTON_RIGHT:
						mImGuiMousePressed[1] = true;
						break;

					case SDL_BUTTON_MIDDLE:
						mImGuiMousePressed[2] = true;
						break;
				}
				break;
			}

			case SDL_TEXTINPUT:
			{
				imGuiIo.AddInputCharactersUTF8(sdlEvent.text.text);
				break;
			}

			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				const int key = sdlEvent.key.keysym.scancode;
				assert(key >= 0 && key < IM_ARRAYSIZE(imGuiIo.KeysDown));
				imGuiIo.KeysDown[key] = (SDL_KEYDOWN == sdlEvent.type);
				imGuiIo.KeyShift	  = ((SDL_GetModState() & KMOD_SHIFT) != 0);
				imGuiIo.KeyCtrl		  = ((SDL_GetModState() & KMOD_CTRL) != 0);
				imGuiIo.KeyAlt		  = ((SDL_GetModState() & KMOD_ALT) != 0);
				imGuiIo.KeySuper	  = ((SDL_GetModState() & KMOD_GUI) != 0);
				break;
			}
		}
	}

	void ApplicationImplSdl2::updateImGuiMousePositionAndButtons()
	{
		assert(ImGui::GetCurrentContext() != nullptr);
		ImGuiIO& imGuiIo = ImGui::GetIO();
		int mousePositionX, mousePositionY;
		const Uint32 sdlMouseButtons = SDL_GetMouseState(&mousePositionX, &mousePositionY);
		imGuiIo.MouseDown[0] = (mImGuiMousePressed[0] || (sdlMouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0);	// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame
		imGuiIo.MouseDown[1] = (mImGuiMousePressed[1] || (sdlMouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0);
		imGuiIo.MouseDown[2] = (mImGuiMousePressed[2] || (sdlMouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0);
		mImGuiMousePressed[0] = mImGuiMousePressed[1] = mImGuiMousePressed[2] = false;
		imGuiIo.MousePos = (SDL_GetWindowFlags(mSdlWindow) & SDL_WINDOW_INPUT_FOCUS) ? ImVec2(static_cast<float>(mousePositionX), static_cast<float>(mousePositionY)) : ImVec2(-FLT_MAX, -FLT_MAX);
	}
#endif
