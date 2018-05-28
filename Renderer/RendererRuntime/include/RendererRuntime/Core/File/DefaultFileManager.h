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
#include "RendererRuntime/Core/Platform/PlatformTypes.h"
#include "RendererRuntime/Core/File/IFileManager.h"
#include "RendererRuntime/Core/File/IFile.h"
#include "RendererRuntime/Core/File/FileSystemHelper.h"

#include <Renderer/Renderer.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <string>
	#include <fstream>
	#include <cassert>
	#include <unordered_map>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ILog;
	class IAssert;
	class IAllocator;
}


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
		static constexpr const char* DEFAULT_LOCAL_DATA_MOUNT_POINT = "LocalData";


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		class DefaultFile : public RendererRuntime::IFile
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline DefaultFile()
			{
				// Nothing here
			}

			inline virtual ~DefaultFile() override
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Public virtual DefaultFile methods                    ]
		//[-------------------------------------------------------]
		public:
			virtual bool isInvalid() const = 0;


		//[-------------------------------------------------------]
		//[ Protected methods                                     ]
		//[-------------------------------------------------------]
		protected:
			explicit DefaultFile(const DefaultFile&) = delete;
			DefaultFile& operator=(const DefaultFile&) = delete;


		};

		class DefaultReadFile final : public DefaultFile
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline explicit DefaultReadFile(const std::string& absoluteFilename) :
				mFileStream(std_filesystem::u8path(absoluteFilename), std::ios::binary)
			{
				assert(mFileStream && "Failed to open default file for reading");
			}

			inline virtual ~DefaultReadFile() override
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Public virtual DefaultFile methods                    ]
		//[-------------------------------------------------------]
		public:
			inline virtual bool isInvalid() const override
			{
				return !mFileStream;
			}


		//[-------------------------------------------------------]
		//[ Public virtual RendererRuntime::IFile methods         ]
		//[-------------------------------------------------------]
		public:
			inline virtual size_t getNumberOfBytes() override
			{
				assert(mFileStream && "Invalid default file access");
				size_t numberOfBytes = 0;
				mFileStream.seekg(0, std::istream::end);
				numberOfBytes = static_cast<size_t>(mFileStream.tellg());
				mFileStream.seekg(0, std::istream::beg);
				return numberOfBytes;
			}

			inline virtual void read(void* destinationBuffer, size_t numberOfBytes) override
			{
				assert((nullptr != destinationBuffer) && "Letting a file read into a null destination buffer is not allowed");
				assert((0 != numberOfBytes) && "Letting a file read zero bytes is not allowed");
				assert(mFileStream && "Invalid default file access");
				mFileStream.read(reinterpret_cast<char*>(destinationBuffer), static_cast<std::streamsize>(numberOfBytes));
			}

			inline virtual void skip(size_t numberOfBytes) override
			{
				assert((0 != numberOfBytes) && "Letting a file skip zero bytes is not allowed");
				assert(mFileStream && "Invalid default file access");
				mFileStream.ignore(static_cast<std::streamsize>(numberOfBytes));
			}

			inline virtual void write(MAYBE_UNUSED const void* sourceBuffer, MAYBE_UNUSED size_t numberOfBytes) override
			{
				assert((nullptr != sourceBuffer) && "Letting a file write from a null source buffer is not allowed");
				assert((0 != numberOfBytes) && "Letting a file write zero bytes is not allowed");
				assert(mFileStream && "Invalid default file access");
				assert(false && "File write method not supported by the default implementation");
			}


		//[-------------------------------------------------------]
		//[ Protected methods                                     ]
		//[-------------------------------------------------------]
		protected:
			explicit DefaultReadFile(const DefaultReadFile&) = delete;
			DefaultReadFile& operator=(const DefaultReadFile&) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			std::ifstream mFileStream;


		};

		class DefaultWriteFile final : public DefaultFile
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline explicit DefaultWriteFile(const std::string& absoluteFilename) :
				mFileStream(std_filesystem::u8path(absoluteFilename), std::ios::binary)
			{
				assert(mFileStream && "Failed to open default file for writing");
			}

			inline virtual ~DefaultWriteFile() override
			{
				// Nothing here
			}


		//[-------------------------------------------------------]
		//[ Public virtual DefaultFile methods                    ]
		//[-------------------------------------------------------]
		public:
			inline virtual bool isInvalid() const override
			{
				return !mFileStream;
			}


		//[-------------------------------------------------------]
		//[ Public virtual RendererRuntime::IFile methods         ]
		//[-------------------------------------------------------]
		public:
			inline virtual size_t getNumberOfBytes() override
			{
				assert(mFileStream && "Invalid default file access");
				assert(false && "File get number of bytes method not supported by the default implementation");
				return 0;
			}

			inline virtual void read(MAYBE_UNUSED void* destinationBuffer, MAYBE_UNUSED size_t numberOfBytes) override
			{
				assert((nullptr != destinationBuffer) && "Letting a file read into a null destination buffer is not allowed");
				assert((0 != numberOfBytes) && "Letting a file read zero bytes is not allowed");
				assert(mFileStream && "Invalid default file access");
				assert(false && "File read method not supported by the default implementation");
			}

			inline virtual void skip(MAYBE_UNUSED size_t numberOfBytes) override
			{
				assert((0 != numberOfBytes) && "Letting a file skip zero bytes is not allowed");
				assert(mFileStream && "Invalid default file access");
				assert(false && "File skip method not supported by the default implementation");
			}

			inline virtual void write(const void* sourceBuffer, size_t numberOfBytes) override
			{
				assert((nullptr != sourceBuffer) && "Letting a file write from a null source buffer is not allowed");
				assert((0 != numberOfBytes) && "Letting a file write zero bytes is not allowed");
				assert(mFileStream && "Invalid default file access");
				mFileStream.write(reinterpret_cast<const char*>(sourceBuffer), static_cast<std::streamsize>(numberOfBytes));
			}


		//[-------------------------------------------------------]
		//[ Protected methods                                     ]
		//[-------------------------------------------------------]
		protected:
			explicit DefaultWriteFile(const DefaultWriteFile&) = delete;
			DefaultWriteFile& operator=(const DefaultWriteFile&) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			std::ofstream mFileStream;


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
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Default file manager implementation class one can use
	*
	*  @note
	*    - Designed to be instanced and used inside a single C++ file
	*    - Primarily for renderer toolkit with more relaxed write access
	*/
	class DefaultFileManager final : public IFileManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline DefaultFileManager(Renderer::ILog& log, Renderer::IAssert& assert, Renderer::IAllocator& allocator, const std::string& absoluteRootDirectory) :
			IFileManager(absoluteRootDirectory),
			mLog(log),
			mAssert(assert),
			mAllocator(allocator)
		{
			// Setup local data mount point
			mAbsoluteBaseDirectory.push_back(absoluteRootDirectory.c_str());
			createDirectories(::detail::DEFAULT_LOCAL_DATA_MOUNT_POINT);
			mountDirectory((absoluteRootDirectory + '/' + ::detail::DEFAULT_LOCAL_DATA_MOUNT_POINT).c_str(), ::detail::DEFAULT_LOCAL_DATA_MOUNT_POINT);
		}

		inline virtual ~DefaultFileManager() override
		{
			#ifdef _DEBUG
				assert((0 == mNumberOfCurrentlyOpenedFiles) && "File leak detected, not all opened files were closed");
			#endif
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IFileManager methods  ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getLocalDataMountPoint() const override
		{
			return ::detail::DEFAULT_LOCAL_DATA_MOUNT_POINT;
		}

		inline virtual bool mountDirectory(AbsoluteDirectoryName absoluteDirectoryName, const char* mountPoint, bool appendToPath = false) override
		{
			// Sanity check
			assert(nullptr != absoluteDirectoryName);
			assert(nullptr != mountPoint);
			#ifdef _DEBUG
				// Additional sanity check: The same absolute directory name shouldn't be added to too different mount points
				for (const auto& pair : mMountedDirectories)
				{
					if (pair.first != mountPoint)
					{
						const AbsoluteDirectoryNames& absoluteDirectoryNames = pair.second;
						assert((absoluteDirectoryNames.cend() == std::find(absoluteDirectoryNames.begin(), absoluteDirectoryNames.end(), absoluteDirectoryName)) && "The same absolute directory name shouldn't be added to too different default mount points");
					}
				}
			#endif

			// Mount directory
			MountedDirectories::iterator mountedDirectoriesIterator = mMountedDirectories.find(mountPoint);
			if (mMountedDirectories.cend() == mountedDirectoriesIterator)
			{
				// The mount point is unknown so far, register it
				mMountedDirectories.emplace(mountPoint, AbsoluteDirectoryNames{absoluteDirectoryName});
			}
			else
			{
				// The mount point is already known, append or prepend?
				AbsoluteDirectoryNames& absoluteDirectoryNames = mountedDirectoriesIterator->second;
				AbsoluteDirectoryNames::const_iterator absoluteDirectoryNamesIterator = std::find(absoluteDirectoryNames.begin(), absoluteDirectoryNames.end(), absoluteDirectoryName);
				if (absoluteDirectoryNames.cend() == absoluteDirectoryNamesIterator)
				{
					if (appendToPath)
					{
						// Append
						absoluteDirectoryNames.push_back(absoluteDirectoryName);
					}
					else
					{
						// Prepend
						absoluteDirectoryNames.insert(absoluteDirectoryNames.begin(), absoluteDirectoryName);
					}
				}
				else
				{
					assert(false && "Duplicate absolute default directory name detected, this situation should be avoided by the caller");
				}
			}

			// Done
			return true;
		}

		inline virtual bool doesFileExist(VirtualFilename virtualFilename) const override
		{
			return !mapVirtualToAbsoluteFilename(FileMode::READ, virtualFilename).empty();
		}

		inline virtual void enumerateFiles(VirtualDirectoryName virtualDirectoryName, EnumerationMode enumerationMode, std::vector<std::string>& virtualFilenames) const override
		{
			std::string mountPoint;
			const std::string absoluteDirectoryName = mapVirtualToAbsoluteFilenameAndMountPoint(FileMode::READ, virtualDirectoryName, mountPoint);
			if (!absoluteDirectoryName.empty())
			{
				const size_t numberOfSkippedBytes = absoluteDirectoryName.length() + 1;	// +1 for '/'-slash at the end of the absolute directory name
				switch (enumerationMode)
				{
					case EnumerationMode::ALL:
						for (const std_filesystem::directory_entry& iterator: std_filesystem::recursive_directory_iterator(absoluteDirectoryName))
						{
							virtualFilenames.push_back(mountPoint + '/' + iterator.path().generic_string().erase(0, numberOfSkippedBytes));
						}
						break;

					case EnumerationMode::FILES:
						for (const std_filesystem::directory_entry& iterator: std_filesystem::recursive_directory_iterator(absoluteDirectoryName))
						{
							if (std_filesystem::is_regular_file(iterator))
							{
								virtualFilenames.push_back(mountPoint + '/' + iterator.path().generic_string().erase(0, numberOfSkippedBytes));
							}
						}
						break;

					case EnumerationMode::DIRECTORIES:
						for (const std_filesystem::directory_entry& iterator: std_filesystem::recursive_directory_iterator(absoluteDirectoryName))
						{
							if (std_filesystem::is_directory(iterator))
							{
								virtualFilenames.push_back(mountPoint + '/' + iterator.path().generic_string().erase(0, numberOfSkippedBytes));
							}
						}
						break;
				}
			}
		}

		inline virtual std::string mapVirtualToAbsoluteFilename(FileMode fileMode, VirtualFilename virtualFilename) const override
		{
			std::string mountPoint;
			return mapVirtualToAbsoluteFilenameAndMountPoint(fileMode, virtualFilename, mountPoint);
		}

		inline virtual int64_t getLastModificationTime(VirtualFilename virtualFilename) const override
		{
			const std::string absoluteFilename = mapVirtualToAbsoluteFilename(FileMode::READ, virtualFilename);
			if (!absoluteFilename.empty())
			{
				const std_filesystem::file_time_type lastWriteTime = std_filesystem::last_write_time(std_filesystem::u8path(absoluteFilename));
				return static_cast<int64_t>(decltype(lastWriteTime)::clock::to_time_t(lastWriteTime));
			}

			// Error!
			return -1;
		}

		inline virtual int64_t getFileSize(VirtualFilename virtualFilename) const override
		{
			const std::string absoluteFilename = mapVirtualToAbsoluteFilename(FileMode::READ, virtualFilename);
			return absoluteFilename.empty() ? -1 : static_cast<int64_t>(std_filesystem::file_size(std_filesystem::u8path(absoluteFilename)));
		}

		inline virtual bool createDirectories(VirtualDirectoryName virtualDirectoryName) const override
		{
			// Sanity check
			assert(nullptr != virtualDirectoryName);

			// Create directories
			const AbsoluteDirectoryNames* absoluteDirectoryNames = nullptr;
			std::string relativeFilename;
			std::string mountPoint;
			if (getAbsoluteDirectoryNamesByMountPoint(virtualDirectoryName, &absoluteDirectoryNames, relativeFilename, mountPoint) && absoluteDirectoryNames != nullptr && !absoluteDirectoryNames->empty())
			{
				// Do only care about the first hit mount point
				const std_filesystem::path absoluteFilename = std_filesystem::u8path(absoluteDirectoryNames->at(0) + '/' + relativeFilename);
				if (!std_filesystem::exists(absoluteFilename) && !std_filesystem::create_directories(absoluteFilename))
				{
					// Failed to create the directories
					return false;
				}
			}

			// Directories have been created successfully
			return true;
		}

		inline virtual IFile* openFile(FileMode fileMode, VirtualFilename virtualFilename) const override
		{
			::detail::DefaultFile* file = nullptr;
			const std::string absoluteFilename = mapVirtualToAbsoluteFilename(fileMode, virtualFilename);
			if (!absoluteFilename.empty())
			{
				if (FileMode::READ == fileMode)
				{
					file = new ::detail::DefaultReadFile(absoluteFilename);
				}
				else
				{
					file = new ::detail::DefaultWriteFile(absoluteFilename);
				}
				if (file->isInvalid())
				{
					if (mLog.print(Renderer::ILog::Type::CRITICAL, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to open file %s", virtualFilename))
					{
						DEBUG_BREAK;
					}
					delete file;
					file = nullptr;
				}
				else
				{
					#ifdef _DEBUG
						++mNumberOfCurrentlyOpenedFiles;
						assert((mNumberOfCurrentlyOpenedFiles < 256) && "Too many simultaneously opened files. The default limit on Microsoft Windows is 512 (can be changed via _setmaxstdio()) and on Mac OS X 256.");
					#endif
				}
			}

			// Done
			return file;
		}

		inline virtual void closeFile(IFile& file) const override
		{
			#ifdef _DEBUG
				--mNumberOfCurrentlyOpenedFiles;
				assert((mNumberOfCurrentlyOpenedFiles >= 0) && "Error, more files closed as opened");
			#endif
			delete static_cast< ::detail::DefaultFile*>(&file);
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit DefaultFileManager(const DefaultFileManager&) = delete;
		DefaultFileManager& operator=(const DefaultFileManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<std::string>								AbsoluteDirectoryNames;
		typedef std::unordered_map<std::string, AbsoluteDirectoryNames>	MountedDirectories;	///< Key = UTF-8 mount point name (example: "MyProject"), value = absolute UTF-8 names of the mounted directories (example: "c:/MyProject")


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline bool getAbsoluteDirectoryNamesByMountPoint(VirtualFilename virtualFilename, const AbsoluteDirectoryNames** absoluteDirectoryNames, std::string& relativeFilename, std::string& mountPoint) const
		{
			assert(nullptr != absoluteDirectoryNames);

			// Get mount point
			const std::string_view stdVirtualFilename = virtualFilename;
			const size_t slashIndex = stdVirtualFilename.find("/");
			if (std::string::npos != slashIndex)
			{
				mountPoint = stdVirtualFilename.substr(0, slashIndex);
				MountedDirectories::const_iterator iterator = mMountedDirectories.find(mountPoint);
				if (mMountedDirectories.cend() != iterator)
				{
					*absoluteDirectoryNames = &iterator->second;
					relativeFilename = stdVirtualFilename.substr(slashIndex + 1);

					// Done
					return true;
				}
				else
				{
					// Error!
					return false;
				}
			}
			else
			{
				// Use base directory
				*absoluteDirectoryNames = &mAbsoluteBaseDirectory;
				relativeFilename = virtualFilename;
				mountPoint.clear();

				// Done
				return true;
			}
		}

		inline std::string mapVirtualToAbsoluteFilenameAndMountPoint(FileMode fileMode, VirtualFilename virtualFilename, std::string& mountPoint) const
		{
			// Sanity check
			assert(nullptr != virtualFilename);

			// Get absolute directory names
			const AbsoluteDirectoryNames* absoluteDirectoryNames = nullptr;
			std::string relativeFilename;
			if (getAbsoluteDirectoryNamesByMountPoint(virtualFilename, &absoluteDirectoryNames, relativeFilename, mountPoint) && absoluteDirectoryNames != nullptr)
			{
				if (mountPoint.empty())
				{
					// Support for absolute filenames
					if (std_filesystem::exists(std_filesystem::u8path(virtualFilename)))
					{
						return virtualFilename;
					}
				}
				else
				{
					for (const std::string& absoluteDirectoryName : *absoluteDirectoryNames)
					{
						const std::string absoluteFilename = FileSystemHelper::lexicallyNormal(absoluteDirectoryName + '/' + relativeFilename).generic_string();
						if (std_filesystem::exists(std_filesystem::u8path(absoluteFilename)))
						{
							return absoluteFilename;
						}
					}

					// Still here and writing a file?
					if (FileMode::WRITE == fileMode && !absoluteDirectoryNames->empty())
					{
						return FileSystemHelper::lexicallyNormal((*absoluteDirectoryNames)[0] + '/' + relativeFilename).generic_string();
					}
				}
			}

			// Error!
			return "";
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::ILog&		   mLog;
		Renderer::IAssert&	   mAssert;
		Renderer::IAllocator&  mAllocator;
		AbsoluteDirectoryNames mAbsoluteBaseDirectory;	///< Absolute UTF-8 base directory, without "/" at the end
		MountedDirectories	   mMountedDirectories;
		#ifdef _DEBUG
			mutable int mNumberOfCurrentlyOpenedFiles = 0;	///< For leak detection
		#endif


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
