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
#include "RendererToolkit/Private/AssetCompiler/MaterialBlueprintAssetCompiler.h"
#include "RendererToolkit/Private/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/Asset/AssetPackage.h>
#include <RendererRuntime/Public/Core/File/MemoryFile.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
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
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <rapidjson/document.h>
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
		void setMaterialBlueprintHeaderNumberOfResourcesByResourceType(const rapidjson::Value& rapidJsonValueResourceType, RendererRuntime::v1MaterialBlueprint::MaterialBlueprintHeader& materialBlueprintHeader)
		{
			const char* valueAsString = rapidJsonValueResourceType.GetString();

			// Define helper macros
			#define IF_VALUE(name, value)			if (strcmp(valueAsString, name) == 0) ++materialBlueprintHeader.value;
			#define ELSE_IF_VALUE(name, value) else if (strcmp(valueAsString, name) == 0) ++materialBlueprintHeader.value;

			// Evaluate value
				 IF_VALUE("UNIFORM_BUFFER",    numberOfUniformBuffers)
			ELSE_IF_VALUE("TEXTURE_BUFFER",	   numberOfTextureBuffers)
			ELSE_IF_VALUE("STRUCTURED_BUFFER", numberOfStructuredBuffers)
			ELSE_IF_VALUE("SAMPLER_STATE",	   numberOfSamplerStates)
			ELSE_IF_VALUE("TEXTURE_1D",		   numberOfTextures)
			ELSE_IF_VALUE("TEXTURE_1D_ARRAY",  numberOfTextures)
			ELSE_IF_VALUE("TEXTURE_2D",		   numberOfTextures)
			ELSE_IF_VALUE("TEXTURE_2D_ARRAY",  numberOfTextures)
			ELSE_IF_VALUE("TEXTURE_3D",		   numberOfTextures)
			ELSE_IF_VALUE("TEXTURE_CUBE",	   numberOfTextures)
			else
			{
				throw std::runtime_error("Invalid resource type \"" + std::string(valueAsString) + '\"');
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}

		void setMaterialBlueprintHeaderNumberOfResourcesByResourceGroups(const rapidjson::Value& rapidJsonValueResourceGroups, RendererRuntime::v1MaterialBlueprint::MaterialBlueprintHeader& materialBlueprintHeader)
		{
			// Initialize number of resources
			materialBlueprintHeader.numberOfUniformBuffers = materialBlueprintHeader.numberOfTextureBuffers = materialBlueprintHeader.numberOfStructuredBuffers = materialBlueprintHeader.numberOfSamplerStates = materialBlueprintHeader.numberOfTextures = 0;

			// Iterate through all resource groups, we're only interested in the "ResourceType" resource parameter
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

					// Check the resource type
					setMaterialBlueprintHeaderNumberOfResourcesByResourceType(rapidJsonMemberIteratorResource->value["ResourceType"], materialBlueprintHeader);

					// Advance resource index
					++resourceIndex;
				}

				// Advance resource group index
				++resourceGroupIndex;
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
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	std::string MaterialBlueprintAssetCompiler::getVirtualOutputAssetFilename(const Input& input, const Configuration&) const
	{
		return (input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string()).append(getOptionalUniqueAssetFilenameExtension());
	}

	bool MaterialBlueprintAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		std::vector<std::string> virtualDependencyFilenames;
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		JsonMaterialBlueprintHelper::getDependencyFiles(input, virtualInputFilename, virtualDependencyFilenames);
		return (input.cacheManager.checkIfFileIsModified(configuration.rhiTarget, input.virtualAssetFilename, {virtualInputFilename}, getVirtualOutputAssetFilename(input, configuration), RendererRuntime::v1MaterialBlueprint::FORMAT_VERSION) || input.cacheManager.dependencyFilesChanged(virtualDependencyFilenames));
	}

	void MaterialBlueprintAssetCompiler::compile(const Input& input, const Configuration& configuration) const
	{
		// Read configuration
		std::string inputFile;
		bool allowCrazyNumberOfShaderCombinations = false;
		{
			// Read material blueprint asset compiler configuration
			const rapidjson::Value& rapidJsonValueMaterialBlueprintAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
			inputFile = JsonHelper::getAssetInputFileByRapidJsonValue(rapidJsonValueMaterialBlueprintAssetCompiler);
			JsonHelper::optionalBooleanProperty(rapidJsonValueMaterialBlueprintAssetCompiler, "AllowCrazyNumberOfShaderCombinations", allowCrazyNumberOfShaderCombinations);
		}
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + inputFile;
		const std::string virtualOutputAssetFilename = getVirtualOutputAssetFilename(input, configuration);

		// Read in dependency files
		std::vector<std::string> virtualDependencyFilenames;
		JsonMaterialBlueprintHelper::getDependencyFiles(input, virtualInputFilename, virtualDependencyFilenames);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, RendererRuntime::v1MaterialBlueprint::FORMAT_VERSION, cacheEntries) || input.cacheManager.dependencyFilesChanged(virtualDependencyFilenames))
		{
			RendererRuntime::MemoryFile memoryFile(0, 4096);

			{ // Material blueprint
				// Parse JSON
				rapidjson::Document rapidJsonDocument;
				JsonMaterialBlueprintHelper::loadDocumentByFilename(input, virtualInputFilename, rapidJsonDocument);

				// Mandatory and optional main sections of the material blueprint
				// -> For ease-of-use the material blueprint is edited by the user in a resource-group-style containing all needed information
				// -> Internally, the material blueprint file content is split into the root signature, resources as well as resource groups
				static const rapidjson::Value EMPTY_VALUE;
				const rapidjson::Value& rapidJsonValueMaterialBlueprintAsset = rapidJsonDocument["MaterialBlueprintAsset"];
				const rapidjson::Value& rapidJsonValueProperties			 = rapidJsonValueMaterialBlueprintAsset.HasMember("Properties") ? rapidJsonValueMaterialBlueprintAsset["Properties"] : EMPTY_VALUE;
				const rapidjson::Value& rapidJsonValueResourceGroups		 = rapidJsonValueMaterialBlueprintAsset["ResourceGroups"];

				// Gather all material properties
				RendererRuntime::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVector;
				RendererRuntime::ShaderProperties visualImportanceOfShaderProperties;
				RendererRuntime::ShaderProperties maximumIntegerValueOfShaderProperties;
				const RendererRuntime::ShaderProperties::SortedPropertyVector& visualImportanceOfShaderPropertiesVector = visualImportanceOfShaderProperties.getSortedPropertyVector();
				const RendererRuntime::ShaderProperties::SortedPropertyVector& maximumIntegerValueOfShaderPropertiesVector = maximumIntegerValueOfShaderProperties.getSortedPropertyVector();
				if (rapidJsonValueProperties.IsObject())
				{
					JsonMaterialBlueprintHelper::readProperties(input, rapidJsonValueProperties, sortedMaterialPropertyVector, visualImportanceOfShaderProperties, maximumIntegerValueOfShaderProperties, false, true, false);

					// Child protection: Throw an exception if there are too many shader combination properties to protect the material blueprint designer of over-engineering material blueprints
					if (!allowCrazyNumberOfShaderCombinations)
					{
						uint32_t numberOfShaderCombinationProperties = 0;
						for (const RendererRuntime::MaterialProperty& materialProperty : sortedMaterialPropertyVector)
						{
							if (materialProperty.getUsage() == RendererRuntime::MaterialProperty::Usage::SHADER_COMBINATION)
							{
								++numberOfShaderCombinationProperties;
							}
						}
						static constexpr uint32_t MAXIMUM_NUMBER_OF_SHADER_COMBINATIONS = 6;	// This is no technical limit. See "RendererRuntime::MaterialBlueprintResource" class documentation regarding shader combination explosion for background information.
						if (numberOfShaderCombinationProperties > MAXIMUM_NUMBER_OF_SHADER_COMBINATIONS)
						{
							throw std::runtime_error("Material blueprint asset \"" + virtualInputFilename + "\" is using " + std::to_string(numberOfShaderCombinationProperties) + " shader combination material properties. In order to prevent an shader combination explosion, only " + std::to_string(MAXIMUM_NUMBER_OF_SHADER_COMBINATIONS) + " shader combination material properties are allowed. If you know what you're doing, the child protection can be disabled by using \"AllowCrazyNumberOfShaderCombinations\"=\"TRUE\" inside the material blueprint asset compiler configuration.");
						}
					}
				}

				{ // Write down the material blueprint header
					RendererRuntime::v1MaterialBlueprint::MaterialBlueprintHeader materialBlueprintHeader;
					materialBlueprintHeader.numberOfProperties							= rapidJsonValueProperties.MemberCount();
					materialBlueprintHeader.numberOfShaderCombinationProperties			= static_cast<uint32_t>(visualImportanceOfShaderPropertiesVector.size());
					materialBlueprintHeader.numberOfIntegerShaderCombinationProperties	= static_cast<uint32_t>(maximumIntegerValueOfShaderPropertiesVector.size());	// Each integer shader combination property must have a defined maximum value
					::detail::setMaterialBlueprintHeaderNumberOfResourcesByResourceGroups(rapidJsonValueResourceGroups, materialBlueprintHeader);
					memoryFile.write(&materialBlueprintHeader, sizeof(RendererRuntime::v1MaterialBlueprint::MaterialBlueprintHeader));
				}

				// Write down all material properties
				if (!sortedMaterialPropertyVector.empty())
				{
					memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
				}

				// Write down visual importance of shader properties
				if (!visualImportanceOfShaderPropertiesVector.empty())
				{
					memoryFile.write(visualImportanceOfShaderPropertiesVector.data(), sizeof(RendererRuntime::ShaderProperties::Property) * visualImportanceOfShaderPropertiesVector.size());
				}

				// Write down maximum integer value of shader properties
				if (!maximumIntegerValueOfShaderPropertiesVector.empty())
				{
					memoryFile.write(maximumIntegerValueOfShaderPropertiesVector.data(), sizeof(RendererRuntime::ShaderProperties::Property) * maximumIntegerValueOfShaderPropertiesVector.size());
				}

				// A material blueprint can have a compute or a graphics pipeline state, but never both at one and the same time
				if (rapidJsonValueMaterialBlueprintAsset.HasMember("ComputePipelineState"))
				{
					// Compute material blueprint

					// Root signature
					JsonMaterialBlueprintHelper::readRootSignatureByResourceGroups(rapidJsonValueResourceGroups, memoryFile, true);

					// Compute pipeline state object (PSO)
					JsonMaterialBlueprintHelper::readComputePipelineStateObject(input, rapidJsonValueMaterialBlueprintAsset["ComputePipelineState"], memoryFile, sortedMaterialPropertyVector);
				}
				else
				{
					// Graphics material blueprint

					// Root signature
					JsonMaterialBlueprintHelper::readRootSignatureByResourceGroups(rapidJsonValueResourceGroups, memoryFile, false);

					// Graphics pipeline state object (PSO)
					JsonMaterialBlueprintHelper::readGraphicsPipelineStateObject(input, rapidJsonValueMaterialBlueprintAsset["GraphicsPipelineState"], memoryFile, sortedMaterialPropertyVector);
				}

				{ // Resources
					// Uniform buffers
					JsonMaterialBlueprintHelper::readUniformBuffersByResourceGroups(input, rapidJsonValueResourceGroups, memoryFile);

					// Texture buffers
					JsonMaterialBlueprintHelper::readTextureBuffersByResourceGroups(rapidJsonValueResourceGroups, memoryFile);

					// Sampler states
					SamplerBaseShaderRegisterNameToIndex samplerBaseShaderRegisterNameToIndex;
					JsonMaterialBlueprintHelper::readSamplerStatesByResourceGroups(rapidJsonValueResourceGroups, sortedMaterialPropertyVector, memoryFile, samplerBaseShaderRegisterNameToIndex);

					// Textures
					JsonMaterialBlueprintHelper::readTexturesByResourceGroups(input, sortedMaterialPropertyVector, rapidJsonValueResourceGroups, samplerBaseShaderRegisterNameToIndex, memoryFile);
				}
			}

			// Write LZ4 compressed output
			if (!memoryFile.writeLz4CompressedDataByVirtualFilename(RendererRuntime::v1MaterialBlueprint::FORMAT_TYPE, RendererRuntime::v1MaterialBlueprint::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str()))
			{
				throw std::runtime_error("Failed to write LZ4 compressed output file \"" + virtualOutputAssetFilename + '\"');
			}

			// Store new cache entries or update existing ones
			input.cacheManager.storeOrUpdateCacheEntries(cacheEntries);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
