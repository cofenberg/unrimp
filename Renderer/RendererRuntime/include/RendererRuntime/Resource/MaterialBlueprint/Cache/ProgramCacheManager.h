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
#include "RendererRuntime/Core/Manager.h"
#include "RendererRuntime/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4548)	// warning C4548: expression before comma has no effect; expected expression with side-effect
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <mutex>
	#include <unordered_map>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class ProgramCache;
	class PipelineStateSignature;
	class PipelineStateCacheManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t ProgramCacheId;	///< Program cache identifier, result of hashing the shader combination IDs of the referenced shaders


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/*
	*  @brief
	*    Program cache manager
	*
	*  @see
	*    - See "RendererRuntime::PipelineStateCacheManager" for additional information
	*/
	class ProgramCacheManager final : private Manager
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class PipelineStateCacheManager;	// Is creating and using a program cache manager instance
		friend class PipelineStateCompiler;		// Is tightly interacting with the program cache manager


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Generate a program cache ID by using a provided pipeline state signature
		*
		*  @param[in] pipelineStateSignature
		*    Pipeline state signature to use
		*
		*  @return
		*    The program cache ID
		*/
		static ProgramCacheId generateProgramCacheId(const PipelineStateSignature& pipelineStateSignature);


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the owner pipeline state cache manager
		*
		*  @return
		*    The owner pipeline state cache manager
		*/
		inline PipelineStateCacheManager& getPipelineStateCacheManager() const
		{
			return mPipelineStateCacheManager;
		}

		/**
		*  @brief
		*    Get program cache by pipeline state signature; synchronous processing
		*
		*  @param[in] pipelineStateSignature
		*    Pipeline state signature to use
		*
		*  @return
		*    The program cache, null pointer on error
		*/
		ProgramCache* getProgramCacheByPipelineStateSignature(const PipelineStateSignature& pipelineStateSignature);

		/**
		*  @brief
		*    Clear the program cache manager
		*/
		void clearCache();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline explicit ProgramCacheManager(PipelineStateCacheManager& pipelineStateCacheManager) :
			mPipelineStateCacheManager(pipelineStateCacheManager)
		{
			// Nothing here
		}

		inline ~ProgramCacheManager()
		{
			clearCache();
		}

		explicit ProgramCacheManager(const ProgramCacheManager&) = delete;
		ProgramCacheManager& operator=(const ProgramCacheManager&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::unordered_map<ProgramCacheId, ProgramCache*> ProgramCacheById;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		PipelineStateCacheManager& mPipelineStateCacheManager;	///< Owner pipeline state cache manager
		ProgramCacheById		   mProgramCacheById;
		std::mutex				   mMutex;						///< Mutex due to "RendererRuntime::PipelineStateCompiler" interaction, no too fine granular lock/unlock required because usually it's only asynchronous or synchronous processing, not both at one and the same time


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
