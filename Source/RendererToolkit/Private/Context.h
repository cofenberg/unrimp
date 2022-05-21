/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
namespace Rhi
{
	class ILog;
	class IAssert;
	class IAllocator;
}
namespace Renderer
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
		*    RHI log instance to use, the log instance must stay valid as long as the renderer toolkit instance exists
		*  @param[in] assert
		*    RHI assert instance to use, the assert instance must stay valid as long as the renderer toolkit instance exists
		*  @param[in] allocator
		*    RHI allocator instance to use, the allocator instance must stay valid as long as the renderer toolkit instance exists
		*  @param[in] fileManager
		*    Renderer file manager instance to use, the file manager instance must stay valid as long as the renderer toolkit instance exists
		*/
		inline Context(Rhi::ILog& log, Rhi::IAssert& assert, Rhi::IAllocator& allocator, Renderer::IFileManager& fileManager) :
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
		*    Return the used RHI log instance
		*
		*  @return
		*    The used RHI log instance
		*/
		[[nodiscard]] inline Rhi::ILog& getLog() const
		{
			return mLog;
		}

		/**
		*  @brief
		*    Return the used RHI assert instance
		*
		*  @return
		*    The used RHI assert instance
		*/
		[[nodiscard]] inline Rhi::IAssert& getAssert() const
		{
			return mAssert;
		}

		/**
		*  @brief
		*    Return the used RHI allocator instance
		*
		*  @return
		*    The used RHI allocator instance
		*/
		[[nodiscard]] inline Rhi::IAllocator& getAllocator() const
		{
			return mAllocator;
		}

		/**
		*  @brief
		*    Return the used renderer file manager instance
		*
		*  @return
		*    The used renderer file manager instance
		*/
		[[nodiscard]] inline Renderer::IFileManager& getFileManager() const
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
		Rhi::ILog&				mLog;
		Rhi::IAssert&			mAssert;
		Rhi::IAllocator&		mAllocator;
		Renderer::IFileManager& mFileManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
