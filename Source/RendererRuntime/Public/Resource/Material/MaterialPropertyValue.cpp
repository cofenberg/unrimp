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
#include "RendererRuntime/Public/Resource/Material/MaterialPropertyValue.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	uint32_t MaterialPropertyValue::getValueTypeNumberOfBytes(ValueType valueType)
	{
		switch (valueType)
		{
			case ValueType::UNKNOWN:
				return 0;

			case ValueType::BOOLEAN:
				return sizeof(bool);

			case ValueType::INTEGER:
				return sizeof(int);

			case ValueType::INTEGER_2:
				return sizeof(int) * 2;

			case ValueType::INTEGER_3:
				return sizeof(int) * 3;

			case ValueType::INTEGER_4:
				return sizeof(int) * 4;

			case ValueType::FLOAT:
				return sizeof(float);

			case ValueType::FLOAT_2:
				return sizeof(float) * 2;

			case ValueType::FLOAT_3:
				return sizeof(float) * 3;

			case ValueType::FLOAT_4:
				return sizeof(float) * 4;

			case ValueType::FLOAT_3_3:
				return sizeof(float) * 3 * 3;

			case ValueType::FLOAT_4_4:
				return sizeof(float) * 4 * 4;

			case ValueType::FILL_MODE:
				return sizeof(Renderer::FillMode);

			case ValueType::CULL_MODE:
				return sizeof(Renderer::CullMode);

			case ValueType::CONSERVATIVE_RASTERIZATION_MODE:
				return sizeof(Renderer::ConservativeRasterizationMode);

			case ValueType::DEPTH_WRITE_MASK:
				return sizeof(Renderer::DepthWriteMask);

			case ValueType::STENCIL_OP:
				return sizeof(Renderer::StencilOp);

			case ValueType::COMPARISON_FUNC:
				return sizeof(Renderer::ComparisonFunc);

			case ValueType::BLEND:
				return sizeof(Renderer::Blend);

			case ValueType::BLEND_OP:
				return sizeof(Renderer::BlendOp);

			case ValueType::FILTER_MODE:
				return sizeof(Renderer::FilterMode);

			case ValueType::TEXTURE_ADDRESS_MODE:
				return sizeof(Renderer::TextureAddressMode);

			case ValueType::TEXTURE_ASSET_ID:
				return sizeof(AssetId);

			case ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
				return sizeof(MaterialPropertyId);
		}

		// Error, we should never ever end up in here
		return 0;
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	bool MaterialPropertyValue::operator ==(const MaterialPropertyValue& materialPropertyValue) const
	{
		// Check value type
		if (mValueType != materialPropertyValue.getValueType())
		{
			// Not identical due to value type mismatch
			return false;
		}

		// Check value type
		switch (mValueType)
		{
			case ValueType::UNKNOWN:
				return true;

			case ValueType::BOOLEAN:
				return (mValue.Boolean == materialPropertyValue.mValue.Boolean);

			case ValueType::INTEGER:
				return (mValue.Integer == materialPropertyValue.mValue.Integer);

			case ValueType::INTEGER_2:
				return (mValue.Integer2[0] == materialPropertyValue.mValue.Integer2[0] &&
						mValue.Integer2[1] == materialPropertyValue.mValue.Integer2[1]);

			case ValueType::INTEGER_3:
				return (mValue.Integer3[0] == materialPropertyValue.mValue.Integer3[0] &&
						mValue.Integer3[1] == materialPropertyValue.mValue.Integer3[1] &&
						mValue.Integer3[2] == materialPropertyValue.mValue.Integer3[2]);

			case ValueType::INTEGER_4:
				return (mValue.Integer4[0] == materialPropertyValue.mValue.Integer4[0] &&
						mValue.Integer4[1] == materialPropertyValue.mValue.Integer4[1] &&
						mValue.Integer4[2] == materialPropertyValue.mValue.Integer4[2] &&
						mValue.Integer4[3] == materialPropertyValue.mValue.Integer4[3]);

			case ValueType::FLOAT:
				return (mValue.Float == materialPropertyValue.mValue.Float);

			case ValueType::FLOAT_2:
				return (mValue.Float2[0] == materialPropertyValue.mValue.Float2[0] &&
						mValue.Float2[1] == materialPropertyValue.mValue.Float2[1]);

			case ValueType::FLOAT_3:
				return (mValue.Float3[0] == materialPropertyValue.mValue.Float3[0] &&
						mValue.Float3[1] == materialPropertyValue.mValue.Float3[1] &&
						mValue.Float3[2] == materialPropertyValue.mValue.Float3[2]);

			case ValueType::FLOAT_4:
				return (mValue.Float4[0] == materialPropertyValue.mValue.Float4[0] &&
						mValue.Float4[1] == materialPropertyValue.mValue.Float4[1] &&
						mValue.Float4[2] == materialPropertyValue.mValue.Float4[2] &&
						mValue.Float4[3] == materialPropertyValue.mValue.Float4[3]);

			case ValueType::FLOAT_3_3:
				// Declaration property only
				return true;

			case ValueType::FLOAT_4_4:
				// Declaration property only
				return true;

			case ValueType::FILL_MODE:
				return (mValue.FillMode == materialPropertyValue.mValue.FillMode);

			case ValueType::CULL_MODE:
				return (mValue.CullMode == materialPropertyValue.mValue.CullMode);

			case ValueType::CONSERVATIVE_RASTERIZATION_MODE:
				return (mValue.ConservativeRasterizationMode == materialPropertyValue.mValue.ConservativeRasterizationMode);

			case ValueType::DEPTH_WRITE_MASK:
				return (mValue.DepthWriteMask == materialPropertyValue.mValue.DepthWriteMask);

			case ValueType::STENCIL_OP:
				return (mValue.StencilOp == materialPropertyValue.mValue.StencilOp);

			case ValueType::COMPARISON_FUNC:
				return (mValue.ComparisonFunc == materialPropertyValue.mValue.ComparisonFunc);

			case ValueType::BLEND:
				return (mValue.Blend == materialPropertyValue.mValue.Blend);

			case ValueType::BLEND_OP:
				return (mValue.BlendOp == materialPropertyValue.mValue.BlendOp);

			case ValueType::FILTER_MODE:
				return (mValue.FilterMode == materialPropertyValue.mValue.FilterMode);

			case ValueType::TEXTURE_ADDRESS_MODE:
				return (mValue.TextureAddressMode == materialPropertyValue.mValue.TextureAddressMode);

			case ValueType::TEXTURE_ASSET_ID:
				return (mValue.TextureAssetId == materialPropertyValue.mValue.TextureAssetId);

			case ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
				return (mValue.GlobalMaterialPropertyId == materialPropertyValue.mValue.GlobalMaterialPropertyId);
		}

		// Not identical
		ASSERT(false);
		return false;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
