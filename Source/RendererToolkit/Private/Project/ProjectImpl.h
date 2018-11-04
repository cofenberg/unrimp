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
#include "RendererToolkit/Private/Project/IProject.h"
#include "RendererToolkit/Private/AssetCompiler/IAssetCompiler.h"	// For "RendererToolkit::QualityStrategy"

#include <RendererRuntime/Public/Asset/AssetPackage.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Ptr_base<_Ty>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::_Ptr_base<_Ty>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <thread>
	#include <atomic>	// For "std::atomic<>"
	#include <string_view>
	#include <unordered_set>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IRendererRuntime;
}
namespace RendererToolkit
{
	class Context;
	class CacheManager;
	class ProjectAssetMonitor;
	class RendererToolkitImpl;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef std::unordered_map<uint32_t, uint32_t>	  SourceAssetIdToCompiledAssetId;	///< Key = source asset ID, value = compiled asset ID ("AssetId"-type not used directly or we would need to define a hash-function for it)
	typedef std::unordered_map<uint32_t, uint32_t>	  CompiledAssetIdToSourceAssetId;	///< Key = compiled asset ID, value = source asset ID ("AssetId"-type not used directly or we would need to define a hash-function for it)
	typedef std::unordered_map<uint32_t, std::string> SourceAssetIdToVirtualFilename;	///< Key = source asset ID, virtual asset filename
	typedef std::unordered_set<uint32_t>			  DefaultTextureAssetIds;			///< "RendererRuntime::AssetId"-type for compiled asset IDs


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Project class implementation
	*/
	class ProjectImpl final : public IProject
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rendererToolkitImpl
		*    The renderer toolkit implementation instance to use
		*/
		explicit ProjectImpl(RendererToolkitImpl& rendererToolkitImpl);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ProjectImpl() override;

		[[nodiscard]] inline const Context& getContext() const
		{
			return mContext;
		}

		[[nodiscard]] inline const std::string& getProjectName() const
		{
			return mProjectName;
		}

		[[nodiscard]] inline const std::string& getAbsoluteProjectDirectory() const	// Has no "/" at the end
		{
			return mAbsoluteProjectDirectory;
		}

		[[nodiscard]] inline const RendererRuntime::AssetPackage& getAssetPackage() const
		{
			return mAssetPackage;
		}

		[[nodiscard]] RendererRuntime::VirtualFilename tryGetVirtualFilenameByAssetId(RendererRuntime::AssetId assetId) const;
		[[nodiscard]] bool checkAssetIsChanged(const RendererRuntime::Asset& asset, const char* rendererTarget);
		void compileAsset(const RendererRuntime::Asset& asset, const char* rendererTarget, RendererRuntime::AssetPackage& outputAssetPackage);
		void compileAssetIncludingDependencies(const RendererRuntime::Asset& asset, const char* rendererTarget, RendererRuntime::AssetPackage& outputAssetPackage) noexcept;

		/**
		*  @brief
		*    Inform project about compilation run finish
		*
		*  @note
		*    - Call this after a compilation run has been finish, this will clear any internal caches/states
		*/
		void onCompilationRunFinished();


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IProject methods      ]
	//[-------------------------------------------------------]
	public:
		virtual void load(RendererRuntime::AbsoluteDirectoryName absoluteProjectDirectoryName) override;
		virtual void compileAllAssets(const char* rendererTarget) override;
		virtual void importAssets(const AbsoluteFilenames& absoluteSourceFilenames, const std::string& targetAssetPackageName, const std::string& targetDirectoryName = "Imported") override;
		virtual void startupAssetMonitor(RendererRuntime::IRendererRuntime& rendererRuntime, const char* rendererTarget) override;
		virtual void shutdownAssetMonitor() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		virtual void selfDestruct() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ProjectImpl(const ProjectImpl& source) = delete;
		ProjectImpl& operator =(const ProjectImpl& source) = delete;

		inline bool isInitialized() const
		{
			return !mAssetCompilerByClassId.empty();
		}

		void initialize();
		void clear();
		void readAssetPackageByDirectory(const std::string& directoryName);	// Directory name has no "/" at the end
		void readTargetsByFilename(const std::string& relativeFilename);
		[[nodiscard]] std::string getRenderTargetDataRootDirectory(const char* rendererTarget) const;	// Directory name has no "/" at the end
		void buildSourceAssetIdToCompiledAssetId();
		const IAssetCompiler* getSourceAssetCompilerAndRapidJsonDocument(const std::string& virtualAssetFilename, rapidjson::Document& rapidJsonDocument) const;
		void threadWorker();


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::unordered_map<uint32_t, IAssetCompiler*> AssetCompilerByClassId;
		typedef std::unordered_map<std::string_view, IAssetCompiler*> AssetCompilerByFilenameExtension;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		RendererToolkitImpl&				mRendererToolkitImpl;
		const Context&						mContext;
		std::string							mProjectName;						///< UTF-8 project name
		std::string							mAbsoluteProjectDirectory;			///< UTF-8 project directory, Has no "/" at the end
		QualityStrategy						mQualityStrategy;
		RendererRuntime::AssetPackage		mAssetPackage;
		std::string							mAssetPackageDirectoryName;			///< UTF-8 asset package name, has no "/" at the end
		SourceAssetIdToCompiledAssetId		mSourceAssetIdToCompiledAssetId;
		CompiledAssetIdToSourceAssetId		mCompiledAssetIdToSourceAssetId;
		SourceAssetIdToVirtualFilename		mSourceAssetIdToVirtualFilename;
		DefaultTextureAssetIds				mDefaultTextureAssetIds;
		rapidjson::Document*				mRapidJsonDocument;					///< There's no real benefit in trying to store the targets data in custom data structures, so we just stick to the read in JSON object
		ProjectAssetMonitor*				mProjectAssetMonitor;
		std::atomic<bool>					mShutdownThread;
		std::thread							mThread;
		CacheManager*						mCacheManager;						///< Cache manager, can be a null pointer, destroy the instance if no longer needed
		AssetCompilerByClassId				mAssetCompilerByClassId;			///< List of asset compilers by key "RendererToolkit::AssetCompilerClassId" (type not used directly or we would need to define a hash-function for it)
		AssetCompilerByFilenameExtension	mAssetCompilerByFilenameExtension;	///< List of asset compilers by key "unique asset filename extension"


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
