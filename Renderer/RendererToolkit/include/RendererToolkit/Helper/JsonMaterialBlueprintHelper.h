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
#include "RendererToolkit/AssetCompiler/IAssetCompiler.h"

#include <RendererRuntime/Resource/Material/MaterialProperties.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <rapidjson/fwd.h>
PRAGMA_WARNING_POP

#include <unordered_map>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
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
	typedef std::unordered_map<uint32_t, std::string> MaterialPropertyIdToName;				///< Key = "RendererRuntime::MaterialPropertyId"
	typedef std::unordered_map<uint32_t, uint32_t> SamplerBaseShaderRegisterNameToIndex;	///< Key = "RendererRuntime::StringId(<Base Shader Register Name>)", value = index of the material blueprint sampler state resource to use


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class JsonMaterialBlueprintHelper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static void optionalPrimitiveTopology(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::PrimitiveTopology& value);
		static Renderer::PrimitiveTopologyType getPrimitiveTopologyTypeByPrimitiveTopology(Renderer::PrimitiveTopology primitiveTopology);
		static void optionalShaderVisibilityProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::ShaderVisibility& value);
		static RendererRuntime::MaterialProperty::Usage mandatoryMaterialPropertyUsage(const rapidjson::Value& rapidJsonValue);
		static RendererRuntime::MaterialProperty::ValueType mandatoryMaterialPropertyValueType(const rapidjson::Value& rapidJsonValue);
		static void getPropertiesByMaterialBlueprintAssetId(const IAssetCompiler::Input& input, RendererRuntime::AssetId materialBlueprintAssetId, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, MaterialPropertyIdToName* materialPropertyIdToName = nullptr);
		static RendererRuntime::MaterialPropertyValue mandatoryMaterialPropertyValue(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName, const RendererRuntime::MaterialProperty::ValueType valueType);
		static void readRootSignatureByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file);
		static void readProperties(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueProperties, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, RendererRuntime::ShaderProperties& visualImportanceOfShaderProperties, RendererRuntime::ShaderProperties& maximumIntegerValueOfShaderProperties, bool ignoreGlobalReferenceFallback, bool sort, bool referencesAllowed, MaterialPropertyIdToName* materialPropertyIdToName = nullptr);
		static void readComputePipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueComputePipelineState, RendererRuntime::IFile& file);
		static void readGraphicsPipelineStateObject(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueGraphicsPipelineState, RendererRuntime::IFile& file, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector);
		static void readUniformBuffersByResourceGroups(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file);
		static void readTextureBuffersByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::IFile& file);
		static void readSamplerStatesByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, RendererRuntime::IFile& file, SamplerBaseShaderRegisterNameToIndex& samplerBaseShaderRegisterNameToIndex);
		static void readTexturesByResourceGroups(const IAssetCompiler::Input& input, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, const rapidjson::Value& rapidJsonValueResourceGroups, const SamplerBaseShaderRegisterNameToIndex& samplerBaseShaderRegisterNameToIndex, RendererRuntime::IFile& file);


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
