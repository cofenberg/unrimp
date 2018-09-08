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
#include "RendererRuntime/Public/Asset/AssetPackage.h"
#include "RendererRuntime/Public/Core/Math/Math.h"
#include "RendererRuntime/Public/Context.h"

#include <Renderer/Public/Renderer.h>

#include <cassert>
#include <algorithm>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Structures                                            ]
		//[-------------------------------------------------------]
		struct OrderByAssetId final
		{
			inline bool operator()(const RendererRuntime::Asset& left, RendererRuntime::AssetId right) const
			{
				return (left.assetId < right);
			}

			inline bool operator()(RendererRuntime::AssetId left, const RendererRuntime::Asset& right) const
			{
				return (left < right.assetId);
			}
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
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void AssetPackage::addAsset(MAYBE_UNUSED const Context& context, AssetId assetId, VirtualFilename virtualFilename)
	{
		RENDERER_ASSERT(context, nullptr == tryGetAssetByAssetId(assetId), "Renderer runtime asset ID is already used")
		RENDERER_ASSERT(context, strlen(virtualFilename) < Asset::MAXIMUM_ASSET_FILENAME_LENGTH, "The renderer runtime asset filename is too long")
		SortedAssetVector::const_iterator iterator = std::lower_bound(mSortedAssetVector.cbegin(), mSortedAssetVector.cend(), assetId, ::detail::OrderByAssetId());
		Asset& asset = *mSortedAssetVector.insert(iterator, Asset());
		asset.assetId = assetId;
		strncpy(asset.virtualFilename, virtualFilename, Asset::MAXIMUM_ASSET_FILENAME_LENGTH - 1);	// -1 not including the terminating zero
	}

	const Asset* AssetPackage::tryGetAssetByAssetId(AssetId assetId) const
	{
		SortedAssetVector::const_iterator iterator = std::lower_bound(mSortedAssetVector.cbegin(), mSortedAssetVector.cend(), assetId, ::detail::OrderByAssetId());
		return (iterator != mSortedAssetVector.cend() && iterator->assetId == assetId) ? &(*iterator) : nullptr;
	}

	bool AssetPackage::validateIntegrity(const IFileManager& fileManager) const
	{
		for (const Asset& asset : mSortedAssetVector)
		{
			if (Math::calculateFileFNV1a64ByVirtualFilename(fileManager, asset.virtualFilename) != asset.fileHash)
			{
				// Invalid integrity
				return false;
			}
		}

		// Valid integrity
		return true;
	}

	Asset* AssetPackage::tryGetWritableAssetByAssetId(AssetId assetId)
	{
		SortedAssetVector::iterator iterator = std::lower_bound(mSortedAssetVector.begin(), mSortedAssetVector.end(), assetId, ::detail::OrderByAssetId());
		return (iterator != mSortedAssetVector.cend() && iterator->assetId == assetId) ? &(*iterator) : nullptr;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
