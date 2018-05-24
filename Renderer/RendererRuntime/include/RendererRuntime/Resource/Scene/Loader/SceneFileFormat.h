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
#include "RendererRuntime/Core/StringId.h"
#include "RendererRuntime/Core/Math/Transform.h"
#include "RendererRuntime/Resource/Scene/Item/Light/LightSceneItem.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;				///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset type>/<asset category>/<asset name>"
	typedef StringId SceneItemTypeId;		///< Scene item type identifier, internally just a POD "uint32_t"
	typedef StringId MaterialTechniqueId;	///< Material technique identifier, internally just a POD "uint32_t", result of hashing the material technique name


	// Scene file format content:
	// - File format header
	// - Scene header
	namespace v1Scene
	{


		//[-------------------------------------------------------]
		//[ Definitions                                           ]
		//[-------------------------------------------------------]
		static constexpr uint32_t FORMAT_TYPE	 = STRING_ID("Scene");
		static constexpr uint32_t FORMAT_VERSION = 3;

		#pragma pack(push)
		#pragma pack(1)
			struct SceneHeader final
			{
				uint32_t unused;	// TODO(co) Currently the scene header is unused
			};

			struct Nodes final
			{
				uint32_t numberOfNodes;
			};

			struct Node final
			{
				Transform transform;
				uint32_t  numberOfItems;
			};

			struct ItemHeader final
			{
				SceneItemTypeId typeId;
				uint32_t		numberOfBytes;
			};

			struct CameraItem final
			{
			};

			struct LightItem final
			{
				LightSceneItem::LightType lightType		   = LightSceneItem::LightType::POINT;
				float					  color[3]		   = { 1.0f, 1.0f, 1.0f };
				float					  radius		   = 1.0f;
				float					  innerAngle	   = glm::radians(40.0f);
				float					  outerAngle	   = glm::radians(50.0f);
				float					  nearClipDistance = 0.0f;
			};

			struct SunlightItem final
			{
				// Usually fixed
				float sunriseTime	   = 07.50f;	// O'clock
				float sunsetTime	   = 20.50f;	// O'clock
				float eastDirection	   = 0.0f;
				float angleOfIncidence = glm::radians(20.0f);
				float timeOfDay		   = 10.00f;	// O'clock
			};

			struct MeshItem final
			{
				AssetId  meshAssetId;
				uint32_t numberOfSubMeshMaterialAssetIds = 0;
			};

			struct SkeletonMeshItem final	// : public MeshItem -> Not derived by intent to be able to reuse the mesh item serialization 1:1
			{
				AssetId skeletonAnimationAssetId;
			};

			struct MaterialItem
			{
				AssetId				materialAssetId;				///< If material blueprint asset ID is set, material asset ID must be uninitialized
				MaterialTechniqueId	materialTechniqueId;			///< Must always be valid
				AssetId				materialBlueprintAssetId;		///< If material asset ID is set, material blueprint asset ID must be uninitialized
				uint32_t			numberOfMaterialProperties = 0;
			};

			struct SkyItem final : public MaterialItem
			{
				// Nothing here
			};

			struct TerrainItem final : public MaterialItem
			{
				// Nothing here
			};

			struct ParticlesItem final : public MaterialItem
			{
				// Nothing here
			};
		#pragma pack(pop)


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // v1Scene
} // RendererRuntime
