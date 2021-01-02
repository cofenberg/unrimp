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
#include "Renderer/Public/Core/StringId.h"
#include "Renderer/Public/Core/GetInvalid.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class CompositorTarget;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId CompositorPassTypeId;	///< Compositor pass type identifier, internally just a POD "uint32_t"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class ICompositorResourcePass
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorTarget;	// Needs to destroy compositor resource pass instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t MAXIMUM_PASS_NAME_LENGTH = 63 + 1;	// +1 for the terminating zero


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline const CompositorTarget& getCompositorTarget() const
		{
			return mCompositorTarget;
		}

		#if defined(RHI_DEBUG) || defined(RENDERER_PROFILER)
			/**
			*  @brief
			*    Return the compositor resource pass debug name
			*
			*  @return
			*    The compositor resource pass debug name, never a null pointer and at least an empty string
			*/
			[[nodiscard]] inline const char* getDebugName() const
			{
				return mDebugName;
			}

			inline void setDebugName(const char debugName[])
			{
				ASSERT(strlen(debugName) < MAXIMUM_PASS_NAME_LENGTH, "Compositor resource pass debug name is not allowed to exceed 63 characters")
				strncpy(mDebugName, debugName, MAXIMUM_PASS_NAME_LENGTH);
				mDebugName[MAXIMUM_PASS_NAME_LENGTH - 1] = '\0';
			}
		#endif

		[[nodiscard]] inline float getMinimumDepth() const
		{
			return mMinimumDepth;
		}

		[[nodiscard]] inline float getMaximumDepth() const
		{
			return mMaximumDepth;
		}

		[[nodiscard]] inline bool getSkipFirstExecution() const
		{
			return mSkipFirstExecution;
		}

		[[nodiscard]] inline uint32_t getNumberOfExecutions() const
		{
			return mNumberOfExecutions;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ICompositorResourcePass methods ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual CompositorPassTypeId getTypeId() const = 0;

		inline virtual void deserialize([[maybe_unused]] uint32_t numberOfBytes, const uint8_t* data)
		{
			// Keep this in sync with "Renderer::v1CompositorNode::Pass"
			// -> Don't include "Renderer/Resource/CompositorNode/Loader/CompositorNodeFileFormat.h" here to keep the header complexity low (compile times matter)
			#pragma pack(push)
			#pragma pack(1)
				struct PassData final
				{
					char	 name[MAXIMUM_PASS_NAME_LENGTH] = { "Compositor pass" };	///< Human readable ASCII pass name for debugging and profiling, contains terminating zero
					float	 minimumDepth					= 0.0f;
					float	 maximumDepth					= 1.0f;
					uint32_t numberOfExecutions				= getInvalid<uint32_t>();
					bool	 skipFirstExecution				= false;
				};
			#pragma pack(pop)

			// Sanity check
			ASSERT(sizeof(PassData) == numberOfBytes, "Invalid number of bytes")

			// Read data
			const PassData* passData = reinterpret_cast<const PassData*>(data);
			#if defined(RHI_DEBUG) || defined(RENDERER_PROFILER)
				strncpy(mDebugName, passData->name, MAXIMUM_PASS_NAME_LENGTH);
			#endif
			mMinimumDepth		= passData->minimumDepth;
			mMaximumDepth		= passData->maximumDepth;
			mNumberOfExecutions = passData->numberOfExecutions;
			mSkipFirstExecution = passData->skipFirstExecution;

			// Sanity checks
			ASSERT(mNumberOfExecutions > 0, "Invalid number of executions")
			ASSERT(!mSkipFirstExecution || mNumberOfExecutions > 1, "Invalid number of executions")
		}

		/**
		*  @brief
		*    Return the render queue index range
		*
		*   @param[in] minimumRenderQueueIndex
		*     If this compositor resource pass has a render queue range defined, this will receive the minimum render queue index (inclusive), if there's no range no value will be written at all
		*   @param[in] maximumRenderQueueIndex
		*     If this compositor resource pass has a render queue range defined, this will receive the maximum render queue index (inclusive), if there's no range no value will be written at all
		*
		*  @return
		*    "true" if this compositor resource pass has a render queue range defined, else "false"
		*/
		[[nodiscard]] inline virtual bool getRenderQueueIndexRange([[maybe_unused]] uint8_t& minimumRenderQueueIndex, [[maybe_unused]] uint8_t& maximumRenderQueueIndex) const
		{
			// This compositor resource pass has no render queue range defined
			return false;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit ICompositorResourcePass(const CompositorTarget& compositorTarget) :
			#if defined(RHI_DEBUG) || defined(RENDERER_PROFILER)
				mDebugName{ "Compositor pass" },
			#endif
			mCompositorTarget(compositorTarget),
			mMinimumDepth(0.0f),
			mMaximumDepth(1.0f),
			mSkipFirstExecution(false),
			mNumberOfExecutions(getInvalid<uint32_t>())
		{
			// Nothing here
		}

		inline virtual ~ICompositorResourcePass()
		{
			// Nothing here
		}

		explicit ICompositorResourcePass(const ICompositorResourcePass&) = delete;
		ICompositorResourcePass& operator=(const ICompositorResourcePass&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		#if defined(RHI_DEBUG) || defined(RENDERER_PROFILER)
			char				mDebugName[MAXIMUM_PASS_NAME_LENGTH];	///< Debug name for easier compositor resource identification when debugging, contains terminating zero, first member variable by intent to see it at once during introspection (debug memory layout change is no problem here)
		#endif
		const CompositorTarget& mCompositorTarget;
		float					mMinimumDepth;
		float					mMaximumDepth;
		bool					mSkipFirstExecution;
		uint32_t				mNumberOfExecutions;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
