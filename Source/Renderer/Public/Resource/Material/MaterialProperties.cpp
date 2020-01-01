/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "Renderer/Public/Resource/Material/MaterialProperties.h"

#include <algorithm>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	const MaterialProperty* MaterialProperties::getPropertyById(MaterialPropertyId materialPropertyId) const
	{
		SortedPropertyVector::const_iterator iterator = std::lower_bound(mSortedPropertyVector.cbegin(), mSortedPropertyVector.cend(), materialPropertyId, detail::OrderByMaterialPropertyId());
		return (iterator != mSortedPropertyVector.end() && iterator->getMaterialPropertyId() == materialPropertyId) ? &(*iterator) : nullptr;
	}

	MaterialProperty* MaterialProperties::setPropertyById(MaterialPropertyId materialPropertyId, const MaterialPropertyValue& materialPropertyValue, MaterialProperty::Usage materialPropertyUsage, bool changeOverwrittenState)
	{
		// Check whether or not this is a new property or a property value change
		SortedPropertyVector::iterator iterator = std::lower_bound(mSortedPropertyVector.begin(), mSortedPropertyVector.end(), materialPropertyId, detail::OrderByMaterialPropertyId());
		if (iterator == mSortedPropertyVector.end() || iterator->getMaterialPropertyId() != materialPropertyId)
		{
			// Add new material property
			iterator = mSortedPropertyVector.insert(iterator, MaterialProperty(materialPropertyId, materialPropertyUsage, materialPropertyValue));
			if (changeOverwrittenState)
			{
				MaterialProperty* materialProperty = &*iterator;
				materialProperty->mOverwritten = true;
				return materialProperty;
			}
			if (MaterialProperty::Usage::SHADER_COMBINATION == materialPropertyUsage)
			{
				++mShaderCombinationGenerationCounter;
			}
			return &*iterator;
		}

		// Update the material property value, in case there's a material property value change
		else if (*iterator != materialPropertyValue)
		{
			// Sanity checks
			ASSERT(iterator->getValueType() == materialPropertyValue.getValueType());
			ASSERT(MaterialProperty::Usage::UNKNOWN == materialPropertyUsage || materialPropertyUsage == iterator->getUsage());

			// Update the material property value
			materialPropertyUsage = iterator->getUsage();
			*iterator = MaterialProperty(materialPropertyId, materialPropertyUsage, materialPropertyValue);
			if (MaterialProperty::Usage::SHADER_COMBINATION == materialPropertyUsage)
			{
				++mShaderCombinationGenerationCounter;
			}

			// Material property change detected
			if (changeOverwrittenState)
			{
				MaterialProperty* materialProperty = &*iterator;
				materialProperty->mOverwritten = true;
				return materialProperty;
			}
			else
			{
				return &*iterator;
			}
		}

		// No material property change detected
		return nullptr;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
