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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/SkeletonAnimation/SkeletonAnimationEvaluator.h"
#include "Renderer/Public/Resource/SkeletonAnimation/SkeletonAnimationResourceManager.h"
#include "Renderer/Public/Resource/SkeletonAnimation/SkeletonAnimationResource.h"
#include "Renderer/Public/Core/Math/Math.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'rtm::mix4::b' in switch of enum 'rtm::mix4' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'initializing': conversion from 'int' to 'uint8_t', signed/unsigned mismatch
	#include <acl/decompression/decompress.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtx/quaternion.hpp>
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		typedef acl::decompression_context<acl::default_transform_decompression_settings> AclDecompressionContext;


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		struct TrackWriter final : public acl::track_writer
		{


			//[-------------------------------------------------------]
			//[ Public data                                           ]
			//[-------------------------------------------------------]
			public:
				rtm::quatf	  mRotation	   = rtm::quat_identity();
				rtm::vector4f mTranslation = rtm::vector_zero();
				rtm::vector4f mScale	   = rtm::vector_set(1.0f);


			//[-------------------------------------------------------]
			//[ Public methods                                        ]
			//[-------------------------------------------------------]
			public:
				// Called by the decoder to write out a quaternion rotation value for a specified bone index
				void RTM_SIMD_CALL write_rotation(uint32_t, rtm::quatf_arg0 rotation)
				{
					mRotation = rotation;
				}

				// Called by the decoder to write out a translation value for a specified bone index
				void RTM_SIMD_CALL write_translation(uint32_t, rtm::vector4f_arg0 translation)
				{
					mTranslation = translation;
				}

				// Called by the decoder to write out a scale value for a specified bone index
				void RTM_SIMD_CALL write_scale(uint32_t, rtm::vector4f_arg0 scale)
				{
					mScale = scale;
				}

				void getTransformMatrix(glm::mat4& transformMatrix) const
				{
					// Build a transformation matrix from it
					// TODO(co) Handle case of no scale
					// TODO(co) Review temporary matrix instances on the C-runtime stack
					glm::quat presentRotation(rtm::quat_get_w(mRotation), rtm::quat_get_x(mRotation), rtm::quat_get_y(mRotation), rtm::quat_get_z(mRotation));
					glm::vec3 presentPosition(rtm::vector_get_x(mTranslation), rtm::vector_get_y(mTranslation), rtm::vector_get_z(mTranslation));
					glm::vec3 presentScale(rtm::vector_get_x(mScale), rtm::vector_get_y(mScale), rtm::vector_get_z(mScale));
					transformMatrix = glm::translate(Renderer::Math::MAT4_IDENTITY, presentPosition) * glm::toMat4(presentRotation) * glm::scale(Renderer::Math::MAT4_IDENTITY, presentScale);
				}


		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	SkeletonAnimationEvaluator::SkeletonAnimationEvaluator(SkeletonAnimationResourceManager& skeletonAnimationResourceManager, SkeletonAnimationResourceId skeletonAnimationResourceId) :
		mSkeletonAnimationResourceManager(skeletonAnimationResourceManager),
		mSkeletonAnimationResourceId(skeletonAnimationResourceId)
	{
		mAclDecompressionContext = new ::detail::AclDecompressionContext();
		const SkeletonAnimationResource& skeletonAnimationResource = mSkeletonAnimationResourceManager.getById(mSkeletonAnimationResourceId);
		static_cast<::detail::AclDecompressionContext*>(mAclDecompressionContext)->initialize(*reinterpret_cast<const acl::compressed_tracks*>(skeletonAnimationResource.getAclCompressedTracks().data()));
		mBoneIds = skeletonAnimationResource.getBoneIds();
		mTransformMatrices.resize(skeletonAnimationResource.getNumberOfChannels());
	}

	SkeletonAnimationEvaluator::~SkeletonAnimationEvaluator()
	{
		delete static_cast<::detail::AclDecompressionContext*>(mAclDecompressionContext);
	}

	void SkeletonAnimationEvaluator::evaluate(float timeInSeconds)
	{
		const SkeletonAnimationResource& skeletonAnimationResource = mSkeletonAnimationResourceManager.getById(mSkeletonAnimationResourceId);
		const uint8_t numberOfChannels = skeletonAnimationResource.getNumberOfChannels();

		// Decompress the ACL compressed skeleton animation tracks
		::detail::AclDecompressionContext* aclDecompressionContext = static_cast<::detail::AclDecompressionContext*>(mAclDecompressionContext);
		const float duration = skeletonAnimationResource.getDurationInTicks() / skeletonAnimationResource.getTicksPerSecond();
		while (timeInSeconds > duration)
		{
			timeInSeconds -= duration;
		}
		static_cast<::detail::AclDecompressionContext*>(mAclDecompressionContext)->seek(timeInSeconds, acl::sample_rounding_policy::none);
		::detail::TrackWriter trackWriter;
		for (uint8_t i = 0; i < numberOfChannels; ++i)
		{
			aclDecompressionContext->decompress_track(i, trackWriter);
			trackWriter.getTransformMatrix(mTransformMatrices[i]);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
