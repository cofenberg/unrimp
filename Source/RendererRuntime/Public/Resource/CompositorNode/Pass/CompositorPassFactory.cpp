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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/CompositorPassFactory.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Clear/CompositorResourcePassClear.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Clear/CompositorInstancePassClear.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorResourcePassVrHiddenAreaMesh.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorInstancePassVrHiddenAreaMesh.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Compute/CompositorInstancePassCompute.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Copy/CompositorResourcePassCopy.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/Copy/CompositorInstancePassCopy.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorInstancePassGenerateMipmaps.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorResourcePassShadowMap.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/DebugGui/CompositorInstancePassDebugGui.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ResolveMultisample/CompositorResourcePassResolveMultisample.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ResolveMultisample/CompositorInstancePassResolveMultisample.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::ICompositorPassFactory methods ]
	//[-------------------------------------------------------]
	ICompositorResourcePass* CompositorPassFactory::createCompositorResourcePass(const CompositorTarget& compositorTarget, CompositorPassTypeId compositorPassTypeId) const
	{
		ICompositorResourcePass* compositorResourcePass = nullptr;

		// Define helper macro
		#define CASE_VALUE(resource) case resource::TYPE_ID: compositorResourcePass = new resource(compositorTarget); break;

		// Evaluate the compositor pass type
		switch (compositorPassTypeId)
		{
			CASE_VALUE(CompositorResourcePassClear)
			CASE_VALUE(CompositorResourcePassVrHiddenAreaMesh)
			CASE_VALUE(CompositorResourcePassScene)
			CASE_VALUE(CompositorResourcePassShadowMap)
			CASE_VALUE(CompositorResourcePassResolveMultisample)
			CASE_VALUE(CompositorResourcePassCopy)
			CASE_VALUE(CompositorResourcePassGenerateMipmaps)
			CASE_VALUE(CompositorResourcePassCompute)
			CASE_VALUE(CompositorResourcePassDebugGui)
		}

		// Undefine helper macro
		#undef CASE_VALUE

		// Done
		return compositorResourcePass;
	}

	ICompositorInstancePass* CompositorPassFactory::createCompositorInstancePass(const ICompositorResourcePass& compositorResourcePass, const CompositorNodeInstance& compositorNodeInstance) const
	{
		ICompositorInstancePass* compositorInstancePass = nullptr;

		// Define helper macro
		#define CASE_VALUE(resource, instance) case resource::TYPE_ID: compositorInstancePass = new instance(static_cast<const resource&>(compositorResourcePass), compositorNodeInstance); break;

		// Evaluate the compositor pass type
		switch (compositorResourcePass.getTypeId())
		{
			CASE_VALUE(CompositorResourcePassClear,				 CompositorInstancePassClear)
			CASE_VALUE(CompositorResourcePassVrHiddenAreaMesh,	 CompositorInstancePassVrHiddenAreaMesh)
			CASE_VALUE(CompositorResourcePassScene,				 CompositorInstancePassScene)
			CASE_VALUE(CompositorResourcePassShadowMap,			 CompositorInstancePassShadowMap)
			CASE_VALUE(CompositorResourcePassResolveMultisample, CompositorInstancePassResolveMultisample)
			CASE_VALUE(CompositorResourcePassCopy,				 CompositorInstancePassCopy)
			CASE_VALUE(CompositorResourcePassGenerateMipmaps,	 CompositorInstancePassGenerateMipmaps)
			CASE_VALUE(CompositorResourcePassCompute,			 CompositorInstancePassCompute)
			CASE_VALUE(CompositorResourcePassDebugGui,			 CompositorInstancePassDebugGui)
		}

		// Undefine helper macro
		#undef CASE_VALUE

		// Done
		return compositorInstancePass;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
