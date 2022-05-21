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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Public/Core/StringId.h>
#include <Renderer/Public/Core/GetInvalid.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	#include <string>
	#include <vector>
	#include <unordered_map>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	typedef const char* VirtualFilename;	///< UTF-8 virtual filename, the virtual filename scheme is "<mount point = project name>/<asset directory>/<asset name>.<file extension>" (example "Example/Mesh/Monster/Squirrel.mesh"), never ever a null pointer and always finished by a terminating zero
}
namespace RendererToolkit
{
	class Context;
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
	*    Cache manager for source assets
	*
	*  @note
	*    - This manager caches the content hash of source assets to speed up project compilation when the source doesn't changes
	*/
	class CacheManager final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct CacheEntry final
		{
			// Combined 64 bit key
			uint32_t fileId			  = 0;	///< ID of the file (string hash of the filename)
			uint32_t rhiTargetId	  = 0;	///< ID of the RHI target (string hash of the RHI target)
			// Data
			uint64_t fileHash		  = 0;	///< The 64-bit FNV-1a hash of the file content
			int64_t  fileSize		  = 0;	///< The file size
			int64_t  fileTime		  = 0;	///< The file time (last write time)
			uint32_t compilerVersion  = 0;	///< Compiler version so we can detect compiler version changes and enforce compiling even if the source data has not been changed
			// Method
			[[nodiscard]] inline uint64_t getKey() const
			{
				return ((static_cast<uint64_t>(rhiTargetId) << 32) | static_cast<uint64_t>(fileId));
			}
			[[nodiscard]] static inline uint64_t generateKey(const std::string& rhiTarget, Renderer::StringId fileId)
			{
				return ((static_cast<uint64_t>(Renderer::StringId::calculateFNV(rhiTarget.c_str())) << 32) | static_cast<uint64_t>(fileId.getId()));
			}
		};

		struct CacheEntries final
		{
			std::vector<CacheEntry>	sourceCacheEntries;
			CacheEntry				assetCacheEntry;
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    The renderer toolkit context to use, the renderer toolkit context instance must stay valid as long as the cache manager instance exists
		*  @param[in] projectName
		*    UTF-8 name of the project this cache is for
		*/
		CacheManager(const Context& context, const std::string& projectName);

		/**
		*  @brief
		*    Destructor
		*/
		~CacheManager();

		/**
		*  @brief
		*    Return if an asset needs to be compiled
		*
		*  @param[in] rhiTarget
		*    The UTF-8 RHI target name for which the asset should be compiled
		*  @param[in] virtualAssetFilename
		*    Virtual UTF-8 filename of the file containing the asset metadata
		*  @param[in] virtualSourceFilename
		*    Virtual UTF-8 source filename of the asset
		*  @param[in] virtualDestinationFilename
		*    The virtual UTF-8 filename of the destination file of the asset which contains the compiled data of the source
		*  @param[in] compilerVersion
		*    Compiler version so we can detect compiler version changes and enforce compiling even if the source data has not been changed
		*  @param[out] cacheEntries
		*    Receives information about the cache entries; to be passed into "RendererToolkit::CacheManager::storeOrUpdateCacheEntries()"
		*
		*  @return
		*    "true" if the file needs to be compiled (aka source changed, destination doesn't exists or is yet unknown file) otherwise "false"
		*/
		[[nodiscard]] bool needsToBeCompiled(const std::string& rhiTarget, const std::string& virtualAssetFilename, const std::string& virtualSourceFilename, const std::string& virtualDestinationFilename, uint32_t compilerVersion, CacheEntries& cacheEntries);

		/**
		*  @brief
		*    Return if an asset needs to be compiled
		*
		*  @param[in] rhiTarget
		*    The UTF-8 rhi target name for which the asset should be compiled
		*  @param[in] virtualAssetFilename
		*    Virtual UTF-8 filename of the file containing the asset metadata
		*  @param[in] virtualSourceFilenames
		*    Virtual UTF-8 source filenames of the assets
		*  @param[in] virtualDestinationFilename
		*    The virtual UTF-8 filename of the destination file of the asset which contains the compiled data of the source
		*  @param[in] compilerVersion
		*    Compiler version so we can detect compiler version changes and enforce compiling even if the source data has not been changed
		*  @param[out] cacheEntries
		*    Receives information about the cache entries; to be passed into "RendererToolkit::CacheManager::storeOrUpdateCacheEntries()"
		*
		*  @return
		*    "true" if the file needs to be compiled (aka source changed, destination doesn't exists or is yet unknown file) otherwise "false"
		*/
		[[nodiscard]] bool needsToBeCompiled(const std::string& rhiTarget, const std::string& virtualAssetFilename, const std::vector<std::string>& virtualSourceFilenames, const std::string& virtualDestinationFilename, uint32_t compilerVersion, CacheEntries& cacheEntries);

