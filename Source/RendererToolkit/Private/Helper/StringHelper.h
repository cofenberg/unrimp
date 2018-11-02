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


//[-------------------------------------------------------]
//[ Global definitions                                    ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IFileManager;
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
	class StringHelper final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		static void replaceFirstString(std::string& stringToUpdate, const std::string& fromString, const std::string& toString);
		static void toLowerCase(std::string& strintToLower);
		static void splitString(const std::string& stringToSplit, char separator, std::vector<std::string>& elements);
		static void splitString(const std::string& stringToSplit, const std::string& separators, std::vector<std::string>& elements);
		static std::string& trimRightWhitespaceCharacters(std::string& s);	// In place
		static std::string& trimLeftWhitespaceCharacters(std::string& s);	// In place
		static std::string& trimWhitespaceCharacters(std::string& s);		// In place
		[[nodiscard]] static bool isSourceAssetIdAsString(const std::string& sourceAssetIdAsString);
		[[nodiscard]] static std::string getSourceAssetFilenameByString(const std::string& sourceAssetIdAsString, const IAssetCompiler::Input& input, bool supportAutomaticallyGeneratedAssetFiles = true);
		[[nodiscard]] static RendererRuntime::AssetId getSourceAssetIdByString(const std::string& sourceAssetIdAsString, const IAssetCompiler::Input& input);
		[[nodiscard]] static RendererRuntime::AssetId getAssetIdByString(const std::string& assetIdAsString, const IAssetCompiler::Input& input);	// Asset ID name + ID directly
		static void readStringByFilename(const RendererRuntime::IFileManager& fileManager, const std::string& virtualFilename, std::string& string);
		static void readStringWithStrippedCommentsByFilename(const RendererRuntime::IFileManager& fileManager, const std::string& virtualFilename, std::string& sourceCode);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		StringHelper(const StringHelper&) = delete;
		StringHelper& operator=(const StringHelper&) = delete;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
