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
#include "RendererRuntime/Public/Export.h"
#include "RendererRuntime/Public/Asset/Asset.h"
#include "RendererRuntime/Public/Core/GetInvalid.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class Context;
	class IFileManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetPackageId;		///< Resource loader type identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset package name>"
	typedef const char* VirtualFilename;	///< UTF-8 virtual filename, the virtual filename scheme is "<mount point = project name>/<asset directory>/<asset name>.<file extension>" (example "Example/Mesh/Monster/Squirrel.mesh"), never ever a null pointer and always finished by a terminating zero


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class AssetPackage final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<Asset> SortedAssetVector;


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class AssetManager;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline AssetPackage() :
			mAssetPackageId(getInvalid<AssetPackageId>())
		{
			// Nothing here
		}

		inline explicit AssetPackage(AssetPackageId assetPackageId) :
			mAssetPackageId(assetPackageId)
		{
			// Nothing here
		}

		inline ~AssetPackage()
		{
			// Nothing here
		}

		[[nodiscard]] inline AssetPackageId getAssetPackageId() const
		{
			return mAssetPackageId;
		}

		inline void clear()
		{
			mSortedAssetVector.clear();
		}

		[[nodiscard]] inline const SortedAssetVector& getSortedAssetVector() const
		{
			return mSortedAssetVector;
		}

		RENDERERRUNTIME_API_EXPORT void addAsset(const Context& context, AssetId assetId, VirtualFilename virtualFilename);
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT const Asset* tryGetAssetByAssetId(AssetId assetId) const;

		[[nodiscard]] inline VirtualFilename tryGetVirtualFilenameByAssetId(AssetId assetId) const
		{
			const Asset* asset = tryGetAssetByAssetId(assetId);
			return (nullptr != asset) ? asset->virtualFilename : nullptr;
		}

		[[nodiscard]] RENDERERRUNTIME_API_EXPORT bool validateIntegrity(const IFileManager& fileManager) const;

		// For internal use only (exposed for API performance reasons)
		[[nodiscard]] inline SortedAssetVector& getWritableSortedAssetVector()
		{
			return mSortedAssetVector;
		}
		[[nodiscard]] RENDERERRUNTIME_API_EXPORT Asset* tryGetWritableAssetByAssetId(AssetId assetId);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit AssetPackage(const AssetPackage&) = delete;
		AssetPackage& operator=(const AssetPackage&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		AssetPackageId	  mAssetPackageId;
		SortedAssetVector mSortedAssetVector;	///< Sorted vector of assets


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
