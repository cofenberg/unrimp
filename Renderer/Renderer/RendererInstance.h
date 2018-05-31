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
#include <Renderer/Renderer.h>

#ifdef SHARED_LIBRARIES
	#ifdef _WIN32
		// Set Windows version to Windows Vista (0x0600), we don't support Windows XP (0x0501)
		#ifdef WINVER
			#undef WINVER
		#endif
		#define WINVER			0x0600
		#ifdef _WIN32_WINNT
			#undef _WIN32_WINNT
		#endif
		#define _WIN32_WINNT	0x0600

		// Exclude some stuff from "windows.h" to speed up compilation a bit
		#define WIN32_LEAN_AND_MEAN
		#define NOGDICAPMASKS
		#define NOMENUS
		#define NOICONS
		#define NOKEYSTATES
		#define NOSYSCOMMANDS
		#define NORASTEROPS
		#define OEMRESOURCE
		#define NOATOM
		#define NOMEMMGR
		#define NOMETAFILE
		#define NOOPENFILE
		#define NOSCROLL
		#define NOSERVICE
		#define NOSOUND
		#define NOWH
		#define NOCOMM
		#define NOKANJI
		#define NOHELP
		#define NOPROFILER
		#define NODEFERWINDOWPOS
		#define NOMCX
		#define NOCRYPT
		#include <windows.h>

		// Get rid of some nasty OS macros
		#undef min
		#undef max
	#elif defined LINUX
		// Nothing here
	#else
		#error "Unsupported platform"
	#endif

	#include <stdio.h>
#endif

#ifdef LINUX
	#include <dlfcn.h>	// We need it also for the non shared libraries case
#endif


