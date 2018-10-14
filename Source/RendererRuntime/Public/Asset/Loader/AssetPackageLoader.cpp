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
#include "RendererRuntime/Public/Asset/Loader/AssetPackageLoader.h"
#include "RendererRuntime/Public/Asset/Loader/AssetPackageFileFormat.h"
#include "RendererRuntime/Public/Asset/AssetPackage.h"
#include "RendererRuntime/Public/Core/File/MemoryFile.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void AssetPackageLoader::loadAssetPackage(AssetPackage& assetPackage, IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data and decompress it at once
		MemoryFile memoryFile;
		if (memoryFile.loadLz4CompressedDataFromFile(v1AssetPackage::FORMAT_TYPE, v1AssetPackage::FORMAT_VERSION, file))
		{
			memoryFile.decompress();

			// Read in the asset package header
			v1AssetPackage::AssetPackageHeader assetPackageHeader;
			memoryFile.read(&assetPackageHeader, sizeof(v1AssetPackage::AssetPackageHeader));

			// Sanity check
			assert((assetPackageHeader.numberOfAssets > 0) && "Invalid empty asset package detected");

			// Read in the asset package content in one single burst
			AssetPackage::SortedAssetVector& sortedAssetVector = assetPackage.getWritableSortedAssetVector();
			sortedAssetVector.resize(assetPackageHeader.numberOfAssets);
			memoryFile.read(sortedAssetVector.data(), sizeof(Asset) * assetPackageHeader.numberOfAssets);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
