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
#include <Renderer/Public/Renderer.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t GraphicsProgramCacheId;	///< Graphics program cache identifier, result of hashing the shader cache IDs of the referenced shaders


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class GraphicsProgramCache final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class GraphicsProgramCacheManager;	// Is creating and managing graphics program cache instances
		friend class GraphicsPipelineStateCompiler;	// Is creating graphics program cache instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the graphics program cache ID
		*
		*  @return
		*    The graphics program cache ID
		*/
		inline GraphicsProgramCacheId getGraphicsProgramCacheId() const
		{
			return mGraphicsProgramCacheId;
		}

		/**
		*  @brief
		*    Return graphics program
		*
		*  @return
		*    The graphics program
		*/
		inline Renderer::IGraphicsProgramPtr getGraphicsProgramPtr() const
		{
			return mGraphicsProgramPtr;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline GraphicsProgramCache(GraphicsProgramCacheId graphicsProgramCacheId, Renderer::IGraphicsProgram& graphicsProgram) :
			mGraphicsProgramCacheId(graphicsProgramCacheId),
			mGraphicsProgramPtr(&graphicsProgram)
		{
			// Nothing here
		}

		inline ~GraphicsProgramCache()
		{
			// Nothing here
		}

		explicit GraphicsProgramCache(const GraphicsProgramCache&) = delete;
		GraphicsProgramCache& operator=(const GraphicsProgramCache&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		GraphicsProgramCacheId		  mGraphicsProgramCacheId;
		Renderer::IGraphicsProgramPtr mGraphicsProgramPtr;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
