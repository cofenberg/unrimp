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
#include "RendererRuntime/Public/Core/GetInvalid.h"
#include "RendererRuntime/Public/Resource/Material/MaterialPropertyValue.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Material property
	*/
	class MaterialProperty final : public MaterialPropertyValue
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class MaterialProperties;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Usage
		*/
		enum class Usage : uint8_t
		{
			UNKNOWN = 0,				///< Usage not known
			STATIC,						///< Static property is used for fixed build in values which usually don't change during runtime (for example hard wired material blueprint textures, hard wired uniform buffer element values or static material properties which the renderer should ignore)
			SHADER_UNIFORM,				///< Dynamic property is considered to change regularly and hence will be handled as shader uniform managed in a combined uniform buffer
			SHADER_COMBINATION,			///< Static property is considered to not change regularly and results in shader combinations
			RASTERIZER_STATE,			///< Graphics pipeline rasterizer state, property is considered to not change regularly
			DEPTH_STENCIL_STATE,		///< Graphics pipeline depth stencil state, property is considered to not change regularly
			BLEND_STATE,				///< Graphics pipeline blend state, property is considered to not change regularly
			SAMPLER_STATE,				///< Sampler state, property is considered to not change regularly
			TEXTURE_REFERENCE,			///< Property is a texture asset reference, property is considered to not change regularly
			GLOBAL_REFERENCE,			///< Property is a global material property reference
			UNKNOWN_REFERENCE,			///< Property is an automatic unknown uniform buffer property reference
			PASS_REFERENCE,				///< Property is an automatic pass uniform buffer property reference
			MATERIAL_REFERENCE,			///< Property is a material uniform buffer property reference
			INSTANCE_REFERENCE,			///< Property is an automatic instance uniform buffer property reference
			GLOBAL_REFERENCE_FALLBACK	///< Property is a fallback for a none existing referenced global material property
		};


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static inline MaterialPropertyValue materialPropertyValueFromReference(ValueType valueType, uint32_t reference)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	 = valueType;
			materialPropertyValue.mValue.Integer = static_cast<int>(reference);
			return materialPropertyValue;
		}

		/**
		*  @brief
		*    Return whether or not the provided material blueprint property usage is a reference to something else
		*
		*  @param[in] usage
		*    Usage to check
		*
		*  @return
		*    "true" if the provided material blueprint property usage is a reference to something else, else "false"
		*/
		static inline bool isReferenceUsage(Usage usage)
		{
			return (Usage::TEXTURE_REFERENCE == usage || Usage::GLOBAL_REFERENCE == usage || Usage::UNKNOWN_REFERENCE == usage || Usage::PASS_REFERENCE == usage || Usage::MATERIAL_REFERENCE == usage || Usage::INSTANCE_REFERENCE == usage);
		}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @note
		*    - For internal usage only
		*/
		inline MaterialProperty() :
			MaterialPropertyValue(fromUnknown()),
			mMaterialPropertyId(getInvalid<MaterialPropertyId>()),
			mUsage(Usage::UNKNOWN),
			mOverwritten(false)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] materialPropertyId
		*    Material property ID
		*  @param[in] usage
		*    Material property usage
		*  @param[in] materialPropertyValue
		*    Material property value
		*/
		inline MaterialProperty(MaterialPropertyId materialPropertyId, Usage usage, const MaterialPropertyValue& materialPropertyValue) :
			MaterialPropertyValue(materialPropertyValue),
			mMaterialPropertyId(materialPropertyId),
			mUsage(usage),
			mOverwritten(false)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~MaterialProperty()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the material property ID
		*
		*  @return
		*    The material property ID
		*/
		inline MaterialPropertyId getMaterialPropertyId() const
		{
			return mMaterialPropertyId;
		}

		/**
		*  @brief
		*    Return the material blueprint property usage
		*
		*  @return
		*    The material blueprint property usage
		*/
		inline Usage getUsage() const
		{
			return mUsage;
		}

		/**
		*  @brief
		*    Return whether or not this is an overwritten property
		*
		*  @return
		*    "true" if this is an overwritten property, else "false"
		*/
		inline bool isOverwritten() const
		{
			return mOverwritten;
		}

		/**
		*  @brief
		*    Set whether or not this is an overwritten property
		*
		*  @param[in] overwritten
		*    "true" if this is an overwritten property, else "false"
		*
		*  @note
		*    - Usually you might not want to manually change the overwritten state
		*/
		inline void setOverwritten(bool overwritten)
		{
			mOverwritten = overwritten;
		}

		/**
		*  @brief
		*    Return whether or not the material blueprint property is a reference to something else
		*
		*  @return
		*    "true" if the material blueprint property is a reference to something else, else "false"
		*/
		inline bool isReferenceUsage() const
		{
			return isReferenceUsage(mUsage);
		}

		//[-------------------------------------------------------]
		//[ Value getter                                          ]
		//[-------------------------------------------------------]
		inline uint32_t getReferenceValue() const
		{
			assert(isReferenceUsage());
			return static_cast<uint32_t>(mValue.Integer);
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		MaterialPropertyId mMaterialPropertyId;
		Usage			   mUsage;
		bool			   mOverwritten;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
