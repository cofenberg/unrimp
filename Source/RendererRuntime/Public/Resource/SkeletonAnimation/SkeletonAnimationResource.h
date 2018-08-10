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
#include "RendererRuntime/Public/Resource/IResource.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
	#include <glm/gtc/quaternion.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	template <class ELEMENT_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class PackedElementManager;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
	class SkeletonAnimationResourceLoader;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
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
	*    Rigid skeleton animation clip resource
	*/
	class SkeletonAnimationResource final : public IResource
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SkeletonAnimationResourceLoader;
		friend PackedElementManager<SkeletonAnimationResource, SkeletonAnimationResourceId, 2048>;										// Type definition of template class
		friend ResourceManagerTemplate<SkeletonAnimationResource, SkeletonAnimationResourceLoader, SkeletonAnimationResourceId, 2048>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<uint32_t> ChannelByteOffsets;
		typedef std::vector<uint8_t>  ChannelData;
		struct ChannelHeader final
		{
			uint32_t boneId;				///< Bone ID ("RendererRuntime::StringId" on bone name)
			uint32_t numberOfPositionKeys;	///< Number of position keys, must be at least one
			uint32_t numberOfRotationKeys;	///< Number of rotation keys, must be at least one
			uint32_t numberOfScaleKeys;		///< Number of optional scale keys, can be zero
		};
		struct Vector3Key final
		{
			float	  timeInTicks;	///< The time of this key in ticks
			glm::vec3 value;		///< The value of this key
		};
		struct QuaternionKey final
		{
			float timeInTicks;	///< The time of this key in ticks
			float value[3];		///< The xyz quaternion value of this key, w will be reconstructed during runtime
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline uint8_t getNumberOfChannels() const
		{
			return mNumberOfChannels;
		}

		inline float getDurationInTicks() const
		{
			return mDurationInTicks;
		}

		inline float getTicksPerSecond() const
		{
			return mTicksPerSecond;
		}

		inline const ChannelByteOffsets& getChannelByteOffsets() const
		{
			return mChannelByteOffsets;
		}

		inline const ChannelData& getChannelData() const
		{
			return mChannelData;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline SkeletonAnimationResource() :
			mNumberOfChannels(0),
			mDurationInTicks(0.0f),
			mTicksPerSecond(0.0f)
		{
			// Nothing here
		}

		inline virtual ~SkeletonAnimationResource() override
		{
			// Sanity checks
			assert(0 == mNumberOfChannels);
			assert(0.0f == mDurationInTicks);
			assert(0.0f == mTicksPerSecond);
			assert(mChannelByteOffsets.empty());
			assert(mChannelData.empty());
		}

		explicit SkeletonAnimationResource(const SkeletonAnimationResource&) = delete;
		SkeletonAnimationResource& operator=(const SkeletonAnimationResource&) = delete;

		inline void clearSkeletonAnimationData()
		{
			mNumberOfChannels = 0;
			mDurationInTicks  = 0.0f;
			mTicksPerSecond   = 0.0f;
			mChannelByteOffsets.clear();
			mChannelData.clear();
		}

		//[-------------------------------------------------------]
		//[ "RendererRuntime::PackedElementManager" management    ]
		//[-------------------------------------------------------]
		inline void initializeElement(SkeletonAnimationResourceId skeletonAnimationResourceId)
		{
			// Sanity checks
			assert(0 == mNumberOfChannels);
			assert(0.0f == mDurationInTicks);
			assert(0.0f == mTicksPerSecond);
			assert(mChannelByteOffsets.empty());
			assert(mChannelData.empty());

			// Call base implementation
			IResource::initializeElement(skeletonAnimationResourceId);
		}

		inline void deinitializeElement()
		{
			// Reset everything
			clearSkeletonAnimationData();

			// Call base implementation
			IResource::deinitializeElement();
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint8_t			   mNumberOfChannels;	///< The number of bone animation channels; each channel affects a single node
		float			   mDurationInTicks;	///< Duration of the animation in ticks
		float			   mTicksPerSecond;		///< Ticks per second; 0 if not specified in the imported file
		ChannelByteOffsets mChannelByteOffsets;	///< Channel byte offsets
		ChannelData		   mChannelData;		///< The data of all bone channels in one big chunk


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
