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
#include <Rhi/Public/Rhi.h>

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
		#include <Windows.h>

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
// Statically linked libraries create RHI instance signatures
// This is needed to do here because the methods in the libraries are also defined in global namespace
#ifndef SHARED_LIBRARIES
	// Null
	#ifdef RHI_NULL
		// "createNullRhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createNullRhiInstance(const Rhi::Context&);
	#endif

	// Vulkan
	#ifdef RHI_VULKAN
		// "createVulkanRhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createVulkanRhiInstance(const Rhi::Context&);
	#endif

	// OpenGL
	#ifdef RHI_OPENGL
		// "createOpenGLRhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createOpenGLRhiInstance(const Rhi::Context&);
	#endif

	// OpenGLES3
	#ifdef RHI_OPENGLES3
		// "createOpenGLES3RhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createOpenGLES3RhiInstance(const Rhi::Context&);
	#endif

	// Direct3D 9
	#ifdef RHI_DIRECT3D9
		// "createDirect3D9RhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createDirect3D9RhiInstance(const Rhi::Context&);
	#endif

	// Direct3D 10
	#ifdef RHI_DIRECT3D10
		// "createDirect3D10RhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createDirect3D10RhiInstance(const Rhi::Context&);
	#endif

	// Direct3D 11
	#ifdef RHI_DIRECT3D11
		// "createDirect3D11RhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createDirect3D11RhiInstance(const Rhi::Context&);
	#endif

	// Direct3D 12
	#ifdef RHI_DIRECT3D12
		// "createDirect3D12RhiInstance()" signature
		[[nodiscard]] extern Rhi::IRhi* createDirect3D12RhiInstance(const Rhi::Context&);
	#endif
