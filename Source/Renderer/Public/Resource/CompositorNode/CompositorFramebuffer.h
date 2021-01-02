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
#include "Renderer/Public/Core/StringId.h"
#include "Renderer/Public/Core/Renderer/FramebufferSignature.h"


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
	/**
	*  @brief
	*    Compositor framebuffer; used for compositor workspace and nodes intermediate rendering results
	*/
	class CompositorFramebuffer final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline CompositorFramebuffer(CompositorFramebufferId compositorFramebufferId, const FramebufferSignature& framebufferSignature) :
			mCompositorFramebufferId(compositorFramebufferId),
			mFramebufferSignature(framebufferSignature)
		{
			// Nothing here
		}

		inline explicit CompositorFramebuffer(const CompositorFramebuffer& compositorFramebuffer) :
			mCompositorFramebufferId(compositorFramebuffer.mCompositorFramebufferId),
			mFramebufferSignature(compositorFramebuffer.mFramebufferSignature)
		{
			// Nothing here
		}

		inline ~CompositorFramebuffer()
		{
			// Nothing here
		}

		inline CompositorFramebuffer& operator=(const CompositorFramebuffer& compositorFramebuffer)
		{
			mCompositorFramebufferId = compositorFramebuffer.mCompositorFramebufferId;
			mFramebufferSignature	 = compositorFramebuffer.mFramebufferSignature;
			return *this;
		}

		[[nodiscard]] inline CompositorFramebufferId getCompositorFramebufferId() const
		{
			return mCompositorFramebufferId;
		}

		[[nodiscard]] inline const FramebufferSignature& getFramebufferSignature() const
		{
			return mFramebufferSignature;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		CompositorFramebufferId mCompositorFramebufferId;
		FramebufferSignature	mFramebufferSignature;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