//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Statically linked libraries create renderer instance signatures
// This is needed to do here because the methods in the libraries are also defined in global namespace
#ifndef SHARED_LIBRARIES
	// Null
	#ifdef RENDERER_NULL
		// "createNullRendererInstance()" signature
		extern Renderer::IRenderer* createNullRendererInstance(const Renderer::Context&);
	#endif

	// Vulkan
	#ifdef RENDERER_VULKAN
		// "createVulkanRendererInstance()" signature
		extern Renderer::IRenderer* createVulkanRendererInstance(const Renderer::Context&);
	#endif

	// OpenGL
	#ifdef RENDERER_OPENGL
		// "createOpenGLRendererInstance()" signature
		extern Renderer::IRenderer* createOpenGLRendererInstance(const Renderer::Context&);
	#endif

	// OpenGLES3
	#ifdef RENDERER_OPENGLES3
		// "createOpenGLES3RendererInstance()" signature
		extern Renderer::IRenderer* createOpenGLES3RendererInstance(const Renderer::Context&);
	#endif

	// Direct3D 9
	#ifdef RENDERER_DIRECT3D9
		// "createDirect3D9RendererInstance()" signature
		extern Renderer::IRenderer* createDirect3D9RendererInstance(const Renderer::Context&);
	#endif

	// Direct3D 10
	#ifdef RENDERER_DIRECT3D10
		// "createDirect3D10RendererInstance()" signature
		extern Renderer::IRenderer* createDirect3D10RendererInstance(const Renderer::Context&);
	#endif

	// Direct3D 11
	#ifdef RENDERER_DIRECT3D11
		// "createDirect3D11RendererInstance()" signature
		extern Renderer::IRenderer* createDirect3D11RendererInstance(const Renderer::Context&);
	#endif

	// Direct3D 12
	#ifdef RENDERER_DIRECT3D12
		// "createDirect3D12RendererInstance()" signature
		extern Renderer::IRenderer* createDirect3D12RendererInstance(const Renderer::Context&);
	#endif
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
		*  @param[in] rendererName
		*    Case sensitive ASCII name of the renderer to instance, must be valid.
		*    Example renderer names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*  @param[in] loadRendererApiSharedLibrary
		*    Indicates if the renderer instance should load the renderer API shared library (true) or not (false, default)
		*/
		RendererInstance(const char* rendererName, Context& context, bool loadRendererApiSharedLibrary = false) :
			mRendererSharedLibrary(nullptr),
			mRendererApiSharedLibrary(nullptr)
		{
			// In order to keep it simple in this test project the supported renderer backends are
			// fixed typed in. For a real system a dynamic plugin system would be a good idea.
			if (loadRendererApiSharedLibrary)
			{
				// User wants us to load the renderer API shared library
				loadRendererApiSharedLibraryInternal(rendererName);
				context.setRendererApiSharedLibrary(mRendererApiSharedLibrary);
			}
			#ifdef SHARED_LIBRARIES
				// Dynamically linked libraries
				#ifdef _WIN32
					// Load in the dll
					char rendererFilename[128];
					snprintf(rendererFilename, countof(rendererFilename), "%sRenderer.dll", rendererName);
					mRendererSharedLibrary = ::LoadLibraryExA(rendererFilename, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr != mRendererSharedLibrary)
					{
						// Get the "CreateRendererInstance()" function pointer
						char functionName[128];
						snprintf(functionName, countof(functionName), "create%sRendererInstance", rendererName);
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mRendererSharedLibrary), functionName);
						if (nullptr != symbol)
						{
							// "createRendererInstance()" signature
							typedef Renderer::IRenderer* (__cdecl *createRendererInstance)(const Renderer::Context&);

							// Create the renderer instance
							mRenderer = (static_cast<createRendererInstance>(symbol))(context);
						}
						else
						{
							// Error!
							RENDERER_LOG(context, CRITICAL, "Failed to locate the entry point \"%s\" within the shared renderer library \"%s\"", functionName, rendererFilename)
						}
					}
					else
					{
						RENDERER_LOG(context, CRITICAL, "Failed to load in the shared renderer library \"%s\"", rendererFilename)
					}
				#elif defined LINUX
					// Load in the shared library
					char rendererFilename[128];
					snprintf(rendererFilename, countof(rendererFilename), "lib%sRenderer.so", rendererName);
					mRendererSharedLibrary = ::dlopen(rendererFilename, RTLD_NOW);
					if (nullptr != mRendererSharedLibrary)
					{
						// Get the "CreateRendererInstance()" function pointer
						char functionName[128];
						snprintf(functionName, countof(functionName), "create%sRendererInstance", rendererName);
						void* symbol = ::dlsym(mRendererSharedLibrary, functionName);
						if (nullptr != symbol)
						{
							// "createRendererInstance()" signature
							typedef Renderer::IRenderer* (*createRendererInstance)(const Renderer::Context&);

							// Create the renderer instance
							mRenderer = (reinterpret_cast<createRendererInstance>(symbol))(context);
						}
						else
						{
							// Error!
							RENDERER_LOG(context, CRITICAL, "Failed to locate the entry point \"%s\" within the shared renderer library \"%s\"", functionName, rendererFilename)
						}
					}
					else
					{
						RENDERER_LOG(context, CRITICAL, "Failed to load in the shared renderer library \"%s\"", rendererFilename)
					}
				#else
					#error "Unsupported platform"
				#endif
			#else
				// Statically linked libraries

				// Null
				#ifdef RENDERER_NULL
					if (0 == strcmp(rendererName, "Null"))
					{
						// Create the renderer instance
						mRenderer = createNullRendererInstance(context);
					}
				#endif

				// Vulkan
				#ifdef RENDERER_VULKAN
					if (0 == strcmp(rendererName, "Vulkan"))
					{
						// Create the renderer instance
						mRenderer = createVulkanRendererInstance(context);
					}
				#endif

				// OpenGL
				#ifdef RENDERER_OPENGL
					if (0 == strcmp(rendererName, "OpenGL"))
					{
						// Create the renderer instance
						mRenderer = createOpenGLRendererInstance(context);
					}
				#endif

				// OpenGLES3
				#ifdef RENDERER_OPENGLES3
					if (0 == strcmp(rendererName, "OpenGLES3"))
					{
						// Create the renderer instance
						mRenderer = createOpenGLES3RendererInstance(context);
					}
				#endif

				// Direct3D 9
				#ifdef RENDERER_DIRECT3D9
					if (0 == strcmp(rendererName, "Direct3D9"))
					{
						// Create the renderer instance
						mRenderer = createDirect3D9RendererInstance(context);
					}
				#endif

				// Direct3D 10
				#ifdef RENDERER_DIRECT3D10
					if (0 == strcmp(rendererName, "Direct3D10"))
					{
						// Create the renderer instance
						mRenderer = createDirect3D10RendererInstance(context);
					}
				#endif

				// Direct3D 11
				#ifdef RENDERER_DIRECT3D11
					if (0 == strcmp(rendererName, "Direct3D11"))
					{
						// Create the renderer instance
						mRenderer = createDirect3D11RendererInstance(context);
					}
				#endif

				// Direct3D 12
				#ifdef RENDERER_DIRECT3D12
					if (0 == strcmp(rendererName, "Direct3D12"))
					{
						// Create the renderer instance
						mRenderer = createDirect3D12RendererInstance(context);
					}
				#endif
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

			// Unload the shared library instance
			#ifdef SHARED_LIBRARIES
				#ifdef _WIN32
					if (nullptr != mRendererSharedLibrary)
					{
						::FreeLibrary(static_cast<HMODULE>(mRendererSharedLibrary));
						mRendererSharedLibrary = nullptr;
					}

					if (nullptr != mRendererApiSharedLibrary)
					{
						::FreeLibrary(static_cast<HMODULE>(mRendererApiSharedLibrary));
						mRendererApiSharedLibrary = nullptr;
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

			// Unload the renderer API shared library instance
			#ifdef _WIN32
				if (nullptr != mRendererApiSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mRendererApiSharedLibrary));
					mRendererApiSharedLibrary = nullptr;
				}
			#elif defined LINUX
				if (nullptr != mRendererApiSharedLibrary)
				{
					::dlclose(mRendererApiSharedLibrary);
					mRendererApiSharedLibrary = nullptr;
				}
			#endif
		}

		/**
		*  @brief
		*    Return the renderer instance
		*
		*  @remarks
		*    The renderer instance, can be a null pointer
		*/
		inline IRenderer* getRenderer() const
		{
			return mRenderer;
		}

		/**
		*  @brief
		*    Destroy renderer instance
		*/
		inline void destroyRenderer()
		{
			mRenderer = nullptr;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		#ifdef SHARED_LIBRARIES
			// Implementation from "08/02/2015 Better array 'countof' implementation with C++ 11 (updated)" - https://www.g-truc.net/post-0708.html
			template<typename T, std::size_t N>
			constexpr std::size_t countof(T const (&)[N])
			{
				return N;
			}
		#endif

		void loadRendererApiSharedLibraryInternal(MAYBE_UNUSED const char* rendererName)
		{
			// TODO(sw) Currently this is only needed for OpenGL (libGL.so) under Linux. This interacts with the library libX11.
			#ifdef LINUX
				// Under Linux the OpenGL library (libGL.so) registers callbacks in libX11 when loaded, which gets called on XCloseDisplay
				// When the OpenGL library gets unloaded before the XCloseDisplay call then the X11 library wants to call the callbacks registered by the OpenGL library -> crash
				// So we load it here. The user must make sure that an instance of this class gets destroyed after XCloseDisplay was called
				// See http://dri.sourceforge.net/doc/DRIuserguide.html "11.5 libGL.so and dlopen()"
				if (0 == strcmp(rendererName, "OpenGL"))
				{
					mRendererApiSharedLibrary = ::dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
				}
			#endif
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		void*		 mRendererSharedLibrary;	///< Shared renderer library, can be a null pointer
		IRendererPtr mRenderer;					///< Renderer instance, can be a null pointer
		void*		 mRendererApiSharedLibrary;	///< Shared renderer API library, can be a null pointer


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
