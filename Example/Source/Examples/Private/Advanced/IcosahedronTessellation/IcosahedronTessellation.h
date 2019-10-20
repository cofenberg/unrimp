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
#include "Examples/Private/Framework/ExampleBase.h"

#include <Renderer/Public/Renderer.h>


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    More advanced tessellation example
*
*  @remarks
*    Demonstrates:
*    - Index buffer object (IBO)
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - Root signature
*    - Graphics pipeline state object (PSO)
*    - Uniform buffer object (UBO)
*    - Vertex shader (VS), tessellation control shader (TCS), tessellation evaluation shader (TES), geometry shader (GS) and fragment shader (FS)
*/
class IcosahedronTessellation final : public ExampleBase
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	inline IcosahedronTessellation() :
		mTessellationLevelOuter(2.0f),
		mTessellationLevelInner(3.0f)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~IcosahedronTessellation() override
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
	virtual void onDraw() override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	void fillCommandBuffer();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Renderer::IBufferManagerPtr			mBufferManager;				///< Buffer manager, can be a null pointer
	Renderer::CommandBuffer				mCommandBuffer;				///< Command buffer
	Renderer::IRootSignaturePtr			mRootSignature;				///< Root signature, can be a null pointer
	Renderer::IUniformBufferPtr			mUniformBufferDynamicTcs;	///< Dynamic tessellation control shader uniform buffer object (UBO), can be a null pointer
	Renderer::IResourceGroupPtr			mUniformBufferGroupTcs;		///< Uniform buffer group with tessellation control shader visibility, can be a null pointer
	Renderer::IResourceGroupPtr			mUniformBufferGroupTes;		///< Uniform buffer group with tessellation evaluation shader visibility, can be a null pointer
	Renderer::IResourceGroupPtr			mUniformBufferGroupGs;		///< Uniform buffer group with geometry visibility, can be a null pointer
	Renderer::IResourceGroupPtr			mUniformBufferGroupFs;		///< Uniform buffer group with fragment shader visibility, can be a null pointer
	Renderer::IGraphicsPipelineStatePtr	mGraphicsPipelineState;		///< Graphics pipeline state object (PSO), can be a null pointer
	Renderer::IVertexArrayPtr			mVertexArray;				///< Vertex array object (VAO), can be a null pointer
	float								mTessellationLevelOuter;	///< Outer tessellation level
	float								mTessellationLevelInner;	///< Inner tessellation level


};
