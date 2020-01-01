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
#include "Renderer/Public/Core/Manager.h"
#include "Renderer/Public/RenderQueue/Renderable.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class Transform;
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
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RHI_DEBUG
			/**
			*  @brief
			*    Return the renderable manager debug name
			*
			*  @return
			*    The renderable manager debug name, never a null pointer and at least an empty string
			*
			*  @note
			*    - If possible, the renderable manager debug name should use the following convention: "<filename>?[<attribute 0>][<attribute n>]" (for "?" see "Renderer::IFileManager::INVALID_CHARACTER")
			*/
			[[nodiscard]] inline const char* getDebugName() const
			{
				return mDebugName;
			}

			inline void setDebugName(const char debugName[])
			{
				ASSERT((strlen(debugName) < 256) && "Renderable manager debug name is not allowed to exceed 255 characters");
				strncpy(mDebugName, debugName, 256);
				mDebugName[255] = '\0';
			}
		#endif

		//[-------------------------------------------------------]
		//[ Data                                                  ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const Renderables& getRenderables() const
		{
			return mRenderables;
		}

		[[nodiscard]] inline Renderables& getRenderables()	// Don't forget to call "Renderer::RenderableManager::updateCachedRenderablesData()" if you changed something relevant in here
		{
			return mRenderables;
		}

		[[nodiscard]] inline uint8_t getNumberOfLods() const
		{
			return mNumberOfLods;
		}

		inline void setNumberOfLods(uint8_t numberOfLods)
		{
			mNumberOfLods = numberOfLods;
		}

		[[nodiscard]] inline const Transform& getTransform() const
		{
			// We know that this pointer is always valid
			ASSERT(nullptr != mTransform);
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
		*    - "Renderer::RenderableManager::updateCachedRenderablesData()"
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
		*    - "Renderer::RenderableManager::updateCachedRenderablesData()"
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
		*    - "Renderer::RenderableManager::updateCachedRenderablesData()"
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
		RENDERER_API_EXPORT void updateCachedRenderablesData();


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
		// Debug
		#ifdef RHI_DEBUG
			char		 mDebugName[256];			///< Debug name for easier renderable manager identification when debugging, contains terminating zero, first member variable by intent to see it at once during introspection (debug memory layout change is no problem here)
		#endif
		// Data
		Renderables		 mRenderables;				///< Renderables, directly containing also the renderables of all LODs, each LOD has the same number of renderables
		uint8_t			 mNumberOfLods;				///< Number of LODs, there's always at least one LOD, namely the original none reduced version
		const Transform* mTransform;				///< Transform instance, always valid, just shared meaning doesn't own the instance so don't delete it
		bool			 mVisible;
		// Cached data
		float			 mCachedDistanceToCamera;	///< Cached distance to camera is updated during the culling phase
		uint8_t			 mMinimumRenderQueueIndex;	///< The minimum renderables render queue index (inclusive, set inside "Renderer::RenderableManager::updateCachedRenderablesData()")
		uint8_t			 mMaximumRenderQueueIndex;	///< The maximum renderables render queue index (inclusive, set inside "Renderer::RenderableManager::updateCachedRenderablesData()")
		bool			 mCastShadows;				///< "true" if at least one of the renderables is casting shadows, else "false" (set inside "Renderer::RenderableManager::updateCachedRenderablesData()")


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
