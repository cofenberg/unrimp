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
#include <RendererRuntime/Public/Core/Math/Math.h>
#include <RendererRuntime/Public/Core/File/MemoryFile.h>
#include <RendererRuntime/Public/Core/File/IFileManager.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Core/Platform/PlatformManager.h>
#include <RendererRuntime/Public/Asset/AssetPackage.h>
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

		void outputAsset(const RendererRuntime::IFileManager& fileManager, const std::string& assetIdAsString, const std::string& virtualOutputAssetFilename, RendererRuntime::AssetPackage& outputAssetPackage)
		{
			// Sanity check
			const std::string virtualFilename = assetIdAsString + std_filesystem::path(virtualOutputAssetFilename).extension().generic_string();
			if (virtualFilename.size() >= RendererRuntime::Asset::MAXIMUM_ASSET_FILENAME_LENGTH)
			{
				throw std::runtime_error("The output asset filename \"" + virtualFilename + "\" exceeds the length limit of " + std::to_string(RendererRuntime::Asset::MAXIMUM_ASSET_FILENAME_LENGTH - 1));	// -1 for not including terminating zero
			}

			// Append or update asset
			RendererRuntime::Asset outputAsset;
			outputAsset.assetId = RendererRuntime::AssetId(assetIdAsString.c_str());
			outputAsset.fileHash = RendererRuntime::Math::calculateFileFNV1a64ByVirtualFilename(fileManager, virtualOutputAssetFilename.c_str());
			RendererRuntime::Asset* asset = outputAssetPackage.tryGetWritableAssetByAssetId(outputAsset.assetId);
			if (nullptr != asset)
			{
				// Update asset, the file hash or virtual filename might have been changed
				asset->fileHash = outputAsset.fileHash;
				strcpy(asset->virtualFilename, virtualFilename.c_str());
			}
			else
			{
				// Append asset
				strcpy(outputAsset.virtualFilename, virtualFilename.c_str());
				outputAssetPackage.getWritableSortedAssetVector().push_back(outputAsset);
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
		mAssetCompilersByClassId.emplace(TextureAssetCompiler::CLASS_ID, new TextureAssetCompiler(mContext));
		mAssetCompilersByClassId.emplace(ShaderPieceAssetCompiler::CLASS_ID, new ShaderPieceAssetCompiler());
		mAssetCompilersByClassId.emplace(ShaderBlueprintAssetCompiler::CLASS_ID, new ShaderBlueprintAssetCompiler());
		mAssetCompilersByClassId.emplace(MaterialBlueprintAssetCompiler::CLASS_ID, new MaterialBlueprintAssetCompiler());
		mAssetCompilersByClassId.emplace(MaterialAssetCompiler::CLASS_ID, new MaterialAssetCompiler());
		mAssetCompilersByClassId.emplace(SkeletonAssetCompiler::CLASS_ID, new SkeletonAssetCompiler());
		mAssetCompilersByClassId.emplace(SkeletonAnimationAssetCompiler::CLASS_ID, new SkeletonAnimationAssetCompiler());
		mAssetCompilersByClassId.emplace(MeshAssetCompiler::CLASS_ID, new MeshAssetCompiler());
		mAssetCompilersByClassId.emplace(SceneAssetCompiler::CLASS_ID, new SceneAssetCompiler());
		mAssetCompilersByClassId.emplace(CompositorNodeAssetCompiler::CLASS_ID, new CompositorNodeAssetCompiler());
		mAssetCompilersByClassId.emplace(CompositorWorkspaceAssetCompiler::CLASS_ID, new CompositorWorkspaceAssetCompiler());
		mAssetCompilersByClassId.emplace(VertexAttributesAssetCompiler::CLASS_ID, new VertexAttributesAssetCompiler());

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
		for (const auto& pair : mAssetCompilersByClassId)
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

		try
		{
			// The renderer toolkit is now considered to be busy
			mRendererToolkitImpl.setState(IRendererToolkit::State::BUSY);

			// Parse JSON
			rapidjson::Document rapidJsonDocument;
			JsonHelper::loadDocumentByFilename(mContext.getFileManager(), virtualAssetFilename, "Asset", "1", rapidJsonDocument);

			// Dispatch asset compiler
			// TODO(co) Add multi-threading support: Add compiler queue which is processed in the background, ensure compiler instances are reused

			// Get the asset input directory and asset output directory
			const std::string virtualAssetPackageInputDirectory = mProjectName + '/' + mAssetPackageDirectoryName;
			const std::string virtualAssetInputDirectory = std_filesystem::path(virtualAssetFilename).parent_path().generic_string();
			const std::string assetDirectory = virtualAssetInputDirectory.substr(virtualAssetInputDirectory.find('/') + 1);
			const std::string compilerClassName = rapidJsonDocument["Asset"]["Compiler"]["ClassName"].GetString();
			const std::string renderTargetDataRootDirectory = getRenderTargetDataRootDirectory(rendererTarget);
			const std::string virtualAssetOutputDirectory = renderTargetDataRootDirectory + '/' + mProjectName + '/' + mAssetPackageDirectoryName + '/' + assetDirectory;

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
			AssetCompilers::const_iterator iterator = mAssetCompilersByClassId.find(AssetCompilerClassId(compilerClassName.c_str()));
			if (mAssetCompilersByClassId.end() != iterator)
			{
				RENDERER_ASSERT(getContext(), nullptr != mRapidJsonDocument, "Invalid renderer toolkit Rapid JSON document")
				const IAssetCompiler::Configuration configuration(rapidJsonDocument, (*mRapidJsonDocument)["Targets"], rendererTarget, mQualityStrategy);
				return iterator->second->checkIfChanged(input, configuration);
			}
		}
		catch (const std::exception& e)
		{
			// In case of an "RendererToolkit::IAssetCompiler::checkIfChanged()"-exception, consider the asset as changed and write at least an informative log message
			RENDERER_LOG(mContext, INFORMATION, "Failed to check asset with filename \"%s\" for change: \"%s\". Considered the asset as changed.", asset.virtualFilename, e.what())
			return true;
		}

		// Not changed
		return false;
	}

	void ProjectImpl::compileAsset(const RendererRuntime::Asset& asset, const char* rendererTarget, RendererRuntime::AssetPackage& outputAssetPackage)
	{
		try
		{
			// The renderer toolkit is now considered to be busy
			mRendererToolkitImpl.setState(IRendererToolkit::State::BUSY);

			// Open the input stream
			const std::string& virtualAssetFilename = asset.virtualFilename;

			// Parse JSON
			rapidjson::Document rapidJsonDocument;
			RendererRuntime::IFileManager& fileManager = mContext.getFileManager();
			JsonHelper::loadDocumentByFilename(fileManager, virtualAssetFilename, "Asset", "1", rapidJsonDocument);

			// Dispatch asset compiler
			// TODO(co) Add multi-threading support: Add compiler queue which is processed in the background, ensure compiler instances are reused

			// Get the asset input directory and asset output directory
			const std::string virtualAssetPackageInputDirectory = mProjectName + '/' + mAssetPackageDirectoryName;
			const std::string virtualAssetInputDirectory = std_filesystem::path(virtualAssetFilename).parent_path().generic_string();
			const std::string assetDirectory = virtualAssetInputDirectory.substr(virtualAssetInputDirectory.find('/') + 1);
			const std::string compilerClassName = rapidJsonDocument["Asset"]["Compiler"]["ClassName"].GetString();
			const std::string renderTargetDataRootDirectory = getRenderTargetDataRootDirectory(rendererTarget);
			const std::string virtualAssetOutputDirectory = renderTargetDataRootDirectory + '/' + mProjectName + '/' + mAssetPackageDirectoryName + '/' + assetDirectory;

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

			// Evaluate the asset type and continue with the processing in the asset type specific way
			// TODO(co) Currently this is fixed build in, later on me might want to have this dynamic so we can plugin additional asset compilers
			AssetCompilers::const_iterator iterator = mAssetCompilersByClassId.find(AssetCompilerClassId(compilerClassName.c_str()));
			if (mAssetCompilersByClassId.end() != iterator)
			{
				const IAssetCompiler* assetCompiler = iterator->second;
				assetCompiler->compile(input, configuration);

				{ // Update the output asset package
					const std::string assetName = std_filesystem::path(input.virtualAssetFilename).stem().generic_string();
					const std::string assetIdAsString = input.projectName + '/' + assetDirectory + '/' + assetName;
					::detail::outputAsset(input.context.getFileManager(), assetIdAsString, assetCompiler->getVirtualOutputAssetFilename(input, configuration), outputAssetPackage);
				}
			}
			else
			{
				throw std::runtime_error("Asset compiler class \"" + compilerClassName + "\" is unknown");
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
			IAssetImporter::Input input(mContext, mProjectName, absoluteSourceFilename, mProjectName + "/Imported/" + std_filesystem::path(absoluteSourceFilename).stem().generic_string());

			// TODO(co) Implement automatic asset importer selection
			SketchfabAssetImporter().import(input);

			++currentSourceAsset;
		}
		RENDERER_LOG(mContext, INFORMATION, "Finished import of %u assets", numberOfSourceAssets)
	}

	void ProjectImpl::compileAllAssets(const char* rendererTarget)
	{
		const RendererRuntime::AssetPackage::SortedAssetVector& sortedAssetVector = mAssetPackage.getSortedAssetVector();
		const size_t numberOfAssets = sortedAssetVector.size();

		// Discover changed assets
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
			const std::string virtualAssetPackageFilename = getRenderTargetDataRootDirectory(rendererTarget) + '/' + mProjectName + '/' + mAssetPackageDirectoryName + '/' + mAssetPackageDirectoryName + ".assets";
			RendererRuntime::IFileManager& fileManager = mContext.getFileManager();
			RendererRuntime::AssetPackage outputAssetPackage;

			{ // Try to load an already compiled asset package to speed up the asset compilation
				// Do we need to mount a directory now? (e.g. "DataPc", "DataMobile" etc.)
				const std::string renderTargetDataRootDirectory = getRenderTargetDataRootDirectory(rendererTarget);
				if (fileManager.getMountPoint(renderTargetDataRootDirectory.c_str()) == nullptr)
				{
					fileManager.mountDirectory((fileManager.getAbsoluteRootDirectory() + '/' + renderTargetDataRootDirectory).c_str(), renderTargetDataRootDirectory.c_str());
				}

				// Tell the memory mapped file about the LZ4 compressed data and decompress it at once
				RendererRuntime::MemoryFile memoryFile;
				if (memoryFile.loadLz4CompressedDataByVirtualFilename(RendererRuntime::v1AssetPackage::FORMAT_TYPE, RendererRuntime::v1AssetPackage::FORMAT_VERSION, fileManager, virtualAssetPackageFilename.c_str()))
				{
					memoryFile.decompress();

					// Read in the asset package header
					RendererRuntime::v1AssetPackage::AssetPackageHeader assetPackageHeader;
					memoryFile.read(&assetPackageHeader, sizeof(RendererRuntime::v1AssetPackage::AssetPackageHeader));

					// Sanity check
					assert((assetPackageHeader.numberOfAssets > 0) && "Invalid empty asset package detected");

					// Read in the asset package content in one single burst
					RendererRuntime::AssetPackage::SortedAssetVector& sortedOutputAssetVector = outputAssetPackage.getWritableSortedAssetVector();
					sortedOutputAssetVector.resize(assetPackageHeader.numberOfAssets);
					memoryFile.read(sortedOutputAssetVector.data(), sizeof(RendererRuntime::Asset) * assetPackageHeader.numberOfAssets);
				}
			}

			// Compile all changed assets
			if (outputAssetPackage.getSortedAssetVector().empty())
			{
				// Slow path: Failed to load an already existing compiled asset package, we need to build a complete one
				outputAssetPackage.getWritableSortedAssetVector().reserve(numberOfAssets);
				for (size_t i = 0; i < numberOfAssets; ++i)
				{
					// Reminder: Assets might not be fully compiled but just collect needed information
					RENDERER_LOG(mContext, INFORMATION, "Compiling asset %u of %u", i + 1, numberOfAssets)
					const RendererRuntime::Asset& asset = sortedAssetVector[i];
					compileAsset(asset, rendererTarget, outputAssetPackage);

					// Call "RendererRuntime::IRendererRuntime::reloadResourceByAssetId()" directly after an asset has been compiled to see changes as early as possible
					if (nullptr != mProjectAssetMonitor)
					{
						const RendererRuntime::AssetId sourceAssetId = asset.assetId;
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
			}
			else
			{
				// Fast path: We were able to load a previously compiled asset package and now only have to care about the changed assets
				const size_t numberOfChangedAssets = changedAssetIds.size();
				for (size_t i = 0; i < numberOfChangedAssets; ++i)
				{
					const RendererRuntime::AssetId sourceAssetId = changedAssetIds[i];
					const RendererRuntime::Asset* asset = mAssetPackage.tryGetAssetByAssetId(sourceAssetId);
					if (nullptr == asset)
					{
						throw std::runtime_error(std::string("Source asset ID ") + std::to_string(sourceAssetId) + " is unknown");
					}
					RENDERER_LOG(mContext, INFORMATION, "Compiling asset %u of %u", i + 1, numberOfChangedAssets)
					compileAsset(*asset, rendererTarget, outputAssetPackage);

					// Call "RendererRuntime::IRendererRuntime::reloadResourceByAssetId()" directly after an asset has been compiled to see changes as early as possible
					if (nullptr != mProjectAssetMonitor)
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
				if (!memoryFile.writeLz4CompressedDataByVirtualFilename(STRING_ID("AssetPackage"), RendererRuntime::v1AssetPackage::FORMAT_VERSION, fileManager, virtualAssetPackageFilename.c_str()))
				{
					throw std::runtime_error("Failed to write LZ4 compressed output file \"" + virtualAssetPackageFilename + '\"');
				}
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

			// Get the relevant asset metadata parts
			const std::string& virtualFilename = asset.virtualFilename;
			const std::string virtualAssetDirectory = std_filesystem::path(virtualFilename).parent_path().generic_string();
			const std::string assetDirectory = virtualAssetDirectory.substr(virtualAssetDirectory.find('/') + 1);
			const std::string assetName = std_filesystem::path(virtualFilename).stem().generic_string();

			// Construct the compiled asset ID as string
			const std::string compiledAssetIdAsString = mProjectName + '/' + assetDirectory + '/' + assetName;

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
