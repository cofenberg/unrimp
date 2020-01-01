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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Core/StringId.h"
#include "Renderer/Public/Core/Math/Transform.h"

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
	class ISceneItem;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
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
		[[nodiscard]] inline const Transform& getTransform() const
		{
			return mTransform;
		}

		// For incremental transform updates
		inline void setTransform(const Transform& transform)
		{
			mTransform = transform;
			updateGlobalTransformRecursive();
		}

		// For instant transform updates
		inline void teleportTransform(const Transform& transform)
		{
			setTransform(transform);
			mPreviousGlobalTransform = mGlobalTransform;
		}

		// For incremental position updates, 64 bit world space position
		inline void setPosition(const glm::dvec3& position)
		{
			mTransform.position = position;
			updateGlobalTransformRecursive();
		}

		// For instant position updates, 64 bit world space position
		inline void teleportPosition(const glm::dvec3& position)
		{
			setPosition(position);
			mPreviousGlobalTransform = mGlobalTransform;
		}

		// For incremental rotation updates
		inline void setRotation(const glm::quat& rotation)
		{
			mTransform.rotation = rotation;
			updateGlobalTransformRecursive();
		}

		// For instant rotation updates
		inline void teleportRotation(const glm::quat& rotation)
		{
			setRotation(rotation);
			mPreviousGlobalTransform = mGlobalTransform;
		}

		// For incremental position and rotation updates, 64 bit world space position
		inline void setPositionRotation(const glm::dvec3& position, const glm::quat& rotation)
		{
			mTransform.position = position;
			mTransform.rotation = rotation;
			updateGlobalTransformRecursive();
		}

		// For instant position and rotation updates, 64 bit world space position
		inline void teleportPositionRotation(const glm::dvec3& position, const glm::quat& rotation)
		{
			setPositionRotation(position, rotation);
			mPreviousGlobalTransform = mGlobalTransform;
		}

		// For incremental scale updates
		inline void setScale(const glm::vec3& scale)
		{
			mTransform.scale = scale;
			updateGlobalTransformRecursive();
		}

		// For instant scale updates
		inline void teleportScale(const glm::vec3& scale)
		{
			setScale(scale);
			mPreviousGlobalTransform = mGlobalTransform;
		}

		//[-------------------------------------------------------]
		//[ Derived global transform                              ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const Transform& getGlobalTransform() const
		{
			return mGlobalTransform;
		}

		[[nodiscard]] inline const Transform& getPreviousGlobalTransform() const
		{
			return mPreviousGlobalTransform;
		}

		//[-------------------------------------------------------]
		//[ Attached scene nodes                                  ]
		//[-------------------------------------------------------]
		RENDERER_API_EXPORT void attachSceneNode(SceneNode& sceneNode);
		RENDERER_API_EXPORT void detachAllSceneNodes();

		[[nodiscard]] inline const AttachedSceneNodes& getAttachedSceneNodes() const
		{
			return mAttachedSceneNodes;
		}

		RENDERER_API_EXPORT void setVisible(bool visible);

		//[-------------------------------------------------------]
		//[ Attached scene items                                  ]
		//[-------------------------------------------------------]
		RENDERER_API_EXPORT void attachSceneItem(ISceneItem& sceneItem);
		RENDERER_API_EXPORT void detachAllSceneItems();

		[[nodiscard]] inline const AttachedSceneItems& getAttachedSceneItems() const
		{
			return mAttachedSceneItems;
		}

		RENDERER_API_EXPORT void setSceneItemsVisible(bool visible);


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
		RENDERER_API_EXPORT void updateGlobalTransformRecursive();
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
} // Renderer
