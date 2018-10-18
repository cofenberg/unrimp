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
#include "RendererToolkit/Private/AssetCompiler/IAssetCompiler.h"

#include <RendererRuntime/Public/Resource/Material/MaterialProperties.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/fwd.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Global definitions                                    ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	typedef StringId AssetId;	///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class JsonHelper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static void loadDocumentByFilename(const RendererRuntime::IFileManager& fileManager, const std::string& virtualFilename, const std::string& formatType, const std::string& formatVersion, rapidjson::Document& rapidJsonDocument);
		static void saveDocumentByFilename(const RendererRuntime::IFileManager& fileManager, const std::string& virtualFilename, const std::string& formatType, const std::string& formatVersion, rapidjson::Value& rapidJsonValue);
		static void mergeObjects(rapidjson::Value& destinationObject, rapidjson::Value& sourceObject, rapidjson::Document& allocatorRapidJsonDocument);
		static std::string getAssetFile(const rapidjson::Value& rapidJsonValue);
		static std::string getAssetInputFileByRapidJsonValue(const rapidjson::Value& rapidJsonValue, const std::string_view& valueName = "InputFile");
		static std::string getAssetInputFileByRapidJsonDocument(const rapidjson::Document& rapidJsonDocument);
		static const RendererRuntime::MaterialProperty* getMaterialPropertyOfUsageAndValueType(const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector, const std::string& valueAsString, RendererRuntime::MaterialProperty::Usage usage, RendererRuntime::MaterialPropertyValue::ValueType valueType);
		static void optionalBooleanProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, bool& value, RendererRuntime::MaterialProperty::Usage usage = RendererRuntime::MaterialProperty::Usage::UNKNOWN, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalBooleanProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, int& value, RendererRuntime::MaterialProperty::Usage usage = RendererRuntime::MaterialProperty::Usage::UNKNOWN, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector = nullptr);
		static void optionalByteProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, uint8_t& value);
		static void optionalIntegerProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, int& value);
		static void optionalIntegerProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, unsigned int& value);
		static void optionalIntegerNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, int value[], uint32_t numberOfComponents);
		static void optionalFloatProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float& value);
		static void optionalFloatNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[], uint32_t numberOfComponents);
		static void optionalUnitNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[], uint32_t numberOfComponents);
		static void optionalUnitNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, double value[], uint32_t numberOfComponents);
		static void optionalFactorNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[], uint32_t numberOfComponents);
		static void optionalRgbColorProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[3]);
		static void optionalAngleProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float& value);	// Angle in radians
		static void optionalRotationQuaternionProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, glm::quat& value);
		static void optionalTimeOfDayProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float& value);
		static void mandatoryStringProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, char* value, uint32_t maximumLength);
		static void optionalStringProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, char* value, uint32_t maximumLength);
		static void optionalStringNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, std::string value[], uint32_t numberOfComponents, char separator = ' ');
		static void mandatoryStringIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::StringId& value);
		static void optionalStringIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::StringId& value);
		static void mandatoryAssetIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::AssetId& value);
		static void optionalAssetIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::AssetId& value);
		static void optionalClearFlagsProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, uint32_t& clearFlags);
		static void optionalCompiledAssetId(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::AssetId& compiledAssetId);
		static RendererRuntime::AssetId getCompiledAssetId(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName);
		static Renderer::TextureFormat::Enum mandatoryTextureFormat(const rapidjson::Value& rapidJsonValue);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		JsonHelper(const JsonHelper&) = delete;
		JsonHelper& operator=(const JsonHelper&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
