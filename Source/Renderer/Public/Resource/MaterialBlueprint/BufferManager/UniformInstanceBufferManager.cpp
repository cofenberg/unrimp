/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Renderer/Public/Resource/MaterialBlueprint/BufferManager/UniformInstanceBufferManager.h"
#include "Renderer/Public/Resource/MaterialBlueprint/Listener/MaterialBlueprintResourceListener.h"
#include "Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "Renderer/Public/Resource/Material/MaterialTechnique.h"
#include "Renderer/Public/RenderQueue/RenderableManager.h"
#include "Renderer/Public/Core/Math/Transform.h"
#include "Renderer/Public/IRenderer.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	UniformInstanceBufferManager::UniformInstanceBufferManager(IRenderer& renderer) :
		mRenderer(renderer),
		mMaximumUniformBufferSize(std::min(renderer.getRhi().getCapabilities().maximumUniformBufferSize, 64u * 1024u)),	// Default uniform buffer number of bytes: 64 KiB
		// Current instance buffer related data
		mCurrentInstanceBufferIndex(getInvalid<size_t>()),
		mCurrentInstanceBuffer(nullptr),
		mStartUniformBufferPointer(nullptr),
		mCurrentUniformBufferPointer(nullptr),
		mStartInstanceLocation(0)
	{
		// There must always be at least one instance buffer instance
		createInstanceBuffer();
	}

	UniformInstanceBufferManager::~UniformInstanceBufferManager()
	{
		// Release uniform buffer instances
		for (InstanceBuffer& instanceBuffer : mInstanceBuffers)
		{
			if (nullptr != instanceBuffer.resourceGroup)
			{
				instanceBuffer.resourceGroup->releaseReference();
			}
			instanceBuffer.uniformBuffer->releaseReference();
		}
	}

	void UniformInstanceBufferManager::startupBufferFilling(const MaterialBlueprintResource& materialBlueprintResource, Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity checks
		RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentInstanceBuffer, "Invalid current instance buffer")
		RHI_ASSERT(mRenderer.getContext(), isInvalid(materialBlueprintResource.getComputeShaderBlueprintResourceId()), "Invalid compute shader blueprint resource ID")

		// Map the current instance buffer
		mapCurrentInstanceBuffer();

		// Get buffer pointers
		const MaterialBlueprintResource::UniformBuffer* instanceUniformBuffer = materialBlueprintResource.getInstanceUniformBuffer();
		if (nullptr != instanceUniformBuffer)
		{
			// Create resource group, if needed
			if (nullptr == mCurrentInstanceBuffer->resourceGroup)
			{
				Rhi::IResource* resources[1] = { mCurrentInstanceBuffer->uniformBuffer };
				mCurrentInstanceBuffer->resourceGroup = materialBlueprintResource.getRootSignaturePtr()->createResourceGroup(instanceUniformBuffer->rootParameterIndex, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, nullptr RHI_RESOURCE_DEBUG_NAME("Uniform instance buffer manager"));
				mCurrentInstanceBuffer->resourceGroup->addReference();
			}

			// Set graphics resource group
			Rhi::Command::SetGraphicsResourceGroup::create(commandBuffer, instanceUniformBuffer->rootParameterIndex, mCurrentInstanceBuffer->resourceGroup);
		}
	}

	uint32_t UniformInstanceBufferManager::fillBuffer(const MaterialBlueprintResource& materialBlueprintResource, PassBufferManager* passBufferManager, const MaterialBlueprintResource::UniformBuffer& instanceUniformBuffer, const Renderable& renderable, MaterialTechnique& materialTechnique, Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity checks
		RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentInstanceBuffer, "Invalid current instance buffer")
		RHI_ASSERT(mRenderer.getContext(), nullptr != mStartUniformBufferPointer, "Invalid start uniform buffer pointer")
		RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentUniformBufferPointer, "Invalid current uniform buffer pointer")
		// RHI_ASSERT(mRenderer.getContext(), 0 == mStartInstanceLocation, "Invalid start instance location")	// Not done by intent
		RHI_ASSERT(mRenderer.getContext(), MaterialBlueprintResource::BufferUsage::INSTANCE == instanceUniformBuffer.bufferUsage, "Currently only the uniform buffer instance buffer usage is supported")

		// Get relevant data
		const Transform& objectSpaceToWorldSpaceTransform = renderable.getRenderableManager().getTransform();
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mRenderer.getMaterialBlueprintResourceManager();
		const MaterialProperties& globalMaterialProperties = materialBlueprintResourceManager.getGlobalMaterialProperties();
		IMaterialBlueprintResourceListener& materialBlueprintResourceListener = materialBlueprintResourceManager.getMaterialBlueprintResourceListener();
		const MaterialBlueprintResource::UniformBufferElementProperties& uniformBufferElementProperties = instanceUniformBuffer.uniformBufferElementProperties;
		const size_t numberOfUniformBufferElementProperties = uniformBufferElementProperties.size();
		static const PassBufferManager::PassData passData = {};
		materialBlueprintResourceListener.beginFillInstance((nullptr != passBufferManager) ? passBufferManager->getPassData() : passData, objectSpaceToWorldSpaceTransform, materialTechnique);

		{ // Handle instance buffer overflow
			// Calculate number of additionally needed uniform buffer bytes
			uint32_t newNeededUniformBufferSize = 0;
			for (size_t i = 0, numberOfPackageBytes = 0; i < numberOfUniformBufferElementProperties; ++i)
			{
				const MaterialProperty& uniformBufferElementProperty = uniformBufferElementProperties[i];

				// Get value type number of bytes
				const uint32_t valueTypeNumberOfBytes = uniformBufferElementProperty.getValueTypeNumberOfBytes(uniformBufferElementProperty.getValueType());

				// Handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
				if (0 != numberOfPackageBytes && numberOfPackageBytes + valueTypeNumberOfBytes > 16)
				{
					// Move the buffer pointer to the location of the next aligned package and restart the package bytes counter
					newNeededUniformBufferSize += static_cast<uint32_t>(sizeof(float) * 4 - numberOfPackageBytes);
					numberOfPackageBytes = 0;
				}
				numberOfPackageBytes += valueTypeNumberOfBytes % 16;

				// Next property
				newNeededUniformBufferSize += valueTypeNumberOfBytes;
			}

			// Detect and handle instance buffer overflow
			const uint32_t totalNeededUniformBufferSize = (static_cast<uint32_t>(mCurrentUniformBufferPointer - mStartUniformBufferPointer) + newNeededUniformBufferSize);
			if (totalNeededUniformBufferSize > mMaximumUniformBufferSize)
			{
				createInstanceBuffer();
				startupBufferFilling(materialBlueprintResource, commandBuffer);
			}
		}

		// Fill the uniform buffer
		for (size_t i = 0, numberOfPackageBytes = 0; i < numberOfUniformBufferElementProperties; ++i)
		{
			const MaterialProperty& uniformBufferElementProperty = uniformBufferElementProperties[i];

			// Get value type number of bytes
			const uint32_t valueTypeNumberOfBytes = uniformBufferElementProperty.getValueTypeNumberOfBytes(uniformBufferElementProperty.getValueType());

			// Handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
			if (0 != numberOfPackageBytes && numberOfPackageBytes + valueTypeNumberOfBytes > 16)
			{
				// Move the buffer pointer to the location of the next aligned package and restart the package bytes counter
				mCurrentUniformBufferPointer += sizeof(float) * 4 - numberOfPackageBytes;
				numberOfPackageBytes = 0;
			}
			numberOfPackageBytes += valueTypeNumberOfBytes % 16;

			// Copy the property value into the buffer
			const MaterialProperty::Usage usage = uniformBufferElementProperty.getUsage();
			if (MaterialProperty::Usage::INSTANCE_REFERENCE == usage)	// Most likely the case, so check this first
			{
				if (!materialBlueprintResourceListener.fillInstanceValue(uniformBufferElementProperty.getReferenceValue(), mCurrentUniformBufferPointer, valueTypeNumberOfBytes, ~0u))
				{
					// Error!
					RHI_ASSERT(mRenderer.getContext(), false, "Can't resolve reference")
				}
			}
			else if (MaterialProperty::Usage::GLOBAL_REFERENCE == usage)
			{
				// Referencing a global material property inside an instance uniform buffer doesn't make really sense performance wise, but don't forbid it

				// Figure out the global material property value
				const MaterialProperty* materialProperty = globalMaterialProperties.getPropertyById(uniformBufferElementProperty.getReferenceValue());
				if (nullptr != materialProperty)
				{
					// TODO(co) Error handling: Usage mismatch, value type mismatch etc.
					memcpy(mCurrentUniformBufferPointer, materialProperty->getData(), valueTypeNumberOfBytes);
				}
				else
				{
					// Try global material property reference fallback
					materialProperty = materialBlueprintResourceManager.getById(materialTechnique.getMaterialBlueprintResourceId()).getMaterialProperties().getPropertyById(uniformBufferElementProperty.getReferenceValue());
					if (nullptr != materialProperty)
					{
						// TODO(co) Error handling: Usage mismatch, value type mismatch etc.
						memcpy(mCurrentUniformBufferPointer, materialProperty->getData(), valueTypeNumberOfBytes);
					}
					else
					{
						// Error!
						RHI_ASSERT(mRenderer.getContext(), false, "Can't resolve reference")
					}
				}
			}
			else if (!uniformBufferElementProperty.isReferenceUsage())	// TODO(co) Performance: Think about such tests, the toolkit should already take care of this so we have well known verified runtime data
			{
				// Referencing a static uniform buffer element property inside an instance uniform buffer doesn't make really sense performance wise, but don't forbid it

				// Just copy over the property value
				memcpy(mCurrentUniformBufferPointer, uniformBufferElementProperty.getData(), valueTypeNumberOfBytes);
			}
			else
			{
				// Error!
				RHI_ASSERT(mRenderer.getContext(), false, "Invalid property")
			}

			// Next property
			mCurrentUniformBufferPointer += valueTypeNumberOfBytes;
		}

		// Done
		++mStartInstanceLocation;
		return mStartInstanceLocation - 1;
	}

	void UniformInstanceBufferManager::onPreCommandBufferExecution()
	{
		// Unmap the current instance buffer and reset the current instance buffer to the first instance
		if (isValid(mCurrentInstanceBufferIndex))
		{
			unmapCurrentInstanceBuffer();
			mCurrentInstanceBufferIndex = 0;
			mCurrentInstanceBuffer = &mInstanceBuffers[mCurrentInstanceBufferIndex];
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void UniformInstanceBufferManager::createInstanceBuffer()
	{
		Rhi::IBufferManager& bufferManager = mRenderer.getBufferManager();

		// Before doing anything else: Unmap the current instance buffer
		unmapCurrentInstanceBuffer();

		// Update current instance buffer
		mCurrentInstanceBufferIndex = isValid(mCurrentInstanceBufferIndex) ? (mCurrentInstanceBufferIndex + 1) : 0;
		if (mCurrentInstanceBufferIndex >= mInstanceBuffers.size())
		{
			// Create uniform buffer instance
			Rhi::IUniformBuffer* uniformBuffer = bufferManager.createUniformBuffer(mMaximumUniformBufferSize, nullptr, Rhi::BufferUsage::DYNAMIC_DRAW RHI_RESOURCE_DEBUG_NAME("Uniform instance buffer manager"));
			uniformBuffer->addReference();

			// Create instance buffer instance
			mInstanceBuffers.emplace_back(*uniformBuffer);
		}
		mCurrentInstanceBuffer = &mInstanceBuffers[mCurrentInstanceBufferIndex];
	}

	void UniformInstanceBufferManager::mapCurrentInstanceBuffer()
	{
		if (nullptr != mCurrentInstanceBuffer && !mCurrentInstanceBuffer->mapped)
		{
			// Sanity checks: Only one mapped instance buffer at a time
			RHI_ASSERT(mRenderer.getContext(), nullptr == mStartUniformBufferPointer, "Invalid start uniform buffer pointer")
			RHI_ASSERT(mRenderer.getContext(), nullptr == mCurrentUniformBufferPointer, "Invalid current uniform buffer pointer")
			RHI_ASSERT(mRenderer.getContext(), 0 == mStartInstanceLocation, "Invalid start instance location")

			// Map instance buffer
			Rhi::IRhi& rhi = mRenderer.getRhi();
			Rhi::MappedSubresource mappedSubresource;
			if (rhi.map(*mCurrentInstanceBuffer->uniformBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, mappedSubresource))
			{
				mStartUniformBufferPointer = mCurrentUniformBufferPointer = static_cast<uint8_t*>(mappedSubresource.data);
				mCurrentInstanceBuffer->mapped = true;
			}
			RHI_ASSERT(mRenderer.getContext(), mCurrentInstanceBuffer->mapped, "Current instance buffer isn't mapped")
		}
	}

	void UniformInstanceBufferManager::unmapCurrentInstanceBuffer()
	{
		if (nullptr != mCurrentInstanceBuffer && mCurrentInstanceBuffer->mapped)
		{
			// Sanity checks
			RHI_ASSERT(mRenderer.getContext(), nullptr != mStartUniformBufferPointer, "Invalid start uniform buffer pointer")
			RHI_ASSERT(mRenderer.getContext(), nullptr != mCurrentUniformBufferPointer, "Invalid current uniform buffer pointer")
			// RHI_ASSERT(mRenderer.getContext(), 0 == mStartInstanceLocation, "Invalid start instance location")	// Not done by intent

			// Unmap instance buffer
			Rhi::IRhi& rhi = mRenderer.getRhi();
			rhi.unmap(*mCurrentInstanceBuffer->uniformBuffer, 0);
			mCurrentInstanceBuffer->mapped = false;
			mStartUniformBufferPointer = nullptr;
			mCurrentUniformBufferPointer = nullptr;
			mStartInstanceLocation = 0;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
