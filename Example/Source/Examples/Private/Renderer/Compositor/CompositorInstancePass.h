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
#include <Renderer/Public/Resource/CompositorNode/Pass/ICompositorInstancePass.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class CompositorResourcePass;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
class CompositorInstancePass final : public Renderer::ICompositorInstancePass
{


//[-------------------------------------------------------]
//[ Friends                                               ]
//[-------------------------------------------------------]
	friend class CompositorPassFactory;	// The only one allowed to create instances of this class


//[-------------------------------------------------------]
//[ Protected virtual Renderer::ICompositorInstancePass methods ]
//[-------------------------------------------------------]
protected:
	virtual void onFillCommandBuffer(const Rhi::IRenderTarget* renderTarget, const Renderer::CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer) override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	CompositorInstancePass(const CompositorResourcePass& compositorResourcePass, const Renderer::CompositorNodeInstance& compositorNodeInstance);

	inline virtual ~CompositorInstancePass() override
	{
		// Nothing here
	}

	explicit CompositorInstancePass(const CompositorInstancePass&) = delete;
	CompositorInstancePass& operator=(const CompositorInstancePass&) = delete;


};
