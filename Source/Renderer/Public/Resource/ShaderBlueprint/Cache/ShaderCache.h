/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Renderer/Public/Core/StringId.h"
#include "Renderer/Public/Core/GetInvalid.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t			 ShaderCacheId;	///< Shader cache identifier, identical to the shader combination ID
	typedef StringId			 AssetId;		///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef std::vector<AssetId> AssetIds;


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class ShaderCache final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class ShaderCacheManager;			// Is creating and managing shader cache instances
		friend class GraphicsPipelineStateCompiler;	// Is creating shader cache instances
		friend class ComputePipelineStateCompiler;	// Is creating shader cache instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the shader cache ID
		*
		*  @return
		*    The shader cache ID
		*/
		[[nodiscard]] inline ShaderCacheId getShaderCacheId() const
		{
			return mShaderCacheId;
		}

		/**
		*  @brief
		*    Return master shader cache
		*
		*  @return
		*    The master shader cache, can be a null pointer, don't destroy the instance
		*/
		[[nodiscard]] inline ShaderCache* getMasterShaderCache() const
		{
			return mMasterShaderCache;
		}

		/**
		*  @brief
		*    Return RHI shader bytecode
		*
		*  @return
		*    The RHI shader bytecode
		*/
		[[nodiscard]] inline const Rhi::ShaderBytecode& getShaderBytecode() const
		{
			return mShaderBytecode;
		}

		/**
		*  @brief
		*    Return RHI shader
		*
		*  @return
		*    The RHI shader
		*/
		[[nodiscard]] inline const Rhi::IShaderPtr& getShaderPtr() const
		{
			return (nullptr != mMasterShaderCache) ? mMasterShaderCache->mShaderPtr : mShaderPtr;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline ShaderCache() :
			mShaderCacheId(getInvalid<ShaderCacheId>()),
			mMasterShaderCache(nullptr)
		{
			// Nothing here
		}

		inline explicit ShaderCache(ShaderCacheId shaderCacheId) :
			mShaderCacheId(shaderCacheId),
			mMasterShaderCache(nullptr)
		{
			// Nothing here
		}

		inline ShaderCache(ShaderCacheId shaderCacheId, ShaderCache* masterShaderCache) :
			mShaderCacheId(shaderCacheId),
			mMasterShaderCache(masterShaderCache)
		{
			// Nothing here
		}

		inline ~ShaderCache()
		{
			// Nothing here
		}

		explicit ShaderCache(const ShaderCache&) = delete;
		ShaderCache& operator=(const ShaderCache&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ShaderCacheId		mShaderCacheId;
		ShaderCache*		mMasterShaderCache;			///< If there's a master shader cache instance, we don't own the references shader but only redirect to it (multiple shader combinations resulting in same shader source code topic), don't destroy the instance
		AssetIds			mAssetIds;					///< List of IDs of the assets (shader blueprint, shader piece) which took part in the shader cache creation
		uint64_t			mCombinedAssetFileHashes;	///< Combination of the file hash of all assets (shader blueprint, shader piece) which took part in the shader cache creation
		Rhi::ShaderBytecode mShaderBytecode;
		Rhi::IShaderPtr		mShaderPtr;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
