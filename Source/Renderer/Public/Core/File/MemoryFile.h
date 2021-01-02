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
#include "Renderer/Public/Core/File/IFile.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <vector>
	#include <cstring>		// For "memcpy()"
	#include <iterator>		// For "std::back_inserter()"
	#include <algorithm>	// For "std::copy()"
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IFileManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef const char* VirtualFilename;	///< UTF-8 virtual filename, the virtual filename scheme is "<mount point = project name>/<asset directory>/<asset name>.<file extension>" (example "Example/Mesh/Monster/Squirrel.mesh"), never ever a null pointer and always finished by a terminating zero


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Memory mapped file
	*
	*  @note
	*    - Supports LZ4 compression ( http://lz4.github.io/lz4/ )
	*    - Designed for instance re-usage
	*/
	class MemoryFile final : public IFile
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<uint8_t> ByteVector;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline MemoryFile() :
			mNumberOfDecompressedBytes(0),
			mCurrentDataPointer(nullptr)
		{
			// Nothing here
		}

		inline MemoryFile(size_t reserveNumberOfCompressedBytes, size_t reserveNumberOfDecompressedBytes) :
			mNumberOfDecompressedBytes(0),
			mCurrentDataPointer(nullptr)
		{
			mCompressedData.reserve(reserveNumberOfCompressedBytes);
			mDecompressedData.reserve(reserveNumberOfDecompressedBytes);
		}

		inline virtual ~MemoryFile() override
		{
			// Nothing here
		}

		[[nodiscard]] inline ByteVector& getByteVector()
		{
			return mDecompressedData;
		}

		[[nodiscard]] inline const ByteVector& getByteVector() const
		{
			return mDecompressedData;
		}

		[[nodiscard]] RENDERER_API_EXPORT bool loadLz4CompressedDataByVirtualFilename(uint32_t formatType, uint32_t formatVersion, const IFileManager& fileManager, VirtualFilename virtualFilename);
		[[nodiscard]] RENDERER_API_EXPORT bool loadLz4CompressedDataFromFile(uint32_t formatType, uint32_t formatVersion, IFile& file);
		RENDERER_API_EXPORT void setLz4CompressedDataByFile(IFile& file, uint32_t numberOfCompressedBytes, uint32_t numberOfDecompressedBytes);
		RENDERER_API_EXPORT void decompress();
		[[nodiscard]] RENDERER_API_EXPORT bool writeLz4CompressedDataByVirtualFilename(uint32_t formatType, uint32_t formatVersion, const IFileManager& fileManager, VirtualFilename virtualFilename) const;


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IFile methods                ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual size_t getNumberOfBytes() override
		{
			return mDecompressedData.size();
		}

		inline virtual void read(void* destinationBuffer, size_t numberOfBytes) override
		{
			ASSERT(nullptr != destinationBuffer, "Letting a file read into a null destination buffer is not allowed")
			ASSERT(0 != numberOfBytes, "Letting a file read zero bytes is not allowed")
			ASSERT((mCurrentDataPointer - mDecompressedData.data()) + numberOfBytes <= mDecompressedData.size(), "Invalid number of bytes")
			memcpy(destinationBuffer, mCurrentDataPointer, numberOfBytes);
			mCurrentDataPointer += numberOfBytes;
		}

		inline virtual void skip(size_t numberOfBytes) override
		{
			ASSERT(0 != numberOfBytes, "Letting a file skip zero bytes is not allowed")
			ASSERT((mCurrentDataPointer - mDecompressedData.data()) + numberOfBytes <= mDecompressedData.size(), "Invalid number of bytes")
			mCurrentDataPointer += numberOfBytes;
		}

		inline virtual void write(const void* sourceBuffer, size_t numberOfBytes) override
		{
			ASSERT(nullptr != sourceBuffer, "Letting a file write from a null source buffer is not allowed")
			ASSERT(0 != numberOfBytes, "Letting a file write zero bytes is not allowed")
			std::copy(static_cast<const uint8_t*>(sourceBuffer), static_cast<const uint8_t*>(sourceBuffer) + numberOfBytes, std::back_inserter(mDecompressedData));
		}

		#ifdef RHI_DEBUG
			[[nodiscard]] inline virtual const char* getDebugFilename() const override
			{
				return mDebugName.c_str();
			}
		#endif


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit MemoryFile(const MemoryFile&) = delete;
		MemoryFile& operator=(const MemoryFile&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ByteVector mCompressedData;		///< Owns the data
		ByteVector mDecompressedData;	///< Owns the data
		uint32_t   mNumberOfDecompressedBytes;
		uint8_t*   mCurrentDataPointer;	///< Pointer to the current uncompressed data position, doesn't own the data
		#ifdef RHI_DEBUG
			std::string mDebugName;	///< Debug name for easier file identification when debugging
		#endif


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
