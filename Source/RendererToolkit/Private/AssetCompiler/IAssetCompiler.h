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
#include <RendererRuntime/Public/Core/StringId.h>

#include <Renderer/Public/Renderer.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <rapidjson/fwd.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <string>
	#include <vector>
	#include <unordered_map>
	#include <unordered_set>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class AssetPackage;
	class IFileManager;
}
namespace RendererToolkit
{
	class Context;
	class CacheManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef RendererRuntime::StringId				  AssetCompilerTypeId;				///< Asset compiler type identifier, internally just a POD "uint32_t"
	typedef std::unordered_map<uint32_t, uint32_t>	  SourceAssetIdToCompiledAssetId;	///< Key = source asset ID, value = compiled asset ID ("AssetId"-type not used directly or we would need to define a hash-function for it)
	typedef std::unordered_map<uint32_t, uint32_t>	  CompiledAssetIdToSourceAssetId;	///< Key = compiled asset ID, value = source asset ID ("AssetId"-type not used directly or we would need to define a hash-function for it)
	typedef std::unordered_map<uint32_t, std::string> SourceAssetIdToVirtualFilename;	///< Key = source asset ID, virtual asset filename
	typedef std::unordered_set<uint32_t>			  DefaultTextureAssetIds;			///< "RendererRuntime::AssetId"-type for compiled asset IDs

	/**
	*  @brief
	*    Overall quality strategy which is a trade-off between "fast" and "good"
	*/
	enum class QualityStrategy
	{
		DEBUG,		///< Best possible speed, quality doesn't matter as long as things can still be identified
		PRODUCTION,	///< Decent speed and decent quality so e.g. artists can fine tune assets
		SHIPPING	///< Product is about to be shipped to clients, best possible speed as long as it finishes before the sun burns out
	};


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    An asset compiler transforms an asset from a source format into a format the renderer runtime understands
	*
	*  @note
	*    - The asset compiler only crunches data already known to the source asset database (aka "data source"), it doesn't process external assets
	*    - An asset compiler only cares about a single asset, it doesn't for example processes automatically all material assets
	*      a mesh asset is referencing and then all texture assets a material assets is referencing
	*    - Either manually integrate new assets into the source asset database or use an asset importer to do so
	*/
	class IAssetCompiler
	{


	//[-------------------------------------------------------]
	//[ Public structures                                     ]
	//[-------------------------------------------------------]
	public:
		struct Input final
		{
			const Context&							context;
			const std::string						projectName;
			CacheManager&							cacheManager;
			const std::string						virtualAssetPackageInputDirectory;	///< Without "/" at the end
			const std::string						virtualAssetFilename;
			const std::string						virtualAssetInputDirectory;			///< Without "/" at the end
			const std::string						virtualAssetOutputDirectory;		///< Without "/" at the end
			const SourceAssetIdToCompiledAssetId&	sourceAssetIdToCompiledAssetId;
			const CompiledAssetIdToSourceAssetId&	compiledAssetIdToSourceAssetId;
			const SourceAssetIdToVirtualFilename&	sourceAssetIdToVirtualFilename;
			const DefaultTextureAssetIds&			defaultTextureAssetIds;

