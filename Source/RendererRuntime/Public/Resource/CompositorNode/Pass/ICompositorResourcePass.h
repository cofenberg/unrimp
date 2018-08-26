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
#include "RendererRuntime/Public/Core/GetInvalid.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::char_traits<wchar_t>::int_type', signed/unsigned mismatch
	#include <cassert>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class CompositorTarget;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
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
		inline const CompositorTarget& getCompositorTarget() const
		{
			return mCompositorTarget;
		}

		#ifdef RENDERER_RUNTIME_PROFILER
			inline const char* getName() const
			{
				return mName;
			}

			inline void setName(const char* name)
			{
				strncpy(mName, name, MAXIMUM_PASS_NAME_LENGTH);
			}
		#endif

		inline float getMinimumDepth() const
		{
			return mMinimumDepth;
		}

		inline float getMaximumDepth() const
		{
			return mMaximumDepth;
		}

		inline bool getSkipFirstExecution() const
		{
			return mSkipFirstExecution;
		}

		inline uint32_t getNumberOfExecutions() const
		{
			return mNumberOfExecutions;
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorResourcePass methods ]
	//[-------------------------------------------------------]
	public:
		virtual CompositorPassTypeId getTypeId() const = 0;

		inline virtual void deserialize(MAYBE_UNUSED uint32_t numberOfBytes, const uint8_t* data)
		{
			// Keep this in sync with "RendererRuntime::v1CompositorNode::Pass"
			// -> Don't include "RendererRuntime/Resource/CompositorNode/Loader/CompositorNodeFileFormat.h" here to keep the header complexity low (compile times matter)
			#pragma pack(push)
			#pragma pack(1)
				struct PassData final
				{
					char	 name[MAXIMUM_PASS_NAME_LENGTH] = { "Compositor pass" };	///< Human readable ASCII pass name for debugging and profiling, contains terminating zero
					float	 minimumDepth					= 0.0f;
					float	 maximumDepth					= 1.0f;
					uint32_t numberOfExecutions				= RendererRuntime::getInvalid<uint32_t>();
					bool	 skipFirstExecution				= false;
				};
			#pragma pack(pop)

			// Sanity check
			assert(sizeof(PassData) == numberOfBytes);

			// Read data
			const PassData* pass = reinterpret_cast<const PassData*>(data);
			#ifdef RENDERER_RUNTIME_PROFILER
				strncpy(mName, pass->name, MAXIMUM_PASS_NAME_LENGTH);
			#endif
			mMinimumDepth		= pass->minimumDepth;
			mMaximumDepth		= pass->maximumDepth;
			mNumberOfExecutions = pass->numberOfExecutions;
			mSkipFirstExecution = pass->skipFirstExecution;

			// Sanity checks
			assert(mNumberOfExecutions > 0);
			assert(!mSkipFirstExecution || mNumberOfExecutions > 1);
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
		inline virtual bool getRenderQueueIndexRange(MAYBE_UNUSED uint8_t& minimumRenderQueueIndex, MAYBE_UNUSED uint8_t& maximumRenderQueueIndex) const
		{
			// This compositor resource pass has no render queue range defined
			return false;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit ICompositorResourcePass(const CompositorTarget& compositorTarget) :
			mCompositorTarget(compositorTarget),
			#ifdef RENDERER_RUNTIME_PROFILER
				mName{ "Compositor pass" },
			#endif
			mMinimumDepth(0.0f),
			mMaximumDepth(1.0f),
			mSkipFirstExecution(false),
			mNumberOfExecutions(RendererRuntime::getInvalid<uint32_t>())
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
		const CompositorTarget& mCompositorTarget;
		#ifdef RENDERER_RUNTIME_PROFILER
			char				mName[MAXIMUM_PASS_NAME_LENGTH];	///< Human readable ASCII pass name for debugging and profiling, contains terminating zero
		#endif
		float					mMinimumDepth;
		float					mMaximumDepth;
		bool					mSkipFirstExecution;
		uint32_t				mNumberOfExecutions;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
