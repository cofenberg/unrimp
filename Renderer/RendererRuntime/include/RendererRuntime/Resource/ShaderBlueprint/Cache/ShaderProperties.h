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
#include "RendererRuntime/Export.h"
#include "RendererRuntime/Core/StringId.h"
#include "RendererRuntime/Core/GetUninitialized.h"

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
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId ShaderPropertyId;	///< Shader property identifier, internally just a POD "uint32_t", result of hashing the property name


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Shader properties
	*/
	class ShaderProperties final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct Property final
		{
			ShaderPropertyId shaderPropertyId;
			int32_t			 value;

			inline Property() :
				shaderPropertyId(getUninitialized<ShaderPropertyId>()),
				value(getUninitialized<int32_t>())
			{
				// Nothing here
			}

			inline Property(ShaderPropertyId _shaderPropertyId, int32_t _value) :
				shaderPropertyId(_shaderPropertyId),
				value(_value)
			{
				// Nothing here
			}

			inline bool operator ==(const Property& property) const
			{
				return (shaderPropertyId == property.shaderPropertyId && value == property.value);
			}
		};

		typedef std::vector<Property> SortedPropertyVector;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		inline ShaderProperties()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor with memory reserve (doesn't add properties, property set is still initially empty)
		*
		*  @param[in] numberOfPropertiesToReserve
		*    Number of properties to reserve
		*/
		inline explicit ShaderProperties(size_t numberOfPropertiesToReserve)
		{
			mSortedPropertyVector.reserve(numberOfPropertiesToReserve);
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~ShaderProperties()
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
		inline const SortedPropertyVector& getSortedPropertyVector() const
		{
			return mSortedPropertyVector;
		}

		inline SortedPropertyVector& getSortedPropertyVector()
		{
			return mSortedPropertyVector;
		}

		/**
		*  @brief
		*    Remove all shader properties
		*/
		inline void clear()
		{
			mSortedPropertyVector.clear();
		}

		/**
		*  @brief
		*    Return whether or not a shader property exists
		*
		*  @param[in] shaderPropertyId
		*    ID of the shader property to check for
		*
		*  @return
		*    "true" if the requested shader property exists, else "false" if the requested shader property doesn't exist
		*/
		RENDERERRUNTIME_API_EXPORT bool hasPropertyValue(ShaderPropertyId shaderPropertyId) const;

		/**
		*  @brief
		*    Return the value of a shader property
		*
		*  @param[in] shaderPropertyId
		*    ID of the shader property to return the value from
		*  @param[out] value
		*    Receives the property value
		*  @param[in] defaultValue
		*    Default value in case the shader property doesn't exist
		*
		*  @return
		*    "true" if the requested shader property exists, else "false" if the requested shader property doesn't exist and the default value was returned instead
		*/
		RENDERERRUNTIME_API_EXPORT bool getPropertyValue(ShaderPropertyId shaderPropertyId, int32_t& value, int32_t defaultValue = 0) const;

		/**
		*  @brief
		*    Return the value of a shader property
		*
		*  @param[in] shaderPropertyId
		*    ID of the shader property to return the value from
		*  @param[in] defaultValue
		*    Default value in case the shader property doesn't exist
		*
		*  @return
		*    The property value or the default value
		*
		*  @note
		*    - Unsafe because one can't figure out by the return value whether or not the property exists or if it's just the default value
		*/
		RENDERERRUNTIME_API_EXPORT int32_t getPropertyValueUnsafe(ShaderPropertyId shaderPropertyId, int32_t defaultValue = 0) const;

		/**
		*  @brief
		*    Set the value of a shader property
		*
		*  @param[in] shaderPropertyId
		*    ID of the shader property to set the value of
		*  @param[in] value
		*    The shader property value to set
		*/
		RENDERERRUNTIME_API_EXPORT void setPropertyValue(ShaderPropertyId shaderPropertyId, int32_t value);

		/**
		*  @brief
		*    Set property values by using a given shader properties instance
		*
		*  @param[in] shaderProperties
		*    Shader properties instance to set the shader properties from
		*/
		RENDERERRUNTIME_API_EXPORT void setPropertyValues(const ShaderProperties& shaderProperties);

		RENDERERRUNTIME_API_EXPORT bool operator ==(const ShaderProperties& shaderProperties) const;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		SortedPropertyVector mSortedPropertyVector;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