			Input() = delete;
			Input(const Context& _context, const std::string _projectName, CacheManager& _cacheManager, const std::string& _virtualAssetPackageInputDirectory, const std::string& _virtualAssetFilename, const std::string& _virtualAssetInputDirectory,
				  const std::string& _virtualAssetOutputDirectory, const SourceAssetIdToCompiledAssetId& _sourceAssetIdToCompiledAssetId, const CompiledAssetIdToSourceAssetId& _compiledAssetIdToSourceAssetId, const SourceAssetIdToVirtualFilename& _sourceAssetIdToVirtualFilename, const DefaultTextureAssetIds& _defaultTextureAssetIds) :
				context(_context),
				projectName(_projectName),
				cacheManager(_cacheManager),
				virtualAssetPackageInputDirectory(_virtualAssetPackageInputDirectory),
				virtualAssetFilename(_virtualAssetFilename),
				virtualAssetInputDirectory(_virtualAssetInputDirectory),
				virtualAssetOutputDirectory(_virtualAssetOutputDirectory),
				sourceAssetIdToCompiledAssetId(_sourceAssetIdToCompiledAssetId),
				compiledAssetIdToSourceAssetId(_compiledAssetIdToSourceAssetId),
				sourceAssetIdToVirtualFilename(_sourceAssetIdToVirtualFilename),
				defaultTextureAssetIds(_defaultTextureAssetIds)
			{
				// Nothing here
			}
			uint32_t getCompiledAssetIdBySourceAssetId(uint32_t sourceAssetId) const
			{
				SourceAssetIdToCompiledAssetId::const_iterator iterator = sourceAssetIdToCompiledAssetId.find(sourceAssetId);
				if (iterator == sourceAssetIdToCompiledAssetId.cend())
				{
					throw std::runtime_error(std::string("Source asset ID ") + std::to_string(sourceAssetId) + " is unknown");
				}
				return iterator->second;
			}
			uint32_t getCompiledAssetIdBySourceAssetIdAsString(const std::string& sourceAssetIdAsString) const
			{
				// Source asset ID naming scheme "<name>.asset"
				SourceAssetIdToCompiledAssetId::const_iterator iterator = sourceAssetIdToCompiledAssetId.find(RendererRuntime::StringId::calculateFNV(sourceAssetIdAsString.c_str()));
				if (iterator == sourceAssetIdToCompiledAssetId.cend())
				{
					throw std::runtime_error(std::string("Source asset ID \"") + sourceAssetIdAsString + "\" is unknown");
				}
				return iterator->second;
			}
			std::string sourceAssetIdToDebugName(uint32_t sourceAssetId) const
			{
				SourceAssetIdToVirtualFilename::const_iterator iterator = sourceAssetIdToVirtualFilename.find(sourceAssetId);
				if (iterator == sourceAssetIdToVirtualFilename.cend())
				{
					throw std::runtime_error(std::string("Source asset ID ") + std::to_string(sourceAssetId) + " is unknown");
				}
				return '\"' + iterator->second + "\" (ID = " + std::to_string(sourceAssetId) + ')';
			}
			const std::string& sourceAssetIdToVirtualAssetFilename(uint32_t sourceAssetId) const
			{
				SourceAssetIdToVirtualFilename::const_iterator iterator = sourceAssetIdToVirtualFilename.find(sourceAssetId);
				if (sourceAssetIdToVirtualFilename.cend() != iterator)
				{
					return iterator->second;
				}
				else
				{
					throw std::runtime_error("Failed to map source asset ID " + std::to_string(sourceAssetId) + " to virtual asset filename");
				}
			}
			const std::string& compiledAssetIdToVirtualAssetFilename(uint32_t compiledAssetId) const
			{
				// Map compiled asset ID to source asset ID
				CompiledAssetIdToSourceAssetId::const_iterator iterator = compiledAssetIdToSourceAssetId.find(compiledAssetId);
				if (iterator == compiledAssetIdToSourceAssetId.cend())
				{
					throw std::runtime_error(std::string("Compiled asset ID ") + std::to_string(compiledAssetId) + " is unknown");
				}

				// Map source asset ID to virtual asset filename
				return sourceAssetIdToVirtualAssetFilename(iterator->second);
			}

			Input(const Input&) = delete;
			Input& operator=(const Input&) = delete;
		};
		struct Configuration final
		{
			const rapidjson::Document& rapidJsonDocumentAsset;
			const rapidjson::Value&    rapidJsonValueTargets;
			std::string				   rendererTarget;
			QualityStrategy			   qualityStrategy;
			Configuration(const rapidjson::Document& _rapidJsonDocumentAsset, const rapidjson::Value& _rapidJsonValueTargets, const std::string& _rendererTarget, QualityStrategy _qualityStrategy) :
				rapidJsonDocumentAsset(_rapidJsonDocumentAsset),
				rapidJsonValueTargets(_rapidJsonValueTargets),
				rendererTarget(_rendererTarget),
				qualityStrategy(_qualityStrategy)
			{
				// Nothing here
			}
			Configuration(const Configuration&) = delete;
			Configuration& operator =(const Configuration&) = delete;
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline virtual ~IAssetCompiler() {}


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	public:
		virtual AssetCompilerTypeId getAssetCompilerTypeId() const = 0;
		virtual std::string getVirtualOutputAssetFilename(const Input& input, const Configuration& configuration) const = 0;
		virtual bool checkIfChanged(const Input& input, const Configuration& configuration) const = 0;
		virtual void compile(const Input& input, const Configuration& configuration) const = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline IAssetCompiler() {}


	};


	//[-------------------------------------------------------]
	//[ Type definitions                                      ]
	//[-------------------------------------------------------]
	typedef Renderer::SmartRefCount<IAssetCompiler> IAssetCompilerPtr;


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
