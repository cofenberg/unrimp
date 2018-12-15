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
#include "RendererRuntime/Public/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Listener/IMaterialBlueprintResourceListener.h"
#include "RendererRuntime/Public/Resource/Material/MaterialTechnique.h"
#include "RendererRuntime/Public/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Public/Core/SwizzleVectorElementRemove.h"
#include "RendererRuntime/Public/IRendererRuntime.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	MaterialBufferManager::MaterialBufferManager(IRendererRuntime& rendererRuntime, const MaterialBlueprintResource& materialBlueprintResource) :
		mRendererRuntime(rendererRuntime),
		mMaterialBlueprintResource(materialBlueprintResource),
		mSlotsPerPool(0),
		mBufferSize(0),
		mLastGraphicsBoundPool(nullptr),
		mLastComputeBoundPool(nullptr)
	{
		const MaterialBlueprintResource::UniformBuffer* materialUniformBuffer = mMaterialBlueprintResource.getMaterialUniformBuffer();
		RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != materialUniformBuffer, "Invalid material uniform buffer")

		// Get the buffer size
		mBufferSize = std::min<uint32_t>(rendererRuntime.getRenderer().getCapabilities().maximumUniformBufferSize, 64 * 1024);
		mScratchBuffer.resize(mBufferSize);

		// Calculate the number of slots per pool
		const uint32_t numberOfBytesPerElement = materialUniformBuffer->uniformBufferNumberOfBytes / materialUniformBuffer->numberOfElements;
		mSlotsPerPool = mBufferSize / numberOfBytesPerElement;
	}

	MaterialBufferManager::~MaterialBufferManager()
	{
		for (BufferPool* bufferPool : mBufferPools)
		{
			delete bufferPool;
		}
	}

	void MaterialBufferManager::requestSlot(MaterialBufferSlot& materialBufferSlot)
	{
		// Release slot, if required
		if (isValid(materialBufferSlot.mAssignedMaterialPool))
		{
			releaseSlot(materialBufferSlot);
		}

		// Find a buffer pool with a free slot
		BufferPools::iterator iterator = mBufferPools.begin();
		BufferPools::iterator iteratorEnd  = mBufferPools.end();
		while (iterator != iteratorEnd && (*iterator)->freeSlots.empty())
		{
			++iterator;
		}
		if (iterator == iteratorEnd)
		{
			mBufferPools.push_back(new BufferPool(mBufferSize, mSlotsPerPool, mRendererRuntime.getBufferManager(), mMaterialBlueprintResource));
			iterator = mBufferPools.end() - 1;
		}

		// Setup received slot
		BufferPool* bufferPool = *iterator;
		materialBufferSlot.mAssignedMaterialPool = bufferPool;
		materialBufferSlot.mAssignedMaterialSlot = bufferPool->freeSlots.back();
		materialBufferSlot.mGlobalIndex			 = static_cast<int>(mMaterialBufferSlots.size());
		mMaterialBufferSlots.push_back(&materialBufferSlot);
		bufferPool->freeSlots.pop_back();
		scheduleForUpdate(materialBufferSlot);
	}

	void MaterialBufferManager::releaseSlot(MaterialBufferSlot& materialBufferSlot)
	{
		BufferPool* bufferPool = static_cast<BufferPool*>(materialBufferSlot.mAssignedMaterialPool);

		// Sanity checks
		RENDERER_ASSERT(mRendererRuntime.getContext(), isValid(materialBufferSlot.mAssignedMaterialPool), "Invalid assigned material pool")
		RENDERER_ASSERT(mRendererRuntime.getContext(), isValid(materialBufferSlot.mAssignedMaterialSlot), "Invalid assigned material slot")
		RENDERER_ASSERT(mRendererRuntime.getContext(), materialBufferSlot.mAssignedMaterialSlot < mSlotsPerPool, "Invalid assigned material slot")
		RENDERER_ASSERT(mRendererRuntime.getContext(), std::find(bufferPool->freeSlots.begin(), bufferPool->freeSlots.end(), materialBufferSlot.mAssignedMaterialSlot) == bufferPool->freeSlots.end(), "Invalid assigned material slot")
		RENDERER_ASSERT(mRendererRuntime.getContext(), materialBufferSlot.mGlobalIndex < static_cast<int>(mMaterialBufferSlots.size()), "Invalid global index")
		RENDERER_ASSERT(mRendererRuntime.getContext(), &materialBufferSlot == *(mMaterialBufferSlots.begin() + materialBufferSlot.mGlobalIndex), "Invalid global index")

		// If the slot is dirty, remove it from the list of dirty slots
		if (materialBufferSlot.mDirty)
		{
			MaterialBufferSlots::iterator iterator = std::find(mDirtyMaterialBufferSlots.begin(), mDirtyMaterialBufferSlots.end(), &materialBufferSlot);
			if (iterator != mDirtyMaterialBufferSlots.end())
			{
				::detail::swizzleVectorElementRemove(mDirtyMaterialBufferSlots, iterator);
			}
		}

		// Put the slot back to the list of free slots
		bufferPool->freeSlots.push_back(materialBufferSlot.mAssignedMaterialSlot);
		materialBufferSlot.mAssignedMaterialPool = nullptr;
		materialBufferSlot.mAssignedMaterialSlot = getInvalid<uint32_t>();
		materialBufferSlot.mDirty				 = false;
		MaterialBufferSlots::iterator iterator = mMaterialBufferSlots.begin() + materialBufferSlot.mGlobalIndex;
		iterator = ::detail::swizzleVectorElementRemove(mMaterialBufferSlots, iterator);
		if (iterator != mMaterialBufferSlots.end())
		{
			// The node that was at the end got swapped and has now a different index
			(*iterator)->mGlobalIndex = static_cast<int>(iterator - mMaterialBufferSlots.begin());
		}
	}

	void MaterialBufferManager::scheduleForUpdate(MaterialBufferSlot& materialBufferSlot)
	{
		if (!materialBufferSlot.mDirty)
		{
			mDirtyMaterialBufferSlots.push_back(&materialBufferSlot);
			materialBufferSlot.mDirty = true;
		}
	}

	void MaterialBufferManager::resetLastGraphicsBoundPool()
	{
		mLastGraphicsBoundPool = nullptr;
		if (!mDirtyMaterialBufferSlots.empty())
		{
			uploadDirtySlots();
		}
	}

	void MaterialBufferManager::resetLastComputeBoundPool()
	{
		mLastComputeBoundPool = nullptr;
		if (!mDirtyMaterialBufferSlots.empty())
		{
			uploadDirtySlots();
		}
	}

	void MaterialBufferManager::fillGraphicsCommandBuffer(MaterialBufferSlot& materialBufferSlot, Renderer::CommandBuffer& commandBuffer)
	{
		if (mLastGraphicsBoundPool != materialBufferSlot.mAssignedMaterialPool)
		{
			mLastGraphicsBoundPool = static_cast<BufferPool*>(materialBufferSlot.mAssignedMaterialPool);
			RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != mLastGraphicsBoundPool, "Invalid last graphics bound pool")

			// Set resource group
			const MaterialBlueprintResource::UniformBuffer* materialUniformBuffer = mMaterialBlueprintResource.getMaterialUniformBuffer();
			RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != materialUniformBuffer, "Invalid material uniform buffer")
			Renderer::Command::SetGraphicsResourceGroup::create(commandBuffer, materialUniformBuffer->rootParameterIndex, mLastGraphicsBoundPool->resourceGroup);
		}
	}

	void MaterialBufferManager::fillComputeCommandBuffer(MaterialBufferSlot& materialBufferSlot, Renderer::CommandBuffer& commandBuffer)
	{
		if (mLastComputeBoundPool != materialBufferSlot.mAssignedMaterialPool)
		{
			mLastComputeBoundPool = static_cast<BufferPool*>(materialBufferSlot.mAssignedMaterialPool);
			RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != mLastComputeBoundPool, "Invalid last compute bound pool")

			// Set resource group
			const MaterialBlueprintResource::UniformBuffer* materialUniformBuffer = mMaterialBlueprintResource.getMaterialUniformBuffer();
			RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != materialUniformBuffer, "Invalid material uniform buffer")
			Renderer::Command::SetComputeResourceGroup::create(commandBuffer, materialUniformBuffer->rootParameterIndex, mLastComputeBoundPool->resourceGroup);
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void MaterialBufferManager::uploadDirtySlots()
	{
		RENDERER_ASSERT(mRendererRuntime.getContext(), !mDirtyMaterialBufferSlots.empty(), "Invalid dirty material buffer slots")
		const MaterialBlueprintResource::UniformBuffer* materialUniformBuffer = mMaterialBlueprintResource.getMaterialUniformBuffer();
		RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != materialUniformBuffer, "Invalid material uniform buffer")
		const MaterialBlueprintResourceManager& materialBlueprintResourceManager = mMaterialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>();
		const MaterialProperties& globalMaterialProperties = materialBlueprintResourceManager.getGlobalMaterialProperties();
		IMaterialBlueprintResourceListener& materialBlueprintResourceListener = materialBlueprintResourceManager.getMaterialBlueprintResourceListener();
		materialBlueprintResourceListener.beginFillMaterial();

		// Update the scratch buffer
		Renderer::IUniformBuffer* uniformBuffer = nullptr;	// TODO(co) Implement proper uniform buffer handling and only update dirty sections
		{
			const MaterialBlueprintResource::UniformBufferElementProperties& uniformBufferElementProperties = materialUniformBuffer->uniformBufferElementProperties;
			const size_t numberOfUniformBufferElementProperties = uniformBufferElementProperties.size();
			const uint32_t numberOfBytesPerElement = materialUniformBuffer->uniformBufferNumberOfBytes / materialUniformBuffer->numberOfElements;
			for (MaterialBufferSlot* materialBufferSlot : mDirtyMaterialBufferSlots)
			{
				const MaterialResource& materialResource = materialBufferSlot->getMaterialResource();
				uint8_t* scratchBufferPointer = mScratchBuffer.data() + numberOfBytesPerElement * materialBufferSlot->mAssignedMaterialSlot;

				// TODO(co) Implement proper uniform buffer handling and only update dirty sections
				uniformBuffer = static_cast<BufferPool*>(materialBufferSlot->mAssignedMaterialPool)->uniformBuffer;

				for (size_t i = 0, numberOfPackageBytes = 0; i < numberOfUniformBufferElementProperties; ++i)
				{
					const MaterialProperty& uniformBufferElementProperty = uniformBufferElementProperties[i];

					// Get value type number of bytes
					const uint32_t valueTypeNumberOfBytes = uniformBufferElementProperty.getValueTypeNumberOfBytes(uniformBufferElementProperty.getValueType());

					// Handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
					if (0 != numberOfPackageBytes && numberOfPackageBytes + valueTypeNumberOfBytes > 16)
					{
						// Move the scratch buffer pointer to the location of the next aligned package and restart the package bytes counter
						scratchBufferPointer += 4 * 4 - numberOfPackageBytes;
						numberOfPackageBytes = 0;
					}
					numberOfPackageBytes += valueTypeNumberOfBytes % 16;

					// Copy the property value into the scratch buffer
					const MaterialProperty::Usage usage = uniformBufferElementProperty.getUsage();
					if (MaterialProperty::Usage::MATERIAL_REFERENCE == usage)	// Most likely the case, so check this first
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
							RENDERER_ASSERT(mRendererRuntime.getContext(), false, "Can't resolve reference")
						}
					}
					else if (MaterialProperty::Usage::GLOBAL_REFERENCE == usage)
					{
						// Referencing a global material property inside a material uniform buffer doesn't make really sense performance wise, but don't forbid it
	
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
								// Error
								RENDERER_ASSERT(mRendererRuntime.getContext(), false, "Can't resolve reference")
							}
						}
					}
					else if (!uniformBufferElementProperty.isReferenceUsage())	// TODO(co) Performance: Think about such tests, the toolkit should already take care of this so we have well known verified runtime data
					{
						// Referencing a static material property inside an material uniform buffer doesn't make really sense performance wise, but don't forbid it

						// Just copy over the property value
						memcpy(scratchBufferPointer, uniformBufferElementProperty.getData(), valueTypeNumberOfBytes);
					}
					else
					{
						// Error!
						RENDERER_ASSERT(mRendererRuntime.getContext(), false, "Invalid property")
					}

					// Next property
					scratchBufferPointer += valueTypeNumberOfBytes;
				}

				// The material buffer slot is now clean
				materialBufferSlot->mDirty = false;
			}
		}

		// Update the uniform buffer by using our scratch buffer
		if (nullptr != uniformBuffer)
		{
			Renderer::MappedSubresource mappedSubresource;
			Renderer::IRenderer& renderer = mRendererRuntime.getRenderer();
			if (renderer.map(*uniformBuffer, 0, Renderer::MapType::WRITE_DISCARD, 0, mappedSubresource))
			{
				memcpy(mappedSubresource.data, mScratchBuffer.data(), static_cast<uint32_t>(mScratchBuffer.size()));
				renderer.unmap(*uniformBuffer, 0);
			}
		}

		// Done
		mDirtyMaterialBufferSlots.clear();
	}


	//[-------------------------------------------------------]
	//[ Public RendererRuntime::MaterialBufferManager::BufferPool methods ]
	//[-------------------------------------------------------]
	MaterialBufferManager::BufferPool::BufferPool(uint32_t bufferSize, uint32_t slotsPerPool, Renderer::IBufferManager& bufferManager, const MaterialBlueprintResource& materialBlueprintResource) :
		uniformBuffer(bufferManager.createUniformBuffer(bufferSize, nullptr, Renderer::BufferUsage::DYNAMIC_DRAW)),
		resourceGroup(nullptr)
	{
		RENDERER_SET_RESOURCE_DEBUG_NAME(uniformBuffer, "Material buffer manager")
		uniformBuffer->addReference();
		Renderer::IResource* resource = static_cast<Renderer::IResource*>(uniformBuffer);
		resourceGroup = materialBlueprintResource.getRootSignaturePtr()->createResourceGroup(materialBlueprintResource.getMaterialUniformBuffer()->rootParameterIndex, 1, &resource);
		RENDERER_SET_RESOURCE_DEBUG_NAME(resourceGroup, "Material buffer manager")
		resourceGroup->addReference();
		freeSlots.reserve(slotsPerPool);
		for (uint32_t i = 0; i < slotsPerPool; ++i)
		{
			freeSlots.push_back((slotsPerPool - i) - 1);
		}
	}

	MaterialBufferManager::BufferPool::~BufferPool()
	{
		resourceGroup->releaseReference();
		uniformBuffer->releaseReference();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
