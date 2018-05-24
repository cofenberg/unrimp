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
#include "RendererRuntime/Core/StringId.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class CompositorTarget;
	class CompositorNodeInstance;
	class ICompositorResourcePass;
	class ICompositorInstancePass;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId CompositorPassTypeId;	///< Compositor pass type identifier, internally just a POD "uint32_t"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class ICompositorPassFactory
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorWorkspaceInstance;	// Needs to create compositor pass instances // TODO(co) Remove this
		friend class CompositorTarget;				// Needs to create compositor pass instances


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorPassFactory methods ]
	//[-------------------------------------------------------]
	protected:
		virtual ICompositorResourcePass* createCompositorResourcePass(const CompositorTarget& compositorTarget, CompositorPassTypeId compositorPassTypeId) const = 0;
		virtual ICompositorInstancePass* createCompositorInstancePass(const ICompositorResourcePass& compositorResourcePass, const CompositorNodeInstance& compositorNodeInstance) const = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline ICompositorPassFactory()
		{
			// Nothing here
		}

		inline virtual ~ICompositorPassFactory()
		{
			// Nothing here
		}

		explicit ICompositorPassFactory(const ICompositorPassFactory&) = delete;
		ICompositorPassFactory& operator=(const ICompositorPassFactory&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
