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
#include "RendererRuntime/Resource/IResourceListener.h"
#include "RendererRuntime/Resource/CompositorNode/Pass/ICompositorInstancePass.h"
#include "RendererRuntime/RenderQueue/RenderableManager.h"
#include "RendererRuntime/RenderQueue/RenderQueue.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class CompositorResourcePassCompute;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compositor instance pass compute via compute or graphics pipeline state
	*
	*  @remarks
	*    Graphics material blueprint: Using a screen covering triangle as discussed at e.g.
	*    - https://web.archive.org/web/20140719063725/http://www.altdev.co/2011/08/08/interesting-vertex-shader-trick/
	*    - "Vertex Shader Tricks by Bill Bilodeau - AMD at GDC14" - http://de.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau
	*    - Attribute-less rendering: "Rendering a Screen Covering Triangle in OpenGL (with no buffers)" - https://rauwendaal.net/2014/06/14/rendering-a-screen-covering-triangle-in-opengl/
	*/
	class CompositorInstancePassCompute : public ICompositorInstancePass, public IResourceListener
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorPassFactory;	// The only one allowed to create instances of this class


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		CompositorInstancePassCompute(const CompositorResourcePassCompute& compositorResourcePassCompute, const CompositorNodeInstance& compositorNodeInstance);
		virtual ~CompositorInstancePassCompute() override;

		inline MaterialResourceId getMaterialResourceId() const
		{
			return mMaterialResourceId;
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	public:
		virtual void onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer) override;

		inline virtual void onPostCommandBufferExecution() override
		{
			// Directly clear the render queue as soon as the frame rendering has been finished to avoid evil dangling pointers
			mRenderQueue.clear();
		}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onLoadingStateChange(const IResource& resource) override;


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::CompositorInstancePassCompute methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void createMaterialResource(MaterialResourceId parentMaterialResourceId);


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit CompositorInstancePassCompute(const CompositorInstancePassCompute&) = delete;
		CompositorInstancePassCompute& operator=(const CompositorInstancePassCompute&) = delete;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		RenderQueue		   mRenderQueue;
		MaterialResourceId mMaterialResourceId;
		RenderableManager  mRenderableManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
