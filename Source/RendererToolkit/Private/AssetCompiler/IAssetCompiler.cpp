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
#include "RendererToolkit/Private/AssetCompiler/IAssetCompiler.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"

#include <RendererRuntime/Public/Core/Math/Math.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Asset/AssetPackage.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void IAssetCompiler::outputAsset(const RendererRuntime::IFileManager& fileManager, const std::string& assetIdAsString, const std::string& virtualOutputAssetFilename, RendererRuntime::AssetPackage& outputAssetPackage)
	{
		// Sanity check
		const std::string virtualFilename = assetIdAsString + std_filesystem::path(virtualOutputAssetFilename).extension().generic_string();
		if (virtualFilename.size() >= RendererRuntime::Asset::MAXIMUM_ASSET_FILENAME_LENGTH)
		{
			throw std::runtime_error("The output asset filename \"" + virtualFilename + "\" exceeds the length limit of " + std::to_string(RendererRuntime::Asset::MAXIMUM_ASSET_FILENAME_LENGTH - 1));	// -1 for not including terminating zero
		}

		// Output asset
		RendererRuntime::Asset outputAsset;
		outputAsset.assetId = StringHelper::getAssetIdByString(assetIdAsString.c_str());
		outputAsset.fileHash = RendererRuntime::Math::calculateFileFNV1a64ByVirtualFilename(fileManager, virtualOutputAssetFilename.c_str());
		strcpy(outputAsset.virtualFilename, virtualFilename.c_str());
		outputAssetPackage.getWritableSortedAssetVector().push_back(outputAsset);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
