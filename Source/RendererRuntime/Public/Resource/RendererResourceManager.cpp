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
#include "RendererRuntime/Public/Resource/RendererResourceManager.h"
#include "RendererRuntime/Public/Core/Math/Math.h"

#include <Renderer/Public/Renderer.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	Renderer::IResourceGroup* RendererResourceManager::createResourceGroup(Renderer::IRootSignature& rootSignature, uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates)
	{
		// Create hash
		uint32_t hash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&rootSignature), sizeof(Renderer::IRootSignature&));
		hash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&rootParameterIndex), sizeof(uint32_t), hash);
		hash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&numberOfResources), sizeof(uint32_t), hash);
		for (uint32_t i = 0; i < numberOfResources; ++i)
		{
			hash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&resources[i]), sizeof(Renderer::IResource*), hash);
			if (nullptr != samplerStates && nullptr != samplerStates[i])
			{
				hash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&samplerStates[i]), sizeof(Renderer::ISamplerState*), hash);
			}
			else
			{
				static const uint32_t NOTHING = 42;	// Not "static constexpr" by intent
				hash = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&NOTHING), sizeof(uint32_t), hash);
			}
		}
		ResourceGroups::const_iterator iterator = mResourceGroups.find(hash);
		if (mResourceGroups.cend() != iterator)
		{
			return iterator->second;
		}
		else
		{
			// Create renderer resource and add the managers reference
			Renderer::IResourceGroup* resourceGroup = rootSignature.createResourceGroup(rootParameterIndex, numberOfResources, resources, samplerStates);
			resourceGroup->addReference();
			mResourceGroups.emplace(hash, resourceGroup);
			return resourceGroup;
		}
	}

	void RendererResourceManager::garbageCollection()
	{
		// TODO(co) "RendererRuntime::RendererResourceManager": From time to time, look for orphaned renderer resources and free them. Currently a trivial approach is used which might cause hiccups. For example distribute the traversal over time.
		++mGarbageCollectionCounter;
		if (mGarbageCollectionCounter > 100)
		{
			ResourceGroups::iterator iterator = mResourceGroups.begin();
			while (iterator != mResourceGroups.end())
			{
				if (iterator->second->getRefCount() == 1)
				{
					iterator->second->releaseReference();
					iterator = mResourceGroups.erase(iterator);
				}
				else
				{
					++iterator;
				}
			}
			mGarbageCollectionCounter = 0;
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	RendererResourceManager::~RendererResourceManager()
	{
		// Release manager renderer resource references
		for (auto& pair : mResourceGroups)
		{
			pair.second->releaseReference();
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
