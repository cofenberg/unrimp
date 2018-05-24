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
#include "RendererToolkit/AssetCompiler/MaterialAssetCompiler.h"
#include "RendererToolkit/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Helper/JsonMaterialHelper.h"
#include "RendererToolkit/Helper/CacheManager.h"
#include "RendererToolkit/Helper/StringHelper.h"
#include "RendererToolkit/Helper/JsonHelper.h"
#include "RendererToolkit/Context.h"

#include <RendererRuntime/Asset/AssetPackage.h>
#include <RendererRuntime/Core/File/MemoryFile.h>
#include <RendererRuntime/Core/File/FileSystemHelper.h>

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
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	MaterialAssetCompiler::MaterialAssetCompiler()
	{
		// Nothing here
	}

	MaterialAssetCompiler::~MaterialAssetCompiler()
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	AssetCompilerTypeId MaterialAssetCompiler::getAssetCompilerTypeId() const
	{
		return TYPE_ID;
	}

	bool MaterialAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		// Read in dependency files
		std::vector<std::string> virtualDependencyFilenames;
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + configuration.rapidJsonDocumentAsset["Asset"]["MaterialAssetCompiler"]["InputFile"].GetString();
		JsonMaterialHelper::getDependencyFiles(input, virtualInputFilename, virtualDependencyFilenames);
		const std::string virtualOutputAssetFilename = input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string() + ".material";

		// Let the cache manager check whether or not the files have been changed in order to speed up later checks and to support dependency tracking
		return (input.cacheManager.checkIfFileIsModified(configuration.rendererTarget, input.virtualAssetFilename, {virtualInputFilename}, virtualOutputAssetFilename, RendererRuntime::v1Material::FORMAT_VERSION) || input.cacheManager.dependencyFilesChanged(virtualDependencyFilenames));
	}

	void MaterialAssetCompiler::compile(const Input& input, const Configuration& configuration, Output& output)
	{
		// Get relevant data
		const rapidjson::Value& rapidJsonValueAsset = configuration.rapidJsonDocumentAsset["Asset"];
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + rapidJsonValueAsset["MaterialAssetCompiler"]["InputFile"].GetString();
		const std::string assetName = std_filesystem::path(input.virtualAssetFilename).stem().generic_string();
		const std::string virtualOutputAssetFilename = input.virtualAssetOutputDirectory + '/' + assetName + ".material";

		// Read in dependency files
		std::vector<std::string> virtualDependencyFilenames;
		JsonMaterialHelper::getDependencyFiles(input, virtualInputFilename, virtualDependencyFilenames);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		std::vector<std::string> virtualInputFilenames;
		virtualInputFilenames.emplace_back(virtualInputFilename);
		if (input.cacheManager.needsToBeCompiled(configuration.rendererTarget, input.virtualAssetFilename, virtualInputFilenames, virtualOutputAssetFilename, RendererRuntime::v1Material::FORMAT_VERSION, cacheEntries) || input.cacheManager.dependencyFilesChanged(virtualDependencyFilenames))
		{
			RendererRuntime::MemoryFile memoryFile(0, 1024);

			{ // Material
				// Parse JSON
				rapidjson::Document rapidJsonDocument;
				JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualInputFilename, "MaterialAsset", "1", rapidJsonDocument);
				std::vector<RendererRuntime::v1Material::Technique> techniques;
				RendererRuntime::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVector;
				JsonMaterialHelper::getTechniquesAndPropertiesByMaterialAssetId(input, rapidJsonDocument, techniques, sortedMaterialPropertyVector);

				{ // Write down the material header
					RendererRuntime::v1Material::MaterialHeader materialHeader;
					materialHeader.numberOfTechniques = static_cast<uint32_t>(techniques.size());
					materialHeader.numberOfProperties = static_cast<uint32_t>(sortedMaterialPropertyVector.size());
					memoryFile.write(&materialHeader, sizeof(RendererRuntime::v1Material::MaterialHeader));
				}

				// Write down the material techniques
				if (!techniques.empty())
				{
					memoryFile.write(techniques.data(), sizeof(RendererRuntime::v1Material::Technique) * techniques.size());
				}

				// Write down all material properties
				if (!sortedMaterialPropertyVector.empty())
				{
					memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
				}
			}

			// Write LZ4 compressed output
			memoryFile.writeLz4CompressedDataByVirtualFilename(RendererRuntime::v1Material::FORMAT_TYPE, RendererRuntime::v1Material::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str());

			// Store new cache entries or update existing ones
			input.cacheManager.storeOrUpdateCacheEntries(cacheEntries);
		}

		{ // Update the output asset package
			const std::string assetCategory = rapidJsonValueAsset["AssetMetadata"]["AssetCategory"].GetString();
			const std::string assetIdAsString = input.projectName + "/Material/" + assetCategory + '/' + assetName;
			outputAsset(input.context.getFileManager(), assetIdAsString, virtualOutputAssetFilename, *output.outputAssetPackage);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
