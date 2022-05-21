/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Renderer/Public/Resource/IResource.h"
#include "Renderer/Public/Core/Manager.h"

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
	class SceneNode;
	class ISceneItem;
	class ISceneFactory;
	class IRenderer;
	class SceneCullingManager;
	class SceneResourceLoader;
	template <class ELEMENT_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class PackedElementManager;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t SceneResourceId;	///< POD scene resource identifier
	typedef StringId SceneItemTypeId;	///< Scene item type identifier, internally just a POD "uint32_t"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class SceneResource final : public IResource
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneResourceManager;															// Needs to be able to update the scene factory instance
		friend PackedElementManager<SceneResource, SceneResourceId, 16>;							// Type definition of template class
		friend ResourceManagerTemplate<SceneResource, SceneResourceLoader, SceneResourceId, 16>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<SceneNode*> SceneNodes;
		typedef std::vector<ISceneItem*> SceneItems;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] RENDERER_API_EXPORT IRenderer& getRenderer() const;

		[[nodiscard]] inline SceneCullingManager& getSceneCullingManager() const
		{
			// We know that this pointer is always valid
			ASSERT(nullptr != mSceneCullingManager, "Invalid scene culling manager")
			return *mSceneCullingManager;
		}

		RENDERER_API_EXPORT void destroyAllSceneNodesAndItems();

		//[-------------------------------------------------------]
		//[ Node                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] RENDERER_API_EXPORT SceneNode* createSceneNode(const Transform& transform);
		RENDERER_API_EXPORT void destroySceneNode(SceneNode& sceneNode);
		RENDERER_API_EXPORT void destroyAllSceneNodes();

		[[nodiscard]] inline const SceneNodes& getSceneNodes() const
		{
			return mSceneNodes;
		}

		//[-------------------------------------------------------]
		//[ Item                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] RENDERER_API_EXPORT ISceneItem* createSceneItem(SceneItemTypeId sceneItemTypeId, SceneNode& sceneNode);

		template <typename T>
		[[nodiscard]] inline T* createSceneItem(SceneNode& sceneNode)
		{
			return static_cast<T*>(createSceneItem(T::TYPE_ID, sceneNode));
		}

		RENDERER_API_EXPORT void destroySceneItem(ISceneItem& sceneItem);
		RENDERER_API_EXPORT void destroyAllSceneItems();

		[[nodiscard]] inline const SceneItems& getSceneItems() const
		{
			return mSceneItems;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline SceneResource() :
			mSceneFactory(nullptr),
			mSceneCullingManager(nullptr)
		{
			// Nothing here
		}

		inline virtual ~SceneResource() override
		{
			// Sanity checks
			ASSERT(nullptr == mSceneFactory, "Invalid scene factory")
			ASSERT(nullptr == mSceneCullingManager, "Invalid scene culling manager")
			ASSERT(mSceneNodes.empty(), "Invalid scene nodes")
			ASSERT(mSceneItems.empty(), "Invalid scene items")
		}

		explicit SceneResource(const SceneResource&) = delete;
		SceneResource& operator=(const SceneResource&) = delete;

		inline SceneResource& operator=(SceneResource&& sceneResource) noexcept
		{
			// Call base implementation
			IResource::operator=(std::move(sceneResource));

			// Swap data
			std::swap(mSceneFactory, sceneResource.mSceneFactory);
			std::swap(mSceneCullingManager, sceneResource.mSceneCullingManager);
			std::swap(mSceneNodes, sceneResource.mSceneNodes);
			std::swap(mSceneItems, sceneResource.mSceneItems);

			// Done
			return *this;
		}

		//[-------------------------------------------------------]
		//[ "Renderer::PackedElementManager" management           ]
		//[-------------------------------------------------------]
		void initializeElement(SceneResourceId sceneResourceId);
		void deinitializeElement();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const ISceneFactory* mSceneFactory;			///< Scene factory instance, always valid, do not destroy the instance
		SceneCullingManager* mSceneCullingManager;	///< Scene culling manager, always valid, destroy the instance if you no longer need it
		SceneNodes			 mSceneNodes;
		SceneItems			 mSceneItems;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
