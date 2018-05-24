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
#include "Framework/IApplicationRenderer.h"


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Shows how to use multiple swap chains
*
*  @remarks
*    Demonstrates:
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Pipeline state object (PSO)
*    - Multiple swap chains
*
*  @note
*    - This example is intentionally using OS dependent native window
*      creation in order to keep the example "close to metal"
*
*  @todo
*    - TODO(sw) Is not derived from ExampleBase because it wants do drive own draw requests without base class interaction
*      And such an example is currently not supported by the example base implementation
*/
class FirstMultipleSwapChains final : public IApplicationRenderer
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] rendererName
	*    Case sensitive ASCII name of the renderer to instance, if null pointer or unknown renderer no renderer will be used.
	*    Example renderer names: "Null", "OpenGL", "OpenGLES3", "Vulkan", "Direct3D9", "Direct3D10", "Direct3D11", "Direct3D12"
	*/
	inline explicit FirstMultipleSwapChains(const char* rendererName) :
		IApplicationRenderer(rendererName)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~FirstMultipleSwapChains() override
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
	virtual void onDrawRequest() override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit FirstMultipleSwapChains(const FirstMultipleSwapChains& source) = delete;
	FirstMultipleSwapChains& operator =(const FirstMultipleSwapChains& source) = delete;

	/**
	*  @brief
	*    Fill the given commando buffer
	*
	*  @param[in] color
	*    RGBA clear color
	*  @param[out] commandBuffer
	*    Command buffer to fill
	*/
	void fillCommandBuffer(const float color[4], Renderer::CommandBuffer& commandBuffer) const;


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Renderer::IBufferManagerPtr	mBufferManager;	///< Buffer manager, can be a null pointer
	Renderer::CommandBuffer		mCommandBuffer;	///< Command buffer
	Renderer::IRootSignaturePtr	mRootSignature;	///< Root signature, can be a null pointer
	Renderer::IPipelineStatePtr	mPipelineState;	///< Pipeline state object (PSO), can be a null pointer
	Renderer::IVertexArrayPtr	mVertexArray;	///< Vertex array object (VAO), can be a null pointer
	Renderer::ISwapChainPtr		mSwapChain;		///< Swap chain, can be a null pointer


};
