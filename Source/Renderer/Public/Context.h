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
#include "Renderer/Public/Export.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class IRhi;
	class ILog;
	class IAssert;
	class IAllocator;
}
namespace Renderer
{
	class IFileManager;
	#ifdef RENDERER_GRAPHICS_DEBUGGER
		class IGraphicsDebugger;
	#endif
	#ifdef RENDERER_PROFILER
		class IProfiler;
	#endif
}


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
		*  @param[in] rhi
		*    RHI instance to use, the render instance must stay valid as long as the renderer instance exists
		*  @param[in] fileManager
		*    File manager instance to use, the file manager instance must stay valid as long as the renderer instance exists
		*  @param[in] graphicsDebugger
		*    Graphics debugger instance to use, the graphics debugger instance must stay valid as long as the renderer instance exists
		*  @param[in] profiler
		*    Profiler instance to use, the profiler instance must stay valid as long as the renderer instance exists
		*/
		#if defined(RENDERER_GRAPHICS_DEBUGGER) && defined(RENDERER_PROFILER)
			RENDERER_API_EXPORT Context(Rhi::IRhi& rhi, IFileManager& fileManager, IGraphicsDebugger& graphicsDebugger, IProfiler& profiler);
		#elif defined RENDERER_GRAPHICS_DEBUGGER
			RENDERER_API_EXPORT Context(Rhi::IRhi& rhi, IFileManager& fileManager, IGraphicsDebugger& graphicsDebugger);
		#elif defined RENDERER_PROFILER
			RENDERER_API_EXPORT Context(Rhi::IRhi& rhi, IFileManager& fileManager, IProfiler& profiler);
		#else
			RENDERER_API_EXPORT Context(Rhi::IRhi& rhi, IFileManager& fileManager);
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
		*    Return the RHI log instance
		*
		*  @return
		*    The RHI log instance
		*/
		[[nodiscard]] inline Rhi::ILog& getLog() const
		{
			return mLog;
		}

		/**
		*  @brief
		*    Return the RHI assert instance
		*
		*  @return
		*    The RHI assert instance
		*/
		[[nodiscard]] inline Rhi::IAssert& getAssert() const
		{
			return mAssert;
		}

		/**
		*  @brief
		*    Return the RHI allocator instance
		*
		*  @return
		*    The RHI allocator instance
		*/
		[[nodiscard]] inline Rhi::IAllocator& getAllocator() const
		{
			return mAllocator;
		}

		/**
		*  @brief
		*    Return the used RHI instance
		*
		*  @return
		*    The used RHI instance
		*/
		[[nodiscard]] inline Rhi::IRhi& getRhi() const
		{
			return mRhi;
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

		#ifdef RENDERER_GRAPHICS_DEBUGGER
			/**
			*  @brief
			*    Return the used graphics debugger instance
			*
			*  @return
			*    The used graphics debugger instance
			*/
			[[nodiscard]] inline IGraphicsDebugger& getGraphicsDebugger() const
			{
				return mGraphicsDebugger;
			}
		#endif

		#ifdef RENDERER_PROFILER
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
		Rhi::ILog&		 mLog;
		Rhi::IAssert&	 mAssert;
		Rhi::IAllocator& mAllocator;
		Rhi::IRhi&		 mRhi;
		IFileManager&	 mFileManager;
		#ifdef RENDERER_GRAPHICS_DEBUGGER
			IGraphicsDebugger& mGraphicsDebugger;
		#endif
		#ifdef RENDERER_PROFILER
			IProfiler& mProfiler;
		#endif


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
