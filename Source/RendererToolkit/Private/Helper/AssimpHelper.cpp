/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "RendererToolkit/Private/Helper/AssimpHelper.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'FORCE_32BIT' in switch of enum 'aiMetadataType' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'return': conversion from 'int' to 'std::_Rand_urng_from_func::result_type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4619)	// warning C4619: #pragma warning: there is no warning number '4351'
	PRAGMA_WARNING_DISABLE_MSVC(5219)	// warning C5219: implicit conversion from 'int' to 'float', possible loss of data
	#include <assimp/scene.h>
	#include <assimp/postprocess.h>
PRAGMA_WARNING_POP

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
	PRAGMA_WARNING_DISABLE_MSVC(5054)	// warning C5054: operator '|': deprecated between enumerations of different types
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
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		// "aiProcess_MakeLeftHanded" is added because the rasterizer states directly map to Direct3D
		static constexpr uint32_t DefaultFlags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder;


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] uint32_t getAssimpFlagByString(const std::string& flagAsString)
		{
			// Define helper macros
			#define IF_VALUE(name, value)			if (flagAsString == #name) return value;
			#define ELSE_IF_VALUE(name, value) else if (flagAsString == #name) return value;

			// Evaluate value
			IF_VALUE(	  DEFAULT_FLAGS,					DefaultFlags)
			ELSE_IF_VALUE(CALCULATE_TANGENT_SPACE,			aiProcess_CalcTangentSpace)
			ELSE_IF_VALUE(JOIN_IDENTICAL_VERTICES,			aiProcess_JoinIdenticalVertices)
			ELSE_IF_VALUE(MAKE_LEFT_HANDED,					aiProcess_MakeLeftHanded)
			ELSE_IF_VALUE(TRIANGULATE,						aiProcess_Triangulate)
			ELSE_IF_VALUE(REMOVE_COMPONENT,					aiProcess_RemoveComponent)	// Not that useful as flag here, but let's be consistent
			ELSE_IF_VALUE(GENERATE_NORMALS,					aiProcess_GenNormals)
			ELSE_IF_VALUE(GENERATE_SMOOTH_NORMALS,			aiProcess_GenSmoothNormals)
			ELSE_IF_VALUE(SPLIT_LARGE_MESHES,				aiProcess_SplitLargeMeshes)
			ELSE_IF_VALUE(PRE_TRANSFORM_VERTICES,			aiProcess_PreTransformVertices)
			ELSE_IF_VALUE(LIMIT_BONE_WEIGHTS,				aiProcess_LimitBoneWeights)
			ELSE_IF_VALUE(VALIDATE_DATA_STRUCTURE,			aiProcess_ValidateDataStructure)
			ELSE_IF_VALUE(IMPROVE_CACHE_LOCALITY,			aiProcess_ImproveCacheLocality)
			ELSE_IF_VALUE(REMOVE_REDUNDANT_MATERIALS,		aiProcess_RemoveRedundantMaterials)
			ELSE_IF_VALUE(FIX_INTERFACING_NORMALS,			aiProcess_FixInfacingNormals)
			ELSE_IF_VALUE(SORT_BY_PTYPE,					aiProcess_SortByPType)
			ELSE_IF_VALUE(FIND_DEGENERATES,					aiProcess_FindDegenerates)
			ELSE_IF_VALUE(FIND_INVALID_DATA,				aiProcess_FindInvalidData)
			ELSE_IF_VALUE(GENERATE_UV_COORDINATES,			aiProcess_GenUVCoords)
			ELSE_IF_VALUE(TRANSFORM_UV_COORDINATES,			aiProcess_TransformUVCoords)
			ELSE_IF_VALUE(FIND_INSTANCES,					aiProcess_FindInstances)
			ELSE_IF_VALUE(OPTIMIZE_MESHES,					aiProcess_OptimizeMeshes)
			ELSE_IF_VALUE(OPTIMIZE_GRAPH,					aiProcess_OptimizeGraph)
			ELSE_IF_VALUE(FLIP_UVS,							aiProcess_FlipUVs)
			ELSE_IF_VALUE(FLIP_WINDING_ORDER,				aiProcess_FlipWindingOrder)
			ELSE_IF_VALUE(SPLIT_BY_BONE_COUNT,				aiProcess_SplitByBoneCount)
			ELSE_IF_VALUE(DEBONE,							aiProcess_Debone)
			ELSE_IF_VALUE(TARGET_REALTIME_FAST,				aiProcessPreset_TargetRealtime_Fast)
			ELSE_IF_VALUE(TARGET_REALTIME_QUALITY,			aiProcessPreset_TargetRealtime_Quality)
			ELSE_IF_VALUE(TARGET_REALTIME_MAXIMUM_QUALITY,	aiProcessPreset_TargetRealtime_MaxQuality)
			else
			{
				throw std::runtime_error(std::string("Flag \"") + flagAsString + "\" is unknown");
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}

		/**
		*  @brief
		*    Get the number of bones recursive
		*
		*  @param[in] assimpNode
		*    Assimp node to gather the data from
		*
		*  @return
		*    The number of bones
		*/
		[[nodiscard]] uint32_t getNumberOfBonesRecursive(const aiNode& assimpNode)
		{
			// Loop through all child nodes recursively
			uint32_t numberOfBones = assimpNode.mNumChildren;
			for (uint32_t i = 0; i < assimpNode.mNumChildren; ++i)
			{
				numberOfBones += getNumberOfBonesRecursive(*assimpNode.mChildren[i]);
			}

			// Done
			return numberOfBones;
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
	uint32_t AssimpHelper::getAssimpFlagsByRapidJsonValue(const rapidjson::Value& rapidJsonValue, const char* propertyName)
	{
		// Flags are defined in C-style: Example: "ImportFlags": "DEFAULT_FLAGS & ~REMOVE_REDUNDANT_MATERIALS"
		// TODO(co) Currently the C-style flags parsing is implemented in a totally primitive way. Support for () and real |& differentiation might be useful later on.
		if (rapidJsonValue.HasMember(propertyName))
		{
			uint32_t flags = 0;

			// Get individual flags
			std::vector<std::string> elements;
			StringHelper::splitString(rapidJsonValue[propertyName].GetString(), "|&", elements);
			if (!elements.empty())
			{
				for (std::string element : elements)
				{
					element = StringHelper::trimWhitespaceCharacters(element);
					if ('~' == element[0])
					{
						// Remove flag
						flags &= ~::detail::getAssimpFlagByString(element.substr(1));
					}
					else
					{
						// Add flag
						flags |= ::detail::getAssimpFlagByString(element);
					}
				}
			}

			// Done
			return flags;
		}
		else
		{
			// Return default Assimp flags
			return ::detail::DefaultFlags;
		}
	}

	uint32_t AssimpHelper::getNumberOfBones(const aiNode& assimpNode)
	{
		uint32_t numberOfBones = 0;

		// OGRE: The scene root node has no name
		if (0 == assimpNode.mName.length)
		{
			if (1 != assimpNode.mNumChildren)
			{
				throw std::runtime_error("There can be only single root bone");
			}
			numberOfBones = ::detail::getNumberOfBonesRecursive(assimpNode);
		}

		// FBX: The scene root node name is "RootNode"
		else if (assimpNode.mName == aiString("RootNode"))
		{
			// TODO(co) Skeleton support is under construction
			NOP;
		}

		// MD5: The MD5 bones hierarchy is stored inside an Assimp node names "<MD5_Hierarchy>"
		else if (assimpNode.mName == aiString("<MD5_Root>"))
		{
			for (uint32_t i = 0; i < assimpNode.mNumChildren; ++i)
			{
				const aiNode* assimpChildNode = assimpNode.mChildren[i];
				if (assimpChildNode->mName == aiString("<MD5_Hierarchy>"))
				{
					numberOfBones = ::detail::getNumberOfBonesRecursive(*assimpChildNode);
					break;
				}
			}
		}

		// Done
		return numberOfBones;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
