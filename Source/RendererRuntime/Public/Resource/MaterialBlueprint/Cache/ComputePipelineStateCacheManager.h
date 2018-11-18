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
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateSignature.h"
#include "RendererRuntime/Public/Core/Manager.h"

#include <Renderer/Public/Renderer.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4548)	// warning C4548: expression before comma has no effect; expected expression with side-effect
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <string>
	#include <unordered_map>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class ComputePipelineStateCache;
}


// Disable warnings
// TODO(co) See "RendererRuntime::ComputePipelineStateCacheManager::ComputePipelineStateCacheManager()": How the heck should we avoid such a situation without using complicated solutions like a pointer to an instance? (= more individual allocations/deallocations)
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t ComputePipelineStateSignatureId;	///< Compute pipeline state signature identifier, result of hashing the referenced shaders as well as other pipeline state properties


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compute pipeline state cache manager
	*
	*  @remarks
	*    The compute pipeline state cache is the top of the shader related cache hierarchy and maps to Vulkan, Direct3D 12,
	*    Apple Metal and other rendering APIs using pipeline state objects (PSO). The next lowest cache hierarchy level is the
	*    shader cache (compute shader) which handles the binary results of the shader compiler.
	*    As of January 2016, although claimed to fulfill the OpenGL 4.1 specification, Apples OpenGL implementation used
	*    on Mac OS X lacks the feature of receiving the program binary in order to reuse it for the next time instead of
	*    fully compiling a program. Hence, at the lowest cache hierarchy, there's a shader source code cache for the build
	*    shader source codes so at least this doesn't need to be performed during each program execution.
	*
	*    Sum up of the cache hierarchy:
	*    - 0: "RendererRuntime::ComputePipelineStateCacheManager": Maps to Vulkan, Direct3D 12, Apple Metal etc.; managed by material blueprint
	*    - 1: "RendererRuntime::ShaderCacheManager": Maps to Direct3D 9 - 11, separate OpenGL shader objects and is still required for Direct3D 12
	*      and other similar designed APIs because the binary shaders are required when creating pipeline state objects;
	*      managed by shader blueprint manager
	*    - 2: "RendererRuntime::ShaderSourceCodeCacheManager": Shader source code cache for the build shader source codes, used for e.g. Apples
	*      OpenGL implementation lacking of binary program support; managed by shader blueprint manager   TODO(co) "RendererRuntime::ShaderSourceCodeCacheManager" doesn't exist, yet
	*
	*    The compute pipeline state cache has two types of IDs:
	*    - "RendererRuntime::ComputePipelineStateSignatureId" -> Result of hashing the material blueprint ID and the shader combination generating shader properties and dynamic shader pieces
	*    - "RendererRuntime::ComputePipelineStateCacheId" -> Includes the hashing the build shader source code
	*    Those two types of IDs are required because it's possible that different "RendererRuntime::ComputePipelineStateSignatureId" result in one and the
	*    same build shader source code of references shaders.
	*
	*  @note
	*    - One pipeline state cache manager per material blueprint instance
	*
	*  @todo
	*    - TODO(co) For Vulkan, DirectX 12 and Apple Metal the pipeline state object instance will be managed in here
	*    - TODO(co) Direct3D 12: Pipeline state object: Add support for "GetCachedBlob" (super efficient material cache), see https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/Samples/D3D12PipelineStateCache/src/PSOLibrary.cpp
	*/
	class ComputePipelineStateCacheManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class MaterialBlueprintResource;	// Is creating and using a compute program cache manager instance


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the owner material blueprint resource
		*
		*  @return
		*    The owner material blueprint resource
		*/
		[[nodiscard]] inline MaterialBlueprintResource& getMaterialBlueprintResource() const
		{
			return mMaterialBlueprintResource;
		}

		/**
		*  @brief
		*    Request a compute pipeline state cache instance by combination
		*
		*  @param[in] shaderProperties
		*    Shader properties to use
		*  @param[in] allowEmergencySynchronousCompilation
		*    Allow emergency synchronous compilation if no fallback could be found? This will result in a runtime hiccup instead of compute artifacts.
		*
		*  @return
		*    The requested compute pipeline state cache instance, null pointer on error, do not destroy the instance
		*/
		[[nodiscard]] Renderer::IComputePipelineStatePtr getComputePipelineStateCacheByCombination(const ShaderProperties& shaderProperties, bool allowEmergencySynchronousCompilation);

		/**
		*  @brief
		*    Clear the pipeline state cache manager
		*/
		void clearCache();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit ComputePipelineStateCacheManager(MaterialBlueprintResource& materialBlueprintResource) :
			mMaterialBlueprintResource(materialBlueprintResource),
			mPipelineStateObjectCacheNeedSaving(false)
		{
			// Nothing here
		}

		explicit ComputePipelineStateCacheManager(const ComputePipelineStateCacheManager&) = delete;

		inline ~ComputePipelineStateCacheManager()
		{
			clearCache();
		}

		ComputePipelineStateCacheManager& operator=(const ComputePipelineStateCacheManager&) = delete;

		ComputePipelineStateCache* getFallbackComputePipelineStateCache(const ShaderProperties& shaderProperties);

		//[-------------------------------------------------------]
		//[ Pipeline state object cache                           ]
		//[-------------------------------------------------------]
		void loadPipelineStateObjectCache(IFile& file);

		[[nodiscard]] inline bool doesPipelineStateObjectCacheNeedSaving() const
		{
			return mPipelineStateObjectCacheNeedSaving;
		}

		void savePipelineStateObjectCache(IFile& file);


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::unordered_map<ComputePipelineStateSignatureId, ComputePipelineStateCache*> ComputePipelineStateCacheByComputePipelineStateSignatureId;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		MaterialBlueprintResource&								   mMaterialBlueprintResource;			///< Owner material blueprint resource
		ComputePipelineStateCacheByComputePipelineStateSignatureId mComputePipelineStateCacheByComputePipelineStateSignatureId;
		bool													   mPipelineStateObjectCacheNeedSaving;	///< "true" if a cache needs saving due to changes during runtime, else "false"

		// Temporary instances to reduce the number of memory allocations/deallocations
		ComputePipelineStateSignature mTemporaryComputePipelineStateSignature;
		ShaderProperties			  mFallbackShaderProperties;
		ComputePipelineStateSignature mFallbackComputePipelineStateSignature;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime


// Reset warning manipulations we did above
PRAGMA_WARNING_POP
