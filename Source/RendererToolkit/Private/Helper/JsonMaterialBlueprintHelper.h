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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererToolkit/Private/AssetCompiler/IAssetCompiler.h"

#include <Renderer/Public/Resource/Material/MaterialProperties.h>

#include <unordered_map>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IFile;
	class ShaderProperties;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef std::unordered_map<uint32_t, std::string> MaterialPropertyIdToName;				///< Key = "Renderer::MaterialPropertyId"
	typedef std::unordered_map<uint32_t, uint32_t> SamplerBaseShaderRegisterNameToIndex;	///< Key = "Renderer::StringId(<Base Shader Register Name>)", value = index of the material blueprint sampler state resource to use


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class JsonMaterialBlueprintHelper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static void loadDocumentByFilename(const IAssetCompiler::Input& input, const std::string& virtualFilename, rapidjson::Document& rapidJsonDocument);
		static void optionalPrimitiveTopology(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::PrimitiveTopology& value);
		[[nodiscard]] static Rhi::PrimitiveTopologyType getPrimitiveTopologyTypeByPrimitiveTopology(Rhi::PrimitiveTopology primitiveTopology);
		static void optionalShaderVisibilityProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Rhi::ShaderVisibility& value);
		[[nodiscard]] static Renderer::MaterialProperty::Usage mandatoryMaterialPropertyUsage(const rapidjson::Value& rapidJsonValue);
		[[nodiscard]] static Renderer::MaterialProperty::ValueType mandatoryMaterialPropertyValueType(const rapidjson::Value& rapidJsonValue);
		static void getPropertiesByMaterialBlueprintAssetId(const IAssetCompiler::Input& input, Renderer::AssetId materialBlueprintAssetId, Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, MaterialPropertyIdToName* materialPropertyIdToName = nullptr);
		[[nodiscard]] static Renderer::MaterialPropertyValue mandatoryMaterialPropertyValue(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName, const Renderer::MaterialProperty::ValueType valueType);
		static void readRootSignatureByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, Renderer::IFile& file, bool isComputeMaterialBlueprint);
		static void readProperties(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueProperties, Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, Renderer::ShaderProperties& visualImportanceOfShaderProperties, Renderer::ShaderProperties& maximumIntegerValueOfShaderProperties, bool ignoreGlobalReferenceFallback, bool sort, bool referencesAllowed, MaterialPropertyIdToName* materialPropertyIdToName = nullptr);
		static void readComputePipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueComputePipelineState, Renderer::IFile& file, const Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector);
		static void readGraphicsPipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueGraphicsPipelineState, Renderer::IFile& file, const Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector);
		static void readUniformBuffersByResourceGroups(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueResourceGroups, Renderer::IFile& file);
		static void readTextureBuffersByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, Renderer::IFile& file);
		static void readSamplerStatesByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, const Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, Renderer::IFile& file, SamplerBaseShaderRegisterNameToIndex& samplerBaseShaderRegisterNameToIndex);
		static void readTexturesByResourceGroups(const IAssetCompiler::Input& input, const Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, const rapidjson::Value& rapidJsonValueResourceGroups, const SamplerBaseShaderRegisterNameToIndex& samplerBaseShaderRegisterNameToIndex, Renderer::IFile& file);
		static void getDependencyFiles(const IAssetCompiler::Input& input, const std::string& virtualInputFilename, std::vector<std::string>& virtualDependencyFilenames);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		JsonMaterialBlueprintHelper(const JsonMaterialBlueprintHelper&) = delete;
		JsonMaterialBlueprintHelper& operator=(const JsonMaterialBlueprintHelper&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
