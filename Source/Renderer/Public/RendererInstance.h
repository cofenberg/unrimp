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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Public/Context.h>
#include <Renderer/Public/IRenderer.h>

#ifdef SHARED_LIBRARIES
	#ifdef _WIN32
		#include "Renderer/Public/Core/Platform/WindowsHeader.h"
	#elif defined LINUX
		#include <dlfcn.h>
	#else
		#error "Unsupported platform"
	#endif

	#include <stdio.h>
#endif


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class Context;
}


//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
#ifndef SHARED_LIBRARIES
	// Statically linked library create renderer instance signatures
	// This is needed to do here because the methods in the library are also defined in global namespace

	// "createRendererInstance()" signature
	[[nodiscard]] extern Renderer::IRenderer* createRendererInstance(Renderer::Context& context);
#endif


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
	*    Renderer instance using runtime linking
	*
	*  @note
	*    - Designed to be instanced and used inside a single C++ file
	*/
	class RendererInstance final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*/
		explicit RendererInstance(Context& context)
		{
			#ifdef SHARED_LIBRARIES
				// Dynamically linked libraries
				#ifdef _WIN32
					// Load in the dll
					static constexpr const char RENDERER_FILENAME[] = "Renderer.dll";
					mRendererSharedLibrary = ::LoadLibraryExA(RENDERER_FILENAME, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr != mRendererSharedLibrary)
					{
						// Get the "createRendererInstance()" function pointer
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mRendererSharedLibrary), "createRendererInstance");
						if (nullptr != symbol)
						{
							// "createRendererInstance()" signature
							typedef IRenderer* (__cdecl *createRendererInstance)(Context& context);

							// Create the renderer instance
							mRenderer = static_cast<createRendererInstance>(symbol)(context);
						}
						else
						{
							// Error!
							RHI_LOG(context, CRITICAL, "Failed to locate the entry point \"createRendererInstance\" within the shared renderer library \"%s\"", RENDERER_FILENAME)
						}
					}
					else
					{
						// Error!
						RHI_LOG(context, CRITICAL, "Failed to load in the shared renderer library \"%s\"", RENDERER_FILENAME)
					}
				#elif defined LINUX
					// Load in the shared library
					static constexpr const char RENDERER_FILENAME[] = "libRenderer.so";
					mRendererSharedLibrary = dlopen(RENDERER_FILENAME, RTLD_NOW);
					if (nullptr != mRendererSharedLibrary)
					{
						// Get the "createRendererInstance()" function pointer
						void* symbol = dlsym(mRendererSharedLibrary, "createRendererInstance");
						if (nullptr != symbol)
						{
							// "createRendererInstance()" signature
							typedef IRenderer* (*createRendererInstance)(Context& context);

							// Create the renderer instance
							mRenderer = reinterpret_cast<createRendererInstance>(symbol)(context);
						}
						else
						{
							// Error!
							RHI_LOG(context, CRITICAL, "Failed to locate the entry point \"createRendererInstance\" within the shared renderer library \"%s\"", RENDERER_FILENAME)
						}
					}
					else
					{
						// Error!
						RHI_LOG(context, CRITICAL, "Failed to load in the shared renderer library \"%s\"", RENDERER_FILENAME)
					}
				#else
					#error "Unsupported platform"
				#endif
			#else
				// Statically linked libraries

				// Create the renderer instance
				mRenderer = createRendererInstance(context);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~RendererInstance()
		{
			// Delete the renderer instance
			mRenderer = nullptr;

			// Destroy the shared library instance
			#ifdef SHARED_LIBRARIES
				#ifdef _WIN32
					if (nullptr != mRendererSharedLibrary)
					{
						::FreeLibrary(static_cast<HMODULE>(mRendererSharedLibrary));
						mRendererSharedLibrary = nullptr;
					}
				#elif defined LINUX
					if (nullptr != mRendererSharedLibrary)
					{
						::dlclose(mRendererSharedLibrary);
						mRendererSharedLibrary = nullptr;
					}
				#else
					#error "Unsupported platform"
				#endif
			#endif
		}

		/**
		*  @brief
		*    Return the renderer instance
		*
		*  @remarks
		*    The renderer instance, can be a null pointer
		*/
		[[nodiscard]] inline IRenderer* getRenderer() const
		{
			return mRenderer;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		void*		 mRendererSharedLibrary;	///< Shared renderer library, can be a null pointer
		IRendererPtr mRenderer;					///< Renderer instance, can be a null pointer


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
