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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/PassBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Listener/IMaterialBlueprintResourceListener.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Core/Math/Math.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	PassBufferManager::PassBufferManager(IRendererRuntime& rendererRuntime, const MaterialBlueprintResource& materialBlueprintResource) :
		mRendererRuntime(rendererRuntime),
		mBufferManager(rendererRuntime.getBufferManager()),
		mMaterialBlueprintResource(materialBlueprintResource),
		mMaterialBlueprintResourceManager(rendererRuntime.getMaterialBlueprintResourceManager()),
		mPassData
		{
			{ Math::MAT4_IDENTITY, Math::MAT4_IDENTITY },	// cameraRelativeWorldSpaceToClipSpaceMatrixReversedZ (glm::mat4)
			{ Math::MAT4_IDENTITY, Math::MAT4_IDENTITY },	// previousCameraRelativeWorldSpaceToClipSpaceMatrixReversedZ (glm::mat4)
			{ Math::MAT4_IDENTITY, Math::MAT4_IDENTITY },	// cameraRelativeWorldSpaceToViewSpaceMatrix (glm::mat4)
			{ Math::QUAT_IDENTITY, Math::QUAT_IDENTITY },	// cameraRelativeWorldSpaceToViewSpaceQuaternion (glm::quat)
			{ Math::MAT4_IDENTITY, Math::MAT4_IDENTITY },	// previousCameraRelativeWorldSpaceToViewSpaceMatrix (glm::mat4)
			{ Math::MAT4_IDENTITY, Math::MAT4_IDENTITY },	// viewSpaceToClipSpaceMatrix (glm::mat4)
			{ Math::MAT4_IDENTITY, Math::MAT4_IDENTITY }	// viewSpaceToClipSpaceMatrixReversedZ (glm::mat4)
		},
		mCurrentUniformBufferIndex(0)
	{
		const MaterialBlueprintResource::UniformBuffer* passUniformBuffer = mMaterialBlueprintResource.getPassUniformBuffer();
		if (nullptr != passUniformBuffer)
		{
			mScratchBuffer.resize(passUniformBuffer->uniformBufferNumberOfBytes);
		}
	}

	PassBufferManager::~PassBufferManager()
	{
		// Destroy all uniform buffers
		for (UniformBuffer& uniformBuffer : mUniformBuffers)
		{
			uniformBuffer.resourceGroup->releaseReference();
			uniformBuffer.uniformBuffer->releaseReference();
		}
	}

	void PassBufferManager::fillBuffer(const Rhi::IRenderTarget* renderTarget, const CompositorContextData& compositorContextData, const MaterialResource& materialResource)
	{
		// Even if there's no pass uniform buffer, there must still be a pass buffer manager filling "RendererRuntime::PassBufferManager::PassData" which is used to fill the instances texture buffer

		// Sanity checks: The render target to render into must be valid for graphics pipeline and must be a null pointer for compute pipeline
		RHI_ASSERT(mRendererRuntime.getContext(), isValid(mMaterialBlueprintResource.getComputeShaderBlueprintResourceId()) || nullptr != renderTarget, "Graphics pipeline used but render target is invalid")
		RHI_ASSERT(mRendererRuntime.getContext(), isInvalid(mMaterialBlueprintResource.getComputeShaderBlueprintResourceId()) || nullptr == renderTarget, "Compute pipeline used but render target is valid")

		// Tell the material blueprint resource listener that we're about to fill a pass uniform buffer
		IMaterialBlueprintResourceListener& materialBlueprintResourceListener = mMaterialBlueprintResourceManager.getMaterialBlueprintResourceListener();
		materialBlueprintResourceListener.beginFillPass(mRendererRuntime, renderTarget, compositorContextData, mPassData);

		// Get the pass uniform buffer containing the description of the element properties
		const MaterialBlueprintResource::UniformBuffer* passUniformBuffer = mMaterialBlueprintResource.getPassUniformBuffer();
		if (nullptr != passUniformBuffer)
		{
			// Startup the pass uniform buffer update
			uint8_t* scratchBufferPointer = mScratchBuffer.data();

			{ // Fill the pass uniform buffer by using the material blueprint resource
				const MaterialProperties& globalMaterialProperties = mMaterialBlueprintResourceManager.getGlobalMaterialProperties();
				const MaterialBlueprintResource::UniformBufferElementProperties& uniformBufferElementProperties = passUniformBuffer->uniformBufferElementProperties;
				const size_t numberOfUniformBufferElementProperties = uniformBufferElementProperties.size();
				for (size_t i = 0, numberOfPackageBytes = 0; i < numberOfUniformBufferElementProperties; ++i)
				{
					const MaterialProperty& uniformBufferElementProperty = uniformBufferElementProperties[i];

					// Get value type number of bytes
					const uint32_t valueTypeNumberOfBytes = uniformBufferElementProperty.getValueTypeNumberOfBytes(uniformBufferElementProperty.getValueType());

					// Handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
					if (0 != numberOfPackageBytes && numberOfPackageBytes + valueTypeNumberOfBytes > 16)
					{
						// Move the current buffer pointer to the location of the next aligned package and restart the package bytes counter
						scratchBufferPointer += 4 * 4 - numberOfPackageBytes;
						numberOfPackageBytes = 0;
					}
					numberOfPackageBytes += valueTypeNumberOfBytes % 16;

					// Copy the property value into the current buffer
					const MaterialProperty::Usage usage = uniformBufferElementProperty.getUsage();
					if (MaterialProperty::Usage::PASS_REFERENCE == usage)	// Most likely the case, so check this first
					{
						if (!materialBlueprintResourceListener.fillPassValue(uniformBufferElementProperty.getReferenceValue(), scratchBufferPointer, valueTypeNumberOfBytes))
						{
							// Error!
							RHI_ASSERT(mRendererRuntime.getContext(), false, "Can't resolve reference")
						}
					}
					else if (MaterialProperty::Usage::GLOBAL_REFERENCE == usage)
					{
						// Figure out the global material property value
						const MaterialProperty* materialProperty = globalMaterialProperties.getPropertyById(uniformBufferElementProperty.getReferenceValue());
						if (nullptr != materialProperty)
						{
							// TODO(co) Error handling: Usage mismatch, value type mismatch etc.
							memcpy(scratchBufferPointer, materialProperty->getData(), valueTypeNumberOfBytes);
						}
						else
						{
							// Try global material property reference fallback
							materialProperty = mMaterialBlueprintResource.getMaterialProperties().getPropertyById(uniformBufferElementProperty.getReferenceValue());
							if (nullptr != materialProperty)
							{
								// TODO(co) Error handling: Usage mismatch, value type mismatch etc.
								memcpy(scratchBufferPointer, materialProperty->getData(), valueTypeNumberOfBytes);
							}
							else
							{
								// Error!
								RHI_ASSERT(mRendererRuntime.getContext(), false, "Can't resolve reference")
							}
						}
					}
					else if (MaterialProperty::Usage::MATERIAL_REFERENCE == usage)
					{
						// Figure out the material property value
						const MaterialProperty* materialProperty = materialResource.getPropertyById(uniformBufferElementProperty.getReferenceValue());
						if (nullptr != materialProperty)
						{
							// TODO(co) Error handling: Usage mismatch, value type mismatch etc.
							memcpy(scratchBufferPointer, materialProperty->getData(), valueTypeNumberOfBytes);
						}
						else if (!materialBlueprintResourceListener.fillMaterialValue(uniformBufferElementProperty.getReferenceValue(), scratchBufferPointer, valueTypeNumberOfBytes))
						{
							// Error!
							RHI_ASSERT(mRendererRuntime.getContext(), false, "Can't resolve reference")
						}
					}
					else if (!uniformBufferElementProperty.isReferenceUsage())
					{
						// Just copy over the property value
						memcpy(scratchBufferPointer, uniformBufferElementProperty.getData(), valueTypeNumberOfBytes);
					}
					else
					{
						// Error!
						RHI_ASSERT(mRendererRuntime.getContext(), false, "Invalid property")
					}

					// Next property
					scratchBufferPointer += valueTypeNumberOfBytes;
				}
			}

			// Create new uniform buffer, if necessary
			if (mCurrentUniformBufferIndex >= static_cast<uint32_t>(mUniformBuffers.size()))
			{
				// Don't directly pass along data or the GPU driver might get confused about the usage and might output performance warnings
				Rhi::IResource* uniformBuffer = mBufferManager.createUniformBuffer(passUniformBuffer->uniformBufferNumberOfBytes, nullptr, Rhi::BufferUsage::DYNAMIC_DRAW);
				RHI_SET_RESOURCE_DEBUG_NAME(uniformBuffer, "Pass buffer manager")
				Rhi::IResourceGroup* resourceGroup = mMaterialBlueprintResource.getRootSignaturePtr()->createResourceGroup(passUniformBuffer->rootParameterIndex, 1, &uniformBuffer);
				RHI_SET_RESOURCE_DEBUG_NAME(resourceGroup, "Pass buffer manager")
				mUniformBuffers.emplace_back(static_cast<Rhi::IUniformBuffer*>(uniformBuffer), resourceGroup);
			}

			{ // Update the uniform buffer by using our scratch buffer
				Rhi::IUniformBuffer* uniformBuffer = mUniformBuffers[mCurrentUniformBufferIndex].uniformBuffer;
				Rhi::MappedSubresource mappedSubresource;
				Rhi::IRhi& rhi = mRendererRuntime.getRhi();
				if (rhi.map(*uniformBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, mappedSubresource))
				{
					memcpy(mappedSubresource.data, mScratchBuffer.data(), static_cast<uint32_t>(mScratchBuffer.size()));
					rhi.unmap(*uniformBuffer, 0);
				}
			}
			++mCurrentUniformBufferIndex;
		}
	}

	void PassBufferManager::fillGraphicsCommandBuffer(Rhi::CommandBuffer& commandBuffer) const
	{
		// Set resource group
		if (!mUniformBuffers.empty())
		{
			const MaterialBlueprintResource::UniformBuffer* passUniformBuffer = mMaterialBlueprintResource.getPassUniformBuffer();
			if (nullptr != passUniformBuffer)
			{
				Rhi::Command::SetGraphicsResourceGroup::create(commandBuffer, passUniformBuffer->rootParameterIndex, mUniformBuffers[mCurrentUniformBufferIndex - 1].resourceGroup);
			}
		}
	}

	void PassBufferManager::fillComputeCommandBuffer(Rhi::CommandBuffer& commandBuffer) const
	{
		// Set resource group
		if (!mUniformBuffers.empty())
		{
			const MaterialBlueprintResource::UniformBuffer* passUniformBuffer = mMaterialBlueprintResource.getPassUniformBuffer();
			if (nullptr != passUniformBuffer)
			{
				Rhi::Command::SetComputeResourceGroup::create(commandBuffer, passUniformBuffer->rootParameterIndex, mUniformBuffers[mCurrentUniformBufferIndex - 1].resourceGroup);
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
