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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererToolkit/Private/Project/ProjectImpl.h"
#include "RendererToolkit/Private/Project/ProjectAssetMonitor.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/AssetImporter/SketchfabAssetImporter.h"
#include "RendererToolkit/Private/AssetCompiler/MeshAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/SceneAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/TextureAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/MaterialAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/SkeletonAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/ShaderPieceAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/CompositorNodeAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/ShaderBlueprintAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/VertexAttributesAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/SkeletonAnimationAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/MaterialBlueprintAssetCompiler.h"
#include "RendererToolkit/Private/AssetCompiler/CompositorWorkspaceAssetCompiler.h"
#include "RendererToolkit/Private/RendererToolkitImpl.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/RendererRuntimeImpl.h>
#include <RendererRuntime/Public/Core/File/MemoryFile.h>
#include <RendererRuntime/Public/Core/File/IFileManager.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Core/Platform/PlatformManager.h>
#include <RendererRuntime/Public/Asset/Loader/AssetPackageFileFormat.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <rapidjson/document.h>
PRAGMA_WARNING_POP

#include <cassert>
#include <algorithm>
#include <unordered_set>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		bool orderByAssetId(const RendererRuntime::Asset& left, const RendererRuntime::Asset& right)
		{
			return (left.assetId < right.assetId);
		}

		void optionalQualityStrategy(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererToolkit::QualityStrategy& value)
		{
			if (rapidJsonValue.HasMember(propertyName))
			{
				const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
				const char* valueAsString = rapidJsonValueValueType.GetString();
				const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

				// Define helper macros
				#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = RendererToolkit::QualityStrategy::name;
				#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = RendererToolkit::QualityStrategy::name;

				// Evaluate value
				IF_VALUE(DEBUG)
				ELSE_IF_VALUE(PRODUCTION)
				ELSE_IF_VALUE(SHIPPING)
				else
				{
					throw std::runtime_error("Quality strategy must be \"DEBUG\", \"PRODUCTION\" or \"SHIPPING\"");
				}

				// Undefine helper macros
				#undef IF_VALUE
				#undef ELSE_IF_VALUE
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
	ProjectImpl::ProjectImpl(RendererToolkitImpl& rendererToolkitImpl) :
		mRendererToolkitImpl(rendererToolkitImpl),
		mContext(rendererToolkitImpl.getContext()),
		mQualityStrategy(QualityStrategy::PRODUCTION),
		mRapidJsonDocument(nullptr),
		mProjectAssetMonitor(nullptr),
		mShutdownThread(false),
		mCacheManager(nullptr)
	{
		mThread = std::thread(&ProjectImpl::threadWorker, this);

		// Setup asset compilers map
		// TODO(co) Currently this is fixed build in, later on me might want to have this dynamic so we can plugin additional asset compilers
		mAssetCompilers.emplace(TextureAssetCompiler::TYPE_ID, new TextureAssetCompiler(mContext));
		mAssetCompilers.emplace(ShaderPieceAssetCompiler::TYPE_ID, new ShaderPieceAssetCompiler());
		mAssetCompilers.emplace(ShaderBlueprintAssetCompiler::TYPE_ID, new ShaderBlueprintAssetCompiler());
		mAssetCompilers.emplace(MaterialBlueprintAssetCompiler::TYPE_ID, new MaterialBlueprintAssetCompiler());
		mAssetCompilers.emplace(MaterialAssetCompiler::TYPE_ID, new MaterialAssetCompiler());
		mAssetCompilers.emplace(SkeletonAssetCompiler::TYPE_ID, new SkeletonAssetCompiler());
		mAssetCompilers.emplace(SkeletonAnimationAssetCompiler::TYPE_ID, new SkeletonAnimationAssetCompiler());
		mAssetCompilers.emplace(MeshAssetCompiler::TYPE_ID, new MeshAssetCompiler());
		mAssetCompilers.emplace(SceneAssetCompiler::TYPE_ID, new SceneAssetCompiler());
		mAssetCompilers.emplace(CompositorNodeAssetCompiler::TYPE_ID, new CompositorNodeAssetCompiler());
		mAssetCompilers.emplace(CompositorWorkspaceAssetCompiler::TYPE_ID, new CompositorWorkspaceAssetCompiler());
		mAssetCompilers.emplace(VertexAttributesAssetCompiler::TYPE_ID, new VertexAttributesAssetCompiler());

		{ // Gather default texture asset IDs
			RendererRuntime::AssetIds assetIds;
			RendererRuntime::RendererRuntimeImpl::getDefaultTextureAssetIds(assetIds);
			std::copy(assetIds.begin(), assetIds.end(), std::inserter(mDefaultTextureAssetIds, mDefaultTextureAssetIds.end()));
		}
	}

	ProjectImpl::~ProjectImpl()
	{
		// Shutdown worker thread
		mShutdownThread = true;
		mThread.join();

		// Clear
		clear();
		for (const auto& pair : mAssetCompilers)
		{
			delete pair.second;
		}

		// Destroy the cache manager
		delete mCacheManager;
	}

	RendererRuntime::VirtualFilename ProjectImpl::tryGetVirtualFilenameByAssetId(RendererRuntime::AssetId assetId) const
	{
		return mAssetPackage.tryGetVirtualFilenameByAssetId(assetId);
	}

	bool ProjectImpl::checkAssetIsChanged(const RendererRuntime::Asset& asset, const char* rendererTarget)
	{
		const std::string& virtualAssetFilename = asset.virtualFilename;

		// The renderer toolkit is now considered to be busy
		mRendererToolkitImpl.setState(IRendererToolkit::State::BUSY);

		// Parse JSON
		rapidjson::Document rapidJsonDocument;
		JsonHelper::loadDocumentByFilename(mContext.getFileManager(), virtualAssetFilename, "Asset", "1", rapidJsonDocument);

		// Mandatory main sections of the asset
		const rapidjson::Value& rapidJsonValueAsset = rapidJsonDocument["Asset"];
		const rapidjson::Value& rapidJsonValueAssetMetadata = rapidJsonValueAsset["AssetMetadata"];

		// Dispatch asset compiler
		// TODO(co) Add multi-threading support: Add compiler queue which is processed in the background, ensure compiler instances are reused

		// Get the asset input directory and asset output directory
		const std::string virtualAssetPackageInputDirectory = mProjectName + '/' + mAssetPackageDirectoryName;
		const std::string virtualAssetInputDirectory = std_filesystem::path(virtualAssetFilename).parent_path().generic_string();
		const std::string assetType = rapidJsonValueAssetMetadata["AssetType"].GetString();
		const std::string assetCategory = rapidJsonValueAssetMetadata["AssetCategory"].GetString();
		const std::string renderTargetDataRootDirectory = getRenderTargetDataRootDirectory(rendererTarget);
		const std::string virtualAssetOutputDirectory = renderTargetDataRootDirectory + '/' + mProjectName + '/' + mAssetPackageDirectoryName + '/' + assetType + '/' + assetCategory;

		{ // Do we need to mount a directory now? (e.g. "DataPc", "DataMobile" etc.)
			RendererRuntime::IFileManager& fileManager = mContext.getFileManager();
			if (fileManager.getMountPoint(renderTargetDataRootDirectory.c_str()) == nullptr)
			{
				fileManager.mountDirectory((fileManager.getAbsoluteRootDirectory() + '/' + renderTargetDataRootDirectory).c_str(), renderTargetDataRootDirectory.c_str());
			}
		}

		// Asset compiler input
		IAssetCompiler::Input input(mContext, mProjectName, *mCacheManager, virtualAssetPackageInputDirectory, virtualAssetFilename, virtualAssetInputDirectory, virtualAssetOutputDirectory, mSourceAssetIdToCompiledAssetId, mCompiledAssetIdToSourceAssetId, mSourceAssetIdToVirtualFilename, mDefaultTextureAssetIds);

		// Asset compiler configuration
		AssetCompilers::const_iterator iterator = mAssetCompilers.find(AssetCompilerTypeId(assetType.c_str()));
		if (mAssetCompilers.end() != iterator)
		{
			RENDERER_ASSERT(getContext(), nullptr != mRapidJsonDocument, "Invalid renderer toolkit Rapid JSON document")
			const IAssetCompiler::Configuration configuration(rapidJsonDocument, (*mRapidJsonDocument)["Targets"], rendererTarget, mQualityStrategy);
			try
			{
				return iterator->second->checkIfChanged(input, configuration);
			}
			catch (const std::exception&)
			{
				// In case of an "RendererToolkit::IAssetCompiler::checkIfChanged()"-exception, consider the asset as changed
				return true;
			}
		}

		// Not changed
		return false;
	}

	void ProjectImpl::compileAsset(const RendererRuntime::Asset& asset, const char* rendererTarget, RendererRuntime::AssetPackage& outputAssetPackage)
	{
		// The renderer toolkit is now considered to be busy
		mRendererToolkitImpl.setState(IRendererToolkit::State::BUSY);

		// Open the input stream
		const std::string& virtualAssetFilename = asset.virtualFilename;

		// Parse JSON
		rapidjson::Document rapidJsonDocument;
		RendererRuntime::IFileManager& fileManager = mContext.getFileManager();
		JsonHelper::loadDocumentByFilename(fileManager, virtualAssetFilename, "Asset", "1", rapidJsonDocument);

		// Mandatory main sections of the asset
		const rapidjson::Value& rapidJsonValueAsset = rapidJsonDocument["Asset"];
		const rapidjson::Value& rapidJsonValueAssetMetadata = rapidJsonValueAsset["AssetMetadata"];

		// Dispatch asset compiler
		// TODO(co) Add multi-threading support: Add compiler queue which is processed in the background, ensure compiler instances are reused

		// Get the asset input directory and asset output directory
		const std::string virtualAssetPackageInputDirectory = mProjectName + '/' + mAssetPackageDirectoryName;
		const std::string virtualAssetInputDirectory = std_filesystem::path(virtualAssetFilename).parent_path().generic_string();
		const std::string assetType = rapidJsonValueAssetMetadata["AssetType"].GetString();
		const std::string assetCategory = rapidJsonValueAssetMetadata["AssetCategory"].GetString();
		const std::string renderTargetDataRootDirectory = getRenderTargetDataRootDirectory(rendererTarget);
		const std::string virtualAssetOutputDirectory = renderTargetDataRootDirectory + '/' + mProjectName + '/' + mAssetPackageDirectoryName + '/' + assetType + '/' + assetCategory;

		// Ensure that the asset output directory exists, else creating output file streams will fail
		fileManager.createDirectories(virtualAssetOutputDirectory.c_str());

		// Do we need to mount a directory now? (e.g. "DataPc", "DataMobile" etc.)
		if (fileManager.getMountPoint(renderTargetDataRootDirectory.c_str()) == nullptr)
		{
			fileManager.mountDirectory((fileManager.getAbsoluteRootDirectory() + '/' + renderTargetDataRootDirectory).c_str(), renderTargetDataRootDirectory.c_str());
		}

		// Asset compiler input
		IAssetCompiler::Input input(mContext, mProjectName, *mCacheManager, virtualAssetPackageInputDirectory, virtualAssetFilename, virtualAssetInputDirectory, virtualAssetOutputDirectory, mSourceAssetIdToCompiledAssetId, mCompiledAssetIdToSourceAssetId, mSourceAssetIdToVirtualFilename, mDefaultTextureAssetIds);

		// Asset compiler configuration
		RENDERER_ASSERT(getContext(), nullptr != mRapidJsonDocument, "Invalid renderer toolkit Rapid JSON document")
		const IAssetCompiler::Configuration configuration(rapidJsonDocument, (*mRapidJsonDocument)["Targets"], rendererTarget, mQualityStrategy);

		// Asset compiler output
		IAssetCompiler::Output output;
		output.outputAssetPackage = &outputAssetPackage;

		// Evaluate the asset type and continue with the processing in the asset type specific way
		// TODO(co) Currently this is fixed build in, later on me might want to have this dynamic so we can plugin additional asset compilers
		try
		{
			AssetCompilers::const_iterator iterator = mAssetCompilers.find(AssetCompilerTypeId(assetType.c_str()));
			if (mAssetCompilers.end() != iterator)
			{
				iterator->second->compile(input, configuration, output);
			}
			else
			{
				throw std::runtime_error("Asset type \"" + assetType + "\" is unknown");
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Failed to compile asset with filename \"" + std::string(asset.virtualFilename) + "\": " + std::string(e.what()));
		}

		// Save renderer toolkit cache
		mCacheManager->saveCache();
	}

	void ProjectImpl::compileAssetIncludingDependencies(const RendererRuntime::Asset& asset, const char* rendererTarget, RendererRuntime::AssetPackage& outputAssetPackage) noexcept
	{
		// Compile the given asset
		compileAsset(asset, rendererTarget, outputAssetPackage);

		// Compile other assets depending on the given asset, if necessary
		const RendererRuntime::AssetPackage::SortedAssetVector& sortedAssetVector = mAssetPackage.getSortedAssetVector();
		const size_t numberOfAssets = sortedAssetVector.size();
		for (size_t i = 0; i < numberOfAssets; ++i)
		{
			const RendererRuntime::Asset& dependedAsset = sortedAssetVector[i];
			if (checkAssetIsChanged(dependedAsset, rendererTarget) && &dependedAsset != &asset)
			{
				compileAsset(dependedAsset, rendererTarget, outputAssetPackage);
			}
		}
	}

	void ProjectImpl::onCompilationRunFinished()
	{
		// Compilation run finished clear internal cache of cache manager
		mCacheManager->saveCache();
		mCacheManager->clearInternalCache();

		// The renderer toolkit is now considered to be idle
		mRendererToolkitImpl.setState(IRendererToolkit::State::IDLE);
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IProject methods      ]
	//[-------------------------------------------------------]
	void ProjectImpl::load(RendererRuntime::AbsoluteDirectoryName absoluteProjectDirectoryName)
	{
		// The renderer toolkit is now considered to be busy
		mRendererToolkitImpl.setState(IRendererToolkit::State::BUSY);

		// Clear the previous project
		clear();

		{ // Get the project name
			mAbsoluteProjectDirectory = std::string(absoluteProjectDirectoryName);
			const size_t lastSlash = mAbsoluteProjectDirectory.find_last_of("/");
			mProjectName = (std::string::npos != lastSlash) ? mAbsoluteProjectDirectory.substr(lastSlash + 1) : mAbsoluteProjectDirectory;
		}

		// Mount project read-only data source file system directory
		RendererRuntime::IFileManager& fileManager = mContext.getFileManager();
		fileManager.mountDirectory(absoluteProjectDirectoryName, mProjectName.c_str());

		// Parse JSON
		rapidjson::Document rapidJsonDocument;
		JsonHelper::loadDocumentByFilename(fileManager, mProjectName + '/' + mProjectName + ".project", "Project", "1", rapidJsonDocument);

		// Read project metadata
		const rapidjson::Value& rapidJsonValueProject = rapidJsonDocument["Project"];

		{ // Read project data
			RENDERER_LOG(mContext, INFORMATION, "Gather asset from %s...", mAbsoluteProjectDirectory.c_str())
			{ // Asset packages
				const rapidjson::Value& rapidJsonValueAssetPackages = rapidJsonValueProject["AssetPackages"];
				if (rapidJsonValueAssetPackages.Size() > 1)
				{
					throw std::runtime_error("TODO(co) Support for multiple asset packages isn't implemented, yet");
				}
				for (rapidjson::SizeType i = 0; i < rapidJsonValueAssetPackages.Size(); ++i)
				{
					readAssetPackageByDirectory(std::string(rapidJsonValueAssetPackages[i].GetString()));
				}
			}
			readTargetsByFilename(rapidJsonValueProject["TargetsFilename"].GetString());
			::detail::optionalQualityStrategy(rapidJsonValueProject, "QualityStrategy", mQualityStrategy);
			RENDERER_LOG(mContext, INFORMATION, "Found %u assets", mAssetPackage.getSortedAssetVector().size())
		}

		// Setup project folder for cache manager, it will store there its data
		mCacheManager = new CacheManager(mContext, mProjectName);

		// The renderer toolkit is now considered to be idle
		mRendererToolkitImpl.setState(IRendererToolkit::State::IDLE);
	}

	void ProjectImpl::importAssets(const AbsoluteFilenames& absoluteSourceFilenames, const std::string& targetAssetPackageName)
	{
		// Sanity check
		// TODO(co) Add support for multiple asset packages
		if (mAssetPackageDirectoryName != targetAssetPackageName)
		{
			throw std::runtime_error("The asset import target asset package name must be \"" + mAssetPackageDirectoryName + '\"');
		}

		// Import all assets
		const size_t numberOfSourceAssets = absoluteSourceFilenames.size();
		size_t currentSourceAsset = 0;
		RENDERER_LOG(mContext, INFORMATION, "Starting import of %u assets", numberOfSourceAssets)
		for (const std::string& absoluteSourceFilename : absoluteSourceFilenames)
		{
			RENDERER_LOG(mContext, INFORMATION, "Importing asset %u of %u: \"%s\"", currentSourceAsset + 1, absoluteSourceFilenames.size(), absoluteSourceFilename.c_str())
			IAssetImporter::Input input(mContext, mProjectName, "Imported", absoluteSourceFilename, mProjectName + "/Imported/" + std_filesystem::path(absoluteSourceFilename).stem().generic_string());

			// TODO(co) Implement automatic asset importer selection
			SketchfabAssetImporter().import(input);

			++currentSourceAsset;
		}
		RENDERER_LOG(mContext, INFORMATION, "Finished import of %u assets", numberOfSourceAssets)
	}

	void ProjectImpl::compileAllAssets(const char* rendererTarget)
	{
		RendererRuntime::AssetPackage outputAssetPackage;

		// Discover changed assets
		const RendererRuntime::AssetPackage::SortedAssetVector& sortedAssetVector = mAssetPackage.getSortedAssetVector();
		const size_t numberOfAssets = sortedAssetVector.size();
		std::vector<RendererRuntime::AssetId> changedAssetIds;
		changedAssetIds.reserve(numberOfAssets);
		RENDERER_LOG(mContext, INFORMATION, "Checking %u assets for changes", numberOfAssets)
		for (size_t i = 0; i < numberOfAssets; ++i)
		{
			const RendererRuntime::Asset& asset = sortedAssetVector[i];
			if (checkAssetIsChanged(asset, rendererTarget))
			{
				changedAssetIds.push_back(asset.assetId);
			}
		}
		RENDERER_LOG(mContext, INFORMATION, "Found %u changed assets", changedAssetIds.size())

		// Compile all changed assets
		if (!changedAssetIds.empty())
		{
			for (size_t i = 0; i < numberOfAssets; ++i)
			{
				// TODO(co) Only compile assets if a change has been detected
				RENDERER_LOG(mContext, INFORMATION, "Compiling asset %u of %u", i + 1, numberOfAssets)
				const RendererRuntime::Asset& asset = sortedAssetVector[i];
				compileAsset(asset, rendererTarget, outputAssetPackage);

				// Call "RendererRuntime::IRendererRuntime::reloadResourceByAssetId()" directly after an asset has been compiled to see changes as early as possible
				if (nullptr != mProjectAssetMonitor)
				{
					RendererRuntime::AssetId sourceAssetId = asset.assetId;
					if (std::find(changedAssetIds.cbegin(), changedAssetIds.cend(), sourceAssetId) != changedAssetIds.cend())
					{
						SourceAssetIdToCompiledAssetId::const_iterator iterator = mSourceAssetIdToCompiledAssetId.find(sourceAssetId);
						if (iterator == mSourceAssetIdToCompiledAssetId.cend())
						{
							throw std::runtime_error(std::string("Source asset ID ") + std::to_string(sourceAssetId) + " is unknown");
						}
						mProjectAssetMonitor->mRendererRuntime.reloadResourceByAssetId(iterator->second);
					}
				}
			}

			{ // Write runtime asset package
				RendererRuntime::AssetPackage::SortedAssetVector& sortedOutputAssetVector = outputAssetPackage.getWritableSortedAssetVector();
				if (sortedOutputAssetVector.empty())
				{
					throw std::runtime_error("The asset package is empty");
				}
				RendererRuntime::MemoryFile memoryFile(0, 4096);

				// Ensure the asset package is sorted
				std::sort(sortedOutputAssetVector.begin(), sortedOutputAssetVector.end(), ::detail::orderByAssetId);

				{ // Write down the asset package header
					RendererRuntime::v1AssetPackage::AssetPackageHeader assetPackageHeader;
					assetPackageHeader.numberOfAssets = static_cast<uint32_t>(sortedOutputAssetVector.size());
					memoryFile.write(&assetPackageHeader, sizeof(RendererRuntime::v1AssetPackage::AssetPackageHeader));
				}

				// Write down the asset package content in one single burst
				memoryFile.write(sortedOutputAssetVector.data(), sizeof(RendererRuntime::Asset) * sortedOutputAssetVector.size());

				// Write LZ4 compressed output
				memoryFile.writeLz4CompressedDataByVirtualFilename(STRING_ID("AssetPackage"), RendererRuntime::v1AssetPackage::FORMAT_VERSION, mContext.getFileManager(), (getRenderTargetDataRootDirectory(rendererTarget) + '/' + mProjectName + '/' + mAssetPackageDirectoryName + '/' + mAssetPackageDirectoryName + ".assets").c_str());
			}
		}

		// Compilation run finished clear internal caches/states
		onCompilationRunFinished();
	}

	void ProjectImpl::startupAssetMonitor(RendererRuntime::IRendererRuntime& rendererRuntime, const char* rendererTarget)
	{
		if (nullptr == mProjectAssetMonitor)
		{
			mProjectAssetMonitor = new ProjectAssetMonitor(*this, rendererRuntime, rendererTarget);
		}
	}

	void ProjectImpl::shutdownAssetMonitor()
	{
		if (nullptr != mProjectAssetMonitor)
		{
			delete mProjectAssetMonitor;
			mProjectAssetMonitor = nullptr;
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	void ProjectImpl::selfDestruct()
	{
		delete this;
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void ProjectImpl::clear()
	{
		// TODO(co) Add support for file system directory unmounting
		shutdownAssetMonitor();
		mProjectName.clear();
		mQualityStrategy = QualityStrategy::PRODUCTION;
		mAbsoluteProjectDirectory.clear();
		mAssetPackage.clear();
		mAssetPackageDirectoryName.clear();
		mSourceAssetIdToCompiledAssetId.clear();
		mCompiledAssetIdToSourceAssetId.clear();
		mSourceAssetIdToVirtualFilename.clear();
		if (nullptr != mRapidJsonDocument)
		{
			delete mRapidJsonDocument;
			mRapidJsonDocument = nullptr;
		}
	}

	void ProjectImpl::readAssetPackageByDirectory(const std::string& directoryName)
	{
		// Get the asset package name
		mAssetPackageDirectoryName = directoryName;

		// Mount project read-only data source file system directory
		RendererRuntime::IFileManager& fileManager = mContext.getFileManager();
		fileManager.mountDirectory((mAbsoluteProjectDirectory + '/' + mAssetPackageDirectoryName).c_str(), mProjectName.c_str());

		// Discover assets
		RendererRuntime::AssetPackage::SortedAssetVector& sortedAssetVector = mAssetPackage.getWritableSortedAssetVector();
		std::vector<std::string> virtualFilenames;
		fileManager.enumerateFiles((mProjectName + '/' + mAssetPackageDirectoryName).c_str(), RendererRuntime::IFileManager::EnumerationMode::FILES, virtualFilenames);
		for (const std::string& virtualFilename : virtualFilenames)
		{
			if (StringHelper::isSourceAssetIdAsString(virtualFilename))
			{
				// Sanity check
				if (virtualFilename.length() >= RendererRuntime::Asset::MAXIMUM_ASSET_FILENAME_LENGTH)
				{
					const std::string message = "Asset filename \"" + virtualFilename + "\" is too long. Maximum allowed asset filename number of bytes is " + std::to_string(RendererRuntime::Asset::MAXIMUM_ASSET_FILENAME_LENGTH - 1);	// -1 for not including the terminating zero
					throw std::runtime_error(message);
				}

				// Copy asset data
				RendererRuntime::Asset asset;
				asset.assetId = RendererRuntime::StringId(virtualFilename.c_str());
				RendererRuntime::setInvalid(asset.fileHash);
				strcpy(asset.virtualFilename, virtualFilename.c_str());
				sortedAssetVector.push_back(asset);
			}
		}
		std::sort(sortedAssetVector.begin(), sortedAssetVector.end(), ::detail::orderByAssetId);

		// Build the source asset ID to compiled asset ID map
		buildSourceAssetIdToCompiledAssetId();
	}

	void ProjectImpl::readTargetsByFilename(const std::string& relativeFilename)
	{
		// Parse JSON
		if (nullptr == mRapidJsonDocument)
		{
			mRapidJsonDocument = new rapidjson::Document();
		}
		JsonHelper::loadDocumentByFilename(mContext.getFileManager(), mProjectName + '/' + relativeFilename, "Targets", "1", *mRapidJsonDocument);
	}

	std::string ProjectImpl::getRenderTargetDataRootDirectory(const char* rendererTarget) const
	{
		RENDERER_ASSERT(getContext(), nullptr != mRapidJsonDocument, "Invalid renderer toolkit Rapid JSON document")
		const rapidjson::Value& rapidJsonValueRendererTargets = (*mRapidJsonDocument)["Targets"]["RendererTargets"];
		const rapidjson::Value& rapidJsonValueRendererTarget = rapidJsonValueRendererTargets[rendererTarget];
		return "Data" + std::string(rapidJsonValueRendererTarget["Platform"].GetString());
	}

	void ProjectImpl::buildSourceAssetIdToCompiledAssetId()
	{
		RENDERER_ASSERT(getContext(), 0 == mSourceAssetIdToCompiledAssetId.size(), "Renderer toolkit source asset ID to compiled asset ID should be empty at this point in time")
		RENDERER_ASSERT(getContext(), 0 == mSourceAssetIdToVirtualFilename.size(), "Renderer toolkit source asset ID to virtual filename should be empty at this point in time")

		const RendererRuntime::AssetPackage::SortedAssetVector& sortedAssetVector = mAssetPackage.getSortedAssetVector();
		const size_t numberOfAssets = sortedAssetVector.size();
		for (size_t i = 0; i < numberOfAssets; ++i)
		{
			const RendererRuntime::Asset& asset = sortedAssetVector[i];

			// Parse JSON
			rapidjson::Document rapidJsonDocument;
			const std::string& virtualFilename = asset.virtualFilename;
			JsonHelper::loadDocumentByFilename(mContext.getFileManager(), virtualFilename, "Asset", "1", rapidJsonDocument);

			// Mandatory main sections of the asset
			const rapidjson::Value& rapidJsonValueAsset = rapidJsonDocument["Asset"];
			const rapidjson::Value& rapidJsonValueAssetMetadata = rapidJsonValueAsset["AssetMetadata"];

			// Get the relevant asset metadata parts
			const std::string assetCategory = rapidJsonValueAssetMetadata["AssetCategory"].GetString();
			const std::string assetType = rapidJsonValueAssetMetadata["AssetType"].GetString();
			const std::string assetName = std_filesystem::path(virtualFilename).stem().generic_string();

			// Construct the asset ID as string
			const std::string compiledAssetIdAsString = mProjectName + '/' + assetType + '/' + assetCategory + '/' + assetName;

			// Hash the asset ID and put it into the map
			const uint32_t compiledAssetId = RendererRuntime::StringId::calculateFNV(compiledAssetIdAsString.c_str());
			mSourceAssetIdToCompiledAssetId.emplace(asset.assetId, compiledAssetId);
			mCompiledAssetIdToSourceAssetId.emplace(compiledAssetId, asset.assetId);
			mSourceAssetIdToVirtualFilename.emplace(asset.assetId, virtualFilename);
		}
	}

	void ProjectImpl::threadWorker()
	{
		RendererRuntime::PlatformManager::setCurrentThreadName("Project worker", "Renderer toolkit: Project worker");

		while (!mShutdownThread)
		{
			// TODO(co) Implement me
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
