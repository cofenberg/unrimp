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
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ILog;
	class IAssert;
	class IAllocator;
}
namespace RendererRuntime
{
	class IFileManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
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
		*  @param[in] log
		*    Log instance to use, the log instance must stay valid as long as the renderer toolkit instance exists
		*  @param[in] assert
		*    Assert instance to use, the assert instance must stay valid as long as the renderer toolkit instance exists
		*  @param[in] allocator
		*    Allocator instance to use, the allocator instance must stay valid as long as the renderer toolkit instance exists
		*  @param[in] fileManager
		*    File manager instance to use, the file manager instance must stay valid as long as the renderer toolkit instance exists
		*/
		inline Context(Renderer::ILog& log, Renderer::IAssert& assert, Renderer::IAllocator& allocator, RendererRuntime::IFileManager& fileManager) :
			mLog(log),
			mAssert(assert),
			mAllocator(allocator),
			mFileManager(fileManager)
		{
			// Nothing here
		}

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
		*    Return the used log instance
		*
		*  @return
		*    The used log instance
		*/
		[[nodiscard]] inline Renderer::ILog& getLog() const
		{
			return mLog;
		}

		/**
		*  @brief
		*    Return the used assert instance
		*
		*  @return
		*    The used assert instance
		*/
		[[nodiscard]] inline Renderer::IAssert& getAssert() const
		{
			return mAssert;
		}

		/**
		*  @brief
		*    Return the used allocator instance
		*
		*  @return
		*    The used allocator instance
		*/
		[[nodiscard]] inline Renderer::IAllocator& getAllocator() const
		{
			return mAllocator;
		}

		/**
		*  @brief
		*    Return the used file manager instance
		*
		*  @return
		*    The used file manager instance
		*/
		[[nodiscard]] inline RendererRuntime::IFileManager& getFileManager() const
		{
			return mFileManager;
		}


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
		Renderer::ILog&				   mLog;
		Renderer::IAssert&			   mAssert;
		Renderer::IAllocator&		   mAllocator;
		RendererRuntime::IFileManager& mFileManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
