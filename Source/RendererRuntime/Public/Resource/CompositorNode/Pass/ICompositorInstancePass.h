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
#include "RendererRuntime/Public/Core/Platform/PlatformTypes.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
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
		[[nodiscard]] inline const ICompositorResourcePass& getCompositorResourcePass() const
		{
			return mCompositorResourcePass;
		}

		[[nodiscard]] inline const CompositorNodeInstance& getCompositorNodeInstance() const
		{
			return mCompositorNodeInstance;
		}

		[[nodiscard]] inline Rhi::IRenderTarget* getRenderTarget() const
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
		*    Fill the compositor pass into the given command buffer
		*
		*  @param[in] renderTarget
		*    RHI render target to render into, can be a null pointer (e.g. for compute shader or resource copy compositor passes)
		*  @param[in] compositorContextData
		*    Compositor context data
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*/
		virtual void onFillCommandBuffer(const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer) = 0;

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
		Rhi::IRenderTarget*			   mRenderTarget;	/// Render target, can be a null pointer, don't destroy the instance
		uint32_t					   mNumberOfExecutionRequests;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
