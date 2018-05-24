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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererToolkit/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Helper/JsonMaterialHelper.h"
#include "RendererToolkit/Helper/StringHelper.h"
#include "RendererToolkit/Helper/JsonHelper.h"
#include "RendererToolkit/Context.h"

#include <RendererRuntime/Core/File/IFile.h>
#include <RendererRuntime/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Resource/ShaderBlueprint/Cache/ShaderProperties.h>
#include <RendererRuntime/Resource/MaterialBlueprint/Loader/MaterialBlueprintFileFormat.h>

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
#include <unordered_set>


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
		bool orderByMaterialPropertyId(const RendererRuntime::MaterialProperty& left, const RendererRuntime::MaterialProperty& right)
		{
			return (left.getMaterialPropertyId() < right.getMaterialPropertyId());
		}

		void optionalBufferUsageProperty(const rapidjson::Value& rapidJsonValueUniformBuffer, const char* propertyName, RendererRuntime::MaterialBlueprintResource::BufferUsage& value)
		{
			if (rapidJsonValueUniformBuffer.HasMember(propertyName))
			{
				const rapidjson::Value& rapidJsonValueUsage = rapidJsonValueUniformBuffer[propertyName];
				const char* valueAsString = rapidJsonValueUsage.GetString();
				const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

				// Define helper macros
				#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = RendererRuntime::MaterialBlueprintResource::BufferUsage::name;
				#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = RendererRuntime::MaterialBlueprintResource::BufferUsage::name;

				// Evaluate value
				IF_VALUE(UNKNOWN)
				ELSE_IF_VALUE(PASS)
				ELSE_IF_VALUE(MATERIAL)
				ELSE_IF_VALUE(INSTANCE)
				ELSE_IF_VALUE(LIGHT)
				else
				{
					throw std::runtime_error("Buffer usage of property \"" + std::string(propertyName) + "\" must be \"UNKNOWN\", \"PASS\", \"MATERIAL\", \"INSTANCE\" or \"LIGHT\", but \"" + std::string(valueAsString) + "\" set");
				}

				// Undefine helper macros
				#undef IF_VALUE
				#undef ELSE_IF_VALUE
			}
		}

		uint32_t roundUpToNextIntegerDivisibleByFactor(uint32_t input, uint32_t factor)
		{
			return (input + factor - 1) / factor * factor;
		}

		// TODO(co) Currently unused. Don't delete the function because it will be used in the future (topic: some more scripting features inside material blueprints).
		/*
		void executeParameterSetInstruction(const std::string& instructionAsString, RendererRuntime::ShaderProperties& shaderProperties)
		{
			// "@pset(<parameter name>, <parameter value to set>)" (same syntax as in "RendererRuntime::ShaderBuilder")

			// Gather required data
			std::vector<std::string> elements;
			RendererToolkit::StringHelper::splitString(instructionAsString, ',', elements);
			if (elements.size() == 2)
			{
				const std::string parameterName = elements[0];
				const int32_t parameterValue = std::atoi(elements[1].c_str());

				// Execute
				shaderProperties.setPropertyValue(RendererRuntime::StringId(parameterName.c_str()), parameterValue);
			}
			else
			{
				throw std::runtime_error("Invalid parameter set instruction, syntax is \"@pset(<parameter name>, <parameter value to set>)\"");
			}
		}
		*/

		int32_t executeCounterInstruction(const std::string& instructionAsString, RendererRuntime::ShaderProperties& shaderProperties)
		{
			// "@counter(<parameter name>)" (same syntax as in "RendererRuntime::ShaderBuilder")

			// Get the shader property ID
			const size_t valueEndIndex = instructionAsString.find(")", 9);
			const RendererRuntime::ShaderPropertyId shaderPropertyId = RendererRuntime::StringId(instructionAsString.substr(9, valueEndIndex - 9).c_str());

			// Execute
			int32_t value = 0;
			shaderProperties.getPropertyValue(shaderPropertyId, value);
			shaderProperties.setPropertyValue(shaderPropertyId, value + 1);

			// Return the parameter value
			return value;
		}

		uint32_t getIntegerFromInstructionString(const char* instructionAsString, RendererRuntime::ShaderProperties& shaderProperties)
		{
			// Check for instruction "@counter(<parameter name>)" (same syntax as in "RendererRuntime::ShaderBuilder")
			// -> TODO(co) We might want to get rid of the implicit std::string parameter conversion below
			return static_cast<uint32_t>((strncmp(instructionAsString, "@counter(", 7) == 0) ? executeCounterInstruction(instructionAsString, shaderProperties) : std::atoi(instructionAsString));
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
	void JsonMaterialBlueprintHelper::optionalPrimitiveTopology(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::PrimitiveTopology& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueUsage.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Renderer::PrimitiveTopology::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Renderer::PrimitiveTopology::name;

			// Evaluate value
			IF_VALUE(POINT_LIST)
			ELSE_IF_VALUE(LINE_LIST)
			ELSE_IF_VALUE(LINE_STRIP)
			ELSE_IF_VALUE(TRIANGLE_LIST)
			ELSE_IF_VALUE(TRIANGLE_STRIP)
			ELSE_IF_VALUE(PATCH_LIST_1)
			ELSE_IF_VALUE(PATCH_LIST_2)
			ELSE_IF_VALUE(PATCH_LIST_3)
			ELSE_IF_VALUE(PATCH_LIST_4)
			ELSE_IF_VALUE(PATCH_LIST_5)
			ELSE_IF_VALUE(PATCH_LIST_6)
			ELSE_IF_VALUE(PATCH_LIST_7)
			ELSE_IF_VALUE(PATCH_LIST_8)
			ELSE_IF_VALUE(PATCH_LIST_9)
			ELSE_IF_VALUE(PATCH_LIST_10)
			ELSE_IF_VALUE(PATCH_LIST_11)
			ELSE_IF_VALUE(PATCH_LIST_12)
			ELSE_IF_VALUE(PATCH_LIST_13)
			ELSE_IF_VALUE(PATCH_LIST_14)
			ELSE_IF_VALUE(PATCH_LIST_15)
			ELSE_IF_VALUE(PATCH_LIST_16)
			ELSE_IF_VALUE(PATCH_LIST_17)
			ELSE_IF_VALUE(PATCH_LIST_18)
			ELSE_IF_VALUE(PATCH_LIST_19)
			ELSE_IF_VALUE(PATCH_LIST_20)
			ELSE_IF_VALUE(PATCH_LIST_21)
			ELSE_IF_VALUE(PATCH_LIST_22)
			ELSE_IF_VALUE(PATCH_LIST_23)
			ELSE_IF_VALUE(PATCH_LIST_24)
			ELSE_IF_VALUE(PATCH_LIST_25)
			ELSE_IF_VALUE(PATCH_LIST_26)
			ELSE_IF_VALUE(PATCH_LIST_27)
			ELSE_IF_VALUE(PATCH_LIST_28)
			ELSE_IF_VALUE(PATCH_LIST_29)
			ELSE_IF_VALUE(PATCH_LIST_30)
			ELSE_IF_VALUE(PATCH_LIST_31)
			ELSE_IF_VALUE(PATCH_LIST_32)
			else
			{
				throw std::runtime_error("Primitive topology of property \"" + std::string(propertyName) + "\" has invalid value \"" + std::string(valueAsString) + "\" set");
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	Renderer::PrimitiveTopologyType JsonMaterialBlueprintHelper::getPrimitiveTopologyTypeByPrimitiveTopology(Renderer::PrimitiveTopology primitiveTopology)
	{
		switch (primitiveTopology)
		{
			default:
			case Renderer::PrimitiveTopology::UNKNOWN:
				return Renderer::PrimitiveTopologyType::UNDEFINED;

			case Renderer::PrimitiveTopology::POINT_LIST:
				return Renderer::PrimitiveTopologyType::POINT;

			case Renderer::PrimitiveTopology::LINE_LIST:
			case Renderer::PrimitiveTopology::LINE_STRIP:
				return Renderer::PrimitiveTopologyType::LINE;

			case Renderer::PrimitiveTopology::TRIANGLE_LIST:
			case Renderer::PrimitiveTopology::TRIANGLE_STRIP:
				return Renderer::PrimitiveTopologyType::TRIANGLE;

			case Renderer::PrimitiveTopology::PATCH_LIST_1:
			case Renderer::PrimitiveTopology::PATCH_LIST_2:
			case Renderer::PrimitiveTopology::PATCH_LIST_3:
			case Renderer::PrimitiveTopology::PATCH_LIST_4:
			case Renderer::PrimitiveTopology::PATCH_LIST_5:
			case Renderer::PrimitiveTopology::PATCH_LIST_6:
			case Renderer::PrimitiveTopology::PATCH_LIST_7:
			case Renderer::PrimitiveTopology::PATCH_LIST_8:
			case Renderer::PrimitiveTopology::PATCH_LIST_9:
			case Renderer::PrimitiveTopology::PATCH_LIST_10:
			case Renderer::PrimitiveTopology::PATCH_LIST_11:
			case Renderer::PrimitiveTopology::PATCH_LIST_12:
			case Renderer::PrimitiveTopology::PATCH_LIST_13:
			case Renderer::PrimitiveTopology::PATCH_LIST_14:
			case Renderer::PrimitiveTopology::PATCH_LIST_15:
			case Renderer::PrimitiveTopology::PATCH_LIST_16:
			case Renderer::PrimitiveTopology::PATCH_LIST_17:
			case Renderer::PrimitiveTopology::PATCH_LIST_18:
			case Renderer::PrimitiveTopology::PATCH_LIST_19:
			case Renderer::PrimitiveTopology::PATCH_LIST_20:
			case Renderer::PrimitiveTopology::PATCH_LIST_21:
			case Renderer::PrimitiveTopology::PATCH_LIST_22:
			case Renderer::PrimitiveTopology::PATCH_LIST_23:
			case Renderer::PrimitiveTopology::PATCH_LIST_24:
			case Renderer::PrimitiveTopology::PATCH_LIST_25:
			case Renderer::PrimitiveTopology::PATCH_LIST_26:
			case Renderer::PrimitiveTopology::PATCH_LIST_27:
			case Renderer::PrimitiveTopology::PATCH_LIST_28:
			case Renderer::PrimitiveTopology::PATCH_LIST_29:
			case Renderer::PrimitiveTopology::PATCH_LIST_30:
			case Renderer::PrimitiveTopology::PATCH_LIST_31:
			case Renderer::PrimitiveTopology::PATCH_LIST_32:
				return Renderer::PrimitiveTopologyType::PATCH;
		}
	}

	void JsonMaterialBlueprintHelper::optionalShaderVisibilityProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::ShaderVisibility& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueUsage.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Renderer::ShaderVisibility::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Renderer::ShaderVisibility::name;

			// Evaluate value
			IF_VALUE(ALL)
			ELSE_IF_VALUE(VERTEX)
			ELSE_IF_VALUE(TESSELLATION_CONTROL)
			ELSE_IF_VALUE(TESSELLATION_EVALUATION)
			ELSE_IF_VALUE(GEOMETRY)
			ELSE_IF_VALUE(FRAGMENT)
			else
			{
				throw std::runtime_error("Shader visibility of property \"" + std::string(propertyName) + "\" must be \"ALL\", \"VERTEX\", \"TESSELLATION_CONTROL\", \"TESSELLATION_EVALUATION\", \"GEOMETRY\" or \"FRAGMENT\", but \"" + std::string(valueAsString) + "\" set");
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}
	}

	RendererRuntime::MaterialProperty::Usage JsonMaterialBlueprintHelper::mandatoryMaterialPropertyUsage(const rapidjson::Value& rapidJsonValue)
	{
		const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue["Usage"];
		const char* valueAsString = rapidJsonValueUsage.GetString();
		const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();
		RendererRuntime::MaterialProperty::Usage usage = RendererRuntime::MaterialProperty::Usage::UNKNOWN;

		// Define helper macros
		#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) usage = RendererRuntime::MaterialProperty::Usage::name;
		#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) usage = RendererRuntime::MaterialProperty::Usage::name;

		// Evaluate value
		IF_VALUE(UNKNOWN)
		ELSE_IF_VALUE(STATIC)
		ELSE_IF_VALUE(SHADER_UNIFORM)
		ELSE_IF_VALUE(SHADER_COMBINATION)
		ELSE_IF_VALUE(RASTERIZER_STATE)
		ELSE_IF_VALUE(DEPTH_STENCIL_STATE)
		ELSE_IF_VALUE(BLEND_STATE)
		ELSE_IF_VALUE(SAMPLER_STATE)
		ELSE_IF_VALUE(TEXTURE_REFERENCE)
		ELSE_IF_VALUE(GLOBAL_REFERENCE)
		ELSE_IF_VALUE(UNKNOWN_REFERENCE)
		ELSE_IF_VALUE(PASS_REFERENCE)
		ELSE_IF_VALUE(MATERIAL_REFERENCE)
		ELSE_IF_VALUE(INSTANCE_REFERENCE)
		ELSE_IF_VALUE(GLOBAL_REFERENCE_FALLBACK)
		else
		{
			throw std::runtime_error("Invalid property usage \"" + std::string(valueAsString) + '\"');
		}

		// Undefine helper macros
		#undef IF_VALUE
		#undef ELSE_IF_VALUE

		// Done
		return usage;
	}

	RendererRuntime::MaterialProperty::ValueType JsonMaterialBlueprintHelper::mandatoryMaterialPropertyValueType(const rapidjson::Value& rapidJsonValue)
	{
		const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue["ValueType"];
		const char* valueAsString = rapidJsonValueValueType.GetString();
		const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();
		RendererRuntime::MaterialProperty::ValueType valueType = RendererRuntime::MaterialProperty::ValueType::UNKNOWN;

		// Define helper macros
		#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) valueType = RendererRuntime::MaterialProperty::ValueType::name;
		#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) valueType = RendererRuntime::MaterialProperty::ValueType::name;

		// Evaluate value
		IF_VALUE(UNKNOWN)
		ELSE_IF_VALUE(BOOLEAN)
		ELSE_IF_VALUE(INTEGER)
		ELSE_IF_VALUE(INTEGER_2)
		ELSE_IF_VALUE(INTEGER_3)
		ELSE_IF_VALUE(INTEGER_4)
		ELSE_IF_VALUE(FLOAT)
		ELSE_IF_VALUE(FLOAT_2)
		ELSE_IF_VALUE(FLOAT_3)
		ELSE_IF_VALUE(FLOAT_4)
		ELSE_IF_VALUE(FLOAT_3_3)
		ELSE_IF_VALUE(FLOAT_4_4)
		ELSE_IF_VALUE(FILL_MODE)
		ELSE_IF_VALUE(CULL_MODE)
		ELSE_IF_VALUE(CONSERVATIVE_RASTERIZATION_MODE)
		ELSE_IF_VALUE(DEPTH_WRITE_MASK)
		ELSE_IF_VALUE(STENCIL_OP)
		ELSE_IF_VALUE(COMPARISON_FUNC)
		ELSE_IF_VALUE(BLEND)
		ELSE_IF_VALUE(BLEND_OP)
		ELSE_IF_VALUE(FILTER_MODE)
		ELSE_IF_VALUE(TEXTURE_ADDRESS_MODE)
		ELSE_IF_VALUE(TEXTURE_ASSET_ID)
		ELSE_IF_VALUE(GLOBAL_MATERIAL_PROPERTY_ID)
		else
		{
			throw std::runtime_error("Invalid property value type \"" + std::string(valueAsString) + '\"');
		}

		// Undefine helper macros
		#undef IF_VALUE
		#undef ELSE_IF_VALUE

		// Done
		return valueType;
	}

	void JsonMaterialBlueprintHelper::getPropertiesByMaterialBlueprintAssetId(const IAssetCompiler::Input& input, RendererRuntime::AssetId materialBlueprintAssetId, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, MaterialPropertyIdToName* materialPropertyIdToName)
	{
		// TODO(co) Error handling and simplification, has several parts of "RendererToolkit::MaterialBlueprintAssetCompiler" in common

		// Read material blueprint asset compiler configuration
		std::string materialBlueprintInputFile;
		const std::string& virtualMaterialBlueprintAssetFilename = input.sourceAssetIdToVirtualAssetFilename(materialBlueprintAssetId);
		{
			// Parse material blueprint asset JSON
			rapidjson::Document rapidJsonDocumentMaterialBlueprintAsset;
			JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualMaterialBlueprintAssetFilename, "Asset", "1", rapidJsonDocumentMaterialBlueprintAsset);
			materialBlueprintInputFile = rapidJsonDocumentMaterialBlueprintAsset["Asset"]["MaterialBlueprintAssetCompiler"]["InputFile"].GetString();
		}

		// Parse material blueprint JSON
		const std::string virtualMaterialBlueprintFilename = std_filesystem::path(virtualMaterialBlueprintAssetFilename).parent_path().generic_string() + '/' + materialBlueprintInputFile;
		rapidjson::Document rapidJsonDocument;
		JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualMaterialBlueprintFilename, "MaterialBlueprintAsset", "2", rapidJsonDocument);
		RendererRuntime::ShaderProperties visualImportanceOfShaderProperties;
		RendererRuntime::ShaderProperties maximumIntegerValueOfShaderProperties;
		readProperties(input, rapidJsonDocument["MaterialBlueprintAsset"]["Properties"], sortedMaterialPropertyVector, visualImportanceOfShaderProperties, maximumIntegerValueOfShaderProperties, true, true, false, materialPropertyIdToName);
	}

	RendererRuntime::MaterialPropertyValue JsonMaterialBlueprintHelper::mandatoryMaterialPropertyValue(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName, const RendererRuntime::MaterialProperty::ValueType valueType)
	{
		// Get the material property default value
		switch (valueType)
		{
			case RendererRuntime::MaterialPropertyValue::ValueType::UNKNOWN:
			{
				// TODO(co) Error, unknown is nothing which is valid when read from assets
				return RendererRuntime::MaterialPropertyValue::fromBoolean(false);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::BOOLEAN:
			{
				int value = 0;
				JsonHelper::optionalBooleanProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromBoolean(0 != value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER:
			{
				int value = 0;
				JsonHelper::optionalIntegerProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromInteger(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_2:
			{
				int values[2] = { 0, 0 };
				JsonHelper::optionalIntegerNProperty(rapidJsonValue, propertyName, values, 2);
				return RendererRuntime::MaterialPropertyValue::fromInteger2(values[0], values[1]);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_3:
			{
				int values[3] = { 0, 0, 0 };
				JsonHelper::optionalIntegerNProperty(rapidJsonValue, propertyName, values, 3);
				return RendererRuntime::MaterialPropertyValue::fromInteger3(values[0], values[1], values[2]);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_4:
			{
				int values[4] = { 0, 0, 0, 0 };
				JsonHelper::optionalIntegerNProperty(rapidJsonValue, propertyName, values, 4);
				return RendererRuntime::MaterialPropertyValue::fromInteger4(values[0], values[1], values[2], values[3]);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT:
			{
				float value = 0.0f;
				JsonHelper::optionalFloatProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromFloat(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_2:
			{
				float values[2] = { 0.0f, 0.0f };
				JsonHelper::optionalFloatNProperty(rapidJsonValue, propertyName, values, 2);
				return RendererRuntime::MaterialPropertyValue::fromFloat2(values[0], values[1]);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3:
			{
				float values[3] = { 0.0f, 0.0f, 0.0f };
				JsonHelper::optionalFloatNProperty(rapidJsonValue, propertyName, values, 3);
				return RendererRuntime::MaterialPropertyValue::fromFloat3(values[0], values[1], values[2]);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4:
			{
				float values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				JsonHelper::optionalFloatNProperty(rapidJsonValue, propertyName, values, 4);
				return RendererRuntime::MaterialPropertyValue::fromFloat4(values[0], values[1], values[2], values[3]);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_3_3:
			{
				// Declaration property only
				return RendererRuntime::MaterialPropertyValue::fromFloat3_3();
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FLOAT_4_4:
			{
				// Declaration property only
				return RendererRuntime::MaterialPropertyValue::fromFloat4_4();
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FILL_MODE:
			{
				Renderer::FillMode value = Renderer::FillMode::SOLID;
				JsonMaterialHelper::optionalFillModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromFillMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::CULL_MODE:
			{
				Renderer::CullMode value = Renderer::CullMode::BACK;
				JsonMaterialHelper::optionalCullModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromCullMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
			{
				Renderer::ConservativeRasterizationMode value = Renderer::ConservativeRasterizationMode::OFF;
				JsonMaterialHelper::optionalConservativeRasterizationModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromConservativeRasterizationMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK:
			{
				Renderer::DepthWriteMask value = Renderer::DepthWriteMask::ALL;
				JsonMaterialHelper::optionalDepthWriteMaskProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromDepthWriteMask(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::STENCIL_OP:
			{
				Renderer::StencilOp value = Renderer::StencilOp::KEEP;
				JsonMaterialHelper::optionalStencilOpProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromStencilOp(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::COMPARISON_FUNC:
			{
				Renderer::ComparisonFunc value = Renderer::ComparisonFunc::GREATER;	// "Renderer::ComparisonFunc::GREATER" instead of "Renderer::ComparisonFunc::LESS" due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				JsonMaterialHelper::optionalComparisonFuncProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromComparisonFunc(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::BLEND:
			{
				Renderer::Blend value = Renderer::Blend::ONE;
				JsonMaterialHelper::optionalBlendProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromBlend(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::BLEND_OP:
			{
				Renderer::BlendOp value = Renderer::BlendOp::ADD;
				JsonMaterialHelper::optionalBlendOpProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromBlendOp(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FILTER_MODE:
			{
				Renderer::FilterMode value = Renderer::FilterMode::MIN_MAG_MIP_LINEAR;
				JsonMaterialHelper::optionalFilterProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromFilterMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE:
			{
				Renderer::TextureAddressMode value = Renderer::TextureAddressMode::CLAMP;
				JsonMaterialHelper::optionalTextureAddressModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromTextureAddressMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID:
			{
				RendererRuntime::AssetId textureAssetId = RendererRuntime::getUninitialized<RendererRuntime::AssetId>();
				if (rapidJsonValue.HasMember(propertyName))
				{
					// Usage of asset IDs is the preferred way to go, but we also need to support the asset ID naming scheme
					// "<project name>/<asset type>/<asset category>/<asset name>" to be able to reference e.g. runtime generated assets
					textureAssetId = StringHelper::getAssetIdByString(rapidJsonValue[propertyName].GetString(), input);
				}
				if (RendererRuntime::isUninitialized(textureAssetId))
				{
					throw std::runtime_error("Inside material blueprints, texture asset reference material properties must always have a value");
				}

				// Done
				return RendererRuntime::MaterialPropertyValue::fromTextureAssetId(textureAssetId);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
			{
				RendererRuntime::MaterialPropertyId materialPropertyId = RendererRuntime::getUninitialized<RendererRuntime::MaterialPropertyId>();
				if (rapidJsonValue.HasMember(propertyName))
				{
					// Get the reference value as string
					static constexpr uint32_t NAME_LENGTH = 128;
					char referenceAsString[NAME_LENGTH] = {};
					JsonHelper::optionalStringProperty(rapidJsonValue, propertyName, referenceAsString, NAME_LENGTH);

					// The character "@" is used to reference e.g. a material property value
					if (referenceAsString[0] == '@')
					{
						// Write down the material property
						materialPropertyId = RendererRuntime::StringId(&referenceAsString[1]);
					}
					else
					{
						throw std::runtime_error("Inside material blueprints, global material property ID material property values must begin with a @");
					}
				}
				if (RendererRuntime::isUninitialized(materialPropertyId))
				{
					throw std::runtime_error("Inside material blueprints, global material property ID material properties must always have a value");
				}

				// Done
				return RendererRuntime::MaterialPropertyValue::fromGlobalMaterialPropertyId(materialPropertyId);
			}
		}

		// TODO(co) Error, we should never ever end up in here
		return RendererRuntime::MaterialPropertyValue::fromBoolean(false);
	}

	void JsonMaterialBlueprintHelper::readRootSignatureByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file)
	{
		// First: Collect everything we need instead of directly writing it down using an inefficient data layout
		std::vector<Renderer::RootParameterData> rootParameters;
		std::vector<Renderer::DescriptorRange> descriptorRanges;
		{
			// Iterate through all resource groups, we're only interested in the following resource parameters
			// - "BaseShaderRegisterName"
			// - "BaseShaderRegister"
			// - "ShaderVisibility"
			// - "ResourceType"
			int resourceGroupIndex = 0;
			RendererRuntime::ShaderProperties shaderProperties;
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResourceGroup = rapidJsonValueResourceGroups.MemberBegin(); rapidJsonMemberIteratorResourceGroup != rapidJsonValueResourceGroups.MemberEnd(); ++rapidJsonMemberIteratorResourceGroup)
			{
				// Sanity check
				if (std::atoi(rapidJsonMemberIteratorResourceGroup->name.GetString()) != resourceGroupIndex)
				{
					throw std::runtime_error("Invalid material blueprint resource group index found, should be " + std::to_string(resourceGroupIndex) + " but is " + rapidJsonMemberIteratorResourceGroup->name.GetString());
				}

				// Iterate through all resources inside the current resource group
				int resourceIndex = 0;
				for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResource = rapidJsonMemberIteratorResourceGroup->value.MemberBegin(); rapidJsonMemberIteratorResource != rapidJsonMemberIteratorResourceGroup->value.MemberEnd(); ++rapidJsonMemberIteratorResource)
				{
					// Sanity check
					if (std::atoi(rapidJsonMemberIteratorResource->name.GetString()) != resourceIndex)
					{
						throw std::runtime_error("Invalid material blueprint resource index inside resource group " + std::to_string(resourceGroupIndex) + " found, should be " + std::to_string(resourceIndex) + " but is " + rapidJsonMemberIteratorResource->name.GetString());
					}

					{ // Process resource
						const rapidjson::Value& rapidJsonValue = rapidJsonMemberIteratorResource->value;
						Renderer::DescriptorRange descriptorRange;

						{ // Mandatory range type
							const rapidjson::Value& rapidJsonValueResourceType = rapidJsonValue["ResourceType"];
							const char* resourceTypeAsString = rapidJsonValueResourceType.GetString();

							// Define helper macros
							#define IF_VALUE(name, rangeTypeValue)			 if (strcmp(resourceTypeAsString, name) == 0) descriptorRange.rangeType = Renderer::DescriptorRangeType::rangeTypeValue;
							#define ELSE_IF_VALUE(name, rangeTypeValue) else if (strcmp(resourceTypeAsString, name) == 0) descriptorRange.rangeType = Renderer::DescriptorRangeType::rangeTypeValue;

							// Evaluate value
							IF_VALUE("UNIFORM_BUFFER", UBV)
							ELSE_IF_VALUE("TEXTURE_BUFFER", UAV)	// TODO(co) Usage of "UAV" is just a temporary hack", also search for other "UAV"-places
							ELSE_IF_VALUE("SAMPLER_STATE", SAMPLER)
							ELSE_IF_VALUE("TEXTURE", SRV)
							else
							{
								throw std::runtime_error("Invalid resource type \"" + std::string(resourceTypeAsString) + "\", must be \"UniformBuffer\", \"TextureBuffer\", \"SamplerState\" or \"Texture\"");
							}

							// Undefine helper macros
							#undef IF_VALUE
							#undef ELSE_IF_VALUE
						}

						// Fixed number of descriptors is always one
						descriptorRange.numberOfDescriptors = 1;

						// Mandatory base shader register
						descriptorRange.baseShaderRegister = ::detail::getIntegerFromInstructionString(rapidJsonValue["BaseShaderRegister"].GetString(), shaderProperties);

						// Fixed register space is always zero
						descriptorRange.registerSpace = 0;

						// Fixed offset in descriptors from table start is always zero
						descriptorRange.offsetInDescriptorsFromTableStart = 0;

						// Optional base shader register name
						descriptorRange.baseShaderRegisterName[0] = '\0';
						JsonHelper::optionalStringProperty(rapidJsonValue, "BaseShaderRegisterName", descriptorRange.baseShaderRegisterName, Renderer::DescriptorRange::NAME_LENGTH);

						// Optional shader visibility
						descriptorRange.shaderVisibility = Renderer::ShaderVisibility::ALL;
						optionalShaderVisibilityProperty(rapidJsonValue, "ShaderVisibility", descriptorRange.shaderVisibility);

						// Add the descriptor range
						descriptorRanges.push_back(descriptorRange);
					}

					// Advance resource index
					++resourceIndex;
				}

				{ // Add the root parameter
					Renderer::RootParameterData rootParameter;
					rootParameter.parameterType = Renderer::RootParameterType::DESCRIPTOR_TABLE;
					rootParameter.numberOfDescriptorRanges = static_cast<uint32_t>(resourceIndex);
					rootParameters.push_back(rootParameter);
				}

				// Advance resource group index
				++resourceGroupIndex;
			}
		}

		{ // Now that we have collected everything we need, perform some editing sanity and security checks before writing down the root signature
		  // -> Base shader register clashes: Direct3D has completely separated shader stages allowing one to e.g. bind a texture buffer at vertex shader texture stage 0 while binding
		  //    a 2D texture at fragment shader stage 0. OpenGL doesn't support something like this and one has to ensure there are no base shader register clashes between separate shader
		  //    stages. Horrible error prone and inflexible restriction, but we can't change that so we have to check for it and sparing the material blueprint editor crazy debugging efforts.
			typedef std::unordered_set<uint32_t> BaseShaderRegisterUsed;	// Key = Base shader register
			BaseShaderRegisterUsed rangeTypeBaseShaderRegisterUsed[static_cast<uint32_t>(Renderer::DescriptorRangeType::NUMBER_OF_RANGE_TYPES)];
			for (const Renderer::DescriptorRange& descriptorRange : descriptorRanges)
			{
				BaseShaderRegisterUsed& baseShaderRegisterUsed = rangeTypeBaseShaderRegisterUsed[static_cast<uint32_t>(descriptorRange.rangeType)];
				if (baseShaderRegisterUsed.find(descriptorRange.baseShaderRegister) == baseShaderRegisterUsed.cend())
				{
					baseShaderRegisterUsed.insert(descriptorRange.baseShaderRegister);
				}
				else
				{
					throw std::runtime_error("Base shader register " + std::to_string(descriptorRange.baseShaderRegister) + " is already used. Please note that to be renderer API independent, base shader register usage is considered to be across all shader stages like OpenGL does.");
				}
			}
		}

		// Sanity check
		if (rootParameters.empty() && !descriptorRanges.empty())
		{
			throw std::runtime_error("Invalid root signature without root parameters but with descriptor ranges detected");
		}

		{ // Write down the root signature header
			RendererRuntime::v1MaterialBlueprint::RootSignatureHeader rootSignatureHeader;
			rootSignatureHeader.numberOfRootParameters	 = static_cast<uint32_t>(rootParameters.size());
			rootSignatureHeader.numberOfDescriptorRanges = static_cast<uint32_t>(descriptorRanges.size());
			rootSignatureHeader.numberOfStaticSamplers	 = 0;									// TODO(co) Add support for static samplers
			rootSignatureHeader.flags					 = Renderer::RootSignatureFlags::NONE;	// TODO(co) Add support for flags
			file.write(&rootSignatureHeader, sizeof(RendererRuntime::v1MaterialBlueprint::RootSignatureHeader));
		}

		// Write down the rest
		if (!rootParameters.empty())
		{
			// Write down the root parameters
			file.write(rootParameters.data(), sizeof(Renderer::RootParameterData) * rootParameters.size());

			// Write down the descriptor ranges
			file.write(descriptorRanges.data(), sizeof(Renderer::DescriptorRange) * descriptorRanges.size());
		}
	}

	void JsonMaterialBlueprintHelper::readProperties(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueProperties, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, RendererRuntime::ShaderProperties& visualImportanceOfShaderProperties, RendererRuntime::ShaderProperties& maximumIntegerValueOfShaderProperties, bool ignoreGlobalReferenceFallback, bool sort, bool referencesAllowed, MaterialPropertyIdToName* materialPropertyIdToName)
	{
		for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIterator = rapidJsonValueProperties.MemberBegin(); rapidJsonMemberIterator != rapidJsonValueProperties.MemberEnd(); ++rapidJsonMemberIterator)
		{
			const rapidjson::Value& rapidJsonValueProperty = rapidJsonMemberIterator->value;

			// Material property ID
			const RendererRuntime::MaterialPropertyId materialPropertyId(rapidJsonMemberIterator->name.GetString());
			if (nullptr != materialPropertyIdToName)
			{
				materialPropertyIdToName->emplace(materialPropertyId, rapidJsonMemberIterator->name.GetString());
			}

			// Material property usage
			// -> Optimization: Material resources don't need to store global reference fallbacks, it's sufficient if those are just stored inside material blueprint resources
			const RendererRuntime::MaterialProperty::Usage usage = mandatoryMaterialPropertyUsage(rapidJsonValueProperty);
			if (!ignoreGlobalReferenceFallback || RendererRuntime::MaterialProperty::Usage::GLOBAL_REFERENCE_FALLBACK != usage)
			{
				const RendererRuntime::MaterialProperty::ValueType valueType = mandatoryMaterialPropertyValueType(rapidJsonValueProperty);
				if (RendererRuntime::MaterialProperty::isReferenceUsage(usage))
				{
					// Get the reference value as string
					static constexpr uint32_t NAME_LENGTH = 128;
					char referenceAsString[NAME_LENGTH] = {};
					JsonHelper::optionalStringProperty(rapidJsonValueProperty, "Value", referenceAsString, NAME_LENGTH);

					// The character "@" is used to reference e.g. a material property value
					if (referenceAsString[0] == '@')
					{
						// Sanity check
						if (!referencesAllowed)
						{
							throw std::runtime_error("Material property \"" + std::string(rapidJsonMemberIterator->name.GetString()) + "\" with value \"" + std::string(referenceAsString) + "\" is using \"@\" to reference e.g. a material property value, but references aren't allowed in the current use-case");
						}

						// Write down the material property
						const RendererRuntime::StringId referenceAsInteger(&referenceAsString[1]);
						sortedMaterialPropertyVector.emplace_back(RendererRuntime::MaterialProperty(materialPropertyId, usage, RendererRuntime::MaterialProperty::materialPropertyValueFromReference(valueType, referenceAsInteger)));
					}
					else
					{
						// Write down the material property
						sortedMaterialPropertyVector.emplace_back(RendererRuntime::MaterialProperty(materialPropertyId, usage, mandatoryMaterialPropertyValue(input, rapidJsonValueProperty, "Value", valueType)));
					}
				}
				else
				{
					// Write down the material property
					sortedMaterialPropertyVector.emplace_back(RendererRuntime::MaterialProperty(materialPropertyId, usage, mandatoryMaterialPropertyValue(input, rapidJsonValueProperty, "Value", valueType)));
				}

				// Optional visual importance of shader property
				if (rapidJsonValueProperty.HasMember("VisualImportance"))
				{
					// Sanity check: "VisualImportance" is only valid for shader combination properties
					if (RendererRuntime::MaterialProperty::Usage::SHADER_COMBINATION == usage)
					{
						const char* valueAsString = rapidJsonValueProperty["VisualImportance"].GetString();
						int32_t visualImportanceOfShaderProperty = RendererRuntime::MaterialBlueprintResource::MANDATORY_SHADER_PROPERTY;
						if (strncmp(valueAsString, "MANDATORY", 9) != 0)
						{
							visualImportanceOfShaderProperty = std::atoi(valueAsString);
						}

						// We're using the same string hashing for material property ID and shader property ID
						visualImportanceOfShaderProperties.setPropertyValue(materialPropertyId, visualImportanceOfShaderProperty);
					}
					else
					{
						throw std::runtime_error("Specifying \"VisualImportance\" is only valid for shader combination properties");
					}
				}
				else if (RendererRuntime::MaterialProperty::Usage::SHADER_COMBINATION == usage)
				{
					// Internally, shader combination properties always need to have a visual importance set
					// -> We're using the same string hashing for material property ID and shader property ID
					visualImportanceOfShaderProperties.setPropertyValue(materialPropertyId, 0);
				}

				// Mandatory maximum value for integer type shader combination properties to be able to keep the total number of shader combinations manageable
				if (RendererRuntime::MaterialProperty::Usage::SHADER_COMBINATION == usage && RendererRuntime::MaterialProperty::ValueType::INTEGER == valueType)
				{
					// TODO(co) Error handling

					// "MaximumIntegerValue" (inclusive)
					const bool hasMaximumIntegerValue = rapidJsonValueProperty.HasMember("MaximumIntegerValue");
					assert(hasMaximumIntegerValue);
					if (hasMaximumIntegerValue)
					{
						const int maximumIntegerValue = std::atoi(rapidJsonValueProperty["MaximumIntegerValue"].GetString());
						assert(maximumIntegerValue > 0);
						maximumIntegerValueOfShaderProperties.setPropertyValue(materialPropertyId, maximumIntegerValue);	// We're using the same string hashing for material property ID and shader property ID
					}
				}
			}
		}

		// Ensure the material properties are sorted, if requested
		if (sort)
		{
			std::sort(sortedMaterialPropertyVector.begin(), sortedMaterialPropertyVector.end(), detail::orderByMaterialPropertyId);
		}
	}

	void JsonMaterialBlueprintHelper::readPipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValuePipelineState, RendererRuntime::IFile& file, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
	{
		{ // Vertex attributes asset ID
			const RendererRuntime::AssetId vertexAttributesAssetId = StringHelper::getAssetIdByString(rapidJsonValuePipelineState["VertexAttributes"].GetString(), input);
			file.write(&vertexAttributesAssetId, sizeof(RendererRuntime::AssetId));
		}

		{ // Shader blueprints
			const rapidjson::Value& rapidJsonValueShaderBlueprints = rapidJsonValuePipelineState["ShaderBlueprints"];

			RendererRuntime::AssetId shaderBlueprintAssetId[RendererRuntime::NUMBER_OF_SHADER_TYPES];
			memset(shaderBlueprintAssetId, static_cast<int>(RendererRuntime::getUninitialized<RendererRuntime::AssetId>()), sizeof(RendererRuntime::AssetId) * RendererRuntime::NUMBER_OF_SHADER_TYPES);
			shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::ShaderType::Vertex)] = JsonHelper::getCompiledAssetId(input, rapidJsonValueShaderBlueprints, "VertexShaderBlueprint");
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "TessellationControlShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::ShaderType::TessellationControl)]);
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "TessellationEvaluationShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::ShaderType::TessellationEvaluation)]);
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "GeometryShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::ShaderType::Geometry)]);
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "FragmentShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::ShaderType::Fragment)]);

			// Write down the shader blueprints
			file.write(&shaderBlueprintAssetId, sizeof(RendererRuntime::AssetId) * RendererRuntime::NUMBER_OF_SHADER_TYPES);
		}

		// Start with the default settings
		Renderer::PipelineState pipelineState = Renderer::PipelineStateBuilder();

		// Optional primitive topology
		optionalPrimitiveTopology(rapidJsonValuePipelineState, "PrimitiveTopology", pipelineState.primitiveTopology);
		pipelineState.primitiveTopologyType = getPrimitiveTopologyTypeByPrimitiveTopology(pipelineState.primitiveTopology);

		// Optional rasterizer state
		if (rapidJsonValuePipelineState.HasMember("RasterizerState"))
		{
			const rapidjson::Value& rapidJsonValueRasterizerState = rapidJsonValuePipelineState["RasterizerState"];
			Renderer::RasterizerState& rasterizerState = pipelineState.rasterizerState;

			// The optional properties
			JsonMaterialHelper::optionalFillModeProperty(rapidJsonValueRasterizerState, "FillMode", rasterizerState.fillMode, &sortedMaterialPropertyVector);
			JsonMaterialHelper::optionalCullModeProperty(rapidJsonValueRasterizerState, "CullMode", rasterizerState.cullMode, &sortedMaterialPropertyVector);
			JsonHelper::optionalBooleanProperty(rapidJsonValueRasterizerState, "FrontCounterClockwise", rasterizerState.frontCounterClockwise);
			JsonHelper::optionalIntegerProperty(rapidJsonValueRasterizerState, "DepthBias", rasterizerState.depthBias);
			JsonHelper::optionalFloatProperty(rapidJsonValueRasterizerState, "DepthBiasClamp", rasterizerState.depthBiasClamp);
			JsonHelper::optionalFloatProperty(rapidJsonValueRasterizerState, "SlopeScaledDepthBias", rasterizerState.slopeScaledDepthBias);
			JsonHelper::optionalBooleanProperty(rapidJsonValueRasterizerState, "DepthClipEnable", rasterizerState.depthClipEnable);
			JsonHelper::optionalBooleanProperty(rapidJsonValueRasterizerState, "MultisampleEnable", rasterizerState.multisampleEnable);
			JsonHelper::optionalBooleanProperty(rapidJsonValueRasterizerState, "AntialiasedLineEnable", rasterizerState.antialiasedLineEnable);
			JsonHelper::optionalIntegerProperty(rapidJsonValueRasterizerState, "ForcedSampleCount", rasterizerState.forcedSampleCount);
			JsonMaterialHelper::optionalConservativeRasterizationModeProperty(rapidJsonValueRasterizerState, "ConservativeRasterizationMode", rasterizerState.conservativeRasterizationMode, &sortedMaterialPropertyVector);
			JsonHelper::optionalBooleanProperty(rapidJsonValueRasterizerState, "ScissorEnable", rasterizerState.scissorEnable);
		}

		// Optional depth stencil state
		if (rapidJsonValuePipelineState.HasMember("DepthStencilState"))
		{
			const rapidjson::Value& rapidJsonValueDepthStencilState = rapidJsonValuePipelineState["DepthStencilState"];
			Renderer::DepthStencilState& depthStencilState = pipelineState.depthStencilState;

			// The optional properties
			JsonHelper::optionalBooleanProperty(rapidJsonValueDepthStencilState, "DepthEnable", depthStencilState.depthEnable);
			JsonMaterialHelper::optionalDepthWriteMaskProperty(rapidJsonValueDepthStencilState, "DepthWriteMask", depthStencilState.depthWriteMask, &sortedMaterialPropertyVector);
			JsonMaterialHelper::optionalComparisonFuncProperty(rapidJsonValueDepthStencilState, "DepthFunc", depthStencilState.depthFunc, &sortedMaterialPropertyVector);

			// TODO(co) Depth stencil state: Read in the rest of the PSO
			/*
			"DepthStencilState":
			{
				"StencilEnable": "FALSE",
				"StencilReadMask": "255",
				"StencilWriteMask": "255",
				"FrontFace":
				{
					"StencilFailOp": "KEEP",
					"StencilDepthFailOp": "KEEP",
					"StencilPassOp": "KEEP",
					"StencilFunc": "ALWAYS"
				},
				"BackFace":
				{
					"StencilFailOp": "KEEP",
					"StencilDepthFailOp": "KEEP",
					"StencilPassOp": "KEEP",
					"StencilFunc": "ALWAYS"
				}
			},
			*/
		}

		// Optional blend state
		if (rapidJsonValuePipelineState.HasMember("BlendState"))
		{
			const rapidjson::Value& rapidJsonValueBlendState = rapidJsonValuePipelineState["BlendState"];
			Renderer::BlendState& blendState = pipelineState.blendState;

			// The optional properties
			JsonHelper::optionalBooleanProperty(rapidJsonValueBlendState, "AlphaToCoverageEnable", blendState.alphaToCoverageEnable, RendererRuntime::MaterialProperty::Usage::BLEND_STATE, &sortedMaterialPropertyVector);
			JsonHelper::optionalBooleanProperty(rapidJsonValueBlendState, "IndependentBlendEnable", blendState.independentBlendEnable);

			// The optional render target properties
			for (int i = 0; i < 8; ++i)
			{
				const std::string renderTarget = "RenderTarget[" + std::to_string(i) + ']';
				if (rapidJsonValueBlendState.HasMember(renderTarget.c_str()))
				{
					const rapidjson::Value& rapidJsonValueRenderTarget = rapidJsonValueBlendState[renderTarget.c_str()];
					Renderer::RenderTargetBlendDesc& renderTargetBlendDesc = blendState.renderTarget[i];

					// The optional properties
					JsonHelper::optionalBooleanProperty(rapidJsonValueRenderTarget, "BlendEnable", renderTargetBlendDesc.blendEnable);
					JsonMaterialHelper::optionalBlendProperty(rapidJsonValueRenderTarget, "SrcBlend", renderTargetBlendDesc.srcBlend, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalBlendProperty(rapidJsonValueRenderTarget, "DestBlend", renderTargetBlendDesc.destBlend, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalBlendOpProperty(rapidJsonValueRenderTarget, "BlendOp", renderTargetBlendDesc.blendOp, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalBlendProperty(rapidJsonValueRenderTarget, "SrcBlendAlpha", renderTargetBlendDesc.srcBlendAlpha, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalBlendProperty(rapidJsonValueRenderTarget, "DestBlendAlpha", renderTargetBlendDesc.destBlendAlpha, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalBlendOpProperty(rapidJsonValueRenderTarget, "BlendOpAlpha", renderTargetBlendDesc.blendOpAlpha, &sortedMaterialPropertyVector);

					// TODO(co) Blend state: Read in the rest of the PSO
					/*
					"RenderTarget[0]":
					{

						"RenderTargetWriteMask": "ALL"
					},
					*/

				}
			}
		}

		// Write down the pipeline state object (PSO)
		file.write(&pipelineState, sizeof(Renderer::SerializedPipelineState));
	}

	void JsonMaterialBlueprintHelper::readUniformBuffersByResourceGroups(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file)
	{
		// Iterate through all resource groups, we're only interested in the following resource parameters
		// - "ResourceType" = "UNIFORM_BUFFER"
		// - "BufferUsage"
		// - "NumberOfElements"
		// - "ElementProperties"
		int resourceGroupIndex = 0;
		for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResourceGroup = rapidJsonValueResourceGroups.MemberBegin(); rapidJsonMemberIteratorResourceGroup != rapidJsonValueResourceGroups.MemberEnd(); ++rapidJsonMemberIteratorResourceGroup)
		{
			// Sanity check
			if (std::atoi(rapidJsonMemberIteratorResourceGroup->name.GetString()) != resourceGroupIndex)
			{
				throw std::runtime_error("Invalid material blueprint resource group index found, should be " + std::to_string(resourceGroupIndex) + " but is " + rapidJsonMemberIteratorResourceGroup->name.GetString());
			}

			// Iterate through all resources inside the current resource group
			int resourceIndex = 0;
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResource = rapidJsonMemberIteratorResourceGroup->value.MemberBegin(); rapidJsonMemberIteratorResource != rapidJsonMemberIteratorResourceGroup->value.MemberEnd(); ++rapidJsonMemberIteratorResource)
			{
				// Sanity check
				if (std::atoi(rapidJsonMemberIteratorResource->name.GetString()) != resourceIndex)
				{
					throw std::runtime_error("Invalid material blueprint resource index inside resource group " + std::to_string(resourceGroupIndex) + " found, should be " + std::to_string(resourceIndex) + " but is " + rapidJsonMemberIteratorResource->name.GetString());
				}

				// We're only interested in uniform buffer resource types
				const rapidjson::Value& rapidJsonValue = rapidJsonMemberIteratorResource->value;
				if (strcmp(rapidJsonValue["ResourceType"].GetString(), "UNIFORM_BUFFER") == 0)
				{
					const rapidjson::Value& rapidJsonValueElementProperties = rapidJsonValue["ElementProperties"];

					// Gather all element properties, don't sort because the user defined order is important in here (data layout in memory)
					RendererRuntime::MaterialProperties::SortedPropertyVector elementProperties;
					RendererRuntime::ShaderProperties visualImportanceOfShaderProperties;
					RendererRuntime::ShaderProperties maximumIntegerValueOfShaderProperties;
					readProperties(input, rapidJsonValueElementProperties, elementProperties, visualImportanceOfShaderProperties, maximumIntegerValueOfShaderProperties, true, false, true);

					// Sanity check
					if (elementProperties.empty())
					{
						// TODO(co) Error handling: General error handling strategy required
						const std::string materialBlueprint = "TODO";
						throw std::runtime_error("Material blueprint " + materialBlueprint + " has a uniform buffer without any element properties");
					}

					// Calculate the uniform buffer size, including handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
					// -> Sum up the number of bytes required by all uniform buffer element properties
					uint32_t numberOfPackageBytes = 0;
					uint32_t numberOfBytesPerElement = 0;
					const size_t numberOfUniformBufferElementProperties = elementProperties.size();
					for (size_t i = 0; i < numberOfUniformBufferElementProperties; ++i)
					{
						// Get value type number of bytes
						const uint32_t valueTypeNumberOfBytes = RendererRuntime::MaterialPropertyValue::getValueTypeNumberOfBytes(elementProperties[i].getValueType());
						numberOfBytesPerElement += valueTypeNumberOfBytes;

						// Handling of packing rules for uniform variables
						//  -> We have to take into account HLSL packing (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
						//  -> GLSL is even more restrictive, with aligning e.g. float2 to an offset divisible by 2 * 4 bytes (float2 size) and float3 to an offset divisible by 4 * 4 bytes (float4 size -- yes, there is no actual float3 alignment)
						if (0 != numberOfPackageBytes)	// No problem if no package was started yet
						{
							// Taking into account GLSL rules here, for HLSL this would always be "numberOfPackageBytes"
							const uint32_t alignmentStartByteOffsetInPackage = ::detail::roundUpToNextIntegerDivisibleByFactor(numberOfPackageBytes, valueTypeNumberOfBytes);

							// Check for float4-size package "overflow" (relevant for both HLSL and GLSL)
							if (numberOfPackageBytes + valueTypeNumberOfBytes > 16)
							{
								// Take the wasted bytes due to aligned packaging into account and restart the package bytes counter
								numberOfBytesPerElement += 4 * 4 - numberOfPackageBytes;
								numberOfPackageBytes = 0;

								// TODO(co) Profiling information: We could provide the material blueprint resource writer with information how many bytes get wasted with the defined layout
							}

							// For GLSL, we are running into problems if there is not overflow, but alignment is not correct
							// TODO(co) Check the documentation, whether there are 16-byte packages at all for GLSL - otherwise this has to be rewritten!
							else if (numberOfPackageBytes != alignmentStartByteOffsetInPackage)
							{
								// TODO(co) Error handling: General error handling strategy required
								const std::string materialBlueprint = "TODO";
								const std::string propertyName = "TODO";
								throw std::runtime_error("Material blueprint " + materialBlueprint + ": Uniform buffer element property alignment is problematic for property " + propertyName + " at offset " + std::to_string(numberOfPackageBytes) + ", which would be aligned to offset " + std::to_string(alignmentStartByteOffsetInPackage));
							}
						}
						numberOfPackageBytes += valueTypeNumberOfBytes % 16;
					}

					// Handling of packing rules for uniform variables (see "Reference for HLSL - Shader Models vs Shader Profiles - Shader Model 4 - Packing Rules for Constant Variables" at https://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx )
					// -> Make a "float 4"-full-house, if required
					if (0 != numberOfPackageBytes)
					{
						// Take the wasted bytes due to aligned packaging into account
						numberOfBytesPerElement += 4 * 4 - numberOfPackageBytes;
					}

					{ // Write down the uniform buffer header
						RendererRuntime::v1MaterialBlueprint::UniformBufferHeader uniformBufferHeader;
						uniformBufferHeader.rootParameterIndex = static_cast<uint32_t>(resourceGroupIndex);
						detail::optionalBufferUsageProperty(rapidJsonValue, "BufferUsage", uniformBufferHeader.bufferUsage);
						JsonHelper::optionalIntegerProperty(rapidJsonValue, "NumberOfElements", uniformBufferHeader.numberOfElements);
						uniformBufferHeader.numberOfElementProperties = static_cast<uint32_t>(elementProperties.size());
						uniformBufferHeader.uniformBufferNumberOfBytes = numberOfBytesPerElement * uniformBufferHeader.numberOfElements;
						file.write(&uniformBufferHeader, sizeof(RendererRuntime::v1MaterialBlueprint::UniformBufferHeader));
					}

					// Write down the uniform buffer element properties
					file.write(elementProperties.data(), sizeof(RendererRuntime::MaterialProperty) * elementProperties.size());
				}

				// Advance resource index
				++resourceIndex;
			}

			// Advance resource group index
			++resourceGroupIndex;
		}
	}

	void JsonMaterialBlueprintHelper::readTextureBuffersByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file)
	{
		// Iterate through all resource groups, we're only interested in the following resource parameters
		// - "ResourceType" = "TEXTURE_BUFFER"
		// - "BufferUsage"
		// - "Value"
		int resourceGroupIndex = 0;
		for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResourceGroup = rapidJsonValueResourceGroups.MemberBegin(); rapidJsonMemberIteratorResourceGroup != rapidJsonValueResourceGroups.MemberEnd(); ++rapidJsonMemberIteratorResourceGroup)
		{
			// Sanity check
			if (std::atoi(rapidJsonMemberIteratorResourceGroup->name.GetString()) != resourceGroupIndex)
			{
				throw std::runtime_error("Invalid material blueprint resource group index found, should be " + std::to_string(resourceGroupIndex) + " but is " + rapidJsonMemberIteratorResourceGroup->name.GetString());
			}

			// Iterate through all resources inside the current resource group
			int resourceIndex = 0;
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResource = rapidJsonMemberIteratorResourceGroup->value.MemberBegin(); rapidJsonMemberIteratorResource != rapidJsonMemberIteratorResourceGroup->value.MemberEnd(); ++rapidJsonMemberIteratorResource)
			{
				// Sanity check
				if (std::atoi(rapidJsonMemberIteratorResource->name.GetString()) != resourceIndex)
				{
					throw std::runtime_error("Invalid material blueprint resource index inside resource group " + std::to_string(resourceGroupIndex) + " found, should be " + std::to_string(resourceIndex) + " but is " + rapidJsonMemberIteratorResource->name.GetString());
				}

				// We're only interested in texture buffer resource types
				const rapidjson::Value& rapidJsonValue = rapidJsonMemberIteratorResource->value;
				if (strcmp(rapidJsonValue["ResourceType"].GetString(), "TEXTURE_BUFFER") == 0)
				{
					// Write down the texture buffer header
					RendererRuntime::v1MaterialBlueprint::TextureBufferHeader textureBufferHeader;
					{ // Value type and value
						const RendererRuntime::MaterialProperty::ValueType valueType = mandatoryMaterialPropertyValueType(rapidJsonValue);

						// Get the reference value as string
						static constexpr uint32_t NAME_LENGTH = 128;
						char referenceAsString[NAME_LENGTH] = {};
						JsonHelper::optionalStringProperty(rapidJsonValue, "Value", referenceAsString, NAME_LENGTH);

						// Construct the material property value
						const RendererRuntime::StringId referenceAsInteger(&referenceAsString[1]);	// Skip the '@'
						textureBufferHeader.materialPropertyValue = RendererRuntime::MaterialProperty::materialPropertyValueFromReference(valueType, referenceAsInteger);
					}
					textureBufferHeader.rootParameterIndex = static_cast<uint32_t>(resourceGroupIndex);
					detail::optionalBufferUsageProperty(rapidJsonValue, "BufferUsage", textureBufferHeader.bufferUsage);
					file.write(&textureBufferHeader, sizeof(RendererRuntime::v1MaterialBlueprint::TextureBufferHeader));
				}

				// Advance resource index
				++resourceIndex;
			}

			// Advance resource group index
			++resourceGroupIndex;
		}
	}

	void JsonMaterialBlueprintHelper::readSamplerStatesByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, RendererRuntime::IFile& file, SamplerBaseShaderRegisterNameToIndex& samplerBaseShaderRegisterNameToIndex)
	{
		// Iterate through all resource groups, we're only interested in the following resource parameters
		// - "ResourceType" = "SAMPLER_STATE"
		// - "BaseShaderRegisterName"
		// - "Filter"
		// - "AddressU"
		// - "AddressV"
		// - "AddressW"
		// - "MipLODBias"
		// - "MaxAnisotropy"
		// - "ComparisonFunc"
		// - "BorderColor"
		// - "MinLOD"
		// - "MaxLOD"
		int resourceGroupIndex = 0;
		uint32_t samplerStateIndex = 0;
		for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResourceGroup = rapidJsonValueResourceGroups.MemberBegin(); rapidJsonMemberIteratorResourceGroup != rapidJsonValueResourceGroups.MemberEnd(); ++rapidJsonMemberIteratorResourceGroup)
		{
			// Sanity check
			if (std::atoi(rapidJsonMemberIteratorResourceGroup->name.GetString()) != resourceGroupIndex)
			{
				throw std::runtime_error("Invalid material blueprint resource group index found, should be " + std::to_string(resourceGroupIndex) + " but is " + rapidJsonMemberIteratorResourceGroup->name.GetString());
			}

			// Iterate through all resources inside the current resource group
			int resourceIndex = 0;
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResource = rapidJsonMemberIteratorResourceGroup->value.MemberBegin(); rapidJsonMemberIteratorResource != rapidJsonMemberIteratorResourceGroup->value.MemberEnd(); ++rapidJsonMemberIteratorResource)
			{
				// Sanity check
				if (std::atoi(rapidJsonMemberIteratorResource->name.GetString()) != resourceIndex)
				{
					throw std::runtime_error("Invalid material blueprint resource index inside resource group " + std::to_string(resourceGroupIndex) + " found, should be " + std::to_string(resourceIndex) + " but is " + rapidJsonMemberIteratorResource->name.GetString());
				}

				// We're only interested in sampler state resource types
				const rapidjson::Value& rapidJsonValue = rapidJsonMemberIteratorResource->value;
				if (strcmp(rapidJsonValue["ResourceType"].GetString(), "SAMPLER_STATE") == 0)
				{
					// Start with the default sampler state
					RendererRuntime::v1MaterialBlueprint::SamplerState materialBlueprintSamplerState;
					materialBlueprintSamplerState.rootParameterIndex = 0;
					Renderer::SamplerState& samplerState = materialBlueprintSamplerState.samplerState;
					samplerState = Renderer::ISamplerState::getDefaultSamplerState();

					{ // Mandatory base shader register name
						char baseShaderRegisterName[Renderer::DescriptorRange::NAME_LENGTH] = {};
						JsonHelper::mandatoryStringProperty(rapidJsonValue, "BaseShaderRegisterName", baseShaderRegisterName, Renderer::DescriptorRange::NAME_LENGTH);
						const uint32_t key = RendererRuntime::StringId::calculateFNV(baseShaderRegisterName);
						if (samplerBaseShaderRegisterNameToIndex.find(key) != samplerBaseShaderRegisterNameToIndex.cend())
						{
							throw std::runtime_error("Sampler state base shader register name \"" + std::string(baseShaderRegisterName) + "\" is defined multiple times");
						}
						samplerBaseShaderRegisterNameToIndex.emplace(key, samplerStateIndex);
						++samplerStateIndex;
					}

					// By default, inside the material blueprint system the texture filter and maximum anisotropy are set to uninitialized. Unless explicitly
					// set by a material blueprint author, those values are dynamic during runtime so the user can decide about the performance/quality trade-off.
					samplerState.filter = Renderer::FilterMode::UNKNOWN;
					RendererRuntime::setUninitialized(samplerState.maxAnisotropy);

					// The optional properties
					materialBlueprintSamplerState.rootParameterIndex = static_cast<uint32_t>(resourceGroupIndex);
					JsonMaterialHelper::optionalFilterProperty(rapidJsonValue, "Filter", samplerState.filter, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalTextureAddressModeProperty(rapidJsonValue, "AddressU", samplerState.addressU, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalTextureAddressModeProperty(rapidJsonValue, "AddressV", samplerState.addressV, &sortedMaterialPropertyVector);
					JsonMaterialHelper::optionalTextureAddressModeProperty(rapidJsonValue, "AddressW", samplerState.addressW, &sortedMaterialPropertyVector);
					JsonHelper::optionalFloatProperty(rapidJsonValue, "MipLODBias", samplerState.mipLODBias);
					JsonHelper::optionalIntegerProperty(rapidJsonValue, "MaxAnisotropy", samplerState.maxAnisotropy);
					JsonMaterialHelper::optionalComparisonFuncProperty(rapidJsonValue, "ComparisonFunc", samplerState.comparisonFunc, &sortedMaterialPropertyVector);
					JsonHelper::optionalFloatNProperty(rapidJsonValue, "BorderColor", samplerState.borderColor, 4);
					JsonHelper::optionalFloatProperty(rapidJsonValue, "MinLOD", samplerState.minLOD);
					JsonHelper::optionalFloatProperty(rapidJsonValue, "MaxLOD", samplerState.maxLOD);

					// Write down the sampler state
					file.write(&materialBlueprintSamplerState, sizeof(RendererRuntime::v1MaterialBlueprint::SamplerState));
				}

				// Advance resource index
				++resourceIndex;
			}

			// Advance resource group index
			++resourceGroupIndex;
		}
	}

	void JsonMaterialBlueprintHelper::readTexturesByResourceGroups(const IAssetCompiler::Input& input, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, const rapidjson::Value& rapidJsonValueResourceGroups, const SamplerBaseShaderRegisterNameToIndex& samplerBaseShaderRegisterNameToIndex, RendererRuntime::IFile& file)
	{
		// Iterate through all resource groups, we're only interested in the following resource parameters
		// - "ResourceType" = "TEXTURE"
		// - "BufferUsage"
		// - "ValueType"
		// - "Value"
		// - "FallbackTexture"
		// - "RgbHardwareGammaCorrection"
		// - "SamplerStateBaseShaderRegisterName"
		int resourceGroupIndex = 0;
		for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResourceGroup = rapidJsonValueResourceGroups.MemberBegin(); rapidJsonMemberIteratorResourceGroup != rapidJsonValueResourceGroups.MemberEnd(); ++rapidJsonMemberIteratorResourceGroup)
		{
			// Sanity check
			if (std::atoi(rapidJsonMemberIteratorResourceGroup->name.GetString()) != resourceGroupIndex)
			{
				throw std::runtime_error("Invalid material blueprint resource group index found, should be " + std::to_string(resourceGroupIndex) + " but is " + rapidJsonMemberIteratorResourceGroup->name.GetString());
			}

			// Iterate through all resources inside the current resource group
			int resourceIndex = 0;
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorResource = rapidJsonMemberIteratorResourceGroup->value.MemberBegin(); rapidJsonMemberIteratorResource != rapidJsonMemberIteratorResourceGroup->value.MemberEnd(); ++rapidJsonMemberIteratorResource)
			{
				// Sanity check
				if (std::atoi(rapidJsonMemberIteratorResource->name.GetString()) != resourceIndex)
				{
					throw std::runtime_error("Invalid material blueprint resource index inside resource group " + std::to_string(resourceGroupIndex) + " found, should be " + std::to_string(resourceIndex) + " but is " + rapidJsonMemberIteratorResource->name.GetString());
				}

				// We're only interested in texture resource types
				const rapidjson::Value& rapidJsonValue = rapidJsonMemberIteratorResource->value;
				if (strcmp(rapidJsonValue["ResourceType"].GetString(), "TEXTURE") == 0)
				{
					// Mandatory root parameter index
					const uint32_t rootParameterIndex = static_cast<uint32_t>(resourceGroupIndex);

					// Mandatory fallback texture asset ID
					// -> We could make this optional, but it's better to be totally restrictive in here so asynchronous texture loading always works nicely (easy when done from the beginning, hard to add this afterwards)
					// -> Often, but not always this value is identical to a texture asset referencing material property. So, we really have to define an own property for this.
					const RendererRuntime::AssetId fallbackTextureAssetId = JsonHelper::getCompiledAssetId(input, rapidJsonValue, "FallbackTexture");

					// Optional RGB hardware gamma correction
					bool rgbHardwareGammaCorrection = false;
					JsonHelper::optionalBooleanProperty(rapidJsonValue, "RgbHardwareGammaCorrection", rgbHardwareGammaCorrection);

					// "MipmapsUsed" with the default value "TRUE" isn't used, but it should be defined if mipmaps are not used to support debugging and optimization possibility spotting

					// Map optional (e.g. texel fetch instead of sampling might be used) "SamplerStateBaseShaderRegisterName" to the index of the material blueprint sampler state resource to use
					uint32_t samplerStateIndex = RendererRuntime::getUninitialized<uint32_t>();
					if (rapidJsonValue.HasMember("SamplerStateBaseShaderRegisterName"))
					{
						char baseShaderRegisterName[Renderer::DescriptorRange::NAME_LENGTH] = {};
						JsonHelper::mandatoryStringProperty(rapidJsonValue, "SamplerStateBaseShaderRegisterName", baseShaderRegisterName, Renderer::DescriptorRange::NAME_LENGTH);
						const uint32_t key = RendererRuntime::StringId::calculateFNV(baseShaderRegisterName);
						SamplerBaseShaderRegisterNameToIndex::const_iterator iterator = samplerBaseShaderRegisterNameToIndex.find(RendererRuntime::StringId::calculateFNV(baseShaderRegisterName));
						if (iterator == samplerBaseShaderRegisterNameToIndex.cend())
						{
							throw std::runtime_error("Unknown sampler state base shader register name \"" + std::string(baseShaderRegisterName) + '\"');
						}
						samplerStateIndex = iterator->second;
					}

					// Mandatory usage
					const RendererRuntime::MaterialProperty::Usage usage = mandatoryMaterialPropertyUsage(rapidJsonValue);
					const RendererRuntime::MaterialProperty::ValueType valueType = mandatoryMaterialPropertyValueType(rapidJsonValue);
					switch (usage)
					{
						case RendererRuntime::MaterialProperty::Usage::STATIC:
						{
							if (RendererRuntime::MaterialProperty::ValueType::TEXTURE_ASSET_ID == valueType)
							{
								// Mandatory asset ID
								const RendererRuntime::MaterialPropertyValue materialPropertyValue = RendererRuntime::MaterialPropertyValue::fromTextureAssetId(StringHelper::getAssetIdByString(rapidJsonValue["Value"].GetString(), input));

								// Write down the texture
								const RendererRuntime::v1MaterialBlueprint::Texture materialBlueprintTexture(rootParameterIndex, RendererRuntime::MaterialProperty(RendererRuntime::getUninitialized<RendererRuntime::MaterialPropertyId>(), usage, materialPropertyValue), fallbackTextureAssetId, rgbHardwareGammaCorrection, samplerStateIndex);
								file.write(&materialBlueprintTexture, sizeof(RendererRuntime::v1MaterialBlueprint::Texture));

								// TODO(co) Error handling: Compiled asset ID not found (meaning invalid source asset ID given)
								break;
							}
							else
							{
								throw std::runtime_error("Textures with \"STATIC\"-usage must have the value type \"TEXTURE_ASSET_ID\"");
							}
						}

						case RendererRuntime::MaterialProperty::Usage::MATERIAL_REFERENCE:
						{
							if (RendererRuntime::MaterialProperty::ValueType::TEXTURE_ASSET_ID == valueType)
							{
								// Get mandatory asset ID
								// -> The character "@" is used to reference a material property value
								const std::string sourceAssetIdAsString = rapidJsonValue["Value"].GetString();
								if (sourceAssetIdAsString.length() > 0 && sourceAssetIdAsString[0] == '@')
								{
									// Reference a material property value
									const RendererRuntime::MaterialPropertyId materialPropertyId(sourceAssetIdAsString.substr(1).c_str());

									// Figure out the material property value
									RendererRuntime::MaterialProperties::SortedPropertyVector::const_iterator iterator = std::lower_bound(sortedMaterialPropertyVector.cbegin(), sortedMaterialPropertyVector.cend(), materialPropertyId, RendererRuntime::detail::OrderByMaterialPropertyId());
									if (iterator != sortedMaterialPropertyVector.end())
									{
										const RendererRuntime::MaterialProperty& materialProperty = *iterator;
										if (materialProperty.getMaterialPropertyId() == materialPropertyId)
										{
											// TODO(co) Error handling: Usage mismatch etc.

											// Write down the texture
											const RendererRuntime::v1MaterialBlueprint::Texture materialBlueprintTexture(rootParameterIndex, RendererRuntime::MaterialProperty(materialPropertyId, usage, materialProperty), fallbackTextureAssetId, rgbHardwareGammaCorrection, samplerStateIndex);
											file.write(&materialBlueprintTexture, sizeof(RendererRuntime::v1MaterialBlueprint::Texture));
										}
									}
								}
								else
								{
									throw std::runtime_error("Textures with \"MATERIAL_REFERENCE\"-usage and the value type \"TEXTURE_ASSET_ID\" must have a value starting with @");
								}
							}
							else
							{
								throw std::runtime_error("Textures with \"MATERIAL_REFERENCE\"-usage must have the value type \"TEXTURE_ASSET_ID\"");
							}
							break;
						}

						case RendererRuntime::MaterialProperty::Usage::UNKNOWN:
						case RendererRuntime::MaterialProperty::Usage::SHADER_UNIFORM:
						case RendererRuntime::MaterialProperty::Usage::SHADER_COMBINATION:
						case RendererRuntime::MaterialProperty::Usage::RASTERIZER_STATE:
						case RendererRuntime::MaterialProperty::Usage::DEPTH_STENCIL_STATE:
						case RendererRuntime::MaterialProperty::Usage::BLEND_STATE:
						case RendererRuntime::MaterialProperty::Usage::SAMPLER_STATE:
						case RendererRuntime::MaterialProperty::Usage::TEXTURE_REFERENCE:
						case RendererRuntime::MaterialProperty::Usage::GLOBAL_REFERENCE:
						case RendererRuntime::MaterialProperty::Usage::UNKNOWN_REFERENCE:
						case RendererRuntime::MaterialProperty::Usage::PASS_REFERENCE:
						case RendererRuntime::MaterialProperty::Usage::INSTANCE_REFERENCE:
						case RendererRuntime::MaterialProperty::Usage::GLOBAL_REFERENCE_FALLBACK:
						default:
						{
							throw std::runtime_error("Invalid texture usage");
						}
					}
				}

				// Advance resource index
				++resourceIndex;
			}

			// Advance resource group index
			++resourceGroupIndex;
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