		/**
		*  @brief
		*    Store new cache entries or update existing ones
		*
		*  @param[in] cacheEntries
		*    The cache entries data to store / update
		*/
		void storeOrUpdateCacheEntries(const CacheEntries& cacheEntries);

		/**
		*  @brief
		*    Return whether or not least one of the given files has been modified since the last check
		*
		*  @param[in] rhiTarget
		*    The UTF-8 rhi target name for which the asset should be compiled
		*  @param[in] virtualAssetFilename
		*    Virtual UTF-8 filename of the file containing the asset metadata
		*  @param[in] virtualSourceFilenames
		*    Virtual UTF-8 source filenames of the assets
		*  @param[in] virtualDestinationFilename
		*    The virtual UTF-8 filename of the destination file of the asset which contains the compiled data of the source
		*  @param[in] compilerVersion
		*    Compiler version so we can detect compiler version changes and enforce compiling even if the source data has not been changed
		*
		*  @return
		*    "true" if any of the files have been modified otherwise "false"
		* 
		*  @note
		*    - This method fills an internal cache which stores the check result in order to speed up "RendererToolkit::CacheManager::needsToBeCompiled()"-calls and support dependency tracking
		*/
		[[nodiscard]] bool checkIfFileIsModified(const std::string& rhiTarget, const std::string& virtualAssetFilename, const std::vector<std::string>& virtualSourceFilenames, const std::string& virtualDestinationFilename, uint32_t compilerVersion);

		/**
		*  @brief
		*    Return whether or not least one of the given files has been modified since the last check
		*
		*  @param[in] virtualDependencyFilenames
		*    Virtual UTF-8 dependency filenames to check
		*
		*  @return
		*    "true" if any of the files have been modified otherwise "false"
		*/
		[[nodiscard]] bool dependencyFilesChanged(const std::vector<std::string>& virtualDependencyFilenames);

		/**
		*  @brief
		*    Clear the internal cache for file changes
		*/
		void clearInternalCache();

		/**
		*  @brief
		*    Save cache
		*/
		void saveCache();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Load cache
		*/
		void loadCache();

		/**
		*  @brief
		*    Fill an cache entry with the stored data, if it exists
		*
		*  @param[in] rhiTarget
		*    The UTF-8 RHI target name for which the asset should be compiled
		*  @param[in] fileId
		*    The file ID (e.g. string hash of the file path) which represents the file to check
		*  @param[out] cacheEntry
		*    The cache entry content, unchanged when nothing found
		*
		*  @return
		*    "true" if a cache entry exists otherwise "false"
		*/
		[[nodiscard]] bool fillEntryForFile(const std::string& rhiTarget, Renderer::StringId fileId, CacheEntry& cacheEntry);

		/**
		*  @brief
		*    Check if file has changed
		*
		*  @param[in] rhiTarget
		*    The UTF-8 RHI target name for which the asset should be compiled
		*  @param[in] virtualFilename
		*    The virtual filename to check
		*  @param[in] compilerVersion
		*    Compiler version so we can detect compiler version changes and enforce compiling even if the source data has not been changed
		*  @param[out] cacheEntry
		*    Receives the cache entry
		*
		*  @return
		*    "true" if the file has changed otherwise "false" (aka the stored hash doesn't equals to the current one or file not yet known)
		*
		*  @note
		*    - When a change was detected the an cache entry is stored/updated
		*/
		[[nodiscard]] bool checkIfFileChanged(const std::string& rhiTarget, Renderer::VirtualFilename virtualFilename, uint32_t compilerVersion, CacheEntry& cacheEntry);

		/**
		*  @brief
		*    Store a new cache entry or update an existing one
		*
		*  @param[in] cacheEntry
		*    The cache entry data to store / update
		*/
		void storeOrUpdateCacheEntry(const CacheEntry& cacheEntry);

		CacheManager(const CacheManager&) = delete;
		CacheManager& operator=(const CacheManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::unordered_map<uint64_t, CacheEntry> StoredCacheEntries;

		struct CheckedFile final
		{
			bool	   changed;
			CacheEntry cacheEntry;
		};
		typedef std::unordered_map<uint32_t, CheckedFile> CheckedFilesStatus;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const Context&	   mContext;
		const std::string  mProjectName;	///< UTF-8 name of the project this cache is for
		StoredCacheEntries mStoredCacheEntries;
		bool			   mDiskCacheDirty;

		// We use here "uint32_t" instead of "Renderer::StringId" because we don't define a "std::hash"-method for "Renderer::StringId", which internal stores an "uint32_t"
		CheckedFilesStatus mCheckedFilesStatus;	///< Holds the status of each file checked via "RendererToolkit::CacheManager::checkIfFileChanged()"


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
