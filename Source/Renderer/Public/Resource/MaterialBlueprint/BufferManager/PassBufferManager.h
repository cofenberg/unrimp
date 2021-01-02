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
#include "Renderer/Public/Core/Manager.h"

#include <Rhi/Public/Rhi.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
	#include <glm/gtx/quaternion.hpp>
PRAGMA_WARNING_POP

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
namespace Renderer
{
	class Transform;
	class IBufferManager;
	class IRenderer;
	class MaterialResource;
	class CompositorContextData;
	class MaterialBlueprintResource;
	class MaterialBlueprintResourceManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Pass buffer manager
	*/
	class PassBufferManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Pass data to each stereo rendering eye
		*/
		struct PassData final
		{
			glm::mat4 cameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[2];
			glm::mat4 previousCameraRelativeWorldSpaceToClipSpaceMatrixReversedZ[2];
			glm::mat4 cameraRelativeWorldSpaceToViewSpaceMatrix[2];
			glm::quat cameraRelativeWorldSpaceToViewSpaceQuaternion[2];
			glm::mat4 previousCameraRelativeWorldSpaceToViewSpaceMatrix[2];
			glm::mat4 viewSpaceToClipSpaceMatrix[2];
			glm::mat4 viewSpaceToClipSpaceMatrixReversedZ[2];
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Renderer instance to use
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource
		*/
		PassBufferManager(IRenderer& renderer, const MaterialBlueprintResource& materialBlueprintResource);

		/**
		*  @brief
		*    Destructor
		*/
		~PassBufferManager();

		/**
		*  @brief
		*    Fill the pass buffer
		*
		*  @param[in] renderTarget
		*    RHI render target to render into, must be valid for graphics pipeline and must be a null pointer for compute pipeline
		*  @param[in] compositorContextData
		*    Compositor context data
		*  @param[in] materialResource
		*    Currently used material resource
		*/
		void fillBuffer(const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, const MaterialResource& materialResource);

		/**
		*  @brief
		*    Return the pass data
		*
		*  @return
		*    The pass data filled inside "Renderer::PassBufferManager::fillBuffer()"
		*/
		[[nodiscard]] inline const PassData& getPassData() const
		{
			return mPassData;
		}

		/**
		*  @brief
		*    Bind the currently used pass buffer into the given graphics command buffer
		*
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*/
		void fillGraphicsCommandBuffer(Rhi::CommandBuffer& commandBuffer) const;

		/**
		*  @brief
		*    Bind the currently used pass buffer into the given compute command buffer
		*
		*  @param[out] commandBuffer
		*    RHI command buffer to fill
		*/
		void fillComputeCommandBuffer(Rhi::CommandBuffer& commandBuffer) const;

		/**
		*  @brief
		*    Called post command buffer dispatch
		*/
		inline void onPostCommandBufferDispatch()
		{
			mCurrentUniformBufferIndex = 0;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit PassBufferManager(const PassBufferManager&) = delete;
		PassBufferManager& operator=(const PassBufferManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct UniformBuffer final
		{
			Rhi::IUniformBuffer* uniformBuffer;
			Rhi::IResourceGroup* resourceGroup;

			UniformBuffer(Rhi::IUniformBuffer* _uniformBuffer, Rhi::IResourceGroup* _resourceGroup) :
				uniformBuffer(_uniformBuffer),
				resourceGroup(_resourceGroup)
			{
				uniformBuffer->addReference();
				resourceGroup->addReference();
			}
		};
		typedef std::vector<UniformBuffer> UniformBuffers;
		typedef std::vector<uint8_t>	   ScratchBuffer;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRenderer&								mRenderer;
		Rhi::IBufferManager&					mBufferManager;
		const MaterialBlueprintResource&		mMaterialBlueprintResource;
		const MaterialBlueprintResourceManager&	mMaterialBlueprintResourceManager;
		PassData								mPassData;
		UniformBuffers							mUniformBuffers;
		uint32_t								mCurrentUniformBufferIndex;
		ScratchBuffer							mScratchBuffer;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
