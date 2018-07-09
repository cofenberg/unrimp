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
#include "RendererRuntime/Resource/MaterialBlueprint/Cache/PipelineStateSignature.h"

#include <Renderer/Renderer.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class PipelineStateCacheManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class PipelineStateCache final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class PipelineStateCompiler;		// Needs to be able to set "mPipelineStateObjectPtr"
		friend class PipelineStateCacheManager;	// Is creating and managing pipeline state cache instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the pipeline state signature of the cache
		*
		*  @return
		*    The pipeline state signature of the cache
		*/
		inline const PipelineStateSignature& getPipelineStateSignature() const
		{
			return mPipelineStateSignature;
		}

		/**
		*  @brief
		*    Return graphics pipeline state object
		*
		*  @return
		*    The graphics pipeline state object
		*/
		inline Renderer::IGraphicsPipelineStatePtr getGraphicsPipelineStateObjectPtr() const
		{
			return mGraphicsPipelineStateObjectPtr;
		}

		/**
		*  @brief
		*    Return whether or not the pipeline state cache is currently using fallback data due to asynchronous compilation
		*
		*  @return
		*    If "true", this pipeline state cache is currently using fallback data because it's in asynchronous compilation
		*/
		inline bool isUsingFallback() const
		{
			return mIsUsingFallback;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit PipelineStateCache(const PipelineStateSignature& pipelineStateSignature) :
			mPipelineStateSignature(pipelineStateSignature),
			mIsUsingFallback(false)
		{
			// Nothing here
		}

		inline ~PipelineStateCache()
		{
			// Nothing here
		}

		explicit PipelineStateCache(const PipelineStateCache&) = delete;
		PipelineStateCache& operator=(const PipelineStateCache&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		PipelineStateSignature				mPipelineStateSignature;
		Renderer::IGraphicsPipelineStatePtr	mGraphicsPipelineStateObjectPtr;
		bool								mIsUsingFallback;					///< If "true", this pipeline state cache is currently using fallback data because it's in asynchronous compilation


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