#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Rhi
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    RHI instance using runtime linking
	*
	*  @note
	*    - Designed to be instanced and used inside a single C++ file
	*/
	class RhiInstance final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rhiName
		*    Case sensitive ASCII name of the RHI to instance, must be valid. Usually "Rhi::DEFAULT_RHI_NAME".
		*    Example RHI names: "Null", "Vulkan", "OpenGL", "OpenGLES3", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
		*  @param[in] context
		*    RHI context, the RHI context instance must stay valid as long as the RHI instance exists
		*  @param[in] loadRhiApiSharedLibrary
		*    Indicates if the RHI instance should load the RHI API shared library (true) or not (false, default)
		*/
		RhiInstance(const char* rhiName, Context& context, bool loadRhiApiSharedLibrary = false) :
			mRhiSharedLibrary(nullptr),
			mOpenGLSharedLibrary(nullptr)
		{
			// In order to keep it simple in this test project the supported RHI implementations are
			// fixed typed in. For a real system a dynamic plugin system would be a good idea.
			if (loadRhiApiSharedLibrary)
			{
				// User wants us to load the RHI API shared library
				loadOpenGLSharedLibraryInternal(rhiName);
				context.setRhiApiSharedLibrary(mOpenGLSharedLibrary);
			}
			#ifdef SHARED_LIBRARIES
				// Dynamically linked libraries
				#ifdef _WIN32
					// Load in the dll
					char rhiFilename[128];
					snprintf(rhiFilename, countof(rhiFilename), "%sRhi.dll", rhiName);
					mRhiSharedLibrary = ::LoadLibraryExA(rhiFilename, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr != mRhiSharedLibrary)
					{
						// Get the "CreateRhiInstance()" function pointer
						char functionName[128];
						snprintf(functionName, countof(functionName), "create%sRhiInstance", rhiName);
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mRhiSharedLibrary), functionName);
						if (nullptr != symbol)
						{
							// "createRhiInstance()" signature
							typedef Rhi::IRhi* (__cdecl *createRhiInstance)(const Rhi::Context&);

							// Create the RHI instance
							mRhi = (static_cast<createRhiInstance>(symbol))(context);
						}
						else
						{
							// Error!
							RHI_LOG(context, CRITICAL, "Failed to locate the entry point \"%s\" within the shared RHI library \"%s\"", functionName, rhiFilename)
						}
					}
					else
					{
						RHI_LOG(context, CRITICAL, "Failed to load in the shared RHI library \"%s\"", rhiFilename)
					}
				#elif defined LINUX
					// Load in the shared library
					char rhiFilename[128];
					snprintf(rhiFilename, countof(rhiFilename), "lib%sRhi.so", rhiName);
					mRhiSharedLibrary = ::dlopen(rhiFilename, RTLD_NOW);
					if (nullptr != mRhiSharedLibrary)
					{
						// Get the "CreateRhiInstance()" function pointer
						char functionName[128];
						snprintf(functionName, countof(functionName), "create%sRhiInstance", rhiName);
						void* symbol = ::dlsym(mRhiSharedLibrary, functionName);
						if (nullptr != symbol)
						{
							// "createRhiInstance()" signature
							typedef Rhi::IRhi* (*createRhiInstance)(const Rhi::Context&);

							// Create the RHI instance
							mRhi = (reinterpret_cast<createRhiInstance>(symbol))(context);
						}
						else
						{
							// Error!
							RHI_LOG(context, CRITICAL, "Failed to locate the entry point \"%s\" within the shared RHI library \"%s\"", functionName, rhiFilename)
						}
					}
					else
					{
						RHI_LOG(context, CRITICAL, "Failed to load in the shared RHI library \"%s\"", rhiFilename)
					}
				#else
					#error "Unsupported platform"
				#endif
			#else
				// Statically linked libraries

				// Null
				#ifdef RHI_NULL
					if (0 == strcmp(rhiName, "Null"))
					{
						// Create the RHI instance
						mRhi = createNullRhiInstance(context);
					}
				#endif

				// Vulkan
				#ifdef RHI_VULKAN
					if (0 == strcmp(rhiName, "Vulkan"))
					{
						// Create the RHI instance
						mRhi = createVulkanRhiInstance(context);
					}
				#endif

				// OpenGL
				#ifdef RHI_OPENGL
					if (0 == strcmp(rhiName, "OpenGL"))
					{
						// Create the RHI instance
						mRhi = createOpenGLRhiInstance(context);
					}
				#endif

				// OpenGLES3
				#ifdef RHI_OPENGLES3
					if (0 == strcmp(rhiName, "OpenGLES3"))
					{
						// Create the RHI instance
						mRhi = createOpenGLES3RhiInstance(context);
					}
				#endif

				// Direct3D 9
				#ifdef RHI_DIRECT3D9
					if (0 == strcmp(rhiName, "Direct3D9"))
					{
						// Create the RHI instance
						mRhi = createDirect3D9RhiInstance(context);
					}
				#endif

				// Direct3D 10
				#ifdef RHI_DIRECT3D10
					if (0 == strcmp(rhiName, "Direct3D10"))
					{
						// Create the RHI instance
						mRhi = createDirect3D10RhiInstance(context);
					}
				#endif

				// Direct3D 11
				#ifdef RHI_DIRECT3D11
					if (0 == strcmp(rhiName, "Direct3D11"))
					{
						// Create the RHI instance
						mRhi = createDirect3D11RhiInstance(context);
					}
				#endif

				// Direct3D 12
				#ifdef RHI_DIRECT3D12
					if (0 == strcmp(rhiName, "Direct3D12"))
					{
						// Create the RHI instance
						mRhi = createDirect3D12RhiInstance(context);
					}
				#endif
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~RhiInstance()
		{
			// Delete the RHI instance
			mRhi = nullptr;

			// Unload the shared library instance
			#ifdef SHARED_LIBRARIES
				#ifdef _WIN32
					if (nullptr != mRhiSharedLibrary)
					{
						::FreeLibrary(static_cast<HMODULE>(mRhiSharedLibrary));
					}
				#elif defined LINUX
					if (nullptr != mRhiSharedLibrary)
					{
						::dlclose(mRhiSharedLibrary);
					}
				#else
					#error "Unsupported platform"
				#endif
			#endif

			// Unload the RHI API shared library instance
			#ifdef _WIN32
				if (nullptr != mOpenGLSharedLibrary)
				{
					::FreeLibrary(static_cast<HMODULE>(mOpenGLSharedLibrary));
				}
			#elif defined LINUX
				if (nullptr != mOpenGLSharedLibrary)
				{
					::dlclose(mOpenGLSharedLibrary);
				}
			#endif
		}

		/**
		*  @brief
		*    Return the RHI instance
		*
		*  @remarks
		*    The RHI instance, can be a null pointer
		*/
		[[nodiscard]] inline IRhi* getRhi() const
		{
			return mRhi;
		}

		/**
		*  @brief
		*    Destroy RHI instance
		*/
		inline void destroyRhi()
		{
			mRhi = nullptr;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		#ifdef SHARED_LIBRARIES
			// Implementation from "08/02/2015 Better array 'countof' implementation with C++ 11 (updated)" - https://www.g-truc.net/post-0708.html
			template<typename T, std::size_t N>
			[[nodiscard]] constexpr std::size_t countof(T const (&)[N])
			{
				return N;
			}
		#endif

		void loadOpenGLSharedLibraryInternal([[maybe_unused]] const char* rhiName)
		{
			// TODO(sw) Currently this is only needed for OpenGL (libGL.so) under Linux. This interacts with the library libX11.
			#ifdef LINUX
				// Under Linux the OpenGL library (libGL.so) registers callbacks in libX11 when loaded, which gets called on XCloseDisplay
				// When the OpenGL library gets unloaded before the XCloseDisplay call then the X11 library wants to call the callbacks registered by the OpenGL library -> crash
				// So we load it here. The user must make sure that an instance of this class gets destroyed after XCloseDisplay was called
				// See http://dri.sourceforge.net/doc/DRIuserguide.html "11.5 libGL.so and dlopen()"
				if (0 == strcmp(rhiName, "OpenGL"))
				{
					mOpenGLSharedLibrary = ::dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
				}
			#endif
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		void*	mRhiSharedLibrary;		///< Shared RHI library, can be a null pointer
		IRhiPtr mRhi;					///< RHI instance, can be a null pointer
		void*	mOpenGLSharedLibrary;	///< Shared OpenGL library, can be a null pointer


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Rhi
