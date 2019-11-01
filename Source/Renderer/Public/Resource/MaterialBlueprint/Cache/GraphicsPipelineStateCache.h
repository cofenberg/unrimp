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
#include "Renderer/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateSignature.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class GraphicsPipelineStateCache final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class GraphicsPipelineStateCompiler;		// Needs to be able to set "mGraphicsPipelineStateObjectPtr"
		friend class GraphicsPipelineStateCacheManager;	// Is creating and managing graphics pipeline state cache instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the graphics pipeline state signature of the cache
		*
		*  @return
		*    The graphics pipeline state signature of the cache
		*/
		[[nodiscard]] inline const GraphicsPipelineStateSignature& getGraphicsPipelineStateSignature() const
		{
			return mGraphicsPipelineStateSignature;
		}

		/**
		*  @brief
		*    Return graphics pipeline state object
		*
		*  @return
		*    The graphics pipeline state object
		*/
		[[nodiscard]] inline const Rhi::IGraphicsPipelineStatePtr& getGraphicsPipelineStateObjectPtr() const
		{
			return mGraphicsPipelineStateObjectPtr;
		}

		/**
		*  @brief
		*    Return whether or not the graphics pipeline state cache is currently using fallback data due to asynchronous compilation
		*
		*  @return
		*    If "true", this graphics pipeline state cache is currently using fallback data because it's in asynchronous compilation
		*/
		[[nodiscard]] inline bool isUsingFallback() const
		{
			return mIsUsingFallback;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit GraphicsPipelineStateCache(const GraphicsPipelineStateSignature& graphicsPipelineStateSignature) :
			mGraphicsPipelineStateSignature(graphicsPipelineStateSignature),
			mIsUsingFallback(false)
		{
			// Nothing here
		}

		inline ~GraphicsPipelineStateCache()
		{
			// Nothing here
		}

		explicit GraphicsPipelineStateCache(const GraphicsPipelineStateCache&) = delete;
		GraphicsPipelineStateCache& operator=(const GraphicsPipelineStateCache&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GraphicsPipelineStateSignature mGraphicsPipelineStateSignature;
		Rhi::IGraphicsPipelineStatePtr mGraphicsPipelineStateObjectPtr;
		bool						   mIsUsingFallback;					///< If "true", this graphics pipeline state cache is currently using fallback data because it's in asynchronous compilation


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
