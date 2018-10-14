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
#include "RendererRuntime/Public/Core/StringId.h"
#include "RendererRuntime/Public/Core/GetInvalid.h"

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
	class ICompositorPassFactory;
	class ICompositorResourcePass;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId CompositorChannelId;		///< Compositor channel identifier, internally just a POD "uint32_t"
	typedef StringId CompositorFramebufferId;	///< Compositor framebuffer identifier, internally just a POD "uint32_t"
	typedef StringId CompositorPassTypeId;		///< Compositor pass type identifier, internally just a POD "uint32_t"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compositor node resource target
	*
	*  @note
	*    - Named in short just compositor target because there's no instance variant
	*    - Render target might be compositor channel (external interconnection) or compositor framebuffer (node internal processing)
	*/
	class CompositorTarget final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<ICompositorResourcePass*> CompositorResourcePasses;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline explicit CompositorTarget(CompositorChannelId compositorChannelId, CompositorFramebufferId compositorFramebufferId) :
			mCompositorChannelId(compositorChannelId),
			mCompositorFramebufferId(compositorFramebufferId)
		{
			// Nothing here
		}

		inline explicit CompositorTarget(const CompositorTarget& compositorTarget) :
			mCompositorChannelId(compositorTarget.mCompositorChannelId),
			mCompositorFramebufferId(compositorTarget.mCompositorFramebufferId)
		{
			// Nothing here
		}

		inline ~CompositorTarget()
		{
			removeAllCompositorResourcePasses();
		}

		[[nodiscard]] inline CompositorChannelId getCompositorChannelId() const
		{
			return mCompositorChannelId;
		}

		[[nodiscard]] inline CompositorFramebufferId getCompositorFramebufferId() const
		{
			return mCompositorFramebufferId;
		}

		//[-------------------------------------------------------]
		//[ Passes                                                ]
		//[-------------------------------------------------------]
		inline void setNumberOfCompositorResourcePasses(uint32_t numberOfCompositorResourcePasses)
		{
			mCompositorResourcePasses.reserve(numberOfCompositorResourcePasses);
		}

		[[nodiscard]] ICompositorResourcePass* addCompositorResourcePass(const ICompositorPassFactory& compositorPassFactory, CompositorPassTypeId compositorPassTypeId);

		[[nodiscard]] inline const CompositorResourcePasses& getCompositorResourcePasses() const
		{
			return mCompositorResourcePasses;
		}

		void removeAllCompositorResourcePasses();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		CompositorTarget& operator=(const CompositorTarget&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		CompositorChannelId		 mCompositorChannelId;
		CompositorFramebufferId	 mCompositorFramebufferId;
		CompositorResourcePasses mCompositorResourcePasses;	///< We're responsible for destroying the instances if we no longer need them


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
