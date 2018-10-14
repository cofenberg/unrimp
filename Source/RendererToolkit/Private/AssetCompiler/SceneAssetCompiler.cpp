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
#include "RendererToolkit/Private/AssetCompiler/SceneAssetCompiler.h"
#include "RendererToolkit/Private/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Private/Helper/JsonMaterialHelper.h"
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/Asset/AssetPackage.h>
#include <RendererRuntime/Public/Core/Math/Math.h>
#include <RendererRuntime/Public/Core/File/MemoryFile.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Sky/SkySceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Volume/VolumeSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Grass/GrassSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Terrain/TerrainSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Camera/CameraSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Light/SunlightSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Particles/ParticlesSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Loader/SceneFileFormat.h>
#include <RendererRuntime/Public/Resource/Material/MaterialProperties.h>
#include <RendererRuntime/Public/Resource/Material/MaterialResourceManager.h>

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
		//[ Functions                                             ]
		//[-------------------------------------------------------]
		void optionalLightTypeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, RendererRuntime::LightSceneItem::LightType& value)
		{
			if (rapidJsonValue.HasMember(propertyName))
			{
				const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
				const char* valueAsString = rapidJsonValueValueType.GetString();
				const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

				// Define helper macros
				#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = RendererRuntime::LightSceneItem::LightType::name;
				#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = RendererRuntime::LightSceneItem::LightType::name;

				// Evaluate value
				IF_VALUE(DIRECTIONAL)
				ELSE_IF_VALUE(POINT)
				ELSE_IF_VALUE(SPOT)
				else
				{
					throw std::runtime_error("Light type \"" + std::string(valueAsString) + "\" is unknown. The light type must be one of the following constants: DIRECTIONAL, POINT or SPOT");
				}

				// Undefine helper macros
				#undef IF_VALUE
				#undef ELSE_IF_VALUE
			}
		}

		void fillSortedMaterialPropertyVector(const RendererToolkit::IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueSceneItem, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
		{
			// Check whether or not material properties should be set
			if (rapidJsonValueSceneItem.HasMember("SetMaterialProperties"))
			{
				if (rapidJsonValueSceneItem.HasMember("Material"))
				{
					RendererToolkit::JsonMaterialHelper::getPropertiesByMaterialAssetId(input, RendererToolkit::StringHelper::getSourceAssetIdByString(rapidJsonValueSceneItem["Material"].GetString(), input), sortedMaterialPropertyVector);
				}
				else if (rapidJsonValueSceneItem.HasMember("MaterialBlueprint"))
				{
					RendererToolkit::JsonMaterialBlueprintHelper::getPropertiesByMaterialBlueprintAssetId(input, RendererToolkit::StringHelper::getSourceAssetIdByString(rapidJsonValueSceneItem["MaterialBlueprint"].GetString(), input), sortedMaterialPropertyVector);
				}
				if (!sortedMaterialPropertyVector.empty())
				{
					// Update material property values were required
					const rapidjson::Value& rapidJsonValueProperties = rapidJsonValueSceneItem["SetMaterialProperties"];
					RendererToolkit::JsonMaterialHelper::readMaterialPropertyValues(input, rapidJsonValueProperties, sortedMaterialPropertyVector);

					// Collect all material property IDs explicitly defined inside the scene item
					typedef std::unordered_map<uint32_t, std::string> DefinedMaterialPropertyIds;	// Key = "RendererRuntime::RendererRuntime::MaterialPropertyId"
					DefinedMaterialPropertyIds definedMaterialPropertyIds;
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorProperties = rapidJsonValueProperties.MemberBegin(); rapidJsonMemberIteratorProperties != rapidJsonValueProperties.MemberEnd(); ++rapidJsonMemberIteratorProperties)
					{
						definedMaterialPropertyIds.emplace(RendererRuntime::MaterialPropertyId(rapidJsonMemberIteratorProperties->name.GetString()), rapidJsonMemberIteratorProperties->value.GetString());
					}

					// Mark material properties as overwritten
					for (RendererRuntime::MaterialProperty& materialProperty : sortedMaterialPropertyVector)
					{
						DefinedMaterialPropertyIds::const_iterator iterator = definedMaterialPropertyIds.find(materialProperty.getMaterialPropertyId());
						if (iterator!= definedMaterialPropertyIds.end())
						{
							materialProperty.setOverwritten(true);
						}
					}
				}
			}
		}

		void readMaterialSceneItem(const RendererToolkit::IAssetCompiler::Input& input, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, const rapidjson::Value& rapidJsonValueSceneItem, RendererRuntime::v1Scene::MaterialItem& materialItem)
		{
			// Set data
			RendererRuntime::AssetId materialAssetId;
			RendererRuntime::AssetId materialBlueprintAssetId;
			RendererToolkit::JsonHelper::optionalCompiledAssetId(input, rapidJsonValueSceneItem, "Material", materialAssetId);
			RendererToolkit::JsonHelper::optionalStringIdProperty(rapidJsonValueSceneItem, "MaterialTechnique", materialItem.materialTechniqueId);
			RendererToolkit::JsonHelper::optionalCompiledAssetId(input, rapidJsonValueSceneItem, "MaterialBlueprint", materialBlueprintAssetId);
			materialItem.materialAssetId = materialAssetId;
			materialItem.materialBlueprintAssetId = materialBlueprintAssetId;
			materialItem.numberOfMaterialProperties = static_cast<uint32_t>(sortedMaterialPropertyVector.size());

			// Sanity checks
			if (RendererRuntime::isInvalid(materialItem.materialAssetId) && RendererRuntime::isInvalid(materialItem.materialBlueprintAssetId))
			{
				throw std::runtime_error("Material asset ID or material blueprint asset ID must be defined");
			}
			if (RendererRuntime::isValid(materialItem.materialAssetId) && RendererRuntime::isValid(materialItem.materialBlueprintAssetId))
			{
				throw std::runtime_error("Material asset ID is defined, but material blueprint asset ID is defined as well. Only one asset ID is allowed.");
			}
			if (RendererRuntime::isInvalid(materialItem.materialTechniqueId))
			{
				materialItem.materialTechniqueId = RendererRuntime::MaterialResourceManager::DEFAULT_MATERIAL_TECHNIQUE_ID;
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
	std::string SceneAssetCompiler::getVirtualOutputAssetFilename(const Input& input, const Configuration&) const
	{
		return input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string() + ".scene";
	}

	bool SceneAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		return input.cacheManager.checkIfFileIsModified(configuration.rendererTarget, input.virtualAssetFilename, {virtualInputFilename}, getVirtualOutputAssetFilename(input, configuration), RendererRuntime::v1Scene::FORMAT_VERSION);
	}

	void SceneAssetCompiler::compile(const Input& input, const Configuration& configuration) const
	{
		// Get relevant data
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		const std::string virtualOutputAssetFilename = getVirtualOutputAssetFilename(input, configuration);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rendererTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, RendererRuntime::v1Scene::FORMAT_VERSION, cacheEntries))
		{
			RendererRuntime::MemoryFile memoryFile(0, 4096);

			{ // Scene
				// Parse JSON
				rapidjson::Document rapidJsonDocument;
				JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualInputFilename, "SceneAsset", "1", rapidJsonDocument);

				{ // Write down the scene resource header
					RendererRuntime::v1Scene::SceneHeader sceneHeader;
					sceneHeader.unused = 42;	// TODO(co) Currently the scene header is unused
					memoryFile.write(&sceneHeader, sizeof(RendererRuntime::v1Scene::SceneHeader));
				}

				// Mandatory main sections of the material blueprint
				const rapidjson::Value& rapidJsonValueSceneAsset = rapidJsonDocument["SceneAsset"];
				const rapidjson::Value& rapidJsonValueNodes = rapidJsonValueSceneAsset["Nodes"];

				// Sanity check
				if (rapidJsonValueNodes.Empty())
				{
					throw std::runtime_error("Scene asset \"" + virtualInputFilename + "\" has no nodes");
				}

				{ // Write down the scene nodes
					const rapidjson::SizeType numberOfNodes = rapidJsonValueNodes.Size();
					RendererRuntime::v1Scene::Nodes nodes;
					nodes.numberOfNodes = numberOfNodes;
					memoryFile.write(&nodes, sizeof(RendererRuntime::v1Scene::Nodes));

					// Loop through all scene nodes
					for (rapidjson::SizeType nodeIndex = 0; nodeIndex < numberOfNodes; ++nodeIndex)
					{
						const rapidjson::Value& rapidJsonValueNode = rapidJsonValueNodes[nodeIndex];
						const rapidjson::Value* rapidJsonValueItems = rapidJsonValueNode.HasMember("Items") ? &rapidJsonValueNode["Items"] : nullptr;

						{ // Write down the scene node
							RendererRuntime::v1Scene::Node node;

							// Get the scene node transform
							node.transform.scale = RendererRuntime::Math::VEC3_ONE;
							if (rapidJsonValueNode.HasMember("Properties"))
							{
								const rapidjson::Value& rapidJsonValueProperties = rapidJsonValueNode["Properties"];

								// Position, rotation and scale
								JsonHelper::optionalUnitNProperty(rapidJsonValueProperties, "Position", &node.transform.position.x, 3);
								JsonHelper::optionalRotationQuaternionProperty(rapidJsonValueProperties, "Rotation", node.transform.rotation);
								JsonHelper::optionalFactorNProperty(rapidJsonValueProperties, "Scale", &node.transform.scale.x, 3);
							}

							// Write down the scene node
							node.numberOfItems = (nullptr != rapidJsonValueItems) ? rapidJsonValueItems->MemberCount() : 0;
							memoryFile.write(&node, sizeof(RendererRuntime::v1Scene::Node));
						}

						// Write down the scene items
						if (nullptr != rapidJsonValueItems)
						{
							for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorItems = rapidJsonValueItems->MemberBegin(); rapidJsonMemberIteratorItems != rapidJsonValueItems->MemberEnd(); ++rapidJsonMemberIteratorItems)
							{
								const rapidjson::Value& rapidJsonValueItem = rapidJsonMemberIteratorItems->value;
								const RendererRuntime::SceneItemTypeId typeId = RendererRuntime::StringId(rapidJsonMemberIteratorItems->name.GetString());

								// Get the scene item type specific data number of bytes
								// TODO(co) Make this more generic via scene factory
								uint32_t numberOfBytes = 0;
								RendererRuntime::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVector;
								switch (typeId)
								{
									case RendererRuntime::CameraSceneItem::TYPE_ID:
										// Nothing here
										break;

									case RendererRuntime::LightSceneItem::TYPE_ID:
										numberOfBytes = sizeof(RendererRuntime::v1Scene::LightItem);
										break;

									case RendererRuntime::SunlightSceneItem::TYPE_ID:
										numberOfBytes = sizeof(RendererRuntime::v1Scene::SunlightItem);
										break;

									case RendererRuntime::MeshSceneItem::TYPE_ID:
									case RendererRuntime::SkeletonMeshSceneItem::TYPE_ID:
									{
										const uint32_t numberOfSubMeshMaterialAssetIds = rapidJsonValueItem.HasMember("SubMeshMaterials") ? rapidJsonValueItem["SubMeshMaterials"].Size() : 0;
										numberOfBytes = sizeof(RendererRuntime::v1Scene::MeshItem) + sizeof(RendererRuntime::AssetId) * numberOfSubMeshMaterialAssetIds;
										if (RendererRuntime::SkeletonMeshSceneItem::TYPE_ID == typeId)
										{
											numberOfBytes += sizeof(RendererRuntime::v1Scene::SkeletonMeshItem);
										}
										break;
									}

									case RendererRuntime::SkySceneItem::TYPE_ID:
									case RendererRuntime::VolumeSceneItem::TYPE_ID:
									case RendererRuntime::GrassSceneItem::TYPE_ID:
									case RendererRuntime::TerrainSceneItem::TYPE_ID:
									case RendererRuntime::ParticlesSceneItem::TYPE_ID:
										::detail::fillSortedMaterialPropertyVector(input, rapidJsonValueItem, sortedMaterialPropertyVector);
										numberOfBytes = static_cast<uint32_t>(sizeof(RendererRuntime::v1Scene::MaterialItem) + sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
										break;

									default:
										// Error!
										throw std::runtime_error("Scene item type \"" + std::string(rapidJsonMemberIteratorItems->name.GetString()) + "\" is unknown");
								}

								{ // Write down the scene item header
									RendererRuntime::v1Scene::ItemHeader itemHeader;
									itemHeader.typeId		 = typeId;
									itemHeader.numberOfBytes = numberOfBytes;
									memoryFile.write(&itemHeader, sizeof(RendererRuntime::v1Scene::ItemHeader));
								}

								// Write down the scene item type specific data, if there is any
								if (0 != numberOfBytes)
								{
									switch (typeId)
									{
										case RendererRuntime::CameraSceneItem::TYPE_ID:
											// Nothing here
											break;

										case RendererRuntime::LightSceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::LightItem lightItem;

											// Read properties
											::detail::optionalLightTypeProperty(rapidJsonValueItem, "LightType", lightItem.lightType);
											JsonHelper::optionalRgbColorProperty(rapidJsonValueItem, "Color", lightItem.color);
											JsonHelper::optionalUnitNProperty(rapidJsonValueItem, "Radius", &lightItem.radius, 1);
											JsonHelper::optionalAngleProperty(rapidJsonValueItem, "InnerAngle", lightItem.innerAngle);
											JsonHelper::optionalAngleProperty(rapidJsonValueItem, "OuterAngle", lightItem.outerAngle);
											JsonHelper::optionalUnitNProperty(rapidJsonValueItem, "NearClipDistance", &lightItem.nearClipDistance, 1);

											// Sanity checks
											if (lightItem.color[0] < 0.0f || lightItem.color[1] < 0.0f || lightItem.color[2] < 0.0f)
											{
												throw std::runtime_error("All light item color components must be positive");
											}
											if (lightItem.lightType != RendererRuntime::LightSceneItem::LightType::DIRECTIONAL && lightItem.radius <= 0.0f)
											{
												throw std::runtime_error("For point or spot light items the radius must be greater as zero");
											}
											if (lightItem.lightType == RendererRuntime::LightSceneItem::LightType::DIRECTIONAL && lightItem.radius != 0.0f)
											{
												throw std::runtime_error("For directional light items the radius must be zero");
											}
											if (lightItem.innerAngle < 0.0f)
											{
												throw std::runtime_error("The inner spot light angle must be >= 0 degrees");
											}
											if (lightItem.outerAngle >= glm::radians(90.0f))
											{
												throw std::runtime_error("The outer spot light angle must be < 90 degrees");
											}
											if (lightItem.innerAngle >= lightItem.outerAngle)
											{
												throw std::runtime_error("The inner spot light angle must be smaller as the outer spot light angle");
											}
											if (lightItem.nearClipDistance < 0.0f)
											{
												throw std::runtime_error("The spot light near clip distance must be greater as zero");
											}

											// Write down
											memoryFile.write(&lightItem, sizeof(RendererRuntime::v1Scene::LightItem));
											break;
										}

										case RendererRuntime::SunlightSceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::SunlightItem sunlightItem;

											// Read properties
											JsonHelper::optionalTimeOfDayProperty(rapidJsonValueItem, "SunriseTime", sunlightItem.sunriseTime);
											JsonHelper::optionalTimeOfDayProperty(rapidJsonValueItem, "SunsetTime", sunlightItem.sunsetTime);
											JsonHelper::optionalAngleProperty(rapidJsonValueItem, "EastDirection", sunlightItem.eastDirection);
											JsonHelper::optionalAngleProperty(rapidJsonValueItem, "AngleOfIncidence", sunlightItem.angleOfIncidence);
											JsonHelper::optionalTimeOfDayProperty(rapidJsonValueItem, "TimeOfDay", sunlightItem.timeOfDay);

											// Write down
											memoryFile.write(&sunlightItem, sizeof(RendererRuntime::v1Scene::SunlightItem));
											break;
										}

										case RendererRuntime::MeshSceneItem::TYPE_ID:
										case RendererRuntime::SkeletonMeshSceneItem::TYPE_ID:
										{
											// Skeleton mesh scene item
											if (RendererRuntime::SkeletonMeshSceneItem::TYPE_ID == typeId)
											{
												RendererRuntime::v1Scene::SkeletonMeshItem skeletonMeshItem;

												// Optional skeleton animation: Map the source asset ID to the compiled asset ID
												skeletonMeshItem.skeletonAnimationAssetId = RendererRuntime::getInvalid<RendererRuntime::AssetId>();
												JsonHelper::optionalCompiledAssetId(input, rapidJsonValueItem, "SkeletonAnimation", skeletonMeshItem.skeletonAnimationAssetId);

												// Write down
												memoryFile.write(&skeletonMeshItem, sizeof(RendererRuntime::v1Scene::SkeletonMeshItem));
											}

											// Mesh scene item
											RendererRuntime::v1Scene::MeshItem meshItem;

											// Map the source asset ID to the compiled asset ID
											meshItem.meshAssetId = JsonHelper::getCompiledAssetId(input, rapidJsonValueItem, "Mesh");

											// Optional sub-mesh material asset IDs to be able to overwrite the original material asset ID of sub-meshes
											std::vector<RendererRuntime::AssetId> subMeshMaterialAssetIds;
											if (rapidJsonValueItem.HasMember("SubMeshMaterials"))
											{
												const rapidjson::Value& rapidJsonValueSubMeshMaterialAssetIds = rapidJsonValueItem["SubMeshMaterials"];
												const uint32_t numberOfSubMeshMaterialAssetIds = rapidJsonValueSubMeshMaterialAssetIds.Size();
												subMeshMaterialAssetIds.resize(numberOfSubMeshMaterialAssetIds);
												for (uint32_t i = 0; i < numberOfSubMeshMaterialAssetIds; ++i)
												{
													// Empty string means "Don't overwrite the original material asset ID of the sub-mesh"
													const std::string valueAsString = rapidJsonValueSubMeshMaterialAssetIds[i].GetString();
													subMeshMaterialAssetIds[i] = valueAsString.empty() ? RendererRuntime::getInvalid<RendererRuntime::AssetId>() : StringHelper::getAssetIdByString(valueAsString, input);
												}
											}
											meshItem.numberOfSubMeshMaterialAssetIds = static_cast<uint32_t>(subMeshMaterialAssetIds.size());

											// Write down
											memoryFile.write(&meshItem, sizeof(RendererRuntime::v1Scene::MeshItem));
											if (!subMeshMaterialAssetIds.empty())
											{
												// Write down all sub-mesh material asset IDs
												memoryFile.write(subMeshMaterialAssetIds.data(), sizeof(RendererRuntime::AssetId) * subMeshMaterialAssetIds.size());
											}
											break;
										}

										case RendererRuntime::SkySceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::SkyItem skyItem;
											::detail::readMaterialSceneItem(input, sortedMaterialPropertyVector, rapidJsonValueItem, skyItem);

											// Write down
											memoryFile.write(&skyItem, sizeof(RendererRuntime::v1Scene::SkyItem));
											if (!sortedMaterialPropertyVector.empty())
											{
												// Write down all material properties
												memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
											}
											break;
										}

										case RendererRuntime::VolumeSceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::VolumeItem volumeItem;
											::detail::readMaterialSceneItem(input, sortedMaterialPropertyVector, rapidJsonValueItem, volumeItem);

											// Write down
											memoryFile.write(&volumeItem, sizeof(RendererRuntime::v1Scene::VolumeItem));
											if (!sortedMaterialPropertyVector.empty())
											{
												// Write down all material properties
												memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
											}
											break;
										}

										case RendererRuntime::GrassSceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::GrassItem grassItem;
											::detail::readMaterialSceneItem(input, sortedMaterialPropertyVector, rapidJsonValueItem, grassItem);

											// Write down
											memoryFile.write(&grassItem, sizeof(RendererRuntime::v1Scene::GrassItem));
											if (!sortedMaterialPropertyVector.empty())
											{
												// Write down all material properties
												memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
											}
											break;
										}

										case RendererRuntime::TerrainSceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::TerrainItem terrainItem;
											::detail::readMaterialSceneItem(input, sortedMaterialPropertyVector, rapidJsonValueItem, terrainItem);

											// Write down
											memoryFile.write(&terrainItem, sizeof(RendererRuntime::v1Scene::TerrainItem));
											if (!sortedMaterialPropertyVector.empty())
											{
												// Write down all material properties
												memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
											}
											break;
										}

										case RendererRuntime::ParticlesSceneItem::TYPE_ID:
										{
											RendererRuntime::v1Scene::ParticlesItem particlesItem;
											::detail::readMaterialSceneItem(input, sortedMaterialPropertyVector, rapidJsonValueItem, particlesItem);

											// Write down
											memoryFile.write(&particlesItem, sizeof(RendererRuntime::v1Scene::ParticlesItem));
											if (!sortedMaterialPropertyVector.empty())
											{
												// Write down all material properties
												memoryFile.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}

			// Write LZ4 compressed output
			if (!memoryFile.writeLz4CompressedDataByVirtualFilename(RendererRuntime::v1Scene::FORMAT_TYPE, RendererRuntime::v1Scene::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str()))
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
