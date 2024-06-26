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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Asset/AssetManager.h"
#include "Renderer/Public/Asset/AssetPackage.h"
#include "Renderer/Public/Asset/Loader/AssetPackageLoader.h"
#include "Renderer/Public/Core/File/IFileManager.h"
#include "Renderer/Public/Core/File/FileSystemHelper.h"
#include "Renderer/Public/IRenderer.h"
#include "Renderer/Public/Context.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void AssetManager::clear()
	{
		const size_t numberOfAssetPackages = mAssetPackageVector.size();
		for (size_t i = 0; i < numberOfAssetPackages; ++i)
		{
			delete mAssetPackageVector[i];
		}
		mAssetPackageVector.clear();
	}

	AssetPackage& AssetManager::addAssetPackage(AssetPackageId assetPackageId)
	{
		RHI_ASSERT(mRenderer.getContext(), nullptr == tryGetAssetPackageById(assetPackageId), "Renderer asset package ID is already used")
		AssetPackage* assetPackage = new AssetPackage(assetPackageId);
		mAssetPackageVector.push_back(assetPackage);
		return *assetPackage;
	}

	AssetPackage* AssetManager::mountAssetPackage(AbsoluteDirectoryName absoluteDirectoryName, const char* projectName)
	{
		// Mount the asset package into the file system
		if (mRenderer.getFileManager().mountDirectory(absoluteDirectoryName, projectName))
		{
			// Generate the asset package ID using the naming scheme "<project name>/<asset package name>" and load the asset package
			const std::string assetPackageName = std_filesystem::path(absoluteDirectoryName).stem().generic_string();
			const std::string projectNameWithSlash = std::string(projectName) + '/';
			return addAssetPackageByVirtualFilename(StringId((projectNameWithSlash + assetPackageName).c_str()), (projectNameWithSlash + assetPackageName + ".assets").c_str());
		}
		else
		{
			// Error!
			RHI_ASSERT(mRenderer.getContext(), false, "Renderer failed to mount the asset package")
			return nullptr;
		}
	}

	AssetPackage* AssetManager::tryGetAssetPackageById(AssetPackageId assetPackageId) const
	{
		AssetPackageVector::const_iterator iterator = std::find_if(mAssetPackageVector.cbegin(), mAssetPackageVector.cend(),
			[assetPackageId](const AssetPackage* assetPackage) { return (assetPackage->getAssetPackageId() == assetPackageId); }
			);
		return (iterator != mAssetPackageVector.cend()) ? *iterator : nullptr;
	}

	AssetPackage& AssetManager::getAssetPackageById(AssetPackageId assetPackageId) const
	{
		AssetPackageVector::const_iterator iterator = std::find_if(mAssetPackageVector.cbegin(), mAssetPackageVector.cend(),
			[assetPackageId](const AssetPackage* assetPackage) { return (assetPackage->getAssetPackageId() == assetPackageId); }
			);
		RHI_ASSERT(mRenderer.getContext(), iterator != mAssetPackageVector.cend(), "Unknown renderer asset package ID")
		return **iterator;
	}

	void AssetManager::removeAssetPackage(AssetPackageId assetPackageId)
	{
		AssetPackageVector::const_iterator iterator = std::find_if(mAssetPackageVector.cbegin(), mAssetPackageVector.cend(),
			[assetPackageId](const AssetPackage* assetPackage) { return (assetPackage->getAssetPackageId() == assetPackageId); }
			);
		RHI_ASSERT(mRenderer.getContext(), iterator != mAssetPackageVector.cend(), "Unknown renderer asset package ID")
		delete *iterator;
		mAssetPackageVector.erase(iterator);
	}

	const Asset* AssetManager::tryGetAssetByAssetId(AssetId assetId) const
	{
		// Search inside all mounted asset packages, later added asset packages cover old ones
		const size_t numberOfAssetPackages = mAssetPackageVector.size();
		for (size_t i = 0; i < numberOfAssetPackages; ++i)
		{
			const Asset* asset = mAssetPackageVector[i]->tryGetAssetByAssetId(assetId);
			if (nullptr != asset)
			{
				return asset;
			}
		}

		// Sorry, the given asset ID is unknown
		return nullptr;
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	AssetPackage* AssetManager::addAssetPackageByVirtualFilename(AssetPackageId assetPackageId, VirtualFilename virtualFilename)
	{
		RHI_ASSERT(mRenderer.getContext(), nullptr == tryGetAssetPackageById(assetPackageId), "Renderer asset package ID is already used")
		IFileManager& fileManager = mRenderer.getFileManager();
		IFile* file = fileManager.openFile(IFileManager::FileMode::READ, virtualFilename);
		if (nullptr != file)
		{
			AssetPackage* assetPackage = new AssetPackage(assetPackageId);
			AssetPackageLoader().loadAssetPackage(*assetPackage, *file);
			mAssetPackageVector.push_back(assetPackage);
			fileManager.closeFile(*file);

			// Done
			return assetPackage;
		}
		else
		{
			// Error! This is horrible. No assets.
			RHI_ASSERT(mRenderer.getContext(), false, "Renderer failed to add asset package")
			return nullptr;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
