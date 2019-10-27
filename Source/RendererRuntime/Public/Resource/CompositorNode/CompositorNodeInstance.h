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

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class CompositorContextData;
	class ICompositorInstancePass;
	class CompositorWorkspaceInstance;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t CompositorNodeResourceId;	///< POD compositor node resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class CompositorNodeInstance final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorWorkspaceInstance;	// Needs to create and execute compositor node instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<ICompositorInstancePass*> CompositorInstancePasses;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline CompositorNodeResourceId getCompositorNodeResourceId() const
		{
			return mCompositorNodeResourceId;
		}

		[[nodiscard]] inline const CompositorWorkspaceInstance& getCompositorWorkspaceInstance() const
		{
			return mCompositorWorkspaceInstance;
		}

		[[nodiscard]] inline const CompositorInstancePasses& getCompositorInstancePasses() const
		{
			return mCompositorInstancePasses;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline CompositorNodeInstance(CompositorNodeResourceId compositorNodeResourceId, const CompositorWorkspaceInstance& compositorWorkspaceInstance) :
			mCompositorNodeResourceId(compositorNodeResourceId),
			mCompositorWorkspaceInstance(compositorWorkspaceInstance)
		{
			// Nothing here
		}

		~CompositorNodeInstance();
		explicit CompositorNodeInstance(const CompositorNodeInstance&) = delete;
		CompositorNodeInstance& operator=(const CompositorNodeInstance&) = delete;
		void compositorWorkspaceInstanceLoadingFinished() const;
		[[nodiscard]] Rhi::IRenderTarget& fillCommandBuffer(Rhi::IRenderTarget& renderTarget, const CompositorContextData& compositorContextData, Rhi::CommandBuffer& commandBuffer) const;

		/**
		*  @brief
		*    Called post command buffer execution
		*/
		void onPostCommandBufferExecution() const;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		CompositorNodeResourceId		   mCompositorNodeResourceId;
		const CompositorWorkspaceInstance& mCompositorWorkspaceInstance;
		CompositorInstancePasses		   mCompositorInstancePasses;	///< Owns the compositor instance passes and has to destroy them if no longer needed


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
