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
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"

#include <RendererRuntime/Public/Core/File/IFile.h>
#include <RendererRuntime/Public/Core/File/IFileManager.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'rapidjson::GenericReader<rapidjson::UTF8<char>,rapidjson::UTF8<char>,rapidjson::CrtAllocator>::IterativeParsingStartState' in switch of enum 'rapidjson::GenericReader<rapidjson::UTF8<char>,rapidjson::UTF8<char>,rapidjson::CrtAllocator>::IterativeParsingState' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4619)	// warning C4619: #pragma warning: there is no warning number '4351'
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	#include <rapidjson/document.h>
	#include <rapidjson/error/en.h>
	#include <rapidjson/prettywriter.h>
	#include <rapidjson/ostreamwrapper.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtc/quaternion.hpp>
	#include <glm/gtc/epsilon.hpp>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::_Rand_urng_from_func::result_type', signed/unsigned mismatch
	#include <sstream>
	#include <algorithm> // For "std::count()"
PRAGMA_WARNING_POP


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
		/**
		*  @brief
		*    Return a RGB color by using a given Kelvin value
		*
		*  @param[in] kelvin
		*    Kelvin value to return as RGB color
		*
		*  @return
		*    RGB color from the given Kelvin value
		*
		*  @note
		*    - Basing on "How to Convert Temperature (K) to RGB: Algorithm and Sample Code" ( http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/ )
		*    - See also "Moving Frostbite to Physically Based Rendering" from DICE, starting page 35 ( http://www.frostbite.com/wp-content/uploads/2014/11/s2014_pbs_frostbite_slides.pdf )
		*/
		glm::vec3 getRgbColorFromKelvin(float kelvin)
		{
			if (kelvin > 40000.0f)
			{
				kelvin = 40000.0f;
			}
			else if (kelvin < 1000.0f)
			{
				kelvin = 1000.0f;
			}
			kelvin = kelvin / 100.0f;

			// Red
			float red = 0.0f;
			if (kelvin <= 66.0f)
			{
				red = 255.0f;
			}
			else
			{
				float temporary = kelvin - 60.0f;
				temporary = 329.698727446f * std::pow(temporary, -0.1332047592f);
				red = temporary;
				if (red < 0.0f)
				{
					red = 0.0f;
				}
				else if (red > 255.0f)
				{
					red = 255.0f;
				}
			}

			// Green
			float green = 0.0f;
			if (kelvin <= 66.0f)
			{
				float temporary = kelvin;
				temporary = 99.4708025861f * std::log(temporary) - 161.1195681661f;
				green = temporary;
				if (green < 0.0f)
				{
					green = 0.0f;
				}
				else if (green > 255.0f)
				{
					green = 255.0f;
				}
			}
			else
			{
				float temporary = kelvin - 60.0f;
				temporary = 288.1221695283f * std::pow(temporary, -0.0755148492f);
				green = temporary;
				if (green < 0.0f)
				{
					green = 0.0f;
				}
				else if (green > 255.0f)
				{
					green = 255.0f;
				}
			}

			// Blue
			float blue = 0.0f;
			if (kelvin >= 66.0f)
			{
				blue = 255.0f;
			}
			else if (kelvin <= 19.0f)
			{
				blue = 0.0f;
			}
			else
			{
				float temporary = kelvin - 10.0f;
				temporary = 138.5177312231f * std::log(temporary) - 305.0447927307f;
				blue = temporary;
				if (blue < 0.0f)
				{
					blue = 0.0f;
				}
				else if (blue > 255.0f)
				{
					blue = 255.0f;
				}
			}

			// Done
			return glm::vec3(std::pow(red / 255.0f, 2.2f), std::pow(green / 255.0f, 2.2f), std::pow(blue / 255.0f, 2.2f));
		}

		// Implementation from https://gist.github.com/fairlight1337/4935ae72bcbcc1ba5c72 - with Unrimp code style adoptions

		// Copyright (c) 2014, Jan Winkler <winkler@cs.uni-bremen.de>
		// All rights reserved.
		//
		// Redistribution and use in source and binary forms, with or without
		// modification, are permitted provided that the following conditions are met:
		//
		//     * Redistributions of source code must retain the above copyright
		//       notice, this list of conditions and the following disclaimer.
		//     * Redistributions in binary form must reproduce the above copyright
		//       notice, this list of conditions and the following disclaimer in the
		//       documentation and/or other materials provided with the distribution.
		//     * Neither the name of Universität Bremen nor the names of its
		//       contributors may be used to endorse or promote products derived from
		//       this software without specific prior written permission.
		//
		// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
		// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
		// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
		// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
		// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
		// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
		// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
		// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
		// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
		// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
		// POSSIBILITY OF SUCH DAMAGE.

		/* Author: Jan Winkler */
		/*! \brief Convert HSV to RGB color space

		  Converts a given set of HSV values `h', `s', `v' into RGB
		  coordinates. The output RGB values are in the range [0, 1], and
		  the input HSV values are in the ranges h = [0, 360], and s, v =
		  [0, 1], respectively.

		  \param h Hue component, used as input, range: [0, 360]
		  \param s Hue component, used as input, range: [0, 1]
		  \param v Hue component, used as input, range: [0, 1]
		  \param r Red component, used as output, range: [0, 1]
		  \param g Green component, used as output, range: [0, 1]
		  \param b Blue component, used as output, range: [0, 1]

		*/
		void hsvToRgb(float h, float s, float v, float& r, float& g, float& b)
		{
			const float c = v * s;	// Chroma
			const float hPrime = static_cast<float>(fmod(h / 60.0, 6));
			const float x = static_cast<float>(c * (1 - fabs(fmod(hPrime, 2) - 1)));
			const float m = v - c;

			if (0 <= hPrime && hPrime < 1)
			{
				r = c;
				g = x;
				b = 0;
			}
			else if (1 <= hPrime && hPrime < 2)
			{
				r = x;
				g = c;
				b = 0;
			}
			else if (2 <= hPrime && hPrime < 3)
			{
				r = 0;
				g = c;
				b = x;
			}
			else if (3 <= hPrime && hPrime < 4)
			{
				r = 0;
				g = x;
				b = c;
			}
			else if (4 <= hPrime && hPrime < 5)
			{
				r = x;
				g = 0;
				b = c;
			}
			else if (5 <= hPrime && hPrime < 6)
			{
				r = c;
				g = 0;
				b = x;
			}
			else
			{
				r = 0;
				g = 0;
				b = 0;
			}

			r += m;
			g += m;
			b += m;
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
	void JsonHelper::loadDocumentByFilename(const RendererRuntime::IFileManager& fileManager, const std::string& virtualFilename, const std::string& formatType, const std::string& formatVersion, rapidjson::Document& rapidJsonDocument)
	{
		// Load the whole file content as string
		std::string fileContentAsString;
		StringHelper::readStringByFilename(fileManager, virtualFilename, fileContentAsString);

		// Load the JSON document
		const rapidjson::ParseResult rapidJsonParseResult = rapidJsonDocument.Parse(fileContentAsString.c_str());
		if (rapidJsonParseResult.Code() != rapidjson::kParseErrorNone)
		{
			// Get the line number
			const std::streamoff lineNumber = std::count(fileContentAsString.begin(), fileContentAsString.begin() + static_cast<std::streamoff>(rapidJsonParseResult.Offset()), '\n');

			// Throw exception with human readable error message
			throw std::runtime_error("Failed to parse JSON file \"" + virtualFilename + "\": " + rapidjson::GetParseError_En(rapidJsonParseResult.Code()) + " (line " + std::to_string(lineNumber) + ')');
		}

		{ // Mandatory format header: Check whether or not the file format matches
			const rapidjson::Value& rapidJsonValueFormat = rapidJsonDocument["Format"];
			{ // Type
				const char* typeString = rapidJsonValueFormat["Type"].GetString();
				if (formatType != typeString)
				{
					throw std::runtime_error("Invalid JSON format type \"" + std::string(typeString) + "\", must be \"" + formatType + "\"");
				}
			}
			{ // Version
				const char* versionString = rapidJsonValueFormat["Version"].GetString();
				if (formatVersion != versionString)
				{
					throw std::runtime_error("Invalid JSON format version" + std::string(versionString) + ", must be " + formatVersion);
				}
			}
		}
	}

	void JsonHelper::saveDocumentByFilename(const RendererRuntime::IFileManager& fileManager, const std::string& virtualFilename, const std::string& formatType, const std::string& formatVersion, rapidjson::Value& rapidJsonValue)
	{
		rapidjson::Document rapidJsonDocument(rapidjson::kObjectType);
		rapidjson::Document::AllocatorType& rapidJsonAllocatorType = rapidJsonDocument.GetAllocator();

		{ // Format
			rapidjson::Value rapidJsonValueFormat(rapidjson::kObjectType);
			rapidJsonValueFormat.AddMember("Type", rapidjson::StringRef(formatType.c_str()), rapidJsonAllocatorType);
			rapidJsonValueFormat.AddMember("Version", rapidjson::StringRef(formatVersion.c_str()), rapidJsonAllocatorType);
			rapidJsonDocument.AddMember("Format", rapidJsonValueFormat, rapidJsonAllocatorType);
		}

		// Add asset format type member
		rapidJsonDocument.AddMember(rapidjson::StringRef(formatType.c_str()), rapidJsonValue, rapidJsonAllocatorType);

		// JSON document to pretty string
		std::ostringstream outputStringStream;
		{
			rapidjson::OStreamWrapper outputStreamWrapper(outputStringStream);
			rapidjson::PrettyWriter<rapidjson::OStreamWrapper> rapidJsonWriter(outputStreamWrapper);
			rapidJsonDocument.Accept(rapidJsonWriter);
		}

		// Write down the texture asset JSON file
		RendererRuntime::IFile* file = fileManager.openFile(RendererRuntime::IFileManager::FileMode::WRITE, virtualFilename.c_str());
		if (nullptr != file)
		{
			const std::string jsonDocumentAsString = outputStringStream.str();
			file->write(jsonDocumentAsString.data(), jsonDocumentAsString.size());

			// Close file
			fileManager.closeFile(*file);
		}
		else
		{
			// Error!
			throw std::runtime_error("Failed to open the file \"" + virtualFilename + "\" for writing");
		}
	}

	void JsonHelper::mergeObjects(rapidjson::Value& destinationObject, rapidjson::Value& sourceObject, rapidjson::Document& allocatorRapidJsonDocument)
	{
		// The implementation is basing on https://stackoverflow.com/a/42491356
		assert(&destinationObject != &sourceObject);
		assert(destinationObject.IsObject() && sourceObject.IsObject());
		rapidjson::Document::AllocatorType& allocatorType = allocatorRapidJsonDocument.GetAllocator();
		for (rapidjson::Value::MemberIterator sourceIterator = sourceObject.MemberBegin(); sourceObject.MemberEnd() != sourceIterator; ++sourceIterator)
		{
			rapidjson::Value::MemberIterator destinationIterator = destinationObject.FindMember(sourceIterator->name);
			if (destinationObject.MemberEnd() != destinationIterator)
			{
				assert(destinationIterator->value.GetType() == sourceIterator->value.GetType());
				if (sourceIterator->value.IsArray())
				{
					for (rapidjson::Value::ValueIterator arrayIterator = sourceIterator->value.Begin(); sourceIterator->value.End() != arrayIterator; ++arrayIterator)
					{
						destinationIterator->value.PushBack(*arrayIterator, allocatorType);
					}
				}
				else if (sourceIterator->value.IsObject())
				{
					mergeObjects(destinationIterator->value, sourceIterator->value, allocatorRapidJsonDocument);
				}
				else
				{
					destinationIterator->value = rapidjson::Value(sourceIterator->value, allocatorType);
				}
			}
			else
			{
				// Deep copy
				destinationObject.AddMember(rapidjson::Value(sourceIterator->name, allocatorType), rapidjson::Value(sourceIterator->value, allocatorType), allocatorType);
			}
		}
	}

	std::string JsonHelper::getAssetFile(const rapidjson::Value& rapidJsonValue)
	{
		// Asset input file must start with "./" = this directory, no variations allowed in this case
		const std::string inputFile = rapidJsonValue.GetString();
		if (inputFile.length() >= 2 && inputFile.substr(0, 2) == "./")
		{
			return inputFile.substr(2);
		}

		// Error!
		throw std::runtime_error("Input files must start with \"./\" but \"" + inputFile + "\" given");
	}

	std::string JsonHelper::getAssetInputFile(const rapidjson::Value& rapidJsonValue, const std::string_view& valueName)
	{
		return getAssetFile(rapidJsonValue[valueName.data()]);
	}

	const RendererRuntime::MaterialProperty* JsonHelper::getMaterialPropertyOfUsageAndValueType(const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector, const std::string& valueAsString, RendererRuntime::MaterialProperty::Usage usage, RendererRuntime::MaterialPropertyValue::ValueType valueType)
	{
		// The character "@" is used to reference a material property value
		if (nullptr != sortedMaterialPropertyVector && !valueAsString.empty() && valueAsString[0] == '@')
		{
			// Reference a material property value
			const std::string materialPropertyName = valueAsString.substr(1);
			const RendererRuntime::MaterialPropertyId materialPropertyId(materialPropertyName.c_str());

			// Figure out the material property value
			RendererRuntime::MaterialProperties::SortedPropertyVector::const_iterator iterator = std::lower_bound(sortedMaterialPropertyVector->cbegin(), sortedMaterialPropertyVector->cend(), materialPropertyId, RendererRuntime::detail::OrderByMaterialPropertyId());
			if (iterator != sortedMaterialPropertyVector->end() && iterator->getMaterialPropertyId() == materialPropertyId)
			{
				const RendererRuntime::MaterialProperty& materialProperty = *iterator;
				if (materialProperty.getUsage() == usage && materialProperty.getValueType() == valueType)
				{
					return &materialProperty;
				}
				else
				{
					throw std::runtime_error("Material property \"" + materialPropertyName + "\" value type mismatch");
				}
			}
			else
			{
				throw std::runtime_error("Unknown material property name \"" + materialPropertyName + '\"');
			}
		}
		else
		{
			throw std::runtime_error("Invalid material property value reference \"" + valueAsString + "\", first character must be @ if you intended to reference a material property");
		}
	}

	void JsonHelper::optionalBooleanProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, bool& value, RendererRuntime::MaterialProperty::Usage usage, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const rapidjson::Value& rapidJsonValueValue = rapidJsonValue[propertyName];
			const char* valueAsString = rapidJsonValueValue.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValueValue.GetStringLength();

			if (strncmp(valueAsString, "FALSE", valueStringLength) == 0)
			{
				value = false;
			}
			else if (strncmp(valueAsString, "TRUE", valueStringLength) == 0)
			{
				value = true;
			}
			else
			{
				// Might be a material property reference, the called method automatically throws an exception if something looks odd
				const RendererRuntime::MaterialProperty* materialProperty = getMaterialPropertyOfUsageAndValueType(sortedMaterialPropertyVector, valueAsString, usage, RendererRuntime::MaterialPropertyValue::ValueType::BOOLEAN);
				if (nullptr != materialProperty)
				{
					value = materialProperty->getBooleanValue();
				}
			}
		}
	}

	void JsonHelper::optionalBooleanProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, int& value, RendererRuntime::MaterialProperty::Usage usage, const RendererRuntime::MaterialProperties::SortedPropertyVector* sortedMaterialPropertyVector)
	{
		bool booleanValue = (0 != value);
		optionalBooleanProperty(rapidJsonValue, propertyName, booleanValue, usage, sortedMaterialPropertyVector);
		value = booleanValue;
	}

	void JsonHelper::optionalByteProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, uint8_t& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			const int integerValue = std::atoi(rapidJsonValue[propertyName].GetString());
			if (integerValue < 0)
			{
				throw std::runtime_error(std::string("The value of property \"") + propertyName + "\" can't be negative");
			}
			if (integerValue > 255)
			{
				throw std::runtime_error(std::string("The value of property \"") + propertyName + "\" can't be above 255");
			}
			value = static_cast<uint8_t>(integerValue);
		}
	}

	void JsonHelper::optionalIntegerProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, int& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			value = std::atoi(rapidJsonValue[propertyName].GetString());
		}
	}

	void JsonHelper::optionalIntegerProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, unsigned int& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			value = static_cast<unsigned int>(std::atoi(rapidJsonValue[propertyName].GetString()));
		}
	}

	void JsonHelper::optionalIntegerNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, int value[], uint32_t numberOfComponents)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);
			if (elements.size() == numberOfComponents)
			{
				for (size_t i = 0; i < numberOfComponents; ++i)
				{
					value[i] = std::atoi(elements[i].c_str());
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" needs exactly " + std::to_string(numberOfComponents) + " components, but " + std::to_string(elements.size()) + " components given");
			}
		}
	}

	void JsonHelper::optionalFloatProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			value = std::stof(rapidJsonValue[propertyName].GetString());
		}
	}

	void JsonHelper::optionalFloatNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[], uint32_t numberOfComponents)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);
			if (elements.size() == numberOfComponents)
			{
				for (size_t i = 0; i < numberOfComponents; ++i)
				{
					value[i] = std::stof(elements[i].c_str());
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" needs exactly" + std::to_string(numberOfComponents) + " components, but " + std::to_string(elements.size()) + " components given");
			}
		}
	}

	void JsonHelper::optionalUnitNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[], uint32_t numberOfComponents)
	{
		// Unit values can be defined as "METER" (e.g. "0.42 METER")
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);
			if (elements.size() == numberOfComponents + 1)	// +1 for the value semantic
			{
				// Get the value semantic
				enum class ValueSemantic
				{
					METER,
					UNKNOWN
				};
				const std::string& valueSemanticAsString = elements[numberOfComponents];
				ValueSemantic valueSemantic = ValueSemantic::UNKNOWN;
				if (valueSemanticAsString == "METER")
				{
					valueSemantic = ValueSemantic::METER;
				}
				if (ValueSemantic::UNKNOWN == valueSemantic)
				{
					throw std::runtime_error('\"' + std::string(propertyName) + "\" is using unknown value semantic \"" + std::string(valueSemanticAsString) + '\"');
				}

				// Get component values
				for (size_t i = 0; i < numberOfComponents; ++i)
				{
					// One unit = one meter
					value[i] = std::stof(elements[i].c_str());
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" needs exactly" + std::to_string(numberOfComponents) + " components and a value semantic \"METER\", but " + std::to_string(elements.size()) + " string parts given");
			}
		}
	}

	void JsonHelper::optionalFactorNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[], uint32_t numberOfComponents)
	{
		// Factor values can be defined as "FACTOR" (e.g. "0.42 FACTOR") or "PERCENTAGE" (e.g. "42.0 PERCENTAGE")
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);
			if (elements.size() == numberOfComponents + 1)	// +1 for the value semantic
			{
				// Get the value semantic
				enum class ValueSemantic
				{
					FACTOR,
					PERCENTAGE,
					UNKNOWN
				};
				const std::string& valueSemanticAsString = elements[numberOfComponents];
				ValueSemantic valueSemantic = ValueSemantic::UNKNOWN;
				if (valueSemanticAsString == "FACTOR")
				{
					valueSemantic = ValueSemantic::FACTOR;
				}
				else if (valueSemanticAsString == "PERCENTAGE")
				{
					valueSemantic = ValueSemantic::PERCENTAGE;
				}
				if (ValueSemantic::UNKNOWN == valueSemantic)
				{
					throw std::runtime_error('\"' + std::string(propertyName) + "\" is using unknown value semantic \"" + std::string(valueSemanticAsString) + '\"');
				}

				// Get component values
				for (size_t i = 0; i < numberOfComponents; ++i)
				{
					value[i] = std::stof(elements[i].c_str());
					if (ValueSemantic::PERCENTAGE == valueSemantic)
					{
						value[i] *= 0.01f;
					}
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" needs exactly" + std::to_string(numberOfComponents) + " components and a value semantic \"FACTOR\" or \"PERCENTAGE\", but " + std::to_string(elements.size()) + " string parts given");
			}
		}
	}

	void JsonHelper::optionalRgbColorProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float value[3])
	{
		// RGB color values can be defined as
		// - "RGB" (e.g. "255 0 255 RGB")
		// - "RGB_FLOAT" (e.g. "1.0 0.0 1.0 RGB_FLOAT")
		// - "HSV" (e.g. "120.0 1 1 HSV")
		// - "HEX" (e.g. "FF00FF HEX")
		// - "INTENSITY" (e.g. "1.0 INTENSITY")
		// - "KELVIN" (e.g. "6600.0 KELVIN")
		// including a combination of color * intensity * kelvin
		if (rapidJsonValue.HasMember(propertyName))
		{
			// Split string
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);

			// The elements the final color will be composed of
			glm::vec3 color(1.0f, 1.0f, 1.0f);
			float intensity = 1.0f;
			float kelvin = 6600.0f;	// Results in white ("1.0 1.0 1.0")

			// Color
			for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
			{
				const std::string& element = elements[elementIndex];
				if ("RGB" == element)
				{
					if (elementIndex < 3)
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\": RGB colors need three color components");
					}
					for (uint8_t i = 0; i < 3; ++i)
					{
						const int integerValue = std::stoi(elements[elementIndex - (3 - i)].c_str());
						if (integerValue < 0 || integerValue > 255)
						{
							throw std::runtime_error("8-bit RGB color values must be between [0, 255]");
						}
						color[i] = static_cast<float>(integerValue) / 255.0f;
					}
					break;
				}
				else if ("RGB_FLOAT" == element)
				{
					if (elementIndex < 3)
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\": RGB colors need three color components");
					}
					for (uint8_t i = 0; i < 3; ++i)
					{
						color[i] = std::stof(elements[elementIndex - (3 - i)].c_str());
					}
					break;
				}
				else if ("HSV" == element)
				{
					if (elementIndex < 3)
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\": HSV colors need three color components");
					}
					for (uint8_t i = 0; i < 3; ++i)
					{
						color[i] = std::stof(elements[elementIndex - (3 - i)].c_str());
					}
					::detail::hsvToRgb(color[0], color[1], color[2], color[0], color[1], color[2]);
					break;
				}
				else if ("HEX" == element)
				{
					if (elementIndex < 1)
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\": HEX colors need one color component");
					}

					// TODO(co) Error handling
					unsigned int r = 0, g = 0, b = 0;
					sscanf(elements[elementIndex - 1].c_str(), "%02x%02x%02x", &r, &g, &b);
					color[0] = r / 255.0f;
					color[1] = g / 255.0f;
					color[2] = b / 255.0f;
					break;
				}
			}

			// Intensity
			for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
			{
				const std::string& element = elements[elementIndex];
				if ("INTENSITY" == element)
				{
					if (elementIndex < 1)
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\": Intensity needs one component");
					}
					intensity = std::stof(elements[elementIndex - 1].c_str());
					break;
				}
			}

			// Kelvin
			for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
			{
				const std::string& element = elements[elementIndex];
				if ("KELVIN" == element)
				{
					if (elementIndex < 1)
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\": Kelvin needs one component");
					}
					kelvin = std::stof(elements[elementIndex - 1].c_str());
					break;
				}
			}

			// Calculate final composed color
			color *= intensity;
			color *= ::detail::getRgbColorFromKelvin(kelvin);

			// Done
			value[0] = color.x;
			value[1] = color.y;
			value[2] = color.z;
		}
	}

	void JsonHelper::optionalAngleProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float& value)
	{
		// Angle values can be defined as Euler angle in "DEGREE" or Euler angle in "RADIAN"
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);
			if (elements.size() == 2)
			{
				value = std::stof(elements[0].c_str());
				if (elements[1] == "DEGREE")
				{
					value = glm::radians(value);
				}
				else if (elements[1] == "RADIAN")
				{
					// Nothing here
				}
				else
				{
					throw std::runtime_error('\"' + std::string(propertyName) + "\" must be x Euler angle in DEGREE/RADIAN");
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" must be x Euler angle in DEGREE/RADIAN");
			}
		}
	}

	void JsonHelper::optionalRotationQuaternionProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, glm::quat& value)
	{
		// Rotation quaternion values can be defined as "QUATERNION", Euler angles in "DEGREE" or Euler angles in "RADIAN"
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			const char* valueAsString = rapidJsonValue[propertyName].GetString();
			StringHelper::splitString(valueAsString, ' ', elements);
			if (elements.size() == 5 && elements[4] == "QUATERNION")
			{
				value.x = std::stof(elements[0].c_str());
				value.y = std::stof(elements[1].c_str());
				value.z = std::stof(elements[2].c_str());
				value.w = std::stof(elements[3].c_str());

				{ // Sanity check
					const float length = glm::length(value);
					if (!glm::epsilonEqual(length, 1.0f, 0.0000001f))
					{
						throw std::runtime_error("The rotation quaternion \"" + std::string(valueAsString) + "\" does not appear to be normalized (length is " + std::to_string(length) + ")");
					}
				}
			}
			else if (elements.size() == 4)
			{
				const float pitch = std::stof(elements[0].c_str());
				const float yaw   = std::stof(elements[1].c_str());
				const float roll  = std::stof(elements[2].c_str());
				if (elements[3] == "DEGREE")
				{
					value = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
				}
				else if (elements[3] == "RADIAN")
				{
					value = glm::quat(glm::vec3(pitch, yaw, roll));
				}
				else
				{
					throw std::runtime_error('\"' + std::string(propertyName) + "\" must be a x y z w QUATERNION, or x y z Euler angles in DEGREE/RADIAN");
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" must be a x y z w QUATERNION, or x y z Euler angles in DEGREE/RADIAN");
			}
		}
	}

	void JsonHelper::optionalTimeOfDayProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, float& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), ' ', elements);
			if (elements.size() == 2)
			{
				value = std::stof(elements[0].c_str());
				if (elements[1] == "O_CLOCK")
				{
					// Sanity check
					if (value < 00.00f || value >= 24.00f)	// O'clock
					{
						throw std::runtime_error("Time-of-day must be within 00.00>= and <24.00 o'clock");
					}
				}
				else
				{
					throw std::runtime_error('\"' + std::string(propertyName) + "\" must be x time-of-day in O_CLOCK");
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" must be x time-of-day in O_CLOCK");
			}
		}
	}

	void JsonHelper::mandatoryStringProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, char* value, uint32_t maximumLength)
	{
		if (0 == maximumLength)
		{
			throw std::runtime_error('\"' + std::string(propertyName) + "\" maximum number of characters must be greater than zero");
		}
		const rapidjson::Value& rapidJsonValueFound = rapidJsonValue[propertyName];
		const char* valueAsString = rapidJsonValueFound.GetString();
		const rapidjson::SizeType valueLength = rapidJsonValueFound.GetStringLength();

		// -1 for the terminating zero reserve
		maximumLength -= 1;
		if (valueLength <= maximumLength)
		{
			memcpy(value, valueAsString, valueLength);
			value[valueLength] = '\0';
		}
		else
		{
			throw std::runtime_error('\"' + std::string(propertyName) + "\" maximum number of characters is " + std::to_string(maximumLength) + ", but the value \"" + std::string(valueAsString) + "\" has " + std::to_string(valueLength) + " characters");
		}
	}

	void JsonHelper::optionalStringProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, char* value, uint32_t maximumLength)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			mandatoryStringProperty(rapidJsonValue, propertyName, value, maximumLength);
		}
	}

	void JsonHelper::optionalStringNProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, std::string value[], uint32_t numberOfComponents, char separator)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), separator, elements);
			if (elements.size() == numberOfComponents)
			{
				for (size_t i = 0; i < numberOfComponents; ++i)
				{
					value[i] = elements[i];
				}
			}
			else
			{
				throw std::runtime_error('\"' + std::string(propertyName) + "\" needs exactly" + std::to_string(numberOfComponents) + " components, but " + std::to_string(elements.size()) + " components given");
			}
		}
	}

	void JsonHelper::mandatoryStringIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::StringId& value)
	{
		value = RendererRuntime::StringId(rapidJsonValue[propertyName].GetString());
	}

	void JsonHelper::optionalStringIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::StringId& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			mandatoryStringIdProperty(rapidJsonValue, propertyName, value);
		}
	}

	void JsonHelper::mandatoryAssetIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::AssetId& value)
	{
		value = RendererRuntime::AssetId(rapidJsonValue[propertyName].GetString());
	}

	void JsonHelper::optionalAssetIdProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::AssetId& value)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			mandatoryAssetIdProperty(rapidJsonValue, propertyName, value);
		}
	}

	void JsonHelper::optionalClearFlagsProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, uint32_t& clearFlags)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			clearFlags = 0;
			std::vector<std::string> flagsAsString;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), '|', flagsAsString);
			if (!flagsAsString.empty())
			{
				// Define helper macros
				#define IF_VALUE(name)			 if (flagAsString == #name) value = Renderer::ClearFlag::name;
				#define ELSE_IF_VALUE(name) else if (flagAsString == #name) value = Renderer::ClearFlag::name;

				// Evaluate flags
				const size_t numberOfFlags = flagsAsString.size();
				for (size_t i = 0; i < numberOfFlags; ++i)
				{
					// Get flag as string
					std::string flagAsString = flagsAsString[i];
					StringHelper::trimWhitespaceCharacters(flagAsString);

					// Evaluate value
					uint32_t value = 0;
					IF_VALUE(COLOR)
					ELSE_IF_VALUE(DEPTH)
					ELSE_IF_VALUE(STENCIL)
					// ELSE_IF_VALUE(COLOR_DEPTH)	// Not added by intent, one has to write "COLOR | DEPTH"
					else
					{
						throw std::runtime_error('\"' + std::string(propertyName) + "\" doesn't know the flag " + flagAsString + ". Must be \"COLOR\", \"DEPTH\" or \"STENCIL\".");
					}

					// Apply value
					clearFlags |= value;
				}

				// Undefine helper macros
				#undef IF_VALUE
				#undef ELSE_IF_VALUE
			}
		}
	}

	void JsonHelper::optionalCompiledAssetId(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::AssetId& compiledAssetId)
	{
		if (rapidJsonValue.HasMember(propertyName))
		{
			compiledAssetId = StringHelper::getAssetIdByString(rapidJsonValue[propertyName].GetString(), input);
		}
	}

	RendererRuntime::AssetId JsonHelper::getCompiledAssetId(const IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValue, const char* propertyName)
	{
		return StringHelper::getAssetIdByString(rapidJsonValue[propertyName].GetString(), input);
	}

	Renderer::TextureFormat::Enum JsonHelper::mandatoryTextureFormat(const rapidjson::Value& rapidJsonValue)
	{
		const rapidjson::Value& rapidJsonValueUsage = rapidJsonValue["TextureFormat"];
		const char* valueAsString = rapidJsonValueUsage.GetString();
		const rapidjson::SizeType valueStringLength = rapidJsonValueUsage.GetStringLength();

		// Define helper macros
		#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) return Renderer::TextureFormat::name;
		#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) return Renderer::TextureFormat::name;

		// Evaluate value
		IF_VALUE(R8)
		ELSE_IF_VALUE(R8G8B8)
		ELSE_IF_VALUE(R8G8B8A8)
		ELSE_IF_VALUE(R8G8B8A8_SRGB)
		ELSE_IF_VALUE(B8G8R8A8)
		ELSE_IF_VALUE(R11G11B10F)
		ELSE_IF_VALUE(R16G16B16A16F)
		ELSE_IF_VALUE(R32G32B32A32F)
		ELSE_IF_VALUE(BC1)
		ELSE_IF_VALUE(BC1_SRGB)
		ELSE_IF_VALUE(BC2)
		ELSE_IF_VALUE(BC2_SRGB)
		ELSE_IF_VALUE(BC3)
		ELSE_IF_VALUE(BC3_SRGB)
		ELSE_IF_VALUE(BC4)
		ELSE_IF_VALUE(BC5)
		ELSE_IF_VALUE(ETC1)
		ELSE_IF_VALUE(R16_UNORM)
		ELSE_IF_VALUE(R32_UINT)
		ELSE_IF_VALUE(R32_FLOAT)
		ELSE_IF_VALUE(D32_FLOAT)
		ELSE_IF_VALUE(R16G16_SNORM)
		ELSE_IF_VALUE(R16G16_FLOAT)
		ELSE_IF_VALUE(UNKNOWN)
		else
		{
			throw std::runtime_error('\"' + std::string(valueAsString) + "\" is no known texture format");
		}

		// Undefine helper macros
		#undef IF_VALUE
		#undef ELSE_IF_VALUE
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
