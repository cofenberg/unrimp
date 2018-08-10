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
#include "RendererRuntime/Public/Core/StringId.h"
#include "RendererRuntime/Public/Core/Math/Transform.h"

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
namespace RendererRuntime
{
	class ISceneItem;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class SceneNode final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneResource;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<SceneNode*> AttachedSceneNodes;
		typedef std::vector<ISceneItem*> AttachedSceneItems;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Local transform                                       ]
		//[-------------------------------------------------------]
		inline const Transform& getTransform() const
		{
			return mTransform;
		}

		inline void setTransform(const Transform& transform)
		{
			mTransform = transform;
			updateGlobalTransformRecursive();
		}

		inline void setPosition(const glm::vec3& position)
		{
			mTransform.position = position;
			updateGlobalTransformRecursive();
		}

		inline void setRotation(const glm::quat& rotation)
		{
			mTransform.rotation = rotation;
			updateGlobalTransformRecursive();
		}

		inline void setPositionRotation(const glm::vec3& position, const glm::quat& rotation)
		{
			mTransform.position = position;
			mTransform.rotation = rotation;
			updateGlobalTransformRecursive();
		}

		inline void setScale(const glm::vec3& scale)
		{
			mTransform.scale = scale;
			updateGlobalTransformRecursive();
		}

		//[-------------------------------------------------------]
		//[ Derived global transform                              ]
		//[-------------------------------------------------------]
		inline const Transform& getGlobalTransform() const
		{
			return mGlobalTransform;
		}

		inline const Transform& getPreviousGlobalTransform() const
		{
			return mPreviousGlobalTransform;
		}

		//[-------------------------------------------------------]
		//[ Attached scene nodes                                  ]
		//[-------------------------------------------------------]
		RENDERERRUNTIME_API_EXPORT void attachSceneNode(SceneNode& sceneNode);
		RENDERERRUNTIME_API_EXPORT void detachAllSceneNodes();

		inline const AttachedSceneNodes& getAttachedSceneNodes() const
		{
			return mAttachedSceneNodes;
		}

		RENDERERRUNTIME_API_EXPORT void setVisible(bool visible);

		//[-------------------------------------------------------]
		//[ Attached scene items                                  ]
		//[-------------------------------------------------------]
		RENDERERRUNTIME_API_EXPORT void attachSceneItem(ISceneItem& sceneItem);
		RENDERERRUNTIME_API_EXPORT void detachAllSceneItems();

		inline const AttachedSceneItems& getAttachedSceneItems() const
		{
			return mAttachedSceneItems;
		}

		RENDERERRUNTIME_API_EXPORT void setSceneItemsVisible(bool visible);


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit SceneNode(const Transform& transform) :
			mParentSceneNode(nullptr),
			mTransform(transform),
			mGlobalTransform(transform),
			mPreviousGlobalTransform(transform)
		{
			// Nothing here
		}

		inline ~SceneNode()
		{
			detachAllSceneNodes();
			detachAllSceneItems();
		}

		explicit SceneNode(const SceneNode&) = delete;
		SceneNode& operator=(const SceneNode&) = delete;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		RENDERERRUNTIME_API_EXPORT void updateGlobalTransformRecursive();
		void updateSceneItemTransform(ISceneItem& sceneItem);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		SceneNode*		   mParentSceneNode;			///< Parent scene node the scene node is attached to, can be a null pointer, don't destroy the instance
		Transform		   mTransform;					///< Local transform
		Transform		   mGlobalTransform;			///< Derived global transform - TODO(co) Will of course later on be handled in another way to be cache efficient and more efficient to calculate and incrementally update. But lets start simple.
		Transform		   mPreviousGlobalTransform;	///< Previous derived global transform
		AttachedSceneNodes mAttachedSceneNodes;
		AttachedSceneItems mAttachedSceneItems;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
