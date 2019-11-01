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
#include <Renderer/Public/Resource/CompositorNode/Pass/CompositorPassFactory.h>


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
class CompositorPassFactoryFirst final : public Renderer::CompositorPassFactory
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	inline CompositorPassFactoryFirst()
	{
		// Nothing here
	}

	inline virtual ~CompositorPassFactoryFirst() override
	{
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Protected virtual Renderer::ICompositorPassFactory methods ]
//[-------------------------------------------------------]
protected:
	[[nodiscard]] virtual Renderer::ICompositorResourcePass* createCompositorResourcePass(const Renderer::CompositorTarget& compositorTarget, Renderer::CompositorPassTypeId compositorPassTypeId) const override;
	[[nodiscard]] virtual Renderer::ICompositorInstancePass* createCompositorInstancePass(const Renderer::ICompositorResourcePass& compositorResourcePass, const Renderer::CompositorNodeInstance& compositorNodeInstance) const override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit CompositorPassFactoryFirst(const CompositorPassFactoryFirst&) = delete;
	CompositorPassFactoryFirst& operator=(const CompositorPassFactoryFirst&) = delete;


};
