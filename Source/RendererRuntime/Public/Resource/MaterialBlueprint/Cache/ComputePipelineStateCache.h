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
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateSignature.h"

#include <Renderer/Public/Renderer.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class ComputePipelineStateCache final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class ComputePipelineStateCompiler;		// Needs to be able to set "mComputePipelineStateObjectPtr"
		friend class ComputePipelineStateCacheManager;	// Is creating and managing compute pipeline state cache instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the compute pipeline state signature of the cache
		*
		*  @return
		*    The compute pipeline state signature of the cache
		*/
		[[nodiscard]] inline const ComputePipelineStateSignature& getComputePipelineStateSignature() const
		{
			return mComputePipelineStateSignature;
		}

		/**
		*  @brief
		*    Return compute pipeline state object
		*
		*  @return
		*    The compute pipeline state object
		*/
		[[nodiscard]] inline const Renderer::IComputePipelineStatePtr& getComputePipelineStateObjectPtr() const
		{
			return mComputePipelineStateObjectPtr;
		}

		/**
		*  @brief
		*    Return whether or not the compute pipeline state cache is currently using fallback data due to asynchronous compilation
		*
		*  @return
		*    If "true", this compute pipeline state cache is currently using fallback data because it's in asynchronous compilation
		*/
		[[nodiscard]] inline bool isUsingFallback() const
		{
			return mIsUsingFallback;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit ComputePipelineStateCache(const ComputePipelineStateSignature& computePipelineStateSignature) :
			mComputePipelineStateSignature(computePipelineStateSignature),
			mIsUsingFallback(false)
		{
			// Nothing here
		}

		inline ~ComputePipelineStateCache()
		{
			// Nothing here
		}

		explicit ComputePipelineStateCache(const ComputePipelineStateCache&) = delete;
		ComputePipelineStateCache& operator=(const ComputePipelineStateCache&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ComputePipelineStateSignature		mComputePipelineStateSignature;
		Renderer::IComputePipelineStatePtr	mComputePipelineStateObjectPtr;
		bool								mIsUsingFallback;					///< If "true", this compute pipeline state cache is currently using fallback data because it's in asynchronous compilation


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
