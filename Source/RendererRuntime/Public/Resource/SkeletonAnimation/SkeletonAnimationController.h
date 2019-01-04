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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Core/StringId.h"
#include "RendererRuntime/Public/Core/GetInvalid.h"
#include "RendererRuntime/Public/Resource/IResourceListener.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IRendererRuntime;
	class SkeletonAnimationEvaluator;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;						///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef uint32_t SkeletonResourceId;			///< POD skeleton resource identifier
	typedef uint32_t SkeletonAnimationResourceId;	///< POD skeleton animation resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Rigid skeleton animation controller
	*
	*  @todo
	*    - TODO(co) Right now only a single skeleton animation at one and the same time is supported to have something to start with.
	*               This isn't practical, of course, and in reality one has multiple animation sources at one and the same time which
	*               are blended together. But well, as mentioned, one has to start somewhere.
	*    - TODO(co) Currently "RendererRuntime::SkeletonAnimationEvaluator" is directly used, probably it makes sense to manage those
	*               and then update all of them in parallel using multi-threading
	*    - TODO(co) It might make sense to let the skeleton animation resource manager manage skeleton animation controller instances as well
	*/
	class SkeletonAnimationController final : public IResourceListener
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SkeletonAnimationResourceManager;	// Calls "RendererRuntime::SkeletonAnimationController::update()"


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rendererRuntime
		*    Renderer runtime to use
		*  @param[in] skeletonResourceId
		*    ID of the controlled skeleton resource
		*/
		inline SkeletonAnimationController(const IRendererRuntime& rendererRuntime, SkeletonResourceId skeletonResourceId) :
			mRendererRuntime(rendererRuntime),
			mSkeletonResourceId(skeletonResourceId),
			mSkeletonAnimationResourceId(getInvalid<SkeletonAnimationResourceId>()),
			mSkeletonAnimationEvaluator(nullptr),
			mTimeInSeconds(0.0f)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~SkeletonAnimationController()
		{
			clear();
		}

		/**
		*  @brief
		*    Start skeleton animation by resource ID
		*
		*  @param[in] skeletonAnimationResourceId
		*    Skeleton animation resource ID
		*/
		void startSkeletonAnimationByResourceId(SkeletonAnimationResourceId skeletonAnimationResourceId);

		/**
		*  @brief
		*    Start skeleton animation by asset ID
		*
		*  @param[in] skeletonAnimationAssetId
		*    Skeleton animation asset ID
		*/
		void startSkeletonAnimationByAssetId(AssetId skeletonAnimationAssetId);

		/**
		*  @brief
		*    Clear the controller
		*/
		void clear();


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onLoadingStateChange(const IResource& resource) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SkeletonAnimationController(const SkeletonAnimationController&) = delete;
		SkeletonAnimationController& operator=(const SkeletonAnimationController&) = delete;
		void createSkeletonAnimationEvaluator();
		void destroySkeletonAnimationEvaluator();

		/**
		*  @brief
		*    Update the controller
		*
		*  @param[in] pastSecondsSinceLastFrame
		*    Past seconds since last frame
		*/
		void update(float pastSecondsSinceLastFrame);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const IRendererRuntime&		mRendererRuntime;				///< Renderer runtime to use
		SkeletonResourceId			mSkeletonResourceId;			///< ID of the controlled skeleton resource
		SkeletonAnimationResourceId mSkeletonAnimationResourceId;	///< Skeleton animation resource ID, can be set to invalid value
		SkeletonAnimationEvaluator* mSkeletonAnimationEvaluator;	///< Skeleton animation evaluator instance, can be a null pointer, destroy the instance if you no longer need it
		float						mTimeInSeconds;					///< Time in seconds


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
