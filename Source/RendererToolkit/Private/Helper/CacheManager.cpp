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
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/AssetCompiler/IAssetCompiler.h"
#include "RendererToolkit/Private/Context.h"

#include <Renderer/Public/IRenderer.h>
#include <Renderer/Public/Core/Math/Math.h>
#include <Renderer/Public/Core/File/MemoryFile.h>
#include <Renderer/Public/Core/File/IFileManager.h>
#include <Renderer/Public/Core/File/FileSystemHelper.h>


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
		namespace RendererToolkitCache
		{
			static constexpr uint32_t FORMAT_TYPE	 = STRING_ID("RendererToolkitCache");
			static constexpr uint32_t FORMAT_VERSION = 1;
		}


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void getRendererToolkitCacheFilename(const Renderer::IFileManager& fileManager, const std::string& projectName, std::string& virtualDirectoryName, std::string& virtualFilename)
		{
			virtualDirectoryName = fileManager.getLocalDataMountPoint();
			virtualFilename = virtualDirectoryName + '/' + projectName + ".renderer_toolkit_cache";
		}

		[[nodiscard]] bool loadRendererToolkitCacheFile(const Renderer::IFileManager& fileManager, const std::string& projectName, Renderer::MemoryFile& memoryFile)
		{
			// Tell the memory mapped file about the LZ4 compressed data and decompress it at once
			std::string virtualDirectoryName;
			std::string virtualFilename;
			getRendererToolkitCacheFilename(fileManager, projectName, virtualDirectoryName, virtualFilename);
			if (fileManager.doesFileExist(virtualFilename.c_str()) && memoryFile.loadLz4CompressedDataByVirtualFilename(RendererToolkitCache::FORMAT_TYPE, RendererToolkitCache::FORMAT_VERSION, fileManager, virtualFilename.c_str()))
			{
				memoryFile.decompress();

				// Done
				return true;
			}
			
			// Failed to load the cache
			// -> No error since the cache might just not exist which is a valid situation
			return false;
		}

		void saveRendererToolkitCacheFile(const RendererToolkit::Context& context, const std::string& projectName, const Renderer::MemoryFile& memoryFile)
		{
			std::string virtualDirectoryName;
			std::string virtualFilename;
			const Renderer::IFileManager& fileManager = context.getFileManager();
			getRendererToolkitCacheFilename(fileManager, projectName, virtualDirectoryName, virtualFilename);
			if (fileManager.createDirectories(virtualDirectoryName.c_str()) && !memoryFile.writeLz4CompressedDataByVirtualFilename(RendererToolkitCache::FORMAT_TYPE, RendererToolkitCache::FORMAT_VERSION, fileManager, virtualFilename.c_str()))
			{
				RHI_LOG(context, CRITICAL, "The renderer toolkit failed to save the cache to \"%s\"", virtualFilename.c_str())
			}
		}


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
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CacheManager::CacheManager(const Context& context, const std::string& projectName) :
		mContext(context),
		mProjectName(projectName),
		mDiskCacheDirty(false)
	{
		loadCache();
	}

	CacheManager::~CacheManager()
	{
		saveCache();
	}

	bool CacheManager::needsToBeCompiled(const std::string& rhiTarget, const std::string& virtualAssetFilename, const std::string& virtualSourceFilename, const std::string& virtualDestinationFilename, uint32_t compilerVersion, CacheEntries& cacheEntries)
	{
		std::vector<std::string> virtualSourceFilenames;
		virtualSourceFilenames.push_back(virtualSourceFilename);
		return needsToBeCompiled(rhiTarget, virtualAssetFilename, virtualSourceFilenames, virtualDestinationFilename, compilerVersion, cacheEntries);
	}

	bool CacheManager::needsToBeCompiled(const std::string& rhiTarget, const std::string& virtualAssetFilename, const std::vector<std::string>& virtualSourceFilenames, const std::string& virtualDestinationFilename, uint32_t compilerVersion, CacheEntries& cacheEntries)
	{
		if (virtualSourceFilenames.empty())
		{
			// No source files given -> nothing to compile
			return false;
		}

		// First check if all source files exists
		const Renderer::IFileManager& fileManager = mContext.getFileManager();
		for (const std::string& virtualSourceFilename : virtualSourceFilenames)
		{
			if (!fileManager.doesFileExist(virtualSourceFilename.c_str()))
			{
				// Error! Source file could not be found.
				throw std::runtime_error("Source file \"" + virtualSourceFilename + "\" doesn't exist");
			}
		}

		// Check if the destination file exists
		const bool destinationExists = fileManager.doesFileExist(virtualDestinationFilename.c_str());

		// Sources exists
		// -> Check if any of the sources has changed
		bool sourceFilesChanged = false;
		for (const std::string& virtualSourceFilename : virtualSourceFilenames)
		{
			cacheEntries.sourceCacheEntries.emplace_back(CacheEntry{});
			if (checkIfFileChanged(rhiTarget, virtualSourceFilename.c_str(), compilerVersion, cacheEntries.sourceCacheEntries.back()))
			{
				// One of the source files has changed
				sourceFilesChanged = true;
			}
		}

		// Check if also the asset file (*.asset) has changed, e.g. compile options has changed
		// -> ".asset"-check for automatically in-memory generated ".asset"-file support
		const bool assetFileChanged = (virtualAssetFilename.find(".asset") != std::string::npos && checkIfFileChanged(rhiTarget, virtualAssetFilename.c_str(), IAssetCompiler::ASSET_FORMAT_VERSION, cacheEntries.assetCacheEntry));
		if (!assetFileChanged && (sourceFilesChanged || !destinationExists))
		{
			// Mark the asset file as changed when asset needs to be compiled and asset file itself didn't changed
			// -> This is needed to get asset dependencies properly checked
			mCheckedFilesStatus[Renderer::StringId::calculateFNV(virtualAssetFilename.c_str())].changed = true;
		}

		// File needs to be compiled either destination doesn't exists, the source data has changed or the asset file has changed
		return (sourceFilesChanged || assetFileChanged || !destinationExists);
	}

	void CacheManager::storeOrUpdateCacheEntries(const CacheEntries& cacheEntries)
	{
		for (const CacheEntry& sourceCacheEntry : cacheEntries.sourceCacheEntries)
		{
			storeOrUpdateCacheEntry(sourceCacheEntry);
		}

		// There must always be an asset metadata file
		storeOrUpdateCacheEntry(cacheEntries.assetCacheEntry);
	}

	bool CacheManager::checkIfFileIsModified(const std::string& rhiTarget, const std::string& virtualAssetFilename, const std::vector<std::string>& virtualSourceFilenames, const std::string& virtualDestinationFilename, uint32_t compilerVersion)
	{
		bool result = false;
		CacheEntry dummyEntry;

		// Check if the destination file exists
		if (mContext.getFileManager().doesFileExist(virtualDestinationFilename.c_str()))
		{
			// Check the source files
			for (const std::string& virtualSourceFilename : virtualSourceFilenames)
			{
				if (checkIfFileChanged(rhiTarget, virtualSourceFilename.c_str(), compilerVersion, dummyEntry))
				{
					result = true;
				}
			}
		
			// Check the asset file
			// -> ".asset"-check for automatically in-memory generated ".asset"-file support
			if (virtualAssetFilename.find(".asset") != std::string::npos && checkIfFileChanged(rhiTarget, virtualAssetFilename.c_str(), IAssetCompiler::ASSET_FORMAT_VERSION, dummyEntry))
			{
				result = true;
			}
			else
			{
				// We don't include this check in the above if to make sure that the function is always called
				if (result)
				{
					// Asset file itself has not changed but the source file so mark the asset file as changed too
					// Dependencies are defined via the asset file and with this change the asset which depends on this asset knows if the referenced asset has changed
					mCheckedFilesStatus[Renderer::StringId::calculateFNV(virtualAssetFilename.c_str())].changed = true;
				}
			}
		}
		else
		{
			// Compiled destination file doesn't exist
			result = true;
		}

		return result;
	}

	bool CacheManager::dependencyFilesChanged(const std::vector<std::string>& virtualDependencyFilenames)
	{
		for (const std::string& virtualDependencyFilename : virtualDependencyFilenames)
		{
			CheckedFilesStatus::const_iterator iterator = mCheckedFilesStatus.find(Renderer::StringId::calculateFNV(virtualDependencyFilename.c_str()));
			if (mCheckedFilesStatus.end() != iterator && iterator->second.changed)
			{
				// Changed
				return true;
			}
		}

		// Not changed
		return false;
	}

	void CacheManager::clearInternalCache()
	{
		mCheckedFilesStatus.clear();
	}

	void CacheManager::saveCache()
	{
		// Do only save the renderer toolkit cache if writing local data is allowed
		if (mDiskCacheDirty && nullptr != mContext.getFileManager().getLocalDataMountPoint())
		{
			const uint32_t numberOfStoredCacheEntries = static_cast<uint32_t>(mStoredCacheEntries.size());
			Renderer::MemoryFile memoryFile(0, sizeof(uint32_t) + numberOfStoredCacheEntries * sizeof(CacheEntry));
			memoryFile.write(&numberOfStoredCacheEntries, sizeof(uint32_t));
			for (const auto& cacheEntryElement : mStoredCacheEntries)
			{
				memoryFile.write(&cacheEntryElement.second, sizeof(CacheEntry));
			}

			// Save file
			::detail::saveRendererToolkitCacheFile(mContext, mProjectName, memoryFile);
			mDiskCacheDirty = false;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void CacheManager::loadCache()
	{
		// Clear cache
		mDiskCacheDirty = false;
		mStoredCacheEntries.clear();
		mCheckedFilesStatus.clear();

		// Load file
		Renderer::MemoryFile memoryFile;
		if (::detail::loadRendererToolkitCacheFile(mContext.getFileManager(), mProjectName, memoryFile))
		{
			uint32_t numberOfStoredCacheEntries = 0;
			memoryFile.read(&numberOfStoredCacheEntries, sizeof(uint32_t));
			mStoredCacheEntries.reserve(numberOfStoredCacheEntries);
			CacheEntry cacheEntry;
			for (uint32_t i = 0; i < numberOfStoredCacheEntries; ++i)
			{
				memoryFile.read(&cacheEntry, sizeof(CacheEntry));
				mStoredCacheEntries.emplace(cacheEntry.getKey(), cacheEntry);
			}
		}
	}

	bool CacheManager::fillEntryForFile(const std::string& rhiTarget, Renderer::StringId fileId, CacheEntry& cacheEntry)
	{
		const StoredCacheEntries::const_iterator iterator = mStoredCacheEntries.find(CacheEntry::generateKey(rhiTarget, fileId));
		if (mStoredCacheEntries.cend() == iterator)
		{
			// No stored cache entry found
			static constexpr CacheEntry defaultCacheEntry;
			cacheEntry = defaultCacheEntry;

			// Error!
			return false;
		}
		else
		{
			// Stored cache entry found
			cacheEntry = iterator->second;

			// Done
			return true;
		}
	}

	bool CacheManager::checkIfFileChanged(const std::string& rhiTarget, Renderer::VirtualFilename virtualFilename, uint32_t compilerVersion, CacheEntry& cacheEntry)
	{
		// Get the last write time
		const Renderer::IFileManager& fileManager = mContext.getFileManager();
		const int64_t fileTime = fileManager.getLastModificationTime(virtualFilename);
		const int64_t fileSize = fileManager.getFileSize(virtualFilename);

		// Get cache entry data if an entry exists
		Renderer::StringId fileId(virtualFilename);
		const bool hasFileEntry = fillEntryForFile(rhiTarget, fileId, cacheEntry);
		if (hasFileEntry)
		{
			// A file might be referenced by different assets so first check if the file was already checked by a previous call to this method
			// If so return the result (the file shouldn't change between two checks while a compilation is running)
			{
				CheckedFilesStatus::const_iterator iterator = mCheckedFilesStatus.find(fileId);
				if (mCheckedFilesStatus.end() != iterator)
				{
					// Copy cache entry data from stored one
					cacheEntry = iterator->second.cacheEntry;

					// The file was already checked before simply return the result
					return iterator->second.changed;
				}
			}

			// First and faster step: Check file size and file time as well as the compiler version (needed so that we also detect compiler version changes here too)
			if (cacheEntry.fileSize == fileSize && cacheEntry.fileTime == fileTime && cacheEntry.compilerVersion == compilerVersion)
			{
				// The file has not changed -> store the result
				CheckedFile& checkedFile = mCheckedFilesStatus[fileId];
				checkedFile.changed = false;
				checkedFile.cacheEntry = cacheEntry;

				// Source file didn't changed
				return false;
			}
			else
			{
				// Current file differs in file size and/or file time do the second step:
				// Check the compiler version and the 64-bit FNV-1a hash
				const uint64_t fileHash = Renderer::Math::calculateFileFNV1a64ByVirtualFilename(fileManager, virtualFilename);
				if (cacheEntry.fileHash == fileHash && cacheEntry.compilerVersion == compilerVersion)
				{
					// Hash of the file and compiler version didn't changed but store the changed file size/time
					cacheEntry.fileSize		   = fileSize;
					cacheEntry.fileTime		   = fileTime;
					cacheEntry.compilerVersion = compilerVersion;
					storeOrUpdateCacheEntry(cacheEntry);

					// The file has not changed -> store the result
					CheckedFile& checkedFile = mCheckedFilesStatus[fileId];
					checkedFile.changed = false;
					checkedFile.cacheEntry = cacheEntry;

					// Source file didn't changed
					return false;
				}
				else
				{
					cacheEntry.fileSize		   = fileSize;
					cacheEntry.fileTime		   = fileTime;
					cacheEntry.fileHash		   = fileHash;
					cacheEntry.compilerVersion = compilerVersion;
					storeOrUpdateCacheEntry(cacheEntry);

					// The file has changed -> store the result
					CheckedFile& checkedFile = mCheckedFilesStatus[fileId];
					checkedFile.changed = true;
					checkedFile.cacheEntry = cacheEntry;
				}
			}
		}
		else
		{
			// No cache entry exists yet: Store the data
			cacheEntry.rhiTargetId		= Renderer::StringId::calculateFNV(rhiTarget.c_str());
			cacheEntry.fileId			= fileId;
			cacheEntry.fileSize			= fileSize;
			cacheEntry.fileTime			= fileTime;
			cacheEntry.fileHash			= Renderer::Math::calculateFileFNV1a64ByVirtualFilename(fileManager, virtualFilename);
			cacheEntry.compilerVersion	= compilerVersion;
			storeOrUpdateCacheEntry(cacheEntry);

			// The file had no cache entry yet -> store it as "has changed"
			CheckedFile& checkedFile = mCheckedFilesStatus[fileId];
			checkedFile.changed = true;
			checkedFile.cacheEntry = cacheEntry;
		}

		// Default file has changed to do not break compilation, if cache doesn't work
		return true;
	}
	
	void CacheManager::storeOrUpdateCacheEntry(const CacheEntry& cacheEntry)
	{
		const uint64_t cacheEntryKey = cacheEntry.getKey();
		const StoredCacheEntries::iterator iterator = mStoredCacheEntries.find(cacheEntryKey);
		if (mStoredCacheEntries.cend() == iterator)
		{
			// Store new cache entry
			mStoredCacheEntries.emplace(cacheEntryKey, cacheEntry);
		}
		else
		{
			// Update existing cache entry
			iterator->second = cacheEntry;
		}
		mDiskCacheDirty = true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
