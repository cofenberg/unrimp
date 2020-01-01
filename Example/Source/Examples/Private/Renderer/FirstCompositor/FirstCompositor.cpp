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
#include "Examples/Private/Renderer/FirstCompositor/FirstCompositor.h"
#include "Examples/Private/Renderer/FirstCompositor/CompositorPassFactoryFirst.h"

#include <Renderer/Public/IRenderer.h>
#include <Renderer/Public/Resource/CompositorNode/CompositorNodeResourceManager.h>
#include <Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		static const CompositorPassFactoryFirst compositorPassFactoryFirst;


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstCompositor::onInitialization()
{
	// Get and check the renderer instance
	Renderer::IRenderer& renderer = getRendererSafe();

	// Set our custom compositor pass factory
	renderer.getCompositorNodeResourceManager().setCompositorPassFactory(&::detail::compositorPassFactoryFirst);

	// Create the compositor workspace instance
	mCompositorWorkspaceInstance = new Renderer::CompositorWorkspaceInstance(renderer, ASSET_ID("Example/CompositorWorkspace/CW_First"));
}

void FirstCompositor::onDeinitialization()
{
	// TODO(co) Implement decent resource handling
	delete mCompositorWorkspaceInstance;
	mCompositorWorkspaceInstance = nullptr;

	// Be polite and unset our custom compositor pass factory
	getRendererSafe().getCompositorNodeResourceManager().setCompositorPassFactory(nullptr);
}

void FirstCompositor::onDraw()
{
	// Is there a compositor workspace instance?
	if (nullptr != mCompositorWorkspaceInstance)
	{
		// Get the main render target and ensure there's one
		Rhi::IRenderTarget* mainRenderTarget = getMainRenderTarget();
		if (nullptr != mainRenderTarget)
		{
			// Execute the compositor workspace instance
			mCompositorWorkspaceInstance->execute(*mainRenderTarget, nullptr, nullptr);
		}
	}
}
