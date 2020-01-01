/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Renderer/FirstCompositor/CompositorPassFactoryFirst.h"
#include "Examples/Private/Renderer/FirstCompositor/CompositorResourcePassFirst.h"
#include "Examples/Private/Renderer/FirstCompositor/CompositorInstancePassFirst.h"


//[-------------------------------------------------------]
//[ Protected virtual Renderer::ICompositorPassFactory methods ]
//[-------------------------------------------------------]
Renderer::ICompositorResourcePass* CompositorPassFactoryFirst::createCompositorResourcePass(const Renderer::CompositorTarget& compositorTarget, Renderer::CompositorPassTypeId compositorPassTypeId) const
{
	// First, let the base implementation try to create an instance
	Renderer::ICompositorResourcePass* compositorResourcePass = CompositorPassFactory::createCompositorResourcePass(compositorTarget, compositorPassTypeId);

	// Evaluate the compositor pass type
	if (nullptr == compositorResourcePass && compositorPassTypeId == CompositorResourcePassFirst::TYPE_ID)
	{
		compositorResourcePass = new CompositorResourcePassFirst(compositorTarget);
	}

	// Done
	return compositorResourcePass;
}

Renderer::ICompositorInstancePass* CompositorPassFactoryFirst::createCompositorInstancePass(const Renderer::ICompositorResourcePass& compositorResourcePass, const Renderer::CompositorNodeInstance& compositorNodeInstance) const
{
	// First, let the base implementation try to create an instance
	Renderer::ICompositorInstancePass* compositorInstancePass = CompositorPassFactory::createCompositorInstancePass(compositorResourcePass, compositorNodeInstance);

	// Evaluate the compositor pass type
	if (nullptr == compositorInstancePass && compositorResourcePass.getTypeId() == CompositorResourcePassFirst::TYPE_ID)
	{
		compositorInstancePass = new CompositorInstancePassFirst(static_cast<const CompositorResourcePassFirst&>(compositorResourcePass), compositorNodeInstance);
	}

	// Done
	return compositorInstancePass;
}
