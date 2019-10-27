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
#include "RendererToolkit/Private/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Private/Helper/JsonMaterialHelper.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/Core/File/IFile.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Resource/Material/MaterialResource.h>
#include <RendererRuntime/Public/Resource/ShaderBlueprint/Cache/ShaderProperties.h>
#include <RendererRuntime/Public/Resource/MaterialBlueprint/Loader/MaterialBlueprintFileFormat.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(4866)	// warning C4866: compiler may not enforce left-to-right evaluation order for call to 'rapidjson::GenericValue<rapidjson::UTF8<char>,rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> >::operator[]<rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> >'
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
		[[nodiscard]] inline bool orderByMaterialPropertyId(const RendererRuntime::MaterialProperty& left, const RendererRuntime::MaterialProperty& right)
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

		[[nodiscard]] uint32_t roundUpToNextIntegerDivisibleByFactor(uint32_t input, uint32_t factor)
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

		[[nodiscard]] int32_t executeCounterInstruction(const std::string& instructionAsString, RendererRuntime::ShaderProperties& shaderProperties)
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

		[[nodiscard]] uint32_t getIntegerFromInstructionString(const char* instructionAsString, RendererRuntime::ShaderProperties& shaderProperties)
		{
			// Check for instruction "@counter(<parameter name>)" (same syntax as in "RendererRuntime::ShaderBuilder")
			// -> TODO(co) We might want to get rid of the implicit std::string parameter conversion below
			return static_cast<uint32_t>((strncmp(instructionAsString, "@counter(", 7) == 0) ? executeCounterInstruction(instructionAsString, shaderProperties) : std::atoi(instructionAsString));
		}

		[[nodiscard]] Rhi::ResourceType mandatoryResourceType(const rapidjson::Value& rapidJsonValue)
		{
			const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue["ResourceType"];
			const char* valueAsString = rapidJsonValueUsage.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();
			Rhi::ResourceType resourceType = Rhi::ResourceType::ROOT_SIGNATURE;

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) resourceType = Rhi::ResourceType::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) resourceType = Rhi::ResourceType::name;

			// Evaluate value
			IF_VALUE(ROOT_SIGNATURE)
			ELSE_IF_VALUE(RESOURCE_GROUP)
			ELSE_IF_VALUE(GRAPHICS_PROGRAM)
			ELSE_IF_VALUE(VERTEX_ARRAY)
			ELSE_IF_VALUE(RENDER_PASS)
			ELSE_IF_VALUE(QUERY_POOL)
			ELSE_IF_VALUE(SWAP_CHAIN)
			ELSE_IF_VALUE(FRAMEBUFFER)
			ELSE_IF_VALUE(INDEX_BUFFER)
			ELSE_IF_VALUE(VERTEX_BUFFER)
			ELSE_IF_VALUE(TEXTURE_BUFFER)
			ELSE_IF_VALUE(STRUCTURED_BUFFER)
			ELSE_IF_VALUE(INDIRECT_BUFFER)
			ELSE_IF_VALUE(UNIFORM_BUFFER)
			ELSE_IF_VALUE(TEXTURE_1D)
			ELSE_IF_VALUE(TEXTURE_1D_ARRAY)
			ELSE_IF_VALUE(TEXTURE_2D)
			ELSE_IF_VALUE(TEXTURE_2D_ARRAY)
			ELSE_IF_VALUE(TEXTURE_3D)
			ELSE_IF_VALUE(TEXTURE_CUBE)
			ELSE_IF_VALUE(GRAPHICS_PIPELINE_STATE)
			ELSE_IF_VALUE(COMPUTE_PIPELINE_STATE)
			ELSE_IF_VALUE(SAMPLER_STATE)
			ELSE_IF_VALUE(VERTEX_SHADER)
			ELSE_IF_VALUE(TESSELLATION_CONTROL_SHADER)
			ELSE_IF_VALUE(TESSELLATION_EVALUATION_SHADER)
			ELSE_IF_VALUE(GEOMETRY_SHADER)
			ELSE_IF_VALUE(FRAGMENT_SHADER)
			ELSE_IF_VALUE(COMPUTE_SHADER)
			else
			{
				throw std::runtime_error("Invalid resource type \"" + std::string(valueAsString) + '\"');
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE

			// Done
			return resourceType;
		}

		void optionalDescriptorRangeType(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::DescriptorRangeType& value)
		{
			if (rapidJsonValue.HasMember(propertyName))
			{
				const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue[propertyName];
				const char* valueAsString = rapidJsonValueUsage.GetString();
				const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

				// Define helper macros
				#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::DescriptorRangeType::name;
				#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::DescriptorRangeType::name;

				// Evaluate value
				IF_VALUE(SRV)
				ELSE_IF_VALUE(UAV)
				ELSE_IF_VALUE(UBV)
				ELSE_IF_VALUE(SAMPLER)
				else
				{
					throw std::runtime_error("Descriptor range type of property \"" + std::string(propertyName) + "\" must be \"SRV\", \"UAV\", \"UBV\" or \"SAMPLER\", but \"" + std::string(valueAsString) + "\" set");
				}

				// Undefine helper macros
				#undef IF_VALUE
				#undef ELSE_IF_VALUE
			}
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
	void JsonMaterialBlueprintHelper::loadDocumentByFilename(const IAssetCompiler::Input& input, const std::string& virtualFilename, rapidjson::Document& rapidJsonDocument)
	{
		// Parse JSON
		rapidjson::Document derivedRapidJsonDocument;
		const RendererRuntime::IFileManager& fileManager = input.context.getFileManager();
		JsonHelper::loadDocumentByFilename(fileManager, virtualFilename, "MaterialBlueprintAsset", "2", derivedRapidJsonDocument);

		// Handle optional base material blueprint
		// -> Named toolkit time base material blueprint and not parent material blueprint by intent to not intermix it with the dynamic runtime parent material blueprint
		// TODO(co) Recursive base material blueprint support would be nice
		rapidjson::Document baseRapidJsonDocument;
		const rapidjson::Value& derivedRapidJsonValueMaterialBlueprintAsset = derivedRapidJsonDocument["MaterialBlueprintAsset"];
		if (derivedRapidJsonValueMaterialBlueprintAsset.HasMember("BaseMaterialBlueprint"))
		{
			// Read material blueprint asset compiler configuration
			// -> ".asset"-check for automatically in-memory generated ".asset"-file support
			std::string materialBlueprintInputFile;
			const std::string virtualMaterialBlueprintAssetFilename = StringHelper::getSourceAssetFilenameByString(derivedRapidJsonValueMaterialBlueprintAsset["BaseMaterialBlueprint"].GetString(), input);
			if (virtualMaterialBlueprintAssetFilename.find(".asset") != std::string::npos)
			{
				// Explicit ".asset"-file: Parse material blueprint asset JSON
				rapidjson::Document rapidJsonDocumentMaterialBlueprintAsset;
				JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualMaterialBlueprintAssetFilename, "Asset", "1", rapidJsonDocumentMaterialBlueprintAsset);
				materialBlueprintInputFile = JsonHelper::getAssetInputFileByRapidJsonDocument(rapidJsonDocumentMaterialBlueprintAsset);
			}
			else
			{
				// Automatically in-memory generated ".asset"-file
				materialBlueprintInputFile = std_filesystem::path(virtualMaterialBlueprintAssetFilename).filename().generic_string();
			}

			// Load material blueprint
			const std::string virtualMaterialBlueprintDirectory = std_filesystem::path(virtualMaterialBlueprintAssetFilename).parent_path().generic_string();
			const std::string virtualMaterialBlueprintFilename = virtualMaterialBlueprintDirectory + '/' + materialBlueprintInputFile;
			JsonHelper::loadDocumentByFilename(fileManager, virtualMaterialBlueprintFilename, "MaterialBlueprintAsset", "2", baseRapidJsonDocument);
			JsonHelper::mergeObjects(baseRapidJsonDocument, derivedRapidJsonDocument, baseRapidJsonDocument);
			rapidJsonDocument.Swap(baseRapidJsonDocument);
		}
		else
		{
			rapidJsonDocument.Swap(derivedRapidJsonDocument);
		}
	}

	void JsonMaterialBlueprintHelper::optionalPrimitiveTopology(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::PrimitiveTopology& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueUsage.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::PrimitiveTopology::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::PrimitiveTopology::name;

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

	Rhi::PrimitiveTopologyType JsonMaterialBlueprintHelper::getPrimitiveTopologyTypeByPrimitiveTopology(Rhi::PrimitiveTopology primitiveTopology)
	{
		switch (primitiveTopology)
		{
			default:
			case Rhi::PrimitiveTopology::UNKNOWN:
				return Rhi::PrimitiveTopologyType::UNDEFINED;

			case Rhi::PrimitiveTopology::POINT_LIST:
				return Rhi::PrimitiveTopologyType::POINT;

			case Rhi::PrimitiveTopology::LINE_LIST:
			case Rhi::PrimitiveTopology::LINE_STRIP:
				return Rhi::PrimitiveTopologyType::LINE;

			case Rhi::PrimitiveTopology::TRIANGLE_LIST:
			case Rhi::PrimitiveTopology::TRIANGLE_STRIP:
				return Rhi::PrimitiveTopologyType::TRIANGLE;

			case Rhi::PrimitiveTopology::PATCH_LIST_1:
			case Rhi::PrimitiveTopology::PATCH_LIST_2:
			case Rhi::PrimitiveTopology::PATCH_LIST_3:
			case Rhi::PrimitiveTopology::PATCH_LIST_4:
			case Rhi::PrimitiveTopology::PATCH_LIST_5:
			case Rhi::PrimitiveTopology::PATCH_LIST_6:
			case Rhi::PrimitiveTopology::PATCH_LIST_7:
			case Rhi::PrimitiveTopology::PATCH_LIST_8:
			case Rhi::PrimitiveTopology::PATCH_LIST_9:
			case Rhi::PrimitiveTopology::PATCH_LIST_10:
			case Rhi::PrimitiveTopology::PATCH_LIST_11:
			case Rhi::PrimitiveTopology::PATCH_LIST_12:
			case Rhi::PrimitiveTopology::PATCH_LIST_13:
			case Rhi::PrimitiveTopology::PATCH_LIST_14:
			case Rhi::PrimitiveTopology::PATCH_LIST_15:
			case Rhi::PrimitiveTopology::PATCH_LIST_16:
			case Rhi::PrimitiveTopology::PATCH_LIST_17:
			case Rhi::PrimitiveTopology::PATCH_LIST_18:
			case Rhi::PrimitiveTopology::PATCH_LIST_19:
			case Rhi::PrimitiveTopology::PATCH_LIST_20:
			case Rhi::PrimitiveTopology::PATCH_LIST_21:
			case Rhi::PrimitiveTopology::PATCH_LIST_22:
			case Rhi::PrimitiveTopology::PATCH_LIST_23:
			case Rhi::PrimitiveTopology::PATCH_LIST_24:
			case Rhi::PrimitiveTopology::PATCH_LIST_25:
			case Rhi::PrimitiveTopology::PATCH_LIST_26:
			case Rhi::PrimitiveTopology::PATCH_LIST_27:
			case Rhi::PrimitiveTopology::PATCH_LIST_28:
			case Rhi::PrimitiveTopology::PATCH_LIST_29:
			case Rhi::PrimitiveTopology::PATCH_LIST_30:
			case Rhi::PrimitiveTopology::PATCH_LIST_31:
			case Rhi::PrimitiveTopology::PATCH_LIST_32:
				return Rhi::PrimitiveTopologyType::PATCH;
		}
	}

	void JsonMaterialBlueprintHelper::optionalShaderVisibilityProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::ShaderVisibility& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueUsage.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::ShaderVisibility::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Rhi::ShaderVisibility::name;

			// Evaluate value
			IF_VALUE(ALL)
			ELSE_IF_VALUE(VERTEX)
			ELSE_IF_VALUE(TESSELLATION_CONTROL)
			ELSE_IF_VALUE(TESSELLATION_EVALUATION)
			ELSE_IF_VALUE(GEOMETRY)
			ELSE_IF_VALUE(FRAGMENT)
			ELSE_IF_VALUE(COMPUTE)
			ELSE_IF_VALUE(ALL_GRAPHICS)
			else
			{
				throw std::runtime_error("Shader visibility of property \"" + std::string(propertyName) + "\" must be \"ALL\", \"VERTEX\", \"TESSELLATION_CONTROL\", \"TESSELLATION_EVALUATION\", \"GEOMETRY\", \"FRAGMENT\", \"COMPUTE\" or \"ALL_GRAPHICS\", but \"" + std::string(valueAsString) + "\" set");
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
		// -> ".asset"-check for automatically in-memory generated ".asset"-file support
		std::string materialBlueprintInputFile;
		const std::string& virtualMaterialBlueprintAssetFilename = input.sourceAssetIdToVirtualAssetFilename(materialBlueprintAssetId);
		if (virtualMaterialBlueprintAssetFilename.find(".asset") != std::string::npos)
		{
			// Explicit ".asset"-file: Parse material blueprint asset JSON
			rapidjson::Document rapidJsonDocumentMaterialBlueprintAsset;
			JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualMaterialBlueprintAssetFilename, "Asset", "1", rapidJsonDocumentMaterialBlueprintAsset);
			materialBlueprintInputFile = JsonHelper::getAssetInputFileByRapidJsonDocument(rapidJsonDocumentMaterialBlueprintAsset);
		}
		else
		{
			// Automatically in-memory generated ".asset"-file
			materialBlueprintInputFile = std_filesystem::path(virtualMaterialBlueprintAssetFilename).filename().generic_string();
		}

		// Parse material blueprint JSON with modified asset compiler input so relative texture asset IDs can be resolved correctly
		const std::string virtualMaterialBlueprintDirectory = std_filesystem::path(virtualMaterialBlueprintAssetFilename).parent_path().generic_string();
		const std::string virtualMaterialBlueprintFilename = virtualMaterialBlueprintDirectory + '/' + materialBlueprintInputFile;
		rapidjson::Document rapidJsonDocument;
		loadDocumentByFilename(input, virtualMaterialBlueprintFilename, rapidJsonDocument);
		RendererRuntime::ShaderProperties visualImportanceOfShaderProperties;
		RendererRuntime::ShaderProperties maximumIntegerValueOfShaderProperties;
		const IAssetCompiler::Input materialBlueprintAssetInput(input.context, input.projectName, input.cacheManager, input.virtualAssetPackageInputDirectory, virtualMaterialBlueprintFilename, virtualMaterialBlueprintDirectory, input.virtualAssetOutputDirectory, input.sourceAssetIdToCompiledAssetId, input.compiledAssetIdToSourceAssetId, input.sourceAssetIdToVirtualFilename, input.defaultTextureAssetIds);
		readProperties(materialBlueprintAssetInput, rapidJsonDocument["MaterialBlueprintAsset"]["Properties"], sortedMaterialPropertyVector, visualImportanceOfShaderProperties, maximumIntegerValueOfShaderProperties, true, true, false, materialPropertyIdToName);
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
				Rhi::FillMode value = Rhi::FillMode::SOLID;
				JsonMaterialHelper::optionalFillModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromFillMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::CULL_MODE:
			{
				Rhi::CullMode value = Rhi::CullMode::BACK;
				JsonMaterialHelper::optionalCullModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromCullMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::CONSERVATIVE_RASTERIZATION_MODE:
			{
				Rhi::ConservativeRasterizationMode value = Rhi::ConservativeRasterizationMode::OFF;
				JsonMaterialHelper::optionalConservativeRasterizationModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromConservativeRasterizationMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::DEPTH_WRITE_MASK:
			{
				Rhi::DepthWriteMask value = Rhi::DepthWriteMask::ALL;
				JsonMaterialHelper::optionalDepthWriteMaskProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromDepthWriteMask(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::STENCIL_OP:
			{
				Rhi::StencilOp value = Rhi::StencilOp::KEEP;
				JsonMaterialHelper::optionalStencilOpProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromStencilOp(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::COMPARISON_FUNC:
			{
				Rhi::ComparisonFunc value = Rhi::ComparisonFunc::GREATER;	// "Rhi::ComparisonFunc::GREATER" instead of "Rhi::ComparisonFunc::LESS" due to usage of Reversed-Z (see e.g. https://developer.nvidia.com/content/depth-precision-visualized and https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/)
				JsonMaterialHelper::optionalComparisonFuncProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromComparisonFunc(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::BLEND:
			{
				Rhi::Blend value = Rhi::Blend::ONE;
				JsonMaterialHelper::optionalBlendProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromBlend(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::BLEND_OP:
			{
				Rhi::BlendOp value = Rhi::BlendOp::ADD;
				JsonMaterialHelper::optionalBlendOpProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromBlendOp(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::FILTER_MODE:
			{
				Rhi::FilterMode value = Rhi::FilterMode::MIN_MAG_MIP_LINEAR;
				JsonMaterialHelper::optionalFilterProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromFilterMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ADDRESS_MODE:
			{
				Rhi::TextureAddressMode value = Rhi::TextureAddressMode::CLAMP;
				JsonMaterialHelper::optionalTextureAddressModeProperty(rapidJsonValue, propertyName, value);
				return RendererRuntime::MaterialPropertyValue::fromTextureAddressMode(value);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID:
			{
				RendererRuntime::AssetId textureAssetId = RendererRuntime::getInvalid<RendererRuntime::AssetId>();
				if (rapidJsonValue.HasMember(propertyName))
				{
					// Usage of asset IDs is the preferred way to go, but we also need to support the asset ID naming scheme
					// "<project name>/<asset directory>/<asset name>" to be able to reference e.g. runtime generated assets
					textureAssetId = StringHelper::getAssetIdByString(rapidJsonValue[propertyName].GetString(), input);
				}
				if (RendererRuntime::isInvalid(textureAssetId))
				{
					throw std::runtime_error("Inside material blueprints, texture asset reference material properties must always have a value");
				}

				// Done
				return RendererRuntime::MaterialPropertyValue::fromTextureAssetId(textureAssetId);
			}

			case RendererRuntime::MaterialPropertyValue::ValueType::GLOBAL_MATERIAL_PROPERTY_ID:
			{
				RendererRuntime::MaterialPropertyId materialPropertyId = RendererRuntime::getInvalid<RendererRuntime::MaterialPropertyId>();
				if (rapidJsonValue.HasMember(propertyName))
				{
					// Get the reference value as string
					static constexpr uint32_t NAME_LENGTH = 127 + 1;	// +1 for the terminating zero
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
				if (RendererRuntime::isInvalid(materialPropertyId))
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

	void JsonMaterialBlueprintHelper::readRootSignatureByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file, bool isComputeMaterialBlueprint)
	{
		// First: Collect everything we need instead of directly writing it down using an inefficient data layout
		std::vector<Rhi::RootParameterData> rootParameters;
		std::vector<Rhi::DescriptorRange> descriptorRanges;
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
						Rhi::DescriptorRange descriptorRange;

						{ // Mandatory resource and range type
							// Resource type
							descriptorRange.resourceType = ::detail::mandatoryResourceType(rapidJsonValue);

							{ // Get descriptor range type default value basing on the resource type
								// Define helper macro
								#define CASE_VALUE(name, rangeTypeValue) case Rhi::ResourceType::name: descriptorRange.rangeType = Rhi::DescriptorRangeType::rangeTypeValue; break;
								#define CASE(name) case Rhi::ResourceType::name:

								// Evaluate value
								switch (descriptorRange.resourceType)
								{
									CASE_VALUE(TEXTURE_BUFFER,	  SRV)
									CASE_VALUE(STRUCTURED_BUFFER, SRV)
									CASE_VALUE(UNIFORM_BUFFER,	  UBV)
									CASE_VALUE(TEXTURE_1D,		  SRV)
									CASE_VALUE(TEXTURE_1D_ARRAY,  SRV)
									CASE_VALUE(TEXTURE_2D,		  SRV)
									CASE_VALUE(TEXTURE_2D_ARRAY,  SRV)
									CASE_VALUE(TEXTURE_3D,		  SRV)
									CASE_VALUE(TEXTURE_CUBE,	  SRV)
									CASE_VALUE(SAMPLER_STATE,	  SAMPLER)
									CASE(ROOT_SIGNATURE)
									CASE(RESOURCE_GROUP)
									CASE(GRAPHICS_PROGRAM)
									CASE(VERTEX_ARRAY)
									CASE(RENDER_PASS)
									CASE(QUERY_POOL)
									CASE(SWAP_CHAIN)
									CASE(FRAMEBUFFER)
									CASE(INDEX_BUFFER)
									CASE(VERTEX_BUFFER)
									CASE(INDIRECT_BUFFER)
									CASE(GRAPHICS_PIPELINE_STATE)
									CASE(COMPUTE_PIPELINE_STATE)
									CASE(VERTEX_SHADER)
									CASE(TESSELLATION_CONTROL_SHADER)
									CASE(TESSELLATION_EVALUATION_SHADER)
									CASE(GEOMETRY_SHADER)
									CASE(FRAGMENT_SHADER)
									CASE(COMPUTE_SHADER)
										throw std::runtime_error("Invalid resource type \"" + std::string(rapidJsonValue["ResourceType"].GetString()) + "\", must be \"TEXTURE_BUFFER\", \"STRUCTURED_BUFFER\", \"UNIFORM_BUFFER\", \"TEXTURE_1D\", \"TEXTURE_1D_ARRAY\", \"TEXTURE_2D\", \"TEXTURE_2D_ARRAY\", \"TEXTURE_3D\", \"TEXTURE_CUBE\" or \"SAMPLER_STATE\"");
								}

								// Undefine helper macro
								#undef CASE_VALUE
							}

							// Descriptor range type and sanity check
							::detail::optionalDescriptorRangeType(rapidJsonValue, "DescriptorRangeType", descriptorRange.rangeType);
							switch (descriptorRange.rangeType)
							{
								case Rhi::DescriptorRangeType::SRV:
									if (Rhi::ResourceType::TEXTURE_BUFFER != descriptorRange.resourceType &&
										Rhi::ResourceType::STRUCTURED_BUFFER != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_1D != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_1D_ARRAY != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_2D != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_2D_ARRAY != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_3D != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_CUBE != descriptorRange.resourceType)
									{
										throw std::runtime_error("Descriptor range type \"SRV\" is only possible for the resource type \"TEXTURE_BUFFER\", \"STRUCTURED_BUFFER\", \"TEXTURE_1D\", \"TEXTURE_1D_ARRAY\", \"TEXTURE_2D\", \"TEXTURE_2D_ARRAY\", \"TEXTURE_3D\" and \"TEXTURE_CUBE\"");
									}
									break;

								case Rhi::DescriptorRangeType::UAV:
									if (Rhi::ResourceType::TEXTURE_BUFFER != descriptorRange.resourceType &&
										Rhi::ResourceType::STRUCTURED_BUFFER != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_1D != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_1D_ARRAY != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_2D != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_2D_ARRAY != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_3D != descriptorRange.resourceType &&
										Rhi::ResourceType::TEXTURE_CUBE != descriptorRange.resourceType)
									{
										throw std::runtime_error("Descriptor range type \"UAV\" is only possible for the resource type \"TEXTURE_BUFFER\", \"STRUCTURED_BUFFER\", \"TEXTURE_1D\", \"TEXTURE_1D_ARRAY\", \"TEXTURE_2D\", \"TEXTURE_2D_ARRAY\", \"TEXTURE_3D\" and \"TEXTURE_CUBE\"");
									}
									break;

								case Rhi::DescriptorRangeType::UBV:
									if (Rhi::ResourceType::UNIFORM_BUFFER != descriptorRange.resourceType)
									{
										throw std::runtime_error("Descriptor range type \"UBV\" is only possible for the resource type \"UNIFORM_BUFFER\"");
									}
									break;

								case Rhi::DescriptorRangeType::SAMPLER:
									if (Rhi::ResourceType::SAMPLER_STATE != descriptorRange.resourceType)
									{
										throw std::runtime_error("Descriptor range type \"SAMPLER\" is only possible for the resource type \"SAMPLER_STATE\"");
									}
									break;

								case Rhi::DescriptorRangeType::NUMBER_OF_RANGE_TYPES:
									// Impossible to end up in here
									break;
							}
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
						JsonHelper::optionalStringProperty(rapidJsonValue, "BaseShaderRegisterName", descriptorRange.baseShaderRegisterName, Rhi::DescriptorRange::NAME_LENGTH);

						// Optional shader visibility
						descriptorRange.shaderVisibility = isComputeMaterialBlueprint ? Rhi::ShaderVisibility::COMPUTE : Rhi::ShaderVisibility::ALL;
						optionalShaderVisibilityProperty(rapidJsonValue, "ShaderVisibility", descriptorRange.shaderVisibility);
						if (isComputeMaterialBlueprint && Rhi::ShaderVisibility::COMPUTE != descriptorRange.shaderVisibility)
						{
							// Remember, the renderer toolkit isn't error tolerant at all by intent, so don't soften this
							throw std::runtime_error("For compute material blueprints, only compute shader visibility is valid");
						}

						// Add the descriptor range
						descriptorRanges.push_back(descriptorRange);
					}

					// Advance resource index
					++resourceIndex;
				}

				{ // Add the root parameter
					Rhi::RootParameterData rootParameter;
					rootParameter.parameterType = Rhi::RootParameterType::DESCRIPTOR_TABLE;
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
			BaseShaderRegisterUsed rangeTypeBaseShaderRegisterUsed[static_cast<uint32_t>(Rhi::DescriptorRangeType::NUMBER_OF_RANGE_TYPES)];
			for (const Rhi::DescriptorRange& descriptorRange : descriptorRanges)
			{
				BaseShaderRegisterUsed& baseShaderRegisterUsed = rangeTypeBaseShaderRegisterUsed[static_cast<uint32_t>(descriptorRange.rangeType)];
				if (baseShaderRegisterUsed.find(descriptorRange.baseShaderRegister) == baseShaderRegisterUsed.cend())
				{
					baseShaderRegisterUsed.insert(descriptorRange.baseShaderRegister);
				}
				else
				{
					throw std::runtime_error("Base shader register " + std::to_string(descriptorRange.baseShaderRegister) + " is already used. Please note that to be RHI implementation independent, base shader register usage is considered to be across all shader stages like OpenGL does.");
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
			rootSignatureHeader.numberOfStaticSamplers	 = 0;								// TODO(co) Add support for static samplers
			rootSignatureHeader.flags					 = Rhi::RootSignatureFlags::NONE;	// TODO(co) Add support for flags
			file.write(&rootSignatureHeader, sizeof(RendererRuntime::v1MaterialBlueprint::RootSignatureHeader));
		}

		// Write down the rest
		if (!rootParameters.empty())
		{
			// Write down the root parameters
			file.write(rootParameters.data(), sizeof(Rhi::RootParameterData) * rootParameters.size());

			// Write down the descriptor ranges
			file.write(descriptorRanges.data(), sizeof(Rhi::DescriptorRange) * descriptorRanges.size());
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
					static constexpr uint32_t NAME_LENGTH = 127 + 1;	// +1 for the terminating zero
					char referenceAsString[NAME_LENGTH] = {};
					JsonHelper::optionalStringProperty(rapidJsonValueProperty, "Value", referenceAsString, NAME_LENGTH);

					// The character "@" is used to reference e.g. a material property value
					if (referenceAsString[0] == '@')
					{
						// Sanity check
						// -> "GlobalComputeSize" is a fixed build in material property with known specialized processing during runtime, hence do always allow references in this special case
						if (!referencesAllowed && RendererRuntime::MaterialResource::GLOBAL_COMPUTE_SIZE_PROPERTY_ID != materialPropertyId)
						{
							throw std::runtime_error("Material property \"" + std::string(rapidJsonMemberIterator->name.GetString()) + "\" with value \"" + std::string(referenceAsString) + "\" is using \"@\" to reference e.g. a material property value, but references aren't allowed in the current use-case");
						}

						// Write down the material property
						const RendererRuntime::StringId referenceAsInteger(&referenceAsString[1]);
						sortedMaterialPropertyVector.emplace_back(materialPropertyId, usage, RendererRuntime::MaterialProperty::materialPropertyValueFromReference(valueType, referenceAsInteger));
					}
					else
					{
						// Write down the material property
						sortedMaterialPropertyVector.emplace_back(materialPropertyId, usage, mandatoryMaterialPropertyValue(input, rapidJsonValueProperty, "Value", valueType));
					}
				}
				else
				{
					// Write down the material property
					sortedMaterialPropertyVector.emplace_back(materialPropertyId, usage, mandatoryMaterialPropertyValue(input, rapidJsonValueProperty, "Value", valueType));
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
					ASSERT(hasMaximumIntegerValue);
					if (hasMaximumIntegerValue)
					{
						const int maximumIntegerValue = std::atoi(rapidJsonValueProperty["MaximumIntegerValue"].GetString());
						ASSERT(maximumIntegerValue > 0);
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

	void JsonMaterialBlueprintHelper::readComputePipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueComputePipelineState, RendererRuntime::IFile& file, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
	{
		// Sanity check
		{ // "LocalComputeSize"-property
			/*
			Static use case example:
				"LocalComputeSize":
				{
					"Usage": "STATIC",
					"ValueType": "INTEGER_3",
					"Value": "32 32 1",
					"Description": "Fixed build in material property for the compute shader local size (also known as number of threads). Must be identical to in-shader values."
				}
			*/
			RendererRuntime::MaterialProperties::SortedPropertyVector::const_iterator iterator = std::lower_bound(sortedMaterialPropertyVector.cbegin(), sortedMaterialPropertyVector.cend(), RendererRuntime::MaterialResource::LOCAL_COMPUTE_SIZE_PROPERTY_ID, RendererRuntime::detail::OrderByMaterialPropertyId());
			if (iterator == sortedMaterialPropertyVector.end() || iterator->getMaterialPropertyId() != RendererRuntime::MaterialResource::LOCAL_COMPUTE_SIZE_PROPERTY_ID)
			{
				throw std::runtime_error("Compute material blueprints need the fixed build in material property \"LocalComputeSize\" for the compute shader local size (also known as number of threads)");
			}
			if (iterator->getValueType() != RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_3)
			{
				throw std::runtime_error("Compute material blueprint fixed build in material property \"LocalComputeSize\" for the compute shader local size (also known as number of threads) value type must be \"INTEGER_3\"");
			}
			if (iterator->getUsage() != RendererRuntime::MaterialProperty::Usage::STATIC)
			{
				throw std::runtime_error("Compute material blueprint fixed build in material property \"LocalComputeSize\" for the compute shader local size (also known as number of threads) usage must be \"STATIC\"");
			}
			const int* integer3Value = iterator->getInteger3Value();
			if (integer3Value[0] <= 0 || integer3Value[1] <= 0 || integer3Value[2] <= 0)
			{
				throw std::runtime_error("Compute material blueprint fixed build in material property \"LocalComputeSize\" for the compute shader local size (also known as number of threads) must be greater or equal to one");
			}
		}
		{ // "GlobalComputeSize"-property
			/*
			Static use case example:
				"GlobalComputeSize":
				{
					"Usage": "STATIC",
					"ValueType": "INTEGER_3",
					"Value": "1920 1080 1",
					"Description": "Fixed build in material property for the compute shader global size
				},

			Dynamic use case example:
				"GlobalComputeSize":
				{
					"Usage": "MATERIAL_REFERENCE",
					"ValueType": "INTEGER_3",
					"Value": "@OutputTexture2D",
					"Description": "Fixed build in material property for the compute shader global size"
				},
				"OutputTexture2D":
				{
					"Usage": "TEXTURE_REFERENCE",
					"ValueType": "TEXTURE_ASSET_ID",
					"Value": "Unrimp/Texture/DynamicByCode/BlackMap2D",
					"Description": "Output texture 2D"
				}
			*/
			RendererRuntime::MaterialProperties::SortedPropertyVector::const_iterator iterator = std::lower_bound(sortedMaterialPropertyVector.cbegin(), sortedMaterialPropertyVector.cend(), RendererRuntime::MaterialResource::GLOBAL_COMPUTE_SIZE_PROPERTY_ID, RendererRuntime::detail::OrderByMaterialPropertyId());
			if (iterator == sortedMaterialPropertyVector.end() || iterator->getMaterialPropertyId() != RendererRuntime::MaterialResource::GLOBAL_COMPUTE_SIZE_PROPERTY_ID)
			{
				throw std::runtime_error("Compute material blueprints need the fixed build in material property \"GlobalComputeSize\" for the compute shader global size");
			}
			if (iterator->getValueType() != RendererRuntime::MaterialPropertyValue::ValueType::INTEGER_3)
			{
				throw std::runtime_error("Compute material blueprint fixed build in material property \"GlobalComputeSize\" for the compute shader global size value type must be \"INTEGER_3\"");
			}
			if (iterator->getUsage() != RendererRuntime::MaterialProperty::Usage::STATIC && iterator->getUsage() != RendererRuntime::MaterialProperty::Usage::MATERIAL_REFERENCE)
			{
				throw std::runtime_error("Compute material blueprint fixed build in material property \"GlobalComputeSize\" for the compute shader global size usage must be \"STATIC\" or \"MATERIAL_REFERENCE\"");
			}
			if (iterator->getUsage() == RendererRuntime::MaterialProperty::Usage::STATIC)
			{
				// Static value
				const int* integer3Value = iterator->getInteger3Value();
				if (integer3Value[0] <= 0 || integer3Value[1] <= 0 || integer3Value[2] <= 0)
				{
					throw std::runtime_error("Compute material blueprint fixed build in material property \"GlobalComputeSize\" for the compute shader global size must be greater or equal to one");
				}
			}
			else
			{
				// Material property reference
				const RendererRuntime::MaterialPropertyId materialPropertyId = iterator->getReferenceValue();
				RendererRuntime::MaterialProperties::SortedPropertyVector::const_iterator referenceIterator = std::lower_bound(sortedMaterialPropertyVector.cbegin(), sortedMaterialPropertyVector.cend(), materialPropertyId, RendererRuntime::detail::OrderByMaterialPropertyId());
				if (referenceIterator == sortedMaterialPropertyVector.end() || referenceIterator->getMaterialPropertyId() != materialPropertyId)
				{
					throw std::runtime_error("Compute material blueprint fixed build in material property \"GlobalComputeSize\" is referencing an unknown material property");
				}
				if (referenceIterator->getValueType() != RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID)
				{
					throw std::runtime_error("Compute material blueprint fixed build in material property \"GlobalComputeSize\" can only reference texture asset material properties with value type \"TEXTURE_ASSET_ID\"");
				}
				if (referenceIterator->getUsage() != RendererRuntime::MaterialProperty::Usage::TEXTURE_REFERENCE)
				{
					throw std::runtime_error("Compute material blueprint fixed build in material property \"GlobalComputeSize\" can only reference texture asset material properties with usage type \"TEXTURE_REFERENCE\"");
				}
				// No need to check the referenced material property value since this such checks are done when parsing the material properties
			}
		}

		// Read compute pipeline state object
		RendererRuntime::AssetId computeShaderBlueprintAssetId = RendererRuntime::getInvalid<RendererRuntime::AssetId>();
		JsonHelper::optionalCompiledAssetId(input, rapidJsonValueComputePipelineState, "ComputeShaderBlueprint", computeShaderBlueprintAssetId);
		file.write(&computeShaderBlueprintAssetId, sizeof(RendererRuntime::AssetId));
	}

	void JsonMaterialBlueprintHelper::readGraphicsPipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueGraphicsPipelineState, RendererRuntime::IFile& file, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
	{
		{ // No compute shader blueprint, this way the loaded knows there's a graphics pipeline state
			const RendererRuntime::AssetId computeShaderBlueprintAssetId = RendererRuntime::getInvalid<RendererRuntime::AssetId>();
			file.write(&computeShaderBlueprintAssetId, sizeof(RendererRuntime::AssetId));
		}

		{ // Vertex attributes asset ID
			const RendererRuntime::AssetId vertexAttributesAssetId = StringHelper::getAssetIdByString(rapidJsonValueGraphicsPipelineState["VertexAttributes"].GetString(), input);
			file.write(&vertexAttributesAssetId, sizeof(RendererRuntime::AssetId));
		}

		{ // Shader blueprints
			const rapidjson::Value& rapidJsonValueShaderBlueprints = rapidJsonValueGraphicsPipelineState["ShaderBlueprints"];

			RendererRuntime::AssetId shaderBlueprintAssetId[RendererRuntime::NUMBER_OF_GRAPHICS_SHADER_TYPES];
			memset(shaderBlueprintAssetId, static_cast<int>(RendererRuntime::getInvalid<RendererRuntime::AssetId>()), sizeof(RendererRuntime::AssetId) * RendererRuntime::NUMBER_OF_GRAPHICS_SHADER_TYPES);
			shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::GraphicsShaderType::Vertex)] = JsonHelper::getCompiledAssetId(input, rapidJsonValueShaderBlueprints, "VertexShaderBlueprint");
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "TessellationControlShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::GraphicsShaderType::TessellationControl)]);
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "TessellationEvaluationShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::GraphicsShaderType::TessellationEvaluation)]);
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "GeometryShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::GraphicsShaderType::Geometry)]);
			JsonHelper::optionalCompiledAssetId(input, rapidJsonValueShaderBlueprints, "FragmentShaderBlueprint", shaderBlueprintAssetId[static_cast<uint8_t>(RendererRuntime::GraphicsShaderType::Fragment)]);

			// Write down the shader blueprints
			file.write(&shaderBlueprintAssetId, sizeof(RendererRuntime::AssetId) * RendererRuntime::NUMBER_OF_GRAPHICS_SHADER_TYPES);
		}

		// Start with the default settings
		Rhi::GraphicsPipelineState graphicsPipelineState = Rhi::GraphicsPipelineStateBuilder();

		// Optional primitive topology
		optionalPrimitiveTopology(rapidJsonValueGraphicsPipelineState, "PrimitiveTopology", graphicsPipelineState.primitiveTopology);
		graphicsPipelineState.primitiveTopologyType = getPrimitiveTopologyTypeByPrimitiveTopology(graphicsPipelineState.primitiveTopology);

		// Optional rasterizer state
		if (rapidJsonValueGraphicsPipelineState.HasMember("RasterizerState"))
		{
			const rapidjson::Value& rapidJsonValueRasterizerState = rapidJsonValueGraphicsPipelineState["RasterizerState"];
			Rhi::RasterizerState& rasterizerState = graphicsPipelineState.rasterizerState;

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
		if (rapidJsonValueGraphicsPipelineState.HasMember("DepthStencilState"))
		{
			const rapidjson::Value& rapidJsonValueDepthStencilState = rapidJsonValueGraphicsPipelineState["DepthStencilState"];
			Rhi::DepthStencilState& depthStencilState = graphicsPipelineState.depthStencilState;

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
		if (rapidJsonValueGraphicsPipelineState.HasMember("BlendState"))
		{
			const rapidjson::Value& rapidJsonValueBlendState = rapidJsonValueGraphicsPipelineState["BlendState"];
			Rhi::BlendState& blendState = graphicsPipelineState.blendState;

			// The optional properties
			JsonHelper::optionalBooleanProperty(rapidJsonValueBlendState, "AlphaToCoverageEnable", blendState.alphaToCoverageEnable, RendererRuntime::MaterialProperty::Usage::BLEND_STATE, &sortedMaterialPropertyVector);
			JsonHelper::optionalBooleanProperty(rapidJsonValueBlendState, "IndependentBlendEnable", blendState.independentBlendEnable);

			// The optional render target properties
			for (int i = 0; i < 8; ++i)
			{
				const std::string renderTarget = "RenderTarget[" + std::to_string(i) + ']';
				if (rapidJsonValueBlendState.HasMember(renderTarget))
				{
					const rapidjson::Value& rapidJsonValueRenderTarget = rapidJsonValueBlendState[renderTarget];
					Rhi::RenderTargetBlendDesc& renderTargetBlendDesc = blendState.renderTarget[i];

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

		// Write down the graphics pipeline state object (PSO)
		file.write(&graphicsPipelineState, sizeof(Rhi::SerializedGraphicsPipelineState));
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
						static constexpr uint32_t NAME_LENGTH = 127 + 1;	// +1 for the terminating zero
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
					Rhi::SamplerState& samplerState = materialBlueprintSamplerState.samplerState;
					samplerState = Rhi::ISamplerState::getDefaultSamplerState();

					{ // Mandatory base shader register name
						char baseShaderRegisterName[Rhi::DescriptorRange::NAME_LENGTH] = {};
						JsonHelper::mandatoryStringProperty(rapidJsonValue, "BaseShaderRegisterName", baseShaderRegisterName, Rhi::DescriptorRange::NAME_LENGTH);
						const uint32_t key = RendererRuntime::StringId::calculateFNV(baseShaderRegisterName);
						if (samplerBaseShaderRegisterNameToIndex.find(key) != samplerBaseShaderRegisterNameToIndex.cend())
						{
							throw std::runtime_error("Sampler state base shader register name \"" + std::string(baseShaderRegisterName) + "\" is defined multiple times");
						}
						samplerBaseShaderRegisterNameToIndex.emplace(key, samplerStateIndex);
						++samplerStateIndex;
					}

					// By default, inside the material blueprint system the texture filter and maximum anisotropy are set to invalid. Unless explicitly
					// set by a material blueprint author, those values are dynamic during runtime so the user can decide about the performance/quality trade-off.
					samplerState.filter = Rhi::FilterMode::UNKNOWN;
					RendererRuntime::setInvalid(samplerState.maxAnisotropy);

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
		// - "ResourceType" = "TEXTURE_1D", "TEXTURE_1D_ARRAY", "TEXTURE_2D", "TEXTURE_2D_ARRAY", "TEXTURE_3D", "TEXTURE_CUBE"
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
				const char* resourceTypeAsString = rapidJsonValue["ResourceType"].GetString();
				if (strcmp(resourceTypeAsString, "TEXTURE_1D") == 0 ||
					strcmp(resourceTypeAsString, "TEXTURE_1D_ARRAY") == 0 ||
					strcmp(resourceTypeAsString, "TEXTURE_2D") == 0 ||
					strcmp(resourceTypeAsString, "TEXTURE_2D_ARRAY") == 0 ||
					strcmp(resourceTypeAsString, "TEXTURE_3D") == 0 ||
					strcmp(resourceTypeAsString, "TEXTURE_CUBE") == 0)
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
					uint32_t samplerStateIndex = RendererRuntime::getInvalid<uint32_t>();
					if (rapidJsonValue.HasMember("SamplerStateBaseShaderRegisterName"))
					{
						char baseShaderRegisterName[Rhi::DescriptorRange::NAME_LENGTH] = {};
						JsonHelper::mandatoryStringProperty(rapidJsonValue, "SamplerStateBaseShaderRegisterName", baseShaderRegisterName, Rhi::DescriptorRange::NAME_LENGTH);
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
								const RendererRuntime::v1MaterialBlueprint::Texture materialBlueprintTexture(rootParameterIndex, RendererRuntime::MaterialProperty(RendererRuntime::getInvalid<RendererRuntime::MaterialPropertyId>(), usage, materialPropertyValue), fallbackTextureAssetId, rgbHardwareGammaCorrection, samplerStateIndex);
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

	void JsonMaterialBlueprintHelper::getDependencyFiles(const IAssetCompiler::Input& input, const std::string& virtualInputFilename, std::vector<std::string>& virtualDependencyFilenames)
	{
		// Parse JSON
		rapidjson::Document rapidJsonDocument;
		JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualInputFilename, "MaterialBlueprintAsset", "2", rapidJsonDocument);

		// Optional base material blueprint
		// -> Named toolkit time base material blueprint and not parent material blueprint by intent to not intermix it with the dynamic runtime parent material blueprint
		const rapidjson::Value& rapidJsonValueMaterialBlueprintAsset = rapidJsonDocument["MaterialBlueprintAsset"];
		if (rapidJsonValueMaterialBlueprintAsset.HasMember("BaseMaterialBlueprint"))
		{
			// Get base material blueprint asset ID
			const rapidjson::Value& rapidJsonValueBaseMaterialBlueprint = rapidJsonValueMaterialBlueprintAsset["BaseMaterialBlueprint"];
			std::string baseMaterialBlueprintVirtualInputFilename;
			try
			{
				const RendererRuntime::AssetId materialBlueprintAssetId = StringHelper::getSourceAssetIdByString(rapidJsonValueBaseMaterialBlueprint.GetString(), input);
				baseMaterialBlueprintVirtualInputFilename = input.sourceAssetIdToVirtualAssetFilename(materialBlueprintAssetId);
				virtualDependencyFilenames.emplace_back(baseMaterialBlueprintVirtualInputFilename);
				StringHelper::replaceFirstString(baseMaterialBlueprintVirtualInputFilename, ".asset", ".material_blueprint");
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Failed to gather dependency files of material blueprint source asset \"" + virtualInputFilename + "\" due to unknown base material blueprint source asset \"" + std::string(rapidJsonValueBaseMaterialBlueprint.GetString()) + "\": " + std::string(e.what()));
			}

			// Go down the rabbit hole recursively
			try
			{
				const IAssetCompiler::Input materialBlueprintAssetInput(input.context, input.projectName, input.cacheManager, input.virtualAssetPackageInputDirectory, baseMaterialBlueprintVirtualInputFilename, std_filesystem::path(baseMaterialBlueprintVirtualInputFilename).parent_path().generic_string(), input.virtualAssetOutputDirectory, input.sourceAssetIdToCompiledAssetId, input.compiledAssetIdToSourceAssetId, input.sourceAssetIdToVirtualFilename, input.defaultTextureAssetIds);
				getDependencyFiles(materialBlueprintAssetInput, baseMaterialBlueprintVirtualInputFilename, virtualDependencyFilenames);
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Failed to gather dependency files of base material blueprint source asset \"" + baseMaterialBlueprintVirtualInputFilename + "\": " + std::string(e.what()));
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
