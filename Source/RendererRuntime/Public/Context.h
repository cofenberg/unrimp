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
#include "RendererRuntime/Public/Export.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ILog;
	class IAssert;
	class IRenderer;
	class IAllocator;
}
namespace RendererRuntime
{
	class IFileManager;
	#ifdef RENDERER_RUNTIME_PROFILER
		class IProfiler;
	#endif
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Context class encapsulating all embedding related wirings
	*/
	class Context final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Renderer instance to use, the render instance must stay valid as long as the renderer runtime instance exists
		*  @param[in] fileManager
		*    File manager instance to use, the file manager instance must stay valid as long as the renderer runtime instance exists
		*  @param[in] profiler
		*    Profiler instance to use, the profiler instance must stay valid as long as the renderer runtime instance exists
		*/
		#ifdef RENDERER_RUNTIME_PROFILER
			RENDERERRUNTIME_API_EXPORT Context(Renderer::IRenderer& renderer, IFileManager& fileManager, IProfiler& profiler);
		#else
			RENDERERRUNTIME_API_EXPORT Context(Renderer::IRenderer& renderer, IFileManager& fileManager);
		#endif

		/**
		*  @brief
		*    Destructor
		*/
		inline ~Context()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the log instance
		*
		*  @return
		*    The log instance
		*/
		[[nodiscard]] inline Renderer::ILog& getLog() const
		{
			return mLog;
		}

		/**
		*  @brief
		*    Return the assert instance
		*
		*  @return
		*    The assert instance
		*/
		[[nodiscard]] inline Renderer::IAssert& getAssert() const
		{
			return mAssert;
		}

		/**
		*  @brief
		*    Return the allocator instance
		*
		*  @return
		*    The allocator instance
		*/
		[[nodiscard]] inline Renderer::IAllocator& getAllocator() const
		{
			return mAllocator;
		}

		/**
		*  @brief
		*    Return the used renderer instance
		*
		*  @return
		*    The used renderer instance
		*/
		[[nodiscard]] inline Renderer::IRenderer& getRenderer() const
		{
			return mRenderer;
		}

		/**
		*  @brief
		*    Return the used file manager instance
		*
		*  @return
		*    The used file manager instance
		*/
		[[nodiscard]] inline IFileManager& getFileManager() const
		{
			return mFileManager;
		}

		#ifdef RENDERER_RUNTIME_PROFILER
			/**
			*  @brief
			*    Return the used profiler instance
			*
			*  @return
			*    The used profiler instance
			*/
			[[nodiscard]] inline IProfiler& getProfiler() const
			{
				return mProfiler;
			}
		#endif


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::ILog&		  mLog;
		Renderer::IAssert&	  mAssert;
		Renderer::IAllocator& mAllocator;
		Renderer::IRenderer&  mRenderer;
		IFileManager&		  mFileManager;
		#ifdef RENDERER_RUNTIME_PROFILER
			IProfiler& mProfiler;
		#endif


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
