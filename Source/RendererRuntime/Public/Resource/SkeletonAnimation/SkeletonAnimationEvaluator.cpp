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
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationEvaluator.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationResourceManager.h"
#include "RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationResource.h"
#include "RendererRuntime/Public/Core/Math/Math.h"

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
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		inline void convertQuaternion(const float in[3], glm::quat& out)
		{
			// We only store the xyz quaternion value of this key, w will be reconstructed during runtime
			out.x = in[0];
			out.y = in[1];
			out.z = in[2];
			const float t = 1.0f - (in[0] * in[0]) - (in[1] * in[1]) - (in[2] * in[2]);
			out.w = (t < 0.0f) ? 0.0f : -std::sqrt(t);
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void SkeletonAnimationEvaluator::evaluate(float timeInSeconds)
	{
		const SkeletonAnimationResource& skeletonAnimationResource = mSkeletonAnimationResourceManager.getById(mSkeletonAnimationResourceId);
		const uint8_t numberOfChannels = skeletonAnimationResource.getNumberOfChannels();
		const float durationInTicks = skeletonAnimationResource.getDurationInTicks();
		const SkeletonAnimationResource::ChannelByteOffsets& channelByteOffsets = skeletonAnimationResource.getChannelByteOffsets();
		const SkeletonAnimationResource::ChannelData& channelData = skeletonAnimationResource.getChannelData();
		if (mTransformMatrices.empty())
		{
			// Allocate memory
			mBoneIds.resize(numberOfChannels);
			mTransformMatrices.resize(numberOfChannels);
			mLastPositions.resize(numberOfChannels, std::make_tuple(0, 0, 0));

			// Backup bone IDs
			for (uint8_t i = 0; i < numberOfChannels; ++i)
			{
				mBoneIds[i] = reinterpret_cast<const SkeletonAnimationResource::ChannelHeader&>(*(channelData.data() + channelByteOffsets[i])).boneId;
			}
		}

		// Extract ticks per second; assume default value if not given
		const float ticksPerSecond = (0.0f != skeletonAnimationResource.getTicksPerSecond()) ? skeletonAnimationResource.getTicksPerSecond() : 25.0f;

		// Every following time calculation happens in ticks
		float timeInTicks = timeInSeconds * ticksPerSecond;

		// Map the time into the duration of the animation
		timeInTicks = (durationInTicks > 0.0f) ? fmod(timeInTicks, durationInTicks) : 0.0f;

		// Calculate the transformations for each animation channel
		for (uint8_t i = 0; i < numberOfChannels; ++i)
		{
			const uint8_t* currentChannelData = channelData.data() + channelByteOffsets[i];

			// Get channel header
			const SkeletonAnimationResource::ChannelHeader& channelHeader = reinterpret_cast<const SkeletonAnimationResource::ChannelHeader&>(*currentChannelData);
			currentChannelData += sizeof(SkeletonAnimationResource::ChannelHeader);

			// Sanity checks
			assert(channelHeader.numberOfPositionKeys > 0);
			assert(channelHeader.numberOfRotationKeys > 0);
			// assert(channelHeader.numberOfScaleKeys > 0);	Scale is optional

			// Get channel keys
			const SkeletonAnimationResource::Vector3Key* positionKeys = reinterpret_cast<const SkeletonAnimationResource::Vector3Key*>(currentChannelData);
			currentChannelData += sizeof(SkeletonAnimationResource::Vector3Key) * channelHeader.numberOfPositionKeys;
			const SkeletonAnimationResource::QuaternionKey* rotationKeys = reinterpret_cast<const SkeletonAnimationResource::QuaternionKey*>(currentChannelData);
			currentChannelData += sizeof(SkeletonAnimationResource::QuaternionKey) * channelHeader.numberOfRotationKeys;
			const SkeletonAnimationResource::Vector3Key* scaleKeys = reinterpret_cast<const SkeletonAnimationResource::Vector3Key*>(currentChannelData);

			// Position
			glm::vec3 presentPosition;
			{
				// Look for present frame number. Search from last position if time is after the last time, else from beginning
				// Should be much quicker than always looking from start for the average use case.
				uint32_t frame = (timeInTicks >= mLastTimeInTicks) ? std::get<0>(mLastPositions[i]) : 0;
				while (frame < channelHeader.numberOfPositionKeys - 1)
				{
					if (timeInTicks < positionKeys[frame + 1].timeInTicks)
					{
						break;
					}
					++frame;
				}

				// Interpolate between this frame's value and next frame's value
				const uint32_t nextFrame = (frame + 1) % channelHeader.numberOfPositionKeys;
				const SkeletonAnimationResource::Vector3Key& key = positionKeys[frame];
				const SkeletonAnimationResource::Vector3Key& nextKey = positionKeys[nextFrame];
				float timeDifference = nextKey.timeInTicks - key.timeInTicks;
				if (timeDifference < 0.0f)
				{
					timeDifference += durationInTicks;
				}
				if (timeDifference > 0.0f)
				{
					const float factor = float((timeInTicks - key.timeInTicks) / timeDifference);
					presentPosition = glm::mix(key.value, nextKey.value, factor);
				}
				else
				{
					presentPosition = key.value;
				}
				std::get<0>(mLastPositions[i]) = frame;
			}

			// Rotation
			glm::quat presentRotation;
			{
				uint32_t frame = (timeInTicks >= mLastTimeInTicks) ? std::get<1>(mLastPositions[i]) : 0;
				while (frame < channelHeader.numberOfRotationKeys - 1)
				{
					if (timeInTicks < rotationKeys[frame + 1].timeInTicks)
					{
						break;
					}
					++frame;
				}

				// Interpolate between this frame's value and next frame's value
				const uint32_t nextFrame = (frame + 1) % channelHeader.numberOfRotationKeys;
				const SkeletonAnimationResource::QuaternionKey& key = rotationKeys[frame];
				const SkeletonAnimationResource::QuaternionKey& nextKey = rotationKeys[nextFrame];
				float timeDifference = nextKey.timeInTicks - key.timeInTicks;
				if (timeDifference < 0.0f)
				{
					timeDifference += durationInTicks;
				}
				if (timeDifference > 0.0f)
				{
					const float factor = float((timeInTicks - key.timeInTicks) / timeDifference);
					glm::quat keyQuaternion;
					::detail::convertQuaternion(key.value, keyQuaternion);
					glm::quat nextKeyQuaternion;
					::detail::convertQuaternion(nextKey.value, nextKeyQuaternion);
					presentRotation = glm::slerp(keyQuaternion, nextKeyQuaternion, factor);
				}
				else
				{
					::detail::convertQuaternion(key.value, presentRotation);
				}
				std::get<1>(mLastPositions[i]) = frame;
			}

			// Scale is optional
			if (channelHeader.numberOfScaleKeys > 0)
			{
				glm::vec3 presentScale;
				{
					uint32_t frame = (timeInTicks >= mLastTimeInTicks) ? std::get<2>(mLastPositions[i]) : 0;
					while (frame < channelHeader.numberOfScaleKeys - 1)
					{
						if (timeInTicks < scaleKeys[frame + 1].timeInTicks)
						{
							break;
						}
						++frame;
					}

					// TODO(co) Interpolation maybe? This time maybe even logarithmic, not linear.
					presentScale = scaleKeys[frame].value;
					std::get<2>(mLastPositions[i]) = frame;
				}

				// Build a transformation matrix from it
				// TODO(co) Review temporary matrix instances on the C-runtime stack
				mTransformMatrices[i] = glm::translate(Math::MAT4_IDENTITY, presentPosition) * glm::toMat4(presentRotation) * glm::scale(Math::MAT4_IDENTITY, presentScale);
			}
			else
			{
				// Build a transformation matrix from it
				// TODO(co) Review temporary matrix instances on the C-runtime stack
				mTransformMatrices[i] = glm::translate(Math::MAT4_IDENTITY, presentPosition) * glm::toMat4(presentRotation);
			}
		}

		mLastTimeInTicks = timeInTicks;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
