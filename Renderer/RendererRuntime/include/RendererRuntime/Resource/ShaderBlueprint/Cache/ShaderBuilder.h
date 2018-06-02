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
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/ShaderProperties.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <map>
	#include <string>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class Context;
}
namespace RendererRuntime
{
	class ShaderBlueprintResource;
	class ShaderPieceResourceManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId						AssetId;				///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset type>/<asset category>/<asset name>"
	typedef std::vector<AssetId>			AssetIds;
	typedef std::map<uint32_t, std::string> DynamicShaderPieces;	///< Key is "RendererRuntime::StringId"	// TODO(co) Visual Studio 2017: "std::unordered_map" appears to have an inefficient assignment operator which does memory handling even if containers are empty all the time, "std::map" isn't the most effective structure either but currently still better


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Shader builder
	*
	*  @note
	*   - Heavily basing on the OGRE 2.1 HLMS shader builder which is directly part of the OGRE class "Ogre::Hlms". So for syntax, have a look into the OGRE 2.1 documentation.
	*/
	class ShaderBuilder final
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		struct BuildShader final
		{
			std::string sourceCode;
			AssetIds	assetIds;						///< List of IDs of the assets (shader blueprint, shader piece) which took part in the shader cache creation
			uint64_t	combinedAssetFileHashes = 0;	///< Combination of the file hash of all assets (shader blueprint, shader piece) which took part in the shader cache creation
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		inline ShaderBuilder(const Renderer::Context& context) :
			mContext(context)
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline ~ShaderBuilder()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Create shader source code by using the given shader blueprint and shader properties
		*
		*  @param[in] shaderPieceResourceManager
		*    Shader piece resource manager to use
		*  @param[in] shaderBlueprintResource
		*    Shader blueprint resource to use
		*  @param[in] shaderProperties
		*    Shader properties to use
		*  @param[out] buildShader
		*    Receives the build shader
		*/
		void createSourceCode(const ShaderPieceResourceManager& shaderPieceResourceManager, const ShaderBlueprintResource& shaderBlueprintResource, const ShaderProperties& shaderProperties, BuildShader& buildShader);


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderBuilder(const ShaderBuilder&) = delete;
		ShaderBuilder& operator=(const ShaderBuilder&) = delete;
		bool parseMath(const std::string& inBuffer, std::string& outBuffer);
		bool parseForEach(const std::string& inBuffer, std::string& outBuffer) const;
		bool parseProperties(std::string& inBuffer, std::string& outBuffer) const;
		bool collectPieces(const std::string& inBuffer, std::string& outBuffer);
		bool insertPieces(std::string& inBuffer, std::string& outBuffer) const;
		bool parseCounter(const std::string& inBuffer, std::string& outBuffer);
		bool parse(const std::string& inBuffer, std::string& outBuffer) const;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		const Renderer::Context& mContext;
		ShaderProperties		 mShaderProperties;
		DynamicShaderPieces		 mDynamicShaderPieces;
		std::string				 mInString;		///< Could be a local variable, but when making it to a member we reduce memory allocations
		std::string				 mOutString;	///< Could be a local variable, but when making it to a member we reduce memory allocations


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
