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
#include "Renderer/Public/Resource/Material/MaterialProperty.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP


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
	*    Material properties
	*/
	class MaterialProperties final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class ShaderPieceResourceLoader;		// TODO(co) Remove this as soon as the incremental hot reloading cache update has been implemented
		friend class ShaderBlueprintResourceLoader;	// TODO(co) Remove this as soon as the incremental hot reloading cache update has been implemented


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<MaterialProperty> SortedPropertyVector;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		inline MaterialProperties() :
			mShaderCombinationGenerationCounter(0u)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~MaterialProperties()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the properties
		*
		*  @return
		*    The properties
		*/
		[[nodiscard]] inline const SortedPropertyVector& getSortedPropertyVector() const
		{
			return mSortedPropertyVector;
		}

		/**
		*  @brief
		*    Remove all material properties
		*/
		inline void removeAllProperties()
		{
			mSortedPropertyVector.clear();
			mShaderCombinationGenerationCounter = 0;
		}

		/**
		*  @brief
		*    Return a material property by its ID
		*
		*  @param[in] materialPropertyId
		*    ID of the material property to return
		*
		*  @return
		*    The requested material property, null pointer on error, don't destroy the returned instance
		*/
		[[nodiscard]] RENDERER_API_EXPORT const MaterialProperty* getPropertyById(MaterialPropertyId materialPropertyId) const;

		/**
		*  @brief
		*    Set a material property value by its ID
		*
		*  @param[in] materialPropertyId
		*    ID of the material property to set the value from
		*  @param[in] materialPropertyValue
		*    The material property value to set
		*  @param[in] materialPropertyUsage
		*    The material property usage
		*  @param[in] changeOverwrittenState
		*    Change overwritten state?
		*
		*  @return
		*    Pointer to the added or changed property, null pointer if no material property change has been detected, don't destroy the returned instance
		*/
		RENDERER_API_EXPORT MaterialProperty* setPropertyById(MaterialPropertyId materialPropertyId, const MaterialPropertyValue& materialPropertyValue, MaterialProperty::Usage materialPropertyUsage = MaterialProperty::Usage::UNKNOWN, bool changeOverwrittenState = false);

		/**
		*  @brief
		*    Return the shader combination generation counter
		*
		*  @return
		*    The shader combination generation counter
		*/
		[[nodiscard]] inline uint32_t getShaderCombinationGenerationCounter() const
		{
			return mShaderCombinationGenerationCounter;
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		SortedPropertyVector mSortedPropertyVector;
		uint32_t			 mShaderCombinationGenerationCounter;	// Incremented each time a shader combination material property gets changed, used for e.g. performance critical renderable cache updates were hashes would be too expensive to generate


	};

	namespace detail
	{
		struct OrderByMaterialPropertyId final
		{
			[[nodiscard]] inline bool operator()(const MaterialProperty& left, MaterialPropertyId right) const
			{
				return (left.getMaterialPropertyId() < right);
			}

			[[nodiscard]] inline bool operator()(MaterialPropertyId left, const MaterialProperty& right) const
			{
				return (left < right.getMaterialPropertyId());
			}
		};
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
