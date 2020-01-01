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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Export.h"
#include "Renderer/Public/Core/StringId.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;				///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef StringId MaterialPropertyId;	///< Material property identifier, internally just a POD "uint32_t", result of hashing the property name


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Material property value
	*
	*  @remarks
	*    Special property value types
	*    - Reference value types to have properties referencing other data
	*    - Declaration only property for value types were we don't need to store a material property value, but only need to know the value type
	*      (examples are float 3x3 and float 4x4 which would blow up the number of bytes required per material property value without a real usage)
	*/
	class MaterialPropertyValue
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class MaterialProperty;	// Needs access to the constructor for the reference usage


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Value type
		*/
		enum class ValueType : uint8_t
		{
			UNKNOWN = 0,						///< Value type not known
			BOOLEAN,							///< Boolean value
			INTEGER,							///< Integer value
			INTEGER_2,							///< Integer value with two components
			INTEGER_3,							///< Integer value with three components
			INTEGER_4,							///< Integer value with four components
			FLOAT,								///< Float value
			FLOAT_2,							///< Float value with two components
			FLOAT_3,							///< Float value with three components
			FLOAT_4,							///< Float value with four components
			FLOAT_3_3,							///< Float value with nine components, declaration property only
			FLOAT_4_4,							///< Float value with sixteen components, declaration property only
			// For graphics pipeline rasterizer state property usage
			FILL_MODE,							///< Graphics pipeline rasterizer state fill mode with possible values: "WIREFRAME", "SOLID"
			CULL_MODE,							///< Graphics pipeline rasterizer state cull mode with possible values: "NONE", "FRONT", "BACK"
			CONSERVATIVE_RASTERIZATION_MODE,	///< Graphics pipeline rasterizer state conservative rasterization mode with possible values: "OFF", "ON"
			// For graphics pipeline depth stencil state property usage
			DEPTH_WRITE_MASK,					///< Graphics pipeline depth stencil state depth write mask with possible values: "ZERO", "ALL"
			STENCIL_OP,							///< Graphics pipeline depth stencil state stencil function with possible values: "KEEP", "ZERO", "REPLACE", "INCR_SAT", "DECR_SAT", "INVERT", "INCREASE", "DECREASE"
			// For graphics pipeline depth stencil state and sampler state property usage
			COMPARISON_FUNC,					///< Graphics pipeline depth stencil state and sampler state comparison function with possible values: "NEVER", "LESS", "EQUAL", "LESS_EQUAL", "GREATER", "NOT_EQUAL", "GREATER_EQUAL", "ALWAYS"
			// For graphics pipeline blend state property usage
			BLEND,								///< Graphics pipeline blend state blend with possible values: "ZERO", "ONE", "SRC_COLOR", "INV_SRC_COLOR", "SRC_ALPHA", "INV_SRC_ALPHA", "DEST_ALPHA", "INV_DEST_ALPHA", "DEST_COLOR", "INV_DEST_COLOR", "SRC_ALPHA_SAT", "BLEND_FACTOR", "INV_BLEND_FACTOR", "SRC_1_COLOR", "INV_SRC_1_COLOR", "SRC_1_ALPHA", "INV_SRC_1_ALPHA"
			BLEND_OP,							///< Graphics pipeline blend state blend operation with possible values: "ADD", "SUBTRACT", "REV_SUBTRACT", "MIN", "MAX"
			// For sampler state property usage
			FILTER_MODE,						///< Sampler state filter mode with possible values: "MIN_MAG_MIP_POINT", "MIN_MAG_POINT_MIP_LINEAR", "MIN_POINT_MAG_LINEAR_MIP_POINT", "MIN_POINT_MAG_MIP_LINEAR", "MIN_LINEAR_MAG_MIP_POINT", "MIN_LINEAR_MAG_POINT_MIP_LINEAR", "MIN_MAG_LINEAR_MIP_POINT", "MIN_MAG_MIP_LINEAR", "ANISOTROPIC", "COMPARISON_MIN_MAG_MIP_POINT", "COMPARISON_MIN_MAG_POINT_MIP_LINEAR", "COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT", "COMPARISON_MIN_POINT_MAG_MIP_LINEAR", "COMPARISON_MIN_LINEAR_MAG_MIP_POINT", "COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR", "COMPARISON_MIN_MAG_LINEAR_MIP_POINT", "COMPARISON_MIN_MAG_MIP_LINEAR", "COMPARISON_ANISOTROPIC", "UNKNOWN"
			TEXTURE_ADDRESS_MODE,				///< Sampler state texture address mode with possible values: "WRAP", "MIRROR", "CLAMP", "BORDER", "MIRROR_ONCE"
			// For texture property usage
			TEXTURE_ASSET_ID,					///< Texture asset ID
			// For shader combination property usage
			GLOBAL_MATERIAL_PROPERTY_ID			///< Global material property ID
		};


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] RENDERER_API_EXPORT static uint32_t getValueTypeNumberOfBytes(ValueType valueType);

		[[nodiscard]] static inline MaterialPropertyValue fromUnknown()
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType = ValueType::UNKNOWN;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromBoolean(bool value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	 = ValueType::BOOLEAN;
			materialPropertyValue.mValue.Boolean = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger(int value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	 = ValueType::INTEGER;
			materialPropertyValue.mValue.Integer = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger2(int value0, int value1)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		 = ValueType::INTEGER_2;
			materialPropertyValue.mValue.Integer2[0] = value0;
			materialPropertyValue.mValue.Integer2[1] = value1;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger2(int value[2])	// It's your responsibility to take care that there are at least two integers
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		 = ValueType::INTEGER_2;
			materialPropertyValue.mValue.Integer2[0] = value[0];
			materialPropertyValue.mValue.Integer2[1] = value[1];
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger3(int value0, int value1, int value2)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		 = ValueType::INTEGER_3;
			materialPropertyValue.mValue.Integer3[0] = value0;
			materialPropertyValue.mValue.Integer3[1] = value1;
			materialPropertyValue.mValue.Integer3[2] = value2;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger3(int value[3])	// It's your responsibility to take care that there are at least three integers
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		 = ValueType::INTEGER_3;
			materialPropertyValue.mValue.Integer3[0] = value[0];
			materialPropertyValue.mValue.Integer3[1] = value[1];
			materialPropertyValue.mValue.Integer3[2] = value[2];
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger4(int value0, int value1, int value2, int value3)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		 = ValueType::INTEGER_4;
			materialPropertyValue.mValue.Integer4[0] = value0;
			materialPropertyValue.mValue.Integer4[1] = value1;
			materialPropertyValue.mValue.Integer4[2] = value2;
			materialPropertyValue.mValue.Integer4[3] = value3;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromInteger4(int value[4])	// It's your responsibility to take care that there are at least four integers
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		 = ValueType::INTEGER_4;
			materialPropertyValue.mValue.Integer4[0] = value[0];
			materialPropertyValue.mValue.Integer4[1] = value[1];
			materialPropertyValue.mValue.Integer4[2] = value[2];
			materialPropertyValue.mValue.Integer4[3] = value[3];
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat(float value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType   = ValueType::FLOAT;
			materialPropertyValue.mValue.Float = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat2(float value0, float value1)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::FLOAT_2;
			materialPropertyValue.mValue.Float2[0] = value0;
			materialPropertyValue.mValue.Float2[1] = value1;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat2(float value[2])	// It's your responsibility to take care that there are at least two floats
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::FLOAT_2;
			materialPropertyValue.mValue.Float2[0] = value[0];
			materialPropertyValue.mValue.Float2[1] = value[1];
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat3(float value0, float value1, float value2)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::FLOAT_3;
			materialPropertyValue.mValue.Float3[0] = value0;
			materialPropertyValue.mValue.Float3[1] = value1;
			materialPropertyValue.mValue.Float3[2] = value2;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat3(float value[3])	// It's your responsibility to take care that there are at least three floats
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::FLOAT_3;
			materialPropertyValue.mValue.Float3[0] = value[0];
			materialPropertyValue.mValue.Float3[1] = value[1];
			materialPropertyValue.mValue.Float3[2] = value[2];
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat4(float value0, float value1, float value2, float value3)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::FLOAT_4;
			materialPropertyValue.mValue.Float4[0] = value0;
			materialPropertyValue.mValue.Float4[1] = value1;
			materialPropertyValue.mValue.Float4[2] = value2;
			materialPropertyValue.mValue.Float4[3] = value3;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat4(float value[4])	// It's your responsibility to take care that there are at least four floats
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::FLOAT_4;
			materialPropertyValue.mValue.Float4[0] = value[0];
			materialPropertyValue.mValue.Float4[1] = value[1];
			materialPropertyValue.mValue.Float4[2] = value[2];
			materialPropertyValue.mValue.Float4[3] = value[3];
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat3_3()	// Declaration property only
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType = ValueType::FLOAT_3_3;
			materialPropertyValue.mValue.Boolean = false;	// Declaration property only
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromFloat4_4()	// Declaration property only
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType = ValueType::FLOAT_4_4;
			materialPropertyValue.mValue.Boolean = false;	// Declaration property only
			return materialPropertyValue;
		}

		// For graphics pipeline rasterizer state property usage
		[[nodiscard]] static inline MaterialPropertyValue fromFillMode(Rhi::FillMode value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	  = ValueType::FILL_MODE;
			materialPropertyValue.mValue.FillMode = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromCullMode(Rhi::CullMode value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	  = ValueType::CULL_MODE;
			materialPropertyValue.mValue.CullMode = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromConservativeRasterizationMode(Rhi::ConservativeRasterizationMode value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType						   = ValueType::CONSERVATIVE_RASTERIZATION_MODE;
			materialPropertyValue.mValue.ConservativeRasterizationMode = value;
			return materialPropertyValue;
		}

		// For graphics pipeline depth stencil state property usage
		[[nodiscard]] static inline MaterialPropertyValue fromDepthWriteMask(Rhi::DepthWriteMask value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType			= ValueType::DEPTH_WRITE_MASK;
			materialPropertyValue.mValue.DepthWriteMask = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromStencilOp(Rhi::StencilOp value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	   = ValueType::STENCIL_OP;
			materialPropertyValue.mValue.StencilOp = value;
			return materialPropertyValue;
		}

		// For graphics pipeline depth stencil state and sampler state property usage
		[[nodiscard]] static inline MaterialPropertyValue fromComparisonFunc(Rhi::ComparisonFunc value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType			= ValueType::COMPARISON_FUNC;
			materialPropertyValue.mValue.ComparisonFunc = value;
			return materialPropertyValue;
		}

		// For graphics pipeline blend state property usage
		[[nodiscard]] static inline MaterialPropertyValue fromBlend(Rhi::Blend value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType   = ValueType::BLEND;
			materialPropertyValue.mValue.Blend = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromBlendOp(Rhi::BlendOp value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType	 = ValueType::BLEND_OP;
			materialPropertyValue.mValue.BlendOp = value;
			return materialPropertyValue;
		}

		// For sampler state property usage
		[[nodiscard]] static inline MaterialPropertyValue fromFilterMode(Rhi::FilterMode value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType		= ValueType::FILTER_MODE;
			materialPropertyValue.mValue.FilterMode = value;
			return materialPropertyValue;
		}

		[[nodiscard]] static inline MaterialPropertyValue fromTextureAddressMode(Rhi::TextureAddressMode value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType				= ValueType::TEXTURE_ADDRESS_MODE;
			materialPropertyValue.mValue.TextureAddressMode = value;
			return materialPropertyValue;
		}

		// For texture property usage
		[[nodiscard]] static inline MaterialPropertyValue fromTextureAssetId(AssetId value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType			= ValueType::TEXTURE_ASSET_ID;
			materialPropertyValue.mValue.TextureAssetId = value;
			return materialPropertyValue;
		}

		// For shader combination property usage
		[[nodiscard]] static inline MaterialPropertyValue fromGlobalMaterialPropertyId(MaterialPropertyId value)
		{
			MaterialPropertyValue materialPropertyValue;
			materialPropertyValue.mValueType					  = ValueType::GLOBAL_MATERIAL_PROPERTY_ID;
			materialPropertyValue.mValue.GlobalMaterialPropertyId = value;
			return materialPropertyValue;
		}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline ~MaterialPropertyValue()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Return the value type
		*
		*  @return
		*    The value type
		*/
		[[nodiscard]] inline ValueType getValueType() const
		{
			return mValueType;
		}

		//[-------------------------------------------------------]
		//[ Value getter                                          ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const uint8_t* getData() const
		{
			return reinterpret_cast<const uint8_t*>(&mValue);
		}

		[[nodiscard]] inline bool getBooleanValue() const
		{
			ASSERT(ValueType::BOOLEAN == mValueType);
			return mValue.Boolean;
		}

		[[nodiscard]] inline int getIntegerValue() const
		{
			ASSERT(ValueType::INTEGER == mValueType);
			return mValue.Integer;
		}

		[[nodiscard]] inline const int* getInteger2Value() const
		{
			ASSERT(ValueType::INTEGER_2 == mValueType);
			return &mValue.Integer2[0];
		}

		[[nodiscard]] inline const int* getInteger3Value() const
		{
			ASSERT(ValueType::INTEGER_3 == mValueType);
			return &mValue.Integer3[0];
		}

		[[nodiscard]] inline const int* getInteger4Value() const
		{
			ASSERT(ValueType::INTEGER_4 == mValueType);
			return &mValue.Integer4[0];
		}

		[[nodiscard]] inline float getFloatValue() const
		{
			ASSERT(ValueType::FLOAT == mValueType);
			return mValue.Float;
		}

		[[nodiscard]] inline const float* getFloat2Value() const
		{
			ASSERT(ValueType::FLOAT_2 == mValueType);
			return &mValue.Float2[0];
		}

		[[nodiscard]] inline const float* getFloat3Value() const
		{
			ASSERT(ValueType::FLOAT_3 == mValueType);
			return &mValue.Float3[0];
		}

		[[nodiscard]] inline const float* getFloat4Value() const
		{
			ASSERT(ValueType::FLOAT_4 == mValueType);
			return &mValue.Float4[0];
		}

		// [[nodiscard]] inline const float* getFloat3_3Value() const;	// Declaration property only
		// [[nodiscard]] inline const float* getFloat4_4Value() const;	// Declaration property only

		// For graphics pipeline rasterizer state property usage
		[[nodiscard]] inline Rhi::FillMode getFillModeValue() const
		{
			ASSERT(ValueType::FILL_MODE == mValueType);
			return mValue.FillMode;
		}

		[[nodiscard]] inline Rhi::CullMode getCullModeValue() const
		{
			ASSERT(ValueType::CULL_MODE == mValueType);
			return mValue.CullMode;
		}

		[[nodiscard]] inline Rhi::ConservativeRasterizationMode getConservativeRasterizationModeValue() const
		{
			ASSERT(ValueType::CONSERVATIVE_RASTERIZATION_MODE == mValueType);
			return mValue.ConservativeRasterizationMode;
		}

		// For graphics pipeline depth stencil state property usage
		[[nodiscard]] inline Rhi::DepthWriteMask getDepthWriteMaskValue() const
		{
			ASSERT(ValueType::DEPTH_WRITE_MASK == mValueType);
			return mValue.DepthWriteMask;
		}

		[[nodiscard]] inline Rhi::StencilOp getStencilOpValue() const
		{
			ASSERT(ValueType::STENCIL_OP == mValueType);
			return mValue.StencilOp;
		}

		// For graphics pipeline depth stencil state and sampler state property usage
		[[nodiscard]] inline Rhi::ComparisonFunc getComparisonFuncValue() const
		{
			ASSERT(ValueType::COMPARISON_FUNC == mValueType);
			return mValue.ComparisonFunc;
		}

		// For graphics pipeline blend state property usage
		[[nodiscard]] inline Rhi::Blend getBlendValue() const
		{
			ASSERT(ValueType::BLEND == mValueType);
			return mValue.Blend;
		}

		[[nodiscard]] inline Rhi::BlendOp getBlendOpValue() const
		{
			ASSERT(ValueType::BLEND_OP == mValueType);
			return mValue.BlendOp;
		}

		// For sampler state property usage
		[[nodiscard]] inline Rhi::FilterMode getFilterMode() const
		{
			ASSERT(ValueType::FILTER_MODE == mValueType);
			return mValue.FilterMode;
		}

		[[nodiscard]] inline Rhi::TextureAddressMode getTextureAddressModeValue() const
		{
			ASSERT(ValueType::TEXTURE_ADDRESS_MODE == mValueType);
			return mValue.TextureAddressMode;
		}

		// For texture property usage
		[[nodiscard]] inline AssetId getTextureAssetIdValue() const
		{
			ASSERT(ValueType::TEXTURE_ASSET_ID == mValueType);
			return mValue.TextureAssetId;
		}

		// For shader combination property usage
		[[nodiscard]] inline MaterialPropertyId getGlobalMaterialPropertyId() const
		{
			ASSERT(ValueType::GLOBAL_MATERIAL_PROPERTY_ID == mValueType);
			return mValue.GlobalMaterialPropertyId;
		}

		//[-------------------------------------------------------]
		//[ Comparison operator                                   ]
		//[-------------------------------------------------------]
		[[nodiscard]] RENDERER_API_EXPORT bool operator ==(const MaterialPropertyValue& materialPropertyValue) const;

		[[nodiscard]] inline bool operator !=(const MaterialPropertyValue& materialPropertyValue) const
		{
			return !(*this == materialPropertyValue);
		}


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		ValueType mValueType;

		/**
		*  @brief
		*    Value, depends on "Renderer::MaterialProperty::ValueType"
		*/
		union Value
		{
			bool								Boolean;
			int									Integer;
			int									Integer2[2];
			int									Integer3[3];
			int									Integer4[4];
			float								Float;
			float								Float2[2];
			float								Float3[3];
			float								Float4[4];
			// float							Float3_3[9];	// Declaration property only
			// float							Float4_4[16];	// Declaration property only
			// For graphics pipeline rasterizer state property usage
			Rhi::FillMode						FillMode;
			Rhi::CullMode						CullMode;
			Rhi::ConservativeRasterizationMode	ConservativeRasterizationMode;
			// For graphics pipeline depth stencil state property usage
			Rhi::DepthWriteMask					DepthWriteMask;
			Rhi::StencilOp						StencilOp;
			// For graphics pipeline depth stencil state and sampler state property usage
			Rhi::ComparisonFunc					ComparisonFunc;
			// // For graphics pipeline blend state property usage
			Rhi::Blend							Blend;
			Rhi::BlendOp						BlendOp;
			// For sampler state property usage
			Rhi::FilterMode						FilterMode;
			Rhi::TextureAddressMode				TextureAddressMode;
			// For texture property usage
			uint32_t							TextureAssetId;
			// For shader combination property usage
			uint32_t							GlobalMaterialPropertyId;	///< "uint32_t" instead of "MaterialPropertyId" since there's no default constructor
		} mValue;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline MaterialPropertyValue()
		{
			// Nothing here, no member initialization by intent in here (see "Renderer::MaterialPropertyValue::fromBoolean()" etc.)
		}


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
