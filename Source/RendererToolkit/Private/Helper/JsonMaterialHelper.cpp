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
#include "RendererToolkit/Private/Helper/JsonMaterialHelper.h"
#include "RendererToolkit/Private/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <Renderer/Public/Core/File/FileSystemHelper.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <rapidjson/document.h>
PRAGMA_WARNING_POP

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
		[[nodiscard]] inline bool orderByMaterialTechniqueId(const Renderer::v1Material::Technique& left, const Renderer::v1Material::Technique& right)
		{
			return (left.materialTechniqueId < right.materialTechniqueId);
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void JsonMaterialHelper::optionalFillModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::FillMode& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::FillMode::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::FillMode::name;

			// Evaluate value
			IF_VALUE(WIREFRAME)
			ELSE_IF_VALUE(SOLID)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::RASTERIZER_STATE, Renderer::MaterialPropertyValue::ValueType::FILL_MODE);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getFillModeValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalCullModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::CullMode& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::CullMode::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::CullMode::name;

			// Evaluate value
			IF_VALUE(NONE)
			ELSE_IF_VALUE(FRONT)
			ELSE_IF_VALUE(BACK)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::RASTERIZER_STATE, Renderer::MaterialPropertyValue::ValueType::CULL_MODE);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getCullModeValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalConservativeRasterizationModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::ConservativeRasterizationMode& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::ConservativeRasterizationMode::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::ConservativeRasterizationMode::name;

			// Evaluate value
			IF_VALUE(OFF)
			ELSE_IF_VALUE(ON)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::RASTERIZER_STATE, Renderer::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getConservativeRasterizationModeValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalDepthWriteMaskProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::DepthWriteMask& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::DepthWriteMask::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::DepthWriteMask::name;

			// Evaluate value
			IF_VALUE(ZERO)
			ELSE_IF_VALUE(ALL)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::DEPTH_STENCIL_STATE, Renderer::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getDepthWriteMaskValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalStencilOpProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::StencilOp& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::StencilOp::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::StencilOp::name;

			// Evaluate value
			IF_VALUE(KEEP)
			ELSE_IF_VALUE(ZERO)
			ELSE_IF_VALUE(REPLACE)
			ELSE_IF_VALUE(INCR_SAT)
			ELSE_IF_VALUE(DECR_SAT)
			ELSE_IF_VALUE(INVERT)
			ELSE_IF_VALUE(INCREASE)
			ELSE_IF_VALUE(DECREASE)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::DEPTH_STENCIL_STATE, Renderer::MaterialPropertyValue::ValueType::STENCIL_OP);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getStencilOpValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalBlendProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::Blend& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::Blend::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::Blend::name;

			// Evaluate value
			IF_VALUE(ZERO)
			ELSE_IF_VALUE(ONE)
			ELSE_IF_VALUE(SRC_COLOR)
			ELSE_IF_VALUE(INV_SRC_COLOR)
			ELSE_IF_VALUE(SRC_ALPHA)
			ELSE_IF_VALUE(INV_SRC_ALPHA)
			ELSE_IF_VALUE(DEST_ALPHA)
			ELSE_IF_VALUE(INV_DEST_ALPHA)
			ELSE_IF_VALUE(DEST_COLOR)
			ELSE_IF_VALUE(INV_DEST_COLOR)
			ELSE_IF_VALUE(SRC_ALPHA_SAT)
			ELSE_IF_VALUE(BLEND_FACTOR)
			ELSE_IF_VALUE(INV_BLEND_FACTOR)
			ELSE_IF_VALUE(SRC_1_COLOR)
			ELSE_IF_VALUE(INV_SRC_1_COLOR)
			ELSE_IF_VALUE(SRC_1_ALPHA)
			ELSE_IF_VALUE(INV_SRC_1_ALPHA)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::BLEND_STATE, Renderer::MaterialPropertyValue::ValueType::BLEND);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getBlendValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalBlendOpProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::BlendOp& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::BlendOp::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::BlendOp::name;

			// Evaluate value
			IF_VALUE(ADD)
			ELSE_IF_VALUE(SUBTRACT)
			ELSE_IF_VALUE(REV_SUBTRACT)
			ELSE_IF_VALUE(MIN)
			ELSE_IF_VALUE(MAX)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::BLEND_STATE, Renderer::MaterialPropertyValue::ValueType::BLEND_OP);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getBlendOpValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalFilterProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::FilterMode& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::FilterMode::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::FilterMode::name;

			// Evaluate value
			IF_VALUE(MIN_MAG_MIP_POINT)
			ELSE_IF_VALUE(MIN_MAG_POINT_MIP_LINEAR)
			ELSE_IF_VALUE(MIN_POINT_MAG_LINEAR_MIP_POINT)
			ELSE_IF_VALUE(MIN_POINT_MAG_MIP_LINEAR)
			ELSE_IF_VALUE(MIN_LINEAR_MAG_MIP_POINT)
			ELSE_IF_VALUE(MIN_LINEAR_MAG_POINT_MIP_LINEAR)
			ELSE_IF_VALUE(MIN_MAG_LINEAR_MIP_POINT)
			ELSE_IF_VALUE(MIN_MAG_MIP_LINEAR)
			ELSE_IF_VALUE(ANISOTROPIC)
			ELSE_IF_VALUE(COMPARISON_MIN_MAG_MIP_POINT)
			ELSE_IF_VALUE(COMPARISON_MIN_MAG_POINT_MIP_LINEAR)
			ELSE_IF_VALUE(COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT)
			ELSE_IF_VALUE(COMPARISON_MIN_POINT_MAG_MIP_LINEAR)
			ELSE_IF_VALUE(COMPARISON_MIN_LINEAR_MAG_MIP_POINT)
			ELSE_IF_VALUE(COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR)
			ELSE_IF_VALUE(COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
			ELSE_IF_VALUE(COMPARISON_MIN_MAG_MIP_LINEAR)
			ELSE_IF_VALUE(COMPARISON_ANISOTROPIC)
			ELSE_IF_VALUE(UNKNOWN)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::SAMPLER_STATE, Renderer::MaterialPropertyValue::ValueType::FILTER_MODE);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getFilterMode();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalTextureAddressModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::TextureAddressMode& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::TextureAddressMode::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::TextureAddressMode::name;

			// Evaluate value
			IF_VALUE(WRAP)
			ELSE_IF_VALUE(MIRROR)
			ELSE_IF_VALUE(CLAMP)
			ELSE_IF_VALUE(BORDER)
			ELSE_IF_VALUE(MIRROR_ONCE)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::SAMPLER_STATE, Renderer::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getTextureAddressModeValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::optionalComparisonFuncProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::ComparisonFunc& value, const Renderer::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValueType.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::ComparisonFunc::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::ComparisonFunc::name;

			// Evaluate value
			IF_VALUE(NEVER)
			ELSE_IF_VALUE(LESS)
			ELSE_IF_VALUE(EQUAL)
			ELSE_IF_VALUE(LESS_EQUAL)
			ELSE_IF_VALUE(GREATER)
			ELSE_IF_VALUE(NOT_EQUAL)
			ELSE_IF_VALUE(GREATER_EQUAL)
			ELSE_IF_VALUE(ALWAYS)
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const Renderer::MaterialProperty* materialProperty = JsonHelper::getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, Renderer::MaterialProperty::Usage::SAMPLER_STATE, Renderer::MaterialPropertyValue::ValueType::COMPARISON_FUNC);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getComparisonFuncValue();
				}
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	void JsonMaterialHelper::readMaterialPropertyValues(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueProperties, Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
	{
		for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorProperties = rapidJsonValueProperties.MemberBegin(); rapidJsonMemberIteratorProperties != rapidJsonValueProperties.MemberEnd(); ++rapidJsonMemberIteratorProperties)
		{
			// Material property ID
			const char* propertyName = rapidJsonMemberIteratorProperties->name.GetString();
			const Renderer::MaterialPropertyId materialPropertyId(propertyName);

			// Figure out the material property value type by using the material blueprint
			Renderer::MaterialProperties::SortedPropertyVector::iterator iterator = std::lower_bound(sortedMaterialPropertyVector.begin(), sortedMaterialPropertyVector.end(), materialPropertyId, Renderer::detail::OrderByMaterialPropertyId());
			if (iterator != sortedMaterialPropertyVector.end() && (*iterator).getMaterialPropertyId() == materialPropertyId)
			{
				// Set the material own property value
				Renderer::MaterialProperty& materialProperty = *iterator;
				static_cast<Renderer::MaterialPropertyValue&>(materialProperty) = JsonMaterialBlueprintHelper::mandatoryMaterialPropertyValue(input, rapidJsonValueProperties, propertyName, materialProperty.getValueType());
				materialProperty.setOverwritten(true);
			}
			else
			{
				throw std::runtime_error(std::string("Material property \"") + propertyName + "\" is unknown");
			}
		}
	}

	void JsonMaterialHelper::getTechniquesAndPropertiesByMaterialAssetId(const IAssetCompiler::Input& input, rapidjson::Document& rapidJsonDocument, std::vector<Renderer::v1Material::Technique>& techniques, Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
	{
		const rapidjson::Value& rapidJsonValueMaterialAsset = rapidJsonDocument["MaterialAsset"];

		// Optional base material
		// -> Named toolkit time base material and not parent material by intent to not intermix it with the dynamic runtime parent material
		if (rapidJsonValueMaterialAsset.HasMember("BaseMaterial"))
		{
			// Sanity check
			if (rapidJsonValueMaterialAsset.HasMember("Techniques"))
			{
				throw std::runtime_error("The material has a base material defined as well as techniques. There can be only one of them.");
			}

			// Get material techniques and properties from the base material
			getPropertiesByMaterialAssetId(input, StringHelper::getSourceAssetIdByString(rapidJsonValueMaterialAsset["BaseMaterial"].GetString(), input), sortedMaterialPropertyVector, &techniques);
		}
		else
		{
			// Gather the asset IDs of all used material blueprints (one material blueprint per material technique)
			const rapidjson::Value& rapidJsonValueTechniques = rapidJsonValueMaterialAsset["Techniques"];
			techniques.reserve(rapidJsonValueTechniques.MemberCount());
			std::unordered_map<uint32_t, std::string> materialTechniqueIdToName;	// Key = "Renderer::MaterialTechniqueId"
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorTechniques = rapidJsonValueTechniques.MemberBegin(); rapidJsonMemberIteratorTechniques != rapidJsonValueTechniques.MemberEnd(); ++rapidJsonMemberIteratorTechniques)
			{
				const std::string sourceAssetIdAsString = rapidJsonMemberIteratorTechniques->value.GetString();

				// Add technique
				Renderer::v1Material::Technique technique;
				technique.materialTechniqueId	   = Renderer::StringId(rapidJsonMemberIteratorTechniques->name.GetString());
				technique.materialBlueprintAssetId = StringHelper::getSourceAssetIdByString(sourceAssetIdAsString, input);
				techniques.push_back(technique);
				materialTechniqueIdToName.emplace(technique.materialTechniqueId, rapidJsonMemberIteratorTechniques->name.GetString());

				// Sanity check since later on we're not able to recover the original asset ID as string
				SourceAssetIdToVirtualFilename::const_iterator iterator = input.sourceAssetIdToVirtualFilename.find(technique.materialBlueprintAssetId);
				if (input.sourceAssetIdToVirtualFilename.cend() == iterator)
				{
					throw std::runtime_error("Failed to map source asset ID " + sourceAssetIdAsString + " to virtual asset filename");
				}
			}
			std::sort(techniques.begin(), techniques.end(), detail::orderByMaterialTechniqueId);

			// Gather all material blueprint properties of all referenced material blueprints
			for (Renderer::v1Material::Technique& technique : techniques)
			{
				Renderer::MaterialProperties::SortedPropertyVector newSortedMaterialPropertyVector;
				MaterialPropertyIdToName materialPropertyIdToName;
				JsonMaterialBlueprintHelper::getPropertiesByMaterialBlueprintAssetId(input, technique.materialBlueprintAssetId, newSortedMaterialPropertyVector, &materialPropertyIdToName);

				// Add properties and avoid duplicates while doing so
				for (const Renderer::MaterialProperty& materialProperty : newSortedMaterialPropertyVector)
				{
					const Renderer::MaterialPropertyId materialPropertyId = materialProperty.getMaterialPropertyId();
					Renderer::MaterialProperties::SortedPropertyVector::const_iterator iterator = std::lower_bound(sortedMaterialPropertyVector.cbegin(), sortedMaterialPropertyVector.cend(), materialPropertyId, Renderer::detail::OrderByMaterialPropertyId());
					if (iterator == sortedMaterialPropertyVector.end() || iterator->getMaterialPropertyId() != materialPropertyId)
					{
						// Add new material property
						sortedMaterialPropertyVector.insert(iterator, materialProperty);
					}
					else
					{
						// Sanity checks
						// -> Do not check for usage since some material properties like "UseAlbedoMap" might be defined inside some material blueprints just for consistency using an unknown usage
						const Renderer::MaterialProperty& otherMaterialProperty = *iterator;
						if (materialProperty.getValueType() != otherMaterialProperty.getValueType())
						{
							throw std::runtime_error(std::string("Material blueprint asset ") + input.sourceAssetIdToDebugName(technique.materialBlueprintAssetId) + " referenced by material technique \"" + materialTechniqueIdToName[technique.materialTechniqueId] + "\" has material property \"" + materialPropertyIdToName[materialProperty.getMaterialPropertyId()] + "\" which differs in value type to another already registered material property. Ensure that the overlapping material properties of all referenced material blueprint assets are consistent.");
						}
						if (static_cast<const Renderer::MaterialPropertyValue&>(materialProperty) != static_cast<const Renderer::MaterialPropertyValue&>(otherMaterialProperty))
						{
							throw std::runtime_error(std::string("Material blueprint asset ") + input.sourceAssetIdToDebugName(technique.materialBlueprintAssetId) + " referenced by material technique \"" + materialTechniqueIdToName[technique.materialTechniqueId] + "\" has material property \"" + materialPropertyIdToName[materialProperty.getMaterialPropertyId()] + "\" which differs in default value to another already registered material property. Ensure that the overlapping material properties of all referenced material blueprint assets are consistent.");
						}
					}
				}

				// Transform the source asset ID into a local asset ID
				technique.materialBlueprintAssetId = input.getCompiledAssetIdBySourceAssetId(technique.materialBlueprintAssetId);
			}
		}

		// Optional properties: Update material property values were required
		if (rapidJsonValueMaterialAsset.HasMember("Properties"))
		{
			JsonMaterialHelper::readMaterialPropertyValues(input, rapidJsonValueMaterialAsset["Properties"], sortedMaterialPropertyVector);
		}
	}

	void JsonMaterialHelper::getPropertiesByMaterialAssetId(const IAssetCompiler::Input& input, Renderer::AssetId materialAssetId, Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, std::vector<Renderer::v1Material::Technique>* techniques)
	{
		// TODO(co) Error handling and simplification

		// Read material asset compiler configuration
		// -> ".asset"-check for automatically in-memory generated ".asset"-file support
		std::string materialInputFile;
		const std::string& virtualMaterialAssetFilename = input.sourceAssetIdToVirtualAssetFilename(materialAssetId);
		if (virtualMaterialAssetFilename.find(".asset") != std::string::npos)
		{
			// Explicit ".asset"-file: Parse material asset JSON
			rapidjson::Document rapidJsonDocumentMaterialAsset;
			JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualMaterialAssetFilename, "Asset", "1", rapidJsonDocumentMaterialAsset);
			materialInputFile = JsonHelper::getAssetInputFileByRapidJsonDocument(rapidJsonDocumentMaterialAsset);
		}
		else
		{
			// Automatically in-memory generated ".asset"-file
			materialInputFile = std_filesystem::path(virtualMaterialAssetFilename).filename().generic_string();
		}

		// Parse material JSON
		const std::string virtualMaterialDirectory = std_filesystem::path(virtualMaterialAssetFilename).parent_path().generic_string();
		const std::string virtualMaterialFilename = virtualMaterialDirectory + '/' + materialInputFile;
		rapidjson::Document rapidJsonDocument;
		JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualMaterialFilename, "MaterialAsset", "1", rapidJsonDocument);
		std::vector<Renderer::v1Material::Technique> temporaryTechniques;
		const IAssetCompiler::Input materialAssetInput(input.context, input.projectName, input.cacheManager, input.virtualAssetPackageInputDirectory, virtualMaterialFilename, virtualMaterialDirectory, input.virtualAssetOutputDirectory, input.sourceAssetIdToCompiledAssetId, input.compiledAssetIdToSourceAssetId, input.sourceAssetIdToVirtualFilename, input.defaultTextureAssetIds);
		getTechniquesAndPropertiesByMaterialAssetId(materialAssetInput, rapidJsonDocument, (nullptr != techniques) ? *techniques : temporaryTechniques, sortedMaterialPropertyVector);
	}

	void JsonMaterialHelper::getDependencyFiles(const IAssetCompiler::Input& input, const std::string& virtualInputFilename, std::vector<std::string>& virtualDependencyFilenames)
	{
		// Parse JSON
		rapidjson::Document rapidJsonDocument;
		JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualInputFilename, "MaterialAsset", "1", rapidJsonDocument);

		{ // Optional base material
		  // -> Named toolkit time base material and not parent material by intent to not intermix it with the dynamic runtime parent material
			const rapidjson::Value& rapidJsonValueMaterialAsset = rapidJsonDocument["MaterialAsset"];
			if (rapidJsonValueMaterialAsset.HasMember("BaseMaterial"))
			{
				// Sanity check
				if (rapidJsonValueMaterialAsset.HasMember("Techniques"))
				{
					throw std::runtime_error("The material has a base material defined as well as techniques. There can be only one of them.");
				}

				// Get base material asset ID
				const rapidjson::Value& rapidJsonValueBaseMaterial = rapidJsonValueMaterialAsset["BaseMaterial"];
				std::string baseMaterialVirtualInputFilename;
				try
				{
					const Renderer::AssetId materialAssetId = StringHelper::getSourceAssetIdByString(rapidJsonValueBaseMaterial.GetString(), input);
					baseMaterialVirtualInputFilename = input.sourceAssetIdToVirtualAssetFilename(materialAssetId);
					virtualDependencyFilenames.emplace_back(baseMaterialVirtualInputFilename);
					StringHelper::replaceFirstString(baseMaterialVirtualInputFilename, ".asset", ".material");
				}
				catch (const std::exception& e)
				{
					throw std::runtime_error("Failed to gather dependency files of material source asset \"" + virtualInputFilename + "\" due to unknown base material source asset \"" + std::string(rapidJsonValueBaseMaterial.GetString()) + "\": " + std::string(e.what()));
				}

				// Go down the rabbit hole recursively
				try
				{
					const IAssetCompiler::Input materialAssetInput(input.context, input.projectName, input.cacheManager, input.virtualAssetPackageInputDirectory, baseMaterialVirtualInputFilename, std_filesystem::path(baseMaterialVirtualInputFilename).parent_path().generic_string(), input.virtualAssetOutputDirectory, input.sourceAssetIdToCompiledAssetId, input.compiledAssetIdToSourceAssetId, input.sourceAssetIdToVirtualFilename, input.defaultTextureAssetIds);
					getDependencyFiles(materialAssetInput, baseMaterialVirtualInputFilename, virtualDependencyFilenames);
				}
				catch (const std::exception& e)
				{
					throw std::runtime_error("Failed to gather dependency files of base material source asset \"" + baseMaterialVirtualInputFilename + "\": " + std::string(e.what()));
				}
			}
			else
			{
				const rapidjson::Value& rapidJsonValueTechniques = rapidJsonValueMaterialAsset["Techniques"];
				for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorTechniques = rapidJsonValueTechniques.MemberBegin(); rapidJsonMemberIteratorTechniques != rapidJsonValueTechniques.MemberEnd(); ++rapidJsonMemberIteratorTechniques)
				{
					const Renderer::AssetId materialBlueprintAssetId = StringHelper::getSourceAssetIdByString(rapidJsonMemberIteratorTechniques->value.GetString(), input);
					SourceAssetIdToVirtualFilename::const_iterator iterator = input.sourceAssetIdToVirtualFilename.find(materialBlueprintAssetId);
					if (input.sourceAssetIdToVirtualFilename.cend() == iterator)
					{
						throw std::runtime_error("Failed to map source asset ID " + std::string(rapidJsonMemberIteratorTechniques->value.GetString()) + " to virtual asset filename");
					}
					virtualDependencyFilenames.emplace_back(input.sourceAssetIdToVirtualAssetFilename(materialBlueprintAssetId));
				}
			}
		}

		// Take material property source asset references into account
		try
		{
			std::vector<Renderer::v1Material::Technique> techniques;
			Renderer::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVector;
			JsonMaterialHelper::getTechniquesAndPropertiesByMaterialAssetId(input, rapidJsonDocument, techniques, sortedMaterialPropertyVector);
			for (Renderer::MaterialProperty& materialProperty : sortedMaterialPropertyVector)
			{
				if (materialProperty.getValueType() == Renderer::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID)
				{
					// The material property stores a compiled texture asset ID
					const Renderer::AssetId textureAssetId = materialProperty.getTextureAssetIdValue();

					// Ignore fixed build in texture assets
					if (input.defaultTextureAssetIds.find(textureAssetId) == input.defaultTextureAssetIds.cend())
					{
						virtualDependencyFilenames.emplace_back(input.compiledAssetIdToVirtualAssetFilename(textureAssetId));
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Failed to gather dependency files of material source asset \"" + virtualInputFilename + "\" because one of the material properties is referencing an unknown source asset: " + std::string(e.what()));
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
