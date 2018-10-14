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
#include "RendererRuntime/Public/Core/Platform/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <cassert>
	#include <inttypes.h>	// For uint32_t, uint64_t etc.
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class MaterialResource;
	class MaterialResourceManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Material buffer slot
	*/
	class MaterialBufferSlot
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class MaterialBufferManager;	// Manages the slot instances


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] materialResource
		*    Owner material resource, only material resource manager and material resource ID will internally be stored
		*/
		explicit MaterialBufferSlot(MaterialResource& materialResource);

		/**
		*  @brief
		*    Destructor
		*/
		inline ~MaterialBufferSlot()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the owner material resource manager
		*
		*  @return
		*    The owner material resource manager
		*/
		[[nodiscard]] inline MaterialResourceManager& getMaterialResourceManager() const
		{
			assert(nullptr != mMaterialResourceManager);
			return *mMaterialResourceManager;
		}

		/**
		*  @brief
		*    Return the owner material resource ID
		*
		*  @return
		*    The owner material resource ID
		*/
		[[nodiscard]] inline MaterialResourceId getMaterialResourceId() const
		{
			return mMaterialResourceId;
		}

		/**
		*  @brief
		*    Return the owner material resource instance
		*
		*  @return
		*    The owner material resource instance
		*
		*  @note
		*    - Ease of use method
		*/
		[[nodiscard]] const MaterialResource& getMaterialResource() const;

		/**
		*  @brief
		*    Return the assigned material slot
		*
		*  @return
		*    The assigned material slot
		*/
		[[nodiscard]] inline uint32_t getAssignedMaterialSlot() const
		{
			return mAssignedMaterialSlot;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MaterialBufferSlot(const MaterialBufferSlot&) = delete;
		MaterialBufferSlot& operator=(const MaterialBufferSlot&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		MaterialResourceManager* mMaterialResourceManager;	///< Owner material resource manager, always valid
		MaterialResourceId		 mMaterialResourceId;		///< Owner material resource ID, always valid
		void*					 mAssignedMaterialPool;		///< "RendererRuntime::MaterialBufferManager::BufferPool*", it's a private inner class which we can't forward declare, but we also don't want to expose too much details, so void* it is in here
		uint32_t				 mAssignedMaterialSlot;
		int						 mGlobalIndex;
		bool					 mDirty;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
