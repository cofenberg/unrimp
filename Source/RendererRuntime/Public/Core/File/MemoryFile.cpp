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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Core/File/MemoryFile.h"
#include "RendererRuntime/Public/Core/File/IFileManager.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <lz4/lz4hc.h>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		struct FileFormatHeader final
		{
			// Format
			uint32_t formatType;
			uint32_t formatVersion;
			// Content
			uint32_t numberOfCompressedBytes;
			uint32_t numberOfDecompressedBytes;
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	bool MemoryFile::loadLz4CompressedDataByVirtualFilename(uint32_t formatType, uint32_t formatVersion, const IFileManager& fileManager, VirtualFilename virtualFilename)
	{
		bool result = false;
		IFile* file = fileManager.openFile(IFileManager::FileMode::READ, virtualFilename);
		if (nullptr != file)
		{
			// Tell the memory mapped file about the LZ4 compressed data
			result = loadLz4CompressedDataFromFile(formatType, formatVersion, *file);
			#ifdef RHI_DEBUG
				mDebugName = virtualFilename;
			#endif

			// Close file
			fileManager.closeFile(*file);
		}

		// Done
		return result;
	}

	bool MemoryFile::loadLz4CompressedDataFromFile(uint32_t formatType, uint32_t formatVersion, IFile& file)
	{
		// Read in the file format header
		::detail::FileFormatHeader fileFormatHeader;
		file.read(&fileFormatHeader, sizeof(::detail::FileFormatHeader));
		if (formatType == fileFormatHeader.formatType && formatVersion == fileFormatHeader.formatVersion)
		{
			// Tell the memory mapped file about the LZ4 compressed data
			setLz4CompressedDataByFile(file, fileFormatHeader.numberOfCompressedBytes, fileFormatHeader.numberOfDecompressedBytes);
			#ifdef RHI_DEBUG
				mDebugName = file.getDebugFilename();
			#endif

			// Done
			return true;
		}
		else
		{
			// Error!
			ASSERT(false);
			return false;
		}
	}

	void MemoryFile::setLz4CompressedDataByFile(IFile& file, uint32_t numberOfCompressedBytes, uint32_t numberOfDecompressedBytes)
	{
		// Sanity checks
		ASSERT((0 != numberOfCompressedBytes) && "Zero LZ4 compressed bytes are invalid");
		ASSERT((0 != numberOfDecompressedBytes) && "Zero LZ4 decompressed bytes are invalid");

		// Read data
		mNumberOfDecompressedBytes = numberOfDecompressedBytes;
		mDecompressedData.clear();
		mCurrentDataPointer = nullptr;
		mCompressedData.resize(numberOfCompressedBytes);
		file.read(mCompressedData.data(), numberOfCompressedBytes);
	}

	void MemoryFile::decompress()
	{
		mDecompressedData.resize(mNumberOfDecompressedBytes);
		[[maybe_unused]] const int numberOfDecompressedBytes = LZ4_decompress_safe(reinterpret_cast<const char*>(mCompressedData.data()), reinterpret_cast<char*>(mDecompressedData.data()), static_cast<int>(mCompressedData.size()), static_cast<int>(mNumberOfDecompressedBytes));
		ASSERT(mNumberOfDecompressedBytes == static_cast<uint32_t>(numberOfDecompressedBytes));
		mCurrentDataPointer = mDecompressedData.data();
	}

	bool MemoryFile::writeLz4CompressedDataByVirtualFilename(uint32_t formatType, uint32_t formatVersion, const IFileManager& fileManager, VirtualFilename virtualFilename) const
	{
		// Open file
		IFile* file = fileManager.openFile(IFileManager::FileMode::WRITE, virtualFilename);
		if (nullptr != file)
		{
			// Write file
			const int destinationCapacity = LZ4_compressBound(static_cast<int>(mDecompressedData.size()));
			char* destination = new char[static_cast<unsigned int>(destinationCapacity)];
			{
				const int numberOfWrittenBytes = LZ4_compress_HC(reinterpret_cast<const char*>(mDecompressedData.data()), destination, static_cast<int>(mDecompressedData.size()), destinationCapacity, LZ4HC_CLEVEL_MAX);

				{ // Write down the file format header
					::detail::FileFormatHeader fileFormatHeader;
					fileFormatHeader.formatType				   = formatType;
					fileFormatHeader.formatVersion			   = formatVersion;
					fileFormatHeader.numberOfCompressedBytes   = static_cast<uint32_t>(numberOfWrittenBytes);
					fileFormatHeader.numberOfDecompressedBytes = static_cast<uint32_t>(mDecompressedData.size());
					file->write(&fileFormatHeader, sizeof(::detail::FileFormatHeader));
				}

				// Write down the compressed data
				file->write(destination, static_cast<size_t>(numberOfWrittenBytes));
			}
			delete [] destination;

			// Close file
			fileManager.closeFile(*file);

			// Done
			return true;
		}

		// Error!
		return false;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
