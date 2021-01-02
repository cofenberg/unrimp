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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererToolkit/Private/Helper/AssimpIOSystem.h"

#include <Renderer/Public/Core/File/IFile.h>
#include <Renderer/Public/Core/File/IFileManager.h>

#include <assimp/IOStream.hpp>

#include <stdexcept>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		class AssimpIOStream final : public Assimp::IOStream
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline AssimpIOStream(const Renderer::IFileManager& fileManager, Renderer::IFile& file) :
				mFileManager(fileManager),
				mFile(file),
				mNumberOfBytes(mFile.getNumberOfBytes()),
				mCurrentPosition(0)
			{
				// Nothing here
			}

			inline virtual ~AssimpIOStream() override
			{
				mFileManager.closeFile(mFile);
			}

			[[nodiscard]] inline Renderer::IFile& getFile() const
			{
				return mFile;
			}


		//[-------------------------------------------------------]
		//[ Public virtual Assimp::IOStream methods               ]
		//[-------------------------------------------------------]
		public:
			[[nodiscard]] virtual size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override
			{
				size_t numberOfBytes = pSize * pCount;
				size_t newCurrentPosition = mCurrentPosition + numberOfBytes;
				if (newCurrentPosition > mNumberOfBytes)
				{
					const size_t numberOfOverflowBytes = (newCurrentPosition - mNumberOfBytes);
					numberOfBytes -= numberOfOverflowBytes;
					newCurrentPosition -= numberOfOverflowBytes;
					pCount = numberOfBytes / pSize;
				}
				if (0 != numberOfBytes)
				{
					mFile.read(pvBuffer, numberOfBytes);
					mCurrentPosition = newCurrentPosition;
				}
				return pCount;
			}

			[[nodiscard]] inline virtual size_t Write(const void*, size_t, size_t) override
			{
				ASSERT(false, "We only support read-only Assimp files")
				return 0;
			}

			[[nodiscard]] virtual aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override
			{
				ASSERT(aiOrigin_END != pOrigin, "We don't support \"aiOrigin_END\" in read-only Assimp files")
				if (aiOrigin_SET == pOrigin)
				{
					ASSERT(pOffset >= mCurrentPosition, "We only support unidirectional sequential byte skipping")
					pOffset = pOffset - mCurrentPosition;
				}
				if (0 != pOffset)
				{
					mFile.skip(pOffset);
					mCurrentPosition += pOffset;
				}
				return aiReturn_SUCCESS;
			}

			[[nodiscard]] inline virtual size_t Tell() const override
			{
				return mCurrentPosition;
			}

			[[nodiscard]] inline virtual size_t FileSize() const override
			{
				return mNumberOfBytes;
			}

			inline virtual void Flush() override
			{
				ASSERT(false, "We only support read-only Assimp files")
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			AssimpIOStream(const AssimpIOStream&) = delete;
			AssimpIOStream& operator=(const AssimpIOStream&) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			const Renderer::IFileManager& mFileManager;
			Renderer::IFile&			  mFile;
			size_t						  mNumberOfBytes;
			size_t						  mCurrentPosition;	///< Current position inside the file in bytes


		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Public virtual Assimp::IOSystem methods               ]
	//[-------------------------------------------------------]
	bool AssimpIOSystem::Exists(const char* pFile) const
	{
		return mFileManager.doesFileExist(pFile);
	}

	Assimp::IOStream* AssimpIOSystem::Open(const char* pFile, const char* pMode)
	{
		ASSERT(nullptr != pFile, "Invalid file name")
		ASSERT(nullptr != pMode, "Invalid mode")
		if (stricmp(pMode, "rb") != 0)
		{
			throw std::runtime_error("We only support read-only Assimp files");
		}

		// First check whether or not the file exists, e.g. "Assimp::FileSystemFilter::Open()" tries multiple file variations until a match has been found
		if (mFileManager.doesFileExist(pFile))
		{
			// Open file
			Renderer::IFile* file = mFileManager.openFile(Renderer::IFileManager::FileMode::READ, pFile);
			if (nullptr != file)
			{
				return new ::detail::AssimpIOStream(mFileManager, *file);
			}
		}

		// Failed to open the file
		return nullptr;
	}

	void AssimpIOSystem::Close(Assimp::IOStream* pFile)
	{
		// Lookout: While some Assimp places use a "Assimp::IOSystem::Open()"->"Assimp::IOSystem::Close()" flow, other Assimp places use a "Assimp::IOSystem::Open()"->"Assimp::~IOStream()" flow
		// -> The "Assimp::~IOStream()"-destructor comment states "Destructor. Deleting the object closes the underlying file, alternatively you may use IOSystem::Close() to release the file."
		// -> In here just destroy the Assimp file wrapper object, not the underlying file object
		// -> Sadly, this Assimp file interface usage makes our custom "AssimpIOStream"-implementation a little bit more fat since we also need to store a reference our file manager with each file wrapper object
		// -> Discussed this behaviour at "Leak: "Assimp::ObjFileImporter::InternReadFile()" doesn't close the opened file #1926" - https://github.com/assimp/assimp/issues/1926
		ASSERT(nullptr != pFile, "Invalid file instance")
		delete pFile;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
