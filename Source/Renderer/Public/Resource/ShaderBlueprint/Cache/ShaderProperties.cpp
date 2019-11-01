/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "Renderer/Public/Resource/ShaderBlueprint/Cache/ShaderProperties.h"

#include <algorithm>


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
		[[nodiscard]] inline bool orderPropertyByShaderPropertyId(const Renderer::ShaderProperties::Property& left, const Renderer::ShaderProperties::Property& right)
		{
			return (left.shaderPropertyId < right.shaderPropertyId);
		}


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
	bool ShaderProperties::hasPropertyValue(ShaderPropertyId shaderPropertyId) const
	{
		const Property property(shaderPropertyId, 0);
		SortedPropertyVector::const_iterator iterator = std::lower_bound(mSortedPropertyVector.begin(), mSortedPropertyVector.end(), property, ::detail::orderPropertyByShaderPropertyId);
		return (iterator != mSortedPropertyVector.end() && iterator->shaderPropertyId == property.shaderPropertyId);
	}

	bool ShaderProperties::getPropertyValue(ShaderPropertyId shaderPropertyId, int32_t& value, int32_t defaultValue) const
	{
		const Property property(shaderPropertyId, 0);
		SortedPropertyVector::const_iterator iterator = std::lower_bound(mSortedPropertyVector.begin(), mSortedPropertyVector.end(), property, ::detail::orderPropertyByShaderPropertyId);
		if (iterator != mSortedPropertyVector.end() && iterator->shaderPropertyId == property.shaderPropertyId)
		{
			value = iterator->value;
			return true;
		}
		else
		{
			value = defaultValue;
			return false;
		}
	}

	int32_t ShaderProperties::getPropertyValueUnsafe(ShaderPropertyId shaderPropertyId, int32_t defaultValue) const
	{
		const Property property(shaderPropertyId, 0);
		SortedPropertyVector::const_iterator iterator = std::lower_bound(mSortedPropertyVector.begin(), mSortedPropertyVector.end(), property, ::detail::orderPropertyByShaderPropertyId);
		return (iterator != mSortedPropertyVector.end() && iterator->shaderPropertyId == property.shaderPropertyId) ? iterator->value : defaultValue;
	}

	void ShaderProperties::setPropertyValue(ShaderPropertyId shaderPropertyId, int32_t value)
	{
		const Property property(shaderPropertyId, value);
		SortedPropertyVector::iterator iterator = std::lower_bound(mSortedPropertyVector.begin(), mSortedPropertyVector.end(), property, ::detail::orderPropertyByShaderPropertyId);
		if (iterator == mSortedPropertyVector.end() || iterator->shaderPropertyId != property.shaderPropertyId)
		{
			// Add new shader property
			mSortedPropertyVector.insert(iterator, property);
		}
		else
		{
			// Just update the shader property value
			*iterator = property;
		}
	}

	void ShaderProperties::setPropertyValues(const ShaderProperties& shaderProperties)
	{
		// We'll have to set the properties by using "Renderer::ShaderProperties::setPropertyValue()" in order to maintain the internal vector order
		const SortedPropertyVector& sortedPropertyVector = shaderProperties.getSortedPropertyVector();
		const size_t numberOfProperties = sortedPropertyVector.size();
		for (size_t i = 0; i < numberOfProperties; ++i)
		{
			const Property& property = sortedPropertyVector[i];
			setPropertyValue(property.shaderPropertyId, property.value);
		}
	}

	bool ShaderProperties::operator ==(const ShaderProperties& shaderProperties) const
	{
		const size_t numberOfProperties = mSortedPropertyVector.size();
		const SortedPropertyVector& sortedPropertyVector = shaderProperties.getSortedPropertyVector();
		if (numberOfProperties != sortedPropertyVector.size())
		{
			// Not equal
			return false;
		}
		for (size_t i = 0; i < numberOfProperties; ++i)
		{
			const Property& leftProperty = mSortedPropertyVector[i];
			const Property& rightProperty = sortedPropertyVector[i];
			if (leftProperty.shaderPropertyId != rightProperty.shaderPropertyId || leftProperty.value != rightProperty.value)
			{
				// Not equal
				return false;
			}
		}

		// Equal
		return true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
