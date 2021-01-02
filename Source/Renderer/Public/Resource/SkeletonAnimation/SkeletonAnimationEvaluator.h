/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Renderer/Public/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP

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
	class SkeletonAnimationResourceManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t SkeletonAnimationResourceId;	///< POD skeleton animation resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Rigid skeleton animation evaluator which calculates transformations for a given timestamp
	*/
	class SkeletonAnimationEvaluator final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<uint32_t>  BoneIds;
		typedef std::vector<glm::mat4> TransformMatrices;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor on a given animation; the animation is fixed throughout the lifetime of the object
		*
		*  @param[in] skeletonAnimationResourceManager
		*    Skeleton animation resource manager to use
		*  @param[in] skeletonAnimationResourceId
		*    Skeleton animation resource ID
		*/
		SkeletonAnimationEvaluator(SkeletonAnimationResourceManager& skeletonAnimationResourceManager, SkeletonAnimationResourceId skeletonAnimationResourceId);

		/**
		*  @brief
		*    Destructor
		*/
		~SkeletonAnimationEvaluator();

		/**
		*  @brief
		*    Evaluates the animation tracks for a given time stamp; the calculated pose can be retrieved as a array of transformation matrices afterwards by calling "Renderer::SkeletonAnimationEvaluator::getTransformMatrices()"
		*
		*  @param[in] timeInSeconds
		*    The time for which you want to evaluate the animation, in seconds. Will be mapped into the animation cycle, so it can be an arbitrary value. Best use with ever-increasing time stamps.
		*/
		void evaluate(float timeInSeconds);

		/**
		*  @brief
		*    Return the bone IDs
		*
		*  @return
		*    The bone IDs
		*/
		[[nodiscard]] inline const BoneIds& getBoneIds() const
		{
			return mBoneIds;
		}

		/**
		*  @brief
		*    Return the transform matrices calculated at the last "Renderer::SkeletonAnimationEvaluator::evaluate()" call
		*
		*  @return
		*    The transform matrices
		*/
		[[nodiscard]] inline const TransformMatrices& getTransformMatrices() const
		{
			return mTransformMatrices;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SkeletonAnimationEvaluator(const SkeletonAnimationEvaluator&) = delete;
		SkeletonAnimationEvaluator& operator=(const SkeletonAnimationEvaluator&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		SkeletonAnimationResourceManager& mSkeletonAnimationResourceManager;	///< Skeleton animation resource manager to use
		SkeletonAnimationResourceId		  mSkeletonAnimationResourceId;			///< Skeleton animation resource ID
		BoneIds							  mBoneIds;								///< Bone IDs ("Renderer::StringId" on bone name)
		TransformMatrices				  mTransformMatrices;					///< The transform matrices calculated at the last "Renderer::SkeletonAnimationEvaluator::evaluate()" call
		void*							  mAclDecompressionContext;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
