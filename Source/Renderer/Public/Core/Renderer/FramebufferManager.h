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
#include "Renderer/Public/Core/Manager.h"
#include "Renderer/Public/Core/Renderer/FramebufferSignature.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP
#include <unordered_map>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class RenderPassManager;
	class RenderTargetTextureManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId CompositorFramebufferId;	///< Compositor framebuffer identifier, internally just a POD "uint32_t"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class FramebufferManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct FramebufferElement final
		{
			FramebufferSignature framebufferSignature;
			Rhi::IFramebuffer*	 framebuffer;			///< Can be a null pointer, no "Rhi::IFramebufferPtr" to not have overhead when internally reallocating
			uint32_t			 numberOfReferences;	///< Number of framebuffer references (don't misuse the RHI framebuffer reference counter for this)

			inline FramebufferElement() :
				framebuffer(nullptr),
				numberOfReferences(0)
			{
				// Nothing here
			}

			inline explicit FramebufferElement(const FramebufferSignature& _framebufferSignature) :
				framebufferSignature(_framebufferSignature),
				framebuffer(nullptr),
				numberOfReferences(0)
			{
				// Nothing here
			}

			inline FramebufferElement(const FramebufferSignature& _framebufferSignature, Rhi::IFramebuffer& _framebuffer) :
				framebufferSignature(_framebufferSignature),
				framebuffer(&_framebuffer),
				numberOfReferences(0)
			{
				// Nothing here
			}
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline FramebufferManager(RenderTargetTextureManager& renderTargetTextureManager, RenderPassManager& renderPassManager) :
			mRenderTargetTextureManager(renderTargetTextureManager),
			mRenderPassManager(renderPassManager)
		{
			// Nothing here
		}

		inline ~FramebufferManager()
		{
			// Nothing here
		}

		explicit FramebufferManager(const FramebufferManager&) = delete;
		FramebufferManager& operator=(const FramebufferManager&) = delete;
		void clear();
		void clearRhiResources();
		void addFramebuffer(CompositorFramebufferId compositorFramebufferId, const FramebufferSignature& framebufferSignature);
		[[nodiscard]] Rhi::IFramebuffer* getFramebufferByCompositorFramebufferId(CompositorFramebufferId compositorFramebufferId) const;
		[[nodiscard]] Rhi::IFramebuffer* getFramebufferByCompositorFramebufferId(CompositorFramebufferId compositorFramebufferId, const Rhi::IRenderTarget& mainRenderTarget, uint8_t numberOfMultisamples, float resolutionScale);
		void releaseFramebufferBySignature(const FramebufferSignature& framebufferSignature);


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<FramebufferElement> SortedFramebufferVector;
		typedef std::unordered_map<uint32_t, FramebufferSignatureId> CompositorFramebufferIdToFramebufferSignatureId;	///< Key = "Renderer::CompositorFramebufferId"


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		RenderTargetTextureManager&						mRenderTargetTextureManager;	///< Render target texture manager, just shared so don't destroy the instance
		RenderPassManager&								mRenderPassManager;				///< Render pass manager, just shared so don't destroy the instance
		SortedFramebufferVector							mSortedFramebufferVector;
		CompositorFramebufferIdToFramebufferSignatureId	mCompositorFramebufferIdToFramebufferSignatureId;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
