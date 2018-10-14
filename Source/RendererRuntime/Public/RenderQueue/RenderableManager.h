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
#include "RendererRuntime/Public/Core/Manager.h"
#include "RendererRuntime/Public/RenderQueue/Renderable.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
	#include <cassert>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class Transform;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Renderable collection management
	*
	*  @note
	*    - Example: Abstract representation of an mesh scene item containing sub-meshes
	*/
	class RenderableManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<Renderable> Renderables;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		RenderableManager();

		inline ~RenderableManager()
		{
			// Nothing here
		}

		//[-------------------------------------------------------]
		//[ Data                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const Renderables& getRenderables() const
		{
			return mRenderables;
		}

		[[nodiscard]] inline Renderables& getRenderables()	// Don't forget to call "RendererRuntime::RenderableManager::updateCachedRenderablesData()" if you changed something relevant in here
		{
			return mRenderables;
		}

		[[nodiscard]] inline const Transform& getTransform() const
		{
			// We know that this pointer is always valid
			assert(nullptr != mTransform);
			return *mTransform;
		}

		void setTransform(const Transform* transform);	// Can be a null pointer (internally a identity transform will be set), transform instance must stay valid as long as the renderable manager is referencing it

		[[nodiscard]] inline bool isVisible() const
		{
			return mVisible;
		}

		inline void setVisible(bool visible)
		{
			mVisible = visible;
		}

		//[-------------------------------------------------------]
		//[ Cached data                                           ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline float getCachedDistanceToCamera() const
		{
			return mCachedDistanceToCamera;
		}

		inline void setCachedDistanceToCamera(float distanceToCamera)
		{
			mCachedDistanceToCamera = distanceToCamera;
		}

		/**
		*  @brief
		*    Return the minimum renderables render queue index (inclusive)
		*
		*  @return
		*    The minimum renderables render queue index (inclusive)
		*
		*  @see
		*    - "RendererRuntime::RenderableManager::updateCachedRenderablesData()"
		*/
		[[nodiscard]] inline uint8_t getMinimumRenderQueueIndex() const
		{
			return mMinimumRenderQueueIndex;
		}

		/**
		*  @brief
		*    Return the maximum renderables render queue index (inclusive)
		*
		*  @return
		*    The maximum renderables render queue index (inclusive)
		*
		*  @see
		*    - "RendererRuntime::RenderableManager::updateCachedRenderablesData()"
		*/
		[[nodiscard]] inline uint8_t getMaximumRenderQueueIndex() const
		{
			return mMaximumRenderQueueIndex;
		}

		/**
		*  @brief
		*    Return whether or not at least one of the renderables is casting shadows
		*
		*  @return
		*    "true" if at least one of the renderables is casting shadows, else "false"
		*
		*  @see
		*    - "RendererRuntime::RenderableManager::updateCachedRenderablesData()"
		*/
		[[nodiscard]] inline bool getCastShadows() const
		{
			return mCastShadows;
		}

		/**
		*  @brief
		*    Update cached renderables data
		*
		*  @note
		*    - Assumed to not be called frequently, optimally only during renderable manager initialization
		*    - Usually only called by the renderable manager owner since renderables and the render queue index and shadow casting data is assumed to not change frequently
		*    - Updates the minimum and maximum renderables render queue index (inclusive) as well as whether or not at least one of the renderables is casting shadows
		*/
		RENDERERRUNTIME_API_EXPORT void updateCachedRenderablesData();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RenderableManager(const RenderableManager&) = delete;
		RenderableManager& operator=(const RenderableManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Data
		Renderables		 mRenderables;				///< Renderables
		const Transform* mTransform;				///< Transform instance, always valid, just shared meaning doesn't own the instance so don't delete it
		bool			 mVisible;
		// Cached data
		float			 mCachedDistanceToCamera;	///< Cached distance to camera is updated during the culling phase
		uint8_t			 mMinimumRenderQueueIndex;	///< The minimum renderables render queue index (inclusive, set inside "RendererRuntime::RenderableManager::updateCachedRenderablesData()")
		uint8_t			 mMaximumRenderQueueIndex;	///< The maximum renderables render queue index (inclusive, set inside "RendererRuntime::RenderableManager::updateCachedRenderablesData()")
		bool			 mCastShadows;				///< "true" if at least one of the renderables is casting shadows, else "false" (set inside "RendererRuntime::RenderableManager::updateCachedRenderablesData()")


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
