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
#include "RendererRuntime/Public/Core/Platform/PlatformTypes.h"


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
	*    Abstract file interface
	*
	*  @note
	*    - There's no "seek()" or "tell()" by intent, the renderer toolkit can transform the data so during runtime the most simple file access pattern is sufficient
	*    - It's only called "file interface" because this is usually the name for such a concept, doesn't mean the concrete implementation needs to work with files
	*    - Renderer runtime loaders try to read bigger chunks as often as possible instead of too fine granular byte wise read calls
	*/
	class IFile
	{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IFile methods         ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Read                                                  ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the number of bytes inside the file
		*
		*  @return
		*    The number of bytes inside the file
		*/
		[[nodiscard]] virtual size_t getNumberOfBytes() = 0;

		/**
		*  @brief
		*    Read a requested number of bytes from the file
		*
		*  @param[out] destinationBuffer
		*    Destination buffer were to write to, must be at least "numberOfBytes" long, never ever a null pointer
		*  @param[in] numberOfBytes
		*    Number of bytes to write into the destination buffer, it's the callers responsibility that this number of byte is correct
		*/
		virtual void read(void* destinationBuffer, size_t numberOfBytes) = 0;

		/**
		*  @brief
		*    Skip a requested number of bytes
		*
		*  @param[in] numberOfBytes
		*    Number of bytes to skip, it's the callers responsibility that this number of byte is correct
		*/
		virtual void skip(size_t numberOfBytes) = 0;

		//[-------------------------------------------------------]
		//[ Write                                                 ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Write a requested number of bytes into the file
		*
		*  @param[in] sourceBuffer
		*    Source buffer were to read from, must be at least "numberOfBytes" long, never ever a null pointer
		*  @param[in] numberOfBytes
		*    Number of bytes to read from the source buffer and write into the file, it's the callers responsibility that this number of byte is correct
		*/
		virtual void write(const void* sourceBuffer, size_t numberOfBytes) = 0;

		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RHI_DEBUG
			/**
			*  @brief
			*    Get debug filename
			*
			*  @return
			*    The debug filename, can be a null pointer, do not free the memory
			*/
			[[nodiscard]] virtual const char* getDebugFilename() const = 0;
		#endif


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline IFile()
		{
			// Nothing here
		}

		inline virtual ~IFile()
		{
			// Nothing here
		}

		explicit IFile(const IFile&) = delete;
		IFile& operator=(const IFile&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
