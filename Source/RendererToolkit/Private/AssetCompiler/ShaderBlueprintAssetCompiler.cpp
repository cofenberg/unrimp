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
#include "RendererToolkit/Private/AssetCompiler/ShaderBlueprintAssetCompiler.h"
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/Asset/AssetPackage.h>
#include <RendererRuntime/Public/Core/File/MemoryFile.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Resource/ShaderBlueprint/Cache/ShaderProperties.h>
#include <RendererRuntime/Public/Resource/ShaderBlueprint/Loader/ShaderBlueprintFileFormat.h>

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
		void gatherReferencedShaderProperties(const std::string& sourceString, const std::string& instructionName, RendererRuntime::ShaderProperties& referencedShaderProperties)
		{
			const size_t instructionNameLength = instructionName.length();
			const size_t endPosition = sourceString.length();
			size_t currentPosition = 0;

			// Till the end...
			std::vector<std::string> stringParts;
			while (currentPosition < endPosition)
			{
				const size_t index = sourceString.find(instructionName, currentPosition);
				if (std::string::npos != index)
				{
					currentPosition = index + instructionNameLength;

					// Find the end of the logical property expression
					size_t expressionEndPosition = sourceString.find("(", currentPosition);
					if (std::string::npos != expressionEndPosition)
					{
						// Skip '('
						++expressionEndPosition;

						// Find the end of the logical property expression
						int numberOfOpeningBrackets = 1;
						int numberOfClosingBrackets = 0;
						for (; (expressionEndPosition < endPosition) && (numberOfOpeningBrackets != numberOfClosingBrackets); ++expressionEndPosition)
						{
							const char currentCharacter = sourceString[expressionEndPosition];
							if ('(' == currentCharacter)
							{
								++numberOfOpeningBrackets;
							}
							else if (')' == currentCharacter)
							{
								++numberOfClosingBrackets;
							}
						}

						// TODO(co) I'm sure we can optimize this, but to have something to start with stick to a simple to implement solution
						stringParts.clear();
						RendererToolkit::StringHelper::splitString(sourceString.substr(currentPosition, expressionEndPosition - currentPosition), " 	()!,", stringParts);
						for (const std::string& stringPart : stringParts)
						{
							if (stringPart != "&&" && stringPart != "||")
							{
								referencedShaderProperties.setPropertyValue(RendererRuntime::StringId(stringPart.c_str()), 1);
							}
						}

						// Next, please
						currentPosition = expressionEndPosition;
					}
					else
					{
						// We're done, end-of-file
						break;
					}
				}
				else
				{
					// We're done, end-of-file
					break;
				}
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
	std::string ShaderBlueprintAssetCompiler::getVirtualOutputAssetFilename(const Input& input, const Configuration&) const
	{
		return input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string() + ".shader_blueprint";
	}

	bool ShaderBlueprintAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		return input.cacheManager.checkIfFileIsModified(configuration.rendererTarget, input.virtualAssetFilename, {virtualInputFilename}, getVirtualOutputAssetFilename(input, configuration), RendererRuntime::v1ShaderBlueprint::FORMAT_VERSION);
	}

	void ShaderBlueprintAssetCompiler::compile(const Input& input, const Configuration& configuration) const
	{
		// Get relevant data
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		const std::string virtualOutputAssetFilename = getVirtualOutputAssetFilename(input, configuration);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rendererTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, RendererRuntime::v1ShaderBlueprint::FORMAT_VERSION, cacheEntries))
		{
			RendererRuntime::MemoryFile memoryFile(0, 4096);

			{ // Shader blueprint
				// Get file size and file data
				std::string sourceCode;
				StringHelper::readStringWithStrippedCommentsByFilename(input.context.getFileManager(), virtualInputFilename, sourceCode);
				const std::size_t numberOfBytes = sourceCode.length();

				// Sanity check
				if (0 == numberOfBytes)
				{
					throw std::runtime_error("The shader blueprint source code is empty");
				}

				// Collect shader piece resources to include
				std::vector<RendererRuntime::AssetId> includeShaderPieceAssetIds;
				{
					// Gather "@includepiece(<asset ID>)" and then remove them from the shader source code
					// TODO(co) This is hacked on-the-fly, we certainly need something more robust
					std::unordered_map<uint32_t, std::string> assetIdToString;
					size_t includePiecePosition = sourceCode.find("@includepiece");
					while (std::string::npos != includePiecePosition)
					{
						// ( asset ID )
						const size_t openingPosition = sourceCode.find("(", includePiecePosition);
						const size_t closingPosition = sourceCode.find(")", openingPosition);
						const size_t numberOfCharacters = closingPosition - openingPosition - 1;
						const std::string assetIdAsString = sourceCode.substr(openingPosition + 1, numberOfCharacters);
						const RendererRuntime::AssetId assetId = StringHelper::getSourceAssetIdByString(assetIdAsString.c_str(), input);
						includeShaderPieceAssetIds.push_back(assetId);
						assetIdToString.emplace(assetId, assetIdAsString);
						for (size_t i = includePiecePosition; i < closingPosition + 1; ++i)
						{
							sourceCode[i] = ' ';
						}

						// Next
						includePiecePosition = sourceCode.find("@includepiece", closingPosition);
					}

					// Map the source asset IDs to the compiled asset IDs
					for (RendererRuntime::AssetId& assetId : includeShaderPieceAssetIds)
					{
						SourceAssetIdToCompiledAssetId::const_iterator iterator = input.sourceAssetIdToCompiledAssetId.find(assetId);
						if (iterator != input.sourceAssetIdToCompiledAssetId.cend())
						{
							assetId = iterator->second;
						}
						else
						{
							throw std::runtime_error("Failed to include unknown shader piece asset \"" + assetIdToString[assetId] + '\"');
						}
					}
				}

				// Gather IDs of shader properties known to the shader blueprint resource
				// -> Directly use "RendererRuntime::ShaderProperties" to keep things simple, although we don't need a shader property value
				RendererRuntime::ShaderProperties referencedShaderProperties;
				::detail::gatherReferencedShaderProperties(sourceCode, "@property", referencedShaderProperties);
				::detail::gatherReferencedShaderProperties(sourceCode, "@foreach", referencedShaderProperties);
				if (sourceCode.find("MAIN_BEGIN_VERTEX") != std::string::npos)
				{
					// Build in support for single pass stereo rendering via instancing as described in "High Performance Stereo Rendering For VR", Timothy Wilson, San Diego, Virtual Reality Meetup
					referencedShaderProperties.setPropertyValue(STRING_ID("SinglePassStereoInstancing"), 1);
				}

				{ // Write down the shader blueprint header
					RendererRuntime::v1ShaderBlueprint::ShaderBlueprintHeader shaderBlueprintHeader;
					shaderBlueprintHeader.numberOfIncludeShaderPieceAssetIds = static_cast<uint16_t>(includeShaderPieceAssetIds.size());
					shaderBlueprintHeader.numberOfReferencedShaderProperties = static_cast<uint16_t>(referencedShaderProperties.getSortedPropertyVector().size());
					shaderBlueprintHeader.numberOfShaderSourceCodeBytes		 = static_cast<uint32_t>(numberOfBytes);
					memoryFile.write(&shaderBlueprintHeader, sizeof(RendererRuntime::v1ShaderBlueprint::ShaderBlueprintHeader));
				}

				// Write down the asset IDs of the shader pieces to include
				if (!includeShaderPieceAssetIds.empty())
				{
					memoryFile.write(includeShaderPieceAssetIds.data(), sizeof(RendererRuntime::AssetId) * includeShaderPieceAssetIds.size());
				}

				{ // Write down the referenced shader properties
					const RendererRuntime::ShaderProperties::SortedPropertyVector& sortedPropertyVector = referencedShaderProperties.getSortedPropertyVector();
					if (!sortedPropertyVector.empty())
					{
						memoryFile.write(sortedPropertyVector.data(), sizeof(RendererRuntime::ShaderProperties::Property) * sortedPropertyVector.size());
					}
				}

				// Dump the unchanged content into the output file stream
				memoryFile.write(sourceCode.c_str(), numberOfBytes);
			}

			// Write LZ4 compressed output
			if (!memoryFile.writeLz4CompressedDataByVirtualFilename(RendererRuntime::v1ShaderBlueprint::FORMAT_TYPE, RendererRuntime::v1ShaderBlueprint::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str()))
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
