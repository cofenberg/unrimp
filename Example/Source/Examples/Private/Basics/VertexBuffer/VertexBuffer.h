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
#include "Examples/Private/Framework/ExampleBase.h"

#include <Rhi/Public/Rhi.h>


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Shows some vertex buffer object (VBO) use cases
*
*  @remarks
*    Demonstrates:
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Graphics pipeline state object (PSO)
*    - Multiple vertex attributes within a single vertex buffer object (VBO), vertex array object (VAO) is only using one vertex buffer object (VBO)
*    - One vertex buffer object (VBO) per vertex attribute, vertex array object (VAO) is using multiple vertex buffer objects (VBO)
*/
class VertexBuffer final : public ExampleBase
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Default constructor
	*/
	inline VertexBuffer()
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~VertexBuffer() override
	{
		// The resources are released within "onDeinitialization()"
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onDraw(Rhi::CommandBuffer& commandBuffer) override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	void fillCommandBuffer();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Rhi::IBufferManagerPtr	mBufferManager;		///< Buffer manager, can be a null pointer
	Rhi::CommandBuffer		mCommandBuffer;		///< Command buffer
	Rhi::IRootSignaturePtr	mRootSignature;		///< Root signature, can be a null pointer
	// Using one vertex buffer object (VBO)
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineStateVbo;	///< Graphics pipeline state object (PSO), can be a null pointer
	Rhi::IVertexArrayPtr			mVertexArrayVbo;			///< Vertex array object (VAO), can be a null pointer
	// Using multiple vertex buffer objects (VBO)
	Rhi::IVertexArrayPtr			mVertexArrayVbos;			///< Vertex array object (VAO), can be a null pointer
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineStateVbos;	///< Graphics pipeline state object (PSO), can be a null pointer


};
