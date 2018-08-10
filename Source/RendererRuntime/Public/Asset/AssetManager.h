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
#include "RendererRuntime/Public/Core/Manager.h"
#include "RendererRuntime/Public/Asset/Asset.h"

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
	class AssetPackage;
	class IRendererRuntime;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetPackageId;			///< Resource loader type identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset package name>"
	typedef const char* VirtualFilename;		///< UTF-8 virtual filename, the virtual filename scheme is "<mount point = project name>/<asset type>/<asset category>/<asset name>.<file extension>" (example "Example/Mesh/Monster/Squirrel.mesh"), never ever a null pointer and always finished by a terminating zero
	typedef const char* AbsoluteDirectoryName;	///< UTF-8 absolute directory name (example: "c:/MyProject"), without "/" at the end, never ever a null pointer and always finished by a terminating zero


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class AssetManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererRuntimeImpl;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<AssetPackage*> AssetPackageVector;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Asset package                                         ]
		//[-------------------------------------------------------]
		void clear();
		RENDERERRUNTIME_API_EXPORT AssetPackage& addAssetPackage(AssetPackageId assetPackageId);
		RENDERERRUNTIME_API_EXPORT AssetPackage* mountAssetPackage(AbsoluteDirectoryName absoluteDirectoryName, const char* projectName);
		RENDERERRUNTIME_API_EXPORT AssetPackage& getAssetPackageById(AssetPackageId assetPackageId) const;
		RENDERERRUNTIME_API_EXPORT AssetPackage* tryGetAssetPackageById(AssetPackageId assetPackageId) const;
		RENDERERRUNTIME_API_EXPORT void removeAssetPackage(AssetPackageId assetPackageId);

		//[-------------------------------------------------------]
		//[ Asset                                                 ]
		//[-------------------------------------------------------]
		RENDERERRUNTIME_API_EXPORT const Asset* tryGetAssetByAssetId(AssetId assetId) const;

		inline const Asset& getAssetByAssetId(AssetId assetId) const
		{
			const Asset* asset = tryGetAssetByAssetId(assetId);
			assert(nullptr != asset);
			return *asset;
		}

		inline VirtualFilename tryGetVirtualFilenameByAssetId(AssetId assetId) const
		{
			const Asset* asset = tryGetAssetByAssetId(assetId);
			return (nullptr != asset) ? asset->virtualFilename : nullptr;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit AssetManager(IRendererRuntime& rendererRuntime) :
			mRendererRuntime(rendererRuntime)
		{
			// Nothing here
		}

		inline ~AssetManager()
		{
			clear();
		}

		explicit AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;
		AssetPackage* addAssetPackageByVirtualFilename(AssetPackageId assetPackageId, VirtualFilename virtualFilename);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&  mRendererRuntime;	///< Renderer runtime instance, do not destroy the instance
		AssetPackageVector mAssetPackageVector;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
