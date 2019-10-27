/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "RendererRuntime/Public/Core/Manager.h"
#include "RendererRuntime/Public/Core/StringId.h"
#include "RendererRuntime/Public/Core/Renderer/RenderTargetTextureSignature.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP
#include <unordered_map>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
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
	typedef StringId AssetId;	///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class RenderTargetTextureManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct RenderTargetTextureElement final
		{
			AssetId						 assetId;
			RenderTargetTextureSignature renderTargetTextureSignature;
			Rhi::ITexture*				 texture;				///< Can be a null pointer, no "Rhi::ITexturePtr" to not have overhead when internally reallocating
			uint32_t					 numberOfReferences;	///< Number of texture references (don't misuse the RHI texture reference counter for this)

			inline RenderTargetTextureElement() :
				assetId(getInvalid<AssetId>()),
				texture(nullptr),
				numberOfReferences(0)
			{
				// Nothing here
			}

			inline explicit RenderTargetTextureElement(const RenderTargetTextureSignature& _renderTargetTextureSignature) :
				assetId(getInvalid<AssetId>()),
				renderTargetTextureSignature(_renderTargetTextureSignature),
				texture(nullptr),
				numberOfReferences(0)
			{
				// Nothing here
			}

			inline RenderTargetTextureElement(AssetId _assetId, const RenderTargetTextureSignature& _renderTargetTextureSignature) :
				assetId(_assetId),
				renderTargetTextureSignature(_renderTargetTextureSignature),
				texture(nullptr),
				numberOfReferences(0)
			{
				// Nothing here
			}

			inline RenderTargetTextureElement(AssetId _assetId, const RenderTargetTextureSignature& _renderTargetTextureSignature, Rhi::ITexture& _texture) :
				assetId(_assetId),
				renderTargetTextureSignature(_renderTargetTextureSignature),
				texture(&_texture),
				numberOfReferences(0)
			{
				// Nothing here
			}
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline explicit RenderTargetTextureManager(IRendererRuntime& rendererRuntime) :
			mRendererRuntime(rendererRuntime)
		{
			// Nothing here
		}

		inline ~RenderTargetTextureManager()
		{
			// Nothing here
		}

		explicit RenderTargetTextureManager(const RenderTargetTextureManager&) = delete;
		RenderTargetTextureManager& operator=(const RenderTargetTextureManager&) = delete;

		[[nodiscard]] inline IRendererRuntime& getRendererRuntime() const
		{
			return mRendererRuntime;
		}

		void clear();
		void clearRhiResources();
		void addRenderTargetTexture(AssetId assetId, const RenderTargetTextureSignature& renderTargetTextureSignature);
		[[nodiscard]] Rhi::ITexture* getTextureByAssetId(AssetId assetId, const Rhi::IRenderTarget& renderTarget, uint8_t numberOfMultisamples, float resolutionScale, const RenderTargetTextureSignature** outRenderTargetTextureSignature);
		void releaseRenderTargetTextureBySignature(const RenderTargetTextureSignature& renderTargetTextureSignature);


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<RenderTargetTextureElement>						 SortedRenderTargetTextureVector;
		typedef std::unordered_map<uint32_t, RenderTargetTextureSignatureId> AssetIdToRenderTargetTextureSignatureId;	///< Key = "RendererRuntime::AssetId"
		typedef std::unordered_map<uint32_t, uint32_t>						 AssetIdToIndex;							///< Key = "RendererRuntime::AssetId"


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&						mRendererRuntime;
		SortedRenderTargetTextureVector			mSortedRenderTargetTextureVector;
		AssetIdToRenderTargetTextureSignatureId	mAssetIdToRenderTargetTextureSignatureId;
		AssetIdToIndex							mAssetIdToIndex;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
