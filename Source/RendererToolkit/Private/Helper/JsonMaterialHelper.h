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

#include <RendererRuntime/Public/Resource/Material/MaterialProperties.h>
#include <RendererRuntime/Public/Resource/Material/Loader/MaterialFileFormat.h>

#include <Renderer/Public/Renderer.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class JsonMaterialHelper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static void optionalFillModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::FillMode& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalCullModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::CullMode& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalConservativeRasterizationModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::ConservativeRasterizationMode& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalDepthWriteMaskProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::DepthWriteMask& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalStencilOpProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::StencilOp& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalBlendProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::Blend& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalBlendOpProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::BlendOp& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalFilterProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::FilterMode& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalTextureAddressModeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::TextureAddressMode& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalComparisonFuncProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::ComparisonFunc& value, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void readMaterialPropertyValues(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueProperties, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector);
		static void getTechniquesAndPropertiesByMaterialAssetId(const IAssetCompiler::Input& input, rapidjson::Document& rapidJsonDocument, std::vector<RendererRuntime::v1Material::Technique>& techniques, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector);
		static void getPropertiesByMaterialAssetId(const IAssetCompiler::Input& input, RendererRuntime::AssetId materialAssetId, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, std::vector<RendererRuntime::v1Material::Technique>* techniques = nullptr);
		static void getDependencyFiles(const IAssetCompiler::Input& input, const std::string& virtualInputFilename, std::vector<std::string>& virtualDependencyFilenames);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		JsonMaterialHelper(const JsonMaterialHelper&) = delete;
		JsonMaterialHelper& operator=(const JsonMaterialHelper&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
