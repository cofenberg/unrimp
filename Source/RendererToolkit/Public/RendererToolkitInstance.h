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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <RendererToolkit/Public/RendererToolkit.h>

#ifdef SHARED_LIBRARIES
	// Dynamically linked libraries
	#ifdef _WIN32
		#include <RendererRuntime/Public/Core/Platform/WindowsHeader.h>
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
namespace RendererRuntime
{
	class IFileManager;
}
namespace RendererToolkit
{
	class Context;
	class IRendererToolkit;
}


//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
#ifndef SHARED_LIBRARIES
	// Statically linked library create renderer runtime instance signatures
	// This is needed to do here because the methods in the library are also defined in global namespace

	// "createRendererToolkitInstance()" signature
	[[nodiscard]] extern RendererToolkit::IRendererToolkit* createRendererToolkitInstance(RendererToolkit::Context& context);
#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	// RendererToolkit/Context.h
	class Context final
	{
	public:
		inline Context(Renderer::ILog& log, Renderer::IAssert& assert, Renderer::IAllocator& allocator, RendererRuntime::IFileManager& fileManager) :
			mLog(log),
			mAssert(assert),
			mAllocator(allocator),
			mFileManager(fileManager)
		{ }
		inline ~Context()
		{ }
		[[nodiscard]] inline Renderer::ILog& getLog() const
		{
			return mLog;
		}
		[[nodiscard]] inline Renderer::IAssert& getAssert() const
		{
			return mAssert;
		}
		[[nodiscard]] inline Renderer::IAllocator& getAllocator() const
		{
			return mAllocator;
		}
		[[nodiscard]] inline RendererRuntime::IFileManager& getFileManager() const
		{
			return mFileManager;
		}
	private:
		explicit Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;
	private:
		Renderer::ILog&				   mLog;
		Renderer::IAssert&			   mAssert;
		Renderer::IAllocator&		   mAllocator;
		RendererRuntime::IFileManager& mFileManager;
	};

	/**
	*  @brief
	*    Renderer toolkit instance using runtime linking
	*
	*  @note
	*    - Designed to be instanced and used inside a single C++ file
	*/
	class RendererToolkitInstance final
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
		*    Renderer toolkit context, the renderer toolkit context instance must stay valid as long as the renderer toolkit instance exists
		*/
		explicit RendererToolkitInstance(Context& context)
		{
			#ifdef SHARED_LIBRARIES
				// Dynamically linked libraries
				#ifdef _WIN32
					// Load in the dll
					static constexpr char RENDERER_TOOLKIT_FILENAME[] = "RendererToolkit.dll";
					mRendererToolkitSharedLibrary = ::LoadLibraryExA(RENDERER_TOOLKIT_FILENAME, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (nullptr != mRendererToolkitSharedLibrary)
					{
						// Get the "createRendererToolkitInstance()" function pointer
						void* symbol = ::GetProcAddress(static_cast<HMODULE>(mRendererToolkitSharedLibrary), "createRendererToolkitInstance");
						if (nullptr != symbol)
						{
							// "createRendererToolkitInstance()" signature
							typedef IRendererToolkit* (__cdecl *createRendererToolkitInstance)(Context& context);

							// Create the renderer toolkit instance
							mRendererToolkit = static_cast<createRendererToolkitInstance>(symbol)(context);
						}
						else
						{
							// Error!
							RENDERER_LOG(context, CRITICAL, "Failed to locate the entry point \"createRendererToolkitInstance\" within the shared renderer toolkit library \"%s\"", RENDERER_TOOLKIT_FILENAME)
						}
					}
					else
					{
						// Error!
						RENDERER_LOG(context, CRITICAL, "Failed to load in the shared renderer toolkit library \"%s\"", RENDERER_TOOLKIT_FILENAME)
					}
				#elif defined LINUX
					// Load in the shared library
					static constexpr char RENDERER_TOOLKIT_FILENAME[] = "libRendererToolkit.so";
					mRendererToolkitSharedLibrary = dlopen(RENDERER_TOOLKIT_FILENAME, RTLD_NOW);
					if (nullptr != mRendererToolkitSharedLibrary)
					{
						// Get the "createRendererToolkitInstance()" function pointer
						void* symbol = dlsym(mRendererToolkitSharedLibrary, "createRendererToolkitInstance");
						if (nullptr != symbol)
						{
							// "createRendererToolkitInstance()" signature
							typedef IRendererToolkit* (*createRendererToolkitInstance)(Context& context);

							// Create the renderer toolkit instance
							mRendererToolkit = reinterpret_cast<createRendererToolkitInstance>(symbol)(context);
						}
						else
						{
							// Error!
							RENDERER_LOG(context, CRITICAL, "Failed to locate the entry point \"createRendererToolkitInstance\" within the renderer toolkit shared renderer toolkit library \"%s\"", RENDERER_TOOLKIT_FILENAME)
						}
					}
					else
					{
						// Error!
						RENDERER_LOG(context, CRITICAL, "Failed to load in the shared renderer toolkit library \"%s\"", RENDERER_TOOLKIT_FILENAME)
					}
				#else
					#error "Unsupported platform"
				#endif
			#else
				// Statically linked libraries

				// Create the renderer toolkit instance
				mRendererToolkit = createRendererToolkitInstance(context);
			#endif
		}

		/**
		*  @brief
		*    Destructor
		*/
		~RendererToolkitInstance()
		{
			// Delete the renderer toolkit instance
			mRendererToolkit = nullptr;

			// Destroy the shared library instance
			#ifdef SHARED_LIBRARIES
				#ifdef _WIN32
					if (nullptr != mRendererToolkitSharedLibrary)
					{
						::FreeLibrary(static_cast<HMODULE>(mRendererToolkitSharedLibrary));
						mRendererToolkitSharedLibrary = nullptr;
					}
				#elif defined LINUX
					if (nullptr != mRendererToolkitSharedLibrary)
					{
						::dlclose(mRendererToolkitSharedLibrary);
						mRendererToolkitSharedLibrary = nullptr;
					}
				#else
					#error "Unsupported platform"
				#endif
			#endif
		}

		/**
		*  @brief
		*    Return the renderer toolkit instance
		*
		*  @remarks
		*    The renderer toolkit instance, can be a null pointer
		*/
		[[nodiscard]] inline IRendererToolkit* getRendererToolkit() const
		{
			return mRendererToolkit;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		void*				mRendererToolkitSharedLibrary;	///< Shared renderer toolkit library, can be a null pointer
		IRendererToolkitPtr mRendererToolkit;				///< Renderer toolkit instance, can be a null pointer


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
