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
#include "RendererRuntime/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IRenderTarget;
	class CommandBuffer;
}
namespace RendererRuntime
{
	class CompositorContextData;
	class CompositorNodeInstance;
	class ICompositorResourcePass;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class ICompositorInstancePass
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorNodeInstance;		// Needs to execute compositor node instances
		friend class CompositorWorkspaceInstance;	// Needs to be able to set the render target


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline const ICompositorResourcePass& getCompositorResourcePass() const
		{
			return mCompositorResourcePass;
		}

		inline const CompositorNodeInstance& getCompositorNodeInstance() const
		{
			return mCompositorNodeInstance;
		}

		inline Renderer::IRenderTarget* getRenderTarget() const
		{
			return mRenderTarget;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorInstancePass methods ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Method is called when the owner compositor workspace instance loading has been finished
		*
		*  @note
		*    - A compositor pass instance can e.g. prefetch a render queue index ranges instance in here to avoid repeating this during runtime
		*    - The default implementation is empty
		*/
		inline virtual void onCompositorWorkspaceInstanceLoadingFinished()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Fill the compositor pass into the given commando buffer
		*
		*  @param[in] renderTarget
		*    Render target to render into
		*  @param[in] compositorContextData
		*    Compositor context data
		*  @param[out] commandBuffer
		*    Command buffer to fill
		*/
		virtual void onFillCommandBuffer(const Renderer::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Renderer::CommandBuffer& commandBuffer) = 0;

		/**
		*  @brief
		*    Called post command buffer execution
		*
		*  @note
		*    - The default implementation is empty
		*/
		inline virtual void onPostCommandBufferExecution()
		{
			// Nothing here
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline ICompositorInstancePass(const ICompositorResourcePass& compositorResourcePass, const CompositorNodeInstance& compositorNodeInstance) :
			mCompositorResourcePass(compositorResourcePass),
			mCompositorNodeInstance(compositorNodeInstance),
			mRenderTarget(nullptr),
			mNumberOfExecutionRequests(0)
		{
			// Nothing here
		}

		inline virtual ~ICompositorInstancePass()
		{
			// Nothing here
		}

		explicit ICompositorInstancePass(const ICompositorInstancePass&) = delete;
		ICompositorInstancePass& operator=(const ICompositorInstancePass&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const ICompositorResourcePass& mCompositorResourcePass;
		const CompositorNodeInstance&  mCompositorNodeInstance;
		Renderer::IRenderTarget*	   mRenderTarget;	/// Render target, can be a null pointer, don't destroy the instance
		uint32_t					   mNumberOfExecutionRequests;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
